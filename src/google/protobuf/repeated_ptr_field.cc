// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// Author: kenton@google.com (Kenton Varda)
//  Based on original Protocol Buffers design by
//  Sanjay Ghemawat, Jeff Dean, and others.

#include "google/protobuf/repeated_ptr_field.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>
#include <new>
#include <string>

#include "absl/base/optimization.h"
#include "absl/base/prefetch.h"
#include "absl/log/absl_check.h"
#include "google/protobuf/arena.h"
#include "google/protobuf/message_lite.h"
#include "google/protobuf/port.h"
#include "google/protobuf/repeated_field.h"

// Must be included last.
#include "google/protobuf/port_def.inc"

namespace google {
namespace protobuf {

namespace internal {

MessageLite* CloneSlow(Arena* arena, const MessageLite& value) {
  auto* msg = value.New(arena);
  msg->CheckTypeAndMergeFrom(value);
  return msg;
}
std::string* CloneSlow(Arena* arena, const std::string& value) {
  return Arena::Create<std::string>(arena, value);
}

void** RepeatedPtrFieldBase::InternalExtend(int extend_amount) {
  ABSL_DCHECK(extend_amount > 0);
  constexpr size_t kPtrSize = sizeof(rep()->elements[0]);
  constexpr size_t kMaxSize = std::numeric_limits<size_t>::max();
  constexpr size_t kMaxCapacity = (kMaxSize - kRepHeaderSize) / kPtrSize;
  const int old_capacity = Capacity();
  Arena* arena = GetArena();
  Rep* new_rep = nullptr;
  {
    int new_capacity = internal::CalculateReserveSize<void*, kRepHeaderSize>(
        old_capacity, old_capacity + extend_amount);
    ABSL_DCHECK_LE(new_capacity, kMaxCapacity)
        << "New capacity is too large to fit into internal representation";
    const size_t new_size = kRepHeaderSize + kPtrSize * new_capacity;
    if (arena == nullptr) {
      const internal::SizedPtr alloc = internal::AllocateAtLeast(new_size);
      new_capacity = static_cast<int>((alloc.n - kRepHeaderSize) / kPtrSize);
      new_rep = reinterpret_cast<Rep*>(alloc.p);
    } else {
      auto* alloc = Arena::CreateArray<char>(arena, new_size);
      new_rep = reinterpret_cast<Rep*>(alloc);
    }
    capacity_proxy_ = new_capacity - kSSOCapacity;
  }

  if (using_sso()) {
    new_rep->allocated_size = tagged_rep_or_elem_ != nullptr ? 1 : 0;
    new_rep->elements[0] = tagged_rep_or_elem_;
  } else {
    Rep* old_rep = rep();
    memcpy(new_rep, old_rep,
           old_rep->allocated_size * kPtrSize + kRepHeaderSize);
    size_t old_size = old_capacity * kPtrSize + kRepHeaderSize;
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

void RepeatedPtrFieldBase::Reserve(int capacity) {
  int delta = capacity - Capacity();
  if (delta > 0) {
    InternalExtend(delta);
  }
}

void RepeatedPtrFieldBase::DestroyProtos() {
  PROTOBUF_ALWAYS_INLINE_CALL Destroy<GenericTypeHandler<MessageLite>>();

  // TODO:  Eliminate this store when invoked from the destructor,
  // since it is dead.
  tagged_rep_or_elem_ = nullptr;
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

void InternalOutOfLineDeleteMessageLite(MessageLite* message) {
  delete message;
}

template PROTOBUF_EXPORT_TEMPLATE_DEFINE void
memswap<ArenaOffsetHelper<RepeatedPtrFieldBase>::value>(
    char* PROTOBUF_RESTRICT, char* PROTOBUF_RESTRICT);

template <>
void RepeatedPtrFieldBase::MergeFrom<std::string>(
    const RepeatedPtrFieldBase& from) {
  Prefetch5LinesFrom1Line(&from);
  ABSL_DCHECK_NE(&from, this);
  int new_size = current_size_ + from.current_size_;
  auto dst = reinterpret_cast<std::string**>(InternalReserve(new_size));
  auto src = reinterpret_cast<std::string* const*>(from.elements());
  auto end = src + from.current_size_;
  auto end_assign = src + std::min(ClearedCount(), from.current_size_);
  for (; src < end_assign; ++dst, ++src) {
    (*dst)->assign(**src);
  }
  if (Arena* const arena = arena_) {
    for (; src < end; ++dst, ++src) {
      *dst = Arena::Create<std::string>(arena, **src);
    }
  } else {
    for (; src < end; ++dst, ++src) {
      *dst = new std::string(**src);
    }
  }
  ExchangeCurrentSize(new_size);
  if (new_size > allocated_size()) {
    rep()->allocated_size = new_size;
  }
}


int RepeatedPtrFieldBase::MergeIntoClearedMessages(
    const RepeatedPtrFieldBase& from) {
  Prefetch5LinesFrom1Line(&from);
  auto dst = reinterpret_cast<MessageLite**>(elements() + current_size_);
  auto src = reinterpret_cast<MessageLite* const*>(from.elements());
  int count = std::min(ClearedCount(), from.current_size_);
  for (int i = 0; i < count; ++i) {
    ABSL_DCHECK(src[i] != nullptr);
    ABSL_DCHECK(TypeId::Get(*src[i]) == TypeId::Get(*src[0]))
        << src[i]->GetTypeName() << " vs " << src[0]->GetTypeName();
    dst[i]->CheckTypeAndMergeFrom(*src[i]);
  }
  return count;
}

void RepeatedPtrFieldBase::MergeFromConcreteMessage(
    const RepeatedPtrFieldBase& from, CopyFn copy_fn) {
  Prefetch5LinesFrom1Line(&from);
  ABSL_DCHECK_NE(&from, this);
  int new_size = current_size_ + from.current_size_;
  void** dst = InternalReserve(new_size);
  const void* const* src = from.elements();
  auto end = src + from.current_size_;
  constexpr ptrdiff_t kPrefetchstride = 1;
  if (ABSL_PREDICT_FALSE(ClearedCount() > 0)) {
    int recycled = MergeIntoClearedMessages(from);
    dst += recycled;
    src += recycled;
  }
  Arena* arena = GetArena();
  if (from.current_size_ >= kPrefetchstride) {
    auto prefetch_end = end - kPrefetchstride;
    for (; src < prefetch_end; ++src, ++dst) {
      auto next = src + kPrefetchstride;
      absl::PrefetchToLocalCache(*next);
      *dst = copy_fn(arena, *src);
    }
  }
  for (; src < end; ++src, ++dst) {
    *dst = copy_fn(arena, *src);
  }
  ExchangeCurrentSize(new_size);
  if (new_size > allocated_size()) {
    rep()->allocated_size = new_size;
  }
}

template <>
void RepeatedPtrFieldBase::MergeFrom<MessageLite>(
    const RepeatedPtrFieldBase& from) {
  Prefetch5LinesFrom1Line(&from);
  ABSL_DCHECK_NE(&from, this);
  ABSL_DCHECK(from.current_size_ > 0);
  int new_size = current_size_ + from.current_size_;
  auto dst = reinterpret_cast<MessageLite**>(InternalReserve(new_size));
  auto src = reinterpret_cast<MessageLite const* const*>(from.elements());
  auto end = src + from.current_size_;
  const MessageLite* prototype = src[0];
  ABSL_DCHECK(prototype != nullptr);
  if (ABSL_PREDICT_FALSE(ClearedCount() > 0)) {
    int recycled = MergeIntoClearedMessages(from);
    dst += recycled;
    src += recycled;
  }
  Arena* arena = GetArena();
  for (; src < end; ++src, ++dst) {
    ABSL_DCHECK(*src != nullptr);
    ABSL_DCHECK(TypeId::Get(**src) == TypeId::Get(*prototype))
        << (**src).GetTypeName() << " vs " << prototype->GetTypeName();
    *dst = prototype->New(arena);
    (*dst)->CheckTypeAndMergeFrom(**src);
  }
  ExchangeCurrentSize(new_size);
  if (new_size > allocated_size()) {
    rep()->allocated_size = new_size;
  }
}

}  // namespace internal
}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"
