/* We encode backwards, to avoid pre-computing lengths (one-pass encode). */

#include "upb/encode.h"

#include <setjmp.h>
#include <string.h>

#include "upb/msg.h"
#include "upb/upb.h"

/* Must be last. */
#include "upb/port_def.inc"

#define UPB_PB_VARINT_MAX_LEN 10

UPB_NOINLINE
static size_t encode_varint64(uint64_t val, char *buf) {
  size_t i = 0;
  do {
    uint8_t byte = val & 0x7fU;
    val >>= 7;
    if (val) byte |= 0x80U;
    buf[i++] = byte;
  } while (val);
  return i;
}

static uint32_t encode_zz32(int32_t n) { return ((uint32_t)n << 1) ^ (n >> 31); }
static uint64_t encode_zz64(int64_t n) { return ((uint64_t)n << 1) ^ (n >> 63); }

typedef struct {
  jmp_buf err;
  upb_alloc *alloc;
  char *buf, *ptr, *limit;
  int options;
  int depth;
  _upb_mapsorter sorter;
} upb_encstate;

static size_t upb_roundup_pow2(size_t bytes) {
  size_t ret = 128;
  while (ret < bytes) {
    ret *= 2;
  }
  return ret;
}

UPB_NORETURN static void encode_err(upb_encstate *e) {
  UPB_LONGJMP(e->err, 1);
}

UPB_NOINLINE
static void encode_growbuffer(upb_encstate *e, size_t bytes) {
  size_t old_size = e->limit - e->buf;
  size_t new_size = upb_roundup_pow2(bytes + (e->limit - e->ptr));
  char *new_buf = upb_realloc(e->alloc, e->buf, old_size, new_size);

  if (!new_buf) encode_err(e);

  /* We want previous data at the end, realloc() put it at the beginning. */
  if (old_size > 0) {
    memmove(new_buf + new_size - old_size, e->buf, old_size);
  }

  e->ptr = new_buf + new_size - (e->limit - e->ptr);
  e->limit = new_buf + new_size;
  e->buf = new_buf;

  e->ptr -= bytes;
}

/* Call to ensure that at least "bytes" bytes are available for writing at
 * e->ptr.  Returns false if the bytes could not be allocated. */
UPB_FORCEINLINE
static void encode_reserve(upb_encstate *e, size_t bytes) {
  if ((size_t)(e->ptr - e->buf) < bytes) {
    encode_growbuffer(e, bytes);
    return;
  }

  e->ptr -= bytes;
}

/* Writes the given bytes to the buffer, handling reserve/advance. */
static void encode_bytes(upb_encstate *e, const void *data, size_t len) {
  if (len == 0) return;  /* memcpy() with zero size is UB */
  encode_reserve(e, len);
  memcpy(e->ptr, data, len);
}

static void encode_fixed64(upb_encstate *e, uint64_t val) {
  val = _upb_be_swap64(val);
  encode_bytes(e, &val, sizeof(uint64_t));
}

static void encode_fixed32(upb_encstate *e, uint32_t val) {
  val = _upb_be_swap32(val);
  encode_bytes(e, &val, sizeof(uint32_t));
}

UPB_NOINLINE
static void encode_longvarint(upb_encstate *e, uint64_t val) {
  size_t len;
  char *start;

  encode_reserve(e, UPB_PB_VARINT_MAX_LEN);
  len = encode_varint64(val, e->ptr);
  start = e->ptr + UPB_PB_VARINT_MAX_LEN - len;
  memmove(start, e->ptr, len);
  e->ptr = start;
}

UPB_FORCEINLINE
static void encode_varint(upb_encstate *e, uint64_t val) {
  if (val < 128 && e->ptr != e->buf) {
    --e->ptr;
    *e->ptr = val;
  } else {
    encode_longvarint(e, val);
  }
}

static void encode_double(upb_encstate *e, double d) {
  uint64_t u64;
  UPB_ASSERT(sizeof(double) == sizeof(uint64_t));
  memcpy(&u64, &d, sizeof(uint64_t));
  encode_fixed64(e, u64);
}

static void encode_float(upb_encstate *e, float d) {
  uint32_t u32;
  UPB_ASSERT(sizeof(float) == sizeof(uint32_t));
  memcpy(&u32, &d, sizeof(uint32_t));
  encode_fixed32(e, u32);
}

static void encode_tag(upb_encstate *e, uint32_t field_number,
                       uint8_t wire_type) {
  encode_varint(e, (field_number << 3) | wire_type);
}

static void encode_fixedarray(upb_encstate *e, const upb_array *arr,
                               size_t elem_size, uint32_t tag) {
  size_t bytes = arr->len * elem_size;
  const char* data = _upb_array_constptr(arr);
  const char* ptr = data + bytes - elem_size;
  if (tag) {
    while (true) {
      encode_bytes(e, ptr, elem_size);
      encode_varint(e, tag);
      if (ptr == data) break;
      ptr -= elem_size;
    }
  } else {
    encode_bytes(e, data, bytes);
  }
}

static void encode_message(upb_encstate *e, const char *msg,
                           const upb_msglayout *m, size_t *size);

static void encode_scalar(upb_encstate *e, const void *_field_mem,
                          const upb_msglayout *m, const upb_msglayout_field *f,
                          bool skip_zero_value) {
  const char *field_mem = _field_mem;
  int wire_type;

#define CASE(ctype, type, wtype, encodeval) \
  {                                         \
    ctype val = *(ctype *)field_mem;        \
    if (skip_zero_value && val == 0) {      \
      return;                               \
    }                                       \
    encode_##type(e, encodeval);            \
    wire_type = wtype;                      \
    break;                                  \
  }

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
      CASE(int32_t, varint, UPB_WIRE_TYPE_VARINT, encode_zz32(val));
    case UPB_DESCRIPTOR_TYPE_SINT64:
      CASE(int64_t, varint, UPB_WIRE_TYPE_VARINT, encode_zz64(val));
    case UPB_DESCRIPTOR_TYPE_STRING:
    case UPB_DESCRIPTOR_TYPE_BYTES: {
      upb_strview view = *(upb_strview*)field_mem;
      if (skip_zero_value && view.size == 0) {
        return;
      }
      encode_bytes(e, view.data, view.size);
      encode_varint(e, view.size);
      wire_type = UPB_WIRE_TYPE_DELIMITED;
      break;
    }
    case UPB_DESCRIPTOR_TYPE_GROUP: {
      size_t size;
      void *submsg = *(void **)field_mem;
      const upb_msglayout *subm = m->submsgs[f->submsg_index];
      if (submsg == NULL) {
        return;
      }
      if (--e->depth == 0) encode_err(e);
      encode_tag(e, f->number, UPB_WIRE_TYPE_END_GROUP);
      encode_message(e, submsg, subm, &size);
      wire_type = UPB_WIRE_TYPE_START_GROUP;
      e->depth++;
      break;
    }
    case UPB_DESCRIPTOR_TYPE_MESSAGE: {
      size_t size;
      void *submsg = *(void **)field_mem;
      const upb_msglayout *subm = m->submsgs[f->submsg_index];
      if (submsg == NULL) {
        return;
      }
      if (--e->depth == 0) encode_err(e);
      encode_message(e, submsg, subm, &size);
      encode_varint(e, size);
      wire_type = UPB_WIRE_TYPE_DELIMITED;
      e->depth++;
      break;
    }
    default:
      UPB_UNREACHABLE();
  }
#undef CASE

  encode_tag(e, f->number, wire_type);
}

static void encode_array(upb_encstate *e, const char *field_mem,
                         const upb_msglayout *m, const upb_msglayout_field *f) {
  const upb_array *arr = *(const upb_array**)field_mem;
  bool packed = f->label == _UPB_LABEL_PACKED;
  size_t pre_len = e->limit - e->ptr;

  if (arr == NULL || arr->len == 0) {
    return;
  }

#define VARINT_CASE(ctype, encode)                                       \
  {                                                                      \
    const ctype *start = _upb_array_constptr(arr);                       \
    const ctype *ptr = start + arr->len;                                 \
    uint32_t tag = packed ? 0 : (f->number << 3) | UPB_WIRE_TYPE_VARINT; \
    do {                                                                 \
      ptr--;                                                             \
      encode_varint(e, encode);                                          \
      if (tag) encode_varint(e, tag);                                    \
    } while (ptr != start);                                              \
  }                                                                      \
  break;

#define TAG(wire_type) (packed ? 0 : (f->number << 3 | wire_type))

  switch (f->descriptortype) {
    case UPB_DESCRIPTOR_TYPE_DOUBLE:
      encode_fixedarray(e, arr, sizeof(double), TAG(UPB_WIRE_TYPE_64BIT));
      break;
    case UPB_DESCRIPTOR_TYPE_FLOAT:
      encode_fixedarray(e, arr, sizeof(float), TAG(UPB_WIRE_TYPE_32BIT));
      break;
    case UPB_DESCRIPTOR_TYPE_SFIXED64:
    case UPB_DESCRIPTOR_TYPE_FIXED64:
      encode_fixedarray(e, arr, sizeof(uint64_t), TAG(UPB_WIRE_TYPE_64BIT));
      break;
    case UPB_DESCRIPTOR_TYPE_FIXED32:
    case UPB_DESCRIPTOR_TYPE_SFIXED32:
      encode_fixedarray(e, arr, sizeof(uint32_t), TAG(UPB_WIRE_TYPE_32BIT));
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
      VARINT_CASE(int32_t, encode_zz32(*ptr));
    case UPB_DESCRIPTOR_TYPE_SINT64:
      VARINT_CASE(int64_t, encode_zz64(*ptr));
    case UPB_DESCRIPTOR_TYPE_STRING:
    case UPB_DESCRIPTOR_TYPE_BYTES: {
      const upb_strview *start = _upb_array_constptr(arr);
      const upb_strview *ptr = start + arr->len;
      do {
        ptr--;
        encode_bytes(e, ptr->data, ptr->size);
        encode_varint(e, ptr->size);
        encode_tag(e, f->number, UPB_WIRE_TYPE_DELIMITED);
      } while (ptr != start);
      return;
    }
    case UPB_DESCRIPTOR_TYPE_GROUP: {
      const void *const*start = _upb_array_constptr(arr);
      const void *const*ptr = start + arr->len;
      const upb_msglayout *subm = m->submsgs[f->submsg_index];
      if (--e->depth == 0) encode_err(e);
      do {
        size_t size;
        ptr--;
        encode_tag(e, f->number, UPB_WIRE_TYPE_END_GROUP);
        encode_message(e, *ptr, subm, &size);
        encode_tag(e, f->number, UPB_WIRE_TYPE_START_GROUP);
      } while (ptr != start);
      e->depth++;
      return;
    }
    case UPB_DESCRIPTOR_TYPE_MESSAGE: {
      const void *const*start = _upb_array_constptr(arr);
      const void *const*ptr = start + arr->len;
      const upb_msglayout *subm = m->submsgs[f->submsg_index];
      if (--e->depth == 0) encode_err(e);
      do {
        size_t size;
        ptr--;
        encode_message(e, *ptr, subm, &size);
        encode_varint(e, size);
        encode_tag(e, f->number, UPB_WIRE_TYPE_DELIMITED);
      } while (ptr != start);
      e->depth++;
      return;
    }
  }
#undef VARINT_CASE

  if (packed) {
    encode_varint(e, e->limit - e->ptr - pre_len);
    encode_tag(e, f->number, UPB_WIRE_TYPE_DELIMITED);
  }
}

static void encode_mapentry(upb_encstate *e, uint32_t number,
                            const upb_msglayout *layout,
                            const upb_map_entry *ent) {
  const upb_msglayout_field *key_field = &layout->fields[0];
  const upb_msglayout_field *val_field = &layout->fields[1];
  size_t pre_len = e->limit - e->ptr;
  size_t size;
  encode_scalar(e, &ent->v, layout, val_field, false);
  encode_scalar(e, &ent->k, layout, key_field, false);
  size = (e->limit - e->ptr) - pre_len;
  encode_varint(e, size);
  encode_tag(e, number, UPB_WIRE_TYPE_DELIMITED);
}

static void encode_map(upb_encstate *e, const char *field_mem,
                       const upb_msglayout *m, const upb_msglayout_field *f) {
  const upb_map *map = *(const upb_map**)field_mem;
  const upb_msglayout *layout = m->submsgs[f->submsg_index];
  UPB_ASSERT(layout->field_count == 2);

  if (map == NULL) return;

  if (e->options & UPB_ENCODE_DETERMINISTIC) {
    _upb_sortedmap sorted;
    _upb_mapsorter_pushmap(&e->sorter, layout->fields[0].descriptortype, map,
                           &sorted);
    upb_map_entry ent;
    while (_upb_sortedmap_next(&e->sorter, map, &sorted, &ent)) {
      encode_mapentry(e, f->number, layout, &ent);
    }
    _upb_mapsorter_popmap(&e->sorter, &sorted);
  } else {
    upb_strtable_iter i;
    upb_strtable_begin(&i, &map->table);
    for(; !upb_strtable_done(&i); upb_strtable_next(&i)) {
      upb_strview key = upb_strtable_iter_key(&i);
      const upb_value val = upb_strtable_iter_value(&i);
      upb_map_entry ent;
      _upb_map_fromkey(key, &ent.k, map->key_size);
      _upb_map_fromvalue(val, &ent.v, map->val_size);
      encode_mapentry(e, f->number, layout, &ent);
    }
  }
}

static void encode_scalarfield(upb_encstate *e, const char *msg,
                               const upb_msglayout *m,
                               const upb_msglayout_field *f) {
  bool skip_empty = false;
  if (f->presence == 0) {
    /* Proto3 presence. */
    skip_empty = true;
  } else if (f->presence > 0) {
    /* Proto2 presence: hasbit. */
    if (!_upb_hasbit_field(msg, f)) return;
  } else {
    /* Field is in a oneof. */
    if (_upb_getoneofcase_field(msg, f) != f->number) return;
  }
  encode_scalar(e, msg + f->offset, m, f, skip_empty);
}

static void encode_message(upb_encstate *e, const char *msg,
                           const upb_msglayout *m, size_t *size) {
  size_t pre_len = e->limit - e->ptr;
  const upb_msglayout_field *f = &m->fields[m->field_count];
  const upb_msglayout_field *first = &m->fields[0];

  if ((e->options & UPB_ENCODE_SKIPUNKNOWN) == 0) {
    size_t unknown_size;
    const char *unknown = upb_msg_getunknown(msg, &unknown_size);

    if (unknown) {
      encode_bytes(e, unknown, unknown_size);
    }
  }

  while (f != first) {
    f--;
    if (_upb_isrepeated(f)) {
      encode_array(e, msg + f->offset, m, f);
    } else if (f->label == _UPB_LABEL_MAP) {
      encode_map(e, msg + f->offset, m, f);
    } else {
      encode_scalarfield(e, msg, m, f);
    }
  }

  *size = (e->limit - e->ptr) - pre_len;
}

char *upb_encode_ex(const void *msg, const upb_msglayout *m, int options,
                    upb_arena *arena, size_t *size) {
  upb_encstate e;
  unsigned depth = (unsigned)options >> 16;

  e.alloc = upb_arena_alloc(arena);
  e.buf = NULL;
  e.limit = NULL;
  e.ptr = NULL;
  e.depth = depth ? depth : 64;
  e.options = options;
  _upb_mapsorter_init(&e.sorter);
  char *ret = NULL;

  if (UPB_SETJMP(e.err)) {
    *size = 0;
    ret = NULL;
  } else {
    encode_message(&e, msg, m, size);
    *size = e.limit - e.ptr;
    if (*size == 0) {
      static char ch;
      ret = &ch;
    } else {
      UPB_ASSERT(e.ptr);
      ret = e.ptr;
    }
  }

  _upb_mapsorter_destroy(&e.sorter);
  return ret;
}
