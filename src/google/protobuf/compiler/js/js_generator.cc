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

#include <google/protobuf/compiler/js/js_generator.h>

#include <assert.h>
#include <algorithm>
#include <limits>
#include <map>
#include <memory>
#ifndef _SHARED_PTR_H
#include <google/protobuf/stubs/shared_ptr.h>
#endif
#include <string>
#include <utility>
#include <vector>

#include <google/protobuf/stubs/logging.h>
#include <google/protobuf/stubs/common.h>
#include <google/protobuf/stubs/stringprintf.h>
#include <google/protobuf/compiler/js/well_known_types_embed.h>
#include <google/protobuf/io/printer.h>
#include <google/protobuf/io/zero_copy_stream.h>
#include <google/protobuf/descriptor.pb.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/stubs/strutil.h>

namespace google {
namespace protobuf {
namespace compiler {
namespace js {

// Sorted list of JavaScript keywords. These cannot be used as names. If they
// appear, we prefix them with "pb_".
const char* kKeyword[] = {
  "abstract",
  "boolean",
  "break",
  "byte",
  "case",
  "catch",
  "char",
  "class",
  "const",
  "continue",
  "debugger",
  "default",
  "delete",
  "do",
  "double",
  "else",
  "enum",
  "export",
  "extends",
  "false",
  "final",
  "finally",
  "float",
  "for",
  "function",
  "goto",
  "if",
  "implements",
  "import",
  "in",
  "instanceof",
  "int",
  "interface",
  "long",
  "native",
  "new",
  "null",
  "package",
  "private",
  "protected",
  "public",
  "return",
  "short",
  "static",
  "super",
  "switch",
  "synchronized",
  "this",
  "throw",
  "throws",
  "transient",
  "try",
  "typeof",
  "var",
  "void",
  "volatile",
  "while",
  "with",
};

static const int kNumKeyword = sizeof(kKeyword) / sizeof(char*);

namespace {

// The mode of operation for bytes fields. Historically JSPB always carried
// bytes as JS {string}, containing base64 content by convention. With binary
// and proto3 serialization the new convention is to represent it as binary
// data in Uint8Array. See b/26173701 for background on the migration.
enum BytesMode {
  BYTES_DEFAULT,  // Default type for getBytesField to return.
  BYTES_B64,      // Explicitly coerce to base64 string where needed.
  BYTES_U8,       // Explicitly coerce to Uint8Array where needed.
};

bool IsReserved(const string& ident) {
  for (int i = 0; i < kNumKeyword; i++) {
    if (ident == kKeyword[i]) {
      return true;
    }
  }
  return false;
}

// Returns a copy of |filename| with any trailing ".protodevel" or ".proto
// suffix stripped.
// TODO(haberman): Unify with copy in compiler/cpp/internal/helpers.cc.
string StripProto(const string& filename) {
  const char* suffix = HasSuffixString(filename, ".protodevel")
      ? ".protodevel" : ".proto";
  return StripSuffixString(filename, suffix);
}

// Given a filename like foo/bar/baz.proto, returns the corresponding JavaScript
// file foo/bar/baz.js.
string GetJSFilename(const GeneratorOptions& options, const string& filename) {
  return StripProto(filename) + options.GetFileNameExtension();
}

// Given a filename like foo/bar/baz.proto, returns the root directory
// path ../../
string GetRootPath(const string& from_filename, const string& to_filename) {
  if (to_filename.find("google/protobuf") == 0) {
    // Well-known types (.proto files in the google/protobuf directory) are
    // assumed to come from the 'google-protobuf' npm package.  We may want to
    // generalize this exception later by letting others put generated code in
    // their own npm packages.
    return "google-protobuf/";
  }

  size_t slashes = std::count(from_filename.begin(), from_filename.end(), '/');
  if (slashes == 0) {
    return "./";
  }
  string result = "";
  for (size_t i = 0; i < slashes; i++) {
    result += "../";
  }
  return result;
}

// Returns the alias we assign to the module of the given .proto filename
// when importing.
string ModuleAlias(const string& filename) {
  // This scheme could technically cause problems if a file includes any 2 of:
  //   foo/bar_baz.proto
  //   foo_bar_baz.proto
  //   foo_bar/baz.proto
  //
  // We'll worry about this problem if/when we actually see it.  This name isn't
  // exposed to users so we can change it later if we need to.
  string basename = StripProto(filename);
  StripString(&basename, "-", '$');
  StripString(&basename, "/", '_');
  return basename + "_pb";
}

// Returns the fully normalized JavaScript path for the given
// file descriptor's package.
string GetPath(const GeneratorOptions& options,
               const FileDescriptor* file) {
  if (!options.namespace_prefix.empty()) {
    return options.namespace_prefix;
  } else if (!file->package().empty()) {
    return "proto." + file->package();
  } else {
    return "proto";
  }
}

// Returns the name of the message with a leading dot and taking into account
// nesting, for example ".OuterMessage.InnerMessage", or returns empty if
// descriptor is null. This function does not handle namespacing, only message
// nesting.
string GetNestedMessageName(const Descriptor* descriptor) {
  if (descriptor == NULL) {
    return "";
  }
  string result =
      StripPrefixString(descriptor->full_name(), descriptor->file()->package());
  // Add a leading dot if one is not already present.
  if (!result.empty() && result[0] != '.') {
    result = "." + result;
  }
  return result;
}

// Returns the path prefix for a message or enumeration that
// lives under the given file and containing type.
string GetPrefix(const GeneratorOptions& options,
                 const FileDescriptor* file_descriptor,
                 const Descriptor* containing_type) {
  string prefix =
      GetPath(options, file_descriptor) + GetNestedMessageName(containing_type);
  if (!prefix.empty()) {
    prefix += ".";
  }
  return prefix;
}


// Returns the fully normalized JavaScript path for the given
// message descriptor.
string GetPath(const GeneratorOptions& options,
               const Descriptor* descriptor) {
  return GetPrefix(
      options, descriptor->file(),
      descriptor->containing_type()) + descriptor->name();
}


// Returns the fully normalized JavaScript path for the given
// field's containing message descriptor.
string GetPath(const GeneratorOptions& options,
               const FieldDescriptor* descriptor) {
  return GetPath(options, descriptor->containing_type());
}

// Returns the fully normalized JavaScript path for the given
// enumeration descriptor.
string GetPath(const GeneratorOptions& options,
               const EnumDescriptor* enum_descriptor) {
  return GetPrefix(
      options, enum_descriptor->file(),
      enum_descriptor->containing_type()) + enum_descriptor->name();
}


// Returns the fully normalized JavaScript path for the given
// enumeration value descriptor.
string GetPath(const GeneratorOptions& options,
               const EnumValueDescriptor* value_descriptor) {
  return GetPath(
      options,
      value_descriptor->type()) + "." + value_descriptor->name();
}

string MaybeCrossFileRef(const GeneratorOptions& options,
                         const FileDescriptor* from_file,
                         const Descriptor* to_message) {
  if (options.import_style == GeneratorOptions::kImportCommonJs &&
      from_file != to_message->file()) {
    // Cross-file ref in CommonJS needs to use the module alias instead of
    // the global name.
    return ModuleAlias(to_message->file()->name()) +
           GetNestedMessageName(to_message->containing_type()) + "." +
           to_message->name();
  } else {
    // Within a single file we use a full name.
    return GetPath(options, to_message);
  }
}

string SubmessageTypeRef(const GeneratorOptions& options,
                         const FieldDescriptor* field) {
  GOOGLE_CHECK(field->cpp_type() == FieldDescriptor::CPPTYPE_MESSAGE);
  return MaybeCrossFileRef(options, field->file(), field->message_type());
}

// - Object field name: LOWER_UNDERSCORE -> LOWER_CAMEL, except for group fields
// (UPPER_CAMEL -> LOWER_CAMEL), with "List" (or "Map") appended if appropriate,
// and with reserved words triggering a "pb_" prefix.
// - Getters/setters: LOWER_UNDERSCORE -> UPPER_CAMEL, except for group fields
// (use the name directly), then append "List" if appropriate, then append "$"
// if resulting name is equal to a reserved word.
// - Enums: just uppercase.

// Locale-independent version of ToLower that deals only with ASCII A-Z.
char ToLowerASCII(char c) {
  if (c >= 'A' && c <= 'Z') {
    return (c - 'A') + 'a';
  } else {
    return c;
  }
}

std::vector<string> ParseLowerUnderscore(const string& input) {
  std::vector<string> words;
  string running = "";
  for (int i = 0; i < input.size(); i++) {
    if (input[i] == '_') {
      if (!running.empty()) {
        words.push_back(running);
        running.clear();
      }
    } else {
      running += ToLowerASCII(input[i]);
    }
  }
  if (!running.empty()) {
    words.push_back(running);
  }
  return words;
}

std::vector<string> ParseUpperCamel(const string& input) {
  std::vector<string> words;
  string running = "";
  for (int i = 0; i < input.size(); i++) {
    if (input[i] >= 'A' && input[i] <= 'Z' && !running.empty()) {
      words.push_back(running);
      running.clear();
    }
    running += ToLowerASCII(input[i]);
  }
  if (!running.empty()) {
    words.push_back(running);
  }
  return words;
}

string ToLowerCamel(const std::vector<string>& words) {
  string result;
  for (int i = 0; i < words.size(); i++) {
    string word = words[i];
    if (i == 0 && (word[0] >= 'A' && word[0] <= 'Z')) {
      word[0] = (word[0] - 'A') + 'a';
    } else if (i != 0 && (word[0] >= 'a' && word[0] <= 'z')) {
      word[0] = (word[0] - 'a') + 'A';
    }
    result += word;
  }
  return result;
}

string ToUpperCamel(const std::vector<string>& words) {
  string result;
  for (int i = 0; i < words.size(); i++) {
    string word = words[i];
    if (word[0] >= 'a' && word[0] <= 'z') {
      word[0] = (word[0] - 'a') + 'A';
    }
    result += word;
  }
  return result;
}

// Based on code from descriptor.cc (Thanks Kenton!)
// Uppercases the entire string, turning ValueName into
// VALUENAME.
string ToEnumCase(const string& input) {
  string result;
  result.reserve(input.size());

  for (int i = 0; i < input.size(); i++) {
    if ('a' <= input[i] && input[i] <= 'z') {
      result.push_back(input[i] - 'a' + 'A');
    } else {
      result.push_back(input[i]);
    }
  }

  return result;
}

string ToFileName(const string& input) {
  string result;
  result.reserve(input.size());

  for (int i = 0; i < input.size(); i++) {
    if ('A' <= input[i] && input[i] <= 'Z') {
      result.push_back(input[i] - 'A' + 'a');
    } else {
      result.push_back(input[i]);
    }
  }

  return result;
}

// When we're generating one output file per type name, this is the filename
// that top-level extensions should go in.
string GetExtensionFileName(const GeneratorOptions& options,
                            const FileDescriptor* file) {
  return options.output_dir + "/" + ToFileName(GetPath(options, file)) +
         options.GetFileNameExtension();
}

// When we're generating one output file per type name, this is the filename
// that a top-level message should go in.
string GetMessageFileName(const GeneratorOptions& options,
                          const Descriptor* desc) {
  return options.output_dir + "/" + ToFileName(desc->name()) +
         options.GetFileNameExtension();
}

// When we're generating one output file per type name, this is the filename
// that a top-level message should go in.
string GetEnumFileName(const GeneratorOptions& options,
                       const EnumDescriptor* desc) {
  return options.output_dir + "/" + ToFileName(desc->name()) +
         options.GetFileNameExtension();
}

// Returns the message/response ID, if set.
string GetMessageId(const Descriptor* desc) {
  return string();
}

bool IgnoreExtensionField(const FieldDescriptor* field) {
  // Exclude descriptor extensions from output "to avoid clutter" (from original
  // codegen).
  return field->is_extension() &&
         field->containing_type()->file()->name() ==
             "google/protobuf/descriptor.proto";
}


// Used inside Google only -- do not remove.
bool IsResponse(const Descriptor* desc) { return false; }

bool IgnoreField(const FieldDescriptor* field) {
  return IgnoreExtensionField(field);
}


// Used inside Google only -- do not remove.
bool ShouldTreatMapsAsRepeatedFields(const FileDescriptor& descriptor) {
  return false;
}

// Do we ignore this message type?
bool IgnoreMessage(const GeneratorOptions& options, const Descriptor* d) {
  return d->options().map_entry() &&
         !ShouldTreatMapsAsRepeatedFields(*d->file());
}

bool IsMap(const GeneratorOptions& options, const FieldDescriptor* field) {
  return field->is_map() && !ShouldTreatMapsAsRepeatedFields(*field->file());
}

// Does JSPB ignore this entire oneof? True only if all fields are ignored.
bool IgnoreOneof(const OneofDescriptor* oneof) {
  for (int i = 0; i < oneof->field_count(); i++) {
    if (!IgnoreField(oneof->field(i))) {
      return false;
    }
  }
  return true;
}

string JSIdent(const GeneratorOptions& options, const FieldDescriptor* field,
               bool is_upper_camel, bool is_map, bool drop_list) {
  string result;
  if (field->type() == FieldDescriptor::TYPE_GROUP) {
    result = is_upper_camel ?
        ToUpperCamel(ParseUpperCamel(field->message_type()->name())) :
        ToLowerCamel(ParseUpperCamel(field->message_type()->name()));
  } else {
    result = is_upper_camel ?
        ToUpperCamel(ParseLowerUnderscore(field->name())) :
        ToLowerCamel(ParseLowerUnderscore(field->name()));
  }
  if (is_map || IsMap(options, field)) {
    // JSPB-style or proto3-style map.
    result += "Map";
  } else if (!drop_list && field->is_repeated()) {
    // Repeated field.
    result += "List";
  }
  return result;
}

string JSObjectFieldName(const GeneratorOptions& options,
                         const FieldDescriptor* field) {
  string name = JSIdent(options, field,
                        /* is_upper_camel = */ false,
                        /* is_map = */ false,
                        /* drop_list = */ false);
  if (IsReserved(name)) {
    name = "pb_" + name;
  }
  return name;
}

string JSByteGetterSuffix(BytesMode bytes_mode) {
  switch (bytes_mode) {
    case BYTES_DEFAULT:
      return "";
    case BYTES_B64:
      return "B64";
    case BYTES_U8:
      return "U8";
    default:
      assert(false);
  }
  return "";
}

// Returns the field name as a capitalized portion of a getter/setter method
// name, e.g. MyField for .getMyField().
string JSGetterName(const GeneratorOptions& options,
                    const FieldDescriptor* field,
                    BytesMode bytes_mode = BYTES_DEFAULT,
                    bool drop_list = false) {
  string name = JSIdent(options, field,
                        /* is_upper_camel = */ true,
                        /* is_map = */ false, drop_list);
  if (field->type() == FieldDescriptor::TYPE_BYTES) {
    string suffix = JSByteGetterSuffix(bytes_mode);
    if (!suffix.empty()) {
      name += "_as" + suffix;
    }
  }
  if (name == "Extension" || name == "JsPbMessageId") {
    // Avoid conflicts with base-class names.
    name += "$";
  }
  return name;
}

string JSMapGetterName(const GeneratorOptions& options,
                       const FieldDescriptor* field) {
  return JSIdent(options, field,
                 /* is_upper_camel = */ true,
                 /* is_map = */ true,
                 /* drop_list = */ false);
}



string JSOneofName(const OneofDescriptor* oneof) {
  return ToUpperCamel(ParseLowerUnderscore(oneof->name()));
}

// Returns the index corresponding to this field in the JSPB array (underlying
// data storage array).
string JSFieldIndex(const FieldDescriptor* field) {
  // Determine whether this field is a member of a group. Group fields are a bit
  // wonky: their "containing type" is a message type created just for the
  // group, and that type's parent type has a field with the group-message type
  // as its message type and TYPE_GROUP as its field type. For such fields, the
  // index we use is relative to the field number of the group submessage field.
  // For all other fields, we just use the field number.
  const Descriptor* containing_type = field->containing_type();
  const Descriptor* parent_type = containing_type->containing_type();
  if (parent_type != NULL) {
    for (int i = 0; i < parent_type->field_count(); i++) {
      if (parent_type->field(i)->type() == FieldDescriptor::TYPE_GROUP &&
          parent_type->field(i)->message_type() == containing_type) {
        return SimpleItoa(field->number() - parent_type->field(i)->number());
      }
    }
  }
  return SimpleItoa(field->number());
}

string JSOneofIndex(const OneofDescriptor* oneof) {
  int index = -1;
  for (int i = 0; i < oneof->containing_type()->oneof_decl_count(); i++) {
    const OneofDescriptor* o = oneof->containing_type()->oneof_decl(i);
    // If at least one field in this oneof is not JSPB-ignored, count the oneof.
    for (int j = 0; j < o->field_count(); j++) {
      const FieldDescriptor* f = o->field(j);
      if (!IgnoreField(f)) {
        index++;
        break;  // inner loop
      }
    }
    if (o == oneof) {
      break;
    }
  }
  return SimpleItoa(index);
}

// Decodes a codepoint in \x0000 -- \xFFFF.
uint16 DecodeUTF8Codepoint(uint8* bytes, size_t* length) {
  if (*length == 0) {
    return 0;
  }
  size_t expected = 0;
  if ((*bytes & 0x80) == 0) {
    expected = 1;
  } else if ((*bytes & 0xe0) == 0xc0) {
    expected = 2;
  } else if ((*bytes & 0xf0) == 0xe0) {
    expected = 3;
  } else {
    // Too long -- don't accept.
    *length = 0;
    return 0;
  }

  if (*length < expected) {
    // Not enough bytes -- don't accept.
    *length = 0;
    return 0;
  }

  *length = expected;
  switch (expected) {
    case 1: return bytes[0];
    case 2: return ((bytes[0] & 0x1F) << 6)  |
                   ((bytes[1] & 0x3F) << 0);
    case 3: return ((bytes[0] & 0x0F) << 12) |
                   ((bytes[1] & 0x3F) << 6)  |
                   ((bytes[2] & 0x3F) << 0);
    default: return 0;
  }
}

// Escapes the contents of a string to be included within double-quotes ("") in
// JavaScript. The input data should be a UTF-8 encoded C++ string of chars.
// Returns false if |out| was truncated because |in| contained invalid UTF-8 or
// codepoints outside the BMP.
// TODO(lukestebbing): Support codepoints outside the BMP.
bool EscapeJSString(const string& in, string* out) {
  size_t decoded = 0;
  for (size_t i = 0; i < in.size(); i += decoded) {
    uint16 codepoint = 0;
    // Decode the next UTF-8 codepoint.
    size_t have_bytes = in.size() - i;
    uint8 bytes[3] = {
        static_cast<uint8>(in[i]),
        static_cast<uint8>(((i + 1) < in.size()) ? in[i + 1] : 0),
        static_cast<uint8>(((i + 2) < in.size()) ? in[i + 2] : 0),
    };
    codepoint = DecodeUTF8Codepoint(bytes, &have_bytes);
    if (have_bytes == 0) {
      return false;
    }
    decoded = have_bytes;

    switch (codepoint) {
      case '\'': *out += "\\x27"; break;
      case '"': *out += "\\x22"; break;
      case '<': *out += "\\x3c"; break;
      case '=': *out += "\\x3d"; break;
      case '>': *out += "\\x3e"; break;
      case '&': *out += "\\x26"; break;
      case '\b': *out += "\\b"; break;
      case '\t': *out += "\\t"; break;
      case '\n': *out += "\\n"; break;
      case '\f': *out += "\\f"; break;
      case '\r': *out += "\\r"; break;
      case '\\': *out += "\\\\"; break;
      default:
        // TODO(lukestebbing): Once we're supporting codepoints outside the BMP,
        // use a single Unicode codepoint escape if the output language is
        // ECMAScript 2015 or above. Otherwise, use a surrogate pair.
        // https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Lexical_grammar#String_literals
        if (codepoint >= 0x20 && codepoint <= 0x7e) {
          *out += static_cast<char>(codepoint);
        } else if (codepoint >= 0x100) {
          *out += StringPrintf("\\u%04x", codepoint);
        } else {
          *out += StringPrintf("\\x%02x", codepoint);
        }
        break;
    }
  }
  return true;
}

string EscapeBase64(const string& in) {
  static const char* kAlphabet =
      "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
  string result;

  for (size_t i = 0; i < in.size(); i += 3) {
    int value = (in[i] << 16) |
        (((i + 1) < in.size()) ? (in[i + 1] << 8) : 0) |
        (((i + 2) < in.size()) ? (in[i + 2] << 0) : 0);
    result += kAlphabet[(value >> 18) & 0x3f];
    result += kAlphabet[(value >> 12) & 0x3f];
    if ((i + 1) < in.size()) {
      result += kAlphabet[(value >>  6) & 0x3f];
    } else {
      result += '=';
    }
    if ((i + 2) < in.size()) {
      result += kAlphabet[(value >>  0) & 0x3f];
    } else {
      result += '=';
    }
  }

  return result;
}

// Post-process the result of SimpleFtoa/SimpleDtoa to *exactly* match the
// original codegen's formatting (which is just .toString() on java.lang.Double
// or java.lang.Float).
string PostProcessFloat(string result) {
  // If inf, -inf or nan, replace with +Infinity, -Infinity or NaN.
  if (result == "inf") {
    return "Infinity";
  } else if (result == "-inf") {
    return "-Infinity";
  } else if (result == "nan") {
    return "NaN";
  }

  // If scientific notation (e.g., "1e10"), (i) capitalize the "e", (ii)
  // ensure that the mantissa (portion prior to the "e") has at least one
  // fractional digit (after the decimal point), and (iii) strip any unnecessary
  // leading zeroes and/or '+' signs from the exponent.
  string::size_type exp_pos = result.find('e');
  if (exp_pos != string::npos) {
    string mantissa = result.substr(0, exp_pos);
    string exponent = result.substr(exp_pos + 1);

    // Add ".0" to mantissa if no fractional part exists.
    if (mantissa.find('.') == string::npos) {
      mantissa += ".0";
    }

    // Strip the sign off the exponent and store as |exp_neg|.
    bool exp_neg = false;
    if (!exponent.empty() && exponent[0] == '+') {
      exponent = exponent.substr(1);
    } else if (!exponent.empty() && exponent[0] == '-') {
      exp_neg = true;
      exponent = exponent.substr(1);
    }

    // Strip any leading zeroes off the exponent.
    while (exponent.size() > 1 && exponent[0] == '0') {
      exponent = exponent.substr(1);
    }

    return mantissa + "E" + string(exp_neg ? "-" : "") + exponent;
  }

  // Otherwise, this is an ordinary decimal number. Append ".0" if result has no
  // decimal/fractional part in order to match output of original codegen.
  if (result.find('.') == string::npos) {
    result += ".0";
  }

  return result;
}

string FloatToString(float value) {
  string result = SimpleFtoa(value);
  return PostProcessFloat(result);
}

string DoubleToString(double value) {
  string result = SimpleDtoa(value);
  return PostProcessFloat(result);
}

string MaybeNumberString(const FieldDescriptor* field, const string& orig) {
  return orig;
}

string JSFieldDefault(const FieldDescriptor* field) {
  if (field->is_repeated()) {
    return "[]";
  }

  switch (field->cpp_type()) {
    case FieldDescriptor::CPPTYPE_INT32:
      return MaybeNumberString(
          field, SimpleItoa(field->default_value_int32()));
    case FieldDescriptor::CPPTYPE_UINT32:
      // The original codegen is in Java, and Java protobufs store unsigned
      // integer values as signed integer values. In order to exactly match the
      // output, we need to reinterpret as base-2 signed. Ugh.
      return MaybeNumberString(
          field, SimpleItoa(static_cast<int32>(field->default_value_uint32())));
    case FieldDescriptor::CPPTYPE_INT64:
      return MaybeNumberString(
          field, SimpleItoa(field->default_value_int64()));
    case FieldDescriptor::CPPTYPE_UINT64:
      // See above note for uint32 -- reinterpreting as signed.
      return MaybeNumberString(
          field, SimpleItoa(static_cast<int64>(field->default_value_uint64())));
    case FieldDescriptor::CPPTYPE_ENUM:
      return SimpleItoa(field->default_value_enum()->number());
    case FieldDescriptor::CPPTYPE_BOOL:
      return field->default_value_bool() ? "true" : "false";
    case FieldDescriptor::CPPTYPE_FLOAT:
      return FloatToString(field->default_value_float());
    case FieldDescriptor::CPPTYPE_DOUBLE:
      return DoubleToString(field->default_value_double());
    case FieldDescriptor::CPPTYPE_STRING:
      if (field->type() == FieldDescriptor::TYPE_STRING) {
        string out;
        bool is_valid = EscapeJSString(field->default_value_string(), &out);
        if (!is_valid) {
          // TODO(lukestebbing): Decide whether this should be a hard error.
          GOOGLE_LOG(WARNING) << "The default value for field " << field->full_name()
                       << " was truncated since it contained invalid UTF-8 or"
                          " codepoints outside the basic multilingual plane.";
        }
        return "\"" + out + "\"";
      } else {  // Bytes
        return "\"" + EscapeBase64(field->default_value_string()) + "\"";
      }
    case FieldDescriptor::CPPTYPE_MESSAGE:
      return "null";
  }
  GOOGLE_LOG(FATAL) << "Shouldn't reach here.";
  return "";
}

string ProtoTypeName(const GeneratorOptions& options,
                     const FieldDescriptor* field) {
  switch (field->type()) {
    case FieldDescriptor::TYPE_BOOL:
      return "bool";
    case FieldDescriptor::TYPE_INT32:
      return "int32";
    case FieldDescriptor::TYPE_UINT32:
      return "uint32";
    case FieldDescriptor::TYPE_SINT32:
      return "sint32";
    case FieldDescriptor::TYPE_FIXED32:
      return "fixed32";
    case FieldDescriptor::TYPE_SFIXED32:
      return "sfixed32";
    case FieldDescriptor::TYPE_INT64:
      return "int64";
    case FieldDescriptor::TYPE_UINT64:
      return "uint64";
    case FieldDescriptor::TYPE_SINT64:
      return "sint64";
    case FieldDescriptor::TYPE_FIXED64:
      return "fixed64";
    case FieldDescriptor::TYPE_SFIXED64:
      return "sfixed64";
    case FieldDescriptor::TYPE_FLOAT:
      return "float";
    case FieldDescriptor::TYPE_DOUBLE:
      return "double";
    case FieldDescriptor::TYPE_STRING:
      return "string";
    case FieldDescriptor::TYPE_BYTES:
      return "bytes";
    case FieldDescriptor::TYPE_GROUP:
      return GetPath(options, field->message_type());
    case FieldDescriptor::TYPE_ENUM:
      return GetPath(options, field->enum_type());
    case FieldDescriptor::TYPE_MESSAGE:
      return GetPath(options, field->message_type());
    default:
      return "";
  }
}

string JSIntegerTypeName(const FieldDescriptor* field) {
  return "number";
}

string JSStringTypeName(const GeneratorOptions& options,
                        const FieldDescriptor* field,
                        BytesMode bytes_mode) {
  if (field->type() == FieldDescriptor::TYPE_BYTES) {
    switch (bytes_mode) {
      case BYTES_DEFAULT:
        return "(string|Uint8Array)";
      case BYTES_B64:
        return "string";
      case BYTES_U8:
        return "Uint8Array";
      default:
        assert(false);
    }
  }
  return "string";
}

string JSTypeName(const GeneratorOptions& options,
                  const FieldDescriptor* field,
                  BytesMode bytes_mode) {
  switch (field->cpp_type()) {
    case FieldDescriptor::CPPTYPE_BOOL:
      return "boolean";
    case FieldDescriptor::CPPTYPE_INT32:
      return JSIntegerTypeName(field);
    case FieldDescriptor::CPPTYPE_INT64:
      return JSIntegerTypeName(field);
    case FieldDescriptor::CPPTYPE_UINT32:
      return JSIntegerTypeName(field);
    case FieldDescriptor::CPPTYPE_UINT64:
      return JSIntegerTypeName(field);
    case FieldDescriptor::CPPTYPE_FLOAT:
      return "number";
    case FieldDescriptor::CPPTYPE_DOUBLE:
      return "number";
    case FieldDescriptor::CPPTYPE_STRING:
      return JSStringTypeName(options, field, bytes_mode);
    case FieldDescriptor::CPPTYPE_ENUM:
      return GetPath(options, field->enum_type());
    case FieldDescriptor::CPPTYPE_MESSAGE:
      return GetPath(options, field->message_type());
    default:
      return "";
  }
}

// Used inside Google only -- do not remove.
bool UseBrokenPresenceSemantics(const GeneratorOptions& options,
                                const FieldDescriptor* field) {
  return false;
}

// Returns true for fields that return "null" from accessors when they are
// unset.  This should normally only be true for non-repeated submessages, but
// we have legacy users who relied on old behavior where accessors behaved this
// way.
bool ReturnsNullWhenUnset(const GeneratorOptions& options,
                          const FieldDescriptor* field) {
  if (field->cpp_type() == FieldDescriptor::CPPTYPE_MESSAGE &&
      field->is_optional()) {
    return true;
  }

  // TODO(haberman): remove this case and unconditionally return false.
  return UseBrokenPresenceSemantics(options, field) && !field->is_repeated() &&
         !field->has_default_value();
}

// In a sane world, this would be the same as ReturnsNullWhenUnset().  But in
// the status quo, some fields declare that they never return null/undefined
// even though they actually do:
//   * required fields
//   * optional enum fields
//   * proto3 primitive fields.
bool DeclaredReturnTypeIsNullable(const GeneratorOptions& options,
                                  const FieldDescriptor* field) {
  if (field->is_required() || field->type() == FieldDescriptor::TYPE_ENUM) {
    return false;
  }

  if (field->file()->syntax() == FileDescriptor::SYNTAX_PROTO3 &&
      field->cpp_type() != FieldDescriptor::CPPTYPE_MESSAGE) {
    return false;
  }

  return ReturnsNullWhenUnset(options, field);
}

bool SetterAcceptsUndefined(const GeneratorOptions& options,
                            const FieldDescriptor* field) {
  if (ReturnsNullWhenUnset(options, field)) {
    return true;
  }

  // Broken presence semantics always accepts undefined for setters.
  return UseBrokenPresenceSemantics(options, field);
}

bool SetterAcceptsNull(const GeneratorOptions& options,
                       const FieldDescriptor* field) {
  if (ReturnsNullWhenUnset(options, field)) {
    return true;
  }

  // With broken presence semantics, fields with defaults accept "null" for
  // setters, but other fields do not.  This is a strange quirk of the old
  // codegen.
  return UseBrokenPresenceSemantics(options, field) &&
         field->has_default_value();
}

// Returns types which are known to by non-nullable by default.
// The style guide requires that we omit "!" in this case.
bool IsPrimitive(const string& type) {
  return type == "undefined" || type == "string" || type == "number" ||
         type == "boolean";
}

string JSFieldTypeAnnotation(const GeneratorOptions& options,
                             const FieldDescriptor* field,
                             bool is_setter_argument,
                             bool force_present,
                             bool singular_if_not_packed,
                             BytesMode bytes_mode = BYTES_DEFAULT) {
  GOOGLE_CHECK(!(is_setter_argument && force_present));
  string jstype = JSTypeName(options, field, bytes_mode);

  if (field->is_repeated() &&
      (field->is_packed() || !singular_if_not_packed)) {
    if (field->type() == FieldDescriptor::TYPE_BYTES &&
        bytes_mode == BYTES_DEFAULT) {
      jstype = "(Array<!Uint8Array>|Array<string>)";
    } else {
      if (!IsPrimitive(jstype)) {
        jstype = "!" + jstype;
      }
      jstype = "Array.<" + jstype + ">";
    }
  }

  bool is_null_or_undefined = false;

  if (is_setter_argument) {
    if (SetterAcceptsNull(options, field)) {
      jstype = "?" + jstype;
      is_null_or_undefined = true;
    }

    if (SetterAcceptsUndefined(options, field)) {
      jstype += "|undefined";
      is_null_or_undefined = true;
    }
  } else if (force_present) {
    // Don't add null or undefined.
  } else {
    if (DeclaredReturnTypeIsNullable(options, field)) {
      jstype = "?" + jstype;
      is_null_or_undefined = true;
    }
  }

  if (!is_null_or_undefined && !IsPrimitive(jstype)) {
    jstype = "!" + jstype;
  }

  return jstype;
}

string JSBinaryReaderMethodType(const FieldDescriptor* field) {
  string name = field->type_name();
  if (name[0] >= 'a' && name[0] <= 'z') {
    name[0] = (name[0] - 'a') + 'A';
  }

  return name;
}

string JSBinaryReadWriteMethodName(const FieldDescriptor* field,
                                   bool is_writer) {
  string name = JSBinaryReaderMethodType(field);
  if (field->is_packed()) {
    name = "Packed" + name;
  } else if (is_writer && field->is_repeated()) {
    name = "Repeated" + name;
  }
  return name;
}

string JSBinaryReaderMethodName(const GeneratorOptions& options,
                                const FieldDescriptor* field) {
  return "jspb.BinaryReader.prototype.read" +
         JSBinaryReadWriteMethodName(field, /* is_writer = */ false);
}

string JSBinaryWriterMethodName(const GeneratorOptions& options,
                                const FieldDescriptor* field) {
  return "jspb.BinaryWriter.prototype.write" +
         JSBinaryReadWriteMethodName(field, /* is_writer = */ true);
}

string JSReturnClause(const FieldDescriptor* desc) {
  return "";
}

string JSReturnDoc(const GeneratorOptions& options,
                   const FieldDescriptor* desc) {
  return "";
}

bool HasRepeatedFields(const GeneratorOptions& options,
                       const Descriptor* desc) {
  for (int i = 0; i < desc->field_count(); i++) {
    if (desc->field(i)->is_repeated() && !IsMap(options, desc->field(i))) {
      return true;
    }
  }
  return false;
}

static const char* kRepeatedFieldArrayName = ".repeatedFields_";

string RepeatedFieldsArrayName(const GeneratorOptions& options,
                               const Descriptor* desc) {
  return HasRepeatedFields(options, desc)
             ? (GetPath(options, desc) + kRepeatedFieldArrayName)
             : "null";
}

bool HasOneofFields(const Descriptor* desc) {
  for (int i = 0; i < desc->field_count(); i++) {
    if (desc->field(i)->containing_oneof()) {
      return true;
    }
  }
  return false;
}

static const char* kOneofGroupArrayName = ".oneofGroups_";

string OneofFieldsArrayName(const GeneratorOptions& options,
                            const Descriptor* desc) {
  return HasOneofFields(desc) ?
      (GetPath(options, desc) + kOneofGroupArrayName) : "null";
}

string RepeatedFieldNumberList(const GeneratorOptions& options,
                               const Descriptor* desc) {
  std::vector<string> numbers;
  for (int i = 0; i < desc->field_count(); i++) {
    if (desc->field(i)->is_repeated() && !IsMap(options, desc->field(i))) {
      numbers.push_back(JSFieldIndex(desc->field(i)));
    }
  }
  return "[" + Join(numbers, ",") + "]";
}

string OneofGroupList(const Descriptor* desc) {
  // List of arrays (one per oneof), each of which is a list of field indices
  std::vector<string> oneof_entries;
  for (int i = 0; i < desc->oneof_decl_count(); i++) {
    const OneofDescriptor* oneof = desc->oneof_decl(i);
    if (IgnoreOneof(oneof)) {
      continue;
    }

    std::vector<string> oneof_fields;
    for (int j = 0; j < oneof->field_count(); j++) {
      if (IgnoreField(oneof->field(j))) {
        continue;
      }
      oneof_fields.push_back(JSFieldIndex(oneof->field(j)));
    }
    oneof_entries.push_back("[" + Join(oneof_fields, ",") + "]");
  }
  return "[" + Join(oneof_entries, ",") + "]";
}

string JSOneofArray(const GeneratorOptions& options,
                    const FieldDescriptor* field) {
  return OneofFieldsArrayName(options, field->containing_type()) + "[" +
      JSOneofIndex(field->containing_oneof()) + "]";
}

string RelativeTypeName(const FieldDescriptor* field) {
  assert(field->cpp_type() == FieldDescriptor::CPPTYPE_ENUM ||
         field->cpp_type() == FieldDescriptor::CPPTYPE_MESSAGE);
  // For a field with an enum or message type, compute a name relative to the
  // path name of the message type containing this field.
  string package = field->file()->package();
  string containing_type = field->containing_type()->full_name() + ".";
  string type = (field->cpp_type() == FieldDescriptor::CPPTYPE_ENUM) ?
      field->enum_type()->full_name() : field->message_type()->full_name();

  // |prefix| is advanced as we find separators '.' past the common package
  // prefix that yield common prefixes in the containing type's name and this
  // type's name.
  int prefix = 0;
  for (int i = 0; i < type.size() && i < containing_type.size(); i++) {
    if (type[i] != containing_type[i]) {
      break;
    }
    if (type[i] == '.' && i >= package.size()) {
      prefix = i + 1;
    }
  }

  return type.substr(prefix);
}

string JSExtensionsObjectName(const GeneratorOptions& options,
                              const FileDescriptor* from_file,
                              const Descriptor* desc) {
  if (desc->full_name() == "google.protobuf.bridge.MessageSet") {
    // TODO(haberman): fix this for the kImportCommonJs case.
    return "jspb.Message.messageSetExtensions";
  } else {
    return MaybeCrossFileRef(options, from_file, desc) + ".extensions";
  }
}

static const int kMapKeyField = 1;
static const int kMapValueField = 2;

const FieldDescriptor* MapFieldKey(const FieldDescriptor* field) {
  assert(field->is_map());
  return field->message_type()->FindFieldByNumber(kMapKeyField);
}

const FieldDescriptor* MapFieldValue(const FieldDescriptor* field) {
  assert(field->is_map());
  return field->message_type()->FindFieldByNumber(kMapValueField);
}

string FieldDefinition(const GeneratorOptions& options,
                       const FieldDescriptor* field) {
  if (IsMap(options, field)) {
    const FieldDescriptor* key_field = MapFieldKey(field);
    const FieldDescriptor* value_field = MapFieldValue(field);
    string key_type = ProtoTypeName(options, key_field);
    string value_type;
    if (value_field->type() == FieldDescriptor::TYPE_ENUM ||
        value_field->type() == FieldDescriptor::TYPE_MESSAGE) {
      value_type = RelativeTypeName(value_field);
    } else {
      value_type = ProtoTypeName(options, value_field);
    }
    return StringPrintf("map<%s, %s> %s = %d;",
                        key_type.c_str(),
                        value_type.c_str(),
                        field->name().c_str(),
                        field->number());
  } else {
    string qualifier = field->is_repeated() ? "repeated" :
        (field->is_optional() ? "optional" : "required");
    string type, name;
    if (field->type() == FieldDescriptor::TYPE_ENUM ||
        field->type() == FieldDescriptor::TYPE_MESSAGE) {
      type = RelativeTypeName(field);
      name = field->name();
    } else if (field->type() == FieldDescriptor::TYPE_GROUP) {
      type = "group";
      name = field->message_type()->name();
    } else {
      type = ProtoTypeName(options, field);
      name = field->name();
    }
    return StringPrintf("%s %s %s = %d;",
                        qualifier.c_str(),
                        type.c_str(),
                        name.c_str(),
                        field->number());
  }
}

string FieldComments(const FieldDescriptor* field, BytesMode bytes_mode) {
  string comments;
  if (field->cpp_type() == FieldDescriptor::CPPTYPE_BOOL) {
    comments +=
        " * Note that Boolean fields may be set to 0/1 when serialized from "
        "a Java server.\n"
        " * You should avoid comparisons like {@code val === true/false} in "
        "those cases.\n";
  }
  if (field->is_repeated()) {
    comments +=
        " * If you change this array by adding, removing or replacing "
        "elements, or if you\n"
        " * replace the array itself, then you must call the setter to "
        "update it.\n";
  }
  if (field->type() == FieldDescriptor::TYPE_BYTES && bytes_mode == BYTES_U8) {
    comments +=
        " * Note that Uint8Array is not supported on all browsers.\n"
        " * @see http://caniuse.com/Uint8Array\n";
  }
  return comments;
}

bool ShouldGenerateExtension(const FieldDescriptor* field) {
  return
      field->is_extension() &&
      !IgnoreField(field);
}

bool HasExtensions(const Descriptor* desc) {
  for (int i = 0; i < desc->extension_count(); i++) {
    if (ShouldGenerateExtension(desc->extension(i))) {
      return true;
    }
  }
  for (int i = 0; i < desc->nested_type_count(); i++) {
    if (HasExtensions(desc->nested_type(i))) {
      return true;
    }
  }
  return false;
}

bool HasExtensions(const FileDescriptor* file) {
  for (int i = 0; i < file->extension_count(); i++) {
    if (ShouldGenerateExtension(file->extension(i))) {
      return true;
    }
  }
  for (int i = 0; i < file->message_type_count(); i++) {
    if (HasExtensions(file->message_type(i))) {
      return true;
    }
  }
  return false;
}

bool HasMap(const GeneratorOptions& options, const Descriptor* desc) {
  for (int i = 0; i < desc->field_count(); i++) {
    if (IsMap(options, desc->field(i))) {
      return true;
    }
  }
  for (int i = 0; i < desc->nested_type_count(); i++) {
    if (HasMap(options, desc->nested_type(i))) {
      return true;
    }
  }
  return false;
}

bool FileHasMap(const GeneratorOptions& options, const FileDescriptor* desc) {
  for (int i = 0; i < desc->message_type_count(); i++) {
    if (HasMap(options, desc->message_type(i))) {
      return true;
    }
  }
  return false;
}

bool IsExtendable(const Descriptor* desc) {
  return desc->extension_range_count() > 0;
}

// Returns the max index in the underlying data storage array beyond which the
// extension object is used.
string GetPivot(const Descriptor* desc) {
  static const int kDefaultPivot = (1 << 29);  // max field number (29 bits)

  // Find the max field number
  int max_field_number = 0;
  for (int i = 0; i < desc->field_count(); i++) {
    if (!IgnoreField(desc->field(i)) &&
        desc->field(i)->number() > max_field_number) {
      max_field_number = desc->field(i)->number();
    }
  }

  int pivot = -1;
  if (IsExtendable(desc)) {
    pivot = ((max_field_number + 1) < kDefaultPivot) ?
        (max_field_number + 1) : kDefaultPivot;
  }

  return SimpleItoa(pivot);
}

// Whether this field represents presence.  For fields with presence, we
// generate extra methods (clearFoo() and hasFoo()) for this field.
bool HasFieldPresence(const GeneratorOptions& options,
                      const FieldDescriptor* field) {
  if (field->is_repeated() || field->is_map()) {
    // We say repeated fields and maps don't have presence, but we still do
    // generate clearFoo() methods for them through a special case elsewhere.
    return false;
  }

  if (UseBrokenPresenceSemantics(options, field)) {
    // Proto3 files with broken presence semantics have field presence.
    return true;
  }

  return field->cpp_type() == FieldDescriptor::CPPTYPE_MESSAGE ||
         field->containing_oneof() != NULL ||
         field->file()->syntax() == FileDescriptor::SYNTAX_PROTO2;
}

// We use this to implement the semantics that same file can be generated
// multiple times, but the last one wins.  We never actually write the files,
// but we keep a set of which descriptors were the final one for a given
// filename.
class FileDeduplicator {
 public:
  explicit FileDeduplicator(const GeneratorOptions& options)
      : error_on_conflict_(options.error_on_name_conflict) {}

