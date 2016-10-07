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

#include <ext/spl/spl_iterators.h>
#include <Zend/zend_API.h>
#include <Zend/zend_interfaces.h>

#include "protobuf.h"
#include "utf8.h"

ZEND_BEGIN_ARG_INFO_EX(arginfo_offsetGet, 0, 0, 1)
  ZEND_ARG_INFO(0, index)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_offsetSet, 0, 0, 2)
  ZEND_ARG_INFO(0, index)
  ZEND_ARG_INFO(0, newval)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_void, 0)
ZEND_END_ARG_INFO()

// Utilities

void* upb_value_memory(upb_value* v) {
  return (void*)(&v->val);
}

// -----------------------------------------------------------------------------
// Basic map operations on top of upb's strtable.
//
// Note that we roll our own `Map` container here because, as for
// `RepeatedField`, we want a strongly-typed container. This is so that any user
// errors due to incorrect map key or value types are raised as close as
// possible to the error site, rather than at some deferred point (e.g.,
// serialization).
//
// We build our `Map` on top of upb_strtable so that we're able to take
// advantage of the native_slot storage abstraction, as RepeatedField does.
// (This is not quite a perfect mapping -- see the key conversions below -- but
// gives us full support and error-checking for all value types for free.)
// -----------------------------------------------------------------------------

// Map values are stored using the native_slot abstraction (as with repeated
// field values), but keys are a bit special. Since we use a strtable, we need
// to store keys as sequences of bytes such that equality of those bytes maps
// one-to-one to equality of keys. We store strings directly (i.e., they map to
// their own bytes) and integers as native integers (using the native_slot
// abstraction).

// Note that there is another tradeoff here in keeping string keys as native
// strings rather than PHP strings: traversing the Map requires conversion to
// PHP string values on every traversal, potentially creating more garbage. We
// should consider ways to cache a PHP version of the key if this becomes an
// issue later.

// Forms a key to use with the underlying strtable from a PHP key value. |buf|
// must point to TABLE_KEY_BUF_LENGTH bytes of temporary space, used to
// construct a key byte sequence if needed. |out_key| and |out_length| provide
// the resulting key data/length.
#define TABLE_KEY_BUF_LENGTH 8  // sizeof(uint64_t)
static bool table_key(Map* self, zval* key,
                      char* buf,
                      const char** out_key,
                      size_t* out_length TSRMLS_DC) {
  switch (self->key_type) {
    case UPB_TYPE_STRING:
      if (!protobuf_convert_to_string(key)) {
        return false;
      }
      if (!is_structurally_valid_utf8(Z_STRVAL_P(key), Z_STRLEN_P(key))) {
        zend_error(E_USER_ERROR, "Given key is not UTF8 encoded.");
        return false;
      }
      *out_key = Z_STRVAL_P(key);
      *out_length = Z_STRLEN_P(key);
      break;

#define CASE_TYPE(upb_type, type, c_type, php_type)                   \
  case UPB_TYPE_##upb_type: {                                         \
    c_type type##_value;                                              \
    if (!protobuf_convert_to_##type(key, &type##_value)) {            \
      return false;                                                   \
    }                                                                 \
    native_slot_set(self->key_type, NULL, buf, key TSRMLS_CC);        \
    *out_key = buf;                                                   \
    *out_length = native_slot_size(self->key_type);                   \
    break;                                                            \
  }
      CASE_TYPE(BOOL, bool, int8_t, BOOL)
      CASE_TYPE(INT32, int32, int32_t, LONG)
      CASE_TYPE(INT64, int64, int64_t, LONG)
      CASE_TYPE(UINT32, uint32, uint32_t, LONG)
      CASE_TYPE(UINT64, uint64, uint64_t, LONG)

#undef CASE_TYPE

    default:
      // Map constructor should not allow a Map with another key type to be
      // constructed.
      assert(false);
      break;
  }

  return true;
}

// -----------------------------------------------------------------------------
// MapField methods
// -----------------------------------------------------------------------------

static zend_function_entry map_field_methods[] = {
  PHP_ME(MapField, __construct,  NULL,              ZEND_ACC_PUBLIC)
  PHP_ME(MapField, offsetExists, arginfo_offsetGet, ZEND_ACC_PUBLIC)
  PHP_ME(MapField, offsetGet,    arginfo_offsetGet, ZEND_ACC_PUBLIC)
  PHP_ME(MapField, offsetSet,    arginfo_offsetSet, ZEND_ACC_PUBLIC)
  PHP_ME(MapField, offsetUnset,  arginfo_offsetGet, ZEND_ACC_PUBLIC)
  PHP_ME(MapField, count,        arginfo_void,      ZEND_ACC_PUBLIC)
  ZEND_FE_END
};

// -----------------------------------------------------------------------------
// MapField creation/desctruction
// -----------------------------------------------------------------------------

zend_class_entry* map_field_type;
zend_object_handlers* map_field_handlers;

static void map_begin_internal(Map *map, MapIter *iter) {
  iter->self = map;
  upb_strtable_begin(&iter->it, &map->table);
}

static HashTable *map_field_get_gc(zval *object, zval ***table,
                                        int *n TSRMLS_DC) {
  // TODO(teboring): Unfortunately, zend engine does not support garbage
  // collection for custom array. We have to use zend engine's native array
  // instead.
  *table = NULL;
  *n = 0;
  return NULL;
}

void map_field_init(TSRMLS_D) {
  zend_class_entry class_type;
  const char* class_name = "Google\\Protobuf\\Internal\\MapField";
  INIT_CLASS_ENTRY_EX(class_type, class_name, strlen(class_name),
                      map_field_methods);

  map_field_type = zend_register_internal_class(&class_type TSRMLS_CC);
  map_field_type->create_object = map_field_create;

  zend_class_implements(map_field_type TSRMLS_CC, 2, spl_ce_ArrayAccess,
                        spl_ce_Countable);

  map_field_handlers = PEMALLOC(zend_object_handlers);
  memcpy(map_field_handlers, zend_get_std_object_handlers(),
         sizeof(zend_object_handlers));
  map_field_handlers->get_gc = map_field_get_gc;
}

zend_object_value map_field_create(zend_class_entry *ce TSRMLS_DC) {
  zend_object_value retval = {0};
  Map *intern;

  intern = emalloc(sizeof(Map));
  memset(intern, 0, sizeof(Map));

  zend_object_std_init(&intern->std, ce TSRMLS_CC);
  object_properties_init(&intern->std, ce);

  // Table value type is always UINT64: this ensures enough space to store the
  // native_slot value.
  if (!upb_strtable_init(&intern->table, UPB_CTYPE_UINT64)) {
    zend_error(E_USER_ERROR, "Could not allocate table.");
  }

  retval.handle = zend_objects_store_put(
      intern, (zend_objects_store_dtor_t)zend_objects_destroy_object,
      (zend_objects_free_object_storage_t)map_field_free, NULL TSRMLS_CC);
  retval.handlers = map_field_handlers;

  return retval;
}

void map_field_free(void *object TSRMLS_DC) {
  Map *map = (Map *)object;

  switch (map->value_type) {
    case UPB_TYPE_MESSAGE:
    case UPB_TYPE_STRING:
    case UPB_TYPE_BYTES: {
      MapIter it;
      int len;
      for (map_begin_internal(map, &it); !map_done(&it); map_next(&it)) {
        upb_value value = map_iter_value(&it, &len);
        void *mem = upb_value_memory(&value);
        zval_ptr_dtor(mem);
      }
      break;
    }
    default:
      break;
  }

  upb_strtable_uninit(&map->table);
  zend_object_std_dtor(&map->std TSRMLS_CC);
  efree(object);
}

void map_field_create_with_type(zend_class_entry *ce, const upb_fielddef *field,
                                zval **map_field TSRMLS_DC) {
  MAKE_STD_ZVAL(*map_field);
  Z_TYPE_PP(map_field) = IS_OBJECT;
  Z_OBJVAL_PP(map_field) =
      map_field_type->create_object(map_field_type TSRMLS_CC);

  Map* intern =
      (Map*)zend_object_store_get_object(*map_field TSRMLS_CC);

  const upb_fielddef *key_field = map_field_key(field);
  const upb_fielddef *value_field = map_field_value(field);
  intern->key_type = upb_fielddef_type(key_field);
  intern->value_type = upb_fielddef_type(value_field);
  intern->msg_ce = field_type_class(value_field TSRMLS_CC);
}

static void map_field_free_element(void *object) {
}

// -----------------------------------------------------------------------------
// MapField Handlers
// -----------------------------------------------------------------------------

static bool map_field_read_dimension(zval *object, zval *key, int type,
                                     zval **retval TSRMLS_DC) {
  Map *intern =
      (Map *)zend_object_store_get_object(object TSRMLS_CC);

  char keybuf[TABLE_KEY_BUF_LENGTH];
  const char* keyval = NULL;
  size_t length = 0;
  upb_value v;
#ifndef NDEBUG
  v.ctype = UPB_CTYPE_UINT64;
#endif
  if (!table_key(intern, key, keybuf, &keyval, &length TSRMLS_CC)) {
    return false;
  }

  if (upb_strtable_lookup2(&intern->table, keyval, length, &v)) {
    void* mem = upb_value_memory(&v);
    native_slot_get(intern->value_type, mem, retval TSRMLS_CC);
    return true;
  } else {
    zend_error(E_USER_ERROR, "Given key doesn't exist.");
    return false;
  }
}

bool map_index_set(Map *intern, const char* keyval, int length, upb_value v) {
  // Replace any existing value by issuing a 'remove' operation first.
  upb_strtable_remove2(&intern->table, keyval, length, NULL);
  if (!upb_strtable_insert2(&intern->table, keyval, length, v)) {
    zend_error(E_USER_ERROR, "Could not insert into table");
    return false;
  }
  return true;
}

static bool map_field_write_dimension(zval *object, zval *key,
                                      zval *value TSRMLS_DC) {
  Map *intern = (Map *)zend_object_store_get_object(object TSRMLS_CC);

  char keybuf[TABLE_KEY_BUF_LENGTH];
  const char* keyval = NULL;
  size_t length = 0;
  upb_value v;
  void* mem;
  if (!table_key(intern, key, keybuf, &keyval, &length TSRMLS_CC)) {
    return false;
  }

  mem = upb_value_memory(&v);
  memset(mem, 0, native_slot_size(intern->value_type));
  if (!native_slot_set(intern->value_type, intern->msg_ce, mem, value
		      TSRMLS_CC)) {
    return false;
  }
#ifndef NDEBUG
  v.ctype = UPB_CTYPE_UINT64;
#endif

  // Replace any existing value by issuing a 'remove' operation first.
  upb_strtable_remove2(&intern->table, keyval, length, NULL);
  if (!upb_strtable_insert2(&intern->table, keyval, length, v)) {
    zend_error(E_USER_ERROR, "Could not insert into table");
    return false;
  }

  return true;
}

static bool map_field_unset_dimension(zval *object, zval *key TSRMLS_DC) {
  Map *intern = (Map *)zend_object_store_get_object(object TSRMLS_CC);

  char keybuf[TABLE_KEY_BUF_LENGTH];
  const char* keyval = NULL;
  size_t length = 0;
  upb_value v;
  if (!table_key(intern, key, keybuf, &keyval, &length TSRMLS_CC)) {
    return false;
  }
#ifndef NDEBUG
  v.ctype = UPB_CTYPE_UINT64;
#endif

  upb_strtable_remove2(&intern->table, keyval, length, &v);

  return true;
}

// -----------------------------------------------------------------------------
// PHP MapField Methods
// -----------------------------------------------------------------------------

PHP_METHOD(MapField, __construct) {
  long key_type, value_type;
  zend_class_entry* klass = NULL;

  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ll|C", &key_type,
                            &value_type, &klass) == FAILURE) {
    return;
  }

  Map* intern =
      (Map*)zend_object_store_get_object(getThis() TSRMLS_CC);
  intern->key_type = to_fieldtype(key_type);
  intern->value_type = to_fieldtype(value_type);
  intern->msg_ce = klass;

  // Check that the key type is an allowed type.
  switch (intern->key_type) {
    case UPB_TYPE_INT32:
    case UPB_TYPE_INT64:
    case UPB_TYPE_UINT32:
    case UPB_TYPE_UINT64:
    case UPB_TYPE_BOOL:
    case UPB_TYPE_STRING:
    case UPB_TYPE_BYTES:
      // These are OK.
      break;
    default:
      zend_error(E_USER_ERROR, "Invalid key type for map.");
  }
}

PHP_METHOD(MapField, offsetExists) {
  zval *key;
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z", &key) ==
      FAILURE) {
    return;
  }

  Map *intern = (Map *)zend_object_store_get_object(getThis() TSRMLS_CC);

  char keybuf[TABLE_KEY_BUF_LENGTH];
  const char* keyval = NULL;
  size_t length = 0;
  upb_value v;
#ifndef NDEBUG
  v.ctype = UPB_CTYPE_UINT64;
#endif
  if (!table_key(intern, key, keybuf, &keyval, &length TSRMLS_CC)) {
    RETURN_BOOL(false);
  }

  RETURN_BOOL(upb_strtable_lookup2(&intern->table, keyval, length, &v));
}

PHP_METHOD(MapField, offsetGet) {
  zval *index, *value;
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z", &index) ==
      FAILURE) {
    return;
  }
  map_field_read_dimension(getThis(), index, BP_VAR_R,
                           return_value_ptr TSRMLS_CC);
}

PHP_METHOD(MapField, offsetSet) {
  zval *index, *value;
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "zz", &index, &value) ==
      FAILURE) {
    return;
  }
  map_field_write_dimension(getThis(), index, value TSRMLS_CC);
}

PHP_METHOD(MapField, offsetUnset) {
  zval *index;
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z", &index) ==
      FAILURE) {
    return;
  }
  map_field_unset_dimension(getThis(), index TSRMLS_CC);
}

PHP_METHOD(MapField, count) {
  Map *intern =
      (Map *)zend_object_store_get_object(getThis() TSRMLS_CC);

  if (zend_parse_parameters_none() == FAILURE) {
    return;
  }

  RETURN_LONG(upb_strtable_count(&intern->table));
}

// -----------------------------------------------------------------------------
// Map Iterator
// -----------------------------------------------------------------------------

void map_begin(zval *map_php, MapIter *iter TSRMLS_DC) {
  Map *self = UNBOX(Map, map_php);
  map_begin_internal(self, iter);
}

void map_next(MapIter *iter) {
  upb_strtable_next(&iter->it);
}

bool map_done(MapIter *iter) {
  return upb_strtable_done(&iter->it);
}

const char *map_iter_key(MapIter *iter, int *len) {
  *len = upb_strtable_iter_keylength(&iter->it);
  return upb_strtable_iter_key(&iter->it);
}

upb_value map_iter_value(MapIter *iter, int *len) {
  *len = native_slot_size(iter->self->value_type);
  return upb_strtable_iter_value(&iter->it);
}
