/*
 * Copyright (c) 2009-2021, Google LLC
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of Google LLC nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL Google LLC BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

// Fast decoder: ~3x the speed of decode.c, but requires x86-64/ARM64.
// Also the table size grows by 2x.
//
// Could potentially be ported to other 64-bit archs that pass at least six
// arguments in registers and have 8 unused high bits in pointers.
//
// The overall design is to create specialized functions for every possible
// field type (eg. oneof boolean field with a 1 byte tag) and then dispatch
// to the specialized function as quickly as possible.

#include "upb/decode_fast.h"

#include "upb/decode_internal.h"

/* Must be last. */
#include "upb/port_def.inc"

#if UPB_FASTTABLE

// The standard set of arguments passed to each parsing function.
// Thanks to x86-64 calling conventions, these will stay in registers.
#define UPB_PARSE_PARAMS                                          \
  upb_decstate *d, const char *ptr, upb_msg *msg, intptr_t table, \
      uint64_t hasbits, uint64_t data

#define UPB_PARSE_ARGS d, ptr, msg, table, hasbits, data

#define RETURN_GENERIC(m)                                                      \
  /* Uncomment either of these for debugging purposes. */                      \
  /* fprintf(stderr, m); */                                                    \
  /*__builtin_trap(); */                                                       \
  return fastdecode_generic(d, ptr, msg, table, hasbits, 0);

typedef enum {
  CARD_s = 0,  /* Singular (optional, non-repeated) */
  CARD_o = 1,  /* Oneof */
  CARD_r = 2,  /* Repeated */
  CARD_p = 3   /* Packed Repeated */
} upb_card;

UPB_NOINLINE
static const char *fastdecode_isdonefallback(UPB_PARSE_PARAMS) {
  int overrun = data;
  ptr = decode_isdonefallback_inl(d, ptr, overrun);
  if (ptr == NULL) {
    return fastdecode_err(d);
  }
  data = fastdecode_loadtag(ptr);
  UPB_MUSTTAIL return fastdecode_tagdispatch(UPB_PARSE_ARGS);
}

UPB_FORCEINLINE
static const char *fastdecode_dispatch(UPB_PARSE_PARAMS) {
  if (UPB_UNLIKELY(ptr >= d->limit_ptr)) {
    int overrun = ptr - d->end;
    if (UPB_LIKELY(overrun == d->limit)) {
      // Parse is finished.
      *(uint32_t*)msg |= hasbits;  // Sync hasbits.
      return ptr;
    } else {
      data = overrun;
      UPB_MUSTTAIL return fastdecode_isdonefallback(UPB_PARSE_ARGS);
    }
  }

  // Read two bytes of tag data (for a one-byte tag, the high byte is junk).
  data = fastdecode_loadtag(ptr);
  UPB_MUSTTAIL return fastdecode_tagdispatch(UPB_PARSE_ARGS);
}

UPB_FORCEINLINE
static bool fastdecode_checktag(uint16_t data, int tagbytes) {
  if (tagbytes == 1) {
    return (data & 0xff) == 0;
  } else {
    return data == 0;
  }
}

UPB_FORCEINLINE
static const char *fastdecode_longsize(const char *ptr, int *size) {
  int i;
  UPB_ASSERT(*size & 0x80);
  *size &= 0xff;
  for (i = 0; i < 3; i++) {
    ptr++;
    size_t byte = (uint8_t)ptr[-1];
    *size += (byte - 1) << (7 + 7 * i);
    if (UPB_LIKELY((byte & 0x80) == 0)) return ptr;
  }
  ptr++;
  size_t byte = (uint8_t)ptr[-1];
  // len is limited by 2gb not 4gb, hence 8 and not 16 as normally expected
  // for a 32 bit varint.
  if (UPB_UNLIKELY(byte >= 8)) return NULL;
  *size += (byte - 1) << 28;
  return ptr;
}

UPB_FORCEINLINE
static bool fastdecode_boundscheck(const char *ptr, size_t len,
                                   const char *end) {
  uintptr_t uptr = (uintptr_t)ptr;
  uintptr_t uend = (uintptr_t)end + 16;
  uintptr_t res = uptr + len;
  return res < uptr || res > uend;
}

UPB_FORCEINLINE
static bool fastdecode_boundscheck2(const char *ptr, size_t len,
                                    const char *end) {
  // This is one extra branch compared to the more normal:
  //   return (size_t)(end - ptr) < size;
  // However it is one less computation if we are just about to use "ptr + len":
  //   https://godbolt.org/z/35YGPz
  // In microbenchmarks this shows an overall 4% improvement.
  uintptr_t uptr = (uintptr_t)ptr;
  uintptr_t uend = (uintptr_t)end;
  uintptr_t res = uptr + len;
  return res < uptr || res > uend;
}

typedef const char *fastdecode_delimfunc(upb_decstate *d, const char *ptr,
                                         void *ctx);

UPB_FORCEINLINE
static const char *fastdecode_delimited(upb_decstate *d, const char *ptr,
                                        fastdecode_delimfunc *func, void *ctx) {
  ptr++;
  int len = (int8_t)ptr[-1];
  if (fastdecode_boundscheck2(ptr, len, d->limit_ptr)) {
    // Slow case: Sub-message is >=128 bytes and/or exceeds the current buffer.
    // If it exceeds the buffer limit, limit/limit_ptr will change during
    // sub-message parsing, so we need to preserve delta, not limit.
    if (UPB_UNLIKELY(len & 0x80)) {
      // Size varint >1 byte (length >= 128).
      ptr = fastdecode_longsize(ptr, &len);
      if (!ptr) {
        // Corrupt wire format: size exceeded INT_MAX.
        return NULL;
      }
    }
    if (ptr - d->end + (int)len > d->limit) {
      // Corrupt wire format: invalid limit.
      return NULL;
    }
    int delta = decode_pushlimit(d, ptr, len);
    ptr = func(d, ptr, ctx);
    decode_poplimit(d, ptr, delta);
  } else {
    // Fast case: Sub-message is <128 bytes and fits in the current buffer.
    // This means we can preserve limit/limit_ptr verbatim.
    const char *saved_limit_ptr = d->limit_ptr;
    int saved_limit = d->limit;
    d->limit_ptr = ptr + len;
    d->limit = d->limit_ptr - d->end;
    UPB_ASSERT(d->limit_ptr == d->end + UPB_MIN(0, d->limit));
    ptr = func(d, ptr, ctx);
    d->limit_ptr = saved_limit_ptr;
    d->limit = saved_limit;
    UPB_ASSERT(d->limit_ptr == d->end + UPB_MIN(0, d->limit));
  }
  return ptr;
}

/* singular, oneof, repeated field handling ***********************************/

typedef struct {
  upb_array *arr;
  void *end;
} fastdecode_arr;

typedef enum {
  FD_NEXT_ATLIMIT,
  FD_NEXT_SAMEFIELD,
  FD_NEXT_OTHERFIELD
} fastdecode_next;

typedef struct {
  void *dst;
  fastdecode_next next;
  uint32_t tag;
} fastdecode_nextret;

UPB_FORCEINLINE
static void *fastdecode_resizearr(upb_decstate *d, void *dst,
                                  fastdecode_arr *farr, int valbytes) {
  if (UPB_UNLIKELY(dst == farr->end)) {
    size_t old_size = farr->arr->size;
    size_t old_bytes = old_size * valbytes;
    size_t new_size = old_size * 2;
    size_t new_bytes = new_size * valbytes;
    char *old_ptr = _upb_array_ptr(farr->arr);
    char *new_ptr = upb_arena_realloc(&d->arena, old_ptr, old_bytes, new_bytes);
    uint8_t elem_size_lg2 = __builtin_ctz(valbytes);
    farr->arr->size = new_size;
    farr->arr->data = _upb_array_tagptr(new_ptr, elem_size_lg2);
    dst = (void*)(new_ptr + (old_size * valbytes));
    farr->end = (void*)(new_ptr + (new_size * valbytes));
  }
  return dst;
}

UPB_FORCEINLINE
static bool fastdecode_tagmatch(uint32_t tag, uint64_t data, int tagbytes) {
  if (tagbytes == 1) {
    return (uint8_t)tag == (uint8_t)data;
  } else {
    return (uint16_t)tag == (uint16_t)data;
  }
}

UPB_FORCEINLINE
static void fastdecode_commitarr(void *dst, fastdecode_arr *farr,
                                 int valbytes) {
  farr->arr->len =
      (size_t)((char *)dst - (char *)_upb_array_ptr(farr->arr)) / valbytes;
}

UPB_FORCEINLINE
static fastdecode_nextret fastdecode_nextrepeated(upb_decstate *d, void *dst,
                                                  const char **ptr,
                                                  fastdecode_arr *farr,
                                                  uint64_t data, int tagbytes,
                                                  int valbytes) {
  fastdecode_nextret ret;
  dst = (char *)dst + valbytes;

  if (UPB_LIKELY(!decode_isdone(d, ptr))) {
    ret.tag = fastdecode_loadtag(*ptr);
    if (fastdecode_tagmatch(ret.tag, data, tagbytes)) {
      ret.next = FD_NEXT_SAMEFIELD;
    } else {
      fastdecode_commitarr(dst, farr, valbytes);
      ret.next = FD_NEXT_OTHERFIELD;
    }
  } else {
    fastdecode_commitarr(dst, farr, valbytes);
    ret.next = FD_NEXT_ATLIMIT;
  }

  ret.dst = dst;
  return ret;
}

UPB_FORCEINLINE
static void *fastdecode_fieldmem(upb_msg *msg, uint64_t data) {
  size_t ofs = data >> 48;
  return (char *)msg + ofs;
}

UPB_FORCEINLINE
static void *fastdecode_getfield(upb_decstate *d, const char *ptr, upb_msg *msg,
                                 uint64_t *data, uint64_t *hasbits,
                                 fastdecode_arr *farr, int valbytes,
                                 upb_card card) {
  switch (card) {
    case CARD_s: {
      uint8_t hasbit_index = *data >> 24;
      // Set hasbit and return pointer to scalar field.
      *hasbits |= 1ull << hasbit_index;
      return fastdecode_fieldmem(msg, *data);
    }
    case CARD_o: {
      uint16_t case_ofs = *data >> 32;
      uint32_t *oneof_case = UPB_PTR_AT(msg, case_ofs, uint32_t);
      uint8_t field_number = *data >> 24;
      *oneof_case = field_number;
      return fastdecode_fieldmem(msg, *data);
    }
    case CARD_r: {
      // Get pointer to upb_array and allocate/expand if necessary.
      uint8_t elem_size_lg2 = __builtin_ctz(valbytes);
      upb_array **arr_p = fastdecode_fieldmem(msg, *data);
      char *begin;
      *(uint32_t*)msg |= *hasbits;
      *hasbits = 0;
      if (UPB_LIKELY(!*arr_p)) {
        farr->arr = _upb_array_new(&d->arena, 8, elem_size_lg2);
        *arr_p = farr->arr;
      } else {
        farr->arr = *arr_p;
      }
      begin = _upb_array_ptr(farr->arr);
      farr->end = begin + (farr->arr->size * valbytes);
      *data = fastdecode_loadtag(ptr);
      return begin + (farr->arr->len * valbytes);
    }
    default:
      UPB_UNREACHABLE();
  }
}

UPB_FORCEINLINE
static bool fastdecode_flippacked(uint64_t *data, int tagbytes) {
  *data ^= (0x2 ^ 0x0);  // Patch data to match packed wiretype.
  return fastdecode_checktag(*data, tagbytes);
}

#define FASTDECODE_CHECKPACKED(tagbytes, card, func)                           \
  if (UPB_UNLIKELY(!fastdecode_checktag(data, tagbytes))) {                    \
    if (card == CARD_r && fastdecode_flippacked(&data, tagbytes)) {            \
      UPB_MUSTTAIL return func(UPB_PARSE_ARGS);                                \
    }                                                                          \
    RETURN_GENERIC("packed check tag mismatch\n");                             \
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
static const char *fastdecode_varint64(const char *ptr, uint64_t *val) {
  ptr++;
  *val = (uint8_t)ptr[-1];
  if (UPB_UNLIKELY(*val & 0x80)) {
    int i;
    for (i = 0; i < 8; i++) {
      ptr++;
      uint64_t byte = (uint8_t)ptr[-1];
      *val += (byte - 1) << (7 + 7 * i);
      if (UPB_LIKELY((byte & 0x80) == 0)) goto done;
    }
    ptr++;
    uint64_t byte = (uint8_t)ptr[-1];
    if (byte > 1) {
      return NULL;
    }
    *val += (byte - 1) << 63;
  }
done:
  UPB_ASSUME(ptr != NULL);
  return ptr;
}

#define FASTDECODE_UNPACKEDVARINT(d, ptr, msg, table, hasbits, data, tagbytes, \
                                  valbytes, card, zigzag, packed)              \
  uint64_t val;                                                                \
  void *dst;                                                                   \
  fastdecode_arr farr;                                                         \
                                                                               \
  FASTDECODE_CHECKPACKED(tagbytes, card, packed);                              \
                                                                               \
  dst = fastdecode_getfield(d, ptr, msg, &data, &hasbits, &farr, valbytes,     \
                            card);                                             \
  if (card == CARD_r) {                                                        \
    if (UPB_UNLIKELY(!dst)) {                                                  \
      RETURN_GENERIC("need array resize\n");                                   \
    }                                                                          \
  }                                                                            \
                                                                               \
  again:                                                                       \
  if (card == CARD_r) {                                                        \
    dst = fastdecode_resizearr(d, dst, &farr, valbytes);                       \
  }                                                                            \
                                                                               \
  ptr += tagbytes;                                                             \
  ptr = fastdecode_varint64(ptr, &val);                                        \
  if (ptr == NULL)                                                             \
    return fastdecode_err(d);                                                  \
  val = fastdecode_munge(val, valbytes, zigzag);                               \
  memcpy(dst, &val, valbytes);                                                 \
                                                                               \
  if (card == CARD_r) {                                                        \
    fastdecode_nextret ret = fastdecode_nextrepeated(                          \
        d, dst, &ptr, &farr, data, tagbytes, valbytes);                        \
    switch (ret.next) {                                                        \
    case FD_NEXT_SAMEFIELD:                                                    \
      dst = ret.dst;                                                           \
      goto again;                                                              \
    case FD_NEXT_OTHERFIELD:                                                   \
      data = ret.tag;                                                          \
      UPB_MUSTTAIL return fastdecode_tagdispatch(UPB_PARSE_ARGS);              \
    case FD_NEXT_ATLIMIT:                                                      \
      return ptr;                                                              \
    }                                                                          \
  }                                                                            \
                                                                               \
  UPB_MUSTTAIL return fastdecode_dispatch(UPB_PARSE_ARGS);

typedef struct {
  uint8_t valbytes;
  bool zigzag;
  void *dst;
  fastdecode_arr farr;
} fastdecode_varintdata;

UPB_FORCEINLINE
static const char *fastdecode_topackedvarint(upb_decstate *d, const char *ptr,
                                             void *ctx) {
  fastdecode_varintdata *data = ctx;
  void *dst = data->dst;
  uint64_t val;

  while (!decode_isdone(d, &ptr)) {
    dst = fastdecode_resizearr(d, dst, &data->farr, data->valbytes);
    ptr = fastdecode_varint64(ptr, &val);
    if (ptr == NULL) return NULL;
    val = fastdecode_munge(val, data->valbytes, data->zigzag);
    memcpy(dst, &val, data->valbytes);
    dst = (char *)dst + data->valbytes;
  }

  fastdecode_commitarr(dst, &data->farr, data->valbytes);
  return ptr;
}

#define FASTDECODE_PACKEDVARINT(d, ptr, msg, table, hasbits, data, tagbytes,   \
                                valbytes, zigzag, unpacked)                    \
  fastdecode_varintdata ctx = {valbytes, zigzag};                              \
                                                                               \
  FASTDECODE_CHECKPACKED(tagbytes, CARD_r, unpacked);                          \
                                                                               \
  ctx.dst = fastdecode_getfield(d, ptr, msg, &data, &hasbits, &ctx.farr,       \
                                valbytes, CARD_r);                             \
  if (UPB_UNLIKELY(!ctx.dst)) {                                                \
    RETURN_GENERIC("need array resize\n");                                     \
  }                                                                            \
                                                                               \
  ptr += tagbytes;                                                             \
  ptr = fastdecode_delimited(d, ptr, &fastdecode_topackedvarint, &ctx);        \
                                                                               \
  if (UPB_UNLIKELY(ptr == NULL)) {                                             \
    return fastdecode_err(d);                                                  \
  }                                                                            \
                                                                               \
  UPB_MUSTTAIL return fastdecode_dispatch(d, ptr, msg, table, hasbits, 0);

#define FASTDECODE_VARINT(d, ptr, msg, table, hasbits, data, tagbytes,     \
                          valbytes, card, zigzag, unpacked, packed)        \
  if (card == CARD_p) {                                                    \
    FASTDECODE_PACKEDVARINT(d, ptr, msg, table, hasbits, data, tagbytes,   \
                            valbytes, zigzag, unpacked);                   \
  } else {                                                                 \
    FASTDECODE_UNPACKEDVARINT(d, ptr, msg, table, hasbits, data, tagbytes, \
                              valbytes, card, zigzag, packed);             \
  }

#define z_ZZ true
#define b_ZZ false
#define v_ZZ false

/* Generate all combinations:
 * {s,o,r,p} x {b1,v4,z4,v8,z8} x {1bt,2bt} */

#define F(card, type, valbytes, tagbytes)                                      \
  UPB_NOINLINE                                                                 \
  const char *upb_p##card##type##valbytes##_##tagbytes##bt(UPB_PARSE_PARAMS) { \
    FASTDECODE_VARINT(d, ptr, msg, table, hasbits, data, tagbytes, valbytes,   \
                      CARD_##card, type##_ZZ,                                  \
                      upb_pr##type##valbytes##_##tagbytes##bt,                 \
                      upb_pp##type##valbytes##_##tagbytes##bt);                \
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
TAGBYTES(p)

#undef z_ZZ
#undef b_ZZ
#undef v_ZZ
#undef o_ONEOF
#undef s_ONEOF
#undef r_ONEOF
#undef F
#undef TYPES
#undef TAGBYTES
#undef FASTDECODE_UNPACKEDVARINT
#undef FASTDECODE_PACKEDVARINT
#undef FASTDECODE_VARINT


/* fixed fields ***************************************************************/

#define FASTDECODE_UNPACKEDFIXED(d, ptr, msg, table, hasbits, data, tagbytes,  \
                                 valbytes, card, packed)                       \
  void *dst;                                                                   \
  fastdecode_arr farr;                                                         \
                                                                               \
  FASTDECODE_CHECKPACKED(tagbytes, card, packed)                               \
                                                                               \
  dst = fastdecode_getfield(d, ptr, msg, &data, &hasbits, &farr, valbytes,     \
                            card);                                             \
  if (card == CARD_r) {                                                        \
    if (UPB_UNLIKELY(!dst)) {                                                  \
      RETURN_GENERIC("couldn't allocate array in arena\n");                    \
    }                                                                          \
  }                                                                            \
                                                                               \
  again:                                                                       \
  if (card == CARD_r) {                                                        \
    dst = fastdecode_resizearr(d, dst, &farr, valbytes);                       \
  }                                                                            \
                                                                               \
  ptr += tagbytes;                                                             \
  memcpy(dst, ptr, valbytes);                                                  \
  ptr += valbytes;                                                             \
                                                                               \
  if (card == CARD_r) {                                                        \
    fastdecode_nextret ret = fastdecode_nextrepeated(                          \
        d, dst, &ptr, &farr, data, tagbytes, valbytes);                        \
    switch (ret.next) {                                                        \
    case FD_NEXT_SAMEFIELD:                                                    \
      dst = ret.dst;                                                           \
      goto again;                                                              \
    case FD_NEXT_OTHERFIELD:                                                   \
      data = ret.tag;                                                          \
      UPB_MUSTTAIL return fastdecode_tagdispatch(UPB_PARSE_ARGS);              \
    case FD_NEXT_ATLIMIT:                                                      \
      return ptr;                                                              \
    }                                                                          \
  }                                                                            \
                                                                               \
  UPB_MUSTTAIL return fastdecode_dispatch(UPB_PARSE_ARGS);

