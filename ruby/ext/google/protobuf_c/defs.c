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

#include <ctype.h>
#include <errno.h>
#include <ruby/version.h>

#include "convert.h"
#include "message.h"
#include "protobuf.h"

// -----------------------------------------------------------------------------
// Global map from upb {msg,enum}defs to wrapper Descriptor/EnumDescriptor
// instances.
// -----------------------------------------------------------------------------

static VALUE get_msgdef_obj(VALUE descriptor_pool, const upb_MessageDef* def);
static VALUE get_enumdef_obj(VALUE descriptor_pool, const upb_EnumDef* def);
static VALUE get_fielddef_obj(VALUE descriptor_pool, const upb_FieldDef* def);
static VALUE get_filedef_obj(VALUE descriptor_pool, const upb_FileDef* def);
static VALUE get_oneofdef_obj(VALUE descriptor_pool, const upb_OneofDef* def);

// A distinct object that is not accessible from Ruby.  We use this as a
// constructor argument to enforce that certain objects cannot be created from
// Ruby.
VALUE c_only_cookie = Qnil;

// -----------------------------------------------------------------------------
// Common utilities.
// -----------------------------------------------------------------------------

static const char* get_str(VALUE str) {
  Check_Type(str, T_STRING);
  return RSTRING_PTR(str);
}

static VALUE rb_str_maybe_null(const char* s) {
  if (s == NULL) {
    s = "";
  }
  return rb_str_new2(s);
}

// -----------------------------------------------------------------------------
// DescriptorPool.
// -----------------------------------------------------------------------------

typedef struct {
  VALUE def_to_descriptor;  // Hash table of def* -> Ruby descriptor.
  upb_DefPool* symtab;
} DescriptorPool;

VALUE cDescriptorPool = Qnil;

// Global singleton DescriptorPool. The user is free to create others, but this
// is used by generated code.
VALUE generated_pool = Qnil;

static void DescriptorPool_mark(void* _self) {
  DescriptorPool* self = _self;
  rb_gc_mark(self->def_to_descriptor);
}

static void DescriptorPool_free(void* _self) {
  DescriptorPool* self = _self;
  upb_DefPool_Free(self->symtab);
  xfree(self);
}

static const rb_data_type_t DescriptorPool_type = {
    "Google::Protobuf::DescriptorPool",
    {DescriptorPool_mark, DescriptorPool_free, NULL},
    .flags = RUBY_TYPED_FREE_IMMEDIATELY,
};

static DescriptorPool* ruby_to_DescriptorPool(VALUE val) {
  DescriptorPool* ret;
  TypedData_Get_Struct(val, DescriptorPool, &DescriptorPool_type, ret);
  return ret;
}

// Exposed to other modules in defs.h.
const upb_DefPool* DescriptorPool_GetSymtab(VALUE desc_pool_rb) {
  DescriptorPool* pool = ruby_to_DescriptorPool(desc_pool_rb);
  return pool->symtab;
}

/*
 * call-seq:
 *     DescriptorPool.new => pool
 *
 * Creates a new, empty, descriptor pool.
 */
static VALUE DescriptorPool_alloc(VALUE klass) {
  DescriptorPool* self = ALLOC(DescriptorPool);
  VALUE ret;

  self->def_to_descriptor = Qnil;
  ret = TypedData_Wrap_Struct(klass, &DescriptorPool_type, self);

  self->def_to_descriptor = rb_hash_new();
  self->symtab = upb_DefPool_New();
  ObjectCache_Add(self->symtab, ret);

  return ret;
}

/*
 * call-seq:
 *     DescriptorPool.add_serialized_file(serialized_file_proto)
 *
 * Adds the given serialized FileDescriptorProto to the pool.
 */
VALUE DescriptorPool_add_serialized_file(VALUE _self,
                                         VALUE serialized_file_proto) {
  DescriptorPool* self = ruby_to_DescriptorPool(_self);
  Check_Type(serialized_file_proto, T_STRING);
  VALUE arena_rb = Arena_new();
  upb_Arena* arena = Arena_get(arena_rb);
  google_protobuf_FileDescriptorProto* file_proto =
      google_protobuf_FileDescriptorProto_parse(
          RSTRING_PTR(serialized_file_proto),
          RSTRING_LEN(serialized_file_proto), arena);
  if (!file_proto) {
    rb_raise(rb_eArgError, "Unable to parse FileDescriptorProto");
  }
  upb_Status status;
  upb_Status_Clear(&status);
  const upb_FileDef* filedef =
      upb_DefPool_AddFile(self->symtab, file_proto, &status);
  if (!filedef) {
    rb_raise(cTypeError, "Unable to build file to DescriptorPool: %s",
             upb_Status_ErrorMessage(&status));
  }
  RB_GC_GUARD(arena_rb);
  return get_filedef_obj(_self, filedef);
}

/*
 * call-seq:
 *     DescriptorPool.lookup(name) => descriptor
 *
 * Finds a Descriptor or EnumDescriptor by name and returns it, or nil if none
 * exists with the given name.
 */
static VALUE DescriptorPool_lookup(VALUE _self, VALUE name) {
  DescriptorPool* self = ruby_to_DescriptorPool(_self);
  const char* name_str = get_str(name);
  const upb_MessageDef* msgdef;
  const upb_EnumDef* enumdef;

  msgdef = upb_DefPool_FindMessageByName(self->symtab, name_str);
  if (msgdef) {
    return get_msgdef_obj(_self, msgdef);
  }

  enumdef = upb_DefPool_FindEnumByName(self->symtab, name_str);
  if (enumdef) {
    return get_enumdef_obj(_self, enumdef);
  }

  return Qnil;
}

/*
 * call-seq:
 *     DescriptorPool.generated_pool => descriptor_pool
 *
 * Class method that returns the global DescriptorPool. This is a singleton into
 * which generated-code message and enum types are registered. The user may also
 * register types in this pool for convenience so that they do not have to hold
 * a reference to a private pool instance.
 */
static VALUE DescriptorPool_generated_pool(VALUE _self) {
  return generated_pool;
}

static void DescriptorPool_register(VALUE module) {
  VALUE klass = rb_define_class_under(module, "DescriptorPool", rb_cObject);
  rb_define_alloc_func(klass, DescriptorPool_alloc);
  rb_define_method(klass, "add_serialized_file",
                   DescriptorPool_add_serialized_file, 1);
  rb_define_method(klass, "lookup", DescriptorPool_lookup, 1);
  rb_define_singleton_method(klass, "generated_pool",
                             DescriptorPool_generated_pool, 0);
  rb_gc_register_address(&cDescriptorPool);
  cDescriptorPool = klass;

  rb_gc_register_address(&generated_pool);
  generated_pool = rb_class_new_instance(0, NULL, klass);
}

// -----------------------------------------------------------------------------
// Descriptor.
// -----------------------------------------------------------------------------

typedef struct {
  const upb_MessageDef* msgdef;
  VALUE klass;
  VALUE descriptor_pool;
} Descriptor;

VALUE cDescriptor = Qnil;

static void Descriptor_mark(void* _self) {
  Descriptor* self = _self;
  rb_gc_mark(self->klass);
  rb_gc_mark(self->descriptor_pool);
}

static const rb_data_type_t Descriptor_type = {
    "Google::Protobuf::Descriptor",
    {Descriptor_mark, RUBY_DEFAULT_FREE, NULL},
    .flags = RUBY_TYPED_FREE_IMMEDIATELY,
};

static Descriptor* ruby_to_Descriptor(VALUE val) {
  Descriptor* ret;
  TypedData_Get_Struct(val, Descriptor, &Descriptor_type, ret);
  return ret;
}

/*
 * call-seq:
 *     Descriptor.new => descriptor
 *
 * Creates a new, empty, message type descriptor. At a minimum, its name must be
 * set before it is added to a pool. It cannot be used to create messages until
 * it is added to a pool, after which it becomes immutable (as part of a
 * finalization process).
 */
static VALUE Descriptor_alloc(VALUE klass) {
  Descriptor* self = ALLOC(Descriptor);
  VALUE ret = TypedData_Wrap_Struct(klass, &Descriptor_type, self);
  self->msgdef = NULL;
  self->klass = Qnil;
  self->descriptor_pool = Qnil;
  return ret;
}

/*
 * call-seq:
 *    Descriptor.new(c_only_cookie, ptr) => Descriptor
 *
 * Creates a descriptor wrapper object.  May only be called from C.
 */
static VALUE Descriptor_initialize(VALUE _self, VALUE cookie,
                                   VALUE descriptor_pool, VALUE ptr) {
  Descriptor* self = ruby_to_Descriptor(_self);

  if (cookie != c_only_cookie) {
    rb_raise(rb_eRuntimeError,
             "Descriptor objects may not be created from Ruby.");
  }

  self->descriptor_pool = descriptor_pool;
  self->msgdef = (const upb_MessageDef*)NUM2ULL(ptr);

  return Qnil;
}

/*
 * call-seq:
 *    Descriptor.file_descriptor
 *
 * Returns the FileDescriptor object this message belongs to.
 */
static VALUE Descriptor_file_descriptor(VALUE _self) {
  Descriptor* self = ruby_to_Descriptor(_self);
  return get_filedef_obj(self->descriptor_pool,
                         upb_MessageDef_File(self->msgdef));
}

/*
 * call-seq:
 *     Descriptor.name => name
 *
 * Returns the name of this message type as a fully-qualified string (e.g.,
 * My.Package.MessageType).
 */
static VALUE Descriptor_name(VALUE _self) {
  Descriptor* self = ruby_to_Descriptor(_self);
  return rb_str_maybe_null(upb_MessageDef_FullName(self->msgdef));
}

/*
 * call-seq:
 *     Descriptor.each(&block)
 *
 * Iterates over fields in this message type, yielding to the block on each one.
 */
static VALUE Descriptor_each(VALUE _self) {
  Descriptor* self = ruby_to_Descriptor(_self);

  int n = upb_MessageDef_FieldCount(self->msgdef);
  for (int i = 0; i < n; i++) {
    const upb_FieldDef* field = upb_MessageDef_Field(self->msgdef, i);
    VALUE obj = get_fielddef_obj(self->descriptor_pool, field);
    rb_yield(obj);
  }
  return Qnil;
}

/*
 * call-seq:
 *     Descriptor.lookup(name) => FieldDescriptor
 *
 * Returns the field descriptor for the field with the given name, if present,
 * or nil if none.
 */
static VALUE Descriptor_lookup(VALUE _self, VALUE name) {
  Descriptor* self = ruby_to_Descriptor(_self);
  const char* s = get_str(name);
  const upb_FieldDef* field = upb_MessageDef_FindFieldByName(self->msgdef, s);
  if (field == NULL) {
    return Qnil;
  }
  return get_fielddef_obj(self->descriptor_pool, field);
}

/*
 * call-seq:
 *     Descriptor.each_oneof(&block) => nil
 *
 * Invokes the given block for each oneof in this message type, passing the
 * corresponding OneofDescriptor.
 */
static VALUE Descriptor_each_oneof(VALUE _self) {
  Descriptor* self = ruby_to_Descriptor(_self);

  int n = upb_MessageDef_OneofCount(self->msgdef);
  for (int i = 0; i < n; i++) {
    const upb_OneofDef* oneof = upb_MessageDef_Oneof(self->msgdef, i);
    VALUE obj = get_oneofdef_obj(self->descriptor_pool, oneof);
    rb_yield(obj);
  }
  return Qnil;
}

/*
 * call-seq:
 *     Descriptor.lookup_oneof(name) => OneofDescriptor
 *
 * Returns the oneof descriptor for the oneof with the given name, if present,
 * or nil if none.
 */
static VALUE Descriptor_lookup_oneof(VALUE _self, VALUE name) {
  Descriptor* self = ruby_to_Descriptor(_self);
  const char* s = get_str(name);
  const upb_OneofDef* oneof = upb_MessageDef_FindOneofByName(self->msgdef, s);
  if (oneof == NULL) {
    return Qnil;
  }
  return get_oneofdef_obj(self->descriptor_pool, oneof);
}

/*
 * call-seq:
 *     Descriptor.msgclass => message_klass
 *
 * Returns the Ruby class created for this message type.
 */
static VALUE Descriptor_msgclass(VALUE _self) {
  Descriptor* self = ruby_to_Descriptor(_self);
  if (self->klass == Qnil) {
    self->klass = build_class_from_descriptor(_self);
  }
  return self->klass;
}

static void Descriptor_register(VALUE module) {
  VALUE klass = rb_define_class_under(module, "Descriptor", rb_cObject);
  rb_define_alloc_func(klass, Descriptor_alloc);
  rb_define_method(klass, "initialize", Descriptor_initialize, 3);
  rb_define_method(klass, "each", Descriptor_each, 0);
  rb_define_method(klass, "lookup", Descriptor_lookup, 1);
  rb_define_method(klass, "each_oneof", Descriptor_each_oneof, 0);
  rb_define_method(klass, "lookup_oneof", Descriptor_lookup_oneof, 1);
  rb_define_method(klass, "msgclass", Descriptor_msgclass, 0);
  rb_define_method(klass, "name", Descriptor_name, 0);
  rb_define_method(klass, "file_descriptor", Descriptor_file_descriptor, 0);
  rb_include_module(klass, rb_mEnumerable);
  rb_gc_register_address(&cDescriptor);
  cDescriptor = klass;
}

// -----------------------------------------------------------------------------
// FileDescriptor.
// -----------------------------------------------------------------------------

typedef struct {
  const upb_FileDef* filedef;
  VALUE descriptor_pool;  // Owns the upb_FileDef.
} FileDescriptor;

static VALUE cFileDescriptor = Qnil;

static void FileDescriptor_mark(void* _self) {
  FileDescriptor* self = _self;
  rb_gc_mark(self->descriptor_pool);
}

static const rb_data_type_t FileDescriptor_type = {
    "Google::Protobuf::FileDescriptor",
    {FileDescriptor_mark, RUBY_DEFAULT_FREE, NULL},
    .flags = RUBY_TYPED_FREE_IMMEDIATELY,
};

static FileDescriptor* ruby_to_FileDescriptor(VALUE val) {
  FileDescriptor* ret;
  TypedData_Get_Struct(val, FileDescriptor, &FileDescriptor_type, ret);
  return ret;
}

static VALUE FileDescriptor_alloc(VALUE klass) {
  FileDescriptor* self = ALLOC(FileDescriptor);
  VALUE ret = TypedData_Wrap_Struct(klass, &FileDescriptor_type, self);
  self->descriptor_pool = Qnil;
  self->filedef = NULL;
  return ret;
}

/*
 * call-seq:
 *     FileDescriptor.new => file
 *
 * Returns a new file descriptor. The syntax must be set before it's passed
 * to a builder.
 */
static VALUE FileDescriptor_initialize(VALUE _self, VALUE cookie,
                                       VALUE descriptor_pool, VALUE ptr) {
  FileDescriptor* self = ruby_to_FileDescriptor(_self);

  if (cookie != c_only_cookie) {
    rb_raise(rb_eRuntimeError,
             "Descriptor objects may not be created from Ruby.");
  }

  self->descriptor_pool = descriptor_pool;
  self->filedef = (const upb_FileDef*)NUM2ULL(ptr);

  return Qnil;
}

/*
 * call-seq:
 *     FileDescriptor.name => name
 *
 * Returns the name of the file.
 */
static VALUE FileDescriptor_name(VALUE _self) {
  FileDescriptor* self = ruby_to_FileDescriptor(_self);
  const char* name = upb_FileDef_Name(self->filedef);
  return name == NULL ? Qnil : rb_str_new2(name);
}

/*
 * call-seq:
 *     FileDescriptor.syntax => syntax
 *
 * Returns this file descriptors syntax.
 *
 * Valid syntax versions are:
 *     :proto2 or :proto3.
 */
static VALUE FileDescriptor_syntax(VALUE _self) {
  FileDescriptor* self = ruby_to_FileDescriptor(_self);

  switch (upb_FileDef_Syntax(self->filedef)) {
    case kUpb_Syntax_Proto3:
      return ID2SYM(rb_intern("proto3"));
    case kUpb_Syntax_Proto2:
      return ID2SYM(rb_intern("proto2"));
    default:
      return Qnil;
  }
}

static void FileDescriptor_register(VALUE module) {
  VALUE klass = rb_define_class_under(module, "FileDescriptor", rb_cObject);
  rb_define_alloc_func(klass, FileDescriptor_alloc);
  rb_define_method(klass, "initialize", FileDescriptor_initialize, 3);
  rb_define_method(klass, "name", FileDescriptor_name, 0);
  rb_define_method(klass, "syntax", FileDescriptor_syntax, 0);
  rb_gc_register_address(&cFileDescriptor);
  cFileDescriptor = klass;
}

// -----------------------------------------------------------------------------
// FieldDescriptor.
// -----------------------------------------------------------------------------

