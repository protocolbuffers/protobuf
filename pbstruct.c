/*
 * pbstream - a stream-oriented implementation of protocol buffers.
 *
 * Copyright (c) 2009 Joshua Haberman.  See LICENSE for details.
 */

#include <stddef.h>
#include "pbstruct.h"

#define alignof(t) offsetof(struct { char c; t x; }, x)
#define ALIGN_UP(p, t) (alignof(t) + ((p - 1) & ~(alignof(t) - 1)))

void pbstruct_init(struct pbstruct *s, struct pbstruct_definition *d)
{
  /* "set" flags start as unset, dynamic pointers start as NULL. */
  memset(s, 0, d->struct_size);
  s->definition = d;
}

pbstruct *pbstruct_new(struct pbstruct_defintion *d, struct pballoc *a)
{
  pbstruct *s = a->realloc(NULL, d->struct_size, d->user_data);
  pbstruct_init(s, d);
}

void pbstruct_free(struct pbstruct *s, struct pballoc *a, bool free_substructs)
{
  pbstruct_free_all_fields(s, free_substructs);
  a->free(s, s->user_data);
}

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

void pbstruct_unset_all(struct pbstruct *s)
{
  for(int i = 0; i < s->definition->num_fields; i++)
    pbstruct_unset(s, &s->definition->fields[i]);
}

void pbstruct_resize_str(struct pbstruct *s, struct pbstruct_field *f,
                         size_t len, struct pballoc *a)
{
  assert(f->type & PBSTRUCT_DELIMITED);
  size_t size = next_pow2(len);
  struct pbstruct_dynamic **dyn = (struct pbstruct_dynamic**)get_addr(s, f);
  if(!*dyn || (*dyn)->size < size) {
    size_t old_len = *dyn ? (*dyn)->len : 0;
    *dyn = a->realloc(*dyn, size*type_size, s->definition->user_data);
    (*dyn)->size = size;
    (*dyn)->len = len;
  }
}

size_t pbstruct_append_array(struct pbstruct *s, struct pbstruct_field *f,
                             struct pballoc *a)
{
  assert(f->type & PBSTRUCT_ARRAY);
  struct pbstruct_dynamic **dyn = (struct pbstruct_dynamic**)get_addr(s, f);
  size_t new_len, new_size;
  if(!*dyn) {
    new_len = 1;
    new_size = 4;  /* Initial size. */
  } else {
    new_len = (*dyn)->len + 1;
    new_size = next_pow2(new_len);
  }
  *dyn = a->realloc(*dyn, ARRSIZE(f->type, new_size), s->defintion->user_data);
  memset(ARRADDR(*dyn, f->type, new_len-1), 0, SIZE(f->type));
}

void pbstruct_clear_array(struct pbstruct *s, struct pbstruct_field *f,
                          struct pballoc *a, bool free_substructs)
{
  assert(f->type & PBSTRUCT_ARRAY);
  struct pbstruct_dynamic *dyn = *(struct pbstruct_dynamic**)get_addr(s, f);
  if(dyn) {
    if((f->type & PBSTRUCT_DELIMITED) || free_substructs) {
      for(int i = 0; i < dyn->len; i++)
        FREE(ARRADDR(dyn, f->type, i), a, s->definition->user_data);
    }
    dyn->size = 0;
  }
}

/* Resizes a delimited field (string or bytes) string that is within an array.
 * The array must already be large enough that str_offset is a valid member. */
void pbstruct_resize_arraystr(struct pbstruct *s, struct pbstruct_field *f,
                              size_t str_offset, size_t len, struct pballoc *a)
{

}

/* Like pbstruct_append_array, but appends a string of the specified length. */
size_t pbstruct_append_arraystr(struct pbstruct *s, struct pbstruct_field *f,
                                size_t len, struct pballoc *a)
{
}

void pbstruct_free_field(struct pbstruct *s, struct pbstruct_field *f,
                         bool free_substructs)
{
  pbstruct_clear_array(s, f, free_substructs);
  struct pbstruct_dynamic **dyn = (struct pbstruct_dynamic**)get_addr(s, f);
  if(*dyn) {
    FREE(*dyn, s);
    *dyn = NULL;
  }
}

void pbstruct_free_all_fields(struct pbstruct *s, bool free_substructs)
{
  for(int i = 0; i < s->definition->num_fields; i++)
    pbstruct_free_field(s, &s->definition->fields[i]);
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
