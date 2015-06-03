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

#ifndef GOOGLE_PROTOBUF_MAP_H__
#define GOOGLE_PROTOBUF_MAP_H__

#include <iterator>
#include <google/protobuf/stubs/hash.h>
#include <limits>  // To support Visual Studio 2008

#include <google/protobuf/arena.h>
#include <google/protobuf/generated_enum_util.h>
#include <google/protobuf/map_type_handler.h>

namespace google {
namespace protobuf {

template <typename Key, typename T>
class Map;

template <typename Enum> struct is_proto_enum;

namespace internal {
template <typename Key, typename T,
          WireFormatLite::FieldType key_wire_type,
          WireFormatLite::FieldType value_wire_type,
          int default_enum_value>
class MapFieldLite;
}

// This is the class for google::protobuf::Map's internal value_type. Instead of using
// std::pair as value_type, we use this class which provides us more control of
// its process of construction and destruction.
template <typename Key, typename T>
class MapPair {
 public:
  typedef const Key first_type;
  typedef T second_type;

  MapPair(const Key& other_first, const T& other_second)
      : first(other_first), second(other_second) {}
  explicit MapPair(const Key& other_first) : first(other_first), second() {}
  MapPair(const MapPair& other)
      : first(other.first), second(other.second) {}

  ~MapPair() {}

  // Implicitly convertible to std::pair of compatible types.
  template <typename T1, typename T2>
  operator std::pair<T1, T2>() const {
    return std::pair<T1, T2>(first, second);
  }

  const Key first;
  T second;

 private:
  typedef void DestructorSkippable_;
  friend class ::google::protobuf::Arena;
  friend class Map<Key, T>;
};

// google::protobuf::Map is an associative container type used to store protobuf map
// fields. Its interface is similar to std::unordered_map. Users should use this
// interface directly to visit or change map fields.
template <typename Key, typename T>
class Map {
  typedef internal::MapCppTypeHandler<Key> KeyTypeHandler;
  typedef internal::MapCppTypeHandler<T> ValueTypeHandler;

 public:
  typedef Key key_type;
  typedef T mapped_type;
  typedef MapPair<Key, T> value_type;

  typedef value_type* pointer;
  typedef const value_type* const_pointer;
  typedef value_type& reference;
  typedef const value_type& const_reference;

  typedef size_t size_type;
  typedef hash<Key> hasher;
  typedef equal_to<Key> key_equal;

  Map()
      : arena_(NULL),
        allocator_(arena_),
        elements_(0, hasher(), key_equal(), allocator_),
        default_enum_value_(0) {}
  explicit Map(Arena* arena)
      : arena_(arena),
        allocator_(arena_),
        elements_(0, hasher(), key_equal(), allocator_),
        default_enum_value_(0) {}

  Map(const Map& other)
      : arena_(NULL),
        allocator_(arena_),
        elements_(0, hasher(), key_equal(), allocator_),
        default_enum_value_(other.default_enum_value_) {
    insert(other.begin(), other.end());
  }

  ~Map() { clear(); }

 private:
  // re-implement std::allocator to use arena allocator for memory allocation.
  // Used for google::protobuf::Map implementation. Users should not use this class
  // directly.
  template <typename U>
  class MapAllocator {
   public:
    typedef U value_type;
    typedef value_type* pointer;
    typedef const value_type* const_pointer;
    typedef value_type& reference;
    typedef const value_type& const_reference;
    typedef size_t size_type;
    typedef ptrdiff_t difference_type;

    MapAllocator() : arena_(NULL) {}
    explicit MapAllocator(Arena* arena) : arena_(arena) {}
    template <typename X>
    MapAllocator(const MapAllocator<X>& allocator)
        : arena_(allocator.arena_) {}

    pointer allocate(size_type n, const_pointer hint = 0) {
      // If arena is not given, malloc needs to be called which doesn't
      // construct element object.
      if (arena_ == NULL) {
        return reinterpret_cast<pointer>(malloc(n * sizeof(value_type)));
      } else {
        return reinterpret_cast<pointer>(
            Arena::CreateArray<uint8>(arena_, n * sizeof(value_type)));
      }
    }

    void deallocate(pointer p, size_type n) {
      if (arena_ == NULL) {
        free(p);
      }
    }

#if __cplusplus >= 201103L && !defined(GOOGLE_PROTOBUF_OS_APPLE)
    template<class NodeType, class... Args>
    void construct(NodeType* p, Args&&... args) {
      new (p) NodeType(std::forward<Args>(args)...);
    }

    template<class NodeType>
    void destroy(NodeType* p) {
      p->~NodeType();
    }
#else
    void construct(pointer p, const_reference t) { new (p) value_type(t); }

    void destroy(pointer p) { p->~value_type(); }
#endif

    template <typename X>
    struct rebind {
      typedef MapAllocator<X> other;
    };

    template <typename X>
    bool operator==(const MapAllocator<X>& other) const {
      return arena_ == other.arena_;
    }

    template <typename X>
    bool operator!=(const MapAllocator<X>& other) const {
      return arena_ != other.arena_;
    }

    // To support Visual Studio 2008
    size_type max_size() const {
      return std::numeric_limits<size_type>::max();
    }

   private:
    Arena* arena_;

    template <typename X>
    friend class MapAllocator;
  };

 public:
  typedef MapAllocator<std::pair<const Key, MapPair<Key, T>*> > Allocator;

  // Iterators
  class const_iterator
      : public std::iterator<std::forward_iterator_tag, value_type, ptrdiff_t,
                             const value_type*, const value_type&> {
    typedef typename hash_map<Key, value_type*, hash<Key>, equal_to<Key>,
                              Allocator>::const_iterator InnerIt;

   public:
    const_iterator() {}
    explicit const_iterator(const InnerIt& it) : it_(it) {}

    const_reference operator*() const { return *it_->second; }
    const_pointer operator->() const { return it_->second; }

    const_iterator& operator++() {
      ++it_;
      return *this;
    }
    const_iterator operator++(int) { return const_iterator(it_++); }

    friend bool operator==(const const_iterator& a, const const_iterator& b) {
      return a.it_ == b.it_;
    }
    friend bool operator!=(const const_iterator& a, const const_iterator& b) {
      return a.it_ != b.it_;
    }

   private:
    InnerIt it_;
  };

  class iterator : public std::iterator<std::forward_iterator_tag, value_type> {
    typedef typename hash_map<Key, value_type*, hasher, equal_to<Key>,
                              Allocator>::iterator InnerIt;

   public:
    iterator() {}
    explicit iterator(const InnerIt& it) : it_(it) {}

    reference operator*() const { return *it_->second; }
    pointer operator->() const { return it_->second; }

    iterator& operator++() {
      ++it_;
      return *this;
    }
    iterator operator++(int) { return iterator(it_++); }

    // Implicitly convertible to const_iterator.
    operator const_iterator() const { return const_iterator(it_); }

    friend bool operator==(const iterator& a, const iterator& b) {
      return a.it_ == b.it_;
    }
    friend bool operator!=(const iterator& a, const iterator& b) {
      return a.it_ != b.it_;
    }

   private:
    friend class Map;
    InnerIt it_;
  };

  iterator begin() { return iterator(elements_.begin()); }
  iterator end() { return iterator(elements_.end()); }
  const_iterator begin() const { return const_iterator(elements_.begin()); }
  const_iterator end() const { return const_iterator(elements_.end()); }
  const_iterator cbegin() const { return begin(); }
  const_iterator cend() const { return end(); }

  // Capacity
  size_type size() const { return elements_.size(); }
  bool empty() const { return elements_.empty(); }

  // Element access
  T& operator[](const key_type& key) {
    value_type** value = &elements_[key];
    if (*value == NULL) {
      *value = CreateValueTypeInternal(key);
      internal::MapValueInitializer<google::protobuf::is_proto_enum<T>::value,
                                    T>::Initialize((*value)->second,
                                                   default_enum_value_);
    }
    return (*value)->second;
  }
  const T& at(const key_type& key) const {
    const_iterator it = find(key);
    GOOGLE_CHECK(it != end());
    return it->second;
  }
  T& at(const key_type& key) {
    iterator it = find(key);
    GOOGLE_CHECK(it != end());
    return it->second;
  }

  // Lookup
  size_type count(const key_type& key) const {
    return elements_.count(key);
  }
  const_iterator find(const key_type& key) const {
    return const_iterator(elements_.find(key));
  }
  iterator find(const key_type& key) {
    return iterator(elements_.find(key));
  }
  std::pair<const_iterator, const_iterator> equal_range(
      const key_type& key) const {
    const_iterator it = find(key);
    if (it == end()) {
      return std::pair<const_iterator, const_iterator>(it, it);
    } else {
      const_iterator begin = it++;
      return std::pair<const_iterator, const_iterator>(begin, it);
    }
  }
  std::pair<iterator, iterator> equal_range(const key_type& key) {
    iterator it = find(key);
    if (it == end()) {
      return std::pair<iterator, iterator>(it, it);
    } else {
      iterator begin = it++;
      return std::pair<iterator, iterator>(begin, it);
    }
  }

  // insert
  std::pair<iterator, bool> insert(const value_type& value) {
    iterator it = find(value.first);
    if (it != end()) {
      return std::pair<iterator, bool>(it, false);
    } else {
      return std::pair<iterator, bool>(
          iterator(elements_.insert(std::pair<Key, value_type*>(
              value.first, CreateValueTypeInternal(value))).first), true);
    }
  }
  template <class InputIt>
  void insert(InputIt first, InputIt last) {
    for (InputIt it = first; it != last; ++it) {
      iterator exist_it = find(it->first);
      if (exist_it == end()) {
        operator[](it->first) = it->second;
      }
    }
  }

  // Erase
  size_type erase(const key_type& key) {
    typename hash_map<Key, value_type*, hash<Key>, equal_to<Key>,
                      Allocator>::iterator it = elements_.find(key);
    if (it == elements_.end()) {
      return 0;
    } else {
      if (arena_ == NULL) delete it->second;
      elements_.erase(it);
      return 1;
    }
  }
  void erase(iterator pos) {
    if (arena_ == NULL) delete pos.it_->second;
    elements_.erase(pos.it_);
  }
  void erase(iterator first, iterator last) {
    for (iterator it = first; it != last;) {
      if (arena_ == NULL) delete it.it_->second;
      elements_.erase((it++).it_);
    }
  }
  void clear() {
    for (iterator it = begin(); it != end(); ++it) {
      if (arena_ == NULL) delete it.it_->second;
    }
    elements_.clear();
  }

  // Assign
  Map& operator=(const Map& other) {
    if (this != &other) {
      clear();
      insert(other.begin(), other.end());
    }
    return *this;
  }

 private:
  // Set default enum value only for proto2 map field whose value is enum type.
  void SetDefaultEnumValue(int default_enum_value) {
    default_enum_value_ = default_enum_value;
  }

  value_type* CreateValueTypeInternal(const Key& key) {
    if (arena_ == NULL) {
      return new value_type(key);
    } else {
      value_type* value = reinterpret_cast<value_type*>(
          Arena::CreateArray<uint8>(arena_, sizeof(value_type)));
      Arena::CreateInArenaStorage(const_cast<Key*>(&value->first), arena_);
      Arena::CreateInArenaStorage(&value->second, arena_);
      const_cast<Key&>(value->first) = key;
      return value;
    }
  }

  value_type* CreateValueTypeInternal(const value_type& value) {
    if (arena_ == NULL) {
      return new value_type(value);
    } else {
      value_type* p = reinterpret_cast<value_type*>(
          Arena::CreateArray<uint8>(arena_, sizeof(value_type)));
      Arena::CreateInArenaStorage(const_cast<Key*>(&p->first), arena_);
      Arena::CreateInArenaStorage(&p->second, arena_);
      const_cast<Key&>(p->first) = value.first;
      p->second = value.second;
      return p;
    }
  }

  Arena* arena_;
  Allocator allocator_;
  hash_map<Key, value_type*, hash<Key>, equal_to<Key>, Allocator> elements_;
  int default_enum_value_;

  friend class ::google::protobuf::Arena;
  typedef void InternalArenaConstructable_;
  typedef void DestructorSkippable_;
  template <typename K, typename V,
            internal::WireFormatLite::FieldType key_wire_type,
            internal::WireFormatLite::FieldType value_wire_type,
            int default_enum_value>
  friend class internal::MapFieldLite;
};

}  // namespace protobuf

}  // namespace google
#endif  // GOOGLE_PROTOBUF_MAP_H__
