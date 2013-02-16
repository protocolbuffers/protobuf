/*
 * upb - a minimalist implementation of protocol buffers.
 *
 * Copyright (c) 2011-2012 Google Inc.  See LICENSE for details.
 * Author: Josh Haberman <jhaberman@gmail.com>
 */

#include "upb/handlers.h"

#include <stdlib.h>
#include <string.h>

// Defined for the sole purpose of having a unique pointer value for
// UPB_NO_CLOSURE.
char _upb_noclosure;

typedef struct {
  upb_func *handler;

  // Could put either or both of these in a separate table to save memory when
  // they are sparse.
  void *data;
  upb_handlerfree *cleanup;

  // TODO(haberman): this is wasteful; only the first "fieldhandler" of a
  // submessage field needs this.  To reduce memory footprint we should either:
  // - put the subhandlers in a separate "fieldhandler", stored as part of
  //   a union with one of the above fields.
  // - count selector offsets by individual pointers instead of by whole
  //   fieldhandlers.
  const upb_handlers *subhandlers;
} fieldhandler;

static const fieldhandler *getfh(
    const upb_handlers *h, upb_selector_t selector) {
  assert(selector < upb_handlers_msgdef(h)->selector_count);
  fieldhandler* fhbase = (void*)&h->fh_base;
  return &fhbase[selector];
}

static fieldhandler *getfh_mutable(upb_handlers *h, upb_selector_t selector) {
  return (fieldhandler*)getfh(h, selector);
}

bool upb_handlers_isfrozen(const upb_handlers *h) {
  return upb_refcounted_isfrozen(upb_upcast(h));
}

uint32_t upb_handlers_selectorbaseoffset(const upb_fielddef *f) {
  return upb_fielddef_isseq(f) ? 2 : 0;
}

uint32_t upb_handlers_selectorcount(const upb_fielddef *f) {
  uint32_t ret = 1;
  if (upb_fielddef_isstring(f)) ret += 2;  // STARTSTR/ENDSTR
  if (upb_fielddef_isseq(f)) ret += 2;  // STARTSEQ/ENDSEQ
  if (upb_fielddef_issubmsg(f)) ret += 2;  // STARTSUBMSG/ENDSUBMSG
  return ret;
}

upb_handlertype_t upb_handlers_getprimitivehandlertype(const upb_fielddef *f) {
  switch (upb_fielddef_type(f)) {
    case UPB_TYPE_INT32:
    case UPB_TYPE_SINT32:
    case UPB_TYPE_SFIXED32:
    case UPB_TYPE_ENUM:
      return UPB_HANDLER_INT32;
    case UPB_TYPE_INT64:
    case UPB_TYPE_SINT64:
    case UPB_TYPE_SFIXED64:
      return UPB_HANDLER_INT64;
    case UPB_TYPE_UINT32:
    case UPB_TYPE_FIXED32:
      return UPB_HANDLER_UINT32;
    case UPB_TYPE_UINT64:
    case UPB_TYPE_FIXED64:
      return UPB_HANDLER_UINT64;
    case UPB_TYPE_FLOAT:
      return UPB_HANDLER_FLOAT;
    case UPB_TYPE_DOUBLE:
      return UPB_HANDLER_DOUBLE;
    case UPB_TYPE_BOOL:
      return UPB_HANDLER_BOOL;
    default: assert(false); return -1;  // Invalid input.
  }
}

bool upb_getselector(
    const upb_fielddef *f, upb_handlertype_t type, upb_selector_t *s) {
  // If the type checks in this function are a hot-spot, we can introduce a
  // separate function that calculates the selector assuming that the type
  // is correct (may even want to make it inline for the upb_sink fast-path.
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
    case UPB_HANDLER_STARTSTR:
      if (!upb_fielddef_isstring(f)) return false;
      *s = f->selector_base;
      break;
    case UPB_HANDLER_STRING:
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
      *s = f->selector_base + 1;
      break;
    case UPB_HANDLER_ENDSUBMSG:
      if (!upb_fielddef_issubmsg(f)) return false;
      *s = f->selector_base + 2;
      break;
  }
  assert(*s < upb_fielddef_msgdef(f)->selector_count);
  return true;
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

static void do_cleanup(upb_handlers* h, const upb_fielddef *f,
                       upb_handlertype_t type) {
  upb_selector_t selector;
  if (!upb_getselector(f, type, &selector)) return;
  fieldhandler *fh = getfh_mutable(h, selector);
  if (fh->cleanup) fh->cleanup(fh->data);
  fh->cleanup = NULL;
  fh->data = NULL;
}

static void freehandlers(upb_refcounted *r) {
  upb_handlers *h = (upb_handlers*)r;
  upb_msg_iter i;
  for(upb_msg_begin(&i, h->msg); !upb_msg_done(&i); upb_msg_next(&i)) {
    upb_fielddef *f = upb_msg_iter_field(&i);
    for (upb_handlertype_t type = 0; type < UPB_HANDLER_MAX; type++)
      do_cleanup(h, f, type);
  }
  upb_msgdef_unref(h->msg, h);
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

upb_handlers *upb_handlers_new(const upb_msgdef *md, const void *owner) {
  assert(upb_msgdef_isfrozen(md));
  static const struct upb_refcounted_vtbl vtbl = {visithandlers, freehandlers};
  size_t fhandlers_size = sizeof(fieldhandler) * md->selector_count;
  upb_handlers *h = calloc(sizeof(*h) - sizeof(void*) + fhandlers_size, 1);
  if (!h) return NULL;
  h->msg = md;
  upb_msgdef_ref(h->msg, h);
  if (!upb_refcounted_init(upb_upcast(h), &vtbl, owner)) goto oom;

  // calloc() above initialized all handlers to NULL.
  return h;

oom:
  freehandlers(upb_upcast(h));
  return NULL;
}

bool upb_handlers_freeze(upb_handlers *const*handlers, int n, upb_status *s) {
  // TODO: verify we have a transitive closure.
  return upb_refcounted_freeze((upb_refcounted*const*)handlers, n, s);
}

const upb_msgdef *upb_handlers_msgdef(const upb_handlers *h) { return h->msg; }

void upb_handlers_setstartmsg(upb_handlers *h, upb_startmsg_handler *handler) {
  assert(!upb_handlers_isfrozen(h));
  h->startmsg = handler;
}

upb_startmsg_handler *upb_handlers_getstartmsg(const upb_handlers *h) {
  return h->startmsg;
}

void upb_handlers_setendmsg(upb_handlers *h, upb_endmsg_handler *handler) {
  assert(!upb_handlers_isfrozen(h));
  h->endmsg = handler;
}

upb_endmsg_handler *upb_handlers_getendmsg(const upb_handlers *h) {
  return h->endmsg;
}

// For now we stuff the subhandlers pointer into the fieldhandlers*
// corresponding to the UPB_HANDLER_STARTSUBMSG handler.
static const upb_handlers **subhandlersptr(upb_handlers *h,
                                           const upb_fielddef *f) {
  assert(upb_fielddef_issubmsg(f));
  upb_selector_t selector;
  bool ok = upb_getselector(f, UPB_HANDLER_STARTSUBMSG, &selector);
  UPB_ASSERT_VAR(ok, ok);
  return &getfh_mutable(h, selector)->subhandlers;
}

bool upb_handlers_setsubhandlers(upb_handlers *h, const upb_fielddef *f,
                                 const upb_handlers *sub) {
  assert(!upb_handlers_isfrozen(h));
  if (!upb_fielddef_issubmsg(f)) return false;
  if (sub != NULL &&
      upb_upcast(upb_handlers_msgdef(sub)) != upb_fielddef_subdef(f)) {
    return false;
  }
  const upb_handlers **stored = subhandlersptr(h, f);
  const upb_handlers *old = *stored;
  if (old) upb_unref2(old, h);
  *stored = sub;
  if (sub) upb_ref2(sub, h);
  return true;
}

const upb_handlers *upb_handlers_getsubhandlers(const upb_handlers *h,
                                                const upb_fielddef *f) {
  const upb_handlers **stored = subhandlersptr((upb_handlers*)h, f);
  return *stored;
}

#define SETTER(name, handlerctype, handlertype) \
  bool upb_handlers_set ## name(upb_handlers *h, const upb_fielddef *f, \
                                handlerctype val, void *data, \
                                upb_handlerfree *cleanup) { \
    assert(!upb_handlers_isfrozen(h)); \
    if (upb_handlers_msgdef(h) != upb_fielddef_msgdef(f)) return false; \
    upb_selector_t selector; \
    bool ok = upb_getselector(f, handlertype, &selector); \
    if (!ok) return false; \
    do_cleanup(h, f, handlertype); \
    fieldhandler *fh = getfh_mutable(h, selector); \
    fh->handler = (upb_func*)val; \
    fh->data = (upb_func*)data; \
    fh->cleanup = (upb_func*)cleanup; \
    return true; \
  } \

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

upb_func *upb_handlers_gethandler(const upb_handlers *h, upb_selector_t s) {
  return getfh(h, s)->handler;
}

void *upb_handlers_gethandlerdata(const upb_handlers *h, upb_selector_t s) {
  return getfh(h, s)->data;
}

typedef struct {
  upb_inttable tab;  // maps upb_msgdef* -> upb_handlers*.
  upb_handlers_callback *callback;
  void *closure;
} dfs_state;

static upb_handlers *newformsg(const upb_msgdef *m, const void *owner,
                               dfs_state *s) {
  upb_handlers *h = upb_handlers_new(m, owner);
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
    const upb_value *subm_ent = upb_inttable_lookupptr(&s->tab, subdef);
    if (subm_ent) {
      upb_handlers_setsubhandlers(h, f, upb_value_getptr(*subm_ent));
    } else {
      upb_handlers *sub_mh = newformsg(subdef, &sub_mh, s);
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

const upb_handlers *upb_handlers_newfrozen(const upb_msgdef *m,
                                           const void *owner,
                                           upb_handlers_callback *callback,
                                           void *closure) {
  dfs_state state;
  state.callback = callback;
  state.closure = closure;
  if (!upb_inttable_init(&state.tab, UPB_CTYPE_PTR)) return NULL;

  upb_handlers *ret = newformsg(m, owner, &state);
  if (!ret) return NULL;
  upb_refcounted *r = upb_upcast(ret);
  upb_status status = UPB_STATUS_INIT;
  bool ok = upb_refcounted_freeze(&r, 1, &status);
  UPB_ASSERT_VAR(ok, ok);
  upb_status_uninit(&status);

  upb_inttable_uninit(&state.tab);
  return ret;
}

#define STDMSG_WRITER(type, ctype)                                            \
  bool upb_stdmsg_set ## type (void *_m, void *fval, ctype val) {             \
    assert(_m != NULL);                                                       \
    const upb_stdmsg_fval *f = fval;                                          \
    uint8_t *m = _m;                                                          \
    if (f->hasbit > 0)                                                        \
      *(uint8_t*)&m[f->hasbit / 8] |= 1 << (f->hasbit % 8);                   \
    *(ctype*)&m[f->offset] = val;                                             \
    return true;                                                              \
  }                                                                           \

STDMSG_WRITER(double, double)
STDMSG_WRITER(float, float)
STDMSG_WRITER(int32, int32_t)
STDMSG_WRITER(int64, int64_t)
STDMSG_WRITER(uint32, uint32_t)
STDMSG_WRITER(uint64, uint64_t)
STDMSG_WRITER(bool, bool)
#undef STDMSG_WRITER
