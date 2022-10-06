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
#include <iterator>
#include <limits>
#include <string>
#include <type_traits>
#include <utility>

#include "google/protobuf/stubs/logging.h"
#include "google/protobuf/stubs/common.h"
#include "google/protobuf/arena.h"
#include "google/protobuf/port.h"
#include "google/protobuf/message_lite.h"
#include "google/protobuf/port.h"
#include "google/protobuf/repeated_ptr_field.h"


// Must be included last.
#include "google/protobuf/port_def.inc"

#ifdef SWIG
#error "You cannot SWIG proto headers"
#endif

namespace google {
namespace protobuf {

class Message;

namespace internal {

template <typename T, int kRepHeaderSize>
constexpr int RepeatedFieldLowerClampLimit() {
  // The header is padded to be at least `sizeof(T)` when it would be smaller
  // otherwise.
  static_assert(sizeof(T) <= kRepHeaderSize, "");
  // We want to pad the minimum size to be a power of two bytes, including the
  // header.
  // The first allocation is kRepHeaderSize bytes worth of elements for a total
  // of 2*kRepHeaderSize bytes.
  // For an 8-byte header, we allocate 8 bool, 2 ints, or 1 int64.
  return kRepHeaderSize / sizeof(T);
}

// kRepeatedFieldUpperClampLimit is the lowest signed integer value that
// overflows when multiplied by 2 (which is undefined behavior). Sizes above
// this will clamp to the maximum int value instead of following exponential
// growth when growing a repeated field.
constexpr int kRepeatedFieldUpperClampLimit =
    (std::numeric_limits<int>::max() / 2) + 1;

// Swaps two blocks of memory of size kSize:
template <size_t kSize>
void memswap(char* a, char* b) {
#if __SIZEOF_INT128__
  using Buffer = __uint128_t;
#else
  using Buffer = uint64_t;
#endif

  constexpr size_t kBlockSize = sizeof(Buffer);
  Buffer buf;
  for (size_t i = 0; i < kSize / kBlockSize; ++i) {
    memcpy(&buf, a, kBlockSize);
    memcpy(a, b, kBlockSize);
    memcpy(b, &buf, kBlockSize);
    a += kBlockSize;
    b += kBlockSize;
  }

#if defined(__GNUC__) && !defined(__clang__)
  // Workaround GCC bug: https://gcc.gnu.org/bugzilla/show_bug.cgi?id=99578
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpragmas"
#pragma GCC diagnostic ignored "-Wstringop-overflow"
#endif  // __GNUC__

  // Swap the leftover bytes, could be zero.
  memcpy(&buf, a, kSize % kBlockSize);
  memcpy(a, b, kSize % kBlockSize);
  memcpy(b, &buf, kSize % kBlockSize);

#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic pop
#endif  // GCC
}

template <typename Element>
class RepeatedIterator;

}  // namespace internal

// RepeatedField is used to represent repeated fields of a primitive type (in
// other words, everything except strings and nested Messages).  Most users will
// not ever use a RepeatedField directly; they will use the get-by-index,
// set-by-index, and add accessors that are generated for all repeated fields.
template <typename Element>
class RepeatedField final {
  static_assert(
      alignof(Arena) >= alignof(Element),
      "We only support types that have an alignment smaller than Arena");

 public:
  constexpr RepeatedField();
  explicit RepeatedField(Arena* arena);

  RepeatedField(const RepeatedField& other);

  template <typename Iter,
            typename = typename std::enable_if<std::is_constructible<
                Element, decltype(*std::declval<Iter>())>::value>::type>
  RepeatedField(Iter begin, Iter end);

  ~RepeatedField();

  RepeatedField& operator=(const RepeatedField& other);

  RepeatedField(RepeatedField&& other) noexcept;
  RepeatedField& operator=(RepeatedField&& other) noexcept;

  bool empty() const;
  int size() const;

  const Element& Get(int index) const;
  Element* Mutable(int index);

  const Element& operator[](int index) const { return Get(index); }
  Element& operator[](int index) { return *Mutable(index); }

  const Element& at(int index) const;
  Element& at(int index);

  void Set(int index, const Element& value);
  void Add(const Element& value);
  // Appends a new element and returns a pointer to it.
  // The new element is uninitialized if |Element| is a POD type.
  Element* Add();
  // Appends elements in the range [begin, end) after reserving
  // the appropriate number of elements.
  template <typename Iter>
  void Add(Iter begin, Iter end);

  // Removes the last element in the array.
  void RemoveLast();

  // Extracts elements with indices in "[start .. start+num-1]".
  // Copies them into "elements[0 .. num-1]" if "elements" is not nullptr.
  // Caution: also moves elements with indices [start+num ..].
  // Calling this routine inside a loop can cause quadratic behavior.
  void ExtractSubrange(int start, int num, Element* elements);

  PROTOBUF_ATTRIBUTE_REINITIALIZES void Clear();
  void MergeFrom(const RepeatedField& other);
  PROTOBUF_ATTRIBUTE_REINITIALIZES void CopyFrom(const RepeatedField& other);

  // Replaces the contents with RepeatedField(begin, end).
  template <typename Iter>
  PROTOBUF_ATTRIBUTE_REINITIALIZES void Assign(Iter begin, Iter end);

  // Reserves space to expand the field to at least the given size.  If the
  // array is grown, it will always be at least doubled in size.
  void Reserve(int new_size);

  // Resizes the RepeatedField to a new, smaller size.  This is O(1).
  void Truncate(int new_size);

  void AddAlreadyReserved(const Element& value);
  // Appends a new element and return a pointer to it.
  // The new element is uninitialized if |Element| is a POD type.
  // Should be called only if Capacity() > Size().
  Element* AddAlreadyReserved();
  Element* AddNAlreadyReserved(int elements);
  int Capacity() const;

  // Like STL resize.  Uses value to fill appended elements.
  // Like Truncate() if new_size <= size(), otherwise this is
  // O(new_size - size()).
  void Resize(int new_size, const Element& value);

  // Gets the underlying array.  This pointer is possibly invalidated by
  // any add or remove operation.
  Element* mutable_data();
  const Element* data() const;

  // Swaps entire contents with "other". If they are separate arenas, then
  // copies data between each other.
  void Swap(RepeatedField* other);

  // Swaps two elements.
  void SwapElements(int index1, int index2);

  // STL-like iterator support
  typedef internal::RepeatedIterator<Element> iterator;
  typedef internal::RepeatedIterator<const Element> const_iterator;
  typedef Element value_type;
  typedef value_type& reference;
  typedef const value_type& const_reference;
  typedef value_type* pointer;
  typedef const value_type* const_pointer;
  typedef int size_type;
  typedef ptrdiff_t difference_type;

  iterator begin();
  const_iterator begin() const;
  const_iterator cbegin() const;
  iterator end();
  const_iterator end() const;
  const_iterator cend() const;

  // Reverse iterator support
  typedef std::reverse_iterator<const_iterator> const_reverse_iterator;
  typedef std::reverse_iterator<iterator> reverse_iterator;
  reverse_iterator rbegin() { return reverse_iterator(end()); }
  const_reverse_iterator rbegin() const {
    return const_reverse_iterator(end());
  }
  reverse_iterator rend() { return reverse_iterator(begin()); }
  const_reverse_iterator rend() const {
    return const_reverse_iterator(begin());
  }

  // Returns the number of bytes used by the repeated field, excluding
  // sizeof(*this)
  size_t SpaceUsedExcludingSelfLong() const;

  int SpaceUsedExcludingSelf() const {
    return internal::ToIntSize(SpaceUsedExcludingSelfLong());
  }

  // Removes the element referenced by position.
  //
  // Returns an iterator to the element immediately following the removed
  // element.
  //
  // Invalidates all iterators at or after the removed element, including end().
  iterator erase(const_iterator position);

  // Removes the elements in the range [first, last).
  //
  // Returns an iterator to the element immediately following the removed range.
  //
  // Invalidates all iterators at or after the removed range, including end().
  iterator erase(const_iterator first, const_iterator last);

  // Gets the Arena on which this RepeatedField stores its elements.
  // Message-owned arenas are not exposed by this method, which will return
  // nullptr for messages owned by MOAs.
  inline Arena* GetArena() const {
    Arena* arena = GetOwningArena();
    if (arena == nullptr || arena->InternalIsMessageOwnedArena()) {
      return nullptr;
    }
    return arena;
  }

  // For internal use only.
  //
  // This is public due to it being called by generated code.
  inline void InternalSwap(RepeatedField* other);


 private:
  template <typename T> friend class Arena::InternalHelper;

  // Gets the Arena on which this RepeatedField stores its elements.
  inline Arena* GetOwningArena() const {
    return (total_size_ == 0) ? static_cast<Arena*>(arena_or_elements_)
                              : rep()->arena;
  }

  // Swaps entire contents with "other". Should be called only if the caller can
  // guarantee that both repeated fields are on the same arena or are on the
  // heap. Swapping between different arenas is disallowed and caught by a
  // GOOGLE_DCHECK (see API docs for details).
  void UnsafeArenaSwap(RepeatedField* other);

  static constexpr int kInitialSize = 0;
  // A note on the representation here (see also comment below for
  // RepeatedPtrFieldBase's struct Rep):
  //
  // We maintain the same sizeof(RepeatedField) as before we added arena support
  // so that we do not degrade performance by bloating memory usage. Directly
  // adding an arena_ element to RepeatedField is quite costly. By using
  // indirection in this way, we keep the same size when the RepeatedField is
  // empty (common case), and add only an 8-byte header to the elements array
  // when non-empty. We make sure to place the size fields directly in the
  // RepeatedField class to avoid costly cache misses due to the indirection.
  int current_size_;
  int total_size_;

  // Replaces current_size_ with new_size and returns the previous value of
  // current_size_. This function is intended to be the only place where
  // current_size_ is modified.
  inline int ExchangeCurrentSize(int new_size) {
    const int prev_size = current_size_;
    current_size_ = new_size;
    return prev_size;
  }

  // Pad the Rep after arena allow for power-of-two byte sizes when
  // sizeof(Element) > sizeof(Arena*). eg for 16-byte objects.
  static PROTOBUF_CONSTEXPR const size_t kRepHeaderSize =
      sizeof(Arena*) < sizeof(Element) ? sizeof(Element) : sizeof(Arena*);
  struct Rep {
    Arena* arena;
    Element* elements() {
      return reinterpret_cast<Element*>(reinterpret_cast<char*>(this) +
                                        kRepHeaderSize);
    }
  };


  // If total_size_ == 0 this points to an Arena otherwise it points to the
  // elements member of a Rep struct. Using this invariant allows the storage of
  // the arena pointer without an extra allocation in the constructor.
  void* arena_or_elements_;

  // Returns a pointer to elements array.
  // pre-condition: the array must have been allocated.
  Element* elements() const {
    GOOGLE_DCHECK_GT(total_size_, 0);
    // Because of above pre-condition this cast is safe.
    return unsafe_elements();
  }

  // Returns a pointer to elements array if it exists; otherwise either null or
  // an invalid pointer is returned. This only happens for empty repeated
  // fields, where you can't dereference this pointer anyway (it's empty).
  Element* unsafe_elements() const {
    return static_cast<Element*>(arena_or_elements_);
  }

  // Returns a pointer to the Rep struct.
  // pre-condition: the Rep must have been allocated, ie elements() is safe.
  Rep* rep() const {
    return reinterpret_cast<Rep*>(reinterpret_cast<char*>(elements()) -
                                  kRepHeaderSize);
  }

  friend class Arena;
  typedef void InternalArenaConstructable_;

  // Moves the contents of |from| into |to|, possibly clobbering |from| in the
  // process.  For primitive types this is just a memcpy(), but it could be
  // specialized for non-primitive types to, say, swap each element instead.
  void MoveArray(Element* to, Element* from, int size);

  // Copies the elements of |from| into |to|.
  void CopyArray(Element* to, const Element* from, int size);

  // Internal helper to delete all elements and deallocate the storage.
  void InternalDeallocate(Rep* rep, int size, bool in_destructor) {
    if (rep != nullptr) {
      Element* e = &rep->elements()[0];
      if (!std::is_trivial<Element>::value) {
        Element* limit = &rep->elements()[size];
        for (; e < limit; e++) {
          e->~Element();
        }
      }
      const size_t bytes = size * sizeof(*e) + kRepHeaderSize;
      if (rep->arena == nullptr) {
        internal::SizedDelete(rep, bytes);
      } else if (!in_destructor) {
        // If we are in the destructor, we might be being destroyed as part of
        // the arena teardown. We can't try and return blocks to the arena then.
        rep->arena->ReturnArrayMemory(rep, bytes);
      }
    }
  }

  // This class is a performance wrapper around RepeatedField::Add(const T&)
  // function. In general unless a RepeatedField is a local stack variable LLVM
  // has a hard time optimizing Add. The machine code tends to be
  // loop:
  // mov %size, dword ptr [%repeated_field]       // load
  // cmp %size, dword ptr [%repeated_field + 4]
  // jae fallback
  // mov %buffer, qword ptr [%repeated_field + 8]
  // mov dword [%buffer + %size * 4], %value
  // inc %size                                    // increment
  // mov dword ptr [%repeated_field], %size       // store
  // jmp loop
  //
  // This puts a load/store in each iteration of the important loop variable
  // size. It's a pretty bad compile that happens even in simple cases, but
  // largely the presence of the fallback path disturbs the compilers mem-to-reg
  // analysis.
  //
  // This class takes ownership of a repeated field for the duration of its
  // lifetime. The repeated field should not be accessed during this time, ie.
  // only access through this class is allowed. This class should always be a
  // function local stack variable. Intended use
  //
  // void AddSequence(const int* begin, const int* end, RepeatedField<int>* out)
  // {
  //   RepeatedFieldAdder<int> adder(out);  // Take ownership of out
  //   for (auto it = begin; it != end; ++it) {
  //     adder.Add(*it);
  //   }
  // }
  //
  // Typically, due to the fact that adder is a local stack variable, the
  // compiler will be successful in mem-to-reg transformation and the machine
  // code will be
  // loop:
  // cmp %size, %capacity
  // jae fallback
  // mov dword ptr [%buffer + %size * 4], %val
  // inc %size
  // jmp loop
  //
  // The first version executes at 7 cycles per iteration while the second
  // version executes at only 1 or 2 cycles.
  template <int = 0, bool = std::is_trivial<Element>::value>
  class FastAdderImpl {
   public:
    explicit FastAdderImpl(RepeatedField* rf) : repeated_field_(rf) {
      index_ = repeated_field_->current_size_;
      capacity_ = repeated_field_->total_size_;
      buffer_ = repeated_field_->unsafe_elements();
    }
    FastAdderImpl(const FastAdderImpl&) = delete;
    FastAdderImpl& operator=(const FastAdderImpl&) = delete;
    ~FastAdderImpl() {
      repeated_field_->current_size_ = index_;
    }

    void Add(Element val) {
      if (index_ == capacity_) {
        repeated_field_->current_size_ = index_;
        repeated_field_->Reserve(index_ + 1);
        capacity_ = repeated_field_->total_size_;
        buffer_ = repeated_field_->unsafe_elements();
      }
      buffer_[index_++] = val;
    }

   private:
    RepeatedField* repeated_field_;
    int index_;
    int capacity_;
    Element* buffer_;
  };

  // FastAdder is a wrapper for adding fields. The specialization above handles
  // POD types more efficiently than RepeatedField.
  template <int I>
  class FastAdderImpl<I, false> {
   public:
    explicit FastAdderImpl(RepeatedField* rf) : repeated_field_(rf) {}
    FastAdderImpl(const FastAdderImpl&) = delete;
    FastAdderImpl& operator=(const FastAdderImpl&) = delete;
    void Add(const Element& val) { repeated_field_->Add(val); }

   private:
    RepeatedField* repeated_field_;
  };

  using FastAdder = FastAdderImpl<>;

  friend class TestRepeatedFieldHelper;
  friend class ::google::protobuf::internal::ParseContext;
};

namespace internal {

// This is a helper template to copy an array of elements efficiently when they
// have a trivial copy constructor, and correctly otherwise. This really
// shouldn't be necessary, but our compiler doesn't optimize std::copy very
// effectively.
template <typename Element,
          bool HasTrivialCopy = std::is_trivial<Element>::value>
struct ElementCopier {
  void operator()(Element* to, const Element* from, int array_size);
};

}  // namespace internal

// implementation ====================================================

template <typename Element>
constexpr RepeatedField<Element>::RepeatedField()
    : current_size_(0), total_size_(0), arena_or_elements_(nullptr) {}

template <typename Element>
inline RepeatedField<Element>::RepeatedField(Arena* arena)
    : current_size_(0), total_size_(0), arena_or_elements_(arena) {}

template <typename Element>
inline RepeatedField<Element>::RepeatedField(const RepeatedField& other)
    : current_size_(0), total_size_(0), arena_or_elements_(nullptr) {
  if (other.current_size_ != 0) {
    Reserve(other.size());
    AddNAlreadyReserved(other.size());
    CopyArray(Mutable(0), &other.Get(0), other.size());
  }
}

template <typename Element>
template <typename Iter, typename>
RepeatedField<Element>::RepeatedField(Iter begin, Iter end)
    : current_size_(0), total_size_(0), arena_or_elements_(nullptr) {
  Add(begin, end);
}

template <typename Element>
RepeatedField<Element>::~RepeatedField() {
#ifndef NDEBUG
  // Try to trigger segfault / asan failure in non-opt builds if arena_
  // lifetime has ended before the destructor.
  auto arena = GetOwningArena();
  if (arena) (void)arena->SpaceAllocated();
#endif
  if (total_size_ > 0) {
    InternalDeallocate(rep(), total_size_, true);
  }
}

template <typename Element>
inline RepeatedField<Element>& RepeatedField<Element>::operator=(
    const RepeatedField& other) {
  if (this != &other) CopyFrom(other);
  return *this;
}

template <typename Element>
inline RepeatedField<Element>::RepeatedField(RepeatedField&& other) noexcept
    : RepeatedField() {
#ifdef PROTOBUF_FORCE_COPY_IN_MOVE
  CopyFrom(other);
#else   // PROTOBUF_FORCE_COPY_IN_MOVE
  // We don't just call Swap(&other) here because it would perform 3 copies if
  // other is on an arena. This field can't be on an arena because arena
  // construction always uses the Arena* accepting constructor.
  if (other.GetOwningArena()) {
    CopyFrom(other);
  } else {
    InternalSwap(&other);
  }
#endif  // !PROTOBUF_FORCE_COPY_IN_MOVE
}

template <typename Element>
inline RepeatedField<Element>& RepeatedField<Element>::operator=(
    RepeatedField&& other) noexcept {
  // We don't just call Swap(&other) here because it would perform 3 copies if
  // the two fields are on different arenas.
  if (this != &other) {
    if (GetOwningArena() != other.GetOwningArena()
#ifdef PROTOBUF_FORCE_COPY_IN_MOVE
        || GetOwningArena() == nullptr
#endif  // !PROTOBUF_FORCE_COPY_IN_MOVE
    ) {
      CopyFrom(other);
    } else {
      InternalSwap(&other);
    }
  }
  return *this;
}

template <typename Element>
inline bool RepeatedField<Element>::empty() const {
  return current_size_ == 0;
}

template <typename Element>
inline int RepeatedField<Element>::size() const {
  return current_size_;
}

template <typename Element>
inline int RepeatedField<Element>::Capacity() const {
  return total_size_;
}

template <typename Element>
inline void RepeatedField<Element>::AddAlreadyReserved(const Element& value) {
  GOOGLE_DCHECK_LT(current_size_, total_size_);
  elements()[ExchangeCurrentSize(current_size_ + 1)] = value;
}

template <typename Element>
inline Element* RepeatedField<Element>::AddAlreadyReserved() {
  GOOGLE_DCHECK_LT(current_size_, total_size_);
  return &elements()[ExchangeCurrentSize(current_size_ + 1)];
}

template <typename Element>
inline Element* RepeatedField<Element>::AddNAlreadyReserved(int elements) {
  GOOGLE_DCHECK_GE(total_size_ - current_size_, elements)
      << total_size_ << ", " << current_size_;
  // Warning: sometimes people call this when elements == 0 and
  // total_size_ == 0. In this case the return pointer points to a zero size
  // array (n == 0). Hence we can just use unsafe_elements(), because the user
  // cannot dereference the pointer anyway.
  return unsafe_elements() + ExchangeCurrentSize(current_size_ + elements);
}

template <typename Element>
inline void RepeatedField<Element>::Resize(int new_size, const Element& value) {
  GOOGLE_DCHECK_GE(new_size, 0);
  if (new_size > current_size_) {
    Reserve(new_size);
    std::fill(&elements()[ExchangeCurrentSize(new_size)], &elements()[new_size],
              value);
  } else {
    ExchangeCurrentSize(new_size);
  }
}

template <typename Element>
inline const Element& RepeatedField<Element>::Get(int index) const {
  GOOGLE_DCHECK_GE(index, 0);
  GOOGLE_DCHECK_LT(index, current_size_);
  return elements()[index];
}

template <typename Element>
inline const Element& RepeatedField<Element>::at(int index) const {
  GOOGLE_CHECK_GE(index, 0);
  GOOGLE_CHECK_LT(index, current_size_);
  return elements()[index];
}

template <typename Element>
inline Element& RepeatedField<Element>::at(int index) {
  GOOGLE_CHECK_GE(index, 0);
  GOOGLE_CHECK_LT(index, current_size_);
  return elements()[index];
}

template <typename Element>
inline Element* RepeatedField<Element>::Mutable(int index) {
  GOOGLE_DCHECK_GE(index, 0);
  GOOGLE_DCHECK_LT(index, current_size_);
  return &elements()[index];
}

template <typename Element>
inline void RepeatedField<Element>::Set(int index, const Element& value) {
  GOOGLE_DCHECK_GE(index, 0);
  GOOGLE_DCHECK_LT(index, current_size_);
  elements()[index] = value;
}

template <typename Element>
inline void RepeatedField<Element>::Add(const Element& value) {
  if (current_size_ == total_size_) {
    // value could reference an element of the array. Reserving new space will
    // invalidate the reference. So we must make a copy first.
    auto tmp = value;
    Reserve(total_size_ + 1);
    elements()[ExchangeCurrentSize(current_size_ + 1)] = std::move(tmp);
  } else {
    elements()[ExchangeCurrentSize(current_size_ + 1)] = value;
  }
}

template <typename Element>
inline Element* RepeatedField<Element>::Add() {
  if (current_size_ == total_size_) Reserve(total_size_ + 1);
  return &elements()[ExchangeCurrentSize(current_size_ + 1)];
}

template <typename Element>
template <typename Iter>
inline void RepeatedField<Element>::Add(Iter begin, Iter end) {
  if (std::is_base_of<
          std::forward_iterator_tag,
          typename std::iterator_traits<Iter>::iterator_category>::value) {
    int additional = static_cast<int>(std::distance(begin, end));
    if (additional == 0) return;

    int new_size = current_size_ + additional;
    Reserve(new_size);
    // TODO(ckennelly):  The compiler loses track of the buffer freshly
    // allocated by Reserve() by the time we call elements, so it cannot
    // guarantee that elements does not alias [begin(), end()).
    //
    // If restrict is available, annotating the pointer obtained from elements()
    // causes this to lower to memcpy instead of memmove.
    std::copy(begin, end, elements() + ExchangeCurrentSize(new_size));
  } else {
    FastAdder fast_adder(this);
    for (; begin != end; ++begin) fast_adder.Add(*begin);
  }
}

template <typename Element>
inline void RepeatedField<Element>::RemoveLast() {
  GOOGLE_DCHECK_GT(current_size_, 0);
  ExchangeCurrentSize(current_size_ - 1);
}

template <typename Element>
void RepeatedField<Element>::ExtractSubrange(int start, int num,
                                             Element* elements) {
  GOOGLE_DCHECK_GE(start, 0);
  GOOGLE_DCHECK_GE(num, 0);
  GOOGLE_DCHECK_LE(start + num, this->current_size_);

  // Save the values of the removed elements if requested.
  if (elements != nullptr) {
    for (int i = 0; i < num; ++i) elements[i] = this->Get(i + start);
  }

  // Slide remaining elements down to fill the gap.
  if (num > 0) {
    for (int i = start + num; i < this->current_size_; ++i)
      this->Set(i - num, this->Get(i));
    this->Truncate(this->current_size_ - num);
  }
}

template <typename Element>
inline void RepeatedField<Element>::Clear() {
  ExchangeCurrentSize(0);
}

template <typename Element>
inline void RepeatedField<Element>::MergeFrom(const RepeatedField& other) {
  GOOGLE_DCHECK_NE(&other, this);
  if (other.current_size_ != 0) {
    int existing_size = size();
    Reserve(existing_size + other.size());
    AddNAlreadyReserved(other.size());
    CopyArray(Mutable(existing_size), &other.Get(0), other.size());
  }
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
    const_iterator position) {
  return erase(position, position + 1);
}

template <typename Element>
inline typename RepeatedField<Element>::iterator RepeatedField<Element>::erase(
    const_iterator first, const_iterator last) {
  size_type first_offset = first - cbegin();
  if (first != last) {
    Truncate(std::copy(last, cend(), begin() + first_offset) - cbegin());
  }
  return begin() + first_offset;
}

template <typename Element>
inline Element* RepeatedField<Element>::mutable_data() {
  return unsafe_elements();
}

template <typename Element>
inline const Element* RepeatedField<Element>::data() const {
  return unsafe_elements();
}

template <typename Element>
inline void RepeatedField<Element>::InternalSwap(RepeatedField* other) {
  GOOGLE_DCHECK(this != other);

  // Swap all fields at once.
  static_assert(std::is_standard_layout<RepeatedField<Element>>::value,
                "offsetof() requires standard layout before c++17");
  internal::memswap<offsetof(RepeatedField, arena_or_elements_) +
                    sizeof(this->arena_or_elements_) -
                    offsetof(RepeatedField, current_size_)>(
      reinterpret_cast<char*>(this) + offsetof(RepeatedField, current_size_),
      reinterpret_cast<char*>(other) + offsetof(RepeatedField, current_size_));
}

template <typename Element>
void RepeatedField<Element>::Swap(RepeatedField* other) {
  if (this == other) return;
#ifdef PROTOBUF_FORCE_COPY_IN_SWAP
  if (GetOwningArena() != nullptr &&
      GetOwningArena() == other->GetOwningArena()) {
#else   // PROTOBUF_FORCE_COPY_IN_SWAP
  if (GetOwningArena() == other->GetOwningArena()) {
#endif  // !PROTOBUF_FORCE_COPY_IN_SWAP
    InternalSwap(other);
  } else {
    RepeatedField<Element> temp(other->GetOwningArena());
    temp.MergeFrom(*this);
    CopyFrom(*other);
    other->UnsafeArenaSwap(&temp);
  }
}

template <typename Element>
void RepeatedField<Element>::UnsafeArenaSwap(RepeatedField* other) {
  if (this == other) return;
  GOOGLE_DCHECK_EQ(GetOwningArena(), other->GetOwningArena());
  InternalSwap(other);
}

template <typename Element>
void RepeatedField<Element>::SwapElements(int index1, int index2) {
  using std::swap;  // enable ADL with fallback
  swap(elements()[index1], elements()[index2]);
}

template <typename Element>
inline typename RepeatedField<Element>::iterator
RepeatedField<Element>::begin() {
  return iterator(unsafe_elements());
}
template <typename Element>
inline typename RepeatedField<Element>::const_iterator
RepeatedField<Element>::begin() const {
  return const_iterator(unsafe_elements());
}
template <typename Element>
inline typename RepeatedField<Element>::const_iterator
RepeatedField<Element>::cbegin() const {
  return const_iterator(unsafe_elements());
}
template <typename Element>
inline typename RepeatedField<Element>::iterator RepeatedField<Element>::end() {
  return iterator(unsafe_elements() + current_size_);
}
template <typename Element>
inline typename RepeatedField<Element>::const_iterator
RepeatedField<Element>::end() const {
  return const_iterator(unsafe_elements() + current_size_);
}
template <typename Element>
inline typename RepeatedField<Element>::const_iterator
RepeatedField<Element>::cend() const {
  return const_iterator(unsafe_elements() + current_size_);
}

template <typename Element>
inline size_t RepeatedField<Element>::SpaceUsedExcludingSelfLong() const {
  return total_size_ > 0 ? (total_size_ * sizeof(Element) + kRepHeaderSize) : 0;
}

namespace internal {
// Returns the new size for a reserved field based on its 'total_size' and the
// requested 'new_size'. The result is clamped to the closed interval:
//   [internal::kMinRepeatedFieldAllocationSize,
//    std::numeric_limits<int>::max()]
// Requires:
//     new_size > total_size &&
//     (total_size == 0 ||
//      total_size >= kRepeatedFieldLowerClampLimit)
template <typename T, int kRepHeaderSize>
inline int CalculateReserveSize(int total_size, int new_size) {
  constexpr int lower_limit = RepeatedFieldLowerClampLimit<T, kRepHeaderSize>();
  if (new_size < lower_limit) {
    // Clamp to smallest allowed size.
    return lower_limit;
  }
  constexpr int kMaxSizeBeforeClamp =
      (std::numeric_limits<int>::max() - kRepHeaderSize) / 2;
  if (PROTOBUF_PREDICT_FALSE(total_size > kMaxSizeBeforeClamp)) {
    return std::numeric_limits<int>::max();
  }
  // We want to double the number of bytes, not the number of elements, to try
  // to stay within power-of-two allocations.
  // The allocation has kRepHeaderSize + sizeof(T) * capacity.
  int doubled_size = 2 * total_size + kRepHeaderSize / sizeof(T);
  return std::max(doubled_size, new_size);
}
}  // namespace internal

// Avoid inlining of Reserve(): new, copy, and delete[] lead to a significant
// amount of code bloat.
template <typename Element>
void RepeatedField<Element>::Reserve(int new_size) {
  if (total_size_ >= new_size) return;
  Rep* old_rep = total_size_ > 0 ? rep() : nullptr;
  Rep* new_rep;
  Arena* arena = GetOwningArena();

  new_size = internal::CalculateReserveSize<Element, kRepHeaderSize>(
      total_size_, new_size);

  GOOGLE_DCHECK_LE(
      static_cast<size_t>(new_size),
      (std::numeric_limits<size_t>::max() - kRepHeaderSize) / sizeof(Element))
      << "Requested size is too large to fit into size_t.";
  size_t bytes =
      kRepHeaderSize + sizeof(Element) * static_cast<size_t>(new_size);
  if (arena == nullptr) {
    new_rep = static_cast<Rep*>(::operator new(bytes));
  } else {
    new_rep = reinterpret_cast<Rep*>(Arena::CreateArray<char>(arena, bytes));
  }
  new_rep->arena = arena;
  int old_total_size = total_size_;
  // Already known: new_size >= internal::kMinRepeatedFieldAllocationSize
  // Maintain invariant:
  //     total_size_ == 0 ||
  //     total_size_ >= internal::kMinRepeatedFieldAllocationSize
  total_size_ = new_size;
  arena_or_elements_ = new_rep->elements();
  // Invoke placement-new on newly allocated elements. We shouldn't have to do
  // this, since Element is supposed to be POD, but a previous version of this
  // code allocated storage with "new Element[size]" and some code uses
  // RepeatedField with non-POD types, relying on constructor invocation. If
  // Element has a trivial constructor (e.g., int32_t), gcc (tested with -O2)
  // completely removes this loop because the loop body is empty, so this has no
  // effect unless its side-effects are required for correctness.
  // Note that we do this before MoveArray() below because Element's copy
  // assignment implementation will want an initialized instance first.
  Element* e = &elements()[0];
  Element* limit = e + total_size_;
  for (; e < limit; e++) {
    new (e) Element;
  }
  if (current_size_ > 0) {
    MoveArray(&elements()[0], old_rep->elements(), current_size_);
  }

  // Likewise, we need to invoke destructors on the old array.
  InternalDeallocate(old_rep, old_total_size, false);

}

template <typename Element>
inline void RepeatedField<Element>::Truncate(int new_size) {
  GOOGLE_DCHECK_LE(new_size, current_size_);
  if (current_size_ > 0) {
    ExchangeCurrentSize(new_size);
  }
}

template <typename Element>
inline void RepeatedField<Element>::MoveArray(Element* to, Element* from,
                                              int array_size) {
  CopyArray(to, from, array_size);
}

template <typename Element>
inline void RepeatedField<Element>::CopyArray(Element* to, const Element* from,
                                              int array_size) {
  internal::ElementCopier<Element>()(to, from, array_size);
}

namespace internal {

template <typename Element, bool HasTrivialCopy>
void ElementCopier<Element, HasTrivialCopy>::operator()(Element* to,
                                                        const Element* from,
                                                        int array_size) {
  std::copy(from, from + array_size, to);
}

template <typename Element>
struct ElementCopier<Element, true> {
  void operator()(Element* to, const Element* from, int array_size) {
    memcpy(to, from, static_cast<size_t>(array_size) * sizeof(Element));
  }
};

}  // namespace internal


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
 public:
  using iterator_category = std::random_access_iterator_tag;
  // Note: remove_const is necessary for std::partial_sum, which uses value_type
  // to determine the summation variable type.
  using value_type = typename std::remove_const<Element>::type;
  using difference_type = std::ptrdiff_t;
  using pointer = Element*;
  using reference = Element&;

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
  constexpr reference operator*() const noexcept { return *it_; }
  constexpr pointer operator->() const noexcept { return it_; }

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
  constexpr reference operator[](difference_type d) const noexcept {
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
  explicit RepeatedIterator(Element* it) noexcept : it_(it) {}

  // The internal iterator.
  Element* it_;
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

// Extern declarations of common instantiations to reduce library bloat.
extern template class PROTOBUF_EXPORT_TEMPLATE_DECLARE RepeatedField<bool>;
extern template class PROTOBUF_EXPORT_TEMPLATE_DECLARE RepeatedField<int32_t>;
extern template class PROTOBUF_EXPORT_TEMPLATE_DECLARE RepeatedField<uint32_t>;
extern template class PROTOBUF_EXPORT_TEMPLATE_DECLARE RepeatedField<int64_t>;
extern template class PROTOBUF_EXPORT_TEMPLATE_DECLARE RepeatedField<uint64_t>;
extern template class PROTOBUF_EXPORT_TEMPLATE_DECLARE RepeatedField<float>;
extern template class PROTOBUF_EXPORT_TEMPLATE_DECLARE RepeatedField<double>;

namespace internal {
extern template class PROTOBUF_EXPORT_TEMPLATE_DECLARE RepeatedIterator<bool>;
extern template class PROTOBUF_EXPORT_TEMPLATE_DECLARE
    RepeatedIterator<int32_t>;
extern template class PROTOBUF_EXPORT_TEMPLATE_DECLARE
    RepeatedIterator<uint32_t>;
extern template class PROTOBUF_EXPORT_TEMPLATE_DECLARE
    RepeatedIterator<int64_t>;
extern template class PROTOBUF_EXPORT_TEMPLATE_DECLARE
    RepeatedIterator<uint64_t>;
extern template class PROTOBUF_EXPORT_TEMPLATE_DECLARE RepeatedIterator<float>;
extern template class PROTOBUF_EXPORT_TEMPLATE_DECLARE RepeatedIterator<double>;
}  // namespace internal

}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"

#endif  // GOOGLE_PROTOBUF_REPEATED_FIELD_H__
