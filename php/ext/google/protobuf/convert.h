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

#ifndef PHP_PROTOBUF_CONVERT_H_
#define PHP_PROTOBUF_CONVERT_H_

#include <php.h>

#include "php-upb.h"
#include "def.h"

upb_fieldtype_t pbphp_dtype_to_type(upb_descriptortype_t type);

// Converts |php_val| to an int64_t. Returns false if the value cannot be
// converted.
bool Convert_PhpToInt64(const zval *php_val, int64_t *i64);

// Converts |php_val| to a upb_msgval according to |type|. If type is
// UPB_TYPE_MESSAGE, then |desc| must be the Descriptor for this message type.
// If type is string, message, or bytes, then |arena| will be used to copy
// string data or fuse this arena to the given message's arena.
bool Convert_PhpToUpb(zval *php_val, upb_msgval *upb_val, TypeInfo type,
                      upb_arena *arena);

// Similar to Convert_PhpToUpb, but supports automatically wrapping the wrapper
// types if a primitive is specified:
//
//   5 -> Int64Wrapper(value=5)
//
// We currently allow this implicit conversion in initializers, but not for
// assignment.
bool Convert_PhpToUpbAutoWrap(zval *val, upb_msgval *upb_val, TypeInfo type,
                              upb_arena *arena);

// Converts |upb_val| to a PHP zval according to |type|. This may involve
// creating a PHP wrapper object. If type == UPB_TYPE_MESSAGE, then |desc| must
// be the Descriptor for this message type. Any newly created wrapper object
// will reference |arena|.
void Convert_UpbToPhp(upb_msgval upb_val, zval *php_val, TypeInfo type,
                      zval *arena);

// Registers the GPBUtil class.
void Convert_ModuleInit(void);

#endif  // PHP_PROTOBUF_CONVERT_H_
