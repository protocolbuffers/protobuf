// Protocol Buffers - Google's data interchange format
// Copyright 2026 Google LLC. All rights reserved.
// Use of this source code is governed by a BSD-style license in LICENSE.
// Test-only extension: never linked into the protobuf distribution.
#include "Python.h"
#include <stdint.h>
#include "python/free_threading/weak_map.h"
#include "upb/mem/alloc.h"

static void FreeMap(PyObject* capsule) {
  PyUpb_WeakMap_Free(PyCapsule_GetPointer(capsule, "weak-map-test"));
}
static PyUpb_WeakMap* GetMap(PyObject* capsule) {
  return PyCapsule_GetPointer(capsule, "weak-map-test");
}
static PyObject* NewMap(PyObject* self, PyObject* unused) {
  PyUpb_WeakMap* map = PyUpb_WeakMap_New();
  if (!map) return NULL;
  PyObject* capsule = PyCapsule_New(map, "weak-map-test", FreeMap);
  if (!capsule) PyUpb_WeakMap_Free(map);
  return capsule;
}
static PyObject* Count(PyObject* self, PyObject* capsule) {
  PyUpb_WeakMap* map = GetMap(capsule);
  if (!map) return NULL;
  return PyLong_FromSize_t(PyUpb_WeakMap_Count(map));
}
static PyObject* Add(PyObject* self, PyObject* args) {
  PyObject *capsule, *obj;
  unsigned long key;
  if (!PyArg_ParseTuple(args, "OkO", &capsule, &key, &obj)) return NULL;
  PyUpb_WeakMap* map = GetMap(capsule);
  if (!map) return NULL;
  Py_INCREF(obj);
  bool ok = PyUpb_WeakMap_Add(map, (const void*)(uintptr_t)(key * 8), &obj);
  Py_DECREF(obj);
  if (!ok) return NULL;
  Py_RETURN_NONE;
}
static PyObject* Erase(PyObject* self, PyObject* args) {
  PyObject *capsule, *obj;
  unsigned long key;
  if (!PyArg_ParseTuple(args, "OkO", &capsule, &key, &obj)) return NULL;
  PyUpb_WeakMap* map = GetMap(capsule);
  if (!map) return NULL;
  return PyBool_FromLong(PyUpb_WeakMap_EraseIfEqual(
      map, (const void*)(uintptr_t)(key * 8), obj));
}
static PyObject* Iterate(PyObject* self, PyObject* args) {
  PyObject *capsule, *callback;
  if (!PyArg_ParseTuple(args, "OO", &capsule, &callback)) return NULL;
  PyUpb_WeakMap* map = GetMap(capsule);
  if (!map) return NULL;
  PyUpb_WeakMapIter iter;
  if (!PyUpb_WeakMapIter_Begin(&iter, map)) return NULL;
  const void* key;
  PyObject* obj;
  PyObject* result = NULL;
  if (PyUpb_WeakMapIter_Next(&iter, &key, &obj)) {
    // Model reentry from exception construction/retirement in a consumer.
    result = PyObject_CallNoArgs(callback);
    if (result) PyUpb_WeakMapIter_Delete(&iter);
  } else {
    result = Py_NewRef(Py_None);
  }
  PyUpb_WeakMapIter_End(&iter);
  return result;
}
static upb_alloc_func* saved_alloc;
static void* FailAllocation(upb_alloc* alloc, void* ptr, size_t oldsize,
                            size_t size, size_t* actual_size) {
  if (size) return NULL;
  return saved_alloc(alloc, ptr, oldsize, size, actual_size);
}
static PyObject* FailAdd(PyObject* self, PyObject* args) {
  PyObject *capsule, *obj, *factory;
  if (!PyArg_ParseTuple(args, "OOO", &capsule, &obj, &factory)) return NULL;
  PyUpb_WeakMap* map = GetMap(capsule);
  if (!map) return NULL;
  PyObject* error = PyObject_CallNoArgs(factory);
  if (!error) return NULL;
  if (!PyExceptionInstance_Check(error)) {
    Py_DECREF(error);
    return PyErr_Format(PyExc_TypeError, "factory must return an exception");
  }
  // Deliberately inject a pending exception whose finalizer reenters this map.
  // This is a native failure-path probe, not a public Python API reproducer.
  PyErr_SetRaisedException(error);
  saved_alloc = upb_alloc_global.func;
  upb_alloc_global.func = FailAllocation;
  bool failed = false;
  for (uintptr_t key = 1000; key < 100000; ++key) {
    PyObject* value = Py_NewRef(obj);
    bool ok = PyUpb_WeakMap_Add(map, (const void*)(key * 8), &value);
    Py_DECREF(value);
    if (!ok) { failed = true; break; }
  }
  upb_alloc_global.func = saved_alloc;
  if (!failed) {
    PyErr_Clear();
    return PyErr_Format(PyExc_AssertionError, "injected allocation did not fail");
  }
  return NULL;
}
static PyObject* FailBegin(PyObject* self, PyObject* args) {
  PyObject *capsule, *factory;
  if (!PyArg_ParseTuple(args, "OO", &capsule, &factory)) return NULL;
  PyUpb_WeakMap* map = GetMap(capsule);
  if (!map) return NULL;
  PyObject* error = PyObject_CallNoArgs(factory);
  if (!error) return NULL;
  if (!PyExceptionInstance_Check(error)) {
    Py_DECREF(error);
    return PyErr_Format(PyExc_TypeError, "factory must return an exception");
  }
  PyErr_SetRaisedException(error);
  saved_alloc = upb_alloc_global.func;
  upb_alloc_global.func = FailAllocation;
  PyUpb_WeakMapIter iter;
  bool ok = PyUpb_WeakMapIter_Begin(&iter, map);
  upb_alloc_global.func = saved_alloc;
  if (ok) {
    PyUpb_WeakMapIter_End(&iter);
    PyErr_Clear();
    return PyErr_Format(PyExc_AssertionError, "snapshot allocation did not fail");
  }
  return NULL;
}
static PyMethodDef methods[] = {
  {"new", NewMap, METH_NOARGS, NULL},
  {"count", Count, METH_O, NULL},
  {"add", Add, METH_VARARGS, NULL},
  {"iterate", Iterate, METH_VARARGS, NULL},
  {"erase", Erase, METH_VARARGS, NULL},
  {"fail_add", FailAdd, METH_VARARGS, NULL},
  {"fail_begin", FailBegin, METH_VARARGS, NULL},
  {NULL, NULL, 0, NULL}
};
static PyModuleDef_Slot slots[] = {
#ifdef Py_GIL_DISABLED
  {Py_mod_gil, Py_MOD_GIL_NOT_USED},
#endif
  {0, NULL}
};
static struct PyModuleDef module = {
  PyModuleDef_HEAD_INIT, "_weak_map_test", NULL, 0, methods, slots,
  NULL, NULL, NULL
};
PyMODINIT_FUNC PyInit__weak_map_test(void) { return PyModuleDef_Init(&module); }
