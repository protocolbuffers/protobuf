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

#include "protobuf_php.h"

UPB_BEGIN_EXTERN_C
#include "ext/date/php_date.h"
UPB_END_EXTERN_C

// -----------------------------------------------------------------------------
// Define static methods
// -----------------------------------------------------------------------------

upb_msgval tomsgval(zval* value, upb_fieldtype_t type, upb_alloc* alloc) {
  switch (type) {

 #define CASE_TYPE(UPBTYPE, TYPE, CTYPE, PHPTYPE)    \
    case UPB_TYPE_##UPBTYPE: {                       \
      CTYPE raw_value;                               \
      protobuf_convert_to_##TYPE(value, &raw_value); \
      return upb_msgval_##TYPE(raw_value);           \
    }

    CASE_TYPE(INT32,  int32,  int32_t,  LONG)
    CASE_TYPE(UINT32, uint32, uint32_t, LONG)
    CASE_TYPE(ENUM,   int32,  int32_t,  LONG)
    CASE_TYPE(INT64,  int64,  int64_t,  LONG)
    CASE_TYPE(UINT64, uint64, uint64_t, LONG)
    CASE_TYPE(FLOAT,  float,  float,    DOUBLE)
    CASE_TYPE(DOUBLE, double, double,   DOUBLE)
    CASE_TYPE(BOOL,   bool,   int8_t,   BOOL)

#undef CASE_TYPE

    case UPB_TYPE_STRING:
    case UPB_TYPE_BYTES: {
      protobuf_convert_to_string(value);
      char *mem = (char*)upb_malloc(alloc, Z_STRLEN_P(value) + 1);
      memcpy(mem, Z_STRVAL_P(value), Z_STRLEN_P(value) + 1);
      return upb_msgval_makestr(mem, Z_STRLEN_P(value));
    }
    case UPB_TYPE_MESSAGE: {
      if (Z_TYPE_P(value) == IS_NULL) {
        return upb_msgval_msg(NULL);
      } else {
        TSRMLS_FETCH();
        Message* intern = UNBOX(Message, value);
        return upb_msgval_msg(intern->msg);
      }
    }
    default:
      break;
  }
  UPB_UNREACHABLE();
}

void tophpval(const upb_msgval &msgval,
              upb_fieldtype_t type,
              zend_class_entry *subklass,
              ARENA arena,
              zval *retval) {
  switch (type) {
    case UPB_TYPE_INT32:
    case UPB_TYPE_ENUM:
      ZVAL_LONG(retval, upb_msgval_getint32(msgval));
      break;
    case UPB_TYPE_INT64:
      ZVAL_LONG(retval, upb_msgval_getint64(msgval));
      break;
    case UPB_TYPE_UINT32:
      ZVAL_LONG(retval, (int32_t)upb_msgval_getuint32(msgval));
      break;
    case UPB_TYPE_UINT64:
      ZVAL_LONG(retval, upb_msgval_getuint64(msgval));
      break;
    case UPB_TYPE_DOUBLE:
      ZVAL_DOUBLE(retval, upb_msgval_getdouble(msgval));
      break;
    case UPB_TYPE_FLOAT:
      ZVAL_DOUBLE(retval, upb_msgval_getfloat(msgval));
      break;
    case UPB_TYPE_BOOL:
      ZVAL_BOOL(retval, upb_msgval_getbool(msgval));
      break;
    case UPB_TYPE_STRING:
    case UPB_TYPE_BYTES: {
      upb_stringview str = upb_msgval_getstr(msgval);
      PROTO_ZVAL_STRINGL(retval, str.data, str.size, 1);
      break;
    }
    case UPB_TYPE_MESSAGE: {
      const upb_msg *msg = upb_msgval_getmsg(msgval);
      if (msg == NULL) {
        ZVAL_NULL(retval);
        return;
      }
      if (Z_TYPE_P(retval) != IS_OBJECT) {
        const upb_msgdef *subdef = class2msgdef(subklass);
        TSRMLS_FETCH();
        ZVAL_OBJ(retval, subklass->create_object(subklass TSRMLS_CC));
        Message* intern = UNBOX(Message, retval);
        Message_wrap(intern, const_cast<upb_msg*>(msg), subdef, arena);
      }
      return;
    }
    default:
      break;
  }
}

static zend_property_info *get_property_info(zval* object,
                                             zval* member TSRMLS_DC) {
#if PHP_MAJOR_VERSION < 7
  return zend_get_property_info(Z_OBJCE_P(object), member, true TSRMLS_CC);
#else
  return zend_get_property_info(Z_OBJCE_P(object), Z_STR_P(member), true);
#endif
}

static void message_set_message_internal(
    zval* object, zval* member, zval* value, const upb_fielddef* f TSRMLS_DC) {
  zend_property_info* property_info =
    get_property_info(object, member TSRMLS_CC);

#if PHP_MAJOR_VERSION < 7
  REPLACE_ZVAL_VALUE(OBJ_PROP(Z_OBJ_P(object), property_info->offset),
                     value, 1);
#else
  zval_ptr_dtor(OBJ_PROP(Z_OBJ_P(object), property_info->offset));
  ZVAL_ZVAL(OBJ_PROP(Z_OBJ_P(object), property_info->offset),
                     value, 1, 0);
#endif
  zval* cached_value = CACHED_VALUE_PTR_TO_ZVAL_PTR(
      OBJ_PROP(Z_OBJ_P(object), property_info->offset));

  // Set to upb msg.
  int field_index = upb_fielddef_index(f);
  upb_fieldtype_t type = upb_fielddef_type(f);
  Message* self = UNBOX(Message, object);
  upb_msgval msgval = tomsgval(cached_value, type, upb_msg_alloc(self->msg));
  upb_msg_set(self->msg, field_index, msgval, self->layout);
}

static void message_set_array_internal(
    zval* object, zval* member, zval* value, const upb_fielddef* f TSRMLS_DC) {
  zval* cached_value = NULL;

  zend_property_info* property_info =
    get_property_info(object, member TSRMLS_CC);
  void * tmp = Z_OBJ_P(object);

  if (Z_TYPE_P(value) == IS_ARRAY) {
    // Create new php repeated field.
#if PHP_MAJOR_VERSION < 7
    SEPARATE_ZVAL_IF_NOT_REF(OBJ_PROP(Z_OBJ_P(object), property_info->offset));
#endif
    cached_value = CACHED_VALUE_PTR_TO_ZVAL_PTR(
        OBJ_PROP(Z_OBJ_P(object), property_info->offset));

    ZVAL_OBJ(cached_value, RepeatedField_type->create_object(
        RepeatedField_type TSRMLS_CC));
#if PHP_MAJOR_VERSION < 7
    Z_SET_ISREF_P(cached_value);
#endif

    // Initialize php repeated field.
    zend_class_entry* klass = NULL;
    if (upb_fielddef_issubmsg(f)) {
      const upb_msgdef *subdef = upb_fielddef_msgsubdef(f);
      klass = (zend_class_entry*)msgdef2class(subdef);
    }

    Message* self = UNBOX(Message, object);
    RepeatedField* intern = UNBOX(RepeatedField, cached_value);
    RepeatedField___construct(intern, upb_fielddef_descriptortype(f), 
                              self->arena, klass);
    //  Add elements
    HashTable* table = HASH_OF(value);
    HashPosition pointer;
    void* memory;
    for (zend_hash_internal_pointer_reset_ex(table, &pointer);
         proto_zend_hash_get_current_data_ex(table, (void**)&memory,
                                                 &pointer) == SUCCESS;
         zend_hash_move_forward_ex(table, &pointer)) {
      RepeatedField_append(intern, CACHED_VALUE_PTR_TO_ZVAL_PTR(
                           (CACHED_VALUE*)memory));
    }
  } else {
#if PHP_MAJOR_VERSION < 7
    REPLACE_ZVAL_VALUE(OBJ_PROP(Z_OBJ_P(object), property_info->offset),
                       value, 1);
#else
    zval_ptr_dtor(OBJ_PROP(Z_OBJ_P(object), property_info->offset));
    ZVAL_ZVAL(OBJ_PROP(Z_OBJ_P(object), property_info->offset),
                       value, 1, 0);
#endif
    cached_value = CACHED_VALUE_PTR_TO_ZVAL_PTR(
        OBJ_PROP(Z_OBJ_P(object), property_info->offset));
  }

  // Set to upb msg.
  upb_msgval msgval;
  RepeatedField *arr = UNBOX(RepeatedField, cached_value);
  Message* self = UNBOX(Message, object);
  int field_index = upb_fielddef_index(f);
  upb_msgval_setarr(&msgval, arr->array);
  upb_msg_set(self->msg, field_index, msgval, self->layout);
}

static void message_set_map_internal(
    zval* object, zval* member, zval* value, const upb_fielddef* f TSRMLS_DC) {
  zval* cached_value = NULL;

  zend_property_info* property_info =
    get_property_info(object, member TSRMLS_CC);

  if (Z_TYPE_P(value) == IS_ARRAY) {
    // Create new php repeated field.
#if PHP_MAJOR_VERSION < 7
    SEPARATE_ZVAL_IF_NOT_REF(OBJ_PROP(Z_OBJ_P(object), property_info->offset));
#endif
    cached_value = CACHED_VALUE_PTR_TO_ZVAL_PTR(
        OBJ_PROP(Z_OBJ_P(object), property_info->offset));

    ZVAL_OBJ(cached_value, MapField_type->create_object(
        MapField_type TSRMLS_CC));
#if PHP_MAJOR_VERSION < 7
    Z_SET_ISREF_P(cached_value);
#endif

    // Initialize php repeated field.
    const upb_msgdef *mapentry_msgdef = upb_fielddef_msgsubdef(f);
    const upb_fielddef *key_fielddef =
        upb_msgdef_ntof(mapentry_msgdef, "key", 3);
    const upb_fielddef *value_fielddef =
        upb_msgdef_ntof(mapentry_msgdef, "value", 5);
    zend_class_entry* klass = NULL;
    if (upb_fielddef_issubmsg(value_fielddef)) {
      const upb_msgdef *subdef = upb_fielddef_msgsubdef(value_fielddef);
      klass = (zend_class_entry*)msgdef2class(subdef);
    }

    Message* self = UNBOX(Message, object);
    MapField* intern = UNBOX(MapField, cached_value);
    MapField___construct(intern, 
                         upb_fielddef_descriptortype(key_fielddef),
                         upb_fielddef_descriptortype(value_fielddef),
                         self->arena, klass);

    //  Add elements
    HashTable* table = HASH_OF(value);
    HashPosition pointer;
    zval key;
    void* value;
    for (zend_hash_internal_pointer_reset_ex(table, &pointer);
         proto_zend_hash_get_current_data_ex(table, (void**)&value,
                                             &pointer) == SUCCESS;
         zend_hash_move_forward_ex(table, &pointer)) {
      zend_hash_get_current_key_zval_ex(table, &key, &pointer);
      MapField_offsetSet(intern, &key, CACHED_VALUE_PTR_TO_ZVAL_PTR(
                         (CACHED_VALUE*)value));
    }
  } else {
#if PHP_MAJOR_VERSION < 7
    REPLACE_ZVAL_VALUE(OBJ_PROP(Z_OBJ_P(object), property_info->offset),
                       value, 1);
#else
    zval_ptr_dtor(OBJ_PROP(Z_OBJ_P(object), property_info->offset));
    ZVAL_ZVAL(OBJ_PROP(Z_OBJ_P(object), property_info->offset),
                       value, 1, 0);
#endif
    cached_value = CACHED_VALUE_PTR_TO_ZVAL_PTR(
        OBJ_PROP(Z_OBJ_P(object), property_info->offset));
  }

  // Set to upb msg.
  upb_msgval msgval;
  MapField *map = UNBOX(MapField, cached_value);
  Message* self = UNBOX(Message, object);
  int field_index = upb_fielddef_index(f);
  upb_msgval_setmap(&msgval, map->map);
  upb_msg_set(self->msg, field_index, msgval, self->layout);
}

static void message_set_property_internal(zval* object, zval* member,
                                          zval* value TSRMLS_DC) {
  Message* self = UNBOX(Message, object);
  const upb_fielddef* f = upb_msgdef_ntofz(self->msgdef, Z_STRVAL_P(member));
  assert(f != NULL);

  if (upb_fielddef_ismap(f)) {
    message_set_map_internal(object, member, value, f TSRMLS_CC);
    return;
  } else if (upb_fielddef_isseq(f)) {
    message_set_array_internal(object, member, value, f TSRMLS_CC);
    return;
  } else if (upb_fielddef_type(f) == UPB_TYPE_MESSAGE) {
    message_set_message_internal(object, member, value, f TSRMLS_CC);
    return;
  }

  // Set scalar fields.
  int field_index = upb_fielddef_index(f);
  upb_fieldtype_t type = upb_fielddef_type(f);
  upb_msgval msgval = tomsgval(value, type, upb_msg_alloc(self->msg));
  upb_msg_set(self->msg, field_index, msgval, self->layout);
}

static zval* message_get_property_internal(zval* object, zval* member TSRMLS_DC) {
  Message* self = UNBOX(Message, object);
  const upb_fielddef* f = upb_msgdef_ntofz(self->msgdef, Z_STRVAL_P(member));
  assert(f != NULL);
  int field_index = upb_fielddef_index(f);
  upb_fieldtype_t type = upb_fielddef_type(f);

  // Find cached zval.
  zend_property_info* property_info;
#if PHP_MAJOR_VERSION < 7
  property_info =
      zend_get_property_info(Z_OBJCE_P(object), member, true TSRMLS_CC);
#else
  property_info =
      zend_get_property_info(Z_OBJCE_P(object), Z_STR_P(member), true);
#endif

#if PHP_MAJOR_VERSION < 7
  if ((type == UPB_TYPE_STRING || type == UPB_TYPE_BYTES) &&
      !upb_fielddef_isseq(f)) {
    zval null_value;
    ZVAL_NULL(&null_value);
    REPLACE_ZVAL_VALUE(OBJ_PROP(Z_OBJ_P(object),
                       property_info->offset), &null_value, 0);
  } else {
    SEPARATE_ZVAL_IF_NOT_REF(OBJ_PROP(Z_OBJ_P(object), property_info->offset));
  }
#else
  if ((type == UPB_TYPE_STRING || type == UPB_TYPE_BYTES) &&
      !upb_fielddef_isseq(f)) {
    zval_ptr_dtor(OBJ_PROP(Z_OBJ_P(object), property_info->offset));
  }
#endif

  zval* retval = CACHED_VALUE_PTR_TO_ZVAL_PTR(
    OBJ_PROP(Z_OBJ_P(object), property_info->offset));

  upb_msgval msgval = upb_msg_get(self->msg, field_index, self->layout);

  // TODO(teboring): Add test for corner cases.
  if (upb_fielddef_ismap(f)) {
    const upb_map *map = upb_msgval_getmap(msgval);
    if (map != NULL) {
      if (Z_TYPE_P(retval) == IS_OBJECT) {
        MapField* cppmap = UNBOX(MapField, retval);
        if (cppmap->map == map) {
          return retval;
        }
      }
    }
  } else if (upb_fielddef_isseq(f)) {
    const upb_array *array = upb_msgval_getarr(msgval);
    if (array != NULL) {
      if (Z_TYPE_P(retval) == IS_OBJECT) {
        RepeatedField* cpparray = UNBOX(RepeatedField, retval);
        if (cpparray->array == array) {
          return retval;
        }
      }
    }
  } else if (upb_fielddef_type(f) == UPB_TYPE_MESSAGE) {
    const upb_msg *msg = upb_msgval_getmsg(msgval);
    if (msg == NULL) {
      if (Z_TYPE_P(retval) == IS_NULL) {
        return retval;
      }
    } else {
      if (Z_TYPE_P(retval) == IS_OBJECT) {
        Message* cppmsg = UNBOX(Message, retval);
        if (cppmsg->msg == msg) {
          return retval;
        }
      }
    }
  }

  // Up to this point, old cached zval is not valid.
  if (Z_TYPE_P(retval) == IS_OBJECT) {
#if PHP_MAJOR_VERSION < 7
    zval null_value;
    ZVAL_NULL(&null_value);
    REPLACE_ZVAL_VALUE(OBJ_PROP(Z_OBJ_P(object),
                       property_info->offset), &null_value, 0);
    retval = CACHED_VALUE_PTR_TO_ZVAL_PTR(
      OBJ_PROP(Z_OBJ_P(object), property_info->offset));
#else
    zval_ptr_dtor(retval);
    ZVAL_NULL(retval);
#endif
  }

  // Update returned value
  if (upb_fielddef_ismap(f)) {
    if (Z_TYPE_P(retval) == IS_NULL) {
      ZVAL_OBJ(retval, MapField_type->create_object(
          MapField_type TSRMLS_CC));
#if PHP_MAJOR_VERSION < 7
       Z_SET_ISREF_P(retval);
#endif
    }
    MapField* intern = UNBOX(MapField, retval);

    zend_class_entry* klass = NULL;
    const upb_msgdef *mapentry_msgdef = upb_fielddef_msgsubdef(f);
    const upb_fielddef *key_fielddef =
        upb_msgdef_ntof(mapentry_msgdef, "key", 3);
    const upb_fielddef *value_fielddef =
        upb_msgdef_ntof(mapentry_msgdef, "value", 5);

    if (upb_fielddef_issubmsg(value_fielddef)) {
      const upb_msgdef *subdef = upb_fielddef_msgsubdef(value_fielddef);
      klass = (zend_class_entry*)msgdef2class(subdef);
    }

    const upb_map *map = upb_msgval_getmap(msgval);
    if (map == NULL) {
      MapField___construct(intern, 
                           upb_fielddef_descriptortype(key_fielddef),
                           upb_fielddef_descriptortype(value_fielddef),
                           self->arena, klass);
      upb_msg_set(self->msg, field_index,
                  upb_msgval_map(intern->map), self->layout);
    } else {
      MapField_wrap(intern, const_cast<upb_map*>(map), klass, self->arena);
    }
  } else if (upb_fielddef_isseq(f)) {
    ZVAL_OBJ(retval, RepeatedField_type->create_object(
        RepeatedField_type TSRMLS_CC));
#if PHP_MAJOR_VERSION < 7
    Z_SET_ISREF_P(retval);
#endif
    RepeatedField* intern = UNBOX(RepeatedField, retval);

    zend_class_entry* klass = NULL;
    if (upb_fielddef_issubmsg(f)) {
      const upb_msgdef *subdef = upb_fielddef_msgsubdef(f);
      klass = (zend_class_entry*)msgdef2class(subdef);
    }

    const upb_array *arr = upb_msgval_getarr(msgval);
    if (arr == NULL) {
      RepeatedField___construct(intern, upb_fielddef_descriptortype(f), 
                                self->arena, klass);
      upb_msg_set(self->msg, field_index,
                  upb_msgval_arr(intern->array), self->layout);
    } else {
      RepeatedField_wrap(intern, const_cast<upb_array*>(arr),
                         klass, self->arena);
    }
  } else {
    zend_class_entry *subklass = NULL;
    if (upb_fielddef_issubmsg(f)) {
      const upb_msgdef *subdef = upb_fielddef_msgsubdef(f);
      subklass = (zend_class_entry*)msgdef2class(subdef);
    }
    tophpval(msgval, type, subklass, self->arena, retval);
  }

  return retval;
}

#if PHP_MAJOR_VERSION < 7
static void message_set_property(zval* object, zval* member, zval* value,
                                 const zend_literal* key TSRMLS_DC) {
#else
static void message_set_property(zval* object, zval* member, zval* value,
                                 void** cache_slot) {
#endif
  if (Z_TYPE_P(member) != IS_STRING) {
    zend_error(E_USER_ERROR, "Unexpected type for field name");
    return;
  }

#if PHP_MAJOR_VERSION < 7 || (PHP_MAJOR_VERSION == 7 && PHP_MINOR_VERSION == 0)
  if (Z_OBJCE_P(object) != EG(scope)) {
#else
  if (Z_OBJCE_P(object) != zend_get_executed_scope()) {
#endif
    // User cannot set property directly (e.g., $m->a = 1)
    zend_error(E_USER_ERROR, "Cannot access private property.");
    return;
  }

  message_set_property_internal(object, member, value TSRMLS_CC);
}

#if PHP_MAJOR_VERSION < 7
static zval* message_get_property(zval* object, zval* member, int type,
                                  const zend_literal* key TSRMLS_DC) {
#else
static zval* message_get_property(zval* object, zval* member, int type,
                                  void** cache_slot, zval* rv) {
#endif
  if (Z_TYPE_P(member) != IS_STRING) {
    zend_error(E_USER_ERROR, "Property name has to be a string.");
    return PROTO_GLOBAL_UNINITIALIZED_ZVAL;
  }

#if PHP_MAJOR_VERSION < 7 || (PHP_MAJOR_VERSION == 7 && PHP_MINOR_VERSION == 0)
  if (Z_OBJCE_P(object) != EG(scope)) {
#else
  if (Z_OBJCE_P(object) != zend_get_executed_scope()) {
#endif
    // User cannot get property directly (e.g., $a = $m->a)
    zend_error(E_USER_ERROR, "Cannot access private property.");
    return PROTO_GLOBAL_UNINITIALIZED_ZVAL;
  }

  return message_get_property_internal(object, member TSRMLS_CC);
}

static void Message_init_handlers(zend_object_handlers* handlers) {
  handlers->write_property = message_set_property;
  handlers->read_property = message_get_property;
//   handlers->get_property_ptr_ptr = message_get_property_ptr_ptr;
//   handlers->get_properties = message_get_properties;
//   handlers->get_gc = message_get_gc;
}

static void Message_init_type(zend_class_entry* klass) {}

// -----------------------------------------------------------------------------
// Define PHP class
// -----------------------------------------------------------------------------

PHP_METHOD(Message, __construct);
PHP_METHOD(Message, clear);
PHP_METHOD(Message, serializeToJsonString);
PHP_METHOD(Message, serializeToString);
PHP_METHOD(Message, mergeFrom);
PHP_METHOD(Message, mergeFromJsonString);
PHP_METHOD(Message, mergeFromString);
PHP_METHOD(Message, writeOneof);
PHP_METHOD(Message, readOneof);
PHP_METHOD(Message, whichOneof);

static zend_function_entry Message_methods[] = {
  PHP_ME(Message, __construct, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(Message, clear, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(Message, serializeToString, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(Message, mergeFrom, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(Message, mergeFromString, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(Message, writeOneof, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(Message, readOneof, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(Message, whichOneof, NULL, ZEND_ACC_PUBLIC)
  {NULL, NULL, NULL}
};

PROTO_DEFINE_CLASS(Message,
             "Google\\Protobuf\\Internal\\Message");

// -----------------------------------------------------------------------------
// Define PHP methods
// -----------------------------------------------------------------------------

PHP_METHOD(Message, __construct) {
  Message* intern = UNBOX(Message, getThis());
  zend_class_entry* ce = Z_OBJCE_P(getThis());
  const upb_msgdef* msgdef = class2msgdef(ce);
  Message___construct(intern, msgdef);
}

PHP_METHOD(Message, clear) {
  Message* intern = UNBOX(Message, getThis());
  Message_clear(intern);
}

PHP_METHOD(Message, serializeToJsonString) {
  Message* intern = UNBOX(Message, getThis());
  stackenv se;
  stackenv_init(&se, "Error occurred during encoding: %s");
  size_t size;
  const char* data = upb_encode2(intern->msg, intern->layout, &se.env, &size);
  PROTO_RETVAL_STRINGL(data, size, 1);
  stackenv_uninit(&se);
}

PHP_METHOD(Message, serializeToString) {
  Message* intern = UNBOX(Message, getThis());
  stackenv se;
  stackenv_init(&se, "Error occurred during encoding: %s");
  size_t size;
  const char* data = upb_encode2(intern->msg, intern->layout, &se.env, &size);
  PROTO_RETVAL_STRINGL(data, size, 1);
  stackenv_uninit(&se);
}

PHP_METHOD(Message, mergeFromString) {
  Message* intern = UNBOX(Message, getThis());
  char *data = NULL;
  PROTO_SIZE size;

  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &data, &size) ==
      FAILURE) {
    return;
  }
  if (!Message_mergeFromString(intern, data, size)) {
    zend_throw_exception(
        NULL, "Invalid data for binary format parsing.",
        0 TSRMLS_CC);
  }
}

PHP_METHOD(Message, mergeFrom) {
  zval* value;
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "O", &value,
                            Message_type) == FAILURE) {
    return;
  }

  Message* from = UNBOX(Message, value);
  Message* to = UNBOX(Message, getThis());

  if(from->msgdef != to->msgdef) {
    zend_error(E_USER_ERROR, "Cannot merge messages with different class.");
    return;
  }

  Message_mergeFrom(from, to);
}

static void Message_readOneof(Message *self, PROTO_LONG index,
                              zval *return_value) {
  const upb_fielddef* f = upb_msgdef_itof(self->msgdef, index);

  zend_class_entry *subklass = NULL;
  if (upb_fielddef_issubmsg(f)) {
    const upb_msgdef *subdef = upb_fielddef_msgsubdef(f);
    subklass = (zend_class_entry*)msgdef2class(subdef);
  }

  upb_msgval msgval = upb_msg_get(self->msg, upb_fielddef_index(f),
                                  self->layout);
  tophpval(msgval, upb_fielddef_type(f), subklass, self->arena, return_value);
}

PHP_METHOD(Message, readOneof) {
  PROTO_LONG index;

  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l", &index) ==
      FAILURE) {
    return;
  }

  Message* self = UNBOX(Message, getThis());
  Message_readOneof(self, index, return_value);
}

static void Message_writeOneof(Message *self, PROTO_LONG index, zval *value) {
  const upb_fielddef* f = upb_msgdef_itof(self->msgdef, index);

  // Remove old reference to submsg.
  int field_index = upb_fielddef_index(f);
  uint32_t oneof_case = *upb_msg_oneofcase(
      self->msg, field_index, self->layout);
  if (oneof_case != 0) {
    const upb_fielddef* old_field = upb_msgdef_itof(self->msgdef, oneof_case);
    if (upb_fielddef_type(old_field) == UPB_TYPE_MESSAGE) {
      upb_msgval msgval = upb_msg_get(self->msg, upb_fielddef_index(old_field),
                                      self->layout);
      const upb_msg *old_msg = upb_msgval_getmsg(msgval);
      // PHP_OBJECT_DELREF(upb_msg_alloc(old_msg));
    }
  }

  upb_msgval msgval = tomsgval(value, upb_fielddef_type(f),
                               upb_msg_alloc(self->msg));
  upb_msg_set(self->msg, upb_fielddef_index(f), msgval, self->layout);
}

PHP_METHOD(Message, writeOneof) {
  PROTO_LONG index;
  zval* value;
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "lz", &index, &value) ==
      FAILURE) {
    return;
  }

  Message* self = UNBOX(Message, getThis());
  Message_writeOneof(self, index, value);
}

static const char* Message_whichOneof(Message* self, const char* oneof_name,
                                      PROTO_SIZE length) {
  const upb_oneofdef* oneof =
      upb_msgdef_ntoo(self->msgdef, oneof_name, length);

  // Get oneof case
  upb_oneof_iter i;
  const upb_fielddef* first_field;

  // Oneof is guaranteed to have at least one field. Get the first field.
  for(upb_oneof_begin(&i, oneof); !upb_oneof_done(&i); upb_oneof_next(&i)) {
    first_field = upb_oneof_iter_field(&i);
    break;
  }
  int field_index = upb_fielddef_index(first_field);
  uint32_t oneof_case = *upb_msg_oneofcase(
      self->msg, field_index, self->layout);

  if (oneof_case == 0) {
    return "";
  }

  const upb_fielddef* field = upb_oneofdef_itof(oneof, oneof_case);
  return upb_fielddef_name(field);
}

PHP_METHOD(Message, whichOneof) {
  char* oneof_name;
  PROTO_SIZE length;

  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &oneof_name,
                            &length) == FAILURE) {
    return;
  }

  Message* self = UNBOX(Message, getThis());
  const char* field_name = Message_whichOneof(self, oneof_name, length);
  PROTO_RETURN_STRINGL(field_name, strlen(field_name), 1);
}

// -----------------------------------------------------------------------------
// Well Known Types
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// Well Known Types Support
// -----------------------------------------------------------------------------

#define PROTO_FIELD_ACCESSORS(UPPER_CLASS, UPPER_FIELD, LOWER_FIELD)           \
  PHP_METHOD(UPPER_CLASS, get##UPPER_FIELD) {                                  \
    zval member;                                                               \
    PROTO_ZVAL_STRING(&member, LOWER_FIELD, 1);                                \
    PROTO_FAKE_SCOPE_BEGIN(UPPER_CLASS##_type);                                \
    zval* value = message_get_property_internal(getThis(), &member TSRMLS_CC); \
    PROTO_FAKE_SCOPE_END;                                                      \
    PROTO_RETVAL_ZVAL(value);                                                  \
  }                                                                            \
  PHP_METHOD(UPPER_CLASS, set##UPPER_FIELD) {                                  \
    zval* value = NULL;                                                        \
    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z", &value) ==       \
        FAILURE) {                                                             \
      return;                                                                  \
    }                                                                          \
    zval member;                                                               \
    PROTO_ZVAL_STRING(&member, LOWER_FIELD, 1);                                \
    PROTO_FAKE_SCOPE_BEGIN(UPPER_CLASS##_type);                                \
    message_set_property_internal(getThis(), &member, value TSRMLS_CC);        \
    PROTO_FAKE_SCOPE_END;                                                      \
    PROTO_RETVAL_ZVAL(getThis());                                              \
  }

#define PROTO_ONEOF_FIELD_ACCESSORS(CLASS, FIELD, FIELD_NUM)             \
  PHP_METHOD(CLASS, get##FIELD) {                                        \
    Message* self = UNBOX(Message, getThis());                           \
    PROTO_FAKE_SCOPE_BEGIN(CLASS##_type);                                \
    Message_readOneof(self, FIELD_NUM, return_value);                    \
    PROTO_FAKE_SCOPE_END;                                                \
  }                                                                      \
  PHP_METHOD(CLASS, set##FIELD) {                                        \
    zval* value = NULL;                                                  \
    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z", &value) == \
        FAILURE) {                                                       \
      return;                                                            \
    }                                                                    \
    Message* self = UNBOX(Message, getThis());                           \
    PROTO_FAKE_SCOPE_BEGIN(CLASS##_type);                                \
    Message_writeOneof(self, FIELD_NUM, value);                          \
    PROTO_FAKE_SCOPE_END;                                                \
  }

#define PROTO_ONEOF_ACCESSORS(UPPER_CLASS, UPPER_FIELD, LOWER_FIELD)  \
  PHP_METHOD(UPPER_CLASS, get##UPPER_FIELD) {                         \
    Message* self = UNBOX(Message, getThis());                        \
    const char* field_name = Message_whichOneof(self, LOWER_FIELD,    \
                                                strlen(LOWER_FIELD)); \
    PROTO_RETURN_STRINGL(field_name, strlen(field_name), 1);          \
  }

// -----------------------------------------------------------------------------
// Well Known Types Files
// -----------------------------------------------------------------------------

// Forward declare file init functions
static void init_file_any(TSRMLS_D);
static void init_file_api(TSRMLS_D);
static void init_file_duration(TSRMLS_D);
static void init_file_field_mask(TSRMLS_D);
static void init_file_empty(TSRMLS_D);
static void init_file_source_context(TSRMLS_D);
static void init_file_struct(TSRMLS_D);
static void init_file_timestamp(TSRMLS_D);
static void init_file_type(TSRMLS_D);
static void init_file_wrappers(TSRMLS_D);

static void hex_to_binary(const char* hex, char** binary, int* binary_len) {
  int i;
  int hex_len = strlen(hex);
  *binary_len = hex_len / 2;
  *binary = ALLOC_N(char, *binary_len);
  for (i = 0; i < *binary_len; i++) {
    char value = 0;
    if (hex[i * 2] >= '0' && hex[i * 2] <= '9') {
      value += (hex[i * 2] - '0') * 16;
    } else {
      value += (hex[i * 2] - 'a' + 10) * 16;
    }
    if (hex[i * 2 + 1] >= '0' && hex[i * 2 + 1] <= '9') {
      value += hex[i * 2 + 1] - '0';
    } else {
      value += hex[i * 2 + 1] - 'a' + 10;
    }
    (*binary)[i] = value;
  }
}

// Define file init functions
static void init_file_any(TSRMLS_D) {
  static bool is_initialized = false;
  if (is_initialized) return;
  init_generated_pool_once(TSRMLS_C);
  const char* generated_file =
      "0acd010a19676f6f676c652f70726f746f6275662f616e792e70726f746f"
      "120f676f6f676c652e70726f746f62756622260a03416e7912100a087479"
      "70655f75726c180120012809120d0a0576616c756518022001280c426f0a"
      "13636f6d2e676f6f676c652e70726f746f6275664208416e7950726f746f"
      "50015a256769746875622e636f6d2f676f6c616e672f70726f746f627566"
      "2f7074797065732f616e79a20203475042aa021e476f6f676c652e50726f"
      "746f6275662e57656c6c4b6e6f776e5479706573620670726f746f33";
  char* binary;
  int binary_len;
  hex_to_binary(generated_file, &binary, &binary_len);
  InternalDescriptorPool *pool =
      UNBOX_HASHTABLE_VALUE(InternalDescriptorPool,
                            internal_generated_pool);
  InternalDescriptorPool_add_generated_file(pool, binary, binary_len);
  FREE(binary);
  is_initialized = true;
}

static void init_file_api(TSRMLS_D) {
  static bool is_initialized = false;
  if (is_initialized) return;
  init_file_source_context(TSRMLS_C);
  init_file_type(TSRMLS_C);
  init_generated_pool_once(TSRMLS_C);
  const char* generated_file =
      "0aee050a19676f6f676c652f70726f746f6275662f6170692e70726f746f"
      "120f676f6f676c652e70726f746f6275661a24676f6f676c652f70726f74"
      "6f6275662f736f757263655f636f6e746578742e70726f746f1a1a676f6f"
      "676c652f70726f746f6275662f747970652e70726f746f2281020a034170"
      "69120c0a046e616d6518012001280912280a076d6574686f647318022003"
      "280b32172e676f6f676c652e70726f746f6275662e4d6574686f6412280a"
      "076f7074696f6e7318032003280b32172e676f6f676c652e70726f746f62"
      "75662e4f7074696f6e120f0a0776657273696f6e18042001280912360a0e"
      "736f757263655f636f6e7465787418052001280b321e2e676f6f676c652e"
      "70726f746f6275662e536f75726365436f6e7465787412260a066d697869"
      "6e7318062003280b32162e676f6f676c652e70726f746f6275662e4d6978"
      "696e12270a0673796e74617818072001280e32172e676f6f676c652e7072"
      "6f746f6275662e53796e74617822d5010a064d6574686f64120c0a046e61"
      "6d6518012001280912180a10726571756573745f747970655f75726c1802"
      "2001280912190a11726571756573745f73747265616d696e671803200128"
      "0812190a11726573706f6e73655f747970655f75726c180420012809121a"
      "0a12726573706f6e73655f73747265616d696e6718052001280812280a07"
      "6f7074696f6e7318062003280b32172e676f6f676c652e70726f746f6275"
      "662e4f7074696f6e12270a0673796e74617818072001280e32172e676f6f"
      "676c652e70726f746f6275662e53796e74617822230a054d6978696e120c"
      "0a046e616d65180120012809120c0a04726f6f7418022001280942750a13"
      "636f6d2e676f6f676c652e70726f746f627566420841706950726f746f50"
      "015a2b676f6f676c652e676f6c616e672e6f72672f67656e70726f746f2f"
      "70726f746f6275662f6170693b617069a20203475042aa021e476f6f676c"
      "652e50726f746f6275662e57656c6c4b6e6f776e5479706573620670726f"
      "746f33";
  char* binary;
  int binary_len;
  hex_to_binary(generated_file, &binary, &binary_len);
  InternalDescriptorPool *pool =
      UNBOX_HASHTABLE_VALUE(InternalDescriptorPool,
                            internal_generated_pool);
  InternalDescriptorPool_add_generated_file(pool, binary, binary_len);
  FREE(binary);
  is_initialized = true;
}

static void init_file_duration(TSRMLS_D) {
  static bool is_initialized = false;
  if (is_initialized) return;
  init_generated_pool_once(TSRMLS_C);
  const char* generated_file =
      "0ae3010a1e676f6f676c652f70726f746f6275662f6475726174696f6e2e"
      "70726f746f120f676f6f676c652e70726f746f627566222a0a0844757261"
      "74696f6e120f0a077365636f6e6473180120012803120d0a056e616e6f73"
      "180220012805427c0a13636f6d2e676f6f676c652e70726f746f62756642"
      "0d4475726174696f6e50726f746f50015a2a6769746875622e636f6d2f67"
      "6f6c616e672f70726f746f6275662f7074797065732f6475726174696f6e"
      "f80101a20203475042aa021e476f6f676c652e50726f746f6275662e5765"
      "6c6c4b6e6f776e5479706573620670726f746f33";
  char* binary;
  int binary_len;
  hex_to_binary(generated_file, &binary, &binary_len);
  InternalDescriptorPool *pool =
      UNBOX_HASHTABLE_VALUE(InternalDescriptorPool,
                            internal_generated_pool);
  InternalDescriptorPool_add_generated_file(pool, binary, binary_len);
  FREE(binary);
  is_initialized = true;
}

static void init_file_field_mask(TSRMLS_D) {
  static bool is_initialized = false;
  if (is_initialized) return;
  init_generated_pool_once(TSRMLS_C);
  const char* generated_file =
      "0ae3010a20676f6f676c652f70726f746f6275662f6669656c645f6d6173"
      "6b2e70726f746f120f676f6f676c652e70726f746f627566221a0a094669"
      "656c644d61736b120d0a0570617468731801200328094289010a13636f6d"
      "2e676f6f676c652e70726f746f627566420e4669656c644d61736b50726f"
      "746f50015a39676f6f676c652e676f6c616e672e6f72672f67656e70726f"
      "746f2f70726f746f6275662f6669656c645f6d61736b3b6669656c645f6d"
      "61736ba20203475042aa021e476f6f676c652e50726f746f6275662e5765"
      "6c6c4b6e6f776e5479706573620670726f746f33";
  char* binary;
  int binary_len;
  hex_to_binary(generated_file, &binary, &binary_len);
  InternalDescriptorPool *pool =
      UNBOX_HASHTABLE_VALUE(InternalDescriptorPool,
                            internal_generated_pool);
  InternalDescriptorPool_add_generated_file(pool, binary, binary_len);
  FREE(binary);
  is_initialized = true;
}

static void init_file_empty(TSRMLS_D) {
  static bool is_initialized = false;
  if (is_initialized) return;
  init_generated_pool_once(TSRMLS_C);
  const char* generated_file =
      "0ab7010a1b676f6f676c652f70726f746f6275662f656d7074792e70726f"
      "746f120f676f6f676c652e70726f746f62756622070a05456d7074794276"
      "0a13636f6d2e676f6f676c652e70726f746f627566420a456d7074795072"
      "6f746f50015a276769746875622e636f6d2f676f6c616e672f70726f746f"
      "6275662f7074797065732f656d707479f80101a20203475042aa021e476f"
      "6f676c652e50726f746f6275662e57656c6c4b6e6f776e54797065736206"
      "70726f746f33";
  char* binary;
  int binary_len;
  hex_to_binary(generated_file, &binary, &binary_len);
  InternalDescriptorPool *pool =
      UNBOX_HASHTABLE_VALUE(InternalDescriptorPool,
                            internal_generated_pool);
  InternalDescriptorPool_add_generated_file(pool, binary, binary_len);
  FREE(binary);
  is_initialized = true;
}

static void init_file_source_context(TSRMLS_D) {
  static bool is_initialized = false;
  if (is_initialized) return;
  init_generated_pool_once(TSRMLS_C);
  const char* generated_file =
      "0afb010a24676f6f676c652f70726f746f6275662f736f757263655f636f"
      "6e746578742e70726f746f120f676f6f676c652e70726f746f6275662222"
      "0a0d536f75726365436f6e7465787412110a0966696c655f6e616d651801"
      "200128094295010a13636f6d2e676f6f676c652e70726f746f6275664212"
      "536f75726365436f6e7465787450726f746f50015a41676f6f676c652e67"
      "6f6c616e672e6f72672f67656e70726f746f2f70726f746f6275662f736f"
      "757263655f636f6e746578743b736f757263655f636f6e74657874a20203"
      "475042aa021e476f6f676c652e50726f746f6275662e57656c6c4b6e6f77"
      "6e5479706573620670726f746f33";
  char* binary;
  int binary_len;
  hex_to_binary(generated_file, &binary, &binary_len);
  InternalDescriptorPool *pool =
      UNBOX_HASHTABLE_VALUE(InternalDescriptorPool,
                            internal_generated_pool);
  InternalDescriptorPool_add_generated_file(pool, binary, binary_len);
  FREE(binary);
  is_initialized = true;
}

static void init_file_struct(TSRMLS_D) {
  static bool is_initialized = false;
  if (is_initialized) return;
  init_generated_pool_once(TSRMLS_C);
  const char* generated_file =
      "0a81050a1c676f6f676c652f70726f746f6275662f7374727563742e7072"
      "6f746f120f676f6f676c652e70726f746f6275662284010a065374727563"
      "7412330a066669656c647318012003280b32232e676f6f676c652e70726f"
      "746f6275662e5374727563742e4669656c6473456e7472791a450a0b4669"
      "656c6473456e747279120b0a036b657918012001280912250a0576616c75"
      "6518022001280b32162e676f6f676c652e70726f746f6275662e56616c75"
      "653a02380122ea010a0556616c756512300a0a6e756c6c5f76616c756518"
      "012001280e321a2e676f6f676c652e70726f746f6275662e4e756c6c5661"
      "6c7565480012160a0c6e756d6265725f76616c7565180220012801480012"
      "160a0c737472696e675f76616c7565180320012809480012140a0a626f6f"
      "6c5f76616c75651804200128084800122f0a0c7374727563745f76616c75"
      "6518052001280b32172e676f6f676c652e70726f746f6275662e53747275"
      "6374480012300a0a6c6973745f76616c756518062001280b321a2e676f6f"
      "676c652e70726f746f6275662e4c69737456616c7565480042060a046b69"
      "6e6422330a094c69737456616c756512260a0676616c7565731801200328"
      "0b32162e676f6f676c652e70726f746f6275662e56616c75652a1b0a094e"
      "756c6c56616c7565120e0a0a4e554c4c5f56414c554510004281010a1363"
      "6f6d2e676f6f676c652e70726f746f627566420b53747275637450726f74"
      "6f50015a316769746875622e636f6d2f676f6c616e672f70726f746f6275"
      "662f7074797065732f7374727563743b7374727563747062f80101a20203"
      "475042aa021e476f6f676c652e50726f746f6275662e57656c6c4b6e6f77"
      "6e5479706573620670726f746f33";
  char* binary;
  int binary_len;
  hex_to_binary(generated_file, &binary, &binary_len);
  InternalDescriptorPool *pool =
      UNBOX_HASHTABLE_VALUE(InternalDescriptorPool,
                            internal_generated_pool);
  InternalDescriptorPool_add_generated_file(pool, binary, binary_len);
  FREE(binary);
  is_initialized = true;
}

static void init_file_timestamp(TSRMLS_D) {
  static bool is_initialized = false;
  if (is_initialized) return;
  init_generated_pool_once(TSRMLS_C);
  const char* generated_file =
      "0ae7010a1f676f6f676c652f70726f746f6275662f74696d657374616d70"
      "2e70726f746f120f676f6f676c652e70726f746f627566222b0a0954696d"
      "657374616d70120f0a077365636f6e6473180120012803120d0a056e616e"
      "6f73180220012805427e0a13636f6d2e676f6f676c652e70726f746f6275"
      "66420e54696d657374616d7050726f746f50015a2b6769746875622e636f"
      "6d2f676f6c616e672f70726f746f6275662f7074797065732f74696d6573"
      "74616d70f80101a20203475042aa021e476f6f676c652e50726f746f6275"
      "662e57656c6c4b6e6f776e5479706573620670726f746f33";
  char* binary;
  int binary_len;
  hex_to_binary(generated_file, &binary, &binary_len);
  InternalDescriptorPool *pool =
      UNBOX_HASHTABLE_VALUE(InternalDescriptorPool,
                            internal_generated_pool);
  InternalDescriptorPool_add_generated_file(pool, binary, binary_len);
  FREE(binary);
  is_initialized = true;
}

static void init_file_type(TSRMLS_D) {
  static bool is_initialized = false;
  if (is_initialized) return;
  init_file_any(TSRMLS_C);
  init_file_source_context(TSRMLS_C);
  init_generated_pool_once(TSRMLS_C);
  const char* generated_file =
      "0aba0c0a1a676f6f676c652f70726f746f6275662f747970652e70726f74"
      "6f120f676f6f676c652e70726f746f6275661a19676f6f676c652f70726f"
      "746f6275662f616e792e70726f746f1a24676f6f676c652f70726f746f62"
      "75662f736f757263655f636f6e746578742e70726f746f22d7010a045479"
      "7065120c0a046e616d6518012001280912260a066669656c647318022003"
      "280b32162e676f6f676c652e70726f746f6275662e4669656c64120e0a06"
      "6f6e656f667318032003280912280a076f7074696f6e7318042003280b32"
      "172e676f6f676c652e70726f746f6275662e4f7074696f6e12360a0e736f"
      "757263655f636f6e7465787418052001280b321e2e676f6f676c652e7072"
      "6f746f6275662e536f75726365436f6e7465787412270a0673796e746178"
      "18062001280e32172e676f6f676c652e70726f746f6275662e53796e7461"
      "7822d5050a054669656c6412290a046b696e6418012001280e321b2e676f"
      "6f676c652e70726f746f6275662e4669656c642e4b696e6412370a0b6361"
      "7264696e616c69747918022001280e32222e676f6f676c652e70726f746f"
      "6275662e4669656c642e43617264696e616c697479120e0a066e756d6265"
      "72180320012805120c0a046e616d6518042001280912100a08747970655f"
      "75726c18062001280912130a0b6f6e656f665f696e646578180720012805"
      "120e0a067061636b656418082001280812280a076f7074696f6e73180920"
      "03280b32172e676f6f676c652e70726f746f6275662e4f7074696f6e1211"
      "0a096a736f6e5f6e616d65180a2001280912150a0d64656661756c745f76"
      "616c7565180b2001280922c8020a044b696e6412100a0c545950455f554e"
      "4b4e4f574e1000120f0a0b545950455f444f55424c451001120e0a0a5459"
      "50455f464c4f41541002120e0a0a545950455f494e5436341003120f0a0b"
      "545950455f55494e5436341004120e0a0a545950455f494e543332100512"
      "100a0c545950455f46495845443634100612100a0c545950455f46495845"
      "4433321007120d0a09545950455f424f4f4c1008120f0a0b545950455f53"
      "5452494e471009120e0a0a545950455f47524f5550100a12100a0c545950"
      "455f4d455353414745100b120e0a0a545950455f4259544553100c120f0a"
      "0b545950455f55494e543332100d120d0a09545950455f454e554d100e12"
      "110a0d545950455f5346495845443332100f12110a0d545950455f534649"
      "58454436341010120f0a0b545950455f53494e5433321011120f0a0b5459"
      "50455f53494e543634101222740a0b43617264696e616c69747912170a13"
      "43415244494e414c4954595f554e4b4e4f574e100012180a144341524449"
      "4e414c4954595f4f5054494f4e414c100112180a1443415244494e414c49"
      "54595f5245515549524544100212180a1443415244494e414c4954595f52"
      "45504541544544100322ce010a04456e756d120c0a046e616d6518012001"
      "2809122d0a09656e756d76616c756518022003280b321a2e676f6f676c65"
      "2e70726f746f6275662e456e756d56616c756512280a076f7074696f6e73"
      "18032003280b32172e676f6f676c652e70726f746f6275662e4f7074696f"
      "6e12360a0e736f757263655f636f6e7465787418042001280b321e2e676f"
      "6f676c652e70726f746f6275662e536f75726365436f6e7465787412270a"
      "0673796e74617818052001280e32172e676f6f676c652e70726f746f6275"
      "662e53796e74617822530a09456e756d56616c7565120c0a046e616d6518"
      "0120012809120e0a066e756d62657218022001280512280a076f7074696f"
      "6e7318032003280b32172e676f6f676c652e70726f746f6275662e4f7074"
      "696f6e223b0a064f7074696f6e120c0a046e616d6518012001280912230a"
      "0576616c756518022001280b32142e676f6f676c652e70726f746f627566"
      "2e416e792a2e0a0653796e74617812110a0d53594e5441585f50524f544f"
      "32100012110a0d53594e5441585f50524f544f331001427d0a13636f6d2e"
      "676f6f676c652e70726f746f62756642095479706550726f746f50015a2f"
      "676f6f676c652e676f6c616e672e6f72672f67656e70726f746f2f70726f"
      "746f6275662f70747970653b7074797065f80101a20203475042aa021e47"
      "6f6f676c652e50726f746f6275662e57656c6c4b6e6f776e547970657362"
      "0670726f746f33";
  char* binary;
  int binary_len;
  hex_to_binary(generated_file, &binary, &binary_len);
  InternalDescriptorPool *pool =
      UNBOX_HASHTABLE_VALUE(InternalDescriptorPool,
                            internal_generated_pool);
  InternalDescriptorPool_add_generated_file(pool, binary, binary_len);
  FREE(binary);
  is_initialized = true;
}

static void init_file_wrappers(TSRMLS_D) {
  static bool is_initialized = false;
  if (is_initialized) return;
  init_generated_pool_once(TSRMLS_C);
  const char* generated_file =
      "0abf030a1e676f6f676c652f70726f746f6275662f77726170706572732e"
      "70726f746f120f676f6f676c652e70726f746f627566221c0a0b446f7562"
      "6c6556616c7565120d0a0576616c7565180120012801221b0a0a466c6f61"
      "7456616c7565120d0a0576616c7565180120012802221b0a0a496e743634"
      "56616c7565120d0a0576616c7565180120012803221c0a0b55496e743634"
      "56616c7565120d0a0576616c7565180120012804221b0a0a496e74333256"
      "616c7565120d0a0576616c7565180120012805221c0a0b55496e74333256"
      "616c7565120d0a0576616c756518012001280d221a0a09426f6f6c56616c"
      "7565120d0a0576616c7565180120012808221c0a0b537472696e6756616c"
      "7565120d0a0576616c7565180120012809221b0a0a427974657356616c75"
      "65120d0a0576616c756518012001280c427c0a13636f6d2e676f6f676c65"
      "2e70726f746f627566420d577261707065727350726f746f50015a2a6769"
      "746875622e636f6d2f676f6c616e672f70726f746f6275662f7074797065"
      "732f7772617070657273f80101a20203475042aa021e476f6f676c652e50"
      "726f746f6275662e57656c6c4b6e6f776e5479706573620670726f746f33";
  char* binary;
  int binary_len;
  hex_to_binary(generated_file, &binary, &binary_len);
  InternalDescriptorPool *pool =
      UNBOX_HASHTABLE_VALUE(InternalDescriptorPool,
                            internal_generated_pool);
  InternalDescriptorPool_add_generated_file(pool, binary, binary_len);
  FREE(binary);
  is_initialized = true;
}

// -----------------------------------------------------------------------------
// Define enum
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// Field_Cardinality
// -----------------------------------------------------------------------------

static zend_function_entry Field_Cardinality_methods[] = {
  {NULL, NULL, NULL}
};

// Init class entry.
PROTO_INIT_ENUMCLASS_START("Google\\Protobuf\\Field_Cardinality",
                           Field_Cardinality)
  zend_declare_class_constant_long(Field_Cardinality_type,
                                   "CARDINALITY_UNKNOWN", 19, 0 TSRMLS_CC);
  zend_declare_class_constant_long(Field_Cardinality_type,
                                   "CARDINALITY_OPTIONAL", 20, 1 TSRMLS_CC);
  zend_declare_class_constant_long(Field_Cardinality_type,
                                   "CARDINALITY_REQUIRED", 20, 2 TSRMLS_CC);
  zend_declare_class_constant_long(Field_Cardinality_type,
                                   "CARDINALITY_REPEATED", 20, 3 TSRMLS_CC);
PROTO_INIT_ENUMCLASS_END

// -----------------------------------------------------------------------------
// Field_Kind
// -----------------------------------------------------------------------------

static zend_function_entry Field_Kind_methods[] = {
  {NULL, NULL, NULL}
};

// Init class entry.
PROTO_INIT_ENUMCLASS_START("Google\\Protobuf\\Field_Kind", Field_Kind)
  zend_declare_class_constant_long(Field_Kind_type,
                                   "TYPE_UNKNOWN", 12, 0 TSRMLS_CC);
  zend_declare_class_constant_long(Field_Kind_type,
                                   "TYPE_DOUBLE", 11, 1 TSRMLS_CC);
  zend_declare_class_constant_long(Field_Kind_type,
                                   "TYPE_FLOAT", 10, 2 TSRMLS_CC);
  zend_declare_class_constant_long(Field_Kind_type,
                                   "TYPE_INT64", 10, 3 TSRMLS_CC);
  zend_declare_class_constant_long(Field_Kind_type,
                                   "TYPE_UINT64", 11, 4 TSRMLS_CC);
  zend_declare_class_constant_long(Field_Kind_type,
                                   "TYPE_INT32", 10, 5 TSRMLS_CC);
  zend_declare_class_constant_long(Field_Kind_type,
                                   "TYPE_FIXED64", 12, 6 TSRMLS_CC);
  zend_declare_class_constant_long(Field_Kind_type,
                                   "TYPE_FIXED32", 12, 7 TSRMLS_CC);
  zend_declare_class_constant_long(Field_Kind_type,
                                   "TYPE_BOOL", 9, 8 TSRMLS_CC);
  zend_declare_class_constant_long(Field_Kind_type,
                                   "TYPE_STRING", 11, 9 TSRMLS_CC);
  zend_declare_class_constant_long(Field_Kind_type,
                                   "TYPE_GROUP", 10, 10 TSRMLS_CC);
  zend_declare_class_constant_long(Field_Kind_type,
                                   "TYPE_MESSAGE", 12, 11 TSRMLS_CC);
  zend_declare_class_constant_long(Field_Kind_type,
                                   "TYPE_BYTES", 10, 12 TSRMLS_CC);
  zend_declare_class_constant_long(Field_Kind_type,
                                   "TYPE_UINT32", 11, 13 TSRMLS_CC);
  zend_declare_class_constant_long(Field_Kind_type,
                                   "TYPE_ENUM", 9, 14 TSRMLS_CC);
  zend_declare_class_constant_long(Field_Kind_type,
                                   "TYPE_SFIXED32", 13, 15 TSRMLS_CC);
  zend_declare_class_constant_long(Field_Kind_type,
                                   "TYPE_SFIXED64", 13, 16 TSRMLS_CC);
  zend_declare_class_constant_long(Field_Kind_type,
                                   "TYPE_SINT32", 11, 17 TSRMLS_CC);
  zend_declare_class_constant_long(Field_Kind_type,
                                   "TYPE_SINT64", 11, 18 TSRMLS_CC);
PROTO_INIT_ENUMCLASS_END

// -----------------------------------------------------------------------------
// NullValue
// -----------------------------------------------------------------------------

static zend_function_entry NullValue_methods[] = {
  {NULL, NULL, NULL}
};

// Init class entry.
PROTO_INIT_ENUMCLASS_START("Google\\Protobuf\\NullValue", NullValue)
  zend_declare_class_constant_long(NullValue_type,
                                   "NULL_VALUE", 10, 0 TSRMLS_CC);
PROTO_INIT_ENUMCLASS_END

// -----------------------------------------------------------------------------
// Syntax
// -----------------------------------------------------------------------------

static zend_function_entry Syntax_methods[] = {
  {NULL, NULL, NULL}
};

// Init class entry.
PROTO_INIT_ENUMCLASS_START("Google\\Protobuf\\Syntax", Syntax)
  zend_declare_class_constant_long(Syntax_type,
                                   "SYNTAX_PROTO2", 13, 0 TSRMLS_CC);
  zend_declare_class_constant_long(Syntax_type,
                                   "SYNTAX_PROTO3", 13, 1 TSRMLS_CC);
PROTO_INIT_ENUMCLASS_END

// -----------------------------------------------------------------------------
// Define message
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Any
// -----------------------------------------------------------------------------

PHP_METHOD(Any, __construct);
PHP_METHOD(Any, getTypeUrl);
PHP_METHOD(Any, setTypeUrl);
PHP_METHOD(Any, getValue);
PHP_METHOD(Any, setValue);
PHP_METHOD(Any, unpack);
PHP_METHOD(Any, pack);
PHP_METHOD(Any, is);

static  zend_function_entry Any_methods[] = {
  PHP_ME(Any, __construct, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(Any, getTypeUrl, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(Any, setTypeUrl, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(Any, getValue, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(Any, setValue, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(Any, pack,     NULL, ZEND_ACC_PUBLIC)
  PHP_ME(Any, unpack,   NULL, ZEND_ACC_PUBLIC)
  PHP_ME(Any, is,       NULL, ZEND_ACC_PUBLIC)
  {NULL, NULL, NULL}
};

PROTO_INIT_MSG_SUBCLASS_START("Google\\Protobuf\\Any", Any)
  zend_declare_property_string(Any_type, "type_url", strlen("type_url"),
                               "" ,ZEND_ACC_PRIVATE TSRMLS_CC);
  zend_declare_property_string(Any_type, "value", strlen("value"),
                               "" ,ZEND_ACC_PRIVATE TSRMLS_CC);
PROTO_INIT_MSG_SUBCLASS_END

static const char TYPE_URL_PREFIX[] = "type.googleapis.com/";

PHP_METHOD(Any, __construct) {
  init_file_any(TSRMLS_C);
  Message* intern = UNBOX(Message, getThis());
  const upb_msgdef* msgdef = class2msgdef(Any_type);
  Message___construct(intern, msgdef);
}

PROTO_FIELD_ACCESSORS(Any, TypeUrl, "type_url")
PROTO_FIELD_ACCESSORS(Any, Value,   "value")

PHP_METHOD(Any, unpack) {
  // Get type url.
  zval type_url_member;
  PROTO_ZVAL_STRING(&type_url_member, "type_url", 1);
  PROTO_FAKE_SCOPE_BEGIN(Any_type);
  zval* type_url_php = message_get_property_internal(
      getThis(), &type_url_member TSRMLS_CC);
  PROTO_FAKE_SCOPE_END;

  // Get fully-qualified name from type url.
  size_t url_prefix_len = strlen(TYPE_URL_PREFIX);
  const char* type_url = Z_STRVAL_P(type_url_php);
  size_t type_url_len = Z_STRLEN_P(type_url_php);

  if (url_prefix_len > type_url_len ||
      strncmp(TYPE_URL_PREFIX, type_url, url_prefix_len) != 0) {
    zend_throw_exception(
        NULL, "Type url needs to be type.googleapis.com/fully-qulified",
        0 TSRMLS_CC);
    return;
  }

  const char* fully_qualified_name = type_url + url_prefix_len;
  zend_class_entry* klass = name2class(std::string(
      fully_qualified_name, type_url_len - url_prefix_len));
  if (klass == NULL) {
    zend_throw_exception(
        NULL, "Specified message in any hasn't been added to descriptor pool",
        0 TSRMLS_CC);
    return;
  }
  ZVAL_OBJ(return_value, klass->create_object(klass TSRMLS_CC));
  Message* msg = UNBOX(Message, return_value);
  const upb_msgdef* msgdef = class2msgdef(klass);
  Message___construct(msg, msgdef);

  // Get value.
  zval value_member;
  PROTO_ZVAL_STRING(&value_member, "value", 1);
  PROTO_FAKE_SCOPE_RESTART(Any_type);
  zval* value = message_get_property_internal(
      getThis(), &value_member TSRMLS_CC);
  PROTO_FAKE_SCOPE_END;

  if (!Message_mergeFromString(msg, Z_STRVAL_P(value), Z_STRLEN_P(value))) {
    zend_throw_exception(
        NULL, "Payload data is invalid.",
        0 TSRMLS_CC);
  }
}

PHP_METHOD(Any, pack) {
  zval* val;

  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "o", &val) ==
      FAILURE) {
    return;
  }

  if (!instanceof_function(Z_OBJCE_P(val), Message_type TSRMLS_CC)) {
    zend_error(E_USER_ERROR, "Given value is not an instance of Message.");
    return;
  }

  // Set value by serialized data.
  zval data;
  stackenv se;
  stackenv_init(&se, "Error occurred during encoding: %s");
  size_t size;
  Message *payload_msg = UNBOX(Message, val);
  const char* payload = upb_encode2(payload_msg->msg, payload_msg->layout,
                                    &se.env, &size);
  PROTO_ZVAL_STRINGL(&data, payload, size, 1);
  stackenv_uninit(&se);

  zval member;
  PROTO_ZVAL_STRING(&member, "value", 1);

  PROTO_FAKE_SCOPE_BEGIN(Any_type);
  message_set_property_internal(getThis(), &member, &data TSRMLS_CC);
  PROTO_FAKE_SCOPE_END;

  // Set type url.
  const upb_msgdef* msgdef = class2msgdef(Z_OBJCE_P(val));
  const char* fully_qualified_name = upb_msgdef_fullname(msgdef);
  size_t type_url_len =
      strlen(TYPE_URL_PREFIX) + strlen(fully_qualified_name) + 1;
  char* type_url = ALLOC_N(char, type_url_len);
  sprintf(type_url, "%s%s", TYPE_URL_PREFIX, fully_qualified_name);
  zval type_url_php;
  PROTO_ZVAL_STRING(&type_url_php, type_url, 1);
  PROTO_ZVAL_STRING(&member, "type_url", 1);

  PROTO_FAKE_SCOPE_RESTART(Any_type);
  message_set_property_internal(getThis(), &member, &type_url_php TSRMLS_CC);
  PROTO_FAKE_SCOPE_END;
  FREE(type_url);
}

PHP_METHOD(Any, is) {
  zend_class_entry *klass = NULL;

  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "C", &klass) ==
      FAILURE) {
    return;
  }

  const upb_msgdef* msgdef = class2msgdef(klass);
  if (msgdef == NULL) {
    RETURN_BOOL(false);
  }

  // Create corresponded type url.
  const char* fully_qualified_name = upb_msgdef_fullname(msgdef);
  size_t type_url_len =
      strlen(TYPE_URL_PREFIX) + strlen(fully_qualified_name) + 1;
  char* type_url = ALLOC_N(char, type_url_len);
  sprintf(type_url, "%s%s", TYPE_URL_PREFIX, fully_qualified_name);

  // Fetch stored type url.
  zval member;
  PROTO_ZVAL_STRING(&member, "type_url", 1);
  PROTO_FAKE_SCOPE_BEGIN(Any_type);
  zval* value = message_get_property_internal(getThis(), &member TSRMLS_CC);
  PROTO_FAKE_SCOPE_END;

  // Compare two type url.
  bool is = strcmp(type_url, Z_STRVAL_P(value)) == 0;
  FREE(type_url);

  RETURN_BOOL(is);
}

// -----------------------------------------------------------------------------
// Duration
// -----------------------------------------------------------------------------

PHP_METHOD(Duration, __construct);
PHP_METHOD(Duration, getSeconds);
PHP_METHOD(Duration, setSeconds);
PHP_METHOD(Duration, getNanos);
PHP_METHOD(Duration, setNanos);

static  zend_function_entry Duration_methods[] = {
  PHP_ME(Duration, __construct, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(Duration, getSeconds, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(Duration, setSeconds, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(Duration, getNanos, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(Duration, setNanos, NULL, ZEND_ACC_PUBLIC)
  {NULL, NULL, NULL}
};

PROTO_INIT_MSG_SUBCLASS_START("Google\\Protobuf\\Duration", Duration)
  zend_declare_property_long(Duration_type, "seconds", strlen("seconds"),
                             0 ,ZEND_ACC_PRIVATE TSRMLS_CC);
  zend_declare_property_long(Duration_type, "nanos", strlen("nanos"),
                             0 ,ZEND_ACC_PRIVATE TSRMLS_CC);
PROTO_INIT_MSG_SUBCLASS_END

PHP_METHOD(Duration, __construct) {
  init_file_duration(TSRMLS_C);
  Message* intern = UNBOX(Message, getThis());
  const upb_msgdef* msgdef = class2msgdef(Duration_type);
  Message___construct(intern, msgdef);
}

PROTO_FIELD_ACCESSORS(Duration, Seconds, "seconds")
PROTO_FIELD_ACCESSORS(Duration, Nanos,   "nanos")

// -----------------------------------------------------------------------------
// Timestamp
// -----------------------------------------------------------------------------

PHP_METHOD(Timestamp, __construct);
PHP_METHOD(Timestamp, fromDateTime);
PHP_METHOD(Timestamp, toDateTime);
PHP_METHOD(Timestamp, getSeconds);
PHP_METHOD(Timestamp, setSeconds);
PHP_METHOD(Timestamp, getNanos);
PHP_METHOD(Timestamp, setNanos);

static  zend_function_entry Timestamp_methods[] = {
  PHP_ME(Timestamp, __construct, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(Timestamp, fromDateTime, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(Timestamp, toDateTime, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(Timestamp, getSeconds, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(Timestamp, setSeconds, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(Timestamp, getNanos, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(Timestamp, setNanos, NULL, ZEND_ACC_PUBLIC)
  {NULL, NULL, NULL}
};

PROTO_INIT_MSG_SUBCLASS_START("Google\\Protobuf\\Timestamp", Timestamp)
  zend_declare_property_long(Timestamp_type, "seconds", strlen("seconds"),
                             0 ,ZEND_ACC_PRIVATE TSRMLS_CC);
  zend_declare_property_long(Timestamp_type, "nanos", strlen("nanos"),
                             0 ,ZEND_ACC_PRIVATE TSRMLS_CC);
PROTO_INIT_MSG_SUBCLASS_END

PHP_METHOD(Timestamp, __construct) {
  init_file_timestamp(TSRMLS_C);
  Message* intern = UNBOX(Message, getThis());
  const upb_msgdef* msgdef = class2msgdef(Timestamp_type);
  Message___construct(intern, msgdef);
}

PROTO_FIELD_ACCESSORS(Timestamp, Seconds, "seconds")
PROTO_FIELD_ACCESSORS(Timestamp, Nanos,   "nanos")

PHP_METHOD(Timestamp, fromDateTime) {
  zval* datetime;
  zval member;

  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "O", &datetime,
                            php_date_get_date_ce()) == FAILURE) {
    return;
  }

  php_date_obj* dateobj = UNBOX(php_date_obj, datetime);
  if (!dateobj->time->sse_uptodate) {
    timelib_update_ts(dateobj->time, NULL);
  }

  int64_t timestamp = dateobj->time->sse;

  // Set seconds
  Message* self = UNBOX(Message, getThis());
  const upb_fielddef* field =
      upb_msgdef_ntofz(self->msgdef, "seconds");
  upb_msgval msgval = upb_msgval_int64(dateobj->time->sse);
  upb_msg_set(self->msg, upb_fielddef_index(field), msgval, self->layout);

  // Set nanos
  field = upb_msgdef_ntofz(self->msgdef, "nanos");
  msgval = upb_msgval_int32(0);
  upb_msg_set(self->msg, upb_fielddef_index(field), msgval, self->layout);
}

PHP_METHOD(Timestamp, toDateTime) {
  zval datetime;
  php_date_instantiate(php_date_get_date_ce(), &datetime TSRMLS_CC);
  php_date_obj* dateobj = UNBOX(php_date_obj, &datetime);

  // Get seconds
  Message* self = UNBOX(Message, getThis());
  const upb_fielddef* field =
      upb_msgdef_ntofz(self->msgdef, "seconds");
  upb_msgval msgval = upb_msg_get(self->msg, upb_fielddef_index(field),
                                  self->layout);
  int64_t seconds = upb_msgval_getint64(msgval);

  // Get nanos
  field = upb_msgdef_ntofz(self->msgdef, "nanos");
  msgval = upb_msg_get(self->msg, upb_fielddef_index(field),
                       self->layout);
  int32_t nanos = upb_msgval_getint32(msgval);

  // Get formated time string.
  char formated_time[50];
  time_t raw_time = seconds;
  struct tm *utc_time = gmtime(&raw_time);
  strftime(formated_time, sizeof(formated_time), "%Y-%m-%dT%H:%M:%SUTC",
           utc_time);

  if (!php_date_initialize(dateobj, formated_time, strlen(formated_time), NULL,
                           NULL, 0 TSRMLS_CC)) {
    zval_dtor(&datetime);
    RETURN_NULL();
  }

  zval* datetime_ptr = &datetime;
  PROTO_RETVAL_ZVAL(datetime_ptr);
}

// -----------------------------------------------------------------------------
// Api
// -----------------------------------------------------------------------------

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

static  zend_function_entry Api_methods[] = {
  PHP_ME(Api, __construct, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(Api, getName, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(Api, setName, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(Api, getMethods, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(Api, setMethods, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(Api, getOptions, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(Api, setOptions, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(Api, getVersion, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(Api, setVersion, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(Api, getSourceContext, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(Api, setSourceContext, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(Api, getMixins, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(Api, setMixins, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(Api, getSyntax, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(Api, setSyntax, NULL, ZEND_ACC_PUBLIC)
  {NULL, NULL, NULL}
};

PROTO_INIT_MSG_SUBCLASS_START("Google\\Protobuf\\Api", Api)
  zend_declare_property_null(Api_type, "name", strlen("name"),
                             ZEND_ACC_PRIVATE TSRMLS_CC);
  zend_declare_property_null(Api_type, "methods", strlen("methods"),
                             ZEND_ACC_PRIVATE TSRMLS_CC);
  zend_declare_property_null(Api_type, "options", strlen("options"),
                             ZEND_ACC_PRIVATE TSRMLS_CC);
  zend_declare_property_null(Api_type, "version", strlen("version"),
                             ZEND_ACC_PRIVATE TSRMLS_CC);
  zend_declare_property_null(Api_type, "source_context",
                             strlen("source_context"),
                             ZEND_ACC_PRIVATE TSRMLS_CC);
  zend_declare_property_null(Api_type, "mixins", strlen("mixins"),
                             ZEND_ACC_PRIVATE TSRMLS_CC);
  zend_declare_property_null(Api_type, "syntax", strlen("syntax"),
                             ZEND_ACC_PRIVATE TSRMLS_CC);
PROTO_INIT_MSG_SUBCLASS_END

PHP_METHOD(Api, __construct) {
  init_file_api(TSRMLS_C);
  Message* intern = UNBOX(Message, getThis());
  const upb_msgdef* msgdef = class2msgdef(Api_type);
  Message___construct(intern, msgdef);
}

PROTO_FIELD_ACCESSORS(Api, Name, "name")
PROTO_FIELD_ACCESSORS(Api, Methods, "methods")
PROTO_FIELD_ACCESSORS(Api, Options, "options")
PROTO_FIELD_ACCESSORS(Api, Version, "version")
PROTO_FIELD_ACCESSORS(Api, SourceContext, "source_context")
PROTO_FIELD_ACCESSORS(Api, Mixins, "mixins")
PROTO_FIELD_ACCESSORS(Api, Syntax, "syntax")

// -----------------------------------------------------------------------------
// BoolValue
// -----------------------------------------------------------------------------

PHP_METHOD(BoolValue, __construct);
PHP_METHOD(BoolValue, getValue);
PHP_METHOD(BoolValue, setValue);

static  zend_function_entry BoolValue_methods[] = {
  PHP_ME(BoolValue, __construct, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(BoolValue, getValue, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(BoolValue, setValue, NULL, ZEND_ACC_PUBLIC)
  {NULL, NULL, NULL}
};

PROTO_INIT_MSG_SUBCLASS_START("Google\\Protobuf\\BoolValue", BoolValue)
  zend_declare_property_null(BoolValue_type, "value", strlen("value"),
                             ZEND_ACC_PRIVATE TSRMLS_CC);
PROTO_INIT_MSG_SUBCLASS_END

PHP_METHOD(BoolValue, __construct) {
  init_file_wrappers(TSRMLS_C);
  Message* intern = UNBOX(Message, getThis());
  const upb_msgdef* msgdef = class2msgdef(BoolValue_type);
  Message___construct(intern, msgdef);
}

PROTO_FIELD_ACCESSORS(BoolValue, Value, "value")

// -----------------------------------------------------------------------------
// BytesValue
// -----------------------------------------------------------------------------

PHP_METHOD(BytesValue, __construct);
PHP_METHOD(BytesValue, getValue);
PHP_METHOD(BytesValue, setValue);

static  zend_function_entry BytesValue_methods[] = {
  PHP_ME(BytesValue, __construct, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(BytesValue, getValue, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(BytesValue, setValue, NULL, ZEND_ACC_PUBLIC)
  {NULL, NULL, NULL}
};

PROTO_INIT_MSG_SUBCLASS_START("Google\\Protobuf\\BytesValue", BytesValue)
  zend_declare_property_null(BytesValue_type, "value", strlen("value"),
                             ZEND_ACC_PRIVATE TSRMLS_CC);
PROTO_INIT_MSG_SUBCLASS_END

PHP_METHOD(BytesValue, __construct) {
  init_file_wrappers(TSRMLS_C);
  Message* intern = UNBOX(Message, getThis());
  const upb_msgdef* msgdef = class2msgdef(BytesValue_type);
  Message___construct(intern, msgdef);
}

PROTO_FIELD_ACCESSORS(BytesValue, Value, "value")

// -----------------------------------------------------------------------------
// DoubleValue
// -----------------------------------------------------------------------------

PHP_METHOD(DoubleValue, __construct);
PHP_METHOD(DoubleValue, getValue);
PHP_METHOD(DoubleValue, setValue);

static  zend_function_entry DoubleValue_methods[] = {
  PHP_ME(DoubleValue, __construct, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(DoubleValue, getValue, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(DoubleValue, setValue, NULL, ZEND_ACC_PUBLIC)
  {NULL, NULL, NULL}
};

PROTO_INIT_MSG_SUBCLASS_START("Google\\Protobuf\\DoubleValue", DoubleValue)
  zend_declare_property_null(DoubleValue_type, "value", strlen("value"),
                             ZEND_ACC_PRIVATE TSRMLS_CC);
PROTO_INIT_MSG_SUBCLASS_END

PHP_METHOD(DoubleValue, __construct) {
  init_file_wrappers(TSRMLS_C);
  Message* intern = UNBOX(Message, getThis());
  const upb_msgdef* msgdef = class2msgdef(DoubleValue_type);
  Message___construct(intern, msgdef);
}

PROTO_FIELD_ACCESSORS(DoubleValue, Value, "value")

// -----------------------------------------------------------------------------
// Enum
// -----------------------------------------------------------------------------

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

static  zend_function_entry Enum_methods[] = {
  PHP_ME(Enum, __construct, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(Enum, getName, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(Enum, setName, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(Enum, getEnumvalue, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(Enum, setEnumvalue, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(Enum, getOptions, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(Enum, setOptions, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(Enum, getSourceContext, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(Enum, setSourceContext, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(Enum, getSyntax, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(Enum, setSyntax, NULL, ZEND_ACC_PUBLIC)
  {NULL, NULL, NULL}
};

PROTO_INIT_MSG_SUBCLASS_START("Google\\Protobuf\\Enum", Enum)
  zend_declare_property_null(Enum_type, "name", strlen("name"),
                             ZEND_ACC_PRIVATE TSRMLS_CC);
  zend_declare_property_null(Enum_type, "enumvalue", strlen("enumvalue"),
                             ZEND_ACC_PRIVATE TSRMLS_CC);
  zend_declare_property_null(Enum_type, "options", strlen("options"),
                             ZEND_ACC_PRIVATE TSRMLS_CC);
  zend_declare_property_null(Enum_type, "source_context", strlen("source_context"),
                             ZEND_ACC_PRIVATE TSRMLS_CC);
  zend_declare_property_null(Enum_type, "syntax", strlen("syntax"),
                             ZEND_ACC_PRIVATE TSRMLS_CC);
PROTO_INIT_MSG_SUBCLASS_END

PHP_METHOD(Enum, __construct) {
  init_file_type(TSRMLS_C);
  Message* intern = UNBOX(Message, getThis());
  const upb_msgdef* msgdef = class2msgdef(Enum_type);
  Message___construct(intern, msgdef);
}

PROTO_FIELD_ACCESSORS(Enum, Name, "name")
PROTO_FIELD_ACCESSORS(Enum, Enumvalue, "enumvalue")
PROTO_FIELD_ACCESSORS(Enum, Options, "options")
PROTO_FIELD_ACCESSORS(Enum, SourceContext, "source_context")
PROTO_FIELD_ACCESSORS(Enum, Syntax, "syntax")

// -----------------------------------------------------------------------------
// EnumValue
// -----------------------------------------------------------------------------

PHP_METHOD(EnumValue, __construct);
PHP_METHOD(EnumValue, getName);
PHP_METHOD(EnumValue, setName);
PHP_METHOD(EnumValue, getNumber);
PHP_METHOD(EnumValue, setNumber);
PHP_METHOD(EnumValue, getOptions);
PHP_METHOD(EnumValue, setOptions);

static  zend_function_entry EnumValue_methods[] = {
  PHP_ME(EnumValue, __construct, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(EnumValue, getName, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(EnumValue, setName, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(EnumValue, getNumber, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(EnumValue, setNumber, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(EnumValue, getOptions, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(EnumValue, setOptions, NULL, ZEND_ACC_PUBLIC)
  {NULL, NULL, NULL}
};

PROTO_INIT_MSG_SUBCLASS_START("Google\\Protobuf\\EnumValue", EnumValue)
  zend_declare_property_null(EnumValue_type, "name", strlen("name"),
                             ZEND_ACC_PRIVATE TSRMLS_CC);
  zend_declare_property_null(EnumValue_type, "number", strlen("number"),
                             ZEND_ACC_PRIVATE TSRMLS_CC);
  zend_declare_property_null(EnumValue_type, "options", strlen("options"),
                             ZEND_ACC_PRIVATE TSRMLS_CC);
PROTO_INIT_MSG_SUBCLASS_END

PHP_METHOD(EnumValue, __construct) {
  init_file_type(TSRMLS_C);
  Message* intern = UNBOX(Message, getThis());
  const upb_msgdef* msgdef = class2msgdef(EnumValue_type);
  Message___construct(intern, msgdef);
}

PROTO_FIELD_ACCESSORS(EnumValue, Name, "name")
PROTO_FIELD_ACCESSORS(EnumValue, Number, "number")
PROTO_FIELD_ACCESSORS(EnumValue, Options, "options")

// -----------------------------------------------------------------------------
// FieldMask
// -----------------------------------------------------------------------------

PHP_METHOD(FieldMask, __construct);
PHP_METHOD(FieldMask, getPaths);
PHP_METHOD(FieldMask, setPaths);

static  zend_function_entry FieldMask_methods[] = {
  PHP_ME(FieldMask, __construct, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(FieldMask, getPaths, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(FieldMask, setPaths, NULL, ZEND_ACC_PUBLIC)
  {NULL, NULL, NULL}
};

PROTO_INIT_MSG_SUBCLASS_START("Google\\Protobuf\\FieldMask", FieldMask)
  zend_declare_property_null(FieldMask_type, "paths", strlen("paths"),
                             ZEND_ACC_PRIVATE TSRMLS_CC);
PROTO_INIT_MSG_SUBCLASS_END

PHP_METHOD(FieldMask, __construct) {
  init_file_field_mask(TSRMLS_C);
  Message* intern = UNBOX(Message, getThis());
  const upb_msgdef* msgdef = class2msgdef(FieldMask_type);
  Message___construct(intern, msgdef);
}

PROTO_FIELD_ACCESSORS(FieldMask, Paths, "paths")

// -----------------------------------------------------------------------------
// Field
// -----------------------------------------------------------------------------

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

static  zend_function_entry Field_methods[] = {
  PHP_ME(Field, __construct, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(Field, getKind, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(Field, setKind, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(Field, getCardinality, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(Field, setCardinality, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(Field, getNumber, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(Field, setNumber, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(Field, getName, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(Field, setName, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(Field, getTypeUrl, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(Field, setTypeUrl, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(Field, getOneofIndex, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(Field, setOneofIndex, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(Field, getPacked, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(Field, setPacked, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(Field, getOptions, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(Field, setOptions, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(Field, getJsonName, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(Field, setJsonName, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(Field, getDefaultValue, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(Field, setDefaultValue, NULL, ZEND_ACC_PUBLIC)
  {NULL, NULL, NULL}
};

PROTO_INIT_MSG_SUBCLASS_START("Google\\Protobuf\\Field", Field)
  zend_declare_property_null(Field_type, "kind", strlen("kind"),
                             ZEND_ACC_PRIVATE TSRMLS_CC);
  zend_declare_property_null(Field_type, "cardinality",
                             strlen("cardinality"),
                             ZEND_ACC_PRIVATE TSRMLS_CC);
  zend_declare_property_null(Field_type, "number", strlen("number"),
                             ZEND_ACC_PRIVATE TSRMLS_CC);
  zend_declare_property_null(Field_type, "name", strlen("name"),
                             ZEND_ACC_PRIVATE TSRMLS_CC);
  zend_declare_property_null(Field_type, "type_url", strlen("type_url"),
                             ZEND_ACC_PRIVATE TSRMLS_CC);
  zend_declare_property_null(Field_type, "oneof_index",
                             strlen("oneof_index"),
                             ZEND_ACC_PRIVATE TSRMLS_CC);
  zend_declare_property_null(Field_type, "packed", strlen("packed"),
                             ZEND_ACC_PRIVATE TSRMLS_CC);
  zend_declare_property_null(Field_type, "options", strlen("options"),
                             ZEND_ACC_PRIVATE TSRMLS_CC);
  zend_declare_property_null(Field_type, "json_name", strlen("json_name"),
                             ZEND_ACC_PRIVATE TSRMLS_CC);
  zend_declare_property_null(Field_type, "default_value",
                             strlen("default_value"),
                             ZEND_ACC_PRIVATE TSRMLS_CC);
PROTO_INIT_MSG_SUBCLASS_END

PHP_METHOD(Field, __construct) {
  init_file_type(TSRMLS_C);
  Message* intern = UNBOX(Message, getThis());
  const upb_msgdef* msgdef = class2msgdef(Field_type);
  Message___construct(intern, msgdef);
}

PROTO_FIELD_ACCESSORS(Field, Kind, "kind")
PROTO_FIELD_ACCESSORS(Field, Cardinality, "cardinality")
PROTO_FIELD_ACCESSORS(Field, Number, "number")
PROTO_FIELD_ACCESSORS(Field, Name, "name")
PROTO_FIELD_ACCESSORS(Field, TypeUrl, "type_url")
PROTO_FIELD_ACCESSORS(Field, OneofIndex, "oneof_index")
PROTO_FIELD_ACCESSORS(Field, Packed, "packed")
PROTO_FIELD_ACCESSORS(Field, Options, "options")
PROTO_FIELD_ACCESSORS(Field, JsonName, "json_name")
PROTO_FIELD_ACCESSORS(Field, DefaultValue, "default_value")

// -----------------------------------------------------------------------------
// FloatValue
// -----------------------------------------------------------------------------

PHP_METHOD(FloatValue, __construct);
PHP_METHOD(FloatValue, getValue);
PHP_METHOD(FloatValue, setValue);

static  zend_function_entry FloatValue_methods[] = {
  PHP_ME(FloatValue, __construct, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(FloatValue, getValue, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(FloatValue, setValue, NULL, ZEND_ACC_PUBLIC)
  {NULL, NULL, NULL}
};

PROTO_INIT_MSG_SUBCLASS_START("Google\\Protobuf\\FloatValue", FloatValue)
  zend_declare_property_null(FloatValue_type, "value", strlen("value"),
                             ZEND_ACC_PRIVATE TSRMLS_CC);
PROTO_INIT_MSG_SUBCLASS_END

PHP_METHOD(FloatValue, __construct) {
  init_file_wrappers(TSRMLS_C);
  Message* intern = UNBOX(Message, getThis());
  const upb_msgdef* msgdef = class2msgdef(FloatValue_type);
  Message___construct(intern, msgdef);
}

PROTO_FIELD_ACCESSORS(FloatValue, Value, "value")

// -----------------------------------------------------------------------------
// GPBEmpty
// -----------------------------------------------------------------------------

PHP_METHOD(Empty, __construct);

static  zend_function_entry Empty_methods[] = {
  PHP_ME(Empty, __construct, NULL, ZEND_ACC_PUBLIC)
  {NULL, NULL, NULL}
};

PROTO_INIT_MSG_SUBCLASS_START("Google\\Protobuf\\GPBEmpty", Empty)
PROTO_INIT_MSG_SUBCLASS_END

PHP_METHOD(Empty, __construct) {
  init_file_empty(TSRMLS_C);
  Message* intern = UNBOX(Message, getThis());
  const upb_msgdef* msgdef = class2msgdef(Empty_type);
  Message___construct(intern, msgdef);
}

// -----------------------------------------------------------------------------
// Int32Value
// -----------------------------------------------------------------------------

PHP_METHOD(Int32Value, __construct);
PHP_METHOD(Int32Value, getValue);
PHP_METHOD(Int32Value, setValue);

static  zend_function_entry Int32Value_methods[] = {
  PHP_ME(Int32Value, __construct, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(Int32Value, getValue, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(Int32Value, setValue, NULL, ZEND_ACC_PUBLIC)
  {NULL, NULL, NULL}
};

PROTO_INIT_MSG_SUBCLASS_START("Google\\Protobuf\\Int32Value", Int32Value)
  zend_declare_property_null(Int32Value_type, "value", strlen("value"),
                             ZEND_ACC_PRIVATE TSRMLS_CC);
PROTO_INIT_MSG_SUBCLASS_END

PHP_METHOD(Int32Value, __construct) {
  init_file_wrappers(TSRMLS_C);
  Message* intern = UNBOX(Message, getThis());
  const upb_msgdef* msgdef = class2msgdef(Int32Value_type);
  Message___construct(intern, msgdef);
}

PROTO_FIELD_ACCESSORS(Int32Value, Value, "value")

// -----------------------------------------------------------------------------
// Int64Value
// -----------------------------------------------------------------------------

PHP_METHOD(Int64Value, __construct);
PHP_METHOD(Int64Value, getValue);
PHP_METHOD(Int64Value, setValue);

static  zend_function_entry Int64Value_methods[] = {
  PHP_ME(Int64Value, __construct, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(Int64Value, getValue, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(Int64Value, setValue, NULL, ZEND_ACC_PUBLIC)
  {NULL, NULL, NULL}
};

PROTO_INIT_MSG_SUBCLASS_START("Google\\Protobuf\\Int64Value", Int64Value)
  zend_declare_property_null(Int64Value_type, "value", strlen("value"),
                             ZEND_ACC_PRIVATE TSRMLS_CC);
PROTO_INIT_MSG_SUBCLASS_END

PHP_METHOD(Int64Value, __construct) {
  init_file_wrappers(TSRMLS_C);
  Message* intern = UNBOX(Message, getThis());
  const upb_msgdef* msgdef = class2msgdef(Int64Value_type);
  Message___construct(intern, msgdef);
}

PROTO_FIELD_ACCESSORS(Int64Value, Value, "value")

// -----------------------------------------------------------------------------
// ListValue
// -----------------------------------------------------------------------------

PHP_METHOD(ListValue, __construct);
PHP_METHOD(ListValue, getValues);
PHP_METHOD(ListValue, setValues);

static  zend_function_entry ListValue_methods[] = {
  PHP_ME(ListValue, __construct, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(ListValue, getValues, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(ListValue, setValues, NULL, ZEND_ACC_PUBLIC)
  {NULL, NULL, NULL}
};

PROTO_INIT_MSG_SUBCLASS_START("Google\\Protobuf\\ListValue", ListValue)
  zend_declare_property_null(ListValue_type, "values", strlen("values"),
                             ZEND_ACC_PRIVATE TSRMLS_CC);
PROTO_INIT_MSG_SUBCLASS_END

PHP_METHOD(ListValue, __construct) {
  init_file_struct(TSRMLS_C);
  Message* intern = UNBOX(Message, getThis());
  const upb_msgdef* msgdef = class2msgdef(ListValue_type);
  Message___construct(intern, msgdef);
}

PROTO_FIELD_ACCESSORS(ListValue, Values, "values")

// -----------------------------------------------------------------------------
// Method
// -----------------------------------------------------------------------------

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

static  zend_function_entry Method_methods[] = {
  PHP_ME(Method, __construct, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(Method, getName, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(Method, setName, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(Method, getRequestTypeUrl, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(Method, setRequestTypeUrl, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(Method, getRequestStreaming, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(Method, setRequestStreaming, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(Method, getResponseTypeUrl, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(Method, setResponseTypeUrl, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(Method, getResponseStreaming, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(Method, setResponseStreaming, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(Method, getOptions, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(Method, setOptions, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(Method, getSyntax, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(Method, setSyntax, NULL, ZEND_ACC_PUBLIC)
  {NULL, NULL, NULL}
};

PROTO_INIT_MSG_SUBCLASS_START("Google\\Protobuf\\Method", Method)
  zend_declare_property_null(Method_type, "name", strlen("name"),
                             ZEND_ACC_PRIVATE TSRMLS_CC);
  zend_declare_property_null(Method_type, "request_type_url",
                             strlen("request_type_url"),
                             ZEND_ACC_PRIVATE TSRMLS_CC);
  zend_declare_property_null(Method_type, "request_streaming",
                             strlen("request_streaming"),
                             ZEND_ACC_PRIVATE TSRMLS_CC);
  zend_declare_property_null(Method_type, "response_type_url",
                             strlen("response_type_url"),
                             ZEND_ACC_PRIVATE TSRMLS_CC);
  zend_declare_property_null(Method_type, "response_streaming",
                             strlen("response_streaming"),
                             ZEND_ACC_PRIVATE TSRMLS_CC);
  zend_declare_property_null(Method_type, "options", strlen("options"),
                             ZEND_ACC_PRIVATE TSRMLS_CC);
  zend_declare_property_null(Method_type, "syntax", strlen("syntax"),
                             ZEND_ACC_PRIVATE TSRMLS_CC);
PROTO_INIT_MSG_SUBCLASS_END

PHP_METHOD(Method, __construct) {
  init_file_api(TSRMLS_C);
  Message* intern = UNBOX(Message, getThis());
  const upb_msgdef* msgdef = class2msgdef(Method_type);
  Message___construct(intern, msgdef);
}

PROTO_FIELD_ACCESSORS(Method, Name, "name")
PROTO_FIELD_ACCESSORS(Method, RequestTypeUrl, "request_type_url")
PROTO_FIELD_ACCESSORS(Method, RequestStreaming, "request_streaming")
PROTO_FIELD_ACCESSORS(Method, ResponseTypeUrl, "response_type_url")
PROTO_FIELD_ACCESSORS(Method, ResponseStreaming, "response_streaming")
PROTO_FIELD_ACCESSORS(Method, Options, "options")
PROTO_FIELD_ACCESSORS(Method, Syntax, "syntax")

// -----------------------------------------------------------------------------
// Mixin
// -----------------------------------------------------------------------------

PHP_METHOD(Mixin, __construct);
PHP_METHOD(Mixin, getName);
PHP_METHOD(Mixin, setName);
PHP_METHOD(Mixin, getRoot);
PHP_METHOD(Mixin, setRoot);

static  zend_function_entry Mixin_methods[] = {
  PHP_ME(Mixin, __construct, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(Mixin, getName, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(Mixin, setName, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(Mixin, getRoot, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(Mixin, setRoot, NULL, ZEND_ACC_PUBLIC)
  {NULL, NULL, NULL}
};

PROTO_INIT_MSG_SUBCLASS_START("Google\\Protobuf\\Mixin", Mixin)
  zend_declare_property_null(Mixin_type, "name", strlen("name"),
                             ZEND_ACC_PRIVATE TSRMLS_CC);
  zend_declare_property_null(Mixin_type, "root", strlen("root"),
                             ZEND_ACC_PRIVATE TSRMLS_CC);
PROTO_INIT_MSG_SUBCLASS_END

PHP_METHOD(Mixin, __construct) {
  init_file_api(TSRMLS_C);
  Message* intern = UNBOX(Message, getThis());
  const upb_msgdef* msgdef = class2msgdef(Mixin_type);
  Message___construct(intern, msgdef);
}

PROTO_FIELD_ACCESSORS(Mixin, Name, "name")
PROTO_FIELD_ACCESSORS(Mixin, Root, "root")

// -----------------------------------------------------------------------------
// Option
// -----------------------------------------------------------------------------

PHP_METHOD(Option, __construct);
PHP_METHOD(Option, getName);
PHP_METHOD(Option, setName);
PHP_METHOD(Option, getValue);
PHP_METHOD(Option, setValue);

static  zend_function_entry Option_methods[] = {
  PHP_ME(Option, __construct, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(Option, getName, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(Option, setName, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(Option, getValue, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(Option, setValue, NULL, ZEND_ACC_PUBLIC)
  {NULL, NULL, NULL}
};

PROTO_INIT_MSG_SUBCLASS_START("Google\\Protobuf\\Option", Option)
  zend_declare_property_null(Option_type, "name", strlen("name"),
                             ZEND_ACC_PRIVATE TSRMLS_CC);
  zend_declare_property_null(Option_type, "value", strlen("value"),
                             ZEND_ACC_PRIVATE TSRMLS_CC);
PROTO_INIT_MSG_SUBCLASS_END

PHP_METHOD(Option, __construct) {
  init_file_type(TSRMLS_C);
  Message* intern = UNBOX(Message, getThis());
  const upb_msgdef* msgdef = class2msgdef(Option_type);
  Message___construct(intern, msgdef);
}

PROTO_FIELD_ACCESSORS(Option, Name, "name")
PROTO_FIELD_ACCESSORS(Option, Value, "value")

// -----------------------------------------------------------------------------
// SourceContext
// -----------------------------------------------------------------------------

PHP_METHOD(SourceContext, __construct);
PHP_METHOD(SourceContext, getFileName);
PHP_METHOD(SourceContext, setFileName);

static  zend_function_entry SourceContext_methods[] = {
  PHP_ME(SourceContext, __construct, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(SourceContext, getFileName, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(SourceContext, setFileName, NULL, ZEND_ACC_PUBLIC)
  {NULL, NULL, NULL}
};

PROTO_INIT_MSG_SUBCLASS_START("Google\\Protobuf\\SourceContext", SourceContext)
  zend_declare_property_null(SourceContext_type, "file_name",
                             strlen("file_name"),
                             ZEND_ACC_PRIVATE TSRMLS_CC);
PROTO_INIT_MSG_SUBCLASS_END

PHP_METHOD(SourceContext, __construct) {
  init_file_source_context(TSRMLS_C);
  Message* intern = UNBOX(Message, getThis());
  const upb_msgdef* msgdef = class2msgdef(SourceContext_type);
  Message___construct(intern, msgdef);
}

PROTO_FIELD_ACCESSORS(SourceContext, FileName, "file_name")

// -----------------------------------------------------------------------------
// StringValue
// -----------------------------------------------------------------------------

PHP_METHOD(StringValue, __construct);
PHP_METHOD(StringValue, getValue);
PHP_METHOD(StringValue, setValue);

static  zend_function_entry StringValue_methods[] = {
  PHP_ME(StringValue, __construct, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(StringValue, getValue, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(StringValue, setValue, NULL, ZEND_ACC_PUBLIC)
  {NULL, NULL, NULL}
};

PROTO_INIT_MSG_SUBCLASS_START("Google\\Protobuf\\StringValue", StringValue)
  zend_declare_property_null(StringValue_type, "value", strlen("value"),
                             ZEND_ACC_PRIVATE TSRMLS_CC);
PROTO_INIT_MSG_SUBCLASS_END

PHP_METHOD(StringValue, __construct) {
  init_file_wrappers(TSRMLS_C);
  Message* intern = UNBOX(Message, getThis());
  const upb_msgdef* msgdef = class2msgdef(StringValue_type);
  Message___construct(intern, msgdef);
}

PROTO_FIELD_ACCESSORS(StringValue, Value, "value")

// -----------------------------------------------------------------------------
// Struct
// -----------------------------------------------------------------------------

PHP_METHOD(Struct, __construct);
PHP_METHOD(Struct, getFields);
PHP_METHOD(Struct, setFields);

static  zend_function_entry Struct_methods[] = {
  PHP_ME(Struct, __construct, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(Struct, getFields, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(Struct, setFields, NULL, ZEND_ACC_PUBLIC)
  {NULL, NULL, NULL}
};

PROTO_INIT_MSG_SUBCLASS_START("Google\\Protobuf\\Struct", Struct)
  zend_declare_property_null(Struct_type, "fields", strlen("fields"),
                             ZEND_ACC_PRIVATE TSRMLS_CC);
PROTO_INIT_MSG_SUBCLASS_END

PHP_METHOD(Struct, __construct) {
  init_file_struct(TSRMLS_C);
  Message* intern = UNBOX(Message, getThis());
  const upb_msgdef* msgdef = class2msgdef(Struct_type);
  Message___construct(intern, msgdef);
}

PROTO_FIELD_ACCESSORS(Struct, Fields, "fields")

// -----------------------------------------------------------------------------
// Type
// -----------------------------------------------------------------------------

PHP_METHOD(GPBType, __construct);
PHP_METHOD(GPBType, getName);
PHP_METHOD(GPBType, setName);
PHP_METHOD(GPBType, getFields);
PHP_METHOD(GPBType, setFields);
PHP_METHOD(GPBType, getOneofs);
PHP_METHOD(GPBType, setOneofs);
PHP_METHOD(GPBType, getOptions);
PHP_METHOD(GPBType, setOptions);
PHP_METHOD(GPBType, getSourceContext);
PHP_METHOD(GPBType, setSourceContext);
PHP_METHOD(GPBType, getSyntax);
PHP_METHOD(GPBType, setSyntax);

static  zend_function_entry GPBType_methods[] = {
  PHP_ME(GPBType, __construct, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(GPBType, getName, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(GPBType, setName, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(GPBType, getFields, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(GPBType, setFields, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(GPBType, getOneofs, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(GPBType, setOneofs, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(GPBType, getOptions, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(GPBType, setOptions, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(GPBType, getSourceContext, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(GPBType, setSourceContext, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(GPBType, getSyntax, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(GPBType, setSyntax, NULL, ZEND_ACC_PUBLIC)
  {NULL, NULL, NULL}
};

PROTO_INIT_MSG_SUBCLASS_START("Google\\Protobuf\\Type", GPBType)
  zend_declare_property_null(GPBType_type, "name", strlen("name"),
                             ZEND_ACC_PRIVATE TSRMLS_CC);
  zend_declare_property_null(GPBType_type, "fields", strlen("fields"),
                             ZEND_ACC_PRIVATE TSRMLS_CC);
  zend_declare_property_null(GPBType_type, "oneofs", strlen("oneofs"),
                             ZEND_ACC_PRIVATE TSRMLS_CC);
  zend_declare_property_null(GPBType_type, "options", strlen("options"),
                             ZEND_ACC_PRIVATE TSRMLS_CC);
  zend_declare_property_null(GPBType_type, "source_context",
                             strlen("source_context"),
                             ZEND_ACC_PRIVATE TSRMLS_CC);
  zend_declare_property_null(GPBType_type, "syntax", strlen("syntax"),
                             ZEND_ACC_PRIVATE TSRMLS_CC);
PROTO_INIT_MSG_SUBCLASS_END

PHP_METHOD(GPBType, __construct) {
  init_file_type(TSRMLS_C);
  Message* intern = UNBOX(Message, getThis());
  const upb_msgdef* msgdef = class2msgdef(GPBType_type);
  Message___construct(intern, msgdef);
}

PROTO_FIELD_ACCESSORS(GPBType, Name, "name")
PROTO_FIELD_ACCESSORS(GPBType, Fields, "fields")
PROTO_FIELD_ACCESSORS(GPBType, Oneofs, "oneofs")
PROTO_FIELD_ACCESSORS(GPBType, Options, "options")
PROTO_FIELD_ACCESSORS(GPBType, SourceContext, "source_context")
PROTO_FIELD_ACCESSORS(GPBType, Syntax, "syntax")

// -----------------------------------------------------------------------------
// UInt32Value
// -----------------------------------------------------------------------------

PHP_METHOD(UInt32Value, __construct);
PHP_METHOD(UInt32Value, getValue);
PHP_METHOD(UInt32Value, setValue);

static  zend_function_entry UInt32Value_methods[] = {
  PHP_ME(UInt32Value, __construct, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(UInt32Value, getValue, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(UInt32Value, setValue, NULL, ZEND_ACC_PUBLIC)
  {NULL, NULL, NULL}
};

PROTO_INIT_MSG_SUBCLASS_START("Google\\Protobuf\\UInt32Value", UInt32Value)
  zend_declare_property_null(UInt32Value_type, "value", strlen("value"),
                             ZEND_ACC_PRIVATE TSRMLS_CC);
PROTO_INIT_MSG_SUBCLASS_END

PHP_METHOD(UInt32Value, __construct) {
  init_file_wrappers(TSRMLS_C);
  Message* intern = UNBOX(Message, getThis());
  const upb_msgdef* msgdef = class2msgdef(UInt32Value_type);
  Message___construct(intern, msgdef);
}

PROTO_FIELD_ACCESSORS(UInt32Value, Value, "value")

// -----------------------------------------------------------------------------
// UInt64Value
// -----------------------------------------------------------------------------

PHP_METHOD(UInt64Value, __construct);
PHP_METHOD(UInt64Value, getValue);
PHP_METHOD(UInt64Value, setValue);

static  zend_function_entry UInt64Value_methods[] = {
  PHP_ME(UInt64Value, __construct, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(UInt64Value, getValue, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(UInt64Value, setValue, NULL, ZEND_ACC_PUBLIC)
  {NULL, NULL, NULL}
};

PROTO_INIT_MSG_SUBCLASS_START("Google\\Protobuf\\UInt64Value", UInt64Value)
  zend_declare_property_null(UInt64Value_type, "value", strlen("value"),
                             ZEND_ACC_PRIVATE TSRMLS_CC);
PROTO_INIT_MSG_SUBCLASS_END

PHP_METHOD(UInt64Value, __construct) {
  init_file_wrappers(TSRMLS_C);
  Message* intern = UNBOX(Message, getThis());
  const upb_msgdef* msgdef = class2msgdef(UInt64Value_type);
  Message___construct(intern, msgdef);
}

PROTO_FIELD_ACCESSORS(UInt64Value, Value, "value")

// -----------------------------------------------------------------------------
// Value
// -----------------------------------------------------------------------------

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

static zend_function_entry Value_methods[] = {
  PHP_ME(Value, __construct, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(Value, getNullValue, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(Value, setNullValue, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(Value, getNumberValue, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(Value, setNumberValue, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(Value, getStringValue, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(Value, setStringValue, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(Value, getBoolValue, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(Value, setBoolValue, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(Value, getStructValue, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(Value, setStructValue, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(Value, getListValue, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(Value, setListValue, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(Value, getKind, NULL, ZEND_ACC_PUBLIC)
  {NULL, NULL, NULL}
};

PROTO_INIT_MSG_SUBCLASS_START("Google\\Protobuf\\Value", Value)
  zend_declare_property_null(Value_type, "kind", strlen("kind"),
                             ZEND_ACC_PRIVATE TSRMLS_CC);
PROTO_INIT_MSG_SUBCLASS_END

PHP_METHOD(Value, __construct) {
  init_file_struct(TSRMLS_C);
  Message* intern = UNBOX(Message, getThis());
  const upb_msgdef* msgdef = class2msgdef(Value_type);
  Message___construct(intern, msgdef);
}

PROTO_ONEOF_FIELD_ACCESSORS(Value, NullValue, 1)
PROTO_ONEOF_FIELD_ACCESSORS(Value, NumberValue, 2)
PROTO_ONEOF_FIELD_ACCESSORS(Value, StringValue, 3)
PROTO_ONEOF_FIELD_ACCESSORS(Value, BoolValue, 4)
PROTO_ONEOF_FIELD_ACCESSORS(Value, StructValue, 5)
PROTO_ONEOF_FIELD_ACCESSORS(Value, ListValue, 6)
PROTO_ONEOF_ACCESSORS(Value, Kind, "kind")
