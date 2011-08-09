/*
 * upb - a minimalist implementation of protocol buffers.
 *
 * Copyright (c) 2009 Google Inc.  See LICENSE for details.
 * Author: Josh Haberman <jhaberman@gmail.com>
 *
 * Python extension exposing the core of upb: definitions, handlers,
 * and a message type.
 */

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

/* PyUpb_Def ******************************************************************/

// All the def types share the same C layout, even though they are different
// Python types.  For the moment we don't bother trying to make them an actual
// inheritance hierarchy.

typedef struct {
  PyObject_HEAD;
  upb_def *def;
} PyUpb_Def;


/* PyUpb_FieldDef *************************************************************/

typedef struct {
  PyObject_HEAD;
  upb_fielddef *field;
} PyUpb_FieldDef;

static PyTypeObject PyUpb_FieldDefType;

#define Check_FieldDef(obj, badret) \
  (void*)obj; do { \
    if(!PyObject_TypeCheck(obj, &PyUpb_FieldDefType)) { \
      PyErr_SetString(PyExc_TypeError, "must be a upb.FieldDef"); \
      return badret; \
    } \
  } while(0)

static void PyUpb_FieldDef_dealloc(PyObject *obj) {
  PyUpb_FieldDef *f = (void*)obj;
  upb_fielddef_unref(f->field);
  obj->ob_type->tp_free(obj);
}

static PyObject* PyUpb_FieldDef_getattro(PyObject *obj, PyObject *attr_name) {
  PyUpb_FieldDef *f = Check_FieldDef(obj, NULL);
  if (!upb_fielddef_ismutable(f->field)) {
    PyErr_SetString(PyExc_TypeError, "fielddef is not mutable.");
    return NULL;
  }
  const char *name = PyString_AsString(attr_name);
  if (streql(name, "name")) {
    const char *name = upb_fielddef_name(f->field);
    return name == NULL ? Py_None : PyString_FromString(name);
  } else if (streql(name, "number")) {
    uint32_t num = upb_fielddef_number(f->field);
    return num == 0 ? Py_None : PyInt_FromLong(num);
  } else if (streql(name, "type")) {
    uint8_t type = upb_fielddef_type(f->field);
    return type == 0 ? Py_None : PyInt_FromLong(type);
  } else if (streql(name, "label")) {
    return PyInt_FromLong(upb_fielddef_label(f->field));
  } else if (streql(name, "type_name")) {
    const char *name = upb_fielddef_typename(f->field);
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
  PyUpb_FieldDef *f = Check_FieldDef(o, -1);
  const char *field = PyString_AsString(key);
  if (!upb_fielddef_ismutable(f->field))
    return PyUpb_ErrorInt("fielddef is not mutable.");
  if (streql(field, "name")) {
    const char *name = PyString_AsString(val);
    if (!name || !upb_fielddef_setname(f->field, name))
      return PyUpb_ErrorInt("Invalid name");
  } else if (streql(field, "number")) {
    // TODO: should check truncation.  Non-security issue.
    // Non-int will return -1, which is already invalid as a field number.
    if (!upb_fielddef_setnumber(f->field, PyInt_AsLong(val)))
      return PyUpb_ErrorInt("Invalid number");
  } else if (streql(field, "type")) {
    // TODO: should check truncation.  Non-security issue.
    if (!upb_fielddef_settype(f->field, PyInt_AsLong(val)))
      return PyUpb_ErrorInt("Invalid type");
  } else if (streql(field, "label")) {
    // TODO: should check truncation.  Non-security issue.
    if (!upb_fielddef_setlabel(f->field, PyInt_AsLong(val)))
      return PyUpb_ErrorInt("Invalid label");
  } else if (streql(field, "type_name")) {
    const char *name = PyString_AsString(val);
    if (!name || !upb_fielddef_settypename(f->field, name))
      return PyUpb_ErrorInt("Invalid type_name");
  } else if (streql(field, "default_value")) {
    // NYI
    return -1;
  } else {
    return PyUpb_ErrorInt("Invalid fielddef member.");
  }
  return 0;
}

static int PyUpb_FieldDef_init(PyObject *self, PyObject *args, PyObject *kwds) {
  if (!kwds) return 0;
  PyObject *key, *value;
  Py_ssize_t pos = 0;
  while (PyDict_Next(kwds, &pos, &key, &value))
    PyUpb_FieldDef_setattro(self, key, value);
  return 0;
}

static PyObject *PyUpb_FieldDef_new(PyTypeObject *subtype,
                                    PyObject *args, PyObject *kwds) {
  PyUpb_FieldDef *f = (PyUpb_FieldDef*)subtype->tp_alloc(subtype, 0);
  f->field = upb_fielddef_new();
  return (PyObject*)f;
}

static PyTypeObject PyUpb_FieldDefType = {
  PyObject_HEAD_INIT(NULL)
  0,                                      /* ob_size */
  "upb.FieldDef",                         /* tp_name */
  sizeof(PyUpb_FieldDef),                 /* tp_basicsize */
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
  0,                                      /* tp_weaklistoffset */
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

static PyMethodDef methods[] = {
  {NULL, NULL}
};

PyMODINIT_FUNC initupb(void) {
  PyObject *mod = Py_InitModule("upb", methods);
  if (PyType_Ready(&PyUpb_FieldDefType) < 0) return;

  // PyModule_AddObject steals a ref, but our object is statically allocated
  // and must not be deleted.
  Py_INCREF(&PyUpb_FieldDefType);

  PyModule_AddObject(mod, "FieldDef", (PyObject*)&PyUpb_FieldDefType);

  // Register constants.
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
}
