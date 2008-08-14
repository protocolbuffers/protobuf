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

// Author: jonskeet@google.com (Jon Skeet)
//  Following the Java generator by kenton@google.com (Kenton Varda)
//  Based on original Protocol Buffers design by
//  Sanjay Ghemawat, Jeff Dean, and others.

#include <google/protobuf/compiler/csharp/csharp_file.h>
#include <google/protobuf/compiler/csharp/csharp_enum.h>
#include <google/protobuf/compiler/csharp/csharp_service.h>
#include <google/protobuf/compiler/csharp/csharp_extension.h>
#include <google/protobuf/compiler/csharp/csharp_helpers.h>
#include <google/protobuf/compiler/csharp/csharp_message.h>
#include <google/protobuf/compiler/code_generator.h>
#include <google/protobuf/io/printer.h>
#include <google/protobuf/io/zero_copy_stream.h>
#include <google/protobuf/descriptor.pb.h>
#include <google/protobuf/stubs/strutil.h>

namespace google {
namespace protobuf {
namespace compiler {
namespace csharp {

FileGenerator::FileGenerator(const FileDescriptor* file)
  : file_(file),
    csharp_namespace_(FileJavaPackage(file)),
    classname_(FileClassName(file)) {}

FileGenerator::~FileGenerator() {}

bool FileGenerator::Validate(string* error) {
  // Check that no class name matches the file's class name.  This is a common
  // problem that leads to Java compile errors that can be hard to understand.
  // It's especially bad when using the csharp_multiple_files, since we would
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
      "Please either rename the type or use the csharp_outer_classname "
      "option to specify a different outer class name for the .proto file.");
    return false;
  }

  return true;
}

// Shameless copy of CEscape, but using backslash-u encoding
// Still won't really work for anything non-ASCII.
static int UnicodeEscapeInternal(const char* src, int src_len, char* dest,
                           int dest_len) {
  const char* src_end = src + src_len;
  int used = 0;
  for (; src < src_end; src++) {
    if (dest_len - used < 2)   // Need space for two letter escape
      return -1;

    bool is_hex_escape = false;
    switch (*src) {
      case '\n': dest[used++] = '\\'; dest[used++] = 'n';  break;
      case '\r': dest[used++] = '\\'; dest[used++] = 'r';  break;
      case '\t': dest[used++] = '\\'; dest[used++] = 't';  break;
      case '\"': dest[used++] = '\\'; dest[used++] = '\"'; break;
      case '\'': dest[used++] = '\\'; dest[used++] = '\''; break;
      case '\\': dest[used++] = '\\'; dest[used++] = '\\'; break;
      default:
        if (*src < 0x20 || *src > 0x7E) {
          if (dest_len - used < 6) // need space for 6 letter escape
            return -1;
          sprintf(dest + used, "\\u00%02x", static_cast<uint8>(*src));
          used += 6;
        } else {
          dest[used++] = *src; break;
        }
    }
  }

  if (dest_len - used < 1)   // make sure that there is room for \0
    return -1;

  dest[used] = '\0';   // doesn't count towards return value though
  return used;
}

string UnicodeEscape(const string& src) {
  const int dest_length = src.size() * 6 + 1; // Maximum possible expansion
  scoped_array<char> dest(new char[dest_length]);
  const int len = UnicodeEscapeInternal(src.data(), src.size(),
                                        dest.get(), dest_length);
  GOOGLE_DCHECK_GE(len, 0);
  return string(dest.get(), len);
}


void FileGenerator::Generate(io::Printer* printer) {
  // We don't import anything because we refer to all classes by their
  // fully-qualified names in the generated source.
  printer->Print(
    "// Generated by the protocol buffer compiler.  DO NOT EDIT!\r\n"
    "\r\n");
  if (!csharp_namespace_.empty()) {
    printer->Print(
      "package $package$;\r\n"
      "\r\n",
      "package", csharp_namespace_);
  }
  printer->Print(
    "public final class $classname$ {\r\n"
    "  private $classname$() {}\r\n",
    "classname", classname_);
  printer->Indent();

  // -----------------------------------------------------------------

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
    "public static com.google.protobuf.Descriptors.FileDescriptor\r\n"
    "    getDescriptor() {\r\n"
    "  return descriptor;\r\n"
    "}\r\n"
    "private static final com.google.protobuf.Descriptors.FileDescriptor\r\n"
    "    descriptor = buildDescriptor();\r\n"
    "private static\r\n"
    "    com.google.protobuf.Descriptors.FileDescriptor\r\n"
    "    buildDescriptor() {\r\n"
    "  string descriptorData =\r\n");
  printer->Indent();
  printer->Indent();

