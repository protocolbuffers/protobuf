/*
 * upb - a minimalist implementation of protocol buffers.
 *
 * Copyright (c) 2009 Joshua Haberman.  See LICENSE for details.
 *
 */

#include "cext.h"

PyMODINIT_FUNC
initcext(void)
{
  PyObject *mod = Py_InitModule("upb.cext", NULL);
  initdefinition();
  initpb();
}
