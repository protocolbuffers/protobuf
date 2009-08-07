// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
// http://code.google.com/p/protobuf/
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

#include <google/protobuf/compiler/java/java_file.h>
#include <google/protobuf/compiler/java/java_enum.h>
#include <google/protobuf/compiler/java/java_service.h>
#include <google/protobuf/compiler/java/java_extension.h>
#include <google/protobuf/compiler/java/java_helpers.h>
#include <google/protobuf/compiler/java/java_message.h>
#include <google/protobuf/compiler/code_generator.h>
#include <google/protobuf/io/printer.h>
#include <google/protobuf/io/zero_copy_stream.h>
#include <google/protobuf/descriptor.pb.h>
#include <google/protobuf/stubs/strutil.h>

namespace google {
namespace protobuf {
namespace compiler {
namespace java {

namespace {

// Recursively searches the given message to see if it contains any extensions.
bool UsesExtensions(const Message& message) {
  const Reflection* reflection = message.GetReflection();

  // We conservatively assume that unknown fields are extensions.
  if (reflection->GetUnknownFields(message).field_count() > 0) return true;

  vector<const FieldDescriptor*> fields;
  reflection->ListFields(message, &fields);

  for (int i = 0; i < fields.size(); i++) {
    if (fields[i]->is_extension()) return true;

    if (fields[i]->cpp_type() == FieldDescriptor::CPPTYPE_MESSAGE) {
      if (fields[i]->is_repeated()) {
        int size = reflection->FieldSize(message, fields[i]);
        for (int j = 0; j < size; j++) {
          const Message& sub_message =
            reflection->GetRepeatedMessage(message, fields[i], j);
          if (UsesExtensions(sub_message)) return true;
        }
      } else {
        const Message& sub_message = reflection->GetMessage(message, fields[i]);
        if (UsesExtensions(sub_message)) return true;
      }
    }
  }

  return false;
}

}  // namespace

FileGenerator::FileGenerator(const FileDescriptor* file)
  : file_(file),
    java_package_(FileJavaPackage(file)),
    classname_(FileClassName(file)) {}

FileGenerator::~FileGenerator() {}

bool FileGenerator::Validate(string* error) {
  // Check that no class name matches the file's class name.  This is a common
  // problem that leads to Java compile errors that can be hard to understand.
  // It's especially bad when using the java_multiple_files, since we would
  // end up overwriting the outer class with one of the inner ones.

  bool found_conflict = false;
  for (int i = 0; i < file_->enum_type_count() && !found_conflict; i++) {
    if (file_->enum_type(i)->name() == classname_) {
      found_conflict = true;
    }
  }
  for (int i = 0; i < file_->message_type_count() && !found_conflict; i++) {
    if (file_->message_type(i)->name() == classname_) {
      found_conflict = true;
    }
  }
  for (int i = 0; i < file_->service_count() && !found_conflict; i++) {
    if (file_->service(i)->name() == classname_) {
      found_conflict = true;
    }
  }

  if (found_conflict) {
    error->assign(file_->name());
    error->append(
      ": Cannot generate Java output because the file's outer class name, \"");
    error->append(classname_);
    error->append(
      "\", matches the name of one of the types declared inside it.  "
      "Please either rename the type or use the java_outer_classname "
      "option to specify a different outer class name for the .proto file.");
    return false;
  }

  return true;
}

void FileGenerator::Generate(io::Printer* printer) {
  // We don't import anything because we refer to all classes by their
  // fully-qualified names in the generated source.
  printer->Print(
    "// Generated by the protocol buffer compiler.  DO NOT EDIT!\n"
    "\n");
  if (!java_package_.empty()) {
    printer->Print(
      "package $package$;\n"
      "\n",
      "package", java_package_);
  }
  printer->Print(
    "public final class $classname$ {\n"
    "  private $classname$() {}\n",
    "classname", classname_);
  printer->Indent();

  // -----------------------------------------------------------------

  printer->Print(
    "public static void registerAllExtensions(\n"
    "    com.google.protobuf.ExtensionRegistry$lite$ registry) {\n",
    "lite", HasDescriptorMethods(file_) ? "" : "Lite");

  printer->Indent();

  for (int i = 0; i < file_->extension_count(); i++) {
    ExtensionGenerator(file_->extension(i)).GenerateRegistrationCode(printer);
  }

  for (int i = 0; i < file_->message_type_count(); i++) {
    MessageGenerator(file_->message_type(i))
      .GenerateExtensionRegistrationCode(printer);
  }

  printer->Outdent();
  printer->Print(
    "}\n");

  // -----------------------------------------------------------------

  if (!file_->options().java_multiple_files()) {
    for (int i = 0; i < file_->enum_type_count(); i++) {
      EnumGenerator(file_->enum_type(i)).Generate(printer);
    }
    for (int i = 0; i < file_->message_type_count(); i++) {
      MessageGenerator(file_->message_type(i)).Generate(printer);
    }
    for (int i = 0; i < file_->service_count(); i++) {
      ServiceGenerator(file_->service(i)).Generate(printer);
    }
  }

  // Extensions must be generated in the outer class since they are values,
  // not classes.
  for (int i = 0; i < file_->extension_count(); i++) {
    ExtensionGenerator(file_->extension(i)).Generate(printer);
  }

  // Static variables.
  for (int i = 0; i < file_->message_type_count(); i++) {
    // TODO(kenton):  Reuse MessageGenerator objects?
    MessageGenerator(file_->message_type(i)).GenerateStaticVariables(printer);
  }

  printer->Print("\n");

  if (HasDescriptorMethods(file_)) {
    GenerateEmbeddedDescriptor(printer);
  } else {
    printer->Print(
      "static {\n");
    printer->Indent();

    for (int i = 0; i < file_->message_type_count(); i++) {
      // TODO(kenton):  Reuse MessageGenerator objects?
      MessageGenerator(file_->message_type(i))
        .GenerateStaticVariableInitializers(printer);
    }

    for (int i = 0; i < file_->extension_count(); i++) {
      // TODO(kenton):  Reuse ExtensionGenerator objects?
      ExtensionGenerator(file_->extension(i))
        .GenerateInitializationCode(printer);
    }

    printer->Outdent();
    printer->Print(
      "}\n");
  }

  // Dummy function we can use to force the static initialization block to
  // run.  Needed by inner classes.  Cannot be private due to
  // java_multiple_files option.
  printer->Print(
    "\n"
    "public static void internalForceInit() {}\n");

  printer->Outdent();
  printer->Print("}\n");
}

void FileGenerator::GenerateEmbeddedDescriptor(io::Printer* printer) {
  // Embed the descriptor.  We simply serialize the entire FileDescriptorProto
  // and embed it as a string literal, which is parsed and built into real
  // descriptors at initialization time.  We unfortunately have to put it in
  // a string literal, not a byte array, because apparently using a literal
  // byte array causes the Java compiler to generate *instructions* to
  // initialize each and every byte of the array, e.g. as if you typed:
  //   b[0] = 123; b[1] = 456; b[2] = 789;
  // This makes huge bytecode files and can easily hit the compiler's internal
  // code size limits (error "code to large").  String literals are apparently
  // embedded raw, which is what we want.
  FileDescriptorProto file_proto;
  file_->CopyTo(&file_proto);
  string file_data;
  file_proto.SerializeToString(&file_data);

  printer->Print(
    "public static com.google.protobuf.Descriptors.FileDescriptor\n"
    "    getDescriptor() {\n"
    "  return descriptor;\n"
    "}\n"
    "private static com.google.protobuf.Descriptors.FileDescriptor\n"
    "    descriptor;\n"
    "static {\n"
    "  java.lang.String[] descriptorData = {\n");
  printer->Indent();
  printer->Indent();

  // Only write 40 bytes per line.
  static const int kBytesPerLine = 40;
  for (int i = 0; i < file_data.size(); i += kBytesPerLine) {
    if (i > 0) {
      // Every 400 lines, start a new string literal, in order to avoid the
      // 64k length limit.
      if (i % 400 == 0) {
        printer->Print(",\n");
      } else {
        printer->Print(" +\n");
      }
    }
    printer->Print("\"$data$\"",
      "data", CEscape(file_data.substr(i, kBytesPerLine)));
  }

  printer->Outdent();
  printer->Print("\n};\n");

  // -----------------------------------------------------------------
  // Create the InternalDescriptorAssigner.

  printer->Print(
    "com.google.protobuf.Descriptors.FileDescriptor."
      "InternalDescriptorAssigner assigner =\n"
    "  new com.google.protobuf.Descriptors.FileDescriptor."
      "InternalDescriptorAssigner() {\n"
    "    public com.google.protobuf.ExtensionRegistry assignDescriptors(\n"
    "        com.google.protobuf.Descriptors.FileDescriptor root) {\n"
    "      descriptor = root;\n");

  printer->Indent();
  printer->Indent();
  printer->Indent();

  for (int i = 0; i < file_->message_type_count(); i++) {
    // TODO(kenton):  Reuse MessageGenerator objects?
    MessageGenerator(file_->message_type(i))
      .GenerateStaticVariableInitializers(printer);
  }

  for (int i = 0; i < file_->extension_count(); i++) {
    // TODO(kenton):  Reuse ExtensionGenerator objects?
    ExtensionGenerator(file_->extension(i))
      .GenerateInitializationCode(printer);
  }

  if (UsesExtensions(file_proto)) {
    // Must construct an ExtensionRegistry containing all possible extensions
    // and return it.
    printer->Print(
      "com.google.protobuf.ExtensionRegistry registry =\n"
      "  com.google.protobuf.ExtensionRegistry.newInstance();\n"
      "registerAllExtensions(registry);\n");
    for (int i = 0; i < file_->dependency_count(); i++) {
      printer->Print(
        "$dependency$.registerAllExtensions(registry);\n",
        "dependency", ClassName(file_->dependency(i)));
    }
    printer->Print(
      "return registry;\n");
  } else {
    printer->Print(
      "return null;\n");
  }

  printer->Outdent();
  printer->Outdent();
  printer->Outdent();

  printer->Print(
    "    }\n"
    "  };\n");

  // -----------------------------------------------------------------
  // Invoke internalBuildGeneratedFileFrom() to build the file.

  printer->Print(
    "com.google.protobuf.Descriptors.FileDescriptor\n"
    "  .internalBuildGeneratedFileFrom(descriptorData,\n"
    "    new com.google.protobuf.Descriptors.FileDescriptor[] {\n");

  for (int i = 0; i < file_->dependency_count(); i++) {
    printer->Print(
      "      $dependency$.getDescriptor(),\n",
      "dependency", ClassName(file_->dependency(i)));
  }

  printer->Print(
    "    }, assigner);\n");

  printer->Outdent();
  printer->Print(
    "}\n");
}

template<typename GeneratorClass, typename DescriptorClass>
static void GenerateSibling(const string& package_dir,
                            const string& java_package,
                            const DescriptorClass* descriptor,
                            OutputDirectory* output_directory,
                            vector<string>* file_list) {
  string filename = package_dir + descriptor->name() + ".java";
  file_list->push_back(filename);

  scoped_ptr<io::ZeroCopyOutputStream> output(
    output_directory->Open(filename));
  io::Printer printer(output.get(), '$');

  printer.Print(
    "// Generated by the protocol buffer compiler.  DO NOT EDIT!\n"
    "\n");
  if (!java_package.empty()) {
    printer.Print(
      "package $package$;\n"
      "\n",
      "package", java_package);
  }

  GeneratorClass(descriptor).Generate(&printer);
}

void FileGenerator::GenerateSiblings(const string& package_dir,
                                     OutputDirectory* output_directory,
                                     vector<string>* file_list) {
  if (file_->options().java_multiple_files()) {
    for (int i = 0; i < file_->enum_type_count(); i++) {
      GenerateSibling<EnumGenerator>(package_dir, java_package_,
                                     file_->enum_type(i),
                                     output_directory, file_list);
    }
    for (int i = 0; i < file_->message_type_count(); i++) {
      GenerateSibling<MessageGenerator>(package_dir, java_package_,
                                        file_->message_type(i),
                                        output_directory, file_list);
    }
    for (int i = 0; i < file_->service_count(); i++) {
      GenerateSibling<ServiceGenerator>(package_dir, java_package_,
                                        file_->service(i),
                                        output_directory, file_list);
    }
  }
}

}  // namespace java
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
