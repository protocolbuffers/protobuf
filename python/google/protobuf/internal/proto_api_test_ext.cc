// An extension module to test proto_api.h.

#include <memory>
#include <stdexcept>
#include <string>
#include <utility>

#include "google/protobuf/descriptor.pb.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/descriptor_database.h"
#include "google/protobuf/dynamic_message.h"
#include "google/protobuf/message.h"
#include "google/protobuf/message_lite.h"
#include "google/protobuf/text_format.h"
#include "google/protobuf/unittest.pb.h"
#include "google/protobuf/proto_api.h"
#include "third_party/pybind11/include/pybind11/eval.h"
#include "third_party/pybind11/include/pybind11/pybind11.h"
#include "third_party/pybind11/include/pybind11/stl.h"

namespace google {
namespace protobuf {
namespace python {

namespace py = pybind11;
using ::google_protobuf_unittest::TestAllTypes;

const PyProto_API* GetProtoApi() {
  py::module_::import("google.protobuf.pyext._message");
  const PyProto_API* py_proto_api = static_cast<const PyProto_API*>(
      PyCapsule_Import(PyProtoAPICapsuleName(), 0));
  if (!py_proto_api) {
    throw py::error_already_set();
  }
  return py_proto_api;
}

// Test for GetConstMessagePointer
auto GetConstMessage(py::handle py_msg) {
  const PyProto_API* api = GetProtoApi();
  auto msg_ptr = api->GetConstMessagePointer(py_msg.ptr());
  if (!msg_ptr.ok()) {
    throw std::runtime_error(msg_ptr.status().ToString());
  }
  const auto* msg = DynamicCastMessage<TestAllTypes>(&msg_ptr->get());
  if (!msg) {
    throw std::runtime_error("Invalid message type");
  }
  return py::make_tuple(msg->optional_int32(), msg->optional_string());
}

// Test for GetClearedMessageMutator
auto SetMessageFieldWithMutator(py::handle py_msg, int value) {
  const PyProto_API* api = GetProtoApi();
  auto status_or_mutator = api->GetClearedMessageMutator(py_msg.ptr());
  if (!status_or_mutator.ok()) {
    throw std::runtime_error(status_or_mutator.status().ToString());
  }
  TestAllTypes* msg = DownCastMessage<TestAllTypes>(status_or_mutator->get());
  msg->set_optional_int32(value);
  // On destruction, the mutator will copy content back to python message.
}

// Test for DescriptorPool_FromPool and NewMessageOwnedExternally
auto ReprDynamicMessage(int value) {
  const PyProto_API* api = GetProtoApi();

  // Create a descriptor pool which copies everything from the linked protos.
  auto owned_pool = std::make_unique<DescriptorPool>(
      DescriptorPool::internal_generated_database());
  const Descriptor* descriptor =
      owned_pool->FindMessageTypeByName("proto2_unittest.TestAllTypes");
  if (!descriptor) {
    throw std::runtime_error("Failed to find file descriptor");
  }
  DynamicMessageFactory factory(owned_pool.get());
  const Message* prototype = factory.GetPrototype(descriptor);
  if (!prototype) {
    throw std::runtime_error("Failed to get prototype for descriptor");
  }
  std::unique_ptr<Message> msg(prototype->New());
  if (!msg) {
    throw std::runtime_error("Failed to create message");
  }
  msg->GetReflection()->SetInt32(
      msg.get(), descriptor->FindFieldByName("optional_int32"), value);

  // These calls to NewMessage fail because the descriptor pool is not
  // known to Python yet.
  {
    auto py_msg =
        py::reinterpret_steal<py::object>(api->NewMessage(descriptor, nullptr));
    if (py_msg) {
      throw std::runtime_error("NewMessage succeeded unexpectedly");
    }
    py_msg = py::reinterpret_steal<py::object>(
        api->NewMessageOwnedExternally(msg.get(), nullptr));
    if (py_msg) {
      throw std::runtime_error("NewMessage succeeded unexpectedly");
    }
  }

  // Create the Python DescriptorPool...
  auto py_pool = py::reinterpret_steal<py::object>(api->DescriptorPool_FromPool(
      std::move(owned_pool),
      std::unique_ptr<const google::protobuf::DescriptorDatabase>()));
  if (!py_pool) {
    throw py::error_already_set();
  }

  // ... And now the API Can use it to create the messages.
  std::string result_string;
  {
    auto py_msg =
        py::reinterpret_steal<py::object>(api->NewMessage(descriptor, nullptr));
    if (!py_msg) {
      throw py::error_already_set();
    }

    py_msg = py::reinterpret_steal<py::object>(
        api->NewMessageOwnedExternally(msg.get(), nullptr));
    if (!py_msg) {
      throw py::error_already_set();
    }
    result_string = py::repr(py_msg);
  }

  // The code above is dangerous! It relies on the C++ DescriptorPool being
  // alive for whole duration of the test.
  // At this point, there are no external references to the Python Message
  // classes, but they always form a reference cycle with their Python
  // MessageFactory.
  // So it is necessary to run the garbage collector.
  py::exec("import gc; gc.collect()");
  // Now the Python MessageFactory has been deleted, and it is safe to destroy
  // the C++ DescriptorPool.

  return result_string;
}

py::object CreateDynamicPoolMessage() {
  FileDescriptorProto file_descriptor;
  file_descriptor.set_name("test_file");
  file_descriptor.set_package("test_package");
  DescriptorProto* message_descriptor = file_descriptor.add_message_type();
  message_descriptor->set_name("MyMessage");
  FieldDescriptorProto* field_descriptor = message_descriptor->add_field();
  field_descriptor->set_name("my_field");
  field_descriptor->set_number(1);
  field_descriptor->set_label(FieldDescriptorProto::LABEL_OPTIONAL);
  field_descriptor->set_type(FieldDescriptorProto::TYPE_INT32);
  auto owned_pool = std::make_unique<DescriptorPool>();
  if (!owned_pool->BuildFile(file_descriptor)) {
    throw std::runtime_error("Failed to build file descriptor");
  }

  // Create a Python DescriptorPool from the C++ one.
  const PyProto_API* api = GetProtoApi();
  auto py_pool = py::reinterpret_steal<py::object>(api->DescriptorPool_FromPool(
      std::move(owned_pool),
      std::unique_ptr<const google::protobuf::DescriptorDatabase>()));
  if (!py_pool) {
    throw py::error_already_set();
  }

  const DescriptorPool* pool = api->DescriptorPool_AsPool(py_pool.ptr());
  if (!pool) {
    throw py::error_already_set();
  }

  // Navigate through the C++ Descriptors, and create a Python message.
  const Descriptor* descriptor =
      pool->FindMessageTypeByName("test_package.MyMessage");
  if (!descriptor) {
    throw std::runtime_error("Failed to find file descriptor");
  }
  auto py_msg =
      py::reinterpret_steal<py::object>(api->NewMessage(descriptor, nullptr));
  if (!py_msg) {
    throw py::error_already_set();
  }
  Message* msg = api->GetMutableMessagePointer(py_msg.ptr());
  if (!msg) {
    throw py::error_already_set();
  }

  // Populate the message, and return it.
  if (!google::protobuf::TextFormat::ParseFromString("my_field: 42", msg)) {
    throw std::runtime_error("Failed to parse message");
  }
  // This is safe: the Python object keeps a reference to the Python
  // DescriptorPool, which owns the C++ DescriptorPool.
  return py_msg;
}

py::object GetPoolFromMessage(py::handle py_msg) {
  const PyProto_API* api = GetProtoApi();
  auto msg_ptr = api->GetConstMessagePointer(py_msg.ptr());
  if (!msg_ptr.ok()) {
    if (PyErr_Occurred()) {
      throw py::error_already_set();
    }
    throw std::runtime_error(std::string(msg_ptr.status().message()));
  }
  const Message& msg = msg_ptr->get();
  const DescriptorPool* pool = msg.GetDescriptor()->file()->pool();
  auto shared_pool =
      std::shared_ptr<const DescriptorPool>(pool, [](const DescriptorPool*) {});
  PyObject* py_pool = api->DescriptorPool_FromSharedPool(
      shared_pool, std::shared_ptr<const google::protobuf::DescriptorDatabase>());
  if (!py_pool) {
    throw py::error_already_set();
  }
  return py::reinterpret_steal<py::object>(py_pool);
}

py::object GetCustomPoolWithDB() {
  const PyProto_API* api = GetProtoApi();
  // 1. Create Database
  auto custom_db = std::make_shared<SimpleDescriptorDatabase>();

  // File 1: Will be built in pool
  FileDescriptorProto pool_file;
  pool_file.set_name("custom_package/custom_message.proto");
  pool_file.set_package("custom_package");
  DescriptorProto* pool_msg = pool_file.add_message_type();
  pool_msg->set_name("CustomMessage");
  FieldDescriptorProto* pool_field = pool_msg->add_field();
  pool_field->set_name("custom_field");
  pool_field->set_number(1);
  pool_field->set_label(FieldDescriptorProto::LABEL_OPTIONAL);
  pool_field->set_type(FieldDescriptorProto::TYPE_INT32);
  if (!custom_db->Add(pool_file)) {
    throw std::runtime_error("Failed to add pool_file to database");
  }

  // File 2: Will NOT be built in pool (database only)
  FileDescriptorProto db_file;
  db_file.set_name("custom_package/db_message.proto");
  db_file.set_package("custom_package");
  DescriptorProto* db_msg = db_file.add_message_type();
  db_msg->set_name("DatabaseMessage");
  FieldDescriptorProto* db_field = db_msg->add_field();
  db_field->set_name("db_field");
  db_field->set_number(1);
  db_field->set_label(FieldDescriptorProto::LABEL_OPTIONAL);
  db_field->set_type(FieldDescriptorProto::TYPE_INT32);
  if (!custom_db->Add(db_file)) {
    throw std::runtime_error("Failed to add db_file to database");
  }

  // 2. Create Pool (linked to DB)
  auto custom_pool = std::make_shared<DescriptorPool>(custom_db.get());

  // 3. Build ONE file in the pool by looking it up
  if (custom_pool->FindMessageTypeByName("custom_package.CustomMessage") ==
      nullptr) {
    throw std::runtime_error("Failed to build CustomMessage in pool");
  }

  // 4. Return PyObject
  PyObject* py_pool =
      api->DescriptorPool_FromSharedPool(custom_pool, custom_db);
  if (!py_pool) {
    throw py::error_already_set();
  }
  return py::reinterpret_steal<py::object>(py_pool);
}

py::object GetCustomPool() {
  const PyProto_API* api = GetProtoApi();
  // 1. Create Pool
  auto custom_pool = std::make_shared<DescriptorPool>();

  // File 1: Call c++ pool BuildFile()
  FileDescriptorProto pool_file;
  pool_file.set_name("custom_package/custom_message.proto");
  pool_file.set_package("custom_package");
  DescriptorProto* pool_msg = pool_file.add_message_type();
  pool_msg->set_name("CustomMessage");
  FieldDescriptorProto* pool_field = pool_msg->add_field();
  pool_field->set_name("custom_field");
  pool_field->set_number(1);
  pool_field->set_label(FieldDescriptorProto::LABEL_OPTIONAL);
  pool_field->set_type(FieldDescriptorProto::TYPE_INT32);

  if (custom_pool->BuildFile(pool_file) == nullptr) {
    throw std::runtime_error("Failed to build FileDescriptor in pool");
  }

  // 2. Return PyObject
  PyObject* py_pool = api->DescriptorPool_FromSharedPool(
      custom_pool, std::shared_ptr<const google::protobuf::DescriptorDatabase>());
  if (!py_pool) {
    throw py::error_already_set();
  }
  return py::reinterpret_steal<py::object>(py_pool);
}

PYBIND11_MODULE(proto_api_test_ext, m) {
  m.def("get_const_message", &GetConstMessage);
  m.def("set_message_field_with_mutator", &SetMessageFieldWithMutator);
  m.def("repr_dynamic_message", &ReprDynamicMessage);
  m.def("create_dynamic_pool_message", &CreateDynamicPoolMessage);
  m.def("get_pool_from_message", &GetPoolFromMessage);
  m.def("get_custom_pool", &GetCustomPool);
  m.def("get_custom_pool_with_db", &GetCustomPoolWithDB);
}

}  // namespace python
}  // namespace protobuf
}  // namespace google
