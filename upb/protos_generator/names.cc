/*
 * Copyright (c) 2009-2021, Google LLC
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of Google LLC nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL Google LLC BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "protos_generator/names.h"

#include <string>

#include "upbc/keywords.h"

namespace protos_generator {

namespace protobuf = ::google::protobuf;

namespace {

std::string NamespaceFromPackageName(absl::string_view package_name) {
  return absl::StrCat(absl::StrReplaceAll(package_name, {{".", "::"}}),
                      "::protos");
}

std::string DotsToColons(const std::string& name) {
  return absl::StrReplaceAll(name, {{".", "::"}});
}

std::string Namespace(const std::string& package) {
  if (package.empty()) return "";
  return "::" + DotsToColons(package);
}

// Return the qualified C++ name for a file level symbol.
std::string QualifiedFileLevelSymbol(const protobuf::FileDescriptor* file,
                                     const std::string& name) {
  if (file->package().empty()) {
    return absl::StrCat("::", name);
  }
  // Append ::protos postfix to package name.
  return absl::StrCat(Namespace(file->package()), "::protos::", name);
}

std::string CppTypeInternal(const protobuf::FieldDescriptor* field,
                            bool is_const, bool is_type_parameter) {
  std::string maybe_const = is_const ? "const " : "";
  switch (field->cpp_type()) {
    case protobuf::FieldDescriptor::CPPTYPE_MESSAGE: {
      if (is_type_parameter) {
        return absl::StrCat(maybe_const,
                            QualifiedClassName(field->message_type()));
      } else {
        return absl::StrCat(maybe_const,
                            QualifiedClassName(field->message_type()), "*");
      }
    }
    case protobuf::FieldDescriptor::CPPTYPE_BOOL:
      return "bool";
    case protobuf::FieldDescriptor::CPPTYPE_FLOAT:
      return "float";
    case protobuf::FieldDescriptor::CPPTYPE_INT32:
    case protobuf::FieldDescriptor::CPPTYPE_ENUM:
      return "int32_t";
    case protobuf::FieldDescriptor::CPPTYPE_UINT32:
      return "uint32_t";
    case protobuf::FieldDescriptor::CPPTYPE_DOUBLE:
      return "double";
    case protobuf::FieldDescriptor::CPPTYPE_INT64:
      return "int64_t";
    case protobuf::FieldDescriptor::CPPTYPE_UINT64:
      return "uint64_t";
    case protobuf::FieldDescriptor::CPPTYPE_STRING:
      return "absl::string_view";
    default:
      ABSL_LOG(FATAL) << "Unexpected type: " << field->cpp_type();
  }
}

}  // namespace

std::string ClassName(const protobuf::Descriptor* descriptor) {
  const protobuf::Descriptor* parent = descriptor->containing_type();
  std::string res;
  // Classes in global namespace without package names are prefixed
  // by protos_ to avoid collision with C compiler structs defined in
  // proto.upb.h.
  if ((parent && parent->file()->package().empty()) ||
      descriptor->file()->package().empty()) {
    res = std::string(kNoPackageNamePrefix);
  }
  if (parent) res += ClassName(parent) + "_";
  absl::StrAppend(&res, descriptor->name());
  return ::upbc::ResolveKeywordConflict(res);
}

std::string QualifiedClassName(const protobuf::Descriptor* descriptor) {
  return QualifiedFileLevelSymbol(descriptor->file(), ClassName(descriptor));
}

std::string QualifiedInternalClassName(const protobuf::Descriptor* descriptor) {
  return QualifiedFileLevelSymbol(
      descriptor->file(), absl::StrCat("internal::", ClassName(descriptor)));
}

std::string CppSourceFilename(const google::protobuf::FileDescriptor* file) {
  return StripExtension(file->name()) + ".upb.proto.cc";
}

std::string ForwardingHeaderFilename(const google::protobuf::FileDescriptor* file) {
  return StripExtension(file->name()) + ".upb.fwd.h";
}

std::string UpbCFilename(const google::protobuf::FileDescriptor* file) {
  return StripExtension(file->name()) + ".upb.h";
}

std::string CppHeaderFilename(const google::protobuf::FileDescriptor* file) {
  return StripExtension(file->name()) + ".upb.proto.h";
}

void WriteStartNamespace(const protobuf::FileDescriptor* file, Output& output) {
  // Skip namespace generation if package name is not specified.
  if (file->package().empty()) {
    return;
  }

  output("namespace $0 {\n\n", NamespaceFromPackageName(file->package()));
}

void WriteEndNamespace(const protobuf::FileDescriptor* file, Output& output) {
  if (file->package().empty()) {
    return;
  }
  output("} //  namespace $0\n\n", NamespaceFromPackageName(file->package()));
}

std::string CppConstType(const protobuf::FieldDescriptor* field) {
  return CppTypeInternal(field, /* is_const= */ true,
                         /* is_type_parameter= */ false);
}

std::string CppTypeParameterName(const protobuf::FieldDescriptor* field) {
  return CppTypeInternal(field, /* is_const= */ false,
                         /* is_type_parameter= */ true);
}

std::string MessageBaseType(const protobuf::FieldDescriptor* field,
                            bool is_const) {
  ABSL_DCHECK(field->cpp_type() == protobuf::FieldDescriptor::CPPTYPE_MESSAGE);
  std::string maybe_const = is_const ? "const " : "";
  return maybe_const + QualifiedClassName(field->message_type());
}

std::string MessagePtrConstType(const protobuf::FieldDescriptor* field,
                                bool is_const) {
  ABSL_DCHECK(field->cpp_type() == protobuf::FieldDescriptor::CPPTYPE_MESSAGE);
  std::string maybe_const = is_const ? "const " : "";
  return "::protos::Ptr<" + maybe_const +
         QualifiedClassName(field->message_type()) + ">";
}

std::string MessageCProxyType(const protobuf::FieldDescriptor* field,
                              bool is_const) {
  ABSL_DCHECK(field->cpp_type() == protobuf::FieldDescriptor::CPPTYPE_MESSAGE);
  std::string maybe_const = is_const ? "const " : "";
  return maybe_const + QualifiedInternalClassName(field->message_type()) +
         "CProxy";
}

std::string MessageProxyType(const protobuf::FieldDescriptor* field,
                             bool is_const) {
  ABSL_DCHECK(field->cpp_type() == protobuf::FieldDescriptor::CPPTYPE_MESSAGE);
  std::string maybe_const = is_const ? "const " : "";
  return maybe_const + QualifiedInternalClassName(field->message_type()) +
         "Proxy";
}

}  // namespace protos_generator
