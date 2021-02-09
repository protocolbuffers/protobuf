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

#ifndef PHP_PROTOBUF_H_
#define PHP_PROTOBUF_H_

#include <php.h>
#include <stdbool.h>

#include "php-upb.h"

const zval *get_generated_pool();

#if PHP_VERSION_ID < 70300
#define GC_ADDREF(h) ++GC_REFCOUNT(h)
#define GC_DELREF(h) --GC_REFCOUNT(h)
#endif

// Since php 7.4, the write_property() object handler now returns the assigned
// value (after possible type coercions) rather than void.
// https://github.com/php/php-src/blob/PHP-7.4.0/UPGRADING.INTERNALS#L171-L173
#if PHP_VERSION_ID < 70400
#define PROTO_RETURN_VAL void
#else
#define PROTO_RETURN_VAL zval*
#endif

// Sine php 8.0, the Object Handlers API was changed to receive zend_object*
// instead of zval* and zend_string* instead of zval* for property names.
// https://github.com/php/php-src/blob/php-8.0.0beta1/UPGRADING.INTERNALS#L37-L39
#if PHP_VERSION_ID < 80000
#define PROTO_VAL zval
#define PROTO_STR zval
#define PROTO_MSG_P(obj) (Message*)Z_OBJ_P(obj)
#define PROTO_STRVAL_P(obj) Z_STRVAL_P(obj)
#define PROTO_STRLEN_P(obj) Z_STRLEN_P(obj)
#else
#define PROTO_VAL zend_object
#define PROTO_STR zend_string
#define PROTO_MSG_P(obj) (Message*)(obj)
#define PROTO_STRVAL_P(obj) ZSTR_VAL(obj)
#define PROTO_STRLEN_P(obj) ZSTR_LEN(obj)
#endif

#define PHP_PROTOBUF_VERSION "3.14.0"

// ptr -> PHP object cache. This is a weak map that caches lazily-created
// wrapper objects around upb types:
//  * upb_msg* -> Message
//  * upb_array* -> RepeatedField
//  * upb_map*, -> MapField
//  * upb_msgdef* -> Descriptor
//  * upb_enumdef* -> EnumDescriptor
//  * zend_class_entry* -> Descriptor
//
// Each wrapped object should add itself to the map when it is constructed, and
// remove itself from the map when it is destroyed. This is how we ensure that
// the map only contains live objects. The map is weak so it does not actually
// take references to the cached objects.
void ObjCache_Add(const void *key, zend_object *php_obj);
void ObjCache_Delete(const void *key);
bool ObjCache_Get(const void *key, zval *val);

// PHP class name map. This is necessary because the pb_name->php_class_name
// transformation is non-reversible, so when we need to look up a msgdef or
// enumdef by PHP class, we can't turn the class name into a pb_name.
//  * php_class_name -> upb_msgdef*
//  * php_class_name -> upb_enumdef*
void NameMap_AddMessage(const upb_msgdef *m);
void NameMap_AddEnum(const upb_enumdef *m);
const upb_msgdef *NameMap_GetMessage(zend_class_entry *ce);
const upb_enumdef *NameMap_GetEnum(zend_class_entry *ce);

// We need our own assert() because PHP takes control of NDEBUG in its headers.
#ifdef PBPHP_ENABLE_ASSERTS
#define PBPHP_ASSERT(x)                                                    \
  do {                                                                     \
    if (!(x)) {                                                            \
      fprintf(stderr, "Assertion failure at %s:%d %s", __FILE__, __LINE__, \
              #x);                                                         \
      abort();                                                             \
    }                                                                      \
  } while (false)
#else
#define PBPHP_ASSERT(x) \
  do {                  \
  } while (false && (x))
#endif

#endif  // PHP_PROTOBUF_H_
