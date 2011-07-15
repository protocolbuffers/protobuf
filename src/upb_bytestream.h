/*
 * upb - a minimalist implementation of protocol buffers.
 *
 * Copyright (c) 2011 Google Inc.  See LICENSE for details.
 * Author: Josh Haberman <jhaberman@gmail.com>
 *
 * This file contains upb_bytesrc and upb_bytesink, which are abstractions of
 * stdio (fread()/fwrite()/etc) that provide useful buffering/sharing
 * semantics.  They are virtual base classes so concrete implementations
 * can get the data from a fd, a string, a cord, etc.
 *
 * Byte streams are NOT thread-safe!  (Like f{read,write}_unlocked())
 */

#ifndef UPB_BYTESTREAM_H
#define UPB_BYTESTREAM_H

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include "upb.h"

#ifdef __cplusplus
extern "C" {
#endif


/* upb_bytesrc ****************************************************************/

// A upb_bytesrc allows the consumer of a stream of bytes to obtain buffers as
// they become available, and to preserve some trailing amount of data.
typedef size_t upb_bytesrc_fetch_func(void*, uint64_t, upb_status*);
typedef void upb_bytesrc_read_func(void*, uint64_t, size_t, char*);
typedef const char *upb_bytesrc_getptr_func(void*, uint64_t, size_t*);
typedef void upb_bytesrc_refregion_func(void*, uint64_t, size_t);
typedef void upb_bytesrc_ref_func(void*);
typedef struct _upb_bytesrc_vtbl {
  upb_bytesrc_fetch_func     *fetch;
  upb_bytesrc_read_func      *read;
  upb_bytesrc_getptr_func    *getptr;
  upb_bytesrc_refregion_func *refregion;
  upb_bytesrc_refregion_func *unrefregion;
  upb_bytesrc_ref_func       *ref;
  upb_bytesrc_ref_func       *unref;
} upb_bytesrc_vtbl;

typedef struct {
  upb_bytesrc_vtbl  *vtbl;
} upb_bytesrc;

INLINE void upb_bytesrc_init(upb_bytesrc *src, upb_bytesrc_vtbl *vtbl) {
  src->vtbl = vtbl;
}

// Fetches at least minlen bytes starting at ofs, returning the actual number
// of bytes fetched (or 0 on error: see "s" for details).  Gives caller a ref
// on the fetched region.  It is safe to re-fetch existing regions but only if
// they are ref'd.  "ofs" may not greater than the end of the region that was
// previously fetched.
INLINE size_t upb_bytesrc_fetch(upb_bytesrc *src, uint64_t ofs, upb_status *s) {
  return src->vtbl->fetch(src, ofs, s);
}

// Copies "len" bytes of data from offset src_ofs to "dst", which must be at
// least "len" bytes long.  The caller must own a ref on the given region.
INLINE void upb_bytesrc_read(upb_bytesrc *src, uint64_t src_ofs, size_t len,
                             char *dst) {
  src->vtbl->read(src, src_ofs, len, dst);
}

// Returns a pointer to the bytesrc's internal buffer, returning how much data
// was actually returned (which may be less than "len" if the given region is
// not contiguous).  The caller must own refs on the entire region from [ofs,
// ofs+len].  The returned buffer is valid for as long as the region remains
// ref'd.
//
// TODO: is "len" really required here?
INLINE const char *upb_bytesrc_getptr(upb_bytesrc *src, uint64_t ofs,
                                      size_t *len) {
  return src->vtbl->getptr(src, ofs, len);
}

// Gives the caller a ref on the given region.  The caller must know that the
// given region is already ref'd.
INLINE void upb_bytesrc_refregion(upb_bytesrc *src, uint64_t ofs, size_t len) {
  src->vtbl->refregion(src, ofs, len);
}

// Releases a ref on the given region, which the caller must have previously
// ref'd.
INLINE void upb_bytesrc_unrefregion(upb_bytesrc *src, uint64_t ofs, size_t len) {
  src->vtbl->unrefregion(src, ofs, len);
}

// Attempts to ref the bytesrc itself, returning false if this bytesrc is
// not ref-able.
INLINE bool upb_bytesrc_tryref(upb_bytesrc *src) {
  if (src->vtbl->ref) {
    src->vtbl->ref(src);
    return true;
  } else {
    return false;
  }
}

// Unref's the bytesrc itself.  May only be called when upb_bytesrc_tryref()
// has previously returned true.
INLINE void upb_bytesrc_unref(upb_bytesrc *src) {
  assert(src->vtbl->unref);
  src->vtbl->unref(src);
}

/* upb_strref *****************************************************************/

// The structure we pass for a string.
typedef struct _upb_strref {
  // Pointer to the string data.  NULL if the string spans multiple input
  // buffers (in which case upb_bytesrc_getptr() must be called to obtain
  // the actual pointers).
  const char *ptr;

  // Bytesrc from which this string data comes.  This is only guaranteed to be
  // alive from inside the callback; however if the handler knows more about
  // its type and how to prolong its life, it may do so.
  upb_bytesrc *bytesrc;

  // Offset in the bytesrc that represents the beginning of this string.
  uint32_t stream_offset;

  // Length of the string.
  uint32_t len;

  // Possibly add optional members here like start_line, start_column, etc.
} upb_strref;

// Copies the contents of the strref into a newly-allocated, NULL-terminated
// string.
INLINE char *upb_strref_dup(struct _upb_strref *r) {
  char *ret = (char*)malloc(r->len + 1);
  upb_bytesrc_read(r->bytesrc, r->stream_offset, r->len, ret);
  ret[r->len] = '\0';
  return ret;
}


/* upb_bytesink ***************************************************************/

typedef bool upb_bytesink_write_func(void*, const char*, size_t, upb_status*);
typedef int32_t upb_bytesink_vprintf_func(
    void*, upb_status*, const char *fmt, va_list args);

typedef struct {
  upb_bytesink_write_func   *write;
  upb_bytesink_vprintf_func *vprintf;
} upb_bytesink_vtbl;

typedef struct {
  upb_bytesink_vtbl *vtbl;
} upb_bytesink;

INLINE void upb_bytesink_init(upb_bytesink *sink, upb_bytesink_vtbl *vtbl) {
  sink->vtbl = vtbl;
}

INLINE bool upb_bytesink_write(upb_bytesink *sink, const char *buf, size_t len,
                               upb_status *s) {
  return sink->vtbl->write(sink, buf, len, s);
}

INLINE bool upb_bytesink_writestr(upb_bytesink *sink, const char *str,
                                  upb_status *s) {
  return upb_bytesink_write(sink, str, strlen(str), s);
}

// Returns the number of bytes written or -1 on error.
INLINE int32_t upb_bytesink_printf(upb_bytesink *sink, upb_status *status,
                                   const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  uint32_t ret = sink->vtbl->vprintf(sink, status, fmt, args);
  va_end(args);
  return ret;
}

// OPT: add getappendbuf()
// OPT: add writefrombytesrc()
// TODO: add flush()


/* upb_cbuf *******************************************************************/

// A circular buffer implementation for bytesrcs that do internal buffering.

#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif
