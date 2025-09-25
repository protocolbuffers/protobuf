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

#ifdef Py_mod_multiple_interpreters
#define PYUPB_USE_MULTI_PHASE_INIT
#endif  // Py_mod_multiple_interpreters

static upb_Arena* PyUpb_NewArena(void);

static void PyUpb_ModuleStateFree(PyUpb_ModuleState* state) {
  assert(state != NULL);

  PyUpb_ModuleInterpreterState_Release(state->interp_state);
  state->interp_state = NULL;

  if (state->c_descriptor_symtab) {
    upb_DefPool_Free(state->c_descriptor_symtab);
    state->c_descriptor_symtab = NULL;
  }
}

static void PyUpb_ModuleInterpreterState_AcquireUnchecked(
    PyUpb_ModuleInterpreterState* state) {
  assert(state != NULL);

  state->refcount += 1;
}

static void PyUpb_ModuleStateInit(PyUpb_ModuleState* state) {
  assert(state != NULL);

  state->interp_state = malloc(sizeof(PyUpb_ModuleInterpreterState));
  memset(state->interp_state, 0, sizeof(PyUpb_ModuleInterpreterState));

  PyUpb_ModuleInterpreterState_AcquireUnchecked(state->interp_state);

  assert(state->interp_state);
  state->interp_state->obj_cache = PyUpb_WeakMap_New();

  state->allow_oversize_protos = false;
  state->c_descriptor_symtab = NULL;

  assert(state->interp_state->obj_cache);
  state->obj_cache = state->interp_state->obj_cache;
}

static int PyUpb_StateClear(PyUpb_ModuleState* state) {
  assert(state != NULL);

  // Lazy imported
  Py_CLEAR(state->wkt_bases);
  Py_CLEAR(state->wkt_module);

  Py_CLEAR(state->unknown_field_type);
  Py_CLEAR(state->unknown_fields_type);

  Py_CLEAR(state->repeated_scalar_container_type);
  Py_CLEAR(state->repeated_composite_container_type);

  Py_CLEAR(state->arena_type);

  // old bases

  Py_CLEAR(state->listfields_item_key);
  Py_CLEAR(state->message_meta_type);
  Py_CLEAR(state->cmessage_type);
  Py_CLEAR(state->message_class);
  Py_CLEAR(state->enum_type_wrapper_class);
  Py_CLEAR(state->encode_error_class);
  Py_CLEAR(state->descriptor_string);
  Py_CLEAR(state->decode_error_class);

  Py_CLEAR(state->scalar_map_container_type);
  Py_CLEAR(state->message_map_container_type);
  Py_CLEAR(state->map_iterator_type);

  Py_CLEAR(state->extension_iterator_type);
  Py_CLEAR(state->extension_dict_type);

  Py_CLEAR(state->descriptor_pool_type);

  Py_CLEAR(state->default_pool);

  Py_CLEAR(state->generic_sequence_type);
  Py_CLEAR(state->by_number_iterator_type);
  Py_CLEAR(state->by_number_map_type);
  Py_CLEAR(state->by_name_iterator_type);
  Py_CLEAR(state->by_name_map_type);

  for (size_t i = 0; i < kPyUpb_Descriptor_Count; i++) {
    Py_CLEAR(state->descriptor_types[i]);
  }

  return 0;
}

static void PyUpb_ModuleDealloc(void* module) {
  PyUpb_ModuleState* state = PyModule_GetState(module);

  PyUpb_StateClear(state);

  PyUpb_ModuleStateFree(state);
}

PyObject* PyUpb_SetAllowOversizeProtos(PyObject* m, PyObject* arg) {
  if (!arg || !PyBool_Check(arg)) {
    PyErr_SetString(PyExc_TypeError,
                    "Argument to SetAllowOversizeProtos must be boolean");
    return NULL;
  }
  PyUpb_ModuleState* state = PyUpb_ModuleState_GetFromModule(m);
  state->allow_oversize_protos = PyObject_IsTrue(arg);
  Py_INCREF(arg);
  return arg;
}

static PyMethodDef PyUpb_ModuleMethods[] = {
    {"SetAllowOversizeProtos", PyUpb_SetAllowOversizeProtos, METH_O,
     "Enable/disable oversize proto parsing."},
    {NULL, NULL}};

PyObject* PyUpb_CreateModule(PyObject* spec, PyModuleDef* def);
int PyUpb_ExecModule(PyObject* module);

#ifdef PYUPB_USE_MULTI_PHASE_INIT
static PyModuleDef_Slot PyUpb_ModuleSlots[] = {
    // Create a slot for the module initialization
    // {Py_mod_create, (void*)(PyUpb_CreateModule)},
    {Py_mod_exec, (void*)(PyUpb_ExecModule)},
    // Support subinterpreters with independent GILs
    {Py_mod_multiple_interpreters, Py_MOD_PER_INTERPRETER_GIL_SUPPORTED},
#if defined(Py_mod_gil)
#if !defined(Py_LIMITED_API) || (Py_LIMITED_API + 0 < 0x030d0000)
    // Always require a GIL held until changes with no GIL support have been
    // merged
    {Py_mod_gil, Py_MOD_GIL_USED},
#endif
#endif
    // Sentinel object
    {0, NULL},
};
#else
#define PyUpb_ModuleSlots NULL
#endif

static struct PyModuleDef module_def = {PyModuleDef_HEAD_INIT,
                                        PYUPB_MODULE_NAME,
                                        "Protobuf Module",
                                        sizeof(PyUpb_ModuleState),
                                        PyUpb_ModuleMethods,   // m_methods
                                        PyUpb_ModuleSlots,     // m_slots
                                        NULL,                  // m_traverse
                                        NULL,                  // m_clear
                                        PyUpb_ModuleDealloc};  // m_free

// -----------------------------------------------------------------------------
// ModuleState
// -----------------------------------------------------------------------------

PyUpb_ModuleState* PyUpb_ModuleState_GetFromModule(PyObject* module) {
  assert(PyModule_GetDef(module) == &module_def);
  assert(module);

  PyUpb_ModuleState* state = PyModule_GetState(module);
  assert(state);

  return state;
}

PyUpb_ModuleState* PyUpb_ModuleState_GetFromType(PyTypeObject* type) {
  assert(type != NULL);

#ifdef PYUPB_USE_MULTI_PHASE_INIT
  PyObject* module = PyType_GetModuleByDef(type, &module_def);
#else
  (void)type;
  PyObject* module = PyState_FindModule(&module_def);
#endif

  assert(module);
  return PyUpb_ModuleState_GetFromModule(module);
}

PyUpb_ModuleInterpreterState* PyUpb_ModuleInterpreterState_Acquire(
    PyUpb_ModuleState* state) {
  assert(state != NULL);
  assert(state->interp_state->refcount > 0);

  PyUpb_ModuleInterpreterState_AcquireUnchecked(state->interp_state);

  return state->interp_state;
}

void PyUpb_ModuleInterpreterState_Release(PyUpb_ModuleInterpreterState* state) {
  assert(state != NULL);
  assert(state->refcount > 0);

  state->refcount -= 1;

  if (state->refcount > 0) {
    return;
  }

  PyUpb_WeakMap_Free(state->obj_cache);

  free(state);
}

PyUpb_ModuleInterpreterState* PyUpb_ModuleInterpreterState_DuplicateHandle(
    PyUpb_ModuleInterpreterState* interp_state) {
  assert(interp_state != NULL);
  assert(interp_state->refcount > 0);

  PyUpb_ModuleInterpreterState_AcquireUnchecked(interp_state);

  return interp_state;
}

PyObject* PyUpb_GetWktBases(PyUpb_ModuleState* state) {
  if (!state->wkt_bases) {
    PyObject* wkt_module = PyImport_ImportModule(PYUPB_PROTOBUF_INTERNAL_PACKAGE
                                                 ".well_known_types");

    if (wkt_module == NULL) {
      return false;
    }

    state->wkt_module = wkt_module;
    state->wkt_bases = PyObject_GetAttrString(wkt_module, "WKTBASES");

    // TODO Question is this really needed? This could lead to a cyclic
    // reference. We instead now hold a reference to wkt_bases and free it once
    // the module state is freed. Reparent ownership to m. PyModule_AddObject(m,
    // "__internal_wktbases", state->wkt_bases);
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

void PyUpb_WeakMap_Free(PyUpb_WeakMap* map) {
  assert(upb_inttable_count(&map->table) == 0);

  upb_Arena_Free(map->arena);
}

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

PyObject* PyUpb_WeakMap_Delete(PyUpb_WeakMap* map, const void* key) {
  upb_value val;
  bool removed =
      upb_inttable_remove(&map->table, PyUpb_WeakMap_GetKey(key), &val);
  (void)removed;
  assert(removed);
  return upb_value_getptr(val);
}

PyObject* PyUpb_WeakMap_TryDelete(PyUpb_WeakMap* map, const void* key) {
  upb_value val;
  upb_inttable_remove(&map->table, PyUpb_WeakMap_GetKey(key), &val);
  return upb_value_getptr(val);
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

PyUpb_WeakMap* PyUpb_ObjCache_Instance(PyUpb_ModuleState* state) {
  assert(state);
  assert(state->obj_cache);
  return state->obj_cache;
}

void PyUpb_ObjCache_Add(PyUpb_ModuleState* state, const void* key,
                        PyObject* py_obj) {
  PyUpb_WeakMap_Add(PyUpb_ObjCache_Instance(state), key, py_obj);
}

PyObject* PyUpb_ObjCache_Delete(PyUpb_ModuleInterpreterState* state,
                                const void* key) {
  assert(state);

  return PyUpb_WeakMap_Delete(state->obj_cache, key);
}

PyObject* PyUpb_ObjCache_Get(PyUpb_ModuleState* state, const void* key) {
  return PyUpb_WeakMap_Get(PyUpb_ObjCache_Instance(state), key);
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
#if 0  // TODO Find out whether this is still needed
#ifdef __GLIBC__
    // TODO Currently mutated by many threads, is this still needed?
    static int count = 0;
    if (++count == 10000) {
      malloc_trim(0);
      count = 0;
    }
#endif
#endif
    return NULL;
  } else {
    return realloc(ptr, size);
  }
}

// TODO Thread unsafe
static upb_alloc trim_alloc = {&upb_trim_allocfunc};
// TODO Referenced value Thread unsafe
static upb_alloc* global_alloc = &trim_alloc;

static upb_Arena* PyUpb_NewArena(void) {
  return upb_Arena_Init(NULL, 0, global_alloc);
}

PyObject* PyUpb_Arena_New(PyUpb_ModuleState* state) {
  assert(state);
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
  return PyUpb_AddClassWithBases(m, spec, NULL);
}

PyTypeObject* PyUpb_AddClassWithBases(PyObject* m, PyType_Spec* spec,
                                      PyObject* bases) {
  PyObject* type = PyType_FromModuleAndSpec(m, spec, bases);
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
  PyObject* type = PyType_FromModuleAndSpec(m, spec, NULL);
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

int PyUpb_ExecModule(PyObject* m) {
  assert(m != NULL);

  PyUpb_ModuleState* state = PyModule_GetState(m);
  assert(state);
  assert(PyModule_GetDef(m) == &module_def);

  if (state == NULL) {
    return -1;
  }

  PyUpb_ModuleStateInit(state);

  assert(PyUpb_ModuleState_GetFromModule(m) == state);

  if (!PyUpb_InitDescriptorContainers(m) || !PyUpb_InitDescriptorPool(m) ||
      !PyUpb_InitDescriptor(m) || !PyUpb_InitArena(m) ||
      !PyUpb_InitExtensionDict(m) || !PyUpb_Map_Init(m) ||
      !PyUpb_InitMessage(m) || !PyUpb_Repeated_Init(m) ||
      !PyUpb_UnknownFields_Init(m)) {
    Py_DECREF(m);
    return -1;
  }

  // Temporary: an cookie we can use in the tests to ensure we are testing upb
  // and not another protobuf library on the system.
  PyModule_AddIntConstant(m, "_IS_UPB", 1);

  return 0;
}

PyMODINIT_FUNC PyInit__message(void) {
  if (!PyUpb_CPythonBits_Global_Init()) {
    return NULL;
  }

#ifdef PYUPB_USE_MULTI_PHASE_INIT
  return PyModuleDef_Init(&module_def);
#else
  PyObject* m = PyModule_Create(&module_def);
  if (!m) {
    return NULL;
  }

  if (PyUpb_ExecModule(m) != 0) {
    Py_XDECREF(m);
    return NULL;
  }

  return m;
#endif
}
