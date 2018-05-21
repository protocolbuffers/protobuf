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

#ifndef __GOOGLE_PROTOBUF_PHP_PROTOBUF_H__
#define __GOOGLE_PROTOBUF_PHP_PROTOBUF_H__

#include <php.h>

// ubp.h has to be placed after php.h. Othwise, php.h will introduce NDEBUG.
#include "upb.h"

#define PHP_PROTOBUF_EXTNAME "protobuf"
#define PHP_PROTOBUF_VERSION "3.5.2"

#define MAX_LENGTH_OF_INT64 20
#define SIZEOF_INT64 8

// -----------------------------------------------------------------------------
// PHP7 Wrappers
// ----------------------------------------------------------------------------

#if PHP_MAJOR_VERSION < 7

#define php_proto_zend_literal const zend_literal*
#define PHP_PROTO_CASE_IS_BOOL IS_BOOL
#define PHP_PROTO_SIZE int
#define PHP_PROTO_LONG long
#define PHP_PROTO_TSRMLS_DC TSRMLS_DC
#define PHP_PROTO_TSRMLS_CC TSRMLS_CC

// PHP String

#define PHP_PROTO_ZVAL_STRING(zval_ptr, s, copy) \
  ZVAL_STRING(zval_ptr, s, copy)
#define PHP_PROTO_ZVAL_STRINGL(zval_ptr, s, len, copy) \
  ZVAL_STRINGL(zval_ptr, s, len, copy)
#define PHP_PROTO_RETURN_STRING(s, copy) RETURN_STRING(s, copy)
#define PHP_PROTO_RETURN_STRINGL(s, len, copy) RETURN_STRINGL(s, len, copy)
#define PHP_PROTO_RETVAL_STRINGL(s, len, copy) RETVAL_STRINGL(s, len, copy)
#define php_proto_zend_make_printable_zval(from, to) \
  {                                                  \
    int use_copy;                                    \
    zend_make_printable_zval(from, to, &use_copy);   \
  }

// PHP Array

#define PHP_PROTO_HASH_OF(array) Z_ARRVAL_P(array)

#define php_proto_zend_hash_index_update_zval(ht, h, pData) \
  zend_hash_index_update(ht, h, &(pData), sizeof(void*), NULL)

#define php_proto_zend_hash_update_zval(ht, key, key_len, value) \
  zend_hash_update(ht, key, key_len, value, sizeof(void*), NULL)

#define php_proto_zend_hash_update(ht, key, key_len) \
  zend_hash_update(ht, key, key_len, 0, 0, NULL)

#define php_proto_zend_hash_index_update_mem(ht, h, pData, nDataSize, pDest) \
  zend_hash_index_update(ht, h, pData, nDataSize, pDest)

#define php_proto_zend_hash_update_mem(ht, key, key_len, pData, nDataSize, \
                                       pDest)                              \
  zend_hash_update(ht, key, key_len, pData, nDataSize, pDest)

#define php_proto_zend_hash_index_find_zval(ht, h, pDest) \
  zend_hash_index_find(ht, h, pDest)

#define php_proto_zend_hash_find(ht, key, key_len, pDest) \
  zend_hash_find(ht, key, key_len, pDest)

#define php_proto_zend_hash_index_find_mem(ht, h, pDest) \
  zend_hash_index_find(ht, h, pDest)

#define php_proto_zend_hash_find_zval(ht, key, key_len, pDest) \
  zend_hash_find(ht, key, key_len, pDest)

#define php_proto_zend_hash_find_mem(ht, key, key_len, pDest) \
  zend_hash_find(ht, key, key_len, pDest)

#define php_proto_zend_hash_next_index_insert_zval(ht, pData) \
  zend_hash_next_index_insert(ht, pData, sizeof(void*), NULL)

#define php_proto_zend_hash_next_index_insert_mem(ht, pData, nDataSize, pDest) \
  zend_hash_next_index_insert(ht, pData, nDataSize, pDest)

#define php_proto_zend_hash_get_current_data_ex(ht, pDest, pos) \
  zend_hash_get_current_data_ex(ht, pDest, pos)

// PHP Object

#define PHP_PROTO_WRAP_OBJECT_START(name) \
  struct name {                           \
    zend_object std;
#define PHP_PROTO_WRAP_OBJECT_END \
  };

#define PHP_PROTO_INIT_SUBMSGCLASS_START(CLASSNAME, CAMELNAME, LOWWERNAME)   \
  void LOWWERNAME##_init(TSRMLS_D) {                                         \
    zend_class_entry class_type;                                             \
    const char* class_name = CLASSNAME;                                      \
    INIT_CLASS_ENTRY_EX(class_type, CLASSNAME, strlen(CLASSNAME),            \
                        LOWWERNAME##_methods);                               \
    LOWWERNAME##_type = zend_register_internal_class(&class_type TSRMLS_CC); \
    LOWWERNAME##_type->create_object = message_create;
#define PHP_PROTO_INIT_SUBMSGCLASS_END \
  }

#define PHP_PROTO_INIT_ENUMCLASS_START(CLASSNAME, CAMELNAME, LOWWERNAME)     \
  void LOWWERNAME##_init(TSRMLS_D) {                                         \
    zend_class_entry class_type;                                             \
    const char* class_name = CLASSNAME;                                      \
    INIT_CLASS_ENTRY_EX(class_type, CLASSNAME, strlen(CLASSNAME),            \
                        LOWWERNAME##_methods);                               \
    LOWWERNAME##_type = zend_register_internal_class(&class_type TSRMLS_CC);
#define PHP_PROTO_INIT_ENUMCLASS_END \
  }

#define PHP_PROTO_INIT_CLASS_START(CLASSNAME, CAMELNAME, LOWWERNAME)         \
  void LOWWERNAME##_init(TSRMLS_D) {                                         \
    zend_class_entry class_type;                                             \
    const char* class_name = CLASSNAME;                                      \
    INIT_CLASS_ENTRY_EX(class_type, CLASSNAME, strlen(CLASSNAME),            \
                        LOWWERNAME##_methods);                               \
    LOWWERNAME##_type = zend_register_internal_class(&class_type TSRMLS_CC); \
    LOWWERNAME##_type->create_object = LOWWERNAME##_create;                  \
    LOWWERNAME##_handlers = PEMALLOC(zend_object_handlers);                  \
    memcpy(LOWWERNAME##_handlers, zend_get_std_object_handlers(),            \
           sizeof(zend_object_handlers));
#define PHP_PROTO_INIT_CLASS_END \
  }

#define PHP_PROTO_OBJECT_CREATE_START(NAME, LOWWERNAME) \
  static zend_object_value LOWWERNAME##_create(         \
      zend_class_entry* ce TSRMLS_DC) {                 \
    PHP_PROTO_ALLOC_CLASS_OBJECT(NAME, ce);             \
    zend_object_std_init(&intern->std, ce TSRMLS_CC);   \
    object_properties_init(&intern->std, ce);
#define PHP_PROTO_OBJECT_CREATE_END(NAME, LOWWERNAME)                          \
  PHP_PROTO_FREE_CLASS_OBJECT(NAME, LOWWERNAME##_free, LOWWERNAME##_handlers); \
  }

#define PHP_PROTO_OBJECT_FREE_START(classname, lowername) \
  void lowername##_free(void* object TSRMLS_DC) {         \
    classname* intern = object;
#define PHP_PROTO_OBJECT_FREE_END                 \
    zend_object_std_dtor(&intern->std TSRMLS_CC); \
    efree(intern);                                \
  }

#define PHP_PROTO_OBJECT_DTOR_START(classname, lowername)
#define PHP_PROTO_OBJECT_DTOR_END

#define CACHED_VALUE zval*
#define CACHED_TO_ZVAL_PTR(VALUE) (VALUE)
#define CACHED_PTR_TO_ZVAL_PTR(VALUE) (*VALUE)
#define ZVAL_PTR_TO_CACHED_PTR(VALUE) (&VALUE)
#define ZVAL_PTR_TO_CACHED_VALUE(VALUE) (VALUE)
#define ZVAL_TO_CACHED_VALUE(VALUE) (&VALUE)

#define CREATE_OBJ_ON_ALLOCATED_ZVAL_PTR(zval_ptr, class_type) \
  ZVAL_OBJ(zval_ptr, class_type->create_object(class_type TSRMLS_CC));

#define PHP_PROTO_SEPARATE_ZVAL_IF_NOT_REF(value) \
  SEPARATE_ZVAL_IF_NOT_REF(value)

#define PHP_PROTO_GLOBAL_UNINITIALIZED_ZVAL EG(uninitialized_zval_ptr)

#define OBJ_PROP(OBJECT, OFFSET) &((OBJECT)->properties_table[OFFSET])

#define php_proto_zval_ptr_dtor(zval_ptr) \
  zval_ptr_dtor(&(zval_ptr))

#define PHP_PROTO_ALLOC_CLASS_OBJECT(class_object, class_type) \
  class_object* intern;                                        \
  intern = (class_object*)emalloc(sizeof(class_object));       \
  memset(intern, 0, sizeof(class_object));

#define PHP_PROTO_FREE_CLASS_OBJECT(class_object, class_object_free, handler) \
  zend_object_value retval = {0};                                             \
  retval.handle = zend_objects_store_put(                                     \
      intern, (zend_objects_store_dtor_t)zend_objects_destroy_object,         \
      class_object_free, NULL TSRMLS_CC);                                     \
  retval.handlers = handler;                                                  \
  return retval;

#define PHP_PROTO_ALLOC_ARRAY(zval_ptr)  \
  ALLOC_HASHTABLE(Z_ARRVAL_P(zval_ptr)); \
  Z_TYPE_P(zval_ptr) = IS_ARRAY;

#define ZVAL_OBJ(zval_ptr, call_create) \
  Z_TYPE_P(zval_ptr) = IS_OBJECT;       \
  Z_OBJVAL_P(zval_ptr) = call_create;

#define UNBOX(class_name, val) \
  (class_name*)zend_object_store_get_object(val TSRMLS_CC);

#define UNBOX_HASHTABLE_VALUE(class_name, val) UNBOX(class_name, val)

#define HASHTABLE_VALUE_DTOR ZVAL_PTR_DTOR

#define PHP_PROTO_HASHTABLE_VALUE zval*
#define HASHTABLE_VALUE_CE(val) Z_OBJCE_P(val)

#define CREATE_HASHTABLE_VALUE(OBJ, WRAPPED_OBJ, OBJ_TYPE, OBJ_CLASS_ENTRY) \
  OBJ_TYPE* OBJ;                                                            \
  PHP_PROTO_HASHTABLE_VALUE WRAPPED_OBJ;                                    \
  MAKE_STD_ZVAL(WRAPPED_OBJ);                                               \
  ZVAL_OBJ(WRAPPED_OBJ,                                                     \
           OBJ_CLASS_ENTRY->create_object(OBJ_CLASS_ENTRY TSRMLS_CC));      \
  OBJ = UNBOX_HASHTABLE_VALUE(OBJ_TYPE, WRAPPED_OBJ);                       \
  Z_DELREF_P(desc_php);

#define PHP_PROTO_CE_DECLARE zend_class_entry**
#define PHP_PROTO_CE_UNREF(ce) (*ce)

#define php_proto_zend_lookup_class(name, name_length, ce) \
  zend_lookup_class(name, name_length, ce TSRMLS_CC)

#define PHP_PROTO_RETVAL_ZVAL(value) ZVAL_ZVAL(return_value, value, 1, 0)

#else  // PHP_MAJOR_VERSION >= 7

#define php_proto_zend_literal void**
#define PHP_PROTO_CASE_IS_BOOL IS_TRUE: case IS_FALSE
#define PHP_PROTO_SIZE size_t
#define PHP_PROTO_LONG zend_long
#define PHP_PROTO_TSRMLS_DC
#define PHP_PROTO_TSRMLS_CC

// PHP String

#define PHP_PROTO_ZVAL_STRING(zval_ptr, s, copy) \
  ZVAL_STRING(zval_ptr, s)
#define PHP_PROTO_ZVAL_STRINGL(zval_ptr, s, len, copy) \
  ZVAL_STRINGL(zval_ptr, s, len)
#define PHP_PROTO_RETURN_STRING(s, copy) RETURN_STRING(s)
#define PHP_PROTO_RETURN_STRINGL(s, len, copy) RETURN_STRINGL(s, len)
#define PHP_PROTO_RETVAL_STRINGL(s, len, copy) RETVAL_STRINGL(s, len)
#define php_proto_zend_make_printable_zval(from, to) \
  zend_make_printable_zval(from, to)

// PHP Array

#define PHP_PROTO_HASH_OF(array) Z_ARRVAL_P(&array)

static inline int php_proto_zend_hash_index_update_zval(HashTable* ht, ulong h,
                                                        zval* pData) {
  void* result = NULL;
  result = zend_hash_index_update(ht, h, pData);
  return result != NULL ? SUCCESS : FAILURE;
}

static inline int php_proto_zend_hash_update(HashTable* ht, const char* key,
                                             size_t key_len) {
  void* result = NULL;
  zval temp;
  ZVAL_LONG(&temp, 0);
  result = zend_hash_str_update(ht, key, key_len, &temp);
  return result != NULL ? SUCCESS : FAILURE;
}

static inline int php_proto_zend_hash_index_update_mem(HashTable* ht, ulong h,
                                                   void* pData, uint nDataSize,
                                                   void** pDest) {
  void* result = NULL;
  result = zend_hash_index_update_mem(ht, h, pData, nDataSize);
  if (pDest != NULL) *pDest = result;
  return result != NULL ? SUCCESS : FAILURE;
}

static inline int php_proto_zend_hash_update_zval(HashTable* ht,
                                                  const char* key, uint key_len,
                                                  zval* pData) {
  zend_string* internal_key = zend_string_init(key, key_len, 0);
  zend_hash_update(ht, internal_key, pData);
}

static inline int php_proto_zend_hash_update_mem(HashTable* ht, const char* key,
                                                 uint key_len, void* pData,
                                                 uint nDataSize, void** pDest) {
  zend_string* internal_key = zend_string_init(key, key_len, 0);
  void* result = zend_hash_update_mem(ht, internal_key, pData, nDataSize);
  zend_string_release(internal_key);
  if (pDest != NULL) *pDest = result;
  return result != NULL ? SUCCESS : FAILURE;
}

static inline int php_proto_zend_hash_index_find_zval(const HashTable* ht,
                                                      ulong h, void** pDest) {
  zval* result = zend_hash_index_find(ht, h);
  if (pDest != NULL) *pDest = result;
  return result != NULL ? SUCCESS : FAILURE;
}

static inline int php_proto_zend_hash_find(const HashTable* ht, const char* key,
                                           size_t key_len, void** pDest) {
  void* result = NULL;
  result = zend_hash_str_find(ht, key, key_len);
  return result != NULL ? SUCCESS : FAILURE;
}

static inline int php_proto_zend_hash_index_find_mem(const HashTable* ht,
                                                     ulong h, void** pDest) {
  void* result = NULL;
  result = zend_hash_index_find_ptr(ht, h);
  if (pDest != NULL) *pDest = result;
  return result != NULL ? SUCCESS : FAILURE;
}

static inline int php_proto_zend_hash_find_zval(const HashTable* ht,
                                                const char* key, uint key_len,
                                                void** pDest) {
  zend_string* internal_key = zend_string_init(key, key_len, 1);
  zval* result = zend_hash_find(ht, internal_key);
  if (pDest != NULL) *pDest = result;
  return result != NULL ? SUCCESS : FAILURE;
}

static inline int php_proto_zend_hash_find_mem(const HashTable* ht,
                                                const char* key, uint key_len,
                                                void** pDest) {
  zend_string* internal_key = zend_string_init(key, key_len, 1);
  void* result = zend_hash_find_ptr(ht, internal_key);
  zend_string_release(internal_key);
  if (pDest != NULL) *pDest = result;
  return result != NULL ? SUCCESS : FAILURE;
}

static inline int php_proto_zend_hash_next_index_insert_zval(HashTable* ht,
                                                             void* pData) {
  zval tmp;
  ZVAL_OBJ(&tmp, *(zend_object**)pData);
  zval* result = zend_hash_next_index_insert(ht, &tmp);
  return result != NULL ? SUCCESS : FAILURE;
}

static inline int php_proto_zend_hash_next_index_insert_mem(HashTable* ht,
                                                            void* pData,
                                                            uint nDataSize,
                                                            void** pDest) {
  void* result = NULL;
  result = zend_hash_next_index_insert_mem(ht, pData, nDataSize);
  if (pDest != NULL) *pDest = result;
  return result != NULL ? SUCCESS : FAILURE;
}

static inline int php_proto_zend_hash_get_current_data_ex(HashTable* ht,
                                                          void** pDest,
                                                          HashPosition* pos) {
  void* result = NULL;
  result = zend_hash_get_current_data_ex(ht, pos);
  if (pDest != NULL) *pDest = result;
  return result != NULL ? SUCCESS : FAILURE;
}

// PHP Object

#define PHP_PROTO_WRAP_OBJECT_START(name) struct name {
#define PHP_PROTO_WRAP_OBJECT_END \
  zend_object std;                \
  };

#define PHP_PROTO_INIT_SUBMSGCLASS_START(CLASSNAME, CAMELNAME, LOWWERNAME)   \
  void LOWWERNAME##_init(TSRMLS_D) {                                         \
    zend_class_entry class_type;                                             \
    const char* class_name = CLASSNAME;                                      \
    INIT_CLASS_ENTRY_EX(class_type, CLASSNAME, strlen(CLASSNAME),            \
                        LOWWERNAME##_methods);                               \
    LOWWERNAME##_type = zend_register_internal_class(&class_type TSRMLS_CC); \
    LOWWERNAME##_type->create_object = message_create;
#define PHP_PROTO_INIT_SUBMSGCLASS_END \
  }

#define PHP_PROTO_INIT_ENUMCLASS_START(CLASSNAME, CAMELNAME, LOWWERNAME)     \
  void LOWWERNAME##_init(TSRMLS_D) {                                         \
    zend_class_entry class_type;                                             \
    const char* class_name = CLASSNAME;                                      \
    INIT_CLASS_ENTRY_EX(class_type, CLASSNAME, strlen(CLASSNAME),            \
                        LOWWERNAME##_methods);                               \
    LOWWERNAME##_type = zend_register_internal_class(&class_type TSRMLS_CC);
#define PHP_PROTO_INIT_ENUMCLASS_END \
  }

#define PHP_PROTO_INIT_CLASS_START(CLASSNAME, CAMELNAME, LOWWERNAME)         \
  void LOWWERNAME##_init(TSRMLS_D) {                                         \
    zend_class_entry class_type;                                             \
    const char* class_name = CLASSNAME;                                      \
    INIT_CLASS_ENTRY_EX(class_type, CLASSNAME, strlen(CLASSNAME),            \
                        LOWWERNAME##_methods);                               \
    LOWWERNAME##_type = zend_register_internal_class(&class_type TSRMLS_CC); \
    LOWWERNAME##_type->create_object = LOWWERNAME##_create;                  \
    LOWWERNAME##_handlers = PEMALLOC(zend_object_handlers);                  \
    memcpy(LOWWERNAME##_handlers, zend_get_std_object_handlers(),            \
           sizeof(zend_object_handlers));                                    \
    LOWWERNAME##_handlers->free_obj = LOWWERNAME##_free;                     \
    LOWWERNAME##_handlers->dtor_obj = LOWWERNAME##_dtor;                     \
    LOWWERNAME##_handlers->offset = XtOffsetOf(CAMELNAME, std);
#define PHP_PROTO_INIT_CLASS_END \
  }

#define PHP_PROTO_OBJECT_FREE_START(classname, lowername) \
  void lowername##_free(zend_object* object) {            \
    classname* intern =                                   \
        (classname*)((char*)object - XtOffsetOf(classname, std));
#define PHP_PROTO_OBJECT_FREE_END           \
  }

#define PHP_PROTO_OBJECT_DTOR_START(classname, lowername) \
  void lowername##_dtor(zend_object* object) {            \
    classname* intern =                                   \
        (classname*)((char*)object - XtOffsetOf(classname, std));
#define PHP_PROTO_OBJECT_DTOR_END           \
    zend_object_std_dtor(object TSRMLS_CC); \
  }

#define PHP_PROTO_OBJECT_CREATE_START(NAME, LOWWERNAME)                     \
  static zend_object* LOWWERNAME##_create(zend_class_entry* ce TSRMLS_DC) { \
    PHP_PROTO_ALLOC_CLASS_OBJECT(NAME, ce);                                 \
    zend_object_std_init(&intern->std, ce TSRMLS_CC);                       \
    object_properties_init(&intern->std, ce);
#define PHP_PROTO_OBJECT_CREATE_END(NAME, LOWWERNAME)                          \
  PHP_PROTO_FREE_CLASS_OBJECT(NAME, LOWWERNAME##_free, LOWWERNAME##_handlers); \
  }

#define CACHED_VALUE zval
#define CACHED_TO_ZVAL_PTR(VALUE) (&VALUE)
#define CACHED_PTR_TO_ZVAL_PTR(VALUE) (VALUE)
#define ZVAL_PTR_TO_CACHED_PTR(VALUE) (VALUE)
#define ZVAL_PTR_TO_CACHED_VALUE(VALUE) (*VALUE)
#define ZVAL_TO_CACHED_VALUE(VALUE) (VALUE)

#define CREATE_OBJ_ON_ALLOCATED_ZVAL_PTR(zval_ptr, class_type) \
  ZVAL_OBJ(zval_ptr, class_type->create_object(class_type));

#define PHP_PROTO_SEPARATE_ZVAL_IF_NOT_REF(value) ;

#define PHP_PROTO_GLOBAL_UNINITIALIZED_ZVAL &EG(uninitialized_zval)

#define php_proto_zval_ptr_dtor(zval_ptr) \
  zval_ptr_dtor(zval_ptr)

#define PHP_PROTO_ALLOC_CLASS_OBJECT(class_object, class_type)               \
  class_object* intern;                                                      \
  int size = sizeof(class_object) + zend_object_properties_size(class_type); \
  intern = ecalloc(1, size);                                                 \
  memset(intern, 0, size);

#define PHP_PROTO_FREE_CLASS_OBJECT(class_object, class_object_free, handler) \
  intern->std.handlers = handler;                                             \
  return &intern->std;

#define PHP_PROTO_ALLOC_ARRAY(zval_ptr) \
  ZVAL_NEW_ARR(zval_ptr)

#define UNBOX(class_name, val) \
  (class_name*)((char*)Z_OBJ_P(val) - XtOffsetOf(class_name, std));

#define UNBOX_HASHTABLE_VALUE(class_name, val) \
  (class_name*)((char*)val - XtOffsetOf(class_name, std))

#define HASHTABLE_VALUE_DTOR php_proto_hashtable_descriptor_release

#define PHP_PROTO_HASHTABLE_VALUE zend_object*
#define HASHTABLE_VALUE_CE(val) val->ce

#define CREATE_HASHTABLE_VALUE(OBJ, WRAPPED_OBJ, OBJ_TYPE, OBJ_CLASS_ENTRY) \
  OBJ_TYPE* OBJ;                                                            \
  PHP_PROTO_HASHTABLE_VALUE WRAPPED_OBJ;                                    \
  WRAPPED_OBJ = OBJ_CLASS_ENTRY->create_object(OBJ_CLASS_ENTRY);            \
  OBJ = UNBOX_HASHTABLE_VALUE(OBJ_TYPE, WRAPPED_OBJ);                       \
  --GC_REFCOUNT(WRAPPED_OBJ);

#define PHP_PROTO_CE_DECLARE zend_class_entry*
#define PHP_PROTO_CE_UNREF(ce) (ce)

static inline int php_proto_zend_lookup_class(
    const char* name, int name_length, zend_class_entry** ce TSRMLS_DC) {
  zend_string *zstr_name = zend_string_init(name, name_length, 0);
  *ce = zend_lookup_class(zstr_name);
  zend_string_release(zstr_name);
  return *ce != NULL ? SUCCESS : FAILURE;
}

#define PHP_PROTO_RETVAL_ZVAL(value) ZVAL_COPY(return_value, value)

#endif  // PHP_MAJOR_VERSION >= 7

#if PHP_MAJOR_VERSION < 7 || (PHP_MAJOR_VERSION == 7 && PHP_MINOR_VERSION == 0)
#define PHP_PROTO_FAKE_SCOPE_BEGIN(klass)  \
  zend_class_entry* old_scope = EG(scope); \
  EG(scope) = klass;
#define PHP_PROTO_FAKE_SCOPE_RESTART(klass) \
  old_scope = EG(scope);                    \
  EG(scope) = klass;
#define PHP_PROTO_FAKE_SCOPE_END EG(scope) = old_scope;
#else
#define PHP_PROTO_FAKE_SCOPE_BEGIN(klass)       \
  zend_class_entry* old_scope = EG(fake_scope); \
  EG(fake_scope) = klass;
#define PHP_PROTO_FAKE_SCOPE_RESTART(klass) \
  old_scope = EG(fake_scope);               \
  EG(fake_scope) = klass;
#define PHP_PROTO_FAKE_SCOPE_END EG(fake_scope) = old_scope;
#endif

// Define PHP class
#define DEFINE_PROTOBUF_INIT_CLASS(CLASSNAME, CAMELNAME, LOWERNAME) \
  PHP_PROTO_INIT_CLASS_START(CLASSNAME, CAMELNAME, LOWERNAME)       \
  PHP_PROTO_INIT_CLASS_END

#define DEFINE_PROTOBUF_CREATE(NAME, LOWERNAME)  \
  PHP_PROTO_OBJECT_CREATE_START(NAME, LOWERNAME) \
  LOWERNAME##_init_c_instance(intern TSRMLS_CC); \
  PHP_PROTO_OBJECT_CREATE_END(NAME, LOWERNAME)

#define DEFINE_PROTOBUF_FREE(CAMELNAME, LOWERNAME)  \
  PHP_PROTO_OBJECT_FREE_START(CAMELNAME, LOWERNAME) \
  LOWERNAME##_free_c(intern TSRMLS_CC);             \
  PHP_PROTO_OBJECT_FREE_END

#define DEFINE_PROTOBUF_DTOR(CAMELNAME, LOWERNAME)  \
  PHP_PROTO_OBJECT_DTOR_START(CAMELNAME, LOWERNAME) \
  PHP_PROTO_OBJECT_DTOR_END

#define DEFINE_CLASS(NAME, LOWERNAME, string_name) \
  zend_class_entry *LOWERNAME##_type;              \
  zend_object_handlers *LOWERNAME##_handlers;      \
  DEFINE_PROTOBUF_FREE(NAME, LOWERNAME)            \
  DEFINE_PROTOBUF_DTOR(NAME, LOWERNAME)            \
  DEFINE_PROTOBUF_CREATE(NAME, LOWERNAME)          \
  DEFINE_PROTOBUF_INIT_CLASS(string_name, NAME, LOWERNAME)

// -----------------------------------------------------------------------------
// Forward Declaration
// ----------------------------------------------------------------------------

struct Any;
struct Api;
struct BoolValue;
struct BytesValue;
struct Descriptor;
struct DescriptorPool;
struct DoubleValue;
struct Duration;
struct Enum;
struct EnumDescriptor;
struct EnumValue;
struct EnumValueDescriptor;
struct Field;
struct FieldDescriptor;
struct FieldMask;
struct Field_Cardinality;
struct Field_Kind;
struct FloatValue;
struct GPBEmpty;
struct Int32Value;
struct Int64Value;
struct InternalDescriptorPool;
struct ListValue;
struct Map;
struct MapIter;
struct MessageField;
struct MessageHeader;
struct MessageLayout;
struct Method;
struct Mixin;
struct NullValue;
struct Oneof;
struct Option;
struct RepeatedField;
struct RepeatedFieldIter;
struct SourceContext;
struct StringValue;
struct Struct;
struct Syntax;
struct Timestamp;
struct Type;
struct UInt32Value;
struct UInt64Value;
struct Value;

typedef struct Any Any;
typedef struct Api Api;
typedef struct BoolValue BoolValue;
typedef struct BytesValue BytesValue;
typedef struct Descriptor Descriptor;
typedef struct DescriptorPool DescriptorPool;
typedef struct DoubleValue DoubleValue;
typedef struct Duration Duration;
typedef struct EnumDescriptor EnumDescriptor;
typedef struct Enum Enum;
typedef struct EnumValueDescriptor EnumValueDescriptor;
typedef struct EnumValue EnumValue;
typedef struct Field_Cardinality Field_Cardinality;
typedef struct FieldDescriptor FieldDescriptor;
typedef struct Field Field;
typedef struct Field_Kind Field_Kind;
typedef struct FieldMask FieldMask;
typedef struct FloatValue FloatValue;
typedef struct GPBEmpty GPBEmpty;
typedef struct Int32Value Int32Value;
typedef struct Int64Value Int64Value;
typedef struct InternalDescriptorPool InternalDescriptorPool;
typedef struct ListValue ListValue;
typedef struct MapIter MapIter;
typedef struct Map Map;
typedef struct MessageField MessageField;
typedef struct MessageHeader MessageHeader;
typedef struct MessageLayout MessageLayout;
typedef struct Method Method;
typedef struct Mixin Mixin;
typedef struct NullValue NullValue;
typedef struct Oneof Oneof;
typedef struct Option Option;
typedef struct RepeatedFieldIter RepeatedFieldIter;
typedef struct RepeatedField RepeatedField;
typedef struct SourceContext SourceContext;
typedef struct StringValue StringValue;
typedef struct Struct Struct;
typedef struct Syntax Syntax;
typedef struct Timestamp Timestamp;
typedef struct Type Type;
typedef struct UInt32Value UInt32Value;
typedef struct UInt64Value UInt64Value;
typedef struct Value Value;

// -----------------------------------------------------------------------------
// Globals.
// -----------------------------------------------------------------------------

ZEND_BEGIN_MODULE_GLOBALS(protobuf)
ZEND_END_MODULE_GLOBALS(protobuf)

// Init module and PHP classes.
void any_init(TSRMLS_D);
void api_init(TSRMLS_D);
void bool_value_init(TSRMLS_D);
void bytes_value_init(TSRMLS_D);
void descriptor_init(TSRMLS_D);
void descriptor_pool_init(TSRMLS_D);
void double_value_init(TSRMLS_D);
void duration_init(TSRMLS_D);
void empty_init(TSRMLS_D);
void enum_descriptor_init(TSRMLS_D);
void enum_init(TSRMLS_D);
void enum_value_init(TSRMLS_D);
void field_cardinality_init(TSRMLS_D);
void field_descriptor_init(TSRMLS_D);
void field_init(TSRMLS_D);
void field_kind_init(TSRMLS_D);
void field_mask_init(TSRMLS_D);
void float_value_init(TSRMLS_D);
void gpb_type_init(TSRMLS_D);
void int32_value_init(TSRMLS_D);
void int64_value_init(TSRMLS_D);
void internal_descriptor_pool_init(TSRMLS_D);
void list_value_init(TSRMLS_D);
void map_field_init(TSRMLS_D);
void map_field_iter_init(TSRMLS_D);
void message_init(TSRMLS_D);
void method_init(TSRMLS_D);
void mixin_init(TSRMLS_D);
void null_value_init(TSRMLS_D);
void oneof_descriptor_init(TSRMLS_D);
void option_init(TSRMLS_D);
void repeated_field_init(TSRMLS_D);
void repeated_field_iter_init(TSRMLS_D);
void source_context_init(TSRMLS_D);
void string_value_init(TSRMLS_D);
void struct_init(TSRMLS_D);
void syntax_init(TSRMLS_D);
void timestamp_init(TSRMLS_D);
void type_init(TSRMLS_D);
void uint32_value_init(TSRMLS_D);
void uint64_value_init(TSRMLS_D);
void util_init(TSRMLS_D);
void value_init(TSRMLS_D);

void gpb_metadata_any_init(TSRMLS_D);
void gpb_metadata_api_init(TSRMLS_D);
void gpb_metadata_duration_init(TSRMLS_D);
void gpb_metadata_field_mask_init(TSRMLS_D);
void gpb_metadata_empty_init(TSRMLS_D);
void gpb_metadata_source_context_init(TSRMLS_D);
void gpb_metadata_struct_init(TSRMLS_D);
void gpb_metadata_timestamp_init(TSRMLS_D);
void gpb_metadata_type_init(TSRMLS_D);
void gpb_metadata_wrappers_init(TSRMLS_D);

// Global map from upb {msg,enum}defs to wrapper Descriptor/EnumDescriptor
// instances.
void add_def_obj(const void* def, PHP_PROTO_HASHTABLE_VALUE value);
PHP_PROTO_HASHTABLE_VALUE get_def_obj(const void* def);

// Global map from PHP class entries to wrapper Descriptor/EnumDescriptor
// instances.
void add_ce_obj(const void* ce, PHP_PROTO_HASHTABLE_VALUE value);
PHP_PROTO_HASHTABLE_VALUE get_ce_obj(const void* ce);
bool class_added(const void* ce);

// Global map from message/enum's proto fully-qualified name to corresponding
// wrapper Descriptor/EnumDescriptor instances.
void add_proto_obj(const char* proto, PHP_PROTO_HASHTABLE_VALUE value);
PHP_PROTO_HASHTABLE_VALUE get_proto_obj(const char* proto);

extern zend_class_entry* map_field_type;
extern zend_class_entry* repeated_field_type;

// -----------------------------------------------------------------------------
// Descriptor.
// -----------------------------------------------------------------------------

PHP_PROTO_WRAP_OBJECT_START(DescriptorPool)
  InternalDescriptorPool* intern;
PHP_PROTO_WRAP_OBJECT_END

PHP_METHOD(DescriptorPool, getGeneratedPool);
PHP_METHOD(DescriptorPool, getDescriptorByClassName);
PHP_METHOD(DescriptorPool, getEnumDescriptorByClassName);

PHP_PROTO_WRAP_OBJECT_START(InternalDescriptorPool)
  upb_symtab* symtab;
  HashTable* pending_list;
PHP_PROTO_WRAP_OBJECT_END

PHP_METHOD(InternalDescriptorPool, getGeneratedPool);
PHP_METHOD(InternalDescriptorPool, internalAddGeneratedFile);

void internal_add_generated_file(const char* data, PHP_PROTO_SIZE data_len,
                                 InternalDescriptorPool* pool TSRMLS_DC);
void init_generated_pool_once(TSRMLS_D);

// wrapper of generated pool
#if PHP_MAJOR_VERSION < 7
extern zval* generated_pool_php;
extern zval* internal_generated_pool_php;
void descriptor_pool_free(void* object TSRMLS_DC);
void internal_descriptor_pool_free(void* object TSRMLS_DC);
#else
extern zend_object *generated_pool_php;
extern zend_object *internal_generated_pool_php;
void descriptor_pool_free(zend_object* object);
void internal_descriptor_pool_free(zend_object* object);
#endif
extern InternalDescriptorPool* generated_pool;  // The actual generated pool

PHP_PROTO_WRAP_OBJECT_START(Descriptor)
  const upb_msgdef* msgdef;
  MessageLayout* layout;
  zend_class_entry* klass;  // begins as NULL
  const upb_handlers* fill_handlers;
  const upb_pbdecodermethod* fill_method;
  const upb_json_parsermethod* json_fill_method;
  const upb_handlers* pb_serialize_handlers;
  const upb_handlers* json_serialize_handlers;
  const upb_handlers* json_serialize_handlers_preserve;
PHP_PROTO_WRAP_OBJECT_END

PHP_METHOD(Descriptor, getClass);
PHP_METHOD(Descriptor, getFullName);
PHP_METHOD(Descriptor, getField);
PHP_METHOD(Descriptor, getFieldCount);
PHP_METHOD(Descriptor, getOneofDecl);
PHP_METHOD(Descriptor, getOneofDeclCount);

extern zend_class_entry* descriptor_type;

void descriptor_name_set(Descriptor *desc, const char *name);

PHP_PROTO_WRAP_OBJECT_START(FieldDescriptor)
  const upb_fielddef* fielddef;
PHP_PROTO_WRAP_OBJECT_END

PHP_METHOD(FieldDescriptor, getName);
PHP_METHOD(FieldDescriptor, getNumber);
PHP_METHOD(FieldDescriptor, getLabel);
PHP_METHOD(FieldDescriptor, getType);
PHP_METHOD(FieldDescriptor, isMap);
PHP_METHOD(FieldDescriptor, getEnumType);
PHP_METHOD(FieldDescriptor, getMessageType);

extern zend_class_entry* field_descriptor_type;

PHP_PROTO_WRAP_OBJECT_START(EnumDescriptor)
  const upb_enumdef* enumdef;
  zend_class_entry* klass;  // begins as NULL
PHP_PROTO_WRAP_OBJECT_END

PHP_METHOD(EnumDescriptor, getValue);
PHP_METHOD(EnumDescriptor, getValueCount);

extern zend_class_entry* enum_descriptor_type;

PHP_PROTO_WRAP_OBJECT_START(EnumValueDescriptor)
  const char* name;
  int32_t number;
PHP_PROTO_WRAP_OBJECT_END

PHP_METHOD(EnumValueDescriptor, getName);
PHP_METHOD(EnumValueDescriptor, getNumber);

extern zend_class_entry* enum_value_descriptor_type;

// -----------------------------------------------------------------------------
// Message class creation.
// -----------------------------------------------------------------------------

void* message_data(MessageHeader* msg);
void custom_data_init(const zend_class_entry* ce,
                      MessageHeader* msg PHP_PROTO_TSRMLS_DC);

// Build PHP class for given descriptor. Instead of building from scratch, this
// function modifies existing class which has been partially defined in PHP
// code.
void build_class_from_descriptor(
    PHP_PROTO_HASHTABLE_VALUE php_descriptor TSRMLS_DC);

extern zend_class_entry* message_type;
extern zend_object_handlers* message_handlers;

// -----------------------------------------------------------------------------
// Message layout / storage.
// -----------------------------------------------------------------------------

/*
 * In c extension, each protobuf message is a zval instance. The zval instance
 * is like union, which can be used to store int, string, zend_object_value and
 * etc. For protobuf message, the zval instance is used to store the
 * zend_object_value.
 *
 * The zend_object_value is composed of handlers and a handle to look up the
 * actual stored data. The handlers are pointers to functions, e.g., read,
 * write, and etc, to access properties.
 *
 * The actual data of protobuf messages is stored as MessageHeader in zend
 * engine's central repository. Each MessageHeader instance is composed of a
 * zend_object, a Descriptor instance and the real message data.
 *
 * For the reason that PHP's native types may not be large enough to store
 * protobuf message's field (e.g., int64), all message's data is stored in
 * custom memory layout and is indexed by the Descriptor instance.
 *
 * The zend_object contains the zend class entry and the properties table. The
 * zend class entry contains all information about protobuf message's
 * corresponding PHP class. The most useful information is the offset table of
 * properties. Because read access to properties requires returning zval
 * instance, we need to convert data from the custom layout to zval instance.
 * Instead of creating zval instance for every read access, we use the zval
 * instances in the properties table in the zend_object as cache.  When
 * accessing properties, the offset is needed to find the zval property in
 * zend_object's properties table. These properties will be updated using the
 * data from custom memory layout only when reading these properties.
 *
 * zval
 * |-zend_object_value obj
 *   |-zend_object_handlers* handlers -> |-read_property_handler
 *   |                                   |-write_property_handler
 *   |                              ++++++++++++++++++++++
 *   |-zend_object_handle handle -> + central repository +
 *                                  ++++++++++++++++++++++
 *  MessageHeader <-----------------|
 *  |-zend_object std
 *  | |-class_entry* ce -> class_entry
 *  | |                    |-HashTable properties_table (name->offset)
 *  | |-zval** properties_table <------------------------------|
 *  |                         |------> zval* property(cache)
 *  |-Descriptor* desc (name->offset)
 *  |-void** data <-----------|
 *           |-----------------------> void* property(data)
 *
 */

#define MESSAGE_FIELD_NO_CASE ((size_t)-1)

struct MessageField {
  size_t offset;
  int cache_index;  // Each field except oneof field has a zval cache to avoid
                    // multiple creation when being accessed.
  size_t case_offset;   // for oneofs, a uint32. Else, MESSAGE_FIELD_NO_CASE.
};

struct MessageLayout {
  const upb_msgdef* msgdef;
  MessageField* fields;
  size_t size;
};

PHP_PROTO_WRAP_OBJECT_START(MessageHeader)
  void* data;  // Point to the real message data.
               // Place needs to be consistent with map_parse_frame_data_t.
  Descriptor* descriptor;  // Kept alive by self.class.descriptor reference.
PHP_PROTO_WRAP_OBJECT_END

MessageLayout* create_layout(const upb_msgdef* msgdef);
void layout_init(MessageLayout* layout, void* storage,
                 zend_object* object PHP_PROTO_TSRMLS_DC);
zval* layout_get(MessageLayout* layout, const void* storage,
                 const upb_fielddef* field, CACHED_VALUE* cache TSRMLS_DC);
void layout_set(MessageLayout* layout, MessageHeader* header,
                const upb_fielddef* field, zval* val TSRMLS_DC);
void layout_merge(MessageLayout* layout, MessageHeader* from,
                  MessageHeader* to TSRMLS_DC);
const char* layout_get_oneof_case(MessageLayout* layout, const void* storage,
                                  const upb_oneofdef* oneof TSRMLS_DC);
void free_layout(MessageLayout* layout);
void* slot_memory(MessageLayout* layout, const void* storage,
                  const upb_fielddef* field);

PHP_METHOD(Message, clear);
PHP_METHOD(Message, mergeFrom);
PHP_METHOD(Message, readOneof);
PHP_METHOD(Message, writeOneof);
PHP_METHOD(Message, whichOneof);
PHP_METHOD(Message, __construct);

// -----------------------------------------------------------------------------
// Encode / Decode.
// -----------------------------------------------------------------------------

// Maximum depth allowed during encoding, to avoid stack overflows due to
// cycles.
#define ENCODE_MAX_NESTING 63

// Constructs the upb decoder method for parsing messages of this type.
// This is called from the message class creation code.
const upb_pbdecodermethod *new_fillmsg_decodermethod(Descriptor *desc,
                                                     const void *owner);
void serialize_to_string(zval* val, zval* return_value TSRMLS_DC);
void merge_from_string(const char* data, int data_len, const Descriptor* desc,
                       MessageHeader* msg);

PHP_METHOD(Message, serializeToString);
PHP_METHOD(Message, mergeFromString);
PHP_METHOD(Message, serializeToJsonString);
PHP_METHOD(Message, mergeFromJsonString);
PHP_METHOD(Message, discardUnknownFields);

// -----------------------------------------------------------------------------
// Type check / conversion.
// -----------------------------------------------------------------------------

bool protobuf_convert_to_int32(zval* from, int32_t* to);
bool protobuf_convert_to_uint32(zval* from, uint32_t* to);
bool protobuf_convert_to_int64(zval* from, int64_t* to);
bool protobuf_convert_to_uint64(zval* from, uint64_t* to);
bool protobuf_convert_to_float(zval* from, float* to);
bool protobuf_convert_to_double(zval* from, double* to);
bool protobuf_convert_to_bool(zval* from, int8_t* to);
bool protobuf_convert_to_string(zval* from);

void check_repeated_field(const zend_class_entry* klass, PHP_PROTO_LONG type,
                          zval* val, zval* return_value);
void check_map_field(const zend_class_entry* klass, PHP_PROTO_LONG key_type,
                     PHP_PROTO_LONG value_type, zval* val, zval* return_value);

PHP_METHOD(Util, checkInt32);
PHP_METHOD(Util, checkUint32);
PHP_METHOD(Util, checkInt64);
PHP_METHOD(Util, checkUint64);
PHP_METHOD(Util, checkEnum);
PHP_METHOD(Util, checkFloat);
PHP_METHOD(Util, checkDouble);
PHP_METHOD(Util, checkBool);
PHP_METHOD(Util, checkString);
PHP_METHOD(Util, checkBytes);
PHP_METHOD(Util, checkMessage);
PHP_METHOD(Util, checkMapField);
PHP_METHOD(Util, checkRepeatedField);

// -----------------------------------------------------------------------------
// Native slot storage abstraction.
// -----------------------------------------------------------------------------

#define NATIVE_SLOT_MAX_SIZE sizeof(uint64_t)

size_t native_slot_size(upb_fieldtype_t type);
bool native_slot_set(upb_fieldtype_t type, const zend_class_entry* klass,
                     void* memory, zval* value TSRMLS_DC);
// String/Message is stored differently in array/map from normal message fields.
// So we need to make a special method to handle that.
bool native_slot_set_by_array(upb_fieldtype_t type,
                              const zend_class_entry* klass, void* memory,
                              zval* value TSRMLS_DC);
bool native_slot_set_by_map(upb_fieldtype_t type, const zend_class_entry* klass,
                            void* memory, zval* value TSRMLS_DC);
void native_slot_init(upb_fieldtype_t type, void* memory, CACHED_VALUE* cache);
// For each property, in order to avoid conversion between the zval object and
// the actual data type during parsing/serialization, the containing message
// object use the custom memory layout to store the actual data type for each
// property inside of it.  To access a property from php code, the property
// needs to be converted to a zval object. The message object is not responsible
// for providing such a zval object. Instead the caller needs to provide one
// (cache) and update it with the actual data (memory).
void native_slot_get(upb_fieldtype_t type, const void* memory,
                     CACHED_VALUE* cache TSRMLS_DC);
// String/Message is stored differently in array/map from normal message fields.
// So we need to make a special method to handle that.
void native_slot_get_by_array(upb_fieldtype_t type, const void* memory,
                     CACHED_VALUE* cache TSRMLS_DC);
void native_slot_get_by_map_key(upb_fieldtype_t type, const void* memory,
                                int length, CACHED_VALUE* cache TSRMLS_DC);
void native_slot_get_by_map_value(upb_fieldtype_t type, const void* memory,
                                  CACHED_VALUE* cache TSRMLS_DC);
void native_slot_get_default(upb_fieldtype_t type,
                             CACHED_VALUE* cache TSRMLS_DC);

// -----------------------------------------------------------------------------
// Map Field.
// -----------------------------------------------------------------------------

extern zend_object_handlers* map_field_handlers;
extern zend_object_handlers* map_field_iter_handlers;

PHP_PROTO_WRAP_OBJECT_START(Map)
  upb_fieldtype_t key_type;
  upb_fieldtype_t value_type;
  const zend_class_entry* msg_ce;  // class entry for value message
  upb_strtable table;
PHP_PROTO_WRAP_OBJECT_END

PHP_PROTO_WRAP_OBJECT_START(MapIter)
  Map* self;
  upb_strtable_iter it;
PHP_PROTO_WRAP_OBJECT_END

void map_begin(zval* self, MapIter* iter TSRMLS_DC);
void map_next(MapIter* iter);
bool map_done(MapIter* iter);
const char* map_iter_key(MapIter* iter, int* len);
upb_value map_iter_value(MapIter* iter, int* len);

// These operate on a map-entry msgdef.
const upb_fielddef* map_entry_key(const upb_msgdef* msgdef);
const upb_fielddef* map_entry_value(const upb_msgdef* msgdef);

void map_field_create_with_field(const zend_class_entry* ce,
                                 const upb_fielddef* field,
                                 CACHED_VALUE* map_field PHP_PROTO_TSRMLS_DC);
void map_field_create_with_type(const zend_class_entry* ce,
                                upb_fieldtype_t key_type,
                                upb_fieldtype_t value_type,
                                const zend_class_entry* msg_ce,
                                CACHED_VALUE* map_field PHP_PROTO_TSRMLS_DC);
void* upb_value_memory(upb_value* v);

#define MAP_KEY_FIELD 1
#define MAP_VALUE_FIELD 2

// These operate on a map field (i.e., a repeated field of submessages whose
// submessage type is a map-entry msgdef).
bool is_map_field(const upb_fielddef* field);
const upb_fielddef* map_field_key(const upb_fielddef* field);
const upb_fielddef* map_field_value(const upb_fielddef* field);

bool map_index_set(Map *intern, const char* keyval, int length, upb_value v);

PHP_METHOD(MapField, __construct);
PHP_METHOD(MapField, offsetExists);
PHP_METHOD(MapField, offsetGet);
PHP_METHOD(MapField, offsetSet);
PHP_METHOD(MapField, offsetUnset);
PHP_METHOD(MapField, count);
PHP_METHOD(MapField, getIterator);

PHP_METHOD(MapFieldIter, rewind);
PHP_METHOD(MapFieldIter, current);
PHP_METHOD(MapFieldIter, key);
PHP_METHOD(MapFieldIter, next);
PHP_METHOD(MapFieldIter, valid);

// -----------------------------------------------------------------------------
// Repeated Field.
// -----------------------------------------------------------------------------

extern zend_object_handlers* repeated_field_handlers;
extern zend_object_handlers* repeated_field_iter_handlers;

PHP_PROTO_WRAP_OBJECT_START(RepeatedField)
#if PHP_MAJOR_VERSION < 7
  zval* array;
#else
  zval array;
#endif
  upb_fieldtype_t type;
  const zend_class_entry* msg_ce;  // class entry for containing message
                                   // (for message field only).
PHP_PROTO_WRAP_OBJECT_END

PHP_PROTO_WRAP_OBJECT_START(RepeatedFieldIter)
  RepeatedField* repeated_field;
  long position;
PHP_PROTO_WRAP_OBJECT_END

void repeated_field_create_with_field(
    zend_class_entry* ce, const upb_fielddef* field,
    CACHED_VALUE* repeated_field PHP_PROTO_TSRMLS_DC);
void repeated_field_create_with_type(
    zend_class_entry* ce, upb_fieldtype_t type, const zend_class_entry* msg_ce,
    CACHED_VALUE* repeated_field PHP_PROTO_TSRMLS_DC);
// Return the element at the index position from the repeated field. There is
// not restriction on the type of stored elements.
void *repeated_field_index_native(RepeatedField *intern, int index TSRMLS_DC);
// Add the element to the end of the repeated field. There is not restriction on
// the type of stored elements.
void repeated_field_push_native(RepeatedField *intern, void *value);

PHP_METHOD(RepeatedField, __construct);
PHP_METHOD(RepeatedField, append);
PHP_METHOD(RepeatedField, offsetExists);
PHP_METHOD(RepeatedField, offsetGet);
PHP_METHOD(RepeatedField, offsetSet);
PHP_METHOD(RepeatedField, offsetUnset);
PHP_METHOD(RepeatedField, count);
PHP_METHOD(RepeatedField, getIterator);

PHP_METHOD(RepeatedFieldIter, rewind);
PHP_METHOD(RepeatedFieldIter, current);
PHP_METHOD(RepeatedFieldIter, key);
PHP_METHOD(RepeatedFieldIter, next);
PHP_METHOD(RepeatedFieldIter, valid);

// -----------------------------------------------------------------------------
// Oneof Field.
// -----------------------------------------------------------------------------

PHP_PROTO_WRAP_OBJECT_START(Oneof)
  upb_oneofdef* oneofdef;
  int index;    // Index of field in oneof. -1 if not set.
  char value[NATIVE_SLOT_MAX_SIZE];
PHP_PROTO_WRAP_OBJECT_END

PHP_METHOD(Oneof, getName);
PHP_METHOD(Oneof, getField);
PHP_METHOD(Oneof, getFieldCount);

extern zend_class_entry* oneof_descriptor_type;

// Oneof case slot value to indicate that no oneof case is set. The value `0` is
// safe because field numbers are used as case identifiers, and no field can
// have a number of 0.
#define ONEOF_CASE_NONE 0

// -----------------------------------------------------------------------------
// Well Known Type.
// -----------------------------------------------------------------------------

extern bool is_inited_file_any;
extern bool is_inited_file_api;
extern bool is_inited_file_duration;
extern bool is_inited_file_field_mask;
extern bool is_inited_file_empty;
extern bool is_inited_file_source_context;
extern bool is_inited_file_struct;
extern bool is_inited_file_timestamp;
extern bool is_inited_file_type;
extern bool is_inited_file_wrappers;

PHP_METHOD(GPBMetadata_Any, initOnce);
PHP_METHOD(GPBMetadata_Api, initOnce);
PHP_METHOD(GPBMetadata_Duration, initOnce);
PHP_METHOD(GPBMetadata_FieldMask, initOnce);
PHP_METHOD(GPBMetadata_Empty, initOnce);
PHP_METHOD(GPBMetadata_SourceContext, initOnce);
PHP_METHOD(GPBMetadata_Struct, initOnce);
PHP_METHOD(GPBMetadata_Timestamp, initOnce);
PHP_METHOD(GPBMetadata_Type, initOnce);
PHP_METHOD(GPBMetadata_Wrappers, initOnce);

PHP_METHOD(Any, __construct);
PHP_METHOD(Any, getTypeUrl);
PHP_METHOD(Any, setTypeUrl);
PHP_METHOD(Any, getValue);
PHP_METHOD(Any, setValue);
PHP_METHOD(Any, unpack);
PHP_METHOD(Any, pack);
PHP_METHOD(Any, is);

PHP_METHOD(Duration, __construct);
PHP_METHOD(Duration, getSeconds);
PHP_METHOD(Duration, setSeconds);
PHP_METHOD(Duration, getNanos);
PHP_METHOD(Duration, setNanos);

PHP_METHOD(Timestamp, __construct);
PHP_METHOD(Timestamp, fromDateTime);
PHP_METHOD(Timestamp, toDateTime);
PHP_METHOD(Timestamp, getSeconds);
PHP_METHOD(Timestamp, setSeconds);
PHP_METHOD(Timestamp, getNanos);
PHP_METHOD(Timestamp, setNanos);

PHP_METHOD(Api, __construct);
PHP_METHOD(Api, getName);
PHP_METHOD(Api, setName);
PHP_METHOD(Api, getMethods);
PHP_METHOD(Api, setMethods);
PHP_METHOD(Api, getOptions);
PHP_METHOD(Api, setOptions);
PHP_METHOD(Api, getVersion);
PHP_METHOD(Api, setVersion);
PHP_METHOD(Api, getSourceContext);
PHP_METHOD(Api, setSourceContext);
PHP_METHOD(Api, getMixins);
PHP_METHOD(Api, setMixins);
PHP_METHOD(Api, getSyntax);
PHP_METHOD(Api, setSyntax);

PHP_METHOD(BoolValue, __construct);
PHP_METHOD(BoolValue, getValue);
PHP_METHOD(BoolValue, setValue);

PHP_METHOD(BytesValue, __construct);
PHP_METHOD(BytesValue, getValue);
PHP_METHOD(BytesValue, setValue);

PHP_METHOD(DoubleValue, __construct);
PHP_METHOD(DoubleValue, getValue);
PHP_METHOD(DoubleValue, setValue);

PHP_METHOD(Enum, __construct);
PHP_METHOD(Enum, getName);
PHP_METHOD(Enum, setName);
PHP_METHOD(Enum, getEnumvalue);
PHP_METHOD(Enum, setEnumvalue);
PHP_METHOD(Enum, getOptions);
PHP_METHOD(Enum, setOptions);
PHP_METHOD(Enum, getSourceContext);
PHP_METHOD(Enum, setSourceContext);
PHP_METHOD(Enum, getSyntax);
PHP_METHOD(Enum, setSyntax);

PHP_METHOD(EnumValue, __construct);
PHP_METHOD(EnumValue, getName);
PHP_METHOD(EnumValue, setName);
PHP_METHOD(EnumValue, getNumber);
PHP_METHOD(EnumValue, setNumber);
PHP_METHOD(EnumValue, getOptions);
PHP_METHOD(EnumValue, setOptions);

