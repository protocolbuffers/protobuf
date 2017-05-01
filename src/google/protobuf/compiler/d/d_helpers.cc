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
#include <vector>
#include <google/protobuf/stubs/hash.h>

#include <google/protobuf/compiler/d/d_helpers.h>
#include <google/protobuf/stubs/logging.h>
#include <google/protobuf/stubs/common.h>
#include <google/protobuf/stubs/strutil.h>
#include <google/protobuf/stubs/substitute.h>


namespace google {
namespace protobuf {
namespace compiler {
namespace d {

namespace {

static const char kAnyMessageName[] = "Any";
static const char kAnyProtoFile[] = "google/protobuf/any.proto";
static const char kGoogleProtobufPrefix[] = "google/protobuf/";

const char* const kKeywordList[] = {
  "abstract", "alias", "align", "asm", "assert", "auto", "body", "bool",
  "break", "byte", "case", "cast", "catch", "cdouble", "cent", "cfloat",
  "char", "class", "const", "continue", "creal", "dchar", "debug", "default",
  "delegate", "delete", "deprecated", "do", "double", "else", "enum", "export",
  "extern", "false", "final", "finally", "float", "for", "foreach",
  "foreach_reverse", "function", "goto", "idouble", "if", "ifloat",
  "immutable", "import", "in", "inout", "int", "interface", "invariant",
  "ireal", "is", "lazy", "long", "macro", "mixin", "module", "new", "nothrow",
  "null", "out", "override", "package", "pragma", "private", "protected",
  "public", "pure", "real", "ref", "return", "scope", "shared", "short",
  "static", "struct", "super", "switch", "synchronized", "template", "this",
  "throw", "true", "try", "typedef", "typeid", "typeof", "ubyte", "ucent",
  "uint", "ulong", "union", "unittest", "ushort", "version", "void",
  "volatile", "wchar", "while", "with", "__FILE__", "__MODULE__", "__LINE__",
  "__FUNCTION__", "__PRETTY_FUNCTION__", "__gshared", "__traits", "__vector",
  "__parameters", "string", "wstring", "dstring", "size_t", "ptrdiff_t",
  "__DATE__", "__EOF__", "__TIME__", "__TIMESTAMP__", "__VENDOR__",
  "__VERSION__",
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
      cap_next_letter = false;
    } else {
      cap_next_letter = true;
    }
  }
  return EscapeKeywords(result);
}

string EscapeKeywords(const string& input, char delim) {
  if (delim == '\0') {
    if (kKeywords.count(input) > 0) {
      return input + '_';
    }
    return input;    
  }

  vector<string> res;
  char delim_str[2] = { '\0', '\0' };
  delim_str[0] = delim;

  SplitStringUsing(input, delim_str, &res);
  for (vector<string>::iterator it = res.begin(); it != res.end(); ++it) {
    *it = EscapeKeywords(*it);
  }
  return JoinStrings(res, delim_str);
}

string StripDotProto(const string& proto_file) {
  int lastindex = proto_file.find_last_of('.');
  return proto_file.substr(0, lastindex);
}

string WireFormat(const FieldDescriptor* field) {
  switch (field->type()) {
    case FieldDescriptor::TYPE_BOOL:
    case FieldDescriptor::TYPE_INT32:
    case FieldDescriptor::TYPE_UINT32:
    case FieldDescriptor::TYPE_INT64:
    case FieldDescriptor::TYPE_UINT64:
    case FieldDescriptor::TYPE_FLOAT:
    case FieldDescriptor::TYPE_DOUBLE:
    case FieldDescriptor::TYPE_STRING:
    case FieldDescriptor::TYPE_BYTES:
    case FieldDescriptor::TYPE_ENUM:
      return "";
    case FieldDescriptor::TYPE_MESSAGE:
      if (field->is_map())
      {
        const string key_wire_format =
            WireFormat(field->message_type()->FindFieldByNumber(1));
        const string value_wire_format =
            WireFormat(field->message_type()->FindFieldByNumber(2));

        if (key_wire_format.length() || value_wire_format.length())
          return key_wire_format + "," + value_wire_format;
      }
      return "";
    case FieldDescriptor::TYPE_SINT32:
    case FieldDescriptor::TYPE_SINT64:
      return "zigzag";
    case FieldDescriptor::TYPE_SFIXED32:
    case FieldDescriptor::TYPE_FIXED32:
    case FieldDescriptor::TYPE_SFIXED64:
    case FieldDescriptor::TYPE_FIXED64:
      return "fixed";
    case FieldDescriptor::TYPE_GROUP:
    default:
      assert(false);
      return "";
  }
}

string BaseTypeName(const FieldDescriptor* field) {
  switch (field->type()) {
    case FieldDescriptor::TYPE_BOOL:
      return "bool";
    case FieldDescriptor::TYPE_INT32:
    case FieldDescriptor::TYPE_SINT32:
    case FieldDescriptor::TYPE_SFIXED32:
      return "int";
    case FieldDescriptor::TYPE_UINT32:
    case FieldDescriptor::TYPE_FIXED32:
      return "uint";
    case FieldDescriptor::TYPE_INT64:
    case FieldDescriptor::TYPE_SINT64:
    case FieldDescriptor::TYPE_SFIXED64:
      return "long";
    case FieldDescriptor::TYPE_UINT64:
    case FieldDescriptor::TYPE_FIXED64:
      return "ulong";
    case FieldDescriptor::TYPE_FLOAT:
      return "float";
    case FieldDescriptor::TYPE_DOUBLE:
      return "double";
    case FieldDescriptor::TYPE_STRING:
      return "string";
    case FieldDescriptor::TYPE_BYTES:
      return "bytes";
    case FieldDescriptor::TYPE_MESSAGE:
      return field->message_type()->name();
    case FieldDescriptor::TYPE_ENUM:
      return field->enum_type()->name();
    case FieldDescriptor::TYPE_GROUP:
    default:
      assert(false);
      return "";
  }
}

string TypeName(const FieldDescriptor* field) {
  string base_type_name = BaseTypeName(field);
  if (field->is_map()) {
    const FieldDescriptor* key_field =
        field->message_type()->FindFieldByNumber(1);
    const FieldDescriptor* value_field =
        field->message_type()->FindFieldByNumber(2);

    return BaseTypeName(value_field) + "[" + BaseTypeName(key_field) + "]";
  }
  if (base_type_name.length() && field->is_repeated()) {
    return base_type_name + "[]";
  }
  else {
    return base_type_name;
  }
}

string ModuleName(const FileDescriptor* file)
{
  string name = StripSuffixString(file->name(), ".proto");
  size_t pos = name.find_last_of('/');
  if (pos != string::npos) {
    name = name.substr(pos + 1);
  }

  if (file->package().empty()) {
    return EscapeKeywords(name);
  } else {
    return EscapeKeywords(file->package() + '.' + name, '.');
  }
}

string OutputFileName(const FileDescriptor* file)
{
  return StringReplace(ModuleName(file), ".", "/", true) + ".d";
}

string EnumValueName(const EnumValueDescriptor* enum_value) {
  string result = enum_value->name();
  if (kKeywords.count(result) > 0) {
    result.append("_");
  }
  return result;
}

bool IsAnyMessage(const FileDescriptor* descriptor) {
  return descriptor->name() == kAnyProtoFile;
}

bool IsAnyMessage(const Descriptor* descriptor) {
  return descriptor->name() == kAnyMessageName &&
         descriptor->file()->name() == kAnyProtoFile;
}

bool IsWellKnownMessage(const FileDescriptor* descriptor) {
  return !descriptor->name().compare(0, 16, kGoogleProtobufPrefix);
}

}  // namespace cpp
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
