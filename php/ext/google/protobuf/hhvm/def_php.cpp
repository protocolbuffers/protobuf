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
// Forward Declaration
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// Descriptor
// -----------------------------------------------------------------------------

static void Descriptor_init_handlers(
    zend_object_handlers* handlers) {}
static void Descriptor_init_type(
    zend_class_entry* handlers) {}

PHP_METHOD(Descriptor, getClass);
PHP_METHOD(Descriptor, getFullName);
PHP_METHOD(Descriptor, getField);
PHP_METHOD(Descriptor, getFieldCount);
PHP_METHOD(Descriptor, getOneofDecl);
PHP_METHOD(Descriptor, getOneofDeclCount);

PROTO_REGISTER_CLASS_METHODS_START(Descriptor)
  PHP_ME(Descriptor, getClass, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(Descriptor, getFullName, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(Descriptor, getField, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(Descriptor, getFieldCount, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(Descriptor, getOneofDecl, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(Descriptor, getOneofDeclCount, NULL, ZEND_ACC_PUBLIC)
PROTO_REGISTER_CLASS_METHODS_END

PROTO_DEFINE_CLASS(Descriptor, "Google\\Protobuf\\Descriptor");

// -----------------------------------------------------------------------------
// EnumDescriptor
// -----------------------------------------------------------------------------

static void EnumDescriptor_init_handlers(
    zend_object_handlers* handlers) {}
static void EnumDescriptor_init_type(
    zend_class_entry* handlers) {}

PHP_METHOD(EnumDescriptor, getValue);
PHP_METHOD(EnumDescriptor, getValueCount);

PROTO_REGISTER_CLASS_METHODS_START(EnumDescriptor)
  PHP_ME(EnumDescriptor, getValue, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(EnumDescriptor, getValueCount, NULL, ZEND_ACC_PUBLIC)
PROTO_REGISTER_CLASS_METHODS_END

PROTO_DEFINE_CLASS(EnumDescriptor, "Google\\Protobuf\\EnumDescriptor");

// -----------------------------------------------------------------------------
// EnumValueDescriptor
// -----------------------------------------------------------------------------

static void EnumValueDescriptor_init_handlers(
    zend_object_handlers* handlers) {}
static void EnumValueDescriptor_init_type(
    zend_class_entry* handlers) {}

PHP_METHOD(EnumValueDescriptor, getName);
PHP_METHOD(EnumValueDescriptor, getNumber);

PROTO_REGISTER_CLASS_METHODS_START(EnumValueDescriptor)
  PHP_ME(EnumValueDescriptor, getName, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(EnumValueDescriptor, getNumber, NULL, ZEND_ACC_PUBLIC)
PROTO_REGISTER_CLASS_METHODS_END

PROTO_DEFINE_CLASS(EnumValueDescriptor,
                   "Google\\Protobuf\\EnumValueDescriptor");

// -----------------------------------------------------------------------------
// FieldDescriptor
// -----------------------------------------------------------------------------

static void FieldDescriptor_init_handlers(
    zend_object_handlers* handlers) {}
static void FieldDescriptor_init_type(
    zend_class_entry* handlers) {}

PHP_METHOD(FieldDescriptor, getName);
PHP_METHOD(FieldDescriptor, getNumber);
PHP_METHOD(FieldDescriptor, getLabel);
PHP_METHOD(FieldDescriptor, getType);
PHP_METHOD(FieldDescriptor, isMap);
PHP_METHOD(FieldDescriptor, getEnumType);
PHP_METHOD(FieldDescriptor, getMessageType);

PROTO_REGISTER_CLASS_METHODS_START(FieldDescriptor)
  PHP_ME(FieldDescriptor, getName, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(FieldDescriptor, getNumber, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(FieldDescriptor, getLabel, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(FieldDescriptor, getType, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(FieldDescriptor, isMap, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(FieldDescriptor, getEnumType, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(FieldDescriptor, getMessageType, NULL, ZEND_ACC_PUBLIC)
PROTO_REGISTER_CLASS_METHODS_END

PROTO_DEFINE_CLASS(FieldDescriptor,
                   "Google\\Protobuf\\FieldDescriptor");

// -----------------------------------------------------------------------------
// OneofDescriptor
// -----------------------------------------------------------------------------

static void OneofDescriptor_init_handlers(
    zend_object_handlers* handlers) {}
static void OneofDescriptor_init_type(
    zend_class_entry* handlers) {}

PHP_METHOD(Oneof, getName);
PHP_METHOD(Oneof, getField);
PHP_METHOD(Oneof, getFieldCount);

PROTO_REGISTER_CLASS_METHODS_START(OneofDescriptor)
  PHP_ME(Oneof, getName, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(Oneof, getField, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(Oneof, getFieldCount, NULL, ZEND_ACC_PUBLIC)
PROTO_REGISTER_CLASS_METHODS_END

PROTO_DEFINE_CLASS(OneofDescriptor,
                   "Google\\Protobuf\\OneofDescriptor");

// -----------------------------------------------------------------------------
// Define PHP Methods
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// Descriptor
// -----------------------------------------------------------------------------

PHP_METHOD(Descriptor, getClass) {
  Descriptor *self = UNBOX(Descriptor, getThis());
#if PHP_MAJOR_VERSION < 7
  const char* classname = self->klass->name;
#else
  const char* classname = ZSTR_VAL(self->klass->name);
#endif
  PROTO_RETURN_STRINGL(classname, strlen(classname), 1);
}

PHP_METHOD(Descriptor, getFullName) {
  Descriptor *self = UNBOX(Descriptor, getThis());
  const char* fullname = upb_msgdef_fullname(self->intern);
  PROTO_RETURN_STRINGL(fullname, strlen(fullname), 1);
}

PHP_METHOD(Descriptor, getField) {
  long index;
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l", &index) ==
      FAILURE) {
    zend_error(E_USER_ERROR, "Expect integer for index.\n");
    return;
  }

  Descriptor *self = UNBOX(Descriptor, getThis());
  int field_num = upb_msgdef_numfields(self->intern);
  if (index < 0 || index >= field_num) {
    zend_error(E_USER_ERROR, "Cannot get element at %ld.\n", index);
    return;
  }

  upb_msg_field_iter iter;
  int i;
  for(upb_msg_field_begin(&iter, self->intern), i = 0;
      !upb_msg_field_done(&iter) && i < index;
      upb_msg_field_next(&iter), i++);
  const upb_fielddef *field = upb_msg_iter_field(&iter);

  ZVAL_OBJ(return_value, FieldDescriptor_type->create_object(
           FieldDescriptor_type TSRMLS_CC));
  FieldDescriptor* desc = UNBOX(FieldDescriptor, return_value);
  desc->intern = field;
}

PHP_METHOD(Descriptor, getFieldCount) {
  Descriptor *self = UNBOX(Descriptor, getThis());
  RETURN_LONG(upb_msgdef_numfields(self->intern));
}

PHP_METHOD(Descriptor, getOneofDecl) {
  long index;
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l", &index) ==
      FAILURE) {
    zend_error(E_USER_ERROR, "Expect integer for index.\n");
    return;
  }

  Descriptor *self = UNBOX(Descriptor, getThis());
  int field_num = upb_msgdef_numoneofs(self->intern);
  if (index < 0 || index >= field_num) {
    zend_error(E_USER_ERROR, "Cannot get element at %ld.\n", index);
    return;
  }

  upb_msg_oneof_iter iter;
  int i;
  for(upb_msg_oneof_begin(&iter, self->intern), i = 0;
      !upb_msg_oneof_done(&iter) && i < index;
      upb_msg_oneof_next(&iter), i++);
  upb_oneofdef *oneof = upb_msg_iter_oneof(&iter);

  ZVAL_OBJ(return_value, OneofDescriptor_type->create_object(
                             OneofDescriptor_type TSRMLS_CC));
  OneofDescriptor *desc = UNBOX(OneofDescriptor, return_value);
  desc->intern = oneof;
}

PHP_METHOD(Descriptor, getOneofDeclCount) {
  Descriptor *self = UNBOX(Descriptor, getThis());
  RETURN_LONG(upb_msgdef_numoneofs(self->intern));
}

// -----------------------------------------------------------------------------
// EnumDescriptor
// -----------------------------------------------------------------------------

PHP_METHOD(EnumDescriptor, getValue) {
  long index;
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l", &index) ==
      FAILURE) {
    zend_error(E_USER_ERROR, "Expect integer for index.\n");
    return;
  }

  EnumDescriptor *self = UNBOX(EnumDescriptor, getThis());
  int field_num = upb_enumdef_numvals(self->intern);
  if (index < 0 || index >= field_num) {
    zend_error(E_USER_ERROR, "Cannot get element at %ld.\n", index);
    return;
  }

  upb_enum_iter iter;
  int i;
  for(upb_enum_begin(&iter, self->intern), i = 0;
      !upb_enum_done(&iter) && i < index;
      upb_enum_next(&iter), i++);

  ZVAL_OBJ(return_value, EnumValueDescriptor_type->create_object(
                             EnumValueDescriptor_type TSRMLS_CC));
  EnumValueDescriptor *desc =
      UNBOX(EnumValueDescriptor, return_value);
  desc->name = upb_enum_iter_name(&iter);
  desc->number = upb_enum_iter_number(&iter);
}

PHP_METHOD(EnumDescriptor, getValueCount) {
  EnumDescriptor *self = UNBOX(EnumDescriptor, getThis());
  RETURN_LONG(upb_enumdef_numvals(self->intern));
}

// -----------------------------------------------------------------------------
// EnumValueDescriptor
// -----------------------------------------------------------------------------

PHP_METHOD(EnumValueDescriptor, getName) {
  EnumValueDescriptor *intern = UNBOX(EnumValueDescriptor, getThis());
  PROTO_RETURN_STRINGL(intern->name, strlen(intern->name), 1);
}

PHP_METHOD(EnumValueDescriptor, getNumber) {
  EnumValueDescriptor *intern = UNBOX(EnumValueDescriptor, getThis());
  RETURN_LONG(intern->number);
}

// -----------------------------------------------------------------------------
// FieldDescriptor
// -----------------------------------------------------------------------------

PHP_METHOD(FieldDescriptor, getName) {
  FieldDescriptor *self = UNBOX(FieldDescriptor, getThis());
  const char* name = upb_fielddef_name(self->intern);
  PROTO_RETURN_STRINGL(name, strlen(name), 1);
}

PHP_METHOD(FieldDescriptor, getNumber) {
  FieldDescriptor *self = UNBOX(FieldDescriptor, getThis());
  RETURN_LONG(upb_fielddef_number(self->intern));
}

PHP_METHOD(FieldDescriptor, getLabel) {
  FieldDescriptor *self = UNBOX(FieldDescriptor, getThis());
  RETURN_LONG(upb_fielddef_label(self->intern));
}

PHP_METHOD(FieldDescriptor, getType) {
  FieldDescriptor *self = UNBOX(FieldDescriptor, getThis());
  RETURN_LONG(upb_fielddef_descriptortype(self->intern));
}

PHP_METHOD(FieldDescriptor, isMap) {
  FieldDescriptor *self = UNBOX(FieldDescriptor, getThis());
  RETURN_BOOL(upb_fielddef_ismap(self->intern));
}

PHP_METHOD(FieldDescriptor, getEnumType) {
  FieldDescriptor *self = UNBOX(FieldDescriptor, getThis());
  const upb_enumdef *enumdef = upb_fielddef_enumsubdef(self->intern);
  if (enumdef == NULL) {
    char error_msg[100];
    sprintf(error_msg, "Cannot get enum type for non-enum field '%s'",
            upb_fielddef_name(self->intern));
    zend_throw_exception(NULL, error_msg, 0 TSRMLS_CC);
    return;
  }

  ZVAL_OBJ(return_value, EnumDescriptor_type->create_object(
           EnumDescriptor_type TSRMLS_CC));
  EnumDescriptor* desc = UNBOX(EnumDescriptor, return_value);
  desc->intern = enumdef;
}

PHP_METHOD(FieldDescriptor, getMessageType) {
  FieldDescriptor *self = UNBOX(FieldDescriptor, getThis());
  const upb_msgdef *msgdef = upb_fielddef_msgsubdef(self->intern);
  if (msgdef == NULL) {
    char error_msg[100];
    sprintf(error_msg, "Cannot get message type for non-message field '%s'",
            upb_fielddef_name(self->intern));
    zend_throw_exception(NULL, error_msg, 0 TSRMLS_CC);
    return;
  }

  ZVAL_OBJ(return_value, Descriptor_type->create_object(
           Descriptor_type TSRMLS_CC));
  Descriptor* desc = UNBOX(Descriptor, return_value);
  desc->intern = msgdef;
}

// -----------------------------------------------------------------------------
// OneofDescriptor
// -----------------------------------------------------------------------------

PHP_METHOD(Oneof, getName) {
  OneofDescriptor *self = UNBOX(OneofDescriptor, getThis());
  const char *name = upb_oneofdef_name(self->intern);
  PROTO_RETURN_STRINGL(name, strlen(name), 1);
}

PHP_METHOD(Oneof, getField) {
  long index;
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l", &index) ==
      FAILURE) {
    zend_error(E_USER_ERROR, "Expect integer for index.\n");
    return;
  }

  OneofDescriptor *self = UNBOX(OneofDescriptor, getThis());
  int field_num = upb_oneofdef_numfields(self->intern);
  if (index < 0 || index >= field_num) {
    zend_error(E_USER_ERROR, "Cannot get element at %ld.\n", index);
    return;
  }

  upb_oneof_iter iter;
  int i;
  for(upb_oneof_begin(&iter, self->intern), i = 0;
      !upb_oneof_done(&iter) && i < index;
      upb_oneof_next(&iter), i++);
  const upb_fielddef *field = upb_oneof_iter_field(&iter);

  ZVAL_OBJ(return_value, FieldDescriptor_type->create_object(
           FieldDescriptor_type TSRMLS_CC));
  FieldDescriptor* desc = UNBOX(FieldDescriptor, return_value);
  desc->intern = field;
}

PHP_METHOD(Oneof, getFieldCount) {
  OneofDescriptor *self = UNBOX(OneofDescriptor, getThis());
  RETURN_LONG(upb_oneofdef_numfields(self->intern));
}

// -----------------------------------------------------------------------------
// InternalDescriptorPool
// -----------------------------------------------------------------------------

#if PHP_MAJOR_VERSION < 7
zval* internal_generated_pool;
zval* generated_pool;
#else
zend_object *internal_generated_pool;
zend_object *generated_pool;
#endif

static void InternalDescriptorPool_init_handlers(
    zend_object_handlers* handlers) {}
static void InternalDescriptorPool_init_type(
    zend_class_entry* handlers) {}

static void DescriptorPool_init_handlers(zend_object_handlers* handlers) {}
static void DescriptorPool_init_type(zend_class_entry* handlers) {}

PHP_METHOD(InternalDescriptorPool, internalAddGeneratedFile);
PHP_METHOD(InternalDescriptorPool, getGeneratedPool);

PHP_METHOD(DescriptorPool, getGeneratedPool);
PHP_METHOD(DescriptorPool, getDescriptorByClassName);
PHP_METHOD(DescriptorPool, getEnumDescriptorByClassName);

PROTO_REGISTER_CLASS_METHODS_START(InternalDescriptorPool)
  PHP_ME(InternalDescriptorPool, internalAddGeneratedFile,
         NULL, ZEND_ACC_PUBLIC)
  PHP_ME(InternalDescriptorPool, getGeneratedPool,
         NULL, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
PROTO_REGISTER_CLASS_METHODS_END

PROTO_REGISTER_CLASS_METHODS_START(DescriptorPool)
  PHP_ME(DescriptorPool, getGeneratedPool, NULL, 
         ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
  PHP_ME(DescriptorPool, getDescriptorByClassName, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(DescriptorPool, getEnumDescriptorByClassName, NULL, ZEND_ACC_PUBLIC)
PROTO_REGISTER_CLASS_METHODS_END

PROTO_DEFINE_CLASS(InternalDescriptorPool,
             "Google\\Protobuf\\Internal\\DescriptorPool");
PROTO_DEFINE_CLASS(DescriptorPool, "Google\\Protobuf\\DescriptorPool");

PHP_METHOD(InternalDescriptorPool, internalAddGeneratedFile) {
  char *data = NULL;
  PROTO_SIZE data_len;

  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &data, &data_len) ==
      FAILURE) {
    return;
  }

  InternalDescriptorPool *pool = UNBOX(InternalDescriptorPool, getThis());
  InternalDescriptorPool_add_generated_file(pool, data, data_len);

  PROTO_RETURN_BOOL(true);
}

static void init_generated_pool_once(TSRMLS_D) {
  if (internal_generated_pool == NULL) {
#if PHP_MAJOR_VERSION < 7
    MAKE_STD_ZVAL(internal_generated_pool);
    MAKE_STD_ZVAL(generated_pool);
    ZVAL_OBJ(internal_generated_pool,
             InternalDescriptorPool_type->create_object(
                 InternalDescriptorPool_type TSRMLS_CC));
    ZVAL_OBJ(generated_pool,
             DescriptorPool_type->create_object(
                 DescriptorPool_type TSRMLS_CC));
    internal_generated_pool_cpp =
        UNBOX(InternalDescriptorPool, internal_generated_pool);
    DescriptorPool* generated_pool_cpp = UNBOX(DescriptorPool, generated_pool);
#else
    internal_generated_pool = InternalDescriptorPool_type->create_object(
        InternalDescriptorPool_type TSRMLS_CC);
    generated_pool = DescriptorPool_type->create_object(
        DescriptorPool_type TSRMLS_CC);
    internal_generated_pool_cpp =
        (InternalDescriptorPool*)((char*)internal_generated_pool -
            XtOffsetOf(InternalDescriptorPool, std));
    DescriptorPool* generated_pool_cpp =
        (DescriptorPool*)((char*)generated_pool -
            XtOffsetOf(DescriptorPool, std));
#endif
    generated_pool_cpp->intern = internal_generated_pool_cpp;
    message_factory = upb_msgfactory_new(
        internal_generated_pool_cpp->symtab);
  }
}

PHP_METHOD(InternalDescriptorPool, getGeneratedPool) {
  init_generated_pool_once(TSRMLS_C);
#if PHP_MAJOR_VERSION < 7
  RETURN_ZVAL(internal_generated_pool, 1, 0);
#else
  ++GC_REFCOUNT(internal_generated_pool);
  RETURN_OBJ(internal_generated_pool);
#endif
}

PHP_METHOD(DescriptorPool, getGeneratedPool) {
  init_generated_pool_once(TSRMLS_C);
#if PHP_MAJOR_VERSION < 7
  RETURN_ZVAL(generated_pool, 1, 0);
#else
  ++GC_REFCOUNT(generated_pool);
  RETURN_OBJ(generated_pool);
#endif
}

PHP_METHOD(DescriptorPool, getDescriptorByClassName) {
  DescriptorPool *self = UNBOX(DescriptorPool, getThis());
  InternalDescriptorPool *pool = self->intern;

  char *classname = NULL;
  PROTO_SIZE classname_len;

  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &classname,
                            &classname_len) == FAILURE) {
    return;
  }

  PROTO_CE_DECLARE pce;
  if (php_proto_zend_lookup_class(classname, classname_len, &pce) ==
      FAILURE) {
    RETURN_NULL();
  }

  const upb_msgdef* msgdef = class2msgdef(PROTO_CE_UNREF(pce));
  if (msgdef == NULL) {
    RETURN_NULL();
  }

  ZVAL_OBJ(return_value, Descriptor_type->create_object(
           Descriptor_type TSRMLS_CC));
  Descriptor* desc = UNBOX(Descriptor, return_value);
  desc->intern = msgdef;
  desc->klass = PROTO_CE_UNREF(pce);
}

PHP_METHOD(DescriptorPool, getEnumDescriptorByClassName) {
  DescriptorPool *self = UNBOX(DescriptorPool, getThis());
  InternalDescriptorPool *pool = self->intern;

  char *classname = NULL;
  PROTO_SIZE classname_len;

  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &classname,
                            &classname_len) == FAILURE) {
    return;
  }

  PROTO_CE_DECLARE pce;
  if (php_proto_zend_lookup_class(classname, classname_len, &pce) ==
      FAILURE) {
    RETURN_NULL();
  }

  const upb_enumdef* enumdef = class2enumdef(PROTO_CE_UNREF(pce));
  if (enumdef == NULL) {
    RETURN_NULL();
  }

  ZVAL_OBJ(return_value, EnumDescriptor_type->create_object(
           EnumDescriptor_type TSRMLS_CC));
  EnumDescriptor* desc = UNBOX(EnumDescriptor, return_value);
  desc->intern = enumdef;
}
