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

#include <ruby/version.h>

#include "defs.h"
#include "map.h"
#include "message.h"
#include "repeated_field.h"

VALUE cParseError;
VALUE cTypeError;

const upb_FieldDef *map_field_key(const upb_FieldDef *field) {
  const upb_MessageDef *entry = upb_FieldDef_MessageSubDef(field);
  return upb_MessageDef_FindFieldByNumber(entry, 1);
}

const upb_FieldDef *map_field_value(const upb_FieldDef *field) {
  const upb_MessageDef *entry = upb_FieldDef_MessageSubDef(field);
  return upb_MessageDef_FindFieldByNumber(entry, 2);
}

// -----------------------------------------------------------------------------
// StringBuilder, for inspect
// -----------------------------------------------------------------------------

struct StringBuilder {
  size_t size;
  size_t cap;
  char *data;
};

typedef struct StringBuilder StringBuilder;

static size_t StringBuilder_SizeOf(size_t cap) {
  return sizeof(StringBuilder) + cap;
}

StringBuilder *StringBuilder_New() {
  const size_t cap = 128;
  StringBuilder *builder = malloc(sizeof(*builder));
  builder->size = 0;
  builder->cap = cap;
  builder->data = malloc(builder->cap);
  return builder;
}

void StringBuilder_Free(StringBuilder *b) {
  free(b->data);
  free(b);
}

void StringBuilder_Printf(StringBuilder *b, const char *fmt, ...) {
  size_t have = b->cap - b->size;
  size_t n;
  va_list args;

  va_start(args, fmt);
  n = vsnprintf(&b->data[b->size], have, fmt, args);
  va_end(args);

  if (have <= n) {
    while (have <= n) {
      b->cap *= 2;
      have = b->cap - b->size;
    }
    b->data = realloc(b->data, StringBuilder_SizeOf(b->cap));
    va_start(args, fmt);
    n = vsnprintf(&b->data[b->size], have, fmt, args);
    va_end(args);
    PBRUBY_ASSERT(n < have);
  }

  b->size += n;
}

VALUE StringBuilder_ToRubyString(StringBuilder *b) {
  VALUE ret = rb_str_new(b->data, b->size);
  rb_enc_associate(ret, rb_utf8_encoding());
  return ret;
}

static void StringBuilder_PrintEnum(StringBuilder *b, int32_t val,
                                    const upb_EnumDef *e) {
  const upb_EnumValueDef *ev = upb_EnumDef_FindValueByNumber(e, val);
  if (ev) {
    StringBuilder_Printf(b, ":%s", upb_EnumValueDef_Name(ev));
  } else {
    StringBuilder_Printf(b, "%" PRId32, val);
  }
}

void StringBuilder_PrintMsgval(StringBuilder *b, upb_MessageValue val,
                               TypeInfo info) {
  switch (info.type) {
    case kUpb_CType_Bool:
      StringBuilder_Printf(b, "%s", val.bool_val ? "true" : "false");
      break;
    case kUpb_CType_Float: {
      VALUE str = rb_inspect(DBL2NUM(val.float_val));
      StringBuilder_Printf(b, "%s", RSTRING_PTR(str));
      break;
    }
    case kUpb_CType_Double: {
      VALUE str = rb_inspect(DBL2NUM(val.double_val));
      StringBuilder_Printf(b, "%s", RSTRING_PTR(str));
      break;
    }
    case kUpb_CType_Int32:
      StringBuilder_Printf(b, "%" PRId32, val.int32_val);
      break;
    case kUpb_CType_UInt32:
      StringBuilder_Printf(b, "%" PRIu32, val.uint32_val);
      break;
    case kUpb_CType_Int64:
      StringBuilder_Printf(b, "%" PRId64, val.int64_val);
      break;
    case kUpb_CType_UInt64:
      StringBuilder_Printf(b, "%" PRIu64, val.uint64_val);
      break;
    case kUpb_CType_String:
      StringBuilder_Printf(b, "\"%.*s\"", (int)val.str_val.size,
                           val.str_val.data);
      break;
    case kUpb_CType_Bytes:
      StringBuilder_Printf(b, "\"%.*s\"", (int)val.str_val.size,
                           val.str_val.data);
      break;
    case kUpb_CType_Enum:
      StringBuilder_PrintEnum(b, val.int32_val, info.def.enumdef);
      break;
    case kUpb_CType_Message:
      Message_PrintMessage(b, val.msg_val, info.def.msgdef);
      break;
  }
}

// -----------------------------------------------------------------------------
// Arena
// -----------------------------------------------------------------------------

typedef struct {
  upb_Arena *arena;
  VALUE pinned_objs;
} Arena;

static void Arena_mark(void *data) {
  Arena *arena = data;
  rb_gc_mark(arena->pinned_objs);
}

static void Arena_free(void *data) {
  Arena *arena = data;
  upb_Arena_Free(arena->arena);
  xfree(arena);
}

static VALUE cArena;

const rb_data_type_t Arena_type = {
    "Google::Protobuf::Internal::Arena",
    {Arena_mark, Arena_free, NULL},
    .flags = RUBY_TYPED_FREE_IMMEDIATELY,
};

static void* ruby_upb_allocfunc(upb_alloc* alloc, void* ptr, size_t oldsize, size_t size) {
  if (size == 0) {
    xfree(ptr);
    return NULL;
  } else {
    return xrealloc(ptr, size);
  }
}

upb_alloc ruby_upb_alloc = {&ruby_upb_allocfunc};

static VALUE Arena_alloc(VALUE klass) {
  Arena *arena = ALLOC(Arena);
  arena->arena = upb_Arena_Init(NULL, 0, &ruby_upb_alloc);
  arena->pinned_objs = Qnil;
  return TypedData_Wrap_Struct(klass, &Arena_type, arena);
}

upb_Arena *Arena_get(VALUE _arena) {
  Arena *arena;
  TypedData_Get_Struct(_arena, Arena, &Arena_type, arena);
  return arena->arena;
}

void Arena_fuse(VALUE _arena, upb_Arena *other) {
  Arena *arena;
  TypedData_Get_Struct(_arena, Arena, &Arena_type, arena);
  if (!upb_Arena_Fuse(arena->arena, other)) {
    rb_raise(rb_eRuntimeError,
             "Unable to fuse arenas. This should never happen since Ruby does "
             "not use initial blocks");
  }
}

VALUE Arena_new() { return Arena_alloc(cArena); }

void Arena_Pin(VALUE _arena, VALUE obj) {
  Arena *arena;
  TypedData_Get_Struct(_arena, Arena, &Arena_type, arena);
  if (arena->pinned_objs == Qnil) {
    arena->pinned_objs = rb_ary_new();
  }
  rb_ary_push(arena->pinned_objs, obj);
}

void Arena_register(VALUE module) {
  VALUE internal = rb_define_module_under(module, "Internal");
  VALUE klass = rb_define_class_under(internal, "Arena", rb_cObject);
  rb_define_alloc_func(klass, Arena_alloc);
  rb_gc_register_address(&cArena);
  cArena = klass;
}

// -----------------------------------------------------------------------------
// Object Cache
// -----------------------------------------------------------------------------

// A pointer -> Ruby Object cache that keeps references to Ruby wrapper
// objects.  This allows us to look up any Ruby wrapper object by the address
// of the object it is wrapping. That way we can avoid ever creating two
// different wrapper objects for the same C object, which saves memory and
// preserves object identity.
//
// We use WeakMap for the cache. For Ruby <2.7 we also need a secondary Hash
// to store WeakMap keys because Ruby <2.7 WeakMap doesn't allow non-finalizable
// keys.
//
// We also need the secondary Hash if sizeof(long) < sizeof(VALUE), because this
// means it may not be possible to fit a pointer into a Fixnum. Keys are
// pointers, and if they fit into a Fixnum, Ruby doesn't collect them, but if
// they overflow and require allocating a Bignum, they could get collected
// prematurely, thus removing the cache entry. This happens on 64-bit Windows,
// on which pointers are 64 bits but longs are 32 bits. In this case, we enable
// the secondary Hash to hold the keys and prevent them from being collected.

