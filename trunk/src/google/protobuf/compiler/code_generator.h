// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.
// http://code.google.com/p/protobuf/
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

// Author: kenton@google.com (Kenton Varda)
//  Based on original Protocol Buffers design by
//  Sanjay Ghemawat, Jeff Dean, and others.
//
// Defines the abstract interface implemented by each of the language-specific
// code generators.

#ifndef GOOGLE_PROTOBUF_COMPILER_CODE_GENERATOR_H__
#define GOOGLE_PROTOBUF_COMPILER_CODE_GENERATOR_H__

#include <google/protobuf/stubs/common.h>
#include <string>

namespace google {
namespace protobuf {

namespace io { class ZeroCopyOutputStream; }
class FileDescriptor;

namespace compiler {

// Defined in this file.
class CodeGenerator;
class OutputDirectory;

// The abstract interface to a class which generates code implementing a
// particular proto file in a particular language.  A number of these may
// be registered with CommandLineInterface to support various languages.
class LIBPROTOC_EXPORT CodeGenerator {
 public:
  inline CodeGenerator() {}
  virtual ~CodeGenerator();

  // Generates code for the given proto file, generating one or more files in
  // the given output directory.
  //
  // A parameter to be passed to the generator can be specified on the
  // command line.  This is intended to be used by Java and similar languages
  // to specify which specific class from the proto file is to be generated,
  // though it could have other uses as well.  It is empty if no parameter was
  // given.
  //
  // Returns true if successful.  Otherwise, sets *error to a description of
  // the problem (e.g. "invalid parameter") and returns false.
  virtual bool Generate(const FileDescriptor* file,
                        const string& parameter,
                        OutputDirectory* output_directory,
                        string* error) const = 0;

 private:
  GOOGLE_DISALLOW_EVIL_CONSTRUCTORS(CodeGenerator);
};

// CodeGenerators generate one or more files in a given directory.  This
// abstract interface represents the directory to which the CodeGenerator is
// to write.
class LIBPROTOC_EXPORT OutputDirectory {
 public:
  inline OutputDirectory() {}
  virtual ~OutputDirectory();

  // Opens the given file, truncating it if it exists, and returns a
  // ZeroCopyOutputStream that writes to the file.  The caller takes ownership
  // of the returned object.  This method never fails (a dummy stream will be
  // returned instead).
  //
  // The filename given should be relative to the root of the source tree.
  // E.g. the C++ generator, when generating code for "foo/bar.proto", will
  // generate the files "foo/bar.pb2.h" and "foo/bar.pb2.cc"; note that
  // "foo/" is included in these filenames.  The filename is not allowed to
  // contain "." or ".." components.
  virtual io::ZeroCopyOutputStream* Open(const string& filename) = 0;

 private:
  GOOGLE_DISALLOW_EVIL_CONSTRUCTORS(OutputDirectory);
};

}  // namespace compiler
}  // namespace protobuf

}  // namespace google
#endif  // GOOGLE_PROTOBUF_COMPILER_CODE_GENERATOR_H__
