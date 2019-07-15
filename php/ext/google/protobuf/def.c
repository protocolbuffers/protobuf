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

#include <php.h>
#include <Zend/zend_exceptions.h>

#include "protobuf.h"
#include "builtin_descriptors.inc"

// Forward declare.
static void descriptor_init_c_instance(Descriptor* intern TSRMLS_DC);
static void descriptor_free_c(Descriptor* object TSRMLS_DC);

static void field_descriptor_init_c_instance(FieldDescriptor* intern TSRMLS_DC);
static void field_descriptor_free_c(FieldDescriptor* object TSRMLS_DC);

static void enum_descriptor_init_c_instance(EnumDescriptor* intern TSRMLS_DC);
static void enum_descriptor_free_c(EnumDescriptor* object TSRMLS_DC);

static void enum_value_descriptor_init_c_instance(
    EnumValueDescriptor *intern TSRMLS_DC);
static void enum_value_descriptor_free_c(EnumValueDescriptor *object TSRMLS_DC);

static void descriptor_pool_free_c(DescriptorPool* object TSRMLS_DC);
static void descriptor_pool_init_c_instance(DescriptorPool* pool TSRMLS_DC);

static void internal_descriptor_pool_free_c(
    InternalDescriptorPool *object TSRMLS_DC);
static void internal_descriptor_pool_init_c_instance(
    InternalDescriptorPool *pool TSRMLS_DC);

static void oneof_descriptor_free_c(Oneof* object TSRMLS_DC);
static void oneof_descriptor_init_c_instance(Oneof* pool TSRMLS_DC);

// -----------------------------------------------------------------------------
// Common Utilities
// -----------------------------------------------------------------------------

static void check_upb_status(const upb_status* status, const char* msg) {
  if (!upb_ok(status)) {
    zend_error(E_ERROR, "%s: %s\n", msg, upb_status_errmsg(status));
  }
}

// Camel-case the field name and append "Entry" for generated map entry name.
// e.g. map<KeyType, ValueType> foo_map => FooMapEntry
static void append_map_entry_name(char *result, const char *field_name,
                                  int pos) {
  bool cap_next = true;
  int i;

  for (i = 0; i < strlen(field_name); ++i) {
    if (field_name[i] == '_') {
      cap_next = true;
    } else if (cap_next) {
      // Note: Do not use ctype.h due to locales.
      if ('a' <= field_name[i] && field_name[i] <= 'z') {
        result[pos++] = field_name[i] - 'a' + 'A';
      } else {
        result[pos++] = field_name[i];
      }
      cap_next = false;
    } else {
      result[pos++] = field_name[i];
    }
  }
  strcat(result, "Entry");
}

// -----------------------------------------------------------------------------
// GPBType
// -----------------------------------------------------------------------------

zend_class_entry* gpb_type_type;

static zend_function_entry gpb_type_methods[] = {
  ZEND_FE_END
};

void gpb_type_init(TSRMLS_D) {
  zend_class_entry class_type;
  INIT_CLASS_ENTRY(class_type, "Google\\Protobuf\\Internal\\GPBType",
                   gpb_type_methods);
  gpb_type_type = zend_register_internal_class(&class_type TSRMLS_CC);
  zend_declare_class_constant_long(gpb_type_type, STR("DOUBLE"),  1 TSRMLS_CC);
  zend_declare_class_constant_long(gpb_type_type, STR("FLOAT"),   2 TSRMLS_CC);
  zend_declare_class_constant_long(gpb_type_type, STR("INT64"),   3 TSRMLS_CC);
  zend_declare_class_constant_long(gpb_type_type, STR("UINT64"),  4 TSRMLS_CC);
  zend_declare_class_constant_long(gpb_type_type, STR("INT32"),   5 TSRMLS_CC);
  zend_declare_class_constant_long(gpb_type_type, STR("FIXED64"), 6 TSRMLS_CC);
  zend_declare_class_constant_long(gpb_type_type, STR("FIXED32"), 7 TSRMLS_CC);
  zend_declare_class_constant_long(gpb_type_type, STR("BOOL"),    8 TSRMLS_CC);
  zend_declare_class_constant_long(gpb_type_type, STR("STRING"),  9 TSRMLS_CC);
  zend_declare_class_constant_long(gpb_type_type, STR("GROUP"),   10 TSRMLS_CC);
  zend_declare_class_constant_long(gpb_type_type, STR("MESSAGE"), 11 TSRMLS_CC);
  zend_declare_class_constant_long(gpb_type_type, STR("BYTES"),   12 TSRMLS_CC);
  zend_declare_class_constant_long(gpb_type_type, STR("UINT32"),  13 TSRMLS_CC);
  zend_declare_class_constant_long(gpb_type_type, STR("ENUM"),    14 TSRMLS_CC);
  zend_declare_class_constant_long(gpb_type_type, STR("SFIXED32"),
                                   15 TSRMLS_CC);
  zend_declare_class_constant_long(gpb_type_type, STR("SFIXED64"),
                                   16 TSRMLS_CC);
  zend_declare_class_constant_long(gpb_type_type, STR("SINT32"), 17 TSRMLS_CC);
  zend_declare_class_constant_long(gpb_type_type, STR("SINT64"), 18 TSRMLS_CC);
}

// -----------------------------------------------------------------------------
// Descriptor
// -----------------------------------------------------------------------------

static zend_function_entry descriptor_methods[] = {
  PHP_ME(Descriptor, getClass, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(Descriptor, getFullName, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(Descriptor, getField, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(Descriptor, getFieldCount, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(Descriptor, getOneofDecl, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(Descriptor, getOneofDeclCount, NULL, ZEND_ACC_PUBLIC)
  ZEND_FE_END
};

DEFINE_CLASS(Descriptor, descriptor, "Google\\Protobuf\\Descriptor");

static void descriptor_free_c(Descriptor *self TSRMLS_DC) {
  if (self->layout) {
    free_layout(self->layout);
  }
}

static void descriptor_init_c_instance(Descriptor *desc TSRMLS_DC) {
  desc->msgdef = NULL;
  desc->layout = NULL;
  desc->klass = NULL;
}

PHP_METHOD(Descriptor, getClass) {
  Descriptor *intern = UNBOX(Descriptor, getThis());
#if PHP_MAJOR_VERSION < 7
  const char* classname = intern->klass->name;
#else
  const char* classname = ZSTR_VAL(intern->klass->name);
#endif
  PHP_PROTO_RETVAL_STRINGL(classname, strlen(classname), 1);
}

PHP_METHOD(Descriptor, getFullName) {
  Descriptor *intern = UNBOX(Descriptor, getThis());
  const char* fullname = upb_msgdef_fullname(intern->msgdef);
  PHP_PROTO_RETVAL_STRINGL(fullname, strlen(fullname), 1);
}

PHP_METHOD(Descriptor, getField) {
  long index;
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l", &index) ==
      FAILURE) {
    zend_error(E_USER_ERROR, "Expect integer for index.\n");
    return;
  }

  Descriptor *intern = UNBOX(Descriptor, getThis());
  int field_num = upb_msgdef_numfields(intern->msgdef);
  if (index < 0 || index >= field_num) {
    zend_error(E_USER_ERROR, "Cannot get element at %ld.\n", index);
    return;
  }

  upb_msg_field_iter iter;
  int i;
  for(upb_msg_field_begin(&iter, intern->msgdef), i = 0;
      !upb_msg_field_done(&iter) && i < index;
      upb_msg_field_next(&iter), i++);
  const upb_fielddef *field = upb_msg_iter_field(&iter);

  PHP_PROTO_HASHTABLE_VALUE field_hashtable_value = get_def_obj(field);
  if (field_hashtable_value == NULL) {
#if PHP_MAJOR_VERSION < 7
    MAKE_STD_ZVAL(field_hashtable_value);
    ZVAL_OBJ(field_hashtable_value, field_descriptor_type->create_object(
                                        field_descriptor_type TSRMLS_CC));
    Z_DELREF_P(field_hashtable_value);
#else
    field_hashtable_value =
        field_descriptor_type->create_object(field_descriptor_type TSRMLS_CC);
    GC_DELREF(field_hashtable_value);
#endif
    FieldDescriptor *field_php =
        UNBOX_HASHTABLE_VALUE(FieldDescriptor, field_hashtable_value);
    field_php->fielddef = field;
    add_def_obj(field, field_hashtable_value);
  }

#if PHP_MAJOR_VERSION < 7
  RETURN_ZVAL(field_hashtable_value, 1, 0);
#else
  GC_ADDREF(field_hashtable_value);
  RETURN_OBJ(field_hashtable_value);
#endif
}

PHP_METHOD(Descriptor, getFieldCount) {
  Descriptor *intern = UNBOX(Descriptor, getThis());
  RETURN_LONG(upb_msgdef_numfields(intern->msgdef));
}

PHP_METHOD(Descriptor, getOneofDecl) {
  long index;
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l", &index) ==
      FAILURE) {
    zend_error(E_USER_ERROR, "Expect integer for index.\n");
    return;
  }

  Descriptor *intern = UNBOX(Descriptor, getThis());
  int field_num = upb_msgdef_numoneofs(intern->msgdef);
  if (index < 0 || index >= field_num) {
    zend_error(E_USER_ERROR, "Cannot get element at %ld.\n", index);
    return;
  }

  upb_msg_oneof_iter iter;
  int i;
  for(upb_msg_oneof_begin(&iter, intern->msgdef), i = 0;
      !upb_msg_oneof_done(&iter) && i < index;
      upb_msg_oneof_next(&iter), i++);
  const upb_oneofdef *oneof = upb_msg_iter_oneof(&iter);

  ZVAL_OBJ(return_value, oneof_descriptor_type->create_object(
                             oneof_descriptor_type TSRMLS_CC));
  Oneof *oneof_php = UNBOX(Oneof, return_value);
  oneof_php->oneofdef = oneof;
}

PHP_METHOD(Descriptor, getOneofDeclCount) {
  Descriptor *intern = UNBOX(Descriptor, getThis());
  RETURN_LONG(upb_msgdef_numoneofs(intern->msgdef));
}

// -----------------------------------------------------------------------------
// EnumDescriptor
// -----------------------------------------------------------------------------

static zend_function_entry enum_descriptor_methods[] = {
  PHP_ME(EnumDescriptor, getValue, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(EnumDescriptor, getValueCount, NULL, ZEND_ACC_PUBLIC)
  ZEND_FE_END
};

DEFINE_CLASS(EnumDescriptor, enum_descriptor,
             "Google\\Protobuf\\EnumDescriptor");

static void enum_descriptor_free_c(EnumDescriptor *self TSRMLS_DC) {
}

static void enum_descriptor_init_c_instance(EnumDescriptor *self TSRMLS_DC) {
  self->enumdef = NULL;
  self->klass = NULL;
}

PHP_METHOD(EnumDescriptor, getValue) {
  long index;
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l", &index) ==
      FAILURE) {
    zend_error(E_USER_ERROR, "Expect integer for index.\n");
    return;
  }

  EnumDescriptor *intern = UNBOX(EnumDescriptor, getThis());
  int field_num = upb_enumdef_numvals(intern->enumdef);
  if (index < 0 || index >= field_num) {
    zend_error(E_USER_ERROR, "Cannot get element at %ld.\n", index);
    return;
  }

  upb_enum_iter iter;
  int i;
  for(upb_enum_begin(&iter, intern->enumdef), i = 0;
      !upb_enum_done(&iter) && i < index;
      upb_enum_next(&iter), i++);

  ZVAL_OBJ(return_value, enum_value_descriptor_type->create_object(
                             enum_value_descriptor_type TSRMLS_CC));
  EnumValueDescriptor *enum_value_php =
      UNBOX(EnumValueDescriptor, return_value);
  enum_value_php->name = upb_enum_iter_name(&iter);
  enum_value_php->number = upb_enum_iter_number(&iter);
}

PHP_METHOD(EnumDescriptor, getValueCount) {
  EnumDescriptor *intern = UNBOX(EnumDescriptor, getThis());
  RETURN_LONG(upb_enumdef_numvals(intern->enumdef));
}

// -----------------------------------------------------------------------------
// EnumValueDescriptor
// -----------------------------------------------------------------------------

static zend_function_entry enum_value_descriptor_methods[] = {
  PHP_ME(EnumValueDescriptor, getName, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(EnumValueDescriptor, getNumber, NULL, ZEND_ACC_PUBLIC)
  ZEND_FE_END
};

DEFINE_CLASS(EnumValueDescriptor, enum_value_descriptor,
             "Google\\Protobuf\\EnumValueDescriptor");

static void enum_value_descriptor_free_c(EnumValueDescriptor *self TSRMLS_DC) {
}

static void enum_value_descriptor_init_c_instance(EnumValueDescriptor *self TSRMLS_DC) {
  self->name = NULL;
  self->number = 0;
}

PHP_METHOD(EnumValueDescriptor, getName) {
  EnumValueDescriptor *intern = UNBOX(EnumValueDescriptor, getThis());
  PHP_PROTO_RETVAL_STRINGL(intern->name, strlen(intern->name), 1);
}

PHP_METHOD(EnumValueDescriptor, getNumber) {
  EnumValueDescriptor *intern = UNBOX(EnumValueDescriptor, getThis());
  RETURN_LONG(intern->number);
}

// -----------------------------------------------------------------------------
// FieldDescriptor
// -----------------------------------------------------------------------------

static zend_function_entry field_descriptor_methods[] = {
  PHP_ME(FieldDescriptor, getName,   NULL, ZEND_ACC_PUBLIC)
  PHP_ME(FieldDescriptor, getNumber, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(FieldDescriptor, getLabel,  NULL, ZEND_ACC_PUBLIC)
  PHP_ME(FieldDescriptor, getType,   NULL, ZEND_ACC_PUBLIC)
  PHP_ME(FieldDescriptor, isMap,     NULL, ZEND_ACC_PUBLIC)
  PHP_ME(FieldDescriptor, getEnumType, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(FieldDescriptor, getMessageType, NULL, ZEND_ACC_PUBLIC)
  ZEND_FE_END
};

DEFINE_CLASS(FieldDescriptor, field_descriptor,
             "Google\\Protobuf\\FieldDescriptor");

static void field_descriptor_free_c(FieldDescriptor *self TSRMLS_DC) {
}

static void field_descriptor_init_c_instance(FieldDescriptor *self TSRMLS_DC) {
  self->fielddef = NULL;
}

upb_fieldtype_t to_fieldtype(upb_descriptortype_t type) {
  switch (type) {
#define CASE(descriptor_type, type)           \
  case UPB_DESCRIPTOR_TYPE_##descriptor_type: \
    return UPB_TYPE_##type;

  CASE(FLOAT,    FLOAT);
  CASE(DOUBLE,   DOUBLE);
  CASE(BOOL,     BOOL);
  CASE(STRING,   STRING);
  CASE(BYTES,    BYTES);
  CASE(MESSAGE,  MESSAGE);
  CASE(GROUP,    MESSAGE);
  CASE(ENUM,     ENUM);
  CASE(INT32,    INT32);
  CASE(INT64,    INT64);
  CASE(UINT32,   UINT32);
  CASE(UINT64,   UINT64);
  CASE(SINT32,   INT32);
  CASE(SINT64,   INT64);
  CASE(FIXED32,  UINT32);
  CASE(FIXED64,  UINT64);
  CASE(SFIXED32, INT32);
  CASE(SFIXED64, INT64);

#undef CONVERT

  }

  zend_error(E_ERROR, "Unknown field type.");
  return 0;
}

PHP_METHOD(FieldDescriptor, getName) {
  FieldDescriptor *intern = UNBOX(FieldDescriptor, getThis());
  const char* name = upb_fielddef_name(intern->fielddef);
  PHP_PROTO_RETVAL_STRINGL(name, strlen(name), 1);
}

PHP_METHOD(FieldDescriptor, getNumber) {
  FieldDescriptor *intern = UNBOX(FieldDescriptor, getThis());
  RETURN_LONG(upb_fielddef_number(intern->fielddef));
}

PHP_METHOD(FieldDescriptor, getLabel) {
  FieldDescriptor *intern = UNBOX(FieldDescriptor, getThis());
  RETURN_LONG(upb_fielddef_label(intern->fielddef));
}

PHP_METHOD(FieldDescriptor, getType) {
  FieldDescriptor *intern = UNBOX(FieldDescriptor, getThis());
  RETURN_LONG(upb_fielddef_descriptortype(intern->fielddef));
}

PHP_METHOD(FieldDescriptor, isMap) {
  FieldDescriptor *intern = UNBOX(FieldDescriptor, getThis());
  RETURN_BOOL(upb_fielddef_ismap(intern->fielddef));
}

PHP_METHOD(FieldDescriptor, getEnumType) {
  FieldDescriptor *intern = UNBOX(FieldDescriptor, getThis());
  if (upb_fielddef_type(intern->fielddef) != UPB_TYPE_ENUM) {
    zend_throw_exception_ex(NULL, 0 TSRMLS_CC,
                            "Cannot get enum type for non-enum field '%s'",
                            upb_fielddef_name(intern->fielddef));
    return;
  }
  const upb_enumdef *enumdef = upb_fielddef_enumsubdef(intern->fielddef);
  PHP_PROTO_HASHTABLE_VALUE desc = get_def_obj(enumdef);

#if PHP_MAJOR_VERSION < 7
  RETURN_ZVAL(desc, 1, 0);
#else
  GC_ADDREF(desc);
  RETURN_OBJ(desc);
#endif
}

PHP_METHOD(FieldDescriptor, getMessageType) {
  FieldDescriptor *intern = UNBOX(FieldDescriptor, getThis());
  if (upb_fielddef_type(intern->fielddef) != UPB_TYPE_MESSAGE) {
    zend_throw_exception_ex(
        NULL, 0 TSRMLS_CC, "Cannot get message type for non-message field '%s'",
        upb_fielddef_name(intern->fielddef));
    return;
  }
  const upb_msgdef *msgdef = upb_fielddef_msgsubdef(intern->fielddef);
  PHP_PROTO_HASHTABLE_VALUE desc = get_def_obj(msgdef);

#if PHP_MAJOR_VERSION < 7
  RETURN_ZVAL(desc, 1, 0);
#else
  GC_ADDREF(desc);
  RETURN_OBJ(desc);
#endif
}

// -----------------------------------------------------------------------------
// Oneof
// -----------------------------------------------------------------------------

static zend_function_entry oneof_descriptor_methods[] = {
  PHP_ME(Oneof, getName,  NULL, ZEND_ACC_PUBLIC)
  PHP_ME(Oneof, getField, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(Oneof, getFieldCount, NULL, ZEND_ACC_PUBLIC)
  ZEND_FE_END
};

DEFINE_CLASS(Oneof, oneof_descriptor,
             "Google\\Protobuf\\OneofDescriptor");

static void oneof_descriptor_free_c(Oneof *self TSRMLS_DC) {
}

static void oneof_descriptor_init_c_instance(Oneof *self TSRMLS_DC) {
  self->oneofdef = NULL;
}

PHP_METHOD(Oneof, getName) {
  Oneof *intern = UNBOX(Oneof, getThis());
  const char *name = upb_oneofdef_name(intern->oneofdef);
  PHP_PROTO_RETVAL_STRINGL(name, strlen(name), 1);
}

PHP_METHOD(Oneof, getField) {
  long index;
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l", &index) ==
      FAILURE) {
    zend_error(E_USER_ERROR, "Expect integer for index.\n");
    return;
  }

  Oneof *intern = UNBOX(Oneof, getThis());
  int field_num = upb_oneofdef_numfields(intern->oneofdef);
  if (index < 0 || index >= field_num) {
    zend_error(E_USER_ERROR, "Cannot get element at %ld.\n", index);
    return;
  }

  upb_oneof_iter iter;
  int i;
  for(upb_oneof_begin(&iter, intern->oneofdef), i = 0;
      !upb_oneof_done(&iter) && i < index;
      upb_oneof_next(&iter), i++);
  const upb_fielddef *field = upb_oneof_iter_field(&iter);

  PHP_PROTO_HASHTABLE_VALUE field_hashtable_value = get_def_obj(field);
  if (field_hashtable_value == NULL) {
#if PHP_MAJOR_VERSION < 7
    MAKE_STD_ZVAL(field_hashtable_value);
    ZVAL_OBJ(field_hashtable_value, field_descriptor_type->create_object(
                                        field_descriptor_type TSRMLS_CC));
#else
    field_hashtable_value =
        field_descriptor_type->create_object(field_descriptor_type TSRMLS_CC);
#endif
    FieldDescriptor *field_php =
        UNBOX_HASHTABLE_VALUE(FieldDescriptor, field_hashtable_value);
    field_php->fielddef = field;
    add_def_obj(field, field_hashtable_value);
  }

#if PHP_MAJOR_VERSION < 7
  RETURN_ZVAL(field_hashtable_value, 1, 0);
#else
  GC_ADDREF(field_hashtable_value);
  RETURN_OBJ(field_hashtable_value);
#endif
}

PHP_METHOD(Oneof, getFieldCount) {
  Oneof *intern = UNBOX(Oneof, getThis());
  RETURN_LONG(upb_oneofdef_numfields(intern->oneofdef));
}

// -----------------------------------------------------------------------------
// DescriptorPool
// -----------------------------------------------------------------------------

static zend_function_entry descriptor_pool_methods[] = {
  PHP_ME(DescriptorPool, getGeneratedPool, NULL,
         ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
  PHP_ME(DescriptorPool, getDescriptorByClassName, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(DescriptorPool, getEnumDescriptorByClassName, NULL, ZEND_ACC_PUBLIC)
  ZEND_FE_END
};

static zend_function_entry internal_descriptor_pool_methods[] = {
  PHP_ME(InternalDescriptorPool, getGeneratedPool, NULL,
         ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
  PHP_ME(InternalDescriptorPool, internalAddGeneratedFile, NULL, ZEND_ACC_PUBLIC)
  ZEND_FE_END
};

DEFINE_CLASS(DescriptorPool, descriptor_pool,
             "Google\\Protobuf\\DescriptorPool");
DEFINE_CLASS(InternalDescriptorPool, internal_descriptor_pool,
             "Google\\Protobuf\\Internal\\DescriptorPool");

// wrapper of generated pool
#if PHP_MAJOR_VERSION < 7
zval* generated_pool_php;
zval* internal_generated_pool_php;
#else
zend_object *generated_pool_php;
zend_object *internal_generated_pool_php;
#endif
InternalDescriptorPool *generated_pool;  // The actual generated pool

void init_generated_pool_once(TSRMLS_D) {
  if (generated_pool == NULL) {
#if PHP_MAJOR_VERSION < 7
    MAKE_STD_ZVAL(generated_pool_php);
    MAKE_STD_ZVAL(internal_generated_pool_php);
    ZVAL_OBJ(internal_generated_pool_php,
             internal_descriptor_pool_type->create_object(
                 internal_descriptor_pool_type TSRMLS_CC));
    generated_pool = UNBOX(InternalDescriptorPool, internal_generated_pool_php);
    ZVAL_OBJ(generated_pool_php, descriptor_pool_type->create_object(
                                     descriptor_pool_type TSRMLS_CC));
#else
    internal_generated_pool_php = internal_descriptor_pool_type->create_object(
        internal_descriptor_pool_type TSRMLS_CC);
    generated_pool = (InternalDescriptorPool *)((char *)internal_generated_pool_php -
                                        XtOffsetOf(InternalDescriptorPool, std));
    generated_pool_php =
        descriptor_pool_type->create_object(descriptor_pool_type TSRMLS_CC);
#endif
  }
}

static void internal_descriptor_pool_init_c_instance(
    InternalDescriptorPool *pool TSRMLS_DC) {
  pool->symtab = upb_symtab_new();
  pool->fill_handler_cache =
      upb_handlercache_new(add_handlers_for_message, NULL);
  pool->pb_serialize_handler_cache = upb_pb_encoder_newcache();
  pool->json_serialize_handler_cache = upb_json_printer_newcache(false);
  pool->json_serialize_handler_preserve_cache = upb_json_printer_newcache(true);
  pool->fill_method_cache = upb_pbcodecache_new(pool->fill_handler_cache);
  pool->json_fill_method_cache = upb_json_codecache_new();
}

static void internal_descriptor_pool_free_c(
    InternalDescriptorPool *pool TSRMLS_DC) {
  upb_symtab_free(pool->symtab);
  upb_handlercache_free(pool->fill_handler_cache);
  upb_handlercache_free(pool->pb_serialize_handler_cache);
  upb_handlercache_free(pool->json_serialize_handler_cache);
  upb_handlercache_free(pool->json_serialize_handler_preserve_cache);
  upb_pbcodecache_free(pool->fill_method_cache);
  upb_json_codecache_free(pool->json_fill_method_cache);
}

static void descriptor_pool_init_c_instance(DescriptorPool *pool TSRMLS_DC) {
  assert(generated_pool != NULL);
  pool->intern = generated_pool;
}

static void descriptor_pool_free_c(DescriptorPool *pool TSRMLS_DC) {
}

static void validate_enumdef(const upb_enumdef *enumdef) {
  // Verify that an entry exists with integer value 0. (This is the default
  // value.)
  const char *lookup = upb_enumdef_iton(enumdef, 0);
  if (lookup == NULL) {
    zend_error(E_USER_ERROR,
               "Enum definition does not contain a value for '0'.");
  }
}

static void validate_msgdef(const upb_msgdef* msgdef) {
  // Verify that no required fields exist. proto3 does not support these.
  upb_msg_field_iter it;
  for (upb_msg_field_begin(&it, msgdef);
       !upb_msg_field_done(&it);
       upb_msg_field_next(&it)) {
    const upb_fielddef* field = upb_msg_iter_field(&it);
    if (upb_fielddef_label(field) == UPB_LABEL_REQUIRED) {
      zend_error(E_ERROR, "Required fields are unsupported in proto3.");
    }
  }
}

PHP_METHOD(DescriptorPool, getGeneratedPool) {
  init_generated_pool_once(TSRMLS_C);
#if PHP_MAJOR_VERSION < 7
  RETURN_ZVAL(generated_pool_php, 1, 0);
#else
  GC_ADDREF(generated_pool_php);
  RETURN_OBJ(generated_pool_php);
#endif
}

PHP_METHOD(InternalDescriptorPool, getGeneratedPool) {
  init_generated_pool_once(TSRMLS_C);
#if PHP_MAJOR_VERSION < 7
  RETURN_ZVAL(internal_generated_pool_php, 1, 0);
#else
  GC_ADDREF(internal_generated_pool_php);
  RETURN_OBJ(internal_generated_pool_php);
#endif
}

static size_t classname_len_max(const char *fullname,
                                const char *package,
                                const char *php_namespace,
                                const char *prefix) {
  size_t fullname_len = strlen(fullname);
  size_t package_len = 0;
  size_t prefix_len = 0;
  size_t namespace_len = 0;
  size_t length = fullname_len;
  int i, segment, classname_start = 0;

  if (package != NULL) {
    package_len = strlen(package);
  }
  if (prefix != NULL) {
    prefix_len = strlen(prefix);
  }
  if (php_namespace != NULL) {
    namespace_len = strlen(php_namespace);
  }

  // Process package
  if (package_len > 0) {
    segment = 1;
    for (i = 0; i < package_len; i++) {
      if (package[i] == '.') {
        segment++;
      }
    }
    // In case of reserved name in package.
    length += 3 * segment;

    classname_start = package_len + 1;
  }

  // Process class name
  segment = 1;
  for (i = classname_start; i < fullname_len; i++) {
    if (fullname[i] == '.') {
      segment++;
    }
  }
  if (prefix_len == 0) {
    length += 3 * segment;
  } else {
    length += prefix_len * segment;
  }

  // The additional 2, one is for preceding '.' and the other is for trailing 0.
  return length + namespace_len + 2;
}

static bool is_reserved(const char *segment, int length) {
  bool result;
  char* lower = ALLOC_N(char, length + 1);
  memset(lower, 0, length + 1);
  memcpy(lower, segment, length);
  int i = 0;
  while(lower[i]) {
    lower[i] = (char)tolower(lower[i]);
    i++;
  }
  lower[length] = 0;
  result = is_reserved_name(lower);
  FREE(lower);
  return result;
}

static void fill_prefix(const char *segment, int length,
                        const char *prefix_given,
                        const char *package_name,
                        stringsink *classname) {
  size_t i;

  if (prefix_given != NULL && strcmp(prefix_given, "") != 0) {
    stringsink_string(classname, NULL, prefix_given,
                      strlen(prefix_given), NULL);
  } else {
    if (is_reserved(segment, length)) {
      if (package_name != NULL &&
          strcmp("google.protobuf", package_name) == 0) {
        stringsink_string(classname, NULL, "GPB", 3, NULL);
      } else {
        stringsink_string(classname, NULL, "PB", 2, NULL);
      }
    }
  }
}

static void fill_segment(const char *segment, int length,
                         stringsink *classname, bool use_camel) {
  if (use_camel && (segment[0] < 'A' || segment[0] > 'Z')) {
    char first = segment[0] + ('A' - 'a');
    stringsink_string(classname, NULL, &first, 1, NULL);
    stringsink_string(classname, NULL, segment + 1, length - 1, NULL);
  } else {
    stringsink_string(classname, NULL, segment, length, NULL);
  }
}

static void fill_namespace(const char *package, const char *php_namespace,
                           stringsink *classname) {
  if (php_namespace != NULL) {
    stringsink_string(classname, NULL, php_namespace, strlen(php_namespace),
                      NULL);
    stringsink_string(classname, NULL, "\\", 1, NULL);
  } else if (package != NULL) {
    int i = 0, j, offset = 0;
    size_t package_len = strlen(package);
    while (i < package_len) {
      j = i;
      while (j < package_len && package[j] != '.') {
        j++;
      }
      fill_prefix(package + i, j - i, "", package, classname);
      fill_segment(package + i, j - i, classname, true);
      stringsink_string(classname, NULL, "\\", 1, NULL);
      i = j + 1;
    }
  }
}

static void fill_classname(const char *fullname,
                           const char *package,
                           const char *prefix,
                           stringsink *classname,
                           bool use_nested_submsg) {
  int classname_start = 0;
  if (package != NULL) {
    size_t package_len = strlen(package);
    classname_start = package_len == 0 ? 0 : package_len + 1;
  }
  size_t fullname_len = strlen(fullname);
  bool is_first_segment = true;

  int i = classname_start, j;
  while (i < fullname_len) {
    j = i;
    while (j < fullname_len && fullname[j] != '.') {
      j++;
    }
    if (use_nested_submsg || is_first_segment && j == fullname_len) {
      fill_prefix(fullname + i, j - i, prefix, package, classname);
    }
    is_first_segment = false;
    fill_segment(fullname + i, j - i, classname, false);
    if (j != fullname_len) {
      if (use_nested_submsg) {
        stringsink_string(classname, NULL, "\\", 1, NULL);
      } else {
        stringsink_string(classname, NULL, "_", 1, NULL);
      }
    }
    i = j + 1;
  }
}

static zend_class_entry *register_class(const upb_filedef *file,
                                        const char *fullname,
                                        PHP_PROTO_HASHTABLE_VALUE desc_php,
                                        bool use_nested_submsg TSRMLS_DC) {
  // Prepend '.' to package name to make it absolute. In the 5 additional
  // bytes allocated, one for '.', one for trailing 0, and 3 for 'GPB' if
  // given message is google.protobuf.Empty.
  const char *package = upb_filedef_package(file);
  const char *php_namespace = upb_filedef_phpnamespace(file);
  const char *prefix = upb_filedef_phpprefix(file);
  size_t classname_len =
      classname_len_max(fullname, package, php_namespace, prefix);
  char* after_package;
  zend_class_entry* ret;
  stringsink namesink;
  stringsink_init(&namesink);

  fill_namespace(package, php_namespace, &namesink);
  fill_classname(fullname, package, prefix, &namesink, use_nested_submsg);
  stringsink_string(&namesink, NULL, "\0", 1, NULL);

  PHP_PROTO_CE_DECLARE pce;
  if (php_proto_zend_lookup_class(namesink.ptr, namesink.len - 1, &pce) ==
      FAILURE) {
    zend_error(
        E_ERROR,
        "Generated message class %s hasn't been defined (%s, %s, %s, %s)",
        namesink.ptr, fullname, package, php_namespace, prefix);
    return NULL;
  }
  ret = PHP_PROTO_CE_UNREF(pce);
  add_ce_obj(ret, desc_php);
  add_proto_obj(fullname, desc_php);
  stringsink_uninit(&namesink);
  return ret;
}

bool depends_on_descriptor(const google_protobuf_FileDescriptorProto* file) {
  const upb_strview *deps;
  upb_strview name = upb_strview_makez("google/protobuf/descriptor.proto");
  size_t i, n;

  deps = google_protobuf_FileDescriptorProto_dependency(file, &n);
  for (i = 0; i < n; i++) {
    if (upb_strview_eql(deps[i], name)) {
      return true;
    }
  }

  return false;
}

const upb_filedef *parse_and_add_descriptor(const char *data,
                                            PHP_PROTO_SIZE data_len,
                                            InternalDescriptorPool *pool,
                                            upb_arena *arena) {
  size_t n;
  google_protobuf_FileDescriptorSet *set;
  const google_protobuf_FileDescriptorProto* const* files;
  const upb_filedef* file;
  upb_status status;

  set = google_protobuf_FileDescriptorSet_parse(
      data, data_len, arena);

  if (!set) {
    zend_error(E_ERROR, "Failed to parse binary descriptor\n");
    return false;
  }

  files = google_protobuf_FileDescriptorSet_file(set, &n);

  if (n != 1) {
    zend_error(E_ERROR, "Serialized descriptors should have exactly one file");
    return false;
  }

  // The PHP code generator currently special-cases descriptor.proto.  It
  // doesn't add it as a dependency even if the proto file actually does
  // depend on it.
  if (depends_on_descriptor(files[0]) &&
      upb_symtab_lookupfile(pool->symtab, "google/protobuf/descriptor.proto") ==
          NULL) {
    if (!parse_and_add_descriptor((char *)descriptor_proto,
                                  descriptor_proto_len, pool, arena)) {
      return false;
    }
  }

  upb_status_clear(&status);
  file = upb_symtab_addfile(pool->symtab, files[0], &status);
  check_upb_status(&status, "Unable to load descriptor");
  return file;
}

void internal_add_generated_file(const char *data, PHP_PROTO_SIZE data_len,
                                 InternalDescriptorPool *pool,
                                 bool use_nested_submsg TSRMLS_DC) {
  int i;
  upb_arena *arena;
  const upb_filedef* file;

  arena = upb_arena_new();
  file = parse_and_add_descriptor(data, data_len, pool, arena);
  upb_arena_free(arena);
  if (!file) return;

  // For each enum/message, we need its PHP class, upb descriptor and its PHP
  // wrapper. These information are needed later for encoding, decoding and type
  // checking. However, sometimes we just have one of them. In order to find
  // them quickly, here, we store the mapping for them.

  for (i = 0; i < upb_filedef_msgcount(file); i++) {
    const upb_msgdef *msgdef = upb_filedef_msg(file, i);
    CREATE_HASHTABLE_VALUE(desc, desc_php, Descriptor, descriptor_type);
    desc->msgdef = msgdef;
    desc->pool = pool;
    add_def_obj(desc->msgdef, desc_php);

    // Unlike other messages, MapEntry is shared by all map fields and doesn't
    // have generated PHP class.
    if (upb_msgdef_mapentry(msgdef)) {
      continue;
    }

    desc->klass = register_class(file, upb_msgdef_fullname(msgdef), desc_php,
                                 use_nested_submsg TSRMLS_CC);

    if (desc->klass == NULL) {
      return;
    }

    build_class_from_descriptor(desc_php TSRMLS_CC);
  }

  for (i = 0; i < upb_filedef_enumcount(file); i++) {
    const upb_enumdef *enumdef = upb_filedef_enum(file, i);
    CREATE_HASHTABLE_VALUE(desc, desc_php, EnumDescriptor, enum_descriptor_type);
    desc->enumdef = enumdef;
    add_def_obj(desc->enumdef, desc_php);
    desc->klass = register_class(file, upb_enumdef_fullname(enumdef), desc_php,
                                 use_nested_submsg TSRMLS_CC);

    if (desc->klass == NULL) {
      return;
    }
  }
}

PHP_METHOD(InternalDescriptorPool, internalAddGeneratedFile) {
  char *data = NULL;
  PHP_PROTO_SIZE data_len;
  upb_filedef **files;
  zend_bool use_nested_submsg = false;
  size_t i;

  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s|b",
                            &data, &data_len, &use_nested_submsg) ==
      FAILURE) {
    return;
  }

  InternalDescriptorPool *pool = UNBOX(InternalDescriptorPool, getThis());
  internal_add_generated_file(data, data_len, pool,
                              use_nested_submsg TSRMLS_CC);
}

PHP_METHOD(DescriptorPool, getDescriptorByClassName) {
  DescriptorPool *public_pool = UNBOX(DescriptorPool, getThis());
  InternalDescriptorPool *pool = public_pool->intern;

  char *classname = NULL;
  PHP_PROTO_SIZE classname_len;

  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &classname,
                            &classname_len) == FAILURE) {
    return;
  }

  PHP_PROTO_CE_DECLARE pce;
  if (php_proto_zend_lookup_class(classname, classname_len, &pce) ==
      FAILURE) {
    RETURN_NULL();
  }

  PHP_PROTO_HASHTABLE_VALUE desc = get_ce_obj(PHP_PROTO_CE_UNREF(pce));
  if (desc == NULL) {
    RETURN_NULL();
  }

  zend_class_entry* instance_ce = HASHTABLE_VALUE_CE(desc);

  if (!instanceof_function(instance_ce, descriptor_type TSRMLS_CC)) {
    RETURN_NULL();
  }

#if PHP_MAJOR_VERSION < 7
  RETURN_ZVAL(desc, 1, 0);
#else
  GC_ADDREF(desc);
  RETURN_OBJ(desc);
#endif
}

PHP_METHOD(DescriptorPool, getEnumDescriptorByClassName) {
  DescriptorPool *public_pool = UNBOX(DescriptorPool, getThis());
  InternalDescriptorPool *pool = public_pool->intern;

  char *classname = NULL;
  PHP_PROTO_SIZE classname_len;

  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &classname,
                            &classname_len) == FAILURE) {
    return;
  }

  PHP_PROTO_CE_DECLARE pce;
  if (php_proto_zend_lookup_class(classname, classname_len, &pce) ==
      FAILURE) {
    RETURN_NULL();
  }

  PHP_PROTO_HASHTABLE_VALUE desc = get_ce_obj(PHP_PROTO_CE_UNREF(pce));
  if (desc == NULL) {
    RETURN_NULL();
  }

  zend_class_entry* instance_ce = HASHTABLE_VALUE_CE(desc);

  if (!instanceof_function(instance_ce, enum_descriptor_type TSRMLS_CC)) {
    RETURN_NULL();
  }

#if PHP_MAJOR_VERSION < 7
  RETURN_ZVAL(desc, 1, 0);
#else
  GC_ADDREF(desc);
  RETURN_OBJ(desc);
#endif
}
