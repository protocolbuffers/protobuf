// Fast decoder: ~3x the speed of decode.c, but x86-64 specific.
// Also the table size grows by 2x.
//
// Could potentially be ported to ARM64 or other 64-bit archs that pass at
// least six arguments in registers.

#include "upb/decode_fast.h"

#include "upb/decode.int.h"

/* Must be last. */
#include "upb/port_def.inc"

// The standard set of arguments passed to each parsing function.
// Thanks to x86-64 calling conventions, these will stay in registers.
#define UPB_PARSE_PARAMS                                                      \
  upb_decstate *d, const char *ptr, upb_msg *msg, const upb_msglayout *table, \
      uint64_t hasbits, uint64_t data

#define UPB_PARSE_ARGS d, ptr, msg, table, hasbits, data

#define RETURN_GENERIC(m)  \
  /* fprintf(stderr, m); */ \
  return fastdecode_generic(d, ptr, msg, table, hasbits, 0);

typedef enum {
  CARD_s = 0,  /* Singular (optional, non-repeated) */
  CARD_o = 1,  /* Oneof */
  CARD_r = 2   /* Repeated */
} upb_card;

UPB_INLINE
upb_msg *decode_newmsg_ceil(upb_decstate *d, const upb_msglayout *l,
                            int msg_ceil_bytes) {
  size_t size = l->size + sizeof(upb_msg_internal);
  char *msg_data;
  if (UPB_LIKELY(msg_ceil_bytes > 0 && _upb_arenahas(&d->arena, msg_ceil_bytes))) {
    UPB_ASSERT(size <= (size_t)msg_ceil_bytes);
    msg_data = d->arena.head.ptr;
    d->arena.head.ptr += size;
    UPB_UNPOISON_MEMORY_REGION(msg_data, msg_ceil_bytes);
    memset(msg_data, 0, msg_ceil_bytes);
    UPB_POISON_MEMORY_REGION(msg_data + size, msg_ceil_bytes - size);
  } else {
    msg_data = (char*)upb_arena_malloc(&d->arena, size);
    memset(msg_data, 0, size);
  }
  return msg_data + sizeof(upb_msg_internal);
}

UPB_FORCEINLINE
static const char *fastdecode_tagdispatch(upb_decstate *d, const char *ptr,
                                          upb_msg *msg,
                                          const upb_msglayout *table,
                                          uint64_t hasbits, uint32_t tag) {
  // Get 5 bits of field number (we pretend the continuation bit is a data bit,
  // speculating that the second byte, if any, will be 0x01).
  size_t idx = (tag & 0xf8) >> 3;

  // Xor the actual tag with the expected tag (in the low bytes of the table)
  // so that the field parser can verify the tag by comparing with zero.
  uint64_t data = table->fasttable[idx].field_data ^ tag;

  // Jump to the specialized field parser function.
  return table->fasttable[idx].field_parser(UPB_PARSE_ARGS);
}

UPB_FORCEINLINE
static uint32_t fastdecode_loadtag(const char *ptr) {
  uint16_t tag;
  memcpy(&tag, ptr, 2);
  return tag;
}

UPB_NOINLINE
static const char *fastdecode_isdonefallback(upb_decstate *d, const char *ptr,
                                             upb_msg *msg,
                                             const upb_msglayout *table,
                                             uint64_t hasbits, int overrun) {
  ptr = decode_isdonefallback_inl(d, ptr, overrun);
  if (ptr == NULL) {
    return fastdecode_err(d);
  }
  uint16_t tag = fastdecode_loadtag(ptr);
  return fastdecode_tagdispatch(d, ptr, msg, table, hasbits, tag);
}

UPB_FORCEINLINE
const char *fastdecode_dispatch(upb_decstate *d, const char *ptr, upb_msg *msg,
                                const upb_msglayout *table, uint64_t hasbits) {
  if (UPB_UNLIKELY(ptr >= d->limit_ptr)) {
    int overrun = ptr - d->end;
    if (UPB_LIKELY(overrun == d->limit)) {
      // Parse is finished.
      *(uint32_t*)msg |= hasbits >> 16;  // Sync hasbits.
      return ptr;
    } else {
      return fastdecode_isdonefallback(d, ptr, msg, table, hasbits, overrun);
    }
  }

  // Read two bytes of tag data (for a one-byte tag, the high byte is junk).
  uint16_t tag = fastdecode_loadtag(ptr);
  return fastdecode_tagdispatch(d, ptr, msg, table, hasbits, tag);
}

UPB_FORCEINLINE
static bool fastdecode_checktag(uint64_t data, int tagbytes) {
  if (tagbytes == 1) {
    return (data & 0xff) == 0;
  } else {
    return (data & 0xffff) == 0;
  }
}

UPB_FORCEINLINE
static void *fastdecode_getfield_ofs(upb_decstate *d, const char *ptr,
                                     upb_msg *msg, uint64_t *data,
                                     uint64_t *hasbits, upb_array **outarr,
                                     void **end, int valbytes,
                                     upb_card card, bool hasbit_is_idx) {
  size_t ofs = *data >> 48;
  void *field = (char *)msg + ofs;

  switch (card) {
    case CARD_s:
      // Set hasbit and return pointer to scalar field.
      if (hasbit_is_idx) {
        *hasbits |= 1ull << ((*data >> 32) & 63);
      } else {
        *hasbits |= *data;
      }
      return field;
    case CARD_r: {
      // Get pointer to upb_array and allocate/expand if necessary.
      uint8_t elem_size_lg2 = __builtin_ctz(valbytes);
      upb_array **arr_p = field;
      upb_array *arr;
      char *begin;
      *hasbits >>= 16;
      *(uint32_t*)msg |= *hasbits;
      *hasbits = 0;
      if (UPB_LIKELY(!*arr_p)) {
        arr = _upb_array_new(&d->arena, 8, elem_size_lg2);
        *arr_p = arr;
      } else {
        arr = *arr_p;
      }
      begin = _upb_array_ptr(arr);
      field = begin + (arr->len * valbytes);
      *end = begin + (arr->size * valbytes);
      *data = fastdecode_loadtag(ptr);
      *outarr = arr;
      return field;
    }
    default:
      UPB_UNREACHABLE();
  }
}

UPB_FORCEINLINE
static void *fastdecode_getfield(upb_decstate *d, const char *ptr, upb_msg *msg,
                                 uint64_t *data, uint64_t *hasbits,
                                 int valbytes, upb_card card) {
  return fastdecode_getfield_ofs(d, ptr, msg, data, hasbits, NULL, NULL,
                                 valbytes, card, false);
}

/* varint fields **************************************************************/

UPB_FORCEINLINE
static uint64_t fastdecode_munge(uint64_t val, int valbytes, bool zigzag) {
  if (valbytes == 1) {
    return val != 0;
  } else if (zigzag) {
    if (valbytes == 4) {
      uint32_t n = val;
      return (n >> 1) ^ -(int32_t)(n & 1);
    } else if (valbytes == 8) {
      return (val >> 1) ^ -(int64_t)(val & 1);
    }
    UPB_UNREACHABLE();
  }
  return val;
}

UPB_FORCEINLINE
static const char *fastdecode_varint(UPB_PARSE_PARAMS, int tagbytes,
                                     int valbytes, upb_card card, bool zigzag) {
  uint64_t val;
  void *dst;
  if (UPB_UNLIKELY(!fastdecode_checktag(data, tagbytes))) {
    RETURN_GENERIC("varint field tag mismatch\n");
  }
  dst = fastdecode_getfield(d, ptr, msg, &data, &hasbits, valbytes,
                            card);
  ptr += tagbytes + 1;
  val = (uint8_t)ptr[-1];
  if (UPB_UNLIKELY(val & 0x80)) {
    int i;
    for (i = 0; i < 8; i++) {
      ptr++;
      uint64_t byte = (uint8_t)ptr[-1];
      val += (byte - 1) << (7 + 7 * i);
      if (UPB_LIKELY((byte & 0x80) == 0)) goto done;
    }
    ptr++;
    uint64_t byte = (uint8_t)ptr[-1];
    if (byte > 1) return fastdecode_err(d);
    val += (byte - 1) << 63;
  }
done:
  val = fastdecode_munge(val, valbytes, zigzag);
  memcpy(dst, &val, valbytes);
  return fastdecode_dispatch(d, ptr, msg, table, hasbits);
}

