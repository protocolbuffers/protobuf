
#include <stdio.h>
#include "upb/pb/varint.int.h"
#include "tests/upb_test.h"

#include "upb/port_def.inc"

/* Test that we can round-trip from int->varint->int. */
static void test_varint_for_num(upb_decoderet (*decoder)(const char*),
                                uint64_t num) {
  char buf[16];
  size_t bytes;
  upb_decoderet r;

  memset(buf, 0xff, sizeof(buf));
  bytes = upb_vencode64(num, buf);

  if (num <= UINT32_MAX) {
    uint64_t encoded = upb_vencode32(num);
    char buf2[16];
    upb_decoderet r;

    memset(buf2, 0, sizeof(buf2));
    memcpy(&buf2, &encoded, 8);
#ifdef UPB_BIG_ENDIAN
    char swap[8];
    swap[0] = buf2[7];
    swap[1] = buf2[6];
    swap[2] = buf2[5];
    swap[3] = buf2[4];
    swap[4] = buf2[3];
    swap[5] = buf2[2];
    swap[6] = buf2[1];
    swap[7] = buf2[0];
    buf2[0] = swap[0];
    buf2[1] = swap[1];
    buf2[2] = swap[2];
    buf2[3] = swap[3];
    buf2[4] = swap[4];
    buf2[5] = swap[5];
    buf2[6] = swap[6];
    buf2[7] = swap[7];
#endif    
    r = decoder(buf2);
    ASSERT(r.val == num);
    ASSERT(r.p == buf2 + upb_value_size(encoded));
    ASSERT(upb_zzenc_32(upb_zzdec_32(num)) == num);
  }

  r = decoder(buf);
  ASSERT(r.val == num);
  ASSERT(r.p == buf + bytes);
  ASSERT(upb_zzenc_64(upb_zzdec_64(num)) == num);
}

static void test_varint_decoder(upb_decoderet (*decoder)(const char*)) {
#define TEST(bytes, expected_val) {\
    size_t n = sizeof(bytes) - 1;  /* for NULL */ \
    char buf[UPB_PB_VARINT_MAX_LEN]; \
    upb_decoderet r; \
    memset(buf, 0xff, sizeof(buf)); \
    memcpy(buf, bytes, n); \
    r = decoder(buf); \
    ASSERT(r.val == expected_val); \
    ASSERT(r.p == buf + n); \
  }

  uint64_t num;

  char twelvebyte[16] = {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 1, 1};
  const char *twelvebyte_buf = twelvebyte;
  /* A varint that terminates before hitting the end of the provided buffer,
   * but in too many bytes (11 instead of 10). */
  upb_decoderet r = decoder(twelvebyte_buf);
  ASSERT(r.p == NULL);

  TEST("\x00",                                                      0ULL);
  TEST("\x01",                                                      1ULL);
  TEST("\x81\x14",                                              0xa01ULL);
  TEST("\x81\x03",                                              0x181ULL);
  TEST("\x81\x83\x07",                                        0x1c181ULL);
  TEST("\x81\x83\x87\x0f",                                  0x1e1c181ULL);
  TEST("\x81\x83\x87\x8f\x1f",                            0x1f1e1c181ULL);
  TEST("\x81\x83\x87\x8f\x9f\x3f",                      0x1f9f1e1c181ULL);
  TEST("\x81\x83\x87\x8f\x9f\xbf\x7f",                0x1fdf9f1e1c181ULL);
  TEST("\x81\x83\x87\x8f\x9f\xbf\xff\x01",            0x3fdf9f1e1c181ULL);
  TEST("\x81\x83\x87\x8f\x9f\xbf\xff\x81\x03",      0x303fdf9f1e1c181ULL);
  TEST("\x81\x83\x87\x8f\x9f\xbf\xff\x81\x83\x07", 0x8303fdf9f1e1c181ULL);
#undef TEST

  for (num = 5; num * 1.5 < UINT64_MAX; num *= 1.5) {
    test_varint_for_num(decoder, num);
  }
  test_varint_for_num(decoder, 0);
}


#define TEST_VARINT_DECODER(decoder) \
  /* Create non-inline versions for convenient inspection of assembly language \
   * output. */ \
  upb_decoderet _upb_vdecode_ ## decoder(const char *p) { \
    return upb_vdecode_ ## decoder(p); \
  } \
  void test_ ## decoder(void) { \
    test_varint_decoder(&_upb_vdecode_ ## decoder); \
  } \

TEST_VARINT_DECODER(check2_branch32)
TEST_VARINT_DECODER(check2_branch64)

int run_tests(int argc, char *argv[]) {
  UPB_UNUSED(argc);
  UPB_UNUSED(argv);
  test_check2_branch32();
  test_check2_branch64();
  return 0;
}
