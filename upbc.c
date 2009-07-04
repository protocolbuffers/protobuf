/*
 * upb - a minimalist implementation of protocol buffers.
 *
 * upbc is the upb compiler.
 *
 * Copyright (c) 2009 Joshua Haberman.  See LICENSE for details.
 */

#include <ctype.h>
#include <inttypes.h>
#include "descriptor.h"
#include "upb_context.h"

/* These are in-place string transformations that do not change the length of
 * the string (and thus never need to re-allocate). */
static void to_cident(struct upb_string str)
{
  for(uint32_t i = 0; i < str.byte_len; i++)
    if(str.ptr[i] == '.' || str.ptr[i] == '/')
      str.ptr[i] = '_';
}

static void to_preproc(struct upb_string str)
{
  to_cident(str);
  for(uint32_t i = 0; i < str.byte_len; i++)
    str.ptr[i] = toupper(str.ptr[i]);
}

/* The .h file defines structs for the types defined in the .proto file.  It
 * also defines constants for the enum values.
 *
 * Assumes that d has been validated. */
static void write_header(google_protobuf_FileDescriptorProto *d, FILE *stream)
{
  /* Header file prologue. */
  fprintf(stream, "/* Auto-generated from " UPB_STRFMT ".  Do not edit. */\n\n",
          UPB_STRARG(*d->name));
  struct upb_string include_guard_name = upb_strdup(*d->name);
  to_preproc(include_guard_name);
  fprintf(stream, "#ifndef " UPB_STRFMT "\n", UPB_STRARG(include_guard_name));
  fprintf(stream, "#define " UPB_STRFMT "\n\n", UPB_STRARG(include_guard_name));
  fputs("#include <upb_msg.h>\n\n", stream);
  fputs("#ifdef __cplusplus\n", stream);
  fputs("extern \"C\" {\n", stream);
  fputs("#endif\n\n", stream);

  /* Enums. */
  if(d->set_flags.has.enum_type) {
    fprintf(stream, "/* Enums. */\n\n");
    for(uint32_t i = 0; i < d->enum_type->len; i++) {  /* Foreach enum */
      google_protobuf_EnumDescriptorProto *e = d->enum_type->elements[i];
      struct upb_string enum_name = upb_strdup(*e->name);
      to_cident(enum_name);
      fprintf(stream, "typedef enum " UPB_STRFMT " {\n", UPB_STRARG(enum_name));
      if(e->set_flags.has.value) {
        for(uint32_t j = 0; j < e->value->len; j++) {  /* Foreach enum value. */
          google_protobuf_EnumValueDescriptorProto *v = e->value->elements[i];
          struct upb_string value_name = upb_strdup(*v->name);
          to_preproc(value_name);
          /* "  GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_TYPE_UINT32 = 13," */
          fprintf(stream, "  " UPB_STRFMT " = %" PRIu32,
                  UPB_STRARG(value_name), v->number);
          if(j != e->value->len-1) fputc(',', stream);
          fputc('\n', stream);
          upb_strfree(value_name);
        }
      }
      fprintf(stream, "} " UPB_STRFMT ";\n\n", UPB_STRARG(enum_name));
      upb_strfree(enum_name);
    }
  }

  /* Epilogue. */
  fputs("#ifdef __cplusplus\n", stream);
  fputs("}  /* extern \"C\" */\n", stream);
  fputs("#endif\n\n", stream);
  fprintf(stream, "#endif  /* " UPB_STRFMT " */\n", UPB_STRARG(include_guard_name));
  upb_strfree(include_guard_name);
}

int main()
{
  write_header(&google_protobuf_filedescriptor, stdout);
}

