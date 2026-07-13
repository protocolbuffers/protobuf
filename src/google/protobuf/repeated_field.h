// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// Author: kenton@google.com (Kenton Varda)
//  Based on original Protocol Buffers design by
//  Sanjay Ghemawat, Jeff Dean, and others.
//
// RepeatedField and RepeatedPtrField are used by generated protocol message
// classes to manipulate repeated fields.  These classes are very similar to
// STL's vector, but include a number of optimizations found to be useful
// specifically in the case of Protocol Buffers.  RepeatedPtrField is
// particularly different from STL vector as it manages ownership of the
// pointers that it contains.
//
// This header covers RepeatedField.

#ifndef GOOGLE_PROTOBUF_REPEATED_FIELD_H__
#define GOOGLE_PROTOBUF_REPEATED_FIELD_H__

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <iterator>
#include <limits>
#include <memory>
#include <type_traits>
#include <utility>

#include "absl/base/attributes.h"
#include "absl/base/dynamic_annotations.h"
#include "absl/base/macros.h"
#include "absl/base/no_destructor.h"
#include "absl/base/optimization.h"
#include "absl/log/absl_check.h"
#include "absl/meta/type_traits.h"
#include "absl/strings/cord.h"
#include "google/protobuf/arena.h"
#include "google/protobuf/arena_align.h"
#include "google/protobuf/field_with_arena.h"
#include "google/protobuf/generated_enum_util.h"
#include "google/protobuf/internal_metadata_locator.h"
#include "google/protobuf/internal_visibility.h"
#include "google/protobuf/message_lite.h"
#include "google/protobuf/port.h"
#include "google/protobuf/repeated_ptr_field.h"
#include "google/protobuf/serial_arena.h"

// Must be included last.
#include "google/protobuf/port_def.inc"

#ifdef SWIG
#error "You cannot SWIG proto headers"
#endif


namespace google {
namespace protobuf {

class Message;
class DynamicMessage;
class UnknownField;  // For the allowlist
class UnknownFieldSet;
class DynamicMessage;
class Reflection;

namespace internal {

template <typename ElementType, bool kOrProxy>
class MutableRepeatedFieldProxyImpl;
class EpsCopyInputStream;
class TcParser;
class WireFormat;

template <typename T, int kHeapRepHeaderSize>
constexpr int RepeatedFieldLowerClampLimit() {
  // The header is padded to be at least `sizeof(T)` when it would be smaller
  // otherwise.
  static_assert(sizeof(T) <= kHeapRepHeaderSize, "");
  // We want to pad the minimum size to be a power of two bytes, including the
  // header.
  // The first allocation is kHeapRepHeaderSize bytes worth of elements for a
  // total of 2*kHeapRepHeaderSize bytes. For an 8-byte header, we allocate 8
  // bool, 2 ints, or 1 int64.
  return kHeapRepHeaderSize / sizeof(T);
}

// kRepeatedFieldUpperClampLimit is the lowest signed integer value that
// overflows when multiplied by 2 (which is undefined behavior). Sizes above
// this will clamp to the maximum int value instead of following exponential
// growth when growing a repeated field.
#if defined(__cpp_inline_variables)
inline constexpr int kRepeatedFieldUpperClampLimit =
#else
constexpr int kRepeatedFieldUpperClampLimit =
#endif
    (std::numeric_limits<int>::max() / 2) + 1;

template <typename Element>
class RepeatedIterator;

// Decays `Element` to a value type (excluding `absl::Cord` in release builds
// to avoid a nontrivial copy). This is useful when forwarding element values
// to user-provided callbacks, which prevents leaking a reference to the
// backing storage of the repeated field.
template <typename Element>
using DecayedRepeatedFieldElement =
    std::conditional_t<std::is_same_v<Element, absl::Cord> &&
                           !internal::PerformDebugChecks(),
                       const Element&, Element>;

// Out-of-line abort for MergeFrom self-reference. Declared here (not in the
// call site) so that the failure path does not pull ABSL_CHECK streaming
// support into every inlined MergeFrom instantiation.
[[noreturn]] PROTOBUF_EXPORT void LogSelfMergeAndAbort() noexcept;

// Align to 8 as sanitizers are picky on the alignment of containers to start at
// 8 byte offsets even when compiling for 32 bit platforms.
class alignas(8) HeapRep {
 public:
  explicit HeapRep(uint32_t capacity) : capacity_(capacity) {}
  // Avoid 'implicitly deleted dtor' warnings on certain compilers.
  ~HeapRep() = delete;

  uint32_t capacity() const { return capacity_; }

  template <typename Element>
  const Element* elements() const {
    const char* elements_ptr =
        reinterpret_cast<const char*>(this) + SizeOf<Element>();
    return reinterpret_cast<const Element*>(elements_ptr);
  }
  template <typename Element>
  Element* elements() {
    char* elements_ptr = reinterpret_cast<char*>(this) + SizeOf<Element>();
    return reinterpret_cast<Element*>(elements_ptr);
  }

  // Returns the size of the HeapRep in bytes. Do not use `sizeof(HeapRep)`,
  // since that does not account for the padding we insert after the data
  // members.
  template <typename Element>
  static constexpr size_t SizeOf() {
    // We pad the header to be at least `sizeof(Element)` so that we have
    // power-of-two sized allocations, which enables Arena optimizations.
    return std::max(sizeof(HeapRep), sizeof(Element));
  }

 private:
  uint32_t capacity_;
  // Unused padding which is always present before the data. By having this
  // field be explicitly initialized to 0, the compiler can sometimes merge
  // writes to adjacent memory locations into a single vector store instruction.
  [[maybe_unused]] const uint32_t unused_ = 0;
};

// We use small object optimization (SOO) to store elements inline when possible
// for small repeated fields. We do so in order to avoid memory indirections.
// Note that SOO is disabled on 32-bit platforms due to alignment limitations.

// SOO data is stored in the same space as the size/capacity ints.
enum { kSooCapacityBytes = 2 * sizeof(int) };

inline constexpr uint32_t kNotSooBit = 0x1;
// The number of bits at the start of the `InternalMetadataResolver` offset to
// use for other purposes. Note that this must be <= log2(alignof(void*)).
inline constexpr uint32_t kResolverTaggedBits = 1;
inline constexpr size_t kSooSizeMask = sizeof(void*);

// The number of elements that can be stored in the SOO rep. On 64-bit
// platforms, this is 1 for int64_t, 2 for int32_t, 3 for bool, and 0 for
// absl::Cord. We return 0 to disable SOO on 32-bit platforms.
template <typename T>
constexpr int SooCapacityElements() {
  // RepeatedPtrField always has SOO capacity of 1.
  if constexpr (std::is_pointer_v<T>) {
    return 1;
  }
  // Disable SOO for RepeatedFields on 32-bit platforms.
  if constexpr (sizeof(void*) < 8) return 0;
  return std::min<int>(kSooCapacityBytes / sizeof(T), kSooSizeMask);
}

// The in-memory layout of a repeated field. This has three states: empty, SOO,
// and heap-allocated.
//
// In every state, the first 8 bytes hold the internal metadata offset and size.
//
// In the SOO state, the last 8 bytes hold the data (1 int64_t, 2 int32_t, 8
// bools, etc.).
//
// In the heap-allocated state, the last 8 bytes hold a pointer to the
// heap-allocated buffer of elements.
class SooRep {
 public:
  constexpr SooRep() = default;
  explicit constexpr SooRep(InternalMetadataOffset offset)
      : resolver_(offset) {}

  bool is_soo() const { return (resolver_.Tag() & kNotSooBit) == 0; }
  Arena* arena() const {
    return ResolveTaggedArena<&SooRep::resolver_, kResolverTaggedBits>(this);
  }
  int size() const {
    int res = size_;
    PROTOBUF_ASSUME(res >= 0);
    return res;
  }
  void set_size(int size) {
    ABSL_DCHECK(!is_soo() || size <= kSooCapacityBytes);
    size_ = size;
  }
  int capacity() const {
    ABSL_DCHECK(!this->is_soo());
    return heap_rep_->capacity();
  }
  // Initializes the SooRep in non-SOO mode with the given heap allocation.
  void set_non_soo(HeapRep* heap_rep) {
    resolver_.SetTag(kNotSooBit);
    heap_rep_ = heap_rep;
  }

  HeapRep* heap_rep() const {
    ABSL_DCHECK(!is_soo());
    return heap_rep_;
  }

  template <typename Element>
  const Element* elements(bool is_soo) const {
    ABSL_DCHECK_EQ(is_soo, this->is_soo());
    if (is_soo) {
      return reinterpret_cast<const Element*>(soo_data_);
    } else {
      return heap_rep_->elements<Element>();
    }
  }

  template <typename Element>
  Element* elements(bool is_soo) {
    ABSL_DCHECK_EQ(is_soo, this->is_soo());
    if (is_soo) {
      return reinterpret_cast<Element*>(soo_data_);
    } else {
      return heap_rep_->elements<Element>();
    }
  }

  void swap(SooRep& other) {
    resolver_.SwapTags(other.resolver_);
    internal::memswap<sizeof(SooRep) - offsetof(SooRep, size_)>(
        reinterpret_cast<char*>(&this->size_),
        reinterpret_cast<char*>(&other.size_));
  }

 private:
  TaggedInternalMetadataResolver<kResolverTaggedBits> resolver_;
  uint32_t size_ = 0;
  union {
    char soo_data_[kSooCapacityBytes];
    HeapRep* heap_rep_;

    // NOTE: in some language versions, we can't have a constexpr constructor
    // if we don't initialize all fields, but we don't need to initialize this
    // field, so initialize an empty dummy variable instead.
    std::true_type dummy_ = {};
  };
};

// The untyped common base class for `RepeatedField`.
class RepeatedFieldBase {
 protected:
  constexpr RepeatedFieldBase() = default;
  constexpr explicit RepeatedFieldBase(internal::InternalMetadataOffset offset)
      : soo_rep_(
            offset
                .TranslateForMember<offsetof(RepeatedFieldBase, soo_rep_)>()) {}

  PROTOBUF_FUTURE_ADD_NODISCARD
  int size() const { return soo_rep_.size(); }

  template <typename T>
  auto ResolveArena(T provider) {
    return internal::GetSerialArena(provider);
  }
  // Passing `SelfArena{}` is equivalent to `this`, but it avoids an extra
  // parameter in the ABI. We don't need to duplicate the `this` parameter.
  struct SelfArena {};
  auto ResolveArena(SelfArena) { return GetSerialArena(); }

  internal::SerialArena* GetSerialArena() { return ResolveArena(GetArena()); }

  // Gets the Arena on which this RepeatedField stores its elements.
  [[nodiscard]] Arena* GetArena() {
    // Note: we make this function non-const to force callers to call the
    // `mutable_*` accessor on the repeated field before calling `GetArena()`,
    // which initializes the field if it is split. If this method were const,
    // then `msg.repeated_field().GetArena()` would be valid, but for split
    // repeated fields `repeated_field()` could point to the default split
    // instance. This would always return `nullptr`, which is incorrect when
    // using arenas.
    return this->soo_rep_.arena();
  }

  bool is_soo() const { return this->soo_rep_.is_soo(); }

  // Returns a pointer to the HeapRep struct.
  // pre-condition: the HeapRep must have been allocated, ie !is_soo().
  internal::HeapRep* heap_rep() const {
    ABSL_DCHECK(!is_soo());
    return this->soo_rep_.heap_rep();
  }

  SooRep soo_rep_;
};

template <typename T>
inline constexpr bool kUseAnonymousElementType =
    std::is_trivially_copy_constructible_v<T> &&
    std::is_trivially_destructible_v<T>;

// This class uses generic types internally to allow code sharing.
// For example, RepeatedField<int32_t> and RepeatedField<uint32_t> would have
// the same RepeatedFieldImpl base class.
template <typename T>
class RepeatedFieldImpl : protected RepeatedFieldBase {
 protected:
  friend internal::FieldWithArena<RepeatedFieldImpl>;

  static_assert(!std::is_arithmetic_v<T>,
                "Arithmetic types use the AnonymousType.");

  using RepeatedFieldBase::RepeatedFieldBase;
  RepeatedFieldImpl(internal::InternalMetadataOffset offset, Arena* arena,
                    const RepeatedFieldImpl& rhs)
      : RepeatedFieldImpl(offset) {
    ABSL_DCHECK_EQ(arena, GetArena());
    AnnotateSize(this->kSooCapacityElements, 0);
    if (auto size = rhs.size()) {
      bool is_soo = true;
      if (size > this->kSooCapacityElements) {
        Grow(arena, is_soo, 0, size);
        is_soo = false;
      }
      ExchangeCurrentSize(size);
      UninitializedCopyN(rhs.elements(rhs.is_soo()), size,
                         unsafe_elements(is_soo));
    }
  }

  static constexpr int kSooCapacityElements =
      internal::SooCapacityElements<T>();
  static constexpr const size_t kHeapRepHeaderSize =
      internal::HeapRep::SizeOf<T>();

  template <typename ArenaProvider>
  void ReserveWithArena(ArenaProvider arena_provider, int new_size) {
    const bool was_soo = is_soo();
    if (ABSL_PREDICT_FALSE(new_size > Capacity(was_soo))) {
      Grow(arena_provider, was_soo, size(), new_size);
    }
  }

  void GrowByWithArena(Arena* arena, int grow_by) {
    ReserveWithArena(arena, size() + grow_by);
  }

  int Capacity(bool is_soo) const {
    int res = is_soo ? kSooCapacityElements : this->soo_rep_.capacity();
    PROTOBUF_ASSUME(res >= 0);
    return res;
  }

  PROTOBUF_FUTURE_ADD_NODISCARD int Capacity() const {
    return Capacity(is_soo());
  }

  void set_size(int size) {
    ABSL_DCHECK_LE(size, Capacity());
    this->soo_rep_.set_size(size);
  }

  void MergeFrom(const RepeatedFieldImpl& other) {
    if (ABSL_PREDICT_FALSE(&other == this)) {
      PROTOBUF_NO_MERGE internal::LogSelfMergeAndAbort();
    }
    const bool other_is_soo = other.is_soo();
    if (auto other_size = other.size()) {
      const int old_size = size();
      Reserve(old_size + other_size);
      const bool is_soo = this->is_soo();
      void* dst = static_cast<char*>(elements(is_soo)) +
                  sizeof(T) * ExchangeCurrentSize(old_size + other_size);
      UninitializedCopyN(other.elements(other_is_soo), other_size, dst);
    }
  }

  // Copy constructs `n` instances in place into the array `dst`.
  // This function is identical to `std::uninitialized_copy_n(src, n, dst)`
  // except that we explicit declare the memory to not be aliased, which will
  // result in `memcpy` code generation instead of `memmove` for trivial types.
  static void UninitializedCopyN(const void* PROTOBUF_RESTRICT src, int n,
                                 void* PROTOBUF_RESTRICT dst) {
    if constexpr (kUseAnonymousElementType<T>) {
      memcpy(dst, src, sizeof(T) * n);
    } else {
      std::uninitialized_copy_n(static_cast<const T*>(src), n,
                                static_cast<T*>(dst));
    }
  }

  template <typename ArenaProvider>
  void* AddWithArena(ArenaProvider arena_provider, T value) {
    ABSL_DCHECK_EQ(ResolveArena(arena_provider), GetSerialArena());

    bool is_soo = this->is_soo();
    const int old_size = size();
    int capacity = Capacity(is_soo);
    void* elem = unsafe_elements(is_soo);
    if (ABSL_PREDICT_FALSE(old_size == capacity)) {
      Grow(arena_provider, is_soo, old_size, internal::CheckedAdd(old_size, 1));
      is_soo = false;
      capacity = Capacity(is_soo);
      elem = unsafe_elements(is_soo);
    }
    int new_size = old_size + 1;
    void* result =
        static_cast<char*>(elem) + sizeof(T) * ExchangeCurrentSize(new_size);
    if constexpr (kUseAnonymousElementType<T>) {
      memcpy(result, &value, sizeof(value));
    } else {
      result = ::new (result) T(std::move(value));
    }

    // The below helps the compiler optimize dense loops.
    // Note: we can't call functions in PROTOBUF_ASSUME so use local variables.
    [[maybe_unused]] const bool final_is_soo = this->is_soo();
    PROTOBUF_ASSUME(is_soo == final_is_soo);
    [[maybe_unused]] const int final_size = size();
    PROTOBUF_ASSUME(new_size == final_size);
    [[maybe_unused]] void* const final_elements = unsafe_elements(is_soo);
    PROTOBUF_ASSUME(elem == final_elements);
    [[maybe_unused]] const int final_capacity = Capacity(is_soo);
    PROTOBUF_ASSUME(capacity == final_capacity);

    return result;
  }

  // Replaces size with new_size and returns the previous value of
  // size. This function is intended to be the only place where
  // size is modified, with the exception of `AddInputIterator()`
  // where the size of added items is not known in advance.
  int ExchangeCurrentSize(int new_size) {
    const int prev_size = size();
    this->AnnotateSize(prev_size, new_size);
    set_size(new_size);
    return prev_size;
  }

  template <typename ArenaProvider>
  void* AddUninitializedWithArena(ArenaProvider arena_provider) {
    ABSL_DCHECK_EQ(this->ResolveArena(arena_provider), this->GetSerialArena());

    bool is_soo = this->is_soo();
    const int old_size = size();
    if (ABSL_PREDICT_FALSE(old_size == Capacity(is_soo))) {
      Grow(arena_provider, is_soo, old_size, internal::CheckedAdd(old_size, 1));
      is_soo = false;
    }
    return static_cast<char*>(unsafe_elements(is_soo)) +
           sizeof(T) * ExchangeCurrentSize(old_size + 1);
  }

  void Destroy() {
    const bool is_soo = this->is_soo();

#ifndef NDEBUG
    auto* arena = GetArena();
    // Try to trigger segfault / asan failure in non-opt builds if arena_
    // lifetime has ended before the destructor.
    if (arena) (void)arena->SpaceAllocated();
#endif
    const int size = this->size();
    if (size > 0) {
      if constexpr (!kUseAnonymousElementType<T>) {
        T* elem = static_cast<T*>(unsafe_elements(is_soo));
        Destroy(elem, elem + size);
      }
    }
    AnnotateForRelease();
    if (!is_soo) {
      InternalDeallocate</*in_destructor=*/true>(SelfArena{});
    }
  }

  void Clear() {
    const bool is_soo = this->is_soo();
    if (!kUseAnonymousElementType<T>) {
      T* elem = static_cast<T*>(unsafe_elements(is_soo));
      Destroy(elem, elem + size());
    }
    ExchangeCurrentSize(0);
  }

  // Destroys all elements in [begin, end).
  // This function does nothing if `Element` is trivial.
  static void Destroy(const void*, const void*) {}
  static void Destroy([[maybe_unused]] const T* begin,
                      [[maybe_unused]] const T* end) {
    if constexpr (!std::is_trivially_destructible_v<T>) {
      std::for_each(begin, end, [&](const T& e) { e.~T(); });
    }
  }

  // Annotates a change in size of this instance. This function should be called
  // with (capacity, old_size) after new memory has been allocated and filled
  // from previous memory.
  void AnnotateSize(int old_size, int new_size) const {
    if (old_size != new_size) {
      [[maybe_unused]] const bool is_soo = this->is_soo();
      [[maybe_unused]] const T* elem =
          this->soo_rep_.template elements<T>(is_soo);
      ABSL_ANNOTATE_CONTIGUOUS_CONTAINER(elem, elem + Capacity(is_soo),
                                         elem + old_size, elem + new_size);
      if (new_size < old_size) {
        ABSL_ANNOTATE_MEMORY_IS_UNINITIALIZED(
            elem + new_size, (old_size - new_size) * sizeof(T));
      }
    }
  }

  void Reserve(int new_size) { ReserveWithArena(SelfArena{}, new_size); }

  void InternalSwap(RepeatedFieldImpl* PROTOBUF_RESTRICT other) {
    ABSL_DCHECK(this != other);

    // We need to unpoison during the swap in case we're in SOO mode.
    AnnotateForRelease();
    other->AnnotateForRelease();

    this->soo_rep_.swap(other->soo_rep_);

    AnnotateSize(Capacity(), size());
    other->AnnotateSize(other->Capacity(), other->size());
  }

  void Swap(RepeatedFieldImpl* other);

  // This should be called right before (previously annotated) memory is
  // released.
  void AnnotateForRelease() const { AnnotateSize(size(), Capacity()); }

  // Reserves space to expand the field to at least the given size.
  // If the array is grown, it will always be at least doubled in size.
  // If `annotate_size` is true (the default), then this function will annotate
  // the old container from `old_size` to `Capacity()` (unpoison memory)
  // directly before it is being released, and annotate the new container from
  // `Capacity()` to `old_size` (poison unused memory).
  template <typename ArenaProvider>
  void Grow(ArenaProvider arena_provider, bool was_soo, int old_size,
            int new_size);

  PROTOBUF_NOINLINE void GrowNoAnnotate(SerialArena* arena, bool was_soo,
                                        int old_size, int new_size);

  // Internal helper to delete all elements and deallocate the storage.
  template <bool in_destructor = false, typename ArenaProvider>
  void InternalDeallocate(ArenaProvider arena_provider) {
    ABSL_DCHECK(!is_soo());
    ABSL_DCHECK_EQ(ResolveArena(arena_provider), GetSerialArena());
    const size_t bytes = Capacity(false) * sizeof(T) + kHeapRepHeaderSize;
    if constexpr (in_destructor && Arena::is_destructor_skippable<T>::value) {
      // Repeated fields with destructor-skippable elements are never destroyed
      // during arena cleanup.
      ABSL_DCHECK_EQ(GetArena(), nullptr);
      internal::SizedDelete(heap_rep(), bytes);
    } else {
      auto* arena = ResolveArena(arena_provider);
      if (arena == nullptr) {
        internal::SizedDelete(heap_rep(), bytes);
      } else if (!in_destructor) {
        // If we are in the destructor, we might be being destroyed as part of
        // the arena teardown. We can't try and return blocks to the arena then.
        arena->ReturnArrayMemory(heap_rep(), bytes);
      }
    }
  }

  template <typename Init>
  void ResizeImpl(int new_size, Init init);

  void ResizeDefault(int new_size) {
    ResizeImpl(new_size, [](auto* first, auto* last) {
      if constexpr (kUseAnonymousElementType<T>) {
        memset(first, 0, (last - first) * sizeof(T));
      } else {
        std::uninitialized_value_construct(first, last);
      }
    });
  }

  void ResizeCopy(int new_size, const T& value) {
    ResizeImpl(new_size, [&](T* first, T* last) {
      if constexpr (kUseAnonymousElementType<T>) {
        T local_copy = value;
        for (; first != last; ++first) {
          memcpy(first, &local_copy, sizeof(local_copy));
        }
      } else {
        std::uninitialized_fill(first, last, value);
      }
    });
  }

  // Swaps entire contents with "other". Should be called only if the caller can
  // guarantee that both repeated fields are on the same arena or are on the
  // heap. Swapping between different arenas is disallowed and caught by a
  // ABSL_DCHECK (see API docs for details).
  void UnsafeArenaSwap(RepeatedFieldImpl* other);

  void SwapFallbackWithTemp(Arena* arena, RepeatedFieldImpl& other,
                            Arena* other_arena, RepeatedFieldImpl& temp);

  // Here we use void* to keep the logic generic and not use T by mistake where
  // we should not.
  void* elements(bool is_soo) {
    ABSL_DCHECK_GT(Capacity(is_soo), 0);
    return unsafe_elements(is_soo);
  }
  const void* elements(bool is_soo) const {
    ABSL_DCHECK_GT(Capacity(is_soo), 0);
    return unsafe_elements(is_soo);
  }
  void* unsafe_elements(bool is_soo) {
    return this->soo_rep_.template elements<T>(is_soo);
  }
  const void* unsafe_elements(bool is_soo) const {
    return this->soo_rep_.template elements<T>(is_soo);
  }
};

template <size_t N>
struct AnonymousType {
  char c[N];

  constexpr AnonymousType() = default;
  constexpr AnonymousType(const AnonymousType&) = default;
  constexpr AnonymousType& operator=(const AnonymousType&) = default;

  template <typename T, typename = std::enable_if_t<
                            !std::is_same_v<T, AnonymousType> &&
                            kUseAnonymousElementType<T> && sizeof(T) == N>>
  AnonymousType(T value) {  // NOLINT(google-explicit-constructor)
    memcpy(&c, &value, N);
  }
};

template <typename T>
using RepeatedFieldImplType = std::conditional_t<kUseAnonymousElementType<T>,
                                                 AnonymousType<sizeof(T)>, T>;

}  // namespace internal

// RepeatedField is used to represent repeated fields of a primitive type (in
// other words, everything except strings and nested Messages).  Most users will
// not ever use a RepeatedField directly; they will use the get-by-index,
// set-by-index, and add accessors that are generated for all repeated fields.
// Actually, in addition to primitive types, we use RepeatedField for repeated
// Cords, because the Cord class is in fact just a reference-counted pointer.
// We have to specialize several methods in the Cord case to get the memory
// management right; e.g. swapping when appropriate, etc.
template <typename Element>
class ABSL_ATTRIBUTE_WARN_UNUSED PROTOBUF_DECLSPEC_EMPTY_BASES
    RepeatedField final
    : private internal::RepeatedFieldImpl<
          internal::RepeatedFieldImplType<Element>>,
      private internal::ContainerDestructorSkippableBase<Element> {
  using Base = typename RepeatedField::RepeatedFieldImpl;
  static_assert(
      alignof(Arena) >= alignof(Element),
      "We only support types that have an alignment smaller than Arena");
  static_assert(!std::is_const_v<Element>,
                "We do not support const value types.");
  static_assert(!std::is_volatile_v<Element>,
                "We do not support volatile value types.");
  static_assert(!std::is_pointer_v<Element>,
                "We do not support pointer value types.");
  static_assert(!std::is_reference_v<Element>,
                "We do not support reference value types.");
  static constexpr PROTOBUF_ALWAYS_INLINE void StaticValidityCheck() {
    static_assert(
        std::disjunction_v<internal::is_supported_integral_type<Element>,
                           internal::is_supported_floating_point_type<Element>,
                           std::is_same<absl::Cord, Element>,
                           std::is_same<UnknownField, Element>,
                           is_proto_enum<Element>>,
        "We only support non-string scalars in RepeatedField.");
  }

 public:
  using value_type = Element;
  using size_type = int;
  using difference_type = ptrdiff_t;
  using reference = Element&;
  using const_reference = const Element&;
  using pointer = Element*;
  using const_pointer = const Element*;
  using iterator = internal::RepeatedIterator<Element>;
  using const_iterator = internal::RepeatedIterator<const Element>;
  using reverse_iterator = std::reverse_iterator<iterator>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;

  constexpr RepeatedField();
  RepeatedField(const RepeatedField& rhs)
      : RepeatedField(internal::InternalMetadataOffset(), /*arena=*/nullptr,
                      rhs) {}

  template <typename Iter, typename = std::enable_if_t<std::is_constructible_v<
                               Element, decltype(*std::declval<Iter>())>>>
  RepeatedField(Iter begin, Iter end);

  // Arena enabled constructors: for internal use only.
  constexpr RepeatedField(internal::InternalVisibility,
                          internal::InternalMetadataOffset offset)
      : RepeatedField(offset) {}
  RepeatedField(internal::InternalVisibility,
                internal::InternalMetadataOffset offset, Arena* arena,
                const RepeatedField& rhs)
      : RepeatedField(offset, arena, rhs) {}

  RepeatedField& operator=(const RepeatedField& other)
      ABSL_ATTRIBUTE_LIFETIME_BOUND;

  RepeatedField(RepeatedField&& rhs) noexcept
      : RepeatedField(internal::InternalMetadataOffset(), /*arena=*/nullptr,
                      std::move(rhs)) {}
  RepeatedField& operator=(RepeatedField&& other) noexcept
      ABSL_ATTRIBUTE_LIFETIME_BOUND;

  ~RepeatedField();

  PROTOBUF_FUTURE_ADD_NODISCARD bool empty() const;
  PROTOBUF_FUTURE_ADD_NODISCARD int size() const { return Base::size(); }

  PROTOBUF_FUTURE_ADD_NODISCARD const_reference
  Get(int index) const ABSL_ATTRIBUTE_LIFETIME_BOUND;
  PROTOBUF_FUTURE_ADD_NODISCARD pointer Mutable(int index)
      ABSL_ATTRIBUTE_LIFETIME_BOUND;

  PROTOBUF_FUTURE_ADD_NODISCARD const_reference
  operator[](int index) const ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return Get(index);
  }
  PROTOBUF_FUTURE_ADD_NODISCARD reference operator[](int index)
      ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return *Mutable(index);
  }

