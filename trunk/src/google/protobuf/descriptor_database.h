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
// Interface for manipulating databases of descriptors.

#ifndef GOOGLE_PROTOBUF_DESCRIPTOR_DATABASE_H__
#define GOOGLE_PROTOBUF_DESCRIPTOR_DATABASE_H__

#include <map>
#include <string>
#include <utility>
#include <vector>
#include <google/protobuf/descriptor.h>

namespace google {
namespace protobuf {

// Abstract interface for a database of descriptors.
//
// This is useful if you want to create a DescriptorPool which loads
// descriptors on-demand from some sort of large database.  If the database
// is large, it may be inefficient to enumerate every .proto file inside it
// calling DescriptorPool::BuildFile() for each one.  Instead, a DescriptorPool
// can be created which wraps a DescriptorDatabase and only builds particular
// descriptors when they are needed.
class LIBPROTOBUF_EXPORT DescriptorDatabase {
 public:
  inline DescriptorDatabase() {}
  virtual ~DescriptorDatabase();

  // Find a file by file name.  Fills in in *output and returns true if found.
  // Otherwise, returns false, leaving the contents of *output undefined.
  virtual bool FindFileByName(const string& filename,
                              FileDescriptorProto* output) = 0;

  // Find the file that declares the given fully-qualified symbol name.
  // If found, fills in *output and returns true, otherwise returns false
  // and leaves *output undefined.
  virtual bool FindFileContainingSymbol(const string& symbol_name,
                                        FileDescriptorProto* output) = 0;

  // Find the file which defines an extension extending the given message type
  // with the given field number.  If found, fills in *output and returns true,
  // otherwise returns false and leaves *output undefined.  containing_type
  // must be a fully-qualified type name.
  virtual bool FindFileContainingExtension(const string& containing_type,
                                           int field_number,
                                           FileDescriptorProto* output) = 0;

 private:
  GOOGLE_DISALLOW_EVIL_CONSTRUCTORS(DescriptorDatabase);
};

// A DescriptorDatabase into which you can insert files manually.
//
// FindFileContainingSymbol() is fully-implemented.  When you add a file, its
// symbols will be indexed for this purpose.
//
// FindFileContainingExtension() is mostly-implemented.  It works if and only
// if the original FieldDescriptorProto defining the extension has a
// fully-qualified type name in its "extendee" field (i.e. starts with a '.').
// If the extendee is a relative name, SimpleDescriptorDatabase will not
// attempt to resolve the type, so it will not know what type the extension is
// extending.  Therefore, calling FindFileContainingExtension() with the
// extension's containing type will never actually find that extension.  Note
// that this is an unlikely problem, as all FileDescriptorProtos created by the
// protocol compiler (as well as ones created by calling
// FileDescriptor::CopyTo()) will always use fully-qualified names for all
// types.  You only need to worry if you are constructing FileDescriptorProtos
// yourself, or are calling compiler::Parser directly.
class LIBPROTOBUF_EXPORT SimpleDescriptorDatabase : public DescriptorDatabase {
 public:
  SimpleDescriptorDatabase();
  ~SimpleDescriptorDatabase();

  // Adds the FileDescriptorProto to the database, making a copy.  The object
  // can be deleted after Add() returns.
  void Add(const FileDescriptorProto& file);

  // Adds the FileDescriptorProto to the database and takes ownership of it.
  void AddAndOwn(const FileDescriptorProto* file);

  // implements DescriptorDatabase -----------------------------------
  bool FindFileByName(const string& filename,
                      FileDescriptorProto* output);
  bool FindFileContainingSymbol(const string& symbol_name,
                                FileDescriptorProto* output);
  bool FindFileContainingExtension(const string& containing_type,
                                   int field_number,
                                   FileDescriptorProto* output);

 private:
  // Helpers to recursively add particular descriptors and all their contents
  // to the by-symbol and by-extension tables.
  void AddMessage(const string& path,
                  const DescriptorProto& message_type,
                  const FileDescriptorProto* file);
  void AddField(const string& path,
                const FieldDescriptorProto& field,
                const FileDescriptorProto* file);
  void AddEnum(const string& path,
               const EnumDescriptorProto& enum_type,
               const FileDescriptorProto* file);
  void AddService(const string& path,
                  const ServiceDescriptorProto& service,
                  const FileDescriptorProto* file);

  vector<const FileDescriptorProto*> files_to_delete_;
  map<string, const FileDescriptorProto*> files_by_name_;
  map<string, const FileDescriptorProto*> files_by_symbol_;
  map<pair<string, int>, const FileDescriptorProto*> files_by_extension_;

  GOOGLE_DISALLOW_EVIL_CONSTRUCTORS(SimpleDescriptorDatabase);
};

// A DescriptorDatabase that fetches files from a given pool.
class LIBPROTOBUF_EXPORT DescriptorPoolDatabase : public DescriptorDatabase {
 public:
  DescriptorPoolDatabase(const DescriptorPool& pool);
  ~DescriptorPoolDatabase();

  // implements DescriptorDatabase -----------------------------------
  bool FindFileByName(const string& filename,
                      FileDescriptorProto* output);
  bool FindFileContainingSymbol(const string& symbol_name,
                                FileDescriptorProto* output);
  bool FindFileContainingExtension(const string& containing_type,
                                   int field_number,
                                   FileDescriptorProto* output);

 private:
  const DescriptorPool& pool_;
  GOOGLE_DISALLOW_EVIL_CONSTRUCTORS(DescriptorPoolDatabase);
};

// A DescriptorDatabase that wraps two or more others.  It first searches the
// first database and, if that fails, tries the second, and so on.
class LIBPROTOBUF_EXPORT MergedDescriptorDatabase : public DescriptorDatabase {
 public:
  // Merge just two databases.  The sources remain property of the caller.
  MergedDescriptorDatabase(DescriptorDatabase* source1,
                           DescriptorDatabase* source2);
  // Merge more than two databases.  The sources remain property of the caller.
  // The vector may be deleted after the constructor returns but the
  // DescriptorDatabases need to stick around.
  MergedDescriptorDatabase(const vector<DescriptorDatabase*>& sources);
  ~MergedDescriptorDatabase();

  // implements DescriptorDatabase -----------------------------------
  bool FindFileByName(const string& filename,
                      FileDescriptorProto* output);
  bool FindFileContainingSymbol(const string& symbol_name,
                                FileDescriptorProto* output);
  bool FindFileContainingExtension(const string& containing_type,
                                   int field_number,
                                   FileDescriptorProto* output);

 private:
  vector<DescriptorDatabase*> sources_;
  GOOGLE_DISALLOW_EVIL_CONSTRUCTORS(MergedDescriptorDatabase);
};

}  // namespace protobuf

}  // namespace google
#endif  // GOOGLE_PROTOBUF_DESCRIPTOR_DATABASE_H__
