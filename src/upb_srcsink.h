/*
 * upb - a minimalist implementation of protocol buffers.
 *
 * Copyright (c) 2010 Joshua Haberman.  See LICENSE for details.
 *
 * This file defines four general-purpose interfaces for pulling/pushing either
 * protobuf data or bytes:
 *
 * - upb_src: pull interface for protobuf data.
 * - upb_sink: push interface for protobuf data.
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

// TODO: decide how to handle unknown fields.

struct upb_src;
typedef struct upb_src upb_src;

// Retrieves the fielddef for the next field in the stream.  Returns NULL on
// error or end-of-stream.
upb_fielddef *upb_src_getdef(upb_src *src);

// Retrieves and stores the next value in "val".  For string types the caller
// does not own a ref to the returned type; you must ref it yourself if you
// want one.  Returns false on error.
bool upb_src_getval(upb_src *src, upb_valueptr val);

// Like upb_src_getval() but skips the value.
bool upb_src_skipval(upb_src *src);

// Descends into a submessage.  May only be called after a def has been
// returned that indicates a submessage.
bool upb_src_startmsg(upb_src *src);

// Stops reading a submessage.  May be called before the stream is EOF, in
// which case the rest of the submessage is skipped.
bool upb_src_endmsg(upb_src *src);

// Returns the current error status for the stream.
upb_status *upb_src_status(upb_src *src);

/* upb_sink *******************************************************************/

struct upb_sink;
typedef struct upb_sink upb_sink;

// Puts the given fielddef into the stream.
bool upb_sink_putdef(upb_sink *sink, upb_fielddef *def);

// Puts the given value into the stream.
bool upb_sink_putval(upb_sink *sink, upb_value val);

// Starts a submessage.  (needed?  the def tells us we're starting a submsg.)
bool upb_sink_startmsg(upb_sink *sink);

// Ends a submessage.
bool upb_sink_endmsg(upb_sink *sink);

// Returns the current error status for the stream.
upb_status *upb_sink_status(upb_sink *sink);

/* upb_bytesrc ****************************************************************/

struct upb_bytesrc;
typedef struct upb_bytesrc upb_bytesrc;

// Returns the next string in the stream.  NULL is returned on error or eof.
// The string must be at least "minlen" bytes long unless the stream is eof.
//
// A ref is passed to the caller, though the caller is encouraged to pass the
// ref back to the bytesrc with upb_bytesrc_recycle().  This can help reduce
// memory allocation/deallocation.
upb_string *upb_bytesrc_get(upb_bytesrc *src, upb_strlen_t minlen);
void upb_bytesrc_recycle(upb_bytesrc *src, upb_string *str);

// Appends the next "len" bytes in the stream in-place to "str".  This should
// be used when the caller needs to build a contiguous string of the existing
// data in "str" with more data.
bool upb_bytesrc_append(upb_bytesrc *src, upb_string *str, upb_strlen_t len);

// Returns the current error status for the stream.
upb_status *upb_bytesrc_status(upb_src *src);

/* upb_bytesink ***************************************************************/

struct upb_bytesink;
typedef struct upb_bytesink upb_bytesink;

// Puts the given string.  Returns the number of bytes that were actually,
// consumed, which may be fewer than were in the string, or <0 on error.
int32_t upb_bytesink_put(upb_bytesink *sink, upb_string *str);

// Returns the current error status for the stream.
upb_status *upb_bytesink_status(upb_bytesink *sink);

/* Dynamic Dispatch implementation for src/sink interfaces ********************/

// The rest of this file only concerns components that are implementing any of
// the above interfaces.  To simple clients the code below should be considered
// private.

// Typedefs for function pointers to all of the above functions.
typedef upb_fielddef (*upb_src_getdef_fptr)(upb_src *src);
typedef bool (*upb_src_getval_fptr)(upb_src *src, upb_valueptr val);
typedef bool (*upb_src_skipval_fptr)(upb_src *src);
typedef bool (*upb_src_startmsg_fptr)(upb_src *src);
typedef bool (*upb_src_endmsg_fptr)(upb_src *src);
typedef upb_status *(*upb_src_status_fptr)(upb_src *src);

typedef bool (*upb_sink_putdef_fptr)(upb_sink *sink, upb_fielddef *def);
typedef bool (*upb_sink_putval_fptr)(upb_sink *sink, upb_value val);
typedef bool (*upb_sink_startmsg_fptr)(upb_sink *sink);
typedef bool (*upb_sink_endmsg_fptr)(upb_sink *sink);
typedef upb_status *(*upb_sink_status_fptr)(upb_sink *sink);

typedef upb_string *(*upb_bytesrc_get_fptr)(upb_bytesrc *src);
typedef bool (*upb_bytesrc_append_fptr)(
    upb_bytesrc *src, upb_string *str, upb_strlen_t len);
typedef upb_status *(*upb_bytesrc_status_fptr)(upb_src *src);

typedef int32_t (*upb_bytesink_put_fptr)(upb_bytesink *sink, upb_string *str);
typedef upb_status *(*upb_bytesink_status_fptr)(upb_bytesink *sink);

// Vtables for the above interfaces.
typedef struct {
  upb_src_getdef_fptr   getdef;
  upb_src_getval_fptr   getval;
  upb_src_skipval_fptr  skipval;
  upb_src_startmsg_fptr startmsg;
  upb_src_endmsg_fptr   endmsg;
  upb_src_status_fptr   status;
} upb_src_vtable;

// "Base Class" definitions; components that implement these interfaces should
// contain one of these structures.

struct upb_src {
  upb_src_vtable *vtbl;
  upb_status status;
  bool eof;
#ifndef NDEBUG
  int state;  // For debug-mode checking of API usage.
#endif
};

INLINE void upb_sink_init(upb_src *s, upb_src_vtable *vtbl) {
  s->vtbl = vtbl;
#ifndef DEBUG
  // TODO: initialize debug-mode checking.
#endif
}

#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif
