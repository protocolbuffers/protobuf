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
#include "descriptor.h"

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
void upb_enum_init(struct upb_enum *e,
                   struct google_protobuf_EnumDescriptorProto *ed);
void upb_enum_free(struct upb_enum *e);

#endif  /* UPB_ENUM_H_ */
