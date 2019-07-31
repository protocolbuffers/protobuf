/*
** upb::Encoder
**
** Since we are implementing pure handlers (ie. without any out-of-band access
** to pre-computed lengths), we have to buffer all submessages before we can
** emit even their first byte.
**
** Not knowing the size of submessages also means we can't write a perfect
** zero-copy implementation, even with buffering.  Lengths are stored as
** varints, which means that we don't know how many bytes to reserve for the
** length until we know what the length is.
**
** This leaves us with three main choices:
**
** 1. buffer all submessage data in a temporary buffer, then copy it exactly
**    once into the output buffer.
**
** 2. attempt to buffer data directly into the output buffer, estimating how
**    many bytes each length will take.  When our guesses are wrong, use
**    memmove() to grow or shrink the allotted space.
**
** 3. buffer directly into the output buffer, allocating a max length
**    ahead-of-time for each submessage length.  If we overallocated, we waste
**    space, but no memcpy() or memmove() is required.  This approach requires
**    defining a maximum size for submessages and rejecting submessages that
**    exceed that size.
**
** (2) and (3) have the potential to have better performance, but they are more
** complicated and subtle to implement:
**
**   (3) requires making an arbitrary choice of the maximum message size; it
**       wastes space when submessages are shorter than this and fails
**       completely when they are longer.  This makes it more finicky and
**       requires configuration based on the input.  It also makes it impossible
**       to perfectly match the output of reference encoders that always use the
**       optimal amount of space for each length.
**
**   (2) requires guessing the the size upfront, and if multiple lengths are
**       guessed wrong the minimum required number of memmove() operations may
**       be complicated to compute correctly.  Implemented properly, it may have
**       a useful amortized or average cost, but more investigation is required
**       to determine this and what the optimal algorithm is to achieve it.
**
**   (1) makes you always pay for exactly one copy, but its implementation is
**       the simplest and its performance is predictable.
**
** So for now, we implement (1) only.  If we wish to optimize later, we should
** be able to do it without affecting users.
**
** The strategy is to buffer the segments of data that do *not* depend on
** unknown lengths in one buffer, and keep a separate buffer of segment pointers
** and lengths.  When the top-level submessage ends, we can go beginning to end,
** alternating the writing of lengths with memcpy() of the rest of the data.
** At the top level though, no buffering is required.
*/

#include "upb/pb/encoder.h"
#include "upb/pb/varint.int.h"

#include "upb/port_def.inc"

/* The output buffer is divided into segments; a segment is a string of data
 * that is "ready to go" -- it does not need any varint lengths inserted into
 * the middle.  The seams between segments are where varints will be inserted
 * once they are known.
 *
 * We also use the concept of a "run", which is a range of encoded bytes that
 * occur at a single submessage level.  Every segment contains one or more runs.
 *
 * A segment can span messages.  Consider:
 *
 *                  .--Submessage lengths---------.
 *                  |       |                     |
 *                  |       V                     V
 *                  V      | |---------------    | |-----------------
 * Submessages:    | |-----------------------------------------------
 * Top-level msg: ------------------------------------------------------------
 *
 * Segments:          -----   -------------------   -----------------
 * Runs:              *----   *--------------*---   *----------------
 * (* marks the start)
 *
 * Note that the top-level menssage is not in any segment because it does not
 * have any length preceding it.
 *
 * A segment is only interrupted when another length needs to be inserted.  So
 * observe how the second segment spans both the inner submessage and part of
 * the next enclosing message. */
typedef struct {
  uint32_t msglen;  /* The length to varint-encode before this segment. */
  uint32_t seglen;  /* Length of the segment. */
} upb_pb_encoder_segment;

struct upb_pb_encoder {
  upb_arena *arena;

  /* Our input and output. */
  upb_sink input_;
  upb_bytessink output_;

  /* The "subclosure" -- used as the inner closure as part of the bytessink
   * protocol. */
  void *subc;

  /* The output buffer and limit, and our current write position.  "buf"
   * initially points to "initbuf", but is dynamically allocated if we need to
   * grow beyond the initial size. */
  char *buf, *ptr, *limit;

  /* The beginning of the current run, or undefined if we are at the top
   * level. */
  char *runbegin;

  /* The list of segments we are accumulating. */
  upb_pb_encoder_segment *segbuf, *segptr, *seglimit;

  /* The stack of enclosing submessages.  Each entry in the stack points to the
   * segment where this submessage's length is being accumulated. */
  int *stack, *top, *stacklimit;

  /* Depth of startmsg/endmsg calls. */
  int depth;
};

/* low-level buffering ********************************************************/

/* Low-level functions for interacting with the output buffer. */

/* TODO(haberman): handle pushback */
static void putbuf(upb_pb_encoder *e, const char *buf, size_t len) {
  size_t n = upb_bytessink_putbuf(e->output_, e->subc, buf, len, NULL);
  UPB_ASSERT(n == len);
}

static upb_pb_encoder_segment *top(upb_pb_encoder *e) {
  return &e->segbuf[*e->top];
}

/* Call to ensure that at least "bytes" bytes are available for writing at
 * e->ptr.  Returns false if the bytes could not be allocated. */
static bool reserve(upb_pb_encoder *e, size_t bytes) {
  if ((size_t)(e->limit - e->ptr) < bytes) {
    /* Grow buffer. */
    char *new_buf;
    size_t needed = bytes + (e->ptr - e->buf);
    size_t old_size = e->limit - e->buf;

    size_t new_size = old_size;

    while (new_size < needed) {
      new_size *= 2;
    }

    new_buf = upb_arena_realloc(e->arena, e->buf, old_size, new_size);

    if (new_buf == NULL) {
      return false;
    }

    e->ptr = new_buf + (e->ptr - e->buf);
    e->runbegin = new_buf + (e->runbegin - e->buf);
    e->limit = new_buf + new_size;
    e->buf = new_buf;
  }

  return true;
}

/* Call when "bytes" bytes have been writte at e->ptr.  The caller *must* have
 * previously called reserve() with at least this many bytes. */
static void encoder_advance(upb_pb_encoder *e, size_t bytes) {
  UPB_ASSERT((size_t)(e->limit - e->ptr) >= bytes);
  e->ptr += bytes;
}

/* Call when all of the bytes for a handler have been written.  Flushes the
 * bytes if possible and necessary, returning false if this failed. */
static bool commit(upb_pb_encoder *e) {
  if (!e->top) {
    /* We aren't inside a delimited region.  Flush our accumulated bytes to
     * the output.
     *
     * TODO(haberman): in the future we may want to delay flushing for
     * efficiency reasons. */
    putbuf(e, e->buf, e->ptr - e->buf);
    e->ptr = e->buf;
  }

  return true;
}

/* Writes the given bytes to the buffer, handling reserve/advance. */
static bool encode_bytes(upb_pb_encoder *e, const void *data, size_t len) {
  if (!reserve(e, len)) {
    return false;
  }

  memcpy(e->ptr, data, len);
  encoder_advance(e, len);
  return true;
}

/* Finish the current run by adding the run totals to the segment and message
 * length. */
static void accumulate(upb_pb_encoder *e) {
  size_t run_len;
  UPB_ASSERT(e->ptr >= e->runbegin);
  run_len = e->ptr - e->runbegin;
  e->segptr->seglen += run_len;
  top(e)->msglen += run_len;
  e->runbegin = e->ptr;
}

/* Call to indicate the start of delimited region for which the full length is
 * not yet known.  All data will be buffered until the length is known.
 * Delimited regions may be nested; their lengths will all be tracked properly. */