  // Only write 40 bytes per line.
  static const int kBytesPerLine = 40;
  for (int i = 0; i < file_data.size(); i += kBytesPerLine) {
    if (i > 0) printer->Print(" +\r\n");
    printer->Print("\"$data$\"",
      "data", UnicodeEscape(file_data.substr(i, kBytesPerLine)));
  }
  printer->Print(";\r\n");

  printer->Outdent();
  printer->Print(
    "try {\r\n"
    "  return com.google.protobuf.Descriptors.FileDescriptor\r\n"
    "    .internalBuildGeneratedFileFrom(descriptorData,\r\n"
    "      new com.google.protobuf.Descriptors.FileDescriptor[] {\r\n");

  for (int i = 0; i < file_->dependency_count(); i++) {
    printer->Print(
      "        $dependency$.getDescriptor(),\r\n",
      "dependency", ClassName(file_->dependency(i)));
  }

  printer->Print(
    "      });\r\n"
    "} catch (Exception e) {\r\n"
    "  throw new RuntimeException(\r\n"
    "    \"Failed to parse protocol buffer descriptor for \" +\r\n"
    "    \"\\\"$filename$\\\".\", e);\r\n"
    "}\r\n",
    "filename", file_->name());

  printer->Outdent();
  printer->Print(
    "}\r\n"
    "\r\n");

  // -----------------------------------------------------------------

  if (!file_->options().csharp_multiple_files()) {
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

  printer->Outdent();
  printer->Print("}\r\n");
}

template<typename GeneratorClass, typename DescriptorClass>
static void GenerateSibling(const string& package_dir,
                            const string& csharp_namespace,
                            const DescriptorClass* descriptor,
                            OutputDirectory* output_directory,
                            vector<string>* file_list) {
  string filename = package_dir + descriptor->name() + ".cs";
  file_list->push_back(filename);

  scoped_ptr<io::ZeroCopyOutputStream> output(
    output_directory->Open(filename));
  io::Printer printer(output.get(), '$');

  printer.Print(
    "// Generated by the protocol buffer compiler.  DO NOT EDIT!\r\n"
    "\r\n");
  if (!csharp_namespace.empty()) {
    printer.Print(
      "package $package$;\r\n"
      "\r\n",
      "package", csharp_namespace);
  }

  GeneratorClass(descriptor).Generate(&printer);
}

void FileGenerator::GenerateSiblings(const string& package_dir,
                                     OutputDirectory* output_directory,
                                     vector<string>* file_list) {
  if (file_->options().csharp_multiple_files()) {
    for (int i = 0; i < file_->enum_type_count(); i++) {
      GenerateSibling<EnumGenerator>(package_dir, csharp_namespace_,
                                     file_->enum_type(i),
                                     output_directory, file_list);
    }
    for (int i = 0; i < file_->message_type_count(); i++) {
      GenerateSibling<MessageGenerator>(package_dir, csharp_namespace_,
                                        file_->message_type(i),
                                        output_directory, file_list);
    }
    for (int i = 0; i < file_->service_count(); i++) {
      GenerateSibling<ServiceGenerator>(package_dir, csharp_namespace_,
                                        file_->service(i),
                                        output_directory, file_list);
    }
  }
}

}  // namespace csharp
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
