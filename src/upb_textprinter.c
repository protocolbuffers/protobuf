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
  upb_status status;
};

#define CHECK(x) if ((x) < 0) goto err;

static int upb_textprinter_indent(upb_textprinter *p) {
  if(!p->single_line)
    for(int i = 0; i < p->indent_depth; i++)
      CHECK(upb_bytesink_putstr(p->bytesink, UPB_STRLIT("  "), &p->status));
  return 0;
err:
  return -1;
}

static int upb_textprinter_endfield(upb_textprinter *p) {
  if(p->single_line) {
    CHECK(upb_bytesink_putstr(p->bytesink, UPB_STRLIT(" "), &p->status));
  } else {
    CHECK(upb_bytesink_putstr(p->bytesink, UPB_STRLIT("\n"), &p->status));
  }
  return 0;
err:
  return -1;
}

static upb_flow_t upb_textprinter_value(void *_p, upb_fielddef *f,
                                        upb_value val) {
  upb_textprinter *p = _p;
  upb_textprinter_indent(p);
  CHECK(upb_bytesink_printf(p->bytesink, &p->status, UPB_STRFMT ": ", UPB_STRARG(f->name)));
#define CASE(fmtstr, member) \
    CHECK(upb_bytesink_printf(p->bytesink, &p->status, fmtstr, upb_value_get ## member(val))); break;
  switch(f->type) {
    case UPB_TYPE(DOUBLE):
      CASE("%0.f", double);
    case UPB_TYPE(FLOAT):
      CASE("%0.f", float)
    case UPB_TYPE(INT64):
    case UPB_TYPE(SFIXED64):
    case UPB_TYPE(SINT64):
      CASE("%" PRId64, int64)
    case UPB_TYPE(UINT64):
    case UPB_TYPE(FIXED64):
      CASE("%" PRIu64, uint64)
    case UPB_TYPE(UINT32):
    case UPB_TYPE(FIXED32):
      CASE("%" PRIu32, uint32);
    case UPB_TYPE(ENUM): {
      upb_enumdef *enum_def = upb_downcast_enumdef(f->def);
      upb_string *enum_label =
          upb_enumdef_iton(enum_def, upb_value_getint32(val));
      if (enum_label) {
        // We found a corresponding string for this enum.  Otherwise we fall
        // through to the int32 code path.
        CHECK(upb_bytesink_putstr(p->bytesink, enum_label, &p->status));
        break;
      }
    }
    case UPB_TYPE(INT32):
    case UPB_TYPE(SFIXED32):
    case UPB_TYPE(SINT32):
      CASE("%" PRId32, int32)
    case UPB_TYPE(BOOL):
      CASE("%hhu", bool);
    case UPB_TYPE(STRING):
    case UPB_TYPE(BYTES):
      // TODO: escaping.
      CHECK(upb_bytesink_putstr(p->bytesink, UPB_STRLIT("\""), &p->status));
      CHECK(upb_bytesink_putstr(p->bytesink, upb_value_getstr(val), &p->status))
      CHECK(upb_bytesink_putstr(p->bytesink, UPB_STRLIT("\""), &p->status));
      break;
  }
  upb_textprinter_endfield(p);
  return UPB_CONTINUE;
err:
  return UPB_BREAK;
}

static upb_flow_t upb_textprinter_startsubmsg(void *_p, upb_fielddef *f,
                                              upb_handlers *delegate_to) {
  (void)delegate_to;
  upb_textprinter *p = _p;
  upb_textprinter_indent(p);
  CHECK(upb_bytesink_printf(p->bytesink, &p->status, UPB_STRFMT " {", UPB_STRARG(f->name)));
  if(!p->single_line) upb_bytesink_putstr(p->bytesink, UPB_STRLIT("\n"), &p->status);
  p->indent_depth++;
  return UPB_CONTINUE;
err:
  return UPB_BREAK;
}

static upb_flow_t upb_textprinter_endsubmsg(void *_p, upb_fielddef *f) {
  (void)f;
  upb_textprinter *p = _p;
  p->indent_depth--;
  upb_textprinter_indent(p);
  upb_bytesink_putstr(p->bytesink, UPB_STRLIT("}"), &p->status);
  upb_textprinter_endfield(p);
  return UPB_CONTINUE;
}

upb_textprinter *upb_textprinter_new() {
  upb_textprinter *p = malloc(sizeof(*p));
  return p;
}

void upb_textprinter_free(upb_textprinter *p) {
  free(p);
}

void upb_textprinter_reset(upb_textprinter *p, upb_handlers *handlers,
                           upb_bytesink *sink, bool single_line) {
  static upb_handlerset handlerset = {
    NULL,  // startmsg
    NULL,  // endmsg
    upb_textprinter_value,
    upb_textprinter_startsubmsg,
    upb_textprinter_endsubmsg,
  };
  p->bytesink = sink;
  p->single_line = single_line;
  p->indent_depth = 0;
  upb_register_handlerset(handlers, &handlerset);
  upb_set_handler_closure(handlers, p, &p->status);
}
