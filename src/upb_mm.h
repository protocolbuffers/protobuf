/*
 * upb - a minimalist implementation of protocol buffers.
 *
 * Copyright (c) 2009 Joshua Haberman.  See LICENSE for details.
 *
 * A parsed protobuf is represented in memory as a tree.  The three kinds of
 * nodes in this tree are messages, arrays, and strings.  This file defines
 * a memory-management scheme for making sure that these nodes are colected
 * at the right times.
 *
 * The basic strategy is reference-counting, but with a twist.  Since any
 * dynamic language that wishes to reference these nodes will need its own,
 * language-specific structure, we provide two different kinds of references:
 *
 * - counted references.  these are references that are tracked with only a
 *   reference count.  They are used for two separate purposes:
 *   1. for references within the tree, from one node to another.
 *   2. for external references into the tree, where the referer does not need
 *      a separate message structure.
 * - listed references.  these are references that have their own separate
 *   data record.  these separate records are kept in a linked list.
 */

#ifndef UPB_MM_H_
#define UPB_MM_H_

#include "upb.h"
#include "upb_string.h"
#include "upb_array.h"
#include "upb_msg.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Structure definitions. *****************************************************/

typedef int16_t upb_mm_id;

struct upb_msg;
struct upb_array;
struct upb_string;
struct upb_msg_fielddef;

struct upb_mm_ref;
/* Info about a mm. */
struct upb_mm {
  /* fromref is set iff this call is from getfieldref or getelemref. */
  struct upb_mm_ref *(*newref_cb)(struct upb_mm_ref *fromref,
                                  union upb_mmptr p, upb_mm_ptrtype type);
};

struct upb_mm_ref {
  union upb_mmptr p;
  /* This is slightly wasteful, because the mm-specific ref will probably also
   * contain the information about what kind of ref this is, in a different
   * form. */
  upb_mm_ptrtype type;
  struct upb_mm *mm;    /* TODO: There are ways to shrink this. */
  struct upb_mm_ref *next;  /* Linked list for refs to the same value. */
};

/* Functions for working with listed references.  *****************************/

/* Create a new top-level message and create a single ref for it. */
struct upb_mm_ref *upb_mm_newmsg_ref(struct upb_msgdef *def, struct upb_mm *mm);

/* Given a pointer to an existing msg, array, or string, find a ref for this
 * mm, creating one if necessary.  'created' indicates whether the returned
 * reference was just created. */
struct upb_mm_ref *upb_mm_getref(union upb_mmptr p, upb_mm_ptrtype type,
                                 struct upb_mm *mm, bool *created);

/* f must be ismm == true.  The msg field may or may not be set (will be
 * created if it doesn't exist).  If a ref already exists for the given field,
 * returns it, otherwise calls the given callback to create one.  'created'
 * indicates whether a new reference was created. */
struct upb_mm_ref *upb_mm_getfieldref(struct upb_mm_ref *msgref,
                                      struct upb_msg_fielddef *f,
                                      bool *refcreated);
/* Array len must be < i. */
struct upb_mm_ref *upb_mm_getelemref(struct upb_mm_ref *arrref, upb_arraylen_t i,
                                     bool *refcreated);

/* Remove this ref from the list for this msg.
 * If that was the last reference, deletes the msg itself. */
void upb_mm_release(struct upb_mm_ref *ref);

void upb_mm_msgset(struct upb_mm_ref *msg, struct upb_mm_ref *to,
                   struct upb_msg_fielddef *f);
void upb_mm_msgclear(struct upb_mm_ref *from, struct upb_msg_fielddef *f);
void upb_mm_msgclear_all(struct upb_mm_ref *from);

void upb_mm_arrset(struct upb_mm_ref *from, struct upb_mm_ref *to, uint32_t i);

/* Defined iff upb_field_ismm(f). */
INLINE upb_mm_ptrtype upb_field_ptrtype(struct upb_msg_fielddef *f);
/* Defined iff upb_elem_ismm(f). */
INLINE upb_mm_ptrtype upb_elem_ptrtype(struct upb_msg_fielddef *f);

INLINE void upb_mm_unref(union upb_mmptr p, upb_mm_ptrtype type);

/* These methods are all a bit silly, since all branches of the case compile
 * to the same thing (which the compiler will recognize), but we do it this way
 * for full union correctness. */
INLINE union upb_mmptr upb_mmptr_read(union upb_value_ptr p, upb_mm_ptrtype t)
{
  union upb_mmptr val;
  switch(t) {
    case UPB_MM_MSG_REF: val.msg = *p.msg; break;
    case UPB_MM_STR_REF: val.str = *p.str; break;
    case UPB_MM_ARR_REF: val.arr = *p.arr; break;
    default: assert(false); val.msg = *p.msg; break;  /* Shouldn't happen. */
  }
  return val;
}

INLINE void upb_mmptr_write(union upb_value_ptr p, union upb_mmptr val,
                            upb_mm_ptrtype t)
{
  switch(t) {
    case UPB_MM_MSG_REF: *p.msg = val.msg; break;
    case UPB_MM_STR_REF: *p.str = val.str; break;
    case UPB_MM_ARR_REF: *p.arr = val.arr; break;
    default: assert(false); val.msg = *p.msg; break;  /* Shouldn't happen. */
  }
}

void upb_array_destroy(struct upb_array *arr);
void upb_msg_destroy(struct upb_msg *msg);

INLINE void upb_msg_unref(struct upb_msg *msg) {
  if(upb_mmhead_unref(&msg->mmhead)) upb_msg_destroy(msg);
}

INLINE void upb_array_unref(struct upb_array *arr) {
  if(upb_mmhead_unref(&arr->mmhead)) upb_array_destroy(arr);
}

INLINE void upb_mm_unref(union upb_mmptr p, upb_mm_ptrtype type)
{
  switch(type) {
    case UPB_MM_MSG_REF: upb_msg_unref(p.msg); break;
    case UPB_MM_STR_REF: upb_string_unref(p.str); break;
    case UPB_MM_ARR_REF: upb_array_unref(p.arr);
  }
}

static struct upb_mmhead *upb_mmhead_addr(union upb_mmptr p, upb_mm_ptrtype t)
{
  switch(t) {
    case UPB_MM_MSG_REF: return &((*p.msg).mmhead);
    case UPB_MM_STR_REF: return &((*p.str).mmhead);
    case UPB_MM_ARR_REF: return &((*p.arr).mmhead);
    default: assert(false); return &((*p.msg).mmhead);  /* Shouldn't happen. */
  }
}

INLINE void upb_mm_ref(union upb_mmptr p, upb_mm_ptrtype type)
{
  upb_mmhead_ref(upb_mmhead_addr(p, type));
}

#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif  /* UPB_MM_MSG_H_ */
