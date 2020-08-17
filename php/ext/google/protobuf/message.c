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

#include "message.h"

#include <inttypes.h>
#include <php.h>
#include <stdlib.h>

// This is not self-contained: it must be after other Zend includes.
#include <Zend/zend_exceptions.h>

#include "arena.h"
#include "array.h"
#include "convert.h"
#include "def.h"
#include "map.h"
#include "php-upb.h"
#include "protobuf.h"

// -----------------------------------------------------------------------------
// Message
// -----------------------------------------------------------------------------

typedef struct {
  zend_object std;
  zval arena;
  const Descriptor* desc;
  upb_msg *msg;
} Message;

zend_class_entry *message_ce;
static zend_object_handlers message_object_handlers;

// PHP Object Handlers /////////////////////////////////////////////////////////

/**
 * Message_create()
 *
 * PHP class entry function to allocate and initialize a new Message object.
 */
static zend_object* Message_create(zend_class_entry *class_type) {
  Message *intern = emalloc(sizeof(Message));
  // XXX(haberman): verify whether we actually want to take this route.
  class_type->default_properties_count = 0;
  zend_object_std_init(&intern->std, class_type);
  intern->std.handlers = &message_object_handlers;
  Arena_Init(&intern->arena);
  return &intern->std;
}

/**
 * Message_dtor()
 *
 * Object handler to destroy a Message. This releases all resources associated
 * with the message. Note that it is possible to access a destroyed object from
 * PHP in rare cases.
 */
static void Message_dtor(zend_object* obj) {
  Message* intern = (Message*)obj;
  ObjCache_Delete(intern->msg);
  zval_dtor(&intern->arena);
  zend_object_std_dtor(&intern->std);
}

/**
 * get_field()
 *
 * Helper function to look up a field given a member name (as a string).
 */
static const upb_fielddef *get_field(Message *msg, PROTO_STR *member) {
  const upb_msgdef *m = msg->desc->msgdef;
  const upb_fielddef *f =
      upb_msgdef_ntof(m, PROTO_STRVAL_P(member), PROTO_STRLEN_P(member));

  if (!f) {
    zend_throw_exception_ex(NULL, 0, "No such property %s.",
                            ZSTR_VAL(msg->desc->class_entry->name));
  }

  return f;
}

/**
 * Message_has_property()
 *
 * Object handler for testing whether a property exists. Called when PHP code
 * does any of:
 *
 *   isset($message->foobar);
 *   property_exists($message->foobar);
 *
 * Note that all properties of generated messages are private, so this should
 * only be possible to invoke from generated code, which has accessors like this
 * (if the field has presence):
 *
 *   public function hasOptionalInt32()
 *   {
 *       return isset($this->optional_int32);
 *   }
 */
static int Message_has_property(PROTO_VAL *obj, PROTO_STR *member,
                                int has_set_exists,
                                void **cache_slot) {
  Message* intern = PROTO_MSG_P(obj);
  const upb_fielddef *f = get_field(intern, member);

  if (!f) return 0;

  if (!upb_fielddef_haspresence(f)) {
    zend_throw_exception_ex(
        NULL, 0,
        "Cannot call isset() on field %s which does not have presence.",
        ZSTR_VAL(intern->desc->class_entry->name));
    return 0;
  }

  return upb_msg_has(intern->msg, f);
}

/**
 * Message_unset_property()
 *
 * Object handler for unsetting a property. Called when PHP code calls:
 * does any of:
 *
 *   unset($message->foobar);
 *
 * Note that all properties of generated messages are private, so this should
 * only be possible to invoke from generated code, which has accessors like this
 * (if the field has presence):
 *
 *   public function clearOptionalInt32()
 *   {
 *       unset($this->optional_int32);
 *   }
 */
static void Message_unset_property(PROTO_VAL *obj, PROTO_STR *member,
                                   void **cache_slot) {
  Message* intern = PROTO_MSG_P(obj);
  const upb_fielddef *f = get_field(intern, member);

  if (!f) return;

  if (!upb_fielddef_haspresence(f)) {
    zend_throw_exception_ex(
        NULL, 0,
        "Cannot call unset() on field %s which does not have presence.",
        ZSTR_VAL(intern->desc->class_entry->name));
    return;
  }

  upb_msg_clearfield(intern->msg, f);
}

/**
 * Message_read_property()
 *
 * Object handler for reading a property in PHP. Called when PHP code does:
 *
 *   $x = $message->foobar;
 *
 * Note that all properties of generated messages are private, so this should
 * only be possible to invoke from generated code, which has accessors like:
 *
 *   public function getOptionalInt32()
 *   {
 *       return $this->optional_int32;
 *   }
 *
 * We lookup the field and return the scalar, RepeatedField, or MapField for
 * this field.
 */
static zval *Message_read_property(PROTO_VAL *obj, PROTO_STR *member,
                                   int type, void **cache_slot, zval *rv) {
  Message* intern = PROTO_MSG_P(obj);
  const upb_fielddef *f = get_field(intern, member);
  upb_arena *arena = Arena_Get(&intern->arena);

  if (!f) return NULL;

  if (upb_fielddef_ismap(f)) {
    upb_mutmsgval msgval = upb_msg_mutable(intern->msg, f, arena);
    MapField_GetPhpWrapper(rv, msgval.map, f, &intern->arena);
  } else if (upb_fielddef_isseq(f)) {
    upb_mutmsgval msgval = upb_msg_mutable(intern->msg, f, arena);
    RepeatedField_GetPhpWrapper(rv, msgval.array, f, &intern->arena);
  } else {
    upb_msgval msgval = upb_msg_get(intern->msg, f);
    const Descriptor *subdesc = Descriptor_GetFromFieldDef(f);
    Convert_UpbToPhp(msgval, rv, upb_fielddef_type(f), subdesc, &intern->arena);
  }

  return rv;
}

/**
 * Message_write_property()
 *
 * Object handler for writing a property in PHP. Called when PHP code does:
 *
 *   $message->foobar = $x;
 *
 * Note that all properties of generated messages are private, so this should
 * only be possible to invoke from generated code, which has accessors like:
 *
 *   public function setOptionalInt32($var)
 *   {
 *       GPBUtil::checkInt32($var);
 *       $this->optional_int32 = $var;
 *
 *       return $this;
 *   }
 *
 * The C extension version of checkInt32() doesn't actually check anything, so
 * we perform all checking and conversion in this function.
 */
static PROTO_RETURN_VAL Message_write_property(
    PROTO_VAL *obj, PROTO_STR *member, zval *val, void **cache_slot) {
  Message* intern = PROTO_MSG_P(obj);
  const upb_fielddef *f = get_field(intern, member);
  upb_arena *arena = Arena_Get(&intern->arena);
  upb_msgval msgval;

  if (!f) goto error;

  if (upb_fielddef_ismap(f)) {
    msgval.map_val = MapField_GetUpbMap(val, f, arena);
    if (!msgval.map_val) goto error;
  } else if (upb_fielddef_isseq(f)) {
    msgval.array_val = RepeatedField_GetUpbArray(val, f, arena);
    if (!msgval.array_val) goto error;
  } else {
    upb_fieldtype_t type = upb_fielddef_type(f);
    const Descriptor *subdesc = Descriptor_GetFromFieldDef(f);
    bool ok = Convert_PhpToUpb(val, &msgval, type, subdesc, arena);
    if (!ok) goto error;
  }

  upb_msg_set(intern->msg, f, msgval, arena);
#if PHP_VERSION_ID < 70400
  return;
#else
  return val;
#endif

error:
#if PHP_VERSION_ID < 70400
  return;
#else
  return &EG(error_zval);
#endif
}

/**
 * Message_get_property_ptr_ptr()
 *
 * Object handler for the get_property_ptr_ptr event in PHP. This returns a
 * reference to our internal properties. We don't support this, so we return
 * NULL.
 */
static zval *Message_get_property_ptr_ptr(PROTO_VAL *object, PROTO_STR *member,
                                          int type,
                                          void **cache_slot) {
  return NULL;  // We do not have a properties table.
}

/**
 * Message_get_properties()
 *
 * Object handler for the get_properties event in PHP. This returns a HashTable
 * of our internal properties. We don't support this, so we return NULL.
 */
static HashTable *Message_get_properties(PROTO_VAL *object) {
  return NULL;  // We don't offer direct references to our properties.
}

// C Functions from message.h. /////////////////////////////////////////////////

// These are documented in the header file.

void Message_GetPhpWrapper(zval *val, const Descriptor *desc, upb_msg *msg,
                           zval *arena) {
  if (!msg) {
    ZVAL_NULL(val);
    return;
  }

  if (!ObjCache_Get(msg, val)) {
    Message *intern = emalloc(sizeof(Message));
    // XXX(haberman): verify whether we actually want to take this route.
    desc->class_entry->default_properties_count = 0;
    zend_object_std_init(&intern->std, desc->class_entry);
    intern->std.handlers = &message_object_handlers;
    ZVAL_COPY(&intern->arena, arena);
    intern->desc = desc;
    intern->msg = msg;
    ZVAL_OBJ(val, &intern->std);
    ObjCache_Add(intern->msg, &intern->std);
  }
}

bool Message_GetUpbMessage(zval *val, const Descriptor *desc, upb_arena *arena,
                           upb_msg **msg) {
  PBPHP_ASSERT(desc);

  if (Z_ISREF_P(val)) {
    ZVAL_DEREF(val);
  }

  if (Z_TYPE_P(val) == IS_NULL) {
    *msg = NULL;
    return true;
  }

  if (Z_TYPE_P(val) == IS_OBJECT &&
      instanceof_function(Z_OBJCE_P(val), desc->class_entry)) {
    Message *intern = (Message*)Z_OBJ_P(val);
    upb_arena_fuse(arena, Arena_Get(&intern->arena));
    *msg = intern->msg;
    return true;
  } else {
    zend_throw_exception_ex(NULL, 0, "Given value is not an instance of %s.",
                            ZSTR_VAL(desc->class_entry->name));
    return false;
  }
}

// Message PHP methods /////////////////////////////////////////////////////////

/**
 * Message_InitFromPhp()
 *
 * Helper method to handle the initialization of a message from a PHP value, eg.
 *
 *   $m = new TestMessage([
 *       'optional_int32' => -42,
 *       'optional_bool' => true,
 *       'optional_string' => 'a',
 *       'optional_enum' => TestEnum::ONE,
 *       'optional_message' => new Sub([
 *           'a' => 33
 *       ]),
 *       'repeated_int32' => [-42, -52],
 *       'repeated_enum' => [TestEnum::ZERO, TestEnum::ONE],
 *       'repeated_message' => [new Sub(['a' => 34]),
 *                              new Sub(['a' => 35])],
 *       'map_int32_int32' => [-62 => -62],
 *       'map_int32_enum' => [1 => TestEnum::ONE],
 *       'map_int32_message' => [1 => new Sub(['a' => 36])],
 *   ]);
 *
 * The initializer must be an array.
 */
bool Message_InitFromPhp(upb_msg *msg, const upb_msgdef *m, zval *init,
                         upb_arena *arena) {
  HashTable* table = HASH_OF(init);
  HashPosition pos;

  if (Z_ISREF_P(init)) {
    ZVAL_DEREF(init);
  }

  if (Z_TYPE_P(init) != IS_ARRAY) {
    zend_throw_exception_ex(NULL, 0,
                            "Initializer for a message %s must be an array.",
                            upb_msgdef_fullname(m));
    return false;
  }

  zend_hash_internal_pointer_reset_ex(table, &pos);

  while (true) {  // Iterate over key/value pairs.
    zval key;
    zval *val;
    const upb_fielddef *f;
    upb_msgval msgval;

    zend_hash_get_current_key_zval_ex(table, &key, &pos);
    val = zend_hash_get_current_data_ex(table, &pos);

    if (!val) return true;  // Finished iteration.

    if (Z_ISREF_P(val)) {
      ZVAL_DEREF(val);
    }

    f = upb_msgdef_ntof(m, Z_STRVAL_P(&key), Z_STRLEN_P(&key));

    if (!f) {
      zend_throw_exception_ex(NULL, 0,
                              "No such field %s", Z_STRVAL_P(&key));
      return false;
    }

    if (upb_fielddef_ismap(f)) {
      msgval.map_val = MapField_GetUpbMap(val, f, arena);
      if (!msgval.map_val) return false;
    } else if (upb_fielddef_isseq(f)) {
      msgval.array_val = RepeatedField_GetUpbArray(val, f, arena);
      if (!msgval.array_val) return false;
    } else {
      const Descriptor *desc = Descriptor_GetFromFieldDef(f);
      upb_fieldtype_t type = upb_fielddef_type(f);
      if (!Convert_PhpToUpbAutoWrap(val, &msgval, type, desc, arena)) {
        return false;
      }
    }

    upb_msg_set(msg, f, msgval, arena);
    zend_hash_move_forward_ex(table, &pos);
    zval_dtor(&key);
  }
}

/**
 * Message::__construct()
 *
 * Constructor for Message.
 * @param array Map of initial values ['k' = val]
 */
PHP_METHOD(Message, __construct) {
  Message* intern = (Message*)Z_OBJ_P(getThis());
  const Descriptor* desc = Descriptor_GetFromClassEntry(Z_OBJCE_P(getThis()));
  const upb_msgdef *msgdef = desc->msgdef;
  upb_arena *arena = Arena_Get(&intern->arena);
  zval *init_arr = NULL;

  intern->desc = desc;
  intern->msg = upb_msg_new(msgdef, arena);
  ObjCache_Add(intern->msg, &intern->std);

  if (zend_parse_parameters(ZEND_NUM_ARGS(), "|a!", &init_arr) == FAILURE) {
    return;
  }

  if (init_arr) {
    Message_InitFromPhp(intern->msg, desc->msgdef, init_arr, arena);
  }
}

/**
 * Message::discardUnknownFields()
 *
 * Discards any unknown fields for this message or any submessages.
 */
PHP_METHOD(Message, discardUnknownFields) {
  Message* intern = (Message*)Z_OBJ_P(getThis());
  upb_msg_discardunknown(intern->msg, intern->desc->msgdef, 64);
}

/**
 * Message::clear()
 *
 * Clears all fields of this message.
 */
PHP_METHOD(Message, clear) {
  Message* intern = (Message*)Z_OBJ_P(getThis());
  upb_msg_clear(intern->msg, intern->desc->msgdef);
}

/**
 * Message::mergeFrom()
 *
 * Merges from the given message, which must be of the same class as us.
 * @param object Message to merge from.
 */
PHP_METHOD(Message, mergeFrom) {
  Message* intern = (Message*)Z_OBJ_P(getThis());
  Message* from;
  upb_arena *arena = Arena_Get(&intern->arena);
  const upb_msglayout *l = upb_msgdef_layout(intern->desc->msgdef);
  zval* value;
  char *pb;
  size_t size;
  bool ok;

  if (zend_parse_parameters(ZEND_NUM_ARGS(), "O", &value,
                            intern->desc->class_entry) == FAILURE) {
    return;
  }

  from = (Message*)Z_OBJ_P(value);

  // Should be guaranteed since we passed the class type to
  // zend_parse_parameters().
  PBPHP_ASSERT(from->desc == intern->desc);

  // TODO(haberman): use a temp arena for this once we can make upb_decode()
  // copy strings.
  pb = upb_encode(from->msg, l, arena, &size);

  if (!pb) {
    zend_throw_exception_ex(NULL, 0, "Max nesting exceeded");
    return;
  }

  ok = upb_decode(pb, size, intern->msg, l, arena);
  PBPHP_ASSERT(ok);
}

/**
 * Message::mergeFromString()
 *
 * Merges from the given string.
 * @param string Binary protobuf data to merge.
 */
PHP_METHOD(Message, mergeFromString) {
  Message* intern = (Message*)Z_OBJ_P(getThis());
  char *data = NULL;
  char *data_copy = NULL;
  zend_long data_len;
  const upb_msglayout *l = upb_msgdef_layout(intern->desc->msgdef);
  upb_arena *arena = Arena_Get(&intern->arena);

  if (zend_parse_parameters(ZEND_NUM_ARGS(), "s", &data, &data_len) ==
      FAILURE) {
    return;
  }

  // TODO(haberman): avoid this copy when we can make the decoder copy.
  data_copy = upb_arena_malloc(arena, data_len);
  memcpy(data_copy, data, data_len);

  if (!upb_decode(data_copy, data_len, intern->msg, l, arena)) {
    zend_throw_exception_ex(NULL, 0, "Error occurred during parsing");
    return;
  }
}

/**
 * Message::serializeToString()
 *
 * Serializes this message instance to protobuf data.
 * @return string Serialized protobuf data.
 */
PHP_METHOD(Message, serializeToString) {
  Message* intern = (Message*)Z_OBJ_P(getThis());
  const upb_msglayout *l = upb_msgdef_layout(intern->desc->msgdef);
  upb_arena *tmp_arena = upb_arena_new();
  char *data;
  size_t size;

  data = upb_encode(intern->msg, l, tmp_arena, &size);

  if (!data) {
    zend_throw_exception_ex(NULL, 0, "Error occurred during serialization");
    upb_arena_free(tmp_arena);
    return;
  }

  RETVAL_STRINGL(data, size);
  upb_arena_free(tmp_arena);
}

/**
 * Message::mergeFromJsonString()
 *
 * Merges the JSON data parsed from the given string.
 * @param string Serialized JSON data.
 */
PHP_METHOD(Message, mergeFromJsonString) {
  Message* intern = (Message*)Z_OBJ_P(getThis());
  char *data = NULL;
  char *data_copy = NULL;
  zend_long data_len;
  upb_arena *arena = Arena_Get(&intern->arena);
  upb_status status;
  zend_bool ignore_json_unknown = false;
  int options = 0;

  if (zend_parse_parameters(ZEND_NUM_ARGS(), "s|b", &data, &data_len,
                            &ignore_json_unknown) == FAILURE) {
    return;
  }

  // TODO(haberman): avoid this copy when we can make the decoder copy.
  data_copy = upb_arena_malloc(arena, data_len + 1);
  memcpy(data_copy, data, data_len);
  data_copy[data_len] = '\0';

  if (ignore_json_unknown) {
    options |= UPB_JSONDEC_IGNOREUNKNOWN;
  }

  upb_status_clear(&status);
  if (!upb_json_decode(data_copy, data_len, intern->msg, intern->desc->msgdef,
                       DescriptorPool_GetSymbolTable(), options, arena,
                       &status)) {
    zend_throw_exception_ex(NULL, 0, "Error occurred during parsing: %s",
                            upb_status_errmsg(&status));
    return;
  }
}

/**
 * Message::serializeToJsonString()
 *
 * Serializes this object to JSON.
 * @return string Serialized JSON data.
 */
PHP_METHOD(Message, serializeToJsonString) {
  Message* intern = (Message*)Z_OBJ_P(getThis());
  size_t size;
  int options = 0;
  char buf[1024];
  zend_bool preserve_proto_fieldnames = false;
  upb_status status;

  if (zend_parse_parameters(ZEND_NUM_ARGS(), "|b",
                            &preserve_proto_fieldnames) == FAILURE) {
    return;
  }

  if (preserve_proto_fieldnames) {
    options |= UPB_JSONENC_PROTONAMES;
  }

  upb_status_clear(&status);
  size = upb_json_encode(intern->msg, intern->desc->msgdef,
                         DescriptorPool_GetSymbolTable(), options, buf,
                         sizeof(buf), &status);

  if (!upb_ok(&status)) {
    zend_throw_exception_ex(NULL, 0,
                            "Error occurred during JSON serialization: %s",
                            upb_status_errmsg(&status));
    return;
  }

  if (size >= sizeof(buf)) {
    char *buf2 = malloc(size + 1);
    upb_json_encode(intern->msg, intern->desc->msgdef,
                    DescriptorPool_GetSymbolTable(), options, buf2, size + 1,
                    &status);
    RETVAL_STRINGL(buf2, size);
    free(buf2);
  } else {
    RETVAL_STRINGL(buf, size);
  }
}

/**
 * Message::readWrapperValue()
 *
 * Returns an unboxed value for the given field. This is called from generated
 * methods for wrapper fields, eg.
 *
 *   public function getDoubleValueUnwrapped()
 *   {
 *       return $this->readWrapperValue("double_value");
 *   }
 *
 * @return Unwrapped field value or null.
 */
PHP_METHOD(Message, readWrapperValue) {
  Message* intern = (Message*)Z_OBJ_P(getThis());
  char* member;
  const upb_fielddef *f;
  zend_long size;

  if (zend_parse_parameters(ZEND_NUM_ARGS(), "s", &member, &size) == FAILURE) {
    return;
  }

  f = upb_msgdef_ntof(intern->desc->msgdef, member, size);

  if (!f || !upb_msgdef_iswrapper(upb_fielddef_msgsubdef(f))) {
    zend_throw_exception_ex(NULL, 0, "Message %s has no field %s",
                            upb_msgdef_fullname(intern->desc->msgdef), member);
    return;
  }

  if (upb_msg_has(intern->msg, f)) {
    const upb_msg *wrapper = upb_msg_get(intern->msg, f).msg_val;
    const upb_msgdef *m = upb_fielddef_msgsubdef(f);
    const upb_fielddef *val_f = upb_msgdef_itof(m, 1);
    const upb_fieldtype_t val_type = upb_fielddef_type(val_f);
    upb_msgval msgval = upb_msg_get(wrapper, val_f);
    zval ret;
    Convert_UpbToPhp(msgval, &ret, val_type, NULL, &intern->arena);
    RETURN_ZVAL(&ret, 1, 0);
  } else {
    RETURN_NULL();
  }
}

/**
 * Message::writeWrapperValue()
 *
 * Sets the given wrapper field to the given unboxed value. This is called from
 * generated methods for wrapper fields, eg.
 *
 *
 *   public function setDoubleValueUnwrapped($var)
 *   {
 *       $this->writeWrapperValue("double_value", $var);
 *       return $this;
 *   }
 *
 * @param Unwrapped field value or null.
 */
PHP_METHOD(Message, writeWrapperValue) {
  Message* intern = (Message*)Z_OBJ_P(getThis());
  upb_arena *arena = Arena_Get(&intern->arena);
  char* member;
  const upb_fielddef *f;
  upb_msgval msgval;
  zend_long size;
  zval* val;

  if (zend_parse_parameters(ZEND_NUM_ARGS(), "sz", &member, &size, &val) ==
      FAILURE) {
    return;
  }

  f = upb_msgdef_ntof(intern->desc->msgdef, member, size);

  if (!f || !upb_msgdef_iswrapper(upb_fielddef_msgsubdef(f))) {
    zend_throw_exception_ex(NULL, 0, "Message %s has no field %s",
                            upb_msgdef_fullname(intern->desc->msgdef), member);
    return;
  }

  if (Z_ISREF_P(val)) {
    ZVAL_DEREF(val);
  }

  if (Z_TYPE_P(val) == IS_NULL) {
    upb_msg_clearfield(intern->msg, f);
  } else {
    const upb_msgdef *m = upb_fielddef_msgsubdef(f);
    const upb_fielddef *val_f = upb_msgdef_itof(m, 1);
    upb_fieldtype_t val_type = upb_fielddef_type(val_f);
    upb_msg *wrapper;

    if (!Convert_PhpToUpb(val, &msgval, val_type, NULL, arena)) {
      return;  // Error is already set.
    }

    wrapper = upb_msg_mutable(intern->msg, f, arena).msg;
    upb_msg_set(wrapper, val_f, msgval, arena);
  }
}

/**
 * Message::whichOneof()
 *
 * Given a oneof name, returns the name of the field that is set for this oneof,
 * or otherwise the empty string.
 *
 * @return string The field name in this oneof that is currently set.
 */
PHP_METHOD(Message, whichOneof) {
  Message* intern = (Message*)Z_OBJ_P(getThis());
  const upb_oneofdef* oneof;
  const upb_fielddef* field;
  char* name;
  zend_long len;

  if (zend_parse_parameters(ZEND_NUM_ARGS(), "s", &name, &len) == FAILURE) {
    return;
  }

  oneof = upb_msgdef_ntoo(intern->desc->msgdef, name, len);

  if (!oneof) {
    zend_throw_exception_ex(NULL, 0, "Message %s has no oneof %s",
                            upb_msgdef_fullname(intern->desc->msgdef), name);
    return;
  }

  field = upb_msg_whichoneof(intern->msg, oneof);
  RETURN_STRING(field ? upb_fielddef_name(field) : "");
}

/**
 * Message::readOneof()
 *
 * Returns the contents of the given oneof field, given a field number. Called
 * from generated code methods such as:
 *
 *    public function getDoubleValueOneof()
 *    {
 *        return $this->readOneof(10);
 *    }
 *
 * @return object The oneof's field value.
 */
PHP_METHOD(Message, readOneof) {
  Message* intern = (Message*)Z_OBJ_P(getThis());
  zend_long field_num;
  const upb_fielddef* f;
  zval ret;

  if (zend_parse_parameters(ZEND_NUM_ARGS(), "l", &field_num) == FAILURE) {
    return;
  }

  f = upb_msgdef_itof(intern->desc->msgdef, field_num);

  if (!f || !upb_fielddef_realcontainingoneof(f)) {
    php_error_docref(NULL, E_USER_ERROR,
                     "Internal error, no such oneof field %d\n",
                     (int)field_num);
  }

  {
    upb_msgval msgval = upb_msg_get(intern->msg, f);
    const Descriptor *subdesc = Descriptor_GetFromFieldDef(f);
    Convert_UpbToPhp(msgval, &ret, upb_fielddef_type(f), subdesc,
                     &intern->arena);
  }

  RETURN_ZVAL(&ret, 1, 0);
}

/**
 * Message::writeOneof()
 *
 * Sets the contents of the given oneof field, given a field number. Called
 * from generated code methods such as:
 *
 *    public function setDoubleValueOneof($var)
 *   {
 *       GPBUtil::checkMessage($var, \Google\Protobuf\DoubleValue::class);
 *       $this->writeOneof(10, $var);
 *
 *       return $this;
 *   }
 *
 * The C extension version of GPBUtil::check*() does nothing, so we perform
 * all type checking and conversion here.
 *
 * @param integer The field number we are setting.
 * @param object The field value we want to set.
 */
PHP_METHOD(Message, writeOneof) {
  Message* intern = (Message*)Z_OBJ_P(getThis());
  zend_long field_num;
  const upb_fielddef* f;
  upb_arena *arena = Arena_Get(&intern->arena);
  upb_msgval msgval;
  zval* val;

  if (zend_parse_parameters(ZEND_NUM_ARGS(), "lz", &field_num, &val) ==
      FAILURE) {
    return;
  }

  f = upb_msgdef_itof(intern->desc->msgdef, field_num);

  if (!Convert_PhpToUpb(val, &msgval, upb_fielddef_type(f),
                        Descriptor_GetFromFieldDef(f), arena)) {
    return;
  }

  upb_msg_set(intern->msg, f, msgval, arena);
}

ZEND_BEGIN_ARG_INFO_EX(arginfo_void, 0, 0, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_mergeFrom, 0, 0, 1)
  ZEND_ARG_INFO(0, data)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_read, 0, 0, 1)
  ZEND_ARG_INFO(0, field)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_write, 0, 0, 2)
  ZEND_ARG_INFO(0, field)
  ZEND_ARG_INFO(0, value)
ZEND_END_ARG_INFO()

static zend_function_entry Message_methods[] = {
  PHP_ME(Message, clear,                 arginfo_void,      ZEND_ACC_PUBLIC)
  PHP_ME(Message, discardUnknownFields,  arginfo_void,      ZEND_ACC_PUBLIC)
  PHP_ME(Message, serializeToString,     arginfo_void,      ZEND_ACC_PUBLIC)
  PHP_ME(Message, mergeFromString,       arginfo_mergeFrom, ZEND_ACC_PUBLIC)
  PHP_ME(Message, serializeToJsonString, arginfo_void,      ZEND_ACC_PUBLIC)
  PHP_ME(Message, mergeFromJsonString,   arginfo_mergeFrom, ZEND_ACC_PUBLIC)
  PHP_ME(Message, mergeFrom,             arginfo_mergeFrom, ZEND_ACC_PUBLIC)
  PHP_ME(Message, readWrapperValue,      arginfo_read,      ZEND_ACC_PROTECTED)
  PHP_ME(Message, writeWrapperValue,     arginfo_write,     ZEND_ACC_PROTECTED)
  PHP_ME(Message, readOneof,             arginfo_read,      ZEND_ACC_PROTECTED)
  PHP_ME(Message, writeOneof,            arginfo_write,     ZEND_ACC_PROTECTED)
  PHP_ME(Message, whichOneof,            arginfo_read,      ZEND_ACC_PROTECTED)
  PHP_ME(Message, __construct,           arginfo_void,      ZEND_ACC_PROTECTED)
  ZEND_FE_END
};

/**
 * Message_ModuleInit()
 *
 * Called when the C extension is loaded to register all types.
 */
void Message_ModuleInit() {
  zend_class_entry tmp_ce;
  zend_object_handlers *h = &message_object_handlers;

  INIT_CLASS_ENTRY(tmp_ce, "Google\\Protobuf\\Internal\\Message",
                   Message_methods);

  message_ce = zend_register_internal_class(&tmp_ce);
  message_ce->create_object = Message_create;

  memcpy(h, &std_object_handlers, sizeof(zend_object_handlers));
  h->dtor_obj = Message_dtor;
  h->read_property = Message_read_property;
  h->write_property = Message_write_property;
  h->has_property = Message_has_property;
  h->unset_property = Message_unset_property;
  h->get_properties = Message_get_properties;
  h->get_property_ptr_ptr = Message_get_property_ptr_ptr;
}
