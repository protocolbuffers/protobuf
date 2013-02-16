/*
 * upb - a minimalist implementation of protocol buffers.
 *
 * Copyright (c) 2011-2012 Google Inc.  See LICENSE for details.
 * Author: Josh Haberman <jhaberman@gmail.com>
 */

#include "upb/sink.h"

static bool chkstack(upb_sink *s) {
  if (s->top + 1 >= s->limit) {
    upb_status_seterrliteral(&s->status, "Nesting too deep.");
    return false;
  } else {
    return true;
  }
}

static upb_selector_t getselector(const upb_fielddef *f,
                                  upb_handlertype_t type) {
  upb_selector_t selector;
  bool ok = upb_getselector(f, type, &selector);
  UPB_ASSERT_VAR(ok, ok);
  return selector;
}

void upb_sink_init(upb_sink *s, const upb_handlers *h) {
  s->limit = &s->stack[UPB_MAX_NESTING];
  s->top = NULL;
  s->stack[0].h = h;
  upb_status_init(&s->status);
}

void upb_sink_reset(upb_sink *s, void *closure) {
  s->top = s->stack;
  s->top->closure = closure;
}

void upb_sink_uninit(upb_sink *s) {
  upb_status_uninit(&s->status);
}

bool upb_sink_startmsg(upb_sink *s) {
  const upb_handlers *h = s->top->h;
  upb_startmsg_handler *startmsg = upb_handlers_getstartmsg(h);
  return startmsg ? startmsg(s->top->closure) : true;
}

void upb_sink_endmsg(upb_sink *s, upb_status *status) {
  UPB_UNUSED(status);
  assert(s->top == s->stack);
  upb_endmsg_handler *endmsg = upb_handlers_getendmsg(s->top->h);
  if (endmsg) endmsg(s->top->closure, &s->status);
}

