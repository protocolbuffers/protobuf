// Protocol Buffers - Google's data interchange format
// Copyright 2014 Google Inc.  All rights reserved.
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
#include <stdlib.h>
#include <ext/json/php_json.h>

#include "protobuf.h"

static zend_class_entry* message_type;
zend_object_handlers* message_handlers;

static  zend_function_entry message_methods[] = {
  PHP_ME(Message, clear, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(Message, encode, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(Message, decode, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(Message, jsonEncode, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(Message, jsonDecode, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(Message, readOneof, NULL, ZEND_ACC_PROTECTED)
  PHP_ME(Message, writeOneof, NULL, ZEND_ACC_PROTECTED)
  PHP_ME(Message, whichOneof, NULL, ZEND_ACC_PROTECTED)
  PHP_ME(Message, __construct, NULL, ZEND_ACC_PROTECTED)
  {NULL, NULL, NULL}
};

// Forward declare static functions.

static void message_set_property(zval* object, zval* member, zval* value,
                                 const zend_literal* key TSRMLS_DC);
static zval* message_get_property(zval* object, zval* member, int type,
                                  const zend_literal* key TSRMLS_DC);
static zval** message_get_property_ptr_ptr(zval* object, zval* member, int type,
                                           const zend_literal* key TSRMLS_DC);
static HashTable* message_get_properties(zval* object TSRMLS_DC);
static HashTable* message_get_gc(zval* object, zval*** table, int* n TSRMLS_DC);

static zend_object_value message_create(zend_class_entry* ce TSRMLS_DC);
static void message_free(void* object TSRMLS_DC);

// -----------------------------------------------------------------------------
// PHP Message Handlers
// -----------------------------------------------------------------------------

void message_init(TSRMLS_D) {
  zend_class_entry class_type;
  INIT_CLASS_ENTRY(class_type, "Google\\Protobuf\\Internal\\Message",
                   message_methods);
  message_type = zend_register_internal_class(&class_type TSRMLS_CC);

  message_handlers = PEMALLOC(zend_object_handlers);
  memcpy(message_handlers, zend_get_std_object_handlers(),
         sizeof(zend_object_handlers));
  message_handlers->write_property = message_set_property;
  message_handlers->read_property = message_get_property;
  message_handlers->get_property_ptr_ptr = message_get_property_ptr_ptr;
  message_handlers->get_properties = message_get_properties;
  message_handlers->get_gc = message_get_gc;
}

static void message_set_property(zval* object, zval* member, zval* value,
                                 const zend_literal* key TSRMLS_DC) {
  if (Z_TYPE_P(member) != IS_STRING) {
    zend_error(E_USER_ERROR, "Unexpected type for field name");
    return;
  }

  if (Z_OBJCE_P(object) != EG(scope)) {
    // User cannot set property directly (e.g., $m->a = 1)
    zend_error(E_USER_ERROR, "Cannot access private property.");
    return;
  }

  const upb_fielddef* field;

  MessageHeader* self = zend_object_store_get_object(object TSRMLS_CC);

  field = upb_msgdef_ntofz(self->descriptor->msgdef, Z_STRVAL_P(member));
  if (field == NULL) {
    zend_error(E_USER_ERROR, "Unknown field: %s", Z_STRVAL_P(member));
  }

  layout_set(self->descriptor->layout, self, field, value TSRMLS_CC);
}

static zval* message_get_property(zval* object, zval* member, int type,
                                  const zend_literal* key TSRMLS_DC) {
  if (Z_TYPE_P(member) != IS_STRING) {
    zend_error(E_USER_ERROR, "Property name has to be a string.");
    return EG(uninitialized_zval_ptr);
  }

  if (Z_OBJCE_P(object) != EG(scope)) {
    // User cannot get property directly (e.g., $a = $m->a)
    zend_error(E_USER_ERROR, "Cannot access private property.");
    return EG(uninitialized_zval_ptr);
  }

  zend_property_info* property_info = NULL;

  // All properties should have been declared in the generated code and have
  // corresponding zvals in properties_table.
  ulong h = zend_get_hash_value(Z_STRVAL_P(member), Z_STRLEN_P(member) + 1);
  if (zend_hash_quick_find(&Z_OBJCE_P(object)->properties_info,
                           Z_STRVAL_P(member), Z_STRLEN_P(member) + 1, h,
                           (void**)&property_info) != SUCCESS) {
    zend_error(E_USER_ERROR, "Property does not exist.");
    return EG(uninitialized_zval_ptr);
  }

  MessageHeader* self =
      (MessageHeader*)zend_object_store_get_object(object TSRMLS_CC);

  const upb_fielddef* field;
  field = upb_msgdef_ntofz(self->descriptor->msgdef, Z_STRVAL_P(member));
  if (field == NULL) {
    return EG(uninitialized_zval_ptr);
  }
  return layout_get(
      self->descriptor->layout, message_data(self), field,
      &Z_OBJ_P(object)->properties_table[property_info->offset] TSRMLS_CC);
}

static zval** message_get_property_ptr_ptr(zval* object, zval* member, int type,
                                           const zend_literal* key TSRMLS_DC) {
  return NULL;
}

static HashTable* message_get_properties(zval* object TSRMLS_DC) {
  return NULL;
}

static HashTable* message_get_gc(zval* object, zval*** table, int* n TSRMLS_DC) {
    zend_object* zobj = Z_OBJ_P(object);
    *table = zobj->properties_table;
    *n = zobj->ce->default_properties_count;
    return NULL;
}

// -----------------------------------------------------------------------------
// C Message Utilities
// -----------------------------------------------------------------------------

void* message_data(void* msg) {
  return ((uint8_t*)msg) + sizeof(MessageHeader);
}

static void message_free(void* object TSRMLS_DC) {
  MessageHeader* msg = (MessageHeader*)object;
  int i;

  for (i = 0; i < msg->std.ce->default_properties_count; i++) {
    zval_ptr_dtor(&msg->std.properties_table[i]);
  }
  efree(msg->std.properties_table);
  efree(msg);
}

static zend_object_value message_create(zend_class_entry* ce TSRMLS_DC) {
  zend_object_value return_value;

  zval* php_descriptor = get_ce_obj(ce);

  Descriptor* desc = zend_object_store_get_object(php_descriptor TSRMLS_CC);
  MessageHeader* msg = (MessageHeader*)ALLOC_N(
      uint8_t, sizeof(MessageHeader) + desc->layout->size);
  memset(message_data(msg), 0, desc->layout->size);

  // We wrap first so that everything in the message object is GC-rooted in
  // case a collection happens during object creation in layout_init().
  msg->descriptor = desc;

  zend_object_std_init(&msg->std, ce TSRMLS_CC);
  object_properties_init(&msg->std, ce);
  layout_init(desc->layout, message_data(msg),
              msg->std.properties_table TSRMLS_CC);

  return_value.handle = zend_objects_store_put(
      msg, (zend_objects_store_dtor_t)zend_objects_destroy_object, message_free,
      NULL TSRMLS_CC);

  return_value.handlers = message_handlers;
  return return_value;
}

void build_class_from_descriptor(zval* php_descriptor TSRMLS_DC) {
  Descriptor* desc = UNBOX(Descriptor, php_descriptor);

  // Map entries don't have existing php class.
  if (upb_msgdef_mapentry(desc->msgdef)) {
    return;
  }

  zend_class_entry* registered_ce = desc->klass;

  if (desc->layout == NULL) {
    MessageLayout* layout = create_layout(desc->msgdef);
    desc->layout = layout;
  }

  registered_ce->create_object = message_create;
}

// -----------------------------------------------------------------------------
// PHP Methods
// -----------------------------------------------------------------------------

// At the first time the message is created, the class entry hasn't been
// modified. As a result, the first created instance will be a normal zend
// object. Here, we manually modify it to our message in such a case.
PHP_METHOD(Message, __construct) {
  if (Z_OBJVAL_P(getThis()).handlers != message_handlers) {
    zend_class_entry* ce = Z_OBJCE_P(getThis());
    zval_dtor(getThis());
    Z_OBJVAL_P(getThis()) = message_create(ce TSRMLS_CC);
  }
}

PHP_METHOD(Message, clear) {
  MessageHeader* msg =
      (MessageHeader*)zend_object_store_get_object(getThis() TSRMLS_CC);
  Descriptor* desc = msg->descriptor;
  zend_class_entry* ce = desc->klass;
  int i;

  for (i = 0; i < msg->std.ce->default_properties_count; i++) {
    zval_ptr_dtor(&msg->std.properties_table[i]);
  }
  efree(msg->std.properties_table);

  zend_object_std_init(&msg->std, ce TSRMLS_CC);
  object_properties_init(&msg->std, ce);
  layout_init(desc->layout, message_data(msg),
              msg->std.properties_table TSRMLS_CC);
}

PHP_METHOD(Message, readOneof) {
  long index;

  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l", &index) ==
      FAILURE) {
    return;
  }

  MessageHeader* msg =
      (MessageHeader*)zend_object_store_get_object(getThis() TSRMLS_CC);

  const upb_fielddef* field = upb_msgdef_itof(msg->descriptor->msgdef, index);

  int property_cache_index =
      msg->descriptor->layout->fields[upb_fielddef_index(field)].cache_index;
  zval** cache_ptr = &(msg->std.properties_table)[property_cache_index];

  layout_get(msg->descriptor->layout, message_data(msg), field,
             &return_value TSRMLS_CC);
}

PHP_METHOD(Message, writeOneof) {
  long index;
  zval* value;
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "lz", &index, &value) ==
      FAILURE) {
    return;
  }

  MessageHeader* msg =
      (MessageHeader*)zend_object_store_get_object(getThis() TSRMLS_CC);

  const upb_fielddef* field = upb_msgdef_itof(msg->descriptor->msgdef, index);

  layout_set(msg->descriptor->layout, msg, field, value TSRMLS_CC);
}

PHP_METHOD(Message, whichOneof) {
  char* oneof_name;
  int length;

  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &oneof_name,
                            &length) == FAILURE) {
    return;
  }

  MessageHeader* msg =
      (MessageHeader*)zend_object_store_get_object(getThis() TSRMLS_CC);

  const upb_oneofdef* oneof =
      upb_msgdef_ntoo(msg->descriptor->msgdef, oneof_name, length);
  const char* oneof_case_name = layout_get_oneof_case(
      msg->descriptor->layout, message_data(msg), oneof TSRMLS_CC);
  RETURN_STRING(oneof_case_name, 1);
}
