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

#include "google/protobuf/compiler/cpp/file.h"

#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "google/protobuf/compiler/scc.h"
#include "absl/container/btree_map.h"
#include "absl/container/btree_set.h"
#include "absl/container/flat_hash_map.h"
#include "absl/container/flat_hash_set.h"
#include "absl/strings/escaping.h"
#include "absl/strings/match.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_format.h"
#include "absl/strings/string_view.h"
#include "absl/strings/strip.h"
#include "google/protobuf/compiler/cpp/enum.h"
#include "google/protobuf/compiler/cpp/extension.h"
#include "google/protobuf/compiler/cpp/helpers.h"
#include "google/protobuf/compiler/cpp/message.h"
#include "google/protobuf/compiler/cpp/names.h"
#include "google/protobuf/compiler/cpp/service.h"
#include "google/protobuf/compiler/retention.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/descriptor.pb.h"
#include "google/protobuf/io/printer.h"

// Must be last.
#include "google/protobuf/port_def.inc"

namespace google {
namespace protobuf {
namespace compiler {
namespace cpp {
namespace {
using Sub = ::google::protobuf::io::Printer::Sub;
using ::google::protobuf::internal::cpp::IsLazilyInitializedFile;

absl::flat_hash_map<absl::string_view, std::string> FileVars(
    const FileDescriptor* file, const Options& options) {
  return {
      {"filename", file->name()},
      {"package_ns", Namespace(file, options)},
      {"tablename", UniqueName("TableStruct", file, options)},
      {"desc_table", DescriptorTableName(file, options)},
      {"dllexport_decl", options.dllexport_decl},
      {"file_level_metadata", UniqueName("file_level_metadata", file, options)},
      {"file_level_enum_descriptors",
       UniqueName("file_level_enum_descriptors", file, options)},
      {"file_level_service_descriptors",
       UniqueName("file_level_service_descriptors", file, options)},
  };
}

// TODO(b/203101078): remove pragmas that suppresses uninitialized warnings when
// clang bug is fixed.
void MuteWuninitialized(io::Printer* p) {
  p->Emit(R"(
    #if defined(__llvm__)
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wuninitialized"
    #endif  // __llvm__
  )");
}

void UnmuteWuninitialized(io::Printer* p) {
  p->Emit(R"(
    #if defined(__llvm__)
    #pragma clang diagnostic pop
    #endif  // __llvm__
  )");
}

}  // namespace

bool FileGenerator::ShouldSkipDependencyImports(
    const FileDescriptor* dep) const {
  // Do not import weak deps.
  if (!options_.opensource_runtime && IsDepWeak(dep)) {
    return true;
  }

  // Skip feature imports, which are a visible (but non-functional) deviation
  // between editions and legacy syntax.
  if (options_.strip_nonfunctional_codegen &&
      dep->name() == "third_party/protobuf/cpp_features.proto") {
    return true;
  }

  return false;
}

FileGenerator::FileGenerator(const FileDescriptor* file, const Options& options)
    : file_(file), options_(options), scc_analyzer_(options) {
  std::vector<const Descriptor*> msgs = FlattenMessagesInFile(file);

  for (int i = 0; i < msgs.size(); ++i) {
    message_generators_.push_back(std::make_unique<MessageGenerator>(
        msgs[i], variables_, i, options, &scc_analyzer_));
    message_generators_.back()->AddGenerators(&enum_generators_,
                                              &extension_generators_);
  }

  for (int i = 0; i < file->enum_type_count(); ++i) {
    enum_generators_.push_back(
        std::make_unique<EnumGenerator>(file->enum_type(i), options));
  }

  for (int i = 0; i < file->service_count(); ++i) {
    service_generators_.push_back(std::make_unique<ServiceGenerator>(
        file->service(i), variables_, options));
  }
  if (HasGenericServices(file_, options_)) {
    for (int i = 0; i < service_generators_.size(); ++i) {
      service_generators_[i]->index_in_metadata_ = i;
    }
  }

  for (int i = 0; i < file->extension_count(); ++i) {
    extension_generators_.push_back(std::make_unique<ExtensionGenerator>(
        file->extension(i), options, &scc_analyzer_));
  }

  for (int i = 0; i < file->weak_dependency_count(); ++i) {
    weak_deps_.insert(file->weak_dependency(i));
  }
}

void FileGenerator::GenerateFile(io::Printer* p, GeneratedFileType file_type,
                                 std::function<void()> cb) {
  auto v = p->WithVars(FileVars(file_, options_));
  auto guard = IncludeGuard(file_, file_type, options_);
  p->Emit({{"cb", cb}, {"guard", guard}}, R"(
    // Generated by the protocol buffer compiler.  DO NOT EDIT!
    // source: $filename$

    #ifndef $guard$
    #define $guard$

    #include <limits>
    #include <string>
    #include <type_traits>

    $cb$;

    #endif  // $guard$
  )");
}

void FileGenerator::GenerateMacroUndefs(io::Printer* p) {
  // Only do this for protobuf's own types. There are some google3 protos using
  // macros as field names and the generated code compiles after the macro
  // expansion. Undefing these macros actually breaks such code.
  if (file_->name() != "third_party/protobuf/compiler/plugin.proto" &&
      file_->name() != "google/protobuf/compiler/plugin.proto") {
    return;
  }

  std::vector<const FieldDescriptor*> fields;
  ListAllFields(file_, &fields);

  absl::flat_hash_set<absl::string_view> all_fields;
  for (const FieldDescriptor* field : fields) {
    all_fields.insert(field->name());
  }

  for (absl::string_view name : {"major", "minor"}) {
    if (!all_fields.contains(name)) {
      continue;
    }

    p->Emit({{"name", std::string(name)}}, R"(
      #ifdef $name$
      #undef $name$
      #endif  // $name$
    )");
  }
}


void FileGenerator::GenerateSharedHeaderCode(io::Printer* p) {
  p->Emit(
      {
          {"port_def",
           [&] { IncludeFile("third_party/protobuf/port_def.inc", p); }},
          {"port_undef",
           [&] { IncludeFile("third_party/protobuf/port_undef.inc", p); }},
          {"dllexport_macro", FileDllExport(file_, options_)},
          {"undefs", [&] { GenerateMacroUndefs(p); }},
          {"global_state_decls",
           [&] { GenerateGlobalStateFunctionDeclarations(p); }},
          {"any_metadata",
           [&] {
             NamespaceOpener ns(ProtobufNamespace(options_), p);
             p->Emit(R"cc(
               namespace internal {
               class AnyMetadata;
               }  // namespace internal
             )cc");
           }},
          {"fwd_decls", [&] { GenerateForwardDeclarations(p); }},
          {"proto2_ns_enums",
           [&] { GenerateProto2NamespaceEnumSpecializations(p); }},
          {"main_decls",
           [&] {
             NamespaceOpener ns(Namespace(file_, options_), p);
             p->Emit(
                 {
                     {"enums", [&] { GenerateEnumDefinitions(p); }},
                     {"messages", [&] { GenerateMessageDefinitions(p); }},
                     {"services", [&] { GenerateServiceDefinitions(p); }},
                     {"extensions", [&] { GenerateExtensionIdentifiers(p); }},
                     {"inline_fns",
                      [&] { GenerateInlineFunctionDefinitions(p); }},
                 },
                 R"(
                   $enums$

                   $hrule_thick$

                   $messages$

                   $hrule_thick$

                   $services$

                   $extensions$

                   $hrule_thick$

                   $inline_fns$

                   // @@protoc_insertion_point(namespace_scope)
                 )");
           }},
      },
      R"(
          // Must be included last.
          $port_def$

          #define $dllexport_macro$$ dllexport_decl$
          $undefs$

          $any_metadata$;

          $global_state_decls$;
          $fwd_decls$

          $main_decls$

          $proto2_ns_enums$

          // @@protoc_insertion_point(global_scope)

          $port_undef$
      )");
}

void FileGenerator::GenerateProtoHeader(io::Printer* p,
                                        absl::string_view info_path) {
  if (!options_.proto_h) {
    return;
  }

  GenerateFile(p, GeneratedFileType::kProtoH, [&] {
    if (!options_.opensource_runtime) {
      p->Emit(R"(
          #ifdef SWIG
          #error "Do not SWIG-wrap protobufs."
          #endif  // SWIG
        )");
    }
    if (IsBootstrapProto(options_, file_)) {
      p->Emit({{"name", StripProto(file_->name())}}, R"cc(
        // IWYU pragma: private, include "$name$.proto.h"
      )cc");
    }

    p->Emit(
        {
            {"library_includes", [&] { GenerateLibraryIncludes(p); }},
            {"proto_includes",
             [&] {
               for (int i = 0; i < file_->public_dependency_count(); ++i) {
                 const FileDescriptor* dep = file_->public_dependency(i);
                 p->Emit({{"name", StripProto(dep->name())}}, R"(
                    #include "$name$.proto.h"
                 )");
               }
             }},
            {"metadata_pragma", [&] { GenerateMetadataPragma(p, info_path); }},
            {"header_main", [&] { GenerateSharedHeaderCode(p); }},
        },
        R"cc(
          $library_includes$;
          $proto_includes$;
          // @@protoc_insertion_point(includes)

          $metadata_pragma$;
          $header_main$;
        )cc");
  });
}

void FileGenerator::GeneratePBHeader(io::Printer* p,
                                     absl::string_view info_path) {
  GenerateFile(p, GeneratedFileType::kPbH, [&] {
    p->Emit(
        {
            {"library_includes",
             [&] {
               if (options_.proto_h) {
                 std::string target_basename = StripProto(file_->name());
                 if (!options_.opensource_runtime) {
                   GetBootstrapBasename(options_, target_basename,
                                        &target_basename);
                 }
                 p->Emit({{"name", target_basename}}, R"(
              #include "$name$.proto.h"  // IWYU pragma: export
              )");
               } else {
                 GenerateLibraryIncludes(p);
               }
             }},
            {"proto_includes",
             [&] {
               if (options_.transitive_pb_h) {
                 GenerateDependencyIncludes(p);
               }
             }},
            {"metadata_pragma", [&] { GenerateMetadataPragma(p, info_path); }},
            {"header_main",
             [&] {
               if (!options_.proto_h) {
                 GenerateSharedHeaderCode(p);
                 return;
               }

               {
                 NamespaceOpener ns(Namespace(file_, options_), p);
                 p->Emit(R"cc(

                   // @@protoc_insertion_point(namespace_scope)
                 )cc");
               }
               p->Emit(R"cc(

                 // @@protoc_insertion_point(global_scope)
               )cc");
             }},
        },
        R"cc(
          $library_includes$;
          $proto_includes$;
          // @@protoc_insertion_point(includes)

          $metadata_pragma$;
          $header_main$;
        )cc");
  });
}

void FileGenerator::DoIncludeFile(absl::string_view google3_name,
                                  bool do_export, io::Printer* p) {
  constexpr absl::string_view prefix = "third_party/protobuf/";
  ABSL_CHECK(absl::StartsWith(google3_name, prefix)) << google3_name;

  auto v = p->WithVars(
      {{"export_suffix", do_export ? "// IWYU pragma: export" : ""}});

  if (options_.opensource_runtime) {
    absl::ConsumePrefix(&google3_name, prefix);
    absl::ConsumePrefix(&google3_name, "internal/");
    absl::ConsumePrefix(&google3_name, "proto/");
    absl::ConsumePrefix(&google3_name, "public/");

    std::string path;
    if (absl::ConsumePrefix(&google3_name, "io/public/")) {
      path = absl::StrCat("io/", google3_name);
    } else {
      path = std::string(google3_name);
    }

    if (options_.runtime_include_base.empty()) {
      p->Emit({{"path", path}}, R"(
        #include "google/protobuf/$path$"$  export_suffix$
      )");
    } else {
      p->Emit({{"base", options_.runtime_include_base}, {"path", path}},
              R"(
        #include "$base$google/protobuf/$path$"$  export_suffix$
      )");
    }
  } else {
    std::string path(google3_name);
    // The bootstrapped proto generated code needs to use the
    // third_party/protobuf header paths to avoid circular dependencies.
    if (options_.bootstrap) {
      constexpr absl::string_view bootstrap_prefix = "net/proto2/public";
      if (absl::ConsumePrefix(&google3_name, bootstrap_prefix)) {
        path = absl::StrCat("third_party/protobuf", google3_name);
      }
    }

    p->Emit({{"path", path}}, R"(
      #include "$path$"$  export_suffix$
    )");
  }
}

std::string FileGenerator::CreateHeaderInclude(absl::string_view basename,
                                               const FileDescriptor* file) {
  if (options_.opensource_runtime && IsWellKnownMessage(file) &&
      !options_.runtime_include_base.empty()) {
    return absl::StrCat("\"", options_.runtime_include_base, basename, "\"");
  }

  return absl::StrCat("\"", basename, "\"");
}

void FileGenerator::GenerateSourceIncludes(io::Printer* p) {
  std::string target_basename = StripProto(file_->name());
  if (!options_.opensource_runtime) {
    GetBootstrapBasename(options_, target_basename, &target_basename);
  }

  absl::StrAppend(&target_basename, options_.proto_h ? ".proto.h" : ".pb.h");
  p->Emit({{"h_include", CreateHeaderInclude(target_basename, file_)}},
          R"(
        // Generated by the protocol buffer compiler.  DO NOT EDIT!
        // source: $filename$

        #include $h_include$

        #include <algorithm>
      )");

  IncludeFile("third_party/protobuf/io/coded_stream.h", p);
  // TODO(gerbens) This is to include parse_context.h, we need a better way
  IncludeFile("third_party/protobuf/extension_set.h", p);
  IncludeFile("third_party/protobuf/wire_format_lite.h", p);

  // Unknown fields implementation in lite mode uses StringOutputStream
  if (!UseUnknownFieldSet(file_, options_) && !message_generators_.empty()) {
    IncludeFile("third_party/protobuf/io/zero_copy_stream_impl_lite.h", p);
  }

  if (HasDescriptorMethods(file_, options_)) {
    IncludeFile("third_party/protobuf/descriptor.h", p);
    IncludeFile("third_party/protobuf/generated_message_reflection.h", p);
    IncludeFile("third_party/protobuf/reflection_ops.h", p);
    IncludeFile("third_party/protobuf/wire_format.h", p);
  }

  if (HasGeneratedMethods(file_, options_) &&
      options_.tctable_mode != Options::kTCTableNever) {
    IncludeFile("third_party/protobuf/generated_message_tctable_impl.h", p);
  }

  if (options_.proto_h) {
    // Use the smaller .proto.h files.
    for (int i = 0; i < file_->dependency_count(); ++i) {
      const FileDescriptor* dep = file_->dependency(i);

      if (ShouldSkipDependencyImports(dep)) continue;

      std::string basename = StripProto(dep->name());
      if (IsBootstrapProto(options_, file_)) {
        GetBootstrapBasename(options_, basename, &basename);
      }
      p->Emit({{"name", basename}}, R"(
        #include "$name$.proto.h"
      )");
    }
  }

  if (HasCordFields(file_, options_)) {
    p->Emit(R"(
      #include "absl/strings/internal/string_constant.h"
    )");
  }

  p->Emit(R"cc(
    // @@protoc_insertion_point(includes)

    // Must be included last.
  )cc");
  IncludeFile("third_party/protobuf/port_def.inc", p);
}

void FileGenerator::GenerateSourcePrelude(io::Printer* p) {
  // For MSVC builds, we use #pragma init_seg to move the initialization of our
  // libraries to happen before the user code.
  // This worksaround the fact that MSVC does not do constant initializers when
  // required by the standard.
  p->Emit(R"cc(
    PROTOBUF_PRAGMA_INIT_SEG
    namespace _pb = ::$proto_ns$;
    namespace _pbi = ::$proto_ns$::internal;
  )cc");

  if (HasGeneratedMethods(file_, options_) &&
      options_.tctable_mode != Options::kTCTableNever) {
    p->Emit(R"cc(
      namespace _fl = ::$proto_ns$::internal::field_layout;
    )cc");
  }
}

void FileGenerator::GenerateSourceDefaultInstance(int idx, io::Printer* p) {
  MessageGenerator* generator = message_generators_[idx].get();

  // Generate the split instance first because it's needed in the constexpr
  // constructor.
  if (ShouldSplit(generator->descriptor(), options_)) {
    // Use a union to disable the destructor of the _instance member.
    // We can constant initialize, but the object will still have a non-trivial
    // destructor that we need to elide.
    //
    // NO_DESTROY is not necessary for correctness. The empty destructor is
    // enough. However, the empty destructor fails to be elided in some
    // configurations (like non-opt or with certain sanitizers). NO_DESTROY is
    // there just to improve performance and binary size in these builds.
    p->Emit(
        {
            {"type", DefaultInstanceType(generator->descriptor(), options_,
                                         /*split=*/true)},
            {"name", DefaultInstanceName(generator->descriptor(), options_,
                                         /*split=*/true)},
            {"default",
             [&] { generator->GenerateInitDefaultSplitInstance(p); }},
            {"class", absl::StrCat(ClassName(generator->descriptor()),
                                   "::Impl_::Split")},
        },
        R"cc(
          struct $type$ {
            PROTOBUF_CONSTEXPR $type$() : _instance{$default$} {}
            union {
              $class$ _instance;
            };
          };

          PROTOBUF_ATTRIBUTE_NO_DESTROY PROTOBUF_CONSTINIT$ dllexport_decl$
              PROTOBUF_ATTRIBUTE_INIT_PRIORITY1 const $type$ $name$;
        )cc");
  }

  generator->GenerateConstexprConstructor(p);

  if (IsFileDescriptorProto(file_, options_)) {
    p->Emit(
        {
            {"type", DefaultInstanceType(generator->descriptor(), options_)},
            {"name", DefaultInstanceName(generator->descriptor(), options_)},
            {"class", ClassName(generator->descriptor())},
        },
        R"cc(
          struct $type$ {
#if defined(PROTOBUF_CONSTINIT_DEFAULT_INSTANCES)
            constexpr $type$() : _instance(::_pbi::ConstantInitialized{}) {}
#else   // defined(PROTOBUF_CONSTINIT_DEFAULT_INSTANCES)
            $type$() {}
            void Init() { ::new (&_instance) $class$(); };
#endif  // defined(PROTOBUF_CONSTINIT_DEFAULT_INSTANCES)
            ~$type$() {}
            union {
              $class$ _instance;
            };
          };

          PROTOBUF_ATTRIBUTE_NO_DESTROY PROTOBUF_CONSTINIT$ dllexport_decl$
              PROTOBUF_ATTRIBUTE_INIT_PRIORITY1 $type$ $name$;
        )cc");
  } else {
    p->Emit(
        {
            {"type", DefaultInstanceType(generator->descriptor(), options_)},
            {"name", DefaultInstanceName(generator->descriptor(), options_)},
            {"class", ClassName(generator->descriptor())},
        },
        R"cc(
          struct $type$ {
            PROTOBUF_CONSTEXPR $type$() : _instance(::_pbi::ConstantInitialized{}) {}
            ~$type$() {}
            union {
              $class$ _instance;
            };
          };

          PROTOBUF_ATTRIBUTE_NO_DESTROY PROTOBUF_CONSTINIT$ dllexport_decl$
              PROTOBUF_ATTRIBUTE_INIT_PRIORITY1 $type$ $name$;
        )cc");
  }

  for (int i = 0; i < generator->descriptor()->field_count(); ++i) {
    const FieldDescriptor* field = generator->descriptor()->field(i);
    if (!IsStringInlined(field, options_)) {
      continue;
    }

    // Force the initialization of the inlined string in the default instance.
    p->Emit(
        {
            {"class", ClassName(generator->descriptor())},
            {"field", FieldName(field)},
            {"default", DefaultInstanceName(generator->descriptor(), options_)},
            {"member", FieldMemberName(field, ShouldSplit(field, options_))},
        },
        R"cc(
          PROTOBUF_ATTRIBUTE_INIT_PRIORITY2 std::true_type
              $class$::Impl_::_init_inline_$field$_ =
                  ($default$._instance.$member$.Init(), std::true_type{});
        )cc");
  }

  if (options_.lite_implicit_weak_fields) {
    p->Emit(
        {
            {"ptr", DefaultInstancePtr(generator->descriptor(), options_)},
            {"name", DefaultInstanceName(generator->descriptor(), options_)},
        },
        R"cc(
          PROTOBUF_CONSTINIT const void* $ptr$ = &$name$;
        )cc");
  }
}

// A list of things defined in one .pb.cc file that we need to reference from
// another .pb.cc file.
struct FileGenerator::CrossFileReferences {
  // When we forward-declare things, we want to create a sorted order so our
  // output is deterministic and minimizes namespace changes.
  struct DescCompare {
    template <typename T>
    bool operator()(const T* const& a, const T* const& b) const {
      return a->full_name() < b->full_name();
    }

    bool operator()(const FileDescriptor* const& a,
                    const FileDescriptor* const& b) const {
      return a->name() < b->name();
    }
  };

  // Populated if we are referencing from messages or files.
  absl::btree_set<const Descriptor*, DescCompare> weak_default_instances;

  // Only if we are referencing from files.
  absl::btree_set<const FileDescriptor*, DescCompare> strong_reflection_files;
  absl::btree_set<const FileDescriptor*, DescCompare> weak_reflection_files;
};

void FileGenerator::GetCrossFileReferencesForField(const FieldDescriptor* field,
                                                   CrossFileReferences* refs) {
  const Descriptor* msg = field->message_type();
  if (msg == nullptr) {
    return;
  }

  if (IsImplicitWeakField(field, options_, &scc_analyzer_) ||
      IsWeak(field, options_)) {
    refs->weak_default_instances.insert(msg);
  }
}

void FileGenerator::GetCrossFileReferencesForFile(const FileDescriptor* file,
                                                  CrossFileReferences* refs) {
  ForEachField(file, [this, refs](const FieldDescriptor* field) {
    GetCrossFileReferencesForField(field, refs);
  });

  if (!HasDescriptorMethods(file, options_)) {
    return;
  }

  for (int i = 0; i < file->dependency_count(); ++i) {
    const FileDescriptor* dep = file->dependency(i);

    if (!ShouldSkipDependencyImports(file->dependency(i))) {
      refs->strong_reflection_files.insert(dep);
    } else if (IsDepWeak(dep)) {
      refs->weak_reflection_files.insert(dep);
    }
  }
}

// Generates references to variables defined in other files.
void FileGenerator::GenerateInternalForwardDeclarations(
    const CrossFileReferences& refs, io::Printer* p) {
  {
    NamespaceOpener ns(p);
    for (auto instance : refs.weak_default_instances) {
      ns.ChangeTo(Namespace(instance, options_));

      if (options_.lite_implicit_weak_fields) {
        p->Emit({{"ptr", DefaultInstancePtr(instance, options_)}}, R"cc(
          PROTOBUF_CONSTINIT __attribute__((weak)) const void* $ptr$ =
              &::_pbi::implicit_weak_message_default_instance;
        )cc");
      } else {
        p->Emit({{"type", DefaultInstanceType(instance, options_)},
                 {"name", DefaultInstanceName(instance, options_)}},
                R"cc(
                  extern __attribute__((weak)) $type$ $name$;
                )cc");
      }
    }
  }

  for (auto file : refs.weak_reflection_files) {
    p->Emit({{"table", DescriptorTableName(file, options_)}}, R"cc(
      extern __attribute__((weak)) const ::_pbi::DescriptorTable $table$;
    )cc");
  }
}

void FileGenerator::GenerateSourceForMessage(int idx, io::Printer* p) {
  auto v = p->WithVars(FileVars(file_, options_));

  GenerateSourceIncludes(p);
  GenerateSourcePrelude(p);

  if (IsAnyMessage(file_, options_)) {
    MuteWuninitialized(p);
  }

  CrossFileReferences refs;
  ForEachField(message_generators_[idx]->descriptor(),
               [this, &refs](const FieldDescriptor* field) {
                 GetCrossFileReferencesForField(field, &refs);
               });

  GenerateInternalForwardDeclarations(refs, p);

  {
    NamespaceOpener ns(Namespace(file_, options_), p);
    p->Emit(
        {
            {"defaults", [&] { GenerateSourceDefaultInstance(idx, p); }},
            {"class_methods",
             [&] { message_generators_[idx]->GenerateClassMethods(p); }},
        },
        R"cc(
          $defaults$;

          $class_methods$;

          // @@protoc_insertion_point(namespace_scope)
        )cc");
  }

  {
    NamespaceOpener proto_ns(ProtobufNamespace(options_), p);
    message_generators_[idx]->GenerateSourceInProto2Namespace(p);
  }

  if (IsAnyMessage(file_, options_)) {
    UnmuteWuninitialized(p);
  }

  p->Emit(R"cc(
    // @@protoc_insertion_point(global_scope)
  )cc");
}

void FileGenerator::GenerateSourceForExtension(int idx, io::Printer* p) {
  auto v = p->WithVars(FileVars(file_, options_));
  GenerateSourceIncludes(p);
  GenerateSourcePrelude(p);

  NamespaceOpener ns(Namespace(file_, options_), p);
  extension_generators_[idx]->GenerateDefinition(p);
}

void FileGenerator::GenerateGlobalSource(io::Printer* p) {
  auto v = p->WithVars(FileVars(file_, options_));
  GenerateSourceIncludes(p);
  GenerateSourcePrelude(p);

  {
    // Define the code to initialize reflection. This code uses a global
    // constructor to register reflection data with the runtime pre-main.
    if (HasDescriptorMethods(file_, options_)) {
      GenerateReflectionInitializationCode(p);
    }
  }

  NamespaceOpener ns(Namespace(file_, options_), p);
  for (int i = 0; i < enum_generators_.size(); ++i) {
    enum_generators_[i]->GenerateMethods(i, p);
  }
}

void FileGenerator::GenerateSource(io::Printer* p) {
  auto v = p->WithVars(FileVars(file_, options_));

  GenerateSourceIncludes(p);
  GenerateSourcePrelude(p);
  CrossFileReferences refs;
  GetCrossFileReferencesForFile(file_, &refs);
  GenerateInternalForwardDeclarations(refs, p);

  if (IsAnyMessage(file_, options_)) {
    MuteWuninitialized(p);
  }

  {
    NamespaceOpener ns(Namespace(file_, options_), p);
    for (int i = 0; i < message_generators_.size(); ++i) {
      GenerateSourceDefaultInstance(i, p);
    }
  }

  {
    if (HasDescriptorMethods(file_, options_)) {
      // Define the code to initialize reflection. This code uses a global
      // constructor to register reflection data with the runtime pre-main.
      GenerateReflectionInitializationCode(p);
    }
  }

  {
    NamespaceOpener ns(Namespace(file_, options_), p);

    // Actually implement the protos

    // Generate enums.
    for (int i = 0; i < enum_generators_.size(); ++i) {
      enum_generators_[i]->GenerateMethods(i, p);
    }

    // Generate classes.
    for (int i = 0; i < message_generators_.size(); ++i) {
      p->Emit(R"(
        $hrule_thick$
      )");
      message_generators_[i]->GenerateClassMethods(p);
    }

    if (HasGenericServices(file_, options_)) {
      // Generate services.
      for (int i = 0; i < service_generators_.size(); ++i) {
        p->Emit(R"(
          $hrule_thick$
        )");
        service_generators_[i]->GenerateImplementation(p);
      }
    }

    // Define extensions.
    for (int i = 0; i < extension_generators_.size(); ++i) {
      extension_generators_[i]->GenerateDefinition(p);
    }

    p->Emit(R"cc(
      // @@protoc_insertion_point(namespace_scope)
    )cc");
  }

  {
    NamespaceOpener proto_ns(ProtobufNamespace(options_), p);
    for (int i = 0; i < message_generators_.size(); ++i) {
      message_generators_[i]->GenerateSourceInProto2Namespace(p);
    }
  }

  p->Emit(R"cc(
    // @@protoc_insertion_point(global_scope)
  )cc");

  if (IsAnyMessage(file_, options_)) {
    UnmuteWuninitialized(p);
  }

  IncludeFile("third_party/protobuf/port_undef.inc", p);
}

void FileGenerator::GenerateReflectionInitializationCode(io::Printer* p) {
  if (!message_generators_.empty()) {
    p->Emit({{"len", message_generators_.size()}}, R"cc(
      static ::_pb::Metadata $file_level_metadata$[$len$];
    )cc");
  }

  if (!enum_generators_.empty()) {
    p->Emit({{"len", enum_generators_.size()}}, R"cc(
      static const ::_pb::EnumDescriptor* $file_level_enum_descriptors$[$len$];
    )cc");
  } else {
    p->Emit(R"cc(
      static constexpr const ::_pb::EnumDescriptor**
          $file_level_enum_descriptors$ = nullptr;
    )cc");
  }

  if (HasGenericServices(file_, options_) && file_->service_count() > 0) {
    p->Emit({{"len", file_->service_count()}}, R"cc(
      static const ::_pb::ServiceDescriptor*
          $file_level_service_descriptors$[$len$];
    )cc");
  } else {
    p->Emit(R"cc(
      static constexpr const ::_pb::ServiceDescriptor**
          $file_level_service_descriptors$ = nullptr;
    )cc");
  }

  if (!message_generators_.empty()) {
    std::vector<std::pair<size_t, size_t>> offsets;
    offsets.reserve(message_generators_.size());

    p->Emit(
        {
            {"offsets",
             [&] {
               for (int i = 0; i < message_generators_.size(); ++i) {
                 offsets.push_back(message_generators_[i]->GenerateOffsets(p));
               }
             }},
            {"schemas",
             [&] {
               int offset = 0;
               for (int i = 0; i < message_generators_.size(); ++i) {
                 message_generators_[i]->GenerateSchema(p, offset,
                                                        offsets[i].second);
                 offset += offsets[i].first;
               }
             }},
            {"defaults",
             [&] {
               for (auto& gen : message_generators_) {
                 p->Emit(
                     {
                         {"ns", Namespace(gen->descriptor(), options_)},
                         {"class", ClassName(gen->descriptor())},
                     },
                     R"cc(
                       &$ns$::_$class$_default_instance_._instance,
                     )cc");
               }
             }},
        },
        R"cc(
          const ::uint32_t $tablename$::offsets[] PROTOBUF_SECTION_VARIABLE(
              protodesc_cold) = {
              $offsets$,
          };

          static const ::_pbi::MigrationSchema
              schemas[] PROTOBUF_SECTION_VARIABLE(protodesc_cold) = {
                  $schemas$,
          };

          static const ::_pb::Message* const file_default_instances[] = {
              $defaults$,
          };
        )cc");
  } else {
    // Ee still need these symbols to exist.
    //
    // MSVC doesn't like empty arrays, so we add a dummy.
    p->Emit(R"cc(
      const ::uint32_t $tablename$::offsets[1] = {};
      static constexpr ::_pbi::MigrationSchema* schemas = nullptr;
      static constexpr ::_pb::Message* const* file_default_instances = nullptr;
    )cc");
  }

  // ---------------------------------------------------------------

  // Embed the descriptor.  We simply serialize the entire
  // FileDescriptorProto/ and embed it as a string literal, which is parsed and
  // built into real descriptors at initialization time.

  FileDescriptorProto file_proto = StripSourceRetentionOptions(*file_);
  std::string file_data;
  file_proto.SerializeToString(&file_data);

  auto desc_name = UniqueName("descriptor_table_protodef", file_, options_);
  p->Emit(
      {{"desc_name", desc_name},
       {"encoded_file_proto",
        [&] {
          if (options_.strip_nonfunctional_codegen) {
            p->Emit(R"cc("")cc");
            return;
          }

          absl::string_view data = file_data;
          if (data.size() <= 65535) {
            static constexpr size_t kBytesPerLine = 40;
            while (!data.empty()) {
              auto to_write = std::min(kBytesPerLine, data.size());
              auto chunk = data.substr(0, to_write);
              data = data.substr(to_write);

              p->Emit({{"text", EscapeTrigraphs(absl::CEscape(chunk))}}, R"cc(
                "$text$"
              )cc");
            }
            return;
          }

          // Workaround for MSVC: "Error C1091: compiler limit: string exceeds
          // 65535 bytes in length". Declare a static array of chars rather than
          // use a string literal. Only write 25 bytes per line.
          static constexpr size_t kBytesPerLine = 25;
          while (!data.empty()) {
            auto to_write = std::min(kBytesPerLine, data.size());
            auto chunk = data.substr(0, to_write);
            data = data.substr(to_write);

            std::string line;
            for (char c : chunk) {
              absl::StrAppend(&line, "'",
                              absl::CEscape(absl::string_view(&c, 1)), "', ");
            }

            p->Emit({{"line", line}}, R"cc(
              $line$
            )cc");
          }
        }}},
      R"cc(
        const char $desc_name$[] PROTOBUF_SECTION_VARIABLE(protodesc_cold) = {
            $encoded_file_proto$,
        };
      )cc");

  CrossFileReferences refs;
  GetCrossFileReferencesForFile(file_, &refs);
  size_t num_deps =
      refs.strong_reflection_files.size() + refs.weak_reflection_files.size();

  // Build array of DescriptorTable deps.
  if (num_deps > 0) {
    p->Emit(
        {
            {"len", num_deps},
            {"deps",
             [&] {
               for (auto dep : refs.strong_reflection_files) {
                 p->Emit({{"name", DescriptorTableName(dep, options_)}}, R"cc(
                   &::$name$,
                 )cc");
               }
               for (auto dep : refs.weak_reflection_files) {
                 p->Emit({{"name", DescriptorTableName(dep, options_)}}, R"cc(
                   &::$name$,
                 )cc");
               }
             }},
        },
        R"cc(
          static const ::_pbi::DescriptorTable* const $desc_table$_deps[$len$] =
              {
                  $deps$,
          };
        )cc");
  }

  // The DescriptorTable itself.
  // Should be "bool eager = NeedsEagerDescriptorAssignment(file_, options_);"
  // however this might cause a tsan failure in superroot b/148382879,
  // so disable for now.
  bool eager = false;
  p->Emit(
      {
          {"eager", eager ? "true" : "false"},
          {"file_proto_len",
           options_.strip_nonfunctional_codegen ? 0 : file_data.size()},
          {"proto_name", desc_name},
          {"deps_ptr", num_deps == 0
                           ? "nullptr"
                           : absl::StrCat(p->LookupVar("desc_table"), "_deps")},
          {"num_deps", num_deps},
          {"num_msgs", message_generators_.size()},
          {"msgs_ptr", message_generators_.empty()
                           ? "nullptr"
                           : std::string(p->LookupVar("file_level_metadata"))},
      },
      R"cc(
        static ::absl::once_flag $desc_table$_once;
        const ::_pbi::DescriptorTable $desc_table$ = {
            false,
            $eager$,
            $file_proto_len$,
            $proto_name$,
            "$filename$",
            &$desc_table$_once,
            $deps_ptr$,
            $num_deps$,
            $num_msgs$,
            schemas,
            file_default_instances,
            $tablename$::offsets,
            $msgs_ptr$,
            $file_level_enum_descriptors$,
            $file_level_service_descriptors$,
        };

        // This function exists to be marked as weak.
        // It can significantly speed up compilation by breaking up LLVM's SCC
        // in the .pb.cc translation units. Large translation units see a
        // reduction of more than 35% of walltime for optimized builds. Without
        // the weak attribute all the messages in the file, including all the
        // vtables and everything they use become part of the same SCC through
        // a cycle like:
        // GetMetadata -> descriptor table -> default instances ->
        //   vtables -> GetMetadata
        // By adding a weak function here we break the connection from the
        // individual vtables back into the descriptor table.
        PROTOBUF_ATTRIBUTE_WEAK const ::_pbi::DescriptorTable* $desc_table$_getter() {
          return &$desc_table$;
        }
      )cc");

  // For descriptor.proto and cpp_features.proto we want to avoid doing any
  // dynamic initialization, because in some situations that would otherwise
  // pull in a lot of unnecessary code that can't be stripped by --gc-sections.
  // Descriptor initialization will still be performed lazily when it's needed.
  if (!IsLazilyInitializedFile(file_->name())) {
    p->Emit({{"dummy", UniqueName("dynamic_init_dummy", file_, options_)}},
            R"cc(
              // Force running AddDescriptors() at dynamic initialization time.
              PROTOBUF_ATTRIBUTE_INIT_PRIORITY2
              static ::_pbi::AddDescriptorsRunner $dummy$(&$desc_table$);
            )cc");
  }

  // However, we must provide a way to force initialize the default instances
  // of FileDescriptorProto which will be used during registration of other
  // files.
  if (IsFileDescriptorProto(file_, options_)) {
    NamespaceOpener ns(p);
    ns.ChangeTo(absl::StrCat(ProtobufNamespace(options_), "::internal"));
    p->Emit(
        {{"dummy", UniqueName("dynamic_init_dummy", file_, options_)},
         {"initializers", absl::StrJoin(message_generators_, "\n",
                                        [&](std::string* out, const auto& gen) {
                                          absl::StrAppend(
                                              out,
                                              DefaultInstanceName(
                                                  gen->descriptor(), options_),
                                              ".Init();");
                                        })}},
        R"cc(
          //~ Emit wants an indented line, so give it a comment to strip.
#if !defined(PROTOBUF_CONSTINIT_DEFAULT_INSTANCES)
          PROTOBUF_EXPORT void InitializeFileDescriptorDefaultInstancesSlow() {
            $initializers$;
          }
          PROTOBUF_ATTRIBUTE_INIT_PRIORITY1
          static std::true_type $dummy${
              (InitializeFileDescriptorDefaultInstances(), std::true_type{})};
#endif  // !defined(PROTOBUF_CONSTINIT_DEFAULT_INSTANCES)
        )cc");
  }
}

class FileGenerator::ForwardDeclarations {
 public:
  void AddMessage(const Descriptor* d) { classes_.emplace(ClassName(d), d); }
  void AddEnum(const EnumDescriptor* d) { enums_.emplace(ClassName(d), d); }
  void AddSplit(const Descriptor* d) { splits_.emplace(ClassName(d), d); }

