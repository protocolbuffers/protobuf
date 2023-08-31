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

#include "google/protobuf/repeated_ptr_field.h"

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
  int old_size = size();
  int old_capacity = Capacity();
  int new_size = old_size + extend_amount;
  if (old_capacity >= new_size) {
    // N.B.: rep_ is non-nullptr because extend_amount is always > 0, hence
    // total_size must be non-zero since it is lower-bounded by new_size.
    return elements() + old_size;
  }
  constexpr size_t ptr_size = sizeof(rep()->elements[0]);
  Arena* arena = GetOwningArena();
  new_size = internal::CalculateReserveSize<void*, kRepHeaderSize>(
      using_sso() ? 0 : old_capacity, new_size);
  ABSL_CHECK_LE(
      static_cast<int64_t>(new_size),
      static_cast<int64_t>(
          (std::numeric_limits<size_t>::max() - kRepHeaderSize) / ptr_size))
      << "Requested size is too large to fit into size_t.";
  size_t bytes = kRepHeaderSize + ptr_size * new_size;
  Rep* new_rep;
  TaggedVoidPointer old_tagged_ptr = rep_or_elem_;
  if (arena == nullptr) {
    internal::SizedPtr res = internal::AllocateAtLeast(bytes);
    new_size = static_cast<int>((res.n - kRepHeaderSize) / ptr_size);
    new_rep = reinterpret_cast<Rep*>(res.p);
  } else {
    new_rep = reinterpret_cast<Rep*>(Arena::CreateArray<char>(arena, bytes));
  }

  new_rep->arena = arena;
  if (using_sso()) {
    new_rep->allocated_size = old_tagged_ptr.is_zero() ? 0 : 1;
    new_rep->elements[0] = old_tagged_ptr.ptr();
  } else {
    Rep* old_rep = rep();
    if (old_rep->allocated_size > 0) {
      memcpy(new_rep->elements, old_rep->elements,
             old_rep->allocated_size * ptr_size);
    }
    new_rep->allocated_size = old_rep->allocated_size;

    size_t old_size = old_capacity * ptr_size + kRepHeaderSize;
    if (arena == nullptr) {
      internal::SizedDelete(old_rep, old_size);
    } else {
      arena->ReturnArrayMemory(old_rep, old_size);
    }
  }

  rep_or_elem_ = TaggedVoidPointer(new_rep, kRepTag);
  rep_metadata_ = {.size = old_size, .capacity = new_size};

  ABSL_DCHECK_EQ(GetOwningArena(), arena);
  ABSL_DCHECK_EQ(size(), old_size);
  ABSL_DCHECK_GT(Capacity(), old_capacity);

  return &new_rep->elements[size()];
}

void RepeatedPtrFieldBase::Reserve(int new_size) {
  if (new_size > size()) {
    InternalExtend(new_size - size());
  }
}

void RepeatedPtrFieldBase::DestroyProtos() {
  ABSL_DCHECK(!rep_or_elem_.is_zero());
  ABSL_DCHECK(GetOwningArena() == nullptr);
  if (using_sso()) {
    delete static_cast<MessageLite*>(rep_or_elem_.ptr());

  } else {
    Rep* r = rep();
    int n = r->allocated_size;
    void* const* elements = r->elements;
    for (int i = 0; i < n; i++) {
      delete static_cast<MessageLite*>(elements[i]);
    }
    const size_t size = Capacity() * sizeof(elements[0]) + kRepHeaderSize;
    internal::SizedDelete(r, size);
    rep_or_elem_ = TaggedVoidPointer(nullptr);
  }
}

void* RepeatedPtrFieldBase::AddOutOfLineHelper(void* obj) {
  if (rep_or_elem_.is_zero()) {
    ABSL_DCHECK_EQ(size(), 0);
    ABSL_DCHECK(using_sso());
    ABSL_DCHECK_EQ(allocated_size(), 0);
    ExchangeCurrentSize(1);
    rep_or_elem_ = TaggedVoidPointer(obj);
    return obj;
  }
  if (using_sso() || rep()->allocated_size == Capacity()) {
    InternalExtend(1);  // Equivalent to "Reserve(Capacity() + 1)"
  }
  Rep* r = rep();
  ++r->allocated_size;
  r->elements[ExchangeCurrentSize(size() + 1)] = obj;
  return obj;
}

void RepeatedPtrFieldBase::CloseGap(int start, int num) {
  if (using_sso()) {
    if (start == 0 && num == 1) {
      rep_or_elem_ = TaggedVoidPointer(nullptr);
    }
  } else {
    // Close up a gap of "num" elements starting at offset "start".
    Rep* r = rep();
    for (int i = start + num; i < r->allocated_size; ++i)
      r->elements[i - num] = r->elements[i];
    r->allocated_size -= num;
  }
  ExchangeCurrentSize(size() - num);
}

MessageLite* RepeatedPtrFieldBase::AddWeak(const MessageLite* prototype) {
  if (size() < allocated_size()) {
    return reinterpret_cast<MessageLite*>(
        element_at(ExchangeCurrentSize(size() + 1)));
  }
  if (allocated_size() == Capacity()) {
    Reserve(Capacity() + 1);
  }
  Arena* arena = GetOwningArena();
  MessageLite* result = prototype
                            ? prototype->New(arena)
                            : Arena::CreateMessage<ImplicitWeakMessage>(arena);
  if (using_sso()) {
    ExchangeCurrentSize(size() + 1);
    rep_or_elem_ = TaggedVoidPointer(result);
  } else {
    Rep* r = rep();
    ++r->allocated_size;
    r->elements[ExchangeCurrentSize(size() + 1)] = result;
  }
  return result;
}

void InternalOutOfLineDeleteMessageLite(MessageLite* message) {
  delete message;
}

}  // namespace internal

}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"
