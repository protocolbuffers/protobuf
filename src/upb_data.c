/*
 * upb - a minimalist implementation of protocol buffers.
 *
 * Copyright (c) 2009 Joshua Haberman.  See LICENSE for details.
 */

#include <stdlib.h>
#include "upb_data.h"
#include "upb_def.h"
#include "upb_parse.h"

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

static char *_upb_string_setptr(upb_strptr s, char *ptr) {
  if(upb_data_hasflag(s.base, UPB_DATA_REFCOUNTED))
    return s.refcounted->ptr = ptr;
  else
    return s.norefcount->ptr = ptr;
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

upb_array *upb_array_new() {
  upb_array *a = malloc(sizeof(upb_refcounted_array));
  data_init(&a->common.base, UPB_DATA_HEAPALLOCATED | UPB_DATA_REFCOUNTED);
  a->refcounted.size = 0;
  a->common.len = 0;
  a->common.elements._void = NULL;
  return a;
}

// ONLY handles refcounted arrays for the moment.
void _upb_array_free(upb_array *a, struct upb_fielddef *f)
{
  if(upb_elem_ismm(f)) {
    for(upb_arraylen_t i = 0; i < a->refcounted.size; i++) {
      union upb_value_ptr p = _upb_array_getptr(a, f, i);
      if(!*p.data) continue;
      data_elem_unref(p, f);
    }
  }
  if(a->refcounted.size != 0) free(a->common.elements._void);
  free(a);
}

static upb_arraylen_t array_get_size(upb_array *a) {
  if(upb_data_hasflag(&a->common.base, UPB_DATA_REFCOUNTED)) {
    return a->refcounted.size;
  } else {
    return (a->norefcount.size_and_flags & 0xFFFFFFF8) >> 3;
  }
}

static void array_set_size(upb_array *a, upb_arraylen_t newsize) {
  if(upb_data_hasflag(&a->common.base, UPB_DATA_REFCOUNTED)) {
    a->refcounted.size = newsize;
  } else {
    a->norefcount.size_and_flags &= 0x7;
    a->norefcount.size_and_flags |= (newsize << 3);
  }
}

void upb_array_resize(upb_array *a, struct upb_fielddef *f, upb_strlen_t len) {
  check_not_frozen(&a->common.base);
  size_t type_size = upb_type_info[f->type].size;
  upb_arraylen_t old_size = array_get_size(a);
  if(old_size < len) {
    // Need to resize.
    size_t new_size = round_up_to_pow2(len);
    a->common.elements._void = realloc(a->common.elements._void, new_size * type_size);
    array_set_size(a, new_size);
    memset(UPB_INDEX(a->common.elements._void, old_size, type_size),
           0,
           (new_size - old_size) * type_size);
  }
  a->common.len = len;
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


/* Parsing.  ******************************************************************/

struct upb_msgparser_frame {
  upb_msg *msg;
  struct upb_msgdef *md;
};

struct upb_msgparser {
  struct upb_cbparser *s;
  bool merge;
  struct upb_msgparser_frame stack[UPB_MAX_NESTING], *top;
};

/* Helper function that returns a pointer to where the next value for field "f"
 * should be stored, taking into account whether f is an array that may need to
 * be allocated or resized. */
static union upb_value_ptr get_value_ptr(upb_msg *msg, struct upb_fielddef *f)
{
  union upb_value_ptr p = _upb_msg_getptr(msg, f);
  if(upb_isarray(f)) {
    if(!upb_msg_has(msg, f)) {
      if(!*p.arr || !upb_data_only(*p.data)) {
        printf("Initializing array field " UPB_STRFMT "\n", UPB_STRARG(f->name));
        if(*p.arr)
          upb_array_unref(*p.arr, f);
        *p.arr = upb_array_new();
      } else {
        printf("REUSING array field " UPB_STRFMT "\n", UPB_STRARG(f->name));
      }
      upb_array_truncate(*p.arr);
      upb_msg_sethas(msg, f);
    } else {
      printf("APPENDING TO EXISTING array field " UPB_STRFMT "\n", UPB_STRARG(f->name));
      assert(*p.arr);
    }
    upb_arraylen_t oldlen = upb_array_len(*p.arr);
    upb_array_resize(*p.arr, f, oldlen + 1);
    p = _upb_array_getptr(*p.arr, f, oldlen);
  }
  return p;
}

// Callbacks for the stream parser.
// TODO: implement these in terms of public interfaces.

static bool value_cb(void *udata, struct upb_msgdef *msgdef,
                     struct upb_fielddef *f, union upb_value val)
{
  (void)msgdef;
  struct upb_msgparser *mp = udata;
  upb_msg *msg = mp->top->msg;
  union upb_value_ptr p = get_value_ptr(msg, f);
  upb_msg_sethas(msg, f);
  upb_value_write(p, val, f->type);
  return true;
}

static bool str_cb(void *udata, struct upb_msgdef *msgdef,
                   struct upb_fielddef *f, const uint8_t *str, size_t avail_len,
                   size_t total_len)
{
  (void)msgdef;
  struct upb_msgparser *mp = udata;
  upb_msg *msg = mp->top->msg;
  union upb_value_ptr p = get_value_ptr(msg, f);
  upb_msg_sethas(msg, f);
  if(avail_len != total_len) abort();  /* TODO: support streaming. */
  if(upb_string_isnull(*p.str) || !upb_data_only(*p.data)) {
    if(!upb_string_isnull(*p.str))
      upb_string_unref(*p.str);
    *p.str = upb_string_new();
  }
  upb_strcpylen(*p.str, str, avail_len);
  return true;
}

static void start_cb(void *udata, struct upb_fielddef *f)
{
  struct upb_msgparser *mp = udata;
  upb_msg *oldmsg = mp->top->msg;
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

  mp->top++;
  mp->top->msg = *p.msg;
}

static void end_cb(void *udata)
{
  struct upb_msgparser *mp = udata;
  mp->top--;
}

/* Externally-visible functions for the msg parser. */

struct upb_msgparser *upb_msgparser_new(struct upb_msgdef *def)
{
  struct upb_msgparser *mp = malloc(sizeof(struct upb_msgparser));
  mp->s = upb_cbparser_new(def, value_cb, str_cb, start_cb, end_cb);
  return mp;
}

void upb_msgparser_reset(struct upb_msgparser *s, upb_msg *msg)
{
  upb_cbparser_reset(s->s, s);
  s->top = s->stack;
  s->top->msg = msg;
}

void upb_msgparser_free(struct upb_msgparser *s)
{
  upb_cbparser_free(s->s);
  free(s);
}

void upb_msg_parsestr(upb_msg *msg, struct upb_msgdef *md, upb_strptr str,
                      struct upb_status *status)
{
  struct upb_msgparser *mp = upb_msgparser_new(md);
  upb_msgparser_reset(mp, msg);
  upb_msg_clear(msg, md);
  upb_msgparser_parse(mp, str, status);
  upb_msgparser_free(mp);
}

size_t upb_msgparser_parse(struct upb_msgparser *s, upb_strptr str,
                           struct upb_status *status)
{
  return upb_cbparser_parse(s->s, str, status);
}
