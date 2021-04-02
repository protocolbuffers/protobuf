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
#include "defs.h"
#include "map.h"
#include "protobuf.h"
#include "repeated_field.h"
#include "third_party/wyhash/wyhash.h"

static VALUE cParseError = Qnil;
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
  VALUE arena;
  const upb_msg* msg;        // Can get as mutable when non-frozen.
  const upb_msgdef* msgdef;  // kept alive by self.class.descriptor reference.
} Message;

static void Message_mark(void* _self) {
  Message* self = (Message *)_self;
  rb_gc_mark(self->arena);
}

static rb_data_type_t Message_type = {
  "Message",
  { Message_mark, RUBY_DEFAULT_FREE, NULL },
  .flags = RUBY_TYPED_FREE_IMMEDIATELY,
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

const upb_msg *Message_Get(VALUE msg_rb, const upb_msgdef **m) {
  Message* msg = ruby_to_Message(msg_rb);
  if (m) *m = msg->msgdef;
  return msg->msg;
}

upb_msg *Message_GetMutable(VALUE msg_rb, const upb_msgdef **m) {
  rb_check_frozen(msg_rb);
  return (upb_msg*)Message_Get(msg_rb, m);
}

void Message_InitPtr(VALUE self_, upb_msg *msg, VALUE arena) {
  Message* self = ruby_to_Message(self_);
  self->msg = msg;
  self->arena = arena;
  ObjectCache_Add(msg, self_);
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

VALUE Message_GetRubyWrapper(upb_msg* msg, const upb_msgdef* m, VALUE arena) {
  if (msg == NULL) return Qnil;

  VALUE val = ObjectCache_Get(msg);

  if (val == Qnil) {
    VALUE klass = Descriptor_DefToClass(m);
    val = Message_alloc(klass);
    Message_InitPtr(val, msg, arena);
  }

  return val;
}

void Message_PrintMessage(StringBuilder* b, const upb_msg* msg,
                          const upb_msgdef* m) {
  bool first = true;
  int n = upb_msgdef_fieldcount(m);
  VALUE klass = Descriptor_DefToClass(m);
  StringBuilder_Printf(b, "<%s: ", rb_class2name(klass));

  for (int i = 0; i < n; i++) {
    const upb_fielddef* field = upb_msgdef_field(m, i);

    if (upb_fielddef_haspresence(field) && !upb_msg_has(msg, field)) {
      continue;
    }

    if (!first) {
      StringBuilder_Printf(b, ", ");
    } else {
      first = false;
    }

    upb_msgval msgval = upb_msg_get(msg, field);

    StringBuilder_Printf(b, "%s: ", upb_fielddef_name(field));

    if (upb_fielddef_ismap(field)) {
      const upb_msgdef* entry_m = upb_fielddef_msgsubdef(field);
      const upb_fielddef* key_f = upb_msgdef_itof(entry_m, 1);
      const upb_fielddef* val_f = upb_msgdef_itof(entry_m, 2);
      TypeInfo val_info = TypeInfo_get(val_f);
      Map_Inspect(b, msgval.map_val, upb_fielddef_type(key_f), val_info);
    } else if (upb_fielddef_isseq(field)) {
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
static bool IsWrapper(const upb_fielddef* f) {
  return upb_fielddef_issubmsg(f) &&
         upb_msgdef_iswrapper(upb_fielddef_msgsubdef(f));
}

static bool Match(const upb_msgdef* m, const char* name, const upb_fielddef** f,
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
  const upb_msgdef* m = self->msgdef;
  const char* name;

  Check_Type(method_name, T_SYMBOL);
  name = rb_id2name(SYM2ID(method_name));

  if (Match(m, name, f, o, "", "")) return METHOD_GETTER;
  if (Match(m, name, f, o, "", "=")) return METHOD_SETTER;
  if (Match(m, name, f, o, "clear_", "")) return METHOD_CLEAR;
  if (Match(m, name, f, o, "has_", "?") &&
      (*o || (*f && upb_fielddef_haspresence(*f)))) {
    // Disallow oneof hazzers for proto3.
    // TODO(haberman): remove this test when we are enabling oneof hazzers for
    // proto3.
    if (*f && !upb_fielddef_issubmsg(*f) &&
        upb_fielddef_realcontainingoneof(*f) &&
        upb_msgdef_syntax(upb_fielddef_containingtype(*f)) !=
            UPB_SYNTAX_PROTO2) {
      return METHOD_UNKNOWN;
    }
    return METHOD_PRESENCE;
  }
  if (Match(m, name, f, o, "", "_as_value") && *f && !upb_fielddef_isseq(*f) &&
      IsWrapper(*f)) {
    return METHOD_WRAPPER_GETTER;
  }
  if (Match(m, name, f, o, "", "_as_value=") && *f && !upb_fielddef_isseq(*f) &&
      IsWrapper(*f)) {
    return METHOD_WRAPPER_SETTER;
  }
  if (Match(m, name, f, o, "", "_const") && *f &&
      upb_fielddef_type(*f) == UPB_TYPE_ENUM) {
    return METHOD_ENUM_GETTER;
  }

  return METHOD_UNKNOWN;
}

static VALUE Message_oneof_accessor(VALUE _self, const upb_oneofdef* o,
                                    int accessor_type) {
  Message* self = ruby_to_Message(_self);
  const upb_fielddef* oneof_field = upb_msg_whichoneof(self->msg, o);

  switch (accessor_type) {
    case METHOD_PRESENCE:
      return oneof_field == NULL ? Qfalse : Qtrue;
    case METHOD_CLEAR:
      if (oneof_field != NULL) {
        upb_msg_clearfield(Message_GetMutable(_self, NULL), oneof_field);
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

static void Message_setfield(upb_msg* msg, const upb_fielddef* f, VALUE val,
                             upb_arena* arena) {
  upb_msgval msgval;
  if (upb_fielddef_ismap(f)) {
    msgval.map_val = Map_GetUpbMap(val, f);
  } else if (upb_fielddef_isseq(f)) {
    msgval.array_val = RepeatedField_GetUpbArray(val, f);
  } else {
    if (val == Qnil &&
        (upb_fielddef_issubmsg(f) || upb_fielddef_realcontainingoneof(f))) {
      upb_msg_clearfield(msg, f);
      return;
    }
    msgval =
        Convert_RubyToUpb(val, upb_fielddef_name(f), TypeInfo_get(f), arena);
  }
  upb_msg_set(msg, f, msgval, arena);
}

VALUE Message_getfield(VALUE _self, const upb_fielddef* f) {
  Message* self = ruby_to_Message(_self);
  // This is a special-case: upb_msg_mutable() for map & array are logically
  // const (they will not change what is serialized) but physically
  // non-const, as they do allocate a repeated field or map. The logical
  // constness means it's ok to do even if the message is frozen.
  upb_msg *msg = (upb_msg*)self->msg;
  upb_arena *arena = Arena_get(self->arena);
  if (upb_fielddef_ismap(f)) {
    upb_map *map = upb_msg_mutable(msg, f, arena).map;
    const upb_fielddef *key_f = map_field_key(f);
    const upb_fielddef *val_f = map_field_value(f);
    upb_fieldtype_t key_type = upb_fielddef_type(key_f);
    TypeInfo value_type_info = TypeInfo_get(val_f);
    return Map_GetRubyWrapper(map, key_type, value_type_info, self->arena);
  } else if (upb_fielddef_isseq(f)) {
    upb_array *arr = upb_msg_mutable(msg, f, arena).array;
    return RepeatedField_GetRubyWrapper(arr, TypeInfo_get(f), self->arena);
  } else if (upb_fielddef_issubmsg(f)) {
    if (!upb_msg_has(self->msg, f)) return Qnil;
    upb_msg *submsg = upb_msg_mutable(msg, f, arena).msg;
    const upb_msgdef *m = upb_fielddef_msgsubdef(f);
    return Message_GetRubyWrapper(submsg, m, self->arena);
  } else {
    upb_msgval msgval = upb_msg_get(self->msg, f);
    return Convert_UpbToRuby(msgval, TypeInfo_get(f), self->arena);
  }
}

static VALUE Message_field_accessor(VALUE _self, const upb_fielddef* f,
                                    int accessor_type, int argc, VALUE* argv) {
  upb_arena *arena = Arena_get(Message_GetArena(_self));

  switch (accessor_type) {
    case METHOD_SETTER:
      Message_setfield(Message_GetMutable(_self, NULL), f, argv[1], arena);
      return Qnil;
    case METHOD_CLEAR:
      upb_msg_clearfield(Message_GetMutable(_self, NULL), f);
      return Qnil;
    case METHOD_PRESENCE:
      if (!upb_fielddef_haspresence(f)) {
        rb_raise(rb_eRuntimeError, "Field does not have presence.");
      }
      return upb_msg_has(Message_Get(_self, NULL), f);
    case METHOD_WRAPPER_GETTER: {
      Message* self = ruby_to_Message(_self);
      if (upb_msg_has(self->msg, f)) {
        PBRUBY_ASSERT(upb_fielddef_issubmsg(f) && !upb_fielddef_isseq(f));
        upb_msgval wrapper = upb_msg_get(self->msg, f);
        const upb_msgdef *wrapper_m = upb_fielddef_msgsubdef(f);
        const upb_fielddef *value_f = upb_msgdef_itof(wrapper_m, 1);
        upb_msgval value = upb_msg_get(wrapper.msg_val, value_f);
        return Convert_UpbToRuby(value, TypeInfo_get(value_f), self->arena);
      } else {
        return Qnil;
      }
    }
    case METHOD_WRAPPER_SETTER: {
      upb_msg *msg = Message_GetMutable(_self, NULL);
      if (argv[1] == Qnil) {
        upb_msg_clearfield(msg, f);
      } else {
        const upb_fielddef *val_f = upb_msgdef_itof(upb_fielddef_msgsubdef(f), 1);
        upb_msgval msgval = Convert_RubyToUpb(argv[1], upb_fielddef_name(f),
                                              TypeInfo_get(val_f), arena);
        upb_msg *wrapper = upb_msg_mutable(msg, f, arena).msg;
        upb_msg_set(wrapper, val_f, msgval, arena);
      }
      return Qnil;
    }
    case METHOD_ENUM_GETTER: {
      upb_msgval msgval = upb_msg_get(Message_Get(_self, NULL), f);

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
  const upb_oneofdef* o;
  const upb_fielddef* f;
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
  const upb_oneofdef* o;
  const upb_fielddef* f;
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

void Message_InitFromValue(upb_msg* msg, const upb_msgdef* m, VALUE val,
                           upb_arena* arena);

typedef struct {
  upb_map *map;
  TypeInfo key_type;
  TypeInfo val_type;
  upb_arena *arena;
} MapInit;

static int Map_initialize_kwarg(VALUE key, VALUE val, VALUE _self) {
  MapInit *map_init = (MapInit*)_self;
  upb_msgval k, v;
  k = Convert_RubyToUpb(key, "", map_init->key_type, NULL);

  if (map_init->val_type.type == UPB_TYPE_MESSAGE && TYPE(val) == T_HASH) {
    upb_msg *msg = upb_msg_new(map_init->val_type.def.msgdef, map_init->arena);
    Message_InitFromValue(msg, map_init->val_type.def.msgdef, val,
                          map_init->arena);
    v.msg_val = msg;
  } else {
    v = Convert_RubyToUpb(val, "", map_init->val_type, map_init->arena);
  }
  upb_map_set(map_init->map, k, v, map_init->arena);
  return ST_CONTINUE;
}

static void Map_InitFromValue(upb_map* map, const upb_fielddef* f, VALUE val,
                       upb_arena* arena) {
  const upb_msgdef* entry_m = upb_fielddef_msgsubdef(f);
  const upb_fielddef* key_f = upb_msgdef_itof(entry_m, 1);
  const upb_fielddef* val_f = upb_msgdef_itof(entry_m, 2);
  if (TYPE(val) != T_HASH) {
    rb_raise(rb_eArgError,
             "Expected Hash object as initializer value for map field '%s' "
             "(given %s).",
             upb_fielddef_name(f), rb_class2name(CLASS_OF(val)));
  }
  MapInit map_init = {map, TypeInfo_get(key_f), TypeInfo_get(val_f), arena};
  rb_hash_foreach(val, Map_initialize_kwarg, (VALUE)&map_init);
}

static upb_msgval MessageValue_FromValue(VALUE val, TypeInfo info,
                                         upb_arena* arena) {
  if (info.type == UPB_TYPE_MESSAGE) {
    upb_msgval msgval;
    upb_msg* msg = upb_msg_new(info.def.msgdef, arena);
    Message_InitFromValue(msg, info.def.msgdef, val, arena);
    msgval.msg_val = msg;
    return msgval;
  } else {
    return Convert_RubyToUpb(val, "", info, arena);
  }
}

static void RepeatedField_InitFromValue(upb_array* arr, const upb_fielddef* f,
                                        VALUE val, upb_arena* arena) {
  TypeInfo type_info = TypeInfo_get(f);

  if (TYPE(val) != T_ARRAY) {
    rb_raise(rb_eArgError,
             "Expected array as initializer value for repeated field '%s' (given %s).",
             upb_fielddef_name(f), rb_class2name(CLASS_OF(val)));
  }

  for (int i = 0; i < RARRAY_LEN(val); i++) {
    VALUE entry = rb_ary_entry(val, i);
    upb_msgval msgval;
    if (upb_fielddef_issubmsg(f) && TYPE(entry) == T_HASH) {
      msgval = MessageValue_FromValue(entry, type_info, arena);
    } else {
      msgval = Convert_RubyToUpb(entry, upb_fielddef_name(f), type_info, arena);
    }
    upb_array_append(arr, msgval, arena);
  }
}

static void Message_InitFieldFromValue(upb_msg* msg, const upb_fielddef* f,
                                       VALUE val, upb_arena* arena) {
  if (TYPE(val) == T_NIL) return;

  if (upb_fielddef_ismap(f)) {
    upb_map *map = upb_msg_mutable(msg, f, arena).map;
    Map_InitFromValue(map, f, val, arena);
  } else if (upb_fielddef_label(f) == UPB_LABEL_REPEATED) {
    upb_array *arr = upb_msg_mutable(msg, f, arena).array;
    RepeatedField_InitFromValue(arr, f, val, arena);
  } else if (upb_fielddef_issubmsg(f)) {
    if (TYPE(val) == T_HASH) {
      upb_msg *submsg = upb_msg_mutable(msg, f, arena).msg;
      Message_InitFromValue(submsg, upb_fielddef_msgsubdef(f), val, arena);
    } else {
      Message_setfield(msg, f, val, arena);
    }
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

static int Message_initialize_kwarg(VALUE key, VALUE val, VALUE _self) {
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

  Message_InitFieldFromValue(msg_init->msg, f, val, msg_init->arena);
  return ST_CONTINUE;
}

void Message_InitFromValue(upb_msg* msg, const upb_msgdef* m, VALUE val,
                           upb_arena* arena) {
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
  upb_arena *arena = Arena_get(arena_rb);
  upb_msg *msg = upb_msg_new(self->msgdef, arena);

  Message_InitPtr(_self, msg, arena_rb);

  if (argc == 0) {
    return Qnil;
  }
  if (argc != 1) {
    rb_raise(rb_eArgError, "Expected 0 or 1 arguments.");
  }
  Message_InitFromValue((upb_msg*)self->msg, self->msgdef, argv[0], arena);
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
  size_t size = upb_msgdef_layout(self->msgdef)->size;

  // TODO(copy unknown fields?)
  // TODO(use official upb msg copy function)
  memcpy((upb_msg*)new_msg_self->msg, self->msg, size);
  upb_arena_fuse(Arena_get(new_msg_self->arena), Arena_get(self->arena));
  return new_msg;
}

// Support function for Message_eq, and also used by other #eq functions.
bool Message_Equal(const upb_msg *m1, const upb_msg *m2, const upb_msgdef *m) {
  if (m1 == m2) return true;

  size_t size1, size2;
  int encode_opts = UPB_ENCODE_SKIPUNKNOWN | UPB_ENCODE_DETERMINISTIC;
  upb_arena *arena_tmp = upb_arena_new();
  const upb_msglayout *layout = upb_msgdef_layout(m);

  // Compare deterministically serialized payloads with no unknown fields.
  char *data1 = upb_encode_ex(m1, layout, encode_opts, arena_tmp, &size1);
  char *data2 = upb_encode_ex(m2, layout, encode_opts, arena_tmp, &size2);

  if (data1 && data2) {
    bool ret = (size1 == size2) && (memcmp(data1, data2, size1) == 0);
    upb_arena_free(arena_tmp);
    return ret;
  } else {
    upb_arena_free(arena_tmp);
    rb_raise(cParseError, "Error comparing messages");
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

uint64_t Message_Hash(const upb_msg* msg, const upb_msgdef* m, uint64_t seed) {
  upb_arena *arena = upb_arena_new();
  const char *data;
  size_t size;

  // Hash a deterministically serialized payloads with no unknown fields.
  data = upb_encode_ex(msg, upb_msgdef_layout(m),
                       UPB_ENCODE_SKIPUNKNOWN | UPB_ENCODE_DETERMINISTIC, arena,
                       &size);

  if (data) {
    uint64_t ret = wyhash(data, size, seed, _wyp);
    upb_arena_free(arena);
    return ret;
  } else {
    upb_arena_free(arena);
    rb_raise(cParseError, "Error calculating hash");
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
  return INT2FIX(Message_Hash(self->msg, self->msgdef, 0));
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

static VALUE RepeatedField_CreateArray(const upb_array* arr,
                                       TypeInfo type_info) {
  int size = arr ? upb_array_size(arr) : 0;
  VALUE ary = rb_ary_new2(size);

  for (int i = 0; i < size; i++) {
    upb_msgval msgval = upb_array_get(arr, i);
    VALUE val = Scalar_CreateHash(msgval, type_info);
    rb_ary_push(ary, val);
  }

  return ary;
}

static VALUE Message_CreateHash(const upb_msg *msg, const upb_msgdef *m) {
  if (!msg) return Qnil;

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

    // Proto2 omits empty map/repeated filds also.

    if (upb_fielddef_ismap(field)) {
      const upb_msgdef *entry_m = upb_fielddef_msgsubdef(field);
      const upb_fielddef *key_f = upb_msgdef_itof(entry_m, 1);
      const upb_fielddef *val_f = upb_msgdef_itof(entry_m, 2);
      upb_fieldtype_t key_type = upb_fielddef_type(key_f);
      msg_value = Map_CreateHash(msgval.map_val, key_type, TypeInfo_get(val_f));
    } else if (upb_fielddef_isseq(field)) {
      if (is_proto2 &&
          (!msgval.array_val || upb_array_size(msgval.array_val) == 0)) {
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

VALUE Scalar_CreateHash(upb_msgval msgval, TypeInfo type_info) {
  if (type_info.type == UPB_TYPE_MESSAGE) {
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
 * call-seq:
 *     Message.[](index) => value
 *
 * Accesses a field's value by field name. The provided field name should be a
 * string.
 */
static VALUE Message_index(VALUE _self, VALUE field_name) {
  Message* self = ruby_to_Message(_self);
  const upb_fielddef* field;

  Check_Type(field_name, T_STRING);
  field = upb_msgdef_ntofz(self->msgdef, RSTRING_PTR(field_name));

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
  const upb_fielddef* f;
  upb_msgval val;
  upb_arena *arena = Arena_get(self->arena);

  Check_Type(field_name, T_STRING);
  f = upb_msgdef_ntofz(self->msgdef, RSTRING_PTR(field_name));

  if (f == NULL) {
    rb_raise(rb_eArgError, "Unknown field: %s", RSTRING_PTR(field_name));
  }

  val = Convert_RubyToUpb(value, upb_fielddef_name(f), TypeInfo_get(f), arena);
  upb_msg_set(Message_GetMutable(_self, NULL), f, val, arena);

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
static VALUE Message_decode(VALUE klass, VALUE data) {
  if (TYPE(data) != T_STRING) {
    rb_raise(rb_eArgError, "Expected string for binary protobuf data.");
  }

  VALUE msg_rb = initialize_rb_class_with_no_args(klass);
  Message* msg = ruby_to_Message(msg_rb);

  if (!upb_decode(RSTRING_PTR(data), RSTRING_LEN(data), (upb_msg*)msg->msg,
                 upb_msgdef_layout(msg->msgdef),
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
static VALUE Message_decode_json(int argc, VALUE* argv, VALUE klass) {
  VALUE data = argv[0];
  int options = 0;
  upb_status status;

  // TODO(haberman): use this message's pool instead.
  const upb_symtab *symtab = DescriptorPool_GetSymtab(generated_pool);

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

  VALUE msg_rb = initialize_rb_class_with_no_args(klass);
  Message* msg = ruby_to_Message(msg_rb);

  // We don't allow users to decode a wrapper type directly.
  if (upb_msgdef_iswrapper(msg->msgdef)) {
    rb_raise(rb_eRuntimeError, "Cannot parse a wrapper directly.");
  }

  upb_status_clear(&status);
  if (!upb_json_decode(RSTRING_PTR(data), RSTRING_LEN(data), (upb_msg*)msg->msg,
                       msg->msgdef, symtab, options,
                       Arena_get(msg->arena), &status)) {
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
static VALUE Message_encode(VALUE klass, VALUE msg_rb) {
  Message* msg = ruby_to_Message(msg_rb);
  upb_arena *arena = upb_arena_new();
  const char *data;
  size_t size;

  if (CLASS_OF(msg_rb) != klass) {
    rb_raise(rb_eArgError, "Message of wrong type.");
  }

  data = upb_encode(msg->msg, upb_msgdef_layout(msg->msgdef), arena,
                    &size);

  if (data) {
    VALUE ret = rb_str_new(data, size);
    rb_enc_associate(ret, rb_ascii8bit_encoding());
    upb_arena_free(arena);
    return ret;
  } else {
    upb_arena_free(arena);
    rb_raise(rb_eRuntimeError, "Exceeded maximum depth (possibly cycle)");
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
static VALUE Message_encode_json(int argc, VALUE* argv, VALUE klass) {
  Message* msg = ruby_to_Message(argv[0]);
  int options = 0;
  char buf[1024];
  size_t size;
  upb_status status;

  // TODO(haberman): use this message's pool instead.
  const upb_symtab *symtab = DescriptorPool_GetSymtab(generated_pool);

  if (argc < 1 || argc > 2) {
    rb_raise(rb_eArgError, "Expected 1 or 2 arguments.");
  }

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
  size = upb_json_encode(msg->msg, msg->msgdef, symtab, options, buf,
                         sizeof(buf), &status);

  if (!upb_ok(&status)) {
    rb_raise(cParseError, "Error occurred during encoding: %s",
             upb_status_errmsg(&status));
  }

  VALUE ret;
  if (size >= sizeof(buf)) {
    char* buf2 = malloc(size + 1);
    upb_json_encode(msg->msg, msg->msgdef, symtab, options, buf2, size + 1,
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
  const char *name;
  VALUE klass;

  name = upb_msgdef_fullname(Descriptor_GetMsgDef(descriptor));
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
  rb_define_method(klass, "freeze", Message_freeze, 0);
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
static VALUE enum_lookup(VALUE self, VALUE number) {
  int32_t num = NUM2INT(number);
  VALUE desc = rb_ivar_get(self, descriptor_instancevar_interned);
  const upb_enumdef *e = EnumDescriptor_GetEnumDef(desc);

  const char* name = upb_enumdef_iton(e, num);
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
static VALUE enum_resolve(VALUE self, VALUE sym) {
  const char* name = rb_id2name(SYM2ID(sym));
  VALUE desc = rb_ivar_get(self, descriptor_instancevar_interned);
  const upb_enumdef *e = EnumDescriptor_GetEnumDef(desc);

  int32_t num = 0;
  bool found = upb_enumdef_ntoiz(e, name, &num);
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
static VALUE enum_descriptor(VALUE self) {
  return rb_ivar_get(self, descriptor_instancevar_interned);
}

VALUE build_module_from_enumdesc(VALUE _enumdesc) {
  const upb_enumdef *e = EnumDescriptor_GetEnumDef(_enumdesc);
  VALUE mod = rb_define_module_id(rb_intern(upb_enumdef_fullname(e)));

  upb_enum_iter it;
  for (upb_enum_begin(&it, e);
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

// Internal only; used by Google::Protobuf.deep_copy.
upb_msg* Message_deep_copy(const upb_msg* msg, const upb_msgdef* m,
                           upb_arena *arena) {
  // Serialize and parse.
  upb_arena *tmp_arena = upb_arena_new();
  const upb_msglayout *layout = upb_msgdef_layout(m);
  size_t size;

  char* data = upb_encode_ex(msg, layout, 0, tmp_arena, &size);
  upb_msg* new_msg = upb_msg_new(m, arena);

  if (!data || !upb_decode(data, size, new_msg, layout, arena)) {
    upb_arena_free(tmp_arena);
    rb_raise(cParseError, "Error occurred copying proto");
  }

  upb_arena_free(tmp_arena);
  return new_msg;
}

const upb_msg* Message_GetUpbMessage(VALUE value, const upb_msgdef* m,
                                     const char* name, upb_arena* arena) {
  if (value == Qnil) {
    rb_raise(cTypeError, "nil message not allowed here.");
  }

  VALUE klass = CLASS_OF(value);
  VALUE desc_rb = rb_ivar_get(klass, descriptor_instancevar_interned);
  const upb_msgdef* val_m =
      desc_rb == Qnil ? NULL : Descriptor_GetMsgDef(desc_rb);

  if (val_m != m) {
    // Check for possible implicit conversions
    // TODO: hash conversion?

    switch (upb_msgdef_wellknowntype(m)) {
      case UPB_WELLKNOWN_TIMESTAMP: {
        // Time -> Google::Protobuf::Timestamp
        upb_msg *msg = upb_msg_new(m, arena);
        upb_msgval sec, nsec;
        struct timespec time;
        const upb_fielddef *sec_f = upb_msgdef_itof(m, 1);
        const upb_fielddef *nsec_f = upb_msgdef_itof(m, 2);

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
        upb_msg *msg = upb_msg_new(m, arena);
        upb_msgval sec, nsec;
        const upb_fielddef *sec_f = upb_msgdef_itof(m, 1);
        const upb_fielddef *nsec_f = upb_msgdef_itof(m, 2);

        if (!rb_obj_is_kind_of(value, rb_cNumeric)) goto badtype;

        sec.int64_val = NUM2LL(value);
        nsec.int32_val = round((NUM2DBL(value) - NUM2LL(value)) * 1000000000);
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

  Message* self = ruby_to_Message(value);
  upb_arena_fuse(arena, Arena_get(self->arena));

  return self->msg;
}

void Message_register(VALUE protobuf) {
  cParseError = rb_const_get(protobuf, rb_intern("ParseError"));

  // Ruby-interned string: "descriptor". We use this identifier to store an
  // instance variable on message classes we create in order to link them back
  // to their descriptors.
  descriptor_instancevar_interned = rb_intern("descriptor");
}
