// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
// http://code.google.com/p/protobuf/
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
// Typically, clients should not need to access RepeatedField objects directly,
// but should instead use the accessor functions generated automatically by the
// protocol compiler.

#ifndef GOOGLE_PROTOBUF_REPEATED_FIELD_H__
#define GOOGLE_PROTOBUF_REPEATED_FIELD_H__

#include <string>
#include <iterator>
#include <google/protobuf/stubs/common.h>
#include <google/protobuf/message.h>


namespace google {
namespace protobuf {

namespace internal {

// DO NOT USE GenericRepeatedField; it should be considered a private detail
// of RepeatedField/RepeatedPtrField that may be removed in the future.
// GeneratedMessageReflection needs to manipulate repeated fields in a
// generic way, so we have them implement this interface.  This should ONLY
// be used by GeneratedMessageReflection.  This would normally be very bad
// design but GeneratedMessageReflection is a big efficiency hack anyway.
//
// TODO(kenton):  Implement something like Jeff's ProtoVoidPtrArray change.
//   Then, get rid of GenericRepeatedField.
class LIBPROTOBUF_EXPORT GenericRepeatedField {
 public:
  inline GenericRepeatedField() {}
#if defined(__DECCXX) && defined(__osf__)
  // HP C++ on Tru64 has trouble when this is not defined inline.
  virtual ~GenericRepeatedField() {}
#else
  virtual ~GenericRepeatedField();
#endif

 private:
  // We only want GeneratedMessageReflection to see and use these, so we
  // make them private.  Yes, it is valid C++ for a subclass to implement
  // a virtual method which is private in the superclass.  Crazy, huh?
  friend class GeneratedMessageReflection;

  virtual const void* GenericGet(int index) const = 0;
  virtual void* GenericMutable(int index) = 0;
  virtual void* GenericAdd() = 0;
  virtual void GenericClear() = 0;
  virtual int GenericSize() const = 0;
  virtual int GenericSpaceUsedExcludingSelf() const = 0;

  GOOGLE_DISALLOW_EVIL_CONSTRUCTORS(GenericRepeatedField);
};

// We need this (from generated_message_reflection.cc).
int StringSpaceUsedExcludingSelf(const string& str);

}  // namespace internal

// RepeatedField is used to represent repeated fields of a primitive type (in
// other words, everything except strings and nested Messages).  Most users will
// not ever use a RepeatedField directly; they will use the get-by-index,
// set-by-index, and add accessors that are generated for all repeated fields.
template <typename Element>
class RepeatedField : public internal::GenericRepeatedField {
 public:
  RepeatedField();
  ~RepeatedField();

  int size() const;

  Element Get(int index) const;
  Element* Mutable(int index);
  void Set(int index, Element value);
  void Add(Element value);
  // Remove the last element in the array.
  // We don't provide a way to remove any element other than the last
  // because it invites inefficient use, such as O(n^2) filtering loops
  // that should have been O(n).  If you want to remove an element other
  // than the last, the best way to do it is to re-arrange the elements
  // so that the one you want removed is at the end, then call RemoveLast().
  void RemoveLast();
  void Clear();
  void MergeFrom(const RepeatedField& other);

  // Reserve space to expand the field to at least the given size.  If the
  // array is grown, it will always be at least doubled in size.
  void Reserve(int new_size);

  // Gets the underlying array.  This pointer is possibly invalidated by
  // any add or remove operation.
  Element* mutable_data();
  const Element* data() const;

  // Swap entire contents with "other".
  void Swap(RepeatedField* other);

  // STL-like iterator support
  typedef Element* iterator;
  typedef const Element* const_iterator;

  iterator begin();
  const_iterator begin() const;
  iterator end();
  const_iterator end() const;

  // Returns the number of bytes used by the repeated field, excluding
  // sizeof(*this)
  int SpaceUsedExcludingSelf() const;

 private:  // See GenericRepeatedField for why this is private.
  // implements GenericRepeatedField ---------------------------------
  const void* GenericGet(int index) const;
  void* GenericMutable(int index);
  void* GenericAdd();
  void GenericClear();
  int GenericSize() const;
  int GenericSpaceUsedExcludingSelf() const;

 private:
  GOOGLE_DISALLOW_EVIL_CONSTRUCTORS(RepeatedField);

  static const int kInitialSize = 4;

  Element* elements_;
  int      current_size_;
  int      total_size_;

  Element  initial_space_[kInitialSize];
};

namespace internal {
template <typename It> class RepeatedPtrIterator;
}  // namespace internal

// RepeatedPtrField is like RepeatedField, but used for repeated strings or
// Messages.
template <typename Element>
class RepeatedPtrField : public internal::GenericRepeatedField {
 public:
  RepeatedPtrField();

  // This constructor is only defined for RepeatedPtrField<Message>.
  // When a RepeatedPtrField is created using this constructor,
  // prototype->New() will be called to allocate new elements, rather than
  // just using the "new" operator.  This is useful for the implementation
  // of DynamicMessage, but is not used by normal generated messages.
  explicit RepeatedPtrField(const Message* prototype);

  ~RepeatedPtrField();

  // Returns the prototype if one was passed to the constructor.
  const Message* prototype() const;

  int size() const;

  const Element& Get(int index) const;
  Element* Mutable(int index);
  Element* Add();
  void RemoveLast();  // Remove the last element in the array.
  void Clear();
  void MergeFrom(const RepeatedPtrField& other);

  // Reserve space to expand the field to at least the given size.  This only
  // resizes the pointer array; it doesn't allocate any objects.  If the
  // array is grown, it will always be at least doubled in size.
  void Reserve(int new_size);

  // Gets the underlying array.  This pointer is possibly invalidated by
  // any add or remove operation.
  Element** mutable_data();
  const Element* const* data() const;

  // Swap entire contents with "other".
  void Swap(RepeatedPtrField* other);

  // STL-like iterator support
  typedef internal::RepeatedPtrIterator<Element**> iterator;
  typedef internal::RepeatedPtrIterator<const Element* const*> const_iterator;

  iterator begin();
  const_iterator begin() const;
  iterator end();
  const_iterator end() const;

  // Returns (an estimate of) the number of bytes used by the repeated field,
  // excluding sizeof(*this).
  int SpaceUsedExcludingSelf() const;

  // Advanced memory management --------------------------------------
  // When hardcore memory management becomes necessary -- as it often
  // does here at Google -- the following methods may be useful.

  // Add an already-allocated object, passing ownership to the
  // RepeatedPtrField.
  void AddAllocated(Element* value);
  // Remove the last element and return it, passing ownership to the
  // caller.
  // Requires:  size() > 0
  Element* ReleaseLast();

  // When elements are removed by calls to RemoveLast() or Clear(), they
  // are not actually freed.  Instead, they are cleared and kept so that
  // they can be reused later.  This can save lots of CPU time when
  // repeatedly reusing a protocol message for similar purposes.
  //
  // Really, extremely hardcore programs may actually want to manipulate
  // these objects to better-optimize memory management.  These methods
  // allow that.

  // Get the number of cleared objects that are currently being kept
  // around for reuse.
  int ClearedCount();
  // Add an element to the pool of cleared objects, passing ownership to
  // the RepeatedPtrField.  The element must be cleared prior to calling
  // this method.
  void AddCleared(Element* value);
  // Remove a single element from the cleared pool and return it, passing
  // ownership to the caller.  The element is guaranteed to be cleared.
  // Requires:  ClearedCount() > 0
  Element* ReleaseCleared();

 private:  // See GenericRepeatedField for why this is private.
  // implements GenericRepeatedField ---------------------------------
  const void* GenericGet(int index) const;
  void* GenericMutable(int index);
  void* GenericAdd();
  void GenericClear();
  int GenericSize() const;
  int GenericSpaceUsedExcludingSelf() const;

 private:
  // Returns (an estimate of) the number of bytes used by an individual
  // element.
  int ElementSpaceUsed(Element* element) const;

  GOOGLE_DISALLOW_EVIL_CONSTRUCTORS(RepeatedPtrField);

  static const int kInitialSize = 4;

  // prototype_ is used for RepeatedPtrField<Message> only (see constructor).
  const Message* prototype_;

  Element** elements_;
  int       current_size_;
  int       allocated_size_;
  int       total_size_;

  Element*  initial_space_[kInitialSize];

  Element* NewElement();
};

// implementation ====================================================

template <typename Element>
inline RepeatedField<Element>::RepeatedField()
  : elements_(initial_space_),
    current_size_(0),
    total_size_(kInitialSize) {
}

template <typename Element>
RepeatedField<Element>::~RepeatedField() {
  if (elements_ != initial_space_) {
    delete [] elements_;
  }
}

template <typename Element>
inline int RepeatedField<Element>::size() const {
  return current_size_;
}


template <typename Element>
inline Element RepeatedField<Element>::Get(int index) const {
  GOOGLE_DCHECK_LT(index, size());
  return elements_[index];
}

template <typename Element>
inline Element* RepeatedField<Element>::Mutable(int index) {
  GOOGLE_DCHECK_LT(index, size());
  return elements_ + index;
}

template <typename Element>
inline void RepeatedField<Element>::Set(int index, Element value) {
  GOOGLE_DCHECK_LT(index, size());
  elements_[index] = value;
}

template <typename Element>
inline void RepeatedField<Element>::Add(Element value) {
  if (current_size_ == total_size_) Reserve(total_size_ + 1);
  elements_[current_size_++] = value;
}

template <typename Element>
inline void RepeatedField<Element>::RemoveLast() {
  GOOGLE_DCHECK_GT(current_size_, 0);
  --current_size_;
}

template <typename Element>
inline void RepeatedField<Element>::Clear() {
  current_size_ = 0;
}

template <typename Element>
void RepeatedField<Element>::MergeFrom(const RepeatedField& other) {
  Reserve(current_size_ + other.current_size_);
  memcpy(elements_ + current_size_, other.elements_,
         sizeof(Element) * other.current_size_);
  current_size_ += other.current_size_;
}

template <typename Element>
inline Element* RepeatedField<Element>::mutable_data() {
  return elements_;
}

template <typename Element>
inline const Element* RepeatedField<Element>::data() const {
  return elements_;
}


template <typename Element>
void RepeatedField<Element>::Swap(RepeatedField* other) {
  Element* swap_elements     = elements_;
  int      swap_current_size = current_size_;
  int      swap_total_size   = total_size_;
  // We may not be using initial_space_ but it's not worth checking.  Just
  // copy it anyway.
  Element swap_initial_space[kInitialSize];
  memcpy(swap_initial_space, initial_space_, sizeof(initial_space_));

  elements_     = other->elements_;
  current_size_ = other->current_size_;
  total_size_   = other->total_size_;
  memcpy(initial_space_, other->initial_space_, sizeof(initial_space_));

  other->elements_     = swap_elements;
  other->current_size_ = swap_current_size;
  other->total_size_   = swap_total_size;
  memcpy(other->initial_space_, swap_initial_space, sizeof(swap_initial_space));

  if (elements_ == other->initial_space_) {
    elements_ = initial_space_;
  }
  if (other->elements_ == initial_space_) {
    other->elements_ = other->initial_space_;
  }
}

template <typename Element>
inline typename RepeatedField<Element>::iterator
RepeatedField<Element>::begin() {
  return elements_;
}
template <typename Element>
inline typename RepeatedField<Element>::const_iterator
RepeatedField<Element>::begin() const {
  return elements_;
}
template <typename Element>
inline typename RepeatedField<Element>::iterator
RepeatedField<Element>::end() {
  return elements_ + current_size_;
}
template <typename Element>
inline typename RepeatedField<Element>::const_iterator
RepeatedField<Element>::end() const {
  return elements_ + current_size_;
}

template <typename Element>
inline int RepeatedField<Element>::SpaceUsedExcludingSelf() const {
  return (elements_ != initial_space_) ? total_size_ * sizeof(elements_[0]) : 0;
}

template <typename Element>
const void* RepeatedField<Element>::GenericGet(int index) const {
  GOOGLE_DCHECK_LT(index, size());
  return elements_ + index;
}

template <typename Element>
void* RepeatedField<Element>::GenericMutable(int index) {
  return Mutable(index);
}

template <typename Element>
void* RepeatedField<Element>::GenericAdd() {
  Add(Element());
  return Mutable(current_size_ - 1);
}

template <typename Element>
void RepeatedField<Element>::GenericClear() {
  Clear();
}

template <typename Element>
int RepeatedField<Element>::GenericSize() const {
  return size();
}

template <typename Element>
int RepeatedField<Element>::GenericSpaceUsedExcludingSelf() const {
  return SpaceUsedExcludingSelf();
}

template <typename Element>
inline void RepeatedField<Element>::Reserve(int new_size) {
  if (total_size_ >= new_size) return;

  Element* old_elements = elements_;
  total_size_ = max(total_size_ * 2, new_size);
  elements_ = new Element[total_size_];
  memcpy(elements_, old_elements, current_size_ * sizeof(elements_[0]));
  if (old_elements != initial_space_) {
    delete [] old_elements;
  }
}

// -------------------------------------------------------------------

template <typename Element>
inline RepeatedPtrField<Element>::RepeatedPtrField()
  : prototype_(NULL),
    elements_(initial_space_),
    current_size_(0),
    allocated_size_(0),
    total_size_(kInitialSize) {
}

template <>
inline RepeatedPtrField<Message>::RepeatedPtrField(const Message* prototype)
  : prototype_(prototype),
    elements_(initial_space_),
    current_size_(0),
    allocated_size_(0),
    total_size_(kInitialSize) {
}

template <typename Element>
RepeatedPtrField<Element>::~RepeatedPtrField() {
  for (int i = 0; i < allocated_size_; i++) {
    delete elements_[i];
  }
  if (elements_ != initial_space_) {
    delete [] elements_;
  }
}

template <>
inline const Message* RepeatedPtrField<Message>::prototype() const {
  return prototype_;
}


template <typename Element>
inline int RepeatedPtrField<Element>::size() const {
  return current_size_;
}


template <typename Element>
inline const Element& RepeatedPtrField<Element>::Get(int index) const {
  GOOGLE_DCHECK_LT(index, size());
  return *elements_[index];
}

template <typename Element>
inline Element* RepeatedPtrField<Element>::Mutable(int index) {
  GOOGLE_DCHECK_LT(index, size());
  return elements_[index];
}

template <typename Element>
inline Element* RepeatedPtrField<Element>::Add() {
  if (current_size_ < allocated_size_) return elements_[current_size_++];
  if (allocated_size_ == total_size_) Reserve(total_size_ + 1);
  ++allocated_size_;
  return elements_[current_size_++] = NewElement();
}

template <typename Element>
inline void RepeatedPtrField<Element>::RemoveLast() {
  GOOGLE_DCHECK_GT(current_size_, 0);
  elements_[--current_size_]->Clear();
}

template <>
inline void RepeatedPtrField<string>::RemoveLast() {
  GOOGLE_DCHECK_GT(current_size_, 0);
  elements_[--current_size_]->clear();
}

template <typename Element>
void RepeatedPtrField<Element>::Clear() {
  for (int i = 0; i < current_size_; i++) {
    elements_[i]->Clear();
  }
  current_size_ = 0;
}

#if defined(__DECCXX) && defined(__osf__)
// HP C++ on Tru64 has trouble when this is not defined inline.
template <>
inline void RepeatedPtrField<string>::Clear() {
  for (int i = 0; i < current_size_; i++) {
    elements_[i]->clear();
  }
  current_size_ = 0;
}
#else
// Specialization defined in repeated_field.cc.
template <>
void LIBPROTOBUF_EXPORT RepeatedPtrField<string>::Clear();
#endif

template <typename Element>
void RepeatedPtrField<Element>::MergeFrom(const RepeatedPtrField& other) {
  Reserve(current_size_ + other.current_size_);
  for (int i = 0; i < other.current_size_; i++) {
    Add()->MergeFrom(other.Get(i));
  }
}

template <>
inline void RepeatedPtrField<string>::MergeFrom(const RepeatedPtrField& other) {
  Reserve(current_size_ + other.current_size_);
  for (int i = 0; i < other.current_size_; i++) {
    Add()->assign(other.Get(i));
  }
}


template <typename Element>
inline Element** RepeatedPtrField<Element>::mutable_data() {
  return elements_;
}

template <typename Element>
inline const Element* const* RepeatedPtrField<Element>::data() const {
  return elements_;
}


template <typename Element>
void RepeatedPtrField<Element>::Swap(RepeatedPtrField* other) {
  Element** swap_elements       = elements_;
  int       swap_current_size   = current_size_;
  int       swap_allocated_size = allocated_size_;
  int       swap_total_size     = total_size_;
  // We may not be using initial_space_ but it's not worth checking.  Just
  // copy it anyway.
  Element* swap_initial_space[kInitialSize];
  memcpy(swap_initial_space, initial_space_, sizeof(initial_space_));

  elements_       = other->elements_;
  current_size_   = other->current_size_;
  allocated_size_ = other->allocated_size_;
  total_size_     = other->total_size_;
  memcpy(initial_space_, other->initial_space_, sizeof(initial_space_));

  other->elements_       = swap_elements;
  other->current_size_   = swap_current_size;
  other->allocated_size_ = swap_allocated_size;
  other->total_size_     = swap_total_size;
  memcpy(other->initial_space_, swap_initial_space, sizeof(swap_initial_space));

  if (elements_ == other->initial_space_) {
    elements_ = initial_space_;
  }
  if (other->elements_ == initial_space_) {
    other->elements_ = other->initial_space_;
  }
}

template <typename Element>
inline int RepeatedPtrField<Element>::SpaceUsedExcludingSelf() const {
  int allocated_bytes =
      (elements_ != initial_space_) ? total_size_ * sizeof(elements_[0]) : 0;
  for (int i = 0; i < allocated_size_; ++i) {
    allocated_bytes += ElementSpaceUsed(elements_[i]);
  }
  return allocated_bytes;
}

template <typename Element>
inline int RepeatedPtrField<Element>::ElementSpaceUsed(Element* e) const {
  return e->SpaceUsed();
}

template <>
inline int RepeatedPtrField<string>::ElementSpaceUsed(string* s) const {
  return sizeof(*s) + internal::StringSpaceUsedExcludingSelf(*s);
}


template <typename Element>
inline void RepeatedPtrField<Element>::AddAllocated(Element* value) {
  if (allocated_size_ == total_size_) Reserve(total_size_ + 1);
  // We don't care about the order of cleared elements, so if there's one
  // in the way, just move it to the back of the array.
  if (current_size_ < allocated_size_) {
    elements_[allocated_size_] = elements_[current_size_];
  }
  ++allocated_size_;
  elements_[current_size_++] = value;
}

template <typename Element>
inline Element* RepeatedPtrField<Element>::ReleaseLast() {
  GOOGLE_DCHECK_GT(current_size_, 0);
  Element* result = elements_[--current_size_];
  --allocated_size_;
  if (current_size_ < allocated_size_) {
    // There are cleared elements on the end; replace the removed element
    // with the last allocated element.
    elements_[current_size_] = elements_[allocated_size_];
  }
  return result;
}


template <typename Element>
inline int RepeatedPtrField<Element>::ClearedCount() {
  return allocated_size_ - current_size_;
}

template <typename Element>
inline void RepeatedPtrField<Element>::AddCleared(Element* value) {
  if (allocated_size_ == total_size_) Reserve(total_size_ + 1);
  elements_[allocated_size_++] = value;
}

template <typename Element>
inline Element* RepeatedPtrField<Element>::ReleaseCleared() {
  GOOGLE_DCHECK_GT(allocated_size_, current_size_);
  return elements_[--allocated_size_];
}


template <typename Element>
const void* RepeatedPtrField<Element>::GenericGet(int index) const {
  return &Get(index);
}

template <typename Element>
void* RepeatedPtrField<Element>::GenericMutable(int index) {
  return Mutable(index);
}

template <typename Element>
void* RepeatedPtrField<Element>::GenericAdd() {
  return Add();
}

template <typename Element>
void RepeatedPtrField<Element>::GenericClear() {
  Clear();
}

template <typename Element>
int RepeatedPtrField<Element>::GenericSize() const {
  return size();
}

template <typename Element>
int RepeatedPtrField<Element>::GenericSpaceUsedExcludingSelf() const {
  return SpaceUsedExcludingSelf();
}


template <typename Element>
inline void RepeatedPtrField<Element>::Reserve(int new_size) {
  if (total_size_ >= new_size) return;

  Element** old_elements = elements_;
  total_size_ = max(total_size_ * 2, new_size);
  elements_ = new Element*[total_size_];
  memcpy(elements_, old_elements, allocated_size_ * sizeof(elements_[0]));
  if (old_elements != initial_space_) {
    delete [] old_elements;
  }
}

template <typename Element>
inline Element* RepeatedPtrField<Element>::NewElement() {
  return new Element;
}

// RepeatedPtrField<Message> is alowed but requires a prototype since Message
// is abstract.
template <>
inline Message* RepeatedPtrField<Message>::NewElement() {
  return prototype_->New();
}

// -------------------------------------------------------------------

namespace internal {

// STL-like iterator implementation for RepeatedPtrField.  You should not
// refer to this class directly; use RepeatedPtrField<T>::iterator instead.
//
// The iterator for RepeatedPtrField<T>, RepeatedPtrIterator<T**>, is
// very similar to iterator_ptr<> in util/gtl/iterator_adaptors-inl.h,
// but adds random-access operators and is slightly more specialized
// for using T** as its base type. I didn't re-use the other class to
// avoid an extra dependency.
//
// This code stolen from net/proto/proto-array-internal.h by Jeffrey Yasskin
// (jyasskin@google.com).
template<typename It>
class RepeatedPtrIterator
    : public std::iterator<
          std::random_access_iterator_tag,
          typename internal::remove_pointer<
              typename internal::remove_pointer<It>::type>::type> {
 public:
  typedef RepeatedPtrIterator<It> iterator;
  typedef std::iterator<
          std::random_access_iterator_tag,
          typename internal::remove_pointer<
              typename internal::remove_pointer<It>::type>::type> superclass;

  // Let the compiler know that these are type names, so we don't have to
  // write "typename" in front of them everywhere.
  typedef typename superclass::reference reference;
  typedef typename superclass::pointer pointer;
  typedef typename superclass::difference_type difference_type;

  RepeatedPtrIterator() : it_(NULL) {}
  explicit RepeatedPtrIterator(const It& it) : it_(it) {}

  // Allow "upcasting" from RepeatedPtrIterator<T**> to
  // RepeatedPtrIterator<const T*const*>.
  template<typename OtherIt>
  RepeatedPtrIterator(const RepeatedPtrIterator<OtherIt>& other)
      : it_(other.base()) {}

  // Provide access to the wrapped iterator.
  const It& base() const { return it_; }

  // dereferenceable
  reference operator*() const { return **it_; }
  pointer   operator->() const { return &(operator*()); }

  // {inc,dec}rementable
  iterator& operator++() { ++it_; return *this; }
  iterator  operator++(int) { return iterator(it_++); }
  iterator& operator--() { --it_; return *this; }
  iterator  operator--(int) { return iterator(it_--); }

  // equality_comparable
  bool operator==(const iterator& x) const { return it_ == x.it_; }
  bool operator!=(const iterator& x) const { return it_ != x.it_; }

  // less_than_comparable
  bool operator<(const iterator& x) const { return it_ < x.it_; }
  bool operator<=(const iterator& x) const { return it_ <= x.it_; }
  bool operator>(const iterator& x) const { return it_ > x.it_; }
  bool operator>=(const iterator& x) const { return it_ >= x.it_; }

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
  reference operator[](difference_type d) const { return *(*this + d); }

  // random access iterator
  difference_type operator-(const iterator& x) const { return it_ - x.it_; }

 private:
  // The internal iterator.
  It it_;
};

}  // namespace internal

template <typename Element>
inline typename RepeatedPtrField<Element>::iterator
RepeatedPtrField<Element>::begin() {
  return iterator(elements_);
}
template <typename Element>
inline typename RepeatedPtrField<Element>::const_iterator
RepeatedPtrField<Element>::begin() const {
  return iterator(elements_);
}
template <typename Element>
inline typename RepeatedPtrField<Element>::iterator
RepeatedPtrField<Element>::end() {
  return iterator(elements_ + current_size_);
}
template <typename Element>
inline typename RepeatedPtrField<Element>::const_iterator
RepeatedPtrField<Element>::end() const {
  return iterator(elements_ + current_size_);
}

}  // namespace protobuf

}  // namespace google
#endif  // GOOGLE_PROTOBUF_REPEATED_FIELD_H__
