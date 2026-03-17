// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "hpb_generator/names.h"

#include <string>

#include "absl/log/absl_check.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/string_view.h"
#include "google/protobuf/compiler/code_generator.h"
#include "hpb_generator/keywords.h"
#include "google/protobuf/descriptor.h"

namespace google {
namespace protobuf {
namespace hpb_generator {

namespace {

std::string DotsToColons(const absl::string_view name) {
  return absl::StrReplaceAll(name, {{".", "::"}});
}

std::string Namespace(const absl::string_view package) {
  if (package.empty()) return "";
  return "::" + DotsToColons(package);
}

// Return the qualified C++ name for a file level symbol.
std::string QualifiedFileLevelSymbol(const google::protobuf::FileDescriptor* file,
                                     const std::string& name) {
  if (file->package().empty()) {
    return absl::StrCat("::", name);
  }
  // Append ::protos postfix to package name.
  return absl::StrCat(Namespace(file->package()), "::protos::", name);
}

std::string CppTypeInternal(const google::protobuf::FieldDescriptor* field, bool is_const,
                            bool is_type_parameter) {
  std::string maybe_const = is_const ? "const " : "";
  switch (field->cpp_type()) {
    case google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE: {
      if (is_type_parameter) {
        return absl::StrCat(maybe_const,
                            QualifiedClassName(field->message_type()));
      } else {
        return absl::StrCat(maybe_const,
                            QualifiedClassName(field->message_type()), "*");
      }
    }
    case google::protobuf::FieldDescriptor::CPPTYPE_BOOL:
      return "bool";
    case google::protobuf::FieldDescriptor::CPPTYPE_FLOAT:
      return "float";
    case google::protobuf::FieldDescriptor::CPPTYPE_INT32:
    case google::protobuf::FieldDescriptor::CPPTYPE_ENUM:
      return "int32_t";
    case google::protobuf::FieldDescriptor::CPPTYPE_UINT32:
      return "uint32_t";
    case google::protobuf::FieldDescriptor::CPPTYPE_DOUBLE:
      return "double";
    case google::protobuf::FieldDescriptor::CPPTYPE_INT64:
      return "int64_t";
    case google::protobuf::FieldDescriptor::CPPTYPE_UINT64:
      return "uint64_t";
    case google::protobuf::FieldDescriptor::CPPTYPE_STRING:
      return "::absl::string_view";
    default:
      ABSL_LOG(FATAL) << "Unexpected type: " << field->cpp_type();
  }
}

}  // namespace

std::string ClassName(const google::protobuf::Descriptor* descriptor) {
  const google::protobuf::Descriptor* parent = descriptor->containing_type();
  std::string res;
  // Classes in global namespace without package names are prefixed
  // by hpb_ to avoid collision with C compiler structs defined in
  // proto.upb.h.
  if ((parent && parent->file()->package().empty()) ||
      descriptor->file()->package().empty()) {
    res = std::string(kNoPackageNamePrefix);
  }
  if (parent) res += ClassName(parent) + "_";
  absl::StrAppend(&res, descriptor->name());
  return ResolveKeywordConflict(res);
}

std::string QualifiedClassName(const google::protobuf::Descriptor* descriptor) {
  return QualifiedFileLevelSymbol(descriptor->file(), ClassName(descriptor));
}

std::string QualifiedInternalClassName(const google::protobuf::Descriptor* descriptor) {
  return QualifiedFileLevelSymbol(
      descriptor->file(), absl::StrCat("internal::", ClassName(descriptor)));
}

std::string CppSourceFilename(const google::protobuf::FileDescriptor* file) {
  return compiler::StripProto(file->name()) + ".hpb.cc";
}

std::string UpbCFilename(const google::protobuf::FileDescriptor* file) {
  return compiler::StripProto(file->name()) + ".upb.h";
}

std::string CppHeaderFilename(const google::protobuf::FileDescriptor* file) {
  return absl::StrCat(compiler::StripProto(file->name()), ".hpb.h");
}

std::string CppConstType(const google::protobuf::FieldDescriptor* field) {
  return CppTypeInternal(field, /* is_const= */ true,
                         /* is_type_parameter= */ false);
}

std::string CppTypeParameterName(const google::protobuf::FieldDescriptor* field) {
  return CppTypeInternal(field, /* is_const= */ false,
                         /* is_type_parameter= */ true);
}

std::string MessageBaseType(const google::protobuf::FieldDescriptor* field,
                            bool is_const) {
  ABSL_DCHECK(field->cpp_type() == google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE);
  std::string maybe_const = is_const ? "const " : "";
  return maybe_const + QualifiedClassName(field->message_type());
}

std::string MessagePtrConstType(const google::protobuf::FieldDescriptor* field,
                                bool is_const) {
  ABSL_DCHECK(field->cpp_type() == google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE);
  std::string maybe_const = is_const ? "const " : "";
  return "::hpb::Ptr<" + maybe_const +
         QualifiedClassName(field->message_type()) + ">";
}

std::string MessageCProxyType(const google::protobuf::FieldDescriptor* field,
                              bool is_const) {
  ABSL_DCHECK(field->cpp_type() == google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE);
  std::string maybe_const = is_const ? "const " : "";
  return maybe_const + QualifiedInternalClassName(field->message_type()) +
         "CProxy";
}

std::string MessageProxyType(const google::protobuf::FieldDescriptor* field,
                             bool is_const) {
  ABSL_DCHECK(field->cpp_type() == google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE);
  std::string maybe_const = is_const ? "const " : "";
  return maybe_const + QualifiedInternalClassName(field->message_type()) +
         "Proxy";
}
}  // namespace hpb_generator
}  // namespace protobuf
}  // namespace google
