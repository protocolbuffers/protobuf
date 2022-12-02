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

#include "google/protobuf/compiler/objectivec/file.h"

#include <algorithm>
#include <functional>
#include <iostream>
#include <iterator>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include "google/protobuf/compiler/code_generator.h"
#include "google/protobuf/descriptor.h"
#include "absl/container/btree_set.h"
#include "absl/container/flat_hash_map.h"
#include "absl/container/flat_hash_set.h"
#include "absl/strings/str_join.h"
#include "google/protobuf/compiler/objectivec/enum.h"
#include "google/protobuf/compiler/objectivec/extension.h"
#include "google/protobuf/compiler/objectivec/import_writer.h"
#include "google/protobuf/compiler/objectivec/message.h"
#include "google/protobuf/compiler/objectivec/names.h"
#include "google/protobuf/io/printer.h"

// NOTE: src/google/protobuf/compiler/plugin.cc makes use of cerr for some
// error cases, so it seems to be ok to use as a back door for errors.

namespace google {
namespace protobuf {
namespace compiler {
namespace objectivec {

namespace {

// This is also found in GPBBootstrap.h, and needs to be kept in sync.
const int32_t GOOGLE_PROTOBUF_OBJC_VERSION = 30005;

const char* kHeaderExtension = ".pbobjc.h";

// Checks if a message contains any extension definitions (on the message or
// a nested message under it).
bool MessageContainsExtensions(const Descriptor* message) {
  if (message->extension_count() > 0) {
    return true;
  }
  for (int i = 0; i < message->nested_type_count(); i++) {
    if (MessageContainsExtensions(message->nested_type(i))) {
      return true;
    }
  }
  return false;
}

// Checks if the file contains any extensions definitions (at the root or
// nested under a message).
bool FileContainsExtensions(const FileDescriptor* file) {
  if (file->extension_count() > 0) {
    return true;
  }
  for (int i = 0; i < file->message_type_count(); i++) {
    if (MessageContainsExtensions(file->message_type(i))) {
      return true;
    }
  }
  return false;
}

bool IsDirectDependency(const FileDescriptor* dep, const FileDescriptor* file) {
  for (int i = 0; i < file->dependency_count(); i++) {
    if (dep == file->dependency(i)) {
      return true;
    }
  }
  return false;
}

struct FileDescriptorsOrderedByName {
  inline bool operator()(const FileDescriptor* a,
                         const FileDescriptor* b) const {
    return a->name() < b->name();
  }
};

void MakeDescriptors(
    const Descriptor* descriptor, const std::string& root_classname,
    std::vector<std::unique_ptr<EnumGenerator>>* enum_generators,
    std::vector<std::unique_ptr<ExtensionGenerator>>* extension_generators,
    std::vector<std::unique_ptr<MessageGenerator>>* message_generators) {
  for (int i = 0; i < descriptor->enum_type_count(); i++) {
    enum_generators->emplace_back(
        std::make_unique<EnumGenerator>(descriptor->enum_type(i)));
  }
  for (int i = 0; i < descriptor->nested_type_count(); i++) {
    message_generators->emplace_back(std::make_unique<MessageGenerator>(
        root_classname, descriptor->nested_type(i)));
    message_generators->back()->AddExtensionGenerators(extension_generators);
    MakeDescriptors(descriptor->nested_type(i), root_classname, enum_generators,
                    extension_generators, message_generators);
  }
}

}  // namespace

const FileGenerator::CommonState::MinDepsEntry&
FileGenerator::CommonState::CollectMinimalFileDepsContainingExtensionsInternal(
    const FileDescriptor* file) {
  auto it = deps_info_cache.find(file);
  if (it != deps_info_cache.end()) {
    return it->second;
  }

  absl::flat_hash_set<const FileDescriptor*> min_deps_collector;
  absl::flat_hash_set<const FileDescriptor*> transitive_deps_collector;
  absl::flat_hash_set<const FileDescriptor*> to_prune;
  for (int i = 0; i < file->dependency_count(); i++) {
    const FileDescriptor* dep = file->dependency(i);
    MinDepsEntry dep_info =
        CollectMinimalFileDepsContainingExtensionsInternal(dep);

    // Everything the dep covered, this file will also cover.
    transitive_deps_collector.insert(dep_info.transitive_deps.begin(),
                                     dep_info.transitive_deps.end());
    // Prune everything from the dep's covered list in case another dep lists it
    // as a min dep.
    to_prune.insert(dep_info.transitive_deps.begin(),
                    dep_info.transitive_deps.end());

    // Does the dep have any extensions...
    if (dep_info.has_extensions) {
      // Yes -> Add this file, prune its min_deps and add them to the covered
      // deps.
      min_deps_collector.insert(dep);
      to_prune.insert(dep_info.min_deps.begin(), dep_info.min_deps.end());
      transitive_deps_collector.insert(dep_info.min_deps.begin(),
                                       dep_info.min_deps.end());
    } else {
      // No -> Just use its min_deps.
      min_deps_collector.insert(dep_info.min_deps.begin(),
                                dep_info.min_deps.end());
    }
  }

  const bool file_has_exts = FileContainsExtensions(file);

  // Fast path: if nothing to prune or there was only one dep, the prune work is
  // a waste, skip it.
  if (to_prune.empty() || file->dependency_count() == 1) {
    return deps_info_cache
        .insert(
            {file,
             {file_has_exts, min_deps_collector, transitive_deps_collector}})
        .first->second;
  }

  absl::flat_hash_set<const FileDescriptor*> min_deps;
  std::copy_if(min_deps_collector.begin(), min_deps_collector.end(),
               std::inserter(min_deps, min_deps.end()),
               [&](const FileDescriptor* value) {
                 return to_prune.find(value) == to_prune.end();
               });
  return deps_info_cache
      .insert({file, {file_has_exts, min_deps, transitive_deps_collector}})
      .first->second;
}

// Collect the deps of the given file that contain extensions. This can be used
// to create the chain of roots that need to be wired together.
//
// NOTE: If any changes are made to this and the supporting functions, you will
// need to manually validate what the generated code is for the test files:
//   objectivec/Tests/unittest_extension_chain_*.proto
// There are comments about what the expected code should be line and limited
// testing objectivec/Tests/GPBUnittestProtos2.m around compilation (#imports
// specifically).
std::vector<const FileDescriptor*>
FileGenerator::CommonState::CollectMinimalFileDepsContainingExtensions(
    const FileDescriptor* file) {
  absl::flat_hash_set<const FileDescriptor*> min_deps =
      CollectMinimalFileDepsContainingExtensionsInternal(file).min_deps;
  // Sort the list since pointer order isn't stable across runs.
  std::vector<const FileDescriptor*> result(min_deps.begin(), min_deps.end());
  std::sort(result.begin(), result.end(), FileDescriptorsOrderedByName());
  return result;
}

FileGenerator::FileGenerator(const FileDescriptor* file,
                             const GenerationOptions& generation_options,
                             CommonState& common_state)
    : file_(file),
      generation_options_(generation_options),
      common_state_(&common_state),
      root_class_name_(FileClassName(file)),
      is_bundled_proto_(IsProtobufLibraryBundledProtoFile(file)) {
  for (int i = 0; i < file_->enum_type_count(); i++) {
    enum_generators_.emplace_back(
        std::make_unique<EnumGenerator>(file_->enum_type(i)));
  }
  for (int i = 0; i < file_->extension_count(); i++) {
    extension_generators_.push_back(std::make_unique<ExtensionGenerator>(
        root_class_name_, file_->extension(i)));
  }
  for (int i = 0; i < file_->message_type_count(); i++) {
    message_generators_.emplace_back(std::make_unique<MessageGenerator>(
        root_class_name_, file_->message_type(i)));
    message_generators_.back()->AddExtensionGenerators(&extension_generators_);
    MakeDescriptors(file_->message_type(i), root_class_name_, &enum_generators_,
                    &extension_generators_, &message_generators_);
  }
}

void FileGenerator::GenerateHeader(io::Printer* p) const {
  GenerateFile(p, GeneratedFileType::kHeader, [&] {
    p->Emit(
        {
            {"fwd_decls",
             [&] {
               absl::btree_set<std::string> fwd_decls;
               for (const auto& generator : message_generators_) {
                 generator->DetermineForwardDeclarations(
                     &fwd_decls,
                     /* include_external_types = */
                     HeadersUseForwardDeclarations());
               }

               if (!fwd_decls.empty()) {
                 p->Emit({{"decls", absl::StrJoin(fwd_decls, "\n")}},
                         R"objc(
                          $decls$

                         )objc");
               }
             }},
            {"enums",
             [&] {
               for (const auto& generator : enum_generators_) {
                 generator->GenerateHeader(p);
               }
             }},
            {"root_extensions",
             [&] {
               // The dynamic methods block is only needed if there are
               // extensions that are file level scoped (not message scoped).
               // The first file_->extension_count() of extension_generators_
               // are the file scoped ones.
               if (file_->extension_count()) {
                 p->Emit(
                     {
                         {"extension_methods",
                          [&] {
                            for (int i = 0; i < file_->extension_count(); i++) {
                              extension_generators_[i]->GenerateMembersHeader(
                                  p);
                            }
                          }},
                     },
                     R"objc(
                       @interface $root_class_name$ (DynamicMethods)
                       $extension_methods$;
                      @end

                     )objc");
               }  // file_->extension_count()
             }},
            {"messages",
             [&] {
               for (const auto& generator : message_generators_) {
                 generator->GenerateMessageHeader(p);
               }
             }},
        },
        R"objc(
          CF_EXTERN_C_BEGIN

          $fwd_decls$;
          NS_ASSUME_NONNULL_BEGIN

          $enums$;
          #pragma mark - $root_class_name$

          /**
           * Exposes the extension registry for this file.
           *
           * The base class provides:
           * @code
           *   + (GPBExtensionRegistry *)extensionRegistry;
           * @endcode
           * which is a @c GPBExtensionRegistry that includes all the extensions defined by
           * this file and all files that it depends on.
           **/
          GPB_FINAL @interface $root_class_name$ : GPBRootObject
          @end

          $root_extensions$;
          $messages$;
          NS_ASSUME_NONNULL_END

          CF_EXTERN_C_END
        )objc");
  });
}

void FileGenerator::GenerateSource(io::Printer* p) const {
  std::vector<const FileDescriptor*> deps_with_extensions =
      common_state_->CollectMinimalFileDepsContainingExtensions(file_);

  // If any indirect dependency provided extensions, it needs to be directly
  // imported so it can get merged into the root's extensions registry.
  // See the Note by CollectMinimalFileDepsContainingExtensions before
  // changing this.
  std::vector<const FileDescriptor*> extra_files;
  for (auto& dep : deps_with_extensions) {
    if (!IsDirectDependency(dep, file_)) {
      extra_files.push_back(dep);
    }
  }

  absl::btree_set<std::string> fwd_decls;
  for (const auto& generator : message_generators_) {
    generator->DetermineObjectiveCClassDefinitions(&fwd_decls);
  }
  for (const auto& generator : extension_generators_) {
    generator->DetermineObjectiveCClassDefinitions(&fwd_decls);
  }

  std::vector<std::string> ignored_warnings;
  // The generated code for oneof's uses direct ivar access, suppress the
  // warning in case developer turn that on in the context they compile the
  // generated code.
  for (const auto& generator : message_generators_) {
    if (generator->IncludesOneOfDefinition()) {
      ignored_warnings.push_back("direct-ivar-access");
      break;
    }
  }
  if (!fwd_decls.empty()) {
    ignored_warnings.push_back("dollar-in-identifier-extension");
  }

  GenerateFile(
      p, GeneratedFileType::kSource, ignored_warnings, extra_files, [&] {
        p->Emit(
            {
                {"fwd_decls",
                 [&] {
                   if (fwd_decls.empty()) {
                     return;
                   }
                   p->Emit({{"decls", absl::StrJoin(fwd_decls, "\n")}},
                           R"objc(
                             #pragma mark - Objective C Class declarations
                             // Forward declarations of Objective C classes that we can use as
                             // static values in struct initializers.
                             // We don't use [Foo class] because it is not a static value.
                             $decls$

                           )objc");
                 }},
                {"root_implementation",
                 [&] { EmitRootImplementation(p, deps_with_extensions); }},
                {"file_descriptor_implementation",
                 [&] { EmitFileDescriptorImplementation(p); }},
                {"enums_and_messages",
                 [&] {
                   for (const auto& generator : enum_generators_) {
                     generator->GenerateSource(p);
                   }
                   for (const auto& generator : message_generators_) {
                     generator->GenerateSource(p);
                   }
                 }},
            },
            R"objc(
              $fwd_decls$;
              $root_implementation$

              $file_descriptor_implementation$;
              $enums_and_messages$;
            )objc");
      });
}

void FileGenerator::GenerateFile(
    io::Printer* p, GeneratedFileType file_type,
    const std::vector<std::string>& ignored_warnings,
    const std::vector<const FileDescriptor*>& extra_files_to_import,
    std::function<void()> body) const {
  ImportWriter import_writer(
      generation_options_.generate_for_named_framework,
      generation_options_.named_framework_to_proto_path_mappings_path,
      generation_options_.runtime_import_prefix,
      /* for_bundled_proto = */ is_bundled_proto_);
  const std::string header_extension(kHeaderExtension);

  switch (file_type) {
    case GeneratedFileType::kHeader:
      // Generated files bundled with the library get minimal imports,
      // everything else gets the wrapper so everything is usable.
      if (is_bundled_proto_) {
        import_writer.AddRuntimeImport("GPBDescriptor.h");
        import_writer.AddRuntimeImport("GPBMessage.h");
        import_writer.AddRuntimeImport("GPBRootObject.h");
      } else {
        import_writer.AddRuntimeImport("GPBProtocolBuffers.h");
      }
      if (HeadersUseForwardDeclarations()) {
        // #import any headers for "public imports" in the proto file.
        for (int i = 0; i < file_->public_dependency_count(); i++) {
          import_writer.AddFile(file_->public_dependency(i), header_extension);
        }
      } else {
        for (int i = 0; i < file_->dependency_count(); i++) {
          import_writer.AddFile(file_->dependency(i), header_extension);
        }
      }
      break;
    case GeneratedFileType::kSource:
      import_writer.AddRuntimeImport("GPBProtocolBuffers_RuntimeSupport.h");
      import_writer.AddFile(file_, header_extension);
      if (HeadersUseForwardDeclarations()) {
        // #import the headers for anything that a plain dependency of this
        // proto file (that means they were just an include, not a "public"
        // include).
        absl::flat_hash_set<std::string> public_import_names;
        for (int i = 0; i < file_->public_dependency_count(); i++) {
          public_import_names.insert(file_->public_dependency(i)->name());
        }
        for (int i = 0; i < file_->dependency_count(); i++) {
          const FileDescriptor* dep = file_->dependency(i);
          if (!public_import_names.contains(dep->name())) {
            import_writer.AddFile(dep, header_extension);
          }
        }
      }
      break;
  }

  for (const auto& dep : extra_files_to_import) {
    import_writer.AddFile(dep, header_extension);
  }

  p->Emit(
      {
          // Avoid the directive within the string below being seen by the
          // tool.
          {"clangfmt", "clang-format"},
          {"filename", file_->name()},
          {"root_class_name", root_class_name_},
          {"source_check",
           [&] {
             if (file_type == GeneratedFileType::kHeader) {
               // Add some verification that the generated code matches the
               // source the code is being compiled with.
               //
               // NOTE: Where used, this captures the raw numeric values at the
               // time the generator was compiled, since that will be the
               // versions for the ObjC runtime at that time.  The constants in
               // the generated code will then get their values at at compile
               // time (so checking against the headers being used to compile).
               p->Emit({{"google_protobuf_objc_version",
                         GOOGLE_PROTOBUF_OBJC_VERSION}},
                       R"objc(
    #if GOOGLE_PROTOBUF_OBJC_VERSION < $google_protobuf_objc_version$
    #error This file was generated by a newer version of protoc which is incompatible with your Protocol Buffer library sources.
    #endif
    #if $google_protobuf_objc_version$ < GOOGLE_PROTOBUF_OBJC_MIN_SUPPORTED_VERSION
    #error This file was generated by an older version of protoc which is incompatible with your Protocol Buffer library sources.
    #endif

                       )objc");
             }
           }},
          {"runtime_imports",
           [&] {
             import_writer.EmitRuntimeImports(
                 p, /* default_cpp_symbol = */ !is_bundled_proto_);
           }},
          {"file_imports", [&] { import_writer.EmitFileImports(p); }},
          {"extra_imports",
           [&] {
             // Enum implementation uses atomic in the generated code, so add
             // the system import as needed.
             if (file_type == GeneratedFileType::kSource &&
                 !enum_generators_.empty()) {
               p->Emit("#import <stdatomic.h>\n\n");
             }
           }},
          {"extra_warnings",
           [&] {
             for (const auto& warning : ignored_warnings) {
               p->Emit({{"warning", warning}},
                       R"objc(
                        #pragma clang diagnostic ignored "-W$warning$"
                      )objc");
             }
           }},
          {"body", body},
      },
      R"objc(
        // Generated by the protocol buffer compiler.  DO NOT EDIT!
        // $clangfmt$ off
        // source: $filename$

        $runtime_imports$

        $source_check$;
        $extra_imports$;
        $file_imports$
        // @@protoc_insertion_point(imports)

        #pragma clang diagnostic push
        #pragma clang diagnostic ignored "-Wdeprecated-declarations"
        $extra_warnings$;

        $body$;

        #pragma clang diagnostic pop

        // @@protoc_insertion_point(global_scope)

        // $clangfmt$ on
  )objc");
}

void FileGenerator::EmitRootImplementation(
    io::Printer* p,
    const std::vector<const FileDescriptor*>& deps_with_extensions) const {
  p->Emit(
      {
          {"root_extension_registry",
           [&] {
             // If there were any extensions or this file has any dependencies,
             // output a registry to override to create the file specific
             // registry.
             if (extension_generators_.empty() &&
                 deps_with_extensions.empty()) {
               if (file_->dependency_count() == 0) {
                 p->Emit(R"objc(
                    // No extensions in the file and no imports, so no need to generate
                    // +extensionRegistry.
                  )objc");
               } else {
                 p->Emit(R"objc(
                    // No extensions in the file and none of the imports (direct or indirect)
                    // defined extensions, so no need to generate +extensionRegistry.
                  )objc");
               }
             } else {
               EmitRootExtensionRegistryImplementation(p, deps_with_extensions);
             }
           }},
      },
      R"objc(
        #pragma mark - $root_class_name$

        @implementation $root_class_name$

        $root_extension_registry$

        @end
      )objc");
}

void FileGenerator::EmitRootExtensionRegistryImplementation(
    io::Printer* p,
    const std::vector<const FileDescriptor*>& deps_with_extensions) const {
  p->Emit(
      {
          {"register_local_extensions",
           [&] {
             if (extension_generators_.empty()) {
               return;
             }
             p->Emit(R"objc(
                 static GPBExtensionDescription descriptions[] = {
                   $register_local_extensions_variable_blocks$
                 };
                 for (size_t i = 0; i < sizeof(descriptions) / sizeof(descriptions[0]); ++i) {
                   GPBExtensionDescriptor *extension =
                       [[GPBExtensionDescriptor alloc] initWithExtensionDescription:&descriptions[i]
                                                                      usesClassRefs:YES];
                   [registry addExtension:extension];
                   [self globallyRegisterExtension:extension];
                   [extension release];
                 }
               )objc");
           }},
          {"register_local_extensions_variable_blocks",
           [&] {
             for (const auto& generator : extension_generators_) {
               generator->GenerateStaticVariablesInitialization(p);
             }
           }},
          {"register_imports",
           [&] {
             if (deps_with_extensions.empty()) {
               p->Emit(R"objc(
                   // None of the imports (direct or indirect) defined extensions, so no need to add
                   // them to this registry.
                 )objc");
             } else {
               p->Emit(R"objc(
                   // Merge in the imports (direct or indirect) that defined extensions.
                 )objc");
               for (const auto& dep : deps_with_extensions) {
                 p->Emit({{"dependency", FileClassName(dep)}},
                         R"objc(
                           [registry addExtensions:[$dependency$ extensionRegistry]];
                         )objc");
               }
             }
           }},
      },
      R"objc(
        + (GPBExtensionRegistry*)extensionRegistry {
          // This is called by +initialize so there is no need to worry
          // about thread safety and initialization of registry.
          static GPBExtensionRegistry* registry = nil;
          if (!registry) {
            GPB_DEBUG_CHECK_RUNTIME_VERSIONS();
            registry = [[GPBExtensionRegistry alloc] init];
            $register_local_extensions$;
            $register_imports$
          }
          return registry;
        }
      )objc");
}

void FileGenerator::EmitFileDescriptorImplementation(io::Printer* p) const {
  // File descriptor only needed if there are messages to use it.
  if (message_generators_.empty()) {
    return;
  }

  std::string syntax;
  switch (file_->syntax()) {
    case FileDescriptor::SYNTAX_UNKNOWN:
      syntax = "GPBFileSyntaxUnknown";
      break;
    case FileDescriptor::SYNTAX_PROTO2:
      syntax = "GPBFileSyntaxProto2";
      break;
    case FileDescriptor::SYNTAX_PROTO3:
      syntax = "GPBFileSyntaxProto3";
      break;
  }
  const std::string objc_prefix(FileClassPrefix(file_));

  p->Emit(
      {
          {"package", file_->package()},
          {"objc_prefix", objc_prefix},
          {"objc_prefix_arg",
           [&] {
             if (!objc_prefix.empty()) {
               p->Emit(R"objc(
                   objcPrefix:@"$objc_prefix$"
                 )objc");
             }
           }},
          {"syntax", syntax},
      },
      R"objc(
        #pragma mark - $root_class_name$_FileDescriptor

        static GPBFileDescriptor *$root_class_name$_FileDescriptor(void) {
          // This is called by +initialize so there is no need to worry
          // about thread safety of the singleton.
          static GPBFileDescriptor *descriptor = NULL;
          if (!descriptor) {
            GPB_DEBUG_CHECK_RUNTIME_VERSIONS();
            descriptor = [[GPBFileDescriptor alloc] initWithPackage:@"$package$"
                                                         $objc_prefix_arg$;
                                                             syntax:$syntax$];
          }
          return descriptor;
        }

      )objc");
}

}  // namespace objectivec
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
