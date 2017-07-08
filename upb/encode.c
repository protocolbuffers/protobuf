
#include "upb/encode.h"
#include "upb/structs.int.h"

#define UPB_PB_VARINT_MAX_LEN 10

static size_t upb_encode_varint(uint64_t val, char *buf) {
  size_t i;
  if (val == 0) { buf[0] = 0; return 1; }
  i = 0;
  while (val) {
    uint8_t byte = val & 0x7fU;
    val >>= 7;
    if (val) byte |= 0x80U;
    buf[i++] = byte;
  }
  return i;
}

static size_t upb_varint_size(uint64_t val) {
  char buf[UPB_PB_VARINT_MAX_LEN];
  return upb_encode_varint(val, buf);
}

static uint32_t upb_zzenc_32(int32_t n) { return (n << 1) ^ (n >> 31); }
static uint64_t upb_zzenc_64(int64_t n) { return (n << 1) ^ (n >> 63); }

typedef enum {
  UPB_WIRE_TYPE_VARINT      = 0,
  UPB_WIRE_TYPE_64BIT       = 1,
  UPB_WIRE_TYPE_DELIMITED   = 2,
  UPB_WIRE_TYPE_START_GROUP = 3,
  UPB_WIRE_TYPE_END_GROUP   = 4,
  UPB_WIRE_TYPE_32BIT       = 5
} upb_wiretype_t;

/* Index is descriptor type. */
const uint8_t upb_native_wiretypes[] = {
  UPB_WIRE_TYPE_END_GROUP,     /* ENDGROUP */
  UPB_WIRE_TYPE_64BIT,         /* DOUBLE */
  UPB_WIRE_TYPE_32BIT,         /* FLOAT */
  UPB_WIRE_TYPE_VARINT,        /* INT64 */
  UPB_WIRE_TYPE_VARINT,        /* UINT64 */
  UPB_WIRE_TYPE_VARINT,        /* INT32 */
  UPB_WIRE_TYPE_64BIT,         /* FIXED64 */
  UPB_WIRE_TYPE_32BIT,         /* FIXED32 */
  UPB_WIRE_TYPE_VARINT,        /* BOOL */
  UPB_WIRE_TYPE_DELIMITED,     /* STRING */
  UPB_WIRE_TYPE_START_GROUP,   /* GROUP */
  UPB_WIRE_TYPE_DELIMITED,     /* MESSAGE */
  UPB_WIRE_TYPE_DELIMITED,     /* BYTES */
  UPB_WIRE_TYPE_VARINT,        /* UINT32 */
  UPB_WIRE_TYPE_VARINT,        /* ENUM */
  UPB_WIRE_TYPE_32BIT,         /* SFIXED32 */
  UPB_WIRE_TYPE_64BIT,         /* SFIXED64 */
  UPB_WIRE_TYPE_VARINT,        /* SINT32 */
  UPB_WIRE_TYPE_VARINT,        /* SINT64 */
};

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
} upb_segment;

typedef struct {
  upb_env *env;
  char *buf, *ptr, *limit;

  /* The beginning of the current run, or undefined if we are at the top
   * level. */
  char *runbegin;

  /* The list of segments we are accumulating. */
  upb_segment *segbuf, *segptr, *seglimit;

  /* The stack of enclosing submessages.  Each entry in the stack points to the
   * segment where this submessage's length is being accumulated. */
  int *stack, *top, *stacklimit;
} upb_encstate;

static upb_segment *upb_encode_top(upb_encstate *e) {
  return &e->segbuf[*e->top];
}

static bool upb_encode_growbuffer(upb_encstate *e, size_t bytes) {
  char *new_buf;
  size_t needed = bytes + (e->ptr - e->buf);
  size_t old_size = e->limit - e->buf;

  size_t new_size = old_size;

  while (new_size < needed) {
    new_size *= 2;
  }

  new_buf = upb_env_realloc(e->env, e->buf, old_size, new_size);

  if (new_buf == NULL) {
    return false;
  }

  e->ptr = new_buf + (e->ptr - e->buf);
  e->runbegin = new_buf + (e->runbegin - e->buf);
  e->limit = new_buf + new_size;
  e->buf = new_buf;
  return true;
}

/* Call to ensure that at least "bytes" bytes are available for writing at
 * e->ptr.  Returns false if the bytes could not be allocated. */
static bool upb_encode_reserve(upb_encstate *e, size_t bytes) {
  if (UPB_LIKELY((size_t)(e->limit - e->ptr) >= bytes)) {
    return true;
  }

  return upb_encode_growbuffer(e, bytes);
}

/* Call when "bytes" bytes have been writte at e->ptr.  The caller *must* have
 * previously called reserve() with at least this many bytes. */
static void upb_encode_advance(upb_encstate *e, size_t bytes) {
  UPB_ASSERT((size_t)(e->limit - e->ptr) >= bytes);
  e->ptr += bytes;
}

/* Writes the given bytes to the buffer, handling reserve/advance. */
static bool upb_put_bytes(upb_encstate *e, const void *data, size_t len) {
  if (!upb_encode_reserve(e, len)) {
    return false;
  }

  memcpy(e->ptr, data, len);
  upb_encode_advance(e, len);
  return true;
}

/* Finish the current run by adding the run totals to the segment and message
 * length. */
static void upb_encode_accumulate(upb_encstate *e) {
  size_t run_len;
  UPB_ASSERT(e->ptr >= e->runbegin);
  run_len = e->ptr - e->runbegin;
  e->segptr->seglen += run_len;
  upb_encode_top(e)->msglen += run_len;
  e->runbegin = e->ptr;
}

/* Call to indicate the start of delimited region for which the full length is
 * not yet known.  The length will be inserted at the current position once it
 * is known (and subsequent data moved if necessary). */
static bool upb_encode_startdelim(upb_encstate *e) {
  if (e->top) {
    /* We are already buffering, advance to the next segment and push it on the
     * stack. */
    upb_encode_accumulate(e);

    if (++e->top == e->stacklimit) {
      /* TODO(haberman): grow stack? */
      return false;
    }

    if (++e->segptr == e->seglimit) {
      /* Grow segment buffer. */
      size_t old_size =
          (e->seglimit - e->segbuf) * sizeof(upb_segment);
      size_t new_size = old_size * 2;
      upb_segment *new_buf =
          upb_env_realloc(e->env, e->segbuf, old_size, new_size);

      if (new_buf == NULL) {
        return false;
      }

      e->segptr = new_buf + (e->segptr - e->segbuf);
      e->seglimit = new_buf + (new_size / sizeof(upb_segment));
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
static bool upb_encode_enddelim(upb_encstate *e) {
  size_t msglen;
  upb_encode_accumulate(e);
  msglen = upb_encode_top(e)->msglen;

  if (e->top == e->stack) {
    /* All lengths are now available, emit all buffered data. */
    char buf[UPB_PB_VARINT_MAX_LEN];
    upb_segment *s;
    const char *ptr = e->buf;
    for (s = e->segbuf; s <= e->segptr; s++) {
      size_t lenbytes = upb_encode_varint(s->msglen, buf);
      //putbuf(e, buf, lenbytes);
      //putbuf(e, ptr, s->seglen);
      ptr += s->seglen;
    }

    e->ptr = e->buf;
    e->top = NULL;
  } else {
    /* Need to keep buffering; propagate length info into enclosing
     * submessages. */
    --e->top;
    upb_encode_top(e)->msglen += msglen + upb_varint_size(msglen);
  }

  return true;
}

/* encoding of wire types *****************************************************/

static bool upb_put_fixed64(upb_encstate *e, uint64_t val) {
  /* TODO(haberman): byte-swap for big endian. */
  return upb_put_bytes(e, &val, sizeof(uint64_t));
}

static bool upb_put_fixed32(upb_encstate *e, uint32_t val) {
  /* TODO(haberman): byte-swap for big endian. */
  return upb_put_bytes(e, &val, sizeof(uint32_t));
}

static bool upb_put_varint(upb_encstate *e, uint64_t val) {
  if (!upb_encode_reserve(e, UPB_PB_VARINT_MAX_LEN)) {
    return false;
  }

  upb_encode_advance(e, upb_encode_varint(val, e->ptr));
  return true;
}

static bool upb_put_double(upb_encstate *e, double d) {
  uint64_t u64;
  UPB_ASSERT(sizeof(double) == sizeof(uint64_t));
  memcpy(&u64, &d, sizeof(uint64_t));
  return upb_put_fixed64(e, u64);
}

static bool upb_put_float(upb_encstate *e, float d) {
  uint32_t u32;
  UPB_ASSERT(sizeof(float) == sizeof(uint32_t));
  memcpy(&u32, &d, sizeof(uint32_t));
  return upb_put_fixed32(e, u32);
}

static uint32_t upb_readcase(const char *msg, const upb_msglayout_msginit_v1 *m,
                             int oneof_index) {
  uint32_t ret;
  memcpy(&ret, msg + m->oneofs[oneof_index].case_offset, sizeof(ret));
  return ret;
}

static bool upb_readhasbit(const char *msg,
                           const upb_msglayout_fieldinit_v1 *f) {
  UPB_ASSERT(f->hasbit != UPB_NO_HASBIT);
  return msg[f->hasbit / 8] & (1 << (f->hasbit % 8));
}

static bool upb_put_tag(upb_encstate *e, int field_number, int wire_type) {
  return upb_put_varint(e, (field_number << 3) | wire_type);
}

static bool upb_put_fixedarray(upb_encstate *e, const upb_array *arr,
                               size_t size) {
  size_t bytes = arr->len * size;
  return upb_put_varint(e, bytes) && upb_put_bytes(e, arr->data, bytes);
}

bool upb_encode_message(upb_encstate *e, const char *msg,
                        const upb_msglayout_msginit_v1 *m);

static bool upb_encode_array(upb_encstate *e, const char *field_mem,
                             const upb_msglayout_msginit_v1 *m,
                             const upb_msglayout_fieldinit_v1 *f) {
  const upb_array *arr = *(const upb_array**)field_mem;

  if (arr->len == 0) {
    return true;
  }

  /* We encode all primitive arrays as packed, regardless of what was specified
   * in the .proto file.  Could special case 1-sized arrays. */
  if (!upb_put_tag(e, f->number, UPB_WIRE_TYPE_DELIMITED)) {
    return false;
  }

#define VARINT_CASE(ctype, encode) { \
  uint64_t *data = arr->data; \
  uint64_t *limit = data + arr->len; \
  if (!upb_encode_startdelim(e)) { \
    return false; \
  } \
  for (; data < limit; data++) { \
    if (!upb_put_varint(e, encode)) { \
      return false; \
    } \
  } \
  return upb_encode_enddelim(e); \
}

  switch (f->type) {
    case UPB_DESCRIPTOR_TYPE_DOUBLE:
      return upb_put_fixedarray(e, arr, sizeof(double));
    case UPB_DESCRIPTOR_TYPE_FLOAT:
      return upb_put_fixedarray(e, arr, sizeof(float));
    case UPB_DESCRIPTOR_TYPE_SFIXED64:
    case UPB_DESCRIPTOR_TYPE_FIXED64:
      return upb_put_fixedarray(e, arr, sizeof(uint64_t));
    case UPB_DESCRIPTOR_TYPE_FIXED32:
    case UPB_DESCRIPTOR_TYPE_SFIXED32:
      return upb_put_fixedarray(e, arr, sizeof(uint32_t));
    case UPB_DESCRIPTOR_TYPE_INT64:
    case UPB_DESCRIPTOR_TYPE_UINT64:
      VARINT_CASE(uint64_t, *data);
    case UPB_DESCRIPTOR_TYPE_UINT32:
    case UPB_DESCRIPTOR_TYPE_INT32:
    case UPB_DESCRIPTOR_TYPE_ENUM:
      VARINT_CASE(uint32_t, *data);
    case UPB_DESCRIPTOR_TYPE_BOOL:
      VARINT_CASE(bool, *data);
    case UPB_DESCRIPTOR_TYPE_SINT32:
      VARINT_CASE(int32_t, upb_zzenc_32(*data));
    case UPB_DESCRIPTOR_TYPE_SINT64:
      VARINT_CASE(int64_t, upb_zzenc_64(*data));
    case UPB_DESCRIPTOR_TYPE_STRING:
    case UPB_DESCRIPTOR_TYPE_BYTES: {
      upb_stringview *data = arr->data;
      upb_stringview *limit = data + arr->len;
      goto put_string_data;  /* Skip first tag, we already put it. */
      for (; data < limit; data++) {
        if (!upb_put_tag(e, f->number, UPB_WIRE_TYPE_DELIMITED)) {
          return false;
        }
put_string_data:
        if (!upb_put_varint(e, data->size) ||
            !upb_put_bytes(e, data->data, data->size)) {
          return false;
        }
      }
    }
    case UPB_DESCRIPTOR_TYPE_GROUP:
    case UPB_DESCRIPTOR_TYPE_MESSAGE: {
      void **data = arr->data;
      void **limit = data + arr->len;
      const upb_msglayout_msginit_v1 *subm = m->submsgs[f->submsg_index];
      goto put_submsg_data;  /* Skip first tag, we already put it. */
      for (; data < limit; data++) {
        if (!upb_put_tag(e, f->number, UPB_WIRE_TYPE_DELIMITED)) {
          return false;
        }
put_submsg_data:
        if (!upb_encode_startdelim(e) ||
            !upb_encode_message(e, *data, subm) ||
            !upb_encode_enddelim(e)) {
          return false;
        }
      }
    }
  }
  UPB_UNREACHABLE();
#undef VARINT_CASE
}

static bool upb_encode_scalarfield(upb_encstate *e, const char *field_mem,
                                   const upb_msglayout_msginit_v1 *m,
                                   const upb_msglayout_fieldinit_v1 *f,
                                   bool is_proto3) {
#define CASE(ctype, type, wire_type, encodeval) { \
  ctype val = *(ctype*)field_mem; \
  if (is_proto3 && val == 0) { \
    return true; \
  } \
  return upb_put_tag(e, f->number, wire_type) && \
      upb_put_ ## type(e, encodeval); \
}

  switch (f->type) {
    case UPB_DESCRIPTOR_TYPE_DOUBLE:
      CASE(double, double, UPB_WIRE_TYPE_64BIT, val)
    case UPB_DESCRIPTOR_TYPE_FLOAT:
      CASE(float, float, UPB_WIRE_TYPE_32BIT, val)
    case UPB_DESCRIPTOR_TYPE_INT64:
    case UPB_DESCRIPTOR_TYPE_UINT64:
      CASE(uint64_t, varint, UPB_WIRE_TYPE_VARINT, val)
    case UPB_DESCRIPTOR_TYPE_UINT32:
    case UPB_DESCRIPTOR_TYPE_INT32:
    case UPB_DESCRIPTOR_TYPE_ENUM:
      CASE(uint32_t, varint, UPB_WIRE_TYPE_VARINT, val)
    case UPB_DESCRIPTOR_TYPE_SFIXED64:
    case UPB_DESCRIPTOR_TYPE_FIXED64:
      CASE(uint64_t, fixed64, UPB_WIRE_TYPE_64BIT, val)
    case UPB_DESCRIPTOR_TYPE_FIXED32:
    case UPB_DESCRIPTOR_TYPE_SFIXED32:
      CASE(uint32_t, fixed32, UPB_WIRE_TYPE_32BIT, val)
    case UPB_DESCRIPTOR_TYPE_BOOL:
      CASE(bool, varint, UPB_WIRE_TYPE_VARINT, val)
    case UPB_DESCRIPTOR_TYPE_SINT32:
      CASE(int32_t, varint, UPB_WIRE_TYPE_VARINT, upb_zzenc_32(val))
    case UPB_DESCRIPTOR_TYPE_SINT64:
      CASE(int64_t, varint, UPB_WIRE_TYPE_VARINT, upb_zzenc_64(val))
    case UPB_DESCRIPTOR_TYPE_STRING:
    case UPB_DESCRIPTOR_TYPE_BYTES: {
      upb_stringview view = *(upb_stringview*)field_mem;
      if (is_proto3 && view.size == 0) {
        return true;
      }
      return upb_put_tag(e, f->number, UPB_WIRE_TYPE_DELIMITED) &&
          upb_put_varint(e, view.size) &&
          upb_put_bytes(e, view.data, view.size);
    }
    case UPB_DESCRIPTOR_TYPE_GROUP:
    case UPB_DESCRIPTOR_TYPE_MESSAGE: {
      void *submsg = *(void**)field_mem;
      if (is_proto3 && submsg == NULL) {
        return true;
      }
      return upb_put_tag(e, f->number, UPB_WIRE_TYPE_DELIMITED) &&
          upb_encode_startdelim(e) &&
          upb_encode_message(e, submsg, m->submsgs[f->submsg_index]) &&
          upb_encode_enddelim(e);
    }
  }
#undef CASE
  UPB_UNREACHABLE();
}

bool upb_encode_hasscalarfield(const char *msg,
                               const upb_msglayout_msginit_v1 *m,
                               const upb_msglayout_fieldinit_v1 *f) {
  if (f->oneof_index != UPB_NOT_IN_ONEOF) {
    return upb_readcase(msg, m, f->oneof_index) == f->number;
  } else if (m->is_proto2) {
    return upb_readhasbit(msg, f);
  } else {
    /* For proto3, we'll test for the field being empty later. */
    return true;
  }
}

bool upb_encode_message(upb_encstate* e, const char *msg,
                        const upb_msglayout_msginit_v1 *m) {
  int i;
  for (i = 0; i < m->field_count; i++) {
    const upb_msglayout_fieldinit_v1 *f = &m->fields[i];

    if (f->label == UPB_LABEL_REPEATED) {
      if (!upb_encode_array(e, msg, m, f)) {
        return NULL;
      }
    } else {
      if (upb_encode_hasscalarfield(msg, m, f) &&
          !upb_encode_scalarfield(e, msg + f->offset, m, f, !m->is_proto2)) {
        return NULL;
      }
    }
  }

  return true;
}

char *upb_encode(const void *msg, const upb_msglayout_msginit_v1 *m,
                 upb_env *env, size_t *size) {
  upb_encstate e;

  if (!upb_encode_message(&e, msg, m)) {
    return false;
  }

  *size = e.ptr - e.buf;
  return e.buf;
}
