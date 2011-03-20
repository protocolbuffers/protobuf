/*
 * upb - a minimalist implementation of protocol buffers.
 *
 * Copyright (c) 2009 Google Inc.  See LICENSE for details.
 * Author: Josh Haberman <jhaberman@gmail.com>
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
#include "upb_msg.h"

static PyTypeObject PyUpb_ContextType;
static struct upb_strtable msgdefs;
static struct upb_strtable contexts;

struct msgtab_entry {
  struct upb_strtable_entry e;
  PyUpb_MsgDef *msgdef;
};

struct contexttab_entry {
  struct upb_strtable_entry e;
  PyUpb_Context *context;
};

#define CheckContext(obj) \
  (void*)obj; do { \
    if(!PyObject_TypeCheck(obj, &PyUpb_ContextType)) { \
      PyErr_SetString(PyExc_TypeError, "Must be a upb.Context"); \
      return NULL; \
    } \
  } while(0)

/* upb.def.MessageDefinition **************************************************/

/* Not implemented yet, but these methods will expose information about the
 * message definition (the upb_msgdef). */
static PyMethodDef msgdef_methods[] = {
  {NULL, NULL}
};

static void msgdef_dealloc(PyObject *obj)
{
  PyUpb_MsgDef *md_obj = (void*)obj;
  Py_DECREF(md_obj->context);
  obj->ob_type->tp_free(obj);
}

PyTypeObject PyUpb_MsgDefType = {
  PyObject_HEAD_INIT(NULL)
  0,                                      /* ob_size */
  "upb.definition.MessageDefinition",     /* tp_name */
  sizeof(PyUpb_MsgDef),                   /* tp_basicsize */
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

/* upb.Context ****************************************************************/

static PyObject *context_parsefds(PyObject *_context, PyObject *args)
{
  PyUpb_Context *context = CheckContext(_context);
  struct upb_string str;
  if(!PyArg_ParseTuple(args, BYTES_FORMAT, &str.ptr, &str.byte_len))
    return NULL;
  str.byte_size = 0;  /* We don't own that mem. */

  if(!upb_context_parsefds(context->context, &str)) {
    /* TODO: an appropriate error. */
    PyErr_SetString(PyExc_TypeError, "Failed to parse."); \
    return NULL; \
  }
  Py_RETURN_NONE;
}

static PyObject *get_or_create_def(struct upb_symtab_entry *e)
{
  switch(e->type) {
    case UPB_SYM_MESSAGE: return (PyObject*)get_or_create_msgdef(e->ref.msg);
    case UPB_SYM_ENUM:
    case UPB_SYM_SERVICE:
    case UPB_SYM_EXTENSION:
    default: fprintf(stderr, "upb.pb, not implemented.\n"); abort(); return NULL;
  }
}

static PyUpb_Context *get_or_create_context(struct upb_context *context)
{
  PyUpb_Context *pycontext = NULL;
  struct upb_string str = {.ptr = (char*)&context, .byte_len = sizeof(void*)};
  struct contexttab_entry *e = upb_strtable_lookup(&contexts, &str);
  if(!e) {
    pycontext = (void*)PyUpb_ContextType.tp_alloc(&PyUpb_ContextType, 0);
    pycontext->context = context;
    struct contexttab_entry new_e = {
      .e = {.key = {.ptr = (char*)&pycontext->context, .byte_len = sizeof(void*)}},
      .context = pycontext
    };
    upb_strtable_insert(&contexts, &new_e.e);
  } else {
    pycontext = e->context;
    Py_INCREF(pycontext);
  }
  return pycontext;
}

PyUpb_MsgDef *get_or_create_msgdef(struct upb_msgdef *def)
{
  PyUpb_MsgDef *pydef = NULL;
  struct upb_string str = {.ptr = (char*)&def, .byte_len = sizeof(void*)};
  struct msgtab_entry *e = upb_strtable_lookup(&msgdefs, &str);
  if(!e) {
    pydef = (void*)PyUpb_MsgDefType.tp_alloc(&PyUpb_MsgDefType, 0);
    pydef->def = def;
    pydef->context = get_or_create_context(def->context);
    struct msgtab_entry new_e = {
      .e = {.key = {.ptr = (char*)&pydef->def, .byte_len = sizeof(void*)}},
      .msgdef = pydef
    };
    upb_strtable_insert(&msgdefs, &new_e.e);
  } else {
    pydef = e->msgdef;
    Py_INCREF(pydef);
  }
  return pydef;
}

static PyObject *context_lookup(PyObject *self, PyObject *args)
{
  PyUpb_Context *context = CheckContext(self);
  struct upb_string str;
  if(!PyArg_ParseTuple(args, "s#", &str.ptr, &str.byte_len))
    return NULL;
  str.byte_size = 0;  /* We don't own that mem. */

  struct upb_symtab_entry e;
  if(upb_context_lookup(context->context, &str, &e)) {
    return get_or_create_def(&e);
  } else {
    Py_RETURN_NONE;
  }
}

static PyObject *context_resolve(PyObject *self, PyObject *args)
{
  PyUpb_Context *context = CheckContext(self);
  struct upb_string str;
  struct upb_string base;
  if(!PyArg_ParseTuple(args, "s#s#", &base.ptr, &base.byte_len,
                                     &str.ptr, &str.byte_len))
    return NULL;
  str.byte_size = 0;  /* We don't own that mem. */

  struct upb_symtab_entry e;
  if(upb_context_resolve(context->context, &base, &str, &e)) {
    return get_or_create_def(&e);
  } else {
    Py_RETURN_NONE;
  }
}

/* Callback for upb_context_enumerate below. */
static void add_string(void *udata, struct upb_symtab_entry *entry)
{
  PyObject *list = udata;
  struct upb_string *s = &entry->e.key;
  /* TODO: check return. */
  PyObject *str = PyString_FromStringAndSize(s->ptr, s->byte_len);
  PyList_Append(list, str);
  Py_DECREF(str);
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
  {"lookup", context_lookup, METH_VARARGS,
   "Finds a symbol by fully-qualified name (eg. foo.bar.MyType)."
  },
  {"resolve", context_resolve, METH_VARARGS,
   "Finds a symbol by a possibly-relative name, which will be interpreted "
   "in the context of the given base."
  },
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
  struct contexttab_entry e = {
    .e = {.key = {.ptr = (char*)&obj->context, .byte_len = sizeof(void*)}},
    .context = obj
  };
  upb_strtable_insert(&contexts, &e.e);
  return (void*)obj;
}

static void context_dealloc(PyObject *obj)
{
  PyUpb_Context *c = (void*)obj;
  upb_context_unref(c->context);
  /* TODO: once strtable supports delete. */
  //struct upb_string ptrstr = {.ptr = (char*)&c->context, .byte_len = sizeof(void*)};
  //upb_strtable_delete(&contexts, &ptrstr);
  obj->ob_type->tp_free(obj);
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

static PyMethodDef methods[] = {
  {NULL, NULL}
};

PyMODINIT_FUNC
initdefinition(void)
{
  if(PyType_Ready(&PyUpb_ContextType) < 0) return;
  if(PyType_Ready(&PyUpb_MsgDefType) < 0) return;

  /* PyModule_AddObject steals a reference.  These objects are statically
   * allocated and must not be deleted, so we increment their refcount. */
  Py_INCREF(&PyUpb_ContextType);
  Py_INCREF(&PyUpb_MsgDefType);

  PyObject *mod = Py_InitModule("upb.cext.definition", methods);
  PyModule_AddObject(mod, "Context", (PyObject*)&PyUpb_ContextType);
  PyModule_AddObject(mod, "MessageDefinition", (PyObject*)&PyUpb_MsgDefType);

  upb_strtable_init(&contexts, 8, sizeof(struct contexttab_entry));
  upb_strtable_init(&msgdefs, 16, sizeof(struct msgtab_entry));
}
