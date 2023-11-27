// Protocol Buffers - Google's data interchange format
// Copyright 2014 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "message.h"

#include <inttypes.h>
#include <php.h>
#include <stdlib.h>

// This is not self-contained: it must be after other Zend includes.
#include <Zend/zend_exceptions.h>
#include <Zend/zend_inheritance.h>

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
  upb_Message* msg;
} Message;

zend_class_entry* message_ce;
static zend_object_handlers message_object_handlers;

static void Message_SuppressDefaultProperties(zend_class_entry* class_type) {
  // We suppress all default properties, because all our properties are handled
  // by our read_property handler.
  //
  // This also allows us to put our zend_object member at the beginning of our
  // struct -- instead of putting it at the end with pointer fixups to access
  // our own data, as recommended in the docs -- because Zend won't add any of
  // its own storage directly after the zend_object if default_properties_count
  // == 0.
  //
  // This is not officially supported, but since it simplifies the code, we'll
  // do it for as long as it works in practice.
  class_type->default_properties_count = 0;
}

// PHP Object Handlers /////////////////////////////////////////////////////////

/**
 * Message_create()
 *
 * PHP class entry function to allocate and initialize a new Message object.
 */
static zend_object* Message_create(zend_class_entry* class_type) {
  Message* intern = emalloc(sizeof(Message));
  Message_SuppressDefaultProperties(class_type);
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
static const upb_FieldDef* get_field(Message* msg, zend_string* member) {
  const upb_MessageDef* m = msg->desc->msgdef;
  const upb_FieldDef* f = upb_MessageDef_FindFieldByNameWithSize(
      m, ZSTR_VAL(member), ZSTR_LEN(member));

  if (!f) {
    zend_throw_exception_ex(NULL, 0, "No such property %s.",
                            ZSTR_VAL(msg->desc->class_entry->name));
  }

  return f;
}

// Check if the field is a well known wrapper type
static bool IsWrapper(const upb_MessageDef* m) {
  if (!m) return false;
  switch (upb_MessageDef_WellKnownType(m)) {
    case kUpb_WellKnown_DoubleValue:
    case kUpb_WellKnown_FloatValue:
    case kUpb_WellKnown_Int64Value:
    case kUpb_WellKnown_UInt64Value:
    case kUpb_WellKnown_Int32Value:
    case kUpb_WellKnown_UInt32Value:
    case kUpb_WellKnown_StringValue:
    case kUpb_WellKnown_BytesValue:
    case kUpb_WellKnown_BoolValue:
      return true;
    default:
      return false;
  }
}

static void Message_get(Message* intern, const upb_FieldDef* f, zval* rv) {
  upb_Arena* arena = Arena_Get(&intern->arena);

  if (upb_FieldDef_IsMap(f)) {
    upb_MutableMessageValue msgval = upb_Message_Mutable(intern->msg, f, arena);
    MapField_GetPhpWrapper(rv, msgval.map, MapType_Get(f), &intern->arena);
  } else if (upb_FieldDef_IsRepeated(f)) {
    upb_MutableMessageValue msgval = upb_Message_Mutable(intern->msg, f, arena);
    RepeatedField_GetPhpWrapper(rv, msgval.array, TypeInfo_Get(f),
                                &intern->arena);
  } else {
    if (upb_FieldDef_IsSubMessage(f) &&
        !upb_Message_HasFieldByDef(intern->msg, f)) {
      ZVAL_NULL(rv);
      return;
    }
    upb_MessageValue msgval = upb_Message_GetFieldByDef(intern->msg, f);
    Convert_UpbToPhp(msgval, rv, TypeInfo_Get(f), &intern->arena);
  }
}

static bool Message_set(Message* intern, const upb_FieldDef* f, zval* val) {
  upb_Arena* arena = Arena_Get(&intern->arena);
  upb_MessageValue msgval;

  if (upb_FieldDef_IsMap(f)) {
    msgval.map_val = MapField_GetUpbMap(val, MapType_Get(f), arena);
    if (!msgval.map_val) return false;
  } else if (upb_FieldDef_IsRepeated(f)) {
    msgval.array_val = RepeatedField_GetUpbArray(val, TypeInfo_Get(f), arena);
    if (!msgval.array_val) return false;
  } else if (upb_FieldDef_IsSubMessage(f) && Z_TYPE_P(val) == IS_NULL) {
    upb_Message_ClearFieldByDef(intern->msg, f);
    return true;
  } else {
    if (!Convert_PhpToUpb(val, &msgval, TypeInfo_Get(f), arena)) return false;
  }

  upb_Message_SetFieldByDef(intern->msg, f, msgval, arena);
  return true;
}

static bool MessageEq(const upb_Message* m1, const upb_Message* m2,
                      const upb_MessageDef* m);

/**
 * ValueEq()
 */
bool ValueEq(upb_MessageValue val1, upb_MessageValue val2, TypeInfo type) {
  switch (type.type) {
    case kUpb_CType_Bool:
      return val1.bool_val == val2.bool_val;
    case kUpb_CType_Int32:
    case kUpb_CType_UInt32:
    case kUpb_CType_Enum:
      return val1.int32_val == val2.int32_val;
    case kUpb_CType_Int64:
    case kUpb_CType_UInt64:
      return val1.int64_val == val2.int64_val;
    case kUpb_CType_Float:
      return val1.float_val == val2.float_val;
    case kUpb_CType_Double:
      return val1.double_val == val2.double_val;
    case kUpb_CType_String:
    case kUpb_CType_Bytes:
      return val1.str_val.size == val2.str_val.size &&
             memcmp(val1.str_val.data, val2.str_val.data, val1.str_val.size) ==
                 0;
    case kUpb_CType_Message:
      return MessageEq(val1.msg_val, val2.msg_val, type.desc->msgdef);
    default:
      return false;
  }
}

/**
 * MessageEq()
 */
static bool MessageEq(const upb_Message* m1, const upb_Message* m2,
                      const upb_MessageDef* m) {
  int n = upb_MessageDef_FieldCount(m);

  for (int i = 0; i < n; i++) {
    const upb_FieldDef* f = upb_MessageDef_Field(m, i);

    if (upb_FieldDef_HasPresence(f)) {
      if (upb_Message_HasFieldByDef(m1, f) !=
          upb_Message_HasFieldByDef(m2, f)) {
        return false;
      }
      if (!upb_Message_HasFieldByDef(m1, f)) continue;
    }

    upb_MessageValue val1 = upb_Message_GetFieldByDef(m1, f);
    upb_MessageValue val2 = upb_Message_GetFieldByDef(m2, f);

    if (upb_FieldDef_IsMap(f)) {
      if (!MapEq(val1.map_val, val2.map_val, MapType_Get(f))) return false;
    } else if (upb_FieldDef_IsRepeated(f)) {
      if (!ArrayEq(val1.array_val, val2.array_val, TypeInfo_Get(f)))
        return false;
    } else {
      if (!ValueEq(val1, val2, TypeInfo_Get(f))) return false;
    }
  }

  return true;
}

/**
 * Message_compare_objects()
 *
 * Object handler for comparing two message objects. Called whenever PHP code
 * does:
 *
 *   $m1 == $m2
 */
static int Message_compare_objects(zval* m1, zval* m2) {
  Message* intern1 = (Message*)Z_OBJ_P(m1);
  Message* intern2 = (Message*)Z_OBJ_P(m2);
  const upb_MessageDef* m = intern1->desc->msgdef;

  if (intern2->desc->msgdef != m) return 1;

  return MessageEq(intern1->msg, intern2->msg, m) ? 0 : 1;
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
static int Message_has_property(zend_object* obj, zend_string* member,
                                int has_set_exists, void** cache_slot) {
  Message* intern = (Message*)obj;
  const upb_FieldDef* f = get_field(intern, member);

  if (!f) return 0;

  if (!upb_FieldDef_HasPresence(f)) {
    zend_throw_exception_ex(
        NULL, 0,
        "Cannot call isset() on field %s which does not have presence.",
        upb_FieldDef_Name(f));
    return 0;
  }

  return upb_Message_HasFieldByDef(intern->msg, f);
}

/**
 * Message_unset_property()
 *
 * Object handler for unsetting a property. Called when PHP code calls:
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
static void Message_unset_property(zend_object* obj, zend_string* member,
                                   void** cache_slot) {
  Message* intern = (Message*)obj;
  const upb_FieldDef* f = get_field(intern, member);

  if (!f) return;

  if (!upb_FieldDef_HasPresence(f)) {
    zend_throw_exception_ex(
        NULL, 0,
        "Cannot call unset() on field %s which does not have presence.",
        upb_FieldDef_Name(f));
    return;
  }

  upb_Message_ClearFieldByDef(intern->msg, f);
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
static zval* Message_read_property(zend_object* obj, zend_string* member,
                                   int type, void** cache_slot, zval* rv) {
  Message* intern = (Message*)obj;
  const upb_FieldDef* f = get_field(intern, member);

  if (!f) return &EG(uninitialized_zval);
  Message_get(intern, f, rv);
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
static zval* Message_write_property(zend_object* obj, zend_string* member,
                                    zval* val, void** cache_slot) {
  Message* intern = (Message*)obj;
  const upb_FieldDef* f = get_field(intern, member);

  if (f && Message_set(intern, f, val)) {
    return val;
  } else {
    return &EG(error_zval);
  }
}

/**
 * Message_get_property_ptr_ptr()
 *
 * Object handler for the get_property_ptr_ptr event in PHP. This returns a
 * reference to our internal properties. We don't support this, so we return
 * NULL.
 */
static zval* Message_get_property_ptr_ptr(zend_object* object,
                                          zend_string* member, int type,
                                          void** cache_slot) {
  return NULL;  // We do not have a properties table.
}

/**
 * Message_clone_obj()
 *
 * Object handler for cloning an object in PHP. Called when PHP code does:
 *
 *   $msg2 = clone $msg;
 */
static zend_object* Message_clone_obj(zend_object* object) {
  Message* intern = (Message*)object;
  const upb_MiniTable* t = upb_MessageDef_MiniTable(intern->desc->msgdef);
  upb_Message* clone = upb_Message_New(t, Arena_Get(&intern->arena));

  // TODO: copy unknown fields?
  // TODO: use official upb msg copy function
  memcpy(clone, intern->msg, t->size);
  zval ret;
  Message_GetPhpWrapper(&ret, intern->desc, clone, &intern->arena);
  return Z_OBJ_P(&ret);
}

/**
 * Message_get_properties()
 *
 * Object handler for the get_properties event in PHP. This returns a HashTable
 * of our internal properties. We don't support this, so we return NULL.
 */
static HashTable* Message_get_properties(zend_object* object) {
  return NULL;  // We don't offer direct references to our properties.
}

// C Functions from message.h. /////////////////////////////////////////////////

// These are documented in the header file.

void Message_GetPhpWrapper(zval* val, const Descriptor* desc, upb_Message* msg,
                           zval* arena) {
  if (!msg) {
    ZVAL_NULL(val);
    return;
  }

  if (!ObjCache_Get(msg, val)) {
    Message* intern = emalloc(sizeof(Message));
    Message_SuppressDefaultProperties(desc->class_entry);
    zend_object_std_init(&intern->std, desc->class_entry);
    intern->std.handlers = &message_object_handlers;
    ZVAL_COPY(&intern->arena, arena);
    intern->desc = desc;
    intern->msg = msg;
    ZVAL_OBJ(val, &intern->std);
    ObjCache_Add(intern->msg, &intern->std);
  }
}

bool Message_GetUpbMessage(zval* val, const Descriptor* desc, upb_Arena* arena,
                           upb_Message** msg) {
  PBPHP_ASSERT(desc);

  if (Z_ISREF_P(val)) {
    ZVAL_DEREF(val);
  }

  if (Z_TYPE_P(val) == IS_OBJECT &&
      instanceof_function(Z_OBJCE_P(val), desc->class_entry)) {
    Message* intern = (Message*)Z_OBJ_P(val);
    upb_Arena_Fuse(arena, Arena_Get(&intern->arena));
    *msg = intern->msg;
    return true;
  } else {
    zend_throw_exception_ex(zend_ce_type_error, 0,
                            "Given value is not an instance of %s.",
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
bool Message_InitFromPhp(upb_Message* msg, const upb_MessageDef* m, zval* init,
                         upb_Arena* arena) {
  HashTable* table = HASH_OF(init);
  HashPosition pos;

  if (Z_ISREF_P(init)) {
    ZVAL_DEREF(init);
  }

  if (Z_TYPE_P(init) != IS_ARRAY) {
    zend_throw_exception_ex(NULL, 0,
                            "Initializer for a message %s must be an array.",
                            upb_MessageDef_FullName(m));
    return false;
  }

  zend_hash_internal_pointer_reset_ex(table, &pos);

  while (true) {  // Iterate over key/value pairs.
    zval key;
    zval* val;
    const upb_FieldDef* f;
    upb_MessageValue msgval;

    zend_hash_get_current_key_zval_ex(table, &key, &pos);
    val = zend_hash_get_current_data_ex(table, &pos);

    if (!val) return true;  // Finished iteration.

    if (Z_ISREF_P(val)) {
      ZVAL_DEREF(val);
    }

    f = upb_MessageDef_FindFieldByNameWithSize(m, Z_STRVAL_P(&key),
                                               Z_STRLEN_P(&key));

    if (!f) {
      zend_throw_exception_ex(NULL, 0, "No such field %s", Z_STRVAL_P(&key));
      return false;
    }

    if (upb_FieldDef_IsMap(f)) {
      msgval.map_val = MapField_GetUpbMap(val, MapType_Get(f), arena);
      if (!msgval.map_val) return false;
    } else if (upb_FieldDef_IsRepeated(f)) {
      msgval.array_val = RepeatedField_GetUpbArray(val, TypeInfo_Get(f), arena);
      if (!msgval.array_val) return false;
    } else {
      if (!Convert_PhpToUpbAutoWrap(val, &msgval, TypeInfo_Get(f), arena)) {
        return false;
      }
    }

    upb_Message_SetFieldByDef(msg, f, msgval, arena);
    zend_hash_move_forward_ex(table, &pos);
    zval_dtor(&key);
  }
}

static void Message_Initialize(Message* intern, const Descriptor* desc) {
  intern->desc = desc;
  const upb_MiniTable* t = upb_MessageDef_MiniTable(desc->msgdef);
  intern->msg = upb_Message_New(t, Arena_Get(&intern->arena));
  ObjCache_Add(intern->msg, &intern->std);
}

/**
 * Message::__construct()
 *
 * Constructor for Message.
 * @param array Map of initial values ['k' = val]
 */
PHP_METHOD(Message, __construct) {
  Message* intern = (Message*)Z_OBJ_P(getThis());
  const Descriptor* desc;
  zend_class_entry* ce = Z_OBJCE_P(getThis());
  upb_Arena* arena = Arena_Get(&intern->arena);
  zval* init_arr = NULL;

  // This descriptor should always be available, as the generated __construct
  // method calls initOnce() to load the descriptor prior to calling us.
  //
  // However, if the user created their own class derived from Message, this
  // will trigger an infinite construction loop and blow the stack.  We
  // store this `ce` in a global variable to break the cycle (see the check in
  // NameMap_GetMessage()).
  NameMap_EnterConstructor(ce);
  desc = Descriptor_GetFromClassEntry(ce);
  NameMap_ExitConstructor(ce);

  if (!desc) {
    zend_throw_exception_ex(
        NULL, 0,
        "Couldn't find descriptor. Note only generated code may derive from "
        "\\Google\\Protobuf\\Internal\\Message");
    return;
  }

  Message_Initialize(intern, desc);

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
  upb_Message_DiscardUnknown(intern->msg, intern->desc->msgdef, 64);
}

/**
 * Message::clear()
 *
 * Clears all fields of this message.
 */
PHP_METHOD(Message, clear) {
  Message* intern = (Message*)Z_OBJ_P(getThis());
  upb_Message_ClearByDef(intern->msg, intern->desc->msgdef);
}

static bool Message_checkEncodeStatus(upb_EncodeStatus status) {
  switch (status) {
    case kUpb_EncodeStatus_Ok:
      return true;
    case kUpb_EncodeStatus_OutOfMemory:
      zend_throw_exception_ex(NULL, 0, "Out of memory");
      return false;
    case kUpb_EncodeStatus_MaxDepthExceeded:
      zend_throw_exception_ex(NULL, 0, "Max nesting exceeded");
      return false;
    case kUpb_EncodeStatus_MissingRequired:
      zend_throw_exception_ex(NULL, 0, "Missing required field");
      return false;
    default:
      zend_throw_exception_ex(NULL, 0, "Unknown error encoding");
      return false;
  }
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
  upb_Arena* arena = Arena_Get(&intern->arena);
  const upb_MiniTable* l = upb_MessageDef_MiniTable(intern->desc->msgdef);
  zval* value;
  char* pb;
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

  // TODO: use a temp arena for this.
  upb_EncodeStatus status = upb_Encode(from->msg, l, 0, arena, &pb, &size);
  if (!Message_checkEncodeStatus(status)) return;

  ok = upb_Decode(pb, size, intern->msg, l, NULL, 0, arena) ==
       kUpb_DecodeStatus_Ok;
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
  char* data = NULL;
  char* data_copy = NULL;
  zend_long data_len;
  const upb_MiniTable* l = upb_MessageDef_MiniTable(intern->desc->msgdef);
  upb_Arena* arena = Arena_Get(&intern->arena);

  if (zend_parse_parameters(ZEND_NUM_ARGS(), "s", &data, &data_len) ==
      FAILURE) {
    return;
  }

  // TODO: avoid this copy when we can make the decoder copy.
  data_copy = upb_Arena_Malloc(arena, data_len);
  memcpy(data_copy, data, data_len);

  if (upb_Decode(data_copy, data_len, intern->msg, l, NULL, 0, arena) !=
      kUpb_DecodeStatus_Ok) {
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
  const upb_MiniTable* l = upb_MessageDef_MiniTable(intern->desc->msgdef);
  upb_Arena* tmp_arena = upb_Arena_New();
  char* data;
  size_t size;

  upb_EncodeStatus status =
      upb_Encode(intern->msg, l, 0, tmp_arena, &data, &size);
  if (!Message_checkEncodeStatus(status)) return;

  if (!data) {
    zend_throw_exception_ex(NULL, 0, "Error occurred during serialization");
    upb_Arena_Free(tmp_arena);
    return;
  }

  RETVAL_STRINGL(data, size);
  upb_Arena_Free(tmp_arena);
}

/**
 * Message::mergeFromJsonString()
 *
 * Merges the JSON data parsed from the given string.
 * @param string Serialized JSON data.
 */
PHP_METHOD(Message, mergeFromJsonString) {
  Message* intern = (Message*)Z_OBJ_P(getThis());
  char* data = NULL;
  char* data_copy = NULL;
  zend_long data_len;
  upb_Arena* arena = Arena_Get(&intern->arena);
  upb_Status status;
  zend_bool ignore_json_unknown = false;
  int options = 0;

  if (zend_parse_parameters(ZEND_NUM_ARGS(), "s|b", &data, &data_len,
                            &ignore_json_unknown) == FAILURE) {
    return;
  }

  // TODO: avoid this copy when we can make the decoder copy.
  data_copy = upb_Arena_Malloc(arena, data_len + 1);
  memcpy(data_copy, data, data_len);
  data_copy[data_len] = '\0';

  if (ignore_json_unknown) {
    options |= upb_JsonDecode_IgnoreUnknown;
  }

  upb_Status_Clear(&status);
  if (!upb_JsonDecode(data_copy, data_len, intern->msg, intern->desc->msgdef,
                      DescriptorPool_GetSymbolTable(), options, arena,
                      &status)) {
    zend_throw_exception_ex(NULL, 0, "Error occurred during parsing: %s",
                            upb_Status_ErrorMessage(&status));
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
  upb_Status status;

  if (zend_parse_parameters(ZEND_NUM_ARGS(), "|b",
                            &preserve_proto_fieldnames) == FAILURE) {
    return;
  }

  if (preserve_proto_fieldnames) {
    options |= upb_JsonEncode_UseProtoNames;
  }

  upb_Status_Clear(&status);
  size = upb_JsonEncode(intern->msg, intern->desc->msgdef,
                        DescriptorPool_GetSymbolTable(), options, buf,
                        sizeof(buf), &status);

  if (!upb_Status_IsOk(&status)) {
    zend_throw_exception_ex(NULL, 0,
                            "Error occurred during JSON serialization: %s",
                            upb_Status_ErrorMessage(&status));
    return;
  }

  if (size >= sizeof(buf)) {
    char* buf2 = malloc(size + 1);
    upb_JsonEncode(intern->msg, intern->desc->msgdef,
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
  const upb_FieldDef* f;
  zend_long size;

  if (zend_parse_parameters(ZEND_NUM_ARGS(), "s", &member, &size) == FAILURE) {
    return;
  }

  f = upb_MessageDef_FindFieldByNameWithSize(intern->desc->msgdef, member,
                                             size);

  if (!f || !IsWrapper(upb_FieldDef_MessageSubDef(f))) {
    zend_throw_exception_ex(NULL, 0, "Message %s has no field %s",
                            upb_MessageDef_FullName(intern->desc->msgdef),
                            member);
    return;
  }

  if (upb_Message_HasFieldByDef(intern->msg, f)) {
    const upb_Message* wrapper =
        upb_Message_GetFieldByDef(intern->msg, f).msg_val;
    const upb_MessageDef* m = upb_FieldDef_MessageSubDef(f);
    const upb_FieldDef* val_f = upb_MessageDef_FindFieldByNumber(m, 1);
    upb_MessageValue msgval = upb_Message_GetFieldByDef(wrapper, val_f);
    zval ret;
    Convert_UpbToPhp(msgval, &ret, TypeInfo_Get(val_f), &intern->arena);
    RETURN_COPY_VALUE(&ret);
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
  upb_Arena* arena = Arena_Get(&intern->arena);
  char* member;
  const upb_FieldDef* f;
  upb_MessageValue msgval;
  zend_long size;
  zval* val;

  if (zend_parse_parameters(ZEND_NUM_ARGS(), "sz", &member, &size, &val) ==
      FAILURE) {
    return;
  }

  f = upb_MessageDef_FindFieldByNameWithSize(intern->desc->msgdef, member,
                                             size);

  if (!f || !IsWrapper(upb_FieldDef_MessageSubDef(f))) {
    zend_throw_exception_ex(NULL, 0, "Message %s has no field %s",
                            upb_MessageDef_FullName(intern->desc->msgdef),
                            member);
    return;
  }

  if (Z_ISREF_P(val)) {
    ZVAL_DEREF(val);
  }

  if (Z_TYPE_P(val) == IS_NULL) {
    upb_Message_ClearFieldByDef(intern->msg, f);
  } else {
    const upb_MessageDef* m = upb_FieldDef_MessageSubDef(f);
    const upb_FieldDef* val_f = upb_MessageDef_FindFieldByNumber(m, 1);
    upb_Message* wrapper;

    if (!Convert_PhpToUpb(val, &msgval, TypeInfo_Get(val_f), arena)) {
      return;  // Error is already set.
    }

    wrapper = upb_Message_Mutable(intern->msg, f, arena).msg;
    upb_Message_SetFieldByDef(wrapper, val_f, msgval, arena);
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
  const upb_OneofDef* oneof;
  const upb_FieldDef* field;
  char* name;
  zend_long len;

  if (zend_parse_parameters(ZEND_NUM_ARGS(), "s", &name, &len) == FAILURE) {
    return;
  }

  oneof =
      upb_MessageDef_FindOneofByNameWithSize(intern->desc->msgdef, name, len);

  if (!oneof) {
    zend_throw_exception_ex(NULL, 0, "Message %s has no oneof %s",
                            upb_MessageDef_FullName(intern->desc->msgdef),
                            name);
    return;
  }

  field = upb_Message_WhichOneof(intern->msg, oneof);
  RETURN_STRING(field ? upb_FieldDef_Name(field) : "");
}

/**
 * Message::hasOneof()
 *
 * Returns the presence of the given oneof field, given a field number. Called
 * from generated code methods such as:
 *
 *    public function hasDoubleValueOneof()
 *    {
 *        return $this->hasOneof(10);
 *    }
 *
 * @return boolean
 */
PHP_METHOD(Message, hasOneof) {
  Message* intern = (Message*)Z_OBJ_P(getThis());
  zend_long field_num;
  const upb_FieldDef* f;

  if (zend_parse_parameters(ZEND_NUM_ARGS(), "l", &field_num) == FAILURE) {
    return;
  }

  f = upb_MessageDef_FindFieldByNumber(intern->desc->msgdef, field_num);

  if (!f || !upb_FieldDef_RealContainingOneof(f)) {
    php_error_docref(NULL, E_USER_ERROR,
                     "Internal error, no such oneof field %d\n",
                     (int)field_num);
  }

  RETVAL_BOOL(upb_Message_HasFieldByDef(intern->msg, f));
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
  const upb_FieldDef* f;
  zval ret;

  if (zend_parse_parameters(ZEND_NUM_ARGS(), "l", &field_num) == FAILURE) {
    return;
  }

  f = upb_MessageDef_FindFieldByNumber(intern->desc->msgdef, field_num);

  if (!f || !upb_FieldDef_RealContainingOneof(f)) {
    php_error_docref(NULL, E_USER_ERROR,
                     "Internal error, no such oneof field %d\n",
                     (int)field_num);
  }

  if (upb_FieldDef_IsSubMessage(f) &&
      !upb_Message_HasFieldByDef(intern->msg, f)) {
    RETURN_NULL();
  }

  {
    upb_MessageValue msgval = upb_Message_GetFieldByDef(intern->msg, f);
    Convert_UpbToPhp(msgval, &ret, TypeInfo_Get(f), &intern->arena);
  }

  RETURN_COPY_VALUE(&ret);
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
  const upb_FieldDef* f;
  upb_Arena* arena = Arena_Get(&intern->arena);
  upb_MessageValue msgval;
  zval* val;

  if (zend_parse_parameters(ZEND_NUM_ARGS(), "lz", &field_num, &val) ==
      FAILURE) {
    return;
  }

  f = upb_MessageDef_FindFieldByNumber(intern->desc->msgdef, field_num);

  if (upb_FieldDef_IsSubMessage(f) && Z_TYPE_P(val) == IS_NULL) {
    upb_Message_ClearFieldByDef(intern->msg, f);
    return;
  } else if (!Convert_PhpToUpb(val, &msgval, TypeInfo_Get(f), arena)) {
    return;
  }

  upb_Message_SetFieldByDef(intern->msg, f, msgval, arena);
}

// clang-format off
ZEND_BEGIN_ARG_INFO_EX(arginfo_construct, 0, 0, 0)
  ZEND_ARG_INFO(0, data)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_mergeFrom, 0, 0, 1)
  ZEND_ARG_INFO(0, data)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_mergeFromWithArg, 0, 0, 1)
  ZEND_ARG_INFO(0, data)
  ZEND_ARG_INFO(0, arg)
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
  PHP_ME(Message, mergeFromJsonString,   arginfo_mergeFromWithArg, ZEND_ACC_PUBLIC)
  PHP_ME(Message, mergeFrom,             arginfo_mergeFrom, ZEND_ACC_PUBLIC)
  PHP_ME(Message, readWrapperValue,      arginfo_read,      ZEND_ACC_PROTECTED)
  PHP_ME(Message, writeWrapperValue,     arginfo_write,     ZEND_ACC_PROTECTED)
  PHP_ME(Message, hasOneof,              arginfo_read,      ZEND_ACC_PROTECTED)
  PHP_ME(Message, readOneof,             arginfo_read,      ZEND_ACC_PROTECTED)
  PHP_ME(Message, writeOneof,            arginfo_write,     ZEND_ACC_PROTECTED)
  PHP_ME(Message, whichOneof,            arginfo_read,      ZEND_ACC_PROTECTED)
  PHP_ME(Message, __construct,           arginfo_construct, ZEND_ACC_PROTECTED)
  ZEND_FE_END
};
// clang-format on

// Well-known types ////////////////////////////////////////////////////////////

static const char TYPE_URL_PREFIX[] = "type.googleapis.com/";

static upb_MessageValue Message_getval(Message* intern,
                                       const char* field_name) {
  const upb_FieldDef* f =
      upb_MessageDef_FindFieldByName(intern->desc->msgdef, field_name);
  PBPHP_ASSERT(f);
  return upb_Message_GetFieldByDef(intern->msg, f);
}

static void Message_setval(Message* intern, const char* field_name,
                           upb_MessageValue val) {
  const upb_FieldDef* f =
      upb_MessageDef_FindFieldByName(intern->desc->msgdef, field_name);
  PBPHP_ASSERT(f);
  upb_Message_SetFieldByDef(intern->msg, f, val, Arena_Get(&intern->arena));
}

static upb_MessageValue StringVal(upb_StringView view) {
  upb_MessageValue ret;
  ret.str_val = view;
  return ret;
}

static bool TryStripUrlPrefix(upb_StringView* str) {
  size_t size = strlen(TYPE_URL_PREFIX);
  if (str->size < size || memcmp(TYPE_URL_PREFIX, str->data, size) != 0) {
    return false;
  }
  str->data += size;
  str->size -= size;
  return true;
}

static bool StrViewEq(upb_StringView view, const char* str) {
  size_t size = strlen(str);
  return view.size == size && memcmp(view.data, str, size) == 0;
}

PHP_METHOD(google_protobuf_Any, unpack) {
  Message* intern = (Message*)Z_OBJ_P(getThis());
  upb_StringView type_url = Message_getval(intern, "type_url").str_val;
  upb_StringView value = Message_getval(intern, "value").str_val;
  upb_DefPool* symtab = DescriptorPool_GetSymbolTable();
  const upb_MessageDef* m;
  Descriptor* desc;
  zval ret;

  // Ensure that type_url has TYPE_URL_PREFIX as a prefix.
  if (!TryStripUrlPrefix(&type_url)) {
    zend_throw_exception(
        NULL, "Type url needs to be type.googleapis.com/fully-qualified", 0);
    return;
  }

  m = upb_DefPool_FindMessageByNameWithSize(symtab, type_url.data,
                                            type_url.size);

  if (m == NULL) {
    zend_throw_exception(
        NULL, "Specified message in any hasn't been added to descriptor pool",
        0);
    return;
  }

  desc = Descriptor_GetFromMessageDef(m);
  PBPHP_ASSERT(desc->class_entry->create_object == Message_create);
  zend_object* obj = Message_create(desc->class_entry);
  Message* msg = (Message*)obj;
  Message_Initialize(msg, desc);
  ZVAL_OBJ(&ret, obj);

  // Get value.
  if (upb_Decode(value.data, value.size, msg->msg,
                 upb_MessageDef_MiniTable(desc->msgdef), NULL, 0,
                 Arena_Get(&msg->arena)) != kUpb_DecodeStatus_Ok) {
    zend_throw_exception_ex(NULL, 0, "Error occurred during parsing");
    zval_dtor(&ret);
    return;
  }

  // Fuse since the parsed message could alias "value".
  upb_Arena_Fuse(Arena_Get(&intern->arena), Arena_Get(&msg->arena));

  RETURN_COPY_VALUE(&ret);
}

PHP_METHOD(google_protobuf_Any, pack) {
  Message* intern = (Message*)Z_OBJ_P(getThis());
  upb_Arena* arena = Arena_Get(&intern->arena);
  zval* val;
  Message* msg;
  upb_StringView value;
  upb_StringView type_url;
  const char* full_name;
  char* buf;

  if (zend_parse_parameters(ZEND_NUM_ARGS(), "o", &val) == FAILURE) {
    return;
  }

  if (!instanceof_function(Z_OBJCE_P(val), message_ce)) {
    zend_error(E_USER_ERROR, "Given value is not an instance of Message.");
    return;
  }

  msg = (Message*)Z_OBJ_P(val);

  // Serialize and set value.
  char* pb;
  upb_EncodeStatus status =
      upb_Encode(msg->msg, upb_MessageDef_MiniTable(msg->desc->msgdef), 0,
                 arena, &pb, &value.size);
  if (!Message_checkEncodeStatus(status)) return;
  value.data = pb;
  Message_setval(intern, "value", StringVal(value));

  // Set type url: type_url_prefix + fully_qualified_name
  full_name = upb_MessageDef_FullName(msg->desc->msgdef);
  type_url.size = strlen(TYPE_URL_PREFIX) + strlen(full_name);
  buf = upb_Arena_Malloc(arena, type_url.size + 1);
  memcpy(buf, TYPE_URL_PREFIX, strlen(TYPE_URL_PREFIX));
  memcpy(buf + strlen(TYPE_URL_PREFIX), full_name, strlen(full_name));
  type_url.data = buf;
  Message_setval(intern, "type_url", StringVal(type_url));
}

PHP_METHOD(google_protobuf_Any, is) {
  Message* intern = (Message*)Z_OBJ_P(getThis());
  upb_StringView type_url = Message_getval(intern, "type_url").str_val;
  zend_class_entry* klass = NULL;
  const upb_MessageDef* m;

  if (zend_parse_parameters(ZEND_NUM_ARGS(), "C", &klass) == FAILURE) {
    return;
  }

  m = NameMap_GetMessage(klass);

  if (m == NULL) {
    RETURN_BOOL(false);
  }

  RETURN_BOOL(TryStripUrlPrefix(&type_url) &&
              StrViewEq(type_url, upb_MessageDef_FullName(m)));
}

PHP_METHOD(google_protobuf_Timestamp, fromDateTime) {
  Message* intern = (Message*)Z_OBJ_P(getThis());
  zval* datetime;
  const char* classname = "\\DatetimeInterface";
  zend_string* classname_str =
      zend_string_init(classname, strlen(classname), 0);
  zend_class_entry* date_interface_ce = zend_lookup_class(classname_str);
  zend_string_release(classname_str);

  if (date_interface_ce == NULL) {
    zend_error(E_ERROR, "Make sure date extension is enabled.");
    return;
  }

  if (zend_parse_parameters(ZEND_NUM_ARGS(), "O", &datetime,
                            date_interface_ce) == FAILURE) {
    zend_error(E_USER_ERROR, "Expect DatetimeInterface.");
    return;
  }

  upb_MessageValue timestamp_seconds;
  {
    zval retval;
    zval function_name;

    ZVAL_STRING(&function_name, "date_timestamp_get");

    if (call_user_function(EG(function_table), NULL, &function_name, &retval, 1,
                           datetime) == FAILURE ||
        !Convert_PhpToUpb(&retval, &timestamp_seconds,
                          TypeInfo_FromType(kUpb_CType_Int64), NULL)) {
      zend_error(E_ERROR, "Cannot get timestamp from DateTime.");
      return;
    }

    zval_dtor(&retval);
    zval_dtor(&function_name);
  }

  upb_MessageValue timestamp_nanos;
  {
    zval retval;
    zval function_name;
    zval format_string;

    ZVAL_STRING(&function_name, "date_format");
    ZVAL_STRING(&format_string, "u");

    zval params[2] = {
        *datetime,
        format_string,
    };

    if (call_user_function(EG(function_table), NULL, &function_name, &retval, 2,
                           params) == FAILURE ||
        !Convert_PhpToUpb(&retval, &timestamp_nanos,
                          TypeInfo_FromType(kUpb_CType_Int32), NULL)) {
      zend_error(E_ERROR, "Cannot format DateTime.");
      return;
    }

    timestamp_nanos.int32_val *= 1000;

    zval_dtor(&retval);
    zval_dtor(&function_name);
    zval_dtor(&format_string);
  }

  Message_setval(intern, "seconds", timestamp_seconds);
  Message_setval(intern, "nanos", timestamp_nanos);

  RETURN_NULL();
}

PHP_METHOD(google_protobuf_Timestamp, toDateTime) {
  Message* intern = (Message*)Z_OBJ_P(getThis());
  upb_MessageValue seconds = Message_getval(intern, "seconds");
  upb_MessageValue nanos = Message_getval(intern, "nanos");

  // Get formatted time string.
  char formatted_time[32];
  snprintf(formatted_time, sizeof(formatted_time), "%" PRId64 ".%06" PRId32,
           seconds.int64_val, nanos.int32_val / 1000);

  // Create Datetime object.
  zval datetime;
  zval function_name;
  zval format_string;
  zval formatted_time_php;

  ZVAL_STRING(&function_name, "date_create_from_format");
  ZVAL_STRING(&format_string, "U.u");
  ZVAL_STRING(&formatted_time_php, formatted_time);

  zval params[2] = {
      format_string,
      formatted_time_php,
  };

  if (call_user_function(EG(function_table), NULL, &function_name, &datetime, 2,
                         params) == FAILURE) {
    zend_error(E_ERROR, "Cannot create DateTime.");
    return;
  }

  zval_dtor(&function_name);
  zval_dtor(&format_string);
  zval_dtor(&formatted_time_php);

  ZVAL_OBJ(return_value, Z_OBJ(datetime));
}

#include "wkt.inc"

/**
 * Message_ModuleInit()
 *
 * Called when the C extension is loaded to register all types.
 */
void Message_ModuleInit() {
  zend_class_entry tmp_ce;
  zend_object_handlers* h = &message_object_handlers;

  INIT_CLASS_ENTRY(tmp_ce, "Google\\Protobuf\\Internal\\Message",
                   Message_methods);

  message_ce = zend_register_internal_class(&tmp_ce);
  message_ce->create_object = Message_create;

  memcpy(h, &std_object_handlers, sizeof(zend_object_handlers));
  h->dtor_obj = Message_dtor;
  h->compare = Message_compare_objects;
  h->read_property = Message_read_property;
  h->write_property = Message_write_property;
  h->has_property = Message_has_property;
  h->unset_property = Message_unset_property;
  h->get_properties = Message_get_properties;
  h->get_property_ptr_ptr = Message_get_property_ptr_ptr;
  h->clone_obj = Message_clone_obj;

  WellKnownTypes_ModuleInit(); /* From wkt.inc. */
}
