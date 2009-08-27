/*
 * upb - a minimalist implementation of protocol buffers.
 *
 * Copyright (c) 2009 Joshua Haberman.  See LICENSE for details.
 *
 * This file defines the in-memory format for messages, arrays, and strings
 * (which are the three dynamically-allocated structures that make up all
 * protobufs). */

#ifndef UPB_STRUCT_H
#define UPB_STRUCT_H

#include "upb.h"

/* mmhead -- this is a "base class" for strings, arrays, and messages ********/

struct upb_mm_ref;
struct upb_mmhead {
  struct upb_mm_ref *refs;  /* Head of linked list. */
  uint32_t refcount;
};

INLINE void upb_mmhead_init(struct upb_mmhead *head) {
  head->refs = NULL;
  head->refcount = 1;
}

INLINE bool upb_mmhead_norefs(struct upb_mmhead *head) {
  return head->refcount == 0 && head->refs == NULL;
}

INLINE bool upb_mmhead_only(struct upb_mmhead *head) {
  return head->refcount == 1 && head->refs == NULL;
}

INLINE bool upb_mmhead_unref(struct upb_mmhead *head) {
  head->refcount--;
  return upb_mmhead_norefs(head);
}

INLINE void upb_mmhead_ref(struct upb_mmhead *head) {
  head->refcount++;
}

/* Structures for msg, string, and array. *************************************/

/* These are all self describing. */

struct upb_msgdef;
struct upb_msg_fielddef;

struct upb_msg {
  struct upb_mmhead mmhead;
  struct upb_msgdef *def;
  uint8_t data[1];
};

typedef uint32_t upb_arraylen_t;  /* can be at most 2**32 elements long. */
struct upb_array {
  struct upb_mmhead mmhead;
  struct upb_msg_fielddef *fielddef;  /* Defines the type of the array. */
  union upb_value_ptr elements;
  upb_arraylen_t len;     /* Number of elements in "elements". */
  upb_arraylen_t size;    /* Memory we own. */
};

struct upb_string {
  struct upb_mmhead mmhead;
  /* We expect the data to be 8-bit clean (uint8_t), but char* is such an
   * ingrained convention that we follow it. */
  char *ptr;
  uint32_t byte_len;
  uint32_t byte_size;  /* How many bytes of ptr we own, 0 if we reference. */
};

/* Type-specific overlays on upb_array. ***************************************/

#define UPB_DEFINE_ARRAY_TYPE(name, type) \
  struct name ## _array { \
    struct upb_mmhead mmhead; \
    struct upb_msg_fielddef *fielddef; \
    type elements; \
    upb_arraylen_t len; \
    upb_arraylen_t size; \
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

/* Defines an array of a specific message type (an overlay of upb_array). */
#define UPB_MSG_ARRAY(msg_type) struct msg_type ## _array
#define UPB_DEFINE_MSG_ARRAY(msg_type) \
  UPB_MSG_ARRAY(msg_type) { \
    struct upb_mmhead mmhead; \
    struct upb_msg_fielddef *fielddef; \
    msg_type **elements; \
    upb_arraylen_t len; \
    upb_arraylen_t size; \
    };

/* mmptr -- a pointer which polymorphically points to one of the above. *******/

union upb_mmptr {
  struct upb_msg *msg;
  struct upb_array *arr;
  struct upb_string *str;
};

enum {
  UPB_MM_MSG_REF,
  UPB_MM_STR_REF,
  UPB_MM_ARR_REF
};
typedef uint8_t upb_mm_ptrtype;

#endif
