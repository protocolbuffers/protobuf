// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
// https://developers.google.com/protocol-buffers/
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

// Author: kenton@google.com (Kenton Varda)
//  Based on original Protocol Buffers design by
//  Sanjay Ghemawat, Jeff Dean, and others.

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>

#include "absl/log/absl_check.h"
#include "google/protobuf/arena.h"
#include "google/protobuf/implicit_weak_message.h"
#include "google/protobuf/port.h"
#include "google/protobuf/repeated_field.h"

// Must be included last.
#include "google/protobuf/port_def.inc"

namespace google {
namespace protobuf {

namespace internal {

void** RepeatedPtrFieldBase::InternalExtend(int extend_amount) {
  int new_size = current_size_ + extend_amount;
  if (total_size_ >= new_size) {
    // N.B.: rep_ is non-nullptr because extend_amount is always > 0, hence
    // total_size must be non-zero since it is lower-bounded by new_size.
    return elements() + current_size_;
  }

  Arena* arena = GetOwningArena();
  new_size = internal::CalculateReserveSize<void*, kRepHeaderSize>(total_size_,
                                                                   new_size);
  ABSL_CHECK_LE(static_cast<int64_t>(new_size),
                static_cast<int64_t>(
                    (std::numeric_limits<size_t>::max() - kRepHeaderSize) /
                    sizeof(rep()->elements[0])))
      << "Requested size is too large to fit into size_t.";
  size_t bytes = kRepHeaderSize + sizeof(rep()->elements[0]) * new_size;
  Rep* new_rep;
  void* old_tagged_ptr = tagged_rep_or_elem_;
  if (arena == nullptr) {
    internal::SizedPtr res = internal::AllocateAtLeast(bytes);
    new_size =
        static_cast<int>((res.n - kRepHeaderSize) / sizeof(rep()->elements[0]));
    new_rep = reinterpret_cast<Rep*>(res.p);
  } else {
    new_rep = reinterpret_cast<Rep*>(Arena::CreateArray<char>(arena, bytes));
  }

  if (using_sso()) {
    new_rep->elements[0] = old_tagged_ptr;
    new_rep->allocated_size = old_tagged_ptr != nullptr ? 1 : 0;
  } else {
    if (old_tagged_ptr) {
      Rep* old_rep = reinterpret_cast<Rep*>(
          reinterpret_cast<uintptr_t>(old_tagged_ptr) - 1);
      if (old_rep->allocated_size > 0) {
        memcpy(new_rep->elements, old_rep->elements,
               old_rep->allocated_size * sizeof(rep()->elements[0]));
      }
      new_rep->allocated_size = old_rep->allocated_size;

      const size_t old_size =
          total_size_ * sizeof(rep()->elements[0]) + kRepHeaderSize;
      if (arena == nullptr) {
        internal::SizedDelete(old_rep, old_size);
      } else {
        arena_->ReturnArrayMemory(old_rep, old_size);
      }
    } else {
      new_rep->allocated_size = 0;
    }
  }

  tagged_rep_or_elem_ =
      reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(new_rep) + 1);
  total_size_ = new_size;
  return &new_rep->elements[current_size_];
}

void RepeatedPtrFieldBase::Reserve(int new_size) {
  if (new_size > current_size_) {
    InternalExtend(new_size - current_size_);
  }
}

void RepeatedPtrFieldBase::DestroyProtos() {
  ABSL_DCHECK(tagged_rep_or_elem_);
  ABSL_DCHECK(arena_ == nullptr);
  if (using_sso()) {
    delete static_cast<MessageLite*>(tagged_rep_or_elem_);

  } else {
    Rep* r = rep();
    int n = r->allocated_size;
    void* const* elements = r->elements;
    for (int i = 0; i < n; i++) {
      delete static_cast<MessageLite*>(elements[i]);
    }
    const size_t size = total_size_ * sizeof(elements[0]) + kRepHeaderSize;
    internal::SizedDelete(r, size);
    tagged_rep_or_elem_ = nullptr;
  }
}

void* RepeatedPtrFieldBase::AddOutOfLineHelper(void* obj) {
  if (tagged_rep_or_elem_ == nullptr) {
    ABSL_DCHECK_EQ(current_size_, 0);
    ABSL_DCHECK(using_sso());
    ABSL_DCHECK_EQ(allocated_size(), 0);
    ExchangeCurrentSize(1);
    tagged_rep_or_elem_ = obj;
    return obj;
  }
  if (using_sso() || rep()->allocated_size == total_size_) {
    InternalExtend(1);  // Equivalent to "Reserve(total_size_ + 1)"
  }
  Rep* r = rep();
  ++r->allocated_size;
  r->elements[ExchangeCurrentSize(current_size_ + 1)] = obj;
  return obj;
}

void RepeatedPtrFieldBase::CloseGap(int start, int num) {
  if (using_sso()) {
    if (start == 0 && num == 1) {
      tagged_rep_or_elem_ = nullptr;
    }
  } else {
    // Close up a gap of "num" elements starting at offset "start".
    Rep* r = rep();
    for (int i = start + num; i < r->allocated_size; ++i)
      r->elements[i - num] = r->elements[i];
    r->allocated_size -= num;
  }
  ExchangeCurrentSize(current_size_ - num);
}

MessageLite* RepeatedPtrFieldBase::AddWeak(const MessageLite* prototype) {
  if (current_size_ < allocated_size()) {
    return reinterpret_cast<MessageLite*>(
        element_at(ExchangeCurrentSize(current_size_ + 1)));
  }
  if (allocated_size() == total_size_) {
    Reserve(total_size_ + 1);
  }
  MessageLite* result = prototype
                            ? prototype->New(arena_)
                            : Arena::CreateMessage<ImplicitWeakMessage>(arena_);
  if (using_sso()) {
    ExchangeCurrentSize(current_size_ + 1);
    tagged_rep_or_elem_ = result;
  } else {
    Rep* r = rep();
    ++r->allocated_size;
    r->elements[ExchangeCurrentSize(current_size_ + 1)] = result;
  }
  return result;
}

}  // namespace internal

}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"
