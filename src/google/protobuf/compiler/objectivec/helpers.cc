// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "google/protobuf/compiler/objectivec/helpers.h"

#include <climits>
#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

#include "absl/log/absl_check.h"
#include "absl/log/absl_log.h"
#include "absl/strings/ascii.h"
#include "absl/strings/escaping.h"
#include "absl/strings/match.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_replace.h"
#include "absl/strings/str_split.h"
#include "absl/strings/string_view.h"
#include "absl/strings/strip.h"
#include "google/protobuf/compiler/objectivec/names.h"
#include "google/protobuf/compiler/objectivec/options.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/io/strtod.h"
#include "google/protobuf/stubs/common.h"

// NOTE: src/google/protobuf/compiler/plugin.cc makes use of cerr for some
// error cases, so it seems to be ok to use as a back door for errors.

namespace google {
namespace protobuf {
namespace compiler {
namespace objectivec {

std::string EscapeTrigraphs(absl::string_view to_escape) {
  return absl::StrReplaceAll(to_escape, {{"?", "\\?"}});
}

namespace {

std::string GetZeroEnumNameForFlagType(const FlagType flag_type) {
  switch (flag_type) {
    case FLAGTYPE_DESCRIPTOR_INITIALIZATION:
      return "GPBDescriptorInitializationFlag_None";
    case FLAGTYPE_EXTENSION:
      return "GPBExtensionNone";
    case FLAGTYPE_FIELD:
      return "GPBFieldNone";
    default:
      ABSL_LOG(FATAL) << "Can't get here.";
      return "0";
  }
}

std::string GetEnumNameForFlagType(const FlagType flag_type) {
  switch (flag_type) {
    case FLAGTYPE_DESCRIPTOR_INITIALIZATION:
      return "GPBDescriptorInitializationFlags";
    case FLAGTYPE_EXTENSION:
      return "GPBExtensionOptions";
    case FLAGTYPE_FIELD:
      return "GPBFieldFlags";
    default:
      ABSL_LOG(FATAL) << "Can't get here.";
      return std::string();
  }
}

std::string HandleExtremeFloatingPoint(std::string val, bool add_float_suffix) {
  if (val == "nan") {
    return "NAN";
  } else if (val == "inf") {
    return "INFINITY";
  } else if (val == "-inf") {
    return "-INFINITY";
  } else {
    // float strings with ., e or E need to have f appended
    if (add_float_suffix &&
        (absl::StrContains(val, '.') || absl::StrContains(val, 'e') ||
         absl::StrContains(val, 'E'))) {
      return absl::StrCat(val, "f");
    }
    return val;
  }
}

const char* kDescriptorProtoName = "google/protobuf/descriptor.proto";

}  // namespace

bool ExtensionIsCustomOption(const FieldDescriptor* extension_field) {
  return extension_field->containing_type()->file()->name() ==
         kDescriptorProtoName;
}

std::string GetCapitalizedType(const FieldDescriptor* field) {
  switch (field->type()) {
    case FieldDescriptor::TYPE_INT32:
      return "Int32";
    case FieldDescriptor::TYPE_UINT32:
      return "UInt32";
    case FieldDescriptor::TYPE_SINT32:
      return "SInt32";
    case FieldDescriptor::TYPE_FIXED32:
      return "Fixed32";
    case FieldDescriptor::TYPE_SFIXED32:
      return "SFixed32";
    case FieldDescriptor::TYPE_INT64:
      return "Int64";
    case FieldDescriptor::TYPE_UINT64:
      return "UInt64";
    case FieldDescriptor::TYPE_SINT64:
      return "SInt64";
    case FieldDescriptor::TYPE_FIXED64:
      return "Fixed64";
    case FieldDescriptor::TYPE_SFIXED64:
      return "SFixed64";
    case FieldDescriptor::TYPE_FLOAT:
      return "Float";
    case FieldDescriptor::TYPE_DOUBLE:
      return "Double";
    case FieldDescriptor::TYPE_BOOL:
      return "Bool";
    case FieldDescriptor::TYPE_STRING:
      return "String";
    case FieldDescriptor::TYPE_BYTES:
      return "Bytes";
    case FieldDescriptor::TYPE_ENUM:
      return "Enum";
    case FieldDescriptor::TYPE_GROUP:
      return "Group";
    case FieldDescriptor::TYPE_MESSAGE:
      return "Message";
  }

  // Some compilers report reaching end of function even though all cases of
  // the enum are handed in the switch.
  ABSL_LOG(FATAL) << "Can't get here.";
  return std::string();
}

ObjectiveCType GetObjectiveCType(FieldDescriptor::Type field_type) {
  switch (field_type) {
    case FieldDescriptor::TYPE_INT32:
    case FieldDescriptor::TYPE_SINT32:
    case FieldDescriptor::TYPE_SFIXED32:
      return OBJECTIVECTYPE_INT32;

    case FieldDescriptor::TYPE_UINT32:
    case FieldDescriptor::TYPE_FIXED32:
      return OBJECTIVECTYPE_UINT32;

    case FieldDescriptor::TYPE_INT64:
    case FieldDescriptor::TYPE_SINT64:
    case FieldDescriptor::TYPE_SFIXED64:
      return OBJECTIVECTYPE_INT64;

    case FieldDescriptor::TYPE_UINT64:
    case FieldDescriptor::TYPE_FIXED64:
      return OBJECTIVECTYPE_UINT64;

    case FieldDescriptor::TYPE_FLOAT:
      return OBJECTIVECTYPE_FLOAT;

    case FieldDescriptor::TYPE_DOUBLE:
      return OBJECTIVECTYPE_DOUBLE;

    case FieldDescriptor::TYPE_BOOL:
      return OBJECTIVECTYPE_BOOLEAN;

    case FieldDescriptor::TYPE_STRING:
      return OBJECTIVECTYPE_STRING;

    case FieldDescriptor::TYPE_BYTES:
      return OBJECTIVECTYPE_DATA;

    case FieldDescriptor::TYPE_ENUM:
      return OBJECTIVECTYPE_ENUM;

    case FieldDescriptor::TYPE_GROUP:
    case FieldDescriptor::TYPE_MESSAGE:
      return OBJECTIVECTYPE_MESSAGE;
  }

  // Some compilers report reaching end of function even though all cases of
  // the enum are handed in the switch.
  ABSL_LOG(FATAL) << "Can't get here.";
  return OBJECTIVECTYPE_INT32;
}

std::string GPBGenericValueFieldName(const FieldDescriptor* field) {
  // Returns the field within the GPBGenericValue union to use for the given
  // field.
  if (field->is_repeated()) {
    return "valueMessage";
  }
  switch (field->cpp_type()) {
    case FieldDescriptor::CPPTYPE_INT32:
      return "valueInt32";
    case FieldDescriptor::CPPTYPE_UINT32:
      return "valueUInt32";
    case FieldDescriptor::CPPTYPE_INT64:
      return "valueInt64";
    case FieldDescriptor::CPPTYPE_UINT64:
      return "valueUInt64";
    case FieldDescriptor::CPPTYPE_FLOAT:
      return "valueFloat";
    case FieldDescriptor::CPPTYPE_DOUBLE:
      return "valueDouble";
    case FieldDescriptor::CPPTYPE_BOOL:
      return "valueBool";
    case FieldDescriptor::CPPTYPE_STRING:
      if (field->type() == FieldDescriptor::TYPE_BYTES) {
        return "valueData";
      } else {
        return "valueString";
      }
    case FieldDescriptor::CPPTYPE_ENUM:
      return "valueEnum";
    case FieldDescriptor::CPPTYPE_MESSAGE:
      return "valueMessage";
  }

  // Some compilers report reaching end of function even though all cases of
  // the enum are handed in the switch.
  ABSL_LOG(FATAL) << "Can't get here.";
  return std::string();
}

std::string DefaultValue(const FieldDescriptor* field) {
  // Repeated fields don't have defaults.
  if (field->is_repeated()) {
    return "nil";
  }

  // Switch on cpp_type since we need to know which default_value_* method
  // of FieldDescriptor to call.
  switch (field->cpp_type()) {
    case FieldDescriptor::CPPTYPE_INT32:
      // gcc and llvm reject the decimal form of kint32min and kint64min.
      if (field->default_value_int32() == INT_MIN) {
        return "-0x80000000";
      }
      return absl::StrCat(field->default_value_int32());
    case FieldDescriptor::CPPTYPE_UINT32:
      return absl::StrCat(field->default_value_uint32(), "U");
    case FieldDescriptor::CPPTYPE_INT64:
      // gcc and llvm reject the decimal form of kint32min and kint64min.
      if (field->default_value_int64() == LLONG_MIN) {
        return "-0x8000000000000000LL";
      }
      return absl::StrCat(field->default_value_int64(), "LL");
    case FieldDescriptor::CPPTYPE_UINT64:
      return absl::StrCat(field->default_value_uint64(), "ULL");
    case FieldDescriptor::CPPTYPE_DOUBLE:
      return HandleExtremeFloatingPoint(
          io::SimpleDtoa(field->default_value_double()), false);
    case FieldDescriptor::CPPTYPE_FLOAT:
      return HandleExtremeFloatingPoint(
          io::SimpleFtoa(field->default_value_float()), true);
    case FieldDescriptor::CPPTYPE_BOOL:
      return field->default_value_bool() ? "YES" : "NO";
    case FieldDescriptor::CPPTYPE_STRING: {
      const bool has_default_value = field->has_default_value();
      absl::string_view default_string = field->default_value_string();
      if (!has_default_value || default_string.empty()) {
        // If the field is defined as being the empty string,
        // then we will just assign to nil, as the empty string is the
        // default for both strings and data.
        return "nil";
      }
      if (field->type() == FieldDescriptor::TYPE_BYTES) {
        // We want constant fields in our data structures so we can
        // declare them as static. To achieve this we cheat and stuff
        // a escaped c string (prefixed with a length) into the data
        // field, and cast it to an (NSData*) so it will compile.
        // The runtime library knows how to handle it.

        // Must convert to a standard byte order for packing length into
        // a cstring.
        uint32_t length = ghtonl((uint32_t)default_string.length());
        std::string bytes((const char*)&length, sizeof(length));
        absl::StrAppend(&bytes, default_string);
        return absl::StrCat("(NSData*)\"",
                            EscapeTrigraphs(absl::CEscape(bytes)), "\"");
      } else {
        return absl::StrCat(
            "@\"", EscapeTrigraphs(absl::CEscape(default_string)), "\"");
      }
    }
    case FieldDescriptor::CPPTYPE_ENUM:
      return EnumValueName(field->default_value_enum());
    case FieldDescriptor::CPPTYPE_MESSAGE:
      return "nil";
  }

  // Some compilers report reaching end of function even though all cases of
  // the enum are handed in the switch.
  ABSL_LOG(FATAL) << "Can't get here.";
  return std::string();
}

std::string BuildFlagsString(FlagType flag_type,
                             const std::vector<std::string>& strings) {
  if (strings.empty()) {
    return GetZeroEnumNameForFlagType(flag_type);
  } else if (strings.size() == 1) {
    return strings[0];
  }
  std::string string =
      absl::StrCat("(", GetEnumNameForFlagType(flag_type), ")(");
  for (size_t i = 0; i != strings.size(); ++i) {
    if (i > 0) {
      string.append(" | ");
    }
    string.append(strings[i]);
  }
  string.append(")");
  return string;
}

std::string ObjCClass(absl::string_view class_name) {
  return absl::StrCat("GPBObjCClass(", class_name, ")");
}

std::string ObjCClassDeclaration(absl::string_view class_name) {
  return absl::StrCat("GPBObjCClassDeclaration(", class_name, ");");
}

void EmitCommentsString(io::Printer* printer, const GenerationOptions& opts,
                        const SourceLocation& location,
                        CommentStringFlags flags) {
  if (opts.experimental_strip_nonfunctional_codegen) {
    // Comments are inherently non-functional, and may change subtly on
    // transformations.
    return;
  }
  absl::string_view comments = location.leading_comments.empty()
                                   ? location.trailing_comments
                                   : location.leading_comments;
  std::vector<absl::string_view> raw_lines(
      absl::StrSplit(comments, '\n', absl::AllowEmpty()));
  while (!raw_lines.empty() && raw_lines.back().empty()) {
    raw_lines.pop_back();
  }
  if (raw_lines.empty()) {
    return;
  }

  std::vector<std::string> lines;
  lines.reserve(raw_lines.size());
  for (absl::string_view l : raw_lines) {
    lines.push_back(absl::StrReplaceAll(
        // Strip any trailing whitespace to avoid any warnings on the generated
        // code; but only strip one leading white space as that tends to be
        // carried over from the .proto file, and we don't want extra spaces,
        // the formatting below will ensure there is a space.
        // NOTE: There could be >1 leading whitespace if the .proto file has
        // formatted comments (see the WKTs), so we maintain any additional
        // leading whitespace.
        absl::StripTrailingAsciiWhitespace(absl::StripPrefix(l, " ")),
        {// HeaderDoc and appledoc use '\' and '@' for markers; escape them.
         {"\\", "\\\\"},
         {"@", "\\@"},
         // Decouple / from * to not have inline comments inside comments.
         {"/*", "/\\*"},
         {"*/", "*\\/"}}));
  }

  if (flags & kCommentStringFlags_AddLeadingNewline) {
    printer->Emit("\n");
  }

  if ((flags & kCommentStringFlags_ForceMultiline) == 0 && lines.size() == 1) {
    printer->Emit({{"text", lines[0]}}, R"(
      /** $text$ */
    )");
    return;
  }

  printer->Emit(
      {
          {"lines",
           [&] {
             for (absl::string_view line : lines) {
               printer->Emit({{"text", line}}, R"(
                *$ text$
              )");
             }
           }},
      },
      R"(
        /**
         $lines$
         **/
      )");
}

bool HasWKTWithObjCCategory(const FileDescriptor* file) {
  // We don't check the name prefix or proto package because some files
  // (descriptor.proto), aren't shipped generated by the library, so this
  // seems to be the safest way to only catch the ones shipped.
  const absl::string_view name = file->name();
  if (name == "google/protobuf/any.proto" ||
      name == "google/protobuf/duration.proto" ||
      name == "google/protobuf/timestamp.proto") {
    ABSL_DCHECK(IsProtobufLibraryBundledProtoFile(file));
    return true;
  }
  return false;
}

bool IsWKTWithObjCCategory(const Descriptor* descriptor) {
  if (!HasWKTWithObjCCategory(descriptor->file())) {
    return false;
  }
  const absl::string_view full_name = descriptor->full_name();
  if (full_name == "google.protobuf.Any" ||
      full_name == "google.protobuf.Duration" ||
      full_name == "google.protobuf.Timestamp") {
    return true;
  }
  return false;
}

void SubstitutionMap::Set(io::Printer::Sub&& sub) {
  if (auto [it, inserted] = subs_map_.try_emplace(sub.key(), subs_.size());
      !inserted) {
    subs_[it->second] = std::move(sub);
  } else {
    subs_.emplace_back(std::move(sub));
  }
}

}  // namespace objectivec
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
