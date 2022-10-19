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

#include "google/protobuf/json/internal/parser.h"

#include <cfloat>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <limits>
#include <memory>
#include <string>
#include <utility>

#include "google/protobuf/type.pb.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/dynamic_message.h"
#include "google/protobuf/message.h"
#include "absl/base/attributes.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/ascii.h"
#include "absl/strings/escaping.h"
#include "absl/strings/match.h"
#include "absl/strings/numbers.h"
#include "absl/strings/str_format.h"
#include "absl/strings/str_split.h"
#include "absl/strings/string_view.h"
#include "absl/types/optional.h"
#include "absl/types/span.h"
#include "google/protobuf/io/zero_copy_sink.h"
#include "google/protobuf/io/zero_copy_stream.h"
#include "google/protobuf/io/zero_copy_stream_impl_lite.h"
#include "google/protobuf/json/internal/descriptor_traits.h"
#include "google/protobuf/json/internal/lexer.h"
#include "google/protobuf/json/internal/parser_traits.h"
#include "google/protobuf/util/type_resolver.h"
#include "google/protobuf/stubs/status_macros.h"

// Must be included last.
#include "google/protobuf/port_def.inc"

namespace google {
namespace protobuf {
namespace json_internal {
namespace {
// This file contains code that drives a JsonLexer to visit a JSON document and
// convert it into some form of proto.
//
// This semantic layer is duplicated: proto2-ish code can deserialize directly
// into a message, whereas proto3-ish code deserializes into a byte stream,
// using TypeResolvers instead of Descriptors.
//
// The parsing code is templated over which of these two reflection + output
// combinations is used. The traits types that collect the per-instantiation
// functionality can be found in json_util2_parser_traits-inl.h.

// This table maps an unsigned `char` value, interpreted as an ASCII character,
// to a corresponding value in the base64 alphabet (both traditional and
// "web-safe" characters are included).
//
// If a character is not valid base64, it maps to -1; this is used by the bit
// operations that assemble a base64-encoded word to determine if an error
// occurred, by checking the sign bit.
constexpr signed char kBase64Table[256] = {
    -1,       -1,       -1,       -1,       -1,       -1,        -1,
    -1,       -1,       -1,       -1,       -1,       -1,        -1,
    -1,       -1,       -1,       -1,       -1,       -1,        -1,
    -1,       -1,       -1,       -1,       -1,       -1,        -1,
    -1,       -1,       -1,       -1,       -1,       -1,        -1,
    -1,       -1,       -1,       -1,       -1,       -1,        -1,
    -1,       62 /*+*/, -1,       62 /*-*/, -1,       63 /*/ */, 52 /*0*/,
    53 /*1*/, 54 /*2*/, 55 /*3*/, 56 /*4*/, 57 /*5*/, 58 /*6*/,  59 /*7*/,
    60 /*8*/, 61 /*9*/, -1,       -1,       -1,       -1,        -1,
    -1,       -1,       0 /*A*/,  1 /*B*/,  2 /*C*/,  3 /*D*/,   4 /*E*/,
    5 /*F*/,  6 /*G*/,  07 /*H*/, 8 /*I*/,  9 /*J*/,  10 /*K*/,  11 /*L*/,
    12 /*M*/, 13 /*N*/, 14 /*O*/, 15 /*P*/, 16 /*Q*/, 17 /*R*/,  18 /*S*/,
    19 /*T*/, 20 /*U*/, 21 /*V*/, 22 /*W*/, 23 /*X*/, 24 /*Y*/,  25 /*Z*/,
    -1,       -1,       -1,       -1,       63 /*_*/, -1,        26 /*a*/,
    27 /*b*/, 28 /*c*/, 29 /*d*/, 30 /*e*/, 31 /*f*/, 32 /*g*/,  33 /*h*/,
    34 /*i*/, 35 /*j*/, 36 /*k*/, 37 /*l*/, 38 /*m*/, 39 /*n*/,  40 /*o*/,
    41 /*p*/, 42 /*q*/, 43 /*r*/, 44 /*s*/, 45 /*t*/, 46 /*u*/,  47 /*v*/,
    48 /*w*/, 49 /*x*/, 50 /*y*/, 51 /*z*/, -1,       -1,        -1,
    -1,       -1,       -1,       -1,       -1,       -1,        -1,
    -1,       -1,       -1,       -1,       -1,       -1,        -1,
    -1,       -1,       -1,       -1,       -1,       -1,        -1,
    -1,       -1,       -1,       -1,       -1,       -1,        -1,
    -1,       -1,       -1,       -1,       -1,       -1,        -1,
    -1,       -1,       -1,       -1,       -1,       -1,        -1,
    -1,       -1,       -1,       -1,       -1,       -1,        -1,
    -1,       -1,       -1,       -1,       -1,       -1,        -1,
    -1,       -1,       -1,       -1,       -1,       -1,        -1,
    -1,       -1,       -1,       -1,       -1,       -1,        -1,
    -1,       -1,       -1,       -1,       -1,       -1,        -1,
    -1,       -1,       -1,       -1,       -1,       -1,        -1,
    -1,       -1,       -1,       -1,       -1,       -1,        -1,
    -1,       -1,       -1,       -1,       -1,       -1,        -1,
    -1,       -1,       -1,       -1,       -1,       -1,        -1,
    -1,       -1,       -1,       -1,       -1,       -1,        -1,
    -1,       -1,       -1,       -1,       -1,       -1,        -1,
    -1,       -1,       -1,       -1,       -1,       -1,        -1,
    -1,       -1,       -1,       -1};

uint32_t Base64Lookup(char c) {
  // Sign-extend return value so high bit will be set on any unexpected char.
  return static_cast<uint32_t>(kBase64Table[static_cast<uint8_t>(c)]);
}

// Decodes `base64` in-place, shrinking the length as appropriate.
absl::StatusOr<absl::Span<char>> DecodeBase64InPlace(absl::Span<char> base64) {
  // We decode in place. This is safe because this is a new buffer (not
  // aliasing the input) and because base64 decoding shrinks 4 bytes into 3.
  char* out = base64.data();
  const char* ptr = base64.data();
  const char* end = ptr + base64.size();
  const char* end4 = ptr + (base64.size() & ~3u);

  for (; ptr < end4; ptr += 4, out += 3) {
    auto val = Base64Lookup(ptr[0]) << 18 | Base64Lookup(ptr[1]) << 12 |
               Base64Lookup(ptr[2]) << 6 | Base64Lookup(ptr[3]) << 0;

    if (static_cast<int32_t>(val) < 0) {
      // Junk chars or padding. Remove trailing padding, if any.
      if (end - ptr == 4 && ptr[3] == '=') {
        if (ptr[2] == '=') {
          end -= 2;
        } else {
          end -= 1;
        }
      }
      break;
    }

    out[0] = val >> 16;
    out[1] = (val >> 8) & 0xff;
    out[2] = val & 0xff;
  }

  if (ptr < end) {
    uint32_t val = ~0u;
    switch (end - ptr) {
      case 2:
        val = Base64Lookup(ptr[0]) << 18 | Base64Lookup(ptr[1]) << 12;
        out[0] = val >> 16;
        out += 1;
        break;
      case 3:
        val = Base64Lookup(ptr[0]) << 18 | Base64Lookup(ptr[1]) << 12 |
              Base64Lookup(ptr[2]) << 6;
        out[0] = val >> 16;
        out[1] = (val >> 8) & 0xff;
        out += 2;
        break;
    }

    if (static_cast<int32_t>(val) < 0) {
      return absl::InvalidArgumentError("corrupt base64");
    }
  }

  return absl::Span<char>(base64.data(),
                          static_cast<size_t>(out - base64.data()));
}

template <typename T>
absl::StatusOr<LocationWith<T>> ParseIntInner(JsonLexer& lex, double lo,
                                              double hi) {
  absl::StatusOr<JsonLexer::Kind> kind = lex.PeekKind();
  RETURN_IF_ERROR(kind.status());

  LocationWith<T> n;
  switch (*kind) {
    case JsonLexer::kNum: {
      absl::StatusOr<LocationWith<MaybeOwnedString>> x = lex.ParseRawNumber();
      RETURN_IF_ERROR(x.status());
      n.loc = x->loc;
      if (absl::SimpleAtoi(x->value.AsView(), &n.value)) {
        break;
      }

      double d;
      if (!absl::SimpleAtod(x->value.AsView(), &d) || !std::isfinite(d)) {
        return x->loc.Invalid(
            absl::StrFormat("invalid number: '%s'", x->value.AsView()));
      }

      // Conversion overflow here would be UB.
      if (lo > d || d > hi) {
        return lex.Invalid("JSON number out of range for int");
      }
      n.value = static_cast<T>(d);
      if (d - static_cast<double>(n.value) != 0) {
        return lex.Invalid(
            "expected integer, but JSON number had fractional part");
      }
      break;
    }
    case JsonLexer::kStr: {
      absl::StatusOr<LocationWith<MaybeOwnedString>> str = lex.ParseUtf8();
      RETURN_IF_ERROR(str.status());
      // SimpleAtoi will ignore leading and trailing whitespace, so we need
      // to check for it ourselves.
      for (char c : str->value.AsView()) {
        if (absl::ascii_isspace(c)) {
          return lex.Invalid("non-number characters in quoted number");
        }
      }
      if (!absl::SimpleAtoi(str->value.AsView(), &n.value)) {
        return str->loc.Invalid("non-number characters in quoted number");
      }
      n.loc = str->loc;
      break;
    }
    default:
      return lex.Invalid("expected number or string");
  }

  return n;
}

template <typename Traits>
absl::StatusOr<int64_t> ParseInt(JsonLexer& lex, Field<Traits> field) {
  absl::StatusOr<LocationWith<int64_t>> n =
      ParseIntInner<int64_t>(lex, -9007199254740992.0, 9007199254740992.0);
  RETURN_IF_ERROR(n.status());

  if (Traits::Is32Bit(field)) {
    if (std::numeric_limits<int32_t>::min() > n->value ||
        n->value > std::numeric_limits<int32_t>::max()) {
      return n->loc.Invalid("integer out of range");
    }
  }

  return n->value;
}

template <typename Traits>
absl::StatusOr<uint64_t> ParseUInt(JsonLexer& lex, Field<Traits> field) {
  absl::StatusOr<LocationWith<uint64_t>> n =
      ParseIntInner<uint64_t>(lex, 0, 18014398509481984.0);
  RETURN_IF_ERROR(n.status());

  if (Traits::Is32Bit(field)) {
    if (n->value > std::numeric_limits<uint32_t>::max()) {
      return n->loc.Invalid("integer out of range");
    }
  }

  return n->value;
}

template <typename Traits>
absl::StatusOr<double> ParseFp(JsonLexer& lex, Field<Traits> field) {
  absl::StatusOr<JsonLexer::Kind> kind = lex.PeekKind();
  RETURN_IF_ERROR(kind.status());

  double n;
  switch (*kind) {
    case JsonLexer::kNum: {
      absl::StatusOr<LocationWith<double>> d = lex.ParseNumber();
      RETURN_IF_ERROR(d.status());
      n = d->value;
      break;
    }
    case JsonLexer::kStr: {
      absl::StatusOr<LocationWith<MaybeOwnedString>> str = lex.ParseUtf8();
      RETURN_IF_ERROR(str.status());

      if (str->value == "NaN") {
        n = NAN;
      } else if (str->value == "Infinity") {
        n = INFINITY;
      } else if (str->value == "-Infinity") {
        n = -INFINITY;
      } else if (!absl::SimpleAtod(str->value.AsView(), &n)) {
        return str->loc.Invalid("non-number characters in quoted number");
      }
      break;
    }
    default:
      return lex.Invalid("expected number or string");
  }

  if (Traits::Is32Bit(field)) {
    // Detect out-of-range 32-bit floats by seeing whether the conversion result
    // is still finite. Finite extreme values may have textual representations
    // that parse to 64-bit values outside the 32-bit range, but which are
    // closer to the 32-bit extreme than to the "next value with the same
    // precision".
    if (std::isfinite(n) && !std::isfinite(static_cast<float>(n))) {
      return lex.Invalid("float out of range");
    }
  }

  return n;
}

template <typename Traits>
absl::StatusOr<std::string> ParseStrOrBytes(JsonLexer& lex,
                                            Field<Traits> field) {
  absl::StatusOr<LocationWith<MaybeOwnedString>> str = lex.ParseUtf8();
  RETURN_IF_ERROR(str.status());

  if (Traits::FieldType(field) == FieldDescriptor::TYPE_BYTES) {
    std::string& b64 = str->value.ToString();
    absl::StatusOr<absl::Span<char>> decoded =
        DecodeBase64InPlace(absl::MakeSpan(&b64[0], b64.size()));
    if (!decoded.ok()) {
      return str->loc.Invalid(decoded.status().message());
    }
    b64.resize(decoded->size());
  }

  return std::move(str->value.ToString());
}

template <typename Traits>
absl::StatusOr<absl::optional<int32_t>> ParseEnumFromStr(JsonLexer& lex,
                                                         MaybeOwnedString& str,
                                                         Field<Traits> field) {
  absl::StatusOr<int32_t> value = Traits::EnumNumberByName(
      field, str.AsView(), lex.options().case_insensitive_enum_parsing);
  if (value.ok()) {
    return absl::optional<int32_t>(*value);
  }

  int32_t i;
  if (absl::SimpleAtoi(str.AsView(), &i)) {
    return absl::optional<int32_t>(i);
  } else if (lex.options().ignore_unknown_fields) {
    return {absl::nullopt};
  }

  return value.status();
}

// Parses an enum; can return nullopt if a quoted enumerator that we don't
// know about is received and `ignore_unknown_fields` is set.
template <typename Traits>
absl::StatusOr<absl::optional<int32_t>> ParseEnum(JsonLexer& lex,
                                                  Field<Traits> field) {
  absl::StatusOr<JsonLexer::Kind> kind = lex.PeekKind();
  RETURN_IF_ERROR(kind.status());

  int32_t n = 0;
  switch (*kind) {
    case JsonLexer::kStr: {
      absl::StatusOr<LocationWith<MaybeOwnedString>> str = lex.ParseUtf8();
      RETURN_IF_ERROR(str.status());

      auto e = ParseEnumFromStr<Traits>(lex, str->value, field);
      RETURN_IF_ERROR(e.status());
      if (!e->has_value()) {
        return {absl::nullopt};
      }
      n = **e;
      break;
    }
    case JsonLexer::kNum:
      return ParseInt<Traits>(lex, field);
    default:
      return lex.Invalid("expected number or string");
  }

  return n;
}

// Mutually recursive with functions that follow.
template <typename Traits>
absl::Status ParseMessage(JsonLexer& lex, const Desc<Traits>& desc,
                          Msg<Traits>& msg, bool any_reparse);
template <typename Traits>
absl::Status ParseField(JsonLexer& lex, const Desc<Traits>& desc,
                        absl::string_view name, Msg<Traits>& msg);

template <typename Traits>
absl::Status ParseSingular(JsonLexer& lex, Field<Traits> field,
                           Msg<Traits>& msg) {
  auto field_type = Traits::FieldType(field);
  if (lex.Peek(JsonLexer::kNull)) {
    auto message_type = ClassifyMessage(Traits::FieldTypeName(field));
    switch (field_type) {
      case FieldDescriptor::TYPE_ENUM:
        if (message_type == MessageType::kNull) {
          Traits::SetEnum(field, msg, 0);
        }
        break;
      case FieldDescriptor::TYPE_MESSAGE: {
        if (message_type == MessageType::kValue) {
          return Traits::NewMsg(
              field, msg,
              [&](const Desc<Traits>& type, Msg<Traits>& msg) -> absl::Status {
                auto field = Traits::FieldByNumber(type, 1);
                GOOGLE_DCHECK(field.has_value());
                RETURN_IF_ERROR(lex.Expect("null"));
                Traits::SetEnum(Traits::MustHaveField(type, 1), msg, 0);
                return absl::OkStatus();
              });
        }
        break;
      }
      default:
        break;
    }
    return lex.Expect("null");
  }

  switch (field_type) {
    case FieldDescriptor::TYPE_FLOAT: {
      auto x = ParseFp<Traits>(lex, field);
      RETURN_IF_ERROR(x.status());
      Traits::SetFloat(field, msg, *x);
      break;
    }
    case FieldDescriptor::TYPE_DOUBLE: {
      auto x = ParseFp<Traits>(lex, field);
      RETURN_IF_ERROR(x.status());
      Traits::SetDouble(field, msg, *x);
      break;
    }

    case FieldDescriptor::TYPE_SFIXED64:
    case FieldDescriptor::TYPE_SINT64:
    case FieldDescriptor::TYPE_INT64: {
      auto x = ParseInt<Traits>(lex, field);
      RETURN_IF_ERROR(x.status());
      Traits::SetInt64(field, msg, *x);
      break;
    }
    case FieldDescriptor::TYPE_FIXED64:
    case FieldDescriptor::TYPE_UINT64: {
      auto x = ParseUInt<Traits>(lex, field);
      RETURN_IF_ERROR(x.status());
      Traits::SetUInt64(field, msg, *x);
      break;
    }

    case FieldDescriptor::TYPE_SFIXED32:
    case FieldDescriptor::TYPE_SINT32:
    case FieldDescriptor::TYPE_INT32: {
      auto x = ParseInt<Traits>(lex, field);
      RETURN_IF_ERROR(x.status());
      Traits::SetInt32(field, msg, static_cast<int32_t>(*x));
      break;
    }
    case FieldDescriptor::TYPE_FIXED32:
    case FieldDescriptor::TYPE_UINT32: {
      auto x = ParseUInt<Traits>(lex, field);
      RETURN_IF_ERROR(x.status());
      Traits::SetUInt32(field, msg, static_cast<uint32_t>(*x));
      break;
    }
    case FieldDescriptor::TYPE_BOOL: {
      absl::StatusOr<JsonLexer::Kind> kind = lex.PeekKind();
      RETURN_IF_ERROR(kind.status());

      switch (*kind) {
        case JsonLexer::kTrue:
          RETURN_IF_ERROR(lex.Expect("true"));
          Traits::SetBool(field, msg, true);
          break;
        case JsonLexer::kFalse:
          RETURN_IF_ERROR(lex.Expect("false"));
          Traits::SetBool(field, msg, false);
          break;
        case JsonLexer::kStr: {
          if (!lex.options().allow_legacy_syntax) {
            goto bad;
          }

          auto x = lex.ParseUtf8();
          RETURN_IF_ERROR(x.status());

          bool flag;
          if (!absl::SimpleAtob(x->value, &flag)) {
            // Is this error a lie? Do we accept things otyher than "true" and
            // "false" because SimpleAtob does? Absolutely!
            return x->loc.Invalid("expected 'true' or 'false'");
          }
          Traits::SetBool(field, msg, flag);

          break;
        }
        bad:
        default:
          return lex.Invalid("expected 'true' or 'false'");
      }
      break;
    }
    case FieldDescriptor::TYPE_STRING:
    case FieldDescriptor::TYPE_BYTES: {
      auto x = ParseStrOrBytes<Traits>(lex, field);
      RETURN_IF_ERROR(x.status());
      Traits::SetString(field, msg, *x);
      break;
    }
    case FieldDescriptor::TYPE_ENUM: {
      absl::StatusOr<absl::optional<int32_t>> x = ParseEnum<Traits>(lex, field);
      RETURN_IF_ERROR(x.status());

      if (x->has_value() || !Traits::IsOptional(field)) {
        Traits::SetEnum(field, msg, x->value_or(0));
      }
      break;
    }
    case FieldDescriptor::TYPE_MESSAGE:
    case FieldDescriptor::TYPE_GROUP: {
      return Traits::NewMsg(
          field, msg,
          [&](const Desc<Traits>& type, Msg<Traits>& msg) -> absl::Status {
            return ParseMessage<Traits>(lex, type, msg,
                                        /*any_reparse=*/false);
          });
    }
    default:
      return lex.Invalid(
          absl::StrCat("unsupported field type: ", Traits::FieldType(field)));
  }

  return absl::OkStatus();
}

template <typename Traits>
absl::Status EmitNull(JsonLexer& lex, Field<Traits> field, Msg<Traits>& msg) {
  switch (Traits::FieldType(field)) {
    case FieldDescriptor::TYPE_FLOAT:
      Traits::SetFloat(field, msg, 0);
      break;
    case FieldDescriptor::TYPE_DOUBLE:
      Traits::SetDouble(field, msg, 0);
      break;
    case FieldDescriptor::TYPE_SFIXED64:
    case FieldDescriptor::TYPE_SINT64:
    case FieldDescriptor::TYPE_INT64:
      Traits::SetInt64(field, msg, 0);
      break;
    case FieldDescriptor::TYPE_FIXED64:
    case FieldDescriptor::TYPE_UINT64:
      Traits::SetUInt64(field, msg, 0);
      break;
    case FieldDescriptor::TYPE_SFIXED32:
    case FieldDescriptor::TYPE_SINT32:
    case FieldDescriptor::TYPE_INT32:
      Traits::SetInt32(field, msg, 0);
      break;
    case FieldDescriptor::TYPE_FIXED32:
    case FieldDescriptor::TYPE_UINT32:
      Traits::SetUInt32(field, msg, 0);
      break;
    case FieldDescriptor::TYPE_BOOL:
      Traits::SetBool(field, msg, false);
      break;
    case FieldDescriptor::TYPE_STRING:
    case FieldDescriptor::TYPE_BYTES:
      Traits::SetString(field, msg, "");
      break;
    case FieldDescriptor::TYPE_ENUM:
      Traits::SetEnum(field, msg, 0);
      break;
    case FieldDescriptor::TYPE_MESSAGE:
    case FieldDescriptor::TYPE_GROUP:
      return Traits::NewMsg(field, msg,
                            [](const auto&, const auto&) -> absl::Status {
                              return absl::OkStatus();
                            });
    default:
      return lex.Invalid(
          absl::StrCat("unsupported field type: ", Traits::FieldType(field)));
  }
  return absl::OkStatus();
}

template <typename Traits>
absl::Status ParseArray(JsonLexer& lex, Field<Traits> field, Msg<Traits>& msg) {
  if (lex.Peek(JsonLexer::kNull)) {
    return lex.Expect("null");
  }

  return lex.VisitArray([&]() -> absl::Status {
    lex.path().NextRepeated();
    MessageType type = ClassifyMessage(Traits::FieldTypeName(field));

    if (lex.Peek(JsonLexer::kNull)) {
      if (type == MessageType::kValue) {
        return ParseSingular<Traits>(lex, field, msg);
      }
      if (type == MessageType::kNull) {
        return ParseSingular<Traits>(lex, field, msg);
      }

      if (lex.options().allow_legacy_syntax) {
        RETURN_IF_ERROR(lex.Expect("null"));
        return EmitNull<Traits>(lex, field, msg);
      }
      return lex.Invalid("null cannot occur inside of repeated fields");
    }

    // Note that this is sufficient to catch when we are inside of a ListValue,
    // because a ListValue's sole field is of type Value. Thus, we only need to
    // classify cases in which we are inside of an array and parsing messages
    // that like looking like arrays.
    //
    // This will also correctly handle e.g. writing out a ListValue with the
    // legacy syntax of `{"values": [[0], [1], [2]]}`, which does not go through
    // the custom parser handler.
    bool can_flatten =
        type != MessageType::kValue && type != MessageType::kList;
    if (can_flatten && lex.options().allow_legacy_syntax &&
        lex.Peek(JsonLexer::kArr)) {
      // You read that right. In legacy mode, if we encounter an array within
      // an array, we just flatten it as part of the current array!
      //
      // This DOES NOT apply when parsing a google.protobuf.Value or a
      // google.protobuf.ListValue!
      return ParseArray<Traits>(lex, field, msg);
    }
    return ParseSingular<Traits>(lex, field, msg);
  });
}

template <typename Traits>
absl::Status ParseMap(JsonLexer& lex, Field<Traits> field, Msg<Traits>& msg) {
  if (lex.Peek(JsonLexer::kNull)) {
    return lex.Expect("null");
  }

  absl::flat_hash_set<std::string> keys_seen;
  return lex.VisitObject(
      [&](LocationWith<MaybeOwnedString>& key) -> absl::Status {
        lex.path().NextRepeated();
        auto insert_result = keys_seen.emplace(key.value.AsView());
        if (!insert_result.second) {
          return key.loc.Invalid(absl::StrFormat(
              "got unexpectedly-repeated repeated map key: '%s'",
              key.value.AsView()));
        }
        return Traits::NewMsg(
            field, msg,
            [&](const Desc<Traits>& type, Msg<Traits>& entry) -> absl::Status {
              auto key_field = Traits::KeyField(type);
              switch (Traits::FieldType(key_field)) {
                case FieldDescriptor::TYPE_INT64:
                case FieldDescriptor::TYPE_SINT64:
                case FieldDescriptor::TYPE_SFIXED64: {
                  int64_t n;
                  if (!absl::SimpleAtoi(key.value.AsView(), &n)) {
                    return key.loc.Invalid(
                        "non-number characters in quoted number");
                  }
                  Traits::SetInt64(key_field, entry, n);
                  break;
                }
                case FieldDescriptor::TYPE_UINT64:
                case FieldDescriptor::TYPE_FIXED64: {
                  uint64_t n;
                  if (!absl::SimpleAtoi(key.value.AsView(), &n)) {
                    return key.loc.Invalid(
                        "non-number characters in quoted number");
                  }
                  Traits::SetUInt64(key_field, entry, n);
                  break;
                }
                case FieldDescriptor::TYPE_INT32:
                case FieldDescriptor::TYPE_SINT32:
                case FieldDescriptor::TYPE_SFIXED32: {
                  int32_t n;
                  if (!absl::SimpleAtoi(key.value.AsView(), &n)) {
                    return key.loc.Invalid(
                        "non-number characters in quoted number");
                  }
                  Traits::SetInt32(key_field, entry, n);
                  break;
                }
                case FieldDescriptor::TYPE_UINT32:
                case FieldDescriptor::TYPE_FIXED32: {
                  uint32_t n;
                  if (!absl::SimpleAtoi(key.value.AsView(), &n)) {
                    return key.loc.Invalid(
                        "non-number characters in quoted number");
                  }
                  Traits::SetUInt32(key_field, entry, n);
                  break;
                }
                case FieldDescriptor::TYPE_BOOL: {
                  if (key.value == "true") {
                    Traits::SetBool(key_field, entry, true);
                  } else if (key.value == "false") {
                    Traits::SetBool(key_field, entry, false);
                  } else {
                    return key.loc.Invalid(absl::StrFormat(
                        "expected bool string, got '%s'", key.value.AsView()));
                  }
                  break;
                }
                case FieldDescriptor::TYPE_ENUM: {
                  MaybeOwnedString key_str = key.value;
                  auto e = ParseEnumFromStr<Traits>(lex, key_str, field);
                  RETURN_IF_ERROR(e.status());
                  Traits::SetEnum(key_field, entry, e->value_or(0));
                  break;
                }
                case FieldDescriptor::TYPE_STRING: {
                  Traits::SetString(key_field, entry,
                                    std::move(key.value.ToString()));
                  break;
                }
                default:
                  return lex.Invalid("unsupported map key type");
              }

              return ParseSingular<Traits>(lex, Traits::ValueField(type),
                                           entry);
            });
      });
}

absl::optional<uint32_t> TakeTimeDigitsWithSuffixAndAdvance(
    absl::string_view& data, int max_digits, absl::string_view end) {
  GOOGLE_DCHECK_LE(max_digits, 9);

  uint32_t val = 0;
  int limit = max_digits;
  while (!data.empty()) {
    if (limit-- < 0) {
      return absl::nullopt;
    }
    uint32_t digit = data[0] - '0';
    if (digit >= 10) {
      break;
    }

    val *= 10;
    val += digit;
    data = data.substr(1);
  }
  if (!absl::StartsWith(data, end)) {
    return absl::nullopt;
  }

  data = data.substr(end.size());
  return val;
}

absl::optional<int32_t> TakeNanosAndAdvance(absl::string_view& data) {
  int32_t frac_secs = 0;
  size_t frac_digits = 0;
  if (absl::StartsWith(data, ".")) {
    for (char c : data.substr(1)) {
      if (!absl::ascii_isdigit(c)) {
        break;
      }
      ++frac_digits;
    }
    auto digits = data.substr(1, frac_digits);
    if (frac_digits == 0 || frac_digits > 9 ||
        !absl::SimpleAtoi(digits, &frac_secs)) {
      return absl::nullopt;
    }
    data = data.substr(frac_digits + 1);
  }
  for (int i = 0; i < 9 - frac_digits; ++i) {
    frac_secs *= 10;
  }
  return frac_secs;
}

template <typename Traits>
absl::Status ParseTimestamp(JsonLexer& lex, const Desc<Traits>& desc,
                            Msg<Traits>& msg) {
  if (lex.Peek(JsonLexer::kNull)) {
    return lex.Expect("null");
  }

  absl::StatusOr<LocationWith<MaybeOwnedString>> str = lex.ParseUtf8();
  RETURN_IF_ERROR(str.status());

  absl::string_view data = str->value.AsView();
  if (data.size() < 20) {
    return str->loc.Invalid("timestamp string too short");
  }

  int64_t secs;
  {
    /* 1972-01-01T01:00:00 */
    auto year = TakeTimeDigitsWithSuffixAndAdvance(data, 4, "-");
    if (!year.has_value() || *year == 0) {
      return str->loc.Invalid("bad year in timestamp");
    }
    auto mon = TakeTimeDigitsWithSuffixAndAdvance(data, 2, "-");
    if (!mon.has_value() || *mon == 0) {
      return str->loc.Invalid("bad month in timestamp");
    }
    auto day = TakeTimeDigitsWithSuffixAndAdvance(data, 2, "T");
    if (!day.has_value() || *day == 0) {
      return str->loc.Invalid("bad day in timestamp");
    }
    auto hour = TakeTimeDigitsWithSuffixAndAdvance(data, 2, ":");
    if (!hour.has_value()) {
      return str->loc.Invalid("bad hours in timestamp");
    }
    auto min = TakeTimeDigitsWithSuffixAndAdvance(data, 2, ":");
    if (!min.has_value()) {
      return str->loc.Invalid("bad minutes in timestamp");
    }
    auto sec = TakeTimeDigitsWithSuffixAndAdvance(data, 2, "");
    if (!sec.has_value()) {
      return str->loc.Invalid("bad seconds in timestamp");
    }

    uint32_t m_adj = *mon - 3;  // March-based month.
    uint32_t carry = m_adj > *mon ? 1 : 0;

    uint32_t year_base = 4800;  // Before min year, multiple of 400.
    uint32_t y_adj = *year + year_base - carry;

    uint32_t month_days = ((m_adj + carry * 12) * 62719 + 769) / 2048;
    uint32_t leap_days = y_adj / 4 - y_adj / 100 + y_adj / 400;
    int32_t epoch_days =
        y_adj * 365 + leap_days + month_days + (*day - 1) - 2472632;

    secs = int64_t{epoch_days} * 86400 + *hour * 3600 + *min * 60 + *sec;
  }

  auto nanos = TakeNanosAndAdvance(data);
  if (!nanos.has_value()) {
    return str->loc.Invalid("timestamp had bad nanoseconds");
  }

  if (data.empty()) {
    return str->loc.Invalid("timestamp missing timezone offset");
  }

  {
    // [+-]hh:mm or Z
    bool neg = false;
    switch (data[0]) {
      case '-':
        neg = true;
        ABSL_FALLTHROUGH_INTENDED;
      case '+': {
        if (data.size() != 6) {
          return str->loc.Invalid("timestamp offset of wrong size.");
        }

        data = data.substr(1);
        auto hour = TakeTimeDigitsWithSuffixAndAdvance(data, 2, ":");
        auto mins = TakeTimeDigitsWithSuffixAndAdvance(data, 2, "");
        if (!hour.has_value() || !mins.has_value()) {
          return str->loc.Invalid("timestamp offset has bad hours and minutes");
        }

        int64_t offset = (*hour * 60 + *mins) * 60;
        secs += (neg ? offset : -offset);
        break;
      }
      // Lowercase z is not accepted, per the spec.
      case 'Z':
        if (data.size() == 1) {
          break;
        }
        ABSL_FALLTHROUGH_INTENDED;
      default:
        return str->loc.Invalid("bad timezone offset");
    }
  }

  Traits::SetInt64(Traits::MustHaveField(desc, 1), msg, secs);
  Traits::SetInt32(Traits::MustHaveField(desc, 2), msg, *nanos);

  return absl::OkStatus();
}

template <typename Traits>
absl::Status ParseDuration(JsonLexer& lex, const Desc<Traits>& desc,
                           Msg<Traits>& msg) {
  if (lex.Peek(JsonLexer::kNull)) {
    return lex.Expect("null");
  }

  constexpr int64_t kMaxSeconds = int64_t{3652500} * 86400;

  absl::StatusOr<LocationWith<MaybeOwnedString>> str = lex.ParseUtf8();
  RETURN_IF_ERROR(str.status());

  size_t int_part_end = 0;
  for (char c : str->value.AsView()) {
    if (!absl::ascii_isdigit(c) && c != '-') {
      break;
    }
    ++int_part_end;
  }
  if (int_part_end == 0) {
    return str->loc.Invalid("duration must start with an integer");
  }

  absl::string_view sec_digits = str->value.AsView().substr(0, int_part_end);
  int64_t secs;
  if (!absl::SimpleAtoi(sec_digits, &secs)) {
    return str->loc.Invalid("duration had bad seconds");
  }

  if (secs > kMaxSeconds || secs < -kMaxSeconds) {
    return str->loc.Invalid("duration out of range");
  }

  absl::string_view rest = str->value.AsView().substr(int_part_end);
  auto nanos = TakeNanosAndAdvance(rest);
  if (!nanos.has_value()) {
    return str->loc.Invalid("duration had bad nanoseconds");
  }

  bool isNegative = (secs < 0) || absl::StartsWith(sec_digits, "-");
  if (isNegative) {
    *nanos *= -1;
  }

  if (rest != "s") {
    return str->loc.Invalid("duration must end with a single 's'");
  }

  Traits::SetInt64(Traits::MustHaveField(desc, 1), msg, secs);
  Traits::SetInt32(Traits::MustHaveField(desc, 2), msg, *nanos);

  return absl::OkStatus();
}

template <typename Traits>
absl::Status ParseFieldMask(JsonLexer& lex, const Desc<Traits>& desc,
                            Msg<Traits>& msg) {
  absl::StatusOr<LocationWith<MaybeOwnedString>> str = lex.ParseUtf8();
  RETURN_IF_ERROR(str.status());
  auto paths = str->value.AsView();

  // The special case of the empty string is not handled correctly below,
  // because StrSplit("", ',') is [""], not [].
  if (paths.empty()) {
    return absl::OkStatus();
  }

  // google.protobuf.FieldMask has a single field with number 1.
  auto paths_field = Traits::MustHaveField(desc, 1);
  for (absl::string_view path : absl::StrSplit(paths, ',')) {
    std::string snake_path;
    // Assume approximately six-letter words, so add one extra space for an
    // underscore for every six bytes.
    snake_path.reserve(path.size() * 7 / 6);
    for (char c : path) {
      if (absl::ascii_isdigit(c) || absl::ascii_islower(c) || c == '.') {
        snake_path.push_back(c);
      } else if (absl::ascii_isupper(c)) {
        snake_path.push_back('_');
        snake_path.push_back(absl::ascii_tolower(c));
      } else if (lex.options().allow_legacy_syntax) {
        snake_path.push_back(c);
      } else {
        return str->loc.Invalid("unexpected character in FieldMask");
      }
    }
    Traits::SetString(paths_field, msg, snake_path);
  }

  return absl::OkStatus();
}

template <typename Traits>
absl::Status ParseAny(JsonLexer& lex, const Desc<Traits>& desc,
                      Msg<Traits>& msg) {
  // Buffer an entire object. Because @type can occur anywhere, we're forced
  // to do this.
  RETURN_IF_ERROR(lex.SkipToToken());
  auto mark = lex.BeginMark();

  // Search for @type, buffering the entire object along the way so we can
  // reparse it.
  absl::optional<MaybeOwnedString> type_url;
  RETURN_IF_ERROR(lex.VisitObject(
      [&](const LocationWith<MaybeOwnedString>& key) -> absl::Status {
        if (key.value == "@type") {
          if (type_url.has_value()) {
            return key.loc.Invalid("repeated @type in Any");
          }

          absl::StatusOr<LocationWith<MaybeOwnedString>> maybe_url =
              lex.ParseUtf8();
          RETURN_IF_ERROR(maybe_url.status());
          type_url = std::move(maybe_url)->value;
          return absl::OkStatus();
        }
        return lex.SkipValue();
      }));

  // Build a new lexer over the skipped object.
  absl::string_view any_text = mark.value.UpToUnread();
  io::ArrayInputStream in(any_text.data(), any_text.size());
  // Copying lex.options() is important; it inherits the recursion
  // limit.
  JsonLexer any_lex(&in, lex.options(), &lex.path(), mark.loc);

  if (!type_url.has_value() && !lex.options().allow_legacy_syntax) {
    return mark.loc.Invalid("missing @type in Any");
  }

  if (type_url.has_value()) {
    Traits::SetString(Traits::MustHaveField(desc, 1), msg, type_url->AsView());
    return Traits::NewDynamic(
        Traits::MustHaveField(desc, 2), type_url->ToString(), msg,
        [&](const Desc<Traits>& desc, Msg<Traits>& msg) {
          auto pop = any_lex.path().Push("<any>", FieldDescriptor::TYPE_MESSAGE,
                                         Traits::TypeName(desc));
          return ParseMessage<Traits>(any_lex, desc, msg,
                                      /*any_reparse=*/true);
        });
  } else {
    // Empty {} is accepted in legacy mode.
    GOOGLE_DCHECK(lex.options().allow_legacy_syntax);
    RETURN_IF_ERROR(any_lex.VisitObject([&](auto&) {
      return mark.loc.Invalid(
          "in legacy mode, missing @type in Any is only allowed for an empty "
          "object");
    }));
    return absl::OkStatus();
  }
}

// These are mutually recursive with ParseValue.
template <typename Traits>
absl::Status ParseStructValue(JsonLexer& lex, const Desc<Traits>& desc,
                              Msg<Traits>& msg);
template <typename Traits>
absl::Status ParseListValue(JsonLexer& lex, const Desc<Traits>& desc,
                            Msg<Traits>& msg);

template <typename Traits>
absl::Status ParseValue(JsonLexer& lex, const Desc<Traits>& desc,
                        Msg<Traits>& msg) {
  auto kind = lex.PeekKind();
  RETURN_IF_ERROR(kind.status());
  // NOTE: The field numbers 1 through 6 are the numbers of the oneof fields
  // in google.protobuf.Value. Conformance tests verify the correctness of
  // these numbers.
  switch (*kind) {
    case JsonLexer::kNull: {
      auto field = Traits::MustHaveField(desc, 1);
      auto pop =
          lex.path().Push(Traits::FieldName(field), Traits::FieldType(field),
                          Traits::FieldTypeName(field));

      RETURN_IF_ERROR(lex.Expect("null"));
      Traits::SetEnum(field, msg, 0);
      break;
    }
    case JsonLexer::kNum: {
      auto field = Traits::MustHaveField(desc, 2);
      auto pop =
          lex.path().Push(Traits::FieldName(field), Traits::FieldType(field),
                          Traits::FieldTypeName(field));

      auto number = lex.ParseNumber();
      RETURN_IF_ERROR(number.status());
      Traits::SetDouble(field, msg, number->value);
      break;
    }
    case JsonLexer::kStr: {
      auto field = Traits::MustHaveField(desc, 3);
      auto pop =
          lex.path().Push(Traits::FieldName(field), Traits::FieldType(field),
                          Traits::FieldTypeName(field));

      auto str = lex.ParseUtf8();
      RETURN_IF_ERROR(str.status());
      Traits::SetString(field, msg, std::move(str->value.ToString()));
      break;
    }
    case JsonLexer::kFalse:
    case JsonLexer::kTrue: {
      auto field = Traits::MustHaveField(desc, 4);
      auto pop =
          lex.path().Push(Traits::FieldName(field), Traits::FieldType(field),
                          Traits::FieldTypeName(field));

      // "Quoted" bools, including non-standard Abseil Atob bools, are not
      // supported, because all strings are treated as genuine JSON strings.
      if (*kind == JsonLexer::kTrue) {
        RETURN_IF_ERROR(lex.Expect("true"));
        Traits::SetBool(field, msg, true);
      } else {
        RETURN_IF_ERROR(lex.Expect("false"));
        Traits::SetBool(field, msg, false);
      }
      break;
    }
    case JsonLexer::kObj: {
      auto field = Traits::MustHaveField(desc, 5);
      auto pop =
          lex.path().Push(Traits::FieldName(field), Traits::FieldType(field),
                          Traits::FieldTypeName(field));

      return Traits::NewMsg(field, msg, [&](auto& desc, auto& msg) {
        return ParseStructValue<Traits>(lex, desc, msg);
      });
    }
    case JsonLexer::kArr: {
      auto field = Traits::MustHaveField(desc, 6);
      auto pop =
          lex.path().Push(Traits::FieldName(field), Traits::FieldType(field),
                          Traits::FieldTypeName(field));

      return Traits::NewMsg(field, msg, [&](auto& desc, auto& msg) {
        return ParseListValue<Traits>(lex, desc, msg);
      });
    }
  }

  return absl::OkStatus();
}

template <typename Traits>
absl::Status ParseStructValue(JsonLexer& lex, const Desc<Traits>& desc,
                              Msg<Traits>& msg) {
  auto entry_field = Traits::MustHaveField(desc, 1);
  auto pop = lex.path().Push("<struct>", FieldDescriptor::TYPE_MESSAGE,
                             Traits::FieldTypeName(entry_field));

  // Structs are always cleared even if set to {}.
  Traits::RecordAsSeen(entry_field, msg);

  // Parsing a map does the right thing: Struct has a single map<string,
  // Value> field; keys are correctly parsed as strings, and the values
  // recurse into ParseMessage, which will be routed into ParseValue. This
  // results in some extra overhead, but performance is not what we're going
  // for here.
  return ParseMap<Traits>(lex, entry_field, msg);
}

template <typename Traits>
absl::Status ParseListValue(JsonLexer& lex, const Desc<Traits>& desc,
                            Msg<Traits>& msg) {
  auto entry_field = Traits::MustHaveField(desc, 1);
  auto pop = lex.path().Push("<list>", FieldDescriptor::TYPE_MESSAGE,
                             Traits::FieldTypeName(entry_field));

  // ListValues are always cleared even if set to [].
  Traits::RecordAsSeen(entry_field, msg);
  // Parsing an array does the right thing: see the analogous comment in
  // ParseStructValue.
  return ParseArray<Traits>(lex, entry_field, msg);
}

template <typename Traits>
absl::Status ParseField(JsonLexer& lex, const Desc<Traits>& desc,
                        absl::string_view name, Msg<Traits>& msg) {
  absl::optional<Field<Traits>> field;
  if (absl::StartsWith(name, "[") && absl::EndsWith(name, "]")) {
    absl::string_view extn_name = name.substr(1, name.size() - 2);
    field = Traits::ExtensionByName(desc, extn_name);
  } else {
    field = Traits::FieldByName(desc, name);
  }

  if (!field.has_value()) {
    if (!lex.options().ignore_unknown_fields) {
      return lex.Invalid(absl::StrFormat("no such field: '%s'", name));
    }
    return lex.SkipValue();
  }

  auto pop = lex.path().Push(name, Traits::FieldType(*field),
                             Traits::FieldTypeName(*field));

  if (Traits::HasParsed(
          *field, msg,
          /*allow_repeated_non_oneof=*/lex.options().allow_legacy_syntax) &&
      !lex.Peek(JsonLexer::kNull)) {
    return lex.Invalid(absl::StrFormat(
        "'%s' has already been set (either directly or as part of a oneof)",
        name));
  }

  if (Traits::IsMap(*field)) {
    return ParseMap<Traits>(lex, *field, msg);
  }

  if (Traits::IsRepeated(*field)) {
    if (lex.options().allow_legacy_syntax && !lex.Peek(JsonLexer::kArr)) {
      // The original ESF parser permits a single element in place of an array
      // thereof.
      return ParseSingular<Traits>(lex, *field, msg);
    }
    return ParseArray<Traits>(lex, *field, msg);
  }

  return ParseSingular<Traits>(lex, *field, msg);
}

template <typename Traits>
absl::Status ParseMessage(JsonLexer& lex, const Desc<Traits>& desc,
                          Msg<Traits>& msg, bool any_reparse) {
  MessageType type = ClassifyMessage(Traits::TypeName(desc));
  if (!any_reparse) {
    switch (type) {
      case MessageType::kAny:
        return ParseAny<Traits>(lex, desc, msg);
      case MessageType::kValue:
        return ParseValue<Traits>(lex, desc, msg);
      case MessageType::kStruct:
        return ParseStructValue<Traits>(lex, desc, msg);
      default:
        break;
    }
    // For some types, the ESF parser permits parsing the "non-special" version.
    // It is not clear if this counts as out-of-spec, but we're treating it as
    // such.
    bool is_upcoming_object = lex.Peek(JsonLexer::kObj);
    if (!(is_upcoming_object && lex.options().allow_legacy_syntax)) {
      switch (type) {
        case MessageType::kList:
          return ParseListValue<Traits>(lex, desc, msg);
        case MessageType::kWrapper: {
          return ParseSingular<Traits>(lex, Traits::MustHaveField(desc, 1),
                                       msg);
        }
        case MessageType::kTimestamp:
          return ParseTimestamp<Traits>(lex, desc, msg);
        case MessageType::kDuration:
          return ParseDuration<Traits>(lex, desc, msg);
        case MessageType::kFieldMask:
          return ParseFieldMask<Traits>(lex, desc, msg);
        default:
          break;
      }
    }
  }

  return lex.VisitObject(
      [&](LocationWith<MaybeOwnedString>& name) -> absl::Status {
        // If this is a well-known type, we expect its contents to be inside
        // of a JSON field named "value".
        if (any_reparse) {
          if (name.value == "@type") {
            RETURN_IF_ERROR(lex.SkipValue());
            return absl::OkStatus();
          }
          if (type != MessageType::kNotWellKnown) {
            if (name.value != "value") {
              return lex.Invalid(
                  "fields in a well-known-typed Any must be @type or value");
            }
            // Parse the upcoming value as the message itself. This is *not*
            // an Any reparse because we do not expect to see @type in the
            // upcoming value.
            return ParseMessage<Traits>(lex, desc, msg,
                                        /*any_reparse=*/false);
          }
        }

        return ParseField<Traits>(lex, desc, name.value.AsView(), msg);
      });
}
}  // namespace

absl::Status JsonStringToMessage(absl::string_view input, Message* message,
                                 json_internal::ParseOptions options) {
  MessagePath path(message->GetDescriptor()->full_name());
  PROTOBUF_DLOG(INFO) << "json2/input: " << absl::CHexEscape(input);
  io::ArrayInputStream in(input.data(), input.size());
  JsonLexer lex(&in, options, &path);

  ParseProto2Descriptor::Msg msg(message);
  absl::Status s =
      ParseMessage<ParseProto2Descriptor>(lex, *message->GetDescriptor(), msg,
                                          /*any_reparse=*/false);
  if (s.ok() && !lex.AtEof()) {
    s = absl::InvalidArgumentError(
        "extraneous characters after end of JSON object");
  }

  PROTOBUF_DLOG(INFO) << "json2/status: " << s;
  PROTOBUF_DLOG(INFO) << "json2/output: " << message->DebugString();

  return s;
}

absl::Status JsonToBinaryStream(google::protobuf::util::TypeResolver* resolver,
                                const std::string& type_url,
                                io::ZeroCopyInputStream* json_input,
                                io::ZeroCopyOutputStream* binary_output,
                                json_internal::ParseOptions options) {
  // NOTE: Most of the contortions in this function are to allow for capture of
  // input and output of the parser in GOOGLE_DLOG mode. Destruction order is very
  // critical in this function, because io::ZeroCopy*Stream types usually only
  // flush on destruction.

  // For GOOGLE_DLOG, we would like to print out the input and output, which requires
  // buffering both instead of doing "zero copy". This block, and the one at
  // the end of the function, set up and tear down interception of the input
  // and output streams.
  std::string copy;
  std::string out;
  absl::optional<io::ArrayInputStream> tee_input;
  absl::optional<io::StringOutputStream> tee_output;
  if (PROTOBUF_DEBUG) {
    const void* data;
    int len;
    while (json_input->Next(&data, &len)) {
      copy.resize(copy.size() + len);
      std::memcpy(&copy[copy.size() - len], data, len);
    }
    tee_input.emplace(copy.data(), copy.size());
    tee_output.emplace(&out);
  }

  PROTOBUF_DLOG(INFO) << "json2/input: " << absl::CHexEscape(copy);

  // This scope forces the CodedOutputStream inside of `msg` to flush before we
  // possibly handle logging the binary protobuf output.
  absl::Status s;
  {
    MessagePath path(type_url);
    JsonLexer lex(tee_input.has_value() ? &*tee_input : json_input, options,
                  &path);
    Msg<ParseProto3Type> msg(tee_output.has_value() ? &*tee_output
                                                    : binary_output);

    ResolverPool pool(resolver);
    auto desc = pool.FindMessage(type_url);
    RETURN_IF_ERROR(desc.status());

    s = ParseMessage<ParseProto3Type>(lex, **desc, msg, /*any_reparse=*/false);
    if (s.ok() && !lex.AtEof()) {
      s = absl::InvalidArgumentError(
          "extraneous characters after end of JSON object");
    }
  }

  if (PROTOBUF_DEBUG) {
    tee_output.reset();  // Flush the output stream.
    io::zc_sink_internal::ZeroCopyStreamByteSink(binary_output)
        .Append(out.data(), out.size());
  }

  PROTOBUF_DLOG(INFO) << "json2/status: " << s;
  PROTOBUF_DLOG(INFO) << "json2/output: " << absl::BytesToHexString(out);
  return s;
}
}  // namespace json_internal
}  // namespace protobuf
}  // namespace google
