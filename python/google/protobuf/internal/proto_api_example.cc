#include <Python.h>

#include "absl/strings/str_join.h"
#include "absl/strings/str_split.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/text_format.h"
#include "google/protobuf/unittest_proto3.pb.h"
#include "google/protobuf/proto_api.h"

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

PyObject* pycheck_cpp_proto(PyObject* self, PyObject* args) {
  PyObject* py_message;
  if (!PyArg_ParseTuple(args, "O", &py_message)) return nullptr;
  auto msg = GetProtoApi()->GetConstMessagePointer(py_message);
  if (!msg.ok()) {
    PyErr_Format(PyExc_TypeError, msg.status().error_message().c_str());
    return nullptr;
  }
  if (msg->get().GetDescriptor()->file()->pool() ==
      google::protobuf::DescriptorPool::generated_pool()) {
    Py_RETURN_TRUE;
  }
  Py_RETURN_FALSE;
}

PyObject* pymessage_get(PyObject* self, PyObject* args) {
  PyObject* py_message;
  if (!PyArg_ParseTuple(args, "O", &py_message)) return nullptr;
  auto message = GetProtoApi()->GetConstMessagePointer(py_message);
  if (!message.ok()) {
    PyErr_Format(PyExc_TypeError, message.status().error_message().c_str());
    return nullptr;
  }
  // Test move constructor.
  google::protobuf::python::PythonConstMessagePointer moved_msg(std::move(*message));
  const proto3_unittest::TestAllTypes& msg_ptr =
      static_cast<const proto3_unittest::TestAllTypes&>(moved_msg.get());
  return PyLong_FromLong(msg_ptr.optional_int32());
}

PyObject* pymessage_mutate_const(PyObject* self, PyObject* args) {
  PyObject* py_message;
  if (!PyArg_ParseTuple(args, "O", &py_message)) return nullptr;
  auto message = GetProtoApi()->GetConstMessagePointer(py_message);
  PyObject_SetAttrString(py_message, "optional_bool", Py_True);
  // The message has been changed. Returns false if detect not changed.
  if (message->NotChanged()) {
    Py_RETURN_FALSE;
  }
  // Change it back before returning.
  PyErr_Clear();
  PyObject* temp = PyObject_CallMethod(py_message, "Clear", nullptr);
  Py_DECREF(temp);
  Py_RETURN_TRUE;
}

PyMethodDef module_methods[] = {
    {"ClearMessage", reinterpret_cast<PyCFunction>(pyclear_message),
     METH_VARARGS, "Clear message."},
    {"ParseMessage", reinterpret_cast<PyCFunction>(pyparse_message),
     METH_VARARGS, "Parse message."},
    {"IsCppProtoLinked", reinterpret_cast<PyCFunction>(pycheck_cpp_proto),
     METH_VARARGS, "Check if the generated cpp proto is linked."},
    {"GetOptionalInt32", reinterpret_cast<PyCFunction>(pymessage_get),
     METH_VARARGS, "Get optional_int32 field."},
    {"MutateConstAlive", reinterpret_cast<PyCFunction>(pymessage_mutate_const),
     METH_VARARGS, "Mutate python message while keep a const pointer."},
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
