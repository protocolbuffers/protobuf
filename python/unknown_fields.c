/*
 * Copyright (c) 2009-2021, Google LLC
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of Google LLC nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL Google LLC BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "python/unknown_fields.h"

#include "python/message.h"
#include "python/protobuf.h"

static const char* PyUpb_DecodeVarint(const char* ptr, const char* end,
                                      uint64_t* val) {
  *val = 0;
  for (int i = 0; ptr < end && i < 10; i++, ptr++) {
    uint64_t byte = (uint8_t)*ptr;
    *val |= (byte & 0x7f) << (i * 7);
    if ((byte & 0x80) == 0) {
      return ptr + 1;
    }
  }
  return NULL;
}

// -----------------------------------------------------------------------------
// UnknownFieldSet
// -----------------------------------------------------------------------------

typedef struct {
  PyObject_HEAD;
  PyObject* fields;
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
  return self;
}

// Generic functions to skip a value or group.

static const char* PyUpb_UnknownFieldSet_SkipGroup(const char* ptr,
                                                   const char* end,
                                                   int group_number);

static const char* PyUpb_UnknownFieldSet_SkipField(const char* ptr,
                                                   const char* end,
                                                   uint32_t tag) {
  int field_number = tag >> 3;
  int wire_type = tag & 7;
  switch (wire_type) {
    case kUpb_WireType_Varint: {
      uint64_t val;
      return PyUpb_DecodeVarint(ptr, end, &val);
    }
    case kUpb_WireType_64Bit:
      if (end - ptr < 8) return NULL;
      return ptr + 8;
    case kUpb_WireType_32Bit:
      if (end - ptr < 4) return NULL;
      return ptr + 4;
    case kUpb_WireType_Delimited: {
      uint64_t size;
      ptr = PyUpb_DecodeVarint(ptr, end, &size);
      if (!ptr || end - ptr < size) return NULL;
      return ptr + size;
    }
    case kUpb_WireType_StartGroup:
      return PyUpb_UnknownFieldSet_SkipGroup(ptr, end, field_number);
    case kUpb_WireType_EndGroup:
      return NULL;
    default:
      assert(0);
      return NULL;
  }
}

static const char* PyUpb_UnknownFieldSet_SkipGroup(const char* ptr,
                                                   const char* end,
                                                   int group_number) {
  uint32_t end_tag = (group_number << 3) | kUpb_WireType_EndGroup;
  while (true) {
    if (ptr == end) return NULL;
    uint64_t tag;
    ptr = PyUpb_DecodeVarint(ptr, end, &tag);
    if (!ptr) return NULL;
    if (tag == end_tag) return ptr;
    ptr = PyUpb_UnknownFieldSet_SkipField(ptr, end, tag);
    if (!ptr) return NULL;
  }
  return ptr;
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
    PyUpb_UnknownFieldSet* self, const char* ptr, const char* end) {
  PyUpb_ModuleState* s = PyUpb_ModuleState_Get();
  int type_id = 0;
  PyObject* msg = NULL;
  while (true) {
    if (ptr == end) goto err;
    uint64_t tag;
    ptr = PyUpb_DecodeVarint(ptr, end, &tag);
    if (!ptr) goto err;
    switch (tag) {
      case kUpb_MessageSet_EndItemTag:
        goto done;
      case kUpb_MessageSet_TypeIdTag: {
        uint64_t tmp;
        ptr = PyUpb_DecodeVarint(ptr, end, &tmp);
        if (!ptr) goto err;
        if (!type_id) type_id = tmp;
        break;
      }
      case kUpb_MessageSet_MessageTag: {
        uint64_t size;
        ptr = PyUpb_DecodeVarint(ptr, end, &size);
        if (!ptr || end - ptr < size) goto err;
        if (!msg) {
          msg = PyBytes_FromStringAndSize(ptr, size);
          if (!msg) goto err;
        } else {
          // already saw a message here so deliberately skipping the duplicate
        }
        ptr += size;
        break;
      }
      default:
        ptr = PyUpb_UnknownFieldSet_SkipField(ptr, end, tag);
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
    PyUpb_UnknownFieldSet* self, const char* ptr, const char* end) {
  self->fields = PyList_New(0);
  while (ptr < end) {
    uint64_t tag;
    ptr = PyUpb_DecodeVarint(ptr, end, &tag);
    if (!ptr) goto err;
    if (tag == kUpb_MessageSet_StartItemTag) {
      ptr = PyUpb_UnknownFieldSet_BuildMessageSetItem(self, ptr, end);
    } else {
      ptr = PyUpb_UnknownFieldSet_SkipField(ptr, end, tag);
    }
    if (!ptr) goto err;
  }
  return ptr;

err:
  Py_DECREF(self->fields);
  self->fields = NULL;
  return NULL;
}

static const char* PyUpb_UnknownFieldSet_Build(PyUpb_UnknownFieldSet* self,
                                               const char* ptr, const char* end,
                                               int group_number);

static const char* PyUpb_UnknownFieldSet_BuildValue(
    PyUpb_UnknownFieldSet* self, const char* ptr, const char* end,
    int field_number, int wire_type, int group_number, PyObject** data) {
  switch (wire_type) {
    case kUpb_WireType_Varint: {
      uint64_t val;
      ptr = PyUpb_DecodeVarint(ptr, end, &val);
      if (!ptr) return NULL;
      *data = PyLong_FromUnsignedLongLong(val);
      return ptr;
    }
    case kUpb_WireType_64Bit: {
      if (end - ptr < 8) return NULL;
      uint64_t val;
      memcpy(&val, ptr, 8);
      *data = PyLong_FromUnsignedLongLong(val);
      return ptr + 8;
    }
    case kUpb_WireType_32Bit: {
      if (end - ptr < 4) return NULL;
      uint32_t val;
      memcpy(&val, ptr, 4);
      *data = PyLong_FromUnsignedLongLong(val);
      return ptr + 4;
    }
    case kUpb_WireType_Delimited: {
      uint64_t size;
      ptr = PyUpb_DecodeVarint(ptr, end, &size);
      if (!ptr || end - ptr < size) return NULL;
      *data = PyBytes_FromStringAndSize(ptr, size);
      return ptr + size;
    }
    case kUpb_WireType_StartGroup: {
      PyUpb_UnknownFieldSet* sub = PyUpb_UnknownFieldSet_NewBare();
      if (!sub) return NULL;
      *data = &sub->ob_base;
      return PyUpb_UnknownFieldSet_Build(sub, ptr, end, field_number);
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
                                               const char* ptr, const char* end,
                                               int group_number) {
  PyUpb_ModuleState* s = PyUpb_ModuleState_Get();
  self->fields = PyList_New(0);
  while (ptr < end) {
    uint64_t tag;
    ptr = PyUpb_DecodeVarint(ptr, end, &tag);
    if (!ptr) goto err;
    PyObject* data = NULL;
    int field_number = tag >> 3;
    int wire_type = tag & 7;
    if (wire_type == kUpb_WireType_EndGroup) {
      if (field_number != group_number) return NULL;
      return ptr;
    }
    ptr = PyUpb_UnknownFieldSet_BuildValue(self, ptr, end, field_number,
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
  return ptr;

err:
  Py_DECREF(self->fields);
  self->fields = NULL;
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

  size_t size;
  const char* ptr = upb_Message_GetUnknown(msg, &size);
  if (size == 0) return &self->ob_base;

  const char* end = ptr + size;
  const upb_MessageDef* msgdef = PyUpb_Message_GetMsgdef(py_msg);

  bool ok;
  if (upb_MessageDef_IsMessageSet(msgdef)) {
    ok = PyUpb_UnknownFieldSet_BuildMessageSet(self, ptr, end) == end;
  } else {
    ok = PyUpb_UnknownFieldSet_Build(self, ptr, end, -1) == end;
  }

  if (!ok) {
    Py_DECREF(&self->ob_base);
    return NULL;
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
