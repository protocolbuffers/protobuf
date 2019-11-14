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
// Global map from message/enum's proto fully-qualified name to corresponding
// wrapper Descriptor/EnumDescriptor instances.
static HashTable* proto_to_php_obj_map;
static HashTable* reserved_names;

// -----------------------------------------------------------------------------
// Global maps.
// -----------------------------------------------------------------------------

static void add_to_table(HashTable* t, const void* def, void* value) {
  uint nIndex = (ulong)def & t->nTableMask;

  zval* pDest = NULL;
  php_proto_zend_hash_index_update_mem(t, (zend_ulong)def, &value,
                                       sizeof(zval*), (void**)&pDest);
}

static void* get_from_table(const HashTable* t, const void* def) {
  void** value;
  if (php_proto_zend_hash_index_find_mem(t, (zend_ulong)def, (void**)&value) ==
      FAILURE) {
    return NULL;
  }
  return *value;
}

static bool exist_in_table(const HashTable* t, const void* def) {
  void** value;
  return (php_proto_zend_hash_index_find_mem(t, (zend_ulong)def,
                                             (void**)&value) == SUCCESS);
}

static void add_to_list(HashTable* t, void* value) {
  zval* pDest = NULL;
  php_proto_zend_hash_next_index_insert_mem(t, &value, sizeof(void*),
                                        (void**)&pDest);
}

static void add_to_strtable(HashTable* t, const char* key, int key_size,
                            void* value) {
  zval* pDest = NULL;
  php_proto_zend_hash_update_mem(t, key, key_size, &value, sizeof(void*),
                                 (void**)&pDest);
}

static void* get_from_strtable(const HashTable* t, const char* key, int key_size) {
  void** value;
  if (php_proto_zend_hash_find_mem(t, key, key_size, (void**)&value) ==
      FAILURE) {
    return NULL;
  }
  return *value;
}

void add_def_obj(const void* def, PHP_PROTO_HASHTABLE_VALUE value) {
#if PHP_MAJOR_VERSION < 7
  Z_ADDREF_P(value);
#else
  GC_ADDREF(value);
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
  GC_ADDREF(value);
#endif
  add_to_table(ce_to_php_obj_map, ce, value);
}

PHP_PROTO_HASHTABLE_VALUE get_ce_obj(const void* ce) {
  return (PHP_PROTO_HASHTABLE_VALUE)get_from_table(ce_to_php_obj_map, ce);
}

bool class_added(const void* ce) {
  return exist_in_table(ce_to_php_obj_map, ce);
}

void add_proto_obj(const char* proto, PHP_PROTO_HASHTABLE_VALUE value) {
#if PHP_MAJOR_VERSION < 7
  Z_ADDREF_P(value);
#else
  GC_ADDREF(value);
#endif
  add_to_strtable(proto_to_php_obj_map, proto, strlen(proto), value);
}

PHP_PROTO_HASHTABLE_VALUE get_proto_obj(const char* proto) {
  return (PHP_PROTO_HASHTABLE_VALUE)get_from_strtable(proto_to_php_obj_map,
                                                      proto, strlen(proto));
}

// -----------------------------------------------------------------------------
// Well Known Types.
// -----------------------------------------------------------------------------

bool is_inited_file_any;
bool is_inited_file_api;
bool is_inited_file_duration;
bool is_inited_file_field_mask;
bool is_inited_file_empty;
bool is_inited_file_source_context;
bool is_inited_file_struct;
bool is_inited_file_timestamp;
bool is_inited_file_type;
bool is_inited_file_wrappers;

// -----------------------------------------------------------------------------
// Reserved Name.
// -----------------------------------------------------------------------------

// Although we already have kReservedNames, we still add them to hash table to
// speed up look up.
const char *const kReservedNames[] = {
    "abstract",   "and",        "array",        "as",           "break",
    "callable",   "case",       "catch",        "class",        "clone",
    "const",      "continue",   "declare",      "default",      "die",
    "do",         "echo",       "else",         "elseif",       "empty",
    "enddeclare", "endfor",     "endforeach",   "endif",        "endswitch",
    "endwhile",   "eval",       "exit",         "extends",      "final",
    "for",        "foreach",    "function",     "global",       "goto",
    "if",         "implements", "include",      "include_once", "instanceof",
    "insteadof",  "interface",  "isset",        "list",         "namespace",
    "new",        "or",         "print",        "private",      "protected",
    "public",     "require",    "require_once", "return",       "static",
    "switch",     "throw",      "trait",        "try",          "unset",
    "use",        "var",        "while",        "xor",          "int",
    "float",      "bool",       "string",       "true",         "false",
    "null",       "void",       "iterable"};
const int kReservedNamesSize = 73;

bool is_reserved_name(const char* name) {
  void** value;
  return (php_proto_zend_hash_find(reserved_names, name, strlen(name),
                                   (void**)&value) == SUCCESS);
}

