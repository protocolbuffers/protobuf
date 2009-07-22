/*
 * upb - a minimalist implementation of protocol buffers.
 *
 * Copyright (c) 2009 Joshua Haberman.  See LICENSE for details.
 */

#include <inttypes.h>
#include "upb_text.h"
#include "descriptor.h"

void upb_text_printval(upb_field_type_t type, union upb_value_ptr p, FILE *file)
{
#define CASE(fmtstr, member) fprintf(file, fmtstr, *p.member); break;
  switch(type) {
    case GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_TYPE_DOUBLE:
      CASE("%0.f", _double);
    case GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_TYPE_FLOAT:
      CASE("%0.f", _float)
    case GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_TYPE_INT64:
    case GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_TYPE_SFIXED64:
    case GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_TYPE_SINT64:
      CASE("%" PRId64, int64)
    case GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_TYPE_UINT64:
    case GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_TYPE_FIXED64:
      CASE("%" PRIu64, uint64)
    case GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_TYPE_INT32:
    case GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_TYPE_SFIXED32:
    case GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_TYPE_SINT32:
      CASE("%" PRId32, int32)
    case GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_TYPE_UINT32:
    case GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_TYPE_FIXED32:
    case GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_TYPE_ENUM:
      CASE("%" PRIu32, uint32);
    case GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_TYPE_BOOL:
      CASE("%hhu", _bool);
    case GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_TYPE_STRING:
    case GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_TYPE_BYTES:
      /* TODO: escaping. */
      fprintf(file, "\"" UPB_STRFMT "\"", UPB_STRARG(**p.str)); break;
  }
}

static void print_indent(struct upb_text_printer *p, FILE *stream)
{
  if(!p->single_line)
    for(int i = 0; i < p->indent_depth; i++)
      fprintf(stream, "  ");
}

void upb_text_printfield(struct upb_text_printer *p,
                         struct upb_string name,
                         upb_field_type_t valtype, union upb_value_ptr val,
                         FILE *stream)
{
  print_indent(p, stream);
  fprintf(stream, UPB_STRFMT ":", UPB_STRARG(name));
  upb_text_printval(valtype, val, stream);
  if(p->single_line)
    fputc(' ', stream);
  else
    fputc('\n', stream);
}

void upb_text_push(struct upb_text_printer *p,
                   struct upb_string submsg_type,
                   FILE *stream)
{
  print_indent(p, stream);
  fprintf(stream, UPB_STRFMT " {", UPB_STRARG(submsg_type));
  if(!p->single_line) fputc('\n', stream);
  p->indent_depth++;
}

void upb_text_pop(struct upb_text_printer *p,
                  FILE *stream)
{
  p->indent_depth--;
  print_indent(p, stream);
  fprintf(stream, "}\n");
}

#if 0
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

    void *data1, struct upb_msg *m, bool single_line)
  /* Must have the same fields set.  TODO: is this wrong?  Should we also
   * consider absent defaults equal to explicitly set defaults? */
  if(memcmp(data1, data2, m->set_flags_bytes) != 0)
    return false;

  /* Possible optimization: create a mask of the bytes in the messages that
   * contain only primitive values (not strings, arrays, submessages, or
   * padding) and memcmp the masked messages. */

  for(uint32_t i = 0; i < m->num_fields; i++) {
    struct upb_msg_field *f = &m->fields[i];
    if(!upb_msg_is_set(data1, f)) continue;
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
#endif
