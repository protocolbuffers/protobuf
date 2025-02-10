// Protocol Buffers - Google's data interchange format
// Copyright 2014 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "message.h"

#include "convert.h"
#include "defs.h"
#include "map.h"
#include "protobuf.h"
#include "repeated_field.h"
#include "shared_message.h"

static VALUE cParseError = Qnil;
static VALUE cAbstractMessage = Qnil;
static ID descriptor_instancevar_interned;

static VALUE initialize_rb_class_with_no_args(VALUE klass) {
  return rb_funcall(klass, rb_intern("new"), 0);
}

VALUE MessageOrEnum_GetDescriptor(VALUE klass) {
  return rb_ivar_get(klass, descriptor_instancevar_interned);
}

// -----------------------------------------------------------------------------
// Class/module creation from msgdefs and enumdefs, respectively.
// -----------------------------------------------------------------------------

typedef struct {
  // IMPORTANT: WB_PROTECTED objects must only use the RB_OBJ_WRITE()
  // macro to update VALUE references, as to trigger write barriers.
  VALUE arena;
  const upb_Message* msg;  // Can get as mutable when non-frozen.
  const upb_MessageDef*
      msgdef;  // kept alive by self.class.descriptor reference.
} Message;

static void Message_mark(void* _self) {
  Message* self = (Message*)_self;
  rb_gc_mark(self->arena);
}

static rb_data_type_t Message_type = {
    "Google::Protobuf::Message",
    {Message_mark, RUBY_DEFAULT_FREE, NULL},
    .flags = RUBY_TYPED_FREE_IMMEDIATELY | RUBY_TYPED_WB_PROTECTED,
};

static Message* ruby_to_Message(VALUE msg_rb) {
  Message* msg;
  TypedData_Get_Struct(msg_rb, Message, &Message_type, msg);
  return msg;
}

static VALUE Message_alloc(VALUE klass) {
  VALUE descriptor = rb_ivar_get(klass, descriptor_instancevar_interned);
  Message* msg = ALLOC(Message);
  VALUE ret;

  msg->msgdef = Descriptor_GetMsgDef(descriptor);
  msg->arena = Qnil;
  msg->msg = NULL;

  ret = TypedData_Wrap_Struct(klass, &Message_type, msg);
  rb_ivar_set(ret, descriptor_instancevar_interned, descriptor);

  return ret;
}

const upb_Message* Message_Get(VALUE msg_rb, const upb_MessageDef** m) {
  Message* msg = ruby_to_Message(msg_rb);
  if (m) *m = msg->msgdef;
  return msg->msg;
}

upb_Message* Message_GetMutable(VALUE msg_rb, const upb_MessageDef** m) {
  rb_check_frozen(msg_rb);
  return (upb_Message*)Message_Get(msg_rb, m);
}

void Message_InitPtr(VALUE self_, upb_Message* msg, VALUE arena) {
  Message* self = ruby_to_Message(self_);
  self->msg = msg;
  RB_OBJ_WRITE(self_, &self->arena, arena);
  VALUE stored = ObjectCache_TryAdd(msg, self_);
  (void)stored;
  PBRUBY_ASSERT(stored == self_);
}

VALUE Message_GetArena(VALUE msg_rb) {
  Message* msg = ruby_to_Message(msg_rb);
  return msg->arena;
}

void Message_CheckClass(VALUE klass) {
  if (rb_get_alloc_func(klass) != &Message_alloc) {
    rb_raise(rb_eArgError,
             "Message class was not returned by the DescriptorPool.");
  }
}

VALUE Message_GetRubyWrapper(upb_Message* msg, const upb_MessageDef* m,
                             VALUE arena) {
  if (msg == NULL) return Qnil;

  VALUE val = ObjectCache_Get(msg);

  if (val == Qnil) {
    VALUE klass = Descriptor_DefToClass(m);
    val = Message_alloc(klass);
    Message_InitPtr(val, msg, arena);
  }

  return val;
}

void Message_PrintMessage(StringBuilder* b, const upb_Message* msg,
                          const upb_MessageDef* m) {
  bool first = true;
  int n = upb_MessageDef_FieldCount(m);
  VALUE klass = Descriptor_DefToClass(m);
  StringBuilder_Printf(b, "<%s: ", rb_class2name(klass));

  for (int i = 0; i < n; i++) {
    const upb_FieldDef* field = upb_MessageDef_Field(m, i);

    if (upb_FieldDef_HasPresence(field) &&
        !upb_Message_HasFieldByDef(msg, field)) {
      continue;
    }

    if (!first) {
      StringBuilder_Printf(b, ", ");
    } else {
      first = false;
    }

    upb_MessageValue msgval = upb_Message_GetFieldByDef(msg, field);

    StringBuilder_Printf(b, "%s: ", upb_FieldDef_Name(field));

    if (upb_FieldDef_IsMap(field)) {
      const upb_MessageDef* entry_m = upb_FieldDef_MessageSubDef(field);
      const upb_FieldDef* key_f = upb_MessageDef_FindFieldByNumber(entry_m, 1);
      const upb_FieldDef* val_f = upb_MessageDef_FindFieldByNumber(entry_m, 2);
      TypeInfo val_info = TypeInfo_get(val_f);
      Map_Inspect(b, msgval.map_val, upb_FieldDef_CType(key_f), val_info);
    } else if (upb_FieldDef_IsRepeated(field)) {
      RepeatedField_Inspect(b, msgval.array_val, TypeInfo_get(field));
    } else {
      StringBuilder_PrintMsgval(b, msgval, TypeInfo_get(field));
    }
  }

  StringBuilder_Printf(b, ">");
}

// Helper functions for #method_missing ////////////////////////////////////////

enum {
  METHOD_UNKNOWN = 0,
  METHOD_GETTER = 1,
  METHOD_SETTER = 2,
  METHOD_CLEAR = 3,
  METHOD_PRESENCE = 4,
  METHOD_ENUM_GETTER = 5,
  METHOD_WRAPPER_GETTER = 6,
  METHOD_WRAPPER_SETTER = 7
};

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

static bool IsFieldWrapper(const upb_FieldDef* f) {
  return IsWrapper(upb_FieldDef_MessageSubDef(f));
}

static bool Match(const upb_MessageDef* m, const char* name,
                  const upb_FieldDef** f, const upb_OneofDef** o,
                  const char* prefix, const char* suffix) {
  size_t sp = strlen(prefix);
  size_t ss = strlen(suffix);
  size_t sn = strlen(name);

  if (sn <= sp + ss) return false;

  if (memcmp(name, prefix, sp) != 0 ||
      memcmp(name + sn - ss, suffix, ss) != 0) {
    return false;
  }

  return upb_MessageDef_FindByNameWithSize(m, name + sp, sn - sp - ss, f, o);
}

