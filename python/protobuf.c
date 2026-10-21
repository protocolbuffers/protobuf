// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "python/protobuf.h"

#if defined(PROTOBUF_PYTHON_EXPERIMENTAL_FREE_THREADING) && \
    !defined(Py_GIL_DISABLED)
#error "Experimental free threading requires a CPython free-threaded build"
#endif

#if defined(PROTOBUF_PYTHON_EXPERIMENTAL_FREE_THREADING) && \
    defined(Py_LIMITED_API)
#error "Experimental free threading requires --//python:limited_api=false"
#endif

#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "google/protobuf/breaking_changes.h"
#include "google/protobuf/descriptor.upbdefs.h"
#include "python/descriptor.h"
#include "python/descriptor_containers.h"
#include "python/descriptor_pool.h"
#include "python/extension_dict.h"
#include "python/free_threading/access.h"
#include "python/free_threading/weak_map.h"
#include "python/map.h"
#include "python/message.h"
#include "python/repeated.h"
#include "python/unknown_fields.h"
#include "upb/mem/alloc.h"
#include "upb/mem/arena.h"

// Must be last.
#include "upb/port/def.inc"

static int PyUpb_ModuleTraverse(PyObject* module, visitproc visit, void* arg) {
  PyUpb_ModuleState* state = PyModule_GetState(module);
  if (!state) return 0;
  // Native state must retain its own references: module attributes are public
  // and may be replaced or deleted without invalidating these pointers.
  for (size_t i = 0; i < kPyUpb_Descriptor_Count; ++i) {
    Py_VISIT(state->descriptor_types[i]);
  }
  Py_VISIT(state->by_name_map_type);
  Py_VISIT(state->by_name_iterator_type);
  Py_VISIT(state->by_number_map_type);
  Py_VISIT(state->by_number_iterator_type);
  Py_VISIT(state->generic_sequence_type);
  Py_VISIT(state->default_pool);
  Py_VISIT(state->descriptor_pool_type);
  Py_VISIT(state->extension_dict_type);
  Py_VISIT(state->extension_iterator_type);
  Py_VISIT(state->map_iterator_type);
  Py_VISIT(state->cmessage_type);
  Py_VISIT(state->message_meta_type);
  Py_VISIT(state->arena_type);
  Py_VISIT(state->repeated_composite_container_type);
  Py_VISIT(state->repeated_scalar_container_type);
  Py_VISIT(state->unknown_fields_type);
  Py_VISIT(PyUpb_LazyPtr_RawGet(&state->wkt_bases));
  Py_VISIT(state->message_map_container_type);
  Py_VISIT(state->scalar_map_container_type);
  Py_VISIT(state->encode_error_class);
  Py_VISIT(state->decode_error_class);
  Py_VISIT(state->descriptor_string);
  Py_VISIT(state->message_class);
  Py_VISIT(state->frozen_instance_error_class);
  Py_VISIT(state->enum_type_wrapper_class);
  Py_VISIT(state->listfields_item_key);
  Py_VISIT(state->unknown_field_type);
  return 0;
}

static int PyUpb_ModuleClear(PyObject* module) {
  PyUpb_ModuleState* state = PyModule_GetState(module);
  if (!state) return 0;
  for (size_t i = 0; i < kPyUpb_Descriptor_Count; ++i) {
    Py_CLEAR(state->descriptor_types[i]);
  }
  Py_CLEAR(state->by_name_map_type);
  Py_CLEAR(state->by_name_iterator_type);
  Py_CLEAR(state->by_number_map_type);
  Py_CLEAR(state->by_number_iterator_type);
  Py_CLEAR(state->generic_sequence_type);
  Py_CLEAR(state->default_pool);
  Py_CLEAR(state->descriptor_pool_type);
  Py_CLEAR(state->extension_dict_type);
  Py_CLEAR(state->extension_iterator_type);
  Py_CLEAR(state->map_iterator_type);
  Py_CLEAR(state->cmessage_type);
  Py_CLEAR(state->message_meta_type);
  Py_CLEAR(state->arena_type);
  Py_CLEAR(state->repeated_composite_container_type);
  Py_CLEAR(state->repeated_scalar_container_type);
  Py_CLEAR(state->unknown_fields_type);
  PyUpb_LazyPtr_Clear(&state->wkt_bases);
  Py_CLEAR(state->message_map_container_type);
  Py_CLEAR(state->scalar_map_container_type);
  Py_CLEAR(state->encode_error_class);
  Py_CLEAR(state->decode_error_class);
  Py_CLEAR(state->descriptor_string);
  Py_CLEAR(state->message_class);
  Py_CLEAR(state->frozen_instance_error_class);
  Py_CLEAR(state->enum_type_wrapper_class);
  Py_CLEAR(state->listfields_item_key);
  Py_CLEAR(state->unknown_field_type);
  return 0;
}

static void PyUpb_ModuleDealloc(void* module) {
  PyUpb_ModuleState* state = PyModule_GetState((PyObject*)module);
  if (!state) return;  // Module-state allocation itself may have failed.
  PyUpb_ModuleClear(module);
  if (state->obj_cache) {
    PyUpb_WeakMap_Free(state->obj_cache);
    state->obj_cache = NULL;
  }
  if (state->c_descriptor_symtab) {
    upb_DefPool_Free(state->c_descriptor_symtab);
    state->c_descriptor_symtab = NULL;
  }
}

PyObject* PyUpb_SetAllowOversizeProtos(PyObject* m, PyObject* arg) {
  if (!arg || !PyBool_Check(arg)) {
    PyErr_SetString(PyExc_TypeError,
                    "Argument to SetAllowOversizeProtos must be boolean");
    return NULL;
  }
  PyUpb_ModuleState* state = PyUpb_ModuleState_Get();
  upb_Atomic_Store(&state->allow_oversize_protos, PyObject_IsTrue(arg),
                   memory_order_relaxed);
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

#ifdef PROTOBUF_PYTHON_EXPERIMENTAL_FREE_THREADING
static int PyUpb_ModuleExec(PyObject* m);
static PyModuleDef_Slot module_slots[] = {
    {Py_mod_exec, PyUpb_ModuleExec},
    {Py_mod_multiple_interpreters,
     (void*)Py_MOD_PER_INTERPRETER_GIL_SUPPORTED},
    {Py_mod_gil, (void*)Py_MOD_GIL_NOT_USED},
    {0, NULL},
};
#endif

static struct PyModuleDef module_def = {PyModuleDef_HEAD_INIT,
                                        PYUPB_MODULE_NAME,
                                        "Protobuf Module",
                                        sizeof(PyUpb_ModuleState),
                                        PyUpb_ModuleMethods,  // m_methods
#ifdef PROTOBUF_PYTHON_EXPERIMENTAL_FREE_THREADING
                                        module_slots,         // m_slots
#else
                                        NULL,                 // m_slots
#endif
                                        PyUpb_ModuleTraverse, // m_traverse
                                        PyUpb_ModuleClear,    // m_clear
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
#ifdef PROTOBUF_PYTHON_EXPERIMENTAL_FREE_THREADING
  PyObject* name = PyUnicode_FromString(PYUPB_MODULE_NAME);
  if (!name) {
    // This lookup also runs from deallocators during interpreter shutdown.
    // They must not leave an exception pending if module state is unavailable.
    PyErr_Clear();
    return NULL;
  }
  PyObject* module = PyImport_GetModule(name);
  Py_DECREF(name);
  if (!module) {
    PyErr_Clear();
    return NULL;
  }
  PyUpb_ModuleState* state = PyModule_GetState(module);
  Py_DECREF(module);
  return state;
#else
  PyObject* module = PyState_FindModule(&module_def);
  return module ? PyModule_GetState(module) : NULL;
#endif
}

PyUpb_ModuleState* PyUpb_ModuleState_GetFromModule(PyObject* module) {
  PyUpb_ModuleState* state = PyModule_GetState(module);
  assert(state);
  assert(PyModule_GetDef(module) == &module_def);
  return state;
}

PyUpb_ModuleState* PyUpb_ModuleState_GetFromType(PyTypeObject* type) {
#ifdef PROTOBUF_PYTHON_EXPERIMENTAL_FREE_THREADING
  PyObject* module = PyType_GetModule(type);
  if (!module) return NULL;
  return PyUpb_ModuleState_GetFromModule(module);
#else
  return PyUpb_ModuleState_Get();
#endif
}

PyUpb_ModuleState* PyUpb_ModuleState_Get(void) {
#ifdef PROTOBUF_PYTHON_EXPERIMENTAL_FREE_THREADING
  PyUpb_ModuleState* state = PyUpb_ModuleState_MaybeGet();
  assert(state);
  return state;
#else
  PyObject* module = PyState_FindModule(&module_def);
  assert(module);
  return PyUpb_ModuleState_GetFromModule(module);
#endif
}

static PyObject* PyUpb_InitWktBases(void* unused) {
  PyObject* wkt_module = PyImport_ImportModule(PYUPB_PROTOBUF_INTERNAL_PACKAGE
                                               ".well_known_types");
  if (!wkt_module) return NULL;
  PyObject* wkt_bases = PyObject_GetAttrString(wkt_module, "WKTBASES");
  Py_DECREF(wkt_module);
  return wkt_bases;
}

PyObject* PyUpb_GetWktBases(PyUpb_ModuleState* state) {
  PyObject* wkt_bases = PyUpb_LazyPtr_LazyInitPyObj(
      &state->wkt_bases, PyUpb_InitWktBases, NULL);
  if (!wkt_bases) return NULL;

  // Preserve the private module attribute used by the existing implementation.
  // Concurrent stores are safe Python dictionary operations and all store the
  // same CAS-selected object.
  PyObject* m = NULL;
#ifdef PROTOBUF_PYTHON_EXPERIMENTAL_FREE_THREADING
  PyObject* module_name = PyUnicode_FromString(PYUPB_MODULE_NAME);
  if (module_name) {
    m = PyImport_GetModule(module_name);
    Py_DECREF(module_name);
  }
#else
  m = PyState_FindModule(&module_def);
#endif
  if (!m || PyModule_AddObjectRef(m, "__internal_wktbases", wkt_bases) < 0) {
#ifdef PROTOBUF_PYTHON_EXPERIMENTAL_FREE_THREADING
    Py_XDECREF(m);
#endif
    Py_DECREF(wkt_bases);
    return NULL;
  }
#ifdef PROTOBUF_PYTHON_EXPERIMENTAL_FREE_THREADING
  Py_DECREF(m);
#endif
  Py_DECREF(wkt_bases);
  return PyUpb_LazyPtr_RawGet(&state->wkt_bases);
}

// -----------------------------------------------------------------------------
// Arena
// -----------------------------------------------------------------------------

typedef struct {
  // clang-format off
  PyObject_HEAD
  upb_Arena* arena;
  // clang-format on
  PyObject* pool;
  PyUpb_WeakMap* obj_cache;
  PyUpb_AtomicInt32 frozen;
#ifdef PROTOBUF_PYTHON_EXPERIMENTAL_FREE_THREADING
  PyUpb_Access access;
#endif
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
    static PyUpb_AtomicInt32 count = 0;
    int32_t current =
        upb_Atomic_Add(&count, 1, memory_order_relaxed) + 1;
    if (current >= 10000) {
      int32_t expected = current;
      if (upb_Atomic_CompareExchangeStrong(
              &count, &expected, 0, memory_order_relaxed,
              memory_order_relaxed)) {
        malloc_trim(0);
      }
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

PyObject* PyUpb_Arena_New(PyObject* pool) {
  assert(pool != NULL);
  PyUpb_ModuleState* state = PyUpb_ModuleState_MaybeGet();
  if (!state) {
    PyErr_SetString(PyExc_RuntimeError, "Interpreter is finalizing");
    return NULL;
  }
  PyUpb_Arena* arena = (void*)PyType_GenericAlloc(state->arena_type, 0);
  if (!arena) return NULL;
  arena->arena = PyUpb_NewArena();
  if (!arena->arena) {
    PyErr_SetNone(PyExc_MemoryError);
    Py_DECREF(arena);
    return NULL;
  }
  arena->obj_cache = PyUpb_WeakMap_New();
  if (!arena->obj_cache) {
    PyErr_SetNone(PyExc_MemoryError);
    Py_DECREF(arena);
    return NULL;
  }
  arena->pool = pool;
  Py_INCREF(pool);
  upb_Atomic_Init(&arena->frozen, false);
#ifdef PROTOBUF_PYTHON_EXPERIMENTAL_FREE_THREADING
  PyUpb_Access_Init(&arena->access);
#endif
  return &arena->ob_base;
}

PyObject* PyUpb_Arena_GetPool(PyObject* self) {
  PyUpb_Arena* me = (PyUpb_Arena*)self;
  assert(me->pool != NULL);
  return me->pool;
}

static void PyUpb_Arena_Dealloc(PyObject* self) {
  PyUpb_Arena* me = (PyUpb_Arena*)self;
  if (me->obj_cache) {
    PyUpb_WeakMap_Free(me->obj_cache);
  }
  Py_XDECREF(me->pool);
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
  return upb_Atomic_Load(&((PyUpb_Arena*)arena)->frozen,
                         memory_order_acquire);
}

void PyUpb_Arena_SetFrozen(PyObject* arena, bool frozen) {
  upb_Atomic_Store(&((PyUpb_Arena*)arena)->frozen, frozen,
                   memory_order_release);
}

bool PyUpb_Arena_TryRead(PyObject* arena) {
#ifdef PROTOBUF_PYTHON_EXPERIMENTAL_FREE_THREADING
  if (!PyUpb_Access_TryRead(&((PyUpb_Arena*)arena)->access)) {
    PyErr_SetString(PyExc_RuntimeError,
                    "concurrent mutation of a protobuf message");
    return false;
  }
#else
  (void)arena;
#endif
  return true;
}

bool PyUpb_Arena_TryWrite(PyObject* arena) {
#ifdef PROTOBUF_PYTHON_EXPERIMENTAL_FREE_THREADING
  if (!PyUpb_Access_TryWrite(&((PyUpb_Arena*)arena)->access)) {
    PyErr_SetString(PyExc_RuntimeError,
                    "concurrent mutation of a protobuf message");
    return false;
  }
#else
  (void)arena;
#endif
  return true;
}

bool PyUpb_Arena_TryUpgradeReadToWrite(PyObject* arena) {
#ifdef PROTOBUF_PYTHON_EXPERIMENTAL_FREE_THREADING
  if (!PyUpb_Access_TryUpgradeReadToWrite(
          &((PyUpb_Arena*)arena)->access)) {
    PyErr_SetString(PyExc_RuntimeError,
                    "concurrent mutation of a protobuf message");
    return false;
  }
#else
  (void)arena;
#endif
  return true;
}

void PyUpb_Arena_EndRead(PyObject* arena) {
#ifdef PROTOBUF_PYTHON_EXPERIMENTAL_FREE_THREADING
  PyUpb_Access_EndRead(&((PyUpb_Arena*)arena)->access);
#else
  (void)arena;
#endif
}

void PyUpb_Arena_EndWrite(PyObject* arena) {
#ifdef PROTOBUF_PYTHON_EXPERIMENTAL_FREE_THREADING
  PyUpb_Access_EndWrite(&((PyUpb_Arena*)arena)->access);
#else
  (void)arena;
#endif
}

bool PyUpb_Arena_CacheAdd(PyObject* _arena, const void* key,
                          PyObject** py_obj) {
  PyUpb_Arena* arena = (PyUpb_Arena*)_arena;
  return PyUpb_WeakMap_Add(arena->obj_cache, key, py_obj);
}

bool PyUpb_Arena_CacheUniqueAdd(PyObject* _arena, const void* key,
                                PyObject* py_obj) {
  PyUpb_Arena* arena = (PyUpb_Arena*)_arena;
  return PyUpb_WeakMap_UniqueAdd(arena->obj_cache, key, py_obj);
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
  #ifdef PROTOBUF_PYTHON_EXPERIMENTAL_FREE_THREADING
  PyObject* type = PyType_FromModuleAndSpec(m, spec, NULL);
#else
  PyObject* type = PyType_FromSpec(spec);
#endif
  if (!type) return NULL;
  if (PyModule_AddObjectRef(m, name, type) < 0) {
    Py_DECREF(type);
    return NULL;
  }
  return (PyTypeObject*)type;
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
  #ifdef PROTOBUF_PYTHON_EXPERIMENTAL_FREE_THREADING
  PyObject* type = PyType_FromModuleAndSpec(m, spec, NULL);
#else
  PyObject* type = PyType_FromSpec(spec);
#endif
  if (!type) return NULL;
  const char* name = PyUpb_GetClassName(spec);
  if (PyModule_AddObjectRef(m, name, type) < 0) {
    Py_XDECREF(type);
    return NULL;
  }
  return (PyTypeObject*)type;
}

PyTypeObject* PyUpb_AddClassWithBases(PyObject* m, PyType_Spec* spec,
                                      PyObject* bases) {
  #ifdef PROTOBUF_PYTHON_EXPERIMENTAL_FREE_THREADING
  PyObject* type = PyType_FromModuleAndSpec(m, spec, bases);
#else
  PyObject* type = PyType_FromSpecWithBases(spec, bases);
#endif
  if (!type) return NULL;
  const char* name = PyUpb_GetClassName(spec);
  if (PyModule_AddObjectRef(m, name, type) < 0) {
    Py_XDECREF(type);
    return NULL;
  }
  return (PyTypeObject*)type;
}

PyTypeObject* PyUpb_AddClassWithRegister(PyObject* m, PyType_Spec* spec,
                                         PyObject* virtual_base,
                                         const char** methods) {
  #ifdef PROTOBUF_PYTHON_EXPERIMENTAL_FREE_THREADING
  PyObject* type = PyType_FromModuleAndSpec(m, spec, NULL);
#else
  PyObject* type = PyType_FromSpec(spec);
#endif
  if (!type) return NULL;
  PyObject* ret1 = PyObject_CallMethod(virtual_base, "register", "O", type);
  if (!ret1) {
    Py_XDECREF(type);
    return NULL;
  }
  Py_DECREF(ret1);
  for (size_t i = 0; methods[i] != NULL; i++) {
    PyObject* method = PyObject_GetAttrString(virtual_base, methods[i]);
    if (!method) {
      Py_XDECREF(type);
      return NULL;
    }
    int ret2 = PyObject_SetAttrString(type, methods[i], method);
    Py_DECREF(method);
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

bool PyUpb_CheckFrozen(bool is_frozen, const char* error_msg) {
  if (is_frozen) {
    PyUpb_SetFrozenErrorWithMsg(error_msg);
    return false;
  }
  return true;
}

// -----------------------------------------------------------------------------
// Module Entry Point
// -----------------------------------------------------------------------------

static int PyUpb_ModuleExec(PyObject* m) {
  PyUpb_ModuleState* state = PyUpb_ModuleState_GetFromModule(m);

  upb_Atomic_Init(&state->allow_oversize_protos, false);
  state->wkt_bases = NULL;
  state->c_descriptor_symtab = upb_DefPool_New();
  if (!state->c_descriptor_symtab ||
      !google_protobuf_FileDescriptorProto_getmsgdef(
          state->c_descriptor_symtab)) {
    PyErr_NoMemory();
    return -1;
  }

  if (!PyUpb_InitDescriptorContainers(m) || !PyUpb_InitDescriptorPool(m) ||
      !PyUpb_InitDescriptor(m) || !PyUpb_InitArena(m) ||
      !PyUpb_InitExtensionDict(m) || !PyUpb_Map_Init(m) ||
      !PyUpb_InitMessage(m) || !PyUpb_Repeated_Init(m) ||
      !PyUpb_UnknownFields_Init(m)) {
    return -1;
  }

  // Temporary: an cookie we can use in the tests to ensure we are testing upb
  // and not another protobuf library on the system.
  if (PyModule_AddIntConstant(m, "_IS_UPB", 1) < 0) return -1;

  return 0;
}

#ifdef PROTOBUF_PYTHON_EXPERIMENTAL_FREE_THREADING
PyMODINIT_FUNC PyInit__message(void) { return PyModuleDef_Init(&module_def); }
#else
PyMODINIT_FUNC PyInit__message(void) {
  PyObject* m = PyModule_Create(&module_def);
  if (!m) return NULL;
  if (PyUpb_ModuleExec(m) < 0) {
    Py_DECREF(m);
    return NULL;
  }
  return m;
}
#endif

#include "upb/port/undef.inc"
