// Protocol Buffers - Google's data interchange format
// Copyright 2014 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include <ctype.h>
#include <errno.h>

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
static VALUE get_servicedef_obj(VALUE descriptor_pool,
                                const upb_ServiceDef* def);
static VALUE get_methoddef_obj(VALUE descriptor_pool, const upb_MethodDef* def);

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
static ID options_instancevar_interned;
// -----------------------------------------------------------------------------
// DescriptorPool.
// -----------------------------------------------------------------------------

typedef struct {
  // IMPORTANT: WB_PROTECTED objects must only use the RB_OBJ_WRITE()
  // macro to update VALUE references, as to trigger write barriers.
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
    .flags = RUBY_TYPED_FREE_IMMEDIATELY | RUBY_TYPED_WB_PROTECTED,
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

/**
 * ruby-doc: DescriptorPool
 *
 * A DescriptorPool is the registry of all known Protobuf descriptor objects.
 *
 */

/*
 * ruby-doc: DescriptorPool.new
 *
 * Creates a new, empty, descriptor pool.
 */
static VALUE DescriptorPool_alloc(VALUE klass) {
  DescriptorPool* self = ALLOC(DescriptorPool);
  VALUE ret;

  self->def_to_descriptor = Qnil;
  ret = TypedData_Wrap_Struct(klass, &DescriptorPool_type, self);

  RB_OBJ_WRITE(ret, &self->def_to_descriptor, rb_hash_new());
  self->symtab = upb_DefPool_New();
  return ObjectCache_TryAdd(self->symtab, ret);
}

/*
 * ruby-doc: DescriptorPool#add_serialized_file
 *
 * Adds the given serialized
 * {https://protobuf.com/docs/descriptors#file-descriptors FileDescriptorProto}
 * to the pool.
 *
 * @param serialized_file_proto [String]
 * @return [FileDescriptor]
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
 * ruby-doc: DescriptorPool#lookup
 *
 * Finds a {Descriptor}, {EnumDescriptor},
 * {FieldDescriptor} or {ServiceDescriptor} by
 * name and returns it, or nil if none exists with the given name.
 *
 * @param name [String]
 * @return [Descriptor,EnumDescriptor,FieldDescriptor,ServiceDescriptor]
 */
static VALUE DescriptorPool_lookup(VALUE _self, VALUE name) {
  DescriptorPool* self = ruby_to_DescriptorPool(_self);
  const char* name_str = get_str(name);
  const upb_MessageDef* msgdef;
  const upb_EnumDef* enumdef;
  const upb_FieldDef* fielddef;
  const upb_ServiceDef* servicedef;
  const upb_FileDef* filedef;

  msgdef = upb_DefPool_FindMessageByName(self->symtab, name_str);
  if (msgdef) {
    return get_msgdef_obj(_self, msgdef);
  }

  fielddef = upb_DefPool_FindExtensionByName(self->symtab, name_str);
  if (fielddef) {
    return get_fielddef_obj(_self, fielddef);
  }

  enumdef = upb_DefPool_FindEnumByName(self->symtab, name_str);
  if (enumdef) {
    return get_enumdef_obj(_self, enumdef);
  }

  servicedef = upb_DefPool_FindServiceByName(self->symtab, name_str);
  if (servicedef) {
    return get_servicedef_obj(_self, servicedef);
  }

  filedef = upb_DefPool_FindFileByName(self->symtab, name_str);
  if (filedef) {
    return get_filedef_obj(_self, filedef);
  }

  return Qnil;
}

/*
 * ruby-doc: DescriptorPool.generated_pool
 *
 * Class method that returns the global {DescriptorPool}. This is a singleton
 * into which generated-code message and enum types are registered. The user may
 * also register types in this pool for convenience so that they do not have to
 * hold a reference to a private pool instance.
 *
 * @return [DescriptorPool]
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
  options_instancevar_interned = rb_intern("options");
}

// -----------------------------------------------------------------------------
// Descriptor.
// -----------------------------------------------------------------------------

typedef struct {
  const upb_MessageDef* msgdef;
  // IMPORTANT: WB_PROTECTED objects must only use the RB_OBJ_WRITE()
  // macro to update VALUE references, as to trigger write barriers.
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
    .flags = RUBY_TYPED_FREE_IMMEDIATELY | RUBY_TYPED_WB_PROTECTED,
};

static Descriptor* ruby_to_Descriptor(VALUE val) {
  Descriptor* ret;
  TypedData_Get_Struct(val, Descriptor, &Descriptor_type, ret);
  return ret;
}

// Decode and return a frozen instance of a Descriptor Option for the given pool
static VALUE decode_options(VALUE self, const char* option_type, int size,
                            const char* bytes, VALUE descriptor_pool) {
  VALUE options_rb = rb_ivar_get(self, options_instancevar_interned);
  if (options_rb != Qnil) {
    return options_rb;
  }

  static const char* prefix = "google.protobuf.";
  char fullname
      [/*strlen(prefix)*/ 16 +
       /*strln(longest option type supported e.g. "MessageOptions")*/ 14 +
       /*null terminator*/ 1];

  snprintf(fullname, sizeof(fullname), "%s%s", prefix, option_type);
  const upb_MessageDef* msgdef = upb_DefPool_FindMessageByName(
      ruby_to_DescriptorPool(descriptor_pool)->symtab, fullname);
  if (!msgdef) {
    rb_raise(rb_eRuntimeError, "Cannot find %s in DescriptorPool", option_type);
  }

  VALUE desc_rb = get_msgdef_obj(descriptor_pool, msgdef);
  const Descriptor* desc = ruby_to_Descriptor(desc_rb);

  options_rb = Message_decode_bytes(size, bytes, 0, desc->klass, false);

  // Strip features from the options proto to keep it internal.
  const upb_MessageDef* decoded_desc = NULL;
  upb_Message* options = Message_GetMutable(options_rb, &decoded_desc);
  PBRUBY_ASSERT(options != NULL);
  PBRUBY_ASSERT(decoded_desc == msgdef);
  const upb_FieldDef* field =
      upb_MessageDef_FindFieldByName(decoded_desc, "features");
  PBRUBY_ASSERT(field != NULL);
  upb_Message_ClearFieldByDef(options, field);

  Message_freeze(options_rb);

  rb_ivar_set(self, options_instancevar_interned, options_rb);
  return options_rb;
}

/*
 * ruby-doc: Descriptor
 *
 * A Descriptor provides information about a given Protobuf definition.
 */

/*
 * ruby-doc: Descriptor.initialize
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

  RB_OBJ_WRITE(_self, &self->descriptor_pool, descriptor_pool);
  self->msgdef = (const upb_MessageDef*)NUM2ULL(ptr);

  return Qnil;
}

/*
 * ruby-doc: Descriptor#file_descriptor
 *
 * Returns the {FileDescriptor} object this message belongs to.
 *
 * @return [FileDescriptor]
 */
static VALUE Descriptor_file_descriptor(VALUE _self) {
  Descriptor* self = ruby_to_Descriptor(_self);
  return get_filedef_obj(self->descriptor_pool,
                         upb_MessageDef_File(self->msgdef));
}

/*
 * ruby-doc: Descriptor#name
 *
 * Returns the name of this message type as a fully-qualified string (e.g.,
 * My.Package.MessageType).
 *
 * @return [String]
 */
static VALUE Descriptor_name(VALUE _self) {
  Descriptor* self = ruby_to_Descriptor(_self);
  return rb_str_maybe_null(upb_MessageDef_FullName(self->msgdef));
}

/*
 * ruby-doc: Descriptor#each
 *
 * Iterates over fields in this message type, yielding to the block on each one.
 *
 * @yield [FieldDescriptor]
 * @return [nil]
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
 * ruby-doc: Descriptor#lookup
 *
 * Returns the field descriptor for the field with the given name, if present,
 * or nil if none.
 *
 * @param name [String]
 * @return [FieldDescriptor]
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
 * ruby-doc: Descriptor#each_oneof
 *
 * Invokes the given block for each oneof in this message type, passing the
 * corresponding {OneofDescriptor}.
 *
 * @yield [OneofDescriptor]
 * @return [nil]
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
 * ruby-doc: Descriptor#lookup_oneof
 *
 * Returns the oneof descriptor for the oneof with the given name, if present,
 * or nil if none.
 *
 * @param name [String]
 * @return [OneofDescriptor]
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
 * ruby-doc: Descriptor#msgclass
 *
 * Returns the Ruby class created for this message type.
 *
 * @return [Class<Google::Protobuf::AbstractMessage>]
 */
static VALUE Descriptor_msgclass(VALUE _self) {
  Descriptor* self = ruby_to_Descriptor(_self);
  if (self->klass == Qnil) {
    RB_OBJ_WRITE(_self, &self->klass, build_class_from_descriptor(_self));
  }
  return self->klass;
}

/*
 * ruby-doc: Descriptor#options
 *
 * Returns the
 * {https://github.com/protocolbuffers/protobuf/blob/v30.2/src/google/protobuf/descriptor.proto#L571
 * MessageOptions} for this {Descriptor}.
 *
 * @return [MessageOptions]
 */
static VALUE Descriptor_options(VALUE _self) {
  Descriptor* self = ruby_to_Descriptor(_self);
  const google_protobuf_MessageOptions* opts =
      upb_MessageDef_Options(self->msgdef);
  upb_Arena* arena = upb_Arena_New();
  size_t size;
  char* serialized =
      google_protobuf_MessageOptions_serialize(opts, arena, &size);
  VALUE message_options = decode_options(_self, "MessageOptions", size,
                                         serialized, self->descriptor_pool);
  upb_Arena_Free(arena);
  return message_options;
}

/*
 * ruby-doc: Descriptor#to_proto
 *
 * Returns the
 * {https://github.com/protocolbuffers/protobuf/blob/v30.2/src/google/protobuf/descriptor.proto#L147
 * DescriptorProto} of this {Descriptor}.
 *
 * @return [DescriptorProto]
 */
static VALUE Descriptor_to_proto(VALUE _self) {
  Descriptor* self = ruby_to_Descriptor(_self);
  upb_Arena* arena = upb_Arena_New();
  google_protobuf_DescriptorProto* proto =
      upb_MessageDef_ToProto(self->msgdef, arena);
  size_t size;
  const char* serialized =
      google_protobuf_DescriptorProto_serialize(proto, arena, &size);
  VALUE proto_class = rb_path2class("Google::Protobuf::DescriptorProto");
  VALUE proto_rb =
      Message_decode_bytes(size, serialized, 0, proto_class, false);
  upb_Arena_Free(arena);
  return proto_rb;
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
  rb_define_method(klass, "options", Descriptor_options, 0);
  rb_define_method(klass, "to_proto", Descriptor_to_proto, 0);
  rb_include_module(klass, rb_mEnumerable);
  rb_gc_register_address(&cDescriptor);
  cDescriptor = klass;
}

// -----------------------------------------------------------------------------
// FileDescriptor.
// -----------------------------------------------------------------------------

typedef struct {
  const upb_FileDef* filedef;
  // IMPORTANT: WB_PROTECTED objects must only use the RB_OBJ_WRITE()
  // macro to update VALUE references, as to trigger write barriers.
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
    .flags = RUBY_TYPED_FREE_IMMEDIATELY | RUBY_TYPED_WB_PROTECTED,
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

/**
 * ruby-doc: FileDescriptor
 *
 * A FileDescriptor provides information about all Protobuf definitions in a
 * particular file.
 */

/*
 * ruby-doc: FileDescriptor#initialize
 *
 * Returns a new file descriptor. May
 * to a builder.
 */
static VALUE FileDescriptor_initialize(VALUE _self, VALUE cookie,
                                       VALUE descriptor_pool, VALUE ptr) {
  FileDescriptor* self = ruby_to_FileDescriptor(_self);

  if (cookie != c_only_cookie) {
    rb_raise(rb_eRuntimeError,
             "Descriptor objects may not be created from Ruby.");
  }

  RB_OBJ_WRITE(_self, &self->descriptor_pool, descriptor_pool);
  self->filedef = (const upb_FileDef*)NUM2ULL(ptr);

  return Qnil;
}

/*
 * ruby-doc: FileDescriptor#name
 *
 * Returns the name of the file.
 *
 * @return [String]
 */
static VALUE FileDescriptor_name(VALUE _self) {
  FileDescriptor* self = ruby_to_FileDescriptor(_self);
  const char* name = upb_FileDef_Name(self->filedef);
  return name == NULL ? Qnil : rb_str_new2(name);
}

/*
 * ruby-doc: FileDescriptor#options
 *
 * Returns the
 * {https://github.com/protocolbuffers/protobuf/blob/v30.2/src/google/protobuf/descriptor.proto#L442
 * FileOptions} for this {FileDescriptor}.
 *
 * @return [FileOptions]
 */
static VALUE FileDescriptor_options(VALUE _self) {
  FileDescriptor* self = ruby_to_FileDescriptor(_self);
  const google_protobuf_FileOptions* opts = upb_FileDef_Options(self->filedef);
  upb_Arena* arena = upb_Arena_New();
  size_t size;
  char* serialized = google_protobuf_FileOptions_serialize(opts, arena, &size);
  VALUE file_options = decode_options(_self, "FileOptions", size, serialized,
                                      self->descriptor_pool);
  upb_Arena_Free(arena);
  return file_options;
}

/*
 * ruby-doc: FileDescriptor#to_proto
 *
 * Returns the
 * {https://github.com/protocolbuffers/protobuf/blob/v30.2/src/google/protobuf/descriptor.proto#L104
 * FileDescriptorProto} of this {FileDescriptor}.
 *
 * @return [FileDescriptorProto]
 */
static VALUE FileDescriptor_to_proto(VALUE _self) {
  FileDescriptor* self = ruby_to_FileDescriptor(_self);
  upb_Arena* arena = upb_Arena_New();
  google_protobuf_FileDescriptorProto* file_proto =
      upb_FileDef_ToProto(self->filedef, arena);

  size_t size;
  const char* serialized =
      google_protobuf_FileDescriptorProto_serialize(file_proto, arena, &size);

  VALUE file_proto_class =
      rb_path2class("Google::Protobuf::FileDescriptorProto");
  VALUE proto_rb =
      Message_decode_bytes(size, serialized, 0, file_proto_class, false);
  upb_Arena_Free(arena);
  return proto_rb;
}

static void FileDescriptor_register(VALUE module) {
  VALUE klass = rb_define_class_under(module, "FileDescriptor", rb_cObject);
  rb_define_alloc_func(klass, FileDescriptor_alloc);
  rb_define_method(klass, "initialize", FileDescriptor_initialize, 3);
  rb_define_method(klass, "name", FileDescriptor_name, 0);
  rb_define_method(klass, "options", FileDescriptor_options, 0);
  rb_define_method(klass, "to_proto", FileDescriptor_to_proto, 0);
  rb_gc_register_address(&cFileDescriptor);
  cFileDescriptor = klass;
}

// -----------------------------------------------------------------------------
// FieldDescriptor.
// -----------------------------------------------------------------------------

typedef struct {
  const upb_FieldDef* fielddef;
  // IMPORTANT: WB_PROTECTED objects must only use the RB_OBJ_WRITE()
  // macro to update VALUE references, as to trigger write barriers.
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
    .flags = RUBY_TYPED_FREE_IMMEDIATELY | RUBY_TYPED_WB_PROTECTED,
};

static FieldDescriptor* ruby_to_FieldDescriptor(VALUE val) {
  FieldDescriptor* ret;
  TypedData_Get_Struct(val, FieldDescriptor, &FieldDescriptor_type, ret);
  return ret;
}

/**
 * ruby-doc: FieldDescriptor
 *
 * A FieldDescriptor provides information about the Protobuf definition of a
 * field inside a {Descriptor}.
 */

/*
 * ruby-doc: FieldDescriptor#initialize
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
 *    FieldDescriptor.new(c_only_cookie, pool, ptr) => FieldDescriptor
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

  RB_OBJ_WRITE(_self, &self->descriptor_pool, descriptor_pool);
  self->fielddef = (const upb_FieldDef*)NUM2ULL(ptr);

  return Qnil;
}

/*
 * ruby-doc: FieldDescriptor#name
 *
 * Returns the name of this field.
 *
 * @return [String]
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
    return kUpb_CType_##upb;              \
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
#define CONVERT(upb, ruby)   \
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
 * ruby-doc: FieldDescriptor#type
 *
 * Returns this field's type, as a Ruby symbol, or nil if not yet set.
 *
 * Valid field types are:
 *     :int32, :int64, :uint32, :uint64, :float, :double, :bool, :string,
 *     :bytes, :message.
 *
 * @return [Symbol]
 */
static VALUE FieldDescriptor__type(VALUE _self) {
  FieldDescriptor* self = ruby_to_FieldDescriptor(_self);
  return descriptortype_to_ruby(upb_FieldDef_Type(self->fielddef));
}

/*
 * ruby-doc: FieldDescriptor#default
 *
 * Returns this field's default, as a Ruby object, or nil if not yet set.
 *
 * @return [Object,nil]
 */
static VALUE FieldDescriptor_default(VALUE _self) {
  FieldDescriptor* self = ruby_to_FieldDescriptor(_self);
  const upb_FieldDef* f = self->fielddef;
  upb_MessageValue default_val = upb_MessageValue_Zero();
  if (upb_FieldDef_IsSubMessage(f)) {
    return Qnil;
  } else if (!upb_FieldDef_IsRepeated(f)) {
    default_val = upb_FieldDef_Default(f);
  }
  return Convert_UpbToRuby(default_val, TypeInfo_get(self->fielddef), Qnil);
}

/*
 * ruby-doc: FieldDescriptor.has_presence?
 *
 * Returns whether this field tracks presence.
 *
 * @return [Boolean]
 */
static VALUE FieldDescriptor_has_presence(VALUE _self) {
  FieldDescriptor* self = ruby_to_FieldDescriptor(_self);
  return upb_FieldDef_HasPresence(self->fielddef) ? Qtrue : Qfalse;
}

/*
 * ruby-doc: FieldDescriptor#required?
 *
 * Returns whether this is a required field.
 *
 * @return [Boolean]
 */
static VALUE FieldDescriptor_is_required(VALUE _self) {
  FieldDescriptor* self = ruby_to_FieldDescriptor(_self);
  return upb_FieldDef_IsRequired(self->fielddef) ? Qtrue : Qfalse;
}

/*
 * ruby-doc: FieldDescriptor#repeated?
 *
 * Returns whether this is a repeated field.
 * @return [Boolean]
 */
static VALUE FieldDescriptor_is_repeated(VALUE _self) {
  FieldDescriptor* self = ruby_to_FieldDescriptor(_self);
  return upb_FieldDef_IsRepeated(self->fielddef) ? Qtrue : Qfalse;
}

/*
 * ruby-doc: FieldDescriptor#is_packed?
 *
 * Returns whether this is a repeated field that uses packed encoding.
 * @return [Boolean]
 */
static VALUE FieldDescriptor_is_packed(VALUE _self) {
  FieldDescriptor* self = ruby_to_FieldDescriptor(_self);
  return upb_FieldDef_IsPacked(self->fielddef) ? Qtrue : Qfalse;
}

/*
 * ruby-doc: FieldDescriptor#json_name
 *
 * Returns this field's json_name, as a Ruby string, or nil if not yet set.
 *
 * @return [String,nil]
 */
static VALUE FieldDescriptor_json_name(VALUE _self) {
  FieldDescriptor* self = ruby_to_FieldDescriptor(_self);
  const upb_FieldDef* f = self->fielddef;
  const char* json_name = upb_FieldDef_JsonName(f);
  return rb_str_new2(json_name);
}

/*
 * ruby-doc: FieldDescriptor#label
 *
 * Returns this field's label (i.e., plurality), as a Ruby symbol.
 * Valid field labels are:
 *   :optional, :repeated
 *
 * @return [Symbol]
 * @deprecated Use {#repeated?} or {#required?} instead.
 */
static VALUE FieldDescriptor_label(VALUE _self) {
  FieldDescriptor* self = ruby_to_FieldDescriptor(_self);
  switch (upb_FieldDef_Label(self->fielddef)) {
#define CONVERT(upb, ruby) \
  case kUpb_Label_##upb:   \
    return ID2SYM(rb_intern(#ruby));

    CONVERT(Optional, optional);
    CONVERT(Required, required);
    CONVERT(Repeated, repeated);

#undef CONVERT
  }

  return Qnil;
}

/*
 * ruby-doc: FieldDescriptor#number
 *
 * Returns the tag number for this field.
 *
 * @return [Integer]
 */
static VALUE FieldDescriptor_number(VALUE _self) {
  FieldDescriptor* self = ruby_to_FieldDescriptor(_self);
  return INT2NUM(upb_FieldDef_Number(self->fielddef));
}

/*
 * ruby-doc: FieldDescriptor#submsg_name
 *
 * Returns the name of the message or enum type corresponding to this field, if
 * it is a message or enum field (respectively), or nil otherwise. This type
 * name will be resolved within the context of the pool to which the containing
 * message type is added.
 *
 * @return [String,nil]
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
 * ruby-doc: FieldDescriptor#subtype
 *
 * Returns the message or enum descriptor corresponding to this field's type if
 * it is a message or enum field, respectively, or nil otherwise. Cannot be
 * called *until* the containing message type is added to a pool (and thus
 * resolved).
 *
 * @return [Descriptor,EnumDescriptor,nil]
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
 * ruby-doc: FieldDescriptor#get
 *
 * Returns the value set for this field on the given message. Raises an
 * exception if message is of the wrong type.
 *
 * @param message [AbstractMessage]
 * @return [Object]
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
 * ruby-doc: FieldDescriptor.has?
 *
 * Returns whether the value is set on the given message. Raises an
 * exception when calling for fields that do not have presence.
 *
 * @param message [AbstractMessage]
 * @return [Boolean]
 */
static VALUE FieldDescriptor_has(VALUE _self, VALUE msg_rb) {
  FieldDescriptor* self = ruby_to_FieldDescriptor(_self);
  const upb_MessageDef* m;
  const upb_Message* msg = Message_Get(msg_rb, &m);

  if (m != upb_FieldDef_ContainingType(self->fielddef)) {
    rb_raise(cTypeError, "has method called on wrong message type");
  } else if (!upb_FieldDef_HasPresence(self->fielddef)) {
    rb_raise(rb_eArgError, "does not track presence");
  }

  return upb_Message_HasFieldByDef(msg, self->fielddef) ? Qtrue : Qfalse;
}

/*
 * ruby-doc: FieldDescriptor#clear
 *
 * Clears the field from the message if it's set.
 *
 * @param message [AbstractMessage]
 * @return [nil]
 */
static VALUE FieldDescriptor_clear(VALUE _self, VALUE msg_rb) {
  FieldDescriptor* self = ruby_to_FieldDescriptor(_self);
  const upb_MessageDef* m;
  upb_Message* msg = Message_GetMutable(msg_rb, &m);

  if (m != upb_FieldDef_ContainingType(self->fielddef)) {
    rb_raise(cTypeError, "has method called on wrong message type");
  }

  upb_Message_ClearFieldByDef(msg, self->fielddef);
  return Qnil;
}

/*
 * ruby-doc: FieldDescriptor#set
 *
 * Sets the value corresponding to this field to the given value on the given
 * message. Raises an exception if message is of the wrong type. Performs the
 * ordinary type-checks for field setting.
 *
 * @param message [AbstractMessage]
 * @param value [Object]
 */
static VALUE FieldDescriptor_set(VALUE _self, VALUE msg_rb, VALUE value) {
  FieldDescriptor* self = ruby_to_FieldDescriptor(_self);
  const upb_MessageDef* m;
  upb_Message* msg = Message_GetMutable(msg_rb, &m);
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

/*
 * ruby-doc: FieldDescriptor#options
 *
 * Returns the
 * {https://github.com/protocolbuffers/protobuf/blob/v30.2/src/google/protobuf/descriptor.proto#L656
 * FieldOptions} for this {FieldDescriptor}.
 *
 * @return [FieldOptions]
 */
static VALUE FieldDescriptor_options(VALUE _self) {
  FieldDescriptor* self = ruby_to_FieldDescriptor(_self);
  const google_protobuf_FieldOptions* opts =
      upb_FieldDef_Options(self->fielddef);
  upb_Arena* arena = upb_Arena_New();
  size_t size;
  char* serialized = google_protobuf_FieldOptions_serialize(opts, arena, &size);
  VALUE field_options = decode_options(_self, "FieldOptions", size, serialized,
                                       self->descriptor_pool);
  upb_Arena_Free(arena);
  return field_options;
}

/*
 * ruby-doc: FieldDescriptor#to_proto
 *
 * Returns the
 * {https://github.com/protocolbuffers/protobuf/blob/v30.2/src/google/protobuf/descriptor.proto#L236
 * FieldDescriptorProto} of this {FieldDescriptor}.
 *
 * @return [FieldDescriptorProto]
 */
static VALUE FieldDescriptor_to_proto(VALUE _self) {
  FieldDescriptor* self = ruby_to_FieldDescriptor(_self);
  upb_Arena* arena = upb_Arena_New();
  google_protobuf_FieldDescriptorProto* proto =
      upb_FieldDef_ToProto(self->fielddef, arena);
  size_t size;
  const char* serialized =
      google_protobuf_FieldDescriptorProto_serialize(proto, arena, &size);
  VALUE proto_class = rb_path2class("Google::Protobuf::FieldDescriptorProto");
  VALUE proto_rb =
      Message_decode_bytes(size, serialized, 0, proto_class, false);
  upb_Arena_Free(arena);
  return proto_rb;
}

static void FieldDescriptor_register(VALUE module) {
  VALUE klass = rb_define_class_under(module, "FieldDescriptor", rb_cObject);
  rb_define_alloc_func(klass, FieldDescriptor_alloc);
  rb_define_method(klass, "initialize", FieldDescriptor_initialize, 3);
  rb_define_method(klass, "name", FieldDescriptor_name, 0);
  rb_define_method(klass, "type", FieldDescriptor__type, 0);
  rb_define_method(klass, "default", FieldDescriptor_default, 0);
  rb_define_method(klass, "has_presence?", FieldDescriptor_has_presence, 0);
  rb_define_method(klass, "required?", FieldDescriptor_is_required, 0);
  rb_define_method(klass, "repeated?", FieldDescriptor_is_repeated, 0);
  rb_define_method(klass, "is_packed?", FieldDescriptor_is_packed, 0);
  rb_define_method(klass, "json_name", FieldDescriptor_json_name, 0);
  rb_define_method(klass, "label", FieldDescriptor_label, 0);
  rb_define_method(klass, "number", FieldDescriptor_number, 0);
  rb_define_method(klass, "submsg_name", FieldDescriptor_submsg_name, 0);
  rb_define_method(klass, "subtype", FieldDescriptor_subtype, 0);
  rb_define_method(klass, "has?", FieldDescriptor_has, 1);
  rb_define_method(klass, "clear", FieldDescriptor_clear, 1);
  rb_define_method(klass, "get", FieldDescriptor_get, 1);
  rb_define_method(klass, "set", FieldDescriptor_set, 2);
  rb_define_method(klass, "options", FieldDescriptor_options, 0);
  rb_define_method(klass, "to_proto", FieldDescriptor_to_proto, 0);
  rb_gc_register_address(&cFieldDescriptor);
  cFieldDescriptor = klass;
}

// -----------------------------------------------------------------------------
// OneofDescriptor.
// -----------------------------------------------------------------------------

typedef struct {
  const upb_OneofDef* oneofdef;
  // IMPORTANT: WB_PROTECTED objects must only use the RB_OBJ_WRITE()
  // macro to update VALUE references, as to trigger write barriers.
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
    .flags = RUBY_TYPED_FREE_IMMEDIATELY | RUBY_TYPED_WB_PROTECTED,
};

static OneofDescriptor* ruby_to_OneofDescriptor(VALUE val) {
  OneofDescriptor* ret;
  TypedData_Get_Struct(val, OneofDescriptor, &OneofDescriptor_type, ret);
  return ret;
}

/**
 * ruby-doc: OneofDescriptor
 *
 * A OneofDescriptor provides information about the Protobuf definition of a
 * oneof inside a {Descriptor}.
 */

/*
 * ruby-doc: OneofDescriptor#initialize
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

  RB_OBJ_WRITE(_self, &self->descriptor_pool, descriptor_pool);
  self->oneofdef = (const upb_OneofDef*)NUM2ULL(ptr);

  return Qnil;
}

/*
 * ruby-doc: OneofDescriptor#name
 *
 * Returns the name of this oneof.
 *
 * @return [String]
 */
static VALUE OneofDescriptor_name(VALUE _self) {
  OneofDescriptor* self = ruby_to_OneofDescriptor(_self);
  return rb_str_maybe_null(upb_OneofDef_Name(self->oneofdef));
}

/*
 * ruby-doc: OneofDescriptor#each
 *
 * Iterates through fields in this oneof, yielding to the block on each one.
 *
 * @yield [FieldDescriptor]
 * @return [nil]
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

/*
 * ruby-doc: OneofDescriptor#options
 *
 * Returns the
 * {https://github.com/protocolbuffers/protobuf/blob/v30.2/src/google/protobuf/descriptor.proto#L824
 * OneofOptions} for this {OneofDescriptor}.
 *
 * @return [OneofOptions]
 */
static VALUE OneOfDescriptor_options(VALUE _self) {
  OneofDescriptor* self = ruby_to_OneofDescriptor(_self);
  const google_protobuf_OneofOptions* opts =
      upb_OneofDef_Options(self->oneofdef);
  upb_Arena* arena = upb_Arena_New();
  size_t size;
  char* serialized = google_protobuf_OneofOptions_serialize(opts, arena, &size);
  VALUE oneof_options = decode_options(_self, "OneofOptions", size, serialized,
                                       self->descriptor_pool);
  upb_Arena_Free(arena);
  return oneof_options;
}

/*
 * ruby-doc: OneofDescriptor#to_proto
 *
 * Returns the
 * {https://github.com/protocolbuffers/protobuf/blob/v30.2/src/google/protobuf/descriptor.proto#L343
 * OneofDescriptorProto} of this {OneofDescriptor}.
 *
 * @return [OneofDescriptorProto]
 */
static VALUE OneOfDescriptor_to_proto(VALUE _self) {
  OneofDescriptor* self = ruby_to_OneofDescriptor(_self);
  upb_Arena* arena = upb_Arena_New();
  google_protobuf_OneofDescriptorProto* proto =
      upb_OneofDef_ToProto(self->oneofdef, arena);
  size_t size;
  const char* serialized =
      google_protobuf_OneofDescriptorProto_serialize(proto, arena, &size);
  VALUE proto_class = rb_path2class("Google::Protobuf::OneofDescriptorProto");
  VALUE proto_rb =
      Message_decode_bytes(size, serialized, 0, proto_class, false);
  upb_Arena_Free(arena);
  return proto_rb;
}

static void OneofDescriptor_register(VALUE module) {
  VALUE klass = rb_define_class_under(module, "OneofDescriptor", rb_cObject);
  rb_define_alloc_func(klass, OneofDescriptor_alloc);
  rb_define_method(klass, "initialize", OneofDescriptor_initialize, 3);
  rb_define_method(klass, "name", OneofDescriptor_name, 0);
  rb_define_method(klass, "each", OneofDescriptor_each, 0);
  rb_define_method(klass, "options", OneOfDescriptor_options, 0);
  rb_define_method(klass, "to_proto", OneOfDescriptor_to_proto, 0);
  rb_include_module(klass, rb_mEnumerable);
  rb_gc_register_address(&cOneofDescriptor);
  cOneofDescriptor = klass;
}

// -----------------------------------------------------------------------------
// EnumDescriptor.
// -----------------------------------------------------------------------------

typedef struct {
  const upb_EnumDef* enumdef;
  // IMPORTANT: WB_PROTECTED objects must only use the RB_OBJ_WRITE()
  // macro to update VALUE references, as to trigger write barriers.
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
    .flags = RUBY_TYPED_FREE_IMMEDIATELY | RUBY_TYPED_WB_PROTECTED,
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

/**
 * ruby-doc: EnumDescriptor
 *
 * An EnumDescriptor provides information about the Protobuf definition of an
 * enum inside a {Descriptor}.
 */

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

  RB_OBJ_WRITE(_self, &self->descriptor_pool, descriptor_pool);
  self->enumdef = (const upb_EnumDef*)NUM2ULL(ptr);

  return Qnil;
}

/*
 * ruby-doc: EnumDescriptor#file_descriptor
 *
 * Returns the {FileDescriptor} object this enum belongs to.
 *
 * @return [FileDescriptor]
 */
static VALUE EnumDescriptor_file_descriptor(VALUE _self) {
  EnumDescriptor* self = ruby_to_EnumDescriptor(_self);
  return get_filedef_obj(self->descriptor_pool,
                         upb_EnumDef_File(self->enumdef));
}

/*
 * ruby-doc: EnumDescriptor#is_closed?
 *
 * Returns whether this enum is open or closed.
 *
 * @return [Boolean]
 */
static VALUE EnumDescriptor_is_closed(VALUE _self) {
  EnumDescriptor* self = ruby_to_EnumDescriptor(_self);
  return upb_EnumDef_IsClosed(self->enumdef) ? Qtrue : Qfalse;
}

/*
 * ruby-doc: EnumDescriptor#name
 *
 * Returns the name of this enum type.
 *
 * @return [String]
 */
static VALUE EnumDescriptor_name(VALUE _self) {
  EnumDescriptor* self = ruby_to_EnumDescriptor(_self);
  return rb_str_maybe_null(upb_EnumDef_FullName(self->enumdef));
}

/*
 * ruby-doc: EnumDescriptor#lookup_name
 *
 * Returns the numeric value corresponding to the given key name (as a Ruby
 * symbol), or nil if none.
 *
 * @param name [Symbol]
 * @return [Integer,nil]
 */
static VALUE EnumDescriptor_lookup_name(VALUE _self, VALUE name) {
  EnumDescriptor* self = ruby_to_EnumDescriptor(_self);
  const char* name_str = rb_id2name(SYM2ID(name));
  const upb_EnumValueDef* ev =
      upb_EnumDef_FindValueByName(self->enumdef, name_str);
  if (ev) {
    return INT2NUM(upb_EnumValueDef_Number(ev));
  } else {
    return Qnil;
  }
}

/*
 * ruby-doc: EnumDescriptor#lookup_value
 *
 * Returns the key name (as a Ruby symbol) corresponding to the integer value,
 * or nil if none.
 *
 * @param name [Integer]
 * @return [Symbol,nil]
 */
static VALUE EnumDescriptor_lookup_value(VALUE _self, VALUE number) {
  EnumDescriptor* self = ruby_to_EnumDescriptor(_self);
  int32_t val = NUM2INT(number);
  const upb_EnumValueDef* ev =
      upb_EnumDef_FindValueByNumber(self->enumdef, val);
  if (ev) {
    return ID2SYM(rb_intern(upb_EnumValueDef_Name(ev)));
  } else {
    return Qnil;
  }
}

/*
 * ruby-doc: EnumDescriptor#each
 *
 * Iterates over key => value mappings in this enum's definition, yielding to
 * the block with (key, value) arguments for each one.
 *
 * @yield [Symbol, Integer]
 * @return [nil]
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
 * ruby-doc: EnumDescriptor#enummodule
 *
 * Returns the Ruby module corresponding to this enum type.
 *
 * @return [Module]
 */
static VALUE EnumDescriptor_enummodule(VALUE _self) {
  EnumDescriptor* self = ruby_to_EnumDescriptor(_self);
  if (self->module == Qnil) {
    RB_OBJ_WRITE(_self, &self->module, build_module_from_enumdesc(_self));
  }
  return self->module;
}

/*
 * ruby-doc: EnumDescriptor#options
 *
 * Returns the
 * {https://github.com/protocolbuffers/protobuf/blob/v30.2/src/google/protobuf/descriptor.proto#L838
 * EnumOptions} for this {EnumDescriptor}.
 *
 * @return [EnumOptions]
 */
static VALUE EnumDescriptor_options(VALUE _self) {
  EnumDescriptor* self = ruby_to_EnumDescriptor(_self);
  const google_protobuf_EnumOptions* opts = upb_EnumDef_Options(self->enumdef);
  upb_Arena* arena = upb_Arena_New();
  size_t size;
  char* serialized = google_protobuf_EnumOptions_serialize(opts, arena, &size);
  VALUE enum_options = decode_options(_self, "EnumOptions", size, serialized,
                                      self->descriptor_pool);
  upb_Arena_Free(arena);
  return enum_options;
}

/*
 * ruby-doc: EnumDescriptor#to_proto
 *
 * Returns the
 * {https://github.com/protocolbuffers/protobuf/blob/v30.2/src/google/protobuf/descriptor.proto#L349
 * EnumDescriptorProto} of this {EnumDescriptor}.
 * @return [EnumDescriptorProto]
 */
static VALUE EnumDescriptor_to_proto(VALUE _self) {
  EnumDescriptor* self = ruby_to_EnumDescriptor(_self);
  upb_Arena* arena = upb_Arena_New();
  google_protobuf_EnumDescriptorProto* proto =
      upb_EnumDef_ToProto(self->enumdef, arena);

  size_t size;
  const char* serialized =
      google_protobuf_EnumDescriptorProto_serialize(proto, arena, &size);

  VALUE proto_class = rb_path2class("Google::Protobuf::EnumDescriptorProto");
  VALUE proto_rb =
      Message_decode_bytes(size, serialized, 0, proto_class, false);
  upb_Arena_Free(arena);
  return proto_rb;
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
  rb_define_method(klass, "is_closed?", EnumDescriptor_is_closed, 0);
  rb_define_method(klass, "options", EnumDescriptor_options, 0);
  rb_define_method(klass, "to_proto", EnumDescriptor_to_proto, 0);
  rb_include_module(klass, rb_mEnumerable);
  rb_gc_register_address(&cEnumDescriptor);
  cEnumDescriptor = klass;
}

// -----------------------------------------------------------------------------
// ServiceDescriptor
// -----------------------------------------------------------------------------

typedef struct {
  const upb_ServiceDef* servicedef;
  // IMPORTANT: WB_PROTECTED objects must only use the RB_OBJ_WRITE()
  // macro to update VALUE references, as to trigger write barriers.
  VALUE module;           // begins as nil
  VALUE descriptor_pool;  // Owns the upb_ServiceDef.
} ServiceDescriptor;

static VALUE cServiceDescriptor = Qnil;

static void ServiceDescriptor_mark(void* _self) {
  ServiceDescriptor* self = _self;
  rb_gc_mark(self->module);
  rb_gc_mark(self->descriptor_pool);
}

static const rb_data_type_t ServiceDescriptor_type = {
    "Google::Protobuf::ServicDescriptor",
    {ServiceDescriptor_mark, RUBY_DEFAULT_FREE, NULL},
    .flags = RUBY_TYPED_FREE_IMMEDIATELY | RUBY_TYPED_WB_PROTECTED,
};

static ServiceDescriptor* ruby_to_ServiceDescriptor(VALUE val) {
  ServiceDescriptor* ret;
  TypedData_Get_Struct(val, ServiceDescriptor, &ServiceDescriptor_type, ret);
  return ret;
}

static VALUE ServiceDescriptor_alloc(VALUE klass) {
  ServiceDescriptor* self = ALLOC(ServiceDescriptor);
  VALUE ret = TypedData_Wrap_Struct(klass, &ServiceDescriptor_type, self);
  self->servicedef = NULL;
  self->module = Qnil;
  self->descriptor_pool = Qnil;
  return ret;
}

/**
 * ruby-doc: ServiceDescriptor
 *
 * A ServiceDescriptor provides information about the Protobuf definition of an
 * RPC service.
 */

/*
 * call-seq:
 *    ServiceDescriptor.new(c_only_cookie, ptr) => ServiceDescriptor
 *
 * Creates a descriptor wrapper object.  May only be called from C.
 */
static VALUE ServiceDescriptor_initialize(VALUE _self, VALUE cookie,
                                          VALUE descriptor_pool, VALUE ptr) {
  ServiceDescriptor* self = ruby_to_ServiceDescriptor(_self);

  if (cookie != c_only_cookie) {
    rb_raise(rb_eRuntimeError,
             "Descriptor objects may not be created from Ruby.");
  }

  RB_OBJ_WRITE(_self, &self->descriptor_pool, descriptor_pool);
  self->servicedef = (const upb_ServiceDef*)NUM2ULL(ptr);

  return Qnil;
}

/*
 * ruby-doc: ServiceDescriptor#name
 *
 * Returns the name of this service.
 *
 * @return [String]
 */
static VALUE ServiceDescriptor_name(VALUE _self) {
  ServiceDescriptor* self = ruby_to_ServiceDescriptor(_self);
  return rb_str_maybe_null(upb_ServiceDef_FullName(self->servicedef));
}

/*
 * ruby-doc: ServiceDescriptor#file_descriptor
 *
 * Returns the {FileDescriptor} object this service belongs to.
 * @return [FileDescriptor]
 */
static VALUE ServiceDescriptor_file_descriptor(VALUE _self) {
  ServiceDescriptor* self = ruby_to_ServiceDescriptor(_self);
  return get_filedef_obj(self->descriptor_pool,
                         upb_ServiceDef_File(self->servicedef));
}

/*
 * ruby-doc: ServiceDescriptor#each
 *
 * Iterates over methods in this service, yielding to the block on each one.
 *
 * @yield [MethodDescriptor]
 * @return [nil]
 */
static VALUE ServiceDescriptor_each(VALUE _self) {
  ServiceDescriptor* self = ruby_to_ServiceDescriptor(_self);

  int n = upb_ServiceDef_MethodCount(self->servicedef);
  for (int i = 0; i < n; i++) {
    const upb_MethodDef* method = upb_ServiceDef_Method(self->servicedef, i);
    VALUE obj = get_methoddef_obj(self->descriptor_pool, method);
    rb_yield(obj);
  }
  return Qnil;
}

/*
 * ruby-doc: ServiceDescriptor#options
 *
 * Returns the
 * {https://github.com/protocolbuffers/protobuf/blob/v30.2/src/google/protobuf/descriptor.proto#L901
 * ServiceOptions} for this {ServiceDescriptor}.
 *
 * @return [ServiceOptions]
 */
static VALUE ServiceDescriptor_options(VALUE _self) {
  ServiceDescriptor* self = ruby_to_ServiceDescriptor(_self);
  const google_protobuf_ServiceOptions* opts =
      upb_ServiceDef_Options(self->servicedef);
  upb_Arena* arena = upb_Arena_New();
  size_t size;
  char* serialized =
      google_protobuf_ServiceOptions_serialize(opts, arena, &size);
  VALUE service_options = decode_options(_self, "ServiceOptions", size,
                                         serialized, self->descriptor_pool);
  upb_Arena_Free(arena);
  return service_options;
}

/*
 * ruby-doc: ServiceDescriptor#to_proto
 *
 * Returns the
 * {https://github.com/protocolbuffers/protobuf/blob/v30.2/src/google/protobuf/descriptor.proto#L386
 * ServiceDescriptorProto} of this {ServiceDescriptor}.
 *
 * @return [ServiceDescriptorProto]
 */
static VALUE ServiceDescriptor_to_proto(VALUE _self) {
  ServiceDescriptor* self = ruby_to_ServiceDescriptor(_self);
  upb_Arena* arena = upb_Arena_New();
  google_protobuf_ServiceDescriptorProto* proto =
      upb_ServiceDef_ToProto(self->servicedef, arena);
  size_t size;
  const char* serialized =
      google_protobuf_ServiceDescriptorProto_serialize(proto, arena, &size);
  VALUE proto_class = rb_path2class("Google::Protobuf::ServiceDescriptorProto");
  VALUE proto_rb =
      Message_decode_bytes(size, serialized, 0, proto_class, false);
  upb_Arena_Free(arena);
  return proto_rb;
}

static void ServiceDescriptor_register(VALUE module) {
  VALUE klass = rb_define_class_under(module, "ServiceDescriptor", rb_cObject);
  rb_define_alloc_func(klass, ServiceDescriptor_alloc);
  rb_define_method(klass, "initialize", ServiceDescriptor_initialize, 3);
  rb_define_method(klass, "name", ServiceDescriptor_name, 0);
  rb_define_method(klass, "each", ServiceDescriptor_each, 0);
  rb_define_method(klass, "file_descriptor", ServiceDescriptor_file_descriptor,
                   0);
  rb_define_method(klass, "options", ServiceDescriptor_options, 0);
  rb_define_method(klass, "to_proto", ServiceDescriptor_to_proto, 0);
  rb_include_module(klass, rb_mEnumerable);
  rb_gc_register_address(&cServiceDescriptor);
  cServiceDescriptor = klass;
}

// -----------------------------------------------------------------------------
// MethodDescriptor
// -----------------------------------------------------------------------------

typedef struct {
  const upb_MethodDef* methoddef;
  // IMPORTANT: WB_PROTECTED objects must only use the RB_OBJ_WRITE()
  // macro to update VALUE references, as to trigger write barriers.
  VALUE module;           // begins as nil
  VALUE descriptor_pool;  // Owns the upb_MethodDef.
} MethodDescriptor;

static VALUE cMethodDescriptor = Qnil;

static void MethodDescriptor_mark(void* _self) {
  MethodDescriptor* self = _self;
  rb_gc_mark(self->module);
  rb_gc_mark(self->descriptor_pool);
}

static const rb_data_type_t MethodDescriptor_type = {
    "Google::Protobuf::MethodDescriptor",
    {MethodDescriptor_mark, RUBY_DEFAULT_FREE, NULL},
    .flags = RUBY_TYPED_FREE_IMMEDIATELY | RUBY_TYPED_WB_PROTECTED,
};

static MethodDescriptor* ruby_to_MethodDescriptor(VALUE val) {
  MethodDescriptor* ret;
  TypedData_Get_Struct(val, MethodDescriptor, &MethodDescriptor_type, ret);
  return ret;
}

static VALUE MethodDescriptor_alloc(VALUE klass) {
  MethodDescriptor* self = ALLOC(MethodDescriptor);
  VALUE ret = TypedData_Wrap_Struct(klass, &MethodDescriptor_type, self);
  self->methoddef = NULL;
  self->module = Qnil;
  self->descriptor_pool = Qnil;
  return ret;
}

/**
 * ruby-doc: MethodDescriptor
 *
 * A MethodDescriptor provides information about the Protobuf definition of a
 * method inside an RPC service.
 */

/*
 * call-seq:
 *    MethodDescriptor.new(c_only_cookie, ptr) => MethodDescriptor
 *
 * Creates a descriptor wrapper object.  May only be called from C.
 */
static VALUE MethodDescriptor_initialize(VALUE _self, VALUE cookie,
                                         VALUE descriptor_pool, VALUE ptr) {
  MethodDescriptor* self = ruby_to_MethodDescriptor(_self);

  if (cookie != c_only_cookie) {
    rb_raise(rb_eRuntimeError,
             "Descriptor objects may not be created from Ruby.");
  }

  RB_OBJ_WRITE(_self, &self->descriptor_pool, descriptor_pool);
  self->methoddef = (const upb_MethodDef*)NUM2ULL(ptr);

  return Qnil;
}

/*
 * ruby-doc: MethodDescriptor#name
 *
 * Returns the name of this method
 *
 * @return [String]
 */
static VALUE MethodDescriptor_name(VALUE _self) {
  MethodDescriptor* self = ruby_to_MethodDescriptor(_self);
  return rb_str_maybe_null(upb_MethodDef_Name(self->methoddef));
}

/*
 * ruby-doc: MethodDescriptor#options
 *
 * Returns the
 * {https://github.com/protocolbuffers/protobuf/blob/v30.2/src/google/protobuf/descriptor.proto#L927
 * MethodOptions} for this {MethodDescriptor}.
 *
 * @return [MethodOptions]
 */
static VALUE MethodDescriptor_options(VALUE _self) {
  MethodDescriptor* self = ruby_to_MethodDescriptor(_self);
  const google_protobuf_MethodOptions* opts =
      upb_MethodDef_Options(self->methoddef);
  upb_Arena* arena = upb_Arena_New();
  size_t size;
  char* serialized =
      google_protobuf_MethodOptions_serialize(opts, arena, &size);
  VALUE method_options = decode_options(_self, "MethodOptions", size,
                                        serialized, self->descriptor_pool);
  upb_Arena_Free(arena);
  return method_options;
}

/*
 * ruby-doc: MethodDescriptor#input_type
 *
 * Returns the {Descriptor} for the request message type of this method
 *
 * @return [Descriptor]
 */
static VALUE MethodDescriptor_input_type(VALUE _self) {
  MethodDescriptor* self = ruby_to_MethodDescriptor(_self);
  const upb_MessageDef* type = upb_MethodDef_InputType(self->methoddef);
  return get_msgdef_obj(self->descriptor_pool, type);
}

/*
 * ruby-doc: MethodDescriptor#output_type
 *
 * Returns the {Descriptor} for the response message type of this method
 *
 * @return [Descriptor]
 */
static VALUE MethodDescriptor_output_type(VALUE _self) {
  MethodDescriptor* self = ruby_to_MethodDescriptor(_self);
  const upb_MessageDef* type = upb_MethodDef_OutputType(self->methoddef);
  return get_msgdef_obj(self->descriptor_pool, type);
}

/*
 * ruby-doc: MethodDescriptor#client_streaming
 *
 * Returns whether or not this is a streaming request method
 *
 * @return [Boolean]
 */
static VALUE MethodDescriptor_client_streaming(VALUE _self) {
  MethodDescriptor* self = ruby_to_MethodDescriptor(_self);
  return upb_MethodDef_ClientStreaming(self->methoddef) ? Qtrue : Qfalse;
}

/*
 * ruby-doc: MethodDescriptor#to_proto
 *
 * Returns the
 * {https://github.com/protocolbuffers/protobuf/blob/v30.2/src/google/protobuf/descriptor.proto#L394
 * MethodDescriptorProto} of this {MethodDescriptor}.
 *
 * @return [MethodDescriptorProto]
 */
static VALUE MethodDescriptor_to_proto(VALUE _self) {
  MethodDescriptor* self = ruby_to_MethodDescriptor(_self);
  upb_Arena* arena = upb_Arena_New();
  google_protobuf_MethodDescriptorProto* proto =
      upb_MethodDef_ToProto(self->methoddef, arena);
  size_t size;
  const char* serialized =
      google_protobuf_MethodDescriptorProto_serialize(proto, arena, &size);
  VALUE proto_class = rb_path2class("Google::Protobuf::MethodDescriptorProto");
  VALUE proto_rb =
      Message_decode_bytes(size, serialized, 0, proto_class, false);
  upb_Arena_Free(arena);
  return proto_rb;
}

/*
 * ruby-doc: MethodDescriptor#server_streaming
 *
 * Returns whether or not this is a streaming response method
 *
 * @return [Boolean]
 */
static VALUE MethodDescriptor_server_streaming(VALUE _self) {
  MethodDescriptor* self = ruby_to_MethodDescriptor(_self);
  return upb_MethodDef_ServerStreaming(self->methoddef) ? Qtrue : Qfalse;
}

static void MethodDescriptor_register(VALUE module) {
  VALUE klass = rb_define_class_under(module, "MethodDescriptor", rb_cObject);
  rb_define_alloc_func(klass, MethodDescriptor_alloc);
  rb_define_method(klass, "initialize", MethodDescriptor_initialize, 3);
  rb_define_method(klass, "name", MethodDescriptor_name, 0);
  rb_define_method(klass, "options", MethodDescriptor_options, 0);
  rb_define_method(klass, "input_type", MethodDescriptor_input_type, 0);
  rb_define_method(klass, "output_type", MethodDescriptor_output_type, 0);
  rb_define_method(klass, "client_streaming", MethodDescriptor_client_streaming,
                   0);
  rb_define_method(klass, "server_streaming", MethodDescriptor_server_streaming,
                   0);
  rb_define_method(klass, "to_proto", MethodDescriptor_to_proto, 0);
  rb_gc_register_address(&cMethodDescriptor);
  cMethodDescriptor = klass;
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

static VALUE get_servicedef_obj(VALUE descriptor_pool,
                                const upb_ServiceDef* def) {
  return get_def_obj(descriptor_pool, def, cServiceDescriptor);
}

static VALUE get_methoddef_obj(VALUE descriptor_pool,
                               const upb_MethodDef* def) {
  return get_def_obj(descriptor_pool, def, cMethodDescriptor);
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
  ServiceDescriptor_register(module);
  MethodDescriptor_register(module);

  rb_gc_register_address(&c_only_cookie);
  c_only_cookie = rb_class_new_instance(0, NULL, rb_cObject);
}
