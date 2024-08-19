#include <Python.h>

#include "google/protobuf/python/proto_api.h"
#include "absl/strings/str_join.h"
#include "absl/strings/str_split.h"
#include "google/protobuf/text_format.h"

// We have to run the pure python import first, otherwise PyCapsule_Import will
// fail if this module is the very first import.
bool PrerequisitesSatisfied() {
  std::vector<std::string> split_path =
      absl::StrSplit(google::protobuf::python::PyProtoAPICapsuleName(), '.');
  if (!split_path.empty()) split_path.pop_back();
  PyObject* imp = PyImport_ImportModule(absl::StrJoin(split_path, ".").c_str());
  if (imp == nullptr) return false;
  Py_DECREF(imp);
  return true;
}

const google::protobuf::python::PyProto_API* GetProtoApi() {
  const auto* py_proto_api = static_cast<const ::google::protobuf::python::PyProto_API*>(
      PyCapsule_Import(google::protobuf::python::PyProtoAPICapsuleName(), 0));
  return py_proto_api;
}

PyObject* pyclear_message(PyObject* self, PyObject* args) {
  PyObject* py_message;
  if (!PyArg_ParseTuple(args, "O", &py_message)) return nullptr;
  auto msg = GetProtoApi()->GetClearedMessageMutator(py_message);
  if (!msg.ok()) {
    PyErr_Format(PyExc_TypeError, msg.status().error_message().c_str());
    return nullptr;
  }
  Py_INCREF(Py_None);
  return Py_None;
}

PyObject* pyparse_message(PyObject* self, PyObject* args) {
  PyObject* py_message;
  char* text_proto;
  if (!PyArg_ParseTuple(args, "sO", &text_proto, &py_message)) return nullptr;
  auto message = GetProtoApi()->GetClearedMessageMutator(py_message);
  if (!message.ok()) {
    PyErr_Format(PyExc_TypeError, message.status().error_message().c_str());
    return nullptr;
  }
  if (message->get() == nullptr) {
    return nullptr;
  }
  // No op. Test '->' operator
  if ((*message)->ByteSizeLong()) {
    return nullptr;
  }

  google::protobuf::TextFormat::Parser parser;
  parser.ParseFromString(text_proto, message.value().get());
  Py_INCREF(Py_None);
  return Py_None;
}

PyMethodDef module_methods[] = {
    {"ClearMessage", reinterpret_cast<PyCFunction>(pyclear_message),
     METH_VARARGS, "Clear message."},
    {"ParseMessage", reinterpret_cast<PyCFunction>(pyparse_message),
     METH_VARARGS, "Parse message."},
    {nullptr, nullptr}};

extern "C" {
PyMODINIT_FUNC PyInit_proto_api_example(void) {
  if (!PrerequisitesSatisfied() || GetProtoApi() == nullptr) {
    return nullptr;
  }
  static struct PyModuleDef moduledef = {
      PyModuleDef_HEAD_INIT,
      "proto_api_example", /* m_name */
      "proto_api test",    /* m_doc */
      -1,                  /* m_size */
      module_methods,      /* m_methods */
      nullptr,             /* m_reload */
      nullptr,             /* m_traverse */
      nullptr,             /* m_clear */
      nullptr,             /* m_free */
  };
  return PyModule_Create(&moduledef);
}
}
