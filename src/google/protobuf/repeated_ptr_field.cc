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
    return elements() + current_size_;
  }
  constexpr size_t ptr_size = sizeof(rep()->elements[0]);
  Arena* arena = GetOwningArena();
  new_size = internal::CalculateReserveSize<void*, kRepHeaderSize>(Capacity(),
                                                                   new_size);
  ABSL_CHECK_LE(
      static_cast<int64_t>(new_size),
      static_cast<int64_t>(
          (std::numeric_limits<size_t>::max() - kRepHeaderSize) / ptr_size))
      << "Requested size is too large to fit into size_t.";
  size_t bytes = kRepHeaderSize + ptr_size * new_size;
  Rep* new_rep;
  void* old_tagged_ptr = tagged_rep_or_elem_;
  if (arena == nullptr) {
    internal::SizedPtr res = internal::AllocateAtLeast(bytes);
    new_rep = reinterpret_cast<Rep*>(res.p);
    new_rep->capacity = static_cast<int>((res.n - kRepHeaderSize) / ptr_size);
  } else {
    new_rep = reinterpret_cast<Rep*>(Arena::CreateArray<char>(arena, bytes));
    new_rep->capacity = new_size;
  }

  if (using_sso()) {
    new_rep->elements[0] = old_tagged_ptr;
  } else {
    Rep* old_rep =
        reinterpret_cast<Rep*>(reinterpret_cast<uintptr_t>(old_tagged_ptr) - 1);
    if (allocated_size_ > 0) {
      memcpy(new_rep->elements, old_rep->elements, allocated_size_ * ptr_size);
    }

    size_t old_size = Capacity() * ptr_size + kRepHeaderSize;
    if (arena == nullptr) {
      internal::SizedDelete(old_rep, old_size);
    } else {
      arena->ReturnArrayMemory(old_rep, old_size);
    }
  }

  tagged_rep_or_elem_ =
      reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(new_rep) + 1);
  return &new_rep->elements[current_size_];
}

void RepeatedPtrFieldBase::Reserve(int new_size) {
  if (new_size > current_size_) {
    InternalExtend(new_size - current_size_);
  }
}

void RepeatedPtrFieldBase::DestroyProtos() {
  ABSL_DCHECK(arena_ == nullptr);
  void** elems = elements();
  for (int i = 0; i < allocated_size_; i++) {
    delete static_cast<MessageLite*>(elems[i]);
  }
  if (!using_sso()) {
    Rep* r = rep();
    internal::SizedDelete(r, r->capacity * sizeof(elems[0]) + kRepHeaderSize);
  }
}

void* RepeatedPtrFieldBase::AddOutOfLineHelper(void* obj) {
  return AddAllocatedForParse(obj);
}

void RepeatedPtrFieldBase::CloseGap(int start, int num) {
    // Close up a gap of "num" elements starting at offset "start".
  void** elems = elements();
  for (int i = start + num; i < allocated_size_; ++i) elems[i - num] = elems[i];
  allocated_size_ -= num;
  ExchangeCurrentSize(current_size_ - num);
}

MessageLite* RepeatedPtrFieldBase::AddWeak(const MessageLite* prototype) {
  if (current_size_ < allocated_size_) {
    return static_cast<MessageLite*>(
        element_at(ExchangeCurrentSize(current_size_ + 1)));
  }
  MessageLite* result = prototype
                            ? prototype->New(arena_)
                            : Arena::CreateMessage<ImplicitWeakMessage>(arena_);
  return static_cast<MessageLite*>(AddAllocatedForParse(result));
}

void InternalOutOfLineDeleteMessageLite(MessageLite* message) {
  delete message;
}

}  // namespace internal

}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"
