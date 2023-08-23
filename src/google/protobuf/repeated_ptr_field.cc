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
#include <limits>

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
  if (Capacity() >= new_size) {
    // N.B.: rep_ is non-nullptr because extend_amount is always > 0, hence
    // total_size must be non-zero since it is lower-bounded by new_size.
    return elements() + current_size_;
  }

  new_size = internal::CalculateReserveSize<void*, kRepHeaderSize>(Capacity(),
                                                                   new_size);
  ABSL_CHECK_LE(static_cast<int64_t>(new_size),
                static_cast<int64_t>(
                    (std::numeric_limits<size_t>::max() - kRepHeaderSize) /
                    sizeof(element_)))
      << "Requested size is too large to fit into size_t.";
  size_t bytes = kRepHeaderSize + sizeof(element_) * new_size;
  Arena* arena;
  Rep* new_rep;
  if (has_arena_) {
    arena = GetOwningArena();
    new_rep = reinterpret_cast<Rep*>(Arena::CreateArray<char>(arena, bytes));
  } else {
    arena = nullptr;
    internal::SizedPtr res = internal::AllocateAtLeast(bytes);
    new_size = static_cast<int>((res.n - kRepHeaderSize) / sizeof(element_));
    new_rep = reinterpret_cast<Rep*>(res.p);
  }

  new_rep->arena = arena;
  int new_allocated_size;
  if (using_sbo_) {
    new_rep->elements()[0] = element_;
    new_allocated_size = element_ != nullptr ? 1 : 0;
  } else {
    new_allocated_size = allocated_size_;
    Rep* old_rep = rep();
    if (allocated_size_ > 0) {
      memcpy(new_rep->elements(), elements_,
             allocated_size_ * sizeof(element_));
    }

    const size_t old_size = total_size_ * sizeof(element_) + kRepHeaderSize;
    if (arena == nullptr) {
      internal::SizedDelete(old_rep, old_size);
    } else {
      arena->ReturnArrayMemory(old_rep, old_size);
    }
  }

  using_sbo_ = false;
  allocated_size_ = new_allocated_size;
  total_size_ = new_size;
  elements_ = new_rep->elements();
  return elements_ + current_size_;
}

void RepeatedPtrFieldBase::Reserve(int new_size) {
  if (new_size > current_size_) {
    InternalExtend(new_size - current_size_);
  }
}

void RepeatedPtrFieldBase::DestroyProtos() {
  ABSL_DCHECK((using_sbo_ && element_ != nullptr) ||
              (!using_sbo_ && elements_ != nullptr));
  ABSL_DCHECK(GetOwningArena() == nullptr);
  if (using_sbo_) {
    delete static_cast<MessageLite*>(element_);

  } else {
    Rep* r = rep();
    for (int i = 0; i < allocated_size_; i++) {
      delete static_cast<MessageLite*>(elements_[i]);
    }
    const size_t size = total_size_ * sizeof(element_) + kRepHeaderSize;
    internal::SizedDelete(r, size);
    elements_ = nullptr;
  }
}

void* RepeatedPtrFieldBase::AddOutOfLineHelper(void* obj) {
  if (using_sbo_ && current_size_ == 0) {
    ABSL_DCHECK_EQ(allocated_size(), 0);
    ExchangeCurrentSize(1);
    element_ = obj;
    return obj;
  }
  if (using_sbo_ || (allocated_size_ == total_size_)) {
    InternalExtend(1);  // Equivalent to "Reserve(total_size_ + 1)"
  }
  ++allocated_size_;
  elements_[ExchangeCurrentSize(current_size_ + 1)] = obj;
  return obj;
}

void RepeatedPtrFieldBase::CloseGap(int start, int num) {
  if (using_sbo_) {
    if (start == 0 && num == 1) {
      element_ = nullptr;
    }
  } else {
    // Close up a gap of "num" elements starting at offset "start".
    for (int i = start + num; i < allocated_size_; ++i)
      elements_[i - num] = elements_[i];
    allocated_size_ -= num;
  }
  ExchangeCurrentSize(current_size_ - num);
}

MessageLite* RepeatedPtrFieldBase::AddWeak(const MessageLite* prototype) {
  if (current_size_ < allocated_size()) {
    return reinterpret_cast<MessageLite*>(
        element_at(ExchangeCurrentSize(current_size_ + 1)));
  }
  if (allocated_size() == Capacity()) {
    Reserve(Capacity() + 1);
  }
  MessageLite* result =
      prototype ? prototype->New(GetOwningArena())
                : Arena::CreateMessage<ImplicitWeakMessage>(GetOwningArena());
  if (using_sbo_) {
    ExchangeCurrentSize(current_size_ + 1);
    element_ = result;
  } else {
    ++allocated_size_;
    elements_[ExchangeCurrentSize(current_size_ + 1)] = result;
  }
  return result;
}

}  // namespace internal

}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"
