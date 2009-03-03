/*
 * pbstream - a stream-oriented implementation of protocol buffers.
 *
 * Copyright (c) 2009 Joshua Haberman.  See LICENSE for details.
 */

#include <stddef.h>
#include "pbstruct.h"

#define alignof(t) offsetof(struct { char c; t x; }, x)
#define ALIGN_UP(p, t) (alignof(t) + ((p - 1) & ~(alignof(t) - 1)))

bool pbstruct_is_set(struct pbstruct *s, struct pbstruct_field *f)
{
  return s->data[f->isset_byte_offset] & f->isset_byte_mask;
}

void pbstruct_unset(struct pbstruct *s, struct pbstruct_field *f)
{
  s->data[f->isset_byte_offset] &= ~f->isset_byte_mask;
}

void pbstruct_set(struct pbstruct *s, struct pbstruct_field *f)
{
  s->data[f->isset_byte_offset] |= f->isset_byte_mask;
}

#define DEFINE_GETTERS(ctype, name) \
  ctype *pbstruct_get_ ## name(struct pbstruct *s, struct pbstruct_field *f) { \
    return (ctype*)(s->data + f->byte_offset); \
  }

DEFINE_GETTERS(double,   double)
DEFINE_GETTERS(float,    float)
DEFINE_GETTERS(int32_t,  int32)
DEFINE_GETTERS(int64_t,  int64)
DEFINE_GETTERS(uint32_t, uint32)
DEFINE_GETTERS(uint64_t, uint64)
DEFINE_GETTERS(bool,     bool)
DEFINE_GETTERS(struct pbstruct_delimited*, bytes)
DEFINE_GETTERS(struct pbstruct_delimited*, string)
DEFINE_GETTERS(struct pbstruct*, substruct)
