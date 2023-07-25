// Copyright (c) 2009-2021, Google LLC
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//     * Redistributions of source code must retain the above copyright
//       notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above copyright
//       notice, this list of conditions and the following disclaimer in the
//       documentation and/or other materials provided with the distribution.
//     * Neither the name of Google LLC nor the
//       names of its contributors may be used to endorse or promote products
//       derived from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL Google LLC BE LIABLE FOR ANY DIRECT,
// INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
// ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include <memory>

#include "google/protobuf/descriptor.pb.h"
#include "google/protobuf/compiler/code_generator.h"
#include "google/protobuf/compiler/plugin.h"
#include "google/protobuf/descriptor.h"
#include "protos_generator/gen_enums.h"
#include "protos_generator/gen_extensions.h"
#include "protos_generator/gen_messages.h"
#include "protos_generator/gen_utils.h"
#include "protos_generator/names.h"
#include "protos_generator/output.h"
#include "upbc/file_layout.h"

namespace protos_generator {
namespace {

namespace protoc = ::google::protobuf::compiler;
namespace protobuf = ::google::protobuf;
using FileDescriptor = ::google::protobuf::FileDescriptor;

void WriteSource(const protobuf::FileDescriptor* file, Output& output,
                 bool fasttable_enabled);
void WriteHeader(const protobuf::FileDescriptor* file, Output& output);
void WriteForwardingHeader(const protobuf::FileDescriptor* file,
                           Output& output);
void WriteMessageImplementations(const protobuf::FileDescriptor* file,
                                 Output& output);
void WriteTypedefForwardingHeader(
    const protobuf::FileDescriptor* file,
    const std::vector<const protobuf::Descriptor*>& file_messages,
    Output& output);
void WriteHeaderMessageForwardDecls(
    const protobuf::FileDescriptor* file,
    Output& output);

class Generator : public protoc::CodeGenerator {
 public:
  ~Generator() override {}
  bool Generate(const protobuf::FileDescriptor* file,
                const std::string& parameter, protoc::GeneratorContext* context,
                std::string* error) const override;
  uint64_t GetSupportedFeatures() const override {
    return FEATURE_PROTO3_OPTIONAL;
  }
};

bool Generator::Generate(const protobuf::FileDescriptor* file,
                         const std::string& parameter,
                         protoc::GeneratorContext* context,
                         std::string* error) const {
  bool fasttable_enabled = false;
  std::vector<std::pair<std::string, std::string>> params;
  google::protobuf::compiler::ParseGeneratorParameter(parameter, &params);

  for (const auto& pair : params) {
    if (pair.first == "fasttable") {
      fasttable_enabled = true;
    } else {
      *error = "Unknown parameter: " + pair.first;
      return false;
    }
  }

  // Write model.upb.fwd.h
  Output forwarding_header_output(
      context->Open(ForwardingHeaderFilename(file)));
  WriteForwardingHeader(file, forwarding_header_output);
  // Write model.upb.proto.h
  Output header_output(context->Open(CppHeaderFilename(file)));
  WriteHeader(file, header_output);
  // Write model.upb.proto.cc
  Output cc_output(context->Open(CppSourceFilename(file)));
  WriteSource(file, cc_output, fasttable_enabled);
  return true;
}

// The forwarding header defines Access/Proxy/CProxy for message classes
// used to include when referencing dependencies to prevent transitive
// dependency headers from being included.
void WriteForwardingHeader(const protobuf::FileDescriptor* file,
                           Output& output) {
  EmitFileWarning(file, output);
  output(
      R"cc(
#ifndef $0_UPB_FWD_H_
#define $0_UPB_FWD_H_
      )cc",
      ToPreproc(file->name()));
  output("\n");
  for (int i = 0; i < file->public_dependency_count(); ++i) {
    output("#include \"$0\"\n",
           ForwardingHeaderFilename(file->public_dependency(i)));
  }
  if (file->public_dependency_count() > 0) {
    output("\n");
  }
  const std::vector<const protobuf::Descriptor*> this_file_messages =
      SortedMessages(file);
  WriteTypedefForwardingHeader(file, this_file_messages, output);
  output("#endif  /* $0_UPB_FWD_H_ */\n", ToPreproc(file->name()));
}

void WriteHeader(const protobuf::FileDescriptor* file, Output& output) {
  EmitFileWarning(file, output);
  output(
      R"cc(
#ifndef $0_UPB_PROTO_H_
#define $0_UPB_PROTO_H_

#include "protos/protos.h"
#include "protos/protos_internal.h"
#include "protos/repeated_field.h"
#include "upb/upb.hpp"

#include "absl/strings/string_view.h"
#include "absl/status/statusor.h"
      )cc",
      ToPreproc(file->name()));

