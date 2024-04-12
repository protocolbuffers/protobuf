// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
// https://developers.google.com/protocol-buffers/
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#ifndef PHP_PROTOBUF_ARRAY_H_
#define PHP_PROTOBUF_ARRAY_H_

#include <php.h>

#include "def.h"
#include "php-upb.h"

// Registers PHP classes for RepeatedField.
void Array_ModuleInit();

// Gets a upb_Array* for the PHP object |val|:
//  * If |val| is a RepeatedField object, we first check its type and verify
//    that that the elements have the correct type for |type|. If so, we return
//    the wrapped upb_Array*. We also make sure that this array's arena is fused
//    to |arena|, so the returned upb_Array is guaranteed to live as long as
//    |arena|.
//  * If |val| is a PHP Array, we attempt to create a new upb_Array using
//    |arena| and add all of the PHP elements to it.
//
// If an error occurs, we raise a PHP error and return NULL.
upb_Array *RepeatedField_GetUpbArray(zval *val, TypeInfo type,
                                     upb_Arena *arena);

// Creates a PHP RepeatedField object for the given upb_Array* and |type| and
// returns it in |val|. The PHP object will keep a reference to this |arena| to
// ensure the underlying array data stays alive.
//
// If |arr| is NULL, this will return a PHP null object.
void RepeatedField_GetPhpWrapper(zval *val, upb_Array *arr, TypeInfo type,
                                 zval *arena);

// Returns true if the given arrays are equal. Both arrays must be of this
// |type| and, if the type is |kUpb_CType_Message|, must have the same |m|.
bool ArrayEq(const upb_Array *a1, const upb_Array *a2, TypeInfo type);

#endif  // PHP_PROTOBUF_ARRAY_H_
