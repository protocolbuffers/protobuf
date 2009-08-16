/*
 * upb - a minimalist implementation of protocol buffers.
 *
 * Copyright (c) 2009 Joshua Haberman.  See LICENSE for details.
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
  struct upb_msgdef *def;
} PyUpb_MessageDefinition;

extern PyTypeObject PyUpb_MessageDefinitionType;

/* What format string should be passed to PyArg_ParseTuple to get just a raw
 * string of bytes and a length. */
extern const char *bytes_format;

#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif
