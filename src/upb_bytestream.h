/*
 * upb - a minimalist implementation of protocol buffers.
 *
 * Copyright (c) 2010-2011 Google Inc.  See LICENSE for details.
 * Author: Josh Haberman <jhaberman@gmail.com>
 *
 * Defines the interfaces upb_bytesrc and upb_bytesink, which are abstractions
 * of read()/write() with useful buffering/sharing semantics.
 */

#ifndef UPB_BYTESTREAM_H
#define UPB_BYTESTREAM_H

#include <stdarg.h>
#include "upb.h"

#ifdef __cplusplus
extern "C" {
#endif

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
