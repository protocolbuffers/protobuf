
#include "upb/decode_fast.h"

#include "upb/decode.h"
#include "upb/decode.int.h"

/* Must be last. */
#include "upb/port_def.inc"

#define UPB_PARSE_PARAMS                                                      \
  upb_decstate *d, const char *ptr, upb_msg *msg, const upb_msglayout *table, \
      uint64_t hasbits, uint64_t data

#define UPB_PARSE_ARGS d, ptr, msg, table, hasbits, data

#define RETURN_GENERIC(msg)  \
  /* fprintf(stderr, msg); */ \
  return fastdecode_generic(UPB_PARSE_ARGS);

typedef enum {
  CARD_s = 0,  /* Singular (optional, non-repeated) */
  CARD_o = 1,  /* Oneof */
  CARD_r = 2   /* Repeated */
} upb_card;

UPB_FORCEINLINE
const char *fastdecode_tag_dispatch(upb_decstate *d, const char *ptr, upb_msg *msg,
                                const upb_msglayout *table, uint64_t hasbits, uint32_t tag) {
  uint64_t data;
  size_t idx;
  idx = (tag & 0xf8) >> 3;
  data = table->fasttable[idx].field_data ^ tag;
  return table->fasttable[idx].field_parser(UPB_PARSE_ARGS);
}

UPB_FORCEINLINE
uint32_t fastdecode_load_tag(const char* ptr) {
  uint16_t tag;
  memcpy(&tag, ptr, 2);
  return tag;
}

UPB_FORCEINLINE
const char *fastdecode_dispatch(upb_decstate *d, const char *ptr, upb_msg *msg,
                                const upb_msglayout *table, uint64_t hasbits) {
  if (UPB_UNLIKELY(ptr >= d->fastlimit)) {
    if (UPB_LIKELY(ptr == d->limit)) {
      *(uint32_t*)msg |= hasbits >> 16;  /* Sync hasbits. */
      return ptr;
    }
    uint64_t data = 0;
    RETURN_GENERIC("dispatch hit end\n");
  }
  return fastdecode_tag_dispatch(d, ptr, msg, table, hasbits, fastdecode_load_tag(ptr));
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
      if (hasbit_is_idx) {
        *hasbits |= 1ull << ((*data >> 32) & 63);
      } else {
        *hasbits |= *data;
      }
      return field;
    case CARD_r: {
      uint8_t elem_size_lg2 = __builtin_ctz(valbytes);
      upb_array **arr_p = field;
      upb_array *arr;
      *hasbits >>= 16;
      *(uint32_t*)msg |= *hasbits;
      *hasbits = 0;
      if (UPB_LIKELY(!*arr_p)) {
        const size_t initial_len = 8;
        size_t need = (valbytes * initial_len) + sizeof(upb_array);
        if (!hasbit_is_idx && UPB_UNLIKELY(!_upb_arenahas(&d->arena, need))) {
          return NULL;
        }
        arr = upb_arena_malloc(&d->arena, need);
        field = arr + 1;
        arr->data = _upb_array_tagptr(field, elem_size_lg2);
        *arr_p = arr;
        arr->size = initial_len;
        *end = (char*)field + (arr->size * valbytes);
      } else {
        arr = *arr_p;
        field = _upb_array_ptr(arr);
        *end = (char*)field + (arr->size * valbytes);
        field = (char*)field + (arr->len * valbytes);
      }
      *data = fastdecode_load_tag(ptr);
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
bool fastdecode_boundscheck(const char *ptr, size_t len, const char *end) {
  uintptr_t uptr = (uintptr_t)ptr;
  uintptr_t uend = (uintptr_t)end;
  uintptr_t res = uptr + len;
  return res < uptr || res > uend;
}

UPB_FORCEINLINE
static const char *fastdecode_string(UPB_PARSE_PARAMS, int tagbytes,
                                     upb_card card) {
  upb_strview *dst;
  const char *str;
  int64_t len;
  if (UPB_UNLIKELY(!fastdecode_checktag(data, tagbytes))) {
    RETURN_GENERIC("string field tag mismatch\n");
  }

  dst = fastdecode_getfield(d, ptr, msg, &data, &hasbits,
                            sizeof(upb_strview), card);
  len = (int8_t)ptr[tagbytes];
  str = ptr + tagbytes + 1;
  dst->data = str;
  dst->size = len;
  if (UPB_UNLIKELY(fastdecode_boundscheck(str, len, d->limit))) {
    dst->size = 0;
    RETURN_GENERIC("string field len >1 byte\n");
  }
  return fastdecode_dispatch(d, str + len, msg, table, hasbits);
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

UPB_NOINLINE static
const char *fastdecode_lendelim_submsg(upb_decstate *d, const char *ptr, upb_msg *msg,
                                const upb_msglayout *table, uint64_t hasbits, const char* saved_limit) {
  size_t len = (uint8_t)ptr[-1];
  if (UPB_UNLIKELY(len & 0x80)) {
    int i;
    for (i = 0; i < 3; i++) {
      ptr++;
      size_t byte = (uint8_t)ptr[-1];
      len += (byte - 1) << (7 + 7 * i);
      if (UPB_LIKELY((byte & 0x80) == 0)) goto done;
    }
    ptr++;
    size_t byte = (uint8_t)ptr[-1];
    // len is limited by 2gb not 4gb, hence 8 and not 16 as normally expected for a 32 bit varint.
    if (UPB_UNLIKELY(byte >= 8)) return fastdecode_err(d);
    len += (byte - 1) << 28;
  }
done:
  if (UPB_UNLIKELY(fastdecode_boundscheck(ptr, len, saved_limit))) {
    return fastdecode_err(d);
  }
  d->limit = ptr + len;
  d->fastlimit = UPB_MIN(d->limit, d->fastend);

  return fastdecode_dispatch(d, ptr, msg, table, hasbits);
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

  const char *saved_limit = d->limit;
  const char *saved_fastlimit = d->fastlimit;

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

  ptr = fastdecode_lendelim_submsg(d, ptr, child, subl, 0, saved_limit);

  if (UPB_UNLIKELY(ptr != d->limit || d->end_group != 0)) {
    return fastdecode_err(d);
  }

  if (card == CARD_r) {
    submsg++;
    if (UPB_LIKELY(ptr < saved_fastlimit)) {
      uint32_t tag = fastdecode_load_tag(ptr);
      if (tagbytes == 1) {
        if ((uint8_t)tag == (uint8_t)data) goto again;
      } else {
        if ((uint16_t)tag == (uint16_t)data) goto again;
      }
      arr->len = submsg - (upb_msg**)_upb_array_ptr(arr);
      d->limit = saved_limit;
      d->fastlimit = saved_fastlimit;
      d->depth++;
      return fastdecode_tag_dispatch(d, ptr, msg, table, hasbits, tag);
    } else {
      if (ptr == saved_limit) {
        arr->len = submsg - (upb_msg**)_upb_array_ptr(arr);
        d->limit = saved_limit;
        d->fastlimit = saved_fastlimit;
        d->depth++;
        return ptr;
      }
      goto repeated_generic;
    }
  }

  d->limit = saved_limit;
  d->fastlimit = saved_fastlimit;
  d->depth++;

  return fastdecode_dispatch(d, ptr, msg, table, hasbits);

repeated_generic:
  arr->len = submsg - (upb_msg**)_upb_array_ptr(arr);
  d->limit = saved_limit;
  d->fastlimit = saved_fastlimit;
  d->depth++;
  RETURN_GENERIC("repeated generic");
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
