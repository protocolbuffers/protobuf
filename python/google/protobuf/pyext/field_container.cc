// Protocol Buffers - Google's data interchange format
// Copyright 2026 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "google/protobuf/pyext/field_container.h"

#define PY_SSIZE_T_CLEAN
#include <Python.h>

#include <memory>

#include "google/protobuf/message.h"
#include "google/protobuf/pyext/free_threading/access_guard.h"
#include "google/protobuf/pyext/message.h"
#include "google/protobuf/pyext/message_tree.h"

namespace google {
namespace protobuf {
namespace python {

bool FieldContainer::CheckParentNotFrozen(const char* error_msg) const {
  return WithRead(false, [&](const ReadAccess& r) {
    return CheckFrozen(Owner(r), r, error_msg) == 0;
  });
}

// All containers which are not messages:
// - Make a new parent message
// - Copy the field
// - return the field.
PyObject* FieldContainer::DeepCopy() {
  CMessage* new_parent =
      this->WithRead(nullptr, [&](const ReadAccess& r) -> CMessage* {
        const Message* parent_msg = this->TargetMessage(r);
        CMessage* new_parent =
            CMessage::NewEmpty(this->parent.Get(r)->GetMessageClass());
        if (new_parent == nullptr) {
          return nullptr;
        }
        // new_parent is newly created and not yet published to other
        // threads, so it can be initialized quiescently.
        Message* mutable_new = parent_msg->New(nullptr);
        new_parent->message.SetQuiescent(mutable_new);
        new_parent->InitTreeQuiescent(new MessageTree());

        // There is no API to copy a single field. The closest operation we have
        // is SwapFields.
        // So, we copy the source into a disposable message and then swap the
        // one field we care about from it.
        // If the performance of this operation matters we can do a copy of the
        // single field, but that would require huge switches for each
        // type+cardinality to call the right read/write field functions.
        std::unique_ptr<Message> tmp(parent_msg->New(nullptr));
        tmp->MergeFrom(*parent_msg);
        tmp->GetReflection()->SwapFields(tmp.get(), mutable_new,
                                         {this->parent_field_descriptor});
        return new_parent;
      });
  if (new_parent == nullptr) {
    return nullptr;
  }

  PyObject* result =
      cmessage::GetFieldValue(new_parent, this->parent_field_descriptor);
  Py_DECREF(new_parent);
  return result;
}

void FieldContainer::RemoveFromParentCache() {
  // Only called from container tp_dealloc, when no other thread can reach this
  // container.
  if (CMessage* parent = this->parent.GetQuiescent()) {
    parent->composite_fields.EraseIfEqualSelf(this->parent_field_descriptor,
                                              this->AsPyObject());
    Py_DECREF(this->parent.TakeQuiescent());
  }
}

}  // namespace python
}  // namespace protobuf
}  // namespace google
