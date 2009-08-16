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
#include "upb_text.h"

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

void upb_msgdef_sortfds(google_protobuf_FieldDescriptorProto **fds, size_t num)
{
  qsort(fds, num, sizeof(void*), compare_fields);
}

bool upb_msgdef_init(struct upb_msgdef *m, google_protobuf_DescriptorProto *d,
                     struct upb_string fqname, bool sort, struct upb_context *c)
{
  /* TODO: more complete validation. */
  if(!d->set_flags.has.field) return false;

  upb_atomic_refcount_init(&m->refcount, 0);
  upb_inttable_init(&m->fields_by_num, d->field->len,
                    sizeof(struct upb_fieldsbynum_entry));
  upb_strtable_init(&m->fields_by_name, d->field->len,
                    sizeof(struct upb_fieldsbyname_entry));

  m->descriptor = d;
  m->fqname = fqname;
  m->context = c;
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
  if(sort) upb_msgdef_sortfds(m->field_descriptors, m->num_fields);

  size_t max_align = 0;
  for(unsigned int i = 0; i < m->num_fields; i++) {
    struct upb_msg_fielddef *f = &m->fields[i];
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
    max_align = UPB_MAX(max_align, type_info->align);
    if(fd->label == GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_LABEL_REQUIRED)
      m->num_required_fields++;

    /* Insert into the tables.  Note that f->ref will be uninitialized, even in
     * the tables' copies of *f, which is why we must update them separately
     * in upb_msg_setref() below. */
    struct upb_fieldsbynum_entry nument = {.e = {.key = fd->number}, .f = *f};
    struct upb_fieldsbyname_entry strent = {.e = {.key = *fd->name}, .f = *f};
    upb_inttable_insert(&m->fields_by_num, &nument.e);
    upb_strtable_insert(&m->fields_by_name, &strent.e);
  }

  m->size = ALIGN_UP(m->size, max_align);
  return true;
}

void upb_msgdef_free(struct upb_msgdef *m)
{
  upb_inttable_free(&m->fields_by_num);
  upb_strtable_free(&m->fields_by_name);
  free(m->fields);
  free(m->field_descriptors);
}

