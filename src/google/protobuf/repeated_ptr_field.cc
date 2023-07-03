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
#include <utility>

#include "absl/log/absl_check.h"
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
    return rep_.elements() + current_size_;
  }
  ElementsRep old_rep = rep_;
  Arena* arena = GetOwningArena();
  new_size = internal::CalculateReserveSize<ElementsRep::Element,
                                            ElementsRep::kHeaderSize>(
      total_size_, new_size);
  ABSL_CHECK_LE(static_cast<int64_t>(new_size),
                static_cast<int64_t>((std::numeric_limits<size_t>::max() -
                                      ElementsRep::kHeaderSize) /
                                     sizeof(ElementsRep::Element)))
      << "Requested size is too large to fit into size_t.";
  size_t bytes = ElementsRep::BytesNeeded(new_size);
  if (arena == nullptr) {
    internal::SizedPtr res = internal::AllocateAtLeast(bytes);
    new_size = ElementsRep::MaxElements(res.n);
    rep_ = ElementsRep(static_cast<unsigned char*>(res.p));
  } else {
    rep_ = ElementsRep(Arena::CreateArray<unsigned char>(arena, bytes));
  }
  const int old_total_size = std::exchange(total_size_, new_size);
  if (old_rep.allocated()) {
    if (old_rep.num_allocated() > 0) {
      memcpy(rep_.elements(), old_rep.elements(),
             old_rep.num_allocated() * sizeof(ElementsRep::Element));
    }
    rep_.num_allocated() = old_rep.num_allocated();

    const size_t old_size = ElementsRep::BytesNeeded(old_total_size);
    if (arena == nullptr) {
      internal::SizedDelete(old_rep.data(), old_size);
    } else {
      arena_->ReturnArrayMemory(old_rep.data(), old_size);
    }
  } else {
    rep_.num_allocated() = 0;
  }
  return rep_.elements() + current_size_;
}

void RepeatedPtrFieldBase::Reserve(int new_size) {
  if (new_size > current_size_) {
    InternalExtend(new_size - current_size_);
  }
}

void RepeatedPtrFieldBase::DestroyProtos() {
  ABSL_DCHECK(rep_.allocated());
  ABSL_DCHECK(arena_ == nullptr);
  int n = rep_.num_allocated();
  void* const* elements = rep_.elements();
  for (int i = 0; i < n; i++) {
    delete static_cast<MessageLite*>(elements[i]);
  }
  internal::SizedDelete(rep_.data(), ElementsRep::BytesNeeded(total_size_));
  rep_ = ElementsRep(nullptr);
}

void* RepeatedPtrFieldBase::AddOutOfLineHelper(void* obj) {
  if (!rep_.allocated() || rep_.num_allocated() == total_size_) {
    InternalExtend(1);  // Equivalent to "Reserve(total_size_ + 1)"
  }
  ++rep_.num_allocated();
  rep_.elements()[ExchangeCurrentSize(current_size_ + 1)] = obj;
  return obj;
}

void RepeatedPtrFieldBase::CloseGap(int start, int num) {
  if (!rep_.allocated()) return;
  // Close up a gap of "num" elements starting at offset "start".
  for (int i = start + num; i < rep_.num_allocated(); ++i)
    rep_.elements()[i - num] = rep_.elements()[i];
  ExchangeCurrentSize(current_size_ - num);
  rep_.num_allocated() -= num;
}

MessageLite* RepeatedPtrFieldBase::AddWeak(const MessageLite* prototype) {
  if (rep_.allocated() && current_size_ < rep_.num_allocated()) {
    return reinterpret_cast<MessageLite*>(
        rep_.elements()[ExchangeCurrentSize(current_size_ + 1)]);
  }
  if (!rep_.allocated() || rep_.num_allocated() == total_size_) {
    Reserve(total_size_ + 1);
  }
  ++rep_.num_allocated();
  MessageLite* result = prototype
                            ? prototype->New(arena_)
                            : Arena::CreateMessage<ImplicitWeakMessage>(arena_);
  rep_.elements()[ExchangeCurrentSize(current_size_ + 1)] = result;
  return result;
}

}  // namespace internal

}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"
