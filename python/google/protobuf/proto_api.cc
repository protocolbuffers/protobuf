#include "google/protobuf/proto_api.h"

#include <string>

#include "absl/log/absl_check.h"
#include "google/protobuf/message.h"
namespace google {
namespace protobuf {
namespace python {

PythonMessageMutator::PythonMessageMutator(Message* owned_msg, Message* message,
                                           PyObject* py_msg)
    : owned_msg_(owned_msg), message_(message), py_msg_(py_msg) {
  ABSL_DCHECK(py_msg != nullptr);
  ABSL_DCHECK(message != nullptr);
  Py_INCREF(py_msg_);
}

PythonMessageMutator::PythonMessageMutator(PythonMessageMutator&& other)
    : owned_msg_(other.owned_msg_ == nullptr ? nullptr
                                             : other.owned_msg_.release()),
      message_(other.message_),
      py_msg_(other.py_msg_) {
  other.message_ = nullptr;
  other.py_msg_ = nullptr;
}

PythonMessageMutator::~PythonMessageMutator() {
  if (py_msg_ == nullptr) {
    return;
  }

  // PyErr_Occurred check is required because PyObject_CallMethod need this
  // check.
  if (!PyErr_Occurred() && owned_msg_ != nullptr) {
    std::string wire;
    message_->SerializeToString(&wire);
    PyObject* py_wire = PyBytes_FromStringAndSize(
        wire.data(), static_cast<Py_ssize_t>(wire.size()));
    PyObject* parse =
        PyObject_CallMethod(py_msg_, "ParseFromString", "O", py_wire);
    Py_DECREF(py_wire);
    if (parse != nullptr) {
      Py_DECREF(parse);
    }
  }
  Py_DECREF(py_msg_);
}

PythonMessageMutator PyProto_API::CreatePythonMessageMutator(
    Message* owned_msg, Message* msg, PyObject* py_msg) const {
  return PythonMessageMutator(owned_msg, msg, py_msg);
}

}  // namespace python
}  // namespace protobuf
}  // namespace google
