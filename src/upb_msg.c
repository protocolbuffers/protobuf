/*
 * upb - a minimalist implementation of protocol buffers.
 *
 * Copyright (c) 2010 Google Inc.  See LICENSE for details.
 * Author: Josh Haberman <jhaberman@gmail.com>
 *
 * Data structure for storing a message of protobuf data.
 */

#include "upb_msg.h"

static uint32_t upb_round_up_pow2(uint32_t v) {
  // http://graphics.stanford.edu/~seander/bithacks.html#RoundUpPowerOf2
  v--;
  v |= v >> 1;
  v |= v >> 2;
  v |= v >> 4;
  v |= v >> 8;
  v |= v >> 16;
  v++;
  return v;
}

static void upb_elem_free(upb_value v, upb_fielddef *f) {
  switch(f->type) {
    case UPB_TYPE(MESSAGE):
    case UPB_TYPE(GROUP):
      _upb_msg_free(upb_value_getmsg(v), upb_downcast_msgdef(f->def));
      break;
    case UPB_TYPE(STRING):
    case UPB_TYPE(BYTES):
      _upb_string_free(upb_value_getstr(v));
      break;
    default:
      abort();
  }
}

static void upb_elem_unref(upb_value v, upb_fielddef *f) {
  assert(upb_elem_ismm(f));
  upb_atomic_refcount_t *refcount = upb_value_getrefcount(v);
  if (refcount && upb_atomic_unref(refcount))
    upb_elem_free(v, f);
}

static void upb_field_free(upb_value v, upb_fielddef *f) {
  if (upb_isarray(f)) {
    _upb_array_free(upb_value_getarr(v), f);
  } else {
    upb_elem_free(v, f);
  }
}

static void upb_field_unref(upb_value v, upb_fielddef *f) {
  assert(upb_field_ismm(f));
  upb_atomic_refcount_t *refcount = upb_value_getrefcount(v);
  if (refcount && upb_atomic_unref(refcount))
    upb_field_free(v, f);
}


/* upb_array ******************************************************************/

upb_array *upb_array_new(void) {
  upb_array *arr = malloc(sizeof(*arr));
  upb_atomic_refcount_init(&arr->refcount, 1);
  arr->size = 0;
  arr->len = 0;
  arr->ptr = NULL;
  return arr;
}

void __attribute__((noinline)) upb_array_dorecycle(upb_array **_arr) {
  upb_array *arr = *_arr;
  if(arr && upb_atomic_only(&arr->refcount)) {
    arr->len = 0;
  } else {
    if (arr) {
      bool was_lastref = upb_atomic_unref(&arr->refcount);
      (void)was_lastref;
      assert(!was_lastref);   // If it was, we would have just recycled.
    }
    *_arr = upb_array_new();
  }
}

void upb_array_recycle(upb_array **_arr) {
  upb_array *arr = *_arr;
  if(arr && upb_atomic_only(&arr->refcount)) {
    arr->len = 0;
  } else {
    upb_array_dorecycle(_arr);
  }
}

void _upb_array_free(upb_array *arr, upb_fielddef *f) {
  if (upb_elem_ismm(f)) {
    // Need to release refs on sub-objects.
    upb_valuetype_t type = upb_elem_valuetype(f);
    for (upb_arraylen_t i = 0; i < arr->size; i++) {
      upb_valueptr p = _upb_array_getptr(arr, f, i);
      upb_elem_unref(upb_value_read(p, type), f);
    }
  }
  free(arr->ptr);
  free(arr);
}

void __attribute__((noinline)) upb_array_doresize(
    upb_array *arr, size_t type_size, upb_arraylen_t len) {
  upb_arraylen_t old_size = arr->size;
  size_t new_size = upb_round_up_pow2(len);
  arr->ptr = realloc(arr->ptr, new_size * type_size);
  arr->size = new_size;
  memset(arr->ptr + (old_size * type_size), 0,
         (new_size - old_size) * type_size);
}

void upb_array_resizefortypesize(upb_array *arr, size_t type_size,
                                 upb_arraylen_t len) {
  if (arr->size < len) upb_array_doresize(arr, type_size, len);
  arr->len = len;
}

void upb_array_resize(upb_array *arr, upb_fielddef *f, upb_arraylen_t len) {
  upb_array_resizefortypesize(arr, upb_types[f->type].size, len);
}


/* upb_msg ********************************************************************/

upb_msg *upb_msg_new(upb_msgdef *md) {
  upb_msg *msg = malloc(md->size);
  // Clear all set bits and cached pointers.
  memset(msg, 0, md->size);
  upb_atomic_refcount_init(&msg->refcount, 1);
  return msg;
}

void _upb_msg_free(upb_msg *msg, upb_msgdef *md) {
  // Need to release refs on all sub-objects.
  upb_msg_iter i;
  for(i = upb_msg_begin(md); !upb_msg_done(i); i = upb_msg_next(md, i)) {
    upb_fielddef *f = upb_msg_iter_field(i);
    upb_valueptr p = _upb_msg_getptr(msg, f);
    upb_valuetype_t type = upb_field_valuetype(f);
    if (upb_field_ismm(f)) upb_field_unref(upb_value_read(p, type), f);
  }
  free(msg);
}

void upb_msg_recycle(upb_msg **_msg, upb_msgdef *msgdef) {
  upb_msg *msg = *_msg;
  if(msg && upb_atomic_only(&msg->refcount)) {
    upb_msg_clear(msg, msgdef);
  } else {
    upb_msg_unref(msg, msgdef);
    if (msg) {
      bool was_lastref = upb_atomic_unref(&msg->refcount);
      (void)was_lastref;
      assert(!was_lastref);
    }
    *_msg = upb_msg_new(msgdef);
  }
}

INLINE void upb_msg_sethas(upb_msg *msg, upb_fielddef *f) {
  msg->data[f->set_bit_offset] |= f->set_bit_mask;
}

void upb_msg_set(upb_msg *msg, upb_fielddef *f, upb_value val) {
  assert(val.type == upb_types[upb_field_valuetype(f)].inmemory_type);
  upb_valueptr ptr = _upb_msg_getptr(msg, f);
  if (upb_field_ismm(f)) {
    // Unref any previous value we may have had there.
    upb_value oldval = upb_value_read(ptr, upb_field_valuetype(f));
    upb_field_unref(oldval, f);

    // Ref the new value.
    upb_atomic_refcount_t *refcount = upb_value_getrefcount(val);
    if (refcount) upb_atomic_ref(refcount);
  }
  upb_msg_sethas(msg, f);
  return upb_value_write(ptr, val, upb_field_valuetype(f));
}

upb_value upb_msg_get(upb_msg *msg, upb_fielddef *f) {
  if (!upb_msg_has(msg, f)) {
    upb_value val = f->default_value;
    if (upb_issubmsg(f)) {
      // TODO: handle arrays also, which must be treated similarly.
      upb_msgdef *md = upb_downcast_msgdef(f->def);
      upb_msg *m = upb_msg_new(md);
      // Copy all set bits and values, except the refcount.
      memcpy(m , upb_value_getmsg(val), md->size);
      upb_atomic_refcount_init(&m->refcount, 0); // The msg will take a ref.
      upb_value_setmsg(&val, m);
    }
    upb_msg_set(msg, f, val);
    return val;
  } else {
    return upb_value_read(_upb_msg_getptr(msg, f), upb_field_valuetype(f));
  }
}

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

static upb_valueptr upb_msg_getappendptr(upb_msg *msg, upb_fielddef *f) {
  upb_valueptr p = _upb_msg_getptr(msg, f);
  if (upb_isarray(f)) {
    // Create/recycle/resize the array if necessary, and find a pointer to
    // a newly-appended element.
    if (!upb_msg_has(msg, f)) {
      upb_array_recycle(p.arr);
      upb_msg_sethas(msg, f);
    }
    assert(*p.arr != NULL);
    upb_arraylen_t oldlen = upb_array_len(*p.arr);
    upb_array_resize(*p.arr, f, oldlen + 1);
    p = _upb_array_getptr(*p.arr, f, oldlen);
  }
  return p;
}

upb_msg *upb_msg_appendmsg(upb_msg *msg, upb_fielddef *f, upb_msgdef *msgdef) {
  upb_valueptr p = upb_msg_getappendptr(msg, f);
  if (upb_isarray(f) || !upb_msg_has(msg, f)) {
    upb_msg_recycle(p.msg, msgdef);
    upb_msg_sethas(msg, f);
  }
  return *p.msg;
}


/* upb_msg handlers ***********************************************************/

#if UPB_MAX_FIELDS > 2048
#error "We're using an 8-bit integer to store a has_offset."
#endif
typedef struct {
  uint8_t has_offset;
  uint8_t has_mask;
  uint16_t val_offset;
  uint16_t msg_size;
  uint8_t set_flags_bytes;
  uint8_t padding;
} upb_msgsink_fval;

static upb_msgsink_fval upb_msgsink_unpackfval(upb_value fval) {
  assert(sizeof(upb_msgsink_fval) == 8);
  upb_msgsink_fval ret;
  uint64_t fval_u64 = upb_value_getuint64(fval);
  memcpy(&ret, &fval_u64, 8);
  return ret;
}

static uint64_t upb_msgsink_packfval(uint8_t has_offset, uint8_t has_mask,
                                     uint16_t val_offset, uint16_t msg_size,
                                     uint8_t set_flags_bytes) {
  upb_msgsink_fval fval = {
      has_offset, has_mask, val_offset, msg_size, set_flags_bytes, 0};
  uint64_t ret = 0;
  memcpy(&ret, &fval, sizeof(fval));
  return ret;
}

upb_valueptr __attribute__((noinline)) upb_array_recycleorresize(
    upb_msg *m, upb_msgsink_fval fval, size_t type_size) {
  upb_array **arr = (void*)&m->data[fval.val_offset];
  if (!(m->data[fval.has_offset] & fval.has_mask)) {
    upb_array_recycle(arr);
    m->data[fval.has_offset] |= fval.has_mask;
  }
  upb_arraylen_t len = upb_array_len(*arr);
  upb_array_resizefortypesize(*arr, type_size, len+1);
  return _upb_array_getptrforsize(*arr, type_size, len);
}

#define SCALAR_VALUE_CB_PAIR(type, ctype)                                     \
  upb_flow_t upb_msgsink_ ## type ## value(void *_m, upb_value _fval,         \
                                                  upb_value val) {            \
    upb_msg *m = _m;                                                          \
    upb_msgsink_fval fval = upb_msgsink_unpackfval(_fval);                    \
    m->data[fval.has_offset] |= fval.has_mask;                                \
    *(ctype*)&m->data[fval.val_offset] = upb_value_get ## type(val);          \
    return UPB_CONTINUE;                                                      \
  }                                                                           \
                                                                              \
  upb_flow_t upb_msgsink_ ## type ## value_r(void *_m, upb_value _fval,       \
                                                    upb_value val) {          \
    upb_msg *m = _m;                                                          \
    upb_msgsink_fval fval = upb_msgsink_unpackfval(_fval);                    \
    upb_array *arr = *(upb_array**)&m->data[fval.val_offset];                 \
    if (!(m->data[fval.has_offset] & fval.has_mask)) {                        \
      if(arr && upb_atomic_only(&arr->refcount)) {                            \
        m->data[fval.has_offset] |= fval.has_mask;                            \
        arr->len = 0;                                                         \
      } else {                                                                \
        goto slow;                                                            \
      }                                                                       \
    } else if (arr->len == arr->size) goto slow;                              \
    upb_valueptr p = _upb_array_getptrforsize(arr, sizeof(ctype),             \
                                              arr->len++);                    \
    *(ctype*)p._void = upb_value_get ## type(val);                            \
    return UPB_CONTINUE;                                                      \
  slow:                                                                       \
    p = upb_array_recycleorresize(m, fval, sizeof(ctype));                    \
    *(ctype*)p._void = upb_value_get ## type(val);                            \
    return UPB_CONTINUE;                                                      \
  }                                                                           \

SCALAR_VALUE_CB_PAIR(double, double)
SCALAR_VALUE_CB_PAIR(float, float)
SCALAR_VALUE_CB_PAIR(int32, int32_t)
SCALAR_VALUE_CB_PAIR(int64, int64_t)
SCALAR_VALUE_CB_PAIR(uint32, uint32_t)
SCALAR_VALUE_CB_PAIR(uint64, uint64_t)
SCALAR_VALUE_CB_PAIR(bool, bool)

upb_flow_t upb_msgsink_strvalue(void *_m, upb_value _fval, upb_value val) {
  upb_msg *m = _m;
  upb_msgsink_fval fval = upb_msgsink_unpackfval(_fval);
  m->data[fval.has_offset] |= fval.has_mask;
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
  upb_string *src = upb_value_getstr(val);
  upb_string **dst = (void*)&m->data[fval.val_offset];
  upb_string_recycle(dst);
  upb_string_substr(*dst, src, 0, upb_string_len(src));
  return UPB_CONTINUE;
}

upb_flow_t upb_msgsink_strvalue_r(void *_m, upb_value _fval,
                                  upb_value val) {
  upb_msg *m = _m;
  upb_msgsink_fval fval = upb_msgsink_unpackfval(_fval);
  upb_array *arr = *(upb_array**)&m->data[fval.val_offset];
  if (!(m->data[fval.has_offset] & fval.has_mask)) {
    if(arr && upb_atomic_only(&arr->refcount)) {
      m->data[fval.has_offset] |= fval.has_mask;
      arr->len = 0;
    } else {
      goto slow;
    }
  } else if (arr->len == arr->size) goto slow;
  ++arr->len;
  upb_valueptr p = _upb_array_getptrforsize(arr, sizeof(void*),
                                            upb_array_len(arr));
  upb_string *src = upb_value_getstr(val);
  upb_string_recycle(p.str);
  upb_string_substr(*p.str, src, 0, upb_string_len(src));
  return UPB_CONTINUE;
slow:
  p = upb_array_recycleorresize(m, fval, sizeof(void*));
  src = upb_value_getstr(val);
  upb_string_recycle(p.str);
  upb_string_substr(*p.str, src, 0, upb_string_len(src));
  return UPB_CONTINUE;
}


upb_sflow_t upb_msgsink_startsubmsg(void *_m, upb_value _fval) {
  upb_msg *m = _m;
  upb_msgsink_fval fval = upb_msgsink_unpackfval(_fval);
  upb_msgdef md;
  md.size = fval.msg_size;
  md.set_flags_bytes = fval.set_flags_bytes;
  upb_fielddef f;
  f.set_bit_mask = fval.has_mask;
  f.set_bit_offset = fval.has_offset;
  f.label = UPB_LABEL(OPTIONAL);  // Just not repeated.
  f.type = UPB_TYPE(MESSAGE);
  f.byte_offset = fval.val_offset;
  return UPB_CONTINUE_WITH(upb_msg_appendmsg(m, &f, &md));
}

upb_sflow_t upb_msgsink_startsubmsg_r(void *_m, upb_value _fval) {
  upb_msg *m = _m;
  upb_msgsink_fval fval = upb_msgsink_unpackfval(_fval);
  upb_msgdef md;
  md.size = fval.msg_size;
  md.set_flags_bytes = fval.set_flags_bytes;
  upb_fielddef f;
  f.set_bit_mask = fval.has_mask;
  f.set_bit_offset = fval.has_offset;
  f.label = UPB_LABEL(REPEATED);
  f.type = UPB_TYPE(MESSAGE);
  f.byte_offset = fval.val_offset;
  return UPB_CONTINUE_WITH(upb_msg_appendmsg(m, &f, &md));
}

INLINE void upb_msg_onfreg(void *c, upb_fhandlers *fh, upb_fielddef *f) {
  (void)c;
  uint16_t msg_size = 0;
  uint8_t set_flags_bytes = 0;
  if (upb_issubmsg(f)) {
    upb_msgdef *md = upb_downcast_msgdef(f->def);
    msg_size = md->size;
    set_flags_bytes = md->set_flags_bytes;
  }
  upb_value_setuint64(&fh->fval,
      upb_msgsink_packfval(f->set_bit_offset, f->set_bit_mask,
                           f->byte_offset, msg_size, set_flags_bytes));
#define CASE(upb_type, type) \
case UPB_TYPE(upb_type): \
    fh->value = upb_isarray(f) ? \
        upb_msgsink_ ## type ## value_r : upb_msgsink_ ## type ## value; \
    break;
  switch (f->type) {
    CASE(DOUBLE,   double)
    CASE(FLOAT,    float)
    CASE(INT32,    int32)
    CASE(INT64,    int64)
    CASE(UINT32,   uint32)
    CASE(UINT64,   uint64)
    CASE(SINT32,   int32)
    CASE(SINT64,   int64)
    CASE(FIXED32,  uint32)
    CASE(FIXED64,  uint64)
    CASE(SFIXED32, int32)
    CASE(SFIXED64, int64)
    CASE(BOOL,     bool)
    CASE(ENUM,     int32)
    CASE(STRING,   str)
    CASE(BYTES,    str)
#undef CASE
    case UPB_TYPE(MESSAGE):
    case UPB_TYPE(GROUP):
      fh->startsubmsg =
          upb_isarray(f) ? upb_msgsink_startsubmsg_r : upb_msgsink_startsubmsg;
      break;
  }
}

upb_mhandlers *upb_msg_reghandlers(upb_handlers *h, upb_msgdef *m) {
  return upb_handlers_regmsgdef(h, m, NULL, &upb_msg_onfreg, NULL);
}
