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

typedef struct {
  bool junk;
  upb_fieldtype_t type;
  // For upb_issubmsg(f) only, the index into the msgdef array of the submsg.
  // -1 if unset (indicates that submsg should be skipped).
  int32_t msgent_index;
  upb_value fval;
  union {
    upb_value_handler_t value;
    upb_startsubmsg_handler_t startsubmsg;
  } cb;
  upb_endsubmsg_handler_t endsubmsg;
} upb_handlers_fieldent;

typedef struct {
  upb_startmsg_handler_t startmsg;
  upb_endmsg_handler_t endmsg;
  upb_unknownval_handler_t unknownval;
  // Maps field number -> upb_handlers_fieldent.
  upb_inttable fieldtab;
} upb_handlers_msgent;

typedef struct {
  upb_msgdef *msgdef;
  int msgent_index;
} upb_handlers_frame;

struct _upb_handlers {
  // Array of msgdefs, [0]=toplevel.
  upb_handlers_msgent *msgs;
  int msgs_len, msgs_size;
  upb_msgdef *toplevel_msgdef;  // We own a ref.
  upb_handlers_msgent *msgent;
  upb_handlers_frame stack[UPB_MAX_TYPE_DEPTH], *top, *limit;
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
                              upb_fieldtype_t type, upb_value_handler_t value,
                              upb_value fval);
void upb_register_typed_submsg(upb_handlers *h, upb_field_number_t fieldnum,
                               upb_fieldtype_t type,
                               upb_startsubmsg_handler_t start,
                               upb_endsubmsg_handler_t end,
                               upb_value fval);
void upb_handlers_typed_link(upb_handlers *h,
                             upb_field_number_t fieldnum,
                             upb_fieldtype_t type,
                             int frames);
void upb_handlers_typed_push(upb_handlers *h, upb_field_number_t fieldnum,
                             upb_fieldtype_t type);
void upb_handlers_typed_pop(upb_handlers *h);

INLINE upb_handlers_msgent *upb_handlers_getmsgent(upb_handlers *h,
                                                   upb_handlers_fieldent *f) {
  assert(f->msgent_index != -1);
  return &h->msgs[f->msgent_index];
}
upb_handlers_fieldent *upb_handlers_lookup(upb_inttable *dispatch_table, upb_field_number_t fieldnum);


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
  upb_handlers_fieldent *f;
  void *closure;
  size_t end_offset;  // For groups, 0.
} upb_dispatcher_frame;

typedef struct {
  upb_dispatcher_frame *top, *limit;

  upb_handlers *handlers;

  // Msg and dispatch table for the current level.
  upb_handlers_msgent *msgent;
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
  upb_dispatcher_frame stack[UPB_MAX_NESTING];
  upb_status status;
} upb_dispatcher;

INLINE bool upb_dispatcher_skipping(upb_dispatcher *d) {
  return d->current_depth >= d->skip_depth;
}

// If true, upb_dispatcher_skipping(d) must also be true.
INLINE bool upb_dispatcher_noframe(upb_dispatcher *d) {
  return d->current_depth >= d->noframe_depth;
}


typedef upb_handlers_fieldent upb_dispatcher_field;

void upb_dispatcher_init(upb_dispatcher *d, upb_handlers *h, size_t top_end_offset);
void upb_dispatcher_reset(upb_dispatcher *d);
void upb_dispatcher_uninit(upb_dispatcher *d);

upb_flow_t upb_dispatch_startmsg(upb_dispatcher *d, void *closure);
void upb_dispatch_endmsg(upb_dispatcher *d, upb_status *status);

// Looks up a field by number for the current message.
INLINE upb_dispatcher_field *upb_dispatcher_lookup(upb_dispatcher *d,
                                                   upb_field_number_t n) {
  return (upb_dispatcher_field*)upb_inttable_fastlookup(
      d->dispatch_table, n, sizeof(upb_dispatcher_field));
}

// Dispatches values or submessages -- the client is responsible for having
// previously looked up the field.
upb_flow_t upb_dispatch_startsubmsg(upb_dispatcher *d,
                                    upb_dispatcher_field *f,
                                    size_t userval);
upb_flow_t upb_dispatch_endsubmsg(upb_dispatcher *d);

INLINE upb_flow_t upb_dispatch_value(upb_dispatcher *d, upb_dispatcher_field *f,
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

/* upb_bytesrc ****************************************************************/

// upb_bytesrc is a pull interface for streams of bytes, basically an
// abstraction of read()/fread(), but it avoids copies where possible.

typedef upb_strlen_t (*upb_bytesrc_read_fptr)(
    upb_bytesrc *src, void *buf, upb_strlen_t count, upb_status *status);
typedef bool (*upb_bytesrc_getstr_fptr)(
    upb_bytesrc *src, upb_string *str, upb_status *status);

typedef struct {
  upb_bytesrc_read_fptr   read;
  upb_bytesrc_getstr_fptr getstr;
} upb_bytesrc_vtbl;

struct _upb_bytesrc {
  upb_bytesrc_vtbl *vtbl;
};

INLINE void upb_bytesrc_init(upb_bytesrc *s, upb_bytesrc_vtbl *vtbl) {
  s->vtbl = vtbl;
}

// Reads up to "count" bytes into "buf", returning the total number of bytes
// read.  If 0, indicates error and puts details in "status".
INLINE upb_strlen_t upb_bytesrc_read(upb_bytesrc *src, void *buf,
                                     upb_strlen_t count, upb_status *status) {
  return src->vtbl->read(src, buf, count, status);
}

// Like upb_bytesrc_read(), but modifies "str" in-place.  Caller must ensure
// that "str" is created or just recycled.  Returns "false" if no data was
// returned, either due to error or EOF (check status for details).
//
// In comparison to upb_bytesrc_read(), this call can possibly alias existing
// string data (which avoids a copy).  On the other hand, if the data was *not*
// already in an existing string, this copies it into a upb_string, and if the
// data needs to be put in a specific range of memory (because eg. you need to
// put it into a different kind of string object) then upb_bytesrc_get() could
// save you a copy.
INLINE bool upb_bytesrc_getstr(upb_bytesrc *src, upb_string *str,
                               upb_status *status) {
  return src->vtbl->getstr(src, str, status);
}


/* upb_bytesink ***************************************************************/

// upb_bytesink: push interface for streams of bytes, basically an abstraction
// of write()/fwrite(), but it avoids copies where possible.

struct _upb_bytesink;
typedef struct _upb_bytesink upb_bytesink;
typedef upb_strlen_t (*upb_bytesink_putstr_fptr)(
    upb_bytesink *bytesink, upb_string *str, upb_status *status);
typedef upb_strlen_t (*upb_bytesink_vprintf_fptr)(
    upb_bytesink *bytesink, upb_status *status, const char *fmt, va_list args);

typedef struct {
  upb_bytesink_putstr_fptr  putstr;
  upb_bytesink_vprintf_fptr vprintf;
} upb_bytesink_vtbl;

struct _upb_bytesink {
  upb_bytesink_vtbl *vtbl;
};

INLINE void upb_bytesink_init(upb_bytesink *s, upb_bytesink_vtbl *vtbl) {
  s->vtbl = vtbl;
}


// TODO: Figure out how buffering should be handled.  Should the caller buffer
// data and only call these functions when a buffer is full?  Seems most
// efficient, but then buffering has to be configured in the caller, which
// could be anything, which makes it hard to have a standard interface for
// controlling buffering.
//
// The downside of having the bytesink buffer is efficiency: the caller is
// making more (virtual) function calls, and the caller can't arrange to have
// a big contiguous buffer.  The bytesink can do this, but will have to copy
// to make the data contiguous.

// Returns the number of bytes written.
INLINE upb_strlen_t upb_bytesink_printf(upb_bytesink *sink, upb_status *status,
                                        const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  upb_strlen_t ret = sink->vtbl->vprintf(sink, status, fmt, args);
  va_end(args);
  return ret;
}

// Puts the given string, returning true if the operation was successful, otherwise
// check "status" for details.  Ownership of the string is *not* passed; if
// the callee wants a reference he must call upb_string_getref() on it.
INLINE upb_strlen_t upb_bytesink_putstr(upb_bytesink *sink, upb_string *str,
                                        upb_status *status) {
  return sink->vtbl->putstr(sink, str, status);
}

#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif
