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

#include "repeated_field.h"

#include "convert.h"
#include "defs.h"
#include "protobuf.h"

// -----------------------------------------------------------------------------
// Repeated field container type.
// -----------------------------------------------------------------------------

// Mark, free, alloc, init and class setup functions.

static void RepeatedField_mark(void* _self) {
  RepeatedField* self = (RepeatedField*)_self;
  rb_gc_mark(self->type_class);
  rb_gc_mark(self->arena);
}

static void RepeatedField_free(void* _self) {
  RepeatedField* self = (RepeatedField *)_self;
  ObjectCache_Remove(self->array);
}

const rb_data_type_t RepeatedField_type = {
  "Google::Protobuf::RepeatedField",
  { RepeatedField_mark, RepeatedField_free, NULL },
};

VALUE cRepeatedField;

VALUE RepeatedField_alloc(VALUE klass) {
  RepeatedField* self = ALLOC(RepeatedField);
  self->arena = Qnil;
  self->type_class = Qnil;
  self->array = NULL;
  return TypedData_Wrap_Struct(klass, &RepeatedField_type, self);
}

VALUE RepeatedField_GetRubyWrapper(upb_array* array, const upb_fielddef* f,
                                   VALUE arena) {
  VALUE val = ObjectCache_Get(array);

  if (val == Qnil) {
    val = RepeatedField_alloc(cRepeatedField);
    RepeatedField* self;
    ObjectCache_Add(array, val);
    TypedData_Get_Struct(val, RepeatedField, &RepeatedField_type, self);
    self->array = array;
    self->arena = arena;
    self->type_info = TypeInfo_get(f);
    if (self->type_info.type == UPB_TYPE_MESSAGE) {
      const upb_msgdef *m = upb_fielddef_msgsubdef(f);
      self->type_class = Descriptor_DefToClass(m);
    }
  }

  return val;
}

void RepeatedField_Inspect(StringBuilder* b, const upb_array* array,
                           TypeInfo info) {
  bool first = true;
  StringBuilder_Printf(b, "[");
  size_t n = array ? upb_array_size(array) : 0;
  for (size_t i = 0; i < n; i++) {
    if (first) {
      first = false;
    } else {
      StringBuilder_Printf(b, ", ");
    }
    StringBuilder_PrintMsgval(b, upb_array_get(array, i), info);
  }
}

static RepeatedField* ruby_to_RepeatedField(VALUE _self) {
  RepeatedField* self;
  TypedData_Get_Struct(_self, RepeatedField, &RepeatedField_type, self);
  return self;
}

upb_array* RepeatedField_GetUpbArray(VALUE val, const upb_fielddef *field) {
  RepeatedField* self;
  TypeInfo type_info = TypeInfo_get(field);

  if (!RB_TYPE_P(val, T_DATA) || !RTYPEDDATA_P(val) ||
      RTYPEDDATA_TYPE(val) != &RepeatedField_type) {
    rb_raise(cTypeError, "Expected repeated field array");
  }

  self = ruby_to_RepeatedField(val);
  if (self->type_info.type != type_info.type) {
    rb_raise(cTypeError, "Repeated field array has wrong element type");
  }

  if (self->type_info.def.msgdef != type_info.def.msgdef) {
    rb_raise(cTypeError, "Repeated field array has wrong message/enum class");
  }

  return self->array;
}

static int index_position(VALUE _index, RepeatedField* repeated_field) {
  int index = NUM2INT(_index);
  int size = upb_array_size(repeated_field->array);
  if (index < 0 && size > 0) {
    index = size + index;
  }

  return index >= 0 && index < size ? index : -1;
}

static VALUE RepeatedField_subarray(RepeatedField* self, long beg, long len) {
  size_t size = upb_array_size(self->array);
  VALUE ary = rb_ary_new2(size);
  long i;

  for (i = beg; i < beg + len; i++) {
    upb_msgval msgval = upb_array_get(self->array, i);
    VALUE elem = Convert_UpbToRuby(msgval, self->type_info, self->arena);
    rb_ary_push(ary, elem);
  }
  return ary;
}

/*
 * call-seq:
 *     RepeatedField.each(&block)
 *
 * Invokes the block once for each element of the repeated field. RepeatedField
 * also includes Enumerable; combined with this method, the repeated field thus
 * acts like an ordinary Ruby sequence.
 */
static VALUE RepeatedField_each(VALUE _self) {
  RepeatedField* self = ruby_to_RepeatedField(_self);
  int size = upb_array_size(self->array);
  int i;

  for (i = 0; i < size; i++) {
    upb_msgval msgval = upb_array_get(self->array, i);
    VALUE val = Convert_UpbToRuby(msgval, self->type_info, self->arena);
    rb_yield(val);
  }
  return _self;
}


/*
 * call-seq:
 *     RepeatedField.[](index) => value
 *
 * Accesses the element at the given index. Returns nil on out-of-bounds
 */
static VALUE RepeatedField_index(int argc, VALUE* argv, VALUE _self) {
  RepeatedField* self = ruby_to_RepeatedField(_self);
  long size = upb_array_size(self->array);

  VALUE arg = argv[0];
  long beg, len;

  if (argc == 1){
    if (FIXNUM_P(arg)) {
      /* standard case */
      upb_msgval msgval;
      int index = index_position(argv[0], self);
      if (index < 0) return Qnil;
      msgval = upb_array_get(self->array, index);
      return Convert_UpbToRuby(msgval, self->type_info, self->arena);
    } else {
      /* check if idx is Range */
      switch (rb_range_beg_len(arg, &beg, &len, size, 0)) {
        case Qfalse:
          break;
        case Qnil:
          return Qnil;
        default:
          return RepeatedField_subarray(self, beg, len);
      }
    }
  }

  /* assume 2 arguments */
  beg = NUM2LONG(argv[0]);
  len = NUM2LONG(argv[1]);
  if (beg < 0) {
    beg += size;
  }
  if (beg >= size) {
    return Qnil;
  }
  return RepeatedField_subarray(self, beg, len);
}

/*
 * call-seq:
 *     RepeatedField.[]=(index, value)
 *
 * Sets the element at the given index. On out-of-bounds assignments, extends
 * the array and fills the hole (if any) with default values.
 */
static VALUE RepeatedField_index_set(VALUE _self, VALUE _index, VALUE val) {
  RepeatedField* self = ruby_to_RepeatedField(_self);
  int size = upb_array_size(self->array);
  upb_arena *arena = Arena_get(self->arena);
  upb_msgval msgval = Convert_RubyToUpb(val, "", self->type_info, arena);

  int index = index_position(_index, self);
  if (index < 0 || index >= (INT_MAX - 1)) {
    return Qnil;
  }

  if (index >= size) {
    upb_array_resize(self->array, index, arena);
  }

  upb_array_append(self->array, msgval, arena);
  return Qnil;
}

/*
 * call-seq:
 *     RepeatedField.push(value, ...)
 *
 * Adds a new element to the repeated field.
 */
static VALUE RepeatedField_push_vararg(int argc, VALUE* argv, VALUE _self) {
  RepeatedField* self = ruby_to_RepeatedField(_self);
  upb_arena *arena = Arena_get(self->arena);
  int i;

  for (i = 0; i < argc; i++) {
    upb_msgval msgval = Convert_RubyToUpb(argv[i], "", self->type_info, arena);
    upb_array_append(self->array, msgval, arena);
  }

  return _self;
}

/*
 * call-seq:
 *     RepeatedField.<<(value)
 *
 * Adds a new element to the repeated field.
 */
static VALUE RepeatedField_push(VALUE _self, VALUE val) {
  RepeatedField* self = ruby_to_RepeatedField(_self);
  upb_arena *arena = Arena_get(self->arena);

  upb_msgval msgval = Convert_RubyToUpb(val, "", self->type_info, arena);
  upb_array_append(self->array, msgval, arena);

  return _self;
}

/*
 * Private ruby method, used by RepeatedField.pop
 */
static VALUE RepeatedField_pop_one(VALUE _self) {
  RepeatedField* self = ruby_to_RepeatedField(_self);
  size_t size = upb_array_size(self->array);
  upb_msgval last;
  VALUE ret;

  if (size == 0) {
    return Qnil;
  }

  last = upb_array_get(self->array, size - 1);
  ret = Convert_UpbToRuby(last, self->type_info, self->arena);

  upb_array_resize(self->array, size - 1, Arena_get(self->arena));
  return ret;
}

/*
 * call-seq:
 *     RepeatedField.replace(list)
 *
 * Replaces the contents of the repeated field with the given list of elements.
 */
static VALUE RepeatedField_replace(VALUE _self, VALUE list) {
  RepeatedField* self = ruby_to_RepeatedField(_self);
  int i;

  Check_Type(list, T_ARRAY);
  upb_array_resize(self->array, 0, Arena_get(self->arena));

  for (i = 0; i < RARRAY_LEN(list); i++) {
    RepeatedField_push(_self, rb_ary_entry(list, i));
  }

  return list;
}

/*
 * call-seq:
 *     RepeatedField.clear
 *
 * Clears (removes all elements from) this repeated field.
 */
static VALUE RepeatedField_clear(VALUE _self) {
  RepeatedField* self = ruby_to_RepeatedField(_self);
  upb_array_resize(self->array, 0, Arena_get(self->arena));
  return _self;
}

/*
 * call-seq:
 *     RepeatedField.length
 *
 * Returns the length of this repeated field.
 */
static VALUE RepeatedField_length(VALUE _self) {
  RepeatedField* self = ruby_to_RepeatedField(_self);
  return INT2NUM(upb_array_size(self->array));
}

static VALUE RepeatedField_new_this_type(VALUE _self) {
  RepeatedField* self = ruby_to_RepeatedField(_self);
  VALUE new_rptfield = Qnil;
  VALUE element_type = fieldtype_to_ruby(self->type_info.type);
  if (self->type_class != Qnil) {
    new_rptfield = rb_funcall(CLASS_OF(_self), rb_intern("new"), 2,
                              element_type, self->type_class);
  } else {
    new_rptfield = rb_funcall(CLASS_OF(_self), rb_intern("new"), 1,
                              element_type);
  }
  return new_rptfield;
}

/*
 * call-seq:
 *     RepeatedField.dup => repeated_field
 *
 * Duplicates this repeated field with a shallow copy. References to all
 * non-primitive element objects (e.g., submessages) are shared.
 */
static VALUE RepeatedField_dup(VALUE _self) {
  RepeatedField* self = ruby_to_RepeatedField(_self);
  VALUE new_rptfield = RepeatedField_new_this_type(_self);
  RepeatedField* new_rptfield_self = ruby_to_RepeatedField(new_rptfield);
  upb_arena* arena = Arena_get(new_rptfield_self->arena);
  int size = upb_array_size(self->array);
  int i;

  for (i = 0; i < size; i++) {
    upb_msgval msgval = upb_array_get(self->array, i);
    upb_array_append(new_rptfield_self->array, msgval, arena);
  }

  return new_rptfield;
}

/*
 * call-seq:
 *     RepeatedField.to_ary => array
 *
 * Used when converted implicitly into array, e.g. compared to an Array.
 * Also called as a fallback of Object#to_a
 */
VALUE RepeatedField_to_ary(VALUE _self) {
  RepeatedField* self = ruby_to_RepeatedField(_self);
  int size = upb_array_size(self->array);
  VALUE ary = rb_ary_new2(size);
  int i;

  for (i = 0; i < size; i++) {
    upb_msgval msgval = upb_array_get(self->array, i);
    VALUE val = Convert_UpbToRuby(msgval, self->type_info, self->arena);
    rb_ary_push(ary, val);
  }

  return ary;
}

/*
 * call-seq:
 *     RepeatedField.==(other) => boolean
 *
 * Compares this repeated field to another. Repeated fields are equal if their
 * element types are equal, their lengths are equal, and each element is equal.
 * Elements are compared as per normal Ruby semantics, by calling their :==
 * methods (or performing a more efficient comparison for primitive types).
 *
 * Repeated fields with dissimilar element types are never equal, even if value
 * comparison (for example, between integers and floats) would have otherwise
 * indicated that every element has equal value.
 */
VALUE RepeatedField_eq(VALUE _self, VALUE _other) {
  RepeatedField* self;
  RepeatedField* other;

  if (_self == _other) {
    return Qtrue;
  }

  if (TYPE(_other) == T_ARRAY) {
    VALUE self_ary = RepeatedField_to_ary(_self);
    return rb_equal(self_ary, _other);
  }

  rb_raise(rb_eRuntimeError, "NYI: RepeatedField_eq");
#if 0
  self = ruby_to_RepeatedField(_self);
  other = ruby_to_RepeatedField(_other);
  if (self->type_info.type != other->type_info.type ||
      self->type_class != other->type_class ||
      upb_array_size(self->array) != upb_array_size(other->array)) {
    return Qfalse;
  }

  {
    upb_fieldtype_t field_type = self->field_type;
    size_t elem_size = native_slot_size(field_type);
    size_t off = 0;
    int i;

    for (i = 0; i < self->size; i++, off += elem_size) {
      void* self_mem = ((uint8_t *)self->elements) + off;
      void* other_mem = ((uint8_t *)other->elements) + off;
      if (!native_slot_eq(field_type, self->field_type_class, self_mem,
                          other_mem)) {
        return Qfalse;
      }
    }
    return Qtrue;
  }
#endif
}

/*
 * call-seq:
 *     RepeatedField.hash => hash_value
 *
 * Returns a hash value computed from this repeated field's elements.
 */
VALUE RepeatedField_hash(VALUE _self) {
  #if 0
  RepeatedField* self = ruby_to_RepeatedField(_self);
  st_index_t h = rb_hash_start(0);
  VALUE hash_sym = rb_intern("hash");
  upb_fieldtype_t field_type = self->field_type;
  VALUE field_type_class = self->field_type_class;
  size_t elem_size = native_slot_size(field_type);
  size_t off = 0;
  int i;

  for (i = 0; i < self->size; i++, off += elem_size) {
    void* mem = ((uint8_t *)self->elements) + off;
    VALUE elem = native_slot_get(field_type, field_type_class, mem);
    h = rb_hash_uint(h, NUM2LONG(rb_funcall(elem, hash_sym, 0)));
  }
  h = rb_hash_end(h);

  return INT2FIX(h);
  #endif
  rb_raise(rb_eRuntimeError, "NYI: RepeatedField_hash");
}

/*
 * call-seq:
 *     RepeatedField.+(other) => repeated field
 *
 * Returns a new repeated field that contains the concatenated list of this
 * repeated field's elements and other's elements. The other (second) list may
 * be either another repeated field or a Ruby array.
 */
VALUE RepeatedField_plus(VALUE _self, VALUE list) {
  VALUE dupped_ = RepeatedField_dup(_self);

  if (TYPE(list) == T_ARRAY) {
    int i;
    for (i = 0; i < RARRAY_LEN(list); i++) {
      VALUE elem = rb_ary_entry(list, i);
      RepeatedField_push(dupped_, elem);
    }
  } else if (RB_TYPE_P(list, T_DATA) && RTYPEDDATA_P(list) &&
             RTYPEDDATA_TYPE(list) == &RepeatedField_type) {
    RepeatedField* self = ruby_to_RepeatedField(_self);
    RepeatedField* list_rptfield = ruby_to_RepeatedField(list);
    RepeatedField* dupped = ruby_to_RepeatedField(dupped_);
    upb_arena* arena = Arena_get(dupped->arena);
    int size = upb_array_size(list_rptfield->array);
    int i;

    if (self->type_info.type != list_rptfield->type_info.type ||
        self->type_class != list_rptfield->type_class) {
      rb_raise(rb_eArgError,
               "Attempt to append RepeatedField with different element type.");
    }

    for (i = 0; i < size; i++) {
      upb_msgval msgval = upb_array_get(list_rptfield->array, i);
      upb_array_append(dupped->array, msgval, arena);
    }
  } else {
    rb_raise(rb_eArgError, "Unknown type appending to RepeatedField");
  }

  return dupped_;
}

/*
 * call-seq:
 *     RepeatedField.concat(other) => self
 *
 * concats the passed in array to self.  Returns a Ruby array.
 */
VALUE RepeatedField_concat(VALUE _self, VALUE list) {
  int i;

  Check_Type(list, T_ARRAY);
  for (i = 0; i < RARRAY_LEN(list); i++) {
    RepeatedField_push(_self, rb_ary_entry(list, i));
  }
  return _self;
}

/*
 * call-seq:
 *     RepeatedField.new(type, type_class = nil, initial_elems = [])
 *
 * Creates a new repeated field. The provided type must be a Ruby symbol, and
 * can take on the same values as those accepted by FieldDescriptor#type=. If
 * the type is :message or :enum, type_class must be non-nil, and must be the
 * Ruby class or module returned by Descriptor#msgclass or
 * EnumDescriptor#enummodule, respectively. An initial list of elements may also
 * be provided.
 */
VALUE RepeatedField_init(int argc, VALUE* argv, VALUE _self) {
  RepeatedField* self = ruby_to_RepeatedField(_self);
  upb_arena *arena;
  VALUE ary = Qnil;

  self->arena = Arena_new();
  arena = Arena_get(self->arena);

  if (argc < 1) {
    rb_raise(rb_eArgError, "Expected at least 1 argument.");
  }

  self->type_info.type = ruby_to_fieldtype(argv[0]);
  self->type_info.def.msgdef = NULL;
  self->array = upb_array_new(arena, self->type_info.type);

  if (self->type_info.type == UPB_TYPE_MESSAGE ||
      self->type_info.type == UPB_TYPE_ENUM) {
    if (argc < 2) {
      rb_raise(rb_eArgError, "Expected at least 2 arguments for message/enum.");
    }
    self->type_class = argv[1];
    if (argc > 2) {
      ary = argv[2];
    }
    validate_type_class(self->type_info.type, self->type_class);
    if (self->type_info.type == UPB_TYPE_MESSAGE) {
      const Descriptor* desc = ruby_to_Descriptor(self->type_class);
      self->type_info.def.msgdef = desc->msgdef;
    } else {
      const EnumDescriptor* desc = ruby_to_EnumDescriptor(self->type_class);
      self->type_info.def.enumdef = desc->enumdef;
    }
  } else {
    if (argc > 2) {
      rb_raise(rb_eArgError, "Too many arguments: expected 1 or 2.");
    }
    if (argc > 1) {
      ary = argv[1];
    }
  }

  if (ary != Qnil) {
    int i;

    if (!RB_TYPE_P(ary, T_ARRAY)) {
      rb_raise(rb_eArgError, "Expected array as initialize argument");
    }
    for (i = 0; i < RARRAY_LEN(ary); i++) {
      RepeatedField_push(_self, rb_ary_entry(ary, i));
    }
  }
  return Qnil;
}

void RepeatedField_register(VALUE module) {
  VALUE klass = rb_define_class_under(
      module, "RepeatedField", rb_cObject);
  rb_define_alloc_func(klass, RepeatedField_alloc);
  rb_gc_register_address(&cRepeatedField);
  cRepeatedField = klass;

  rb_define_method(klass, "initialize",
                   RepeatedField_init, -1);
  rb_define_method(klass, "each", RepeatedField_each, 0);
  rb_define_method(klass, "[]", RepeatedField_index, -1);
  rb_define_method(klass, "at", RepeatedField_index, -1);
  rb_define_method(klass, "[]=", RepeatedField_index_set, 2);
  rb_define_method(klass, "push", RepeatedField_push_vararg, -1);
  rb_define_method(klass, "<<", RepeatedField_push, 1);
  rb_define_private_method(klass, "pop_one", RepeatedField_pop_one, 0);
  rb_define_method(klass, "replace", RepeatedField_replace, 1);
  rb_define_method(klass, "clear", RepeatedField_clear, 0);
  rb_define_method(klass, "length", RepeatedField_length, 0);
  rb_define_method(klass, "size", RepeatedField_length, 0);
  rb_define_method(klass, "dup", RepeatedField_dup, 0);
  // Also define #clone so that we don't inherit Object#clone.
  rb_define_method(klass, "clone", RepeatedField_dup, 0);
  rb_define_method(klass, "==", RepeatedField_eq, 1);
  rb_define_method(klass, "to_ary", RepeatedField_to_ary, 0);
  rb_define_method(klass, "hash", RepeatedField_hash, 0);
  rb_define_method(klass, "+", RepeatedField_plus, 1);
  rb_define_method(klass, "concat", RepeatedField_concat, 1);
  rb_include_module(klass, rb_mEnumerable);
}
