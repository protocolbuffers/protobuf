/*
 * upb - a minimalist implementation of protocol buffers.
 *
 * Copyright (c) 2010 Joshua Haberman.  See LICENSE for details.
 *
 * This file defines four general-purpose interfaces for pulling/pushing either
 * protobuf data or bytes:
 *
 * - upb_src: pull interface for protobuf key/value pairs.
 * - upb_sink: push interface for protobuf key/value pairs.
 * - upb_bytesrc: pull interface for bytes.
 * - upb_bytesink: push interface for bytes.
 *
 * These interfaces are used as general-purpose glue in upb.  For example, the
 * decoder interface works by implementing a upb_src and calling a upb_bytesrc.
 */

#ifndef UPB_SRCSINK_H
#define UPB_SRCSINK_H

#include "upb_def.h"

#ifdef __cplusplus
extern "C" {
#endif

/* upb_src ********************************************************************/

// Retrieves the fielddef for the next field in the stream.  Returns NULL on
// error or end-of-stream.
upb_fielddef *upb_src_getdef(upb_src *src);

// Retrieves and stores the next value in "val".  For string types the caller
// does not own a ref to the returned type; you must ref it yourself if you
// want one.  Returns false on error.
bool upb_src_getval(upb_src *src, upb_valueptr val);

// Like upb_src_getval() but skips the value.
bool upb_src_skipval(upb_src *src);

// Descends into a submessage.
bool upb_src_startmsg(upb_src *src);

// Stops reading a submessage.  May be called before the stream is EOF, in
// which case the rest of the submessage is skipped.
bool upb_src_endmsg(upb_src *src);

// Returns the current error status for the stream.
upb_status *upb_src_status(upb_src *src);

/* upb_bytesrc ****************************************************************/

// Returns the next string in the stream.  The caller does not own a ref on the
// returned string; you must ref it yourself if you want one.
upb_string *upb_bytesrc_get(upb_bytesrc *src);

// Appends the next "len" bytes in the stream in-place to "str".  This should
// be used when the caller needs to build a contiguous string of the existing
// data in "str" with more data.
bool upb_bytesrc_append(upb_bytesrc *src, upb_string *str, upb_strlen_t len);

// Returns the current error status for the stream.
upb_status *upb_bytesrc_status(upb_src *src);

/* upb_sink callbacks *********************************************************/

// The value callback is called for a regular value (ie. not a string or
// submessage).
typedef upb_sink_status (*upb_value_cb)(upb_sink *s, upb_fielddef *f,
                                        upb_value val, upb_status *status);

// The string callback is called for string data.  "str" is the string in which
// the data lives, but it may contain more data than the effective string.
// "start" and "end" indicate the substring of "str" that is the effective
// string.  If "start" is <0, this string is a continuation of the previous
// string for this field.  If end > upb_strlen(str) then there is more data to
// follow for this string.  "end" can also be used as a hint for how much data
// follows, but this is only a hint and is not guaranteed.
//
// The data is supplied this way to give you the opportunity to reference this
// data instead of copying it (perhaps using upb_strslice), or to minimize
// copying if it is unavoidable.
typedef upb_sink_status (*upb_str_cb)(upb_sink *s, upb_fielddef *f,
                                      upb_strptr str,
                                      int32_t start, uint32_t end,
                                      upb_status *status);

// The start and end callbacks are called when a submessage begins and ends,
// respectively.  The caller is responsible for ensuring that the nesting
// level never exceeds UPB_MAX_NESTING.
typedef upb_sink_status (*upb_start_cb)(upb_sink *s, upb_fielddef *f,
                                        upb_status *status);
typedef upb_sink_status (*upb_end_cb)(upb_sink *s, upb_fielddef *f,
                                      upb_status *status);


/* upb_sink implementation ****************************************************/

typedef struct upb_sink_callbacks {
  upb_value_cb value_cb;
  upb_str_cb str_cb;
  upb_start_cb start_cb;
  upb_end_cb end_cb;
} upb_sink_callbacks;

// These macros implement a mini virtual function dispatch for upb_sink instances.
// This allows functions that call upb_sinks to just write:
//
//   upb_sink_onvalue(sink, field, val);
//
// The macro will handle the virtual function lookup and dispatch.  We could
// potentially define these later to also be capable of calling a C++ virtual
// method instead of doing the virtual dispatch manually.  This would make it
// possible to write C++ sinks in a more natural style without loss of
// efficiency.  We could have a flag in upb_sink defining whether it is a C
// sink or a C++ one.
#define upb_sink_onvalue(s, f, val, status) s->vtbl->value_cb(s, f, val, status)
#define upb_sink_onstr(s, f, str, start, end, status) s->vtbl->str_cb(s, f, str, start, end, status)
#define upb_sink_onstart(s, f, status) s->vtbl->start_cb(s, f, status)
#define upb_sink_onend(s, f, status) s->vtbl->end_cb(s, f, status)

// Initializes a plain C visitor with the given vtbl.  The sink must have been
// allocated separately.
INLINE void upb_sink_init(upb_sink *s, upb_sink_callbacks *vtbl) {
  s->vtbl = vtbl;
}


/* upb_bytesink ***************************************************************/

// A upb_bytesink is like a upb_sync, but for bytes instead of structured
// protobuf data.  Parsers implement upb_bytesink and push to a upb_sink,
// serializers do the opposite (implement upb_sink and push to upb_bytesink).
//
// The two simplest kinds of sinks are "write to string" and "write to FILE*".

// A forward declaration solely for the benefit of declaring upb_byte_cb below.
// Always prefer upb_bytesink (without the "struct" keyword) instead.
struct _upb_bytesink;

// The single bytesink callback; it takes the bytes to be written and returns
// how many were successfully written.  If the return value is <0, the caller
// should stop processing.
typedef int32_t (*upb_byte_cb)(struct _upb_bytesink *s, upb_strptr str,
                               uint32_t start, uint32_t end,
                               upb_status *status);

typedef struct _upb_bytesink {
  upb_byte_cb *cb;
} upb_bytesink;

#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif
