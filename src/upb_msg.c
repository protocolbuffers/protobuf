/*
 * upb - a minimalist implementation of protocol buffers.
 *
 * Copyright (c) 2009 Joshua Haberman.  See LICENSE for details.
 */

#include <inttypes.h>
#include <stdlib.h>
#include "descriptor.h"
#include "upb_msg.h"
#include "upb_parse.h"
#include "upb_serialize.h"

/* Rounds p up to the next multiple of t. */
#define ALIGN_UP(p, t) ((p) % (t) == 0 ? (p) : (p) + ((t) - ((p) % (t))))

static int div_round_up(int numerator, int denominator) {
  /* cf. http://stackoverflow.com/questions/17944/how-to-round-up-the-result-of-integer-division */
  return numerator > 0 ? (numerator - 1) / denominator + 1 : 0;
}

/* Callback for sorting fields. */
static int compare_fields(const void *e1, const void *e2) {
  const google_protobuf_FieldDescriptorProto *fd1 = *(void**)e1;
  const google_protobuf_FieldDescriptorProto *fd2 = *(void**)e2;
  /* Required fields go before non-required. */
  bool req1 = fd1->label == GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_LABEL_REQUIRED;
  bool req2 = fd2->label == GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_LABEL_REQUIRED;
  if(req1 != req2) {
    return req2 - req1;
  } else {
    /* Within required and non-required field lists, list in number order.
     * TODO: consider ordering by data size to reduce padding. */
    return fd1->number - fd2->number;
  }
}

void upb_msg_sortfds(google_protobuf_FieldDescriptorProto **fds, size_t num)
{
  qsort(fds, num, sizeof(void*), compare_fields);
}

bool upb_msg_init(struct upb_msg *m, google_protobuf_DescriptorProto *d,
                  struct upb_string fqname, bool sort)
{
  /* TODO: more complete validation. */
  if(!d->set_flags.has.field) return false;

  upb_inttable_init(&m->fields_by_num, d->field->len,
                    sizeof(struct upb_fieldsbynum_entry));
  upb_strtable_init(&m->fields_by_name, d->field->len,
                    sizeof(struct upb_fieldsbyname_entry));

  m->descriptor = d;
  m->fqname = fqname;
  m->num_fields = d->field->len;
  m->set_flags_bytes = div_round_up(m->num_fields, 8);
  /* These are incremented in the loop. */
  m->num_required_fields = 0;
  m->size = m->set_flags_bytes;

  m->fields = malloc(sizeof(*m->fields) * m->num_fields);
  m->field_descriptors = malloc(sizeof(*m->field_descriptors) * m->num_fields);
  for(unsigned int i = 0; i < m->num_fields; i++) {
    /* We count on the caller to keep this pointer alive. */
    m->field_descriptors[i] = d->field->elements[i];
  }
  if(sort) upb_msg_sortfds(m->field_descriptors, m->num_fields);

  size_t max_align = 0;
  for(unsigned int i = 0; i < m->num_fields; i++) {
    struct upb_msg_field *f = &m->fields[i];
    google_protobuf_FieldDescriptorProto *fd = m->field_descriptors[i];
    struct upb_type_info *type_info = &upb_type_info[fd->type];

    /* General alignment rules are: each member must be at an address that is a
     * multiple of that type's alignment.  Also, the size of the structure as
     * a whole must be a multiple of the greatest alignment of any member. */
    f->field_index = i;
    f->byte_offset = ALIGN_UP(m->size, type_info->align);
    f->type = fd->type;
    f->label = fd->label;
    m->size = f->byte_offset + type_info->size;
    max_align = max(max_align, type_info->align);
    if(fd->label == GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_LABEL_REQUIRED)
      m->num_required_fields++;

    /* Insert into the tables.  Note that f->ref will be uninitialized, even in
     * the tables' copies of *f, which is why we must update them separately
     * in upb_msg_ref() below. */
    struct upb_fieldsbynum_entry nument = {.e = {.key = fd->number}, .f = *f};
    struct upb_fieldsbyname_entry strent = {.e = {.key = *fd->name}, .f = *f};
    upb_inttable_insert(&m->fields_by_num, &nument.e);
    upb_strtable_insert(&m->fields_by_name, &strent.e);
  }

  m->size = ALIGN_UP(m->size, max_align);
  return true;
}

void upb_msg_free(struct upb_msg *m)
{
  upb_inttable_free(&m->fields_by_num);
  upb_strtable_free(&m->fields_by_name);
  free(m->fields);
  free(m->field_descriptors);
}

void upb_msg_ref(struct upb_msg *m, struct upb_msg_field *f,
                 union upb_symbol_ref ref) {
  struct google_protobuf_FieldDescriptorProto *d =
      upb_msg_field_descriptor(f, m);
  struct upb_fieldsbynum_entry *int_e = upb_inttable_fast_lookup(
      &m->fields_by_num, d->number, sizeof(struct upb_fieldsbynum_entry));
  struct upb_fieldsbyname_entry *str_e =
      upb_strtable_lookup(&m->fields_by_name, d->name);
  assert(int_e && str_e);
  f->ref = ref;
  int_e->f.ref = ref;
  str_e->f.ref = ref;
}

/* Memory management  *********************************************************/

/* Our memory management scheme is as follows:
 *
 * All pointers to dynamic memory (strings, arrays, and submessages) are
 * expected to be good pointers if they are non-zero, *regardless* of whether
 * that field's bit is set!  That way we can reuse the memory even if the field
 * is unset and then set later. */

/* For our memory-managed strings and arrays we store extra information
 * (compared to a plain upb_string or upb_array).  But the data starts with
 * a upb_string and upb_array, so we can overlay onto the regular types. */
struct mm_upb_string {
  struct upb_string s;
  /* Track the allocated size, so we know when we need to reallocate. */
  uint32_t size;
  /* Our allocated data.  Stored separately so that clients can point s.ptr to
   * a referenced string, but we can reuse this data later. */
  char *data;
};

struct mm_upb_array {
  struct upb_array a;
  /* Track the allocated size, so we know when we need to reallocate. */
  uint32_t size;
};

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

void *upb_msgdata_new(struct upb_msg *m)
{
  void *msg = malloc(m->size);
  memset(msg, 0, m->size);  /* Clear all pointers, values, and set bits. */
  return msg;
}

static void free_value(union upb_value_ptr p, struct upb_msg_field *f,
                       bool free_submsgs)
{
  switch(f->type) {
    case GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_TYPE_STRING:
    case GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_TYPE_BYTES: {
      struct mm_upb_string *mm_str = (void*)*p.str;
      if(mm_str) {
        free(mm_str->data);
        free(mm_str);
      }
      break;
    }
    case GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_TYPE_MESSAGE:
    case GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_TYPE_GROUP:
      if(free_submsgs) upb_msgdata_free(*p.msg, f->ref.msg, free_submsgs);
      break;
    default: break;  /* For non-dynamic types, do nothing. */
  }
}

void upb_msgdata_free(void *data, struct upb_msg *m, bool free_submsgs)
{
  if(!data) return;  /* A very free-like thing to do. */
  for(unsigned int i = 0; i < m->num_fields; i++) {
    struct upb_msg_field *f = &m->fields[i];
    union upb_value_ptr p = upb_msg_getptr(data, f);
    if(f->label == GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_LABEL_REPEATED) {
      if(*p.arr) {
        for(uint32_t j = 0; j < (*p.arr)->len; j++)
          free_value(upb_array_getelementptr(*p.arr, j, f->type),
                     f, free_submsgs);
        free((*p.arr)->elements._void);
        free(*p.arr);
      }
    } else {
      free_value(p, f, free_submsgs);
    }
  }
  free(data);
}

void upb_msg_reuse_str(struct upb_string **str, uint32_t size)
{
  if(!*str) {
    *str = malloc(sizeof(struct mm_upb_string));
    memset(*str, 0, sizeof(struct mm_upb_string));
  }
  struct mm_upb_string *s = (void*)*str;
  if(s->size < size) {
    size = max(16, round_up_to_pow2(size));
    s->data = realloc(s->data, size);
    s->size = size;
  }
  s->s.ptr = s->data;
}

void upb_msg_reuse_array(struct upb_array **arr, uint32_t size, upb_field_type_t t)
{
  if(!*arr) {
    *arr = malloc(sizeof(struct mm_upb_array));
    memset(*arr, 0, sizeof(struct mm_upb_array));
  }
  struct mm_upb_array *a = (void*)*arr;
  if(a->size < size) {
    size = max(4, round_up_to_pow2(size));
    size_t type_size = upb_type_info[t].size;
    a->a.elements._void = realloc(a->a.elements._void, size * type_size);
    /* Zero any newly initialized memory. */
    memset(UPB_INDEX(a->a.elements._void, a->size, type_size), 0,
           (size - a->size) * type_size);
    a->size = size;
  }
}

void upb_msg_reuse_strref(struct upb_string **str) { upb_msg_reuse_str(str, 0); }

void upb_msg_reuse_submsg(void **msg, struct upb_msg *m)
{
  if(!*msg) *msg = upb_msgdata_new(m);
}

/* Parsing.  ******************************************************************/

static upb_field_type_t tag_cb(void *udata, struct upb_tag *tag,
                               void **user_field_desc)
{
  struct upb_msg_parse_state *s = udata;
  struct upb_msg_field *f = upb_msg_fieldbynum(s->top->m, tag->field_number);
  if(!f || !upb_check_type(tag->wire_type, f->type))
    return 0;  /* Skip unknown or fields of the wrong type. */
  *user_field_desc = f;
  return f->type;
}

/* Returns a pointer to where the next value for field "f" should be stored,
 * taking into account whether f is an array that may need to be reallocatd. */
static union upb_value_ptr get_value_ptr(void *data, struct upb_msg_field *f)
{
  union upb_value_ptr p = upb_msg_getptr(data, f);
  if(upb_isarray(f)) {
    size_t len = upb_msg_isset(data, f) ? (*p.arr)->len : 0;
    upb_msg_reuse_array(p.arr, len+1, f->type);
    (*p.arr)->len = len + 1;
    assert(p._void);
    p = upb_array_getelementptr(*p.arr, len, f->type);
    assert(p._void);
  }
  assert(p._void);
  return p;
}

static upb_status_t value_cb(void *udata, uint8_t *buf, uint8_t *end,
                             void *user_field_desc, uint8_t **outbuf)
{
  struct upb_msg_parse_state *s = udata;
  struct upb_msg_field *f = user_field_desc;
  union upb_value_ptr p = get_value_ptr(s->top->data, f);
  upb_msg_set(s->top->data, f);
  UPB_CHECK(upb_parse_value(buf, end, f->type, p, outbuf));
  //google_protobuf_FieldDescriptorProto *fd = upb_msg_field_descriptor(f, s->top->m);
  //upb_text_printfield(&s->p, *fd->name, f->type, upb_deref(p, f->type), stdout);
  return UPB_STATUS_OK;
}

static void str_cb(void *udata, uint8_t *str,
                   size_t avail_len, size_t total_len,
                   void *udesc)
{
  struct upb_msg_parse_state *s = udata;
  struct upb_msg_field *f = udesc;
  union upb_value_ptr p = get_value_ptr(s->top->data, f);
  upb_msg_set(s->top->data, f);
  if(avail_len != total_len) abort();  /* TODO: support streaming. */
  if(s->byref) {
    upb_msg_reuse_strref(p.str);
    (*p.str)->ptr = (char*)str;
    (*p.str)->byte_len = avail_len;
  } else {
    upb_msg_reuse_str(p.str, avail_len);
    memcpy((*p.str)->ptr, str, avail_len);
    (*p.str)->byte_len = avail_len;
  }
  //google_protobuf_FieldDescriptorProto *fd = upb_msg_field_descriptor(f, s->top->m);
  //upb_text_printfield(&s->p, *fd->name, f->type, upb_deref(p, fd->type), stdout);
}

static void submsg_start_cb(void *udata, void *user_field_desc)
{
  struct upb_msg_parse_state *s = udata;
  struct upb_msg_field *f = user_field_desc;
  struct upb_msg *m = f->ref.msg;
  void *data = s->top->data;  /* The message from the existing frame. */
  union upb_value_ptr p = get_value_ptr(data, f);
  upb_msg_reuse_submsg(p.msg, m);
  if(!upb_msg_isset(data, f) || !s->merge)
    upb_msg_clear(*p.msg, m);
  upb_msg_set(data, f);
  s->top++;
  s->top->m = m;
  s->top->data = *p.msg;
  //upb_text_push(&s->p, *s->top->m->descriptor->name, stdout);
}

static void submsg_end_cb(void *udata)
{
  struct upb_msg_parse_state *s = udata;
  s->top--;
  //upb_text_pop(&s->p, stdout);
}

void upb_msg_parse_reset(struct upb_msg_parse_state *s, void *msg,
                         struct upb_msg *m, bool merge, bool byref)
{
  upb_parse_reset(&s->s, s);
  upb_text_printer_init(&s->p, false);
  s->merge = merge;
  s->byref = byref;
  if(!merge && msg == NULL) msg = upb_msgdata_new(m);
  upb_msg_clear(msg, m);
  s->top = s->stack;
  s->top->m = m;
  s->top->data = msg;
  s->s.tag_cb = tag_cb;
  s->s.value_cb = value_cb;
  s->s.str_cb = str_cb;
  s->s.submsg_start_cb = submsg_start_cb;
  s->s.submsg_end_cb = submsg_end_cb;
}

void upb_msg_parse_init(struct upb_msg_parse_state *s, void *msg,
                        struct upb_msg *m, bool merge, bool byref)
{
  upb_parse_init(&s->s, s);
  upb_msg_parse_reset(s, msg, m, merge, byref);
}

void upb_msg_parse_free(struct upb_msg_parse_state *s)
{
  upb_parse_free(&s->s);
}

upb_status_t upb_msg_parse(struct upb_msg_parse_state *s,
                           void *data, size_t len, size_t *read)
{
  return upb_parse(&s->s, data, len, read);
}

void *upb_alloc_and_parse(struct upb_msg *m, struct upb_string *str, bool byref)
{
  struct upb_msg_parse_state s;
  void *msg = upb_msgdata_new(m);
  upb_msg_parse_init(&s, msg, m, false, byref);
  size_t read;
  upb_status_t status = upb_msg_parse(&s, str->ptr, str->byte_len, &read);
  upb_msg_parse_free(&s);
  if(status == UPB_STATUS_OK && read == str->byte_len) {
    return msg;
  } else {
    upb_msg_free(msg);
    return NULL;
  }
}

/* Serialization.  ************************************************************/

/* We store the message sizes linearly in post-order (size of parent after sizes
 * of children) for a right-to-left traversal of the message tree.  Iterating
 * over this in reverse gives us a pre-order (size of parent before sizes of
 * children) left-to-right traversal, which is what we want for parsing. */
struct upb_msgsizes {
  int len;
  int size;
  size_t *sizes;
};

/* Declared below -- this and get_valuesize are mutually recursive. */
static size_t get_msgsize(struct upb_msgsizes *sizes, void *data,
                          struct upb_msg *m);

/* Returns a size of a value as it will be serialized.  Does *not* include
 * the size of the tag -- that is already accounted for. */
static size_t get_valuesize(struct upb_msgsizes *sizes, union upb_value_ptr p,
                            struct upb_msg_field *f,
                            google_protobuf_FieldDescriptorProto *fd)
{
  switch(f->type) {
    default: assert(false); return 0;  /* Internal corruption. */
    case GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_TYPE_MESSAGE: {
      size_t submsg_size = get_msgsize(sizes, p.msg, f->ref.msg);
      return upb_get_INT32_size(submsg_size) + submsg_size;
    }
    case GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_TYPE_GROUP: {
      size_t endgrp_tag_size = upb_get_tag_size(fd->number);
      return endgrp_tag_size + get_msgsize(sizes, p.msg, f->ref.msg);
    }
#define CASE(type, member) \
    case GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_TYPE_ ## type: \
      return upb_get_ ## type ## _size(*p.member);
    CASE(DOUBLE,   _double)
    CASE(FLOAT,    _float)
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
    CASE(BOOL,     _bool)
    CASE(ENUM,     int32)
#undef CASE
  }
}

/* This is mostly just a pure recursive function to calculate the size of a
 * message.  However it also stores the results of each level of the recursion
 * in sizes, because we need all of this intermediate information later. */
static size_t get_msgsize(struct upb_msgsizes *sizes, void *data,
                          struct upb_msg *m)
{
  size_t size = 0;
  /* We iterate over fields and arrays in reverse order. */
  for(int32_t i = m->num_fields - 1; i >= 0; i--) {
    struct upb_msg_field *f = &m->fields[i];
    google_protobuf_FieldDescriptorProto *fd = upb_msg_field_descriptor(f, m);
    if(!upb_msg_isset(data, f)) continue;
    union upb_value_ptr p = upb_msg_getptr(data, f);
    if(upb_isarray(f)) {
      for(int32_t j = (*p.arr)->len - 1; j >= 0; j--) {
        union upb_value_ptr elem = upb_array_getelementptr((*p.arr), j, f->type);
        /* TODO: for packed arrays tag size goes outside the loop. */
        size += upb_get_tag_size(fd->number);
        size += get_valuesize(sizes, elem, f, fd);
      }
    } else {
      size += upb_get_tag_size(fd->number);
      size += get_valuesize(sizes, p, f, fd);
    }
  }
  /* Resize the 'sizes' array if necessary. */
  assert(sizes->len <= sizes->size);
  if(sizes->len == sizes->size) {
    sizes->size *= 2;
    sizes->sizes = realloc(sizes->sizes, sizes->size * sizeof(size_t));
  }
  /* Add our size (already added our children, so post-order). */
  sizes->sizes[sizes->len++] = size;
  return size;
}

void upb_msgsizes_read(struct upb_msgsizes *sizes, void *data, struct upb_msg *m)
{
  get_msgsize(sizes, data, m);
}

/* Initialize/free a upb_msg_sizes for the given message. */
void upb_msgsizes_init(struct upb_msgsizes *sizes)
{
  sizes->len = 0;
  sizes->size = 0;
  sizes->sizes = NULL;
}

void upb_msgsizes_free(struct upb_msgsizes *sizes)
{
  free(sizes->sizes);
}

size_t upb_msgsizes_totalsize(struct upb_msgsizes *sizes)
{
  return sizes->sizes[sizes->len-1];
}

struct upb_msg_serialize_state {
  struct {
    int field_iter;
    int elem_iter;
    struct upb_msg *m;
    void *msg;
  } stack[UPB_MAX_NESTING], *top, *limit;
};

void upb_msg_serialize_alloc(struct upb_msg_serialize_state *s)
{
  (void)s;
}

void upb_msg_serialize_free(struct upb_msg_serialize_state *s)
{
  (void)s;
}

void upb_msg_serialize_init(struct upb_msg_serialize_state *s, void *data,
                            struct upb_msg *m, struct upb_msgsizes *sizes)
{
  (void)s;
  (void)data;
  (void)m;
  (void)sizes;
}

static upb_status_t serialize_tag(uint8_t *buf, uint8_t *end,
                                  struct upb_msg_field *f, uint8_t **outptr)
{
  /* TODO: need to have the field number also. */
  UPB_CHECK(upb_put_UINT32(buf, end, f->type, outptr));
  return UPB_STATUS_OK;
}

/* Serializes the next set of bytes into buf (which has size len).  Returns
 * UPB_STATUS_OK if serialization is complete, or UPB_STATUS_NEED_MORE_DATA
 * if there is more data from the message left to be serialized.
 *
 * The number of bytes written to buf is returned in *read.  This will be
 * equal to len unless we finished serializing. */
