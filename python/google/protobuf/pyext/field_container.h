// Protocol Buffers - Google's data interchange format
// Copyright 2026 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef GOOGLE_PROTOBUF_PYEXT_FIELD_CONTAINER_H__
#define GOOGLE_PROTOBUF_PYEXT_FIELD_CONTAINER_H__

#define PY_SSIZE_T_CLEAN
#include <Python.h>

#include "absl/log/absl_check.h"
#include "google/protobuf/pyext/container_base.h"
#include "google/protobuf/pyext/free_threading/access_guard.h"

namespace google {
namespace protobuf {
namespace python {

struct CMessage;

// Base class of the containers that are views of a field of their parent
// message: MapContainer, RepeatedCompositeContainer and
// RepeatedScalarContainer.
struct FieldContainer : public TreeLockable<FieldContainer> {
  // A container is a view of its parent message (see TreeLockable). Requires
  // access to this container's tree (the parent is on the same tree).
  const CMessage* Owner(const ReadAccess& access) const {
    // Containers always have a real parent message while reachable (only
    // CMessages can have a null or Py_None parent).
    const CMessage* parent_message = parent.Get(access);
    ABSL_DCHECK(parent_message != nullptr &&
                reinterpret_cast<const PyObject*>(parent_message) != Py_None);
    return parent_message;
  }
  CMessage* Owner(const WriteAccess& access) {
    // Containers always have a real parent message while reachable (only
    // CMessages can have a null or Py_None parent).
    CMessage* parent_message = parent.Get(access);
    ABSL_DCHECK(parent_message != nullptr &&
                reinterpret_cast<PyObject*>(parent_message) != Py_None);
    return parent_message;
  }

  // Acquires a read lock on this container's tree and verifies that the parent
  // message is not frozen. Returns true if writable, false (with a Python
  // exception set) if frozen or on concurrent modification.
  bool CheckParentNotFrozen(
      const char* error_msg = "Container is immutable") const;

  // Makes a new parent message, copies the field into it, and returns the
  // new parent's container for the field.
  PyObject* DeepCopy();
  // Delete this container object from its parent.
  void RemoveFromParentCache();
};

}  // namespace python
}  // namespace protobuf
}  // namespace google

#endif  // GOOGLE_PROTOBUF_PYEXT_FIELD_CONTAINER_H__
