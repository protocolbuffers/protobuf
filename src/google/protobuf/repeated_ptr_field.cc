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
#include <string>

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

RepeatedPtrFieldBase::Rep* RepeatedPtrFieldBase::InternalExtend(
    int extend_amount) {
  ABSL_DCHECK(extend_amount > 0);
  constexpr size_t ptr_size = sizeof(rep()->elements[0]);
  int new_capacity = capacity_ + extend_amount;
  Arena* arena = GetArena();
  new_capacity = internal::CalculateReserveSize<void*, kRepHeaderSize>(
      capacity_, new_capacity);
  ABSL_DCHECK_LE(
      static_cast<int64_t>(new_capacity),
      static_cast<int64_t>(
          (std::numeric_limits<size_t>::max() - kRepHeaderSize) / ptr_size))
      << "Requested size is too large to fit into size_t.";
  size_t bytes = kRepHeaderSize + ptr_size * new_capacity;
  Rep* new_rep;
  if (arena == nullptr) {
    internal::SizedPtr res = internal::AllocateAtLeast(bytes);
    new_capacity = static_cast<int>((res.n - kRepHeaderSize) / ptr_size);
    new_rep = reinterpret_cast<Rep*>(res.p);
  } else {
    new_rep = reinterpret_cast<Rep*>(Arena::CreateArray<char>(arena, bytes));
  }

  if (!has_rep()) {
    new_rep->cleared_size = 0;
    new_rep->arena = arena;
  } else {
    Rep* old_rep = rep();
    memcpy(new_rep, old_rep,
           (size_ + old_rep->cleared_size) * ptr_size + kRepHeaderSize);

    size_t old_size = capacity_ * ptr_size + kRepHeaderSize;
    if (arena == nullptr) {
      internal::SizedDelete(old_rep, old_size);
    } else {
      arena->ReturnArrayMemory(old_rep, old_size);
    }
  }

  arena_or_elements_ = new_rep->elements;
  capacity_ = new_capacity;
  return new_rep;
}

void RepeatedPtrFieldBase::Reserve(int capacity) {
  int delta = capacity - Capacity();
  if (delta > 0) {
    InternalExtend(delta);
  }
}

void RepeatedPtrFieldBase::DestroyStrings() {
  PROTOBUF_ALWAYS_INLINE_CALL Destroy<GenericTypeHandler<std::string>>();
}

void RepeatedPtrFieldBase::DestroyProtos() {
  PROTOBUF_ALWAYS_INLINE_CALL Destroy<GenericTypeHandler<MessageLite>>();

  // TODO:  Eliminate this store when invoked from the destructor,
  // since it is dead.
  arena_or_elements_ = nullptr;
}

template <typename F>
void* RepeatedPtrFieldBase::AddInternal(F factory) {
  absl::PrefetchToLocalCache(arena_or_elements_);
  Rep* r;
  if (SizeAtCapacity()) {
    r = InternalExtend(1);
  } else {
    r = rep();
    if (r->cleared_size != 0) {
      --r->cleared_size;
      return r->elements[ExchangeCurrentSize(size_ + 1)];
    }
  }
  void*& result = r->elements[ExchangeCurrentSize(size_ + 1)];
  result = factory(r->arena);
  return result;
}

void* RepeatedPtrFieldBase::AddMessageLite(ElementFactory factory) {
  return AddInternal(factory);
}

void* RepeatedPtrFieldBase::AddString() {
  return AddInternal([](Arena* arena) { return NewStringElement(arena); });
}

void RepeatedPtrFieldBase::CloseGap(int start, int num) {
  if (!has_rep()) return;
  // Close up a gap of "num" elements starting at offset "start".
  Rep* r = rep();
  int n = size_ + r->cleared_size;
  for (int i = start + num; i < n; ++i) {
    r->elements[i - num] = r->elements[i];
  }
  ExchangeCurrentSize(size_ - num);
}

MessageLite* RepeatedPtrFieldBase::AddMessage(const MessageLite* prototype) {
  return static_cast<MessageLite*>(
      AddInternal([prototype](Arena* a) { return prototype->New(a); }));
}

