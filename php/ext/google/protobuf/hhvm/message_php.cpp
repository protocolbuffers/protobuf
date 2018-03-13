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
      char *mem = (char*)upb_gmalloc(Z_STRLEN_P(value) + 1);
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
        Message_wrap(intern, const_cast<upb_msg*>(msg), subdef);
      }
      return;
    }
    default:
      break;
  }
}

static void message_set_property_internal(zval* object, zval* member,
                                          zval* value TSRMLS_DC) {
  Message* self = UNBOX(Message, object);
  const upb_fielddef* f = upb_msgdef_ntofz(self->msgdef, Z_STRVAL_P(member));
  assert(f != NULL);
  int field_index = upb_fielddef_index(f);
  upb_fieldtype_t type = upb_fielddef_type(f);
  zval* cached_value = NULL;

  if (upb_fielddef_isseq(f) || upb_fielddef_ismap(f)) {
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
    SEPARATE_ZVAL_IF_NOT_REF(OBJ_PROP(Z_OBJ_P(object), property_info->offset));
#endif

#if PHP_MAJOR_VERSION < 7
    REPLACE_ZVAL_VALUE(OBJ_PROP(Z_OBJ_P(object), property_info->offset),
                       value, 1);
#else
    zval_ptr_dtor(OBJ_PROP(Z_OBJ_P(object), property_info->offset));
    ZVAL_ZVAL(OBJ_PROP(Z_OBJ_P(object), property_info->offset),
                       value, 1, 0);
#endif
  }

  if (upb_fielddef_ismap(f)) {
    MapField *map = UNBOX(MapField, value);
    upb_msgval msgval;
    upb_msgval_setmap(&msgval, map->map);
    upb_msg_set(self->msg, field_index, msgval, self->layout);
  } else if (upb_fielddef_isseq(f)) {
    RepeatedField *arr = UNBOX(RepeatedField, value);
    upb_msgval msgval;
    upb_msgval_setarr(&msgval, arr->array);
    upb_msg_set(self->msg, field_index, msgval, self->layout);
  } else {
    upb_msgval msgval = tomsgval(value, type, upb_msg_alloc(self->msg));
    upb_msg_set(self->msg, field_index, msgval, self->layout);
  }
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
  SEPARATE_ZVAL_IF_NOT_REF(OBJ_PROP(Z_OBJ_P(object), property_info->offset));
#endif

  zval* retval = CACHED_VALUE_PTR_TO_ZVAL_PTR(
    OBJ_PROP(Z_OBJ_P(object), property_info->offset));

//   // Find return value
//   zend_op_array* op_array = CG(active_op_array);
//   zval* retval = (op_array->opcodes[op_array->last]).result.var;

  upb_msgval msgval = upb_msg_get(self->msg, field_index, self->layout);

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
      klass = const_cast<zend_class_entry*>(msgdef2class(subdef));
    }

    const upb_map *map = upb_msgval_getmap(msgval);
    if (map == NULL) {
      MapField___construct(intern, 
                           upb_fielddef_descriptortype(key_fielddef),
                           upb_fielddef_descriptortype(value_fielddef),
                           klass);
      upb_msg_set(self->msg, field_index,
                  upb_msgval_map(intern->map), self->layout);
    } else {
      MapField_wrap(intern, const_cast<upb_map*>(map), klass);
    }
  } else if (upb_fielddef_isseq(f)) {
    if (Z_TYPE_P(retval) == IS_NULL) {
      ZVAL_OBJ(retval, RepeatedField_type->create_object(
          RepeatedField_type TSRMLS_CC));
#if PHP_MAJOR_VERSION < 7
       Z_SET_ISREF_P(retval);
#endif
    }
    RepeatedField* intern = UNBOX(RepeatedField, retval);

    zend_class_entry* klass = NULL;
    if (upb_fielddef_issubmsg(f)) {
      const upb_msgdef *subdef = upb_fielddef_msgsubdef(f);
      klass = const_cast<zend_class_entry*>(msgdef2class(subdef));
    }

    const upb_array *arr = upb_msgval_getarr(msgval);
    if (arr == NULL) {
      RepeatedField___construct(intern, upb_fielddef_descriptortype(f), klass);
      upb_msg_set(self->msg, field_index,
                  upb_msgval_arr(intern->array), self->layout);
    } else {
      RepeatedField_wrap(intern, const_cast<upb_array*>(arr), klass);
    }
  } else {
    zend_class_entry *subklass = NULL;
    if (upb_fielddef_issubmsg(f)) {
      const upb_msgdef *subdef = upb_fielddef_msgsubdef(f);
      subklass = const_cast<zend_class_entry*>(msgdef2class(subdef));
    }
    tophpval(msgval, type, subklass, retval);
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
PHP_METHOD(Message, serializeToString);
PHP_METHOD(Message, mergeFromString);
PHP_METHOD(Message, writeOneof);
PHP_METHOD(Message, readOneof);
PHP_METHOD(Message, whichOneof);

static zend_function_entry Message_methods[] = {
  PHP_ME(Message, __construct, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(Message, serializeToString, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(Message, mergeFromString, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(Message, writeOneof, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(Message, readOneof, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(Message, whichOneof, NULL, ZEND_ACC_PUBLIC)
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

PHP_METHOD(Message, serializeToString) {
  Message* intern = UNBOX(Message, getThis());
  size_t size;
  const char* data = Message_serializeToString(intern, &size);
  PROTO_RETURN_STRINGL(data, size, 1);
}

PHP_METHOD(Message, mergeFromString) {
  Message* intern = UNBOX(Message, getThis());
  char *data = NULL;
  PROTO_SIZE size;

  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &data, &size) ==
      FAILURE) {
    return;
  }
  Message_mergeFromString(intern, data, size);
}

PHP_METHOD(Message, readOneof) {
  PROTO_LONG index;

  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l", &index) ==
      FAILURE) {
    return;
  }

  Message* self = UNBOX(Message, getThis());

  const upb_fielddef* f = upb_msgdef_itof(self->msgdef, index);

  zend_class_entry *subklass = NULL;
  if (upb_fielddef_issubmsg(f)) {
    const upb_msgdef *subdef = upb_fielddef_msgsubdef(f);
    subklass = const_cast<zend_class_entry*>(msgdef2class(subdef));
  }

  upb_msgval msgval = upb_msg_get(self->msg, upb_fielddef_index(f),
                                  self->layout);
  tophpval(msgval, upb_fielddef_type(f), subklass, return_value);
}

PHP_METHOD(Message, writeOneof) {
  PROTO_LONG index;
  zval* value;
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "lz", &index, &value) ==
      FAILURE) {
    return;
  }

  Message* self = UNBOX(Message, getThis());

  const upb_fielddef* f = upb_msgdef_itof(self->msgdef, index);

  upb_msgval msgval = tomsgval(value, upb_fielddef_type(f),
                               upb_msg_alloc(self->msg));
  upb_msg_set(self->msg, upb_fielddef_index(f), msgval, self->layout);
}

PHP_METHOD(Message, whichOneof) {
  char* oneof_name;
  PROTO_SIZE length;

  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &oneof_name,
                            &length) == FAILURE) {
    return;
  }

  Message* self = UNBOX(Message, getThis());

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
    PROTO_RETURN_STRINGL("", 0, 1);
  }

  const upb_fielddef* field = upb_oneofdef_itof(oneof, oneof_case);
  const char* field_name = upb_fielddef_name(field);
  PROTO_RETURN_STRINGL(field_name, strlen(field_name), 1);
}
