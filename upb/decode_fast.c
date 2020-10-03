
#include "upb/decode.h"

#include "upb/port_def.inc"

#define UPB_PARSE_PARAMS                                                \
  upb_decstate *d, const char *ptr, upb_msg *msg, upb_fasttable *table, \
      uint64_t hasbits, uint64_t data

#define UPB_PARSE_ARGS d, ptr, msg, table, hasbits, data

const char *fastdecode_err(upb_decstate *d);

const char *fastdecode_reallocarr(upb_decstate *d, const char *ptr,
                                  upb_msg *msg, upb_fasttable *table,
                                  size_t needbytes);

UPB_NOINLINE
static const char *fastdecode_dispatch(upb_decstate *d, const char *ptr,
                                       upb_msg *msg, upb_fasttable *table,
                                       uint64_t hasbits) {
  uint16_t tag;
  uint64_t data;
  if (UPB_UNLIKELY(ptr >= d->fastlimit)) return ptr;
  memcpy(&tag, ptr, 2);
  data = table->field_data[(tag & 0xf7) >> 3] ^ tag;
  return table->field_parser[(tag & 0xf7) >> 3](UPB_PARSE_ARGS);
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
bool fastdecode_boundscheck(const char *ptr, unsigned len, const char *end) {
  uintptr_t uptr = (uintptr_t)ptr;
  uintptr_t uend = (uintptr_t)end;
  uintptr_t res = uptr + len;
  return res < uptr || res > uend;
}

UPB_FORCEINLINE
static bool fastdecode_readlongsize(const char *ptr, uint32_t *size,
                                    const char **out) {
  int i;
  *size = ptr[0];
  UPB_ASSERT(ptr[0] < 0);
  for (i = 1; i < 5; i++) {
    uint32_t byte = ptr[i];
    (*size) += (byte - 1) << (7 * i);
    if (UPB_LIKELY(byte < 128)) {
      *out = ptr + i + 1;
      return true;
    }
  }
  return false;
}

UPB_FORCEINLINE
static uint16_t fastdecode_readtag(const char *ptr, int tagbytes) {
  uint16_t ret = 0;
  memcpy(&ret, ptr, tagbytes);
  return ret;
}

typedef enum {
  CARD_s = 0,
  CARD_o = 1,
  CARD_r = 2,
  CARD_p = 3
} upb_card;

UPB_FORCEINLINE
static void *fastdecode_getfield(upb_decstate *d, const char *ptr, upb_msg *msg,
                                 uint64_t *data, uint64_t *hasbits,
                                 upb_card card, int tagbytes, int valbytes,
                                 bool store_len) {
  void *field = (char *)msg + (*data >> 48);

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
      upb_array **arr_p = field;
      upb_array *arr;
      uint64_t elem_avail;
      uint16_t expected_tag;
      *hasbits >>= 16;
      *(uint32_t*)msg |= *hasbits;
      *hasbits = 0;
      if (UPB_LIKELY(!*arr_p)) {
        size_t need = (valbytes * 4) + sizeof(upb_array);
        if (UPB_UNLIKELY((size_t)(d->arena_end - d->arena_ptr) < need)) {
          *data = 0;
          return NULL;
        }
        arr = (void*)d->arena_ptr;
        field = arr + 1;
        arr->data = (uintptr_t)field;
        *arr_p = arr;
        arr->size = 4;
        if (store_len) arr->len = 5;
        elem_avail = 4;
        d->arena_ptr += need;
      } else {
        arr = *arr_p;
        field = _upb_array_ptr(arr);
        elem_avail = arr->size - arr->len;
        field = (char*)field + (arr->len * valbytes);
        if (store_len) arr->len = arr->size + 1;
      }
      expected_tag = fastdecode_readtag(ptr, tagbytes);
      *data = elem_avail | ((uint64_t)expected_tag << 32);
      d->arr = arr;
      return field;
    }
    default:
      UPB_UNREACHABLE();
  }
}

/* string fields **************************************************************/

UPB_NOINLINE
static const char *fastdecode_longstring(upb_decstate *d, const char *ptr,
                                         upb_msg *msg, upb_fasttable *table,
                                         uint64_t hasbits, upb_strview *dst) {
  uint32_t len;
  if (!fastdecode_readlongsize(ptr, &len, &ptr)) {
    return fastdecode_err(d);
  }
  dst->data = ptr;
  dst->size = len;
  if (UPB_UNLIKELY(fastdecode_boundscheck(ptr, len, d->limit))) {
    return fastdecode_err(d);
  }
  ptr += len;
  return fastdecode_dispatch(d, ptr, msg, table, hasbits);
}

UPB_FORCEINLINE
static const char *fastdecode_string(UPB_PARSE_PARAMS, int tagbytes,
                                     upb_card card) {
  upb_strview *dst;

  if (UPB_UNLIKELY(!fastdecode_checktag(data, tagbytes))) {
    return ptr;
  }

  dst = fastdecode_getfield(d, ptr, msg, &data, &hasbits, card, tagbytes,
                            sizeof(upb_strview), true);

again:
  if (card == CARD_r) {
    if (UPB_UNLIKELY((uint32_t)data == 0)) {
      return fastdecode_reallocarr(d, ptr, msg, table, sizeof(upb_strview));
    }
  }

  {
    int64_t len = ptr[tagbytes];
    if (UPB_UNLIKELY(len < 0)) {
      if (card == CARD_r) {
        d->arr->len -= (uint32_t)data;
      }
      return fastdecode_longstring(d, ptr, msg, table, hasbits, dst);
    }
    ptr += tagbytes + 1;
    dst->data = ptr;
    dst->size = len;
    if (UPB_UNLIKELY(!fastdecode_boundscheck(ptr, len, d->limit))) {
      return fastdecode_err(d);
    }
    ptr += len;
  }


  if (card == CARD_r) {
    if (UPB_LIKELY(ptr < d->fastlimit) &&
        fastdecode_readtag(ptr, tagbytes) == (uint16_t)(data >> 32)) {
      data--;
      dst++;
      goto again;
    }
    d->arr->len -= (uint32_t)data;
  }

  return fastdecode_dispatch(d, ptr, msg, table, hasbits);
}

const char *upb_pss_1bt(UPB_PARSE_PARAMS) {
  return fastdecode_string(UPB_PARSE_ARGS, 1, CARD_s);
}

const char *upb_pos_1bt(UPB_PARSE_PARAMS) {
  return fastdecode_string(UPB_PARSE_ARGS, 1, CARD_o);
}

const char *upb_prs_1bt(UPB_PARSE_PARAMS) {
  return fastdecode_string(UPB_PARSE_ARGS, 1, CARD_r);
}

const char *upb_pss_2bt(UPB_PARSE_PARAMS) {
  return fastdecode_string(UPB_PARSE_ARGS, 2, CARD_s);
}

const char *upb_pos_2bt(UPB_PARSE_PARAMS) {
  return fastdecode_string(UPB_PARSE_ARGS, 2, CARD_o);
}

const char *upb_prs_2bt(UPB_PARSE_PARAMS) {
  return fastdecode_string(UPB_PARSE_ARGS, 2, CARD_r);
}

/* fixed fields ***************************************************************/

UPB_FORCEINLINE
static const char *fastdecode_fixed(UPB_PARSE_PARAMS, int tagbytes, int valbytes,
                                    upb_card card, _upb_field_parser *fallback) {
  char *dst;

  if (UPB_UNLIKELY(!fastdecode_checktag(data, tagbytes))) {
    if (fallback) {
      // Patch data to amtch packed wiretype.
      data ^= 0x2 ^ (valbytes == 4 ? 5 : 1);
      return fallback(UPB_PARSE_ARGS);
    } else {
      return ptr;
    }
  }

  dst = fastdecode_getfield(d, ptr, msg, &data, &hasbits, card, tagbytes,
                            valbytes, true);

again:
  if (card == CARD_r) {
    if (UPB_UNLIKELY((uint32_t)data == 0)) {
      return fastdecode_reallocarr(d, ptr, msg, table, valbytes);
    }
  }

  {
    ptr += tagbytes;
    memcpy(dst, ptr, valbytes);
    ptr += valbytes;
  }

  if (card == CARD_r) {
    if (UPB_LIKELY(ptr < d->fastlimit) &&
        fastdecode_readtag(ptr, tagbytes) == (uint16_t)(data >> 32)) {
      data--;
      dst += valbytes;
      goto again;
    }
    d->arr->len -= (uint32_t)data;
  }

  return fastdecode_dispatch(d, ptr, msg, table, hasbits);
}

UPB_NOINLINE
const char *fastdecode_longfixedpacked(upb_decstate *d, const char *ptr,
                                       upb_msg *msg, upb_fasttable *table,
                                       uint64_t hasbits, void *dst);

UPB_FORCEINLINE
static const char *fastdecode_fixedpacked(UPB_PARSE_PARAMS, int tagbytes, int valbytes,
                                          _upb_field_parser *fallback) {
  char *dst;
  char *end;
  uint32_t len;

  if (UPB_UNLIKELY(!fastdecode_checktag(data, tagbytes))) {
    if (fallback) {
      // Patch data to amtch packed wiretype.
      data ^= 0x2 ^ (valbytes == 4 ? 5 : 1);
      return fallback(UPB_PARSE_ARGS);
    } else {
      return ptr;
    }
  }

  dst = fastdecode_getfield(d, ptr, msg, &data, &hasbits, CARD_r, tagbytes,
                            valbytes, false);

  len = ptr[tagbytes];
  if (UPB_UNLIKELY(len < 0)) {
    return fastdecode_longfixedpacked(d, ptr, msg, table, hasbits, dst);
  }

  if (UPB_UNLIKELY(len > (size_t)(uint32_t)data * valbytes)) {
    return fastdecode_reallocarr(d, ptr, msg, table, len);
  }

  ptr += tagbytes;

  if (UPB_UNLIKELY((len & (valbytes - 1)) != 0 || /* Non-multiple length. */
                   fastdecode_boundscheck(ptr, len, d->limit))) {
    return fastdecode_err(d);
  }

  end = dst + len;
  d->arr->len += len / valbytes;

  while (dst < end) {
    memcpy(dst, ptr, valbytes);  /* Inline memcpy() loop */
    dst += valbytes;
    ptr += valbytes;
  }

  return fastdecode_dispatch(d, ptr, msg, table, hasbits);
}

UPB_NOINLINE
const char *upb_prf8_1bt(UPB_PARSE_PARAMS);

UPB_NOINLINE
const char *upb_ppf8_1bt(UPB_PARSE_PARAMS) {
  return fastdecode_fixedpacked(UPB_PARSE_ARGS, 1, 8, &upb_prf8_1bt);
}

UPB_NOINLINE
const char *upb_psf8_1bt(UPB_PARSE_PARAMS) {
  return fastdecode_fixed(UPB_PARSE_ARGS, 1, 8, CARD_s, NULL);
}

UPB_NOINLINE
const char *upb_pof8_1bt(UPB_PARSE_PARAMS) {
  return fastdecode_fixed(UPB_PARSE_ARGS, 1, 8, CARD_o, NULL);
}

UPB_NOINLINE
const char *upb_prf8_1bt(UPB_PARSE_PARAMS) {
  return fastdecode_fixed(UPB_PARSE_ARGS, 1, 8, CARD_r, &upb_ppf8_1bt);
}

#if 0


// Generate all fixed functions.
// {s,o,r,p} x {f4,f8} x {1bt,2bt}

#define F(card, valbytes, tagbytes)                                           \
  const char *upb_p##card##f##valbytes##_##tagbytes##bt(UPB_PARSE_PARAMS) {   \
    return fastdecode_fixed(UPB_PARSE_ARGS, tagbytes, valbytes, CARD_##card); \
  }

#define TYPES(card, tagbytes) \
  F(card, 4, tagbytes)        \
  F(card, 8, tagbytes)

#define TAGBYTES(card) \
  TYPES(card, 1)       \
  TYPES(card, 2)

