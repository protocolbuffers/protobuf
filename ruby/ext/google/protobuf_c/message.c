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

#include "protobuf.h"

// -----------------------------------------------------------------------------
// Class/module creation from msgdefs and enumdefs, respectively.
// -----------------------------------------------------------------------------

void* Message_data(void* msg) {
  return ((uint8_t *)msg) + sizeof(MessageHeader);
}

void Message_mark(void* _self) {
  MessageHeader* self = (MessageHeader *)_self;
  layout_mark(self->descriptor->layout, Message_data(self));
}

void Message_free(void* self) {
  stringsink* unknown = ((MessageHeader *)self)->unknown_fields;
  if (unknown != NULL) {
    stringsink_uninit(unknown);
    free(unknown);
  }
  xfree(self);
}

rb_data_type_t Message_type = {
  "Message",
  { Message_mark, Message_free, NULL },
};

VALUE Message_alloc(VALUE klass) {
  VALUE descriptor = rb_ivar_get(klass, descriptor_instancevar_interned);
  Descriptor* desc = ruby_to_Descriptor(descriptor);
  MessageHeader* msg = (MessageHeader*)ALLOC_N(
      uint8_t, sizeof(MessageHeader) + desc->layout->size);
  VALUE ret;

  memset(Message_data(msg), 0, desc->layout->size);

  // We wrap first so that everything in the message object is GC-rooted in case
  // a collection happens during object creation in layout_init().
  ret = TypedData_Wrap_Struct(klass, &Message_type, msg);
  msg->descriptor = desc;
  rb_ivar_set(ret, descriptor_instancevar_interned, descriptor);

  msg->unknown_fields = NULL;

  layout_init(desc->layout, Message_data(msg));

  return ret;
}

static const upb_fielddef* which_oneof_field(MessageHeader* self, const upb_oneofdef* o) {
  upb_oneof_iter it;
  size_t case_ofs;
  uint32_t oneof_case;
  const upb_fielddef* first_field;
  const upb_fielddef* f;

  // If no fields in the oneof, always nil.
  if (upb_oneofdef_numfields(o) == 0) {
    return NULL;
  }
  // Grab the first field in the oneof so we can get its layout info to find the
  // oneof_case field.
  upb_oneof_begin(&it, o);
  assert(!upb_oneof_done(&it));
  first_field = upb_oneof_iter_field(&it);
  assert(upb_fielddef_containingoneof(first_field) != NULL);

  case_ofs =
      self->descriptor->layout->
      fields[upb_fielddef_index(first_field)].case_offset;
  oneof_case = *((uint32_t*)((char*)Message_data(self) + case_ofs));

  if (oneof_case == ONEOF_CASE_NONE) {
    return NULL;
  }

  // oneof_case is a field index, so find that field.
  f = upb_oneofdef_itof(o, oneof_case);
  assert(f != NULL);

  return f;
}

enum {
  METHOD_UNKNOWN = 0,
  METHOD_GETTER = 1,
  METHOD_SETTER = 2,
  METHOD_CLEAR = 3,
  METHOD_PRESENCE = 4
};

static int extract_method_call(VALUE method_name, MessageHeader* self,
			       const upb_fielddef **f, const upb_oneofdef **o) {
  Check_Type(method_name, T_SYMBOL);

  VALUE method_str = rb_id2str(SYM2ID(method_name));
  char* name = RSTRING_PTR(method_str);
  size_t name_len = RSTRING_LEN(method_str);
  int accessor_type;
  const upb_oneofdef* test_o;
  const upb_fielddef* test_f;

  if (name[name_len - 1] == '=') {
    accessor_type = METHOD_SETTER;
    name_len--;
    // We want to ensure if the proto has something named clear_foo or has_foo?,
    // we don't strip the prefix.
  } else if (strncmp("clear_", name, 6) == 0 &&
             !upb_msgdef_lookupname(self->descriptor->msgdef, name, name_len,
				    &test_f, &test_o)) {
    accessor_type = METHOD_CLEAR;
    name = name + 6;
    name_len = name_len - 6;
  } else if (strncmp("has_", name, 4) == 0 && name[name_len - 1] == '?' &&
             !upb_msgdef_lookupname(self->descriptor->msgdef, name, name_len,
				    &test_f, &test_o)) {
    accessor_type = METHOD_PRESENCE;
    name = name + 4;
    name_len = name_len - 5;
  } else {
    accessor_type = METHOD_GETTER;
  }

  // Verify the name corresponds to a oneof or field in this message.
  if (!upb_msgdef_lookupname(self->descriptor->msgdef, name, name_len,
			     &test_f, &test_o)) {
    return METHOD_UNKNOWN;
  }

  // Method calls like 'has_foo?' are not allowed if field "foo" does not have
  // a hasbit (e.g. repeated fields or non-message type fields for proto3
  // syntax).
  if (accessor_type == METHOD_PRESENCE && test_f != NULL &&
      !upb_fielddef_haspresence(test_f)) {
    return METHOD_UNKNOWN;
  }

  *o = test_o;
  *f = test_f;
  return accessor_type;
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
  MessageHeader* self;
  const upb_oneofdef* o;
  const upb_fielddef* f;

  TypedData_Get_Struct(_self, MessageHeader, &Message_type, self);
  if (argc < 1) {
    rb_raise(rb_eArgError, "Expected method name as first argument.");
  }

  int accessor_type = extract_method_call(argv[0], self, &f, &o);
  if (accessor_type == METHOD_UNKNOWN || (o == NULL && f == NULL) ) {
    return rb_call_super(argc, argv);
  } else if (accessor_type == METHOD_SETTER) {
    if (argc != 2) {
      rb_raise(rb_eArgError, "Expected 2 arguments, received %d", argc);
    }
  } else if (argc != 1) {
    rb_raise(rb_eArgError, "Expected 1 argument, received %d", argc);
  }

  // Return which of the oneof fields are set
  if (o != NULL) {
    if (accessor_type == METHOD_SETTER) {
      rb_raise(rb_eRuntimeError, "Oneof accessors are read-only.");
    }

    const upb_fielddef* oneof_field = which_oneof_field(self, o);
    if (accessor_type == METHOD_PRESENCE) {
      return oneof_field == NULL ? Qfalse : Qtrue;
    } else if (accessor_type == METHOD_CLEAR) {
      if (oneof_field != NULL) {
	layout_clear(self->descriptor->layout, Message_data(self), oneof_field);
      }
      return Qnil;
    } else {
      // METHOD_ACCESSOR
      return oneof_field == NULL ? Qnil :
	ID2SYM(rb_intern(upb_fielddef_name(oneof_field)));
    }
  // Otherwise we're operating on a single proto field
  } else if (accessor_type == METHOD_SETTER) {
    layout_set(self->descriptor->layout, Message_data(self), f, argv[1]);
    return Qnil;
  } else if (accessor_type == METHOD_CLEAR) {
    layout_clear(self->descriptor->layout, Message_data(self), f);
    return Qnil;
  } else if (accessor_type == METHOD_PRESENCE) {
    return layout_has(self->descriptor->layout, Message_data(self), f);
  } else {
    return layout_get(self->descriptor->layout, Message_data(self), f);
  }
}


VALUE Message_respond_to_missing(int argc, VALUE* argv, VALUE _self) {
  MessageHeader* self;
  const upb_oneofdef* o;
  const upb_fielddef* f;

  TypedData_Get_Struct(_self, MessageHeader, &Message_type, self);
  if (argc < 1) {
    rb_raise(rb_eArgError, "Expected method name as first argument.");
  }

  int accessor_type = extract_method_call(argv[0], self, &f, &o);
  if (accessor_type == METHOD_UNKNOWN) {
    return rb_call_super(argc, argv);
  } else if (o != NULL) {
    return accessor_type == METHOD_SETTER ? Qfalse : Qtrue;
  } else {
    return Qtrue;
  }
}

VALUE create_submsg_from_hash(const upb_fielddef *f, VALUE hash) {
  const upb_def *d = upb_fielddef_subdef(f);
  assert(d != NULL);

  VALUE descriptor = get_def_obj(d);
  VALUE msgclass = rb_funcall(descriptor, rb_intern("msgclass"), 0, NULL);

  VALUE args[1] = { hash };
  return rb_class_new_instance(1, args, msgclass);
}

int Message_initialize_kwarg(VALUE key, VALUE val, VALUE _self) {
  MessageHeader* self;
  char *name;
  const upb_fielddef* f;
  TypedData_Get_Struct(_self, MessageHeader, &Message_type, self);

  if (TYPE(key) == T_STRING) {
    name = RSTRING_PTR(key);
  } else if (TYPE(key) == T_SYMBOL) {
    name = RSTRING_PTR(rb_id2str(SYM2ID(key)));
  } else {
    rb_raise(rb_eArgError,
             "Expected string or symbols as hash keys when initializing proto from hash.");
  }

  f = upb_msgdef_ntofz(self->descriptor->msgdef, name);
  if (f == NULL) {
    rb_raise(rb_eArgError,
             "Unknown field name '%s' in initialization map entry.", name);
  }

  if (TYPE(val) == T_NIL) {
    return 0;
  }

  if (is_map_field(f)) {
    VALUE map;

    if (TYPE(val) != T_HASH) {
      rb_raise(rb_eArgError,
               "Expected Hash object as initializer value for map field '%s'.", name);
    }
    map = layout_get(self->descriptor->layout, Message_data(self), f);
    Map_merge_into_self(map, val);
  } else if (upb_fielddef_label(f) == UPB_LABEL_REPEATED) {
    VALUE ary;

    if (TYPE(val) != T_ARRAY) {
      rb_raise(rb_eArgError,
               "Expected array as initializer value for repeated field '%s'.", name);
    }
    ary = layout_get(self->descriptor->layout, Message_data(self), f);
    for (int i = 0; i < RARRAY_LEN(val); i++) {
      VALUE entry = rb_ary_entry(val, i);
      if (TYPE(entry) == T_HASH && upb_fielddef_issubmsg(f)) {
        entry = create_submsg_from_hash(f, entry);
      }

      RepeatedField_push(ary, entry);
    }
  } else {
    if (TYPE(val) == T_HASH && upb_fielddef_issubmsg(f)) {
      val = create_submsg_from_hash(f, val);
    }

    layout_set(self->descriptor->layout, Message_data(self), f, val);
  }
  return 0;
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
  VALUE hash_args;

  if (argc == 0) {
    return Qnil;
  }
  if (argc != 1) {
    rb_raise(rb_eArgError, "Expected 0 or 1 arguments.");
  }
  hash_args = argv[0];
  if (TYPE(hash_args) != T_HASH) {
    rb_raise(rb_eArgError, "Expected hash arguments.");
  }

  rb_hash_foreach(hash_args, Message_initialize_kwarg, _self);
  return Qnil;
}

/*
 * call-seq:
 *     Message.dup => new_message
 *
 * Performs a shallow copy of this message and returns the new copy.
 */
VALUE Message_dup(VALUE _self) {
  MessageHeader* self;
  VALUE new_msg;
  MessageHeader* new_msg_self;
  TypedData_Get_Struct(_self, MessageHeader, &Message_type, self);

  new_msg = rb_class_new_instance(0, NULL, CLASS_OF(_self));
  TypedData_Get_Struct(new_msg, MessageHeader, &Message_type, new_msg_self);

  layout_dup(self->descriptor->layout,
             Message_data(new_msg_self),
             Message_data(self));

  return new_msg;
}

// Internal only; used by Google::Protobuf.deep_copy.
VALUE Message_deep_copy(VALUE _self) {
  MessageHeader* self;
  MessageHeader* new_msg_self;
  VALUE new_msg;
  TypedData_Get_Struct(_self, MessageHeader, &Message_type, self);

  new_msg = rb_class_new_instance(0, NULL, CLASS_OF(_self));
  TypedData_Get_Struct(new_msg, MessageHeader, &Message_type, new_msg_self);

  layout_deep_copy(self->descriptor->layout,
                   Message_data(new_msg_self),
                   Message_data(self));

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
  MessageHeader* self;
  MessageHeader* other;
  if (TYPE(_self) != TYPE(_other)) {
    return Qfalse;
  }
  TypedData_Get_Struct(_self, MessageHeader, &Message_type, self);
  TypedData_Get_Struct(_other, MessageHeader, &Message_type, other);

  if (self->descriptor != other->descriptor) {
    return Qfalse;
  }

  return layout_eq(self->descriptor->layout,
                   Message_data(self),
                   Message_data(other));
}

/*
 * call-seq:
 *     Message.hash => hash_value
 *
 * Returns a hash value that represents this message's field values.
 */
VALUE Message_hash(VALUE _self) {
  MessageHeader* self;
  TypedData_Get_Struct(_self, MessageHeader, &Message_type, self);

  return layout_hash(self->descriptor->layout, Message_data(self));
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
  MessageHeader* self;
  VALUE str;
  TypedData_Get_Struct(_self, MessageHeader, &Message_type, self);

  str = rb_str_new2("<");
  str = rb_str_append(str, rb_str_new2(rb_class2name(CLASS_OF(_self))));
  str = rb_str_cat2(str, ": ");
  str = rb_str_append(str, layout_inspect(
      self->descriptor->layout, Message_data(self)));
  str = rb_str_cat2(str, ">");
  return str;
}

/*
 * call-seq:
 *     Message.to_h => {}
 *
 * Returns the message as a Ruby Hash object, with keys as symbols.
 */
VALUE Message_to_h(VALUE _self) {
  MessageHeader* self;
  VALUE hash;
  upb_msg_field_iter it;
  TypedData_Get_Struct(_self, MessageHeader, &Message_type, self);

  hash = rb_hash_new();

  for (upb_msg_field_begin(&it, self->descriptor->msgdef);
       !upb_msg_field_done(&it);
       upb_msg_field_next(&it)) {
    const upb_fielddef* field = upb_msg_iter_field(&it);

    // For proto2, do not include fields which are not set.
    if (upb_msgdef_syntax(self->descriptor->msgdef) == UPB_SYNTAX_PROTO2 &&
	field_contains_hasbit(self->descriptor->layout, field) &&
	!layout_has(self->descriptor->layout, Message_data(self), field)) {
      continue;
    }

    VALUE msg_value = layout_get(self->descriptor->layout, Message_data(self),
                                 field);
    VALUE msg_key   = ID2SYM(rb_intern(upb_fielddef_name(field)));
    if (is_map_field(field)) {
      msg_value = Map_to_h(msg_value);
    } else if (upb_fielddef_label(field) == UPB_LABEL_REPEATED) {
      msg_value = RepeatedField_to_ary(msg_value);
      if (upb_msgdef_syntax(self->descriptor->msgdef) == UPB_SYNTAX_PROTO2 &&
          RARRAY_LEN(msg_value) == 0) {
        continue;
      }

      if (upb_fielddef_type(field) == UPB_TYPE_MESSAGE) {
        for (int i = 0; i < RARRAY_LEN(msg_value); i++) {
          VALUE elem = rb_ary_entry(msg_value, i);
          rb_ary_store(msg_value, i, Message_to_h(elem));
        }
      }

    } else if (msg_value != Qnil &&
               upb_fielddef_type(field) == UPB_TYPE_MESSAGE) {
      msg_value = Message_to_h(msg_value);
    }
    rb_hash_aset(hash, msg_key, msg_value);
  }
  return hash;
}



/*
 * call-seq:
 *     Message.[](index) => value
 *
 * Accesses a field's value by field name. The provided field name should be a
 * string.
 */
VALUE Message_index(VALUE _self, VALUE field_name) {
  MessageHeader* self;
  const upb_fielddef* field;
  TypedData_Get_Struct(_self, MessageHeader, &Message_type, self);
  Check_Type(field_name, T_STRING);
  field = upb_msgdef_ntofz(self->descriptor->msgdef, RSTRING_PTR(field_name));
  if (field == NULL) {
    return Qnil;
  }
  return layout_get(self->descriptor->layout, Message_data(self), field);
}

/*
 * call-seq:
 *     Message.[]=(index, value)
 *
 * Sets a field's value by field name. The provided field name should be a
 * string.
 */
VALUE Message_index_set(VALUE _self, VALUE field_name, VALUE value) {
  MessageHeader* self;
  const upb_fielddef* field;
  TypedData_Get_Struct(_self, MessageHeader, &Message_type, self);
  Check_Type(field_name, T_STRING);
  field = upb_msgdef_ntofz(self->descriptor->msgdef, RSTRING_PTR(field_name));
  if (field == NULL) {
    rb_raise(rb_eArgError, "Unknown field: %s", RSTRING_PTR(field_name));
  }
  layout_set(self->descriptor->layout, Message_data(self), field, value);
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

VALUE build_class_from_descriptor(Descriptor* desc) {
  const char *name;
  VALUE klass;

  if (desc->layout == NULL) {
    desc->layout = create_layout(desc->msgdef);
  }
  if (desc->fill_method == NULL) {
    desc->fill_method = new_fillmsg_decodermethod(desc, &desc->fill_method);
  }

  name = upb_msgdef_fullname(desc->msgdef);
  if (name == NULL) {
    rb_raise(rb_eRuntimeError, "Descriptor does not have assigned name.");
  }

  klass = rb_define_class_id(
      // Docs say this parameter is ignored. User will assign return value to
      // their own toplevel constant class name.
      rb_intern("Message"),
      rb_cObject);
  rb_ivar_set(klass, descriptor_instancevar_interned,
              get_def_obj(desc->msgdef));
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
  rb_define_method(klass, "hash", Message_hash, 0);
  rb_define_method(klass, "to_h", Message_to_h, 0);
  rb_define_method(klass, "to_hash", Message_to_h, 0);
  rb_define_method(klass, "inspect", Message_inspect, 0);
  rb_define_method(klass, "[]", Message_index, 1);
  rb_define_method(klass, "[]=", Message_index_set, 2);
  rb_define_singleton_method(klass, "decode", Message_decode, 1);
  rb_define_singleton_method(klass, "encode", Message_encode, 1);
  rb_define_singleton_method(klass, "decode_json", Message_decode_json, 1);
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

VALUE build_module_from_enumdesc(EnumDescriptor* enumdesc) {
  VALUE mod = rb_define_module_id(
      rb_intern(upb_enumdef_fullname(enumdesc->enumdef)));

  upb_enum_iter it;
  for (upb_enum_begin(&it, enumdesc->enumdef);
       !upb_enum_done(&it);
       upb_enum_next(&it)) {
    const char* name = upb_enum_iter_name(&it);
    int32_t value = upb_enum_iter_number(&it);
    if (name[0] < 'A' || name[0] > 'Z') {
      rb_raise(cTypeError,
               "Enum value '%s' does not start with an uppercase letter "
               "as is required for Ruby constants.",
               name);
    }
    rb_define_const(mod, name, INT2NUM(value));
  }

  rb_define_singleton_method(mod, "lookup", enum_lookup, 1);
  rb_define_singleton_method(mod, "resolve", enum_resolve, 1);
  rb_define_singleton_method(mod, "descriptor", enum_descriptor, 0);
  rb_ivar_set(mod, descriptor_instancevar_interned,
              get_def_obj(enumdesc->enumdef));

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
