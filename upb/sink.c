/*
 * upb - a minimalist implementation of protocol buffers.
 *
 * Copyright (c) 2011-2012 Google Inc.  See LICENSE for details.
 * Author: Josh Haberman <jhaberman@gmail.com>
 */

#include "upb/sink.h"

#include <stdlib.h>
#include <string.h>

static void upb_sink_init(upb_sink *s, const upb_handlers *h, upb_pipeline *p);
static void upb_sink_resetobj(void *obj);
static const upb_frametype upb_sink_frametype;

static bool chkstack(upb_sink *s) {
  if (s->top + 1 >= s->limit) {
    upb_status_seterrliteral(&s->pipeline_->status_, "Nesting too deep.");
    return false;
  } else {
    return true;
  }
}

#define alignof(type) offsetof (struct { char c; type member; }, member)

typedef union { double u; void *p; long l; } maxalign_t;
static const size_t maxalign = alignof(maxalign_t);

static void *align_up(void *p) {
  if (!p) return NULL;
  uintptr_t val = (uintptr_t)p;
  uintptr_t aligned =
      val % maxalign == 0 ? val : val + maxalign - (val % maxalign);
  return (void*)aligned;
}

void *upb_realloc(void *ud, void *ptr, size_t size) {
  UPB_UNUSED(ud);
  return realloc(ptr, size);
}


/* upb_pipeline ***************************************************************/

// For the moment we get fixed-size blocks of this size, but we could change
// this strategy if necessary.
#define BLOCK_SIZE 8192

struct region {
  struct region *prev;
  maxalign_t data[1];  // Region data follows.
};

size_t regionsize(size_t usable_size) {
  return sizeof(struct region) - sizeof(maxalign_t) + usable_size;
}

struct obj {
  struct obj *prev;
  const upb_frametype *ft;
  maxalign_t data;  // Region data follows.
};

size_t objsize(size_t memsize) {
  return sizeof(struct obj) - sizeof(maxalign_t) + memsize;
}

void upb_pipeline_init(upb_pipeline *p, void *initial_mem, size_t initial_size,
                       void *(*realloc)(void *ud, void *ptr, size_t bytes),
                       void *ud) {
  p->realloc = realloc;
  p->ud = ud;
  p->bump_top = initial_mem;
  p->bump_limit = initial_mem ? initial_mem + initial_size : NULL;
  p->region_head = NULL;
  p->obj_head = NULL;
  p->last_alloc = NULL;
  upb_status_init(&p->status_);
}

void upb_pipeline_uninit(upb_pipeline *p) {
  for (struct obj *o = p->obj_head; o; o = o->prev) {
    if (o->ft->uninit)
      o->ft->uninit(&o->data);
  }

  for (struct region *r = p->region_head; r; ) {
    struct region *prev = r->prev;
    p->realloc(p->ud, r, 0);
    r = prev;
  }
  upb_status_uninit(&p->status_);
}

void *upb_pipeline_alloc(upb_pipeline *p, size_t bytes) {
  void *mem = align_up(p->bump_top);
  if (!mem || mem > p->bump_limit || p->bump_limit - mem < bytes) {
    size_t size = regionsize(UPB_MAX(BLOCK_SIZE, bytes));
    struct region *r;
    if (!p->realloc || !(r = p->realloc(p->ud, NULL, size))) {
      return NULL;
    }
    r->prev = p->region_head;
    p->region_head = r;
    p->bump_limit = (char*)r + size;
    mem = &r->data[0];
    assert(p->bump_limit > mem);
    assert(p->bump_limit - mem >= bytes);
  }
  p->bump_top = mem + bytes;
  p->last_alloc = mem;
  return mem;
}

void *upb_pipeline_realloc(upb_pipeline *p, void *ptr,
                           size_t oldsize, size_t bytes) {
  if (ptr && ptr == p->last_alloc &&
      p->bump_limit - ptr >= bytes) {
    p->bump_top = ptr + bytes;
    return ptr;
  } else {
    void *mem = upb_pipeline_alloc(p, bytes);
    memcpy(mem, ptr, oldsize);
    return mem;
  }
}

void *upb_pipeline_allocobj(upb_pipeline *p, const upb_frametype *ft) {
  struct obj *obj = upb_pipeline_alloc(p, objsize(ft->size));
  if (!obj) return NULL;

  obj->prev = p->obj_head;
  obj->ft = ft;
  p->obj_head = obj;
  if (ft->init) ft->init(&obj->data, p);
  return &obj->data;
}

void upb_pipeline_reset(upb_pipeline *p) {
  upb_status_clear(&p->status_);
  for (struct obj *o = p->obj_head; o; o = o->prev) {
    if (o->ft->reset)
      o->ft->reset(&o->data);
  }
}

upb_sink *upb_pipeline_newsink(upb_pipeline *p, const upb_handlers *handlers) {
  upb_sink *s = upb_pipeline_allocobj(p, &upb_sink_frametype);
  upb_sink_init(s, handlers, p);
  return s;
}

const upb_status *upb_pipeline_status(const upb_pipeline *p) {
  return &p->status_;
}

typedef struct {
  const upb_handlers *h;
} handlersref_t;

static void freehandlersref(void *r) {
  handlersref_t *ref = r;
  upb_handlers_unref(ref->h, &ref->h);
}

static const upb_frametype handlersref_frametype = {
  sizeof(handlersref_t),
  NULL,
  freehandlersref,
  NULL,
};

void upb_pipeline_donateref(
    upb_pipeline *p, const upb_handlers *h, const void *owner) {
  handlersref_t *ref = upb_pipeline_allocobj(p, &handlersref_frametype);
  upb_handlers_donateref(h, owner, &ref->h);
  ref->h = h;
}


/* upb_sink *******************************************************************/

static const upb_frametype upb_sink_frametype = {
  sizeof(upb_sink),
  NULL,
  NULL,
  upb_sink_resetobj,
};

void upb_sink_reset(upb_sink *s, void *closure) {
  s->top = s->stack;
  s->top->closure = closure;
}

static void upb_sink_resetobj(void *obj) {
  upb_sink *s = obj;
  s->top = s->stack;
}

static void upb_sink_init(upb_sink *s, const upb_handlers *h, upb_pipeline *p) {
  s->pipeline_ = p;
  s->stack = upb_pipeline_alloc(p, sizeof(*s->stack) * UPB_MAX_NESTING);
  s->top = s->stack;
  s->limit = s->stack + UPB_MAX_NESTING;
  s->top->h = h;
  if (h->ft) {
    s->top->closure = upb_pipeline_allocobj(p, h->ft);
  }
}

upb_pipeline *upb_sink_pipeline(const upb_sink *s) {
  return s->pipeline_;
}

void *upb_sink_getobj(const upb_sink *s) {
  return s->stack[0].closure;
}

bool upb_sink_startmsg(upb_sink *s) {
  const upb_handlers *h = s->top->h;
  upb_startmsg_handler *startmsg =
      (upb_startmsg_handler *)upb_handlers_gethandler(h, UPB_STARTMSG_SELECTOR);
  if (startmsg) {
    const void *hd = upb_handlers_gethandlerdata(h, UPB_STARTMSG_SELECTOR);
    bool ok = startmsg(s->top->closure, hd);
    if (!ok) return false;
  }
  return true;
}

bool upb_sink_endmsg(upb_sink *s) {
  assert(s->top == s->stack);
  const upb_handlers *h = s->top->h;
  upb_endmsg_handler *endmsg =
      (upb_endmsg_handler *)upb_handlers_gethandler(h, UPB_ENDMSG_SELECTOR);
  if (endmsg) {
    const void *hd = upb_handlers_gethandlerdata(h, UPB_ENDMSG_SELECTOR);
    bool ok = endmsg(s->top->closure, hd, &s->pipeline_->status_);
    if (!ok) return false;
  }
  return true;
}

#define PUTVAL(type, ctype) \
  bool upb_sink_put ## type(upb_sink *s, upb_selector_t sel, ctype val) { \
    const upb_handlers *h = s->top->h; \
    upb_ ## type ## _handler *handler = (upb_ ## type ## _handler*) \
        upb_handlers_gethandler(h, sel); \
    if (handler) { \
      const void *hd = upb_handlers_gethandlerdata(h, sel); \
      bool ok = handler(s->top->closure, hd, val); \
      if (!ok) return false; \
    } \
    return true; \
  }

PUTVAL(int32,  int32_t);
PUTVAL(int64,  int64_t);
PUTVAL(uint32, uint32_t);
PUTVAL(uint64, uint64_t);
PUTVAL(float,  float);
PUTVAL(double, double);
PUTVAL(bool,   bool);
#undef PUTVAL

size_t upb_sink_putstring(upb_sink *s, upb_selector_t sel,
                          const char *buf, size_t n) {
  const upb_handlers *h = s->top->h;
  upb_string_handler *handler =
      (upb_string_handler*)upb_handlers_gethandler(h, sel);

  if (handler) {
    const void *hd = upb_handlers_gethandlerdata(h, sel);;
    n = handler(s->top->closure, hd, buf, n);
  }

  return n;
}

