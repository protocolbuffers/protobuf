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
// This header covers RepeatedPtrField.

#ifndef GOOGLE_PROTOBUF_REPEATED_PTR_FIELD_H__
#define GOOGLE_PROTOBUF_REPEATED_PTR_FIELD_H__

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <iterator>
#include <limits>
#include <string>
#include <type_traits>
#include <utility>

#include "absl/base/attributes.h"
#include "absl/base/no_destructor.h"
#include "absl/base/optimization.h"
#include "absl/base/prefetch.h"
#include "absl/functional/function_ref.h"
#include "absl/log/absl_check.h"
#include "absl/strings/string_view.h"
#include "google/protobuf/arena.h"
#include "google/protobuf/arena_align.h"
#include "google/protobuf/field_with_arena.h"
#include "google/protobuf/internal_metadata_locator.h"
#include "google/protobuf/internal_visibility.h"
#include "google/protobuf/message_lite.h"
#include "google/protobuf/port.h"

// Must be included last.
#include "google/protobuf/port_def.inc"

#ifdef SWIG
#error "You cannot SWIG proto headers"
#endif

namespace google {
namespace protobuf {

class DynamicMessage;
class Message;
class Reflection;


template <typename T>
struct WeakRepeatedPtrField;

namespace internal {

class MergePartialFromCodedStreamHelper;
class SwapFieldHelper;
class MapFieldBase;

template <typename Element>
class RepeatedPtrIterator;
template <typename Element>
class RepeatedPtrOverPtrsIterator;
template <typename T>
class RepeatedPtrFieldBackInsertIterator;
template <typename T>
class AllocatedRepeatedPtrFieldBackInsertIterator;

class RepeatedPtrFieldTest;

// Swaps two non-overlapping blocks of memory of size `N`
template <size_t N>
inline void memswap(char* PROTOBUF_RESTRICT a, char* PROTOBUF_RESTRICT b) {
  // `PROTOBUF_RESTRICT` tells compiler that blocks do not overlapping which
  // allows it to generate optimized code for swap_ranges.
  std::swap_ranges(a, a + N, b);
}

// A trait that tells offset of `T::resolver_`.
//
// Do not use this struct - it exists for internal use only.
template <typename T>
struct InternalMetadataResolverOffsetHelper {
  static constexpr size_t value = offsetof(T, resolver_);
};

// Copies the object in the arena.
// Used in the slow path. Out-of-line for lower binary size cost.
PROTOBUF_EXPORT MessageLite* CloneSlow(Arena* arena, const MessageLite& value);
PROTOBUF_EXPORT std::string* CloneSlow(Arena* arena, const std::string& value);

enum class BoundsCheckMessageType {
  kIndex,
  kGe,
  kLe,
};

// A utility function for logging that doesn't need any template types.
PROTOBUF_EXPORT void LogIndexOutOfBounds(int index, int size);

// A utility function for logging that doesn't need any template types. Same as
// LogIndexOutOfBounds, but aborts the program in all cases by logging to FATAL
// instead of DFATAL.
// TODO: Remove preserve_all and add no_return once experiment is
// complete.
PROTOBUF_PRESERVE_ALL PROTOBUF_EXPORT void LogIndexOutOfBoundsAndAbort(
    int64_t index, int64_t size,
    BoundsCheckMessageType type = BoundsCheckMessageType::kIndex);
PROTOBUF_EXPORT inline void RuntimeAssertInBounds(int index, int size) {
  if constexpr (GetBoundsCheckMode() == BoundsCheckMode::kAbort) {
    if (ABSL_PREDICT_FALSE(index < 0 || index >= size)) {
      // "No merge" attribute used to improve debuggability by telling the
      // compiler not to merge these failure paths. Note that this is currently
      // best-effort in clang/llvm.
      PROTOBUF_NO_MERGE LogIndexOutOfBoundsAndAbort(index, size);
    }
  }
  ABSL_DCHECK_GE(index, 0);
  ABSL_DCHECK_LT(index, size);
}

PROTOBUF_EXPORT inline void RuntimeAssertInBoundsLE(int64_t value,
                                                    int64_t limit) {
  if constexpr (GetBoundsCheckMode() == BoundsCheckMode::kAbort) {
    if (ABSL_PREDICT_FALSE(value > limit)) {
      PROTOBUF_NO_MERGE LogIndexOutOfBoundsAndAbort(
          value, limit, BoundsCheckMessageType::kLe);
    }
  }

  ABSL_DCHECK_LE(value, limit);
}

PROTOBUF_EXPORT inline void RuntimeAssertInBoundsGE(int64_t value,
                                                    int64_t limit) {
  if constexpr (GetBoundsCheckMode() == BoundsCheckMode::kAbort) {
    if (ABSL_PREDICT_FALSE(value < limit)) {
      PROTOBUF_NO_MERGE LogIndexOutOfBoundsAndAbort(
          value, limit, BoundsCheckMessageType::kGe);
    }
  }
  ABSL_DCHECK_GE(value, limit);
}

// Defined further below.
template <typename Type>
class GenericTypeHandler;

using ElementNewFn = void(Arena*, void*& ptr);

// This is the common base class for RepeatedPtrFields.  It deals only in void*
// pointers.  Users should not use this interface directly.
//
// The methods of this interface correspond to the methods of RepeatedPtrField,
// but may have a template argument called TypeHandler.  Its signature is:
//   class TypeHandler {
//    public:
//     using Type = MyType;
//
//     static Type*(*)(Arena*) GetNewFunc();
//     static Type*(*)(Arena*) GetNewFromPrototypeFunc(const Type* prototype);
//     static Arena* GetArena(Type* value);
//
//     static Type* New(Arena* arena, Type&& value);
//     static void Delete(Type*, Arena* arena);
//     static void Clear(Type*);
//
//     // Only needs to be implemented if SpaceUsedExcludingSelf() is called.
//     static int SpaceUsedLong(const Type&);
//
//     static const Type& default_instance();
//   };
class PROTOBUF_EXPORT RepeatedPtrFieldBase {
  template <typename TypeHandler>
  using Value = typename TypeHandler::Type;

  static constexpr int kSSOCapacity = 1;

 protected:
  // We use the same TypeHandler for all Message types to deduplicate generated
  // code.
  template <typename TypeHandler>
  using CommonHandler = typename std::conditional<
      std::is_base_of<MessageLite, Value<TypeHandler>>::value,
      GenericTypeHandler<MessageLite>, TypeHandler>::type;

  constexpr RepeatedPtrFieldBase()
      : tagged_rep_or_elem_(nullptr), current_size_(0) {}
  constexpr explicit RepeatedPtrFieldBase(InternalMetadataOffset offset)
      : tagged_rep_or_elem_(nullptr), current_size_(0), resolver_(offset) {}

  RepeatedPtrFieldBase(const RepeatedPtrFieldBase&) = delete;
  RepeatedPtrFieldBase& operator=(const RepeatedPtrFieldBase&) = delete;

  ~RepeatedPtrFieldBase() {
#ifndef NDEBUG
    // Try to trigger segfault / asan failure in non-opt builds if the arena
    // lifetime has ended before the destructor. Note that `GetArena()` is
    // not free, but this is debug-only.
    const Arena* arena = GetArena();
    if (arena != nullptr) (void)arena->SpaceAllocated();
#endif
  }

  bool empty() const { return current_size_ == 0; }
  int size() const { return current_size_; }
  // Returns the size of the buffer with pointers to elements.
  //
  // Note:
  //
  //   * prefer `SizeAtCapacity()` to `size() == Capacity()`;
  //   * prefer `AllocatedSizeAtCapacity()` to `allocated_size() == Capacity()`.
  int Capacity() const { return using_sso() ? kSSOCapacity : rep()->capacity; }

  template <typename TypeHandler>
  const Value<TypeHandler>& at(int index) const {
    ABSL_CHECK_GE(index, 0);
    ABSL_CHECK_LT(index, current_size_);
    return *cast<TypeHandler>(element_at(index));
  }

  template <typename TypeHandler>
  Value<TypeHandler>& at(int index) {
    ABSL_CHECK_GE(index, 0);
    ABSL_CHECK_LT(index, current_size_);
    return *cast<TypeHandler>(element_at(index));
  }

  template <typename TypeHandler>
  Value<TypeHandler>* Mutable(int index) {
    RuntimeAssertInBounds(index, current_size_);
    return cast<TypeHandler>(element_at(index));
  }

  template <typename TypeHandler>
  Value<TypeHandler>* Add(Arena* arena) {
    return cast<TypeHandler>(AddInternal(arena, TypeHandler::GetNewFunc()));
  }

  template <typename TypeHandler>
  void Add(Arena* arena, Value<TypeHandler>&& value) {
    if (ClearedCount() > 0) {
      *cast<TypeHandler>(element_at(ExchangeCurrentSize(current_size_ + 1))) =
          std::move(value);
    } else {
      AddInternal(arena, TypeHandler::GetNewWithMoveFunc(std::move(value)));
    }
  }

  // Must be called from destructor.
  //
  // Pre-condition: NeedsDestroy() returns true.
  template <typename TypeHandler>
  void Destroy() {
    ABSL_DCHECK(NeedsDestroy());
    ABSL_DCHECK_EQ(GetArena(), nullptr);

    using H = CommonHandler<TypeHandler>;
    int n = allocated_size();
    ABSL_DCHECK_LE(n, Capacity());
    void** elems = elements();
    for (int i = 0; i < n; i++) {
      if (i + 5 < n) {
        absl::PrefetchToLocalCacheNta(elems[i + 5]);
      }
      Delete<H>(elems[i], nullptr);
    }
    if (!using_sso()) {
      internal::SizedDelete(rep(),
                            Capacity() * sizeof(elems[0]) + kRepHeaderSize);
    }
  }

  inline bool NeedsDestroy() const {
    // Either there is an allocated element in SSO buffer or there is an
    // allocated Rep.
    return tagged_rep_or_elem_ != nullptr;
  }

  // Pre-condition: NeedsDestroy() returns true.
  void DestroyProtos();

 public:
  // The next few methods are public so that they can be called from generated
  // code when implicit weak fields are used, but they should never be called by
  // application code.

  template <typename TypeHandler>
  PROTOBUF_FUTURE_ADD_NODISCARD const Value<TypeHandler>& Get(int index) const {
    if constexpr (GetBoundsCheckMode() == BoundsCheckMode::kReturnDefault) {
      if (ABSL_PREDICT_FALSE(index < 0 || index >= current_size_)) {
        // `default_instance()` is not supported for MessageLite and Message.
        if constexpr (TypeHandler::has_default_instance()) {
          LogIndexOutOfBounds(index, current_size_);
          return TypeHandler::default_instance();
        }
      }
    }
    // We refactor this to a separate function instead of inlining it so we
    // can measure the performance impact more easily.
    RuntimeAssertInBounds(index, current_size_);
    return *cast<TypeHandler>(element_at(index));
  }

  // Creates and adds an element using the given prototype, without introducing
  // a link-time dependency on the concrete message type.
  //
  // Pre-condition: `prototype` must not be nullptr.
  template <typename TypeHandler>
  PROTOBUF_ALWAYS_INLINE Value<TypeHandler>* AddFromPrototype(
      Arena* arena, const Value<TypeHandler>* prototype) {
    using H = CommonHandler<TypeHandler>;
    Value<TypeHandler>* result = cast<TypeHandler>(
        AddInternal(arena, H::GetNewFromPrototypeFunc(prototype)));
    return result;
  }

  // As above, but returns a callable that caches useful information that can be
  // reused between add calls.
  template <typename TypeHandler>
  PROTOBUF_ALWAYS_INLINE auto GetAdderFromPrototype(
      const Value<TypeHandler>* prototype) {
    using H = CommonHandler<TypeHandler>;
    auto func = H::GetNewFromPrototypeFunc(prototype);
    return [this, func](Arena* arena) {
      return cast<TypeHandler>(AddInternal(arena, func));
    };
  }

  // Creates and adds an element using the given ClassData, without introducing
  // a link-time dependency on the concrete message type. This is generally
  // faster and should be preferred over the equivalent `AddFromPrototype()` if
  // the caller already has or can get the `ClassData`, and especially if
  // elements are added repeatedly.
  //
  // Pre-condition: `class_data` must not be nullptr.
  template <typename TypeHandler>
  PROTOBUF_ALWAYS_INLINE Value<TypeHandler>* AddFromClassData(
      Arena* arena, const ClassData* class_data) {
    using H = CommonHandler<TypeHandler>;
    Value<TypeHandler>* result = cast<TypeHandler>(
        AddInternal(arena, H::GetNewFromClassDataFunc(class_data)));
    return result;
  }

  template <typename TypeHandler>
  void Clear() {
    const int n = current_size_;
    ABSL_DCHECK_GE(n, 0);
    if (n > 0) {
      using H = CommonHandler<TypeHandler>;
      ClearNonEmpty<H>();
    }
  }

  template <typename T, typename CopyElementFn, typename CreateAndMergeFn>
  void MergeFromInternal(const RepeatedPtrFieldBase& from, Arena* arena,
                         CopyElementFn&& copy_fn,
                         CreateAndMergeFn&& create_and_merge_fn);
  template <typename T, typename CopyElementFn>
  void MergeFromInternal(const RepeatedPtrFieldBase& from, Arena* arena,
                         CopyElementFn&& copy_fn);

  // Appends all message values from `from` to this instance.
  template <typename T>
  void MergeFrom(const RepeatedPtrFieldBase& from, Arena* arena) {
    static_assert(std::is_base_of<MessageLite, T>::value, "");
    if constexpr (!std::is_base_of<Message, T>::value) {
      // For LITE objects we use the generic MergeFrom to save on binary size.
      return MergeFrom<MessageLite>(from, arena);
    }
    MergeFromConcreteMessage(from, arena, Arena::CopyConstruct<T>);
  }

  inline void InternalSwap(RepeatedPtrFieldBase* PROTOBUF_RESTRICT rhs) {
    ABSL_DCHECK(this != rhs);

    // Swap all fields except arena offset and arena pointer at once.
    internal::memswap<
        InternalMetadataResolverOffsetHelper<RepeatedPtrFieldBase>::value>(
        reinterpret_cast<char*>(this), reinterpret_cast<char*>(rhs));
  }

  // Returns true if there are no preallocated elements in the array.
  PROTOBUF_FUTURE_ADD_NODISCARD bool PrepareForParse() {
    return allocated_size() == current_size_;
  }

  // Similar to `AddAllocated` but faster.
  //
  // Pre-condition: PrepareForParse() is true.
  void AddAllocatedForParse(void* value, Arena* arena) {
    ABSL_DCHECK(PrepareForParse());
    if (ABSL_PREDICT_FALSE(SizeAtCapacity())) {
      *InternalExtend(1, arena) = value;
      ++rep()->allocated_size;
    } else {
      if (using_sso()) {
        tagged_rep_or_elem_ = value;
      } else {
        rep()->elements[current_size_] = value;
        ++rep()->allocated_size;
      }
    }
    ExchangeCurrentSize(current_size_ + 1);
  }

 protected:
  template <typename TypeHandler, typename AddOne>
  void ResizeImpl(int new_size, AddOne add_one);

  template <typename TypeHandler>
  void RemoveLast() {
    ABSL_DCHECK_GT(current_size_, 0);
    ExchangeCurrentSize(current_size_ - 1);
    using H = CommonHandler<TypeHandler>;
    H::Clear(cast<H>(element_at(current_size_)));
  }

  template <typename TypeHandler>
  void CopyFrom(const RepeatedPtrFieldBase& other, Arena* arena) {
    ABSL_DCHECK_EQ(arena, GetArena());
    if (&other == this) return;
    Clear<TypeHandler>();
    if (other.empty()) return;
    MergeFrom<typename TypeHandler::Type>(other, arena);
  }

  void CloseGap(int start, int num);

  void ReserveWithArena(Arena* arena, int capacity);

  template <typename TypeHandler>
  static inline Value<TypeHandler>* copy(const Value<TypeHandler>* value) {
    return cast<TypeHandler>(CloneSlow(nullptr, *value));
  }

  // Used for constructing iterators.
  void* const* raw_data() const { return elements(); }
  void** raw_mutable_data() { return elements(); }

  template <typename TypeHandler>
  Value<TypeHandler>** mutable_data() {
    // TODO:  Breaks C++ aliasing rules.  We should probably remove this
    //   method entirely.
    return reinterpret_cast<Value<TypeHandler>**>(raw_mutable_data());
  }

  template <typename TypeHandler>
  const Value<TypeHandler>* const* data() const {
    // TODO:  Breaks C++ aliasing rules.  We should probably remove this
    //   method entirely.
    return reinterpret_cast<const Value<TypeHandler>* const*>(raw_data());
  }

  template <typename TypeHandler>
  PROTOBUF_NDEBUG_INLINE void Swap(Arena* arena, RepeatedPtrFieldBase* other,
                                   Arena* other_arena) {
    ABSL_DCHECK_EQ(arena, GetArena());
    ABSL_DCHECK_EQ(other_arena, other->GetArena());
    if (internal::CanUseInternalSwap(arena, other_arena)) {
      InternalSwap(other);
    } else {
      SwapFallback<TypeHandler>(arena, other, other_arena);
    }
  }

  void SwapElements(int index1, int index2) {
    internal::RuntimeAssertInBounds(index1, current_size_);
    internal::RuntimeAssertInBounds(index2, current_size_);
    using std::swap;  // enable ADL with fallback
    swap(element_at(index1), element_at(index2));
  }

  template <typename TypeHandler>
  PROTOBUF_NOINLINE size_t SpaceUsedExcludingSelfLong() const {
    size_t allocated_bytes =
        using_sso()
            ? 0
            : static_cast<size_t>(Capacity()) * sizeof(void*) + kRepHeaderSize;
    const int n = allocated_size();
    void* const* elems = elements();
    for (int i = 0; i < n; ++i) {
      allocated_bytes +=
          TypeHandler::SpaceUsedLong(*cast<TypeHandler>(elems[i]));
    }
    return allocated_bytes;
  }

  // Advanced memory management --------------------------------------

  // Like Add(), but if there are no cleared objects to use, returns nullptr.
  template <typename TypeHandler>
  Value<TypeHandler>* AddFromCleared() {
    if (current_size_ < allocated_size()) {
      return cast<TypeHandler>(
          element_at(ExchangeCurrentSize(current_size_ + 1)));
    } else {
      return nullptr;
    }
  }

  template <typename TypeHandler>
  void AddAllocated(Arena* arena, Value<TypeHandler>* value) {
    ABSL_DCHECK_EQ(arena, GetArena());
    ABSL_DCHECK_NE(value, nullptr);
    Arena* element_arena = TypeHandler::GetArena(value);
    if (arena != element_arena || AllocatedSizeAtCapacity()) {
      AddAllocatedSlowWithCopy<TypeHandler>(value, element_arena, arena);
      return;
    }
    // Fast path: underlying arena representation (tagged pointer) is equal to
    // our arena pointer, and we can add to array without resizing it (at
    // least one slot that is not allocated).
    void** elems = elements();
    if (current_size_ < allocated_size()) {
      // Make space at [current] by moving first allocated element to end of
      // allocated list.
      elems[allocated_size()] = elems[current_size_];
    }
    elems[ExchangeCurrentSize(current_size_ + 1)] = value;
    if (!using_sso()) ++rep()->allocated_size;
  }

  template <typename TypeHandler>
  void UnsafeArenaAddAllocated(Arena* arena, Value<TypeHandler>* value) {
    ABSL_DCHECK_EQ(arena, GetArena());
    ABSL_DCHECK_NE(value, nullptr);
    // Make room for the new pointer.
    if (SizeAtCapacity()) {
      // The array is completely full with no cleared objects, so grow it.
      InternalExtend(1, arena);
      ++rep()->allocated_size;
    } else if (AllocatedSizeAtCapacity()) {
      // There is no more space in the pointer array because it contains some
      // cleared objects awaiting reuse.  We don't want to grow the array in
      // this case because otherwise a loop calling AddAllocated() followed by
      // Clear() would leak memory.
      using H = CommonHandler<TypeHandler>;
      Delete<H>(element_at(current_size_), arena);
    } else if (current_size_ < allocated_size()) {
      // We have some cleared objects.  We don't care about their order, so we
      // can just move the first one to the end to make space.
      element_at(allocated_size()) = element_at(current_size_);
      ++rep()->allocated_size;
    } else {
      // There are no cleared objects.
      if (!using_sso()) ++rep()->allocated_size;
    }

    element_at(ExchangeCurrentSize(current_size_ + 1)) = value;
  }

  template <typename TypeHandler>
  PROTOBUF_FUTURE_ADD_NODISCARD Value<TypeHandler>* ReleaseLast(Arena* arena) {
    ABSL_DCHECK_EQ(arena, GetArena());
    Value<TypeHandler>* result = UnsafeArenaReleaseLast<TypeHandler>();
    // Now perform a copy if we're on an arena.

    if (internal::DebugHardenForceCopyInRelease()) {
      auto* new_result = copy<TypeHandler>(result);
      if (arena == nullptr) delete result;
      return new_result;
    } else {
      return (arena == nullptr) ? result : copy<TypeHandler>(result);
    }
  }

  // Releases and returns the last element, but does not do out-of-arena copy.
  // Instead, just returns the raw pointer to the contained element in the
  // arena.
  template <typename TypeHandler>
  Value<TypeHandler>* UnsafeArenaReleaseLast() {
    internal::RuntimeAssertInBounds(0, current_size_);
    ExchangeCurrentSize(current_size_ - 1);
    auto* result = cast<TypeHandler>(element_at(current_size_));
    if (using_sso()) {
      tagged_rep_or_elem_ = nullptr;
    } else {
      --rep()->allocated_size;
      if (current_size_ < allocated_size()) {
        // There are cleared elements on the end; replace the removed element
        // with the last allocated element.
        element_at(current_size_) = element_at(allocated_size());
      }
    }
    return result;
  }

  int ClearedCount() const { return allocated_size() - current_size_; }

  // Slowpath handles all cases, copying if necessary.
  template <typename TypeHandler>
  PROTOBUF_NOINLINE void AddAllocatedSlowWithCopy(
      // Pass value_arena and my_arena to avoid duplicate virtual call (value)
      // or load (mine).
      Value<TypeHandler>* value, Arena* value_arena, Arena* my_arena) {
    using H = CommonHandler<TypeHandler>;
    ABSL_DCHECK_EQ(my_arena, GetArena());
    ABSL_DCHECK_EQ(value_arena, TypeHandler::GetArena(value));
    // Ensure that either the value is in the same arena, or if not, we do the
    // appropriate thing: Own() it (if it's on heap and we're in an arena) or
    // copy it to our arena/heap (otherwise).
    if (my_arena != nullptr && value_arena == nullptr) {
      my_arena->Own(value);
    } else if (my_arena != value_arena) {
      ABSL_DCHECK(value_arena != nullptr);
      value = cast<TypeHandler>(CloneSlow(my_arena, *value));
    }

    UnsafeArenaAddAllocated<H>(my_arena, value);
  }

  template <typename TypeHandler>
  PROTOBUF_NOINLINE void SwapFallbackWithTemp(Arena* arena,
                                              RepeatedPtrFieldBase* other,
                                              Arena* other_arena,
                                              RepeatedPtrFieldBase& temp);

  template <typename TypeHandler>
  PROTOBUF_NOINLINE void SwapFallback(Arena* arena, RepeatedPtrFieldBase* other,
                                      Arena* other_arena);

  // Gets the Arena on which this RepeatedPtrField stores its elements.
  inline Arena* GetArena() const {
    return ResolveArena<&RepeatedPtrFieldBase::resolver_>(this);
  }

 private:
  // Tests that need to access private methods.
  friend class RepeatedPtrFieldTest;
  friend class
      RepeatedPtrFieldTest_UnsafeArenaAddAllocatedReleaseLastOnBaseField_Test;