static int extract_method_call(VALUE method_name, Message* self,
                               const upb_FieldDef** f, const upb_OneofDef** o) {
  const upb_MessageDef* m = self->msgdef;
  const char* name;

  Check_Type(method_name, T_SYMBOL);
  name = rb_id2name(SYM2ID(method_name));

  if (Match(m, name, f, o, "", "")) return METHOD_GETTER;
  if (Match(m, name, f, o, "", "=")) return METHOD_SETTER;
  if (Match(m, name, f, o, "clear_", "")) return METHOD_CLEAR;
  if (Match(m, name, f, o, "has_", "?") &&
      (*o || (*f && upb_FieldDef_HasPresence(*f)))) {
    return METHOD_PRESENCE;
  }
  if (Match(m, name, f, o, "", "_as_value") && *f &&
      !upb_FieldDef_IsRepeated(*f) && IsFieldWrapper(*f)) {
    return METHOD_WRAPPER_GETTER;
  }
  if (Match(m, name, f, o, "", "_as_value=") && *f &&
      !upb_FieldDef_IsRepeated(*f) && IsFieldWrapper(*f)) {
    return METHOD_WRAPPER_SETTER;
  }
  if (Match(m, name, f, o, "", "_const") && *f &&
      upb_FieldDef_CType(*f) == kUpb_CType_Enum) {
    return METHOD_ENUM_GETTER;
  }

  return METHOD_UNKNOWN;
}

static VALUE Message_oneof_accessor(VALUE _self, const upb_OneofDef* o,
                                    int accessor_type) {
  Message* self = ruby_to_Message(_self);
  const upb_FieldDef* oneof_field = upb_Message_WhichOneof(self->msg, o);

  switch (accessor_type) {
    case METHOD_PRESENCE:
      return oneof_field == NULL ? Qfalse : Qtrue;
    case METHOD_CLEAR:
      if (oneof_field != NULL) {
        upb_Message_ClearFieldByDef(Message_GetMutable(_self, NULL),
                                    oneof_field);
      }
      return Qnil;
    case METHOD_GETTER:
      return oneof_field == NULL
                 ? Qnil
                 : ID2SYM(rb_intern(upb_FieldDef_Name(oneof_field)));
    case METHOD_SETTER:
      rb_raise(rb_eRuntimeError, "Oneof accessors are read-only.");
  }
  rb_raise(rb_eRuntimeError, "Invalid access of oneof field.");
}

static void Message_setfield(upb_Message* msg, const upb_FieldDef* f, VALUE val,
                             upb_Arena* arena) {
  upb_MessageValue msgval;
  if (upb_FieldDef_IsMap(f)) {
    msgval.map_val = Map_GetUpbMap(val, f, arena);
  } else if (upb_FieldDef_IsRepeated(f)) {
    msgval.array_val = RepeatedField_GetUpbArray(val, f, arena);
  } else {
    if (val == Qnil &&
        (upb_FieldDef_IsSubMessage(f) || upb_FieldDef_RealContainingOneof(f))) {
      upb_Message_ClearFieldByDef(msg, f);
      return;
    }
    msgval =
        Convert_RubyToUpb(val, upb_FieldDef_Name(f), TypeInfo_get(f), arena);
  }
  upb_Message_SetFieldByDef(msg, f, msgval, arena);
}

VALUE Message_getfield(VALUE _self, const upb_FieldDef* f) {
  Message* self = ruby_to_Message(_self);
  // This is a special-case: upb_Message_Mutable() for map & array are logically
  // const (they will not change what is serialized) but physically
  // non-const, as they do allocate a repeated field or map. The logical
  // constness means it's ok to do even if the message is frozen.
  upb_Message* msg = (upb_Message*)self->msg;
  upb_Arena* arena = Arena_get(self->arena);
  if (upb_FieldDef_IsMap(f)) {
    upb_Map* map = upb_Message_Mutable(msg, f, arena).map;
    const upb_FieldDef* key_f = map_field_key(f);
    const upb_FieldDef* val_f = map_field_value(f);
    upb_CType key_type = upb_FieldDef_CType(key_f);
    TypeInfo value_type_info = TypeInfo_get(val_f);
    return Map_GetRubyWrapper(map, key_type, value_type_info, self->arena);
  } else if (upb_FieldDef_IsRepeated(f)) {
    upb_Array* arr = upb_Message_Mutable(msg, f, arena).array;
    return RepeatedField_GetRubyWrapper(arr, TypeInfo_get(f), self->arena);
  } else if (upb_FieldDef_IsSubMessage(f)) {
    if (!upb_Message_HasFieldByDef(self->msg, f)) return Qnil;
    upb_Message* submsg = upb_Message_Mutable(msg, f, arena).msg;
    const upb_MessageDef* m = upb_FieldDef_MessageSubDef(f);
    return Message_GetRubyWrapper(submsg, m, self->arena);
  } else {
    upb_MessageValue msgval = upb_Message_GetFieldByDef(self->msg, f);
    return Convert_UpbToRuby(msgval, TypeInfo_get(f), self->arena);
  }
}

static VALUE Message_field_accessor(VALUE _self, const upb_FieldDef* f,
                                    int accessor_type, int argc, VALUE* argv) {
  upb_Arena* arena = Arena_get(Message_GetArena(_self));

  switch (accessor_type) {
    case METHOD_SETTER:
      Message_setfield(Message_GetMutable(_self, NULL), f, argv[1], arena);
      return Qnil;
    case METHOD_CLEAR:
      upb_Message_ClearFieldByDef(Message_GetMutable(_self, NULL), f);
      return Qnil;
    case METHOD_PRESENCE:
      if (!upb_FieldDef_HasPresence(f)) {
        rb_raise(rb_eRuntimeError, "Field does not have presence.");
      }
      return upb_Message_HasFieldByDef(Message_Get(_self, NULL), f);
    case METHOD_WRAPPER_GETTER: {
      Message* self = ruby_to_Message(_self);
      if (upb_Message_HasFieldByDef(self->msg, f)) {
        PBRUBY_ASSERT(upb_FieldDef_IsSubMessage(f) &&
                      !upb_FieldDef_IsRepeated(f));
        upb_MessageValue wrapper = upb_Message_GetFieldByDef(self->msg, f);
        const upb_MessageDef* wrapper_m = upb_FieldDef_MessageSubDef(f);
        const upb_FieldDef* value_f =
            upb_MessageDef_FindFieldByNumber(wrapper_m, 1);
        upb_MessageValue value =
            upb_Message_GetFieldByDef(wrapper.msg_val, value_f);
        return Convert_UpbToRuby(value, TypeInfo_get(value_f), self->arena);
      } else {
        return Qnil;
      }
    }
    case METHOD_WRAPPER_SETTER: {
      upb_Message* msg = Message_GetMutable(_self, NULL);
      if (argv[1] == Qnil) {
        upb_Message_ClearFieldByDef(msg, f);
      } else {
        const upb_FieldDef* val_f =
            upb_MessageDef_FindFieldByNumber(upb_FieldDef_MessageSubDef(f), 1);
        upb_MessageValue msgval = Convert_RubyToUpb(
            argv[1], upb_FieldDef_Name(f), TypeInfo_get(val_f), arena);
        upb_Message* wrapper = upb_Message_Mutable(msg, f, arena).msg;
        upb_Message_SetFieldByDef(wrapper, val_f, msgval, arena);
      }
      return Qnil;
    }
    case METHOD_ENUM_GETTER: {
      upb_MessageValue msgval =
          upb_Message_GetFieldByDef(Message_Get(_self, NULL), f);

      if (upb_FieldDef_Label(f) == kUpb_Label_Repeated) {
        // Map repeated fields to a new type with ints
        VALUE arr = rb_ary_new();
        size_t i, n = upb_Array_Size(msgval.array_val);
        for (i = 0; i < n; i++) {
          upb_MessageValue elem = upb_Array_Get(msgval.array_val, i);
          rb_ary_push(arr, INT2NUM(elem.int32_val));
        }
        return arr;
      } else {
        return INT2NUM(msgval.int32_val);
      }
    }
    case METHOD_GETTER:
      return Message_getfield(_self, f);
    default:
      rb_raise(rb_eRuntimeError, "Internal error, no such accessor: %d",
               accessor_type);
  }
}

/*
 * call-seq:
 *     Message.method_missing(*args)
 *
 * Provides accessors and setters and methods to clear and check for presence of
 * message fields according to their field names.
 *
 * For any field whose name does not conflict with a built-in method, an
 * accessor is provided with the same name as the field, and a setter is
 * provided with the name of the field plus the '=' suffix. Thus, given a
 * message instance 'msg' with field 'foo', the following code is valid:
 *
 *     msg.foo = 42
 *     puts msg.foo
 *
 * This method also provides read-only accessors for oneofs. If a oneof exists
 * with name 'my_oneof', then msg.my_oneof will return a Ruby symbol equal to
 * the name of the field in that oneof that is currently set, or nil if none.
 *
 * It also provides methods of the form 'clear_fieldname' to clear the value
 * of the field 'fieldname'. For basic data types, this will set the default
 * value of the field.
 *
 * Additionally, it provides methods of the form 'has_fieldname?', which returns
 * true if the field 'fieldname' is set in the message object, else false. For
 * 'proto3' syntax, calling this for a basic type field will result in an error.
 */
static VALUE Message_method_missing(int argc, VALUE* argv, VALUE _self) {
  Message* self = ruby_to_Message(_self);
  const upb_OneofDef* o;
  const upb_FieldDef* f;
  int accessor_type;

  if (argc < 1) {
    rb_raise(rb_eArgError, "Expected method name as first argument.");
  }

  accessor_type = extract_method_call(argv[0], self, &f, &o);

  if (accessor_type == METHOD_UNKNOWN) return rb_call_super(argc, argv);

  // Validate argument count.
  switch (accessor_type) {
    case METHOD_SETTER:
    case METHOD_WRAPPER_SETTER:
      if (argc != 2) {
        rb_raise(rb_eArgError, "Expected 2 arguments, received %d", argc);
      }
      rb_check_frozen(_self);
      break;
    default:
      if (argc != 1) {
        rb_raise(rb_eArgError, "Expected 1 argument, received %d", argc);
      }
      break;
  }

  // Dispatch accessor.
  if (o != NULL) {
    return Message_oneof_accessor(_self, o, accessor_type);
  } else {
    return Message_field_accessor(_self, f, accessor_type, argc, argv);
  }
}

static VALUE Message_respond_to_missing(int argc, VALUE* argv, VALUE _self) {
  Message* self = ruby_to_Message(_self);
  const upb_OneofDef* o;
  const upb_FieldDef* f;
  int accessor_type;

  if (argc < 1) {
    rb_raise(rb_eArgError, "Expected method name as first argument.");
  }

  accessor_type = extract_method_call(argv[0], self, &f, &o);

  if (accessor_type == METHOD_UNKNOWN) {
    return rb_call_super(argc, argv);
  } else if (o != NULL) {
    return accessor_type == METHOD_SETTER ? Qfalse : Qtrue;
  } else {
    return Qtrue;
  }
}

void Message_InitFromValue(upb_Message* msg, const upb_MessageDef* m, VALUE val,
                           upb_Arena* arena);

typedef struct {
  upb_Map* map;
  TypeInfo key_type;
  TypeInfo val_type;
  upb_Arena* arena;
} MapInit;

static int Map_initialize_kwarg(VALUE key, VALUE val, VALUE _self) {
  MapInit* map_init = (MapInit*)_self;
  upb_MessageValue k, v;
  k = Convert_RubyToUpb(key, "", map_init->key_type, NULL);

  if (map_init->val_type.type == kUpb_CType_Message && TYPE(val) == T_HASH) {
    const upb_MiniTable* t =
        upb_MessageDef_MiniTable(map_init->val_type.def.msgdef);
    upb_Message* msg = upb_Message_New(t, map_init->arena);
    Message_InitFromValue(msg, map_init->val_type.def.msgdef, val,
                          map_init->arena);
    v.msg_val = msg;
  } else {
    v = Convert_RubyToUpb(val, "", map_init->val_type, map_init->arena);
  }
  upb_Map_Set(map_init->map, k, v, map_init->arena);
  return ST_CONTINUE;
}

static void Map_InitFromValue(upb_Map* map, const upb_FieldDef* f, VALUE val,
                              upb_Arena* arena) {
  const upb_MessageDef* entry_m = upb_FieldDef_MessageSubDef(f);
  const upb_FieldDef* key_f = upb_MessageDef_FindFieldByNumber(entry_m, 1);
  const upb_FieldDef* val_f = upb_MessageDef_FindFieldByNumber(entry_m, 2);
  if (TYPE(val) != T_HASH) {
    rb_raise(rb_eArgError,
             "Expected Hash object as initializer value for map field '%s' "
             "(given %s).",
             upb_FieldDef_Name(f), rb_class2name(CLASS_OF(val)));
  }
  MapInit map_init = {map, TypeInfo_get(key_f), TypeInfo_get(val_f), arena};
  rb_hash_foreach(val, Map_initialize_kwarg, (VALUE)&map_init);
}

static upb_MessageValue MessageValue_FromValue(VALUE val, TypeInfo info,
                                               upb_Arena* arena) {
  if (info.type == kUpb_CType_Message) {
    upb_MessageValue msgval;
    const upb_MiniTable* t = upb_MessageDef_MiniTable(info.def.msgdef);
    upb_Message* msg = upb_Message_New(t, arena);
    Message_InitFromValue(msg, info.def.msgdef, val, arena);
    msgval.msg_val = msg;
    return msgval;
  } else {
    return Convert_RubyToUpb(val, "", info, arena);
  }
}

static void RepeatedField_InitFromValue(upb_Array* arr, const upb_FieldDef* f,
                                        VALUE val, upb_Arena* arena) {
  TypeInfo type_info = TypeInfo_get(f);

  if (TYPE(val) != T_ARRAY) {
    rb_raise(rb_eArgError,
             "Expected array as initializer value for repeated field '%s' "
             "(given %s).",
             upb_FieldDef_Name(f), rb_class2name(CLASS_OF(val)));
  }

  for (int i = 0; i < RARRAY_LEN(val); i++) {
    VALUE entry = rb_ary_entry(val, i);
    upb_MessageValue msgval;
    if (upb_FieldDef_IsSubMessage(f) && TYPE(entry) == T_HASH) {
      msgval = MessageValue_FromValue(entry, type_info, arena);
    } else {
      msgval = Convert_RubyToUpb(entry, upb_FieldDef_Name(f), type_info, arena);
    }
    upb_Array_Append(arr, msgval, arena);
  }
}

static void Message_InitFieldFromValue(upb_Message* msg, const upb_FieldDef* f,
                                       VALUE val, upb_Arena* arena) {
  if (TYPE(val) == T_NIL) return;

  if (upb_FieldDef_IsMap(f)) {
    upb_Map* map = upb_Message_Mutable(msg, f, arena).map;
    Map_InitFromValue(map, f, val, arena);
  } else if (upb_FieldDef_Label(f) == kUpb_Label_Repeated) {
    upb_Array* arr = upb_Message_Mutable(msg, f, arena).array;
    RepeatedField_InitFromValue(arr, f, val, arena);
  } else if (upb_FieldDef_IsSubMessage(f)) {
    if (TYPE(val) == T_HASH) {
      upb_Message* submsg = upb_Message_Mutable(msg, f, arena).msg;
      Message_InitFromValue(submsg, upb_FieldDef_MessageSubDef(f), val, arena);
    } else {
      Message_setfield(msg, f, val, arena);
    }
  } else {
    upb_MessageValue msgval =
        Convert_RubyToUpb(val, upb_FieldDef_Name(f), TypeInfo_get(f), arena);
    upb_Message_SetFieldByDef(msg, f, msgval, arena);
  }
}

typedef struct {
  upb_Message* msg;
  const upb_MessageDef* msgdef;
  upb_Arena* arena;
} MsgInit;

static int Message_initialize_kwarg(VALUE key, VALUE val, VALUE _self) {
  MsgInit* msg_init = (MsgInit*)_self;
  const char* name;

  if (TYPE(key) == T_STRING) {
    name = RSTRING_PTR(key);
  } else if (TYPE(key) == T_SYMBOL) {
    name = RSTRING_PTR(rb_id2str(SYM2ID(key)));
  } else {
    rb_raise(rb_eArgError,
             "Expected string or symbols as hash keys when initializing proto "
             "from hash.");
  }

  const upb_FieldDef* f =
      upb_MessageDef_FindFieldByName(msg_init->msgdef, name);

  if (f == NULL) {
    rb_raise(rb_eArgError,
             "Unknown field name '%s' in initialization map entry.", name);
  }

  Message_InitFieldFromValue(msg_init->msg, f, val, msg_init->arena);
  return ST_CONTINUE;
}

void Message_InitFromValue(upb_Message* msg, const upb_MessageDef* m, VALUE val,
                           upb_Arena* arena) {
  MsgInit msg_init = {msg, m, arena};
  if (TYPE(val) == T_HASH) {
    rb_hash_foreach(val, Message_initialize_kwarg, (VALUE)&msg_init);
  } else {
    rb_raise(rb_eArgError, "Expected hash arguments or message, not %s",
             rb_class2name(CLASS_OF(val)));
  }
}

/*
 * call-seq:
 *     Message.new(kwargs) => new_message
 *
 * Creates a new instance of the given message class. Keyword arguments may be
 * provided with keywords corresponding to field names.
 *
 * Note that no literal Message class exists. Only concrete classes per message
 * type exist, as provided by the #msgclass method on Descriptors after they
 * have been added to a pool. The method definitions described here on the
 * Message class are provided on each concrete message class.
 */
static VALUE Message_initialize(int argc, VALUE* argv, VALUE _self) {
  Message* self = ruby_to_Message(_self);
  VALUE arena_rb = Arena_new();
  upb_Arena* arena = Arena_get(arena_rb);
  const upb_MiniTable* t = upb_MessageDef_MiniTable(self->msgdef);
  upb_Message* msg = upb_Message_New(t, arena);

  Message_InitPtr(_self, msg, arena_rb);

  if (argc == 0) {
    return Qnil;
  }
  if (argc != 1) {
    rb_raise(rb_eArgError, "Expected 0 or 1 arguments.");
  }
  Message_InitFromValue((upb_Message*)self->msg, self->msgdef, argv[0], arena);
  return Qnil;
}

/*
 * call-seq:
 *     Message.dup => new_message
 *
 * Performs a shallow copy of this message and returns the new copy.
 */
static VALUE Message_dup(VALUE _self) {
  Message* self = ruby_to_Message(_self);
  VALUE new_msg = rb_class_new_instance(0, NULL, CLASS_OF(_self));
  Message* new_msg_self = ruby_to_Message(new_msg);
  size_t size = upb_MessageDef_MiniTable(self->msgdef)->size;

  // TODO
  // TODO
  memcpy((upb_Message*)new_msg_self->msg, self->msg, size);
  Arena_fuse(self->arena, Arena_get(new_msg_self->arena));
  return new_msg;
}

// Support function for Message_eq, and also used by other #eq functions.
bool Message_Equal(const upb_Message* m1, const upb_Message* m2,
                   const upb_MessageDef* m) {
  upb_Status status;
  upb_Status_Clear(&status);
  bool return_value = shared_Message_Equal(m1, m2, m, &status);
  if (upb_Status_IsOk(&status)) {
    return return_value;
  } else {
    rb_raise(cParseError, "Message_Equal(): %s",
             upb_Status_ErrorMessage(&status));
  }
}

/*
 * call-seq:
 *     Message.==(other) => boolean
 *
 * Performs a deep comparison of this message with another. Messages are equal
 * if they have the same type and if each field is equal according to the :==
 * method's semantics (a more efficient comparison may actually be done if the
 * field is of a primitive type).
 */
static VALUE Message_eq(VALUE _self, VALUE _other) {
  if (CLASS_OF(_self) != CLASS_OF(_other)) return Qfalse;

  Message* self = ruby_to_Message(_self);
  Message* other = ruby_to_Message(_other);
  assert(self->msgdef == other->msgdef);

  return Message_Equal(self->msg, other->msg, self->msgdef) ? Qtrue : Qfalse;
}

uint64_t Message_Hash(const upb_Message* msg, const upb_MessageDef* m,
                      uint64_t seed) {
  upb_Status status;
  upb_Status_Clear(&status);
  uint64_t return_value = shared_Message_Hash(msg, m, seed, &status);
  if (upb_Status_IsOk(&status)) {
    return return_value;
  } else {
    rb_raise(cParseError, "Message_Hash(): %s",
             upb_Status_ErrorMessage(&status));
  }
}

/*
 * call-seq:
 *     Message.hash => hash_value
 *
 * Returns a hash value that represents this message's field values.
 */
static VALUE Message_hash(VALUE _self) {
  Message* self = ruby_to_Message(_self);
  uint64_t hash_value = Message_Hash(self->msg, self->msgdef, 0);
  // RUBY_FIXNUM_MAX should be one less than a power of 2.
  assert((RUBY_FIXNUM_MAX & (RUBY_FIXNUM_MAX + 1)) == 0);
  return INT2FIX(hash_value & RUBY_FIXNUM_MAX);
}

/*
 * call-seq:
 *     Message.inspect => string
 *
 * Returns a human-readable string representing this message. It will be
 * formatted as "<MessageType: field1: value1, field2: value2, ...>". Each
 * field's value is represented according to its own #inspect method.
 */
static VALUE Message_inspect(VALUE _self) {
  Message* self = ruby_to_Message(_self);

  StringBuilder* builder = StringBuilder_New();
  Message_PrintMessage(builder, self->msg, self->msgdef);
  VALUE ret = StringBuilder_ToRubyString(builder);
  StringBuilder_Free(builder);
  return ret;
}

// Support functions for Message_to_h //////////////////////////////////////////

static VALUE RepeatedField_CreateArray(const upb_Array* arr,
                                       TypeInfo type_info) {
  int size = arr ? upb_Array_Size(arr) : 0;
  VALUE ary = rb_ary_new2(size);

  for (int i = 0; i < size; i++) {
    upb_MessageValue msgval = upb_Array_Get(arr, i);
    VALUE val = Scalar_CreateHash(msgval, type_info);
    rb_ary_push(ary, val);
  }

  return ary;
}

static VALUE Message_CreateHash(const upb_Message* msg,
                                const upb_MessageDef* m) {
  if (!msg) return Qnil;

  VALUE hash = rb_hash_new();
  int n = upb_MessageDef_FieldCount(m);
  bool is_proto2;

  // We currently have a few behaviors that are specific to proto2.
  // This is unfortunate, we should key behaviors off field attributes (like
  // whether a field has presence), not proto2 vs. proto3. We should see if we
  // can change this without breaking users.
  is_proto2 = upb_MessageDef_Syntax(m) == kUpb_Syntax_Proto2;

  for (int i = 0; i < n; i++) {
    const upb_FieldDef* field = upb_MessageDef_Field(m, i);
    TypeInfo type_info = TypeInfo_get(field);
    upb_MessageValue msgval;
    VALUE msg_value;
    VALUE msg_key;

    if (!is_proto2 && upb_FieldDef_IsSubMessage(field) &&
        !upb_FieldDef_IsRepeated(field) &&
        !upb_Message_HasFieldByDef(msg, field)) {
      // TODO: Legacy behavior, remove when we fix the is_proto2 differences.
      msg_key = ID2SYM(rb_intern(upb_FieldDef_Name(field)));
      rb_hash_aset(hash, msg_key, Qnil);
      continue;
    }

    // Do not include fields that are not present (oneof or optional fields).
    if (is_proto2 && upb_FieldDef_HasPresence(field) &&
        !upb_Message_HasFieldByDef(msg, field)) {
      continue;
    }

    msg_key = ID2SYM(rb_intern(upb_FieldDef_Name(field)));
    msgval = upb_Message_GetFieldByDef(msg, field);

    // Proto2 omits empty map/repeated filds also.

    if (upb_FieldDef_IsMap(field)) {
      const upb_MessageDef* entry_m = upb_FieldDef_MessageSubDef(field);
      const upb_FieldDef* key_f = upb_MessageDef_FindFieldByNumber(entry_m, 1);
      const upb_FieldDef* val_f = upb_MessageDef_FindFieldByNumber(entry_m, 2);
      upb_CType key_type = upb_FieldDef_CType(key_f);
      msg_value = Map_CreateHash(msgval.map_val, key_type, TypeInfo_get(val_f));
    } else if (upb_FieldDef_IsRepeated(field)) {
      if (is_proto2 &&
          (!msgval.array_val || upb_Array_Size(msgval.array_val) == 0)) {
        continue;
      }
      msg_value = RepeatedField_CreateArray(msgval.array_val, type_info);
    } else {
      msg_value = Scalar_CreateHash(msgval, type_info);
    }

    rb_hash_aset(hash, msg_key, msg_value);
  }

  return hash;
}

VALUE Scalar_CreateHash(upb_MessageValue msgval, TypeInfo type_info) {
  if (type_info.type == kUpb_CType_Message) {
    return Message_CreateHash(msgval.msg_val, type_info.def.msgdef);
  } else {
    return Convert_UpbToRuby(msgval, type_info, Qnil);
  }
}

/*
 * call-seq:
 *     Message.to_h => {}
 *
 * Returns the message as a Ruby Hash object, with keys as symbols.
 */
static VALUE Message_to_h(VALUE _self) {
  Message* self = ruby_to_Message(_self);
  return Message_CreateHash(self->msg, self->msgdef);
}

/*
 * call-seq:
 *     Message.freeze => self
 *
 * Freezes the message object. We have to intercept this so we can pin the
 * Ruby object into memory so we don't forget it's frozen.
 */
static VALUE Message_freeze(VALUE _self) {
  Message* self = ruby_to_Message(_self);
  if (!RB_OBJ_FROZEN(_self)) {
    Arena_Pin(self->arena, _self);
    RB_OBJ_FREEZE(_self);
  }
  return _self;
}

/*
 * Deep freezes the message object recursively.
 * Internal use only.
 */
VALUE Message_internal_deep_freeze(VALUE _self) {
  Message* self = ruby_to_Message(_self);
  Message_freeze(_self);

  int n = upb_MessageDef_FieldCount(self->msgdef);
  for (int i = 0; i < n; i++) {
    const upb_FieldDef* f = upb_MessageDef_Field(self->msgdef, i);
    VALUE field = Message_getfield(_self, f);

    if (field != Qnil) {
      if (upb_FieldDef_IsMap(f)) {
        Map_internal_deep_freeze(field);
      } else if (upb_FieldDef_IsRepeated(f)) {
        RepeatedField_internal_deep_freeze(field);
      } else if (upb_FieldDef_IsSubMessage(f)) {
        Message_internal_deep_freeze(field);
      }
    }
  }
  return _self;
}

/*
 * call-seq:
 *     Message.[](index) => value
 *
 * Accesses a field's value by field name. The provided field name should be a
 * string.
 */
static VALUE Message_index(VALUE _self, VALUE field_name) {
  Message* self = ruby_to_Message(_self);
  const upb_FieldDef* field;

  Check_Type(field_name, T_STRING);
  field = upb_MessageDef_FindFieldByName(self->msgdef, RSTRING_PTR(field_name));

  if (field == NULL) {
    return Qnil;
  }

  return Message_getfield(_self, field);
}

/*
 * call-seq:
 *     Message.[]=(index, value)
 *
 * Sets a field's value by field name. The provided field name should be a
 * string.
 */
static VALUE Message_index_set(VALUE _self, VALUE field_name, VALUE value) {
  Message* self = ruby_to_Message(_self);
  const upb_FieldDef* f;
  upb_MessageValue val;
  upb_Arena* arena = Arena_get(self->arena);

  Check_Type(field_name, T_STRING);
  f = upb_MessageDef_FindFieldByName(self->msgdef, RSTRING_PTR(field_name));

  if (f == NULL) {
    rb_raise(rb_eArgError, "Unknown field: %s", RSTRING_PTR(field_name));
  }

  val = Convert_RubyToUpb(value, upb_FieldDef_Name(f), TypeInfo_get(f), arena);
  upb_Message_SetFieldByDef(Message_GetMutable(_self, NULL), f, val, arena);

  return Qnil;
}

/*
 * call-seq:
 *     MessageClass.decode(data, options) => message
 *
 * Decodes the given data (as a string containing bytes in protocol buffers wire
 * format) under the interpretation given by this message class's definition
 * and returns a message object with the corresponding field values.
 * @param options [Hash] options for the decoder
 *  recursion_limit: set to maximum decoding depth for message (default is 64)
 */
static VALUE Message_decode(int argc, VALUE* argv, VALUE klass) {
  VALUE data = argv[0];
  int options = 0;

  if (argc < 1 || argc > 2) {
    rb_raise(rb_eArgError, "Expected 1 or 2 arguments.");
  }

  if (argc == 2) {
    VALUE hash_args = argv[1];
    if (TYPE(hash_args) != T_HASH) {
      rb_raise(rb_eArgError, "Expected hash arguments.");
    }

    VALUE depth =
        rb_hash_lookup(hash_args, ID2SYM(rb_intern("recursion_limit")));

    if (depth != Qnil && TYPE(depth) == T_FIXNUM) {
      options |= upb_DecodeOptions_MaxDepth(FIX2INT(depth));
    }
  }

  if (TYPE(data) != T_STRING) {
    rb_raise(rb_eArgError, "Expected string for binary protobuf data.");
  }

  return Message_decode_bytes(RSTRING_LEN(data), RSTRING_PTR(data), options,
                              klass, /*freeze*/ false);
}

VALUE Message_decode_bytes(int size, const char* bytes, int options,
                           VALUE klass, bool freeze) {
  VALUE msg_rb = initialize_rb_class_with_no_args(klass);
  Message* msg = ruby_to_Message(msg_rb);

  const upb_FileDef* file = upb_MessageDef_File(msg->msgdef);
  const upb_ExtensionRegistry* extreg =
      upb_DefPool_ExtensionRegistry(upb_FileDef_Pool(file));
  upb_DecodeStatus status = upb_Decode(bytes, size, (upb_Message*)msg->msg,
                                       upb_MessageDef_MiniTable(msg->msgdef),
                                       extreg, options, Arena_get(msg->arena));
  if (status != kUpb_DecodeStatus_Ok) {
    rb_raise(cParseError, "Error occurred during parsing");
  }
  if (freeze) {
    Message_internal_deep_freeze(msg_rb);
  }
  return msg_rb;
}

/*
 * call-seq:
 *     MessageClass.decode_json(data, options = {}) => message
 *
 * Decodes the given data (as a string containing bytes in protocol buffers wire
 * format) under the interpretration given by this message class's definition
 * and returns a message object with the corresponding field values.
 *
 *  @param options [Hash] options for the decoder
 *   ignore_unknown_fields: set true to ignore unknown fields (default is to
 *   raise an error)
 */
static VALUE Message_decode_json(int argc, VALUE* argv, VALUE klass) {
  VALUE data = argv[0];
  int options = 0;
  upb_Status status;

  // TODO: use this message's pool instead.
  const upb_DefPool* symtab = DescriptorPool_GetSymtab(generated_pool);

  if (argc < 1 || argc > 2) {
    rb_raise(rb_eArgError, "Expected 1 or 2 arguments.");
  }

  if (argc == 2) {
    VALUE hash_args = argv[1];
    if (TYPE(hash_args) != T_HASH) {
      rb_raise(rb_eArgError, "Expected hash arguments.");
    }

    if (RTEST(rb_hash_lookup2(
            hash_args, ID2SYM(rb_intern("ignore_unknown_fields")), Qfalse))) {
      options |= upb_JsonDecode_IgnoreUnknown;
    }
  }

  if (TYPE(data) != T_STRING) {
    rb_raise(rb_eArgError, "Expected string for JSON data.");
  }

  // TODO: Check and respect string encoding. If not UTF-8, we need to
  // convert, because string handlers pass data directly to message string
  // fields.

  VALUE msg_rb = initialize_rb_class_with_no_args(klass);
  Message* msg = ruby_to_Message(msg_rb);

  // We don't allow users to decode a wrapper type directly.
  if (IsWrapper(msg->msgdef)) {
    rb_raise(rb_eRuntimeError, "Cannot parse a wrapper directly.");
  }

  upb_Status_Clear(&status);
  if (!upb_JsonDecode(RSTRING_PTR(data), RSTRING_LEN(data),
                      (upb_Message*)msg->msg, msg->msgdef, symtab, options,
                      Arena_get(msg->arena), &status)) {
    rb_raise(cParseError, "Error occurred during parsing: %s",
             upb_Status_ErrorMessage(&status));
  }

  return msg_rb;
}

/*
 * call-seq:
 *     MessageClass.encode(msg, options) => bytes
 *
 * Encodes the given message object to its serialized form in protocol buffers
 * wire format.
 * @param options [Hash] options for the encoder
 *  recursion_limit: set to maximum encoding depth for message (default is 64)
 */
