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
#include <inttypes.h>

#include "protobuf.h"
#include "utf8.h"

zend_class_entry* message_type;
zend_object_handlers* message_handlers;
static const char TYPE_URL_PREFIX[] = "type.googleapis.com/";
static void hex_to_binary(const char* hex, char** binary, int* binary_len);

static  zend_function_entry message_methods[] = {
  PHP_ME(Message, clear, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(Message, discardUnknownFields, NULL, ZEND_ACC_PUBLIC)
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
  if (*(void**)intern->data != NULL) {
    stringsink_uninit(*(void**)intern->data);
    FREE(*(void**)intern->data);
  }
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
#else
  property_info =
      zend_get_property_info(Z_OBJCE_P(object), Z_STR_P(member), true);
#endif
  return layout_get(
      self->descriptor->layout, message_data(self), field,
      OBJ_PROP(Z_OBJ_P(object), property_info->offset) TSRMLS_CC);
}

static void message_get_oneof_property_internal(zval* object, zval* member,
                                                zval* return_value TSRMLS_DC) {
  MessageHeader* self = UNBOX(MessageHeader, object);
  const upb_fielddef* field;
  field = upb_msgdef_ntofz(self->descriptor->msgdef, Z_STRVAL_P(member));
  if (field == NULL) {
    return;
  }

  layout_get(self->descriptor->layout, message_data(self), field,
             ZVAL_PTR_TO_CACHED_PTR(return_value) TSRMLS_CC);
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

#define INIT_MESSAGE_WITH_ARRAY                                    \
  {                                                                \
    zval* array_wrapper = NULL;                                    \
    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC,           \
                              "|a!", &array_wrapper) == FAILURE) { \
      return;                                                      \
    }                                                              \
    Message_construct(getThis(), array_wrapper);                   \
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

void Message_construct(zval* msg, zval* array_wrapper) {
  TSRMLS_FETCH();
  zend_class_entry* ce = Z_OBJCE_P(msg);
  MessageHeader* intern = NULL;
  if (EXPECTED(class_added(ce))) {
    intern = UNBOX(MessageHeader, msg);
    custom_data_init(ce, intern PHP_PROTO_TSRMLS_CC);
  }

  if (array_wrapper == NULL) {
    return;
  }

  HashTable* array = Z_ARRVAL_P(array_wrapper);
  HashPosition pointer;
  zval key;
  void* value;
  const upb_fielddef* field;

  for (zend_hash_internal_pointer_reset_ex(array, &pointer);
       php_proto_zend_hash_get_current_data_ex(array, (void**)&value,
                                               &pointer) == SUCCESS;
       zend_hash_move_forward_ex(array, &pointer)) {
    zend_hash_get_current_key_zval_ex(array, &key, &pointer);
    field = upb_msgdef_ntofz(intern->descriptor->msgdef, Z_STRVAL_P(&key));
    if (field == NULL) {
      zend_error(E_USER_ERROR, "Unknown field: %s", Z_STRVAL_P(&key));
    }
    if (upb_fielddef_ismap(field)) {
      PHP_PROTO_FAKE_SCOPE_BEGIN(Z_OBJCE_P(msg));
      zval* submap = message_get_property_internal(msg, &key TSRMLS_CC);
      PHP_PROTO_FAKE_SCOPE_END;
      HashTable* subtable = HASH_OF(
          CACHED_PTR_TO_ZVAL_PTR((CACHED_VALUE*)value));
      HashPosition subpointer;
      zval subkey;
      void* memory;
      for (zend_hash_internal_pointer_reset_ex(subtable, &subpointer);
           php_proto_zend_hash_get_current_data_ex(subtable, (void**)&memory,
                                                   &subpointer) == SUCCESS;
           zend_hash_move_forward_ex(subtable, &subpointer)) {
        zend_hash_get_current_key_zval_ex(subtable, &subkey, &subpointer);
        map_field_handlers->write_dimension(
            submap, &subkey,
            CACHED_PTR_TO_ZVAL_PTR((CACHED_VALUE*)memory) TSRMLS_CC);
        zval_dtor(&subkey);
      }
    } else if (upb_fielddef_isseq(field)) {
      PHP_PROTO_FAKE_SCOPE_BEGIN(Z_OBJCE_P(msg));
      zval* subarray = message_get_property_internal(msg, &key TSRMLS_CC);
      PHP_PROTO_FAKE_SCOPE_END;
      HashTable* subtable = HASH_OF(
          CACHED_PTR_TO_ZVAL_PTR((CACHED_VALUE*)value));
      HashPosition subpointer;
      void* memory;
      for (zend_hash_internal_pointer_reset_ex(subtable, &subpointer);
           php_proto_zend_hash_get_current_data_ex(subtable, (void**)&memory,
                                                   &subpointer) == SUCCESS;
           zend_hash_move_forward_ex(subtable, &subpointer)) {
        repeated_field_handlers->write_dimension(
            subarray, NULL,
            CACHED_PTR_TO_ZVAL_PTR((CACHED_VALUE*)memory) TSRMLS_CC);
      }
    } else if (upb_fielddef_issubmsg(field)) {
      const upb_msgdef* submsgdef = upb_fielddef_msgsubdef(field);
      PHP_PROTO_HASHTABLE_VALUE desc_php = get_def_obj(submsgdef);
      Descriptor* desc = UNBOX_HASHTABLE_VALUE(Descriptor, desc_php);
      zend_property_info* property_info;
      PHP_PROTO_FAKE_SCOPE_BEGIN(Z_OBJCE_P(msg));
#if PHP_MAJOR_VERSION < 7
      property_info =
          zend_get_property_info(Z_OBJCE_P(msg), &key, true TSRMLS_CC);
#else
      property_info =
          zend_get_property_info(Z_OBJCE_P(msg), Z_STR_P(&key), true);
#endif
      PHP_PROTO_FAKE_SCOPE_END;
      CACHED_VALUE* cached = OBJ_PROP(Z_OBJ_P(msg), property_info->offset);
#if PHP_MAJOR_VERSION < 7
      SEPARATE_ZVAL_IF_NOT_REF(cached);
#endif
      zval* submsg = CACHED_PTR_TO_ZVAL_PTR(cached);
      ZVAL_OBJ(submsg, desc->klass->create_object(desc->klass TSRMLS_CC));
      Message_construct(submsg, NULL);
      MessageHeader* to = UNBOX(MessageHeader, submsg);
      const upb_filedef *file = upb_def_file(upb_msgdef_upcast(submsgdef));
      if (!strcmp(upb_filedef_name(file), "google/protobuf/wrappers.proto") &&
          Z_TYPE_P(CACHED_PTR_TO_ZVAL_PTR((CACHED_VALUE*)value)) != IS_OBJECT) {
        const upb_fielddef *value_field = upb_msgdef_itof(submsgdef, 1);
        layout_set(to->descriptor->layout, to,
                   value_field, CACHED_PTR_TO_ZVAL_PTR((CACHED_VALUE*)value)
                   TSRMLS_CC);
      } else {
        MessageHeader* from =
            UNBOX(MessageHeader,
                  CACHED_PTR_TO_ZVAL_PTR((CACHED_VALUE*)value));
        if(from->descriptor != to->descriptor) {
          zend_error(E_USER_ERROR,
                     "Cannot merge messages with different class.");
          return;
        }

        layout_merge(from->descriptor->layout, from, to TSRMLS_CC);
      }
    } else {
      message_set_property_internal(msg, &key,
          CACHED_PTR_TO_ZVAL_PTR((CACHED_VALUE*)value) TSRMLS_CC);
    }
    zval_dtor(&key);
  }
}

// At the first time the message is created, the class entry hasn't been
// modified. As a result, the first created instance will be a normal zend
// object. Here, we manually modify it to our message in such a case.
PHP_METHOD(Message, __construct) {
  INIT_MESSAGE_WITH_ARRAY;
}

PHP_METHOD(Message, clear) {
  MessageHeader* msg = UNBOX(MessageHeader, getThis());
  Descriptor* desc = msg->descriptor;
  zend_class_entry* ce = desc->klass;

  zend_object_std_dtor(&msg->std TSRMLS_CC);
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
    zval_dtor(&member);                                                        \
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
    zval_dtor(&member);                                                        \
    PHP_PROTO_RETVAL_ZVAL(getThis());                                          \
  }

#define PHP_PROTO_ONEOF_FIELD_ACCESSORS(UPPER_CLASS, LOWER_CLASS, UPPER_FIELD, \
                                        LOWER_FIELD)                           \
  PHP_METHOD(UPPER_CLASS, get##UPPER_FIELD) {                                  \
    zval member;                                                               \
    PHP_PROTO_ZVAL_STRING(&member, LOWER_FIELD, 1);                            \
    PHP_PROTO_FAKE_SCOPE_BEGIN(LOWER_CLASS##_type);                            \
    message_get_oneof_property_internal(getThis(), &member,                    \
                                        return_value TSRMLS_CC);               \
    PHP_PROTO_FAKE_SCOPE_END;                                                  \
    zval_dtor(&member);                                                        \
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
    zval_dtor(&member);                                                        \
    PHP_PROTO_RETVAL_ZVAL(getThis());                                          \
  }

#define PHP_PROTO_ONEOF_ACCESSORS(UPPER_CLASS, LOWER_CLASS, UPPER_FIELD, \
                                  LOWER_FIELD)                           \
  PHP_METHOD(UPPER_CLASS, get##UPPER_FIELD) {                            \
    MessageHeader* msg = UNBOX(MessageHeader, getThis());                \
    PHP_PROTO_FAKE_SCOPE_BEGIN(LOWER_CLASS##_type);                      \
    const upb_oneofdef* oneof = upb_msgdef_ntoo(                         \
        msg->descriptor->msgdef, LOWER_FIELD, strlen(LOWER_FIELD));      \
    const char* oneof_case_name = layout_get_oneof_case(                 \
        msg->descriptor->layout, message_data(msg), oneof TSRMLS_CC);    \
    PHP_PROTO_FAKE_SCOPE_END;                                            \
    PHP_PROTO_RETURN_STRING(oneof_case_name, 1);                         \
  }

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

// Define file init functions
static void init_file_any(TSRMLS_D) {
  if (is_inited_file_any) return;
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
  is_inited_file_any = true;
}

static void init_file_api(TSRMLS_D) {
  if (is_inited_file_api) return;
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
  internal_add_generated_file(binary, binary_len, generated_pool TSRMLS_CC);
  FREE(binary);
  is_inited_file_api = true;
}

static void init_file_duration(TSRMLS_D) {
  if (is_inited_file_duration) return;
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
  is_inited_file_duration = true;
}

static void init_file_field_mask(TSRMLS_D) {
  if (is_inited_file_field_mask) return;
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
  internal_add_generated_file(binary, binary_len, generated_pool TSRMLS_CC);
  FREE(binary);
  is_inited_file_field_mask = true;
}

static void init_file_empty(TSRMLS_D) {
  if (is_inited_file_empty) return;
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
  internal_add_generated_file(binary, binary_len, generated_pool TSRMLS_CC);
  FREE(binary);
  is_inited_file_empty = true;
}

static void init_file_source_context(TSRMLS_D) {
  if (is_inited_file_source_context) return;
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
  internal_add_generated_file(binary, binary_len, generated_pool TSRMLS_CC);
  FREE(binary);
  is_inited_file_source_context = true;
}

static void init_file_struct(TSRMLS_D) {
  if (is_inited_file_struct) return;
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
  internal_add_generated_file(binary, binary_len, generated_pool TSRMLS_CC);
  FREE(binary);
  is_inited_file_struct = true;
}

static void init_file_timestamp(TSRMLS_D) {
  if (is_inited_file_timestamp) return;
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
  is_inited_file_timestamp = true;
}

static void init_file_type(TSRMLS_D) {
  if (is_inited_file_type) return;
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
  internal_add_generated_file(binary, binary_len, generated_pool TSRMLS_CC);
  FREE(binary);
  is_inited_file_type = true;
}

static void init_file_wrappers(TSRMLS_D) {
  if (is_inited_file_wrappers) return;
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
  internal_add_generated_file(binary, binary_len, generated_pool TSRMLS_CC);
  FREE(binary);
  is_inited_file_wrappers = true;
}

// -----------------------------------------------------------------------------
// Define enum
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// Field_Cardinality
// -----------------------------------------------------------------------------

static zend_function_entry field_cardinality_methods[] = {
  PHP_ME(Field_Cardinality, name, NULL, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
  PHP_ME(Field_Cardinality, value, NULL, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
  {NULL, NULL, NULL}
};

zend_class_entry* field_cardinality_type;

// Init class entry.
PHP_PROTO_INIT_ENUMCLASS_START("Google\\Protobuf\\Field\\Cardinality",
                                Field_Cardinality, field_cardinality)
  zend_declare_class_constant_long(field_cardinality_type,
                                   "CARDINALITY_UNKNOWN", 19, 0 TSRMLS_CC);
  zend_declare_class_constant_long(field_cardinality_type,
                                   "CARDINALITY_OPTIONAL", 20, 1 TSRMLS_CC);
  zend_declare_class_constant_long(field_cardinality_type,
                                   "CARDINALITY_REQUIRED", 20, 2 TSRMLS_CC);
  zend_declare_class_constant_long(field_cardinality_type,
                                   "CARDINALITY_REPEATED", 20, 3 TSRMLS_CC);
  const char *alias = "Google\\Protobuf\\Field_Cardinality";
#if PHP_VERSION_ID < 70300
  zend_register_class_alias_ex(alias, strlen(alias), field_cardinality_type TSRMLS_CC);
#else
  zend_register_class_alias_ex(alias, strlen(alias), field_cardinality_type, 1);
#endif
PHP_PROTO_INIT_ENUMCLASS_END

PHP_METHOD(Field_Cardinality, name) {
  PHP_PROTO_LONG value;
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l", &value) ==
      FAILURE) {
    return;
  }
  switch (value) {
    case 0:
      PHP_PROTO_RETURN_STRING("CARDINALITY_UNKNOWN", 1);
    case 1:
      PHP_PROTO_RETURN_STRING("CARDINALITY_OPTIONAL", 1);
    case 2:
      PHP_PROTO_RETURN_STRING("CARDINALITY_REQUIRED", 1);
    case 3:
      PHP_PROTO_RETURN_STRING("CARDINALITY_REPEATED", 1);
    default:
      zend_throw_exception(
          NULL,
          "Enum Google\\Protobuf\\Field_Cardinality has no name "
          "defined for value %d.",
          value,
          0 TSRMLS_CC);
  }
}

PHP_METHOD(Field_Cardinality, value) {
  char *name = NULL;
  PHP_PROTO_SIZE name_len;

  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &name, &name_len) ==
      FAILURE) {
    return;
  }

  if (strncmp(name, "CARDINALITY_UNKNOWN", name_len) == 0) RETURN_LONG(0);
  if (strncmp(name, "CARDINALITY_OPTIONAL", name_len) == 0) RETURN_LONG(1);
  if (strncmp(name, "CARDINALITY_REQUIRED", name_len) == 0) RETURN_LONG(2);
  if (strncmp(name, "CARDINALITY_REPEATED", name_len) == 0) RETURN_LONG(3);

  zend_throw_exception(
      NULL,
      "Enum Google\\Protobuf\\Field_Cardinality has no value "
      "defined for name %s.",
      name,
      0 TSRMLS_CC);
}

// -----------------------------------------------------------------------------
// Field_Kind
// -----------------------------------------------------------------------------

static zend_function_entry field_kind_methods[] = {
  PHP_ME(Field_Kind, name, NULL, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
  PHP_ME(Field_Kind, value, NULL, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
  {NULL, NULL, NULL}
};

zend_class_entry* field_kind_type;

// Init class entry.
PHP_PROTO_INIT_ENUMCLASS_START("Google\\Protobuf\\Field\\Kind",
                                Field_Kind, field_kind)
  zend_declare_class_constant_long(field_kind_type,
                                   "TYPE_UNKNOWN", 12, 0 TSRMLS_CC);
  zend_declare_class_constant_long(field_kind_type,
                                   "TYPE_DOUBLE", 11, 1 TSRMLS_CC);
  zend_declare_class_constant_long(field_kind_type,
                                   "TYPE_FLOAT", 10, 2 TSRMLS_CC);
  zend_declare_class_constant_long(field_kind_type,
                                   "TYPE_INT64", 10, 3 TSRMLS_CC);
  zend_declare_class_constant_long(field_kind_type,
                                   "TYPE_UINT64", 11, 4 TSRMLS_CC);
  zend_declare_class_constant_long(field_kind_type,
                                   "TYPE_INT32", 10, 5 TSRMLS_CC);
  zend_declare_class_constant_long(field_kind_type,
                                   "TYPE_FIXED64", 12, 6 TSRMLS_CC);
  zend_declare_class_constant_long(field_kind_type,
                                   "TYPE_FIXED32", 12, 7 TSRMLS_CC);
  zend_declare_class_constant_long(field_kind_type,
                                   "TYPE_BOOL", 9, 8 TSRMLS_CC);
  zend_declare_class_constant_long(field_kind_type,
                                   "TYPE_STRING", 11, 9 TSRMLS_CC);
  zend_declare_class_constant_long(field_kind_type,
                                   "TYPE_GROUP", 10, 10 TSRMLS_CC);
  zend_declare_class_constant_long(field_kind_type,
                                   "TYPE_MESSAGE", 12, 11 TSRMLS_CC);
  zend_declare_class_constant_long(field_kind_type,
                                   "TYPE_BYTES", 10, 12 TSRMLS_CC);
  zend_declare_class_constant_long(field_kind_type,
                                   "TYPE_UINT32", 11, 13 TSRMLS_CC);
  zend_declare_class_constant_long(field_kind_type,
                                   "TYPE_ENUM", 9, 14 TSRMLS_CC);
  zend_declare_class_constant_long(field_kind_type,
                                   "TYPE_SFIXED32", 13, 15 TSRMLS_CC);
  zend_declare_class_constant_long(field_kind_type,
                                   "TYPE_SFIXED64", 13, 16 TSRMLS_CC);
  zend_declare_class_constant_long(field_kind_type,
                                   "TYPE_SINT32", 11, 17 TSRMLS_CC);
  zend_declare_class_constant_long(field_kind_type,
                                   "TYPE_SINT64", 11, 18 TSRMLS_CC);
  const char *alias = "Google\\Protobuf\\Field_Kind";
#if PHP_VERSION_ID < 70300
  zend_register_class_alias_ex(alias, strlen(alias), field_kind_type TSRMLS_CC);
#else
  zend_register_class_alias_ex(alias, strlen(alias), field_kind_type, 1);
#endif
PHP_PROTO_INIT_ENUMCLASS_END

PHP_METHOD(Field_Kind, name) {
  PHP_PROTO_LONG value;
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l", &value) ==
      FAILURE) {
    return;
  }
  switch (value) {
    case 0:
      PHP_PROTO_RETURN_STRING("TYPE_UNKNOWN", 1);
    case 1:
      PHP_PROTO_RETURN_STRING("TYPE_DOUBLE", 1);
    case 2:
      PHP_PROTO_RETURN_STRING("TYPE_FLOAT", 1);
    case 3:
      PHP_PROTO_RETURN_STRING("TYPE_INT64", 1);
    case 4:
      PHP_PROTO_RETURN_STRING("TYPE_UINT64", 1);
    case 5:
      PHP_PROTO_RETURN_STRING("TYPE_INT32", 1);
    case 6:
      PHP_PROTO_RETURN_STRING("TYPE_FIXED64", 1);
    case 7:
      PHP_PROTO_RETURN_STRING("TYPE_FIXED32", 1);
    case 8:
      PHP_PROTO_RETURN_STRING("TYPE_BOOL", 1);
    case 9:
      PHP_PROTO_RETURN_STRING("TYPE_STRING", 1);
    case 10:
      PHP_PROTO_RETURN_STRING("TYPE_GROUP", 1);
    case 11:
      PHP_PROTO_RETURN_STRING("TYPE_MESSAGE", 1);
    case 12:
      PHP_PROTO_RETURN_STRING("TYPE_BYTES", 1);
    case 13:
      PHP_PROTO_RETURN_STRING("TYPE_UINT32", 1);
    case 14:
      PHP_PROTO_RETURN_STRING("TYPE_ENUM", 1);
    case 15:
      PHP_PROTO_RETURN_STRING("TYPE_SFIXED32", 1);
    case 16:
      PHP_PROTO_RETURN_STRING("TYPE_SFIXED64", 1);
    case 17:
      PHP_PROTO_RETURN_STRING("TYPE_SINT32", 1);
    case 18:
      PHP_PROTO_RETURN_STRING("TYPE_SINT64", 1);
    default:
      zend_throw_exception(
          NULL,
          "Enum Google\\Protobuf\\Field_Kind has no name "
          "defined for value %d.",
          value,
          0 TSRMLS_CC);
  }
}

PHP_METHOD(Field_Kind, value) {
  char *name = NULL;
  PHP_PROTO_SIZE name_len;

  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &name, &name_len) ==
      FAILURE) {
    return;
  }

  if (strncmp(name, "TYPE_UNKNOWN", name_len) == 0) RETURN_LONG(0);
  if (strncmp(name, "TYPE_DOUBLE", name_len) == 0) RETURN_LONG(1);
  if (strncmp(name, "TYPE_FLOAT", name_len) == 0) RETURN_LONG(2);
  if (strncmp(name, "TYPE_INT64", name_len) == 0) RETURN_LONG(3);
  if (strncmp(name, "TYPE_UINT64", name_len) == 0) RETURN_LONG(4);
  if (strncmp(name, "TYPE_INT32", name_len) == 0) RETURN_LONG(5);
  if (strncmp(name, "TYPE_FIXED64", name_len) == 0) RETURN_LONG(6);
  if (strncmp(name, "TYPE_FIXED32", name_len) == 0) RETURN_LONG(7);
  if (strncmp(name, "TYPE_BOOL", name_len) == 0) RETURN_LONG(8);
  if (strncmp(name, "TYPE_STRING", name_len) == 0) RETURN_LONG(9);
  if (strncmp(name, "TYPE_GROUP", name_len) == 0) RETURN_LONG(10);
  if (strncmp(name, "TYPE_MESSAGE", name_len) == 0) RETURN_LONG(11);
  if (strncmp(name, "TYPE_BYTES", name_len) == 0) RETURN_LONG(12);
  if (strncmp(name, "TYPE_UINT32", name_len) == 0) RETURN_LONG(13);
  if (strncmp(name, "TYPE_ENUM", name_len) == 0) RETURN_LONG(14);
  if (strncmp(name, "TYPE_SFIXED32", name_len) == 0) RETURN_LONG(15);
  if (strncmp(name, "TYPE_SFIXED64", name_len) == 0) RETURN_LONG(16);
  if (strncmp(name, "TYPE_SINT32", name_len) == 0) RETURN_LONG(17);
  if (strncmp(name, "TYPE_SINT64", name_len) == 0) RETURN_LONG(18);

  zend_throw_exception(
      NULL,
      "Enum Google\\Protobuf\\Field_Kind has no value "
      "defined for name %s.",
      name,
      0 TSRMLS_CC);
}

// -----------------------------------------------------------------------------
// NullValue
// -----------------------------------------------------------------------------

static zend_function_entry null_value_methods[] = {
  PHP_ME(NullValue, name, NULL, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
  PHP_ME(NullValue, value, NULL, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
  {NULL, NULL, NULL}
};

zend_class_entry* null_value_type;

// Init class entry.
PHP_PROTO_INIT_ENUMCLASS_START("Google\\Protobuf\\NullValue",
                                NullValue, null_value)
  zend_declare_class_constant_long(null_value_type,
                                   "NULL_VALUE", 10, 0 TSRMLS_CC);
PHP_PROTO_INIT_ENUMCLASS_END

PHP_METHOD(NullValue, name) {
  PHP_PROTO_LONG value;
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l", &value) ==
      FAILURE) {
    return;
  }
  switch (value) {
    case 0:
      PHP_PROTO_RETURN_STRING("NULL_VALUE", 1);
    default:
      zend_throw_exception(
          NULL,
          "Enum Google\\Protobuf\\NullValue has no name "
          "defined for value %d.",
          value,
          0 TSRMLS_CC);
  }
}

PHP_METHOD(NullValue, value) {
  char *name = NULL;
  PHP_PROTO_SIZE name_len;

  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &name, &name_len) ==
      FAILURE) {
    return;
  }

  if (strncmp(name, "NULL_VALUE", name_len) == 0) RETURN_LONG(0);

  zend_throw_exception(
      NULL,
      "Enum Google\\Protobuf\\NullValue has no value "
      "defined for name %s.",
      name,
      0 TSRMLS_CC);
}

// -----------------------------------------------------------------------------
// Syntax
// -----------------------------------------------------------------------------

static zend_function_entry syntax_methods[] = {
  PHP_ME(Syntax, name, NULL, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
  PHP_ME(Syntax, value, NULL, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
  {NULL, NULL, NULL}
};

zend_class_entry* syntax_type;

// Init class entry.
PHP_PROTO_INIT_ENUMCLASS_START("Google\\Protobuf\\Syntax",
                                Syntax, syntax)
  zend_declare_class_constant_long(syntax_type,
                                   "SYNTAX_PROTO2", 13, 0 TSRMLS_CC);
  zend_declare_class_constant_long(syntax_type,
                                   "SYNTAX_PROTO3", 13, 1 TSRMLS_CC);
PHP_PROTO_INIT_ENUMCLASS_END

PHP_METHOD(Syntax, name) {
  PHP_PROTO_LONG value;
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l", &value) ==
      FAILURE) {
    return;
  }
  switch (value) {
    case 0:
      PHP_PROTO_RETURN_STRING("SYNTAX_PROTO2", 1);
    case 1:
      PHP_PROTO_RETURN_STRING("SYNTAX_PROTO3", 1);
    default:
      zend_throw_exception(
          NULL,
          "Enum Google\\Protobuf\\Syntax has no name "
          "defined for value %d.",
          value,
          0 TSRMLS_CC);
  }
}

PHP_METHOD(Syntax, value) {
  char *name = NULL;
  PHP_PROTO_SIZE name_len;

  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &name, &name_len) ==
      FAILURE) {
    return;
  }

  if (strncmp(name, "SYNTAX_PROTO2", name_len) == 0) RETURN_LONG(0);
  if (strncmp(name, "SYNTAX_PROTO3", name_len) == 0) RETURN_LONG(1);

  zend_throw_exception(
      NULL,
      "Enum Google\\Protobuf\\Syntax has no value "
      "defined for name %s.",
      name,
      0 TSRMLS_CC);
}

// -----------------------------------------------------------------------------
// Define message
// -----------------------------------------------------------------------------

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

PHP_METHOD(Any, __construct) {
  init_file_any(TSRMLS_C);
  MessageHeader* intern = UNBOX(MessageHeader, getThis());
  INIT_MESSAGE_WITH_ARRAY;
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
  zval_dtor(&type_url_member);
  PHP_PROTO_FAKE_SCOPE_END;

  // Get fully-qualified name from type url.
  size_t url_prefix_len = strlen(TYPE_URL_PREFIX);
  const char* type_url = Z_STRVAL_P(type_url_php);
  size_t type_url_len = Z_STRLEN_P(type_url_php);

  if (url_prefix_len > type_url_len ||
      strncmp(TYPE_URL_PREFIX, type_url, url_prefix_len) != 0) {
    zend_throw_exception(
        NULL, "Type url needs to be type.googleapis.com/fully-qualified",
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
  zval_dtor(&value_member);
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
  zval_dtor(&data);
  zval_dtor(&member);
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
  zval_dtor(&type_url_php);
  zval_dtor(&member);
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
  zval_dtor(&member);
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
  init_file_duration(TSRMLS_C);
  MessageHeader* intern = UNBOX(MessageHeader, getThis());
  INIT_MESSAGE_WITH_ARRAY;
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
  init_file_timestamp(TSRMLS_C);
  MessageHeader* intern = UNBOX(MessageHeader, getThis());
  INIT_MESSAGE_WITH_ARRAY;
}

PHP_PROTO_FIELD_ACCESSORS(Timestamp, timestamp, Seconds, "seconds")
PHP_PROTO_FIELD_ACCESSORS(Timestamp, timestamp, Nanos,   "nanos")

PHP_METHOD(Timestamp, fromDateTime) {
  zval* datetime;
  zval member;

  PHP_PROTO_CE_DECLARE date_interface_ce;
  if (php_proto_zend_lookup_class("\\DatetimeInterface", 18,
                                  &date_interface_ce) == FAILURE) {
    zend_error(E_ERROR, "Make sure date extension is enabled.");
    return;
  }

  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "O", &datetime,
                            PHP_PROTO_CE_UNREF(date_interface_ce)) == FAILURE) {
    zend_error(E_USER_ERROR, "Expect DatetimeInterface.");
    return;
  }

  int64_t timestamp_seconds;
  {
    zval retval;
    zval function_name;

#if PHP_MAJOR_VERSION < 7
    INIT_ZVAL(retval);
    INIT_ZVAL(function_name);
#endif

    PHP_PROTO_ZVAL_STRING(&function_name, "date_timestamp_get", 1);

    if (call_user_function(EG(function_table), NULL, &function_name, &retval, 1,
            ZVAL_PTR_TO_CACHED_PTR(datetime) TSRMLS_CC) == FAILURE) {
      zend_error(E_ERROR, "Cannot get timestamp from DateTime.");
      return;
    }

    protobuf_convert_to_int64(&retval, &timestamp_seconds);

    zval_dtor(&retval);
    zval_dtor(&function_name);
  }

  int64_t timestamp_micros;
  {
    zval retval;
    zval function_name;
    zval format_string;

#if PHP_MAJOR_VERSION < 7
    INIT_ZVAL(retval);
    INIT_ZVAL(function_name);
    INIT_ZVAL(format_string);
#endif

    PHP_PROTO_ZVAL_STRING(&function_name, "date_format", 1);
    PHP_PROTO_ZVAL_STRING(&format_string, "u", 1);

    CACHED_VALUE params[2] = {
      ZVAL_PTR_TO_CACHED_VALUE(datetime),
      ZVAL_TO_CACHED_VALUE(format_string),
    };

    if (call_user_function(EG(function_table), NULL, &function_name, &retval,
            ARRAY_SIZE(params), params TSRMLS_CC) == FAILURE) {
      zend_error(E_ERROR, "Cannot format DateTime.");
      return;
    }

    protobuf_convert_to_int64(&retval, &timestamp_micros);

    zval_dtor(&retval);
    zval_dtor(&function_name);
    zval_dtor(&format_string);
  }

  // Set seconds
  MessageHeader* self = UNBOX(MessageHeader, getThis());
  const upb_fielddef* field =
      upb_msgdef_ntofz(self->descriptor->msgdef, "seconds");
  void* storage = message_data(self);
  void* memory = slot_memory(self->descriptor->layout, storage, field);
  *(int64_t*)memory = timestamp_seconds;

  // Set nanos
  field = upb_msgdef_ntofz(self->descriptor->msgdef, "nanos");
  storage = message_data(self);
  memory = slot_memory(self->descriptor->layout, storage, field);
  *(int32_t*)memory = timestamp_micros * 1000;

  RETURN_NULL();
}

PHP_METHOD(Timestamp, toDateTime) {
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

  // Get formatted time string.
  char formatted_time[32];
  snprintf(formatted_time, sizeof(formatted_time), "%" PRId64 ".%06" PRId32,
           seconds, nanos / 1000);

  // Create Datetime object.
  zval datetime;
  zval function_name;
  zval format_string;
  zval formatted_time_php;

#if PHP_MAJOR_VERSION < 7
  INIT_ZVAL(function_name);
  INIT_ZVAL(format_string);
  INIT_ZVAL(formatted_time_php);
#endif

  PHP_PROTO_ZVAL_STRING(&function_name, "date_create_from_format", 1);
  PHP_PROTO_ZVAL_STRING(&format_string, "U.u", 1);
  PHP_PROTO_ZVAL_STRING(&formatted_time_php, formatted_time, 1);

  CACHED_VALUE params[2] = {
    ZVAL_TO_CACHED_VALUE(format_string),
    ZVAL_TO_CACHED_VALUE(formatted_time_php),
  };

  if (call_user_function(EG(function_table), NULL, &function_name, &datetime,
          ARRAY_SIZE(params), params TSRMLS_CC) == FAILURE) {
    zend_error(E_ERROR, "Cannot create DateTime.");
    return;
  }

  zval_dtor(&function_name);
  zval_dtor(&format_string);
  zval_dtor(&formatted_time_php);

#if PHP_MAJOR_VERSION < 7
  zval* datetime_ptr = &datetime;
  PHP_PROTO_RETVAL_ZVAL(datetime_ptr);
#else
  ZVAL_OBJ(return_value, Z_OBJ(datetime));
#endif
}

// -----------------------------------------------------------------------------
// Api
// -----------------------------------------------------------------------------

static  zend_function_entry api_methods[] = {
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

zend_class_entry* api_type;

// Init class entry.
PHP_PROTO_INIT_SUBMSGCLASS_START("Google\\Protobuf\\Api",
                                 Api, api)
  zend_class_implements(api_type TSRMLS_CC, 1, message_type);
  zend_declare_property_null(api_type, "name", strlen("name"),
                             ZEND_ACC_PRIVATE TSRMLS_CC);
  zend_declare_property_null(api_type, "methods", strlen("methods"),
                             ZEND_ACC_PRIVATE TSRMLS_CC);
  zend_declare_property_null(api_type, "options", strlen("options"),
                             ZEND_ACC_PRIVATE TSRMLS_CC);
  zend_declare_property_null(api_type, "version", strlen("version"),
                             ZEND_ACC_PRIVATE TSRMLS_CC);
  zend_declare_property_null(api_type, "source_context", strlen("source_context"),
                             ZEND_ACC_PRIVATE TSRMLS_CC);
  zend_declare_property_null(api_type, "mixins", strlen("mixins"),
                             ZEND_ACC_PRIVATE TSRMLS_CC);
  zend_declare_property_null(api_type, "syntax", strlen("syntax"),
                             ZEND_ACC_PRIVATE TSRMLS_CC);
PHP_PROTO_INIT_SUBMSGCLASS_END

PHP_METHOD(Api, __construct) {
  init_file_api(TSRMLS_C);
  MessageHeader* intern = UNBOX(MessageHeader, getThis());
  INIT_MESSAGE_WITH_ARRAY;
}

PHP_PROTO_FIELD_ACCESSORS(Api, api, Name, "name")
PHP_PROTO_FIELD_ACCESSORS(Api, api, Methods, "methods")
PHP_PROTO_FIELD_ACCESSORS(Api, api, Options, "options")
PHP_PROTO_FIELD_ACCESSORS(Api, api, Version, "version")
PHP_PROTO_FIELD_ACCESSORS(Api, api, SourceContext, "source_context")
PHP_PROTO_FIELD_ACCESSORS(Api, api, Mixins, "mixins")
PHP_PROTO_FIELD_ACCESSORS(Api, api, Syntax, "syntax")

// -----------------------------------------------------------------------------
// BoolValue
// -----------------------------------------------------------------------------

static  zend_function_entry bool_value_methods[] = {
  PHP_ME(BoolValue, __construct, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(BoolValue, getValue, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(BoolValue, setValue, NULL, ZEND_ACC_PUBLIC)
  {NULL, NULL, NULL}
};

zend_class_entry* bool_value_type;

// Init class entry.
PHP_PROTO_INIT_SUBMSGCLASS_START("Google\\Protobuf\\BoolValue",
                                 BoolValue, bool_value)
  zend_class_implements(bool_value_type TSRMLS_CC, 1, message_type);
  zend_declare_property_null(bool_value_type, "value", strlen("value"),
                             ZEND_ACC_PRIVATE TSRMLS_CC);
PHP_PROTO_INIT_SUBMSGCLASS_END

PHP_METHOD(BoolValue, __construct) {
  init_file_wrappers(TSRMLS_C);
  MessageHeader* intern = UNBOX(MessageHeader, getThis());
  INIT_MESSAGE_WITH_ARRAY;
}

PHP_PROTO_FIELD_ACCESSORS(BoolValue, bool_value, Value, "value")

// -----------------------------------------------------------------------------
// BytesValue
// -----------------------------------------------------------------------------

static  zend_function_entry bytes_value_methods[] = {
  PHP_ME(BytesValue, __construct, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(BytesValue, getValue, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(BytesValue, setValue, NULL, ZEND_ACC_PUBLIC)
  {NULL, NULL, NULL}
};

zend_class_entry* bytes_value_type;

// Init class entry.
PHP_PROTO_INIT_SUBMSGCLASS_START("Google\\Protobuf\\BytesValue",
                                 BytesValue, bytes_value)
  zend_class_implements(bytes_value_type TSRMLS_CC, 1, message_type);
  zend_declare_property_null(bytes_value_type, "value", strlen("value"),
                             ZEND_ACC_PRIVATE TSRMLS_CC);
PHP_PROTO_INIT_SUBMSGCLASS_END

PHP_METHOD(BytesValue, __construct) {
  init_file_wrappers(TSRMLS_C);
  MessageHeader* intern = UNBOX(MessageHeader, getThis());
  INIT_MESSAGE_WITH_ARRAY;
}

PHP_PROTO_FIELD_ACCESSORS(BytesValue, bytes_value, Value, "value")

// -----------------------------------------------------------------------------
// DoubleValue
// -----------------------------------------------------------------------------

static  zend_function_entry double_value_methods[] = {
  PHP_ME(DoubleValue, __construct, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(DoubleValue, getValue, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(DoubleValue, setValue, NULL, ZEND_ACC_PUBLIC)
  {NULL, NULL, NULL}
};

zend_class_entry* double_value_type;

// Init class entry.
PHP_PROTO_INIT_SUBMSGCLASS_START("Google\\Protobuf\\DoubleValue",
                                 DoubleValue, double_value)
  zend_class_implements(double_value_type TSRMLS_CC, 1, message_type);
  zend_declare_property_null(double_value_type, "value", strlen("value"),
                             ZEND_ACC_PRIVATE TSRMLS_CC);
PHP_PROTO_INIT_SUBMSGCLASS_END

PHP_METHOD(DoubleValue, __construct) {
  init_file_wrappers(TSRMLS_C);
  MessageHeader* intern = UNBOX(MessageHeader, getThis());
  INIT_MESSAGE_WITH_ARRAY;
}

PHP_PROTO_FIELD_ACCESSORS(DoubleValue, double_value, Value, "value")

// -----------------------------------------------------------------------------
// Enum
// -----------------------------------------------------------------------------

static  zend_function_entry enum_methods[] = {
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

zend_class_entry* enum_type;

// Init class entry.
PHP_PROTO_INIT_SUBMSGCLASS_START("Google\\Protobuf\\Enum",
                                 Enum, enum)
  zend_class_implements(enum_type TSRMLS_CC, 1, message_type);
  zend_declare_property_null(enum_type, "name", strlen("name"),
                             ZEND_ACC_PRIVATE TSRMLS_CC);
  zend_declare_property_null(enum_type, "enumvalue", strlen("enumvalue"),
                             ZEND_ACC_PRIVATE TSRMLS_CC);
  zend_declare_property_null(enum_type, "options", strlen("options"),
                             ZEND_ACC_PRIVATE TSRMLS_CC);
  zend_declare_property_null(enum_type, "source_context", strlen("source_context"),
                             ZEND_ACC_PRIVATE TSRMLS_CC);
  zend_declare_property_null(enum_type, "syntax", strlen("syntax"),
                             ZEND_ACC_PRIVATE TSRMLS_CC);
PHP_PROTO_INIT_SUBMSGCLASS_END

PHP_METHOD(Enum, __construct) {
  init_file_type(TSRMLS_C);
  MessageHeader* intern = UNBOX(MessageHeader, getThis());
  INIT_MESSAGE_WITH_ARRAY;
}

PHP_PROTO_FIELD_ACCESSORS(Enum, enum, Name, "name")
PHP_PROTO_FIELD_ACCESSORS(Enum, enum, Enumvalue, "enumvalue")
PHP_PROTO_FIELD_ACCESSORS(Enum, enum, Options, "options")
PHP_PROTO_FIELD_ACCESSORS(Enum, enum, SourceContext, "source_context")
PHP_PROTO_FIELD_ACCESSORS(Enum, enum, Syntax, "syntax")

// -----------------------------------------------------------------------------
// EnumValue
// -----------------------------------------------------------------------------

static  zend_function_entry enum_value_methods[] = {
  PHP_ME(EnumValue, __construct, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(EnumValue, getName, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(EnumValue, setName, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(EnumValue, getNumber, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(EnumValue, setNumber, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(EnumValue, getOptions, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(EnumValue, setOptions, NULL, ZEND_ACC_PUBLIC)
  {NULL, NULL, NULL}
};

zend_class_entry* enum_value_type;

// Init class entry.
PHP_PROTO_INIT_SUBMSGCLASS_START("Google\\Protobuf\\EnumValue",
                                 EnumValue, enum_value)
  zend_class_implements(enum_value_type TSRMLS_CC, 1, message_type);
  zend_declare_property_null(enum_value_type, "name", strlen("name"),
                             ZEND_ACC_PRIVATE TSRMLS_CC);
  zend_declare_property_null(enum_value_type, "number", strlen("number"),
                             ZEND_ACC_PRIVATE TSRMLS_CC);
  zend_declare_property_null(enum_value_type, "options", strlen("options"),
                             ZEND_ACC_PRIVATE TSRMLS_CC);
PHP_PROTO_INIT_SUBMSGCLASS_END

PHP_METHOD(EnumValue, __construct) {
  init_file_type(TSRMLS_C);
  MessageHeader* intern = UNBOX(MessageHeader, getThis());
  INIT_MESSAGE_WITH_ARRAY;
}

PHP_PROTO_FIELD_ACCESSORS(EnumValue, enum_value, Name, "name")
PHP_PROTO_FIELD_ACCESSORS(EnumValue, enum_value, Number, "number")
PHP_PROTO_FIELD_ACCESSORS(EnumValue, enum_value, Options, "options")

// -----------------------------------------------------------------------------
// FieldMask
// -----------------------------------------------------------------------------

static  zend_function_entry field_mask_methods[] = {
  PHP_ME(FieldMask, __construct, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(FieldMask, getPaths, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(FieldMask, setPaths, NULL, ZEND_ACC_PUBLIC)
  {NULL, NULL, NULL}
};

zend_class_entry* field_mask_type;

// Init class entry.
PHP_PROTO_INIT_SUBMSGCLASS_START("Google\\Protobuf\\FieldMask",
                                 FieldMask, field_mask)
  zend_class_implements(field_mask_type TSRMLS_CC, 1, message_type);
  zend_declare_property_null(field_mask_type, "paths", strlen("paths"),
                             ZEND_ACC_PRIVATE TSRMLS_CC);
PHP_PROTO_INIT_SUBMSGCLASS_END

PHP_METHOD(FieldMask, __construct) {
  init_file_field_mask(TSRMLS_C);
  MessageHeader* intern = UNBOX(MessageHeader, getThis());
  INIT_MESSAGE_WITH_ARRAY;
}

PHP_PROTO_FIELD_ACCESSORS(FieldMask, field_mask, Paths, "paths")

// -----------------------------------------------------------------------------
// Field
// -----------------------------------------------------------------------------

static  zend_function_entry field_methods[] = {
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

zend_class_entry* field_type;

// Init class entry.
PHP_PROTO_INIT_SUBMSGCLASS_START("Google\\Protobuf\\Field",
                                 Field, field)
  zend_class_implements(field_type TSRMLS_CC, 1, message_type);
  zend_declare_property_null(field_type, "kind", strlen("kind"),
                             ZEND_ACC_PRIVATE TSRMLS_CC);
  zend_declare_property_null(field_type, "cardinality", strlen("cardinality"),
                             ZEND_ACC_PRIVATE TSRMLS_CC);
  zend_declare_property_null(field_type, "number", strlen("number"),
                             ZEND_ACC_PRIVATE TSRMLS_CC);
  zend_declare_property_null(field_type, "name", strlen("name"),
                             ZEND_ACC_PRIVATE TSRMLS_CC);
  zend_declare_property_null(field_type, "type_url", strlen("type_url"),
                             ZEND_ACC_PRIVATE TSRMLS_CC);
  zend_declare_property_null(field_type, "oneof_index", strlen("oneof_index"),
                             ZEND_ACC_PRIVATE TSRMLS_CC);
  zend_declare_property_null(field_type, "packed", strlen("packed"),
                             ZEND_ACC_PRIVATE TSRMLS_CC);
  zend_declare_property_null(field_type, "options", strlen("options"),
                             ZEND_ACC_PRIVATE TSRMLS_CC);
  zend_declare_property_null(field_type, "json_name", strlen("json_name"),
                             ZEND_ACC_PRIVATE TSRMLS_CC);
  zend_declare_property_null(field_type, "default_value", strlen("default_value"),
                             ZEND_ACC_PRIVATE TSRMLS_CC);
PHP_PROTO_INIT_SUBMSGCLASS_END

PHP_METHOD(Field, __construct) {
  init_file_type(TSRMLS_C);
  MessageHeader* intern = UNBOX(MessageHeader, getThis());
  INIT_MESSAGE_WITH_ARRAY;
}

PHP_PROTO_FIELD_ACCESSORS(Field, field, Kind, "kind")
PHP_PROTO_FIELD_ACCESSORS(Field, field, Cardinality, "cardinality")
PHP_PROTO_FIELD_ACCESSORS(Field, field, Number, "number")
PHP_PROTO_FIELD_ACCESSORS(Field, field, Name, "name")
PHP_PROTO_FIELD_ACCESSORS(Field, field, TypeUrl, "type_url")
PHP_PROTO_FIELD_ACCESSORS(Field, field, OneofIndex, "oneof_index")
PHP_PROTO_FIELD_ACCESSORS(Field, field, Packed, "packed")
PHP_PROTO_FIELD_ACCESSORS(Field, field, Options, "options")
PHP_PROTO_FIELD_ACCESSORS(Field, field, JsonName, "json_name")
PHP_PROTO_FIELD_ACCESSORS(Field, field, DefaultValue, "default_value")

// -----------------------------------------------------------------------------
// FloatValue
// -----------------------------------------------------------------------------

static  zend_function_entry float_value_methods[] = {
  PHP_ME(FloatValue, __construct, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(FloatValue, getValue, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(FloatValue, setValue, NULL, ZEND_ACC_PUBLIC)
  {NULL, NULL, NULL}
};

zend_class_entry* float_value_type;

// Init class entry.
PHP_PROTO_INIT_SUBMSGCLASS_START("Google\\Protobuf\\FloatValue",
                                 FloatValue, float_value)
  zend_class_implements(float_value_type TSRMLS_CC, 1, message_type);
  zend_declare_property_null(float_value_type, "value", strlen("value"),
                             ZEND_ACC_PRIVATE TSRMLS_CC);
PHP_PROTO_INIT_SUBMSGCLASS_END

PHP_METHOD(FloatValue, __construct) {
  init_file_wrappers(TSRMLS_C);
  MessageHeader* intern = UNBOX(MessageHeader, getThis());
  INIT_MESSAGE_WITH_ARRAY;
}

PHP_PROTO_FIELD_ACCESSORS(FloatValue, float_value, Value, "value")

// -----------------------------------------------------------------------------
// GPBEmpty
// -----------------------------------------------------------------------------

static  zend_function_entry empty_methods[] = {
  PHP_ME(GPBEmpty, __construct, NULL, ZEND_ACC_PUBLIC)
  {NULL, NULL, NULL}
};

zend_class_entry* empty_type;

// Init class entry.
PHP_PROTO_INIT_SUBMSGCLASS_START("Google\\Protobuf\\GPBEmpty",
                                 GPBEmpty, empty)
  zend_class_implements(empty_type TSRMLS_CC, 1, message_type);
PHP_PROTO_INIT_SUBMSGCLASS_END

PHP_METHOD(GPBEmpty, __construct) {
  init_file_empty(TSRMLS_C);
  MessageHeader* intern = UNBOX(MessageHeader, getThis());
  INIT_MESSAGE_WITH_ARRAY;
}


// -----------------------------------------------------------------------------
// Int32Value
// -----------------------------------------------------------------------------

static  zend_function_entry int32_value_methods[] = {
  PHP_ME(Int32Value, __construct, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(Int32Value, getValue, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(Int32Value, setValue, NULL, ZEND_ACC_PUBLIC)
  {NULL, NULL, NULL}
};

zend_class_entry* int32_value_type;

// Init class entry.
PHP_PROTO_INIT_SUBMSGCLASS_START("Google\\Protobuf\\Int32Value",
                                 Int32Value, int32_value)
  zend_class_implements(int32_value_type TSRMLS_CC, 1, message_type);
  zend_declare_property_null(int32_value_type, "value", strlen("value"),
                             ZEND_ACC_PRIVATE TSRMLS_CC);
PHP_PROTO_INIT_SUBMSGCLASS_END

PHP_METHOD(Int32Value, __construct) {
  init_file_wrappers(TSRMLS_C);
  MessageHeader* intern = UNBOX(MessageHeader, getThis());
  INIT_MESSAGE_WITH_ARRAY;
}

PHP_PROTO_FIELD_ACCESSORS(Int32Value, int32_value, Value, "value")

// -----------------------------------------------------------------------------
// Int64Value
// -----------------------------------------------------------------------------

static  zend_function_entry int64_value_methods[] = {
  PHP_ME(Int64Value, __construct, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(Int64Value, getValue, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(Int64Value, setValue, NULL, ZEND_ACC_PUBLIC)
  {NULL, NULL, NULL}
};

zend_class_entry* int64_value_type;

// Init class entry.
PHP_PROTO_INIT_SUBMSGCLASS_START("Google\\Protobuf\\Int64Value",
                                 Int64Value, int64_value)
  zend_class_implements(int64_value_type TSRMLS_CC, 1, message_type);
  zend_declare_property_null(int64_value_type, "value", strlen("value"),
                             ZEND_ACC_PRIVATE TSRMLS_CC);
PHP_PROTO_INIT_SUBMSGCLASS_END

PHP_METHOD(Int64Value, __construct) {
  init_file_wrappers(TSRMLS_C);
  MessageHeader* intern = UNBOX(MessageHeader, getThis());
  INIT_MESSAGE_WITH_ARRAY;
}

PHP_PROTO_FIELD_ACCESSORS(Int64Value, int64_value, Value, "value")

// -----------------------------------------------------------------------------
// ListValue
// -----------------------------------------------------------------------------

static  zend_function_entry list_value_methods[] = {
  PHP_ME(ListValue, __construct, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(ListValue, getValues, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(ListValue, setValues, NULL, ZEND_ACC_PUBLIC)
  {NULL, NULL, NULL}
};

zend_class_entry* list_value_type;

// Init class entry.
PHP_PROTO_INIT_SUBMSGCLASS_START("Google\\Protobuf\\ListValue",
                                 ListValue, list_value)
  zend_class_implements(list_value_type TSRMLS_CC, 1, message_type);
  zend_declare_property_null(list_value_type, "values", strlen("values"),
                             ZEND_ACC_PRIVATE TSRMLS_CC);
PHP_PROTO_INIT_SUBMSGCLASS_END

PHP_METHOD(ListValue, __construct) {
  init_file_struct(TSRMLS_C);
  MessageHeader* intern = UNBOX(MessageHeader, getThis());
  INIT_MESSAGE_WITH_ARRAY;
}

PHP_PROTO_FIELD_ACCESSORS(ListValue, list_value, Values, "values")

// -----------------------------------------------------------------------------
// Method
// -----------------------------------------------------------------------------

static  zend_function_entry method_methods[] = {
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

zend_class_entry* method_type;

// Init class entry.
PHP_PROTO_INIT_SUBMSGCLASS_START("Google\\Protobuf\\Method",
                                 Method, method)
  zend_class_implements(method_type TSRMLS_CC, 1, message_type);
  zend_declare_property_null(method_type, "name", strlen("name"),
                             ZEND_ACC_PRIVATE TSRMLS_CC);
  zend_declare_property_null(method_type, "request_type_url", strlen("request_type_url"),
                             ZEND_ACC_PRIVATE TSRMLS_CC);
  zend_declare_property_null(method_type, "request_streaming", strlen("request_streaming"),
                             ZEND_ACC_PRIVATE TSRMLS_CC);
  zend_declare_property_null(method_type, "response_type_url", strlen("response_type_url"),
                             ZEND_ACC_PRIVATE TSRMLS_CC);
  zend_declare_property_null(method_type, "response_streaming", strlen("response_streaming"),
                             ZEND_ACC_PRIVATE TSRMLS_CC);
  zend_declare_property_null(method_type, "options", strlen("options"),
                             ZEND_ACC_PRIVATE TSRMLS_CC);
  zend_declare_property_null(method_type, "syntax", strlen("syntax"),
                             ZEND_ACC_PRIVATE TSRMLS_CC);
PHP_PROTO_INIT_SUBMSGCLASS_END

PHP_METHOD(Method, __construct) {
  init_file_api(TSRMLS_C);
  MessageHeader* intern = UNBOX(MessageHeader, getThis());
  INIT_MESSAGE_WITH_ARRAY;
}

PHP_PROTO_FIELD_ACCESSORS(Method, method, Name, "name")
PHP_PROTO_FIELD_ACCESSORS(Method, method, RequestTypeUrl, "request_type_url")
PHP_PROTO_FIELD_ACCESSORS(Method, method, RequestStreaming, "request_streaming")
PHP_PROTO_FIELD_ACCESSORS(Method, method, ResponseTypeUrl, "response_type_url")
PHP_PROTO_FIELD_ACCESSORS(Method, method, ResponseStreaming, "response_streaming")
PHP_PROTO_FIELD_ACCESSORS(Method, method, Options, "options")
PHP_PROTO_FIELD_ACCESSORS(Method, method, Syntax, "syntax")

// -----------------------------------------------------------------------------
// Mixin
// -----------------------------------------------------------------------------

static  zend_function_entry mixin_methods[] = {
  PHP_ME(Mixin, __construct, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(Mixin, getName, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(Mixin, setName, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(Mixin, getRoot, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(Mixin, setRoot, NULL, ZEND_ACC_PUBLIC)
  {NULL, NULL, NULL}
};

zend_class_entry* mixin_type;

// Init class entry.
PHP_PROTO_INIT_SUBMSGCLASS_START("Google\\Protobuf\\Mixin",
                                 Mixin, mixin)
  zend_class_implements(mixin_type TSRMLS_CC, 1, message_type);
  zend_declare_property_null(mixin_type, "name", strlen("name"),
                             ZEND_ACC_PRIVATE TSRMLS_CC);
  zend_declare_property_null(mixin_type, "root", strlen("root"),
                             ZEND_ACC_PRIVATE TSRMLS_CC);
PHP_PROTO_INIT_SUBMSGCLASS_END

PHP_METHOD(Mixin, __construct) {
  init_file_api(TSRMLS_C);
  MessageHeader* intern = UNBOX(MessageHeader, getThis());
  INIT_MESSAGE_WITH_ARRAY;
}

PHP_PROTO_FIELD_ACCESSORS(Mixin, mixin, Name, "name")
PHP_PROTO_FIELD_ACCESSORS(Mixin, mixin, Root, "root")

// -----------------------------------------------------------------------------
// Option
// -----------------------------------------------------------------------------

static  zend_function_entry option_methods[] = {
  PHP_ME(Option, __construct, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(Option, getName, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(Option, setName, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(Option, getValue, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(Option, setValue, NULL, ZEND_ACC_PUBLIC)
  {NULL, NULL, NULL}
};

zend_class_entry* option_type;

// Init class entry.
PHP_PROTO_INIT_SUBMSGCLASS_START("Google\\Protobuf\\Option",
                                 Option, option)
  zend_class_implements(option_type TSRMLS_CC, 1, message_type);
  zend_declare_property_null(option_type, "name", strlen("name"),
                             ZEND_ACC_PRIVATE TSRMLS_CC);
  zend_declare_property_null(option_type, "value", strlen("value"),
                             ZEND_ACC_PRIVATE TSRMLS_CC);
PHP_PROTO_INIT_SUBMSGCLASS_END

PHP_METHOD(Option, __construct) {
  init_file_type(TSRMLS_C);
  MessageHeader* intern = UNBOX(MessageHeader, getThis());
  INIT_MESSAGE_WITH_ARRAY;
}

PHP_PROTO_FIELD_ACCESSORS(Option, option, Name, "name")
PHP_PROTO_FIELD_ACCESSORS(Option, option, Value, "value")

// -----------------------------------------------------------------------------
// SourceContext
// -----------------------------------------------------------------------------

static  zend_function_entry source_context_methods[] = {
  PHP_ME(SourceContext, __construct, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(SourceContext, getFileName, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(SourceContext, setFileName, NULL, ZEND_ACC_PUBLIC)
  {NULL, NULL, NULL}
};

zend_class_entry* source_context_type;

// Init class entry.
PHP_PROTO_INIT_SUBMSGCLASS_START("Google\\Protobuf\\SourceContext",
                                 SourceContext, source_context)
  zend_class_implements(source_context_type TSRMLS_CC, 1, message_type);
  zend_declare_property_null(source_context_type, "file_name", strlen("file_name"),
                             ZEND_ACC_PRIVATE TSRMLS_CC);
PHP_PROTO_INIT_SUBMSGCLASS_END

PHP_METHOD(SourceContext, __construct) {
  init_file_source_context(TSRMLS_C);
  MessageHeader* intern = UNBOX(MessageHeader, getThis());
  INIT_MESSAGE_WITH_ARRAY;
}

PHP_PROTO_FIELD_ACCESSORS(SourceContext, source_context, FileName, "file_name")

// -----------------------------------------------------------------------------
// StringValue
// -----------------------------------------------------------------------------

static  zend_function_entry string_value_methods[] = {
  PHP_ME(StringValue, __construct, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(StringValue, getValue, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(StringValue, setValue, NULL, ZEND_ACC_PUBLIC)
  {NULL, NULL, NULL}
};

zend_class_entry* string_value_type;

// Init class entry.
PHP_PROTO_INIT_SUBMSGCLASS_START("Google\\Protobuf\\StringValue",
                                 StringValue, string_value)
  zend_class_implements(string_value_type TSRMLS_CC, 1, message_type);
  zend_declare_property_null(string_value_type, "value", strlen("value"),
                             ZEND_ACC_PRIVATE TSRMLS_CC);
PHP_PROTO_INIT_SUBMSGCLASS_END

PHP_METHOD(StringValue, __construct) {
  init_file_wrappers(TSRMLS_C);
  MessageHeader* intern = UNBOX(MessageHeader, getThis());
  INIT_MESSAGE_WITH_ARRAY;
}

PHP_PROTO_FIELD_ACCESSORS(StringValue, string_value, Value, "value")

// -----------------------------------------------------------------------------
// Struct
// -----------------------------------------------------------------------------

static  zend_function_entry struct_methods[] = {
  PHP_ME(Struct, __construct, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(Struct, getFields, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(Struct, setFields, NULL, ZEND_ACC_PUBLIC)
  {NULL, NULL, NULL}
};

zend_class_entry* struct_type;

// Init class entry.
PHP_PROTO_INIT_SUBMSGCLASS_START("Google\\Protobuf\\Struct",
                                 Struct, struct)
  zend_class_implements(struct_type TSRMLS_CC, 1, message_type);
  zend_declare_property_null(struct_type, "fields", strlen("fields"),
                             ZEND_ACC_PRIVATE TSRMLS_CC);
PHP_PROTO_INIT_SUBMSGCLASS_END

PHP_METHOD(Struct, __construct) {
  init_file_struct(TSRMLS_C);
  MessageHeader* intern = UNBOX(MessageHeader, getThis());
  INIT_MESSAGE_WITH_ARRAY;
}

PHP_PROTO_FIELD_ACCESSORS(Struct, struct, Fields, "fields")

// -----------------------------------------------------------------------------
// Type
// -----------------------------------------------------------------------------

static  zend_function_entry type_methods[] = {
  PHP_ME(Type, __construct, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(Type, getName, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(Type, setName, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(Type, getFields, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(Type, setFields, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(Type, getOneofs, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(Type, setOneofs, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(Type, getOptions, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(Type, setOptions, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(Type, getSourceContext, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(Type, setSourceContext, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(Type, getSyntax, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(Type, setSyntax, NULL, ZEND_ACC_PUBLIC)
  {NULL, NULL, NULL}
};

zend_class_entry* type_type;

// Init class entry.
PHP_PROTO_INIT_SUBMSGCLASS_START("Google\\Protobuf\\Type",
                                 Type, type)
  zend_class_implements(type_type TSRMLS_CC, 1, message_type);
  zend_declare_property_null(type_type, "name", strlen("name"),
                             ZEND_ACC_PRIVATE TSRMLS_CC);
  zend_declare_property_null(type_type, "fields", strlen("fields"),
                             ZEND_ACC_PRIVATE TSRMLS_CC);
  zend_declare_property_null(type_type, "oneofs", strlen("oneofs"),
                             ZEND_ACC_PRIVATE TSRMLS_CC);
  zend_declare_property_null(type_type, "options", strlen("options"),
                             ZEND_ACC_PRIVATE TSRMLS_CC);
  zend_declare_property_null(type_type, "source_context", strlen("source_context"),
                             ZEND_ACC_PRIVATE TSRMLS_CC);
  zend_declare_property_null(type_type, "syntax", strlen("syntax"),
                             ZEND_ACC_PRIVATE TSRMLS_CC);
PHP_PROTO_INIT_SUBMSGCLASS_END

PHP_METHOD(Type, __construct) {
  init_file_type(TSRMLS_C);
  MessageHeader* intern = UNBOX(MessageHeader, getThis());
  INIT_MESSAGE_WITH_ARRAY;
}

PHP_PROTO_FIELD_ACCESSORS(Type, type, Name, "name")
PHP_PROTO_FIELD_ACCESSORS(Type, type, Fields, "fields")
PHP_PROTO_FIELD_ACCESSORS(Type, type, Oneofs, "oneofs")
PHP_PROTO_FIELD_ACCESSORS(Type, type, Options, "options")
PHP_PROTO_FIELD_ACCESSORS(Type, type, SourceContext, "source_context")
PHP_PROTO_FIELD_ACCESSORS(Type, type, Syntax, "syntax")

// -----------------------------------------------------------------------------
// UInt32Value
// -----------------------------------------------------------------------------

static  zend_function_entry u_int32_value_methods[] = {
  PHP_ME(UInt32Value, __construct, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(UInt32Value, getValue, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(UInt32Value, setValue, NULL, ZEND_ACC_PUBLIC)
  {NULL, NULL, NULL}
};

zend_class_entry* u_int32_value_type;

// Init class entry.
PHP_PROTO_INIT_SUBMSGCLASS_START("Google\\Protobuf\\UInt32Value",
                                 UInt32Value, u_int32_value)
  zend_class_implements(u_int32_value_type TSRMLS_CC, 1, message_type);
  zend_declare_property_null(u_int32_value_type, "value", strlen("value"),
                             ZEND_ACC_PRIVATE TSRMLS_CC);
PHP_PROTO_INIT_SUBMSGCLASS_END

PHP_METHOD(UInt32Value, __construct) {
  init_file_wrappers(TSRMLS_C);
  MessageHeader* intern = UNBOX(MessageHeader, getThis());
  INIT_MESSAGE_WITH_ARRAY;
}

PHP_PROTO_FIELD_ACCESSORS(UInt32Value, u_int32_value, Value, "value")

// -----------------------------------------------------------------------------
// UInt64Value
// -----------------------------------------------------------------------------

static  zend_function_entry u_int64_value_methods[] = {
  PHP_ME(UInt64Value, __construct, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(UInt64Value, getValue, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(UInt64Value, setValue, NULL, ZEND_ACC_PUBLIC)
  {NULL, NULL, NULL}
};

zend_class_entry* u_int64_value_type;

// Init class entry.
PHP_PROTO_INIT_SUBMSGCLASS_START("Google\\Protobuf\\UInt64Value",
                                 UInt64Value, u_int64_value)
  zend_class_implements(u_int64_value_type TSRMLS_CC, 1, message_type);
  zend_declare_property_null(u_int64_value_type, "value", strlen("value"),
                             ZEND_ACC_PRIVATE TSRMLS_CC);
PHP_PROTO_INIT_SUBMSGCLASS_END

PHP_METHOD(UInt64Value, __construct) {
  init_file_wrappers(TSRMLS_C);
  MessageHeader* intern = UNBOX(MessageHeader, getThis());
  INIT_MESSAGE_WITH_ARRAY;
}

PHP_PROTO_FIELD_ACCESSORS(UInt64Value, u_int64_value, Value, "value")

// -----------------------------------------------------------------------------
// Value
// -----------------------------------------------------------------------------

static zend_function_entry value_methods[] = {
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

zend_class_entry* value_type;

// Init class entry.
PHP_PROTO_INIT_SUBMSGCLASS_START("Google\\Protobuf\\Value",
                                 Value, value)
  zend_class_implements(value_type TSRMLS_CC, 1, message_type);
  zend_declare_property_null(value_type, "kind", strlen("kind"),
                             ZEND_ACC_PRIVATE TSRMLS_CC);
PHP_PROTO_INIT_SUBMSGCLASS_END

PHP_METHOD(Value, __construct) {
  init_file_struct(TSRMLS_C);
  MessageHeader* intern = UNBOX(MessageHeader, getThis());
  INIT_MESSAGE_WITH_ARRAY;
}

PHP_PROTO_ONEOF_FIELD_ACCESSORS(Value, value, NullValue, "null_value")
PHP_PROTO_ONEOF_FIELD_ACCESSORS(Value, value, NumberValue, "number_value")
PHP_PROTO_ONEOF_FIELD_ACCESSORS(Value, value, StringValue, "string_value")
PHP_PROTO_ONEOF_FIELD_ACCESSORS(Value, value, BoolValue, "bool_value")
PHP_PROTO_ONEOF_FIELD_ACCESSORS(Value, value, StructValue, "struct_value")
PHP_PROTO_ONEOF_FIELD_ACCESSORS(Value, value, ListValue, "list_value")
PHP_PROTO_ONEOF_ACCESSORS(Value, value, Kind, "kind")

// -----------------------------------------------------------------------------
// GPBMetadata files for well known types
// -----------------------------------------------------------------------------

#define DEFINE_GPBMETADATA_FILE(LOWERNAME, CAMELNAME, CLASSNAME)      \
  zend_class_entry* gpb_metadata_##LOWERNAME##_type;                  \
  static zend_function_entry gpb_metadata_##LOWERNAME##_methods[] = { \
    PHP_ME(GPBMetadata_##CAMELNAME, initOnce, NULL,                   \
           ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)                         \
    ZEND_FE_END                                                       \
  };                                                                  \
  void gpb_metadata_##LOWERNAME##_init(TSRMLS_D) {                    \
    zend_class_entry class_type;                                      \
    INIT_CLASS_ENTRY(class_type, CLASSNAME,                           \
                     gpb_metadata_##LOWERNAME##_methods);             \
    gpb_metadata_##LOWERNAME##_type =                                 \
        zend_register_internal_class(&class_type TSRMLS_CC);          \
  }                                                                   \
  PHP_METHOD(GPBMetadata_##CAMELNAME, initOnce) {                     \
    init_file_##LOWERNAME(TSRMLS_C);                                  \
  }

DEFINE_GPBMETADATA_FILE(any, Any, "GPBMetadata\\Google\\Protobuf\\Any");
DEFINE_GPBMETADATA_FILE(api, Api, "GPBMetadata\\Google\\Protobuf\\Api");
DEFINE_GPBMETADATA_FILE(duration, Duration,
                        "GPBMetadata\\Google\\Protobuf\\Duration");
DEFINE_GPBMETADATA_FILE(field_mask, FieldMask,
                        "GPBMetadata\\Google\\Protobuf\\FieldMask");
DEFINE_GPBMETADATA_FILE(empty, Empty,
                        "GPBMetadata\\Google\\Protobuf\\GPBEmpty");
DEFINE_GPBMETADATA_FILE(source_context, SourceContext,
                        "GPBMetadata\\Google\\Protobuf\\SourceContext");
DEFINE_GPBMETADATA_FILE(struct, Struct,
                        "GPBMetadata\\Google\\Protobuf\\Struct");
DEFINE_GPBMETADATA_FILE(timestamp, Timestamp,
                        "GPBMetadata\\Google\\Protobuf\\Timestamp");
DEFINE_GPBMETADATA_FILE(type, Type, "GPBMetadata\\Google\\Protobuf\\Type");
DEFINE_GPBMETADATA_FILE(wrappers, Wrappers,
                        "GPBMetadata\\Google\\Protobuf\\Wrappers");

#undef DEFINE_GPBMETADATA_FILE
