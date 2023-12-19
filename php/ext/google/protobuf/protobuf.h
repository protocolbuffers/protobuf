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

upb_DefPool *get_global_symtab();

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

// Since php 8.0, the Object Handlers API was changed to receive zend_object*
// instead of zval* and zend_string* instead of zval* for property names.
// https://github.com/php/php-src/blob/php-8.0.0beta1/UPGRADING.INTERNALS#L37-L39
#if PHP_VERSION_ID < 80000
#define PROTO_VAL zval
#define PROTO_STR zval
#define PROTO_VAL_P(obj) (void*)Z_OBJ_P(obj)
#define PROTO_STRVAL_P(obj) Z_STRVAL_P(obj)
#define PROTO_STRLEN_P(obj) Z_STRLEN_P(obj)
#define ZVAL_OBJ_COPY(z, o) do { ZVAL_OBJ(z, o); GC_ADDREF(o); } while (0)
#define RETVAL_OBJ_COPY(r) ZVAL_OBJ_COPY(return_value, r)
#define RETURN_OBJ_COPY(r) do { RETVAL_OBJ_COPY(r); return; } while (0)
#define RETURN_COPY(zv) do { ZVAL_COPY(return_value, zv); return; } while (0)
#define RETURN_COPY_VALUE(zv) do { ZVAL_COPY_VALUE(return_value, zv); return; } while (0)
#else
#define PROTO_VAL zend_object
#define PROTO_STR zend_string
#define PROTO_VAL_P(obj) (void*)(obj)
#define PROTO_STRVAL_P(obj) ZSTR_VAL(obj)
#define PROTO_STRLEN_P(obj) ZSTR_LEN(obj)
#endif

// In PHP 8.1, several old interfaces are removed:
// https://github.com/php/php-src/blob/14f599ea7def7c7a59c40aff763ce8b105573e7a/UPGRADING.INTERNALS#L27-L31
//
// We now use the new interfaces (zend_ce_arrayaccess and zend_ce_countable).
// However we have to polyfill zend_ce_countable, which was only introduced in
// PHP 7.2.0.
#if PHP_VERSION_ID < 70200
#define zend_ce_countable spl_ce_Countable
#define ZEND_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(name, return_reference, required_num_args, class_name, allow_null) \
        ZEND_BEGIN_ARG_INFO_EX(name, return_reference, required_num_args, allow_null)
#endif

// polyfill for ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX, which changes between 7.1 and 7.2
#if PHP_VERSION_ID < 70200
#define PROTOBUF_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(name, return_reference, required_num_args, type, allow_null) \
        ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(name, return_reference, required_num_args, type, /*class_name*/ 0, allow_null)
#else
#define PROTOBUF_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(name, return_reference, required_num_args, type, allow_null) \
        ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(name, return_reference, required_num_args, type, allow_null)
#endif

// In PHP 8.1, mismatched tentative return types emit a deprecation notice.
// https://wiki.php.net/rfc/internal_method_return_types
//
// When compiling for earlier php versions, the return type is dropped.
#if PHP_VERSION_ID < 80100
#define ZEND_BEGIN_ARG_WITH_TENTATIVE_RETURN_TYPE_INFO_EX(name, return_reference, required_num_args, type, allow_null) \
        ZEND_BEGIN_ARG_INFO_EX(name, return_reference, required_num_args, allow_null)
#endif

#ifndef IS_VOID
#define IS_VOID 99
#endif

#ifndef IS_MIXED
#define IS_MIXED 99
#endif

#ifndef _IS_BOOL
#define _IS_BOOL 99
#endif

#ifndef IS_LONG
#define IS_LONG 99
#endif

ZEND_BEGIN_ARG_INFO(arginfo_void, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_setter, 0, 0, 1)
  ZEND_ARG_INFO(0, value)
ZEND_END_ARG_INFO()

#define PHP_PROTOBUF_VERSION "3.21.12"

// ptr -> PHP object cache. This is a weak map that caches lazily-created
// wrapper objects around upb types:
//  * upb_Message* -> Message
//  * upb_Array* -> RepeatedField
//  * upb_Map*, -> MapField
//  * upb_MessageDef* -> Descriptor
//  * upb_EnumDef* -> EnumDescriptor
//  * upb_MessageDef* -> Descriptor
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
//  * php_class_name -> upb_MessageDef*
//  * php_class_name -> upb_EnumDef*
void NameMap_AddMessage(const upb_MessageDef *m);
void NameMap_AddEnum(const upb_EnumDef *m);
const upb_MessageDef *NameMap_GetMessage(zend_class_entry *ce);
const upb_EnumDef *NameMap_GetEnum(zend_class_entry *ce);
void NameMap_EnterConstructor(zend_class_entry* ce);
void NameMap_ExitConstructor(zend_class_entry* ce);

// Add this descriptor object to the global list of descriptors that will be
// kept alive for the duration of the request but destroyed when the request
// is ending.
void Descriptors_Add(zend_object *desc);

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