upb_status_t upb_msg_serialize(struct upb_msg_serialize_state *s,
                               void *_buf, size_t len, size_t *written)
{
  uint8_t *buf = _buf;
  uint8_t *end = buf + len;
  uint8_t *const start = buf;
  int i = s->top->field_iter;
  int j = s->top->elem_iter;
  void *msg = s->top->msg;
  struct upb_msg *m = s->top->m;

  while(buf < end) {
    struct upb_msg_field *f = &m->fields[i];
    union upb_value_ptr p = upb_msg_getptr(msg, f);
    serialize_tag(buf, end, f, &buf);
    if(f->type == GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_TYPE_MESSAGE) {
    } else if(f->type == GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_TYPE_GROUP) {
    } else if(upb_isstring(f)) {
    } else {
      upb_serialize_value(buf, end, f->type, p, &buf);
    }
  }
  *written = buf - start;
  return UPB_STATUS_OK;
}

/* Comparison.  ***************************************************************/

bool upb_value_eql(union upb_value_ptr p1, union upb_value_ptr p2,
                   upb_field_type_t type)
{
#define CMP(type) return *p1.type == *p2.type;
  switch(type) {
    case GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_TYPE_DOUBLE:
      CMP(_double)
    case GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_TYPE_FLOAT:
      CMP(_float)
    case GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_TYPE_INT64:
    case GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_TYPE_SFIXED64:
    case GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_TYPE_SINT64:
      CMP(int64)
    case GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_TYPE_UINT64:
    case GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_TYPE_FIXED64:
      CMP(uint64)
    case GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_TYPE_INT32:
    case GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_TYPE_SFIXED32:
    case GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_TYPE_SINT32:
      CMP(int32)
    case GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_TYPE_UINT32:
    case GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_TYPE_FIXED32:
    case GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_TYPE_ENUM:
      CMP(uint32);
    case GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_TYPE_BOOL:
      CMP(_bool);
    case GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_TYPE_STRING:
    case GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_TYPE_BYTES:
      return upb_streql(*p1.str, *p2.str);
    default: return false;
  }
}

bool upb_array_eql(struct upb_array *arr1, struct upb_array *arr2,
                   struct upb_msg_field *f, bool recursive)
{
  if(arr1->len != arr2->len) return false;
  if(upb_issubmsg(f)) {
    if(!recursive) return true;
    for(uint32_t i = 0; i < arr1->len; i++)
      if(!upb_msg_eql(arr1->elements.msg[i], arr2->elements.msg[i],
         f->ref.msg, recursive))
        return false;
  } else if(upb_isstring(f)) {
    for(uint32_t i = 0; i < arr1->len; i++)
      if(!upb_streql(arr1->elements.str[i], arr2->elements.str[i]))
        return false;
  } else {
    /* For primitive types we can compare the memory directly. */
    return memcmp(arr1->elements._void, arr2->elements._void,
                  arr1->len * upb_type_info[f->type].size) == 0;
  }
  return true;
}

bool upb_msg_eql(void *data1, void *data2, struct upb_msg *m, bool recursive)
{
  /* Must have the same fields set.  TODO: is this wrong?  Should we also
   * consider absent defaults equal to explicitly set defaults? */
  if(memcmp(data1, data2, m->set_flags_bytes) != 0)
    return false;

  /* Possible optimization: create a mask of the bytes in the messages that
   * contain only primitive values (not strings, arrays, submessages, or
   * padding) and memcmp the masked messages. */

  for(uint32_t i = 0; i < m->num_fields; i++) {
    struct upb_msg_field *f = &m->fields[i];
    if(!upb_msg_isset(data1, f)) continue;
    union upb_value_ptr p1 = upb_msg_getptr(data1, f);
    union upb_value_ptr p2 = upb_msg_getptr(data2, f);
    if(f->label == GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_LABEL_REPEATED) {
      if(!upb_array_eql(*p1.arr, *p2.arr, f, recursive)) return false;
    } else {
      if(upb_issubmsg(f)) {
        if(recursive && !upb_msg_eql(p1.msg, p2.msg, f->ref.msg, recursive))
          return false;
      } else if(!upb_value_eql(p1, p2, f->type)) {
        return false;
      }
    }
  }
  return true;
}