#if RUBY_API_VERSION_CODE >= 20700 && SIZEOF_LONG >= SIZEOF_VALUE
#define USE_SECONDARY_MAP 0
#else
#define USE_SECONDARY_MAP 1
#endif

#if USE_SECONDARY_MAP

// Maps Numeric -> Object. The object is then used as a key into the WeakMap.
// This is needed for Ruby <2.7 where a number cannot be a key to WeakMap.
// The object is used only for its identity; it does not contain any data.
VALUE secondary_map = Qnil;

// Mutations to the map are under a mutex, because SeconaryMap_MaybeGC()
// iterates over the map which cannot happen in parallel with insertions, or
// Ruby will throw:
//   can't add a new key into hash during iteration (RuntimeError)
VALUE secondary_map_mutex = Qnil;

// Lambda that will GC entries from the secondary map that are no longer present
// in the primary map.
VALUE gc_secondary_map_lambda = Qnil;
ID length;

extern VALUE weak_obj_cache;

static void SecondaryMap_Init() {
  rb_gc_register_address(&secondary_map);
  rb_gc_register_address(&gc_secondary_map_lambda);
  rb_gc_register_address(&secondary_map_mutex);
  secondary_map = rb_hash_new();
  gc_secondary_map_lambda = rb_eval_string(
      "->(secondary, weak) {\n"
      "  secondary.delete_if { |k, v| !weak.key?(v) }\n"
      "}\n");
  secondary_map_mutex = rb_mutex_new();
  length = rb_intern("length");
}

// The secondary map is a regular Hash, and will never shrink on its own.
// The main object cache is a WeakMap that will automatically remove entries
// when the target object is no longer reachable, but unless we manually
// remove the corresponding entries from the secondary map, it will grow
// without bound.
//
// To avoid this unbounded growth we periodically remove entries from the
// secondary map that are no longer present in the WeakMap. The logic of
// how often to perform this GC is an artbirary tuning parameter that
// represents a straightforward CPU/memory tradeoff.
//
// Requires: secondary_map_mutex is held.
static void SecondaryMap_MaybeGC() {
  PBRUBY_ASSERT(rb_mutex_locked_p(secondary_map_mutex) == Qtrue);
  size_t weak_len = NUM2ULL(rb_funcall(weak_obj_cache, length, 0));
  size_t secondary_len = RHASH_SIZE(secondary_map);
  if (secondary_len < weak_len) {
    // Logically this case should not be possible: a valid entry cannot exist in
    // the weak table unless there is a corresponding entry in the secondary
    // table. It should *always* be the case that secondary_len >= weak_len.
    //
    // However ObjectSpace::WeakMap#length (and therefore weak_len) is
    // unreliable: it overreports its true length by including non-live objects.
    // However these non-live objects are not yielded in iteration, so we may
    // have previously deleted them from the secondary map in a previous
    // invocation of SecondaryMap_MaybeGC().
    //
    // In this case, we can't measure any waste, so we just return.
    return;
  }
  size_t waste = secondary_len - weak_len;
  // GC if we could remove at least 2000 entries or 20% of the table size
  // (whichever is greater).  Since the cost of the GC pass is O(N), we
  // want to make sure that we condition this on overall table size, to
  // avoid O(N^2) CPU costs.
  size_t threshold = PBRUBY_MAX(secondary_len * 0.2, 2000);
  if (waste > threshold) {
    rb_funcall(gc_secondary_map_lambda, rb_intern("call"), 2, secondary_map,
               weak_obj_cache);
  }
}

// Requires: secondary_map_mutex is held by this thread iff create == true.
static VALUE SecondaryMap_Get(VALUE key, bool create) {
  PBRUBY_ASSERT(!create || rb_mutex_locked_p(secondary_map_mutex) == Qtrue);
  VALUE ret = rb_hash_lookup(secondary_map, key);
  if (ret == Qnil && create) {
    SecondaryMap_MaybeGC();
    ret = rb_class_new_instance(0, NULL, rb_cObject);
    rb_hash_aset(secondary_map, key, ret);
  }
  return ret;
}

#endif

// Requires: secondary_map_mutex is held by this thread iff create == true.
static VALUE ObjectCache_GetKey(const void *key, bool create) {
  VALUE key_val = (VALUE)key;
  PBRUBY_ASSERT((key_val & 3) == 0);
  VALUE ret = LL2NUM(key_val >> 2);
#if USE_SECONDARY_MAP
  ret = SecondaryMap_Get(ret, create);
#endif
  return ret;
}

// Public ObjectCache API.

VALUE weak_obj_cache = Qnil;
ID item_get;
ID item_set;

static void ObjectCache_Init() {
  rb_gc_register_address(&weak_obj_cache);
  VALUE klass = rb_eval_string("ObjectSpace::WeakMap");
  weak_obj_cache = rb_class_new_instance(0, NULL, klass);
  item_get = rb_intern("[]");
  item_set = rb_intern("[]=");
#if USE_SECONDARY_MAP
  SecondaryMap_Init();
#endif
}

void ObjectCache_Add(const void *key, VALUE val) {
  PBRUBY_ASSERT(ObjectCache_Get(key) == Qnil);
#if USE_SECONDARY_MAP
  rb_mutex_lock(secondary_map_mutex);
#endif
  VALUE key_rb = ObjectCache_GetKey(key, true);
  rb_funcall(weak_obj_cache, item_set, 2, key_rb, val);
#if USE_SECONDARY_MAP
  rb_mutex_unlock(secondary_map_mutex);
#endif
  PBRUBY_ASSERT(ObjectCache_Get(key) == val);
}

// Returns the cached object for this key, if any. Otherwise returns Qnil.
VALUE ObjectCache_Get(const void *key) {
  VALUE key_rb = ObjectCache_GetKey(key, false);
  return rb_funcall(weak_obj_cache, item_get, 1, key_rb);
}

/*
 * call-seq:
 *     Google::Protobuf.discard_unknown(msg)
 *
 * Discard unknown fields in the given message object and recursively discard
 * unknown fields in submessages.
 */
static VALUE Google_Protobuf_discard_unknown(VALUE self, VALUE msg_rb) {
  const upb_MessageDef *m;
  upb_Message *msg = Message_GetMutable(msg_rb, &m);
  if (!upb_Message_DiscardUnknown(msg, m, 128)) {
    rb_raise(rb_eRuntimeError, "Messages nested too deeply.");
  }

  return Qnil;
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
    VALUE new_arena_rb = Arena_new();
    upb_Arena *new_arena = Arena_get(new_arena_rb);
    const upb_MessageDef *m;
    const upb_Message *msg = Message_Get(obj, &m);
    upb_Message *new_msg = Message_deep_copy(msg, m, new_arena);
    return Message_GetRubyWrapper(new_msg, m, new_arena_rb);
  }
}

// -----------------------------------------------------------------------------
// Initialization/entry point.
// -----------------------------------------------------------------------------

// This must be named "Init_protobuf_c" because the Ruby module is named
// "protobuf_c" -- the VM looks for this symbol in our .so.
__attribute__((visibility("default"))) void Init_protobuf_c() {
  ObjectCache_Init();

  VALUE google = rb_define_module("Google");
  VALUE protobuf = rb_define_module_under(google, "Protobuf");

  Arena_register(protobuf);
  Defs_register(protobuf);
  RepeatedField_register(protobuf);
  Map_register(protobuf);
  Message_register(protobuf);

  cParseError = rb_const_get(protobuf, rb_intern("ParseError"));
  rb_gc_register_mark_object(cParseError);
  cTypeError = rb_const_get(protobuf, rb_intern("TypeError"));
  rb_gc_register_mark_object(cTypeError);

  rb_define_singleton_method(protobuf, "discard_unknown",
                             Google_Protobuf_discard_unknown, 1);
  rb_define_singleton_method(protobuf, "deep_copy", Google_Protobuf_deep_copy,
                             1);
}
