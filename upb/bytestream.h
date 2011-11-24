/*
 * upb - a minimalist implementation of protocol buffers.
 *
 * Copyright (c) 2011 Google Inc.  See LICENSE for details.
 * Author: Josh Haberman <jhaberman@gmail.com>
 *
 * This file defines three core interfaces:
 * - upb_bytesink: for writing streams of data.
 * - upb_bytesrc: for reading streams of data.
 * - upb_byteregion: for reading from a specific region of a upb_bytesrc;
 *   should be used by decoders instead of using upb_bytesrc directly.
 *
 * These interfaces are used by streaming encoders and decoders: for example, a
 * protobuf parser gets its input from a upb_byteregion.  They are virtual base
 * classes so concrete implementations can get the data from a fd, a FILE*, a
 * string, etc.
 */

// A upb_byteregion represents a region of data from a bytesrc.
//
// Parsers get data from this interface instead of a bytesrc because we often
// want to parse only a specific region of the input.  For example, if we parse
// a string from our input but know that the string represents a protobuf, we
// can pass its upb_byteregion to an appropriate protobuf parser.
//
// Since the bytes may be coming from a file or network socket, bytes must be
// fetched before they can be read (though in some cases this fetch may be a
// no-op).  "fetch" is the only operation on a byteregion that could fail or
// block, because it is the only operation that actually performs I/O.
//
// Bytes can be discarded when they are no longer needed.  Parsers should
// always discard bytes they no longer need, both so the buffers can be freed
// when possible and to give better visibility into what bytes the parser is
// still using.
//
// start      discard                     read             fetch             end
// ofs          ofs                       ofs               ofs              ofs
// |             |--->discard()            |                 |--->fetch()      |
// V             V                         V                 V                 V
// +-------------+-------------------------+-----------------+-----------------+
// |  discarded  |                         |                 |    fetchable    |
// +-------------+-------------------------+-----------------+-----------------+
//               | <------------- loaded ------------------> |
//                                         | <- available -> |
//                                         | <---------- remaining ----------> |
//
// Note that the start offset may be something other than zero!  A byteregion
// is a view into an underlying bytesrc stream, and the region may start
// somewhere other than the beginning of that stream.
//
// The region can be either delimited or nondelimited.  A non-delimited region
// will keep returning data until the underlying data source returns EOF.  A
// delimited region will return EOF at a predetermined offset.
//
//                       end
//                       ofs
//                         |
//                         V
// +-----------------------+
// |  delimited region     |   <-- hard EOF, even if data source has more data.
// +-----------------------+
//
// +------------------------
// | nondelimited region   Z   <-- won't return EOF until data source hits EOF.
// +------------------------


#ifndef UPB_BYTESTREAM_H
#define UPB_BYTESTREAM_H

#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "upb.h"

#ifdef __cplusplus
extern "C" {
#endif


/* upb_bytesrc ****************************************************************/

// A upb_bytesrc allows the consumer of a stream of bytes to obtain buffers as
// they become available, and to preserve some trailing amount of data before
// it is discarded.  Consumers should not use upb_bytesrc directly, but rather
// should use a upb_byteregion (which allows access to a region of a bytesrc).
//
// upb_bytesrc is a virtual base class with implementations that get data from
// eg. a string, a cord, a file descriptor, a FILE*, etc.

typedef uint32_t upb_bytesrc_fetch_func(void*, uint64_t, upb_status*);
typedef void upb_bytesrc_discard_func(void*, uint64_t);
typedef void upb_bytesrc_copy_func(const void*, uint64_t, uint32_t, char*);
typedef const char *upb_bytesrc_getptr_func(const void*, uint64_t, uint32_t*);
typedef struct _upb_bytesrc_vtbl {
  upb_bytesrc_fetch_func     *fetch;
  upb_bytesrc_discard_func   *discard;
  upb_bytesrc_copy_func      *copy;
  upb_bytesrc_getptr_func    *getptr;
} upb_bytesrc_vtbl;

typedef struct {
  upb_bytesrc_vtbl  *vtbl;
} upb_bytesrc;

INLINE void upb_bytesrc_init(upb_bytesrc *src, upb_bytesrc_vtbl *vtbl) {
  src->vtbl = vtbl;
}

// Fetches at least one byte starting at ofs, returning the actual number of
// bytes fetched (or 0 on EOF or error: see *s for details).  Some bytesrc's
// may set EOF on *s after a successful read if no further data is available,
// but not all bytesrc's support this.  It is valid for bytes to be fetched
// multiple times, as long as the bytes have not been previously discarded.
INLINE uint32_t upb_bytesrc_fetch(upb_bytesrc *src, uint64_t ofs,
                                  upb_status *s) {
  return src->vtbl->fetch(src, ofs, s);
}

// Discards all data prior to ofs (except data that is pinned, if pinning
// support is added -- see TODO below).
INLINE void upb_bytesrc_discard(upb_bytesrc *src, uint64_t ofs) {
  src->vtbl->discard(src, ofs);
}

// Copies "len" bytes of data from ofs to "dst", which must be at least "len"
// bytes long.  The given region must not be discarded.
INLINE void upb_bytesrc_copy(const upb_bytesrc *src, uint64_t ofs, uint32_t len,
                             char *dst) {
  src->vtbl->copy(src, ofs, len, dst);
}

// Returns a pointer to the bytesrc's internal buffer, storing in *len how much
// data is available.  The given offset must not be discarded.  The returned
// buffer is valid for as long as its bytes are not discarded (in the case that
// part of the returned buffer is discarded, only the non-discarded bytes
// remain valid).
INLINE const char *upb_bytesrc_getptr(const upb_bytesrc *src, uint64_t ofs,
                                      uint32_t *len) {
  return src->vtbl->getptr(src, ofs, len);
}

// TODO: Add if/when there is a demonstrated need:
//
// // When the caller pins a region (which must not be already discarded), it
// // is guaranteed that the region will not be discarded (nor will the bytesrc
// // be destroyed) until the region is unpinned.  However, not all bytesrc's
// // support pinning; a false return indicates that a pin was not possible.
// INLINE bool upb_bytesrc_pin(upb_bytesrc *src, uint64_t ofs, uint32_t len) {
//   return src->vtbl->refregion(src, ofs, len);
// }
//
// // Releases some number of pinned bytes from the beginning of a pinned
// // region (which may be fewer than the total number of bytes pinned).
// INLINE void upb_bytesrc_unpin(upb_bytesrc *src, uint64_t ofs, uint32_t len,
//                               uint32_t bytes_to_release) {
//   src->vtbl->unpin(src, ofs, len);
// }
//
// Adding pinning support would also involve adding a "pin_ofs" parameter to
// upb_bytesrc_fetch, so that the fetch can extend an already-pinned region.


/* upb_byteregion *************************************************************/

#define UPB_NONDELIMITED (0xffffffffffffffffULL)

typedef struct _upb_byteregion {
  uint64_t start;
  uint64_t discard;
  uint64_t fetch;
  uint64_t end;         // UPB_NONDELIMITED if nondelimited.
  upb_bytesrc *bytesrc;
  bool toplevel;        // If true, discards hit the underlying byteregion.
} upb_byteregion;

// Initializes a byteregion.  Its initial value will be empty.  No methods may
// be called on an empty byteregion except upb_byteregion_reset().
void upb_byteregion_init(upb_byteregion *r);
void upb_byteregion_uninit(upb_byteregion *r);

// Accessors for the regions bounds -- the meaning of these is described in the
// diagram above.
INLINE uint64_t upb_byteregion_startofs(const upb_byteregion *r) {
  return r->start;
}
INLINE uint64_t upb_byteregion_discardofs(const upb_byteregion *r) {
  return r->discard;
}
INLINE uint64_t upb_byteregion_fetchofs(const upb_byteregion *r) {
  return r->fetch;
}
INLINE uint64_t upb_byteregion_endofs(const upb_byteregion *r) {
  return r->end;
}

// Returns how many bytes are fetched and available for reading starting
// from offset "o".
INLINE uint64_t upb_byteregion_available(const upb_byteregion *r, uint64_t o) {
  assert(o >= upb_byteregion_discardofs(r));
  assert(o <= r->fetch);  // Could relax this.
  return r->fetch - o;
}

// Returns the total number of bytes remaining after offset "o", or
// UPB_NONDELIMITED if the byteregion is non-delimited.
INLINE uint64_t upb_byteregion_remaining(const upb_byteregion *r, uint64_t o) {
  return r->end == UPB_NONDELIMITED ? UPB_NONDELIMITED : r->end - o;
}

INLINE uint64_t upb_byteregion_len(const upb_byteregion *r) {
  return upb_byteregion_remaining(r, r->start);
}

// Sets the value of this byteregion to be a subset of the given byteregion's
// data.  The caller is responsible for releasing this region before the src
// region is released (unless the region is first pinned, if pinning support is
// added.  see below).
void upb_byteregion_reset(upb_byteregion *r, const upb_byteregion *src,
                          uint64_t ofs, uint64_t len);
void upb_byteregion_release(upb_byteregion *r);

// Attempts to fetch more data, extending the fetched range of this byteregion.
// Returns true if the fetched region was extended by at least one byte, false
// on EOF or error (see *s for details).
bool upb_byteregion_fetch(upb_byteregion *r, upb_status *s);

// Fetches all remaining data for "r", returning false if the operation failed
// (see "*s" for details).  May only be used on delimited byteregions.
INLINE bool upb_byteregion_fetchall(upb_byteregion *r, upb_status *s) {
  assert(upb_byteregion_len(r) != UPB_NONDELIMITED);
  while (upb_byteregion_fetch(r, s)) ;  // Empty body.
  return upb_eof(s);
}

// Discards bytes from the byteregion up until ofs (which must be greater or
// equal to upb_byteregion_discardofs()).  It is valid to discard bytes that
// have not been fetched (such bytes will never be fetched) but it is an error
// to discard past the end of a delimited byteregion.
INLINE void upb_byteregion_discard(upb_byteregion *r, uint64_t ofs) {
  assert(ofs >= upb_byteregion_discardofs(r));
  assert(ofs <= upb_byteregion_endofs(r));
  r->discard = ofs;
  if (r->toplevel) upb_bytesrc_discard(r->bytesrc, ofs);
}

// Copies "len" bytes of data into "dst", starting at ofs.  The specified
// region must be available.
INLINE void upb_byteregion_copy(const upb_byteregion *r, uint64_t ofs,
                                uint32_t len, char *dst) {
  assert(ofs >= upb_byteregion_discardofs(r));
  assert(len <= upb_byteregion_available(r, ofs));
  upb_bytesrc_copy(r->bytesrc, ofs, len, dst);
}

// Copies all bytes from the byteregion into dst.  Requires that the entire
// byteregion is fetched and that none has been discarded.
INLINE void upb_byteregion_copyall(const upb_byteregion *r, char *dst) {
  assert(r->start == r->discard && r->end == r->fetch);
  upb_byteregion_copy(r, r->start, upb_byteregion_len(r), dst);
}

// Returns a pointer to the internal buffer for the byteregion starting at
// offset "ofs." Stores the number of bytes available in this buffer in *len.
// The returned buffer is invalidated when the byteregion is reset or released,
// or when the bytes are discarded.  If the byteregion is not currently pinned,
// the pointer is only valid for the lifetime of the parent byteregion.
INLINE const char *upb_byteregion_getptr(const upb_byteregion *r,
                                         uint64_t ofs, uint32_t *len) {
  assert(ofs >= upb_byteregion_discardofs(r));
  const char *ret = upb_bytesrc_getptr(r->bytesrc, ofs, len);
  *len = UPB_MIN(*len, upb_byteregion_available(r, ofs));
  return ret;
}

// TODO: add if/when there is a demonstrated need.
//
// // Pins this byteregion's bytes in memory, allowing it to outlive its parent
// // byteregion.  Normally a byteregion may only be used while its parent is
// // still valid, but a pinned byteregion may continue to be used until it is
// // reset or released.  A byteregion must be fully fetched to be pinned
// // (this implies that the byteregion must be delimited).
// //
// // In some cases this operation may cause the input data to be copied.
// //
// // void upb_byteregion_pin(upb_byteregion *r);

// Convenience functions for creating and destroying a byteregion with a simple
// string as its data.  These are relatively inefficient compared with creating
// your own bytesrc (they call malloc() and copy the string data) so should not
// be used on any critical path.
//
// The string data in the returned region is guaranteed to be contiguous and
// NULL-terminated.
upb_byteregion *upb_byteregion_new(const void *str);
upb_byteregion *upb_byteregion_newl(const void *str, uint32_t len);
// May *only* be called on a byteregion created with upb_byteregion_new[l]()!
void upb_byteregion_free(upb_byteregion *r);

// Copies the contents of the byteregion into a newly-allocated, NULL-terminated
// string.  Requires that the byteregion is fully fetched.
char *upb_byteregion_strdup(const upb_byteregion *r);


/* upb_bytesink ***************************************************************/

// A bytesink is an interface that allows the caller to push byte-wise data.
// It is very simple -- the only special capability is the ability to "rewind"
// the stream, which is really only a mechanism of having the bytesink ignore
// some subsequent calls.
typedef int upb_bytesink_write_func(void*, const void*, int);
typedef int upb_bytesink_vprintf_func(void*, const char *fmt, va_list args);

typedef struct {
  upb_bytesink_write_func   *write;
  upb_bytesink_vprintf_func *vprintf;
} upb_bytesink_vtbl;

typedef struct {
  upb_bytesink_vtbl *vtbl;
  upb_status status;
  uint64_t offset;
} upb_bytesink;

// Should be called by derived classes.
void upb_bytesink_init(upb_bytesink *sink, upb_bytesink_vtbl *vtbl);
void upb_bytesink_uninit(upb_bytesink *sink);

INLINE int upb_bytesink_write(upb_bytesink *s, const void *buf, int len) {
  return s->vtbl->write(s, buf, len);
}

INLINE int upb_bytesink_writestr(upb_bytesink *sink, const char *str) {
  return upb_bytesink_write(sink, str, strlen(str));
}

// Returns the number of bytes written or -1 on error.
INLINE int upb_bytesink_printf(upb_bytesink *sink, const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  uint32_t ret = sink->vtbl->vprintf(sink, fmt, args);
  va_end(args);
  return ret;
}

INLINE int upb_bytesink_putc(upb_bytesink *sink, char ch) {
  return upb_bytesink_write(sink, &ch, 1);
}

INLINE int upb_bytesink_putrepeated(upb_bytesink *sink, char ch, int len) {
  for (int i = 0; i < len; i++)
    if (upb_bytesink_write(sink, &ch, 1) < 0)
      return -1;
  return len;
}

INLINE uint64_t upb_bytesink_getoffset(upb_bytesink *sink) {
  return sink->offset;
}

// Rewinds the stream to the given offset.  This cannot actually "unput" any
// data, it is for situations like:
//
// // If false is returned (because of error), call again later to resume.
// bool write_some_data(upb_bytesink *sink, int indent) {
//   uint64_t start_offset = upb_bytesink_getoffset(sink);
//   if (upb_bytesink_writestr(sink, "Some data") < 0) goto err;
//   if (upb_bytesink_putrepeated(sink, ' ', indent) < 0) goto err;
//   return true;
//  err:
//   upb_bytesink_rewind(sink, start_offset);
//   return false;
// }
//
// The subsequent bytesink writes *must* be identical to the writes that were
// rewinded past.
INLINE void upb_bytesink_rewind(upb_bytesink *sink, uint64_t offset) {
  // TODO
  (void)sink;
  (void)offset;
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
  uint32_t len;
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
  upb_byteregion byteregion;
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

upb_byteregion *upb_stdio_allbytes(upb_stdio *stdio);
upb_bytesink *upb_stdio_bytesink(upb_stdio *stdio);


/* upb_stringsrc **************************************************************/

// bytesrc/bytesink for a simple contiguous string.

typedef struct {
  upb_bytesrc bytesrc;
  const char *str;
  uint32_t len;
  upb_byteregion byteregion;
} upb_stringsrc;

// Create/free a stringsrc.
void upb_stringsrc_init(upb_stringsrc *s);
void upb_stringsrc_uninit(upb_stringsrc *s);

// Resets the stringsrc to a state where it will vend the given string.  The
// string data must be valid until the stringsrc is reset again or destroyed.
void upb_stringsrc_reset(upb_stringsrc *s, const char *str, uint32_t len);

// Returns the top-level upb_byteregion* for this stringsrc.  Invalidated when
// the stringsrc is reset.
INLINE upb_byteregion *upb_stringsrc_allbytes(upb_stringsrc *s) {
  return &s->byteregion;
}


/* upb_stringsink *************************************************************/

struct _upb_stringsink {
  upb_bytesink bytesink;
  char *str;
  uint32_t len, size;
};
typedef struct _upb_stringsink upb_stringsink;

// Create/free a stringsrc.
void upb_stringsink_init(upb_stringsink *s);
void upb_stringsink_uninit(upb_stringsink *s);

// Resets the sink's string to "str", which the sink takes ownership of.
// "str" may be NULL, which will make the sink allocate a new string.
void upb_stringsink_reset(upb_stringsink *s, char *str, uint32_t len);

// Releases ownership of the returned string (which is "len" bytes long) and
// resets the internal string to be empty again (as if reset were called with
// NULL).
const char *upb_stringsink_release(upb_stringsink *s, uint32_t *len);

// Returns the upb_bytesink* for this stringsrc.  Invalidated by reset above.
upb_bytesink *upb_stringsink_bytesink(upb_stringsink *s);

#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif
