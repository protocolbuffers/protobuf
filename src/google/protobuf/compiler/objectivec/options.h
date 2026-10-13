// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef GOOGLE_PROTOBUF_COMPILER_OBJECTIVEC_OPTIONS_H__
#define GOOGLE_PROTOBUF_COMPILER_OBJECTIVEC_OPTIONS_H__

#include <string>

namespace google {
namespace protobuf {
namespace compiler {
namespace objectivec {

enum class ExtensionGenerationMode {
  // Root classes are kept. Extension descriptor and registry functions are
  // generated as ObjC classes & methods. This is the default.
  kClassBased,

  // C function based descriptor and registry functions are generated alongside
  // ObjC classes and methods. This is intended to be a transitional state to
  // help with migration to C function mode.
  kMigration,

  // Root classes are removed. Extension descriptor and registry functions are
  // generated as C functions. This is the preferred mode for new code,
  // because it avoids potential namespace collisions, allows the generated
  // code to be stripped by the linker, reduces binary size, and defers some
  // initialization logic to the first use instead of at app startup.
  kCFunction,
};

// The experimental multi source generation modes. This is an experiment that is
// not officially supported; it can change in behavior or go away at any time.
enum class MultiSourceGenerationMode {
  // Each .proto file generates a single .pbobjc.m file. This is the default.
  kNone,

  // Each ObjC class (messages and roots) generates to a unique .m file.
  kAll,

  // Messages and enums are generated into one .m file (the same code as single
  // source generation), and only the extension registry/root and the extension
  // descriptor functions are split into their own .m files.
  kExtensionsOnly,
};

// Generation options, documented within generator.cc.
struct GenerationOptions {
  std::string generate_for_named_framework;
  std::string named_framework_to_proto_path_mappings_path;
  std::string runtime_import_prefix;

  bool headers_use_forward_declarations = false;
  bool strip_custom_options = true;
  bool generate_minimal_imports = true;

  // These are experiments that are not officially supported. They can change
  // in behavior or go away at any time.
  MultiSourceGenerationMode experimental_multi_source_generation =
      MultiSourceGenerationMode::kNone;
  bool experimental_strip_nonfunctional_codegen = false;

  // The name of the pragma that will be used to indicate the start of the
  // metadata annotations. Must be set (along with `annotation_guard_name`) for
  // cross-references to be generated.
  std::string annotation_pragma_name;
  // The name of the preprocessor guard that will be used to guard the metadata
  // annotations. Must be set (along with `annotation_pragma_name`) for
  // cross-references to be generated.
  std::string annotation_guard_name;

  // The mode to use when generating extension code (C function, class based or
  // migration mode)
  ExtensionGenerationMode extension_generation_mode;

  bool EmitClassBasedExtensions() const {
    return extension_generation_mode == ExtensionGenerationMode::kClassBased ||
           extension_generation_mode == ExtensionGenerationMode::kMigration;
  }

  bool EmitCFunctionExtensions() const {
    return extension_generation_mode == ExtensionGenerationMode::kCFunction ||
           extension_generation_mode == ExtensionGenerationMode::kMigration;
  }

  bool IsMultiSourceGeneration() const {
    return experimental_multi_source_generation !=
           MultiSourceGenerationMode::kNone;
  }
};

}  // namespace objectivec
}  // namespace compiler
}  // namespace protobuf
}  // namespace google

#endif  // GOOGLE_PROTOBUF_COMPILER_OBJECTIVEC_OPTIONS_H__
