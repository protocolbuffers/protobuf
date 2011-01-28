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
  upb_bytesink *bytesink;
  int indent_depth;
  bool single_line;
  upb_fielddef *f;
};

static void upb_textprinter_indent(upb_textprinter *p)
{
  if(!p->single_line)
    for(int i = 0; i < p->indent_depth; i++)
      upb_bytesink_put(p->bytesink, UPB_STRLIT("  "));
}

static void upb_textprinter_endfield(upb_textprinter *p) {
  if(p->single_line)
    upb_bytesink_put(p->bytesink, UPB_STRLIT(" "));
  else
    upb_bytesink_put(p->bytesink, UPB_STRLIT("\n"));
}

static upb_flow_t upb_textprinter_value(void *_p, upb_fielddef *f,
                                        upb_value val) {
  upb_textprinter *p = _p;
  upb_textprinter_indent(p);
  upb_bytesink_printf(p->bytesink, UPB_STRFMT ": ", UPB_STRARG(f->name));
#define CASE(fmtstr, member) upb_bytesink_printf(p->bytesink, fmtstr, val.member); break;
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
      CASE("%" PRIu32, uint32);
    case UPB_TYPE(ENUM): {
      upb_enumdef *enum_def;
      upb_string *enum_label;
       (enum_def = upb_downcast_enumdef(p->f->def)) != NULL &&
       (enum_label = upb_enumdef_iton(enum_def, val.int32)) != NULL) {
      // This is an enum value for which we found a corresponding string.
      upb_bytesink_put(p->bytesink, enum_label);
      CASE("%" PRIu32, uint32);
    }
    case UPB_TYPE(BOOL):
      CASE("%hhu", _bool);
    case UPB_TYPE(STRING):
    case UPB_TYPE(BYTES):
      upb_bytesink_put(p->bytesink, UPB_STRLIT(": \""));
      upb_bytesink_put(p->bytesink, str);
      upb_bytesink_put(p->bytesink, UPB_STRLIT("\""));
      break;
  }
  upb_textprinter_endfield(p);
  return UPB_CONTINUE;
}

static upb_flow_t upb_textprinter_startsubmsg(void *_p, upb_fielddef *f) {
  upb_textprinter *p = _p;
  p->indent_depth++;
  upb_bytesink_put(p->bytesink, UPB_STRLIT(" {"));
  if(!p->single_line) upb_bytesink_put(p->bytesink, UPB_STRLIT("\n"));
  return UPB_CONTINUE;
}

static upb_flow_t upb_textprinter_endsubmsg(void *_p)
{
  upb_textprinter *p = _p;
  p->indent_depth--;
  upb_textprinter_indent(p);
  upb_bytesink_put(p->bytesink, UPB_STRLIT("}"));
  upb_textprinter_endfield(p);
  return UPB_CONTINUE;
}

upb_textprinter *upb_textprinter_new() {
  static upb_handlerset handlers = {
    NULL,  // startmsg
    NULL,  // endmsg
    upb_textprinter_putval,
    upb_textprinter_startsubmsg,
    upb_textprinter_endsubmsg,
  };
  upb_textprinter *p = malloc(sizeof(*p));
  upb_byte_init(&p->sink, &upb_textprinter_vtbl);
  return p;
}

void upb_textprinter_free(upb_textprinter *p) {
  free(p);
}

void upb_textprinter_reset(upb_textprinter *p, upb_bytesink *sink,
                           bool single_line) {
  p->bytesink = sink;
  p->single_line = single_line;
  p->indent_depth = 0;
}

upb_sink *upb_textprinter_sink(upb_textprinter *p) { return &p->sink; }