TAGBYTES(s)
TAGBYTES(o)
TAGBYTES(r)
TAGBYTES(p)


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
static const char *fastdecode_longvarint_impl(const char *ptr, void *field,
                                              int valbytes) {
  // The algorithm relies on sign extension to set all high bits when the varint
  // continues. This way it can use "and" to aggregate in to the result.
  const int8_t *p = (const int8_t*)(ptr);
  int64_t res1 = *p;
  uint64_t ones = res1;  // save the useful high bit 1's in res1
  uint64_t byte;
  int64_t res2, res3;
  int sign_bit;

  // However this requires the low bits after shifting to be 1's as well. On
  // x86_64 a shld from a single register filled with enough 1's in the high
  // bits can accomplish all this in one instruction. It so happens that res1
  // has 57 high bits of ones, which is enough for the largest shift done.
  assert(res1 >> 7 == -1);

#define SHLD(n) byte = ((byte << (n * 7)) | (ones >> (64 - (n * 7))))

  // Micro benchmarks show a substantial improvement to capture the sign
  // of the result in the case of just assigning the result of the shift
  // (ie first 2 steps).
#if defined(__GCC_ASM_FLAG_OUTPUTS__) && defined(__x86_64__)
#define SHLD_SIGN(n)                  \
  __asm__("shldq %3, %2, %1"              \
      : "=@ccs"(sign_bit), "+r"(byte) \
      : "r"(ones), "i"(n * 7))
#else
#define SHLD_SIGN(n)                \
  do {                              \
    SHLD(n);                        \
    sign_bit = (int64_t)(byte) < 0; \
  } while (0)
#endif
  byte = p[1];
  SHLD_SIGN(1);
  res2 = byte;
  if (!sign_bit) goto done2;
  byte = p[2];
  SHLD_SIGN(2);
  res3 = byte;
  if (!sign_bit) goto done3;
  byte = p[3];
  SHLD(3);
  res1 &= byte;
  if (res1 >= 0) goto done4;
  byte = p[4];
  SHLD(4);
  res2 &= byte;
  if (res2 >= 0) goto done5;
  byte = p[5];
  SHLD(5);
  res3 &= byte;
  if (res3 >= 0) goto done6;
  byte = p[6];
  SHLD(6);
  res1 &= byte;
  if (res1 >= 0) goto done7;
  byte = p[7];
  SHLD(7);
  res2 &= byte;
  if (res2 >= 0) goto done8;
  byte = p[8];
  SHLD(8);
  res3 &= byte;
  if (res3 >= 0) goto done9;
  byte = p[9];
  // Last byte only contains 0 or 1 for valid 64bit varints. If it's 0 it's
  // a denormalized varint that shouldn't happen. The continuation bit of byte
  // 9 has already the right value hence just expect byte to be 1.
  if (UPB_LIKELY(byte == 1)) goto done10;
  if (byte == 0) {
    res3 ^= (uint64_t)(1) << 63;
    goto done10;
  }

  return NULL;  // Value is too long to be a varint64

#define DONE(n)                        \
  done##n : {                          \
    uint64_t val = res1 & res2 & res3; \
    memcpy(field, &val, valbytes);     \
    return (const char *)p + n;        \
  };

done2 : {
  uint64_t val = res1 & res2;
  memcpy(field, &val, valbytes);
  return (const char*)p + 2;
}

  DONE(3)
  DONE(4)
  DONE(5)
  DONE(6)
  DONE(7)
  DONE(8)
  DONE(9)
  DONE(10)

#undef DONE
#undef SHLD
#undef SHLD_SIGN
}

UPB_NOINLINE
static const char *fastdecode_longvarint32(UPB_PARSE_PARAMS) {
  (void)d;
  (void)msg;
  (void)table;
  (void)hasbits;
  return fastdecode_longvarint_impl(ptr, (void*)data, 4);
}

UPB_NOINLINE
static const char *fastdecode_longvarint64(UPB_PARSE_PARAMS) {
  (void)d;
  (void)msg;
  (void)table;
  (void)hasbits;
  return fastdecode_longvarint_impl(ptr, (void*)data, 8);
}

UPB_FORCEINLINE
static const char *fastdecode_longvarint(UPB_PARSE_PARAMS, int valbytes) {
  if (valbytes == 4) {
    return fastdecode_longvarint32(UPB_PARSE_ARGS);
  } else if (valbytes == 8) {
    return fastdecode_longvarint64(UPB_PARSE_ARGS);
  }
  UPB_UNREACHABLE();
}

UPB_FORCEINLINE
static const char *fastdecode_varint(UPB_PARSE_PARAMS, int tagbytes,
                                     int valbytes, bool zigzag, bool oneof) {
  uint64_t val = 0;
  void *field;
  if (UPB_UNLIKELY(!fastdecode_checktag(data, tagbytes))) return ptr;
  ptr += tagbytes;
  fastdecode_getfield(msg, data, &hasbits, oneof);
  field = (char*)msg + (data >> 48);
  if (UPB_UNLIKELY(*ptr < 0)) {
    return fastdecode_longvarint(d, ptr, msg, table, hasbits, (uint64_t)field,
                                 valbytes);
  }
  val = fastdecode_munge(*ptr, valbytes, zigzag);
  memcpy(field, &val, valbytes);
  return fastdecode_dispatch(d, ptr + 1, msg, table, hasbits);
}

// Generate all varint functions.
// {s,o,r} x {b1,v4,z4,v8,z8} x {1bt,2bt}

#define z_ZZ true
#define b_ZZ false
#define v_ZZ false

#define F(card, type, valbytes, tagbytes)                                      \
  const char *upb_p##card##type##valbytes##_##tagbytes##bt(UPB_PARSE_PARAMS) { \
    return fastdecode_varint(UPB_PARSE_ARGS, tagbytes, valbytes, type##_ZZ,    \
                             card##_ONEOF);                                    \
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
TAGBYTES(r)

#undef z_ZZ
#undef b_ZZ
#undef v_ZZ
#undef o_ONEOF
#undef s_ONEOF
#undef r_ONEOF
#undef F
#undef TYPES
#undef TAGBYTES

#endif
