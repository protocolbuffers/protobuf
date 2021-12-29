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

#include "python/descriptor.h"

#include "python/convert.h"
#include "python/descriptor_containers.h"
#include "python/descriptor_pool.h"
#include "python/message.h"
#include "python/protobuf.h"
#include "upb/def.h"
#include "upb/util/def_to_proto.h"

// -----------------------------------------------------------------------------
// DescriptorBase
// -----------------------------------------------------------------------------

// This representation is used by all concrete descriptors.

typedef struct {
  PyObject_HEAD
  PyObject* pool;  // We own a ref.
  const void* def;    // Type depends on the class. Kept alive by "pool".
  PyObject* options;  // NULL if not present or not cached.
} PyUpb_DescriptorBase;

PyObject* PyUpb_AnyDescriptor_GetPool(PyObject* desc) {
  PyUpb_DescriptorBase* base = (void*)desc;
  return base->pool;
}

const void* PyUpb_AnyDescriptor_GetDef(PyObject* desc) {
  PyUpb_DescriptorBase* base = (void*)desc;
  return base->def;
}

static PyUpb_DescriptorBase* PyUpb_DescriptorBase_DoCreate(
    PyUpb_DescriptorType type, const void* def, const upb_filedef* file) {
  PyUpb_ModuleState* state = PyUpb_ModuleState_Get();
  PyTypeObject* type_obj = state->descriptor_types[type];
  assert(def);

  PyUpb_DescriptorBase* base = (void*)PyType_GenericAlloc(type_obj, 0);
  base->pool = PyUpb_DescriptorPool_Get(upb_filedef_symtab(file));
  base->def = def;
  base->options = NULL;

  PyUpb_ObjCache_Add(def, &base->ob_base);
  return base;
}

// Returns a Python object wrapping |def|, of descriptor type |type|.  If a
// wrapper was previously created for this def, returns it, otherwise creates a
// new wrapper.
static PyObject* PyUpb_DescriptorBase_Get(PyUpb_DescriptorType type,
                                          const void* def,
                                          const upb_filedef* file) {
  PyUpb_DescriptorBase* base = (PyUpb_DescriptorBase*)PyUpb_ObjCache_Get(def);

  if (!base) {
    base = PyUpb_DescriptorBase_DoCreate(type, def, file);
  }

  return &base->ob_base;
}

static PyUpb_DescriptorBase* PyUpb_DescriptorBase_Check(
    PyObject* obj, PyUpb_DescriptorType type) {
  PyUpb_ModuleState* state = PyUpb_ModuleState_Get();
  PyTypeObject* type_obj = state->descriptor_types[type];
  if (!PyObject_TypeCheck(obj, type_obj)) {
    PyErr_Format(PyExc_TypeError, "Expected object of type %S, but got %R",
                 type_obj, obj);
    return NULL;
  }
  return (PyUpb_DescriptorBase*)obj;
}

static PyObject* PyUpb_DescriptorBase_GetOptions(PyUpb_DescriptorBase* self,
                                                 const upb_msg* opts,
                                                 const upb_msglayout* layout,
                                                 const char* msg_name) {
  if (!self->options) {
    // Load descriptors protos if they are not loaded already. We have to do
    // this lazily, otherwise, it would lead to circular imports.
    PyObject* mod = PyImport_ImportModule("google.protobuf.descriptor_pb2");
    Py_DECREF(mod);

    // Find the correct options message.
    PyObject* default_pool = PyUpb_DescriptorPool_GetDefaultPool();
    const upb_symtab* symtab = PyUpb_DescriptorPool_GetSymtab(default_pool);
    const upb_msgdef* m = upb_symtab_lookupmsg(symtab, msg_name);
    assert(m);

    // Copy the options message from C to Python using serialize+parse.
    // We don't wrap the C object directly because there is no guarantee that
    // the descriptor_pb2 that was loaded at runtime has the same members or
    // layout as the C types that were compiled in.
    size_t size;
    PyObject* py_arena = PyUpb_Arena_New();
    upb_arena* arena = PyUpb_Arena_Get(py_arena);
    char* pb = upb_encode(opts, layout, arena, &size);
    upb_msg* opts2 = upb_msg_new(m, arena);
    assert(opts2);
    bool ok = _upb_decode(pb, size, opts2, upb_msgdef_layout(m),
                          upb_symtab_extreg(symtab), 0,
                          arena) == kUpb_DecodeStatus_Ok;
    (void)ok;
    assert(ok);

    // Disabled until python/message.c is reviewed and checked in.
    // self->options = PyUpb_CMessage_Get(opts2, m, py_arena);
    Py_DECREF(py_arena);
    PyErr_Format(PyExc_NotImplementedError, "Not yet implemented");
    return NULL;
  }

  Py_INCREF(self->options);
  return self->options;
}

typedef void* PyUpb_ToProto_Func(const void* def, upb_arena* arena);

static PyObject* PyUpb_DescriptorBase_GetSerializedProto(
    PyObject* _self, PyUpb_ToProto_Func* func, const upb_msglayout* layout) {
  PyUpb_DescriptorBase* self = (void*)_self;
  upb_arena* arena = upb_arena_new();
  if (!arena) PYUPB_RETURN_OOM;
  upb_msg* proto = func(self->def, arena);
  if (!proto) goto oom;
  size_t size;
  char* pb = upb_encode(proto, layout, arena, &size);
  if (!pb) goto oom;
  PyObject* str = PyBytes_FromStringAndSize(pb, size);
  upb_arena_free(arena);
  return str;

oom:
  upb_arena_free(arena);
  PyErr_SetNone(PyExc_MemoryError);
  return NULL;
}

static PyObject* PyUpb_DescriptorBase_CopyToProto(PyObject* _self,
                                                  PyUpb_ToProto_Func* func,
                                                  const upb_msglayout* layout,
                                                  PyObject* py_proto) {
  PyObject* serialized =
      PyUpb_DescriptorBase_GetSerializedProto(_self, func, layout);
  if (!serialized) return NULL;
  return PyUpb_CMessage_MergeFromString(py_proto, serialized);
}

static void PyUpb_DescriptorBase_Dealloc(PyUpb_DescriptorBase* base) {
  PyUpb_ObjCache_Delete(base->def);
  Py_DECREF(base->pool);
  Py_XDECREF(base->options);
  PyUpb_Dealloc(base);
}

#define DESCRIPTOR_BASE_SLOTS                           \
  {Py_tp_new, (void*)&PyUpb_Forbidden_New},             \
  {Py_tp_dealloc, (void*)&PyUpb_DescriptorBase_Dealloc}

// -----------------------------------------------------------------------------
// Descriptor
// -----------------------------------------------------------------------------

PyObject* PyUpb_Descriptor_Get(const upb_msgdef* m) {
  assert(m);
  const upb_filedef* file = upb_msgdef_file(m);
  return PyUpb_DescriptorBase_Get(kPyUpb_Descriptor, m, file);
}

PyObject* PyUpb_Descriptor_GetClass(const upb_msgdef* m) {
  PyObject* ret = PyUpb_ObjCache_Get(upb_msgdef_layout(m));
  assert(ret);
  return ret;
}

// The LookupNested*() functions provide name lookup for entities nested inside
// a message.  This uses the symtab's table, which requires that the symtab is
// not being mutated concurrently.  We can guarantee this for Python-owned
// symtabs, but upb cannot guarantee it in general for an arbitrary
// `const upb_msgdef*`.

static const void* PyUpb_Descriptor_LookupNestedMessage(const upb_msgdef* m,
                                                        const char* name) {
  const upb_filedef* filedef = upb_msgdef_file(m);
  const upb_symtab* symtab = upb_filedef_symtab(filedef);
  PyObject* qname = PyUnicode_FromFormat("%s.%s", upb_msgdef_fullname(m), name);
  const upb_msgdef* ret =
      upb_symtab_lookupmsg(symtab, PyUnicode_AsUTF8AndSize(qname, NULL));
  Py_DECREF(qname);
  return ret;
}

static const void* PyUpb_Descriptor_LookupNestedEnum(const upb_msgdef* m,
                                                     const char* name) {
  const upb_filedef* filedef = upb_msgdef_file(m);
  const upb_symtab* symtab = upb_filedef_symtab(filedef);
  PyObject* qname = PyUnicode_FromFormat("%s.%s", upb_msgdef_fullname(m), name);
  const upb_enumdef* ret =
      upb_symtab_lookupenum(symtab, PyUnicode_AsUTF8AndSize(qname, NULL));
  Py_DECREF(qname);
  return ret;
}

static const void* PyUpb_Descriptor_LookupNestedExtension(const upb_msgdef* m,
                                                          const char* name) {
  const upb_filedef* filedef = upb_msgdef_file(m);
  const upb_symtab* symtab = upb_filedef_symtab(filedef);
  PyObject* qname = PyUnicode_FromFormat("%s.%s", upb_msgdef_fullname(m), name);
  const upb_fielddef* ret =
      upb_symtab_lookupext(symtab, PyUnicode_AsUTF8AndSize(qname, NULL));
  Py_DECREF(qname);
  return ret;
}

static PyObject* PyUpb_Descriptor_GetExtensionRanges(PyObject* _self,
                                                     void* closure) {
  PyUpb_DescriptorBase* self = (PyUpb_DescriptorBase*)_self;
  int n = upb_msgdef_extrangecount(self->def);
  PyObject* range_list = PyList_New(n);

  for (int i = 0; i < n; i++) {
    const upb_extrange* range = upb_msgdef_extrange(self->def, i);
    PyObject* start = PyLong_FromLong(upb_extrange_start(range));
    PyObject* end = PyLong_FromLong(upb_extrange_end(range));
    PyList_SetItem(range_list, i, PyTuple_Pack(2, start, end));
  }

  return range_list;
}

static PyObject* PyUpb_Descriptor_GetExtensions(PyObject* _self,
                                                void* closure) {
  PyUpb_DescriptorBase* self = (void*)_self;
  static PyUpb_GenericSequence_Funcs funcs = {
      (void*)&upb_msgdef_nestedextcount,
      (void*)&upb_msgdef_nestedext,
      (void*)&PyUpb_FieldDescriptor_Get,
  };
  return PyUpb_GenericSequence_New(&funcs, self->def, self->pool);
}

static PyObject* PyUpb_Descriptor_GetExtensionsByName(PyObject* _self,
                                                      void* closure) {
  PyUpb_DescriptorBase* self = (void*)_self;
  static PyUpb_ByNameMap_Funcs funcs = {
      {
          (void*)&upb_msgdef_nestedextcount,
          (void*)&upb_msgdef_nestedext,
          (void*)&PyUpb_FieldDescriptor_Get,
      },
      (void*)&PyUpb_Descriptor_LookupNestedExtension,
      (void*)&upb_fielddef_name,
  };
  return PyUpb_ByNameMap_New(&funcs, self->def, self->pool);
}

static PyObject* PyUpb_Descriptor_GetEnumTypes(PyObject* _self, void* closure) {
  PyUpb_DescriptorBase* self = (void*)_self;
  static PyUpb_GenericSequence_Funcs funcs = {
      (void*)&upb_msgdef_nestedenumcount,
      (void*)&upb_msgdef_nestedenum,
      (void*)&PyUpb_EnumDescriptor_Get,
  };
  return PyUpb_GenericSequence_New(&funcs, self->def, self->pool);
}

static PyObject* PyUpb_Descriptor_GetOneofs(PyObject* _self, void* closure) {
  PyUpb_DescriptorBase* self = (void*)_self;
  static PyUpb_GenericSequence_Funcs funcs = {
      (void*)&upb_msgdef_oneofcount,
      (void*)&upb_msgdef_oneof,
      (void*)&PyUpb_OneofDescriptor_Get,
  };
  return PyUpb_GenericSequence_New(&funcs, self->def, self->pool);
}

static PyObject* PyUpb_Descriptor_GetOptions(PyObject* _self, PyObject* args) {
  PyUpb_DescriptorBase* self = (void*)_self;
  return PyUpb_DescriptorBase_GetOptions(
      self, upb_msgdef_options(self->def),
      &google_protobuf_MessageOptions_msginit,
      "google.protobuf.MessageOptions");
}

static PyObject* PyUpb_Descriptor_CopyToProto(PyObject* _self,
                                              PyObject* py_proto) {
  return PyUpb_DescriptorBase_CopyToProto(
      _self, (PyUpb_ToProto_Func*)&upb_MessageDef_ToProto,
      &google_protobuf_DescriptorProto_msginit, py_proto);
}

static PyObject* PyUpb_Descriptor_EnumValueName(PyObject* _self,
                                                PyObject* args) {
  PyUpb_DescriptorBase* self = (void*)_self;
  const char* enum_name;
  int number;
  if (!PyArg_ParseTuple(args, "si", &enum_name, &number)) return NULL;
  const upb_enumdef* e =
      PyUpb_Descriptor_LookupNestedEnum(self->def, enum_name);
  if (!e) {
    PyErr_SetString(PyExc_KeyError, enum_name);
    return NULL;
  }
  const upb_enumvaldef* ev = upb_enumdef_lookupnum(e, number);
  if (!ev) {
    PyErr_Format(PyExc_KeyError, "%d", number);
    return NULL;
  }
  return PyUnicode_FromString(upb_enumvaldef_name(ev));
}

static PyObject* PyUpb_Descriptor_GetFieldsByName(PyObject* _self,
                                                  void* closure) {
  PyUpb_DescriptorBase* self = (void*)_self;
  static PyUpb_ByNameMap_Funcs funcs = {
      {
          (void*)&upb_msgdef_fieldcount,
          (void*)&upb_msgdef_field,
          (void*)&PyUpb_FieldDescriptor_Get,
      },
      (void*)&upb_msgdef_ntofz,
      (void*)&upb_fielddef_name,
  };
  return PyUpb_ByNameMap_New(&funcs, self->def, self->pool);
}

static PyObject* PyUpb_Descriptor_GetFieldsByCamelCaseName(PyObject* _self,
                                                           void* closure) {
  PyUpb_DescriptorBase* self = (void*)_self;
  static PyUpb_ByNameMap_Funcs funcs = {
      {
          (void*)&upb_msgdef_fieldcount,
          (void*)&upb_msgdef_field,
          (void*)&PyUpb_FieldDescriptor_Get,
      },
      (void*)&upb_msgdef_lookupjsonnamez,
      (void*)&upb_fielddef_jsonname,
  };
  return PyUpb_ByNameMap_New(&funcs, self->def, self->pool);
}

static PyObject* PyUpb_Descriptor_GetFieldsByNumber(PyObject* _self,
                                                    void* closure) {
  static PyUpb_ByNumberMap_Funcs funcs = {
      {
          (void*)&upb_msgdef_fieldcount,
          (void*)&upb_msgdef_field,
          (void*)&PyUpb_FieldDescriptor_Get,
      },
      (void*)&upb_msgdef_itof,
      (void*)&upb_fielddef_number,
  };
  PyUpb_DescriptorBase* self = (void*)_self;
  return PyUpb_ByNumberMap_New(&funcs, self->def, self->pool);
}

static PyObject* PyUpb_Descriptor_GetNestedTypes(PyObject* _self,
                                                 void* closure) {
  PyUpb_DescriptorBase* self = (void*)_self;
  static PyUpb_GenericSequence_Funcs funcs = {
      (void*)&upb_msgdef_nestedmsgcount,
      (void*)&upb_msgdef_nestedmsg,
      (void*)&PyUpb_Descriptor_Get,
  };
  return PyUpb_GenericSequence_New(&funcs, self->def, self->pool);
}

static PyObject* PyUpb_Descriptor_GetNestedTypesByName(PyObject* _self,
                                                       void* closure) {
  PyUpb_DescriptorBase* self = (void*)_self;
  static PyUpb_ByNameMap_Funcs funcs = {
      {
          (void*)&upb_msgdef_nestedmsgcount,
          (void*)&upb_msgdef_nestedmsg,
          (void*)&PyUpb_Descriptor_Get,
      },
      (void*)&PyUpb_Descriptor_LookupNestedMessage,
      (void*)&upb_msgdef_name,
  };
  return PyUpb_ByNameMap_New(&funcs, self->def, self->pool);
}

static PyObject* PyUpb_Descriptor_GetContainingType(PyObject* _self,
                                                    void* closure) {
  // upb does not natively store the lexical parent of a message type, but we
  // can derive it with some string manipulation and a lookup.
  PyUpb_DescriptorBase* self = (void*)_self;
  const upb_msgdef* m = self->def;
  const upb_filedef* file = upb_msgdef_file(m);
  const upb_symtab* symtab = upb_filedef_symtab(file);
  const char* full_name = upb_msgdef_fullname(m);
  const char* last_dot = strrchr(full_name, '.');
  if (!last_dot) Py_RETURN_NONE;
  const upb_msgdef* parent =
      upb_symtab_lookupmsg2(symtab, full_name, last_dot - full_name);
  if (!parent) Py_RETURN_NONE;
  return PyUpb_Descriptor_Get(parent);
}

static PyObject* PyUpb_Descriptor_GetEnumTypesByName(PyObject* _self,
                                                     void* closure) {
  PyUpb_DescriptorBase* self = (void*)_self;
  static PyUpb_ByNameMap_Funcs funcs = {
      {
          (void*)&upb_msgdef_nestedenumcount,
          (void*)&upb_msgdef_nestedenum,
          (void*)&PyUpb_EnumDescriptor_Get,
      },
      (void*)&PyUpb_Descriptor_LookupNestedEnum,
      (void*)&upb_enumdef_name,
  };
  return PyUpb_ByNameMap_New(&funcs, self->def, self->pool);
}

static PyObject* PyUpb_Descriptor_GetIsExtendable(PyObject* _self,
                                                  void* closure) {
  PyUpb_DescriptorBase* self = (void*)_self;
  if (upb_msgdef_extrangecount(self->def) > 0) {
    Py_RETURN_TRUE;
  } else {
    Py_RETURN_FALSE;
  }
}

static PyObject* PyUpb_Descriptor_GetFullName(PyObject* self, void* closure) {
  const upb_msgdef* msgdef = PyUpb_Descriptor_GetDef(self);
  return PyUnicode_FromString(upb_msgdef_fullname(msgdef));
}

static PyObject* PyUpb_Descriptor_GetConcreteClass(PyObject* self,
                                                   void* closure) {
  const upb_msgdef* msgdef = PyUpb_Descriptor_GetDef(self);
  return PyUpb_Descriptor_GetClass(msgdef);
}

static PyObject* PyUpb_Descriptor_GetFile(PyObject* self, void* closure) {
  const upb_msgdef* msgdef = PyUpb_Descriptor_GetDef(self);
  return PyUpb_FileDescriptor_Get(upb_msgdef_file(msgdef));
}

static PyObject* PyUpb_Descriptor_GetFields(PyObject* _self, void* closure) {
  PyUpb_DescriptorBase* self = (void*)_self;
  static PyUpb_GenericSequence_Funcs funcs = {
      (void*)&upb_msgdef_fieldcount,
      (void*)&upb_msgdef_field,
      (void*)&PyUpb_FieldDescriptor_Get,
  };
  return PyUpb_GenericSequence_New(&funcs, self->def, self->pool);
}

static PyObject* PyUpb_Descriptor_GetHasOptions(PyObject* _self,
                                                void* closure) {
  PyUpb_DescriptorBase* self = (void*)_self;
  return PyBool_FromLong(upb_msgdef_hasoptions(self->def));
}

static PyObject* PyUpb_Descriptor_GetName(PyObject* self, void* closure) {
  const upb_msgdef* msgdef = PyUpb_Descriptor_GetDef(self);
  return PyUnicode_FromString(upb_msgdef_name(msgdef));
}

static PyObject* PyUpb_Descriptor_GetOneofsByName(PyObject* _self,
                                                  void* closure) {
  PyUpb_DescriptorBase* self = (void*)_self;
  static PyUpb_ByNameMap_Funcs funcs = {
      {
          (void*)&upb_msgdef_oneofcount,
          (void*)&upb_msgdef_oneof,
          (void*)&PyUpb_OneofDescriptor_Get,
      },
      (void*)&upb_msgdef_ntooz,
      (void*)&upb_oneofdef_name,
  };
  return PyUpb_ByNameMap_New(&funcs, self->def, self->pool);
}

static PyObject* PyUpb_Descriptor_GetSyntax(PyObject* self, void* closure) {
  const upb_msgdef* msgdef = PyUpb_Descriptor_GetDef(self);
  const char* syntax =
      upb_msgdef_syntax(msgdef) == UPB_SYNTAX_PROTO2 ? "proto2" : "proto3";
  return PyUnicode_InternFromString(syntax);
}

static PyGetSetDef PyUpb_Descriptor_Getters[] = {
    {"name", PyUpb_Descriptor_GetName, NULL, "Last name"},
    {"full_name", PyUpb_Descriptor_GetFullName, NULL, "Full name"},
    {"_concrete_class", PyUpb_Descriptor_GetConcreteClass, NULL,
     "concrete class"},
    {"file", PyUpb_Descriptor_GetFile, NULL, "File descriptor"},
    {"fields", PyUpb_Descriptor_GetFields, NULL, "Fields sequence"},
    {"fields_by_name", PyUpb_Descriptor_GetFieldsByName, NULL,
     "Fields by name"},
    {"fields_by_camelcase_name", PyUpb_Descriptor_GetFieldsByCamelCaseName,
     NULL, "Fields by camelCase name"},
    {"fields_by_number", PyUpb_Descriptor_GetFieldsByNumber, NULL,
     "Fields by number"},
    {"nested_types", PyUpb_Descriptor_GetNestedTypes, NULL,
     "Nested types sequence"},
    {"nested_types_by_name", PyUpb_Descriptor_GetNestedTypesByName, NULL,
     "Nested types by name"},
    {"extensions", PyUpb_Descriptor_GetExtensions, NULL, "Extensions Sequence"},
    {"extensions_by_name", PyUpb_Descriptor_GetExtensionsByName, NULL,
     "Extensions by name"},
    {"extension_ranges", PyUpb_Descriptor_GetExtensionRanges, NULL,
     "Extension ranges"},
    {"enum_types", PyUpb_Descriptor_GetEnumTypes, NULL, "Enum sequence"},
    {"enum_types_by_name", PyUpb_Descriptor_GetEnumTypesByName, NULL,
     "Enum types by name"},
    // TODO(https://github.com/protocolbuffers/upb/issues/459)
    //{ "enum_values_by_name", PyUpb_Descriptor_GetEnumValuesByName, NULL,
    //  "Enum values by name"},
    {"oneofs_by_name", PyUpb_Descriptor_GetOneofsByName, NULL,
     "Oneofs by name"},
    {"oneofs", PyUpb_Descriptor_GetOneofs, NULL, "Oneofs Sequence"},
    {"containing_type", PyUpb_Descriptor_GetContainingType, NULL,
     "Containing type"},
    {"is_extendable", PyUpb_Descriptor_GetIsExtendable, NULL},
    {"has_options", PyUpb_Descriptor_GetHasOptions, NULL, "Has Options"},
    {"syntax", &PyUpb_Descriptor_GetSyntax, NULL, "Syntax"},
    {NULL}};

static PyMethodDef PyUpb_Descriptor_Methods[] = {
    {"GetOptions", PyUpb_Descriptor_GetOptions, METH_NOARGS},
    {"CopyToProto", PyUpb_Descriptor_CopyToProto, METH_O},
    {"EnumValueName", PyUpb_Descriptor_EnumValueName, METH_VARARGS},
    {NULL}};

static PyType_Slot PyUpb_Descriptor_Slots[] = {
    DESCRIPTOR_BASE_SLOTS,
    {Py_tp_methods, PyUpb_Descriptor_Methods},
    {Py_tp_getset, PyUpb_Descriptor_Getters},
    {0, NULL}};

static PyType_Spec PyUpb_Descriptor_Spec = {
    PYUPB_MODULE_NAME ".Descriptor",  // tp_name
    sizeof(PyUpb_DescriptorBase),     // tp_basicsize
    0,                                // tp_itemsize
    Py_TPFLAGS_DEFAULT,               // tp_flags
    PyUpb_Descriptor_Slots,
};

const upb_msgdef* PyUpb_Descriptor_GetDef(PyObject* _self) {
  PyUpb_DescriptorBase* self =
      PyUpb_DescriptorBase_Check(_self, kPyUpb_Descriptor);
  return self ? self->def : NULL;
}

// -----------------------------------------------------------------------------
// EnumDescriptor
// -----------------------------------------------------------------------------

PyObject* PyUpb_EnumDescriptor_Get(const upb_enumdef* enumdef) {
  const upb_filedef* file = upb_enumdef_file(enumdef);
  return PyUpb_DescriptorBase_Get(kPyUpb_EnumDescriptor, enumdef, file);
}

const upb_enumdef* PyUpb_EnumDescriptor_GetDef(PyObject* _self) {
  PyUpb_DescriptorBase* self =
      PyUpb_DescriptorBase_Check(_self, kPyUpb_EnumDescriptor);
  return self ? self->def : NULL;
}

static PyObject* PyUpb_EnumDescriptor_GetFullName(PyObject* self,
                                                  void* closure) {
  const upb_enumdef* enumdef = PyUpb_EnumDescriptor_GetDef(self);
  return PyUnicode_FromString(upb_enumdef_fullname(enumdef));
}

static PyObject* PyUpb_EnumDescriptor_GetName(PyObject* self, void* closure) {
  const upb_enumdef* enumdef = PyUpb_EnumDescriptor_GetDef(self);
  return PyUnicode_FromString(upb_enumdef_name(enumdef));
}

static PyObject* PyUpb_EnumDescriptor_GetFile(PyObject* self, void* closure) {
  const upb_enumdef* enumdef = PyUpb_EnumDescriptor_GetDef(self);
  return PyUpb_FileDescriptor_Get(upb_enumdef_file(enumdef));
}

static PyObject* PyUpb_EnumDescriptor_GetValues(PyObject* _self,
                                                void* closure) {
  PyUpb_DescriptorBase* self = (void*)_self;
  static PyUpb_GenericSequence_Funcs funcs = {
      (void*)&upb_enumdef_valuecount,
      (void*)&upb_enumdef_value,
      (void*)&PyUpb_EnumValueDescriptor_Get,
  };
  return PyUpb_GenericSequence_New(&funcs, self->def, self->pool);
}

static PyObject* PyUpb_EnumDescriptor_GetValuesByName(PyObject* _self,
                                                      void* closure) {
  static PyUpb_ByNameMap_Funcs funcs = {
      {
          (void*)&upb_enumdef_valuecount,
          (void*)&upb_enumdef_value,
          (void*)&PyUpb_EnumValueDescriptor_Get,
      },
      (void*)&upb_enumdef_lookupnamez,
      (void*)&upb_enumvaldef_name,
  };
  PyUpb_DescriptorBase* self = (void*)_self;
  return PyUpb_ByNameMap_New(&funcs, self->def, self->pool);
}

static PyObject* PyUpb_EnumDescriptor_GetValuesByNumber(PyObject* _self,
                                                        void* closure) {
  static PyUpb_ByNumberMap_Funcs funcs = {
      {
          (void*)&upb_enumdef_valuecount,
          (void*)&upb_enumdef_value,
          (void*)&PyUpb_EnumValueDescriptor_Get,
      },
      (void*)&upb_enumdef_lookupnum,
      (void*)&upb_enumvaldef_number,
  };
  PyUpb_DescriptorBase* self = (void*)_self;
  return PyUpb_ByNumberMap_New(&funcs, self->def, self->pool);
}

static PyObject* PyUpb_EnumDescriptor_GetContainingType(PyObject* _self,
                                                        void* closure) {
  PyUpb_DescriptorBase* self = (void*)_self;
  const upb_msgdef* m = upb_enumdef_containingtype(self->def);
  if (!m) Py_RETURN_NONE;
  return PyUpb_Descriptor_Get(m);
}

static PyObject* PyUpb_EnumDescriptor_GetHasOptions(PyObject* _self,
                                                    void* closure) {
  PyUpb_DescriptorBase* self = (void*)_self;
  return PyBool_FromLong(upb_enumdef_hasoptions(self->def));
}

static PyObject* PyUpb_EnumDescriptor_GetOptions(PyObject* _self,
                                                 PyObject* args) {
  PyUpb_DescriptorBase* self = (void*)_self;
  return PyUpb_DescriptorBase_GetOptions(self, upb_enumdef_options(self->def),
                                         &google_protobuf_EnumOptions_msginit,
                                         "google.protobuf.EnumOptions");
}

static PyObject* PyUpb_EnumDescriptor_CopyToProto(PyObject* _self,
                                                  PyObject* py_proto) {
  return PyUpb_DescriptorBase_CopyToProto(
      _self, (PyUpb_ToProto_Func*)&upb_EnumDef_ToProto,
      &google_protobuf_EnumDescriptorProto_msginit, py_proto);
}

static PyGetSetDef PyUpb_EnumDescriptor_Getters[] = {
    {"full_name", PyUpb_EnumDescriptor_GetFullName, NULL, "Full name"},
    {"name", PyUpb_EnumDescriptor_GetName, NULL, "last name"},
    {"file", PyUpb_EnumDescriptor_GetFile, NULL, "File descriptor"},
    {"values", PyUpb_EnumDescriptor_GetValues, NULL, "values"},
    {"values_by_name", PyUpb_EnumDescriptor_GetValuesByName, NULL,
     "Enum values by name"},
    {"values_by_number", PyUpb_EnumDescriptor_GetValuesByNumber, NULL,
     "Enum values by number"},
    {"containing_type", PyUpb_EnumDescriptor_GetContainingType, NULL,
     "Containing type"},
    {"has_options", PyUpb_EnumDescriptor_GetHasOptions, NULL, "Has Options"},
    {NULL}};

static PyMethodDef PyUpb_EnumDescriptor_Methods[] = {
    {"GetOptions", PyUpb_EnumDescriptor_GetOptions, METH_NOARGS},
    {"CopyToProto", PyUpb_EnumDescriptor_CopyToProto, METH_O},
    {NULL}};

static PyType_Slot PyUpb_EnumDescriptor_Slots[] = {
    DESCRIPTOR_BASE_SLOTS,
    {Py_tp_methods, PyUpb_EnumDescriptor_Methods},
    {Py_tp_getset, PyUpb_EnumDescriptor_Getters},
    {0, NULL}};

static PyType_Spec PyUpb_EnumDescriptor_Spec = {
    PYUPB_MODULE_NAME ".EnumDescriptor",  // tp_name
    sizeof(PyUpb_DescriptorBase),         // tp_basicsize
    0,                                    // tp_itemsize
    Py_TPFLAGS_DEFAULT,                   // tp_flags
    PyUpb_EnumDescriptor_Slots,
};

// -----------------------------------------------------------------------------
// EnumValueDescriptor
// -----------------------------------------------------------------------------

PyObject* PyUpb_EnumValueDescriptor_Get(const upb_enumvaldef* ev) {
  const upb_filedef* file = upb_enumdef_file(upb_enumvaldef_enum(ev));
  return PyUpb_DescriptorBase_Get(kPyUpb_EnumValueDescriptor, ev, file);
}

static PyObject* PyUpb_EnumValueDescriptor_GetName(PyObject* self,
                                                   void* closure) {
  PyUpb_DescriptorBase* base = (PyUpb_DescriptorBase*)self;
  return PyUnicode_FromString(upb_enumvaldef_name(base->def));
}

static PyObject* PyUpb_EnumValueDescriptor_GetNumber(PyObject* self,
                                                     void* closure) {
  PyUpb_DescriptorBase* base = (PyUpb_DescriptorBase*)self;
  return PyLong_FromLong(upb_enumvaldef_number(base->def));
}

static PyObject* PyUpb_EnumValueDescriptor_GetType(PyObject* self,
                                                   void* closure) {
  PyUpb_DescriptorBase* base = (PyUpb_DescriptorBase*)self;
  return PyUpb_EnumDescriptor_Get(upb_enumvaldef_enum(base->def));
}

static PyObject* PyUpb_EnumValueDescriptor_GetHasOptions(PyObject* _self,
                                                         void* closure) {
  PyUpb_DescriptorBase* self = (void*)_self;
  return PyBool_FromLong(upb_enumvaldef_hasoptions(self->def));
}

static PyObject* PyUpb_EnumValueDescriptor_GetOptions(PyObject* _self,
                                                      PyObject* args) {
  PyUpb_DescriptorBase* self = (void*)_self;
  return PyUpb_DescriptorBase_GetOptions(
      self, upb_enumvaldef_options(self->def),
      &google_protobuf_EnumValueOptions_msginit,
      "google.protobuf.EnumValueOptions");
}

static PyGetSetDef PyUpb_EnumValueDescriptor_Getters[] = {
    {"name", PyUpb_EnumValueDescriptor_GetName, NULL, "name"},
    {"number", PyUpb_EnumValueDescriptor_GetNumber, NULL, "number"},
    // TODO(https://github.com/protocolbuffers/upb/issues/459)
    //{ "index", (getter)GetIndex, NULL, "index"},
    {"type", PyUpb_EnumValueDescriptor_GetType, NULL, "index"},
    {"has_options", PyUpb_EnumValueDescriptor_GetHasOptions, NULL,
     "Has Options"},
    {NULL}};

static PyMethodDef PyUpb_EnumValueDescriptor_Methods[] = {
    {
        "GetOptions",
        PyUpb_EnumValueDescriptor_GetOptions,
        METH_NOARGS,
    },
    {NULL}};

static PyType_Slot PyUpb_EnumValueDescriptor_Slots[] = {
    DESCRIPTOR_BASE_SLOTS,
    {Py_tp_methods, PyUpb_EnumValueDescriptor_Methods},
    {Py_tp_getset, PyUpb_EnumValueDescriptor_Getters},
    {0, NULL}};

static PyType_Spec PyUpb_EnumValueDescriptor_Spec = {
    PYUPB_MODULE_NAME ".EnumValueDescriptor",  // tp_name
    sizeof(PyUpb_DescriptorBase),              // tp_basicsize
    0,                                         // tp_itemsize
    Py_TPFLAGS_DEFAULT,                        // tp_flags
    PyUpb_EnumValueDescriptor_Slots,
};

// -----------------------------------------------------------------------------
// FieldDescriptor
// -----------------------------------------------------------------------------

const upb_fielddef* PyUpb_FieldDescriptor_GetDef(PyObject* _self) {
  PyUpb_DescriptorBase* self =
      PyUpb_DescriptorBase_Check(_self, kPyUpb_FieldDescriptor);
  return self ? self->def : NULL;
}

PyObject* PyUpb_FieldDescriptor_Get(const upb_fielddef* field) {
  const upb_filedef* file = upb_fielddef_file(field);
  return PyUpb_DescriptorBase_Get(kPyUpb_FieldDescriptor, field, file);
}

static PyObject* PyUpb_FieldDescriptor_GetFullName(PyUpb_DescriptorBase* self,
                                                   void* closure) {
  return PyUnicode_FromString(upb_fielddef_fullname(self->def));
}

static PyObject* PyUpb_FieldDescriptor_GetName(PyUpb_DescriptorBase* self,
                                               void* closure) {
  return PyUnicode_FromString(upb_fielddef_name(self->def));
}

static PyObject* PyUpb_FieldDescriptor_GetCamelCaseName(
    PyUpb_DescriptorBase* self, void* closure) {
  // TODO: Ok to use jsonname here?
  return PyUnicode_FromString(upb_fielddef_jsonname(self->def));
}

static PyObject* PyUpb_FieldDescriptor_GetJsonName(PyUpb_DescriptorBase* self,
                                                   void* closure) {
  return PyUnicode_FromString(upb_fielddef_jsonname(self->def));
}

static PyObject* PyUpb_FieldDescriptor_GetFile(PyUpb_DescriptorBase* self,
                                               void* closure) {
  const upb_filedef* file = upb_fielddef_file(self->def);
  if (!file) Py_RETURN_NONE;
  return PyUpb_FileDescriptor_Get(file);
}

static PyObject* PyUpb_FieldDescriptor_GetType(PyUpb_DescriptorBase* self,
                                               void* closure) {
  return PyLong_FromLong(upb_fielddef_descriptortype(self->def));
}

static PyObject* PyUpb_FieldDescriptor_GetCppType(PyUpb_DescriptorBase* self,
                                                  void* closure) {
  // Enum values copied from descriptor.h in C++.
  enum CppType {
    CPPTYPE_INT32 = 1,     // TYPE_INT32, TYPE_SINT32, TYPE_SFIXED32
    CPPTYPE_INT64 = 2,     // TYPE_INT64, TYPE_SINT64, TYPE_SFIXED64
    CPPTYPE_UINT32 = 3,    // TYPE_UINT32, TYPE_FIXED32
    CPPTYPE_UINT64 = 4,    // TYPE_UINT64, TYPE_FIXED64
    CPPTYPE_DOUBLE = 5,    // TYPE_DOUBLE
    CPPTYPE_FLOAT = 6,     // TYPE_FLOAT
    CPPTYPE_BOOL = 7,      // TYPE_BOOL
    CPPTYPE_ENUM = 8,      // TYPE_ENUM
    CPPTYPE_STRING = 9,    // TYPE_STRING, TYPE_BYTES
    CPPTYPE_MESSAGE = 10,  // TYPE_MESSAGE, TYPE_GROUP
  };
  static const uint8_t cpp_types[] = {
    -1,
    [UPB_TYPE_INT32] = CPPTYPE_INT32,
    [UPB_TYPE_INT64] = CPPTYPE_INT64,
    [UPB_TYPE_UINT32] = CPPTYPE_UINT32,
    [UPB_TYPE_UINT64] = CPPTYPE_UINT64,
    [UPB_TYPE_DOUBLE] = CPPTYPE_DOUBLE,
    [UPB_TYPE_FLOAT] = CPPTYPE_FLOAT,
    [UPB_TYPE_BOOL] = CPPTYPE_BOOL,
    [UPB_TYPE_ENUM] = CPPTYPE_ENUM,
    [UPB_TYPE_STRING] = CPPTYPE_STRING,
    [UPB_TYPE_BYTES] = CPPTYPE_STRING,
    [UPB_TYPE_MESSAGE] = CPPTYPE_MESSAGE,
  };
  return PyLong_FromLong(cpp_types[upb_fielddef_type(self->def)]);
}

static PyObject* PyUpb_FieldDescriptor_GetLabel(PyUpb_DescriptorBase* self,
                                                void* closure) {
  return PyLong_FromLong(upb_fielddef_label(self->def));
}

static PyObject* PyUpb_FieldDescriptor_GetIsExtension(
    PyUpb_DescriptorBase* self, void* closure) {
  return PyBool_FromLong(upb_fielddef_isextension(self->def));
}

static PyObject* PyUpb_FieldDescriptor_GetNumber(PyUpb_DescriptorBase* self,
                                                 void* closure) {
  return PyLong_FromLong(upb_fielddef_number(self->def));
}

static PyObject* PyUpb_FieldDescriptor_GetIndex(PyUpb_DescriptorBase* self,
                                                void* closure) {
  return PyLong_FromLong(upb_fielddef_index(self->def));
}

static PyObject* PyUpb_FieldDescriptor_GetMessageType(
    PyUpb_DescriptorBase* self, void* closure) {
  const upb_msgdef* subdef = upb_fielddef_msgsubdef(self->def);
  if (!subdef) Py_RETURN_NONE;
  return PyUpb_Descriptor_Get(subdef);
}

static PyObject* PyUpb_FieldDescriptor_GetEnumType(PyUpb_DescriptorBase* self,
                                                   void* closure) {
  const upb_enumdef* enumdef = upb_fielddef_enumsubdef(self->def);
  if (!enumdef) Py_RETURN_NONE;
  return PyUpb_EnumDescriptor_Get(enumdef);
}

static PyObject* PyUpb_FieldDescriptor_GetContainingType(
    PyUpb_DescriptorBase* self, void* closure) {
  const upb_msgdef* m = upb_fielddef_containingtype(self->def);
  if (!m) Py_RETURN_NONE;
  return PyUpb_Descriptor_Get(m);
}

static PyObject* PyUpb_FieldDescriptor_HasDefaultValue(
    PyUpb_DescriptorBase* self, void* closure) {
  return PyBool_FromLong(upb_fielddef_hasdefault(self->def));
}

static PyObject* PyUpb_FieldDescriptor_GetDefaultValue(
    PyUpb_DescriptorBase* self, void* closure) {
  const upb_fielddef* f = self->def;
  if (upb_fielddef_isseq(f)) return PyList_New(0);
  if (upb_fielddef_issubmsg(f)) Py_RETURN_NONE;
  return PyUpb_UpbToPy(upb_fielddef_default(self->def), self->def, NULL);
}

static PyObject* PyUpb_FieldDescriptor_GetContainingOneof(
    PyUpb_DescriptorBase* self, void* closure) {
  const upb_oneofdef* oneof = upb_fielddef_containingoneof(self->def);
  if (!oneof) Py_RETURN_NONE;
  return PyUpb_OneofDescriptor_Get(oneof);
}

static PyObject* PyUpb_FieldDescriptor_GetHasOptions(
    PyUpb_DescriptorBase* _self, void* closure) {
  PyUpb_DescriptorBase* self = (void*)_self;
  return PyBool_FromLong(upb_fielddef_hasoptions(self->def));
}

static PyObject* PyUpb_FieldDescriptor_GetOptions(PyObject* _self,
                                                  PyObject* args) {
  PyUpb_DescriptorBase* self = (void*)_self;
  return PyUpb_DescriptorBase_GetOptions(self, upb_fielddef_options(self->def),
                                         &google_protobuf_FieldOptions_msginit,
                                         "google.protobuf.FieldOptions");
}

static PyGetSetDef PyUpb_FieldDescriptor_Getters[] = {
    {"full_name", (getter)PyUpb_FieldDescriptor_GetFullName, NULL, "Full name"},
    {"name", (getter)PyUpb_FieldDescriptor_GetName, NULL, "Unqualified name"},
    {"camelcase_name", (getter)PyUpb_FieldDescriptor_GetCamelCaseName, NULL,
     "CamelCase name"},
    {"json_name", (getter)PyUpb_FieldDescriptor_GetJsonName, NULL, "Json name"},
    {"file", (getter)PyUpb_FieldDescriptor_GetFile, NULL, "File Descriptor"},
    {"type", (getter)PyUpb_FieldDescriptor_GetType, NULL, "Type"},
    {"cpp_type", (getter)PyUpb_FieldDescriptor_GetCppType, NULL, "C++ Type"},
    {"label", (getter)PyUpb_FieldDescriptor_GetLabel, NULL, "Label"},
    {"number", (getter)PyUpb_FieldDescriptor_GetNumber, NULL, "Number"},
    {"index", (getter)PyUpb_FieldDescriptor_GetIndex, NULL, "Index"},
    {"default_value", (getter)PyUpb_FieldDescriptor_GetDefaultValue, NULL,
     "Default Value"},
    {"has_default_value", (getter)PyUpb_FieldDescriptor_HasDefaultValue},
    {"is_extension", (getter)PyUpb_FieldDescriptor_GetIsExtension, NULL, "ID"},
    // TODO(https://github.com/protocolbuffers/upb/issues/459)
    //{ "id", (getter)GetID, NULL, "ID"},
    {"message_type", (getter)PyUpb_FieldDescriptor_GetMessageType, NULL,
     "Message type"},
    {"enum_type", (getter)PyUpb_FieldDescriptor_GetEnumType, NULL, "Enum type"},
    {"containing_type", (getter)PyUpb_FieldDescriptor_GetContainingType, NULL,
     "Containing type"},
    // TODO(https://github.com/protocolbuffers/upb/issues/459)
    //{ "extension_scope", (getter)GetExtensionScope, (setter)NULL,
    //  "Extension scope"},
    {"containing_oneof", (getter)PyUpb_FieldDescriptor_GetContainingOneof, NULL,
     "Containing oneof"},
    {"has_options", (getter)PyUpb_FieldDescriptor_GetHasOptions, NULL,
     "Has Options"},
    // TODO(https://github.com/protocolbuffers/upb/issues/459)
    //{ "_options",
    //(getter)NULL, (setter)SetOptions, "Options"}, { "_serialized_options",
    //(getter)NULL, (setter)SetSerializedOptions, "Serialized Options"},
    {NULL}};

static PyMethodDef PyUpb_FieldDescriptor_Methods[] = {
    {
        "GetOptions",
        PyUpb_FieldDescriptor_GetOptions,
        METH_NOARGS,
    },
    {NULL}};

static PyType_Slot PyUpb_FieldDescriptor_Slots[] = {
    DESCRIPTOR_BASE_SLOTS,
    {Py_tp_methods, PyUpb_FieldDescriptor_Methods},
    {Py_tp_getset, PyUpb_FieldDescriptor_Getters},
    {0, NULL}};

static PyType_Spec PyUpb_FieldDescriptor_Spec = {
    PYUPB_MODULE_NAME ".FieldDescriptor",
    sizeof(PyUpb_DescriptorBase),
    0,  // tp_itemsize
    Py_TPFLAGS_DEFAULT,
    PyUpb_FieldDescriptor_Slots,
};

// -----------------------------------------------------------------------------
// FileDescriptor
// -----------------------------------------------------------------------------

PyObject* PyUpb_FileDescriptor_Get(const upb_filedef* file) {
  return PyUpb_DescriptorBase_Get(kPyUpb_FileDescriptor, file, file);
}

// These are not provided on upb_filedef because they use the underlying
// symtab's hash table. This works for Python because everything happens under
// the GIL, but in general the caller has to guarantee that the symtab is not
// being mutated concurrently.
typedef const void* PyUpb_FileDescriptor_LookupFunc(const upb_symtab*,
                                                    const char*);

static const void* PyUpb_FileDescriptor_NestedLookup(
    const upb_filedef* filedef, const char* name,
    PyUpb_FileDescriptor_LookupFunc* func) {
  const upb_symtab* symtab = upb_filedef_symtab(filedef);
  const char* package = upb_filedef_package(filedef);
  if (package) {
    PyObject* qname = PyUnicode_FromFormat("%s.%s", package, name);
    const void* ret = func(symtab, PyUnicode_AsUTF8AndSize(qname, NULL));
    Py_DECREF(qname);
    return ret;
  } else {
    return func(symtab, name);
  }
}

static const void* PyUpb_FileDescriptor_LookupMessage(
    const upb_filedef* filedef, const char* name) {
  return PyUpb_FileDescriptor_NestedLookup(filedef, name,
                                           (void*)&upb_symtab_lookupmsg);
}

static const void* PyUpb_FileDescriptor_LookupEnum(const upb_filedef* filedef,
                                                   const char* name) {
  return PyUpb_FileDescriptor_NestedLookup(filedef, name,
                                           (void*)&upb_symtab_lookupenum);
}

static const void* PyUpb_FileDescriptor_LookupExtension(
    const upb_filedef* filedef, const char* name) {
  return PyUpb_FileDescriptor_NestedLookup(filedef, name,
                                           (void*)&upb_symtab_lookupext);
}

static const void* PyUpb_FileDescriptor_LookupService(
    const upb_filedef* filedef, const char* name) {
  return PyUpb_FileDescriptor_NestedLookup(filedef, name,
                                           (void*)&upb_symtab_lookupservice);
}

static PyObject* PyUpb_FileDescriptor_GetName(PyUpb_DescriptorBase* self,
                                              void* closure) {
  return PyUnicode_FromString(upb_filedef_name(self->def));
}

static PyObject* PyUpb_FileDescriptor_GetPool(PyObject* _self, void* closure) {
  PyUpb_DescriptorBase* self = (PyUpb_DescriptorBase*)_self;
  Py_INCREF(self->pool);
  return self->pool;
}

static PyObject* PyUpb_FileDescriptor_GetPackage(PyObject* _self,
                                                 void* closure) {
  PyUpb_DescriptorBase* self = (PyUpb_DescriptorBase*)_self;
  return PyUnicode_FromString(upb_filedef_package(self->def));
}

static PyObject* PyUpb_FileDescriptor_GetSerializedPb(PyObject* self,
                                                      void* closure) {
  return PyUpb_DescriptorBase_GetSerializedProto(
      self, (PyUpb_ToProto_Func*)&upb_FileDef_ToProto,
      &google_protobuf_FileDescriptorProto_msginit);
}

static PyObject* PyUpb_FileDescriptor_GetMessageTypesByName(PyObject* _self,
                                                            void* closure) {
  static PyUpb_ByNameMap_Funcs funcs = {
      {
          (void*)&upb_filedef_toplvlmsgcount,
          (void*)&upb_filedef_toplvlmsg,
          (void*)&PyUpb_Descriptor_Get,
      },
      (void*)&PyUpb_FileDescriptor_LookupMessage,
      (void*)&upb_msgdef_name,
  };
  PyUpb_DescriptorBase* self = (void*)_self;
  return PyUpb_ByNameMap_New(&funcs, self->def, self->pool);
}

static PyObject* PyUpb_FileDescriptor_GetEnumTypesByName(PyObject* _self,
                                                         void* closure) {
  static PyUpb_ByNameMap_Funcs funcs = {
      {
          (void*)&upb_filedef_toplvlenumcount,
          (void*)&upb_filedef_toplvlenum,
          (void*)&PyUpb_EnumDescriptor_Get,
      },
      (void*)&PyUpb_FileDescriptor_LookupEnum,
      (void*)&upb_enumdef_name,
  };
  PyUpb_DescriptorBase* self = (void*)_self;
  return PyUpb_ByNameMap_New(&funcs, self->def, self->pool);
}

static PyObject* PyUpb_FileDescriptor_GetExtensionsByName(PyObject* _self,
                                                          void* closure) {
  static PyUpb_ByNameMap_Funcs funcs = {
      {
          (void*)&upb_filedef_toplvlextcount,
          (void*)&upb_filedef_toplvlext,
          (void*)&PyUpb_FieldDescriptor_Get,
      },
      (void*)&PyUpb_FileDescriptor_LookupExtension,
      (void*)&upb_fielddef_name,
  };
  PyUpb_DescriptorBase* self = (void*)_self;
  return PyUpb_ByNameMap_New(&funcs, self->def, self->pool);
}

static PyObject* PyUpb_FileDescriptor_GetServicesByName(PyObject* _self,
                                                        void* closure) {
  static PyUpb_ByNameMap_Funcs funcs = {
      {
          (void*)&upb_filedef_servicecount,
          (void*)&upb_filedef_service,
          (void*)&PyUpb_ServiceDescriptor_Get,
      },
      (void*)&PyUpb_FileDescriptor_LookupService,
      (void*)&upb_servicedef_name,
  };
  PyUpb_DescriptorBase* self = (void*)_self;
  return PyUpb_ByNameMap_New(&funcs, self->def, self->pool);
}

static PyObject* PyUpb_FileDescriptor_GetDependencies(PyObject* _self,
                                                      void* closure) {
  PyUpb_DescriptorBase* self = (void*)_self;
  static PyUpb_GenericSequence_Funcs funcs = {
      (void*)&upb_filedef_depcount,
      (void*)&upb_filedef_dep,
      (void*)&PyUpb_FileDescriptor_Get,
  };
  return PyUpb_GenericSequence_New(&funcs, self->def, self->pool);
}

static PyObject* PyUpb_FileDescriptor_GetPublicDependencies(PyObject* _self,
                                                            void* closure) {
  PyUpb_DescriptorBase* self = (void*)_self;
  static PyUpb_GenericSequence_Funcs funcs = {
      (void*)&upb_filedef_publicdepcount,
      (void*)&upb_filedef_publicdep,
      (void*)&PyUpb_FileDescriptor_Get,
  };
  return PyUpb_GenericSequence_New(&funcs, self->def, self->pool);
}

static PyObject* PyUpb_FileDescriptor_GetSyntax(PyObject* _self,
                                                void* closure) {
  PyUpb_DescriptorBase* self = (void*)_self;
  const char* syntax =
      upb_filedef_syntax(self->def) == UPB_SYNTAX_PROTO2 ? "proto2" : "proto3";
  return PyUnicode_FromString(syntax);
}

static PyObject* PyUpb_FileDescriptor_GetHasOptions(PyObject* _self,
                                                    void* closure) {
  PyUpb_DescriptorBase* self = (void*)_self;
  return PyBool_FromLong(upb_filedef_hasoptions(self->def));
}

static PyObject* PyUpb_FileDescriptor_GetOptions(PyObject* _self,
                                                 PyObject* args) {
  PyUpb_DescriptorBase* self = (void*)_self;
  return PyUpb_DescriptorBase_GetOptions(self, upb_filedef_options(self->def),
                                         &google_protobuf_FileOptions_msginit,
                                         "google.protobuf.FileOptions");
}

static PyObject* PyUpb_FileDescriptor_CopyToProto(PyObject* _self,
                                                  PyObject* py_proto) {
  return PyUpb_DescriptorBase_CopyToProto(
      _self, (PyUpb_ToProto_Func*)&upb_FileDef_ToProto,
      &google_protobuf_FileDescriptorProto_msginit, py_proto);
}

static PyGetSetDef PyUpb_FileDescriptor_Getters[] = {
    {"pool", PyUpb_FileDescriptor_GetPool, NULL, "pool"},
    {"name", (getter)PyUpb_FileDescriptor_GetName, NULL, "name"},
    {"package", PyUpb_FileDescriptor_GetPackage, NULL, "package"},
    {"serialized_pb", PyUpb_FileDescriptor_GetSerializedPb},
    {"message_types_by_name", PyUpb_FileDescriptor_GetMessageTypesByName, NULL,
     "Messages by name"},
    {"enum_types_by_name", PyUpb_FileDescriptor_GetEnumTypesByName, NULL,
     "Enums by name"},
    {"extensions_by_name", PyUpb_FileDescriptor_GetExtensionsByName, NULL,
     "Extensions by name"},
    {"services_by_name", PyUpb_FileDescriptor_GetServicesByName, NULL,
     "Services by name"},
    {"dependencies", PyUpb_FileDescriptor_GetDependencies, NULL,
     "Dependencies"},
    {"public_dependencies", PyUpb_FileDescriptor_GetPublicDependencies, NULL,
     "Dependencies"},
    {"has_options", PyUpb_FileDescriptor_GetHasOptions, NULL, "Has Options"},
    {"syntax", PyUpb_FileDescriptor_GetSyntax, (setter)NULL, "Syntax"},
    {NULL},
};

static PyMethodDef PyUpb_FileDescriptor_Methods[] = {
    {"GetOptions", PyUpb_FileDescriptor_GetOptions, METH_NOARGS},
    {"CopyToProto", PyUpb_FileDescriptor_CopyToProto, METH_O},
    {NULL}};

static PyType_Slot PyUpb_FileDescriptor_Slots[] = {
    DESCRIPTOR_BASE_SLOTS,
    {Py_tp_methods, PyUpb_FileDescriptor_Methods},
    {Py_tp_getset, PyUpb_FileDescriptor_Getters},
    {0, NULL}};

static PyType_Spec PyUpb_FileDescriptor_Spec = {
    PYUPB_MODULE_NAME ".FileDescriptor",  // tp_name
    sizeof(PyUpb_DescriptorBase),         // tp_basicsize
    0,                                    // tp_itemsize
    Py_TPFLAGS_DEFAULT,                   // tp_flags
    PyUpb_FileDescriptor_Slots,
};

const upb_filedef* PyUpb_FileDescriptor_GetDef(PyObject* _self) {
  PyUpb_DescriptorBase* self =
      PyUpb_DescriptorBase_Check(_self, kPyUpb_FileDescriptor);
  return self ? self->def : NULL;
}

// -----------------------------------------------------------------------------
// MethodDescriptor
// -----------------------------------------------------------------------------

const upb_methoddef* PyUpb_MethodDescriptor_GetDef(PyObject* _self) {
  PyUpb_DescriptorBase* self =
      PyUpb_DescriptorBase_Check(_self, kPyUpb_MethodDescriptor);
  return self ? self->def : NULL;
}

PyObject* PyUpb_MethodDescriptor_Get(const upb_methoddef* m) {
  const upb_filedef* file = upb_servicedef_file(upb_methoddef_service(m));
  return PyUpb_DescriptorBase_Get(kPyUpb_MethodDescriptor, m, file);
}

static PyObject* PyUpb_MethodDescriptor_GetName(PyObject* self, void* closure) {
  const upb_methoddef* m = PyUpb_MethodDescriptor_GetDef(self);
  return PyUnicode_FromString(upb_methoddef_name(m));
}

static PyObject* PyUpb_MethodDescriptor_GetFullName(PyObject* self,
                                                    void* closure) {
  const upb_methoddef* m = PyUpb_MethodDescriptor_GetDef(self);
  return PyUnicode_FromString(upb_methoddef_fullname(m));
}

static PyObject* PyUpb_MethodDescriptor_GetContainingService(PyObject* self,
                                                             void* closure) {
  const upb_methoddef* m = PyUpb_MethodDescriptor_GetDef(self);
  return PyUpb_ServiceDescriptor_Get(upb_methoddef_service(m));
}

static PyObject* PyUpb_MethodDescriptor_GetInputType(PyObject* self,
                                                     void* closure) {
  const upb_methoddef* m = PyUpb_MethodDescriptor_GetDef(self);
  return PyUpb_Descriptor_Get(upb_methoddef_inputtype(m));
}

static PyObject* PyUpb_MethodDescriptor_GetOutputType(PyObject* self,
                                                      void* closure) {
  const upb_methoddef* m = PyUpb_MethodDescriptor_GetDef(self);
  return PyUpb_Descriptor_Get(upb_methoddef_outputtype(m));
}

static PyObject* PyUpb_MethodDescriptor_GetOptions(PyObject* _self,
                                                   PyObject* args) {
  PyUpb_DescriptorBase* self = (void*)_self;
  return PyUpb_DescriptorBase_GetOptions(self, upb_methoddef_options(self->def),
                                         &google_protobuf_MethodOptions_msginit,
                                         "google.protobuf.MethodOptions");
}

static PyObject* PyUpb_MethodDescriptor_CopyToProto(PyObject* _self,
                                                    PyObject* py_proto) {
  return PyUpb_DescriptorBase_CopyToProto(
      _self, (PyUpb_ToProto_Func*)&upb_MethodDef_ToProto,
      &google_protobuf_MethodDescriptorProto_msginit, py_proto);
}

static PyGetSetDef PyUpb_MethodDescriptor_Getters[] = {
    {"name", PyUpb_MethodDescriptor_GetName, NULL, "Name", NULL},
    {"full_name", PyUpb_MethodDescriptor_GetFullName, NULL, "Full name", NULL},
    // TODO(https://github.com/protocolbuffers/upb/issues/459)
    //{ "index", PyUpb_MethodDescriptor_GetIndex, NULL, "Index", NULL},
    {"containing_service", PyUpb_MethodDescriptor_GetContainingService, NULL,
     "Containing service", NULL},
    {"input_type", PyUpb_MethodDescriptor_GetInputType, NULL, "Input type",
     NULL},
    {"output_type", PyUpb_MethodDescriptor_GetOutputType, NULL, "Output type",
     NULL},
    {NULL}};

static PyMethodDef PyUpb_MethodDescriptor_Methods[] = {
    {"GetOptions", PyUpb_MethodDescriptor_GetOptions, METH_NOARGS},
    {"CopyToProto", PyUpb_MethodDescriptor_CopyToProto, METH_O},
    {NULL}};

static PyType_Slot PyUpb_MethodDescriptor_Slots[] = {
    DESCRIPTOR_BASE_SLOTS,
    {Py_tp_methods, PyUpb_MethodDescriptor_Methods},
    {Py_tp_getset, PyUpb_MethodDescriptor_Getters},
    {0, NULL}};

static PyType_Spec PyUpb_MethodDescriptor_Spec = {
    PYUPB_MODULE_NAME ".MethodDescriptor",  // tp_name
    sizeof(PyUpb_DescriptorBase),           // tp_basicsize
    0,                                      // tp_itemsize
    Py_TPFLAGS_DEFAULT,                     // tp_flags
    PyUpb_MethodDescriptor_Slots,
};

// -----------------------------------------------------------------------------
// OneofDescriptor
// -----------------------------------------------------------------------------

const upb_oneofdef* PyUpb_OneofDescriptor_GetDef(PyObject* _self) {
  PyUpb_DescriptorBase* self =
      PyUpb_DescriptorBase_Check(_self, kPyUpb_OneofDescriptor);
  return self ? self->def : NULL;
}

PyObject* PyUpb_OneofDescriptor_Get(const upb_oneofdef* oneof) {
  const upb_filedef* file = upb_msgdef_file(upb_oneofdef_containingtype(oneof));
  return PyUpb_DescriptorBase_Get(kPyUpb_OneofDescriptor, oneof, file);
}

static PyObject* PyUpb_OneofDescriptor_GetName(PyObject* self, void* closure) {
  const upb_oneofdef* oneof = PyUpb_OneofDescriptor_GetDef(self);
  return PyUnicode_FromString(upb_oneofdef_name(oneof));
}

static PyObject* PyUpb_OneofDescriptor_GetFullName(PyObject* self,
                                                   void* closure) {
  const upb_oneofdef* oneof = PyUpb_OneofDescriptor_GetDef(self);
  return PyUnicode_FromFormat(
      "%s.%s", upb_msgdef_fullname(upb_oneofdef_containingtype(oneof)),
      upb_oneofdef_name(oneof));
}

static PyObject* PyUpb_OneofDescriptor_GetIndex(PyObject* self, void* closure) {
  const upb_oneofdef* oneof = PyUpb_OneofDescriptor_GetDef(self);
  return PyLong_FromLong(upb_oneofdef_index(oneof));
}

static PyObject* PyUpb_OneofDescriptor_GetContainingType(PyObject* self,
                                                         void* closure) {
  const upb_oneofdef* oneof = PyUpb_OneofDescriptor_GetDef(self);
  return PyUpb_Descriptor_Get(upb_oneofdef_containingtype(oneof));
}

static PyObject* PyUpb_OneofDescriptor_GetHasOptions(PyObject* _self,
                                                     void* closure) {
  PyUpb_DescriptorBase* self = (void*)_self;
  return PyBool_FromLong(upb_oneofdef_hasoptions(self->def));
}

