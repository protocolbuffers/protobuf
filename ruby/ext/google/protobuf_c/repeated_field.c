// Protocol Buffers - Google's data interchange format
// Copyright 2014 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "repeated_field.h"

#include "convert.h"
#include "defs.h"
#include "message.h"
#include "protobuf.h"

// -----------------------------------------------------------------------------
// Repeated field container type.
// -----------------------------------------------------------------------------

typedef struct {
  const upb_Array* array;  // Can get as mutable when non-frozen.
  TypeInfo type_info;
  VALUE type_class;  // To GC-root the msgdef/enumdef in type_info.
  VALUE arena;       // To GC-root the upb_Array.
} RepeatedField;

VALUE cRepeatedField;

static void RepeatedField_mark(void* _self) {
  RepeatedField* self = (RepeatedField*)_self;
  rb_gc_mark(self->type_class);
  rb_gc_mark(self->arena);
}

const rb_data_type_t RepeatedField_type = {
    "Google::Protobuf::RepeatedField",
    {RepeatedField_mark, RUBY_DEFAULT_FREE, NULL},
    .flags = RUBY_TYPED_FREE_IMMEDIATELY,
};

static RepeatedField* ruby_to_RepeatedField(VALUE _self) {
  RepeatedField* self;
  TypedData_Get_Struct(_self, RepeatedField, &RepeatedField_type, self);
  return self;
}

static upb_Array* RepeatedField_GetMutable(VALUE _self) {
  const upb_Array* array = ruby_to_RepeatedField(_self)->array;
  Protobuf_CheckNotFrozen(_self, upb_Array_IsFrozen(array));
  return (upb_Array*)array;
}

VALUE RepeatedField_alloc(VALUE klass) {
  RepeatedField* self = ALLOC(RepeatedField);
  self->arena = Qnil;
  self->type_class = Qnil;
  self->array = NULL;
  return TypedData_Wrap_Struct(klass, &RepeatedField_type, self);
}

VALUE RepeatedField_EmptyFrozen(const upb_FieldDef* f) {
  PBRUBY_ASSERT(upb_FieldDef_IsRepeated(f));
  VALUE val = ObjectCache_Get(f);

  if (val == Qnil) {
    val = RepeatedField_alloc(cRepeatedField);
    RepeatedField* self;
    TypedData_Get_Struct(val, RepeatedField, &RepeatedField_type, self);
    self->arena = Arena_new();
    TypeInfo type_info = TypeInfo_get(f);
    self->array = upb_Array_New(Arena_get(self->arena), type_info.type);
    self->type_info = type_info;
    if (self->type_info.type == kUpb_CType_Message) {
      self->type_class = Descriptor_DefToClass(type_info.def.msgdef);
    }
    val = ObjectCache_TryAdd(f, RepeatedField_freeze(val));
  }
  PBRUBY_ASSERT(RB_OBJ_FROZEN(val));
  PBRUBY_ASSERT(upb_Array_IsFrozen(ruby_to_RepeatedField(val)->array));
  return val;
}

VALUE RepeatedField_GetRubyWrapper(const upb_Array* array, TypeInfo type_info,
                                   VALUE arena) {
  PBRUBY_ASSERT(array);
  PBRUBY_ASSERT(arena != Qnil);
  VALUE val = ObjectCache_Get(array);

  if (val == Qnil) {
    val = RepeatedField_alloc(cRepeatedField);
    RepeatedField* self;
    TypedData_Get_Struct(val, RepeatedField, &RepeatedField_type, self);
    self->array = array;
    self->arena = arena;
    self->type_info = type_info;
    if (self->type_info.type == kUpb_CType_Message) {
      self->type_class = Descriptor_DefToClass(type_info.def.msgdef);
    }
    val = ObjectCache_TryAdd(array, val);
  }

  PBRUBY_ASSERT(ruby_to_RepeatedField(val)->type_info.type == type_info.type);
  PBRUBY_ASSERT(ruby_to_RepeatedField(val)->type_info.def.msgdef ==
                type_info.def.msgdef);
  PBRUBY_ASSERT(ruby_to_RepeatedField(val)->array == array);
  return val;
}

