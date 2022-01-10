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
#include "upb/def.h"
#include "upb/reflection.h"
#include "upb/text_encode.h"
#include "upb/util/required_fields.h"

static const upb_msgdef* PyUpb_MessageMeta_GetMsgdef(PyObject* cls);
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
  getattrofunc type_getattro;  // PyTypeObject.tp_getattro
  setattrofunc type_setattro;  // PyTypeObject.tp_setattro
  size_t type_basicsize;       // sizeof(PyHeapTypeObject)

  // While we can refer to PY_VERSION_HEX in the limited API, this will give us
  // the version of Python we were compiled against, which may be different
  // than the version we are dynamically linked against.  Here we want the
  // version that is actually running in this process.
  long python_version_hex;     // PY_VERSION_HEX
} PyUpb_CPythonBits;

// A global containing the values for this process.
PyUpb_CPythonBits cpython_bits;

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
  bits->type_getattro = PyType_GetSlot(type, Py_tp_getattro);
  bits->type_setattro = PyType_GetSlot(type, Py_tp_setattro);

  size = PyObject_GetAttrString((PyObject*)&PyType_Type, "__basicsize__");
  if (!size) goto err;
  bits->type_basicsize = PyLong_AsLong(size);
  if (bits->type_basicsize == -1) goto err;

  assert(bits->type_new && bits->type_getattro && bits->type_setattro);

#ifndef Py_LIMITED_API
  assert(bits->type_new == PyType_Type.tp_new);
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
// CMessage
// -----------------------------------------------------------------------------

// The main message object.  The type of the object (PyUpb_CMessage.ob_type)
// will be an instance of the PyUpb_MessageMeta type (defined below).  So the
// chain is:
//   FooMessage = MessageMeta(...)
//   foo = FooMessage()
//
// Which becomes:
//   Object             C Struct Type        Python type (ob_type)
//   -----------------  -----------------    ---------------------
//   foo                PyUpb_CMessage       FooMessage
//   FooMessage         PyUpb_MessageMeta    message_meta_type
//   message_meta_type  PyTypeObject         'type' in Python
//
// A message object can be in one of two states: present or non-present.  When
// a message is non-present, it stores a reference to its parent, and a write
// to any attribute will trigger the message to become present in its parent.
// The parent may also be non-present, in which case a mutation will trigger a
// chain reaction.
typedef struct PyUpb_CMessage {
  PyObject_HEAD
  PyObject* arena;
  uintptr_t def;  // Tagged, low bit 1 == upb_fielddef*, else upb_msgdef*
  union {
    // when def is msgdef, the data for this msg.
    upb_msg* msg;
    // when def is fielddef, owning pointer to parent
    struct PyUpb_CMessage* parent;
  } ptr;
  PyObject* ext_dict;  // Weak pointer to extension dict, if any.
  // name->obj dict for non-present msg/map/repeated, NULL if none.
  PyUpb_WeakMap* unset_subobj_map;
  int version;
} PyUpb_CMessage;

static PyObject* PyUpb_CMessage_GetAttr(PyObject* _self, PyObject* attr);

bool PyUpb_CMessage_IsStub(PyUpb_CMessage* msg) { return msg->def & 1; }

const upb_fielddef* PyUpb_CMessage_GetFieldDef(PyUpb_CMessage* msg) {
  assert(PyUpb_CMessage_IsStub(msg));
  return (void*)(msg->def & ~(uintptr_t)1);
}

static const upb_msgdef* _PyUpb_CMessage_GetMsgdef(PyUpb_CMessage* msg) {
  return PyUpb_CMessage_IsStub(msg)
             ? upb_fielddef_msgsubdef(PyUpb_CMessage_GetFieldDef(msg))
             : (void*)msg->def;
}

const upb_msgdef* PyUpb_CMessage_GetMsgdef(PyObject* self) {
  return _PyUpb_CMessage_GetMsgdef((PyUpb_CMessage*)self);
}

static upb_msg* PyUpb_CMessage_GetMsg(PyUpb_CMessage* self) {
  assert(!PyUpb_CMessage_IsStub(self));
  return self->ptr.msg;
}

bool PyUpb_CMessage_TryCheck(PyObject* self) {
  PyUpb_ModuleState* state = PyUpb_ModuleState_Get();
  PyObject* type = (PyObject*)Py_TYPE(self);
  return Py_TYPE(type) == state->message_meta_type;
}

bool PyUpb_CMessage_Check(PyObject* self) {
  if (!PyUpb_CMessage_TryCheck(self)) {
    PyErr_Format(PyExc_TypeError, "Expected a message object, but got %R.",
                 self);
    return false;
  }
  return true;
}

// If the message is reified, returns it.  Otherwise, returns NULL.
// If NULL is returned, the object is empty and has no underlying data.
upb_msg* PyUpb_CMessage_GetIfReified(PyObject* _self) {
  PyUpb_CMessage* self = (void*)_self;
  return PyUpb_CMessage_IsStub(self) ? NULL : self->ptr.msg;
}

static PyObject* PyUpb_CMessage_New(PyObject* cls, PyObject* unused_args,
                                    PyObject* unused_kwargs) {
  const upb_msgdef* msgdef = PyUpb_MessageMeta_GetMsgdef(cls);
  PyUpb_CMessage* msg = (void*)PyType_GenericAlloc((PyTypeObject*)cls, 0);
  msg->def = (uintptr_t)msgdef;
  msg->arena = PyUpb_Arena_New();
  msg->ptr.msg = upb_msg_new(msgdef, PyUpb_Arena_Get(msg->arena));
  msg->unset_subobj_map = NULL;
  msg->ext_dict = NULL;
  msg->version = 0;

  PyObject* ret = &msg->ob_base;
  PyUpb_ObjCache_Add(msg->ptr.msg, ret);
  return ret;
}

/*
 * PyUpb_CMessage_LookupName()
 *
 * Tries to find a field or oneof named `py_name` in the message object `self`.
 * The user must pass `f` and/or `o` to indicate whether a field or a oneof name
 * is expected.  If the name is found and it has an expected type, the function
 * sets `*f` or `*o` respectively and returns true.  Otherwise returns false
 * and sets an exception of type `exc_type` if provided.
 */
static bool PyUpb_CMessage_LookupName(PyUpb_CMessage* self, PyObject* py_name,
                                      const upb_fielddef** f,
                                      const upb_oneofdef** o,
                                      PyObject* exc_type) {
  assert(f || o);
  Py_ssize_t size;
  const char* name = NULL;
  if (PyUnicode_Check(py_name)) {
    name = PyUnicode_AsUTF8AndSize(py_name, &size);
  } else if (PyBytes_Check(py_name)) {
    PyBytes_AsStringAndSize(py_name, (char**)&name, &size);
  }
  if (!name) return NULL;
  const upb_msgdef* msgdef = _PyUpb_CMessage_GetMsgdef(self);

  if (!upb_msgdef_lookupname(msgdef, name, size, f, o)) {
    if (exc_type) {
      PyErr_Format(exc_type,
                   "Protocol message %s has no field or oneof named %s.",
                   upb_msgdef_fullname(msgdef), name);
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

static bool PyUpb_CMessage_InitMessageMapEntry(PyObject* dst, PyObject* src) {
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

int PyUpb_CMessage_InitMapAttributes(PyObject* map, PyObject* value,
                                     const upb_fielddef* f) {
  const upb_msgdef* entry_m = upb_fielddef_msgsubdef(f);
  const upb_fielddef* val_f = upb_msgdef_field(entry_m, 1);
  PyObject* it = NULL;
  PyObject* tmp = NULL;
  int ret = -1;
  if (upb_fielddef_issubmsg(val_f)) {
    it = PyObject_GetIter(value);
    if (it == NULL) {
      PyErr_Format(PyExc_TypeError, "Argument for field %s is not iterable",
                   upb_fielddef_fullname(f));
      goto err;
    }
    PyObject* e;
    while ((e = PyIter_Next(it)) != NULL) {
      PyObject* src = PyObject_GetItem(value, e);
      PyObject* dst = PyObject_GetItem(map, e);
      Py_DECREF(e);
      bool ok = PyUpb_CMessage_InitMessageMapEntry(dst, src);
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

void PyUpb_CMessage_EnsureReified(PyUpb_CMessage* self);

static bool PyUpb_CMessage_InitMapAttribute(PyObject* _self, PyObject* name,
                                            const upb_fielddef* f,
                                            PyObject* value) {
  PyObject* map = PyUpb_CMessage_GetAttr(_self, name);
  int ok = PyUpb_CMessage_InitMapAttributes(map, value, f);
  Py_DECREF(map);
  return ok >= 0;
}

static bool PyUpb_CMessage_InitRepeatedAttribute(PyObject* _self,
                                                 PyObject* name,
                                                 PyObject* value) {
  PyObject* repeated = PyUpb_CMessage_GetAttr(_self, name);
  PyObject* tmp = PyUpb_RepeatedContainer_Extend(repeated, value);
  if (!tmp) return false;
  Py_DECREF(tmp);
  return true;
}

static bool PyUpb_CMessage_InitMessageAttribute(PyObject* _self, PyObject* name,
                                                PyObject* value) {
  PyObject* submsg = PyUpb_CMessage_GetAttr(_self, name);
  if (!submsg) return -1;
  assert(!PyErr_Occurred());
  bool ok;
  if (PyUpb_CMessage_TryCheck(value)) {
    PyObject* tmp = PyUpb_CMessage_MergeFrom(submsg, value);
    ok = tmp != NULL;
    Py_DECREF(tmp);
  } else if (PyDict_Check(value)) {
    assert(!PyErr_Occurred());
    ok = PyUpb_CMessage_InitAttributes(submsg, NULL, value) >= 0;
  } else {
    const upb_msgdef* m = PyUpb_CMessage_GetMsgdef(_self);
    PyErr_Format(PyExc_TypeError, "Message must be initialized with a dict: %s",
                 upb_msgdef_fullname(m));
    ok = false;
  }
  Py_DECREF(submsg);
  return ok;
}

static bool PyUpb_CMessage_InitScalarAttribute(upb_msg* msg,
                                               const upb_fielddef* f,
                                               PyObject* value,
                                               upb_arena* arena) {
  upb_msgval msgval;
  assert(!PyErr_Occurred());
  if (!PyUpb_PyToUpb(value, f, &msgval, arena)) return false;
  upb_msg_set(msg, f, msgval, arena);
  return true;
}

int PyUpb_CMessage_InitAttributes(PyObject* _self, PyObject* args,
                                  PyObject* kwargs) {
  assert(!PyErr_Occurred());

  if (args != NULL && PyTuple_Size(args) != 0) {
    PyErr_SetString(PyExc_TypeError, "No positional arguments allowed");
    return -1;
  }

  if (kwargs == NULL) return 0;

  PyUpb_CMessage* self = (void*)_self;
  Py_ssize_t pos = 0;
  PyObject* name;
  PyObject* value;
  PyUpb_CMessage_EnsureReified(self);
  upb_msg* msg = PyUpb_CMessage_GetMsg(self);
  upb_arena* arena = PyUpb_Arena_Get(self->arena);

  while (PyDict_Next(kwargs, &pos, &name, &value)) {
    assert(!PyErr_Occurred());
    const upb_fielddef* f;
    assert(!PyErr_Occurred());
    if (!PyUpb_CMessage_LookupName(self, name, &f, NULL, PyExc_ValueError)) {
      return -1;
    }

    if (value == Py_None) continue;  // Ignored.

    assert(!PyErr_Occurred());

    if (upb_fielddef_ismap(f)) {
      if (!PyUpb_CMessage_InitMapAttribute(_self, name, f, value)) return -1;
    } else if (upb_fielddef_isseq(f)) {
      if (!PyUpb_CMessage_InitRepeatedAttribute(_self, name, value)) return -1;
    } else if (upb_fielddef_issubmsg(f)) {
      if (!PyUpb_CMessage_InitMessageAttribute(_self, name, value)) return -1;
    } else {
      if (!PyUpb_CMessage_InitScalarAttribute(msg, f, value, arena)) return -1;
    }
    if (PyErr_Occurred()) return -1;
  }

  if (PyErr_Occurred()) return -1;
  return 0;
}

static int PyUpb_CMessage_Init(PyObject* _self, PyObject* args,
                               PyObject* kwargs) {
  if (args != NULL && PyTuple_Size(args) != 0) {
    PyErr_SetString(PyExc_TypeError, "No positional arguments allowed");
    return -1;
  }

  return PyUpb_CMessage_InitAttributes(_self, args, kwargs);
}

static PyObject* PyUpb_CMessage_NewStub(PyObject* parent, const upb_fielddef* f,
                                        PyObject* arena) {
  const upb_msgdef* sub_m = upb_fielddef_msgsubdef(f);
  PyObject* cls = PyUpb_Descriptor_GetClass(sub_m);

  PyUpb_CMessage* msg = (void*)PyType_GenericAlloc((PyTypeObject*)cls, 0);
  msg->def = (uintptr_t)f | 1;
  msg->arena = arena;
  msg->ptr.parent = (PyUpb_CMessage*)parent;
  msg->unset_subobj_map = NULL;
  msg->ext_dict = NULL;
  msg->version = 0;

  Py_DECREF(cls);
  Py_INCREF(parent);
  Py_INCREF(arena);
  return &msg->ob_base;
}

static bool PyUpb_CMessage_IsEqual(PyUpb_CMessage* m1, PyObject* _m2) {
  PyUpb_CMessage* m2 = (void*)_m2;
  if (m1 == m2) return true;
  if (!PyObject_TypeCheck(_m2, m1->ob_base.ob_type)) {
    return false;
  }
  const upb_msgdef* m1_msgdef = _PyUpb_CMessage_GetMsgdef(m1);
#ifndef NDEBUG
  const upb_msgdef* m2_msgdef = _PyUpb_CMessage_GetMsgdef(m2);
  assert(m1_msgdef == m2_msgdef);
#endif
  const upb_msg* m1_msg = PyUpb_CMessage_GetIfReified((PyObject*)m1);
  const upb_msg* m2_msg = PyUpb_CMessage_GetIfReified(_m2);
  return PyUpb_Message_IsEqual(m1_msg, m2_msg, m1_msgdef);
}

static const upb_fielddef* PyUpb_CMessage_InitAsMsg(PyUpb_CMessage* m,
                                                    upb_arena* arena) {
  const upb_fielddef* f = PyUpb_CMessage_GetFieldDef(m);
  m->ptr.msg = upb_msg_new(upb_fielddef_msgsubdef(f), arena);
  m->def = (uintptr_t)upb_fielddef_msgsubdef(f);
  PyUpb_ObjCache_Add(m->ptr.msg, &m->ob_base);
  return f;
}

static void PyUpb_CMessage_SetField(PyUpb_CMessage* parent,
                                    const upb_fielddef* f,
                                    PyUpb_CMessage* child, upb_arena* arena) {
  upb_msgval msgval = {.msg_val = PyUpb_CMessage_GetMsg(child)};
  upb_msg_set(PyUpb_CMessage_GetMsg(parent), f, msgval, arena);
  PyUpb_WeakMap_Delete(parent->unset_subobj_map, f);
  // Releases a ref previously owned by child->ptr.parent of our child.
  Py_DECREF(child);
}

/*
 * PyUpb_CMessage_EnsureReified()
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
 *   PyUpb_CMessage_IsStub(self) is false
 */
void PyUpb_CMessage_EnsureReified(PyUpb_CMessage* self) {
  if (!PyUpb_CMessage_IsStub(self)) return;
  upb_arena* arena = PyUpb_Arena_Get(self->arena);

  // This is a non-present message. We need to create a real upb_msg for this
  // object and every parent until we reach a present message.
  PyUpb_CMessage* child = self;
  PyUpb_CMessage* parent = self->ptr.parent;
  const upb_fielddef* child_f = PyUpb_CMessage_InitAsMsg(child, arena);
  Py_INCREF(child);  // To avoid a special-case in PyUpb_CMessage_SetField().

  do {
    PyUpb_CMessage* next_parent = parent->ptr.parent;
    const upb_fielddef* parent_f = NULL;
    if (PyUpb_CMessage_IsStub(parent)) {
      parent_f = PyUpb_CMessage_InitAsMsg(parent, arena);
    }
    PyUpb_CMessage_SetField(parent, child_f, child, arena);
    child = parent;
    child_f = parent_f;
    parent = next_parent;
  } while (child_f);

  // Releases ref previously owned by child->ptr.parent of our child.
  Py_DECREF(child);
  self->version++;
}

static void PyUpb_CMessage_SyncSubobjs(PyUpb_CMessage* self);

/*
 * PyUpb_CMessage_Reify()
 *
 * The message equivalent of PyUpb_*Container_Reify(), this transitions
 * the wrapper from the unset state (owning a reference on self->ptr.parent) to the
 * set state (having a non-owning pointer to self->ptr.msg).
 */
static void PyUpb_CMessage_Reify(PyUpb_CMessage* self, const upb_fielddef* f,
                                 upb_msg* msg) {
  assert(f == PyUpb_CMessage_GetFieldDef(self));
  if (!msg) {
    const upb_msgdef* msgdef = PyUpb_CMessage_GetMsgdef((PyObject*)self);
    msg = upb_msg_new(msgdef, PyUpb_Arena_Get(self->arena));
  }
  PyUpb_ObjCache_Add(msg, &self->ob_base);
  Py_DECREF(&self->ptr.parent->ob_base);
  self->ptr.msg = msg;  // Overwrites self->ptr.parent
  self->def = (uintptr_t)upb_fielddef_msgsubdef(f);
  PyUpb_CMessage_SyncSubobjs(self);
}

/*
 * PyUpb_CMessage_SyncSubobjs()
 *
 * This operation must be invoked whenever the underlying upb_msg has been
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
static void PyUpb_CMessage_SyncSubobjs(PyUpb_CMessage* self) {
  PyUpb_WeakMap* subobj_map = self->unset_subobj_map;
  if (!subobj_map) return;

  upb_msg* msg = PyUpb_CMessage_GetMsg(self);
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
    const upb_fielddef* f = key;
    if (upb_fielddef_haspresence(f) && !upb_msg_has(msg, f)) continue;
    upb_msgval msgval = upb_msg_get(msg, f);
    PyUpb_WeakMap_DeleteIter(subobj_map, &iter);
    if (upb_fielddef_ismap(f)) {
      if (!msgval.map_val) continue;
      PyUpb_MapContainer_Reify(obj, (upb_map*)msgval.map_val);
    } else if (upb_fielddef_isseq(f)) {
      if (!msgval.array_val) continue;
      PyUpb_RepeatedContainer_Reify(obj, (upb_array*)msgval.array_val);
    } else {
      PyUpb_CMessage* sub = (void*)obj;
      assert(self == sub->ptr.parent);
      PyUpb_CMessage_Reify(sub, f, (upb_msg*)msgval.msg_val);
    }
  }

  Py_DECREF(&self->ob_base);

  // TODO(haberman): present fields need to be iterated too if they can reach
  // a WeakMap.
}

static PyObject* PyUpb_CMessage_ToString(PyUpb_CMessage* self) {
  if (PyUpb_CMessage_IsStub(self)) {
    return PyUnicode_FromStringAndSize(NULL, 0);
  }
  upb_msg* msg = PyUpb_CMessage_GetMsg(self);
  const upb_msgdef* msgdef = _PyUpb_CMessage_GetMsgdef(self);
  const upb_symtab* symtab = upb_filedef_symtab(upb_msgdef_file(msgdef));
  char buf[1024];
  int options = UPB_TXTENC_SKIPUNKNOWN;
  size_t size = upb_text_encode(msg, msgdef, symtab, options, buf, sizeof(buf));
  if (size < sizeof(buf)) {
    return PyUnicode_FromStringAndSize(buf, size);
  } else {
    char* buf2 = malloc(size + 1);
    size_t size2 =
        upb_text_encode(msg, msgdef, symtab, options, buf2, size + 1);
    assert(size == size2);
    PyObject* ret = PyUnicode_FromStringAndSize(buf2, size2);
    free(buf2);
    return ret;
  }
}

static PyObject* PyUpb_CMessage_RichCompare(PyObject* _self, PyObject* other,
                                            int opid) {
  PyUpb_CMessage* self = (void*)_self;
  if (opid != Py_EQ && opid != Py_NE) {
    Py_INCREF(Py_NotImplemented);
    return Py_NotImplemented;
  }
  bool ret = PyUpb_CMessage_IsEqual(self, other);
  if (opid == Py_NE) ret = !ret;
  return PyBool_FromLong(ret);
}

void PyUpb_CMessage_CacheDelete(PyObject* _self, const upb_fielddef* f) {
  PyUpb_CMessage* self = (void*)_self;
  PyUpb_WeakMap_Delete(self->unset_subobj_map, f);
}

void PyUpb_CMessage_SetConcreteSubobj(PyObject* _self, const upb_fielddef* f,
                                      upb_msgval subobj) {
  PyUpb_CMessage* self = (void*)_self;
  PyUpb_CMessage_EnsureReified(self);
  PyUpb_CMessage_CacheDelete(_self, f);
  upb_msg_set(self->ptr.msg, f, subobj, PyUpb_Arena_Get(self->arena));
}

static void PyUpb_CMessage_Dealloc(PyObject* _self) {
  PyUpb_CMessage* self = (void*)_self;

  if (PyUpb_CMessage_IsStub(self)) {
    PyUpb_CMessage_CacheDelete((PyObject*)self->ptr.parent,
                               PyUpb_CMessage_GetFieldDef(self));
    Py_DECREF(self->ptr.parent);
  } else {
    PyUpb_ObjCache_Delete(self->ptr.msg);
  }

  if (self->unset_subobj_map) {
    PyUpb_WeakMap_Free(self->unset_subobj_map);
  }

  Py_DECREF(self->arena);

  // We do not use PyUpb_Dealloc() here because CMessage is a base type and for
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

PyObject* PyUpb_CMessage_Get(upb_msg* u_msg, const upb_msgdef* m,
                             PyObject* arena) {
  PyObject* ret = PyUpb_ObjCache_Get(u_msg);
  if (ret) return ret;

  PyObject* cls = PyUpb_Descriptor_GetClass(m);
  // It is not safe to use PyObject_{,GC}_New() due to:
  //    https://bugs.python.org/issue35810
  PyUpb_CMessage* py_msg = (void*)PyType_GenericAlloc((PyTypeObject*)cls, 0);
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

/* PyUpb_CMessage_GetStub()
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
PyObject* PyUpb_CMessage_GetStub(PyUpb_CMessage* self,
                                 const upb_fielddef* field) {
  PyObject* _self = (void*)self;
  if (!self->unset_subobj_map) {
    self->unset_subobj_map = PyUpb_WeakMap_New();
  }
  PyObject* subobj = PyUpb_WeakMap_Get(self->unset_subobj_map, field);

  if (subobj) return subobj;

  if (upb_fielddef_ismap(field)) {
    subobj = PyUpb_MapContainer_NewStub(_self, field, self->arena);
  } else if (upb_fielddef_isseq(field)) {
    subobj = PyUpb_RepeatedContainer_NewStub(_self, field, self->arena);
  } else {
    subobj = PyUpb_CMessage_NewStub(&self->ob_base, field, self->arena);
  }
  PyUpb_WeakMap_Add(self->unset_subobj_map, field, subobj);

  assert(!PyErr_Occurred());
  return subobj;
}

PyObject* PyUpb_CMessage_GetPresentWrapper(PyUpb_CMessage* self,
                                           const upb_fielddef* field) {
  assert(!PyUpb_CMessage_IsStub(self));
  upb_mutmsgval mutval =
      upb_msg_mutable(self->ptr.msg, field, PyUpb_Arena_Get(self->arena));
  if (upb_fielddef_ismap(field)) {
    return PyUpb_MapContainer_GetOrCreateWrapper(mutval.map, field,
                                                 self->arena);
  } else {
    return PyUpb_RepeatedContainer_GetOrCreateWrapper(mutval.array, field,
                                                      self->arena);
  }
}

PyObject* PyUpb_CMessage_GetScalarValue(PyUpb_CMessage* self,
                                        const upb_fielddef* field) {
  upb_msgval val;
  if (PyUpb_CMessage_IsStub(self)) {
    // Unset message always returns default values.
    val = upb_fielddef_default(field);
  } else {
    val = upb_msg_get(self->ptr.msg, field);
  }
  return PyUpb_UpbToPy(val, field, self->arena);
}

/*
 * PyUpb_CMessage_GetFieldValue()
 *
 * Implements the equivalent of getattr(msg, field), once `field` has
 * already been resolved to a `upb_fielddef*`.
 *
 * This may involve constructing a wrapper object for the given field, or
 * returning one that was previously constructed.  If the field is not actually
 * set, the wrapper object will be an "unset" object that is not actually
 * connected to any C data.
 */
PyObject* PyUpb_CMessage_GetFieldValue(PyObject* _self,
                                       const upb_fielddef* field) {
  PyUpb_CMessage* self = (void*)_self;
  assert(upb_fielddef_containingtype(field) == PyUpb_CMessage_GetMsgdef(_self));
  bool submsg = upb_fielddef_issubmsg(field);
  bool seq = upb_fielddef_isseq(field);

  if ((PyUpb_CMessage_IsStub(self) && (submsg || seq)) ||
      (submsg && !seq && !upb_msg_has(self->ptr.msg, field))) {
    return PyUpb_CMessage_GetStub(self, field);
  } else if (seq) {
    return PyUpb_CMessage_GetPresentWrapper(self, field);
  } else {
    return PyUpb_CMessage_GetScalarValue(self, field);
  }
}

int PyUpb_CMessage_SetFieldValue(PyObject* _self, const upb_fielddef* field,
                                 PyObject* value) {
  PyUpb_CMessage* self = (void*)_self;
  assert(value);

  if (upb_fielddef_issubmsg(field) || upb_fielddef_isseq(field)) {
    PyErr_Format(PyExc_AttributeError,
                 "Assignment not allowed to message, map, or repeated "
                 "field \"%s\" in protocol message object.",
                 upb_fielddef_name(field));
    return -1;
  }

  PyUpb_CMessage_EnsureReified(self);

  upb_msgval val;
  upb_arena* arena = PyUpb_Arena_Get(self->arena);
  if (!PyUpb_PyToUpb(value, field, &val, arena)) {
    return -1;
  }

  upb_msg_set(self->ptr.msg, field, val, arena);
  return 0;
}

int PyUpb_CMessage_GetVersion(PyObject* _self) {
  PyUpb_CMessage* self = (void*)_self;
  return self->version;
}

/*
 * PyUpb_CMessage_GetAttr()
 *
 * Implements:
 *   foo = msg.foo
 *
 * Attribute lookup must find both message fields and base class methods like
 * msg.SerializeToString().
 */
__attribute__((flatten)) static PyObject* PyUpb_CMessage_GetAttr(
    PyObject* _self, PyObject* attr) {
  PyUpb_CMessage* self = (void*)_self;

  // Lookup field by name.
  const upb_fielddef* field;
  if (PyUpb_CMessage_LookupName(self, attr, &field, NULL, NULL)) {
    return PyUpb_CMessage_GetFieldValue(_self, field);
  }

  // Check base class attributes.
  assert(!PyErr_Occurred());
  PyObject* ret = PyObject_GenericGetAttr(_self, attr);
  if (ret) return ret;

  // Swallow AttributeError if it occurred and try again on the metaclass
  // to pick up class attributes.  But we have to special-case "Extensions"
  // which affirmatively returns AttributeError when a message is not
  // extendable.
  if (PyErr_ExceptionMatches(PyExc_AttributeError) &&
      strcmp(PyUpb_GetStrData(attr), "Extensions") != 0) {
    PyErr_Clear();
    return PyUpb_MessageMeta_GetAttr((PyObject*)Py_TYPE(_self), attr);
  }

  return NULL;
}

/*
 * PyUpb_CMessage_SetAttr()
 *
 * Implements:
 *   msg.foo = foo
 */
static int PyUpb_CMessage_SetAttr(PyObject* _self, PyObject* attr,
                                  PyObject* value) {
  PyUpb_CMessage* self = (void*)_self;
  const upb_fielddef* field;
  if (!PyUpb_CMessage_LookupName(self, attr, &field, NULL,
                                 PyExc_AttributeError)) {
    return -1;
  }

  return PyUpb_CMessage_SetFieldValue(_self, field, value);
}

static PyObject* PyUpb_CMessage_HasField(PyObject* _self, PyObject* arg) {
  PyUpb_CMessage* self = (void*)_self;
  const upb_fielddef* field;
  const upb_oneofdef* oneof;

  if (!PyUpb_CMessage_LookupName(self, arg, &field, &oneof, PyExc_ValueError)) {
    return NULL;
  }

  if (field && !upb_fielddef_haspresence(field)) {
    PyErr_Format(PyExc_ValueError, "Field %s does not have presence.",
                 upb_fielddef_fullname(field));
    return NULL;
  }

  if (PyUpb_CMessage_IsStub(self)) Py_RETURN_FALSE;

  return PyBool_FromLong(field ? upb_msg_has(self->ptr.msg, field)
                               : upb_msg_whichoneof(self->ptr.msg, oneof) != NULL);
}

static PyObject* PyUpb_CMessage_FindInitializationErrors(PyObject* _self,
                                                         PyObject* arg);

static PyObject* PyUpb_CMessage_IsInitialized(PyObject* _self, PyObject* args) {
  PyObject* errors = NULL;
  if (!PyArg_ParseTuple(args, "|O", &errors)) {
    return NULL;
  }
  upb_msg* msg = PyUpb_CMessage_GetIfReified(_self);
  if (!msg) Py_RETURN_NONE;  // TODO
  if (errors) {
    PyObject* list = PyUpb_CMessage_FindInitializationErrors(_self, NULL);
    if (!list) return NULL;
    if (PyList_Size(list) == 0) {
      return PyBool_FromLong(true);
    }
    PyObject* extend_name = PyUnicode_FromString("extend");
    if (extend_name == NULL) {
      Py_DECREF(list);
      return NULL;
    }
    PyObject_CallMethodObjArgs(errors, extend_name, list, NULL);
    return PyBool_FromLong(false);
  } else {
    const upb_msgdef* m = PyUpb_CMessage_GetMsgdef(_self);
    const upb_symtab* symtab = upb_filedef_symtab(upb_msgdef_file(m));
    bool initialized = !upb_util_HasUnsetRequired(msg, m, symtab, NULL);
    return PyBool_FromLong(initialized);
  }
}

static PyObject* PyUpb_CMessage_ListFieldsLessThan(PyObject* self,
                                                   PyObject* val) {
  assert(PyTuple_Check(val));
  PyObject* field = PyTuple_GetItem(val, 0);
  const upb_fielddef* f = PyUpb_FieldDescriptor_GetDef(field);
  return PyLong_FromLong(upb_fielddef_number(f));
}

static PyObject* PyUpb_CMessage_ListFields(PyObject* _self, PyObject* arg) {
  PyObject* list = PyList_New(0);
  upb_msg* msg = PyUpb_CMessage_GetIfReified(_self);
  if (!msg) return list;

  size_t iter1 = UPB_MSG_BEGIN;
  const upb_msgdef* m = PyUpb_CMessage_GetMsgdef(_self);
  const upb_symtab* symtab = upb_filedef_symtab(upb_msgdef_file(m));
  const upb_fielddef* f;
  PyObject* field_desc = NULL;
  PyObject* py_val = NULL;
  PyObject* tuple = NULL;
  upb_msgval val;
  uint32_t last_field = 0;
  bool in_order = true;
  while (upb_msg_next(msg, m, symtab, &f, &val, &iter1)) {
    const uint32_t field_number = upb_fielddef_number(f);
    if (field_number < last_field) in_order = false;
    last_field = field_number;
    PyObject* field_desc = PyUpb_FieldDescriptor_Get(f);
    PyObject* py_val = PyUpb_CMessage_GetFieldValue(_self, f);
    if (!field_desc || !py_val) goto err;
    PyObject* tuple = Py_BuildValue("(NN)", field_desc, py_val);
    field_desc = NULL;
    py_val = NULL;
    if (!tuple) goto err;
    if (PyList_Append(list, tuple)) goto err;
    Py_DECREF(tuple);
    tuple = NULL;
  }

  if (!in_order) {
    PyUpb_ModuleState* state = PyUpb_ModuleState_Get();
    PyObject* args = PyList_New(0);
    PyObject* kwargs = PyDict_New();
    PyDict_SetItemString(kwargs, "key", state->listfields_cmp_lt);
    PyObject* m = PyObject_GetAttrString(list, "sort");
    assert(m);
    assert(args);
    assert(kwargs);
    PyObject* ret = PyObject_Call(m, args, kwargs);
    Py_XDECREF(ret);
    Py_XDECREF(m);
    Py_XDECREF(args);
    Py_XDECREF(kwargs);
    if (!ret) return NULL;
  }

  return list;

err:
  Py_XDECREF(field_desc);
  Py_XDECREF(py_val);
  Py_XDECREF(tuple);
  Py_DECREF(list);
  return NULL;
}

PyObject* PyUpb_CMessage_MergeFrom(PyObject* self, PyObject* arg) {
  if (self->ob_type != arg->ob_type) {
    PyErr_Format(PyExc_TypeError,
                 "Parameter to MergeFrom() must be instance of same class: "
                 "expected %S got %S.",
                 Py_TYPE(self), Py_TYPE(arg));
    return NULL;
  }
  // OPT: exit if src is empty.
  PyObject* subargs = PyTuple_New(0);
  PyObject* serialized = PyUpb_CMessage_SerializeToString(arg, subargs, NULL);
  Py_DECREF(subargs);
  if (!serialized) return NULL;
  PyObject* ret = PyUpb_CMessage_MergeFromString(self, serialized);
  Py_DECREF(serialized);
  Py_DECREF(ret);
  Py_RETURN_NONE;
}

static PyObject* PyUpb_CMessage_SetInParent(PyObject* _self, PyObject* arg) {
  PyUpb_CMessage* self = (void*)_self;
  PyUpb_CMessage_EnsureReified(self);
  Py_RETURN_NONE;
}

static PyObject* PyUpb_CMessage_UnknownFields(PyObject* _self, PyObject* arg) {
  // TODO(haberman): re-enable when unknown fields are added.
  // return PyUpb_UnknownFields_New(_self);
  PyErr_SetString(PyExc_NotImplementedError, "unknown field accessor");
  return NULL;
}

PyObject* PyUpb_CMessage_MergeFromString(PyObject* _self, PyObject* arg) {
  PyUpb_CMessage* self = (void*)_self;
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

  PyUpb_CMessage_EnsureReified(self);
  const upb_msgdef* msgdef = _PyUpb_CMessage_GetMsgdef(self);
  const upb_filedef* file = upb_msgdef_file(msgdef);
  const upb_extreg* extreg = upb_symtab_extreg(upb_filedef_symtab(file));
  const upb_msglayout* layout = upb_msgdef_layout(msgdef);
  upb_arena* arena = PyUpb_Arena_Get(self->arena);
  PyUpb_ModuleState* state = PyUpb_ModuleState_Get();
  int options =
      UPB_DECODE_MAXDEPTH(state->allow_oversize_protos ? UINT32_MAX : 100);
  upb_DecodeStatus status =
      _upb_decode(buf, size, self->ptr.msg, layout, extreg, options, arena);
  Py_XDECREF(bytes);
  if (status != kUpb_DecodeStatus_Ok) {
    PyErr_Format(state->decode_error_class, "Error parsing message");
    return NULL;
  }
  PyUpb_CMessage_SyncSubobjs(self);
  return PyLong_FromSsize_t(size);
}

static PyObject* PyUpb_CMessage_Clear(PyUpb_CMessage* self, PyObject* args);

static PyObject* PyUpb_CMessage_ParseFromString(PyObject* self, PyObject* arg) {
  PyObject* tmp = PyUpb_CMessage_Clear((PyUpb_CMessage*)self, NULL);
  Py_DECREF(tmp);
  return PyUpb_CMessage_MergeFromString(self, arg);
}

static PyObject* PyUpb_CMessage_ByteSize(PyObject* self, PyObject* args) {
  // TODO(https://github.com/protocolbuffers/upb/issues/462): At the moment upb
  // does not have a "byte size" function, so we just serialize to string and
  // get the size of the string.
  PyObject* subargs = PyTuple_New(0);
  PyObject* serialized = PyUpb_CMessage_SerializeToString(self, subargs, NULL);
  Py_DECREF(subargs);
  if (!serialized) return NULL;
  size_t size = PyBytes_Size(serialized);
  Py_DECREF(serialized);
  return PyLong_FromSize_t(size);
}

static PyObject* PyUpb_CMessage_Clear(PyUpb_CMessage* self, PyObject* args) {
  PyUpb_CMessage_EnsureReified(self);
  const upb_msgdef* msgdef = _PyUpb_CMessage_GetMsgdef(self);
  PyUpb_WeakMap* subobj_map = self->unset_subobj_map;

  if (subobj_map) {
    upb_msg* msg = PyUpb_CMessage_GetMsg(self);
    intptr_t iter = PYUPB_WEAKMAP_BEGIN;
    const void* key;
    PyObject* obj;

    while (PyUpb_WeakMap_Next(subobj_map, &key, &obj, &iter)) {
      const upb_fielddef* f = key;
      PyUpb_WeakMap_DeleteIter(subobj_map, &iter);
      if (upb_fielddef_ismap(f)) {
        assert(upb_msg_get(msg, f).map_val == NULL);
        PyUpb_MapContainer_Reify(obj, NULL);
      } else if (upb_fielddef_isseq(f)) {
        assert(upb_msg_get(msg, f).array_val == NULL);
        PyUpb_RepeatedContainer_Reify(obj, NULL);
      } else {
        assert(!upb_msg_has(msg, f));
        PyUpb_CMessage* sub = (void*)obj;
        assert(self == sub->ptr.parent);
        PyUpb_CMessage_Reify(sub, f, NULL);
      }
    }
  }

  upb_msg_clear(self->ptr.msg, msgdef);
  Py_RETURN_NONE;
}

void PyUpb_CMessage_DoClearField(PyObject* _self, const upb_fielddef* f) {
  PyUpb_CMessage* self = (void*)_self;
  PyUpb_CMessage_EnsureReified((PyUpb_CMessage*)self);

  // We must ensure that any stub object is reified so its parent no longer
  // points to us.
  PyObject* sub = self->unset_subobj_map
                      ? PyUpb_WeakMap_Get(self->unset_subobj_map, f)
                      : NULL;

  if (upb_fielddef_ismap(f)) {
    // For maps we additionally have to invalidate any iterators.  So we need
    // to get an object even if it's reified.
    if (!sub) {
      sub = PyUpb_CMessage_GetFieldValue(_self, f);
    }
    PyUpb_MapContainer_EnsureReified(sub);
    PyUpb_MapContainer_Invalidate(sub);
  } else if (upb_fielddef_isseq(f)) {
    if (sub) {
      PyUpb_RepeatedContainer_EnsureReified(sub);
    }
  } else if (upb_fielddef_issubmsg(f)) {
    if (sub) {
      PyUpb_CMessage_EnsureReified((PyUpb_CMessage*)sub);
    }
  }

  Py_XDECREF(sub);
  upb_msg_clearfield(self->ptr.msg, f);
}

static PyObject* PyUpb_CMessage_ClearExtension(PyObject* _self, PyObject* arg) {
  PyUpb_CMessage* self = (void*)_self;
  PyUpb_CMessage_EnsureReified(self);
  const upb_fielddef* f = PyUpb_CMessage_GetExtensionDef(_self, arg);
  if (!f) return NULL;
  PyUpb_CMessage_DoClearField(_self, f);
  Py_RETURN_NONE;
}

static PyObject* PyUpb_CMessage_ClearField(PyObject* _self, PyObject* arg) {
  PyUpb_CMessage* self = (void*)_self;

  // We always need EnsureReified() here (even for an unset message) to
  // preserve behavior like:
  //   msg = FooMessage()
  //   msg.foo.Clear()
  //   assert msg.HasField("foo")
  PyUpb_CMessage_EnsureReified(self);

  const upb_fielddef* f;
  const upb_oneofdef* o;
  if (!PyUpb_CMessage_LookupName(self, arg, &f, &o, PyExc_ValueError)) {
    return NULL;
  }

  if (o) f = upb_msg_whichoneof(self->ptr.msg, o);
  PyUpb_CMessage_DoClearField(_self, f);
  Py_RETURN_NONE;
}

static PyObject* PyUpb_CMessage_DiscardUnknownFields(PyUpb_CMessage* self,
                                                     PyObject* arg) {
  PyUpb_CMessage_EnsureReified(self);
  const upb_msgdef* msgdef = _PyUpb_CMessage_GetMsgdef(self);
  upb_msg_discardunknown(self->ptr.msg, msgdef, 64);
  Py_RETURN_NONE;
}

static PyObject* PyUpb_CMessage_FindInitializationErrors(PyObject* _self,
                                                         PyObject* arg) {
  PyUpb_CMessage* self = (void*)_self;
  upb_msg* msg = PyUpb_CMessage_GetIfReified(_self);
  if (!msg) return PyList_New(0);
  const upb_msgdef* msgdef = _PyUpb_CMessage_GetMsgdef(self);
  const upb_symtab* ext_pool = upb_filedef_symtab(upb_msgdef_file(msgdef));
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
      PyList_Append(ret, PyUnicode_FromString(buf));
    }
    free(buf);
  }
  return ret;
}

static PyObject* PyUpb_CMessage_FromString(PyObject* cls,
                                           PyObject* serialized) {
  PyObject* ret = NULL;
  PyObject* length = NULL;

  ret = PyObject_CallObject(cls, NULL);
  if (ret == NULL) goto err;
  length = PyUpb_CMessage_MergeFromString(ret, serialized);
  if (length == NULL) goto err;

done:
  Py_XDECREF(length);
  return ret;

err:
  Py_XDECREF(ret);
  ret = NULL;
  goto done;
}

const upb_fielddef* PyUpb_CMessage_GetExtensionDef(PyObject* _self, PyObject* key) {
  const upb_fielddef* f = PyUpb_FieldDescriptor_GetDef(key);
  if (!f) {
    PyErr_Clear();
    PyErr_Format(PyExc_KeyError, "Object %R is not a field descriptor\n", key);
    return NULL;
  }
  if (!upb_fielddef_isextension(f)) {
    PyErr_Format(PyExc_KeyError, "Field %s is not an extension\n",
                 upb_fielddef_fullname(f));
    return NULL;
  }
  const upb_msgdef* msgdef = PyUpb_CMessage_GetMsgdef(_self);
  if (upb_fielddef_containingtype(f) != msgdef) {
    PyErr_Format(PyExc_KeyError, "Extension doesn't match (%s vs %s)",
                 upb_msgdef_fullname(msgdef), upb_fielddef_fullname(f));
    return NULL;
  }
  return f;
}


static PyObject* PyUpb_CMessage_HasExtension(PyObject* _self,
                                             PyObject* ext_desc) {
  upb_msg* msg = PyUpb_CMessage_GetIfReified(_self);
  const upb_fielddef* f = PyUpb_CMessage_GetExtensionDef(_self, ext_desc);
  if (!f) return NULL;
  if (upb_fielddef_isseq(f)) {
    PyErr_SetString(PyExc_KeyError,
                    "Field is repeated. A singular method is required.");
    return NULL;
  }
  if (!msg) Py_RETURN_FALSE;
  return PyBool_FromLong(upb_msg_has(msg, f));
}

PyObject* PyUpb_CMessage_SerializeInternal(PyObject* _self, PyObject* args,
                                           PyObject* kwargs,
                                           bool check_required) {
  PyUpb_CMessage* self = (void*)_self;
  if (!PyUpb_CMessage_Check((PyObject*)self)) return NULL;
  static const char* kwlist[] = {"deterministic", NULL};
  int deterministic = 0;
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|p", (char**)(kwlist),
                                   &deterministic)) {
    return NULL;
  }

  if (PyUpb_CMessage_IsStub(self)) {
    return PyBytes_FromStringAndSize(NULL, 0);
  }

  upb_arena* arena = upb_arena_new();
  const upb_msgdef* msgdef = _PyUpb_CMessage_GetMsgdef(self);
  const upb_msglayout* layout = upb_msgdef_layout(msgdef);
  size_t size = 0;
  int options = UPB_ENCODE_MAXDEPTH(UINT32_MAX);
  if (check_required) options |= UPB_ENCODE_CHECKREQUIRED;
  if (deterministic) options |= UPB_ENCODE_DETERMINISTIC;
  // Python does not currently have any effective limit on serialization depth.
  char* pb = upb_encode_ex(self->ptr.msg, layout, options, arena, &size);
  PyObject* ret = NULL;

  if (!pb) {
    PyUpb_ModuleState* state = PyUpb_ModuleState_Get();
    PyObject* errors = PyUpb_CMessage_FindInitializationErrors(_self, NULL);
    if (PyList_Size(errors) != 0) {
      PyObject* comma = PyUnicode_FromString(",");
      PyObject* missing_fields = PyUnicode_Join(comma, errors);
      PyErr_Format(state->encode_error_class,
                   "Message %s is missing required fields: %U",
                   upb_msgdef_fullname(msgdef), missing_fields);
      Py_XDECREF(comma);
      Py_XDECREF(missing_fields);
      Py_DECREF(errors);
      goto done;
    }
    PyErr_Format(state->encode_error_class, "Failed to serialize proto");
    goto done;
  }

  ret = PyBytes_FromStringAndSize(pb, size);

done:
  upb_arena_free(arena);
  return ret;
}

PyObject* PyUpb_CMessage_SerializeToString(PyObject* _self, PyObject* args,
                                           PyObject* kwargs) {
  return PyUpb_CMessage_SerializeInternal(_self, args, kwargs, true);
}

PyObject* PyUpb_CMessage_SerializePartialToString(PyObject* _self,
                                                  PyObject* args,
                                                  PyObject* kwargs) {
  return PyUpb_CMessage_SerializeInternal(_self, args, kwargs, false);
}

static PyObject* PyUpb_CMessage_WhichOneof(PyObject* _self, PyObject* name) {
  PyUpb_CMessage* self = (void*)_self;
  const upb_oneofdef* o;
  if (!PyUpb_CMessage_LookupName(self, name, NULL, &o, PyExc_ValueError)) {
    return NULL;
  }
  upb_msg* msg = PyUpb_CMessage_GetIfReified(_self);
  if (!msg) Py_RETURN_NONE;
  const upb_fielddef* f = upb_msg_whichoneof(msg, o);
  if (!f) Py_RETURN_NONE;
  return PyUnicode_FromString(upb_fielddef_name(f));
}

void PyUpb_CMessage_ClearExtensionDict(PyObject* _self) {
  PyUpb_CMessage* self = (void*)_self;
  assert(self->ext_dict);
  self->ext_dict = NULL;
}

static PyObject* PyUpb_CMessage_GetExtensionDict(PyObject* _self,
                                                 void* closure) {
  PyUpb_CMessage* self = (void*)_self;
  if (self->ext_dict) {
    return self->ext_dict;
  }

  const upb_msgdef* m = _PyUpb_CMessage_GetMsgdef(self);
  if (upb_msgdef_extrangecount(m) == 0) {
    PyErr_SetNone(PyExc_AttributeError);
    return NULL;
  }

  self->ext_dict = PyUpb_ExtensionDict_New(_self);
  return self->ext_dict;
}

static PyGetSetDef PyUpb_CMessage_Getters[] = {
    {"Extensions", PyUpb_CMessage_GetExtensionDict, NULL, "Extension dict"},
    {NULL}};

static PyMethodDef PyUpb_CMessage_Methods[] = {
    // TODO(https://github.com/protocolbuffers/upb/issues/459)
    //{ "__deepcopy__", (PyCFunction)DeepCopy, METH_VARARGS,
    //  "Makes a deep copy of the class." },
    //{ "__unicode__", (PyCFunction)ToUnicode, METH_NOARGS,
    //  "Outputs a unicode representation of the message." },
    {"ByteSize", (PyCFunction)PyUpb_CMessage_ByteSize, METH_NOARGS,
     "Returns the size of the message in bytes."},
    {"Clear", (PyCFunction)PyUpb_CMessage_Clear, METH_NOARGS,
     "Clears the message."},
    {"ClearExtension", PyUpb_CMessage_ClearExtension, METH_O,
     "Clears a message field."},
    {"ClearField", PyUpb_CMessage_ClearField, METH_O,
     "Clears a message field."},
    // TODO(https://github.com/protocolbuffers/upb/issues/459)
    //{ "CopyFrom", (PyCFunction)CopyFrom, METH_O,
    //  "Copies a protocol message into the current message." },
    {"DiscardUnknownFields", (PyCFunction)PyUpb_CMessage_DiscardUnknownFields,
     METH_NOARGS, "Discards the unknown fields."},
    {"FindInitializationErrors", PyUpb_CMessage_FindInitializationErrors,
     METH_NOARGS, "Finds unset required fields."},
    {"FromString", PyUpb_CMessage_FromString, METH_O | METH_CLASS,
     "Creates new method instance from given serialized data."},
    {"HasExtension", PyUpb_CMessage_HasExtension, METH_O,
     "Checks if a message field is set."},
    {"HasField", PyUpb_CMessage_HasField, METH_O,
     "Checks if a message field is set."},
    {"IsInitialized", PyUpb_CMessage_IsInitialized, METH_VARARGS,
     "Checks if all required fields of a protocol message are set."},
    {"ListFields", PyUpb_CMessage_ListFields, METH_NOARGS,
     "Lists all set fields of a message."},
    {"MergeFrom", PyUpb_CMessage_MergeFrom, METH_O,
     "Merges a protocol message into the current message."},
    {"MergeFromString", PyUpb_CMessage_MergeFromString, METH_O,
     "Merges a serialized message into the current message."},
    {"ParseFromString", PyUpb_CMessage_ParseFromString, METH_O,
     "Parses a serialized message into the current message."},
    // TODO(https://github.com/protocolbuffers/upb/issues/459)
    //{ "RegisterExtension", (PyCFunction)RegisterExtension, METH_O |
    // METH_CLASS,
    //  "Registers an extension with the current message." },
    {"SerializePartialToString",
     (PyCFunction)PyUpb_CMessage_SerializePartialToString,
     METH_VARARGS | METH_KEYWORDS,
     "Serializes the message to a string, even if it isn't initialized."},
    {"SerializeToString", (PyCFunction)PyUpb_CMessage_SerializeToString,
     METH_VARARGS | METH_KEYWORDS,
     "Serializes the message to a string, only for initialized messages."},
    {"SetInParent", (PyCFunction)PyUpb_CMessage_SetInParent, METH_NOARGS,
     "Sets the has bit of the given field in its parent message."},
    {"UnknownFields", (PyCFunction)PyUpb_CMessage_UnknownFields, METH_NOARGS,
     "Parse unknown field set"},
    {"WhichOneof", PyUpb_CMessage_WhichOneof, METH_O,
     "Returns the name of the field set inside a oneof, "
     "or None if no field is set."},
    {"_ListFieldsLessThan", PyUpb_CMessage_ListFieldsLessThan,
     METH_O | METH_STATIC,
     "Compares ListFields() list entries by field number"},
    {NULL, NULL}};

static PyType_Slot PyUpb_CMessage_Slots[] = {
    {Py_tp_dealloc, PyUpb_CMessage_Dealloc},
    {Py_tp_doc, "A ProtocolMessage"},
    {Py_tp_getattro, PyUpb_CMessage_GetAttr},
    {Py_tp_getset, PyUpb_CMessage_Getters},
    {Py_tp_hash, PyObject_HashNotImplemented},
    {Py_tp_methods, PyUpb_CMessage_Methods},
    {Py_tp_new, PyUpb_CMessage_New},
    {Py_tp_str, PyUpb_CMessage_ToString},
    {Py_tp_repr, PyUpb_CMessage_ToString},
    {Py_tp_richcompare, PyUpb_CMessage_RichCompare},
    {Py_tp_setattro, PyUpb_CMessage_SetAttr},
    {Py_tp_init, PyUpb_CMessage_Init},
    {0, NULL}};

PyType_Spec PyUpb_CMessage_Spec = {
    PYUPB_MODULE_NAME ".CMessage",             // tp_name
    sizeof(PyUpb_CMessage),                    // tp_basicsize
    0,                                         // tp_itemsize
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,  // tp_flags
    PyUpb_CMessage_Slots,
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
  const upb_msglayout* layout;
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

static const upb_msgdef* PyUpb_MessageMeta_GetMsgdef(PyObject* cls) {
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

  const upb_msgdef* msgdef = PyUpb_Descriptor_GetDef(py_descriptor);
  assert(msgdef);
  assert(!PyUpb_ObjCache_Get(upb_msgdef_layout(msgdef)));

  PyObject* slots = PyTuple_New(0);
  if (PyDict_SetItemString(dict, "__slots__", slots) < 0) {
    return NULL;
  }

  // Bases are either:
  //    (CMessage, Message)            # for regular messages
  //    (CMessage, Message, WktBase)   # For well-known types
  PyObject* wkt_bases = PyUpb_GetWktBases(state);
  PyObject* wkt_base =
      PyDict_GetItemString(wkt_bases, upb_msgdef_fullname(msgdef));
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
  meta->layout = upb_msgdef_layout(msgdef);
  Py_INCREF(meta->py_message_descriptor);

  PyUpb_ObjCache_Add(upb_msgdef_layout(msgdef), ret);

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

  const upb_msgdef* m = PyUpb_Descriptor_GetDef(py_descriptor);
  PyObject* ret = PyUpb_ObjCache_Get(upb_msgdef_layout(m));
  if (ret) return ret;
  return PyUpb_MessageMeta_DoCreateClass(py_descriptor, name, dict);
}

static void PyUpb_MessageMeta_Dealloc(PyObject* self) {
  PyUpb_MessageMeta* meta = PyUpb_GetMessageMeta(self);
  PyUpb_ObjCache_Delete(meta->layout);
  Py_DECREF(meta->py_message_descriptor);
  PyUpb_Dealloc(self);
}

static PyObject* PyUpb_MessageMeta_GetDynamicAttr(PyObject* self,
                                                  PyObject* name) {
  const char* name_buf = PyUpb_GetStrData(name);
  const upb_msgdef* msgdef = PyUpb_MessageMeta_GetMsgdef(self);
  const upb_filedef* filedef = upb_msgdef_file(msgdef);
  const upb_symtab* symtab = upb_filedef_symtab(filedef);

  PyObject* py_key =
      PyBytes_FromFormat("%s.%s", upb_msgdef_fullname(msgdef), name_buf);
  const char* key = PyUpb_GetStrData(py_key);
  PyObject* ret = NULL;
  const upb_msgdef* nested = upb_symtab_lookupmsg(symtab, key);
  const upb_enumdef* enumdef;
  const upb_enumvaldef* enumval;
  const upb_fielddef* ext;

  if (nested) {
    ret = PyUpb_Descriptor_GetClass(nested);
  } else if ((enumdef = upb_symtab_lookupenum(symtab, key))) {
    PyUpb_ModuleState* state = PyUpb_ModuleState_Get();
    PyObject* klass = state->enum_type_wrapper_class;
    ret = PyUpb_EnumDescriptor_Get(enumdef);
    ret = PyObject_CallFunctionObjArgs(klass, ret, NULL);
  } else if ((enumval = upb_symtab_lookupenumval(symtab, key))) {
    ret = PyLong_FromLong(upb_enumvaldef_number(enumval));
  } else if ((ext = upb_symtab_lookupext(symtab, key))) {
    ret = PyUpb_FieldDescriptor_Get(ext);
  }

  Py_DECREF(py_key);

  const char* suffix =  "_FIELD_NUMBER";
  size_t n = strlen(name_buf);
  size_t suffix_n = strlen(suffix);
  if (n > strlen(suffix) &&
      memcmp(suffix, name_buf + n - suffix_n, suffix_n) == 0) {
    // We can't look up field names dynamically, because the <NAME>_FIELD_NUMBER
    // naming scheme upper-cases the field name and is therefore non-reversible.
    // So we just add all field numbers.
    int n = upb_msgdef_fieldcount(msgdef);
    for (int i = 0; i < n; i++) {
      const upb_fielddef* f = upb_msgdef_field(msgdef, i);
      PyObject* name = PyUnicode_FromFormat(
          "%s_FIELD_NUMBER", upb_fielddef_name(f));
      PyObject* upper = PyObject_CallMethod(name, "upper", "");
      PyObject_SetAttr(self, upper, PyLong_FromLong(upb_fielddef_number(f)));
      Py_DECREF(name);
      Py_DECREF(upper);
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
  state->cmessage_type = PyUpb_AddClass(m, &PyUpb_CMessage_Spec);
  state->message_meta_type = (PyTypeObject*)message_meta_type;

  if (!state->cmessage_type || !state->message_meta_type) return false;
  if (PyModule_AddObject(m, "MessageMeta", message_meta_type)) return false;
  state->listfields_cmp_lt =
      PyObject_GetAttrString((PyObject*)state->cmessage_type, "_ListFieldsLessThan");

  PyObject* mod = PyImport_ImportModule("google.protobuf.message");
  if (mod == NULL) return false;

  state->encode_error_class = PyObject_GetAttrString(mod, "EncodeError");
  state->decode_error_class = PyObject_GetAttrString(mod, "DecodeError");
  state->message_class = PyObject_GetAttrString(mod, "Message");
  Py_DECREF(mod);

  PyObject* enum_type_wrapper =
      PyImport_ImportModule("google.protobuf.internal.enum_type_wrapper");
  if (enum_type_wrapper == NULL) return false;

  state->enum_type_wrapper_class =
      PyObject_GetAttrString(enum_type_wrapper, "EnumTypeWrapper");
  Py_DECREF(enum_type_wrapper);

  if (!state->encode_error_class || !state->decode_error_class ||
      !state->message_class || !state->listfields_cmp_lt ||
      !state->enum_type_wrapper_class) {
    return false;
  }

  return true;
}
