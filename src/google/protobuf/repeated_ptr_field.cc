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
#include <utility>

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

void** RepeatedPtrFieldBase::InternalExtendDisjoint(int extend_amount,
                                                    Arena* arena) {
  ABSL_DCHECK(extend_amount > 0);
  ABSL_DCHECK_EQ(arena, GetArena());

  constexpr size_t kMaxSize = std::numeric_limits<size_t>::max();
  constexpr size_t kPtrSize = sizeof(void*);
  constexpr size_t kMaxCapacity = (kMaxSize - Rep::HeaderSize()) / kPtrSize;
  const int old_capacity = Capacity();
  void* new_rep_ptr;

  int new_capacity = internal::CalculateReserveSize<void*, Rep::HeaderSize()>(
      old_capacity, old_capacity + extend_amount);
  {
    ABSL_DCHECK_LE(new_capacity, kMaxCapacity)
        << "New capacity is too large to fit into internal representation";
    const size_t new_size = Rep::HeaderSize() + kPtrSize * new_capacity;
    if (arena == nullptr) {
      const internal::SizedPtr alloc = internal::AllocateAtLeast(new_size);
      new_capacity = static_cast<int>((alloc.n - Rep::HeaderSize()) / kPtrSize);
      new_rep_ptr = alloc.p;
    } else {
      new_rep_ptr = Arena::CreateArray<char>(arena, new_size);
    }
  }

  Rep* new_rep;
  if (using_sso()) {
    new_rep = new (new_rep_ptr) Rep(new_capacity, tagged_rep_or_elem_);
  } else {
    new_rep = new (new_rep_ptr) Rep(arena, new_capacity, disjoint_rep());
  }

  tagged_rep_or_elem_ =
      reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(new_rep) + 1);

  return new_rep->elements() + current_size_;
}

void RepeatedPtrFieldBase::ReserveWithArena(Arena* arena,
#ifdef PROTOBUF_INTERNAL_CONTIGUOUS_REPEATED_PTR_FIELD_LAYOUT
                                            AllocTraits alloc_traits,
                                            bool use_contiguous_layout,
#endif
                                            int capacity) {
  ABSL_DCHECK_EQ(arena, GetArena());
  int delta = capacity - Capacity();
  if (delta > 0) {
    InternalExtend(delta,
#ifdef PROTOBUF_INTERNAL_CONTIGUOUS_REPEATED_PTR_FIELD_LAYOUT
                   alloc_traits, use_contiguous_layout,
#endif
                   arena);
  }
}

void RepeatedPtrFieldBase::DestroyProtos(
#ifdef PROTOBUF_INTERNAL_CONTIGUOUS_REPEATED_PTR_FIELD_LAYOUT
    AllocTraits alloc_traits
#endif
) {
  PROTOBUF_ALWAYS_INLINE_CALL Destroy<GenericTypeHandler<MessageLite>>(
#ifdef PROTOBUF_INTERNAL_CONTIGUOUS_REPEATED_PTR_FIELD_LAYOUT
      alloc_traits
#endif
  );

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
    auto* r = rep();
    for (int i = start + num; i < r->allocated_size(); ++i)
      r->elements()[i - num] = r->elements()[i];
    r->set_allocated_size(r->allocated_size() - num);
  }
  ExchangeCurrentSize(current_size_ - num);
}

RepeatedPtrFieldBase::StorageBlock*
RepeatedPtrFieldBase::StorageBlock::PlacementNew(void* mem_begin,
                                                 PrevBlockPtr prev,
                                                 uint32_t size_bytes) {
  StorageBlock* block = reinterpret_cast<StorageBlock*>(
      reinterpret_cast<uint8_t*>(mem_begin) + HeaderDistance(size_bytes));
  block->prev_ = prev;
  block->size_bytes_ = size_bytes;
  return block;
}

void RepeatedPtrFieldBase::StorageBlock::InitPointersToNewObjects(
    void** elements, uint32_t num_new_objects, AllocTraits alloc_traits,
    void* new_objects_begin) {
  ABSL_DCHECK_GT(new_objects_begin, MemBegin());
  ABSL_DCHECK_EQ(new_objects_begin,
                 reinterpret_cast<const uint8_t*>(MemBegin()) +
                     (ObjectStoreSize(size_bytes_, alloc_traits.size_of) -
                      num_new_objects) *
                         alloc_traits.size_of);
  uint8_t* begin = reinterpret_cast<uint8_t*>(new_objects_begin);
  for (uint32_t i = 0; i < num_new_objects; ++i) {
    elements[i] = begin + i * alloc_traits.size_of;
  }
}

RepeatedPtrFieldBase::LiveBlock* RepeatedPtrFieldBase::LiveBlock::New(
    Arena* arena, AllocTraits alloc_traits, uint32_t size_bytes,
    uint32_t capacity, void* sso_object) {
  void* block_mem = AllocateBlockMem(arena, size_bytes);
  uint32_t prev_capacity = sso_object != nullptr ? 1 : 0;
  LiveBlock* block =
      LiveBlock::PlacementNew(block_mem, size_bytes, capacity, prev_capacity,
                              /*allocated_elements=*/nullptr);

  void** elements = block->elements();
  if (sso_object != nullptr) {
    elements[0] = sso_object;
  }
  block->InitPointersToNewObjects(alloc_traits, block_mem, prev_capacity);
  block->full_rep().storage_blocks.set_sso_object(sso_object);
  return block;
}

RepeatedPtrFieldBase::LiveBlock* RepeatedPtrFieldBase::LiveBlock::New(
    Arena* arena, AllocTraits alloc_traits, uint32_t size_bytes,
    uint32_t capacity, LiveBlock* prev, uint32_t prev_capacity,
    size_t prev_block_object_store_size,
    size_t repurposed_block_object_store_size) {
  ABSL_DCHECK_EQ(prev_block_object_store_size,
                 ObjectStoreSize(prev->SizeBytes(), prev->capacity(),
                                 alloc_traits.size_of));
  ABSL_DCHECK_LE(prev_capacity + repurposed_block_object_store_size, capacity);
  void* block_mem = AllocateBlockMem(arena, size_bytes);
  LiveBlock* block = LiveBlock::PlacementNew(
      block_mem, size_bytes, capacity, prev->allocated_size(),
      prev->full_rep().allocated_elements);

  std::memcpy(block->elements(), prev->elements(),
              prev_capacity * sizeof(void*));

  auto* prev_storage_block = StorageBlock::PlacementNew(
      prev->MemBegin(), prev->full_rep().storage_blocks, prev->SizeBytes());
  prev_storage_block->InitPointersToNewObjects(
      block->elements() + prev_capacity, repurposed_block_object_store_size,
      alloc_traits,
      reinterpret_cast<uint8_t*>(prev->MemBegin()) +
          prev_block_object_store_size * alloc_traits.size_of);

  block->InitPointersToNewObjects(
      alloc_traits, block_mem,
      prev_capacity + repurposed_block_object_store_size);

  block->full_rep().storage_blocks.set_prev_block(prev_storage_block);
  return block;
}

void* RepeatedPtrFieldBase::LiveBlock::UninitializedElementAt(uint32_t index) {
  ABSL_DCHECK_GE(index, allocated_size());
  ABSL_DCHECK_LT(index, capacity());
  return elements()[index];
}

void* RepeatedPtrFieldBase::LiveBlock::AllocateBlockMem(Arena* arena,
                                                        uint32_t size_bytes) {
  return arena == nullptr
             ? internal::Allocate(size_bytes)
             : arena->AllocateAligned(size_bytes, alignof(LiveBlock));
}

void RepeatedPtrFieldBase::LiveBlock::InitPointersToNewObjects(
    AllocTraits alloc_traits, void* block_mem, uint32_t prev_capacity) {
  void** elements = this->elements();
  uint32_t capacity = this->capacity();
  uint8_t* new_objects_begin = reinterpret_cast<uint8_t*>(block_mem);
  for (uint32_t i = prev_capacity; i < capacity; ++i) {
    elements[i] =
        new_objects_begin + (i - prev_capacity) * alloc_traits.size_of;
  }
}

RepeatedPtrFieldBase::StorageBlock*
RepeatedPtrFieldBase::LiveBlock::to_storage_block() {
  return reinterpret_cast<StorageBlock*>(this);
}

RepeatedPtrFieldBase::Rep::Rep(int capacity, void* sso_object)
    : RepBase(capacity, sso_object != nullptr ? 1 : 0) {
  elements()[0] = sso_object;
}

RepeatedPtrFieldBase::Rep::Rep(Arena* arena, int capacity, Rep* old_rep)
    : RepBase(capacity, old_rep->allocated_size()) {
  memcpy(elements(), old_rep->elements(), allocated_size() * kPtrSize);
  size_t old_total_size = HeaderSize() + old_rep->capacity() * kPtrSize;
  if (arena == nullptr) {
    internal::SizedDelete(old_rep, old_total_size);
  } else {
    arena->ReturnArrayMemory(old_rep, old_total_size);
  }
}

void InternalOutOfLineDeleteMessageLite(MessageLite* message) {
#ifdef PROTOBUF_INTERNAL_CONTIGUOUS_REPEATED_PTR_FIELD_LAYOUT
  message->~MessageLite();
#else
  delete message;
#endif
}

template PROTOBUF_EXPORT_TEMPLATE_DEFINE void
memswap<InternalMetadataResolverOffsetHelper<RepeatedPtrFieldBase>::value>(
    char* PROTOBUF_RESTRICT, char* PROTOBUF_RESTRICT);

template <typename T, typename CopyElementFn, typename CreateAndMergeFn>
PROTOBUF_ALWAYS_INLINE void RepeatedPtrFieldBase::MergeFromInternal(
    const RepeatedPtrFieldBase& from,
#ifdef PROTOBUF_INTERNAL_CONTIGUOUS_REPEATED_PTR_FIELD_LAYOUT
    AllocTraits alloc_traits,
#endif
    Arena* arena, CopyElementFn&& copy_fn,
    CreateAndMergeFn&& create_and_merge_fn) {
  Prefetch5LinesFrom1Line(&from);
  ABSL_DCHECK_EQ(arena, GetArena());
  ABSL_DCHECK_NE(&from, this);
  int new_size = current_size_ + from.current_size_;
  void** dst = InternalReserve(new_size,
#ifdef PROTOBUF_INTERNAL_CONTIGUOUS_REPEATED_PTR_FIELD_LAYOUT
                               alloc_traits,
                               GenericTypeHandler<T>::UseContiguousLayout(),
#endif
                               arena);
  const int cleared_count = ClearedCount();
#ifdef PROTOBUF_INTERNAL_CONTIGUOUS_REPEATED_PTR_FIELD_LAYOUT
  if constexpr (GenericTypeHandler<T>::UseContiguousLayout()) {
    if (using_sso() && tagged_rep_or_elem_ == nullptr) {
      tagged_rep_or_elem_ = AllocTraits::Allocate(alloc_traits, arena);
    }
  }
#endif
  auto src = reinterpret_cast<T* const*>(from.elements());
  auto end = src + from.current_size_;
  auto end_assign = src + std::min(cleared_count, from.current_size_);
  for (; src < end_assign; ++dst, ++src) {
    copy_fn(arena, reinterpret_cast<T*>(*dst), **src);
  }
  for (; src < end; ++dst, ++src) {
#ifdef PROTOBUF_INTERNAL_CONTIGUOUS_REPEATED_PTR_FIELD_LAYOUT
    create_and_merge_fn(arena, *dst, **src);
#else
    *dst = create_and_merge_fn(arena, **src);
#endif
  }
  ExchangeCurrentSize(new_size);
  if (new_size > allocated_size()) {
    rep()->set_allocated_size(new_size);
  }
}

template <typename T, typename CopyElementFn>
PROTOBUF_ALWAYS_INLINE void RepeatedPtrFieldBase::MergeFromInternal(
    const RepeatedPtrFieldBase& from,
#ifdef PROTOBUF_INTERNAL_CONTIGUOUS_REPEATED_PTR_FIELD_LAYOUT
    AllocTraits alloc_traits,
#endif
    Arena* arena, CopyElementFn&& copy_fn) {
  if (arena != nullptr) {
    MergeFromInternal<T>(
        from,
#ifdef PROTOBUF_INTERNAL_CONTIGUOUS_REPEATED_PTR_FIELD_LAYOUT
        alloc_traits,
#endif
        arena, std::forward<CopyElementFn>(copy_fn),
#ifdef PROTOBUF_INTERNAL_CONTIGUOUS_REPEATED_PTR_FIELD_LAYOUT
        [](Arena* arena, void*& dst, const T& src) {
          if constexpr (GenericTypeHandler<T>::UseContiguousLayout()) {
            Arena::PlacementConstruct<T>(arena, dst, src);
          } else {
            dst = Arena::Create<T>(arena, src);
          }
        }
#else
        [](Arena* arena, const T& src) -> T* {
          return Arena::Create<T>(arena, src);
        }
#endif
    );
  } else {
    MergeFromInternal<T>(
        from,
#ifdef PROTOBUF_INTERNAL_CONTIGUOUS_REPEATED_PTR_FIELD_LAYOUT
        alloc_traits,
#endif
        /*arena=*/nullptr, std::forward<CopyElementFn>(copy_fn),
#ifdef PROTOBUF_INTERNAL_CONTIGUOUS_REPEATED_PTR_FIELD_LAYOUT
        [](Arena* arena, void*& dst, const T& src) {
          if constexpr (GenericTypeHandler<T>::UseContiguousLayout()) {
            new (dst) T(src);
          } else {
            dst = new T(src);
          }
        }
#else
        [](Arena* arena, const T& src) -> T* { return new T(src); }
#endif
    );
  }
}

