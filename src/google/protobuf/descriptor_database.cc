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

#include <google/protobuf/descriptor_database.h>
#include <google/protobuf/descriptor.pb.h>
#include <google/protobuf/stubs/stl_util-inl.h>
#include <google/protobuf/stubs/map-util.h>

namespace google {
namespace protobuf {

DescriptorDatabase::~DescriptorDatabase() {}

// ===================================================================

SimpleDescriptorDatabase::SimpleDescriptorDatabase() {}
SimpleDescriptorDatabase::~SimpleDescriptorDatabase() {
  STLDeleteElements(&files_to_delete_);
}

void SimpleDescriptorDatabase::Add(const FileDescriptorProto& file) {
  FileDescriptorProto* new_file = new FileDescriptorProto;
  new_file->CopyFrom(file);
  AddAndOwn(new_file);
}

void SimpleDescriptorDatabase::AddAndOwn(const FileDescriptorProto* file) {
  files_to_delete_.push_back(file);
  InsertOrUpdate(&files_by_name_, file->name(), file);

  string path = file->package();
  if (!path.empty()) path += '.';

  for (int i = 0; i < file->message_type_size(); i++) {
    AddMessage(path, file->message_type(i), file);
  }
  for (int i = 0; i < file->enum_type_size(); i++) {
    AddEnum(path, file->enum_type(i), file);
  }
  for (int i = 0; i < file->extension_size(); i++) {
    AddField(path, file->extension(i), file);
  }
  for (int i = 0; i < file->service_size(); i++) {
    AddService(path, file->service(i), file);
  }
}

void SimpleDescriptorDatabase::AddMessage(
    const string& path,
    const DescriptorProto& message_type,
    const FileDescriptorProto* file) {
  string full_name = path + message_type.name();
  InsertOrUpdate(&files_by_symbol_, full_name, file);

  string sub_path = full_name + '.';
  for (int i = 0; i < message_type.nested_type_size(); i++) {
    AddMessage(sub_path, message_type.nested_type(i), file);
  }
  for (int i = 0; i < message_type.enum_type_size(); i++) {
    AddEnum(sub_path, message_type.enum_type(i), file);
  }
  for (int i = 0; i < message_type.field_size(); i++) {
    AddField(sub_path, message_type.field(i), file);
  }
  for (int i = 0; i < message_type.extension_size(); i++) {
    AddField(sub_path, message_type.extension(i), file);
  }
}

void SimpleDescriptorDatabase::AddField(
    const string& path,
    const FieldDescriptorProto& field,
    const FileDescriptorProto* file) {
  string full_name = path + field.name();
  InsertOrUpdate(&files_by_symbol_, full_name, file);

  if (field.has_extendee()) {
    // This field is an extension.
    if (!field.extendee().empty() && field.extendee()[0] == '.') {
      // The extension is fully-qualified.  We can use it as a lookup key in
      // the files_by_symbol_ table.
      InsertOrUpdate(&files_by_extension_,
                     make_pair(field.extendee().substr(1), field.number()),
                     file);
    } else {
      // Not fully-qualified.  We can't really do anything here, unfortunately.
    }
  }
}

void SimpleDescriptorDatabase::AddEnum(
    const string& path,
    const EnumDescriptorProto& enum_type,
    const FileDescriptorProto* file) {
  string full_name = path + enum_type.name();
  InsertOrUpdate(&files_by_symbol_, full_name, file);

  string sub_path = full_name + '.';
  for (int i = 0; i < enum_type.value_size(); i++) {
    InsertOrUpdate(&files_by_symbol_,
                   sub_path + enum_type.value(i).name(),
                   file);
  }
}

void SimpleDescriptorDatabase::AddService(
    const string& path,
    const ServiceDescriptorProto& service,
    const FileDescriptorProto* file) {
  string full_name = path + service.name();
  InsertOrUpdate(&files_by_symbol_, full_name, file);

  string sub_path = full_name + '.';
  for (int i = 0; i < service.method_size(); i++) {
    InsertOrUpdate(&files_by_symbol_,
                   sub_path + service.method(i).name(),
                   file);
  }
}

bool SimpleDescriptorDatabase::FindFileByName(
    const string& filename,
    FileDescriptorProto* output) {
  const FileDescriptorProto* result = FindPtrOrNull(files_by_name_, filename);
  if (result == NULL) {
    return false;
  } else {
    output->CopyFrom(*result);
    return true;
  }
}

bool SimpleDescriptorDatabase::FindFileContainingSymbol(
    const string& symbol_name,
    FileDescriptorProto* output) {
  const FileDescriptorProto* result =
    FindPtrOrNull(files_by_symbol_, symbol_name);
  if (result == NULL) {
    return false;
  } else {
    output->CopyFrom(*result);
    return true;
  }
}

bool SimpleDescriptorDatabase::FindFileContainingExtension(
    const string& containing_type,
    int field_number,
    FileDescriptorProto* output) {
  const FileDescriptorProto* result =
    FindPtrOrNull(files_by_extension_,
                  make_pair(containing_type, field_number));
  if (result == NULL) {
    return false;
  } else {
    output->CopyFrom(*result);
    return true;
  }
}

// ===================================================================

DescriptorPoolDatabase::DescriptorPoolDatabase(const DescriptorPool& pool)
  : pool_(pool) {}
DescriptorPoolDatabase::~DescriptorPoolDatabase() {}

bool DescriptorPoolDatabase::FindFileByName(
    const string& filename,
    FileDescriptorProto* output) {
  const FileDescriptor* file = pool_.FindFileByName(filename);
  if (file == NULL) return false;
  output->Clear();
  file->CopyTo(output);
  return true;
}

bool DescriptorPoolDatabase::FindFileContainingSymbol(
    const string& symbol_name,
    FileDescriptorProto* output) {
  const FileDescriptor* file = pool_.FindFileContainingSymbol(symbol_name);
  if (file == NULL) return false;
  output->Clear();
  file->CopyTo(output);
  return true;
}

bool DescriptorPoolDatabase::FindFileContainingExtension(
    const string& containing_type,
    int field_number,
    FileDescriptorProto* output) {
  const Descriptor* extendee = pool_.FindMessageTypeByName(containing_type);
  if (extendee == NULL) return false;

  const FieldDescriptor* extension =
    pool_.FindExtensionByNumber(extendee, field_number);
  if (extension == NULL) return false;

  output->Clear();
  extension->file()->CopyTo(output);
  return true;
}

// ===================================================================

MergedDescriptorDatabase::MergedDescriptorDatabase(
    DescriptorDatabase* source1,
    DescriptorDatabase* source2) {
  sources_.push_back(source1);
  sources_.push_back(source2);
}
MergedDescriptorDatabase::MergedDescriptorDatabase(
    const vector<DescriptorDatabase*>& sources)
  : sources_(sources) {}
MergedDescriptorDatabase::~MergedDescriptorDatabase() {}

bool MergedDescriptorDatabase::FindFileByName(
    const string& filename,
    FileDescriptorProto* output) {
  for (int i = 0; i < sources_.size(); i++) {
    if (sources_[i]->FindFileByName(filename, output)) {
      return true;
    }
  }
  return false;
}

bool MergedDescriptorDatabase::FindFileContainingSymbol(
    const string& symbol_name,
    FileDescriptorProto* output) {
  for (int i = 0; i < sources_.size(); i++) {
    if (sources_[i]->FindFileContainingSymbol(symbol_name, output)) {
      // The symbol was found in source i.  However, if one of the previous
      // sources defines a file with the same name (which presumably doesn't
      // contain the symbol, since it wasn't found in that source), then we
      // must hide it from the caller.
      FileDescriptorProto temp;
      for (int j = 0; j < i; j++) {
        if (sources_[j]->FindFileByName(output->name(), &temp)) {
          // Found conflicting file in a previous source.
          return false;
        }
      }
      return true;
    }
  }
  return false;
}

bool MergedDescriptorDatabase::FindFileContainingExtension(
    const string& containing_type,
    int field_number,
    FileDescriptorProto* output) {
  for (int i = 0; i < sources_.size(); i++) {
    if (sources_[i]->FindFileContainingExtension(
          containing_type, field_number, output)) {
      // The symbol was found in source i.  However, if one of the previous
      // sources defines a file with the same name (which presumably doesn't
      // contain the symbol, since it wasn't found in that source), then we
      // must hide it from the caller.
      FileDescriptorProto temp;
      for (int j = 0; j < i; j++) {
        if (sources_[j]->FindFileByName(output->name(), &temp)) {
          // Found conflicting file in a previous source.
          return false;
        }
      }
      return true;
    }
  }
  return false;
}

}  // namespace protobuf
}  // namespace google
