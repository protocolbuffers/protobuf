/*
 * upb - a minimalist implementation of protocol buffers.
 *
 * Copyright (c) 2009 Joshua Haberman.  See LICENSE for details.
 */

#include "upb_textprinter.h"

#include <inttypes.h>
#include <stdlib.h>
#include "upb_def.h"
#include "upb_string.h"

struct _upb_textprinter {
  upb_sink sink;
  upb_bytesink *bytesink;
  upb_string *str;
  int indent_depth;
  bool single_line;
  upb_fielddef *f;
};

static void upb_textprinter_endfield(upb_textprinter *p)
{
  if(p->single_line)
    upb_bytesink_put(p->bytesink, UPB_STRLIT(" "));
  else
    upb_bytesink_put(p->bytesink, UPB_STRLIT("\n"));
}

static bool upb_textprinter_putval(upb_textprinter *p, upb_value val) {
  upb_bytesink_put(p->bytesink, UPB_STRLIT(": "));
  upb_enumdef *enum_def;
  upb_string *enum_label;
  if(p->f->type == UPB_TYPE(ENUM) &&
     (enum_def = upb_downcast_enumdef(p->f->def)) != NULL &&
     (enum_label = upb_enumdef_iton(enum_def, val.int32)) != NULL) {
    // This is an enum value for which we found a corresponding string.
    upb_bytesink_put(p->bytesink, enum_label);
  } else {
    p->str = upb_string_tryrecycle(p->str);
#define CASE(fmtstr, member) upb_string_printf(p->str, fmtstr, val.member); break;
    switch(p->f->type) {
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
    upb_bytesink_put(p->bytesink, p->str);
  }
  upb_textprinter_endfield(p);
  return upb_ok(upb_bytesink_status(p->bytesink));
}

static bool upb_textprinter_putstr(upb_textprinter *p, upb_string *str) {
  upb_bytesink_put(p->bytesink, UPB_STRLIT(": \""));
  // TODO: escaping.
  upb_bytesink_put(p->bytesink, str);
  upb_bytesink_put(p->bytesink, UPB_STRLIT("\""));
  upb_textprinter_endfield(p);
  return upb_ok(upb_bytesink_status(p->bytesink));
}

static void upb_textprinter_indent(upb_textprinter *p)
{
  if(!p->single_line)
    for(int i = 0; i < p->indent_depth; i++)
      upb_bytesink_put(p->bytesink, UPB_STRLIT("  "));
}

static bool upb_textprinter_putdef(upb_textprinter *p, upb_fielddef *f)
{
  upb_textprinter_indent(p);
  upb_bytesink_put(p->bytesink, f->name);
  p->f = f;
  return upb_ok(upb_bytesink_status(p->bytesink));
}

static bool upb_textprinter_startmsg(upb_textprinter *p)
{
  upb_bytesink_put(p->bytesink, UPB_STRLIT(" {"));
  if(!p->single_line) upb_bytesink_put(p->bytesink, UPB_STRLIT("\n"));
  p->indent_depth++;
  return upb_ok(upb_bytesink_status(p->bytesink));
}

static bool upb_textprinter_endmsg(upb_textprinter *p)
{
  p->indent_depth--;
  upb_textprinter_indent(p);
  upb_bytesink_put(p->bytesink, UPB_STRLIT("}"));
  upb_textprinter_endfield(p);
  return upb_ok(upb_bytesink_status(p->bytesink));
}

upb_sink_vtable upb_textprinter_vtbl = {
  (upb_sink_putdef_fptr)upb_textprinter_putdef,
  (upb_sink_putval_fptr)upb_textprinter_putval,
  (upb_sink_putstr_fptr)upb_textprinter_putstr,
  (upb_sink_startmsg_fptr)upb_textprinter_startmsg,
  (upb_sink_endmsg_fptr)upb_textprinter_endmsg,
};

upb_textprinter *upb_textprinter_new() {
  upb_textprinter *p = malloc(sizeof(*p));
  upb_sink_init(&p->sink, &upb_textprinter_vtbl);
  p->str = NULL;
  return p;
}

void upb_textprinter_free(upb_textprinter *p) {
  upb_string_unref(p->str);
  free(p);
}

void upb_textprinter_reset(upb_textprinter *p, upb_bytesink *sink,
                           bool single_line) {
  p->bytesink = sink;
  p->single_line = single_line;
  p->indent_depth = 0;
}

upb_sink *upb_textprinter_sink(upb_textprinter *p) { return &p->sink; }
