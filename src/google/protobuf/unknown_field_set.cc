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

#include <google/protobuf/unknown_field_set.h>
#include <google/protobuf/stubs/stl_util-inl.h>
#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/io/zero_copy_stream.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>
#include <google/protobuf/wire_format.h>

namespace google {
namespace protobuf {

UnknownFieldSet::UnknownFieldSet()
  : internal_(NULL) {}

UnknownFieldSet::~UnknownFieldSet() {
  if (internal_ != NULL) {
    STLDeleteValues(&internal_->fields_);
    delete internal_;
  }
}

void UnknownFieldSet::Clear() {
  if (internal_ == NULL) return;

  if (internal_->fields_.size() > kMaxInactiveFields) {
    STLDeleteValues(&internal_->fields_);
  } else {
    // Don't delete the UnknownField objects.  Just remove them from the active
    // set.
    for (int i = 0; i < internal_->active_fields_.size(); i++) {
      internal_->active_fields_[i]->Clear();
      internal_->active_fields_[i]->index_ = -1;
    }
  }

  internal_->active_fields_.clear();
}

void UnknownFieldSet::MergeFrom(const UnknownFieldSet& other) {
  for (int i = 0; i < other.field_count(); i++) {
    AddField(other.field(i).number())->MergeFrom(other.field(i));
  }
}

bool UnknownFieldSet::MergeFromCodedStream(io::CodedInputStream* input) {

  UnknownFieldSet other;
  if (internal::WireFormat::SkipMessage(input, &other) &&
                                  input->ConsumedEntireMessage()) {
    MergeFrom(other);
    return true;
  } else {
    return false;
  }
}

bool UnknownFieldSet::ParseFromCodedStream(io::CodedInputStream* input) {
  Clear();
  return MergeFromCodedStream(input);
}

bool UnknownFieldSet::ParseFromZeroCopyStream(io::ZeroCopyInputStream* input) {
  io::CodedInputStream coded_input(input);
  return ParseFromCodedStream(&coded_input) &&
    coded_input.ConsumedEntireMessage();
}

bool UnknownFieldSet::ParseFromArray(const void* data, int size) {
  io::ArrayInputStream input(data, size);
  return ParseFromZeroCopyStream(&input);
}

const UnknownField* UnknownFieldSet::FindFieldByNumber(int number) const {
  if (internal_ == NULL) return NULL;

  map<int, UnknownField*>::iterator iter = internal_->fields_.find(number);
  if (iter != internal_->fields_.end() && iter->second->index() != -1) {
    return iter->second;
  } else {
    return NULL;
  }
}

UnknownField* UnknownFieldSet::AddField(int number) {
  if (internal_ == NULL) internal_ = new Internal;

  UnknownField** map_slot = &internal_->fields_[number];
  if (*map_slot == NULL) {
    *map_slot = new UnknownField(number);
  }

  UnknownField* field = *map_slot;
  if (field->index() == -1) {
    field->index_ = internal_->active_fields_.size();
    internal_->active_fields_.push_back(field);
  }
  return field;
}

UnknownField::UnknownField(int number)
  : number_(number),
    index_(-1) {
}

UnknownField::~UnknownField() {
}

void UnknownField::Clear() {
  clear_varint();
  clear_fixed32();
  clear_fixed64();
  clear_length_delimited();
  clear_group();
}

void UnknownField::MergeFrom(const UnknownField& other) {
  varint_          .MergeFrom(other.varint_          );
  fixed32_         .MergeFrom(other.fixed32_         );
  fixed64_         .MergeFrom(other.fixed64_         );
  length_delimited_.MergeFrom(other.length_delimited_);
  group_           .MergeFrom(other.group_           );
}

}  // namespace protobuf
}  // namespace google