  using InternalArenaConstructable_ = void;
  using DestructorSkippable_ = void;

  // FieldWithArena needs to call our protected internal metadata offset
  // constructors.
  friend class internal::FieldWithArena<RepeatedPtrFieldBase>;

  friend google::protobuf::Arena;

  template <typename T>
  friend class Arena::InternalHelper;

  // ExtensionSet stores repeated message extensions as
  // RepeatedPtrField<MessageLite>, but non-lite ExtensionSets need to implement
  // SpaceUsedLong(), and thus need to call SpaceUsedExcludingSelfLong()
  // reinterpreting MessageLite as Message.  ExtensionSet also needs to make use
  // of AddFromCleared(), which is not part of the public interface.
  friend class ExtensionSet;

  // The MapFieldBase implementation needs to call protected methods directly,
  // reinterpreting pointers as being to Message instead of a specific Message
  // subclass.
  friend class MapFieldBase;
  friend struct MapFieldTestPeer;

  // The table-driven MergePartialFromCodedStream implementation needs to
  // operate on RepeatedPtrField<MessageLite>.
  friend class MergePartialFromCodedStreamHelper;

  friend class AccessorHelper;

  template <typename T>
  friend struct google::protobuf::WeakRepeatedPtrField;

  friend class internal::TcParser;  // TODO: Remove this friend.

  // Expose offset of `resolver_` without exposing the member itself. Used to
  // optimize code size of `InternalSwap` method.
  template <typename T>
  friend struct InternalMetadataResolverOffsetHelper;

  // The reflection implementation needs to call protected methods directly,
  // reinterpreting pointers as being to Message instead of a specific Message
  // subclass.
  friend class google::protobuf::Reflection;
  friend class internal::SwapFieldHelper;

  friend class RustRepeatedMessageHelper;

  // Concrete Arena enabled copy function used to copy messages instances.
  // This follows the `Arena::CopyConstruct` signature so that the compiler
  // can have the inlined call into the out of line copy function(s) simply pass
  // the address of `Arena::CopyConstruct` 'as is'.
  using CopyFn = void* (*)(Arena*, const void*);

  struct Rep {
    // The size of the elements array, in number of elements.
    int capacity;
    // The number of elements allocated in the elements array (including cleared
    // elements). This is always >= current_size.
    int allocated_size;
    // Here we declare a huge array as a way of approximating C's "flexible
    // array member" feature without relying on undefined behavior.
    void* elements[(std::numeric_limits<int>::max() - 2 * sizeof(int)) /
                   sizeof(void*)];
  };

  static constexpr size_t kRepHeaderSize = offsetof(Rep, elements);

  // Replaces current_size_ with new_size and returns the previous value of
  // current_size_. This function is intended to be the only place where
  // current_size_ is modified.
  inline int ExchangeCurrentSize(int new_size) {
    return std::exchange(current_size_, new_size);
  }
  inline bool SizeAtCapacity() const {
    // Harden invariant size() <= allocated_size() <= Capacity().
    ABSL_DCHECK_LE(size(), allocated_size());
    ABSL_DCHECK_LE(allocated_size(), Capacity());
    return current_size_ == Capacity();
  }
  inline bool AllocatedSizeAtCapacity() const {
    // Harden invariant size() <= allocated_size() <= Capacity().
    ABSL_DCHECK_LE(size(), allocated_size());
    ABSL_DCHECK_LE(allocated_size(), Capacity());
    return allocated_size() == Capacity();
  }

  void* const* elements() const {
    return using_sso() ? &tagged_rep_or_elem_ : +rep()->elements;
  }
  void** elements() {
    return using_sso() ? &tagged_rep_or_elem_ : +rep()->elements;
  }

  void*& element_at(int index) {
    if (using_sso()) {
      ABSL_DCHECK_EQ(index, 0);
      return tagged_rep_or_elem_;
    }
    return rep()->elements[index];
  }
  const void* element_at(int index) const {
    return const_cast<RepeatedPtrFieldBase*>(this)->element_at(index);
  }

  int allocated_size() const {
    return using_sso() ? (tagged_rep_or_elem_ != nullptr ? 1 : 0)
                       : rep()->allocated_size;
  }
  Rep* rep() {
    ABSL_DCHECK(!using_sso());
    return reinterpret_cast<Rep*>(
        reinterpret_cast<uintptr_t>(tagged_rep_or_elem_) - 1);
  }
  const Rep* rep() const {
    return const_cast<RepeatedPtrFieldBase*>(this)->rep();
  }

  bool using_sso() const {
    return (reinterpret_cast<uintptr_t>(tagged_rep_or_elem_) & 1) == 0;
  }

  template <typename TypeHandler>
  static inline Value<TypeHandler>* cast(void* element) {
    return reinterpret_cast<Value<TypeHandler>*>(element);
  }
  template <typename TypeHandler>
  static inline const Value<TypeHandler>* cast(const void* element) {
    return reinterpret_cast<const Value<TypeHandler>*>(element);
  }
  template <typename TypeHandler>
  static inline void Delete(void* obj, Arena* arena) {
    TypeHandler::Delete(cast<TypeHandler>(obj), arena);
  }

  // Out-of-line helper routine for Clear() once the inlined check has
  // determined the container is non-empty
  template <typename TypeHandler>
  PROTOBUF_NOINLINE void ClearNonEmpty() {
    const int n = current_size_;
    void* const* elems = elements();
    int i = 0;
    ABSL_DCHECK_GT(n, 0);
    // do/while loop to avoid initial test because we know n > 0
    do {
      TypeHandler::Clear(cast<TypeHandler>(elems[i++]));
    } while (i < n);
    ExchangeCurrentSize(0);
  }

  // Merges messages from `from` into available, cleared messages sitting in the
  // range `[size(), allocated_size())`. Returns the number of message merged
  // which is `ClearedCount(), from.size())`.
  // Note that this function does explicitly NOT update `current_size_`.
  // This function is out of line as it should be the slow path: this scenario
  // only happens when a caller constructs and fills a repeated field, then
  // shrinks it, and then merges additional messages into it.
  int MergeIntoClearedMessages(const RepeatedPtrFieldBase& from);

  // Appends all messages from `from` to this instance, using the
  // provided `copy_fn` copy function to copy existing messages.
  void MergeFromConcreteMessage(const RepeatedPtrFieldBase& from, Arena* arena,
                                CopyFn copy_fn);

  // Extends capacity by at least |extend_amount|. Returns a pointer to the
  // next available element slot.
  //
  // Pre-condition: |extend_amount| must be > 0.
  void** InternalExtend(int extend_amount, Arena* arena);

  // Ensures that capacity is at least `n` elements.
  // Returns a pointer to the element directly beyond the last element.
  inline void** InternalReserve(int n, Arena* arena) {
    if (n <= Capacity()) {
      void** elements = using_sso() ? &tagged_rep_or_elem_ : rep()->elements;
      return elements + current_size_;
    }
    return InternalExtend(n - Capacity(), arena);
  }

  // Common implementation used by various Add* methods. `factory` is an object
  // used to construct a new element unless there are spare cleared elements
  // ready for reuse. Returns pointer to the new element.
  void* AddInternal(Arena* arena, absl::FunctionRef<ElementNewFn> factory);

  // A few notes on internal representation:
  //
  // We use an indirected approach, with struct Rep, to keep
  // sizeof(RepeatedPtrFieldBase) small. An instance of Rep is allocated only
  // when the repeated field is non-empty, and it is a dynamically-sized struct
  // (the header is directly followed by elements[]). We place current_size_
  // directly in the object to avoid cache misses due to the indirection,
  // because this field is checked frequently. Placing all fields directly in
  // the RepeatedPtrFieldBase instance would cost significant performance for
  // memory-sensitive workloads.
  void* tagged_rep_or_elem_;
  int current_size_;
  InternalMetadataResolver resolver_;
};

// Appends all message values from `from` to this instance using the abstract
// message interface. This overload is used in places like reflection and
// other locations where the underlying type is unavailable
template <>
void RepeatedPtrFieldBase::MergeFrom<MessageLite>(
    const RepeatedPtrFieldBase& from, Arena* arena);

template <>
inline void RepeatedPtrFieldBase::MergeFrom<Message>(
    const RepeatedPtrFieldBase& from, Arena* arena) {
  return MergeFrom<MessageLite>(from, arena);
}

// Appends all `std::string` values from `from` to this instance.
template <>
PROTOBUF_EXPORT void RepeatedPtrFieldBase::MergeFrom<std::string>(
    const RepeatedPtrFieldBase& from, Arena* arena);


inline void* RepeatedPtrFieldBase::AddInternal(
    Arena* arena, absl::FunctionRef<ElementNewFn> factory) {
  ABSL_DCHECK_EQ(arena, GetArena());
  if (tagged_rep_or_elem_ == nullptr) {
    ExchangeCurrentSize(1);
    factory(arena, tagged_rep_or_elem_);
    return tagged_rep_or_elem_;
  }
  absl::PrefetchToLocalCache(tagged_rep_or_elem_);
  if (using_sso()) {
    if (current_size_ == 0) {
      ExchangeCurrentSize(1);
      return tagged_rep_or_elem_;
    }
    void*& result = *InternalExtend(1, arena);
    factory(arena, result);
    Rep* r = rep();
    r->allocated_size = 2;
    ExchangeCurrentSize(2);
    return result;
  }
  Rep* r = rep();
  if (ABSL_PREDICT_FALSE(SizeAtCapacity())) {
    InternalExtend(1, arena);
    r = rep();
  } else {
    if (ClearedCount() > 0) {
      return r->elements[ExchangeCurrentSize(current_size_ + 1)];
    }
  }
  ++r->allocated_size;
  void*& result = r->elements[ExchangeCurrentSize(current_size_ + 1)];
  factory(arena, result);
  return result;
}

// A container that holds a RepeatedPtrFieldBase and an arena pointer. This is
// used when constructing `RepeatedPtrFieldBase`s on the arena, and in
// `SwapFallback`.
using RepeatedPtrFieldWithArenaBase = FieldWithArena<RepeatedPtrFieldBase>;

template <>
struct FieldArenaRep<RepeatedPtrFieldBase> {
  using Type = RepeatedPtrFieldWithArenaBase;

  static inline RepeatedPtrFieldBase* Get(
      RepeatedPtrFieldWithArenaBase* arena_rep) {
    return &arena_rep->field();
  }
};

template <>
struct FieldArenaRep<const RepeatedPtrFieldBase> {
  using Type = const RepeatedPtrFieldWithArenaBase;

