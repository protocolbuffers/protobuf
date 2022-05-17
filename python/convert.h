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

#ifndef PYUPB_CONVERT_H__
#define PYUPB_CONVERT_H__

#include "protobuf.h"
#include "upb/def.h"
#include "upb/reflection.h"

// Converts `val` to a Python object according to the type information in `f`.
// Any newly-created Python objects that reference non-primitive data from `val`
// will take a reference on `arena`; the caller must ensure that `val` belongs
// to `arena`. If the conversion cannot be performed, returns NULL and sets a
// Python error.
PyObject* PyUpb_UpbToPy(upb_MessageValue val, const upb_FieldDef* f,
                        PyObject* arena);

// Converts `obj` to a upb_MessageValue `*val` according to the type information
// in `f`. If `arena` is provided, any string data will be copied into `arena`,
// otherwise the returned value will alias the Python-owned data (this can be
// useful for an ephemeral upb_MessageValue).  If the conversion cannot be
// performed, returns false.
bool PyUpb_PyToUpb(PyObject* obj, const upb_FieldDef* f, upb_MessageValue* val,
                   upb_Arena* arena);

// Returns true if the given values (of type `f`) are equal.
bool PyUpb_ValueEq(upb_MessageValue val1, upb_MessageValue val2,
                   const upb_FieldDef* f);

// Returns true if the two arrays (with element type `f`) are equal.
bool PyUpb_Array_IsEqual(const upb_Array* arr1, const upb_Array* arr2,
                         const upb_FieldDef* f);

// Returns true if the given messages (of type `m`) are equal.
bool upb_Message_IsEqual(const upb_Message* msg1, const upb_Message* msg2,
                         const upb_MessageDef* m);

#endif  // PYUPB_CONVERT_H__
