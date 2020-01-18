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

#ifndef GOOGLE_PROTOBUF_PYTHON_CPP_DESCRIPTOR_POOL_H__
#define GOOGLE_PROTOBUF_PYTHON_CPP_DESCRIPTOR_POOL_H__

#include <Python.h>

#include <unordered_map>
#include <string>
#include <vector>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/compiler/importer.h>
#include <google/protobuf/stubs/mutex.h>

namespace google {
namespace protobuf {
namespace python {

struct ParseError {
  std::string filename;
  int line;
  int column;
  std::string message;

  ParseError();
  ParseError(std::string filename, int line, int column, std::string message);

  std::string msg() const;
};

typedef ParseError ParseWarning;

class PyErrorCollector
    : public ::google::protobuf::compiler::MultiFileErrorCollector {
 public:
  void AddError(const std::string& filename, int line, int column,
                const std::string& message);

  void AddWarning(const std::string& filename, int line, int column,
                  const std::string& message);

  std::string Errors() const;
  std::string Warnings() const;
  size_t WarningCount() const;

  void Clear();

 private:
  std::vector<ParseError> errors_;
  std::vector<ParseWarning> warnings_;
  mutable Mutex mutex_;
};

// An implementation of DescriptorDatabase which returns FileDescriptorProtos
// already present in the process.
//
// NOTE: This class circumvents the inability to call DescriptorPool::BuildFile
// on a DescriptorPool with an associated DescriptorDatabase. As it recommends,
// this class enables "get(ting) your file into the DescriptorDatabase", while
// still allowing pre-serialized protos to be cross-linked with protos loaded
// from disk.
//
// This DescriptorDatabase is meant to be used with a DiskSourceTreeDatabase
// used as the fallback_database though, in theory, any DescriptorDatabase
// should work.
//
// All methods of this class are thread-safe besides the constructor.
class InProcessDescriptorDatabase
  : public ::google::protobuf::DescriptorDatabase {

public:
  InProcessDescriptorDatabase() = default;

  // If non-NULL, fallback_db will be checked if a FileDescriptorProto hasn't
  // already been registered in the DB.
  InProcessDescriptorDatabase(::google::protobuf::DescriptorDatabase* fallback_db);

  // Registers a FileDescriptorProto in the database. If the same entry is
  // present in the fallback_db, this will take precedence. Takes ownership of
  // the FileDescriptorProto.
  void Register(FileDescriptorProto&& proto);

  // The next three methods implement DescriptorDatabase.
  bool FindFileByName(const std::string& filename,
                      FileDescriptorProto* output) override;

  // Note: Always returns false to indicate that the operation is not supported.
  bool FindFileContainingSymbol(const std::string& symbol_name,
                                FileDescriptorProto* output) override;

  // Note: Always returns false to indicate that the operation is not supported.
  bool FindFileContainingExtension(const std::string& containing_type,
                                   int field_number,
                                   FileDescriptorProto* output) override;

private:
  std::unordered_map<std::string, FileDescriptorProto> fd_protos_;
  ::google::protobuf::DescriptorDatabase* fallback_db_;
  mutable ::google::protobuf::internal::Mutex mutex_;
};


struct PyMessageFactory;

// The (meta) type of all Messages classes.
struct CMessageClass;

// Wraps operations to the global DescriptorPool which contains information
// about all messages and fields.
//
// There is normally one pool per process. We make it a Python object only
// because it contains many Python references.
//
// "Methods" that interacts with this DescriptorPool are in the cdescriptor_pool
// namespace.
typedef struct PyDescriptorPool {
  PyObject_HEAD

  // The C++ pool containing Descriptors.
  DescriptorPool* pool;

  // The error collector to store error info. Can be NULL. This pointer is
  // owned.
  DescriptorPool::ErrorCollector* error_collector;

  // The C++ pool acting as an underlay. Can be NULL.
  // This pointer is not owned and must stay alive.
  const DescriptorPool* underlay;

  // The C++ descriptor database used to fetch unknown protos. Can be NULL.
  // This pointer is owned.
  const DescriptorDatabase* database;

  // The preferred MessageFactory to be used by descriptors.
  // TODO(amauryfa): Don't create the Factory from the DescriptorPool, but
  // use the one passed while creating message classes. And remove this member.
  PyMessageFactory* py_message_factory;

  // A DescriptorDatabase instance that allows both instantiation of descriptors
  // from in-process strings (as in generated code) and from files on disk (as
  // with runtime imports of ".proto" files). This database is always set on the
  // default descriptor pool. Can be NULL. This member is mutually
  // exclusive with database. If this attribute is set, file_error_collector,
  // source_tree, and disk_database must also be set. This poiner is owned.
  InProcessDescriptorDatabase* in_process_database;

  // The error collector to be used when parsing files. Can be NULL. Will always
  // be set on the default descriptor pool. This pointer is owned.
  PyErrorCollector* file_error_collector;

  // The disk source tree used to search for ".proto" files. Can be NULL. Will
  // always be set on the default descriptor pool. This pointer is owned.
  ::google::protobuf::compiler::DiskSourceTree* disk_source_tree;

  // The DiskSourceTree to be used to find protos on the filesystem. Can be
  // NULL. Will always be set on the default descriptor pool. This pointer is
  // owned.
  ::google::protobuf::compiler::SourceTreeDescriptorDatabase* disk_database;


  // Cache the options for any kind of descriptor.
  // Descriptor pointers are owned by the DescriptorPool above.
  // Python objects are owned by the map.
  std::unordered_map<const void*, PyObject*>* descriptor_options;
} PyDescriptorPool;


extern PyTypeObject PyDescriptorPool_Type;

namespace cdescriptor_pool {

// The functions below are also exposed as methods of the DescriptorPool type.

// Looks up a field by name. Returns a PyFieldDescriptor corresponding to
// the field on success, or NULL on failure.
//
// Returns a new reference.
PyObject* FindFieldByName(PyDescriptorPool* self, PyObject* name);

// Looks up an extension by name. Returns a PyFieldDescriptor corresponding
// to the field on success, or NULL on failure.
//
// Returns a new reference.
PyObject* FindExtensionByName(PyDescriptorPool* self, PyObject* arg);

// Looks up an enum type by name. Returns a PyEnumDescriptor corresponding
// to the field on success, or NULL on failure.
//
// Returns a new reference.
PyObject* FindEnumTypeByName(PyDescriptorPool* self, PyObject* arg);

// Looks up a oneof by name. Returns a COneofDescriptor corresponding
// to the oneof on success, or NULL on failure.
//
// Returns a new reference.
PyObject* FindOneofByName(PyDescriptorPool* self, PyObject* arg);

}  // namespace cdescriptor_pool

// Retrieve the global descriptor pool owned by the _message module.
// This is the one used by pb2.py generated modules.
// Returns a *borrowed* reference.
// "Default" pool used to register messages from _pb2.py modules.
PyDescriptorPool* GetDefaultDescriptorPool();

// Retrieve the python descriptor pool owning a C++ descriptor pool.
// Returns a *borrowed* reference.
PyDescriptorPool* GetDescriptorPool_FromPool(const DescriptorPool* pool);

// Initialize objects used by this module.
bool InitDescriptorPool();

}  // namespace python
}  // namespace protobuf
}  // namespace google

#endif  // GOOGLE_PROTOBUF_PYTHON_CPP_DESCRIPTOR_POOL_H__