static bool start_delim(upb_pb_encoder *e) {
  if (e->top) {
    /* We are already buffering, advance to the next segment and push it on the
     * stack. */
    accumulate(e);

    if (++e->top == e->stacklimit) {
      /* TODO(haberman): grow stack? */
      return false;
    }

    if (++e->segptr == e->seglimit) {
      /* Grow segment buffer. */
      size_t old_size =
          (e->seglimit - e->segbuf) * sizeof(upb_pb_encoder_segment);
      size_t new_size = old_size * 2;
      upb_pb_encoder_segment *new_buf =
          upb_arena_realloc(e->arena, e->segbuf, old_size, new_size);

      if (new_buf == NULL) {
        return false;
      }

      e->segptr = new_buf + (e->segptr - e->segbuf);
      e->seglimit = new_buf + (new_size / sizeof(upb_pb_encoder_segment));
      e->segbuf = new_buf;
    }
  } else {
    /* We were previously at the top level, start buffering. */
    e->segptr = e->segbuf;
    e->top = e->stack;
    e->runbegin = e->ptr;
  }

  *e->top = e->segptr - e->segbuf;
  e->segptr->seglen = 0;
  e->segptr->msglen = 0;

  return true;
}

/* Call to indicate the end of a delimited region.  We now know the length of
 * the delimited region.  If we are not nested inside any other delimited
 * regions, we can now emit all of the buffered data we accumulated. */
static bool end_delim(upb_pb_encoder *e) {
  size_t msglen;
  accumulate(e);
  msglen = top(e)->msglen;

  if (e->top == e->stack) {
    /* All lengths are now available, emit all buffered data. */
    char buf[UPB_PB_VARINT_MAX_LEN];
    upb_pb_encoder_segment *s;
    const char *ptr = e->buf;
    for (s = e->segbuf; s <= e->segptr; s++) {
      size_t lenbytes = upb_vencode64(s->msglen, buf);
      putbuf(e, buf, lenbytes);
      putbuf(e, ptr, s->seglen);
      ptr += s->seglen;
    }

    e->ptr = e->buf;
    e->top = NULL;
  } else {
    /* Need to keep buffering; propagate length info into enclosing
     * submessages. */
    --e->top;
    top(e)->msglen += msglen + upb_varint_size(msglen);
  }

  return true;
}


/* tag_t **********************************************************************/

/* A precomputed (pre-encoded) tag and length. */

typedef struct {
  uint8_t bytes;
  char tag[7];
} tag_t;

/* Allocates a new tag for this field, and sets it in these handlerattr. */
static void new_tag(upb_handlers *h, const upb_fielddef *f, upb_wiretype_t wt,
                    upb_handlerattr *attr) {
  uint32_t n = upb_fielddef_number(f);

  tag_t *tag = upb_gmalloc(sizeof(tag_t));
  tag->bytes = upb_vencode64((n << 3) | wt, tag->tag);

  attr->handler_data = tag;
  upb_handlers_addcleanup(h, tag, upb_gfree);
}

static bool encode_tag(upb_pb_encoder *e, const tag_t *tag) {
  return encode_bytes(e, tag->tag, tag->bytes);
}


/* encoding of wire types *****************************************************/

static bool encode_fixed64(upb_pb_encoder *e, uint64_t val) {
  /* TODO(haberman): byte-swap for big endian. */
  return encode_bytes(e, &val, sizeof(uint64_t));
}

static bool encode_fixed32(upb_pb_encoder *e, uint32_t val) {
  /* TODO(haberman): byte-swap for big endian. */
  return encode_bytes(e, &val, sizeof(uint32_t));
}

static bool encode_varint(upb_pb_encoder *e, uint64_t val) {
  if (!reserve(e, UPB_PB_VARINT_MAX_LEN)) {
    return false;
  }

  encoder_advance(e, upb_vencode64(val, e->ptr));
  return true;
}

static uint64_t dbl2uint64(double d) {
  uint64_t ret;
  memcpy(&ret, &d, sizeof(uint64_t));
  return ret;
}

static uint32_t flt2uint32(float d) {
  uint32_t ret;
  memcpy(&ret, &d, sizeof(uint32_t));
  return ret;
}


/* encoding of proto types ****************************************************/

static bool startmsg(void *c, const void *hd) {
  upb_pb_encoder *e = c;
  UPB_UNUSED(hd);
  if (e->depth++ == 0) {
    upb_bytessink_start(e->output_, 0, &e->subc);
  }
  return true;
}

static bool endmsg(void *c, const void *hd, upb_status *status) {
  upb_pb_encoder *e = c;
  UPB_UNUSED(hd);
  UPB_UNUSED(status);
  if (--e->depth == 0) {
    upb_bytessink_end(e->output_);
  }
  return true;
}

static void *encode_startdelimfield(void *c, const void *hd) {
  bool ok = encode_tag(c, hd) && commit(c) && start_delim(c);
  return ok ? c : UPB_BREAK;
}

static bool encode_unknown(void *c, const void *hd, const char *buf,
                           size_t len) {
  UPB_UNUSED(hd);
  return encode_bytes(c, buf, len) && commit(c);
}

static bool encode_enddelimfield(void *c, const void *hd) {
  UPB_UNUSED(hd);
  return end_delim(c);
}

static void *encode_startgroup(void *c, const void *hd) {
  return (encode_tag(c, hd) && commit(c)) ? c : UPB_BREAK;
}

static bool encode_endgroup(void *c, const void *hd) {
  return encode_tag(c, hd) && commit(c);
}

static void *encode_startstr(void *c, const void *hd, size_t size_hint) {
  UPB_UNUSED(size_hint);
  return encode_startdelimfield(c, hd);
}

static size_t encode_strbuf(void *c, const void *hd, const char *buf,
                            size_t len, const upb_bufhandle *h) {
  UPB_UNUSED(hd);
  UPB_UNUSED(h);
  return encode_bytes(c, buf, len) ? len : 0;
}

#define T(type, ctype, convert, encode)                                  \
  static bool encode_scalar_##type(void *e, const void *hd, ctype val) { \
    return encode_tag(e, hd) && encode(e, (convert)(val)) && commit(e);  \
  }                                                                      \
  static bool encode_packed_##type(void *e, const void *hd, ctype val) { \
    UPB_UNUSED(hd);                                                      \
    return encode(e, (convert)(val));                                    \
  }

T(double,   double,   dbl2uint64,   encode_fixed64)
T(float,    float,    flt2uint32,   encode_fixed32)
T(int64,    int64_t,  uint64_t,     encode_varint)
T(int32,    int32_t,  int64_t,      encode_varint)
T(fixed64,  uint64_t, uint64_t,     encode_fixed64)
T(fixed32,  uint32_t, uint32_t,     encode_fixed32)
T(bool,     bool,     bool,         encode_varint)
T(uint32,   uint32_t, uint32_t,     encode_varint)
T(uint64,   uint64_t, uint64_t,     encode_varint)
T(enum,     int32_t,  uint32_t,     encode_varint)
T(sfixed32, int32_t,  uint32_t,     encode_fixed32)
T(sfixed64, int64_t,  uint64_t,     encode_fixed64)
T(sint32,   int32_t,  upb_zzenc_32, encode_varint)
T(sint64,   int64_t,  upb_zzenc_64, encode_varint)

#undef T


/* code to build the handlers *************************************************/

#include <stdio.h>
static void newhandlers_callback(const void *closure, upb_handlers *h) {
  const upb_msgdef *m;
  upb_msg_field_iter i;

  UPB_UNUSED(closure);

  upb_handlers_setstartmsg(h, startmsg, NULL);
  upb_handlers_setendmsg(h, endmsg, NULL);
  upb_handlers_setunknown(h, encode_unknown, NULL);

  m = upb_handlers_msgdef(h);
  for(upb_msg_field_begin(&i, m);
      !upb_msg_field_done(&i);
      upb_msg_field_next(&i)) {
    const upb_fielddef *f = upb_msg_iter_field(&i);
    bool packed = upb_fielddef_isseq(f) && upb_fielddef_isprimitive(f) &&
                  upb_fielddef_packed(f);
    upb_handlerattr attr = UPB_HANDLERATTR_INIT;
    upb_wiretype_t wt =
        packed ? UPB_WIRE_TYPE_DELIMITED
               : upb_pb_native_wire_types[upb_fielddef_descriptortype(f)];

    /* Pre-encode the tag for this field. */
    new_tag(h, f, wt, &attr);

    if (packed) {
      upb_handlers_setstartseq(h, f, encode_startdelimfield, &attr);
      upb_handlers_setendseq(h, f, encode_enddelimfield, &attr);
    }

#define T(upper, lower, upbtype)                                     \
  case UPB_DESCRIPTOR_TYPE_##upper:                                  \
    if (packed) {                                                    \
      upb_handlers_set##upbtype(h, f, encode_packed_##lower, &attr); \
    } else {                                                         \
      upb_handlers_set##upbtype(h, f, encode_scalar_##lower, &attr); \
    }                                                                \
    break;

    switch (upb_fielddef_descriptortype(f)) {
      T(DOUBLE,   double,   double);
      T(FLOAT,    float,    float);
      T(INT64,    int64,    int64);
      T(INT32,    int32,    int32);
      T(FIXED64,  fixed64,  uint64);
      T(FIXED32,  fixed32,  uint32);
      T(BOOL,     bool,     bool);
      T(UINT32,   uint32,   uint32);
      T(UINT64,   uint64,   uint64);
      T(ENUM,     enum,     int32);
      T(SFIXED32, sfixed32, int32);
      T(SFIXED64, sfixed64, int64);
      T(SINT32,   sint32,   int32);
      T(SINT64,   sint64,   int64);
      case UPB_DESCRIPTOR_TYPE_STRING:
      case UPB_DESCRIPTOR_TYPE_BYTES:
        upb_handlers_setstartstr(h, f, encode_startstr, &attr);
        upb_handlers_setendstr(h, f, encode_enddelimfield, &attr);
        upb_handlers_setstring(h, f, encode_strbuf, &attr);
        break;
      case UPB_DESCRIPTOR_TYPE_MESSAGE:
        upb_handlers_setstartsubmsg(h, f, encode_startdelimfield, &attr);
        upb_handlers_setendsubmsg(h, f, encode_enddelimfield, &attr);
        break;
      case UPB_DESCRIPTOR_TYPE_GROUP: {
        /* Endgroup takes a different tag (wire_type = END_GROUP). */
        upb_handlerattr attr2 = UPB_HANDLERATTR_INIT;
        new_tag(h, f, UPB_WIRE_TYPE_END_GROUP, &attr2);

        upb_handlers_setstartsubmsg(h, f, encode_startgroup, &attr);
        upb_handlers_setendsubmsg(h, f, encode_endgroup, &attr2);

        break;
      }
    }

#undef T
  }
}

void upb_pb_encoder_reset(upb_pb_encoder *e) {
  e->segptr = NULL;
  e->top = NULL;
  e->depth = 0;
}


/* public API *****************************************************************/

upb_handlercache *upb_pb_encoder_newcache(void) {
  return upb_handlercache_new(newhandlers_callback, NULL);
}

upb_pb_encoder *upb_pb_encoder_create(upb_arena *arena, const upb_handlers *h,
                                      upb_bytessink output) {
  const size_t initial_bufsize = 256;
  const size_t initial_segbufsize = 16;
  /* TODO(haberman): make this configurable. */
  const size_t stack_size = 64;
#ifndef NDEBUG
  const size_t size_before = upb_arena_bytesallocated(arena);
#endif

  upb_pb_encoder *e = upb_arena_malloc(arena, sizeof(upb_pb_encoder));
  if (!e) return NULL;

  e->buf = upb_arena_malloc(arena, initial_bufsize);
  e->segbuf = upb_arena_malloc(arena, initial_segbufsize * sizeof(*e->segbuf));
  e->stack = upb_arena_malloc(arena, stack_size * sizeof(*e->stack));

  if (!e->buf || !e->segbuf || !e->stack) {
    return NULL;
  }

  e->limit = e->buf + initial_bufsize;
  e->seglimit = e->segbuf + initial_segbufsize;
  e->stacklimit = e->stack + stack_size;

  upb_pb_encoder_reset(e);
  upb_sink_reset(&e->input_, h, e);

  e->arena = arena;
  e->output_ = output;
  e->subc = output.closure;
  e->ptr = e->buf;

  /* If this fails, increase the value in encoder.h. */
  UPB_ASSERT_DEBUGVAR(upb_arena_bytesallocated(arena) - size_before <=
                      UPB_PB_ENCODER_SIZE);
  return e;
}

upb_sink upb_pb_encoder_input(upb_pb_encoder *e) { return e->input_; }
