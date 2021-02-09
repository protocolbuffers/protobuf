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
// Basic map operations on top of upb's strtable.
//
// Note that we roll our own `Map` container here because, as for
// `RepeatedField`, we want a strongly-typed container. This is so that any user
// errors due to incorrect map key or value types are raised as close as
// possible to the error site, rather than at some deferred point (e.g.,
// serialization).
//
// We build our `Map` on top of upb_strtable so that we're able to take
// advantage of the native_slot storage abstraction, as RepeatedField does.
// (This is not quite a perfect mapping -- see the key conversions below -- but
// gives us full support and error-checking for all value types for free.)
// -----------------------------------------------------------------------------

// Map values are stored using the native_slot abstraction (as with repeated
// field values), but keys are a bit special. Since we use a strtable, we need
// to store keys as sequences of bytes such that equality of those bytes maps
// one-to-one to equality of keys. We store strings directly (i.e., they map to
// their own bytes) and integers as native integers (using the native_slot
// abstraction).

// Note that there is another tradeoff here in keeping string keys as native
// strings rather than Ruby strings: traversing the Map requires conversion to
// Ruby string values on every traversal, potentially creating more garbage. We
// should consider ways to cache a Ruby version of the key if this becomes an
// issue later.

// Forms a key to use with the underlying strtable from a Ruby key value. |buf|
// must point to TABLE_KEY_BUF_LENGTH bytes of temporary space, used to
// construct a key byte sequence if needed. |out_key| and |out_length| provide
// the resulting key data/length.
#define TABLE_KEY_BUF_LENGTH 8  // sizeof(uint64_t)
static VALUE table_key(Map* self, VALUE key,
                       char* buf,
                       const char** out_key,
                       size_t* out_length) {
  switch (self->key_type) {
    case UPB_TYPE_BYTES:
    case UPB_TYPE_STRING:
      // Strings: use string content directly.
      if (TYPE(key) == T_SYMBOL) {
        key = rb_id2str(SYM2ID(key));
      }
      Check_Type(key, T_STRING);
      key = native_slot_encode_and_freeze_string(self->key_type, key);
      *out_key = RSTRING_PTR(key);
      *out_length = RSTRING_LEN(key);
      break;

    case UPB_TYPE_BOOL:
    case UPB_TYPE_INT32:
    case UPB_TYPE_INT64:
    case UPB_TYPE_UINT32:
    case UPB_TYPE_UINT64:
      native_slot_set("", self->key_type, Qnil, buf, key);
      *out_key = buf;
      *out_length = native_slot_size(self->key_type);
      break;

    default:
      // Map constructor should not allow a Map with another key type to be
      // constructed.
      assert(false);
      break;
  }

  return key;
}

static VALUE table_key_to_ruby(Map* self, upb_strview key) {
  switch (self->key_type) {
    case UPB_TYPE_BYTES:
    case UPB_TYPE_STRING: {
      VALUE ret = rb_str_new(key.data, key.size);
      rb_enc_associate(ret,
                       (self->key_type == UPB_TYPE_BYTES) ?
                       kRubyString8bitEncoding : kRubyStringUtf8Encoding);
      return ret;
    }

    case UPB_TYPE_BOOL:
    case UPB_TYPE_INT32:
    case UPB_TYPE_INT64:
    case UPB_TYPE_UINT32:
    case UPB_TYPE_UINT64:
      return native_slot_get(self->key_type, Qnil, key.data);

    default:
      assert(false);
      return Qnil;
  }
}

static void* value_memory(upb_value* v) {
  return (void*)(&v->val);
}

// -----------------------------------------------------------------------------
// Map container type.
// -----------------------------------------------------------------------------

const rb_data_type_t Map_type = {
  "Google::Protobuf::Map",
  { Map_mark, Map_free, NULL },
};

VALUE cMap;

Map* ruby_to_Map(VALUE _self) {
  Map* self;
  TypedData_Get_Struct(_self, Map, &Map_type, self);
  return self;
}

void Map_mark(void* _self) {
  Map* self = _self;

  rb_gc_mark(self->value_type_class);
  rb_gc_mark(self->parse_frame);

  if (self->value_type == UPB_TYPE_STRING ||
      self->value_type == UPB_TYPE_BYTES ||
      self->value_type == UPB_TYPE_MESSAGE) {
    upb_strtable_iter it;
    for (upb_strtable_begin(&it, &self->table);
         !upb_strtable_done(&it);
         upb_strtable_next(&it)) {
      upb_value v = upb_strtable_iter_value(&it);
      void* mem = value_memory(&v);
      native_slot_mark(self->value_type, mem);
    }
  }
}

void Map_free(void* _self) {
  Map* self = _self;
  upb_strtable_uninit(&self->table);
  xfree(self);
}

VALUE Map_alloc(VALUE klass) {
  Map* self = ALLOC(Map);
  memset(self, 0, sizeof(Map));
  self->value_type_class = Qnil;
  return TypedData_Wrap_Struct(klass, &Map_type, self);
}

VALUE Map_set_frame(VALUE map, VALUE val) {
  Map* self = ruby_to_Map(map);
  self->parse_frame = val;
  return val;
}

static bool needs_typeclass(upb_fieldtype_t type) {
  switch (type) {
    case UPB_TYPE_MESSAGE:
    case UPB_TYPE_ENUM:
      return true;
    default:
      return false;
  }
}

/*
 * call-seq:
 *     Map.new(key_type, value_type, value_typeclass = nil, init_hashmap = {})
 *     => new map
 *
 * Allocates a new Map container. This constructor may be called with 2, 3, or 4
 * arguments. The first two arguments are always present and are symbols (taking
 * on the same values as field-type symbols in message descriptors) that
 * indicate the type of the map key and value fields.
 *
 * The supported key types are: :int32, :int64, :uint32, :uint64, :bool,
 * :string, :bytes.
 *
 * The supported value types are: :int32, :int64, :uint32, :uint64, :bool,
 * :string, :bytes, :enum, :message.
 *
 * The third argument, value_typeclass, must be present if value_type is :enum
 * or :message. As in RepeatedField#new, this argument must be a message class
 * (for :message) or enum module (for :enum).
 *
 * The last argument, if present, provides initial content for map. Note that
 * this may be an ordinary Ruby hashmap or another Map instance with identical
 * key and value types. Also note that this argument may be present whether or
 * not value_typeclass is present (and it is unambiguously separate from
 * value_typeclass because value_typeclass's presence is strictly determined by
 * value_type). The contents of this initial hashmap or Map instance are
 * shallow-copied into the new Map: the original map is unmodified, but
 * references to underlying objects will be shared if the value type is a
 * message type.
 */
VALUE Map_init(int argc, VALUE* argv, VALUE _self) {
  Map* self = ruby_to_Map(_self);
  int init_value_arg;

  // We take either two args (:key_type, :value_type), three args (:key_type,
  // :value_type, "ValueMessageType"), or four args (the above plus an initial
  // hashmap).
  if (argc < 2 || argc > 4) {
    rb_raise(rb_eArgError, "Map constructor expects 2, 3 or 4 arguments.");
  }

  self->key_type = ruby_to_fieldtype(argv[0]);
  self->value_type = ruby_to_fieldtype(argv[1]);
  self->parse_frame = Qnil;

  // Check that the key type is an allowed type.
  switch (self->key_type) {
    case UPB_TYPE_INT32:
    case UPB_TYPE_INT64:
    case UPB_TYPE_UINT32:
    case UPB_TYPE_UINT64:
    case UPB_TYPE_BOOL:
    case UPB_TYPE_STRING:
    case UPB_TYPE_BYTES:
      // These are OK.
      break;
    default:
      rb_raise(rb_eArgError, "Invalid key type for map.");
  }

  init_value_arg = 2;
  if (needs_typeclass(self->value_type) && argc > 2) {
    self->value_type_class = argv[2];
    validate_type_class(self->value_type, self->value_type_class);
    init_value_arg = 3;
  }

  // Table value type is always UINT64: this ensures enough space to store the
  // native_slot value.
  if (!upb_strtable_init(&self->table, UPB_CTYPE_UINT64)) {
    rb_raise(rb_eRuntimeError, "Could not allocate table.");
  }

  if (argc > init_value_arg) {
    Map_merge_into_self(_self, argv[init_value_arg]);
  }

  return Qnil;
}

/*
 * call-seq:
 *     Map.each(&block)
 *
 * Invokes &block on each |key, value| pair in the map, in unspecified order.
 * Note that Map also includes Enumerable; map thus acts like a normal Ruby
 * sequence.
 */
VALUE Map_each(VALUE _self) {
  Map* self = ruby_to_Map(_self);

  upb_strtable_iter it;
  for (upb_strtable_begin(&it, &self->table);
       !upb_strtable_done(&it);
       upb_strtable_next(&it)) {
    VALUE key = table_key_to_ruby(self, upb_strtable_iter_key(&it));

    upb_value v = upb_strtable_iter_value(&it);
    void* mem = value_memory(&v);
    VALUE value = native_slot_get(self->value_type,
                                  self->value_type_class,
                                  mem);

    rb_yield_values(2, key, value);
  }

  return Qnil;
}

/*
 * call-seq:
 *     Map.keys => [list_of_keys]
 *
 * Returns the list of keys contained in the map, in unspecified order.
 */
VALUE Map_keys(VALUE _self) {
  Map* self = ruby_to_Map(_self);

  VALUE ret = rb_ary_new();
  upb_strtable_iter it;
  for (upb_strtable_begin(&it, &self->table);
       !upb_strtable_done(&it);
       upb_strtable_next(&it)) {
    VALUE key = table_key_to_ruby(self, upb_strtable_iter_key(&it));

    rb_ary_push(ret, key);
  }

  return ret;
}

/*
 * call-seq:
 *     Map.values => [list_of_values]
 *
 * Returns the list of values contained in the map, in unspecified order.
 */
VALUE Map_values(VALUE _self) {
  Map* self = ruby_to_Map(_self);

  VALUE ret = rb_ary_new();
  upb_strtable_iter it;
  for (upb_strtable_begin(&it, &self->table);
       !upb_strtable_done(&it);
       upb_strtable_next(&it)) {

    upb_value v = upb_strtable_iter_value(&it);
    void* mem = value_memory(&v);
    VALUE value = native_slot_get(self->value_type,
                                  self->value_type_class,
                                  mem);

    rb_ary_push(ret, value);
  }

  return ret;
}

/*
 * call-seq:
 *     Map.[](key) => value
 *
 * Accesses the element at the given key. Throws an exception if the key type is
 * incorrect. Returns nil when the key is not present in the map.
 */
VALUE Map_index(VALUE _self, VALUE key) {
  Map* self = ruby_to_Map(_self);

  char keybuf[TABLE_KEY_BUF_LENGTH];
  const char* keyval = NULL;
  size_t length = 0;
  upb_value v;
  key = table_key(self, key, keybuf, &keyval, &length);

  if (upb_strtable_lookup2(&self->table, keyval, length, &v)) {
    void* mem = value_memory(&v);
    return native_slot_get(self->value_type, self->value_type_class, mem);
  } else {
    return Qnil;
  }
}

/*
 * call-seq:
 *     Map.[]=(key, value) => value
 *
 * Inserts or overwrites the value at the given key with the given new value.
 * Throws an exception if the key type is incorrect. Returns the new value that
 * was just inserted.
 */
VALUE Map_index_set(VALUE _self, VALUE key, VALUE value) {
  Map* self = ruby_to_Map(_self);
  char keybuf[TABLE_KEY_BUF_LENGTH];
  const char* keyval = NULL;
  size_t length = 0;
  upb_value v;
  void* mem;
  key = table_key(self, key, keybuf, &keyval, &length);

  rb_check_frozen(_self);

  if (TYPE(value) == T_HASH) {
    VALUE args[1] = { value };
    value = rb_class_new_instance(1, args, self->value_type_class);
  }

  mem = value_memory(&v);
  native_slot_set("", self->value_type, self->value_type_class, mem, value);

  // Replace any existing value by issuing a 'remove' operation first.
  upb_strtable_remove2(&self->table, keyval, length, NULL);
  if (!upb_strtable_insert2(&self->table, keyval, length, v)) {
    rb_raise(rb_eRuntimeError, "Could not insert into table");
  }

  // Ruby hashmap's :[]= method also returns the inserted value.
  return value;
}

/*
 * call-seq:
 *     Map.has_key?(key) => bool
 *
 * Returns true if the given key is present in the map. Throws an exception if
 * the key has the wrong type.
 */
VALUE Map_has_key(VALUE _self, VALUE key) {
  Map* self = ruby_to_Map(_self);

  char keybuf[TABLE_KEY_BUF_LENGTH];
  const char* keyval = NULL;
  size_t length = 0;
  key = table_key(self, key, keybuf, &keyval, &length);

  if (upb_strtable_lookup2(&self->table, keyval, length, NULL)) {
    return Qtrue;
  } else {
    return Qfalse;
  }
}

/*
 * call-seq:
 *     Map.delete(key) => old_value
 *
 * Deletes the value at the given key, if any, returning either the old value or
 * nil if none was present. Throws an exception if the key is of the wrong type.
 */
VALUE Map_delete(VALUE _self, VALUE key) {
  Map* self = ruby_to_Map(_self);
  char keybuf[TABLE_KEY_BUF_LENGTH];
  const char* keyval = NULL;
  size_t length = 0;
  upb_value v;
  key = table_key(self, key, keybuf, &keyval, &length);

  rb_check_frozen(_self);

  if (upb_strtable_remove2(&self->table, keyval, length, &v)) {
    void* mem = value_memory(&v);
    return native_slot_get(self->value_type, self->value_type_class, mem);
  } else {
    return Qnil;
  }
}

/*
 * call-seq:
 *     Map.clear
 *
 * Removes all entries from the map.
 */
VALUE Map_clear(VALUE _self) {
  Map* self = ruby_to_Map(_self);

  rb_check_frozen(_self);

  // Uninit and reinit the table -- this is faster than iterating and doing a
  // delete-lookup on each key.
  upb_strtable_uninit(&self->table);
  if (!upb_strtable_init(&self->table, UPB_CTYPE_INT64)) {
    rb_raise(rb_eRuntimeError, "Unable to re-initialize table");
  }
  return Qnil;
}

/*
 * call-seq:
 *     Map.length
 *
 * Returns the number of entries (key-value pairs) in the map.
 */
VALUE Map_length(VALUE _self) {
  Map* self = ruby_to_Map(_self);
  return ULL2NUM(upb_strtable_count(&self->table));
}

VALUE Map_new_this_type(VALUE _self) {
  Map* self = ruby_to_Map(_self);
  VALUE new_map = Qnil;
  VALUE key_type = fieldtype_to_ruby(self->key_type);
  VALUE value_type = fieldtype_to_ruby(self->value_type);
  if (self->value_type_class != Qnil) {
    new_map = rb_funcall(CLASS_OF(_self), rb_intern("new"), 3,
                         key_type, value_type, self->value_type_class);
  } else {
    new_map = rb_funcall(CLASS_OF(_self), rb_intern("new"), 2,
                         key_type, value_type);
  }
  return new_map;
}

