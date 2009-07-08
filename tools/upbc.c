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

static int memrchr(char *data, char c, size_t len)
{
  int off = len-1;
  while(off > 0 && data[off] != c) --off;
  return off;
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

    struct upb_string enum_val_prefix = upb_strdup(entry->e.key);
    enum_val_prefix.byte_len = memrchr(enum_val_prefix.ptr,
                                       UPB_SYMBOL_SEPARATOR,
                                       enum_val_prefix.byte_len);
    enum_val_prefix.byte_len++;
    to_preproc(enum_val_prefix);

    fprintf(stream, "typedef enum " UPB_STRFMT " {\n", UPB_STRARG(enum_name));
    if(ed->set_flags.has.value) {
      for(uint32_t j = 0; j < ed->value->len; j++) {  /* Foreach enum value. */
        google_protobuf_EnumValueDescriptorProto *v = ed->value->elements[j];
        struct upb_string value_name = upb_strdup(*v->name);
        to_preproc(value_name);
        /* "  GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_TYPE_UINT32 = 13," */
        fprintf(stream, "  " UPB_STRFMT UPB_STRFMT " = %" PRIu32,
                UPB_STRARG(enum_val_prefix), UPB_STRARG(value_name), v->number);
        if(j != ed->value->len-1) fputc(',', stream);
        fputc('\n', stream);
        upb_strfree(value_name);
      }
    }
    fprintf(stream, "} " UPB_STRFMT ";\n\n", UPB_STRARG(enum_name));
    upb_strfree(enum_name);
    upb_strfree(enum_val_prefix);
  }

  /* Forward declarations. */
  fputs("/* Forward declarations of all message types.\n", stream);
  fputs(" * So they can refer to each other in ", stream);
  fputs("possibly-recursive ways. */\n\n", stream);

  for(int i = 0; i < num_entries; i++) {  /* Foreach message */
    if(entries[i].type != UPB_SYM_MESSAGE) continue;
    struct upb_symtab_entry *entry = &entries[i];
    /* We use entry->e.key (the fully qualified name). */
    struct upb_string msg_name = upb_strdup(entry->e.key);
    to_cident(msg_name);
    fprintf(stream, "struct " UPB_STRFMT ";\n", UPB_STRARG(msg_name));
    fprintf(stream, "typedef struct " UPB_STRFMT "\n  " UPB_STRFMT ";\n\n",
            UPB_STRARG(msg_name), UPB_STRARG(msg_name));
    upb_strfree(msg_name);
  }

  /* Message Declarations. */
  fputs("/* The message definitions themselves. */\n\n", stream);
  for(int i = 0; i < num_entries; i++) {  /* Foreach message */
    if(entries[i].type != UPB_SYM_MESSAGE) continue;
    struct upb_symtab_entry *entry = &entries[i];
    struct upb_msg *m = entry->ref.msg;
    /* We use entry->e.key (the fully qualified name). */
    struct upb_string msg_name = upb_strdup(entry->e.key);
    to_cident(msg_name);
    fprintf(stream, "struct " UPB_STRFMT " {\n", UPB_STRARG(msg_name));
    fputs("  union {\n", stream);
    fprintf(stream, "    uint8_t bytes[%" PRIu32 "];\n", m->set_flags_bytes);
    fputs("    struct {\n", stream);
    for(uint32_t j = 0; j < m->num_fields; j++) {
      static char* labels[] = {"", "optional", "required", "repeated"};
      struct google_protobuf_FieldDescriptorProto *fd = m->field_descriptors[j];
      fprintf(stream, "      bool " UPB_STRFMT ":1;  /* = %" PRIu32 ", %s. */\n",
              UPB_STRARG(*fd->name), fd->number, labels[fd->label]);
    }
    fputs("    } has;\n", stream);
    fputs("  } set_flags;\n", stream);
    for(uint32_t j = 0; j < m->num_fields; j++) {
      struct upb_msg_field *f = &m->fields[j];
      struct google_protobuf_FieldDescriptorProto *fd = m->field_descriptors[j];
      if(f->type == GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_TYPE_GROUP ||
         f->type == GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_TYPE_MESSAGE) {
        /* Submessages get special treatment, since we have to use the message
         * name directly. */
        struct upb_string type_name_ref = *fd->type_name;
        if(type_name_ref.ptr[0] == UPB_SYMBOL_SEPARATOR) {
          /* Omit leading '.'. */
          type_name_ref.ptr++;
          type_name_ref.byte_len--;
        }
        struct upb_string type_name = upb_strdup(type_name_ref);
        to_cident(type_name);
        if(f->label == GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_LABEL_REPEATED) {
          fprintf(stream, "  UPB_MSG_ARRAY(" UPB_STRFMT ")* " UPB_STRFMT ";\n",
                  UPB_STRARG(type_name), UPB_STRARG(*fd->name));
        } else {
          fprintf(stream, "  " UPB_STRFMT "* " UPB_STRFMT ";\n",
                  UPB_STRARG(type_name), UPB_STRARG(*fd->name));
        }
        upb_strfree(type_name);
      } else if(f->label == GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_LABEL_REPEATED) {
        static char* c_types[] = {
          "", "struct upb_double_array*", "struct upb_float_array*",
          "struct upb_int64_array*", "struct upb_uint64_array*",
          "struct upb_int32_array*", "struct upb_uint64_array*",
          "struct upb_uint32_array*", "struct upb_bool_array*",
          "struct upb_string_array*", "", "",
          "struct upb_string_array*", "struct upb_uint32_array*",
          "struct upb_uint32_array*", "struct upb_int32_array*",
          "struct upb_int64_array*", "struct upb_int32_array*",
          "struct upb_int64_array*"
        };
        fprintf(stream, "  %s " UPB_STRFMT ";\n",
                c_types[fd->type], UPB_STRARG(*fd->name));
      } else {
        static char* c_types[] = {
          "", "double", "float", "int64_t", "uint64_t", "int32_t", "uint64_t",
          "uint32_t", "bool", "struct upb_string*", "", "",
          "struct upb_string*", "uint32_t", "uint32_t", "int32_t", "int64_t",
          "int32_t", "int64_t"
        };
        fprintf(stream, "  %s " UPB_STRFMT ";\n",
                c_types[fd->type], UPB_STRARG(*fd->name));
      }
    }
    fputs("};\n", stream);
    fprintf(stream, "UPB_DEFINE_MSG_ARRAY(" UPB_STRFMT ")\n\n",
            UPB_STRARG(msg_name));
    upb_strfree(msg_name);
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
  upb_context_free(&c);
  upb_strfree(fds);
}

