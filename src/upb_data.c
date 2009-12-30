/*
 * upb - a minimalist implementation of protocol buffers.
 *
 * Copyright (c) 2009 Joshua Haberman.  See LICENSE for details.
 */

#include <stdlib.h>
#include "upb_data.h"
#include "upb_def.h"

INLINE void data_init(upb_data *d, int flags) {
  d->v = flags;
}

static uint32_t round_up_to_pow2(uint32_t v)
{
  /* cf. http://graphics.stanford.edu/~seander/bithacks.html#RoundUpPowerOf2 */
  v--;
  v |= v >> 1;
  v |= v >> 2;
  v |= v >> 4;
  v |= v >> 8;
  v |= v >> 16;
  v++;
  return v;
}

static void check_not_frozen(upb_data *d) {
  // On one hand I am reluctant to put abort() calls in a low-level library
  // that are enabled in a production build.  On the other hand, this is a bug
  // in the client code that we cannot recover from, and it seems better to get
  // the error here than later.
  if(upb_data_hasflag(d, UPB_DATA_FROZEN)) abort();
}

static upb_strlen_t string_get_bytesize(upb_string *s) {
  if(upb_data_hasflag(&s->common.base, UPB_DATA_REFCOUNTED)) {
    return s->refcounted.byte_size;
  } else {
    return (s->norefcount.byte_size_and_flags & 0xFFFFFFF8) >> 3;
  }
}

static void string_set_bytesize(upb_string *s, upb_strlen_t newsize) {
  if(upb_data_hasflag(&s->common.base, UPB_DATA_REFCOUNTED)) {
    s->refcounted.byte_size = newsize;
  } else {
    s->norefcount.byte_size_and_flags &= 0x7;
    s->norefcount.byte_size_and_flags |= (newsize << 3);
  }
}

upb_string *upb_string_new() {
  upb_string *s = malloc(sizeof(upb_refcounted_string));
  data_init(&s->common.base, UPB_DATA_HEAPALLOCATED | UPB_DATA_REFCOUNTED);
  s->refcounted.byte_size = 0;
  s->common.byte_len = 0;
  s->common.ptr = NULL;
  return s;
}

void _upb_string_free(upb_string *s)
{
  if(string_get_bytesize(s) != 0) free(s->common.ptr);
  free(s);
}

void upb_string_resize(upb_string *s, upb_strlen_t byte_len) {
  check_not_frozen(&s->common.base);
  if(string_get_bytesize(s) < byte_len) {
    // Need to resize.
    size_t new_byte_size = round_up_to_pow2(byte_len);
    s->common.ptr = realloc(s->common.ptr, new_byte_size);
    string_set_bytesize(s, new_byte_size);
  }
  s->common.byte_len = byte_len;
}

upb_msg *upb_msg_new(struct upb_msgdef *md) {
  upb_msg *msg = malloc(md->size);
  memset(msg, 0, md->size);
  data_init(&msg->base, UPB_DATA_HEAPALLOCATED | UPB_DATA_REFCOUNTED);
  return msg;
}

#if 0
void upb_msg_destroy(struct upb_msg *msg) {
  for(upb_field_count_t i = 0; i < msg->def->num_fields; i++) {
    struct upb_fielddef *f = &msg->def->fields[i];
    if(!upb_msg_isset(msg, f) || !upb_field_ismm(f)) continue;
    upb_mm_destroy(upb_msg_getptr(msg, f), upb_field_ptrtype(f));
  }
  upb_def_unref(UPB_UPCAST(msg->def));
  free(msg);
}

void upb_array_destroy(struct upb_array *arr)
{
  if(upb_elem_ismm(arr->fielddef)) {
    upb_arraylen_t i;
    /* Unref elements. */
    for(i = 0; i < arr->size; i++) {
      union upb_value_ptr p = upb_array_getelementptr(arr, i);
      upb_mm_destroy(p, upb_elem_ptrtype(arr->fielddef));
    }
  }
  if(arr->size != 0) free(arr->elements._void);
  free(arr);
}
#endif
