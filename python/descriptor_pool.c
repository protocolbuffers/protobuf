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

#include "google/protobuf/descriptor.upbdefs.h"
#include "python/convert.h"
#include "python/descriptor.h"
#include "python/message.h"
#include "python/protobuf.h"
#include "upb/def.h"
#include "upb/util/def_to_proto.h"

// -----------------------------------------------------------------------------
// DescriptorPool
// -----------------------------------------------------------------------------

typedef struct {
  PyObject_HEAD;
  upb_DefPool* symtab;
  PyObject* db;  // The DescriptorDatabase underlying this pool.  May be NULL.
} PyUpb_DescriptorPool;

PyObject* PyUpb_DescriptorPool_GetDefaultPool() {
  PyUpb_ModuleState* s = PyUpb_ModuleState_Get();
  return s->default_pool;
}

const upb_MessageDef* PyUpb_DescriptorPool_GetFileProtoDef(void) {
  PyUpb_ModuleState* s = PyUpb_ModuleState_Get();
  if (!s->c_descriptor_symtab) {
    s->c_descriptor_symtab = upb_DefPool_New();
  }
  return google_protobuf_FileDescriptorProto_getmsgdef(s->c_descriptor_symtab);
}

static PyObject* PyUpb_DescriptorPool_DoCreateWithCache(
    PyTypeObject* type, PyObject* db, PyUpb_WeakMap* obj_cache) {
  PyUpb_DescriptorPool* pool = (void*)PyType_GenericAlloc(type, 0);
  pool->symtab = upb_DefPool_New();
  pool->db = db;
  Py_XINCREF(pool->db);
  PyUpb_WeakMap_Add(obj_cache, pool->symtab, &pool->ob_base);
  return &pool->ob_base;
}

static PyObject* PyUpb_DescriptorPool_DoCreate(PyTypeObject* type,
                                               PyObject* db) {
  return PyUpb_DescriptorPool_DoCreateWithCache(type, db,
                                                PyUpb_ObjCache_Instance());
}

upb_DefPool* PyUpb_DescriptorPool_GetSymtab(PyObject* pool) {
  return ((PyUpb_DescriptorPool*)pool)->symtab;
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

PyObject* PyUpb_DescriptorPool_Get(const upb_DefPool* symtab) {
  PyObject* pool = PyUpb_ObjCache_Get(symtab);
  assert(pool);
  return pool;
}

static void PyUpb_DescriptorPool_Dealloc(PyUpb_DescriptorPool* self) {
  PyUpb_DescriptorPool_Clear(self);
  upb_DefPool_Free(self->symtab);
  PyUpb_ObjCache_Delete(self->symtab);
  PyUpb_Dealloc(self);
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

  if (db == Py_None) db = NULL;
  return PyUpb_DescriptorPool_DoCreate(type, db);
}

static PyObject* PyUpb_DescriptorPool_DoAdd(PyObject* _self,
                                            PyObject* file_desc);

static bool PyUpb_DescriptorPool_TryLoadFileProto(PyUpb_DescriptorPool* self,
                                                  PyObject* proto) {
  if (proto == NULL) {
    if (PyErr_ExceptionMatches(PyExc_KeyError)) {
      // Expected error: item was simply not found.
      PyErr_Clear();
      return true;  // We didn't accomplish our goal, but we didn't error out.
    }
    return false;
  }
  if (proto == Py_None) return true;
  PyObject* ret = PyUpb_DescriptorPool_DoAdd((PyObject*)self, proto);
  bool ok = ret != NULL;
  Py_XDECREF(ret);
  return ok;
}

static bool PyUpb_DescriptorPool_TryLoadSymbol(PyUpb_DescriptorPool* self,
                                               PyObject* sym) {
  if (!self->db) return false;
  PyObject* file_proto =
      PyObject_CallMethod(self->db, "FindFileContainingSymbol", "O", sym);
  bool ret = PyUpb_DescriptorPool_TryLoadFileProto(self, file_proto);
  Py_XDECREF(file_proto);
  return ret;
}

static bool PyUpb_DescriptorPool_TryLoadFilename(PyUpb_DescriptorPool* self,
                                                 PyObject* filename) {
  if (!self->db) return false;
  PyObject* file_proto =
      PyObject_CallMethod(self->db, "FindFileByName", "O", filename);
  bool ret = PyUpb_DescriptorPool_TryLoadFileProto(self, file_proto);
  Py_XDECREF(file_proto);
  return ret;
}

bool PyUpb_DescriptorPool_CheckNoDatabase(PyObject* _self) { return true; }

static bool PyUpb_DescriptorPool_LoadDependentFiles(
    PyUpb_DescriptorPool* self, google_protobuf_FileDescriptorProto* proto) {
  size_t n;
  const upb_StringView* deps =
      google_protobuf_FileDescriptorProto_dependency(proto, &n);
  for (size_t i = 0; i < n; i++) {
    const upb_FileDef* dep = upb_DefPool_FindFileByNameWithSize(
        self->symtab, deps[i].data, deps[i].size);
    if (!dep) {
      PyObject* filename =
          PyUnicode_FromStringAndSize(deps[i].data, deps[i].size);
      if (!filename) return false;
      bool ok = PyUpb_DescriptorPool_TryLoadFilename(self, filename);
      Py_DECREF(filename);
      if (!ok) return false;
    }
  }
  return true;
}

static PyObject* PyUpb_DescriptorPool_DoAddSerializedFile(
    PyObject* _self, PyObject* serialized_pb) {
  PyUpb_DescriptorPool* self = (PyUpb_DescriptorPool*)_self;
  upb_Arena* arena = upb_Arena_New();
  if (!arena) PYUPB_RETURN_OOM;
  PyObject* result = NULL;

  char* buf;
  Py_ssize_t size;
  if (PyBytes_AsStringAndSize(serialized_pb, &buf, &size) < 0) {
    goto done;
  }

  google_protobuf_FileDescriptorProto* proto =
      google_protobuf_FileDescriptorProto_parse(buf, size, arena);
  if (!proto) {
    PyErr_SetString(PyExc_TypeError, "Couldn't parse file content!");
    goto done;
  }

  upb_StringView name = google_protobuf_FileDescriptorProto_name(proto);
  const upb_FileDef* file =
      upb_DefPool_FindFileByNameWithSize(self->symtab, name.data, name.size);

  if (file) {
    // If the existing file is equal to the new file, then silently ignore the
    // duplicate add.
    google_protobuf_FileDescriptorProto* existing =
        upb_FileDef_ToProto(file, arena);
    if (!existing) {
      PyErr_SetNone(PyExc_MemoryError);
      goto done;
    }
    const upb_MessageDef* m = PyUpb_DescriptorPool_GetFileProtoDef();
    if (upb_Message_IsEqual(proto, existing, m)) {
      Py_INCREF(Py_None);
      result = Py_None;
      goto done;
    }
  }

  if (self->db) {
    if (!PyUpb_DescriptorPool_LoadDependentFiles(self, proto)) goto done;
  }

  upb_Status status;
  upb_Status_Clear(&status);

  const upb_FileDef* filedef =
      upb_DefPool_AddFile(self->symtab, proto, &status);
  if (!filedef) {
    PyErr_Format(PyExc_TypeError,
                 "Couldn't build proto file into descriptor pool: %s",
                 upb_Status_ErrorMessage(&status));
    goto done;
  }

  result = PyUpb_FileDescriptor_Get(filedef);

done:
  upb_Arena_Free(arena);
  return result;
}

static PyObject* PyUpb_DescriptorPool_DoAdd(PyObject* _self,
                                            PyObject* file_desc) {
  if (!PyUpb_Message_Verify(file_desc)) return NULL;
  const upb_MessageDef* m = PyUpb_Message_GetMsgdef(file_desc);
  const char* file_proto_name =
      PYUPB_DESCRIPTOR_PROTO_PACKAGE ".FileDescriptorProto";
  if (strcmp(upb_MessageDef_FullName(m), file_proto_name) != 0) {
    return PyErr_Format(PyExc_TypeError, "Can only add FileDescriptorProto");
  }
  PyObject* subargs = PyTuple_New(0);
  if (!subargs) return NULL;
  PyObject* serialized =
      PyUpb_Message_SerializeToString(file_desc, subargs, NULL);
  Py_DECREF(subargs);
  if (!serialized) return NULL;
  PyObject* ret = PyUpb_DescriptorPool_DoAddSerializedFile(_self, serialized);
  Py_DECREF(serialized);
  return ret;
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
  if (self->db) {
    PyErr_SetString(
        PyExc_ValueError,
        "Cannot call AddSerializedFile on a DescriptorPool that uses a "
        "DescriptorDatabase. Add your file to the underlying database.");
    return false;
  }
  return PyUpb_DescriptorPool_DoAddSerializedFile(_self, serialized_pb);
}

static PyObject* PyUpb_DescriptorPool_Add(PyObject* _self,
                                          PyObject* file_desc) {
  PyUpb_DescriptorPool* self = (PyUpb_DescriptorPool*)_self;
  if (self->db) {
    PyErr_SetString(
        PyExc_ValueError,
        "Cannot call Add on a DescriptorPool that uses a DescriptorDatabase. "
        "Add your file to the underlying database.");
    return false;
  }
  return PyUpb_DescriptorPool_DoAdd(_self, file_desc);
}

/*
 * PyUpb_DescriptorPool_FindFileByName()
 *
 * Implements:
 *   DescriptorPool.FindFileByName(self, name)
 */
static PyObject* PyUpb_DescriptorPool_FindFileByName(PyObject* _self,
                                                     PyObject* arg) {
  PyUpb_DescriptorPool* self = (PyUpb_DescriptorPool*)_self;

  const char* name = PyUpb_VerifyStrData(arg);
  if (!name) return NULL;

  const upb_FileDef* file = upb_DefPool_FindFileByName(self->symtab, name);
  if (file == NULL && self->db) {
    if (!PyUpb_DescriptorPool_TryLoadFilename(self, arg)) return NULL;
    file = upb_DefPool_FindFileByName(self->symtab, name);
  }
  if (file == NULL) {
    return PyErr_Format(PyExc_KeyError, "Couldn't find file %.200s", name);
  }

  return PyUpb_FileDescriptor_Get(file);
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

  const char* name = PyUpb_VerifyStrData(arg);
  if (!name) return NULL;

  const upb_FieldDef* field =
      upb_DefPool_FindExtensionByName(self->symtab, name);
  if (field == NULL && self->db) {
    if (!PyUpb_DescriptorPool_TryLoadSymbol(self, arg)) return NULL;
    field = upb_DefPool_FindExtensionByName(self->symtab, name);
  }
  if (field == NULL) {
    return PyErr_Format(PyExc_KeyError, "Couldn't find extension %.200s", name);
  }

  return PyUpb_FieldDescriptor_Get(field);
}

/*
 * PyUpb_DescriptorPool_FindMessageTypeByName()
 *
 * Implements:
 *   DescriptorPool.FindMessageTypeByName(self, name)
 */
static PyObject* PyUpb_DescriptorPool_FindMessageTypeByName(PyObject* _self,
                                                            PyObject* arg) {
  PyUpb_DescriptorPool* self = (PyUpb_DescriptorPool*)_self;

  const char* name = PyUpb_VerifyStrData(arg);
  if (!name) return NULL;

  const upb_MessageDef* m = upb_DefPool_FindMessageByName(self->symtab, name);
  if (m == NULL && self->db) {
    if (!PyUpb_DescriptorPool_TryLoadSymbol(self, arg)) return NULL;
    m = upb_DefPool_FindMessageByName(self->symtab, name);
  }
  if (m == NULL) {
    return PyErr_Format(PyExc_KeyError, "Couldn't find message %.200s", name);
  }

  return PyUpb_Descriptor_Get(m);
}

// Splits a dotted symbol like foo.bar.baz on the last dot.  Returns the portion
// after the last dot (baz) and updates `*parent_size` to the length of the
// parent (foo.bar).  Returns NULL if no dots were present.
static const char* PyUpb_DescriptorPool_SplitSymbolName(const char* sym,
                                                        size_t* parent_size) {
  const char* last_dot = strrchr(sym, '.');
  if (!last_dot) return NULL;
  *parent_size = last_dot - sym;
  return last_dot + 1;
}

/*
 * PyUpb_DescriptorPool_FindFieldByName()
 *
 * Implements:
 *   DescriptorPool.FindFieldByName(self, name)
 */
static PyObject* PyUpb_DescriptorPool_FindFieldByName(PyObject* _self,
                                                      PyObject* arg) {
  PyUpb_DescriptorPool* self = (PyUpb_DescriptorPool*)_self;

  const char* name = PyUpb_VerifyStrData(arg);
  if (!name) return NULL;

  size_t parent_size;
  const char* child = PyUpb_DescriptorPool_SplitSymbolName(name, &parent_size);
  const upb_FieldDef* f = NULL;
  if (child) {
    const upb_MessageDef* parent =
        upb_DefPool_FindMessageByNameWithSize(self->symtab, name, parent_size);
    if (parent == NULL && self->db) {
      if (!PyUpb_DescriptorPool_TryLoadSymbol(self, arg)) return NULL;
      parent = upb_DefPool_FindMessageByNameWithSize(self->symtab, name,
                                                     parent_size);
    }
    if (parent) {
      f = upb_MessageDef_FindFieldByName(parent, child);
    }
  }

  if (!f) {
    return PyErr_Format(PyExc_KeyError, "Couldn't find message %.200s", name);
  }

  return PyUpb_FieldDescriptor_Get(f);
}

/*
 * PyUpb_DescriptorPool_FindEnumTypeByName()
 *
 * Implements:
 *   DescriptorPool.FindEnumTypeByName(self, name)
 */
static PyObject* PyUpb_DescriptorPool_FindEnumTypeByName(PyObject* _self,
                                                         PyObject* arg) {
  PyUpb_DescriptorPool* self = (PyUpb_DescriptorPool*)_self;

  const char* name = PyUpb_VerifyStrData(arg);
  if (!name) return NULL;

  const upb_EnumDef* e = upb_DefPool_FindEnumByName(self->symtab, name);
  if (e == NULL && self->db) {
    if (!PyUpb_DescriptorPool_TryLoadSymbol(self, arg)) return NULL;
    e = upb_DefPool_FindEnumByName(self->symtab, name);
  }
  if (e == NULL) {
    return PyErr_Format(PyExc_KeyError, "Couldn't find enum %.200s", name);
  }

  return PyUpb_EnumDescriptor_Get(e);
}

/*
 * PyUpb_DescriptorPool_FindOneofByName()
 *
 * Implements:
 *   DescriptorPool.FindOneofByName(self, name)
 */
static PyObject* PyUpb_DescriptorPool_FindOneofByName(PyObject* _self,
                                                      PyObject* arg) {
  PyUpb_DescriptorPool* self = (PyUpb_DescriptorPool*)_self;

  const char* name = PyUpb_VerifyStrData(arg);
  if (!name) return NULL;

  size_t parent_size;
  const char* child = PyUpb_DescriptorPool_SplitSymbolName(name, &parent_size);

  if (child) {
    const upb_MessageDef* parent =
        upb_DefPool_FindMessageByNameWithSize(self->symtab, name, parent_size);
    if (parent == NULL && self->db) {
      if (!PyUpb_DescriptorPool_TryLoadSymbol(self, arg)) return NULL;
      parent = upb_DefPool_FindMessageByNameWithSize(self->symtab, name,
                                                     parent_size);
    }
    if (parent) {
      const upb_OneofDef* o = upb_MessageDef_FindOneofByName(parent, child);
      return PyUpb_OneofDescriptor_Get(o);
    }
  }

  return PyErr_Format(PyExc_KeyError, "Couldn't find oneof %.200s", name);
}

static PyObject* PyUpb_DescriptorPool_FindServiceByName(PyObject* _self,
                                                        PyObject* arg) {
  PyUpb_DescriptorPool* self = (PyUpb_DescriptorPool*)_self;

  const char* name = PyUpb_VerifyStrData(arg);
  if (!name) return NULL;

  const upb_ServiceDef* s = upb_DefPool_FindServiceByName(self->symtab, name);
  if (s == NULL && self->db) {
    if (!PyUpb_DescriptorPool_TryLoadSymbol(self, arg)) return NULL;
    s = upb_DefPool_FindServiceByName(self->symtab, name);
  }
  if (s == NULL) {
    return PyErr_Format(PyExc_KeyError, "Couldn't find service %.200s", name);
  }

  return PyUpb_ServiceDescriptor_Get(s);
}

static PyObject* PyUpb_DescriptorPool_FindMethodByName(PyObject* _self,
                                                       PyObject* arg) {
  PyUpb_DescriptorPool* self = (PyUpb_DescriptorPool*)_self;

  const char* name = PyUpb_VerifyStrData(arg);
  if (!name) return NULL;
  size_t parent_size;
  const char* child = PyUpb_DescriptorPool_SplitSymbolName(name, &parent_size);

  if (!child) goto err;
  const upb_ServiceDef* parent =
      upb_DefPool_FindServiceByNameWithSize(self->symtab, name, parent_size);
  if (parent == NULL && self->db) {
    if (!PyUpb_DescriptorPool_TryLoadSymbol(self, arg)) return NULL;
    parent =
        upb_DefPool_FindServiceByNameWithSize(self->symtab, name, parent_size);
  }
  if (!parent) goto err;
  const upb_MethodDef* m = upb_ServiceDef_FindMethodByName(parent, child);
  if (!m) goto err;
  return PyUpb_MethodDescriptor_Get(m);

err:
  return PyErr_Format(PyExc_KeyError, "Couldn't find method %.200s", name);
}

static PyObject* PyUpb_DescriptorPool_FindFileContainingSymbol(PyObject* _self,
                                                               PyObject* arg) {
  PyUpb_DescriptorPool* self = (PyUpb_DescriptorPool*)_self;

  const char* name = PyUpb_VerifyStrData(arg);
  if (!name) return NULL;

  const upb_FileDef* f =
      upb_DefPool_FindFileContainingSymbol(self->symtab, name);
  if (f == NULL && self->db) {
    if (!PyUpb_DescriptorPool_TryLoadSymbol(self, arg)) return NULL;
    f = upb_DefPool_FindFileContainingSymbol(self->symtab, name);
  }
  if (f == NULL) {
    return PyErr_Format(PyExc_KeyError, "Couldn't find symbol %.200s", name);
  }

  return PyUpb_FileDescriptor_Get(f);
}

static PyObject* PyUpb_DescriptorPool_FindExtensionByNumber(PyObject* _self,
                                                            PyObject* args) {
  PyUpb_DescriptorPool* self = (PyUpb_DescriptorPool*)_self;
  PyObject* message_descriptor;
  int number;
  if (!PyArg_ParseTuple(args, "Oi", &message_descriptor, &number)) {
    return NULL;
  }

  const upb_FieldDef* f = upb_DefPool_FindExtensionByNumber(
      self->symtab, PyUpb_Descriptor_GetDef(message_descriptor), number);
  if (f == NULL) {
    return PyErr_Format(PyExc_KeyError, "Couldn't find Extension %d", number);
  }

  return PyUpb_FieldDescriptor_Get(f);
}

static PyObject* PyUpb_DescriptorPool_FindAllExtensions(PyObject* _self,
                                                        PyObject* msg_desc) {
  PyUpb_DescriptorPool* self = (PyUpb_DescriptorPool*)_self;
  const upb_MessageDef* m = PyUpb_Descriptor_GetDef(msg_desc);
  size_t n;
  const upb_FieldDef** ext = upb_DefPool_GetAllExtensions(self->symtab, m, &n);
  PyObject* ret = PyList_New(n);
  if (!ret) goto done;
  for (size_t i = 0; i < n; i++) {
    PyObject* field = PyUpb_FieldDescriptor_Get(ext[i]);
    if (!field) {
      Py_DECREF(ret);
      ret = NULL;
      goto done;
    }
    PyList_SetItem(ret, i, field);
  }
done:
  free(ext);
  return ret;
}

static PyMethodDef PyUpb_DescriptorPool_Methods[] = {
    {"Add", PyUpb_DescriptorPool_Add, METH_O,
     "Adds the FileDescriptorProto and its types to this pool."},
    {"AddSerializedFile", PyUpb_DescriptorPool_AddSerializedFile, METH_O,
     "Adds a serialized FileDescriptorProto to this pool."},
    {"FindFileByName", PyUpb_DescriptorPool_FindFileByName, METH_O,
     "Searches for a file descriptor by its .proto name."},
    {"FindMessageTypeByName", PyUpb_DescriptorPool_FindMessageTypeByName,
     METH_O, "Searches for a message descriptor by full name."},
    {"FindFieldByName", PyUpb_DescriptorPool_FindFieldByName, METH_O,
     "Searches for a field descriptor by full name."},
    {"FindExtensionByName", PyUpb_DescriptorPool_FindExtensionByName, METH_O,
     "Searches for extension descriptor by full name."},
    {"FindEnumTypeByName", PyUpb_DescriptorPool_FindEnumTypeByName, METH_O,
     "Searches for enum type descriptor by full name."},
    {"FindOneofByName", PyUpb_DescriptorPool_FindOneofByName, METH_O,
     "Searches for oneof descriptor by full name."},
    {"FindServiceByName", PyUpb_DescriptorPool_FindServiceByName, METH_O,
     "Searches for service descriptor by full name."},
    {"FindMethodByName", PyUpb_DescriptorPool_FindMethodByName, METH_O,
     "Searches for method descriptor by full name."},
    {"FindFileContainingSymbol", PyUpb_DescriptorPool_FindFileContainingSymbol,
     METH_O, "Gets the FileDescriptor containing the specified symbol."},
    {"FindExtensionByNumber", PyUpb_DescriptorPool_FindExtensionByNumber,
     METH_VARARGS, "Gets the extension descriptor for the given number."},
    {"FindAllExtensions", PyUpb_DescriptorPool_FindAllExtensions, METH_O,
     "Gets all known extensions of the given message descriptor."},
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
  PyUpb_ModuleState* state = PyUpb_ModuleState_GetFromModule(m);
  PyTypeObject* descriptor_pool_type =
      PyUpb_AddClass(m, &PyUpb_DescriptorPool_Spec);

  if (!descriptor_pool_type) return false;

  state->default_pool = PyUpb_DescriptorPool_DoCreateWithCache(
      descriptor_pool_type, NULL, state->obj_cache);
  return state->default_pool &&
         PyModule_AddObject(m, "default_pool", state->default_pool) == 0;
}
