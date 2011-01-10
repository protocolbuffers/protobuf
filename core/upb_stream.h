/*
 * upb - a minimalist implementation of protocol buffers.
 *
 * This file defines four general-purpose streaming interfaces for protobuf
 * data or bytes:
 *
 * - upb_src: pull interface for protobuf data.
 * - upb_sink: push interface for protobuf data.
 * - upb_bytesrc: pull interface for bytes.
 * - upb_bytesink: push interface for bytes.
 *
 * These interfaces are used as general-purpose glue in upb.  For example, the
 * decoder interface works by implementing a upb_src and calling a upb_bytesrc.
 *
 * Copyright (c) 2010 Joshua Haberman.  See LICENSE for details.
 *
 */

#ifndef UPB_SRCSINK_H
#define UPB_SRCSINK_H

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
  UPB_CONTINUE,

  // Skips to the end of the current submessage (or if we are at the top
  // level, skips to the end of the entire message).
  UPB_SKIP,

  // Caller should stop sending values; check sink status for details.
  // If processing resumes later, it should resume with the next value.
  UPB_STOP,

  // When returned from a startsubmsg handler, indicates that the submessage
  // should be handled by a different set of handlers, which have been
  // registered on the provided upb_handlers object.  May not be returned
  // from any other callback.
  UPB_DELEGATE,
} upb_flow_t;

// upb_handlers
struct _upb_handlers;
typedef struct _upb_handlers upb_handlers;

typedef void (*upb_startmsg_handler_t)(void *closure);
typedef void (*upb_endmsg_handler_t)(void *closure);
typedef upb_flow_t (*upb_value_handler_t)(void *closure,
                                          struct _upb_fielddef *f,
                                          upb_value val);
typedef upb_flow_t (*upb_startsubmsg_handler_t)(void *closure,
                                                struct _upb_fielddef *f,
                                                upb_handlers *delegate_to);
typedef upb_flow_t (*upb_endsubmsg_handler_t)(void *closure);
typedef upb_flow_t (*upb_unknownval_handler_t)(void *closure,
                                               upb_field_number_t fieldnum,
                                               upb_value val);

// An empty set of handlers, for convenient copy/paste:
//
// static void startmsg(void *closure) {
//   // Called when the top-level message begins.
// }
//
// static void endmsg(void *closure) {
//   // Called when the top-level message ends.
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
// static upb_flow_t endsubmsg(void *closure) {
//   // Called when a submessage ends.
//   return UPB_CONTINUE;
// }
//
// static upb_flow_t unknownval(void *closure, upb_field_number_t fieldnum,
//                              upb_value val) {
//   Called with an unknown value is encountered.
//   return UPB_CONTINUE;
// }
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
INLINE void upb_set_handler_closure(upb_handlers *h, void *closure);

// An object that transparently handles delegation so that the caller needs
// only follow the protocol as if delegation did not exist.
struct _upb_dispatcher;
typedef struct _upb_dispatcher upb_dispatcher;
INLINE void upb_dispatcher_init(upb_dispatcher *d);
INLINE void upb_dispatcher_reset(upb_dispatcher *d, upb_handlers *h);
INLINE void upb_dispatch_startmsg(upb_dispatcher *d);
INLINE void upb_dispatch_endmsg(upb_dispatcher *d);
INLINE upb_flow_t upb_dispatch_startsubmsg(upb_dispatcher *d, struct _upb_fielddef *f);
INLINE upb_flow_t upb_dispatch_endsubmsg(upb_dispatcher *d);
INLINE upb_flow_t upb_dispatch_value(upb_dispatcher *d, struct _upb_fielddef *f,
                                     upb_value val);
INLINE upb_flow_t upb_dispatch_unknownval(upb_dispatcher *d,
                                   upb_field_number_t fieldnum, upb_value val);


/* upb_src ********************************************************************/

struct _upb_src;
typedef struct _upb_src upb_src;


/* upb_bytesrc ****************************************************************/

struct _upb_bytesrc;
typedef struct _upb_bytesrc upb_bytesrc;

// Returns the next string in the stream.  false is returned on error or eof.
// The string must be at least "minlen" bytes long unless the stream is eof.
INLINE bool upb_bytesrc_get(upb_bytesrc *src, upb_string *str, upb_strlen_t minlen);

// Appends the next "len" bytes in the stream in-place to "str".  This should
// be used when the caller needs to build a contiguous string of the existing
// data in "str" with more data.  The call fails if fewer than len bytes are
// available in the stream.
INLINE bool upb_bytesrc_append(upb_bytesrc *src, upb_string *str, upb_strlen_t len);

// Returns the current error status for the stream.
// Note!  The "eof" flag works like feof() in C; it cannot report end-of-file
// until a read has failed due to eof.  It cannot preemptively tell you that
// the next call will fail due to eof.  Since these are the semantics that C
// and UNIX provide, we're stuck with them if we want to support eg. stdio.
INLINE upb_status *upb_bytesrc_status(upb_bytesrc *src);
INLINE bool upb_bytesrc_eof(upb_bytesrc *src);

/* upb_bytesink ***************************************************************/

struct _upb_bytesink;
typedef struct _upb_bytesink upb_bytesink;

// Puts the given string.  Returns the number of bytes that were actually,
// consumed, which may be fewer than were in the string, or <0 on error.
INLINE int32_t upb_bytesink_put(upb_bytesink *sink, upb_string *str);

// Returns the current error status for the stream.
INLINE upb_status *upb_bytesink_status(upb_bytesink *sink);

#include "upb_stream_vtbl.h"

#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif
