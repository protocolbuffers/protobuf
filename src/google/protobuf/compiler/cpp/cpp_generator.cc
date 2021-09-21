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

#include <google/protobuf/compiler/cpp/cpp_generator.h>

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <google/protobuf/stubs/strutil.h>
#include <google/protobuf/compiler/cpp/cpp_file.h>
#include <google/protobuf/compiler/cpp/cpp_helpers.h>
#include <google/protobuf/descriptor.pb.h>
#include <google/protobuf/io/printer.h>
#include <google/protobuf/io/zero_copy_stream.h>

namespace google {
namespace protobuf {
namespace compiler {
namespace cpp {

CppGenerator::CppGenerator() {}
CppGenerator::~CppGenerator() {}

namespace {
std::string NumberedCcFileName(const std::string& basename, int number) {
  return StrCat(basename, ".out/", number, ".cc");
}
}  // namespace

bool CppGenerator::Generate(const FileDescriptor* file,
                            const std::string& parameter,
                            GeneratorContext* generator_context,
                            std::string* error) const {
  std::vector<std::pair<std::string, std::string> > options;
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
  Options file_options;

  file_options.opensource_runtime = opensource_runtime_;
  file_options.runtime_include_base = runtime_include_base_;

  for (int i = 0; i < options.size(); i++) {
    if (options[i].first == "dllexport_decl") {
      file_options.dllexport_decl = options[i].second;
    } else if (options[i].first == "safe_boundary_check") {
      file_options.safe_boundary_check = true;
    } else if (options[i].first == "annotate_headers") {
      file_options.annotate_headers = true;
    } else if (options[i].first == "annotation_pragma_name") {
      file_options.annotation_pragma_name = options[i].second;
    } else if (options[i].first == "annotation_guard_name") {
      file_options.annotation_guard_name = options[i].second;
    } else if (options[i].first == "speed") {
      file_options.enforce_mode = EnforceOptimizeMode::kSpeed;
    } else if (options[i].first == "code_size") {
      file_options.enforce_mode = EnforceOptimizeMode::kCodeSize;
    } else if (options[i].first == "lite") {
      file_options.enforce_mode = EnforceOptimizeMode::kLiteRuntime;
    } else if (options[i].first == "lite_implicit_weak_fields") {
      file_options.enforce_mode = EnforceOptimizeMode::kLiteRuntime;
      file_options.lite_implicit_weak_fields = true;
      if (!options[i].second.empty()) {
        file_options.num_cc_files =
            strto32(options[i].second.c_str(), NULL, 10);
      }
    } else if (options[i].first == "annotate_accessor") {
      file_options.annotate_accessor = true;
    } else if (options[i].first == "inject_field_listener_events") {
      file_options.field_listener_options.inject_field_listener_events = true;
    } else if (options[i].first == "forbidden_field_listener_events") {
      std::size_t pos = 0;
      do {
        std::size_t next_pos = options[i].second.find_first_of("+", pos);
        if (next_pos == std::string::npos) {
          next_pos = options[i].second.size();
        }
        if (next_pos > pos)
          file_options.field_listener_options.forbidden_field_listener_events
              .insert(options[i].second.substr(pos, next_pos - pos));
        pos = next_pos + 1;
      } while (pos < options[i].second.size());
    } else if (options[i].first == "eagerly_verified_lazy") {
      file_options.eagerly_verified_lazy = true;
    } else if (options[i].first == "force_eagerly_verified_lazy") {
      file_options.force_eagerly_verified_lazy = true;
    } else if (options[i].first == "table_driven_parsing") {
      file_options.table_driven_parsing = true;
    } else if (options[i].first == "table_driven_serialization") {
      file_options.table_driven_serialization = true;
    } else if (options[i].first == "experimental_tail_call_table_mode") {
      if (options[i].second == "never") {
        file_options.tctable_mode = Options::kTCTableNever;
      } else if (options[i].second == "guarded") {
        file_options.tctable_mode = Options::kTCTableGuarded;
      } else if (options[i].second == "always") {
        file_options.tctable_mode = Options::kTCTableAlways;
      } else {
        *error = "Unknown value for experimental_tail_call_table_mode: " +
                 options[i].second;
        return false;
      }
    } else {
      *error = "Unknown generator option: " + options[i].first;
      return false;
    }
  }

  // The safe_boundary_check option controls behavior for Google-internal
  // protobuf APIs.
  if (file_options.safe_boundary_check && file_options.opensource_runtime) {
    *error =
        "The safe_boundary_check option is not supported outside of Google.";
    return false;
  }

  // -----------------------------------------------------------------


  std::string basename = StripProto(file->name());

  if (MaybeBootstrap(file_options, generator_context, file_options.bootstrap,
                     &basename)) {
    return true;
  }

  FileGenerator file_generator(file, file_options);

  // Generate header(s).
  if (file_options.proto_h) {
    std::unique_ptr<io::ZeroCopyOutputStream> output(
        generator_context->Open(basename + ".proto.h"));
    GeneratedCodeInfo annotations;
    io::AnnotationProtoCollector<GeneratedCodeInfo> annotation_collector(
        &annotations);
    std::string info_path = basename + ".proto.h.meta";
    io::Printer printer(
        output.get(), '$',
        file_options.annotate_headers ? &annotation_collector : NULL);
    file_generator.GenerateProtoHeader(
        &printer, file_options.annotate_headers ? info_path : "");
    if (file_options.annotate_headers) {
      std::unique_ptr<io::ZeroCopyOutputStream> info_output(
          generator_context->Open(info_path));
      annotations.SerializeToZeroCopyStream(info_output.get());
    }
  }

  {
    std::unique_ptr<io::ZeroCopyOutputStream> output(
        generator_context->Open(basename + ".pb.h"));
    GeneratedCodeInfo annotations;
    io::AnnotationProtoCollector<GeneratedCodeInfo> annotation_collector(
        &annotations);
    std::string info_path = basename + ".pb.h.meta";
    io::Printer printer(
        output.get(), '$',
        file_options.annotate_headers ? &annotation_collector : NULL);
    file_generator.GeneratePBHeader(
        &printer, file_options.annotate_headers ? info_path : "");
    if (file_options.annotate_headers) {
      std::unique_ptr<io::ZeroCopyOutputStream> info_output(
          generator_context->Open(info_path));
      annotations.SerializeToZeroCopyStream(info_output.get());
    }
  }

  // Generate cc file(s).
  if (UsingImplicitWeakFields(file, file_options)) {
    {
      // This is the global .cc file, containing
      // enum/services/tables/reflection
      std::unique_ptr<io::ZeroCopyOutputStream> output(
          generator_context->Open(basename + ".pb.cc"));
      io::Printer printer(output.get(), '$');
      file_generator.GenerateGlobalSource(&printer);
    }

    int num_cc_files =
        file_generator.NumMessages() + file_generator.NumExtensions();

    // If we're using implicit weak fields then we allow the user to
    // optionally specify how many files to generate, not counting the global
    // pb.cc file. If we have more files than messages, then some files will
    // be generated as empty placeholders.
    if (file_options.num_cc_files > 0) {
      GOOGLE_CHECK_LE(num_cc_files, file_options.num_cc_files)
          << "There must be at least as many numbered .cc files as messages "
             "and extensions.";
      num_cc_files = file_options.num_cc_files;
    }
    int cc_file_number = 0;
    for (int i = 0; i < file_generator.NumMessages(); i++) {
      std::unique_ptr<io::ZeroCopyOutputStream> output(generator_context->Open(
          NumberedCcFileName(basename, cc_file_number++)));
      io::Printer printer(output.get(), '$');
      file_generator.GenerateSourceForMessage(i, &printer);
    }
    for (int i = 0; i < file_generator.NumExtensions(); i++) {
      std::unique_ptr<io::ZeroCopyOutputStream> output(generator_context->Open(
          NumberedCcFileName(basename, cc_file_number++)));
      io::Printer printer(output.get(), '$');
      file_generator.GenerateSourceForExtension(i, &printer);
    }
    // Create empty placeholder files if necessary to match the expected number
    // of files.
    for (; cc_file_number < num_cc_files; ++cc_file_number) {
      std::unique_ptr<io::ZeroCopyOutputStream> output(generator_context->Open(
          NumberedCcFileName(basename, cc_file_number)));
    }
  } else {
    std::unique_ptr<io::ZeroCopyOutputStream> output(
        generator_context->Open(basename + ".pb.cc"));
    io::Printer printer(output.get(), '$');
    file_generator.GenerateSource(&printer);
  }

  return true;
}

}  // namespace cpp
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
