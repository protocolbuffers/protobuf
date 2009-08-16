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
#include "upb_atomic.h"
#include "upb_context.h"
#include "upb_table.h"
#include "descriptor.h"

struct upb_enum {
  upb_atomic_refcount_t refcount;
  struct upb_context *context;
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

INLINE void upb_enum_ref(struct upb_enum *e) {
  if(upb_atomic_ref(&e->refcount)) upb_context_ref(e->context);
}

INLINE void upb_enum_unref(struct upb_enum *e) {
  if(upb_atomic_unref(&e->refcount)) upb_context_unref(e->context);
}


/* Initializes and frees an enum, respectively.  Caller retains ownership of
 * ed, but it must outlive e. */
void upb_enum_init(struct upb_enum *e,
                   struct google_protobuf_EnumDescriptorProto *ed,
                   struct upb_context *c);
void upb_enum_free(struct upb_enum *e);

#endif  /* UPB_ENUM_H_ */