/*
 * call-seq:
 *     Map.dup => new_map
 *
 * Duplicates this map with a shallow copy. References to all non-primitive
 * element objects (e.g., submessages) are shared.
 */
VALUE Map_dup(VALUE _self) {
  Map* self = ruby_to_Map(_self);
  VALUE new_map = Map_new_this_type(_self);
  Map* new_self = ruby_to_Map(new_map);

  upb_strtable_iter it;
  for (upb_strtable_begin(&it, &self->table);
       !upb_strtable_done(&it);
       upb_strtable_next(&it)) {
    upb_strview k = upb_strtable_iter_key(&it);
    upb_value v = upb_strtable_iter_value(&it);
    void* mem = value_memory(&v);
    upb_value dup;
    void* dup_mem = value_memory(&dup);
    native_slot_dup(self->value_type, dup_mem, mem);

    if (!upb_strtable_insert2(&new_self->table, k.data, k.size, dup)) {
      rb_raise(rb_eRuntimeError, "Error inserting value into new table");
    }
  }

  return new_map;
}

// Used by Google::Protobuf.deep_copy but not exposed directly.
VALUE Map_deep_copy(VALUE _self) {
  Map* self = ruby_to_Map(_self);
  VALUE new_map = Map_new_this_type(_self);
  Map* new_self = ruby_to_Map(new_map);

  upb_strtable_iter it;
  for (upb_strtable_begin(&it, &self->table);
       !upb_strtable_done(&it);
       upb_strtable_next(&it)) {
    upb_strview k = upb_strtable_iter_key(&it);
    upb_value v = upb_strtable_iter_value(&it);
    void* mem = value_memory(&v);
    upb_value dup;
    void* dup_mem = value_memory(&dup);
    native_slot_deep_copy(self->value_type, self->value_type_class, dup_mem,
                          mem);

    if (!upb_strtable_insert2(&new_self->table, k.data, k.size, dup)) {
      rb_raise(rb_eRuntimeError, "Error inserting value into new table");
    }
  }

  return new_map;
}

/*
 * call-seq:
 *     Map.==(other) => boolean
 *
 * Compares this map to another. Maps are equal if they have identical key sets,
 * and for each key, the values in both maps compare equal. Elements are
 * compared as per normal Ruby semantics, by calling their :== methods (or
 * performing a more efficient comparison for primitive types).
 *
 * Maps with dissimilar key types or value types/typeclasses are never equal,
 * even if value comparison (for example, between integers and floats) would
 * have otherwise indicated that every element has equal value.
 */
VALUE Map_eq(VALUE _self, VALUE _other) {
  Map* self = ruby_to_Map(_self);
  Map* other;
  upb_strtable_iter it;

  // Allow comparisons to Ruby hashmaps by converting to a temporary Map
  // instance. Slow, but workable.
  if (TYPE(_other) == T_HASH) {
    VALUE other_map = Map_new_this_type(_self);
    Map_merge_into_self(other_map, _other);
    _other = other_map;
  }

  other = ruby_to_Map(_other);

  if (self == other) {
    return Qtrue;
  }
  if (self->key_type != other->key_type ||
      self->value_type != other->value_type ||
      self->value_type_class != other->value_type_class) {
    return Qfalse;
  }
  if (upb_strtable_count(&self->table) != upb_strtable_count(&other->table)) {
    return Qfalse;
  }

  // For each member of self, check that an equal member exists at the same key
  // in other.
  for (upb_strtable_begin(&it, &self->table);
       !upb_strtable_done(&it);
       upb_strtable_next(&it)) {
    upb_strview k = upb_strtable_iter_key(&it);
    upb_value v = upb_strtable_iter_value(&it);
    void* mem = value_memory(&v);
    upb_value other_v;
    void* other_mem = value_memory(&other_v);

    if (!upb_strtable_lookup2(&other->table, k.data, k.size, &other_v)) {
      // Not present in other map.
      return Qfalse;
    }

    if (!native_slot_eq(self->value_type, self->value_type_class, mem,
                        other_mem)) {
      // Present, but value not equal.
      return Qfalse;
    }
  }

  return Qtrue;
}

/*
 * call-seq:
 *     Map.hash => hash_value
 *
 * Returns a hash value based on this map's contents.
 */
VALUE Map_hash(VALUE _self) {
  Map* self = ruby_to_Map(_self);

  st_index_t h = rb_hash_start(0);
  VALUE hash_sym = rb_intern("hash");

  upb_strtable_iter it;
  for (upb_strtable_begin(&it, &self->table); !upb_strtable_done(&it);
       upb_strtable_next(&it)) {
    VALUE key = table_key_to_ruby(self, upb_strtable_iter_key(&it));

    upb_value v = upb_strtable_iter_value(&it);
    void* mem = value_memory(&v);
    VALUE value = native_slot_get(self->value_type,
                                  self->value_type_class,
                                  mem);

    h = rb_hash_uint(h, NUM2LONG(rb_funcall(key, hash_sym, 0)));
    h = rb_hash_uint(h, NUM2LONG(rb_funcall(value, hash_sym, 0)));
  }

  return INT2FIX(h);
}

/*
 * call-seq:
 *     Map.to_h => {}
 *
 * Returns a Ruby Hash object containing all the values within the map
 */
VALUE Map_to_h(VALUE _self) {
  Map* self = ruby_to_Map(_self);
  VALUE hash = rb_hash_new();
  upb_strtable_iter it;
  for (upb_strtable_begin(&it, &self->table);
       !upb_strtable_done(&it);
       upb_strtable_next(&it)) {
    VALUE key = table_key_to_ruby(self, upb_strtable_iter_key(&it));
    upb_value v = upb_strtable_iter_value(&it);
    void* mem = value_memory(&v);
    VALUE value = native_slot_get(self->value_type,
                                  self->value_type_class,
                                  mem);

    if (self->value_type == UPB_TYPE_MESSAGE) {
      value = Message_to_h(value);
    }
    rb_hash_aset(hash, key, value);
  }
  return hash;
}

/*
 * call-seq:
 *     Map.inspect => string
 *
 * Returns a string representing this map's elements. It will be formatted as
 * "{key => value, key => value, ...}", with each key and value string
 * representation computed by its own #inspect method.
 */
VALUE Map_inspect(VALUE _self) {
  Map* self = ruby_to_Map(_self);

  VALUE str = rb_str_new2("{");

  bool first = true;
  VALUE inspect_sym = rb_intern("inspect");

  upb_strtable_iter it;
  for (upb_strtable_begin(&it, &self->table); !upb_strtable_done(&it);
       upb_strtable_next(&it)) {
    VALUE key = table_key_to_ruby(self, upb_strtable_iter_key(&it));

    upb_value v = upb_strtable_iter_value(&it);
    void* mem = value_memory(&v);
    VALUE value = native_slot_get(self->value_type,
                                  self->value_type_class,
                                  mem);

    if (!first) {
      str = rb_str_cat2(str, ", ");
    } else {
      first = false;
    }
    str = rb_str_append(str, rb_funcall(key, inspect_sym, 0));
    str = rb_str_cat2(str, "=>");
    str = rb_str_append(str, rb_funcall(value, inspect_sym, 0));
  }

  str = rb_str_cat2(str, "}");
  return str;
}

/*
 * call-seq:
 *     Map.merge(other_map) => map
 *
 * Copies key/value pairs from other_map into a copy of this map. If a key is
 * set in other_map and this map, the value from other_map overwrites the value
 * in the new copy of this map. Returns the new copy of this map with merged
 * contents.
 */
VALUE Map_merge(VALUE _self, VALUE hashmap) {
  VALUE dupped = Map_dup(_self);
  return Map_merge_into_self(dupped, hashmap);
}

static int merge_into_self_callback(VALUE key, VALUE value, VALUE self) {
  Map_index_set(self, key, value);
  return ST_CONTINUE;
}

// Used only internally -- shared by #merge and #initialize.
VALUE Map_merge_into_self(VALUE _self, VALUE hashmap) {
  if (TYPE(hashmap) == T_HASH) {
    rb_hash_foreach(hashmap, merge_into_self_callback, _self);
  } else if (RB_TYPE_P(hashmap, T_DATA) && RTYPEDDATA_P(hashmap) &&
             RTYPEDDATA_TYPE(hashmap) == &Map_type) {

    Map* self = ruby_to_Map(_self);
    Map* other = ruby_to_Map(hashmap);
    upb_strtable_iter it;

    if (self->key_type != other->key_type ||
        self->value_type != other->value_type ||
        self->value_type_class != other->value_type_class) {
      rb_raise(rb_eArgError, "Attempt to merge Map with mismatching types");
    }

    for (upb_strtable_begin(&it, &other->table);
         !upb_strtable_done(&it);
         upb_strtable_next(&it)) {
      upb_strview k = upb_strtable_iter_key(&it);

      // Replace any existing value by issuing a 'remove' operation first.
      upb_value v;
      upb_value oldv;
      upb_strtable_remove2(&self->table, k.data, k.size, &oldv);

      v = upb_strtable_iter_value(&it);
      upb_strtable_insert2(&self->table, k.data, k.size, v);
    }
  } else {
    rb_raise(rb_eArgError, "Unknown type merging into Map");
  }
  return _self;
}

// Internal method: map iterator initialization (used for serialization).
void Map_begin(VALUE _self, Map_iter* iter) {
  Map* self = ruby_to_Map(_self);
  iter->self = self;
  upb_strtable_begin(&iter->it, &self->table);
}

void Map_next(Map_iter* iter) {
  upb_strtable_next(&iter->it);
}

bool Map_done(Map_iter* iter) {
  return upb_strtable_done(&iter->it);
}

VALUE Map_iter_key(Map_iter* iter) {
  return table_key_to_ruby(iter->self, upb_strtable_iter_key(&iter->it));
}

VALUE Map_iter_value(Map_iter* iter) {
  upb_value v = upb_strtable_iter_value(&iter->it);
  void* mem = value_memory(&v);
  return native_slot_get(iter->self->value_type,
                         iter->self->value_type_class,
                         mem);
}

void Map_register(VALUE module) {
  VALUE klass = rb_define_class_under(module, "Map", rb_cObject);
  rb_define_alloc_func(klass, Map_alloc);
  rb_gc_register_address(&cMap);
  cMap = klass;

  rb_define_method(klass, "initialize", Map_init, -1);
  rb_define_method(klass, "each", Map_each, 0);
  rb_define_method(klass, "keys", Map_keys, 0);
  rb_define_method(klass, "values", Map_values, 0);
  rb_define_method(klass, "[]", Map_index, 1);
  rb_define_method(klass, "[]=", Map_index_set, 2);
  rb_define_method(klass, "has_key?", Map_has_key, 1);
  rb_define_method(klass, "delete", Map_delete, 1);
  rb_define_method(klass, "clear", Map_clear, 0);
  rb_define_method(klass, "length", Map_length, 0);
  rb_define_method(klass, "dup", Map_dup, 0);
  rb_define_method(klass, "==", Map_eq, 1);
  rb_define_method(klass, "hash", Map_hash, 0);
  rb_define_method(klass, "to_h", Map_to_h, 0);
  rb_define_method(klass, "inspect", Map_inspect, 0);
  rb_define_method(klass, "merge", Map_merge, 1);
  rb_include_module(klass, rb_mEnumerable);
}