static VALUE RepeatedField_new_this_type(RepeatedField* from) {
  VALUE arena_rb = Arena_new();
  upb_Array* array = upb_Array_New(Arena_get(arena_rb), from->type_info.type);
  VALUE ret = RepeatedField_GetRubyWrapper(array, from->type_info, arena_rb);
  PBRUBY_ASSERT(ruby_to_RepeatedField(ret)->type_class == from->type_class);
  return ret;
}

void RepeatedField_Inspect(StringBuilder* b, const upb_Array* array,
                           TypeInfo info) {
  bool first = true;
  StringBuilder_Printf(b, "[");
  size_t n = array ? upb_Array_Size(array) : 0;
  for (size_t i = 0; i < n; i++) {
    if (first) {
      first = false;
    } else {
      StringBuilder_Printf(b, ", ");
    }
    StringBuilder_PrintMsgval(b, upb_Array_Get(array, i), info);
  }
  StringBuilder_Printf(b, "]");
}

VALUE RepeatedField_deep_copy(VALUE _self) {
  RepeatedField* self = ruby_to_RepeatedField(_self);
  VALUE new_rptfield = RepeatedField_new_this_type(self);
  RepeatedField* new_self = ruby_to_RepeatedField(new_rptfield);
  VALUE arena_rb = new_self->arena;
  upb_Array* new_array = RepeatedField_GetMutable(new_rptfield);
  upb_Arena* arena = Arena_get(arena_rb);
  size_t elements = upb_Array_Size(self->array);

  upb_Array_Resize(new_array, elements, arena);

  size_t size = upb_Array_Size(self->array);
  for (size_t i = 0; i < size; i++) {
    upb_MessageValue msgval = upb_Array_Get(self->array, i);
    upb_MessageValue copy = Msgval_DeepCopy(msgval, self->type_info, arena);
    upb_Array_Set(new_array, i, copy);
  }

  return new_rptfield;
}

const upb_Array* RepeatedField_GetUpbArray(VALUE val, const upb_FieldDef* field,
                                           upb_Arena* arena) {
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

  Arena_fuse(self->arena, arena);
  return self->array;
}

static int index_position(VALUE _index, RepeatedField* repeated_field) {
  int index = NUM2INT(_index);
  if (index < 0) index += upb_Array_Size(repeated_field->array);
  return index;
}

static VALUE RepeatedField_subarray(RepeatedField* self, long beg, long len) {
  size_t size = upb_Array_Size(self->array);
  VALUE ary = rb_ary_new2(size);
  long i;

  for (i = beg; i < beg + len; i++) {
    upb_MessageValue msgval = upb_Array_Get(self->array, i);
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
  int size = upb_Array_Size(self->array);
  int i;

  for (i = 0; i < size; i++) {
    upb_MessageValue msgval = upb_Array_Get(self->array, i);
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
  long size = upb_Array_Size(self->array);

  VALUE arg = argv[0];
  long beg, len;

  if (argc == 1) {
    if (FIXNUM_P(arg)) {
      /* standard case */
      upb_MessageValue msgval;
      int index = index_position(argv[0], self);
      if (index < 0 || (size_t)index >= upb_Array_Size(self->array)) {
        return Qnil;
      }
      msgval = upb_Array_Get(self->array, index);
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
  int size = upb_Array_Size(self->array);
  upb_Array* array = RepeatedField_GetMutable(_self);
  upb_Arena* arena = Arena_get(self->arena);
  upb_MessageValue msgval = Convert_RubyToUpb(val, "", self->type_info, arena);

  int index = index_position(_index, self);
  if (index < 0 || index >= (INT_MAX - 1)) {
    return Qnil;
  }

  if (index >= size) {
    upb_Array_Resize(array, index + 1, arena);
    upb_MessageValue fill;
    memset(&fill, 0, sizeof(fill));
    for (int i = size; i < index; i++) {
      // Fill default values.
      // TODO: should this happen at the upb level?
      upb_Array_Set(array, i, fill);
    }
  }

  upb_Array_Set(array, index, msgval);
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
  upb_Arena* arena = Arena_get(self->arena);
  upb_Array* array = RepeatedField_GetMutable(_self);
  int i;

  for (i = 0; i < argc; i++) {
    upb_MessageValue msgval =
        Convert_RubyToUpb(argv[i], "", self->type_info, arena);
    upb_Array_Append(array, msgval, arena);
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
  upb_Arena* arena = Arena_get(self->arena);
  upb_Array* array = RepeatedField_GetMutable(_self);

  upb_MessageValue msgval = Convert_RubyToUpb(val, "", self->type_info, arena);
  upb_Array_Append(array, msgval, arena);

  return _self;
}

/*
 * Private ruby method, used by RepeatedField.pop
 */
static VALUE RepeatedField_pop_one(VALUE _self) {
  RepeatedField* self = ruby_to_RepeatedField(_self);
  size_t size = upb_Array_Size(self->array);
  upb_Array* array = RepeatedField_GetMutable(_self);
  upb_MessageValue last;
  VALUE ret;

  if (size == 0) {
    return Qnil;
  }

  last = upb_Array_Get(self->array, size - 1);
  ret = Convert_UpbToRuby(last, self->type_info, self->arena);

  upb_Array_Resize(array, size - 1, Arena_get(self->arena));
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
  upb_Array* array = RepeatedField_GetMutable(_self);
  int i;

  Check_Type(list, T_ARRAY);
  upb_Array_Resize(array, 0, Arena_get(self->arena));

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
  upb_Array* array = RepeatedField_GetMutable(_self);
  upb_Array_Resize(array, 0, Arena_get(self->arena));
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
  return INT2NUM(upb_Array_Size(self->array));
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
  VALUE new_rptfield = RepeatedField_new_this_type(self);
  RepeatedField* new_rptfield_self = ruby_to_RepeatedField(new_rptfield);
  upb_Array* new_array = RepeatedField_GetMutable(new_rptfield);
  upb_Arena* arena = Arena_get(new_rptfield_self->arena);
  int size = upb_Array_Size(self->array);
  int i;

  Arena_fuse(self->arena, arena);

  for (i = 0; i < size; i++) {
    upb_MessageValue msgval = upb_Array_Get(self->array, i);
    upb_Array_Append(new_array, msgval, arena);
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
  int size = upb_Array_Size(self->array);
  VALUE ary = rb_ary_new2(size);
  int i;

  for (i = 0; i < size; i++) {
    upb_MessageValue msgval = upb_Array_Get(self->array, i);
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

  self = ruby_to_RepeatedField(_self);
  other = ruby_to_RepeatedField(_other);
  size_t n = upb_Array_Size(self->array);

  if (self->type_info.type != other->type_info.type ||
      self->type_class != other->type_class ||
      upb_Array_Size(other->array) != n) {
    return Qfalse;
  }

  for (size_t i = 0; i < n; i++) {
    upb_MessageValue val1 = upb_Array_Get(self->array, i);
    upb_MessageValue val2 = upb_Array_Get(other->array, i);
    if (!Msgval_IsEqual(val1, val2, self->type_info)) {
      return Qfalse;
    }
  }

  return Qtrue;
}

/*
 * call-seq:
 *     RepeatedField.frozen? => bool
 *
 * Returns true if the repeated field is frozen in either Ruby or the underlying
 * representation. Freezes the Ruby repeated field object if it is not already
 * frozen in Ruby but it is frozen in the underlying representation.
 */
VALUE RepeatedField_frozen(VALUE _self) {
  RepeatedField* self = ruby_to_RepeatedField(_self);
  if (!upb_Array_IsFrozen(self->array)) {
    PBRUBY_ASSERT(!RB_OBJ_FROZEN(_self));
    return Qfalse;
  }

  // Lazily freeze the Ruby wrapper.
  if (!RB_OBJ_FROZEN(_self)) RB_OBJ_FREEZE(_self);
  return Qtrue;
}

/*
 * call-seq:
 *     RepeatedField.freeze => self
 *
 * Freezes the repeated field object. We have to intercept this so we can freeze
 * the underlying representation, not just the Ruby wrapper.
 */
VALUE RepeatedField_freeze(VALUE _self) {
  RepeatedField* self = ruby_to_RepeatedField(_self);
  if (RB_OBJ_FROZEN(_self)) {
    PBRUBY_ASSERT(upb_Array_IsFrozen(self->array));
    return _self;
  }

  if (!upb_Array_IsFrozen(self->array)) {
    if (self->type_info.type == kUpb_CType_Message) {
      upb_Array_Freeze(RepeatedField_GetMutable(_self),
                       upb_MessageDef_MiniTable(self->type_info.def.msgdef));
    } else {
      upb_Array_Freeze(RepeatedField_GetMutable(_self), NULL);
    }
  }
  RB_OBJ_FREEZE(_self);
  return _self;
}

/*
 * call-seq:
 *     RepeatedField.hash => hash_value
 *
 * Returns a hash value computed from this repeated field's elements.
 */
VALUE RepeatedField_hash(VALUE _self) {
  RepeatedField* self = ruby_to_RepeatedField(_self);
  uint64_t hash = 0;
  size_t n = upb_Array_Size(self->array);

  for (size_t i = 0; i < n; i++) {
    upb_MessageValue val = upb_Array_Get(self->array, i);
    hash = Msgval_GetHash(val, self->type_info, hash);
  }

  return LL2NUM(hash);
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
    upb_Array* dupped_array = RepeatedField_GetMutable(dupped_);
    upb_Arena* arena = Arena_get(dupped->arena);
    Arena_fuse(list_rptfield->arena, arena);
    int size = upb_Array_Size(list_rptfield->array);
    int i;

    if (self->type_info.type != list_rptfield->type_info.type ||
        self->type_class != list_rptfield->type_class) {
      rb_raise(rb_eArgError,
               "Attempt to append RepeatedField with different element type.");
    }

    for (i = 0; i < size; i++) {
      upb_MessageValue msgval = upb_Array_Get(list_rptfield->array, i);
      upb_Array_Append(dupped_array, msgval, arena);
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
  upb_Arena* arena;
  VALUE ary = Qnil;

  self->arena = Arena_new();
  arena = Arena_get(self->arena);

  if (argc < 1) {
    rb_raise(rb_eArgError, "Expected at least 1 argument.");
  }

  self->type_info = TypeInfo_FromClass(argc, argv, 0, &self->type_class, &ary);
  self->array = upb_Array_New(arena, self->type_info.type);
  VALUE stored_val = ObjectCache_TryAdd(self->array, _self);
  PBRUBY_ASSERT(stored_val == _self);

  if (ary != Qnil) {
    if (!RB_TYPE_P(ary, T_ARRAY)) {
      rb_raise(rb_eArgError, "Expected array as initialize argument");
    }
    for (int i = 0; i < RARRAY_LEN(ary); i++) {
      RepeatedField_push(_self, rb_ary_entry(ary, i));
    }
  }
  return Qnil;
}

void RepeatedField_register(VALUE module) {
  VALUE klass = rb_define_class_under(module, "RepeatedField", rb_cObject);
  rb_define_alloc_func(klass, RepeatedField_alloc);
  rb_gc_register_address(&cRepeatedField);
  cRepeatedField = klass;

  rb_define_method(klass, "initialize", RepeatedField_init, -1);
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
  rb_define_method(klass, "freeze", RepeatedField_freeze, 0);
  rb_define_method(klass, "frozen?", RepeatedField_frozen, 0);
  rb_define_method(klass, "hash", RepeatedField_hash, 0);
  rb_define_method(klass, "+", RepeatedField_plus, 1);
  rb_define_method(klass, "concat", RepeatedField_concat, 1);
  rb_include_module(klass, rb_mEnumerable);
}
