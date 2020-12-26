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

#include "defs.h"
#include "map.h"
#include "message.h"
#include "repeated_field.h"

VALUE cError;
VALUE cParseError;
VALUE cTypeError;

static VALUE cached_empty_string = Qnil;
static VALUE cached_empty_bytes = Qnil;

static VALUE create_frozen_string(const char* str, size_t size, bool binary) {
  VALUE str_rb = rb_str_new(str, size);

  rb_enc_associate(str_rb,
                   binary ? kRubyString8bitEncoding : kRubyStringUtf8Encoding);
  rb_obj_freeze(str_rb);
  return str_rb;
}

VALUE get_frozen_string(const char* str, size_t size, bool binary) {
  if (size == 0) {
    return binary ? cached_empty_bytes : cached_empty_string;
  } else {
    // It is harder to memoize non-empty strings.  The obvious approach would be
    // to use a Ruby hash keyed by string as memo table, but looking up in such a table
    // requires constructing a string (the very thing we're trying to avoid).
    //
    // Since few fields have defaults, we will just optimize the empty string
    // case for now.
    return create_frozen_string(str, size, binary);
  }
}

const upb_fielddef* map_field_key(const upb_fielddef* field) {
  const upb_msgdef *entry = upb_fielddef_msgsubdef(field);
  return upb_msgdef_itof(entry, 1);
}

const upb_fielddef* map_field_value(const upb_fielddef* field) {
  const upb_msgdef *entry = upb_fielddef_msgsubdef(field);
  return upb_msgdef_itof(entry, 2);
}

// -----------------------------------------------------------------------------
// Utilities.
// -----------------------------------------------------------------------------

// String encodings: we look these up once, at load time, and then cache them
// here.
rb_encoding* kRubyStringUtf8Encoding;
rb_encoding* kRubyString8bitEncoding;

// Ruby-interned string: "descriptor". We use this identifier to store an
// instance variable on message classes we create in order to link them back to
// their descriptors.
//
// We intern this once at module load time then use the interned identifier at
// runtime in order to avoid the cost of repeatedly interning in hot paths.
const char* kDescriptorInstanceVar = "descriptor";
ID descriptor_instancevar_interned;

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

StringBuilder* StringBuilder_New() {
  const size_t cap = 128;
  StringBuilder* builder = malloc(sizeof(*builder));
  builder->size = 0;
  builder->cap = cap;
  builder->data = malloc(builder->cap);
  return builder;
}

void StringBuilder_Free(StringBuilder* b) {
  free(b->data);
  free(b);
}

void StringBuilder_Printf(StringBuilder* b, const char *fmt, ...) {
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

VALUE StringBuilder_ToRubyString(StringBuilder* b) {
  VALUE ret = rb_str_new(b->data, b->size);
  rb_enc_associate(ret, kRubyStringUtf8Encoding);
  return ret;
}

static void StringBuilder_PrintEnum(StringBuilder* b, int32_t val,
                                    const upb_enumdef* e) {
  const char *name = upb_enumdef_iton(e, val);
  if (name) {
    StringBuilder_Printf(b, ":%s", name);
  } else {
    StringBuilder_Printf(b, "%" PRId32, val);
  }
}

void StringBuilder_PrintMsgval(StringBuilder* b, upb_msgval val,
                               TypeInfo info) {
  switch (info.type) {
    case UPB_TYPE_BOOL:
      StringBuilder_Printf(b, "%s", val.bool_val ? "true" : "false");
      break;
    case UPB_TYPE_FLOAT:
      StringBuilder_Printf(b, "%f", val.float_val);
      break;
    case UPB_TYPE_DOUBLE:
      StringBuilder_Printf(b, "%f", val.double_val);
      break;
    case UPB_TYPE_INT32:
      StringBuilder_Printf(b, "%" PRId32, val.int32_val);
      break;
    case UPB_TYPE_UINT32:
      StringBuilder_Printf(b, "%" PRIu32, val.uint32_val);
      break;
    case UPB_TYPE_INT64:
      StringBuilder_Printf(b, "%" PRId64, val.int64_val);
      break;
    case UPB_TYPE_UINT64:
      StringBuilder_Printf(b, "%" PRIu64, val.uint64_val);
      break;
    case UPB_TYPE_STRING:
      StringBuilder_Printf(b, "\"%.*s\"", (int)val.str_val.size, val.str_val.data);
      break;
    case UPB_TYPE_BYTES:
      StringBuilder_Printf(b, "\"%.*s\"", (int)val.str_val.size, val.str_val.data);
      break;
    case UPB_TYPE_ENUM:
      StringBuilder_PrintEnum(b, val.int32_val, info.def.enumdef);
      break;
    case UPB_TYPE_MESSAGE:
      Message_PrintMessage(b, val.msg_val, info.def.msgdef);
      break;
  }
}

// -----------------------------------------------------------------------------
// Arena
// -----------------------------------------------------------------------------

void Arena_free(void* data) { upb_arena_free(data); }

static VALUE cArena;

const rb_data_type_t Arena_type = {
  "Google::Protobuf::Internal::Arena",
  { NULL, Arena_free, NULL },
};

static VALUE Arena_alloc(VALUE klass) {
  upb_arena *arena = upb_arena_new();
  return TypedData_Wrap_Struct(klass, &Arena_type, arena);
}

upb_arena *Arena_get(VALUE _arena) {
  upb_arena *arena;
  TypedData_Get_Struct(_arena, upb_arena, &Arena_type, arena);
  return arena;
}

VALUE Arena_new() {
  return Arena_alloc(cArena);
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

static upb_inttable obj_cache;
VALUE obj_cache2 = Qnil;

static VALUE ObjectCache_GetKey(const void* key) {
  //char data[sizeof(key)];
  //memcpy(data, &key, sizeof(key));
  //return rb_str_new(data, sizeof(data));
  return LL2NUM((intptr_t)key);
}

void ObjectCache_Add(const void* key, VALUE val) {
  //PBRUBY_ASSERT(key);
  //fprintf(stderr, "Inserting key=%p, value=%s\n", key, rb_class2name(CLASS_OF(val)));
  //upb_inttable_insertptr(&obj_cache, key, upb_value_uint64(val));
  VALUE key_rb = ObjectCache_GetKey(key);
  rb_funcall(obj_cache2, rb_intern("[]="), 2, key_rb, val);
  PBRUBY_ASSERT(rb_funcall(obj_cache2, rb_intern("[]"), 1, key_rb) == val);
  PBRUBY_ASSERT(ObjectCache_Get(key) == val);
}

void ObjectCache_Remove(const void* key) {
  //fprintf(stderr, "Removing key=%p\n", key);
  //bool ok = upb_inttable_removeptr(&obj_cache, key, NULL);
  //PBRUBY_ASSERT(ok);
}

// Returns the cached object for this key, if any. Otherwise returns Qnil.
VALUE ObjectCache_Get(const void* key) {
  //PBRUBY_ASSERT(key);
  //upb_value val;
  //if (upb_inttable_lookupptr(&obj_cache, key, &val)) {
  //  return (VALUE)upb_value_getuint64(val);
  //} else {
  //  return Qnil;
  //}
  VALUE key_rb = ObjectCache_GetKey(key);
  VALUE ret = rb_funcall(obj_cache2, rb_intern("[]"), 1, key_rb);
  //fprintf(stderr, "Getting key=%p, ret=%s\n", key, rb_class2name(CLASS_OF(ret)));
  return ret;
}

/*
 * call-seq:
 *     Google::Protobuf.discard_unknown(msg)
 *
 * Discard unknown fields in the given message object and recursively discard
 * unknown fields in submessages.
 */
static VALUE Google_Protobuf_discard_unknown(VALUE self, VALUE msg_rb) {
  const upb_msgdef *m;
  upb_msg *msg = Message_GetMutable(msg_rb, &m);
  if (!upb_msg_discardunknown(msg, m, 128)) {
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
    upb_arena *new_arena = Arena_get(new_arena_rb);
    const upb_msgdef *m;
    const upb_msg *msg = Message_Get(obj, &m);
    upb_msg* new_msg = Message_deep_copy(msg, m, new_arena);
    return Message_GetRubyWrapper(new_msg, m, new_arena_rb);
  }
}

// -----------------------------------------------------------------------------
// Initialization/entry point.
// -----------------------------------------------------------------------------

// This must be named "Init_protobuf_c" because the Ruby module is named
// "protobuf_c" -- the VM looks for this symbol in our .so.
void Init_protobuf_c() {
  upb_inttable_init(&obj_cache, UPB_CTYPE_UINT64);
  VALUE google = rb_define_module("Google");
  VALUE protobuf = rb_define_module_under(google, "Protobuf");

  VALUE cWeakMap = rb_eval_string("ObjectSpace::WeakMap");
  rb_gc_register_address(&obj_cache2);
  obj_cache2 = rb_class_new_instance(0, NULL, cWeakMap);

  descriptor_instancevar_interned = rb_intern(kDescriptorInstanceVar);
  Arena_register(protobuf);
  Defs_register(protobuf);
  RepeatedField_register(protobuf);
  Map_register(protobuf);

  cError = rb_const_get(protobuf, rb_intern("Error"));
  cParseError = rb_const_get(protobuf, rb_intern("ParseError"));
  cTypeError = rb_const_get(protobuf, rb_intern("TypeError"));

  rb_define_singleton_method(protobuf, "discard_unknown",
                             Google_Protobuf_discard_unknown, 1);
  rb_define_singleton_method(protobuf, "deep_copy",
                             Google_Protobuf_deep_copy, 1);

  kRubyStringUtf8Encoding = rb_utf8_encoding();
  kRubyString8bitEncoding = rb_ascii8bit_encoding();

  rb_gc_register_address(&cached_empty_string);
  rb_gc_register_address(&cached_empty_bytes);
  cached_empty_string = create_frozen_string("", 0, false);
  cached_empty_bytes = create_frozen_string("", 0, true);
}
