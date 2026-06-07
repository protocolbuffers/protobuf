// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef GOOGLE_PROTOBUF_COMPILER_PERL_GENERATOR_H__
#define GOOGLE_PROTOBUF_COMPILER_PERL_GENERATOR_H__

#ifndef GOOGLE3
// Alias proto2_ symbols to google_protobuf_ for local builds using vendored upb
// These macros exceed 80 characters because they map long, contiguous C symbol
// tokens for open-source portability and cannot be physically wrapped.
#define proto2_DescriptorProto google_protobuf_DescriptorProto
#define proto2_DescriptorProto_enum_type \
  google_protobuf_DescriptorProto_enum_type  // NOLINT(whitespace/line_length)
#define proto2_DescriptorProto_field google_protobuf_DescriptorProto_field
#define proto2_DescriptorProto_name google_protobuf_DescriptorProto_name
#define proto2_DescriptorProto_nested_type \
  google_protobuf_DescriptorProto_nested_type  // NOLINT(whitespace/line_length)
#define proto2_EnumDescriptorProto google_protobuf_EnumDescriptorProto
#define proto2_EnumDescriptorProto_name google_protobuf_EnumDescriptorProto_name
#define proto2_EnumDescriptorProto_value \
  google_protobuf_EnumDescriptorProto_value  // NOLINT(whitespace/line_length)
#define proto2_EnumValueDescriptorProto google_protobuf_EnumValueDescriptorProto
#define proto2_EnumValueDescriptorProto_name \
  google_protobuf_EnumValueDescriptorProto_name  // NOLINT(whitespace/line_length)
#define proto2_EnumValueDescriptorProto_number \
  google_protobuf_EnumValueDescriptorProto_number  // NOLINT(whitespace/line_length)
#define proto2_FieldDescriptorProto google_protobuf_FieldDescriptorProto
#define proto2_FieldDescriptorProto_name \
  google_protobuf_FieldDescriptorProto_name  // NOLINT(whitespace/line_length)
#define proto2_FieldDescriptorProto_type \
  google_protobuf_FieldDescriptorProto_type  // NOLINT(whitespace/line_length)
#define proto2_FieldDescriptorProto_type_name \
  google_protobuf_FieldDescriptorProto_type_name  // NOLINT(whitespace/line_length)
#define proto2_FileDescriptorProto google_protobuf_FileDescriptorProto
#define proto2_FileDescriptorProto_dependency \
  google_protobuf_FileDescriptorProto_dependency  // NOLINT(whitespace/line_length)
#define proto2_FileDescriptorProto_enum_type \
  google_protobuf_FileDescriptorProto_enum_type  // NOLINT(whitespace/line_length)
#define proto2_FileDescriptorProto_message_type \
  google_protobuf_FileDescriptorProto_message_type  // NOLINT(whitespace/line_length)
#define proto2_FileDescriptorProto_name google_protobuf_FileDescriptorProto_name
#define proto2_FileDescriptorProto_package \
  google_protobuf_FileDescriptorProto_package  // NOLINT(whitespace/line_length)
#define proto2_FileDescriptorProto_serialize \
  google_protobuf_FileDescriptorProto_serialize  // NOLINT(whitespace/line_length)
#define proto2_FileDescriptorProto_service \
  google_protobuf_FileDescriptorProto_service  // NOLINT(whitespace/line_length)
#define proto2_MethodDescriptorProto google_protobuf_MethodDescriptorProto
#define proto2_MethodDescriptorProto_input_type \
  google_protobuf_MethodDescriptorProto_input_type  // NOLINT(whitespace/line_length)
#define proto2_MethodDescriptorProto_name \
  google_protobuf_MethodDescriptorProto_name  // NOLINT(whitespace/line_length)
#define proto2_MethodDescriptorProto_output_type \
  google_protobuf_MethodDescriptorProto_output_type  // NOLINT(whitespace/line_length)
#define proto2_ServiceDescriptorProto google_protobuf_ServiceDescriptorProto
#define proto2_ServiceDescriptorProto_method \
  google_protobuf_ServiceDescriptorProto_method  // NOLINT(whitespace/line_length)
#define proto2_ServiceDescriptorProto_name \
  google_protobuf_ServiceDescriptorProto_name  // NOLINT(whitespace/line_length)

// Alias proto2_compiler_ symbols to google_protobuf_compiler_ symbols
#define proto2_compiler_CodeGeneratorRequest \
  google_protobuf_compiler_CodeGeneratorRequest  // NOLINT(whitespace/line_length)
#define proto2_compiler_CodeGeneratorResponse \
  google_protobuf_compiler_CodeGeneratorResponse  // NOLINT(whitespace/line_length)
#define proto2_compiler_CodeGeneratorResponse_File \
  google_protobuf_compiler_CodeGeneratorResponse_File  // NOLINT(whitespace/line_length)

#define proto2__compiler__CodeGeneratorRequest_msg_init \
  google__protobuf__compiler__CodeGeneratorRequest_msg_init  // NOLINT(whitespace/line_length)
#define proto2__compiler__CodeGeneratorResponse_msg_init \
  google__protobuf__compiler__CodeGeneratorResponse_msg_init  // NOLINT(whitespace/line_length)
#define proto2__compiler__CodeGeneratorResponse__File_msg_init \
  google__protobuf__compiler__CodeGeneratorResponse__File_msg_init  // NOLINT(whitespace/line_length)

#define proto2_compiler_CodeGeneratorRequest_proto_file \
  google_protobuf_compiler_CodeGeneratorRequest_proto_file  // NOLINT(whitespace/line_length)
#define proto2_compiler_CodeGeneratorRequest_file_to_generate \
  google_protobuf_compiler_CodeGeneratorRequest_file_to_generate  // NOLINT(whitespace/line_length)
#define proto2_compiler_CodeGeneratorResponse_new \
  google_protobuf_compiler_CodeGeneratorResponse_new  // NOLINT(whitespace/line_length)
#define proto2_compiler_CodeGeneratorResponse_set_supported_features \
  google_protobuf_compiler_CodeGeneratorResponse_set_supported_features  // NOLINT(whitespace/line_length)
#define proto2_compiler_CodeGeneratorResponse_set_minimum_edition \
  google_protobuf_compiler_CodeGeneratorResponse_set_minimum_edition  // NOLINT(whitespace/line_length)
#define proto2_compiler_CodeGeneratorResponse_set_maximum_edition \
  google_protobuf_compiler_CodeGeneratorResponse_set_maximum_edition  // NOLINT(whitespace/line_length)
#define proto2_compiler_CodeGeneratorResponse_add_file \
  google_protobuf_compiler_CodeGeneratorResponse_add_file  // NOLINT(whitespace/line_length)
#define proto2_compiler_CodeGeneratorResponse_File_set_name \
  google_protobuf_compiler_CodeGeneratorResponse_File_set_name  // NOLINT(whitespace/line_length)
#define proto2_compiler_CodeGeneratorResponse_File_set_content \
  google_protobuf_compiler_CodeGeneratorResponse_File_set_content  // NOLINT(whitespace/line_length)

#define proto2_compiler_CodeGeneratorRequest_parse \
  google_protobuf_compiler_CodeGeneratorRequest_parse  // NOLINT(whitespace/line_length)
#define proto2_compiler_CodeGeneratorResponse_serialize \
  google_protobuf_compiler_CodeGeneratorResponse_serialize  // NOLINT(whitespace/line_length)
#define proto2_compiler_CodeGeneratorResponse_FEATURE_PROTO3_OPTIONAL \
  google_protobuf_compiler_CodeGeneratorResponse_FEATURE_PROTO3_OPTIONAL  // NOLINT(whitespace/line_length)
#define proto2_compiler_CodeGeneratorResponse_FEATURE_SUPPORTS_EDITIONS \
  google_protobuf_compiler_CodeGeneratorResponse_FEATURE_SUPPORTS_EDITIONS  // NOLINT(whitespace/line_length)
#endif

#include <sstream>
#include <string>
#include <unordered_map>

#ifdef _WIN32
#include <string_view>
namespace absl {
using string_view = std::string_view;
}  // namespace absl
#elif defined(GOOGLE3)
#include "absl/strings/string_view.h"
#else
#include "absl/strings/string_view.h"
#endif
#include "upb/mem/arena.h"

#ifdef GOOGLE3
#include "google/protobuf/descriptor.upb.h"
#include "google/protobuf/compiler/perl/plugin.upb.h"
#else
#include "google/protobuf/compiler/plugin.upb.h"
#include "google/protobuf/descriptor.upb.h"
#endif

namespace google {
namespace protobuf {
namespace compiler {
namespace perl {

class PerlCodeGenerator {
 public:
  PerlCodeGenerator(const proto2_compiler_CodeGeneratorRequest* request,
                    upb_Arena* arena)
      : request_(request), arena_(arena) {
    BuildPackageMap();
    BuildTypeMap();
  }

  proto2_compiler_CodeGeneratorResponse* Generate();

 private:
  void BuildPackageMap();
  void BuildTypeMap();
  void RegisterTypesRecursively(const proto2_DescriptorProto* msg_proto,
                                absl::string_view proto_prefix,
                                absl::string_view perl_prefix);
  std::string ResolvePerlClass(absl::string_view proto_type);
  std::string GetPerlPackage(const proto2_FileDescriptorProto* file_proto);

  std::string GenerateModule(const proto2_FileDescriptorProto* file_proto);
  std::string GenerateTypes(const proto2_FileDescriptorProto* file_proto);

  void PrintTypesRecursively(const proto2_DescriptorProto* msg_proto,
                             absl::string_view current_package,
                             std::stringstream& ss);
  void PrintMessageTypes(const proto2_DescriptorProto* msg_proto,
                         absl::string_view current_package,
                         std::stringstream& ss);
  void PrintEnumTypes(const proto2_EnumDescriptorProto* enum_proto,
                      absl::string_view current_package, std::stringstream& ss);

  upb_StringView ArenaCopy(absl::string_view s) {
    char* buf = (char*)upb_Arena_Malloc(arena_, s.length());
    memcpy(buf, s.data(), s.length());
    return upb_StringView_FromDataAndSize(buf, s.length());
  }

  const proto2_compiler_CodeGeneratorRequest* request_;
  upb_Arena* arena_;
  std::unordered_map<std::string, std::string> file_to_package_;
  std::unordered_map<std::string, std::string> type_to_perl_class_;
  std::string package_name_;
};

}  // namespace perl
}  // namespace compiler
}  // namespace protobuf
}  // namespace google

#endif  // GOOGLE_PROTOBUF_COMPILER_PERL_GENERATOR_H__
