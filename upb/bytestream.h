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

// Fetches at least one byte starting at ofs, returning the actual number of
// bytes fetched (or 0 on error: see "s" for details).  Gives caller a ref on
// the fetched region.  It is safe to re-fetch existing regions but only if
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

  // Length of the string.
  uint32_t len;

  // Offset in the bytesrc that represents the beginning of this string.
  uint32_t stream_offset;

  // Bytesrc from which this string data comes.  May be NULL if ptr is set.  If
  // non-NULL, the bytesrc is only guaranteed to be alive from inside the
  // callback; however if the handler knows more about its type and how to
  // prolong its life, it may do so.
  upb_bytesrc *bytesrc;

  // Possibly add optional members here like start_line, start_column, etc.
} upb_strref;

// Copies the contents of the strref into a newly-allocated, NULL-terminated
// string.
char *upb_strref_dup(struct _upb_strref *r);


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


/* upb_stdio ******************************************************************/

// bytesrc/bytesink for ANSI C stdio, which is less efficient than posixfd, but
// more portable.
//
// Specifically, stdio functions acquire locks on every operation (unless you
// use the f{read,write,...}_unlocked variants, which are not standard) and
// performs redundant buffering (unless you disable it with setvbuf(), but we
// can only do this on newly-opened filehandles).

typedef struct {
  uint64_t ofs;
  uint32_t refcount;
  char data[];
} upb_stdio_buf;

// We use a single object for both bytesrc and bytesink for simplicity.
// The object is still not thread-safe, and may only be used by one reader
// and one writer at a time.
typedef struct {
  upb_bytesrc src;
  upb_bytesink sink;
  FILE *file;
  bool should_close;
  upb_stdio_buf **bufs;
  uint32_t nbuf, szbuf;
} upb_stdio;

void upb_stdio_init(upb_stdio *stdio);
// Caller should call upb_stdio_flush prior to calling this to ensure that
// all data is flushed, otherwise data can be silently dropped if an error
// occurs flushing the remaining buffers.
void upb_stdio_uninit(upb_stdio *stdio);

// Resets the object to read/write to the given "file."  The caller is
// responsible for closing the file, which must outlive this object.
void upb_stdio_reset(upb_stdio *stdio, FILE *file);

// As an alternative to upb_stdio_reset(), initializes the object by opening a
// file, and will handle closing it.  This may result in more efficient I/O
// than the previous since we can call setvbuf() to disable buffering.
void upb_stdio_open(upb_stdio *stdio, const char *filename, const char *mode,
                    upb_status *s);

upb_bytesrc *upb_stdio_bytesrc(upb_stdio *stdio);
upb_bytesink *upb_stdio_bytesink(upb_stdio *stdio);


/* upb_stringsrc **************************************************************/

// bytesrc/bytesink for a simple contiguous string.

struct _upb_stringsrc {
  upb_bytesrc bytesrc;
  const char *str;
  size_t len;
};
typedef struct _upb_stringsrc upb_stringsrc;

// Create/free a stringsrc.
void upb_stringsrc_init(upb_stringsrc *s);
void upb_stringsrc_uninit(upb_stringsrc *s);

// Resets the stringsrc to a state where it will vend the given string.  The
// stringsrc will take a reference on the string, so the caller need not ensure
// that it outlives the stringsrc.  A stringsrc can be reset multiple times.
void upb_stringsrc_reset(upb_stringsrc *s, const char *str, size_t len);

// Returns the upb_bytesrc* for this stringsrc.
upb_bytesrc *upb_stringsrc_bytesrc(upb_stringsrc *s);


/* upb_stringsink *************************************************************/

struct _upb_stringsink {
  upb_bytesink bytesink;
  char *str;
  size_t len, size;
};
typedef struct _upb_stringsink upb_stringsink;

// Create/free a stringsrc.
void upb_stringsink_init(upb_stringsink *s);
void upb_stringsink_uninit(upb_stringsink *s);

// Resets the sink's string to "str", which the sink takes ownership of.
// "str" may be NULL, which will make the sink allocate a new string.
void upb_stringsink_reset(upb_stringsink *s, char *str, size_t size);

// Releases ownership of the returned string (which is "len" bytes long) and
// resets the internal string to be empty again (as if reset were called with
// NULL).
const char *upb_stringsink_release(upb_stringsink *s, size_t *len);

// Returns the upb_bytesink* for this stringsrc.  Invalidated by reset above.
upb_bytesink *upb_stringsink_bytesink(void);

#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif
