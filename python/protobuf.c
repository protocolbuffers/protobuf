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

#include "python/protobuf.h"

#include "python/descriptor.h"
#include "python/descriptor_containers.h"
#include "python/descriptor_pool.h"
#include "python/extension_dict.h"
#include "python/map.h"
#include "python/message.h"
#include "python/repeated.h"
#include "python/unknown_fields.h"

static void PyUpb_ModuleDealloc(void* module) {
  PyUpb_ModuleState* s = PyModule_GetState(module);
  PyUpb_WeakMap_Free(s->obj_cache);
  if (s->c_descriptor_symtab) {
    upb_DefPool_Free(s->c_descriptor_symtab);
  }
}

PyObject* PyUpb_SetAllowOversizeProtos(PyObject* m, PyObject* arg) {
  if (!arg || !PyBool_Check(arg)) {
    PyErr_SetString(PyExc_TypeError,
                    "Argument to SetAllowOversizeProtos must be boolean");
    return NULL;
  }
  PyUpb_ModuleState* state = PyUpb_ModuleState_Get();
  state->allow_oversize_protos = PyObject_IsTrue(arg);
  Py_INCREF(arg);
  return arg;
}

static PyMethodDef PyUpb_ModuleMethods[] = {
    {"SetAllowOversizeProtos", PyUpb_SetAllowOversizeProtos, METH_O,
     "Enable/disable oversize proto parsing."},
    {NULL, NULL}};

static struct PyModuleDef module_def = {PyModuleDef_HEAD_INIT,
                                        PYUPB_MODULE_NAME,
                                        "Protobuf Module",
                                        sizeof(PyUpb_ModuleState),
                                        PyUpb_ModuleMethods,  // m_methods
                                        NULL,                 // m_slots
                                        NULL,                 // m_traverse
                                        NULL,                 // m_clear
                                        PyUpb_ModuleDealloc};

// -----------------------------------------------------------------------------
// ModuleState
// -----------------------------------------------------------------------------

PyUpb_ModuleState* PyUpb_ModuleState_MaybeGet(void) {
  PyObject* module = PyState_FindModule(&module_def);
  return module ? PyModule_GetState(module) : NULL;
}

PyUpb_ModuleState* PyUpb_ModuleState_GetFromModule(PyObject* module) {
  PyUpb_ModuleState* state = PyModule_GetState(module);
  assert(state);
  assert(PyModule_GetDef(module) == &module_def);
  return state;
}

PyUpb_ModuleState* PyUpb_ModuleState_Get(void) {
  PyObject* module = PyState_FindModule(&module_def);
  assert(module);
  return PyUpb_ModuleState_GetFromModule(module);
}

PyObject* PyUpb_GetWktBases(PyUpb_ModuleState* state) {
  if (!state->wkt_bases) {
    PyObject* wkt_module = PyImport_ImportModule(PYUPB_PROTOBUF_INTERNAL_PACKAGE
                                                 ".well_known_types");

    if (wkt_module == NULL) {
      return false;
    }

    state->wkt_bases = PyObject_GetAttrString(wkt_module, "WKTBASES");
    PyObject* m = PyState_FindModule(&module_def);
    // Reparent ownership to m.
    PyModule_AddObject(m, "__internal_wktbases", state->wkt_bases);
    Py_DECREF(wkt_module);
  }

  return state->wkt_bases;
}

// -----------------------------------------------------------------------------
// WeakMap
// -----------------------------------------------------------------------------

struct PyUpb_WeakMap {
  upb_inttable table;
  upb_Arena* arena;
};

PyUpb_WeakMap* PyUpb_WeakMap_New(void) {
  upb_Arena* arena = upb_Arena_New();
  PyUpb_WeakMap* map = upb_Arena_Malloc(arena, sizeof(*map));
  map->arena = arena;
  upb_inttable_init(&map->table, map->arena);
  return map;
}

void PyUpb_WeakMap_Free(PyUpb_WeakMap* map) { upb_Arena_Free(map->arena); }

// To give better entropy in the table key, we shift away low bits that are
// always zero.
static const int PyUpb_PtrShift = (sizeof(void*) == 4) ? 2 : 3;

uintptr_t PyUpb_WeakMap_GetKey(const void* key) {
  uintptr_t n = (uintptr_t)key;
  assert((n & ((1 << PyUpb_PtrShift) - 1)) == 0);
  return n >> PyUpb_PtrShift;
}

void PyUpb_WeakMap_Add(PyUpb_WeakMap* map, const void* key, PyObject* py_obj) {
  upb_inttable_insert(&map->table, PyUpb_WeakMap_GetKey(key),
                      upb_value_ptr(py_obj), map->arena);
}

void PyUpb_WeakMap_Delete(PyUpb_WeakMap* map, const void* key) {
  upb_value val;
  bool removed =
      upb_inttable_remove(&map->table, PyUpb_WeakMap_GetKey(key), &val);
  (void)removed;
  assert(removed);
}

void PyUpb_WeakMap_TryDelete(PyUpb_WeakMap* map, const void* key) {
  upb_inttable_remove(&map->table, PyUpb_WeakMap_GetKey(key), NULL);
}

PyObject* PyUpb_WeakMap_Get(PyUpb_WeakMap* map, const void* key) {
  upb_value val;
  if (upb_inttable_lookup(&map->table, PyUpb_WeakMap_GetKey(key), &val)) {
    PyObject* ret = upb_value_getptr(val);
    Py_INCREF(ret);
    return ret;
  } else {
    return NULL;
  }
}

bool PyUpb_WeakMap_Next(PyUpb_WeakMap* map, const void** key, PyObject** obj,
                        intptr_t* iter) {
  uintptr_t u_key;
  upb_value val;
  if (!upb_inttable_next2(&map->table, &u_key, &val, iter)) return false;
  *key = (void*)(u_key << PyUpb_PtrShift);
  *obj = upb_value_getptr(val);
  return true;
}

void PyUpb_WeakMap_DeleteIter(PyUpb_WeakMap* map, intptr_t* iter) {
  upb_inttable_removeiter(&map->table, iter);
}

// -----------------------------------------------------------------------------
// ObjCache
// -----------------------------------------------------------------------------

PyUpb_WeakMap* PyUpb_ObjCache_Instance(void) {
  PyUpb_ModuleState* state = PyUpb_ModuleState_Get();
  return state->obj_cache;
}

void PyUpb_ObjCache_Add(const void* key, PyObject* py_obj) {
  PyUpb_WeakMap_Add(PyUpb_ObjCache_Instance(), key, py_obj);
}

void PyUpb_ObjCache_Delete(const void* key) {
  PyUpb_ModuleState* state = PyUpb_ModuleState_MaybeGet();
  if (!state) {
    // During the shutdown sequence, our object's Dealloc() methods can be
    // called *after* our module Dealloc() method has been called.  At that
    // point our state will be NULL and there is nothing to delete out of the
    // map.
    return;
  }
  PyUpb_WeakMap_Delete(state->obj_cache, key);
}

PyObject* PyUpb_ObjCache_Get(const void* key) {
  return PyUpb_WeakMap_Get(PyUpb_ObjCache_Instance(), key);
}

// -----------------------------------------------------------------------------
// Arena
// -----------------------------------------------------------------------------

typedef struct {
  PyObject_HEAD;
  upb_Arena* arena;
} PyUpb_Arena;

PyObject* PyUpb_Arena_New(void) {
  PyUpb_ModuleState* state = PyUpb_ModuleState_Get();
  PyUpb_Arena* arena = (void*)PyType_GenericAlloc(state->arena_type, 0);
  arena->arena = upb_Arena_New();
  return &arena->ob_base;
}

static void PyUpb_Arena_Dealloc(PyObject* self) {
  upb_Arena_Free(PyUpb_Arena_Get(self));
  PyUpb_Dealloc(self);
}

upb_Arena* PyUpb_Arena_Get(PyObject* arena) {
  return ((PyUpb_Arena*)arena)->arena;
}

static PyType_Slot PyUpb_Arena_Slots[] = {
    {Py_tp_dealloc, PyUpb_Arena_Dealloc},
    {0, NULL},
};

static PyType_Spec PyUpb_Arena_Spec = {
    PYUPB_MODULE_NAME ".Arena",
    sizeof(PyUpb_Arena),
    0,  // itemsize
    Py_TPFLAGS_DEFAULT,
    PyUpb_Arena_Slots,
};

static bool PyUpb_InitArena(PyObject* m) {
  PyUpb_ModuleState* state = PyUpb_ModuleState_GetFromModule(m);
  state->arena_type = PyUpb_AddClass(m, &PyUpb_Arena_Spec);
  return state->arena_type;
}

// -----------------------------------------------------------------------------
// Utilities
// -----------------------------------------------------------------------------

PyTypeObject* AddObject(PyObject* m, const char* name, PyType_Spec* spec) {
  PyObject* type = PyType_FromSpec(spec);
  return type && PyModule_AddObject(m, name, type) == 0 ? (PyTypeObject*)type
                                                        : NULL;
}

static const char* PyUpb_GetClassName(PyType_Spec* spec) {
  // spec->name contains a fully-qualified name, like:
  //   google.protobuf.pyext._message.FooBar
  //
  // Find the rightmost '.' to get "FooBar".
  const char* name = strrchr(spec->name, '.');
  assert(name);
  return name + 1;
}

PyTypeObject* PyUpb_AddClass(PyObject* m, PyType_Spec* spec) {
  PyObject* type = PyType_FromSpec(spec);
  const char* name = PyUpb_GetClassName(spec);
  if (PyModule_AddObject(m, name, type) < 0) {
    Py_XDECREF(type);
    return NULL;
  }
  return (PyTypeObject*)type;
}

PyTypeObject* PyUpb_AddClassWithBases(PyObject* m, PyType_Spec* spec,
                                      PyObject* bases) {
  PyObject* type = PyType_FromSpecWithBases(spec, bases);
  const char* name = PyUpb_GetClassName(spec);
  if (PyModule_AddObject(m, name, type) < 0) {
    Py_XDECREF(type);
    return NULL;
  }
  return (PyTypeObject*)type;
}

const char* PyUpb_GetStrData(PyObject* obj) {
  if (PyUnicode_Check(obj)) {
    return PyUnicode_AsUTF8AndSize(obj, NULL);
  } else if (PyBytes_Check(obj)) {
    return PyBytes_AsString(obj);
  } else {
    return NULL;
  }
}

const char* PyUpb_VerifyStrData(PyObject* obj) {
  const char* ret = PyUpb_GetStrData(obj);
  if (ret) return ret;
  PyErr_Format(PyExc_TypeError, "Expected string: %S", obj);
  return NULL;
}

PyObject* PyUpb_Forbidden_New(PyObject* cls, PyObject* args, PyObject* kwds) {
  PyObject* name = PyObject_GetAttrString(cls, "__name__");
  PyErr_Format(PyExc_RuntimeError,
               "Objects of type %U may not be created directly.", name);
  Py_XDECREF(name);
  return NULL;
}

// -----------------------------------------------------------------------------
// Module Entry Point
// -----------------------------------------------------------------------------

__attribute__((visibility("default"))) PyMODINIT_FUNC PyInit__message(void) {
  PyObject* m = PyModule_Create(&module_def);
  if (!m) return NULL;

  PyUpb_ModuleState* state = PyUpb_ModuleState_GetFromModule(m);

  state->allow_oversize_protos = false;
  state->wkt_bases = NULL;
  state->obj_cache = PyUpb_WeakMap_New();
  state->c_descriptor_symtab = NULL;

  if (!PyUpb_InitDescriptorContainers(m) || !PyUpb_InitDescriptorPool(m) ||
      !PyUpb_InitDescriptor(m) || !PyUpb_InitArena(m) ||
      !PyUpb_InitExtensionDict(m) || !PyUpb_Map_Init(m) ||
      !PyUpb_InitMessage(m) || !PyUpb_Repeated_Init(m) ||
      !PyUpb_UnknownFields_Init(m)) {
    Py_DECREF(m);
    return NULL;
  }

  // Temporary: an cookie we can use in the tests to ensure we are testing upb
  // and not another protobuf library on the system.
  PyModule_AddIntConstant(m, "_IS_UPB", 1);

  return m;
}