  bool AddFile(const string& filename, const void* desc, string* error) {
    if (descs_by_filename_.find(filename) != descs_by_filename_.end()) {
      if (error_on_conflict_) {
        *error = "Name conflict: file name " + filename +
                 " would be generated by two descriptors";
        return false;
      }
      allowed_descs_.erase(descs_by_filename_[filename]);
    }

    descs_by_filename_[filename] = desc;
    allowed_descs_.insert(desc);
    return true;
  }

  void GetAllowedSet(std::set<const void*>* allowed_set) {
    *allowed_set = allowed_descs_;
  }

 private:
  bool error_on_conflict_;
  std::map<string, const void*> descs_by_filename_;
  std::set<const void*> allowed_descs_;
};

void DepthFirstSearch(const FileDescriptor* file,
                      std::vector<const FileDescriptor*>* list,
                      std::set<const FileDescriptor*>* seen) {
  if (!seen->insert(file).second) {
    return;
  }

  // Add all dependencies.
  for (int i = 0; i < file->dependency_count(); i++) {
    DepthFirstSearch(file->dependency(i), list, seen);
  }

  // Add this file.
  list->push_back(file);
}

// A functor for the predicate to remove_if() below.  Returns true if a given
// FileDescriptor is not in the given set.
class NotInSet {
 public:
  explicit NotInSet(const std::set<const FileDescriptor*>& file_set)
      : file_set_(file_set) {}

