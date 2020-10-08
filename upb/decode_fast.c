
#include "upb/decode.h"

#include "upb/port_def.inc"

#define UPB_PARSE_PARAMS                                                      \
  upb_decstate *d, const char *ptr, upb_msg *msg, const upb_msglayout *table, \
      uint64_t hasbits, uint64_t data

#define UPB_PARSE_ARGS d, ptr, msg, table, hasbits, data

#define RETURN_GENERIC(msg)  \
  /* fprintf(stderr, msg); */ \
  return fastdecode_generic(UPB_PARSE_ARGS);

typedef enum {
  CARD_s = 0,
  CARD_o = 1,
  CARD_r = 2,
  CARD_p = 3
} upb_card;

UPB_NOINLINE
const char *fastdecode_dispatch(upb_decstate *d, const char *ptr, upb_msg *msg,
                                const upb_msglayout *table, uint64_t hasbits) {
  uint16_t tag;
  uint64_t data = 0;;
  size_t idx;
  if (UPB_UNLIKELY(ptr >= d->fastlimit)) {
    if (UPB_LIKELY(ptr == d->limit)) {
      *(uint32_t*)msg |= hasbits >> 16;  /* Sync hasbits. */
      return ptr;
    }
    RETURN_GENERIC("dispatch hit end\n");
  }
  memcpy(&tag, ptr, 2);
  idx = (tag & 0xf8) >> 3;
  data = table->field_data[idx] ^ tag;
  return table->field_parser[idx](UPB_PARSE_ARGS);
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
static uint16_t fastdecode_readtag(const char *ptr, int tagbytes) {
  uint16_t ret = 0;
  memcpy(&ret, ptr, tagbytes);
  return ret;
}

UPB_FORCEINLINE
static void *fastdecode_getfield_ofs(upb_decstate *d, const char *ptr,
                                     upb_msg *msg, size_t ofs, uint64_t *data,
                                     uint64_t *hasbits, upb_array **outarr,
                                     void **end, int tagbytes, int valbytes,
                                     upb_card card) {
  void *field = (char *)msg + ofs;

  switch (card) {
    case CARD_s:
      *hasbits |= *data;
      return field;
    case CARD_o: {
      uint32_t *case_ptr = UPB_PTR_AT(msg, (*data >> 16) & 0xffff, uint32_t);
      *case_ptr = (*data >> 32) & 0xffff;
      return field;
    }
    case CARD_r: {
      uint8_t elem_size_lg2 = __builtin_ctz(valbytes);
      upb_array **arr_p = field;
      upb_array *arr;
      uint16_t expected_tag;
      *hasbits >>= 16;
      *(uint32_t*)msg |= *hasbits;
      *hasbits = 0;
      if (UPB_LIKELY(!*arr_p)) {
        const size_t initial_len = 32;
        size_t need = (valbytes * initial_len) + sizeof(upb_array);
        if (UPB_UNLIKELY((size_t)(d->arena_end - d->arena_ptr) < need)) {
          *outarr = NULL;
          *data = 0;
          *end = NULL;
          return NULL;
        }
        arr = (void*)d->arena_ptr;
        field = arr + 1;
        arr->data = _upb_array_tagptr(field, elem_size_lg2);
        *arr_p = arr;
        arr->size = initial_len;
        *end = (char*)field + (arr->size * valbytes);
        d->arena_ptr += need;
      } else {
        arr = *arr_p;
        field = _upb_array_ptr(arr);
        *end = (char*)field + (arr->size * valbytes);
        field = (char*)field + (arr->len * valbytes);
      }
      expected_tag = fastdecode_readtag(ptr, tagbytes);
      *data = expected_tag;
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
                                 int tagbytes, int valbytes, upb_card card) {
  return fastdecode_getfield_ofs(d, ptr, msg, *data >> 48, data, hasbits, NULL,
                                 NULL, tagbytes, valbytes, card);
}

/* varint fields **************************************************************/

#ifdef __BMI2__
#include <immintrin.h>
#endif

UPB_FORCEINLINE uint64_t fastdecode_munge(uint64_t val, int valbytes, bool zigzag) {
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
static int fastdecode_varintlen(uint64_t data64) {
  uint64_t clear_bits = ~data64 & 0x8080808080808080;
  if (clear_bits == 0) return -1;
  return __builtin_ctzl(clear_bits) / 8 + 1;
}

UPB_FORCEINLINE
static const char *fastdecode_longvarint(UPB_PARSE_PARAMS, int valbytes,
                                         int varintbytes, bool zigzag) {
  uint64_t val = data >> 18;
  size_t ofs = (uint16_t)data;
  uint64_t data64;
  int sawbytes;
  memcpy(&data64, ptr + 2, 8);
  sawbytes = fastdecode_varintlen(data64) + 2;
  UPB_ASSERT(sawbytes == varintbytes);
#ifdef __BMI2__
  if (varintbytes != 3) {
    uint64_t mask = 0x7f7f7f7f7f7f7f7f >> (8 * (10  - varintbytes));
    val |= _pext_u64(data64, mask) << 14;
  } else
#endif
  {
    for (int i = 2; i < varintbytes; i++) {
      uint64_t byte = ptr[i];
      if (i != varintbytes - 1) byte &= 0x7f;
      val |= byte << (7 * i);
    }
  }
  val = fastdecode_munge(val, valbytes, zigzag);
  memcpy((char*)msg + ofs, &val, valbytes);
  return fastdecode_dispatch(d, ptr + varintbytes, msg, table, hasbits);
}

UPB_FORCEINLINE
static const char *fastdecode_longvarintjmp(UPB_PARSE_PARAMS,
                                            _upb_field_parser **funcs) {
  int len;
  uint64_t data64;
  memcpy(&data64, ptr + 2, 8);
  len = fastdecode_varintlen(data64);
  if (len < 0) return fastdecode_err(d);
  return funcs[len - 1](UPB_PARSE_ARGS);
}

UPB_FORCEINLINE
static const char *fastdecode_varint(UPB_PARSE_PARAMS, int tagbytes,
                                     int valbytes, upb_card card, bool zigzag,
                                     _upb_field_parser **funcs) {
  uint64_t val = 0;
  void *dst;
  if (UPB_UNLIKELY(!fastdecode_checktag(data, tagbytes))) {
    RETURN_GENERIC("varint field tag mismatch\n");
  }
  dst = fastdecode_getfield(d, ptr, msg, &data, &hasbits, tagbytes, valbytes,
                            card);
  val = (uint8_t)ptr[tagbytes];
  if (UPB_UNLIKELY(val & 0x80)) {
    uint32_t byte = (uint8_t)ptr[tagbytes + 1];
    val += (byte - 1) << 7;
    if (UPB_UNLIKELY(byte & 0x80)) {
      ptr += tagbytes;
      data = (uint32_t)(val << 18 | data >> 48);
      return fastdecode_longvarintjmp(UPB_PARSE_ARGS, funcs);
    }
    ptr += tagbytes + 2;
  } else {
    ptr += tagbytes + 1;
  }
  val = fastdecode_munge(val, valbytes, zigzag);
  memcpy(dst, &val, valbytes);
  return fastdecode_dispatch(d, ptr, msg, table, hasbits);
}

#define z_ZZ true
#define b_ZZ false
#define v_ZZ false

// Generate varint vallbacks.

#define FUNCNAME(type, valbytes, varintbytes) \
  upb_pl##type##valbytes##_##varintbytes##bv

#define TABLENAME(type, valbytes) \
  upb_pl##type##valbytes##_table

#define F(type, valbytes, varintbytes)                                         \
  static const char *FUNCNAME(type, valbytes, varintbytes)(UPB_PARSE_PARAMS) { \
    return fastdecode_longvarint(UPB_PARSE_ARGS, valbytes, varintbytes,        \
                                 type##_ZZ);                                   \
  }

#define FALLBACKS(type, valbytes)                                 \
  F(type, valbytes, 3)                                            \
  F(type, valbytes, 4)                                            \
  F(type, valbytes, 5)                                            \
  F(type, valbytes, 6)                                            \
  F(type, valbytes, 7)                                            \
  F(type, valbytes, 8)                                            \
  F(type, valbytes, 9)                                            \
  F(type, valbytes, 10)                                           \
  static _upb_field_parser *TABLENAME(type, valbytes)[8] = {      \
      &FUNCNAME(type, valbytes, 3), &FUNCNAME(type, valbytes, 4), \
      &FUNCNAME(type, valbytes, 5), &FUNCNAME(type, valbytes, 6), \
      &FUNCNAME(type, valbytes, 7), &FUNCNAME(type, valbytes, 8), \
      &FUNCNAME(type, valbytes, 9), &FUNCNAME(type, valbytes, 10)};

FALLBACKS(b, 1)
FALLBACKS(v, 4)
FALLBACKS(v, 8)
FALLBACKS(z, 4)
FALLBACKS(z, 8)

#undef F
#undef FALLBACKS
#undef FUNCNAME

// Generate all varint functions.
// {s,o,r} x {b1,v4,z4,v8,z8} x {1bt,2bt}

#define F(card, type, valbytes, tagbytes)                                      \
  const char *upb_p##card##type##valbytes##_##tagbytes##bt(UPB_PARSE_PARAMS) { \
    return fastdecode_varint(UPB_PARSE_ARGS, tagbytes, valbytes, CARD_##card,  \
                             type##_ZZ, TABLENAME(type, valbytes));            \
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
//TAGBYTES(r)

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
bool fastdecode_boundscheck(const char *ptr, unsigned len, const char *end) {
  uintptr_t uptr = (uintptr_t)ptr;
  uintptr_t uend = (uintptr_t)end;
  uintptr_t res = uptr + len;
  return res < uptr || res > uend;
}

UPB_FORCEINLINE
static const char *fastdecode_string(UPB_PARSE_PARAMS, int tagbytes,
                                     upb_card card) {
  upb_strview *dst;
  int64_t len;
  if (UPB_UNLIKELY(!fastdecode_checktag(data, tagbytes))) {
    RETURN_GENERIC("string field tag mismatch\n");
  }

  dst = fastdecode_getfield(d, ptr, msg, &data, &hasbits, tagbytes,
                            sizeof(upb_strview), card);
  len = ptr[tagbytes];
  if (UPB_UNLIKELY(len < 0)) {
    RETURN_GENERIC("string field len >1 byte\n");
  }
  ptr += tagbytes + 1;
  dst->data = ptr;
  dst->size = len;
  if (UPB_UNLIKELY(fastdecode_boundscheck(ptr, len, d->limit))) {
    return fastdecode_err(d);
  }
  ptr += len;
  return fastdecode_dispatch(d, ptr, msg, table, hasbits);
}

const char *upb_pss_1bt(UPB_PARSE_PARAMS) {
  return fastdecode_string(UPB_PARSE_ARGS, 1, CARD_s);
}

const char *upb_pos_1bt(UPB_PARSE_PARAMS) {
  return fastdecode_string(UPB_PARSE_ARGS, 1, CARD_o);
}

const char *upb_pss_2bt(UPB_PARSE_PARAMS) {
  return fastdecode_string(UPB_PARSE_ARGS, 2, CARD_s);
}

const char *upb_pos_2bt(UPB_PARSE_PARAMS) {
  return fastdecode_string(UPB_PARSE_ARGS, 2, CARD_o);
}

/* message fields *************************************************************/

UPB_FORCEINLINE
static const char *fastdecode_submsg(UPB_PARSE_PARAMS, int tagbytes,
                                     int msg_ceil_bytes, upb_card card) {
  const char *saved_limit = d->limit;
  const char *saved_fastlimit = d->fastlimit;
  const upb_msglayout_field *field = &table->fields[data >> 48];
  size_t ofs = field->offset;
  const upb_msglayout *subl = table->submsgs[field->submsg_index];
  upb_array *arr;
  upb_msg **submsg;
  void *end;

  if (UPB_UNLIKELY(!fastdecode_checktag(data, tagbytes))) {
    RETURN_GENERIC("submessage field tag mismatch\n");
  }

  if (--d->depth < 0) return fastdecode_err(d);

  submsg = fastdecode_getfield_ofs(d, ptr, msg, ofs, &data, &hasbits, &arr,
                                   &end, tagbytes, sizeof(upb_msg *), card);

again:
  if (card == CARD_r) {
    if (UPB_UNLIKELY(submsg == end)) {
      if (arr) arr->len = submsg - (upb_msg**)_upb_array_ptr(arr);
      d->limit = saved_limit;
      d->fastlimit = saved_fastlimit;
      d->depth++;
      RETURN_GENERIC("need array realloc\n");
    }
  }

  {
    uint32_t len = (uint8_t)ptr[tagbytes];
    if (UPB_UNLIKELY(len & 0x80)) {
      uint32_t byte = (uint8_t)ptr[tagbytes + 1];
      len += (byte - 1) << 7;
      if (UPB_UNLIKELY(byte & 0x80)) {
        if (card == CARD_r) {
          arr->len = submsg - (upb_msg**)_upb_array_ptr(arr);
        }
        d->limit = saved_limit;
        d->fastlimit = saved_fastlimit;
        d->depth++;
        RETURN_GENERIC("submessage field len >2 bytes\n");
      }
      ptr++;
    }
    ptr += tagbytes + 1;
    if (UPB_UNLIKELY(fastdecode_boundscheck(ptr, len, saved_limit))) {
      return fastdecode_err(d);
    }
    d->limit = ptr + len;
    d->fastlimit = UPB_MIN(d->limit, d->fastend);
  }

  if (card == CARD_r || !*submsg) {
    *submsg = decode_newmsg_ceil(d, subl, msg_ceil_bytes);
  }
  ptr = fastdecode_dispatch(d, ptr, *submsg, subl, 0);
  submsg++;

  if (ptr != d->limit || d->end_group != 0) {
    return fastdecode_err(d);
  }

  if (card == CARD_r) {
    if (UPB_LIKELY(ptr < saved_fastlimit) &&
        fastdecode_readtag(ptr, tagbytes) == (uint16_t)data) {
      goto again;
    }
    arr->len = submsg - (upb_msg**)_upb_array_ptr(arr);
  }

  d->limit = saved_limit;
  d->fastlimit = saved_fastlimit;
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
