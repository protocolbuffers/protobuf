/*
 * upb - a minimalist implementation of protocol buffers.
 *
 * Copyright (c) 2009 Joshua Haberman.  See LICENSE for details.
 *
 * A fieldmap is a data structure that supports fast lookup of fields by
 * number.  It is logically a map of {field_number -> <field info>}, where
 * <field info> is any struct that begins with the field number.  Fast lookup
 * is important, because it is in the critical path of parsing. */

#ifndef UPB_FIELDMAP_H_
#define UPB_FIELDMAP_H_

#include "upb.h"

#ifdef __cplusplus
extern "C" {
#endif

struct upb_fieldmap {
  int array_size;
  void *array;
  /* TODO: the hashtable part. */
};

/* Takes an array of num_fields fields and builds an optimized table for fast
 * lookup of fields by number.  The input fields need not be sorted.  This
 * fieldmap must be freed with upb_free_fieldmap(). */
void upb_init_fieldmap(struct upb_fieldmap *fieldmap,
                       void *fields,
                       int num_fields,
                       int field_size);
void upb_free_fieldmap(struct upb_fieldmap *fieldmap);

/* Looks the given field number up in the fieldmap, and returns the
 * corresponding field definition (or NULL if this field number does not exist
 * in this fieldmap). */
inline void *upb_fieldmap_find(struct upb_fieldmap *fm,
                               upb_field_number_t num,
                               size_t info_size)
{
  if (num < array_size) {
    return (char*)fs->array + (num*info_size);
  } else {
    /* TODO: the hashtable part. */
  }
}

#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif  /* UPB_PARSE_H_ */
