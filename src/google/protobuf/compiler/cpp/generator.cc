// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// Author: kenton@google.com (Kenton Varda)
//  Based on original Protocol Buffers design by
//  Sanjay Ghemawat, Jeff Dean, and others.

#include "google/protobuf/compiler/cpp/generator.h"

#include <cstdlib>
#include <cstring>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "absl/container/flat_hash_map.h"
#include "absl/log/absl_check.h"
#include "absl/memory/memory.h"
#include "absl/status/status.h"
#include "absl/strings/match.h"
#include "absl/strings/numbers.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_split.h"
#include "absl/strings/string_view.h"
#include "google/protobuf/compiler/code_generator.h"
#include "google/protobuf/compiler/cpp/file.h"
#include "google/protobuf/compiler/cpp/helpers.h"
#include "google/protobuf/compiler/cpp/options.h"
#include "google/protobuf/cpp_features.pb.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/descriptor.pb.h"
#include "google/protobuf/descriptor_visitor.h"
#include "google/protobuf/io/printer.h"


namespace google {
namespace protobuf {
namespace compiler {
namespace cpp {
namespace {

std::string NumberedCcFileName(absl::string_view basename, int number) {
  return absl::StrCat(basename, ".out/", number, ".cc");
}

absl::flat_hash_map<absl::string_view, std::string> CommonVars(
    const Options& options) {
  bool is_oss = options.opensource_runtime;
  return {
      {"pb", absl::StrCat("::", ProtobufNamespace(options))},
      {"pbi", absl::StrCat("::", ProtobufNamespace(options), "::internal")},

      {"string", "std::string"},
      {"int8", "::int8_t"},
      {"int32", "::int32_t"},
      {"int64", "::int64_t"},
      {"uint8", "::uint8_t"},
      {"uint32", "::uint32_t"},
      {"uint64", "::uint64_t"},

      {"hrule_thick", kThickSeparator},
      {"hrule_thin", kThinSeparator},

      {"nullable", "PROTOBUF_NULLABLE"},
      {"nonnull", "PROTOBUF_NONNULL"},

      // Warning: there is some clever naming/splitting here to avoid extract
      // script rewrites.  The names of these variables must not be things that
      // the extract script will rewrite.  That's why we use "CHK" (for example)
      // instead of "ABSL_CHECK".
      //
      // These values are things the extract script would rewrite if we did not
      // split them.  It might not strictly matter since we don't generate
      // google3 code in open-source.  But it's good to prevent surprising
      // things from happening.
      {"GOOGLE_PROTOBUF", is_oss ? "GOOGLE_PROTOBUF"
                                 : "GOOGLE3_PROTOBU"
                                   "F"},
      {"CHK",
       "ABSL_CHEC"
       "K"},
      {"DCHK",
       "ABSL_DCHEC"
       "K"},
  };
}


}  // namespace

bool CppGenerator::GenerateImpl(const FileDescriptor* file,
                                const std::string& parameter,
                                GeneratorContext* generator_context,
                                std::string* error,
                                const Options& file_options) const {
  ABSL_DCHECK_NE(file, nullptr);


  std::string basename = StripProto(file->name());

  auto generate_reserved_static_reflection_header = [&basename,
                                                     &generator_context]() {
    auto output = absl::WrapUnique(generator_context->Open(
        absl::StrCat(basename, ".proto.static_reflection.h")));
    io::Printer(output.get()).Emit(R"cc(
      // Reserved for future use.
    )cc");
  };
  // Suppress maybe unused warning.
  (void)generate_reserved_static_reflection_header;

  if (MaybeBootstrap(file_options, generator_context, file_options.bootstrap,
                     &basename)) {
    return true;
  }

  absl::Status validation_result = ValidateFeatures(file);
  if (!validation_result.ok()) {
    *error = std::string(validation_result.message());
    return false;
  }


  FileGenerator file_generator(file, file_options);

  // Generate header(s).
  if (file_options.proto_h) {
    auto output = absl::WrapUnique(
        generator_context->Open(absl::StrCat(basename, ".proto.h")));

    GeneratedCodeInfo annotations;
    io::AnnotationProtoCollector<GeneratedCodeInfo> annotation_collector(
        &annotations);
    io::Printer::Options options;
    if (file_options.annotate_headers) {
      options.annotation_collector = &annotation_collector;
    }

    io::Printer p(output.get(), options);
    auto v = p.WithVars(CommonVars(file_options));

    std::string info_path = absl::StrCat(basename, ".proto.h.meta");
    file_generator.GenerateProtoHeader(
        &p, file_options.annotate_headers ? info_path : "");

    if (file_options.annotate_headers) {
      auto info_output = absl::WrapUnique(generator_context->Open(info_path));
      ABSL_CHECK(annotations.SerializeToZeroCopyStream(info_output.get()));
    }
  }

  {
    auto output = absl::WrapUnique(
        generator_context->Open(absl::StrCat(basename, ".pb.h")));

    GeneratedCodeInfo annotations;
    io::AnnotationProtoCollector<GeneratedCodeInfo> annotation_collector(
        &annotations);
    io::Printer::Options options;
    if (file_options.annotate_headers) {
      options.annotation_collector = &annotation_collector;
    }

    io::Printer p(output.get(), options);
    auto v = p.WithVars(CommonVars(file_options));

    std::string info_path = absl::StrCat(basename, ".pb.h.meta");
    file_generator.GeneratePBHeader(
        &p, file_options.annotate_headers ? info_path : "");

    if (file_options.annotate_headers) {
      auto info_output = absl::WrapUnique(generator_context->Open(info_path));
      ABSL_CHECK(annotations.SerializeToZeroCopyStream(info_output.get()));
    }
  }

  // Generate cc file(s).
  if (UsingImplicitWeakFields(file, file_options)) {
    {
      // This is the global .cc file, containing
      // enum/services/tables/reflection
      auto output = absl::WrapUnique(
          generator_context->Open(absl::StrCat(basename, ".pb.cc")));
      io::Printer p(output.get());
      auto v = p.WithVars(CommonVars(file_options));

      file_generator.GenerateGlobalSource(&p);
    }

    int num_cc_files =
        file_generator.NumMessages() + file_generator.NumExtensions();

    // If we're using implicit weak fields then we allow the user to
    // optionally specify how many files to generate, not counting the global
    // pb.cc file. If we have more files than messages, then some files will
    // be generated as empty placeholders.
    if (file_options.num_cc_files > 0) {
      ABSL_CHECK_LE(num_cc_files, file_options.num_cc_files)
          << "There must be at least as many numbered .cc files as messages "
             "and extensions.";
      num_cc_files = file_options.num_cc_files;
    }

    int cc_file_number = 0;
    for (int i = 0; i < file_generator.NumMessages(); ++i) {
      auto output = absl::WrapUnique(generator_context->Open(
          NumberedCcFileName(basename, cc_file_number++)));
      io::Printer p(output.get());
      auto v = p.WithVars(CommonVars(file_options));

      file_generator.GenerateSourceForMessage(i, &p);
    }

    for (int i = 0; i < file_generator.NumExtensions(); ++i) {
      auto output = absl::WrapUnique(generator_context->Open(
          NumberedCcFileName(basename, cc_file_number++)));
      io::Printer p(output.get());
      auto v = p.WithVars(CommonVars(file_options));

      file_generator.GenerateSourceForExtension(i, &p);
    }

    // Create empty placeholder files if necessary to match the expected number
    // of files.
    while (cc_file_number < num_cc_files) {
      (void)absl::WrapUnique(generator_context->Open(
          NumberedCcFileName(basename, cc_file_number++)));
    }
  } else {
    auto output = absl::WrapUnique(
        generator_context->Open(absl::StrCat(basename, ".pb.cc")));
    io::Printer p(output.get());
    auto v = p.WithVars(CommonVars(file_options));

    file_generator.GenerateSource(&p);
  }

  return true;
}

static bool IsEnumMapType(const FieldDescriptor& field) {
  if (!field.is_map()) return false;
  for (int i = 0; i < field.message_type()->field_count(); ++i) {
    if (field.message_type()->field(i)->type() == FieldDescriptor::TYPE_ENUM) {
      return true;
    }
  }
  return false;
}

absl::Status CppGenerator::ValidateFeatures(const FileDescriptor* file) const {
  absl::Status status = absl::OkStatus();
  google::protobuf::internal::VisitDescriptors(*file, [&](const FieldDescriptor& field) {
    const FeatureSet& resolved_features = GetResolvedSourceFeatures(field);
    const pb::CppFeatures& unresolved_features =
        GetUnresolvedSourceFeatures(field, pb::cpp);
    if (field.enum_type() != nullptr &&
        resolved_features.GetExtension(::pb::cpp).legacy_closed_enum() &&
        resolved_features.field_presence() == FeatureSet::IMPLICIT) {
      status = absl::FailedPreconditionError(
          absl::StrCat("Field ", field.full_name(),
                       " has a closed enum type with implicit presence."));
    }

    if (field.containing_type() == nullptr ||
        !field.containing_type()->options().map_entry()) {
      // Skip validation of explicit features on generated map fields.  These
      // will be blindly propagated from the original map field, and may violate
      // a lot of these conditions.  Note: we do still validate the
      // user-specified map field.
      if (unresolved_features.has_legacy_closed_enum() &&
          field.cpp_type() != FieldDescriptor::CPPTYPE_ENUM &&
          !IsEnumMapType(field)) {
        status = absl::FailedPreconditionError(
            absl::StrCat("Field ", field.full_name(),
                         " specifies the legacy_closed_enum feature but has "
                         "non-enum type."));
      }
    }

    if ((unresolved_features.string_type() == pb::CppFeatures::CORD ||
         field.legacy_proto_ctype() == FieldOptions::CORD) &&
        field.is_extension()) {
      status = absl::FailedPreconditionError(
          absl::StrCat("Extension ", field.full_name(),
                       " specifies CORD string type which is not supported "
                       "for extensions."));
    }

    if ((unresolved_features.has_string_type() ||
         field.has_legacy_proto_ctype()) &&
        field.cpp_type() != FieldDescriptor::CPPTYPE_STRING) {
      status = absl::FailedPreconditionError(absl::StrCat(
          "Field ", field.full_name(),
          " specifies string_type, but is not a string nor bytes field."));
    }

    if (unresolved_features.has_string_type() &&
        field.has_legacy_proto_ctype()) {
      status = absl::FailedPreconditionError(absl::StrCat(
          "Field ", field.full_name(),
          " specifies both string_type and ctype which is not supported."));
    }
  });
  return status;
}

bool CppGenerator::GenerateAll(const std::vector<const FileDescriptor*>& files,
                               const std::string& parameter,
                               GeneratorContext* generator_context,
                               std::string* error) const {
  std::vector<std::pair<std::string, std::string>> options;
  ParseGeneratorParameter(parameter, &options);

  // -----------------------------------------------------------------
  // parse generator options

  // If the dllexport_decl option is passed to the compiler, we need to write
  // it in front of every symbol that should be exported if this .proto is
  // compiled into a Windows DLL.  E.g., if the user invokes the protocol
  // compiler as:
  //   protoc --cpp_out=dllexport_decl=FOO_EXPORT:outdir foo.proto
  // then we'll define classes like this:
  //   class FOO_EXPORT Foo {
  //     ...
  //   }
  // FOO_EXPORT is a macro which should expand to __declspec(dllexport) or
  // __declspec(dllimport) depending on what is being compiled.
  //
  // If the proto_h option is passed to the compiler, we will generate all
  // classes and enums so that they can be forward-declared from files that
  // need them from imports.
  //
  // If the lite option is passed to the compiler, we will generate the
  // current files and all transitive dependencies using the LITE runtime.
  Options common_file_options;

  common_file_options.opensource_runtime = opensource_runtime_;
  common_file_options.runtime_include_base = runtime_include_base_;

  std::vector<std::string> protos_for_field_listener_events;

  for (const auto& option : options) {
    const auto& key = option.first;
    const auto& value = option.second;

    if (key == "dllexport_decl") {
      common_file_options.dllexport_decl = value;
    } else if (key == "annotate_headers") {
      common_file_options.annotate_headers = true;
    } else if (key == "annotation_pragma_name") {
      common_file_options.annotation_pragma_name = value;
    } else if (key == "annotation_guard_name") {
      common_file_options.annotation_guard_name = value;
    } else if (key == "speed") {
      common_file_options.enforce_mode = EnforceOptimizeMode::kSpeed;
    } else if (key == "code_size") {
      common_file_options.enforce_mode = EnforceOptimizeMode::kCodeSize;
    } else if (key == "lite") {
      common_file_options.enforce_mode = EnforceOptimizeMode::kLiteRuntime;
    } else if (key == "lite_implicit_weak_fields") {
      common_file_options.enforce_mode = EnforceOptimizeMode::kLiteRuntime;
      common_file_options.lite_implicit_weak_fields = true;
      int num_cc_files;
      if (!value.empty() && absl::SimpleAtoi(value, &num_cc_files)) {
        common_file_options.num_cc_files = num_cc_files;
      }
    } else if (key == "descriptor_implicit_weak_messages") {
      common_file_options.descriptor_implicit_weak_messages = true;
    } else if (key == "proto_h") {
      common_file_options.proto_h = true;
    } else if (key == "proto_static_reflection_h") {
    } else if (key == "annotate_accessor") {
      common_file_options.annotate_accessor = true;
    } else if (key == "protos_for_field_listener_events") {
      protos_for_field_listener_events = absl::StrSplit(value, ':');
    } else if (key == "inject_field_listener_events") {
      common_file_options.field_listener_options.inject_field_listener_events =
          true;
    } else if (key == "forbidden_field_listener_events") {
      std::size_t pos = 0;
      do {
        std::size_t next_pos = value.find_first_of("+", pos);
        if (next_pos == std::string::npos) {
          next_pos = value.size();
        }
        if (next_pos > pos)
          common_file_options.field_listener_options
              .forbidden_field_listener_events.emplace(
                  value.substr(pos, next_pos - pos));
        pos = next_pos + 1;
      } while (pos < value.size());
    } else if (key == "force_eagerly_verified_lazy") {
      common_file_options.force_eagerly_verified_lazy = true;
    } else if (key == "experimental_strip_nonfunctional_codegen") {
      common_file_options.strip_nonfunctional_codegen = true;
    } else if (key == "experimental_cpp_micro_string") {
      common_file_options.experimental_use_micro_string = true;
    } else {
      *error = absl::StrCat("Unknown generator option: ", key);
      return false;
    }
  }

  // -----------------------------------------------------------------

  MessageSCCAnalyzer scc_analyzer(common_file_options);
  common_file_options.scc_analyzer = &scc_analyzer;

  for (size_t i = 0; i < files.size(); i++) {
    const FileDescriptor* file = files[i];
    Options file_options = common_file_options;
    for (absl::string_view proto : protos_for_field_listener_events) {
      if (file->name() == proto) {
        file_options.field_listener_options.inject_field_listener_events = true;
        break;
      }
    }


    if (!GenerateImpl(file, parameter, generator_context, error,
                      file_options)) {
      *error = absl::StrCat(file->name(), ": ", *error);
      return false;
    }
  }
  return true;
}

bool CppGenerator::Generate(const FileDescriptor* file,
                            const std::string& parameter,
                            GeneratorContext* generator_context,
                            std::string* error) const {
  return GenerateAll({file}, parameter, generator_context, error);
}

}  // namespace cpp
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
