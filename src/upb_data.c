/*
 * upb - a minimalist implementation of protocol buffers.
 *
 * Copyright (c) 2009 Joshua Haberman.  See LICENSE for details.
 */

#include <stdlib.h>
#include "upb_data.h"
#include "upb_decoder.h"
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

static void data_elem_unref(union upb_value_ptr p, struct upb_fielddef *f) {
  if(upb_issubmsg(f)) {
    upb_msg_unref(*p.msg, upb_downcast_msgdef(f->def));
  } else if(upb_isstring(f)) {
    upb_string_unref(*p.str);
  } else {
    assert(false);
  }
}

static void data_unref(union upb_value_ptr p, struct upb_fielddef *f) {
  if(upb_isarray(f)) {
    upb_array_unref(*p.arr, f);
  } else {
    data_elem_unref(p, f);
  }
}

INLINE void data_init(upb_data *d, int flags) {
  d->v = REFCOUNT_ONE | flags;
}

static void check_not_frozen(upb_data *d) {
  // On one hand I am reluctant to put abort() calls in a low-level library
  // that are enabled in a production build.  On the other hand, this is a bug
  // in the client code that we cannot recover from, and it seems better to get
  // the error here than later.
  if(upb_data_hasflag(d, UPB_DATA_FROZEN)) abort();
}


/* upb_string *******************************************************************/

void _upb_string_setptr(upb_strptr s, char *ptr) {
  if(upb_data_hasflag(s.base, UPB_DATA_REFCOUNTED))
    s.refcounted->ptr = ptr;
  else
    s.norefcount->ptr = ptr;
}

static void _upb_string_set_bytelen(upb_strptr s, upb_strlen_t newlen) {
  if(upb_data_hasflag(s.base, UPB_DATA_REFCOUNTED)) {
    s.refcounted->byte_len = newlen;
  } else {
    s.norefcount->byte_len = newlen;
  }
}

upb_strptr upb_string_new() {
  upb_strptr s;
  s.refcounted = malloc(sizeof(struct upb_refcounted_string));
  data_init(s.base, UPB_DATA_HEAPALLOCATED | UPB_DATA_REFCOUNTED);
  s.refcounted->byte_size = 0;
  s.refcounted->byte_len = 0;
  s.refcounted->ptr = NULL;
  return s;
}

static upb_strlen_t string_get_bytesize(upb_strptr s) {
  if(upb_data_hasflag(s.base, UPB_DATA_REFCOUNTED)) {
    return s.refcounted->byte_size;
  } else {
    return (s.norefcount->byte_size_and_flags & 0xFFFFFFF8) >> 3;
  }
}

static void string_set_bytesize(upb_strptr s, upb_strlen_t newsize) {
  if(upb_data_hasflag(s.base, UPB_DATA_REFCOUNTED)) {
    s.refcounted->byte_size = newsize;
  } else {
    s.norefcount->byte_size_and_flags &= 0x7;
    s.norefcount->byte_size_and_flags |= (newsize << 3);
  }
}

void _upb_string_free(upb_strptr s)
{
  if(string_get_bytesize(s) != 0) free((void*)upb_string_getrobuf(s));
  free(s.base);
}

void upb_string_resize(upb_strptr s, upb_strlen_t byte_len) {
  check_not_frozen(s.base);
  if(string_get_bytesize(s) < byte_len) {
    // Need to resize.
    size_t new_byte_size = round_up_to_pow2(byte_len);
    _upb_string_setptr(s, realloc(_upb_string_getptr(s), new_byte_size));
    string_set_bytesize(s, new_byte_size);
  }
  _upb_string_set_bytelen(s, byte_len);
}

upb_strptr upb_string_getref(upb_strptr s, int ref_flags) {
  if(_upb_data_incref(s.base, ref_flags)) return s;
  upb_strptr copy = upb_strdup(s);
  if(ref_flags == UPB_REF_FROZEN)
    upb_data_setflag(copy.base, UPB_DATA_FROZEN);
  return copy;
}

upb_strptr upb_strreadfile(const char *filename) {
  FILE *f = fopen(filename, "rb");
  if(!f) return UPB_STRING_NULL;
  if(fseek(f, 0, SEEK_END) != 0) goto error;
  long size = ftell(f);
  if(size < 0) goto error;
  if(fseek(f, 0, SEEK_SET) != 0) goto error;
  upb_strptr s = upb_string_new();
  char *buf = upb_string_getrwbuf(s, size);
  if(fread(buf, size, 1, f) != 1) goto error;
  fclose(f);
  return s;

error:
  fclose(f);
  return UPB_STRING_NULL;
}

upb_strptr upb_strdupc(const char *src) {
  upb_strptr copy = upb_string_new();
  upb_strlen_t len = strlen(src);
  char *buf = upb_string_getrwbuf(copy, len);
  memcpy(buf, src, len);
  return copy;
}

void upb_strcat(upb_strptr s, upb_strptr append) {
  upb_strlen_t s_len = upb_strlen(s);
  upb_strlen_t append_len = upb_strlen(append);
  upb_strlen_t newlen = s_len + append_len;
  memcpy(upb_string_getrwbuf(s, newlen) + s_len,
         upb_string_getrobuf(append), append_len);
}

upb_strptr upb_strslice(upb_strptr s, int offset, int len) {
  upb_strptr slice = upb_string_new();
  len = UPB_MIN((upb_strlen_t)len, upb_strlen(s) - (upb_strlen_t)offset);
  memcpy(upb_string_getrwbuf(slice, len), upb_string_getrobuf(s) + offset, len);
  return slice;
}

upb_strptr upb_strdup(upb_strptr s) {
  upb_strptr copy = upb_string_new();
  upb_strcpy(copy, s);
  return copy;
}

int upb_strcmp(upb_strptr s1, upb_strptr s2) {
  upb_strlen_t common_length = UPB_MIN(upb_strlen(s1), upb_strlen(s2));
  int common_diff = memcmp(upb_string_getrobuf(s1), upb_string_getrobuf(s2),
                           common_length);
  return common_diff ==
      0 ? ((int)upb_strlen(s1) - (int)upb_strlen(s2)) : common_diff;
}


/* upb_array ******************************************************************/

static void _upb_array_setptr(upb_arrayptr a, void *ptr) {
  if(upb_data_hasflag(a.base, UPB_DATA_REFCOUNTED))
    a.refcounted->elements._void = ptr;
  else
    a.norefcount->elements._void = ptr;
}

static void _upb_array_setlen(upb_arrayptr a, upb_strlen_t newlen) {
  if(upb_data_hasflag(a.base, UPB_DATA_REFCOUNTED)) {
    a.refcounted->len = newlen;
  } else {
    a.norefcount->len = newlen;
  }
}

upb_arrayptr upb_array_new() {
  upb_arrayptr a;
  a.refcounted = malloc(sizeof(struct upb_refcounted_array));
  data_init(a.base, UPB_DATA_HEAPALLOCATED | UPB_DATA_REFCOUNTED);
  a.refcounted->size = 0;
  a.refcounted->len = 0;
  a.refcounted->elements._void = NULL;
  return a;
}

// ONLY handles refcounted arrays for the moment.
void _upb_array_free(upb_arrayptr a, struct upb_fielddef *f)
{
  if(upb_elem_ismm(f)) {
    for(upb_arraylen_t i = 0; i < a.refcounted->size; i++) {
      union upb_value_ptr p = _upb_array_getptr(a, f, i);
      if(!*p.data) continue;
      data_elem_unref(p, f);
    }
  }
  if(a.refcounted->size != 0) free(a.refcounted->elements._void);
  free(a.refcounted);
}

static upb_arraylen_t array_get_size(upb_arrayptr a) {
  if(upb_data_hasflag(a.base, UPB_DATA_REFCOUNTED)) {
    return a.refcounted->size;
  } else {
    return (a.norefcount->base.v & 0xFFFFFFF8) >> 3;
  }
}

static void array_set_size(upb_arrayptr a, upb_arraylen_t newsize) {
  if(upb_data_hasflag(a.base, UPB_DATA_REFCOUNTED)) {
    a.refcounted->size = newsize;
  } else {
    a.norefcount->base.v &= 0x7;
    a.norefcount->base.v |= (newsize << 3);
  }
}

void upb_array_resize(upb_arrayptr a, struct upb_fielddef *f, upb_strlen_t len) {
  check_not_frozen(a.base);
  size_t type_size = upb_type_info[f->type].size;
  upb_arraylen_t old_size = array_get_size(a);
  if(old_size < len) {
    // Need to resize.
    size_t new_size = round_up_to_pow2(len);
    _upb_array_setptr(a, realloc(_upb_array_getptr_raw(a, 0, 0)._void, new_size * type_size));
    array_set_size(a, new_size);
    memset(_upb_array_getptr_raw(a, old_size, type_size)._void,
           0,
           (new_size - old_size) * type_size);
  }
  _upb_array_setlen(a, len);
}


/* upb_msg ********************************************************************/

static void upb_msg_sethas(upb_msg *msg, struct upb_fielddef *f) {
  msg->data[f->field_index/8] |= (1 << (f->field_index % 8));
}

upb_msg *upb_msg_new(struct upb_msgdef *md) {
  upb_msg *msg = malloc(md->size);
  memset(msg, 0, md->size);
  data_init(&msg->base, UPB_DATA_HEAPALLOCATED | UPB_DATA_REFCOUNTED);
  upb_def_ref(UPB_UPCAST(md));
  return msg;
}

// ONLY handles refcounted messages for the moment.
void _upb_msg_free(upb_msg *msg, struct upb_msgdef *md)
{
  for(int i = 0; i < md->num_fields; i++) {
    struct upb_fielddef *f = &md->fields[i];
    union upb_value_ptr p = _upb_msg_getptr(msg, f);
    if(!upb_field_ismm(f) || !*p.data) continue;
    data_unref(p, f);
  }
  upb_def_unref(UPB_UPCAST(md));
  free(msg);
}

void upb_msg_decodestr(upb_msg *msg, struct upb_msgdef *md, upb_strptr str,
                       struct upb_status *status)
{
  upb_decoder *d = upb_decoder_new(md);
  upb_msgsink *s = upb_msgsink_new(md);

  upb_msgsink_reset(s, msg);
  upb_decoder_reset(d, upb_msgsink_sink(s));
  upb_msg_clear(msg, md);
  upb_decoder_decode(d, str, status);

  upb_decoder_free(d);
  upb_msgsink_free(s);
}


/* upb_msgsrc  ****************************************************************/

static void _upb_msgsrc_produceval(union upb_value v, struct upb_fielddef *f,
                                   upb_sink *sink, bool reverse)
{
  if(upb_issubmsg(f)) {
    upb_sink_onstart(sink, f);
    upb_msgsrc_produce(v.msg, upb_downcast_msgdef(f->def), sink, reverse);
    upb_sink_onend(sink, f);
  } else if(upb_isstring(f)) {
    upb_sink_onstr(sink, f, v.str, 0, upb_strlen(v.str));
  } else {
    upb_sink_onvalue(sink, f, v);
  }
}

void upb_msgsrc_produce(upb_msg *msg, struct upb_msgdef *md, upb_sink *sink,
                        bool reverse)
{
  for(int i = 0; i < md->num_fields; i++) {
    struct upb_fielddef *f = &md->fields[reverse ? md->num_fields - i - 1 : i];
    if(!upb_msg_has(msg, f)) continue;
    union upb_value v = upb_msg_get(msg, f);
    if(upb_isarray(f)) {
      upb_arrayptr arr = v.arr;
      upb_arraylen_t len = upb_array_len(arr);
      for(upb_arraylen_t j = 0; j < upb_array_len(arr); j++) {
        union upb_value elem = upb_array_get(arr, f, reverse ? len - j - 1 : j);
        _upb_msgsrc_produceval(elem, f, sink, reverse);
      }
    } else {
      _upb_msgsrc_produceval(v, f, sink, reverse);
    }
  }
}


/* upb_msgsink  ***************************************************************/

struct upb_msgsink_frame {
  upb_msg *msg;
  struct upb_msgdef *md;
};

struct upb_msgsink {
  upb_sink base;
  struct upb_msgdef *toplevel_msgdef;
  struct upb_msgsink_frame stack[UPB_MAX_NESTING], *top;
};

/* Helper function that returns a pointer to where the next value for field "f"
 * should be stored, taking into account whether f is an array that may need to
 * be allocated or resized. */
static union upb_value_ptr get_value_ptr(upb_msg *msg, struct upb_fielddef *f)
{
  union upb_value_ptr p = _upb_msg_getptr(msg, f);
  if(upb_isarray(f)) {
    if(!upb_msg_has(msg, f)) {
      if(upb_array_isnull(*p.arr) || !upb_data_only(*p.data)) {
        if(!upb_array_isnull(*p.arr))
          upb_array_unref(*p.arr, f);
        *p.arr = upb_array_new();
      }
      upb_array_truncate(*p.arr);
      upb_msg_sethas(msg, f);
    } else {
      assert(!upb_array_isnull(*p.arr));
    }
    upb_arraylen_t oldlen = upb_array_len(*p.arr);
    upb_array_resize(*p.arr, f, oldlen + 1);
    p = _upb_array_getptr(*p.arr, f, oldlen);
  }
  return p;
}

// Callbacks for upb_sink.
// TODO: implement these in terms of public interfaces.

static upb_sink_status _upb_msgsink_valuecb(upb_sink *s, struct upb_fielddef *f,
                                            union upb_value val)
{
  upb_msgsink *ms = (upb_msgsink*)s;
  upb_msg *msg = ms->top->msg;
  union upb_value_ptr p = get_value_ptr(msg, f);
  upb_msg_sethas(msg, f);
  upb_value_write(p, val, f->type);
  return UPB_SINK_CONTINUE;
}

static upb_sink_status _upb_msgsink_strcb(upb_sink *s, struct upb_fielddef *f,
                                          upb_strptr str,
                                          int32_t start, uint32_t end)
{
  upb_msgsink *ms = (upb_msgsink*)s;
  upb_msg *msg = ms->top->msg;
  union upb_value_ptr p = get_value_ptr(msg, f);
  upb_msg_sethas(msg, f);
  if(end > upb_strlen(str)) abort();  /* TODO: support streaming. */
  if(upb_string_isnull(*p.str) || !upb_data_only(*p.data)) {
    if(!upb_string_isnull(*p.str))
      upb_string_unref(*p.str);
    *p.str = upb_string_new();
  }
  upb_strcpylen(*p.str, upb_string_getrobuf(str) + start, end - start);
  return UPB_SINK_CONTINUE;
}

static upb_sink_status _upb_msgsink_startcb(upb_sink *s, struct upb_fielddef *f)
{
  upb_msgsink *ms = (upb_msgsink*)s;
  upb_msg *oldmsg = ms->top->msg;
  union upb_value_ptr p = get_value_ptr(oldmsg, f);

  if(upb_isarray(f) || !upb_msg_has(oldmsg, f)) {
    struct upb_msgdef *md = upb_downcast_msgdef(f->def);
    if(!*p.msg || !upb_data_only(*p.data)) {
      if(*p.msg)
        upb_msg_unref(*p.msg, md);
      *p.msg = upb_msg_new(md);
    }
    upb_msg_clear(*p.msg, md);
    upb_msg_sethas(oldmsg, f);
  }

  ms->top++;
  ms->top->msg = *p.msg;
  return UPB_SINK_CONTINUE;
}

static upb_sink_status _upb_msgsink_endcb(upb_sink *s, struct upb_fielddef *f)
{
  (void)f;  // Unused.
  upb_msgsink *ms = (upb_msgsink*)s;
  ms->top--;
  return UPB_SINK_CONTINUE;
}

static upb_sink_callbacks _upb_msgsink_vtbl = {
  _upb_msgsink_valuecb,
  _upb_msgsink_strcb,
  _upb_msgsink_startcb,
  _upb_msgsink_endcb
};

//
// External upb_msgsink interface.
//

upb_msgsink *upb_msgsink_new(struct upb_msgdef *md)
{
  upb_msgsink *ms = malloc(sizeof(*ms));
  upb_sink_init(&ms->base, &_upb_msgsink_vtbl);
  ms->toplevel_msgdef = md;
  return ms;
}

void upb_msgsink_free(upb_msgsink *sink)
{
  free(sink);
}

upb_sink *upb_msgsink_sink(upb_msgsink *sink)
{
  return &sink->base;
}

void upb_msgsink_reset(upb_msgsink *ms, upb_msg *msg)
{
  ms->top = ms->stack;
  ms->top->msg = msg;
  ms->top->md = ms->toplevel_msgdef;
}