static VALUE Message_encode(int argc, VALUE* argv, VALUE klass) {
  Message* msg = ruby_to_Message(argv[0]);
  int options = 0;
  char* data;
  size_t size;

  if (CLASS_OF(argv[0]) != klass) {
    rb_raise(rb_eArgError, "Message of wrong type.");
  }

  if (argc < 1 || argc > 2) {
    rb_raise(rb_eArgError, "Expected 1 or 2 arguments.");
  }

  if (argc == 2) {
    VALUE hash_args = argv[1];
    if (TYPE(hash_args) != T_HASH) {
      rb_raise(rb_eArgError, "Expected hash arguments.");
    }
    VALUE depth =
        rb_hash_lookup(hash_args, ID2SYM(rb_intern("recursion_limit")));

    if (depth != Qnil && TYPE(depth) == T_FIXNUM) {
      options |= upb_DecodeOptions_MaxDepth(FIX2INT(depth));
    }
  }

  upb_Arena* arena = upb_Arena_New();

  upb_EncodeStatus status =
      upb_Encode(msg->msg, upb_MessageDef_MiniTable(msg->msgdef), options,
                 arena, &data, &size);

  if (status == kUpb_EncodeStatus_Ok) {
    VALUE ret = rb_str_new(data, size);
    rb_enc_associate(ret, rb_ascii8bit_encoding());
    upb_Arena_Free(arena);
    return ret;
  } else {
    upb_Arena_Free(arena);
    rb_raise(rb_eRuntimeError, "Exceeded maximum depth (possibly cycle)");
  }
}

/*
 * call-seq:
 *     MessageClass.encode_json(msg, options = {}) => json_string
 *
 * Encodes the given message object into its serialized JSON representation.
 * @param options [Hash] options for the decoder
 *  preserve_proto_fieldnames: set true to use original fieldnames (default is
 * to camelCase) emit_defaults: set true to emit 0/false values (default is to
 * omit them)
 */
static VALUE Message_encode_json(int argc, VALUE* argv, VALUE klass) {
  Message* msg = ruby_to_Message(argv[0]);
  int options = 0;
  char buf[1024];
  size_t size;
  upb_Status status;

  // TODO: use this message's pool instead.
  const upb_DefPool* symtab = DescriptorPool_GetSymtab(generated_pool);

  if (argc < 1 || argc > 2) {
    rb_raise(rb_eArgError, "Expected 1 or 2 arguments.");
  }

  if (argc == 2) {
    VALUE hash_args = argv[1];
    if (TYPE(hash_args) != T_HASH) {
      if (RTEST(rb_funcall(hash_args, rb_intern("respond_to?"), 1,
                           rb_str_new2("to_h")))) {
        hash_args = rb_funcall(hash_args, rb_intern("to_h"), 0);
      } else {
        rb_raise(rb_eArgError, "Expected hash arguments.");
      }
    }

    if (RTEST(rb_hash_lookup2(hash_args,
                              ID2SYM(rb_intern("preserve_proto_fieldnames")),
                              Qfalse))) {
      options |= upb_JsonEncode_UseProtoNames;
    }

    if (RTEST(rb_hash_lookup2(hash_args, ID2SYM(rb_intern("emit_defaults")),
                              Qfalse))) {
      options |= upb_JsonEncode_EmitDefaults;
    }

    if (RTEST(rb_hash_lookup2(hash_args,
                              ID2SYM(rb_intern("format_enums_as_integers")),
                              Qfalse))) {
      options |= upb_JsonEncode_FormatEnumsAsIntegers;
    }
  }

  upb_Status_Clear(&status);
  size = upb_JsonEncode(msg->msg, msg->msgdef, symtab, options, buf,
                        sizeof(buf), &status);

  if (!upb_Status_IsOk(&status)) {
    rb_raise(cParseError, "Error occurred during encoding: %s",
             upb_Status_ErrorMessage(&status));
  }

  VALUE ret;
  if (size >= sizeof(buf)) {
    char* buf2 = malloc(size + 1);
    upb_JsonEncode(msg->msg, msg->msgdef, symtab, options, buf2, size + 1,
                   &status);
    ret = rb_str_new(buf2, size);
    free(buf2);
  } else {
    ret = rb_str_new(buf, size);
  }

  rb_enc_associate(ret, rb_utf8_encoding());
  return ret;
}

/*
 * call-seq:
 *     Message.descriptor => descriptor
 *
 * Class method that returns the Descriptor instance corresponding to this
 * message class's type.
 */
static VALUE Message_descriptor(VALUE klass) {
  return rb_ivar_get(klass, descriptor_instancevar_interned);
}

VALUE build_class_from_descriptor(VALUE descriptor) {
  const char* name;
  VALUE klass;

  name = upb_MessageDef_FullName(Descriptor_GetMsgDef(descriptor));
  if (name == NULL) {
    rb_raise(rb_eRuntimeError, "Descriptor does not have assigned name.");
  }

  klass = rb_define_class_id(
      // Docs say this parameter is ignored. User will assign return value to
      // their own toplevel constant class name.
      rb_intern("Message"), cAbstractMessage);
  rb_ivar_set(klass, descriptor_instancevar_interned, descriptor);
  return klass;
}

/*
 * call-seq:
 *     Enum.lookup(number) => name
 *
 * This module method, provided on each generated enum module, looks up an enum
 * value by number and returns its name as a Ruby symbol, or nil if not found.
 */
static VALUE enum_lookup(VALUE self, VALUE number) {
  int32_t num = NUM2INT(number);
  VALUE desc = rb_ivar_get(self, descriptor_instancevar_interned);
  const upb_EnumDef* e = EnumDescriptor_GetEnumDef(desc);
  const upb_EnumValueDef* ev = upb_EnumDef_FindValueByNumber(e, num);
  if (ev) {
    return ID2SYM(rb_intern(upb_EnumValueDef_Name(ev)));
  } else {
    return Qnil;
  }
}

/*
 * call-seq:
 *     Enum.resolve(name) => number
 *
 * This module method, provided on each generated enum module, looks up an enum
 * value by name (as a Ruby symbol) and returns its name, or nil if not found.
 */
static VALUE enum_resolve(VALUE self, VALUE sym) {
  const char* name = rb_id2name(SYM2ID(sym));
  VALUE desc = rb_ivar_get(self, descriptor_instancevar_interned);
  const upb_EnumDef* e = EnumDescriptor_GetEnumDef(desc);
  const upb_EnumValueDef* ev = upb_EnumDef_FindValueByName(e, name);
  if (ev) {
    return INT2NUM(upb_EnumValueDef_Number(ev));
  } else {
    return Qnil;
  }
}

/*
 * call-seq:
 *     Enum.descriptor
 *
 * This module method, provided on each generated enum module, returns the
 * EnumDescriptor corresponding to this enum type.
 */
static VALUE enum_descriptor(VALUE self) {
  return rb_ivar_get(self, descriptor_instancevar_interned);
}

VALUE build_module_from_enumdesc(VALUE _enumdesc) {
  const upb_EnumDef* e = EnumDescriptor_GetEnumDef(_enumdesc);
  VALUE mod = rb_define_module_id(rb_intern(upb_EnumDef_FullName(e)));

  int n = upb_EnumDef_ValueCount(e);
  for (int i = 0; i < n; i++) {
    const upb_EnumValueDef* ev = upb_EnumDef_Value(e, i);
    upb_Arena* arena = upb_Arena_New();
    const char* src_name = upb_EnumValueDef_Name(ev);
    char* name = upb_strdup2(src_name, strlen(src_name), arena);
    int32_t value = upb_EnumValueDef_Number(ev);
    if (name[0] < 'A' || name[0] > 'Z') {
      if (name[0] >= 'a' && name[0] <= 'z') {
        name[0] -= 32;  // auto capitalize
      } else {
        rb_warn(
            "Enum value '%s' does not start with an uppercase letter "
            "as is required for Ruby constants.",
            name);
      }
    }
    rb_define_const(mod, name, INT2NUM(value));
    upb_Arena_Free(arena);
  }

  rb_define_singleton_method(mod, "lookup", enum_lookup, 1);
  rb_define_singleton_method(mod, "resolve", enum_resolve, 1);
  rb_define_singleton_method(mod, "descriptor", enum_descriptor, 0);
  rb_ivar_set(mod, descriptor_instancevar_interned, _enumdesc);

  return mod;
}

// Internal to the library; used by Google::Protobuf.deep_copy.
upb_Message* Message_deep_copy(const upb_Message* msg, const upb_MessageDef* m,
                               upb_Arena* arena) {
  // Serialize and parse.
  upb_Arena* tmp_arena = upb_Arena_New();
  const upb_MiniTable* layout = upb_MessageDef_MiniTable(m);
  size_t size;

  upb_Message* new_msg = upb_Message_New(layout, arena);
  char* data;

  const upb_FileDef* file = upb_MessageDef_File(m);
  const upb_ExtensionRegistry* extreg =
      upb_DefPool_ExtensionRegistry(upb_FileDef_Pool(file));
  if (upb_Encode(msg, layout, 0, tmp_arena, &data, &size) !=
          kUpb_EncodeStatus_Ok ||
      upb_Decode(data, size, new_msg, layout, extreg, 0, arena) !=
          kUpb_DecodeStatus_Ok) {
    upb_Arena_Free(tmp_arena);
    rb_raise(cParseError, "Error occurred copying proto");
  }

  upb_Arena_Free(tmp_arena);
  return new_msg;
}

const upb_Message* Message_GetUpbMessage(VALUE value, const upb_MessageDef* m,
                                         const char* name, upb_Arena* arena) {
  if (value == Qnil) {
    rb_raise(cTypeError, "nil message not allowed here.");
  }

  VALUE klass = CLASS_OF(value);
  VALUE desc_rb = rb_ivar_get(klass, descriptor_instancevar_interned);
  const upb_MessageDef* val_m =
      desc_rb == Qnil ? NULL : Descriptor_GetMsgDef(desc_rb);

  if (val_m != m) {
    // Check for possible implicit conversions
    // TODO: hash conversion?

    switch (upb_MessageDef_WellKnownType(m)) {
      case kUpb_WellKnown_Timestamp: {
        // Time -> Google::Protobuf::Timestamp
        const upb_MiniTable* t = upb_MessageDef_MiniTable(m);
        upb_Message* msg = upb_Message_New(t, arena);
        upb_MessageValue sec, nsec;
        struct timespec time;
        const upb_FieldDef* sec_f = upb_MessageDef_FindFieldByNumber(m, 1);
        const upb_FieldDef* nsec_f = upb_MessageDef_FindFieldByNumber(m, 2);

        if (!rb_obj_is_kind_of(value, rb_cTime)) goto badtype;

        time = rb_time_timespec(value);
        sec.int64_val = time.tv_sec;
        nsec.int32_val = time.tv_nsec;
        upb_Message_SetFieldByDef(msg, sec_f, sec, arena);
        upb_Message_SetFieldByDef(msg, nsec_f, nsec, arena);
        return msg;
      }
      case kUpb_WellKnown_Duration: {
        // Numeric -> Google::Protobuf::Duration
        const upb_MiniTable* t = upb_MessageDef_MiniTable(m);
        upb_Message* msg = upb_Message_New(t, arena);
        upb_MessageValue sec, nsec;
        const upb_FieldDef* sec_f = upb_MessageDef_FindFieldByNumber(m, 1);
        const upb_FieldDef* nsec_f = upb_MessageDef_FindFieldByNumber(m, 2);

        if (!rb_obj_is_kind_of(value, rb_cNumeric)) goto badtype;

        sec.int64_val = NUM2LL(value);
        nsec.int32_val = round((NUM2DBL(value) - NUM2LL(value)) * 1000000000);
        upb_Message_SetFieldByDef(msg, sec_f, sec, arena);
        upb_Message_SetFieldByDef(msg, nsec_f, nsec, arena);
        return msg;
      }
      default:
      badtype:
        rb_raise(cTypeError,
                 "Invalid type %s to assign to submessage field '%s'.",
                 rb_class2name(CLASS_OF(value)), name);
    }
  }

  Message* self = ruby_to_Message(value);
  Arena_fuse(self->arena, arena);

  return self->msg;
}

static void Message_define_class(VALUE klass) {
  rb_define_alloc_func(klass, Message_alloc);

  rb_require("google/protobuf/message_exts");
  rb_define_method(klass, "method_missing", Message_method_missing, -1);
  rb_define_method(klass, "respond_to_missing?", Message_respond_to_missing,
                   -1);
  rb_define_method(klass, "initialize", Message_initialize, -1);
  rb_define_method(klass, "dup", Message_dup, 0);
  // Also define #clone so that we don't inherit Object#clone.
  rb_define_method(klass, "clone", Message_dup, 0);
  rb_define_method(klass, "==", Message_eq, 1);
  rb_define_method(klass, "eql?", Message_eq, 1);
  rb_define_method(klass, "freeze", Message_freeze, 0);
  rb_define_method(klass, "hash", Message_hash, 0);
  rb_define_method(klass, "to_h", Message_to_h, 0);
  rb_define_method(klass, "inspect", Message_inspect, 0);
  rb_define_method(klass, "to_s", Message_inspect, 0);
  rb_define_method(klass, "[]", Message_index, 1);
  rb_define_method(klass, "[]=", Message_index_set, 2);
  rb_define_singleton_method(klass, "decode", Message_decode, -1);
  rb_define_singleton_method(klass, "encode", Message_encode, -1);
  rb_define_singleton_method(klass, "decode_json", Message_decode_json, -1);
  rb_define_singleton_method(klass, "encode_json", Message_encode_json, -1);
  rb_define_singleton_method(klass, "descriptor", Message_descriptor, 0);
}

void Message_register(VALUE protobuf) {
  cParseError = rb_const_get(protobuf, rb_intern("ParseError"));
  cAbstractMessage =
      rb_define_class_under(protobuf, "AbstractMessage", rb_cObject);
  Message_define_class(cAbstractMessage);
  rb_gc_register_address(&cAbstractMessage);

  // Ruby-interned string: "descriptor". We use this identifier to store an
  // instance variable on message classes we create in order to link them back
  // to their descriptors.
  descriptor_instancevar_interned = rb_intern("@descriptor");
}
