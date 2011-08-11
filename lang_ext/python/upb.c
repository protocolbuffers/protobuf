/*
 * upb - a minimalist implementation of protocol buffers.
 *
 * Copyright (c) 2009 Google Inc.  See LICENSE for details.
 * Author: Josh Haberman <jhaberman@gmail.com>
 *
 * Python extension exposing the core of upb: definitions, handlers,
 * and a message type.
 */

#include <stddef.h>
#include <Python.h>
#include "upb/def.h"

static bool streql(const char *a, const char *b) { return strcmp(a, b) == 0; }

PyObject *PyUpb_Error(const char *str) {
  PyErr_SetString(PyExc_TypeError, str);
  return NULL;
}

int PyUpb_ErrorInt(const char *str) {
  PyErr_SetString(PyExc_TypeError, str);
  return -1;
}


/* Object cache ***************************************************************/

// For objects that are just wrappers around a C object pointer, we keep a
// cache mapping C pointer -> wrapper object.  This allows us to consistently
// vend the same Python object given the same C object.  This prevents us from
// creating too many Python objects unnecessarily.  More importantly, it provides
// the expected semantics:
//
//   if field.subdef is field.subdef:
//     print "Sanity prevails."
//
// If we conjured up a new wrapper object every time, the above would not be
// true.
//
// The cost is having to put all such objects in a table, but since this only
// applies to schema-level objects (defs, handlers, etc) this seems acceptable.
// We do *not* have to put all message objects in this table.
//
// We use weak refs so that the cache does not prevent the wrapper objects from
// being collected.  The table is stored as a static variable; to use
// sub-interpreters this would need to change, but I believe that using
// sub-interpreters is exceedingly rare in practice.

typedef struct {
  PyObject_HEAD;
  void *obj;
  PyObject *weakreflist;
} PyUpb_ObjWrapper;

static PyObject *obj_cache = NULL;
static PyObject *reverse_cache = NULL;
static PyObject *weakref_callback = NULL;

// Utility functions for manipulating Python dictionaries keyed by pointer.

static PyObject *PyUpb_StringForPointer(void *ptr) {
  PyObject *o = PyString_FromStringAndSize((const char *)&ptr, sizeof(void*));
  assert(o);
  return o;
}

static PyObject *PyUpb_ObjCacheDeleteCallback(PyObject *self, PyObject *ref) {
  // Python very unfortunately clears the weakref before running our callback.
  // This prevents us from using the weakref to find the C pointer we need to
  // remove from the cache.  As a result we are forced to keep a second map
  // mapping weakref->C pointer.
  PyObject *ptr_str = PyDict_GetItem(reverse_cache, ref);
  assert(ptr_str);
  int err = PyDict_DelItem(obj_cache, ptr_str);
  assert(!err);
  err = PyDict_DelItem(reverse_cache, ref);
  assert(!err);
  return Py_None;
}

static PyObject *PyUpb_ObjCacheGet(void *obj, PyTypeObject *type) {
  PyObject *kv = PyUpb_StringForPointer(obj);
  PyObject *ref = PyDict_GetItem(obj_cache, kv);
  PyObject *ret;
  if (ref) {
    ret = PyWeakref_GetObject(ref);
    assert(ret != Py_None);
    Py_INCREF(ret);
  } else {
    PyUpb_ObjWrapper *wrapper = (PyUpb_ObjWrapper*)type->tp_alloc(type, 0);
    wrapper->obj = obj;
    wrapper->weakreflist = NULL;
    ret = (PyObject*)wrapper;
    ref = PyWeakref_NewRef(ret, weakref_callback);
    assert(PyWeakref_GetObject(ref) == ret);
    assert(ref);
    PyDict_SetItem(obj_cache, kv, ref);
    PyDict_SetItem(reverse_cache, ref, kv);
  }
  assert(ret);
  Py_DECREF(kv);
  return ret;
}


/* PyUpb_Def ******************************************************************/

static void PyUpb_Def_dealloc(PyObject *obj) {
  PyUpb_ObjWrapper *wrapper = (void*)obj;
  upb_def_unref((upb_def*)wrapper->obj);
  obj->ob_type->tp_free(obj);
}


/* PyUpb_FieldDef *************************************************************/

static PyTypeObject PyUpb_FieldDefType;
static int PyUpb_FieldDef_setattro(PyObject *o, PyObject *key, PyObject *val);

#define Check_FieldDef(o, badret) \
  (void*)(((PyUpb_ObjWrapper*)o)->obj); do { \
    if(!PyObject_TypeCheck(o, &PyUpb_FieldDefType)) { \
      PyErr_SetString(PyExc_TypeError, "must be a upb.FieldDef"); \
      return badret; \
    } \
  } while(0)

static PyObject *PyUpb_FieldDef_GetOrCreate(upb_fielddef *f) {
  return PyUpb_ObjCacheGet(f, &PyUpb_FieldDefType);
}

static PyObject *PyUpb_FieldDef_new(PyTypeObject *subtype,
                                    PyObject *args, PyObject *kwds) {
  return PyUpb_ObjCacheGet(upb_fielddef_new(), subtype);
}

static int PyUpb_FieldDef_init(PyObject *self, PyObject *args, PyObject *kwds) {
  if (!kwds) return 0;
  PyObject *key, *value;
  Py_ssize_t pos = 0;
  while (PyDict_Next(kwds, &pos, &key, &value))
    PyUpb_FieldDef_setattro(self, key, value);
  return 0;
}

static void PyUpb_FieldDef_dealloc(PyObject *obj) {
  PyUpb_ObjWrapper *wrapper = (void*)obj;
  if (wrapper->weakreflist) PyObject_ClearWeakRefs(obj);
  upb_fielddef_unref((upb_fielddef*)wrapper->obj);
  obj->ob_type->tp_free(obj);
}

static PyObject *PyUpb_FieldDef_getattro(PyObject *obj, PyObject *attr_name) {
  upb_fielddef *f = Check_FieldDef(obj, NULL);
  if (!upb_fielddef_ismutable(f)) {
    PyErr_SetString(PyExc_TypeError, "fielddef is not mutable.");
    return NULL;
  }
  const char *name = PyString_AsString(attr_name);
  if (streql(name, "name")) {
    const char *name = upb_fielddef_name(f);
    return name == NULL ? Py_None : PyString_FromString(name);
  } else if (streql(name, "number")) {
    uint32_t num = upb_fielddef_number(f);
    return num == 0 ? Py_None : PyInt_FromLong(num);
  } else if (streql(name, "type")) {
    uint8_t type = upb_fielddef_type(f);
    return type == 0 ? Py_None : PyInt_FromLong(type);
  } else if (streql(name, "label")) {
    return PyInt_FromLong(upb_fielddef_label(f));
  } else if (streql(name, "type_name")) {
    const char *name = upb_fielddef_typename(f);
    return name == NULL ? Py_None : PyString_FromString(name);
  } else if (streql(name, "subdef")) {
    // NYI;
    return NULL;
  } else if (streql(name, "msgdef")) {
    // NYI;
    return NULL;
  } else {
    return PyUpb_Error("Invalid fielddef member.");
  }
}

static int PyUpb_FieldDef_setattro(PyObject *o, PyObject *key, PyObject *val) {
  upb_fielddef *f = Check_FieldDef(o, -1);
  const char *field = PyString_AsString(key);
  if (!upb_fielddef_ismutable(f))
    return PyUpb_ErrorInt("fielddef is not mutable.");
  if (streql(field, "name")) {
    const char *name = PyString_AsString(val);
    if (!name || !upb_fielddef_setname(f, name))
      return PyUpb_ErrorInt("Invalid name");
  } else if (streql(field, "number")) {
    // TODO: should check truncation.  Non-security issue.
    // Non-int will return -1, which is already invalid as a field number.
    if (!upb_fielddef_setnumber(f, PyInt_AsLong(val)))
      return PyUpb_ErrorInt("Invalid number");
  } else if (streql(field, "type")) {
    // TODO: should check truncation.  Non-security issue.
    if (!upb_fielddef_settype(f, PyInt_AsLong(val)))
      return PyUpb_ErrorInt("Invalid type");
  } else if (streql(field, "label")) {
    // TODO: should check truncation.  Non-security issue.
    if (!upb_fielddef_setlabel(f, PyInt_AsLong(val)))
      return PyUpb_ErrorInt("Invalid label");
  } else if (streql(field, "type_name")) {
    const char *name = PyString_AsString(val);
    if (!name || !upb_fielddef_settypename(f, name))
      return PyUpb_ErrorInt("Invalid type_name");
  } else if (streql(field, "default_value")) {
    // NYI
    return -1;
  } else {
    return PyUpb_ErrorInt("Invalid fielddef member.");
  }
  return 0;
}

static PyTypeObject PyUpb_FieldDefType = {
  PyObject_HEAD_INIT(NULL)
  0,                                      /* ob_size */
  "upb.FieldDef",                         /* tp_name */
  sizeof(PyUpb_ObjWrapper),               /* tp_basicsize */
  0,                                      /* tp_itemsize */
  &PyUpb_FieldDef_dealloc,                /* tp_dealloc */
  0,                                      /* tp_print */
  0,                                      /* tp_getattr */
  0,                                      /* tp_setattr */
  0,                                      /* tp_compare */
  0, /* TODO */                           /* tp_repr */
  0,                                      /* tp_as_number */
  0,                                      /* tp_as_sequence */
  0,                                      /* tp_as_mapping */
  0,                                      /* tp_hash */
  0,                                      /* tp_call */
  0,                                      /* tp_str */
  &PyUpb_FieldDef_getattro,               /* tp_getattro */
  &PyUpb_FieldDef_setattro,               /* tp_setattro */
  0,                                      /* tp_as_buffer */
  Py_TPFLAGS_DEFAULT,                     /* tp_flags */
  0,                                      /* tp_doc */
  0,                                      /* tp_traverse */
  0,                                      /* tp_clear */
  0,                                      /* tp_richcompare */
  offsetof(PyUpb_ObjWrapper, weakreflist),/* tp_weaklistoffset */
  0,                                      /* tp_iter */
  0,                                      /* tp_iternext */
  0,                                      /* tp_methods */
  0,                                      /* tp_members */
  0,                                      /* tp_getset */
  0,                                      /* tp_base */
  0,                                      /* tp_dict */
  0,                                      /* tp_descr_get */
  0,                                      /* tp_descr_set */
  0,                                      /* tp_dictoffset */
  &PyUpb_FieldDef_init,                   /* tp_init */
  0,                                      /* tp_alloc */
  &PyUpb_FieldDef_new,                    /* tp_new */
  0,                                      /* tp_free */
};


/* PyUpb_MessageDef ***********************************************************/

static PyTypeObject PyUpb_MessageDefType;
static int PyUpb_MessageDef_setattro(PyObject *o, PyObject *key, PyObject *val);

#define Check_MessageDef(o, badret) \
  (void*)(((PyUpb_ObjWrapper*)o)->obj); do { \
    if(!PyObject_TypeCheck(o, &PyUpb_MessageDefType)) { \
      PyErr_SetString(PyExc_TypeError, "must be a upb.MessageDef"); \
      return badret; \
    } \
  } while(0)

static PyObject *PyUpb_MessageDef_new(PyTypeObject *subtype,
                                      PyObject *args, PyObject *kwds) {
  return PyUpb_ObjCacheGet(upb_msgdef_new(), subtype);
}

static PyObject *PyUpb_MessageDef_addfields(PyObject *o, PyObject *args);

static int PyUpb_MessageDef_init(PyObject *self, PyObject *args, PyObject *kwds) {
  if (!kwds) return 0;
  PyObject *key, *value;
  Py_ssize_t pos = 0;
  while (PyDict_Next(kwds, &pos, &key, &value)) {
    const char *field = PyString_AsString(key);
    if (streql(field, "fields")) {
      PyUpb_MessageDef_addfields(self, value);
    } else {
      PyUpb_MessageDef_setattro(self, key, value);
    }
  }
  return 0;
}

static PyObject *PyUpb_MessageDef_getattro(PyObject *obj, PyObject *attr_name) {
  upb_msgdef *m = Check_MessageDef(obj, NULL);
  const char *name = PyString_AsString(attr_name);
  if (streql(name, "fqname")) {
    const char *fqname = upb_def_fqname(UPB_UPCAST(m));
    return fqname == NULL ? Py_None : PyString_FromString(fqname);
  }
  return PyObject_GenericGetAttr(obj, attr_name);
}

static int PyUpb_MessageDef_setattro(PyObject *o, PyObject *key, PyObject *val) {
  upb_msgdef *m = Check_MessageDef(o, -1);
  if (!upb_def_ismutable(UPB_UPCAST(m))) {
    PyErr_SetString(PyExc_TypeError, "MessageDef is not mutable.");
    return -1;
  }
  const char *name = PyString_AsString(key);
  if (streql(name, "fqname")) {
    const char *fqname = PyString_AsString(val);
    if (!fqname || !upb_def_setfqname(UPB_UPCAST(m), fqname))
      return PyUpb_ErrorInt("Invalid fqname");
  } else {
    return PyUpb_ErrorInt("Invalid MessageDef member.");
  }
  return 0;
}

static PyObject *PyUpb_MessageDef_fields(PyObject *obj, PyObject *args) {
  upb_msgdef *m = Check_MessageDef(obj, NULL);
  PyObject *ret = PyList_New(0);
  upb_msg_iter i;
  for(i = upb_msg_begin(m); !upb_msg_done(i); i = upb_msg_next(m, i)) {
    upb_fielddef *f = upb_msg_iter_field(i);
    PyList_Append(ret, PyUpb_FieldDef_GetOrCreate(f));
  }
  return ret;
}

static PyObject *PyUpb_MessageDef_addfields(PyObject *o, PyObject *fields) {
  upb_msgdef *m = Check_MessageDef(o, NULL);
  Py_ssize_t len = PySequence_Length(fields);
  if (len > UPB_MAX_FIELDS) return PyUpb_Error("Too many fields.");
  upb_fielddef *f[len];
  int i;
  for (i = 0; i < len; i++) {
    PyObject *field = PySequence_GetItem(fields, i);
    f[i] = Check_FieldDef(field, NULL);
  }
  upb_msgdef_addfields(m, f, len);
  return Py_None;
}

static PyMethodDef PyUpb_MessageDef_methods[] = {
  {"fields", &PyUpb_MessageDef_fields, METH_NOARGS, "Returns list of fields."},
  {NULL, NULL}
};

static PyTypeObject PyUpb_MessageDefType = {
  PyObject_HEAD_INIT(NULL)
  0,                                      /* ob_size */
  "upb.MessageDef",                       /* tp_name */
  sizeof(PyUpb_ObjWrapper),               /* tp_basicsize */
  0,                                      /* tp_itemsize */
  &PyUpb_Def_dealloc,                     /* tp_dealloc */
  0,                                      /* tp_print */
  0,                                      /* tp_getattr */
  0,                                      /* tp_setattr */
  0,                                      /* tp_compare */
  0, /* TODO */                           /* tp_repr */
  0,                                      /* tp_as_number */
  0,                                      /* tp_as_sequence */
  0,                                      /* tp_as_mapping */
  0,                                      /* tp_hash */
  0,                                      /* tp_call */
  0,                                      /* tp_str */
  &PyUpb_MessageDef_getattro,             /* tp_getattro */
  &PyUpb_MessageDef_setattro,             /* tp_setattro */
  0,                                      /* tp_as_buffer */
  Py_TPFLAGS_DEFAULT,                     /* tp_flags */
  0,                                      /* tp_doc */
  0,                                      /* tp_traverse */
  0,                                      /* tp_clear */
  0,                                      /* tp_richcompare */
  offsetof(PyUpb_ObjWrapper, weakreflist),/* tp_weaklistoffset */
  0,                                      /* tp_iter */
  0,                                      /* tp_iternext */
  PyUpb_MessageDef_methods,               /* tp_methods */
  0,                                      /* tp_members */
  0,                                      /* tp_getset */
  0,                                      /* tp_base */
  0,                                      /* tp_dict */
  0,                                      /* tp_descr_get */
  0,                                      /* tp_descr_set */
  0,                                      /* tp_dictoffset */
  &PyUpb_MessageDef_init,                 /* tp_init */
  0,                                      /* tp_alloc */
  &PyUpb_MessageDef_new,                  /* tp_new */
  0,                                      /* tp_free */
};


/* Toplevel *******************************************************************/

static PyMethodDef methods[] = {
  {NULL, NULL}
};

// PyModule_AddObject steals a ref, but our object is statically allocated
// and must not be deleted.
#define PyUpb_AddType(mod, name, type) \
  if (PyType_Ready(type) < 0) return; \
  Py_INCREF(type); \
  PyModule_AddObject(mod, name, (PyObject*)type);

PyMODINIT_FUNC initupb(void) {
  PyObject *mod = Py_InitModule("upb", methods);

  PyUpb_AddType(mod, "FieldDef", &PyUpb_FieldDefType);
  PyUpb_AddType(mod, "MessageDef", &PyUpb_MessageDefType);

  PyModule_AddIntConstant(mod, "LABEL_OPTIONAL", UPB_LABEL(OPTIONAL));
  PyModule_AddIntConstant(mod, "LABEL_REQUIRED", UPB_LABEL(REQUIRED));
  PyModule_AddIntConstant(mod, "LABEL_REPEATED", UPB_LABEL(REPEATED));

  PyModule_AddIntConstant(mod, "TYPE_DOUBLE", UPB_TYPE(DOUBLE));
  PyModule_AddIntConstant(mod, "TYPE_FLOAT", UPB_TYPE(FLOAT));
  PyModule_AddIntConstant(mod, "TYPE_INT64", UPB_TYPE(INT64));
  PyModule_AddIntConstant(mod, "TYPE_UINT64", UPB_TYPE(UINT64));
  PyModule_AddIntConstant(mod, "TYPE_INT32", UPB_TYPE(INT32));
  PyModule_AddIntConstant(mod, "TYPE_FIXED64", UPB_TYPE(FIXED64));
  PyModule_AddIntConstant(mod, "TYPE_FIXED32", UPB_TYPE(FIXED32));
  PyModule_AddIntConstant(mod, "TYPE_BOOL", UPB_TYPE(BOOL));
  PyModule_AddIntConstant(mod, "TYPE_STRING", UPB_TYPE(STRING));
  PyModule_AddIntConstant(mod, "TYPE_GROUP", UPB_TYPE(GROUP));
  PyModule_AddIntConstant(mod, "TYPE_MESSAGE", UPB_TYPE(MESSAGE));
  PyModule_AddIntConstant(mod, "TYPE_BYTES", UPB_TYPE(BYTES));
  PyModule_AddIntConstant(mod, "TYPE_UINT32", UPB_TYPE(UINT32));
  PyModule_AddIntConstant(mod, "TYPE_ENUM", UPB_TYPE(ENUM));
  PyModule_AddIntConstant(mod, "TYPE_SFIXED32", UPB_TYPE(SFIXED32));
  PyModule_AddIntConstant(mod, "TYPE_SFIXED64", UPB_TYPE(SFIXED64));
  PyModule_AddIntConstant(mod, "TYPE_SINT32", UPB_TYPE(SINT32));
  PyModule_AddIntConstant(mod, "TYPE_SINT64", UPB_TYPE(SINT64));

  obj_cache = PyDict_New();
  reverse_cache = PyDict_New();
  static PyMethodDef method = {
        "WeakRefCallback", &PyUpb_ObjCacheDeleteCallback, METH_O, NULL};
  PyObject *pyname = PyString_FromString(method.ml_name);
  weakref_callback = PyCFunction_NewEx(&method, NULL, pyname);
  Py_DECREF(pyname);
}
