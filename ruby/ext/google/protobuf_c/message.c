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
  MessageHeader* msg;
  VALUE ret;
  size_t size;

  if (desc->layout == NULL) {
    create_layout(desc);
  }

  msg = ALLOC_N(uint8_t, sizeof(MessageHeader) + desc->layout->size);
  msg->descriptor = desc;
  msg->unknown_fields = NULL;
  memcpy(Message_data(msg), desc->layout->empty_template, desc->layout->size);

  ret = TypedData_Wrap_Struct(klass, &Message_type, msg);
  rb_ivar_set(ret, descriptor_instancevar_interned, descriptor);

  return ret;
}

static const upb_fielddef* which_oneof_field(MessageHeader* self, const upb_oneofdef* o) {
  uint32_t oneof_case;
  const upb_fielddef* f;

  oneof_case =
      slot_read_oneof_case(self->descriptor->layout, Message_data(self), o);

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
  METHOD_PRESENCE = 4,
  METHOD_ENUM_GETTER = 5,
  METHOD_WRAPPER_GETTER = 6,
  METHOD_WRAPPER_SETTER = 7
};

// Check if the field is a well known wrapper type
static bool is_wrapper_type_field(const MessageLayout* layout,
                                  const upb_fielddef* field) {
  const char* field_type_name = rb_class2name(field_type_class(layout, field));

  return strcmp(field_type_name, "Google::Protobuf::DoubleValue") == 0 ||
         strcmp(field_type_name, "Google::Protobuf::FloatValue") == 0 ||
         strcmp(field_type_name, "Google::Protobuf::Int32Value") == 0 ||
         strcmp(field_type_name, "Google::Protobuf::Int64Value") == 0 ||
         strcmp(field_type_name, "Google::Protobuf::UInt32Value") == 0 ||
         strcmp(field_type_name, "Google::Protobuf::UInt64Value") == 0 ||
         strcmp(field_type_name, "Google::Protobuf::BoolValue") == 0 ||
         strcmp(field_type_name, "Google::Protobuf::StringValue") == 0 ||
         strcmp(field_type_name, "Google::Protobuf::BytesValue") == 0;
}

// Get a new Ruby wrapper type and set the initial value
static VALUE ruby_wrapper_type(const MessageLayout* layout,
                               const upb_fielddef* field, const VALUE value) {
  if (is_wrapper_type_field(layout, field) && value != Qnil) {
    VALUE hash = rb_hash_new();
    rb_hash_aset(hash, rb_str_new2("value"), value);
    {
      VALUE args[1] = {hash};
      return rb_class_new_instance(1, args, field_type_class(layout, field));
    }
  }
  return Qnil;
}

static int extract_method_call(VALUE method_name, MessageHeader* self,
                            const upb_fielddef **f, const upb_oneofdef **o) {
  VALUE method_str;
  char* name;
  size_t name_len;
  int accessor_type;
  const upb_oneofdef* test_o;
  const upb_fielddef* test_f;
  bool has_field;

  Check_Type(method_name, T_SYMBOL);

  method_str = rb_id2str(SYM2ID(method_name));
  name = RSTRING_PTR(method_str);
  name_len = RSTRING_LEN(method_str);

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

  has_field = upb_msgdef_lookupname(self->descriptor->msgdef, name, name_len,
                                    &test_f, &test_o);

  // Look for wrapper type accessor of the form <field_name>_as_value
  if (!has_field &&
      (accessor_type == METHOD_GETTER || accessor_type == METHOD_SETTER) &&
      name_len > 9 && strncmp(name + name_len - 9, "_as_value", 9) == 0) {
    const upb_oneofdef* test_o_wrapper;
    const upb_fielddef* test_f_wrapper;
    char wrapper_field_name[name_len - 8];

    // Find the field name
    strncpy(wrapper_field_name, name, name_len - 9);
    wrapper_field_name[name_len - 9] = '\0';

    // Check if field exists and is a wrapper type
    if (upb_msgdef_lookupname(self->descriptor->msgdef, wrapper_field_name,
                              name_len - 9, &test_f_wrapper, &test_o_wrapper) &&
        upb_fielddef_type(test_f_wrapper) == UPB_TYPE_MESSAGE &&
        is_wrapper_type_field(self->descriptor->layout, test_f_wrapper)) {
      // It does exist!
      has_field = true;
      if (accessor_type == METHOD_SETTER) {
        accessor_type = METHOD_WRAPPER_SETTER;
      } else {
        accessor_type = METHOD_WRAPPER_GETTER;
      }
      test_o = test_o_wrapper;
      test_f = test_f_wrapper;
    }
  }

  // Look for enum accessor of the form <enum_name>_const
  if (!has_field && accessor_type == METHOD_GETTER &&
      name_len > 6 && strncmp(name + name_len - 6, "_const", 6) == 0) {
    const upb_oneofdef* test_o_enum;
    const upb_fielddef* test_f_enum;
    char enum_name[name_len - 5];

    // Find enum field name
    strncpy(enum_name, name, name_len - 6);
    enum_name[name_len - 6] = '\0';

    // Check if enum field exists
    if (upb_msgdef_lookupname(self->descriptor->msgdef, enum_name, name_len - 6,
                                             &test_f_enum, &test_o_enum) &&
        upb_fielddef_type(test_f_enum) == UPB_TYPE_ENUM) {
      // It does exist!
      has_field = true;
      accessor_type = METHOD_ENUM_GETTER;
      test_o = test_o_enum;
      test_f = test_f_enum;
    }
  }

  // Verify the name corresponds to a oneof or field in this message.
  if (!has_field) {
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
  int accessor_type;

  TypedData_Get_Struct(_self, MessageHeader, &Message_type, self);
  if (argc < 1) {
    rb_raise(rb_eArgError, "Expected method name as first argument.");
  }

  accessor_type = extract_method_call(argv[0], self, &f, &o);
  if (accessor_type == METHOD_UNKNOWN || (o == NULL && f == NULL) ) {
    return rb_call_super(argc, argv);
  } else if (accessor_type == METHOD_SETTER || accessor_type == METHOD_WRAPPER_SETTER) {
    if (argc != 2) {
      rb_raise(rb_eArgError, "Expected 2 arguments, received %d", argc);
    }
    rb_check_frozen(_self);
  } else if (argc != 1) {
    rb_raise(rb_eArgError, "Expected 1 argument, received %d", argc);
  }

  // Return which of the oneof fields are set
  if (o != NULL) {
    const upb_fielddef* oneof_field = which_oneof_field(self, o);

    if (accessor_type == METHOD_SETTER) {
      rb_raise(rb_eRuntimeError, "Oneof accessors are read-only.");
    }

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
  } else if (accessor_type == METHOD_WRAPPER_GETTER) {
    VALUE value = layout_get(self->descriptor->layout, Message_data(self), f);
    if (value != Qnil) {
      value = rb_funcall(value, rb_intern("value"), 0);
    }
    return value;
  } else if (accessor_type == METHOD_WRAPPER_SETTER) {
    VALUE wrapper = ruby_wrapper_type(self->descriptor->layout, f, argv[1]);
    layout_set(self->descriptor->layout, Message_data(self), f, wrapper);
    return Qnil;
  } else if (accessor_type == METHOD_ENUM_GETTER) {
    VALUE enum_type = field_type_class(self->descriptor->layout, f);
    VALUE method = rb_intern("const_get");
    VALUE raw_value = layout_get(self->descriptor->layout, Message_data(self), f);

    // Map repeated fields to a new type with ints
    if (upb_fielddef_label(f) == UPB_LABEL_REPEATED) {
      int array_size = FIX2INT(rb_funcall(raw_value, rb_intern("length"), 0));
      int i;
      VALUE array_args[1] = { ID2SYM(rb_intern("int64")) };
      VALUE array = rb_class_new_instance(1, array_args, CLASS_OF(raw_value));
      for (i = 0; i < array_size; i++) {
        VALUE entry = rb_funcall(enum_type, method, 1, rb_funcall(raw_value,
                                 rb_intern("at"), 1, INT2NUM(i)));
        rb_funcall(array, rb_intern("push"), 1, entry);
      }
      return array;
    }
    // Convert the value for singular fields
    return rb_funcall(enum_type, method, 1, raw_value);
  } else {
    return layout_get(self->descriptor->layout, Message_data(self), f);
  }
}


VALUE Message_respond_to_missing(int argc, VALUE* argv, VALUE _self) {
  MessageHeader* self;
  const upb_oneofdef* o;
  const upb_fielddef* f;
  int accessor_type;

  TypedData_Get_Struct(_self, MessageHeader, &Message_type, self);
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

VALUE create_submsg_from_hash(const MessageLayout* layout,
                              const upb_fielddef* f, VALUE hash) {
  VALUE args[1] = { hash };
  return rb_class_new_instance(1, args, field_type_class(layout, f));
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
               "Expected Hash object as initializer value for map field '%s' (given %s).",
               name, rb_class2name(CLASS_OF(val)));
    }
    map = layout_get(self->descriptor->layout, Message_data(self), f);
    Map_merge_into_self(map, val);
  } else if (upb_fielddef_label(f) == UPB_LABEL_REPEATED) {
    VALUE ary;
    int i;

    if (TYPE(val) != T_ARRAY) {
      rb_raise(rb_eArgError,
               "Expected array as initializer value for repeated field '%s' (given %s).",
               name, rb_class2name(CLASS_OF(val)));
    }
    ary = layout_get(self->descriptor->layout, Message_data(self), f);
    for (i = 0; i < RARRAY_LEN(val); i++) {
      VALUE entry = rb_ary_entry(val, i);
      if (TYPE(entry) == T_HASH && upb_fielddef_issubmsg(f)) {
        entry = create_submsg_from_hash(self->descriptor->layout, f, entry);
      }

      RepeatedField_push(ary, entry);
    }
  } else {
    if (TYPE(val) == T_HASH && upb_fielddef_issubmsg(f)) {
      val = create_submsg_from_hash(self->descriptor->layout, f, val);
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
  MessageHeader* self;
  VALUE hash_args;
  TypedData_Get_Struct(_self, MessageHeader, &Message_type, self);

  layout_init(self->descriptor->layout, Message_data(self));

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
    VALUE msg_value;
    VALUE msg_key;

    // For proto2, do not include fields which are not set.
    if (upb_msgdef_syntax(self->descriptor->msgdef) == UPB_SYNTAX_PROTO2 &&
       field_contains_hasbit(self->descriptor->layout, field) &&
       !layout_has(self->descriptor->layout, Message_data(self), field)) {
      continue;
    }

    msg_value = layout_get(self->descriptor->layout, Message_data(self), field);
    msg_key = ID2SYM(rb_intern(upb_fielddef_name(field)));
    if (is_map_field(field)) {
      msg_value = Map_to_h(msg_value);
    } else if (upb_fielddef_label(field) == UPB_LABEL_REPEATED) {
      msg_value = RepeatedField_to_ary(msg_value);
      if (upb_msgdef_syntax(self->descriptor->msgdef) == UPB_SYNTAX_PROTO2 &&
          RARRAY_LEN(msg_value) == 0) {
        continue;
      }

      if (upb_fielddef_type(field) == UPB_TYPE_MESSAGE) {
        int i;
        for (i = 0; i < RARRAY_LEN(msg_value); i++) {
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
