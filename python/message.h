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

#ifndef PYPB_MESSAGE_H__
#define PYPB_MESSAGE_H__

#include <stdbool.h>

#include "python/protobuf.h"
#include "upb/reflection.h"

// Removes the wrapper object for this field from the unset subobject cache.
void PyUpb_CMessage_CacheDelete(PyObject* _self, const upb_fielddef* f);

// Sets the field value for `f` to `subobj`, evicting the wrapper object from
// the "unset subobject" cache now that real data exists for it.  The caller
// must also update the wrapper associated with `f` to point to `subobj` also.
void PyUpb_CMessage_SetConcreteSubobj(PyObject* _self, const upb_fielddef* f,
                                      upb_msgval subobj);

// Gets a Python wrapper object for message `u_msg` of type `m`, returning a
// cached wrapper if one was previously created.  If a new object is created,
// it will reference `arena`, which must own `u_msg`.
PyObject* PyUpb_CMessage_Get(upb_msg* u_msg, const upb_msgdef* m,
                             PyObject* arena);

// Verifies that a Python object is a message.  Sets a TypeError exception and
// returns false on failure.
bool PyUpb_CMessage_Check(PyObject* self);

// Gets the upb_msg* for this message object if the message is set/writable.
// Otherwise returns NULL.
upb_msg* PyUpb_CMessage_GetIfWritable(PyObject* _self);

// Returns the `upb_msgdef` for a given CMessage.
const upb_msgdef* PyUpb_CMessage_GetMsgdef(PyObject* self);

// Functions that match the corresponding methods on the message object.
PyObject* PyUpb_CMessage_MergeFrom(PyObject* self, PyObject* arg);
PyObject* PyUpb_CMessage_MergeFromString(PyObject* self, PyObject* arg);
PyObject* PyUpb_CMessage_SerializeToString(PyObject* self, PyObject* args,
                                           PyObject* kwargs);

// Sets fields of the message according to the attribuges in `kwargs`.
int PyUpb_CMessage_InitAttributes(PyObject* _self, PyObject* args,
                                  PyObject* kwargs);

// Clears the ExtensionDict from the message.  The message must have an
// ExtensionDict set.
void PyUpb_CMessage_ClearExtensionDict(PyObject* _self);

// Implements the equivalent of getattr(msg, field), once `field` has
// already been resolved to a `upb_fielddef*`.
PyObject* PyUpb_CMessage_GetFieldValue(PyObject* _self,
                                       const upb_fielddef* field);

// Implements the equivalent of setattr(msg, field, value), once `field` has
// already been resolved to a `upb_fielddef*`.
int PyUpb_CMessage_SetFieldValue(PyObject* _self, const upb_fielddef* field,
                                 PyObject* value);

// Returns the version associated with this message.  The version will be
// incremented when the message changes.
int PyUpb_CMessage_GetVersion(PyObject* _self);

// Module-level init.
bool PyUpb_InitMessage(PyObject* m);

#endif  // PYPB_MESSAGE_H__
