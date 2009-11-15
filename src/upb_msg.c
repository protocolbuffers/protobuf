/*
 * upb - a minimalist implementation of protocol buffers.
 *
 * Copyright (c) 2009 Joshua Haberman.  See LICENSE for details.
 */

#include <inttypes.h>
#include <stdlib.h>
#include "upb_msg.h"
#include "descriptor.h"
#include "upb_mm.h"
#include "upb_parse.h"
#include "upb_serialize.h"
#include "upb_text.h"

/* Parsing.  ******************************************************************/

struct upb_msgparser_frame {
  struct upb_msg *msg;
};

struct upb_msgparser {
  struct upb_cbparser *s;
  bool merge;
  bool byref;
  struct upb_msgparser_frame stack[UPB_MAX_NESTING], *top;
};

/* Helper function that returns a pointer to where the next value for field "f"
 * should be stored, taking into account whether f is an array that may need to
 * be allocated or resized. */
static union upb_value_ptr get_value_ptr(struct upb_msg *msg,
                                         struct upb_fielddef *f)
{
  union upb_value_ptr p = upb_msg_getptr(msg, f);
  if(upb_isarray(f)) {
    if(!upb_msg_isset(msg, f)) {
      if(!*p.arr || !upb_mmhead_only(&((*p.arr)->mmhead))) {
        if(*p.arr)
          upb_array_unref(*p.arr);
        *p.arr = upb_array_new(f);
      }
      upb_array_truncate(*p.arr);
      upb_msg_set(msg, f);
    }
    p = upb_array_append(*p.arr);
  }
  return p;
}

/* Callbacks for the stream parser. */

static bool value_cb(void *udata, struct upb_msgdef *msgdef,
                     struct upb_fielddef *f, union upb_value val)
{
  (void)msgdef;
  struct upb_msgparser *mp = udata;
  struct upb_msg *msg = mp->top->msg;
  union upb_value_ptr p = get_value_ptr(msg, f);
  upb_msg_set(msg, f);
  upb_value_write(p, val, f->type);
  return true;
}

static bool str_cb(void *udata, struct upb_msgdef *msgdef,
                   struct upb_fielddef *f, uint8_t *str, size_t avail_len,
                   size_t total_len)
{
  (void)msgdef;
  struct upb_msgparser *mp = udata;
  struct upb_msg *msg = mp->top->msg;
  union upb_value_ptr p = get_value_ptr(msg, f);
  upb_msg_set(msg, f);
  if(avail_len != total_len) abort();  /* TODO: support streaming. */
  //bool byref = avail_len == total_len && mp->byref;
  if(!*p.str || !upb_mmhead_only(&((*p.str)->mmhead))) {
    if(*p.str)
      upb_string_unref(*p.str);
    *p.str = upb_string_new();
  }
  //if(byref) {
  //  upb_strdrop(*p.str);
  //  (*p.str)->ptr = (char*)str;
  //  (*p.str)->byte_len = avail_len;
  //} else {
    upb_string_resize(*p.str, total_len);
    memcpy((*p.str)->ptr, str, avail_len);
    (*p.str)->byte_len = avail_len;
  //}
  return true;
}

static void start_cb(void *udata, struct upb_fielddef *f)
{
  struct upb_msgparser *mp = udata;
  struct upb_msg *oldmsg = mp->top->msg;
  union upb_value_ptr p = get_value_ptr(oldmsg, f);

  if(upb_isarray(f) || !upb_msg_isset(oldmsg, f)) {
    if(!*p.msg || !upb_mmhead_only(&((*p.msg)->mmhead))) {
      if(*p.msg)
        upb_msg_unref(*p.msg);
      *p.msg = upb_msg_new(f->ref.msg);
    }
    upb_msg_clear(*p.msg);
    upb_msg_set(oldmsg, f);
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

void upb_msgparser_reset(struct upb_msgparser *s, struct upb_msg *msg, bool byref)
{
  upb_cbparser_reset(s->s, s);
  s->byref = byref;
  s->top = s->stack;
  s->top->msg = msg;
}

void upb_msgparser_free(struct upb_msgparser *s)
{
  upb_cbparser_free(s->s);
  free(s);
}

void upb_msg_parsestr(struct upb_msg *msg, void *buf, size_t len,
                      struct upb_status *status)
{
  struct upb_msgparser *mp = upb_msgparser_new(msg->def);
  upb_msgparser_reset(mp, msg, false);
  upb_msg_clear(msg);
  upb_msgparser_parse(mp, buf, len, status);
  upb_msgparser_free(mp);
}

size_t upb_msgparser_parse(struct upb_msgparser *s, void *data, size_t len,
                           struct upb_status *status)
{
  return upb_cbparser_parse(s->s, data, len, status);
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
static size_t get_msgsize(struct upb_msgsizes *sizes, struct upb_msg *m);

/* Returns a size of a value as it will be serialized.  Does *not* include
 * the size of the tag -- that is already accounted for. */
static size_t get_valuesize(struct upb_msgsizes *sizes, union upb_value_ptr p,
                            struct upb_fielddef *f,
                            google_protobuf_FieldDescriptorProto *fd)
{
  switch(f->type) {
    default: assert(false); return 0;  /* Internal corruption. */
    case GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_TYPE_MESSAGE: {
      size_t submsg_size = get_msgsize(sizes, *p.msg);
      return upb_get_INT32_size(submsg_size) + submsg_size;
    }
    case GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_TYPE_GROUP: {
      size_t endgrp_tag_size = upb_get_tag_size(fd->number);
      return endgrp_tag_size + get_msgsize(sizes, *p.msg);
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
static size_t get_msgsize(struct upb_msgsizes *sizes, struct upb_msg *m)
{
  size_t size = 0;
  /* We iterate over fields and arrays in reverse order. */
  for(int32_t i = m->def->num_fields - 1; i >= 0; i--) {
    struct upb_fielddef *f = &m->def->fields[i];
    google_protobuf_FieldDescriptorProto *fd = upb_msg_field_descriptor(f, m->def);
    if(!upb_msg_isset(m, f)) continue;
    union upb_value_ptr p = upb_msg_getptr(m, f);
    if(upb_isarray(f)) {
      for(int32_t j = (*p.arr)->len - 1; j >= 0; j--) {
        union upb_value_ptr elem = upb_array_getelementptr(*p.arr, j);
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

void upb_msgsizes_read(struct upb_msgsizes *sizes, struct upb_msg *m)
{
  get_msgsize(sizes, m);
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

void upb_msg_serialize_init(struct upb_msg_serialize_state *s, struct upb_msg *m,
                            struct upb_msgsizes *sizes)
{
  (void)s;
  (void)m;
  (void)sizes;
}

#if 0
static uint8_t *serialize_tag(uint8_t *buf, uint8_t *end,
                              struct upb_fielddef *f,
                              struct upb_status *status)
{
  /* TODO: need to have the field number also. */
  return upb_put_UINT32(buf, end, f->type, status);
}

/* Serializes the next set of bytes into buf (which has size len).  Returns
 * UPB_STATUS_OK if serialization is complete, or UPB_STATUS_NEED_MORE_DATA
 * if there is more data from the message left to be serialized.
 *
 * The number of bytes written to buf is returned in *read.  This will be
 * equal to len unless we finished serializing. */
size_t upb_msg_serialize(struct upb_msg_serialize_state *s,
                         void *_buf, size_t len, struct upb_status *status)
{
  uint8_t *buf = _buf;
  uint8_t *end = buf + len;
  uint8_t *const start = buf;
  int i = s->top->field_iter;
  //int j = s->top->elem_iter;
  void *msg = s->top->msg;
  struct upb_msgdef *m = s->top->m;

  while(buf < end) {
    struct upb_fielddef *f = &m->fields[i];
    //union upb_value_ptr p = upb_msg_getptr(msg, f);
    buf = serialize_tag(buf, end, f, status);
    if(f->type == GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_TYPE_MESSAGE) {
    } else if(f->type == GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_TYPE_GROUP) {
    } else if(upb_isstring(f)) {
    } else {
      //upb_serialize_value(buf, end, f->type, p, status);
    }
  }
  return buf - start;
}
#endif


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
                   struct upb_fielddef *f, bool recursive)
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
    struct upb_fielddef *f = &m->fields[i];
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
