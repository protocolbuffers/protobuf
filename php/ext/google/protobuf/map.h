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

#ifndef PHP_PROTOBUF_MAP_H_
#define PHP_PROTOBUF_MAP_H_

#include <php.h>

#include "def.h"
#include "php-upb.h"

void Map_ModuleInit();

typedef struct {
  upb_fieldtype_t key_type;
  TypeInfo val_type;
} MapField_Type;

MapField_Type MapType_Get(const upb_fielddef *f);

// Gets a upb_map* for the PHP object |val|:
//  * If |val| is a RepeatedField object, we first check its type and verify
//    that that the elements have the correct type for |f|. If so, we return the
//    wrapped upb_map*. We also make sure that this map's arena is fused to
//    |arena|, so the returned upb_map is guaranteed to live as long as
//    |arena|.
//  * If |val| is a PHP Map, we attempt to create a new upb_map using
//    |arena| and add all of the PHP elements to it.
//
// If an error occurs, we raise a PHP error and return NULL.
upb_map *MapField_GetUpbMap(zval *val, MapField_Type type, upb_arena *arena);

// Creates a PHP MapField object for the given upb_map* and |f| and returns it
// in |val|. The PHP object will keep a reference to this |arena| to ensure the
// underlying array data stays alive.
//
// If |map| is NULL, this will return a PHP null object.
void MapField_GetPhpWrapper(zval *val, upb_map *arr, MapField_Type type,
                            zval *arena);

bool MapEq(const upb_map *m1, const upb_map *m2, MapField_Type type);

#endif  // PHP_PROTOBUF_MAP_H_
