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

#ifndef GOOGLE_PROTOBUF_COMPILER_JS_GENERATOR_H__
#define GOOGLE_PROTOBUF_COMPILER_JS_GENERATOR_H__

#include <string>
#include <set>

#include <google/protobuf/compiler/code_generator.h>

namespace google {
namespace protobuf {

class Descriptor;
class EnumDescriptor;
class FieldDescriptor;
class OneofDescriptor;
class FileDescriptor;

namespace io { class Printer; }

namespace compiler {
namespace js {

struct GeneratorOptions {
  // Add a `goog.requires()` call for each enum type used. If not set, a forward
  // declaration with `goog.forwardDeclare` is produced instead.
  bool add_require_for_enums;
  // Set this as a test-only module via `goog.setTestOnly();`.
  bool testonly;
  // Output path.
  string output_dir;
  // Namespace prefix.
  string namespace_prefix;
  // Create a library with name <name>_lib.js rather than a separate .js file
  // per type?
  string library;
  // Error if there are two types that would generate the same output file?
  bool error_on_name_conflict;
  // Enable binary-format support?
  bool binary;

  GeneratorOptions()
      : add_require_for_enums(false),
        testonly(false),
        output_dir("."),
        namespace_prefix(""),
        library(""),
        error_on_name_conflict(false),
        binary(false) {}

  bool ParseFromOptions(
      const vector< pair< string, string > >& options,
      string* error);
};

class LIBPROTOC_EXPORT Generator : public CodeGenerator {
 public:
  Generator() {}
  virtual ~Generator() {}

  virtual bool Generate(const FileDescriptor* file,
                        const string& parameter,
                        GeneratorContext* context,
                        string* error) const {
    *error = "Unimplemented Generate() method. Call GenerateAll() instead.";
    return false;
  }

  virtual bool HasGenerateAll() const { return true; }

  virtual bool GenerateAll(const vector<const FileDescriptor*>& files,
                           const string& parameter,
                           GeneratorContext* context,
                           string* error) const;

 private:
  void GenerateHeader(const GeneratorOptions& options,
                      io::Printer* printer) const;

  // Generate goog.provides() calls.
  void FindProvides(const GeneratorOptions& options,
                    io::Printer* printer,
                    const vector<const FileDescriptor*>& file,
                    std::set<string>* provided) const;
  void FindProvidesForMessage(const GeneratorOptions& options,
                              io::Printer* printer,
                              const Descriptor* desc,
                              std::set<string>* provided) const;
  void FindProvidesForEnum(const GeneratorOptions& options,
                           io::Printer* printer,
                           const EnumDescriptor* enumdesc,
                           std::set<string>* provided) const;
  // For extension fields at file scope.
  void FindProvidesForFields(const GeneratorOptions& options,
                             io::Printer* printer,
                             const vector<const FieldDescriptor*>& fields,
                             std::set<string>* provided) const;
  // Print the goog.provides() found by the methods above.
  void GenerateProvides(const GeneratorOptions& options,
                        io::Printer* printer,
                        std::set<string>* provided) const;

  // Generate goog.setTestOnly() if indicated.
  void GenerateTestOnly(const GeneratorOptions& options,
                        io::Printer* printer) const;

  // Generate goog.requires() calls.
  void GenerateRequires(const GeneratorOptions& options,
                        io::Printer* printer,
                        const vector<const FileDescriptor*>& file,
                        std::set<string>* provided) const;
  void GenerateRequires(const GeneratorOptions& options,
                        io::Printer* printer,
                        const Descriptor* desc,
                        std::set<string>* provided) const;
  // For extension fields at file scope.
  void GenerateRequires(const GeneratorOptions& options,
                        io::Printer* printer,
                        const vector<const FieldDescriptor*>& fields,
                        std::set<string>* provided) const;
  void GenerateRequiresImpl(const GeneratorOptions& options,
                            io::Printer* printer,
                            std::set<string>* required,
                            std::set<string>* forwards,
                            std::set<string>* provided,
                            bool require_jspb,
                            bool require_extension) const;
  void FindRequiresForMessage(const GeneratorOptions& options,
                              const Descriptor* desc,
                              std::set<string>* required,
                              std::set<string>* forwards,
                              bool* have_message) const;
  void FindRequiresForField(const GeneratorOptions& options,
                            const FieldDescriptor* field,
                            std::set<string>* required,
                            std::set<string>* forwards) const;
  void FindRequiresForExtension(const GeneratorOptions& options,
                                const FieldDescriptor* field,
                                std::set<string>* required,
                                std::set<string>* forwards) const;

  // Generate definitions for all message classes and enums in all files,
  // processing the files in dependence order.
  void GenerateFilesInDepOrder(const GeneratorOptions& options,
                               io::Printer* printer,
                               const vector<const FileDescriptor*>& file) const;
  // Helper for above.
  void GenerateFileAndDeps(const GeneratorOptions& options,
                           io::Printer* printer,
                           const FileDescriptor* root,
                           std::set<const FileDescriptor*>* all_files,
                           std::set<const FileDescriptor*>* generated) const;

  // Generate definitions for all message classes and enums.
  void GenerateClassesAndEnums(const GeneratorOptions& options,
                               io::Printer* printer,
                               const FileDescriptor* file) const;

  // Generate definition for one class.
  void GenerateClass(const GeneratorOptions& options,
                     io::Printer* printer,
                     const Descriptor* desc) const;
  void GenerateClassConstructor(const GeneratorOptions& options,
                                io::Printer* printer,
                                const Descriptor* desc) const;
  void GenerateClassFieldInfo(const GeneratorOptions& options,
                              io::Printer* printer,
                              const Descriptor* desc) const;
  void GenerateClassXid(const GeneratorOptions& options,
                        io::Printer* printer,
                        const Descriptor* desc) const;
  void GenerateOneofCaseDefinition(const GeneratorOptions& options,
                                   io::Printer* printer,
                                   const OneofDescriptor* oneof) const;
  void GenerateClassToObject(const GeneratorOptions& options,
                             io::Printer* printer,
                             const Descriptor* desc) const;
  void GenerateClassFieldToObject(const GeneratorOptions& options,
                                  io::Printer* printer,
                                  const FieldDescriptor* field) const;
  void GenerateClassFromObject(const GeneratorOptions& options,
                               io::Printer* printer,
                               const Descriptor* desc) const;
  void GenerateClassFieldFromObject(const GeneratorOptions& options,
                                    io::Printer* printer,
                                    const FieldDescriptor* field) const;
  void GenerateClassClone(const GeneratorOptions& options,
                          io::Printer* printer,
                          const Descriptor* desc) const;
  void GenerateClassRegistration(const GeneratorOptions& options,
                                 io::Printer* printer,
                                 const Descriptor* desc) const;
  void GenerateClassFields(const GeneratorOptions& options,
                           io::Printer* printer,
                           const Descriptor* desc) const;
  void GenerateClassField(const GeneratorOptions& options,
                          io::Printer* printer,
                          const FieldDescriptor* desc) const;
  void GenerateClassExtensionFieldInfo(const GeneratorOptions& options,
                                       io::Printer* printer,
                                       const Descriptor* desc) const;
  void GenerateClassDeserialize(const GeneratorOptions& options,
                                io::Printer* printer,
                                const Descriptor* desc) const;
  void GenerateClassDeserializeBinary(const GeneratorOptions& options,
                                      io::Printer* printer,
                                      const Descriptor* desc) const;
  void GenerateClassDeserializeBinaryField(const GeneratorOptions& options,
                                           io::Printer* printer,
                                           const FieldDescriptor* field) const;
  void GenerateClassSerializeBinary(const GeneratorOptions& options,
                                    io::Printer* printer,
                                    const Descriptor* desc) const;
  void GenerateClassSerializeBinaryField(const GeneratorOptions& options,
                                         io::Printer* printer,
                                         const FieldDescriptor* field) const;

  // Generate definition for one enum.
  void GenerateEnum(const GeneratorOptions& options,
                    io::Printer* printer,
                    const EnumDescriptor* enumdesc) const;

  // Generate an extension definition.
  void GenerateExtension(const GeneratorOptions& options,
                         io::Printer* printer,
                         const FieldDescriptor* field) const;

  GOOGLE_DISALLOW_EVIL_CONSTRUCTORS(Generator);
};

}  // namespace js
}  // namespace compiler
}  // namespace protobuf

}  // namespace google
#endif  // GOOGLE_PROTOBUF_COMPILER_JS_GENERATOR_H__
