// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef PYUPB_PROTOBUF_H__
#define PYUPB_PROTOBUF_H__

#include <assert.h>
#include <stdbool.h>

#include "python/python_api.h"
#include "upb/hash/int_table.h"
#include "upb/reflection/def.h"

#define PYUPB_PROTOBUF_PUBLIC_PACKAGE "google.protobuf"
#define PYUPB_PROTOBUF_INTERNAL_PACKAGE "google.protobuf.internal"
#define PYUPB_DESCRIPTOR_PROTO_PACKAGE "google.protobuf"
#define PYUPB_MODULE_NAME "google._upb._message"

#define PYUPB_DESCRIPTOR_MODULE "google.protobuf.descriptor_pb2"
#define PYUPB_RETURN_OOM return PyErr_SetNone(PyExc_MemoryError), NULL

struct PyUpb_WeakMap;
typedef struct PyUpb_WeakMap PyUpb_WeakMap;

// -----------------------------------------------------------------------------
// ModuleState
// -----------------------------------------------------------------------------

// We store all "global" state in this struct instead of using (C) global
// variables. This makes this extension compatible with sub-interpreters.

typedef struct {
  // A reference counter of how many handles are acquired for this interpreter
  // state. Handles can be acquired via PyUpb_ModuleInterpreterState_Acquire and
  // PyUpb_ModuleInterpreterState_Release.
  //
  // Since the interpreter state is not shared between interpreters,
  // this counter is not atomic.
  int refcount;

  // The unique owning pointer to the object cache.
  PyUpb_WeakMap* obj_cache;

} PyUpb_ModuleInterpreterState;

typedef enum {
  kPyUpb_Descriptor = 0,
  kPyUpb_EnumDescriptor = 1,
  kPyUpb_EnumValueDescriptor = 2,
  kPyUpb_FieldDescriptor = 3,
  kPyUpb_FileDescriptor = 4,
  kPyUpb_MethodDescriptor = 5,
  kPyUpb_OneofDescriptor = 6,
  kPyUpb_ServiceDescriptor = 7,
  kPyUpb_Descriptor_Count = 8,
} PyUpb_DescriptorType;

typedef struct {
  PyUpb_ModuleInterpreterState* interp_state;

  // From descriptor.c
  PyTypeObject* descriptor_types[kPyUpb_Descriptor_Count];

  // From descriptor_containers.c
  PyTypeObject* by_name_map_type;
  PyTypeObject* by_name_iterator_type;
  PyTypeObject* by_number_map_type;
  PyTypeObject* by_number_iterator_type;
  PyTypeObject* generic_sequence_type;

  // From descriptor_pool.c
  PyObject* default_pool;

  // From descriptor_pool.c
  PyTypeObject* descriptor_pool_type;
  upb_DefPool* c_descriptor_symtab;

  // From extension_dict.c
  PyTypeObject* extension_dict_type;
  PyTypeObject* extension_iterator_type;

  // From map.c
  PyTypeObject* map_iterator_type;
  PyTypeObject* message_map_container_type;
  PyTypeObject* scalar_map_container_type;

  // From message.c
  PyObject* decode_error_class;
  PyObject* descriptor_string;
  PyObject* encode_error_class;
  PyObject* enum_type_wrapper_class;
  PyObject* message_class;
  PyTypeObject* cmessage_type;
  PyTypeObject* message_meta_type;
  PyObject* listfields_item_key;

  // From protobuf.c
  bool allow_oversize_protos;
  PyObject* wkt_bases;
  PyObject* wkt_module;
  PyTypeObject* arena_type;

  // A copy of the obj_cache pointer from the per interpreter state.
  // The map is bound to the lifetime of the per interpreter state,
  // which can outlife the module state.
  PyUpb_WeakMap* obj_cache;

  // From repeated.c
  PyTypeObject* repeated_composite_container_type;
  PyTypeObject* repeated_scalar_container_type;

  // From unknown_fields.c
  PyTypeObject* unknown_fields_type;
  PyObject* unknown_field_type;
} PyUpb_ModuleState;

// Returns a valid PyUpb_ModuleState* from the given module
PyUpb_ModuleState* PyUpb_ModuleState_GetFromModule(PyObject* module);
// Returns a valid PyUpb_ModuleState* from the given type
PyUpb_ModuleState* PyUpb_ModuleState_GetFromType(PyTypeObject* type);

// Returns a new handle to the shared UPB state of this interpreter instance
// from the given module state with incrementing the reference counter.
PyUpb_ModuleInterpreterState* PyUpb_ModuleInterpreterState_Acquire(
    PyUpb_ModuleState* state);
// Releases a handle to the shared UPB state.
void PyUpb_ModuleInterpreterState_Release(PyUpb_ModuleInterpreterState* state);
// Duplicates a handle to the shared UPB state with incrementing the reference
// counter.
PyUpb_ModuleInterpreterState* PyUpb_ModuleInterpreterState_DuplicateHandle(
    PyUpb_ModuleInterpreterState* interp_state);

// Returns:
//   from google.protobuf.internal.well_known_types import WKTBASES
//
// This has to be imported lazily rather than at module load time, because
// otherwise it would cause a circular import.
PyObject* PyUpb_GetWktBases(PyUpb_ModuleState* state);

// -----------------------------------------------------------------------------
// WeakMap
// -----------------------------------------------------------------------------

// A WeakMap maps C pointers to the corresponding Python wrapper object. We
// want a consistent Python wrapper object for each C object, both to save
// memory and to provide object stability (ie. x is x).
//
// Each wrapped object should add itself to the map when it is constructed and
// remove itself from the map when it is destroyed. The map is weak so it does
// not take references to the cached objects.

PyUpb_WeakMap* PyUpb_WeakMap_New(void);
void PyUpb_WeakMap_Free(PyUpb_WeakMap* map);

// Adds the given object to the map, indexed by the given key.
void PyUpb_WeakMap_Add(PyUpb_WeakMap* map, const void* key, PyObject* py_obj);

// Removes the given key from the cache. It must exist in the cache currently.
PyObject* PyUpb_WeakMap_Delete(PyUpb_WeakMap* map, const void* key);
PyObject* PyUpb_WeakMap_TryDelete(PyUpb_WeakMap* map, const void* key);

// Returns a new reference to an object if it exists, otherwise returns NULL.
PyObject* PyUpb_WeakMap_Get(PyUpb_WeakMap* map, const void* key);

#define PYUPB_WEAKMAP_BEGIN UPB_INTTABLE_BEGIN

// Iteration over the weak map, eg.
//
// intptr_t it = PYUPB_WEAKMAP_BEGIN;
// while (PyUpb_WeakMap_Next(map, &key, &obj, &it)) {
//   // ...
// }
//
// Note that the callee does not own a ref on the returned `obj`.
bool PyUpb_WeakMap_Next(PyUpb_WeakMap* map, const void** key, PyObject** obj,
                        intptr_t* iter);
void PyUpb_WeakMap_DeleteIter(PyUpb_WeakMap* map, intptr_t* iter);

// -----------------------------------------------------------------------------
// ObjCache
// -----------------------------------------------------------------------------

// The object cache is a global WeakMap for mapping upb objects to the
// corresponding wrapper.
void PyUpb_ObjCache_Add(PyUpb_ModuleState* state, const void* key,
                        PyObject* py_obj);
PyObject* PyUpb_ObjCache_Delete(PyUpb_ModuleInterpreterState* state,
                                const void* key);
PyObject* PyUpb_ObjCache_Get(PyUpb_ModuleState* state,
                             const void* key);  // returns NULL if not present.
PyUpb_WeakMap* PyUpb_ObjCache_Instance(PyUpb_ModuleState* state);

// -----------------------------------------------------------------------------
// Arena
// -----------------------------------------------------------------------------

PyObject* PyUpb_Arena_New(PyUpb_ModuleState* state);
upb_Arena* PyUpb_Arena_Get(PyObject* arena);

// -----------------------------------------------------------------------------
// Utilities
// -----------------------------------------------------------------------------

// Creates a Python type from `spec` and adds it to the given module `m`.
PyTypeObject* PyUpb_AddClass(PyObject* m, PyType_Spec* spec);

// Like PyUpb_AddClass(), but allows you to specify a tuple of base classes
// in `bases`.
PyTypeObject* PyUpb_AddClassWithBases(PyObject* m, PyType_Spec* spec,
                                      PyObject* bases);

// Like PyUpb_AddClass(), but allows you to specify a tuple of base classes in
// `bases` to register as a "virtual subclass" with mixin methods.
PyTypeObject* PyUpb_AddClassWithRegister(PyObject* m, PyType_Spec* spec,
                                         PyObject* virtual_base,
                                         const char** methods);

// A function that implements the tp_new slot for types that we do not allow
// users to create directly. This will immediately fail with an error message.
PyObject* PyUpb_Forbidden_New(PyObject* cls, PyObject* args, PyObject* kwds);

// Our standard dealloc func. It follows the guidance defined in:
//   https://docs.python.org/3/c-api/typeobj.html#c.PyTypeObject.tp_dealloc
// It requires that the type is a heap type, which all of our types are.
static inline void PyUpb_Dealloc(void* self) {
  PyTypeObject* tp = Py_TYPE(self);
  assert(PyType_GetFlags(tp) & Py_TPFLAGS_HEAPTYPE);
  freefunc tp_free = (freefunc)PyType_GetSlot(tp, Py_tp_free);
  tp_free(self);
  Py_DECREF(tp);
}

// Equivalent to the Py_NewRef() function introduced in Python 3.10.  If/when we
// drop support for Python <3.10, we can remove this function and replace all
// callers with Py_NewRef().
static inline PyObject* PyUpb_NewRef(PyObject* obj) {
  Py_INCREF(obj);
  return obj;
}

const char* PyUpb_GetStrData(PyObject* obj);
const char* PyUpb_VerifyStrData(PyObject* obj);

// For an expression like:
//    foo[index]
//
// Converts `index` to an effective i/count/step, for a repeated field
// or descriptor sequence of size 'size'.
bool PyUpb_IndexToRange(PyObject* index, Py_ssize_t size, Py_ssize_t* i,
                        Py_ssize_t* count, Py_ssize_t* step);
#endif  // PYUPB_PROTOBUF_H__
