#include "google/protobuf/proto_api.h"

#include <Python.h>

#include <memory>
#include <string>

#include "absl/log/absl_check.h"
#include "google/protobuf/io/zero_copy_stream_impl_lite.h"
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

PythonConstMessagePointer::PythonConstMessagePointer(Message* owned_msg,
                                                     const Message* message,
                                                     PyObject* py_msg)
    : owned_msg_(owned_msg), message_(message), py_msg_(py_msg) {
  ABSL_DCHECK(py_msg != nullptr);
  ABSL_DCHECK(message != nullptr);
  Py_INCREF(py_msg_);
}

PythonConstMessagePointer::PythonConstMessagePointer(
    PythonConstMessagePointer&& other)
    : owned_msg_(other.owned_msg_ == nullptr ? nullptr
                                             : other.owned_msg_.release()),
      message_(other.message_),
      py_msg_(other.py_msg_) {
  other.message_ = nullptr;
  other.py_msg_ = nullptr;
}

bool PythonConstMessagePointer::NotChanged() {
  ABSL_DCHECK(!PyErr_Occurred());
  if (owned_msg_ == nullptr) {
    return false;
  }

  PyObject* py_serialized_pb(
      PyObject_CallMethod(py_msg_, "SerializeToString", nullptr));
  if (py_serialized_pb == nullptr) {
    PyErr_Format(PyExc_ValueError, "Fail to serialize py_msg");
    return false;
  }
  char* data;
  Py_ssize_t len;
  if (PyBytes_AsStringAndSize(py_serialized_pb, &data, &len) < 0) {
    Py_DECREF(py_serialized_pb);
    PyErr_Format(PyExc_ValueError, "Fail to get bytes from serialized data");
    return false;
  }

  // Even if serialize python message deterministic above, the
  // serialize result may still diff between languages. So parse to
  // another c++ message for compare.
  std::unique_ptr<google::protobuf::Message> parsed_msg(owned_msg_->New());
  parsed_msg->ParseFromArray(data, static_cast<int>(len));
  std::string wire_other;
  google::protobuf::io::StringOutputStream stream_other(&wire_other);
  google::protobuf::io::CodedOutputStream output_other(&stream_other);
  output_other.SetSerializationDeterministic(true);
  parsed_msg->SerializeToCodedStream(&output_other);

  std::string wire;
  google::protobuf::io::StringOutputStream stream(&wire);
  google::protobuf::io::CodedOutputStream output(&stream);
  output.SetSerializationDeterministic(true);
  owned_msg_->SerializeToCodedStream(&output);

  if (wire == wire_other) {
    Py_DECREF(py_serialized_pb);
    return true;
  }
  PyErr_Format(PyExc_ValueError, "pymessage has been changed");
  Py_DECREF(py_serialized_pb);
  return false;
}

PythonConstMessagePointer::~PythonConstMessagePointer() {
  if (py_msg_ == nullptr) {
    ABSL_DCHECK(message_ == nullptr);
    ABSL_DCHECK(owned_msg_ == nullptr);
    return;
  }
  ABSL_DCHECK(owned_msg_ != nullptr);
  ABSL_DCHECK(NotChanged());
  Py_DECREF(py_msg_);
}

PythonConstMessagePointer PyProto_API::CreatePythonConstMessagePointer(
    Message* owned_msg, const Message* msg, PyObject* py_msg) const {
  return PythonConstMessagePointer(owned_msg, msg, py_msg);
}

}  // namespace python
}  // namespace protobuf
}  // namespace google
