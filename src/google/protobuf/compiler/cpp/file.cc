// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// Author: kenton@google.com (Kenton Varda)
//  Based on original Protocol Buffers design by
//  Sanjay Ghemawat, Jeff Dean, and others.

#include "google/protobuf/compiler/cpp/file.h"

#include <algorithm>
#include <cstddef>
#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "absl/container/btree_map.h"
#include "absl/container/btree_set.h"
#include "absl/container/flat_hash_map.h"
#include "absl/container/flat_hash_set.h"
#include "absl/log/absl_check.h"
#include "absl/strings/escaping.h"
#include "absl/strings/match.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_join.h"
#include "absl/strings/string_view.h"
#include "absl/strings/strip.h"
#include "google/protobuf/compiler/code_generator.h"
#include "google/protobuf/compiler/cpp/enum.h"
#include "google/protobuf/compiler/cpp/extension.h"
#include "google/protobuf/compiler/cpp/helpers.h"
#include "google/protobuf/compiler/cpp/message.h"
#include "google/protobuf/compiler/cpp/names.h"
#include "google/protobuf/compiler/cpp/options.h"
#include "google/protobuf/compiler/cpp/service.h"
#include "google/protobuf/compiler/retention.h"
#include "google/protobuf/compiler/versions.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/descriptor.pb.h"
#include "google/protobuf/dynamic_message.h"
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

// TODO: remove pragmas that suppresses uninitialized warnings when
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
      IsKnownFeatureProto(dep->name())) {
    return true;
  }

  return false;
}

