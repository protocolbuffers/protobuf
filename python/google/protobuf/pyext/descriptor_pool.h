// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef GOOGLE_PROTOBUF_PYTHON_CPP_DESCRIPTOR_POOL_H__
#define GOOGLE_PROTOBUF_PYTHON_CPP_DESCRIPTOR_POOL_H__

#define PY_SSIZE_T_CLEAN
#include <Python.h>

#include <memory>
#include <utility>

#include "absl/container/flat_hash_map.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/pyext/free_threading_mutex.h"
#include "google/protobuf/pyext/scoped_pyobject_ptr.h"

namespace google {
namespace protobuf {
namespace python {

struct PyMessageFactory;
struct cdescriptor_pool;

// Wraps operations to the global DescriptorPool which contains information
// about all messages and fields.
//
// There is normally one pool per process. We make it a Python object only
// because it contains many Python references.
class PyDescriptorPool {
 public:
  ~PyDescriptorPool();

  PyObject* AsPyObject() { return &ob_base; }

  const DescriptorPool* pool() const { return pool_.get(); }

  MessageFactory* message_factory() const;
  PyMessageFactory* py_message_factory() const { return py_message_factory_; }


  template <typename BuildFn>
  ScopedPyObjectPtr GetOrCacheOptions(const void* descriptor,
                                      BuildFn build_fn) {
    return GetOrCache(descriptor_options_, descriptor, build_fn);
  }

  template <typename BuildFn>
  ScopedPyObjectPtr GetOrCacheFeatures(const void* descriptor,
                                       BuildFn build_fn) {
    return GetOrCache(descriptor_features_, descriptor, build_fn);
  }

 private:
  friend cdescriptor_pool;

  class BuildFileErrorCollector;

  PyDescriptorPool();

  template <typename BuildFn>
  ScopedPyObjectPtr GetOrCache(
      absl::flat_hash_map<const void*, ScopedPyObjectPtr>& cache,
      const void* key, BuildFn build_fn) {
    {
      FreeThreadingLockGuard lock(cache_mutex_);
      if (auto it = cache.find(key); it != cache.end()) {
        return it->second;
      }
    }
    ScopedPyObjectPtr value = build_fn();
    if (value == nullptr) {
      return nullptr;
    }
    FreeThreadingLockGuard lock(cache_mutex_);
    auto [it, inserted] = cache.try_emplace(key, std::move(value));
    return it->second;
  }

  // clang-format off
  PyObject_HEAD

  // Note: Member declaration order is important for destruction order:
  // `error_collector_` and `database_` must outlive `pool_` (which holds
  // unowned pointers to them), and `pool_` must outlive `descriptor_options_`
  // and `descriptor_features_`.

  // The error collector to store error info. Can be NULL.
  std::unique_ptr<BuildFileErrorCollector> error_collector_;
  // clang-format on

  // The C++ pool acting as an underlay. Can be NULL.
  // This pointer is not owned and must stay alive.
  const DescriptorPool* underlay_ = nullptr;

  // The C++ descriptor database used to fetch unknown protos. Can be NULL.
  std::shared_ptr<const DescriptorDatabase> database_;

  // The C++ pool containing Descriptors.
  std::shared_ptr<const DescriptorPool> pool_;

  // True if this pool accepts new proto definitions.
  bool is_mutable_ = false;


  // The preferred MessageFactory to be used by descriptors.
  PyMessageFactory* py_message_factory_ = nullptr;

  // Cache the options for any kind of descriptor.
  absl::flat_hash_map<const void*, ScopedPyObjectPtr> descriptor_options_;
  // Similar cache for features.
  absl::flat_hash_map<const void*, ScopedPyObjectPtr> descriptor_features_;

  // Mutex protecting the caching maps above.
  FreeThreadingMutex cache_mutex_;
};

extern PyTypeObject PyDescriptorPool_Type;

// Retrieves the global descriptor pool owned by the _message module.
// This is the one used by pb2.py generated modules.
// Returns a *borrowed* reference.
// "Default" pool used to register messages from _pb2.py modules.
PyDescriptorPool* GetDefaultDescriptorPool();

// Retrieves an existing python descriptor pool owning the C++ descriptor pool.
// Returns a *borrowed* reference.
PyDescriptorPool* GetDescriptorPool_FromPool(const DescriptorPool* pool);

// Wraps a C++ descriptor pool in a Python object, creates it if necessary.
// Returns a new reference.
PyObject* PyDescriptorPool_FromPool(const DescriptorPool* pool);

// Wraps a C++ descriptor pool (held by shared_ptr) in a Python object.
// The Python object extends the lifetime of the C++ pool and optional database.
PyObject* PyDescriptorPool_FromSharedPool(
    std::shared_ptr<const DescriptorPool> pool,
    std::shared_ptr<const DescriptorDatabase> database = nullptr);

// Takes ownership of a C++ DescriptorPool and returns a new Python
// DescriptorPool that wraps it.
// If set, the DescriptorDatabase is also managed by the returned object.
PyObject* PyDescriptorPool_FromPool(
    std::unique_ptr<const google::protobuf::DescriptorPool> pool,
    std::unique_ptr<const google::protobuf::DescriptorDatabase> database);

// Returns the C++ descriptor pool wrapped by a Python object.
const DescriptorPool* PyDescriptorPool_AsPool(PyObject* pool);

// Initialize objects used by this module.
bool InitDescriptorPool();

}  // namespace python
}  // namespace protobuf
}  // namespace google

#endif  // GOOGLE_PROTOBUF_PYTHON_CPP_DESCRIPTOR_POOL_H__
