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

#include "google/protobuf/json/internal/untyped_message.h"

#include <algorithm>
#include <cfloat>
#include <cstdint>
#include <memory>
#include <sstream>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include "google/protobuf/type.pb.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/dynamic_message.h"
#include "google/protobuf/message.h"
#include "google/protobuf/wire_format.h"
#include "google/protobuf/wire_format_lite.h"
#include "absl/container/flat_hash_map.h"
#include "absl/status/status.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_format.h"
#include "absl/strings/string_view.h"
#include "absl/types/optional.h"
#include "absl/types/span.h"
#include "absl/types/variant.h"
#include "google/protobuf/io/coded_stream.h"
#include "google/protobuf/util/type_resolver.h"
#include "google/protobuf/stubs/status_macros.h"


// Must be included last.
#include "google/protobuf/port_def.inc"

namespace google {
namespace protobuf {
namespace json_internal {
using ::google::protobuf::Field;
using ::google::protobuf::internal::WireFormatLite;

absl::StatusOr<const ResolverPool::Message*> ResolverPool::Field::MessageType()
    const {
  GOOGLE_CHECK(proto().kind() == google::protobuf::Field::TYPE_MESSAGE ||
        proto().kind() == google::protobuf::Field::TYPE_GROUP)
      << proto().kind();
  if (type_ == nullptr) {
    auto type = pool_->FindMessage(proto().type_url());
    RETURN_IF_ERROR(type.status());
    type_ = *type;
  }
  return reinterpret_cast<const Message*>(type_);
}

absl::StatusOr<const ResolverPool::Enum*> ResolverPool::Field::EnumType()
    const {
  GOOGLE_CHECK(proto().kind() == google::protobuf::Field::TYPE_ENUM) << proto().kind();
  if (type_ == nullptr) {
    auto type = pool_->FindEnum(proto().type_url());
    RETURN_IF_ERROR(type.status());
    type_ = *type;
  }
  return reinterpret_cast<const Enum*>(type_);
}

absl::Span<const ResolverPool::Field> ResolverPool::Message::FieldsByIndex()
    const {
  if (raw_.fields_size() > 0 && fields_ == nullptr) {
    fields_ = std::unique_ptr<Field[]>(new Field[raw_.fields_size()]);
    for (size_t i = 0; i < raw_.fields_size(); ++i) {
      fields_[i].pool_ = pool_;
      fields_[i].raw_ = &raw_.fields(i);
      fields_[i].parent_ = this;
    }
  }

  return absl::MakeSpan(fields_.get(), proto().fields_size());
}

const ResolverPool::Field* ResolverPool::Message::FindField(
    absl::string_view name) const {
  if (raw_.fields_size() == 0) {
    return nullptr;
  }

  if (fields_by_name_.empty()) {
    const Field* found = nullptr;
    for (auto& field : FieldsByIndex()) {
      if (field.proto().name() == name || field.proto().json_name() == name) {
        found = &field;
      }
      fields_by_name_.try_emplace(field.proto().name(), &field);
      fields_by_name_.try_emplace(field.proto().json_name(), &field);
    }
    return found;
  }

  auto it = fields_by_name_.find(name);
  return it == fields_by_name_.end() ? nullptr : it->second;
}

const ResolverPool::Field* ResolverPool::Message::FindField(
    int32_t number) const {
  if (raw_.fields_size() == 0) {
    return nullptr;
  }

  bool is_small = raw_.fields_size() < 8;
  if (is_small || fields_by_number_.empty()) {
    const Field* found = nullptr;
    for (auto& field : FieldsByIndex()) {
      if (field.proto().number() == number) {
        found = &field;
      }
      if (!is_small) {
        fields_by_number_.try_emplace(field.proto().number(), &field);
      }
    }
    return found;
  }

  auto it = fields_by_number_.find(number);
  return it == fields_by_number_.end() ? nullptr : it->second;
}

absl::StatusOr<const ResolverPool::Message*> ResolverPool::FindMessage(
    absl::string_view url) {
  auto it = messages_.find(url);
  if (it != messages_.end()) {
    return it->second.get();
  }

  auto msg = absl::WrapUnique(new Message(this));
  std::string url_buf(url);
  RETURN_IF_ERROR(resolver_->ResolveMessageType(url_buf, &msg->raw_));

  return messages_.try_emplace(std::move(url_buf), std::move(msg))
      .first->second.get();
}

absl::StatusOr<const ResolverPool::Enum*> ResolverPool::FindEnum(
    absl::string_view url) {
  auto it = enums_.find(url);
  if (it != enums_.end()) {
    return it->second.get();
  }

  auto enoom = absl::WrapUnique(new Enum(this));
  std::string url_buf(url);
  RETURN_IF_ERROR(resolver_->ResolveEnumType(url_buf, &enoom->raw_));

  return enums_.try_emplace(std::move(url_buf), std::move(enoom))
      .first->second.get();
}

absl::Status UntypedMessage::Decode(io::CodedInputStream& stream,
                                    absl::optional<int32_t> current_group) {
  while (true) {
    std::vector<int32_t> group_stack;
    uint32_t tag = stream.ReadTag();
    if (tag == 0) {
      return absl::OkStatus();
    }

    int32_t field_number = tag >> 3;
    int32_t wire_type = tag & 7;

    // EGROUP markers can show up as "unknown fields", so we need to handle them
    // before we even do field lookup. Being inside of a group behaves as if a
    // special field has been added to the message.
    if (wire_type == WireFormatLite::WIRETYPE_END_GROUP) {
      if (!current_group.has_value()) {
        return absl::InvalidArgumentError(absl::StrFormat(
            "attempted to close group %d before SGROUP tag", field_number));
      }
      if (field_number != *current_group) {
        return absl::InvalidArgumentError(
            absl::StrFormat("attempted to close group %d while inside group %d",
                            field_number, *current_group));
      }
      return absl::OkStatus();
    }

    const auto* field = desc_->FindField(field_number);
    if (!group_stack.empty() || field == nullptr) {
      // Skip unknown field. If the group-stack is non-empty, we are in the
      // process of working through an unknown group.
      switch (wire_type) {
        case WireFormatLite::WIRETYPE_VARINT: {
          uint64_t x;
          if (!stream.ReadVarint64(&x)) {
            return absl::InvalidArgumentError("unexpected EOF");
          }
          continue;
        }
        case WireFormatLite::WIRETYPE_FIXED64: {
          uint64_t x;
          if (!stream.ReadLittleEndian64(&x)) {
            return absl::InvalidArgumentError("unexpected EOF");
          }
          continue;
        }
        case WireFormatLite::WIRETYPE_FIXED32: {
          uint32_t x;
          if (!stream.ReadLittleEndian32(&x)) {
            return absl::InvalidArgumentError("unexpected EOF");
          }
          continue;
        }
        case WireFormatLite::WIRETYPE_LENGTH_DELIMITED: {
          uint32_t x;
          if (!stream.ReadVarint32(&x)) {
            return absl::InvalidArgumentError("unexpected EOF");
          }
          stream.Skip(x);
          continue;
        }
        case WireFormatLite::WIRETYPE_START_GROUP: {
          group_stack.push_back(field_number);
          continue;
        }
        case WireFormatLite::WIRETYPE_END_GROUP: {
          if (group_stack.empty()) {
            return absl::InvalidArgumentError(absl::StrFormat(
                "attempted to close group %d before SGROUP tag", field_number));
          }
          if (field_number != group_stack.back()) {
            return absl::InvalidArgumentError(absl::StrFormat(
                "attempted to close group %d while inside group %d",
                field_number, *current_group));
          }
          group_stack.pop_back();
          continue;
        }
        default:
          return absl::InvalidArgumentError(
              absl::StrCat("unknown wire type: ", wire_type));
      }
    }
    switch (wire_type) {
      case WireFormatLite::WIRETYPE_VARINT:
        RETURN_IF_ERROR(DecodeVarint(stream, *field));
        break;
      case WireFormatLite::WIRETYPE_FIXED64:
        RETURN_IF_ERROR(Decode64Bit(stream, *field));
        break;
      case WireFormatLite::WIRETYPE_FIXED32:
        RETURN_IF_ERROR(Decode32Bit(stream, *field));
        break;
      case WireFormatLite::WIRETYPE_LENGTH_DELIMITED:
        RETURN_IF_ERROR(DecodeDelimited(stream, *field));
        break;
      case WireFormatLite::WIRETYPE_START_GROUP: {
        if (field->proto().kind() != Field::TYPE_GROUP) {
          return absl::InvalidArgumentError(absl::StrFormat(
              "field number %d is not a group", field->proto().number()));
        }
        auto group_desc = field->MessageType();
        RETURN_IF_ERROR(group_desc.status());

        UntypedMessage group(*group_desc);
        RETURN_IF_ERROR(group.Decode(stream, field_number));
        RETURN_IF_ERROR(InsertField<UntypedMessage>(*field, std::move(group)));
        break;
      }
      case WireFormatLite::WIRETYPE_END_GROUP:
        GOOGLE_CHECK(false) << "unreachable";
        break;
      default:
        return absl::InvalidArgumentError(
            absl::StrCat("unknown wire type: ", wire_type));
    }
  }

  return absl::OkStatus();
}

absl::Status UntypedMessage::DecodeVarint(io::CodedInputStream& stream,
                                          const ResolverPool::Field& field) {
  switch (field.proto().kind()) {
    case Field::TYPE_BOOL: {
      char byte;
      if (!stream.ReadRaw(&byte, 1)) {
        return absl::InvalidArgumentError("unexpected EOF");
      }
      switch (byte) {
        case 0:
          RETURN_IF_ERROR(InsertField(field, kFalse));
          break;
        case 1:
          RETURN_IF_ERROR(InsertField(field, kTrue));
          break;
        default:
          return absl::InvalidArgumentError(
              absl::StrFormat("bad value for bool: \\x%02x", byte));
      }
      break;
    }
    case Field::TYPE_INT32:
    case Field::TYPE_SINT32:
    case Field::TYPE_UINT32:
    case Field::TYPE_ENUM: {
      uint32_t x;
      if (!stream.ReadVarint32(&x)) {
        return absl::InvalidArgumentError("unexpected EOF");
      }
      if (field.proto().kind() == Field::TYPE_UINT32) {
        RETURN_IF_ERROR(InsertField(field, x));
        break;
      }
      if (field.proto().kind() == Field::TYPE_SINT32) {
        x = WireFormatLite::ZigZagDecode32(x);
      }
      RETURN_IF_ERROR(InsertField(field, static_cast<int32_t>(x)));
      break;
    }
    case Field::TYPE_INT64:
    case Field::TYPE_SINT64:
    case Field::TYPE_UINT64: {
      uint64_t x;
      if (!stream.ReadVarint64(&x)) {
        return absl::InvalidArgumentError("unexpected EOF");
      }
      if (field.proto().kind() == Field::TYPE_UINT64) {
        RETURN_IF_ERROR(InsertField(field, x));
        break;
      }
      if (field.proto().kind() == Field::TYPE_SINT64) {
        x = WireFormatLite::ZigZagDecode64(x);
      }
      RETURN_IF_ERROR(InsertField(field, static_cast<int64_t>(x)));
      break;
    }
    default:
      return absl::InvalidArgumentError(absl::StrFormat(
          "field type %d (number %d) does not support varint fields",
          field.proto().kind(), field.proto().number()));
  }
  return absl::OkStatus();
}

absl::Status UntypedMessage::Decode64Bit(io::CodedInputStream& stream,
                                         const ResolverPool::Field& field) {
  switch (field.proto().kind()) {
    case Field::TYPE_FIXED64: {
      uint64_t x;
      if (!stream.ReadLittleEndian64(&x)) {
        return absl::InvalidArgumentError("unexpected EOF");
      }
      RETURN_IF_ERROR(InsertField(field, x));
      break;
    }
    case Field::TYPE_SFIXED64: {
      uint64_t x;
      if (!stream.ReadLittleEndian64(&x)) {
        return absl::InvalidArgumentError("unexpected EOF");
      }
      RETURN_IF_ERROR(InsertField(field, static_cast<int64_t>(x)));
      break;
    }
    case Field::TYPE_DOUBLE: {
      uint64_t x;
      if (!stream.ReadLittleEndian64(&x)) {
        return absl::InvalidArgumentError("unexpected EOF");
      }
      RETURN_IF_ERROR(InsertField(field, absl::bit_cast<double>(x)));
      break;
    }
    default:
      return absl::InvalidArgumentError(
          absl::StrFormat("field type %d (number %d) does not support "
                          "type 64-bit fields",
                          field.proto().kind(), field.proto().number()));
  }
  return absl::OkStatus();
}

absl::Status UntypedMessage::Decode32Bit(io::CodedInputStream& stream,
                                         const ResolverPool::Field& field) {
  switch (field.proto().kind()) {
    case Field::TYPE_FIXED32: {
      uint32_t x;
      if (!stream.ReadLittleEndian32(&x)) {
        return absl::InvalidArgumentError("unexpected EOF");
      }
      RETURN_IF_ERROR(InsertField(field, x));
      break;
    }
    case Field::TYPE_SFIXED32: {
      uint32_t x;
      if (!stream.ReadLittleEndian32(&x)) {
        return absl::InvalidArgumentError("unexpected EOF");
      }
      RETURN_IF_ERROR(InsertField(field, static_cast<int32_t>(x)));
      break;
    }
    case Field::TYPE_FLOAT: {
      uint32_t x;
      if (!stream.ReadLittleEndian32(&x)) {
        return absl::InvalidArgumentError("unexpected EOF");
      }
      RETURN_IF_ERROR(InsertField(field, absl::bit_cast<float>(x)));
      break;
    }
    default:
      return absl::InvalidArgumentError(absl::StrFormat(
          "field type %d (number %d) does not support 32-bit fields",
          field.proto().kind(), field.proto().number()));
  }
  return absl::OkStatus();
}

absl::Status UntypedMessage::DecodeDelimited(io::CodedInputStream& stream,
                                             const ResolverPool::Field& field) {
  auto limit = stream.ReadLengthAndPushLimit();
  if (limit == 0) {
    return absl::InvalidArgumentError("unexpected EOF");
  }

  switch (field.proto().kind()) {
    case Field::TYPE_STRING:
    case Field::TYPE_BYTES: {
      std::string buf;
      if (!stream.ReadString(&buf, stream.BytesUntilLimit())) {
        return absl::InvalidArgumentError("unexpected EOF");
      }
      if (field.proto().kind() == Field::TYPE_STRING) {
        if (desc_->proto().syntax() == google::protobuf::SYNTAX_PROTO3 &&
            ::google::protobuf::internal::IsStructurallyValidUTF8(buf.data(), buf.size())) {
          return absl::InvalidArgumentError("proto3 strings must be UTF-8");
        }
      }

      RETURN_IF_ERROR(InsertField<std::string>(field, std::move(buf)));
      break;
    }
    case Field::TYPE_MESSAGE: {
      auto inner_desc = field.MessageType();
      RETURN_IF_ERROR(inner_desc.status());

      auto inner = ParseFromStream(*inner_desc, stream);
      RETURN_IF_ERROR(inner.status());
      RETURN_IF_ERROR(InsertField<UntypedMessage>(field, std::move(*inner)));
      break;
    }
    default: {
      // This is definitely a packed field.
      while (stream.BytesUntilLimit() > 0) {
        switch (field.proto().kind()) {
          case Field::TYPE_BOOL:
          case Field::TYPE_INT32:
          case Field::TYPE_SINT32:
          case Field::TYPE_UINT32:
          case Field::TYPE_ENUM:
          case Field::TYPE_INT64:
          case Field::TYPE_SINT64:
          case Field::TYPE_UINT64:
            RETURN_IF_ERROR(DecodeVarint(stream, field));
            break;
          case Field::TYPE_FIXED64:
          case Field::TYPE_SFIXED64:
          case Field::TYPE_DOUBLE:
            RETURN_IF_ERROR(Decode64Bit(stream, field));
            break;
          case Field::TYPE_FIXED32:
          case Field::TYPE_SFIXED32:
          case Field::TYPE_FLOAT:
            RETURN_IF_ERROR(Decode32Bit(stream, field));
            break;
          default:
            return absl::InvalidArgumentError(absl::StrFormat(
                "field type %d (number %d) does not support type 2 records",
                field.proto().kind(), field.proto().number()));
        }
      }
      break;
    }
  }
  stream.PopLimit(limit);
  return absl::OkStatus();
}

template <typename T>
absl::Status UntypedMessage::InsertField(const ResolverPool::Field& field,
                                         T value) {
  int32_t number = field.proto().number();
  auto emplace_result = fields_.try_emplace(number, std::move(value));
  if (emplace_result.second) {
    return absl::OkStatus();
  }

  if (field.proto().cardinality() !=
      google::protobuf::Field::CARDINALITY_REPEATED) {
    return absl::InvalidArgumentError(
        absl::StrCat("repeated entries for singular field number ", number));
  }

  Value& slot = emplace_result.first->second;
  if (auto* extant = absl::get_if<T>(&slot)) {
    std::vector<T> repeated;
    repeated.push_back(std::move(*extant));
    repeated.push_back(std::move(value));

    slot = std::move(repeated);
  } else if (auto* extant = absl::get_if<std::vector<T>>(&slot)) {
    extant->push_back(std::move(value));
  } else {
    return absl::InvalidArgumentError(
        absl::StrFormat("inconsistent types for field number %d: tried to "
                        "insert %s, but index was %d",
                        number, typeid(T).name(), slot.index()));
  }

  return absl::OkStatus();
}

}  // namespace json_internal
}  // namespace protobuf
}  // namespace google
