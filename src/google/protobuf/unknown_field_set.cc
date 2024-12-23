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

void UnknownFieldSet::ClearFallback() {
  ABSL_DCHECK(!fields_.empty());
  if (arena() == nullptr) {
    int n = fields_.size();
    do {
      (fields_)[--n].Delete();
    } while (n > 0);
  }
  fields_.Clear();
}

void UnknownFieldSet::MergeFrom(const UnknownFieldSet& other) {
  int other_field_count = other.field_count();
  if (other_field_count > 0) {
    fields_.Reserve(fields_.size() + other_field_count);
    for (auto elem : other.fields_) {
      fields_.Add(elem.DeepCopy(arena()));
    }
  }
}

// A specialized MergeFrom for performance when we are merging from an UFS that
// is temporary and can be destroyed in the process.
void UnknownFieldSet::MergeFromAndDestroy(UnknownFieldSet* other) {
  if (arena() != other->arena()) {
    MergeFrom(*other);
  } else if (fields_.empty()) {
    fields_.Swap(&other->fields_);
  } else {
    fields_.MergeFrom(other->fields_);
    other->fields_.Clear();
  }
}

void UnknownFieldSet::MergeToInternalMetadata(
    const UnknownFieldSet& other, internal::InternalMetadata* metadata) {
  metadata->mutable_unknown_fields<UnknownFieldSet>()->MergeFrom(other);
}

size_t UnknownFieldSet::SpaceUsedExcludingSelfLong() const {
  if (fields_.empty()) return 0;

  size_t total_size = fields_.SpaceUsedExcludingSelfLong();

  for (const UnknownField& field : fields_) {
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

void UnknownFieldSet::AddVarint(int number, uint64_t value) {
  auto& field = *fields_.Add();
  field.number_ = number;
  field.SetType(UnknownField::TYPE_VARINT);
  field.data_.varint = value;
}

void UnknownFieldSet::AddFixed32(int number, uint32_t value) {
  auto& field = *fields_.Add();
  field.number_ = number;
  field.SetType(UnknownField::TYPE_FIXED32);
  field.data_.fixed32 = value;
}

void UnknownFieldSet::AddFixed64(int number, uint64_t value) {
  auto& field = *fields_.Add();
  field.number_ = number;
  field.SetType(UnknownField::TYPE_FIXED64);
  field.data_.fixed64 = value;
}

void UnknownFieldSet::AddLengthDelimited(int number, const absl::Cord& value) {
  absl::CopyCordToString(value, AddLengthDelimited(number));
}

template <int&...>
void UnknownFieldSet::AddLengthDelimited(int number, std::string&& value) {
  auto& field = *fields_.Add();
  field.number_ = number;
  field.SetType(UnknownField::TYPE_LENGTH_DELIMITED);
  field.data_.string_value =
      Arena::Create<std::string>(arena(), std::move(value));
}
template void UnknownFieldSet::AddLengthDelimited(int, std::string&&);

std::string* UnknownFieldSet::AddLengthDelimited(int number) {
  auto& field = *fields_.Add();
  field.number_ = number;
  field.SetType(UnknownField::TYPE_LENGTH_DELIMITED);
  field.data_.string_value = Arena::Create<std::string>(arena());
  return field.data_.string_value;
}

UnknownFieldSet* UnknownFieldSet::AddGroup(int number) {
  auto& field = *fields_.Add();
  field.number_ = number;
  field.SetType(UnknownField::TYPE_GROUP);
  field.data_.group = Arena::Create<UnknownFieldSet>(arena());
  return field.data_.group;
}

void UnknownFieldSet::AddField(const UnknownField& field) {
  fields_.Add(field.DeepCopy(arena()));
}

void UnknownFieldSet::DeleteSubrange(int start, int num) {
  if (arena() == nullptr) {
    // Delete the specified fields.
    for (int i = 0; i < num; ++i) {
      (fields_)[i + start].Delete();
    }
  }
  fields_.ExtractSubrange(start, num, nullptr);
}

void UnknownFieldSet::DeleteByNumber(int number) {
  int left = 0;  // The number of fields left after deletion.
  for (int i = 0; i < fields_.size(); ++i) {
    UnknownField* field = &(fields_)[i];
    if (field->number() == number) {
      if (arena() == nullptr) {
        field->Delete();
      }
    } else {
      if (i != left) {
        (fields_)[left] = (fields_)[i];
      }
      ++left;
    }
  }
  fields_.Truncate(left);
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
  google::protobuf::internal::WireFormat::SerializeUnknownFieldsToArray(
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

void UnknownField::Delete() {
  switch (type()) {
    case UnknownField::TYPE_LENGTH_DELIMITED:
      delete data_.string_value;
      break;
    case UnknownField::TYPE_GROUP:
      delete data_.group;
      break;
    default:
      break;
  }
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

namespace internal {

class UnknownFieldParserHelper {
 public:
  explicit UnknownFieldParserHelper(UnknownFieldSet* unknown)
      : unknown_(unknown) {}

  void AddVarint(uint32_t num, uint64_t value) {
    unknown_->AddVarint(num, value);
  }
  void AddFixed64(uint32_t num, uint64_t value) {
    unknown_->AddFixed64(num, value);
  }
  const char* ParseLengthDelimited(uint32_t num, const char* ptr,
                                   ParseContext* ctx) {
    std::string* s = unknown_->AddLengthDelimited(num);
    int size = ReadSize(&ptr);
    GOOGLE_PROTOBUF_PARSER_ASSERT(ptr);
    return ctx->ReadString(ptr, size, s);
  }
  const char* ParseGroup(uint32_t num, const char* ptr, ParseContext* ctx) {
    return ctx->ParseGroupInlined(ptr, num * 8 + 3, [&](const char* ptr) {
      UnknownFieldParserHelper child(unknown_->AddGroup(num));
      return WireFormatParser(child, ptr, ctx);
    });
  }
  void AddFixed32(uint32_t num, uint32_t value) {
    unknown_->AddFixed32(num, value);
  }

 private:
  UnknownFieldSet* unknown_;
};

const char* UnknownGroupParse(UnknownFieldSet* unknown, const char* ptr,
                              ParseContext* ctx) {
  UnknownFieldParserHelper field_parser(unknown);
  return WireFormatParser(field_parser, ptr, ctx);
}

const char* UnknownFieldParse(uint64_t tag, UnknownFieldSet* unknown,
                              const char* ptr, ParseContext* ctx) {
  UnknownFieldParserHelper field_parser(unknown);
  return FieldParser(tag, field_parser, ptr, ctx);
}

}  // namespace internal
}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"