typedef struct {
  const upb_FieldDef* fielddef;
  VALUE descriptor_pool;  // Owns the upb_FieldDef.
} FieldDescriptor;

static VALUE cFieldDescriptor = Qnil;

static void FieldDescriptor_mark(void* _self) {
  FieldDescriptor* self = _self;
  rb_gc_mark(self->descriptor_pool);
}

static const rb_data_type_t FieldDescriptor_type = {
    "Google::Protobuf::FieldDescriptor",
    {FieldDescriptor_mark, RUBY_DEFAULT_FREE, NULL},
    .flags = RUBY_TYPED_FREE_IMMEDIATELY,
};

static FieldDescriptor* ruby_to_FieldDescriptor(VALUE val) {
  FieldDescriptor* ret;
  TypedData_Get_Struct(val, FieldDescriptor, &FieldDescriptor_type, ret);
  return ret;
}

/*
 * call-seq:
 *     FieldDescriptor.new => field
 *
 * Returns a new field descriptor. Its name, type, etc. must be set before it is
 * added to a message type.
 */
static VALUE FieldDescriptor_alloc(VALUE klass) {
  FieldDescriptor* self = ALLOC(FieldDescriptor);
  VALUE ret = TypedData_Wrap_Struct(klass, &FieldDescriptor_type, self);
  self->fielddef = NULL;
  return ret;
}

/*
 * call-seq:
 *    EnumDescriptor.new(c_only_cookie, pool, ptr) => EnumDescriptor
 *
 * Creates a descriptor wrapper object.  May only be called from C.
 */
static VALUE FieldDescriptor_initialize(VALUE _self, VALUE cookie,
                                        VALUE descriptor_pool, VALUE ptr) {
  FieldDescriptor* self = ruby_to_FieldDescriptor(_self);

  if (cookie != c_only_cookie) {
    rb_raise(rb_eRuntimeError,
             "Descriptor objects may not be created from Ruby.");
  }

  self->descriptor_pool = descriptor_pool;
  self->fielddef = (const upb_FieldDef*)NUM2ULL(ptr);

  return Qnil;
}

/*
 * call-seq:
 *     FieldDescriptor.name => name
 *
 * Returns the name of this field.
 */
static VALUE FieldDescriptor_name(VALUE _self) {
  FieldDescriptor* self = ruby_to_FieldDescriptor(_self);
  return rb_str_maybe_null(upb_FieldDef_Name(self->fielddef));
}

// Non-static, exposed to other .c files.
upb_CType ruby_to_fieldtype(VALUE type) {
  if (TYPE(type) != T_SYMBOL) {
    rb_raise(rb_eArgError, "Expected symbol for field type.");
  }

#define CONVERT(upb, ruby)                \
  if (SYM2ID(type) == rb_intern(#ruby)) { \
    return kUpb_CType_##upb;                \
  }

  CONVERT(Float, float);
  CONVERT(Double, double);
  CONVERT(Bool, bool);
  CONVERT(String, string);
  CONVERT(Bytes, bytes);
  CONVERT(Message, message);
  CONVERT(Enum, enum);
  CONVERT(Int32, int32);
  CONVERT(Int64, int64);
  CONVERT(UInt32, uint32);
  CONVERT(UInt64, uint64);

#undef CONVERT

  rb_raise(rb_eArgError, "Unknown field type.");
  return 0;
}

static VALUE descriptortype_to_ruby(upb_FieldType type) {
  switch (type) {
#define CONVERT(upb, ruby)        \
  case kUpb_FieldType_##upb: \
    return ID2SYM(rb_intern(#ruby));
    CONVERT(Float, float);
    CONVERT(Double, double);
    CONVERT(Bool, bool);
    CONVERT(String, string);
    CONVERT(Bytes, bytes);
    CONVERT(Message, message);
    CONVERT(Group, group);
    CONVERT(Enum, enum);
    CONVERT(Int32, int32);
    CONVERT(Int64, int64);
    CONVERT(UInt32, uint32);
    CONVERT(UInt64, uint64);
    CONVERT(SInt32, sint32);
    CONVERT(SInt64, sint64);
    CONVERT(Fixed32, fixed32);
    CONVERT(Fixed64, fixed64);
    CONVERT(SFixed32, sfixed32);
    CONVERT(SFixed64, sfixed64);
#undef CONVERT
  }
  return Qnil;
}

/*
 * call-seq:
 *     FieldDescriptor.type => type
 *
 * Returns this field's type, as a Ruby symbol, or nil if not yet set.
 *
 * Valid field types are:
 *     :int32, :int64, :uint32, :uint64, :float, :double, :bool, :string,
 *     :bytes, :message.
 */
static VALUE FieldDescriptor__type(VALUE _self) {
  FieldDescriptor* self = ruby_to_FieldDescriptor(_self);
  return descriptortype_to_ruby(upb_FieldDef_Type(self->fielddef));
}

/*
 * call-seq:
 *     FieldDescriptor.default => default
 *
 * Returns this field's default, as a Ruby object, or nil if not yet set.
 */
static VALUE FieldDescriptor_default(VALUE _self) {
  FieldDescriptor* self = ruby_to_FieldDescriptor(_self);
  const upb_FieldDef* f = self->fielddef;
  upb_MessageValue default_val = {0};
  if (upb_FieldDef_IsSubMessage(f)) {
    return Qnil;
  } else if (!upb_FieldDef_IsRepeated(f)) {
    default_val = upb_FieldDef_Default(f);
  }
  return Convert_UpbToRuby(default_val, TypeInfo_get(self->fielddef), Qnil);
}

/*
 * call-seq:
 *     FieldDescriptor.json_name => json_name
 *
 * Returns this field's json_name, as a Ruby string, or nil if not yet set.
 */
static VALUE FieldDescriptor_json_name(VALUE _self) {
  FieldDescriptor* self = ruby_to_FieldDescriptor(_self);
  const upb_FieldDef* f = self->fielddef;
  const char* json_name = upb_FieldDef_JsonName(f);
  return rb_str_new2(json_name);
}

/*
 * call-seq:
 *     FieldDescriptor.label => label
 *
 * Returns this field's label (i.e., plurality), as a Ruby symbol.
 *
 * Valid field labels are:
 *     :optional, :repeated
 */
static VALUE FieldDescriptor_label(VALUE _self) {
  FieldDescriptor* self = ruby_to_FieldDescriptor(_self);
  switch (upb_FieldDef_Label(self->fielddef)) {
#define CONVERT(upb, ruby) \
  case kUpb_Label_##upb:    \
    return ID2SYM(rb_intern(#ruby));

    CONVERT(Optional, optional);
    CONVERT(Required, required);
    CONVERT(Repeated, repeated);

#undef CONVERT
  }

  return Qnil;
}

/*
 * call-seq:
 *     FieldDescriptor.number => number
 *
 * Returns the tag number for this field.
 */
static VALUE FieldDescriptor_number(VALUE _self) {
  FieldDescriptor* self = ruby_to_FieldDescriptor(_self);
  return INT2NUM(upb_FieldDef_Number(self->fielddef));
}

/*
 * call-seq:
 *     FieldDescriptor.submsg_name => submsg_name
 *
 * Returns the name of the message or enum type corresponding to this field, if
 * it is a message or enum field (respectively), or nil otherwise. This type
 * name will be resolved within the context of the pool to which the containing
 * message type is added.
 */
static VALUE FieldDescriptor_submsg_name(VALUE _self) {
  FieldDescriptor* self = ruby_to_FieldDescriptor(_self);
  switch (upb_FieldDef_CType(self->fielddef)) {
    case kUpb_CType_Enum:
      return rb_str_new2(
          upb_EnumDef_FullName(upb_FieldDef_EnumSubDef(self->fielddef)));
    case kUpb_CType_Message:
      return rb_str_new2(
          upb_MessageDef_FullName(upb_FieldDef_MessageSubDef(self->fielddef)));
    default:
      return Qnil;
  }
}

/*
 * call-seq:
 *     FieldDescriptor.subtype => message_or_enum_descriptor
 *
 * Returns the message or enum descriptor corresponding to this field's type if
 * it is a message or enum field, respectively, or nil otherwise. Cannot be
 * called *until* the containing message type is added to a pool (and thus
 * resolved).
 */
static VALUE FieldDescriptor_subtype(VALUE _self) {
  FieldDescriptor* self = ruby_to_FieldDescriptor(_self);
  switch (upb_FieldDef_CType(self->fielddef)) {
    case kUpb_CType_Enum:
      return get_enumdef_obj(self->descriptor_pool,
                             upb_FieldDef_EnumSubDef(self->fielddef));
    case kUpb_CType_Message:
      return get_msgdef_obj(self->descriptor_pool,
                            upb_FieldDef_MessageSubDef(self->fielddef));
    default:
      return Qnil;
  }
}

/*
 * call-seq:
 *     FieldDescriptor.get(message) => value
 *
 * Returns the value set for this field on the given message. Raises an
 * exception if message is of the wrong type.
 */
static VALUE FieldDescriptor_get(VALUE _self, VALUE msg_rb) {
  FieldDescriptor* self = ruby_to_FieldDescriptor(_self);
  const upb_MessageDef* m;

  Message_Get(msg_rb, &m);

  if (m != upb_FieldDef_ContainingType(self->fielddef)) {
    rb_raise(cTypeError, "get method called on wrong message type");
  }

  return Message_getfield(msg_rb, self->fielddef);
}

/*
 * call-seq:
 *     FieldDescriptor.has?(message) => boolean
 *
 * Returns whether the value is set on the given message. Raises an
 * exception when calling for fields that do not have presence.
 */
static VALUE FieldDescriptor_has(VALUE _self, VALUE msg_rb) {
  FieldDescriptor* self = ruby_to_FieldDescriptor(_self);
  const upb_MessageDef* m;
  const upb_MessageDef* msg = Message_Get(msg_rb, &m);

  if (m != upb_FieldDef_ContainingType(self->fielddef)) {
    rb_raise(cTypeError, "has method called on wrong message type");
  } else if (!upb_FieldDef_HasPresence(self->fielddef)) {
    rb_raise(rb_eArgError, "does not track presence");
  }

  return upb_Message_HasFieldByDef(msg, self->fielddef) ? Qtrue : Qfalse;
}

/*
 * call-seq:
 *     FieldDescriptor.clear(message)
 *
 * Clears the field from the message if it's set.
 */
static VALUE FieldDescriptor_clear(VALUE _self, VALUE msg_rb) {
  FieldDescriptor* self = ruby_to_FieldDescriptor(_self);
  const upb_MessageDef* m;
  upb_MessageDef* msg = Message_GetMutable(msg_rb, &m);

  if (m != upb_FieldDef_ContainingType(self->fielddef)) {
    rb_raise(cTypeError, "has method called on wrong message type");
  }

  upb_Message_ClearFieldByDef(msg, self->fielddef);
  return Qnil;
}

/*
 * call-seq:
 *     FieldDescriptor.set(message, value)
 *
 * Sets the value corresponding to this field to the given value on the given
 * message. Raises an exception if message is of the wrong type. Performs the
 * ordinary type-checks for field setting.
 */
static VALUE FieldDescriptor_set(VALUE _self, VALUE msg_rb, VALUE value) {
  FieldDescriptor* self = ruby_to_FieldDescriptor(_self);
  const upb_MessageDef* m;
  upb_MessageDef* msg = Message_GetMutable(msg_rb, &m);
  upb_Arena* arena = Arena_get(Message_GetArena(msg_rb));
  upb_MessageValue msgval;

  if (m != upb_FieldDef_ContainingType(self->fielddef)) {
    rb_raise(cTypeError, "set method called on wrong message type");
  }

  msgval = Convert_RubyToUpb(value, upb_FieldDef_Name(self->fielddef),
                             TypeInfo_get(self->fielddef), arena);
  upb_Message_SetFieldByDef(msg, self->fielddef, msgval, arena);
  return Qnil;
}

static void FieldDescriptor_register(VALUE module) {
  VALUE klass = rb_define_class_under(module, "FieldDescriptor", rb_cObject);
  rb_define_alloc_func(klass, FieldDescriptor_alloc);
  rb_define_method(klass, "initialize", FieldDescriptor_initialize, 3);
  rb_define_method(klass, "name", FieldDescriptor_name, 0);
  rb_define_method(klass, "type", FieldDescriptor__type, 0);
  rb_define_method(klass, "default", FieldDescriptor_default, 0);
  rb_define_method(klass, "json_name", FieldDescriptor_json_name, 0);
  rb_define_method(klass, "label", FieldDescriptor_label, 0);
  rb_define_method(klass, "number", FieldDescriptor_number, 0);
  rb_define_method(klass, "submsg_name", FieldDescriptor_submsg_name, 0);
  rb_define_method(klass, "subtype", FieldDescriptor_subtype, 0);
  rb_define_method(klass, "has?", FieldDescriptor_has, 1);
  rb_define_method(klass, "clear", FieldDescriptor_clear, 1);
  rb_define_method(klass, "get", FieldDescriptor_get, 1);
  rb_define_method(klass, "set", FieldDescriptor_set, 2);
  rb_gc_register_address(&cFieldDescriptor);
  cFieldDescriptor = klass;
}

// -----------------------------------------------------------------------------
// OneofDescriptor.
// -----------------------------------------------------------------------------

typedef struct {
  const upb_OneofDef* oneofdef;
  VALUE descriptor_pool;  // Owns the upb_OneofDef.
} OneofDescriptor;

static VALUE cOneofDescriptor = Qnil;

static void OneofDescriptor_mark(void* _self) {
  OneofDescriptor* self = _self;
  rb_gc_mark(self->descriptor_pool);
}

static const rb_data_type_t OneofDescriptor_type = {
    "Google::Protobuf::OneofDescriptor",
    {OneofDescriptor_mark, RUBY_DEFAULT_FREE, NULL},
    .flags = RUBY_TYPED_FREE_IMMEDIATELY,
};

static OneofDescriptor* ruby_to_OneofDescriptor(VALUE val) {
  OneofDescriptor* ret;
  TypedData_Get_Struct(val, OneofDescriptor, &OneofDescriptor_type, ret);
  return ret;
}

/*
 * call-seq:
 *     OneofDescriptor.new => oneof_descriptor
 *
 * Creates a new, empty, oneof descriptor. The oneof may only be modified prior
 * to being added to a message descriptor which is subsequently added to a pool.
 */
static VALUE OneofDescriptor_alloc(VALUE klass) {
  OneofDescriptor* self = ALLOC(OneofDescriptor);
  VALUE ret = TypedData_Wrap_Struct(klass, &OneofDescriptor_type, self);
  self->oneofdef = NULL;
  self->descriptor_pool = Qnil;
  return ret;
}

/*
 * call-seq:
 *    OneofDescriptor.new(c_only_cookie, pool, ptr) => OneofDescriptor
 *
 * Creates a descriptor wrapper object.  May only be called from C.
 */
static VALUE OneofDescriptor_initialize(VALUE _self, VALUE cookie,
                                        VALUE descriptor_pool, VALUE ptr) {
  OneofDescriptor* self = ruby_to_OneofDescriptor(_self);

  if (cookie != c_only_cookie) {
    rb_raise(rb_eRuntimeError,
             "Descriptor objects may not be created from Ruby.");
  }

  self->descriptor_pool = descriptor_pool;
  self->oneofdef = (const upb_OneofDef*)NUM2ULL(ptr);

  return Qnil;
}

/*
 * call-seq:
 *     OneofDescriptor.name => name
 *
 * Returns the name of this oneof.
 */
static VALUE OneofDescriptor_name(VALUE _self) {
  OneofDescriptor* self = ruby_to_OneofDescriptor(_self);
  return rb_str_maybe_null(upb_OneofDef_Name(self->oneofdef));
}

/*
 * call-seq:
 *     OneofDescriptor.each(&block) => nil
 *
 * Iterates through fields in this oneof, yielding to the block on each one.
 */
static VALUE OneofDescriptor_each(VALUE _self) {
  OneofDescriptor* self = ruby_to_OneofDescriptor(_self);

  int n = upb_OneofDef_FieldCount(self->oneofdef);
  for (int i = 0; i < n; i++) {
    const upb_FieldDef* f = upb_OneofDef_Field(self->oneofdef, i);
    VALUE obj = get_fielddef_obj(self->descriptor_pool, f);
    rb_yield(obj);
  }
  return Qnil;
}

static void OneofDescriptor_register(VALUE module) {
  VALUE klass = rb_define_class_under(module, "OneofDescriptor", rb_cObject);
  rb_define_alloc_func(klass, OneofDescriptor_alloc);
  rb_define_method(klass, "initialize", OneofDescriptor_initialize, 3);
  rb_define_method(klass, "name", OneofDescriptor_name, 0);
  rb_define_method(klass, "each", OneofDescriptor_each, 0);
  rb_include_module(klass, rb_mEnumerable);
  rb_gc_register_address(&cOneofDescriptor);
  cOneofDescriptor = klass;
}

// -----------------------------------------------------------------------------
// EnumDescriptor.
// -----------------------------------------------------------------------------

typedef struct {
  const upb_EnumDef* enumdef;
  VALUE module;           // begins as nil
  VALUE descriptor_pool;  // Owns the upb_EnumDef.
} EnumDescriptor;

static VALUE cEnumDescriptor = Qnil;

static void EnumDescriptor_mark(void* _self) {
  EnumDescriptor* self = _self;
  rb_gc_mark(self->module);
  rb_gc_mark(self->descriptor_pool);
}

static const rb_data_type_t EnumDescriptor_type = {
    "Google::Protobuf::EnumDescriptor",
    {EnumDescriptor_mark, RUBY_DEFAULT_FREE, NULL},
    .flags = RUBY_TYPED_FREE_IMMEDIATELY,
};

static EnumDescriptor* ruby_to_EnumDescriptor(VALUE val) {
  EnumDescriptor* ret;
  TypedData_Get_Struct(val, EnumDescriptor, &EnumDescriptor_type, ret);
  return ret;
}

static VALUE EnumDescriptor_alloc(VALUE klass) {
  EnumDescriptor* self = ALLOC(EnumDescriptor);
  VALUE ret = TypedData_Wrap_Struct(klass, &EnumDescriptor_type, self);
  self->enumdef = NULL;
  self->module = Qnil;
  self->descriptor_pool = Qnil;
  return ret;
}

// Exposed to other modules in defs.h.
const upb_EnumDef* EnumDescriptor_GetEnumDef(VALUE enum_desc_rb) {
  EnumDescriptor* desc = ruby_to_EnumDescriptor(enum_desc_rb);
  return desc->enumdef;
}

/*
 * call-seq:
 *    EnumDescriptor.new(c_only_cookie, ptr) => EnumDescriptor
 *
 * Creates a descriptor wrapper object.  May only be called from C.
 */
static VALUE EnumDescriptor_initialize(VALUE _self, VALUE cookie,
                                       VALUE descriptor_pool, VALUE ptr) {
  EnumDescriptor* self = ruby_to_EnumDescriptor(_self);

  if (cookie != c_only_cookie) {
    rb_raise(rb_eRuntimeError,
             "Descriptor objects may not be created from Ruby.");
  }

  self->descriptor_pool = descriptor_pool;
  self->enumdef = (const upb_EnumDef*)NUM2ULL(ptr);

  return Qnil;
}

/*
 * call-seq:
 *    EnumDescriptor.file_descriptor
 *
 * Returns the FileDescriptor object this enum belongs to.
 */
static VALUE EnumDescriptor_file_descriptor(VALUE _self) {
  EnumDescriptor* self = ruby_to_EnumDescriptor(_self);
  return get_filedef_obj(self->descriptor_pool,
                         upb_EnumDef_File(self->enumdef));
}

/*
 * call-seq:
 *     EnumDescriptor.name => name
 *
 * Returns the name of this enum type.
 */
static VALUE EnumDescriptor_name(VALUE _self) {
  EnumDescriptor* self = ruby_to_EnumDescriptor(_self);
  return rb_str_maybe_null(upb_EnumDef_FullName(self->enumdef));
}

/*
 * call-seq:
 *     EnumDescriptor.lookup_name(name) => value
 *
 * Returns the numeric value corresponding to the given key name (as a Ruby
 * symbol), or nil if none.
 */
static VALUE EnumDescriptor_lookup_name(VALUE _self, VALUE name) {
  EnumDescriptor* self = ruby_to_EnumDescriptor(_self);
  const char* name_str = rb_id2name(SYM2ID(name));
  const upb_EnumValueDef *ev =
      upb_EnumDef_FindValueByName(self->enumdef, name_str);
  if (ev) {
    return INT2NUM(upb_EnumValueDef_Number(ev));
  } else {
    return Qnil;
  }
}

/*
 * call-seq:
 *     EnumDescriptor.lookup_value(name) => value
 *
 * Returns the key name (as a Ruby symbol) corresponding to the integer value,
 * or nil if none.
 */
static VALUE EnumDescriptor_lookup_value(VALUE _self, VALUE number) {
  EnumDescriptor* self = ruby_to_EnumDescriptor(_self);
  int32_t val = NUM2INT(number);
  const upb_EnumValueDef* ev = upb_EnumDef_FindValueByNumber(self->enumdef, val);
  if (ev) {
    return ID2SYM(rb_intern(upb_EnumValueDef_Name(ev)));
  } else {
    return Qnil;
  }
}

/*
 * call-seq:
 *     EnumDescriptor.each(&block)
 *
 * Iterates over key => value mappings in this enum's definition, yielding to
 * the block with (key, value) arguments for each one.
 */
static VALUE EnumDescriptor_each(VALUE _self) {
  EnumDescriptor* self = ruby_to_EnumDescriptor(_self);

  int n = upb_EnumDef_ValueCount(self->enumdef);
  for (int i = 0; i < n; i++) {
    const upb_EnumValueDef* ev = upb_EnumDef_Value(self->enumdef, i);
    VALUE key = ID2SYM(rb_intern(upb_EnumValueDef_Name(ev)));
    VALUE number = INT2NUM(upb_EnumValueDef_Number(ev));
    rb_yield_values(2, key, number);
  }

  return Qnil;
}

/*
 * call-seq:
 *     EnumDescriptor.enummodule => module
 *
 * Returns the Ruby module corresponding to this enum type.
 */
static VALUE EnumDescriptor_enummodule(VALUE _self) {
  EnumDescriptor* self = ruby_to_EnumDescriptor(_self);
  if (self->module == Qnil) {
    self->module = build_module_from_enumdesc(_self);
  }
  return self->module;
}

static void EnumDescriptor_register(VALUE module) {
  VALUE klass = rb_define_class_under(module, "EnumDescriptor", rb_cObject);
  rb_define_alloc_func(klass, EnumDescriptor_alloc);
  rb_define_method(klass, "initialize", EnumDescriptor_initialize, 3);
  rb_define_method(klass, "name", EnumDescriptor_name, 0);
  rb_define_method(klass, "lookup_name", EnumDescriptor_lookup_name, 1);
  rb_define_method(klass, "lookup_value", EnumDescriptor_lookup_value, 1);
  rb_define_method(klass, "each", EnumDescriptor_each, 0);
  rb_define_method(klass, "enummodule", EnumDescriptor_enummodule, 0);
  rb_define_method(klass, "file_descriptor", EnumDescriptor_file_descriptor, 0);
  rb_include_module(klass, rb_mEnumerable);
  rb_gc_register_address(&cEnumDescriptor);
  cEnumDescriptor = klass;
}

static VALUE get_def_obj(VALUE _descriptor_pool, const void* ptr, VALUE klass) {
  DescriptorPool* descriptor_pool = ruby_to_DescriptorPool(_descriptor_pool);
  VALUE key = ULL2NUM((intptr_t)ptr);
  VALUE def;

  def = rb_hash_aref(descriptor_pool->def_to_descriptor, key);

  if (ptr == NULL) {
    return Qnil;
  }

  if (def == Qnil) {
    // Lazily create wrapper object.
    VALUE args[3] = {c_only_cookie, _descriptor_pool, key};
    def = rb_class_new_instance(3, args, klass);
    rb_hash_aset(descriptor_pool->def_to_descriptor, key, def);
  }

  return def;
}

static VALUE get_msgdef_obj(VALUE descriptor_pool, const upb_MessageDef* def) {
  return get_def_obj(descriptor_pool, def, cDescriptor);
}

static VALUE get_enumdef_obj(VALUE descriptor_pool, const upb_EnumDef* def) {
  return get_def_obj(descriptor_pool, def, cEnumDescriptor);
}

static VALUE get_fielddef_obj(VALUE descriptor_pool, const upb_FieldDef* def) {
  return get_def_obj(descriptor_pool, def, cFieldDescriptor);
}

static VALUE get_filedef_obj(VALUE descriptor_pool, const upb_FileDef* def) {
  return get_def_obj(descriptor_pool, def, cFileDescriptor);
}

static VALUE get_oneofdef_obj(VALUE descriptor_pool, const upb_OneofDef* def) {
  return get_def_obj(descriptor_pool, def, cOneofDescriptor);
}

// -----------------------------------------------------------------------------
// Shared functions
// -----------------------------------------------------------------------------

// Functions exposed to other modules in defs.h.

VALUE Descriptor_DefToClass(const upb_MessageDef* m) {
  const upb_DefPool* symtab = upb_FileDef_Pool(upb_MessageDef_File(m));
  VALUE pool = ObjectCache_Get(symtab);
  PBRUBY_ASSERT(pool != Qnil);
  VALUE desc_rb = get_msgdef_obj(pool, m);
  const Descriptor* desc = ruby_to_Descriptor(desc_rb);
  return desc->klass;
}

const upb_MessageDef* Descriptor_GetMsgDef(VALUE desc_rb) {
  const Descriptor* desc = ruby_to_Descriptor(desc_rb);
  return desc->msgdef;
}

VALUE TypeInfo_InitArg(int argc, VALUE* argv, int skip_arg) {
  if (argc > skip_arg) {
    if (argc > 1 + skip_arg) {
      rb_raise(rb_eArgError, "Expected a maximum of %d arguments.",
               skip_arg + 1);
    }
    return argv[skip_arg];
  } else {
    return Qnil;
  }
}

TypeInfo TypeInfo_FromClass(int argc, VALUE* argv, int skip_arg,
                            VALUE* type_class, VALUE* init_arg) {
  TypeInfo ret = {ruby_to_fieldtype(argv[skip_arg])};

  if (ret.type == kUpb_CType_Message || ret.type == kUpb_CType_Enum) {
    *init_arg = TypeInfo_InitArg(argc, argv, skip_arg + 2);

    if (argc < 2 + skip_arg) {
      rb_raise(rb_eArgError, "Expected at least %d arguments for message/enum.",
               2 + skip_arg);
    }

    VALUE klass = argv[1 + skip_arg];
    VALUE desc = MessageOrEnum_GetDescriptor(klass);
    *type_class = klass;

    if (desc == Qnil) {
      rb_raise(rb_eArgError,
               "Type class has no descriptor. Please pass a "
               "class or enum as returned by the DescriptorPool.");
    }

    if (ret.type == kUpb_CType_Message) {
      ret.def.msgdef = ruby_to_Descriptor(desc)->msgdef;
      Message_CheckClass(klass);
    } else {
      PBRUBY_ASSERT(ret.type == kUpb_CType_Enum);
      ret.def.enumdef = ruby_to_EnumDescriptor(desc)->enumdef;
    }
  } else {
    *init_arg = TypeInfo_InitArg(argc, argv, skip_arg + 1);
  }

  return ret;
}

void Defs_register(VALUE module) {
  DescriptorPool_register(module);
  Descriptor_register(module);
  FileDescriptor_register(module);
  FieldDescriptor_register(module);
  OneofDescriptor_register(module);
  EnumDescriptor_register(module);

  rb_gc_register_address(&c_only_cookie);
  c_only_cookie = rb_class_new_instance(0, NULL, rb_cObject);
}
