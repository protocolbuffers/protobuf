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

#ifndef GOOGLE_PROTOBUF_EXTENSION_SET_INL_H__
#define GOOGLE_PROTOBUF_EXTENSION_SET_INL_H__

#include <google/protobuf/extension_set.h>

namespace google {
namespace protobuf {
namespace internal {

#if GOOGLE_PROTOBUF_ENABLE_EXPERIMENTAL_PARSER
template <typename T>
std::pair<const char*, bool> ExtensionSet::ParseFieldWithExtensionInfo(
    int number, bool was_packed_on_wire, const ExtensionInfo& extension,
    T* metadata, ParseClosure parent, const char* begin, const char* end,
    internal::ParseContext* ctx) {
  auto ptr = begin;
  ParseClosure child;
  int depth;
  if (was_packed_on_wire) {
    switch (extension.type) {
#define HANDLE_TYPE(UPPERCASE, CPP_CAMELCASE)                                \
  case WireFormatLite::TYPE_##UPPERCASE:                                     \
    child = {                                                                \
        internal::Packed##CPP_CAMELCASE##Parser,                             \
        MutableRawRepeatedField(number, extension.type, extension.is_packed, \
                                extension.descriptor)};                      \
    goto length_delim
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
        ctx->extra_parse_data().SetEnumValidatorArg(
            extension.enum_validity_check.func,
            extension.enum_validity_check.arg,
            metadata->mutable_unknown_fields(), number);
        child = {
            internal::PackedValidEnumParserLiteArg,
            MutableRawRepeatedField(number, extension.type, extension.is_packed,
                                    extension.descriptor)};
        goto length_delim;
      case WireFormatLite::TYPE_STRING:
      case WireFormatLite::TYPE_BYTES:
      case WireFormatLite::TYPE_GROUP:
      case WireFormatLite::TYPE_MESSAGE:
        GOOGLE_LOG(FATAL) << "Non-primitive types can't be packed.";
        break;
    }
  } else {
    switch (extension.type) {
#define HANDLE_VARINT_TYPE(UPPERCASE, CPP_CAMELCASE)                        \
  case WireFormatLite::TYPE_##UPPERCASE: {                                  \
    uint64 value;                                                           \
    ptr = Varint::Parse64(ptr, &value);                                     \
    GOOGLE_PROTOBUF_ASSERT_RETURN(ptr, std::make_pair(nullptr, true));     \
    if (extension.is_repeated) {                                            \
      Add##CPP_CAMELCASE(number, WireFormatLite::TYPE_##UPPERCASE,          \
                         extension.is_packed, value, extension.descriptor); \
    } else {                                                                \
      Set##CPP_CAMELCASE(number, WireFormatLite::TYPE_##UPPERCASE, value,   \
                         extension.descriptor);                             \
    }                                                                       \
  } break

      HANDLE_VARINT_TYPE(INT32, Int32);
      HANDLE_VARINT_TYPE(INT64, Int64);
      HANDLE_VARINT_TYPE(UINT32, UInt32);
      HANDLE_VARINT_TYPE(UINT64, UInt64);
#undef HANDLE_VARINT_TYPE
#define HANDLE_SVARINT_TYPE(UPPERCASE, CPP_CAMELCASE, SIZE)                 \
  case WireFormatLite::TYPE_##UPPERCASE: {                                  \
    uint64 val;                                                             \
    ptr = Varint::Parse64(ptr, &val);                                       \
    GOOGLE_PROTOBUF_ASSERT_RETURN(ptr, std::make_pair(nullptr, true));     \
    auto value = WireFormatLite::ZigZagDecode##SIZE(val);                   \
    if (extension.is_repeated) {                                            \
      Add##CPP_CAMELCASE(number, WireFormatLite::TYPE_##UPPERCASE,          \
                         extension.is_packed, value, extension.descriptor); \
    } else {                                                                \
      Set##CPP_CAMELCASE(number, WireFormatLite::TYPE_##UPPERCASE, value,   \
                         extension.descriptor);                             \
    }                                                                       \
  } break

      HANDLE_SVARINT_TYPE(SINT32, Int32, 32);
      HANDLE_SVARINT_TYPE(SINT64, Int64, 64);
#undef HANDLE_SVARINT_TYPE
#define HANDLE_FIXED_TYPE(UPPERCASE, CPP_CAMELCASE, CPPTYPE)                \
  case WireFormatLite::TYPE_##UPPERCASE: {                                  \
    CPPTYPE value;                                                          \
    std::memcpy(&value, ptr, sizeof(CPPTYPE));                              \
    ptr += sizeof(CPPTYPE);                                                 \
    if (extension.is_repeated) {                                            \
      Add##CPP_CAMELCASE(number, WireFormatLite::TYPE_##UPPERCASE,          \
                         extension.is_packed, value, extension.descriptor); \
    } else {                                                                \
      Set##CPP_CAMELCASE(number, WireFormatLite::TYPE_##UPPERCASE, value,   \
                         extension.descriptor);                             \
    }                                                                       \
  } break

      HANDLE_FIXED_TYPE(FIXED32, UInt32, uint32);
      HANDLE_FIXED_TYPE(FIXED64, UInt64, uint64);
      HANDLE_FIXED_TYPE(SFIXED32, Int32, int32);
      HANDLE_FIXED_TYPE(SFIXED64, Int64, int64);
      HANDLE_FIXED_TYPE(FLOAT, Float, float);
      HANDLE_FIXED_TYPE(DOUBLE, Double, double);
      HANDLE_FIXED_TYPE(BOOL, Bool, bool);
#undef HANDLE_FIXED_TYPE

      case WireFormatLite::TYPE_ENUM: {
        uint64 val;
        ptr = Varint::Parse64(ptr, &val);
        GOOGLE_PROTOBUF_ASSERT_RETURN(ptr, std::make_pair(nullptr, true));
        int value = val;

        if (!extension.enum_validity_check.func(
                extension.enum_validity_check.arg, value)) {
          WriteVarint(number, val, metadata->mutable_unknown_fields());
        } else if (extension.is_repeated) {
          AddEnum(number, WireFormatLite::TYPE_ENUM, extension.is_packed, value,
                  extension.descriptor);
        } else {
          SetEnum(number, WireFormatLite::TYPE_ENUM, value,
                  extension.descriptor);
        }
        break;
      }

      case WireFormatLite::TYPE_BYTES:
      case WireFormatLite::TYPE_STRING: {
        std::string* value = extension.is_repeated
                            ? AddString(number, WireFormatLite::TYPE_STRING,
                                        extension.descriptor)
                            : MutableString(number, WireFormatLite::TYPE_STRING,
                                            extension.descriptor);
        child = {StringParser, value};
        goto length_delim;
      }

      case WireFormatLite::TYPE_GROUP: {
        MessageLite* value =
            extension.is_repeated
                ? AddMessage(number, WireFormatLite::TYPE_GROUP,
                             *extension.message_info.prototype,
                             extension.descriptor)
                : MutableMessage(number, WireFormatLite::TYPE_GROUP,
                                 *extension.message_info.prototype,
                                 extension.descriptor);
        child = {extension.message_info.parse_func, value};
        uint32 tag = (number << 3) + WireFormatLite::WIRETYPE_START_GROUP;
        auto res = ctx->ParseGroup(tag, child, ptr, end, &depth);
        ptr = res.first;
        GOOGLE_PROTOBUF_ASSERT_RETURN(ptr, std::make_pair(nullptr, true));
        if (res.second) {
          GOOGLE_PROTOBUF_ASSERT_RETURN(
              ctx->StoreGroup(parent, child, depth, tag),
              std::make_pair(nullptr, true));
          return std::make_pair(ptr, true);
        }
        break;
      }

      case WireFormatLite::TYPE_MESSAGE: {
        MessageLite* value =
            extension.is_repeated
                ? AddMessage(number, WireFormatLite::TYPE_MESSAGE,
                             *extension.message_info.prototype,
                             extension.descriptor)
                : MutableMessage(number, WireFormatLite::TYPE_MESSAGE,
                                 *extension.message_info.prototype,
                                 extension.descriptor);
        child = {extension.message_info.parse_func, value};
        goto length_delim;
      }
    }
  }

  return std::make_pair(ptr, false);

length_delim:
  uint32 size;
  ptr = Varint::Parse32Inline(ptr, &size);
  GOOGLE_PROTOBUF_ASSERT_RETURN(ptr, std::make_pair(nullptr, true));
  if (size > end - ptr) goto len_delim_till_end;
  {
    auto newend = ptr + size;
    bool ok = ctx->ParseExactRange(child, ptr, newend);
    GOOGLE_PROTOBUF_ASSERT_RETURN(ok, std::make_pair(nullptr, true));
    ptr = newend;
  }
  return std::make_pair(ptr, false);
len_delim_till_end:
  return std::make_pair(ctx->StoreAndTailCall(ptr, end, parent, child, size),
                        true);
}
#endif

}  // namespace internal
}  // namespace protobuf
}  // namespace google

#endif  // GOOGLE_PROTOBUF_EXTENSION_SET_INL_H__
