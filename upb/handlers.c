/*
 * upb - a minimalist implementation of protocol buffers.
 *
 * Copyright (c) 2011-2012 Google Inc.  See LICENSE for details.
 * Author: Josh Haberman <jhaberman@gmail.com>
 *
 * TODO(haberman): it's unclear whether a lot of the consistency checks should
 * assert() or return false.
 */

#include "upb/handlers.h"

#include <stdlib.h>
#include <string.h>

#include "upb/sink.h"

// Defined for the sole purpose of having a unique pointer value for
// UPB_NO_CLOSURE.
char _upb_noclosure;

typedef struct {
  void (*func)();
  void *data;
} tabent;

static void freehandlers(upb_refcounted *r) {
  upb_handlers *h = (upb_handlers*)r;
  upb_msgdef_unref(h->msg, h);
  for (size_t i = 0; i < h->cleanup_len; i++) {
    h->cleanup[i].cleanup(h->cleanup[i].ptr);
  }
  if (h->status_) {
    upb_status_uninit(h->status_);
    free(h->status_);
  }
  free(h->cleanup);
  free(h);
}

static void visithandlers(const upb_refcounted *r, upb_refcounted_visit *visit,
                          void *closure) {
  const upb_handlers *h = (const upb_handlers*)r;
  upb_msg_iter i;
  for(upb_msg_begin(&i, h->msg); !upb_msg_done(&i); upb_msg_next(&i)) {
    upb_fielddef *f = upb_msg_iter_field(&i);
    if (!upb_fielddef_issubmsg(f)) continue;
    const upb_handlers *sub = upb_handlers_getsubhandlers(h, f);
    if (sub) visit(r, upb_upcast(sub), closure);
  }
}

static const struct upb_refcounted_vtbl vtbl = {visithandlers, freehandlers};

typedef struct {
  upb_inttable tab;  // maps upb_msgdef* -> upb_handlers*.
  upb_handlers_callback *callback;
  void *closure;
} dfs_state;

static upb_handlers *newformsg(const upb_msgdef *m, const upb_frametype *ft,
                               const void *owner,
                               dfs_state *s) {
  upb_handlers *h = upb_handlers_new(m, ft, owner);
  if (!h) return NULL;
  if (!upb_inttable_insertptr(&s->tab, m, upb_value_ptr(h))) goto oom;

  s->callback(s->closure, h);

  // For each submessage field, get or create a handlers object and set it as
  // the subhandlers.
  upb_msg_iter i;
  for(upb_msg_begin(&i, m); !upb_msg_done(&i); upb_msg_next(&i)) {
    upb_fielddef *f = upb_msg_iter_field(&i);
    if (!upb_fielddef_issubmsg(f)) continue;

    const upb_msgdef *subdef = upb_downcast_msgdef(upb_fielddef_subdef(f));
    upb_value subm_ent;
    if (upb_inttable_lookupptr(&s->tab, subdef, &subm_ent)) {
      upb_handlers_setsubhandlers(h, f, upb_value_getptr(subm_ent));
    } else {
      upb_handlers *sub_mh = newformsg(subdef, ft, &sub_mh, s);
      if (!sub_mh) goto oom;
      upb_handlers_setsubhandlers(h, f, sub_mh);
      upb_handlers_unref(sub_mh, &sub_mh);
    }
  }
  return h;

oom:
  upb_handlers_unref(h, owner);
  return NULL;
}

// This wastes a bit of space since the "func" member of this slot is unused,
// but the code is simpler.  Worst-case overhead is 20% (messages with only
// non-repeated submessage fields).  Can change later if necessary.
#define SUBH(h, field_base) h->table[field_base + 2].data

static int32_t getsel(upb_handlers *h, const upb_fielddef *f,
                      upb_handlertype_t type) {
  upb_selector_t sel;
  assert(!upb_handlers_isfrozen(h));
  if (upb_handlers_msgdef(h) != upb_fielddef_msgdef(f)) {
    upb_status_seterrf(
        h->status_, "type mismatch: field %s does not belong to message %s",
        upb_fielddef_name(f), upb_msgdef_fullname(upb_handlers_msgdef(h)));
    return -1;
  }
  if (!upb_handlers_getselector(f, type, &sel)) {
    upb_status_seterrf(
        h->status_,
        "type mismatch: cannot register handler type %d for field %s",
        type, upb_fielddef_name(f));
    return -1;
  }
  return sel;
}

