/*
 * upb - a minimalist implementation of protocol buffers.
 *
 * Copyright (c) 2009 Joshua Haberman.  See LICENSE for details.
 *
 * upb_enum is a simple object that allows run-time reflection over the values
 * defined within an enum. */

#ifndef UPB_ENUM_H_
#define UPB_ENUM_H_

#include <stdint.h>
#include "upb_table.h"

/* Forward declaration from descriptor.h. */
struct google_protobuf_EnumDescriptorProto;
struct google_protobuf_EnumValueDescriptorProto;

struct upb_enum {
  struct google_protobuf_EnumDescriptorProto *descriptor;
  struct upb_strtable nametoint;
  struct upb_inttable inttoname;
};

struct upb_enum_ntoi_entry {
  struct upb_strtable_entry e;
  uint32_t value;
};

struct upb_enum_iton_entry {
  struct upb_inttable_entry e;
  struct upb_string *string;
};

/* Initializes and frees an enum, respectively.  Caller retains ownership of
 * ed, but it must outlive e. */
INLINE void upb_enum_init(struct upb_enum *e,
                          struct google_protobuf_EnumDescriptorProto *ed) {
  int num_values = ed->set_flags.has.value ? ed->value->len : 0;
  e->descriptor = ed;
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

INLINE void upb_enum_free(struct upb_enum *e) {
  upb_strtable_free(&e->nametoint);
  upb_inttable_free(&e->inttoname);
}

#endif  /* UPB_ENUM_H_ */
