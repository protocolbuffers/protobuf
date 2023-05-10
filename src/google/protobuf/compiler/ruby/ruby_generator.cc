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

#include "google/protobuf/compiler/ruby/ruby_generator.h"

#include <iomanip>
#include <memory>
#include <sstream>

#include "google/protobuf/compiler/code_generator.h"
#include "absl/container/flat_hash_set.h"
#include "absl/log/absl_log.h"
#include "absl/strings/escaping.h"
#include "google/protobuf/compiler/plugin.h"
#include "google/protobuf/compiler/retention.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/descriptor.pb.h"
#include "google/protobuf/descriptor_legacy.h"
#include "google/protobuf/io/printer.h"
#include "google/protobuf/io/zero_copy_stream.h"

namespace google {
namespace protobuf {
namespace compiler {
namespace ruby {

// Forward decls.
template <class numeric_type>
std::string NumberToString(numeric_type value);
std::string GetRequireName(absl::string_view proto_file);
std::string LabelForField(FieldDescriptor* field);
std::string TypeName(FieldDescriptor* field);
void GenerateMessageAssignment(absl::string_view prefix,
                               const Descriptor* message, io::Printer* printer);
void GenerateEnumAssignment(absl::string_view prefix, const EnumDescriptor* en,
                            io::Printer* printer);
std::string DefaultValueForField(const FieldDescriptor* field);

template<class numeric_type>
std::string NumberToString(numeric_type value) {
  std::ostringstream os;
  os << value;
  return os.str();
}

std::string GetRequireName(absl::string_view proto_file) {
  size_t lastindex = proto_file.find_last_of('.');
  return absl::StrCat(proto_file.substr(0, lastindex), "_pb");
}

std::string GetOutputFilename(absl::string_view proto_file) {
  return absl::StrCat(GetRequireName(proto_file), ".rb");
}

// Locale-agnostic utility functions.
bool IsLower(char ch) { return ch >= 'a' && ch <= 'z'; }

bool IsUpper(char ch) { return ch >= 'A' && ch <= 'Z'; }

bool IsAlpha(char ch) { return IsLower(ch) || IsUpper(ch); }

char UpperChar(char ch) { return IsLower(ch) ? (ch - 'a' + 'A') : ch; }


// Package names in protobuf are snake_case by convention, but Ruby module
// names must be PascalCased.
//
//   foo_bar_baz -> FooBarBaz
std::string PackageToModule(absl::string_view name) {
  bool next_upper = true;
  std::string result;
  result.reserve(name.size());

  for (int i = 0; i < name.size(); i++) {
    if (name[i] == '_') {
      next_upper = true;
    } else {
      if (next_upper) {
        result.push_back(UpperChar(name[i]));
      } else {
        result.push_back(name[i]);
      }
      next_upper = false;
    }
  }

  return result;
}

// Class and enum names in protobuf should be PascalCased by convention, but
// since there is nothing enforcing this we need to ensure that they are valid
// Ruby constants.  That mainly means making sure that the first character is
// an upper-case letter.
std::string RubifyConstant(absl::string_view name) {
  std::string ret(name);
  if (!ret.empty()) {
    if (IsLower(ret[0])) {
      // If it starts with a lowercase letter, capitalize it.
      ret[0] = UpperChar(ret[0]);
    } else if (!IsAlpha(ret[0])) {
      // Otherwise (e.g. if it begins with an underscore), we need to come up
      // with some prefix that starts with a capital letter. We could be smarter
      // here, e.g. try to strip leading underscores, but this may cause other
      // problems if the user really intended the name. So let's just prepend a
      // well-known suffix.
      return absl::StrCat("PB_", ret);
    }
  }

  return ret;
}

void GenerateMessageAssignment(absl::string_view prefix,
                               const Descriptor* message,
                               io::Printer* printer) {
  // Don't generate MapEntry messages -- we use the Ruby extension's native
  // support for map fields instead.
  if (message->options().map_entry()) {
    return;
  }

  printer->Print(
    "$prefix$$name$ = ",
    "prefix", prefix,
    "name", RubifyConstant(message->name()));
  printer->Print(
    "::Google::Protobuf::DescriptorPool.generated_pool."
    "lookup(\"$full_name$\").msgclass\n",
    "full_name", message->full_name());

  std::string nested_prefix =
      absl::StrCat(prefix, RubifyConstant(message->name()), "::");
  for (int i = 0; i < message->nested_type_count(); i++) {
    GenerateMessageAssignment(nested_prefix, message->nested_type(i), printer);
  }
  for (int i = 0; i < message->enum_type_count(); i++) {
    GenerateEnumAssignment(nested_prefix, message->enum_type(i), printer);
  }
}

void GenerateEnumAssignment(absl::string_view prefix, const EnumDescriptor* en,
                            io::Printer* printer) {
  printer->Print(
    "$prefix$$name$ = ",
    "prefix", prefix,
    "name", RubifyConstant(en->name()));
  printer->Print(
    "::Google::Protobuf::DescriptorPool.generated_pool."
    "lookup(\"$full_name$\").enummodule\n",
    "full_name", en->full_name());
}

int GeneratePackageModules(const FileDescriptor* file, io::Printer* printer) {
  int levels = 0;
  bool need_change_to_module = true;
  std::string package_name;

  // Determine the name to use in either format:
  //   proto package:         one.two.three
  //   option ruby_package:   One::Two::Three
  if (file->options().has_ruby_package()) {
    package_name = file->options().ruby_package();

    // If :: is in the package use the Ruby formatted name as-is
    //    -> A::B::C
    // otherwise, use the dot separator
    //    -> A.B.C
    if (package_name.find("::") != std::string::npos) {
      need_change_to_module = false;
    } else if (package_name.find('.') != std::string::npos) {
      ABSL_LOG(WARNING) << "ruby_package option should be in the form of:"
                        << " 'A::B::C' and not 'A.B.C'";
    }
  } else {
    package_name = file->package();
  }

  // Use the appropriate delimiter
  std::string delimiter = need_change_to_module ? "." : "::";
  int delimiter_size = need_change_to_module ? 1 : 2;

  // Extract each module name and indent
  while (!package_name.empty()) {
    size_t dot_index = package_name.find(delimiter);
    std::string component;
    if (dot_index == std::string::npos) {
      component = package_name;
      package_name = "";
    } else {
      component = package_name.substr(0, dot_index);
      package_name = package_name.substr(dot_index + delimiter_size);
    }
    if (need_change_to_module) {
      component = PackageToModule(component);
    }
    printer->Print(
      "module $name$\n",
      "name", component);
    printer->Indent();
    levels++;
  }
  return levels;
}

void EndPackageModules(int levels, io::Printer* printer) {
  while (levels > 0) {
    levels--;
    printer->Outdent();
    printer->Print(
      "end\n");
  }
}

std::string SerializedDescriptor(const FileDescriptor* file) {
  FileDescriptorProto file_proto = StripSourceRetentionOptions(*file);
  std::string file_data;
  file_proto.SerializeToString(&file_data);
  return file_data;
}

template <class F>
void ForEachField(const Descriptor* d, F&& func) {
  for (int i = 0; i < d->field_count(); i++) {
    func(d->field(i));
  }
  for (int i = 0; i < d->nested_type_count(); i++) {
    ForEachField(d->nested_type(i), func);
  }
}

template <class F>
void ForEachField(const FileDescriptor* file, F&& func) {
  for (int i = 0; i < file->message_type_count(); i++) {
    ForEachField(file->message_type(i), func);
  }
  for (int i = 0; i < file->extension_count(); i++) {
    func(file->extension(i));
  }
}

std::string DumpImportList(const FileDescriptor* file) {
  // For each import, find a symbol that comes from that file.
  absl::flat_hash_set<const FileDescriptor*> seen{file};
  std::string ret;
  ForEachField(file, [&](const FieldDescriptor* field) {
    if (!field->message_type()) return;
    const FileDescriptor* f = field->message_type()->file();
    if (!seen.insert(f).second) return;
    absl::StrAppend(&ret, "    [\"", field->message_type()->full_name(),
                    "\", \"", f->name(), "\"],\n");
  });
  return ret;
}

void GenerateBinaryDescriptor(const FileDescriptor* file, io::Printer* printer,
                              std::string* error) {
  printer->Print(R"(
descriptor_data = "$descriptor_data$"

pool = Google::Protobuf::DescriptorPool.generated_pool

begin
  pool.add_serialized_file(descriptor_data)
rescue TypeError => e
  # Compatibility code: will be removed in the next major version.
  require 'google/protobuf/descriptor_pb'
  parsed = Google::Protobuf::FileDescriptorProto.decode(descriptor_data)
  parsed.clear_dependency
  serialized = parsed.class.encode(parsed)
  file = pool.add_serialized_file(serialized)
  warn "Warning: Protobuf detected an import path issue while loading generated file #{__FILE__}"
  imports = [
$imports$  ]
  imports.each do |type_name, expected_filename|
    import_file = pool.lookup(type_name).file_descriptor
    if import_file.name != expected_filename
      warn "- #{file.name} imports #{expected_filename}, but that import was loaded as #{import_file.name}"
    end
  end
  warn "Each proto file must use a consistent fully-qualified name."
  warn "This will become an error in the next major version."
end

)",
                 "descriptor_data",
                 absl::CHexEscape(SerializedDescriptor(file)), "imports",
                 DumpImportList(file));
}

bool GenerateFile(const FileDescriptor* file, io::Printer* printer,
                  std::string* error) {
  printer->Print(
      "# frozen_string_literal: true\n"
      "# Generated by the protocol buffer compiler.  DO NOT EDIT!\n"
      "# source: $filename$\n"
      "\n",
      "filename", file->name());

  printer->Print("require 'google/protobuf'\n\n");

  if (file->dependency_count() != 0) {
    for (int i = 0; i < file->dependency_count(); i++) {
      printer->Print("require '$name$'\n", "name", GetRequireName(file->dependency(i)->name()));
    }
    printer->Print("\n");
  }

  // TODO: Remove this when ruby supports extensions.
  if (file->extension_count() > 0) {
    ABSL_LOG(WARNING) << "Extensions are not yet supported in Ruby.";
  }

  GenerateBinaryDescriptor(file, printer, error);

  int levels = GeneratePackageModules(file, printer);
  for (int i = 0; i < file->message_type_count(); i++) {
    GenerateMessageAssignment("", file->message_type(i), printer);
  }
  for (int i = 0; i < file->enum_type_count(); i++) {
    GenerateEnumAssignment("", file->enum_type(i), printer);
  }
  EndPackageModules(levels, printer);

  return true;
}

bool Generator::Generate(
    const FileDescriptor* file,
    const std::string& parameter,
    GeneratorContext* generator_context,
    std::string* error) const {
  if (FileDescriptorLegacy(file).syntax() ==
      FileDescriptorLegacy::Syntax::SYNTAX_UNKNOWN) {
    *error = "Invalid or unsupported proto syntax";
    return false;
  }

  std::unique_ptr<io::ZeroCopyOutputStream> output(
      generator_context->Open(GetOutputFilename(file->name())));
  io::Printer printer(output.get(), '$');

  return GenerateFile(file, &printer, error);
}

}  // namespace ruby
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
