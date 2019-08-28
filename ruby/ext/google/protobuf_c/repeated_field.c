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
// Repeated field container type.
// -----------------------------------------------------------------------------

const rb_data_type_t RepeatedField_type = {
  "Google::Protobuf::RepeatedField",
  { RepeatedField_mark, RepeatedField_free, NULL },
};

VALUE cRepeatedField;

RepeatedField* ruby_to_RepeatedField(VALUE _self) {
  RepeatedField* self;
  TypedData_Get_Struct(_self, RepeatedField, &RepeatedField_type, self);
  return self;
}

void* RepeatedField_memoryat(RepeatedField* self, int index, int element_size) {
  return ((uint8_t *)self->elements) + index * element_size;
}

static int index_position(VALUE _index, RepeatedField* repeated_field) {
  int index = NUM2INT(_index);
  if (index < 0 && repeated_field->size > 0) {
    index = repeated_field->size + index;
  }
  return index;
}

VALUE RepeatedField_subarray(VALUE _self, long beg, long len) {
  RepeatedField* self = ruby_to_RepeatedField(_self);
  int element_size = native_slot_size(self->field_type);
  upb_fieldtype_t field_type = self->field_type;
  VALUE field_type_class = self->field_type_class;
  size_t off = beg * element_size;
  VALUE ary = rb_ary_new2(len);
  int i;

  for (i = beg; i < beg + len; i++, off += element_size) {
    void* mem = ((uint8_t *)self->elements) + off;
    VALUE elem = native_slot_get(field_type, field_type_class, mem);
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
VALUE RepeatedField_each(VALUE _self) {
  RepeatedField* self = ruby_to_RepeatedField(_self);
  upb_fieldtype_t field_type = self->field_type;
  VALUE field_type_class = self->field_type_class;
  int element_size = native_slot_size(field_type);
  size_t off = 0;
  int i;

  for (i = 0; i < self->size; i++, off += element_size) {
    void* memory = (void *) (((uint8_t *)self->elements) + off);
    VALUE val = native_slot_get(field_type, field_type_class, memory);
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
VALUE RepeatedField_index(int argc, VALUE* argv, VALUE _self) {
  RepeatedField* self = ruby_to_RepeatedField(_self);
  int element_size = native_slot_size(self->field_type);
  upb_fieldtype_t field_type = self->field_type;
  VALUE field_type_class = self->field_type_class;

  VALUE arg = argv[0];
  long beg, len;

  if (argc == 1){
    if (FIXNUM_P(arg)) {
      /* standard case */
      void* memory;
      int index = index_position(argv[0], self);
      if (index < 0 || index >= self->size) {
        return Qnil;
      }
      memory = RepeatedField_memoryat(self, index, element_size);
      return native_slot_get(field_type, field_type_class, memory);
    }else{
      /* check if idx is Range */
      switch (rb_range_beg_len(arg, &beg, &len, self->size, 0)) {
        case Qfalse:
          break;
        case Qnil:
          return Qnil;
        default:
          return RepeatedField_subarray(_self, beg, len);
      }
    }
  }
  /* assume 2 arguments */
  beg = NUM2LONG(argv[0]);
  len = NUM2LONG(argv[1]);
  if (beg < 0) {
    beg += self->size;
  }
  if (beg >= self->size) {
    return Qnil;
  }
  return RepeatedField_subarray(_self, beg, len);
}

/*
 * call-seq:
 *     RepeatedField.[]=(index, value)
 *
 * Sets the element at the given index. On out-of-bounds assignments, extends
 * the array and fills the hole (if any) with default values.
 */
VALUE RepeatedField_index_set(VALUE _self, VALUE _index, VALUE val) {
  RepeatedField* self = ruby_to_RepeatedField(_self);
  upb_fieldtype_t field_type = self->field_type;
  VALUE field_type_class = self->field_type_class;
  int element_size = native_slot_size(field_type);
  void* memory;

  int index = index_position(_index, self);
  if (index < 0 || index >= (INT_MAX - 1)) {
    return Qnil;
  }
  if (index >= self->size) {
    upb_fieldtype_t field_type = self->field_type;
    int element_size = native_slot_size(field_type);
    int i;

    RepeatedField_reserve(self, index + 1);
    for (i = self->size; i <= index; i++) {
      void* elem = RepeatedField_memoryat(self, i, element_size);
      native_slot_init(field_type, elem);
    }
    self->size = index + 1;
  }

  memory = RepeatedField_memoryat(self, index, element_size);
  native_slot_set("", field_type, field_type_class, memory, val);
  return Qnil;
}

static int kInitialSize = 8;

void RepeatedField_reserve(RepeatedField* self, int new_size) {
  void* old_elems = self->elements;
  int elem_size = native_slot_size(self->field_type);
  if (new_size <= self->capacity) {
    return;
  }
  if (self->capacity == 0) {
    self->capacity = kInitialSize;
  }
  while (self->capacity < new_size) {
    self->capacity *= 2;
  }
  self->elements = ALLOC_N(uint8_t, elem_size * self->capacity);
  if (old_elems != NULL) {
    memcpy(self->elements, old_elems, self->size * elem_size);
    xfree(old_elems);
  }
}

/*
 * call-seq:
 *     RepeatedField.push(value)
 *
 * Adds a new element to the repeated field.
 */
VALUE RepeatedField_push(VALUE _self, VALUE val) {
  RepeatedField* self = ruby_to_RepeatedField(_self);
  upb_fieldtype_t field_type = self->field_type;
  int element_size = native_slot_size(field_type);
  void* memory;

  RepeatedField_reserve(self, self->size + 1);
  memory = (void *) (((uint8_t *)self->elements) + self->size * element_size);
  native_slot_set("", field_type, self->field_type_class, memory, val);
  // native_slot_set may raise an error; bump size only after set.
  self->size++;
  return _self;
}

VALUE RepeatedField_push_vararg(VALUE _self, VALUE args) {
  int i;
  for (i = 0; i < RARRAY_LEN(args); i++) {
    RepeatedField_push(_self, rb_ary_entry(args, i));
  }
  return _self;
}

// Used by parsing handlers.
void RepeatedField_push_native(VALUE _self, void* data) {
  RepeatedField* self = ruby_to_RepeatedField(_self);
  upb_fieldtype_t field_type = self->field_type;
  int element_size = native_slot_size(field_type);
  void* memory;

  RepeatedField_reserve(self, self->size + 1);
  memory = (void *) (((uint8_t *)self->elements) + self->size * element_size);
  memcpy(memory, data, element_size);
  self->size++;
}

void* RepeatedField_index_native(VALUE _self, int index) {
  RepeatedField* self = ruby_to_RepeatedField(_self);
  upb_fieldtype_t field_type = self->field_type;
  int element_size = native_slot_size(field_type);
  return RepeatedField_memoryat(self, index, element_size);
}

int RepeatedField_size(VALUE _self) {
  RepeatedField* self = ruby_to_RepeatedField(_self);
  return self->size;
}

/*
 * Private ruby method, used by RepeatedField.pop
 */
VALUE RepeatedField_pop_one(VALUE _self) {
  RepeatedField* self = ruby_to_RepeatedField(_self);
  upb_fieldtype_t field_type = self->field_type;
  VALUE field_type_class = self->field_type_class;
  int element_size = native_slot_size(field_type);
  int index;
  void* memory;
  VALUE ret;

  if (self->size == 0) {
    return Qnil;
  }
  index = self->size - 1;
  memory = RepeatedField_memoryat(self, index, element_size);
  ret = native_slot_get(field_type, field_type_class, memory);
  self->size--;
  return ret;
}

/*
 * call-seq:
 *     RepeatedField.replace(list)
 *
 * Replaces the contents of the repeated field with the given list of elements.
 */
VALUE RepeatedField_replace(VALUE _self, VALUE list) {
  RepeatedField* self = ruby_to_RepeatedField(_self);
  int i;

  Check_Type(list, T_ARRAY);
  self->size = 0;
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
VALUE RepeatedField_clear(VALUE _self) {
  RepeatedField* self = ruby_to_RepeatedField(_self);
  self->size = 0;
  return _self;
}

/*
 * call-seq:
 *     RepeatedField.length
 *
 * Returns the length of this repeated field.
 */
VALUE RepeatedField_length(VALUE _self) {
  RepeatedField* self = ruby_to_RepeatedField(_self);
  return INT2NUM(self->size);
}

VALUE RepeatedField_new_this_type(VALUE _self) {
  RepeatedField* self = ruby_to_RepeatedField(_self);
  VALUE new_rptfield = Qnil;
  VALUE element_type = fieldtype_to_ruby(self->field_type);
  if (self->field_type_class != Qnil) {
    new_rptfield = rb_funcall(CLASS_OF(_self), rb_intern("new"), 2,
                              element_type, self->field_type_class);
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
VALUE RepeatedField_dup(VALUE _self) {
  RepeatedField* self = ruby_to_RepeatedField(_self);
  VALUE new_rptfield = RepeatedField_new_this_type(_self);
  RepeatedField* new_rptfield_self = ruby_to_RepeatedField(new_rptfield);
  upb_fieldtype_t field_type = self->field_type;
  size_t elem_size = native_slot_size(field_type);
  size_t off = 0;
  int i;

  RepeatedField_reserve(new_rptfield_self, self->size);
  for (i = 0; i < self->size; i++, off += elem_size) {
    void* to_mem = (uint8_t *)new_rptfield_self->elements + off;
    void* from_mem = (uint8_t *)self->elements + off;
    native_slot_dup(field_type, to_mem, from_mem);
    new_rptfield_self->size++;
  }

  return new_rptfield;
}

// Internal only: used by Google::Protobuf.deep_copy.
VALUE RepeatedField_deep_copy(VALUE _self) {
  RepeatedField* self = ruby_to_RepeatedField(_self);
  VALUE new_rptfield = RepeatedField_new_this_type(_self);
  RepeatedField* new_rptfield_self = ruby_to_RepeatedField(new_rptfield);
  upb_fieldtype_t field_type = self->field_type;
  size_t elem_size = native_slot_size(field_type);
  size_t off = 0;
  int i;

  RepeatedField_reserve(new_rptfield_self, self->size);
  for (i = 0; i < self->size; i++, off += elem_size) {
    void* to_mem = (uint8_t *)new_rptfield_self->elements + off;
    void* from_mem = (uint8_t *)self->elements + off;
    native_slot_deep_copy(field_type, to_mem, from_mem);
    new_rptfield_self->size++;
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
  upb_fieldtype_t field_type = self->field_type;
  size_t elem_size = native_slot_size(field_type);
  size_t off = 0;
  VALUE ary = rb_ary_new2(self->size);
  int i;

  for (i = 0; i < self->size; i++, off += elem_size) {
    void* mem = ((uint8_t *)self->elements) + off;
    VALUE elem = native_slot_get(field_type, self->field_type_class, mem);
    rb_ary_push(ary, elem);
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

  self = ruby_to_RepeatedField(_self);
  other = ruby_to_RepeatedField(_other);
  if (self->field_type != other->field_type ||
      self->field_type_class != other->field_type_class ||
      self->size != other->size) {
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
      if (!native_slot_eq(field_type, self_mem, other_mem)) {
        return Qfalse;
      }
    }
    return Qtrue;
  }
}

/*
 * call-seq:
 *     RepeatedField.hash => hash_value
 *
 * Returns a hash value computed from this repeated field's elements.
 */
VALUE RepeatedField_hash(VALUE _self) {
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
  VALUE dupped = RepeatedField_dup(_self);

  if (TYPE(list) == T_ARRAY) {
    int i;
    for (i = 0; i < RARRAY_LEN(list); i++) {
      VALUE elem = rb_ary_entry(list, i);
      RepeatedField_push(dupped, elem);
    }
  } else if (RB_TYPE_P(list, T_DATA) && RTYPEDDATA_P(list) &&
             RTYPEDDATA_TYPE(list) == &RepeatedField_type) {
    RepeatedField* self = ruby_to_RepeatedField(_self);
    RepeatedField* list_rptfield = ruby_to_RepeatedField(list);
    int i;

    if (self->field_type != list_rptfield->field_type ||
        self->field_type_class != list_rptfield->field_type_class) {
      rb_raise(rb_eArgError,
               "Attempt to append RepeatedField with different element type.");
    }
    for (i = 0; i < list_rptfield->size; i++) {
      void* mem = RepeatedField_index_native(list, i);
      RepeatedField_push_native(dupped, mem);
    }
  } else {
    rb_raise(rb_eArgError, "Unknown type appending to RepeatedField");
  }

  return dupped;
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

void RepeatedField_init_args(int argc, VALUE* argv,
                             VALUE _self) {
  RepeatedField* self = ruby_to_RepeatedField(_self);
  VALUE ary = Qnil;
  if (argc < 1) {
    rb_raise(rb_eArgError, "Expected at least 1 argument.");
  }
  self->field_type = ruby_to_fieldtype(argv[0]);

  if (self->field_type == UPB_TYPE_MESSAGE ||
      self->field_type == UPB_TYPE_ENUM) {
    if (argc < 2) {
      rb_raise(rb_eArgError, "Expected at least 2 arguments for message/enum.");
    }
    self->field_type_class = argv[1];
    if (argc > 2) {
      ary = argv[2];
    }
    validate_type_class(self->field_type, self->field_type_class);
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
}

// Mark, free, alloc, init and class setup functions.

void RepeatedField_mark(void* _self) {
  RepeatedField* self = (RepeatedField*)_self;
  upb_fieldtype_t field_type = self->field_type;
  int element_size = native_slot_size(field_type);
  int i;

  rb_gc_mark(self->field_type_class);
  for (i = 0; i < self->size; i++) {
    void* memory = (((uint8_t *)self->elements) + i * element_size);
    native_slot_mark(self->field_type, memory);
  }
}

void RepeatedField_free(void* _self) {
  RepeatedField* self = (RepeatedField*)_self;
  xfree(self->elements);
  xfree(self);
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
VALUE RepeatedField_alloc(VALUE klass) {
  RepeatedField* self = ALLOC(RepeatedField);
  self->elements = NULL;
  self->size = 0;
  self->capacity = 0;
  self->field_type = -1;
  self->field_type_class = Qnil;
  return TypedData_Wrap_Struct(klass, &RepeatedField_type, self);
}

VALUE RepeatedField_init(int argc, VALUE* argv, VALUE self) {
  RepeatedField_init_args(argc, argv, self);
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
  rb_define_method(klass, "push", RepeatedField_push_vararg, -2);
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
