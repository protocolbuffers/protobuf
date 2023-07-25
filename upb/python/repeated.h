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

#ifndef PYUPB_REPEATED_H__
#define PYUPB_REPEATED_H__

#include <stdbool.h>

#include "python/python_api.h"
#include "upb/reflection/def.h"

// Creates a new repeated field stub for field `f` of message object `parent`.
// Precondition: `parent` must be a stub.
PyObject* PyUpb_RepeatedContainer_NewStub(PyObject* parent,
                                          const upb_FieldDef* f,
                                          PyObject* arena);

// Returns a repeated field object wrapping `arr`, of field type `f`, which
// must be on `arena`.  If an existing wrapper object exists, it will be
// returned, otherwise a new object will be created.  The caller always owns a
// ref on the returned value.
PyObject* PyUpb_RepeatedContainer_GetOrCreateWrapper(upb_Array* arr,
                                                     const upb_FieldDef* f,
                                                     PyObject* arena);

// Reifies a repeated field stub to point to the concrete data in `arr`.
// If `arr` is NULL, an appropriate empty array will be constructed.
void PyUpb_RepeatedContainer_Reify(PyObject* self, upb_Array* arr);

// Reifies this repeated object if it is not already reified.
upb_Array* PyUpb_RepeatedContainer_EnsureReified(PyObject* self);

// Implements repeated_field.extend(iterable).  `_self` must be a repeated
// field (either repeated composite or repeated scalar).
PyObject* PyUpb_RepeatedContainer_Extend(PyObject* _self, PyObject* value);

// Implements repeated_field.add(initial_values).  `_self` must be a repeated
// composite field.
PyObject* PyUpb_RepeatedCompositeContainer_Add(PyObject* _self, PyObject* args,
                                               PyObject* kwargs);

// Module-level init.
bool PyUpb_Repeated_Init(PyObject* m);

#endif  // PYUPB_REPEATED_H__
