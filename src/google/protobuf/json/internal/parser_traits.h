// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef GOOGLE_PROTOBUF_JSON_INTERNAL_PARSER_TRAITS_H__
#define GOOGLE_PROTOBUF_JSON_INTERNAL_PARSER_TRAITS_H__

#include <cfloat>
#include <cmath>
#include <cstdint>
#include <limits>
#include <memory>
#include <string>
#include <type_traits>
#include <utility>

#include "google/protobuf/type.pb.h"
#include "absl/base/attributes.h"
#include "absl/base/casts.h"
#include "absl/container/flat_hash_map.h"
#include "absl/container/flat_hash_set.h"
#include "absl/status/status.h"
#include "absl/strings/str_format.h"
#include "absl/strings/string_view.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/io/coded_stream.h"
#include "google/protobuf/io/zero_copy_stream.h"
#include "google/protobuf/io/zero_copy_stream_impl_lite.h"
#include "google/protobuf/json/internal/descriptor_traits.h"
#include "google/protobuf/json/internal/status_macros.h"
#include "google/protobuf/wire_format_lite.h"
#include "google/protobuf/stubs/status_macros.h"

// Must be included last.
#include "google/protobuf/port_def.inc"

namespace google {
namespace protobuf {
namespace json_internal {

template <typename T>
constexpr bool IsGoodParam() {
  static_assert(sizeof(T) <= 16);
  static_assert(std::is_trivially_copyable_v<T>);
  return true;
}

// We use this for lambdas to make sure we are passing them in registers instead
// of by ref to avoid stack usage on the caller.
// If the assertion fails it means we are capturing too much on the lambda.
template <typename T, bool = IsGoodParam<T>()>
using GoodParam = T;

enum class SeenState {
  kFirstSeen,
  kOneofAlreadySeen,
  kFieldAlreadySeen,
};

using ::google::protobuf::internal::WireFormatLite;

// See the comment in json_util2_parser.cc for more information.
//
// The type traits in this file  describe how to parse to a protobuf
// representation used by the JSON API, either via proto reflection or by
// emitting wire format to an output stream.

// Helper alias templates to avoid needing to write `typename` in function
// signatures.
template <typename Traits>
using Msg = typename Traits::Msg;

struct ParseProto2Descriptor : Proto2Descriptor {
  // A message value that fields can be written to, but not read from.
  class Msg {
   public:
    explicit Msg(Message* msg) : impl_(std::make_unique<Impl>()) {
      impl_->msg = msg;
    }

   private:
    friend ParseProto2Descriptor;
    // Use pImpl to reduce stack usage.
    struct Impl {
      Message* msg;
      // Because `msg` might already have oneofs set, we need to track which
      // were set *during* the parse separately.
      absl::flat_hash_set<int> parsed_oneofs_indices;
      absl::flat_hash_set<int> parsed_fields;
    };

    std::unique_ptr<Impl> impl_;
  };

  /// Functions for writing fields. ///

  // Marks a field as having been "seen". This will return a SeenState
  // indicating whether it is first seen, duplicate field seen, or oneof
  // conflict seen.
  static SeenState RecordAsSeen(Field f, Msg& msg) {
    if (f->real_containing_oneof() != nullptr) {
      int oneof_index = f->real_containing_oneof()->index();
      bool oneof_inserted =
          msg.impl_->parsed_oneofs_indices.insert(oneof_index).second;
      if (!oneof_inserted) {
        // Oneof takes precedent over Field in the case of "this is in a oneof
        // and the field is set"
        return SeenState::kOneofAlreadySeen;
      }
    }

    bool field_inserted = msg.impl_->parsed_fields.insert(f->number()).second;
    if (!field_inserted) {
      return SeenState::kFieldAlreadySeen;
    }

    return SeenState::kFirstSeen;
  }

  static void ClearField(Field f, Msg& msg) {
    msg.impl_->msg->GetReflection()->ClearField(msg.impl_->msg, f);
  }

  // Adds a new message and calls body on it.
  //
  // Body should have a signature `absl::Status(const Desc&, Msg&)`.
  template <typename F>
  static absl::Status NewMsg(Field f, Msg& msg, GoodParam<F> body) {
    Message* new_msg;
    if (f->is_repeated()) {
      new_msg = msg.impl_->msg->GetReflection()->AddMessage(msg.impl_->msg, f);
    } else {
      new_msg =
          msg.impl_->msg->GetReflection()->MutableMessage(msg.impl_->msg, f);
    }
    Msg wrapper(new_msg);
    return body(*f->message_type(), wrapper);
  }

  // Adds a new dynamic message with the given type name and calls body on it.
  //
  // Body should have a signature `absl::Status(const Desc&, Msg&)`.
  template <typename F>
  static absl::Status NewDynamic(Field f, const std::string& type_url, Msg& msg,
                                 GoodParam<F> body) {
    return WithDynamicType(
        *f->containing_type(), type_url, [&](const Desc& desc) -> absl::Status {
          DynamicMessageFactory factory;
          std::unique_ptr<Message> dynamic(factory.GetPrototype(&desc)->New());
          Msg wrapper(dynamic.get());
          PB_RETURN_IF_ERROR(body(desc, wrapper));

          if (f->is_repeated()) {
            msg.impl_->msg->GetReflection()->AddString(
                msg.impl_->msg, f, dynamic->SerializePartialAsString());
          } else {
            msg.impl_->msg->GetReflection()->SetString(
                msg.impl_->msg, f, dynamic->SerializePartialAsString());
          }
          return absl::OkStatus();
        });
  }

  static void SetFloat(Field f, Msg& msg, float x) {
    if (f->is_repeated()) {
      msg.impl_->msg->GetReflection()->AddFloat(msg.impl_->msg, f, x);
    } else {
      msg.impl_->msg->GetReflection()->SetFloat(msg.impl_->msg, f, x);
    }
  }

  static void SetDouble(Field f, Msg& msg, double x) {
    if (f->is_repeated()) {
      msg.impl_->msg->GetReflection()->AddDouble(msg.impl_->msg, f, x);
    } else {
      msg.impl_->msg->GetReflection()->SetDouble(msg.impl_->msg, f, x);
    }
  }

  static void SetInt64(Field f, Msg& msg, int64_t x) {
    if (f->is_repeated()) {
      msg.impl_->msg->GetReflection()->AddInt64(msg.impl_->msg, f, x);
    } else {
      msg.impl_->msg->GetReflection()->SetInt64(msg.impl_->msg, f, x);
    }
  }

  static void SetUInt64(Field f, Msg& msg, uint64_t x) {
    if (f->is_repeated()) {
      msg.impl_->msg->GetReflection()->AddUInt64(msg.impl_->msg, f, x);
    } else {
      msg.impl_->msg->GetReflection()->SetUInt64(msg.impl_->msg, f, x);
    }
  }

  static void SetInt32(Field f, Msg& msg, int32_t x) {
    if (f->is_repeated()) {
      msg.impl_->msg->GetReflection()->AddInt32(msg.impl_->msg, f, x);
    } else {
      msg.impl_->msg->GetReflection()->SetInt32(msg.impl_->msg, f, x);
    }
  }

  static void SetUInt32(Field f, Msg& msg, uint32_t x) {
    if (f->is_repeated()) {
      msg.impl_->msg->GetReflection()->AddUInt32(msg.impl_->msg, f, x);
    } else {
      msg.impl_->msg->GetReflection()->SetUInt32(msg.impl_->msg, f, x);
    }
  }

  static void SetBool(Field f, Msg& msg, bool x) {
    if (f->is_repeated()) {
      msg.impl_->msg->GetReflection()->AddBool(msg.impl_->msg, f, x);
    } else {
      msg.impl_->msg->GetReflection()->SetBool(msg.impl_->msg, f, x);
    }
  }

  static void SetString(Field f, Msg& msg, absl::string_view x) {
    if (f->is_repeated()) {
      msg.impl_->msg->GetReflection()->AddString(msg.impl_->msg, f,
                                                 std::string(x));
    } else {
      msg.impl_->msg->GetReflection()->SetString(msg.impl_->msg, f,
                                                 std::string(x));
    }
  }

  static void SetEnum(Field f, Msg& msg, int32_t x) {
    if (f->is_repeated()) {
      msg.impl_->msg->GetReflection()->AddEnumValue(msg.impl_->msg, f, x);
    } else {
      msg.impl_->msg->GetReflection()->SetEnumValue(msg.impl_->msg, f, x);
    }
  }
};

// Traits for proto3-ish deserialization.
//
// This includes a rudimentary proto serializer, since message fields are
// written directly instead of being reflectively written to a proto field.
//
// See MessageTraits for API docs.
struct ParseProto3Type : Proto3Type {
  class Msg {
   public:
    Msg() : impl_(std::make_unique<Impl>(nullptr)) {}
    explicit Msg(io::ZeroCopyOutputStream* stream)
        : impl_(std::make_unique<Impl>(stream)) {}

   private:
    friend ParseProto3Type;
    // Use pImpl to reduce stack usage.
    struct Impl {
      explicit Impl(io::ZeroCopyOutputStream* zs)
          : stream(zs ? zs : &sstream) {}
      // Declaration order matters: `stream` writes into `&sstream`/`&out` when
      // `zs == nullptr`, so `stream` must be destroyed before `sstream`/`out`.
      std::string out;
      io::StringOutputStream sstream{&out};
      io::CodedOutputStream stream;
      absl::flat_hash_set<int32_t> parsed_oneofs_indices;
      absl::flat_hash_set<int32_t> parsed_fields;
    };
    std::unique_ptr<Impl> impl_;
  };

  /// Functions for writing fields. ///

  static SeenState RecordAsSeen(Field f, Msg& msg) {
    int oneof_index = f->proto().oneof_index();
    if (oneof_index != 0) {
      bool oneof_inserted =
          msg.impl_->parsed_oneofs_indices.insert(oneof_index).second;
      if (!oneof_inserted) {
        // Oneof takes precedent over Field in the case of "this is in a oneof
        // and the field is set"
        return SeenState::kOneofAlreadySeen;
      }
    }

    bool field_inserted =
        msg.impl_->parsed_fields.insert(f->proto().number()).second;
    if (!field_inserted) {
      return SeenState::kFieldAlreadySeen;
    }

    return SeenState::kFirstSeen;
  }

  static void ClearField(Field f, Msg& msg) {}

  template <typename F>
  static absl::Status NewMsg(Field f, Msg& msg, GoodParam<F> body) {
    return NewDynamic(f, f->proto().type_url(), msg, body);
  }

  template <typename F>
  static absl::Status NewDynamic(Field f, const std::string& type_url, Msg& msg,
                                 GoodParam<F> body) {
    return WithDynamicType(
        f->parent(), type_url, [&](const Desc& desc) -> absl::Status {
          if (f->proto().kind() == google::protobuf::Field::TYPE_GROUP) {
            msg.impl_->stream.WriteTag(f->proto().number() << 3 |
                                       WireFormatLite::WIRETYPE_START_GROUP);
            PB_RETURN_IF_ERROR(body(desc, msg));
            msg.impl_->stream.WriteTag(f->proto().number() << 3 |
                                       WireFormatLite::WIRETYPE_END_GROUP);
            return absl::OkStatus();
          }

          Msg new_msg;
          PB_RETURN_IF_ERROR(body(desc, new_msg));

          new_msg.impl_->stream.Trim();  // Should probably be called "Flush()".
          absl::string_view written =
              absl::string_view(new_msg.impl_->out)
                  .substr(0, static_cast<size_t>(
                                 new_msg.impl_->stream.ByteCount()));
          SetString(f, msg, written);
          return absl::OkStatus();
        });
  }

  static void SetFloat(Field f, Msg& msg, float x) {
    msg.impl_->stream.WriteTag(f->proto().number() << 3 |
                               WireFormatLite::WIRETYPE_FIXED32);
    msg.impl_->stream.WriteLittleEndian32(absl::bit_cast<uint32_t>(x));
  }

  static void SetDouble(Field f, Msg& msg, double x) {
    msg.impl_->stream.WriteTag(f->proto().number() << 3 |
                               WireFormatLite::WIRETYPE_FIXED64);
    msg.impl_->stream.WriteLittleEndian64(absl::bit_cast<uint64_t>(x));
  }

  static void SetInt64(Field f, Msg& msg, int64_t x) {
    SetInt<int64_t, google::protobuf::Field::TYPE_INT64,
           google::protobuf::Field::TYPE_SFIXED64,
           google::protobuf::Field::TYPE_SINT64>(f, msg, x);
  }

  static void SetUInt64(Field f, Msg& msg, uint64_t x) {
    SetInt<uint64_t, google::protobuf::Field::TYPE_UINT64,
           google::protobuf::Field::TYPE_FIXED64,
           google::protobuf::Field::TYPE_UNKNOWN>(f, msg, x);
  }

  static void SetInt32(Field f, Msg& msg, int32_t x) {
    SetInt<int32_t, google::protobuf::Field::TYPE_INT32,
           google::protobuf::Field::TYPE_SFIXED32,
           google::protobuf::Field::TYPE_SINT32>(f, msg, x);
  }

  static void SetUInt32(Field f, Msg& msg, uint32_t x) {
    SetInt<uint32_t, google::protobuf::Field::TYPE_UINT32,
           google::protobuf::Field::TYPE_FIXED32,
           google::protobuf::Field::TYPE_UNKNOWN>(f, msg, x);
  }

  static void SetBool(Field f, Msg& msg, bool x) {
    msg.impl_->stream.WriteTag(f->proto().number() << 3);
    char b = x ? 0x01 : 0x00;
    msg.impl_->stream.WriteRaw(&b, 1);
  }

  static void SetString(Field f, Msg& msg, absl::string_view x) {
    msg.impl_->stream.WriteTag(f->proto().number() << 3 |
                               WireFormatLite::WIRETYPE_LENGTH_DELIMITED);
    msg.impl_->stream.WriteVarint64(static_cast<uint64_t>(x.size()));
    msg.impl_->stream.WriteRaw(x.data(), x.size());
  }

  static void SetEnum(Field f, Msg& msg, int32_t x) {
    msg.impl_->stream.WriteTag(f->proto().number() << 3);
    // Sign extension is deliberate here.
    msg.impl_->stream.WriteVarint32(x);
  }

 private:
  using Kind = google::protobuf::Field::Kind;
  // Sets a field of *some* integer type, with the given kinds for the possible
  // encodings. This avoids quadruplicating this code in the helpers for the
  // four major integer types.
  template <typename Int, Kind varint, Kind fixed, Kind zigzag>
  static void SetInt(Field f, Msg& msg, Int x) {
    switch (f->proto().kind()) {
      case zigzag:
        // Regardless of the integer type, ZigZag64 will do the right thing,
        // because ZigZag is not dependent on the width of the integer: it is
        // always `2 * abs(n) + (n < 0)`.
        x = static_cast<Int>(
            internal::WireFormatLite::ZigZagEncode64(static_cast<int64_t>(x)));
        [[fallthrough]];
      case varint:
        msg.impl_->stream.WriteTag(f->proto().number() << 3 |
                                   WireFormatLite::WIRETYPE_VARINT);
        if (sizeof(Int) == 4) {
          msg.impl_->stream.WriteVarint32(static_cast<uint32_t>(x));
        } else {
          msg.impl_->stream.WriteVarint64(static_cast<uint64_t>(x));
        }
        break;
      case fixed: {
        if (sizeof(Int) == 4) {
          msg.impl_->stream.WriteTag(f->proto().number() << 3 |
                                     WireFormatLite::WIRETYPE_FIXED32);
          msg.impl_->stream.WriteLittleEndian32(static_cast<uint32_t>(x));
        } else {
          msg.impl_->stream.WriteTag(f->proto().number() << 3 |
                                     WireFormatLite::WIRETYPE_FIXED64);
          msg.impl_->stream.WriteLittleEndian64(static_cast<uint64_t>(x));
        }
        break;
      }
      default: {  // Unreachable.
      }
    }
  }
};
}  // namespace json_internal
}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"
#endif  // GOOGLE_PROTOBUF_JSON_INTERNAL_PARSER_TRAITS_H__
