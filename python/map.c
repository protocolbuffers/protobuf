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

#include "python/map.h"

#include "python/convert.h"
#include "python/message.h"
#include "python/protobuf.h"
#include "upb/map.h"

// -----------------------------------------------------------------------------
// MapContainer
// -----------------------------------------------------------------------------

typedef struct {
  PyObject_HEAD;
  PyObject* arena;
  // The field descriptor (upb_FieldDef*).
  // The low bit indicates whether the container is reified (see ptr below).
  //   - low bit set: repeated field is a stub (empty map, no underlying data).
  //   - low bit clear: repeated field is reified (points to upb_Array).
  uintptr_t field;
  union {
    PyObject* parent;  // stub: owning pointer to parent message.
    upb_Map* map;      // reified: the data for this array.
  } ptr;
  int version;
} PyUpb_MapContainer;

static PyObject* PyUpb_MapIterator_New(PyUpb_MapContainer* map);

static bool PyUpb_MapContainer_IsStub(PyUpb_MapContainer* self) {
  return self->field & 1;
}

// If the map is reified, returns it.  Otherwise, returns NULL.
// If NULL is returned, the object is empty and has no underlying data.
static upb_Map* PyUpb_MapContainer_GetIfReified(PyUpb_MapContainer* self) {
  return PyUpb_MapContainer_IsStub(self) ? NULL : self->ptr.map;
}

static const upb_FieldDef* PyUpb_MapContainer_GetField(
    PyUpb_MapContainer* self) {
  return (const upb_FieldDef*)(self->field & ~(uintptr_t)1);
}

static void PyUpb_MapContainer_Dealloc(void* _self) {
  PyUpb_MapContainer* self = _self;
  Py_DECREF(self->arena);
  if (PyUpb_MapContainer_IsStub(self)) {
    PyUpb_Message_CacheDelete(self->ptr.parent,
                              PyUpb_MapContainer_GetField(self));
    Py_DECREF(self->ptr.parent);
  } else {
    PyUpb_ObjCache_Delete(self->ptr.map);
  }
  PyUpb_Dealloc(_self);
}

PyTypeObject* PyUpb_MapContainer_GetClass(const upb_FieldDef* f) {
  assert(upb_FieldDef_IsMap(f));
  PyUpb_ModuleState* state = PyUpb_ModuleState_Get();
  return upb_FieldDef_IsSubMessage(f) ? state->message_map_container_type
                                      : state->scalar_map_container_type;
}

PyObject* PyUpb_MapContainer_NewStub(PyObject* parent, const upb_FieldDef* f,
                                     PyObject* arena) {
  // We only create stubs when the parent is reified, by convention.  However
  // this is not an invariant: the parent could become reified at any time.
  assert(PyUpb_Message_GetIfReified(parent) == NULL);
  PyTypeObject* cls = PyUpb_MapContainer_GetClass(f);
  PyUpb_MapContainer* map = (void*)PyType_GenericAlloc(cls, 0);
  map->arena = arena;
  map->field = (uintptr_t)f | 1;
  map->ptr.parent = parent;
  map->version = 0;
  Py_INCREF(arena);
  Py_INCREF(parent);
  return &map->ob_base;
}

void PyUpb_MapContainer_Reify(PyObject* _self, upb_Map* map) {
  PyUpb_MapContainer* self = (PyUpb_MapContainer*)_self;
  if (!map) {
    const upb_FieldDef* f = PyUpb_MapContainer_GetField(self);
    upb_Arena* arena = PyUpb_Arena_Get(self->arena);
    const upb_MessageDef* entry_m = upb_FieldDef_MessageSubDef(f);
    const upb_FieldDef* key_f = upb_MessageDef_Field(entry_m, 0);
    const upb_FieldDef* val_f = upb_MessageDef_Field(entry_m, 1);
    map = upb_Map_New(arena, upb_FieldDef_CType(key_f),
                      upb_FieldDef_CType(val_f));
  }
  PyUpb_ObjCache_Add(map, &self->ob_base);
  Py_DECREF(self->ptr.parent);
  self->ptr.map = map;  // Overwrites self->ptr.parent.
  self->field &= ~(uintptr_t)1;
  assert(!PyUpb_MapContainer_IsStub(self));
}

void PyUpb_MapContainer_Invalidate(PyObject* obj) {
  PyUpb_MapContainer* self = (PyUpb_MapContainer*)obj;
  self->version++;
}

upb_Map* PyUpb_MapContainer_EnsureReified(PyObject* _self) {
  PyUpb_MapContainer* self = (PyUpb_MapContainer*)_self;
  self->version++;
  upb_Map* map = PyUpb_MapContainer_GetIfReified(self);
  if (map) return map;  // Already writable.

  const upb_FieldDef* f = PyUpb_MapContainer_GetField(self);
  upb_Arena* arena = PyUpb_Arena_Get(self->arena);
  const upb_MessageDef* entry_m = upb_FieldDef_MessageSubDef(f);
  const upb_FieldDef* key_f = upb_MessageDef_Field(entry_m, 0);
  const upb_FieldDef* val_f = upb_MessageDef_Field(entry_m, 1);
  map =
      upb_Map_New(arena, upb_FieldDef_CType(key_f), upb_FieldDef_CType(val_f));
  upb_MessageValue msgval = {.map_val = map};
  PyUpb_Message_SetConcreteSubobj(self->ptr.parent, f, msgval);
  PyUpb_MapContainer_Reify((PyObject*)self, map);
  return map;
}

bool PyUpb_MapContainer_Set(PyUpb_MapContainer* self, upb_Map* map,
                            upb_MessageValue key, upb_MessageValue val,
                            upb_Arena* arena) {
  switch (upb_Map_Insert(map, key, val, arena)) {
    case kUpb_MapInsertStatus_Inserted:
      return true;
    case kUpb_MapInsertStatus_Replaced:
      // We did not insert a new key, undo the previous invalidate.
      self->version--;
      return true;
    case kUpb_MapInsertStatus_OutOfMemory:
      return false;
  }
  return false;  // Unreachable, silence compiler warning.
}

int PyUpb_MapContainer_AssignSubscript(PyObject* _self, PyObject* key,
                                       PyObject* val) {
  PyUpb_MapContainer* self = (PyUpb_MapContainer*)_self;
  upb_Map* map = PyUpb_MapContainer_EnsureReified(_self);
  const upb_FieldDef* f = PyUpb_MapContainer_GetField(self);
  const upb_MessageDef* entry_m = upb_FieldDef_MessageSubDef(f);
  const upb_FieldDef* key_f = upb_MessageDef_Field(entry_m, 0);
  const upb_FieldDef* val_f = upb_MessageDef_Field(entry_m, 1);
  upb_Arena* arena = PyUpb_Arena_Get(self->arena);
  upb_MessageValue u_key, u_val;
  if (!PyUpb_PyToUpb(key, key_f, &u_key, arena)) return -1;

  if (val) {
    if (!PyUpb_PyToUpb(val, val_f, &u_val, arena)) return -1;
    if (!PyUpb_MapContainer_Set(self, map, u_key, u_val, arena)) return -1;
  } else {
    if (!upb_Map_Delete(map, u_key)) {
      PyErr_Format(PyExc_KeyError, "Key not present in map");
      return -1;
    }
  }
  return 0;
}

PyObject* PyUpb_MapContainer_Subscript(PyObject* _self, PyObject* key) {
  PyUpb_MapContainer* self = (PyUpb_MapContainer*)_self;
  upb_Map* map = PyUpb_MapContainer_GetIfReified(self);
  const upb_FieldDef* f = PyUpb_MapContainer_GetField(self);
  const upb_MessageDef* entry_m = upb_FieldDef_MessageSubDef(f);
  const upb_FieldDef* key_f = upb_MessageDef_Field(entry_m, 0);
  const upb_FieldDef* val_f = upb_MessageDef_Field(entry_m, 1);
  upb_Arena* arena = PyUpb_Arena_Get(self->arena);
  upb_MessageValue u_key, u_val;
  if (!PyUpb_PyToUpb(key, key_f, &u_key, arena)) return NULL;
  if (!map || !upb_Map_Get(map, u_key, &u_val)) {
    map = PyUpb_MapContainer_EnsureReified(_self);
    upb_Arena* arena = PyUpb_Arena_Get(self->arena);
    if (upb_FieldDef_IsSubMessage(val_f)) {
      u_val.msg_val = upb_Message_New(upb_FieldDef_MessageSubDef(val_f), arena);
    } else {
      memset(&u_val, 0, sizeof(u_val));
    }
    if (!PyUpb_MapContainer_Set(self, map, u_key, u_val, arena)) return false;
  }
  return PyUpb_UpbToPy(u_val, val_f, self->arena);
}

PyObject* PyUpb_MapContainer_Contains(PyObject* _self, PyObject* key) {
  PyUpb_MapContainer* self = (PyUpb_MapContainer*)_self;
  upb_Map* map = PyUpb_MapContainer_GetIfReified(self);
  if (!map) Py_RETURN_FALSE;
  const upb_FieldDef* f = PyUpb_MapContainer_GetField(self);
  const upb_MessageDef* entry_m = upb_FieldDef_MessageSubDef(f);
  const upb_FieldDef* key_f = upb_MessageDef_Field(entry_m, 0);
  upb_MessageValue u_key;
  if (!PyUpb_PyToUpb(key, key_f, &u_key, NULL)) return NULL;
  if (upb_Map_Get(map, u_key, NULL)) {
    Py_RETURN_TRUE;
  } else {
    Py_RETURN_FALSE;
  }
}

PyObject* PyUpb_MapContainer_Clear(PyObject* _self, PyObject* key) {
  upb_Map* map = PyUpb_MapContainer_EnsureReified(_self);
  upb_Map_Clear(map);
  Py_RETURN_NONE;
}

static PyObject* PyUpb_MapContainer_Get(PyObject* _self, PyObject* args,
                                        PyObject* kwargs) {
  PyUpb_MapContainer* self = (PyUpb_MapContainer*)_self;
  static const char* kwlist[] = {"key", "default", NULL};
  PyObject* key;
  PyObject* default_value = NULL;
  upb_Map* map = PyUpb_MapContainer_GetIfReified(self);
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O|O", (char**)kwlist, &key,
                                   &default_value)) {
    return NULL;
  }

  const upb_FieldDef* f = PyUpb_MapContainer_GetField(self);
  const upb_MessageDef* entry_m = upb_FieldDef_MessageSubDef(f);
  const upb_FieldDef* key_f = upb_MessageDef_Field(entry_m, 0);
  const upb_FieldDef* val_f = upb_MessageDef_Field(entry_m, 1);
  upb_Arena* arena = PyUpb_Arena_Get(self->arena);
  upb_MessageValue u_key, u_val;
  if (!PyUpb_PyToUpb(key, key_f, &u_key, arena)) return NULL;
  if (map && upb_Map_Get(map, u_key, &u_val)) {
    return PyUpb_UpbToPy(u_val, val_f, self->arena);
  }
  if (default_value) {
    Py_INCREF(default_value);
    return default_value;
  }
  Py_RETURN_NONE;
}

static PyObject* PyUpb_MapContainer_GetEntryClass(PyObject* _self,
                                                  PyObject* arg) {
  PyUpb_MapContainer* self = (PyUpb_MapContainer*)_self;
  const upb_FieldDef* f = PyUpb_MapContainer_GetField(self);
  const upb_MessageDef* entry_m = upb_FieldDef_MessageSubDef(f);
  return PyUpb_Descriptor_GetClass(entry_m);
}

Py_ssize_t PyUpb_MapContainer_Length(PyObject* _self) {
  PyUpb_MapContainer* self = (PyUpb_MapContainer*)_self;
  upb_Map* map = PyUpb_MapContainer_GetIfReified(self);
  return map ? upb_Map_Size(map) : 0;
}

PyUpb_MapContainer* PyUpb_MapContainer_Check(PyObject* _self) {
  PyUpb_ModuleState* state = PyUpb_ModuleState_Get();
  if (!PyObject_TypeCheck(_self, state->message_map_container_type) &&
      !PyObject_TypeCheck(_self, state->scalar_map_container_type)) {
    PyErr_Format(PyExc_TypeError, "Expected protobuf map, but got %R", _self);
    return NULL;
  }
  return (PyUpb_MapContainer*)_self;
}

int PyUpb_Message_InitMapAttributes(PyObject* map, PyObject* value,
                                    const upb_FieldDef* f);

static PyObject* PyUpb_MapContainer_MergeFrom(PyObject* _self, PyObject* _arg) {
  PyUpb_MapContainer* self = (PyUpb_MapContainer*)_self;
  const upb_FieldDef* f = PyUpb_MapContainer_GetField(self);

  if (PyDict_Check(_arg)) {
    return PyErr_Format(PyExc_AttributeError, "Merging of dict is not allowed");
  }

  if (PyUpb_Message_InitMapAttributes(_self, _arg, f) < 0) {
    return NULL;
  }

  Py_RETURN_NONE;
}

static PyObject* PyUpb_MapContainer_Repr(PyObject* _self) {
  PyUpb_MapContainer* self = (PyUpb_MapContainer*)_self;
  upb_Map* map = PyUpb_MapContainer_GetIfReified(self);
  PyObject* dict = PyDict_New();
  if (map) {
    const upb_FieldDef* f = PyUpb_MapContainer_GetField(self);
    const upb_MessageDef* entry_m = upb_FieldDef_MessageSubDef(f);
    const upb_FieldDef* key_f = upb_MessageDef_Field(entry_m, 0);
    const upb_FieldDef* val_f = upb_MessageDef_Field(entry_m, 1);
    size_t iter = kUpb_Map_Begin;
    while (upb_MapIterator_Next(map, &iter)) {
      PyObject* key =
          PyUpb_UpbToPy(upb_MapIterator_Key(map, iter), key_f, self->arena);
      PyObject* val =
          PyUpb_UpbToPy(upb_MapIterator_Value(map, iter), val_f, self->arena);
      if (!key || !val) {
        Py_XDECREF(key);
        Py_XDECREF(val);
        Py_DECREF(dict);
        return NULL;
      }
      PyDict_SetItem(dict, key, val);
      Py_DECREF(key);
      Py_DECREF(val);
    }
  }
  PyObject* repr = PyObject_Repr(dict);
  Py_DECREF(dict);
  return repr;
}

PyObject* PyUpb_MapContainer_GetOrCreateWrapper(upb_Map* map,
                                                const upb_FieldDef* f,
                                                PyObject* arena) {
  PyUpb_MapContainer* ret = (void*)PyUpb_ObjCache_Get(map);
  if (ret) return &ret->ob_base;

  PyTypeObject* cls = PyUpb_MapContainer_GetClass(f);
  ret = (void*)PyType_GenericAlloc(cls, 0);
  ret->arena = arena;
  ret->field = (uintptr_t)f;
  ret->ptr.map = map;
  ret->version = 0;
  Py_INCREF(arena);
  PyUpb_ObjCache_Add(map, &ret->ob_base);
  return &ret->ob_base;
}

// -----------------------------------------------------------------------------
// ScalarMapContainer
// -----------------------------------------------------------------------------

static PyMethodDef PyUpb_ScalarMapContainer_Methods[] = {
    {"__contains__", PyUpb_MapContainer_Contains, METH_O,
     "Tests whether a key is a member of the map."},
    {"clear", PyUpb_MapContainer_Clear, METH_NOARGS,
     "Removes all elements from the map."},
    {"get", (PyCFunction)PyUpb_MapContainer_Get, METH_VARARGS | METH_KEYWORDS,
     "Gets the value for the given key if present, or otherwise a default"},
    {"GetEntryClass", PyUpb_MapContainer_GetEntryClass, METH_NOARGS,
     "Return the class used to build Entries of (key, value) pairs."},
    {"MergeFrom", PyUpb_MapContainer_MergeFrom, METH_O,
     "Merges a map into the current map."},
    /*
   { "__deepcopy__", (PyCFunction)DeepCopy, METH_VARARGS,
     "Makes a deep copy of the class." },
   { "__reduce__", (PyCFunction)Reduce, METH_NOARGS,
     "Outputs picklable representation of the repeated field." },
   */
    {NULL, NULL},
};

static PyType_Slot PyUpb_ScalarMapContainer_Slots[] = {
    {Py_tp_dealloc, PyUpb_MapContainer_Dealloc},
    {Py_mp_length, PyUpb_MapContainer_Length},
    {Py_mp_subscript, PyUpb_MapContainer_Subscript},
    {Py_mp_ass_subscript, PyUpb_MapContainer_AssignSubscript},
    {Py_tp_methods, PyUpb_ScalarMapContainer_Methods},
    {Py_tp_iter, PyUpb_MapIterator_New},
    {Py_tp_repr, PyUpb_MapContainer_Repr},
    {Py_tp_hash, PyObject_HashNotImplemented},
    {0, NULL},
};

static PyType_Spec PyUpb_ScalarMapContainer_Spec = {
    PYUPB_MODULE_NAME ".ScalarMapContainer",
    sizeof(PyUpb_MapContainer),
    0,
    Py_TPFLAGS_DEFAULT,
    PyUpb_ScalarMapContainer_Slots,
};

// -----------------------------------------------------------------------------
// MessageMapContainer
// -----------------------------------------------------------------------------

static PyMethodDef PyUpb_MessageMapContainer_Methods[] = {
    {"__contains__", PyUpb_MapContainer_Contains, METH_O,
     "Tests whether the map contains this element."},
    {"clear", PyUpb_MapContainer_Clear, METH_NOARGS,
     "Removes all elements from the map."},
    {"get", (PyCFunction)PyUpb_MapContainer_Get, METH_VARARGS | METH_KEYWORDS,
     "Gets the value for the given key if present, or otherwise a default"},
    {"get_or_create", PyUpb_MapContainer_Subscript, METH_O,
     "Alias for getitem, useful to make explicit that the map is mutated."},
    {"GetEntryClass", PyUpb_MapContainer_GetEntryClass, METH_NOARGS,
     "Return the class used to build Entries of (key, value) pairs."},
    {"MergeFrom", PyUpb_MapContainer_MergeFrom, METH_O,
     "Merges a map into the current map."},
    /*
   { "__deepcopy__", (PyCFunction)DeepCopy, METH_VARARGS,
     "Makes a deep copy of the class." },
   { "__reduce__", (PyCFunction)Reduce, METH_NOARGS,
     "Outputs picklable representation of the repeated field." },
   */
    {NULL, NULL},
};

static PyType_Slot PyUpb_MessageMapContainer_Slots[] = {
    {Py_tp_dealloc, PyUpb_MapContainer_Dealloc},
    {Py_mp_length, PyUpb_MapContainer_Length},
    {Py_mp_subscript, PyUpb_MapContainer_Subscript},
    {Py_mp_ass_subscript, PyUpb_MapContainer_AssignSubscript},
    {Py_tp_methods, PyUpb_MessageMapContainer_Methods},
    {Py_tp_iter, PyUpb_MapIterator_New},
    {Py_tp_repr, PyUpb_MapContainer_Repr},
    {Py_tp_hash, PyObject_HashNotImplemented},
    {0, NULL}};

static PyType_Spec PyUpb_MessageMapContainer_Spec = {
    PYUPB_MODULE_NAME ".MessageMapContainer", sizeof(PyUpb_MapContainer), 0,
    Py_TPFLAGS_DEFAULT, PyUpb_MessageMapContainer_Slots};

// -----------------------------------------------------------------------------
// MapIterator
// -----------------------------------------------------------------------------

typedef struct {
  PyObject_HEAD;
  PyUpb_MapContainer* map;  // We own a reference.
  size_t iter;
  int version;
} PyUpb_MapIterator;

static PyObject* PyUpb_MapIterator_New(PyUpb_MapContainer* map) {
  PyUpb_ModuleState* state = PyUpb_ModuleState_Get();
  PyUpb_MapIterator* iter =
      (void*)PyType_GenericAlloc(state->map_iterator_type, 0);
  iter->map = map;
  iter->iter = kUpb_Map_Begin;
  iter->version = map->version;
  Py_INCREF(map);
  return &iter->ob_base;
}

static void PyUpb_MapIterator_Dealloc(void* _self) {
  PyUpb_MapIterator* self = (PyUpb_MapIterator*)_self;
  Py_DECREF(&self->map->ob_base);
  PyUpb_Dealloc(_self);
}

PyObject* PyUpb_MapIterator_IterNext(PyObject* _self) {
  PyUpb_MapIterator* self = (PyUpb_MapIterator*)_self;
  if (self->version != self->map->version) {
    return PyErr_Format(PyExc_RuntimeError, "Map modified during iteration.");
  }
  upb_Map* map = PyUpb_MapContainer_GetIfReified(self->map);
  if (!map) return NULL;
  if (!upb_MapIterator_Next(map, &self->iter)) return NULL;
  upb_MessageValue key = upb_MapIterator_Key(map, self->iter);
  const upb_FieldDef* f = PyUpb_MapContainer_GetField(self->map);
  const upb_MessageDef* entry_m = upb_FieldDef_MessageSubDef(f);
  const upb_FieldDef* key_f = upb_MessageDef_Field(entry_m, 0);
  return PyUpb_UpbToPy(key, key_f, self->map->arena);
}

static PyType_Slot PyUpb_MapIterator_Slots[] = {
    {Py_tp_dealloc, PyUpb_MapIterator_Dealloc},
    {Py_tp_iter, PyObject_SelfIter},
    {Py_tp_iternext, PyUpb_MapIterator_IterNext},
    {0, NULL}};

static PyType_Spec PyUpb_MapIterator_Spec = {
    PYUPB_MODULE_NAME ".MapIterator", sizeof(PyUpb_MapIterator), 0,
    Py_TPFLAGS_DEFAULT, PyUpb_MapIterator_Slots};

// -----------------------------------------------------------------------------
// Top Level
// -----------------------------------------------------------------------------

static PyObject* GetMutableMappingBase(void) {
  PyObject* collections = NULL;
  PyObject* mapping = NULL;
  PyObject* bases = NULL;
  if ((collections = PyImport_ImportModule("collections.abc")) &&
      (mapping = PyObject_GetAttrString(collections, "MutableMapping"))) {
    bases = Py_BuildValue("(O)", mapping);
  }
  Py_XDECREF(collections);
  Py_XDECREF(mapping);
  return bases;
}

bool PyUpb_Map_Init(PyObject* m) {
  PyUpb_ModuleState* state = PyUpb_ModuleState_GetFromModule(m);
  PyObject* bases = GetMutableMappingBase();
  if (!bases) return false;

  state->message_map_container_type =
      PyUpb_AddClassWithBases(m, &PyUpb_MessageMapContainer_Spec, bases);
  state->scalar_map_container_type =
      PyUpb_AddClassWithBases(m, &PyUpb_ScalarMapContainer_Spec, bases);
  state->map_iterator_type = PyUpb_AddClass(m, &PyUpb_MapIterator_Spec);

  Py_DECREF(bases);

  return state->message_map_container_type &&
         state->scalar_map_container_type && state->map_iterator_type;
}
