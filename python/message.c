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

#include "python/message.h"

#include "python/convert.h"
#include "python/descriptor.h"
#include "python/extension_dict.h"
#include "python/map.h"
#include "python/repeated.h"
#include "upb/reflection/def.h"
#include "upb/reflection/message.h"
#include "upb/text/encode.h"
#include "upb/util/required_fields.h"
#include "upb/wire/common.h"

static const upb_MessageDef* PyUpb_MessageMeta_GetMsgdef(PyObject* cls);
static PyObject* PyUpb_MessageMeta_GetAttr(PyObject* self, PyObject* name);

// -----------------------------------------------------------------------------
// CPythonBits
// -----------------------------------------------------------------------------

// This struct contains a few things that are not exposed directly through the
// limited API, but that we can get at in somewhat more roundabout ways. The
// roundabout ways are slower, so we cache the values here.
//
// These values are valid to cache in a global, even across sub-interpreters,
// because they are not pointers to interpreter state.  They are process
// globals that will be the same for any interpreter in this process.
typedef struct {
  // For each member, we note the equivalent expression that we could use in the
  // full (non-limited) API.
  newfunc type_new;            // PyTypeObject.tp_new
  destructor type_dealloc;     // PyTypeObject.tp_dealloc
  getattrofunc type_getattro;  // PyTypeObject.tp_getattro
  setattrofunc type_setattro;  // PyTypeObject.tp_setattro
  size_t type_basicsize;       // sizeof(PyHeapTypeObject)

  // While we can refer to PY_VERSION_HEX in the limited API, this will give us
  // the version of Python we were compiled against, which may be different
  // than the version we are dynamically linked against.  Here we want the
  // version that is actually running in this process.
  long python_version_hex;  // PY_VERSION_HEX
} PyUpb_CPythonBits;

// A global containing the values for this process.
PyUpb_CPythonBits cpython_bits;

destructor upb_Pre310_PyType_GetDeallocSlot(PyTypeObject* type_subclass) {
  // This is a bit desperate.  We need type_dealloc(), but PyType_GetSlot(type,
  // Py_tp_dealloc) will return subtype_dealloc().  There appears to be no way
  // whatsoever to fetch type_dealloc() through the limited API until Python
  // 3.10.
  //
  // To work around this so we attempt to find it by looking for the offset of
  // tp_dealloc in PyTypeObject, then memcpy() it directly.  This should always
  // work in practice.
  //
  // Starting with Python 3.10 on you can call PyType_GetSlot() on non-heap
  // types.  We will be able to replace all this hack with just:
  //
  //   PyType_GetSlot(&PyType_Type, Py_tp_dealloc)
  //
  destructor subtype_dealloc = PyType_GetSlot(type_subclass, Py_tp_dealloc);
  for (size_t i = 0; i < 2000; i += sizeof(uintptr_t)) {
    destructor maybe_subtype_dealloc;
    memcpy(&maybe_subtype_dealloc, (char*)type_subclass + i,
           sizeof(destructor));
    if (maybe_subtype_dealloc == subtype_dealloc) {
      destructor type_dealloc;
      memcpy(&type_dealloc, (char*)&PyType_Type + i, sizeof(destructor));
      return type_dealloc;
    }
  }
  assert(false);
  return NULL;
}

static bool PyUpb_CPythonBits_Init(PyUpb_CPythonBits* bits) {
  PyObject* bases = NULL;
  PyTypeObject* type = NULL;
  PyObject* size = NULL;
  PyObject* sys = NULL;
  PyObject* hex_version = NULL;
  bool ret = false;

  // PyType_GetSlot() only works on heap types, so we cannot use it on
  // &PyType_Type directly. Instead we create our own (temporary) type derived
  // from PyType_Type: this will inherit all of the slots from PyType_Type, but
  // as a heap type it can be queried with PyType_GetSlot().
  static PyType_Slot dummy_slots[] = {{0, NULL}};

  static PyType_Spec dummy_spec = {
      "module.DummyClass",  // tp_name
      0,  // To be filled in by size of base     // tp_basicsize
      0,  // tp_itemsize
      Py_TPFLAGS_DEFAULT,  // tp_flags
      dummy_slots,
  };

  bases = Py_BuildValue("(O)", &PyType_Type);
  if (!bases) goto err;
  type = (PyTypeObject*)PyType_FromSpecWithBases(&dummy_spec, bases);
  if (!type) goto err;

  bits->type_new = PyType_GetSlot(type, Py_tp_new);
  bits->type_dealloc = upb_Pre310_PyType_GetDeallocSlot(type);
  bits->type_getattro = PyType_GetSlot(type, Py_tp_getattro);
  bits->type_setattro = PyType_GetSlot(type, Py_tp_setattro);

  size = PyObject_GetAttrString((PyObject*)&PyType_Type, "__basicsize__");
  if (!size) goto err;
  bits->type_basicsize = PyLong_AsLong(size);
  if (bits->type_basicsize == -1) goto err;

  assert(bits->type_new);
  assert(bits->type_dealloc);
  assert(bits->type_getattro);
  assert(bits->type_setattro);

#ifndef Py_LIMITED_API
  assert(bits->type_new == PyType_Type.tp_new);
  assert(bits->type_dealloc == PyType_Type.tp_dealloc);
  assert(bits->type_getattro == PyType_Type.tp_getattro);
  assert(bits->type_setattro == PyType_Type.tp_setattro);
  assert(bits->type_basicsize == sizeof(PyHeapTypeObject));
#endif

  sys = PyImport_ImportModule("sys");
  hex_version = PyObject_GetAttrString(sys, "hexversion");
  bits->python_version_hex = PyLong_AsLong(hex_version);
  ret = true;

err:
  Py_XDECREF(bases);
  Py_XDECREF(type);
  Py_XDECREF(size);
  Py_XDECREF(sys);
  Py_XDECREF(hex_version);
  return ret;
}

// -----------------------------------------------------------------------------
// Message
// -----------------------------------------------------------------------------

// The main message object.  The type of the object (PyUpb_Message.ob_type)
// will be an instance of the PyUpb_MessageMeta type (defined below).  So the
// chain is:
//   FooMessage = MessageMeta(...)
//   foo = FooMessage()
//
// Which becomes:
//   Object             C Struct Type        Python type (ob_type)
//   -----------------  -----------------    ---------------------
//   foo                PyUpb_Message        FooMessage
//   FooMessage         PyUpb_MessageMeta    message_meta_type
//   message_meta_type  PyTypeObject         'type' in Python
//
// A message object can be in one of two states: present or non-present.  When
// a message is non-present, it stores a reference to its parent, and a write
// to any attribute will trigger the message to become present in its parent.
// The parent may also be non-present, in which case a mutation will trigger a
// chain reaction.
typedef struct PyUpb_Message {
  PyObject_HEAD;
  PyObject* arena;
  uintptr_t def;  // Tagged, low bit 1 == upb_FieldDef*, else upb_MessageDef*
  union {
    // when def is msgdef, the data for this msg.
    upb_Message* msg;
    // when def is fielddef, owning pointer to parent
    struct PyUpb_Message* parent;
  } ptr;
  PyObject* ext_dict;  // Weak pointer to extension dict, if any.
  // name->obj dict for non-present msg/map/repeated, NULL if none.
  PyUpb_WeakMap* unset_subobj_map;
  int version;
} PyUpb_Message;

static PyObject* PyUpb_Message_GetAttr(PyObject* _self, PyObject* attr);

bool PyUpb_Message_IsStub(PyUpb_Message* msg) { return msg->def & 1; }

const upb_FieldDef* PyUpb_Message_GetFieldDef(PyUpb_Message* msg) {
  assert(PyUpb_Message_IsStub(msg));
  return (void*)(msg->def & ~(uintptr_t)1);
}

static const upb_MessageDef* _PyUpb_Message_GetMsgdef(PyUpb_Message* msg) {
  return PyUpb_Message_IsStub(msg)
             ? upb_FieldDef_MessageSubDef(PyUpb_Message_GetFieldDef(msg))
             : (void*)msg->def;
}

const upb_MessageDef* PyUpb_Message_GetMsgdef(PyObject* self) {
  return _PyUpb_Message_GetMsgdef((PyUpb_Message*)self);
}

static upb_Message* PyUpb_Message_GetMsg(PyUpb_Message* self) {
  assert(!PyUpb_Message_IsStub(self));
  return self->ptr.msg;
}

bool PyUpb_Message_TryCheck(PyObject* self) {
  PyUpb_ModuleState* state = PyUpb_ModuleState_Get();
  PyObject* type = (PyObject*)Py_TYPE(self);
  return Py_TYPE(type) == state->message_meta_type;
}

bool PyUpb_Message_Verify(PyObject* self) {
  if (!PyUpb_Message_TryCheck(self)) {
    PyErr_Format(PyExc_TypeError, "Expected a message object, but got %R.",
                 self);
    return false;
  }
  return true;
}

// If the message is reified, returns it.  Otherwise, returns NULL.
// If NULL is returned, the object is empty and has no underlying data.
upb_Message* PyUpb_Message_GetIfReified(PyObject* _self) {
  PyUpb_Message* self = (void*)_self;
  return PyUpb_Message_IsStub(self) ? NULL : self->ptr.msg;
}

static PyObject* PyUpb_Message_New(PyObject* cls, PyObject* unused_args,
                                   PyObject* unused_kwargs) {
  const upb_MessageDef* msgdef = PyUpb_MessageMeta_GetMsgdef(cls);
  const upb_MiniTable* layout = upb_MessageDef_MiniTable(msgdef);
  PyUpb_Message* msg = (void*)PyType_GenericAlloc((PyTypeObject*)cls, 0);
  msg->def = (uintptr_t)msgdef;
  msg->arena = PyUpb_Arena_New();
  msg->ptr.msg = upb_Message_New(layout, PyUpb_Arena_Get(msg->arena));
  msg->unset_subobj_map = NULL;
  msg->ext_dict = NULL;
  msg->version = 0;

  PyObject* ret = &msg->ob_base;
  PyUpb_ObjCache_Add(msg->ptr.msg, ret);
  return ret;
}

/*
 * PyUpb_Message_LookupName()
 *
 * Tries to find a field or oneof named `py_name` in the message object `self`.
 * The user must pass `f` and/or `o` to indicate whether a field or a oneof name
 * is expected.  If the name is found and it has an expected type, the function
 * sets `*f` or `*o` respectively and returns true.  Otherwise returns false
 * and sets an exception of type `exc_type` if provided.
 */
static bool PyUpb_Message_LookupName(PyUpb_Message* self, PyObject* py_name,
                                     const upb_FieldDef** f,
                                     const upb_OneofDef** o,
                                     PyObject* exc_type) {
  assert(f || o);
  Py_ssize_t size;
  const char* name = NULL;
  if (PyUnicode_Check(py_name)) {
    name = PyUnicode_AsUTF8AndSize(py_name, &size);
  } else if (PyBytes_Check(py_name)) {
    PyBytes_AsStringAndSize(py_name, (char**)&name, &size);
  }
  if (!name) {
    PyErr_Format(exc_type,
                 "Expected a field name, but got non-string argument %S.",
                 py_name);
    return false;
  }
  const upb_MessageDef* msgdef = _PyUpb_Message_GetMsgdef(self);

  if (!upb_MessageDef_FindByNameWithSize(msgdef, name, size, f, o)) {
    if (exc_type) {
      PyErr_Format(exc_type, "Protocol message %s has no \"%s\" field.",
                   upb_MessageDef_Name(msgdef), name);
    }
    return false;
  }

  if (!o && !*f) {
    if (exc_type) {
      PyErr_Format(exc_type, "Expected a field name, but got oneof name %s.",
                   name);
    }
    return false;
  }

  if (!f && !*o) {
    if (exc_type) {
      PyErr_Format(exc_type, "Expected a oneof name, but got field name %s.",
                   name);
    }
    return false;
  }

  return true;
}

static bool PyUpb_Message_InitMessageMapEntry(PyObject* dst, PyObject* src) {
  if (!src || !dst) return false;

  // TODO(haberman): Currently we are doing Clear()+MergeFrom().  Replace with
  // CopyFrom() once that is implemented.
  PyObject* ok = PyObject_CallMethod(dst, "Clear", NULL);
  if (!ok) return false;
  Py_DECREF(ok);
  ok = PyObject_CallMethod(dst, "MergeFrom", "O", src);
  if (!ok) return false;
  Py_DECREF(ok);

  return true;
}

int PyUpb_Message_InitMapAttributes(PyObject* map, PyObject* value,
                                    const upb_FieldDef* f) {
  const upb_MessageDef* entry_m = upb_FieldDef_MessageSubDef(f);
  const upb_FieldDef* val_f = upb_MessageDef_Field(entry_m, 1);
  PyObject* it = NULL;
  PyObject* tmp = NULL;
  int ret = -1;
  if (upb_FieldDef_IsSubMessage(val_f)) {
    it = PyObject_GetIter(value);
    if (it == NULL) {
      PyErr_Format(PyExc_TypeError, "Argument for field %s is not iterable",
                   upb_FieldDef_FullName(f));
      goto err;
    }
    PyObject* e;
    while ((e = PyIter_Next(it)) != NULL) {
      PyObject* src = PyObject_GetItem(value, e);
      PyObject* dst = PyObject_GetItem(map, e);
      Py_DECREF(e);
      bool ok = PyUpb_Message_InitMessageMapEntry(dst, src);
      Py_XDECREF(src);
      Py_XDECREF(dst);
      if (!ok) goto err;
    }
  } else {
    tmp = PyObject_CallMethod(map, "update", "O", value);
    if (!tmp) goto err;
  }
  ret = 0;

err:
  Py_XDECREF(it);
  Py_XDECREF(tmp);
  return ret;
}

void PyUpb_Message_EnsureReified(PyUpb_Message* self);

static bool PyUpb_Message_InitMapAttribute(PyObject* _self, PyObject* name,
                                           const upb_FieldDef* f,
                                           PyObject* value) {
  PyObject* map = PyUpb_Message_GetAttr(_self, name);
  int ok = PyUpb_Message_InitMapAttributes(map, value, f);
  Py_DECREF(map);
  return ok >= 0;
}

static bool PyUpb_Message_InitRepeatedMessageAttribute(PyObject* _self,
                                                       PyObject* repeated,
                                                       PyObject* value,
                                                       const upb_FieldDef* f) {
  PyObject* it = PyObject_GetIter(value);
  if (!it) {
    PyErr_Format(PyExc_TypeError, "Argument for field %s is not iterable",
                 upb_FieldDef_FullName(f));
    return false;
  }
  PyObject* e = NULL;
  PyObject* m = NULL;
  while ((e = PyIter_Next(it)) != NULL) {
    if (PyDict_Check(e)) {
      m = PyUpb_RepeatedCompositeContainer_Add(repeated, NULL, e);
      if (!m) goto err;
    } else {
      m = PyUpb_RepeatedCompositeContainer_Add(repeated, NULL, NULL);
      if (!m) goto err;
      PyObject* merged = PyUpb_Message_MergeFrom(m, e);
      if (!merged) goto err;
      Py_DECREF(merged);
    }
    Py_DECREF(e);
    Py_DECREF(m);
    m = NULL;
  }

err:
  Py_XDECREF(it);
  Py_XDECREF(e);
  Py_XDECREF(m);
  return !PyErr_Occurred();  // Check PyIter_Next() exit.
}

static bool PyUpb_Message_InitRepeatedAttribute(PyObject* _self, PyObject* name,
                                                PyObject* value) {
  PyUpb_Message* self = (void*)_self;
  const upb_FieldDef* field;
  if (!PyUpb_Message_LookupName(self, name, &field, NULL,
                                PyExc_AttributeError)) {
    return false;
  }
  bool ok = false;
  PyObject* repeated = PyUpb_Message_GetFieldValue(_self, field);
  PyObject* tmp = NULL;
  if (!repeated) goto err;
  if (upb_FieldDef_IsSubMessage(field)) {
    if (!PyUpb_Message_InitRepeatedMessageAttribute(_self, repeated, value,
                                                    field)) {
      goto err;
    }
  } else {
    tmp = PyUpb_RepeatedContainer_Extend(repeated, value);
    if (!tmp) goto err;
  }
  ok = true;

err:
  Py_XDECREF(repeated);
  Py_XDECREF(tmp);
  return ok;
}

static PyObject* PyUpb_Message_MergePartialFrom(PyObject*, PyObject*);

static bool PyUpb_Message_InitMessageAttribute(PyObject* _self, PyObject* name,
                                               PyObject* value) {
  PyObject* submsg = PyUpb_Message_GetAttr(_self, name);
  if (!submsg) return -1;
  assert(!PyErr_Occurred());
  bool ok;
  if (PyUpb_Message_TryCheck(value)) {
    PyObject* tmp = PyUpb_Message_MergePartialFrom(submsg, value);
    ok = tmp != NULL;
    Py_XDECREF(tmp);
  } else if (PyDict_Check(value)) {
    assert(!PyErr_Occurred());
    ok = PyUpb_Message_InitAttributes(submsg, NULL, value) >= 0;
  } else {
    const upb_MessageDef* m = PyUpb_Message_GetMsgdef(_self);
    PyErr_Format(PyExc_TypeError, "Message must be initialized with a dict: %s",
                 upb_MessageDef_FullName(m));
    ok = false;
  }
  Py_DECREF(submsg);
  return ok;
}

static bool PyUpb_Message_InitScalarAttribute(upb_Message* msg,
                                              const upb_FieldDef* f,
                                              PyObject* value,
                                              upb_Arena* arena) {
  upb_MessageValue msgval;
  assert(!PyErr_Occurred());
  if (!PyUpb_PyToUpb(value, f, &msgval, arena)) return false;
  upb_Message_SetFieldByDef(msg, f, msgval, arena);
  return true;
}

int PyUpb_Message_InitAttributes(PyObject* _self, PyObject* args,
                                 PyObject* kwargs) {
  assert(!PyErr_Occurred());

  if (args != NULL && PyTuple_Size(args) != 0) {
    PyErr_SetString(PyExc_TypeError, "No positional arguments allowed");
    return -1;
  }

  if (kwargs == NULL) return 0;

  PyUpb_Message* self = (void*)_self;
  Py_ssize_t pos = 0;
  PyObject* name;
  PyObject* value;
  PyUpb_Message_EnsureReified(self);
  upb_Message* msg = PyUpb_Message_GetMsg(self);
  upb_Arena* arena = PyUpb_Arena_Get(self->arena);

  while (PyDict_Next(kwargs, &pos, &name, &value)) {
    assert(!PyErr_Occurred());
    const upb_FieldDef* f;
    assert(!PyErr_Occurred());
    if (!PyUpb_Message_LookupName(self, name, &f, NULL, PyExc_ValueError)) {
      return -1;
    }

    if (value == Py_None) continue;  // Ignored.

    assert(!PyErr_Occurred());

    if (upb_FieldDef_IsMap(f)) {
      if (!PyUpb_Message_InitMapAttribute(_self, name, f, value)) return -1;
    } else if (upb_FieldDef_IsRepeated(f)) {
      if (!PyUpb_Message_InitRepeatedAttribute(_self, name, value)) return -1;
    } else if (upb_FieldDef_IsSubMessage(f)) {
      if (!PyUpb_Message_InitMessageAttribute(_self, name, value)) return -1;
    } else {
      if (!PyUpb_Message_InitScalarAttribute(msg, f, value, arena)) return -1;
    }
    if (PyErr_Occurred()) return -1;
  }

  if (PyErr_Occurred()) return -1;
  return 0;
}

static int PyUpb_Message_Init(PyObject* _self, PyObject* args,
                              PyObject* kwargs) {
  if (args != NULL && PyTuple_Size(args) != 0) {
    PyErr_SetString(PyExc_TypeError, "No positional arguments allowed");
    return -1;
  }

  return PyUpb_Message_InitAttributes(_self, args, kwargs);
}

static PyObject* PyUpb_Message_NewStub(PyObject* parent, const upb_FieldDef* f,
                                       PyObject* arena) {
  const upb_MessageDef* sub_m = upb_FieldDef_MessageSubDef(f);
  PyObject* cls = PyUpb_Descriptor_GetClass(sub_m);

  PyUpb_Message* msg = (void*)PyType_GenericAlloc((PyTypeObject*)cls, 0);
  msg->def = (uintptr_t)f | 1;
  msg->arena = arena;
  msg->ptr.parent = (PyUpb_Message*)parent;
  msg->unset_subobj_map = NULL;
  msg->ext_dict = NULL;
  msg->version = 0;

  Py_DECREF(cls);
  Py_INCREF(parent);
  Py_INCREF(arena);
  return &msg->ob_base;
}

static bool PyUpb_Message_IsEmpty(const upb_Message* msg,
                                  const upb_MessageDef* m,
                                  const upb_DefPool* ext_pool) {
  if (!msg) return true;

  size_t iter = kUpb_Message_Begin;
  const upb_FieldDef* f;
  upb_MessageValue val;
  if (upb_Message_Next(msg, m, ext_pool, &f, &val, &iter)) return false;

  size_t len;
  (void)upb_Message_GetUnknown(msg, &len);
  return len == 0;
}

static bool PyUpb_Message_IsEqual(PyUpb_Message* m1, PyObject* _m2) {
  PyUpb_Message* m2 = (void*)_m2;
  if (m1 == m2) return true;
  if (!PyObject_TypeCheck(_m2, m1->ob_base.ob_type)) {
    return false;
  }
  const upb_MessageDef* m1_msgdef = _PyUpb_Message_GetMsgdef(m1);
#ifndef NDEBUG
  const upb_MessageDef* m2_msgdef = _PyUpb_Message_GetMsgdef(m2);
  assert(m1_msgdef == m2_msgdef);
#endif
  const upb_Message* m1_msg = PyUpb_Message_GetIfReified((PyObject*)m1);
  const upb_Message* m2_msg = PyUpb_Message_GetIfReified(_m2);
  const upb_DefPool* symtab = upb_FileDef_Pool(upb_MessageDef_File(m1_msgdef));

  const bool e1 = PyUpb_Message_IsEmpty(m1_msg, m1_msgdef, symtab);
  const bool e2 = PyUpb_Message_IsEmpty(m2_msg, m1_msgdef, symtab);
  if (e1 || e2) return e1 && e2;

  return upb_Message_IsEqual(m1_msg, m2_msg, m1_msgdef);
}

static const upb_FieldDef* PyUpb_Message_InitAsMsg(PyUpb_Message* m,
                                                   upb_Arena* arena) {
  const upb_FieldDef* f = PyUpb_Message_GetFieldDef(m);
  const upb_MessageDef* m2 = upb_FieldDef_MessageSubDef(f);
  m->ptr.msg = upb_Message_New(upb_MessageDef_MiniTable(m2), arena);
  m->def = (uintptr_t)m2;
  PyUpb_ObjCache_Add(m->ptr.msg, &m->ob_base);
  return f;
}

static void PyUpb_Message_SetField(PyUpb_Message* parent, const upb_FieldDef* f,
                                   PyUpb_Message* child, upb_Arena* arena) {
  upb_MessageValue msgval = {.msg_val = PyUpb_Message_GetMsg(child)};
  upb_Message_SetFieldByDef(PyUpb_Message_GetMsg(parent), f, msgval, arena);
  PyUpb_WeakMap_Delete(parent->unset_subobj_map, f);
  // Releases a ref previously owned by child->ptr.parent of our child.
  Py_DECREF(child);
}

/*
 * PyUpb_Message_EnsureReified()
 *
 * This implements the "expando" behavior of Python protos:
 *   foo = FooProto()
 *
 *   # The intermediate messages don't really exist, and won't be serialized.
 *   x = foo.bar.bar.bar.bar.bar.baz
 *
 *   # Now all the intermediate objects are created.
 *   foo.bar.bar.bar.bar.bar.baz = 5
 *
 * This function should be called before performing any mutation of a protobuf
 * object.
 *
 * Post-condition:
 *   PyUpb_Message_IsStub(self) is false
 */
void PyUpb_Message_EnsureReified(PyUpb_Message* self) {
  if (!PyUpb_Message_IsStub(self)) return;
  upb_Arena* arena = PyUpb_Arena_Get(self->arena);

  // This is a non-present message. We need to create a real upb_Message for
  // this object and every parent until we reach a present message.
  PyUpb_Message* child = self;
  PyUpb_Message* parent = self->ptr.parent;
  const upb_FieldDef* child_f = PyUpb_Message_InitAsMsg(child, arena);
  Py_INCREF(child);  // To avoid a special-case in PyUpb_Message_SetField().

  do {
    PyUpb_Message* next_parent = parent->ptr.parent;
    const upb_FieldDef* parent_f = NULL;
    if (PyUpb_Message_IsStub(parent)) {
      parent_f = PyUpb_Message_InitAsMsg(parent, arena);
    }
    PyUpb_Message_SetField(parent, child_f, child, arena);
    child = parent;
    child_f = parent_f;
    parent = next_parent;
  } while (child_f);

  // Releases ref previously owned by child->ptr.parent of our child.
  Py_DECREF(child);
  self->version++;
}

static void PyUpb_Message_SyncSubobjs(PyUpb_Message* self);

/*
 * PyUpb_Message_Reify()
 *
 * The message equivalent of PyUpb_*Container_Reify(), this transitions
 * the wrapper from the unset state (owning a reference on self->ptr.parent) to
 * the set state (having a non-owning pointer to self->ptr.msg).
 */
static void PyUpb_Message_Reify(PyUpb_Message* self, const upb_FieldDef* f,
                                upb_Message* msg) {
  assert(f == PyUpb_Message_GetFieldDef(self));
  if (!msg) {
    const upb_MessageDef* msgdef = PyUpb_Message_GetMsgdef((PyObject*)self);
    const upb_MiniTable* layout = upb_MessageDef_MiniTable(msgdef);
    msg = upb_Message_New(layout, PyUpb_Arena_Get(self->arena));
  }
  PyUpb_ObjCache_Add(msg, &self->ob_base);
  Py_DECREF(&self->ptr.parent->ob_base);
  self->ptr.msg = msg;  // Overwrites self->ptr.parent
  self->def = (uintptr_t)upb_FieldDef_MessageSubDef(f);
  PyUpb_Message_SyncSubobjs(self);
}

/*
 * PyUpb_Message_SyncSubobjs()
 *
 * This operation must be invoked whenever the underlying upb_Message has been
 * mutated directly in C.  This will attach any newly-present field data
 * to previously returned stub wrapper objects.
 *
 * For example:
 *   foo = FooMessage()
 *   sub = foo.submsg  # Empty, unset sub-message
 *
 *   # SyncSubobjs() is required to connect our existing 'sub' wrapper to the
 *   # newly created foo.submsg data in C.
 *   foo.MergeFrom(FooMessage(submsg={}))
 *
 * This requires that all of the new sub-objects that have appeared are owned
 * by `self`'s arena.
 */
static void PyUpb_Message_SyncSubobjs(PyUpb_Message* self) {
  PyUpb_WeakMap* subobj_map = self->unset_subobj_map;
  if (!subobj_map) return;

  upb_Message* msg = PyUpb_Message_GetMsg(self);
  intptr_t iter = PYUPB_WEAKMAP_BEGIN;
  const void* key;
  PyObject* obj;

  // The last ref to this message could disappear during iteration.
  // When we call PyUpb_*Container_Reify() below, the container will drop
  // its ref on `self`.  If that was the last ref on self, the object will be
  // deleted, and `subobj_map` along with it.  We need it to live until we are
  // done iterating.
  Py_INCREF(&self->ob_base);

  while (PyUpb_WeakMap_Next(subobj_map, &key, &obj, &iter)) {
    const upb_FieldDef* f = key;
    if (upb_FieldDef_HasPresence(f) && !upb_Message_HasFieldByDef(msg, f))
      continue;
    upb_MessageValue msgval = upb_Message_GetFieldByDef(msg, f);
    PyUpb_WeakMap_DeleteIter(subobj_map, &iter);
    if (upb_FieldDef_IsMap(f)) {
      if (!msgval.map_val) continue;
      PyUpb_MapContainer_Reify(obj, (upb_Map*)msgval.map_val);
    } else if (upb_FieldDef_IsRepeated(f)) {
      if (!msgval.array_val) continue;
      PyUpb_RepeatedContainer_Reify(obj, (upb_Array*)msgval.array_val);
    } else {
      PyUpb_Message* sub = (void*)obj;
      assert(self == sub->ptr.parent);
      PyUpb_Message_Reify(sub, f, (upb_Message*)msgval.msg_val);
    }
  }

  Py_DECREF(&self->ob_base);

  // TODO(haberman): present fields need to be iterated too if they can reach
  // a WeakMap.
}

static PyObject* PyUpb_Message_ToString(PyUpb_Message* self) {
  if (PyUpb_Message_IsStub(self)) {
    return PyUnicode_FromStringAndSize(NULL, 0);
  }
  upb_Message* msg = PyUpb_Message_GetMsg(self);
  const upb_MessageDef* msgdef = _PyUpb_Message_GetMsgdef(self);
  const upb_DefPool* symtab = upb_FileDef_Pool(upb_MessageDef_File(msgdef));
  char buf[1024];
  int options = UPB_TXTENC_SKIPUNKNOWN;
  size_t size = upb_TextEncode(msg, msgdef, symtab, options, buf, sizeof(buf));
  if (size < sizeof(buf)) {
    return PyUnicode_FromStringAndSize(buf, size);
  } else {
    char* buf2 = malloc(size + 1);
    size_t size2 = upb_TextEncode(msg, msgdef, symtab, options, buf2, size + 1);
    assert(size == size2);
    PyObject* ret = PyUnicode_FromStringAndSize(buf2, size2);
    free(buf2);
    return ret;
  }
}

static PyObject* PyUpb_Message_RichCompare(PyObject* _self, PyObject* other,
                                           int opid) {
  PyUpb_Message* self = (void*)_self;
  if (opid != Py_EQ && opid != Py_NE) {
    Py_INCREF(Py_NotImplemented);
    return Py_NotImplemented;
  }
  bool ret = PyUpb_Message_IsEqual(self, other);
  if (opid == Py_NE) ret = !ret;
  return PyBool_FromLong(ret);
}

void PyUpb_Message_CacheDelete(PyObject* _self, const upb_FieldDef* f) {
  PyUpb_Message* self = (void*)_self;
  PyUpb_WeakMap_Delete(self->unset_subobj_map, f);
}

void PyUpb_Message_SetConcreteSubobj(PyObject* _self, const upb_FieldDef* f,
                                     upb_MessageValue subobj) {
  PyUpb_Message* self = (void*)_self;
  PyUpb_Message_EnsureReified(self);
  PyUpb_Message_CacheDelete(_self, f);
  upb_Message_SetFieldByDef(self->ptr.msg, f, subobj,
                            PyUpb_Arena_Get(self->arena));
}

static void PyUpb_Message_Dealloc(PyObject* _self) {
  PyUpb_Message* self = (void*)_self;

  if (PyUpb_Message_IsStub(self)) {
    PyUpb_Message_CacheDelete((PyObject*)self->ptr.parent,
                              PyUpb_Message_GetFieldDef(self));
    Py_DECREF(self->ptr.parent);
  } else {
    PyUpb_ObjCache_Delete(self->ptr.msg);
  }

  if (self->unset_subobj_map) {
    PyUpb_WeakMap_Free(self->unset_subobj_map);
  }

  Py_DECREF(self->arena);

  // We do not use PyUpb_Dealloc() here because Message is a base type and for
  // base types there is a bug we have to work around in this case (see below).
  PyTypeObject* tp = Py_TYPE(self);
  freefunc tp_free = PyType_GetSlot(tp, Py_tp_free);
  tp_free(self);

  if (cpython_bits.python_version_hex >= 0x03080000) {
    // Prior to Python 3.8 there is a bug where deallocating the type here would
    // lead to a double-decref: https://bugs.python.org/issue37879
    Py_DECREF(tp);
  }
}

PyObject* PyUpb_Message_Get(upb_Message* u_msg, const upb_MessageDef* m,
                            PyObject* arena) {
  PyObject* ret = PyUpb_ObjCache_Get(u_msg);
  if (ret) return ret;

  PyObject* cls = PyUpb_Descriptor_GetClass(m);
  // It is not safe to use PyObject_{,GC}_New() due to:
  //    https://bugs.python.org/issue35810
  PyUpb_Message* py_msg = (void*)PyType_GenericAlloc((PyTypeObject*)cls, 0);
  py_msg->arena = arena;
  py_msg->def = (uintptr_t)m;
  py_msg->ptr.msg = u_msg;
  py_msg->unset_subobj_map = NULL;
  py_msg->ext_dict = NULL;
  py_msg->version = 0;
  ret = &py_msg->ob_base;
  Py_DECREF(cls);
  Py_INCREF(arena);
  PyUpb_ObjCache_Add(u_msg, ret);
  return ret;
}

/* PyUpb_Message_GetStub()
 *
 * Non-present messages return "stub" objects that point to their parent, but
 * will materialize into real upb objects if they are mutated.
 *
 * Note: we do *not* create stubs for repeated/map fields unless the parent
 * is a stub:
 *
 *    msg = TestMessage()
 *    msg.submessage                # (A) Creates a stub
 *    msg.repeated_foo              # (B) Does *not* create a stub
 *    msg.submessage.repeated_bar   # (C) Creates a stub
 *
 * In case (B) we have some freedom: we could either create a stub, or create
 * a reified object with underlying data.  It appears that either could work
 * equally well, with no observable change to users.  There isn't a clear
 * advantage to either choice.  We choose to follow the behavior of the
 * pre-existing C++ behavior for consistency, but if it becomes apparent that
 * there would be some benefit to reversing this decision, it should be totally
 * within the realm of possibility.
 */
PyObject* PyUpb_Message_GetStub(PyUpb_Message* self,
                                const upb_FieldDef* field) {
  PyObject* _self = (void*)self;
  if (!self->unset_subobj_map) {
    self->unset_subobj_map = PyUpb_WeakMap_New();
  }
  PyObject* subobj = PyUpb_WeakMap_Get(self->unset_subobj_map, field);

  if (subobj) return subobj;

  if (upb_FieldDef_IsMap(field)) {
    subobj = PyUpb_MapContainer_NewStub(_self, field, self->arena);
  } else if (upb_FieldDef_IsRepeated(field)) {
    subobj = PyUpb_RepeatedContainer_NewStub(_self, field, self->arena);
  } else {
    subobj = PyUpb_Message_NewStub(&self->ob_base, field, self->arena);
  }
  PyUpb_WeakMap_Add(self->unset_subobj_map, field, subobj);

  assert(!PyErr_Occurred());
  return subobj;
}

PyObject* PyUpb_Message_GetPresentWrapper(PyUpb_Message* self,
                                          const upb_FieldDef* field) {
  assert(!PyUpb_Message_IsStub(self));
  upb_MutableMessageValue mutval =
      upb_Message_Mutable(self->ptr.msg, field, PyUpb_Arena_Get(self->arena));
  if (upb_FieldDef_IsMap(field)) {
    return PyUpb_MapContainer_GetOrCreateWrapper(mutval.map, field,
                                                 self->arena);
  } else {
    return PyUpb_RepeatedContainer_GetOrCreateWrapper(mutval.array, field,
                                                      self->arena);
  }
}

PyObject* PyUpb_Message_GetScalarValue(PyUpb_Message* self,
                                       const upb_FieldDef* field) {
  upb_MessageValue val;
  if (PyUpb_Message_IsStub(self)) {
    // Unset message always returns default values.
    val = upb_FieldDef_Default(field);
  } else {
    val = upb_Message_GetFieldByDef(self->ptr.msg, field);
  }
  return PyUpb_UpbToPy(val, field, self->arena);
}

/*
 * PyUpb_Message_GetFieldValue()
 *
 * Implements the equivalent of getattr(msg, field), once `field` has
 * already been resolved to a `upb_FieldDef*`.
 *
 * This may involve constructing a wrapper object for the given field, or
 * returning one that was previously constructed.  If the field is not actually
 * set, the wrapper object will be an "unset" object that is not actually
 * connected to any C data.
 */
PyObject* PyUpb_Message_GetFieldValue(PyObject* _self,
                                      const upb_FieldDef* field) {
  PyUpb_Message* self = (void*)_self;
  assert(upb_FieldDef_ContainingType(field) == PyUpb_Message_GetMsgdef(_self));
  bool submsg = upb_FieldDef_IsSubMessage(field);
  bool seq = upb_FieldDef_IsRepeated(field);

  if ((PyUpb_Message_IsStub(self) && (submsg || seq)) ||
      (submsg && !seq && !upb_Message_HasFieldByDef(self->ptr.msg, field))) {
    return PyUpb_Message_GetStub(self, field);
  } else if (seq) {
    return PyUpb_Message_GetPresentWrapper(self, field);
  } else {
    return PyUpb_Message_GetScalarValue(self, field);
  }
}

int PyUpb_Message_SetFieldValue(PyObject* _self, const upb_FieldDef* field,
                                PyObject* value, PyObject* exc) {
  PyUpb_Message* self = (void*)_self;
  assert(value);

  if (upb_FieldDef_IsSubMessage(field) || upb_FieldDef_IsRepeated(field)) {
    PyErr_Format(exc,
                 "Assignment not allowed to message, map, or repeated "
                 "field \"%s\" in protocol message object.",
                 upb_FieldDef_Name(field));
    return -1;
  }

  PyUpb_Message_EnsureReified(self);

  upb_MessageValue val;
  upb_Arena* arena = PyUpb_Arena_Get(self->arena);
  if (!PyUpb_PyToUpb(value, field, &val, arena)) {
    return -1;
  }

  upb_Message_SetFieldByDef(self->ptr.msg, field, val, arena);
  return 0;
}

int PyUpb_Message_GetVersion(PyObject* _self) {
  PyUpb_Message* self = (void*)_self;
  return self->version;
}

/*
 * PyUpb_Message_GetAttr()
 *
 * Implements:
 *   foo = msg.foo
 *
 * Attribute lookup must find both message fields and base class methods like
 * msg.SerializeToString().
 */
__attribute__((flatten)) static PyObject* PyUpb_Message_GetAttr(
    PyObject* _self, PyObject* attr) {
  PyUpb_Message* self = (void*)_self;

  // Lookup field by name.
  const upb_FieldDef* field;
  if (PyUpb_Message_LookupName(self, attr, &field, NULL, NULL)) {
    return PyUpb_Message_GetFieldValue(_self, field);
  }

  // Check base class attributes.
  assert(!PyErr_Occurred());
  PyObject* ret = PyObject_GenericGetAttr(_self, attr);
  if (ret) return ret;

  // Swallow AttributeError if it occurred and try again on the metaclass
  // to pick up class attributes.  But we have to special-case "Extensions"
  // which affirmatively returns AttributeError when a message is not
  // extendable.
  const char* name;
  if (PyErr_ExceptionMatches(PyExc_AttributeError) &&
      (name = PyUpb_GetStrData(attr)) && strcmp(name, "Extensions") != 0) {
    PyErr_Clear();
    return PyUpb_MessageMeta_GetAttr((PyObject*)Py_TYPE(_self), attr);
  }

  return NULL;
}

/*
 * PyUpb_Message_SetAttr()
 *
 * Implements:
 *   msg.foo = foo
 */
static int PyUpb_Message_SetAttr(PyObject* _self, PyObject* attr,
                                 PyObject* value) {
  PyUpb_Message* self = (void*)_self;
  const upb_FieldDef* field;
  if (!PyUpb_Message_LookupName(self, attr, &field, NULL,
                                PyExc_AttributeError)) {
    return -1;
  }

  return PyUpb_Message_SetFieldValue(_self, field, value, PyExc_AttributeError);
}