#define z_ZZ true
#define b_ZZ false
#define v_ZZ false

/* Generate all varint functions.
 * {s,o,r} x {b1,v4,z4,v8,z8} x {1bt,2bt} */

#define F(card, type, valbytes, tagbytes)                                      \
  const char *upb_p##card##type##valbytes##_##tagbytes##bt(UPB_PARSE_PARAMS) { \
    return fastdecode_varint(UPB_PARSE_ARGS, tagbytes, valbytes, CARD_##card,  \
                             type##_ZZ);            \
  }

#define TYPES(card, tagbytes) \
  F(card, b, 1, tagbytes)     \
  F(card, v, 4, tagbytes)     \
  F(card, v, 8, tagbytes)     \
  F(card, z, 4, tagbytes)     \
  F(card, z, 8, tagbytes)

#define TAGBYTES(card) \
  TYPES(card, 1)       \
  TYPES(card, 2)

TAGBYTES(s)
TAGBYTES(o)
/* TAGBYTES(r) */

#undef z_ZZ
#undef b_ZZ
#undef v_ZZ
#undef o_ONEOF
#undef s_ONEOF
#undef r_ONEOF
#undef F
#undef TYPES
#undef TAGBYTES

/* string fields **************************************************************/

UPB_FORCEINLINE
static bool fastdecode_boundscheck(const char *ptr, size_t len,
                                   const char *end) {
  uintptr_t uptr = (uintptr_t)ptr;
  uintptr_t uend = (uintptr_t)end + 16;
  uintptr_t res = uptr + len;
  return res < uptr || res > uend;
}

typedef const char *fastdecode_copystr_func(struct upb_decstate *d,
                                            const char *ptr, upb_msg *msg,
                                            const upb_msglayout *table,
                                            uint64_t hasbits, upb_strview *dst);

UPB_FORCEINLINE
static const char *fastdecode_copystring(struct upb_decstate *d,
                                         const char *ptr, upb_msg *msg,
                                         const upb_msglayout *table,
                                         uint64_t hasbits, upb_strview *dst,
                                         int tagbytes) {
  int64_t len = (int8_t)*ptr;
  ptr++;
  if (UPB_UNLIKELY(fastdecode_boundscheck(ptr, len, d->limit_ptr))) {
    ptr -= tagbytes + 1;
    RETURN_GENERIC("string field len >1 byte\n");
  }
  char *data = upb_arena_malloc(&d->arena, len);
  if (!data) {
    return fastdecode_err(d);
  }
  memcpy(data, ptr, len);
  dst->data = data;
  dst->size = len;
  return fastdecode_dispatch(d, ptr + len, msg, table, hasbits);
}

UPB_FORCEINLINE
static const char *fastdecode_string(UPB_PARSE_PARAMS, int tagbytes,
                                     upb_card card, fastdecode_copystr_func *func) {
  upb_strview *dst;
  int64_t len;
  if (UPB_UNLIKELY(!fastdecode_checktag(data, tagbytes))) {
    RETURN_GENERIC("string field tag mismatch\n");
  }

  dst = fastdecode_getfield(d, ptr, msg, &data, &hasbits,
                            sizeof(upb_strview), card);
  if (UPB_UNLIKELY(!d->alias)) {
    len = (uint8_t)ptr[tagbytes];
    if (UPB_UNLIKELY(len > 15 - tagbytes || !_upb_arenahas(&d->arena, 16))) {
      return func(d, ptr + tagbytes, msg, table, hasbits, dst);
    }
    char *data = d->arena.head.ptr;
    d->arena.head.ptr += 16;
    UPB_UNPOISON_MEMORY_REGION(data, 16);
    memcpy(data, ptr, 16);
    UPB_ASSERT(tagbytes + 1 + len <= 16);
    ptr += tagbytes + 1;
    dst->data = data + tagbytes + 1;
    dst->size = len;
    UPB_POISON_MEMORY_REGION(data, 1);
    UPB_POISON_MEMORY_REGION(data + 1 + len, 16 - len - 1);
    return fastdecode_dispatch(d, ptr + len, msg, table, hasbits);
  }

  len = (int8_t)ptr[tagbytes];
  ptr += tagbytes + 1;
  dst->data = ptr;
  dst->size = len;
  if (UPB_UNLIKELY(fastdecode_boundscheck(ptr, len, d->limit_ptr))) {
    dst->size = 0;
    ptr -= tagbytes + 1;
    RETURN_GENERIC("string field len >1 byte\n");
  }
  return fastdecode_dispatch(d, ptr + len, msg, table, hasbits);
}