  PROTOBUF_FUTURE_ADD_NODISCARD const_reference
  at(int index) const ABSL_ATTRIBUTE_LIFETIME_BOUND;
  PROTOBUF_FUTURE_ADD_NODISCARD reference at(int index)
      ABSL_ATTRIBUTE_LIFETIME_BOUND;

  void Set(int index, const Element& value);
  void Add(Element value);

  // Appends a new element and returns a pointer to it.
  // The new element is uninitialized if |Element| is a POD type.
  pointer Add() ABSL_ATTRIBUTE_LIFETIME_BOUND;
  // Appends elements in the range [begin, end) after reserving
  // the appropriate number of elements.
  template <typename Iter>
  void Add(Iter begin, Iter end);

  // The following APIs are for internal use only.
  template <typename ArenaProvider>
  void InternalAddWithArena(internal::InternalVisibility,
                            ArenaProvider arena_provider, Element value);
  template <typename ArenaProvider>
  pointer InternalAddWithArena(internal::InternalVisibility,
                               ArenaProvider arena_provider)
      ABSL_ATTRIBUTE_LIFETIME_BOUND;

  // Removes the last element in the array.
  void RemoveLast();

  // Extracts elements with indices in "[start .. start+num-1]".
  // Copies them into "elements[0 .. num-1]" if "elements" is not nullptr.
  // Caution: also moves elements with indices [start+num ..].
  // Calling this routine inside a loop can cause quadratic behavior.
  void ExtractSubrange(int start, int num, Element* elements);

  ABSL_ATTRIBUTE_REINITIALIZES void Clear() { Base::Clear(); }

  // Appends the elements from `other` after this instance.
  // The end result length will be `other.size() + this->size()`.
  void MergeFrom(const RepeatedField& other);

  // Replaces the contents with a copy of the elements from `other`.
  ABSL_ATTRIBUTE_REINITIALIZES void CopyFrom(const RepeatedField& other);

  // Replaces the contents with RepeatedField(begin, end).
  template <typename Iter>
  ABSL_ATTRIBUTE_REINITIALIZES void Assign(Iter begin, Iter end);

  // Reserves space to expand the field to at least the given size.  If the
  // array is grown, it will always be at least doubled in size.
  void Reserve(int new_size) { Base::Reserve(new_size); }

  // Resizes the RepeatedField to a new, smaller size.  This is O(1).
  // Except for RepeatedField<Cord>, for which it is O(size-new_size).
  void Truncate(int new_size);

  void AddAlreadyReserved(Element value);
  PROTOBUF_FUTURE_ADD_NODISCARD int Capacity() const;

  // Adds `n` elements to this instance asserting there is enough capacity.
  // The added elements are uninitialized if `Element` is trivial.
  pointer AddAlreadyReserved() ABSL_ATTRIBUTE_LIFETIME_BOUND;
  pointer AddNAlreadyReserved(int n) ABSL_ATTRIBUTE_LIFETIME_BOUND;

  ABSL_DEPRECATE_AND_INLINE()
  void Resize(size_type new_size, const Element& value);

  // Like STL resize.  Uses value to fill appended elements.
  // Like Truncate() if new_size <= size(), otherwise this is
  // O(new_size - size()).
  void resize(size_type new_size);
  void resize(size_type new_size, const Element& value);

  // Gets the underlying array.  This pointer is possibly invalidated by
  // any add or remove operation.
  PROTOBUF_FUTURE_ADD_NODISCARD pointer mutable_data()
      ABSL_ATTRIBUTE_LIFETIME_BOUND;
  PROTOBUF_FUTURE_ADD_NODISCARD const_pointer
  data() const ABSL_ATTRIBUTE_LIFETIME_BOUND;

  // Swaps entire contents with "other". If they are separate arenas, then
  // copies data between each other.
  void Swap(RepeatedField* other) { Base::Swap(other); }

  // Swaps two elements.
  void SwapElements(int index1, int index2);

  PROTOBUF_FUTURE_ADD_NODISCARD iterator begin() ABSL_ATTRIBUTE_LIFETIME_BOUND;
  PROTOBUF_FUTURE_ADD_NODISCARD const_iterator
  begin() const ABSL_ATTRIBUTE_LIFETIME_BOUND;
  PROTOBUF_FUTURE_ADD_NODISCARD const_iterator
  cbegin() const ABSL_ATTRIBUTE_LIFETIME_BOUND;
  PROTOBUF_FUTURE_ADD_NODISCARD iterator end() ABSL_ATTRIBUTE_LIFETIME_BOUND;
  PROTOBUF_FUTURE_ADD_NODISCARD const_iterator
  end() const ABSL_ATTRIBUTE_LIFETIME_BOUND;
  PROTOBUF_FUTURE_ADD_NODISCARD const_iterator
  cend() const ABSL_ATTRIBUTE_LIFETIME_BOUND;

  // Reverse iterator support
  PROTOBUF_FUTURE_ADD_NODISCARD reverse_iterator rbegin()
      ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return reverse_iterator(end());
  }
  PROTOBUF_FUTURE_ADD_NODISCARD const_reverse_iterator
  rbegin() const ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return const_reverse_iterator(end());
  }
  PROTOBUF_FUTURE_ADD_NODISCARD reverse_iterator rend()
      ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return reverse_iterator(begin());
  }
  PROTOBUF_FUTURE_ADD_NODISCARD const_reverse_iterator
  rend() const ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return const_reverse_iterator(begin());
  }

  // Returns the number of bytes used by the repeated field, excluding
  // sizeof(*this)
  PROTOBUF_FUTURE_ADD_NODISCARD size_t SpaceUsedExcludingSelfLong() const;

  PROTOBUF_FUTURE_ADD_NODISCARD int SpaceUsedExcludingSelf() const {
    return internal::ToIntSize(SpaceUsedExcludingSelfLong());
  }

  // Removes the element referenced by position.
  //
  // Returns an iterator to the element immediately following the removed
  // element.
  //
  // Invalidates all iterators at or after the removed element, including end().
  iterator erase(const_iterator position) ABSL_ATTRIBUTE_LIFETIME_BOUND;

  // Removes the elements in the range [first, last).
  //
  // Returns an iterator to the element immediately following the removed range.
  //
  // Invalidates all iterators at or after the removed range, including end().
  iterator erase(const_iterator first,
                 const_iterator last) ABSL_ATTRIBUTE_LIFETIME_BOUND;

  // Gets the Arena on which this RepeatedField stores its elements.
  PROTOBUF_FUTURE_ADD_NODISCARD inline Arena* GetArena() {
    // Note: we make this function non-const to force callers to call the
    // `mutable_*` accessor on the repeated field before calling `GetArena()`,
    // which initializes the field if it is split. If this method were const,
    // then `msg.repeated_field().GetArena()` would be valid, but for split
    // repeated fields `repeated_field()` could point to the default split
    // instance. This would always return `nullptr`, which is incorrect when
    // using arenas.
    return Base::GetArena();
  }

  // For internal use only.
  //
  // This is public due to it being called by generated code.
  void InternalSwap(RepeatedField* other) { Base::InternalSwap(other); }

 private:
  using InternalArenaConstructable_ = void;

  template <typename T>
  friend class Arena::InternalHelper;

  friend class Arena;

  friend class DynamicMessage;

  friend class internal::FieldWithArena<RepeatedField<Element>>;

  // For access to private `*WithArena` functions.
  friend class google::protobuf::Reflection;
  friend class internal::EpsCopyInputStream;
  friend class internal::TcParser;
  friend class internal::WireFormat;

  template <typename ElementType, bool kOrProxy>
  friend class internal::MutableRepeatedFieldProxyImpl;

  // For access to private arena constructor.
  friend class UnknownFieldSet;

  static constexpr int kInitialSize = 0;

  explicit constexpr RepeatedField(internal::InternalMetadataOffset offset);
  RepeatedField(internal::InternalMetadataOffset offset, Arena* arena,
                const RepeatedField& rhs);
  RepeatedField(internal::InternalMetadataOffset offset, Arena* arena,
                RepeatedField&& rhs);

  template <typename ArenaProvider>
  pointer AddWithArena(ArenaProvider arena_provider, Element value) {
    return reinterpret_cast<pointer>(
        Base::AddWithArena(arena_provider, std::move(value)));
  }
  template <typename ArenaProvider>
  pointer AddWithArena(ArenaProvider arena_provider)
      ABSL_ATTRIBUTE_LIFETIME_BOUND;
  template <typename ArenaProvider, typename Iter>
  void AddWithArena(ArenaProvider arena_provider, Iter begin, Iter end);

  // Private-only API for in-place construction of elements in the repeated
  // field.
  template <typename... Args>
  pointer EmplaceWithArena(Arena* arena, Args&&... args);

  // Copy constructs `[begin, end)` instances in place into the array `dst`.
  // See above `UninitializedCopyN()` function comments for more information.
  template <typename Iter>
  static inline void UninitializedCopy(Iter begin, Iter end,
                                       Element* PROTOBUF_RESTRICT dst) {
    std::uninitialized_copy(begin, end, dst);
  }

  template <typename ArenaProvider, typename Iter>
  void AddForwardIterator(ArenaProvider arena_provider, Iter begin, Iter end);

  template <typename ArenaProvider, typename Iter>
  void AddInputIterator(ArenaProvider arena_provider, Iter begin, Iter end);

  // Returns a pointer to elements array.
  // pre-condition: Capacity() > 0.
  Element* elements(bool is_soo) {
    ABSL_DCHECK_GT(Base::Capacity(is_soo), 0);
    return unsafe_elements(is_soo);
  }
  const Element* elements(bool is_soo) const {
    ABSL_DCHECK_GT(Base::Capacity(is_soo), 0);
    return unsafe_elements(is_soo);
  }

  // Returns a pointer to elements array if it exists; otherwise an invalid
  // pointer is returned. This only happens for empty repeated fields, where you
  // can't dereference this pointer anyway (it's empty).
  Element* unsafe_elements(bool is_soo) {
    return this->soo_rep_.template elements<Element>(is_soo);
  }
  const Element* unsafe_elements(bool is_soo) const {
    return this->soo_rep_.template elements<Element>(is_soo);
  }
};

namespace internal {

template <typename Element>
using RepeatedFieldWithArena = internal::FieldWithArena<RepeatedField<Element>>;

template <typename Element>
struct FieldArenaRep<RepeatedField<Element>> {
  using Type = RepeatedFieldWithArena<Element>;

  static RepeatedField<Element>* Get(Type* arena_rep) {
    return &arena_rep->field();
  }
};

template <typename Element>
struct FieldArenaRep<const RepeatedField<Element>> {
  using Type = const RepeatedFieldWithArena<Element>;

  static const RepeatedField<Element>* Get(Type* arena_rep) {
    return &arena_rep->field();
  }
};

}  // namespace internal

// implementation ====================================================

template <typename Element>
constexpr RepeatedField<Element>::RepeatedField() {
  StaticValidityCheck();
#ifdef __cpp_lib_is_constant_evaluated
  if (!std::is_constant_evaluated()) {
    this->AnnotateSize(this->kSooCapacityElements, 0);
  }
#endif  // __cpp_lib_is_constant_evaluated
}

template <typename Element>
constexpr RepeatedField<Element>::RepeatedField(
    internal::InternalMetadataOffset offset)
    : Base(offset) {
  static_assert(
      sizeof(RepeatedField) == sizeof(Base),
      "Since we are passing an `InternalMetadataOffset` for this "
      "`RepeatedField` to the `RepeatedFieldBase` constructor, "
      "`RepeatedFieldBase` must have the same start address as `this`. We can "
      "validate this by checking that the two types have the same size.");
  StaticValidityCheck();
#ifdef __cpp_lib_is_constant_evaluated
  if (!std::is_constant_evaluated()) {
    this->AnnotateSize(this->kSooCapacityElements, 0);
  }
#endif  // __cpp_lib_is_constant_evaluated
}

template <typename Element>
inline RepeatedField<Element>::RepeatedField(
    internal::InternalMetadataOffset offset, Arena* arena,
    const RepeatedField& rhs)
    : Base(offset, arena, rhs) {
  StaticValidityCheck();
}

template <typename Element>
template <typename Iter, typename>
RepeatedField<Element>::RepeatedField(Iter begin, Iter end) {
  StaticValidityCheck();
  this->AnnotateSize(this->kSooCapacityElements, 0);
  Add(begin, end);
}

template <typename Element>
RepeatedField<Element>::~RepeatedField() {
  StaticValidityCheck();
  Base::Destroy();
}

template <typename Element>
inline RepeatedField<Element>& RepeatedField<Element>::operator=(
    const RepeatedField& other) ABSL_ATTRIBUTE_LIFETIME_BOUND {
  if (this != &other) CopyFrom(other);
  return *this;
}

template <typename Element>
inline RepeatedField<Element>::RepeatedField(
    internal::InternalMetadataOffset offset, Arena* arena, RepeatedField&& rhs)
    : RepeatedField(offset) {
  ABSL_DCHECK_EQ(arena, GetArena());
  if (internal::CanMoveWithInternalSwap(arena, rhs.GetArena())) {
    InternalSwap(&rhs);
  } else {
    // We don't just call Swap(&rhs) here because it would perform 3 copies if
    // rhs is on a different arena.
    CopyFrom(rhs);
  }
}

template <typename Element>
inline RepeatedField<Element>& RepeatedField<Element>::operator=(
    RepeatedField&& other) noexcept ABSL_ATTRIBUTE_LIFETIME_BOUND {
  // We don't just call Swap(&other) here because it would perform 3 copies if
  // the two fields are on different arenas.
  if (this != &other) {
    if (internal::CanMoveWithInternalSwap(GetArena(), other.GetArena())) {
      InternalSwap(&other);
    } else {
      CopyFrom(other);
    }
  }
  return *this;
}

template <typename Element>
inline bool RepeatedField<Element>::empty() const {
  return size() == 0;
}

template <typename Element>
inline int RepeatedField<Element>::Capacity() const {
  return Base::Capacity();
}

template <typename Element>
inline void RepeatedField<Element>::AddAlreadyReserved(Element value) {
  const bool is_soo = this->is_soo();
  const int old_size = size();
  internal::RuntimeAssertInBounds(old_size, Base::Capacity(is_soo));
  void* p = elements(is_soo) + this->ExchangeCurrentSize(old_size + 1);
  ::new (p) Element(std::move(value));
}

template <typename Element>
inline Element* RepeatedField<Element>::AddAlreadyReserved()
    ABSL_ATTRIBUTE_LIFETIME_BOUND {
  const bool is_soo = this->is_soo();
  const int old_size = size();
  internal::RuntimeAssertInBounds(old_size, Base::Capacity(is_soo));
  // new (p) <TrivialType> compiles into nothing: this is intentional as this
  // function is documented to return uninitialized data for trivial types.
  void* p = elements(is_soo) + this->ExchangeCurrentSize(old_size + 1);
  return ::new (p) Element;
}

template <typename Element>
inline Element* RepeatedField<Element>::AddNAlreadyReserved(int n)
    ABSL_ATTRIBUTE_LIFETIME_BOUND {
  internal::RuntimeAssertInBoundsGE(n, 0);
  const bool is_soo = this->is_soo();
  const int old_size = size();
  [[maybe_unused]] const int capacity = Base::Capacity(is_soo);

  const int64_t new_size_64 = static_cast<int64_t>(old_size) + n;
  internal::RuntimeAssertInBoundsLE(new_size_64, capacity);

  Element* p =
      unsafe_elements(is_soo) + this->ExchangeCurrentSize(old_size + n);
  for (Element *begin = p, *end = p + n; begin != end; ++begin) {
    new (static_cast<void*>(begin)) Element;
  }
  return p;
}

template <typename Element>
ABSL_DEPRECATE_AND_INLINE()
inline void RepeatedField<Element>::Resize(int new_size, const Element& value) {
  resize(new_size, value);
}

template <typename T>
template <typename Init>
inline void internal::RepeatedFieldImpl<T>::ResizeImpl(int new_size,
                                                       Init init) {
  ABSL_DCHECK_GE(new_size, 0);
  bool is_soo = this->is_soo();
  const int old_size = size();
  if (new_size > old_size) {
    if (new_size > Capacity(is_soo)) {
      Grow(SelfArena{}, is_soo, old_size, new_size);
      is_soo = false;
    }
    T* elem = static_cast<T*>(elements(is_soo));
    T* first = elem + ExchangeCurrentSize(new_size);
    init(first, elem + new_size);
  } else if (new_size < old_size) {
    T* elem = static_cast<T*>(unsafe_elements(is_soo));
    Destroy(elem + new_size, elem + old_size);
    ExchangeCurrentSize(new_size);
  }
}

template <typename Element>
inline void RepeatedField<Element>::resize(size_type new_size,
                                           const Element& value) {
  this->ResizeCopy(new_size, value);
}

template <typename Element>
inline void RepeatedField<Element>::resize(size_type new_size) {
  this->ResizeDefault(new_size);
}

template <typename Element>
inline const Element& RepeatedField<Element>::Get(int index) const
    ABSL_ATTRIBUTE_LIFETIME_BOUND {
  internal::RuntimeAssertInBounds(index, size());
  return elements(this->is_soo())[index];
}

template <typename Element>
inline const Element& RepeatedField<Element>::at(int index) const
    ABSL_ATTRIBUTE_LIFETIME_BOUND {
  ABSL_CHECK_GE(index, 0);
  ABSL_CHECK_LT(index, size());
  return elements(this->is_soo())[index];
}

template <typename Element>
inline Element& RepeatedField<Element>::at(int index)
    ABSL_ATTRIBUTE_LIFETIME_BOUND {
  ABSL_CHECK_GE(index, 0);
  ABSL_CHECK_LT(index, size());
  return elements(this->is_soo())[index];
}

template <typename Element>
inline Element* RepeatedField<Element>::Mutable(int index)
    ABSL_ATTRIBUTE_LIFETIME_BOUND {
  internal::RuntimeAssertInBounds(index, size());
  return &elements(this->is_soo())[index];
}

template <typename Element>
inline void RepeatedField<Element>::Set(int index, const Element& value) {
  *Mutable(index) = value;
}

template <typename Element>
inline void RepeatedField<Element>::Add(Element value) {
  AddWithArena(typename Base::SelfArena{}, std::move(value));
}

template <typename Element>
template <typename ArenaProvider>
inline void RepeatedField<Element>::InternalAddWithArena(
    internal::InternalVisibility, ArenaProvider arena_provider, Element value) {
  AddWithArena(arena_provider, std::move(value));
}

template <typename Element>
inline Element* RepeatedField<Element>::Add() ABSL_ATTRIBUTE_LIFETIME_BOUND {
  return AddWithArena(typename Base::SelfArena{});
}

template <typename Element>
template <typename ArenaProvider>
inline Element* RepeatedField<Element>::InternalAddWithArena(
    internal::InternalVisibility,
    ArenaProvider arena_provider) ABSL_ATTRIBUTE_LIFETIME_BOUND {
  return AddWithArena(arena_provider);
}

template <typename Element>
template <typename ArenaProvider>
inline Element* RepeatedField<Element>::AddWithArena(
    ArenaProvider arena_provider) ABSL_ATTRIBUTE_LIFETIME_BOUND {
  return ::new (this->AddUninitializedWithArena(arena_provider)) Element;
}

template <typename Element>
template <typename ArenaProvider, typename Iter>
inline void RepeatedField<Element>::AddForwardIterator(
    ArenaProvider arena_provider, Iter begin, Iter end) {
  ABSL_DCHECK_EQ(this->ResolveArena(arena_provider), this->GetSerialArena());

  bool is_soo = this->is_soo();
  const int old_size = size();
  int capacity = Base::Capacity(is_soo);
  Element* elem = unsafe_elements(is_soo);
  // Check for signed overflow.
  const size_t distance = std::distance(begin, end);
  ABSL_CHECK_LE(distance, static_cast<size_t>(std::numeric_limits<int>::max()))
      << "Input too large";
  // Check again for signed overflow.
  const int new_size =
      internal::CheckedAdd(old_size, static_cast<int>(distance));
  if (ABSL_PREDICT_FALSE(new_size > capacity)) {
    this->Grow(arena_provider, is_soo, old_size, new_size);
    is_soo = false;
    elem = unsafe_elements(is_soo);
    capacity = Base::Capacity(is_soo);
  }
  UninitializedCopy(begin, end, elem + this->ExchangeCurrentSize(new_size));

  // The below helps the compiler optimize dense loops.
  // Note: we can't call functions in PROTOBUF_ASSUME so use local variables.
  [[maybe_unused]] const bool final_is_soo = this->is_soo();
  PROTOBUF_ASSUME(is_soo == final_is_soo);
  [[maybe_unused]] const int final_size = size();
  PROTOBUF_ASSUME(new_size == final_size);
  [[maybe_unused]] Element* const final_elements = unsafe_elements(is_soo);
  PROTOBUF_ASSUME(elem == final_elements);
  [[maybe_unused]] const int final_capacity = Base::Capacity(is_soo);
  PROTOBUF_ASSUME(capacity == final_capacity);
}

template <typename Element>
template <typename ArenaProvider, typename Iter>
inline void RepeatedField<Element>::AddInputIterator(
    ArenaProvider arena_provider, Iter begin, Iter end) {
  ABSL_DCHECK_EQ(this->ResolveArena(arena_provider), this->GetSerialArena());

  bool is_soo = this->is_soo();
  int size = this->size();
  int capacity = Base::Capacity(is_soo);
  Element* elem = unsafe_elements(is_soo);
  Element* first = elem + size;
  Element* last = elem + capacity;
  this->AnnotateForRelease();

  while (begin != end) {
    if (ABSL_PREDICT_FALSE(first == last)) {
      size = first - elem;
      this->GrowNoAnnotate(this->ResolveArena(arena_provider), is_soo, size,
                           internal::CheckedAdd(size, 1));
      is_soo = false;
      elem = unsafe_elements(is_soo);
      capacity = Base::Capacity(is_soo);
      first = elem + size;
      last = elem + capacity;
    }
    ::new (static_cast<void*>(first)) Element(*begin);
    ++begin;
    ++first;
  }

  const int new_size = first - elem;
  this->soo_rep_.set_size(new_size);
  this->AnnotateSize(capacity, new_size);
}

template <typename Element>
template <typename Iter>
inline void RepeatedField<Element>::Add(Iter begin, Iter end) {
  this->AddWithArena(typename Base::SelfArena{}, std::move(begin),
                     std::move(end));
}

template <typename Element>
template <typename ArenaProvider, typename Iter>
inline void RepeatedField<Element>::AddWithArena(ArenaProvider arena_provider,
                                                 Iter begin, Iter end) {
  if (std::is_base_of_v<
          std::forward_iterator_tag,
          typename std::iterator_traits<Iter>::iterator_category>) {
    AddForwardIterator(arena_provider, begin, end);
  } else {
    AddInputIterator(arena_provider, begin, end);
  }
}

template <typename Element>
template <typename... Args>
typename RepeatedField<Element>::pointer
RepeatedField<Element>::EmplaceWithArena(Arena* arena, Args&&... args) {
  return ::new (this->AddUninitializedWithArena(arena))
      Element(std::forward<Args>(args)...);
}

template <typename Element>
inline void RepeatedField<Element>::RemoveLast() {
  const bool is_soo = this->is_soo();
  const int old_size = size();
  ABSL_DCHECK_GT(old_size, 0);
  elements(is_soo)[old_size - 1].~Element();
  this->ExchangeCurrentSize(old_size - 1);
}

template <typename Element>
void RepeatedField<Element>::ExtractSubrange(int start, int num,
                                             Element* elements) {
  ABSL_DCHECK_GE(start, 0);
  ABSL_DCHECK_GE(num, 0);
  const bool is_soo = this->is_soo();
  const int old_size = size();
  ABSL_DCHECK_LE(start + num, old_size);
  Element* elem = unsafe_elements(is_soo);

  // Save the values of the removed elements if requested.
  if (elements != nullptr) {
    for (int i = 0; i < num; ++i) elements[i] = std::move(elem[i + start]);
  }

  // Slide remaining elements down to fill the gap.
  if (num > 0) {
    for (int i = start + num; i < old_size; ++i)
      elem[i - num] = std::move(elem[i]);
    Truncate(old_size - num);
  }
}

template <typename Element>
inline void RepeatedField<Element>::MergeFrom(const RepeatedField& other) {
  return Base::MergeFrom(other);
}

template <typename Element>
inline void RepeatedField<Element>::CopyFrom(const RepeatedField& other) {
  if (&other == this) return;
  Clear();
  MergeFrom(other);
}

template <typename Element>
template <typename Iter>
inline void RepeatedField<Element>::Assign(Iter begin, Iter end) {
  Clear();
  Add(begin, end);
}

template <typename Element>
inline typename RepeatedField<Element>::iterator RepeatedField<Element>::erase(
    const_iterator position) ABSL_ATTRIBUTE_LIFETIME_BOUND {
  return erase(position, position + 1);
}

template <typename Element>
inline typename RepeatedField<Element>::iterator RepeatedField<Element>::erase(
    const_iterator first, const_iterator last) ABSL_ATTRIBUTE_LIFETIME_BOUND {
  size_type first_offset = first - cbegin();
  if (first != last) {
    Truncate(std::copy(last, cend(), begin() + first_offset) - cbegin());
  }
  return begin() + first_offset;
}

template <typename Element>
inline Element* RepeatedField<Element>::mutable_data()
    ABSL_ATTRIBUTE_LIFETIME_BOUND {
  return unsafe_elements(this->is_soo());
}

template <typename Element>
inline const Element* RepeatedField<Element>::data() const
    ABSL_ATTRIBUTE_LIFETIME_BOUND {
  return unsafe_elements(this->is_soo());
}

template <typename T>
void internal::RepeatedFieldImpl<T>::SwapFallbackWithTemp(
    Arena* arena, RepeatedFieldImpl& other, Arena* other_arena,
    RepeatedFieldImpl& temp) {
  ABSL_DCHECK(this != &other);

  temp.MergeFrom(*this);
  Clear();
  MergeFrom(other);
  other.UnsafeArenaSwap(&temp);
}

template <typename T>
void internal::RepeatedFieldImpl<T>::Swap(RepeatedFieldImpl* other) {
  if (this == other) return;
  Arena* arena = GetArena();
  Arena* other_arena = other->GetArena();
  if (internal::CanUseInternalSwap(arena, other_arena)) {
    InternalSwap(other);
  } else if (other_arena != nullptr) {
    // We can't call the destructor of the temp container since it allocates
    // memory from an arena, and the destructor of FieldWithArena expects to be
    // called only when arena is nullptr.
    absl::NoDestructor<internal::FieldWithArena<RepeatedFieldImpl>>
        temp_container(other_arena);
    auto& temp = temp_container->field();
    SwapFallbackWithTemp(arena, *other, other_arena, temp);
    if constexpr (!Arena::is_destructor_skippable<T>::value) {
      temp.Destroy();
    }
  } else {
    RepeatedFieldImpl temp;
    SwapFallbackWithTemp(arena, *other, other_arena, temp);
    temp.Destroy();
  }
}

template <typename T>
void internal::RepeatedFieldImpl<T>::UnsafeArenaSwap(RepeatedFieldImpl* other) {
  if (this == other) return;
  ABSL_DCHECK_EQ(GetArena(), other->GetArena());
  InternalSwap(other);
}

template <typename Element>
void RepeatedField<Element>::SwapElements(int index1, int index2) {
  internal::RuntimeAssertInBounds(index1, size());
  internal::RuntimeAssertInBounds(index2, size());
  Element* elem = elements(this->is_soo());
  using std::swap;  // enable ADL with fallback
  swap(elem[index1], elem[index2]);
}

template <typename Element>
inline typename RepeatedField<Element>::iterator RepeatedField<Element>::begin()
    ABSL_ATTRIBUTE_LIFETIME_BOUND {
  return iterator(unsafe_elements(this->is_soo()));
}
template <typename Element>
inline typename RepeatedField<Element>::const_iterator
RepeatedField<Element>::begin() const ABSL_ATTRIBUTE_LIFETIME_BOUND {
  return const_iterator(unsafe_elements(this->is_soo()));
}
template <typename Element>
inline typename RepeatedField<Element>::const_iterator
RepeatedField<Element>::cbegin() const ABSL_ATTRIBUTE_LIFETIME_BOUND {
  return const_iterator(unsafe_elements(this->is_soo()));
}
template <typename Element>
inline typename RepeatedField<Element>::iterator RepeatedField<Element>::end()
    ABSL_ATTRIBUTE_LIFETIME_BOUND {
  const bool is_soo = this->is_soo();
  return iterator(unsafe_elements(is_soo) + size());
}
template <typename Element>
inline typename RepeatedField<Element>::const_iterator
RepeatedField<Element>::end() const ABSL_ATTRIBUTE_LIFETIME_BOUND {
  const bool is_soo = this->is_soo();
  return const_iterator(unsafe_elements(is_soo) + size());
}
template <typename Element>
inline typename RepeatedField<Element>::const_iterator
RepeatedField<Element>::cend() const ABSL_ATTRIBUTE_LIFETIME_BOUND {
  const bool is_soo = this->is_soo();
  return const_iterator(unsafe_elements(is_soo) + size());
}

template <typename Element>
inline size_t RepeatedField<Element>::SpaceUsedExcludingSelfLong() const {
  const int capacity = Capacity();
  return capacity > this->kSooCapacityElements
             ? capacity * sizeof(Element) + this->kHeapRepHeaderSize
             : 0;
}

// Like C++20's std::erase_if, for RepeatedField
template <typename T, typename Pred>
size_t erase_if(RepeatedField<T>& cont, Pred pred) {
  // Intentionally decay `elem` to avoid exposing a reference to elements of the
  // repeated field directly.
  using DecayedElement = internal::DecayedRepeatedFieldElement<T>;
  auto it =
      std::remove_if(cont.begin(), cont.end(),
                     [&pred](const DecayedElement elem) { return pred(elem); });
  size_t removed = cont.end() - it;
  cont.Truncate(cont.size() - removed);
  return removed;
}

// Like C++20's std::erase, for RepeatedField
template <typename T, typename U>
size_t erase(RepeatedField<T>& cont, const U& value) {
  return google::protobuf::erase_if(cont,
                          [&](const auto& elem) { return elem == value; });
}

// These functions mimic their std counterpart, but potentially more efficient
// for Protobuf containers.
template <int&..., typename T, typename Compare>
void sort(internal::RepeatedIterator<T> begin,
          internal::RepeatedIterator<T> end, Compare cmp) {
  // Intentionally decay `lhs` and `rhs` to avoid exposing a reference to
  // elements of the repeated field directly.
  using DecayedElement = internal::DecayedRepeatedFieldElement<T>;
  std::sort(begin, end,
            [&cmp](const DecayedElement lhs, const DecayedElement rhs) {
              return cmp(lhs, rhs);
            });
}
template <int&..., typename T>
void sort(internal::RepeatedIterator<T> begin,
          internal::RepeatedIterator<T> end) {
  google::protobuf::sort(begin, end, std::less<>{});
}
template <int&..., typename T, typename Compare>
void stable_sort(internal::RepeatedIterator<T> begin,
                 internal::RepeatedIterator<T> end, Compare cmp) {
  // Intentionally decay `lhs` and `rhs` to avoid exposing a reference to
  // elements of the repeated field directly.
  using DecayedElement = internal::DecayedRepeatedFieldElement<T>;
  std::stable_sort(begin, end,
                   [&cmp](const DecayedElement lhs, const DecayedElement rhs) {
                     return cmp(lhs, rhs);
                   });
}
template <int&..., typename T>
void stable_sort(internal::RepeatedIterator<T> begin,
                 internal::RepeatedIterator<T> end) {
  google::protobuf::stable_sort(begin, end, std::less<>{});
}

// These functions mimic their absl counterpart, but they are more efficient for
// Protobuf containers.
template <int&..., typename T, typename Compare>
void c_sort(RepeatedField<T>& cont, Compare cmp) {
  google::protobuf::sort(cont.begin(), cont.end(), cmp);
}
template <int&..., typename T>
void c_sort(RepeatedField<T>& cont) {
  google::protobuf::c_sort(cont, std::less<>{});
}
template <int&..., typename T, typename Compare>
void c_stable_sort(RepeatedField<T>& cont, Compare cmp) {
  google::protobuf::stable_sort(cont.begin(), cont.end(), cmp);
}
template <int&..., typename T>
void c_stable_sort(RepeatedField<T>& cont) {
  google::protobuf::c_stable_sort(cont, std::less<>{});
}

namespace internal {
// Returns the new size for a reserved field based on its 'capacity' and the
// requested 'new_size'. The result is clamped to the closed interval:
//   [internal::kMinRepeatedFieldAllocationSize,
//    std::numeric_limits<int>::max()]
// Requires: new_size > capacity
template <typename T, int kHeapRepHeaderSize>
inline int CalculateReserveSize(int capacity, int new_size) {
  constexpr int lower_limit =
      RepeatedFieldLowerClampLimit<T, kHeapRepHeaderSize>();
  if (new_size < lower_limit) {
    // Clamp to smallest allowed size.
    return lower_limit;
  }
  constexpr int kMaxSizeBeforeClamp =
      (std::numeric_limits<int>::max() - kHeapRepHeaderSize) / 2;
  if (ABSL_PREDICT_FALSE(capacity > kMaxSizeBeforeClamp)) {
    return std::numeric_limits<int>::max();
  }
  constexpr int kSooCapacityElements = SooCapacityElements<T>();
  if (kSooCapacityElements > 0 && kSooCapacityElements < lower_limit) {
    // In this case, we need to set capacity to 0 here to ensure power-of-two
    // sized allocations.
    if (capacity < lower_limit) capacity = 0;
  } else {
    ABSL_DCHECK(capacity == 0 || capacity >= lower_limit)
        << capacity << " " << lower_limit;
  }
  // We want to double the number of bytes, not the number of elements, to try
  // to stay within power-of-two allocations.
  // The allocation has kHeapRepHeaderSize + sizeof(T) * capacity.
  int doubled_size = 2 * capacity + kHeapRepHeaderSize / sizeof(T);
  return std::max(doubled_size, new_size);
}
}  // namespace internal

// Avoid inlining of Reserve(): new, copy, and delete[] lead to a significant
// amount of code bloat.
template <typename T>
PROTOBUF_NOINLINE void internal::RepeatedFieldImpl<T>::GrowNoAnnotate(
    internal::SerialArena* arena, bool was_soo, int old_size, int new_size) {
  const int old_capacity = Capacity(was_soo);
  ABSL_DCHECK_GT(new_size, old_capacity);
  internal::HeapRep* new_rep;

  new_size = internal::CalculateReserveSize<T, kHeapRepHeaderSize>(old_capacity,
                                                                   new_size);

  ABSL_DCHECK_LE(
      static_cast<size_t>(new_size),
      (std::numeric_limits<size_t>::max() - kHeapRepHeaderSize) / sizeof(T))
      << "Requested size is too large to fit into size_t.";
  size_t bytes = kHeapRepHeaderSize + sizeof(T) * static_cast<size_t>(new_size);
  if (arena == nullptr) {
    ABSL_DCHECK_LE((bytes - kHeapRepHeaderSize) / sizeof(T),
                   static_cast<size_t>(std::numeric_limits<int>::max()))
        << "Requested size is too large to fit element count into int.";
    internal::SizedPtr res = internal::AllocateAtLeast(bytes);
    size_t num_available =
        std::min((res.n - kHeapRepHeaderSize) / sizeof(T),
                 static_cast<size_t>(std::numeric_limits<int>::max()));
    new_size = static_cast<int>(num_available);
    new_rep = new (res.p) internal::HeapRep(new_size);
  } else {
    if constexpr (internal::ArenaAlignDefault::Ceil(sizeof(T)) != sizeof(T)) {
      // We need to manually align the allocation.
      bytes = internal::ArenaAlignDefault::Ceil(bytes);
    }
    new_rep =
        new (arena->AllocateAligned<internal::AllocationClient::kArray>(bytes))
            internal::HeapRep(new_size);
  }

  if (old_size > 0) {
    if constexpr (std::is_trivially_copyable_v<T> ||
                  absl::is_trivially_relocatable<T>::value) {
      void* pnew = new_rep->elements<T>();
      void* pold = elements(was_soo);
      memcpy(pnew, pold, old_size * sizeof(T));
    } else {
      T* pnew = new_rep->elements<T>();
      T* pold = static_cast<T*>(elements(was_soo));
      for (T* end = pnew + old_size; pnew != end; ++pnew, ++pold) {
        ::new (static_cast<void*>(pnew)) T(std::move(*pold));
        pold->~T();
      }
    }
  }
  if (!was_soo) InternalDeallocate(arena);

  this->soo_rep_.set_non_soo(new_rep);
}

// Ideally we would be able to use:
//   template <bool annotate_size = true>
//   void Grow();
// However, as explained in b/266411038#comment9, this causes issues
// in shared libraries for Youtube (and possibly elsewhere).
template <typename T>
template <typename ArenaProvider>
void internal::RepeatedFieldImpl<T>::Grow(ArenaProvider arena_provider,
                                          bool was_soo, int old_size,
                                          int new_size) {
  AnnotateForRelease();
  GrowNoAnnotate(ResolveArena(arena_provider), was_soo, old_size, new_size);
  AnnotateSize(Capacity(), old_size);
}

template <typename Element>
inline void RepeatedField<Element>::Truncate(int new_size) {
  const int old_size = size();
  ABSL_DCHECK_LE(new_size, old_size);
  if (new_size < old_size) {
    Element* elem = unsafe_elements(this->is_soo());
    this->Destroy(elem + new_size, elem + old_size);
    this->ExchangeCurrentSize(new_size);
  }
}

template <>
PROTOBUF_EXPORT size_t
RepeatedField<absl::Cord>::SpaceUsedExcludingSelfLong() const;

// -------------------------------------------------------------------

// Iterators and helper functions that follow the spirit of the STL
// std::back_insert_iterator and std::back_inserter but are tailor-made
// for RepeatedField and RepeatedPtrField. Typical usage would be:
//
//   std::copy(some_sequence.begin(), some_sequence.end(),
//             RepeatedFieldBackInserter(proto.mutable_sequence()));
//
// Ported by johannes from util/gtl/proto-array-iterators.h

namespace internal {

// STL-like iterator implementation for RepeatedField.  You should not
// refer to this class directly; use RepeatedField<T>::iterator instead.
//
// Note: All of the iterator operators *must* be inlined to avoid performance
// regressions.  This is caused by the extern template declarations below (which
// are required because of the RepeatedField extern template declarations).  If
// any of these functions aren't explicitly inlined (e.g. defined in the class),
// the compiler isn't allowed to inline them.
template <typename Element>
class RepeatedIterator {
 private:
  using traits = std::iterator_traits<std::remove_const_t<Element>*>;

 public:
  // Note: value_type is never cv-qualified.
  using value_type = typename traits::value_type;
  using difference_type = typename traits::difference_type;
  using pointer = Element*;
  using reference = Element&;
  using iterator_category = typename traits::iterator_category;
  using iterator_concept = typename IteratorConceptSupport<traits>::tag;

  constexpr RepeatedIterator() noexcept : it_(nullptr) {}

  // Allows "upcasting" from RepeatedIterator<T**> to
  // RepeatedIterator<const T*const*>.
  template <typename OtherElement,
            typename std::enable_if<std::is_convertible<
                OtherElement*, pointer>::value>::type* = nullptr>
  constexpr RepeatedIterator(
      const RepeatedIterator<OtherElement>& other) noexcept
      : it_(other.it_) {}

  // dereferenceable
  PROTOBUF_FUTURE_ADD_NODISCARD constexpr reference operator*() const noexcept {
    return *it_;
  }
  PROTOBUF_FUTURE_ADD_NODISCARD constexpr pointer operator->() const noexcept {
    return it_;
  }

 private:
  // Helper alias to hide the internal type.
  using iterator = RepeatedIterator<Element>;

 public:
  // {inc,dec}rementable
  iterator& operator++() noexcept {
    ++it_;
    return *this;
  }
  iterator operator++(int) noexcept { return iterator(it_++); }
  iterator& operator--() noexcept {
    --it_;
    return *this;
  }
  iterator operator--(int) noexcept { return iterator(it_--); }

  // equality_comparable
  friend constexpr bool operator==(const iterator& x,
                                   const iterator& y) noexcept {
    return x.it_ == y.it_;
  }
  friend constexpr bool operator!=(const iterator& x,
                                   const iterator& y) noexcept {
    return x.it_ != y.it_;
  }

  // less_than_comparable
  friend constexpr bool operator<(const iterator& x,
                                  const iterator& y) noexcept {
    return x.it_ < y.it_;
  }
  friend constexpr bool operator<=(const iterator& x,
                                   const iterator& y) noexcept {
    return x.it_ <= y.it_;
  }
  friend constexpr bool operator>(const iterator& x,
                                  const iterator& y) noexcept {
    return x.it_ > y.it_;
  }
  friend constexpr bool operator>=(const iterator& x,
                                   const iterator& y) noexcept {
    return x.it_ >= y.it_;
  }

  // addable, subtractable
  iterator& operator+=(difference_type d) noexcept {
    it_ += d;
    return *this;
  }
  constexpr iterator operator+(difference_type d) const noexcept {
    return iterator(it_ + d);
  }
  friend constexpr iterator operator+(const difference_type d,
                                      iterator it) noexcept {
    return it + d;
  }

  iterator& operator-=(difference_type d) noexcept {
    it_ -= d;
    return *this;
  }
  iterator constexpr operator-(difference_type d) const noexcept {
    return iterator(it_ - d);
  }

  // indexable
  PROTOBUF_FUTURE_ADD_NODISCARD constexpr reference operator[](
      difference_type d) const noexcept {
    return it_[d];
  }

  // random access iterator
  friend constexpr difference_type operator-(iterator it1,
                                             iterator it2) noexcept {
    return it1.it_ - it2.it_;
  }

 private:
  template <typename OtherElement>
  friend class RepeatedIterator;

  // Allow construction from RepeatedField.
  friend class RepeatedField<value_type>;
  explicit RepeatedIterator(pointer it) noexcept : it_(it) {}

  // The internal iterator.
  pointer it_;
};

// A back inserter for RepeatedField objects.
template <typename T>
class RepeatedFieldBackInsertIterator {
 public:
  using iterator_category = std::output_iterator_tag;
  using value_type = T;
  using pointer = void;
  using reference = void;
  using difference_type = std::ptrdiff_t;

  explicit RepeatedFieldBackInsertIterator(
      RepeatedField<T>* const mutable_field)
      : field_(mutable_field) {}
  RepeatedFieldBackInsertIterator<T>& operator=(const T& value) {
    field_->Add(value);
    return *this;
  }
  RepeatedFieldBackInsertIterator<T>& operator*() { return *this; }
  RepeatedFieldBackInsertIterator<T>& operator++() { return *this; }
  RepeatedFieldBackInsertIterator<T>& operator++(int /* unused */) {
    return *this;
  }

 private:
  RepeatedField<T>* field_;
};

}  // namespace internal

// Provides a back insert iterator for RepeatedField instances,
// similar to std::back_inserter().
template <typename T>
internal::RepeatedFieldBackInsertIterator<T> RepeatedFieldBackInserter(
    RepeatedField<T>* const mutable_field) {
  return internal::RepeatedFieldBackInsertIterator<T>(mutable_field);
}

namespace internal {
template <typename T>
inline void CheckIndexInBoundsOrAbort(const RepeatedField<T>& field,
                                      int index) {
  if (ABSL_PREDICT_FALSE(index < 0 || index >= field.size())) {
    LogIndexOutOfBoundsAndAbort(index, field.size());
  }
}

template <typename T>
const T& CheckedGetOrAbort(const RepeatedField<T>& field, int index) {
  CheckIndexInBoundsOrAbort(field, index);
  return field.Get(index);
}

template <typename T>
inline T* CheckedMutableOrAbort(RepeatedField<T>* field, int index) {
  CheckIndexInBoundsOrAbort(*field, index);
  return field->Mutable(index);
}
}  // namespace internal


}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"

#endif  // GOOGLE_PROTOBUF_REPEATED_FIELD_H__