static PyObject* PyUpb_Message_HasField(PyObject* _self, PyObject* arg) {
  PyUpb_Message* self = (void*)_self;
  const upb_FieldDef* field;
  const upb_OneofDef* oneof;

  if (!PyUpb_Message_LookupName(self, arg, &field, &oneof, PyExc_ValueError)) {
    return NULL;
  }

  if (field && !upb_FieldDef_HasPresence(field)) {
    PyErr_Format(PyExc_ValueError, "Field %s does not have presence.",
                 upb_FieldDef_FullName(field));
    return NULL;
  }

  if (PyUpb_Message_IsStub(self)) Py_RETURN_FALSE;

  return PyBool_FromLong(field ? upb_Message_HasFieldByDef(self->ptr.msg, field)
                               : upb_Message_WhichOneof(self->ptr.msg, oneof) !=
                                     NULL);
}

static PyObject* PyUpb_Message_FindInitializationErrors(PyObject* _self,
                                                        PyObject* arg);

static PyObject* PyUpb_Message_IsInitializedAppendErrors(PyObject* _self,
                                                         PyObject* errors) {
  PyObject* list = PyUpb_Message_FindInitializationErrors(_self, NULL);
  if (!list) return NULL;
  bool ok = PyList_Size(list) == 0;
  PyObject* ret = NULL;
  PyObject* extend_result = NULL;
  if (!ok) {
    extend_result = PyObject_CallMethod(errors, "extend", "O", list);
    if (!extend_result) goto done;
  }
  ret = PyBool_FromLong(ok);

done:
  Py_XDECREF(list);
  Py_XDECREF(extend_result);
  return ret;
}

static PyObject* PyUpb_Message_IsInitialized(PyObject* _self, PyObject* args) {
  PyObject* errors = NULL;
  if (!PyArg_ParseTuple(args, "|O", &errors)) {
    return NULL;
  }
  if (errors) {
    // We need to collect a list of unset required fields and append it to
    // `errors`.
    return PyUpb_Message_IsInitializedAppendErrors(_self, errors);
  } else {
    // We just need to return a boolean "true" or "false" for whether all
    // required fields are set.
    upb_Message* msg = PyUpb_Message_GetIfReified(_self);
    const upb_MessageDef* m = PyUpb_Message_GetMsgdef(_self);
    const upb_DefPool* symtab = upb_FileDef_Pool(upb_MessageDef_File(m));
    bool initialized = !upb_util_HasUnsetRequired(msg, m, symtab, NULL);
    return PyBool_FromLong(initialized);
  }
}

static PyObject* PyUpb_Message_ListFieldsItemKey(PyObject* self,
                                                 PyObject* val) {
  assert(PyTuple_Check(val));
  PyObject* field = PyTuple_GetItem(val, 0);
  const upb_FieldDef* f = PyUpb_FieldDescriptor_GetDef(field);
  return PyLong_FromLong(upb_FieldDef_Number(f));
}

static PyObject* PyUpb_Message_CheckCalledFromGeneratedFile(
    PyObject* unused, PyObject* unused_arg) {
  PyErr_SetString(
      PyExc_TypeError,
      "Descriptors cannot not be created directly.\n"
      "If this call came from a _pb2.py file, your generated code is out of "
      "date and must be regenerated with protoc >= 3.19.0.\n"
      "If you cannot immediately regenerate your protos, some other possible "
      "workarounds are:\n"
      " 1. Downgrade the protobuf package to 3.20.x or lower.\n"
      " 2. Set PROTOCOL_BUFFERS_PYTHON_IMPLEMENTATION=python (but this will "
      "use pure-Python parsing and will be much slower).\n"
      "\n"
      "More information: "
      "https://developers.google.com/protocol-buffers/docs/news/"
      "2022-05-06#python-updates");
  return NULL;
}

static bool PyUpb_Message_SortFieldList(PyObject* list) {
  PyUpb_ModuleState* state = PyUpb_ModuleState_Get();
  bool ok = false;
  PyObject* args = PyTuple_New(0);
  PyObject* kwargs = PyDict_New();
  PyObject* method = PyObject_GetAttrString(list, "sort");
  PyObject* call_result = NULL;
  if (!args || !kwargs || !method) goto err;
  if (PyDict_SetItemString(kwargs, "key", state->listfields_item_key) < 0) {
    goto err;
  }
  call_result = PyObject_Call(method, args, kwargs);
  if (!call_result) goto err;
  ok = true;

err:
  Py_XDECREF(method);
  Py_XDECREF(args);
  Py_XDECREF(kwargs);
  Py_XDECREF(call_result);
  return ok;
}

static PyObject* PyUpb_Message_ListFields(PyObject* _self, PyObject* arg) {
  PyObject* list = PyList_New(0);
  upb_Message* msg = PyUpb_Message_GetIfReified(_self);
  if (!msg) return list;

  size_t iter1 = kUpb_Message_Begin;
  const upb_MessageDef* m = PyUpb_Message_GetMsgdef(_self);
  const upb_DefPool* symtab = upb_FileDef_Pool(upb_MessageDef_File(m));
  const upb_FieldDef* f;
  PyObject* field_desc = NULL;
  PyObject* py_val = NULL;
  PyObject* tuple = NULL;
  upb_MessageValue val;
  uint32_t last_field = 0;
  bool in_order = true;
  while (upb_Message_Next(msg, m, symtab, &f, &val, &iter1)) {
    const uint32_t field_number = upb_FieldDef_Number(f);
    if (field_number < last_field) in_order = false;
    last_field = field_number;
    PyObject* field_desc = PyUpb_FieldDescriptor_Get(f);
    PyObject* py_val = PyUpb_Message_GetFieldValue(_self, f);
    if (!field_desc || !py_val) goto err;
    PyObject* tuple = Py_BuildValue("(NN)", field_desc, py_val);
    field_desc = NULL;
    py_val = NULL;
    if (!tuple) goto err;
    if (PyList_Append(list, tuple)) goto err;
    Py_DECREF(tuple);
    tuple = NULL;
  }

  // Users rely on fields being returned in field number order.
  if (!in_order && !PyUpb_Message_SortFieldList(list)) goto err;

  return list;

err:
  Py_XDECREF(field_desc);
  Py_XDECREF(py_val);
  Py_XDECREF(tuple);
  Py_DECREF(list);
  return NULL;
}

static PyObject* PyUpb_Message_MergeInternal(PyObject* self, PyObject* arg,
                                             bool check_required) {
  if (self->ob_type != arg->ob_type) {
    PyErr_Format(PyExc_TypeError,
                 "Parameter to MergeFrom() must be instance of same class: "
                 "expected %S got %S.",
                 Py_TYPE(self), Py_TYPE(arg));
    return NULL;
  }
  // OPT: exit if src is empty.
  PyObject* subargs = PyTuple_New(0);
  PyObject* serialized =
      check_required
          ? PyUpb_Message_SerializeToString(arg, subargs, NULL)
          : PyUpb_Message_SerializePartialToString(arg, subargs, NULL);
  Py_DECREF(subargs);
  if (!serialized) return NULL;
  PyObject* ret = PyUpb_Message_MergeFromString(self, serialized);
  Py_DECREF(serialized);
  Py_DECREF(ret);
  Py_RETURN_NONE;
}

PyObject* PyUpb_Message_MergeFrom(PyObject* self, PyObject* arg) {
  return PyUpb_Message_MergeInternal(self, arg, true);
}

static PyObject* PyUpb_Message_MergePartialFrom(PyObject* self, PyObject* arg) {
  return PyUpb_Message_MergeInternal(self, arg, false);
}

static PyObject* PyUpb_Message_SetInParent(PyObject* _self, PyObject* arg) {
  PyUpb_Message* self = (void*)_self;
  PyUpb_Message_EnsureReified(self);
  Py_RETURN_NONE;
}

static PyObject* PyUpb_Message_UnknownFields(PyObject* _self, PyObject* arg) {
  // TODO(haberman): re-enable when unknown fields are added.
  // return PyUpb_UnknownFields_New(_self);
  PyErr_SetString(PyExc_NotImplementedError, "unknown field accessor");
  return NULL;
}

PyObject* PyUpb_Message_MergeFromString(PyObject* _self, PyObject* arg) {
  PyUpb_Message* self = (void*)_self;
  char* buf;
  Py_ssize_t size;
  PyObject* bytes = NULL;

  if (PyMemoryView_Check(arg)) {
    bytes = PyBytes_FromObject(arg);
    // Cannot fail when passed something of the correct type.
    int err = PyBytes_AsStringAndSize(bytes, &buf, &size);
    (void)err;
    assert(err >= 0);
  } else if (PyBytes_AsStringAndSize(arg, &buf, &size) < 0) {
    return NULL;
  }

  PyUpb_Message_EnsureReified(self);
  const upb_MessageDef* msgdef = _PyUpb_Message_GetMsgdef(self);
  const upb_FileDef* file = upb_MessageDef_File(msgdef);
  const upb_ExtensionRegistry* extreg =
      upb_DefPool_ExtensionRegistry(upb_FileDef_Pool(file));
  const upb_MiniTable* layout = upb_MessageDef_MiniTable(msgdef);
  upb_Arena* arena = PyUpb_Arena_Get(self->arena);
  PyUpb_ModuleState* state = PyUpb_ModuleState_Get();
  int options = upb_DecodeOptions_MaxDepth(
      state->allow_oversize_protos ? UINT16_MAX
                                   : kUpb_WireFormat_DefaultDepthLimit);
  upb_DecodeStatus status =
      upb_Decode(buf, size, self->ptr.msg, layout, extreg, options, arena);
  Py_XDECREF(bytes);
  if (status != kUpb_DecodeStatus_Ok) {
    PyErr_Format(state->decode_error_class, "Error parsing message");
    return NULL;
  }
  PyUpb_Message_SyncSubobjs(self);
  return PyLong_FromSsize_t(size);
}

static PyObject* PyUpb_Message_Clear(PyUpb_Message* self, PyObject* args);

static PyObject* PyUpb_Message_ParseFromString(PyObject* self, PyObject* arg) {
  PyObject* tmp = PyUpb_Message_Clear((PyUpb_Message*)self, NULL);
  Py_DECREF(tmp);
  return PyUpb_Message_MergeFromString(self, arg);
}

static PyObject* PyUpb_Message_ByteSize(PyObject* self, PyObject* args) {
  // TODO(https://github.com/protocolbuffers/upb/issues/462): At the moment upb
  // does not have a "byte size" function, so we just serialize to string and
  // get the size of the string.
  PyObject* subargs = PyTuple_New(0);
  PyObject* serialized = PyUpb_Message_SerializeToString(self, subargs, NULL);
  Py_DECREF(subargs);
  if (!serialized) return NULL;
  size_t size = PyBytes_Size(serialized);
  Py_DECREF(serialized);
  return PyLong_FromSize_t(size);
}

static PyObject* PyUpb_Message_Clear(PyUpb_Message* self, PyObject* args) {
  PyUpb_Message_EnsureReified(self);
  const upb_MessageDef* msgdef = _PyUpb_Message_GetMsgdef(self);
  PyUpb_WeakMap* subobj_map = self->unset_subobj_map;

  if (subobj_map) {
    upb_Message* msg = PyUpb_Message_GetMsg(self);
    (void)msg;  // Suppress unused warning when asserts are disabled.
    intptr_t iter = PYUPB_WEAKMAP_BEGIN;
    const void* key;
    PyObject* obj;

    while (PyUpb_WeakMap_Next(subobj_map, &key, &obj, &iter)) {
      const upb_FieldDef* f = key;
      PyUpb_WeakMap_DeleteIter(subobj_map, &iter);
      if (upb_FieldDef_IsMap(f)) {
        assert(upb_Message_GetFieldByDef(msg, f).map_val == NULL);
        PyUpb_MapContainer_Reify(obj, NULL);
      } else if (upb_FieldDef_IsRepeated(f)) {
        assert(upb_Message_GetFieldByDef(msg, f).array_val == NULL);
        PyUpb_RepeatedContainer_Reify(obj, NULL);
      } else {
        assert(!upb_Message_HasFieldByDef(msg, f));
        PyUpb_Message* sub = (void*)obj;
        assert(self == sub->ptr.parent);
        PyUpb_Message_Reify(sub, f, NULL);
      }
    }
  }

  upb_Message_ClearByDef(self->ptr.msg, msgdef);
  Py_RETURN_NONE;
}

void PyUpb_Message_DoClearField(PyObject* _self, const upb_FieldDef* f) {
  PyUpb_Message* self = (void*)_self;
  PyUpb_Message_EnsureReified((PyUpb_Message*)self);

  // We must ensure that any stub object is reified so its parent no longer
  // points to us.
  PyObject* sub = self->unset_subobj_map
                      ? PyUpb_WeakMap_Get(self->unset_subobj_map, f)
                      : NULL;

  if (upb_FieldDef_IsMap(f)) {
    // For maps we additionally have to invalidate any iterators.  So we need
    // to get an object even if it's reified.
    if (!sub) {
      sub = PyUpb_Message_GetFieldValue(_self, f);
    }
    PyUpb_MapContainer_EnsureReified(sub);
    PyUpb_MapContainer_Invalidate(sub);
  } else if (upb_FieldDef_IsRepeated(f)) {
    if (sub) {
      PyUpb_RepeatedContainer_EnsureReified(sub);
    }
  } else if (upb_FieldDef_IsSubMessage(f)) {
    if (sub) {
      PyUpb_Message_EnsureReified((PyUpb_Message*)sub);
    }
  }

  Py_XDECREF(sub);
  upb_Message_ClearFieldByDef(self->ptr.msg, f);
}

static PyObject* PyUpb_Message_ClearExtension(PyObject* _self, PyObject* arg) {
  PyUpb_Message* self = (void*)_self;
  PyUpb_Message_EnsureReified(self);
  const upb_FieldDef* f = PyUpb_Message_GetExtensionDef(_self, arg);
  if (!f) return NULL;
  PyUpb_Message_DoClearField(_self, f);
  Py_RETURN_NONE;
}

static PyObject* PyUpb_Message_ClearField(PyObject* _self, PyObject* arg) {
  PyUpb_Message* self = (void*)_self;

  // We always need EnsureReified() here (even for an unset message) to
  // preserve behavior like:
  //   msg = FooMessage()
  //   msg.foo.Clear()
  //   assert msg.HasField("foo")
  PyUpb_Message_EnsureReified(self);

  const upb_FieldDef* f;
  const upb_OneofDef* o;
  if (!PyUpb_Message_LookupName(self, arg, &f, &o, PyExc_ValueError)) {
    return NULL;
  }

  if (o) f = upb_Message_WhichOneof(self->ptr.msg, o);
  if (f) PyUpb_Message_DoClearField(_self, f);
  Py_RETURN_NONE;
}

static PyObject* PyUpb_Message_DiscardUnknownFields(PyUpb_Message* self,
                                                    PyObject* arg) {
  PyUpb_Message_EnsureReified(self);
  const upb_MessageDef* msgdef = _PyUpb_Message_GetMsgdef(self);
  upb_Message_DiscardUnknown(self->ptr.msg, msgdef, 64);
  Py_RETURN_NONE;
}

static PyObject* PyUpb_Message_FindInitializationErrors(PyObject* _self,
                                                        PyObject* arg) {
  PyUpb_Message* self = (void*)_self;
  upb_Message* msg = PyUpb_Message_GetIfReified(_self);
  const upb_MessageDef* msgdef = _PyUpb_Message_GetMsgdef(self);
  const upb_DefPool* ext_pool = upb_FileDef_Pool(upb_MessageDef_File(msgdef));
  upb_FieldPathEntry* fields;
  PyObject* ret = PyList_New(0);
  if (upb_util_HasUnsetRequired(msg, msgdef, ext_pool, &fields)) {
    char* buf = NULL;
    size_t size = 0;
    assert(fields->field);
    while (fields->field) {
      upb_FieldPathEntry* field = fields;
      size_t need = upb_FieldPath_ToText(&fields, buf, size);
      if (need >= size) {
        fields = field;
        size = size ? size * 2 : 16;
        while (size <= need) size *= 2;
        buf = realloc(buf, size);
        need = upb_FieldPath_ToText(&fields, buf, size);
        assert(size > need);
      }
      PyObject* str = PyUnicode_FromString(buf);
      PyList_Append(ret, str);
      Py_DECREF(str);
    }
    free(buf);
  }
  return ret;
}

static PyObject* PyUpb_Message_FromString(PyObject* cls, PyObject* serialized) {
  PyObject* ret = NULL;
  PyObject* length = NULL;

  ret = PyObject_CallObject(cls, NULL);
  if (ret == NULL) goto err;
  length = PyUpb_Message_MergeFromString(ret, serialized);
  if (length == NULL) goto err;

done:
  Py_XDECREF(length);
  return ret;

err:
  Py_XDECREF(ret);
  ret = NULL;
  goto done;
}

const upb_FieldDef* PyUpb_Message_GetExtensionDef(PyObject* _self,
                                                  PyObject* key) {
  const upb_FieldDef* f = PyUpb_FieldDescriptor_GetDef(key);
  if (!f) {
    PyErr_Clear();
    PyErr_Format(PyExc_KeyError, "Object %R is not a field descriptor\n", key);
    return NULL;
  }
  if (!upb_FieldDef_IsExtension(f)) {
    PyErr_Format(PyExc_KeyError, "Field %s is not an extension\n",
                 upb_FieldDef_FullName(f));
    return NULL;
  }
  const upb_MessageDef* msgdef = PyUpb_Message_GetMsgdef(_self);
  if (upb_FieldDef_ContainingType(f) != msgdef) {
    PyErr_Format(PyExc_KeyError, "Extension doesn't match (%s vs %s)",
                 upb_MessageDef_FullName(msgdef), upb_FieldDef_FullName(f));
    return NULL;
  }
  return f;
}

static PyObject* PyUpb_Message_HasExtension(PyObject* _self,
                                            PyObject* ext_desc) {
  upb_Message* msg = PyUpb_Message_GetIfReified(_self);
  const upb_FieldDef* f = PyUpb_Message_GetExtensionDef(_self, ext_desc);
  if (!f) return NULL;
  if (upb_FieldDef_IsRepeated(f)) {
    PyErr_SetString(PyExc_KeyError,
                    "Field is repeated. A singular method is required.");
    return NULL;
  }
  if (!msg) Py_RETURN_FALSE;
  return PyBool_FromLong(upb_Message_HasFieldByDef(msg, f));
}

void PyUpb_Message_ReportInitializationErrors(const upb_MessageDef* msgdef,
                                              PyObject* errors, PyObject* exc) {
  PyObject* comma = PyUnicode_FromString(",");
  PyObject* missing_fields = NULL;
  if (!comma) goto done;
  missing_fields = PyUnicode_Join(comma, errors);
  if (!missing_fields) goto done;
  PyErr_Format(exc, "Message %s is missing required fields: %U",
               upb_MessageDef_FullName(msgdef), missing_fields);
done:
  Py_XDECREF(comma);
  Py_XDECREF(missing_fields);
  Py_DECREF(errors);
}

PyObject* PyUpb_Message_SerializeInternal(PyObject* _self, PyObject* args,
                                          PyObject* kwargs,
                                          bool check_required) {
  PyUpb_Message* self = (void*)_self;
  if (!PyUpb_Message_Verify((PyObject*)self)) return NULL;
  static const char* kwlist[] = {"deterministic", NULL};
  int deterministic = 0;
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|p", (char**)(kwlist),
                                   &deterministic)) {
    return NULL;
  }

  const upb_MessageDef* msgdef = _PyUpb_Message_GetMsgdef(self);
  if (PyUpb_Message_IsStub(self)) {
    // Nothing to serialize, but we do have to check whether the message is
    // initialized.
    PyUpb_ModuleState* state = PyUpb_ModuleState_Get();
    PyObject* errors = PyUpb_Message_FindInitializationErrors(_self, NULL);
    if (!errors) return NULL;
    if (PyList_Size(errors) == 0) {
      Py_DECREF(errors);
      return PyBytes_FromStringAndSize(NULL, 0);
    }
    PyUpb_Message_ReportInitializationErrors(msgdef, errors,
                                             state->encode_error_class);
    return NULL;
  }

  upb_Arena* arena = upb_Arena_New();
  const upb_MiniTable* layout = upb_MessageDef_MiniTable(msgdef);
  size_t size = 0;
  // Python does not currently have any effective limit on serialization depth.
  int options = upb_EncodeOptions_MaxDepth(UINT16_MAX);
  if (check_required) options |= kUpb_EncodeOption_CheckRequired;
  if (deterministic) options |= kUpb_EncodeOption_Deterministic;
  char* pb;
  upb_EncodeStatus status =
      upb_Encode(self->ptr.msg, layout, options, arena, &pb, &size);
  PyObject* ret = NULL;

  if (status != kUpb_EncodeStatus_Ok) {
    PyUpb_ModuleState* state = PyUpb_ModuleState_Get();
    PyObject* errors = PyUpb_Message_FindInitializationErrors(_self, NULL);
    if (PyList_Size(errors) != 0) {
      PyUpb_Message_ReportInitializationErrors(msgdef, errors,
                                               state->encode_error_class);
    } else {
      PyErr_Format(state->encode_error_class, "Failed to serialize proto");
    }
    goto done;
  }

  ret = PyBytes_FromStringAndSize(pb, size);

done:
  upb_Arena_Free(arena);
  return ret;
}

PyObject* PyUpb_Message_SerializeToString(PyObject* _self, PyObject* args,
                                          PyObject* kwargs) {
  return PyUpb_Message_SerializeInternal(_self, args, kwargs, true);
}

PyObject* PyUpb_Message_SerializePartialToString(PyObject* _self,
                                                 PyObject* args,
                                                 PyObject* kwargs) {
  return PyUpb_Message_SerializeInternal(_self, args, kwargs, false);
}

static PyObject* PyUpb_Message_WhichOneof(PyObject* _self, PyObject* name) {
  PyUpb_Message* self = (void*)_self;
  const upb_OneofDef* o;
  if (!PyUpb_Message_LookupName(self, name, NULL, &o, PyExc_ValueError)) {
    return NULL;
  }
  upb_Message* msg = PyUpb_Message_GetIfReified(_self);
  if (!msg) Py_RETURN_NONE;
  const upb_FieldDef* f = upb_Message_WhichOneof(msg, o);
  if (!f) Py_RETURN_NONE;
  return PyUnicode_FromString(upb_FieldDef_Name(f));
}

void PyUpb_Message_ClearExtensionDict(PyObject* _self) {
  PyUpb_Message* self = (void*)_self;
  assert(self->ext_dict);
  self->ext_dict = NULL;
}

static PyObject* PyUpb_Message_GetExtensionDict(PyObject* _self,
                                                void* closure) {
  PyUpb_Message* self = (void*)_self;
  if (self->ext_dict) {
    Py_INCREF(self->ext_dict);
    return self->ext_dict;
  }

  const upb_MessageDef* m = _PyUpb_Message_GetMsgdef(self);
  if (upb_MessageDef_ExtensionRangeCount(m) == 0) {
    PyErr_SetNone(PyExc_AttributeError);
    return NULL;
  }

  self->ext_dict = PyUpb_ExtensionDict_New(_self);
  return self->ext_dict;
}

static PyGetSetDef PyUpb_Message_Getters[] = {
    {"Extensions", PyUpb_Message_GetExtensionDict, NULL, "Extension dict"},
    {NULL}};

static PyMethodDef PyUpb_Message_Methods[] = {
    // TODO(https://github.com/protocolbuffers/upb/issues/459)
    //{ "__deepcopy__", (PyCFunction)DeepCopy, METH_VARARGS,
    //  "Makes a deep copy of the class." },
    //{ "__unicode__", (PyCFunction)ToUnicode, METH_NOARGS,
    //  "Outputs a unicode representation of the message." },
    {"ByteSize", (PyCFunction)PyUpb_Message_ByteSize, METH_NOARGS,
     "Returns the size of the message in bytes."},
    {"Clear", (PyCFunction)PyUpb_Message_Clear, METH_NOARGS,
     "Clears the message."},
    {"ClearExtension", PyUpb_Message_ClearExtension, METH_O,
     "Clears a message field."},
    {"ClearField", PyUpb_Message_ClearField, METH_O, "Clears a message field."},
    // TODO(https://github.com/protocolbuffers/upb/issues/459)
    //{ "CopyFrom", (PyCFunction)CopyFrom, METH_O,
    //  "Copies a protocol message into the current message." },
    {"DiscardUnknownFields", (PyCFunction)PyUpb_Message_DiscardUnknownFields,
     METH_NOARGS, "Discards the unknown fields."},
    {"FindInitializationErrors", PyUpb_Message_FindInitializationErrors,
     METH_NOARGS, "Finds unset required fields."},
    {"FromString", PyUpb_Message_FromString, METH_O | METH_CLASS,
     "Creates new method instance from given serialized data."},
    {"HasExtension", PyUpb_Message_HasExtension, METH_O,
     "Checks if a message field is set."},
    {"HasField", PyUpb_Message_HasField, METH_O,
     "Checks if a message field is set."},
    {"IsInitialized", PyUpb_Message_IsInitialized, METH_VARARGS,
     "Checks if all required fields of a protocol message are set."},
    {"ListFields", PyUpb_Message_ListFields, METH_NOARGS,
     "Lists all set fields of a message."},
    {"MergeFrom", PyUpb_Message_MergeFrom, METH_O,
     "Merges a protocol message into the current message."},
    {"MergeFromString", PyUpb_Message_MergeFromString, METH_O,
     "Merges a serialized message into the current message."},
    {"ParseFromString", PyUpb_Message_ParseFromString, METH_O,
     "Parses a serialized message into the current message."},
    // TODO(https://github.com/protocolbuffers/upb/issues/459)
    //{ "RegisterExtension", (PyCFunction)RegisterExtension, METH_O |
    // METH_CLASS,
    //  "Registers an extension with the current message." },
    {"SerializePartialToString",
     (PyCFunction)PyUpb_Message_SerializePartialToString,
     METH_VARARGS | METH_KEYWORDS,
     "Serializes the message to a string, even if it isn't initialized."},
    {"SerializeToString", (PyCFunction)PyUpb_Message_SerializeToString,
     METH_VARARGS | METH_KEYWORDS,
     "Serializes the message to a string, only for initialized messages."},
    {"SetInParent", (PyCFunction)PyUpb_Message_SetInParent, METH_NOARGS,
     "Sets the has bit of the given field in its parent message."},
    {"UnknownFields", (PyCFunction)PyUpb_Message_UnknownFields, METH_NOARGS,
     "Parse unknown field set"},
    {"WhichOneof", PyUpb_Message_WhichOneof, METH_O,
     "Returns the name of the field set inside a oneof, "
     "or None if no field is set."},
    {"_ListFieldsItemKey", PyUpb_Message_ListFieldsItemKey,
     METH_O | METH_STATIC,
     "Compares ListFields() list entries by field number"},
    {"_CheckCalledFromGeneratedFile",
     PyUpb_Message_CheckCalledFromGeneratedFile, METH_NOARGS | METH_STATIC,
     "Raises TypeError if the caller is not in a _pb2.py file."},
    {NULL, NULL}};

static PyType_Slot PyUpb_Message_Slots[] = {
    {Py_tp_dealloc, PyUpb_Message_Dealloc},
    {Py_tp_doc, "A ProtocolMessage"},
    {Py_tp_getattro, PyUpb_Message_GetAttr},
    {Py_tp_getset, PyUpb_Message_Getters},
    {Py_tp_hash, PyObject_HashNotImplemented},
    {Py_tp_methods, PyUpb_Message_Methods},
    {Py_tp_new, PyUpb_Message_New},
    {Py_tp_str, PyUpb_Message_ToString},
    {Py_tp_repr, PyUpb_Message_ToString},
    {Py_tp_richcompare, PyUpb_Message_RichCompare},
    {Py_tp_setattro, PyUpb_Message_SetAttr},
    {Py_tp_init, PyUpb_Message_Init},
    {0, NULL}};

PyType_Spec PyUpb_Message_Spec = {
    PYUPB_MODULE_NAME ".Message",              // tp_name
    sizeof(PyUpb_Message),                     // tp_basicsize
    0,                                         // tp_itemsize
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,  // tp_flags
    PyUpb_Message_Slots,
};

// -----------------------------------------------------------------------------
// MessageMeta
// -----------------------------------------------------------------------------

// MessageMeta is the metaclass for message objects.  The generated code uses it
// to construct message classes, ie.
//
// FooMessage = _message.MessageMeta('FooMessage', (_message.Message), {...})
//
// (This is not quite true: at the moment the Python library subclasses
// MessageMeta, and uses that subclass as the metaclass.  There is a TODO below
// to simplify this, so that the illustration above is indeed accurate).

typedef struct {
  const upb_MiniTable* layout;
  PyObject* py_message_descriptor;
} PyUpb_MessageMeta;

// The PyUpb_MessageMeta struct is trailing data tacked onto the end of
// MessageMeta instances.  This means that we get our instances of this struct
// by adding the appropriate number of bytes.
static PyUpb_MessageMeta* PyUpb_GetMessageMeta(PyObject* cls) {
#ifndef NDEBUG
  PyUpb_ModuleState* state = PyUpb_ModuleState_MaybeGet();
  assert(!state || cls->ob_type == state->message_meta_type);
#endif
  return (PyUpb_MessageMeta*)((char*)cls + cpython_bits.type_basicsize);
}

static const upb_MessageDef* PyUpb_MessageMeta_GetMsgdef(PyObject* cls) {
  PyUpb_MessageMeta* self = PyUpb_GetMessageMeta(cls);
  return PyUpb_Descriptor_GetDef(self->py_message_descriptor);
}

PyObject* PyUpb_MessageMeta_DoCreateClass(PyObject* py_descriptor,
                                          const char* name, PyObject* dict) {
  PyUpb_ModuleState* state = PyUpb_ModuleState_Get();
  PyTypeObject* descriptor_type = state->descriptor_types[kPyUpb_Descriptor];
  if (!PyObject_TypeCheck(py_descriptor, descriptor_type)) {
    return PyErr_Format(PyExc_TypeError, "Expected a message Descriptor");
  }

  const upb_MessageDef* msgdef = PyUpb_Descriptor_GetDef(py_descriptor);
  assert(msgdef);
  assert(!PyUpb_ObjCache_Get(upb_MessageDef_MiniTable(msgdef)));

  PyObject* slots = PyTuple_New(0);
  if (!slots) return NULL;
  int status = PyDict_SetItemString(dict, "__slots__", slots);
  Py_DECREF(slots);
  if (status < 0) return NULL;

  // Bases are either:
  //    (Message, Message)            # for regular messages
  //    (Message, Message, WktBase)   # For well-known types
  PyObject* wkt_bases = PyUpb_GetWktBases(state);
  PyObject* wkt_base =
      PyDict_GetItemString(wkt_bases, upb_MessageDef_FullName(msgdef));
  PyObject* args;
  if (wkt_base == NULL) {
    args = Py_BuildValue("s(OO)O", name, state->cmessage_type,
                         state->message_class, dict);
  } else {
    args = Py_BuildValue("s(OOO)O", name, state->cmessage_type,
                         state->message_class, wkt_base, dict);
  }

  PyObject* ret = cpython_bits.type_new(state->message_meta_type, args, NULL);
  Py_DECREF(args);
  if (!ret) return NULL;

  PyUpb_MessageMeta* meta = PyUpb_GetMessageMeta(ret);
  meta->py_message_descriptor = py_descriptor;
  meta->layout = upb_MessageDef_MiniTable(msgdef);
  Py_INCREF(meta->py_message_descriptor);

  PyUpb_ObjCache_Add(meta->layout, ret);

  return ret;
}

static PyObject* PyUpb_MessageMeta_New(PyTypeObject* type, PyObject* args,
                                       PyObject* kwargs) {
  PyUpb_ModuleState* state = PyUpb_ModuleState_Get();
  static const char* kwlist[] = {"name", "bases", "dict", 0};
  PyObject *bases, *dict;
  const char* name;

  // Check arguments: (name, bases, dict)
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "sO!O!:type", (char**)kwlist,
                                   &name, &PyTuple_Type, &bases, &PyDict_Type,
                                   &dict)) {
    return NULL;
  }

  // Check bases: only (), or (message.Message,) are allowed
  Py_ssize_t size = PyTuple_Size(bases);
  if (!(size == 0 ||
        (size == 1 && PyTuple_GetItem(bases, 0) == state->message_class))) {
    PyErr_Format(PyExc_TypeError,
                 "A Message class can only inherit from Message, not %S",
                 bases);
    return NULL;
  }

  // Check dict['DESCRIPTOR']
  PyObject* py_descriptor = PyDict_GetItemString(dict, "DESCRIPTOR");
  if (py_descriptor == NULL) {
    PyErr_SetString(PyExc_TypeError, "Message class has no DESCRIPTOR");
    return NULL;
  }

  const upb_MessageDef* m = PyUpb_Descriptor_GetDef(py_descriptor);
  PyObject* ret = PyUpb_ObjCache_Get(upb_MessageDef_MiniTable(m));
  if (ret) return ret;
  return PyUpb_MessageMeta_DoCreateClass(py_descriptor, name, dict);
}

static void PyUpb_MessageMeta_Dealloc(PyObject* self) {
  PyUpb_MessageMeta* meta = PyUpb_GetMessageMeta(self);
  PyUpb_ObjCache_Delete(meta->layout);
  Py_DECREF(meta->py_message_descriptor);
  PyTypeObject* tp = Py_TYPE(self);
  cpython_bits.type_dealloc(self);
  Py_DECREF(tp);
}

void PyUpb_MessageMeta_AddFieldNumber(PyObject* self, const upb_FieldDef* f) {
  PyObject* name =
      PyUnicode_FromFormat("%s_FIELD_NUMBER", upb_FieldDef_Name(f));
  PyObject* upper = PyObject_CallMethod(name, "upper", "");
  PyObject_SetAttr(self, upper, PyLong_FromLong(upb_FieldDef_Number(f)));
  Py_DECREF(name);
  Py_DECREF(upper);
}

static PyObject* PyUpb_MessageMeta_GetDynamicAttr(PyObject* self,
                                                  PyObject* name) {
  const char* name_buf = PyUpb_GetStrData(name);
  if (!name_buf) return NULL;
  const upb_MessageDef* msgdef = PyUpb_MessageMeta_GetMsgdef(self);
  const upb_FileDef* filedef = upb_MessageDef_File(msgdef);
  const upb_DefPool* symtab = upb_FileDef_Pool(filedef);

  PyObject* py_key =
      PyBytes_FromFormat("%s.%s", upb_MessageDef_FullName(msgdef), name_buf);
  const char* key = PyUpb_GetStrData(py_key);
  PyObject* ret = NULL;
  const upb_MessageDef* nested = upb_DefPool_FindMessageByName(symtab, key);
  const upb_EnumDef* enumdef;
  const upb_EnumValueDef* enumval;
  const upb_FieldDef* ext;

  if (nested) {
    ret = PyUpb_Descriptor_GetClass(nested);
  } else if ((enumdef = upb_DefPool_FindEnumByName(symtab, key))) {
    PyUpb_ModuleState* state = PyUpb_ModuleState_Get();
    PyObject* klass = state->enum_type_wrapper_class;
    ret = PyUpb_EnumDescriptor_Get(enumdef);
    ret = PyObject_CallFunctionObjArgs(klass, ret, NULL);
  } else if ((enumval = upb_DefPool_FindEnumByNameval(symtab, key))) {
    ret = PyLong_FromLong(upb_EnumValueDef_Number(enumval));
  } else if ((ext = upb_DefPool_FindExtensionByName(symtab, key))) {
    ret = PyUpb_FieldDescriptor_Get(ext);
  }

  Py_DECREF(py_key);

  const char* suffix = "_FIELD_NUMBER";
  size_t n = strlen(name_buf);
  size_t suffix_n = strlen(suffix);
  if (n > suffix_n && memcmp(suffix, name_buf + n - suffix_n, suffix_n) == 0) {
    // We can't look up field names dynamically, because the <NAME>_FIELD_NUMBER
    // naming scheme upper-cases the field name and is therefore non-reversible.
    // So we just add all field numbers.
    int n = upb_MessageDef_FieldCount(msgdef);
    for (int i = 0; i < n; i++) {
      PyUpb_MessageMeta_AddFieldNumber(self, upb_MessageDef_Field(msgdef, i));
    }
    n = upb_MessageDef_NestedExtensionCount(msgdef);
    for (int i = 0; i < n; i++) {
      PyUpb_MessageMeta_AddFieldNumber(
          self, upb_MessageDef_NestedExtension(msgdef, i));
    }
    ret = PyObject_GenericGetAttr(self, name);
  }

  return ret;
}

static PyObject* PyUpb_MessageMeta_GetAttr(PyObject* self, PyObject* name) {
  // We want to first delegate to the type's tp_dict to retrieve any attributes
  // that were previously calculated and cached in the type's dict.
  PyObject* ret = cpython_bits.type_getattro(self, name);
  if (ret) return ret;

  // We did not find a cached attribute. Try to calculate the attribute
  // dynamically, using the descriptor as an argument.
  PyErr_Clear();
  ret = PyUpb_MessageMeta_GetDynamicAttr(self, name);

  if (ret) {
    PyObject_SetAttr(self, name, ret);
    PyErr_Clear();
    return ret;
  }

  PyErr_SetObject(PyExc_AttributeError, name);
  return NULL;
}

static PyType_Slot PyUpb_MessageMeta_Slots[] = {
    {Py_tp_new, PyUpb_MessageMeta_New},
    {Py_tp_dealloc, PyUpb_MessageMeta_Dealloc},
    {Py_tp_getattro, PyUpb_MessageMeta_GetAttr},
    {0, NULL}};

static PyType_Spec PyUpb_MessageMeta_Spec = {
    PYUPB_MODULE_NAME ".MessageMeta",  // tp_name
    0,  // To be filled in by size of base     // tp_basicsize
    0,  // tp_itemsize
    // TODO(haberman): remove BASETYPE, Python should just use MessageMeta
    // directly instead of subclassing it.
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,  // tp_flags
    PyUpb_MessageMeta_Slots,
};

static PyObject* PyUpb_MessageMeta_CreateType(void) {
  PyObject* bases = Py_BuildValue("(O)", &PyType_Type);
  if (!bases) return NULL;
  PyUpb_MessageMeta_Spec.basicsize =
      cpython_bits.type_basicsize + sizeof(PyUpb_MessageMeta);
  PyObject* type = PyType_FromSpecWithBases(&PyUpb_MessageMeta_Spec, bases);
  Py_DECREF(bases);
  return type;
}

bool PyUpb_InitMessage(PyObject* m) {
  if (!PyUpb_CPythonBits_Init(&cpython_bits)) return false;
  PyObject* message_meta_type = PyUpb_MessageMeta_CreateType();

  PyUpb_ModuleState* state = PyUpb_ModuleState_GetFromModule(m);
  state->cmessage_type = PyUpb_AddClass(m, &PyUpb_Message_Spec);
  state->message_meta_type = (PyTypeObject*)message_meta_type;

  if (!state->cmessage_type || !state->message_meta_type) return false;
  if (PyModule_AddObject(m, "MessageMeta", message_meta_type)) return false;
  state->listfields_item_key = PyObject_GetAttrString(
      (PyObject*)state->cmessage_type, "_ListFieldsItemKey");

  PyObject* mod =
      PyImport_ImportModule(PYUPB_PROTOBUF_PUBLIC_PACKAGE ".message");
  if (mod == NULL) return false;

  state->encode_error_class = PyObject_GetAttrString(mod, "EncodeError");
  state->decode_error_class = PyObject_GetAttrString(mod, "DecodeError");
  state->message_class = PyObject_GetAttrString(mod, "Message");
  Py_DECREF(mod);

  PyObject* enum_type_wrapper = PyImport_ImportModule(
      PYUPB_PROTOBUF_INTERNAL_PACKAGE ".enum_type_wrapper");
  if (enum_type_wrapper == NULL) return false;

  state->enum_type_wrapper_class =
      PyObject_GetAttrString(enum_type_wrapper, "EnumTypeWrapper");
  Py_DECREF(enum_type_wrapper);

  if (!state->encode_error_class || !state->decode_error_class ||
      !state->message_class || !state->listfields_item_key ||
      !state->enum_type_wrapper_class) {
    return false;
  }

  return true;
}