void InternalOutOfLineDeleteMessageLite(MessageLite* message) {
  delete message;
}

template <>
void RepeatedPtrFieldBase::MergeFrom<std::string>(
    const RepeatedPtrFieldBase& from) {
  ABSL_DCHECK_NE(&from, this);
  int new_size = size_ + from.size_;
  auto dst = reinterpret_cast<std::string**>(InternalReserve(new_size));
  auto src = reinterpret_cast<std::string* const*>(from.elements());
  int recycled = std::min(rep()->cleared_size, from.size_);
  auto end = src + from.size_;
  auto end_assign = src + recycled;
  for (; src < end_assign; ++dst, ++src) {
    (*dst)->assign(**src);
  }
  if (Arena* const arena = rep()->arena) {
    for (; src < end; ++dst, ++src) {
      *dst = Arena::Create<std::string>(arena, **src);
    }
  } else {
    for (; src < end; ++dst, ++src) {
      *dst = new std::string(**src);
    }
  }
  ExchangeCurrentSize(new_size);
  rep()->cleared_size -= recycled;
}


int RepeatedPtrFieldBase::MergeIntoClearedMessages(
    const RepeatedPtrFieldBase& from) {
  auto dst = reinterpret_cast<MessageLite**>(elements() + size_);
  auto src = reinterpret_cast<MessageLite* const*>(from.elements());
  int count = std::min(ClearedCount(), from.size_);
  for (int i = 0; i < count; ++i) {
    ABSL_DCHECK(src[i] != nullptr);
#if PROTOBUF_RTTI
    // TODO: remove or replace with a cleaner check.
    ABSL_DCHECK(typeid(*src[i]) == typeid(*src[0]))
        << typeid(*src[i]).name() << " vs " << typeid(*src[0]).name();
#endif
    dst[i]->CheckTypeAndMergeFrom(*src[i]);
  }
  return count;
}

void RepeatedPtrFieldBase::MergeFromConcreteMessage(
    const RepeatedPtrFieldBase& from, CopyFn copy_fn) {
  ABSL_DCHECK_NE(&from, this);
  int new_size = size_ + from.size_;
  void** dst = InternalReserve(new_size);
  const void* const* src = from.elements();
  auto end = src + from.size_;
  if (PROTOBUF_PREDICT_FALSE(rep()->cleared_size > 0)) {
    int recycled = MergeIntoClearedMessages(from);
    dst += recycled;
    src += recycled;
    rep()->cleared_size -= recycled;
  }
  Arena* arena = rep()->arena;
  for (; src < end; ++src, ++dst) {
    *dst = copy_fn(arena, *src);
  }
  ExchangeCurrentSize(new_size);
}

template <>
void RepeatedPtrFieldBase::MergeFrom<MessageLite>(
    const RepeatedPtrFieldBase& from) {
  ABSL_DCHECK_NE(&from, this);
  ABSL_DCHECK(from.size_ > 0);
  int new_size = size_ + from.size_;
  auto dst = reinterpret_cast<MessageLite**>(InternalReserve(new_size));
  auto src = reinterpret_cast<MessageLite const* const*>(from.elements());
  auto end = src + from.size_;
  const MessageLite* prototype = src[0];
  ABSL_DCHECK(prototype != nullptr);
  if (PROTOBUF_PREDICT_FALSE(rep()->cleared_size > 0)) {
    int recycled = MergeIntoClearedMessages(from);
    dst += recycled;
    src += recycled;
    rep()->cleared_size -= recycled;
  }
  Arena* arena = rep()->arena;
  for (; src < end; ++src, ++dst) {
    ABSL_DCHECK(*src != nullptr);
#if PROTOBUF_RTTI
    // TODO: remove or replace with a cleaner check.
    ABSL_DCHECK(typeid(**src) == typeid(*prototype))
        << typeid(**src).name() << " vs " << typeid(*prototype).name();
#endif
    *dst = prototype->New(arena);
    (*dst)->CheckTypeAndMergeFrom(**src);
  }
  ExchangeCurrentSize(new_size);
}

void* NewStringElement(Arena* arena) {
  return Arena::Create<std::string>(arena);
}

}  // namespace internal
}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"
