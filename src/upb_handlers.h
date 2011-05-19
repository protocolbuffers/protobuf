/*
 * upb - a minimalist implementation of protocol buffers.
 *
 * Copyright (c) 2010-2011 Google Inc.  See LICENSE for details.
 * Author: Josh Haberman <jhaberman@gmail.com>
 *
 * upb_handlers is a generic visitor-like interface for iterating over a stream
 * of protobuf data.  You can register function pointers that will be called
 * for each message and/or field as the data is being parsed or iterated over,
 * without having to know the source format that we are parsing from.  This
 * decouples the parsing logic from the processing logic.
 */

#ifndef UPB_HANDLERS_H
#define UPB_HANDLERS_H

#include <limits.h>
#include "upb.h"
#include "upb_def.h"

#ifdef __cplusplus
extern "C" {
#endif

/* upb_handlers ***************************************************************/

// A upb_handlers object represents a graph of handlers.  Each message can have
// a set of handlers as well as a set of fields which themselves have handlers.
// Fields that represent submessages or groups are linked to other message
// handlers, so the overall set of handlers can form a graph structure (which
// may be cyclic).
//
// The upb_mhandlers (message handlers) object can have the following handlers:
//
//   static upb_flow_t startmsg(void *closure) {
//     // Called when the message begins.  "closure" was supplied by our caller.
//     return UPB_CONTINUE;
//   }
//
//   static void endmsg(void *closure, upb_status *status) {
//     // Called when processing of this message ends, whether in success or
//     // failure.  "status" indicates the final status of processing, and can
//     /  also be modified in-place to update the final status.
//     //
//     // Since this callback is guaranteed to always be called eventually, it
//     // can be used to free any resources that were allocated during processing.
//   }
//
//   TODO: unknown field handler.
//
// The upb_fhandlers (field handlers) object can have the following handlers:
//
//   static upb_flow_t value(void *closure, upb_value fval, upb_value val) {
//     // Called when the field's value is encountered.  "fval" contains
//     // whatever value was bound to this field at registration type
//     // (for upb_register_all(), this will be the field's upb_fielddef*).
//     return UPB_CONTINUE;
//   }
//
//   static upb_sflow_t startsubmsg(void *closure, upb_value fval) {
//     // Called when a submessage begins.  The second element of the return
//     // value is the closure for the submessage.
//     return UPB_CONTINUE_WITH(closure);
//   }
//
//   static upb_flow_t endsubmsg(void *closure, upb_value fval) {
//     // Called when a submessage ends.
//     return UPB_CONTINUE;
//   }
//
// All handlers except the endmsg handler return a value from this enum, to
// control whether parsing will continue or not.
typedef enum {
  // Data source should continue calling callbacks.
  UPB_CONTINUE = 0,

  // Halt processing permanently (in a non-resumable way).  The endmsg handlers
  // for any currently open messages will be called which can supply a more
  // specific status message.  No further input data will be consumed.
  UPB_BREAK,

  // Skips to the end of the current submessage (or if we are at the top
  // level, skips to the end of the entire message).  In other words, it is
  // like a UPB_BREAK that applies only to the current level.
  //
  // If you UPB_SKIPSUBMSG from a startmsg handler, the endmsg handler will
  // be called to perform cleanup and return a status.  Returning
  // UPB_SKIPSUBMSG from a startsubmsg handler will *not* call the startmsg,
  // endmsg, or endsubmsg handlers.
  //
  // If UPB_SKIPSUBMSG is called from the top-level message, no further input
  // data will be consumed.
  UPB_SKIPSUBMSG,

  // TODO: Add UPB_SUSPEND, for resumable producers/consumers.
} upb_flow_t;

// Typedefs for all of the handler functions defined above.
typedef struct _upb_sflow upb_sflow_t;
typedef upb_flow_t (upb_startmsg_handler)(void *c);
typedef void (upb_endmsg_handler)(void *c, upb_status *status);
typedef upb_flow_t (upb_value_handler)(void *c, upb_value fval, upb_value val);
typedef upb_sflow_t (upb_startsubmsg_handler)(void *closure, upb_value fval);
typedef upb_flow_t (upb_endsubmsg_handler)(void *closure, upb_value fval);

// No-op implementations of all of the above handlers.  Use these instead of
// rolling your own -- the JIT can recognize these and optimize away the call.
upb_flow_t upb_startmsg_nop(void *closure);
void upb_endmsg_nop(void *closure, upb_status *status);
upb_flow_t upb_value_nop(void *closure, upb_value fval, upb_value val);
upb_sflow_t upb_startsubmsg_nop(void *closure, upb_value fval);
upb_flow_t upb_endsubmsg_nop(void *closure, upb_value fval);

// Structure definitions.  Do not access any fields directly!  Accessors are
// provided for the fields that may be get/set.
typedef struct _upb_mhandlers {
  upb_startmsg_handler *startmsg;
  upb_endmsg_handler *endmsg;
  upb_inttable fieldtab;  // Maps field number -> upb_fhandlers.
  uint32_t jit_startmsg_pclabel;
  uint32_t jit_endofbuf_pclabel;
  uint32_t jit_endofmsg_pclabel;
  uint32_t jit_unknownfield_pclabel;
  bool is_group;
  int32_t jit_parent_field_done_pclabel;
  uint32_t max_field_number;
  // Currently keyed on field number.  Could also try keying it
  // on encoded or decoded tag, or on encoded field number.
  void **tablearray;
} upb_mhandlers;

struct _upb_decoder;
typedef struct _upb_fieldent {
  bool junk;
  upb_fieldtype_t type;
  bool repeated;
  bool is_repeated_primitive;
  uint32_t number;
  upb_mhandlers *submsg;  // Must be set iff upb_issubmsgtype(type) == true.
  upb_value fval;
  upb_value_handler *value;
  upb_startsubmsg_handler *startsubmsg;
  upb_endsubmsg_handler *endsubmsg;
  uint32_t jit_pclabel;
  uint32_t jit_pclabel_notypecheck;
  uint32_t jit_submsg_done_pclabel;
  void (*decode)(struct _upb_decoder *d, struct _upb_fieldent *f);
} upb_fhandlers;

struct _upb_handlers {
  // Array of msgdefs, [0]=toplevel.
  upb_mhandlers **msgs;
  int msgs_len, msgs_size;
  bool should_jit;
};
typedef struct _upb_handlers upb_handlers;

void upb_handlers_init(upb_handlers *h);
void upb_handlers_uninit(upb_handlers *h);

// The startsubmsg handler needs to also pass a closure to the submsg.
struct _upb_sflow {
  upb_flow_t flow;
  void *closure;
};
INLINE upb_sflow_t UPB_SFLOW(upb_flow_t flow, void *closure) {
  upb_sflow_t ret = {flow, closure};
  return ret;
}
#define UPB_CONTINUE_WITH(c) UPB_SFLOW(UPB_CONTINUE, c)
#define UPB_SBREAK UPB_SFLOW(UPB_BREAK, NULL)

// Appends a new message to the graph of handlers and returns it.  This message
// can be obtained later at index upb_handlers_msgcount()-1.  All handlers will
// be initialized to no-op handlers.
upb_mhandlers *upb_handlers_newmsg(upb_handlers *h);
upb_mhandlers *upb_handlers_getmsg(upb_handlers *h, int index);

// Creates a new field with the given name and number.  There must not be an
// existing field with either this name or number or abort() will be called.
// TODO: this should take a name also.
upb_fhandlers *upb_mhandlers_newfield(upb_mhandlers *m, uint32_t n,
                                      upb_fieldtype_t type, bool repeated);
// Like the previous but for MESSAGE or GROUP fields.  For GROUP fields, the
// given submessage must not have any fields with this field number.
upb_fhandlers *upb_mhandlers_newsubmsgfield(upb_mhandlers *m, uint32_t n,
                                            upb_fieldtype_t type, bool repeated,
                                            upb_mhandlers *subm);

// upb_mhandlers accessors.
#define UPB_MHANDLERS_ACCESSORS(name, type) \
  INLINE void upb_mhandlers_set ## name(upb_mhandlers *m, type v){m->name = v;} \
  INLINE type upb_mhandlers_get ## name(upb_mhandlers *m) { return m->name; }
UPB_MHANDLERS_ACCESSORS(startmsg, upb_startmsg_handler*);
UPB_MHANDLERS_ACCESSORS(endmsg, upb_endmsg_handler*);

// upb_fhandlers accessors
#define UPB_FHANDLERS_ACCESSORS(name, type) \
  INLINE void upb_fhandlers_set ## name(upb_fhandlers *f, type v){f->name = v;} \
  INLINE type upb_fhandlers_get ## name(upb_fhandlers *f) { return f->name; }
UPB_FHANDLERS_ACCESSORS(fval, upb_value)
UPB_FHANDLERS_ACCESSORS(value, upb_value_handler*)
UPB_FHANDLERS_ACCESSORS(startsubmsg, upb_startsubmsg_handler*)
UPB_FHANDLERS_ACCESSORS(endsubmsg, upb_endsubmsg_handler*)
UPB_FHANDLERS_ACCESSORS(submsg, upb_mhandlers*)

// Convenience function for registering handlers for all messages and
// fields in a msgdef and all its children.  For every registered message
// "msgreg_cb" will be called with the newly-created mhandlers, and likewise
// with "fieldreg_cb"
//
// See upb_handlers_reghandlerset() below for an example.
typedef void upb_onmsgreg(void *closure, upb_mhandlers *mh, upb_msgdef *m);
typedef void upb_onfieldreg(void *closure, upb_fhandlers *mh, upb_fielddef *m);
upb_mhandlers *upb_handlers_regmsgdef(upb_handlers *h, upb_msgdef *m,
                                      upb_onmsgreg *msgreg_cb,
                                      upb_onfieldreg *fieldreg_cb,
                                      void *closure);

// Convenience function for registering a set of handlers for all messages and
// fields in a msgdef and its children, with the fval bound to the upb_fielddef.
// Any of the handlers may be NULL, in which case no callback will be set and
// the nop callback will be used.
typedef struct {
  upb_startmsg_handler *startmsg;
  upb_endmsg_handler *endmsg;
  upb_value_handler *value;
  upb_startsubmsg_handler *startsubmsg;
  upb_endsubmsg_handler *endsubmsg;
} upb_handlerset;

INLINE void upb_onmreg_hset(void *c, upb_mhandlers *mh, upb_msgdef *m) {
  (void)m;
  upb_handlerset *hs = (upb_handlerset*)c;
  if (hs->startmsg) upb_mhandlers_setstartmsg(mh, hs->startmsg);
  if (hs->endmsg) upb_mhandlers_setendmsg(mh, hs->endmsg);
}
INLINE void upb_onfreg_hset(void *c, upb_fhandlers *fh, upb_fielddef *f) {
  upb_handlerset *hs = (upb_handlerset*)c;
  if (hs->value) upb_fhandlers_setvalue(fh, hs->value);
  if (hs->startsubmsg) upb_fhandlers_setstartsubmsg(fh, hs->startsubmsg);
  if (hs->endsubmsg) upb_fhandlers_setendsubmsg(fh, hs->endsubmsg);
  upb_value val;
  upb_value_setfielddef(&val, f);
  upb_fhandlers_setfval(fh, val);
}
INLINE upb_mhandlers *upb_handlers_reghandlerset(upb_handlers *h, upb_msgdef *m,
                                                 upb_handlerset *hs) {
  return upb_handlers_regmsgdef(h, m, &upb_onmreg_hset, &upb_onfreg_hset, hs);
}


/* upb_dispatcher *************************************************************/

// upb_dispatcher can be used by sources of data to invoke the appropriate
// handlers on a upb_handlers object.  Besides maintaining the runtime stack of
// closures and handlers, the dispatcher checks the return status of user
// callbacks and properly handles statuses other than UPB_CONTINUE, invoking
// "skip" or "exit" handlers on the underlying data source as appropriate.

typedef struct {
  upb_fhandlers *f;
  void *closure;

  // Members to use as the data source requires.
  void *srcclosure;
  uint16_t msgindex;
  uint16_t fieldindex;
  uint32_t end_offset;

  // Does this frame represent a sequence or a submsg (f might be both).
  // We only need a single bit here, but this will make each individual
  // frame grow from 32 to 40 bytes on LP64, which is a bit excessive.
  bool is_sequence;
} upb_dispatcher_frame;

// Called when some of the input needs to be skipped.  All frames from
// top to bottom, inclusive, should be skipped.
typedef void upb_skip_handler(void *, upb_dispatcher_frame *top,
                              upb_dispatcher_frame *bottom);
typedef void upb_exit_handler(void *);

typedef struct {
  upb_dispatcher_frame *top, *limit;

  upb_handlers *handlers;

  // Msg and dispatch table for the current level.
  upb_mhandlers *msgent;
  upb_inttable *dispatch_table;
  upb_skip_handler *skip;
  upb_exit_handler *exit;
  void *srcclosure;

  // Stack.
  upb_status status;
  upb_dispatcher_frame stack[UPB_MAX_NESTING];
} upb_dispatcher;

void upb_dispatcher_init(upb_dispatcher *d, upb_handlers *h,
                         upb_skip_handler *skip, upb_exit_handler *exit,
                         void *closure);
upb_dispatcher_frame *upb_dispatcher_reset(upb_dispatcher *d, void *topclosure);
void upb_dispatcher_uninit(upb_dispatcher *d);

// Tests whether the runtime stack is in the base level message.
bool upb_dispatcher_stackempty(upb_dispatcher *d);

// Looks up a field by number for the current message.
INLINE upb_fhandlers *upb_dispatcher_lookup(upb_dispatcher *d, uint32_t n) {
  return (upb_fhandlers*)upb_inttable_fastlookup(
      d->dispatch_table, n, sizeof(upb_fhandlers));
}

void _upb_dispatcher_unwind(upb_dispatcher *d, upb_flow_t flow);

// Dispatch functions -- call the user handler and handle errors.
INLINE void upb_dispatch_value(upb_dispatcher *d, upb_fhandlers *f,
                               upb_value val) {
  upb_flow_t flow = f->value(d->top->closure, f->fval, val);
  if (flow != UPB_CONTINUE) _upb_dispatcher_unwind(d, flow);
}
void upb_dispatch_startmsg(upb_dispatcher *d);
void upb_dispatch_endmsg(upb_dispatcher *d, upb_status *status);
upb_dispatcher_frame *upb_dispatch_startsubmsg(upb_dispatcher *d,
                                               upb_fhandlers *f);
upb_dispatcher_frame *upb_dispatch_endsubmsg(upb_dispatcher *d);

#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif
