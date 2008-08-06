// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.
// http://code.google.com/p/protobuf/
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

// Author: kenton@google.com (Kenton Varda)
//  Based on original Protocol Buffers design by
//  Sanjay Ghemawat, Jeff Dean, and others.
//
// Contains classes used to keep track of unrecognized fields seen while
// parsing a protocol message.

#ifndef GOOGLE_PROTOBUF_UNKNOWN_FIELD_SET_H__
#define GOOGLE_PROTOBUF_UNKNOWN_FIELD_SET_H__

#include <string>
#include <map>
#include <vector>
#include <google/protobuf/repeated_field.h>

namespace google {
namespace protobuf {

class Message;                      // message.h
class UnknownField;                 // below

// An UnknownFieldSet contains fields that were encountered while parsing a
// message but were not defined by its type.  Keeping track of these can be
// useful, especially in that they may be written if the message is serialized
// again without being cleared in between.  This means that software which
// simply receives messages and forwards them to other servers does not need
// to be updated every time a new field is added to the message definition.
//
// To get the UnknownFieldSet attached to any message, call
// Message::Reflection::GetUnknownFields().
//
// This class is necessarily tied to the protocol buffer wire format, unlike
// the Reflection interface which is independent of any serialization scheme.
class LIBPROTOBUF_EXPORT UnknownFieldSet {
 public:
  UnknownFieldSet();
  ~UnknownFieldSet();

  // Remove all fields.
  void Clear();

  // Is this set empty?
  inline bool empty() const;

  // Merge the contents of some other UnknownFieldSet with this one.
  void MergeFrom(const UnknownFieldSet& other);

  // Returns the number of fields present in the UnknownFieldSet.
  inline int field_count() const;
  // Get a field in the set, where 0 <= index < field_count().  The fields
  // appear in arbitrary order.
  inline const UnknownField& field(int index) const;
  // Get a mutable pointer to a field in the set, where
  // 0 <= index < field_count().  The fields appear in arbitrary order.
  inline UnknownField* mutable_field(int index);

  // Find a field by field number.  Returns NULL if not found.
  const UnknownField* FindFieldByNumber(int number) const;

  // Add a field by field number.  If the field number already exists, returns
  // the existing UnknownField.
  UnknownField* AddField(int number);

  // Parsing helpers -------------------------------------------------
  // These work exactly like the similarly-named methods of Message.

  bool MergeFromCodedStream(io::CodedInputStream* input);
  bool ParseFromCodedStream(io::CodedInputStream* input);
  bool ParseFromZeroCopyStream(io::ZeroCopyInputStream* input);
  bool ParseFromArray(const void* data, int size);
  inline bool ParseFromString(const string& data) {
    return ParseFromArray(data.data(), data.size());
  }

 private:
  // "Active" fields are ones which have been added since the last time Clear()
  // was called.  Inactive fields are objects we are keeping around incase
  // they become active again.

  struct Internal {
    // Contains all UnknownFields that have been allocated for this
    // UnknownFieldSet, including ones not currently active.  Keyed by
    // field number.  We intentionally try to reuse UnknownField objects for
    // the same field number they were used for originally because this makes
    // it more likely that the previously-allocated memory will have the right
    // layout.
    map<int, UnknownField*> fields_;

    // Contains the fields from fields_ that are currently active.
    vector<UnknownField*> active_fields_;
  };

  // We want an UnknownFieldSet to use no more space than a single pointer
  // until the first field is added.
  Internal* internal_;

  // Don't keep more inactive fields than this.
  static const int kMaxInactiveFields = 100;

  GOOGLE_DISALLOW_EVIL_CONSTRUCTORS(UnknownFieldSet);
};

// Represents one field in an UnknownFieldSet.
//
// UnknownFiled's accessors are similar to those that would be produced by the
// protocol compiler for the fields:
//   repeated uint64 varint;
//   repeated fixed32 fixed32;
//   repeated fixed64 fixed64;
//   repeated bytes length_delimited;
//   repeated UnknownFieldSet group;
// (OK, so the last one isn't actually a valid field type but you get the
// idea.)
class LIBPROTOBUF_EXPORT UnknownField {
 public:
  ~UnknownField();

  // Clears all fields.
  void Clear();

  // Merge the contents of some other UnknownField with this one.  For each
  // wire type, the values are simply concatenated.
  void MergeFrom(const UnknownField& other);

  // The field's tag number, as seen on the wire.
  inline int number() const;

  // The index of this UnknownField within the UknownFieldSet (e.g.
  // set.field(field.index()) == field).
  inline int index() const;

  inline int varint_size          () const;
  inline int fixed32_size         () const;
  inline int fixed64_size         () const;
  inline int length_delimited_size() const;
  inline int group_size           () const;

  inline uint64 varint (int index) const;
  inline uint32 fixed32(int index) const;
  inline uint64 fixed64(int index) const;
  inline const string& length_delimited(int index) const;
  inline const UnknownFieldSet& group(int index) const;

  inline void set_varint (int index, uint64 value);
  inline void set_fixed32(int index, uint32 value);
  inline void set_fixed64(int index, uint64 value);
  inline void set_length_delimited(int index, const string& value);
  inline string* mutable_length_delimited(int index);
  inline UnknownFieldSet* mutable_group(int index);

  inline void add_varint (uint64 value);
  inline void add_fixed32(uint32 value);
  inline void add_fixed64(uint64 value);
  inline void add_length_delimited(const string& value);
  inline string* add_length_delimited();
  inline UnknownFieldSet* add_group();

  inline void clear_varint ();
  inline void clear_fixed32();
  inline void clear_fixed64();
  inline void clear_length_delimited();
  inline void clear_group();

  inline const RepeatedField   <uint64         >& varint          () const;
  inline const RepeatedField   <uint32         >& fixed32         () const;
  inline const RepeatedField   <uint64         >& fixed64         () const;
  inline const RepeatedPtrField<string         >& length_delimited() const;
  inline const RepeatedPtrField<UnknownFieldSet>& group           () const;

  inline RepeatedField   <uint64         >* mutable_varint          ();
  inline RepeatedField   <uint32         >* mutable_fixed32         ();
  inline RepeatedField   <uint64         >* mutable_fixed64         ();
  inline RepeatedPtrField<string         >* mutable_length_delimited();
  inline RepeatedPtrField<UnknownFieldSet>* mutable_group           ();

 private:
  friend class UnknownFieldSet;
  UnknownField(int number);

  int number_;
  int index_;

  RepeatedField   <uint64         > varint_;
  RepeatedField   <uint32         > fixed32_;
  RepeatedField   <uint64         > fixed64_;
  RepeatedPtrField<string         > length_delimited_;
  RepeatedPtrField<UnknownFieldSet> group_;

  GOOGLE_DISALLOW_EVIL_CONSTRUCTORS(UnknownField);
};

// ===================================================================
// inline implementations

inline bool UnknownFieldSet::empty() const {
  return internal_ == NULL || internal_->active_fields_.empty();
}

inline int UnknownFieldSet::field_count() const {
  return (internal_ == NULL) ? 0 : internal_->active_fields_.size();
}
inline const UnknownField& UnknownFieldSet::field(int index) const {
  return *(internal_->active_fields_[index]);
}
inline UnknownField* UnknownFieldSet::mutable_field(int index) {
  return internal_->active_fields_[index];
}

inline int UnknownField::number() const { return number_; }
inline int UnknownField::index () const { return index_; }

inline int UnknownField::varint_size          () const {return varint_.size();}
inline int UnknownField::fixed32_size         () const {return fixed32_.size();}
inline int UnknownField::fixed64_size         () const {return fixed64_.size();}
inline int UnknownField::length_delimited_size() const {
  return length_delimited_.size();
}
inline int UnknownField::group_size           () const {return group_.size();}

inline uint64 UnknownField::varint (int index) const {
  return varint_.Get(index);
}
inline uint32 UnknownField::fixed32(int index) const {
  return fixed32_.Get(index);
}
inline uint64 UnknownField::fixed64(int index) const {
  return fixed64_.Get(index);
}
inline const string& UnknownField::length_delimited(int index) const {
  return length_delimited_.Get(index);
}
inline const UnknownFieldSet& UnknownField::group(int index) const {
  return group_.Get(index);
}

inline void UnknownField::set_varint (int index, uint64 value) {
  varint_.Set(index, value);
}
inline void UnknownField::set_fixed32(int index, uint32 value) {
  fixed32_.Set(index, value);
}
inline void UnknownField::set_fixed64(int index, uint64 value) {
  fixed64_.Set(index, value);
}
inline void UnknownField::set_length_delimited(int index, const string& value) {
  length_delimited_.Mutable(index)->assign(value);
}
inline string* UnknownField::mutable_length_delimited(int index) {
  return length_delimited_.Mutable(index);
}
inline UnknownFieldSet* UnknownField::mutable_group(int index) {
  return group_.Mutable(index);
}

inline void UnknownField::add_varint (uint64 value) {
  varint_.Add(value);
}
inline void UnknownField::add_fixed32(uint32 value) {
  fixed32_.Add(value);
}
inline void UnknownField::add_fixed64(uint64 value) {
  fixed64_.Add(value);
}
inline void UnknownField::add_length_delimited(const string& value) {
  length_delimited_.Add()->assign(value);
}
inline string* UnknownField::add_length_delimited() {
  return length_delimited_.Add();
}
inline UnknownFieldSet* UnknownField::add_group() {
  return group_.Add();
}

inline void UnknownField::clear_varint () { varint_.Clear(); }
inline void UnknownField::clear_fixed32() { varint_.Clear(); }
inline void UnknownField::clear_fixed64() { varint_.Clear(); }
inline void UnknownField::clear_length_delimited() {
  length_delimited_.Clear();
}
inline void UnknownField::clear_group() { group_.Clear(); }

inline const RepeatedField<uint64>& UnknownField::varint () const {
  return varint_;
}
inline const RepeatedField<uint32>& UnknownField::fixed32() const {
  return fixed32_;
}
inline const RepeatedField<uint64>& UnknownField::fixed64() const {
  return fixed64_;
}
inline const RepeatedPtrField<string>& UnknownField::length_delimited() const {
  return length_delimited_;
}
inline const RepeatedPtrField<UnknownFieldSet>& UnknownField::group() const {
  return group_;
}

inline RepeatedField<uint64>* UnknownField::mutable_varint () {
  return &varint_;
}
inline RepeatedField<uint32>* UnknownField::mutable_fixed32() {
  return &fixed32_;
}
inline RepeatedField<uint64>* UnknownField::mutable_fixed64() {
  return &fixed64_;
}
inline RepeatedPtrField<string>* UnknownField::mutable_length_delimited() {
  return &length_delimited_;
}
inline RepeatedPtrField<UnknownFieldSet>* UnknownField::mutable_group() {
  return &group_;
}

}  // namespace protobuf

}  // namespace google
#endif  // GOOGLE_PROTOBUF_UNKNOWN_FIELD_SET_H__