  void Print(io::Printer* p, const Options& options) const {
    for (const auto& e : enums_) {
      p->Emit({Sub("enum", e.first).AnnotatedAs(e.second)}, R"cc(
        enum $enum$ : int;
        bool $enum$_IsValid(int value);
      )cc");
    }

    for (const auto& c : classes_) {
      const Descriptor* desc = c.second;
      p->Emit(
          {
              Sub("class", c.first).AnnotatedAs(desc),
              {"default_type", DefaultInstanceType(desc, options)},
              {"default_name", DefaultInstanceName(desc, options)},
          },
          R"cc(
            class $class$;
            struct $default_type$;
            $dllexport_decl $extern $default_type$ $default_name$;
          )cc");
    }

    for (const auto& s : splits_) {
      const Descriptor* desc = s.second;
      p->Emit(
          {
              {"default_type",
               DefaultInstanceType(desc, options, /*split=*/true)},
              {"default_name",
               DefaultInstanceName(desc, options, /*split=*/true)},
          },
          R"cc(
            struct $default_type$;
            $dllexport_decl $extern const $default_type$ $default_name$;
          )cc");
    }
  }

  void PrintTopLevelDecl(io::Printer* p, const Options& options) const {
    if (ShouldGenerateExternSpecializations(options)) {
      for (const auto& c : classes_) {
        // To reduce total linker input size in large binaries we make these
        // functions extern and define then in the pb.cc file. This avoids bloat
        // in callers by having duplicate definitions of the template.
        // However, it increases the size of the pb.cc translation units so it
        // is a tradeoff.
        p->Emit({{"class", QualifiedClassName(c.second, options)}}, R"cc(
          template <>
          $dllexport_decl $$class$* Arena::CreateMaybeMessage<$class$>(Arena*);
        )cc");
      }
    }
  }

