// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "google/protobuf/json/internal/unparser.h"

#include <cmath>
#include <cstdint>
#include <cstring>
#include <limits>
#include <optional>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include "absl/log/absl_check.h"
#include "absl/log/absl_log.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/ascii.h"
#include "absl/strings/escaping.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_format.h"
#include "absl/strings/string_view.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/dynamic_message.h"
#include "google/protobuf/io/coded_stream.h"
#include "google/protobuf/io/zero_copy_sink.h"
#include "google/protobuf/io/zero_copy_stream.h"
#include "google/protobuf/io/zero_copy_stream_impl_lite.h"
#include "google/protobuf/json/internal/descriptor_traits.h"
#include "google/protobuf/json/internal/unparser_traits.h"
#include "google/protobuf/json/internal/writer.h"
#include "google/protobuf/message.h"
#include "google/protobuf/util/type_resolver.h"
#include "google/protobuf/stubs/status_macros.h"

// Must be included last.
#include "google/protobuf/port_def.inc"

namespace google {
namespace protobuf {
namespace json_internal {
namespace {
template <typename Traits>
bool IsEmpty(const Msg<Traits>& msg, const Desc<Traits>& desc) {
  size_t count = Traits::FieldCount(desc);
  for (size_t i = 0; i < count; ++i) {
    if (Traits::GetSize(Traits::FieldByIndex(desc, i), msg) > 0) {
      return false;
    }
  }
  return true;
}

enum class IntegerEnumStyle {
  kQuoted,
  kUnquoted,
};

template <typename Traits>
void WriteEnum(JsonWriter& writer, Field<Traits> field, int32_t value,
               IntegerEnumStyle int_style = IntegerEnumStyle::kUnquoted) {
  if (ClassifyMessage(Traits::FieldTypeName(field)) == MessageType::kNull) {
    writer.Write("null");
    return;
  }

  if (!writer.options().always_print_enums_as_ints) {
    auto name = Traits::EnumNameByNumber(field, value);
    if (name.ok()) {
      writer.Write("\"", *name, "\"");
      return;
    }
  }

  if (int_style == IntegerEnumStyle::kQuoted) {
    writer.Write("\"", value, "\"");
  } else {
    writer.Write(value);
  }
}

// Returns true if x round-trips through being cast to a double, i.e., if
// x is represenable exactly as a double. This is a slightly weaker condition
// than x < 2^52.
template <typename Int>
bool RoundTripsThroughDouble(Int x) {
  auto d = static_cast<double>(x);
  // d has guaranteed to be finite with no fractional part, because it came from
  // an integer, so we only need to check that it is not outside of the
  // representable range of `int`. The way to do this is somewhat not obvious:
  // UINT64_MAX isn't representable, and what it gets rounded to when we go
  // int->double is unspecified!
  //
  // Thus, we have to go through ldexp.
  double min = 0;
  double max_plus_one = std::ldexp(1.0, sizeof(Int) * 8);
  if (std::is_signed<Int>::value) {
    max_plus_one /= 2;
    min = -max_plus_one;
  }

  if (d < min || d >= max_plus_one) {
    return false;
  }

  return static_cast<Int>(d) == x;
}

// Mutually recursive with functions that follow.
template <typename Traits>
absl::Status WriteMessage(JsonWriter& writer, const Msg<Traits>& msg,
                          const Desc<Traits>& desc, bool is_top_level = false);

// This is templatized so that defaults, singular, and repeated fields can both
// use the same enormous switch-case.
template <typename Traits, typename... Args>
absl::Status WriteSingular(JsonWriter& writer, Field<Traits> field,
                           Args&&... args) {
  // When the pack `args` is empty, the caller has requested printing the
  // default value.
  bool is_default = sizeof...(Args) == 0;
  switch (Traits::FieldType(field)) {
    case FieldDescriptor::TYPE_FLOAT: {
      auto x = Traits::GetFloat(field, std::forward<Args>(args)...);
      RETURN_IF_ERROR(x.status());
      if (writer.options().allow_legacy_syntax && is_default &&
          !std::isfinite(*x)) {
        *x = 0;
      }
      writer.Write(*x);
      break;
    }
    case FieldDescriptor::TYPE_DOUBLE: {
      auto x = Traits::GetDouble(field, std::forward<Args>(args)...);
      RETURN_IF_ERROR(x.status());
      if (writer.options().allow_legacy_syntax && is_default &&
          !std::isfinite(*x)) {
        *x = 0;
      }
      writer.Write(*x);
      break;
    }
    case FieldDescriptor::TYPE_SFIXED64:
    case FieldDescriptor::TYPE_SINT64:
    case FieldDescriptor::TYPE_INT64: {
      auto x = Traits::GetInt64(field, std::forward<Args>(args)...);
      RETURN_IF_ERROR(x.status());
      if (writer.options().unquote_int64_if_possible &&
          RoundTripsThroughDouble(*x)) {
        writer.Write(*x);
      } else {
        writer.Write(MakeQuoted(*x));
      }
      break;
    }
    case FieldDescriptor::TYPE_FIXED64:
    case FieldDescriptor::TYPE_UINT64: {
      auto x = Traits::GetUInt64(field, std::forward<Args>(args)...);
      RETURN_IF_ERROR(x.status());
      if (writer.options().unquote_int64_if_possible &&
          RoundTripsThroughDouble(*x)) {
        writer.Write(*x);
      } else {
        writer.Write(MakeQuoted(*x));
      }
      break;
    }
    case FieldDescriptor::TYPE_SFIXED32:
    case FieldDescriptor::TYPE_SINT32:
    case FieldDescriptor::TYPE_INT32: {
      auto x = Traits::GetInt32(field, std::forward<Args>(args)...);
      RETURN_IF_ERROR(x.status());
      writer.Write(*x);
      break;
    }
    case FieldDescriptor::TYPE_FIXED32:
    case FieldDescriptor::TYPE_UINT32: {
      auto x = Traits::GetUInt32(field, std::forward<Args>(args)...);
      RETURN_IF_ERROR(x.status());
      writer.Write(*x);
      break;
    }
    case FieldDescriptor::TYPE_BOOL: {
      auto x = Traits::GetBool(field, std::forward<Args>(args)...);
      RETURN_IF_ERROR(x.status());
      writer.Write(*x ? "true" : "false");
      break;
    }
    case FieldDescriptor::TYPE_STRING: {
      auto x = Traits::GetString(field, writer.ScratchBuf(),
                                 std::forward<Args>(args)...);
      RETURN_IF_ERROR(x.status());
      writer.Write(MakeQuoted(*x));
      break;
    }
    case FieldDescriptor::TYPE_BYTES: {
      auto x = Traits::GetString(field, writer.ScratchBuf(),
                                 std::forward<Args>(args)...);
      RETURN_IF_ERROR(x.status());
      if (writer.options().allow_legacy_syntax && is_default) {
        // Although difficult to verify, it appears that the original ESF parser
        // fails to unescape the contents of a
        // google.protobuf.Field.default_value, which may potentially be
        // escaped if it is for a `bytes` field (note that default_value is a
        // `string` regardless of what type the field is).
        //
        // However, our parser's type.proto guts actually know to do this
        // correctly, so this bug must be manually re-introduced.
        writer.WriteBase64(absl::CEscape(*x));
      } else {
        writer.WriteBase64(*x);
      }
      break;
    }
    case FieldDescriptor::TYPE_ENUM: {
      auto x = Traits::GetEnumValue(field, std::forward<Args>(args)...);
      RETURN_IF_ERROR(x.status());
      WriteEnum<Traits>(writer, field, *x);
      break;
    }
    case FieldDescriptor::TYPE_MESSAGE:
    case FieldDescriptor::TYPE_GROUP: {
      auto x = Traits::GetMessage(field, std::forward<Args>(args)...);
      RETURN_IF_ERROR(x.status());
      return WriteMessage<Traits>(writer, **x, Traits::GetDesc(**x));
    }
    default:
      return absl::InvalidArgumentError(
          absl::StrCat("unsupported field type: ", Traits::FieldType(field)));
  }

  return absl::OkStatus();
}

template <typename Traits>
absl::Status WriteRepeated(JsonWriter& writer, const Msg<Traits>& msg,
                           Field<Traits> field) {
  writer.Write("[");
  writer.Push();

  size_t count = Traits::GetSize(field, msg);
  bool first = true;
  for (size_t i = 0; i < count; ++i) {
    if (ClassifyMessage(Traits::FieldTypeName(field)) == MessageType::kValue) {
      bool empty = false;
      RETURN_IF_ERROR(Traits::WithFieldType(
          field, [&](const Desc<Traits>& desc) -> absl::Status {
            auto inner = Traits::GetMessage(field, msg, i);
            RETURN_IF_ERROR(inner.status());
            empty = IsEmpty<Traits>(**inner, desc);
            return absl::OkStatus();
          }));

      // Empty google.protobuf.Values are silently discarded.
      if (empty) {
        continue;
      }
    }
    writer.WriteComma(first);
    writer.NewLine();
    RETURN_IF_ERROR(WriteSingular<Traits>(writer, field, msg, i));
  }

  writer.Pop();
  if (!first) {
    writer.NewLine();
  }
  writer.Write("]");
  return absl::OkStatus();
}

template <typename Traits>
absl::Status WriteMapKey(JsonWriter& writer, const Msg<Traits>& entry,
                         Field<Traits> field) {
  switch (Traits::FieldType(field)) {
    case FieldDescriptor::TYPE_SFIXED64:
    case FieldDescriptor::TYPE_SINT64:
    case FieldDescriptor::TYPE_INT64: {
      auto x = Traits::GetInt64(field, entry);
      RETURN_IF_ERROR(x.status());
      writer.Write(MakeQuoted(*x));
      break;
    }
    case FieldDescriptor::TYPE_FIXED64:
    case FieldDescriptor::TYPE_UINT64: {
      auto x = Traits::GetUInt64(field, entry);
      RETURN_IF_ERROR(x.status());
      writer.Write(MakeQuoted(*x));
      break;
    }
    case FieldDescriptor::TYPE_SFIXED32:
    case FieldDescriptor::TYPE_SINT32:
    case FieldDescriptor::TYPE_INT32: {
      auto x = Traits::GetInt32(field, entry);
      RETURN_IF_ERROR(x.status());
      writer.Write(MakeQuoted(*x));
      break;
    }
    case FieldDescriptor::TYPE_FIXED32:
    case FieldDescriptor::TYPE_UINT32: {
      auto x = Traits::GetUInt32(field, entry);
      RETURN_IF_ERROR(x.status());
      writer.Write(MakeQuoted(*x));
      break;
    }
    case FieldDescriptor::TYPE_BOOL: {
      auto x = Traits::GetBool(field, entry);
      RETURN_IF_ERROR(x.status());
      writer.Write(MakeQuoted(*x ? "true" : "false"));
      break;
    }
    case FieldDescriptor::TYPE_STRING: {
      auto x = Traits::GetString(field, writer.ScratchBuf(), entry);
      RETURN_IF_ERROR(x.status());
      writer.Write(MakeQuoted(*x));
      break;
    }
    case FieldDescriptor::TYPE_ENUM: {
      auto x = Traits::GetEnumValue(field, entry);
      RETURN_IF_ERROR(x.status());
      WriteEnum<Traits>(writer, field, *x, IntegerEnumStyle::kQuoted);
      break;
    }
    default:
      return absl::InvalidArgumentError(
          absl::StrCat("unsupported map key type: ", Traits::FieldType(field)));
  }
  return absl::OkStatus();
}

template <typename Traits>
absl::StatusOr<bool> IsEmptyValue(const Msg<Traits>& msg, Field<Traits> field) {
  if (ClassifyMessage(Traits::FieldTypeName(field)) != MessageType::kValue) {
    return false;
  }
  bool empty = false;
  RETURN_IF_ERROR(Traits::WithFieldType(
      field, [&](const Desc<Traits>& desc) -> absl::Status {
        auto inner = Traits::GetMessage(field, msg);
        RETURN_IF_ERROR(inner.status());
        empty = IsEmpty<Traits>(**inner, desc);
        return absl::OkStatus();
      }));
  return empty;
}

template <typename Traits>
absl::Status WriteMap(JsonWriter& writer, const Msg<Traits>& msg,
                      Field<Traits> field) {
  writer.Write("{");
  writer.Push();

  size_t count = Traits::GetSize(field, msg);
  bool first = true;
  for (size_t i = 0; i < count; ++i) {
    absl::StatusOr<const Msg<Traits>*> entry =
        Traits::GetMessage(field, msg, i);
    RETURN_IF_ERROR(entry.status());
    const Desc<Traits>& type = Traits::GetDesc(**entry);

    auto is_empty = IsEmptyValue<Traits>(**entry, Traits::ValueField(type));
    RETURN_IF_ERROR(is_empty.status());
    if (*is_empty) {
      // Empty google.protobuf.Values are silently discarded.
      continue;
    }

    writer.WriteComma(first);
    writer.NewLine();
    RETURN_IF_ERROR(
        WriteMapKey<Traits>(writer, **entry, Traits::KeyField(type)));
    writer.Write(":");
    writer.Whitespace(" ");
    RETURN_IF_ERROR(
        WriteSingular<Traits>(writer, Traits::ValueField(type), **entry));
  }

  writer.Pop();
  if (!first) {
    writer.NewLine();
  }
  writer.Write("}");
  return absl::OkStatus();
}

template <typename Traits>
absl::Status WriteField(JsonWriter& writer, const Msg<Traits>& msg,
                        Field<Traits> field, bool& first) {
  if (!Traits::IsRepeated(field)) {  // Repeated case is handled in
                                     // WriteRepeated.
    auto is_empty = IsEmptyValue<Traits>(msg, field);
    RETURN_IF_ERROR(is_empty.status());
    if (*is_empty) {
      // Empty google.protobuf.Values are silently discarded.
      return absl::OkStatus();
    }
  }

  writer.WriteComma(first);
  writer.NewLine();

  if (Traits::IsExtension(field)) {
    writer.Write(MakeQuoted("[", Traits::FieldFullName(field), "]"), ":");
  } else if (writer.options().preserve_proto_field_names) {
    writer.Write(MakeQuoted(Traits::FieldName(field)), ":");
  } else {
    // The generator for type.proto and the internals of descriptor.cc disagree
    // on what the json name of a PascalCase field is supposed to be; type.proto
    // seems to (incorrectly?) capitalize the first letter, which is the
    // behavior ESF defaults to. To fix this, if the original field name starts
    // with an uppercase letter, and the Json name does not, we uppercase it.
    absl::string_view original_name = Traits::FieldName(field);
    absl::string_view json_name = Traits::FieldJsonName(field);
    if (writer.options().allow_legacy_syntax &&
        absl::ascii_isupper(original_name[0]) &&
        !absl::ascii_isupper(json_name[0])) {
      writer.Write(MakeQuoted(absl::ascii_toupper(original_name[0]),
                              original_name.substr(1)),
                   ":");
    } else {
      writer.Write(MakeQuoted(json_name), ":");
    }
  }
  writer.Whitespace(" ");

  if (Traits::IsMap(field)) {
    return WriteMap<Traits>(writer, msg, field);
  } else if (Traits::IsRepeated(field)) {
    return WriteRepeated<Traits>(writer, msg, field);
  } else if (Traits::GetSize(field, msg) == 0) {

    if (Traits::FieldType(field) == FieldDescriptor::TYPE_GROUP) {
      // We do not yet have full group support, but this is required so that we
      // pass the same tests as the ESF parser.
      writer.Write("null");
      return absl::OkStatus();
    }
    return WriteSingular<Traits>(writer, field);
  }

  return WriteSingular<Traits>(writer, field, msg);
}

template <typename Traits>
absl::Status WriteFields(JsonWriter& writer, const Msg<Traits>& msg,
                         const Desc<Traits>& desc, bool& first) {
  std::vector<Field<Traits>> fields;
  size_t total = Traits::FieldCount(desc);
  fields.reserve(total);
  for (size_t i = 0; i < total; ++i) {
    Field<Traits> field = Traits::FieldByIndex(desc, i);

    bool has = Traits::GetSize(field, msg) > 0;

    if (writer.options().always_print_fields_with_no_presence) {
      has |= Traits::IsRepeated(field) || Traits::IsImplicitPresence(field);
    }

    if (has) {
      fields.push_back(field);
    }
  }

  // Add extensions *before* sorting.
  Traits::FindAndAppendExtensions(msg, fields);

  // Fields are guaranteed to be serialized in field number order.
  absl::c_sort(fields, [](const auto& a, const auto& b) {
    return Traits::FieldNumber(a) < Traits::FieldNumber(b);
  });

  for (auto field : fields) {
    RETURN_IF_ERROR(WriteField<Traits>(writer, msg, field, first));
  }

  return absl::OkStatus();
}

template <typename Traits>
absl::Status WriteStructValue(JsonWriter& writer, const Msg<Traits>& msg,
                              const Desc<Traits>& desc);
template <typename Traits>
absl::Status WriteListValue(JsonWriter& writer, const Msg<Traits>& msg,
                            const Desc<Traits>& desc);

template <typename Traits>
absl::Status WriteValue(JsonWriter& writer, const Msg<Traits>& msg,
                        const Desc<Traits>& desc, bool is_top_level) {
  // NOTE: The field numbers 1 through 6 are the numbers of the oneof fields in
  // google.protobuf.Value. Conformance tests verify the correctness of these
  // numbers.
  if (Traits::GetSize(Traits::MustHaveField(desc, 1), msg) > 0) {
    writer.Write("null");
    return absl::OkStatus();
  }

  auto number_field = Traits::MustHaveField(desc, 2);
  if (Traits::GetSize(number_field, msg) > 0) {
    auto x = Traits::GetDouble(number_field, msg);
    RETURN_IF_ERROR(x.status());
    if (std::isnan(*x)) {
      return absl::InvalidArgumentError(
          "google.protobuf.Value cannot encode double values for nan, "
          "because it would be parsed as a string");
    }
    if (*x == std::numeric_limits<double>::infinity() ||
        *x == -std::numeric_limits<double>::infinity()) {
      return absl::InvalidArgumentError(
          "google.protobuf.Value cannot encode double values for "
          "infinity, because it would be parsed as a string");
    }
    writer.Write(*x);
    return absl::OkStatus();
  }

  auto string_field = Traits::MustHaveField(desc, 3);
  if (Traits::GetSize(string_field, msg) > 0) {
    auto x = Traits::GetString(string_field, writer.ScratchBuf(), msg);
    RETURN_IF_ERROR(x.status());
    writer.Write(MakeQuoted(*x));
    return absl::OkStatus();
  }

  auto bool_field = Traits::MustHaveField(desc, 4);
  if (Traits::GetSize(bool_field, msg) > 0) {
    auto x = Traits::GetBool(bool_field, msg);
    RETURN_IF_ERROR(x.status());
    writer.Write(*x ? "true" : "false");
    return absl::OkStatus();
  }

  auto struct_field = Traits::MustHaveField(desc, 5);
  if (Traits::GetSize(struct_field, msg) > 0) {
    auto x = Traits::GetMessage(struct_field, msg);
    RETURN_IF_ERROR(x.status());
    return Traits::WithFieldType(struct_field, [&](const Desc<Traits>& type) {
      return WriteStructValue<Traits>(writer, **x, type);
    });
  }

  auto list_field = Traits::MustHaveField(desc, 6);
  if (Traits::GetSize(list_field, msg) > 0) {
    auto x = Traits::GetMessage(list_field, msg);
    RETURN_IF_ERROR(x.status());
    return Traits::WithFieldType(list_field, [&](const Desc<Traits>& type) {
      return WriteListValue<Traits>(writer, **x, type);
    });
  }

  ABSL_CHECK(is_top_level)
      << "empty, non-top-level Value must be handled one layer "
         "up, since it prints an empty string; reaching this "
         "statement is always a bug";
  return absl::OkStatus();
}

template <typename Traits>
absl::Status WriteStructValue(JsonWriter& writer, const Msg<Traits>& msg,
                              const Desc<Traits>& desc) {
  return WriteMap<Traits>(writer, msg, Traits::MustHaveField(desc, 1));
}

template <typename Traits>
absl::Status WriteListValue(JsonWriter& writer, const Msg<Traits>& msg,
                            const Desc<Traits>& desc) {
  return WriteRepeated<Traits>(writer, msg, Traits::MustHaveField(desc, 1));
}

template <typename Traits>
absl::Status WriteTimestamp(JsonWriter& writer, const Msg<Traits>& msg,
                            const Desc<Traits>& desc) {
  auto secs_field = Traits::MustHaveField(desc, 1);
  auto secs = Traits::GetSize(secs_field, msg) > 0
                  ? Traits::GetInt64(secs_field, msg)
                  : 0;
  RETURN_IF_ERROR(secs.status());

  if (*secs < -62135596800) {
    return absl::InvalidArgumentError(
        "minimum acceptable time value is 0001-01-01T00:00:00Z");
  } else if (*secs > 253402300799) {
    return absl::InvalidArgumentError(
        "maximum acceptable time value is 9999-12-31T23:59:59Z");
  }

  // Ensure seconds is positive.
  *secs += 62135596800;

  auto nanos_field = Traits::MustHaveField(desc, 2);
  auto nanos = Traits::GetSize(nanos_field, msg) > 0
                   ? Traits::GetInt32(nanos_field, msg)
                   : 0;
  RETURN_IF_ERROR(nanos.status());

  // Julian Day -> Y/M/D, Algorithm from:
  // Fliegel, H. F., and Van Flandern, T. C., "A Machine Algorithm for
  //   Processing Calendar Dates," Communications of the Association of
  //   Computing Machines, vol. 11 (1968), p. 657.
  int32_t L, N, I, J, K;
  L = static_cast<int32_t>(*secs / 86400) - 719162 + 68569 + 2440588;
  N = 4 * L / 146097;
  L = L - (146097 * N + 3) / 4;
  I = 4000 * (L + 1) / 1461001;
  L = L - 1461 * I / 4 + 31;
  J = 80 * L / 2447;
  K = L - 2447 * J / 80;
  L = J / 11;
  J = J + 2 - 12 * L;
  I = 100 * (N - 49) + I + L;

  int32_t sec = *secs % 60;
  int32_t min = (*secs / 60) % 60;
  int32_t hour = (*secs / 3600) % 24;

  if (*nanos == 0) {
    writer.Write(absl::StrFormat(R"("%04d-%02d-%02dT%02d:%02d:%02dZ")", I, J, K,
                                 hour, min, sec));
    return absl::OkStatus();
  }

  size_t digits = 9;
  uint32_t frac_seconds = std::abs(*nanos);
  while (frac_seconds % 1000 == 0) {
    frac_seconds /= 1000;
    digits -= 3;
  }

  writer.Write(absl::StrFormat(R"("%04d-%02d-%02dT%02d:%02d:%02d.%.*dZ")", I, J,
                               K, hour, min, sec, digits, frac_seconds));
  return absl::OkStatus();
}

template <typename Traits>
absl::Status WriteDuration(JsonWriter& writer, const Msg<Traits>& msg,
                           const Desc<Traits>& desc) {
  constexpr int64_t kMaxSeconds = int64_t{3652500} * 86400;
  constexpr int64_t kMaxNanos = 999999999;

  auto secs_field = Traits::MustHaveField(desc, 1);
  auto secs = Traits::GetSize(secs_field, msg) > 0
                  ? Traits::GetInt64(secs_field, msg)
                  : 0;
  RETURN_IF_ERROR(secs.status());

  if (*secs > kMaxSeconds || *secs < -kMaxSeconds) {
    return absl::InvalidArgumentError("duration out of range");
  }

  auto nanos_field = Traits::MustHaveField(desc, 2);
  auto nanos = Traits::GetSize(nanos_field, msg) > 0
                   ? Traits::GetInt32(nanos_field, msg)
                   : 0;
  RETURN_IF_ERROR(nanos.status());

  if (*nanos > kMaxNanos || *nanos < -kMaxNanos) {
    return absl::InvalidArgumentError("duration out of range");
  }
  if ((*secs != 0) && (*nanos != 0) && ((*secs < 0) != (*nanos < 0))) {
    return absl::InvalidArgumentError("nanos and seconds signs do not match");
  }

  if (*nanos == 0) {
    writer.Write(absl::StrFormat(R"("%ds")", *secs));
    return absl::OkStatus();
  }

  size_t digits = 9;
  uint32_t frac_seconds = std::abs(*nanos);
  while (frac_seconds % 1000 == 0) {
    frac_seconds /= 1000;
    digits -= 3;
  }

  absl::string_view sign = ((*secs < 0) || (*nanos < 0)) ? "-" : "";
  writer.Write(absl::StrFormat(R"("%s%d.%.*ds")", sign, std::abs(*secs), digits,
                               frac_seconds));
  return absl::OkStatus();
}

template <typename Traits>
absl::Status WriteFieldMask(JsonWriter& writer, const Msg<Traits>& msg,
                            const Desc<Traits>& desc) {
  // google.protobuf.FieldMask has a single field with number 1.
  auto paths_field = Traits::MustHaveField(desc, 1);
  size_t paths = Traits::GetSize(paths_field, msg);
  writer.Write('"');

  bool first = true;
  for (size_t i = 0; i < paths; ++i) {
    writer.WriteComma(first);
    auto path = Traits::GetString(paths_field, writer.ScratchBuf(), msg, i);
    RETURN_IF_ERROR(path.status());
    bool saw_under = false;
    for (char c : *path) {
      if (absl::ascii_islower(c) && saw_under) {
        writer.Write(absl::ascii_toupper(c));
      } else if (absl::ascii_isdigit(c) || absl::ascii_islower(c) || c == '.') {
        writer.Write(c);
      } else if (c == '_' &&
                 (!saw_under || writer.options().allow_legacy_syntax)) {
        saw_under = true;
        continue;
      } else if (!writer.options().allow_legacy_syntax) {
        return absl::InvalidArgumentError("unexpected character in FieldMask");
      } else {
        if (saw_under) {
          writer.Write('_');
        }
        writer.Write(c);
      }
      saw_under = false;
    }
  }
  writer.Write('"');

  return absl::OkStatus();
}

template <typename Traits>
absl::Status WriteAny(JsonWriter& writer, const Msg<Traits>& msg,
                      const Desc<Traits>& desc) {
  auto type_url_field = Traits::MustHaveField(desc, 1);
  auto value_field = Traits::MustHaveField(desc, 2);

  bool has_type_url = Traits::GetSize(type_url_field, msg) > 0;
  bool has_value = Traits::GetSize(value_field, msg) > 0;
  if (!has_type_url && !has_value) {
    writer.Write("{}");
    return absl::OkStatus();
  } else if (!has_type_url) {
    return absl::InvalidArgumentError("broken Any: missing type URL");
  } else if (!has_value && !writer.options().allow_legacy_syntax) {
    return absl::InvalidArgumentError("broken Any: missing value");
  }

  writer.Write("{");
  writer.Push();

  auto type_url = Traits::GetString(type_url_field, writer.ScratchBuf(), msg);
  RETURN_IF_ERROR(type_url.status());
  writer.NewLine();
  writer.Write("\"@type\":");
  writer.Whitespace(" ");
  writer.Write(MakeQuoted(*type_url));

  return Traits::WithDynamicType(
      desc, std::string(*type_url),
      [&](const Desc<Traits>& any_desc) -> absl::Status {
        absl::string_view any_bytes;
        if (has_value) {
          absl::StatusOr<absl::string_view> bytes =
              Traits::GetString(value_field, writer.ScratchBuf(), msg);
          RETURN_IF_ERROR(bytes.status());
          any_bytes = *bytes;
        }

        return Traits::WithDecodedMessage(
            any_desc, any_bytes,
            [&](const Msg<Traits>& unerased) -> absl::Status {
              bool first = false;
              if (ClassifyMessage(Traits::TypeName(any_desc)) !=
                  MessageType::kNotWellKnown) {
                writer.WriteComma(first);
                writer.NewLine();
                writer.Write("\"value\":");
                writer.Whitespace(" ");
                RETURN_IF_ERROR(
                    WriteMessage<Traits>(writer, unerased, any_desc));
              } else {
                RETURN_IF_ERROR(
                    WriteFields<Traits>(writer, unerased, any_desc, first));
              }
              writer.Pop();
              if (!first) {
                writer.NewLine();
              }
              writer.Write("}");
              return absl::OkStatus();
            });
      });
}

template <typename Traits>
absl::Status WriteMessage(JsonWriter& writer, const Msg<Traits>& msg,
                          const Desc<Traits>& desc, bool is_top_level) {
  switch (ClassifyMessage(Traits::TypeName(desc))) {
    case MessageType::kAny:
      return WriteAny<Traits>(writer, msg, desc);
    case MessageType::kWrapper: {
      auto field = Traits::MustHaveField(desc, 1);
      if (Traits::GetSize(field, msg) == 0) {
        return WriteSingular<Traits>(writer, field);
      }
      return WriteSingular<Traits>(writer, field, msg);
    }
    case MessageType::kValue:
      return WriteValue<Traits>(writer, msg, desc, is_top_level);
    case MessageType::kStruct:
      return WriteStructValue<Traits>(writer, msg, desc);
    case MessageType::kList:
      return WriteListValue<Traits>(writer, msg, desc);
    case MessageType::kTimestamp:
      return WriteTimestamp<Traits>(writer, msg, desc);
    case MessageType::kDuration:
      return WriteDuration<Traits>(writer, msg, desc);
    case MessageType::kFieldMask:
      return WriteFieldMask<Traits>(writer, msg, desc);
    default: {
      writer.Write("{");
      writer.Push();
      bool first = true;
      RETURN_IF_ERROR(WriteFields<Traits>(writer, msg, desc, first));
      writer.Pop();
      if (!first) {
        writer.NewLine();
      }
      writer.Write("}");
      return absl::OkStatus();
    }
  }
}
}  // namespace

absl::Status MessageToJsonStream(const Message& message,
                                 io::ZeroCopyOutputStream* json_output,
                                 json_internal::WriterOptions options) {
  if (PROTOBUF_DEBUG) {
    ABSL_DLOG(INFO) << "json2/input: " << message.DebugString();
  }
  JsonWriter writer(json_output, options);
  absl::Status s = WriteMessage<UnparseProto2Descriptor>(
      writer, message, *message.GetDescriptor(), /*is_top_level=*/true);
  if (PROTOBUF_DEBUG) ABSL_DLOG(INFO) << "json2/status: " << s;
  RETURN_IF_ERROR(s);

  writer.NewLine();
  return absl::OkStatus();
}

absl::Status BinaryToJsonStream(google::protobuf::util::TypeResolver* resolver,
                                const std::string& type_url,
                                io::ZeroCopyInputStream* binary_input,
                                io::ZeroCopyOutputStream* json_output,
                                json_internal::WriterOptions options) {
  // NOTE: Most of the contortions in this function are to allow for capture of
  // input and output of the parser in ABSL_DLOG mode. Destruction order is very
  // critical in this function, because io::ZeroCopy*Stream types usually only
  // flush on destruction.

  // For ABSL_DLOG, we would like to print out the input and output, which
  // requires buffering both instead of doing "zero copy". This block, and the
  // one at the end of the function, set up and tear down interception of the
  // input and output streams.
  std::string copy;
  std::string out;
  std::optional<io::ArrayInputStream> tee_input;
  std::optional<io::StringOutputStream> tee_output;
  if (PROTOBUF_DEBUG) {
    const void* data;
    int len;
    while (binary_input->Next(&data, &len)) {
      copy.resize(copy.size() + len);
      std::memcpy(&copy[copy.size() - len], data, len);
    }
    tee_input.emplace(copy.data(), copy.size());
    tee_output.emplace(&out);
    ABSL_DLOG(INFO) << "json2/input: " << absl::BytesToHexString(copy);
  }

  ResolverPool pool(resolver);
  auto desc = pool.FindMessage(type_url);
  RETURN_IF_ERROR(desc.status());

  io::CodedInputStream stream(tee_input.has_value() ? &*tee_input
                                                    : binary_input);
  auto msg = UntypedMessage::ParseFromStream(*desc, stream);
  RETURN_IF_ERROR(msg.status());

  JsonWriter writer(tee_output.has_value() ? &*tee_output : json_output,
                    options);
  absl::Status s = WriteMessage<UnparseProto3Type>(
      writer, *msg, UnparseProto3Type::GetDesc(*msg),
      /*is_top_level=*/true);
  if (PROTOBUF_DEBUG) ABSL_DLOG(INFO) << "json2/status: " << s;
  RETURN_IF_ERROR(s);

  if (PROTOBUF_DEBUG) {
    tee_output.reset();  // Flush the output stream.
    io::zc_sink_internal::ZeroCopyStreamByteSink(json_output)
        .Append(out.data(), out.size());
    ABSL_DLOG(INFO) << "json2/output: " << absl::CHexEscape(out);
  }

  writer.NewLine();
  return absl::OkStatus();
}
}  // namespace json_internal
}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"
