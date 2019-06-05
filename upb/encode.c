/* We encode backwards, to avoid pre-computing lengths (one-pass encode). */

#include "upb/encode.h"

#include <string.h>

#include "upb/msg.h"
#include "upb/upb.h"

#include "upb/port_def.inc"

#define UPB_PB_VARINT_MAX_LEN 10
#define CHK(x) do { if (!(x)) { return false; } } while(0)

static size_t upb_encode_varint(uint64_t val, char *buf) {
  size_t i;
  if (val < 128) { buf[0] = val; return 1; }
  i = 0;
  while (val) {
    uint8_t byte = val & 0x7fU;
    val >>= 7;
    if (val) byte |= 0x80U;
    buf[i++] = byte;
  }
  return i;
}

static uint32_t upb_zzencode_32(int32_t n) { return (n << 1) ^ (n >> 31); }
static uint64_t upb_zzencode_64(int64_t n) { return (n << 1) ^ (n >> 63); }

typedef struct {
  upb_alloc *alloc;
  char *buf, *ptr, *limit;
} upb_encstate;

static size_t upb_roundup_pow2(size_t bytes) {
  size_t ret = 128;
  while (ret < bytes) {
    ret *= 2;
  }
  return ret;
}

static bool upb_encode_growbuffer(upb_encstate *e, size_t bytes) {
  size_t old_size = e->limit - e->buf;
  size_t new_size = upb_roundup_pow2(bytes + (e->limit - e->ptr));
  char *new_buf = upb_realloc(e->alloc, e->buf, old_size, new_size);
  CHK(new_buf);

  /* We want previous data at the end, realloc() put it at the beginning. */
  memmove(new_buf + new_size - old_size, e->buf, old_size);

  e->ptr = new_buf + new_size - (e->limit - e->ptr);
  e->limit = new_buf + new_size;
  e->buf = new_buf;
  return true;
}

/* Call to ensure that at least "bytes" bytes are available for writing at
 * e->ptr.  Returns false if the bytes could not be allocated. */
static bool upb_encode_reserve(upb_encstate *e, size_t bytes) {
  CHK(UPB_LIKELY((size_t)(e->ptr - e->buf) >= bytes) ||
      upb_encode_growbuffer(e, bytes));

  e->ptr -= bytes;
  return true;
}

/* Writes the given bytes to the buffer, handling reserve/advance. */
static bool upb_put_bytes(upb_encstate *e, const void *data, size_t len) {
  CHK(upb_encode_reserve(e, len));
  memcpy(e->ptr, data, len);
  return true;
}

static bool upb_put_fixed64(upb_encstate *e, uint64_t val) {
  /* TODO(haberman): byte-swap for big endian. */
  return upb_put_bytes(e, &val, sizeof(uint64_t));
}

static bool upb_put_fixed32(upb_encstate *e, uint32_t val) {
  /* TODO(haberman): byte-swap for big endian. */
  return upb_put_bytes(e, &val, sizeof(uint32_t));
}

static bool upb_put_varint(upb_encstate *e, uint64_t val) {
  size_t len;
  char *start;
  CHK(upb_encode_reserve(e, UPB_PB_VARINT_MAX_LEN));
  len = upb_encode_varint(val, e->ptr);
  start = e->ptr + UPB_PB_VARINT_MAX_LEN - len;
  memmove(start, e->ptr, len);
  e->ptr = start;
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

static uint32_t upb_readcase(const char *msg, const upb_msglayout_field *f) {
  uint32_t ret;
  uint32_t offset = ~f->presence;
  memcpy(&ret, msg + offset, sizeof(ret));
  return ret;
}

static bool upb_readhasbit(const char *msg, const upb_msglayout_field *f) {
  uint32_t hasbit = f->presence;
  UPB_ASSERT(f->presence > 0);
  return msg[hasbit / 8] & (1 << (hasbit % 8));
}

static bool upb_put_tag(upb_encstate *e, int field_number, int wire_type) {
  return upb_put_varint(e, (field_number << 3) | wire_type);
}

static bool upb_put_fixedarray(upb_encstate *e, const upb_array *arr,
                               size_t size) {
  size_t bytes = arr->len * size;
  return upb_put_bytes(e, arr->data, bytes) && upb_put_varint(e, bytes);
}

bool upb_encode_message(upb_encstate *e, const char *msg,
                        const upb_msglayout *m, size_t *size);

static bool upb_encode_array(upb_encstate *e, const char *field_mem,
                             const upb_msglayout *m,
                             const upb_msglayout_field *f) {
  const upb_array *arr = *(const upb_array**)field_mem;

  if (arr == NULL || arr->len == 0) {
    return true;
  }

#define VARINT_CASE(ctype, encode) { \
  ctype *start = arr->data; \
  ctype *ptr = start + arr->len; \
  size_t pre_len = e->limit - e->ptr; \
  do { \
    ptr--; \
    CHK(upb_put_varint(e, encode)); \
  } while (ptr != start); \
  CHK(upb_put_varint(e, e->limit - e->ptr - pre_len)); \
} \
break; \
do { ; } while(0)

  switch (f->descriptortype) {
    case UPB_DESCRIPTOR_TYPE_DOUBLE:
      CHK(upb_put_fixedarray(e, arr, sizeof(double)));
      break;
    case UPB_DESCRIPTOR_TYPE_FLOAT:
      CHK(upb_put_fixedarray(e, arr, sizeof(float)));
      break;
    case UPB_DESCRIPTOR_TYPE_SFIXED64:
    case UPB_DESCRIPTOR_TYPE_FIXED64:
      CHK(upb_put_fixedarray(e, arr, sizeof(uint64_t)));
      break;
    case UPB_DESCRIPTOR_TYPE_FIXED32:
    case UPB_DESCRIPTOR_TYPE_SFIXED32:
      CHK(upb_put_fixedarray(e, arr, sizeof(uint32_t)));
      break;
    case UPB_DESCRIPTOR_TYPE_INT64:
    case UPB_DESCRIPTOR_TYPE_UINT64:
      VARINT_CASE(uint64_t, *ptr);
    case UPB_DESCRIPTOR_TYPE_UINT32:
      VARINT_CASE(uint32_t, *ptr);
    case UPB_DESCRIPTOR_TYPE_INT32:
    case UPB_DESCRIPTOR_TYPE_ENUM:
      VARINT_CASE(int32_t, (int64_t)*ptr);
    case UPB_DESCRIPTOR_TYPE_BOOL:
      VARINT_CASE(bool, *ptr);
    case UPB_DESCRIPTOR_TYPE_SINT32:
      VARINT_CASE(int32_t, upb_zzencode_32(*ptr));
    case UPB_DESCRIPTOR_TYPE_SINT64:
      VARINT_CASE(int64_t, upb_zzencode_64(*ptr));
    case UPB_DESCRIPTOR_TYPE_STRING:
    case UPB_DESCRIPTOR_TYPE_BYTES: {
      upb_strview *start = arr->data;
      upb_strview *ptr = start + arr->len;
      do {
        ptr--;
        CHK(upb_put_bytes(e, ptr->data, ptr->size) &&
            upb_put_varint(e, ptr->size) &&
            upb_put_tag(e, f->number, UPB_WIRE_TYPE_DELIMITED));
      } while (ptr != start);
      return true;
    }
    case UPB_DESCRIPTOR_TYPE_GROUP: {
      void **start = arr->data;
      void **ptr = start + arr->len;
      const upb_msglayout *subm = m->submsgs[f->submsg_index];
      do {
        size_t size;
        ptr--;
        CHK(upb_put_tag(e, f->number, UPB_WIRE_TYPE_END_GROUP) &&
            upb_encode_message(e, *ptr, subm, &size) &&
            upb_put_tag(e, f->number, UPB_WIRE_TYPE_START_GROUP));
      } while (ptr != start);
      return true;
    }
    case UPB_DESCRIPTOR_TYPE_MESSAGE: {
      void **start = arr->data;
      void **ptr = start + arr->len;
      const upb_msglayout *subm = m->submsgs[f->submsg_index];
      do {
        size_t size;
        ptr--;
        CHK(upb_encode_message(e, *ptr, subm, &size) &&
            upb_put_varint(e, size) &&
            upb_put_tag(e, f->number, UPB_WIRE_TYPE_DELIMITED));
      } while (ptr != start);
      return true;
    }
  }
#undef VARINT_CASE

  /* We encode all primitive arrays as packed, regardless of what was specified
   * in the .proto file.  Could special case 1-sized arrays. */
  CHK(upb_put_tag(e, f->number, UPB_WIRE_TYPE_DELIMITED));
  return true;
}

static bool upb_encode_scalarfield(upb_encstate *e, const char *field_mem,
                                   const upb_msglayout *m,
                                   const upb_msglayout_field *f,
                                   bool skip_zero_value) {
#define CASE(ctype, type, wire_type, encodeval) do { \
  ctype val = *(ctype*)field_mem; \
  if (skip_zero_value && val == 0) { \
    return true; \
  } \
  return upb_put_ ## type(e, encodeval) && \
      upb_put_tag(e, f->number, wire_type); \
} while(0)

  switch (f->descriptortype) {
    case UPB_DESCRIPTOR_TYPE_DOUBLE:
      CASE(double, double, UPB_WIRE_TYPE_64BIT, val);
    case UPB_DESCRIPTOR_TYPE_FLOAT:
      CASE(float, float, UPB_WIRE_TYPE_32BIT, val);
    case UPB_DESCRIPTOR_TYPE_INT64:
    case UPB_DESCRIPTOR_TYPE_UINT64:
      CASE(uint64_t, varint, UPB_WIRE_TYPE_VARINT, val);
    case UPB_DESCRIPTOR_TYPE_UINT32:
      CASE(uint32_t, varint, UPB_WIRE_TYPE_VARINT, val);
    case UPB_DESCRIPTOR_TYPE_INT32:
    case UPB_DESCRIPTOR_TYPE_ENUM:
      CASE(int32_t, varint, UPB_WIRE_TYPE_VARINT, (int64_t)val);
    case UPB_DESCRIPTOR_TYPE_SFIXED64:
    case UPB_DESCRIPTOR_TYPE_FIXED64:
      CASE(uint64_t, fixed64, UPB_WIRE_TYPE_64BIT, val);
    case UPB_DESCRIPTOR_TYPE_FIXED32:
    case UPB_DESCRIPTOR_TYPE_SFIXED32:
      CASE(uint32_t, fixed32, UPB_WIRE_TYPE_32BIT, val);
    case UPB_DESCRIPTOR_TYPE_BOOL:
      CASE(bool, varint, UPB_WIRE_TYPE_VARINT, val);
    case UPB_DESCRIPTOR_TYPE_SINT32:
      CASE(int32_t, varint, UPB_WIRE_TYPE_VARINT, upb_zzencode_32(val));
    case UPB_DESCRIPTOR_TYPE_SINT64:
      CASE(int64_t, varint, UPB_WIRE_TYPE_VARINT, upb_zzencode_64(val));
    case UPB_DESCRIPTOR_TYPE_STRING:
    case UPB_DESCRIPTOR_TYPE_BYTES: {
      upb_strview view = *(upb_strview*)field_mem;
      if (skip_zero_value && view.size == 0) {
        return true;
      }
      return upb_put_bytes(e, view.data, view.size) &&
          upb_put_varint(e, view.size) &&
          upb_put_tag(e, f->number, UPB_WIRE_TYPE_DELIMITED);
    }
    case UPB_DESCRIPTOR_TYPE_GROUP: {
      size_t size;
      void *submsg = *(void **)field_mem;
      const upb_msglayout *subm = m->submsgs[f->submsg_index];
      if (submsg == NULL) {
        return true;
      }
      return upb_put_tag(e, f->number, UPB_WIRE_TYPE_END_GROUP) &&
          upb_encode_message(e, submsg, subm, &size) &&
          upb_put_tag(e, f->number, UPB_WIRE_TYPE_START_GROUP);
    }
    case UPB_DESCRIPTOR_TYPE_MESSAGE: {
      size_t size;
      void *submsg = *(void **)field_mem;
      const upb_msglayout *subm = m->submsgs[f->submsg_index];
      if (submsg == NULL) {
        return true;
      }
      return upb_encode_message(e, submsg, subm, &size) &&
          upb_put_varint(e, size) &&
          upb_put_tag(e, f->number, UPB_WIRE_TYPE_DELIMITED);
    }
  }
#undef CASE
  UPB_UNREACHABLE();
}

bool upb_encode_message(upb_encstate *e, const char *msg,
                        const upb_msglayout *m, size_t *size) {
  int i;
  size_t pre_len = e->limit - e->ptr;
  const char *unknown;
  size_t unknown_size;

  for (i = m->field_count - 1; i >= 0; i--) {
    const upb_msglayout_field *f = &m->fields[i];

    if (f->label == UPB_LABEL_REPEATED) {
      CHK(upb_encode_array(e, msg + f->offset, m, f));
    } else {
      bool skip_empty = false;
      if (f->presence == 0) {
        /* Proto3 presence. */
        skip_empty = true;
      } else if (f->presence > 0) {
        /* Proto2 presence: hasbit. */
        if (!upb_readhasbit(msg, f)) {
          continue;
        }
      } else {
        /* Field is in a oneof. */
        if (upb_readcase(msg, f) != f->number) {
          continue;
        }
      }
      CHK(upb_encode_scalarfield(e, msg + f->offset, m, f, skip_empty));
    }
  }

  unknown = upb_msg_getunknown(msg, &unknown_size);

  if (unknown) {
    upb_put_bytes(e, unknown, unknown_size);
  }

  *size = (e->limit - e->ptr) - pre_len;
  return true;
}

char *upb_encode(const void *msg, const upb_msglayout *m, upb_arena *arena,
                 size_t *size) {
  upb_encstate e;
  e.alloc = upb_arena_alloc(arena);
  e.buf = NULL;
  e.limit = NULL;
  e.ptr = NULL;

  if (!upb_encode_message(&e, msg, m, size)) {
    *size = 0;
    return NULL;
  }

  *size = e.limit - e.ptr;

  if (*size == 0) {
    static char ch;
    return &ch;
  } else {
    UPB_ASSERT(e.ptr);
    return e.ptr;
  }
}

#undef CHK
