// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef GOOGLE_PROTOBUF_EXTENSION_SET_INL_H__
#define GOOGLE_PROTOBUF_EXTENSION_SET_INL_H__

#include <cstdint>
#include <string>
#include <utility>

#include "absl/base/optimization.h"
#include "google/protobuf/extension_set.h"
#include "google/protobuf/message_lite.h"
#include "google/protobuf/message_traits.h"
#include "google/protobuf/metadata_lite.h"
#include "google/protobuf/parse_context.h"
#include "google/protobuf/private_access.h"
#include "google/protobuf/wire_format_lite.h"
#include "utf8_validity.h"

namespace google {
namespace protobuf {
namespace internal {

template <typename T>
const char* ExtensionSet::ParseFieldWithExtensionInfo(
    MessageLite& parent, int number, bool was_packed_on_wire,
    const ExtensionInfo& info, const char* ptr, internal::ParseContext* ctx) {
  if (was_packed_on_wire) {
    switch (info.type) {
#define HANDLE_TYPE(UPPERCASE, CPP_CAMELCASE)                              \
  case WireFormatLite::TYPE_##UPPERCASE:                                   \
    return internal::Packed##CPP_CAMELCASE##Parser(                        \
        parent,                                                            \
        MutableRawRepeatedField(parent, number, info.type, info.is_packed, \
                                info.descriptor),                          \
        ptr, ctx);
      HANDLE_TYPE(INT32, Int32);
      HANDLE_TYPE(INT64, Int64);
      HANDLE_TYPE(UINT32, UInt32);
      HANDLE_TYPE(UINT64, UInt64);
      HANDLE_TYPE(SINT32, SInt32);
      HANDLE_TYPE(SINT64, SInt64);
      HANDLE_TYPE(FIXED32, Fixed32);
      HANDLE_TYPE(FIXED64, Fixed64);
      HANDLE_TYPE(SFIXED32, SFixed32);
      HANDLE_TYPE(SFIXED64, SFixed64);
      HANDLE_TYPE(FLOAT, Float);
      HANDLE_TYPE(DOUBLE, Double);
      HANDLE_TYPE(BOOL, Bool);
#undef HANDLE_TYPE

      case WireFormatLite::TYPE_ENUM:
        return internal::PackedEnumParserArg<T>(
            parent,
            MutableRawRepeatedField(parent, number, info.type, info.is_packed,
                                    info.descriptor),
            ptr, ctx, info.enum_validity_check, number);
      case WireFormatLite::TYPE_STRING:
      case WireFormatLite::TYPE_BYTES:
      case WireFormatLite::TYPE_GROUP:
      case WireFormatLite::TYPE_MESSAGE:
        ABSL_LOG(FATAL) << "Non-primitive types can't be packed.";
        break;
    }
  } else {
    switch (info.type) {
#define HANDLE_VARINT_TYPE(UPPERCASE, CPPTYPE)                              \
  case WireFormatLite::TYPE_##UPPERCASE: {                                  \
    uint64_t value;                                                         \
    ptr = VarintParse(ptr, &value);                                         \
    GOOGLE_PROTOBUF_PARSER_ASSERT(ptr);                                    \
    if (info.is_repeated) {                                                 \
      Add<CPPTYPE>(parent, number, WireFormatLite::TYPE_##UPPERCASE,        \
                   info.is_packed, value, info.descriptor);                 \
    } else {                                                                \
      Set<CPPTYPE>(parent, number, WireFormatLite::TYPE_##UPPERCASE, value, \
                   info.descriptor);                                        \
    }                                                                       \
  } break

      HANDLE_VARINT_TYPE(INT32, int32_t);
      HANDLE_VARINT_TYPE(INT64, int64_t);
      HANDLE_VARINT_TYPE(UINT32, uint32_t);
      HANDLE_VARINT_TYPE(UINT64, uint64_t);
      HANDLE_VARINT_TYPE(BOOL, bool);
#undef HANDLE_VARINT_TYPE
#define HANDLE_SVARINT_TYPE(UPPERCASE, Type, SIZE)                       \
  case WireFormatLite::TYPE_##UPPERCASE: {                               \
    uint64_t val;                                                        \
    ptr = VarintParse(ptr, &val);                                        \
    GOOGLE_PROTOBUF_PARSER_ASSERT(ptr);                                 \
    auto value = WireFormatLite::ZigZagDecode##SIZE(val);                \
    if (info.is_repeated) {                                              \
      Add<Type>(parent, number, WireFormatLite::TYPE_##UPPERCASE,        \
                info.is_packed, value, info.descriptor);                 \
    } else {                                                             \
      Set<Type>(parent, number, WireFormatLite::TYPE_##UPPERCASE, value, \
                info.descriptor);                                        \
    }                                                                    \
  } break

      HANDLE_SVARINT_TYPE(SINT32, int32_t, 32);
      HANDLE_SVARINT_TYPE(SINT64, int64_t, 64);
#undef HANDLE_SVARINT_TYPE
#define HANDLE_FIXED_TYPE(UPPERCASE, CPPTYPE)                               \
  case WireFormatLite::TYPE_##UPPERCASE: {                                  \
    auto value = UnalignedLoad<CPPTYPE>(ptr);                               \
    ptr += sizeof(CPPTYPE);                                                 \
    if (info.is_repeated) {                                                 \
      Add<CPPTYPE>(parent, number, WireFormatLite::TYPE_##UPPERCASE,        \
                   info.is_packed, value, info.descriptor);                 \
    } else {                                                                \
      Set<CPPTYPE>(parent, number, WireFormatLite::TYPE_##UPPERCASE, value, \
                   info.descriptor);                                        \
    }                                                                       \
  } break

      HANDLE_FIXED_TYPE(FIXED32, uint32_t);
      HANDLE_FIXED_TYPE(FIXED64, uint64_t);
      HANDLE_FIXED_TYPE(SFIXED32, int32_t);
      HANDLE_FIXED_TYPE(SFIXED64, int64_t);
      HANDLE_FIXED_TYPE(FLOAT, float);
      HANDLE_FIXED_TYPE(DOUBLE, double);
#undef HANDLE_FIXED_TYPE

      case WireFormatLite::TYPE_ENUM: {
        uint64_t tmp;
        ptr = VarintParse(ptr, &tmp);
        GOOGLE_PROTOBUF_PARSER_ASSERT(ptr);
        int value = tmp;

        if (!info.enum_validity_check.IsValid(value)) {
          WriteVarint(number, value,
                      PrivateAccess::GetInternalMetadata(parent)
                          .mutable_unknown_fields<T>());
        } else if (info.is_repeated) {
          Add<int>(parent, number, WireFormatLite::TYPE_ENUM, info.is_packed,
                   value, info.descriptor);
        } else {
          Set<int>(parent, number, WireFormatLite::TYPE_ENUM, value,
                   info.descriptor);
        }
        break;
      }

      case WireFormatLite::TYPE_BYTES:
      case WireFormatLite::TYPE_STRING: {
        std::string* value =
            info.is_repeated
                ? AddString(parent, number, WireFormatLite::TYPE_STRING,
                            info.descriptor)
                : MutableString(parent, number, WireFormatLite::TYPE_STRING,
                                info.descriptor);
        int size = ReadSize(&ptr);
        GOOGLE_PROTOBUF_PARSER_ASSERT(ptr);
        if (info.is_utf8) {
          ptr = ctx->ReadString(ptr, size, value);
          if ABSL_PREDICT_FALSE (!utf8_range::IsStructurallyValid(*value)) {
            return nullptr;
          }
          return ptr;
        } else {
          return ctx->ReadString(ptr, size, value);
        }
      }

      case WireFormatLite::TYPE_GROUP: {
        MessageLite* value =
            info.is_repeated
                ? AddMessage(parent, number, WireFormatLite::TYPE_GROUP,
                             info.message_info.class_data, info.descriptor)
                : MutableMessageByClassData(
                      parent, number, WireFormatLite::TYPE_GROUP,
                      info.message_info.class_data, info.descriptor);
        uint32_t tag = (number << 3) + WireFormatLite::WIRETYPE_START_GROUP;
        return ctx->ParseGroup(value, ptr, tag);
      }

      case WireFormatLite::TYPE_MESSAGE: {
        MessageLite* value =
            info.is_repeated
                ? AddMessage(parent, number, WireFormatLite::TYPE_MESSAGE,
                             info.message_info.class_data, info.descriptor)
                : MutableMessageByClassData(
                      parent, number, WireFormatLite::TYPE_MESSAGE,
                      info.message_info.class_data, info.descriptor);
        return ctx->ParseMessage(value, ptr);
      }
    }
  }
  return ptr;
}

