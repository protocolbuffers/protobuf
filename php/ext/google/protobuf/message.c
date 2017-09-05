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
#include <ext/date/php_date.h>
#include <stdlib.h>

#include "protobuf.h"
#include "utf8.h"

zend_class_entry* message_type;
zend_object_handlers* message_handlers;
static const char TYPE_URL_PREFIX[] = "type.googleapis.com/";

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

static void message_set_property_internal(zval* object, zval* member,
                                          zval* value TSRMLS_DC) {
  const upb_fielddef* field;

  MessageHeader* self = UNBOX(MessageHeader, object);

  field = upb_msgdef_ntofz(self->descriptor->msgdef, Z_STRVAL_P(member));
  if (field == NULL) {
    zend_error(E_USER_ERROR, "Unknown field: %s", Z_STRVAL_P(member));
  }

  layout_set(self->descriptor->layout, self, field, value TSRMLS_CC);
}

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

  message_set_property_internal(object, member, value TSRMLS_CC);
}

static zval* message_get_property_internal(zval* object,
                                           zval* member TSRMLS_DC) {
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

  return message_get_property_internal(object, member TSRMLS_CC);
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

// -----------------------------------------------------------------------------
// Well Known Types Support
// -----------------------------------------------------------------------------

#define PHP_PROTO_FIELD_ACCESSORS(UPPER_CLASS, LOWER_CLASS, UPPER_FIELD,       \
                                  LOWER_FIELD)                                 \
  PHP_METHOD(UPPER_CLASS, get##UPPER_FIELD) {                                  \
    zval member;                                                               \
    PHP_PROTO_ZVAL_STRING(&member, LOWER_FIELD, 1);                            \
    PHP_PROTO_FAKE_SCOPE_BEGIN(LOWER_CLASS##_type);                            \
    zval* value = message_get_property_internal(getThis(), &member TSRMLS_CC); \
    PHP_PROTO_FAKE_SCOPE_END;                                                  \
    PHP_PROTO_RETVAL_ZVAL(value);                                              \
  }                                                                            \
  PHP_METHOD(UPPER_CLASS, set##UPPER_FIELD) {                                  \
    zval* value = NULL;                                                        \
    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z", &value) ==       \
        FAILURE) {                                                             \
      return;                                                                  \
    }                                                                          \
    zval member;                                                               \
    PHP_PROTO_ZVAL_STRING(&member, LOWER_FIELD, 1);                            \
    message_set_property_internal(getThis(), &member, value TSRMLS_CC);        \
    PHP_PROTO_RETVAL_ZVAL(getThis());                                          \
  }

// -----------------------------------------------------------------------------
// Any
// -----------------------------------------------------------------------------

static  zend_function_entry any_methods[] = {
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

zend_class_entry* any_type;

// Init class entry.
PHP_PROTO_INIT_SUBMSGCLASS_START("Google\\Protobuf\\Any", Any, any)
  zend_class_implements(any_type TSRMLS_CC, 1, message_type);
  zend_declare_property_string(any_type, "type_url", strlen("type_url"),
                               "" ,ZEND_ACC_PRIVATE TSRMLS_CC);
  zend_declare_property_string(any_type, "value", strlen("value"),
                               "" ,ZEND_ACC_PRIVATE TSRMLS_CC);
PHP_PROTO_INIT_SUBMSGCLASS_END

void hex_to_binary(const char* hex, char** binary, int* binary_len) {
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

PHP_METHOD(Any, __construct) {
  PHP_PROTO_HASHTABLE_VALUE desc_php = get_ce_obj(any_type);
  if (desc_php == NULL) {
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

    internal_add_generated_file(binary, binary_len, generated_pool TSRMLS_CC);
    FREE(binary);
  }

  MessageHeader* intern = UNBOX(MessageHeader, getThis());
  custom_data_init(any_type, intern PHP_PROTO_TSRMLS_CC);
}

PHP_PROTO_FIELD_ACCESSORS(Any, any, TypeUrl, "type_url")
PHP_PROTO_FIELD_ACCESSORS(Any, any, Value,   "value")

PHP_METHOD(Any, unpack) {
  // Get type url.
  zval type_url_member;
  PHP_PROTO_ZVAL_STRING(&type_url_member, "type_url", 1);
  PHP_PROTO_FAKE_SCOPE_BEGIN(any_type);
  zval* type_url_php = php_proto_message_read_property(
      getThis(), &type_url_member PHP_PROTO_TSRMLS_CC);
  PHP_PROTO_FAKE_SCOPE_END;

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
  PHP_PROTO_HASHTABLE_VALUE desc_php = get_proto_obj(fully_qualified_name);
  if (desc_php == NULL) {
    zend_throw_exception(
        NULL, "Specified message in any hasn't been added to descriptor pool",
        0 TSRMLS_CC);
    return;
  }
  Descriptor* desc = UNBOX_HASHTABLE_VALUE(Descriptor, desc_php);
  zend_class_entry* klass = desc->klass;
  ZVAL_OBJ(return_value, klass->create_object(klass TSRMLS_CC));
  MessageHeader* msg = UNBOX(MessageHeader, return_value);
  custom_data_init(klass, msg PHP_PROTO_TSRMLS_CC);

  // Get value.
  zval value_member;
  PHP_PROTO_ZVAL_STRING(&value_member, "value", 1);
  PHP_PROTO_FAKE_SCOPE_RESTART(any_type);
  zval* value = php_proto_message_read_property(
      getThis(), &value_member PHP_PROTO_TSRMLS_CC);
  PHP_PROTO_FAKE_SCOPE_END;

  merge_from_string(Z_STRVAL_P(value), Z_STRLEN_P(value), desc, msg);
}

PHP_METHOD(Any, pack) {
  zval* val;

  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "o", &val) ==
      FAILURE) {
    return;
  }

  if (!instanceof_function(Z_OBJCE_P(val), message_type TSRMLS_CC)) {
    zend_error(E_USER_ERROR, "Given value is not an instance of Message.");
    return;
  }

  // Set value by serialized data.
  zval data;
  serialize_to_string(val, &data TSRMLS_CC);

  zval member;
  PHP_PROTO_ZVAL_STRING(&member, "value", 1);

  PHP_PROTO_FAKE_SCOPE_BEGIN(any_type);
  message_handlers->write_property(getThis(), &member, &data,
                                   NULL PHP_PROTO_TSRMLS_CC);
  PHP_PROTO_FAKE_SCOPE_END;

  // Set type url.
  Descriptor* desc =
      UNBOX_HASHTABLE_VALUE(Descriptor, get_ce_obj(Z_OBJCE_P(val)));
  const char* fully_qualified_name = upb_msgdef_fullname(desc->msgdef);
  size_t type_url_len =
      strlen(TYPE_URL_PREFIX) + strlen(fully_qualified_name) + 1;
  char* type_url = ALLOC_N(char, type_url_len);
  sprintf(type_url, "%s%s", TYPE_URL_PREFIX, fully_qualified_name);
  zval type_url_php;
  PHP_PROTO_ZVAL_STRING(&type_url_php, type_url, 1);
  PHP_PROTO_ZVAL_STRING(&member, "type_url", 1);

  PHP_PROTO_FAKE_SCOPE_RESTART(any_type);
  message_handlers->write_property(getThis(), &member, &type_url_php,
                                   NULL PHP_PROTO_TSRMLS_CC);
  PHP_PROTO_FAKE_SCOPE_END;
  FREE(type_url);
}

PHP_METHOD(Any, is) {
  zend_class_entry *klass = NULL;

  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "C", &klass) ==
      FAILURE) {
    return;
  }

  PHP_PROTO_HASHTABLE_VALUE desc_php = get_ce_obj(klass);
  if (desc_php == NULL) {
    RETURN_BOOL(false);
  }

  // Create corresponded type url.
  Descriptor* desc =
      UNBOX_HASHTABLE_VALUE(Descriptor, get_ce_obj(klass));
  const char* fully_qualified_name = upb_msgdef_fullname(desc->msgdef);
  size_t type_url_len =
      strlen(TYPE_URL_PREFIX) + strlen(fully_qualified_name) + 1;
  char* type_url = ALLOC_N(char, type_url_len);
  sprintf(type_url, "%s%s", TYPE_URL_PREFIX, fully_qualified_name);

  // Fetch stored type url.
  zval member;
  PHP_PROTO_ZVAL_STRING(&member, "type_url", 1);
  PHP_PROTO_FAKE_SCOPE_BEGIN(any_type);
  zval* value =
      php_proto_message_read_property(getThis(), &member PHP_PROTO_TSRMLS_CC);
  PHP_PROTO_FAKE_SCOPE_END;

  // Compare two type url.
  bool is = strcmp(type_url, Z_STRVAL_P(value)) == 0;
  FREE(type_url);

  RETURN_BOOL(is);
}

// -----------------------------------------------------------------------------
// Duration
// -----------------------------------------------------------------------------

static  zend_function_entry duration_methods[] = {
  PHP_ME(Duration, __construct, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(Duration, getSeconds, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(Duration, setSeconds, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(Duration, getNanos, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(Duration, setNanos, NULL, ZEND_ACC_PUBLIC)
  {NULL, NULL, NULL}
};

zend_class_entry* duration_type;

// Init class entry.
PHP_PROTO_INIT_SUBMSGCLASS_START("Google\\Protobuf\\Duration",
                                 Duration, duration)
  zend_class_implements(duration_type TSRMLS_CC, 1, message_type);
  zend_declare_property_long(duration_type, "seconds", strlen("seconds"),
                             0 ,ZEND_ACC_PRIVATE TSRMLS_CC);
  zend_declare_property_long(duration_type, "nanos", strlen("nanos"),
                             0 ,ZEND_ACC_PRIVATE TSRMLS_CC);
PHP_PROTO_INIT_SUBMSGCLASS_END

PHP_METHOD(Duration, __construct) {
  PHP_PROTO_HASHTABLE_VALUE desc_php = get_ce_obj(duration_type);
  if (desc_php == NULL) {
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

    internal_add_generated_file(binary, binary_len, generated_pool TSRMLS_CC);
    FREE(binary);
  }

  MessageHeader* intern = UNBOX(MessageHeader, getThis());
  custom_data_init(duration_type, intern PHP_PROTO_TSRMLS_CC);
}

PHP_PROTO_FIELD_ACCESSORS(Duration, duration, Seconds, "seconds")
PHP_PROTO_FIELD_ACCESSORS(Duration, duration, Nanos,   "nanos")

// -----------------------------------------------------------------------------
// Timestamp
// -----------------------------------------------------------------------------

static  zend_function_entry timestamp_methods[] = {
  PHP_ME(Timestamp, __construct, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(Timestamp, fromDateTime, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(Timestamp, toDateTime, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(Timestamp, getSeconds, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(Timestamp, setSeconds, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(Timestamp, getNanos, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(Timestamp, setNanos, NULL, ZEND_ACC_PUBLIC)
  {NULL, NULL, NULL}
};

zend_class_entry* timestamp_type;

// Init class entry.
PHP_PROTO_INIT_SUBMSGCLASS_START("Google\\Protobuf\\Timestamp",
                                 Timestamp, timestamp)
  zend_class_implements(timestamp_type TSRMLS_CC, 1, message_type);
  zend_declare_property_long(timestamp_type, "seconds", strlen("seconds"),
                             0 ,ZEND_ACC_PRIVATE TSRMLS_CC);
  zend_declare_property_long(timestamp_type, "nanos", strlen("nanos"),
                             0 ,ZEND_ACC_PRIVATE TSRMLS_CC);
PHP_PROTO_INIT_SUBMSGCLASS_END

PHP_METHOD(Timestamp, __construct) {
  PHP_PROTO_HASHTABLE_VALUE desc_php = get_ce_obj(timestamp_type);
  if (desc_php == NULL) {
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

    internal_add_generated_file(binary, binary_len, generated_pool TSRMLS_CC);
    FREE(binary);
  }

  MessageHeader* intern = UNBOX(MessageHeader, getThis());
  custom_data_init(timestamp_type, intern PHP_PROTO_TSRMLS_CC);
}

PHP_PROTO_FIELD_ACCESSORS(Timestamp, timestamp, Seconds, "seconds")
PHP_PROTO_FIELD_ACCESSORS(Timestamp, timestamp, Nanos,   "nanos")

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
  MessageHeader* self = UNBOX(MessageHeader, getThis());
  const upb_fielddef* field =
      upb_msgdef_ntofz(self->descriptor->msgdef, "seconds");
  void* storage = message_data(self);
  void* memory = slot_memory(self->descriptor->layout, storage, field);
  *(int64_t*)memory = dateobj->time->sse;

  // Set nanos
  field = upb_msgdef_ntofz(self->descriptor->msgdef, "nanos");
  storage = message_data(self);
  memory = slot_memory(self->descriptor->layout, storage, field);
  *(int32_t*)memory = 0;
}

PHP_METHOD(Timestamp, toDateTime) {
  zval datetime;
  php_date_instantiate(php_date_get_date_ce(), &datetime TSRMLS_CC);
  php_date_obj* dateobj = UNBOX(php_date_obj, &datetime);

  // Get seconds
  MessageHeader* self = UNBOX(MessageHeader, getThis());
  const upb_fielddef* field =
      upb_msgdef_ntofz(self->descriptor->msgdef, "seconds");
  void* storage = message_data(self);
  void* memory = slot_memory(self->descriptor->layout, storage, field);
  int64_t seconds = *(int64_t*)memory;

  // Get nanos
  field = upb_msgdef_ntofz(self->descriptor->msgdef, "nanos");
  memory = slot_memory(self->descriptor->layout, storage, field);
  int32_t nanos = *(int32_t*)memory;

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
  PHP_PROTO_RETVAL_ZVAL(datetime_ptr);
}
