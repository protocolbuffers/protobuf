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

ZEND_DECLARE_MODULE_GLOBALS(protobuf)

// Global map from upb {msg,enum}defs to wrapper Descriptor/EnumDescriptor
// instances.
static HashTable* upb_def_to_php_obj_map;
static upb_inttable upb_def_to_desc_map_persistent;
static upb_inttable upb_def_to_enumdesc_map_persistent;
// Global map from message/enum's php class entry to corresponding wrapper
// Descriptor/EnumDescriptor instances.
static HashTable* ce_to_php_obj_map;
static upb_strtable ce_to_desc_map_persistent;
static upb_strtable ce_to_enumdesc_map_persistent;
// Global map from message/enum's proto fully-qualified name to corresponding
// wrapper Descriptor/EnumDescriptor instances.
static upb_strtable proto_to_desc_map_persistent;
static upb_strtable class_to_desc_map_persistent;

upb_strtable reserved_names;

// -----------------------------------------------------------------------------
// Global maps.
// -----------------------------------------------------------------------------

static void add_to_table(HashTable* t, const void* def, void* value) {
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

void add_msgdef_desc(const upb_msgdef* m, DescriptorInternal* desc) {
  upb_inttable_insertptr(&upb_def_to_desc_map_persistent,
                         m, upb_value_ptr(desc));
}

DescriptorInternal* get_msgdef_desc(const upb_msgdef* m) {
  upb_value v;
#ifndef NDEBUG
  v.ctype = UPB_CTYPE_PTR;
#endif
  if (!upb_inttable_lookupptr(&upb_def_to_desc_map_persistent, m, &v)) {
    return NULL;
  } else {
    return upb_value_getptr(v);
  }
}

void add_enumdef_enumdesc(const upb_enumdef* e, EnumDescriptorInternal* desc) {
  upb_inttable_insertptr(&upb_def_to_enumdesc_map_persistent,
                         e, upb_value_ptr(desc));
}

EnumDescriptorInternal* get_enumdef_enumdesc(const upb_enumdef* e) {
  upb_value v;
#ifndef NDEBUG
  v.ctype = UPB_CTYPE_PTR;
#endif
  if (!upb_inttable_lookupptr(&upb_def_to_enumdesc_map_persistent, e, &v)) {
    return NULL;
  } else {
    return upb_value_getptr(v);
  }
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

void add_ce_desc(const zend_class_entry* ce, DescriptorInternal* desc) {
#if PHP_MAJOR_VERSION < 7
  const char* klass = ce->name;
#else
  const char* klass = ZSTR_VAL(ce->name);
#endif
  upb_strtable_insert(&ce_to_desc_map_persistent, klass,
                      upb_value_ptr(desc));
}

DescriptorInternal* get_ce_desc(const zend_class_entry* ce) {
#if PHP_MAJOR_VERSION < 7
  const char* klass = ce->name;
#else
  const char* klass = ZSTR_VAL(ce->name);
#endif

  upb_value v;
#ifndef NDEBUG
  v.ctype = UPB_CTYPE_PTR;
#endif

  if (!upb_strtable_lookup(&ce_to_desc_map_persistent, klass, &v)) {
    return NULL;
  } else {
    return upb_value_getptr(v);
  }
}

void add_ce_enumdesc(const zend_class_entry* ce, EnumDescriptorInternal* desc) {
#if PHP_MAJOR_VERSION < 7
  const char* klass = ce->name;
#else
  const char* klass = ZSTR_VAL(ce->name);
#endif
  upb_strtable_insert(&ce_to_enumdesc_map_persistent, klass,
                      upb_value_ptr(desc));
}

EnumDescriptorInternal* get_ce_enumdesc(const zend_class_entry* ce) {
#if PHP_MAJOR_VERSION < 7
  const char* klass = ce->name;
#else
  const char* klass = ZSTR_VAL(ce->name);
#endif
  upb_value v;
#ifndef NDEBUG
  v.ctype = UPB_CTYPE_PTR;
#endif
  if (!upb_strtable_lookup(&ce_to_enumdesc_map_persistent, klass, &v)) {
    return NULL;
  } else {
    return upb_value_getptr(v);
  }
}

bool class_added(const void* ce) {
  return get_ce_desc(ce) != NULL;
}

void add_proto_desc(const char* proto, DescriptorInternal* desc) {
  upb_strtable_insert(&proto_to_desc_map_persistent, proto,
                      upb_value_ptr(desc));
}

DescriptorInternal* get_proto_desc(const char* proto) {
  upb_value v;
#ifndef NDEBUG
  v.ctype = UPB_CTYPE_PTR;
#endif
  if (!upb_strtable_lookup(&proto_to_desc_map_persistent, proto, &v)) {
    return NULL;
  } else {
    return upb_value_getptr(v);
  }
}

void add_class_desc(const char* klass, DescriptorInternal* desc) {
  upb_strtable_insert(&class_to_desc_map_persistent, klass,
                      upb_value_ptr(desc));
}

DescriptorInternal* get_class_desc(const char* klass) {
  upb_value v;
#ifndef NDEBUG
  v.ctype = UPB_CTYPE_PTR;
#endif
  if (!upb_strtable_lookup(&class_to_desc_map_persistent, klass, &v)) {
    return NULL;
  } else {
    return upb_value_getptr(v);
  }
}

void add_class_enumdesc(const char* klass, EnumDescriptorInternal* desc) {
  upb_strtable_insert(&class_to_desc_map_persistent, klass,
                      upb_value_ptr(desc));
}

EnumDescriptorInternal* get_class_enumdesc(const char* klass) {
  upb_value v;
#ifndef NDEBUG
  v.ctype = UPB_CTYPE_PTR;
#endif
  if (!upb_strtable_lookup(&class_to_desc_map_persistent, klass, &v)) {
    return NULL;
  } else {
    return upb_value_getptr(v);
  }
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
  upb_value v;
#ifndef NDEBUG
  v.ctype = UPB_CTYPE_UINT64;
#endif
  return upb_strtable_lookup2(&reserved_names, name, strlen(name), &v);
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
}

static PHP_GSHUTDOWN_FUNCTION(protobuf) {
}

#if PHP_MAJOR_VERSION >= 7
static void php_proto_hashtable_descriptor_release(zval* value) {
  void* ptr = Z_PTR_P(value);
  zend_object* object = *(zend_object**)ptr;
  GC_DELREF(object);
  if(GC_REFCOUNT(object) == 0) {
    zend_objects_store_del(object);
  }
  efree(ptr);
}
#endif

static void initialize_persistent_descriptor_pool(TSRMLS_D) {
  upb_inttable_init(&upb_def_to_desc_map_persistent, UPB_CTYPE_PTR);
  upb_inttable_init(&upb_def_to_enumdesc_map_persistent, UPB_CTYPE_PTR);
  upb_strtable_init(&ce_to_desc_map_persistent, UPB_CTYPE_PTR);
  upb_strtable_init(&ce_to_enumdesc_map_persistent, UPB_CTYPE_PTR);
  upb_strtable_init(&proto_to_desc_map_persistent, UPB_CTYPE_PTR);
  upb_strtable_init(&class_to_desc_map_persistent, UPB_CTYPE_PTR);

  internal_descriptor_pool_impl_init(&generated_pool_impl TSRMLS_CC);

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
  ALLOC_HASHTABLE(upb_def_to_php_obj_map);
  zend_hash_init(upb_def_to_php_obj_map, 16, NULL, HASHTABLE_VALUE_DTOR, 0);

  ALLOC_HASHTABLE(ce_to_php_obj_map);
  zend_hash_init(ce_to_php_obj_map, 16, NULL, HASHTABLE_VALUE_DTOR, 0);

  generated_pool = NULL;
  generated_pool_php = NULL;
  internal_generated_pool_php = NULL;

  if (PROTOBUF_G(keep_descriptor_pool_after_request)) {
    // Needs to clean up obsolete class entry
    upb_strtable_iter i;
    upb_value v;

    DescriptorInternal* desc;
    for(upb_strtable_begin(&i, &ce_to_desc_map_persistent);
        !upb_strtable_done(&i);
        upb_strtable_next(&i)) {
      v = upb_strtable_iter_value(&i);
      desc = upb_value_getptr(v);
      desc->klass = NULL;
    }

    EnumDescriptorInternal* enumdesc;
    for(upb_strtable_begin(&i, &ce_to_enumdesc_map_persistent);
        !upb_strtable_done(&i);
        upb_strtable_next(&i)) {
      v = upb_strtable_iter_value(&i);
      enumdesc = upb_value_getptr(v);
      enumdesc->klass = NULL;
    }
  } else {
    initialize_persistent_descriptor_pool(TSRMLS_C);
  }

  return 0;
}

static void cleanup_desc_table(upb_inttable* t) {
  upb_inttable_iter i;
  upb_value v;
  DescriptorInternal* desc;
  for(upb_inttable_begin(&i, t);
      !upb_inttable_done(&i);
      upb_inttable_next(&i)) {
    v = upb_inttable_iter_value(&i);
    desc = upb_value_getptr(v);
    if (desc->layout) {
      free_layout(desc->layout);
      desc->layout = NULL;
    }
    free(desc->classname);
    SYS_FREE(desc);
  }
}

static void cleanup_enumdesc_table(upb_inttable* t) {
  upb_inttable_iter i;
  upb_value v;
  EnumDescriptorInternal* desc;
  for(upb_inttable_begin(&i, t);
      !upb_inttable_done(&i);
      upb_inttable_next(&i)) {
    v = upb_inttable_iter_value(&i);
    desc = upb_value_getptr(v);
    free(desc->classname);
    SYS_FREE(desc);
  }
}

static void cleanup_persistent_descriptor_pool(TSRMLS_D) {
  // Clean up

  // Only needs to clean one map out of three (def=>desc, ce=>desc, proto=>desc)
  cleanup_desc_table(&upb_def_to_desc_map_persistent);
  cleanup_enumdesc_table(&upb_def_to_enumdesc_map_persistent);

  internal_descriptor_pool_impl_destroy(&generated_pool_impl TSRMLS_CC);

  upb_inttable_uninit(&upb_def_to_desc_map_persistent);
  upb_inttable_uninit(&upb_def_to_enumdesc_map_persistent);
  upb_strtable_uninit(&ce_to_desc_map_persistent);
  upb_strtable_uninit(&ce_to_enumdesc_map_persistent);
  upb_strtable_uninit(&proto_to_desc_map_persistent);
  upb_strtable_uninit(&class_to_desc_map_persistent);
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
  if (internal_generated_pool_php != NULL) {
    zval_dtor(internal_generated_pool_php);
    FREE_ZVAL(internal_generated_pool_php);
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

  if (!PROTOBUF_G(keep_descriptor_pool_after_request)) {
    cleanup_persistent_descriptor_pool(TSRMLS_C);
  }

  return 0;
}

static void reserved_names_init() {
  size_t i;
  upb_value v = upb_value_bool(false);
  for (i = 0; i < kReservedNamesSize; i++) {
    upb_strtable_insert2(&reserved_names, kReservedNames[i],
                         strlen(kReservedNames[i]), v);
  }
}

PHP_INI_BEGIN()
STD_PHP_INI_ENTRY("protobuf.keep_descriptor_pool_after_request", "0",
                  PHP_INI_SYSTEM, OnUpdateBool,
                  keep_descriptor_pool_after_request, zend_protobuf_globals,
                  protobuf_globals)
PHP_INI_END()

static PHP_MINIT_FUNCTION(protobuf) {
  REGISTER_INI_ENTRIES();

  upb_strtable_init(&reserved_names, UPB_CTYPE_UINT64);
  reserved_names_init();

  if (PROTOBUF_G(keep_descriptor_pool_after_request)) {
    initialize_persistent_descriptor_pool(TSRMLS_C);
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
    cleanup_persistent_descriptor_pool(TSRMLS_C);
  }

  upb_strtable_uninit(&reserved_names);

  PEFREE(message_handlers);
  PEFREE(repeated_field_handlers);
  PEFREE(repeated_field_iter_handlers);
  PEFREE(map_field_handlers);
  PEFREE(map_field_iter_handlers);

  return 0;
}
