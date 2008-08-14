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

#include <google/protobuf/compiler/cpp/cpp_generator.h>

#include <vector>
#include <utility>

#include <google/protobuf/compiler/cpp/cpp_file.h>
#include <google/protobuf/compiler/cpp/cpp_helpers.h>
#include <google/protobuf/io/printer.h>
#include <google/protobuf/io/zero_copy_stream.h>
#include <google/protobuf/descriptor.pb.h>
#include <google/protobuf/stubs/strutil.h>

namespace google {
namespace protobuf {
namespace compiler {
namespace cpp {

namespace {

// Parses a set of comma-delimited name/value pairs, e.g.:
//   "foo=bar,baz,qux=corge"
// parses to the pairs:
//   ("foo", "bar"), ("baz", ""), ("qux", "corge")
void ParseOptions(const string& text, vector<pair<string, string> >* output) {
  vector<string> parts;
  SplitStringUsing(text, ",", &parts);

  for (int i = 0; i < parts.size(); i++) {
    string::size_type equals_pos = parts[i].find_first_of('=');
    pair<string, string> value;
    if (equals_pos == string::npos) {
      value.first = parts[i];
      value.second = "";
    } else {
      value.first = parts[i].substr(0, equals_pos);
      value.second = parts[i].substr(equals_pos + 1);
    }
    output->push_back(value);
  }
}

}  // namespace

CppGenerator::CppGenerator() {}
CppGenerator::~CppGenerator() {}

bool CppGenerator::Generate(const FileDescriptor* file,
                            const string& parameter,
                            OutputDirectory* output_directory,
                            string* error) const {
  vector<pair<string, string> > options;
  ParseOptions(parameter, &options);

  // -----------------------------------------------------------------
  // parse generator options

  // TODO(kenton):  If we ever have more options, we may want to create a
  //   class that encapsulates them which we can pass down to all the
  //   generator classes.  Currently we pass dllexport_decl down to all of
  //   them via the constructors, but we don't want to have to add another
  //   constructor parameter for every option.

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
  string dllexport_decl;

  for (int i = 0; i < options.size(); i++) {
    if (options[i].first == "dllexport_decl") {
      dllexport_decl = options[i].second;
    } else {
      *error = "Unknown generator option: " + options[i].first;
      return false;
    }
  }

  // -----------------------------------------------------------------


  string basename = StripProto(file->name());
  basename.append(".pb");

  FileGenerator file_generator(file, dllexport_decl);

  // Generate header.
  {
    scoped_ptr<io::ZeroCopyOutputStream> output(
      output_directory->Open(basename + ".h"));
    io::Printer printer(output.get(), '$');
    file_generator.GenerateHeader(&printer);
  }

  // Generate cc file.
  {
    scoped_ptr<io::ZeroCopyOutputStream> output(
      output_directory->Open(basename + ".cc"));
    io::Printer printer(output.get(), '$');
    file_generator.GenerateSource(&printer);
  }

  return true;
}

}  // namespace cpp
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
