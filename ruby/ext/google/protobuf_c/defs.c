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

static void rewrite_enum_default(const upb_symtab* symtab,
                                 google_protobuf_FileDescriptorProto* file,
                                 google_protobuf_FieldDescriptorProto* field) {
  upb_strview defaultval;
  const char *type_name_str;
  char *end;
  long val;
  const upb_enumdef *e;
  upb_strview type_name;

  /* Look for TYPE_ENUM fields that have a default. */
  if (google_protobuf_FieldDescriptorProto_type(field) !=
          google_protobuf_FieldDescriptorProto_TYPE_ENUM ||
      !google_protobuf_FieldDescriptorProto_has_default_value(field) ||
      !google_protobuf_FieldDescriptorProto_has_type_name(field)) {
    return;
  }

  defaultval = google_protobuf_FieldDescriptorProto_default_value(field);
  type_name = google_protobuf_FieldDescriptorProto_type_name(field);

  if (defaultval.size == 0 || !isdigit(defaultval.data[0])) {
    return;
  }

  if (type_name.size == 0 || type_name.data[0] != '.') {
    return;
  }

  type_name_str = type_name.data + 1;

  errno = 0;
  val = strtol(defaultval.data, &end, 10);

  if (errno != 0 || *end != 0 || val < INT32_MIN || val > INT32_MAX) {
    return;
  }

  /* Now find the corresponding enum definition. */
  e = upb_symtab_lookupenum(symtab, type_name_str);
  if (e) {
    /* Look in previously loaded files. */
    const char *label = upb_enumdef_iton(e, val);
    if (!label) {
      return;
    }
    google_protobuf_FieldDescriptorProto_set_default_value(
        field, upb_strview_makez(label));
  } else {
    /* Look in enums defined in this file. */
    const google_protobuf_EnumDescriptorProto* matching_enum = NULL;
    size_t i, n;
    const google_protobuf_EnumDescriptorProto* const* enums =
        google_protobuf_FileDescriptorProto_enum_type(file, &n);
    const google_protobuf_EnumValueDescriptorProto* const* values;

    for (i = 0; i < n; i++) {
      if (upb_strview_eql(google_protobuf_EnumDescriptorProto_name(enums[i]),
                          upb_strview_makez(type_name_str))) {
        matching_enum = enums[i];
        break;
      }
    }

    if (!matching_enum) {
      return;
    }

    values = google_protobuf_EnumDescriptorProto_value(matching_enum, &n);
    for (i = 0; i < n; i++) {
      if (google_protobuf_EnumValueDescriptorProto_number(values[i]) == val) {
        google_protobuf_FieldDescriptorProto_set_default_value(
            field, google_protobuf_EnumValueDescriptorProto_name(values[i]));
        return;
      }
    }

    /* We failed to find an enum default.  But we'll just leave the enum
     * untouched and let the normal def-building code catch it. */
  }
}

/* Historically we allowed enum defaults to be specified as a number.  In
 * retrospect this was a mistake as descriptors require defaults to be
 * specified as a label. This can make a difference if multiple labels have the
 * same number.
 *
 * Here we do a pass over all enum defaults and rewrite numeric defaults by
 * looking up their labels.  This is compilcated by the fact that the enum
 * definition can live in either the symtab or the file_proto.
 * */
static void rewrite_enum_defaults(
    const upb_symtab* symtab, google_protobuf_FileDescriptorProto* file_proto) {
  size_t i, n;
  google_protobuf_DescriptorProto** msgs =
      google_protobuf_FileDescriptorProto_mutable_message_type(file_proto, &n);

  for (i = 0; i < n; i++) {
    size_t j, m;
    google_protobuf_FieldDescriptorProto** fields =
        google_protobuf_DescriptorProto_mutable_field(msgs[i], &m);
    for (j = 0; j < m; j++) {
      rewrite_enum_default(symtab, file_proto, fields[j]);
    }
  }
}

static void remove_path(upb_strview *name) {
  const char* last = strrchr(name->data, '.');
  if (last) {
    size_t remove = last - name->data + 1;
    name->data += remove;
    name->size -= remove;
  }
}

static void rewrite_nesting(VALUE msg_ent, google_protobuf_DescriptorProto* msg,
                            google_protobuf_DescriptorProto* const* msgs,
                            google_protobuf_EnumDescriptorProto* const* enums,
                            upb_arena *arena) {
  VALUE submsgs = rb_hash_aref(msg_ent, ID2SYM(rb_intern("msgs")));
  VALUE enum_pos = rb_hash_aref(msg_ent, ID2SYM(rb_intern("enums")));
  int submsg_count;
  int enum_count;
  int i;
  google_protobuf_DescriptorProto** msg_msgs;
  google_protobuf_EnumDescriptorProto** msg_enums;

  Check_Type(submsgs, T_ARRAY);
  Check_Type(enum_pos, T_ARRAY);

  submsg_count = RARRAY_LEN(submsgs);
  enum_count = RARRAY_LEN(enum_pos);

  msg_msgs = google_protobuf_DescriptorProto_resize_nested_type(
      msg, submsg_count, arena);
  msg_enums =
      google_protobuf_DescriptorProto_resize_enum_type(msg, enum_count, arena);

  for (i = 0; i < submsg_count; i++) {
    VALUE submsg_ent = RARRAY_PTR(submsgs)[i];
    VALUE pos = rb_hash_aref(submsg_ent, ID2SYM(rb_intern("pos")));
    upb_strview name;

    msg_msgs[i] = msgs[NUM2INT(pos)];
    name = google_protobuf_DescriptorProto_name(msg_msgs[i]);
    remove_path(&name);
    google_protobuf_DescriptorProto_set_name(msg_msgs[i], name);
    rewrite_nesting(submsg_ent, msg_msgs[i], msgs, enums, arena);
  }

  for (i = 0; i < enum_count; i++) {
    VALUE pos = RARRAY_PTR(enum_pos)[i];
    msg_enums[i] = enums[NUM2INT(pos)];
  }
}

/* We have to do some relatively complicated logic here for backward
 * compatibility.
 *
 * In descriptor.proto, messages are nested inside other messages if that is
 * what the original .proto file looks like.  For example, suppose we have this
 * foo.proto:
 *
 * package foo;
 * message Bar {
 *   message Baz {}
 * }
 *
 * The descriptor for this must look like this:
 *
 * file {
 *   name: "test.proto"
 *   package: "foo"
 *   message_type {
 *     name: "Bar"
 *     nested_type {
 *       name: "Baz"
 *     }
 *   }
 * }
 *
 * However, the Ruby generated code has always generated messages in a flat,
 * non-nested way:
 *
 * Google::Protobuf::DescriptorPool.generated_pool.build do
 *   add_message "foo.Bar" do
 *   end
 *   add_message "foo.Bar.Baz" do
 *   end
 * end
 *
 * Here we need to do a translation where we turn this generated code into the
 * above descriptor.  We need to infer that "foo" is the package name, and not
 * a message itself.
 *
 * We delegate to Ruby to compute the transformation, for more concice and
 * readable code than we can do in C */
static void rewrite_names(VALUE _file_builder,
                          google_protobuf_FileDescriptorProto* file_proto) {
  FileBuilderContext* file_builder = ruby_to_FileBuilderContext(_file_builder);
  upb_arena *arena = file_builder->arena;
  // Build params (package, msg_names, enum_names).
  VALUE package = Qnil;
  VALUE msg_names = rb_ary_new();
  VALUE enum_names = rb_ary_new();
  size_t msg_count, enum_count, i;
  VALUE new_package, nesting, msg_ents, enum_ents;
  google_protobuf_DescriptorProto** msgs;
  google_protobuf_EnumDescriptorProto** enums;

  if (google_protobuf_FileDescriptorProto_has_package(file_proto)) {
    upb_strview package_str =
        google_protobuf_FileDescriptorProto_package(file_proto);
    package = rb_str_new(package_str.data, package_str.size);
  }

  msgs = google_protobuf_FileDescriptorProto_mutable_message_type(file_proto,
                                                                  &msg_count);
  for (i = 0; i < msg_count; i++) {
    upb_strview name = google_protobuf_DescriptorProto_name(msgs[i]);
    rb_ary_push(msg_names, rb_str_new(name.data, name.size));
  }

  enums = google_protobuf_FileDescriptorProto_mutable_enum_type(file_proto,
                                                                &enum_count);
  for (i = 0; i < enum_count; i++) {
    upb_strview name = google_protobuf_EnumDescriptorProto_name(enums[i]);
    rb_ary_push(enum_names, rb_str_new(name.data, name.size));
  }

  {
    // Call Ruby code to calculate package name and nesting.
    VALUE args[3] = { package, msg_names, enum_names };
    VALUE internal = rb_eval_string("Google::Protobuf::Internal");
    VALUE ret = rb_funcallv(internal, rb_intern("fixup_descriptor"), 3, args);

    new_package = rb_ary_entry(ret, 0);
    nesting = rb_ary_entry(ret, 1);
  }

  // Rewrite package and names.
  if (new_package != Qnil) {
    upb_strview new_package_str =
        FileBuilderContext_strdup(_file_builder, new_package);
    google_protobuf_FileDescriptorProto_set_package(file_proto,
                                                    new_package_str);
  }

  for (i = 0; i < msg_count; i++) {
    upb_strview name = google_protobuf_DescriptorProto_name(msgs[i]);
    remove_path(&name);
    google_protobuf_DescriptorProto_set_name(msgs[i], name);
  }

  for (i = 0; i < enum_count; i++) {
    upb_strview name = google_protobuf_EnumDescriptorProto_name(enums[i]);
    remove_path(&name);
    google_protobuf_EnumDescriptorProto_set_name(enums[i], name);
  }

  // Rewrite nesting.
  msg_ents = rb_hash_aref(nesting, ID2SYM(rb_intern("msgs")));
  enum_ents = rb_hash_aref(nesting, ID2SYM(rb_intern("enums")));

  Check_Type(msg_ents, T_ARRAY);
  Check_Type(enum_ents, T_ARRAY);

  for (i = 0; i < (size_t)RARRAY_LEN(msg_ents); i++) {
    VALUE msg_ent = rb_ary_entry(msg_ents, i);
    VALUE pos = rb_hash_aref(msg_ent, ID2SYM(rb_intern("pos")));
    msgs[i] = msgs[NUM2INT(pos)];
    rewrite_nesting(msg_ent, msgs[i], msgs, enums, arena);
  }

  for (i = 0; i < (size_t)RARRAY_LEN(enum_ents); i++) {
    VALUE enum_pos = rb_ary_entry(enum_ents, i);
    enums[i] = enums[NUM2INT(enum_pos)];
  }

  google_protobuf_FileDescriptorProto_resize_message_type(
      file_proto, RARRAY_LEN(msg_ents), arena);
  google_protobuf_FileDescriptorProto_resize_enum_type(
      file_proto, RARRAY_LEN(enum_ents), arena);
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
VALUE generated_pool = Qnil;

DEFINE_CLASS(DescriptorPool, "Google::Protobuf::DescriptorPool");

void DescriptorPool_mark(void* _self) {
  DescriptorPool* self = _self;
  rb_gc_mark(self->def_to_descriptor);
}

void DescriptorPool_free(void* _self) {
  DescriptorPool* self = _self;

  upb_symtab_free(self->symtab);
  upb_handlercache_free(self->fill_handler_cache);
  upb_handlercache_free(self->pb_serialize_handler_cache);
  upb_handlercache_free(self->json_serialize_handler_cache);
  upb_handlercache_free(self->json_serialize_handler_preserve_cache);
  upb_pbcodecache_free(self->fill_method_cache);
  upb_json_codecache_free(self->json_fill_method_cache);

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
  VALUE ret;

  self->def_to_descriptor = Qnil;
  ret = TypedData_Wrap_Struct(klass, &_DescriptorPool_type, self);

  self->def_to_descriptor = rb_hash_new();
  self->symtab = upb_symtab_new();
  self->fill_handler_cache =
      upb_handlercache_new(add_handlers_for_message, (void*)ret);
  self->pb_serialize_handler_cache = upb_pb_encoder_newcache();
  self->json_serialize_handler_cache = upb_json_printer_newcache(false);
  self->json_serialize_handler_preserve_cache =
      upb_json_printer_newcache(true);
  self->fill_method_cache = upb_pbcodecache_new(self->fill_handler_cache);
  self->json_fill_method_cache = upb_json_codecache_new();

  return ret;
}

void DescriptorPool_register(VALUE module) {
  VALUE klass = rb_define_class_under(
      module, "DescriptorPool", rb_cObject);
  rb_define_alloc_func(klass, DescriptorPool_alloc);
  rb_define_method(klass, "build", DescriptorPool_build, -1);
  rb_define_method(klass, "lookup", DescriptorPool_lookup, 1);
  rb_define_singleton_method(klass, "generated_pool",
                             DescriptorPool_generated_pool, 0);
  rb_gc_register_address(&cDescriptorPool);
  cDescriptorPool = klass;

  rb_gc_register_address(&generated_pool);
  generated_pool = rb_class_new_instance(0, NULL, klass);
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
  VALUE ctx = rb_class_new_instance(1, &_self, cBuilder);
  VALUE block = rb_block_proc();
  rb_funcall_with_block(ctx, rb_intern("instance_eval"), 0, NULL, block);
  Builder_build(ctx);
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
  const upb_msgdef* msgdef;
  const upb_enumdef* enumdef;

  msgdef = upb_symtab_lookupmsg(self->symtab, name_str);
  if (msgdef) {
    return get_msgdef_obj(_self, msgdef);
  }

  enumdef = upb_symtab_lookupenum(self->symtab, name_str);
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
  rb_gc_mark(self->descriptor_pool);
  if (self->layout && self->layout->empty_template) {
    layout_mark(self->layout, self->layout->empty_template);
  }
}

void Descriptor_free(void* _self) {
  Descriptor* self = _self;
  if (self->layout) {
    free_layout(self->layout);
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
  self->msgdef = NULL;
  self->klass = Qnil;
  self->descriptor_pool = Qnil;
  self->layout = NULL;
  return ret;
}

void Descriptor_register(VALUE module) {
  VALUE klass = rb_define_class_under(
      module, "Descriptor", rb_cObject);
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

/*
 * call-seq:
 *    Descriptor.new(c_only_cookie, ptr) => Descriptor
 *
 * Creates a descriptor wrapper object.  May only be called from C.
 */
VALUE Descriptor_initialize(VALUE _self, VALUE cookie,
                            VALUE descriptor_pool, VALUE ptr) {
  DEFINE_SELF(Descriptor, self, _self);

  if (cookie != c_only_cookie) {
    rb_raise(rb_eRuntimeError,
             "Descriptor objects may not be created from Ruby.");
  }

  self->descriptor_pool = descriptor_pool;
  self->msgdef = (const upb_msgdef*)NUM2ULL(ptr);

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
  return get_filedef_obj(self->descriptor_pool, upb_msgdef_file(self->msgdef));
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
VALUE Descriptor_lookup(VALUE _self, VALUE name) {
  DEFINE_SELF(Descriptor, self, _self);
  const char* s = get_str(name);
  const upb_fielddef* field = upb_msgdef_ntofz(self->msgdef, s);
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
VALUE Descriptor_each_oneof(VALUE _self) {
  DEFINE_SELF(Descriptor, self, _self);

  upb_msg_oneof_iter it;
  for (upb_msg_oneof_begin(&it, self->msgdef);
       !upb_msg_oneof_done(&it);
       upb_msg_oneof_next(&it)) {
    const upb_oneofdef* oneof = upb_msg_iter_oneof(&it);
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
VALUE Descriptor_lookup_oneof(VALUE _self, VALUE name) {
  DEFINE_SELF(Descriptor, self, _self);
  const char* s = get_str(name);
  const upb_oneofdef* oneof = upb_msgdef_ntooz(self->msgdef, s);
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
VALUE Descriptor_msgclass(VALUE _self) {
  DEFINE_SELF(Descriptor, self, _self);
  if (self->klass == Qnil) {
    self->klass = build_class_from_descriptor(_self);
  }
  return self->klass;
}

// -----------------------------------------------------------------------------
// FileDescriptor.
// -----------------------------------------------------------------------------

DEFINE_CLASS(FileDescriptor, "Google::Protobuf::FileDescriptor");

void FileDescriptor_mark(void* _self) {
  FileDescriptor* self = _self;
  rb_gc_mark(self->descriptor_pool);
}

void FileDescriptor_free(void* _self) {
  xfree(_self);
}

VALUE FileDescriptor_alloc(VALUE klass) {
  FileDescriptor* self = ALLOC(FileDescriptor);
  VALUE ret = TypedData_Wrap_Struct(klass, &_FileDescriptor_type, self);
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
VALUE FileDescriptor_initialize(VALUE _self, VALUE cookie,
                                VALUE descriptor_pool, VALUE ptr) {
  DEFINE_SELF(FileDescriptor, self, _self);

  if (cookie != c_only_cookie) {
    rb_raise(rb_eRuntimeError,
             "Descriptor objects may not be created from Ruby.");
  }

  self->descriptor_pool = descriptor_pool;
  self->filedef = (const upb_filedef*)NUM2ULL(ptr);

  return Qnil;
}

void FileDescriptor_register(VALUE module) {
  VALUE klass = rb_define_class_under(
      module, "FileDescriptor", rb_cObject);
  rb_define_alloc_func(klass, FileDescriptor_alloc);
  rb_define_method(klass, "initialize", FileDescriptor_initialize, 3);
  rb_define_method(klass, "name", FileDescriptor_name, 0);
  rb_define_method(klass, "syntax", FileDescriptor_syntax, 0);
  rb_gc_register_address(&cFileDescriptor);
  cFileDescriptor = klass;
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

// -----------------------------------------------------------------------------
// FieldDescriptor.
// -----------------------------------------------------------------------------

DEFINE_CLASS(FieldDescriptor, "Google::Protobuf::FieldDescriptor");

void FieldDescriptor_mark(void* _self) {
  FieldDescriptor* self = _self;
  rb_gc_mark(self->descriptor_pool);
}

void FieldDescriptor_free(void* _self) {
  xfree(_self);
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
  self->fielddef = NULL;
  return ret;
}

void FieldDescriptor_register(VALUE module) {
  VALUE klass = rb_define_class_under(
      module, "FieldDescriptor", rb_cObject);
  rb_define_alloc_func(klass, FieldDescriptor_alloc);
  rb_define_method(klass, "initialize", FieldDescriptor_initialize, 3);
  rb_define_method(klass, "name", FieldDescriptor_name, 0);
  rb_define_method(klass, "type", FieldDescriptor_type, 0);
  rb_define_method(klass, "default", FieldDescriptor_default, 0);
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

/*
 * call-seq:
 *    EnumDescriptor.new(c_only_cookie, pool, ptr) => EnumDescriptor
 *
 * Creates a descriptor wrapper object.  May only be called from C.
 */
VALUE FieldDescriptor_initialize(VALUE _self, VALUE cookie,
                                 VALUE descriptor_pool, VALUE ptr) {
  DEFINE_SELF(FieldDescriptor, self, _self);

  if (cookie != c_only_cookie) {
    rb_raise(rb_eRuntimeError,
             "Descriptor objects may not be created from Ruby.");
  }

  self->descriptor_pool = descriptor_pool;
  self->fielddef = (const upb_fielddef*)NUM2ULL(ptr);

  return Qnil;
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

VALUE ruby_to_label(VALUE label) {
  upb_label_t upb_label;
  bool converted = false;

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

  return upb_label;
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
  return descriptortype_to_ruby(upb_fielddef_descriptortype(self->fielddef));
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
 *     FieldDescriptor.submsg_name => submsg_name
 *
 * Returns the name of the message or enum type corresponding to this field, if
 * it is a message or enum field (respectively), or nil otherwise. This type
 * name will be resolved within the context of the pool to which the containing
 * message type is added.
 */
VALUE FieldDescriptor_submsg_name(VALUE _self) {
  DEFINE_SELF(FieldDescriptor, self, _self);
  switch (upb_fielddef_type(self->fielddef)) {
    case UPB_TYPE_ENUM:
      return rb_str_new2(
          upb_enumdef_fullname(upb_fielddef_enumsubdef(self->fielddef)));
    case UPB_TYPE_MESSAGE:
      return rb_str_new2(
          upb_msgdef_fullname(upb_fielddef_msgsubdef(self->fielddef)));
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
VALUE FieldDescriptor_subtype(VALUE _self) {
  DEFINE_SELF(FieldDescriptor, self, _self);
  switch (upb_fielddef_type(self->fielddef)) {
    case UPB_TYPE_ENUM:
      return get_enumdef_obj(self->descriptor_pool,
                             upb_fielddef_enumsubdef(self->fielddef));
    case UPB_TYPE_MESSAGE:
      return get_msgdef_obj(self->descriptor_pool,
                            upb_fielddef_msgsubdef(self->fielddef));
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
  OneofDescriptor* self = _self;
  rb_gc_mark(self->descriptor_pool);
}

void OneofDescriptor_free(void* _self) {
  xfree(_self);
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
  self->oneofdef = NULL;
  self->descriptor_pool = Qnil;
  return ret;
}

void OneofDescriptor_register(VALUE module) {
  VALUE klass = rb_define_class_under(
      module, "OneofDescriptor", rb_cObject);
  rb_define_alloc_func(klass, OneofDescriptor_alloc);
  rb_define_method(klass, "initialize", OneofDescriptor_initialize, 3);
  rb_define_method(klass, "name", OneofDescriptor_name, 0);
  rb_define_method(klass, "each", OneofDescriptor_each, 0);
  rb_include_module(klass, rb_mEnumerable);
  rb_gc_register_address(&cOneofDescriptor);
  cOneofDescriptor = klass;
}

/*
 * call-seq:
 *    OneofDescriptor.new(c_only_cookie, pool, ptr) => OneofDescriptor
 *
 * Creates a descriptor wrapper object.  May only be called from C.
 */
VALUE OneofDescriptor_initialize(VALUE _self, VALUE cookie,
                                 VALUE descriptor_pool, VALUE ptr) {
  DEFINE_SELF(OneofDescriptor, self, _self);

  if (cookie != c_only_cookie) {
    rb_raise(rb_eRuntimeError,
             "Descriptor objects may not be created from Ruby.");
  }

  self->descriptor_pool = descriptor_pool;
  self->oneofdef = (const upb_oneofdef*)NUM2ULL(ptr);

  return Qnil;
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
 *     OneofDescriptor.each(&block) => nil
 *
 * Iterates through fields in this oneof, yielding to the block on each one.
 */
VALUE OneofDescriptor_each(VALUE _self) {
  DEFINE_SELF(OneofDescriptor, self, _self);
  upb_oneof_iter it;
  for (upb_oneof_begin(&it, self->oneofdef);
       !upb_oneof_done(&it);
       upb_oneof_next(&it)) {
    const upb_fielddef* f = upb_oneof_iter_field(&it);
    VALUE obj = get_fielddef_obj(self->descriptor_pool, f);
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
  rb_gc_mark(self->descriptor_pool);
}

void EnumDescriptor_free(void* _self) {
  xfree(_self);
}

VALUE EnumDescriptor_alloc(VALUE klass) {
  EnumDescriptor* self = ALLOC(EnumDescriptor);
  VALUE ret = TypedData_Wrap_Struct(klass, &_EnumDescriptor_type, self);
  self->enumdef = NULL;
  self->module = Qnil;
  self->descriptor_pool = Qnil;
  return ret;
}

/*
 * call-seq:
 *    EnumDescriptor.new(c_only_cookie, ptr) => EnumDescriptor
 *
 * Creates a descriptor wrapper object.  May only be called from C.
 */
VALUE EnumDescriptor_initialize(VALUE _self, VALUE cookie,
                                VALUE descriptor_pool, VALUE ptr) {
  DEFINE_SELF(EnumDescriptor, self, _self);

  if (cookie != c_only_cookie) {
    rb_raise(rb_eRuntimeError,
             "Descriptor objects may not be created from Ruby.");
  }

  self->descriptor_pool = descriptor_pool;
  self->enumdef = (const upb_enumdef*)NUM2ULL(ptr);

  return Qnil;
}

void EnumDescriptor_register(VALUE module) {
  VALUE klass = rb_define_class_under(
      module, "EnumDescriptor", rb_cObject);
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

/*
 * call-seq:
 *    EnumDescriptor.file_descriptor
 *
 * Returns the FileDescriptor object this enum belongs to.
 */
VALUE EnumDescriptor_file_descriptor(VALUE _self) {
  DEFINE_SELF(EnumDescriptor, self, _self);
  return get_filedef_obj(self->descriptor_pool,
                         upb_enumdef_file(self->enumdef));
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
 * Returns the Ruby module corresponding to this enum type.
 */
VALUE EnumDescriptor_enummodule(VALUE _self) {
  DEFINE_SELF(EnumDescriptor, self, _self);
  if (self->module == Qnil) {
    self->module = build_module_from_enumdesc(_self);
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
  rb_gc_mark(self->file_builder);
}

void MessageBuilderContext_free(void* _self) {
  MessageBuilderContext* self = _self;
  xfree(self);
}

VALUE MessageBuilderContext_alloc(VALUE klass) {
  MessageBuilderContext* self = ALLOC(MessageBuilderContext);
  VALUE ret = TypedData_Wrap_Struct(
      klass, &_MessageBuilderContext_type, self);
  self->file_builder = Qnil;
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
 *     MessageBuilderContext.new(file_builder, name) => context
 *
 * Create a new message builder context around the given message descriptor and
 * builder context. This class is intended to serve as a DSL context to be used
 * with #instance_eval.
 */
VALUE MessageBuilderContext_initialize(VALUE _self,
                                       VALUE _file_builder,
                                       VALUE name) {
  DEFINE_SELF(MessageBuilderContext, self, _self);
  FileBuilderContext* file_builder = ruby_to_FileBuilderContext(_file_builder);
  google_protobuf_FileDescriptorProto* file_proto = file_builder->file_proto;

  self->file_builder = _file_builder;
  self->msg_proto = google_protobuf_FileDescriptorProto_add_message_type(
      file_proto, file_builder->arena);

  google_protobuf_DescriptorProto_set_name(
      self->msg_proto, FileBuilderContext_strdup(_file_builder, name));

  return Qnil;
}

static void msgdef_add_field(VALUE msgbuilder_rb, upb_label_t label, VALUE name,
                             VALUE type, VALUE number, VALUE type_class,
                             VALUE options, int oneof_index) {
  DEFINE_SELF(MessageBuilderContext, self, msgbuilder_rb);
  FileBuilderContext* file_context =
      ruby_to_FileBuilderContext(self->file_builder);
  google_protobuf_FieldDescriptorProto* field_proto;
  VALUE name_str;

  field_proto = google_protobuf_DescriptorProto_add_field(self->msg_proto,
                                                          file_context->arena);

  Check_Type(name, T_SYMBOL);
  name_str = rb_id2str(SYM2ID(name));

  google_protobuf_FieldDescriptorProto_set_name(
      field_proto, FileBuilderContext_strdup(self->file_builder, name_str));
  google_protobuf_FieldDescriptorProto_set_number(field_proto, NUM2INT(number));
  google_protobuf_FieldDescriptorProto_set_label(field_proto, (int)label);
  google_protobuf_FieldDescriptorProto_set_type(
      field_proto, (int)ruby_to_descriptortype(type));

  if (type_class != Qnil) {
    Check_Type(type_class, T_STRING);

    // Make it an absolute type name by prepending a dot.
    type_class = rb_str_append(rb_str_new2("."), type_class);
    google_protobuf_FieldDescriptorProto_set_type_name(
        field_proto, FileBuilderContext_strdup(self->file_builder, type_class));
  }

  if (options != Qnil) {
    Check_Type(options, T_HASH);

    if (rb_funcall(options, rb_intern("key?"), 1,
                   ID2SYM(rb_intern("default"))) == Qtrue) {
      VALUE default_value =
          rb_hash_lookup(options, ID2SYM(rb_intern("default")));

      /* Call #to_s since all defaults are strings in the descriptor. */
      default_value = rb_funcall(default_value, rb_intern("to_s"), 0);

      google_protobuf_FieldDescriptorProto_set_default_value(
          field_proto,
          FileBuilderContext_strdup(self->file_builder, default_value));
    }
  }

  if (oneof_index >= 0) {
    google_protobuf_FieldDescriptorProto_set_oneof_index(field_proto,
                                                         oneof_index);
  }
}

static VALUE make_mapentry(VALUE _message_builder, VALUE types, int argc,
                           VALUE* argv) {
  DEFINE_SELF(MessageBuilderContext, message_builder, _message_builder);
  VALUE type_class = rb_ary_entry(types, 2);
  FileBuilderContext* file_context =
      ruby_to_FileBuilderContext(message_builder->file_builder);
  google_protobuf_MessageOptions* options =
      google_protobuf_DescriptorProto_mutable_options(
          message_builder->msg_proto, file_context->arena);

  google_protobuf_MessageOptions_set_map_entry(options, true);

  // optional <type> key = 1;
  rb_funcall(_message_builder, rb_intern("optional"), 3,
             ID2SYM(rb_intern("key")), rb_ary_entry(types, 0), INT2NUM(1));

  // optional <type> value = 2;
  if (type_class == Qnil) {
    rb_funcall(_message_builder, rb_intern("optional"), 3,
               ID2SYM(rb_intern("value")), rb_ary_entry(types, 1), INT2NUM(2));
  } else {
    rb_funcall(_message_builder, rb_intern("optional"), 4,
               ID2SYM(rb_intern("value")), rb_ary_entry(types, 1), INT2NUM(2),
               type_class);
  }

  return Qnil;
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
  VALUE name, type, number;
  VALUE type_class, options = Qnil;

  rb_scan_args(argc, argv, "32", &name, &type, &number, &type_class, &options);

  // Allow passing (name, type, number, options) or
  // (name, type, number, type_class, options)
  if (argc == 4 && RB_TYPE_P(type_class, T_HASH)) {
    options = type_class;
    type_class = Qnil;
  }

  msgdef_add_field(_self, UPB_LABEL_OPTIONAL, name, type, number, type_class,
                   options, -1);

  return Qnil;
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
  VALUE name, type, number;
  VALUE type_class, options = Qnil;

  rb_scan_args(argc, argv, "32", &name, &type, &number, &type_class, &options);

  // Allow passing (name, type, number, options) or
  // (name, type, number, type_class, options)
  if (argc == 4 && RB_TYPE_P(type_class, T_HASH)) {
    options = type_class;
    type_class = Qnil;
  }

  msgdef_add_field(_self, UPB_LABEL_REQUIRED, name, type, number, type_class,
                   options, -1);

  return Qnil;
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
  VALUE name, type, number, type_class;

  if (argc < 3) {
    rb_raise(rb_eArgError, "Expected at least 3 arguments.");
  }
  name = argv[0];
  type = argv[1];
  number = argv[2];
  type_class = (argc > 3) ? argv[3] : Qnil;

  msgdef_add_field(_self, UPB_LABEL_REPEATED, name, type, number, type_class,
                   Qnil, -1);

  return Qnil;
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
  VALUE mapentry_desc_name;
  FileBuilderContext* file_builder;
  upb_strview msg_name;

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

  file_builder = ruby_to_FileBuilderContext(self->file_builder);

  // TODO(haberman): remove this restriction, maps are supported in proto2.
  if (upb_strview_eql(
          google_protobuf_FileDescriptorProto_syntax(file_builder->file_proto),
          upb_strview_makez("proto2"))) {
    rb_raise(rb_eArgError,
             "Cannot add a native map field using proto2 syntax.");
  }

  // Create a new message descriptor for the map entry message, and create a
  // repeated submessage field here with that type.
  msg_name = google_protobuf_DescriptorProto_name(self->msg_proto);
  mapentry_desc_name = rb_str_new(msg_name.data, msg_name.size);
  mapentry_desc_name = rb_str_cat2(mapentry_desc_name, "_MapEntry_");
  mapentry_desc_name =
      rb_str_cat2(mapentry_desc_name, rb_id2name(SYM2ID(name)));

  {
    // message <msgname>_MapEntry_ { /* ... */ }
    VALUE args[1] = {mapentry_desc_name};
    VALUE types = rb_ary_new3(3, key_type, value_type, type_class);
    rb_block_call(self->file_builder, rb_intern("add_message"), 1, args,
                  make_mapentry, types);
  }

  // If this file is in a package, we need to qualify the map entry type.
  if (google_protobuf_FileDescriptorProto_has_package(file_builder->file_proto)) {
    upb_strview package_view =
        google_protobuf_FileDescriptorProto_package(file_builder->file_proto);
    VALUE package = rb_str_new(package_view.data, package_view.size);
    package = rb_str_cat2(package, ".");
    mapentry_desc_name = rb_str_concat(package, mapentry_desc_name);
  }

  // repeated MapEntry <name> = <number>;
  rb_funcall(_self, rb_intern("repeated"), 4, name,
             ID2SYM(rb_intern("message")), number, mapentry_desc_name);

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
  size_t oneof_count;
  FileBuilderContext* file_context =
      ruby_to_FileBuilderContext(self->file_builder);
  google_protobuf_OneofDescriptorProto* oneof_proto;

  // Existing oneof_count becomes oneof_index.
  google_protobuf_DescriptorProto_oneof_decl(self->msg_proto, &oneof_count);

  // Create oneof_proto and set its name.
  oneof_proto = google_protobuf_DescriptorProto_add_oneof_decl(
      self->msg_proto, file_context->arena);
  google_protobuf_OneofDescriptorProto_set_name(
      oneof_proto, FileBuilderContext_strdup_sym(self->file_builder, name));

  // Evaluate the block with the builder as argument.
  {
    VALUE args[2] = { INT2NUM(oneof_count), _self };
    VALUE ctx = rb_class_new_instance(2, args, cOneofBuilderContext);
    VALUE block = rb_block_proc();
    rb_funcall_with_block(ctx, rb_intern("instance_eval"), 0, NULL, block);
  }

  return Qnil;
}

// -----------------------------------------------------------------------------
// OneofBuilderContext.
// -----------------------------------------------------------------------------

DEFINE_CLASS(OneofBuilderContext,
    "Google::Protobuf::Internal::OneofBuilderContext");

void OneofBuilderContext_mark(void* _self) {
  OneofBuilderContext* self = _self;
  rb_gc_mark(self->message_builder);
}

void OneofBuilderContext_free(void* _self) {
  xfree(_self);
}

VALUE OneofBuilderContext_alloc(VALUE klass) {
  OneofBuilderContext* self = ALLOC(OneofBuilderContext);
  VALUE ret = TypedData_Wrap_Struct(
      klass, &_OneofBuilderContext_type, self);
  self->oneof_index = 0;
  self->message_builder = Qnil;
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
 *     OneofBuilderContext.new(oneof_index, message_builder) => context
 *
 * Create a new oneof builder context around the given oneof descriptor and
 * builder context. This class is intended to serve as a DSL context to be used
 * with #instance_eval.
 */
VALUE OneofBuilderContext_initialize(VALUE _self,
                                     VALUE oneof_index,
                                     VALUE message_builder) {
  DEFINE_SELF(OneofBuilderContext, self, _self);
  self->oneof_index = NUM2INT(oneof_index);
  self->message_builder = message_builder;
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

  msgdef_add_field(self->message_builder, UPB_LABEL_OPTIONAL, name, type,
                   number, type_class, options, self->oneof_index);

  return Qnil;
}

// -----------------------------------------------------------------------------
// EnumBuilderContext.
// -----------------------------------------------------------------------------

DEFINE_CLASS(EnumBuilderContext,
    "Google::Protobuf::Internal::EnumBuilderContext");

void EnumBuilderContext_mark(void* _self) {
  EnumBuilderContext* self = _self;
  rb_gc_mark(self->file_builder);
}

void EnumBuilderContext_free(void* _self) {
  xfree(_self);
}

VALUE EnumBuilderContext_alloc(VALUE klass) {
  EnumBuilderContext* self = ALLOC(EnumBuilderContext);
  VALUE ret = TypedData_Wrap_Struct(
      klass, &_EnumBuilderContext_type, self);
  self->enum_proto = NULL;
  self->file_builder = Qnil;
  return ret;
}

void EnumBuilderContext_register(VALUE module) {
  VALUE klass = rb_define_class_under(
      module, "EnumBuilderContext", rb_cObject);
  rb_define_alloc_func(klass, EnumBuilderContext_alloc);
  rb_define_method(klass, "initialize", EnumBuilderContext_initialize, 2);
  rb_define_method(klass, "value", EnumBuilderContext_value, 2);
  rb_gc_register_address(&cEnumBuilderContext);
  cEnumBuilderContext = klass;
}

/*
 * call-seq:
 *     EnumBuilderContext.new(file_builder) => context
 *
 * Create a new builder context around the given enum descriptor. This class is
 * intended to serve as a DSL context to be used with #instance_eval.
 */
VALUE EnumBuilderContext_initialize(VALUE _self, VALUE _file_builder,
                                    VALUE name) {
  DEFINE_SELF(EnumBuilderContext, self, _self);
  FileBuilderContext* file_builder = ruby_to_FileBuilderContext(_file_builder);
  google_protobuf_FileDescriptorProto* file_proto = file_builder->file_proto;

  self->file_builder = _file_builder;
  self->enum_proto = google_protobuf_FileDescriptorProto_add_enum_type(
      file_proto, file_builder->arena);

  google_protobuf_EnumDescriptorProto_set_name(
      self->enum_proto, FileBuilderContext_strdup(_file_builder, name));

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
  FileBuilderContext* file_builder =
      ruby_to_FileBuilderContext(self->file_builder);
  google_protobuf_EnumValueDescriptorProto* enum_value;

  enum_value = google_protobuf_EnumDescriptorProto_add_value(
      self->enum_proto, file_builder->arena);

  google_protobuf_EnumValueDescriptorProto_set_name(
      enum_value, FileBuilderContext_strdup_sym(self->file_builder, name));
  google_protobuf_EnumValueDescriptorProto_set_number(enum_value,
                                                      NUM2INT(number));

  return Qnil;
}


// -----------------------------------------------------------------------------
// FileBuilderContext.
// -----------------------------------------------------------------------------

DEFINE_CLASS(FileBuilderContext,
             "Google::Protobuf::Internal::FileBuilderContext");

void FileBuilderContext_mark(void* _self) {
  FileBuilderContext* self = _self;
  rb_gc_mark(self->descriptor_pool);
}

void FileBuilderContext_free(void* _self) {
  FileBuilderContext* self = _self;
  upb_arena_free(self->arena);
  xfree(self);
}

upb_strview FileBuilderContext_strdup2(VALUE _self, const char *str) {
  DEFINE_SELF(FileBuilderContext, self, _self);
  upb_strview ret;
  char *data;

  ret.size = strlen(str);
  data = upb_malloc(upb_arena_alloc(self->arena), ret.size + 1);
  ret.data = data;
  memcpy(data, str, ret.size);
  /* Null-terminate required by rewrite_enum_defaults() above. */
  data[ret.size] = '\0';
  return ret;
}

upb_strview FileBuilderContext_strdup(VALUE _self, VALUE rb_str) {
  return FileBuilderContext_strdup2(_self, get_str(rb_str));
}

upb_strview FileBuilderContext_strdup_sym(VALUE _self, VALUE rb_sym) {
  Check_Type(rb_sym, T_SYMBOL);
  return FileBuilderContext_strdup(_self, rb_id2str(SYM2ID(rb_sym)));
}

VALUE FileBuilderContext_alloc(VALUE klass) {
  FileBuilderContext* self = ALLOC(FileBuilderContext);
  VALUE ret = TypedData_Wrap_Struct(klass, &_FileBuilderContext_type, self);
  self->arena = upb_arena_new();
  self->file_proto = google_protobuf_FileDescriptorProto_new(self->arena);
  self->descriptor_pool = Qnil;
  return ret;
}

void FileBuilderContext_register(VALUE module) {
  VALUE klass = rb_define_class_under(module, "FileBuilderContext", rb_cObject);
  rb_define_alloc_func(klass, FileBuilderContext_alloc);
  rb_define_method(klass, "initialize", FileBuilderContext_initialize, 3);
  rb_define_method(klass, "add_message", FileBuilderContext_add_message, 1);
  rb_define_method(klass, "add_enum", FileBuilderContext_add_enum, 1);
  rb_gc_register_address(&cFileBuilderContext);
  cFileBuilderContext = klass;
}

/*
 * call-seq:
 *     FileBuilderContext.new(descriptor_pool) => context
 *
 * Create a new file builder context for the given file descriptor and
 * builder context. This class is intended to serve as a DSL context to be used
 * with #instance_eval.
 */
VALUE FileBuilderContext_initialize(VALUE _self, VALUE descriptor_pool,
                                    VALUE name, VALUE options) {
  DEFINE_SELF(FileBuilderContext, self, _self);
  self->descriptor_pool = descriptor_pool;

  google_protobuf_FileDescriptorProto_set_name(
      self->file_proto, FileBuilderContext_strdup(_self, name));

  // Default syntax for Ruby is proto3.
  google_protobuf_FileDescriptorProto_set_syntax(
      self->file_proto,
      FileBuilderContext_strdup(_self, rb_str_new2("proto3")));

  if (options != Qnil) {
    VALUE syntax;

    Check_Type(options, T_HASH);
    syntax = rb_hash_lookup2(options, ID2SYM(rb_intern("syntax")), Qnil);

    if (syntax != Qnil) {
      VALUE syntax_str;

      Check_Type(syntax, T_SYMBOL);
      syntax_str = rb_id2str(SYM2ID(syntax));
      google_protobuf_FileDescriptorProto_set_syntax(
          self->file_proto, FileBuilderContext_strdup(_self, syntax_str));
    }
  }

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
  VALUE args[2] = { _self, name };
  VALUE ctx = rb_class_new_instance(2, args, cMessageBuilderContext);
  VALUE block = rb_block_proc();
  rb_funcall_with_block(ctx, rb_intern("instance_eval"), 0, NULL, block);
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
  VALUE args[2] = { _self, name };
  VALUE ctx = rb_class_new_instance(2, args, cEnumBuilderContext);
  VALUE block = rb_block_proc();
  rb_funcall_with_block(ctx, rb_intern("instance_eval"), 0, NULL, block);
  return Qnil;
}

void FileBuilderContext_build(VALUE _self) {
  DEFINE_SELF(FileBuilderContext, self, _self);
  DescriptorPool* pool = ruby_to_DescriptorPool(self->descriptor_pool);
  upb_status status;

  rewrite_enum_defaults(pool->symtab, self->file_proto);
  rewrite_names(_self, self->file_proto);

  upb_status_clear(&status);
  if (!upb_symtab_addfile(pool->symtab, self->file_proto, &status)) {
    rb_raise(cTypeError, "Unable to add defs to DescriptorPool: %s",
             upb_status_errmsg(&status));
  }
}

// -----------------------------------------------------------------------------
// Builder.
// -----------------------------------------------------------------------------

DEFINE_CLASS(Builder, "Google::Protobuf::Internal::Builder");

void Builder_mark(void* _self) {
  Builder* self = _self;
  rb_gc_mark(self->descriptor_pool);
  rb_gc_mark(self->default_file_builder);
}

void Builder_free(void* _self) {
  xfree(_self);
}

VALUE Builder_alloc(VALUE klass) {
  Builder* self = ALLOC(Builder);
  VALUE ret = TypedData_Wrap_Struct(
      klass, &_Builder_type, self);
  self->descriptor_pool = Qnil;
  self->default_file_builder = Qnil;
  return ret;
}

void Builder_register(VALUE module) {
  VALUE klass = rb_define_class_under(module, "Builder", rb_cObject);
  rb_define_alloc_func(klass, Builder_alloc); 
  rb_define_method(klass, "initialize", Builder_initialize, 1);
  rb_define_method(klass, "add_file", Builder_add_file, -1);
  rb_define_method(klass, "add_message", Builder_add_message, 1);
  rb_define_method(klass, "add_enum", Builder_add_enum, 1);
  rb_gc_register_address(&cBuilder);
  cBuilder = klass;
}

/*
 * call-seq:
 *     Builder.new(descriptor_pool) => builder
 *
 * Creates a new Builder. A Builder can accumulate a set of new message and enum
 * descriptors and atomically register them into a pool in a way that allows for
 * (co)recursive type references.
 */
VALUE Builder_initialize(VALUE _self, VALUE pool) {
  DEFINE_SELF(Builder, self, _self);
  self->descriptor_pool = pool;
  self->default_file_builder = Qnil;  // Created lazily if needed.
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
  VALUE name, options;
  VALUE ctx;
  VALUE block;

  rb_scan_args(argc, argv, "11", &name, &options);

  {
    VALUE args[3] = { self->descriptor_pool, name, options };
    ctx = rb_class_new_instance(3, args, cFileBuilderContext);
  }

  block = rb_block_proc();
  rb_funcall_with_block(ctx, rb_intern("instance_eval"), 0, NULL, block);
  FileBuilderContext_build(ctx);

  return Qnil;
}

static VALUE Builder_get_default_file(VALUE _self) {
  DEFINE_SELF(Builder, self, _self);

  /* Lazily create only if legacy builder-level methods are called. */
  if (self->default_file_builder == Qnil) {
    VALUE name = rb_str_new2("ruby_default_file.proto");
    VALUE args [3] = { self->descriptor_pool, name, rb_hash_new() };
    self->default_file_builder =
        rb_class_new_instance(3, args, cFileBuilderContext);
  }

  return self->default_file_builder;
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
  VALUE file_builder = Builder_get_default_file(_self);
  rb_funcall_with_block(file_builder, rb_intern("add_message"), 1, &name,
                        rb_block_proc());
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
  VALUE file_builder = Builder_get_default_file(_self);
  rb_funcall_with_block(file_builder, rb_intern("add_enum"), 1, &name,
                        rb_block_proc());
  return Qnil;
}

/* This method is hidden from Ruby, and only called directly from
 * DescriptorPool_build(). */
VALUE Builder_build(VALUE _self) {
  DEFINE_SELF(Builder, self, _self);

  if (self->default_file_builder != Qnil) {
    FileBuilderContext_build(self->default_file_builder);
    self->default_file_builder = Qnil;
  }

  return Qnil;
}

static VALUE get_def_obj(VALUE _descriptor_pool, const void* ptr, VALUE klass) {
  DEFINE_SELF(DescriptorPool, descriptor_pool, _descriptor_pool);
  VALUE key = ULL2NUM((intptr_t)ptr);
  VALUE def;

  def = rb_hash_aref(descriptor_pool->def_to_descriptor, key);

  if (ptr == NULL) {
    return Qnil;
  }

  if (def == Qnil) {
    // Lazily create wrapper object.
    VALUE args[3] = { c_only_cookie, _descriptor_pool, key };
    def = rb_class_new_instance(3, args, klass);
    rb_hash_aset(descriptor_pool->def_to_descriptor, key, def);

    // For message defs, we now eagerly get/create descriptors for all
    // submessages.  We will need these anyway to parse or serialize this
    // message type.  But more importantly, we must do this now so that
    // add_handlers_for_message() (which calls get_msgdef_obj()) does *not*
    // need to create a Ruby object or insert into a Ruby Hash.  We need to
    // avoid triggering GC, which can switch Ruby threads and re-enter our
    // C extension from a different thread.  This wreaks havoc on our state
    // if we were in the middle of building handlers.
    if (klass == cDescriptor) {
      const upb_msgdef *m = ptr;
      upb_msg_field_iter it;
      for (upb_msg_field_begin(&it, m);
           !upb_msg_field_done(&it);
           upb_msg_field_next(&it)) {
        const upb_fielddef* f = upb_msg_iter_field(&it);
        if (upb_fielddef_issubmsg(f)) {
          get_msgdef_obj(_descriptor_pool, upb_fielddef_msgsubdef(f));
        }
      }
    }
  }

  return def;
}

VALUE get_msgdef_obj(VALUE descriptor_pool, const upb_msgdef* def) {
  return get_def_obj(descriptor_pool, def, cDescriptor);
}

VALUE get_enumdef_obj(VALUE descriptor_pool, const upb_enumdef* def) {
  return get_def_obj(descriptor_pool, def, cEnumDescriptor);
}

VALUE get_fielddef_obj(VALUE descriptor_pool, const upb_fielddef* def) {
  return get_def_obj(descriptor_pool, def, cFieldDescriptor);
}

VALUE get_filedef_obj(VALUE descriptor_pool, const upb_filedef* def) {
  return get_def_obj(descriptor_pool, def, cFileDescriptor);
}

VALUE get_oneofdef_obj(VALUE descriptor_pool, const upb_oneofdef* def) {
  return get_def_obj(descriptor_pool, def, cOneofDescriptor);
}