UPB_NOINLINE
static const char *upb_copystr_1bt(upb_decstate *d, const char *ptr,
                                   upb_msg *msg, const upb_msglayout *table,
                                   uint64_t hasbits, upb_strview *dst) {
  return fastdecode_copystring(d, ptr, msg, table, hasbits, dst, 1);
}

UPB_NOINLINE
static const char *upb_copystr_2bt(upb_decstate *d, const char *ptr, upb_msg *msg,
                            const upb_msglayout *table, uint64_t hasbits,
                            upb_strview *dst) {
  return fastdecode_copystring(d, ptr, msg, table, hasbits, dst, 2);
}

const char *upb_pss_1bt(UPB_PARSE_PARAMS) {
  return fastdecode_string(UPB_PARSE_ARGS, 1, CARD_s, &upb_copystr_1bt);
}

const char *upb_pos_1bt(UPB_PARSE_PARAMS) {
  return fastdecode_string(UPB_PARSE_ARGS, 1, CARD_o, &upb_copystr_1bt);
}

const char *upb_pss_2bt(UPB_PARSE_PARAMS) {
  return fastdecode_string(UPB_PARSE_ARGS, 2, CARD_s, &upb_copystr_2bt);
}

const char *upb_pos_2bt(UPB_PARSE_PARAMS) {
  return fastdecode_string(UPB_PARSE_ARGS, 2, CARD_o, &upb_copystr_2bt);
}

/* message fields *************************************************************/

UPB_FORCEINLINE
static bool fastdecode_boundscheck2(const char *ptr, size_t len,
                                   const char *end) {
  uintptr_t uptr = (uintptr_t)ptr;
  uintptr_t uend = (uintptr_t)end;
  uintptr_t res = uptr + len;
  return res < uptr || res > uend;
}

UPB_FORCEINLINE
static const char *fastdecode_submsg(UPB_PARSE_PARAMS, int tagbytes,
                                     int msg_ceil_bytes, upb_card card) {

  if (UPB_UNLIKELY(!fastdecode_checktag(data, tagbytes))) {
    RETURN_GENERIC("submessage field tag mismatch\n");
  }

  if (--d->depth == 0) return fastdecode_err(d);

  upb_msg **submsg;
  upb_array *arr;
  void *end;
  uint32_t submsg_idx = data;
  submsg_idx >>= 16;
  const upb_msglayout *subl = table->submsgs[submsg_idx];
  submsg = fastdecode_getfield_ofs(d, ptr, msg, &data, &hasbits, &arr, &end,
                                   sizeof(upb_msg *), card, true);

  if (card == CARD_s) {
    *(uint32_t*)msg |= hasbits >> 16;
    hasbits = 0;
  }

again:
  if (card == CARD_r) {
    if (UPB_UNLIKELY(submsg == end)) {
      size_t old_size = arr->size;
      size_t old_bytes = old_size * sizeof(upb_msg*);
      size_t new_size = old_size * 2;
      size_t new_bytes = new_size * sizeof(upb_msg*);
      char *old_ptr = _upb_array_ptr(arr);
      char *new_ptr = upb_arena_realloc(&d->arena, old_ptr, old_bytes, new_bytes);
      arr->size = new_size;
      arr->data = _upb_array_tagptr(new_ptr, 3);
      submsg = (void*)(new_ptr + (old_size * sizeof(upb_msg*)));
      end = (void*)(new_ptr + (new_size * sizeof(upb_msg*)));
    }
  }

  upb_msg* child = *submsg;

  if (card == CARD_r || UPB_LIKELY(!child)) {
    *submsg = child = decode_newmsg_ceil(d, subl, msg_ceil_bytes);
  }

  ptr += tagbytes + 1;
  int64_t len = (int8_t)ptr[-1];
  if (fastdecode_boundscheck2(ptr, len, d->limit_ptr)) {
    if (UPB_UNLIKELY(len & 0x80)) {
      len &= 0xff;
      int i;
      for (i = 0; i < 3; i++) {
        ptr++;
        size_t byte = (uint8_t)ptr[-1];
        len += (byte - 1) << (7 + 7 * i);
        if (UPB_LIKELY((byte & 0x80) == 0)) goto done;
      }
      ptr++;
      size_t byte = (uint8_t)ptr[-1];
      // len is limited by 2gb not 4gb, hence 8 and not 16 as normally expected
      // for a 32 bit varint.
      if (UPB_UNLIKELY(byte >= 8)) return fastdecode_err(d);
      len += (byte - 1) << 28;
    }
  done:
    if (ptr - d->end + (int)len > d->limit) {
      return fastdecode_err(d);
    }
    int delta = decode_pushlimit(d, ptr, len);
    ptr = fastdecode_dispatch(d, ptr, child, subl, 0);
    decode_poplimit(d, delta);
  } else {
    UPB_ASSERT(d->limit_ptr - ptr >= len);
    UPB_ASSERT(d->limit_ptr == d->end + UPB_MIN(0, d->limit));
    const char *saved_limit_ptr = d->limit_ptr;
    int saved_limit = d->limit;
    d->limit_ptr = ptr + len;
    d->limit = d->limit_ptr - d->end;
    UPB_ASSERT(d->limit_ptr == d->end + UPB_MIN(0, d->limit));
    ptr = fastdecode_dispatch(d, ptr, child, subl, 0);
    d->limit_ptr = saved_limit_ptr;
    d->limit = saved_limit;
    UPB_ASSERT(d->limit_ptr == d->end + UPB_MIN(0, d->limit));
  }
  if (UPB_UNLIKELY(d->end_group != 0)) {
    return fastdecode_err(d);
  }

  if (card == CARD_r) {
    submsg++;
    if (UPB_LIKELY(!decode_isdone(d, &ptr))) {
      uint32_t tag = fastdecode_loadtag(ptr);
      if (tagbytes == 1) {
        if ((uint8_t)tag == (uint8_t)data) goto again;
      } else {
        if ((uint16_t)tag == (uint16_t)data) goto again;
      }
      arr->len = submsg - (upb_msg**)_upb_array_ptr(arr);
      d->depth++;
      return fastdecode_tagdispatch(d, ptr, msg, table, hasbits, tag);
    } else {
      arr->len = submsg - (upb_msg**)_upb_array_ptr(arr);
      d->depth++;
      return ptr;
    }
  }

  d->depth++;
  return fastdecode_dispatch(d, ptr, msg, table, hasbits);
}

#define F(card, tagbytes, size_ceil, ceil_arg)                                 \
  const char *upb_p##card##m_##tagbytes##bt_max##size_ceil##b(                 \
      UPB_PARSE_PARAMS) {                                                      \
    return fastdecode_submsg(UPB_PARSE_ARGS, tagbytes, ceil_arg, CARD_##card); \
  }

#define SIZES(card, tagbytes) \
  F(card, tagbytes, 64, 64) \
  F(card, tagbytes, 128, 128) \
  F(card, tagbytes, 192, 192) \
  F(card, tagbytes, 256, 256) \
  F(card, tagbytes, max, -1)

#define TAGBYTES(card) \
  SIZES(card, 1) \
  SIZES(card, 2)

TAGBYTES(s)
TAGBYTES(o)
TAGBYTES(r)

#undef TAGBYTES
#undef SIZES
#undef F
