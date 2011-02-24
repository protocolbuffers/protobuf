/*
 * upb - a minimalist implementation of protocol buffers.
 *
 * vtable declarations for types that are implementing any of the src or sink
 * interfaces.  Only components that are implementing these interfaces need
 * to worry about this file.
 *
 * Copyright (c) 2010 Joshua Haberman.  See LICENSE for details.
 */

#ifndef UPB_SRCSINK_VTBL_H_
#define UPB_SRCSINK_VTBL_H_

#include <assert.h>
#include "upb_stream.h"
#include "upb_string.h"

#ifdef __cplusplus
extern "C" {
#endif

// Typedefs for function pointers to all of the virtual functions.

// upb_src
typedef void (*upb_src_sethandlers_fptr)(upb_src *src, upb_handlers *handlers);
typedef void (*upb_src_run_fptr)(upb_src *src, upb_status *status);

// upb_bytesrc.
typedef upb_strlen_t (*upb_bytesrc_read_fptr)(
    upb_bytesrc *src, void *buf, upb_strlen_t count, upb_status *status);
typedef bool (*upb_bytesrc_getstr_fptr)(
    upb_bytesrc *src, upb_string *str, upb_status *status);

// upb_bytesink.
typedef upb_strlen_t (*upb_bytesink_write_fptr)(
    upb_bytesink *bytesink, void *buf, upb_strlen_t count);
typedef upb_strlen_t (*upb_bytesink_putstr_fptr)(
    upb_bytesink *bytesink, upb_string *str, upb_status *status);
typedef upb_strlen_t (*upb_bytesink_vprintf_fptr)(
    upb_bytesink *bytesink, upb_status *status, const char *fmt, va_list args);

// Vtables for the above interfaces.
typedef struct {
  upb_bytesrc_read_fptr   read;
  upb_bytesrc_getstr_fptr getstr;
} upb_bytesrc_vtbl;

typedef struct {
  upb_bytesink_write_fptr   write;
  upb_bytesink_putstr_fptr  putstr;
  upb_bytesink_vprintf_fptr vprintf;
} upb_bytesink_vtbl;

typedef struct {
  upb_src_sethandlers_fptr sethandlers;
  upb_src_run_fptr         run;
} upb_src_vtbl;


// "Base Class" definitions; components that implement these interfaces should
// contain one of these structures.

struct _upb_bytesrc {
  upb_bytesrc_vtbl *vtbl;
};

struct _upb_bytesink {
  upb_bytesink_vtbl *vtbl;
};

struct _upb_src {
  upb_src_vtbl *vtbl;
};

INLINE void upb_bytesrc_init(upb_bytesrc *s, upb_bytesrc_vtbl *vtbl) {
  s->vtbl = vtbl;
}

INLINE void upb_bytesink_init(upb_bytesink *s, upb_bytesink_vtbl *vtbl) {
  s->vtbl = vtbl;
}

INLINE void upb_src_init(upb_src *s, upb_src_vtbl *vtbl) {
  s->vtbl = vtbl;
}

// Implementation of virtual function dispatch.

// upb_src
INLINE void upb_src_sethandlers(upb_src *src, upb_handlers *handlers) {
  src->vtbl->sethandlers(src, handlers);
}

INLINE void upb_src_run(upb_src *src, upb_status *status) {
  src->vtbl->run(src, status);
}

// upb_bytesrc
INLINE upb_strlen_t upb_bytesrc_read(upb_bytesrc *src, void *buf,
                                     upb_strlen_t count, upb_status *status) {
  return src->vtbl->read(src, buf, count, status);
}

INLINE bool upb_bytesrc_getstr(upb_bytesrc *src, upb_string *str,
                               upb_status *status) {
  return src->vtbl->getstr(src, str, status);
}

INLINE bool upb_bytesrc_getfullstr(upb_bytesrc *src, upb_string *str,
                                   upb_status *status) {
  // We start with a getstr, because that could possibly alias data instead of
  // copying.
  if (!upb_bytesrc_getstr(src, str, status)) return false;
  // Trade-off between number of read calls and amount of overallocation.
  const size_t bufsize = 4096;
  do {
    upb_strlen_t len = upb_string_len(str);
    char *buf = upb_string_getrwbuf(str, len + bufsize);
    upb_strlen_t read = upb_bytesrc_read(src, buf + len, bufsize, status);
    if (read < 0) return false;
    // Resize to proper size.
    upb_string_getrwbuf(str, len + read);
  } while (!status->code != UPB_EOF);
  return true;
}


// upb_bytesink
INLINE upb_strlen_t upb_bytesink_write(upb_bytesink *sink, void *buf,
                                       upb_strlen_t count) {
  return sink->vtbl->write(sink, buf, count);
}

INLINE upb_strlen_t upb_bytesink_putstr(upb_bytesink *sink, upb_string *str, upb_status *status) {
  return sink->vtbl->putstr(sink, str, status);
}

INLINE upb_strlen_t upb_bytesink_printf(upb_bytesink *sink, upb_status *status, const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  upb_strlen_t ret = sink->vtbl->vprintf(sink, status, fmt, args);
  va_end(args);
  return ret;
}

// upb_handlers
struct _upb_handlers {
  upb_handlerset *set;
  void *closure;
  upb_status *status;  // We don't own this.
};

INLINE void upb_handlers_init(upb_handlers *h) {
  (void)h;
}
INLINE void upb_handlers_uninit(upb_handlers *h) {
  (void)h;
}

INLINE void upb_handlers_reset(upb_handlers *h) {
  h->set = NULL;
  h->closure = NULL;
}

INLINE bool upb_handlers_isempty(upb_handlers *h) {
  return !h->set && !h->closure;
}

INLINE upb_flow_t upb_nop(void *closure) {
  (void)closure;
  return UPB_CONTINUE;
}

INLINE upb_flow_t upb_value_nop(void *closure, struct _upb_fielddef *f, upb_value val) {
  (void)closure;
  (void)f;
  (void)val;
  return UPB_CONTINUE;
}

INLINE upb_flow_t upb_startsubmsg_nop(void *closure, struct _upb_fielddef *f,
                                      upb_handlers *delegate_to) {
  (void)closure;
  (void)f;
  (void)delegate_to;
  return UPB_CONTINUE;
}

INLINE upb_flow_t upb_endsubmsg_nop(void *closure, struct _upb_fielddef *f) {
  (void)closure;
  (void)f;
  return UPB_CONTINUE;
}

INLINE upb_flow_t upb_unknownval_nop(void *closure, upb_field_number_t fieldnum,
                                     upb_value val) {
  (void)closure;
  (void)fieldnum;
  (void)val;
  return UPB_CONTINUE;
}

INLINE void upb_register_handlerset(upb_handlers *h, upb_handlerset *set) {
  if (!set->startmsg) set->startmsg = &upb_nop;
  if (!set->endmsg) set->endmsg = &upb_nop;
  if (!set->value) set->value = &upb_value_nop;
  if (!set->startsubmsg) set->startsubmsg = &upb_startsubmsg_nop;
  if (!set->endsubmsg) set->endsubmsg = &upb_endsubmsg_nop;
  if (!set->unknownval) set->unknownval = &upb_unknownval_nop;
  h->set = set;
}

INLINE void upb_set_handler_closure(upb_handlers *h, void *closure,
                                    upb_status *status) {
  h->closure = closure;
  h->status = status;
}

// upb_dispatcher
typedef struct {
  upb_handlers handlers;
  int depth;
} upb_dispatcher_frame;

struct _upb_dispatcher {
  upb_dispatcher_frame stack[UPB_MAX_NESTING], *top, *limit;
  bool supports_skip;
};

INLINE void upb_dispatcher_init(upb_dispatcher *d) {
  d->limit = d->stack + sizeof(d->stack);
}

INLINE void upb_dispatcher_reset(upb_dispatcher *d, upb_handlers *h,
                                 bool supports_skip) {
  d->top = d->stack;
  d->top->depth = 1;  // Never want to trigger end-of-delegation.
  d->top->handlers = *h;
  d->supports_skip = supports_skip;
}

INLINE upb_flow_t upb_dispatch_startmsg(upb_dispatcher *d) {
  assert(d->stack == d->top);
  return d->top->handlers.set->startmsg(d->top->handlers.closure);
}

INLINE upb_flow_t upb_dispatch_endmsg(upb_dispatcher *d) {
  assert(d->stack == d->top);
  return d->top->handlers.set->endmsg(d->top->handlers.closure);
}

// TODO: several edge cases to fix:
// - delegated start returns UPB_BREAK, should replay the start on resume.
// - endsubmsg returns UPB_BREAK, should NOT replay the delegated endmsg.
INLINE upb_flow_t upb_dispatch_startsubmsg(upb_dispatcher *d,
                                           struct _upb_fielddef *f) {
  upb_handlers handlers;
  upb_handlers_init(&handlers);
  upb_handlers_reset(&handlers);
  upb_flow_t ret = d->top->handlers.set->startsubmsg(d->top->handlers.closure, f, &handlers);
  assert((ret == UPB_DELEGATE) == !upb_handlers_isempty(&handlers));
  if (ret == UPB_DELEGATE) {
    ++d->top;
    d->top->handlers = handlers;
    d->top->depth = 0;
    ret = d->top->handlers.set->startmsg(d->top->handlers.closure);
  }
  if (ret == UPB_CONTINUE || !d->supports_skip) ++d->top->depth;
  upb_handlers_uninit(&handlers);
  return ret;
}

INLINE upb_flow_t upb_dispatch_endsubmsg(upb_dispatcher *d,
                                         struct _upb_fielddef *f) {
  upb_flow_t ret;
  if (--d->top->depth == 0) {
    ret = d->top->handlers.set->endmsg(d->top->handlers.closure);
    //assert(ret != UPB_BREAK);
    if (ret != UPB_CONTINUE) return ret;
    --d->top;
    assert(d->top >= d->stack);
  }
  return d->top->handlers.set->endsubmsg(d->top->handlers.closure, f);
}

INLINE upb_flow_t upb_dispatch_value(upb_dispatcher *d,
                                     struct _upb_fielddef *f,
                                     upb_value val) {
  return d->top->handlers.set->value(d->top->handlers.closure, f, val);
}

INLINE upb_flow_t upb_dispatch_unknownval(upb_dispatcher *d,
                                          upb_field_number_t fieldnum,
                                          upb_value val) {
  return d->top->handlers.set->unknownval(d->top->handlers.closure,
                                          fieldnum, val);
}

#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif
