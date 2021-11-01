/*
 * Copyright (c) 2009-2021, Google LLC
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of Google LLC nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL Google LLC BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "upb/util/def_to_proto.h"

#include <inttypes.h>
#include <setjmp.h>
#include <stdio.h>

#include "upb/reflection.h"

/* Must be last. */
#include "upb/port_def.inc"

typedef struct {
  upb_arena *arena;
  jmp_buf err;
} upb_ToProto_Context;

#define CHK_OOM(val) \
  if (!(val)) UPB_LONGJMP(ctx->err, 1);

// We want to copy the options verbatim into the destination options proto.
// We use serialize+parse as our deep copy.
#define SET_OPTIONS(proto, desc_type, options_type, src)                    \
  {                                                                         \
    size_t size;                                                            \
    /* MEM: could use a temporary arena here instead. */                    \
    char *pb =                                                              \
        google_protobuf_##options_type##_serialize(src, ctx->arena, &size); \
    CHK_OOM(pb);                                                            \
    google_protobuf_##options_type *dst =                                   \
        google_protobuf_##options_type##_parse(pb, size, ctx->arena);       \
    CHK_OOM(dst);                                                           \
    google_protobuf_##desc_type##_set_options(proto, dst);                  \
  }

static upb_strview strviewdup2(upb_ToProto_Context *ctx, upb_strview str) {
  char *p = upb_arena_malloc(ctx->arena, str.size);
  CHK_OOM(p);
  memcpy(p, str.data, str.size);
  return (upb_strview){.data = p, .size = str.size};
}

static upb_strview strviewdup(upb_ToProto_Context *ctx, const char *s) {
  return strviewdup2(ctx, (upb_strview){.data = s, .size = strlen(s)});
}

static upb_strview qual_dup(upb_ToProto_Context *ctx, const char *s) {
  size_t n = strlen(s);
  char *p = upb_arena_malloc(ctx->arena, n + 1);
  CHK_OOM(p);
  p[0] = '.';
  memcpy(p + 1, s, n);
  return (upb_strview){.data = p, .size = n + 1};
}

UPB_PRINTF(2, 3)
static upb_strview printf_dup(upb_ToProto_Context *ctx, const char *fmt, ...) {
  const size_t max = 32;
  char *p = upb_arena_malloc(ctx->arena, max);
  CHK_OOM(p);
  va_list args;
  va_start(args, fmt);
  size_t n = vsnprintf(p, max, fmt, args);
  va_end(args);
  UPB_ASSERT(n < max);
  return (upb_strview){.data = p, .size = n};
}

static upb_strview default_string(upb_ToProto_Context *ctx,
                                  const upb_fielddef *f) {
  upb_msgval d = upb_fielddef_default(f);
  switch (upb_fielddef_type(f)) {
    case UPB_TYPE_BOOL:
      return strviewdup(ctx, d.bool_val ? "true" : "false");
    case UPB_TYPE_ENUM: {
      const upb_enumdef *e = upb_fielddef_enumsubdef(f);
      const upb_enumvaldef *ev = upb_enumdef_lookupnum(e, d.int32_val);
      return strviewdup(ctx, upb_enumvaldef_name(ev));
    }
    case UPB_TYPE_INT64:
      return printf_dup(ctx, "%" PRId64, d.int64_val);
    case UPB_TYPE_UINT64:
      return printf_dup(ctx, "%" PRIu64, d.uint64_val);
    case UPB_TYPE_INT32:
      return printf_dup(ctx, "%" PRId32, d.int32_val);
    case UPB_TYPE_UINT32:
      return printf_dup(ctx, "%" PRIu32, d.uint32_val);
    case UPB_TYPE_FLOAT:
      return printf_dup(ctx, "%.9g", d.float_val);
    case UPB_TYPE_DOUBLE:
      return printf_dup(ctx, "%.17g", d.double_val);
    case UPB_TYPE_STRING:
      return strviewdup2(ctx, d.str_val);
    case UPB_TYPE_BYTES:
      return strviewdup2(ctx, d.str_val);  // XXX C-escape
    default:
      UPB_UNREACHABLE();
  }
}

static google_protobuf_FieldDescriptorProto *fielddef_toproto(
    upb_ToProto_Context *ctx, const upb_fielddef *f) {
  google_protobuf_FieldDescriptorProto *proto =
      google_protobuf_FieldDescriptorProto_new(ctx->arena);
  CHK_OOM(proto);

  google_protobuf_FieldDescriptorProto_set_name(
      proto, strviewdup(ctx, upb_fielddef_name(f)));
  google_protobuf_FieldDescriptorProto_set_number(proto,
                                                  upb_fielddef_number(f));
  google_protobuf_FieldDescriptorProto_set_label(proto, upb_fielddef_label(f));
  google_protobuf_FieldDescriptorProto_set_type(proto,
                                                upb_fielddef_descriptortype(f));

  if (upb_fielddef_hasjsonname(f)) {
    google_protobuf_FieldDescriptorProto_set_json_name(
        proto, strviewdup(ctx, upb_fielddef_jsonname(f)));
  }

  if (upb_fielddef_issubmsg(f)) {
    google_protobuf_FieldDescriptorProto_set_type_name(
        proto, qual_dup(ctx, upb_msgdef_fullname(upb_fielddef_msgsubdef(f))));
  } else if (upb_fielddef_type(f) == UPB_TYPE_ENUM) {
    google_protobuf_FieldDescriptorProto_set_type_name(
        proto, qual_dup(ctx, upb_enumdef_fullname(upb_fielddef_enumsubdef(f))));
  }

  if (upb_fielddef_isextension(f)) {
    google_protobuf_FieldDescriptorProto_set_extendee(
        proto,
        qual_dup(ctx, upb_msgdef_fullname(upb_fielddef_containingtype(f))));
  }

  if (upb_fielddef_hasdefault(f)) {
    google_protobuf_FieldDescriptorProto_set_default_value(
        proto, default_string(ctx, f));
  }

  const upb_oneofdef *o = upb_fielddef_containingoneof(f);
  if (o) {
    google_protobuf_FieldDescriptorProto_set_oneof_index(proto,
                                                         upb_oneofdef_index(o));
  }

  if (_upb_fielddef_proto3optional(f)) {
    google_protobuf_FieldDescriptorProto_set_proto3_optional(proto, true);
  }

  if (upb_fielddef_hasoptions(f)) {
    SET_OPTIONS(proto, FieldDescriptorProto, FieldOptions,
                upb_fielddef_options(f));
  }

  return proto;
}

static google_protobuf_OneofDescriptorProto *oneofdef_toproto(
    upb_ToProto_Context *ctx, const upb_oneofdef *o) {
  google_protobuf_OneofDescriptorProto *proto =
      google_protobuf_OneofDescriptorProto_new(ctx->arena);
  CHK_OOM(proto);

  google_protobuf_OneofDescriptorProto_set_name(
      proto, strviewdup(ctx, upb_oneofdef_name(o)));

  if (upb_oneofdef_hasoptions(o)) {
    SET_OPTIONS(proto, OneofDescriptorProto, OneofOptions,
                upb_oneofdef_options(o));
  }

  return proto;
}

static google_protobuf_EnumValueDescriptorProto *enumvaldef_toproto(
    upb_ToProto_Context *ctx, const upb_enumvaldef *e) {
  google_protobuf_EnumValueDescriptorProto *proto =
      google_protobuf_EnumValueDescriptorProto_new(ctx->arena);
  CHK_OOM(proto);

  google_protobuf_EnumValueDescriptorProto_set_name(
      proto, strviewdup(ctx, upb_enumvaldef_name(e)));
  google_protobuf_EnumValueDescriptorProto_set_number(proto,
                                                      upb_enumvaldef_number(e));

  if (upb_enumvaldef_hasoptions(e)) {
    SET_OPTIONS(proto, EnumValueDescriptorProto, EnumValueOptions,
                upb_enumvaldef_options(e));
  }

  return proto;
}

static google_protobuf_EnumDescriptorProto *enumdef_toproto(
    upb_ToProto_Context *ctx, const upb_enumdef *e) {
  google_protobuf_EnumDescriptorProto *proto =
      google_protobuf_EnumDescriptorProto_new(ctx->arena);
  CHK_OOM(proto);

  google_protobuf_EnumDescriptorProto_set_name(
      proto, strviewdup(ctx, upb_enumdef_name(e)));

  int n;

  n = upb_enumdef_valuecount(e);
  google_protobuf_EnumValueDescriptorProto **vals =
      google_protobuf_EnumDescriptorProto_resize_value(proto, n, ctx->arena);
  CHK_OOM(vals);
  for (int i = 0; i < n; i++) {
    vals[i] = enumvaldef_toproto(ctx, upb_enumdef_value(e, i));
  }

  // TODO: reserved range, reserved name

  if (upb_enumdef_hasoptions(e)) {
    SET_OPTIONS(proto, EnumDescriptorProto, EnumOptions,
                upb_enumdef_options(e));
  }

  return proto;
}

static google_protobuf_DescriptorProto_ExtensionRange *extrange_toproto(
    upb_ToProto_Context *ctx, const upb_extrange *e) {
  google_protobuf_DescriptorProto_ExtensionRange *proto =
      google_protobuf_DescriptorProto_ExtensionRange_new(ctx->arena);
  CHK_OOM(proto);

  google_protobuf_DescriptorProto_ExtensionRange_set_start(
      proto, upb_extrange_start(e));
  google_protobuf_DescriptorProto_ExtensionRange_set_end(proto,
                                                         upb_extrange_end(e));

  if (upb_extrange_hasoptions(e)) {
    SET_OPTIONS(proto, DescriptorProto_ExtensionRange, ExtensionRangeOptions,
                upb_extrange_options(e));
  }

  return proto;
}

static google_protobuf_DescriptorProto *msgdef_toproto(upb_ToProto_Context *ctx,
                                                       const upb_msgdef *m) {
  google_protobuf_DescriptorProto *proto =
      google_protobuf_DescriptorProto_new(ctx->arena);
  CHK_OOM(proto);

  google_protobuf_DescriptorProto_set_name(proto,
                                           strviewdup(ctx, upb_msgdef_name(m)));

  int n;

  n = upb_msgdef_fieldcount(m);
  google_protobuf_FieldDescriptorProto **fields =
      google_protobuf_DescriptorProto_resize_field(proto, n, ctx->arena);
  CHK_OOM(fields);
  for (int i = 0; i < n; i++) {
    fields[i] = fielddef_toproto(ctx, upb_msgdef_field(m, i));
  }

  n = upb_msgdef_oneofcount(m);
  google_protobuf_OneofDescriptorProto **oneofs =
      google_protobuf_DescriptorProto_resize_oneof_decl(proto, n, ctx->arena);
  for (int i = 0; i < n; i++) {
    oneofs[i] = oneofdef_toproto(ctx, upb_msgdef_oneof(m, i));
  }

  n = upb_msgdef_nestedmsgcount(m);
  google_protobuf_DescriptorProto **nested_msgs =
      google_protobuf_DescriptorProto_resize_nested_type(proto, n, ctx->arena);
  for (int i = 0; i < n; i++) {
    nested_msgs[i] = msgdef_toproto(ctx, upb_msgdef_nestedmsg(m, i));
  }

  n = upb_msgdef_nestedenumcount(m);
  google_protobuf_EnumDescriptorProto **nested_enums =
      google_protobuf_DescriptorProto_resize_enum_type(proto, n, ctx->arena);
  for (int i = 0; i < n; i++) {
    nested_enums[i] = enumdef_toproto(ctx, upb_msgdef_nestedenum(m, i));
  }

  n = upb_msgdef_nestedextcount(m);
  google_protobuf_FieldDescriptorProto **nested_exts =
      google_protobuf_DescriptorProto_resize_extension(proto, n, ctx->arena);
  for (int i = 0; i < n; i++) {
    nested_exts[i] = fielddef_toproto(ctx, upb_msgdef_nestedext(m, i));
  }

  n = upb_msgdef_extrangecount(m);
  google_protobuf_DescriptorProto_ExtensionRange **ext_ranges =
      google_protobuf_DescriptorProto_resize_extension_range(proto, n,
                                                             ctx->arena);
  for (int i = 0; i < n; i++) {
    ext_ranges[i] = extrange_toproto(ctx, upb_msgdef_extrange(m, i));
  }

  // TODO: reserved ranges and reserved names

  if (upb_msgdef_hasoptions(m)) {
    SET_OPTIONS(proto, DescriptorProto, MessageOptions, upb_msgdef_options(m));
  }

  return proto;
}

static google_protobuf_MethodDescriptorProto *methoddef_toproto(
    upb_ToProto_Context *ctx, const upb_methoddef *m) {
  google_protobuf_MethodDescriptorProto *proto =
      google_protobuf_MethodDescriptorProto_new(ctx->arena);
  CHK_OOM(proto);

  google_protobuf_MethodDescriptorProto_set_name(
      proto, strviewdup(ctx, upb_methoddef_name(m)));

  google_protobuf_MethodDescriptorProto_set_input_type(
      proto, qual_dup(ctx, upb_msgdef_fullname(upb_methoddef_inputtype(m))));
  google_protobuf_MethodDescriptorProto_set_output_type(
      proto, qual_dup(ctx, upb_msgdef_fullname(upb_methoddef_outputtype(m))));

  if (upb_methoddef_clientstreaming(m)) {
    google_protobuf_MethodDescriptorProto_set_client_streaming(proto, true);
  }

  if (upb_methoddef_serverstreaming(m)) {
    google_protobuf_MethodDescriptorProto_set_server_streaming(proto, true);
  }

  if (upb_methoddef_hasoptions(m)) {
    SET_OPTIONS(proto, MethodDescriptorProto, MethodOptions,
                upb_methoddef_options(m));
  }

  return proto;
}

static google_protobuf_ServiceDescriptorProto *servicedef_toproto(
    upb_ToProto_Context *ctx, const upb_servicedef *s) {
  google_protobuf_ServiceDescriptorProto *proto =
      google_protobuf_ServiceDescriptorProto_new(ctx->arena);
  CHK_OOM(proto);

  google_protobuf_ServiceDescriptorProto_set_name(
      proto, strviewdup(ctx, upb_servicedef_name(s)));

  size_t n = upb_servicedef_methodcount(s);
  google_protobuf_MethodDescriptorProto **methods =
      google_protobuf_ServiceDescriptorProto_resize_method(proto, n,
                                                           ctx->arena);
  for (int i = 0; i < n; i++) {
    methods[i] = methoddef_toproto(ctx, upb_servicedef_method(s, i));
  }

  if (upb_servicedef_hasoptions(s)) {
    SET_OPTIONS(proto, ServiceDescriptorProto, ServiceOptions,
                upb_servicedef_options(s));
  }

  return proto;
}

static google_protobuf_FileDescriptorProto *filedef_toproto(
    upb_ToProto_Context *ctx, const upb_filedef *f) {
  google_protobuf_FileDescriptorProto *proto =
      google_protobuf_FileDescriptorProto_new(ctx->arena);
  CHK_OOM(proto);

  google_protobuf_FileDescriptorProto_set_name(
      proto, strviewdup(ctx, upb_filedef_name(f)));

  size_t n = strlen(upb_filedef_package(f));
  if (n) {
    google_protobuf_FileDescriptorProto_set_package(
        proto, strviewdup(ctx, upb_filedef_package(f)));
  }

  if (upb_filedef_syntax(f) == UPB_SYNTAX_PROTO3) {
    google_protobuf_FileDescriptorProto_set_syntax(proto,
                                                   strviewdup(ctx, "proto3"));
  }

  n = upb_filedef_depcount(f);
  upb_strview *deps = google_protobuf_FileDescriptorProto_resize_dependency(
      proto, n, ctx->arena);
  for (int i = 0; i < n; i++) {
    deps[i] = strviewdup(ctx, upb_filedef_name(upb_filedef_dep(f, i)));
  }

  n = upb_filedef_publicdepcount(f);
  int32_t *public_deps =
      google_protobuf_FileDescriptorProto_resize_public_dependency(proto, n,
                                                                   ctx->arena);
  const int32_t *public_dep_nums = _upb_filedef_publicdepnums(f);
  memcpy(public_deps, public_dep_nums, n * sizeof(int32_t));

  n = upb_filedef_weakdepcount(f);
  int32_t *weak_deps =
      google_protobuf_FileDescriptorProto_resize_weak_dependency(proto, n,
                                                                 ctx->arena);
  const int32_t *weak_dep_nums = _upb_filedef_weakdepnums(f);
  memcpy(weak_deps, weak_dep_nums, n * sizeof(int32_t));

  n = upb_filedef_toplvlmsgcount(f);
  google_protobuf_DescriptorProto **msgs =
      google_protobuf_FileDescriptorProto_resize_message_type(proto, n,
                                                              ctx->arena);
  for (int i = 0; i < n; i++) {
    msgs[i] = msgdef_toproto(ctx, upb_filedef_toplvlmsg(f, i));
  }

  n = upb_filedef_toplvlenumcount(f);
  google_protobuf_EnumDescriptorProto **enums =
      google_protobuf_FileDescriptorProto_resize_enum_type(proto, n,
                                                           ctx->arena);
  for (int i = 0; i < n; i++) {
    enums[i] = enumdef_toproto(ctx, upb_filedef_toplvlenum(f, i));
  }

  n = upb_filedef_servicecount(f);
  google_protobuf_ServiceDescriptorProto **services =
      google_protobuf_FileDescriptorProto_resize_service(proto, n, ctx->arena);
  for (int i = 0; i < n; i++) {
    services[i] = servicedef_toproto(ctx, upb_filedef_service(f, i));
  }

  n = upb_filedef_toplvlextcount(f);
  google_protobuf_FieldDescriptorProto **exts =
      google_protobuf_FileDescriptorProto_resize_extension(proto, n,
                                                           ctx->arena);
  for (int i = 0; i < n; i++) {
    exts[i] = fielddef_toproto(ctx, upb_filedef_toplvlext(f, i));
  }

  if (upb_filedef_hasoptions(f)) {
    SET_OPTIONS(proto, FileDescriptorProto, FileOptions,
                upb_filedef_options(f));
  }

  return proto;
}

google_protobuf_DescriptorProto *upb_MessageDef_ToProto(const upb_msgdef *m,
                                                        upb_arena *a) {
  upb_ToProto_Context ctx = {a};
  if (UPB_SETJMP(ctx.err)) return NULL;
  return msgdef_toproto(&ctx, m);
}

google_protobuf_EnumDescriptorProto *upb_EnumDef_ToProto(const upb_enumdef *e,
                                                         upb_arena *a) {
  upb_ToProto_Context ctx = {a};
  if (UPB_SETJMP(ctx.err)) return NULL;
  return enumdef_toproto(&ctx, e);
}

google_protobuf_EnumValueDescriptorProto *upb_EnumValueDef_ToProto(
    const upb_enumvaldef *e, upb_arena *a) {
  upb_ToProto_Context ctx = {a};
  if (UPB_SETJMP(ctx.err)) return NULL;
  return enumvaldef_toproto(&ctx, e);
}

google_protobuf_FieldDescriptorProto *upb_FieldDef_ToProto(
    const upb_fielddef *f, upb_arena *a) {
  upb_ToProto_Context ctx = {a};
  if (UPB_SETJMP(ctx.err)) return NULL;
  return fielddef_toproto(&ctx, f);
}

google_protobuf_OneofDescriptorProto *upb_OneofDef_ToProto(
    const upb_oneofdef *o, upb_arena *a) {
  upb_ToProto_Context ctx = {a};
  if (UPB_SETJMP(ctx.err)) return NULL;
  return oneofdef_toproto(&ctx, o);
}

google_protobuf_FileDescriptorProto *upb_FileDef_ToProto(const upb_filedef *f,
                                                         upb_arena *a) {
  upb_ToProto_Context ctx = {a};
  if (UPB_SETJMP(ctx.err)) return NULL;
  return filedef_toproto(&ctx, f);
}

google_protobuf_MethodDescriptorProto *upb_MethodDef_ToProto(
    const upb_methoddef *m, upb_arena *a) {
  upb_ToProto_Context ctx = {a};
  if (UPB_SETJMP(ctx.err)) return NULL;
  return methoddef_toproto(&ctx, m);
}

google_protobuf_ServiceDescriptorProto *upb_ServiceDef_ToProto(
    const upb_servicedef *s, upb_arena *a) {
  upb_ToProto_Context ctx = {a};
  if (UPB_SETJMP(ctx.err)) return NULL;
  return servicedef_toproto(&ctx, s);
}
