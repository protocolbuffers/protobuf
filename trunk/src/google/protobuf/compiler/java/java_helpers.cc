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

#include <google/protobuf/compiler/java/java_helpers.h>
#include <google/protobuf/descriptor.pb.h>
#include <google/protobuf/stubs/strutil.h>

namespace google {
namespace protobuf {
namespace compiler {
namespace java {

const char kThickSeparator[] =
  "// ===================================================================\n";
const char kThinSeparator[] =
  "// -------------------------------------------------------------------\n";

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
  if (file->options().has_java_outer_classname()) {
    return file->options().java_outer_classname();
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

string FileJavaPackage(const FileDescriptor* file) {
  if (file->options().has_java_package()) {
    return file->options().java_package();
  } else {
    string result = kDefaultPackage;
    if (!file->package().empty()) {
      if (!result.empty()) result += '.';
      result += file->package();
    }
    return result;
  }
}

string ToJavaName(const string& full_name, const FileDescriptor* file) {
  string result;
  if (file->options().java_multiple_files()) {
    result = FileJavaPackage(file);
  } else {
    result = ClassName(file);
  }
  if (!result.empty()) {
    result += '.';
  }
  if (file->package().empty()) {
    result += full_name;
  } else {
    // Strip the proto package from full_name since we've replaced it with
    // the Java package.
    result += full_name.substr(file->package().size() + 1);
  }
  return result;
}

string ClassName(const FileDescriptor* descriptor) {
  string result = FileJavaPackage(descriptor);
  if (!result.empty()) result += '.';
  result += FileClassName(descriptor);
  return result;
}

JavaType GetJavaType(FieldDescriptor::Type field_type) {
  switch (field_type) {
    case FieldDescriptor::TYPE_INT32:
    case FieldDescriptor::TYPE_UINT32:
    case FieldDescriptor::TYPE_SINT32:
    case FieldDescriptor::TYPE_FIXED32:
    case FieldDescriptor::TYPE_SFIXED32:
      return JAVATYPE_INT;

    case FieldDescriptor::TYPE_INT64:
    case FieldDescriptor::TYPE_UINT64:
    case FieldDescriptor::TYPE_SINT64:
    case FieldDescriptor::TYPE_FIXED64:
    case FieldDescriptor::TYPE_SFIXED64:
      return JAVATYPE_LONG;

    case FieldDescriptor::TYPE_FLOAT:
      return JAVATYPE_FLOAT;

    case FieldDescriptor::TYPE_DOUBLE:
      return JAVATYPE_DOUBLE;

    case FieldDescriptor::TYPE_BOOL:
      return JAVATYPE_BOOLEAN;

    case FieldDescriptor::TYPE_STRING:
      return JAVATYPE_STRING;

    case FieldDescriptor::TYPE_BYTES:
      return JAVATYPE_BYTES;

    case FieldDescriptor::TYPE_ENUM:
      return JAVATYPE_ENUM;

    case FieldDescriptor::TYPE_GROUP:
    case FieldDescriptor::TYPE_MESSAGE:
      return JAVATYPE_MESSAGE;

    // No default because we want the compiler to complain if any new
    // types are added.
  }

  GOOGLE_LOG(FATAL) << "Can't get here.";
  return JAVATYPE_INT;
}

const char* BoxedPrimitiveTypeName(JavaType type) {
  switch (type) {
    case JAVATYPE_INT    : return "java.lang.Integer";
    case JAVATYPE_LONG   : return "java.lang.Long";
    case JAVATYPE_FLOAT  : return "java.lang.Float";
    case JAVATYPE_DOUBLE : return "java.lang.Double";
    case JAVATYPE_BOOLEAN: return "java.lang.Boolean";
    case JAVATYPE_STRING : return "java.lang.String";
    case JAVATYPE_BYTES  : return "com.google.protobuf.ByteString";
    case JAVATYPE_ENUM   : return NULL;
    case JAVATYPE_MESSAGE: return NULL;

    // No default because we want the compiler to complain if any new
    // JavaTypes are added.
  }

  GOOGLE_LOG(FATAL) << "Can't get here.";
  return NULL;
}

}  // namespace java
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
