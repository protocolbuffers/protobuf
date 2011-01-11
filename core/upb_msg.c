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

void _upb_elem_free(upb_value v, upb_fielddef *f) {
  switch(f->type) {
    case UPB_TYPE(MESSAGE):
    case UPB_TYPE(GROUP):
      _upb_msg_free(v.msg, upb_downcast_msgdef(f->def));
      break;
    case UPB_TYPE(STRING):
    case UPB_TYPE(BYTES):
      _upb_string_free(v.str);
      break;
    default:
      abort();
  }
}

void _upb_field_free(upb_value v, upb_fielddef *f) {
  if (upb_isarray(f)) {
    _upb_array_free(v.arr, f);
  } else {
    _upb_elem_free(v, f);
  }
}

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
    if (upb_field_ismm(f)) _upb_field_unref(upb_value_read(p, type), f);
  }
  free(msg);
}

upb_array *upb_array_new(void) {
  upb_array *arr = malloc(sizeof(*arr));
  upb_atomic_refcount_init(&arr->refcount, 1);
  arr->size = 0;
  arr->len = 0;
  arr->elements._void = NULL;
  return arr;
}

void _upb_array_free(upb_array *arr, upb_fielddef *f) {
  if (upb_elem_ismm(f)) {
    // Need to release refs on sub-objects.
    upb_valuetype_t type = upb_elem_valuetype(f);
    for (upb_arraylen_t i = 0; i < arr->size; i++) {
      upb_valueptr p = _upb_array_getptr(arr, f, i);
      _upb_elem_unref(upb_value_read(p, type), f);
    }
  }
  if (arr->elements._void) free(arr->elements._void);
  free(arr);
}

upb_value upb_field_new(upb_fielddef *f, upb_valuetype_t type) {
  upb_value v;
  switch(type) {
    case UPB_TYPE(MESSAGE):
    case UPB_TYPE(GROUP):
      v.msg = upb_msg_new(upb_downcast_msgdef(f->def));
    case UPB_TYPE(STRING):
    case UPB_TYPE(BYTES):
      v.str = upb_string_new();
    case UPB_VALUETYPE_ARRAY:
      v.arr = upb_array_new();
    default:
      abort();
  }
  return v;
}

static void upb_field_recycle(upb_value val) {
  (void)val;
}

upb_value upb_field_tryrecycle(upb_valueptr p, upb_value val, upb_fielddef *f,
                               upb_valuetype_t type) {
  if (val._void == NULL || !upb_atomic_only(val.refcount)) {
    if (val._void != NULL) upb_atomic_unref(val.refcount);
    val = upb_field_new(f, type);
    upb_value_write(p, val, type);
  } else {
    upb_field_recycle(val);
  }
  return val;
}

void upb_msg_decodestr(upb_msg *msg, upb_msgdef *md, upb_string *str,
                       upb_status *status) {
  upb_stringsrc *ssrc = upb_stringsrc_new();
  upb_stringsrc_reset(ssrc, str);
  upb_decoder *d = upb_decoder_new(md);
  upb_decoder_reset(d, upb_stringsrc_bytesrc(ssrc));

  upb_decoder_free(d);
  upb_stringsrc_free(ssrc);
}

void upb_msg_encodestr(upb_msg *msg, upb_msgdef *md, upb_string *str,
                       upb_status *status) {
  (void)msg;
  (void)md;
  (void)str;
  (void)status;
}
