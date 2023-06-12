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

#ifndef GOOGLE_PROTOBUF_MAP_ENTRY_LITE_H__
#define GOOGLE_PROTOBUF_MAP_ENTRY_LITE_H__

#include <assert.h>

#include <algorithm>
#include <string>
#include <utility>

#include "google/protobuf/arena.h"
#include "absl/base/casts.h"
#include "google/protobuf/arenastring.h"
#include "google/protobuf/generated_message_util.h"
#include "google/protobuf/io/coded_stream.h"
#include "google/protobuf/map.h"
#include "google/protobuf/map_type_handler.h"
#include "google/protobuf/parse_context.h"
#include "google/protobuf/port.h"
#include "google/protobuf/wire_format_lite.h"

// Must be included last.
#include "google/protobuf/port_def.inc"

#ifdef SWIG
#error "You cannot SWIG proto headers"
#endif

namespace google {
namespace protobuf {
namespace internal {
template <typename Derived, typename Key, typename Value,
          WireFormatLite::FieldType kKeyFieldType,
          WireFormatLite::FieldType kValueFieldType>
class MapEntry;
template <typename Derived, typename Key, typename Value,
          WireFormatLite::FieldType kKeyFieldType,
          WireFormatLite::FieldType kValueFieldType>
class MapFieldLite;
}  // namespace internal
}  // namespace protobuf
}  // namespace google

namespace google {
namespace protobuf {
namespace internal {

// MoveHelper::Move is used to set *dest.  It copies *src, or moves it (in
// the C++11 sense), or swaps it. *src is left in a sane state for
// subsequent destruction, but shouldn't be used for anything.
template <bool is_enum, bool is_message, bool is_stringlike, typename T>
struct MoveHelper {  // primitives
  static void Move(T* src, T* dest) { *dest = *src; }
};

template <bool is_message, bool is_stringlike, typename T>
struct MoveHelper<true, is_message, is_stringlike, T> {  // enums
  static void Move(T* src, T* dest) { *dest = *src; }
  // T is an enum here, so allow conversions to and from int.
  static void Move(T* src, int* dest) { *dest = static_cast<int>(*src); }
  static void Move(int* src, T* dest) { *dest = static_cast<T>(*src); }
};

template <bool is_stringlike, typename T>
struct MoveHelper<false, true, is_stringlike, T> {  // messages
  static void Move(T* src, T* dest) { dest->Swap(src); }
};

template <typename T>
struct MoveHelper<false, false, true, T> {  // strings and similar
  static void Move(T* src, T* dest) {
    *dest = std::move(*src);
  }
};

// MapEntryImpl is used to implement parsing and serialization of map entries.
// It uses Curiously Recurring Template Pattern (CRTP) to provide the type of
// the eventual code to the template code.
template <typename Derived, typename Base, typename Key, typename Value,
          WireFormatLite::FieldType kKeyFieldType,
          WireFormatLite::FieldType kValueFieldType>
class MapEntryImpl : public Base {
 public:
  typedef MapEntryFuncs<Key, Value, kKeyFieldType, kValueFieldType> Funcs;

 protected:
  // Provide utilities to parse/serialize key/value.  Provide utilities to
  // manipulate internal stored type.
  typedef MapTypeHandler<kKeyFieldType, Key> KeyTypeHandler;
  typedef MapTypeHandler<kValueFieldType, Value> ValueTypeHandler;

  // Define internal memory layout. Strings and messages are stored as
  // pointers, while other types are stored as values.
  typedef typename KeyTypeHandler::TypeOnMemory KeyOnMemory;
  typedef typename ValueTypeHandler::TypeOnMemory ValueOnMemory;

  // Enum type cannot be used for MapTypeHandler::Read. Define a type
  // which will replace Enum with int.
  typedef typename KeyTypeHandler::MapEntryAccessorType KeyMapEntryAccessorType;
  typedef
      typename ValueTypeHandler::MapEntryAccessorType ValueMapEntryAccessorType;

  // Constants for field number.
  static const int kKeyFieldNumber = 1;
  static const int kValueFieldNumber = 2;

  // Constants for field tag.
  static const uint8_t kKeyTag =
      GOOGLE_PROTOBUF_WIRE_FORMAT_MAKE_TAG(kKeyFieldNumber, KeyTypeHandler::kWireType);
  static const uint8_t kValueTag = GOOGLE_PROTOBUF_WIRE_FORMAT_MAKE_TAG(
      kValueFieldNumber, ValueTypeHandler::kWireType);
  static const size_t kTagSize = 1;

 public:
  // Work-around for a compiler bug (see repeated_field.h).
  typedef void MapEntryHasMergeTypeTrait;
  typedef Derived EntryType;
  typedef Key EntryKeyType;
  typedef Value EntryValueType;
  static const WireFormatLite::FieldType kEntryKeyFieldType = kKeyFieldType;
  static const WireFormatLite::FieldType kEntryValueFieldType = kValueFieldType;

  constexpr MapEntryImpl()
      : key_(KeyTypeHandler::Constinit()),
        value_(ValueTypeHandler::Constinit()),
        _has_bits_{} {}

  explicit MapEntryImpl(Arena* arena)
      : Base(arena),
        key_(KeyTypeHandler::Constinit()),
        value_(ValueTypeHandler::Constinit()),
        _has_bits_{} {}

  MapEntryImpl(const MapEntryImpl&) = delete;
  MapEntryImpl& operator=(const MapEntryImpl&) = delete;

  ~MapEntryImpl() override {
    if (Base::GetArenaForAllocation() != nullptr) return;
    KeyTypeHandler::DeleteNoArena(key_);
    ValueTypeHandler::DeleteNoArena(value_);
  }

  // accessors ======================================================

  inline const KeyMapEntryAccessorType& key() const {
    return KeyTypeHandler::GetExternalReference(key_);
  }
  inline const ValueMapEntryAccessorType& value() const {
    return ValueTypeHandler::DefaultIfNotInitialized(value_);
  }
  inline KeyMapEntryAccessorType* mutable_key() {
    set_has_key();
    return KeyTypeHandler::EnsureMutable(&key_, Base::GetArenaForAllocation());
  }
  inline ValueMapEntryAccessorType* mutable_value() {
    set_has_value();
    return ValueTypeHandler::EnsureMutable(&value_,
                                           Base::GetArenaForAllocation());
  }

  // implements MessageLite =========================================

  // MapEntryImpl is for implementation only and this function isn't called
  // anywhere. Just provide a fake implementation here for MessageLite.
  std::string GetTypeName() const override { return ""; }

  void CheckTypeAndMergeFrom(const MessageLite& other) override {
    MergeFromInternal(*::google::protobuf::internal::DownCast<const Derived*>(&other));
  }

  const char* _InternalParse(const char* ptr, ParseContext* ctx) final {
    while (!ctx->Done(&ptr)) {
      uint32_t tag;
      ptr = ReadTag(ptr, &tag);
      GOOGLE_PROTOBUF_PARSER_ASSERT(ptr);
      if (tag == kKeyTag) {
        set_has_key();
        KeyMapEntryAccessorType* key = mutable_key();
        ptr = KeyTypeHandler::Read(ptr, ctx, key);
        if (!Derived::ValidateKey(key)) return nullptr;
      } else if (tag == kValueTag) {
        set_has_value();
        ValueMapEntryAccessorType* value = mutable_value();
        ptr = ValueTypeHandler::Read(ptr, ctx, value);
        if (!Derived::ValidateValue(value)) return nullptr;
      } else {
        if (tag == 0 || WireFormatLite::GetTagWireType(tag) ==
                            WireFormatLite::WIRETYPE_END_GROUP) {
          ctx->SetLastTag(tag);
          return ptr;
        }
        ptr = UnknownFieldParse(tag, static_cast<std::string*>(nullptr), ptr,
                                ctx);
      }
      GOOGLE_PROTOBUF_PARSER_ASSERT(ptr);
    }
    return ptr;
  }

  size_t ByteSizeLong() const override {
    size_t size = 0;
    size += kTagSize + static_cast<size_t>(KeyTypeHandler::ByteSize(key()));
    size += kTagSize + static_cast<size_t>(ValueTypeHandler::ByteSize(value()));
    return size;
  }

  ::uint8_t* _InternalSerialize(
      ::uint8_t* ptr, io::EpsCopyOutputStream* stream) const override {
    ptr = KeyTypeHandler::Write(kKeyFieldNumber, key(), ptr, stream);
    return ValueTypeHandler::Write(kValueFieldNumber, value(), ptr, stream);
  }

  // Don't override SerializeWithCachedSizesToArray.  Use MessageLite's.

  int GetCachedSize() const override {
    int size = 0;
    size += has_key() ? static_cast<int>(kTagSize) +
                            KeyTypeHandler::GetCachedSize(key())
                      : 0;
    size += has_value() ? static_cast<int>(kTagSize) +
                              ValueTypeHandler::GetCachedSize(value())
                        : 0;
    return size;
  }

  bool IsInitialized() const override {
    return ValueTypeHandler::IsInitialized(value_);
  }

  Base* New(Arena* arena) const override {
    Derived* entry = Arena::CreateMessage<Derived>(arena);
    return entry;
  }

 protected:
  // We can't declare this function directly here as it would hide the other
  // overload (const Message&).
  void MergeFromInternal(const MapEntryImpl& from) {
    if (from._has_bits_[0]) {
      if (from.has_key()) {
        KeyTypeHandler::EnsureMutable(&key_, Base::GetArenaForAllocation());
        KeyTypeHandler::Merge(from.key(), &key_, Base::GetArenaForAllocation());
        set_has_key();
      }
      if (from.has_value()) {
        ValueTypeHandler::EnsureMutable(&value_, Base::GetArenaForAllocation());
        ValueTypeHandler::Merge(from.value(), &value_,
                                Base::GetArenaForAllocation());
        set_has_value();
      }
    }
  }

 public:
  void Clear() override {
    KeyTypeHandler::Clear(&key_, Base::GetArenaForAllocation());
    ValueTypeHandler::Clear(&value_, Base::GetArenaForAllocation());
    clear_has_key();
    clear_has_value();
  }

  // Parsing using MergePartialFromCodedStream, above, is not as
  // efficient as it could be.  This helper class provides a speedier way.
  template <typename MapField, typename Map>
  class Parser {
   public:
    explicit Parser(MapField* mf) : mf_(mf), map_(mf->MutableMap()) {}
    ~Parser() {
      if (entry_ != nullptr && entry_->GetArenaForAllocation() == nullptr)
        delete entry_;
    }

    const char* _InternalParse(const char* ptr, ParseContext* ctx) {
      if (PROTOBUF_PREDICT_TRUE(!ctx->Done(&ptr) && *ptr == kKeyTag)) {
        ptr = KeyTypeHandler::Read(ptr + 1, ctx, &key_);
        if (PROTOBUF_PREDICT_FALSE(!ptr || !Derived::ValidateKey(&key_))) {
          return nullptr;
        }
        if (PROTOBUF_PREDICT_TRUE(!ctx->Done(&ptr) && *ptr == kValueTag)) {
          typename Map::size_type map_size = map_->size();
          value_ptr_ = &(*map_)[key_];
          if (PROTOBUF_PREDICT_TRUE(map_size != map_->size())) {
            using T =
                typename MapIf<ValueTypeHandler::kIsEnum, int*, Value*>::type;
            ptr = ValueTypeHandler::Read(ptr + 1, ctx,
                                         reinterpret_cast<T>(value_ptr_));
            if (PROTOBUF_PREDICT_FALSE(!ptr ||
                                       !Derived::ValidateValue(value_ptr_))) {
              map_->erase(key_);  // Failure! Undo insertion.
              return nullptr;
            }
            if (PROTOBUF_PREDICT_TRUE(ctx->Done(&ptr))) return ptr;
            if (!ptr) return nullptr;
            NewEntry();
            ValueMover::Move(value_ptr_, entry_->mutable_value());
            map_->erase(key_);
            goto move_key;
          }
        } else {
          if (!ptr) return nullptr;
        }
        NewEntry();
      move_key:
        KeyMover::Move(&key_, entry_->mutable_key());
      } else {
        if (!ptr) return nullptr;
        NewEntry();
      }
      ptr = entry_->_InternalParse(ptr, ctx);
      if (ptr) UseKeyAndValueFromEntry();
      return ptr;
    }

    template <typename UnknownType>
    const char* ParseWithEnumValidation(const char* ptr, ParseContext* ctx,
                                        bool (*is_valid)(int),
                                        uint32_t field_num,
                                        InternalMetadata* metadata) {
      auto entry = NewEntry();
      ptr = entry->_InternalParse(ptr, ctx);
      if (!ptr) return nullptr;
      if (is_valid(entry->value())) {
        UseKeyAndValueFromEntry();
      } else {
        WriteLengthDelimited(field_num, entry->SerializeAsString(),
                             metadata->mutable_unknown_fields<UnknownType>());
      }
      return ptr;
    }

    MapEntryImpl* NewEntry() { return entry_ = mf_->NewEntry(); }

    const Key& key() const { return key_; }
    const Value& value() const { return *value_ptr_; }

    const Key& entry_key() const { return entry_->key(); }
    const Value& entry_value() const { return entry_->value(); }

   private:
    void UseKeyAndValueFromEntry() {
      // Update key_ in case we need it later (because key() is called).
      // This is potentially inefficient, especially if the key is
      // expensive to copy (e.g., a long string), but this is a cold
      // path, so it's not a big deal.
      key_ = entry_->key();
      value_ptr_ = &(*map_)[key_];
      ValueMover::Move(entry_->mutable_value(), value_ptr_);
    }

    // After reading a key and value successfully, and inserting that data
    // into map_, we are not at the end of the input.  This is unusual, but
    // allowed by the spec.
    bool ReadBeyondKeyValuePair(io::CodedInputStream* input) PROTOBUF_COLD {
      NewEntry();
      ValueMover::Move(value_ptr_, entry_->mutable_value());
      map_->erase(key_);
      KeyMover::Move(&key_, entry_->mutable_key());
      const bool result = entry_->MergePartialFromCodedStream(input);
      if (result) UseKeyAndValueFromEntry();
      return result;
    }

    typedef MoveHelper<KeyTypeHandler::kIsEnum, KeyTypeHandler::kIsMessage,
                       KeyTypeHandler::kWireType ==
                           WireFormatLite::WIRETYPE_LENGTH_DELIMITED,
                       Key>
        KeyMover;
    typedef MoveHelper<ValueTypeHandler::kIsEnum, ValueTypeHandler::kIsMessage,
                       ValueTypeHandler::kWireType ==
                           WireFormatLite::WIRETYPE_LENGTH_DELIMITED,
                       Value>
        ValueMover;

    MapField* const mf_;
    Map* const map_;
    Key key_;
    Value* value_ptr_;
    MapEntryImpl* entry_ = nullptr;
  };

 protected:
  void set_has_key() { _has_bits_[0] |= 0x00000001u; }
  bool has_key() const { return (_has_bits_[0] & 0x00000001u) != 0; }
  void clear_has_key() { _has_bits_[0] &= ~0x00000001u; }
  void set_has_value() { _has_bits_[0] |= 0x00000002u; }
  bool has_value() const { return (_has_bits_[0] & 0x00000002u) != 0; }
  void clear_has_value() { _has_bits_[0] &= ~0x00000002u; }

 public:
  inline Arena* GetArena() const { return Base::GetArena(); }

 protected:  // Needed for constructing tables
  KeyOnMemory key_;
  ValueOnMemory value_;
  uint32_t _has_bits_[1];

 private:
  friend class google::protobuf::Arena;
  typedef void InternalArenaConstructable_;
  typedef void DestructorSkippable_;
  template <typename C, typename K, typename V, WireFormatLite::FieldType,
            WireFormatLite::FieldType>
  friend class google::protobuf::internal::MapEntry;
  template <typename C, typename K, typename V, WireFormatLite::FieldType,
            WireFormatLite::FieldType>
  friend class google::protobuf::internal::MapFieldLite;

  template <typename DerivedT, typename KeyT, typename TT,
            WireFormatLite::FieldType, WireFormatLite::FieldType>
  friend class google::protobuf::internal::MapField;
};

template <typename T, typename Key, typename Value,
          WireFormatLite::FieldType kKeyFieldType,
          WireFormatLite::FieldType kValueFieldType>
class MapEntryLite : public MapEntryImpl<T, MessageLite, Key, Value,
                                         kKeyFieldType, kValueFieldType> {
 public:
  typedef MapEntryImpl<T, MessageLite, Key, Value, kKeyFieldType,
                       kValueFieldType>
      SuperType;
  constexpr MapEntryLite() {}
  MapEntryLite(const MapEntryLite&) = delete;
  MapEntryLite& operator=(const MapEntryLite&) = delete;
  explicit MapEntryLite(Arena* arena) : SuperType(arena) {}
  ~MapEntryLite() override {
    MessageLite::_internal_metadata_.template Delete<std::string>();
  }
  void MergeFrom(const MapEntryLite& other) { MergeFromInternal(other); }
};

// Helpers for deterministic serialization =============================

// Iterator base for MapSorterFlat and MapSorterPtr.
template <typename storage_type>
struct MapSorterIt {
  storage_type* ptr;
  MapSorterIt(storage_type* ptr) : ptr(ptr) {}
  bool operator==(const MapSorterIt& other) const { return ptr == other.ptr; }
  bool operator!=(const MapSorterIt& other) const { return !(*this == other); }
  MapSorterIt& operator++() { ++ptr; return *this; }
  MapSorterIt operator++(int) { auto other = *this; ++ptr; return other; }
  MapSorterIt operator+(int v) { return MapSorterIt{ptr + v}; }
};

// Defined outside of MapSorterFlat to only be templatized on the key.
template <typename KeyT>
struct MapSorterLessThan {
  using storage_type = std::pair<KeyT, const void*>;
  bool operator()(const storage_type& a, const storage_type& b) const {
    return a.first < b.first;
  }
};

// MapSorterFlat stores keys inline with pointers to map entries, so that
// keys can be compared without indirection. This type is used for maps with
// keys that are not strings.
template <typename MapT>
class MapSorterFlat {
 public:
  using value_type = typename MapT::value_type;
  // To avoid code bloat we don't put `value_type` in `storage_type`. It is not
  // necessary for the call to sort, and avoiding it prevents unnecessary
  // separate instantiations of sort.
  using storage_type = std::pair<typename MapT::key_type, const void*>;

  // This const_iterator dereferenes to the map entry stored in the sorting
  // array pairs. This is the same interface as the Map::const_iterator type,
  // and allows generated code to use the same loop body with either form:
  //   for (const auto& entry : map) { ... }
  //   for (const auto& entry : MapSorterFlat(map)) { ... }
  struct const_iterator : public MapSorterIt<storage_type> {
    using pointer = const typename MapT::value_type*;
    using reference = const typename MapT::value_type&;
    using MapSorterIt<storage_type>::MapSorterIt;

    pointer operator->() const {
      return static_cast<const value_type*>(this->ptr->second);
    }
    reference operator*() const { return *this->operator->(); }
  };

  explicit MapSorterFlat(const MapT& m)
      : size_(m.size()), items_(size_ ? new storage_type[size_] : nullptr) {
    if (!size_) return;
    storage_type* it = &items_[0];
    for (const auto& entry : m) {
      *it++ = {entry.first, &entry};
    }
    std::sort(&items_[0], &items_[size_],
              MapSorterLessThan<typename MapT::key_type>{});
  }
  size_t size() const { return size_; }
  const_iterator begin() const { return {items_.get()}; }
  const_iterator end() const { return {items_.get() + size_}; }

 private:
  size_t size_;
  std::unique_ptr<storage_type[]> items_;
};

// Defined outside of MapSorterPtr to only be templatized on the key.
template <typename KeyT>
struct MapSorterPtrLessThan {
  bool operator()(const void* a, const void* b) const {
    // The pointers point to the `std::pair<const Key, Value>` object.
    // We cast directly to the key to read it.
    return *reinterpret_cast<const KeyT*>(a) <
           *reinterpret_cast<const KeyT*>(b);
  }
};

// MapSorterPtr stores and sorts pointers to map entries. This type is used for
// maps with keys that are strings.
template <typename MapT>
class MapSorterPtr {
 public:
  using value_type = typename MapT::value_type;
  // To avoid code bloat we don't put `value_type` in `storage_type`. It is not
  // necessary for the call to sort, and avoiding it prevents unnecessary
  // separate instantiations of sort.
  using storage_type = const void*;

  // This const_iterator dereferenes the map entry pointer stored in the sorting
  // array. This is the same interface as the Map::const_iterator type, and
  // allows generated code to use the same loop body with either form:
  //   for (const auto& entry : map) { ... }
  //   for (const auto& entry : MapSorterPtr(map)) { ... }
  struct const_iterator : public MapSorterIt<storage_type> {
    using pointer = const typename MapT::value_type*;
    using reference = const typename MapT::value_type&;
    using MapSorterIt<storage_type>::MapSorterIt;

    pointer operator->() const {
      return static_cast<const value_type*>(*this->ptr);
    }
    reference operator*() const { return *this->operator->(); }
  };

  explicit MapSorterPtr(const MapT& m)
      : size_(m.size()), items_(size_ ? new storage_type[size_] : nullptr) {
    if (!size_) return;
    storage_type* it = &items_[0];
    for (const auto& entry : m) {
      *it++ = &entry;
    }
    static_assert(PROTOBUF_FIELD_OFFSET(typename MapT::value_type, first) == 0,
                  "Must hold for MapSorterPtrLessThan to work.");
    std::sort(&items_[0], &items_[size_],
              MapSorterPtrLessThan<typename MapT::key_type>{});
  }
  size_t size() const { return size_; }
  const_iterator begin() const { return {items_.get()}; }
  const_iterator end() const { return {items_.get() + size_}; }

 private:
  size_t size_;
  std::unique_ptr<storage_type[]> items_;
};

}  // namespace internal
}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"

#endif  // GOOGLE_PROTOBUF_MAP_ENTRY_LITE_H__