#define FASTDECODE_PACKEDFIXED(d, ptr, msg, table, hasbits, data, tagbytes, \
                               valbytes, unpacked)                          \
  FASTDECODE_CHECKPACKED(tagbytes, CARD_r, unpacked)                        \
                                                                            \
  ptr += tagbytes;                                                          \
  int size = (uint8_t)ptr[0];                                               \
  ptr++;                                                                    \
  if (size & 0x80) {                                                        \
    ptr = fastdecode_longsize(ptr, &size);                                  \
  }                                                                         \
                                                                            \
  if (UPB_UNLIKELY(fastdecode_boundscheck(ptr, size, d->limit_ptr) ||       \
                   (size % valbytes) != 0)) {                               \
    return fastdecode_err(d);                                               \
  }                                                                         \
                                                                            \
  upb_array **arr_p = fastdecode_fieldmem(msg, data);                       \
  upb_array *arr = *arr_p;                                                  \
  uint8_t elem_size_lg2 = __builtin_ctz(valbytes);                          \
  int elems = size / valbytes;                                              \
                                                                            \
  if (UPB_LIKELY(!arr)) {                                                   \
    *arr_p = arr = _upb_array_new(&d->arena, elems, elem_size_lg2);         \
    if (!arr) {                                                             \
      return fastdecode_err(d);                                             \
    }                                                                       \
  } else {                                                                  \
    _upb_array_resize(arr, elems, &d->arena);                               \
  }                                                                         \
                                                                            \
  char *dst = _upb_array_ptr(arr);                                          \
  memcpy(dst, ptr, size);                                                   \
  arr->len = elems;                                                         \
                                                                            \
  ptr += size;                                                              \
  UPB_MUSTTAIL return fastdecode_dispatch(UPB_PARSE_ARGS);

#define FASTDECODE_FIXED(d, ptr, msg, table, hasbits, data, tagbytes,     \
                         valbytes, card, unpacked, packed)                \
  if (card == CARD_p) {                                                   \
    FASTDECODE_PACKEDFIXED(d, ptr, msg, table, hasbits, data, tagbytes,   \
                           valbytes, unpacked);                           \
  } else {                                                                \
    FASTDECODE_UNPACKEDFIXED(d, ptr, msg, table, hasbits, data, tagbytes, \
                             valbytes, card, packed);                     \
  }

/* Generate all combinations:
 * {s,o,r,p} x {f4,f8} x {1bt,2bt} */

#define F(card, valbytes, tagbytes)                                         \
  UPB_NOINLINE                                                              \
  const char *upb_p##card##f##valbytes##_##tagbytes##bt(UPB_PARSE_PARAMS) { \
    FASTDECODE_FIXED(d, ptr, msg, table, hasbits, data, tagbytes, valbytes, \
                     CARD_##card, upb_ppf##valbytes##_##tagbytes##bt,       \
                     upb_prf##valbytes##_##tagbytes##bt);                   \
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

#undef F
#undef TYPES
#undef TAGBYTES
#undef FASTDECODE_UNPACKEDFIXED
#undef FASTDECODE_PACKEDFIXED

/* string fields **************************************************************/

typedef const char *fastdecode_copystr_func(struct upb_decstate *d,
                                            const char *ptr, upb_msg *msg,
                                            const upb_msglayout *table,
                                            uint64_t hasbits, upb_strview *dst);

UPB_NOINLINE
static const char *fastdecode_verifyutf8(upb_decstate *d, const char *ptr,
                                         upb_msg *msg, intptr_t table,
                                         uint64_t hasbits, uint64_t data) {
  upb_strview *dst = (upb_strview*)data;
  if (!decode_verifyutf8_inl(dst->data, dst->size)) {
    return fastdecode_err(d);
  }
  UPB_MUSTTAIL return fastdecode_dispatch(UPB_PARSE_ARGS);
}

#define FASTDECODE_LONGSTRING(d, ptr, msg, table, hasbits, dst, validate_utf8) \
  int size = (uint8_t)ptr[0]; /* Could plumb through hasbits. */               \
  ptr++;                                                                       \
  if (size & 0x80) {                                                           \
    ptr = fastdecode_longsize(ptr, &size);                                     \
  }                                                                            \
                                                                               \
  if (UPB_UNLIKELY(fastdecode_boundscheck(ptr, size, d->limit_ptr))) {         \
    dst->size = 0;                                                             \
    return fastdecode_err(d);                                                  \
  }                                                                            \
                                                                               \
  if (d->alias) {                                                              \
    dst->data = ptr;                                                           \
    dst->size = size;                                                          \
  } else {                                                                     \
    char *data = upb_arena_malloc(&d->arena, size);                            \
    if (!data) {                                                               \
      return fastdecode_err(d);                                                \
    }                                                                          \
    memcpy(data, ptr, size);                                                   \
    dst->data = data;                                                          \
    dst->size = size;                                                          \
  }                                                                            \
                                                                               \
  ptr += size;                                                                 \
  if (validate_utf8) {                                                         \
    data = (uint64_t)dst;                                                      \
    UPB_MUSTTAIL return fastdecode_verifyutf8(UPB_PARSE_ARGS);                 \
  } else {                                                                     \
    UPB_MUSTTAIL return fastdecode_dispatch(UPB_PARSE_ARGS);                   \
  }

UPB_NOINLINE
static const char *fastdecode_longstring_utf8(struct upb_decstate *d,
                                              const char *ptr, upb_msg *msg,
                                              intptr_t table, uint64_t hasbits,
                                              uint64_t data) {
  upb_strview *dst = (upb_strview*)data;
  FASTDECODE_LONGSTRING(d, ptr, msg, table, hasbits, dst, true);
}

UPB_NOINLINE
static const char *fastdecode_longstring_noutf8(struct upb_decstate *d,
                                                const char *ptr, upb_msg *msg,
                                                intptr_t table,
                                                uint64_t hasbits,
                                                uint64_t data) {
  upb_strview *dst = (upb_strview*)data;
  FASTDECODE_LONGSTRING(d, ptr, msg, table, hasbits, dst, false);
}

UPB_FORCEINLINE
static void fastdecode_docopy(upb_decstate *d, const char *ptr, uint32_t size,
                              int copy, char *data, upb_strview *dst) {
  d->arena.head.ptr += copy;
  dst->data = data;
  UPB_UNPOISON_MEMORY_REGION(data, copy);
  memcpy(data, ptr, copy);
  UPB_POISON_MEMORY_REGION(data + size, copy - size);
}

#define FASTDECODE_COPYSTRING(d, ptr, msg, table, hasbits, data, tagbytes,     \
                              card, validate_utf8)                             \
  upb_strview *dst;                                                            \
  fastdecode_arr farr;                                                         \
  int64_t size;                                                                \
  size_t arena_has;                                                            \
  size_t common_has;                                                           \
  char *buf;                                                                   \
                                                                               \
  UPB_ASSERT(!d->alias);                                                       \
  UPB_ASSERT(fastdecode_checktag(data, tagbytes));                             \
                                                                               \
  dst = fastdecode_getfield(d, ptr, msg, &data, &hasbits, &farr,               \
                            sizeof(upb_strview), card);                        \
                                                                               \
  again:                                                                       \
  if (card == CARD_r) {                                                        \
    dst = fastdecode_resizearr(d, dst, &farr, sizeof(upb_strview));            \
  }                                                                            \
                                                                               \
  size = (uint8_t)ptr[tagbytes];                                               \
  ptr += tagbytes + 1;                                                         \
  dst->size = size;                                                            \
                                                                               \
  buf = d->arena.head.ptr;                                                     \
  arena_has = _upb_arenahas(&d->arena);                                        \
  common_has = UPB_MIN(arena_has, (d->end - ptr) + 16);                        \
                                                                               \
  if (UPB_LIKELY(size <= 15 - tagbytes)) {                                     \
    if (arena_has < 16)                                                        \
      goto longstr;                                                            \
    d->arena.head.ptr += 16;                                                   \
    memcpy(buf, ptr - tagbytes - 1, 16);                                       \
    dst->data = buf + tagbytes + 1;                                            \
  } else if (UPB_LIKELY(size <= 32)) {                                         \
    if (UPB_UNLIKELY(common_has < 32))                                         \
      goto longstr;                                                            \
    fastdecode_docopy(d, ptr, size, 32, buf, dst);                             \
  } else if (UPB_LIKELY(size <= 64)) {                                         \
    if (UPB_UNLIKELY(common_has < 64))                                         \
      goto longstr;                                                            \
    fastdecode_docopy(d, ptr, size, 64, buf, dst);                             \
  } else if (UPB_LIKELY(size < 128)) {                                         \
    if (UPB_UNLIKELY(common_has < 128))                                        \
      goto longstr;                                                            \
    fastdecode_docopy(d, ptr, size, 128, buf, dst);                            \
  } else {                                                                     \
    goto longstr;                                                              \
  }                                                                            \
                                                                               \
  ptr += size;                                                                 \
                                                                               \
  if (card == CARD_r) {                                                        \
    if (validate_utf8 && !decode_verifyutf8_inl(dst->data, dst->size)) {       \
      return fastdecode_err(d);                                                \
    }                                                                          \
    fastdecode_nextret ret = fastdecode_nextrepeated(                          \
        d, dst, &ptr, &farr, data, tagbytes, sizeof(upb_strview));             \
    switch (ret.next) {                                                        \
    case FD_NEXT_SAMEFIELD:                                                    \
      dst = ret.dst;                                                           \
      goto again;                                                              \
    case FD_NEXT_OTHERFIELD:                                                   \
      data = ret.tag;                                                          \
      UPB_MUSTTAIL return fastdecode_tagdispatch(UPB_PARSE_ARGS);              \
    case FD_NEXT_ATLIMIT:                                                      \
      return ptr;                                                              \
    }                                                                          \
  }                                                                            \
                                                                               \
  if (card != CARD_r && validate_utf8) {                                       \
    data = (uint64_t)dst;                                                      \
    UPB_MUSTTAIL return fastdecode_verifyutf8(UPB_PARSE_ARGS);                 \
  }                                                                            \
                                                                               \
  UPB_MUSTTAIL return fastdecode_dispatch(UPB_PARSE_ARGS);                     \
                                                                               \
  longstr:                                                                     \
  ptr--;                                                                       \
  if (validate_utf8) {                                                         \
    UPB_MUSTTAIL return fastdecode_longstring_utf8(d, ptr, msg, table,         \
                                                   hasbits, (uint64_t)dst);    \
  } else {                                                                     \
    UPB_MUSTTAIL return fastdecode_longstring_noutf8(d, ptr, msg, table,       \
                                                     hasbits, (uint64_t)dst);  \
  }