  static inline const RepeatedPtrFieldBase* Get(
      const RepeatedPtrFieldWithArenaBase* arena_rep) {
    return &arena_rep->field();
  }
};

template <typename TypeHandler>
PROTOBUF_NOINLINE void RepeatedPtrFieldBase::SwapFallbackWithTemp(
    Arena* arena, RepeatedPtrFieldBase* other, Arena* other_arena,
    RepeatedPtrFieldBase& temp) {
  ABSL_DCHECK(!internal::CanUseInternalSwap(GetArena(), other->GetArena()));
  ABSL_DCHECK_EQ(arena, GetArena());
  ABSL_DCHECK_EQ(other_arena, other->GetArena());

  // Copy semantics in this case. We try to improve efficiency by placing the
  // temporary on |other|'s arena so that messages are copied twice rather
  // than three times.
  if (!this->empty()) {
    temp.MergeFrom<typename TypeHandler::Type>(*this, other_arena);
  }
  this->CopyFrom<TypeHandler>(*other, arena);
  other->InternalSwap(&temp);
}

template <typename TypeHandler>
PROTOBUF_NOINLINE void RepeatedPtrFieldBase::SwapFallback(
    Arena* arena, RepeatedPtrFieldBase* other, Arena* other_arena) {
  ABSL_DCHECK(!internal::CanUseInternalSwap(GetArena(), other->GetArena()));
  ABSL_DCHECK_EQ(arena, GetArena());
  ABSL_DCHECK_EQ(other_arena, other->GetArena());

  // Copy semantics in this case. We try to improve efficiency by placing the
  // temporary on |other|'s arena so that messages are copied twice rather
  // than three times.
  if (other_arena != nullptr) {
    // We can't call the destructor of the temp container since it allocates
    // memory from an arena, and the destructor of FieldWithArena expects to be
    // called only when arena is nullptr.
    absl::NoDestructor<RepeatedPtrFieldWithArenaBase> temp_container(
        other_arena);
    RepeatedPtrFieldBase& temp = temp_container->field();
    SwapFallbackWithTemp<TypeHandler>(arena, other, other_arena, temp);
    return;
  }

  RepeatedPtrFieldBase temp;
  SwapFallbackWithTemp<TypeHandler>(arena, other, other_arena, temp);
  if (temp.NeedsDestroy()) {
    temp.Destroy<TypeHandler>();
  }
}

template <typename TypeHandler, typename AddOne>
void RepeatedPtrFieldBase::ResizeImpl(int new_size, AddOne add_one) {
  internal::RuntimeAssertInBoundsGE(new_size, 0);
  int diff = new_size - size();
  if (diff > 0) {
    // We need to add.
    auto* arena = GetArena();
    ReserveWithArena(arena, new_size);
    for (; diff > 0; --diff) {
      add_one(arena);
    }
  } else {
    for (; diff < 0; ++diff) {
      RemoveLast<TypeHandler>();
    }
  }
}

PROTOBUF_EXPORT void InternalOutOfLineDeleteMessageLite(MessageLite* message);

// Encapsulates the minimally required subset of T's properties in a
// `RepeatedPtrField<T>` specialization so the type-agnostic
// `RepeatedPtrFieldBase` could do its job without knowing T.
//
// This generic definition is for types derived from `MessageLite`. That is
// statically asserted, but only where a non-conforming type would emit a
// compile-time diagnostic that lacks proper guidance for fixing. Asserting
// at the top level isn't possible, because some template argument types are not
// yet fully defined at the instantiation point.
//
// Explicit specializations are provided for `std::string` and
// `StringPieceField` further below.
template <typename GenericType>
class GenericTypeHandler {
 public:
  using Type = GenericType;

  using CopyConstructReferenceType = const Type&;

  // NOTE: Can't `static_assert(std::is_base_of_v<MessageLite, Type>)` here,
  // because the type is not yet fully defined at this point sometimes, so we
  // are forced to assert in every function that needs it.

  static constexpr auto GetNewFunc() {
    return [](Arena* arena, void*& ptr) {
      ptr = Arena::DefaultConstruct<Type>(arena);
    };
  }
  static constexpr auto GetNewWithMoveFunc(
      Type&& from ABSL_ATTRIBUTE_LIFETIME_BOUND) {
    return [&from](Arena* arena, void*& ptr) {
      ptr = Arena::Create<Type>(arena, std::move(from));
    };
  }
  static constexpr auto GetNewFromPrototypeFunc(
      const Type* prototype ABSL_ATTRIBUTE_LIFETIME_BOUND) {
    ABSL_DCHECK(prototype != nullptr);
    return [prototype](Arena* arena, void*& ptr) {
      ptr = GetClassData(*prototype)->New(arena);
    };
  }
  static constexpr auto GetNewFromClassDataFunc(
      const ClassData* class_data ABSL_ATTRIBUTE_LIFETIME_BOUND) {
    ABSL_DCHECK(class_data != nullptr);
    return [class_data](Arena* arena, void*& ptr) {
      ptr = class_data->New(arena);
    };
  }

  static inline Arena* GetArena(Type* value) {
    return Arena::InternalGetArena(value);
  }

  static inline void Delete(Type* value, Arena* arena) {
    static_assert(std::is_base_of_v<MessageLite, Type>);
    if (arena != nullptr) return;
    // Using virtual destructor to reduce generated code size that would have
    // happened otherwise due to inlined `~Type()`.
    InternalOutOfLineDeleteMessageLite(value);
  }
  static inline void Clear(Type* value) {
    static_assert(std::is_base_of_v<MessageLite, Type>);
    value->Clear();
  }
  static inline size_t SpaceUsedLong(const Type& value) {
    // NOTE: For `SpaceUsedLong()`, we do need `Message`, not `MessageLite`.
    static_assert(std::is_base_of_v<Message, Type>);
    return value.SpaceUsedLong();
  }

  static void CopyFrom(Type* elem, const Type& value) {
    elem->CheckTypeAndMergeFrom(value);
  }

  static const Type& default_instance() {
    static_assert(has_default_instance());
    return *static_cast<const GenericType*>(
        MessageTraits<Type>::default_instance());
  }
  static constexpr bool has_default_instance() {
    return !std::is_same_v<Type, Message> && !std::is_same_v<Type, MessageLite>;
  }

  static const Type& ForEraseIf(const Type* ptr) { return *ptr; }
};

template <>
class GenericTypeHandler<std::string> {
 public:
  using Type = std::string;

  using CopyConstructReferenceType = absl::string_view;

  static constexpr auto GetNewFunc() {
    return [](Arena* arena, void*& ptr) { ptr = Arena::Create<Type>(arena); };
  }
  static constexpr auto GetNewWithMoveFunc(
      Type&& from ABSL_ATTRIBUTE_LIFETIME_BOUND) {
    return [&from](Arena* arena, void*& ptr) {
      ptr = Arena::Create<Type>(arena, std::move(from));
    };
  }
  static constexpr auto GetNewFromPrototypeFunc(const Type* /*prototype*/) {
    return GetNewFunc();
  }

  static inline Arena* GetArena(Type*) { return nullptr; }

  static inline void Delete(Type* value, Arena* arena) {
    if (arena == nullptr) {
      delete value;
    }
  }
  static inline void Clear(Type* value) { value->clear(); }
  static inline void Merge(const Type& from, Type* to) { *to = from; }
  static size_t SpaceUsedLong(const Type& value) {
    return sizeof(value) + StringSpaceUsedExcludingSelfLong(value);
  }

  static void CopyFrom(Type* elem, absl::string_view value) {
    elem->assign(value.data(), value.size());
  }

  static const Type& default_instance() {
    return GetEmptyStringAlreadyInited();
  }
  static constexpr bool has_default_instance() { return true; }

  static absl::string_view ForEraseIf(const std::string* ptr) { return *ptr; }
};


}  // namespace internal

// RepeatedPtrField is like RepeatedField, but used for repeated strings or
// Messages.
template <typename Element>
class ABSL_ATTRIBUTE_WARN_UNUSED RepeatedPtrField final
    : private internal::RepeatedPtrFieldBase {
  static_assert(!std::is_const<Element>::value,
                "We do not support const value types.");
  static_assert(!std::is_volatile<Element>::value,
                "We do not support volatile value types.");
  static_assert(!std::is_pointer<Element>::value,
                "We do not support pointer value types.");
  static_assert(!std::is_reference<Element>::value,
                "We do not support reference value types.");
  static constexpr PROTOBUF_ALWAYS_INLINE void StaticValidityCheck() {
    static_assert(
        std::disjunction<
            internal::is_supported_string_type<Element>,
            internal::is_supported_message_type<Element>>::value,
        "We only support string and Message types in RepeatedPtrField.");
    static_assert(alignof(Element) <= internal::ArenaAlignDefault::align,
                  "Overaligned types are not supported");
  }

  using CopyConstructReferenceType = typename internal::GenericTypeHandler<
      Element>::CopyConstructReferenceType;

 public:
  using value_type = Element;
  using size_type = int;
  using difference_type = ptrdiff_t;
  using reference = Element&;
  using const_reference = const Element&;
  using pointer = Element*;
  using const_pointer = const Element*;
  using iterator = internal::RepeatedPtrIterator<Element>;
  using const_iterator = internal::RepeatedPtrIterator<const Element>;
  using reverse_iterator = std::reverse_iterator<iterator>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;
  // Custom STL-like iterator that iterates over and returns the underlying
  // pointers to Element rather than Element itself.
  using pointer_iterator = internal::RepeatedPtrOverPtrsIterator<Element>;
  using const_pointer_iterator =
      internal::RepeatedPtrOverPtrsIterator<const Element>;

  constexpr RepeatedPtrField();

  // Arena enabled constructors: for internal use only.
  constexpr PROTOBUF_ALWAYS_INLINE RepeatedPtrField(
      internal::InternalVisibility, internal::InternalMetadataOffset offset)
      : RepeatedPtrField(offset) {}
  PROTOBUF_ALWAYS_INLINE RepeatedPtrField(
      internal::InternalVisibility, internal::InternalMetadataOffset offset,
      const RepeatedPtrField& rhs)
      : RepeatedPtrField(offset, rhs) {}

  template <typename Iter,
            typename = typename std::enable_if<std::is_constructible<
                Element, decltype(*std::declval<Iter>())>::value>::type>
  RepeatedPtrField(Iter begin, Iter end);

  PROTOBUF_ALWAYS_INLINE RepeatedPtrField(const RepeatedPtrField& rhs)
      : RepeatedPtrField(internal::InternalMetadataOffset(), rhs) {}
  RepeatedPtrField& operator=(const RepeatedPtrField& other)
      ABSL_ATTRIBUTE_LIFETIME_BOUND;

  PROTOBUF_ALWAYS_INLINE RepeatedPtrField(RepeatedPtrField&& rhs) noexcept
      : RepeatedPtrField(internal::InternalMetadataOffset(), std::move(rhs)) {}
  RepeatedPtrField& operator=(RepeatedPtrField&& other) noexcept
      ABSL_ATTRIBUTE_LIFETIME_BOUND;

  ~RepeatedPtrField();

  PROTOBUF_FUTURE_ADD_NODISCARD bool empty() const;
  PROTOBUF_FUTURE_ADD_NODISCARD int size() const;

  PROTOBUF_FUTURE_ADD_NODISCARD const_reference
  Get(int index) const ABSL_ATTRIBUTE_LIFETIME_BOUND;
  PROTOBUF_FUTURE_ADD_NODISCARD pointer Mutable(int index)
      ABSL_ATTRIBUTE_LIFETIME_BOUND;

  // Unlike std::vector, adding an element to a RepeatedPtrField doesn't always
  // make a new element; it might re-use an element left over from when the
  // field was Clear()'d or resize()'d smaller.  For this reason, Add() is the
  // fastest API for adding a new element.
  pointer Add() ABSL_ATTRIBUTE_LIFETIME_BOUND;

  // `Add(std::move(value));` is equivalent to `*Add() = std::move(value);`
  // It will either move-construct to the end of this field, or swap value
  // with the new-or-recycled element at the end of this field.  Note that
  // this operation is very slow if this RepeatedPtrField is not on the
  // same Arena, if any, as `value`.
  void Add(Element&& value);

  // Copying to the end of this RepeatedPtrField is slowest of all; it can't
  // reliably copy-construct to the last element of this RepeatedPtrField, for
  // example (unlike std::vector).
  // We currently block this API.  The right way to add to the end is to call
  // Add() and modify the element it points to.
  // If you must add an existing value, call `*Add() = value;`
  void Add(const Element& value) = delete;

  // Append elements in the range [begin, end) after reserving
  // the appropriate number of elements.
  template <typename Iter>
  void Add(Iter begin, Iter end);

  // If `new_size < size()`, truncate the container, destroying the removed
  // elements.
  // If `new_size > size()`, add value-initialized elements to reach the desired
  // size.
  void resize(size_type new_size);

  // If `new_size < size()`, truncate the container, destroying the removed
  // elements.
  // If `new_size > size()`, add copies of `value` elements to reach the desired
  // size.
  void resize(size_type new_size, CopyConstructReferenceType value);

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

  // Removes the last element in the array.
  // Ownership of the element is retained by the array.
  void RemoveLast();

  // Deletes elements with indices in the range [start .. start+num-1].
  // Caution: moves all elements with indices [start+num .. ].
  // Calling this routine inside a loop can cause quadratic behavior.
  void DeleteSubrange(int start, int num);

  ABSL_ATTRIBUTE_REINITIALIZES void Clear();

  // Appends the elements from `other` after this instance.
  // The end result length will be `other.size() + this->size()`.
  void MergeFrom(const RepeatedPtrField& other);

  // Replaces the contents with a copy of the elements from `other`.
  ABSL_ATTRIBUTE_REINITIALIZES void CopyFrom(const RepeatedPtrField& other);

  // Replaces the contents with RepeatedPtrField(begin, end).
  template <typename Iter>
  ABSL_ATTRIBUTE_REINITIALIZES void Assign(Iter begin, Iter end);

  // Reserves space to expand the field to at least the given size.  This only
  // resizes the pointer array; it doesn't allocate any objects.  If the
  // array is grown, it will always be at least doubled in size.
  void Reserve(int new_size);

  PROTOBUF_FUTURE_ADD_NODISCARD int Capacity() const;

  // Gets the underlying array.  This pointer is possibly invalidated by
  // any add or remove operation.
  PROTOBUF_FUTURE_ADD_NODISCARD Element** mutable_data()
      ABSL_ATTRIBUTE_LIFETIME_BOUND;
  PROTOBUF_FUTURE_ADD_NODISCARD const Element* const* data() const
      ABSL_ATTRIBUTE_LIFETIME_BOUND;

  // Swaps entire contents with "other". If they are on separate arenas, then
  // copies data.
  void Swap(RepeatedPtrField* other);

  // Swaps entire contents with "other". Caller should guarantee that either
  // both fields are on the same arena or both are on the heap. Swapping between
  // different arenas with this function is disallowed and is caught via
  // ABSL_DCHECK.
  void UnsafeArenaSwap(RepeatedPtrField* other);

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

  PROTOBUF_FUTURE_ADD_NODISCARD pointer_iterator pointer_begin()
      ABSL_ATTRIBUTE_LIFETIME_BOUND;
  PROTOBUF_FUTURE_ADD_NODISCARD const_pointer_iterator
  pointer_begin() const ABSL_ATTRIBUTE_LIFETIME_BOUND;
  PROTOBUF_FUTURE_ADD_NODISCARD pointer_iterator pointer_end()
      ABSL_ATTRIBUTE_LIFETIME_BOUND;
  PROTOBUF_FUTURE_ADD_NODISCARD const_pointer_iterator
  pointer_end() const ABSL_ATTRIBUTE_LIFETIME_BOUND;

  // Returns (an estimate of) the number of bytes used by the repeated field,
  // excluding sizeof(*this).
  PROTOBUF_FUTURE_ADD_NODISCARD size_t SpaceUsedExcludingSelfLong() const;

  PROTOBUF_FUTURE_ADD_NODISCARD int SpaceUsedExcludingSelf() const {
    return internal::ToIntSize(SpaceUsedExcludingSelfLong());
  }

  // Advanced memory management --------------------------------------
  // When hardcore memory management becomes necessary -- as it sometimes
  // does here at Google -- the following methods may be useful.

  // Adds an already-allocated object, passing ownership to the
  // RepeatedPtrField.
  //
  // Note that some special behavior occurs with respect to arenas:
  //
  //   (i) if this field holds submessages, the new submessage will be copied if
  //   the original is in an arena and this RepeatedPtrField is either in a
  //   different arena, or on the heap.
  //   (ii) if this field holds strings, the passed-in string *must* be
  //   heap-allocated, not arena-allocated. There is no way to dynamically check
  //   this at runtime, so User Beware.
  // Requires:  value != nullptr
  void AddAllocated(Element* value);

  // Removes and returns the last element, passing ownership to the caller.
  // Requires:  size() > 0
  //
  // If this RepeatedPtrField is on an arena, an object copy is required to pass
  // ownership back to the user (for compatible semantics). Use
  // UnsafeArenaReleaseLast() if this behavior is undesired.
  PROTOBUF_FUTURE_ADD_NODISCARD Element* ReleaseLast();

  // Adds an already-allocated object, skipping arena-ownership checks. The user
  // must guarantee that the given object is in the same arena as this
  // RepeatedPtrField.
  // It is also useful in legacy code that uses temporary ownership to avoid
  // copies. Example:
  //   RepeatedPtrField<T> temp_field;
  //   temp_field.UnsafeArenaAddAllocated(new T);
  //   ... // Do something with temp_field
  //   temp_field.UnsafeArenaExtractSubrange(0, temp_field.size(), nullptr);
  // If you put temp_field on the arena this fails, because the ownership
  // transfers to the arena at the "AddAllocated" call and is not released
  // anymore, causing a double delete. UnsafeArenaAddAllocated prevents this.
  // Requires:  value != nullptr
  void UnsafeArenaAddAllocated(Element* value);

  // Removes and returns the last element.  Unlike ReleaseLast, the returned
  // pointer is always to the original object.  This may be in an arena, in
  // which case it would have the arena's lifetime.
  // Requires: current_size_ > 0
  pointer UnsafeArenaReleaseLast();

  // Extracts elements with indices in the range "[start .. start+num-1]".
  // The caller assumes ownership of the extracted elements and is responsible
  // for deleting them when they are no longer needed.
  // If "elements" is non-nullptr, then pointers to the extracted elements
  // are stored in "elements[0 .. num-1]" for the convenience of the caller.
  // If "elements" is nullptr, then the caller must use some other mechanism
  // to perform any further operations (like deletion) on these elements.
  // Caution: implementation also moves elements with indices [start+num ..].
  // Calling this routine inside a loop can cause quadratic behavior.
  //
  // Memory copying behavior is identical to ReleaseLast(), described above: if
  // this RepeatedPtrField is on an arena, an object copy is performed for each
  // returned element, so that all returned element pointers are to
  // heap-allocated copies. If this copy is not desired, the user should call
  // UnsafeArenaExtractSubrange().
  void ExtractSubrange(int start, int num, Element** elements);

  // Identical to ExtractSubrange() described above, except that no object
  // copies are ever performed. Instead, the raw object pointers are returned.
  // Thus, if on an arena, the returned objects must not be freed, because they
  // will not be heap-allocated objects.
  void UnsafeArenaExtractSubrange(int start, int num, Element** elements);

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

  // Gets the arena on which this RepeatedPtrField stores its elements.
  PROTOBUF_FUTURE_ADD_NODISCARD inline Arena* GetArena();

  // For internal use only.
  //
  // This is public due to it being called by generated code.
  void InternalSwap(RepeatedPtrField* PROTOBUF_RESTRICT other) {
    internal::RepeatedPtrFieldBase::InternalSwap(other);
  }

  // For internal use only.
  //
  // Like `Add()`, but uses the given arena instead of calling `GetArena()`. It
  // is the responsibility of the caller to ensure that this arena is the same
  // as the arena returned from `GetArena()`.
  pointer InternalAddWithArena(internal::InternalVisibility,
                               Arena* arena) ABSL_ATTRIBUTE_LIFETIME_BOUND;

  // For internal use only.
  //
  // Like `Add(Element&&)`, but uses the given arena instead of calling
  // `GetArena()`. It is the responsibility of the caller to ensure that this
  // arena is the same as the arena returned from `GetArena()`.
  void InternalAddWithArena(internal::InternalVisibility, Arena* arena,
                            Element&& value);

  // For internal use only.
  //
  // Like `MergeFrom(const RepeatedPtrField& other)`, but uses the given arena
  // instead of calling `GetArena()`. It is the responsibility of the caller to
  // ensure that this arena is the same as the arena returned from `GetArena()`.
  void InternalMergeFromWithArena(internal::InternalVisibility, Arena* arena,
                                  const RepeatedPtrField& other);

 private:
  using InternalArenaConstructable_ = void;
  using DestructorSkippable_ = void;

  // Friended to allow calling `*WithArena` variants.
  template <typename T>
  friend class internal::RepeatedPtrFieldBackInsertIterator;

  // Friended to allow calling `AddAllocated` from our private base class with
  // arena pointers, which may be cached in the iterator. This saves us needing
  // to call `GetArena()` on the `RepeatedPtrField` every insertion, which has 2
  // levels of indirection.
  template <typename T>
  friend class internal::AllocatedRepeatedPtrFieldBackInsertIterator;

  friend class internal::RepeatedPtrFieldTest;

  friend class Arena;

  friend class internal::FieldWithArena<RepeatedPtrField<Element>>;

  friend class DynamicMessage;

  friend class google::protobuf::Reflection;

  friend class google::protobuf::internal::ExtensionSet;

  friend class internal::MapFieldBase;

  friend class internal::TcParser;

  template <typename T>
  friend struct WeakRepeatedPtrField;

  // The MapFieldBase implementation needs to be able to static_cast down to
  // `RepeatedPtrFieldBase`.
  friend internal::MapFieldBase;

  friend class internal::v2::TableDrivenParse;

  // Note:  RepeatedPtrField SHOULD NOT be subclassed by users.
  using TypeHandler = internal::GenericTypeHandler<Element>;

  constexpr explicit RepeatedPtrField(internal::InternalMetadataOffset offset);
  RepeatedPtrField(internal::InternalMetadataOffset offset,
                   const RepeatedPtrField& rhs);
  RepeatedPtrField(internal::InternalMetadataOffset offset,
                   RepeatedPtrField&& rhs);


  pointer AddWithArena(Arena* arena) ABSL_ATTRIBUTE_LIFETIME_BOUND;

  void AddWithArena(Arena* arena, Element&& value);

  template <typename Iter>
  void AddWithArena(Arena* arena, Iter begin, Iter end);

  void AddAllocatedWithArena(Arena* arena, Element* value);

  PROTOBUF_FUTURE_ADD_NODISCARD Element* ReleaseLastWithArena(Arena* arena);

  void UnsafeArenaAddAllocatedWithArena(Arena* arena, Element* value);

  void ExtractSubrangeWithArena(Arena* arena, int start, int num,
                                Element** elements);

  void AddAllocatedForParse(Element* p, Arena* arena) {
    return RepeatedPtrFieldBase::AddAllocatedForParse(p, arena);
  }
};

// -------------------------------------------------------------------

template <typename Element>
constexpr RepeatedPtrField<Element>::RepeatedPtrField()
    : RepeatedPtrFieldBase() {
  // We can't have `StaticValidityCheck` here because it requires Element to
  // be a complete type, and split `RepeatedPtrField`s call
  // `Arena::DefaultConstruct` with an incomplete `Element`, in particular when
  // constructing `RepeatedPtrField`s without arena offsets in arena.h.
}

template <typename Element>
constexpr PROTOBUF_ALWAYS_INLINE RepeatedPtrField<Element>::RepeatedPtrField(
    internal::InternalMetadataOffset offset)
    : RepeatedPtrFieldBase(offset) {
  // We can't have StaticValidityCheck here because that requires Element to be
  // a complete type, and in split repeated fields cases, we call
  // CreateMessage<RepeatedPtrField<T>> for incomplete Ts.
}

template <typename Element>
PROTOBUF_ALWAYS_INLINE RepeatedPtrField<Element>::RepeatedPtrField(
    internal::InternalMetadataOffset offset, const RepeatedPtrField& rhs)
    : RepeatedPtrFieldBase(offset) {
  StaticValidityCheck();
  MergeFrom(rhs);
}

template <typename Element>
template <typename Iter, typename>
PROTOBUF_ALWAYS_INLINE RepeatedPtrField<Element>::RepeatedPtrField(Iter begin,
                                                                   Iter end) {
  StaticValidityCheck();
  Add(begin, end);
}

template <typename Element>
RepeatedPtrField<Element>::~RepeatedPtrField() {
  StaticValidityCheck();
  if (!NeedsDestroy()) return;
  if constexpr (std::is_base_of<MessageLite, Element>::value) {
    DestroyProtos();
  } else {
    Destroy<TypeHandler>();
  }
}

template <typename Element>
void RepeatedPtrField<Element>::resize(size_type new_size) {
  static_assert(!std::is_same_v<Element, Message>);
  static_assert(!std::is_same_v<Element, MessageLite>);
  ResizeImpl<TypeHandler>(new_size,
                          [&](auto* arena) { return AddWithArena(arena); });
}

template <typename Element>
void RepeatedPtrField<Element>::resize(size_type new_size,
                                       CopyConstructReferenceType value) {
  const auto adder = [&] {
    if constexpr (std::is_base_of_v<MessageLite, Element>) {
      return GetAdderFromPrototype<TypeHandler>(&value);
    } else {
      return [this](auto* arena) { return AddWithArena(arena); };
    }
  }();
  ResizeImpl<TypeHandler>(new_size, [adder, &value](auto* arena) {
    TypeHandler::CopyFrom(adder(arena), value);
  });
}

template <typename Element>
inline RepeatedPtrField<Element>& RepeatedPtrField<Element>::operator=(
    const RepeatedPtrField& other) ABSL_ATTRIBUTE_LIFETIME_BOUND {
  if (this != &other) CopyFrom(other);
  return *this;
}

template <typename Element>
inline RepeatedPtrField<Element>::RepeatedPtrField(
    internal::InternalMetadataOffset offset, RepeatedPtrField&& rhs)
    : RepeatedPtrFieldBase(offset) {
  // We don't just call Swap(&rhs) here because it would perform 3 copies if rhs
  // is on a different arena.
  Arena* arena = GetArena();
  if (internal::CanMoveWithInternalSwap(arena, rhs.GetArena())) {
    InternalSwap(&rhs);
  } else {
    RepeatedPtrFieldBase::CopyFrom<TypeHandler>(rhs, arena);
  }
}

template <typename Element>
inline RepeatedPtrField<Element>& RepeatedPtrField<Element>::operator=(
    RepeatedPtrField&& other) noexcept ABSL_ATTRIBUTE_LIFETIME_BOUND {
  // We don't just call Swap(&other) here because it would perform 3 copies if
  // the two fields are on different arenas.
  if (this != &other) {
    Arena* arena = GetArena();
    Arena* other_arena = other.GetArena();
    if (internal::CanMoveWithInternalSwap(arena, other_arena)) {
      InternalSwap(&other);
    } else {
      RepeatedPtrFieldBase::CopyFrom<TypeHandler>(other, arena);
    }
  }
  return *this;
}

template <typename Element>
inline bool RepeatedPtrField<Element>::empty() const {
  return RepeatedPtrFieldBase::empty();
}

template <typename Element>
inline int RepeatedPtrField<Element>::size() const {
  return RepeatedPtrFieldBase::size();
}

template <typename Element>
inline const Element& RepeatedPtrField<Element>::Get(int index) const
    ABSL_ATTRIBUTE_LIFETIME_BOUND {
  return RepeatedPtrFieldBase::Get<TypeHandler>(index);
}

template <typename Element>
inline const Element& RepeatedPtrField<Element>::at(int index) const
    ABSL_ATTRIBUTE_LIFETIME_BOUND {
  return RepeatedPtrFieldBase::at<TypeHandler>(index);
}

template <typename Element>
inline Element& RepeatedPtrField<Element>::at(int index)
    ABSL_ATTRIBUTE_LIFETIME_BOUND {
  return RepeatedPtrFieldBase::at<TypeHandler>(index);
}

template <typename Element>
inline Element* RepeatedPtrField<Element>::Mutable(int index)
    ABSL_ATTRIBUTE_LIFETIME_BOUND {
  return RepeatedPtrFieldBase::Mutable<TypeHandler>(index);
}

template <typename Element>
PROTOBUF_NDEBUG_INLINE Element* RepeatedPtrField<Element>::Add()
    ABSL_ATTRIBUTE_LIFETIME_BOUND {
  return AddWithArena(GetArena());
}

template <typename Element>
PROTOBUF_NDEBUG_INLINE Element* RepeatedPtrField<Element>::InternalAddWithArena(
    internal::InternalVisibility, Arena* arena) ABSL_ATTRIBUTE_LIFETIME_BOUND {
  return AddWithArena(arena);
}

template <typename Element>
PROTOBUF_NDEBUG_INLINE Element* RepeatedPtrField<Element>::AddWithArena(
    Arena* arena) ABSL_ATTRIBUTE_LIFETIME_BOUND {
  return RepeatedPtrFieldBase::Add<TypeHandler>(arena);
}

template <typename Element>
PROTOBUF_NDEBUG_INLINE void RepeatedPtrField<Element>::Add(Element&& value) {
  AddWithArena(GetArena(), std::move(value));
}

template <typename Element>
PROTOBUF_NDEBUG_INLINE void RepeatedPtrField<Element>::InternalAddWithArena(
    internal::InternalVisibility, Arena* arena, Element&& value) {
  AddWithArena(arena, std::move(value));
}

template <typename Element>
PROTOBUF_NDEBUG_INLINE void RepeatedPtrField<Element>::AddWithArena(
    Arena* arena, Element&& value) {
  RepeatedPtrFieldBase::Add<TypeHandler>(arena, std::move(value));
}

template <typename Element>
template <typename Iter>
PROTOBUF_NDEBUG_INLINE void RepeatedPtrField<Element>::Add(Iter begin,
                                                           Iter end) {
  AddWithArena(GetArena(), std::move(begin), std::move(end));
}

template <typename Element>
template <typename Iter>
PROTOBUF_NDEBUG_INLINE void RepeatedPtrField<Element>::AddWithArena(
    Arena* arena, Iter begin, Iter end) {
  if (std::is_base_of<
          std::forward_iterator_tag,
          typename std::iterator_traits<Iter>::iterator_category>::value) {
    int reserve = static_cast<int>(std::distance(begin, end));
    ReserveWithArena(arena, size() + reserve);
  }
  for (; begin != end; ++begin) {
    *AddWithArena(arena) = *begin;
  }
}

template <typename Element>
inline void RepeatedPtrField<Element>::RemoveLast() {
  RepeatedPtrFieldBase::RemoveLast<TypeHandler>();
}

template <typename Element>
inline void RepeatedPtrField<Element>::DeleteSubrange(int start, int num) {
  internal::RuntimeAssertInBoundsGE(start, 0);
  internal::RuntimeAssertInBoundsGE(num, 0);
  internal::RuntimeAssertInBoundsLE(static_cast<int64_t>(start) + num, size());
  void** subrange = raw_mutable_data() + start;
  Arena* arena = GetArena();
  for (int i = 0; i < num; ++i) {
    using H = CommonHandler<TypeHandler>;
    H::Delete(static_cast<Element*>(subrange[i]), arena);
  }
  UnsafeArenaExtractSubrange(start, num, nullptr);
}

template <typename Element>
inline void RepeatedPtrField<Element>::ExtractSubrange(int start, int num,
                                                       Element** elements) {
  ExtractSubrangeWithArena(GetArena(), start, num, elements);
}

template <typename Element>
inline void RepeatedPtrField<Element>::ExtractSubrangeWithArena(
    Arena* arena, int start, int num, Element** elements) {
  ABSL_DCHECK_EQ(arena, GetArena());
  internal::RuntimeAssertInBoundsGE(start, 0);
  internal::RuntimeAssertInBoundsGE(num, 0);
  internal::RuntimeAssertInBoundsLE(static_cast<int64_t>(start) + num, size());

  if (num == 0) return;

  ABSL_DCHECK_NE(elements, nullptr)
      << "Releasing elements without transferring ownership is an unsafe "
         "operation.  Use UnsafeArenaExtractSubrange.";
  if (elements != nullptr) {
    auto* extracted = data() + start;
    if (internal::DebugHardenForceCopyInRelease()) {
      // Always copy.
      for (int i = 0; i < num; ++i) {
        elements[i] = copy<TypeHandler>(extracted[i]);
      }
      if (arena == nullptr) {
        for (int i = 0; i < num; ++i) {
          delete extracted[i];
        }
      }
    } else {
      // If we're on an arena, we perform a copy for each element so that the
      // returned elements are heap-allocated. Otherwise, just forward it.
      if (arena != nullptr) {
        for (int i = 0; i < num; ++i) {
          elements[i] = copy<TypeHandler>(extracted[i]);
        }
      } else {
        memcpy(elements, extracted, num * sizeof(Element*));
      }
    }
  }
  CloseGap(start, num);
}

template <typename Element>
inline void RepeatedPtrField<Element>::UnsafeArenaExtractSubrange(
    int start, int num, Element** elements) {
  internal::RuntimeAssertInBoundsGE(start, 0);
  internal::RuntimeAssertInBoundsGE(num, 0);
  internal::RuntimeAssertInBoundsLE(static_cast<int64_t>(start) + num, size());

  if (num > 0) {
    // Save the values of the removed elements if requested.
    if (elements != nullptr) {
      memcpy(elements, data() + start, num * sizeof(Element*));
    }
    CloseGap(start, num);
  }
}

template <typename Element>
inline void RepeatedPtrField<Element>::Clear() {
  RepeatedPtrFieldBase::Clear<TypeHandler>();
}

template <typename Element>
inline void RepeatedPtrField<Element>::MergeFrom(
    const RepeatedPtrField& other) {
  if (other.empty()) return;
  RepeatedPtrFieldBase::MergeFrom<Element>(other, GetArena());
}

template <typename Element>
inline void RepeatedPtrField<Element>::InternalMergeFromWithArena(
    internal::InternalVisibility, Arena* arena, const RepeatedPtrField& other) {
  if (other.empty()) return;
  RepeatedPtrFieldBase::MergeFrom<Element>(other, arena);
}

template <typename Element>
inline void RepeatedPtrField<Element>::CopyFrom(const RepeatedPtrField& other) {
  RepeatedPtrFieldBase::CopyFrom<TypeHandler>(other, GetArena());
}

template <typename Element>
template <typename Iter>
inline void RepeatedPtrField<Element>::Assign(Iter begin, Iter end) {
  Clear();
  Add(begin, end);
}

template <typename Element>
inline typename RepeatedPtrField<Element>::iterator
RepeatedPtrField<Element>::erase(const_iterator position)
    ABSL_ATTRIBUTE_LIFETIME_BOUND {
  return erase(position, position + 1);
}

template <typename Element>
inline typename RepeatedPtrField<Element>::iterator
RepeatedPtrField<Element>::erase(const_iterator first, const_iterator last)
    ABSL_ATTRIBUTE_LIFETIME_BOUND {
  size_type pos_offset = static_cast<size_type>(std::distance(cbegin(), first));
  size_type last_offset = static_cast<size_type>(std::distance(cbegin(), last));
  DeleteSubrange(pos_offset, last_offset - pos_offset);
  return begin() + pos_offset;
}

template <typename Element>
inline Element** RepeatedPtrField<Element>::mutable_data()
    ABSL_ATTRIBUTE_LIFETIME_BOUND {
  return RepeatedPtrFieldBase::mutable_data<TypeHandler>();
}

template <typename Element>
inline const Element* const* RepeatedPtrField<Element>::data() const
    ABSL_ATTRIBUTE_LIFETIME_BOUND {
  return RepeatedPtrFieldBase::data<TypeHandler>();
}

template <typename Element>
inline void RepeatedPtrField<Element>::Swap(RepeatedPtrField* other) {
  if (this == other) return;
  RepeatedPtrFieldBase::Swap<TypeHandler>(GetArena(), other, other->GetArena());
}

template <typename Element>
inline void RepeatedPtrField<Element>::UnsafeArenaSwap(
    RepeatedPtrField* other) {
  if (this == other) return;
  ABSL_DCHECK_EQ(GetArena(), other->GetArena());
  RepeatedPtrFieldBase::InternalSwap(other);
}

template <typename Element>
inline void RepeatedPtrField<Element>::SwapElements(int index1, int index2) {
  RepeatedPtrFieldBase::SwapElements(index1, index2);
}

template <typename Element>
inline Arena* RepeatedPtrField<Element>::GetArena() {
  // Note: we make this function non-const to force callers to call the
  // `mutable_*` accessor on the repeated field before calling `GetArena()`,
  // which initializes the field if it is split. If this method were const, then
  // `msg.repeated_ptr_field().GetArena()` would be valid, but for split
  // repeated fields `repeated_ptr_field()` could point to the default split
  // instance. This would always return `nullptr`, which is incorrect when using
  // arenas.
  return RepeatedPtrFieldBase::GetArena();
}

template <typename Element>
inline size_t RepeatedPtrField<Element>::SpaceUsedExcludingSelfLong() const {
  // `google::protobuf::Message` has a virtual method `SpaceUsedLong`, hence we can
  // instantiate just one function for all protobuf messages.
  // Note: std::is_base_of requires that `Element` is a concrete class.
  using H = typename std::conditional<std::is_base_of<Message, Element>::value,
                                      internal::GenericTypeHandler<Message>,
                                      TypeHandler>::type;
  return RepeatedPtrFieldBase::SpaceUsedExcludingSelfLong<H>();
}

template <typename Element>
inline void RepeatedPtrField<Element>::AddAllocated(Element* value) {
  AddAllocatedWithArena(GetArena(), value);
}

template <typename Element>
inline void RepeatedPtrField<Element>::AddAllocatedWithArena(Arena* arena,
                                                             Element* value) {
  ABSL_DCHECK_EQ(arena, GetArena());
  RepeatedPtrFieldBase::AddAllocated<TypeHandler>(arena, value);
}

template <typename Element>
inline void RepeatedPtrField<Element>::UnsafeArenaAddAllocated(Element* value) {
  UnsafeArenaAddAllocatedWithArena(GetArena(), value);
}

template <typename Element>
inline void RepeatedPtrField<Element>::UnsafeArenaAddAllocatedWithArena(
    Arena* arena, Element* value) {
  ABSL_DCHECK_EQ(arena, GetArena());
  RepeatedPtrFieldBase::UnsafeArenaAddAllocated<TypeHandler>(arena, value);
}

template <typename Element>
inline Element* RepeatedPtrField<Element>::ReleaseLast() {
  return ReleaseLastWithArena(GetArena());
}
template <typename Element>
inline Element* RepeatedPtrField<Element>::ReleaseLastWithArena(Arena* arena) {
  ABSL_DCHECK_EQ(arena, GetArena());
  return RepeatedPtrFieldBase::ReleaseLast<TypeHandler>(arena);
}

template <typename Element>
inline Element* RepeatedPtrField<Element>::UnsafeArenaReleaseLast() {
  return RepeatedPtrFieldBase::UnsafeArenaReleaseLast<TypeHandler>();
}

template <typename Element>
inline void RepeatedPtrField<Element>::Reserve(int new_size) {
  return RepeatedPtrFieldBase::ReserveWithArena(GetArena(), new_size);
}

template <typename Element>
inline int RepeatedPtrField<Element>::Capacity() const {
  return RepeatedPtrFieldBase::Capacity();
}

// -------------------------------------------------------------------

namespace internal {

// A container that holds a RepeatedPtrField<Element> and an arena pointer. This
// is used for both directly arena-allocated RepeatedPtrField's and split
// RepeatedPtrField's. Both cases need to be able to allocate memory in case a
// user calls mutating methods on the RepeatedPtrField pointer.
template <typename Element>
using RepeatedPtrFieldWithArena = FieldWithArena<RepeatedPtrField<Element>>;

template <typename Element>
struct FieldArenaRep<RepeatedPtrField<Element>> {
  using Type = RepeatedPtrFieldWithArena<Element>;

  static inline RepeatedPtrField<Element>* Get(
      RepeatedPtrFieldWithArena<Element>* arena_rep) {
    return &arena_rep->field();
  }
};

template <typename Element>
struct FieldArenaRep<const RepeatedPtrField<Element>> {
  using Type = const RepeatedPtrFieldWithArena<Element>;

  static inline const RepeatedPtrField<Element>* Get(
      const RepeatedPtrFieldWithArena<Element>* arena_rep) {
    return &arena_rep->field();
  }
};

// This class gives the Rust implementation access to some protected methods on
// RepeatedPtrFieldBase. These methods allow us to operate solely on the
// MessageLite interface so that we do not need to generate code for each
// concrete message type.
class RustRepeatedMessageHelper {
 public:
  static RepeatedPtrFieldBase* New() { return new RepeatedPtrFieldBase; }

  static void Delete(RepeatedPtrFieldBase* field) {
    if (field->NeedsDestroy()) {
      field->DestroyProtos();
    }
    delete field;
  }

  static size_t Size(const RepeatedPtrFieldBase& field) {
    return static_cast<size_t>(field.size());
  }

  static auto Add(RepeatedPtrFieldBase& field, const MessageLite* prototype) {
    return field.AddFromPrototype<GenericTypeHandler<MessageLite>>(
        field.GetArena(), prototype);
  }

  static void CopyFrom(const RepeatedPtrFieldBase& src,
                       RepeatedPtrFieldBase& dst) {
    dst.Clear<GenericTypeHandler<google::protobuf::MessageLite>>();
    dst.MergeFrom<google::protobuf::MessageLite>(src, dst.GetArena());
  }

  static void Reserve(RepeatedPtrFieldBase& field, size_t additional) {
    field.ReserveWithArena(field.GetArena(), field.size() + additional);
  }

  static const MessageLite& At(const RepeatedPtrFieldBase& field,
                               size_t index) {
    return field.at<GenericTypeHandler<MessageLite>>(index);
  }

  static MessageLite& At(RepeatedPtrFieldBase& field, size_t index) {
    return field.at<GenericTypeHandler<MessageLite>>(index);
  }
};

// STL-like iterator implementation for RepeatedPtrField.  You should not
// refer to this class directly; use RepeatedPtrField<T>::iterator instead.
//
// The iterator for RepeatedPtrField<T>, RepeatedPtrIterator<T>, is
// very similar to iterator_ptr<T**> in util/gtl/iterator_adaptors.h,
// but adds random-access operators and is modified to wrap a void** base
// iterator (since RepeatedPtrField stores its array as a void* array and
// casting void** to T** would violate C++ aliasing rules).
//
// This code based on net/proto/proto-array-internal.h by Jeffrey Yasskin
// (jyasskin@google.com).
template <typename Element>
class ABSL_ATTRIBUTE_VIEW RepeatedPtrIterator {
 public:
  using iterator = RepeatedPtrIterator<Element>;
  using iterator_category = std::random_access_iterator_tag;
  using value_type = typename std::remove_const<Element>::type;
  using difference_type = std::ptrdiff_t;
  using pointer = Element*;
  using reference = Element&;

  RepeatedPtrIterator() : it_(nullptr) {}
  explicit RepeatedPtrIterator(void* const* it) : it_(it) {}

  // Allows "upcasting" from RepeatedPtrIterator<T**> to
  // RepeatedPtrIterator<const T*const*>.
  template <typename OtherElement,
            typename std::enable_if<std::is_convertible<
                OtherElement*, pointer>::value>::type* = nullptr>
  RepeatedPtrIterator(const RepeatedPtrIterator<OtherElement>& other)
      : it_(other.it_) {}

  // dereferenceable
  PROTOBUF_FUTURE_ADD_NODISCARD reference operator*() const {
    return *reinterpret_cast<Element*>(*it_);
  }
  PROTOBUF_FUTURE_ADD_NODISCARD pointer operator->() const {
    return &(operator*());
  }

  // {inc,dec}rementable
  iterator& operator++() {
    ++it_;
    return *this;
  }
  iterator operator++(int) { return iterator(it_++); }
  iterator& operator--() {
    --it_;
    return *this;
  }
  iterator operator--(int) { return iterator(it_--); }

  // equality_comparable
  friend bool operator==(const iterator& x, const iterator& y) {
    return x.it_ == y.it_;
  }
  friend bool operator!=(const iterator& x, const iterator& y) {
    return x.it_ != y.it_;
  }

  // less_than_comparable
  friend bool operator<(const iterator& x, const iterator& y) {
    return x.it_ < y.it_;
  }
  friend bool operator<=(const iterator& x, const iterator& y) {
    return x.it_ <= y.it_;
  }
  friend bool operator>(const iterator& x, const iterator& y) {
    return x.it_ > y.it_;
  }
  friend bool operator>=(const iterator& x, const iterator& y) {
    return x.it_ >= y.it_;
  }

  // addable, subtractable
  iterator& operator+=(difference_type d) {
    it_ += d;
    return *this;
  }
  friend iterator operator+(iterator it, const difference_type d) {
    it += d;
    return it;
  }
  friend iterator operator+(const difference_type d, iterator it) {
    it += d;
    return it;
  }
  iterator& operator-=(difference_type d) {
    it_ -= d;
    return *this;
  }
  friend iterator operator-(iterator it, difference_type d) {
    it -= d;
    return it;
  }

  // indexable
  PROTOBUF_FUTURE_ADD_NODISCARD reference operator[](difference_type d) const {
    return *(*this + d);
  }

  // random access iterator
  friend difference_type operator-(iterator it1, iterator it2) {
    return it1.it_ - it2.it_;
  }

 private:
  template <typename OtherElement>
  friend class RepeatedPtrIterator;

  // The internal iterator.
  void* const* it_;
};

template <typename Traits, typename = void>
struct IteratorConceptSupport {
  using tag = typename Traits::iterator_category;
};

template <typename Traits>
struct IteratorConceptSupport<Traits,
                              std::void_t<typename Traits::iterator_concept>> {
  using tag = typename Traits::iterator_concept;
};

// Provides an iterator that operates on pointers to the underlying objects
// rather than the objects themselves as RepeatedPtrIterator does.
// Consider using this when working with stl algorithms that change
// the array.
// The VoidPtr template parameter holds the type-agnostic pointer value
// referenced by the iterator.  It should either be "void *" for a mutable
// iterator, or "const void* const" for a constant iterator.
template <typename Element>
class RepeatedPtrOverPtrsIterator {
 private:
  using traits = std::iterator_traits<Element**>;

  using ElementPtr =
      std::conditional_t<std::is_const_v<Element>, Element* const, Element*>;
  using VoidPtr =
      std::conditional_t<std::is_const_v<Element>, const void* const, void*>;

 public:
  using value_type = typename traits::value_type;
  using difference_type = typename traits::difference_type;
  using pointer = ElementPtr*;
  using reference = ElementPtr&;
  using iterator_category = typename traits::iterator_category;
  using iterator_concept = typename IteratorConceptSupport<traits>::tag;

  using iterator = RepeatedPtrOverPtrsIterator<Element>;

  RepeatedPtrOverPtrsIterator() : it_(nullptr) {}
  explicit RepeatedPtrOverPtrsIterator(VoidPtr* it) : it_(it) {}

  // Allow "upcasting" from RepeatedPtrOverPtrsIterator<T> to
  // RepeatedPtrOverPtrsIterator<const T>.
  template <typename E = Element,
            typename = std::enable_if_t<std::is_const_v<E>>>
  RepeatedPtrOverPtrsIterator(
      const RepeatedPtrOverPtrsIterator<std::remove_const_t<Element>>& other)
      : it_(other.it_) {}

  // dereferenceable
  PROTOBUF_FUTURE_ADD_NODISCARD reference operator*() const {
    return *reinterpret_cast<pointer>(it_);
  }
  PROTOBUF_FUTURE_ADD_NODISCARD pointer operator->() const {
    return reinterpret_cast<pointer>(it_);
  }

  // {inc,dec}rementable
  iterator& operator++() {
    ++it_;
    return *this;
  }
  iterator operator++(int) { return iterator(it_++); }
  iterator& operator--() {
    --it_;
    return *this;
  }
  iterator operator--(int) { return iterator(it_--); }

  // equality_comparable
  friend bool operator==(const iterator& x, const iterator& y) {
    return x.it_ == y.it_;
  }
  friend bool operator!=(const iterator& x, const iterator& y) {
    return x.it_ != y.it_;
  }

  // less_than_comparable
  friend bool operator<(const iterator& x, const iterator& y) {
    return x.it_ < y.it_;
  }
  friend bool operator<=(const iterator& x, const iterator& y) {
    return x.it_ <= y.it_;
  }
  friend bool operator>(const iterator& x, const iterator& y) {
    return x.it_ > y.it_;
  }
  friend bool operator>=(const iterator& x, const iterator& y) {
    return x.it_ >= y.it_;
  }

  // addable, subtractable
  iterator& operator+=(difference_type d) {
    it_ += d;
    return *this;
  }
  friend iterator operator+(iterator it, difference_type d) {
    it += d;
    return it;
  }
  friend iterator operator+(difference_type d, iterator it) {
    it += d;
    return it;
  }
  iterator& operator-=(difference_type d) {
    it_ -= d;
    return *this;
  }
  friend iterator operator-(iterator it, difference_type d) {
    it -= d;
    return it;
  }

  // indexable
  PROTOBUF_FUTURE_ADD_NODISCARD reference operator[](difference_type d) const {
    return *(*this + d);
  }

  // random access iterator
  friend difference_type operator-(iterator it1, iterator it2) {
    return it1.it_ - it2.it_;
  }

 private:
  template <typename OtherElement>
  friend class RepeatedPtrOverPtrsIterator;

  // The internal iterator.
  VoidPtr* it_;
};

}  // namespace internal

template <typename Element>
inline typename RepeatedPtrField<Element>::iterator
RepeatedPtrField<Element>::begin() ABSL_ATTRIBUTE_LIFETIME_BOUND {
  return iterator(raw_data());
}
template <typename Element>
inline typename RepeatedPtrField<Element>::const_iterator
RepeatedPtrField<Element>::begin() const ABSL_ATTRIBUTE_LIFETIME_BOUND {
  return iterator(raw_data());
}
template <typename Element>
inline typename RepeatedPtrField<Element>::const_iterator
RepeatedPtrField<Element>::cbegin() const ABSL_ATTRIBUTE_LIFETIME_BOUND {
  return begin();
}
template <typename Element>
inline typename RepeatedPtrField<Element>::iterator
RepeatedPtrField<Element>::end() ABSL_ATTRIBUTE_LIFETIME_BOUND {
  return iterator(raw_data() + size());
}
template <typename Element>
inline typename RepeatedPtrField<Element>::const_iterator
RepeatedPtrField<Element>::end() const ABSL_ATTRIBUTE_LIFETIME_BOUND {
  return iterator(raw_data() + size());
}
template <typename Element>
inline typename RepeatedPtrField<Element>::const_iterator
RepeatedPtrField<Element>::cend() const ABSL_ATTRIBUTE_LIFETIME_BOUND {
  return end();
}

template <typename Element>
inline typename RepeatedPtrField<Element>::pointer_iterator
RepeatedPtrField<Element>::pointer_begin() ABSL_ATTRIBUTE_LIFETIME_BOUND {
  return pointer_iterator(raw_mutable_data());
}
template <typename Element>
inline typename RepeatedPtrField<Element>::const_pointer_iterator
RepeatedPtrField<Element>::pointer_begin() const ABSL_ATTRIBUTE_LIFETIME_BOUND {
  return const_pointer_iterator(const_cast<const void* const*>(raw_data()));
}
template <typename Element>
inline typename RepeatedPtrField<Element>::pointer_iterator
RepeatedPtrField<Element>::pointer_end() ABSL_ATTRIBUTE_LIFETIME_BOUND {
  return pointer_iterator(raw_mutable_data() + size());
}
template <typename Element>
inline typename RepeatedPtrField<Element>::const_pointer_iterator
RepeatedPtrField<Element>::pointer_end() const ABSL_ATTRIBUTE_LIFETIME_BOUND {
  return const_pointer_iterator(
      const_cast<const void* const*>(raw_data() + size()));
}

// Like C++20's std::erase_if, for RepeatedPtrField
// For string containers, the predicate is called with an `absl::string_view`.
// Otherwise, it is called with a `const T&`.
template <typename T, typename Pred>
size_t erase_if(RepeatedPtrField<T>& cont, Pred pred) {
  // We use `partition` instead of `remove` to keep all the erased elements at
  // the end for cleanup.
  auto it = std::stable_partition(
      cont.pointer_begin(), cont.pointer_end(), [&](const auto* elem) {
        return !pred(internal::GenericTypeHandler<T>::ForEraseIf(elem));
      });
  const size_t removed = cont.pointer_end() - it;
  cont.DeleteSubrange(it - cont.pointer_begin(), removed);
  return removed;
}

// Like C++20's std::erase, for RepeatedPtrField
template <typename T, typename U>
size_t erase(RepeatedPtrField<T>& cont, const U& value) {
  static_assert(!std::is_base_of_v<Message, T>, "Not supported. Use erase_if.");
  return google::protobuf::erase_if(cont,
                          [&](const auto& elem) { return elem == value; });
}

// Iterators and helper functions that follow the spirit of the STL
// std::back_insert_iterator and std::back_inserter but are tailor-made
// for RepeatedField and RepeatedPtrField. Typical usage would be:
//
//   std::copy(some_sequence.begin(), some_sequence.end(),
//             RepeatedFieldBackInserter(proto.mutable_sequence()));
//
// Ported by johannes from util/gtl/proto-array-iterators.h

namespace internal {

// A back inserter for RepeatedPtrField objects.
template <typename T>
class RepeatedPtrFieldBackInsertIterator {
 public:
  using iterator_category = std::output_iterator_tag;
  using value_type = T;
  using pointer = void;
  using reference = void;
  using difference_type = std::ptrdiff_t;

  RepeatedPtrFieldBackInsertIterator(RepeatedPtrField<T>* const mutable_field)
      : field_(mutable_field), arena_(mutable_field->GetArena()) {}
  RepeatedPtrFieldBackInsertIterator<T>& operator=(const T& value) {
    *field_->AddWithArena(arena_) = value;
    return *this;
  }
  RepeatedPtrFieldBackInsertIterator<T>& operator=(
      const T* const ptr_to_value) {
    *field_->AddWithArena(arena_) = *ptr_to_value;
    return *this;
  }
  RepeatedPtrFieldBackInsertIterator<T>& operator=(T&& value) {
    *field_->AddWithArena(arena_) = std::move(value);
    return *this;
  }
  RepeatedPtrFieldBackInsertIterator<T>& operator*() { return *this; }
  RepeatedPtrFieldBackInsertIterator<T>& operator++() { return *this; }
  RepeatedPtrFieldBackInsertIterator<T>& operator++(int /* unused */) {
    return *this;
  }

 private:
  RepeatedPtrField<T>* field_;
  Arena* arena_;
};

// A back inserter for RepeatedPtrFields that inserts by transferring ownership
// of a pointer.
template <typename T>
class AllocatedRepeatedPtrFieldBackInsertIterator {
 public:
  using iterator_category = std::output_iterator_tag;
  using value_type = T;
  using pointer = void;
  using reference = void;
  using difference_type = std::ptrdiff_t;

  explicit AllocatedRepeatedPtrFieldBackInsertIterator(
      RepeatedPtrField<T>* const mutable_field)
      : field_(mutable_field), arena_(mutable_field->GetArena()) {}
  AllocatedRepeatedPtrFieldBackInsertIterator<T>& operator=(
      T* const ptr_to_value) {
    // Directly call AddAllocated with the cached arena pointer. This avoids
    // the cost of calling `GetArena()` on `RepeatedPtrField`, which is
    // expensive.
    field_->RepeatedPtrFieldBase::template AddAllocated<
        typename RepeatedPtrField<T>::TypeHandler>(arena_, ptr_to_value);
    return *this;
  }
  AllocatedRepeatedPtrFieldBackInsertIterator<T>& operator*() { return *this; }
  AllocatedRepeatedPtrFieldBackInsertIterator<T>& operator++() { return *this; }
  AllocatedRepeatedPtrFieldBackInsertIterator<T>& operator++(int /* unused */) {
    return *this;
  }

 private:
  RepeatedPtrField<T>* field_;
  Arena* arena_;
};

// Almost identical to AllocatedRepeatedPtrFieldBackInsertIterator. This one
// uses the UnsafeArenaAddAllocated instead.
template <typename T>
class UnsafeArenaAllocatedRepeatedPtrFieldBackInsertIterator {
 public:
  using iterator_category = std::output_iterator_tag;
  using value_type = T;
  using pointer = void;
  using reference = void;
  using difference_type = std::ptrdiff_t;

  explicit UnsafeArenaAllocatedRepeatedPtrFieldBackInsertIterator(
      RepeatedPtrField<T>* const mutable_field)
      : field_(mutable_field) {}
  UnsafeArenaAllocatedRepeatedPtrFieldBackInsertIterator<T>& operator=(
      T const* const ptr_to_value) {
    field_->UnsafeArenaAddAllocated(const_cast<T*>(ptr_to_value));
    return *this;
  }
  UnsafeArenaAllocatedRepeatedPtrFieldBackInsertIterator<T>& operator*() {
    return *this;
  }
  UnsafeArenaAllocatedRepeatedPtrFieldBackInsertIterator<T>& operator++() {
    return *this;
  }
  UnsafeArenaAllocatedRepeatedPtrFieldBackInsertIterator<T>& operator++(
      int /* unused */) {
    return *this;
  }

 private:
  RepeatedPtrField<T>* field_;
};
}  // namespace internal

// Provides a back insert iterator for RepeatedPtrField instances,
// similar to std::back_inserter().
template <typename T>
internal::RepeatedPtrFieldBackInsertIterator<T> RepeatedPtrFieldBackInserter(
    RepeatedPtrField<T>* const mutable_field) {
  return internal::RepeatedPtrFieldBackInsertIterator<T>(mutable_field);
}

// Special back insert iterator for RepeatedPtrField instances, just in
// case someone wants to write generic template code that can access both
// RepeatedFields and RepeatedPtrFields using a common name.
template <typename T>
internal::RepeatedPtrFieldBackInsertIterator<T> RepeatedFieldBackInserter(
    RepeatedPtrField<T>* const mutable_field) {
  return internal::RepeatedPtrFieldBackInsertIterator<T>(mutable_field);
}

// Provides a back insert iterator for RepeatedPtrField instances
// similar to std::back_inserter() which transfers the ownership while
// copying elements.
template <typename T>
internal::AllocatedRepeatedPtrFieldBackInsertIterator<T>
AllocatedRepeatedPtrFieldBackInserter(
    RepeatedPtrField<T>* const mutable_field) {
  return internal::AllocatedRepeatedPtrFieldBackInsertIterator<T>(
      mutable_field);
}

// Similar to AllocatedRepeatedPtrFieldBackInserter, using
// UnsafeArenaAddAllocated instead of AddAllocated.
// This is slightly faster if that matters. It is also useful in legacy code
// that uses temporary ownership to avoid copies. Example:
//   RepeatedPtrField<T> temp_field;
//   temp_field.UnsafeArenaAddAllocated(new T);
//   ... // Do something with temp_field
//   temp_field.UnsafeArenaExtractSubrange(0, temp_field.size(), nullptr);
// Putting temp_field on the arena fails because the ownership transfers to the
// arena at the "AddAllocated" call and is not released anymore causing a
// double delete. This function uses UnsafeArenaAddAllocated to prevent this.
template <typename T>
internal::UnsafeArenaAllocatedRepeatedPtrFieldBackInsertIterator<T>
UnsafeArenaAllocatedRepeatedPtrFieldBackInserter(
    RepeatedPtrField<T>* const mutable_field) {
  return internal::UnsafeArenaAllocatedRepeatedPtrFieldBackInsertIterator<T>(
      mutable_field);
}


namespace internal {
// Size optimization for `memswap<N>` - supplied below N is used by every
// `RepeatedPtrField<T>`.
extern template PROTOBUF_EXPORT_TEMPLATE_DECLARE void
memswap<InternalMetadataResolverOffsetHelper<RepeatedPtrFieldBase>::value>(
    char* PROTOBUF_RESTRICT, char* PROTOBUF_RESTRICT);
}  // namespace internal

}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"

#endif  // GOOGLE_PROTOBUF_REPEATED_PTR_FIELD_H__