// -----------------------------------------------------------------------------
// Utilities.
// -----------------------------------------------------------------------------

zend_function_entry protobuf_functions[] = {
  ZEND_FE_END
};

static const zend_module_dep protobuf_deps[] = {
  ZEND_MOD_OPTIONAL("date")
  ZEND_MOD_END
};

zend_module_entry protobuf_module_entry = {
  STANDARD_MODULE_HEADER_EX,
  NULL,
  protobuf_deps,
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
  protobuf_globals->keep_descriptor_pool_after_request = false;
}

static PHP_GSHUTDOWN_FUNCTION(protobuf) {
}

PHP_INI_BEGIN()
STD_PHP_INI_ENTRY("protobuf.keep_descriptor_pool_after_request", "0",
                  PHP_INI_SYSTEM, OnUpdateBool,
                  keep_descriptor_pool_after_request, zend_protobuf_globals,
                  protobuf_globals)
PHP_INI_END()

#if PHP_MAJOR_VERSION < 7
static void php_proto_hashtable_descriptor_release_persistent(
    void* zval_ptr_ptr) {
  zval* zval_ptr = *(zval**)zval_ptr_ptr;

  if (!Z_DELREF_P(zval_ptr)) {
    TSRMLS_FETCH();
    zval_dtor(zval_ptr);
    pefree(zval_ptr, 1);
  }
}
#else
static void php_proto_hashtable_descriptor_release(zval* value) {
  void* ptr = Z_PTR_P(value);
  zend_object* object = *(zend_object**)ptr;
  GC_DELREF(object);
  if(GC_REFCOUNT(object) == 0) {
    zend_objects_store_del(object);
  }
  efree(ptr);
}

static void php_proto_hashtable_descriptor_release_persistent(zval* value) {
  void* ptr = Z_PTR_P(value);
  zend_object* object = *(zend_object**)ptr;
  GC_DELREF(object);
  if(GC_REFCOUNT(object) == 0) {
    zend_objects_store_del(object);
  }
  pefree(ptr, 1);
}
#endif

static void descriptors_create(bool persistent) {
  int i = 0;

  upb_def_to_php_obj_map = PEMALLOC(HashTable, persistent);
  ce_to_php_obj_map = PEMALLOC(HashTable, persistent);
  proto_to_php_obj_map = PEMALLOC(HashTable, persistent);
  reserved_names = PEMALLOC(HashTable, persistent);

  if (persistent) {
    zend_hash_init(upb_def_to_php_obj_map, 16, NULL,
                   php_proto_hashtable_descriptor_release_persistent,
                   persistent);
    zend_hash_init(ce_to_php_obj_map, 16, NULL,
                   php_proto_hashtable_descriptor_release_persistent,
                   persistent);

    zend_hash_init(proto_to_php_obj_map, 16, NULL,
                   php_proto_hashtable_descriptor_release_persistent,
                   persistent);
  } else {
    zend_hash_init(upb_def_to_php_obj_map, 16, NULL,
                   HASHTABLE_VALUE_DTOR,
                   persistent);
    zend_hash_init(ce_to_php_obj_map, 16, NULL,
                   HASHTABLE_VALUE_DTOR,
                   persistent);

    zend_hash_init(proto_to_php_obj_map, 16, NULL,
                   HASHTABLE_VALUE_DTOR,
                   persistent);
  }

  zend_hash_init(reserved_names, 16, NULL, NULL, persistent);
  for (i = 0; i < kReservedNamesSize; i++) {
    php_proto_zend_hash_update(reserved_names, kReservedNames[i],
                               strlen(kReservedNames[i]));
  }

  generated_pool = NULL;
  generated_pool_php = NULL;
  internal_generated_pool_php = NULL;

  is_inited_file_any = false;
  is_inited_file_api = false;
  is_inited_file_duration = false;
  is_inited_file_field_mask = false;
  is_inited_file_empty = false;
  is_inited_file_source_context = false;
  is_inited_file_struct = false;
  is_inited_file_timestamp = false;
  is_inited_file_type = false;
  is_inited_file_wrappers = false;
}

static void descriptors_free(bool persistent) {
  zend_hash_destroy(upb_def_to_php_obj_map);
  PEFREE(upb_def_to_php_obj_map, persistent);

  zend_hash_destroy(ce_to_php_obj_map);
  PEFREE(ce_to_php_obj_map, persistent);

  zend_hash_destroy(proto_to_php_obj_map);
  PEFREE(proto_to_php_obj_map, persistent);

  zend_hash_destroy(reserved_names);
  PEFREE(reserved_names, persistent);

#if PHP_MAJOR_VERSION < 7
  if (generated_pool_php != NULL) {
    zval_dtor(generated_pool_php);
    if (persistent) {
      PEFREE(generated_pool_php, 1);
    } else {
      FREE_ZVAL(generated_pool_php);
    }
  }
  if (internal_generated_pool_php != NULL) {
    zval_dtor(internal_generated_pool_php);
    if (persistent) {
      PEFREE(internal_generated_pool_php, 1);
    } else {
      FREE_ZVAL(internal_generated_pool_php);
    }
  }
#else
  if (generated_pool_php != NULL) {
    zval tmp;
    ZVAL_OBJ(&tmp, generated_pool_php);
    zval_dtor(&tmp);
  }
  if (internal_generated_pool_php != NULL) {
    zval tmp;
    ZVAL_OBJ(&tmp, internal_generated_pool_php);
    zval_dtor(&tmp);
  }
#endif

  is_inited_file_any = false;
  is_inited_file_api = false;
  is_inited_file_duration = false;
  is_inited_file_field_mask = false;
  is_inited_file_empty = false;
  is_inited_file_source_context = false;
  is_inited_file_struct = false;
  is_inited_file_timestamp = false;
  is_inited_file_type = false;
  is_inited_file_wrappers = false;
}

static PHP_RINIT_FUNCTION(protobuf) {
  if (PROTOBUF_G(keep_descriptor_pool_after_request)) {
    return 0;
  }

  descriptors_create(false);
  return 0;
}

static PHP_RSHUTDOWN_FUNCTION(protobuf) {
  if (PROTOBUF_G(keep_descriptor_pool_after_request)) {
    return 0;
  }

  descriptors_free(false);

  return 0;
}

static PHP_MINIT_FUNCTION(protobuf) {
  REGISTER_INI_ENTRIES();

  if (PROTOBUF_G(keep_descriptor_pool_after_request)) {
    descriptors_create(true);
  }

  descriptor_pool_init(TSRMLS_C);
  descriptor_init(TSRMLS_C);
  enum_descriptor_init(TSRMLS_C);
  enum_value_descriptor_init(TSRMLS_C);
  field_descriptor_init(TSRMLS_C);
  gpb_type_init(TSRMLS_C);
  internal_descriptor_pool_init(TSRMLS_C);
  map_field_init(TSRMLS_C);
  map_field_iter_init(TSRMLS_C);
  message_init(TSRMLS_C);
  oneof_descriptor_init(TSRMLS_C);
  repeated_field_init(TSRMLS_C);
  repeated_field_iter_init(TSRMLS_C);
  util_init(TSRMLS_C);

  gpb_metadata_any_init(TSRMLS_C);
  gpb_metadata_api_init(TSRMLS_C);
  gpb_metadata_duration_init(TSRMLS_C);
  gpb_metadata_field_mask_init(TSRMLS_C);
  gpb_metadata_empty_init(TSRMLS_C);
  gpb_metadata_source_context_init(TSRMLS_C);
  gpb_metadata_struct_init(TSRMLS_C);
  gpb_metadata_timestamp_init(TSRMLS_C);
  gpb_metadata_type_init(TSRMLS_C);
  gpb_metadata_wrappers_init(TSRMLS_C);

  any_init(TSRMLS_C);
  api_init(TSRMLS_C);
  bool_value_init(TSRMLS_C);
  bytes_value_init(TSRMLS_C);
  double_value_init(TSRMLS_C);
  duration_init(TSRMLS_C);
  enum_init(TSRMLS_C);
  enum_value_init(TSRMLS_C);
  field_cardinality_init(TSRMLS_C);
  field_init(TSRMLS_C);
  field_kind_init(TSRMLS_C);
  field_mask_init(TSRMLS_C);
  float_value_init(TSRMLS_C);
  empty_init(TSRMLS_C);
  int32_value_init(TSRMLS_C);
  int64_value_init(TSRMLS_C);
  list_value_init(TSRMLS_C);
  method_init(TSRMLS_C);
  mixin_init(TSRMLS_C);
  null_value_init(TSRMLS_C);
  option_init(TSRMLS_C);
  source_context_init(TSRMLS_C);
  string_value_init(TSRMLS_C);
  struct_init(TSRMLS_C);
  syntax_init(TSRMLS_C);
  timestamp_init(TSRMLS_C);
  type_init(TSRMLS_C);
  u_int32_value_init(TSRMLS_C);
  u_int64_value_init(TSRMLS_C);
  value_init(TSRMLS_C);

  return 0;
}

static PHP_MSHUTDOWN_FUNCTION(protobuf) {
  if (PROTOBUF_G(keep_descriptor_pool_after_request)) {
    descriptors_free(true);
  }
  PEFREE(message_handlers, 1);
  PEFREE(repeated_field_handlers, 1);
  PEFREE(repeated_field_iter_handlers, 1);
  PEFREE(map_field_handlers, 1);
  PEFREE(map_field_iter_handlers, 1);

  return 0;
}
