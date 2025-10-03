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

  void** new_elems = new_rep->elements;

  if (using_sso()) {
    new_rep->capacity = new_capacity;
    new_elems[0] = tagged_rep_or_elem_;
  } else {
    Rep* old_rep = rep();
    void** old_elems = old_rep->elements;
    new_rep->capacity = new_capacity;
    memcpy(new_elems, old_elems, old_capacity * kPtrSize);
    size_t old_total_size = old_capacity * kPtrSize + kRepHeaderSize;
    if (arena == nullptr) {
      internal::SizedDelete(old_rep, old_total_size);
    } else {
      arena->ReturnArrayMemory(old_rep, old_total_size);
    }
  }

  std::fill_n(reinterpret_cast<char*>(new_elems + old_capacity),
              (new_capacity - old_capacity) * kPtrSize, '\0');

  tagged_rep_or_elem_ =
      reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(new_rep) + 1);

  return &new_elems[current_size_];
}

void RepeatedPtrFieldBase::Reserve(int capacity, Arena* arena) {
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
    void** elems = rep()->elements;
    const int allocated_size = AllocatedSize();
    for (int i = start + num; i < allocated_size; ++i) {
      elems[i - num] = elems[i];
    }
    for (int i = allocated_size - num; i < allocated_size; ++i) {
      elems[i] = nullptr;
    }
  }
  ExchangeCurrentSize(current_size_ - num);
}

void InternalOutOfLineDeleteMessageLite(MessageLite* message) {
  delete message;
}

template PROTOBUF_EXPORT_TEMPLATE_DEFINE void
memswap<InternalMetadataResolverOffsetHelper<RepeatedPtrFieldBase>::value>(
    char* PROTOBUF_RESTRICT, char* PROTOBUF_RESTRICT);

template <>
void RepeatedPtrFieldBase::MergeFrom<std::string>(
    const RepeatedPtrFieldBase& from, Arena* arena) {
  Prefetch5LinesFrom1Line(&from);
  ABSL_DCHECK_EQ(arena, GetArena());
  ABSL_DCHECK_NE(&from, this);
  if (from.empty()) return;
  int new_size = current_size_ + from.current_size_;
  auto dst = reinterpret_cast<std::string**>(InternalReserve(new_size, arena));
  auto src = reinterpret_cast<std::string* const*>(from.elements());
  auto end = src + from.current_size_;
  for (; src < end && *dst != nullptr; ++dst, ++src) {
    (*dst)->assign(**src);
  }
  if (arena != nullptr) {
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
  Prefetch5LinesFrom1Line(&from);
  ABSL_DCHECK_NE(&from, this);
  ABSL_DCHECK(HasCleared());
  if (from.empty()) return 0;
  auto dst = reinterpret_cast<MessageLite**>(elements() + current_size_);
  auto dst_end = dst + Capacity();
  auto src = reinterpret_cast<MessageLite* const*>(from.elements());
  auto src_end = src + from.current_size_;
  const ClassData* class_data = GetClassData(*src[0]);
  int count = 0;
  for (; src < src_end && dst < dst_end && *dst != nullptr; ++dst, ++src) {
    ABSL_DCHECK(*src != nullptr);
    (*dst)->MergeFromWithClassData(**src, class_data);
    ++count;
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
  if (ABSL_PREDICT_FALSE(HasCleared())) {
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
}

template <>
void RepeatedPtrFieldBase::MergeFrom<MessageLite>(
    const RepeatedPtrFieldBase& from, Arena* arena) {
  Prefetch5LinesFrom1Line(&from);
  ABSL_DCHECK_EQ(arena, GetArena());
  ABSL_DCHECK_NE(&from, this);
  ABSL_DCHECK(from.current_size_ > 0);
  int new_size = current_size_ + from.current_size_;
  auto dst = reinterpret_cast<MessageLite**>(InternalReserve(new_size, arena));
  auto src = reinterpret_cast<MessageLite const* const*>(from.elements());
  auto end = src + from.current_size_;
  const ClassData* class_data = GetClassData(*src[0]);
  if (ABSL_PREDICT_FALSE(HasCleared())) {
    int recycled = MergeIntoClearedMessages(from);
    dst += recycled;
    src += recycled;
  }
  for (; src < end; ++src, ++dst) {
    ABSL_DCHECK(*src != nullptr);
    *dst = class_data->New(arena);
    (*dst)->MergeFromWithClassData(**src, class_data);
  }
  ExchangeCurrentSize(new_size);
}

}  // namespace internal
}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"
