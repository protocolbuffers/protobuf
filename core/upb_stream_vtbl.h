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

#ifdef __cplusplus
extern "C" {
#endif

// Typedefs for function pointers to all of the virtual functions.

// upb_bytesrc.
typedef bool (*upb_bytesrc_get_fptr)(
    upb_bytesrc *src, upb_string *str, upb_strlen_t minlen);
typedef bool (*upb_bytesrc_append_fptr)(
    upb_bytesrc *src, upb_string *str, upb_strlen_t len);

// upb_bytesink.
typedef int32_t (*upb_bytesink_put_fptr)(upb_bytesink *sink, upb_string *str);

// Vtables for the above interfaces.
typedef struct {
  upb_bytesrc_get_fptr     get;
  upb_bytesrc_append_fptr  append;
} upb_bytesrc_vtable;

typedef struct {
  upb_bytesink_put_fptr  put;
} upb_bytesink_vtable;

// "Base Class" definitions; components that implement these interfaces should
// contain one of these structures.

struct _upb_bytesrc {
  upb_bytesrc_vtable *vtbl;
  upb_status status;
  bool eof;
};

struct _upb_bytesink {
  upb_bytesink_vtable *vtbl;
  upb_status status;
  bool eof;
};

INLINE void upb_bytesrc_init(upb_bytesrc *s, upb_bytesrc_vtable *vtbl) {
  s->vtbl = vtbl;
  s->eof = false;
  upb_status_init(&s->status);
}

INLINE void upb_bytesink_init(upb_bytesink *s, upb_bytesink_vtable *vtbl) {
  s->vtbl = vtbl;
  s->eof = false;
  upb_status_init(&s->status);
}

// Implementation of virtual function dispatch.

// upb_bytesrc
INLINE bool upb_bytesrc_get(
    upb_bytesrc *bytesrc, upb_string *str, upb_strlen_t minlen) {
  return bytesrc->vtbl->get(bytesrc, str, minlen);
}

INLINE bool upb_bytesrc_append(
    upb_bytesrc *bytesrc, upb_string *str, upb_strlen_t len) {
  return bytesrc->vtbl->append(bytesrc, str, len);
}

INLINE upb_status *upb_bytesrc_status(upb_bytesrc *src) { return &src->status; }
INLINE bool upb_bytesrc_eof(upb_bytesrc *src) { return src->eof; }

// upb_handlers
struct _upb_handlers {
  upb_handlerset *set;
  void *closure;
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

INLINE void upb_register_handlerset(upb_handlers *h, upb_handlerset *set) {
  h->set = set;
}

INLINE void upb_set_handler_closure(upb_handlers *h, void *closure) {
  h->closure = closure;
}

// upb_dispatcher
typedef struct {
  upb_handlers handlers;
  int depth;
} upb_dispatcher_frame;

struct _upb_dispatcher {
  upb_dispatcher_frame stack[UPB_MAX_NESTING], *top, *limit;
};

INLINE void upb_dispatcher_init(upb_dispatcher *d) {
  d->limit = d->stack + sizeof(d->stack);
}

INLINE void upb_dispatcher_reset(upb_dispatcher *d, upb_handlers *h) {
  d->top = d->stack;
  d->top->depth = 1;  // Never want to trigger end-of-delegation.
  d->top->handlers = *h;
}

INLINE void upb_dispatch_startmsg(upb_dispatcher *d) {
  assert(d->stack == d->top);
  d->top->handlers.set->startmsg(d->top->handlers.closure);
}

INLINE void upb_dispatch_endmsg(upb_dispatcher *d) {
  assert(d->stack == d->top);
  d->top->handlers.set->endmsg(d->top->handlers.closure);
}

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
    d->top->handlers.set->startmsg(d->top->handlers.closure);
    ret = UPB_CONTINUE;
  }
  ++d->top->depth;
  upb_handlers_uninit(&handlers);
  return ret;
}

INLINE upb_flow_t upb_dispatch_endsubmsg(upb_dispatcher *d) {
  if (--d->top->depth == 0) {
    d->top->handlers.set->endmsg(d->top->handlers.closure);
    --d->top;
  }
  return d->top->handlers.set->endsubmsg(d->top->handlers.closure);
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

// upb_bytesink
INLINE int32_t upb_bytesink_put(upb_bytesink *sink, upb_string *str) {
  return sink->vtbl->put(sink, str);
}
INLINE upb_status *upb_bytesink_status(upb_bytesink *sink) {
  return &sink->status;
}

// upb_bytesink


#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif
