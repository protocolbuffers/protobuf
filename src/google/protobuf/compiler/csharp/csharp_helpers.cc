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

// Author: jonskeet@google.com (Jon Skeet)
//  Following the Java generator by kenton@google.com (Kenton Varda)
//  Based on original Protocol Buffers design by
//  Sanjay Ghemawat, Jeff Dean, and others.

#include <vector>

#include <google/protobuf/compiler/csharp/csharp_helpers.h>
#include <google/protobuf/descriptor.pb.h>
#include <google/protobuf/stubs/strutil.h>

namespace google {
namespace protobuf {
namespace compiler {
namespace csharp {

const char kThickSeparator[] =
  "// ===================================================================\r\n";
const char kThinSeparator[] =
  "// -------------------------------------------------------------------\r\n";

namespace {

const char* kDefaultPackage = "";

const string& FieldName(const FieldDescriptor* field) {
  // Groups are hacky:  The name of the field is just the lower-cased name
  // of the group type.  In Java, though, we would like to retain the original
  // capitalization of the type name.
  if (field->type() == FieldDescriptor::TYPE_GROUP) {
    return field->message_type()->name();
  } else {
    return field->name();
  }
}

string UnderscoresToCamelCaseImpl(const string& input, bool cap_next_letter) {
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
      if (i == 0 && !cap_next_letter) {
        // Force first letter to lower-case unless explicitly told to
        // capitalize it.
        result += input[i] + ('a' - 'A');
      } else {
        // Capital letters after the first are left as-is.
        result += input[i];
      }
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

}  // namespace

string UnderscoresToCamelCase(const FieldDescriptor* field) {
  return UnderscoresToCamelCaseImpl(FieldName(field), false);
}

string UnderscoresToCapitalizedCamelCase(const FieldDescriptor* field) {
  return UnderscoresToCamelCaseImpl(FieldName(field), true);
}

string UnderscoresToCapitalizedCamelCase(const MethodDescriptor* method) {
  return UnderscoresToCamelCaseImpl(method->name(), true);
}

string UnderscoresToCamelCase(const MethodDescriptor* method) {
  return UnderscoresToCamelCaseImpl(method->name(), false);
}

string StripProto(const string& filename) {
  if (HasSuffixString(filename, ".protodevel")) {
    return StripSuffixString(filename, ".protodevel");
  } else {
    return StripSuffixString(filename, ".proto");
  }
}

string FileClassName(const FileDescriptor* file) {
  if (file->options().has_csharp_file_classname()) {
    return file->options().csharp_file_classname();
  } else {
    string basename;
    string::size_type last_slash = file->name().find_last_of('/');
    if (last_slash == string::npos) {
      basename = file->name();
    } else {
      basename = file->name().substr(last_slash + 1);
    }
    return UnderscoresToCamelCaseImpl(StripProto(basename), true);
  }
}

string FileCSharpNamespace(const FileDescriptor* file) {
  if (file->options().has_csharp_namespace()) {
    return file->options().csharp_namespace();
  } else {
    string result = kDefaultPackage;
    if (!file->package().empty()) {
      if (!result.empty()) result += '.';
      result += file->package();
    }
    return result;
  }
}

string ToCSharpName(const string& full_name, const FileDescriptor* file) {
  string result;
  if (!file->options().csharp_nest_classes()) {
    result = "";
  } else {
    result = ClassName(file);
  }
  if (!result.empty()) {
    result += '.';
  }
  string classname;
  if (file->package().empty()) {
    classname = full_name;
  } else {
    // Strip the proto package from full_name since we've replaced it with
    // the C# package.
    classname = full_name.substr(file->package().size() + 1);
  }
  result += StringReplace(classname, ".", ".Types.", true);
  const char *prefix = FileCSharpNamespace(file).empty() ? "global::" : "self::";
  return prefix + result;
}

string ClassName(const FileDescriptor* descriptor) {
  string alias = FileCSharpNamespace(descriptor).empty() ? "global::" : "self::";
  return alias + FileClassName(descriptor);
}

MappedType GetMappedType(FieldDescriptor::Type field_type) {
  switch (field_type) {
    case FieldDescriptor::TYPE_INT32:
    case FieldDescriptor::TYPE_SINT32:
    case FieldDescriptor::TYPE_SFIXED32:
      return MAPPEDTYPE_INT;

    case FieldDescriptor::TYPE_INT64:
    case FieldDescriptor::TYPE_SINT64:
    case FieldDescriptor::TYPE_SFIXED64:
      return MAPPEDTYPE_LONG;

    case FieldDescriptor::TYPE_UINT32:
    case FieldDescriptor::TYPE_FIXED32:
      return MAPPEDTYPE_UINT;

    case FieldDescriptor::TYPE_UINT64:
    case FieldDescriptor::TYPE_FIXED64:
      return MAPPEDTYPE_ULONG;

    case FieldDescriptor::TYPE_FLOAT:
      return MAPPEDTYPE_FLOAT;

    case FieldDescriptor::TYPE_DOUBLE:
      return MAPPEDTYPE_DOUBLE;

    case FieldDescriptor::TYPE_BOOL:
      return MAPPEDTYPE_BOOLEAN;

    case FieldDescriptor::TYPE_STRING:
      return MAPPEDTYPE_STRING;

    case FieldDescriptor::TYPE_BYTES:
      return MAPPEDTYPE_BYTES;

    case FieldDescriptor::TYPE_ENUM:
      return MAPPEDTYPE_ENUM;

    case FieldDescriptor::TYPE_GROUP:
    case FieldDescriptor::TYPE_MESSAGE:
      return MAPPEDTYPE_MESSAGE;

    // No default because we want the compiler to complain if any new
    // types are added.
  }

  GOOGLE_LOG(FATAL) << "Can't get here.";
  return MAPPEDTYPE_INT;
}

const char* MappedTypeName(MappedType type) {
  switch (type) {
    case MAPPEDTYPE_INT    : return "int";
    case MAPPEDTYPE_LONG   : return "long";
    case MAPPEDTYPE_UINT    : return "uint";
    case MAPPEDTYPE_ULONG   : return "ulong";
    case MAPPEDTYPE_FLOAT  : return "float";
    case MAPPEDTYPE_DOUBLE : return "double";
    case MAPPEDTYPE_BOOLEAN: return "bool";
    case MAPPEDTYPE_STRING : return "string";
    case MAPPEDTYPE_BYTES  : return "pb::ByteString";
    case MAPPEDTYPE_ENUM   : return NULL;
    case MAPPEDTYPE_MESSAGE: return NULL;

    // No default because we want the compiler to complain if any new
    // MappedTypes are added.
  }

  GOOGLE_LOG(FATAL) << "Can't get here.";
  return NULL;
}

const char* ClassAccessLevel(const FileDescriptor* file) {
  return file->options().csharp_public_classes() ? "public" : "internal";
}

}  // namespace csharp
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
