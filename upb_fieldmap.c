/*
 * upb - a minimalist implementation of protocol buffers.
 *
 * Copyright (c) 2009 Joshua Haberman.  See LICENSE for details.
 */

#include "upb_fieldmap.h"

#include <stdlib.h>

void pbstream_init_fieldmap(struct pbstream_fieldmap *fieldmap,
                            struct pbstream_field *fields,
                            int num_fields)
{
  qsort(fields, num_fields, sizeof(*fields), compare_fields);

  /* Find the largest n for which at least half the fieldnums <n are used.
   * Start at 8 to avoid noise of small numbers. */
  pbstream_field_number_t n = 0, maybe_n;
  for(int i = 0; i < num_fields; i++) {
    maybe_n = fields[i].field_number;
    if(maybe_n > 8 && maybe_n/(i+1) >= 2) break;
    n = maybe_n;
  }

  fieldmap->num_fields = num_fields;
  fieldmap->fields = malloc(sizeof(*fieldmap->fields)*num_fields);
  memcpy(fieldmap->fields, fields, sizeof(*fields)*num_fields);

  fieldmap->array_size = n;
  fieldmap->array = malloc(sizeof(*fieldmap->array)*n);
  memset(fieldmap->array, 0, sizeof(*fieldmap->array)*n);

  for (int i = 0; i < num_fields && fields[i].field_number <= n; i++)
    fieldmap->array[fields[i].field_number-1] = &fieldmap->fields[i];

  /* Until we support the hashtable part... */
  assert(n == fields[num_fields-1].field_number);
}

void pbstream_free_fieldmap(struct pbstream_fieldmap *fieldmap)
{
  free(fieldmap->fields);
  free(fieldmap->array);
}

/* Emit definition for inline function. */
extern void *upb_fieldmap_find(struct upb_fieldmap *fm,
                               pbstream_field_number_t num,
                               size_t info_size);
