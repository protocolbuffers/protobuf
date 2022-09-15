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

#ifndef PYUPB_DESCRIPTOR_H__
#define PYUPB_DESCRIPTOR_H__

#include <stdbool.h>

#include "python/python_api.h"
#include "upb/reflection/def.h"

typedef enum {
  kPyUpb_Descriptor = 0,
  kPyUpb_EnumDescriptor = 1,
  kPyUpb_EnumValueDescriptor = 2,
  kPyUpb_FieldDescriptor = 3,
  kPyUpb_FileDescriptor = 4,
  kPyUpb_MethodDescriptor = 5,
  kPyUpb_OneofDescriptor = 6,
  kPyUpb_ServiceDescriptor = 7,
  kPyUpb_Descriptor_Count = 8,
} PyUpb_DescriptorType;

// Given a descriptor object |desc|, returns a Python message class object for
// the msgdef |m|, which must be from the same pool.
PyObject* PyUpb_Descriptor_GetClass(const upb_MessageDef* m);

// Returns a Python wrapper object for the given def. This will return an
// existing object if one already exists, otherwise a new object will be
// created.  The caller always owns a ref on the returned object.
PyObject* PyUpb_Descriptor_Get(const upb_MessageDef* msgdef);
PyObject* PyUpb_EnumDescriptor_Get(const upb_EnumDef* enumdef);
PyObject* PyUpb_FieldDescriptor_Get(const upb_FieldDef* field);
PyObject* PyUpb_FileDescriptor_Get(const upb_FileDef* file);
PyObject* PyUpb_OneofDescriptor_Get(const upb_OneofDef* oneof);
PyObject* PyUpb_EnumValueDescriptor_Get(const upb_EnumValueDef* enumval);
PyObject* PyUpb_Descriptor_GetOrCreateWrapper(const upb_MessageDef* msg);
PyObject* PyUpb_ServiceDescriptor_Get(const upb_ServiceDef* s);
PyObject* PyUpb_MethodDescriptor_Get(const upb_MethodDef* s);

// Returns the underlying |def| for a given wrapper object. The caller must
// have already verified that the given Python object is of the expected type.
const upb_FileDef* PyUpb_FileDescriptor_GetDef(PyObject* file);
const upb_FieldDef* PyUpb_FieldDescriptor_GetDef(PyObject* file);
const upb_MessageDef* PyUpb_Descriptor_GetDef(PyObject* _self);
const void* PyUpb_AnyDescriptor_GetDef(PyObject* _self);

// Returns the underlying |def| for a given wrapper object. The caller must
// have already verified that the given Python object is of the expected type.
const upb_FileDef* PyUpb_FileDescriptor_GetDef(PyObject* file);

// Module-level init.
bool PyUpb_InitDescriptor(PyObject* m);

#endif  // PYUPB_DESCRIPTOR_H__
