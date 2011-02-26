/*
 * upb - a minimalist implementation of protocol buffers.
 *
 * Copyright (c) 2010 Joshua Haberman.  See LICENSE for details.
 *
 * Data structure for storing a message of protobuf data.
 */

#include "upb_msg.h"
#include "upb_decoder.h"
#include "upb_strstream.h"

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

void upb_array_recycle(upb_array **_arr, upb_fielddef *f) {
  upb_array *arr = *_arr;
  if(arr && upb_atomic_only(&arr->refcount)) {
    arr->len = 0;
  } else {
    upb_array_unref(arr, f);
    *_arr = upb_array_new();
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

void upb_array_resize(upb_array *arr, upb_fielddef *f, upb_arraylen_t len) {
  size_t type_size = upb_types[f->type].size;
  upb_arraylen_t old_size = arr->size;
  if (old_size < len) {
    // Need to resize.
    size_t new_size = upb_round_up_pow2(len);
    arr->ptr = realloc(arr->ptr, new_size * type_size);
    arr->size = new_size;
    memset(arr->ptr + (old_size * type_size), 0,
           (new_size - old_size) * type_size);
  }
  arr->len = len;
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
                                   upb_dispatcher *d, upb_status *s);

static upb_flow_t upb_msg_pushval(upb_value val, upb_fielddef *f,
                                  upb_dispatcher *d, upb_status *s) {
#define CHECK_FLOW(x) do { \
  flow = x; if (flow != UPB_CONTINUE) return flow; \
  } while(0)

// For when a SKIP can be implemented just through an early return.
#define CHECK_FLOW_LOCAL(x) do { \
  flow = x; \
  if (flow != UPB_CONTINUE) { \
    if (flow == UPB_SKIPSUBMSG) flow = UPB_CONTINUE; \
    goto end; \
  } \
} while (0)
  upb_flow_t flow;
  if (upb_issubmsg(f)) {
    upb_msg *msg = upb_value_getmsg(val);
    CHECK_FLOW_LOCAL(upb_dispatch_startsubmsg(d, f));
    CHECK_FLOW_LOCAL(upb_msg_dispatch(msg, upb_downcast_msgdef(f->def), d, s));
    CHECK_FLOW(upb_dispatch_endsubmsg(d, f));
  } else {
    CHECK_FLOW(upb_dispatch_value(d, f, val));
  }

end:
  return flow;
}

static upb_flow_t upb_msg_dispatch(upb_msg *msg, upb_msgdef *md,
                                   upb_dispatcher *d, upb_status *s) {
  upb_msg_iter i;
  upb_flow_t flow;
  for(i = upb_msg_begin(md); !upb_msg_done(i); i = upb_msg_next(md, i)) {
    upb_fielddef *f = upb_msg_iter_field(i);
    if (!upb_msg_has(msg, f)) continue;
    upb_value val = upb_msg_get(msg, f);
    if (upb_isarray(f)) {
      upb_array *arr = upb_value_getarr(val);
      for (uint32_t j = 0; j < upb_array_len(arr); ++j) {
        CHECK_FLOW_LOCAL(upb_msg_pushval(upb_array_get(arr, f, j), f, d, s));
      }
    } else {
      CHECK_FLOW_LOCAL(upb_msg_pushval(val, f, d, s));
    }
  }
  return UPB_CONTINUE;

end:
  // Need to copy/massage the error.
  upb_copyerr(s, d->top->handlers.status);
  if (upb_ok(s)) {
    upb_seterr(s, UPB_ERROR, "Callback returned UPB_BREAK");
  }
  return flow;
#undef CHECK_FLOW
#undef CHECK_FLOW_LOCAL
}

void upb_msg_runhandlers(upb_msg *msg, upb_msgdef *md, upb_handlers *h,
                         upb_status *status) {
  upb_dispatcher d;
  upb_dispatcher_init(&d);
  upb_dispatcher_reset(&d, h, true);

  if (upb_dispatch_startmsg(&d) != UPB_CONTINUE) return;
  if (upb_msg_dispatch(msg, md, &d, status) != UPB_CONTINUE) return;
  if (upb_dispatch_endmsg(&d) != UPB_CONTINUE) return;
}

static upb_valueptr upb_msg_getappendptr(upb_msg *msg, upb_fielddef *f) {
  upb_valueptr p = _upb_msg_getptr(msg, f);
  if (upb_isarray(f)) {
    // Create/recycle/resize the array if necessary, and find a pointer to
    // a newly-appended element.
    if (!upb_msg_has(msg, f)) {
      upb_array_recycle(p.arr, f);
      upb_msg_sethas(msg, f);
    }
    assert(*p.arr != NULL);
    upb_arraylen_t oldlen = upb_array_len(*p.arr);
    upb_array_resize(*p.arr, f, oldlen + 1);
    p = _upb_array_getptr(*p.arr, f, oldlen);
  }
  return p;
}

static void upb_msg_appendval(upb_msg *msg, upb_fielddef *f, upb_value val) {
  upb_valueptr p = upb_msg_getappendptr(msg, f);
  if (upb_isstring(f)) {
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
    // option of the upb_msgpopulator which behavior is desired.
    upb_string *src = upb_value_getstr(val);
    upb_string_recycle(p.str);
    upb_string_substr(*p.str, src, 0, upb_string_len(src));
  } else {
    upb_value_write(p, val, f->type);
  }
  upb_msg_sethas(msg, f);
}

upb_msg *upb_msg_appendmsg(upb_msg *msg, upb_fielddef *f, upb_msgdef *msgdef) {
  upb_valueptr p = upb_msg_getappendptr(msg, f);
  if (upb_isarray(f) || !upb_msg_has(msg, f)) {
    upb_msg_recycle(p.msg, msgdef);
    upb_msg_sethas(msg, f);
  }
  return *p.msg;
}


/* upb_msgpopulator ***********************************************************/

void upb_msgpopulator_init(upb_msgpopulator *p) {
  upb_status_init(&p->status);
}

void upb_msgpopulator_reset(upb_msgpopulator *p, upb_msg *m, upb_msgdef *md) {
  p->top = p->stack;
  p->limit = p->stack + sizeof(p->stack);
  p->top->msg = m;
  p->top->msgdef = md;
}

void upb_msgpopulator_uninit(upb_msgpopulator *p) {
  upb_status_uninit(&p->status);
}

static upb_flow_t upb_msgpopulator_value(void *_p, upb_fielddef *f, upb_value val) {
  upb_msgpopulator *p = _p;
  upb_msg_appendval(p->top->msg, f, val);
  return UPB_CONTINUE;
}

static upb_flow_t upb_msgpopulator_startsubmsg(void *_p, upb_fielddef *f,
                                               upb_handlers *delegate_to) {
  upb_msgpopulator *p = _p;
  (void)delegate_to;
  upb_msg *oldmsg = p->top->msg;
  if (++p->top == p->limit) {
    upb_seterr(&p->status, UPB_ERROR, "Exceeded maximum nesting");
    return UPB_BREAK;
  }
  upb_msgdef *msgdef = upb_downcast_msgdef(f->def);
  p->top->msgdef = msgdef;
  p->top->msg = upb_msg_appendmsg(oldmsg, f, msgdef);
  return UPB_CONTINUE;
}

static upb_flow_t upb_msgpopulator_endsubmsg(void *_p, upb_fielddef *f) {
  (void)f;
  upb_msgpopulator *p = _p;
  --p->top;
  return UPB_CONTINUE;
}

void upb_msgpopulator_register_handlers(upb_msgpopulator *p, upb_handlers *h) {
  static upb_handlerset handlerset = {
    NULL,   // startmsg
    NULL,   // endmsg
    &upb_msgpopulator_value,
    &upb_msgpopulator_startsubmsg,
    &upb_msgpopulator_endsubmsg,
  };
  upb_register_handlerset(h, &handlerset);
  upb_set_handler_closure(h, p, &p->status);
}
