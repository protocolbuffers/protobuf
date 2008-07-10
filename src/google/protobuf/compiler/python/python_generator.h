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

// Author: robinson@google.com (Will Robinson)
//
// Generates Python code for a given .proto file.

#ifndef GOOGLE_PROTOBUF_COMPILER_PYTHON_GENERATOR_H__
#define GOOGLE_PROTOBUF_COMPILER_PYTHON_GENERATOR_H__

#include <string>

#include <google/protobuf/compiler/code_generator.h>
#include <google/protobuf/stubs/common.h>

namespace google {
namespace protobuf {

class Descriptor;
class EnumDescriptor;
class EnumValueDescriptor;
class FieldDescriptor;
class ServiceDescriptor;

namespace io { class Printer; }

namespace compiler {
namespace python {

// CodeGenerator implementation for generated Python protocol buffer classes.
// If you create your own protocol compiler binary and you want it to support
// Python output, you can do so by registering an instance of this
// CodeGenerator with the CommandLineInterface in your main() function.
class LIBPROTOC_EXPORT Generator : public CodeGenerator {
 public:
  Generator();
  virtual ~Generator();

  // CodeGenerator methods.
  virtual bool Generate(const FileDescriptor* file,
                        const string& parameter,
                        OutputDirectory* output_directory,
                        string* error) const;

 private:
  void PrintImports() const;
  void PrintTopLevelEnums() const;
  void PrintAllNestedEnumsInFile() const;
  void PrintNestedEnums(const Descriptor& descriptor) const;
  void PrintEnum(const EnumDescriptor& enum_descriptor) const;

  void PrintTopLevelExtensions() const;

  void PrintFieldDescriptor(
      const FieldDescriptor& field, bool is_extension) const;
  void PrintFieldDescriptorsInDescriptor(
      const Descriptor& message_descriptor,
      bool is_extension,
      const string& list_variable_name,
      int (Descriptor::*CountFn)() const,
      const FieldDescriptor* (Descriptor::*GetterFn)(int) const) const;
  void PrintFieldsInDescriptor(const Descriptor& message_descriptor) const;
  void PrintExtensionsInDescriptor(const Descriptor& message_descriptor) const;
  void PrintMessageDescriptors() const;
  void PrintDescriptor(const Descriptor& message_descriptor) const;
  void PrintNestedDescriptors(const Descriptor& containing_descriptor) const;

  void PrintMessages() const;
  void PrintMessage(const Descriptor& message_descriptor) const;
  void PrintNestedMessages(const Descriptor& containing_descriptor) const;

  void FixForeignFieldsInDescriptors() const;
  void FixForeignFieldsInDescriptor(const Descriptor& descriptor) const;
  void FixForeignFieldsInField(const Descriptor* containing_type,
                               const FieldDescriptor& field,
                               const string& python_dict_name) const;
  string FieldReferencingExpression(const Descriptor* containing_type,
                                    const FieldDescriptor& field,
                                    const string& python_dict_name) const;

  void FixForeignFieldsInExtensions() const;
  void FixForeignFieldsInExtension(
      const FieldDescriptor& extension_field) const;
  void FixForeignFieldsInNestedExtensions(const Descriptor& descriptor) const;

  void PrintServices() const;
  void PrintServiceDescriptor(const ServiceDescriptor& descriptor) const;
  void PrintServiceClass(const ServiceDescriptor& descriptor) const;
  void PrintServiceStub(const ServiceDescriptor& descriptor) const;

  void PrintEnumValueDescriptor(const EnumValueDescriptor& descriptor) const;
  string OptionsValue(const string& class_name,
                      const string& serialized_options) const;
  bool GeneratingDescriptorProto() const;

  template <typename DescriptorT>
  string ModuleLevelDescriptorName(const DescriptorT& descriptor) const;
  string ModuleLevelMessageName(const Descriptor& descriptor) const;
  string ModuleLevelServiceDescriptorName(
      const ServiceDescriptor& descriptor) const;

  // Very coarse-grained lock to ensure that Generate() is reentrant.
  // Guards file_ and printer_.
  mutable Mutex mutex_;
  mutable const FileDescriptor* file_;  // Set in Generate().  Under mutex_.
  mutable io::Printer* printer_;  // Set in Generate().  Under mutex_.

  GOOGLE_DISALLOW_EVIL_CONSTRUCTORS(Generator);
};

}  // namespace python
}  // namespace compiler
}  // namespace protobuf

}  // namespace google
#endif  // GOOGLE_PROTOBUF_COMPILER_PYTHON_GENERATOR_H__
