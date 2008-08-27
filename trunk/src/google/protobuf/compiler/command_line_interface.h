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
// Implements the Protocol Compiler front-end such that it may be reused by
// custom compilers written to support other languages.

#ifndef GOOGLE_PROTOBUF_COMPILER_COMMAND_LINE_INTERFACE_H__
#define GOOGLE_PROTOBUF_COMPILER_COMMAND_LINE_INTERFACE_H__

#include <google/protobuf/stubs/common.h>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <utility>

namespace google {
namespace protobuf {

class FileDescriptor;        // descriptor.h
class DescriptorPool;        // descriptor.h

namespace compiler {

class CodeGenerator;        // code_generator.h
class DiskSourceTree;       // importer.h

// This class implements the command-line interface to the protocol compiler.
// It is designed to make it very easy to create a custom protocol compiler
// supporting the languages of your choice.  For example, if you wanted to
// create a custom protocol compiler binary which includes both the regular
// C++ support plus support for your own custom output "Foo", you would
// write a class "FooGenerator" which implements the CodeGenerator interface,
// then write a main() procedure like this:
//
//   int main(int argc, char* argv[]) {
//     google::protobuf::compiler::CommandLineInterface cli;
//
//     // Support generation of C++ source and headers.
//     google::protobuf::compiler::cpp::CppGenerator cpp_generator;
//     cli.RegisterGenerator("--cpp_out", &cpp_generator,
//       "Generate C++ source and header.");
//
//     // Support generation of Foo code.
//     FooGenerator foo_generator;
//     cli.RegisterGenerator("--foo_out", &foo_generator,
//       "Generate Foo file.");
//
//     return cli.Run(argc, argv);
//   }
//
// The compiler is invoked with syntax like:
//   protoc --cpp_out=outdir --foo_out=outdir --proto_path=src src/foo.proto
//
// For a full description of the command-line syntax, invoke it with --help.
class LIBPROTOC_EXPORT CommandLineInterface {
 public:
  CommandLineInterface();
  ~CommandLineInterface();

  // Register a code generator for a language.
  //
  // Parameters:
  // * flag_name: The command-line flag used to specify an output file of
  //   this type.  The name must start with a '-'.  If the name is longer
  //   than one letter, it must start with two '-'s.
  // * generator: The CodeGenerator which will be called to generate files
  //   of this type.
  // * help_text: Text describing this flag in the --help output.
  //
  // Some generators accept extra parameters.  You can specify this parameter
  // on the command-line by placing it before the output directory, separated
  // by a colon:
  //   protoc --foo_out=enable_bar:outdir
  // The text before the colon is passed to CodeGenerator::Generate() as the
  // "parameter".
  void RegisterGenerator(const string& flag_name,
                         CodeGenerator* generator,
                         const string& help_text);

  // Run the Protocol Compiler with the given command-line parameters.
  // Returns the error code which should be returned by main().
  //
  // It may not be safe to call Run() in a multi-threaded environment because
  // it calls strerror().  I'm not sure why you'd want to do this anyway.
  int Run(int argc, const char* const argv[]);

  // Call SetInputsAreCwdRelative(true) if the input files given on the command
  // line should be interpreted relative to the proto import path specified
  // using --proto_path or -I flags.  Otherwise, input file names will be
  // interpreted relative to the current working directory (or as absolute
  // paths if they start with '/'), though they must still reside inside
  // a directory given by --proto_path or the compiler will fail.  The latter
  // mode is generally more intuitive and easier to use, especially e.g. when
  // defining implicit rules in Makefiles.
  void SetInputsAreProtoPathRelative(bool enable) {
    inputs_are_proto_path_relative_ = enable;
  }

  // Provides some text which will be printed when the --version flag is
  // used.  The version of libprotoc will also be printed on the next line
  // after this text.
  void SetVersionInfo(const string& text) {
    version_info_ = text;
  }


 private:
  // -----------------------------------------------------------------

  class ErrorPrinter;
  class DiskOutputDirectory;
  class ErrorReportingFileOutput;

  // Clear state from previous Run().
  void Clear();

  // Remaps each file in input_files_ so that it is relative to one of the
  // directories in proto_path_.  Returns false if an error occurred.  This
  // is only used if inputs_are_proto_path_relative_ is false.
  bool MakeInputsBeProtoPathRelative(
    DiskSourceTree* source_tree);

  // Parse all command-line arguments.
  bool ParseArguments(int argc, const char* const argv[]);

  // Parses a command-line argument into a name/value pair.  Returns
  // true if the next argument in the argv should be used as the value,
  // false otherwise.
  //
  // Exmaples:
  //   "-Isrc/protos" ->
  //     name = "-I", value = "src/protos"
  //   "--cpp_out=src/foo.pb2.cc" ->
  //     name = "--cpp_out", value = "src/foo.pb2.cc"
  //   "foo.proto" ->
  //     name = "", value = "foo.proto"
  bool ParseArgument(const char* arg, string* name, string* value);

  // Interprets arguments parsed with ParseArgument.
  bool InterpretArgument(const string& name, const string& value);

  // Print the --help text to stderr.
  void PrintHelpText();

  // Generate the given output file from the given input.
  struct OutputDirective;  // see below
  bool GenerateOutput(const FileDescriptor* proto_file,
                      const OutputDirective& output_directive);

  // Implements --encode and --decode.
  bool EncodeOrDecode(const DescriptorPool* pool);

  // Implements the --descriptor_set_out option.
  bool WriteDescriptorSet(const vector<const FileDescriptor*> parsed_files);

  // -----------------------------------------------------------------

  // The name of the executable as invoked (i.e. argv[0]).
  string executable_name_;

  // Version info set with SetVersionInfo().
  string version_info_;

  // Map from flag names to registered generators.
  struct GeneratorInfo {
    CodeGenerator* generator;
    string help_text;
  };
  typedef map<string, GeneratorInfo> GeneratorMap;
  GeneratorMap generators_;

  // Stuff parsed from command line.
  enum Mode {
    MODE_COMPILE,  // Normal mode:  parse .proto files and compile them.
    MODE_ENCODE,   // --encode:  read text from stdin, write binary to stdout.
    MODE_DECODE    // --decode:  read binary from stdin, write text to stdout.
  };

  Mode mode_;

  vector<pair<string, string> > proto_path_;  // Search path for proto files.
  vector<string> input_files_;                // Names of the input proto files.

  // output_directives_ lists all the files we are supposed to output and what
  // generator to use for each.
  struct OutputDirective {
    string name;
    CodeGenerator* generator;
    string parameter;
    string output_location;
  };
  vector<OutputDirective> output_directives_;

  // When using --encode or --decode, this names the type we are encoding or
  // decoding.  (Empty string indicates --decode_raw.)
  string codec_type_;

  // If --descriptor_set_out was given, this is the filename to which the
  // FileDescriptorSet should be written.  Otherwise, empty.
  string descriptor_set_name_;

  // True if --include_imports was given, meaning that we should
  // write all transitive dependencies to the DescriptorSet.  Otherwise, only
  // the .proto files listed on the command-line are added.
  bool imports_in_descriptor_set_;

  // Was the --disallow_services flag used?
  bool disallow_services_;

  // See SetInputsAreProtoPathRelative().
  bool inputs_are_proto_path_relative_;

  GOOGLE_DISALLOW_EVIL_CONSTRUCTORS(CommandLineInterface);
};

}  // namespace compiler
}  // namespace protobuf

}  // namespace google
#endif  // GOOGLE_PROTOBUF_COMPILER_COMMAND_LINE_INTERFACE_H__
