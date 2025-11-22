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

void** RepeatedPtrFieldBase::InternalExtend(int extend_amount, Arena* arena) {
  ABSL_DCHECK(extend_amount > 0);
  ABSL_DCHECK_EQ(arena, GetArena());
  constexpr size_t kPtrSize = sizeof(rep()->elements[0]);
  constexpr size_t kMaxSize = std::numeric_limits<size_t>::max();
  constexpr size_t kMaxCapacity = (kMaxSize - kRepHeaderSize) / kPtrSize;
  const int old_capacity = Capacity();
  Rep* new_rep = nullptr;

  int new_capacity = internal::CalculateReserveSize<void*, kRepHeaderSize>(
      old_capacity, old_capacity + extend_amount);
  {
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
  }

  if (using_sso()) {
    new_rep->capacity = new_capacity;
    new_rep->allocated_size = tagged_rep_or_elem_ != nullptr ? 1 : 0;
    new_rep->elements[0] = tagged_rep_or_elem_;
  } else {
    Rep* old_rep = rep();
    new_rep->capacity = new_capacity;
    new_rep->allocated_size = old_rep->allocated_size;
    memcpy(new_rep->elements, old_rep->elements,
           new_rep->allocated_size * kPtrSize);
    size_t old_total_size = old_capacity * kPtrSize + kRepHeaderSize;
    if (arena == nullptr) {
      internal::SizedDelete(old_rep, old_total_size);
    } else {
      arena->ReturnArrayMemory(old_rep, old_total_size);
    }
  }

  tagged_rep_or_elem_ =
      reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(new_rep) + 1);

  return &new_rep->elements[current_size_];
}

void RepeatedPtrFieldBase::ReserveWithArena(Arena* arena, int capacity) {
  ABSL_DCHECK_EQ(arena, GetArena());
  int delta = capacity - Capacity();
  if (delta > 0) {
    InternalExtend(delta, arena);
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
memswap<InternalMetadataResolverOffsetHelper<RepeatedPtrFieldBase>::value>(
    char* PROTOBUF_RESTRICT, char* PROTOBUF_RESTRICT);

template <typename T, typename CopyElementFn, typename CreateAndMergeFn>
PROTOBUF_ALWAYS_INLINE void RepeatedPtrFieldBase::MergeFromInternal(
    const RepeatedPtrFieldBase& from, Arena* arena, CopyElementFn&& copy_fn,
    CreateAndMergeFn&& create_and_merge_fn) {
  Prefetch5LinesFrom1Line(&from);
  ABSL_DCHECK_EQ(arena, GetArena());
  ABSL_DCHECK_NE(&from, this);
  int new_size = current_size_ + from.current_size_;
  auto dst = reinterpret_cast<T**>(InternalReserve(new_size, arena));
  auto src = reinterpret_cast<T* const*>(from.elements());
  auto end = src + from.current_size_;
  auto end_assign = src + std::min(ClearedCount(), from.current_size_);
  for (; src < end_assign; ++dst, ++src) {
    copy_fn(arena, *dst, **src);
  }
  for (; src < end; ++dst, ++src) {
    *dst = create_and_merge_fn(arena, **src);
  }
  ExchangeCurrentSize(new_size);
  if (new_size > allocated_size()) {
    rep()->allocated_size = new_size;
  }
}

template <typename T, typename CopyElementFn>
PROTOBUF_ALWAYS_INLINE void RepeatedPtrFieldBase::MergeFromInternal(
    const RepeatedPtrFieldBase& from, Arena* arena, CopyElementFn&& copy_fn) {
  if (arena != nullptr) {
    MergeFromInternal<T>(from, arena, std::forward<CopyElementFn>(copy_fn),
                         [](Arena* arena, const T& src) -> T* {
                           return Arena::Create<T>(arena, src);
                         });
  } else {
    MergeFromInternal<T>(
        from, /*arena=*/nullptr, std::forward<CopyElementFn>(copy_fn),
        [](Arena* arena, const T& src) -> T* { return new T(src); });
  }
}

template <>
void RepeatedPtrFieldBase::MergeFrom<std::string>(
    const RepeatedPtrFieldBase& from, Arena* arena) {
  MergeFromInternal<std::string>(
      from, arena, [](Arena* arena, std::string* dst, const std::string& src) {
        dst->assign(src);
      });
}


int RepeatedPtrFieldBase::MergeIntoClearedMessages(
    const RepeatedPtrFieldBase& from) {
  Prefetch5LinesFrom1Line(&from);
  auto dst = reinterpret_cast<MessageLite**>(elements() + current_size_);
  auto src = reinterpret_cast<MessageLite* const*>(from.elements());
  int count = std::min(ClearedCount(), from.current_size_);
  const ClassData* class_data = GetClassData(*src[0]);
  for (int i = 0; i < count; ++i) {
    ABSL_DCHECK(src[i] != nullptr);
    dst[i]->MergeFromWithClassData(*src[i], class_data);
  }
  return count;
}

void RepeatedPtrFieldBase::MergeFromConcreteMessage(
    const RepeatedPtrFieldBase& from, Arena* arena, CopyFn copy_fn) {
  Prefetch5LinesFrom1Line(&from);
  ABSL_DCHECK_EQ(arena, GetArena());
  ABSL_DCHECK_NE(&from, this);
  int new_size = current_size_ + from.current_size_;
  void** dst = InternalReserve(new_size, arena);
  const void* const* src = from.elements();
  auto end = src + from.current_size_;
  constexpr ptrdiff_t kPrefetchstride = 1;
  if (ABSL_PREDICT_FALSE(ClearedCount() > 0)) {
    int recycled = MergeIntoClearedMessages(from);
    dst += recycled;
    src += recycled;
  }
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
    const RepeatedPtrFieldBase& from, Arena* arena) {
  ABSL_DCHECK(from.current_size_ > 0);
  const ClassData* class_data =
      GetClassData(*reinterpret_cast<const MessageLite*>(from.element_at(0)));
  MergeFromInternal<MessageLite>(
      from, arena,
      [class_data](Arena* arena, MessageLite* dst, const MessageLite& src) {
        dst->MergeFromWithClassData(src, class_data);
      },
      [class_data](Arena* arena, const MessageLite& src) -> MessageLite* {
        MessageLite* dst = class_data->New(arena);
        dst->MergeFromWithClassData(src, class_data);
        return dst;
      });
}

}  // namespace internal
}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"