  // Import headers for proto public dependencies.
  for (int i = 0; i < file->public_dependency_count(); i++) {
    if (i == 0) {
      output("// Public Imports.\n");
    }
    output("#include \"$0\"\n", CppHeaderFilename(file->public_dependency(i)));
    if (i == file->public_dependency_count() - 1) {
      output("\n");
    }
  }

  output("#include \"upb/port/def.inc\"\n");

  const std::vector<const protobuf::Descriptor*> this_file_messages =
      SortedMessages(file);
  const std::vector<const protobuf::FieldDescriptor*> this_file_exts =
      SortedExtensions(file);

  if (!this_file_messages.empty()) {
    output("\n");
  }

  WriteHeaderMessageForwardDecls(file, output);
  WriteStartNamespace(file, output);

  std::vector<const protobuf::EnumDescriptor*> this_file_enums =
      SortedEnums(file);

  // Write Class and Enums.
  WriteEnumDeclarations(this_file_enums, output);
  output("\n");

  for (auto message : this_file_messages) {
    WriteMessageClassDeclarations(message, this_file_exts, this_file_enums,
                                  output);
  }
  output("\n");

  WriteExtensionIdentifiersHeader(this_file_exts, output);
  output("\n");

  WriteEndNamespace(file, output);

  output("\n#include \"upb/port/undef.inc\"\n\n");
  // End of "C" section.

  output("#endif  /* $0_UPB_PROTO_H_ */\n", ToPreproc(file->name()));
}

// Writes a .upb.cc source file.
void WriteSource(const protobuf::FileDescriptor* file, Output& output,
                 bool fasttable_enabled) {
  EmitFileWarning(file, output);

  output(
      R"cc(
#include <stddef.h>
#include "absl/strings/string_view.h"
#include "protos/protos.h"
#include "$0"
      )cc",
      CppHeaderFilename(file));

  for (int i = 0; i < file->dependency_count(); i++) {
    output("#include \"$0\"\n", CppHeaderFilename(file->dependency(i)));
  }
  output("#include \"upb/port/def.inc\"\n");

  WriteStartNamespace(file, output);
  WriteMessageImplementations(file, output);
  const std::vector<const protobuf::FieldDescriptor*> this_file_exts =
      SortedExtensions(file);
  WriteExtensionIdentifiers(this_file_exts, output);
  WriteEndNamespace(file, output);

  output("#include \"upb/port/undef.inc\"\n\n");
}

void WriteMessageImplementations(const protobuf::FileDescriptor* file,
                                 Output& output) {
  const std::vector<const protobuf::FieldDescriptor*> file_exts =
      SortedExtensions(file);
  const std::vector<const protobuf::Descriptor*> this_file_messages =
      SortedMessages(file);
  for (auto message : this_file_messages) {
    WriteMessageImplementation(message, file_exts, output);
  }
}

void WriteTypedefForwardingHeader(
    const protobuf::FileDescriptor* file,
    const std::vector<const protobuf::Descriptor*>& file_messages,
    Output& output) {
  WriteStartNamespace(file, output);

  // Forward-declare types defined in this file.
  for (auto message : file_messages) {
    output(
        R"cc(
          class $0;
          namespace internal {
          class $0Access;
          class $0Proxy;
          class $0CProxy;
          }  // namespace internal
        )cc",
        ClassName(message));
  }
  output("\n");
  WriteEndNamespace(file, output);
}

/// Writes includes for upb C minitables and fwd.h for transitive typedefs.
void WriteHeaderMessageForwardDecls(
    const protobuf::FileDescriptor* file,
    Output& output) {
  // Import forward-declaration of types defined in this file.
  output("#include \"$0\"\n", UpbCFilename(file));
  output("#include \"$0\"\n", ForwardingHeaderFilename(file));
  // Import forward-declaration of types in dependencies.
  for (int i = 0; i < file->dependency_count(); ++i) {
    output("#include \"$0\"\n", ForwardingHeaderFilename(file->dependency(i)));
  }
  output("\n");
}

}  // namespace
}  // namespace protos_generator

int main(int argc, char** argv) {
  protos_generator::Generator generator_cc;
  return google::protobuf::compiler::PluginMain(argc, argv, &generator_cc);
}
