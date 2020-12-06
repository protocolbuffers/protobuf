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

#include "convert.h"
#include "map.h"
#include "repeated_field.h"
#include "protobuf.h"
#include "third_party/wyhash/wyhash.h"

static VALUE initialize_rb_class_with_no_args(VALUE klass) {
    return rb_funcall(klass, rb_intern("new"), 0);
}

// -----------------------------------------------------------------------------
// Class/module creation from msgdefs and enumdefs, respectively.
// -----------------------------------------------------------------------------

void Message_mark(void* _self) {
  Message* self = (Message *)_self;
  rb_gc_mark(self->arena);
}

rb_data_type_t Message_type = {
  "Message",
  { Message_mark, NULL, NULL },
};

VALUE Message_alloc(VALUE klass) {
  VALUE descriptor = rb_ivar_get(klass, descriptor_instancevar_interned);
  Descriptor* desc = ruby_to_Descriptor(descriptor);
  Message* msg = ALLOC(Message);
  VALUE ret;

  msg->descriptor = desc;
  msg->arena = Arena_new();

  ret = TypedData_Wrap_Struct(klass, &Message_type, msg);
  rb_ivar_set(ret, descriptor_instancevar_interned, descriptor);

  return ret;
}

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
bool is_wrapper_type_field(const upb_fielddef* field) {
  const upb_msgdef *m;
  if (upb_fielddef_type(field) != UPB_TYPE_MESSAGE) {
    return false;
  }
  m = upb_fielddef_msgsubdef(field);
  switch (upb_msgdef_wellknowntype(m)) {
    case UPB_WELLKNOWN_DOUBLEVALUE:
    case UPB_WELLKNOWN_FLOATVALUE:
    case UPB_WELLKNOWN_INT64VALUE:
    case UPB_WELLKNOWN_UINT64VALUE:
    case UPB_WELLKNOWN_INT32VALUE:
    case UPB_WELLKNOWN_UINT32VALUE:
    case UPB_WELLKNOWN_STRINGVALUE:
    case UPB_WELLKNOWN_BYTESVALUE:
    case UPB_WELLKNOWN_BOOLVALUE:
      return true;
    default:
      return false;
  }
}

// Get a new Ruby wrapper type and set the initial value
VALUE ruby_wrapper_type(VALUE type_class, VALUE value) {
  if (value != Qnil) {
    VALUE hash = rb_hash_new();
    rb_hash_aset(hash, rb_str_new2("value"), value);
    {
      VALUE args[1] = {hash};
      return rb_class_new_instance(1, args, type_class);
    }
  }
  return Qnil;
}

void validate_type_class(upb_fieldtype_t type, VALUE klass) {
  if (rb_ivar_get(klass, descriptor_instancevar_interned) == Qnil) {
    rb_raise(rb_eArgError,
             "Type class has no descriptor. Please pass a "
             "class or enum as returned by the DescriptorPool.");
  }
  if (type == UPB_TYPE_MESSAGE) {
    VALUE desc = rb_ivar_get(klass, descriptor_instancevar_interned);
    if (!RB_TYPE_P(desc, T_DATA) || !RTYPEDDATA_P(desc) ||
        RTYPEDDATA_TYPE(desc) != &_Descriptor_type) {
      rb_raise(rb_eArgError, "Descriptor has an incorrect type.");
    }
    if (rb_get_alloc_func(klass) != &Message_alloc) {
      rb_raise(rb_eArgError,
               "Message class was not returned by the DescriptorPool.");
    }
  } else if (type == UPB_TYPE_ENUM) {
    VALUE enumdesc = rb_ivar_get(klass, descriptor_instancevar_interned);
    if (!RB_TYPE_P(enumdesc, T_DATA) || !RTYPEDDATA_P(enumdesc) ||
        RTYPEDDATA_TYPE(enumdesc) != &_EnumDescriptor_type) {
      rb_raise(rb_eArgError, "Descriptor has an incorrect type.");
    }
  }
}

static bool match(const upb_msgdef* m, const char* name, const upb_fielddef** f,
                  const upb_oneofdef** o, const char* prefix,
                  const char* suffix) {
  size_t sp = strlen(prefix);
  size_t ss = strlen(suffix);
  size_t sn = strlen(name);

  if (sn <= sp + ss) return false;

  if (memcmp(name, prefix, sp) != 0 ||
      memcmp(name + sn - ss, suffix, ss) != 0) {
    return false;
  }

  return upb_msgdef_lookupname(m, name + sp, sn - sp - ss, f, o);
}

static int extract_method_call(VALUE method_name, Message* self,
                               const upb_fielddef** f, const upb_oneofdef** o) {
  const upb_msgdef* m = self->descriptor->msgdef;
  char* name;

  Check_Type(method_name, T_SYMBOL);
  name = RSTRING_PTR(method_name);

  if (match(m, name, f, o, "", "")) return METHOD_GETTER;
  if (match(m, name, f, o, "", "=")) return METHOD_SETTER;
  if (match(m, name, f, o, "clear_", "")) return METHOD_CLEAR;
  if (match(m, name, f, o, "has_", "?")) return METHOD_PRESENCE;
  if (match(m, name, f, o, "", "_as_value")) return METHOD_WRAPPER_GETTER;
  if (match(m, name, f, o, "", "_as_value=")) return METHOD_WRAPPER_SETTER;
  if (match(m, name, f, o, "", "_const")) return METHOD_ENUM_GETTER;

  return METHOD_UNKNOWN;
}

upb_msg* Message_GetUpbMessage(VALUE value, const Descriptor* desc,
                               const char* name, upb_arena* arena) {
  Message* self;

  if (value == Qnil) return NULL;

  if (CLASS_OF(value) != desc->klass) {
    // Check for possible implicit conversions
    // TODO: hash conversion?

    switch (upb_msgdef_wellknowntype(desc->msgdef)) {
      case UPB_WELLKNOWN_TIMESTAMP: {
        // Time -> Google::Protobuf::Timestamp
        upb_msg *msg = upb_msg_new(desc->msgdef, arena);
        upb_msgval sec, nsec;
        struct timespec time;
        const upb_fielddef *sec_f = upb_msgdef_itof(desc->msgdef, 1);
        const upb_fielddef *nsec_f = upb_msgdef_itof(desc->msgdef, 2);

        if (!rb_obj_is_kind_of(value, rb_cTime)) goto badtype;

        time = rb_time_timespec(value);
        sec.int64_val = time.tv_sec;
        nsec.int32_val = time.tv_nsec;
        upb_msg_set(msg, sec_f, sec, arena);
        upb_msg_set(msg, nsec_f, nsec, arena);
        return msg;
      }
      case UPB_WELLKNOWN_DURATION: {
        // Numeric -> Google::Protobuf::Duration
        upb_msg *msg = upb_msg_new(desc->msgdef, arena);
        upb_msgval sec, nsec;
        const upb_fielddef *sec_f = upb_msgdef_itof(desc->msgdef, 1);
        const upb_fielddef *nsec_f = upb_msgdef_itof(desc->msgdef, 2);

        if (!rb_obj_is_kind_of(value, rb_cNumeric)) goto badtype;

        sec.int64_val = NUM2LL(value);
        nsec.int32_val = (NUM2DBL(value) - NUM2LL(value)) * 1000000000;
        upb_msg_set(msg, sec_f, sec, arena);
        upb_msg_set(msg, nsec_f, nsec, arena);
        return msg;
      }
      default:
      badtype:
        rb_raise(cTypeError,
                 "Invalid type %s to assign to submessage field '%s'.",
                rb_class2name(CLASS_OF(value)), name);
    }

  }

  TypedData_Get_Struct(value, Message, &Message_type, self);
  upb_arena_fuse(arena, Arena_get(self->arena));

  return self->msg;
}

static VALUE Message_oneof_accessor(Message* self, const upb_oneofdef* o,
                                    int accessor_type) {
  const upb_fielddef* oneof_field = upb_msg_whichoneof(self->msg, o);

  switch (accessor_type) {
    case METHOD_PRESENCE:
      return oneof_field == NULL ? Qfalse : Qtrue;
    case METHOD_CLEAR:
      if (oneof_field != NULL) {
        upb_msg_clearfield(self->msg, oneof_field);
      }
      return Qnil;
    case METHOD_GETTER:
      return oneof_field == NULL
                 ? Qnil
                 : ID2SYM(rb_intern(upb_fielddef_name(oneof_field)));
    case METHOD_SETTER:
      rb_raise(rb_eRuntimeError, "Oneof accessors are read-only.");
  }
  rb_raise(rb_eRuntimeError, "Invalid access of oneof field.");
}

static VALUE Message_field_accessor(Message* self, const upb_fielddef* f,
                                    int accessor_type, int argc, VALUE* argv) {
  upb_arena *arena = Arena_get(self->arena);

  switch (accessor_type) {
    case METHOD_SETTER: {
      upb_msgval msgval = Convert_RubyToUpb(argv[1], upb_fielddef_name(f),
                                            TypeInfo_get(f), arena);
      upb_msg_set(self->msg, f, msgval, arena);
      return Qnil;
    }
    case METHOD_CLEAR:
      upb_msg_clearfield(self->msg, f);
      return Qnil;
    case METHOD_PRESENCE:
      return upb_msg_has(self->msg, f);
    case METHOD_WRAPPER_GETTER: {
      upb_msgval msgval = upb_msg_get(self->msg, f);
      VALUE value = Convert_UpbToRuby(msgval, TypeInfo_get(f), self->arena);
      switch (TYPE(value)) {
        case T_DATA:
          return rb_funcall(value, rb_intern("value"), 0);
        case T_NIL:
          return Qnil;
        default:
          return value;
      }
    }
    case METHOD_WRAPPER_SETTER: {
      VALUE wrapper = ruby_wrapper_type(self->descriptor->klass, argv[1]);
      upb_msgval msgval = Convert_RubyToUpb(wrapper, upb_fielddef_name(f),
                                            TypeInfo_get(f), arena);
      upb_msg_set(self->msg, f, msgval, arena);
      return Qnil;
    }
    case METHOD_ENUM_GETTER: {
      upb_msgval msgval = upb_msg_get(self->msg, f);

      if (upb_fielddef_label(f) == UPB_LABEL_REPEATED) {
        // Map repeated fields to a new type with ints
        VALUE arr = rb_ary_new();
        size_t i, n = upb_array_size(msgval.array_val);
        for (i = 0; i < n; i++) {
          upb_msgval elem = upb_array_get(msgval.array_val, i);
          rb_ary_push(arr, INT2NUM(elem.int32_val));
        }
        return arr;
      } else {
        return INT2NUM(msgval.int32_val);
      }
    }
    case METHOD_GETTER:
      return Convert_UpbToRuby(upb_msg_get(self->msg, f), TypeInfo_get(f),
                               self->arena);
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
VALUE Message_method_missing(int argc, VALUE* argv, VALUE _self) {
  Message* self;
  const upb_oneofdef* o;
  const upb_fielddef* f;
  int accessor_type;

  TypedData_Get_Struct(_self, Message, &Message_type, self);
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
    return Message_oneof_accessor(self, o, accessor_type);
  } else {
    return Message_field_accessor(self, f, accessor_type, argc, argv);
  }
}

VALUE Message_respond_to_missing(int argc, VALUE* argv, VALUE _self) {
  Message* self;
  const upb_oneofdef* o;
  const upb_fielddef* f;
  int accessor_type;

  TypedData_Get_Struct(_self, Message, &Message_type, self);
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

void Message_InitFromValue(upb_msg* msg, const upb_msgdef* m, VALUE val,
                           upb_arena* arena);

void Map_InitFromValue(upb_map* map, upb_fieldtype_t key_type,
                       TypeInfo val_type, VALUE val, upb_arena* arena);

upb_msgval MessageValue_FromValue(VALUE val, TypeInfo info, upb_arena *arena) {
  if (info.type == UPB_TYPE_MESSAGE) {
    upb_msgval msgval;
    upb_msg *msg = upb_msg_new(info.def.msgdef->msgdef, arena);
    Message_InitFromValue(msg, info.def.msgdef->msgdef, val, arena);
    msgval.msg_val = msg;
    return msgval;
  } else {
    return Convert_RubyToUpb(val, "", info, arena);
  }
}

void Array_InitFromValue(upb_array* arr, const upb_fielddef* f, VALUE val,
                         upb_arena* arena) {
  TypeInfo type_info = TypeInfo_get(f);

  if (TYPE(val) != T_ARRAY) {
    rb_raise(rb_eArgError,
             "Expected array as initializer value for repeated field '%s' (given %s).",
             upb_fielddef_name(f), rb_class2name(CLASS_OF(val)));
  }

  for (int i = 0; i < RARRAY_LEN(val); i++) {
    VALUE entry = rb_ary_entry(val, i);
    upb_msgval msgval = MessageValue_FromValue(entry, type_info, arena);
    upb_array_append(arr, msgval, arena);
  }
}

void Message_InitFieldFromValue(upb_msg* msg, const upb_fielddef* f, VALUE val,
                                upb_arena* arena) {
  if (TYPE(val) == T_NIL) return;

  if (upb_fielddef_ismap(f)) {
    upb_map *map = upb_msg_mutable(msg, f, arena).map;
    const upb_msgdef *entry_m = upb_fielddef_msgsubdef(f);
    const upb_fielddef *key_f = upb_msgdef_itof(entry_m, 1);
    const upb_fielddef *val_f = upb_msgdef_itof(entry_m, 2);
    upb_fieldtype_t key_type = upb_fielddef_type(key_f);
    Map_InitFromValue(map, key_type, TypeInfo_get(val_f), val, arena);
  } else if (upb_fielddef_label(f) == UPB_LABEL_REPEATED) {
    upb_array *arr = upb_msg_mutable(msg, f, arena).array;
    Array_InitFromValue(arr, f, val, arena);
  } else if (upb_fielddef_issubmsg(f)) {
    upb_msg *submsg = upb_msg_mutable(msg, f, arena).msg;
    Message_InitFromValue(submsg, upb_fielddef_msgsubdef(f), val, arena);
  } else {
    upb_msgval msgval =
        Convert_RubyToUpb(val, upb_fielddef_name(f), TypeInfo_get(f), arena);
    upb_msg_set(msg, f, msgval, arena);
  }
}

typedef struct {
  upb_msg *msg;
  const upb_msgdef *msgdef;
  upb_arena *arena;
} MsgInit;

int Message_initialize_kwarg(VALUE key, VALUE val, VALUE _self) {
  MsgInit *msg_init = (MsgInit*)_self;
  const char *name;

  if (TYPE(key) == T_STRING) {
    name = RSTRING_PTR(key);
  } else if (TYPE(key) == T_SYMBOL) {
    name = RSTRING_PTR(rb_id2str(SYM2ID(key)));
  } else {
    rb_raise(rb_eArgError,
             "Expected string or symbols as hash keys when initializing proto from hash.");
  }

  const upb_fielddef* f = upb_msgdef_ntofz(msg_init->msgdef, name);

  if (f == NULL) {
    rb_raise(rb_eArgError,
             "Unknown field name '%s' in initialization map entry.", name);
  }

  Message_InitFromValue(msg_init->msg, upb_fielddef_msgsubdef(f), val,
                        msg_init->arena);
  return ST_CONTINUE;
}

void Message_InitFromValue(upb_msg* msg, const upb_msgdef* m, VALUE val,
                           upb_arena* arena) {
  MsgInit msg_init = {msg, m, arena};
  if (TYPE(val) != T_HASH) {
    rb_raise(rb_eArgError, "Expected hash arguments.");
  }

  rb_hash_foreach(val, Message_initialize_kwarg, (VALUE)&msg_init);
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
VALUE Message_initialize(int argc, VALUE* argv, VALUE _self) {
  Message* self;
  TypedData_Get_Struct(_self, Message, &Message_type, self);
  upb_arena *arena = Arena_get(self->arena);

  self->msg = upb_msg_new(self->descriptor->msgdef, arena);

  if (argc == 0) {
    return Qnil;
  }
  if (argc != 1) {
    rb_raise(rb_eArgError, "Expected 0 or 1 arguments.");
  }
  Message_InitFromValue(self->msg, self->descriptor->msgdef, argv[0], arena);
  return Qnil;
}

/*
 * call-seq:
 *     Message.dup => new_message
 *
 * Performs a shallow copy of this message and returns the new copy.
 */
VALUE Message_dup(VALUE _self) {
  Message* self;
  VALUE new_msg;
  Message* new_msg_self;
  TypedData_Get_Struct(_self, Message, &Message_type, self);
  size_t size = upb_msgdef_layout(self->descriptor->msgdef)->size;

  new_msg = rb_class_new_instance(0, NULL, CLASS_OF(_self));
  TypedData_Get_Struct(new_msg, Message, &Message_type, new_msg_self);

  // TODO(copy unknown fields?)
  // TODO(use official function)
  memcpy(new_msg_self->msg, self->msg, size);
  upb_arena_fuse(Arena_get(new_msg_self->arena), Arena_get(self->arena));
  return new_msg;
}

// Internal only; used by Google::Protobuf.deep_copy.
static VALUE Message_deep_copy(VALUE _self) {
  Message* self;
  Message* new_msg_self;
  VALUE new_msg;
  TypedData_Get_Struct(_self, Message, &Message_type, self);

  new_msg = rb_class_new_instance(0, NULL, CLASS_OF(_self));
  TypedData_Get_Struct(new_msg, Message, &Message_type, new_msg_self);

  const char *data;
  size_t size;

  // Serialize and parse.
  upb_arena *arena = upb_arena_new();
  data = upb_encode_ex(self->msg, upb_msgdef_layout(self->descriptor->msgdef),
                       0, arena, &size);
  if (!data || !upb_decode(data, size, new_msg_self->msg,
                           upb_msgdef_layout(new_msg_self->descriptor->msgdef),
                           Arena_get(new_msg_self->arena))) {
    upb_arena_free(arena);
    rb_raise(cParseError, "Error occurred copying proto");
  }

  upb_arena_free(arena);
  return new_msg;
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
VALUE Message_eq(VALUE _self, VALUE _other) {
  Message* self;
  Message* other;
  const char *data1, *data2;
  size_t size1, size2;

  if (TYPE(_self) != TYPE(_other)) {
    return Qfalse;
  }

  TypedData_Get_Struct(_self, Message, &Message_type, self);
  TypedData_Get_Struct(_other, Message, &Message_type, other);

  // Compare deterministically serialized payloads with no unknown fields.
  upb_arena *arena = upb_arena_new();
  data1 = upb_encode_ex(self->msg, upb_msgdef_layout(self->descriptor->msgdef),
                        UPB_ENCODE_SKIPUNKNOWN | UPB_ENCODE_DETERMINISTIC,
                        arena, &size1);
  data2 = upb_encode_ex(
      other->msg, upb_msgdef_layout(other->descriptor->msgdef),
      UPB_ENCODE_SKIPUNKNOWN | UPB_ENCODE_DETERMINISTIC, arena, &size2);

  if (data1 && data2) {
    bool ret = size1 == size2 && memcmp(data1, data2, size1) == 0;
    upb_arena_free(arena);
    return ret ? Qtrue : Qfalse;
  } else {
    upb_arena_free(arena);
    rb_raise(cParseError, "Error comparing messages");
  }
}

/*
 * call-seq:
 *     Message.hash => hash_value
 *
 * Returns a hash value that represents this message's field values.
 */
VALUE Message_hash(VALUE _self) {
  Message* self;
  upb_arena *arena = upb_arena_new();
  const char *data;
  size_t size;

  TypedData_Get_Struct(_self, Message, &Message_type, self);

  // Hash a deterministically serialized payloads with no unknown fields.
  data = upb_encode_ex(self->msg, upb_msgdef_layout(self->descriptor->msgdef),
                       UPB_ENCODE_SKIPUNKNOWN | UPB_ENCODE_DETERMINISTIC, arena,
                       &size);

  if (data) {
    uint32_t ret = wyhash(data, size, 0, _wyp);
    upb_arena_free(arena);
    return INT2FIX(ret);
  } else {
    upb_arena_free(arena);
    rb_raise(cParseError, "Error calculating hash");
  }
}

/*
 * call-seq:
 *     Message.inspect => string
 *
 * Returns a human-readable string representing this message. It will be
 * formatted as "<MessageType: field1: value1, field2: value2, ...>". Each
 * field's value is represented according to its own #inspect method.
 */
VALUE Message_inspect(VALUE _self) {
  Message* self;
  VALUE str = rb_str_new2("");
  bool first = true;
  TypedData_Get_Struct(_self, Message, &Message_type, self);
  const upb_msgdef *m = self->descriptor->msgdef;
  int n = upb_msgdef_fieldcount(m);

  str = rb_str_new2("<");
  str = rb_str_append(str, rb_str_new2(rb_class2name(CLASS_OF(_self))));
  str = rb_str_cat2(str, ": ");

  for (int i = 0; i < n; i++) {
    const upb_fielddef* field = upb_msgdef_field(m, i);
    // OPT: this would be more efficient if we didn't create Ruby objects for
    // the whole tree. But to maintain backward-compatibility, we would need to
    // make sure we are printing all values in the same way that #inspect
    // would.
    VALUE field_val = Convert_UpbToRuby(upb_msg_get(self->msg, field),
                                        TypeInfo_get(field), self->arena);

    if (!first) {
      str = rb_str_cat2(str, ", ");
    } else {
      first = false;
    }
    str = rb_str_cat2(str, upb_fielddef_name(field));
    str = rb_str_cat2(str, ": ");

    str = rb_str_append(str, rb_funcall(field_val, rb_intern("inspect"), 0));
  }

  str = rb_str_cat2(str, ">");
  return str;
}

static VALUE Scalar_CreateHash(upb_msgval val, TypeInfo type_info);

static VALUE RepeatedField_CreateArray(const upb_array* arr,
                                       TypeInfo type_info) {
  int size = upb_array_size(arr);
  VALUE ary = rb_ary_new2(size);

  for (int i = 0; i < size; i++) {
    upb_msgval msgval = upb_array_get(arr, i);
    VALUE val = Scalar_CreateHash(msgval, type_info);
    rb_ary_push(ary, val);
  }

  return ary;
}

static VALUE Map_CreateHash(const upb_map* map, upb_fieldtype_t key_type,
                            TypeInfo val_info) {
  VALUE hash = rb_hash_new();
  size_t iter = UPB_MAP_BEGIN;
  TypeInfo key_info = TypeInfo_from_type(key_type);

  while (upb_mapiter_next(map, &iter)) {
    upb_msgval key = upb_mapiter_key(map, iter);
    upb_msgval val = upb_mapiter_value(map, iter);
    VALUE key_val = Convert_UpbToRuby(key, key_info, Qnil);
    VALUE val_val = Scalar_CreateHash(val, val_info);
    rb_hash_aset(hash, key_val, val_val);
  }

  return hash;
}

static VALUE Message_CreateHash(const upb_msg *msg, const upb_msgdef *m) {
  VALUE hash = rb_hash_new();
  int n = upb_msgdef_fieldcount(m);
  bool is_proto2;

  // We currently have a few behaviors that are specific to proto2.
  // This is unfortunate, we should key behaviors off field attributes (like
  // whether a field has presence), not proto2 vs. proto3. We should see if we
  // can change this without breaking users.
  is_proto2 = upb_msgdef_syntax(m) == UPB_SYNTAX_PROTO2;

  for (int i = 0; i < n; i++) {
    const upb_fielddef* field = upb_msgdef_field(m, i);
    TypeInfo type_info = TypeInfo_get(field);
    upb_msgval msgval;
    VALUE msg_value;
    VALUE msg_key;

    // Do not include fields that are not present (oneof or optional fields).
    if (is_proto2 && upb_fielddef_haspresence(field) &&
        !upb_msg_has(msg, field)) {
      continue;
    }

    msg_key = ID2SYM(rb_intern(upb_fielddef_name(field)));
    msgval = upb_msg_get(msg, field);

    if (upb_fielddef_ismap(field)) {
      const upb_msgdef *entry_m = upb_fielddef_msgsubdef(field);
      const upb_fielddef *key_f = upb_msgdef_itof(entry_m, 1);
      const upb_fielddef *val_f = upb_msgdef_itof(entry_m, 2);
      upb_fieldtype_t key_type = upb_fielddef_type(key_f);
      msg_value = Map_CreateHash(msgval.map_val, key_type, TypeInfo_get(val_f));
    } else if (upb_fielddef_isseq(field)) {
      if (is_proto2 && upb_array_size(msgval.array_val) == 0) {
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

static VALUE Scalar_CreateHash(upb_msgval msgval, TypeInfo type_info) {
  if (type_info.type == UPB_TYPE_MESSAGE) {
    return Message_CreateHash(msgval.msg_val, type_info.def.msgdef->msgdef);
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
  Message* self;
  TypedData_Get_Struct(_self, Message, &Message_type, self);

  return Message_CreateHash(self->msg, self->descriptor->msgdef);
}

/*
 * call-seq:
 *     Message.[](index) => value
 *
 * Accesses a field's value by field name. The provided field name should be a
 * string.
 */
VALUE Message_index(VALUE _self, VALUE field_name) {
  Message* self;
  const upb_fielddef* field;
  upb_msgval val;

  TypedData_Get_Struct(_self, Message, &Message_type, self);
  Check_Type(field_name, T_STRING);
  field = upb_msgdef_ntofz(self->descriptor->msgdef, RSTRING_PTR(field_name));

  if (field == NULL) {
    return Qnil;
  }

  val = upb_msg_get(self->msg, field);
  return Convert_UpbToRuby(val, TypeInfo_get(field), self->arena);
}

/*
 * call-seq:
 *     Message.[]=(index, value)
 *
 * Sets a field's value by field name. The provided field name should be a
 * string.
 */
VALUE Message_index_set(VALUE _self, VALUE field_name, VALUE value) {
  Message* self;
  const upb_fielddef* f;
  upb_msgval val;
  upb_arena *arena;

  TypedData_Get_Struct(_self, Message, &Message_type, self);
  Check_Type(field_name, T_STRING);

  arena = Arena_get(self->arena);
  f = upb_msgdef_ntofz(self->descriptor->msgdef, RSTRING_PTR(field_name));

  if (f == NULL) {
    rb_raise(rb_eArgError, "Unknown field: %s", RSTRING_PTR(field_name));
  }

  val = Convert_RubyToUpb(value, upb_fielddef_name(f), TypeInfo_get(f), arena);
  upb_msg_set(self->msg, f, val, arena);

  return Qnil;
}

/*
 * call-seq:
 *     MessageClass.decode(data) => message
 *
 * Decodes the given data (as a string containing bytes in protocol buffers wire
 * format) under the interpretration given by this message class's definition
 * and returns a message object with the corresponding field values.
 */
VALUE Message_decode(VALUE klass, VALUE data) {
  Message* msg;
  VALUE msg_rb;

  if (TYPE(data) != T_STRING) {
    rb_raise(rb_eArgError, "Expected string for binary protobuf data.");
  }

  msg_rb = initialize_rb_class_with_no_args(klass);
  TypedData_Get_Struct(msg_rb, Message, &Message_type, msg);

  if (!upb_decode(RSTRING_PTR(data), RSTRING_LEN(data), msg->msg,
                 upb_msgdef_layout(msg->descriptor->msgdef),
                 Arena_get(msg->arena))) {
    rb_raise(cParseError, "Error occurred during parsing");
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
VALUE Message_decode_json(int argc, VALUE* argv, VALUE klass) {
  Message* msg;
  VALUE msg_rb;
  VALUE data = argv[0];
  int options = 0;
  upb_status status;

  // TODO(haberman): use this message's pool instead.
  DescriptorPool* pool = ruby_to_DescriptorPool(generated_pool);

  if (argc < 1 || argc > 2) {
    rb_raise(rb_eArgError, "Expected 1 or 2 arguments.");
  }

  if (argc == 2) {
    VALUE hash_args = argv[1];
    if (TYPE(hash_args) != T_HASH) {
      rb_raise(rb_eArgError, "Expected hash arguments.");
    }

    if (RTEST(rb_hash_lookup2( hash_args, ID2SYM(rb_intern("ignore_unknown_fields")), Qfalse))) {
      options |= UPB_JSONDEC_IGNOREUNKNOWN;
    }
  }

  if (TYPE(data) != T_STRING) {
    rb_raise(rb_eArgError, "Expected string for JSON data.");
  }

  // TODO(cfallin): Check and respect string encoding. If not UTF-8, we need to
  // convert, because string handlers pass data directly to message string
  // fields.

  msg_rb = initialize_rb_class_with_no_args(klass);
  TypedData_Get_Struct(msg_rb, Message, &Message_type, msg);

  upb_status_clear(&status);
  if (!upb_json_decode(RSTRING_PTR(data), RSTRING_LEN(data), msg->msg,
                       msg->descriptor->msgdef, pool->symtab,
                       options, Arena_get(msg->arena), &status)) {
    rb_raise(cParseError, "Error occurred during parsing: %s",
             upb_status_errmsg(&status));
  }

  return msg_rb;
}

/*
 * call-seq:
 *     MessageClass.encode(msg) => bytes
 *
 * Encodes the given message object to its serialized form in protocol buffers
 * wire format.
 */
VALUE Message_encode(VALUE klass, VALUE msg_rb) {
  Message* msg;
  upb_arena *arena = upb_arena_new();
  const char *data;
  size_t size;

  TypedData_Get_Struct(msg_rb, Message, &Message_type, msg);

  data = upb_encode(msg->msg, upb_msgdef_layout(msg->descriptor->msgdef), arena,
                    &size);

  if (data) {
    VALUE ret = rb_str_new(data, size);
    upb_arena_free(arena);
    return ret;
  } else {
    upb_arena_free(arena);
    rb_raise(cParseError, "Error occurred during encoding");
  }
}

/*
 * call-seq:
 *     MessageClass.encode_json(msg, options = {}) => json_string
 *
 * Encodes the given message object into its serialized JSON representation.
 * @param options [Hash] options for the decoder
 *  preserve_proto_fieldnames: set true to use original fieldnames (default is to camelCase)
 *  emit_defaults: set true to emit 0/false values (default is to omit them)
 */
VALUE Message_encode_json(int argc, VALUE* argv, VALUE klass) {
  VALUE msg_rb;
  Message* msg;
  int options = 0;
  char buf[1024];
  size_t size;
  upb_status status;

  // TODO(haberman): use this message's pool instead.
  DescriptorPool* pool = ruby_to_DescriptorPool(generated_pool);

  if (argc < 1 || argc > 2) {
    rb_raise(rb_eArgError, "Expected 1 or 2 arguments.");
  }

  msg_rb = argv[0];
  TypedData_Get_Struct(msg_rb, Message, &Message_type, msg);

  if (argc == 2) {
    VALUE hash_args = argv[1];
    if (TYPE(hash_args) != T_HASH) {
      rb_raise(rb_eArgError, "Expected hash arguments.");
    }

    if (RTEST(rb_hash_lookup2(hash_args,
                              ID2SYM(rb_intern("preserve_proto_fieldnames")),
                              Qfalse))) {
      options |= UPB_JSONENC_PROTONAMES;
    }

    if (RTEST(rb_hash_lookup2(hash_args, ID2SYM(rb_intern("emit_defaults")),
                              Qfalse))) {
      options |= UPB_JSONENC_EMITDEFAULTS;
    }
  }

  upb_status_clear(&status);
  size = upb_json_encode(msg->msg, msg->descriptor->msgdef, pool->symtab,
                         options, buf, sizeof(buf), &status);

  if (!upb_ok(&status)) {
    rb_raise(cParseError, "Error occurred during encoding: %s",
             upb_status_errmsg(&status));
  }

  if (size >= sizeof(buf)) {
    char *buf2 = malloc(size + 1);
    VALUE ret;
    upb_json_encode(msg->msg, msg->descriptor->msgdef, pool->symtab, options,
                    buf2, size + 1, &status);
    ret = rb_str_new(buf2, size);
    free(buf2);
    return ret;
  } else {
    return rb_str_new(buf, size);
  }
}

/*
 * call-seq:
 *     Google::Protobuf.discard_unknown(msg)
 *
 * Discard unknown fields in the given message object and recursively discard
 * unknown fields in submessages.
 */
VALUE Google_Protobuf_discard_unknown(VALUE self, VALUE msg_rb) {
  Message* msg;

  if (CLASS_OF(msg_rb) == cRepeatedField || CLASS_OF(msg_rb) == cMap) {
    rb_raise(rb_eArgError, "Expected proto msg for discard unknown.");
  }

  TypedData_Get_Struct(msg_rb, Message, &Message_type, msg);
  if (!upb_msg_discardunknown(msg->msg, msg->descriptor->msgdef, 128)) {
    rb_raise(rb_eRuntimeError, "Messages nested too deeply.");
  }

  return Qnil;
}

/*
 * call-seq:
 *     Message.descriptor => descriptor
 *
 * Class method that returns the Descriptor instance corresponding to this
 * message class's type.
 */
VALUE Message_descriptor(VALUE klass) {
  return rb_ivar_get(klass, descriptor_instancevar_interned);
}

VALUE build_class_from_descriptor(VALUE descriptor) {
  Descriptor* desc = ruby_to_Descriptor(descriptor);
  const char *name;
  VALUE klass;

  name = upb_msgdef_fullname(desc->msgdef);
  if (name == NULL) {
    rb_raise(rb_eRuntimeError, "Descriptor does not have assigned name.");
  }

  klass = rb_define_class_id(
      // Docs say this parameter is ignored. User will assign return value to
      // their own toplevel constant class name.
      rb_intern("Message"),
      rb_cObject);
  rb_ivar_set(klass, descriptor_instancevar_interned, descriptor);
  rb_define_alloc_func(klass, Message_alloc);
  rb_require("google/protobuf/message_exts");
  rb_include_module(klass, rb_eval_string("::Google::Protobuf::MessageExts"));
  rb_extend_object(
      klass, rb_eval_string("::Google::Protobuf::MessageExts::ClassMethods"));

  rb_define_method(klass, "method_missing",
                   Message_method_missing, -1);
  rb_define_method(klass, "respond_to_missing?",
                   Message_respond_to_missing, -1);
  rb_define_method(klass, "initialize", Message_initialize, -1);
  rb_define_method(klass, "dup", Message_dup, 0);
  // Also define #clone so that we don't inherit Object#clone.
  rb_define_method(klass, "clone", Message_dup, 0);
  rb_define_method(klass, "==", Message_eq, 1);
  rb_define_method(klass, "eql?", Message_eq, 1);
  rb_define_method(klass, "hash", Message_hash, 0);
  rb_define_method(klass, "to_h", Message_to_h, 0);
  rb_define_method(klass, "inspect", Message_inspect, 0);
  rb_define_method(klass, "to_s", Message_inspect, 0);
  rb_define_method(klass, "[]", Message_index, 1);
  rb_define_method(klass, "[]=", Message_index_set, 2);
  rb_define_singleton_method(klass, "decode", Message_decode, 1);
  rb_define_singleton_method(klass, "encode", Message_encode, 1);
  rb_define_singleton_method(klass, "decode_json", Message_decode_json, -1);
  rb_define_singleton_method(klass, "encode_json", Message_encode_json, -1);
  rb_define_singleton_method(klass, "descriptor", Message_descriptor, 0);

  return klass;
}

/*
 * call-seq:
 *     Enum.lookup(number) => name
 *
 * This module method, provided on each generated enum module, looks up an enum
 * value by number and returns its name as a Ruby symbol, or nil if not found.
 */
VALUE enum_lookup(VALUE self, VALUE number) {
  int32_t num = NUM2INT(number);
  VALUE desc = rb_ivar_get(self, descriptor_instancevar_interned);
  EnumDescriptor* enumdesc = ruby_to_EnumDescriptor(desc);

  const char* name = upb_enumdef_iton(enumdesc->enumdef, num);
  if (name == NULL) {
    return Qnil;
  } else {
    return ID2SYM(rb_intern(name));
  }
}

/*
 * call-seq:
 *     Enum.resolve(name) => number
 *
 * This module method, provided on each generated enum module, looks up an enum
 * value by name (as a Ruby symbol) and returns its name, or nil if not found.
 */
VALUE enum_resolve(VALUE self, VALUE sym) {
  const char* name = rb_id2name(SYM2ID(sym));
  VALUE desc = rb_ivar_get(self, descriptor_instancevar_interned);
  EnumDescriptor* enumdesc = ruby_to_EnumDescriptor(desc);

  int32_t num = 0;
  bool found = upb_enumdef_ntoiz(enumdesc->enumdef, name, &num);
  if (!found) {
    return Qnil;
  } else {
    return INT2NUM(num);
  }
}

/*
 * call-seq:
 *     Enum.descriptor
 *
 * This module method, provided on each generated enum module, returns the
 * EnumDescriptor corresponding to this enum type.
 */
VALUE enum_descriptor(VALUE self) {
  return rb_ivar_get(self, descriptor_instancevar_interned);
}

VALUE build_module_from_enumdesc(VALUE _enumdesc) {
  EnumDescriptor* enumdesc = ruby_to_EnumDescriptor(_enumdesc);
  VALUE mod = rb_define_module_id(
      rb_intern(upb_enumdef_fullname(enumdesc->enumdef)));

  upb_enum_iter it;
  for (upb_enum_begin(&it, enumdesc->enumdef);
       !upb_enum_done(&it);
       upb_enum_next(&it)) {
    const char* name = upb_enum_iter_name(&it);
    int32_t value = upb_enum_iter_number(&it);
    if (name[0] < 'A' || name[0] > 'Z') {
      rb_warn("Enum value '%s' does not start with an uppercase letter "
              "as is required for Ruby constants.",
              name);
    }
    rb_define_const(mod, name, INT2NUM(value));
  }

  rb_define_singleton_method(mod, "lookup", enum_lookup, 1);
  rb_define_singleton_method(mod, "resolve", enum_resolve, 1);
  rb_define_singleton_method(mod, "descriptor", enum_descriptor, 0);
  rb_ivar_set(mod, descriptor_instancevar_interned, _enumdesc);

  return mod;
}

/*
 * call-seq:
 *     Google::Protobuf.deep_copy(obj) => copy_of_obj
 *
 * Performs a deep copy of a RepeatedField instance, a Map instance, or a
 * message object, recursively copying its members.
 */
VALUE Google_Protobuf_deep_copy(VALUE self, VALUE obj) {
  VALUE klass = CLASS_OF(obj);
  if (klass == cRepeatedField) {
    return RepeatedField_deep_copy(obj);
  } else if (klass == cMap) {
    return Map_deep_copy(obj);
  } else {
    return Message_deep_copy(obj);
  }
}
