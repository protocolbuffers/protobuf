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
#include <string>

#include "absl/base/prefetch.h"
#include "absl/log/absl_check.h"
#include "google/protobuf/arena.h"
#include "google/protobuf/message_lite.h"
#include "google/protobuf/port.h"

// Must be included last.
#include "google/protobuf/port_def.inc"

namespace google {
namespace protobuf {
namespace internal {

constexpr int kMinAllocatedCapacity = 4;

void** RepeatedPtrFieldBase::InternalExtend(int extend_amount) {
  ABSL_DCHECK(extend_amount > 0);
  constexpr size_t ptr_size = sizeof(tagged_rep_or_elem_);
  int capacity = Capacity();
  Arena* arena = GetArena();
  // Extend is always amortized.
  // Growth factor of 2.
  int new_capacity = capacity + std::max(capacity, extend_amount);
  new_capacity = std::max(new_capacity, kMinAllocatedCapacity);
  size_t bytes = ptr_size * new_capacity;
  void** new_elements;
  if (arena == nullptr) {
    internal::SizedPtr res = internal::AllocateAtLeast(bytes);
    new_capacity = static_cast<int>(res.n / ptr_size);
    new_elements = static_cast<void**>(res.p);
  } else {
    new_elements =
        reinterpret_cast<void**>(Arena::CreateArray<char>(arena, bytes));
  }

  if (using_sso()) {
    new_elements[0] = tagged_rep_or_elem_;
  } else {
    void** old_elements = unsafe_elements();
    memcpy(new_elements, old_elements, capacity * ptr_size);

    size_t old_bytes = capacity * ptr_size;
    if (arena == nullptr) {
      internal::SizedDelete(old_elements, old_bytes);
    } else {
      arena->ReturnArrayMemory(old_elements, old_bytes);
    }
  }
  // Zero out the rest of buffer to distinguish allocated elements.
  std::fill(new_elements + capacity, new_elements + new_capacity, nullptr);

  tagged_rep_or_elem_ =
      reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(new_elements) + 1);
  capacity_proxy_ = new_capacity - kSSOCapacity;
  return new_elements + current_size_;
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

template <typename F>
void* RepeatedPtrFieldBase::AddInternal(F factory) {
  Arena* const arena = GetArena();
  // In a common case (no growth), the code only touches the last element.
  absl::PrefetchToLocalCache(static_cast<void**>(tagged_rep_or_elem_) +
                             current_size_);
  if (tagged_rep_or_elem_ == nullptr) {
    ExchangeCurrentSize(1);
    tagged_rep_or_elem_ = factory(arena);
    return tagged_rep_or_elem_;
  }
  if (using_sso()) {
    if (current_size_ == 0) {
      ExchangeCurrentSize(1);
      return tagged_rep_or_elem_;
    }
    void** last = InternalExtend(1);
    ExchangeCurrentSize(2);
    *last = factory(arena);
    return *last;
  }
  void** last;
  if (PROTOBUF_PREDICT_FALSE(SizeAtCapacity())) {
    last = InternalExtend(1);
  } else {
    last = unsafe_elements() + current_size_;
    if (*last != nullptr) {
      ExchangeCurrentSize(current_size_ + 1);
      return *last;
    }
  }
  ExchangeCurrentSize(current_size_ + 1);
  *last = factory(arena);
  return *last;
}

void* RepeatedPtrFieldBase::AddMessageLite(ElementFactory factory) {
  return AddInternal(factory);
}

void* RepeatedPtrFieldBase::AddString() {
  return AddInternal([](Arena* arena) { return NewStringElement(arena); });
}

void RepeatedPtrFieldBase::CloseGap(int start, int num) {
  if (using_sso()) {
    if (start == 0 && num == 1) {
      tagged_rep_or_elem_ = nullptr;
    }
  } else {
    // Close up a gap of "num" elements starting at offset "start".
    void** elems = elements();
    std::copy(elems + start + num, elems + Capacity(), elems + start);
    std::fill(elems + Capacity() - num, elems + Capacity(), nullptr);
  }
  ExchangeCurrentSize(current_size_ - num);
}

void** RepeatedPtrFieldBase::FirstUnallocatedSlow(void** first, void** last) {
  return std::partition_point(first, last,
                              [](void* p) { return p != nullptr; });
}

MessageLite* RepeatedPtrFieldBase::AddMessage(const MessageLite* prototype) {
  return static_cast<MessageLite*>(
      AddInternal([prototype](Arena* a) { return prototype->New(a); }));
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
  ABSL_DCHECK_NE(&from, this);
  int new_size = current_size_ + from.current_size_;
  auto dst = reinterpret_cast<std::string**>(InternalReserve(new_size));
  auto src = reinterpret_cast<std::string* const*>(from.elements());
  auto end = src + from.current_size_;
  auto end_assign = src + std::min(Capacity(), from.current_size_);
  for (; src < end_assign && *dst != nullptr; ++dst, ++src) {
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
}


int RepeatedPtrFieldBase::MergeIntoClearedMessages(
    const RepeatedPtrFieldBase& from) {
  auto dst = reinterpret_cast<MessageLite**>(elements() + current_size_);
  auto src = reinterpret_cast<MessageLite* const*>(from.elements());
  int count = std::min(Capacity(), from.current_size_);
  int i = 0;
  for (; i < count && dst[i] != nullptr; ++i) {
    ABSL_DCHECK(src[i] != nullptr);
#if PROTOBUF_RTTI
    // TODO: remove or replace with a cleaner check.
    ABSL_DCHECK(typeid(*src[i]) == typeid(*src[0]))
        << typeid(*src[i]).name() << " vs " << typeid(*src[0]).name();
#endif
    dst[i]->CheckTypeAndMergeFrom(*src[i]);
  }
  return i;
}

void RepeatedPtrFieldBase::MergeFromConcreteMessage(
    const RepeatedPtrFieldBase& from, CopyFn copy_fn) {
  ABSL_DCHECK_NE(&from, this);
  int new_size = current_size_ + from.current_size_;
  void** dst = InternalReserve(new_size);
  const void* const* src = from.elements();
  auto end = src + from.current_size_;
  if (PROTOBUF_PREDICT_FALSE(HasCleared())) {
    int recycled = MergeIntoClearedMessages(from);
    dst += recycled;
    src += recycled;
  }
  Arena* arena = GetArena();
  for (; src < end; ++src, ++dst) {
    *dst = copy_fn(arena, *src);
  }
  ExchangeCurrentSize(new_size);
}

template <>
void RepeatedPtrFieldBase::MergeFrom<MessageLite>(
    const RepeatedPtrFieldBase& from) {
  ABSL_DCHECK_NE(&from, this);
  ABSL_DCHECK(from.current_size_ > 0);
  int new_size = current_size_ + from.current_size_;
  auto dst = reinterpret_cast<MessageLite**>(InternalReserve(new_size));
  auto src = reinterpret_cast<MessageLite const* const*>(from.elements());
  auto end = src + from.current_size_;
  const MessageLite* prototype = src[0];
  ABSL_DCHECK(prototype != nullptr);
  if (PROTOBUF_PREDICT_FALSE(HasCleared())) {
    int recycled = MergeIntoClearedMessages(from);
    dst += recycled;
    src += recycled;
  }
  Arena* arena = GetArena();
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
