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

#include "protobuf.h"

#include "descriptor.h"
#include "descriptor_pool.h"

static void PyUpb_ModuleDealloc(void *module) {
  PyUpb_ModuleState *s = PyModule_GetState(module);
  upb_arena_free(s->obj_cache_arena);
}

static struct PyModuleDef module_def = {PyModuleDef_HEAD_INIT,
                                        PYUPB_MODULE_NAME,
                                        "Protobuf Module",
                                        sizeof(PyUpb_ModuleState),
                                        NULL,  // m_methods
                                        NULL,  // m_slots
                                        NULL,  // m_traverse
                                        NULL,  // m_clear
                                        PyUpb_ModuleDealloc};

// -----------------------------------------------------------------------------
// ModuleState
// -----------------------------------------------------------------------------

PyUpb_ModuleState *PyUpb_ModuleState_Get() {
  PyObject *module = PyState_FindModule(&module_def);
  return PyModule_GetState(module);
}

// -----------------------------------------------------------------------------
// ObjectCache
// -----------------------------------------------------------------------------

void PyUpb_ObjCache_Add(const void *key, PyObject *py_obj) {
  PyUpb_ModuleState *s = PyUpb_ModuleState_Get();
  upb_inttable_insert(&s->obj_cache, (uintptr_t)key, upb_value_ptr(py_obj),
                      s->obj_cache_arena);
}

void PyUpb_ObjCache_Delete(const void *key) {
  PyUpb_ModuleState *s = PyUpb_ModuleState_Get();
  upb_value val;
  upb_inttable_remove(&s->obj_cache, (uintptr_t)key, &val);
  assert(upb_value_getptr(val));
}

PyObject *PyUpb_ObjCache_Get(const void *key) {
  PyUpb_ModuleState *s = PyUpb_ModuleState_Get();
  upb_value val;
  if (upb_inttable_lookup(&s->obj_cache, (uintptr_t)key, &val)) {
    PyObject *ret = upb_value_getptr(val);
    Py_INCREF(ret);
    return ret;
  } else {
    return NULL;
  }
}

// -----------------------------------------------------------------------------
// Utilities
// -----------------------------------------------------------------------------

PyTypeObject *AddObject(PyObject *m, const char *name, PyType_Spec *spec) {
  PyObject *type = PyType_FromSpec(spec);
  return type && PyModule_AddObject(m, name, type) == 0 ? (PyTypeObject *)type
                                                        : NULL;
}

const char *PyUpb_GetStrData(PyObject *obj) {
  if (PyUnicode_Check(obj)) {
    return PyUnicode_AsUTF8AndSize(obj, NULL);
  } else if (PyBytes_Check(obj)) {
    return PyBytes_AsString(obj);
  } else {
    return NULL;
  }
}

// -----------------------------------------------------------------------------
// Module Entry Point
// -----------------------------------------------------------------------------

PyMODINIT_FUNC PyInit__message(void) {
  PyObject *m = PyModule_Create(&module_def);
  PyState_AddModule(m, &module_def);
  PyUpb_ModuleState *state = PyUpb_ModuleState_Get();

  state->obj_cache_arena = upb_arena_new();
  upb_inttable_init(&state->obj_cache, state->obj_cache_arena);

  if (!PyUpb_InitDescriptorPool(m) || !PyUpb_InitDescriptor(m)) {
    Py_DECREF(m);
    return NULL;
  }

  // Temporary: an cookie we can use in the tests to ensure we are testing upb
  // and not another protobuf library on the system.
  PyModule_AddObject(m, "_IS_UPB", Py_True);

  return m;
}