#define FASTDECODE_STRING(d, ptr, msg, table, hasbits, data, tagbytes, card,   \
                          copyfunc, validate_utf8)                             \
  upb_strview *dst;                                                            \
  fastdecode_arr farr;                                                         \
  int64_t size;                                                                \
                                                                               \
  if (UPB_UNLIKELY(!fastdecode_checktag(data, tagbytes))) {                    \
    RETURN_GENERIC("string field tag mismatch\n");                             \
  }                                                                            \
                                                                               \
  if (UPB_UNLIKELY(!d->alias)) {                                               \
    UPB_MUSTTAIL return copyfunc(UPB_PARSE_ARGS);                              \
  }                                                                            \
                                                                               \
  dst = fastdecode_getfield(d, ptr, msg, &data, &hasbits, &farr,               \
                            sizeof(upb_strview), card);                        \
                                                                               \
  again:                                                                       \
  if (card == CARD_r) {                                                        \
    dst = fastdecode_resizearr(d, dst, &farr, sizeof(upb_strview));            \
  }                                                                            \
                                                                               \
  size = (int8_t)ptr[tagbytes];                                                \
  ptr += tagbytes + 1;                                                         \
  dst->data = ptr;                                                             \
  dst->size = size;                                                            \
                                                                               \
  if (UPB_UNLIKELY(fastdecode_boundscheck(ptr, size, d->end))) {               \
    ptr--;                                                                     \
    if (validate_utf8) {                                                       \
      return fastdecode_longstring_utf8(d, ptr, msg, table, hasbits,           \
                                        (uint64_t)dst);                        \
    } else {                                                                   \
      return fastdecode_longstring_noutf8(d, ptr, msg, table, hasbits,         \
                                          (uint64_t)dst);                      \
    }                                                                          \
  }                                                                            \
                                                                               \
  ptr += size;                                                                 \
                                                                               \
  if (card == CARD_r) {                                                        \
    if (validate_utf8 && !decode_verifyutf8_inl(dst->data, dst->size)) {       \
      return fastdecode_err(d);                                                \
    }                                                                          \
    fastdecode_nextret ret = fastdecode_nextrepeated(                          \
        d, dst, &ptr, &farr, data, tagbytes, sizeof(upb_strview));             \
    switch (ret.next) {                                                        \
    case FD_NEXT_SAMEFIELD:                                                    \
      dst = ret.dst;                                                           \
      if (UPB_UNLIKELY(!d->alias)) {                                           \
        /* Buffer flipped and we can't alias any more. Bounce to */            \
        /* copyfunc(), but via dispatch since we need to reload table */       \
        /* data also. */                                                       \
        fastdecode_commitarr(dst, &farr, sizeof(upb_strview));                 \
        data = ret.tag;                                                        \
        UPB_MUSTTAIL return fastdecode_tagdispatch(UPB_PARSE_ARGS);            \
      }                                                                        \
      goto again;                                                              \
    case FD_NEXT_OTHERFIELD:                                                   \
      data = ret.tag;                                                          \
      UPB_MUSTTAIL return fastdecode_tagdispatch(UPB_PARSE_ARGS);              \
    case FD_NEXT_ATLIMIT:                                                      \
      return ptr;                                                              \
    }                                                                          \
  }                                                                            \
                                                                               \
  if (card != CARD_r && validate_utf8) {                                       \
    data = (uint64_t)dst;                                                      \
    UPB_MUSTTAIL return fastdecode_verifyutf8(UPB_PARSE_ARGS);                 \
  }                                                                            \
                                                                               \
  UPB_MUSTTAIL return fastdecode_dispatch(UPB_PARSE_ARGS);

/* Generate all combinations:
 * {p,c} x {s,o,r} x {s, b} x {1bt,2bt} */

#define s_VALIDATE true
#define b_VALIDATE false

#define F(card, tagbytes, type)                                        \
  UPB_NOINLINE                                                         \
  const char *upb_c##card##type##_##tagbytes##bt(UPB_PARSE_PARAMS) {   \
    FASTDECODE_COPYSTRING(d, ptr, msg, table, hasbits, data, tagbytes, \
                          CARD_##card, type##_VALIDATE);               \
  }                                                                    \
  const char *upb_p##card##type##_##tagbytes##bt(UPB_PARSE_PARAMS) {   \
    FASTDECODE_STRING(d, ptr, msg, table, hasbits, data, tagbytes,     \
                      CARD_##card, upb_c##card##type##_##tagbytes##bt, \
                      type##_VALIDATE);                                \
  }

#define UTF8(card, tagbytes) \
  F(card, tagbytes, s)       \
  F(card, tagbytes, b)

#define TAGBYTES(card) \
  UTF8(card, 1)        \
  UTF8(card, 2)

TAGBYTES(s)
TAGBYTES(o)
TAGBYTES(r)

#undef s_VALIDATE
#undef b_VALIDATE
#undef F
#undef TAGBYTES
#undef FASTDECODE_LONGSTRING
#undef FASTDECODE_COPYSTRING
#undef FASTDECODE_STRING

/* message fields *************************************************************/

UPB_INLINE
upb_msg *decode_newmsg_ceil(upb_decstate *d, const upb_msglayout *l,
                            int msg_ceil_bytes) {
  size_t size = l->size + sizeof(upb_msg_internal);
  char *msg_data;
  if (UPB_LIKELY(msg_ceil_bytes > 0 &&
                 _upb_arenahas(&d->arena) >= msg_ceil_bytes)) {
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

typedef struct {
  intptr_t table;
  upb_msg *msg;
} fastdecode_submsgdata;

UPB_FORCEINLINE
static const char *fastdecode_tosubmsg(upb_decstate *d, const char *ptr,
                                       void *ctx) {
  fastdecode_submsgdata *submsg = ctx;
  ptr = fastdecode_dispatch(d, ptr, submsg->msg, submsg->table, 0, 0);
  UPB_ASSUME(ptr != NULL);
  return ptr;
}

#define FASTDECODE_SUBMSG(d, ptr, msg, table, hasbits, data, tagbytes,    \
                          msg_ceil_bytes, card)                           \
                                                                          \
  if (UPB_UNLIKELY(!fastdecode_checktag(data, tagbytes))) {               \
    RETURN_GENERIC("submessage field tag mismatch\n");                    \
  }                                                                       \
                                                                          \
  if (--d->depth == 0) return fastdecode_err(d);                          \
                                                                          \
  upb_msg **dst;                                                          \
  uint32_t submsg_idx = (data >> 16) & 0xff;                              \
  const upb_msglayout *tablep = decode_totablep(table);                   \
  const upb_msglayout *subtablep = tablep->subs[submsg_idx].submsg;       \
  fastdecode_submsgdata submsg = {decode_totable(subtablep)};             \
  fastdecode_arr farr;                                                    \
                                                                          \
  if (subtablep->table_mask == (uint8_t)-1) {                             \
    RETURN_GENERIC("submessage doesn't have fast tables.");               \
  }                                                                       \
                                                                          \
  dst = fastdecode_getfield(d, ptr, msg, &data, &hasbits, &farr,          \
                            sizeof(upb_msg *), card);                     \
                                                                          \
  if (card == CARD_s) {                                                   \
    *(uint32_t *)msg |= hasbits;                                          \
    hasbits = 0;                                                          \
  }                                                                       \
                                                                          \
  again:                                                                  \
  if (card == CARD_r) {                                                   \
    dst = fastdecode_resizearr(d, dst, &farr, sizeof(upb_msg *));         \
  }                                                                       \
                                                                          \
  submsg.msg = *dst;                                                      \
                                                                          \
  if (card == CARD_r || UPB_LIKELY(!submsg.msg)) {                        \
    *dst = submsg.msg = decode_newmsg_ceil(d, subtablep, msg_ceil_bytes); \
  }                                                                       \
                                                                          \
  ptr += tagbytes;                                                        \
  ptr = fastdecode_delimited(d, ptr, fastdecode_tosubmsg, &submsg);       \
                                                                          \
  if (UPB_UNLIKELY(ptr == NULL || d->end_group != DECODE_NOGROUP)) {      \
    return fastdecode_err(d);                                             \
  }                                                                       \
                                                                          \
  if (card == CARD_r) {                                                   \
    fastdecode_nextret ret = fastdecode_nextrepeated(                     \
        d, dst, &ptr, &farr, data, tagbytes, sizeof(upb_msg *));          \
    switch (ret.next) {                                                   \
      case FD_NEXT_SAMEFIELD:                                             \
        dst = ret.dst;                                                    \
        goto again;                                                       \
      case FD_NEXT_OTHERFIELD:                                            \
        d->depth++;                                                       \
        data = ret.tag;                                                   \
        UPB_MUSTTAIL return fastdecode_tagdispatch(UPB_PARSE_ARGS);       \
      case FD_NEXT_ATLIMIT:                                               \
        d->depth++;                                                       \
        return ptr;                                                       \
    }                                                                     \
  }                                                                       \
                                                                          \
  d->depth++;                                                             \
  UPB_MUSTTAIL return fastdecode_dispatch(UPB_PARSE_ARGS);

#define F(card, tagbytes, size_ceil, ceil_arg)                               \
  const char *upb_p##card##m_##tagbytes##bt_max##size_ceil##b(               \
      UPB_PARSE_PARAMS) {                                                    \
    FASTDECODE_SUBMSG(d, ptr, msg, table, hasbits, data, tagbytes, ceil_arg, \
                      CARD_##card);                                          \
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
#undef FASTDECODE_SUBMSG

#endif  /* UPB_FASTTABLE */