  bool operator()(const FileDescriptor* file) {
    return file_set_.count(file) == 0;
  }

 private:
  const std::set<const FileDescriptor*>& file_set_;
};

// This function generates an ordering of the input FileDescriptors that matches
// the logic of the old code generator.  The order is significant because two
// different input files can generate the same output file, and the last one
// needs to win.
void GenerateJspbFileOrder(const std::vector<const FileDescriptor*>& input,
                           std::vector<const FileDescriptor*>* ordered) {
  // First generate an ordering of all reachable files (including dependencies)
  // with depth-first search.  This mimics the behavior of --include_imports,
  // which is what the old codegen used.
  ordered->clear();
  std::set<const FileDescriptor*> seen;
  std::set<const FileDescriptor*> input_set;
  for (int i = 0; i < input.size(); i++) {
    DepthFirstSearch(input[i], ordered, &seen);
    input_set.insert(input[i]);
  }

  // Now remove the entries that are not actually in our input list.
  ordered->erase(
      std::remove_if(ordered->begin(), ordered->end(), NotInSet(input_set)),
      ordered->end());
}

// If we're generating code in file-per-type mode, avoid overwriting files
// by choosing the last descriptor that writes each filename and permitting
// only those to generate code.

bool GenerateJspbAllowedSet(const GeneratorOptions& options,
                            const std::vector<const FileDescriptor*>& files,
                            std::set<const void*>* allowed_set,
                            string* error) {
  std::vector<const FileDescriptor*> files_ordered;
  GenerateJspbFileOrder(files, &files_ordered);

  // Choose the last descriptor for each filename.
  FileDeduplicator dedup(options);
  for (int i = 0; i < files_ordered.size(); i++) {
    for (int j = 0; j < files_ordered[i]->message_type_count(); j++) {
      const Descriptor* desc = files_ordered[i]->message_type(j);
      if (!dedup.AddFile(GetMessageFileName(options, desc), desc, error)) {
        return false;
      }
    }
    for (int j = 0; j < files_ordered[i]->enum_type_count(); j++) {
      const EnumDescriptor* desc = files_ordered[i]->enum_type(j);
      if (!dedup.AddFile(GetEnumFileName(options, desc), desc, error)) {
        return false;
      }
    }

    // Pull out all free-floating extensions and generate files for those too.
    bool has_extension = false;

    for (int j = 0; j < files_ordered[i]->extension_count(); j++) {
      if (ShouldGenerateExtension(files_ordered[i]->extension(j))) {
        has_extension = true;
      }
    }

    if (has_extension) {
      if (!dedup.AddFile(GetExtensionFileName(options, files_ordered[i]),
                         files_ordered[i], error)) {
        return false;
      }
    }
  }

  dedup.GetAllowedSet(allowed_set);

  return true;
}

}  // anonymous namespace

void Generator::GenerateHeader(const GeneratorOptions& options,
                               io::Printer* printer) const {
  printer->Print("/**\n"
                 " * @fileoverview\n"
                 " * @enhanceable\n"
                 " * @public\n"
                 " */\n"
                 "// GENERATED CODE -- DO NOT EDIT!\n"
                 "\n");
}

void Generator::FindProvidesForFile(const GeneratorOptions& options,
                                    io::Printer* printer,
                                    const FileDescriptor* file,
                                    std::set<string>* provided) const {
  for (int i = 0; i < file->message_type_count(); i++) {
    FindProvidesForMessage(options, printer, file->message_type(i), provided);
  }
  for (int i = 0; i < file->enum_type_count(); i++) {
    FindProvidesForEnum(options, printer, file->enum_type(i), provided);
  }
}

void Generator::FindProvides(const GeneratorOptions& options,
                             io::Printer* printer,
                             const std::vector<const FileDescriptor*>& files,
                             std::set<string>* provided) const {
  for (int i = 0; i < files.size(); i++) {
    FindProvidesForFile(options, printer, files[i], provided);
  }

  printer->Print("\n");
}

void Generator::FindProvidesForMessage(
    const GeneratorOptions& options,
    io::Printer* printer,
    const Descriptor* desc,
    std::set<string>* provided) const {
  if (IgnoreMessage(options, desc)) {
    return;
  }

  string name = GetPath(options, desc);
  provided->insert(name);

  for (int i = 0; i < desc->enum_type_count(); i++) {
    FindProvidesForEnum(options, printer, desc->enum_type(i),
                        provided);
  }
  for (int i = 0; i < desc->nested_type_count(); i++) {
    FindProvidesForMessage(options, printer, desc->nested_type(i),
                           provided);
  }
}

void Generator::FindProvidesForEnum(const GeneratorOptions& options,
                                    io::Printer* printer,
                                    const EnumDescriptor* enumdesc,
                                    std::set<string>* provided) const {
  string name = GetPath(options, enumdesc);
  provided->insert(name);
}

void Generator::FindProvidesForFields(
    const GeneratorOptions& options,
    io::Printer* printer,
    const std::vector<const FieldDescriptor*>& fields,
    std::set<string>* provided) const {
  for (int i = 0; i < fields.size(); i++) {
    const FieldDescriptor* field = fields[i];

    if (IgnoreField(field)) {
      continue;
    }

    string name =
        GetPath(options, field->file()) + "." +
        JSObjectFieldName(options, field);
    provided->insert(name);
  }
}

void Generator::GenerateProvides(const GeneratorOptions& options,
                                 io::Printer* printer,
                                 std::set<string>* provided) const {
  for (std::set<string>::iterator it = provided->begin();
       it != provided->end(); ++it) {
    if (options.import_style == GeneratorOptions::kImportClosure) {
      printer->Print("goog.provide('$name$');\n", "name", *it);
    } else {
      // We aren't using Closure's import system, but we use goog.exportSymbol()
      // to construct the expected tree of objects, eg.
      //
      //   goog.exportSymbol('foo.bar.Baz', null, this);
      //
      //   // Later generated code expects foo.bar = {} to exist:
      //   foo.bar.Baz = function() { /* ... */ }
      printer->Print("goog.exportSymbol('$name$', null, global);\n", "name",
                     *it);
    }
  }
}

void Generator::GenerateRequiresForMessage(const GeneratorOptions& options,
                                           io::Printer* printer,
                                           const Descriptor* desc,
                                           std::set<string>* provided) const {
  std::set<string> required;
  std::set<string> forwards;
  bool have_message = false;
  FindRequiresForMessage(options, desc,
                         &required, &forwards, &have_message);

  GenerateRequiresImpl(options, printer, &required, &forwards, provided,
                       /* require_jspb = */ have_message,
                       /* require_extension = */ HasExtensions(desc),
                       /* require_map = */ HasMap(options, desc));
}

void Generator::GenerateRequiresForLibrary(
    const GeneratorOptions& options, io::Printer* printer,
    const std::vector<const FileDescriptor*>& files,
    std::set<string>* provided) const {
  GOOGLE_CHECK_EQ(options.import_style, GeneratorOptions::kImportClosure);
  // For Closure imports we need to import every message type individually.
  std::set<string> required;
  std::set<string> forwards;
  bool have_extensions = false;
  bool have_map = false;
  bool have_message = false;

  for (int i = 0; i < files.size(); i++) {
    for (int j = 0; j < files[i]->message_type_count(); j++) {
      const Descriptor* desc = files[i]->message_type(j);
      if (!IgnoreMessage(options, desc)) {
        FindRequiresForMessage(options, desc, &required, &forwards,
                               &have_message);
      }
    }

    if (!have_extensions && HasExtensions(files[i])) {
      have_extensions = true;
    }

    if (!have_map && FileHasMap(options, files[i])) {
      have_map = true;
    }

    for (int j = 0; j < files[i]->extension_count(); j++) {
      const FieldDescriptor* extension = files[i]->extension(j);
      if (IgnoreField(extension)) {
        continue;
      }
      if (extension->containing_type()->full_name() !=
        "google.protobuf.bridge.MessageSet") {
        required.insert(GetPath(options, extension->containing_type()));
      }
      FindRequiresForField(options, extension, &required, &forwards);
      have_extensions = true;
    }
  }

  GenerateRequiresImpl(options, printer, &required, &forwards, provided,
                       /* require_jspb = */ have_message,
                       /* require_extension = */ have_extensions,
                       /* require_map = */ have_map);
}

void Generator::GenerateRequiresForExtensions(
    const GeneratorOptions& options, io::Printer* printer,
    const std::vector<const FieldDescriptor*>& fields,
    std::set<string>* provided) const {
  std::set<string> required;
  std::set<string> forwards;
  for (int i = 0; i < fields.size(); i++) {
    const FieldDescriptor* field = fields[i];
    if (IgnoreField(field)) {
      continue;
    }
    FindRequiresForExtension(options, field, &required, &forwards);
  }

  GenerateRequiresImpl(options, printer, &required, &forwards, provided,
                       /* require_jspb = */ false,
                       /* require_extension = */ fields.size() > 0,
                       /* require_map = */ false);
}

void Generator::GenerateRequiresImpl(const GeneratorOptions& options,
                                     io::Printer* printer,
                                     std::set<string>* required,
                                     std::set<string>* forwards,
                                     std::set<string>* provided,
                                     bool require_jspb, bool require_extension,
                                     bool require_map) const {
  if (require_jspb) {
    printer->Print(
        "goog.require('jspb.Message');\n"
        "goog.require('jspb.BinaryReader');\n"
        "goog.require('jspb.BinaryWriter');\n");
  }
  if (require_extension) {
    printer->Print("goog.require('jspb.ExtensionFieldBinaryInfo');\n");
    printer->Print(
        "goog.require('jspb.ExtensionFieldInfo');\n");
  }
  if (require_map) {
    printer->Print("goog.require('jspb.Map');\n");
  }

  std::set<string>::iterator it;
  for (it = required->begin(); it != required->end(); ++it) {
    if (provided->find(*it) != provided->end()) {
      continue;
    }
    printer->Print("goog.require('$name$');\n",
                   "name", *it);
  }

  printer->Print("\n");

  for (it = forwards->begin(); it != forwards->end(); ++it) {
    if (provided->find(*it) != provided->end()) {
      continue;
    }
    printer->Print("goog.forwardDeclare('$name$');\n",
                   "name", *it);
  }
}

bool NamespaceOnly(const Descriptor* desc) {
  return false;
}

void Generator::FindRequiresForMessage(
    const GeneratorOptions& options,
    const Descriptor* desc,
    std::set<string>* required,
    std::set<string>* forwards,
    bool* have_message) const {


  if (!NamespaceOnly(desc)) {
    *have_message = true;
    for (int i = 0; i < desc->field_count(); i++) {
      const FieldDescriptor* field = desc->field(i);
      if (IgnoreField(field)) {
        continue;
      }
      FindRequiresForField(options, field, required, forwards);
    }
  }

  for (int i = 0; i < desc->extension_count(); i++) {
    const FieldDescriptor* field = desc->extension(i);
    if (IgnoreField(field)) {
      continue;
    }
    FindRequiresForExtension(options, field, required, forwards);
  }

  for (int i = 0; i < desc->nested_type_count(); i++) {
    FindRequiresForMessage(options, desc->nested_type(i), required, forwards,
                           have_message);
  }
}

void Generator::FindRequiresForField(const GeneratorOptions& options,
                                     const FieldDescriptor* field,
                                     std::set<string>* required,
                                     std::set<string>* forwards) const {
    if (field->cpp_type() == FieldDescriptor::CPPTYPE_ENUM &&
        // N.B.: file-level extensions with enum type do *not* create
        // dependencies, as per original codegen.
        !(field->is_extension() && field->extension_scope() == NULL)) {
      if (options.add_require_for_enums) {
        required->insert(GetPath(options, field->enum_type()));
      } else {
        forwards->insert(GetPath(options, field->enum_type()));
      }
    } else if (field->cpp_type() == FieldDescriptor::CPPTYPE_MESSAGE) {
      if (!IgnoreMessage(options, field->message_type())) {
        required->insert(GetPath(options, field->message_type()));
      }
    }
}

void Generator::FindRequiresForExtension(const GeneratorOptions& options,
                                         const FieldDescriptor* field,
                                         std::set<string>* required,
                                         std::set<string>* forwards) const {
    if (field->containing_type()->full_name() != "google.protobuf.bridge.MessageSet") {
      required->insert(GetPath(options, field->containing_type()));
    }
    FindRequiresForField(options, field, required, forwards);
}

void Generator::GenerateTestOnly(const GeneratorOptions& options,
                                 io::Printer* printer) const {
  if (options.testonly) {
    printer->Print("goog.setTestOnly();\n\n");
  }
  printer->Print("\n");
}

void Generator::GenerateClassesAndEnums(const GeneratorOptions& options,
                                        io::Printer* printer,
                                        const FileDescriptor* file) const {
  for (int i = 0; i < file->message_type_count(); i++) {
    GenerateClass(options, printer, file->message_type(i));
  }
  for (int i = 0; i < file->enum_type_count(); i++) {
    GenerateEnum(options, printer, file->enum_type(i));
  }
}

void Generator::GenerateClass(const GeneratorOptions& options,
                              io::Printer* printer,
                              const Descriptor* desc) const {
  if (IgnoreMessage(options, desc)) {
    return;
  }

  if (!NamespaceOnly(desc)) {
    printer->Print("\n");
    GenerateClassConstructor(options, printer, desc);
    GenerateClassFieldInfo(options, printer, desc);


    GenerateClassToObject(options, printer, desc);
    // These must come *before* the extension-field info generation in
    // GenerateClassRegistration so that references to the binary
    // serialization/deserialization functions may be placed in the extension
    // objects.
    GenerateClassDeserializeBinary(options, printer, desc);
    GenerateClassSerializeBinary(options, printer, desc);
  }

  // Recurse on nested types. These must come *before* the extension-field
  // info generation in GenerateClassRegistration so that extensions that 
  // reference nested types proceed the definitions of the nested types.
  for (int i = 0; i < desc->enum_type_count(); i++) {
    GenerateEnum(options, printer, desc->enum_type(i));
  }
  for (int i = 0; i < desc->nested_type_count(); i++) {
    GenerateClass(options, printer, desc->nested_type(i));
  }

  if (!NamespaceOnly(desc)) {
    GenerateClassRegistration(options, printer, desc);
    GenerateClassFields(options, printer, desc);
    if (IsExtendable(desc) && desc->full_name() != "google.protobuf.bridge.MessageSet") {
      GenerateClassExtensionFieldInfo(options, printer, desc);
    }

    if (options.import_style != GeneratorOptions::kImportClosure) {
      for (int i = 0; i < desc->extension_count(); i++) {
        GenerateExtension(options, printer, desc->extension(i));
      }
    }
  }

}

void Generator::GenerateClassConstructor(const GeneratorOptions& options,
                                         io::Printer* printer,
                                         const Descriptor* desc) const {
  printer->Print(
      "/**\n"
      " * Generated by JsPbCodeGenerator.\n"
      " * @param {Array=} opt_data Optional initial data array, typically "
      "from a\n"
      " * server response, or constructed directly in Javascript. The array "
      "is used\n"
      " * in place and becomes part of the constructed object. It is not "
      "cloned.\n"
      " * If no data is provided, the constructed object will be empty, but "
      "still\n"
      " * valid.\n"
      " * @extends {jspb.Message}\n"
      " * @constructor\n"
      " */\n"
      "$classname$ = function(opt_data) {\n",
      "classname", GetPath(options, desc));
  string message_id = GetMessageId(desc);
  printer->Print(
      "  jspb.Message.initialize(this, opt_data, $messageId$, $pivot$, "
      "$rptfields$, $oneoffields$);\n",
      "messageId", !message_id.empty() ?
                   ("'" + message_id + "'") :
                   (IsResponse(desc) ? "''" : "0"),
      "pivot", GetPivot(desc),
      "rptfields", RepeatedFieldsArrayName(options, desc),
      "oneoffields", OneofFieldsArrayName(options, desc));
  printer->Print(
      "};\n"
      "goog.inherits($classname$, jspb.Message);\n"
      "if (goog.DEBUG && !COMPILED) {\n"
      "  $classname$.displayName = '$classname$';\n"
      "}\n",
      "classname", GetPath(options, desc));
}

void Generator::GenerateClassFieldInfo(const GeneratorOptions& options,
                                       io::Printer* printer,
                                       const Descriptor* desc) const {
  if (HasRepeatedFields(options, desc)) {
    printer->Print(
        "/**\n"
        " * List of repeated fields within this message type.\n"
        " * @private {!Array<number>}\n"
        " * @const\n"
        " */\n"
        "$classname$$rptfieldarray$ = $rptfields$;\n"
        "\n",
        "classname", GetPath(options, desc),
        "rptfieldarray", kRepeatedFieldArrayName,
        "rptfields", RepeatedFieldNumberList(options, desc));
  }

  if (HasOneofFields(desc)) {
    printer->Print(
        "/**\n"
        " * Oneof group definitions for this message. Each group defines the "
        "field\n"
        " * numbers belonging to that group. When of these fields' value is "
        "set, all\n"
        " * other fields in the group are cleared. During deserialization, if "
        "multiple\n"
        " * fields are encountered for a group, only the last value seen will "
        "be kept.\n"
        " * @private {!Array<!Array<number>>}\n"
        " * @const\n"
        " */\n"
        "$classname$$oneofgrouparray$ = $oneofgroups$;\n"
        "\n",
        "classname", GetPath(options, desc),
        "oneofgrouparray", kOneofGroupArrayName,
        "oneofgroups", OneofGroupList(desc));

    for (int i = 0; i < desc->oneof_decl_count(); i++) {
      if (IgnoreOneof(desc->oneof_decl(i))) {
        continue;
      }
      GenerateOneofCaseDefinition(options, printer, desc->oneof_decl(i));
    }
  }
}

void Generator::GenerateClassXid(const GeneratorOptions& options,
                                 io::Printer* printer,
                                 const Descriptor* desc) const {
  printer->Print(
      "\n"
      "\n"
      "$class$.prototype.messageXid = xid('$class$');\n",
      "class", GetPath(options, desc));
}

void Generator::GenerateOneofCaseDefinition(
    const GeneratorOptions& options,
    io::Printer* printer,
    const OneofDescriptor* oneof) const {
  printer->Print(
      "/**\n"
      " * @enum {number}\n"
      " */\n"
      "$classname$.$oneof$Case = {\n"
      "  $upcase$_NOT_SET: 0",
      "classname", GetPath(options, oneof->containing_type()),
      "oneof", JSOneofName(oneof),
      "upcase", ToEnumCase(oneof->name()));

  for (int i = 0; i < oneof->field_count(); i++) {
    if (IgnoreField(oneof->field(i))) {
      continue;
    }

    printer->Print(
        ",\n"
        "  $upcase$: $number$",
        "upcase", ToEnumCase(oneof->field(i)->name()),
        "number", JSFieldIndex(oneof->field(i)));
  }

  printer->Print(
      "\n"
      "};\n"
      "\n"
      "/**\n"
      " * @return {$class$.$oneof$Case}\n"
      " */\n"
      "$class$.prototype.get$oneof$Case = function() {\n"
      "  return /** @type {$class$.$oneof$Case} */(jspb.Message."
      "computeOneofCase(this, $class$.oneofGroups_[$oneofindex$]));\n"
      "};\n"
      "\n",
      "class", GetPath(options, oneof->containing_type()),
      "oneof", JSOneofName(oneof),
      "oneofindex", JSOneofIndex(oneof));
}

void Generator::GenerateClassToObject(const GeneratorOptions& options,
                                      io::Printer* printer,
                                      const Descriptor* desc) const {
  printer->Print(
      "\n"
      "\n"
      "if (jspb.Message.GENERATE_TO_OBJECT) {\n"
      "/**\n"
      " * Creates an object representation of this proto suitable for use in "
      "Soy templates.\n"
      " * Field names that are reserved in JavaScript and will be renamed to "
      "pb_name.\n"
      " * To access a reserved field use, foo.pb_<name>, eg, foo.pb_default.\n"
      " * For the list of reserved names please see:\n"
      " *     com.google.apps.jspb.JsClassTemplate.JS_RESERVED_WORDS.\n"
      " * @param {boolean=} opt_includeInstance Whether to include the JSPB "
      "instance\n"
      " *     for transitional soy proto support: http://goto/soy-param-"
      "migration\n"
      " * @return {!Object}\n"
      " */\n"
      "$classname$.prototype.toObject = function(opt_includeInstance) {\n"
      "  return $classname$.toObject(opt_includeInstance, this);\n"
      "};\n"
      "\n"
      "\n"
      "/**\n"
      " * Static version of the {@see toObject} method.\n"
      " * @param {boolean|undefined} includeInstance Whether to include the "
      "JSPB\n"
      " *     instance for transitional soy proto support:\n"
      " *     http://goto/soy-param-migration\n"
      " * @param {!$classname$} msg The msg instance to transform.\n"
      " * @return {!Object}\n"
      " */\n"
      "$classname$.toObject = function(includeInstance, msg) {\n"
      "  var f, obj = {",
      "classname", GetPath(options, desc));

  bool first = true;
  for (int i = 0; i < desc->field_count(); i++) {
    const FieldDescriptor* field = desc->field(i);
    if (IgnoreField(field)) {
      continue;
    }

    if (!first) {
      printer->Print(",\n    ");
    } else {
      printer->Print("\n    ");
      first = false;
    }

    GenerateClassFieldToObject(options, printer, field);
  }

  if (!first) {
    printer->Print("\n  };\n\n");
  } else {
    printer->Print("\n\n  };\n\n");
  }

  if (IsExtendable(desc)) {
    printer->Print(
        "  jspb.Message.toObjectExtension(/** @type {!jspb.Message} */ (msg), "
        "obj,\n"
        "      $extObject$, $class$.prototype.getExtension,\n"
        "      includeInstance);\n",
        "extObject", JSExtensionsObjectName(options, desc->file(), desc),
        "class", GetPath(options, desc));
  }

  printer->Print(
      "  if (includeInstance) {\n"
      "    obj.$$jspbMessageInstance = msg;\n"
      "  }\n"
      "  return obj;\n"
      "};\n"
      "}\n"
      "\n"
      "\n",
      "classname", GetPath(options, desc));
}

void Generator::GenerateFieldValueExpression(io::Printer* printer,
                                             const char *obj_reference,
                                             const FieldDescriptor* field,
                                             bool use_default) const {
  bool is_float_or_double =
      field->cpp_type() == FieldDescriptor::CPPTYPE_FLOAT ||
      field->cpp_type() == FieldDescriptor::CPPTYPE_DOUBLE;
  if (use_default) {
    if (is_float_or_double) {
      // Coerce "Nan" and "Infinity" to actual float values.
      //
      // This will change null to 0, but that doesn't matter since we're getting
      // with a default.
      printer->Print("+");
    }

    printer->Print(
        "jspb.Message.getFieldWithDefault($obj$, $index$, $default$)",
        "obj", obj_reference,
        "index", JSFieldIndex(field),
        "default", JSFieldDefault(field));
  } else {
    if (is_float_or_double) {
      if (field->is_required()) {
        // Use "+" to convert all fields to numeric (including null).
        printer->Print(
            "+jspb.Message.getField($obj$, $index$)",
            "index", JSFieldIndex(field),
            "obj", obj_reference);
      } else {
        // Converts "NaN" and "Infinity" while preserving null.
        printer->Print(
            "jspb.Message.get$cardinality$FloatingPointField($obj$, $index$)",
            "cardinality", field->is_repeated() ? "Repeated" : "Optional",
            "index", JSFieldIndex(field),
            "obj", obj_reference);
      }
    } else {
      printer->Print("jspb.Message.getField($obj$, $index$)",
                     "index", JSFieldIndex(field),
                     "obj", obj_reference);
    }
  }
}

void Generator::GenerateClassFieldToObject(const GeneratorOptions& options,
                                           io::Printer* printer,
                                           const FieldDescriptor* field) const {
  printer->Print("$fieldname$: ",
                 "fieldname", JSObjectFieldName(options, field));

  if (IsMap(options, field)) {
    const FieldDescriptor* value_field = MapFieldValue(field);
    // If the map values are of a message type, we must provide their static
    // toObject() method; otherwise we pass undefined for that argument.
    string value_to_object;
    if (value_field->cpp_type() == FieldDescriptor::CPPTYPE_MESSAGE) {
      value_to_object =
          GetPath(options, value_field->message_type()) + ".toObject";
    } else {
      value_to_object = "undefined";
    }
    printer->Print(
        "(f = msg.get$name$()) ? f.toObject(includeInstance, $valuetoobject$) "
        ": []",
        "name", JSGetterName(options, field), "valuetoobject", value_to_object);
  } else if (field->cpp_type() == FieldDescriptor::CPPTYPE_MESSAGE) {
    // Message field.
    if (field->is_repeated()) {
      {
        printer->Print("jspb.Message.toObjectList(msg.get$getter$(),\n"
                       "    $type$.toObject, includeInstance)",
                       "getter", JSGetterName(options, field),
                       "type", SubmessageTypeRef(options, field));
      }
    } else {
      printer->Print("(f = msg.get$getter$()) && "
                     "$type$.toObject(includeInstance, f)",
                     "getter", JSGetterName(options, field),
                     "type", SubmessageTypeRef(options, field));
    }
  } else if (field->type() == FieldDescriptor::TYPE_BYTES) {
    // For bytes fields we want to always return the B64 data.
    printer->Print("msg.get$getter$()",
                   "getter", JSGetterName(options, field, BYTES_B64));
  } else {
    bool use_default = field->has_default_value();

    if (field->file()->syntax() == FileDescriptor::SYNTAX_PROTO3 &&
        // Repeated fields get initialized to their default in the constructor
        // (why?), so we emit a plain getField() call for them.
        !field->is_repeated() && !UseBrokenPresenceSemantics(options, field)) {
      // Proto3 puts all defaults (including implicit defaults) in toObject().
      // But for proto2 we leave the existing semantics unchanged: unset fields
      // without default are unset.
      use_default = true;
    }

    // We don't implement this by calling the accessors, because the semantics
    // of the accessors are changing independently of the toObject() semantics.
    // We are migrating the accessors to return defaults instead of null, but
    // it may take longer to migrate toObject (or we might not want to do it at
    // all).  So we want to generate independent code.
    GenerateFieldValueExpression(printer, "msg", field, use_default);
  }
}

void Generator::GenerateClassFromObject(const GeneratorOptions& options,
                                        io::Printer* printer,
                                        const Descriptor* desc) const {
  printer->Print(
      "if (jspb.Message.GENERATE_FROM_OBJECT) {\n"
      "/**\n"
      " * Loads data from an object into a new instance of this proto.\n"
      " * @param {!Object} obj The object representation of this proto to\n"
      " *     load the data from.\n"
      " * @return {!$classname$}\n"
      " */\n"
      "$classname$.fromObject = function(obj) {\n"
      "  var f, msg = new $classname$();\n",
      "classname", GetPath(options, desc));

  for (int i = 0; i < desc->field_count(); i++) {
    const FieldDescriptor* field = desc->field(i);
    GenerateClassFieldFromObject(options, printer, field);
  }

  printer->Print(
      "  return msg;\n"
      "};\n"
      "}\n");
}

void Generator::GenerateClassFieldFromObject(
    const GeneratorOptions& options,
    io::Printer* printer,
    const FieldDescriptor* field) const {
  if (IsMap(options, field)) {
    const FieldDescriptor* value_field = MapFieldValue(field);
    if (value_field->type() == FieldDescriptor::TYPE_MESSAGE) {
      // Since the map values are of message type, we have to do some extra work
      // to recursively call fromObject() on them before setting the map field.
      printer->Print(
          "  goog.isDef(obj.$name$) && jspb.Message.setWrapperField(\n"
          "      msg, $index$, jspb.Map.fromObject(obj.$name$, $fieldclass$, "
          "$fieldclass$.fromObject));\n",
          "name", JSObjectFieldName(options, field),
          "index", JSFieldIndex(field),
          "fieldclass", GetPath(options, value_field->message_type()));
    } else {
      // `msg` is a newly-constructed message object that has not yet built any
      // map containers wrapping underlying arrays, so we can simply directly
      // set the array here without fear of a stale wrapper.
      printer->Print(
          "  goog.isDef(obj.$name$) && "
          "jspb.Message.setField(msg, $index$, obj.$name$);\n",
          "name", JSObjectFieldName(options, field),
          "index", JSFieldIndex(field));
    }
  } else if (field->cpp_type() == FieldDescriptor::CPPTYPE_MESSAGE) {
    // Message field (singular or repeated)
    if (field->is_repeated()) {
      {
        printer->Print(
            "  goog.isDef(obj.$name$) && "
            "jspb.Message.setRepeatedWrapperField(\n"
            "      msg, $index$, goog.array.map(obj.$name$, function(i) {\n"
            "        return $fieldclass$.fromObject(i);\n"
            "      }));\n",
            "name", JSObjectFieldName(options, field),
            "index", JSFieldIndex(field),
            "fieldclass", SubmessageTypeRef(options, field));
      }
    } else {
      printer->Print(
          "  goog.isDef(obj.$name$) && jspb.Message.setWrapperField(\n"
          "      msg, $index$, $fieldclass$.fromObject(obj.$name$));\n",
          "name", JSObjectFieldName(options, field),
          "index", JSFieldIndex(field),
          "fieldclass", SubmessageTypeRef(options, field));
    }
  } else {
    // Simple (primitive) field.
    printer->Print(
        "  goog.isDef(obj.$name$) && jspb.Message.setField(msg, $index$, "
        "obj.$name$);\n",
        "name", JSObjectFieldName(options, field),
        "index", JSFieldIndex(field));
  }
}

void Generator::GenerateClassRegistration(const GeneratorOptions& options,
                                          io::Printer* printer,
                                          const Descriptor* desc) const {
  // Register any extensions defined inside this message type.
  for (int i = 0; i < desc->extension_count(); i++) {
    const FieldDescriptor* extension = desc->extension(i);
    if (ShouldGenerateExtension(extension)) {
      GenerateExtension(options, printer, extension);
    }
  }

}

void Generator::GenerateClassFields(const GeneratorOptions& options,
                                    io::Printer* printer,
                                    const Descriptor* desc) const {
  for (int i = 0; i < desc->field_count(); i++) {
    if (!IgnoreField(desc->field(i))) {
      GenerateClassField(options, printer, desc->field(i));
    }
  }
}

void GenerateBytesWrapper(const GeneratorOptions& options,
                          io::Printer* printer,
                          const FieldDescriptor* field,
                          BytesMode bytes_mode) {
  string type = JSFieldTypeAnnotation(
      options, field,
      /* is_setter_argument = */ false,
      /* force_present = */ false,
      /* singular_if_not_packed = */ false, bytes_mode);
  printer->Print(
      "/**\n"
      " * $fielddef$\n"
      "$comment$"
      " * This is a type-conversion wrapper around `get$defname$()`\n"
      " * @return {$type$}\n"
      " */\n"
      "$class$.prototype.get$name$ = function() {\n"
      "  return /** @type {$type$} */ (jspb.Message.bytes$list$As$suffix$(\n"
      "      this.get$defname$()));\n"
      "};\n"
      "\n"
      "\n",
      "fielddef", FieldDefinition(options, field),
      "comment", FieldComments(field, bytes_mode),
      "type", type,
      "class", GetPath(options, field->containing_type()),
      "name", JSGetterName(options, field, bytes_mode),
      "list", field->is_repeated() ? "List" : "",
      "suffix", JSByteGetterSuffix(bytes_mode),
      "defname", JSGetterName(options, field, BYTES_DEFAULT));
}


void Generator::GenerateClassField(const GeneratorOptions& options,
                                   io::Printer* printer,
                                   const FieldDescriptor* field) const {
  if (IsMap(options, field)) {
    const FieldDescriptor* key_field = MapFieldKey(field);
    const FieldDescriptor* value_field = MapFieldValue(field);
    // Map field: special handling to instantiate the map object on demand.
    string key_type =
        JSFieldTypeAnnotation(
            options, key_field,
            /* is_setter_argument = */ false,
            /* force_present = */ true,
            /* singular_if_not_packed = */ false);
    string value_type =
        JSFieldTypeAnnotation(
            options, value_field,
            /* is_setter_argument = */ false,
            /* force_present = */ true,
            /* singular_if_not_packed = */ false);

    printer->Print(
        "/**\n"
        " * $fielddef$\n"
        " * @param {boolean=} opt_noLazyCreate Do not create the map if\n"
        " * empty, instead returning `undefined`\n"
        " * @return {!jspb.Map<$keytype$,$valuetype$>}\n"
        " */\n",
        "fielddef", FieldDefinition(options, field),
        "keytype", key_type,
        "valuetype", value_type);
    printer->Print(
        "$class$.prototype.get$name$ = function(opt_noLazyCreate) {\n"
        "  return /** @type {!jspb.Map<$keytype$,$valuetype$>} */ (\n",
        "class", GetPath(options, field->containing_type()),
        "name", JSGetterName(options, field),
        "keytype", key_type,
        "valuetype", value_type);
    printer->Print(
        "      jspb.Message.getMapField(this, $index$, opt_noLazyCreate",
        "index", JSFieldIndex(field));

    if (value_field->type() == FieldDescriptor::TYPE_MESSAGE) {
      printer->Print(",\n"
          "      $messageType$",
            "messageType", GetPath(options, value_field->message_type()));
    } else {
      printer->Print(",\n"
          "      null");
    }

    printer->Print(
        "));\n");

    printer->Print(
        "};\n"
        "\n"
        "\n");
  } else if (field->cpp_type() == FieldDescriptor::CPPTYPE_MESSAGE) {
    // Message field: special handling in order to wrap the underlying data
    // array with a message object.

    printer->Print(
        "/**\n"
        " * $fielddef$\n"
        "$comment$"
        " * @return {$type$}\n"
        " */\n",
        "fielddef", FieldDefinition(options, field),
        "comment", FieldComments(field, BYTES_DEFAULT),
        "type", JSFieldTypeAnnotation(options, field,
                                      /* is_setter_argument = */ false,
                                      /* force_present = */ false,
                                      /* singular_if_not_packed = */ false));
    printer->Print(
        "$class$.prototype.get$name$ = function() {\n"
        "  return /** @type{$type$} */ (\n"
        "    jspb.Message.get$rpt$WrapperField(this, $wrapperclass$, "
        "$index$$required$));\n"
        "};\n"
        "\n"
        "\n",
        "class", GetPath(options, field->containing_type()),
        "name", JSGetterName(options, field),
        "type", JSFieldTypeAnnotation(options, field,
                                      /* is_setter_argument = */ false,
                                      /* force_present = */ false,
                                      /* singular_if_not_packed = */ false),
        "rpt", (field->is_repeated() ? "Repeated" : ""),
        "index", JSFieldIndex(field),
        "wrapperclass", SubmessageTypeRef(options, field),
        "required", (field->label() == FieldDescriptor::LABEL_REQUIRED ?
                     ", 1" : ""));
    printer->Print(
        "/** @param {$optionaltype$} value$returndoc$ */\n"
        "$class$.prototype.set$name$ = function(value) {\n"
        "  jspb.Message.set$oneoftag$$repeatedtag$WrapperField(",
        "optionaltype",
        JSFieldTypeAnnotation(options, field,
                              /* is_setter_argument = */ true,
                              /* force_present = */ false,
                              /* singular_if_not_packed = */ false),
        "returndoc", JSReturnDoc(options, field),
        "class", GetPath(options, field->containing_type()),
        "name", JSGetterName(options, field),
        "oneoftag", (field->containing_oneof() ? "Oneof" : ""),
        "repeatedtag", (field->is_repeated() ? "Repeated" : ""));

    printer->Print(
        "this, $index$$oneofgroup$, value);$returnvalue$\n"
        "};\n"
        "\n"
        "\n",
        "index", JSFieldIndex(field),
        "oneofgroup", (field->containing_oneof() ?
                       (", " + JSOneofArray(options, field)) : ""),
        "returnvalue", JSReturnClause(field));

    if (field->is_repeated()) {
      GenerateRepeatedMessageHelperMethods(options, printer, field);
    }

  } else {
    bool untyped =
        false;

    // Simple (primitive) field, either singular or repeated.

    // TODO(b/26173701): Always use BYTES_DEFAULT for the getter return type;
    // at this point we "lie" to non-binary users and tell the the return
    // type is always base64 string, pending a LSC to migrate to typed getters.
    BytesMode bytes_mode =
        field->type() == FieldDescriptor::TYPE_BYTES && !options.binary ?
            BYTES_B64 : BYTES_DEFAULT;
    string typed_annotation = JSFieldTypeAnnotation(
        options, field,
        /* is_setter_argument = */ false,
        /* force_present = */ false,
        /* singular_if_not_packed = */ false,
        /* bytes_mode = */ bytes_mode);
    if (untyped) {
      printer->Print(
          "/**\n"
          " * @return {?} Raw field, untyped.\n"
          " */\n");
    } else {
      printer->Print(
          "/**\n"
          " * $fielddef$\n"
          "$comment$"
          " * @return {$type$}\n"
          " */\n",
          "fielddef", FieldDefinition(options, field),
          "comment", FieldComments(field, bytes_mode),
          "type", typed_annotation);
    }

    printer->Print(
        "$class$.prototype.get$name$ = function() {\n",
        "class", GetPath(options, field->containing_type()),
        "name", JSGetterName(options, field));

    if (untyped) {
      printer->Print(
          "  return ");
    } else {
      printer->Print(
          "  return /** @type {$type$} */ (",
          "type", typed_annotation);
    }

    bool use_default = !ReturnsNullWhenUnset(options, field);

    // Raw fields with no default set should just return undefined.
    if (untyped && !field->has_default_value()) {
      use_default = false;
    }

    // Repeated fields get initialized to their default in the constructor
    // (why?), so we emit a plain getField() call for them.
    if (field->is_repeated()) {
      use_default = false;
    }

    GenerateFieldValueExpression(printer, "this", field, use_default);

    if (untyped) {
      printer->Print(
          ";\n"
          "};\n"
          "\n"
          "\n");
    } else {
      printer->Print(
          ");\n"
          "};\n"
          "\n"
          "\n");
    }

    if (field->type() == FieldDescriptor::TYPE_BYTES && !untyped) {
      GenerateBytesWrapper(options, printer, field, BYTES_B64);
      GenerateBytesWrapper(options, printer, field, BYTES_U8);
    }

    if (untyped) {
      printer->Print(
          "/**\n"
          " * @param {*} value$returndoc$\n"
          " */\n",
          "returndoc", JSReturnDoc(options, field));
    } else {
      printer->Print(
          "/** @param {$optionaltype$} value$returndoc$ */\n", "optionaltype",
          JSFieldTypeAnnotation(
              options, field,
              /* is_setter_argument = */ true,
              /* force_present = */ false,
              /* singular_if_not_packed = */ false),
          "returndoc", JSReturnDoc(options, field));
    }
    printer->Print(
        "$class$.prototype.set$name$ = function(value) {\n"
        "  jspb.Message.set$oneoftag$Field(this, $index$",
        "class", GetPath(options, field->containing_type()),
        "name", JSGetterName(options, field),
        "oneoftag", (field->containing_oneof() ? "Oneof" : ""),
        "index", JSFieldIndex(field));
    printer->Print(
        "$oneofgroup$, $type$value$rptvalueinit$$typeclose$);$returnvalue$\n"
        "};\n"
        "\n"
        "\n",
        "type",
        untyped ? "/** @type{string|number|boolean|Array|undefined} */(" : "",
        "typeclose", untyped ? ")" : "",
        "oneofgroup",
        (field->containing_oneof() ? (", " + JSOneofArray(options, field))
                                   : ""),
        "returnvalue", JSReturnClause(field), "rptvalueinit",
        (field->is_repeated() ? " || []" : ""));

    if (untyped) {
      printer->Print(
          "/**\n"
          " * Clears the value.$returndoc$\n"
          " */\n",
          "returndoc", JSReturnDoc(options, field));
    }


    if (field->is_repeated()) {
      GenerateRepeatedPrimitiveHelperMethods(options, printer, field, untyped);
    }
  }

  // Generate clearFoo() method for map fields, repeated fields, and other
  // fields with presence.
  if (IsMap(options, field)) {
    printer->Print(
        "$class$.prototype.clear$name$ = function() {\n"
        "  this.get$name$().clear();$returnvalue$\n"
        "};\n"
        "\n"
        "\n",
        "class", GetPath(options, field->containing_type()),
        "name", JSGetterName(options, field),
        "returnvalue", JSReturnClause(field));
  } else if (field->is_repeated() ||
             (field->cpp_type() == FieldDescriptor::CPPTYPE_MESSAGE &&
              !field->is_required())) {
    // Fields where we can delegate to the regular setter.
    printer->Print(
        "$class$.prototype.clear$name$ = function() {\n"
        "  this.set$name$($clearedvalue$);$returnvalue$\n"
        "};\n"
        "\n"
        "\n",
        "class", GetPath(options, field->containing_type()),
        "name", JSGetterName(options, field),
        "clearedvalue", (field->is_repeated() ? "[]" : "undefined"),
        "returnvalue", JSReturnClause(field));
  } else if (HasFieldPresence(options, field)) {
    // Fields where we can't delegate to the regular setter because it doesn't
    // accept "undefined" as an argument.
    printer->Print(
        "$class$.prototype.clear$name$ = function() {\n"
        "  jspb.Message.set$maybeoneof$Field(this, "
        "$index$$maybeoneofgroup$, ",
        "class", GetPath(options, field->containing_type()),
        "name", JSGetterName(options, field),
        "maybeoneof", (field->containing_oneof() ? "Oneof" : ""),
        "maybeoneofgroup", (field->containing_oneof() ?
                           (", " + JSOneofArray(options, field)) : ""),
        "index", JSFieldIndex(field));
    printer->Print(
        "$clearedvalue$);$returnvalue$\n"
        "};\n"
        "\n"
        "\n",
        "clearedvalue", (field->is_repeated() ? "[]" : "undefined"),
        "returnvalue", JSReturnClause(field));
  }

  if (HasFieldPresence(options, field)) {
    printer->Print(
        "/**\n"
        " * Returns whether this field is set.\n"
        " * @return {!boolean}\n"
        " */\n"
        "$class$.prototype.has$name$ = function() {\n"
        "  return jspb.Message.getField(this, $index$) != null;\n"
        "};\n"
        "\n"
        "\n",
        "class", GetPath(options, field->containing_type()),
        "name", JSGetterName(options, field),
        "index", JSFieldIndex(field));
  }
}

void Generator::GenerateRepeatedPrimitiveHelperMethods(
    const GeneratorOptions& options, io::Printer* printer,
    const FieldDescriptor* field, bool untyped) const {
  printer->Print(
      "/**\n"
      " * @param {!$optionaltype$} value\n"
      " * @param {number=} opt_index\n"
      " */\n"
      "$class$.prototype.add$name$ = function(value, opt_index) {\n"
      "  jspb.Message.addToRepeatedField(this, $index$",
      "class", GetPath(options, field->containing_type()), "name",
      JSGetterName(options, field, BYTES_DEFAULT,
                   /* drop_list = */ true),
      "optionaltype", JSTypeName(options, field, BYTES_DEFAULT), "index",
      JSFieldIndex(field));
  printer->Print(
      "$oneofgroup$, $type$value$rptvalueinit$$typeclose$, opt_index);\n"
      "};\n"
      "\n"
      "\n",
      "type", untyped ? "/** @type{string|number|boolean|!Uint8Array} */(" : "",
      "typeclose", untyped ? ")" : "", "oneofgroup",
      (field->containing_oneof() ? (", " + JSOneofArray(options, field)) : ""),
      "rptvalueinit", "");
}

void Generator::GenerateRepeatedMessageHelperMethods(
    const GeneratorOptions& options, io::Printer* printer,
    const FieldDescriptor* field) const {
  printer->Print(
      "/**\n"
      " * @param {!$optionaltype$=} opt_value\n"
      " * @param {number=} opt_index\n"
      " * @return {!$optionaltype$}\n"
      " */\n"
      "$class$.prototype.add$name$ = function(opt_value, opt_index) {\n"
      "  return jspb.Message.addTo$repeatedtag$WrapperField(",
      "optionaltype", JSTypeName(options, field, BYTES_DEFAULT), "class",
      GetPath(options, field->containing_type()), "name",
      JSGetterName(options, field, BYTES_DEFAULT,
                   /* drop_list = */ true),
      "repeatedtag", (field->is_repeated() ? "Repeated" : ""));

  printer->Print(
      "this, $index$$oneofgroup$, opt_value, $ctor$, opt_index);\n"
      "};\n"
      "\n"
      "\n",
      "index", JSFieldIndex(field), "oneofgroup",
      (field->containing_oneof() ? (", " + JSOneofArray(options, field)) : ""),
      "ctor", GetPath(options, field->message_type()));
}

void Generator::GenerateClassExtensionFieldInfo(const GeneratorOptions& options,
                                                io::Printer* printer,
                                                const Descriptor* desc) const {
  if (IsExtendable(desc)) {
    printer->Print(
        "\n"
        "/**\n"
        " * The extensions registered with this message class. This is a "
        "map of\n"
        " * extension field number to fieldInfo object.\n"
        " *\n"
        " * For example:\n"
        " *     { 123: {fieldIndex: 123, fieldName: {my_field_name: 0}, "
        "ctor: proto.example.MyMessage} }\n"
        " *\n"
        " * fieldName contains the JsCompiler renamed field name property "
        "so that it\n"
        " * works in OPTIMIZED mode.\n"
        " *\n"
        " * @type {!Object.<number, jspb.ExtensionFieldInfo>}\n"
        " */\n"
        "$class$.extensions = {};\n"
        "\n",
        "class", GetPath(options, desc));

    printer->Print(
        "\n"
        "/**\n"
        " * The extensions registered with this message class. This is a "
        "map of\n"
        " * extension field number to fieldInfo object.\n"
        " *\n"
        " * For example:\n"
        " *     { 123: {fieldIndex: 123, fieldName: {my_field_name: 0}, "
        "ctor: proto.example.MyMessage} }\n"
        " *\n"
        " * fieldName contains the JsCompiler renamed field name property "
        "so that it\n"
        " * works in OPTIMIZED mode.\n"
        " *\n"
        " * @type {!Object.<number, jspb.ExtensionFieldBinaryInfo>}\n"
        " */\n"
        "$class$.extensionsBinary = {};\n"
        "\n",
        "class", GetPath(options, desc));
  }
}


void Generator::GenerateClassDeserializeBinary(const GeneratorOptions& options,
                                               io::Printer* printer,
                                               const Descriptor* desc) const {
  // TODO(cfallin): Handle lazy decoding when requested by field option and/or
  // by default for 'bytes' fields and packed repeated fields.

  printer->Print(
      "/**\n"
      " * Deserializes binary data (in protobuf wire format).\n"
      " * @param {jspb.ByteSource} bytes The bytes to deserialize.\n"
      " * @return {!$class$}\n"
      " */\n"
      "$class$.deserializeBinary = function(bytes) {\n"
      "  var reader = new jspb.BinaryReader(bytes);\n"
      "  var msg = new $class$;\n"
      "  return $class$.deserializeBinaryFromReader(msg, reader);\n"
      "};\n"
      "\n"
      "\n"
      "/**\n"
      " * Deserializes binary data (in protobuf wire format) from the\n"
      " * given reader into the given message object.\n"
      " * @param {!$class$} msg The message object to deserialize into.\n"
      " * @param {!jspb.BinaryReader} reader The BinaryReader to use.\n"
      " * @return {!$class$}\n"
      " */\n"
      "$class$.deserializeBinaryFromReader = function(msg, reader) {\n"
      "  while (reader.nextField()) {\n"
      "    if (reader.isEndGroup()) {\n"
      "      break;\n"
      "    }\n"
      "    var field = reader.getFieldNumber();\n"
      "    switch (field) {\n",
      "class", GetPath(options, desc));

  for (int i = 0; i < desc->field_count(); i++) {
    if (!IgnoreField(desc->field(i))) {
      GenerateClassDeserializeBinaryField(options, printer, desc->field(i));
    }
  }

  printer->Print(
      "    default:\n");
  if (IsExtendable(desc)) {
    printer->Print(
        "      jspb.Message.readBinaryExtension(msg, reader, $extobj$Binary,\n"
        "        $class$.prototype.getExtension,\n"
        "        $class$.prototype.setExtension);\n"
        "      break;\n",
        "extobj", JSExtensionsObjectName(options, desc->file(), desc),
        "class", GetPath(options, desc));
  } else {
    printer->Print(
        "      reader.skipField();\n"
        "      break;\n");
  }

  printer->Print(
      "    }\n"
      "  }\n"
      "  return msg;\n"
      "};\n"
      "\n"
      "\n");
}

void Generator::GenerateClassDeserializeBinaryField(
    const GeneratorOptions& options,
    io::Printer* printer,
    const FieldDescriptor* field) const {

  printer->Print("    case $num$:\n",
                 "num", SimpleItoa(field->number()));

  if (IsMap(options, field)) {
    const FieldDescriptor* key_field = MapFieldKey(field);
    const FieldDescriptor* value_field = MapFieldValue(field);
    printer->Print(
        "      var value = msg.get$name$();\n"
        "      reader.readMessage(value, function(message, reader) {\n",
        "name", JSGetterName(options, field));

    printer->Print("        jspb.Map.deserializeBinary(message, reader, "
                   "$keyReaderFn$, $valueReaderFn$",
          "keyReaderFn", JSBinaryReaderMethodName(options, key_field),
          "valueReaderFn", JSBinaryReaderMethodName(options, value_field));

    if (value_field->type() == FieldDescriptor::TYPE_MESSAGE) {
      printer->Print(", $messageType$.deserializeBinaryFromReader",
          "messageType", GetPath(options, value_field->message_type()));
    }

    printer->Print(");\n");
    printer->Print("         });\n");
  } else {
    if (field->cpp_type() == FieldDescriptor::CPPTYPE_MESSAGE) {
      printer->Print(
          "      var value = new $fieldclass$;\n"
          "      reader.read$msgOrGroup$($grpfield$value,"
          "$fieldclass$.deserializeBinaryFromReader);\n",
        "fieldclass", SubmessageTypeRef(options, field),
          "msgOrGroup", (field->type() == FieldDescriptor::TYPE_GROUP) ?
                        "Group" : "Message",
          "grpfield", (field->type() == FieldDescriptor::TYPE_GROUP) ?
                      (SimpleItoa(field->number()) + ", ") : "");
    } else {
      printer->Print(
          "      var value = /** @type {$fieldtype$} */ "
          "(reader.read$reader$());\n",
          "fieldtype", JSFieldTypeAnnotation(options, field, false, true,
                                             /* singular_if_not_packed */ true,
                                             BYTES_U8),
          "reader",
          JSBinaryReadWriteMethodName(field, /* is_writer = */ false));
    }

    if (field->is_repeated() && !field->is_packed()) {
      printer->Print(
          "      msg.add$name$(value);\n", "name",
          JSGetterName(options, field, BYTES_DEFAULT, /* drop_list = */ true));
    } else {
      // Singular fields, and packed repeated fields, receive a |value| either
      // as the field's value or as the array of all the field's values; set
      // this as the field's value directly.
      printer->Print(
          "      msg.set$name$(value);\n",
          "name", JSGetterName(options, field));
    }
  }

  printer->Print("      break;\n");
}

void Generator::GenerateClassSerializeBinary(const GeneratorOptions& options,
                                             io::Printer* printer,
                                             const Descriptor* desc) const {
  printer->Print(
      "/**\n"
      " * Serializes the message to binary data (in protobuf wire format).\n"
      " * @return {!Uint8Array}\n"
      " */\n"
      "$class$.prototype.serializeBinary = function() {\n"
      "  var writer = new jspb.BinaryWriter();\n"
      "  $class$.serializeBinaryToWriter(this, writer);\n"
      "  return writer.getResultBuffer();\n"
      "};\n"
      "\n"
      "\n"
      "/**\n"
      " * Serializes the given message to binary data (in protobuf wire\n"
      " * format), writing to the given BinaryWriter.\n"
      " * @param {!$class$} message\n"
      " * @param {!jspb.BinaryWriter} writer\n"
      " */\n"
      "$class$.serializeBinaryToWriter = function(message, "
      "writer) {\n"
      "  var f = undefined;\n",
      "class", GetPath(options, desc));

  for (int i = 0; i < desc->field_count(); i++) {
    if (!IgnoreField(desc->field(i))) {
      GenerateClassSerializeBinaryField(options, printer, desc->field(i));
    }
  }

  if (IsExtendable(desc)) {
    printer->Print(
        "  jspb.Message.serializeBinaryExtensions(message, writer,\n"
        "    $extobj$Binary, $class$.prototype.getExtension);\n",
        "extobj", JSExtensionsObjectName(options, desc->file(), desc),
        "class", GetPath(options, desc));
  }

  printer->Print(
      "};\n"
      "\n"
      "\n");
}

void Generator::GenerateClassSerializeBinaryField(
    const GeneratorOptions& options,
    io::Printer* printer,
    const FieldDescriptor* field) const {
  if (HasFieldPresence(options, field) &&
      field->cpp_type() != FieldDescriptor::CPPTYPE_MESSAGE) {
    string typed_annotation = JSFieldTypeAnnotation(
        options, field,
        /* is_setter_argument = */ false,
        /* force_present = */ false,
        /* singular_if_not_packed = */ false,
        /* bytes_mode = */ BYTES_DEFAULT);
    printer->Print(
        "  f = /** @type {$type$} */ "
        "(jspb.Message.getField(message, $index$));\n",
        "index", JSFieldIndex(field),
        "type", typed_annotation);
  } else {
    printer->Print(
        "  f = message.get$name$($nolazy$);\n",
        "name", JSGetterName(options, field, BYTES_U8),
        // No lazy creation for maps containers -- fastpath the empty case.
        "nolazy", IsMap(options, field) ? "true" : "");
  }

  // Print an `if (condition)` statement that evaluates to true if the field
  // goes on the wire.
  if (IsMap(options, field)) {
    printer->Print(
        "  if (f && f.getLength() > 0) {\n");
  } else if (field->is_repeated()) {
    printer->Print(
        "  if (f.length > 0) {\n");
  } else {
    if (HasFieldPresence(options, field)) {
      printer->Print(
          "  if (f != null) {\n");
    } else {
      // No field presence: serialize onto the wire only if value is
      // non-default.  Defaults are documented here:
      // https://goto.google.com/lhdfm
      switch (field->cpp_type()) {
        case FieldDescriptor::CPPTYPE_INT32:
        case FieldDescriptor::CPPTYPE_INT64:
        case FieldDescriptor::CPPTYPE_UINT32:
        case FieldDescriptor::CPPTYPE_UINT64: {
          {
            printer->Print("  if (f !== 0) {\n");
          }
          break;
        }

        case FieldDescriptor::CPPTYPE_ENUM:
        case FieldDescriptor::CPPTYPE_FLOAT:
        case FieldDescriptor::CPPTYPE_DOUBLE:
          printer->Print(
              "  if (f !== 0.0) {\n");
          break;
        case FieldDescriptor::CPPTYPE_BOOL:
          printer->Print(
              "  if (f) {\n");
          break;
        case FieldDescriptor::CPPTYPE_STRING:
          printer->Print(
              "  if (f.length > 0) {\n");
          break;
        default:
          assert(false);
          break;
      }
    }
  }

  // Write the field on the wire.
  if (IsMap(options, field)) {
    const FieldDescriptor* key_field = MapFieldKey(field);
    const FieldDescriptor* value_field = MapFieldValue(field);
    printer->Print(
        "    f.serializeBinary($index$, writer, "
                              "$keyWriterFn$, $valueWriterFn$",
        "index", SimpleItoa(field->number()),
        "keyWriterFn", JSBinaryWriterMethodName(options, key_field),
        "valueWriterFn", JSBinaryWriterMethodName(options, value_field));

    if (value_field->type() == FieldDescriptor::TYPE_MESSAGE) {
      printer->Print(", $messageType$.serializeBinaryToWriter",
          "messageType", GetPath(options, value_field->message_type()));
    }

    printer->Print(");\n");
  } else {
    printer->Print(
        "    writer.write$method$(\n"
        "      $index$,\n"
        "      f",
        "method", JSBinaryReadWriteMethodName(field, /* is_writer = */ true),
        "index", SimpleItoa(field->number()));

    if (field->cpp_type() == FieldDescriptor::CPPTYPE_MESSAGE &&
        !IsMap(options, field)) {
      printer->Print(
          ",\n"
          "      $submsg$.serializeBinaryToWriter\n",
        "submsg", SubmessageTypeRef(options, field));
    } else {
      printer->Print("\n");
    }

    printer->Print(
        "    );\n");
  }

  // Close the `if`.
  printer->Print(
      "  }\n");
}

void Generator::GenerateEnum(const GeneratorOptions& options,
                             io::Printer* printer,
                             const EnumDescriptor* enumdesc) const {
  printer->Print(
      "/**\n"
      " * @enum {number}\n"
      " */\n"
      "$name$ = {\n",
      "name", GetPath(options, enumdesc));

  for (int i = 0; i < enumdesc->value_count(); i++) {
    const EnumValueDescriptor* value = enumdesc->value(i);
    printer->Print(
        "  $name$: $value$$comma$\n",
        "name", ToEnumCase(value->name()),
        "value", SimpleItoa(value->number()),
        "comma", (i == enumdesc->value_count() - 1) ? "" : ",");
  }

  printer->Print(
      "};\n"
      "\n");
}

void Generator::GenerateExtension(const GeneratorOptions& options,
                                  io::Printer* printer,
                                  const FieldDescriptor* field) const {
  string extension_scope =
      (field->extension_scope() ?
       GetPath(options, field->extension_scope()) :
       GetPath(options, field->file()));

  printer->Print(
      "\n"
      "/**\n"
      " * A tuple of {field number, class constructor} for the extension\n"
      " * field named `$name$`.\n"
      " * @type {!jspb.ExtensionFieldInfo.<$extensionType$>}\n"
      " */\n"
      "$class$.$name$ = new jspb.ExtensionFieldInfo(\n",
      "name", JSObjectFieldName(options, field),
      "class", extension_scope,
      "extensionType", JSFieldTypeAnnotation(
          options, field,
          /* is_setter_argument = */ false,
          /* force_present = */ true,
          /* singular_if_not_packed = */ false));
  printer->Print(
      "    $index$,\n"
      "    {$name$: 0},\n"
      "    $ctor$,\n"
      "     /** @type {?function((boolean|undefined),!jspb.Message=): "
      "!Object} */ (\n"
      "         $toObject$),\n"
      "    $repeated$);\n",
      "index", SimpleItoa(field->number()),
      "name", JSObjectFieldName(options, field),
      "ctor", (field->cpp_type() == FieldDescriptor::CPPTYPE_MESSAGE ?
               SubmessageTypeRef(options, field) : string("null")),
      "toObject", (field->cpp_type() == FieldDescriptor::CPPTYPE_MESSAGE ?
                   (SubmessageTypeRef(options, field) + ".toObject") :
                   string("null")),
      "repeated", (field->is_repeated() ? "1" : "0"));

  printer->Print(
      "\n"
      "$extendName$Binary[$index$] = new jspb.ExtensionFieldBinaryInfo(\n"
      "    $class$.$name$,\n"
      "    $binaryReaderFn$,\n"
      "    $binaryWriterFn$,\n"
      "    $binaryMessageSerializeFn$,\n"
      "    $binaryMessageDeserializeFn$,\n",
      "extendName",
      JSExtensionsObjectName(options, field->file(), field->containing_type()),
      "index", SimpleItoa(field->number()), "class", extension_scope, "name",
      JSObjectFieldName(options, field), "binaryReaderFn",
      JSBinaryReaderMethodName(options, field), "binaryWriterFn",
      JSBinaryWriterMethodName(options, field), "binaryMessageSerializeFn",
      (field->cpp_type() == FieldDescriptor::CPPTYPE_MESSAGE)
          ? (SubmessageTypeRef(options, field) + ".serializeBinaryToWriter")
          : "undefined",
      "binaryMessageDeserializeFn",
      (field->cpp_type() == FieldDescriptor::CPPTYPE_MESSAGE)
          ? (SubmessageTypeRef(options, field) + ".deserializeBinaryFromReader")
          : "undefined");

  printer->Print("    $isPacked$);\n", "isPacked",
                 (field->is_packed() ? "true" : "false"));

  printer->Print(
      "// This registers the extension field with the extended class, so that\n"
      "// toObject() will function correctly.\n"
      "$extendName$[$index$] = $class$.$name$;\n"
      "\n",
      "extendName", JSExtensionsObjectName(options, field->file(),
                                           field->containing_type()),
      "index", SimpleItoa(field->number()),
      "class", extension_scope,
      "name", JSObjectFieldName(options, field));
}

bool GeneratorOptions::ParseFromOptions(
    const std::vector< std::pair< string, string > >& options,
    string* error) {
  for (int i = 0; i < options.size(); i++) {
    if (options[i].first == "add_require_for_enums") {
      if (options[i].second != "") {
        *error = "Unexpected option value for add_require_for_enums";
        return false;
      }
      add_require_for_enums = true;
    } else if (options[i].first == "binary") {
      if (options[i].second != "") {
        *error = "Unexpected option value for binary";
        return false;
      }
      binary = true;
    } else if (options[i].first == "testonly") {
      if (options[i].second != "") {
        *error = "Unexpected option value for testonly";
        return false;
      }
      testonly = true;
    } else if (options[i].first == "error_on_name_conflict") {
      if (options[i].second != "") {
        *error = "Unexpected option value for error_on_name_conflict";
        return false;
      }
      error_on_name_conflict = true;
    } else if (options[i].first == "output_dir") {
      output_dir = options[i].second;
    } else if (options[i].first == "namespace_prefix") {
      namespace_prefix = options[i].second;
    } else if (options[i].first == "library") {
      library = options[i].second;
    } else if (options[i].first == "import_style") {
      if (options[i].second == "closure") {
        import_style = kImportClosure;
      } else if (options[i].second == "commonjs") {
        import_style = kImportCommonJs;
      } else if (options[i].second == "browser") {
        import_style = kImportBrowser;
      } else if (options[i].second == "es6") {
        import_style = kImportEs6;
      } else {
        *error = "Unknown import style " + options[i].second + ", expected " +
                 "one of: closure, commonjs, browser, es6.";
      }
    } else if (options[i].first == "extension") {
      extension = options[i].second;
    } else if (options[i].first == "one_output_file_per_input_file") {
      if (!options[i].second.empty()) {
        *error = "Unexpected option value for one_output_file_per_input_file";
        return false;
      }
      one_output_file_per_input_file = true;
    } else {
      // Assume any other option is an output directory, as long as it is a bare
      // `key` rather than a `key=value` option.
      if (options[i].second != "") {
        *error = "Unknown option: " + options[i].first;
        return false;
      }
      output_dir = options[i].first;
    }
  }

  if (import_style != kImportClosure &&
      (add_require_for_enums || testonly || !library.empty() ||
       error_on_name_conflict || extension != ".js" ||
       one_output_file_per_input_file)) {
    *error =
        "The add_require_for_enums, testonly, library, error_on_name_conflict, "
        "extension, and one_output_file_per_input_file options should only be "
        "used for import_style=closure";
    return false;
  }

  return true;
}

GeneratorOptions::OutputMode GeneratorOptions::output_mode() const {
  // We use one output file per input file if we are not using Closure or if
  // this is explicitly requested.
  if (import_style != kImportClosure || one_output_file_per_input_file) {
    return kOneOutputFilePerInputFile;
  }

  // If a library name is provided, we put everything in that one file.
  if (!library.empty()) {
    return kEverythingInOneFile;
  }

  // Otherwise, we create one output file per type.
  return kOneOutputFilePerType;
}

void Generator::GenerateFilesInDepOrder(
    const GeneratorOptions& options,
    io::Printer* printer,
    const std::vector<const FileDescriptor*>& files) const {
  // Build a std::set over all files so that the DFS can detect when it recurses
  // into a dep not specified in the user's command line.
  std::set<const FileDescriptor*> all_files(files.begin(), files.end());
  // Track the in-progress set of files that have been generated already.
  std::set<const FileDescriptor*> generated;
  for (int i = 0; i < files.size(); i++) {
    GenerateFileAndDeps(options, printer, files[i], &all_files, &generated);
  }
}

void Generator::GenerateFileAndDeps(
    const GeneratorOptions& options,
    io::Printer* printer,
    const FileDescriptor* root,
    std::set<const FileDescriptor*>* all_files,
    std::set<const FileDescriptor*>* generated) const {
  // Skip if already generated.
  if (generated->find(root) != generated->end()) {
    return;
  }
  generated->insert(root);

  // Generate all dependencies before this file's content.
  for (int i = 0; i < root->dependency_count(); i++) {
    const FileDescriptor* dep = root->dependency(i);
    GenerateFileAndDeps(options, printer, dep, all_files, generated);
  }

  // Generate this file's content.  Only generate if the file is part of the
  // original set requested to be generated; i.e., don't take all transitive
  // deps down to the roots.
  if (all_files->find(root) != all_files->end()) {
    GenerateClassesAndEnums(options, printer, root);
  }
}

void Generator::GenerateFile(const GeneratorOptions& options,
                             io::Printer* printer,
                             const FileDescriptor* file) const {
  GenerateHeader(options, printer);

  // Generate "require" statements.
  if (options.import_style == GeneratorOptions::kImportCommonJs) {
    printer->Print("var jspb = require('google-protobuf');\n");
    printer->Print("var goog = jspb;\n");
    printer->Print("var global = Function('return this')();\n\n");

    for (int i = 0; i < file->dependency_count(); i++) {
      const string& name = file->dependency(i)->name();
      printer->Print(
          "var $alias$ = require('$file$');\n",
          "alias", ModuleAlias(name),
          "file", GetRootPath(file->name(), name) + GetJSFilename(options, name));
    }
  }

  std::set<string> provided;
  std::set<const FieldDescriptor*> extensions;
  for (int i = 0; i < file->extension_count(); i++) {
    // We honor the jspb::ignore option here only when working with
    // Closure-style imports. Use of this option is discouraged and so we want
    // to avoid adding new support for it.
    if (options.import_style == GeneratorOptions::kImportClosure &&
        IgnoreField(file->extension(i))) {
      continue;
    }
    provided.insert(GetPath(options, file) + "." +
                    JSObjectFieldName(options, file->extension(i)));
    extensions.insert(file->extension(i));
  }

  FindProvidesForFile(options, printer, file, &provided);
  GenerateProvides(options, printer, &provided);
  std::vector<const FileDescriptor*> files;
  files.push_back(file);
  if (options.import_style == GeneratorOptions::kImportClosure) {
    GenerateRequiresForLibrary(options, printer, files, &provided);
  }

  GenerateClassesAndEnums(options, printer, file);

  // Generate code for top-level extensions. Extensions nested inside messages
  // are emitted inside GenerateClassesAndEnums().
  for (std::set<const FieldDescriptor*>::const_iterator it = extensions.begin();
       it != extensions.end(); ++it) {
    GenerateExtension(options, printer, *it);
  }

  if (options.import_style == GeneratorOptions::kImportCommonJs) {
    printer->Print("goog.object.extend(exports, $package$);\n",
                   "package", GetPath(options, file));
  }

  // Emit well-known type methods.
  for (FileToc* toc = well_known_types_js; toc->name != NULL; toc++) {
    string name = string("google/protobuf/") + toc->name;
    if (name == StripProto(file->name()) + ".js") {
      printer->Print(toc->data);
    }
  }
}

bool Generator::GenerateAll(const std::vector<const FileDescriptor*>& files,
                            const string& parameter,
                            GeneratorContext* context,
                            string* error) const {
  std::vector< std::pair< string, string > > option_pairs;
  ParseGeneratorParameter(parameter, &option_pairs);
  GeneratorOptions options;
  if (!options.ParseFromOptions(option_pairs, error)) {
    return false;
  }


  if (options.output_mode() == GeneratorOptions::kEverythingInOneFile) {
    // All output should go in a single file.
    string filename = options.output_dir + "/" + options.library +
                      options.GetFileNameExtension();
    google::protobuf::scoped_ptr<io::ZeroCopyOutputStream> output(context->Open(filename));
    GOOGLE_CHECK(output.get());
    io::Printer printer(output.get(), '$');

    // Pull out all extensions -- we need these to generate all
    // provides/requires.
    std::vector<const FieldDescriptor*> extensions;
    for (int i = 0; i < files.size(); i++) {
      for (int j = 0; j < files[i]->extension_count(); j++) {
        const FieldDescriptor* extension = files[i]->extension(j);
        extensions.push_back(extension);
      }
    }

    GenerateHeader(options, &printer);

    std::set<string> provided;
    FindProvides(options, &printer, files, &provided);
    FindProvidesForFields(options, &printer, extensions, &provided);
    GenerateProvides(options, &printer, &provided);
    GenerateTestOnly(options, &printer);
    GenerateRequiresForLibrary(options, &printer, files, &provided);

    GenerateFilesInDepOrder(options, &printer, files);

    for (int i = 0; i < extensions.size(); i++) {
      if (ShouldGenerateExtension(extensions[i])) {
        GenerateExtension(options, &printer, extensions[i]);
      }
    }

    if (printer.failed()) {
      return false;
    }
  } else if (options.output_mode() == GeneratorOptions::kOneOutputFilePerType) {
    std::set<const void*> allowed_set;
    if (!GenerateJspbAllowedSet(options, files, &allowed_set, error)) {
      return false;
    }

    for (int i = 0; i < files.size(); i++) {
      const FileDescriptor* file = files[i];
      for (int j = 0; j < file->message_type_count(); j++) {
        const Descriptor* desc = file->message_type(j);
        if (allowed_set.count(desc) == 0) {
          continue;
        }

        string filename = GetMessageFileName(options, desc);
        google::protobuf::scoped_ptr<io::ZeroCopyOutputStream> output(
            context->Open(filename));
        GOOGLE_CHECK(output.get());
        io::Printer printer(output.get(), '$');

        GenerateHeader(options, &printer);

        std::set<string> provided;
        FindProvidesForMessage(options, &printer, desc, &provided);
        GenerateProvides(options, &printer, &provided);
        GenerateTestOnly(options, &printer);
        GenerateRequiresForMessage(options, &printer, desc, &provided);

        GenerateClass(options, &printer, desc);

        if (printer.failed()) {
          return false;
        }
      }
      for (int j = 0; j < file->enum_type_count(); j++) {
        const EnumDescriptor* enumdesc = file->enum_type(j);
        if (allowed_set.count(enumdesc) == 0) {
          continue;
        }

        string filename = GetEnumFileName(options, enumdesc);
        google::protobuf::scoped_ptr<io::ZeroCopyOutputStream> output(
            context->Open(filename));
        GOOGLE_CHECK(output.get());
        io::Printer printer(output.get(), '$');

        GenerateHeader(options, &printer);

        std::set<string> provided;
        FindProvidesForEnum(options, &printer, enumdesc, &provided);
        GenerateProvides(options, &printer, &provided);
        GenerateTestOnly(options, &printer);

        GenerateEnum(options, &printer, enumdesc);

        if (printer.failed()) {
          return false;
        }
      }
      // File-level extensions (message-level extensions are generated under
      // the enclosing message).
      if (allowed_set.count(file) == 1) {
        string filename = GetExtensionFileName(options, file);

        google::protobuf::scoped_ptr<io::ZeroCopyOutputStream> output(
            context->Open(filename));
        GOOGLE_CHECK(output.get());
        io::Printer printer(output.get(), '$');

        GenerateHeader(options, &printer);

        std::set<string> provided;
        std::vector<const FieldDescriptor*> fields;

        for (int j = 0; j < files[i]->extension_count(); j++) {
          if (ShouldGenerateExtension(files[i]->extension(j))) {
            fields.push_back(files[i]->extension(j));
          }
        }

        FindProvidesForFields(options, &printer, fields, &provided);
        GenerateProvides(options, &printer, &provided);
        GenerateTestOnly(options, &printer);
        GenerateRequiresForExtensions(options, &printer, fields, &provided);

        for (int j = 0; j < files[i]->extension_count(); j++) {
          if (ShouldGenerateExtension(files[i]->extension(j))) {
            GenerateExtension(options, &printer, files[i]->extension(j));
          }
        }
      }
    }
  } else /* options.output_mode() == kOneOutputFilePerInputFile */ {
    // Generate one output file per input (.proto) file.

    for (int i = 0; i < files.size(); i++) {
      const google::protobuf::FileDescriptor* file = files[i];

      string filename =
          options.output_dir + "/" + GetJSFilename(options, file->name());
      google::protobuf::scoped_ptr<io::ZeroCopyOutputStream> output(context->Open(filename));
      GOOGLE_CHECK(output.get());
      io::Printer printer(output.get(), '$');

      GenerateFile(options, &printer, file);

      if (printer.failed()) {
        return false;
      }
    }
  }

  return true;
}

}  // namespace js
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