#define PUTVAL(type, ctype, htype) \
  bool upb_sink_put ## type(upb_sink *s, const upb_fielddef *f, ctype val) { \
    upb_selector_t selector; \
    if (!upb_getselector(f, UPB_HANDLER_ ## htype, &selector)) return false; \
    upb_ ## type ## _handler *handler = (upb_ ## type ## _handler*) \
        upb_handlers_gethandler(s->top->h, selector); \
    if (handler) { \
      void *data = upb_handlers_gethandlerdata(s->top->h, selector); \
      if (!handler(s->top->closure, data, val)) return false; \
    } \
    return true; \
  }

PUTVAL(int32,  int32_t,         INT32);
PUTVAL(int64,  int64_t,         INT64);
PUTVAL(uint32, uint32_t,        UINT32);
PUTVAL(uint64, uint64_t,        UINT64);
PUTVAL(float,  float,           FLOAT);
PUTVAL(double, double,          DOUBLE);
PUTVAL(bool,   bool,            BOOL);
#undef PUTVAL

size_t upb_sink_putstring(upb_sink *s, const upb_fielddef *f,
                          const char *buf, size_t n) {
  upb_selector_t selector;
  if (!upb_getselector(f, UPB_HANDLER_STRING, &selector)) return false;
  upb_string_handler *handler = (upb_string_handler*)
      upb_handlers_gethandler(s->top->h, selector);
  if (handler) {
    void *data = upb_handlers_gethandlerdata(s->top->h, selector); \
    return handler(s->top->closure, data, buf, n);
  }
  return n;
}

bool upb_sink_startseq(upb_sink *s, const upb_fielddef *f) {
  assert(upb_fielddef_isseq(f));
  if (!chkstack(s)) return false;

  void *subc = s->top->closure;
  const upb_handlers *h = s->top->h;
  upb_selector_t selector;
  if (!upb_getselector(f, UPB_HANDLER_STARTSEQ, &selector)) return false;
  upb_startfield_handler *startseq =
      (upb_startfield_handler*)upb_handlers_gethandler(h, selector);
  if (startseq) {
    subc = startseq(s->top->closure, upb_handlers_gethandlerdata(h, selector));
    if (!subc) return false;
  }

  ++s->top;
  s->top->end = getselector(f, UPB_HANDLER_ENDSEQ);
  s->top->h = h;
  s->top->closure = subc;
  return true;
}

bool upb_sink_endseq(upb_sink *s, const upb_fielddef *f) {
  upb_selector_t selector = s->top->end;
  assert(selector == getselector(f, UPB_HANDLER_ENDSEQ));
  --s->top;

  const upb_handlers *h = s->top->h;
  upb_endfield_handler *endseq =
      (upb_endfield_handler*)upb_handlers_gethandler(h, selector);
  return endseq ?
      endseq(s->top->closure, upb_handlers_gethandlerdata(h, selector)) :
      true;
}

bool upb_sink_startstr(upb_sink *s, const upb_fielddef *f, size_t size_hint) {
  assert(upb_fielddef_isstring(f));
  if (!chkstack(s)) return false;

  void *subc = s->top->closure;
  const upb_handlers *h = s->top->h;
  upb_selector_t selector;
  if (!upb_getselector(f, UPB_HANDLER_STARTSTR, &selector)) return false;
  upb_startstr_handler *startstr =
      (upb_startstr_handler*)upb_handlers_gethandler(h, selector);
  if (startstr) {
    subc = startstr(
        s->top->closure, upb_handlers_gethandlerdata(h, selector), size_hint);
    if (!subc) return false;
  }

  ++s->top;
  s->top->end = getselector(f, UPB_HANDLER_ENDSTR);
  s->top->h = h;
  s->top->closure = subc;
  return true;
}

bool upb_sink_endstr(upb_sink *s, const upb_fielddef *f) {
  upb_selector_t selector = s->top->end;
  assert(selector == getselector(f, UPB_HANDLER_ENDSTR));
  --s->top;

  const upb_handlers *h = s->top->h;
  upb_endfield_handler *endstr =
      (upb_endfield_handler*)upb_handlers_gethandler(h, selector);
  return endstr ?
      endstr(s->top->closure, upb_handlers_gethandlerdata(h, selector)) :
      true;
}

bool upb_sink_startsubmsg(upb_sink *s, const upb_fielddef *f) {
  assert(upb_fielddef_issubmsg(f));
  if (!chkstack(s)) return false;

  const upb_handlers *h = s->top->h;
  upb_selector_t selector;
  if (!upb_getselector(f, UPB_HANDLER_STARTSUBMSG, &selector)) return false;
  upb_startfield_handler *startsubmsg =
      (upb_startfield_handler*)upb_handlers_gethandler(h, selector);
  void *subc = s->top->closure;

  if (startsubmsg) {
    void *data = upb_handlers_gethandlerdata(h, selector);
    subc = startsubmsg(s->top->closure, data);
    if (!subc) return false;
  }

  ++s->top;
  s->top->end = getselector(f, UPB_HANDLER_ENDSUBMSG);
  s->top->h = upb_handlers_getsubhandlers(h, f);
  s->top->closure = subc;
  upb_sink_startmsg(s);
  return true;
}

bool upb_sink_endsubmsg(upb_sink *s, const upb_fielddef *f) {
  upb_selector_t selector = s->top->end;
  assert(selector == getselector(f, UPB_HANDLER_ENDSUBMSG));

  upb_endmsg_handler *endmsg = upb_handlers_getendmsg(s->top->h);
  if (endmsg) endmsg(s->top->closure, &s->status);
  --s->top;

  const upb_handlers *h = s->top->h;
  upb_endfield_handler *endfield =
      (upb_endfield_handler*)upb_handlers_gethandler(h, selector);
  return endfield ?
      endfield(s->top->closure, upb_handlers_gethandlerdata(h, selector)) :
      true;
}

const upb_handlers *upb_sink_tophandlers(upb_sink *s) {
  return s->top->h;
}
