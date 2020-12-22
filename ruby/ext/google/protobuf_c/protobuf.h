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

#ifndef __GOOGLE_PROTOBUF_RUBY_PROTOBUF_H__
#define __GOOGLE_PROTOBUF_RUBY_PROTOBUF_H__

#include <ruby/ruby.h>
#include <ruby/vm.h>
#include <ruby/encoding.h>

#include "ruby-upb.h"

// Forward decls.
struct DescriptorPool;
struct Descriptor;
struct TypeInfo;
struct FileDescriptor;
struct FieldDescriptor;
struct EnumDescriptor;
struct Message;
struct MessageLayout;
struct MessageField;
struct MessageBuilderContext;
struct EnumBuilderContext;
struct FileBuilderContext;
struct Builder;

typedef struct DescriptorPool DescriptorPool;
typedef struct Descriptor Descriptor;
typedef struct TypeInfo TypeInfo;
typedef struct FileDescriptor FileDescriptor;
typedef struct FieldDescriptor FieldDescriptor;
typedef struct OneofDescriptor OneofDescriptor;
typedef struct EnumDescriptor EnumDescriptor;
typedef struct Message Message;
typedef struct MessageLayout MessageLayout;
typedef struct MessageField MessageField;
typedef struct MessageOneof MessageOneof;
typedef struct MessageBuilderContext MessageBuilderContext;
typedef struct OneofBuilderContext OneofBuilderContext;
typedef struct EnumBuilderContext EnumBuilderContext;
typedef struct FileBuilderContext FileBuilderContext;
typedef struct Builder Builder;

/*
 It can be a bit confusing how the C structs defined below and the Ruby
 objects interact and hold references to each other. First, a few principles:

 - Ruby's "TypedData" abstraction lets a Ruby VALUE hold a pointer to a C
   struct (or arbitrary memory chunk), own it, and free it when collected.
   Thus, each struct below will have a corresponding Ruby object
   wrapping/owning it.

 - To get back from an underlying upb {msg,enum}def to the Ruby object, we
   keep a global hashmap, accessed by get_def_obj/add_def_obj below.

 The in-memory structure is then something like:

   Ruby                        |      upb
                               |
   DescriptorPool  ------------|-----------> upb_symtab____________________
                               |                | (message types)          \
                               |                v                           \
   Descriptor   ---------------|-----------> upb_msgdef         (enum types)|
    |--> msgclass              |                |   ^                       |
    |    (dynamically built)   |                |   | (submsg fields)       |
    |--> MessageLayout         |                |   |                       /
    |--------------------------|> decoder method|   |                      /
    \--------------------------|> serialize     |   |                     /
                               |  handlers      v   |                    /
   FieldDescriptor  -----------|-----------> upb_fielddef               /
                               |                    |                  /
                               |                    v (enum fields)   /
   EnumDescriptor  ------------|-----------> upb_enumdef  <----------'
                               |
                               |
               ^               |               \___/
               `---------------|-----------------'    (get_def_obj map)
 */

// -----------------------------------------------------------------------------
// Ruby class structure definitions.
// -----------------------------------------------------------------------------

struct DescriptorPool {
  VALUE def_to_descriptor;  // Hash table of def* -> Ruby descriptor.
  upb_symtab* symtab;
};

struct Descriptor {
  const upb_msgdef* msgdef;
  VALUE klass;
  VALUE descriptor_pool;
};

struct TypeInfo {
  upb_fieldtype_t type;
  union {
    const upb_msgdef* msgdef;  // When type == UPB_TYPE_MESSAGE
    const upb_enumdef* enumdef;    // When type == UPB_TYPE_ENUM
  } def;
};

struct FileDescriptor {
  const upb_filedef* filedef;
  VALUE descriptor_pool;  // Owns the upb_filedef.
};

struct FieldDescriptor {
  const upb_fielddef* fielddef;
  VALUE descriptor_pool;  // Owns the upb_fielddef.
};

struct OneofDescriptor {
  const upb_oneofdef* oneofdef;
  VALUE descriptor_pool;  // Owns the upb_oneofdef.
};

struct EnumDescriptor {
  const upb_enumdef* enumdef;
  VALUE module;  // begins as nil
  VALUE descriptor_pool;  // Owns the upb_enumdef.
};

struct MessageBuilderContext {
  google_protobuf_DescriptorProto* msg_proto;
  VALUE file_builder;
};

struct OneofBuilderContext {
  int oneof_index;
  VALUE message_builder;
};

struct EnumBuilderContext {
  google_protobuf_EnumDescriptorProto* enum_proto;
  VALUE file_builder;
};

struct FileBuilderContext {
  upb_arena *arena;
  google_protobuf_FileDescriptorProto* file_proto;
  VALUE descriptor_pool;
};

struct Builder {
  VALUE descriptor_pool;
  VALUE default_file_builder;
};

extern VALUE cDescriptorPool;
extern VALUE cDescriptor;
extern VALUE cFileDescriptor;
extern VALUE cFieldDescriptor;
extern VALUE cEnumDescriptor;
extern VALUE cMessageBuilderContext;
extern VALUE cOneofBuilderContext;
extern VALUE cEnumBuilderContext;
extern VALUE cFileBuilderContext;
extern VALUE cBuilder;

extern VALUE cError;
extern VALUE cParseError;
extern VALUE cTypeError;

// We forward-declare all of the Ruby method implementations here because we
// sometimes call the methods directly across .c files, rather than going
// through Ruby's method dispatching (e.g. during message parse). It's cleaner
// to keep the list of object methods together than to split them between
// static-in-file definitions and header declarations.

void DescriptorPool_mark(void* _self);
void DescriptorPool_free(void* _self);
VALUE DescriptorPool_alloc(VALUE klass);
void DescriptorPool_register(VALUE module);
DescriptorPool* ruby_to_DescriptorPool(VALUE value);
VALUE DescriptorPool_build(int argc, VALUE* argv, VALUE _self);
VALUE DescriptorPool_lookup(VALUE _self, VALUE name);
VALUE DescriptorPool_generated_pool(VALUE _self);

extern VALUE generated_pool;

static inline TypeInfo TypeInfo_get(const upb_fielddef *f) {
  TypeInfo ret = {upb_fielddef_type(f), {NULL}};
  switch (ret.type) {
    case UPB_TYPE_MESSAGE:
      ret.def.msgdef = upb_fielddef_msgsubdef(f);
      break;
    case UPB_TYPE_ENUM:
      ret.def.enumdef = upb_fielddef_enumsubdef(f);
      break;
    default:
      break;
  }
  return ret;
}

static inline TypeInfo TypeInfo_from_type(upb_fieldtype_t type) {
  TypeInfo ret = {type};
  assert(type != UPB_TYPE_MESSAGE && type != UPB_TYPE_ENUM);
  return ret;
}

void Descriptor_mark(void* _self);
void Descriptor_free(void* _self);
VALUE Descriptor_alloc(VALUE klass);
void Descriptor_register(VALUE module);
Descriptor* ruby_to_Descriptor(VALUE value);
Descriptor* Descriptor_from_fielddef(const upb_fielddef *f);
VALUE Descriptor_initialize(VALUE _self, VALUE cookie, VALUE descriptor_pool,
                            VALUE ptr);
VALUE Descriptor_name(VALUE _self);
VALUE Descriptor_each(VALUE _self);
VALUE Descriptor_lookup(VALUE _self, VALUE name);
VALUE Descriptor_each_oneof(VALUE _self);
VALUE Descriptor_lookup_oneof(VALUE _self, VALUE name);
VALUE Descriptor_msgclass(VALUE _self);
VALUE Descriptor_file_descriptor(VALUE _self);
extern const rb_data_type_t _Descriptor_type;

void FileDescriptor_mark(void* _self);
void FileDescriptor_free(void* _self);
VALUE FileDescriptor_alloc(VALUE klass);
void FileDescriptor_register(VALUE module);
FileDescriptor* ruby_to_FileDescriptor(VALUE value);
VALUE FileDescriptor_initialize(VALUE _self, VALUE cookie,
                                VALUE descriptor_pool, VALUE ptr);
VALUE FileDescriptor_name(VALUE _self);
VALUE FileDescriptor_syntax(VALUE _self);

void FieldDescriptor_mark(void* _self);
void FieldDescriptor_free(void* _self);
VALUE FieldDescriptor_alloc(VALUE klass);
void FieldDescriptor_register(VALUE module);
FieldDescriptor* ruby_to_FieldDescriptor(VALUE value);
VALUE FieldDescriptor_initialize(VALUE _self, VALUE cookie,
                                 VALUE descriptor_pool, VALUE ptr);
VALUE FieldDescriptor_name(VALUE _self);
VALUE FieldDescriptor_type(VALUE _self);
VALUE FieldDescriptor_default(VALUE _self);
VALUE FieldDescriptor_label(VALUE _self);
VALUE FieldDescriptor_number(VALUE _self);
VALUE FieldDescriptor_submsg_name(VALUE _self);
VALUE FieldDescriptor_subtype(VALUE _self);
VALUE FieldDescriptor_has(VALUE _self, VALUE msg_rb);
VALUE FieldDescriptor_clear(VALUE _self, VALUE msg_rb);
VALUE FieldDescriptor_get(VALUE _self, VALUE msg_rb);
VALUE FieldDescriptor_set(VALUE _self, VALUE msg_rb, VALUE value);
upb_fieldtype_t ruby_to_fieldtype(VALUE type);
VALUE fieldtype_to_ruby(upb_fieldtype_t type);

void OneofDescriptor_mark(void* _self);
void OneofDescriptor_free(void* _self);
VALUE OneofDescriptor_alloc(VALUE klass);
void OneofDescriptor_register(VALUE module);
OneofDescriptor* ruby_to_OneofDescriptor(VALUE value);
VALUE OneofDescriptor_initialize(VALUE _self, VALUE cookie,
                                 VALUE descriptor_pool, VALUE ptr);
VALUE OneofDescriptor_name(VALUE _self);
VALUE OneofDescriptor_each(VALUE _self);

void EnumDescriptor_mark(void* _self);
void EnumDescriptor_free(void* _self);
VALUE EnumDescriptor_alloc(VALUE klass);
VALUE EnumDescriptor_initialize(VALUE _self, VALUE cookie,
                                VALUE descriptor_pool, VALUE ptr);
void EnumDescriptor_register(VALUE module);
EnumDescriptor* ruby_to_EnumDescriptor(VALUE value);
VALUE EnumDescriptor_file_descriptor(VALUE _self);
VALUE EnumDescriptor_name(VALUE _self);
VALUE EnumDescriptor_lookup_name(VALUE _self, VALUE name);
VALUE EnumDescriptor_lookup_value(VALUE _self, VALUE number);
VALUE EnumDescriptor_each(VALUE _self);
VALUE EnumDescriptor_enummodule(VALUE _self);
extern const rb_data_type_t _EnumDescriptor_type;

void MessageBuilderContext_mark(void* _self);
void MessageBuilderContext_free(void* _self);
VALUE MessageBuilderContext_alloc(VALUE klass);
void MessageBuilderContext_register(VALUE module);
MessageBuilderContext* ruby_to_MessageBuilderContext(VALUE value);
VALUE MessageBuilderContext_initialize(VALUE _self,
                                       VALUE _file_builder,
                                       VALUE name);
VALUE MessageBuilderContext_optional(int argc, VALUE* argv, VALUE _self);
VALUE MessageBuilderContext_proto3_optional(int argc, VALUE* argv, VALUE _self);
VALUE MessageBuilderContext_required(int argc, VALUE* argv, VALUE _self);
VALUE MessageBuilderContext_repeated(int argc, VALUE* argv, VALUE _self);
VALUE MessageBuilderContext_map(int argc, VALUE* argv, VALUE _self);
VALUE MessageBuilderContext_oneof(VALUE _self, VALUE name);

void OneofBuilderContext_mark(void* _self);
void OneofBuilderContext_free(void* _self);
VALUE OneofBuilderContext_alloc(VALUE klass);
void OneofBuilderContext_register(VALUE module);
OneofBuilderContext* ruby_to_OneofBuilderContext(VALUE value);
VALUE OneofBuilderContext_initialize(VALUE _self,
                                     VALUE descriptor,
                                     VALUE builder);
VALUE OneofBuilderContext_optional(int argc, VALUE* argv, VALUE _self);

void EnumBuilderContext_mark(void* _self);
void EnumBuilderContext_free(void* _self);
VALUE EnumBuilderContext_alloc(VALUE klass);
void EnumBuilderContext_register(VALUE module);
EnumBuilderContext* ruby_to_EnumBuilderContext(VALUE value);
VALUE EnumBuilderContext_initialize(VALUE _self, VALUE _file_builder,
                                    VALUE name);
VALUE EnumBuilderContext_value(VALUE _self, VALUE name, VALUE number);

void FileBuilderContext_mark(void* _self);
void FileBuilderContext_free(void* _self);
VALUE FileBuilderContext_alloc(VALUE klass);
void FileBuilderContext_register(VALUE module);
FileBuilderContext* ruby_to_FileBuilderContext(VALUE _self);
upb_strview FileBuilderContext_strdup(VALUE _self, VALUE rb_str);
upb_strview FileBuilderContext_strdup_name(VALUE _self, VALUE rb_str);
upb_strview FileBuilderContext_strdup_sym(VALUE _self, VALUE rb_sym);
VALUE FileBuilderContext_initialize(VALUE _self, VALUE descriptor_pool,
                                    VALUE name, VALUE options);
VALUE FileBuilderContext_add_message(VALUE _self, VALUE name);
VALUE FileBuilderContext_add_enum(VALUE _self, VALUE name);
VALUE FileBuilderContext_pending_descriptors(VALUE _self);

void Builder_mark(void* _self);
void Builder_free(void* _self);
VALUE Builder_alloc(VALUE klass);
void Builder_register(VALUE module);
Builder* ruby_to_Builder(VALUE value);
VALUE Builder_build(VALUE _self);
VALUE Builder_initialize(VALUE _self, VALUE descriptor_pool);
VALUE Builder_add_file(int argc, VALUE *argv, VALUE _self);
VALUE Builder_add_message(VALUE _self, VALUE name);
VALUE Builder_add_enum(VALUE _self, VALUE name);
VALUE Builder_finalize_to_pool(VALUE _self, VALUE pool_rb);

// -----------------------------------------------------------------------------
// Native slot storage abstraction.
// -----------------------------------------------------------------------------

#define NATIVE_SLOT_MAX_SIZE sizeof(uint64_t)

size_t native_slot_size(upb_fieldtype_t type);
void native_slot_set(const char* name,
                     upb_fieldtype_t type,
                     VALUE type_class,
                     void* memory,
                     VALUE value);
// Atomically (with respect to Ruby VM calls) either update the value and set a
// oneof case, or do neither. If |case_memory| is null, then no case value is
// set.
void native_slot_set_value_and_case(const char* name,
                                    upb_fieldtype_t type,
                                    VALUE type_class,
                                    void* memory,
                                    VALUE value,
                                    uint32_t* case_memory,
                                    uint32_t case_number);
VALUE native_slot_get(upb_fieldtype_t type,
                      VALUE type_class,
                      const void* memory);
void native_slot_init(upb_fieldtype_t type, void* memory);
void native_slot_mark(upb_fieldtype_t type, void* memory);
void native_slot_dup(upb_fieldtype_t type, void* to, void* from);
void native_slot_deep_copy(upb_fieldtype_t type, VALUE type_class, void* to,
                           void* from);
bool native_slot_eq(upb_fieldtype_t type, VALUE type_class, void* mem1,
                    void* mem2);

VALUE native_slot_encode_and_freeze_string(upb_fieldtype_t type, VALUE value);
void native_slot_check_int_range_precision(const char* name, upb_fieldtype_t type, VALUE value);
uint32_t slot_read_oneof_case(MessageLayout* layout, const void* storage,
                              const upb_oneofdef* oneof);
bool is_value_field(const upb_fielddef* f);

extern rb_encoding* kRubyStringUtf8Encoding;
extern rb_encoding* kRubyStringASCIIEncoding;
extern rb_encoding* kRubyString8bitEncoding;

VALUE field_type_class(const MessageLayout* layout, const upb_fielddef* field);

#define MAP_KEY_FIELD 1
#define MAP_VALUE_FIELD 2

// Oneof case slot value to indicate that no oneof case is set. The value `0` is
// safe because field numbers are used as case identifiers, and no field can
// have a number of 0.
#define ONEOF_CASE_NONE 0

// These operate on a map field (i.e., a repeated field of submessages whose
// submessage type is a map-entry msgdef).
bool is_map_field(const upb_fielddef* field);
const upb_fielddef* map_field_key(const upb_fielddef* field);
const upb_fielddef* map_field_value(const upb_fielddef* field);

// These operate on a map-entry msgdef.
const upb_fielddef* map_entry_key(const upb_msgdef* msgdef);
const upb_fielddef* map_entry_value(const upb_msgdef* msgdef);

// -----------------------------------------------------------------------------
// Repeated field container type.
// -----------------------------------------------------------------------------

typedef struct {
  upb_array *array;
  TypeInfo type_info;
  VALUE type_class;  // To GC-root the msgdef/enumdef in type_info.
  VALUE arena;       // To GC-root the upb_array.
} RepeatedField;

void RepeatedField_register(VALUE module);
void validate_type_class(upb_fieldtype_t type, VALUE klass);

extern const rb_data_type_t RepeatedField_type;
extern VALUE cRepeatedField;

extern VALUE cMap;

bool is_wrapper_type_field(const upb_fielddef* field);
VALUE ruby_wrapper_type(VALUE type_class, VALUE value);

// -----------------------------------------------------------------------------
// Message class creation.
// -----------------------------------------------------------------------------

struct Message {
  VALUE arena;
  upb_msg* msg;
  Descriptor* descriptor;      // kept alive by self.class.descriptor reference.
};

VALUE Arena_new();
upb_arena *Arena_get(VALUE arena);

extern rb_data_type_t Message_type;

VALUE build_class_from_descriptor(VALUE descriptor);

VALUE Google_Protobuf_discard_unknown(VALUE self, VALUE msg_rb);
VALUE Google_Protobuf_deep_copy(VALUE self, VALUE obj);

VALUE build_module_from_enumdesc(VALUE _enumdesc);
VALUE enum_lookup(VALUE self, VALUE number);
VALUE enum_resolve(VALUE self, VALUE sym);
VALUE enum_descriptor(VALUE self);

// Maximum depth allowed during encoding, to avoid stack overflows due to
// cycles.
#define ENCODE_MAX_NESTING 63

// -----------------------------------------------------------------------------
// A cache of frozen string objects to use as field defaults.
// -----------------------------------------------------------------------------
VALUE get_frozen_string(const char* data, size_t size, bool binary);

// -----------------------------------------------------------------------------
// Global map from upb {msg,enum}defs to wrapper Descriptor/EnumDescriptor
// instances.
// -----------------------------------------------------------------------------
VALUE get_msgdef_obj(VALUE descriptor_pool, const upb_msgdef* def);
VALUE get_enumdef_obj(VALUE descriptor_pool, const upb_enumdef* def);
VALUE get_fielddef_obj(VALUE descriptor_pool, const upb_fielddef* def);
VALUE get_filedef_obj(VALUE descriptor_pool, const upb_filedef* def);
VALUE get_oneofdef_obj(VALUE descriptor_pool, const upb_oneofdef* def);

// -----------------------------------------------------------------------------
// Global object cache from upb array/map/message/symtab to wrapper object.
// -----------------------------------------------------------------------------

// This is a conceptually "weak" cache, in that it does not prevent "val" from
// being collected.
//
// To prevent dangling references, the finalizer for "val" *must* call
// ObjectCache_Remove(). Only objects with such a finalizer are suitable to be
// stored in the cache.
void ObjectCache_Add(const void* key, VALUE val);
void ObjectCache_Remove(const void* key);

// Returns the cached object for this key, if any. Otherwise returns Qnil.
VALUE ObjectCache_Get(const void* key);

// -----------------------------------------------------------------------------
// StringBuilder, for inspect
// -----------------------------------------------------------------------------

struct StringBuilder;
typedef struct StringBuilder StringBuilder;

StringBuilder* StringBuilder_New();
void StringBuilder_Free(StringBuilder* b);
void StringBuilder_Printf(StringBuilder* b, const char *fmt, ...);
VALUE StringBuilder_ToRubyString(StringBuilder* b);

void StringBuilder_PrintMsgval(StringBuilder* b, upb_msgval val, TypeInfo info);

// -----------------------------------------------------------------------------
// Utilities.
// -----------------------------------------------------------------------------

void check_upb_status(const upb_status* status, const char* msg);

#define CHECK_UPB(code, msg) do {                                             \
    upb_status status = UPB_STATUS_INIT;                                      \
    code;                                                                     \
    check_upb_status(&status, msg);                                           \
} while (0)

extern ID descriptor_instancevar_interned;

// A distinct object that is not accessible from Ruby.  We use this as a
// constructor argument to enforce that certain objects cannot be created from
// Ruby.
extern VALUE c_only_cookie;

#ifdef NDEBUG
#define PBRUBY_ASSERT(expr) do {} while (false && (expr))
#else
#define PBRUBY_ASSERT(expr) assert(expr)
#endif

#define UPB_UNUSED(var) (void)var

#endif  // __GOOGLE_PROTOBUF_RUBY_PROTOBUF_H__