 private:
  absl::btree_map<std::string, const Descriptor*> classes_;
  absl::btree_map<std::string, const EnumDescriptor*> enums_;
  absl::btree_map<std::string, const Descriptor*> splits_;
};

static void PublicImportDFS(
    const FileDescriptor* fd,
    absl::flat_hash_set<const FileDescriptor*>& fd_set) {
  for (int i = 0; i < fd->public_dependency_count(); ++i) {
    const FileDescriptor* dep = fd->public_dependency(i);
    if (fd_set.insert(dep).second) {
      PublicImportDFS(dep, fd_set);
    }
  }
}

void FileGenerator::GenerateForwardDeclarations(io::Printer* p) {
  std::vector<const Descriptor*> classes;
  FlattenMessagesInFile(file_, &classes);  // All messages need forward decls.

  std::vector<const EnumDescriptor*> enums;
  if (options_.proto_h) {  // proto.h needs extra forward declarations.
    // All classes / enums referred to as field members
    std::vector<const FieldDescriptor*> fields;
    ListAllFields(file_, &fields);
    for (const auto* field : fields) {
      classes.push_back(field->containing_type());
      classes.push_back(field->message_type());
      enums.push_back(field->enum_type());
    }

    ListAllTypesForServices(file_, &classes);
  }

  // Calculate the set of files whose definitions we get through include.
  // No need to forward declare types that are defined in these.
  absl::flat_hash_set<const FileDescriptor*> public_set;
  PublicImportDFS(file_, public_set);

  absl::btree_map<std::string, ForwardDeclarations> decls;
  for (const auto* d : classes) {
    if (d != nullptr && !public_set.count(d->file()))
      decls[Namespace(d, options_)].AddMessage(d);
  }
  for (const auto* e : enums) {
    if (e != nullptr && !public_set.count(e->file()))
      decls[Namespace(e, options_)].AddEnum(e);
  }
  for (const auto& mg : message_generators_) {
    const Descriptor* d = mg->descriptor();
    if (d != nullptr && public_set.count(d->file()) == 0u &&
        ShouldSplit(mg->descriptor(), options_))
      decls[Namespace(d, options_)].AddSplit(d);
  }

  NamespaceOpener ns(p);
  for (const auto& decl : decls) {
    ns.ChangeTo(decl.first);
    decl.second.Print(p, options_);
  }

  ns.ChangeTo(ProtobufNamespace(options_));
  for (const auto& decl : decls) {
    decl.second.PrintTopLevelDecl(p, options_);
  }

  if (IsFileDescriptorProto(file_, options_)) {
    ns.ChangeTo(absl::StrCat(ProtobufNamespace(options_), "::internal"));
    p->Emit(R"cc(
      //~ Emit wants an indented line, so give it a comment to strip.
#if !defined(PROTOBUF_CONSTINIT_DEFAULT_INSTANCES)
      PROTOBUF_EXPORT void InitializeFileDescriptorDefaultInstancesSlow();
#endif  // !defined(PROTOBUF_CONSTINIT_DEFAULT_INSTANCES)
    )cc");
  }
}

void FileGenerator::GenerateLibraryIncludes(io::Printer* p) {
  if (UsingImplicitWeakFields(file_, options_)) {
    IncludeFile("third_party/protobuf/implicit_weak_message.h", p);
  }
  if (HasWeakFields(file_, options_)) {
    ABSL_CHECK(!options_.opensource_runtime);
    IncludeFile("third_party/protobuf/weak_field_map.h", p);
  }
  if (HasLazyFields(file_, options_, &scc_analyzer_)) {
    ABSL_CHECK(!options_.opensource_runtime);
    IncludeFile("third_party/protobuf/lazy_field.h", p);
  }
  if (ShouldVerify(file_, options_, &scc_analyzer_)) {
    IncludeFile("third_party/protobuf/wire_format_verify.h", p);
  }

  if (options_.opensource_runtime) {
    // Verify the protobuf library header version is compatible with the protoc
    // version before going any further.
    IncludeFile("third_party/protobuf/port_def.inc", p);
    p->Emit(
        {
            {"min_version", PROTOBUF_MIN_HEADER_VERSION_FOR_PROTOC},
            {"version", PROTOBUF_VERSION},
        },
        R"(
        #if PROTOBUF_VERSION < $min_version$
        #error "This file was generated by a newer version of protoc which is"
        #error "incompatible with your Protocol Buffer headers. Please update"
        #error "your headers."
        #endif  // PROTOBUF_VERSION

        #if $version$ < PROTOBUF_MIN_PROTOC_VERSION
        #error "This file was generated by an older version of protoc which is"
        #error "incompatible with your Protocol Buffer headers. Please"
        #error "regenerate this file with a newer version of protoc."
        #endif  // PROTOBUF_MIN_PROTOC_VERSION
    )");
    IncludeFile("third_party/protobuf/port_undef.inc", p);
  }

  // OK, it's now safe to #include other files.
  IncludeFile("third_party/protobuf/io/coded_stream.h", p);
  IncludeFile("third_party/protobuf/arena.h", p);
  IncludeFile("third_party/protobuf/arenastring.h", p);
  if ((options_.force_inline_string || options_.profile_driven_inline_string) &&
      !options_.opensource_runtime) {
    IncludeFile("third_party/protobuf/inlined_string_field.h", p);
  }
  if (HasSimpleBaseClasses(file_, options_)) {
    IncludeFile("third_party/protobuf/generated_message_bases.h", p);
  }
  if (HasGeneratedMethods(file_, options_) &&
      options_.tctable_mode != Options::kTCTableNever) {
    IncludeFile("third_party/protobuf/generated_message_tctable_decl.h", p);
  }
  IncludeFile("third_party/protobuf/generated_message_util.h", p);
  IncludeFile("third_party/protobuf/metadata_lite.h", p);

  if (HasDescriptorMethods(file_, options_)) {
    IncludeFile("third_party/protobuf/generated_message_reflection.h", p);
  }

  if (!message_generators_.empty()) {
    if (HasDescriptorMethods(file_, options_)) {
      IncludeFile("third_party/protobuf/message.h", p);
    } else {
      IncludeFile("third_party/protobuf/message_lite.h", p);
    }
  }
  if (options_.opensource_runtime) {
    // Open-source relies on unconditional includes of these.
    IncludeFileAndExport("third_party/protobuf/repeated_field.h", p);
    IncludeFileAndExport("third_party/protobuf/extension_set.h", p);
  } else {
    // Google3 includes these files only when they are necessary.
    if (HasExtensionsOrExtendableMessage(file_)) {
      IncludeFileAndExport("third_party/protobuf/extension_set.h", p);
    }
    if (HasRepeatedFields(file_)) {
      IncludeFileAndExport("third_party/protobuf/repeated_field.h", p);
    }
    if (HasStringPieceFields(file_, options_)) {
      IncludeFile("third_party/protobuf/string_piece_field_support.h", p);
    }
  }
  if (HasCordFields(file_, options_)) {
    p->Emit(R"(
      #include "absl/strings/cord.h"
      )");
  }
  if (HasMapFields(file_)) {
    IncludeFileAndExport("third_party/protobuf/map.h", p);
    if (HasDescriptorMethods(file_, options_)) {
      IncludeFile("third_party/protobuf/map_entry.h", p);
      IncludeFile("third_party/protobuf/map_field_inl.h", p);
    } else {
      IncludeFile("third_party/protobuf/map_entry_lite.h", p);
      IncludeFile("third_party/protobuf/map_field_lite.h", p);
    }
  }

  if (HasEnumDefinitions(file_)) {
    if (HasDescriptorMethods(file_, options_)) {
      IncludeFile("third_party/protobuf/generated_enum_reflection.h", p);
    } else {
      IncludeFile("third_party/protobuf/generated_enum_util.h", p);
    }
  }

  if (HasGenericServices(file_, options_)) {
    IncludeFile("third_party/protobuf/service.h", p);
  }

  if (UseUnknownFieldSet(file_, options_) && !message_generators_.empty()) {
    IncludeFile("third_party/protobuf/unknown_field_set.h", p);
  }
}

void FileGenerator::GenerateMetadataPragma(io::Printer* p,
                                           absl::string_view info_path) {
  if (info_path.empty() || options_.annotation_pragma_name.empty() ||
      options_.annotation_guard_name.empty()) {
    return;
  }

  p->Emit(
      {
          {"guard", options_.annotation_guard_name},
          {"pragma", options_.annotation_pragma_name},
          {"info_path", std::string(info_path)},
      },
      R"(
        #ifdef $guard$
        #pragma $pragma$ "$info_path$"
        #endif  // $guard$
      )");
}

void FileGenerator::GenerateDependencyIncludes(io::Printer* p) {
  for (int i = 0; i < file_->dependency_count(); ++i) {
    const FileDescriptor* dep = file_->dependency(i);

    if (ShouldSkipDependencyImports(dep)) {
      continue;
    }

    std::string basename = StripProto(dep->name());
    if (IsBootstrapProto(options_, file_)) {
      GetBootstrapBasename(options_, basename, &basename);
    }

    p->Emit(
        {{"name", CreateHeaderInclude(absl::StrCat(basename, ".pb.h"), dep)}},
        R"(
          #include $name$
        )");
  }
}

void FileGenerator::GenerateGlobalStateFunctionDeclarations(io::Printer* p) {
  // Forward-declare the DescriptorTable because this is referenced by .pb.cc
  // files depending on this file.
  //
  // The TableStruct is also outputted in weak_message_field.cc, because the
  // weak fields must refer to table struct but cannot include the header.
  // Also it annotates extra weak attributes.
  // TODO(gerbens) make sure this situation is handled better.
  p->Emit(R"cc(
    // Internal implementation detail -- do not use these members.
    struct $dllexport_decl $$tablename$ {
      static const ::uint32_t offsets[];
    };
  )cc");

  if (HasDescriptorMethods(file_, options_)) {
    p->Emit(R"cc(
      $dllexport_decl $extern const ::$proto_ns$::internal::DescriptorTable
          $desc_table$;
    )cc");
  }
}

void FileGenerator::GenerateMessageDefinitions(io::Printer* p) {
  for (int i = 0; i < message_generators_.size(); ++i) {
    p->Emit(R"cc(
      $hrule_thin$
    )cc");
    message_generators_[i]->GenerateClassDefinition(p);
  }
}

void FileGenerator::GenerateEnumDefinitions(io::Printer* p) {
  for (int i = 0; i < enum_generators_.size(); ++i) {
    enum_generators_[i]->GenerateDefinition(p);
  }
}

void FileGenerator::GenerateServiceDefinitions(io::Printer* p) {
  if (!HasGenericServices(file_, options_)) {
    return;
  }

  for (int i = 0; i < service_generators_.size(); ++i) {
    p->Emit(R"cc(
      $hrule_thin$
    )cc");
    service_generators_[i]->GenerateDeclarations(p);
  }

  p->Emit(R"cc(
    $hrule_thick$
  )cc");
}

void FileGenerator::GenerateExtensionIdentifiers(io::Printer* p) {
  // Declare extension identifiers. These are in global scope and so only
  // the global scope extensions.
  for (auto& extension_generator : extension_generators_) {
    if (extension_generator->IsScoped()) {
      continue;
    }
    extension_generator->GenerateDeclaration(p);
  }
}

void FileGenerator::GenerateInlineFunctionDefinitions(io::Printer* p) {
  // TODO(gerbens) remove pragmas when gcc is no longer used. Current version
  // of gcc fires a bogus error when compiled with strict-aliasing.
  p->Emit(R"(
      #ifdef __GNUC__
      #pragma GCC diagnostic push
      #pragma GCC diagnostic ignored "-Wstrict-aliasing"
      #endif  // __GNUC__
  )");

  for (int i = 0; i < message_generators_.size(); ++i) {
    p->Emit(R"cc(
      $hrule_thin$
    )cc");
    message_generators_[i]->GenerateInlineMethods(p);
  }

  p->Emit(R"(
      #ifdef __GNUC__
      #pragma GCC diagnostic pop
      #endif  // __GNUC__
  )");
}

void FileGenerator::GenerateProto2NamespaceEnumSpecializations(io::Printer* p) {
  // Emit GetEnumDescriptor specializations into google::protobuf namespace.
  if (!HasEnumDefinitions(file_)) {
    return;
  }

  p->PrintRaw("\n");
  NamespaceOpener ns(ProtobufNamespace(options_), p);
  p->PrintRaw("\n");
  for (auto& gen : enum_generators_) {
    gen->GenerateGetEnumDescriptorSpecializations(p);
  }
  p->PrintRaw("\n");
}
}  // namespace cpp
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
