// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// Author: kenton@google.com (Kenton Varda)
//  Based on original Protocol Buffers design by
//  Sanjay Ghemawat, Jeff Dean, and others.

#include "google/protobuf/unknown_field_set.h"

#include <cstdint>
#include <cstring>
#include <string>
#include <utility>

#include "absl/log/absl_check.h"
#include "absl/strings/cord.h"
#include "absl/strings/internal/resize_uninitialized.h"
#include "absl/strings/string_view.h"
#include "google/protobuf/extension_set.h"
#include "google/protobuf/generated_message_tctable_impl.h"
#include "google/protobuf/io/coded_stream.h"
#include "google/protobuf/io/zero_copy_stream.h"
#include "google/protobuf/io/zero_copy_stream_impl_lite.h"
#include "google/protobuf/message_lite.h"
#include "google/protobuf/parse_context.h"
#include "google/protobuf/wire_format.h"

// Must be included last.
#include "google/protobuf/port_def.inc"

namespace google {
namespace protobuf {

void UnknownFieldSet::MergeFrom(const UnknownFieldSet& other) {
  int other_field_count = other.field_count();
  if (other_field_count > 0) {
    auto& fields = this->fields();
    fields.Reserve(fields.size() + other_field_count);
    for (auto elem : other.fields()) {
      fields.Add(elem.DeepCopy(arena()));
    }
  }
}

// A specialized MergeFrom for performance when we are merging from an UFS that
// is temporary and can be destroyed in the process.
void UnknownFieldSet::MergeFromAndDestroy(UnknownFieldSet* other) {
  if (arena() != other->arena()) {
    MergeFrom(*other);
    return;
  }

  auto& fields = this->fields();
  auto& other_fields = other->fields();
  if (fields.empty()) {
    fields.Swap(&other_fields);
  } else {
    fields.MergeFrom(other_fields);
    other_fields.Clear();
  }
}

void UnknownFieldSet::MergeToInternalMetadata(
    const UnknownFieldSet& other, internal::InternalMetadata* metadata) {
  metadata->mutable_unknown_fields<UnknownFieldSet>()->MergeFrom(other);
}

size_t UnknownFieldSet::SpaceUsedExcludingSelfLong() const {
  auto& fields = this->fields();
  if (fields.empty()) return 0;

  size_t total_size = fields.SpaceUsedExcludingSelfLong();

  for (const UnknownField& field : fields) {
    switch (field.type()) {
      case UnknownField::TYPE_LENGTH_DELIMITED:
        total_size += sizeof(*field.data_.string_value) +
                      internal::StringSpaceUsedExcludingSelfLong(
                          *field.data_.string_value);
        break;
      case UnknownField::TYPE_GROUP:
        total_size += field.data_.group->SpaceUsedLong();
        break;
      default:
        break;
    }
  }
  return total_size;
}

size_t UnknownFieldSet::SpaceUsedLong() const {
  return sizeof(*this) + SpaceUsedExcludingSelf();
}



void UnknownFieldSet::AddLengthDelimited(int number, const absl::Cord& value) {
  absl::CopyCordToString(value, AddLengthDelimited(number));
}

template <int&...>
void UnknownFieldSet::AddLengthDelimited(int number, std::string&& value) {
  auto& field = *fields().Add();
  field.number_ = number;
  field.SetType(UnknownField::TYPE_LENGTH_DELIMITED);
  field.data_.string_value =
      Arena::Create<std::string>(arena(), std::move(value));
}
template void UnknownFieldSet::AddLengthDelimited(int, std::string&&);



void UnknownFieldSet::AddField(const UnknownField& field) {
  fields().Add(field.DeepCopy(arena()));
}

void UnknownFieldSet::DeleteSubrange(int start, int num) {
  auto& fields = this->fields();
  if (arena() == nullptr) {
    // Delete the specified fields.
    for (int i = 0; i < num; ++i) {
      fields[i + start].Delete();
    }
  }
  fields.ExtractSubrange(start, num, nullptr);
}

void UnknownFieldSet::DeleteByNumber(int number) {
  auto& fields = this->fields();
  int left = 0;  // The number of fields left after deletion.
  for (int i = 0; i < fields.size(); ++i) {
    UnknownField* field = &fields[i];
    if (field->number() == number) {
      if (arena() == nullptr) {
        field->Delete();
      }
    } else {
      if (i != left) {
        fields[left] = fields[i];
      }
      ++left;
    }
  }
  fields.Truncate(left);
}

bool UnknownFieldSet::MergeFromCodedStream(io::CodedInputStream* input) {
  UnknownFieldSet other;
  if (internal::WireFormat::SkipMessage(input, &other) &&
      input->ConsumedEntireMessage()) {
    MergeFromAndDestroy(&other);
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
  return (ParseFromCodedStream(&coded_input) &&
          coded_input.ConsumedEntireMessage());
}

bool UnknownFieldSet::ParseFromArray(const void* data, int size) {
  io::ArrayInputStream input(data, size);
  return ParseFromZeroCopyStream(&input);
}

bool UnknownFieldSet::SerializeToString(std::string* output) const {
  const size_t size =
      google::protobuf::internal::WireFormat::ComputeUnknownFieldsSize(*this);
  absl::strings_internal::STLStringResizeUninitializedAmortized(output, size);
  // TODO: Remove this suppression.
  (void)google::protobuf::internal::WireFormat::SerializeUnknownFieldsToArray(
      *this, reinterpret_cast<uint8_t*>(const_cast<char*>(output->data())));
  return true;
}

bool UnknownFieldSet::SerializeToCodedStream(
    io::CodedOutputStream* output) const {
  google::protobuf::internal::WireFormat::SerializeUnknownFields(*this, output);
  return !output->HadError();
}

bool UnknownFieldSet::SerializeToCord(absl::Cord* output) const {
  const size_t size =
      google::protobuf::internal::WireFormat::ComputeUnknownFieldsSize(*this);
  io::CordOutputStream cord_output_stream(size);
  {
    io::CodedOutputStream coded_output_stream(&cord_output_stream);
    if (!SerializeToCodedStream(&coded_output_stream)) return false;
  }
  *output = cord_output_stream.Consume();
  return true;
}

UnknownField UnknownField::DeepCopy(Arena* arena) const {
  UnknownField copy = *this;
  switch (type()) {
    case UnknownField::TYPE_LENGTH_DELIMITED:
      copy.data_.string_value =
          Arena::Create<std::string>(arena, *data_.string_value);
      break;
    case UnknownField::TYPE_GROUP: {
      UnknownFieldSet* group = Arena::Create<UnknownFieldSet>(arena);
      group->MergeFrom(*data_.group);
      copy.data_.group = group;
      break;
    }
    default:
      break;
  }
  return copy;
}

void UnknownFieldSet::SwapSlow(UnknownFieldSet* other) {
  UnknownFieldSet tmp;
  tmp.MergeFrom(*this);
  this->Clear();
  this->MergeFrom(*other);
  other->Clear();
  other->MergeFrom(tmp);
}

uint8_t* UnknownField::InternalSerializeLengthDelimitedNoTag(
    uint8_t* target, io::EpsCopyOutputStream* stream) const {
  ABSL_DCHECK_EQ(TYPE_LENGTH_DELIMITED, type());
  const absl::string_view data = *data_.string_value;
  target = io::CodedOutputStream::WriteVarint32ToArray(data.size(), target);
  target = stream->WriteRaw(data.data(), data.size(), target);
  return target;
}


}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"
