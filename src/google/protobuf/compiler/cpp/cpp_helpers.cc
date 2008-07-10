// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.
// http://code.google.com/p/protobuf/
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

// Author: kenton@google.com (Kenton Varda)
//  Based on original Protocol Buffers design by
//  Sanjay Ghemawat, Jeff Dean, and others.

#include <vector>
#include <google/protobuf/stubs/hash.h>

#include <google/protobuf/compiler/cpp/cpp_helpers.h>
#include <google/protobuf/stubs/common.h>
#include <google/protobuf/stubs/strutil.h>

namespace google {
namespace protobuf {
namespace compiler {
namespace cpp {

namespace {

string DotsToUnderscores(const string& name) {
  return StringReplace(name, ".", "_", true);
}

string DotsToColons(const string& name) {
  return StringReplace(name, ".", "::", true);
}

const char* const kKeywordList[] = {
  "and", "and_eq", "asm", "auto", "bitand", "bitor", "bool", "break", "case",
  "catch", "char", "class", "compl", "const", "const_cast", "continue",
  "default", "delete", "do", "double", "dynamic_cast", "else", "enum",
  "explicit", "extern", "false", "float", "for", "friend", "goto", "if",
  "inline", "int", "long", "mutable", "namespace", "new", "not", "not_eq",
  "operator", "or", "or_eq", "private", "protected", "public", "register",
  "reinterpret_cast", "return", "short", "signed", "sizeof", "static",
  "static_cast", "struct", "switch", "template", "this", "throw", "true", "try",
  "typedef", "typeid", "typename", "union", "unsigned", "using", "virtual",
  "void", "volatile", "wchar_t", "while", "xor", "xor_eq"
};

hash_set<string> MakeKeywordsMap() {
  hash_set<string> result;
  for (int i = 0; i < GOOGLE_ARRAYSIZE(kKeywordList); i++) {
    result.insert(kKeywordList[i]);
  }
  return result;
}

hash_set<string> kKeywords = MakeKeywordsMap();

}  // namespace

const char kThickSeparator[] =
  "// ===================================================================\n";
const char kThinSeparator[] =
  "// -------------------------------------------------------------------\n";

string ClassName(const Descriptor* descriptor, bool qualified) {
  // Find "outer", the descriptor of the top-level message in which
  // "descriptor" is embedded.
  const Descriptor* outer = descriptor;
  while (outer->containing_type() != NULL) outer = outer->containing_type();

  const string& outer_name = outer->full_name();
  string inner_name = descriptor->full_name().substr(outer_name.size());

  if (qualified) {
    return "::" + DotsToColons(outer_name) + DotsToUnderscores(inner_name);
  } else {
    return outer->name() + DotsToUnderscores(inner_name);
  }
}

string ClassName(const EnumDescriptor* enum_descriptor, bool qualified) {
  if (enum_descriptor->containing_type() == NULL) {
    if (qualified) {
      return DotsToColons(enum_descriptor->full_name());
    } else {
      return enum_descriptor->name();
    }
  } else {
    string result = ClassName(enum_descriptor->containing_type(), qualified);
    result += '_';
    result += enum_descriptor->name();
    return result;
  }
}

string FieldName(const FieldDescriptor* field) {
  string result = field->name();
  LowerString(&result);
  if (kKeywords.count(result) > 0) {
    result.append("_");
  }
  return result;
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

// Convert a file name into a valid identifier.
string FilenameIdentifier(const string& filename) {
  string result;
  for (int i = 0; i < filename.size(); i++) {
    if (ascii_isalnum(filename[i])) {
      result.push_back(filename[i]);
    } else {
      // Not alphanumeric.  To avoid any possibility of name conflicts we
      // use the hex code for the character.
      result.push_back('_');
      char buffer[kFastToBufferSize];
      result.append(FastHexToBuffer(static_cast<uint8>(filename[i]), buffer));
    }
  }
  return result;
}

// Return the name of the BuildDescriptors() function for a given file.
string GlobalBuildDescriptorsName(const string& filename) {
  return "proto_BuildDescriptors_" + FilenameIdentifier(filename);
}

}  // namespace cpp
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
