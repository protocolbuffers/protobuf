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
#include "upb_enum.h"

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
static void write_header(struct upb_symtab_entry entries[], int num_entries,
                         struct upb_string outfile_name, FILE *stream)
{
  /* Header file prologue. */
  struct upb_string include_guard_name = upb_strdup(outfile_name);
  to_preproc(include_guard_name);
  fprintf(stream, "#ifndef " UPB_STRFMT "\n", UPB_STRARG(include_guard_name));
  fprintf(stream, "#define " UPB_STRFMT "\n\n", UPB_STRARG(include_guard_name));
  fputs("#include <upb_msg.h>\n\n", stream);
  fputs("#ifdef __cplusplus\n", stream);
  fputs("extern \"C\" {\n", stream);
  fputs("#endif\n\n", stream);

  /* Enums. */
  fprintf(stream, "/* Enums. */\n\n");
  for(int i = 0; i < num_entries; i++) {  /* Foreach enum */
    if(entries[i].type != UPB_SYM_ENUM) continue;
    struct upb_symtab_entry *entry = &entries[i];
    struct upb_enum *e = entry->ref._enum;
    google_protobuf_EnumDescriptorProto *ed = e->descriptor;
    /* We use entry->e.key (the fully qualified name) instead of ed->name. */
    struct upb_string enum_name = upb_strdup(entry->e.key);
    to_cident(enum_name);
    fprintf(stream, "typedef enum " UPB_STRFMT " {\n", UPB_STRARG(enum_name));
    if(ed->set_flags.has.value) {
      for(uint32_t j = 0; j < ed->value->len; j++) {  /* Foreach enum value. */
        google_protobuf_EnumValueDescriptorProto *v = ed->value->elements[j];
        struct upb_string value_name = upb_strdup(*v->name);
        to_preproc(value_name);
        /* "  GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_TYPE_UINT32 = 13," */
        fprintf(stream, "  " UPB_STRFMT " = %" PRIu32,
                UPB_STRARG(value_name), v->number);
        if(j != ed->value->len-1) fputc(',', stream);
        fputc('\n', stream);
        upb_strfree(value_name);
      }
    }
    fprintf(stream, "} " UPB_STRFMT ";\n\n", UPB_STRARG(enum_name));
    upb_strfree(enum_name);
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
  struct upb_context c;
  upb_context_init(&c);
  struct upb_string fds;
  assert(upb_strreadfile("/tmp/descriptor.proto.bin", &fds));
  assert(upb_context_parsefds(&c, &fds));
  struct upb_strtable *t = &c.symtab;
  int symcount = t->t.count;
  struct upb_symtab_entry entries[symcount];
  struct upb_symtab_entry *e = upb_strtable_begin(t);
  int i = 0;
  for(; e && i < symcount; e = upb_strtable_next(t, &e->e), i++)
    entries[i] = *e;
  assert(e == NULL && i == symcount);
  struct upb_string name = UPB_STRLIT("descriptor.proto");
  write_header(entries, symcount, name, stdout);
}

