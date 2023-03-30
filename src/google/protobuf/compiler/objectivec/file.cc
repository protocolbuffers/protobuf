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

#include "absl/container/btree_set.h"
#include "absl/container/flat_hash_map.h"
#include "absl/container/flat_hash_set.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_join.h"
#include "google/protobuf/compiler/objectivec/enum.h"
#include "google/protobuf/compiler/objectivec/extension.h"
#include "google/protobuf/compiler/objectivec/import_writer.h"
#include "google/protobuf/compiler/objectivec/message.h"
#include "google/protobuf/compiler/objectivec/names.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/descriptor_legacy.h"
#include "google/protobuf/io/printer.h"

// NOTE: src/google/protobuf/compiler/plugin.cc makes use of cerr for some
// error cases, so it seems to be ok to use as a back door for errors.

namespace google {
namespace protobuf {
namespace compiler {
namespace objectivec {

namespace {

// This is also found in GPBBootstrap.h, and needs to be kept in sync.
const int32_t GOOGLE_PROTOBUF_OBJC_VERSION = 30007;

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
    const Descriptor* descriptor, const std::string& file_description_name,
    std::vector<std::unique_ptr<EnumGenerator>>* enum_generators,
    std::vector<std::unique_ptr<ExtensionGenerator>>* extension_generators,
    std::vector<std::unique_ptr<MessageGenerator>>* message_generators) {
  for (int i = 0; i < descriptor->enum_type_count(); i++) {
    enum_generators->emplace_back(
        std::make_unique<EnumGenerator>(descriptor->enum_type(i)));
  }
  for (int i = 0; i < descriptor->nested_type_count(); i++) {
    message_generators->emplace_back(std::make_unique<MessageGenerator>(
        file_description_name, descriptor->nested_type(i)));
    message_generators->back()->AddExtensionGenerators(extension_generators);
    MakeDescriptors(descriptor->nested_type(i), file_description_name,
                    enum_generators, extension_generators, message_generators);
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
               std::inserter(min_deps, min_deps.begin()),
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
      file_description_name_(FileClassName(file) + "_FileDescription"),
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
        file_description_name_, file_->message_type(i)));
    message_generators_.back()->AddExtensionGenerators(&extension_generators_);
    MakeDescriptors(file_->message_type(i), file_description_name_,
                    &enum_generators_, &extension_generators_,
                    &message_generators_);
  }
}

void FileGenerator::GenerateHeader(io::Printer* p) const {
  GenerateFile(p, GeneratedFileType::kHeader, [&] {
    p->Print("CF_EXTERN_C_BEGIN\n\n");

    absl::btree_set<std::string> fwd_decls;
    for (const auto& generator : message_generators_) {
      generator->DetermineForwardDeclarations(&fwd_decls,
                                              /* include_external_types = */
                                              HeadersUseForwardDeclarations());
    }
    if (!fwd_decls.empty()) {
      p->Print("$fwd_decls$\n\n", "fwd_decls", absl::StrJoin(fwd_decls, "\n"));
    }

    p->Print("NS_ASSUME_NONNULL_BEGIN\n\n");

    for (const auto& generator : enum_generators_) {
      generator->GenerateHeader(p);
    }

    // For extensions to chain together, the Root gets created even if there
    // are no extensions.
    p->Print(
        // clang-format off
        "#pragma mark - $root_class_name$\n"
        "\n"
        "/**\n"
        " * Exposes the extension registry for this file.\n"
        " *\n"
        " * The base class provides:\n"
        " * @code\n"
        " *   + (GPBExtensionRegistry *)extensionRegistry;\n"
        " * @endcode\n"
        " * which is a @c GPBExtensionRegistry that includes all the extensions defined by\n"
        " * this file and all files that it depends on.\n"
        " **/\n"
        "GPB_FINAL @interface $root_class_name$ : GPBRootObject\n"
        "@end\n"
        "\n",
        // clang-format on
        "root_class_name", root_class_name_);

    // The dynamic methods block is only needed if there are extensions that are
    // file level scoped (not message scoped). The first
    // file_->extension_count() of extension_generators_ are the file scoped
    // ones.
    if (file_->extension_count()) {
      p->Print("@interface $root_class_name$ (DynamicMethods)\n",
               "root_class_name", root_class_name_);

      for (int i = 0; i < file_->extension_count(); i++) {
        extension_generators_[i]->GenerateMembersHeader(p);
      }

      p->Print("@end\n\n");
    }  // file_->extension_count()

    for (const auto& generator : message_generators_) {
      generator->GenerateMessageHeader(p);
    }

    // clang-format off
    p->Print(
      "NS_ASSUME_NONNULL_END\n"
      "\n"
      "CF_EXTERN_C_END\n");
    // clang-format on
  });
}

void FileGenerator::GenerateSource(io::Printer* p) const {
  std::vector<const FileDescriptor*> deps_with_extensions =
      common_state_->CollectMinimalFileDepsContainingExtensions(file_);
  GeneratedFileOptions file_options;

  // If any indirect dependency provided extensions, it needs to be directly
  // imported so it can get merged into the root's extensions registry.
  // See the Note by CollectMinimalFileDepsContainingExtensions before
  // changing this.
  for (auto& dep : deps_with_extensions) {
    if (!IsDirectDependency(dep, file_)) {
      file_options.extra_files_to_import.push_back(dep);
    }
  }

  absl::btree_set<std::string> fwd_decls;
  for (const auto& generator : message_generators_) {
    generator->DetermineObjectiveCClassDefinitions(&fwd_decls);
  }
  for (const auto& generator : extension_generators_) {
    generator->DetermineObjectiveCClassDefinitions(&fwd_decls);
  }

  // The generated code for oneof's uses direct ivar access, suppress the
  // warning in case developer turn that on in the context they compile the
  // generated code.
  for (const auto& generator : message_generators_) {
    if (generator->IncludesOneOfDefinition()) {
      file_options.ignored_warnings.push_back("direct-ivar-access");
      break;
    }
  }
  if (!fwd_decls.empty()) {
    file_options.ignored_warnings.push_back("dollar-in-identifier-extension");
  }

  // Enum implementation uses atomic in the generated code, so add
  // the system import as needed.
  if (!enum_generators_.empty()) {
    file_options.extra_system_headers.push_back("stdatomic.h");
  }

  GenerateFile(p, GeneratedFileType::kSource, file_options, [&] {
    if (!fwd_decls.empty()) {
      p->Print(
          // clang-format off
          "#pragma mark - Objective-C Class declarations\n"
          "// Forward declarations of Objective-C classes that we can use as\n"
          "// static values in struct initializers.\n"
          "// We don't use [Foo class] because it is not a static value.\n"
          "$fwd_decls$\n"
          "\n",
          // clang-format on
          "fwd_decls", absl::StrJoin(fwd_decls, "\n"));
    }

    PrintRootImplementation(p, deps_with_extensions);
    PrintFileDescription(p);

    for (const auto& generator : enum_generators_) {
      generator->GenerateSource(p);
    }
    for (const auto& generator : message_generators_) {
      generator->GenerateSource(p);
    }
  });
}

void FileGenerator::GenerateGlobalSource(io::Printer* p) const {
  std::vector<const FileDescriptor*> deps_with_extensions =
      common_state_->CollectMinimalFileDepsContainingExtensions(file_);
  GeneratedFileOptions file_options;

  // If any indirect dependency provided extensions, it needs to be directly
  // imported so it can get merged into the root's extensions registry.
  // See the Note by CollectMinimalFileDepsContainingExtensions before
  // changing this.
  for (auto& dep : deps_with_extensions) {
    if (!IsDirectDependency(dep, file_)) {
      file_options.extra_files_to_import.push_back(dep);
    }
  }

  absl::btree_set<std::string> fwd_decls;
  for (const auto& generator : extension_generators_) {
    generator->DetermineObjectiveCClassDefinitions(&fwd_decls);
  }

  if (!fwd_decls.empty()) {
    file_options.ignored_warnings.push_back("dollar-in-identifier-extension");
  }

  GenerateFile(p, GeneratedFileType::kSource, file_options, [&] {
    if (!fwd_decls.empty()) {
      p->Print(
          // clang-format off
              "#pragma mark - Objective-C Class declarations\n"
              "// Forward declarations of Objective-C classes that we can use as\n"
              "// static values in struct initializers.\n"
              "// We don't use [Foo class] because it is not a static value.\n"
              "$fwd_decls$\n"
              "\n",
          // clang-format on
          "fwd_decls", absl::StrJoin(fwd_decls, "\n"));
    }

    PrintRootImplementation(p, deps_with_extensions);
  });
}

void FileGenerator::GenerateSourceForEnums(io::Printer* p) const {
  // Enum implementation uses atomic in the generated code.
  GeneratedFileOptions file_options;
  file_options.extra_system_headers.push_back("stdatomic.h");

  GenerateFile(p, GeneratedFileType::kSource, file_options, [&] {
    for (const auto& generator : enum_generators_) {
      generator->GenerateSource(p);
    }
  });
}

void FileGenerator::GenerateSourceForMessage(int idx, io::Printer* p) const {
  const auto& generator = message_generators_[idx];

  absl::btree_set<std::string> fwd_decls;
  generator->DetermineObjectiveCClassDefinitions(&fwd_decls);

  GeneratedFileOptions file_options;
  // The generated code for oneof's uses direct ivar access, suppress the
  // warning in case developer turn that on in the context they compile the
  // generated code.
  if (generator->IncludesOneOfDefinition()) {
    file_options.ignored_warnings.push_back("direct-ivar-access");
  }

  GenerateFile(p, GeneratedFileType::kSource, file_options, [&] {
    if (!fwd_decls.empty()) {
      p->Print(
          // clang-format off
          "#pragma mark - Objective-C Class declarations\n"
          "// Forward declarations of Objective-C classes that we can use as\n"
          "// static values in struct initializers.\n"
          "// We don't use [Foo class] because it is not a static value.\n"
          "$fwd_decls$\n"
          "\n",
          // clang-format on
          "fwd_decls", absl::StrJoin(fwd_decls, "\n"));
    }

    PrintFileDescription(p);
    generator->GenerateSource(p);
  });
}

void FileGenerator::GenerateFile(io::Printer* p, GeneratedFileType file_type,
                                 const GeneratedFileOptions& file_options,
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

  for (const auto& dep : file_options.extra_files_to_import) {
    import_writer.AddFile(dep, header_extension);
  }

  p->Print(
      "// Generated by the protocol buffer compiler.  DO NOT EDIT!\n"
      "// $clangfmt$ off\n"
      "// source: $filename$\n"
      "\n",
      "filename", file_->name(), "clangfmt", "clang-format");

  import_writer.PrintRuntimeImports(
      p, /* default_cpp_symbol = */ !is_bundled_proto_);

  p->Print("\n");

  // Add some verification that the generated code matches the source the
  // code is being compiled with.
  // NOTE: This captures the raw numeric values at the time the generator was
  // compiled, since that will be the versions for the ObjC runtime at that
  // time.  The constants in the generated code will then get their values at
  // compile time (so checking against the headers being used to compile).
  p->Print(
      // clang-format off
      "#if GOOGLE_PROTOBUF_OBJC_VERSION < $google_protobuf_objc_version$\n"
      "#error This file was generated by a newer version of protoc which is incompatible with your Protocol Buffer library sources.\n"
      "#endif\n"
      "#if $google_protobuf_objc_version$ < GOOGLE_PROTOBUF_OBJC_MIN_SUPPORTED_VERSION\n"
      "#error This file was generated by an older version of protoc which is incompatible with your Protocol Buffer library sources.\n"
      "#endif\n"
      "\n",
      // clang-format on
      "google_protobuf_objc_version",
      absl::StrCat(GOOGLE_PROTOBUF_OBJC_VERSION));

  if (!file_options.extra_system_headers.empty()) {
    for (const auto& system_header : file_options.extra_system_headers) {
      p->Print("#import <$header$>\n", "header", system_header);
    }
    p->Print("\n");
  }

  import_writer.PrintFileImports(p);

  // clang-format off
  p->Print(
      "// @@protoc_insertion_point(imports)\n"
      "\n"
      "#pragma clang diagnostic push\n"
      "#pragma clang diagnostic ignored \"-Wdeprecated-declarations\"\n");
  // clang-format on
  for (const auto& warning : file_options.ignored_warnings) {
    p->Print("#pragma clang diagnostic ignored \"-W$warning$\"\n", "warning",
             warning);
  }
  p->Print("\n");

  body();

  p->Print(
      "\n"
      "#pragma clang diagnostic pop\n"
      "\n"
      "// @@protoc_insertion_point(global_scope)\n"
      "\n"
      "// $clangfmt$ on\n",
      "clangfmt", "clang-format");
}

void FileGenerator::PrintRootImplementation(
    io::Printer* p,
    const std::vector<const FileDescriptor*>& deps_with_extensions) const {
  p->Print(
      // clang-format off
      "#pragma mark - $root_class_name$\n"
      "\n"
      "@implementation $root_class_name$\n"
      "\n",
      // clang-format on
      "root_class_name", root_class_name_);

  // If there were any extensions or this file has any dependencies, output a
  // registry to override to create the file specific registry.
  if (extension_generators_.empty() && deps_with_extensions.empty()) {
    if (file_->dependency_count() == 0) {
      // clang-format off
      p->Print(
          "// No extensions in the file and no imports, so no need to generate\n"
          "// +extensionRegistry.\n");
      // clang-format on
    } else {
      // clang-format off
      p->Print(
          "// No extensions in the file and none of the imports (direct or indirect)\n"
          "// defined extensions, so no need to generate +extensionRegistry.\n");
      // clang-format on
    }
  } else {
    PrintRootExtensionRegistryImplementation(p, deps_with_extensions);
  }

  p->Print("\n@end\n\n");
}

void FileGenerator::PrintRootExtensionRegistryImplementation(
    io::Printer* p,
    const std::vector<const FileDescriptor*>& deps_with_extensions) const {
  // clang-format off
  p->Print(
      "+ (GPBExtensionRegistry*)extensionRegistry {\n"
      "  // This is called by +initialize so there is no need to worry\n"
      "  // about thread safety and initialization of registry.\n"
      "  static GPBExtensionRegistry* registry = nil;\n"
      "  if (!registry) {\n"
      "    GPB_DEBUG_CHECK_RUNTIME_VERSIONS();\n"
      "    registry = [[GPBExtensionRegistry alloc] init];\n");
  // clang-format on

  p->Indent();
  p->Indent();

  if (!extension_generators_.empty()) {
    p->Print("static GPBExtensionDescription descriptions[] = {\n");
    p->Indent();
    for (const auto& generator : extension_generators_) {
      generator->GenerateStaticVariablesInitialization(p);
    }
    p->Outdent();
    // clang-format off
    p->Print(
        "};\n"
        "for (size_t i = 0; i < sizeof(descriptions) / sizeof(descriptions[0]); ++i) {\n"
        "  GPBExtensionDescriptor *extension =\n"
        "      [[GPBExtensionDescriptor alloc] initWithExtensionDescription:&descriptions[i]\n"
        "                                                     usesClassRefs:YES];\n"
        "  [registry addExtension:extension];\n"
        "  [self globallyRegisterExtension:extension];\n"
        "  [extension release];\n"
        "}\n");
    // clang-format on
  }

  if (deps_with_extensions.empty()) {
    // clang-format off
    p->Print(
        "// None of the imports (direct or indirect) defined extensions, so no need to add\n"
        "// them to this registry.\n");
    // clang-format on
  } else {
    // clang-format off
    p->Print(
        "// Merge in the imports (direct or indirect) that defined extensions.\n");
    // clang-format on
    for (const auto& dep : deps_with_extensions) {
      const std::string root_class_name(FileClassName((dep)));
      p->Print("[registry addExtensions:[$dependency$ extensionRegistry]];\n",
               "dependency", root_class_name);
    }
  }

  p->Outdent();
  p->Outdent();

  // clang-format off
  p->Print(
      "  }\n"
      "  return registry;\n"
      "}\n");
  // clang-format on
}

void FileGenerator::PrintFileDescription(io::Printer* p) const {
  // File descriptor only needed if there are messages to use it.
  if (message_generators_.empty()) {
    return;
  }

  const std::string objc_prefix(FileClassPrefix(file_));
  absl::flat_hash_map<absl::string_view, std::string> vars;
  vars["file_description_name"] = file_description_name_;
  vars["package_value"] = file_->package().empty()
                              ? "NULL"
                              : absl::StrCat("\"", file_->package(), "\"");
  switch (FileDescriptorLegacy(file_).syntax()) {
    case FileDescriptorLegacy::Syntax::SYNTAX_UNKNOWN:
      vars["syntax"] = "GPBFileSyntaxUnknown";
      break;
    case FileDescriptorLegacy::Syntax::SYNTAX_PROTO2:
      vars["syntax"] = "GPBFileSyntaxProto2";
      break;
    case FileDescriptorLegacy::Syntax::SYNTAX_PROTO3:
      vars["syntax"] = "GPBFileSyntaxProto3";
      break;
  }
  if (objc_prefix.empty() && !file_->options().has_objc_class_prefix()) {
    vars["prefix_value"] = "NULL";
  } else {
    vars["prefix_value"] = absl::StrCat("\"", objc_prefix, "\"");
  }

  // clang-format off
  p->Print(
      vars,
      "static GPBFileDescription $file_description_name$ = {\n"
      "  .package = $package_value$,\n"
      "  .prefix = $prefix_value$,\n"
      "  .syntax = $syntax$\n"
      "};\n"
      "\n");
  // clang-format on
}

}  // namespace objectivec
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
