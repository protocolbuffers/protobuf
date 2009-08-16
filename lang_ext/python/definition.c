/*
 * upb - a minimalist implementation of protocol buffers.
 *
 * Copyright (c) 2009 Joshua Haberman.  See LICENSE for details.
 *
 * This file defines the Python module upb.definition.  This module
 * defines the following Python classes, which wrap upb's internal
 * definitions:
 *
 * - upb.definition.MessageDefintion
 * - upb.definition.EnumDefinition (TODO)
 * - upb.definition.ServiceDefinition (TODO)
 *
 * We also define a class upb.definition.Context, which provides the
 * mechanism for loading the above definitions from .proto files or
 * from binary descriptors.
 *
 * Once these definitions are loaded, we can use them to create
 * the Python types for each .proto message type.  That is covered
 * in other files.
 */

#include "definition.h"
#include "upb_context.h"

#if PY_MAJOR_VERSION > 3
const char *bytes_format = "y#";
#else
const char *bytes_format = "s#";
#endif


/* upb.def.MessageDefinition **************************************************/

#if 0
/* Not implemented yet, but these methods will expose information about the
 * message definition (the upb_msgdef). */
static PyMethodDef PyUpb_MessageDefinitionMethods[] = {
};

PyTypeObject PyUpb_MessageDefinitionType = {
  PyObject_HEAD_INIT(NULL)
  0,                                      /* ob_size */
  "upb.definition.MessageDefinition",     /* tp_name */
  sizeof(PyUpb_MessageDefinition),        /* tp_basicsize */
  0,                                      /* tp_itemsize */
  msgdef_dealloc,                         /* tp_dealloc */
  0,                                      /* tp_print */
  0,                                      /* tp_getattr */
  0,                                      /* tp_setattr */
  0,                                      /* tp_compare */
  0,                                      /* tp_repr */
  0,                                      /* tp_as_number */
  0,                                      /* tp_as_sequence */
  0,                                      /* tp_as_mapping */
  0,                                      /* tp_hash */
  0,                                      /* tp_call */
  0,                                      /* tp_str */
  0,                                      /* tp_getattro */
  0,                                      /* tp_setattro */
  0,                                      /* tp_as_buffer */
  Py_TPFLAGS_DEFAULT,                     /* tp_flags */
  0,                                      /* tp_doc */
  0,                                      /* tp_traverse */
  0,                                      /* tp_clear */
  0,                                      /* tp_richcompare */
  0,                                      /* tp_weaklistoffset */
  0,                                      /* tp_iter */
  0,                                      /* tp_iternext */
  msgdef_methods,                         /* tp_methods */
  0,                                      /* tp_members */
  0,                                      /* tp_getset */
  0,                                      /* tp_base */
  0,                                      /* tp_dict */
  0,                                      /* tp_descr_get */
  0,                                      /* tp_descr_set */
  0,                                      /* tp_dictoffset */
  0,                                      /* tp_init */
  0,                                      /* tp_alloc */
  0, /* Can't be created in Python. */    /* tp_new */
  0,                                      /* tp_free */
};
#endif

/* upb.Context ****************************************************************/

typedef struct {
  PyObject_HEAD
  struct upb_context *context;
} PyUpb_Context;

static PyTypeObject PyUpb_ContextType;  /* forward decl. */

#define CheckContext(obj) \
  (void*)obj; do { \
    if(!PyObject_TypeCheck(obj, &PyUpb_ContextType)) { \
      PyErr_SetString(PyExc_TypeError, "Must be a upb.Context"); \
      return NULL; \
    } \
  } while(0)

static PyObject *context_parsefds(PyObject *_context, PyObject *args)
{
  PyUpb_Context *context = CheckContext(_context);
  struct upb_string str;
  if(!PyArg_ParseTuple(args, bytes_format, &str.ptr, &str.byte_len))
    return NULL;
  str.byte_size = 0;  /* We don't own that mem. */

  if(!upb_context_parsefds(context->context, &str)) {
    /* TODO: an appropriate error. */
    PyErr_SetString(PyExc_TypeError, "Failed to parse."); \
    return NULL; \
  }
  Py_RETURN_NONE;
}

//static PyObject *context_lookup(PyObject *self, PyObject *args)
//{
//  PyUpb_Context *context = CheckContext(self);
//  struct upb_string str;
//  if(!PyArg_ParseTuple(args, "s#", &str.ptr, &str.byte_len))
//    return NULL;
//  str.byte_size = 0;  /* We don't own that mem. */
//
//  struct upb_symtab_entry e;
//  if(upb_context_lookup(context->context, &str, &e)) {
//    return get_or_create_def(&e);
//  } else {
//    Py_RETURN_NONE;
//  }
//}
//
//static PyObject *context_resolve(PyObject *self, PyObject *args)
//{
//  PyUpb_Context *context = CheckContext(self);
//  struct upb_string str;
//  if(!PyArg_ParseTuple(args, "s#", &str.ptr, &str.byte_len))
//    return NULL;
//  str.byte_size = 0;  /* We don't own that mem. */
//
//  struct upb_symtab_entry e;
//  if(upb_context_resolve(context->context, &str, &e)) {
//    return get_or_create_def(&e);
//  } else {
//    Py_RETURN_NONE;
//  }
//}

static void add_string(void *udata, struct upb_symtab_entry *entry)
{
  PyObject *list = udata;
  struct upb_string *s = &entry->e.key;
  /* TODO: check return. */
  PyObject *str = PyString_FromStringAndSize(s->ptr, s->byte_len);
  PyList_Append(list, str);
}

static PyObject *context_symbols(PyObject *self, PyObject *args)
{
  PyUpb_Context *context = CheckContext(self);
  PyObject *list = PyList_New(0);  /* TODO: check return. */
  upb_context_enumerate(context->context, add_string, list);
  return list;
}

static PyMethodDef context_methods[] = {
  {"parse_file_descriptor_set", context_parsefds, METH_VARARGS,
   "Parses a string containing a serialized FileDescriptorSet and adds its "
   "definitions to the context."
  },
  //{"lookup", context_lookup, METH_VARARGS,
  // "Finds a symbol by fully-qualified name (eg. foo.bar.MyType)."
  //},
  //{"resolve", context_resolve, METH_VARARGS,
  // "Finds a symbol by a possibly-relative name, which will be interpreted "
  // "in the context of the given base."
  //}
  {"symbols", context_symbols, METH_NOARGS,
   "Returns a list of symbol names that are defined in this context."
  },
  {NULL, NULL}
};

static PyObject *context_new(PyTypeObject *subtype,
                             PyObject *args, PyObject *kwds)
{
  PyUpb_Context *obj = (void*)subtype->tp_alloc(subtype, 0);
  obj->context = upb_context_new();
  return (void*)obj;
}

static void context_dealloc(PyObject *obj)
{
  PyUpb_Context *c = (void*)obj;
  upb_context_unref(c->context);
}

static PyTypeObject PyUpb_ContextType = {
  PyObject_HEAD_INIT(NULL)
  0,                                      /* ob_size */
  "upb.definition.Context",               /* tp_name */
  sizeof(PyUpb_Context),                  /* tp_basicsize */
  0,                                      /* tp_itemsize */
  (destructor)context_dealloc,            /* tp_dealloc */
  0,                                      /* tp_print */
  0,                                      /* tp_getattr */
  0,                                      /* tp_setattr */
  0,                                      /* tp_compare */
  0,                                      /* tp_repr */
  0,                                      /* tp_as_number */
  0,                                      /* tp_as_sequence */
  0,                                      /* tp_as_mapping */
  0,                                      /* tp_hash */
  0,                                      /* tp_call */
  0,                                      /* tp_str */
  0,                                      /* tp_getattro */
  0,                                      /* tp_setattro */
  0,                                      /* tp_as_buffer */
  Py_TPFLAGS_DEFAULT,                     /* tp_flags */
  0,                                      /* tp_doc */
  0,                                      /* tp_traverse */
  0,                                      /* tp_clear */
  0,                                      /* tp_richcompare */
  0,                                      /* tp_weaklistoffset */
  0,                                      /* tp_iter */
  0,                                      /* tp_iternext */
  context_methods,                        /* tp_methods */
  0,                                      /* tp_members */
  0,                                      /* tp_getset */
  0,                                      /* tp_base */
  0,                                      /* tp_dict */
  0,                                      /* tp_descr_get */
  0,                                      /* tp_descr_set */
  0,                                      /* tp_dictoffset */
  0,                                      /* tp_init */
  0,                                      /* tp_alloc */
  context_new,                            /* tp_new */
  0,                                      /* tp_free */
};

PyMethodDef methods[] = {
};

PyMODINIT_FUNC
initdefinition(void)
{
  if(PyType_Ready(&PyUpb_ContextType) < 0) return;
  Py_INCREF(&PyUpb_ContextType);  /* TODO: necessary? */

  PyObject *mod = Py_InitModule("upb.definition", methods);
  PyModule_AddObject(mod, "Context", (PyObject*)&PyUpb_ContextType);
}
