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

#include "protobuf.h"

#include <zend_hash.h>

ZEND_DECLARE_MODULE_GLOBALS(protobuf)
static PHP_GINIT_FUNCTION(protobuf);
static PHP_GSHUTDOWN_FUNCTION(protobuf);
static PHP_RINIT_FUNCTION(protobuf);
static PHP_RSHUTDOWN_FUNCTION(protobuf);
static PHP_MINIT_FUNCTION(protobuf);
static PHP_MSHUTDOWN_FUNCTION(protobuf);

// Global map from upb {msg,enum}defs to wrapper Descriptor/EnumDescriptor
// instances.
static HashTable* upb_def_to_php_obj_map;
// Global map from message/enum's php class entry to corresponding wrapper
// Descriptor/EnumDescriptor instances.
static HashTable* ce_to_php_obj_map;

// -----------------------------------------------------------------------------
// Global maps.
// -----------------------------------------------------------------------------

static void add_to_table(HashTable* t, const void* def, void* value) {
  uint nIndex = (ulong)def & t->nTableMask;

  zval* pDest = NULL;
  php_proto_zend_hash_index_update(t, (zend_ulong)def, &value, sizeof(zval*),
                                   (void**)&pDest);
}

static void* get_from_table(const HashTable* t, const void* def) {
  void** value;
  if (php_proto_zend_hash_index_find(t, (zend_ulong)def, (void**)&value) ==
      FAILURE) {
    zend_error(E_ERROR, "PHP object not found for given definition.\n");
    return NULL;
  }
  return *value;
}

static bool exist_in_table(const HashTable* t, const void* def) {
  void** value;
  return (php_proto_zend_hash_index_find(t, (zend_ulong)def, (void**)&value) ==
          SUCCESS);
}

static void add_to_list(HashTable* t, void* value) {
  zval* pDest = NULL;
  php_proto_zend_hash_next_index_insert(t, &value, sizeof(void*),
                                        (void**)&pDest);
}

void add_def_obj(const void* def, PHP_PROTO_HASHTABLE_VALUE value) {
#if PHP_MAJOR_VERSION < 7
  Z_ADDREF_P(value);
#else
  ++GC_REFCOUNT(value);
#endif
  add_to_table(upb_def_to_php_obj_map, def, value);
}

PHP_PROTO_HASHTABLE_VALUE get_def_obj(const void* def) {
  return (PHP_PROTO_HASHTABLE_VALUE)get_from_table(upb_def_to_php_obj_map, def);
}

void add_ce_obj(const void* ce, PHP_PROTO_HASHTABLE_VALUE value) {
#if PHP_MAJOR_VERSION < 7
  Z_ADDREF_P(value);
#else
  ++GC_REFCOUNT(value);
#endif
  add_to_table(ce_to_php_obj_map, ce, value);
}

PHP_PROTO_HASHTABLE_VALUE get_ce_obj(const void* ce) {
  return (PHP_PROTO_HASHTABLE_VALUE)get_from_table(ce_to_php_obj_map, ce);
}

bool class_added(const void* ce) {
  return exist_in_table(ce_to_php_obj_map, ce);
}

// -----------------------------------------------------------------------------
// Utilities.
// -----------------------------------------------------------------------------

zend_function_entry protobuf_functions[] = {
  ZEND_FE_END
};

zend_module_entry protobuf_module_entry = {
  STANDARD_MODULE_HEADER,
  PHP_PROTOBUF_EXTNAME,     // extension name
  protobuf_functions,       // function list
  PHP_MINIT(protobuf),      // process startup
  PHP_MSHUTDOWN(protobuf),  // process shutdown
  PHP_RINIT(protobuf),      // request shutdown
  PHP_RSHUTDOWN(protobuf),  // request shutdown
  NULL,                 // extension info
  PHP_PROTOBUF_VERSION, // extension version
  PHP_MODULE_GLOBALS(protobuf),  // globals descriptor
  PHP_GINIT(protobuf),  // globals ctor
  PHP_GSHUTDOWN(protobuf),  // globals dtor
  NULL,  // post deactivate
  STANDARD_MODULE_PROPERTIES_EX
};

// install module
ZEND_GET_MODULE(protobuf)

// global variables
static PHP_GINIT_FUNCTION(protobuf) {
}

static PHP_GSHUTDOWN_FUNCTION(protobuf) {
}

#if PHP_MAJOR_VERSION >= 7
static void php_proto_hashtable_descriptor_release(zval* value) {
  void* ptr = Z_PTR_P(value);
  zend_object* object = *(zend_object**)ptr;
  if(--GC_REFCOUNT(object) == 0) {
    zend_objects_store_del(object);
  }
  efree(ptr);
}
#endif

static PHP_RINIT_FUNCTION(protobuf) {
  ALLOC_HASHTABLE(upb_def_to_php_obj_map);
  zend_hash_init(upb_def_to_php_obj_map, 16, NULL, HASHTABLE_VALUE_DTOR, 0);

  ALLOC_HASHTABLE(ce_to_php_obj_map);
  zend_hash_init(ce_to_php_obj_map, 16, NULL, HASHTABLE_VALUE_DTOR, 0);

  generated_pool = NULL;
  generated_pool_php = NULL;

  return 0;
}

static PHP_RSHUTDOWN_FUNCTION(protobuf) {
  zend_hash_destroy(upb_def_to_php_obj_map);
  FREE_HASHTABLE(upb_def_to_php_obj_map);

  zend_hash_destroy(ce_to_php_obj_map);
  FREE_HASHTABLE(ce_to_php_obj_map);

#if PHP_MAJOR_VERSION < 7
  if (generated_pool_php != NULL) {
    zval_dtor(generated_pool_php);
    FREE_ZVAL(generated_pool_php);
  }
#endif

  return 0;
}

static PHP_MINIT_FUNCTION(protobuf) {
  map_field_init(TSRMLS_C);
  map_field_iter_init(TSRMLS_C);
  repeated_field_init(TSRMLS_C);
  repeated_field_iter_init(TSRMLS_C);
  gpb_type_init(TSRMLS_C);
  message_init(TSRMLS_C);
  descriptor_pool_init(TSRMLS_C);
  descriptor_init(TSRMLS_C);
  enum_descriptor_init(TSRMLS_C);
  util_init(TSRMLS_C);

  return 0;
}

static PHP_MSHUTDOWN_FUNCTION(protobuf) {
  PEFREE(message_handlers);
  PEFREE(repeated_field_handlers);
  PEFREE(repeated_field_iter_handlers);
  PEFREE(map_field_handlers);
  PEFREE(map_field_iter_handlers);

  return 0;
}
