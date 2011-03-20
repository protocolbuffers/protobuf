/*
 * upb - a minimalist implementation of protocol buffers.
 *
 * Copyright (c) 2009 Joshua Haberman.  See LICENSE for details.
 * Author: Josh Haberman <jhaberman@gmail.com>
 */

#ifndef UPB_PYTHON_CEXT_H_
#define UPB_PYTHON_CEXT_H_

#include <Python.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  PyObject_HEAD
  struct upb_context *context;
  PyObject *created_defs;
} PyUpb_Context;

typedef struct {
  PyObject_HEAD
  struct upb_msgdef *def;
  PyUpb_Context *context;
} PyUpb_MsgDef;

extern PyTypeObject PyUpb_MsgDefType;

/* What format string should be passed to PyArg_ParseTuple to get just a raw
 * string of bytes and a length. */
#if PY_MAJOR_VERSION >= 3
#define BYTES_FORMAT "y#"
#else
#define BYTES_FORMAT "s#"
#endif

#define RETURN_BOOL(val) if(val) { Py_RETURN_TRUE; } else { Py_RETURN_FALSE; }

extern PyMODINIT_FUNC initdefinition(void);
extern PyMODINIT_FUNC initpb(void);

#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif
