// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "python/unknown_fields.h"

#include "python/message.h"
#include "python/protobuf.h"
#include "upb/message/message.h"
#include "upb/wire/eps_copy_input_stream.h"
#include "upb/wire/reader.h"
#include "upb/wire/types.h"

// -----------------------------------------------------------------------------
// UnknownFieldSet
// -----------------------------------------------------------------------------

typedef struct {
  // clang-format off
  PyObject_HEAD
  PyObject* fields;
  // clang-format on
} PyUpb_UnknownFieldSet;

static void PyUpb_UnknownFieldSet_Dealloc(PyObject* _self) {
  PyUpb_UnknownFieldSet* self = (PyUpb_UnknownFieldSet*)_self;
  Py_XDECREF(self->fields);
  PyUpb_Dealloc(self);
}

PyUpb_UnknownFieldSet* PyUpb_UnknownFieldSet_NewBare(void) {
  PyUpb_ModuleState* s = PyUpb_ModuleState_Get();
  PyUpb_UnknownFieldSet* self =
      (void*)PyType_GenericAlloc(s->unknown_fields_type, 0);
  self->fields = PyList_New(0);
  return self;
}

// For MessageSet the established behavior is for UnknownFieldSet to interpret
// the MessageSet wire format:
//    message MessageSet {
//      repeated group Item = 1 {
//        required int32 type_id = 2;
//        required bytes message = 3;
//      }
//    }
//
// And create unknown fields like:
//   UnknownField(type_id, WIRE_TYPE_DELIMITED, message)
//
// For any unknown fields that are unexpected per the wire format defined above,
// we drop them on the floor.

enum {
  kUpb_MessageSet_StartItemTag = (1 << 3) | kUpb_WireType_StartGroup,
  kUpb_MessageSet_EndItemTag = (1 << 3) | kUpb_WireType_EndGroup,
  kUpb_MessageSet_TypeIdTag = (2 << 3) | kUpb_WireType_Varint,
  kUpb_MessageSet_MessageTag = (3 << 3) | kUpb_WireType_Delimited,
};

static const char* PyUpb_UnknownFieldSet_BuildMessageSetItem(
    PyUpb_UnknownFieldSet* self, upb_EpsCopyInputStream* stream,
    const char* ptr) {
  PyUpb_ModuleState* s = PyUpb_ModuleState_Get();
  int type_id = 0;
  PyObject* msg = NULL;
  while (!upb_EpsCopyInputStream_IsDone(stream, &ptr)) {
    uint32_t tag;
    ptr = upb_WireReader_ReadTag(ptr, &tag);
    if (!ptr) goto err;
    switch (tag) {
      case kUpb_MessageSet_EndItemTag:
        goto done;
      case kUpb_MessageSet_TypeIdTag: {
        uint64_t tmp;
        ptr = upb_WireReader_ReadVarint(ptr, &tmp);
        if (!ptr) goto err;
        if (!type_id) type_id = tmp;
        break;
      }
      case kUpb_MessageSet_MessageTag: {
        int size;
        ptr = upb_WireReader_ReadSize(ptr, &size);
        if (!upb_EpsCopyInputStream_CheckDataSizeAvailable(stream, ptr, size)) {
          goto err;
        }
        const char* str = ptr;
        ptr = upb_EpsCopyInputStream_ReadStringAliased(stream, &str, size);
        if (!msg) {
          msg = PyBytes_FromStringAndSize(str, size);
          if (!msg) goto err;
        } else {
          // already saw a message here so deliberately skipping the duplicate
        }
        break;
      }
      default:
        ptr = upb_WireReader_SkipValue(ptr, tag, stream);
        if (!ptr) goto err;
    }
  }

done:
  if (type_id && msg) {
    PyObject* field = PyObject_CallFunction(
        s->unknown_field_type, "iiO", type_id, kUpb_WireType_Delimited, msg);
    if (!field) goto err;
    PyList_Append(self->fields, field);
    Py_DECREF(field);
  }
  Py_XDECREF(msg);
  return ptr;

err:
  Py_XDECREF(msg);
  return NULL;
}

static const char* PyUpb_UnknownFieldSet_BuildMessageSet(
    PyUpb_UnknownFieldSet* self, upb_EpsCopyInputStream* stream,
    const char* ptr) {
  while (!upb_EpsCopyInputStream_IsDone(stream, &ptr)) {
    uint32_t tag;
    ptr = upb_WireReader_ReadTag(ptr, &tag);
    if (!ptr) goto err;
    if (tag == kUpb_MessageSet_StartItemTag) {
      ptr = PyUpb_UnknownFieldSet_BuildMessageSetItem(self, stream, ptr);
    } else {
      ptr = upb_WireReader_SkipValue(ptr, tag, stream);
    }
    if (!ptr) goto err;
  }
  if (upb_EpsCopyInputStream_IsError(stream)) goto err;
  return ptr;

err:
  Py_DECREF(self->fields);
  return NULL;
}

static const char* PyUpb_UnknownFieldSet_Build(PyUpb_UnknownFieldSet* self,
                                               upb_EpsCopyInputStream* stream,
                                               const char* ptr,
                                               int group_number);

static const char* PyUpb_UnknownFieldSet_BuildValue(
    PyUpb_UnknownFieldSet* self, upb_EpsCopyInputStream* stream,
    const char* ptr, int field_number, int wire_type, int group_number,
    PyObject** data) {
  switch (wire_type) {
    case kUpb_WireType_Varint: {
      uint64_t val;
      ptr = upb_WireReader_ReadVarint(ptr, &val);
      if (!ptr) return NULL;
      *data = PyLong_FromUnsignedLongLong(val);
      return ptr;
    }
    case kUpb_WireType_64Bit: {
      uint64_t val;
      ptr = upb_WireReader_ReadFixed64(ptr, &val);
      *data = PyLong_FromUnsignedLongLong(val);
      return ptr;
    }
    case kUpb_WireType_32Bit: {
      uint32_t val;
      ptr = upb_WireReader_ReadFixed32(ptr, &val);
      *data = PyLong_FromUnsignedLongLong(val);
      return ptr;
    }
    case kUpb_WireType_Delimited: {
      int size;
      ptr = upb_WireReader_ReadSize(ptr, &size);
      if (!upb_EpsCopyInputStream_CheckDataSizeAvailable(stream, ptr, size)) {
        return NULL;
      }
      const char* str = ptr;
      ptr = upb_EpsCopyInputStream_ReadStringAliased(stream, &str, size);
      *data = PyBytes_FromStringAndSize(str, size);
      return ptr;
    }
    case kUpb_WireType_StartGroup: {
      PyUpb_UnknownFieldSet* sub = PyUpb_UnknownFieldSet_NewBare();
      if (!sub) return NULL;
      *data = &sub->ob_base;
      return PyUpb_UnknownFieldSet_Build(sub, stream, ptr, field_number);
    }
    default:
      assert(0);
      *data = NULL;
      return NULL;
  }
}

// For non-MessageSet we just build the unknown fields exactly as they exist on
// the wire.
static const char* PyUpb_UnknownFieldSet_Build(PyUpb_UnknownFieldSet* self,
                                               upb_EpsCopyInputStream* stream,
                                               const char* ptr,
                                               int group_number) {
  PyUpb_ModuleState* s = PyUpb_ModuleState_Get();
  while (!upb_EpsCopyInputStream_IsDone(stream, &ptr)) {
    uint32_t tag;
    ptr = upb_WireReader_ReadTag(ptr, &tag);
    if (!ptr) goto err;
    PyObject* data = NULL;
    int field_number = upb_WireReader_GetFieldNumber(tag);
    int wire_type = upb_WireReader_GetWireType(tag);
    if (wire_type == kUpb_WireType_EndGroup) {
      if (field_number != group_number) return NULL;
      return ptr;
    }
    ptr = PyUpb_UnknownFieldSet_BuildValue(self, stream, ptr, field_number,
                                           wire_type, group_number, &data);
    if (!ptr) {
      Py_XDECREF(data);
      goto err;
    }
    assert(data);
    PyObject* field = PyObject_CallFunction(s->unknown_field_type, "iiN",
                                            field_number, wire_type, data);
    PyList_Append(self->fields, field);
    Py_DECREF(field);
  }
  if (upb_EpsCopyInputStream_IsError(stream)) goto err;
  return ptr;

err:
  Py_DECREF(self->fields);
  return NULL;
}

static PyObject* PyUpb_UnknownFieldSet_New(PyTypeObject* type, PyObject* args,
                                           PyObject* kwargs) {
  char* kwlist[] = {"message", 0};
  PyObject* py_msg = NULL;

  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O", kwlist, &py_msg)) {
    return NULL;
  }

  if (!PyUpb_Message_Verify(py_msg)) return NULL;
  PyUpb_UnknownFieldSet* self = PyUpb_UnknownFieldSet_NewBare();
  upb_Message* msg = PyUpb_Message_GetIfReified(py_msg);
  if (!msg) return &self->ob_base;

  uintptr_t iter = kUpb_Message_UnknownBegin;
  upb_StringView view;
  while (upb_Message_NextUnknown(msg, &view, &iter)) {
    const char* ptr = view.data;
    upb_EpsCopyInputStream stream;
    upb_EpsCopyInputStream_Init(&stream, &ptr, view.size, true);
    const upb_MessageDef* msgdef = PyUpb_Message_GetMsgdef(py_msg);

    bool ok;
    if (upb_MessageDef_IsMessageSet(msgdef)) {
      ok = PyUpb_UnknownFieldSet_BuildMessageSet(self, &stream, ptr) != NULL;
    } else {
      ok = PyUpb_UnknownFieldSet_Build(self, &stream, ptr, -1) != NULL;
    }

    if (!ok) {
      Py_DECREF(&self->ob_base);
      return NULL;
    }
  }

  return &self->ob_base;
}

static Py_ssize_t PyUpb_UnknownFieldSet_Length(PyObject* _self) {
  PyUpb_UnknownFieldSet* self = (PyUpb_UnknownFieldSet*)_self;
  return self->fields ? PyObject_Length(self->fields) : 0;
}

static PyObject* PyUpb_UnknownFieldSet_GetItem(PyObject* _self,
                                               Py_ssize_t index) {
  PyUpb_UnknownFieldSet* self = (PyUpb_UnknownFieldSet*)_self;
  if (!self->fields) {
    PyErr_Format(PyExc_IndexError, "list index (%zd) out of range", index);
    return NULL;
  }
  PyObject* ret = PyList_GetItem(self->fields, index);
  if (ret) Py_INCREF(ret);
  return ret;
}

static PyType_Slot PyUpb_UnknownFieldSet_Slots[] = {
    {Py_tp_new, &PyUpb_UnknownFieldSet_New},
    {Py_tp_dealloc, &PyUpb_UnknownFieldSet_Dealloc},
    {Py_sq_length, PyUpb_UnknownFieldSet_Length},
    {Py_sq_item, PyUpb_UnknownFieldSet_GetItem},
    {Py_tp_hash, PyObject_HashNotImplemented},
    {0, NULL},
};

static PyType_Spec PyUpb_UnknownFieldSet_Spec = {
    PYUPB_MODULE_NAME ".UnknownFieldSet",  // tp_name
    sizeof(PyUpb_UnknownFieldSet),         // tp_basicsize
    0,                                     // tp_itemsize
    Py_TPFLAGS_DEFAULT,                    // tp_flags
    PyUpb_UnknownFieldSet_Slots,
};

// -----------------------------------------------------------------------------
// Top Level
// -----------------------------------------------------------------------------

PyObject* PyUpb_UnknownFieldSet_CreateNamedTuple(void) {
  PyObject* mod = NULL;
  PyObject* namedtuple = NULL;
  PyObject* ret = NULL;

  mod = PyImport_ImportModule("collections");
  if (!mod) goto done;
  namedtuple = PyObject_GetAttrString(mod, "namedtuple");
  if (!namedtuple) goto done;
  ret = PyObject_CallFunction(namedtuple, "s[sss]", "PyUnknownField",
                              "field_number", "wire_type", "data");

done:
  Py_XDECREF(mod);
  Py_XDECREF(namedtuple);
  return ret;
}

bool PyUpb_UnknownFields_Init(PyObject* m) {
  PyUpb_ModuleState* s = PyUpb_ModuleState_GetFromModule(m);

  s->unknown_fields_type = PyUpb_AddClass(m, &PyUpb_UnknownFieldSet_Spec);
  s->unknown_field_type = PyUpb_UnknownFieldSet_CreateNamedTuple();

  return s->unknown_fields_type && s->unknown_field_type;
}
