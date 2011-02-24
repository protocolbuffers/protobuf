/*
 * upb - a minimalist implementation of protocol buffers.
 *
 * This file defines four general-purpose streaming data interfaces.
 *
 * - upb_handlers: represents a set of callbacks, very much like in XML's SAX
 *   API, that a client can register to do a streaming tree traversal over a
 *   stream of structured protobuf data, without knowing where that data is
 *   coming from.  There is only one upb_handlers type (it is not a virtual
 *   base class), but the object lets you register any set of handlers.
 *
 *   The upb_handlers interface supports delegation: when entering a submessage,
 *   you can delegate to another set of upb_handlers instead of handling the
 *   submessage yourself.  This allows upb_handlers objects to *compose* -- you
 *   can implement a set of upb_handlers without knowing or caring whether this
 *   is the top-level message or not.
 *
 * The other interfaces are the C equivalent of "virtual base classes" that
 * anyone can implement:
 *
 * - upb_src: an interface that represents a source of streaming protobuf data.
 *   It lets you register a set of upb_handlers, and then call upb_src_run(),
 *   which pulls the protobuf data from somewhere and then calls the handlers.
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
 *
 * Copyright (c) 2010-2011 Joshua Haberman.  See LICENSE for details.
 *
 */

#ifndef UPB_STREAM_H
#define UPB_STREAM_H

#include "upb.h"

#ifdef __cplusplus
extern "C" {
#endif

// Forward-declare.  We can't include upb_def.h; it would be circular.
struct _upb_fielddef;

/* upb_handlers ***************************************************************/

// upb_handlers define the interface by which a upb_src passes data to a
// upb_sink.

// Constants that a handler returns to indicate to its caller whether it should
// continue or not.
typedef enum {
  // Caller should continue sending values to the sink.
  UPB_CONTINUE = 0,

  // Stop processing for now; check status for details.  If no status was set,
  // a generic error will be returned.  If the error is resumable, it is not
  // (yet) defined where processing will resume -- waiting for real-world
  // examples of resumable decoders and resume-requiring clients.  upb_src
  // implementations that are not capable of resuming will override the return
  // status to be non-resumable if a resumable status was set by the handlers.
  UPB_BREAK,

  // Skips to the end of the current submessage (or if we are at the top
  // level, skips to the end of the entire message).
  UPB_SKIPSUBMSG,

  // When returned from a startsubmsg handler, indicates that the submessage
  // should be handled by a different set of handlers, which have been
  // registered on the provided upb_handlers object.  This allows upb_handlers
  // objects to compose; a set of upb_handlers need not know whether it is the
  // top-level message or a sub-message.  May not be returned from any other
  // callback.
  //
  // WARNING!  The delegation API is slated for some significant changes.
  // See: http://code.google.com/p/upb/issues/detail?id=6
  UPB_DELEGATE,
} upb_flow_t;

// upb_handlers
struct _upb_handlers;
typedef struct _upb_handlers upb_handlers;

typedef upb_flow_t (*upb_startmsg_handler_t)(void *closure);
typedef upb_flow_t (*upb_endmsg_handler_t)(void *closure);
typedef upb_flow_t (*upb_value_handler_t)(void *closure,
                                          struct _upb_fielddef *f,
                                          upb_value val);
typedef upb_flow_t (*upb_startsubmsg_handler_t)(void *closure,
                                                struct _upb_fielddef *f,
                                                upb_handlers *delegate_to);
typedef upb_flow_t (*upb_endsubmsg_handler_t)(void *closure,
                                              struct _upb_fielddef *f);
typedef upb_flow_t (*upb_unknownval_handler_t)(void *closure,
                                               upb_field_number_t fieldnum,
                                               upb_value val);

// An empty set of handlers, for convenient copy/paste:
//
// static upb_flow_t startmsg(void *closure) {
//   // Called when the top-level message begins.
//   return UPB_CONTINUE;
// }
//
// static upb_flow_t endmsg(void *closure) {
//   // Called when the top-level message ends.
//   return UPB_CONTINUE;
// }
//
// static upb_flow_t value(void *closure, upb_fielddef *f, upb_value val) {
//   // Called for every value in the stream.
//   return UPB_CONTINUE;
// }
//
// static upb_flow_t startsubmsg(void *closure, upb_fielddef *f,
//                               upb_handlers *delegate_to) {
//   // Called when a submessage begins; can delegate by returning UPB_DELEGATE.
//   return UPB_CONTINUE;
// }
//
// static upb_flow_t endsubmsg(void *closure, upb_fielddef *f) {
//   // Called when a submessage ends.
//   return UPB_CONTINUE;
// }
//
// static upb_flow_t unknownval(void *closure, upb_field_number_t fieldnum,
//                              upb_value val) {
//   // Called with an unknown value is encountered.
//   return UPB_CONTINUE;
// }
//
// // Any handlers you don't need can be set to NULL.
// static upb_handlerset handlers = {
//   startmsg,
//   endmsg,
//   value,
//   startsubmsg,
//   endsubmsg,
//   unknownval,
// };
typedef struct {
  upb_startmsg_handler_t startmsg;
  upb_endmsg_handler_t endmsg;
  upb_value_handler_t value;
  upb_startsubmsg_handler_t startsubmsg;
  upb_endsubmsg_handler_t endsubmsg;
  upb_unknownval_handler_t unknownval;
} upb_handlerset;

// Functions to register handlers on a upb_handlers object.
INLINE void upb_handlers_init(upb_handlers *h);
INLINE void upb_handlers_uninit(upb_handlers *h);
INLINE void upb_handlers_reset(upb_handlers *h);
INLINE bool upb_handlers_isempty(upb_handlers *h);
INLINE void upb_register_handlerset(upb_handlers *h, upb_handlerset *set);

// TODO: for clients that want to increase efficiency by preventing bytesrcs
// from automatically being converted to strings in the value callback.
// INLINE void upb_handlers_use_bytesrcs(bool use_bytesrcs);

// The closure will be passed to every handler.  The status will be read by the
// upb_src immediately after a handler has returned UPB_BREAK and used as the
// overall upb_src status; it will not be referenced at any other time.
INLINE void upb_set_handler_closure(upb_handlers *h, void *closure,
                                    upb_status *status);


/* upb_src ********************************************************************/

struct _upb_src;
typedef struct _upb_src upb_src;

// upb_src_sethandlers() must be called once and only once before upb_src_run()
// is called.  This sets up the callbacks that will handle the parse.  A
// upb_src that is fully initialized except for the call to
// upb_src_sethandlers() is called "prepared" -- this is useful for library
// functions that want to consume the output of a generic upb_src.
// Calling sethandlers() multiple times is an error and will trigger an abort().
INLINE void upb_src_sethandlers(upb_src *src, upb_handlers *handlers);

// Runs the src, calling the callbacks that were registered with
// upb_src_sethandlers(), and returning the status of the operation in
// "status." The status might indicate UPB_TRYAGAIN (indicating EAGAIN on a
// non-blocking socket) or a resumable error; in both cases upb_src_run can be
// called again later.  TRYAGAIN could come from either the src (input buffers
// are empty) or the handlers (output buffers are full).
INLINE void upb_src_run(upb_src *src, upb_status *status);


// A convenience object that a upb_src can use to invoke handlers.  It
// transparently handles delegation so that the upb_src needs only follow the
// protocol as if delegation did not exist.
struct _upb_dispatcher;
typedef struct _upb_dispatcher upb_dispatcher;
INLINE void upb_dispatcher_init(upb_dispatcher *d);
INLINE void upb_dispatcher_reset(upb_dispatcher *d, upb_handlers *h,
                                 bool supports_skip);
INLINE upb_flow_t upb_dispatch_startmsg(upb_dispatcher *d);
INLINE upb_flow_t upb_dispatch_endmsg(upb_dispatcher *d);
INLINE upb_flow_t upb_dispatch_startsubmsg(upb_dispatcher *d,
                                           struct _upb_fielddef *f);
INLINE upb_flow_t upb_dispatch_endsubmsg(upb_dispatcher *d,
                                         struct _upb_fielddef *f);
INLINE upb_flow_t upb_dispatch_value(upb_dispatcher *d, struct _upb_fielddef *f,
                                     upb_value val);
INLINE upb_flow_t upb_dispatch_unknownval(upb_dispatcher *d,
                                          upb_field_number_t fieldnum,
                                          upb_value val);

/* upb_bytesrc ****************************************************************/

// Reads up to "count" bytes into "buf", returning the total number of bytes
// read.  If 0, indicates error and puts details in "status".
INLINE upb_strlen_t upb_bytesrc_read(upb_bytesrc *src, void *buf,
                                     upb_strlen_t count, upb_status *status);

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
                               upb_status *status);

// A convenience function for getting all the remaining data in a upb_bytesrc
// as a upb_string.  Returns false and sets "status" if the operation fails.
INLINE bool upb_bytesrc_getfullstr(upb_bytesrc *src, upb_string *str,
                                   upb_status *status);
INLINE bool upb_value_getfullstr(upb_value val, upb_string *str,
                                 upb_status *status) {
  return upb_bytesrc_getfullstr(upb_value_getbytesrc(val), str, status);
}


/* upb_bytesink ***************************************************************/

struct _upb_bytesink;
typedef struct _upb_bytesink upb_bytesink;

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
                                        const char *fmt, ...);

// Puts the given string, returning true if the operation was successful, otherwise
// check "status" for details.  Ownership of the string is *not* passed; if
// the callee wants a reference he must call upb_string_getref() on it.
INLINE upb_strlen_t upb_bytesink_putstr(upb_bytesink *sink, upb_string *str,
                                        upb_status *status);

#include "upb_stream_vtbl.h"

#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif
