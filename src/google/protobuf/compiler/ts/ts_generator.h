// Protocol Buffers - Google's data interchange format
// Copyright 2020 Levi Behunin.  All rights reserved.
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

// Generates TypeScript code for a given .proto file.
//
#ifndef GOOGLE_PROTOBUF_COMPILER_TS_GENERATOR_H__
#define GOOGLE_PROTOBUF_COMPILER_TS_GENERATOR_H__

#include <google/protobuf/compiler/code_generator.h>

#include <string>

// Must be included last.
#include <google/protobuf/port_def.inc>

namespace google {
namespace protobuf {

class Descriptor;
class EnumDescriptor;
class FieldDescriptor;
class OneofDescriptor;
class FileDescriptor;
class ServiceDescriptor;

namespace io {
class Printer;
}

namespace compiler {
namespace ts {

struct GeneratorOptions {
  // Output path.
  std::string output_dir;
  // Namespace prefix.
  std::string name;
  // Input to Output ratio
  std::string ratio;
  // Dependencies handling
  std::string deps;
  // Generate generic services.
  bool services;

  GeneratorOptions()
      : output_dir("."),
        name("YourNamespaceHere"),
        ratio(""),
        deps(""),
        services(false),
        library(""),
        extension(".ts") {}

  bool ParseFromOptions(
      const std::vector<std::pair<std::string, std::string>>& options,
      std::string* error);

  // Returns the file name extension to use for generated code.
  std::string GetFileNameExtension() const { return extension; }

  enum OutputMode {
    // Create an output file for each input .proto file.
    kOneOutputFilePerInputFile,
    // Create an output file for each type.
    kOneOutputFilePerService,
    // Put everything in a single file named by the library option.
    kEverythingInOneFile,
  };

  // Indicates how to output the generated code based on the provided options.
  OutputMode output_mode() const;
  // Create a library with name <name>_lib.ts rather than a separate .ts file
  // per type?
  std::string library;
  // The extension to use for output file names.
  std::string extension;
};

// CodeGenerator implementation which generates a TypeScript source file and
// header.  If you create your own protocol compiler binary and you want it to
// support TypeScript output, you can do so by registering an instance of this
// CodeGenerator with the CommandLineInterface in your main() function.
class PROTOC_EXPORT Generator : public CodeGenerator {
 public:
  Generator() {}
  virtual ~Generator() {}

  virtual bool Generate(const FileDescriptor* file,
                        const std::string& parameter, GeneratorContext* context,
                        std::string* error) const {
    *error = "Unimplemented Generate() method. Call GenerateAll() instead.";
    return false;
  }

  virtual bool HasGenerateAll() const { return true; }

  virtual bool GenerateAll(const std::vector<const FileDescriptor*>& files,
                           const std::string& parameter,
                           GeneratorContext* context, std::string* error) const;

 private:
  void Header(std::map<std::string, std::string>* vars,
              io::Printer* printer) const;
  void ES6Imports(const FileDescriptor* file, io::Printer* printer,
                  std::map<std::string, std::string>* vars) const;
  bool File(const FileDescriptor* file, const GeneratorOptions& options,
            GeneratorContext* context) const;
  void File(io::Printer* printer, const FileDescriptor* file,
            const GeneratorOptions& options,
            std::map<std::string, std::string>* vars) const;
  void ClassesAndEnums(io::Printer* printer, const FileDescriptor* file,
                       std::map<std::string, std::string>* vars) const;
  void ClassConstructor(io::Printer* printer,
                        std::map<std::string, std::string>* vars) const;
  void Class(io::Printer* printer, const Descriptor* desc,
             std::map<std::string, std::string>* vars) const;
  void Fields(io::Printer* printer, const Descriptor* desc,
              std::map<std::string, std::string>* vars) const;
  void Field(io::Printer* printer, const FieldDescriptor* desc,
             std::map<std::string, std::string>* vars) const;
  void DeserializeBinary(io::Printer* printer, const Descriptor* desc,
                         std::map<std::string, std::string>* vars) const;
  void DeserializeBinaryField(io::Printer* printer,
                              const FieldDescriptor* field,
                              std::map<std::string, std::string>* vars,
                              bool tmp = false) const;
  void SerializeBinary(io::Printer* printer, const Descriptor* desc,
                       std::map<std::string, std::string>* vars) const;
  void SerializeBinaryField(io::Printer* printer, const FieldDescriptor* field,
                            std::map<std::string, std::string>* vars) const;
  void OneofCaseDefinition(io::Printer* printer, const OneofDescriptor* oneof,
                           std::map<std::string, std::string>* vars) const;
  void Nested(io::Printer* printer, const Descriptor* desc,
              std::map<std::string, std::string>* vars) const;
  void Enum(io::Printer* printer, const EnumDescriptor* enumdesc,
            std::map<std::string, std::string>* vars) const;
  void ServiceUtil(io::Printer* printer) const;
  void ServiceClass(io::Printer* printer, const ServiceDescriptor* des,
                    std::map<std::string, std::string>* vars) const;

  GOOGLE_DISALLOW_EVIL_CONSTRUCTORS(Generator);
};

}  // namespace ts
}  // namespace compiler
}  // namespace protobuf
}  // namespace google

#include <google/protobuf/port_undef.inc>

#endif  // GOOGLE_PROTOBUF_COMPILER_TS_GENERATOR_H__