static bool addcleanup(upb_handlers *h, void *ptr, void (*cleanup)(void*)) {
  if (h->cleanup_len == h->cleanup_size) {
    h->cleanup_size = UPB_MAX(4, h->cleanup_size * 2);
    void *resized = realloc(h->cleanup, sizeof(*h->cleanup) * h->cleanup_size);
    if (!resized) {
      h->cleanup_size = h->cleanup_len;
      cleanup(ptr);
      upb_status_seterrliteral(h->status_, "out of memory");
      return false;
    }
    h->cleanup = resized;
  }
  h->cleanup[h->cleanup_len].ptr = ptr;
  h->cleanup[h->cleanup_len].cleanup = cleanup;
  h->cleanup_len++;
  return true;
}

static bool doset(upb_handlers *h, upb_selector_t sel, upb_func *func,
                  void *data, upb_handlerfree *cleanup) {
  assert(!upb_handlers_isfrozen(h));
  if (h->table[sel].func) {
    upb_status_seterrliteral(h->status_,
                             "cannot change handler once it has been set.");
    return false;
  }
  if (cleanup && !addcleanup(h, data, cleanup)) return false;
  h->table[sel].func = (upb_func*)func;
  h->table[sel].data = data;
  return true;
}


/* Public interface ***********************************************************/

bool upb_handlers_isfrozen(const upb_handlers *h) {
  return upb_refcounted_isfrozen(upb_upcast(h));
}

void upb_handlers_ref(const upb_handlers *h, const void *owner) {
  upb_refcounted_ref(upb_upcast(h), owner);
}

void upb_handlers_unref(const upb_handlers *h, const void *owner) {
  upb_refcounted_unref(upb_upcast(h), owner);
}

void upb_handlers_donateref(
    const upb_handlers *h, const void *from, const void *to) {
  upb_refcounted_donateref(upb_upcast(h), from, to);
}

void upb_handlers_checkref(const upb_handlers *h, const void *owner) {
  upb_refcounted_checkref(upb_upcast(h), owner);
}


upb_handlers *upb_handlers_new(const upb_msgdef *md, const upb_frametype *ft,
                               const void *owner) {
  assert(upb_msgdef_isfrozen(md));

  int extra = sizeof(upb_handlers_tabent) * (md->selector_count - 1 + 100);
  upb_handlers *h = calloc(sizeof(*h) + extra, 1);
  if (!h) return NULL;

  h->status_ = malloc(sizeof(*h->status_));
  if (!h->status_) goto oom;
  upb_status_init(h->status_);

  h->msg = md;
  h->ft = ft;
  h->cleanup = NULL;
  h->cleanup_size = 0;
  h->cleanup_len = 0;
  upb_msgdef_ref(h->msg, h);
  if (!upb_refcounted_init(upb_upcast(h), &vtbl, owner)) goto oom;

  // calloc() above initialized all handlers to NULL.
  return h;

oom:
  freehandlers(upb_upcast(h));
  return NULL;
}

const upb_handlers *upb_handlers_newfrozen(const upb_msgdef *m,
                                           const upb_frametype *ft,
                                           const void *owner,
                                           upb_handlers_callback *callback,
                                           void *closure) {
  dfs_state state;
  state.callback = callback;
  state.closure = closure;
  if (!upb_inttable_init(&state.tab, UPB_CTYPE_PTR)) return NULL;

  upb_handlers *ret = newformsg(m, ft, owner, &state);

  upb_inttable_uninit(&state.tab);
  if (!ret) return NULL;

  upb_refcounted *r = upb_upcast(ret);
  bool ok = upb_refcounted_freeze(&r, 1, NULL);
  UPB_ASSERT_VAR(ok, ok);

  return ret;
}

const upb_status *upb_handlers_status(upb_handlers *h) {
  assert(!upb_handlers_isfrozen(h));
  return h->status_;
}

void upb_handlers_clearerr(upb_handlers *h) {
  assert(!upb_handlers_isfrozen(h));
  upb_status_clear(h->status_);
}

#define SETTER(name, handlerctype, handlertype) \
  bool upb_handlers_set ## name(upb_handlers *h, const upb_fielddef *f, \
                                handlerctype func, void *data, \
                                upb_handlerfree *cleanup) { \
    int32_t sel = getsel(h, f, handlertype); \
    return sel >= 0 && doset(h, sel, (upb_func*)func, data, cleanup); \
  }

SETTER(int32,       upb_int32_handler*,       UPB_HANDLER_INT32);
SETTER(int64,       upb_int64_handler*,       UPB_HANDLER_INT64);
SETTER(uint32,      upb_uint32_handler*,      UPB_HANDLER_UINT32);
SETTER(uint64,      upb_uint64_handler*,      UPB_HANDLER_UINT64);
SETTER(float,       upb_float_handler*,       UPB_HANDLER_FLOAT);
SETTER(double,      upb_double_handler*,      UPB_HANDLER_DOUBLE);
SETTER(bool,        upb_bool_handler*,        UPB_HANDLER_BOOL);
SETTER(startstr,    upb_startstr_handler*,    UPB_HANDLER_STARTSTR);
SETTER(string,      upb_string_handler*,      UPB_HANDLER_STRING);
SETTER(endstr,      upb_endfield_handler*,    UPB_HANDLER_ENDSTR);
SETTER(startseq,    upb_startfield_handler*,  UPB_HANDLER_STARTSEQ);
SETTER(startsubmsg, upb_startfield_handler*,  UPB_HANDLER_STARTSUBMSG);
SETTER(endsubmsg,   upb_endfield_handler*,    UPB_HANDLER_ENDSUBMSG);
SETTER(endseq,      upb_endfield_handler*,    UPB_HANDLER_ENDSEQ);

#undef SETTER

bool upb_handlers_setstartmsg(upb_handlers *h, upb_startmsg_handler *handler,
                              void *d, upb_handlerfree *cleanup) {
  return doset(h, UPB_STARTMSG_SELECTOR, (upb_func*)handler, d, cleanup);
}

bool upb_handlers_setendmsg(upb_handlers *h, upb_endmsg_handler *handler,
                            void *d, upb_handlerfree *cleanup) {
  assert(!upb_handlers_isfrozen(h));
  return doset(h, UPB_ENDMSG_SELECTOR, (upb_func*)handler, d, cleanup);
}

bool upb_handlers_setsubhandlers(upb_handlers *h, const upb_fielddef *f,
                                 const upb_handlers *sub) {
  assert(sub);
  assert(!upb_handlers_isfrozen(h));
  assert(upb_fielddef_issubmsg(f));
  if (SUBH(h, f->selector_base)) return false;  // Can't reset.
  if (upb_upcast(upb_handlers_msgdef(sub)) != upb_fielddef_subdef(f)) {
    return false;
  }
  SUBH(h, f->selector_base) = sub;
  upb_ref2(sub, h);
  return true;
}

const upb_handlers *upb_handlers_getsubhandlers(const upb_handlers *h,
                                                const upb_fielddef *f) {
  assert(upb_fielddef_issubmsg(f));
  return SUBH(h, f->selector_base);
}

const upb_handlers *upb_handlers_getsubhandlers_sel(const upb_handlers *h,
                                                    upb_selector_t sel) {
  // STARTSUBMSG selector in sel is the field's selector base.
  return SUBH(h, sel);
}

const upb_msgdef *upb_handlers_msgdef(const upb_handlers *h) { return h->msg; }

const upb_frametype *upb_handlers_frametype(const upb_handlers *h) {
  return h->ft;
}

upb_func *upb_handlers_gethandler(const upb_handlers *h, upb_selector_t s) {
  return (upb_func *)h->table[s].func;
}

const void *upb_handlers_gethandlerdata(const upb_handlers *h,
                                        upb_selector_t s) {
  return h->table[s].data;
}

/* "Static" methods ***********************************************************/

bool upb_handlers_freeze(upb_handlers *const*handlers, int n, upb_status *s) {
  // TODO: verify we have a transitive closure.
  for (int i = 0; i < n; i++) {
    if (!upb_ok(handlers[i]->status_)) {
      upb_status_seterrf(s, "handlers for message %s had error status: %s",
                         upb_msgdef_fullname(upb_handlers_msgdef(handlers[i])),
                         upb_status_getstr(handlers[i]->status_));
      return false;
    }
  }

  if (!upb_refcounted_freeze((upb_refcounted*const*)handlers, n, s)) {
    return false;
  }

  for (int i = 0; i < n; i++) {
    upb_status_uninit(handlers[i]->status_);
    free(handlers[i]->status_);
    handlers[i]->status_ = NULL;
  }
  return true;
}

upb_handlertype_t upb_handlers_getprimitivehandlertype(const upb_fielddef *f) {
  switch (upb_fielddef_type(f)) {
    case UPB_TYPE_INT32:
    case UPB_TYPE_ENUM: return UPB_HANDLER_INT32;
    case UPB_TYPE_INT64: return UPB_HANDLER_INT64;
    case UPB_TYPE_UINT32: return UPB_HANDLER_UINT32;
    case UPB_TYPE_UINT64: return UPB_HANDLER_UINT64;
    case UPB_TYPE_FLOAT: return UPB_HANDLER_FLOAT;
    case UPB_TYPE_DOUBLE: return UPB_HANDLER_DOUBLE;
    case UPB_TYPE_BOOL: return UPB_HANDLER_BOOL;
    default: assert(false); return -1;  // Invalid input.
  }
}

bool upb_handlers_getselector(const upb_fielddef *f, upb_handlertype_t type,
                              upb_selector_t *s) {
  switch (type) {
    case UPB_HANDLER_INT32:
    case UPB_HANDLER_INT64:
    case UPB_HANDLER_UINT32:
    case UPB_HANDLER_UINT64:
    case UPB_HANDLER_FLOAT:
    case UPB_HANDLER_DOUBLE:
    case UPB_HANDLER_BOOL:
      if (!upb_fielddef_isprimitive(f) ||
          upb_handlers_getprimitivehandlertype(f) != type)
        return false;
      *s = f->selector_base;
      break;
    case UPB_HANDLER_STRING:
      if (!upb_fielddef_isstring(f)) return false;
      *s = f->selector_base;
      break;
    case UPB_HANDLER_STARTSTR:
      if (!upb_fielddef_isstring(f)) return false;
      *s = f->selector_base + 1;
      break;
    case UPB_HANDLER_ENDSTR:
      if (!upb_fielddef_isstring(f)) return false;
      *s = f->selector_base + 2;
      break;
    case UPB_HANDLER_STARTSEQ:
      if (!upb_fielddef_isseq(f)) return false;
      *s = f->selector_base - 2;
      break;
    case UPB_HANDLER_ENDSEQ:
      if (!upb_fielddef_isseq(f)) return false;
      *s = f->selector_base - 1;
      break;
    case UPB_HANDLER_STARTSUBMSG:
      if (!upb_fielddef_issubmsg(f)) return false;
      *s = f->selector_base;
      break;
    case UPB_HANDLER_ENDSUBMSG:
      if (!upb_fielddef_issubmsg(f)) return false;
      *s = f->selector_base + 1;
      break;
    // Subhandler slot is selector_base + 2.
  }
  assert(*s < upb_fielddef_msgdef(f)->selector_count);
  return true;
}

uint32_t upb_handlers_selectorbaseoffset(const upb_fielddef *f) {
  return upb_fielddef_isseq(f) ? 2 : 0;
}

uint32_t upb_handlers_selectorcount(const upb_fielddef *f) {
  uint32_t ret = 1;
  if (upb_fielddef_isseq(f)) ret += 2;    // STARTSEQ/ENDSEQ
  if (upb_fielddef_isstring(f)) ret += 2; // [STARTSTR]/STRING/ENDSTR
  if (upb_fielddef_issubmsg(f)) ret += 2;   // [STARTSUBMSG]/ENDSUBMSG/SUBH
  return ret;
}