static PyObject* PyUpb_OneofDescriptor_GetOptions(PyObject* _self,
                                                  PyObject* args) {
  PyUpb_DescriptorBase* self = (void*)_self;
  return PyUpb_DescriptorBase_GetOptions(self, upb_oneofdef_options(self->def),
                                         &google_protobuf_OneofOptions_msginit,
                                         "google.protobuf.OneofOptions");
}

static PyGetSetDef PyUpb_OneofDescriptor_Getters[] = {
    {"name", PyUpb_OneofDescriptor_GetName, NULL, "Name"},
    {"full_name", PyUpb_OneofDescriptor_GetFullName, NULL, "Full name"},
    {"index", PyUpb_OneofDescriptor_GetIndex, NULL, "Index"},
    {"containing_type", PyUpb_OneofDescriptor_GetContainingType, NULL,
     "Containing type"},
    {"has_options", PyUpb_OneofDescriptor_GetHasOptions, NULL, "Has Options"},
    // TODO(https://github.com/protocolbuffers/upb/issues/459)
    //{ "fields", (getter)GetFields, NULL, "Fields"},
    {NULL}};

static PyMethodDef PyUpb_OneofDescriptor_Methods[] = {
    {"GetOptions", PyUpb_OneofDescriptor_GetOptions, METH_NOARGS}, {NULL}};

static PyType_Slot PyUpb_OneofDescriptor_Slots[] = {
    DESCRIPTOR_BASE_SLOTS,
    {Py_tp_methods, PyUpb_OneofDescriptor_Methods},
    {Py_tp_getset, PyUpb_OneofDescriptor_Getters},
    {0, NULL}};

static PyType_Spec PyUpb_OneofDescriptor_Spec = {
    PYUPB_MODULE_NAME ".OneofDescriptor",  // tp_name
    sizeof(PyUpb_DescriptorBase),          // tp_basicsize
    0,                                     // tp_itemsize
    Py_TPFLAGS_DEFAULT,                    // tp_flags
    PyUpb_OneofDescriptor_Slots,
};

// -----------------------------------------------------------------------------
// ServiceDescriptor
// -----------------------------------------------------------------------------

const upb_servicedef* PyUpb_ServiceDescriptor_GetDef(PyObject* _self) {
  PyUpb_DescriptorBase* self =
      PyUpb_DescriptorBase_Check(_self, kPyUpb_ServiceDescriptor);
  return self ? self->def : NULL;
}

PyObject* PyUpb_ServiceDescriptor_Get(const upb_servicedef* s) {
  const upb_filedef* file = upb_servicedef_file(s);
  return PyUpb_DescriptorBase_Get(kPyUpb_ServiceDescriptor, s, file);
}

static PyObject* PyUpb_ServiceDescriptor_GetFullName(PyObject* self,
                                                     void* closure) {
  const upb_servicedef* s = PyUpb_ServiceDescriptor_GetDef(self);
  return PyUnicode_FromString(upb_servicedef_fullname(s));
}

static PyObject* PyUpb_ServiceDescriptor_GetName(PyObject* self,
                                                 void* closure) {
  const upb_servicedef* s = PyUpb_ServiceDescriptor_GetDef(self);
  return PyUnicode_FromString(upb_servicedef_name(s));
}

static PyObject* PyUpb_ServiceDescriptor_GetFile(PyObject* self,
                                                 void* closure) {
  const upb_servicedef* s = PyUpb_ServiceDescriptor_GetDef(self);
  return PyUpb_FileDescriptor_Get(upb_servicedef_file(s));
}

static PyObject* PyUpb_ServiceDescriptor_GetIndex(PyObject* self,
                                                  void* closure) {
  const upb_servicedef* s = PyUpb_ServiceDescriptor_GetDef(self);
  return PyLong_FromLong(upb_servicedef_index(s));
}

static PyObject* PyUpb_ServiceDescriptor_GetMethods(PyObject* _self,
                                                    void* closure) {
  PyUpb_DescriptorBase* self = (void*)_self;
  static PyUpb_GenericSequence_Funcs funcs = {
      (void*)&upb_servicedef_methodcount,
      (void*)&upb_servicedef_method,
      (void*)&PyUpb_MethodDescriptor_Get,
  };
  return PyUpb_GenericSequence_New(&funcs, self->def, self->pool);
}

static PyObject* PyUpb_ServiceDescriptor_GetMethodsByName(PyObject* _self,
                                                          void* closure) {
  static PyUpb_ByNameMap_Funcs funcs = {
      {
          (void*)&upb_servicedef_methodcount,
          (void*)&upb_servicedef_method,
          (void*)&PyUpb_MethodDescriptor_Get,
      },
      (void*)&upb_servicedef_lookupmethod,
      (void*)&upb_methoddef_name,
  };
  PyUpb_DescriptorBase* self = (void*)_self;
  return PyUpb_ByNameMap_New(&funcs, self->def, self->pool);
}

static PyObject* PyUpb_ServiceDescriptor_GetOptions(PyObject* _self,
                                                    PyObject* args) {
  PyUpb_DescriptorBase* self = (void*)_self;
  return PyUpb_DescriptorBase_GetOptions(
      self, upb_servicedef_options(self->def),
      &google_protobuf_ServiceOptions_msginit,
      "google.protobuf.ServiceOptions");
}

static PyObject* PyUpb_ServiceDescriptor_CopyToProto(PyObject* _self,
                                                     PyObject* py_proto) {
  return PyUpb_DescriptorBase_CopyToProto(
      _self, (PyUpb_ToProto_Func*)&upb_ServiceDef_ToProto,
      &google_protobuf_ServiceDescriptorProto_msginit, py_proto);
}

static PyObject* PyUpb_ServiceDescriptor_FindMethodByName(PyObject* _self,
                                                          PyObject* py_name) {
  PyUpb_DescriptorBase* self = (void*)_self;
  const char* name = PyUnicode_AsUTF8AndSize(py_name, NULL);
  if (!name) return NULL;
  const upb_methoddef* method = upb_servicedef_lookupmethod(self->def, name);
  if (method == NULL) {
    return PyErr_Format(PyExc_KeyError, "Couldn't find method %.200s", name);
  }
  return PyUpb_MethodDescriptor_Get(method);
}

static PyGetSetDef PyUpb_ServiceDescriptor_Getters[] = {
    {"name", PyUpb_ServiceDescriptor_GetName, NULL, "Name", NULL},
    {"full_name", PyUpb_ServiceDescriptor_GetFullName, NULL, "Full name", NULL},
    {"file", PyUpb_ServiceDescriptor_GetFile, NULL, "File descriptor"},
    {"index", PyUpb_ServiceDescriptor_GetIndex, NULL, "Index", NULL},
    {"methods", PyUpb_ServiceDescriptor_GetMethods, NULL, "Methods", NULL},
    {"methods_by_name", PyUpb_ServiceDescriptor_GetMethodsByName, NULL,
     "Methods by name", NULL},
    {NULL}};

static PyMethodDef PyUpb_ServiceDescriptor_Methods[] = {
    {"GetOptions", PyUpb_ServiceDescriptor_GetOptions, METH_NOARGS},
    {"CopyToProto", PyUpb_ServiceDescriptor_CopyToProto, METH_O},
    {"FindMethodByName", PyUpb_ServiceDescriptor_FindMethodByName, METH_O},
    {NULL}};

static PyType_Slot PyUpb_ServiceDescriptor_Slots[] = {
    DESCRIPTOR_BASE_SLOTS,
    {Py_tp_methods, PyUpb_ServiceDescriptor_Methods},
    {Py_tp_getset, PyUpb_ServiceDescriptor_Getters},
    {0, NULL}};

static PyType_Spec PyUpb_ServiceDescriptor_Spec = {
    PYUPB_MODULE_NAME ".ServiceDescriptor",  // tp_name
    sizeof(PyUpb_DescriptorBase),            // tp_basicsize
    0,                                       // tp_itemsize
    Py_TPFLAGS_DEFAULT,                      // tp_flags
    PyUpb_ServiceDescriptor_Slots,
};

// -----------------------------------------------------------------------------
// Top Level
// -----------------------------------------------------------------------------

// These must be in the same order as PyUpb_DescriptorType in the header.
static PyType_Spec* desc_specs[] = {
    &PyUpb_Descriptor_Spec,          &PyUpb_EnumDescriptor_Spec,
    &PyUpb_EnumValueDescriptor_Spec, &PyUpb_FieldDescriptor_Spec,
    &PyUpb_FileDescriptor_Spec,      &PyUpb_MethodDescriptor_Spec,
    &PyUpb_OneofDescriptor_Spec,     &PyUpb_ServiceDescriptor_Spec,
};

bool PyUpb_InitDescriptor(PyObject* m) {
  PyUpb_ModuleState* s = PyUpb_ModuleState_GetFromModule(m);

  for (size_t i = 0; i < kPyUpb_Descriptor_Count; i++) {
    s->descriptor_types[i] = PyUpb_AddClass(m, desc_specs[i]);
    if (!s->descriptor_types[i]) {
      return false;
    }
  }

  return true;
}
