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

#include "python/descriptor_pool.h"

#include "python/descriptor.h"
#include "python/protobuf.h"
#include "upb/def.h"

// -----------------------------------------------------------------------------
// DescriptorPool
// -----------------------------------------------------------------------------

typedef struct {
  PyObject_HEAD
  upb_symtab* symtab;
  PyObject* db;
} PyUpb_DescriptorPool;

static PyObject* PyUpb_DescriptorPool_DoCreate(PyTypeObject* type,
                                               PyObject* db) {
  PyUpb_DescriptorPool* pool = PyObject_GC_New(PyUpb_DescriptorPool, type);
  pool->symtab = upb_symtab_new();
  pool->db = db;
  Py_XINCREF(pool->db);
  PyObject_GC_Track(&pool->ob_base);
  return &pool->ob_base;
}

static int PyUpb_DescriptorPool_Traverse(PyUpb_DescriptorPool* self,
                                         visitproc visit, void* arg) {
  Py_VISIT(self->db);
  return 0;
}

static int PyUpb_DescriptorPool_Clear(PyUpb_DescriptorPool* self) {
  Py_CLEAR(self->db);
  return 0;
}

static void PyUpb_DescriptorPool_Dealloc(PyUpb_DescriptorPool* self) {
  upb_symtab_free(self->symtab);
  PyUpb_DescriptorPool_Clear(self);
  PyObject_GC_Del(self);
}

/*
 * DescriptorPool.__new__()
 *
 * Implements:
 *   DescriptorPool(descriptor_db=None)
 */
static PyObject* PyUpb_DescriptorPool_New(PyTypeObject* type, PyObject* args,
                                          PyObject* kwargs) {
  char* kwlist[] = {"descriptor_db", 0};
  PyObject* db = NULL;

  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|O", kwlist, &db)) {
    return NULL;
  }

  return PyUpb_DescriptorPool_DoCreate(type, db);
}

/*
 * PyUpb_DescriptorPool_AddSerializedFile()
 *
 * Implements:
 *   DescriptorPool.AddSerializedFile(self, serialized_file_descriptor)
 *
 * Adds the given serialized FileDescriptorProto to the pool.
 */
static PyObject* PyUpb_DescriptorPool_AddSerializedFile(
    PyObject* _self, PyObject* serialized_pb) {
  PyUpb_DescriptorPool* self = (PyUpb_DescriptorPool*)_self;
  char* buf;
  Py_ssize_t size;
  upb_arena* arena = upb_arena_new();
  PyObject* result = NULL;

  if (self->db) {
    PyErr_SetString(
        PyExc_ValueError,
        "Cannot call Add on a DescriptorPool that uses a DescriptorDatabase. "
        "Add your file to the underlying database.");
    return NULL;
  }

  if (PyBytes_AsStringAndSize(serialized_pb, &buf, &size) < 0) {
    goto done;
  }

  google_protobuf_FileDescriptorProto* proto =
      google_protobuf_FileDescriptorProto_parse(buf, size, arena);
  if (!proto) {
    PyErr_SetString(PyExc_TypeError, "Couldn't parse file content!");
    goto done;
  }

  upb_status status;
  upb_status_clear(&status);

  const upb_filedef* filedef = upb_symtab_addfile(self->symtab, proto, &status);
  if (!filedef) {
    PyErr_Format(PyExc_TypeError,
                 "Couldn't build proto file into descriptor pool: %s",
                 upb_status_errmsg(&status));
    goto done;
  }

  result = PyUpb_FileDescriptor_GetOrCreateWrapper(filedef, _self);

done:
  upb_arena_free(arena);
  return result;
}

/*
 * PyUpb_DescriptorPool_FindExtensionByName()
 *
 * Implements:
 *   DescriptorPool.FindExtensionByName(self, name)
 */
static PyObject* PyUpb_DescriptorPool_FindExtensionByName(PyObject* _self,
                                                          PyObject* arg) {
  PyUpb_DescriptorPool* self = (PyUpb_DescriptorPool*)_self;

  const char* name = PyUpb_GetStrData(arg);
  if (!name) return NULL;

  const upb_fielddef* field = upb_symtab_lookupext(self->symtab, name);
  if (field == NULL) {
    return PyErr_Format(PyExc_KeyError, "Couldn't find extension %.200s", name);
  }

  return PyUpb_FieldDescriptor_GetOrCreateWrapper(field, _self);
}

static PyMethodDef PyUpb_DescriptorPool_Methods[] = {
    /*
      TODO: implement remaining methods.
    { "Add", Add, METH_O,
      "Adds the FileDescriptorProto and its types to this pool." },
      */
    {"AddSerializedFile", PyUpb_DescriptorPool_AddSerializedFile, METH_O,
     "Adds a serialized FileDescriptorProto to this pool."},
    /*
    { "FindFileByName", FindFileByName, METH_O,
      "Searches for a file descriptor by its .proto name." },
    { "FindMessageTypeByName", FindMessageByName, METH_O,
      "Searches for a message descriptor by full name." },
    { "FindFieldByName", FindFieldByNameMethod, METH_O,
      "Searches for a field descriptor by full name." },
      */
    {"FindExtensionByName", PyUpb_DescriptorPool_FindExtensionByName, METH_O,
     "Searches for extension descriptor by full name."},
    /*
    { "FindEnumTypeByName", FindEnumTypeByNameMethod, METH_O,
      "Searches for enum type descriptor by full name." },
    { "FindOneofByName", FindOneofByNameMethod, METH_O,
      "Searches for oneof descriptor by full name." },
    { "FindServiceByName", FindServiceByName, METH_O,
      "Searches for service descriptor by full name." },
    { "FindMethodByName", FindMethodByName, METH_O,
      "Searches for method descriptor by full name." },
    { "FindFileContainingSymbol", FindFileContainingSymbol, METH_O,
      "Gets the FileDescriptor containing the specified symbol." },
    { "FindExtensionByNumber", FindExtensionByNumber, METH_VARARGS,
      "Gets the extension descriptor for the given number." },
    { "FindAllExtensions", FindAllExtensions, METH_O,
      "Gets all known extensions of the given message descriptor." },
    */
    {NULL}};

static PyType_Slot PyUpb_DescriptorPool_Slots[] = {
    {Py_tp_clear, PyUpb_DescriptorPool_Clear},
    {Py_tp_dealloc, PyUpb_DescriptorPool_Dealloc},
    {Py_tp_methods, PyUpb_DescriptorPool_Methods},
    {Py_tp_new, PyUpb_DescriptorPool_New},
    {Py_tp_traverse, PyUpb_DescriptorPool_Traverse},
    {0, NULL}};

static PyType_Spec PyUpb_DescriptorPool_Spec = {
    PYUPB_MODULE_NAME ".DescriptorPool",
    sizeof(PyUpb_DescriptorPool),
    0,  // tp_itemsize
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC,
    PyUpb_DescriptorPool_Slots,
};

// -----------------------------------------------------------------------------
// Top Level
// -----------------------------------------------------------------------------

bool PyUpb_InitDescriptorPool(PyObject* m) {
  PyTypeObject* descriptor_pool_type =
      AddObject(m, "DescriptorPool", &PyUpb_DescriptorPool_Spec);

  if (!descriptor_pool_type) return false;

  PyObject* default_pool =
      PyUpb_DescriptorPool_DoCreate(descriptor_pool_type, NULL);
  return default_pool &&
         PyModule_AddObject(m, "default_pool", default_pool) == 0;
}
