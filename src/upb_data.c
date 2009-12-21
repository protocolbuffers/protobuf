/*
 * upb - a minimalist implementation of protocol buffers.
 *
 * Copyright (c) 2009 Joshua Haberman.  See LICENSE for details.
 */

#include "upb_data.h"

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

upb_string *upb_string_new() {
  upb_string *s = malloc(sizeof(*s));
  s->byte_size = 0;
  s->byte_len = 0;
  s->ptr = NULL;
  s->is_heap_allocated = true;
  s->is_frozen = false;
  return s;
}

static void _upb_string_free(upb_string *s)
{
  if(s->byte_size != 0) free(s->ptr);
  free(s);
}

INLINE upb_string *upb_string_getref(upb_string *s, upb_flags_t ref_flags) {
  if((ref_flags == UPB_REF_FROZEN && !s->is_frozen && s
      upb_atomic_read((void*)(s + 1)) > 1) ||
     (ref_flags == UPB_REF_MUTABLE && s->is_frozen && 
}

char *upb_string_getrwbuf(upb_string *s, upb_strlen_t byte_len)
{
  if(s->byte_size < byte_len) {
    // Need to resize.
    s->byte_size = round_up_to_pow2(byte_len);
    s->ptr = realloc(s->ptr, s->byte_size);
  }
  s->byte_len = byte_len;
  return s->ptr;
}

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

