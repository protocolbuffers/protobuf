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

// Author: kenton@google.com (Kenton Varda)
//  Based on original Protocol Buffers design by
//  Sanjay Ghemawat, Jeff Dean, and others.

#include <limits>
#include <map>
#include <queue>
#include <unordered_set>
#include <vector>

#include <google/protobuf/stubs/logging.h>
#include <google/protobuf/stubs/common.h>
#include <google/protobuf/compiler/cpp/cpp_helpers.h>
#include <google/protobuf/compiler/scc.h>
#include <google/protobuf/io/printer.h>
#include <google/protobuf/io/zero_copy_stream.h>
#include <google/protobuf/wire_format.h>
#include <google/protobuf/wire_format_lite.h>
#include <google/protobuf/stubs/strutil.h>
#include <google/protobuf/stubs/substitute.h>


#include <google/protobuf/stubs/hash.h>


#include <google/protobuf/port_def.inc>

namespace google {
namespace protobuf {
namespace compiler {
namespace cpp {

namespace {

static const char kAnyMessageName[] = "Any";
static const char kAnyProtoFile[] = "google/protobuf/any.proto";
static const char kGoogleProtobufPrefix[] = "google/protobuf/";

string DotsToUnderscores(const string& name) {
  return StringReplace(name, ".", "_", true);
}

string DotsToColons(const string& name) {
  return StringReplace(name, ".", "::", true);
}

const char* const kKeywordList[] = {
  "alignas", "alignof", "and", "and_eq", "asm", "auto", "bitand", "bitor",
  "bool", "break", "case", "catch", "char", "class", "compl", "const",
  "constexpr", "const_cast", "continue", "decltype", "default", "delete", "do",
  "double", "dynamic_cast", "else", "enum", "explicit", "export", "extern",
  "false", "float", "for", "friend", "goto", "if", "inline", "int", "long",
  "mutable", "namespace", "new", "noexcept", "not", "not_eq", "nullptr",
  "operator", "or", "or_eq", "private", "protected", "public", "register",
  "reinterpret_cast", "return", "short", "signed", "sizeof", "static",
  "static_assert", "static_cast", "struct", "switch", "template", "this",
  "thread_local", "throw", "true", "try", "typedef", "typeid", "typename",
  "union", "unsigned", "using", "virtual", "void", "volatile", "wchar_t",
  "while", "xor", "xor_eq"
};

std::unordered_set<string> MakeKeywordsMap() {
  std::unordered_set<string> result;
  for (int i = 0; i < GOOGLE_ARRAYSIZE(kKeywordList); i++) {
    result.insert(kKeywordList[i]);
  }
  return result;
}

std::unordered_set<string> kKeywords = MakeKeywordsMap();

// Returns whether the provided descriptor has an extension. This includes its
// nested types.
bool HasExtension(const Descriptor* descriptor) {
  if (descriptor->extension_count() > 0) {
    return true;
  }
  for (int i = 0; i < descriptor->nested_type_count(); ++i) {
    if (HasExtension(descriptor->nested_type(i))) {
      return true;
    }
  }
  return false;
}

// Encode [0..63] as 'A'-'Z', 'a'-'z', '0'-'9', '_'
char Base63Char(int value) {
  GOOGLE_CHECK_GE(value, 0);
  if (value < 26) return 'A' + value;
  value -= 26;
  if (value < 26) return 'a' + value;
  value -= 26;
  if (value < 10) return '0' + value;
  GOOGLE_CHECK_EQ(value, 10);
  return '_';
}

// Given a c identifier has 63 legal characters we can't implement base64
// encoding. So we return the k least significant "digits" in base 63.
template <typename I>
string Base63(I n, int k) {
  string res;
  while (k-- > 0) {
    res += Base63Char(static_cast<int>(n % 63));
    n /= 63;
  }
  return res;
}

string IntTypeName(const Options& options, const string& type) {
  if (options.opensource_runtime) {
    return "::google::protobuf::" + type;
  } else {
    return "::" + type;
  }
}

string StringTypeName(const Options& options) {
  return options.opensource_runtime ? "::std::string" : "::std::string";
}

void SetIntVar(const Options& options, const string& type,
               std::map<string, string>* variables) {
  (*variables)[type] = IntTypeName(options, type);
}

}  // namespace

void SetCommonVars(const Options& options,
                   std::map<string, string>* variables) {
  (*variables)["proto_ns"] = ProtobufNamespace(options);

  // Warning: there is some clever naming/splitting here to avoid extract script
  // rewrites.  The names of these variables must not be things that the extract
  // script will rewrite.  That's why we use "CHK" (for example) instead of
  // "GOOGLE_CHECK".
  if (options.opensource_runtime) {
    (*variables)["GOOGLE_PROTOBUF"] = "GOOGLE_PROTOBUF";
    (*variables)["CHK"] = "GOOGLE_CHECK";
    (*variables)["DCHK"] = "GOOGLE_DCHECK";
  } else {
    // These values are things the extract script would rewrite if we did not
    // split them.  It might not strictly matter since we don't generate google3
    // code in open-source.  But it's good to prevent surprising things from
    // happening.
    (*variables)["GOOGLE_PROTOBUF"] = "GOOGLE3" "_PROTOBUF";
    (*variables)["CHK"] = "CH" "ECK";
    (*variables)["DCHK"] = "DCH" "ECK";
  }

  SetIntVar(options, "uint8", variables);
  SetIntVar(options, "uint32", variables);
  SetIntVar(options, "uint64", variables);
  SetIntVar(options, "int32", variables);
  SetIntVar(options, "int64", variables);
  (*variables)["string"] = StringTypeName(options);
}

string UnderscoresToCamelCase(const string& input, bool cap_next_letter) {
  string result;
  // Note:  I distrust ctype.h due to locales.
  for (int i = 0; i < input.size(); i++) {
    if ('a' <= input[i] && input[i] <= 'z') {
      if (cap_next_letter) {
        result += input[i] + ('A' - 'a');
      } else {
        result += input[i];
      }
      cap_next_letter = false;
    } else if ('A' <= input[i] && input[i] <= 'Z') {
      // Capital letters are left as-is.
      result += input[i];
      cap_next_letter = false;
    } else if ('0' <= input[i] && input[i] <= '9') {
      result += input[i];
      cap_next_letter = true;
    } else {
      cap_next_letter = true;
    }
  }
  return result;
}

const char kThickSeparator[] =
  "// ===================================================================\n";
const char kThinSeparator[] =
  "// -------------------------------------------------------------------\n";

bool CanInitializeByZeroing(const FieldDescriptor* field) {
  if (field->is_repeated() || field->is_extension()) return false;
  switch (field->cpp_type()) {
    case FieldDescriptor::CPPTYPE_ENUM:
      return field->default_value_enum()->number() == 0;
    case FieldDescriptor::CPPTYPE_INT32:
      return field->default_value_int32() == 0;
    case FieldDescriptor::CPPTYPE_INT64:
      return field->default_value_int64() == 0;
    case FieldDescriptor::CPPTYPE_UINT32:
      return field->default_value_uint32() == 0;
    case FieldDescriptor::CPPTYPE_UINT64:
      return field->default_value_uint64() == 0;
    case FieldDescriptor::CPPTYPE_FLOAT:
      return field->default_value_float() == 0;
    case FieldDescriptor::CPPTYPE_DOUBLE:
      return field->default_value_double() == 0;
    case FieldDescriptor::CPPTYPE_BOOL:
      return field->default_value_bool() == false;
    default:
      return false;
  }
}

string ClassName(const Descriptor* descriptor) {
  const Descriptor* parent = descriptor->containing_type();
  string res;
  if (parent) res += ClassName(parent) + "_";
  res += descriptor->name();
  if (IsMapEntryMessage(descriptor)) res += "_DoNotUse";
  return res;
}

string ClassName(const EnumDescriptor* enum_descriptor) {
  if (enum_descriptor->containing_type() == NULL) {
    return enum_descriptor->name();
  } else {
    return ClassName(enum_descriptor->containing_type()) + "_" +
           enum_descriptor->name();
  }
}

string QualifiedClassName(const Descriptor* d) {
  return Namespace(d) + "::" + ClassName(d);
}

string QualifiedClassName(const EnumDescriptor* d) {
  return Namespace(d) + "::" + ClassName(d);
}

string Namespace(const string& package) {
  if (package.empty()) return "";
  return "::" + DotsToColons(package);
}

string Namespace(const Descriptor* d) { return Namespace(d->file()); }

string Namespace(const FieldDescriptor* d) { return Namespace(d->file()); }

string Namespace(const EnumDescriptor* d) { return Namespace(d->file()); }

string DefaultInstanceName(const Descriptor* descriptor) {
  string prefix = descriptor->file()->package().empty() ? "" : "::";
  return prefix + DotsToColons(descriptor->file()->package()) + "::_" +
      ClassName(descriptor, false) + "_default_instance_";
}

string ReferenceFunctionName(const Descriptor* descriptor) {
  return QualifiedClassName(descriptor) + "_ReferenceStrong";
}

string SuperClassName(const Descriptor* descriptor, const Options& options) {
  return "::" + ProtobufNamespace(options) +
         (HasDescriptorMethods(descriptor->file(), options) ? "::Message"
                                                            : "::MessageLite");
}

string FieldName(const FieldDescriptor* field) {
  string result = field->name();
  LowerString(&result);
  if (kKeywords.count(result) > 0) {
    result.append("_");
  }
  return result;
}

string EnumValueName(const EnumValueDescriptor* enum_value) {
  string result = enum_value->name();
  if (kKeywords.count(result) > 0) {
    result.append("_");
  }
  return result;
}

int EstimateAlignmentSize(const FieldDescriptor* field) {
  if (field == NULL) return 0;
  if (field->is_repeated()) return 8;
  switch (field->cpp_type()) {
    case FieldDescriptor::CPPTYPE_BOOL:
      return 1;

    case FieldDescriptor::CPPTYPE_INT32:
    case FieldDescriptor::CPPTYPE_UINT32:
    case FieldDescriptor::CPPTYPE_ENUM:
    case FieldDescriptor::CPPTYPE_FLOAT:
      return 4;

    case FieldDescriptor::CPPTYPE_INT64:
    case FieldDescriptor::CPPTYPE_UINT64:
    case FieldDescriptor::CPPTYPE_DOUBLE:
    case FieldDescriptor::CPPTYPE_STRING:
    case FieldDescriptor::CPPTYPE_MESSAGE:
      return 8;
  }
  GOOGLE_LOG(FATAL) << "Can't get here.";
  return -1;  // Make compiler happy.
}

string FieldConstantName(const FieldDescriptor *field) {
  string field_name = UnderscoresToCamelCase(field->name(), true);
  string result = "k" + field_name + "FieldNumber";

  if (!field->is_extension() &&
      field->containing_type()->FindFieldByCamelcaseName(
        field->camelcase_name()) != field) {
    // This field's camelcase name is not unique.  As a hack, add the field
    // number to the constant name.  This makes the constant rather useless,
    // but what can we do?
    result += "_" + SimpleItoa(field->number());
  }

  return result;
}

string FieldMessageTypeName(const FieldDescriptor* field) {
  // Note:  The Google-internal version of Protocol Buffers uses this function
  //   as a hook point for hacks to support legacy code.
  return ClassName(field->message_type(), true);
}

string StripProto(const string& filename) {
  if (HasSuffixString(filename, ".protodevel")) {
    return StripSuffixString(filename, ".protodevel");
  } else {
    return StripSuffixString(filename, ".proto");
  }
}

const char* PrimitiveTypeName(FieldDescriptor::CppType type) {
  switch (type) {
    case FieldDescriptor::CPPTYPE_INT32  : return "::google::protobuf::int32";
    case FieldDescriptor::CPPTYPE_INT64  : return "::google::protobuf::int64";
    case FieldDescriptor::CPPTYPE_UINT32 : return "::google::protobuf::uint32";
    case FieldDescriptor::CPPTYPE_UINT64 : return "::google::protobuf::uint64";
    case FieldDescriptor::CPPTYPE_DOUBLE : return "double";
    case FieldDescriptor::CPPTYPE_FLOAT  : return "float";
    case FieldDescriptor::CPPTYPE_BOOL   : return "bool";
    case FieldDescriptor::CPPTYPE_ENUM   : return "int";
    case FieldDescriptor::CPPTYPE_STRING : return "::std::string";
    case FieldDescriptor::CPPTYPE_MESSAGE: return NULL;

    // No default because we want the compiler to complain if any new
    // CppTypes are added.
  }

  GOOGLE_LOG(FATAL) << "Can't get here.";
  return NULL;
}

string PrimitiveTypeName(const Options& options,
                         FieldDescriptor::CppType type) {
  switch (type) {
    case FieldDescriptor::CPPTYPE_INT32:
      return IntTypeName(options, "int32");
    case FieldDescriptor::CPPTYPE_INT64:
      return IntTypeName(options, "int64");
    case FieldDescriptor::CPPTYPE_UINT32:
      return IntTypeName(options, "uint32");
    case FieldDescriptor::CPPTYPE_UINT64:
      return IntTypeName(options, "uint64");
    case FieldDescriptor::CPPTYPE_DOUBLE:
      return "double";
    case FieldDescriptor::CPPTYPE_FLOAT:
      return "float";
    case FieldDescriptor::CPPTYPE_BOOL:
      return "bool";
    case FieldDescriptor::CPPTYPE_ENUM:
      return "int";
    case FieldDescriptor::CPPTYPE_STRING:
      return StringTypeName(options);
    case FieldDescriptor::CPPTYPE_MESSAGE:
      return "";

      // No default because we want the compiler to complain if any new
      // CppTypes are added.
  }

  GOOGLE_LOG(FATAL) << "Can't get here.";
  return "";
}

const char* DeclaredTypeMethodName(FieldDescriptor::Type type) {
  switch (type) {
    case FieldDescriptor::TYPE_INT32   : return "Int32";
    case FieldDescriptor::TYPE_INT64   : return "Int64";
    case FieldDescriptor::TYPE_UINT32  : return "UInt32";
    case FieldDescriptor::TYPE_UINT64  : return "UInt64";
    case FieldDescriptor::TYPE_SINT32  : return "SInt32";
    case FieldDescriptor::TYPE_SINT64  : return "SInt64";
    case FieldDescriptor::TYPE_FIXED32 : return "Fixed32";
    case FieldDescriptor::TYPE_FIXED64 : return "Fixed64";
    case FieldDescriptor::TYPE_SFIXED32: return "SFixed32";
    case FieldDescriptor::TYPE_SFIXED64: return "SFixed64";
    case FieldDescriptor::TYPE_FLOAT   : return "Float";
    case FieldDescriptor::TYPE_DOUBLE  : return "Double";

    case FieldDescriptor::TYPE_BOOL    : return "Bool";
    case FieldDescriptor::TYPE_ENUM    : return "Enum";

    case FieldDescriptor::TYPE_STRING  : return "String";
    case FieldDescriptor::TYPE_BYTES   : return "Bytes";
    case FieldDescriptor::TYPE_GROUP   : return "Group";
    case FieldDescriptor::TYPE_MESSAGE : return "Message";

    // No default because we want the compiler to complain if any new
    // types are added.
  }
  GOOGLE_LOG(FATAL) << "Can't get here.";
  return "";
}

string Int32ToString(int number) {
  if (number == kint32min) {
    // This needs to be special-cased, see explanation here:
    // https://gcc.gnu.org/bugzilla/show_bug.cgi?id=52661
    return SimpleItoa(number + 1) + " - 1";
  } else {
    return SimpleItoa(number);
  }
}

string Int64ToString(const string& macro_prefix, int64 number) {
  if (number == kint64min) {
    // This needs to be special-cased, see explanation here:
    // https://gcc.gnu.org/bugzilla/show_bug.cgi?id=52661
    return macro_prefix + "_LONGLONG(" + SimpleItoa(number + 1) +
           ") - 1";
  }
  return macro_prefix + "_LONGLONG(" + SimpleItoa(number) + ")";
}

string UInt64ToString(const string& macro_prefix, uint64 number) {
  return macro_prefix + "_ULONGLONG(" + SimpleItoa(number) + ")";
}

string DefaultValue(const FieldDescriptor* field) {
  switch (field->cpp_type()) {
    case FieldDescriptor::CPPTYPE_INT64:
      return Int64ToString("GG", field->default_value_int64());
    case FieldDescriptor::CPPTYPE_UINT64:
      return UInt64ToString("GG", field->default_value_uint64());
    default:
      return DefaultValue(Options(), field);
  }
}

string DefaultValue(const Options& options, const FieldDescriptor* field) {
  switch (field->cpp_type()) {
    case FieldDescriptor::CPPTYPE_INT32:
      return Int32ToString(field->default_value_int32());
    case FieldDescriptor::CPPTYPE_UINT32:
      return SimpleItoa(field->default_value_uint32()) + "u";
    case FieldDescriptor::CPPTYPE_INT64:
      return Int64ToString("PROTOBUF", field->default_value_int64());
    case FieldDescriptor::CPPTYPE_UINT64:
      return UInt64ToString("PROTOBUF", field->default_value_uint64());
    case FieldDescriptor::CPPTYPE_DOUBLE: {
      double value = field->default_value_double();
      if (value == std::numeric_limits<double>::infinity()) {
        return "std::numeric_limits<double>::infinity()";
      } else if (value == -std::numeric_limits<double>::infinity()) {
        return "-std::numeric_limits<double>::infinity()";
      } else if (value != value) {
        return "std::numeric_limits<double>::quiet_NaN()";
      } else {
        return SimpleDtoa(value);
      }
    }
    case FieldDescriptor::CPPTYPE_FLOAT:
      {
        float value = field->default_value_float();
        if (value == std::numeric_limits<float>::infinity()) {
          return "std::numeric_limits<float>::infinity()";
        } else if (value == -std::numeric_limits<float>::infinity()) {
          return "-std::numeric_limits<float>::infinity()";
        } else if (value != value) {
          return "std::numeric_limits<float>::quiet_NaN()";
        } else {
          string float_value = SimpleFtoa(value);
          // If floating point value contains a period (.) or an exponent
          // (either E or e), then append suffix 'f' to make it a float
          // literal.
          if (float_value.find_first_of(".eE") != string::npos) {
            float_value.push_back('f');
          }
          return float_value;
        }
      }
    case FieldDescriptor::CPPTYPE_BOOL:
      return field->default_value_bool() ? "true" : "false";
    case FieldDescriptor::CPPTYPE_ENUM:
      // Lazy:  Generate a static_cast because we don't have a helper function
      //   that constructs the full name of an enum value.
      return strings::Substitute(
          "static_cast< $0 >($1)",
          ClassName(field->enum_type(), true),
          Int32ToString(field->default_value_enum()->number()));
    case FieldDescriptor::CPPTYPE_STRING:
      return "\"" + EscapeTrigraphs(
        CEscape(field->default_value_string())) +
        "\"";
    case FieldDescriptor::CPPTYPE_MESSAGE:
      return "*" + FieldMessageTypeName(field) +
             "::internal_default_instance()";
  }
  // Can't actually get here; make compiler happy.  (We could add a default
  // case above but then we wouldn't get the nice compiler warning when a
  // new type is added.)
  GOOGLE_LOG(FATAL) << "Can't get here.";
  return "";
}

// Convert a file name into a valid identifier.
string FilenameIdentifier(const string& filename) {
  string result;
  for (int i = 0; i < filename.size(); i++) {
    if (ascii_isalnum(filename[i])) {
      result.push_back(filename[i]);
    } else {
      // Not alphanumeric.  To avoid any possibility of name conflicts we
      // use the hex code for the character.
      StrAppend(&result, "_", strings::Hex(static_cast<uint8>(filename[i])));
    }
  }
  return result;
}

string UniqueName(const string& name, const string& filename,
                  const Options& options) {
  return name + "_" + FilenameIdentifier(filename);
}

// Return the qualified C++ name for a file level symbol.
string QualifiedFileLevelSymbol(const string& package, const string& name) {
  if (package.empty()) {
    return StrCat("::", name);
  }
  return StrCat("::", DotsToColons(package), "::", name);
}

// Escape C++ trigraphs by escaping question marks to \?
string EscapeTrigraphs(const string& to_escape) {
  return StringReplace(to_escape, "?", "\\?", true);
}

// Escaped function name to eliminate naming conflict.
string SafeFunctionName(const Descriptor* descriptor,
                        const FieldDescriptor* field,
                        const string& prefix) {
  // Do not use FieldName() since it will escape keywords.
  string name = field->name();
  LowerString(&name);
  string function_name = prefix + name;
  if (descriptor->FindFieldByName(function_name)) {
    // Single underscore will also make it conflicting with the private data
    // member. We use double underscore to escape function names.
    function_name.append("__");
  } else if (kKeywords.count(name) > 0) {
    // If the field name is a keyword, we append the underscore back to keep it
    // consistent with other function names.
    function_name.append("_");
  }
  return function_name;
}

static bool HasLazyFields(const Descriptor* descriptor,
                          const Options& options) {
  for (int field_idx = 0; field_idx < descriptor->field_count(); field_idx++) {
    if (IsLazy(descriptor->field(field_idx), options)) {
      return true;
    }
  }
  for (int idx = 0; idx < descriptor->extension_count(); idx++) {
    if (IsLazy(descriptor->extension(idx), options)) {
      return true;
    }
  }
  for (int idx = 0; idx < descriptor->nested_type_count(); idx++) {
    if (HasLazyFields(descriptor->nested_type(idx), options)) {
      return true;
    }
  }
  return false;
}

// Does the given FileDescriptor use lazy fields?
bool HasLazyFields(const FileDescriptor* file, const Options& options) {
  for (int i = 0; i < file->message_type_count(); i++) {
    const Descriptor* descriptor(file->message_type(i));
    if (HasLazyFields(descriptor, options)) {
      return true;
    }
  }
  for (int field_idx = 0; field_idx < file->extension_count(); field_idx++) {
    if (IsLazy(file->extension(field_idx), options)) {
      return true;
    }
  }
  return false;
}

static bool HasRepeatedFields(const Descriptor* descriptor) {
  for (int i = 0; i < descriptor->field_count(); ++i) {
    if (descriptor->field(i)->label() == FieldDescriptor::LABEL_REPEATED) {
      return true;
    }
  }
  for (int i = 0; i < descriptor->nested_type_count(); ++i) {
    if (HasRepeatedFields(descriptor->nested_type(i))) return true;
  }
  return false;
}

bool HasRepeatedFields(const FileDescriptor* file) {
  for (int i = 0; i < file->message_type_count(); ++i) {
    if (HasRepeatedFields(file->message_type(i))) return true;
  }
  return false;
}

static bool IsStringPieceField(const FieldDescriptor* field,
                               const Options& options) {
  return field->cpp_type() == FieldDescriptor::CPPTYPE_STRING &&
         EffectiveStringCType(field, options) == FieldOptions::STRING_PIECE;
}

static bool HasStringPieceFields(const Descriptor* descriptor,
                                 const Options& options) {
  for (int i = 0; i < descriptor->field_count(); ++i) {
    if (IsStringPieceField(descriptor->field(i), options)) return true;
  }
  for (int i = 0; i < descriptor->nested_type_count(); ++i) {
    if (HasStringPieceFields(descriptor->nested_type(i), options)) return true;
  }
  return false;
}

bool HasStringPieceFields(const FileDescriptor* file, const Options& options) {
  for (int i = 0; i < file->message_type_count(); ++i) {
    if (HasStringPieceFields(file->message_type(i), options)) return true;
  }
  return false;
}

static bool IsCordField(const FieldDescriptor* field, const Options& options) {
  return field->cpp_type() == FieldDescriptor::CPPTYPE_STRING &&
         EffectiveStringCType(field, options) == FieldOptions::CORD;
}

static bool HasCordFields(const Descriptor* descriptor,
                          const Options& options) {
  for (int i = 0; i < descriptor->field_count(); ++i) {
    if (IsCordField(descriptor->field(i), options)) return true;
  }
  for (int i = 0; i < descriptor->nested_type_count(); ++i) {
    if (HasCordFields(descriptor->nested_type(i), options)) return true;
  }
  return false;
}

bool HasCordFields(const FileDescriptor* file, const Options& options) {
  for (int i = 0; i < file->message_type_count(); ++i) {
    if (HasCordFields(file->message_type(i), options)) return true;
  }
  return false;
}

static bool HasExtensionsOrExtendableMessage(const Descriptor* descriptor) {
  if (descriptor->extension_range_count() > 0) return true;
  if (descriptor->extension_count() > 0) return true;
  for (int i = 0; i < descriptor->nested_type_count(); ++i) {
    if (HasExtensionsOrExtendableMessage(descriptor->nested_type(i))) {
      return true;
    }
  }
  return false;
}

bool HasExtensionsOrExtendableMessage(const FileDescriptor* file) {
  if (file->extension_count() > 0) return true;
  for (int i = 0; i < file->message_type_count(); ++i) {
    if (HasExtensionsOrExtendableMessage(file->message_type(i))) return true;
  }
  return false;
}

static bool HasMapFields(const Descriptor* descriptor) {
  for (int i = 0; i < descriptor->field_count(); ++i) {
    if (descriptor->field(i)->is_map()) {
      return true;
    }
  }
  for (int i = 0; i < descriptor->nested_type_count(); ++i) {
    if (HasMapFields(descriptor->nested_type(i))) return true;
  }
  return false;
}

bool HasMapFields(const FileDescriptor* file) {
  for (int i = 0; i < file->message_type_count(); ++i) {
    if (HasMapFields(file->message_type(i))) return true;
  }
  return false;
}

static bool HasEnumDefinitions(const Descriptor* message_type) {
  if (message_type->enum_type_count() > 0) return true;
  for (int i = 0; i < message_type->nested_type_count(); ++i) {
    if (HasEnumDefinitions(message_type->nested_type(i))) return true;
  }
  return false;
}

bool HasEnumDefinitions(const FileDescriptor* file) {
  if (file->enum_type_count() > 0) return true;
  for (int i = 0; i < file->message_type_count(); ++i) {
    if (HasEnumDefinitions(file->message_type(i))) return true;
  }
  return false;
}

bool IsStringOrMessage(const FieldDescriptor* field) {
  switch (field->cpp_type()) {
    case FieldDescriptor::CPPTYPE_INT32:
    case FieldDescriptor::CPPTYPE_INT64:
    case FieldDescriptor::CPPTYPE_UINT32:
    case FieldDescriptor::CPPTYPE_UINT64:
    case FieldDescriptor::CPPTYPE_DOUBLE:
    case FieldDescriptor::CPPTYPE_FLOAT:
    case FieldDescriptor::CPPTYPE_BOOL:
    case FieldDescriptor::CPPTYPE_ENUM:
      return false;
    case FieldDescriptor::CPPTYPE_STRING:
    case FieldDescriptor::CPPTYPE_MESSAGE:
      return true;
  }

  GOOGLE_LOG(FATAL) << "Can't get here.";
  return false;
}

FieldOptions::CType EffectiveStringCType(const FieldDescriptor* field,
                                         const Options& options) {
  GOOGLE_DCHECK(field->cpp_type() == FieldDescriptor::CPPTYPE_STRING);
  if (options.opensource_runtime) {
    // Open-source protobuf release only supports STRING ctype.
    return FieldOptions::STRING;
  } else {
    // Google-internal supports all ctypes.
    return field->options().ctype();
  }
}

bool IsAnyMessage(const FileDescriptor* descriptor, const Options& options) {
  // For now we do not support Any in lite mode, so if we're building for lite
  // then we just treat Any as if it's an ordinary message with no special
  // behavior.
  return descriptor->name() == kAnyProtoFile &&
         GetOptimizeFor(descriptor, options) != FileOptions::LITE_RUNTIME;
}

bool IsAnyMessage(const Descriptor* descriptor, const Options& options) {
  return descriptor->name() == kAnyMessageName &&
         IsAnyMessage(descriptor->file(), options);
}

bool IsWellKnownMessage(const FileDescriptor* descriptor) {
  return !descriptor->name().compare(0, 16, kGoogleProtobufPrefix);
}

enum Utf8CheckMode {
  STRICT = 0,  // Parsing will fail if non UTF-8 data is in string fields.
  VERIFY = 1,  // Only log an error but parsing will succeed.
  NONE = 2,  // No UTF-8 check.
};

static bool FieldEnforceUtf8(const FieldDescriptor* field,
                             const Options& options) {
  return true;
}

static bool FileUtf8Verification(const FileDescriptor* file,
                                 const Options& options) {
  return true;
}

// Which level of UTF-8 enforcemant is placed on this file.
static Utf8CheckMode GetUtf8CheckMode(const FieldDescriptor* field,
                                      const Options& options) {
  if (field->file()->syntax() == FileDescriptor::SYNTAX_PROTO3 &&
      FieldEnforceUtf8(field, options)) {
    return STRICT;
  } else if (GetOptimizeFor(field->file(), options) !=
                 FileOptions::LITE_RUNTIME &&
             FileUtf8Verification(field->file(), options)) {
    return VERIFY;
  } else {
    return NONE;
  }
}

string GetUtf8Suffix(const FieldDescriptor* field, const Options& options) {
  switch (GetUtf8CheckMode(field, options)) {
    case STRICT:
      return "UTF8";
    case VERIFY:
      return "UTF8Verify";
    case NONE:
    default:  // Some build configs warn on missing return without default.
      return "";
  }
}

static void GenerateUtf8CheckCode(const FieldDescriptor* field,
                                  const Options& options, bool for_parse,
                                  const char* parameters,
                                  const char* strict_function,
                                  const char* verify_function,
                                  const Formatter& format) {
  switch (GetUtf8CheckMode(field, options)) {
    case STRICT: {
      if (for_parse) {
        format("DO_(");
      }
      format("::$proto_ns$::internal::WireFormatLite::$1$(\n", strict_function);
      format.Indent();
      format(parameters);
      if (for_parse) {
        format("::$proto_ns$::internal::WireFormatLite::PARSE,\n");
      } else {
        format("::$proto_ns$::internal::WireFormatLite::SERIALIZE,\n");
      }
      format("\"$1$\")", field->full_name());
      if (for_parse) {
        format(")");
      }
      format(";\n");
      format.Outdent();
      break;
    }
    case VERIFY: {
      format("::$proto_ns$::internal::WireFormat::$1$(\n", verify_function);
      format.Indent();
      format(parameters);
      if (for_parse) {
        format("::$proto_ns$::internal::WireFormat::PARSE,\n");
      } else {
        format("::$proto_ns$::internal::WireFormat::SERIALIZE,\n");
      }
      format("\"$1$\");\n", field->full_name());
      format.Outdent();
      break;
    }
    case NONE:
      break;
  }
}

void GenerateUtf8CheckCodeForString(const FieldDescriptor* field,
                                    const Options& options, bool for_parse,
                                    const char* parameters,
                                    const Formatter& format) {
  GenerateUtf8CheckCode(field, options, for_parse, parameters,
                        "VerifyUtf8String", "VerifyUTF8StringNamedField",
                        format);
}

void GenerateUtf8CheckCodeForCord(const FieldDescriptor* field,
                                  const Options& options, bool for_parse,
                                  const char* parameters,
                                  const Formatter& format) {
  GenerateUtf8CheckCode(field, options, for_parse, parameters, "VerifyUtf8Cord",
                        "VerifyUTF8CordNamedField", format);
}

namespace {

void Flatten(const Descriptor* descriptor,
             std::vector<const Descriptor*>* flatten) {
  for (int i = 0; i < descriptor->nested_type_count(); i++)
    Flatten(descriptor->nested_type(i), flatten);
  flatten->push_back(descriptor);
}

}  // namespace

void FlattenMessagesInFile(const FileDescriptor* file,
                           std::vector<const Descriptor*>* result) {
  for (int i = 0; i < file->message_type_count(); i++) {
    Flatten(file->message_type(i), result);
  }
}

bool HasWeakFields(const Descriptor* descriptor, const Options& options) {
  for (int i = 0; i < descriptor->field_count(); i++) {
    if (IsWeak(descriptor->field(i), options)) return true;
  }
  return false;
}

bool HasWeakFields(const FileDescriptor* file, const Options& options) {
  for (int i = 0; i < file->message_type_count(); ++i) {
    if (HasWeakFields(file->message_type(i), options)) return true;
  }
  return false;
}

bool UsingImplicitWeakFields(const FileDescriptor* file,
                             const Options& options) {
  return options.lite_implicit_weak_fields &&
         GetOptimizeFor(file, options) == FileOptions::LITE_RUNTIME;
}

bool IsImplicitWeakField(const FieldDescriptor* field, const Options& options,
                         MessageSCCAnalyzer* scc_analyzer) {
  return UsingImplicitWeakFields(field->file(), options) &&
         field->type() == FieldDescriptor::TYPE_MESSAGE &&
         !field->is_required() && !field->is_map() &&
         field->containing_oneof() == NULL &&
         !IsWellKnownMessage(field->message_type()->file()) &&
         // We do not support implicit weak fields between messages in the same
         // strongly-connected component.
         scc_analyzer->GetSCC(field->containing_type()) !=
             scc_analyzer->GetSCC(field->message_type());
}

MessageAnalysis MessageSCCAnalyzer::GetSCCAnalysis(const SCC* scc) {
  if (analysis_cache_.count(scc)) return analysis_cache_[scc];
  MessageAnalysis result{};
  for (int i = 0; i < scc->descriptors.size(); i++) {
    const Descriptor* descriptor = scc->descriptors[i];
    if (descriptor->extension_range_count() > 0) {
      result.contains_extension = true;
      // Extensions are found by looking up default_instance and extension
      // number in a map. So you'd maybe expect here
      // result.constructor_requires_initialization = true;
      // However the extension registration mechanism already makes sure
      // the default will be initialized.
    }
    for (int i = 0; i < descriptor->field_count(); i++) {
      const FieldDescriptor* field = descriptor->field(i);
      if (field->is_required()) {
        result.contains_required = true;
      }
      switch (field->type()) {
        case FieldDescriptor::TYPE_STRING:
        case FieldDescriptor::TYPE_BYTES: {
          result.constructor_requires_initialization = true;
          if (field->options().ctype() == FieldOptions::CORD) {
            result.contains_cord = true;
          }
          break;
        }
        case FieldDescriptor::TYPE_GROUP:
        case FieldDescriptor::TYPE_MESSAGE: {
          result.constructor_requires_initialization = true;
          const SCC* child = analyzer_.GetSCC(field->message_type());
          if (child != scc) {
            MessageAnalysis analysis = GetSCCAnalysis(child);
            result.contains_cord |= analysis.contains_cord;
            result.contains_extension |= analysis.contains_extension;
            if (!ShouldIgnoreRequiredFieldCheck(field, options_)) {
              result.contains_required |= analysis.contains_required;
            }
          } else {
            // This field points back into the same SCC hence the messages
            // in the SCC are recursive. Note if SCC contains more than two
            // nodes it has to be recursive, however this test also works for
            // a single node that is recursive.
            result.is_recursive = true;
          }
          break;
        }
        default:
          break;
      }
    }
  }
  // We deliberately only insert the result here. After we contracted the SCC
  // in the graph, the graph should be a DAG. Hence we shouldn't need to mark
  // nodes visited as we can never return to them. By inserting them here
  // we will go in an infinite loop if the SCC is not correct.
  return analysis_cache_[scc] = result;
}

void ListAllFields(const Descriptor* d,
                   std::vector<const FieldDescriptor*>* fields) {
  // Collect sub messages
  for (int i = 0; i < d->nested_type_count(); i++) {
    ListAllFields(d->nested_type(i), fields);
  }
  // Collect message level extensions.
  for (int i = 0; i < d->extension_count(); i++) {
    fields->push_back(d->extension(i));
  }
  // Add types of fields necessary
  for (int i = 0; i < d->field_count(); i++) {
    fields->push_back(d->field(i));
  }
}

void ListAllFields(const FileDescriptor* d,
                   std::vector<const FieldDescriptor*>* fields) {
  // Collect file level message.
  for (int i = 0; i < d->message_type_count(); i++) {
    ListAllFields(d->message_type(i), fields);
  }
  // Collect message level extensions.
  for (int i = 0; i < d->extension_count(); i++) {
    fields->push_back(d->extension(i));
  }
}

void ListAllTypesForServices(const FileDescriptor* fd,
                             std::vector<const Descriptor*>* types) {
  for (int i = 0; i < fd->service_count(); i++) {
    const ServiceDescriptor* sd = fd->service(i);
    for (int j = 0; j < sd->method_count(); j++) {
      const MethodDescriptor* method = sd->method(j);
      types->push_back(method->input_type());
      types->push_back(method->output_type());
    }
  }
}

bool GetBootstrapBasename(const Options& options, const string& basename,
                          string* bootstrap_basename) {
  if (options.opensource_runtime || options.lite_implicit_weak_fields) {
    return false;
  }

  std::unordered_map<string, string> bootstrap_mapping{
      {"net/proto2/proto/descriptor",
       "net/proto2/internal/descriptor"},
      {"net/proto2/compiler/proto/plugin",
       "net/proto2/compiler/proto/plugin"},
      {"net/proto2/compiler/proto/profile",
       "net/proto2/compiler/proto/profile_bootstrap"},
  };
  auto iter = bootstrap_mapping.find(basename);
  if (iter == bootstrap_mapping.end()) {
    *bootstrap_basename = basename;
    return false;
  } else {
    *bootstrap_basename = iter->second;
    return true;
  }
}

bool IsBootstrapProto(const Options& options, const FileDescriptor* file) {
  string my_name = StripProto(file->name());
  return GetBootstrapBasename(options, my_name, &my_name);
}

bool MaybeBootstrap(const Options& options, GeneratorContext* generator_context,
                    bool bootstrap_flag, string* basename) {
  string bootstrap_basename;
  if (!GetBootstrapBasename(options, *basename, &bootstrap_basename)) {
    return false;
  }

  if (bootstrap_flag) {
    // Adjust basename, but don't abort code generation.
    *basename = bootstrap_basename;
    return false;
  } else {
    string forward_to_basename = bootstrap_basename;

    // Generate forwarding headers and empty .pb.cc.
    {
      std::unique_ptr<io::ZeroCopyOutputStream> output(
          generator_context->Open(*basename + ".pb.h"));
      io::Printer printer(output.get(), '$', nullptr);
      printer.Print(
          "#ifndef PROTOBUF_INCLUDED_$filename_identifier$_FORWARD_PB_H\n"
          "#define PROTOBUF_INCLUDED_$filename_identifier$_FORWARD_PB_H\n"
          "#include \"$forward_to_basename$.pb.h\"  // IWYU pragma: export\n"
          "#endif  // PROTOBUF_INCLUDED_$filename_identifier$_FORWARD_PB_H\n",
          "forward_to_basename", forward_to_basename,
          "filename_identifier", FilenameIdentifier(*basename));

      if (!options.opensource_runtime) {
        // HACK HACK HACK, tech debt from the deeps of proto1 and SWIG
        // protocoltype is SWIG'ed and we need to forward
        if (*basename == "net/proto/protocoltype") {
          printer.Print(
              "#ifdef SWIG\n"
              "%include \"$forward_to_basename$.pb.h\"\n"
              "#endif  // SWIG\n",
              "forward_to_basename", forward_to_basename);
        }
      }
    }

    {
      std::unique_ptr<io::ZeroCopyOutputStream> output(
          generator_context->Open(*basename + ".proto.h"));
      io::Printer printer(output.get(), '$', nullptr);
      printer.Print(
          "#ifndef PROTOBUF_INCLUDED_$filename_identifier$_FORWARD_PROTO_H\n"
          "#define PROTOBUF_INCLUDED_$filename_identifier$_FORWARD_PROTO_H\n"
          "#include \"$forward_to_basename$.proto.h\"  // IWYU pragma: "
          "export\n"
          "#endif  // "
          "PROTOBUF_INCLUDED_$filename_identifier$_FORWARD_PROTO_H\n",
          "forward_to_basename", forward_to_basename,
          "filename_identifier", FilenameIdentifier(*basename));
    }

    {
      std::unique_ptr<io::ZeroCopyOutputStream> output(
          generator_context->Open(*basename + ".pb.cc"));
      io::Printer printer(output.get(), '$', nullptr);
      printer.Print("\n");
    }

    {
      std::unique_ptr<io::ZeroCopyOutputStream> output(
          generator_context->Open(*basename + ".pb.h.meta"));
    }

    {
      std::unique_ptr<io::ZeroCopyOutputStream> output(
          generator_context->Open(*basename + ".proto.h.meta"));
    }

    // Abort code generation.
    return true;
  }
}

bool ShouldRepeat(const FieldDescriptor* descriptor,
                  internal::WireFormatLite::WireType wiretype) {
  return descriptor->is_repeated() &&
         (!descriptor->is_packable() ||
          wiretype != internal::WireFormatLite::WIRETYPE_LENGTH_DELIMITED);
}

void GenerateStrings(const FieldDescriptor* field, const Options& options,
                     const Formatter& format, bool check_utf8) {
  string utf8;
  if (check_utf8) {
    utf8 = GetUtf8Suffix(field, options);
    if (!utf8.empty()) {
      string name = "nullptr";
      if (HasDescriptorMethods(field->file(), options)) {
        name = "\"" + field->full_name() + "\"";
      }
      format("ctx->extra_parse_data().SetFieldName($1$);\n", name);
    }
  }
  format(
      "auto str = msg->$1$_$2$();\n"
      "if (size > end - ptr + "
      "::$proto_ns$::internal::ParseContext::kSlopBytes) {\n"
      "  object = str;\n",
      field->is_repeated() && !field->is_packable() ? "add" : "mutable",
      FieldName(field));
  string name;
  if (field->options().ctype() == FieldOptions::STRING ||
      (IsProto1(field->file(), options) &&
       field->options().ctype() == FieldOptions::STRING_PIECE)) {
    name = "GreedyStringParser";
    format("  str->clear();\n");
    // TODO(gerbens) evaluate security
    format("  str->reserve(size);\n");
  } else if (field->options().ctype() == FieldOptions::CORD) {
    name = "CordParser";
    format("  str->Clear();\n");
  } else if (field->options().ctype() == FieldOptions::STRING_PIECE) {
    name = "StringPieceParser";
    format("  str->Clear();\n");
  }
  format(
      "  parser_till_end = ::$proto_ns$::internal::$1$$2$;\n"
      "  goto len_delim_till_end;\n"
      "}\n"
      "$GOOGLE_PROTOBUF$_PARSER_ASSERT(::$proto_ns$::internal::StringCheck$2$("
      "ptr, size, ctx));\n"
      "::$proto_ns$::internal::Inline$1$(str, ptr, size, ctx);\n"
      "ptr += size;\n",
      name, utf8);
}

void GenerateLengthDelim(
                      const FieldDescriptor* field, const Options& options,
                      MessageSCCAnalyzer* scc_analyzer,
                      const Formatter& format) {
  format(
      "ptr = Varint::Parse32Inline(ptr, &size);\n"
      "$GOOGLE_PROTOBUF$_PARSER_ASSERT(ptr);\n");
  if (!IsProto1(field->file(), options) && field->is_packable()) {
    if (!HasPreservingUnknownEnumSemantics(field->file()) &&
        field->type() == FieldDescriptor::TYPE_ENUM) {
      format(
          "ctx->extra_parse_data().SetEnumValidator($1$_IsValid, "
          "msg->mutable_unknown_fields(), $2$);\n"
          "parser_till_end = "
          "::$proto_ns$::internal::PackedValidEnumParser$3$;\n"
          "object = msg->mutable_$4$();\n",
          QualifiedClassName(field->enum_type()), field->number(),
          UseUnknownFieldSet(field->file(), options) ? "" : "Lite",
          FieldName(field));
    } else {
      format(
          "parser_till_end = ::$proto_ns$::internal::Packed$1$Parser;\n"
          "object = msg->mutable_$2$();\n",
          DeclaredTypeMethodName(field->type()), FieldName(field));
    }
    format(
        "if (size > end - ptr) goto len_delim_till_end;\n"
        "auto newend = ptr + size;\n"
        "if (size) ptr = parser_till_end(ptr, newend, object, ctx);\n"
        "$GOOGLE_PROTOBUF$_PARSER_ASSERT(ptr == newend);\n");
  } else {
    auto field_type = field->type();
    if (IsProto1(field->file(), options)) {
      if (field->is_packable()) {
        // Sigh ... packed fields endup as a string in proto1
        field_type = FieldDescriptor::TYPE_BYTES;
      }
      if (field_type == FieldDescriptor::TYPE_STRING) {
        // In proto1 strings are treated as bytes
        field_type = FieldDescriptor::TYPE_BYTES;
      }
    }
    switch (field_type) {
      case FieldDescriptor::TYPE_STRING:
        GenerateStrings(field, options, format, true /* utf8 */);
        break;
      case FieldDescriptor::TYPE_BYTES:
        GenerateStrings(field, options, format, false /* utf8 */);
        break;
      case FieldDescriptor::TYPE_MESSAGE: {
        GOOGLE_CHECK(field->message_type());
        if (!IsProto1(field->file(), options) && field->is_map()) {
          const FieldDescriptor* val =
              field->message_type()->FindFieldByName("value");
          GOOGLE_CHECK(val);
          if (HasFieldPresence(field->file()) &&
              val->type() == FieldDescriptor::TYPE_ENUM) {
            format(
                "ctx->extra_parse_data().field_number = $1$;\n"
                "ctx->extra_parse_data().unknown_fields = "
                "&msg->_internal_metadata_;\n",
                field->number());
          }
          format(
              "parser_till_end = ::$proto_ns$::internal::SlowMapEntryParser;\n"
              "auto parse_map = $1$::_ParseMap;\n"
              "ctx->extra_parse_data().payload.clear();\n"
              "ctx->extra_parse_data().parse_map = parse_map;\n"
              "object = &msg->$2$_;\n"
              "if (size > end - ptr) goto len_delim_till_end;\n"
              "auto newend = ptr + size;\n"
              "GOOGLE_PROTOBUF_PARSER_ASSERT(parse_map(ptr, newend, "
              "object, ctx));\n"
              "ptr = newend;\n",
              QualifiedClassName(field->message_type()), FieldName(field));
          break;
        }
        if (!IsProto1(field->file(), options) && IsLazy(field, options)) {
          if (field->containing_oneof() != nullptr) {
            format(
                "if (!msg->has_$1$()) {\n"
                "  msg->clear_$1$();\n"
                "  msg->$2$_.$1$_ = ::google::protobuf::Arena::CreateMessage<\n"
                "      ::google::protobuf::internal::LazyField>(msg->GetArenaNoVirtual());\n"
                "  msg->set_has_$1$();\n"
                "}\n"
                "auto parse_closure = msg->$2$_.$1$_->_ParseClosure();\n",
                FieldName(field), field->containing_oneof()->name());
          } else if (HasFieldPresence(field->file())) {
            format(
                "HasBitSetters::set_has_$1$(msg);\n"
                "auto parse_closure = msg->$1$_._ParseClosure();\n",
                FieldName(field));
          } else {
            format(
                "auto parse_closure = msg->$1$_._ParseClosure();\n",
                FieldName(field));
          }
          format(
              "parser_till_end = parse_closure.func;\n"
              "object = parse_closure.object;\n"
              "if (size > end - ptr) goto len_delim_till_end;\n"
              "auto newend = ptr + size;\n"
              "GOOGLE_PROTOBUF_PARSER_ASSERT(ctx->ParseExactRange(parse_closure, ptr, newend));\n"
              "ptr = newend;\n");
          break;
        }
        if (IsImplicitWeakField(field, options, scc_analyzer)) {
          if (!field->is_repeated()) {
            format("object = HasBitSetters::mutable_$1$(msg);\n",
                   FieldName(field));
          } else {
            format(
                "object = "
                "CastToBase(&msg->$1$_)->AddWeak(reinterpret_cast<const "
                "::google::protobuf::MessageLite*>(&$2$::_$3$_default_instance_));\n",
                FieldName(field), Namespace(field->message_type()),
                ClassName(field->message_type()));
          }
          format(
              "parser_till_end = static_cast<::$proto_ns$::MessageLite*>("
              "object)->_ParseFunc();\n");
        } else if (IsWeak(field, options)) {
          if (IsProto1(field->file(), options)) {
            format("object = msg->internal_mutable_$1$();\n",
                   FieldName(field));
          } else {
            format(
                "object = msg->_weak_field_map_.MutableMessage($1$, "
                "_$classname$_default_instance_.$2$_);\n",
                field->number(), FieldName(field));
          }
          format(
              "parser_till_end = static_cast<::$proto_ns$::MessageLite*>("
              "object)->_ParseFunc();\n");
        } else {
          format(
              "parser_till_end = $1$::_InternalParse;\n"
              "object = msg->$2$_$3$();\n",
              QualifiedClassName(field->message_type()),
              field->is_repeated() ? "add" : "mutable", FieldName(field));
        }
        format(
            "if (size > end - ptr) goto len_delim_till_end;\n"
            "auto newend = ptr + size;\n"
            "bool ok = ctx->ParseExactRange({parser_till_end, object},\n"
            "                               ptr, newend);\n"
            "$GOOGLE_PROTOBUF$_PARSER_ASSERT(ok);\n"
            "ptr = newend;\n");
        break;
      }
      default:
        GOOGLE_LOG(FATAL) << "Illegal combination for length delimited wiretype "
                   << " filed type is " << field->type();
    }
  }
}

void GenerateCaseBody(internal::WireFormatLite::WireType wiretype,
                      const FieldDescriptor* field, const Options& options,
                      MessageSCCAnalyzer* scc_analyzer,
                      const Formatter& format) {
  using internal::WireFormat;
  using internal::WireFormatLite;

  if (ShouldRepeat(field, wiretype)) {
    format("do {\n");
    format.Indent();
  }
  switch (wiretype) {
    case WireFormatLite::WIRETYPE_VARINT: {
      format(
          "$uint64$ val;\n"
          "ptr = Varint::Parse64(ptr, &val);\n"
          "$GOOGLE_PROTOBUF$_PARSER_ASSERT(ptr);\n");
      string type = PrimitiveTypeName(options, field->cpp_type());
      if ((field->type() == FieldDescriptor::TYPE_SINT32 ||
           field->type() == FieldDescriptor::TYPE_SINT64) &&
          !IsProto1(field->file(), options)) {
        int size = EstimateAlignmentSize(field) * 8;
        format(
            "$1$ value = "
            "::$proto_ns$::internal::WireFormatLite::ZigZagDecode$2$(val);\n",
            type, size);
      } else if (field->type() == FieldDescriptor::TYPE_ENUM &&
                 !IsProto1(field->file(), options)) {
        if (!HasPreservingUnknownEnumSemantics(field->file())) {
          format(
              "if (!$1$_IsValid(val)) {\n"
              "  ::$proto_ns$::internal::WriteVarint($2$, val, "
              "msg->mutable_unknown_fields());\n"
              "  break;\n"
              "}\n",
              QualifiedClassName(field->enum_type()), field->number());
        }
        format("$1$ value = static_cast<$1$>(val);\n",
               QualifiedClassName(field->enum_type()));
      } else {
        format("$1$ value = val;\n", type);
      }
      if (field->is_repeated()) {
        format("msg->add_$1$(value);\n", FieldName(field));
      } else {
        format("msg->set_$1$(value);\n", FieldName(field));
      }
      break;
    }
    case WireFormatLite::WIRETYPE_FIXED64: {
      string type = PrimitiveTypeName(options, field->cpp_type());
      format(
          "$1$ val;\n"
          "::std::memcpy(&val, ptr, 8);\n"
          "ptr += 8;\n",
          type);
      if (field->is_repeated()) {
        format("msg->add_$1$(val);\n", FieldName(field));
      } else {
        format("msg->set_$1$(val);\n", FieldName(field));
      }
      break;
    }
    case WireFormatLite::WIRETYPE_LENGTH_DELIMITED: {
      GenerateLengthDelim(field, options, scc_analyzer, format);
      break;
    }
    case WireFormatLite::WIRETYPE_START_GROUP: {
      format(
          "parser_till_end = $1$::_InternalParse;\n"
          "object = msg->$2$_$3$();\n"
          "auto res = ctx->ParseGroup(tag, {parser_till_end, object}, ptr, "
          "end, "
          "&depth);\n"
          "ptr = res.first;\n"
          "$GOOGLE_PROTOBUF$_PARSER_ASSERT(ptr);\n"
          "if (res.second) goto group_continues;\n",
          QualifiedClassName(field->message_type()),
          field->is_repeated() ? "add" : "mutable", FieldName(field));
      break;
    }
    case WireFormatLite::WIRETYPE_END_GROUP: {
      GOOGLE_LOG(FATAL) << "Can't have end group field\n";
      break;
    }
    case WireFormatLite::WIRETYPE_FIXED32: {
      string type = PrimitiveTypeName(options, field->cpp_type());
      format(
          "$1$ val;\n"
          "std::memcpy(&val, ptr, 4);\n"
          "ptr += 4;\n",
          type);
      if (field->is_repeated()) {
        format("msg->add_$1$(val);\n", FieldName(field));
      } else {
        format("msg->set_$1$(val);\n", FieldName(field));
      }
      break;
    }
  }  // switch (wire_type)

  if (ShouldRepeat(field, wiretype)) {
    format("if (ptr >= end) break;\n");
    uint32 x = field->number() * 8 + wiretype;
    uint64 y = 0;
    int cnt = 0;
    do {
      y += static_cast<uint64>((x & 0x7F) + (x >= 128 ? 128 : 0))
           << (cnt++ * 8);
      x >>= 7;
    } while (x);
    uint64 mask = (1ull << (cnt * 8)) - 1;
    format.Outdent();
    format(
        "} while ((::$proto_ns$::io::UnalignedLoad<$uint64$>(ptr) & $1$) == "
        "$2$ && (ptr += $3$));\n",
        mask, y, cnt);
  }
  format("break;\n");
}

void GenerateCaseBody(const FieldDescriptor* field, const Options& options,
                      MessageSCCAnalyzer* scc_analyzer,
                      const Formatter& format) {
  using internal::WireFormat;
  using internal::WireFormatLite;

  if (!IsProto1(field->file(), options) && field->is_packable()) {
    auto expected_wiretype = WireFormat::WireTypeForFieldType(field->type());
    GOOGLE_CHECK(expected_wiretype != WireFormatLite::WIRETYPE_LENGTH_DELIMITED);
    uint32 expected_tag =
        WireFormatLite::MakeTag(field->number(), expected_wiretype);
    auto fallback_wiretype = WireFormatLite::WIRETYPE_LENGTH_DELIMITED;
    uint32 fallback_tag =
        WireFormatLite::MakeTag(field->number(), fallback_wiretype);

    if (field->is_packed()) {
      std::swap(expected_tag, fallback_tag);
      std::swap(expected_wiretype, fallback_wiretype);
    }

    format("if (static_cast<$uint8$>(tag) == $1$) {\n", expected_tag & 0xFF);
    format.Indent();
    GenerateCaseBody(expected_wiretype, field, options, scc_analyzer, format);
    format.Outdent();
    format(
        "} else if (static_cast<$uint8$>(tag) != $1$) goto handle_unusual;\n",
        fallback_tag & 0xFF);
    GenerateCaseBody(fallback_wiretype, field, options, scc_analyzer, format);
  } else {
    auto wiretype = WireFormat::WireTypeForField(field);
    format("if (static_cast<$uint8$>(tag) != $1$) goto handle_unusual;\n",
           WireFormat::MakeTag(field) & 0xFF);
    GenerateCaseBody(wiretype, field, options, scc_analyzer, format);
  }
}

void GenerateParserLoop(const Descriptor* descriptor, const Options& options,
                        MessageSCCAnalyzer* scc_analyzer,
                        io::Printer* printer) {
  using internal::WireFormat;
  using internal::WireFormatLite;

  Formatter format(printer);
  format.Set("classname", ClassName(descriptor));
  format.Set("proto_ns", ProtobufNamespace(options));
  std::map<string, string> vars;
  SetCommonVars(options, &vars);
  format.AddMap(vars);

  std::vector<const FieldDescriptor*> ordered_fields;
  for (auto field : FieldRange(descriptor)) {
    ordered_fields.push_back(field);
  }
  std::sort(ordered_fields.begin(), ordered_fields.end(),
            [](const FieldDescriptor* a, const FieldDescriptor* b) {
              return a->number() < b->number();
            });

  format(
      "const char* $classname$::_InternalParse(const char* begin, const char* "
      "end, void* object,\n"
      "                  ::$proto_ns$::internal::ParseContext* ctx) {\n"
      "  auto msg = static_cast<$classname$*>(object);\n"
      "  $uint32$ size; (void)size;\n"
      "  int depth; (void)depth;\n"
      "  $uint32$ tag;\n"
      "  ::$proto_ns$::internal::ParseFunc parser_till_end; "
      "(void)parser_till_end;\n"
      "  auto ptr = begin;\n"
      "  while (ptr < end) {\n"
      "    ptr = Varint::Parse32Inline(ptr, &tag);\n"
      "    $GOOGLE_PROTOBUF$_PARSER_ASSERT(ptr);\n"
      "    switch (tag >> 3) {\n");

  format.Indent();
  format.Indent();
  format.Indent();

  for (const auto* field : ordered_fields) {
    if (IsProto1(descriptor->file(), options)) {
      if (field->number() >= (1 << 14)) continue;
    }
    // Print the field's (or oneof's) proto-syntax definition as a comment.
    // We don't want to print group bodies so we cut off after the first
    // line.
    string def;
    {
      DebugStringOptions options;
      options.elide_group_body = true;
      options.elide_oneof_body = true;
      def = field->DebugStringWithOptions(options);
      def = def.substr(0, def.find_first_of('\n'));
    }
    format(
        "// $1$\n"
        "case $2$: {\n",
        def, field->number());
    format.Indent();
    GenerateCaseBody(field, options, scc_analyzer, format);
    format.Outdent();
    format("}\n");  // case
  }                 // for fields
  format(
      "default: {\n"
      "handle_unusual: (void)&&handle_unusual;\n"
      "  if ((tag & 7) == 4 || tag == 0) {\n"
      "    ctx->EndGroup(tag);\n"
      "    return ptr;\n"
      "  }\n");
  if (IsMapEntryMessage(descriptor)) {
    format(
        "  break;\n"
        "}\n");
  } else {
    if (descriptor->extension_range_count() > 0) {
      format("if (");
      for (int i = 0; i < descriptor->extension_range_count(); i++) {
        const Descriptor::ExtensionRange* range =
            descriptor->extension_range(i);
        if (i > 0) format(" ||\n    ");

        uint32 start_tag = WireFormatLite::MakeTag(
            range->start, static_cast<WireFormatLite::WireType>(0));
        uint32 end_tag = WireFormatLite::MakeTag(
            range->end, static_cast<WireFormatLite::WireType>(0));

        if (range->end > FieldDescriptor::kMaxNumber) {
          format("($1$u <= tag)", start_tag);
        } else {
          format("($1$u <= tag && tag < $2$u)", start_tag, end_tag);
        }
      }
      format(") {\n");
      format(
          "  auto res = msg->_extensions_.ParseField(tag, {_InternalParse, "
          "msg}, ptr, end,\n"
          "      internal_default_instance(), &msg->_internal_metadata_, "
          "ctx);\n"
          "  ptr = res.first;\n"
          "  $GOOGLE_PROTOBUF$_PARSER_ASSERT(ptr != nullptr);\n"
          "  if (res.second) return ptr;\n"
          "  continue;\n"
          "}\n");
    }
    format(
        "  auto res = UnknownFieldParse(tag, {_InternalParse, msg},\n"
        "    ptr, end, msg->_internal_metadata_.mutable_unknown_fields(), "
        "ctx);\n"
        "  ptr = res.first;\n"
        "  $GOOGLE_PROTOBUF$_PARSER_ASSERT(ptr != nullptr);\n"
        "  if (res.second) return ptr;\n"
        "}\n");  // default case
  }
  format.Outdent();
  format.Outdent();
  format.Outdent();
  format(
      "    }  // switch\n"
      "  }  // while\n"
      "  return ptr;\n"
      "len_delim_till_end: (void)&&len_delim_till_end;\n"
      "  return ctx->StoreAndTailCall(ptr, end, {_InternalParse, msg},\n"
      "                                 {parser_till_end, object}, size);\n"
      "group_continues: (void)&&group_continues;\n"
      "  $DCHK$(ptr >= end);\n"
      // Group crossed end and must be continued. Either this a parse failure
      // or we need to resume on the next chunk and thus save the state.
      "  $GOOGLE_PROTOBUF$_PARSER_ASSERT(ctx->StoreGroup({_InternalParse, msg},"
      " {parser_till_end, object}, depth, tag));\n"
      "  return ptr;\n"
      "}\n");
}

}  // namespace cpp
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