bool upb_sink_startseq(upb_sink *s, upb_selector_t sel) {
  if (!chkstack(s)) return false;

  void *subc = s->top->closure;
  const upb_handlers *h = s->top->h;
  upb_startfield_handler *startseq =
      (upb_startfield_handler*)upb_handlers_gethandler(h, sel);

  if (startseq) {
    const void *hd = upb_handlers_gethandlerdata(h, sel);
    subc = startseq(s->top->closure, hd);
    if (subc == UPB_BREAK) {
      return false;
    }
  }

  s->top->selector = upb_handlers_getendselector(sel);
  ++s->top;
  s->top->h = h;
  s->top->closure = subc;
  return true;
}

bool upb_sink_endseq(upb_sink *s, upb_selector_t sel) {
  --s->top;
  assert(sel == s->top->selector);

  const upb_handlers *h = s->top->h;
  upb_endfield_handler *endseq =
      (upb_endfield_handler*)upb_handlers_gethandler(h, sel);

  if (endseq) {
    const void *hd = upb_handlers_gethandlerdata(h, sel);
    bool ok = endseq(s->top->closure, hd);
    if (!ok) {
      ++s->top;
      return false;
    }
  }

  return true;
}

bool upb_sink_startstr(upb_sink *s, upb_selector_t sel, size_t size_hint) {
  if (!chkstack(s)) return false;

  void *subc = s->top->closure;
  const upb_handlers *h = s->top->h;
  upb_startstr_handler *startstr =
      (upb_startstr_handler*)upb_handlers_gethandler(h, sel);

  if (startstr) {
    const void *hd = upb_handlers_gethandlerdata(h, sel);
    subc = startstr(s->top->closure, hd, size_hint);
    if (subc == UPB_BREAK) {
      return false;
    }
  }

  s->top->selector = upb_handlers_getendselector(sel);
  ++s->top;
  s->top->h = h;
  s->top->closure = subc;
  return true;
}

bool upb_sink_endstr(upb_sink *s, upb_selector_t sel) {
  --s->top;
  assert(sel == s->top->selector);
  const upb_handlers *h = s->top->h;
  upb_endfield_handler *endstr =
      (upb_endfield_handler*)upb_handlers_gethandler(h, sel);

  if (endstr) {
    const void *hd = upb_handlers_gethandlerdata(h, sel);
    bool ok = endstr(s->top->closure, hd);
    if (!ok) {
      ++s->top;
      return false;
    }
  }

  return true;
}

bool upb_sink_startsubmsg(upb_sink *s, upb_selector_t sel) {
  if (!chkstack(s)) return false;

  void *subc = s->top->closure;
  const upb_handlers *h = s->top->h;
  upb_startfield_handler *startsubmsg =
      (upb_startfield_handler*)upb_handlers_gethandler(h, sel);

  if (startsubmsg) {
    const void *hd = upb_handlers_gethandlerdata(h, sel);
    subc = startsubmsg(s->top->closure, hd);
    if (subc == UPB_BREAK) {
      return false;
    }
  }

  s->top->selector= upb_handlers_getendselector(sel);
  ++s->top;
  s->top->h = upb_handlers_getsubhandlers_sel(h, sel);
  // TODO: should add support for submessages without any handlers
  assert(s->top->h);
  s->top->closure = subc;
  upb_sink_startmsg(s);
  return true;
}

bool upb_sink_endsubmsg(upb_sink *s, upb_selector_t sel) {
  upb_endmsg_handler *endmsg = (upb_endmsg_handler *)upb_handlers_gethandler(
      s->top->h, UPB_ENDMSG_SELECTOR);
  if (endmsg) {
    // TODO(haberman): check return value.
    const void *hd =
        upb_handlers_gethandlerdata(s->top->h, UPB_ENDMSG_SELECTOR);
    endmsg(s->top->closure, hd, &s->pipeline_->status_);
  }
  --s->top;

  assert(sel == s->top->selector);
  const upb_handlers *h = s->top->h;
  upb_endfield_handler *endsubmsg =
      (upb_endfield_handler*)upb_handlers_gethandler(h, sel);

  if (endsubmsg) {
    const void *hd = upb_handlers_gethandlerdata(h, sel);
    bool ok = endsubmsg(s->top->closure, hd);
    if (!ok) {
      ++s->top;
      return false;
    }
  }

  return true;
}

const upb_handlers *upb_sink_tophandlers(upb_sink *s) {
  return s->top->h;
}
