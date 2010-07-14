/*
 * upb - a minimalist implementation of protocol buffers.
 *
 * Copyright (c) 2009 Joshua Haberman.  See LICENSE for details.
 */

#include <inttypes.h>
#include "descriptor.h"
#include "upb_text.h"
#include "upb_data.h"

bool upb_textprinter_putval(upb_textprinter *p, upb_value val) {
  upb_string *p->str = upb_string_tryrecycle(p->str);
#define CASE(fmtstr, member) upb_string_printf(p->str, fmtstr, val.member); break;
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
  }
  return upb_bytesink_put(p->str);
}

bool upb_textprinter_putstr(upb_textprinter *p, upb_string *str) {
  upb_bytesink_put(UPB_STRLIT("\""));
  // TODO: escaping.
  upb_bytesink_put(str);
  upb_bytesink_put(UPB_STRLIT("\""));
}

static void print_indent(upb_text_printer *p, FILE *stream)
{
  if(!p->single_line)
    for(int i = 0; i < p->indent_depth; i++)
      upb_bytesink_put(UPB_STRLIT("  "));
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

void upb_textprinter_startmsg(upb_textprinter *p)
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
