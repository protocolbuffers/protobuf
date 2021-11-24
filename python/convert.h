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
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL Google LLC BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef PYUPB_CONVERT_H__
#define PYUPB_CONVERT_H__

#include "upb/def.h"
#include "upb/reflection.h"

#include "protobuf.h"

// Converts `val` to a Python object according to the type information in `f`.
// Any newly-created objects that reference non-primitive data from `val` should
// reference `arena` to keep the referent alive.  If the conversion cannot be
// performed, returns NULL and sets a Python error.
PyObject *PyUpb_UpbToPy(upb_msgval val, const upb_fielddef *f, PyObject *arena);

// Converts `obj` to a upb_msgval `*val` according to the type information in
// `f`. If `arena` is provided, any string data will be copied into `arena`,
// otherwise the returned value will alias the Python-owned data (this can be
// useful for an ephemeral upb_msgval).  If the conversion cannot be performed,
// returns false.
bool PyUpb_PyToUpb(PyObject *obj, const upb_fielddef *f, upb_msgval *val,
                   upb_arena *arena);

// Returns true if the given values (of type `f`) are equal.
bool PyUpb_ValueEq(upb_msgval val1, upb_msgval val2, const upb_fielddef *f);

// Returns true if the given messages (of type `m`) are equal.
bool PyUpb_Message_IsEqual(const upb_msg *msg1, const upb_msg *msg2,
                           const upb_msgdef *m);

// Returns true if the two arrays (with element type `f`) are equal.
bool PyUpb_Array_IsEqual(const upb_array *arr1, const upb_array *arr2,
                         const upb_fielddef *f);

#endif  // PYUPB_CONVERT_H__