PHP_METHOD(FieldMask, __construct);
PHP_METHOD(FieldMask, getPaths);
PHP_METHOD(FieldMask, setPaths);

PHP_METHOD(Field, __construct);
PHP_METHOD(Field, getKind);
PHP_METHOD(Field, setKind);
PHP_METHOD(Field, getCardinality);
PHP_METHOD(Field, setCardinality);
PHP_METHOD(Field, getNumber);
PHP_METHOD(Field, setNumber);
PHP_METHOD(Field, getName);
PHP_METHOD(Field, setName);
PHP_METHOD(Field, getTypeUrl);
PHP_METHOD(Field, setTypeUrl);
PHP_METHOD(Field, getOneofIndex);
PHP_METHOD(Field, setOneofIndex);
PHP_METHOD(Field, getPacked);
PHP_METHOD(Field, setPacked);
PHP_METHOD(Field, getOptions);
PHP_METHOD(Field, setOptions);
PHP_METHOD(Field, getJsonName);
PHP_METHOD(Field, setJsonName);
PHP_METHOD(Field, getDefaultValue);
PHP_METHOD(Field, setDefaultValue);

PHP_METHOD(FloatValue, __construct);
PHP_METHOD(FloatValue, getValue);
PHP_METHOD(FloatValue, setValue);

PHP_METHOD(GPBEmpty, __construct);

PHP_METHOD(Int32Value, __construct);
PHP_METHOD(Int32Value, getValue);
PHP_METHOD(Int32Value, setValue);

PHP_METHOD(Int64Value, __construct);
PHP_METHOD(Int64Value, getValue);
PHP_METHOD(Int64Value, setValue);

PHP_METHOD(ListValue, __construct);
PHP_METHOD(ListValue, getValues);
PHP_METHOD(ListValue, setValues);

PHP_METHOD(Method, __construct);
PHP_METHOD(Method, getName);
PHP_METHOD(Method, setName);
PHP_METHOD(Method, getRequestTypeUrl);
PHP_METHOD(Method, setRequestTypeUrl);
PHP_METHOD(Method, getRequestStreaming);
PHP_METHOD(Method, setRequestStreaming);
PHP_METHOD(Method, getResponseTypeUrl);
PHP_METHOD(Method, setResponseTypeUrl);
PHP_METHOD(Method, getResponseStreaming);
PHP_METHOD(Method, setResponseStreaming);
PHP_METHOD(Method, getOptions);
PHP_METHOD(Method, setOptions);
PHP_METHOD(Method, getSyntax);
PHP_METHOD(Method, setSyntax);

PHP_METHOD(Mixin, __construct);
PHP_METHOD(Mixin, getName);
PHP_METHOD(Mixin, setName);
PHP_METHOD(Mixin, getRoot);
PHP_METHOD(Mixin, setRoot);

PHP_METHOD(Option, __construct);
PHP_METHOD(Option, getName);
PHP_METHOD(Option, setName);
PHP_METHOD(Option, getValue);
PHP_METHOD(Option, setValue);

PHP_METHOD(SourceContext, __construct);
PHP_METHOD(SourceContext, getFileName);
PHP_METHOD(SourceContext, setFileName);

PHP_METHOD(StringValue, __construct);
PHP_METHOD(StringValue, getValue);
PHP_METHOD(StringValue, setValue);

PHP_METHOD(Struct, __construct);
PHP_METHOD(Struct, getFields);
PHP_METHOD(Struct, setFields);

PHP_METHOD(Type, __construct);
PHP_METHOD(Type, getName);
PHP_METHOD(Type, setName);
PHP_METHOD(Type, getFields);
PHP_METHOD(Type, setFields);
PHP_METHOD(Type, getOneofs);
PHP_METHOD(Type, setOneofs);
PHP_METHOD(Type, getOptions);
PHP_METHOD(Type, setOptions);
PHP_METHOD(Type, getSourceContext);
PHP_METHOD(Type, setSourceContext);
PHP_METHOD(Type, getSyntax);
PHP_METHOD(Type, setSyntax);

PHP_METHOD(UInt32Value, __construct);
PHP_METHOD(UInt32Value, getValue);
PHP_METHOD(UInt32Value, setValue);

PHP_METHOD(UInt64Value, __construct);
PHP_METHOD(UInt64Value, getValue);
PHP_METHOD(UInt64Value, setValue);

PHP_METHOD(Value, __construct);
PHP_METHOD(Value, getNullValue);
PHP_METHOD(Value, setNullValue);
PHP_METHOD(Value, getNumberValue);
PHP_METHOD(Value, setNumberValue);
PHP_METHOD(Value, getStringValue);
PHP_METHOD(Value, setStringValue);
PHP_METHOD(Value, getBoolValue);
PHP_METHOD(Value, setBoolValue);
PHP_METHOD(Value, getStructValue);
PHP_METHOD(Value, setStructValue);
PHP_METHOD(Value, getListValue);
PHP_METHOD(Value, setListValue);
PHP_METHOD(Value, getKind);

extern zend_class_entry* any_type;
extern zend_class_entry* api_type;
extern zend_class_entry* bool_value_type;
extern zend_class_entry* bytes_value_type;
extern zend_class_entry* double_value_type;
extern zend_class_entry* duration_type;
extern zend_class_entry* empty_type;
extern zend_class_entry* enum_type;
extern zend_class_entry* enum_value_type;
extern zend_class_entry* field_cardinality_type;
extern zend_class_entry* field_kind_type;
extern zend_class_entry* field_mask_type;
extern zend_class_entry* field_type;
extern zend_class_entry* float_value_type;
extern zend_class_entry* int32_value_type;
extern zend_class_entry* int64_value_type;
extern zend_class_entry* list_value_type;
extern zend_class_entry* method_type;
extern zend_class_entry* mixin_type;
extern zend_class_entry* null_value_type;
extern zend_class_entry* option_type;
extern zend_class_entry* source_context_type;
extern zend_class_entry* string_value_type;
extern zend_class_entry* struct_type;
extern zend_class_entry* syntax_type;
extern zend_class_entry* timestamp_type;
extern zend_class_entry* type_type;
extern zend_class_entry* uint32_value_type;
extern zend_class_entry* uint64_value_type;
extern zend_class_entry* value_type;

// -----------------------------------------------------------------------------
// Upb.
// -----------------------------------------------------------------------------

upb_fieldtype_t to_fieldtype(upb_descriptortype_t type);
const zend_class_entry* field_type_class(
    const upb_fielddef* field PHP_PROTO_TSRMLS_DC);

// -----------------------------------------------------------------------------
// Utilities.
// -----------------------------------------------------------------------------

// Memory management
#define ALLOC(class_name) (class_name*) emalloc(sizeof(class_name))
#define PEMALLOC(class_name) (class_name*) pemalloc(sizeof(class_name), 1)
#define ALLOC_N(class_name, n) (class_name*) emalloc(sizeof(class_name) * n)
#define FREE(object) efree(object)
#define PEFREE(object) pefree(object, 1)

// String argument.
#define STR(str) (str), strlen(str)

// Zend Value
#if PHP_MAJOR_VERSION < 7
#define Z_OBJ_P(zval_p)                                       \
  ((zend_object*)(EG(objects_store)                           \
                      .object_buckets[Z_OBJ_HANDLE_P(zval_p)] \
                      .bucket.obj.object))
#endif

// Message handler
static inline zval* php_proto_message_read_property(
    zval* msg, zval* member PHP_PROTO_TSRMLS_DC) {
#if PHP_MAJOR_VERSION < 7
  return message_handlers->read_property(msg, member, BP_VAR_R,
                                         NULL PHP_PROTO_TSRMLS_CC);
#else
  return message_handlers->read_property(msg, member, BP_VAR_R, NULL,
                                         NULL PHP_PROTO_TSRMLS_CC);
#endif
}

// Reserved name
bool is_reserved_name(const char* name);
bool is_valid_constant_name(const char* name);

#endif  // __GOOGLE_PROTOBUF_PHP_PROTOBUF_H__
