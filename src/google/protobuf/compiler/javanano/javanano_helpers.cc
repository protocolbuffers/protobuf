// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
// http://code.google.com/p/protobuf/
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

#include <vector>

#include <google/protobuf/compiler/javanano/javanano_helpers.h>
#include <google/protobuf/compiler/javanano/javanano_params.h>
#include <google/protobuf/descriptor.pb.h>
#include <google/protobuf/stubs/strutil.h>
#include <google/protobuf/stubs/substitute.h>

namespace google {
namespace protobuf {
namespace compiler {
namespace javanano {

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

string FileClassName(const Params& params, const FileDescriptor* file) {
  string name;

  if (params.has_java_outer_classname(file->name())) {
      name = params.java_outer_classname(file->name());
  } else {
    if ((file->message_type_count() == 1)
        || (file->enum_type_count() == 0)) {
      // If no outer calls and only one message then
      // use the message name as the file name
      name = file->message_type(0)->name();
    } else {
      // Use the filename it self with underscores removed
      // and a CamelCase style name.
      string basename;
      string::size_type last_slash = file->name().find_last_of('/');
      if (last_slash == string::npos) {
        basename = file->name();
      } else {
        basename = file->name().substr(last_slash + 1);
      }
      name = UnderscoresToCamelCaseImpl(StripProto(basename), true);
    }
  }

  return name;
}

string FileJavaPackage(const Params& params, const FileDescriptor* file) {
  if (params.has_java_package(file->name())) {
    return params.java_package(file->name());
  } else {
    string result = kDefaultPackage;
    if (!file->package().empty()) {
      if (!result.empty()) result += '.';
      result += file->package();
    }
    return result;
  }
}

string ToJavaName(const Params& params, const string& full_name,
    const FileDescriptor* file) {
  string result;
  if (params.java_multiple_files()) {
    result = FileJavaPackage(params, file);
  } else {
    result = ClassName(params, file);
  }
  if (file->package().empty()) {
    result += '.';
    result += full_name;
  } else {
    // Strip the proto package from full_name since we've replaced it with
    // the Java package. If there isn't an outer classname then strip it too.
    int sizeToSkipPackageName = file->package().size();
    int sizeToSkipOutClassName;
    if (params.has_java_outer_classname(file->name())) {
      sizeToSkipOutClassName = 0;
    } else {
      sizeToSkipOutClassName =
                full_name.find_first_of('.', sizeToSkipPackageName + 1);
    }
    int sizeToSkip = sizeToSkipOutClassName > 0 ?
            sizeToSkipOutClassName : sizeToSkipPackageName;
    string class_name = full_name.substr(sizeToSkip + 1);
    if (class_name == FileClassName(params, file)) {
      // Done class_name is already present.
    } else {
      result += '.';
      result += class_name;
    }
  }
  return result;
}

string ClassName(const Params& params, const FileDescriptor* descriptor) {
  string result = FileJavaPackage(params, descriptor);
  if (!result.empty()) result += '.';
  result += FileClassName(params, descriptor);
  return result;
}

string ClassName(const Params& params, const EnumDescriptor* descriptor) {
  string result;
  const FileDescriptor* file = descriptor->file();
  const string file_name = file->name();
  const string full_name = descriptor->full_name();

  // Remove enum class name as we use int's for enums
  string base_name = full_name.substr(0, full_name.find_last_of('.'));

  if (!file->package().empty()) {
    if (file->package() == base_name.substr(0, file->package().size())) {
      // Remove package name leaving just the parent class of the enum
      int offset = file->package().size();
      if (base_name.size() > offset) {
        // Remove period between package and class name if there is a classname
        offset += 1;
      }
      base_name = base_name.substr(offset);
    } else {
      GOOGLE_LOG(FATAL) << "Expected package name to start an enum";
    }
  }

  // Construct the path name from the package and outer class

  // Add the java package name if it exsits
  if (params.has_java_package(file_name)) {
    result += params.java_package(file_name);
  }

  // Add the outer classname if it exists
  if (params.has_java_outer_classname(file_name)) {
    if (!result.empty()) {
      result += ".";
    }
    result += params.java_outer_classname(file_name);
  }

  // Create the full class name from the base and path
  if (!base_name.empty()) {
    if (!result.empty()) {
      result += ".";
    }
    result += base_name;
  }
  return result;
}

string FieldConstantName(const FieldDescriptor *field) {
  string name = field->name() + "_FIELD_NUMBER";
  UpperString(&name);
  return name;
}

string FieldDefaultConstantName(const FieldDescriptor *field) {
  string name = field->name() + "_DEFAULT";
  UpperString(&name);
  return name;
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
    case JAVATYPE_BYTES  : return "byte[]";
    case JAVATYPE_ENUM   : return "java.lang.Integer";
    case JAVATYPE_MESSAGE: return NULL;

    // No default because we want the compiler to complain if any new
    // JavaTypes are added.
  }

  GOOGLE_LOG(FATAL) << "Can't get here.";
  return NULL;
}

string EmptyArrayName(const Params& params, const FieldDescriptor* field) {
  switch (GetJavaType(field)) {
    case JAVATYPE_INT    : return "com.google.protobuf.nano.WireFormatNano.EMPTY_INT_ARRAY";
    case JAVATYPE_LONG   : return "com.google.protobuf.nano.WireFormatNano.EMPTY_LONG_ARRAY";
    case JAVATYPE_FLOAT  : return "com.google.protobuf.nano.WireFormatNano.EMPTY_FLOAT_ARRAY";
    case JAVATYPE_DOUBLE : return "com.google.protobuf.nano.WireFormatNano.EMPTY_DOUBLE_ARRAY";
    case JAVATYPE_BOOLEAN: return "com.google.protobuf.nano.WireFormatNano.EMPTY_BOOLEAN_ARRAY";
    case JAVATYPE_STRING : return "com.google.protobuf.nano.WireFormatNano.EMPTY_STRING_ARRAY";
    case JAVATYPE_BYTES  : return "com.google.protobuf.nano.WireFormatNano.EMPTY_BYTES_ARRAY";
    case JAVATYPE_ENUM   : return "com.google.protobuf.nano.WireFormatNano.EMPTY_INT_ARRAY";
    case JAVATYPE_MESSAGE: return ClassName(params, field->message_type()) + ".EMPTY_ARRAY";

    // No default because we want the compiler to complain if any new
    // JavaTypes are added.
  }

  GOOGLE_LOG(FATAL) << "Can't get here.";
  return "";
}

string DefaultValue(const Params& params, const FieldDescriptor* field) {
  if (field->label() == FieldDescriptor::LABEL_REPEATED) {
    return EmptyArrayName(params, field);
  }

  // Switch on cpp_type since we need to know which default_value_* method
  // of FieldDescriptor to call.
  switch (field->cpp_type()) {
    case FieldDescriptor::CPPTYPE_INT32:
      return SimpleItoa(field->default_value_int32());
    case FieldDescriptor::CPPTYPE_UINT32:
      // Need to print as a signed int since Java has no unsigned.
      return SimpleItoa(static_cast<int32>(field->default_value_uint32()));
    case FieldDescriptor::CPPTYPE_INT64:
      return SimpleItoa(field->default_value_int64()) + "L";
    case FieldDescriptor::CPPTYPE_UINT64:
      return SimpleItoa(static_cast<int64>(field->default_value_uint64())) +
             "L";
    case FieldDescriptor::CPPTYPE_DOUBLE:
      return SimpleDtoa(field->default_value_double()) + "D";
    case FieldDescriptor::CPPTYPE_FLOAT:
      return SimpleFtoa(field->default_value_float()) + "F";
    case FieldDescriptor::CPPTYPE_BOOL:
      return field->default_value_bool() ? "true" : "false";
    case FieldDescriptor::CPPTYPE_STRING:
      if (!field->default_value_string().empty()) {
        // Point it to the static final in the generated code.
        return FieldDefaultConstantName(field);
      } else {
        if (field->type() == FieldDescriptor::TYPE_BYTES) {
          return "com.google.protobuf.nano.WireFormatNano.EMPTY_BYTES";
        } else {
          return "\"\"";
        }
      }

    case FieldDescriptor::CPPTYPE_ENUM:
      return ClassName(params, field->enum_type()) + "." +
             field->default_value_enum()->name();

    case FieldDescriptor::CPPTYPE_MESSAGE:
      return "null";

    // No default because we want the compiler to complain if any new
    // types are added.
  }

  GOOGLE_LOG(FATAL) << "Can't get here.";
  return "";
}

}  // namespace javanano
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
