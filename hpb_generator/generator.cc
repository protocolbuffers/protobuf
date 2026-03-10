// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "hpb_generator/generator.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "absl/strings/str_cat.h"
#include "absl/strings/str_replace.h"
#include "google/protobuf/compiler/code_generator.h"
#include "google/protobuf/compiler/code_generator_lite.h"
#include "hpb_generator/context.h"
#include "hpb_generator/gen_enums.h"
#include "hpb_generator/gen_extensions.h"
#include "hpb_generator/gen_messages.h"
#include "hpb_generator/gen_utils.h"
#include "hpb_generator/names.h"
#include "google/protobuf/descriptor.h"
#include "upb_generator/c/names.h"
#include "upb_generator/minitable/names.h"

namespace google {
namespace protobuf {
namespace hpb_generator {
namespace {

using FileDescriptor = ::google::protobuf::FileDescriptor;

void WriteTypedefForwardingHeader(
    const google::protobuf::FileDescriptor* file,
    const std::vector<const google::protobuf::Descriptor*>& file_messages, Context& ctx);

void WriteHeaderMessageForwardDecls(const google::protobuf::FileDescriptor* file,
                                    Context& ctx);

void WriteMessageImplementations(const google::protobuf::FileDescriptor* file,
                                 Context& ctx);

void WriteForwardDecls(const google::protobuf::FileDescriptor* file, Context& ctx) {
  for (int i = 0; i < file->public_dependency_count(); ++i) {
    const auto target_file_messages =
        SortedMessages(file->public_dependency(i));
    WriteTypedefForwardingHeader(file->public_dependency(i),
                                 target_file_messages, ctx);
  }
  const auto this_file_messages = SortedMessages(file);
  WriteTypedefForwardingHeader(file, this_file_messages, ctx);
}

void WriteHeader(const google::protobuf::FileDescriptor* file, Context& ctx) {
  if (ctx.options().backend == Backend::CPP) {
    EmitFileWarning(file, ctx);

    ctx.Emit({{"filename", ToPreproc(file->name())}},
             R"cc(
#ifndef $filename$_HPB_PROTO_H_
#define $filename$_HPB_PROTO_H_
             )cc");

    // Import headers for proto public dependencies.
    for (int i = 0; i < file->public_dependency_count(); i++) {
      if (i == 0) {
        ctx.Emit("// Public Imports.\n");
      }
      ctx.Emit({{"header", CppHeaderFilename(file->public_dependency(i))}},
               "#include \"$header$\"\n");
      if (i == file->public_dependency_count() - 1) {
        ctx.Emit("\n");
      }
    }

    ctx.Emit(
        "#include \"hpb/internal/os_macros_undef.inc\"\n");

    const std::vector<const google::protobuf::Descriptor*> this_file_messages =
        SortedMessages(file);
    const std::vector<const google::protobuf::FieldDescriptor*>
        this_file_exts{};  // TODO: extensions

    if (!this_file_messages.empty()) {
      ctx.Emit("\n");
    }

    WriteHeaderMessageForwardDecls(file, ctx);
    WriteForwardDecls(file, ctx);

    std::vector<const google::protobuf::EnumDescriptor*> this_file_enums =
        SortedEnums(file);

    WrapNamespace(file, ctx, [&]() {
      // Write Class and Enums.
      WriteEnumDeclarations(this_file_enums, ctx);
      ctx.Emit("\n");
      // TODO: class decls
      // TODO: extension identifiers
    });

    const auto msgs = SortedMessages(file);
    for (auto message : msgs) {
      ctx.Emit({{"type", QualifiedClassName(message)},
                {"class_name", ClassName(message)},
                {"namespace", absl::StrCat(absl::StrReplaceAll(file->package(),
                                                               {{".", "::"}}),
                                           "::protos")}},
               R"cc(
#include "hpb/internal/internal.h"

                 // message stubs
                 namespace $namespace$ {

                 class $class_name$ {
                  public:
                   using CProxy = bool;
                   using Proxy = bool;
                   using Access = bool;

                   $class_name$() = default;

                  private:
                   $class_name$($type$* msg) : msg_(msg) {}

                   $type$* msg_;

                   $type$* msg() const { return msg_; }

                   friend struct ::hpb::internal::PrivateAccess;
                 };
                 }  // namespace $namespace$
               )cc");
    }
    ctx.Emit(
        "#include "
        "\"hpb/internal/os_macros_restore.inc\"\n");
    ctx.Emit({{"filename", ToPreproc(file->name())}},
             "#endif  /* $filename$_HPB_PROTO_H_ */\n");
    return;
  }
  EmitFileWarning(file, ctx);
  ctx.Emit({{"filename", ToPreproc(file->name())}},
           R"cc(
#ifndef $filename$_HPB_PROTO_H_
#define $filename$_HPB_PROTO_H_

#include "hpb/repeated_field.h"

#include "absl/status/statusor.h"
#include "absl/strings/string_view.h"
           )cc");

  // Import headers for proto public dependencies.
  for (int i = 0; i < file->public_dependency_count(); i++) {
    if (i == 0) {
      ctx.Emit("// Public Imports.\n");
    }
    ctx.Emit({{"header", CppHeaderFilename(file->public_dependency(i))}},
             "#include \"$header$\"\n");
    if (i == file->public_dependency_count() - 1) {
      ctx.Emit("\n");
    }
  }

  ctx.Emit("#include \"upb/port/def.inc\"\n");
  ctx.Emit(
      "#include \"hpb/internal/os_macros_undef.inc\"\n");

  const std::vector<const google::protobuf::Descriptor*> this_file_messages =
      SortedMessages(file);
  const std::vector<const google::protobuf::FieldDescriptor*> this_file_exts =
      SortedExtensions(file);

  if (!this_file_messages.empty()) {
    ctx.Emit("\n");
  }

  WriteHeaderMessageForwardDecls(file, ctx);
  WriteForwardDecls(file, ctx);

  std::vector<const google::protobuf::EnumDescriptor*> this_file_enums =
      SortedEnums(file);

  WrapNamespace(file, ctx, [&]() {
    // Write Class and Enums.
    WriteEnumDeclarations(this_file_enums, ctx);
    ctx.Emit("\n");

    for (auto message : this_file_messages) {
      WriteMessageClassDeclarations(message, this_file_exts, this_file_enums,
                                    ctx);
    }
    ctx.Emit("\n");

    WriteExtensionIdentifiersHeader(this_file_exts, ctx);
    ctx.Emit("\n");
  });

  ctx.Emit("namespace hpb::internal {\n");
  const std::vector<const google::protobuf::Descriptor*> messages_to_emit_helpers =
      SortedMessages(file);
  for (auto desc : messages_to_emit_helpers) {
    if (desc->map_key() != nullptr) {
      continue;
    }
    std::string outer_namespace =
        absl::StrCat(NamespaceFromPackageName(file->package()), "::");
    if (file->package().empty()) {
      outer_namespace = "";
    }
    ctx.Emit({{"class_name", ClassName(desc)},
              {"minitable_name",
               upb::generator::MiniTableMessageVarName(desc->full_name())},
              {"outer_namespace", outer_namespace},
              {"c_api_msg_type",
               upb::generator::CApiMessageType(desc->full_name())}},
             R"cc(
               template <>
               struct AssociatedUpbTypes<$outer_namespace$$class_name$> {
                 using CMessageType = $c_api_msg_type$;
                 static inline const upb_MiniTable* kMiniTable = &$minitable_name$;
               };
             )cc");
  }

  ctx.Emit("}  // namespace hpb::internal\n");
  ctx.Emit(
      "#include \"hpb/internal/os_macros_restore.inc\"\n");
  ctx.Emit("\n#include \"upb/port/undef.inc\"\n\n");
  // End of "C" section.

  ctx.Emit({{"filename", ToPreproc(file->name())}},
           "#endif  /* $filename$_HPB_PROTO_H_ */\n");
}

// Writes a .hpb.cc source file.
void WriteSource(const google::protobuf::FileDescriptor* file, Context& ctx) {
  if (ctx.options().backend == Backend::CPP) {
    ctx.Emit("// Placeholder hpb C++ source stub");
    return;
  }
  EmitFileWarning(file, ctx);

  ctx.Emit({{"header", CppHeaderFilename(file)}},
           R"cc(
#include <stddef.h>
#include "absl/log/absl_check.h"
#include "absl/strings/string_view.h"
#include "$header$"
           )cc");

  for (int i = 0; i < file->dependency_count(); i++) {
    if (ctx.options().strip_feature_includes &&
        compiler::IsKnownFeatureProto(file->dependency(i)->name())) {
      // Strip feature imports for editions codegen tests.
      continue;
    }
    ctx.Emit({{"header", CppHeaderFilename(file->dependency(i))}},
             "#include \"$header$\"\n");
  }
  ctx.Emit("#include \"upb/port/def.inc\"\n");

  WrapNamespace(file, ctx, [&]() { WriteMessageImplementations(file, ctx); });

  ctx.Emit("#include \"upb/port/undef.inc\"\n\n");
}

void WriteMessageImplementations(const google::protobuf::FileDescriptor* file,
                                 Context& ctx) {
  const std::vector<const google::protobuf::FieldDescriptor*> file_exts =
      SortedExtensions(file);
  const std::vector<const google::protobuf::Descriptor*> this_file_messages =
      SortedMessages(file);
  for (auto message : this_file_messages) {
    WriteMessageImplementation(message, file_exts, ctx);
  }
}

void WriteTypedefForwardingHeader(
    const google::protobuf::FileDescriptor* file,
    const std::vector<const google::protobuf::Descriptor*>& file_messages, Context& ctx) {
  WrapNamespace(file, ctx, [&]() {
    // Forward-declare types defined in this file.
    for (auto message : file_messages) {
      ctx.Emit({{"class_name", ClassName(message)}},
               R"cc(
                 class $class_name$;
                 namespace internal {
                 class $class_name$Access;
                 class $class_name$Proxy;
                 class $class_name$CProxy;
                 }  // namespace internal
               )cc");
    }
  });
  ctx.Emit("\n");
}

/// Writes includes for upb C minitables and fwd.h for transitive typedefs.
void WriteHeaderMessageForwardDecls(const google::protobuf::FileDescriptor* file,
                                    Context& ctx) {
  // Import forward-declaration of types defined in this file.
  if (ctx.options().backend == Backend::UPB) {
    ctx.Emit({{"upb_filename", UpbCFilename(file)}},
             "#include \"$upb_filename$\"\n");
  }
  WriteForwardDecls(file, ctx);
  // Import forward-declaration of types in dependencies.
  for (int i = 0; i < file->dependency_count(); ++i) {
    if (ctx.options().strip_feature_includes &&
        compiler::IsKnownFeatureProto(file->dependency(i)->name())) {
      // Strip feature imports for editions codegen tests.
      continue;
    }
    WriteForwardDecls(file->dependency(i), ctx);
  }
  ctx.Emit("\n");
}

}  // namespace

bool Generator::Generate(const google::protobuf::FileDescriptor* file,
                         const std::string& parameter,
                         protoc::GeneratorContext* context,
                         std::string* error) const {
  {
    bool strip_nonfunctional_codegen = false;
    Backend backend = Backend::UPB;
    std::vector<std::pair<std::string, std::string>> params;
    google::protobuf::compiler::ParseGeneratorParameter(parameter, &params);

    for (const auto& pair : params) {
      if (pair.first == "experimental_strip_nonfunctional_codegen") {
        strip_nonfunctional_codegen = true;
      } else if (pair.first == "backend" && pair.second == "cpp") {
        backend = Backend::CPP;
      } else {
        *error = "Unknown parameter: " + pair.first;
        return false;
      }
    }
    // Write model.hpb.h
    Options options = {.backend = backend,
                       .strip_feature_includes = strip_nonfunctional_codegen};
    std::unique_ptr<google::protobuf::io::ZeroCopyOutputStream> header_output_stream(
        context->Open(CppHeaderFilename(file)));
    Context hdr_ctx(file, header_output_stream.get(), options);
    WriteHeader(file, hdr_ctx);

    // Write model.hpb.cc
    std::unique_ptr<google::protobuf::io::ZeroCopyOutputStream> cc_output_stream(
        context->Open(CppSourceFilename(file)));
    auto cc_ctx = Context(file, cc_output_stream.get(), options);
    WriteSource(file, cc_ctx);
    return true;
  }
}
}  // namespace hpb_generator
}  // namespace protobuf
}  // namespace google
