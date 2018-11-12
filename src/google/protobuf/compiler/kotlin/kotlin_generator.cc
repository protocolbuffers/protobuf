// Protocol Buffers - Google's data interchange format
// Copyright 2018 Oleksii Pylypenko.  All rights reserved.
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

// Author: oleksiy.pylypenko@gmail.com

#include <google/protobuf/compiler/kotlin/kotlin_generator.h>

#include <memory>

#include <google/protobuf/compiler/kotlin/kotlin_file.h>
#include <google/protobuf/compiler/kotlin/kotlin_options.h>
#include <google/protobuf/compiler/java/java_helpers.h>
#include <google/protobuf/descriptor.pb.h>
#include <google/protobuf/io/printer.h>
#include <google/protobuf/io/zero_copy_stream.h>

#include <google/protobuf/stubs/strutil.h>

namespace google {
namespace protobuf {
namespace compiler {
namespace kotlin {

KotlinGenerator::KotlinGenerator() {}
KotlinGenerator::~KotlinGenerator() {}

bool KotlinGenerator::Generate(const FileDescriptor *file,
                               const string &parameter,
                               GeneratorContext *context,
                               string *error) const {
  // -----------------------------------------------------------------
  // first generate Java classes

  if (!JavaGenerator::Generate(file, parameter, context, error)) {
    return false;
  }

  // -----------------------------------------------------------------
  // parse options

  java::Options file_options;
  if (!ParseGeneratorOptions(parameter, file_options, error)) {
    return false;
  }

  // -----------------------------------------------------------------

  std::vector<string> all_files;
  std::vector<string> all_annotations;

  std::vector<FileGenerator*> file_generators;
  if (file_options.generate_immutable_code) {
    file_generators.push_back(new FileGenerator(file, file_options,
        /* immutable = */ true));
  }
  if (file_options.generate_mutable_code) {
    file_generators.push_back(new FileGenerator(file, file_options,
        /* mutable = */ false));
  }

  for (int i = 0; i < file_generators.size(); ++i) {
    if (!file_generators[i]->Validate(error)) {
      for (int j = 0; j < file_generators.size(); ++j) {
        delete file_generators[j];
      }
      return false;
    }
  }

  for (int i = 0; i < file_generators.size(); ++i) {
    FileGenerator* file_generator = file_generators[i];

    string package_dir = java::JavaPackageToDir(file_generator->java_package());

    string java_filename = package_dir;
    java_filename += file_generator->classname();
    java_filename += ".kt";
    all_files.push_back(java_filename);
    string info_full_path = java_filename + ".pb.meta";
    if (file_options.annotate_code) {
      all_annotations.push_back(info_full_path);
    }

    // Generate main java file.
    std::unique_ptr<io::ZeroCopyOutputStream> output(
        context->Open(java_filename));
    GeneratedCodeInfo annotations;
    io::AnnotationProtoCollector<GeneratedCodeInfo> annotation_collector(
        &annotations);
    io::Printer printer(output.get(), '$', file_options.annotate_code
                                           ? &annotation_collector
                                           : NULL);

    file_generator->Generate(&printer);

    if (file_options.annotate_code) {
      std::unique_ptr<io::ZeroCopyOutputStream> info_output(
          context->Open(info_full_path));
      annotations.SerializeToZeroCopyStream(info_output.get());
    }
  }


  for (int i = 0; i < file_generators.size(); ++i) {
    delete file_generators[i];
  }
  file_generators.clear();

  // Generate output list if requested.
  if (!file_options.output_list_file.empty()) {
    // Generate output list.  This is just a simple text file placed in a
    // deterministic location which lists the .java files being generated.
    std::unique_ptr<io::ZeroCopyOutputStream> srclist_raw_output(
        context->Open(file_options.output_list_file));
    io::Printer srclist_printer(srclist_raw_output.get(), '$');
    for (int i = 0; i < all_files.size(); i++) {
      srclist_printer.Print("$filename$\n", "filename", all_files[i]);
    }
  }

  if (!file_options.annotation_list_file.empty()) {
    std::unique_ptr<io::ZeroCopyOutputStream> annotation_list_raw_output(
        context->Open(file_options.annotation_list_file));
    io::Printer annotation_list_printer(annotation_list_raw_output.get(), '$');
    for (int i = 0; i < all_annotations.size(); i++) {
      annotation_list_printer.Print("$filename$\n", "filename",
                                    all_annotations[i]);
    }
  }

  return true;
}

}  // namespace kotlin
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
