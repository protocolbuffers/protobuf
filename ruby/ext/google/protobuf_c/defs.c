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

static upb_def* check_notfrozen(const upb_def* def) {
  if (upb_def_isfrozen(def)) {
    rb_raise(rb_eRuntimeError,
             "Attempt to modify a frozen descriptor. Once descriptors are "
             "added to the descriptor pool, they may not be modified.");
  }
  return (upb_def*)def;
}

static upb_msgdef* check_msg_notfrozen(const upb_msgdef* def) {
  return upb_downcast_msgdef_mutable(check_notfrozen((const upb_def*)def));
}

static upb_fielddef* check_field_notfrozen(const upb_fielddef* def) {
  return upb_downcast_fielddef_mutable(check_notfrozen((const upb_def*)def));
}

static upb_oneofdef* check_oneof_notfrozen(const upb_oneofdef* def) {
  return (upb_oneofdef*)check_notfrozen((const upb_def*)def);
}

static upb_enumdef* check_enum_notfrozen(const upb_enumdef* def) {
  return (upb_enumdef*)check_notfrozen((const upb_def*)def);
}

// -----------------------------------------------------------------------------
// DescriptorPool.
// -----------------------------------------------------------------------------

#define DEFINE_CLASS(name, string_name)                             \
    VALUE c ## name = Qnil;                                         \
    const rb_data_type_t _ ## name ## _type = {                     \
      string_name,                                                  \
      { name ## _mark, name ## _free, NULL },                       \
    };                                                              \
    name* ruby_to_ ## name(VALUE val) {                             \
      name* ret;                                                    \
      TypedData_Get_Struct(val, name, &_ ## name ## _type, ret);    \
      return ret;                                                   \
    }                                                               \

#define DEFINE_SELF(type, var, rb_var)                              \
    type* var = ruby_to_ ## type(rb_var)

// Global singleton DescriptorPool. The user is free to create others, but this
// is used by generated code.
VALUE generated_pool;

DEFINE_CLASS(DescriptorPool, "Google::Protobuf::DescriptorPool");

void DescriptorPool_mark(void* _self) {
}

void DescriptorPool_free(void* _self) {
  DescriptorPool* self = _self;
  upb_symtab_free(self->symtab);
  xfree(self);
}

/*
 * call-seq:
 *     DescriptorPool.new => pool
 *
 * Creates a new, empty, descriptor pool.
 */
VALUE DescriptorPool_alloc(VALUE klass) {
  DescriptorPool* self = ALLOC(DescriptorPool);
  self->symtab = upb_symtab_new();
  return TypedData_Wrap_Struct(klass, &_DescriptorPool_type, self);
}

void DescriptorPool_register(VALUE module) {
  VALUE klass = rb_define_class_under(
      module, "DescriptorPool", rb_cObject);
  rb_define_alloc_func(klass, DescriptorPool_alloc);
  rb_define_method(klass, "add", DescriptorPool_add, 1);
  rb_define_method(klass, "build", DescriptorPool_build, -1);
  rb_define_method(klass, "lookup", DescriptorPool_lookup, 1);
  rb_define_singleton_method(klass, "generated_pool",
                             DescriptorPool_generated_pool, 0);
  rb_gc_register_address(&cDescriptorPool);
  cDescriptorPool = klass;

  rb_gc_register_address(&generated_pool);
  generated_pool = rb_class_new_instance(0, NULL, klass);
}

static void add_descriptor_to_pool(DescriptorPool* self,
                                   Descriptor* descriptor) {
  CHECK_UPB(
      upb_symtab_add(self->symtab, (upb_def**)&descriptor->msgdef, 1,
                     NULL, &status),
      "Adding Descriptor to DescriptorPool failed");
}

static void add_enumdesc_to_pool(DescriptorPool* self,
                                 EnumDescriptor* enumdesc) {
  CHECK_UPB(
      upb_symtab_add(self->symtab, (upb_def**)&enumdesc->enumdef, 1,
                     NULL, &status),
      "Adding EnumDescriptor to DescriptorPool failed");
}

/*
 * call-seq:
 *     DescriptorPool.add(descriptor)
 *
 * Adds the given Descriptor or EnumDescriptor to this pool. All references to
 * other types in a Descriptor's fields must be resolvable within this pool or
 * an exception will be raised.
 */
VALUE DescriptorPool_add(VALUE _self, VALUE def) {
  DEFINE_SELF(DescriptorPool, self, _self);
  VALUE def_klass = rb_obj_class(def);
  if (def_klass == cDescriptor) {
    add_descriptor_to_pool(self, ruby_to_Descriptor(def));
  } else if (def_klass == cEnumDescriptor) {
    add_enumdesc_to_pool(self, ruby_to_EnumDescriptor(def));
  } else {
    rb_raise(rb_eArgError,
             "Second argument must be a Descriptor or EnumDescriptor.");
  }
  return Qnil;
}

/*
 * call-seq:
 *     DescriptorPool.build(&block)
 *
 * Invokes the block with a Builder instance as self. All message and enum types
 * added within the block are committed to the pool atomically, and may refer
 * (co)recursively to each other. The user should call Builder#add_message and
 * Builder#add_enum within the block as appropriate.  This is the recommended,
 * idiomatic way to define new message and enum types.
 */
VALUE DescriptorPool_build(int argc, VALUE* argv, VALUE _self) {
  VALUE ctx = rb_class_new_instance(0, NULL, cBuilder);
  VALUE block = rb_block_proc();
  rb_funcall_with_block(ctx, rb_intern("instance_eval"), 0, NULL, block);
  rb_funcall(ctx, rb_intern("finalize_to_pool"), 1, _self);
  return Qnil;
}

/*
 * call-seq:
 *     DescriptorPool.lookup(name) => descriptor
 *
 * Finds a Descriptor or EnumDescriptor by name and returns it, or nil if none
 * exists with the given name.
 */
VALUE DescriptorPool_lookup(VALUE _self, VALUE name) {
  DEFINE_SELF(DescriptorPool, self, _self);
  const char* name_str = get_str(name);
  const upb_def* def = upb_symtab_lookup(self->symtab, name_str);
  if (!def) {
    return Qnil;
  }
  return get_def_obj(def);
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
VALUE DescriptorPool_generated_pool(VALUE _self) {
  return generated_pool;
}

// -----------------------------------------------------------------------------
// Descriptor.
// -----------------------------------------------------------------------------

DEFINE_CLASS(Descriptor, "Google::Protobuf::Descriptor");

void Descriptor_mark(void* _self) {
  Descriptor* self = _self;
  rb_gc_mark(self->klass);
}

void Descriptor_free(void* _self) {
  Descriptor* self = _self;
  upb_msgdef_unref(self->msgdef, &self->msgdef);
  if (self->layout) {
    free_layout(self->layout);
  }
  if (self->fill_handlers) {
    upb_handlers_unref(self->fill_handlers, &self->fill_handlers);
  }
  if (self->fill_method) {
    upb_pbdecodermethod_unref(self->fill_method, &self->fill_method);
  }
  if (self->json_fill_method) {
    upb_json_parsermethod_unref(self->json_fill_method,
                                &self->json_fill_method);
  }
  if (self->pb_serialize_handlers) {
    upb_handlers_unref(self->pb_serialize_handlers,
                       &self->pb_serialize_handlers);
  }
  if (self->json_serialize_handlers) {
    upb_handlers_unref(self->json_serialize_handlers,
                       &self->json_serialize_handlers);
  }
  if (self->json_serialize_handlers_preserve) {
    upb_handlers_unref(self->json_serialize_handlers_preserve,
                       &self->json_serialize_handlers_preserve);
  }
  xfree(self);
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
VALUE Descriptor_alloc(VALUE klass) {
  Descriptor* self = ALLOC(Descriptor);
  VALUE ret = TypedData_Wrap_Struct(klass, &_Descriptor_type, self);
  self->msgdef = upb_msgdef_new(&self->msgdef);
  self->klass = Qnil;
  self->layout = NULL;
  self->fill_handlers = NULL;
  self->fill_method = NULL;
  self->json_fill_method = NULL;
  self->pb_serialize_handlers = NULL;
  self->json_serialize_handlers = NULL;
  self->json_serialize_handlers_preserve = NULL;
  return ret;
}

void Descriptor_register(VALUE module) {
  VALUE klass = rb_define_class_under(
      module, "Descriptor", rb_cObject);
  rb_define_alloc_func(klass, Descriptor_alloc);
  rb_define_method(klass, "initialize", Descriptor_initialize, 1);
  rb_define_method(klass, "each", Descriptor_each, 0);
  rb_define_method(klass, "lookup", Descriptor_lookup, 1);
  rb_define_method(klass, "add_field", Descriptor_add_field, 1);
  rb_define_method(klass, "add_oneof", Descriptor_add_oneof, 1);
  rb_define_method(klass, "each_oneof", Descriptor_each_oneof, 0);
  rb_define_method(klass, "lookup_oneof", Descriptor_lookup_oneof, 1);
  rb_define_method(klass, "msgclass", Descriptor_msgclass, 0);
  rb_define_method(klass, "name", Descriptor_name, 0);
  rb_define_method(klass, "name=", Descriptor_name_set, 1);
  rb_define_method(klass, "file_descriptor", Descriptor_file_descriptor, 0);
  rb_include_module(klass, rb_mEnumerable);
  rb_gc_register_address(&cDescriptor);
  cDescriptor = klass;
}

/*
 * call-seq:
 *    Descriptor.new(file_descriptor)
 *
 * Initializes a new descriptor and assigns a file descriptor to it.
 */
VALUE Descriptor_initialize(VALUE _self, VALUE file_descriptor_rb) {
  DEFINE_SELF(Descriptor, self, _self);

  FileDescriptor* file_descriptor = ruby_to_FileDescriptor(file_descriptor_rb);

  CHECK_UPB(
        upb_filedef_addmsg(file_descriptor->filedef, self->msgdef, NULL, &status),
        "Failed to associate message to file descriptor.");
  add_def_obj(file_descriptor->filedef, file_descriptor_rb);

  return Qnil;
}

/*
 * call-seq:
 *    Descriptor.file_descriptor
 *
 * Returns the FileDescriptor object this message belongs to.
 */
VALUE Descriptor_file_descriptor(VALUE _self) {
  DEFINE_SELF(Descriptor, self, _self);
  return get_def_obj(upb_def_file(self->msgdef));
}

/*
 * call-seq:
 *     Descriptor.name => name
 *
 * Returns the name of this message type as a fully-qualfied string (e.g.,
 * My.Package.MessageType).
 */
VALUE Descriptor_name(VALUE _self) {
  DEFINE_SELF(Descriptor, self, _self);
  return rb_str_maybe_null(upb_msgdef_fullname(self->msgdef));
}

/*
 * call-seq:
 *    Descriptor.name = name
 *
 * Assigns a name to this message type. The descriptor must not have been added
 * to a pool yet.
 */
VALUE Descriptor_name_set(VALUE _self, VALUE str) {
  DEFINE_SELF(Descriptor, self, _self);
  upb_msgdef* mut_def = check_msg_notfrozen(self->msgdef);
  const char* name = get_str(str);
  CHECK_UPB(
      upb_msgdef_setfullname(mut_def, name, &status),
      "Error setting Descriptor name");
  return Qnil;
}

/*
 * call-seq:
 *     Descriptor.each(&block)
 *
 * Iterates over fields in this message type, yielding to the block on each one.
 */
VALUE Descriptor_each(VALUE _self) {
  DEFINE_SELF(Descriptor, self, _self);

  upb_msg_field_iter it;
  for (upb_msg_field_begin(&it, self->msgdef);
       !upb_msg_field_done(&it);
       upb_msg_field_next(&it)) {
    const upb_fielddef* field = upb_msg_iter_field(&it);
    VALUE obj = get_def_obj(field);
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
VALUE Descriptor_lookup(VALUE _self, VALUE name) {
  DEFINE_SELF(Descriptor, self, _self);
  const char* s = get_str(name);
  const upb_fielddef* field = upb_msgdef_ntofz(self->msgdef, s);
  if (field == NULL) {
    return Qnil;
  }
  return get_def_obj(field);
}

/*
 * call-seq:
 *     Descriptor.add_field(field) => nil
 *
 * Adds the given FieldDescriptor to this message type. This descriptor must not
 * have been added to a pool yet. Raises an exception if a field with the same
 * name or number already exists. Sub-type references (e.g. for fields of type
 * message) are not resolved at this point.
 */
VALUE Descriptor_add_field(VALUE _self, VALUE obj) {
  DEFINE_SELF(Descriptor, self, _self);
  upb_msgdef* mut_def = check_msg_notfrozen(self->msgdef);
  FieldDescriptor* def = ruby_to_FieldDescriptor(obj);
  upb_fielddef* mut_field_def = check_field_notfrozen(def->fielddef);
  CHECK_UPB(
      upb_msgdef_addfield(mut_def, mut_field_def, NULL, &status),
      "Adding field to Descriptor failed");
  add_def_obj(def->fielddef, obj);
  return Qnil;
}

/*
 * call-seq:
 *     Descriptor.add_oneof(oneof) => nil
 *
 * Adds the given OneofDescriptor to this message type. This descriptor must not
 * have been added to a pool yet. Raises an exception if a oneof with the same
 * name already exists, or if any of the oneof's fields' names or numbers
 * conflict with an existing field in this message type. All fields in the oneof
 * are added to the message descriptor. Sub-type references (e.g. for fields of
 * type message) are not resolved at this point.
 */
VALUE Descriptor_add_oneof(VALUE _self, VALUE obj) {
  DEFINE_SELF(Descriptor, self, _self);
  upb_msgdef* mut_def = check_msg_notfrozen(self->msgdef);
  OneofDescriptor* def = ruby_to_OneofDescriptor(obj);
  upb_oneofdef* mut_oneof_def = check_oneof_notfrozen(def->oneofdef);
  CHECK_UPB(
      upb_msgdef_addoneof(mut_def, mut_oneof_def, NULL, &status),
      "Adding oneof to Descriptor failed");
  add_def_obj(def->oneofdef, obj);
  return Qnil;
}

/*
 * call-seq:
 *     Descriptor.each_oneof(&block) => nil
 *
 * Invokes the given block for each oneof in this message type, passing the
 * corresponding OneofDescriptor.
 */
VALUE Descriptor_each_oneof(VALUE _self) {
  DEFINE_SELF(Descriptor, self, _self);

  upb_msg_oneof_iter it;
  for (upb_msg_oneof_begin(&it, self->msgdef);
       !upb_msg_oneof_done(&it);
       upb_msg_oneof_next(&it)) {
    const upb_oneofdef* oneof = upb_msg_iter_oneof(&it);
    VALUE obj = get_def_obj(oneof);
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
VALUE Descriptor_lookup_oneof(VALUE _self, VALUE name) {
  DEFINE_SELF(Descriptor, self, _self);
  const char* s = get_str(name);
  const upb_oneofdef* oneof = upb_msgdef_ntooz(self->msgdef, s);
  if (oneof == NULL) {
    return Qnil;
  }
  return get_def_obj(oneof);
}

/*
 * call-seq:
 *     Descriptor.msgclass => message_klass
 *
 * Returns the Ruby class created for this message type. Valid only once the
 * message type has been added to a pool.
 */
VALUE Descriptor_msgclass(VALUE _self) {
  DEFINE_SELF(Descriptor, self, _self);
  if (!upb_def_isfrozen((const upb_def*)self->msgdef)) {
    rb_raise(rb_eRuntimeError,
             "Cannot fetch message class from a Descriptor not yet in a pool.");
  }
  if (self->klass == Qnil) {
    self->klass = build_class_from_descriptor(self);
  }
  return self->klass;
}

// -----------------------------------------------------------------------------
// FileDescriptor.
// -----------------------------------------------------------------------------

DEFINE_CLASS(FileDescriptor, "Google::Protobuf::FileDescriptor");

void FileDescriptor_mark(void* _self) {
}

void FileDescriptor_free(void* _self) {
  FileDescriptor* self = _self;
  upb_filedef_unref(self->filedef, &self->filedef);
  xfree(self);
}

/*
 * call-seq:
 *     FileDescriptor.new => file
 *
 * Returns a new file descriptor. The syntax must be set before it's passed
 * to a builder.
 */
VALUE FileDescriptor_alloc(VALUE klass) {
  FileDescriptor* self = ALLOC(FileDescriptor);
  VALUE ret = TypedData_Wrap_Struct(klass, &_FileDescriptor_type, self);
  upb_filedef* filedef = upb_filedef_new(&self->filedef);
  self->filedef = filedef;
  return ret;
}

void FileDescriptor_register(VALUE module) {
  VALUE klass = rb_define_class_under(
      module, "FileDescriptor", rb_cObject);
  rb_define_alloc_func(klass, FileDescriptor_alloc);
  rb_define_method(klass, "initialize", FileDescriptor_initialize, -1);
  rb_define_method(klass, "name", FileDescriptor_name, 0);
  rb_define_method(klass, "syntax", FileDescriptor_syntax, 0);
  rb_define_method(klass, "syntax=", FileDescriptor_syntax_set, 1);
  rb_gc_register_address(&cFileDescriptor);
  cFileDescriptor = klass;
}

/*
 * call-seq:
 *     FileDescriptor.new(name, options = nil) => file
 *
 * Initializes a new file descriptor with the given file name.
 * Also accepts an optional "options" hash, specifying other optional
 * metadata about the file. The options hash currently accepts the following
 *   * "syntax": :proto2 or :proto3 (default: :proto3)
 */
VALUE FileDescriptor_initialize(int argc, VALUE* argv, VALUE _self) {
  DEFINE_SELF(FileDescriptor, self, _self);

  VALUE name_rb;
  VALUE options = Qnil;
  rb_scan_args(argc, argv, "11", &name_rb, &options);

  if (name_rb != Qnil) {
    Check_Type(name_rb, T_STRING);
    const char* name = get_str(name_rb);
    CHECK_UPB(upb_filedef_setname(self->filedef, name, &status),
	      "Error setting file name");
  }

  // Default syntax is proto3.
  VALUE syntax = ID2SYM(rb_intern("proto3"));
  if (options != Qnil) {
    Check_Type(options, T_HASH);

    if (rb_funcall(options, rb_intern("key?"), 1,
		   ID2SYM(rb_intern("syntax"))) == Qtrue) {
      syntax = rb_hash_lookup(options, ID2SYM(rb_intern("syntax")));
    }
  }
  FileDescriptor_syntax_set(_self, syntax);

  return Qnil;
}

/*
 * call-seq:
 *     FileDescriptor.name => name
 *
 * Returns the name of the file.
 */
VALUE FileDescriptor_name(VALUE _self) {
  DEFINE_SELF(FileDescriptor, self, _self);
  const char* name = upb_filedef_name(self->filedef);
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
VALUE FileDescriptor_syntax(VALUE _self) {
  DEFINE_SELF(FileDescriptor, self, _self);

  switch (upb_filedef_syntax(self->filedef)) {
    case UPB_SYNTAX_PROTO3: return ID2SYM(rb_intern("proto3"));
    case UPB_SYNTAX_PROTO2: return ID2SYM(rb_intern("proto2"));
    default: return Qnil;
  }
}

/*
 * call-seq:
 *     FileDescriptor.syntax = version
 *
 * Sets this file descriptor's syntax, can be :proto3 or :proto2.
 */
VALUE FileDescriptor_syntax_set(VALUE _self, VALUE syntax_rb) {
  DEFINE_SELF(FileDescriptor, self, _self);
  Check_Type(syntax_rb, T_SYMBOL);

  upb_syntax_t syntax;
  if (SYM2ID(syntax_rb) == rb_intern("proto3")) {
    syntax = UPB_SYNTAX_PROTO3;
  } else if (SYM2ID(syntax_rb) == rb_intern("proto2")) {
    syntax = UPB_SYNTAX_PROTO2;
  } else {
    rb_raise(rb_eArgError, "Expected :proto3 or :proto3, received '%s'",
	     rb_id2name(SYM2ID(syntax_rb)));
  }

  CHECK_UPB(upb_filedef_setsyntax(self->filedef, syntax, &status),
          "Error setting file syntax for proto");
  return Qnil;
}

// -----------------------------------------------------------------------------
// FieldDescriptor.
// -----------------------------------------------------------------------------

DEFINE_CLASS(FieldDescriptor, "Google::Protobuf::FieldDescriptor");

void FieldDescriptor_mark(void* _self) {
}

void FieldDescriptor_free(void* _self) {
  FieldDescriptor* self = _self;
  upb_fielddef_unref(self->fielddef, &self->fielddef);
  xfree(self);
}

/*
 * call-seq:
 *     FieldDescriptor.new => field
 *
 * Returns a new field descriptor. Its name, type, etc. must be set before it is
 * added to a message type.
 */
VALUE FieldDescriptor_alloc(VALUE klass) {
  FieldDescriptor* self = ALLOC(FieldDescriptor);
  VALUE ret = TypedData_Wrap_Struct(klass, &_FieldDescriptor_type, self);
  upb_fielddef* fielddef = upb_fielddef_new(&self->fielddef);
  upb_fielddef_setpacked(fielddef, false);
  self->fielddef = fielddef;
  return ret;
}

void FieldDescriptor_register(VALUE module) {
  VALUE klass = rb_define_class_under(
      module, "FieldDescriptor", rb_cObject);
  rb_define_alloc_func(klass, FieldDescriptor_alloc);
  rb_define_method(klass, "name", FieldDescriptor_name, 0);
  rb_define_method(klass, "name=", FieldDescriptor_name_set, 1);
  rb_define_method(klass, "type", FieldDescriptor_type, 0);
  rb_define_method(klass, "type=", FieldDescriptor_type_set, 1);
  rb_define_method(klass, "default", FieldDescriptor_default, 0);
  rb_define_method(klass, "default=", FieldDescriptor_default_set, 1);
  rb_define_method(klass, "label", FieldDescriptor_label, 0);
  rb_define_method(klass, "label=", FieldDescriptor_label_set, 1);
  rb_define_method(klass, "number", FieldDescriptor_number, 0);
  rb_define_method(klass, "number=", FieldDescriptor_number_set, 1);
  rb_define_method(klass, "submsg_name", FieldDescriptor_submsg_name, 0);
  rb_define_method(klass, "submsg_name=", FieldDescriptor_submsg_name_set, 1);
  rb_define_method(klass, "subtype", FieldDescriptor_subtype, 0);
  rb_define_method(klass, "has?", FieldDescriptor_has, 1);
  rb_define_method(klass, "clear", FieldDescriptor_clear, 1);
  rb_define_method(klass, "get", FieldDescriptor_get, 1);
  rb_define_method(klass, "set", FieldDescriptor_set, 2);
  rb_gc_register_address(&cFieldDescriptor);
  cFieldDescriptor = klass;
}

/*
 * call-seq:
 *     FieldDescriptor.name => name
 *
 * Returns the name of this field.
 */
VALUE FieldDescriptor_name(VALUE _self) {
  DEFINE_SELF(FieldDescriptor, self, _self);
  return rb_str_maybe_null(upb_fielddef_name(self->fielddef));
}

/*
 * call-seq:
 *     FieldDescriptor.name = name
 *
 * Sets the name of this field. Cannot be called once the containing message
 * type, if any, is added to a pool.
 */
VALUE FieldDescriptor_name_set(VALUE _self, VALUE str) {
  DEFINE_SELF(FieldDescriptor, self, _self);
  upb_fielddef* mut_def = check_field_notfrozen(self->fielddef);
  const char* name = get_str(str);
  CHECK_UPB(upb_fielddef_setname(mut_def, name, &status),
            "Error setting FieldDescriptor name");
  return Qnil;
}

upb_fieldtype_t ruby_to_fieldtype(VALUE type) {
  if (TYPE(type) != T_SYMBOL) {
    rb_raise(rb_eArgError, "Expected symbol for field type.");
  }

#define CONVERT(upb, ruby)                                           \
  if (SYM2ID(type) == rb_intern( # ruby )) {                         \
    return UPB_TYPE_ ## upb;                                         \
  }

  CONVERT(FLOAT, float);
  CONVERT(DOUBLE, double);
  CONVERT(BOOL, bool);
  CONVERT(STRING, string);
  CONVERT(BYTES, bytes);
  CONVERT(MESSAGE, message);
  CONVERT(ENUM, enum);
  CONVERT(INT32, int32);
  CONVERT(INT64, int64);
  CONVERT(UINT32, uint32);
  CONVERT(UINT64, uint64);

#undef CONVERT

  rb_raise(rb_eArgError, "Unknown field type.");
  return 0;
}

VALUE fieldtype_to_ruby(upb_fieldtype_t type) {
  switch (type) {
#define CONVERT(upb, ruby)                                           \
    case UPB_TYPE_ ## upb : return ID2SYM(rb_intern( # ruby ));
    CONVERT(FLOAT, float);
    CONVERT(DOUBLE, double);
    CONVERT(BOOL, bool);
    CONVERT(STRING, string);
    CONVERT(BYTES, bytes);
    CONVERT(MESSAGE, message);
    CONVERT(ENUM, enum);
    CONVERT(INT32, int32);
    CONVERT(INT64, int64);
    CONVERT(UINT32, uint32);
    CONVERT(UINT64, uint64);
#undef CONVERT
  }
  return Qnil;
}

upb_descriptortype_t ruby_to_descriptortype(VALUE type) {
  if (TYPE(type) != T_SYMBOL) {
    rb_raise(rb_eArgError, "Expected symbol for field type.");
  }

#define CONVERT(upb, ruby)                                           \
  if (SYM2ID(type) == rb_intern( # ruby )) {                         \
    return UPB_DESCRIPTOR_TYPE_ ## upb;                              \
  }

  CONVERT(FLOAT, float);
  CONVERT(DOUBLE, double);
  CONVERT(BOOL, bool);
  CONVERT(STRING, string);
  CONVERT(BYTES, bytes);
  CONVERT(MESSAGE, message);
  CONVERT(GROUP, group);
  CONVERT(ENUM, enum);
  CONVERT(INT32, int32);
  CONVERT(INT64, int64);
  CONVERT(UINT32, uint32);
  CONVERT(UINT64, uint64);
  CONVERT(SINT32, sint32);
  CONVERT(SINT64, sint64);
  CONVERT(FIXED32, fixed32);
  CONVERT(FIXED64, fixed64);
  CONVERT(SFIXED32, sfixed32);
  CONVERT(SFIXED64, sfixed64);

#undef CONVERT

  rb_raise(rb_eArgError, "Unknown field type.");
  return 0;
}

VALUE descriptortype_to_ruby(upb_descriptortype_t type) {
  switch (type) {
#define CONVERT(upb, ruby)                                           \
    case UPB_DESCRIPTOR_TYPE_ ## upb : return ID2SYM(rb_intern( # ruby ));
    CONVERT(FLOAT, float);
    CONVERT(DOUBLE, double);
    CONVERT(BOOL, bool);
    CONVERT(STRING, string);
    CONVERT(BYTES, bytes);
    CONVERT(MESSAGE, message);
    CONVERT(GROUP, group);
    CONVERT(ENUM, enum);
    CONVERT(INT32, int32);
    CONVERT(INT64, int64);
    CONVERT(UINT32, uint32);
    CONVERT(UINT64, uint64);
    CONVERT(SINT32, sint32);
    CONVERT(SINT64, sint64);
    CONVERT(FIXED32, fixed32);
    CONVERT(FIXED64, fixed64);
    CONVERT(SFIXED32, sfixed32);
    CONVERT(SFIXED64, sfixed64);
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
VALUE FieldDescriptor_type(VALUE _self) {
  DEFINE_SELF(FieldDescriptor, self, _self);
  if (!upb_fielddef_typeisset(self->fielddef)) {
    return Qnil;
  }
  return descriptortype_to_ruby(upb_fielddef_descriptortype(self->fielddef));
}

/*
 * call-seq:
 *     FieldDescriptor.type = type
 *
 * Sets this field's type. Cannot be called if field is part of a message type
 * already in a pool.
 */
VALUE FieldDescriptor_type_set(VALUE _self, VALUE type) {
  DEFINE_SELF(FieldDescriptor, self, _self);
  upb_fielddef* mut_def = check_field_notfrozen(self->fielddef);
  upb_fielddef_setdescriptortype(mut_def, ruby_to_descriptortype(type));
  return Qnil;
}

/*
 * call-seq:
 *     FieldDescriptor.default => default
 *
 * Returns this field's default, as a Ruby object, or nil if not yet set.
 */
VALUE FieldDescriptor_default(VALUE _self) {
  DEFINE_SELF(FieldDescriptor, self, _self);
  return layout_get_default(self->fielddef);
}

/*
 * call-seq:
 *     FieldDescriptor.default = default
 *
 * Sets this field's default value. Raises an exception when calling with
 * proto syntax 3.
 */
VALUE FieldDescriptor_default_set(VALUE _self, VALUE default_value) {
  DEFINE_SELF(FieldDescriptor, self, _self);
  upb_fielddef* mut_def = check_field_notfrozen(self->fielddef);

  switch (upb_fielddef_type(mut_def)) {
    case UPB_TYPE_FLOAT:
      upb_fielddef_setdefaultfloat(mut_def, NUM2DBL(default_value));
      break;
    case UPB_TYPE_DOUBLE:
      upb_fielddef_setdefaultdouble(mut_def, NUM2DBL(default_value));
      break;
    case UPB_TYPE_BOOL:
      if (!RB_TYPE_P(default_value, T_TRUE) &&
	  !RB_TYPE_P(default_value, T_FALSE) &&
	  !RB_TYPE_P(default_value, T_NIL)) {
        rb_raise(cTypeError, "Expected boolean for default value.");
      }

      upb_fielddef_setdefaultbool(mut_def, RTEST(default_value));
      break;
    case UPB_TYPE_ENUM:
    case UPB_TYPE_INT32:
      upb_fielddef_setdefaultint32(mut_def, NUM2INT(default_value));
      break;
    case UPB_TYPE_INT64:
      upb_fielddef_setdefaultint64(mut_def, NUM2INT(default_value));
      break;
    case UPB_TYPE_UINT32:
      upb_fielddef_setdefaultuint32(mut_def, NUM2UINT(default_value));
      break;
    case UPB_TYPE_UINT64:
      upb_fielddef_setdefaultuint64(mut_def, NUM2UINT(default_value));
      break;
    case UPB_TYPE_STRING:
    case UPB_TYPE_BYTES:
      CHECK_UPB(upb_fielddef_setdefaultcstr(mut_def, StringValuePtr(default_value),
					    &status),
                "Error setting default string");
      break;
    default:
      rb_raise(rb_eArgError, "Defaults not supported on field %s.%s",
	       upb_fielddef_fullname(mut_def), upb_fielddef_name(mut_def));
  }

  return Qnil;
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
VALUE FieldDescriptor_label(VALUE _self) {
  DEFINE_SELF(FieldDescriptor, self, _self);
  switch (upb_fielddef_label(self->fielddef)) {
#define CONVERT(upb, ruby)                                           \
    case UPB_LABEL_ ## upb : return ID2SYM(rb_intern( # ruby ));

    CONVERT(OPTIONAL, optional);
    CONVERT(REQUIRED, required);
    CONVERT(REPEATED, repeated);

#undef CONVERT
  }

  return Qnil;
}

/*
 * call-seq:
 *     FieldDescriptor.label = label
 *
 * Sets the label on this field. Cannot be called if field is part of a message
 * type already in a pool.
 */
VALUE FieldDescriptor_label_set(VALUE _self, VALUE label) {
  DEFINE_SELF(FieldDescriptor, self, _self);
  upb_fielddef* mut_def = check_field_notfrozen(self->fielddef);
  upb_label_t upb_label = -1;
  bool converted = false;

  if (TYPE(label) != T_SYMBOL) {
    rb_raise(rb_eArgError, "Expected symbol for field label.");
  }

#define CONVERT(upb, ruby)                                           \
  if (SYM2ID(label) == rb_intern( # ruby )) {                        \
    upb_label = UPB_LABEL_ ## upb;                                   \
    converted = true;                                                \
  }

  CONVERT(OPTIONAL, optional);
  CONVERT(REQUIRED, required);
  CONVERT(REPEATED, repeated);

#undef CONVERT

  if (!converted) {
    rb_raise(rb_eArgError, "Unknown field label.");
  }

  upb_fielddef_setlabel(mut_def, upb_label);

  return Qnil;
}

/*
 * call-seq:
 *     FieldDescriptor.number => number
 *
 * Returns the tag number for this field.
 */
VALUE FieldDescriptor_number(VALUE _self) {
  DEFINE_SELF(FieldDescriptor, self, _self);
  return INT2NUM(upb_fielddef_number(self->fielddef));
}

/*
 * call-seq:
 *     FieldDescriptor.number = number
 *
 * Sets the tag number for this field. Cannot be called if field is part of a
 * message type already in a pool.
 */
VALUE FieldDescriptor_number_set(VALUE _self, VALUE number) {
  DEFINE_SELF(FieldDescriptor, self, _self);
  upb_fielddef* mut_def = check_field_notfrozen(self->fielddef);
  CHECK_UPB(upb_fielddef_setnumber(mut_def, NUM2INT(number), &status),
            "Error setting field number");
  return Qnil;
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
VALUE FieldDescriptor_submsg_name(VALUE _self) {
  DEFINE_SELF(FieldDescriptor, self, _self);
  if (!upb_fielddef_hassubdef(self->fielddef)) {
    return Qnil;
  }
  return rb_str_maybe_null(upb_fielddef_subdefname(self->fielddef));
}

/*
 * call-seq:
 *     FieldDescriptor.submsg_name = submsg_name
 *
 * Sets the name of the message or enum type corresponding to this field, if it
 * is a message or enum field (respectively). This type name will be resolved
 * within the context of the pool to which the containing message type is added.
 * Cannot be called on field that are not of message or enum type, or on fields
 * that are part of a message type already added to a pool.
 */
VALUE FieldDescriptor_submsg_name_set(VALUE _self, VALUE value) {
  DEFINE_SELF(FieldDescriptor, self, _self);
  upb_fielddef* mut_def = check_field_notfrozen(self->fielddef);
  const char* str = get_str(value);
  if (!upb_fielddef_hassubdef(self->fielddef)) {
    rb_raise(cTypeError, "FieldDescriptor does not have subdef.");
  }
  CHECK_UPB(upb_fielddef_setsubdefname(mut_def, str, &status),
            "Error setting submessage name");
  return Qnil;
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
VALUE FieldDescriptor_subtype(VALUE _self) {
  DEFINE_SELF(FieldDescriptor, self, _self);
  const upb_def* def;

  if (!upb_fielddef_hassubdef(self->fielddef)) {
    return Qnil;
  }
  def = upb_fielddef_subdef(self->fielddef);
  if (def == NULL) {
    return Qnil;
  }
  return get_def_obj(def);
}

/*
 * call-seq:
 *     FieldDescriptor.get(message) => value
 *
 * Returns the value set for this field on the given message. Raises an
 * exception if message is of the wrong type.
 */
VALUE FieldDescriptor_get(VALUE _self, VALUE msg_rb) {
  DEFINE_SELF(FieldDescriptor, self, _self);
  MessageHeader* msg;
  TypedData_Get_Struct(msg_rb, MessageHeader, &Message_type, msg);
  if (msg->descriptor->msgdef != upb_fielddef_containingtype(self->fielddef)) {
    rb_raise(cTypeError, "get method called on wrong message type");
  }
  return layout_get(msg->descriptor->layout, Message_data(msg), self->fielddef);
}

/*
 * call-seq:
 *     FieldDescriptor.has?(message) => boolean
 *
 * Returns whether the value is set on the given message. Raises an
 * exception when calling with proto syntax 3.
 */
VALUE FieldDescriptor_has(VALUE _self, VALUE msg_rb) {
  DEFINE_SELF(FieldDescriptor, self, _self);
  MessageHeader* msg;
  TypedData_Get_Struct(msg_rb, MessageHeader, &Message_type, msg);
  if (msg->descriptor->msgdef != upb_fielddef_containingtype(self->fielddef)) {
    rb_raise(cTypeError, "has method called on wrong message type");
  } else if (!upb_fielddef_haspresence(self->fielddef)) {
    rb_raise(rb_eArgError, "does not track presence");
  }

  return layout_has(msg->descriptor->layout, Message_data(msg), self->fielddef);
}

/*
 * call-seq:
 *     FieldDescriptor.clear(message)
 *
 * Clears the field from the message if it's set.
 */
VALUE FieldDescriptor_clear(VALUE _self, VALUE msg_rb) {
  DEFINE_SELF(FieldDescriptor, self, _self);
  MessageHeader* msg;
  TypedData_Get_Struct(msg_rb, MessageHeader, &Message_type, msg);
  if (msg->descriptor->msgdef != upb_fielddef_containingtype(self->fielddef)) {
    rb_raise(cTypeError, "has method called on wrong message type");
  }

  layout_clear(msg->descriptor->layout, Message_data(msg), self->fielddef);
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
VALUE FieldDescriptor_set(VALUE _self, VALUE msg_rb, VALUE value) {
  DEFINE_SELF(FieldDescriptor, self, _self);
  MessageHeader* msg;
  TypedData_Get_Struct(msg_rb, MessageHeader, &Message_type, msg);
  if (msg->descriptor->msgdef != upb_fielddef_containingtype(self->fielddef)) {
    rb_raise(cTypeError, "set method called on wrong message type");
  }
  layout_set(msg->descriptor->layout, Message_data(msg), self->fielddef, value);
  return Qnil;
}

// -----------------------------------------------------------------------------
// OneofDescriptor.
// -----------------------------------------------------------------------------

DEFINE_CLASS(OneofDescriptor, "Google::Protobuf::OneofDescriptor");

void OneofDescriptor_mark(void* _self) {
}

void OneofDescriptor_free(void* _self) {
  OneofDescriptor* self = _self;
  upb_oneofdef_unref(self->oneofdef, &self->oneofdef);
  xfree(self);
}

/*
 * call-seq:
 *     OneofDescriptor.new => oneof_descriptor
 *
 * Creates a new, empty, oneof descriptor. The oneof may only be modified prior
 * to being added to a message descriptor which is subsequently added to a pool.
 */
VALUE OneofDescriptor_alloc(VALUE klass) {
  OneofDescriptor* self = ALLOC(OneofDescriptor);
  VALUE ret = TypedData_Wrap_Struct(klass, &_OneofDescriptor_type, self);
  self->oneofdef = upb_oneofdef_new(&self->oneofdef);
  return ret;
}

void OneofDescriptor_register(VALUE module) {
  VALUE klass = rb_define_class_under(
      module, "OneofDescriptor", rb_cObject);
  rb_define_alloc_func(klass, OneofDescriptor_alloc);
  rb_define_method(klass, "name", OneofDescriptor_name, 0);
  rb_define_method(klass, "name=", OneofDescriptor_name_set, 1);
  rb_define_method(klass, "add_field", OneofDescriptor_add_field, 1);
  rb_define_method(klass, "each", OneofDescriptor_each, 0);
  rb_include_module(klass, rb_mEnumerable);
  rb_gc_register_address(&cOneofDescriptor);
  cOneofDescriptor = klass;
}

/*
 * call-seq:
 *     OneofDescriptor.name => name
 *
 * Returns the name of this oneof.
 */
VALUE OneofDescriptor_name(VALUE _self) {
  DEFINE_SELF(OneofDescriptor, self, _self);
  return rb_str_maybe_null(upb_oneofdef_name(self->oneofdef));
}

/*
 * call-seq:
 *     OneofDescriptor.name = name
 *
 * Sets a new name for this oneof. The oneof must not have been added to a
 * message descriptor yet.
 */
VALUE OneofDescriptor_name_set(VALUE _self, VALUE value) {
  DEFINE_SELF(OneofDescriptor, self, _self);
  upb_oneofdef* mut_def = check_oneof_notfrozen(self->oneofdef);
  const char* str = get_str(value);
  CHECK_UPB(upb_oneofdef_setname(mut_def, str, &status),
            "Error setting oneof name");
  return Qnil;
}

/*
 * call-seq:
 *     OneofDescriptor.add_field(field) => nil
 *
 * Adds a field to this oneof. The field may have been added to this oneof in
 * the past, or the message to which this oneof belongs (if any), but may not
 * have already been added to any other oneof or message. Otherwise, an
 * exception is raised.
 *
 * All fields added to the oneof via this method will be automatically added to
 * the message to which this oneof belongs, if it belongs to one currently, or
 * else will be added to any message to which the oneof is later added at the
 * time that it is added.
 */
VALUE OneofDescriptor_add_field(VALUE _self, VALUE obj) {
  DEFINE_SELF(OneofDescriptor, self, _self);
  upb_oneofdef* mut_def = check_oneof_notfrozen(self->oneofdef);
  FieldDescriptor* def = ruby_to_FieldDescriptor(obj);
  upb_fielddef* mut_field_def = check_field_notfrozen(def->fielddef);
  CHECK_UPB(
      upb_oneofdef_addfield(mut_def, mut_field_def, NULL, &status),
      "Adding field to OneofDescriptor failed");
  add_def_obj(def->fielddef, obj);
  return Qnil;
}

/*
 * call-seq:
 *     OneofDescriptor.each(&block) => nil
 *
 * Iterates through fields in this oneof, yielding to the block on each one.
 */
VALUE OneofDescriptor_each(VALUE _self, VALUE field) {
  DEFINE_SELF(OneofDescriptor, self, _self);
  upb_oneof_iter it;
  for (upb_oneof_begin(&it, self->oneofdef);
       !upb_oneof_done(&it);
       upb_oneof_next(&it)) {
    const upb_fielddef* f = upb_oneof_iter_field(&it);
    VALUE obj = get_def_obj(f);
    rb_yield(obj);
  }
  return Qnil;
}

// -----------------------------------------------------------------------------
// EnumDescriptor.
// -----------------------------------------------------------------------------

DEFINE_CLASS(EnumDescriptor, "Google::Protobuf::EnumDescriptor");

void EnumDescriptor_mark(void* _self) {
  EnumDescriptor* self = _self;
  rb_gc_mark(self->module);
}

void EnumDescriptor_free(void* _self) {
  EnumDescriptor* self = _self;
  upb_enumdef_unref(self->enumdef, &self->enumdef);
  xfree(self);
}

/*
 * call-seq:
 *     EnumDescriptor.new => enum_descriptor
 *
 * Creates a new, empty, enum descriptor. Must be added to a pool before the
 * enum type can be used. The enum type may only be modified prior to adding to
 * a pool.
 */
VALUE EnumDescriptor_alloc(VALUE klass) {
  EnumDescriptor* self = ALLOC(EnumDescriptor);
  VALUE ret = TypedData_Wrap_Struct(klass, &_EnumDescriptor_type, self);
  self->enumdef = upb_enumdef_new(&self->enumdef);
  self->module = Qnil;
  return ret;
}

void EnumDescriptor_register(VALUE module) {
  VALUE klass = rb_define_class_under(
      module, "EnumDescriptor", rb_cObject);
  rb_define_alloc_func(klass, EnumDescriptor_alloc);
  rb_define_method(klass, "initialize", EnumDescriptor_initialize, 1);
  rb_define_method(klass, "name", EnumDescriptor_name, 0);
  rb_define_method(klass, "name=", EnumDescriptor_name_set, 1);
  rb_define_method(klass, "add_value", EnumDescriptor_add_value, 2);
  rb_define_method(klass, "lookup_name", EnumDescriptor_lookup_name, 1);
  rb_define_method(klass, "lookup_value", EnumDescriptor_lookup_value, 1);
  rb_define_method(klass, "each", EnumDescriptor_each, 0);
  rb_define_method(klass, "enummodule", EnumDescriptor_enummodule, 0);
  rb_define_method(klass, "file_descriptor", EnumDescriptor_file_descriptor, 0);
  rb_include_module(klass, rb_mEnumerable);
  rb_gc_register_address(&cEnumDescriptor);
  cEnumDescriptor = klass;
}

/*
 * call-seq:
 *    Descriptor.new(file_descriptor)
 *
 * Initializes a new descriptor and assigns a file descriptor to it.
 */
VALUE EnumDescriptor_initialize(VALUE _self, VALUE file_descriptor_rb) {
  DEFINE_SELF(EnumDescriptor, self, _self);
  FileDescriptor* file_descriptor = ruby_to_FileDescriptor(file_descriptor_rb);
  CHECK_UPB(
        upb_filedef_addenum(file_descriptor->filedef, self->enumdef,
			    NULL, &status),
        "Failed to associate enum to file descriptor.");
  add_def_obj(file_descriptor->filedef, file_descriptor_rb);

  return Qnil;
}

/*
 * call-seq:
 *    Descriptor.file_descriptor
 *
 * Returns the FileDescriptor object this enum belongs to.
 */
VALUE EnumDescriptor_file_descriptor(VALUE _self) {
  DEFINE_SELF(EnumDescriptor, self, _self);
  return get_def_obj(upb_def_file(self->enumdef));
}

/*
 * call-seq:
 *     EnumDescriptor.name => name
 *
 * Returns the name of this enum type.
 */
VALUE EnumDescriptor_name(VALUE _self) {
  DEFINE_SELF(EnumDescriptor, self, _self);
  return rb_str_maybe_null(upb_enumdef_fullname(self->enumdef));
}

/*
 * call-seq:
 *     EnumDescriptor.name = name
 *
 * Sets the name of this enum type. Cannot be called if the enum type has
 * already been added to a pool.
 */
VALUE EnumDescriptor_name_set(VALUE _self, VALUE str) {
  DEFINE_SELF(EnumDescriptor, self, _self);
  upb_enumdef* mut_def = check_enum_notfrozen(self->enumdef);
  const char* name = get_str(str);
  CHECK_UPB(upb_enumdef_setfullname(mut_def, name, &status),
            "Error setting EnumDescriptor name");
  return Qnil;
}

/*
 * call-seq:
 *     EnumDescriptor.add_value(key, value)
 *
 * Adds a new key => value mapping to this enum type. Key must be given as a
 * Ruby symbol. Cannot be called if the enum type has already been added to a
 * pool. Will raise an exception if the key or value is already in use.
 */
VALUE EnumDescriptor_add_value(VALUE _self, VALUE name, VALUE number) {
  DEFINE_SELF(EnumDescriptor, self, _self);
  upb_enumdef* mut_def = check_enum_notfrozen(self->enumdef);
  const char* name_str = rb_id2name(SYM2ID(name));
  int32_t val = NUM2INT(number);
  CHECK_UPB(upb_enumdef_addval(mut_def, name_str, val, &status),
            "Error adding value to enum");
  return Qnil;
}

/*
 * call-seq:
 *     EnumDescriptor.lookup_name(name) => value
 *
 * Returns the numeric value corresponding to the given key name (as a Ruby
 * symbol), or nil if none.
 */
VALUE EnumDescriptor_lookup_name(VALUE _self, VALUE name) {
  DEFINE_SELF(EnumDescriptor, self, _self);
  const char* name_str= rb_id2name(SYM2ID(name));
  int32_t val = 0;
  if (upb_enumdef_ntoiz(self->enumdef, name_str, &val)) {
    return INT2NUM(val);
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
VALUE EnumDescriptor_lookup_value(VALUE _self, VALUE number) {
  DEFINE_SELF(EnumDescriptor, self, _self);
  int32_t val = NUM2INT(number);
  const char* name = upb_enumdef_iton(self->enumdef, val);
  if (name != NULL) {
    return ID2SYM(rb_intern(name));
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
VALUE EnumDescriptor_each(VALUE _self) {
  DEFINE_SELF(EnumDescriptor, self, _self);

  upb_enum_iter it;
  for (upb_enum_begin(&it, self->enumdef);
       !upb_enum_done(&it);
       upb_enum_next(&it)) {
    VALUE key = ID2SYM(rb_intern(upb_enum_iter_name(&it)));
    VALUE number = INT2NUM(upb_enum_iter_number(&it));
    rb_yield_values(2, key, number);
  }

  return Qnil;
}

/*
 * call-seq:
 *     EnumDescriptor.enummodule => module
 *
 * Returns the Ruby module corresponding to this enum type. Cannot be called
 * until the enum descriptor has been added to a pool.
 */
VALUE EnumDescriptor_enummodule(VALUE _self) {
  DEFINE_SELF(EnumDescriptor, self, _self);
  if (!upb_def_isfrozen((const upb_def*)self->enumdef)) {
    rb_raise(rb_eRuntimeError,
             "Cannot fetch enum module from an EnumDescriptor not yet "
             "in a pool.");
  }
  if (self->module == Qnil) {
    self->module = build_module_from_enumdesc(self);
  }
  return self->module;
}

// -----------------------------------------------------------------------------
// MessageBuilderContext.
// -----------------------------------------------------------------------------

DEFINE_CLASS(MessageBuilderContext,
    "Google::Protobuf::Internal::MessageBuilderContext");

void MessageBuilderContext_mark(void* _self) {
  MessageBuilderContext* self = _self;
  rb_gc_mark(self->descriptor);
  rb_gc_mark(self->builder);
}

void MessageBuilderContext_free(void* _self) {
  MessageBuilderContext* self = _self;
  xfree(self);
}

VALUE MessageBuilderContext_alloc(VALUE klass) {
  MessageBuilderContext* self = ALLOC(MessageBuilderContext);
  VALUE ret = TypedData_Wrap_Struct(
      klass, &_MessageBuilderContext_type, self);
  self->descriptor = Qnil;
  self->builder = Qnil;
  return ret;
}

void MessageBuilderContext_register(VALUE module) {
  VALUE klass = rb_define_class_under(
      module, "MessageBuilderContext", rb_cObject);
  rb_define_alloc_func(klass, MessageBuilderContext_alloc);
  rb_define_method(klass, "initialize",
                   MessageBuilderContext_initialize, 2);
  rb_define_method(klass, "optional", MessageBuilderContext_optional, -1);
  rb_define_method(klass, "required", MessageBuilderContext_required, -1);
  rb_define_method(klass, "repeated", MessageBuilderContext_repeated, -1);
  rb_define_method(klass, "map", MessageBuilderContext_map, -1);
  rb_define_method(klass, "oneof", MessageBuilderContext_oneof, 1);
  rb_gc_register_address(&cMessageBuilderContext);
  cMessageBuilderContext = klass;
}

/*
 * call-seq:
 *     MessageBuilderContext.new(desc, builder) => context
 *
 * Create a new message builder context around the given message descriptor and
 * builder context. This class is intended to serve as a DSL context to be used
 * with #instance_eval.
 */
VALUE MessageBuilderContext_initialize(VALUE _self,
                                       VALUE msgdef,
                                       VALUE builder) {
  DEFINE_SELF(MessageBuilderContext, self, _self);
  self->descriptor = msgdef;
  self->builder = builder;
  return Qnil;
}

static VALUE msgdef_add_field(VALUE msgdef_rb,
                              const char* label, VALUE name,
                              VALUE type, VALUE number,
                              VALUE type_class,
                              VALUE options) {
  VALUE fielddef_rb = rb_class_new_instance(0, NULL, cFieldDescriptor);
  VALUE name_str = rb_str_new2(rb_id2name(SYM2ID(name)));

  rb_funcall(fielddef_rb, rb_intern("label="), 1, ID2SYM(rb_intern(label)));
  rb_funcall(fielddef_rb, rb_intern("name="), 1, name_str);
  rb_funcall(fielddef_rb, rb_intern("type="), 1, type);
  rb_funcall(fielddef_rb, rb_intern("number="), 1, number);

  if (type_class != Qnil) {
    Check_Type(type_class, T_STRING);

    // Make it an absolute type name by prepending a dot.
    type_class = rb_str_append(rb_str_new2("."), type_class);
    rb_funcall(fielddef_rb, rb_intern("submsg_name="), 1, type_class);
  }

  if (options != Qnil) {
    Check_Type(options, T_HASH);

    if (rb_funcall(options, rb_intern("key?"), 1,
		   ID2SYM(rb_intern("default"))) == Qtrue) {
      Descriptor* msgdef = ruby_to_Descriptor(msgdef_rb);
      if (upb_msgdef_syntax((upb_msgdef*)msgdef->msgdef) == UPB_SYNTAX_PROTO3) {
        rb_raise(rb_eArgError, "Cannot set :default when using proto3 syntax.");
      }

      FieldDescriptor* fielddef = ruby_to_FieldDescriptor(fielddef_rb);
      if (!upb_fielddef_haspresence((upb_fielddef*)fielddef->fielddef) ||
	  upb_fielddef_issubmsg((upb_fielddef*)fielddef->fielddef)) {
        rb_raise(rb_eArgError, "Cannot set :default on this kind of field.");
      }

      rb_funcall(fielddef_rb, rb_intern("default="), 1,
		 rb_hash_lookup(options, ID2SYM(rb_intern("default"))));
    }
  }

  rb_funcall(msgdef_rb, rb_intern("add_field"), 1, fielddef_rb);
  return fielddef_rb;
}

/*
 * call-seq:
 *     MessageBuilderContext.optional(name, type, number, type_class = nil,
 *                                    options = nil)
 *
 * Defines a new optional field on this message type with the given type, tag
 * number, and type class (for message and enum fields). The type must be a Ruby
 * symbol (as accepted by FieldDescriptor#type=) and the type_class must be a
 * string, if present (as accepted by FieldDescriptor#submsg_name=).
 */
VALUE MessageBuilderContext_optional(int argc, VALUE* argv, VALUE _self) {
  DEFINE_SELF(MessageBuilderContext, self, _self);
  VALUE name, type, number;
  VALUE type_class, options = Qnil;

  rb_scan_args(argc, argv, "32", &name, &type, &number, &type_class, &options);

  // Allow passing (name, type, number, options) or
  // (name, type, number, type_class, options)
  if (argc == 4 && RB_TYPE_P(type_class, T_HASH)) {
    options = type_class;
    type_class = Qnil;
  }

  return msgdef_add_field(self->descriptor, "optional",
                          name, type, number, type_class, options);
}

/*
 * call-seq:
 *     MessageBuilderContext.required(name, type, number, type_class = nil,
 *                                    options = nil)
 *
 * Defines a new required field on this message type with the given type, tag
 * number, and type class (for message and enum fields). The type must be a Ruby
 * symbol (as accepted by FieldDescriptor#type=) and the type_class must be a
 * string, if present (as accepted by FieldDescriptor#submsg_name=).
 *
 * Proto3 does not have required fields, but this method exists for
 * completeness. Any attempt to add a message type with required fields to a
 * pool will currently result in an error.
 */
VALUE MessageBuilderContext_required(int argc, VALUE* argv, VALUE _self) {
  DEFINE_SELF(MessageBuilderContext, self, _self);
  VALUE name, type, number;
  VALUE type_class, options = Qnil;

  rb_scan_args(argc, argv, "32", &name, &type, &number, &type_class, &options);

  // Allow passing (name, type, number, options) or
  // (name, type, number, type_class, options)
  if (argc == 4 && RB_TYPE_P(type_class, T_HASH)) {
    options = type_class;
    type_class = Qnil;
  }

  return msgdef_add_field(self->descriptor, "required",
                          name, type, number, type_class, options);
}

/*
 * call-seq:
 *     MessageBuilderContext.repeated(name, type, number, type_class = nil)
 *
 * Defines a new repeated field on this message type with the given type, tag
 * number, and type class (for message and enum fields). The type must be a Ruby
 * symbol (as accepted by FieldDescriptor#type=) and the type_class must be a
 * string, if present (as accepted by FieldDescriptor#submsg_name=).
 */
VALUE MessageBuilderContext_repeated(int argc, VALUE* argv, VALUE _self) {
  DEFINE_SELF(MessageBuilderContext, self, _self);
  VALUE name, type, number, type_class;

  if (argc < 3) {
    rb_raise(rb_eArgError, "Expected at least 3 arguments.");
  }
  name = argv[0];
  type = argv[1];
  number = argv[2];
  type_class = (argc > 3) ? argv[3] : Qnil;

  return msgdef_add_field(self->descriptor, "repeated",
                          name, type, number, type_class, Qnil);
}

/*
 * call-seq:
 *     MessageBuilderContext.map(name, key_type, value_type, number,
 *                               value_type_class = nil)
 *
 * Defines a new map field on this message type with the given key and value
 * types, tag number, and type class (for message and enum value types). The key
 * type must be :int32/:uint32/:int64/:uint64, :bool, or :string. The value type
 * type must be a Ruby symbol (as accepted by FieldDescriptor#type=) and the
 * type_class must be a string, if present (as accepted by
 * FieldDescriptor#submsg_name=).
 */
VALUE MessageBuilderContext_map(int argc, VALUE* argv, VALUE _self) {
  DEFINE_SELF(MessageBuilderContext, self, _self);
  VALUE name, key_type, value_type, number, type_class;
  VALUE mapentry_desc, mapentry_desc_name;

  if (argc < 4) {
    rb_raise(rb_eArgError, "Expected at least 4 arguments.");
  }
  name = argv[0];
  key_type = argv[1];
  value_type = argv[2];
  number = argv[3];
  type_class = (argc > 4) ? argv[4] : Qnil;

  // Validate the key type. We can't accept enums, messages, or floats/doubles
  // as map keys. (We exclude these explicitly, and the field-descriptor setter
  // below then ensures that the type is one of the remaining valid options.)
  if (SYM2ID(key_type) == rb_intern("float") ||
      SYM2ID(key_type) == rb_intern("double") ||
      SYM2ID(key_type) == rb_intern("enum") ||
      SYM2ID(key_type) == rb_intern("message")) {
    rb_raise(rb_eArgError,
             "Cannot add a map field with a float, double, enum, or message "
             "type.");
  }

  Descriptor* descriptor = ruby_to_Descriptor(self->descriptor);
  if (upb_msgdef_syntax(descriptor->msgdef) == UPB_SYNTAX_PROTO2) {
    rb_raise(rb_eArgError,
	     "Cannot add a native map field using proto2 syntax.");
  }

  // Create a new message descriptor for the map entry message, and create a
  // repeated submessage field here with that type.
  VALUE file_descriptor_rb =
      rb_funcall(self->descriptor, rb_intern("file_descriptor"), 0);
  mapentry_desc = rb_class_new_instance(1, &file_descriptor_rb, cDescriptor);
  mapentry_desc_name = rb_funcall(self->descriptor, rb_intern("name"), 0);
  mapentry_desc_name = rb_str_cat2(mapentry_desc_name, "_MapEntry_");
  mapentry_desc_name = rb_str_cat2(mapentry_desc_name,
                                   rb_id2name(SYM2ID(name)));
  Descriptor_name_set(mapentry_desc, mapentry_desc_name);

  {
    // The 'mapentry' attribute has no Ruby setter because we do not want the
    // user attempting to DIY the setup below; we want to ensure that the fields
    // are correct. So we reach into the msgdef here to set the bit manually.
    Descriptor* mapentry_desc_self = ruby_to_Descriptor(mapentry_desc);
    upb_msgdef_setmapentry((upb_msgdef*)mapentry_desc_self->msgdef, true);
  }

  {
    // optional <type> key = 1;
    VALUE key_field = rb_class_new_instance(0, NULL, cFieldDescriptor);
    FieldDescriptor_name_set(key_field, rb_str_new2("key"));
    FieldDescriptor_label_set(key_field, ID2SYM(rb_intern("optional")));
    FieldDescriptor_number_set(key_field, INT2NUM(1));
    FieldDescriptor_type_set(key_field, key_type);
    Descriptor_add_field(mapentry_desc, key_field);
  }

  {
    // optional <type> value = 2;
    VALUE value_field = rb_class_new_instance(0, NULL, cFieldDescriptor);
    FieldDescriptor_name_set(value_field, rb_str_new2("value"));
    FieldDescriptor_label_set(value_field, ID2SYM(rb_intern("optional")));
    FieldDescriptor_number_set(value_field, INT2NUM(2));
    FieldDescriptor_type_set(value_field, value_type);
    if (type_class != Qnil) {
      VALUE submsg_name = rb_str_new2("."); // prepend '.' to make absolute.
      submsg_name = rb_str_append(submsg_name, type_class);
      FieldDescriptor_submsg_name_set(value_field, submsg_name);
    }
    Descriptor_add_field(mapentry_desc, value_field);
  }

  {
    // Add the map-entry message type to the current builder, and use the type
    // to create the map field itself.
    Builder* builder = ruby_to_Builder(self->builder);
    rb_ary_push(builder->pending_list, mapentry_desc);
  }

  {
    VALUE map_field = rb_class_new_instance(0, NULL, cFieldDescriptor);
    VALUE name_str = rb_str_new2(rb_id2name(SYM2ID(name)));
    VALUE submsg_name;

    FieldDescriptor_name_set(map_field, name_str);
    FieldDescriptor_number_set(map_field, number);
    FieldDescriptor_label_set(map_field, ID2SYM(rb_intern("repeated")));
    FieldDescriptor_type_set(map_field, ID2SYM(rb_intern("message")));
    submsg_name = rb_str_new2("."); // prepend '.' to make name absolute.
    submsg_name = rb_str_append(submsg_name, mapentry_desc_name);
    FieldDescriptor_submsg_name_set(map_field, submsg_name);
    Descriptor_add_field(self->descriptor, map_field);
  }

  return Qnil;
}

/*
 * call-seq:
 *     MessageBuilderContext.oneof(name, &block) => nil
 *
 * Creates a new OneofDescriptor with the given name, creates a
 * OneofBuilderContext attached to that OneofDescriptor, evaluates the given
 * block in the context of that OneofBuilderContext with #instance_eval, and
 * then adds the oneof to the message.
 *
 * This is the recommended, idiomatic way to build oneof definitions.
 */
VALUE MessageBuilderContext_oneof(VALUE _self, VALUE name) {
  DEFINE_SELF(MessageBuilderContext, self, _self);
  VALUE oneofdef = rb_class_new_instance(0, NULL, cOneofDescriptor);
  VALUE args[2] = { oneofdef, self->builder };
  VALUE ctx = rb_class_new_instance(2, args, cOneofBuilderContext);
  VALUE block = rb_block_proc();
  VALUE name_str = rb_str_new2(rb_id2name(SYM2ID(name)));
  rb_funcall(oneofdef, rb_intern("name="), 1, name_str);
  rb_funcall_with_block(ctx, rb_intern("instance_eval"), 0, NULL, block);
  Descriptor_add_oneof(self->descriptor, oneofdef);

  return Qnil;
}

// -----------------------------------------------------------------------------
// OneofBuilderContext.
// -----------------------------------------------------------------------------

DEFINE_CLASS(OneofBuilderContext,
    "Google::Protobuf::Internal::OneofBuilderContext");

void OneofBuilderContext_mark(void* _self) {
  OneofBuilderContext* self = _self;
  rb_gc_mark(self->descriptor);
  rb_gc_mark(self->builder);
}

void OneofBuilderContext_free(void* _self) {
  OneofBuilderContext* self = _self;
  xfree(self);
}

VALUE OneofBuilderContext_alloc(VALUE klass) {
  OneofBuilderContext* self = ALLOC(OneofBuilderContext);
  VALUE ret = TypedData_Wrap_Struct(
      klass, &_OneofBuilderContext_type, self);
  self->descriptor = Qnil;
  self->builder = Qnil;
  return ret;
}

void OneofBuilderContext_register(VALUE module) {
  VALUE klass = rb_define_class_under(
      module, "OneofBuilderContext", rb_cObject);
  rb_define_alloc_func(klass, OneofBuilderContext_alloc);
  rb_define_method(klass, "initialize",
                   OneofBuilderContext_initialize, 2);
  rb_define_method(klass, "optional", OneofBuilderContext_optional, -1);
  rb_gc_register_address(&cOneofBuilderContext);
  cOneofBuilderContext = klass;
}

/*
 * call-seq:
 *     OneofBuilderContext.new(desc, builder) => context
 *
 * Create a new oneof builder context around the given oneof descriptor and
 * builder context. This class is intended to serve as a DSL context to be used
 * with #instance_eval.
 */
VALUE OneofBuilderContext_initialize(VALUE _self,
                                     VALUE oneofdef,
                                     VALUE builder) {
  DEFINE_SELF(OneofBuilderContext, self, _self);
  self->descriptor = oneofdef;
  self->builder = builder;
  return Qnil;
}

/*
 * call-seq:
 *     OneofBuilderContext.optional(name, type, number, type_class = nil,
 *                                  default_value = nil)
 *
 * Defines a new optional field in this oneof with the given type, tag number,
 * and type class (for message and enum fields). The type must be a Ruby symbol
 * (as accepted by FieldDescriptor#type=) and the type_class must be a string,
 * if present (as accepted by FieldDescriptor#submsg_name=).
 */
VALUE OneofBuilderContext_optional(int argc, VALUE* argv, VALUE _self) {
  DEFINE_SELF(OneofBuilderContext, self, _self);
  VALUE name, type, number;
  VALUE type_class, options = Qnil;

  rb_scan_args(argc, argv, "32", &name, &type, &number, &type_class, &options);

  return msgdef_add_field(self->descriptor, "optional",
                          name, type, number, type_class, options);
}

// -----------------------------------------------------------------------------
// EnumBuilderContext.
// -----------------------------------------------------------------------------

DEFINE_CLASS(EnumBuilderContext,
    "Google::Protobuf::Internal::EnumBuilderContext");

void EnumBuilderContext_mark(void* _self) {
  EnumBuilderContext* self = _self;
  rb_gc_mark(self->enumdesc);
}

void EnumBuilderContext_free(void* _self) {
  EnumBuilderContext* self = _self;
  xfree(self);
}

VALUE EnumBuilderContext_alloc(VALUE klass) {
  EnumBuilderContext* self = ALLOC(EnumBuilderContext);
  VALUE ret = TypedData_Wrap_Struct(
      klass, &_EnumBuilderContext_type, self);
  self->enumdesc = Qnil;
  return ret;
}

void EnumBuilderContext_register(VALUE module) {
  VALUE klass = rb_define_class_under(
      module, "EnumBuilderContext", rb_cObject);
  rb_define_alloc_func(klass, EnumBuilderContext_alloc);
  rb_define_method(klass, "initialize",
                   EnumBuilderContext_initialize, 1);
  rb_define_method(klass, "value", EnumBuilderContext_value, 2);
  rb_gc_register_address(&cEnumBuilderContext);
  cEnumBuilderContext = klass;
}

/*
 * call-seq:
 *     EnumBuilderContext.new(enumdesc) => context
 *
 * Create a new builder context around the given enum descriptor. This class is
 * intended to serve as a DSL context to be used with #instance_eval.
 */
VALUE EnumBuilderContext_initialize(VALUE _self, VALUE enumdef) {
  DEFINE_SELF(EnumBuilderContext, self, _self);
  self->enumdesc = enumdef;
  return Qnil;
}

static VALUE enumdef_add_value(VALUE enumdef,
                               VALUE name, VALUE number) {
  rb_funcall(enumdef, rb_intern("add_value"), 2, name, number);
  return Qnil;
}

/*
 * call-seq:
 *     EnumBuilder.add_value(name, number)
 *
 * Adds the given name => number mapping to the enum type. Name must be a Ruby
 * symbol.
 */
VALUE EnumBuilderContext_value(VALUE _self, VALUE name, VALUE number) {
  DEFINE_SELF(EnumBuilderContext, self, _self);
  return enumdef_add_value(self->enumdesc, name, number);
}


// -----------------------------------------------------------------------------
// FileBuilderContext.
// -----------------------------------------------------------------------------

DEFINE_CLASS(FileBuilderContext,
	     "Google::Protobuf::Internal::FileBuilderContext");

void FileBuilderContext_mark(void* _self) {
  FileBuilderContext* self = _self;
  rb_gc_mark(self->pending_list);
  rb_gc_mark(self->file_descriptor);
  rb_gc_mark(self->builder);
}

void FileBuilderContext_free(void* _self) {
  FileBuilderContext* self = _self;
  xfree(self);
}

VALUE FileBuilderContext_alloc(VALUE klass) {
  FileBuilderContext* self = ALLOC(FileBuilderContext);
  VALUE ret = TypedData_Wrap_Struct(klass, &_FileBuilderContext_type, self);
  self->pending_list = Qnil;
  self->file_descriptor = Qnil;
  self->builder = Qnil;
  return ret;
}

void FileBuilderContext_register(VALUE module) {
  VALUE klass = rb_define_class_under(module, "FileBuilderContext", rb_cObject);
  rb_define_alloc_func(klass, FileBuilderContext_alloc);
  rb_define_method(klass, "initialize", FileBuilderContext_initialize, 2);
  rb_define_method(klass, "add_message", FileBuilderContext_add_message, 1);
  rb_define_method(klass, "add_enum", FileBuilderContext_add_enum, 1);
  rb_gc_register_address(&cFileBuilderContext);
  cFileBuilderContext = klass;
}

/*
 * call-seq:
 *     FileBuilderContext.new(file_descriptor, builder) => context
 *
 * Create a new file builder context for the given file descriptor and
 * builder context. This class is intended to serve as a DSL context to be used
 * with #instance_eval.
 */
VALUE FileBuilderContext_initialize(VALUE _self, VALUE file_descriptor,
				    VALUE builder) {
  DEFINE_SELF(FileBuilderContext, self, _self);
  self->pending_list = rb_ary_new();
  self->file_descriptor = file_descriptor;
  self->builder = builder;
  return Qnil;
}

/*
 * call-seq:
 *     FileBuilderContext.add_message(name, &block)
 *
 * Creates a new, empty descriptor with the given name, and invokes the block in
 * the context of a MessageBuilderContext on that descriptor. The block can then
 * call, e.g., MessageBuilderContext#optional and MessageBuilderContext#repeated
 * methods to define the message fields.
 *
 * This is the recommended, idiomatic way to build message definitions.
 */
VALUE FileBuilderContext_add_message(VALUE _self, VALUE name) {
  DEFINE_SELF(FileBuilderContext, self, _self);
  VALUE msgdef = rb_class_new_instance(1, &self->file_descriptor, cDescriptor);
  VALUE args[2] = { msgdef, self->builder };
  VALUE ctx = rb_class_new_instance(2, args, cMessageBuilderContext);
  VALUE block = rb_block_proc();
  rb_funcall(msgdef, rb_intern("name="), 1, name);
  rb_funcall_with_block(ctx, rb_intern("instance_eval"), 0, NULL, block);
  rb_ary_push(self->pending_list, msgdef);
  return Qnil;
}

/*
 * call-seq:
 *     FileBuilderContext.add_enum(name, &block)
 *
 * Creates a new, empty enum descriptor with the given name, and invokes the
 * block in the context of an EnumBuilderContext on that descriptor. The block
 * can then call EnumBuilderContext#add_value to define the enum values.
 *
 * This is the recommended, idiomatic way to build enum definitions.
 */
VALUE FileBuilderContext_add_enum(VALUE _self, VALUE name) {
  DEFINE_SELF(FileBuilderContext, self, _self);
  VALUE enumdef =
      rb_class_new_instance(1, &self->file_descriptor, cEnumDescriptor);
  VALUE ctx = rb_class_new_instance(1, &enumdef, cEnumBuilderContext);
  VALUE block = rb_block_proc();
  rb_funcall(enumdef, rb_intern("name="), 1, name);
  rb_funcall_with_block(ctx, rb_intern("instance_eval"), 0, NULL, block);
  rb_ary_push(self->pending_list, enumdef);
  return Qnil;
}

VALUE FileBuilderContext_pending_descriptors(VALUE _self) {
  DEFINE_SELF(FileBuilderContext, self, _self);
  return self->pending_list;
}

// -----------------------------------------------------------------------------
// Builder.
// -----------------------------------------------------------------------------

DEFINE_CLASS(Builder, "Google::Protobuf::Internal::Builder");

void Builder_mark(void* _self) {
  Builder* self = _self;
  rb_gc_mark(self->pending_list);
  rb_gc_mark(self->default_file_descriptor);
}

void Builder_free(void* _self) {
  Builder* self = _self;
  xfree(self->defs);
  xfree(self);
}

/*
 * call-seq:
 *     Builder.new => builder
 *
 * Creates a new Builder. A Builder can accumulate a set of new message and enum
 * descriptors and atomically register them into a pool in a way that allows for
 * (co)recursive type references.
 */
VALUE Builder_alloc(VALUE klass) {
  Builder* self = ALLOC(Builder);
  VALUE ret = TypedData_Wrap_Struct(
      klass, &_Builder_type, self);
  self->pending_list = Qnil;
  self->defs = NULL;
  self->default_file_descriptor = Qnil;
  return ret;
}

void Builder_register(VALUE module) {
  VALUE klass = rb_define_class_under(module, "Builder", rb_cObject);
  rb_define_alloc_func(klass, Builder_alloc);
  rb_define_method(klass, "initialize", Builder_initialize, 0);
  rb_define_method(klass, "add_file", Builder_add_file, -1);
  rb_define_method(klass, "add_message", Builder_add_message, 1);
  rb_define_method(klass, "add_enum", Builder_add_enum, 1);
  rb_define_method(klass, "finalize_to_pool", Builder_finalize_to_pool, 1);
  rb_gc_register_address(&cBuilder);
  cBuilder = klass;
}

/*
 * call-seq:
 *    Builder.new
 *
 * Initializes a new builder.
 */
VALUE Builder_initialize(VALUE _self) {
  DEFINE_SELF(Builder, self, _self);
  self->pending_list = rb_ary_new();
  VALUE file_name = Qnil;
  self->default_file_descriptor =
      rb_class_new_instance(1, &file_name, cFileDescriptor);
  return Qnil;
}

/*
 * call-seq:
 *     Builder.add_file(name, options = nil, &block)
 *
 * Creates a new, file descriptor with the given name and options and invokes
 * the block in the context of a FileBuilderContext on that descriptor. The
 * block can then call FileBuilderContext#add_message or
 * FileBuilderContext#add_enum to define new messages or enums, respectively.
 *
 * This is the recommended, idiomatic way to build file descriptors.
 */
VALUE Builder_add_file(int argc, VALUE* argv, VALUE _self) {
  DEFINE_SELF(Builder, self, _self);
  VALUE file_descriptor = rb_class_new_instance(argc, argv, cFileDescriptor);
  VALUE args[2] = { file_descriptor, _self };
  VALUE ctx = rb_class_new_instance(2, args, cFileBuilderContext);
  VALUE block = rb_block_proc();
  rb_funcall_with_block(ctx, rb_intern("instance_eval"), 0, NULL, block);

  rb_ary_concat(self->pending_list,
      FileBuilderContext_pending_descriptors(ctx));
  return Qnil;
}

/*
 * call-seq:
 *     Builder.add_message(name, &block)
 *
 * Old and deprecated way to create a new descriptor.
 * See FileBuilderContext.add_message for the recommended way.
 *
 * Exists for backwards compatibility to allow building descriptor pool for
 * files generated by protoc which don't add messages within "add_file" block.
 * Descriptors created this way get assigned to a default empty FileDescriptor.
 */
VALUE Builder_add_message(VALUE _self, VALUE name) {
  DEFINE_SELF(Builder, self, _self);
  VALUE msgdef =
      rb_class_new_instance(1, &self->default_file_descriptor, cDescriptor);
  VALUE args[2] = { msgdef, _self };
  VALUE ctx = rb_class_new_instance(2, args, cMessageBuilderContext);
  VALUE block = rb_block_proc();
  rb_funcall(msgdef, rb_intern("name="), 1, name);
  rb_funcall_with_block(ctx, rb_intern("instance_eval"), 0, NULL, block);
  rb_ary_push(self->pending_list, msgdef);
  return Qnil;
}

/*
 * call-seq:
 *     Builder.add_enum(name, &block)
 *
 * Old and deprecated way to create a new enum descriptor.
 * See FileBuilderContext.add_enum for the recommended way.
 *
 * Exists for backwards compatibility to allow building descriptor pool for
 * files generated by protoc which don't add enums within "add_file" block.
 * Enum descriptors created this way get assigned to a default empty
 * FileDescriptor.
 */
VALUE Builder_add_enum(VALUE _self, VALUE name) {
  DEFINE_SELF(Builder, self, _self);
  VALUE enumdef =
      rb_class_new_instance(1, &self->default_file_descriptor, cEnumDescriptor);
  VALUE ctx = rb_class_new_instance(1, &enumdef, cEnumBuilderContext);
  VALUE block = rb_block_proc();
  rb_funcall(enumdef, rb_intern("name="), 1, name);
  rb_funcall_with_block(ctx, rb_intern("instance_eval"), 0, NULL, block);
  rb_ary_push(self->pending_list, enumdef);
  return Qnil;
}

static void proto3_validate_msgdef(const upb_msgdef* msgdef) {
  // Verify that no required fields exist. proto3 does not support these.
  upb_msg_field_iter it;
  for (upb_msg_field_begin(&it, msgdef);
       !upb_msg_field_done(&it);
       upb_msg_field_next(&it)) {
    const upb_fielddef* field = upb_msg_iter_field(&it);
    if (upb_fielddef_label(field) == UPB_LABEL_REQUIRED) {
      rb_raise(cTypeError, "Required fields are unsupported in proto3.");
    }
  }
}

static void proto3_validate_enumdef(const upb_enumdef* enumdef) {
  // Verify that an entry exists with integer value 0. (This is the default
  // value.)
  const char* lookup = upb_enumdef_iton(enumdef, 0);
  if (lookup == NULL) {
    rb_raise(cTypeError,
             "Enum definition does not contain a value for '0'.");
  }
}

/*
 * call-seq:
 *     Builder.finalize_to_pool(pool)
 *
 * Adds all accumulated message and enum descriptors created in this builder
 * context to the given pool. The operation occurs atomically, and all
 * descriptors can refer to each other (including in cycles). This is the only
 * way to build (co)recursive message definitions.
 *
 * This method is usually called automatically by DescriptorPool#build after it
 * invokes the given user block in the context of the builder. The user should
 * not normally need to call this manually because a Builder is not normally
 * created manually.
 */
VALUE Builder_finalize_to_pool(VALUE _self, VALUE pool_rb) {
  DEFINE_SELF(Builder, self, _self);

  DescriptorPool* pool = ruby_to_DescriptorPool(pool_rb);

  REALLOC_N(self->defs, upb_def*, RARRAY_LEN(self->pending_list));

  for (int i = 0; i < RARRAY_LEN(self->pending_list); i++) {
    VALUE def_rb = rb_ary_entry(self->pending_list, i);
    if (CLASS_OF(def_rb) == cDescriptor) {
      self->defs[i] = (upb_def*)ruby_to_Descriptor(def_rb)->msgdef;

      if (upb_filedef_syntax(upb_def_file(self->defs[i])) == UPB_SYNTAX_PROTO3) {
        proto3_validate_msgdef((const upb_msgdef*)self->defs[i]);
      }
    } else if (CLASS_OF(def_rb) == cEnumDescriptor) {
      self->defs[i] = (upb_def*)ruby_to_EnumDescriptor(def_rb)->enumdef;

      if (upb_filedef_syntax(upb_def_file(self->defs[i])) == UPB_SYNTAX_PROTO3) {
        proto3_validate_enumdef((const upb_enumdef*)self->defs[i]);
      }
    }
  }

  CHECK_UPB(upb_symtab_add(pool->symtab, (upb_def**)self->defs,
                           RARRAY_LEN(self->pending_list), NULL, &status),
            "Unable to add defs to DescriptorPool");

  for (int i = 0; i < RARRAY_LEN(self->pending_list); i++) {
    VALUE def_rb = rb_ary_entry(self->pending_list, i);
    add_def_obj(self->defs[i], def_rb);
  }

  self->pending_list = rb_ary_new();
  return Qnil;
}
