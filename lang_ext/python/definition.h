/*
 * upb - a minimalist implementation of protocol buffers.
 *
 * Copyright (c) 2009 Google Inc.  See LICENSE for details.
 * Author: Josh Haberman <jhaberman@gmail.com>
 *
 * See def.h for a general description.  These definitions
 * must be shared so that specific Python message types (for the
 * different proto APIs) can have access to the C definitions. */

#ifndef UPB_PYTHON_DEFINITION_H_
#define UPB_PYTHON_DEFINITION_H_

#include <Python.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  PyObject_HEAD
  struct upb_context *context;
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

PyUpb_MsgDef *get_or_create_msgdef(struct upb_msgdef *def);

#define RETURN_BOOL(val) if(val) { Py_RETURN_TRUE; } else { Py_RETURN_FALSE; }

#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif
