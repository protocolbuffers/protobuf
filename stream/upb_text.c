/*
 * upb - a minimalist implementation of protocol buffers.
 *
 * Copyright (c) 2009 Joshua Haberman.  See LICENSE for details.
 */

#include <inttypes.h>
#include "descriptor.h"
#include "upb_text.h"
#include "upb_data.h"

void upb_text_printval(upb_field_type_t type, upb_value val, FILE *file)
{
#define CASE(fmtstr, member) fprintf(file, fmtstr, val.member); break;
  switch(type) {
    case UPB_TYPE(DOUBLE):
      CASE("%0.f", _double);
    case UPB_TYPE(FLOAT):
      CASE("%0.f", _float)
    case UPB_TYPE(INT64):
    case UPB_TYPE(SFIXED64):
    case UPB_TYPE(SINT64):
      CASE("%" PRId64, int64)
    case UPB_TYPE(UINT64):
    case UPB_TYPE(FIXED64):
      CASE("%" PRIu64, uint64)
    case UPB_TYPE(INT32):
    case UPB_TYPE(SFIXED32):
    case UPB_TYPE(SINT32):
      CASE("%" PRId32, int32)
    case UPB_TYPE(UINT32):
    case UPB_TYPE(FIXED32):
    case UPB_TYPE(ENUM):
      CASE("%" PRIu32, uint32);
    case UPB_TYPE(BOOL):
      CASE("%hhu", _bool);
    case UPB_TYPE(STRING):
    case UPB_TYPE(BYTES):
      /* TODO: escaping. */
      fprintf(file, "\"" UPB_STRFMT "\"", UPB_STRARG(val.str)); break;
  }
}

static void print_indent(upb_text_printer *p, FILE *stream)
{
  if(!p->single_line)
    for(int i = 0; i < p->indent_depth; i++)
      fprintf(stream, "  ");
}

void upb_text_printfield(upb_text_printer *p, upb_strptr name,
                         upb_field_type_t valtype, upb_value val,
                         FILE *stream)
{
  print_indent(p, stream);
  fprintf(stream, UPB_STRFMT ":", UPB_STRARG(name));
  upb_text_printval(valtype, val, stream);
  if(p->single_line)
    fputc(' ', stream);
  else
    fputc('\n', stream);
}

void upb_text_push(upb_text_printer *p, upb_strptr submsg_type, FILE *stream)
{
  print_indent(p, stream);
  fprintf(stream, UPB_STRFMT " {", UPB_STRARG(submsg_type));
  if(!p->single_line) fputc('\n', stream);
  p->indent_depth++;
}

void upb_text_pop(upb_text_printer *p, FILE *stream)
{
  p->indent_depth--;
  print_indent(p, stream);
  fprintf(stream, "}\n");
}

static void printval(upb_text_printer *printer, upb_value v, upb_fielddef *f,
                     FILE *stream);

static void printmsg(upb_text_printer *printer, upb_msg *msg, upb_msgdef *md,
                     FILE *stream)
{
  for(upb_field_count_t i = 0; i < md->num_fields; i++) {
    upb_fielddef *f = &md->fields[i];
    if(!upb_msg_has(msg, f)) continue;
    upb_value v = upb_msg_get(msg, f);
    if(upb_isarray(f)) {
      upb_arrayptr arr = v.arr;
      for(uint32_t j = 0; j < upb_array_len(arr); j++) {
        upb_value elem = upb_array_get(arr, f, j);
        printval(printer, elem, f, stream);
      }
    } else {
      printval(printer, v, f, stream);
    }
  }
}

static void printval(upb_text_printer *printer, upb_value v, upb_fielddef *f,
                     FILE *stream)
{
  if(upb_issubmsg(f)) {
    upb_text_push(printer, f->name, stream);
    printmsg(printer, v.msg, upb_downcast_msgdef(f->def), stream);
    upb_text_pop(printer, stream);
  } else {
    upb_text_printfield(printer, f->name, f->type, v, stream);
  }
}


void upb_msg_print(upb_msg *msg, upb_msgdef *md, bool single_line,
                   FILE *stream)
{
  upb_text_printer printer;
  upb_text_printer_init(&printer, single_line);
  printmsg(&printer, msg, md, stream);
}

