// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "google/protobuf/compiler/objectivec/generator.h"

#include <cstddef>
#include <cstdlib>
#include <fstream>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "absl/container/flat_hash_set.h"
#include "absl/memory/memory.h"
#include "absl/strings/ascii.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_split.h"
#include "absl/strings/string_view.h"
#include "absl/strings/strip.h"
#include "google/protobuf/compiler/code_generator.h"
#include "google/protobuf/compiler/objectivec/file.h"
#include "google/protobuf/compiler/objectivec/names.h"
#include "google/protobuf/compiler/objectivec/options.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/io/printer.h"
#include "google/protobuf/io/zero_copy_stream.h"

namespace google {
namespace protobuf {
namespace compiler {
namespace objectivec {

namespace {

// Convert a string with "yes"/"no" (case insensitive) to a boolean, returning
// true/false for if the input string was a valid value. The empty string is
// also treated as a true value. If the input string is invalid, `result` is
// unchanged.
bool StringToBool(const std::string& value, bool* result) {
  std::string upper_value(value);
  absl::AsciiStrToUpper(&upper_value);
  if (upper_value == "NO") {
    *result = false;
    return true;
  }
  if (upper_value == "YES" || upper_value.empty()) {
    *result = true;
    return true;
  }

  return false;
}

std::string NumberedObjCMFileName(absl::string_view basename, int number) {
  return absl::StrCat(basename, ".out/", number, ".pbobjc.m");
}

}  // namespace

bool ObjectiveCGenerator::HasGenerateAll() const { return true; }

bool ObjectiveCGenerator::Generate(const FileDescriptor* file,
                                   const std::string& parameter,
                                   GeneratorContext* context,
                                   std::string* error) const {
  *error = "Unimplemented Generate() method. Call GenerateAll() instead.";
  return false;
}

bool ObjectiveCGenerator::GenerateAll(
    const std::vector<const FileDescriptor*>& files,
    const std::string& parameter, GeneratorContext* context,
    std::string* error) const {
  // -----------------------------------------------------------------
  // Parse generator options. These options are passed to the compiler using the
  // --objc_opt flag. The options are passed as a comma separated list of
  // options along with their values. If the option appears multiple times, only
  // the last value will be considered.
  //
  // e.g. protoc ...
  // --objc_opt=expected_prefixes=file.txt,generate_for_named_framework=MyFramework

  Options validation_options;
  GenerationOptions generation_options;

  std::vector<std::pair<std::string, std::string> > options;
  ParseGeneratorParameter(parameter, &options);
  for (size_t i = 0; i < options.size(); i++) {
    if (options[i].first == "expected_prefixes_path") {
      // Path to find a file containing the expected prefixes
      // (objc_class_prefix "PREFIX") for proto packages (package NAME). The
      // generator will then issue warnings/errors if in the proto files being
      // generated the option is not listed/wrong/etc in the file.
      //
      // The format of the file is:
      //   - An entry is a line of "package=prefix".
      //   - Comments start with "#".
      //   - A comment can go on a line after a expected package/prefix pair.
      //     (i.e. - "package=prefix # comment")
      //   - For files that do NOT have a proto package (not recommended), an
      //     entry can be made as "no_package:PATH=prefix", where PATH is the
      //     path for the .proto file.
      //
      // There is no validation that the prefixes are good prefixes, it is
      // assumed that they are when you create the file.
      validation_options.expected_prefixes_path = options[i].second;
    } else if (options[i].first == "expected_prefixes_suppressions") {
      // A semicolon delimited string that lists the paths of .proto files to
      // exclude from the package prefix validations (expected_prefixes_path).
      // This is provided as an "out", to skip some files being checked.
      for (absl::string_view split_piece :
           absl::StrSplit(options[i].second, ';', absl::SkipEmpty())) {
        validation_options.expected_prefixes_suppressions.push_back(
            std::string(split_piece));
      }
    } else if (options[i].first == "prefixes_must_be_registered") {
      // If objc prefix file option value must be registered to be used. This
      // option has no meaning if an "expected_prefixes_path" isn't set. The
      // available options are:
      //   "no": They don't have to be registered.
      //   "yes": They must be registered and an error will be raised if a files
      //     tried to use a prefix that isn't registered.
      // Default is "no".
      if (!StringToBool(options[i].second,
                        &validation_options.prefixes_must_be_registered)) {
        *error = absl::StrCat(
            "error: Unknown value for prefixes_must_be_registered: ",
            options[i].second);
        return false;
      }
    } else if (options[i].first == "require_prefixes") {
      // If every file must have an objc prefix file option to be used. The
      // available options are:
      //   "no": Files can be generated without the prefix option.
      //   "yes": Files must have the objc prefix option, and an error will be
      //     raised if a files doesn't have one.
      // Default is "no".
      if (!StringToBool(options[i].second,
                        &validation_options.require_prefixes)) {
        *error = absl::StrCat("error: Unknown value for require_prefixes: ",
                              options[i].second);
        return false;
      }
    } else if (options[i].first == "generate_for_named_framework") {
      // The name of the framework that protos are being generated for. This
      // will cause the #import statements to be framework based using this
      // name (i.e. - "#import <NAME/proto.pbobjc.h>).
      //
      // NOTE: If this option is used with
      // named_framework_to_proto_path_mappings_path, then this is effectively
      // the "default" framework name used for everything that wasn't mapped by
      // the mapping file.
      generation_options.generate_for_named_framework = options[i].second;
    } else if (options[i].first ==
               "named_framework_to_proto_path_mappings_path") {
      // Path to find a file containing the list of framework names and proto
      // files. The generator uses this to decide if a proto file
      // referenced should use a framework style import vs. a user level import
      // (#import <FRAMEWORK/file.pbobjc.h> vs #import "dir/file.pbobjc.h").
      //
      // The format of the file is:
      //   - An entry is a line of "frameworkName: file.proto, dir/file2.proto".
      //   - Comments start with "#".
      //   - A comment can go on a line after a expected package/prefix pair.
      //     (i.e. - "frameworkName: file.proto # comment")
      //
      // Any number of files can be listed for a framework, just separate them
      // with commas.
      //
      // There can be multiple lines listing the same frameworkName in case it
      // has a lot of proto files included in it; having multiple lines makes
      // things easier to read. If a proto file is not configured in the
      // mappings file, it will use the default framework name if one was passed
      // with generate_for_named_framework, or the relative path to it's include
      // path otherwise.
      generation_options.named_framework_to_proto_path_mappings_path =
          options[i].second;
    } else if (options[i].first == "runtime_import_prefix") {
      // Path to use as a prefix on #imports of runtime provided headers in the
      // generated files. When integrating ObjC protos into a build system,
      // this can be used to avoid having to add the runtime directory to the
      // header search path since the generate #import will be more complete.
      generation_options.runtime_import_prefix =
          std::string(absl::StripSuffix(options[i].second, "/"));
    } else if (options[i].first == "package_to_prefix_mappings_path") {
      // Path to use for when loading the objc class prefix mappings to use.
      // The `objc_class_prefix` file option is always honored first if one is
      // present. This option also has precedent over the use_package_as_prefix
      // option.
      //
      // The format of the file is:
      //   - An entry is a line of "package=prefix".
      //   - Comments start with "#".
      //   - A comment can go on a line after a expected package/prefix pair.
      //     (i.e. - "package=prefix # comment")
      //   - For files that do NOT have a proto package (not recommended), an
      //     entry can be made as "no_package:PATH=prefix", where PATH is the
      //     path for the .proto file.
      //
      SetPackageToPrefixMappingsPath(options[i].second);
    } else if (options[i].first == "use_package_as_prefix") {
      // Controls how the symbols should be prefixed to avoid symbols
      // collisions. The objc_class_prefix file option is always honored, this
      // is just what to do if that isn't set. The available options are:
      //   "no": Not prefixed (the existing mode).
      //   "yes": Make a prefix out of the proto package.
      bool value = false;
      if (StringToBool(options[i].second, &value)) {
        SetUseProtoPackageAsDefaultPrefix(value);
      } else {
        *error = absl::StrCat("error: Unknown use_package_as_prefix: ",
                              options[i].second);
        return false;
      }
    } else if (options[i].first == "proto_package_prefix_exceptions_path") {
      // Path to find a file containing the list of proto package names that are
      // exceptions when use_package_as_prefix is enabled. This can be used to
      // migrate packages one at a time to use_package_as_prefix since there
      // are likely code updates needed with each one.
      //
      // The format of the file is:
      //   - An entry is a line of "proto.package.name".
      //   - Comments start with "#".
      //   - A comment can go on a line after a expected package/prefix pair.
      //     (i.e. - "some.proto.package # comment")
      SetProtoPackagePrefixExceptionList(options[i].second);
    } else if (options[i].first == "package_as_prefix_forced_prefix") {
      // String to use as the prefix when deriving a prefix from the package
      // name. So this only applies when use_package_as_prefix is also used.
      SetForcedPackagePrefix(options[i].second);
    } else if (options[i].first == "headers_use_forward_declarations") {
      if (!StringToBool(options[i].second,
                        &generation_options.headers_use_forward_declarations)) {
        *error = absl::StrCat(
            "error: Unknown value for headers_use_forward_declarations: ",
            options[i].second);
        return false;
      }
    } else if (options[i].first == "strip_custom_options") {
      // Controls if extensions that define custom options are included the
      // generated code. Since ObjC protos does not capture these descriptor
      // options, there normally isn't a need for these extensions. Docs on
      // custom options:
      //   https://protobuf.dev/programming-guides/proto2/#customoptions
      if (!StringToBool(options[i].second,
                        &generation_options.strip_custom_options)) {
        *error = absl::StrCat("error: Unknown value for strip_custom_options: ",
                              options[i].second);
        return false;
      }
    } else if (options[i].first == "generate_minimal_imports") {
      // Controls if minimal imports should be generated from a files imports.
      // Since custom options require imports, they current cause generated
      // imports even though there is nothing captured in the generated code,
      // this provides smaller imports only for the things referenced. This
      // could break code in complex cases where code uses types via long
      // import chains with public imports mixed through the way, as things
      // that aren't really needed for the local usages could be pruned.
      if (!StringToBool(options[i].second,
                        &generation_options.generate_minimal_imports)) {
        *error =
            absl::StrCat("error: Unknown value for generate_minimal_imports: ",
                         options[i].second);
        return false;
      }
    } else if (options[i].first == "experimental_multi_source_generation") {
      // This is an experimental option, and could be removed or change at any
      // time; it is not documented in the README.md for that reason.
      //
      // Enables a mode where each ObjC class (messages and roots) generates to
      // a unique .m file; this is to explore impacts on code size when not
      // compiling/linking with `-ObjC` as then only linker visible needs should
      // be pulled into the builds.
      if (!StringToBool(
              options[i].second,
              &generation_options.experimental_multi_source_generation)) {
        *error = absl::StrCat(
            "error: Unknown value for experimental_multi_source_generation: ",
            options[i].second);
        return false;
      }
    } else if (options[i].first == "experimental_strip_nonfunctional_codegen") {
      if (!StringToBool(
              options[i].second,
              &generation_options.experimental_strip_nonfunctional_codegen)) {
        *error = absl::StrCat(
            "error: Unknown value for "
            "experimental_strip_nonfunctional_codegen: ",
            options[i].second);
        return false;
      }
    } else {
      *error =
          absl::StrCat("error: Unknown generator option: ", options[i].first);
      return false;
    }
  }

  // Multi source generation forces:
  // - off the use of fwd decls in favor of imports
  // - on the minimal imports support
  if (generation_options.experimental_multi_source_generation) {
    generation_options.headers_use_forward_declarations = false;
    generation_options.generate_minimal_imports = true;
  }
  if (generation_options.experimental_strip_nonfunctional_codegen) {
    generation_options.generate_minimal_imports = true;
  }

  // -----------------------------------------------------------------

  // These are not official generation options and could be removed/changed in
  // the future and doing that won't count as a breaking change.
  bool headers_only = getenv("GPB_OBJC_HEADERS_ONLY") != nullptr;
  absl::flat_hash_set<std::string> skip_impls;
  if (getenv("GPB_OBJC_SKIP_IMPLS_FILE") != nullptr) {
    std::ifstream skip_file(getenv("GPB_OBJC_SKIP_IMPLS_FILE"));
    if (skip_file.is_open()) {
      std::string line;
      while (std::getline(skip_file, line)) {
        skip_impls.insert(line);
      }
    } else {
      *error = "error: Failed to open GPB_OBJC_SKIP_IMPLS_FILE file";
      return false;
    }
  }

  // -----------------------------------------------------------------

  // Validate the objc prefix/package pairings.
  if (!ValidateObjCClassPrefixes(files, validation_options, error)) {
    // *error will have been filled in.
    return false;
  }

  FileGenerator::CommonState state(!generation_options.strip_custom_options);
  for (const auto& file : files) {
    const FileGenerator file_generator(file, generation_options, state);
    std::string filepath = FilePath(file);

    // Generate header.
    {
      auto output =
          absl::WrapUnique(context->Open(absl::StrCat(filepath, ".pbobjc.h")));
      io::Printer printer(output.get());
      file_generator.GenerateHeader(&printer);
      if (printer.failed()) {
        *error = absl::StrCat("error: internal error generating a header: ",
                              file->name());
        return false;
      }
    }

    // Generate m file(s).
    if (!headers_only && skip_impls.count(file->name()) == 0) {
      if (generation_options.experimental_multi_source_generation) {
        int file_number = 0;

        // Generate the Root and FileDescriptor (if needed).
        {
          std::unique_ptr<io::ZeroCopyOutputStream> output(
              context->Open(NumberedObjCMFileName(filepath, file_number++)));
          io::Printer printer(output.get());
          file_generator.GenerateGlobalSource(&printer);
          if (printer.failed()) {
            *error = absl::StrCat(
                "error: internal error generating an implementation:",
                file->name());
            return false;
          }
        }

        // Enums only generate C functions, so they can all go in one file as
        // dead stripping anything not used.
        if (file_generator.NumEnums() > 0) {
          std::unique_ptr<io::ZeroCopyOutputStream> output(
              context->Open(NumberedObjCMFileName(filepath, file_number++)));
          io::Printer printer(output.get());
          file_generator.GenerateSourceForEnums(&printer);
          if (printer.failed()) {
            *error = absl::StrCat(
                "error: internal error generating an enum implementation(s):",
                file->name());
            return false;
          }
        }

        for (int i = 0; i < file_generator.NumMessages(); ++i) {
          std::unique_ptr<io::ZeroCopyOutputStream> output(
              context->Open(NumberedObjCMFileName(filepath, file_number++)));
          io::Printer printer(output.get());
          file_generator.GenerateSourceForMessage(i, &printer);
          if (printer.failed()) {
            *error = absl::StrCat(
                "error: internal error generating an message implementation:",
                file->name(), "::", i);
            return false;
          }
        }
      } else {
        auto output = absl::WrapUnique(
            context->Open(absl::StrCat(filepath, ".pbobjc.m")));
        io::Printer printer(output.get());
        file_generator.GenerateSource(&printer);
        if (printer.failed()) {
          *error = absl::StrCat(
              "error: internal error generating an implementation:",
              file->name());
          return false;
        }
      }
    }  // if (!headers_only && skip_impls.count(file->name()) == 0)
  }    // for(file : files)

  return true;
}

}  // namespace objectivec
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