// Parses the fields of a MessageSet item group, up to and including its end
// group tag, and dispatches the type_id and message fields to the callbacks:
//
//   on_message(uint32_t type_id, const char* ptr) -> const char*
//     The message follows the type_id; `ptr` points at its length prefix.
//   on_payload(uint32_t type_id, const std::string& payload) -> bool
//     The message (`payload`) preceded the type_id.
//   on_other_field(uint32_t tag, const char* ptr) -> const char*
//     Any other field; `ptr` points just past its tag.
//
// The first type_id and the first message win; later ones are ignored. A
// type_id that is 0 after the cast to uint32_t fails the parse. Shared by
// ExtensionSet::ParseMessageSetItemTmpl and the eager verifier so that both
// apply the same rules.
template <typename OnMessage, typename OnPayload, typename OnOtherField>
const char* ParseMessageSetItemFields(const char* ptr, ParseContext* ctx,
                                      OnMessage on_message,
                                      OnPayload on_payload,
                                      OnOtherField on_other_field) {
  std::string payload;
  uint32_t type_id = 0;
  enum class State { kNoTag, kHasType, kHasPayload, kDone };
  State state = State::kNoTag;

  while (!ctx->Done(&ptr)) {
    uint32_t tag = static_cast<uint8_t>(*ptr++);
    if (tag == WireFormatLite::kMessageSetTypeIdTag) {
      uint64_t tmp;
      ptr = ParseBigVarint(ptr, &tmp);
      if (ABSL_PREDICT_FALSE(ptr == nullptr ||
                             static_cast<uint32_t>(tmp) == 0)) {
        return nullptr;
      }
      // Any order is accepted, but type_id usually precedes the message.
      if (ABSL_PREDICT_TRUE(state == State::kNoTag)) {
        type_id = static_cast<uint32_t>(tmp);
        state = State::kHasType;
      } else if (state == State::kHasPayload) {
        type_id = static_cast<uint32_t>(tmp);
        if (!on_payload(type_id, payload)) return nullptr;
        state = State::kDone;
      }
      // Otherwise, ignore type_id tags after the first.
    } else if (tag == WireFormatLite::kMessageSetMessageTag) {
      // Any order is accepted, but type_id usually precedes the message.
      if (ABSL_PREDICT_TRUE(state == State::kHasType)) {
        ptr = on_message(type_id, ptr);
        state = State::kDone;
      } else {
        int32_t size = ReadSize(&ptr);
        if (ptr == nullptr) return nullptr;
        if (state == State::kNoTag) {
          ptr = ctx->ReadString(ptr, size, &payload);
          state = State::kHasPayload;
        } else {
          // Ignore messages after the first.
          ptr = ctx->Skip(ptr, size);
        }
      }
    } else {
      ptr = ReadTag(ptr - 1, &tag);
      if (tag == 0 || (tag & 7) == 4) {
        ctx->SetLastTag(tag);
        return ptr;
      }
      ptr = on_other_field(tag, ptr);
    }
    if (ptr == nullptr) return nullptr;
  }
  return ptr;
}

template <typename Msg, typename T>
const char* ExtensionSet::ParseMessageSetItemTmpl(MessageLite& parent,
                                                  const char* ptr,
                                                  const Msg* extendee,
                                                  internal::ParseContext* ctx) {
  return ParseMessageSetItemFields(
      ptr, ctx,
      [&](uint32_t type_id, const char* message_ptr) {
        return ParseFieldMaybeLazily(parent,
                                     static_cast<uint64_t>(type_id) * 8 + 2,
                                     message_ptr, extendee, ctx);
      },
      [&](uint32_t type_id, const std::string& payload) {
        ExtensionInfo extension;
        bool was_packed_on_wire;
        if (!FindExtension(2, type_id, extendee, ctx, &extension,
                           &was_packed_on_wire)) {
          WriteLengthDelimited(type_id, payload,
                               PrivateAccess::GetInternalMetadata(parent)
                                   .mutable_unknown_fields<T>());
          return true;
        }
        MessageLite* value =
            extension.is_repeated
                ? AddMessage(parent, type_id, WireFormatLite::TYPE_MESSAGE,
                             extension.message_info.class_data,
                             extension.descriptor)
                : MutableMessageByClassData(
                      parent, type_id, WireFormatLite::TYPE_MESSAGE,
                      extension.message_info.class_data, extension.descriptor);

        const char* p;
        // Use Spawn to transfer all attributes for recursion.
        // However, use Spawn<1> to decrease depth an extra time to take into
        // account that `payload` came from a subfield.
        ParseContext tmp_ctx(ParseContext::Spawn<1>{}, *ctx, &p, payload);
        if (p == nullptr) return false;
        return value->_InternalParse(p, &tmp_ctx) != nullptr &&
               tmp_ctx.EndedAtLimit();
      },
      [&](uint32_t tag, const char* field_ptr) {
        return ParseField(parent, tag, field_ptr, extendee, ctx);
      });
}

}  // namespace internal
}  // namespace protobuf
}  // namespace google

#endif  // GOOGLE_PROTOBUF_EXTENSION_SET_INL_H__
