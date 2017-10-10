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

#include "protobuf.h"

static zend_class_entry* message_type;
zend_object_handlers* message_handlers;

static  zend_function_entry message_methods[] = {
  PHP_ME(Message, clear, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(Message, serializeToString, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(Message, mergeFromString, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(Message, serializeToJsonString, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(Message, mergeFromJsonString, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(Message, mergeFrom, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(Message, readOneof, NULL, ZEND_ACC_PROTECTED)
  PHP_ME(Message, writeOneof, NULL, ZEND_ACC_PROTECTED)
  PHP_ME(Message, whichOneof, NULL, ZEND_ACC_PROTECTED)
  PHP_ME(Message, __construct, NULL, ZEND_ACC_PROTECTED)
  {NULL, NULL, NULL}
};

// Forward declare static functions.

#if PHP_MAJOR_VERSION < 7
static void message_set_property(zval* object, zval* member, zval* value,
                                 php_proto_zend_literal key TSRMLS_DC);
static zval* message_get_property(zval* object, zval* member, int type,
                                  const zend_literal* key TSRMLS_DC);
static zval** message_get_property_ptr_ptr(zval* object, zval* member, int type,
                                           php_proto_zend_literal key TSRMLS_DC);
static HashTable* message_get_gc(zval* object, zval*** table, int* n TSRMLS_DC);
#else
static void message_set_property(zval* object, zval* member, zval* value,
                                 void** cache_slot);
static zval* message_get_property(zval* object, zval* member, int type,
                                  void** cache_slot, zval* rv);
static zval* message_get_property_ptr_ptr(zval* object, zval* member, int type,
                                          void** cache_slot);
static HashTable* message_get_gc(zval* object, zval** table, int* n);
#endif
static HashTable* message_get_properties(zval* object TSRMLS_DC);

// -----------------------------------------------------------------------------
// PHP Message Handlers
// -----------------------------------------------------------------------------

// Define object free method.
PHP_PROTO_OBJECT_FREE_START(MessageHeader, message)
  FREE(intern->data);
PHP_PROTO_OBJECT_FREE_END

PHP_PROTO_OBJECT_DTOR_START(MessageHeader, message)
PHP_PROTO_OBJECT_DTOR_END

// Define object create method.
PHP_PROTO_OBJECT_CREATE_START(MessageHeader, message)
// Because php call this create func before calling the sub-message's
// constructor defined in PHP, it's possible that the decriptor of this class
// hasn't been added to descritpor pool (when the class is first
// instantiated). In that case, we will defer the initialization of the custom
// data to the parent Message's constructor, which will be called by
// sub-message's constructors after the descriptor has been added.
PHP_PROTO_OBJECT_CREATE_END(MessageHeader, message)

// Init class entry.
PHP_PROTO_INIT_CLASS_START("Google\\Protobuf\\Internal\\Message",
                           MessageHeader, message)
  message_handlers->write_property = message_set_property;
  message_handlers->read_property = message_get_property;
  message_handlers->get_property_ptr_ptr = message_get_property_ptr_ptr;
  message_handlers->get_properties = message_get_properties;
  message_handlers->get_gc = message_get_gc;
PHP_PROTO_INIT_CLASS_END

#if PHP_MAJOR_VERSION < 7
static void message_set_property(zval* object, zval* member, zval* value,
                                 php_proto_zend_literal key TSRMLS_DC) {
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

  const upb_fielddef* field;

  MessageHeader* self = UNBOX(MessageHeader, object);

  field = upb_msgdef_ntofz(self->descriptor->msgdef, Z_STRVAL_P(member));
  if (field == NULL) {
    zend_error(E_USER_ERROR, "Unknown field: %s", Z_STRVAL_P(member));
  }

  layout_set(self->descriptor->layout, self, field, value TSRMLS_CC);
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
    return PHP_PROTO_GLOBAL_UNINITIALIZED_ZVAL;
  }

#if PHP_MAJOR_VERSION < 7 || (PHP_MAJOR_VERSION == 7 && PHP_MINOR_VERSION == 0)
  if (Z_OBJCE_P(object) != EG(scope)) {
#else
  if (Z_OBJCE_P(object) != zend_get_executed_scope()) {
#endif
    // User cannot get property directly (e.g., $a = $m->a)
    zend_error(E_USER_ERROR, "Cannot access private property.");
    return PHP_PROTO_GLOBAL_UNINITIALIZED_ZVAL;
  }

  MessageHeader* self = UNBOX(MessageHeader, object);
  const upb_fielddef* field;
  field = upb_msgdef_ntofz(self->descriptor->msgdef, Z_STRVAL_P(member));
  if (field == NULL) {
    return PHP_PROTO_GLOBAL_UNINITIALIZED_ZVAL;
  }

  zend_property_info* property_info;
#if PHP_MAJOR_VERSION < 7
  property_info =
      zend_get_property_info(Z_OBJCE_P(object), member, true TSRMLS_CC);
  return layout_get(
      self->descriptor->layout, message_data(self), field,
      OBJ_PROP(Z_OBJ_P(object), property_info->offset) TSRMLS_CC);
#else
  property_info =
      zend_get_property_info(Z_OBJCE_P(object), Z_STR_P(member), true);
  return layout_get(
      self->descriptor->layout, message_data(self), field,
      OBJ_PROP(Z_OBJ_P(object), property_info->offset) TSRMLS_CC);
#endif
}

#if PHP_MAJOR_VERSION < 7
static zval** message_get_property_ptr_ptr(zval* object, zval* member, int type,
                                           php_proto_zend_literal key
                                               TSRMLS_DC) {
#else
static zval* message_get_property_ptr_ptr(zval* object, zval* member, int type,
                                          void** cache_slot) {
#endif
  return NULL;
}

static HashTable* message_get_properties(zval* object TSRMLS_DC) {
  return NULL;
}

static HashTable* message_get_gc(zval* object, CACHED_VALUE** table,
                                 int* n TSRMLS_DC) {
  zend_object* zobj = Z_OBJ_P(object);
  *table = zobj->properties_table;
  *n = zobj->ce->default_properties_count;
  return NULL;
}

// -----------------------------------------------------------------------------
// C Message Utilities
// -----------------------------------------------------------------------------

void* message_data(MessageHeader* msg) {
  return msg->data;
}

void custom_data_init(const zend_class_entry* ce,
                      MessageHeader* intern PHP_PROTO_TSRMLS_DC) {
  Descriptor* desc = UNBOX_HASHTABLE_VALUE(Descriptor, get_ce_obj(ce));
  intern->data = ALLOC_N(uint8_t, desc->layout->size);
  memset(message_data(intern), 0, desc->layout->size);
  // We wrap first so that everything in the message object is GC-rooted in
  // case a collection happens during object creation in layout_init().
  intern->descriptor = desc;
  layout_init(desc->layout, message_data(intern),
              &intern->std PHP_PROTO_TSRMLS_CC);
}

void build_class_from_descriptor(
    PHP_PROTO_HASHTABLE_VALUE php_descriptor TSRMLS_DC) {
  Descriptor* desc = UNBOX_HASHTABLE_VALUE(Descriptor, php_descriptor);

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
  zend_class_entry* ce = Z_OBJCE_P(getThis());
  if (EXPECTED(class_added(ce))) {
    MessageHeader* intern = UNBOX(MessageHeader, getThis());
    custom_data_init(ce, intern PHP_PROTO_TSRMLS_CC);
  }
}

PHP_METHOD(Message, clear) {
  MessageHeader* msg = UNBOX(MessageHeader, getThis());
  Descriptor* desc = msg->descriptor;
  zend_class_entry* ce = desc->klass;

  object_properties_init(&msg->std, ce);
  layout_init(desc->layout, message_data(msg), &msg->std TSRMLS_CC);
}

PHP_METHOD(Message, mergeFrom) {
  zval* value;
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "O", &value,
                            message_type) == FAILURE) {
    return;
  }

  MessageHeader* from = UNBOX(MessageHeader, value);
  MessageHeader* to = UNBOX(MessageHeader, getThis());

  if(from->descriptor != to->descriptor) {
    zend_error(E_USER_ERROR, "Cannot merge messages with different class.");
    return;
  }

  layout_merge(from->descriptor->layout, from, to TSRMLS_CC);
}

PHP_METHOD(Message, readOneof) {
  PHP_PROTO_LONG index;

  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l", &index) ==
      FAILURE) {
    return;
  }

  MessageHeader* msg = UNBOX(MessageHeader, getThis());

  const upb_fielddef* field = upb_msgdef_itof(msg->descriptor->msgdef, index);

  int property_cache_index =
      msg->descriptor->layout->fields[upb_fielddef_index(field)].cache_index;
  zval* property_ptr = CACHED_PTR_TO_ZVAL_PTR(
      OBJ_PROP(Z_OBJ_P(getThis()), property_cache_index));

  // Unlike singular fields, oneof fields share cached property. So we cannot
  // let lay_get modify the cached property. Instead, we pass in the return
  // value directly.
  layout_get(msg->descriptor->layout, message_data(msg), field,
             ZVAL_PTR_TO_CACHED_PTR(return_value) TSRMLS_CC);
}

PHP_METHOD(Message, writeOneof) {
  PHP_PROTO_LONG index;
  zval* value;
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "lz", &index, &value) ==
      FAILURE) {
    return;
  }

  MessageHeader* msg = UNBOX(MessageHeader, getThis());

  const upb_fielddef* field = upb_msgdef_itof(msg->descriptor->msgdef, index);

  layout_set(msg->descriptor->layout, msg, field, value TSRMLS_CC);
}

PHP_METHOD(Message, whichOneof) {
  char* oneof_name;
  PHP_PROTO_SIZE length;

  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &oneof_name,
                            &length) == FAILURE) {
    return;
  }

  MessageHeader* msg = UNBOX(MessageHeader, getThis());

  const upb_oneofdef* oneof =
      upb_msgdef_ntoo(msg->descriptor->msgdef, oneof_name, length);
  const char* oneof_case_name = layout_get_oneof_case(
      msg->descriptor->layout, message_data(msg), oneof TSRMLS_CC);
  PHP_PROTO_RETURN_STRING(oneof_case_name, 1);
}
