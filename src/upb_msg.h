/*
 * upb - a minimalist implementation of protocol buffers.
 *
 * Copyright (c) 2010-2011 Google Inc.  See LICENSE for details.
 * Author: Josh Haberman <jhaberman@gmail.com>
 *
 * Routines for reading and writing message data to an in-memory structure,
 * similar to a C struct.
 *
 * upb does not define one single message object that everyone must use.
 * Rather it defines an abstract interface for reading and writing members
 * of a message object, and all of the parsers and serializers use this
 * abstract interface.  This allows upb's parsers and serializers to be used
 * regardless of what memory management scheme or synchronization model the
 * application is using.
 *
 * A standard set of accessors is provided for doing simple reads and writes at
 * a known offset into the message.  These accessors should be used when
 * possible, because they are specially optimized -- for example, the JIT can
 * recognize them and emit specialized code instead of having to call the
 * function at all.  The application can substitute its own accessors when the
 * standard accessors are not suitable.
 */

#ifndef UPB_MSG_H
#define UPB_MSG_H

#include <stdlib.h>
#include "upb_def.h"
#include "upb_handlers.h"

#ifdef __cplusplus
extern "C" {
#endif


/* upb_accessor ***************************************************************/

// A upb_accessor is a table of function pointers for doing reads and writes
// for one specific upb_fielddef.  Each field has a separate accessor, which
// lives in the fielddef.

typedef bool upb_has_reader(void *m, upb_value fval);
typedef upb_value upb_value_reader(void *m, upb_value fval);

typedef void *upb_seqbegin_handler(void *s);
typedef void *upb_seqnext_handler(void *s, void *iter);
typedef upb_value upb_seqget_handler(void *iter);
INLINE bool upb_seq_done(void *iter) { return iter == NULL; }

typedef struct _upb_accessor_vtbl {
  // Writers.  These take an fval as a parameter because the callbacks are used
  // as upb_handlers, but the fval is always the fielddef for that field.
  upb_startfield_handler *appendseq;     // Repeated fields only.
  upb_startfield_handler *appendsubmsg;  // Submsg fields (repeated or no).
  upb_value_handler      *set;           // Scalar fields (repeated or no).

  // Readers.
  upb_has_reader         *has;
  upb_value_reader       *get;
  upb_seqbegin_handler   *seqbegin;
  upb_seqnext_handler    *seqnext;
  upb_seqget_handler     *seqget;
} upb_accessor_vtbl;

// Registers handlers for writing into a message of the given type.
upb_mhandlers *upb_accessors_reghandlers(upb_handlers *h, upb_msgdef *m);

// Returns an stdmsg accessor for the given fielddef.
upb_accessor_vtbl *upb_stdmsg_accessor(upb_fielddef *f);


/* upb_msg/upb_seq ************************************************************/

// upb_msg and upb_seq allow for generic access to a message through its
// accessor vtable.  Note that these do *not* allow you to create, destroy, or
// take references on the objects -- these operations are specifically outside
// the scope of what the accessors define.

// Clears all hasbits.
// TODO: Add a separate function for setting primitive values back to their
// defaults (but not strings, submessages, or arrays).
void upb_msg_clear(void *msg, upb_msgdef *md);

// Could add a method that recursively clears submessages, strings, and
// arrays if desired.  This could be a win if you wanted to merge without
// needing hasbits, because during parsing you would never clear submessages
// or arrays.  Also this could be desired to provide proto2 operations on
// generated messages.

INLINE bool upb_msg_has(void *m, upb_fielddef *f) {
  return f->accessor && f->accessor->has(m, f->fval);
}

// May only be called for fields that are known to be set.
INLINE upb_value upb_msg_get(void *m, upb_fielddef *f) {
  assert(upb_msg_has(m, f));
  return f->accessor->get(m, f->fval);
}

INLINE void *upb_seq_begin(void *s, upb_fielddef *f) {
  assert(f->accessor);
  return f->accessor->seqbegin(s);
}
INLINE void *upb_seq_next(void *s, void *iter, upb_fielddef *f) {
  assert(f->accessor);
  assert(!upb_seq_done(iter));
  return f->accessor->seqnext(s, iter);
}
INLINE upb_value upb_seq_get(void *iter, upb_fielddef *f) {
  assert(f->accessor);
  assert(!upb_seq_done(iter));
  return f->accessor->seqget(iter);
}


/* upb_msgvisitor *************************************************************/

// A upb_msgvisitor reads data from an in-memory structure using its accessors,
// pushing the results to a given set of upb_handlers.
// TODO: not yet implemented.

typedef struct {
  upb_fhandlers *fh;
  upb_fielddef *f;
  uint16_t msgindex;  // Only when upb_issubmsg(f).
} upb_msgvisitor_field;

typedef struct {
  upb_msgvisitor_field *fields;
  int fields_len;
} upb_msgvisitor_msg;

typedef struct {
  uint16_t msgindex;
  uint16_t fieldindex;
  uint32_t arrayindex;  // UINT32_MAX if not an array frame.
} upb_msgvisitor_frame;

typedef struct {
  upb_msgvisitor_msg *messages;
  int messages_len;
  upb_dispatcher dispatcher;
} upb_msgvisitor;

// Initializes a msgvisitor that will push data from messages of the given
// msgdef to the given set of handlers.
void upb_msgvisitor_init(upb_msgvisitor *v, upb_msgdef *md, upb_handlers *h);
void upb_msgvisitor_uninit(upb_msgvisitor *v);

void upb_msgvisitor_reset(upb_msgvisitor *v, upb_msg *m);
void upb_msgvisitor_visit(upb_msgvisitor *v, upb_status *status);


/* Standard writers. **********************************************************/

// Allocates a new stdmsg.
void *upb_stdmsg_new(upb_msgdef *md);

// Recursively frees any strings or submessages that the message refers to.
void upb_stdmsg_free(void *m, upb_msgdef *md);

// "hasbit" must be <= UPB_MAX_FIELDS.  If it is <0, this field has no hasbit.
upb_value upb_stdmsg_packfval(int16_t hasbit, uint16_t value_offset);
upb_value upb_stdmsg_packfval_subm(int16_t hasbit, uint16_t value_offset,
                                   uint16_t subm_size, uint8_t subm_setbytes);

// Value writers for every in-memory type: write the data to a known offset
// from the closure "c" and set the hasbit (if any).
// TODO: can we get away with having only one for int64, uint64, double, etc?
// The main thing in the way atm is that the upb_value is strongly typed.
// in debug mode.
upb_flow_t upb_stdmsg_setint64(void *c, upb_value fval, upb_value val);
upb_flow_t upb_stdmsg_setint32(void *c, upb_value fval, upb_value val);
upb_flow_t upb_stdmsg_setuint64(void *c, upb_value fval, upb_value val);
upb_flow_t upb_stdmsg_setuint32(void *c, upb_value fval, upb_value val);
upb_flow_t upb_stdmsg_setdouble(void *c, upb_value fval, upb_value val);
upb_flow_t upb_stdmsg_setfloat(void *c, upb_value fval, upb_value val);
upb_flow_t upb_stdmsg_setbool(void *c, upb_value fval, upb_value val);

// Value writers for repeated fields: the closure points to a standard array
// struct, appends the value to the end of the array, resizing with realloc()
// if necessary.
typedef struct {
  char *ptr;
  int32_t len;   // Number of elements present.
  int32_t size;  // Number of elements allocated.
} upb_stdarray;

upb_flow_t upb_stdmsg_setint64_r(void *c, upb_value fval, upb_value val);
upb_flow_t upb_stdmsg_setint32_r(void *c, upb_value fval, upb_value val);
upb_flow_t upb_stdmsg_setuint64_r(void *c, upb_value fval, upb_value val);
upb_flow_t upb_stdmsg_setuint32_r(void *c, upb_value fval, upb_value val);
upb_flow_t upb_stdmsg_setdouble_r(void *c, upb_value fval, upb_value val);
upb_flow_t upb_stdmsg_setfloat_r(void *c, upb_value fval, upb_value val);
upb_flow_t upb_stdmsg_setbool_r(void *c, upb_value fval, upb_value val);

// Writers for C strings (NULL-terminated): we can find a char* at a known
// offset from the closure "c".  Calls realloc() on the pointer to allocate
// the memory (TODO: investigate whether checking malloc_usable_size() would
// be cheaper than realloc()).  Also sets the hasbit, if any.
//
// Since the string is NULL terminated and does not store an explicit length,
// these are not suitable for binary data that can contain NULLs.
upb_flow_t upb_stdmsg_setcstr(void *c, upb_value fval, upb_value val);
upb_flow_t upb_stdmsg_setcstr_r(void *c, upb_value fval, upb_value val);

// Writers for length-delimited strings: we explicitly store the length, so
// the data can contain NULLs.  Stores the data using upb_stdarray
// which is located at a known offset from the closure "c" (note that it
// is included inline rather than pointed to).  Also sets the hasbit, if any.
upb_flow_t upb_stdmsg_setstr(void *c, upb_value fval, upb_value val);
upb_flow_t upb_stdmsg_setstr_r(void *c, upb_value fval, upb_value val);

// Writers for startseq and startmsg which allocate (or reuse, if possible)
// a sub data structure (upb_stdarray or a submessage, respectively),
// setting the hasbit.  If the hasbit is already set, the existing data
// structure is used verbatim.  If the hasbit is not already set, the pointer
// is checked for NULL.  If it is NULL, a new substructure is allocated,
// cleared, and used.  If it is not NULL, the existing substructure is
// cleared and reused.
//
// If there is no hasbit, we always behave as if the hasbit was not set,
// so any existing data for this array or submessage is cleared.  In most
// cases this will be fine since each array or non-repeated submessage should
// occur at most once in the stream.  But if the client is using "concatenation
// as merging", it will want to make sure hasbits are allocated so merges can
// happen appropriately.
//
// If there was a demand for the behavior that absence of a hasbit acts as if
// the bit was always set, we could provide that also.  But Clear() would need
// to act recursively, which is less efficient since it requires an extra pass
// over the tree.
upb_sflow_t upb_stdmsg_startseq(void *c, upb_value fval);
upb_sflow_t upb_stdmsg_startsubmsg(void *c, upb_value fval);
upb_sflow_t upb_stdmsg_startsubmsg_r(void *c, upb_value fval);


/* Standard readers. **********************************************************/

bool upb_stdmsg_has(void *c, upb_value fval);
void *upb_stdmsg_seqbegin(void *c);

upb_value upb_stdmsg_getint64(void *c, upb_value fval);
upb_value upb_stdmsg_getint32(void *c, upb_value fval);
upb_value upb_stdmsg_getuint64(void *c, upb_value fval);
upb_value upb_stdmsg_getuint32(void *c, upb_value fval);
upb_value upb_stdmsg_getdouble(void *c, upb_value fval);
upb_value upb_stdmsg_getfloat(void *c, upb_value fval);
upb_value upb_stdmsg_getbool(void *c, upb_value fval);
upb_value upb_stdmsg_getptr(void *c, upb_value fval);

void *upb_stdmsg_8byte_seqnext(void *c, void *iter);
void *upb_stdmsg_4byte_seqnext(void *c, void *iter);
void *upb_stdmsg_1byte_seqnext(void *c, void *iter);

upb_value upb_stdmsg_seqgetint64(void *c);
upb_value upb_stdmsg_seqgetint32(void *c);
upb_value upb_stdmsg_seqgetuint64(void *c);
upb_value upb_stdmsg_seqgetuint32(void *c);
upb_value upb_stdmsg_seqgetdouble(void *c);
upb_value upb_stdmsg_seqgetfloat(void *c);
upb_value upb_stdmsg_seqgetbool(void *c);
upb_value upb_stdmsg_seqgetptr(void *c);

#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif
