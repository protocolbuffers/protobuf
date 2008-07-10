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

#include <google/protobuf/compiler/java/java_generator.h>
#include <google/protobuf/compiler/java/java_file.h>
#include <google/protobuf/compiler/java/java_helpers.h>
#include <google/protobuf/io/printer.h>
#include <google/protobuf/io/zero_copy_stream.h>
#include <google/protobuf/descriptor.pb.h>
#include <google/protobuf/stubs/strutil.h>

namespace google {
namespace protobuf {
namespace compiler {
namespace java {

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

JavaGenerator::JavaGenerator() {}
JavaGenerator::~JavaGenerator() {}

bool JavaGenerator::Generate(const FileDescriptor* file,
                             const string& parameter,
                             OutputDirectory* output_directory,
                             string* error) const {
  vector<pair<string, string> > options;
  ParseOptions(parameter, &options);

  // -----------------------------------------------------------------
  // parse generator options

  // Name a file where we will write a list of generated file names, one
  // per line.
  string output_list_file;

  for (int i = 0; i < options.size(); i++) {
    if (options[i].first == "output_list_file") {
      output_list_file = options[i].second;
    } else {
      *error = "Unknown generator option: " + options[i].first;
      return false;
    }
  }


  // -----------------------------------------------------------------


  FileGenerator file_generator(file);
  if (!file_generator.Validate(error)) {
    return false;
  }

  string package_dir =
    StringReplace(file_generator.java_package(), ".", "/", true);
  if (!package_dir.empty()) package_dir += "/";

  vector<string> all_files;

  string java_filename = package_dir;
  java_filename += file_generator.classname();
  java_filename += ".java";
  all_files.push_back(java_filename);

  // Generate main java file.
  scoped_ptr<io::ZeroCopyOutputStream> output(
    output_directory->Open(java_filename));
  io::Printer printer(output.get(), '$');
  file_generator.Generate(&printer);

  // Generate sibling files.
  file_generator.GenerateSiblings(package_dir, output_directory, &all_files);

  // Generate output list if requested.
  if (!output_list_file.empty()) {
    // Generate output list.  This is just a simple text file placed in a
    // deterministic location which lists the .java files being generated.
    scoped_ptr<io::ZeroCopyOutputStream> srclist_raw_output(
      output_directory->Open(output_list_file));
    io::Printer srclist_printer(srclist_raw_output.get(), '$');
    for (int i = 0; i < all_files.size(); i++) {
      srclist_printer.Print("$filename$\n", "filename", all_files[i]);
    }
  }

  return true;
}

}  // namespace java
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
