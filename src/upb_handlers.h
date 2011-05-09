/*
 * upb - a minimalist implementation of protocol buffers.
 *
 * Copyright (c) 2010-2011 Google Inc.  See LICENSE for details.
 * Author: Josh Haberman <jhaberman@gmail.com>
 *
 * This file defines general-purpose streaming data interfaces:
 *
 * - upb_handlers: represents a set of callbacks, very much like in XML's SAX
 *   API, that a client can register to do a streaming tree traversal over a
 *   stream of structured protobuf data, without knowing where that data is
 *   coming from.
 *
 * - upb_bytesrc: a pull interface for streams of bytes, basically an
 *   abstraction of read()/fread(), but it avoids copies where possible.
 *
 * - upb_bytesink: push interface for streams of bytes, basically an
 *   abstraction of write()/fwrite(), but it avoids copies where possible.
 *
 * All of the encoders and decoders are based on these generic interfaces,
 * which lets you write streaming algorithms that do not depend on a specific
 * serialization format; for example, you can write a pretty printer that works
 * with input that came from protobuf binary format, protobuf text format, or
 * even an in-memory upb_msg -- the pretty printer will not know the
 * difference.
 */

#ifndef UPB_STREAM_H
#define UPB_STREAM_H

#include <limits.h>
#include "upb.h"
#include "upb_def.h"

#ifdef __cplusplus
extern "C" {
#endif

/* upb_handlers ***************************************************************/

// A upb_handlers object is a table of callbacks that are bound to specific
// messages and fields.  A consumer of data registers callbacks and then
// passes the upb_handlers object to the producer, which calls them at the
// appropriate times.

// All handlers except the endmsg handler return a value from this enum, to
// control whether parsing will continue or not.
typedef enum {
  // Data source should continue calling callbacks.
  UPB_CONTINUE = 0,

  // Halt processing permanently (in a non-resumable way).  The endmsg handlers
  // for any currently open messages will be called which can supply a more
  // specific status message.  If UPB_BREAK is returned from inside a delegated
  // message, processing will continue normally in the containing message (though
  // the containing message can inspect the returned status and choose to also
  // return UPB_BREAK if it is not ok).
  UPB_BREAK,

  // Skips to the end of the current submessage (or if we are at the top
  // level, skips to the end of the entire message).  In other words, it is
  // like a UPB_BREAK that applies only to the current level.
  //
  // If you UPB_SKIPSUBMSG from a startmsg handler, the endmsg handler will
  // be called to perform cleanup and return a status.  Returning
  // UPB_SKIPSUBMSG from a startsubmsg handler will *not* call the startmsg,
  // endmsg, or endsubmsg handlers.
  UPB_SKIPSUBMSG,

  // TODO: Add UPB_SUSPEND, for resumable producers/consumers.
} upb_flow_t;

typedef struct _upb_sflow upb_sflow_t;
typedef upb_flow_t (*upb_startmsg_handler_t)(void *closure);
typedef void (*upb_endmsg_handler_t)(void *closure, upb_status *status);
typedef upb_flow_t (*upb_value_handler_t)(
    void *closure, upb_value fval, upb_value val);
typedef upb_sflow_t (*upb_startsubmsg_handler_t)(
    void *closure, upb_value fval);
typedef upb_flow_t (*upb_endsubmsg_handler_t)(void *closure, upb_value fval);
typedef upb_flow_t (*upb_unknownval_handler_t)(
    void *closure, upb_field_number_t fieldnum, upb_value val);

upb_flow_t upb_startmsg_nop(void *closure);
void upb_endmsg_nop(void *closure, upb_status *status);
upb_flow_t upb_value_nop(void *closure, upb_value fval, upb_value val);
upb_sflow_t upb_startsubmsg_nop(void *closure, upb_value fval);
upb_flow_t upb_endsubmsg_nop(void *closure, upb_value fval);
upb_flow_t upb_unknownval_nop(void *closure, upb_field_number_t fieldnum,
                                     upb_value val);
struct _upb_decoder;
typedef struct _upb_fieldent {
  bool junk;
  upb_fieldtype_t type;
  bool repeated;
  bool is_repeated_primitive;
  uint32_t number;
  // For upb_issubmsg(f) only, the index into the msgdef array of the submsg.
  // -1 if unset (indicates that submsg should be skipped).
  int32_t msgent_index;
  upb_value fval;
  union {
    upb_value_handler_t value;
    upb_startsubmsg_handler_t startsubmsg;
  } cb;
  upb_endsubmsg_handler_t endsubmsg;
  uint32_t jit_pclabel;
  uint32_t jit_pclabel_notypecheck;
  uint32_t jit_submsg_done_pclabel;
  void (*decode)(struct _upb_decoder *d, struct _upb_fieldent *f);
} upb_fieldent;

typedef struct _upb_msgent {
  upb_startmsg_handler_t startmsg;
  upb_endmsg_handler_t endmsg;
  upb_unknownval_handler_t unknownval;
  // Maps field number -> upb_fieldent.
  upb_inttable fieldtab;
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
} upb_msgent;

typedef struct {
  upb_msgdef *msgdef;
  int msgent_index;
} upb_handlers_frame;

struct _upb_handlers {
  // Array of msgdefs, [0]=toplevel.
  upb_msgent *msgs;
  int msgs_len, msgs_size;
  upb_msgdef *toplevel_msgdef;  // We own a ref.
  upb_msgent *msgent;
  upb_handlers_frame stack[UPB_MAX_TYPE_DEPTH], *top, *limit;
  bool should_jit;
};
typedef struct _upb_handlers upb_handlers;

// The handlers object takes a ref on md.  md can be NULL iff the client calls
// only upb_*_typed_*() (only upb_symtab should do this).
void upb_handlers_init(upb_handlers *h, upb_msgdef *md);
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
#define UPB_S_BREAK UPB_SFLOW(UPB_BREAK, NULL)

// Each message can have its own set of handlers.  Here are empty definitions
// of the handlers for convenient copy/paste.
// TODO: Should endsubmsg get a copy of the upb_status*, so it can decide what
// to do in the case of a delegated failure?
//
// static upb_flow_t startmsg(void *closure) {
//   // Called when the message begins.  "closure" was supplied by our caller.
//   // "mval" is whatever was bound to this message at registration time (for
//   // upb_register_all() it will be its upb_msgdef*).
//   return UPB_CONTINUE;
// }
//
// static void endmsg(void *closure, upb_status *status) {
//   // Called when processing of this top-level message ends, whether in
//   // success or failure.  "status" indicates the final status of processing,
//   // and can also be modified in-place to update the final status.
//   //
//   // Since this callback is guaranteed to always be called eventually, it
//   // can be used to free any resources that were allocated during processing.
// }
//
// static upb_flow_t value(void *closure, upb_value fval, upb_value val) {
//   // Called for every non-submessage value in the stream.  "fval" contains
//   // whatever value was bound to this field at registration type
//   // (for upb_register_all(), this will be the field's upb_fielddef*).
//   return UPB_CONTINUE;
// }
//
// static upb_sflow_t startsubmsg(void *closure, upb_value fval) {
//   // Called when a submessage begins.  The second element of the return
//   // value is the closure for the submessage.
//   return UPB_CONTINUE_WITH(closure);
// }
//
// static upb_flow_t endsubmsg(void *closure, upb_value fval) {
//   // Called when a submessage ends.
//   return UPB_CONTINUE;
// }
//
// static upb_flow_t unknownval(void *closure, upb_field_number_t fieldnum,
//                              upb_value val) {
//   // Called with an unknown value is encountered.
//   return UPB_CONTINUE;
// }

// Functions to register the above handlers.
// TODO: as an optimization, we could special-case handlers that don't
// need fval, to avoid even generating the code that sets the argument.
// If a value does not have a handler registered and there is no unknownval
// handler, the value will be skipped.
void upb_register_startend(upb_handlers *h, upb_startmsg_handler_t startmsg,
                           upb_endmsg_handler_t endmsg);
void upb_register_value(upb_handlers *h, upb_fielddef *f,
                        upb_value_handler_t value, upb_value fval);
void upb_register_unknownval(upb_handlers *h, upb_unknownval_handler_t unknown);

// To register handlers for a submessage, push the fielddef and pop it
// when you're done.  This can be used to delegate a submessage to a
// different processing component which does not need to be aware whether
// it is at the top level or not.
void upb_handlers_push(upb_handlers *h, upb_fielddef *f,
                       upb_startsubmsg_handler_t start,
                       upb_endsubmsg_handler_t end, upb_value fval,
                       bool delegate);
void upb_handlers_pop(upb_handlers *h, upb_fielddef *f);

// In the case where types are self-recursive or mutually recursive, you can
// use this function which will link a set of handlers to a set that is
// already on our stack.  This allows us to handle a tree of arbitrary
// depth without having to register an arbitrary number of levels of handlers.
// Returns "true" if the given type is indeed on the stack already and was
// linked.
//
// If more than one message of this type is on the stack, it chooses the
// one that is deepest in the tree (if necessary, we could give the caller
// more control over this).
bool upb_handlers_link(upb_handlers *h, upb_fielddef *f);

// Convenience function for registering the given handler for the given
// field path.  This will overwrite any startsubmsg handlers that were
// previously registered along the path.  These can be overwritten again
// later if desired.
// TODO: upb_register_path_submsg()?
void upb_register_path_value(upb_handlers *h, const char *path,
                             upb_value_handler_t value, upb_value fval);

// Convenience function for registering a single set of handlers on every
// message in our hierarchy.  mvals are bound to upb_msgdef* and fvals are
// bound to upb_fielddef*.  Any of the handlers can be NULL.
void upb_register_all(upb_handlers *h, upb_startmsg_handler_t start,
                      upb_endmsg_handler_t end,
                      upb_value_handler_t value,
                      upb_startsubmsg_handler_t startsubmsg,
                      upb_endsubmsg_handler_t endsubmsg,
                      upb_unknownval_handler_t unknown);

// TODO: for clients that want to increase efficiency by preventing bytesrcs
// from automatically being converted to strings in the value callback.
// INLINE void upb_handlers_use_bytesrcs(upb_handlers *h, bool use_bytesrcs);

// Low-level functions -- internal-only.
void upb_register_typed_value(upb_handlers *h, upb_field_number_t fieldnum,
                              upb_fieldtype_t type, bool repeated,
                              upb_value_handler_t value, upb_value fval);
void upb_register_typed_submsg(upb_handlers *h, upb_field_number_t fieldnum,
                               upb_fieldtype_t type, bool repeated,
                               upb_startsubmsg_handler_t start,
                               upb_endsubmsg_handler_t end,
                               upb_value fval);
void upb_handlers_typed_link(upb_handlers *h, upb_field_number_t fieldnum,
                             upb_fieldtype_t type, bool repeated, int frames);
void upb_handlers_typed_push(upb_handlers *h, upb_field_number_t fieldnum,
                             upb_fieldtype_t type, bool repeated);
void upb_handlers_typed_pop(upb_handlers *h);

INLINE upb_msgent *upb_handlers_getmsgent(upb_handlers *h, upb_fieldent *f) {
  assert(f->msgent_index != -1);
  return &h->msgs[f->msgent_index];
}
upb_fieldent *upb_handlers_lookup(upb_inttable *dispatch_table, upb_field_number_t fieldnum);


/* upb_dispatcher *************************************************************/

// upb_dispatcher can be used by sources of data to invoke the appropriate
// handlers.  It takes care of details such as:
//   - ensuring all endmsg callbacks (cleanup handlers) are called.
//   - propagating status all the way back to the top-level message.
//   - handling UPB_BREAK properly (clients only need to handle UPB_SKIPSUBMSG).
//   - handling UPB_SKIPSUBMSG if the client doesn't (but this is less
//     efficient, because then you can't skip the actual work).
//   - tracking the stack of closures.
//
// TODO: it might be best to actually surface UPB_BREAK to clients in the case
// that the can't efficiently skip the submsg; eg. with groups.  Then the client
// would know to just unwind the stack without bothering to consume the rest of
// the input.  On the other hand, it might be important for all the input to be
// consumed, like if this is a submessage of a larger stream.

typedef struct {
  upb_fieldent *f;
  void *closure;
  // Relative to the beginning of this buffer.
  // For groups and the top-level: UINT32_MAX.
  uint32_t end_offset;
  bool is_packed;  // == !upb_issubmsg(f) && end_offset != UPB_REPATEDEND
} upb_dispatcher_frame;

typedef struct {
  upb_dispatcher_frame *top, *limit;

  upb_handlers *handlers;

  // Msg and dispatch table for the current level.
  upb_msgent *msgent;
  upb_inttable *dispatch_table;

  // The number of startsubmsg calls without a corresponding endsubmsg call.
  int current_depth;

  // For all frames >= skip_depth, we are skipping all values in the submsg.
  // For all frames >= noframe_depth, we did not even push a frame.
  // These are INT_MAX when nothing is being skipped.
  // Invariant: noframe_depth >= skip_depth
  int skip_depth;
  int noframe_depth;

  // Depth of stack entries we'll skip if a callback returns UPB_BREAK.
  int delegated_depth;

  // Stack.
  upb_status status;
  upb_dispatcher_frame stack[UPB_MAX_NESTING];
} upb_dispatcher;

INLINE bool upb_dispatcher_skipping(upb_dispatcher *d) {
  return d->current_depth >= d->skip_depth;
}

// If true, upb_dispatcher_skipping(d) must also be true.
INLINE bool upb_dispatcher_noframe(upb_dispatcher *d) {
  return d->current_depth >= d->noframe_depth;
}


void upb_dispatcher_init(upb_dispatcher *d, upb_handlers *h);
void upb_dispatcher_reset(upb_dispatcher *d, void *top_closure, uint32_t top_end_offset);
void upb_dispatcher_uninit(upb_dispatcher *d);

upb_flow_t upb_dispatch_startmsg(upb_dispatcher *d);
void upb_dispatch_endmsg(upb_dispatcher *d, upb_status *status);

// Looks up a field by number for the current message.
INLINE upb_fieldent *upb_dispatcher_lookup(upb_dispatcher *d,
                                           upb_field_number_t n) {
  return (upb_fieldent*)upb_inttable_fastlookup(
      d->dispatch_table, n, sizeof(upb_fieldent));
}

// Dispatches values or submessages -- the client is responsible for having
// previously looked up the field.
upb_flow_t upb_dispatch_startsubmsg(upb_dispatcher *d,
                                    upb_fieldent *f,
                                    size_t userval);
upb_flow_t upb_dispatch_endsubmsg(upb_dispatcher *d);

INLINE upb_flow_t upb_dispatch_value(upb_dispatcher *d, upb_fieldent *f,
                                     upb_value val) {
  if (upb_dispatcher_skipping(d)) return UPB_SKIPSUBMSG;
  upb_flow_t flow = f->cb.value(d->top->closure, f->fval, val);
  if (flow != UPB_CONTINUE) {
    d->noframe_depth = d->current_depth + 1;
    d->skip_depth = (flow == UPB_BREAK) ? d->delegated_depth : d->current_depth;
    return UPB_SKIPSUBMSG;
  }
  return UPB_CONTINUE;
}
INLINE upb_flow_t upb_dispatch_unknownval(upb_dispatcher *d, upb_field_number_t n,
                                          upb_value val) {
  // TODO.
  (void)d;
  (void)n;
  (void)val;
  return UPB_CONTINUE;
}
INLINE bool upb_dispatcher_stackempty(upb_dispatcher *d) {
  return d->top == d->stack;
}

#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif
