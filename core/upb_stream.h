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

#include "upb_stream_vtbl.h"

#ifdef __cplusplus
extern "C" {
#endif

// Forward-declare.  We can't include upb_def.h; it would be circular.
struct _upb_fielddef;

/* upb_sink *******************************************************************/

// A upb_sink is a component that receives a stream of protobuf data.
// It is an abstract interface that is implemented either by the system or
// by users.
//
// TODO: unknown fields.

// Constants that a sink returns to indicate to its caller whether it should
// continue or not.
typedef enum {
  // Caller should continue sending values to the sink.
  UPB_SINK_CONTINUE,

  // Return from upb_sink_putdef() to skip the next value (which may be a
  // submessage).
  UPB_SINK_SKIP,

  // Caller should stop sending values; check sink status for details.
  // If processing resumes later, it should resume with the next value.
  UPB_SINK_STOP,
} upb_sinkret_t;

// Puts the given fielddef into the stream.
upb_sinkret_t upb_sink_putdef(upb_sink *sink, struct _upb_fielddef *def);

// Puts the given value into the stream.
upb_sinkret_t upb_sink_putval(upb_sink *sink, upb_value val);
upb_sinkret_t upb_sink_putstr(upb_sink *sink, upb_string *str);

// Starts/ends a submessage.  upb_sink_startmsg may seem redundant, but a
// client could have a submessage already serialized, and therefore put it
// as a string instead of its individual elements.
upb_sinkret_t upb_sink_startmsg(upb_sink *sink);
upb_sinkret_t upb_sink_endmsg(upb_sink *sink);

// Returns the current error status for the stream.
upb_status *upb_sink_status(upb_sink *sink);


/* upb_src ********************************************************************/

// A upb_src is a resumable push parser for protobuf data.  It works by first
// accepting registration of a upb_sink to which it will push data, then
// in a second phase is parses the actual data.
//

// Sets the given sink as the target of this src.  It will be called when the
// upb_src_parse() is run.
void upb_src_setsink(upb_src *src, upb_sink *sink);

// Pushes data from this src to the previously registered sink, returning
// true if all data was processed.  If false is returned, check
// upb_src_status() for details; if it is a resumable status, upb_src_run
// may be called again to resume processing.
bool upb_src_run(upb_src *src);


/* upb_bytesrc ****************************************************************/

// Returns the next string in the stream.  false is returned on error or eof.
// The string must be at least "minlen" bytes long unless the stream is eof.
bool upb_bytesrc_get(upb_bytesrc *src, upb_string *str, upb_strlen_t minlen);

// Appends the next "len" bytes in the stream in-place to "str".  This should
// be used when the caller needs to build a contiguous string of the existing
// data in "str" with more data.  The call fails if fewer than len bytes are
// available in the stream.
bool upb_bytesrc_append(upb_bytesrc *src, upb_string *str, upb_strlen_t len);

// Returns the current error status for the stream.
// Note!  The "eof" flag works like feof() in C; it cannot report end-of-file
// until a read has failed due to eof.  It cannot preemptively tell you that
// the next call will fail due to eof.  Since these are the semantics that C
// and UNIX provide, we're stuck with them if we want to support eg. stdio.
INLINE upb_status *upb_bytesrc_status(upb_bytesrc *src) { return &src->status; }
INLINE bool upb_bytesrc_eof(upb_bytesrc *src) { return src->eof; }

/* upb_bytesink ***************************************************************/

// Puts the given string.  Returns the number of bytes that were actually,
// consumed, which may be fewer than were in the string, or <0 on error.
int32_t upb_bytesink_put(upb_bytesink *sink, upb_string *str);

// Returns the current error status for the stream.
upb_status *upb_bytesink_status(upb_bytesink *sink);

/* Utility functions **********************************************************/

// Streams data from src to sink until EOF or error.
void upb_streamdata(upb_src *src, upb_sink *sink, upb_status *status);


#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif
