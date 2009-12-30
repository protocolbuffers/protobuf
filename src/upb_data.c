/*
 * upb - a minimalist implementation of protocol buffers.
 *
 * Copyright (c) 2009 Joshua Haberman.  See LICENSE for details.
 */

#include <stdlib.h>
#include "upb_data.h"
#include "upb_def.h"

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

/* upb_data *******************************************************************/

static void data_elem_unref(void *d, struct upb_fielddef *f) {
  if(f->type == UPB_TYPE(MESSAGE) || f->type == UPB_TYPE(GROUP)) {
    upb_msg_unref((upb_msg*)d, upb_downcast_msgdef(f->def));
  } else if(f->type == UPB_TYPE(STRING) || f->type == UPB_TYPE(BYTES)) {
    upb_string_unref((upb_string*)d);
  }
}

static void data_unref(void *d, struct upb_fielddef *f) {
  if(upb_isarray(f)) {
    upb_array_unref((upb_array*)d, f);
  } else {
    data_elem_unref(d, f);
  }
}

INLINE void data_init(upb_data *d, int flags) {
  d->v = flags;
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


/* upb_string *******************************************************************/

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

upb_string *upb_strreadfile(const char *filename) {
  FILE *f = fopen(filename, "rb");
  if(!f) return false;
  if(fseek(f, 0, SEEK_END) != 0) goto error;
  long size = ftell(f);
  if(size < 0) goto error;
  if(fseek(f, 0, SEEK_SET) != 0) goto error;
  upb_string *s = upb_string_new();
  char *buf = upb_string_getrwbuf(s, size);
  if(fread(buf, size, 1, f) != 1) goto error;
  fclose(f);
  return s;

error:
  fclose(f);
  return NULL;
}


/* upb_array ******************************************************************/

// ONLY handles refcounted arrays for the moment.
void _upb_array_free(upb_array *a, struct upb_fielddef *f)
{
  if(upb_elem_ismm(f)) {
    for(upb_arraylen_t i = 0; i < a->refcounted.size; i++) {
      union upb_value_ptr p = _upb_array_getptr(a, f, i);
      data_elem_unref(p._void, f);
    }
  }
  if(a->refcounted.size != 0) free(a->common.elements._void);
  free(a);
}


/* upb_msg ********************************************************************/

upb_msg *upb_msg_new(struct upb_msgdef *md) {
  upb_msg *msg = malloc(md->size);
  memset(msg, 0, md->size);
  data_init(&msg->base, UPB_DATA_HEAPALLOCATED | UPB_DATA_REFCOUNTED);
  return msg;
}

// ONLY handles refcounted messages for the moment.
void _upb_msg_free(upb_msg *msg, struct upb_msgdef *md)
{
  for(int i = 0; i < md->num_fields; i++) {
    struct upb_fielddef *f = &md->fields[i];
    union upb_value_ptr p = _upb_msg_getptr(msg, f);
    if(!upb_field_ismm(f) || !p._void) continue;
    data_unref(p._void, f);
  }
  upb_def_unref(UPB_UPCAST(md));
  free(msg);
}

