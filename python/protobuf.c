// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "python/protobuf.h"

#include "python/descriptor.h"
#include "python/descriptor_containers.h"
#include "python/descriptor_pool.h"
#include "python/extension_dict.h"
#include "python/map.h"
#include "python/message.h"
#include "python/repeated.h"
#include "python/unknown_fields.h"

// Mutex wrapper when GIL-disabled. Zero-cost when GIL-enabled.
// NOTE: Protobuf Free-threading support is still experimental.
// TODO: Remove mutex and make upb python lock free.
#ifdef Py_GIL_DISABLED
#ifdef _POSIX_THREADS
#define ENABLE_MUTEX 1
#include <pthread.h>
#else
#error "GIL is disabled but _POSIX_THREADS isn't available"
#endif  // _POSIX_THREADS
#endif  // Py_GIL_DISABLED

typedef struct {
#ifdef ENABLE_MUTEX
  pthread_mutex_t mutex;
#endif
} FreeThreadingMutex;

#ifdef ENABLE_MUTEX
static FreeThreadingMutex obj_cache_mutex = {PTHREAD_MUTEX_INITIALIZER};
#else
static FreeThreadingMutex obj_cache_mutex = {};
#endif

static upb_Arena* PyUpb_NewArena(void);

void FreeThreadingLock(FreeThreadingMutex* thread_mutex) {
#ifdef ENABLE_MUTEX
  pthread_mutex_lock(&(thread_mutex->mutex));
#endif
}

void FreeThreadingUnlock(FreeThreadingMutex* thread_mutex) {
#ifdef ENABLE_MUTEX
  pthread_mutex_unlock(&(thread_mutex->mutex));
#endif
}

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
// WeakMap
// -----------------------------------------------------------------------------

struct PyUpb_WeakMap {
  upb_inttable table;
  upb_Arena* arena;
};

PyUpb_WeakMap* PyUpb_WeakMap_New(void) {
  upb_Arena* arena = PyUpb_NewArena();
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
  if (!upb_inttable_next(&map->table, &u_key, &val, iter)) return false;
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

static PyUpb_WeakMap* PyUpb_ObjCache_MaybeInstance(void) {
  // During the shutdown sequence, our objects may need to be deallocated, or
  // there may even still be attempts to *construct* new ones (in user code).
  // At that point our state will be NULL, so all access to the cache has to be
  // no-op.
  PyUpb_ModuleState* state = PyUpb_ModuleState_MaybeGet();
  if (!state) {
    return NULL;
  }
  return state->obj_cache;
}

void PyUpb_ObjCache_Add(const void* key, PyObject* py_obj) {
  PyUpb_WeakMap* cache = PyUpb_ObjCache_MaybeInstance();
  if (!cache) {
    return;
  }
  FreeThreadingLock(&obj_cache_mutex);
  PyUpb_WeakMap_Add(cache, key, py_obj);
  FreeThreadingUnlock(&obj_cache_mutex);
}

void PyUpb_KnownObjCache_Add(PyUpb_WeakMap* cache, const void* key,
                             PyObject* py_obj) {
  FreeThreadingLock(&obj_cache_mutex);
  PyUpb_WeakMap_Add(cache, key, py_obj);
  FreeThreadingUnlock(&obj_cache_mutex);
}

void PyUpb_ObjCache_Delete(const void* key) {
  PyUpb_WeakMap* cache = PyUpb_ObjCache_MaybeInstance();
  if (!cache) {
    return;
  }
  FreeThreadingLock(&obj_cache_mutex);
  PyUpb_WeakMap_Delete(cache, key);
  FreeThreadingUnlock(&obj_cache_mutex);
}

PyObject* PyUpb_ObjCache_Get(const void* key) {
  PyUpb_WeakMap* cache = PyUpb_ObjCache_MaybeInstance();
  if (!cache) {
    // When the interpreter is finalizing, the state may not be available, so
    // we don't try to find existing objects in the object cache - in order to
    // prevent a crash. Unfortunately, doing this can lead to different
    // semantics in protobuf APIs during shutdown. For example:
    //
    //   m = MyMessage()
    //
    //   # Returns true most of the time, but false during shutdown.
    //   assert(m.submsg is m.submsg)
    //   assert(Py_IsFinalizing());
    //
    //
    //   # Returns true most of the time, but false during shutdown.
    //   assert(foo_pb2.DESCRIPTOR.message_types_by_name['Bar'] is
    //          foo_pb2.DESCRIPTOR.message_types_by_name['Bar')
#if PY_VERSION_HEX >= 0x030D0000  // >= 3.13
    assert(Py_IsFinalizing());
#endif
    return NULL;
  }
  FreeThreadingLock(&obj_cache_mutex);
  PyObject* obj = PyUpb_WeakMap_Get(cache, key);
  FreeThreadingUnlock(&obj_cache_mutex);
  return obj;
}

// -----------------------------------------------------------------------------
// Arena
// -----------------------------------------------------------------------------

typedef struct {
  // clang-format off
  PyObject_HEAD
  upb_Arena* arena;
  // clang-format on
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
  return upb_Arena_Init(NULL, 0, global_alloc);
}

PyObject* PyUpb_Arena_New(void) {
  PyUpb_ModuleState* state = PyUpb_ModuleState_Get();
  PyUpb_Arena* arena = (void*)PyType_GenericAlloc(state->arena_type, 0);
  arena->arena = PyUpb_NewArena();
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

// -----------------------------------------------------------------------------
// Module Entry Point
// -----------------------------------------------------------------------------

PyMODINIT_FUNC PyInit__message(void) {
  PyObject* m = PyModule_Create(&module_def);
  if (!m) return NULL;

  PyUpb_ModuleState* state = PyUpb_ModuleState_GetFromModule(m);

  state->allow_oversize_protos = false;
  state->wkt_bases = NULL;
  FreeThreadingLock(&obj_cache_mutex);
  state->obj_cache = PyUpb_WeakMap_New();
  FreeThreadingUnlock(&obj_cache_mutex);
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