template <>
void RepeatedPtrFieldBase::MergeFrom<GenericTypeHandler<std::string>>(
    const RepeatedPtrFieldBase& from, Arena* arena) {
  MergeFromInternal<std::string>(
      from,
#ifdef PROTOBUF_INTERNAL_CONTIGUOUS_REPEATED_PTR_FIELD_LAYOUT
      AllocTraits::FromType<std::string>(),
#endif
      arena, [](Arena* arena, std::string* dst, const std::string& src) {
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
    const RepeatedPtrFieldBase& from,
#ifdef PROTOBUF_INTERNAL_CONTIGUOUS_REPEATED_PTR_FIELD_LAYOUT
    AllocTraits alloc_traits,
#endif
    Arena* arena, CopyFn copy_fn) {
  Prefetch5LinesFrom1Line(&from);
  ABSL_DCHECK_EQ(arena, GetArena());
  ABSL_DCHECK_NE(&from, this);
  int new_size = current_size_ + from.current_size_;
  void** dst = InternalReserve(
      new_size,
#ifdef PROTOBUF_INTERNAL_CONTIGUOUS_REPEATED_PTR_FIELD_LAYOUT
      alloc_traits, GenericTypeHandler<MessageLite>::UseContiguousLayout(),
#endif
      arena);
  const int cleared_count = ClearedCount();
#ifdef PROTOBUF_INTERNAL_CONTIGUOUS_REPEATED_PTR_FIELD_LAYOUT
  if (using_sso() && tagged_rep_or_elem_ == nullptr) {
    tagged_rep_or_elem_ = AllocTraits::Allocate(alloc_traits, arena);
  }
#endif
  const void* const* src = from.elements();
  auto end = src + from.current_size_;
  constexpr ptrdiff_t kPrefetchstride = 1;
  if (ABSL_PREDICT_FALSE(cleared_count > 0)) {
    int recycled = MergeIntoClearedMessages(from);
    dst += recycled;
    src += recycled;
  }
  if (from.current_size_ >= kPrefetchstride) {
    auto prefetch_end = end - kPrefetchstride;
    for (; src < prefetch_end; ++src, ++dst) {
      auto next = src + kPrefetchstride;
      absl::PrefetchToLocalCache(*next);
#ifdef PROTOBUF_INTERNAL_CONTIGUOUS_REPEATED_PTR_FIELD_LAYOUT
      copy_fn(arena, *dst, *src);
#else
      *dst = copy_fn(arena, *src);
#endif
    }
  }
  for (; src < end; ++src, ++dst) {
#ifdef PROTOBUF_INTERNAL_CONTIGUOUS_REPEATED_PTR_FIELD_LAYOUT
    copy_fn(arena, *dst, *src);
#else
    *dst = copy_fn(arena, *src);
#endif
  }
  ExchangeCurrentSize(new_size);
  if (new_size > allocated_size()) {
    rep()->set_allocated_size(new_size);
  }
}

template <>
void RepeatedPtrFieldBase::MergeFrom<GenericTypeHandler<MessageLite>>(
    const RepeatedPtrFieldBase& from, Arena* arena) {
  ABSL_DCHECK(from.current_size_ > 0);
  const ClassData* class_data =
      GetClassData(*reinterpret_cast<const MessageLite*>(from.element_at(0)));
  MergeFromInternal<MessageLite>(
      from,
#ifdef PROTOBUF_INTERNAL_CONTIGUOUS_REPEATED_PTR_FIELD_LAYOUT
      AllocTraits(class_data),
#endif
      arena,
      [class_data](Arena* arena, MessageLite* dst, const MessageLite& src) {
        dst->MergeFromWithClassData(src, class_data);
      },
#ifdef PROTOBUF_INTERNAL_CONTIGUOUS_REPEATED_PTR_FIELD_LAYOUT
      [class_data](Arena* arena, void*& dst, const MessageLite& src) {
        auto* msg = class_data->PlacementNew(dst, arena);
        msg->MergeFromWithClassData(src, class_data);
      }
#else
      [class_data](Arena* arena, const MessageLite& src) -> MessageLite* {
        MessageLite* dst = class_data->New(arena);
        dst->MergeFromWithClassData(src, class_data);
        return dst;
      }
#endif
  );
}

}  // namespace internal
}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"
