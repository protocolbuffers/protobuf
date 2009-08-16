/*
 * upb - a minimalist implementation of protocol buffers.
 *
 * Copyright (c) 2009 Joshua Haberman.  See LICENSE for details.
 */

#include "descriptor.h"
#include "upb_enum.h"

void upb_enum_init(struct upb_enum *e,
                   struct google_protobuf_EnumDescriptorProto *ed,
                   struct upb_context *c) {
  int num_values = ed->set_flags.has.value ? ed->value->len : 0;
  e->descriptor = ed;
  e->context = c;
  upb_atomic_refcount_init(&e->refcount, 0);
  upb_strtable_init(&e->nametoint, num_values, sizeof(struct upb_enum_ntoi_entry));
  upb_inttable_init(&e->inttoname, num_values, sizeof(struct upb_enum_iton_entry));

  for(int i = 0; i < num_values; i++) {
    google_protobuf_EnumValueDescriptorProto *value = ed->value->elements[i];
    struct upb_enum_ntoi_entry ntoi_entry = {.e = {.key = *value->name},
                                             .value = value->number};
    struct upb_enum_iton_entry iton_entry = {.e = {.key = value->number},
                                             .string = value->name};
    upb_strtable_insert(&e->nametoint, &ntoi_entry.e);
    upb_inttable_insert(&e->inttoname, &iton_entry.e);
  }
}

void upb_enum_free(struct upb_enum *e) {
  upb_strtable_free(&e->nametoint);
  upb_inttable_free(&e->inttoname);
}
