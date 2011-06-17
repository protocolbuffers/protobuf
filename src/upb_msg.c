/*
 * upb - a minimalist implementation of protocol buffers.
 *
 * Copyright (c) 2010 Google Inc.  See LICENSE for details.
 * Author: Josh Haberman <jhaberman@gmail.com>
 *
 * Data structure for storing a message of protobuf data.
 */

#include "upb_msg.h"

void upb_msg_clear(void *msg, upb_msgdef *md) {
  memset(msg, 0, md->hasbit_bytes);
  // TODO: set primitive fields to defaults?
}

void *upb_stdarray_append(upb_stdarray *a, size_t type_size) {
  assert(a->len <= a->size);
  if (a->len == a->size) {
    size_t old_size = a->size;
    a->size = old_size == 0 ? 8 : (old_size * 2);
    a->ptr = realloc(a->ptr, a->size * type_size);
    memset(&a->ptr[old_size * type_size], 0, (a->size - old_size) * type_size);
  }
  return &a->ptr[a->len++ * type_size];
}

#if 0
static upb_flow_t upb_msg_dispatch(upb_msg *msg, upb_msgdef *md,
                                   upb_dispatcher *d);

static upb_flow_t upb_msg_pushval(upb_value val, upb_fielddef *f,
                                  upb_dispatcher *d, upb_fhandlers *hf) {
  if (upb_issubmsg(f)) {
    upb_msg *msg = upb_value_getmsg(val);
    upb_dispatch_startsubmsg(d, hf);
    upb_msg_dispatch(msg, upb_downcast_msgdef(f->def), d);
    upb_dispatch_endsubmsg(d);
  } else {
    upb_dispatch_value(d, hf, val);
  }
  return UPB_CONTINUE;
}

static upb_flow_t upb_msg_dispatch(upb_msg *msg, upb_msgdef *md,
                                   upb_dispatcher *d) {
  upb_msg_iter i;
  for(i = upb_msg_begin(md); !upb_msg_done(i); i = upb_msg_next(md, i)) {
    upb_fielddef *f = upb_msg_iter_field(i);
    if (!upb_msg_has(msg, f)) continue;
    upb_fhandlers *hf = upb_dispatcher_lookup(d, f->number);
    if (!hf) continue;
    upb_value val = upb_msg_get(msg, f);
    if (upb_isarray(f)) {
      upb_array *arr = upb_value_getarr(val);
      for (uint32_t j = 0; j < upb_array_len(arr); ++j) {
        upb_msg_pushval(upb_array_get(arr, f, j), f, d, hf);
      }
    } else {
      upb_msg_pushval(val, f, d, hf);
    }
  }
  return UPB_CONTINUE;
}

void upb_msg_runhandlers(upb_msg *msg, upb_msgdef *md, upb_handlers *h,
                         void *closure, upb_status *status) {
  upb_dispatcher d;
  upb_dispatcher_init(&d, h, NULL, NULL, NULL);
  upb_dispatcher_reset(&d, closure);

  upb_dispatch_startmsg(&d);
  upb_msg_dispatch(msg, md, &d);
  upb_dispatch_endmsg(&d, status);

  upb_dispatcher_uninit(&d);
}
#endif

/* Standard writers. **********************************************************/

void upb_stdmsg_sethas(void *_m, upb_value fval) {
  char *m = _m;
  upb_fielddef *f = upb_value_getfielddef(fval);
  if (f->hasbit >= 0) m[f->hasbit / 8] |= (1 << (f->hasbit % 8));
}

bool upb_stdmsg_has(void *_m, upb_value fval) {
  char *m = _m;
  upb_fielddef *f = upb_value_getfielddef(fval);
  return f->hasbit < 0 || (m[f->hasbit / 8] & (1 << (f->hasbit % 8)));
}

#define UPB_ACCESSORS(type, ctype)                                            \
  upb_flow_t upb_stdmsg_set ## type (void *_m, upb_value fval,                \
                                     upb_value val) {                         \
    upb_fielddef *f = upb_value_getfielddef(fval);                            \
    uint8_t *m = _m;                                                          \
    upb_stdmsg_sethas(_m, fval);                                              \
    *(ctype*)&m[f->offset] = upb_value_get ## type(val);                      \
    return UPB_CONTINUE;                                                      \
  }                                                                           \
                                                                              \
  upb_flow_t upb_stdmsg_set ## type ## _r(void *a, upb_value _fval,           \
                                          upb_value val) {                    \
    (void)_fval;                                                              \
    ctype *p = upb_stdarray_append((upb_stdarray*)a, sizeof(ctype));          \
    *p = upb_value_get ## type(val);                                          \
    return UPB_CONTINUE;                                                      \
  }                                                                           \
                                                                              \
  upb_value upb_stdmsg_get ## type(void *_m, upb_value fval) {                \
    uint8_t *m = _m;                                                          \
    upb_fielddef *f = upb_value_getfielddef(fval);                            \
    upb_value ret;                                                            \
    upb_value_set ## type(&ret, *(ctype*)&m[f->offset]);                      \
    return ret;                                                               \
  }                                                                           \
  upb_value upb_stdmsg_seqget ## type(void *i) {                              \
    upb_value val;                                                            \
    upb_value_set ## type(&val, *(ctype*)i);                                  \
    return val;                                                               \
  }

UPB_ACCESSORS(double, double)
UPB_ACCESSORS(float, float)
UPB_ACCESSORS(int32, int32_t)
UPB_ACCESSORS(int64, int64_t)
UPB_ACCESSORS(uint32, uint32_t)
UPB_ACCESSORS(uint64, uint64_t)
UPB_ACCESSORS(bool, bool)
UPB_ACCESSORS(ptr, void*)
#undef UPB_ACCESSORS

static void _upb_stdmsg_setstr(void *_dst, upb_value _src) {
  // We do:
  //  - upb_string_recycle(), upb_string_substr() instead of
  //  - upb_string_unref(), upb_string_getref()
  // because we can conveniently cache these upb_string objects in the
  // upb_msg, whereas the upb_src who is sending us these strings may not
  // have a good way of caching them.  This saves the upb_src from allocating
  // new upb_strings all the time to give us.
  //
  // If you were using this to copy one upb_msg to another this would
  // allocate string objects whereas a upb_string_getref could have avoided
  // those allocations completely; if this is an issue, we could make it an
  // option of the upb_msgsink which behavior is desired.
  upb_string **dst = _dst;
  upb_string *src = upb_value_getstr(_src);
  upb_string_recycle(dst);
  upb_string_substr(*dst, src, 0, upb_string_len(src));
}

upb_flow_t upb_stdmsg_setstr(void *_m, upb_value fval, upb_value val) {
  char *m = _m;
  upb_fielddef *f = upb_value_getfielddef(fval);
  upb_stdmsg_sethas(_m, fval);
  _upb_stdmsg_setstr(&m[f->offset], val);
  return UPB_CONTINUE;
}

upb_flow_t upb_stdmsg_setstr_r(void *a, upb_value fval, upb_value val) {
  (void)fval;
  _upb_stdmsg_setstr(upb_stdarray_append((upb_stdarray*)a, sizeof(void*)), val);
  return UPB_CONTINUE;
}

upb_value upb_stdmsg_getstr(void *m, upb_value fval) {
  upb_value val = upb_stdmsg_getptr(m, fval);
  upb_value_setstr(&val, upb_value_getptr(val));
  return val;
}

upb_value upb_stdmsg_seqgetstr(void *i) {
  upb_value val = upb_stdmsg_seqgetptr(i);
  upb_value_setstr(&val, upb_value_getptr(val));
  return val;
}

void *upb_stdmsg_new(upb_msgdef *md) {
  void *m = malloc(md->size);
  memset(m, 0, md->size);
  upb_msg_clear(m, md);
  return m;
}

void upb_stdseq_free(void *s, upb_fielddef *f) {
  upb_stdarray *a = s;
  if (upb_issubmsg(f) || upb_isstring(f)) {
    void **p = (void**)a->ptr;
    for (int i = 0; i < a->size; i++) {
      if (upb_issubmsg(f)) {
        upb_stdmsg_free(p[i], upb_downcast_msgdef(f->def));
      } else {
        upb_string_unref(p[i]);
      }
    }
  }
  free(a->ptr);
  free(a);
}

void upb_stdmsg_free(void *m, upb_msgdef *md) {
  if (m == NULL) return;
  upb_msg_iter i;
  for(i = upb_msg_begin(md); !upb_msg_done(i); i = upb_msg_next(md, i)) {
    upb_fielddef *f = upb_msg_iter_field(i);
    if (!upb_isseq(f) && !upb_issubmsg(f) && !upb_isstring(f)) continue;
    void *subp = upb_value_getptr(upb_stdmsg_getptr(m, f->fval));
    if (subp == NULL) continue;
    if (upb_isseq(f)) {
      upb_stdseq_free(subp, f);
    } else if (upb_issubmsg(f)) {
      upb_stdmsg_free(subp, upb_downcast_msgdef(f->def));
    } else {
      upb_string_unref(subp);
    }
  }
  free(m);
}

upb_sflow_t upb_stdmsg_startseq(void *_m, upb_value fval) {
  char *m = _m;
  upb_fielddef *f = upb_value_getfielddef(fval);
  upb_stdarray **arr = (void*)&m[f->offset];
  if (!upb_stdmsg_has(_m, fval)) {
    if (!*arr) {
      *arr = malloc(sizeof(**arr));
      (*arr)->size = 0;
      (*arr)->ptr = NULL;
    }
    (*arr)->len = 0;
    upb_stdmsg_sethas(m, fval);
  }
  return UPB_CONTINUE_WITH(*arr);
}

void upb_stdmsg_recycle(void **m, upb_msgdef *md) {
  if (*m)
    upb_msg_clear(*m, md);
  else
    *m = upb_stdmsg_new(md);
}

upb_sflow_t upb_stdmsg_startsubmsg(void *_m, upb_value fval) {
  char *m = _m;
  upb_fielddef *f = upb_value_getfielddef(fval);
  void **subm = (void*)&m[f->offset];
  if (!upb_stdmsg_has(m, fval)) {
    upb_stdmsg_recycle(subm, upb_downcast_msgdef(f->def));
    upb_stdmsg_sethas(m, fval);
  }
  return UPB_CONTINUE_WITH(*subm);
}

upb_sflow_t upb_stdmsg_startsubmsg_r(void *a, upb_value fval) {
  assert(a != NULL);
  upb_fielddef *f = upb_value_getfielddef(fval);
  void **subm = upb_stdarray_append((upb_stdarray*)a, sizeof(void*));
  upb_stdmsg_recycle(subm, upb_downcast_msgdef(f->def));
  return UPB_CONTINUE_WITH(*subm);
}

void *upb_stdmsg_seqbegin(void *_a) {
  upb_stdarray *a = _a;
  return a->len > 0 ? a->ptr : NULL;
}

#define NEXTFUNC(size) \
  void *upb_stdmsg_ ## size ## byte_seqnext(void *_a, void *iter) {      \
    upb_stdarray *a = _a;                                                \
    void *next = (char*)iter + size;                                     \
    return (char*)next < (char*)a->ptr + (a->len * size) ? next : NULL;  \
  }

NEXTFUNC(8)
NEXTFUNC(4)
NEXTFUNC(1)

#define STDMSG(type) { static upb_accessor_vtbl vtbl = {NULL, &upb_stdmsg_startsubmsg, \
  &upb_stdmsg_set ## type, &upb_stdmsg_has, &upb_stdmsg_get ## type, \
  NULL, NULL, NULL}; return &vtbl; }
#define STDMSG_R(type, size) { static upb_accessor_vtbl vtbl = { \
  &upb_stdmsg_startseq, &upb_stdmsg_startsubmsg_r, &upb_stdmsg_set ## type ## _r, \
  &upb_stdmsg_has, &upb_stdmsg_getptr, &upb_stdmsg_seqbegin, \
  &upb_stdmsg_ ## size ## byte_seqnext, &upb_stdmsg_seqget ## type}; \
  return &vtbl; }

upb_accessor_vtbl *upb_stdmsg_accessor(upb_fielddef *f) {
  if (upb_isseq(f)) {
    switch (f->type) {
      case UPB_TYPE(DOUBLE): STDMSG_R(double, 8)
      case UPB_TYPE(FLOAT): STDMSG_R(float, 4)
      case UPB_TYPE(UINT64):
      case UPB_TYPE(FIXED64): STDMSG_R(uint64, 8)
      case UPB_TYPE(INT64):
      case UPB_TYPE(SFIXED64):
      case UPB_TYPE(SINT64): STDMSG_R(int64, 8)
      case UPB_TYPE(INT32):
      case UPB_TYPE(SINT32):
      case UPB_TYPE(ENUM):
      case UPB_TYPE(SFIXED32): STDMSG_R(int32, 4)
      case UPB_TYPE(UINT32):
      case UPB_TYPE(FIXED32): STDMSG_R(uint32, 4)
      case UPB_TYPE(BOOL): STDMSG_R(bool, 1)
      case UPB_TYPE(STRING):
      case UPB_TYPE(BYTES):
      case UPB_TYPE(GROUP):
      case UPB_TYPE(MESSAGE): STDMSG_R(str, 8)  // TODO: 32-bit
    }
  } else {
    switch (f->type) {
      case UPB_TYPE(DOUBLE): STDMSG(double)
      case UPB_TYPE(FLOAT): STDMSG(float)
      case UPB_TYPE(UINT64):
      case UPB_TYPE(FIXED64): STDMSG(uint64)
      case UPB_TYPE(INT64):
      case UPB_TYPE(SFIXED64):
      case UPB_TYPE(SINT64): STDMSG(int64)
      case UPB_TYPE(INT32):
      case UPB_TYPE(SINT32):
      case UPB_TYPE(ENUM):
      case UPB_TYPE(SFIXED32): STDMSG(int32)
      case UPB_TYPE(UINT32):
      case UPB_TYPE(FIXED32): STDMSG(uint32)
      case UPB_TYPE(BOOL): STDMSG(bool)
      case UPB_TYPE(STRING):
      case UPB_TYPE(BYTES):
      case UPB_TYPE(GROUP):
      case UPB_TYPE(MESSAGE): STDMSG(str)
    }
  }
  return NULL;
}

static void upb_accessors_onfreg(void *c, upb_fhandlers *fh, upb_fielddef *f) {
  (void)c;
  if (f->accessor) {
    upb_fhandlers_setstartseq(fh, f->accessor->appendseq);
    upb_fhandlers_setvalue(fh, f->accessor->set);
    upb_fhandlers_setstartsubmsg(fh, f->accessor->appendsubmsg);
    upb_fhandlers_setfval(fh, f->fval);
  }
}

upb_mhandlers *upb_accessors_reghandlers(upb_handlers *h, upb_msgdef *m) {
  return upb_handlers_regmsgdef(h, m, NULL, &upb_accessors_onfreg, NULL);
}
