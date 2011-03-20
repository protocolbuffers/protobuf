/*
 * upb - a minimalist implementation of protocol buffers.
 *
 * Copyright (c) 2009 Google Inc.  See LICENSE for details.
 * Author: Josh Haberman <jhaberman@gmail.com>
 */

#include "cext.h"

PyMODINIT_FUNC
initcext(void)
{
  PyObject *mod = Py_InitModule("upb.cext", NULL);
  (void)mod;
  initdefinition();
  initpb();
}