void upb_msgdef_setref(struct upb_msgdef *m, struct upb_msg_fielddef *f,
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

/* Simple, one-shot parsing ***************************************************/

static void *upb_msg_new(struct upb_msgdef *md)
{
  size_t size = md->size + (sizeof(void*) * 2);
  struct upb_msg *msg = malloc(size);
  memset(msg, 0, size);
  msg->def = md;
  return msg;
}

/* Allocation callbacks. */
struct upb_array *getarray_cb(
    void *from_gptr, struct upb_array *existingval, struct upb_msg_fielddef *f)
{
  (void)from_gptr;
  (void)existingval;  /* Don't care -- always zero. */
  (void)f;
  return upb_array_new();
}

static struct upb_string *getstring_cb(
    void *from_gptr, struct upb_string *existingval, struct upb_msg_fielddef *f,
    bool byref)
{
  (void)from_gptr;
  (void)existingval;  /* Don't care -- always zero. */
  (void)f;
  (void)byref;
  return upb_strnew();
}

static struct upb_msg *getmsg_cb(
    void *from_gptr, struct upb_msg *existingval, struct upb_msg_fielddef *f)
{
  (void)from_gptr;
  (void)existingval;  /* Don't care -- always zero. */
  return upb_msg_new(f->ref.msg);
}

struct upb_msg *upb_msg_parsenew(struct upb_msgdef *md, struct upb_string *s)
{
  struct upb_msg_parser mp;
  struct upb_msg *msg = upb_msg_new(md);
  upb_msg_parser_reset(&mp, msg, false);
  mp.getarray_cb = getarray_cb;
  mp.getstring_cb = getstring_cb;
  mp.getmsg_cb = getmsg_cb;
  size_t read;
  upb_status_t status = upb_msg_parser_parse(&mp, s->ptr, s->byte_len, &read);
  if(status == UPB_STATUS_OK && read == s->byte_len) {
    return msg;
  } else {
    upb_msg_free(msg);
    return NULL;
  }
}

/* For simple, one-shot parsing we assume that a dynamic field exists (and
 * needs to be freed) iff its set bit is set. */
static void free_value(union upb_value_ptr p, struct upb_msg_fielddef *f)
{
  if(upb_isstring(f)) {
    free((*p.str)->ptr);
    free(*p.str);
  } else if(upb_issubmsg(f)) {
    upb_msg_free(*p.msg);
  }
}

void upb_msg_free(struct upb_msg *msg)
{
  if(!msg) return;  /* A very free-like thing to do. */
  struct upb_msgdef *m = msg->def;
  for(unsigned int i = 0; i < m->num_fields; i++) {
    struct upb_msg_fielddef *f = &m->fields[i];
    if(!upb_msg_isset(msg, f)) continue;
    union upb_value_ptr p = upb_msg_getptr(msg, f);
    if(upb_isarray(f)) {
      assert(*p.arr);
      for(upb_arraylen_t j = 0; j < (*p.arr)->len; j++)
        free_value(upb_array_getelementptr(*p.arr, j, f->type), f);
      upb_array_free(*p.arr);
    } else {
      free_value(p, f);
    }
  }
  free(msg);
}

/* Parsing.  ******************************************************************/

/* Helper function that returns a pointer to where the next value for field "f"
 * should be stored, taking into account whether f is an array that may need to
 * be allocated or resized. */
static union upb_value_ptr get_value_ptr(struct upb_msg *msg,
                                         struct upb_msg_fielddef *f,
                                         void **gptr,
                                         upb_msg_getandref_array_cb_t getarray_cb)
{
  union upb_value_ptr p = upb_msg_getptr(msg, f);
  if(upb_isarray(f)) {
    bool isset = upb_msg_isset(msg, f);
    size_t len = isset ? (*p.arr)->len : 0;
    if(!isset) *p.arr = getarray_cb(*gptr, *p.arr, f);
    upb_array_resize(*p.arr, len+1, f->type);
    *gptr = (*p.arr)->gptr;
    p = upb_array_getelementptr(*p.arr, len, f->type);
  }
  return p;
}

/* Callbacks for the stream parser. */

static upb_field_type_t tag_cb(void *udata, struct upb_tag *tag,
                               void **user_field_desc)
{
  struct upb_msg_parser *mp = udata;
  struct upb_msg_fielddef *f =
      upb_msg_fieldbynum(mp->top->msg->def, tag->field_number);
  if(!f || !upb_check_type(tag->wire_type, f->type))
    return 0;  /* Skip unknown or fields of the wrong type. */
  *user_field_desc = f;
  return f->type;
}

static upb_status_t value_cb(void *udata, uint8_t *buf, uint8_t *end,
                             void *user_field_desc, uint8_t **outbuf)
{
  struct upb_msg_parser *mp = udata;
  struct upb_msg_fielddef *f = user_field_desc;
  struct upb_msg *msg = mp->top->msg;
  void *gptr = upb_msg_gptr(msg);
  union upb_value_ptr p = get_value_ptr(msg, f, &gptr, mp->getarray_cb);
  upb_msg_set(msg, f);
  UPB_CHECK(upb_parse_value(buf, end, f->type, p, outbuf));
  return UPB_STATUS_OK;
}

static void str_cb(void *udata, uint8_t *str,
                   size_t avail_len, size_t total_len,
                   void *udesc)
{
  struct upb_msg_parser *mp = udata;
  struct upb_msg_fielddef *f = udesc;
  struct upb_msg *msg = mp->top->msg;
  void *gptr = upb_msg_gptr(msg);
  union upb_value_ptr p = get_value_ptr(msg, f, &gptr, mp->getarray_cb);
  upb_msg_set(msg, f);
  if(avail_len != total_len) abort();  /* TODO: support streaming. */
  bool byref = avail_len == total_len && mp->byref;
  *p.str = mp->getstring_cb(gptr, *p.str, f, byref);
  if(byref) {
    upb_strdrop(*p.str);
    (*p.str)->ptr = (char*)str;
    (*p.str)->byte_len = avail_len;
  } else {
    upb_stralloc(*p.str, total_len);
    memcpy((*p.str)->ptr, str, avail_len);
    (*p.str)->byte_len = avail_len;
  }
}

static void submsg_start_cb(void *udata, void *user_field_desc)
{
  struct upb_msg_parser *mp = udata;
  struct upb_msg_fielddef *f = user_field_desc;
  struct upb_msg *oldmsg = mp->top->msg;
  void *gptr = upb_msg_gptr(oldmsg);
  union upb_value_ptr p = get_value_ptr(oldmsg, f, &gptr, mp->getarray_cb);
  upb_msg_set(oldmsg, f);
  *p.msg = mp->getmsg_cb(gptr, *p.msg, f);
  mp->top++;
  mp->top->msg = *p.msg;
}

static void submsg_end_cb(void *udata)
{
  struct upb_msg_parser *mp = udata;
  mp->top--;
}

/* Externally-visible functions for the msg parser. */

void upb_msg_parser_reset(struct upb_msg_parser *s, struct upb_msg *msg, bool byref)
{
  upb_stream_parser_reset(&s->s, s);
  s->byref = byref;
  s->top = s->stack;
  s->top->msg = msg;
  s->s.tag_cb = tag_cb;
  s->s.value_cb = value_cb;
  s->s.str_cb = str_cb;
  s->s.submsg_start_cb = submsg_start_cb;
  s->s.submsg_end_cb = submsg_end_cb;
}

upb_status_t upb_msg_parser_parse(struct upb_msg_parser *s,
                                  void *data, size_t len, size_t *read)
{
  return upb_stream_parser_parse(&s->s, data, len, read);
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
                          struct upb_msgdef *m);

/* Returns a size of a value as it will be serialized.  Does *not* include
 * the size of the tag -- that is already accounted for. */
static size_t get_valuesize(struct upb_msgsizes *sizes, union upb_value_ptr p,
                            struct upb_msg_fielddef *f,
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
                          struct upb_msgdef *m)
{
  size_t size = 0;
  /* We iterate over fields and arrays in reverse order. */
  for(int32_t i = m->num_fields - 1; i >= 0; i--) {
    struct upb_msg_fielddef *f = &m->fields[i];
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

void upb_msgsizes_read(struct upb_msgsizes *sizes, void *data, struct upb_msgdef *m)
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
    struct upb_msgdef *m;
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
                            struct upb_msgdef *m, struct upb_msgsizes *sizes)
{
  (void)s;
  (void)data;
  (void)m;
  (void)sizes;
}

static upb_status_t serialize_tag(uint8_t *buf, uint8_t *end,
                                  struct upb_msg_fielddef *f, uint8_t **outptr)
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
  //int j = s->top->elem_iter;
  void *msg = s->top->msg;
  struct upb_msgdef *m = s->top->m;

  while(buf < end) {
    struct upb_msg_fielddef *f = &m->fields[i];
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
                   struct upb_msg_fielddef *f, bool recursive)
{
  if(arr1->len != arr2->len) return false;
  if(upb_issubmsg(f)) {
    if(!recursive) return true;
    for(uint32_t i = 0; i < arr1->len; i++)
      if(!upb_msg_eql(arr1->elements.msg[i], arr2->elements.msg[i], recursive))
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

bool upb_msg_eql(struct upb_msg *msg1, struct upb_msg *msg2, bool recursive)
{
  /* Must have the same fields set.  TODO: is this wrong?  Should we also
   * consider absent defaults equal to explicitly set defaults? */
  if(msg1->def != msg2->def) return false;
  struct upb_msgdef *m = msg1->def;
  if(memcmp(msg1->data, msg2->data, msg1->def->set_flags_bytes) != 0)
    return false;

  /* Possible optimization: create a mask of the bytes in the messages that
   * contain only primitive values (not strings, arrays, submessages, or
   * padding) and memcmp the masked messages. */

  for(uint32_t i = 0; i < m->num_fields; i++) {
    struct upb_msg_fielddef *f = &m->fields[i];
    bool msg1set = upb_msg_isset(msg1, f);
    bool msg2set = upb_msg_isset(msg2, f);
    if(msg1set != msg2set) return false;
    if(!msg1set) continue;
    union upb_value_ptr p1 = upb_msg_getptr(msg1, f);
    union upb_value_ptr p2 = upb_msg_getptr(msg2, f);
    if(upb_isarray(f)) {
      if(!upb_array_eql(*p1.arr, *p2.arr, f, recursive)) return false;
    } else if(upb_issubmsg(f)) {
      if(recursive && !upb_msg_eql(*p1.msg, *p2.msg, recursive))
        return false;
    } else if(!upb_value_eql(p1, p2, f->type)) {
      return false;
    }
  }
  return true;
}


static void printval(struct upb_text_printer *printer, union upb_value_ptr p,
                     struct upb_msg_fielddef *f,
                     google_protobuf_FieldDescriptorProto *fd,
                     FILE *stream);

static void printmsg(struct upb_text_printer *printer, struct upb_msg *msg,
                     FILE *stream)
{
  struct upb_msgdef *m = msg->def;
  for(uint32_t i = 0; i < m->num_fields; i++) {
    struct upb_msg_fielddef *f = &m->fields[i];
    google_protobuf_FieldDescriptorProto *fd = upb_msg_field_descriptor(f, m);
    if(!upb_msg_isset(msg, f)) continue;
    union upb_value_ptr p = upb_msg_getptr(msg, f);
    if(upb_isarray(f)) {
      struct upb_array *arr = *p.arr;
      for(uint32_t j = 0; j < arr->len; j++) {
        union upb_value_ptr elem_p = upb_array_getelementptr(arr, j, f->type);
        printval(printer, elem_p, f, fd, stream);
      }
    } else {
      printval(printer, p, f, fd, stream);
    }
  }
}

static void printval(struct upb_text_printer *printer, union upb_value_ptr p,
                     struct upb_msg_fielddef *f,
                     google_protobuf_FieldDescriptorProto *fd,
                     FILE *stream)
{
  if(upb_issubmsg(f)) {
    upb_text_push(printer, fd->name, stream);
    printmsg(printer, *p.msg, stream);
    upb_text_pop(printer, stream);
  } else {
    upb_text_printfield(printer, fd->name, f->type, upb_deref(p, f->type), stream);
  }
}

void upb_msg_print(struct upb_msg *msg, bool single_line, FILE *stream)
{
  struct upb_text_printer printer;
  upb_text_printer_init(&printer, single_line);
  printmsg(&printer, msg, stream);
}
