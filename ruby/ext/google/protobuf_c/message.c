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

  layout_init(desc->layout, Message_data(msg));

  return ret;
}

static VALUE which_oneof_field(MessageHeader* self, const upb_oneofdef* o) {
  upb_oneof_iter it;
  size_t case_ofs;
  uint32_t oneof_case;
  const upb_fielddef* first_field;
  const upb_fielddef* f;

  // If no fields in the oneof, always nil.
  if (upb_oneofdef_numfields(o) == 0) {
    return Qnil;
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
    return Qnil;
  }

  // oneof_case is a field index, so find that field.
  f = upb_oneofdef_itof(o, oneof_case);
  assert(f != NULL);

  return ID2SYM(rb_intern(upb_fielddef_name(f)));
}

/*
 * call-seq:
 *     Message.method_missing(*args)
 *
 * Provides accessors and setters for message fields according to their field
 * names. For any field whose name does not conflict with a built-in method, an
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
 */
VALUE Message_method_missing(int argc, VALUE* argv, VALUE _self) {
  MessageHeader* self;
  VALUE method_name, method_str;
  char* name;
  size_t name_len;
  bool setter;
  const upb_oneofdef* o;
  const upb_fielddef* f;

  TypedData_Get_Struct(_self, MessageHeader, &Message_type, self);
  if (argc < 1) {
    rb_raise(rb_eArgError, "Expected method name as first argument.");
  }
  method_name = argv[0];
  if (!SYMBOL_P(method_name)) {
    rb_raise(rb_eArgError, "Expected symbol as method name.");
  }
  method_str = rb_id2str(SYM2ID(method_name));
  name = RSTRING_PTR(method_str);
  name_len = RSTRING_LEN(method_str);
  setter = false;

  // Setters have names that end in '='.
  if (name[name_len - 1] == '=') {
    setter = true;
    name_len--;
  }

  // Check for a oneof name first.
  o = upb_msgdef_ntoo(self->descriptor->msgdef,
                                          name, name_len);
  if (o != NULL) {
    if (setter) {
      rb_raise(rb_eRuntimeError, "Oneof accessors are read-only.");
    }
    return which_oneof_field(self, o);
  }

  // Otherwise, check for a field with that name.
  f = upb_msgdef_ntof(self->descriptor->msgdef,
                                          name, name_len);

  if (f == NULL) {
    return rb_call_super(argc, argv);
  }

  if (setter) {
    if (argc < 2) {
      rb_raise(rb_eArgError, "No value provided to setter.");
    }
    layout_set(self->descriptor->layout, Message_data(self), f, argv[1]);
    return Qnil;
  } else {
    return layout_get(self->descriptor->layout, Message_data(self), f);
  }
}

int Message_initialize_kwarg(VALUE key, VALUE val, VALUE _self) {
  MessageHeader* self;
  VALUE method_str;
  char* name;
  const upb_fielddef* f;
  TypedData_Get_Struct(_self, MessageHeader, &Message_type, self);

  if (!SYMBOL_P(key)) {
    rb_raise(rb_eArgError,
             "Expected symbols as hash keys in initialization map.");
  }

  method_str = rb_id2str(SYM2ID(key));
  name = RSTRING_PTR(method_str);
  f = upb_msgdef_ntofz(self->descriptor->msgdef, name);
  if (f == NULL) {
    rb_raise(rb_eArgError,
             "Unknown field name '%s' in initialization map entry.", name);
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
      RepeatedField_push(ary, rb_ary_entry(val, i));
    }
  } else {
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
    VALUE msg_value = layout_get(self->descriptor->layout, Message_data(self),
                                 field);
    VALUE msg_key   = ID2SYM(rb_intern(upb_fielddef_name(field)));
    if (upb_fielddef_label(field) == UPB_LABEL_REPEATED) {
      msg_value = RepeatedField_to_ary(msg_value);
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
  rb_include_module(klass, rb_eval_string("Google::Protobuf::MessageExts"));
  rb_extend_object(
      klass, rb_eval_string("Google::Protobuf::MessageExts::ClassMethods"));

  rb_define_method(klass, "method_missing",
                   Message_method_missing, -1);
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
  rb_define_singleton_method(klass, "encode_json", Message_encode_json, 1);
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
      rb_raise(rb_eTypeError,
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
