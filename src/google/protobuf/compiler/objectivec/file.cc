// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "google/protobuf/compiler/objectivec/file.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <iterator>
#include <memory>
#include <string>
#include <vector>

#include "absl/container/btree_set.h"
#include "absl/container/flat_hash_map.h"
#include "absl/container/flat_hash_set.h"
#include "absl/log/absl_check.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_join.h"
#include "absl/strings/string_view.h"
#include "google/protobuf/compiler/code_generator.h"
#include "google/protobuf/compiler/objectivec/enum.h"
#include "google/protobuf/compiler/objectivec/extension.h"
#include "google/protobuf/compiler/objectivec/helpers.h"
#include "google/protobuf/compiler/objectivec/import_writer.h"
#include "google/protobuf/compiler/objectivec/message.h"
#include "google/protobuf/compiler/objectivec/names.h"
#include "google/protobuf/compiler/objectivec/options.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/descriptor.pb.h"
#include "google/protobuf/io/printer.h"

namespace google {
namespace protobuf {
namespace compiler {
namespace objectivec {

namespace {

// This is also found in GPBBootstrap.h, and needs to be kept in sync.
const int32_t GOOGLE_PROTOBUF_OBJC_VERSION = 40311;

const char* kHeaderExtension = ".pbobjc.h";

bool IsMapEntryMessage(const Descriptor* descriptor) {
  return descriptor->options().map_entry();
}

// Checks if a message contains extension definitions (on the message or
// a nested message under it). `include_custom_options` decides if custom
// options count as extensions.
bool MessageContainsExtensions(const Descriptor* message,
                               bool include_custom_options) {
  if (message->extension_count() > 0) {
    if (include_custom_options) {
      return true;
    }
    for (int i = 0; i < message->extension_count(); i++) {
      if (!ExtensionIsCustomOption(message->extension(i))) {
        return true;
      }
    }
  }
  for (int i = 0; i < message->nested_type_count(); i++) {
    if (MessageContainsExtensions(message->nested_type(i),
                                  include_custom_options)) {
      return true;
    }
  }
  return false;
}

// Checks if the file contains extensions definitions (at the root or
// nested under a message). `include_custom_options` decides if custom
// options count as extensions.
bool FileContainsExtensions(const FileDescriptor* file,
                            bool include_custom_options) {
  if (file->extension_count() > 0) {
    if (include_custom_options) {
      return true;
    }
    for (int i = 0; i < file->extension_count(); i++) {
      if (!ExtensionIsCustomOption(file->extension(i))) {
        return true;
      }
    }
  }
  for (int i = 0; i < file->message_type_count(); i++) {
    if (MessageContainsExtensions(file->message_type(i),
                                  include_custom_options)) {
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
    std::vector<std::unique_ptr<MessageGenerator>>* message_generators,
    const GenerationOptions& generation_options) {
  for (int i = 0; i < descriptor->enum_type_count(); i++) {
    enum_generators->emplace_back(std::make_unique<EnumGenerator>(
        descriptor->enum_type(i), generation_options));
  }
  for (int i = 0; i < descriptor->nested_type_count(); i++) {
    const Descriptor* message_type = descriptor->nested_type(i);
    if (IsMapEntryMessage(message_type)) {
      // Map entries can't have extensions, or sub messages, they are an
      // implementation detail of how map<> works.
      continue;
    }
    message_generators->emplace_back(std::make_unique<MessageGenerator>(
        file_description_name, message_type, generation_options));
    message_generators->back()->AddExtensionGenerators(extension_generators);
    MakeDescriptors(message_type, file_description_name, enum_generators,
                    extension_generators, message_generators,
                    generation_options);
  }
}

void EmitLinkWKTs(absl::string_view name, io::Printer* p) {
  absl::string_view::size_type last_slash = name.rfind('/');
  std::string basename;
  if (last_slash == absl::string_view::npos) {
    basename = std::string(name);
  } else {
    basename = std::string(name.substr(last_slash + 1));
  }

  p->Emit({{"basename", StripProto(basename)}},
          R"objc(
            // This is to help make sure that the GPBWellKnownTypes.* categories get linked and
            // developers do not have to use the `-ObjC` linker flag. More information
            // here: https://medium.com/ios-os-x-development/categories-in-static-libraries-78e41f8ddb96
            __attribute__((used)) static NSString* $basename$_importCategories(void) {
              return GPBWellKnownTypesErrorDomain;
            }
          )objc");
  p->Emit("\n");
}

void EmitSourceFwdDecls(const absl::btree_set<std::string>& fwd_decls,
                        io::Printer* p) {
  if (fwd_decls.empty()) {
    return;
  }

  p->Emit({{"fwd_decls", absl::StrJoin(fwd_decls, "\n")}},
          R"objc(
            #pragma mark - Objective-C Class declarations
            // Forward declarations of Objective-C classes that we can use as
            // static values in struct initializers.
            // We don't use [Foo class] because it is not a static value.
            $fwd_decls$
          )objc");
  p->Emit("\n");
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

  const bool file_has_exts =
      FileContainsExtensions(file, include_custom_options);

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

FileGenerator::FileGenerator(Edition edition, const FileDescriptor* file,
                             const GenerationOptions& generation_options,
                             CommonState& common_state)
    : edition_(edition),
      file_(file),
      generation_options_(generation_options),
      common_state_(&common_state),
      root_class_name_(FileClassName(file)),
      file_description_name_(FileClassName(file) + "_FileDescription"),
      is_bundled_proto_(IsProtobufLibraryBundledProtoFile(file)) {
  for (int i = 0; i < file_->enum_type_count(); i++) {
    enum_generators_.emplace_back(std::make_unique<EnumGenerator>(
        file_->enum_type(i), generation_options));
  }
  for (int i = 0; i < file_->extension_count(); i++) {
    const FieldDescriptor* extension = file_->extension(i);
    if (!generation_options.strip_custom_options ||
        !ExtensionIsCustomOption(extension)) {
      extension_generators_.push_back(std::make_unique<ExtensionGenerator>(
          root_class_name_, extension, generation_options));
    }
  }
  file_scoped_extension_count_ = extension_generators_.size();
  for (int i = 0; i < file_->message_type_count(); i++) {
    const Descriptor* message_type = file_->message_type(i);
    if (IsMapEntryMessage(message_type)) {
      // Map entries can't have extensions, or sub messages, they are an
      // implementation detail of how map<> works.
      continue;
    }
    message_generators_.emplace_back(std::make_unique<MessageGenerator>(
        file_description_name_, message_type, generation_options));
    message_generators_.back()->AddExtensionGenerators(&extension_generators_);
    MakeDescriptors(message_type, file_description_name_, &enum_generators_,
                    &extension_generators_, &message_generators_,
                    generation_options);
  }
}

void FileGenerator::GenerateHeader(io::Printer* p,
                                   absl::string_view info_path) const {
  GenerateFile(p, GeneratedFileType::kHeader, [&] {
    absl::btree_set<std::string> fwd_decls;
    for (const auto& generator : message_generators_) {
      generator->DetermineForwardDeclarations(&fwd_decls,
                                              /* include_external_types = */
                                              HeadersUseForwardDeclarations());
    }

    p->Emit("CF_EXTERN_C_BEGIN\n\n");

    if (!fwd_decls.empty()) {
      p->Emit({{"fwd_decls", absl::StrJoin(fwd_decls, "\n")}},
              "$fwd_decls$\n\n");
    }

    p->Emit("NS_ASSUME_NONNULL_BEGIN\n\n");

    if (!info_path.empty()) {
      p->Emit({{"info_path", info_path},
               {"guard", generation_options_.annotation_guard_name},
               {"pragma", generation_options_.annotation_pragma_name}},
              R"objc(
                #ifdef $guard$
                #pragma $pragma$ "$info_path$"
                #endif  // $guard$
              )objc");
      p->Emit("\n");
    }

    for (const auto& generator : enum_generators_) {
      generator->GenerateHeader(p);
    }

    // For extensions to chain together, the Root gets created even if there
    // are no extensions.
    p->Emit(R"objc(
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
    )objc");
    p->Emit("\n");

    // The dynamic methods block is only needed if there are extensions that are
    // file level scoped (not message scoped). The first
    // file_scoped_extension_count_ of extension_generators_ are the file scoped
    // ones.
    if (file_scoped_extension_count_) {
      p->Emit("@interface $root_class_name$ (DynamicMethods)\n");

      for (size_t i = 0; i < file_scoped_extension_count_; i++) {
        extension_generators_[i]->GenerateMembersHeader(p);
      }

      p->Emit("@end\n\n");
    }

    for (const auto& generator : message_generators_) {
      generator->GenerateMessageHeader(p);
    }

    p->Emit(R"objc(
      NS_ASSUME_NONNULL_END

      CF_EXTERN_C_END
    )objc");
  });
}

void FileGenerator::GenerateSource(io::Printer* p) const {
  std::vector<const FileDescriptor*> deps_with_extensions =
      common_state_->CollectMinimalFileDepsContainingExtensions(file_);
  GeneratedFileOptions file_options;
  file_options.forced_files_to_import = deps_with_extensions;

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
    EmitSourceFwdDecls(fwd_decls, p);
    EmitRootImplementation(p, deps_with_extensions);
    EmitFileDescription(p);

    if (is_bundled_proto_ && HasWKTWithObjCCategory(file_)) {
      EmitLinkWKTs(file_->name(), p);
    }

    for (const auto& generator : enum_generators_) {
      generator->GenerateSource(p);
    }
    for (const auto& generator : message_generators_) {
      generator->GenerateSource(p);
    }
  });
}

void FileGenerator::GenerateGlobalSource(io::Printer* p) const {
  ABSL_CHECK(!is_bundled_proto_)
      << "Bundled protos aren't expected to use multi source generation.";
  std::vector<const FileDescriptor*> deps_with_extensions =
      common_state_->CollectMinimalFileDepsContainingExtensions(file_);
  GeneratedFileOptions file_options;
  file_options.forced_files_to_import = deps_with_extensions;

  absl::btree_set<std::string> fwd_decls;
  for (const auto& generator : extension_generators_) {
    generator->DetermineObjectiveCClassDefinitions(&fwd_decls);
  }

  if (!fwd_decls.empty()) {
    file_options.ignored_warnings.push_back("dollar-in-identifier-extension");
  }

  GenerateFile(p, GeneratedFileType::kSource, file_options, [&] {
    EmitSourceFwdDecls(fwd_decls, p);
    EmitRootImplementation(p, deps_with_extensions);
  });
}

void FileGenerator::GenerateSourceForEnums(io::Printer* p) const {
  ABSL_CHECK(!is_bundled_proto_)
      << "Bundled protos aren't expected to use multi source generation.";
  // Enum implementation uses atomic in the generated code.
  GeneratedFileOptions file_options;
  file_options.extra_system_headers.push_back("stdatomic.h");

  GenerateFile(p, GeneratedFileType::kSource, file_options, [&] {
    for (const auto& generator : enum_generators_) {
      generator->GenerateSource(p);
    }
  });
}

void FileGenerator::GenerateSourceForMessage(size_t idx, io::Printer* p) const {
  ABSL_CHECK(!is_bundled_proto_)
      << "Bundled protos aren't expected to use multi source generation.";
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
    EmitSourceFwdDecls(fwd_decls, p);
    EmitFileDescription(p);
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

  absl::flat_hash_set<const FileDescriptor*> file_imports;
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
          file_imports.insert(file_->public_dependency(i));
        }
      } else if (generation_options_.generate_minimal_imports) {
        DetermineNeededDeps(&file_imports, PublicDepsHandling::kForceInclude);
      } else {
        for (int i = 0; i < file_->dependency_count(); i++) {
          file_imports.insert(file_->dependency(i));
        }
      }
      break;
    case GeneratedFileType::kSource:
      import_writer.AddRuntimeImport("GPBProtocolBuffers_RuntimeSupport.h");
      if (is_bundled_proto_ && HasWKTWithObjCCategory(file_)) {
        import_writer.AddRuntimeImport("GPBWellKnownTypes.h");
      }
      import_writer.AddFile(file_, header_extension);
      if (HeadersUseForwardDeclarations()) {
        if (generation_options_.generate_minimal_imports) {
          DetermineNeededDeps(&file_imports, PublicDepsHandling::kExclude);
        } else {
          // #import the headers for anything that a plain dependency of this
          // proto file (that means they were just an include, not a "public"
          // include).
          absl::flat_hash_set<absl::string_view> public_import_names;
          for (int i = 0; i < file_->public_dependency_count(); i++) {
            public_import_names.insert(file_->public_dependency(i)->name());
          }
          for (int i = 0; i < file_->dependency_count(); i++) {
            const FileDescriptor* dep = file_->dependency(i);
            if (!public_import_names.contains(dep->name())) {
              file_imports.insert(dep);
            }
          }
        }
      }
      break;
  }

  // If a forced file was a direct dep, move it into the file_imports.
  std::vector<const FileDescriptor*> extra_files_to_import;
  for (const auto& dep : file_options.forced_files_to_import) {
    if (IsDirectDependency(dep, file_)) {
      file_imports.insert(dep);
    } else {
      extra_files_to_import.push_back(dep);
    }
  }

  if (!file_imports.empty()) {
    // Output the file_imports in the order they were listed as dependencies.
    for (int i = 0; i < file_->dependency_count(); i++) {
      const FileDescriptor* dep = file_->dependency(i);
      if (file_imports.contains(dep)) {
        import_writer.AddFile(file_->dependency(i), header_extension);
        file_imports.erase(dep);
      }
    }
    if (!file_imports.empty()) {
      // If there are still things in file_imports, then there were files that
      // were public imports into the non public imports, add those files are
      // needed to define the types also.
      //
      // Sort them (to get stable generation), and add them to the extra files
      // to imports.

      // This can really only happen in minimal imports mode, every other case,
      // it shouldn't happen.
      ABSL_CHECK(generation_options_.generate_minimal_imports);
      std::vector<const FileDescriptor*> still_needed(file_imports.begin(),
                                                      file_imports.end());
      std::sort(still_needed.begin(), still_needed.end(),
                FileDescriptorsOrderedByName());
      extra_files_to_import.insert(extra_files_to_import.end(),
                                   still_needed.begin(), still_needed.end());
    }
  }

  for (const auto& dep : extra_files_to_import) {
    import_writer.AddFile(dep, header_extension);
  }

  // Some things for all Emit() calls to have access too.
  auto vars = p->WithVars({
      {// Avoid the directive within the template strings as the tool would
       // then honor the directives within the generators sources.
       "clangfmt", "clang-format"},
      {"root_class_name", root_class_name_},
      {"google_protobuf_runtime_support",
       absl::StrCat("GOOGLE_PROTOBUF_OBJC_EXPECTED_GENCODE_VERSION_",
                    GOOGLE_PROTOBUF_OBJC_VERSION)},
  });

  p->Emit(
      {
          {"no_checked_in",
           "NO CHECKED-IN"
           // Intentional line breaker
           " PROTOBUF GENCODE"},
          {"filename", file_->name()},
          {"google_protobuf_objc_version", GOOGLE_PROTOBUF_OBJC_VERSION},
          {"runtime_imports",
           [&] {
             import_writer.PrintRuntimeImports(
                 p, /* default_cpp_symbol = */ !is_bundled_proto_);
           }},
          {"extra_system_imports",
           [&] {
             if (file_options.extra_system_headers.empty()) {
               return;
             }
             for (const auto& system_header :
                  file_options.extra_system_headers) {
               p->Emit({{"header", system_header}},
                       R"objc(
                         #import <$header$>
                       )objc");
             }
             p->Emit("\n");
           }},
          {"file_imports", [&] { import_writer.PrintFileImports(p); }},
          {"extra_warnings",
           [&] {
             for (const auto& warning : file_options.ignored_warnings) {
               p->Emit({{"warning", warning}},
                       R"objc(
                         #pragma clang diagnostic ignored "-W$warning$"
                       )objc");
             }
           }},
      },
      R"objc(
        // Generated by the protocol buffer compiler.  DO NOT EDIT!
        // $no_checked_in$
        // $clangfmt$ off
        // source: $filename$

        $runtime_imports$

        #if GOOGLE_PROTOBUF_OBJC_VERSION < $google_protobuf_objc_version$
        #error This file was generated by a newer version of protoc which is incompatible with your Protocol Buffer library sources.
        #endif
        #if $google_protobuf_objc_version$ < GOOGLE_PROTOBUF_OBJC_MIN_SUPPORTED_VERSION
        #error This file was generated by an older version of protoc which is incompatible with your Protocol Buffer library sources.
        #endif

        $extra_system_imports$
        $file_imports$
        // @@protoc_insertion_point(imports)

        #pragma clang diagnostic push
        #pragma clang diagnostic ignored "-Wdeprecated-declarations"
        $extra_warnings$
      )objc");

  p->Emit("\n");

  body();

  p->Emit("\n");

  p->Emit(R"objc(
    #pragma clang diagnostic pop

    // @@protoc_insertion_point(global_scope)

    // $clangfmt$ on
  )objc");
}

void FileGenerator::EmitRootImplementation(
    io::Printer* p,
    const std::vector<const FileDescriptor*>& deps_with_extensions) const {
  p->Emit(
      R"objc(
        #pragma mark - $root_class_name$

        @implementation $root_class_name$
      )objc");

  p->Emit("\n");

  // If there were any extensions or this file has any dependencies,
  // output a registry to override to create the file specific
  // registry.
  if (extension_generators_.empty() && deps_with_extensions.empty()) {
    p->Emit(R"objc(
      // No extensions in the file and no imports or none of the imports (direct or
      // indirect) defined extensions, so no need to generate +extensionRegistry.
    )objc");
  } else {
    EmitRootExtensionRegistryImplementation(p, deps_with_extensions);
  }

  p->Emit("\n");
  p->Emit("@end\n\n");
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
             p->Emit(
                 {
                     {"register_local_extensions_variable_blocks",
                      [&] {
                        for (const auto& generator : extension_generators_) {
                          generator->GenerateStaticVariablesInitialization(p);
                        }
                      }},
                 },
                 R"objc(
                   static GPBExtensionDescription descriptions[] = {
                     $register_local_extensions_variable_blocks$
                   };
                   for (size_t i = 0; i < sizeof(descriptions) / sizeof(descriptions[0]); ++i) {
                     GPBExtensionDescriptor *extension =
                         [[GPBExtensionDescriptor alloc] initWithExtensionDescription:&descriptions[i]
                                                                       runtimeSupport:&$google_protobuf_runtime_support$];
                     [registry addExtension:extension];
                     [self globallyRegisterExtension:extension];
                     [extension release];
                   }
                 )objc");
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
            registry = [[GPBExtensionRegistry alloc] init];
            $register_local_extensions$;
            $register_imports$
          }
          return registry;
        }
      )objc");
}

void FileGenerator::EmitFileDescription(io::Printer* p) const {
  // File descriptor only needed if there are messages to use it.
  if (message_generators_.empty()) {
    return;
  }

  const std::string objc_prefix(FileClassPrefix(file_));
  std::string syntax;
  if (generation_options_.experimental_strip_nonfunctional_codegen) {
    // Doesn't matter for current sources, use Unknown as a marker for this
    // mode.
    syntax = "GPBFileSyntaxUnknown";
  } else {
    switch (edition_) {
      case Edition::EDITION_UNKNOWN:
        syntax = "GPBFileSyntaxUnknown";
        break;
      case Edition::EDITION_PROTO2:
        syntax = "GPBFileSyntaxProto2";
        break;
      case Edition::EDITION_PROTO3:
        syntax = "GPBFileSyntaxProto3";
        break;
      default:
        syntax = "GPBFileSyntaxProtoEditions";
        break;
    }
  }

  p->Emit({{"file_description_name", file_description_name_},
           {"package_value", file_->package().empty()
                                 ? "NULL"
                                 : absl::StrCat("\"", file_->package(), "\"")},
           {"prefix_value",
            objc_prefix.empty() && !file_->options().has_objc_class_prefix()
                ? "NULL"
                : absl::StrCat("\"", objc_prefix, "\"")}},
          R"objc(
            static GPBFilePackageAndPrefix $file_description_name$ = {
              .package = $package_value$,
              .prefix = $prefix_value$
            };
          )objc");
  p->Emit("\n");
}

void FileGenerator::DetermineNeededDeps(
    absl::flat_hash_set<const FileDescriptor*>* deps,
    PublicDepsHandling public_deps_handling) const {
  // This logic captures the deps that are needed for types thus removing the
  // ones that are only deps because they provide the definitions for custom
  // options. If protoc gets something like "import options" then this logic can
  // go away as the non "import options" deps would be the ones needed.

  if (public_deps_handling == PublicDepsHandling::kForceInclude) {
    for (int i = 0; i < file_->public_dependency_count(); i++) {
      deps->insert(file_->public_dependency(i));
    }
  }

  for (const auto& generator : message_generators_) {
    generator->DetermineNeededFiles(deps);
  }
  for (const auto& generator : extension_generators_) {
    generator->DetermineNeededFiles(deps);
  }

  if (public_deps_handling == PublicDepsHandling::kExclude) {
    for (int i = 0; i < file_->public_dependency_count(); i++) {
      deps->erase(file_);
    }
  }
}

}  // namespace objectivec
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
