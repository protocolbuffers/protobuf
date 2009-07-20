/*
 * upb - a minimalist implementation of protocol buffers.
 *
 * Copyright (c) 2009 Joshua Haberman.  See LICENSE for details.

 * Defines an in-memory array type.  TODO: more documentation.
 *
 */

#ifndef UPB_ARRAY_H_
#define UPB_ARRAY_H_

#ifdef __cplusplus
extern "C" {
#endif

struct upb_string;

#include "upb.h"

/* Represents an array (a repeated field) of any type.  The interpretation of
 * the data in the array depends on the type. */
struct upb_array {
  union upb_value_ptr elements;
  uint32_t len;     /* Measured in elements. */
};

/* Returns a pointer to an array element. */
INLINE union upb_value_ptr upb_array_getelementptr(
    struct upb_array *arr, uint32_t n, upb_field_type_t type)
{
  union upb_value_ptr ptr;
  ptr._void = (void*)((char*)arr->elements._void + n*upb_type_info[type].size);
  return ptr;
}

/* These are all overlays on upb_array, pointers between them can be cast. */
#define UPB_DEFINE_ARRAY_TYPE(name, type) \
  struct name ## _array { \
    type *elements; \
    uint32_t len; \
  };

UPB_DEFINE_ARRAY_TYPE(upb_double, double)
UPB_DEFINE_ARRAY_TYPE(upb_float,  float)
UPB_DEFINE_ARRAY_TYPE(upb_int32,  int32_t)
UPB_DEFINE_ARRAY_TYPE(upb_int64,  int64_t)
UPB_DEFINE_ARRAY_TYPE(upb_uint32, uint32_t)
UPB_DEFINE_ARRAY_TYPE(upb_uint64, uint64_t)
UPB_DEFINE_ARRAY_TYPE(upb_bool,   bool)
UPB_DEFINE_ARRAY_TYPE(upb_string, struct upb_string*)
UPB_DEFINE_ARRAY_TYPE(upb_msg,    void*)

/* Defines an array of a specific message type. */
#define UPB_MSG_ARRAY(msg_type) struct msg_type ## _array
#define UPB_DEFINE_MSG_ARRAY(msg_type) \
  UPB_MSG_ARRAY(msg_type) { \
    msg_type **elements; \
    uint32_t len; \
  };

#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif
