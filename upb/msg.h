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
#include "upb/def.h"
#include "upb/handlers.h"

#ifdef __cplusplus
extern "C" {
#endif


/* upb_accessor ***************************************************************/

// A upb_accessor is a table of function pointers for doing reads and writes
// for one specific upb_fielddef.  Each field has a separate accessor, which
// lives in the fielddef.

typedef bool upb_has_reader(const void *m, upb_value fval);
typedef upb_value upb_value_reader(const void *m, upb_value fval);

typedef const void *upb_seqbegin_handler(const void *s);
typedef const void *upb_seqnext_handler(const void *s, const void *iter);
typedef upb_value upb_seqget_handler(const void *iter);
INLINE bool upb_seq_done(const void *iter) { return iter == NULL; }

typedef struct _upb_accessor_vtbl {
  // Writers.  These take an fval as a parameter because the callbacks are used
  // as upb_handlers, but the fval is always the fielddef for that field.
  upb_startfield_handler *startsubmsg;     // Non-repeated submsg fields.
  upb_value_handler      *set;             // Non-repeated scalar fields.
  upb_startfield_handler *startseq;        // Repeated fields only.
  upb_startfield_handler *appendsubmsg;    // Repeated submsg fields.
  upb_value_handler      *append;          // Repeated scalar fields.

  // TODO: expect to also need endsubmsg and endseq.

  // Readers.
  upb_has_reader         *has;
  upb_value_reader       *getseq;
  upb_value_reader       *get;
  upb_seqbegin_handler   *seqbegin;
  upb_seqnext_handler    *seqnext;
  upb_seqget_handler     *seqget;
} upb_accessor_vtbl;

// Registers handlers for writing into a message of the given type using
// whatever accessors it has defined.
upb_mhandlers *upb_accessors_reghandlers(upb_handlers *h, const upb_msgdef *m);

INLINE void upb_msg_clearbit(void *msg, const upb_fielddef *f) {
  ((char*)msg)[f->hasbit / 8] &= ~(1 << (f->hasbit % 8));
}

/* upb_msg/upb_seq ************************************************************/

// These accessor functions are simply convenience methods for reading or
// writing to a message through its accessors.

INLINE bool upb_msg_has(const void *m, const upb_fielddef *f) {
  return f->accessor && f->accessor->has(m, f->fval);
}

// May only be called for fields that have accessors.
INLINE upb_value upb_msg_get(const void *m, const upb_fielddef *f) {
  assert(f->accessor && !upb_isseq(f));
  return f->accessor->get(m, f->fval);
}

// May only be called for fields that have accessors.
INLINE upb_value upb_msg_getseq(const void *m, const upb_fielddef *f) {
  assert(f->accessor && upb_isseq(f));
  return f->accessor->getseq(m, f->fval);
}

INLINE void upb_msg_set(void *m, const upb_fielddef *f, upb_value val) {
  assert(f->accessor);
  f->accessor->set(m, f->fval, val);
}

INLINE const void *upb_seq_begin(const void *s, const upb_fielddef *f) {
  assert(f->accessor);
  return f->accessor->seqbegin(s);
}
INLINE const void *upb_seq_next(const void *s, const void *iter,
                                const upb_fielddef *f) {
  assert(f->accessor);
  assert(!upb_seq_done(iter));
  return f->accessor->seqnext(s, iter);
}
INLINE upb_value upb_seq_get(const void *iter, const upb_fielddef *f) {
  assert(f->accessor);
  assert(!upb_seq_done(iter));
  return f->accessor->seqget(iter);
}

INLINE bool upb_msg_has_named(const void *m, const upb_msgdef *md,
                              const char *field_name) {
  const upb_fielddef *f = upb_msgdef_ntof(md, field_name);
  return f && upb_msg_has(m, f);
}

INLINE bool upb_msg_get_named(const void *m, const upb_msgdef *md,
                                   const char *field_name, upb_value *val) {
  const upb_fielddef *f = upb_msgdef_ntof(md, field_name);
  if (!f) return false;
  *val = upb_msg_get(m, f);
  return true;
}

// Value writers for every in-memory type: write the data to a known offset
// from the closure "c."
//
// TODO(haberman): instead of having standard writer functions, should we have
// a bool in the accessor that says "write raw value to the field's offset"?
upb_flow_t upb_stdmsg_setint64(void *c, upb_value fval, upb_value val);
upb_flow_t upb_stdmsg_setint32(void *c, upb_value fval, upb_value val);
upb_flow_t upb_stdmsg_setuint64(void *c, upb_value fval, upb_value val);
upb_flow_t upb_stdmsg_setuint32(void *c, upb_value fval, upb_value val);
upb_flow_t upb_stdmsg_setdouble(void *c, upb_value fval, upb_value val);
upb_flow_t upb_stdmsg_setfloat(void *c, upb_value fval, upb_value val);
upb_flow_t upb_stdmsg_setbool(void *c, upb_value fval, upb_value val);
upb_flow_t upb_stdmsg_setptr(void *c, upb_value fval, upb_value val);

#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif
