/*
** TODO(haberman): it's unclear whether a lot of the consistency checks should
** UPB_ASSERT() or return false.
*/

#include "upb/handlers.h"

#include <string.h>

#include "upb/sink.h"

#include "upb/port_def.inc"

struct upb_handlers {
  upb_handlercache *cache;
  const upb_msgdef *msg;
  const upb_handlers **sub;
  const void *top_closure_type;
  upb_handlers_tabent table[1];  /* Dynamically-sized field handler array. */
};

static void *upb_calloc(upb_arena *arena, size_t size) {
  void *mem = upb_malloc(upb_arena_alloc(arena), size);
  if (mem) {
    memset(mem, 0, size);
  }
  return mem;
}

/* Defined for the sole purpose of having a unique pointer value for
 * UPB_NO_CLOSURE. */
char _upb_noclosure;

/* Given a selector for a STARTSUBMSG handler, resolves to a pointer to the
 * subhandlers for this submessage field. */
#define SUBH(h, selector) (h->sub[selector])

/* The selector for a submessage field is the field index. */
#define SUBH_F(h, f) SUBH(h, upb_fielddef_index(f))

static int32_t trygetsel(upb_handlers *h, const upb_fielddef *f,
                         upb_handlertype_t type) {
  upb_selector_t sel;
  bool ok;

  ok = upb_handlers_getselector(f, type, &sel);

  UPB_ASSERT(upb_handlers_msgdef(h) == upb_fielddef_containingtype(f));
  UPB_ASSERT(ok);

  return sel;
}

static upb_selector_t handlers_getsel(upb_handlers *h, const upb_fielddef *f,
                             upb_handlertype_t type) {
  int32_t sel = trygetsel(h, f, type);
  UPB_ASSERT(sel >= 0);
  return sel;
}

static const void **returntype(upb_handlers *h, const upb_fielddef *f,
                               upb_handlertype_t type) {
  return &h->table[handlers_getsel(h, f, type)].attr.return_closure_type;
}

static bool doset(upb_handlers *h, int32_t sel, const upb_fielddef *f,
                  upb_handlertype_t type, upb_func *func,
                  const upb_handlerattr *attr) {
  upb_handlerattr set_attr = UPB_HANDLERATTR_INIT;
  const void *closure_type;
  const void **context_closure_type;

  UPB_ASSERT(!h->table[sel].func);

  if (attr) {
    set_attr = *attr;
  }

  /* Check that the given closure type matches the closure type that has been
   * established for this context (if any). */
  closure_type = set_attr.closure_type;

  if (type == UPB_HANDLER_STRING) {
    context_closure_type = returntype(h, f, UPB_HANDLER_STARTSTR);
  } else if (f && upb_fielddef_isseq(f) &&
             type != UPB_HANDLER_STARTSEQ &&
             type != UPB_HANDLER_ENDSEQ) {
    context_closure_type = returntype(h, f, UPB_HANDLER_STARTSEQ);
  } else {
    context_closure_type = &h->top_closure_type;
  }

  if (closure_type && *context_closure_type &&
      closure_type != *context_closure_type) {
    return false;
  }

  if (closure_type)
    *context_closure_type = closure_type;

  /* If this is a STARTSEQ or STARTSTR handler, check that the returned pointer
   * matches any pre-existing expectations about what type is expected. */
  if (type == UPB_HANDLER_STARTSEQ || type == UPB_HANDLER_STARTSTR) {
    const void *return_type = set_attr.return_closure_type;
    const void *table_return_type = h->table[sel].attr.return_closure_type;
    if (return_type && table_return_type && return_type != table_return_type) {
      return false;
    }

    if (table_return_type && !return_type) {
      set_attr.return_closure_type = table_return_type;
    }
  }

  h->table[sel].func = (upb_func*)func;
  h->table[sel].attr = set_attr;
  return true;
}

/* Returns the effective closure type for this handler (which will propagate
 * from outer frames if this frame has no START* handler).  Not implemented for
 * UPB_HANDLER_STRING at the moment since this is not needed.  Returns NULL is
 * the effective closure type is unspecified (either no handler was registered
 * to specify it or the handler that was registered did not specify the closure
 * type). */
const void *effective_closure_type(upb_handlers *h, const upb_fielddef *f,
                                   upb_handlertype_t type) {
  const void *ret;
  upb_selector_t sel;

  UPB_ASSERT(type != UPB_HANDLER_STRING);
  ret = h->top_closure_type;

  if (upb_fielddef_isseq(f) &&
      type != UPB_HANDLER_STARTSEQ &&
      type != UPB_HANDLER_ENDSEQ &&
      h->table[sel = handlers_getsel(h, f, UPB_HANDLER_STARTSEQ)].func) {
    ret = h->table[sel].attr.return_closure_type;
  }

  if (type == UPB_HANDLER_STRING &&
      h->table[sel = handlers_getsel(h, f, UPB_HANDLER_STARTSTR)].func) {
    ret = h->table[sel].attr.return_closure_type;
  }

  /* The effective type of the submessage; not used yet.
   * if (type == SUBMESSAGE &&
   *     h->table[sel = handlers_getsel(h, f, UPB_HANDLER_STARTSUBMSG)].func) {
   *   ret = h->table[sel].attr.return_closure_type;
   * } */

  return ret;
}

static upb_handlers *upb_handlers_new(const upb_msgdef *md,
                                      upb_handlercache *cache,
                                      upb_arena *arena) {
  int extra;
  upb_handlers *h;

  extra =
      (int)(sizeof(upb_handlers_tabent) * (upb_msgdef_selectorcount(md) - 1));
  h = upb_calloc(arena, sizeof(*h) + extra);
  if (!h) return NULL;

  h->cache = cache;
  h->msg = md;

  if (upb_msgdef_submsgfieldcount(md) > 0) {
    size_t bytes = upb_msgdef_submsgfieldcount(md) * sizeof(*h->sub);
    h->sub = upb_calloc(arena, bytes);
    if (!h->sub) return NULL;
  } else {
    h->sub = 0;
  }

  /* calloc() above initialized all handlers to NULL. */
  return h;
}

/* Public interface ***********************************************************/

#define SETTER(name, handlerctype, handlertype)                       \
  bool upb_handlers_set##name(upb_handlers *h, const upb_fielddef *f, \
                              handlerctype func,                      \
                              const upb_handlerattr *attr) {          \
    int32_t sel = trygetsel(h, f, handlertype);                       \
    return doset(h, sel, f, handlertype, (upb_func *)func, attr);     \
  }

SETTER(int32,       upb_int32_handlerfunc*,       UPB_HANDLER_INT32)
SETTER(int64,       upb_int64_handlerfunc*,       UPB_HANDLER_INT64)
SETTER(uint32,      upb_uint32_handlerfunc*,      UPB_HANDLER_UINT32)
SETTER(uint64,      upb_uint64_handlerfunc*,      UPB_HANDLER_UINT64)
SETTER(float,       upb_float_handlerfunc*,       UPB_HANDLER_FLOAT)
SETTER(double,      upb_double_handlerfunc*,      UPB_HANDLER_DOUBLE)
SETTER(bool,        upb_bool_handlerfunc*,        UPB_HANDLER_BOOL)
SETTER(startstr,    upb_startstr_handlerfunc*,    UPB_HANDLER_STARTSTR)
SETTER(string,      upb_string_handlerfunc*,      UPB_HANDLER_STRING)
SETTER(endstr,      upb_endfield_handlerfunc*,    UPB_HANDLER_ENDSTR)
SETTER(startseq,    upb_startfield_handlerfunc*,  UPB_HANDLER_STARTSEQ)
SETTER(startsubmsg, upb_startfield_handlerfunc*,  UPB_HANDLER_STARTSUBMSG)
SETTER(endsubmsg,   upb_endfield_handlerfunc*,    UPB_HANDLER_ENDSUBMSG)
SETTER(endseq,      upb_endfield_handlerfunc*,    UPB_HANDLER_ENDSEQ)

#undef SETTER

bool upb_handlers_setunknown(upb_handlers *h, upb_unknown_handlerfunc *func,
                             const upb_handlerattr *attr) {
  return doset(h, UPB_UNKNOWN_SELECTOR, NULL, UPB_HANDLER_INT32,
               (upb_func *)func, attr);
}

bool upb_handlers_setstartmsg(upb_handlers *h, upb_startmsg_handlerfunc *func,
                              const upb_handlerattr *attr) {
  return doset(h, UPB_STARTMSG_SELECTOR, NULL, UPB_HANDLER_INT32,
               (upb_func *)func, attr);
}

bool upb_handlers_setendmsg(upb_handlers *h, upb_endmsg_handlerfunc *func,
                            const upb_handlerattr *attr) {
  return doset(h, UPB_ENDMSG_SELECTOR, NULL, UPB_HANDLER_INT32,
               (upb_func *)func, attr);
}

bool upb_handlers_setsubhandlers(upb_handlers *h, const upb_fielddef *f,
                                 const upb_handlers *sub) {
  UPB_ASSERT(sub);
  UPB_ASSERT(upb_fielddef_issubmsg(f));
  if (SUBH_F(h, f)) return false;  /* Can't reset. */
  if (upb_handlers_msgdef(sub) != upb_fielddef_msgsubdef(f)) {
    return false;
  }
  SUBH_F(h, f) = sub;
  return true;
}

const upb_handlers *upb_handlers_getsubhandlers(const upb_handlers *h,
                                                const upb_fielddef *f) {
  UPB_ASSERT(upb_fielddef_issubmsg(f));
  return SUBH_F(h, f);
}

upb_func *upb_handlers_gethandler(const upb_handlers *h, upb_selector_t s,
                                  const void **handler_data) {
  upb_func *ret = (upb_func *)h->table[s].func;
  if (ret && handler_data) {
    *handler_data = h->table[s].attr.handler_data;
  }
  return ret;
}

bool upb_handlers_getattr(const upb_handlers *h, upb_selector_t sel,
                          upb_handlerattr *attr) {
  if (!upb_handlers_gethandler(h, sel, NULL))
    return false;
  *attr = h->table[sel].attr;
  return true;
}

const upb_handlers *upb_handlers_getsubhandlers_sel(const upb_handlers *h,
                                                    upb_selector_t sel) {
  /* STARTSUBMSG selector in sel is the field's selector base. */
  return SUBH(h, sel - UPB_STATIC_SELECTOR_COUNT);
}

const upb_msgdef *upb_handlers_msgdef(const upb_handlers *h) { return h->msg; }

bool upb_handlers_addcleanup(upb_handlers *h, void *p, upb_handlerfree *func) {
  return upb_handlercache_addcleanup(h->cache, p, func);
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
    default: UPB_ASSERT(false); return -1;  /* Invalid input. */
  }
}

bool upb_handlers_getselector(const upb_fielddef *f, upb_handlertype_t type,
                              upb_selector_t *s) {
  uint32_t selector_base = upb_fielddef_selectorbase(f);
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
      *s = selector_base;
      break;
    case UPB_HANDLER_STRING:
      if (upb_fielddef_isstring(f)) {
        *s = selector_base;
      } else if (upb_fielddef_lazy(f)) {
        *s = selector_base + 3;
      } else {
        return false;
      }
      break;
    case UPB_HANDLER_STARTSTR:
      if (upb_fielddef_isstring(f) || upb_fielddef_lazy(f)) {
        *s = selector_base + 1;
      } else {
        return false;
      }
      break;
    case UPB_HANDLER_ENDSTR:
      if (upb_fielddef_isstring(f) || upb_fielddef_lazy(f)) {
        *s = selector_base + 2;
      } else {
        return false;
      }
      break;
    case UPB_HANDLER_STARTSEQ:
      if (!upb_fielddef_isseq(f)) return false;
      *s = selector_base - 2;
      break;
    case UPB_HANDLER_ENDSEQ:
      if (!upb_fielddef_isseq(f)) return false;
      *s = selector_base - 1;
      break;
    case UPB_HANDLER_STARTSUBMSG:
      if (!upb_fielddef_issubmsg(f)) return false;
      /* Selectors for STARTSUBMSG are at the beginning of the table so that the
       * selector can also be used as an index into the "sub" array of
       * subhandlers.  The indexes for the two into these two tables are the
       * same, except that in the handler table the static selectors come first. */
      *s = upb_fielddef_index(f) + UPB_STATIC_SELECTOR_COUNT;
      break;
    case UPB_HANDLER_ENDSUBMSG:
      if (!upb_fielddef_issubmsg(f)) return false;
      *s = selector_base;
      break;
  }
  UPB_ASSERT((size_t)*s < upb_msgdef_selectorcount(upb_fielddef_containingtype(f)));
  return true;
}

/* upb_handlercache ***********************************************************/

struct upb_handlercache {
  upb_arena *arena;
  upb_inttable tab;  /* maps upb_msgdef* -> upb_handlers*. */
  upb_handlers_callback *callback;
  const void *closure;
};

const upb_handlers *upb_handlercache_get(upb_handlercache *c,
                                         const upb_msgdef *md) {
  int i, n;
  upb_value v;
  upb_handlers *h;

  if (upb_inttable_lookupptr(&c->tab, md, &v)) {
    return upb_value_getptr(v);
  }

  h = upb_handlers_new(md, c, c->arena);
  v = upb_value_ptr(h);

  if (!h) return NULL;
  if (!upb_inttable_insertptr(&c->tab, md, v)) return NULL;

  c->callback(c->closure, h);

  /* For each submessage field, get or create a handlers object and set it as
   * the subhandlers. */
  n = upb_msgdef_fieldcount(md);
  for (i = 0; i < n; i++) {
    const upb_fielddef *f = upb_msgdef_field(md, i);

    if (upb_fielddef_issubmsg(f)) {
      const upb_msgdef *subdef = upb_fielddef_msgsubdef(f);
      const upb_handlers *sub_mh = upb_handlercache_get(c, subdef);

      if (!sub_mh) return NULL;

      upb_handlers_setsubhandlers(h, f, sub_mh);
    }
  }

  return h;
}


upb_handlercache *upb_handlercache_new(upb_handlers_callback *callback,
                                       const void *closure) {
  upb_handlercache *cache = upb_gmalloc(sizeof(*cache));

  if (!cache) return NULL;

  cache->arena = upb_arena_new();

  cache->callback = callback;
  cache->closure = closure;

  if (!upb_inttable_init(&cache->tab, UPB_CTYPE_PTR)) goto oom;

  return cache;

oom:
  upb_gfree(cache);
  return NULL;
}

void upb_handlercache_free(upb_handlercache *cache) {
  upb_inttable_uninit(&cache->tab);
  upb_arena_free(cache->arena);
  upb_gfree(cache);
}

bool upb_handlercache_addcleanup(upb_handlercache *c, void *p,
                                 upb_handlerfree *func) {
  return upb_arena_addcleanup(c->arena, p, func);
}

/* upb_byteshandler ***********************************************************/

bool upb_byteshandler_setstartstr(upb_byteshandler *h,
                                  upb_startstr_handlerfunc *func, void *d) {
  h->table[UPB_STARTSTR_SELECTOR].func = (upb_func*)func;
  h->table[UPB_STARTSTR_SELECTOR].attr.handler_data = d;
  return true;
}

bool upb_byteshandler_setstring(upb_byteshandler *h,
                                upb_string_handlerfunc *func, void *d) {
  h->table[UPB_STRING_SELECTOR].func = (upb_func*)func;
  h->table[UPB_STRING_SELECTOR].attr.handler_data = d;
  return true;
}

bool upb_byteshandler_setendstr(upb_byteshandler *h,
                                upb_endfield_handlerfunc *func, void *d) {
  h->table[UPB_ENDSTR_SELECTOR].func = (upb_func*)func;
  h->table[UPB_ENDSTR_SELECTOR].attr.handler_data = d;
  return true;
}

/** Handlers for upb_msg ******************************************************/

typedef struct {
  size_t offset;
  int32_t hasbit;
} upb_msg_handlerdata;

/* Fallback implementation if the handler is not specialized by the producer. */
#define MSG_WRITER(type, ctype)                                               \
  bool upb_msg_set ## type (void *c, const void *hd, ctype val) {             \
    uint8_t *m = c;                                                           \
    const upb_msg_handlerdata *d = hd;                                        \
    if (d->hasbit > 0)                                                        \
      *(uint8_t*)&m[d->hasbit / 8] |= 1 << (d->hasbit % 8);                   \
    *(ctype*)&m[d->offset] = val;                                             \
    return true;                                                              \
  }                                                                           \

MSG_WRITER(double, double)
MSG_WRITER(float,  float)
MSG_WRITER(int32,  int32_t)
MSG_WRITER(int64,  int64_t)
MSG_WRITER(uint32, uint32_t)
MSG_WRITER(uint64, uint64_t)
MSG_WRITER(bool,   bool)

bool upb_msg_setscalarhandler(upb_handlers *h, const upb_fielddef *f,
                              size_t offset, int32_t hasbit) {
  upb_handlerattr attr = UPB_HANDLERATTR_INIT;
  bool ok;

  upb_msg_handlerdata *d = upb_gmalloc(sizeof(*d));
  if (!d) return false;
  d->offset = offset;
  d->hasbit = hasbit;

  attr.handler_data = d;
  attr.alwaysok = true;
  upb_handlers_addcleanup(h, d, upb_gfree);

#define TYPE(u, l) \
  case UPB_TYPE_##u: \
    ok = upb_handlers_set##l(h, f, upb_msg_set##l, &attr); break;

  ok = false;

  switch (upb_fielddef_type(f)) {
    TYPE(INT64,  int64);
    TYPE(INT32,  int32);
    TYPE(ENUM,   int32);
    TYPE(UINT64, uint64);
    TYPE(UINT32, uint32);
    TYPE(DOUBLE, double);
    TYPE(FLOAT,  float);
    TYPE(BOOL,   bool);
    default: UPB_ASSERT(false); break;
  }
#undef TYPE

  return ok;
}

bool upb_msg_getscalarhandlerdata(const upb_handlers *h,
                                  upb_selector_t s,
                                  upb_fieldtype_t *type,
                                  size_t *offset,
                                  int32_t *hasbit) {
  const upb_msg_handlerdata *d;
  const void *p;
  upb_func *f = upb_handlers_gethandler(h, s, &p);

  if ((upb_int64_handlerfunc*)f == upb_msg_setint64) {
    *type = UPB_TYPE_INT64;
  } else if ((upb_int32_handlerfunc*)f == upb_msg_setint32) {
    *type = UPB_TYPE_INT32;
  } else if ((upb_uint64_handlerfunc*)f == upb_msg_setuint64) {
    *type = UPB_TYPE_UINT64;
  } else if ((upb_uint32_handlerfunc*)f == upb_msg_setuint32) {
    *type = UPB_TYPE_UINT32;
  } else if ((upb_double_handlerfunc*)f == upb_msg_setdouble) {
    *type = UPB_TYPE_DOUBLE;
  } else if ((upb_float_handlerfunc*)f == upb_msg_setfloat) {
    *type = UPB_TYPE_FLOAT;
  } else if ((upb_bool_handlerfunc*)f == upb_msg_setbool) {
    *type = UPB_TYPE_BOOL;
  } else {
    return false;
  }

  d = p;
  *offset = d->offset;
  *hasbit = d->hasbit;
  return true;
}
