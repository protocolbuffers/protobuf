// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "python/protobuf.h"

#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "google/protobuf/breaking_changes.h"
#include "python/descriptor.h"
#include "python/descriptor_containers.h"
#include "python/descriptor_pool.h"
#include "python/extension_dict.h"
#include "python/map.h"
#include "python/message.h"
#include "python/repeated.h"
#include "python/unknown_fields.h"
#include "upb/hash/common.h"
#include "upb/hash/int_table.h"
#include "upb/mem/alloc.h"
#include "upb/mem/arena.h"
#include "upb/reflection/def.h"

// Must be last.
#include "upb/port/def.inc"

static void PyUpb_ModuleDealloc(void* module) {
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

PyObject* PyUpb_AllocationCount_IsAvailable(PyObject* self, PyObject* args) {
  if (upb_AllocationCount_IsAvailable()) {
    Py_RETURN_TRUE;
  } else {
    Py_RETURN_FALSE;
  }
}

PyObject* PyUpb_AllocationCount_Get(PyObject* self, PyObject* args) {
  return PyLong_FromSize_t(upb_AllocationCount_Get());
}

PyObject* PyUpb_AllocationCount_Reset(PyObject* self, PyObject* args) {
  upb_AllocationCount_Reset();
  Py_RETURN_NONE;
}

PyObject* PyUpb_AllocationCount_FailOn(PyObject* self, PyObject* arg) {
  Py_ssize_t n = PyNumber_AsSsize_t(arg, PyExc_OverflowError);
  if (n == -1 && PyErr_Occurred()) {
    return NULL;
  }
  if (n < 0) {
    PyErr_SetString(PyExc_ValueError, "FailOn must be non-negative");
    return NULL;
  }
  upb_AllocationCount_FailOn((size_t)n);
  Py_RETURN_NONE;
}

static PyMethodDef PyUpb_ModuleMethods[] = {
    {"SetAllowOversizeProtos", PyUpb_SetAllowOversizeProtos, METH_O,
     "Enable/disable oversize proto parsing."},
    {"_AllocationCount_IsAvailable", PyUpb_AllocationCount_IsAvailable,
     METH_NOARGS, "Returns whether allocation count debugging is available."},
    {"_AllocationCount_Get", PyUpb_AllocationCount_Get, METH_NOARGS,
     "Returns the current allocation.count."},
    {"_AllocationCount_Reset", PyUpb_AllocationCount_Reset, METH_NOARGS,
     "Resets the current allocation count and failure settings."},
    {"_AllocationCount_FailOn", PyUpb_AllocationCount_FailOn, METH_O,
     "Configures allocation failure at the N-th allocation."},
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
#if PY_VERSION_HEX >= 0x030D0000  // >= 3.13
  /* Calling `PyState_FindModule` during interpreter shutdown causes a crash. */
  if (Py_IsFinalizing()) {
    return NULL;
  }
#endif
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
      return NULL;
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
// Arena
// -----------------------------------------------------------------------------

typedef struct {
  // clang-format off
  PyObject_HEAD
  upb_Arena* arena;
  // clang-format on
  PyUpb_WeakMap* obj_cache;
  bool frozen;
} PyUpb_Arena;

#ifdef __GLIBC__
#include <malloc.h>  // malloc_trim()
#endif

// A special allocator that calls malloc_trim() periodically to release
// memory to the OS.  Without this call, we appear to leak memory, at least
// as measured in RSS.
//
// We opt to use this instead of PyMalloc (which would also solve the
// problem) because the latter requires the GIL to be held.  This would make
// our messages unsafe to share with other languages that could free at
// unpredictable
// times.
static void* upb_trim_allocfunc(upb_alloc* alloc, void* ptr, size_t oldsize,
                                size_t size, size_t* actual_size) {
  (void)alloc;
  (void)oldsize;
  if (size == 0) {
    free(ptr);
#ifdef __GLIBC__
    static int count = 0;
    if (++count == 10000) {
      malloc_trim(0);
      count = 0;
    }
#endif
    return NULL;
  } else {
    return realloc(ptr, size);
  }
}
static upb_alloc trim_alloc = {&upb_trim_allocfunc};
static upb_alloc* global_alloc = &trim_alloc;

static upb_Arena* PyUpb_NewArena(void) {
  upb_Arena* arena = upb_Arena_Init(NULL, 0, global_alloc);
  if (!arena) {
    PyErr_SetNone(PyExc_MemoryError);
  }
  return arena;
}

PyObject* PyUpb_Arena_New(void) {
  PyUpb_ModuleState* state = PyUpb_ModuleState_MaybeGet();
  if (!state) {
    PyErr_SetString(PyExc_RuntimeError, "Interpreter is finalizing");
    return NULL;
  }
  PyUpb_Arena* arena = (void*)PyType_GenericAlloc(state->arena_type, 0);
  if (!arena) return NULL;
  arena->arena = PyUpb_NewArena();
  if (!arena->arena) {
    Py_DECREF(arena);
    return NULL;
  }
  arena->obj_cache = PyUpb_WeakMap_New();
  if (!arena->obj_cache) {
    Py_DECREF(arena);
    return NULL;
  }
  arena->frozen = false;
  return &arena->ob_base;
}

static void PyUpb_Arena_Dealloc(PyObject* self) {
  PyUpb_Arena* me = (PyUpb_Arena*)self;
  if (me->obj_cache) {
    PyUpb_WeakMap_Free(me->obj_cache);
  }
  upb_Arena* arena = PyUpb_Arena_Get(self);
  if (arena) {
    upb_Arena_Free(arena);
  }
  PyUpb_Dealloc(self);
}

upb_Arena* PyUpb_Arena_Get(PyObject* arena) {
  return ((PyUpb_Arena*)arena)->arena;
}

bool PyUpb_Arena_IsFrozen(PyObject* arena) {
  return ((PyUpb_Arena*)arena)->frozen;
}

void PyUpb_Arena_SetFrozen(PyObject* arena, bool frozen) {
  ((PyUpb_Arena*)arena)->frozen = frozen;
}

bool PyUpb_Arena_CacheAdd(PyObject* _arena, const void* key,
                          PyObject** py_obj) {
  PyUpb_Arena* arena = (PyUpb_Arena*)_arena;
  return PyUpb_WeakMap_Add(arena->obj_cache, key, py_obj);
}

PyObject* PyUpb_Arena_CacheGet(PyObject* _arena, const void* key) {
  PyUpb_Arena* arena = (PyUpb_Arena*)_arena;
  return PyUpb_WeakMap_Get(arena->obj_cache, key);
}

bool PyUpb_Arena_CacheEraseIfEqual(PyObject* _arena, const void* key,
                                   PyObject* obj) {
  PyUpb_Arena* arena = (PyUpb_Arena*)_arena;
  return PyUpb_WeakMap_EraseIfEqual(arena->obj_cache, key, obj);
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

PyTypeObject* PyUpb_AddClassWithRegister(PyObject* m, PyType_Spec* spec,
                                         PyObject* virtual_base,
                                         const char** methods) {
  PyObject* type = PyType_FromSpec(spec);
  PyObject* ret1 = PyObject_CallMethod(virtual_base, "register", "O", type);
  if (!ret1) {
    Py_XDECREF(type);
    return NULL;
  }
  for (size_t i = 0; methods[i] != NULL; i++) {
    PyObject* method = PyObject_GetAttrString(virtual_base, methods[i]);
    if (!method) {
      Py_XDECREF(type);
      return NULL;
    }
    int ret2 = PyObject_SetAttrString(type, methods[i], method);
    if (ret2 < 0) {
      Py_XDECREF(type);
      return NULL;
    }
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

bool PyUpb_IndexToRange(PyObject* index, Py_ssize_t size, Py_ssize_t* i,
                        Py_ssize_t* count, Py_ssize_t* step) {
  assert(i && count && step);
  if (PySlice_Check(index)) {
    Py_ssize_t start, stop;
    if (PySlice_Unpack(index, &start, &stop, step) < 0) return false;
    *count = PySlice_AdjustIndices(size, &start, &stop, *step);
    *i = start;
  } else {
    *i = PyNumber_AsSsize_t(index, PyExc_IndexError);

    if (*i == -1 && PyErr_Occurred()) {
      PyErr_SetString(PyExc_TypeError, "list indices must be integers");
      return false;
    }

    if (*i < 0) *i += size;
    *step = 0;
    *count = 1;

    if (*i < 0 || size <= *i) {
      PyErr_Format(PyExc_IndexError, "list index out of range");
      return false;
    }
  }
  return true;
}

PyObject* PyUpb_SetFrozenErrorWithMsg(const char* msg) {
  PyUpb_ModuleState* state = PyUpb_ModuleState_Get();
  PyErr_SetString(state->frozen_instance_error_class, msg);
  return NULL;
}

PyObject* PyUpb_SetFrozenError(void) {
  return PyUpb_SetFrozenErrorWithMsg("Message is immutable.");
}

int PyUpb_WarnFrozen(void) {
  return PyErr_WarnEx(
      PyExc_FutureWarning,
      "Mutating messages or containers returned by GetOptions() is deprecated"
      " and will raise an exception in a future release.",
      3);
}

bool PyUpb_CheckFrozen(bool is_frozen, const char* error_msg) {
  if (is_frozen) {
#if PROTOBUF_PY_FUTURE_FREEZE_OPTIONS
    PyUpb_SetFrozenErrorWithMsg(error_msg);
    return false;
#else
    return PyUpb_WarnFrozen() >= 0;
#endif
  }
  return true;
}

// -----------------------------------------------------------------------------
// Module Entry Point
// -----------------------------------------------------------------------------

PyMODINIT_FUNC PyInit__message(void) {
  PyObject* m = PyModule_Create(&module_def);
  if (!m) return NULL;

  PyUpb_ModuleState* state = PyUpb_ModuleState_GetFromModule(m);

  state->allow_oversize_protos = false;
  state->wkt_bases = NULL;
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

#include "upb/port/undef.inc"