FileGenerator::FileGenerator(const FileDescriptor* file, const Options& options)
    : file_(file), options_(options), scc_analyzer_(options) {
  std::vector<const Descriptor*> msgs = FlattenMessagesInFile(file);
  std::vector<const Descriptor*> msgs_topologically_ordered =
      TopologicalSortMessagesInFile(file, scc_analyzer_);
  ABSL_CHECK(msgs_topologically_ordered.size() == msgs.size())
      << "Size mismatch";

  for (size_t i = 0; i < msgs.size(); ++i) {
    message_generators_.push_back(std::make_unique<MessageGenerator>(
        msgs[i], variables_, i, options, &scc_analyzer_));
    message_generators_.back()->AddGenerators(&enum_generators_,
                                              &extension_generators_);
  }
  absl::flat_hash_map<const Descriptor*, int> msg_to_index;
  for (size_t i = 0; i < msgs.size(); ++i) {
    msg_to_index[msgs[i]] = i;
  }
  // Populate the topological order.
  for (size_t i = 0; i < msgs.size(); ++i) {
    auto it = msg_to_index.find(msgs_topologically_ordered[i]);
    ABSL_DCHECK(it != msg_to_index.end())
        << "Topological order has a message not present in the file!";
    message_generators_topologically_ordered_.push_back(it->second);
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
    for (size_t i = 0; i < service_generators_.size(); ++i) {
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
  p->Print(
      "// Generated by the protocol buffer compiler.  DO NOT EDIT!\n"
      "// NO CHECKED-IN PROTOBUF "
      "GENCODE\n"
      "// source: $filename$\n");
  if (options_.opensource_runtime) {
    p->Print("// Protobuf C++ Version: $protobuf_cpp_version$\n",
             "protobuf_cpp_version", PROTOBUF_CPP_VERSION_STRING);
  }
  p->Print("\n");
  p->Emit({{"cb", cb}, {"guard", guard}}, R"(
    #ifndef $guard$
    #define $guard$

    #include <limits>
    #include <string>
    #include <type_traits>
    #include <utility>

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
  p->Print(
      "// Generated by the protocol buffer compiler.  DO NOT EDIT!\n"
      "// NO CHECKED-IN PROTOBUF "
      "GENCODE\n"
      "// source: $filename$\n");
  if (options_.opensource_runtime) {
    p->Print("// Protobuf C++ Version: $protobuf_cpp_version$\n",
             "protobuf_cpp_version", PROTOBUF_CPP_VERSION_STRING);
  }
  p->Print("\n");
  p->Emit({{"h_include", CreateHeaderInclude(target_basename, file_)}},
          R"(
        #include $h_include$

        #include <algorithm>
        #include <type_traits>
      )");

  IncludeFile("third_party/protobuf/io/coded_stream.h", p);
  IncludeFile("third_party/protobuf/generated_message_tctable_impl.h", p);
  // TODO This is to include parse_context.h, we need a better way
  IncludeFile("third_party/protobuf/extension_set.h", p);
  IncludeFile("third_party/protobuf/wire_format_lite.h", p);

  if (ShouldVerify(file_, options_, &scc_analyzer_)) {
    IncludeFile("third_party/protobuf/wire_format_verify.h", p);
  }

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

  if (options_.proto_h) {
    // Use the smaller .proto.h files.
    for (int i = 0; i < file_->dependency_count(); ++i) {
      const FileDescriptor* dep = file_->dependency(i);

      if (ShouldSkipDependencyImports(dep)) continue;

      std::string basename = StripProto(dep->name());
      if (options_.bootstrap) {
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
    namespace _fl = ::$proto_ns$::internal::field_layout;
  )cc");
}

void FileGenerator::GenerateSourceDefaultInstance(int idx, io::Printer* p) {
  MessageGenerator* generator = message_generators_[idx].get();

  if (!ShouldGenerateClass(generator->descriptor(), options_)) return;

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
  } else if (UsingImplicitWeakDescriptor(file_, options_)) {
    p->Emit(
        {
            {"index", generator->index_in_file_messages()},
            {"type", DefaultInstanceType(generator->descriptor(), options_)},
            {"name", DefaultInstanceName(generator->descriptor(), options_)},
            {"class", ClassName(generator->descriptor())},
            {"section", WeakDefaultInstanceSection(
                            generator->descriptor(),
                            generator->index_in_file_messages(), options_)},
        },
        R"cc(
          struct $type$ {
            PROTOBUF_CONSTEXPR $type$() : _instance(::_pbi::ConstantInitialized{}) {}
            ~$type$() {}
            //~ _instance must be the first member.
            union {
              $class$ _instance;
            };
            ::_pbi::WeakDescriptorDefaultTail tail = {
                file_default_instances + $index$, sizeof($type$)};
          };

          PROTOBUF_ATTRIBUTE_NO_DESTROY PROTOBUF_CONSTINIT$ dllexport_decl$
              PROTOBUF_ATTRIBUTE_INIT_PRIORITY1 $type$ $name$
              __attribute__((section("$section$")));
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

  if (IsAnyMessage(file_)) {
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

  if (IsAnyMessage(file_)) {
    UnmuteWuninitialized(p);
  }

  p->Emit(R"cc(
    // @@protoc_insertion_point(global_scope)
  )cc");
}

void FileGenerator::GenerateStaticInitializer(io::Printer* p) {
  int priority = 0;
  for (auto& inits : static_initializers_) {
    ++priority;
    if (inits.empty()) continue;
    p->Emit(
        {{"priority", priority},
         {"expr",
          [&] {
            for (auto& init : inits) {
              init(p);
            }
          }}},
        R"cc(
          PROTOBUF_ATTRIBUTE_INIT_PRIORITY$priority$ static ::std::false_type
              _static_init$priority$_ PROTOBUF_UNUSED =
                  ($expr$, ::std::false_type{});
        )cc");
    // Reset the vector because we might be generating many files.
    inits.clear();
  }
}

void FileGenerator::GenerateSourceForExtension(int idx, io::Printer* p) {
  auto v = p->WithVars(FileVars(file_, options_));
  GenerateSourceIncludes(p);
  GenerateSourcePrelude(p);

  NamespaceOpener ns(Namespace(file_, options_), p);
  extension_generators_[idx]->GenerateDefinition(p);
  for (auto priority : {kInitPriority101, kInitPriority102}) {
    if (extension_generators_[idx]->WillGenerateRegistration(priority)) {
      static_initializers_[priority].push_back([this, idx, priority](auto* p) {
        extension_generators_[idx]->GenerateRegistration(p, priority);
      });
    }
  }
  GenerateStaticInitializer(p);
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
  for (size_t i = 0; i < enum_generators_.size(); ++i) {
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

  // When in weak descriptor mode, we generate the file_default_instances before
  // the default instances.
  if (UsingImplicitWeakDescriptor(file_, options_) &&
      !message_generators_.empty()) {
    p->Emit(
        {
            {"weak_defaults",
             [&] {
               for (auto& gen : message_generators_) {
                 p->Emit(
                     {
                         {"class", QualifiedClassName(gen->descriptor())},
                         {"section",
                          WeakDefaultInstanceSection(
                              gen->descriptor(), gen->index_in_file_messages(),
                              options_)},
                     },
                     R"cc(
                       extern const $class$ __start_$section$
                           __attribute__((weak));
                     )cc");
               }
             }},
            {"defaults",
             [&] {
               for (auto& gen : message_generators_) {
                 p->Emit({{"section",
                           WeakDefaultInstanceSection(
                               gen->descriptor(), gen->index_in_file_messages(),
                               options_)}},
                         R"cc(
                           &__start_$section$,
                         )cc");
               }
             }},
        },
        R"cc(
          $weak_defaults$;
          static const ::_pb::Message* file_default_instances[] = {
              $defaults$,
          };
        )cc");
  }


  if (IsAnyMessage(file_)) {
    MuteWuninitialized(p);
  }

  {
    NamespaceOpener ns(Namespace(file_, options_), p);
    for (size_t i = 0; i < message_generators_.size(); ++i) {
      GenerateSourceDefaultInstance(
          message_generators_topologically_ordered_[i], p);
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
    for (size_t i = 0; i < enum_generators_.size(); ++i) {
      enum_generators_[i]->GenerateMethods(i, p);
    }

    // Generate classes.
    for (size_t i = 0; i < message_generators_.size(); ++i) {
      p->Emit(R"(
        $hrule_thick$
      )");
      message_generators_[i]->GenerateClassMethods(p);
    }

    if (HasGenericServices(file_, options_)) {
      // Generate services.
      for (size_t i = 0; i < service_generators_.size(); ++i) {
        p->Emit(R"(
          $hrule_thick$
        )");
        service_generators_[i]->GenerateImplementation(p);
      }
    }

    // Define extensions.
    const auto is_lazily_init = IsLazilyInitializedFile(file_->name());
    for (size_t i = 0; i < extension_generators_.size(); ++i) {
      extension_generators_[i]->GenerateDefinition(p);
      if (!is_lazily_init) {
        for (auto priority : {kInitPriority101, kInitPriority102}) {
          if (extension_generators_[i]->WillGenerateRegistration(priority)) {
            static_initializers_[priority].push_back(
                [this, i, priority](auto* p) {
                  extension_generators_[i]->GenerateRegistration(p, priority);
                });
          }
        }
      }
    }

    p->Emit(R"cc(
      // @@protoc_insertion_point(namespace_scope)
    )cc");
  }

  {
    NamespaceOpener proto_ns(ProtobufNamespace(options_), p);
    for (size_t i = 0; i < message_generators_.size(); ++i) {
      message_generators_[i]->GenerateSourceInProto2Namespace(p);
    }
  }

  p->Emit(R"cc(
    // @@protoc_insertion_point(global_scope)
  )cc");

  if (IsAnyMessage(file_)) {
    UnmuteWuninitialized(p);
  }

  GenerateStaticInitializer(p);

  IncludeFile("third_party/protobuf/port_undef.inc", p);
}

static void GatherAllCustomOptionTypes(
    const FileDescriptor* file,
    absl::btree_map<absl::string_view, const Descriptor*>& out) {
  const DescriptorPool* pool = file->pool();
  const Descriptor* fd_proto_descriptor = pool->FindMessageTypeByName(
      FileDescriptorProto::descriptor()->full_name());
  // Not all pools have descriptor.proto in them. In these cases there for sure
  // are no custom options.
  if (fd_proto_descriptor == nullptr) {
    return;
  }

  // It's easier to inspect file as a proto, because we can use reflection on
  // the proto to iterate over all content.
  // However, we can't use the generated proto linked into the proto compiler
  // for this, since it doesn't know the extensions that are potentially present
  // the protos that are being compiled.
  // Use a dynamic one from the correct pool to parse them.
  DynamicMessageFactory factory(pool);
  std::unique_ptr<Message> fd_proto(
      factory.GetPrototype(fd_proto_descriptor)->New());

  {
    FileDescriptorProto linkedin_fd_proto;
    file->CopyTo(&linkedin_fd_proto);
    ABSL_CHECK(
        fd_proto->ParseFromString(linkedin_fd_proto.SerializeAsString()));
  }

  // Now find all the messages used, recursively.
  std::vector<const Message*> to_process = {fd_proto.get()};
  while (!to_process.empty()) {
    const Message& msg = *to_process.back();
    to_process.pop_back();

    std::vector<const FieldDescriptor*> fields;
    const Reflection& reflection = *msg.GetReflection();
    reflection.ListFields(msg, &fields);

    for (auto* field : fields) {
      if (field->is_extension()) {
        // Always add the extended.
        const Descriptor* desc = msg.GetDescriptor();
        out[desc->full_name()] = desc;
      }

      // Add and recurse of the extendee if it is a message.
      const auto* field_msg = field->message_type();
      if (field_msg == nullptr) continue;
      if (field->is_extension()) {
        out[field_msg->full_name()] = field_msg;
      }
      if (field->is_repeated()) {
        for (int i = 0; i < reflection.FieldSize(msg, field); i++) {
          to_process.push_back(&reflection.GetRepeatedMessage(msg, field, i));
        }
      } else {
        to_process.push_back(&reflection.GetMessage(msg, field));
      }
    }
  }
}

static std::vector<const Descriptor*>
GetMessagesToPinGloballyForWeakDescriptors(const FileDescriptor* file,
                                           const Options& options) {
  // Sorted map to dedup and to make deterministic.
  absl::btree_map<absl::string_view, const Descriptor*> res;

  // For simplicity we force pin request/response messages for all
  // services. The current implementation of services might not do
  // the pin itself, so it is simpler.
  // This is a place for improvement in the future.
  for (int i = 0; i < file->service_count(); ++i) {
    auto* service = file->service(i);
    for (int j = 0; j < service->method_count(); ++j) {
      auto* method = service->method(j);
      res[method->input_type()->full_name()] = method->input_type();
      res[method->output_type()->full_name()] = method->output_type();
    }
  }

  // For correctness, we must ensure that all messages used as custom options in
  // the descriptor are pinned. Otherwise, we can't properly parse the
  // descriptor.
  GatherAllCustomOptionTypes(file, res);

  std::vector<const Descriptor*> out;
  for (auto& p : res) {
    // We don't need to pin the bootstrap types. It is wasteful.
    if (IsBootstrapProto(options, p.second->file())) continue;
    out.push_back(p.second);
  }
  return out;
}

void FileGenerator::GenerateReflectionInitializationCode(io::Printer* p) {
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
               for (size_t i = 0; i < message_generators_.size(); ++i) {
                 offsets.push_back(message_generators_[i]->GenerateOffsets(p));
               }
             }},
            {"schemas",
             [&] {
               int offset = 0;
               for (size_t i = 0; i < message_generators_.size(); ++i) {
                 message_generators_[i]->GenerateSchema(p, offset,
                                                        offsets[i].second);
                 offset += offsets[i].first;
               }
             }},
        },
        R"cc(
          const ::uint32_t
              $tablename$::offsets[] ABSL_ATTRIBUTE_SECTION_VARIABLE(
                  protodesc_cold) = {
                  $offsets$,
          };

          static const ::_pbi::MigrationSchema
              schemas[] ABSL_ATTRIBUTE_SECTION_VARIABLE(protodesc_cold) = {
                  $schemas$,
          };
        )cc");
    if (!UsingImplicitWeakDescriptor(file_, options_)) {
      p->Emit({{"defaults",
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
                }}},
              R"cc(
                static const ::_pb::Message* const file_default_instances[] = {
                    $defaults$,
                };
              )cc");
    }
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
        const char $desc_name$[] ABSL_ATTRIBUTE_SECTION_VARIABLE(
            protodesc_cold) = {
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
      },
      R"cc(
        static ::absl::once_flag $desc_table$_once;
        PROTOBUF_CONSTINIT const ::_pbi::DescriptorTable $desc_table$ = {
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
            $file_level_enum_descriptors$,
            $file_level_service_descriptors$,
        };
      )cc");

  // For descriptor.proto and cpp_features.proto we want to avoid doing any
  // dynamic initialization, because in some situations that would otherwise
  // pull in a lot of unnecessary code that can't be stripped by --gc-sections.
  // Descriptor initialization will still be performed lazily when it's needed.
  if (!IsLazilyInitializedFile(file_->name())) {
    if (UsingImplicitWeakDescriptor(file_, options_)) {
      for (auto* pinned :
           GetMessagesToPinGloballyForWeakDescriptors(file_, options_)) {
        static_initializers_[kInitPriority102].push_back([this,
                                                          pinned](auto* p) {
          p->Emit({{"pin", StrongReferenceToType(pinned, options_)}},
                  R"cc(
                    $pin$,
                  )cc");
        });
      }
    }
    static_initializers_[kInitPriority102].push_back([](auto* p) {
      p->Emit(R"cc(
        ::_pbi::AddDescriptors(&$desc_table$),
      )cc");
    });
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
        if (!ShouldGenerateClass(c.second, options)) continue;
        // To reduce total linker input size in large binaries we make these
        // functions extern and define then in the pb.cc file. This avoids bloat
        // in callers by having duplicate definitions of the template.
        // However, it increases the size of the pb.cc translation units so it
        // is a tradeoff.
        p->Emit({{"class", QualifiedClassName(c.second, options)}}, R"cc(
          extern template void* Arena::DefaultConstruct<$class$>(Arena*);
        )cc");
        if (!IsMapEntryMessage(c.second)) {
          p->Emit({{"class", QualifiedClassName(c.second, options)}}, R"cc(
            extern template void* Arena::CopyConstruct<$class$>(Arena*,
                                                                const void*);
          )cc");
        }
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
    if (d != nullptr && !public_set.contains(d->file()) &&
        ShouldGenerateClass(d, options_))
      decls[Namespace(d, options_)].AddMessage(d);
  }
  for (const auto* e : enums) {
    if (e != nullptr && !public_set.contains(e->file()))
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

  IncludeFile("third_party/protobuf/runtime_version.h", p);
  int version;
  if (options_.opensource_runtime) {
    const auto& v = GetProtobufCPPVersion(/*oss_runtime=*/true);
    version = v.major() * 1000000 + v.minor() * 1000 + v.patch();
  } else {
    version = GetProtobufCPPVersion(/*oss_runtime=*/false).minor();
  }
  p->Emit(
      {
          {"version", version},

          // Downgrade to warnings if version mismatches for bootstrapped files,
          // so that release_compiler.h can build protoc_minimal successfully
          // and update stale files.
          {"err_level", options_.bootstrap ? "warning" : "error"},
      },
      R"(
    #if PROTOBUF_VERSION != $version$
    #$err_level$ "Protobuf C++ gencode is built with an incompatible version of"
    #$err_level$ "Protobuf C++ headers/runtime. See"
    #$err_level$ "https://protobuf.dev/support/cross-version-runtime-guarantee/#cpp"
    #endif
  )");

  // OK, it's now safe to #include other files.
  IncludeFile("third_party/protobuf/io/coded_stream.h", p);
  IncludeFile("third_party/protobuf/arena.h", p);
  IncludeFile("third_party/protobuf/arenastring.h", p);
  if (IsStringInliningEnabled(options_)) {
    IncludeFile("third_party/protobuf/inlined_string_field.h", p);
  }
  if (HasSimpleBaseClasses(file_, options_)) {
    IncludeFile("third_party/protobuf/generated_message_bases.h", p);
  }
  if (HasGeneratedMethods(file_, options_)) {
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
    if (options_.bootstrap) {
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
  // TODO make sure this situation is handled better.
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
  for (size_t i = 0; i < message_generators_.size(); ++i) {
    p->Emit(R"cc(
      $hrule_thin$
    )cc");
    message_generators_[message_generators_topologically_ordered_[i]]
        ->GenerateClassDefinition(p);
  }
}

void FileGenerator::GenerateEnumDefinitions(io::Printer* p) {
  for (size_t i = 0; i < enum_generators_.size(); ++i) {
    enum_generators_[i]->GenerateDefinition(p);
  }
}

void FileGenerator::GenerateServiceDefinitions(io::Printer* p) {
  if (!HasGenericServices(file_, options_)) {
    return;
  }

  for (size_t i = 0; i < service_generators_.size(); ++i) {
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
  // TODO remove pragmas when gcc is no longer used. Current version
  // of gcc fires a bogus error when compiled with strict-aliasing.
  p->Emit(R"(
      #ifdef __GNUC__
      #pragma GCC diagnostic push
      #pragma GCC diagnostic ignored "-Wstrict-aliasing"
      #endif  // __GNUC__
  )");

  for (size_t i = 0; i < message_generators_.size(); ++i) {
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

std::vector<const Descriptor*> FileGenerator::MessagesInTopologicalOrder()
    const {
  std::vector<const Descriptor*> descs;
  descs.reserve(message_generators_.size());
  for (size_t i = 0; i < message_generators_.size(); ++i) {
    descs.push_back(
        message_generators_[message_generators_topologically_ordered_[i]]
            ->descriptor());
  }
  return descs;
}
}  // namespace cpp
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
