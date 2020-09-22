
#include "upb/decode.h"

#include "upb/port_def.inc"

#define UPB_PARSE_PARAMS                                                \
  upb_decstate *d, const char *ptr, upb_msg *msg, upb_fasttable *table, \
      uint64_t hasbits, uint64_t data

UPB_NOINLINE
const char *fastdecode_dispatch(upb_decstate *d, const char *ptr, upb_msg *msg,
                                upb_fasttable *table, uint64_t hasbits) {
  uint16_t tag;
  uint64_t data;
  if (UPB_UNLIKELY(ptr >= d->fastlimit)) return ptr;
  memcpy(&tag, ptr, 2);
  data = table->field_data[(tag & 0xf7) >> 3] ^ tag;
  return table->field_parser[(tag & 0xf7) >> 3](d, ptr, msg, table, hasbits,
                                                data);
}

UPB_FORCEINLINE bool fastdecode_checktag(uint64_t data, int tagbytes) {
  const char zeros[2] = {0, 0};
  return memcmp(&data, &zeros, tagbytes) == 0;
}

UPB_FORCEINLINE
static const char *fastdecode_scalarfixed(UPB_PARSE_PARAMS, int tagbytes,
                                    int valbytes) {
  char *field;
  if (UPB_UNLIKELY(!fastdecode_checktag(data, tagbytes))) return ptr;
  hasbits |= data;
  field = (char*)msg + (data >> 48);
  memcpy(field, ptr + tagbytes, valbytes);
  return fastdecode_dispatch(d, ptr + tagbytes + valbytes, msg, table, hasbits);
}

const char *upb_psf64_1bt(UPB_PARSE_PARAMS) {
  return fastdecode_scalarfixed(d, ptr, msg, table, hasbits, data, 1, 8);
}

const char *upb_psf64_2bt(UPB_PARSE_PARAMS) {
  return fastdecode_scalarfixed(d, ptr, msg, table, hasbits, data, 2, 8);
}

const char *upb_psf32_1bt(UPB_PARSE_PARAMS) {
  return fastdecode_scalarfixed(d, ptr, msg, table, hasbits, data, 1, 4);
}

const char *upb_psf32_2bt(UPB_PARSE_PARAMS) {
  return fastdecode_scalarfixed(d, ptr, msg, table, hasbits, data, 2, 4);
}

UPB_FORCEINLINE
static const char *fastdecode_longvarint_impl(UPB_PARSE_PARAMS, int64_t res1,
                                              int valbytes) {
  char *field = (char *)data;

  // The algorithm relies on sign extension to set all high bits when the varint
  // continues. This way it can use "and" to aggregate in to the result.
  const int8_t *p = (const int8_t*)(ptr);
  // However this requires the low bits after shifting to be 1's as well. On
  // x86_64 a shld from a single register filled with enough 1's in the high
  // bits can accomplish all this in one instruction. It so happens that res1
  // has 57 high bits of ones, which is enough for the largest shift done.
  assert(res1 >> 7 == -1);
  uint64_t ones = res1;  // save the useful high bit 1's in res1
  uint64_t byte;
  int64_t res2, res3;
  int sign_bit;

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

#define DONE(n)                                                              \
  done##n : {                                                                \
    uint64_t val = res1 & res2 & res3;                                       \
    memcpy(field, &val, valbytes);                                           \
    return fastdecode_dispatch(d, (const char *)p + n, msg, table, hasbits); \
  };

done2 : {
  uint64_t val = res1 & res2;
  memcpy(field, &val, valbytes);
  return fastdecode_dispatch(d, (const char*)p + 2, msg, table, hasbits);
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
}

UPB_NOINLINE
static const char *fastdecode_longvarint32(UPB_PARSE_PARAMS, int64_t val) {
  return fastdecode_longvarint_impl(d, ptr, msg, table, hasbits, data, val, 4);
}

UPB_NOINLINE
static const char *fastdecode_longvarint64(UPB_PARSE_PARAMS, int64_t val) {
  return fastdecode_longvarint_impl(d, ptr, msg, table, hasbits, data, val, 8);
}

UPB_FORCEINLINE
static const char *fastdecode_longvarint(UPB_PARSE_PARAMS, int64_t val,
                                         int valbytes) {
  if (valbytes == 4) {
    return fastdecode_longvarint32(d, ptr, msg, table, hasbits, data, val);
  } else if (valbytes == 8) {
    return fastdecode_longvarint64(d, ptr, msg, table, hasbits, data, val);
  }
  UPB_UNREACHABLE();
}

UPB_FORCEINLINE
static const char *fastdecode_scalarvarint(UPB_PARSE_PARAMS, int tagbytes,
                                           int valbytes) {
  int64_t val;
  void *field;
  if (UPB_UNLIKELY(!fastdecode_checktag(data, tagbytes))) return ptr;
  ptr += tagbytes;
  hasbits |= data;
  field = (char*)msg + (data >> 48);
  val = *ptr;
  if (UPB_UNLIKELY(val < 0)) {
    return fastdecode_longvarint(d, ptr, msg, table, hasbits, (uint64_t)field,
                                 val, valbytes);
  }
  memcpy(field, &val, valbytes);
  return fastdecode_dispatch(d, ptr + 1, msg, table, hasbits);
}

const char *upb_psv32_1bt(UPB_PARSE_PARAMS) {
  return fastdecode_scalarvarint(d, ptr, msg, table, hasbits, data, 1, 4);
}

const char *upb_psv32_2bt(UPB_PARSE_PARAMS) {
  return fastdecode_scalarvarint(d, ptr, msg, table, hasbits, data, 2, 4);
}

const char *upb_psv64_1bt(UPB_PARSE_PARAMS) {
  return fastdecode_scalarvarint(d, ptr, msg, table, hasbits, data, 1, 8);
}

const char *upb_psv64_2bt(UPB_PARSE_PARAMS) {
  return fastdecode_scalarvarint(d, ptr, msg, table, hasbits, data, 2, 8);
}
