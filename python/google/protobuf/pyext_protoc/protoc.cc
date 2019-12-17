// Protocol Buffers - Google's data interchange format
// Copyright 2019 Google Inc.  All rights reserved.
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

#include <google/protobuf/pyext_protoc/protoc.h>
#include <google/protobuf/compiler/python/python_generator.h>
#include <google/protobuf/compiler/code_generator.h>
#include <google/protobuf/compiler/importer.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/io/zero_copy_stream_impl_lite.h>

#include <algorithm>
#include <map>
#include <string>
#include <tuple>
#include <unordered_set>
#include <vector>

namespace google {
namespace protobuf {
namespace python {
namespace protoc {

// TODO: Move to internal namespace.
struct ProtocError {
  std::string filename;
  int line;
  int column;
  std::string message;

  ProtocError() {}
  ProtocError(std::string filename, int line, int column, std::string message)
      : filename(filename), line(line), column(column), message(message) {}
};

typedef ProtocError ProtocWarning;

namespace internal {

// TODO: Pull this out to a common source that can also be used by gRPC?
class GeneratorContextImpl
    : public ::google::protobuf::compiler::GeneratorContext {
 public:
  GeneratorContextImpl(
      const std::vector<const ::google::protobuf::FileDescriptor*>&
          parsed_files,
      std::vector<std::pair<std::string, std::string>>* files_out)
      : files_(files_out), parsed_files_(parsed_files) {}

  ::google::protobuf::io::ZeroCopyOutputStream* Open(
      const std::string& filename) {
    files_->emplace_back(filename, "");
    return new ::google::protobuf::io::StringOutputStream(
        &(files_->back().second));
  }

  // NOTE(rbellevi): Equivalent to Open, since all files start out empty.
  ::google::protobuf::io::ZeroCopyOutputStream* OpenForAppend(
      const std::string& filename) {
    return Open(filename);
  }

  // NOTE(rbellevi): Equivalent to Open, since all files start out empty.
  ::google::protobuf::io::ZeroCopyOutputStream* OpenForInsert(
      const std::string& filename, const std::string& insertion_point) {
    return Open(filename);
  }

  void ListParsedFiles(
      std::vector<const ::google::protobuf::FileDescriptor*>* output) {
    *output = parsed_files_;
  }

 private:
  std::vector<std::pair<std::string, std::string>>* files_;
  const std::vector<const ::google::protobuf::FileDescriptor*>& parsed_files_;
};

class ErrorCollectorImpl
    : public ::google::protobuf::compiler::MultiFileErrorCollector {
 public:
  ErrorCollectorImpl(std::vector<::google::protobuf::python::protoc::ProtocError>* errors,
                     std::vector<::google::protobuf::python::protoc::ProtocWarning>* warnings)
      : errors_(errors), warnings_(warnings) {}

  void AddError(const std::string& filename, int line, int column,
                const std::string& message) {
    errors_->emplace_back(filename, line, column, message);
  }

  void AddWarning(const std::string& filename, int line, int column,
                  const std::string& message) {
    warnings_->emplace_back(filename, line, column, message);
  }

 private:
  std::vector<::google::protobuf::python::protoc::ProtocError>* errors_;
  std::vector<::google::protobuf::python::protoc::ProtocWarning>* warnings_;
};

static void calculate_transitive_closure(
    const ::google::protobuf::FileDescriptor* descriptor,
    std::vector<const ::google::protobuf::FileDescriptor*>* transitive_closure,
    std::unordered_set<const ::google::protobuf::FileDescriptor*>* visited) {
  for (int i = 0; i < descriptor->dependency_count(); ++i) {
    const ::google::protobuf::FileDescriptor* dependency =
        descriptor->dependency(i);
    if (visited->find(dependency) == visited->end()) {
      calculate_transitive_closure(dependency, transitive_closure, visited);
    }
  }
  transitive_closure->push_back(descriptor);
  visited->insert(descriptor);
}

} // namespace internal

static int generate_code(
    const ::google::protobuf::compiler::CodeGenerator* code_generator,
    const char* protobuf_path, const std::vector<std::string>* include_paths,
    std::vector<std::pair<std::string, std::string>>* files_out,
    std::vector<::google::protobuf::python::protoc::ProtocError>* errors,
    std::vector<::google::protobuf::python::protoc::ProtocWarning>* warnings) {
  std::unique_ptr<internal::ErrorCollectorImpl> error_collector(
      new internal::ErrorCollectorImpl(errors, warnings));
  std::unique_ptr<::google::protobuf::compiler::DiskSourceTree> source_tree(
      new ::google::protobuf::compiler::DiskSourceTree());
  for (const auto& include_path : *include_paths) {
    source_tree->MapPath("", include_path);
  }
  ::google::protobuf::compiler::Importer importer(source_tree.get(),
                                                  error_collector.get());
  const ::google::protobuf::FileDescriptor* parsed_file =
      importer.Import(protobuf_path);
  if (parsed_file == nullptr) {
    return 1;
  }
  std::vector<const ::google::protobuf::FileDescriptor*> transitive_closure;
  std::unordered_set<const ::google::protobuf::FileDescriptor*> visited;
  internal::calculate_transitive_closure(parsed_file, &transitive_closure,
                                         &visited);
  internal::GeneratorContextImpl generator_context(transitive_closure,
                                                   files_out);
  std::string error;
  for (const auto descriptor : transitive_closure) {
    code_generator->Generate(descriptor, "", &generator_context, &error);
  }
  return 0;
}

// TODO: Remove.
static int protoc_get_protos(
    const char* protobuf_path, const std::vector<std::string>* include_paths,
    std::vector<std::pair<std::string, std::string>>* files_out,
    std::vector<::google::protobuf::python::protoc::ProtocError>* errors,
    std::vector<::google::protobuf::python::protoc::ProtocWarning>* warnings) {
  ::google::protobuf::compiler::python::Generator python_generator;
  return generate_code(&python_generator, protobuf_path, include_paths,
                       files_out, errors, warnings);
}

static bool parse_args(PyObject* args,
                      const char** protobuf_path,
                      std::vector<std::string>* include_paths)
{
  if (PyTuple_Size(args) != 2) {
    PyErr_SetString(PyExc_ValueError, "Expected 2 arguments.");
    return false;
  }
  PyObject* py_protobuf_path = PyTuple_GetItem(args, 0);
  PyObject* py_include_paths = PyTuple_GetItem(args, 1);
  include_paths->reserve(PySequence_Size(py_include_paths));
  for (Py_ssize_t i = 0; i < PySequence_Size(py_include_paths); ++i) {
    PyObject* py_include_path = PySequence_ITEM(py_include_paths, i);
    if (!py_include_path) {
      return false;
    }
    const char* include_path = PyBytes_AsString(py_include_path);
    Py_DECREF(py_include_path);
    if (include_path == NULL) {
      return false;
    }
    include_paths->push_back(include_path);
  }
  *protobuf_path = PyBytes_AsString(py_protobuf_path);
  if (*protobuf_path == NULL) {
    return false;
  }
  return true;
}

static bool from_generator_parse_args(PyObject* args,
                                      const char** protobuf_path,
                                      std::vector<std::string>* include_paths,
                                      ::google::protobuf::compiler::CodeGenerator const * * code_generator)
{
  if (PyTuple_Size(args) != 3) {
    PyErr_SetString(PyExc_ValueError, "Expected 3 arguments.");
    return false;
  }
  PyObject* py_code_generator = PyTuple_GetItem(args, 0);
  PyObject* py_protobuf_path = PyTuple_GetItem(args, 1);
  PyObject* py_include_paths = PyTuple_GetItem(args, 2);
  *code_generator = reinterpret_cast<const ::google::protobuf::compiler::CodeGenerator*>(PyLong_AsLong(py_code_generator));
  if (*code_generator == NULL) {
    return false;
  }
  include_paths->reserve(PySequence_Size(py_include_paths));
  for (Py_ssize_t i = 0; i < PySequence_Size(py_include_paths); ++i) {
    PyObject* py_include_path = PySequence_ITEM(py_include_paths, i);
    if (!py_include_path) {
      return false;
    }
    const char* include_path = PyBytes_AsString(py_include_path);
    Py_DECREF(py_include_path);
    if (include_path == NULL) {
      return false;
    }
    include_paths->push_back(include_path);
  }
  *protobuf_path = PyBytes_AsString(py_protobuf_path);
  if (*protobuf_path == NULL) {
    return false;
  }
  return true;
}

// NOTE: Returns new reference to List[Tuple[bytes, bytes]].
static PyObject* pack_results(const std::vector<std::pair<std::string, std::string>>& files_out) {
  PyObject* py_files_out = PyList_New(files_out.size());
  if (py_files_out == NULL) {
    PyErr_SetString(PyExc_OSError, "Failed to allocate list object.");
    return NULL;
  }
  Py_ssize_t i = 0;
  for (const auto& file_pair : files_out) {
    PyObject* py_path = PyBytes_FromString(file_pair.first.c_str());
    PyObject* py_code = PyBytes_FromString(file_pair.second.c_str());
    if (py_path == NULL || py_code == NULL) {
      PyErr_SetString(PyExc_OSError, "Failed to allocate bytes object.");
      return NULL;
    }
    PyObject* py_file_pair = PyTuple_Pack(2, py_path, py_code);
    if (py_file_pair == NULL) {
      PyErr_SetString(PyExc_OSError, "Failed to allocate tuple object.");
      return NULL;
    }
    PyList_SET_ITEM(py_files_out, i, py_file_pair);
    ++i;
  }
  return py_files_out;
}

static void process_errors(const std::vector<::google::protobuf::python::protoc::ProtocError>& errors,
                           const std::vector<::google::protobuf::python::protoc::ProtocWarning>& warnings) {

  // TODO: Fully report errors.
  PyErr_SetString(PyExc_RuntimeError, errors[0].message.c_str());
}

// TODO: Implement in Python.
// NOTE: Returns new reference to List[Tuple[bytes, bytes]].
static PyObject* get_protos_as_list(PyObject* unused_module, PyObject* args) {
  const char* protobuf_path;
  std::vector<std::string> include_paths;
  if (!parse_args(args, &protobuf_path, &include_paths)) {
    return NULL;
  }
  std::vector<std::pair<std::string, std::string>> files_out;
  std::vector<::google::protobuf::python::protoc::ProtocError> errors;
  std::vector<::google::protobuf::python::protoc::ProtocWarning> warnings;
  int rc;
  Py_BEGIN_ALLOW_THREADS;
  rc = protoc_get_protos(protobuf_path, &include_paths, &files_out, &errors, &warnings);
  Py_END_ALLOW_THREADS;
  if (rc != 0) {
    process_errors(errors, warnings);
    return NULL;
  }
  return pack_results(files_out);
}

// NOTE: Returns new reference to List[Tuple[bytes, bytes]].
static PyObject* get_protos_from_generator(PyObject* unused_module, PyObject* args) {
  const char* protobuf_path;
  std::vector<std::string> include_paths;
  const ::google::protobuf::compiler::CodeGenerator* code_generator;
  if (!from_generator_parse_args(args, &protobuf_path, &include_paths, &code_generator)) {
    return NULL;
  }
  // TODO: Implement.
  std::vector<std::pair<std::string, std::string>> files_out;
  std::vector<::google::protobuf::python::protoc::ProtocError> errors;
  std::vector<::google::protobuf::python::protoc::ProtocWarning> warnings;
  int rc;
  Py_BEGIN_ALLOW_THREADS;
  rc = generate_code(code_generator, protobuf_path, &include_paths, &files_out, &errors, &warnings);
  Py_END_ALLOW_THREADS;
  if (rc != 0) {
    process_errors(errors, warnings);
    return NULL;
  }
  return pack_results(files_out);
}
} // namespace protoc
} // namespace python
} // namespace protobuf
} // namespace google

// TODO: Decref in error cases.
// TODO: Clean up the #if's
#if PY_MAJOR_VERSION >= 3
static const char module_docstring[] =
  "The _protoc module enables generation of code from protocol buffer "
  "definitions at runtime.";

static PyMethodDef module_methods[] = {
  {
    "get_protos_as_list",
    (PyCFunction)::google::protobuf::python::protoc::get_protos_as_list,
    METH_VARARGS,
    // TODO: Think harder about this docstring.
    "Compiles .proto files and returns the generated code."
  },
  {
    "get_protos_from_generator",
    (PyCFunction)::google::protobuf::python::protoc::get_protos_from_generator,
    METH_VARARGS,
    "Compiles .proto files and returns the generated code.",
  },
  {NULL, NULL}
};

static const ::google::protobuf::compiler::python::Generator g_code_generator;
#endif

#if PY_MAJOR_VERSION >= 3
#define PROTOC_INITFUNC PyInit__protoc

static struct PyModuleDef _protoc_module = {
  PyModuleDef_HEAD_INIT,
  "_protoc",
  module_docstring,
  -1,
  module_methods,
  NULL,
  NULL,
  NULL,
  NULL
};
#else  // Python 2
#define PROTOC_INITFUNC init_protoc
#endif

PyMODINIT_FUNC PROTOC_INITFUNC() {
#if PY_MAJOR_VERSION >= 3
  PyObject* module =  PyModule_Create(&_protoc_module);
  PyObject* py_code_generator = PyLong_FromLong(reinterpret_cast<long>(&g_code_generator));
  if (py_code_generator == NULL) {
    return NULL;
  }
  if (PyModule_AddObject(module, "code_generator", py_code_generator)) {
    return NULL;
  }
  return module;
#else  // Python 2
  // NOTE: Dynamic stub generation is not supported under Python 2,
  //  so we don't implement the module at all.
  return;
#endif
}
