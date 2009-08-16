
#undef NDEBUG  /* ensure tests always assert. */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include "upb_parse.h"
#include "upb_context.h"

int num_assertions = 0;
#define ASSERT(expr) do { \
  ++num_assertions; \
  assert(expr); \
  } while(0)

static void test_get_v_uint64_t()
{
#define TEST(name, bytes, val) {\
    upb_status_t status; \
    uint8_t name[] = bytes; \
    uint8_t *name ## _buf = name; \
    uint64_t name ## _val = 0; \
    status = upb_get_v_uint64_t(name, name + sizeof(name) - 1, &name ## _val, &name ## _buf); \
    ASSERT(status == UPB_STATUS_OK); \
    ASSERT(name ## _val == val); \
    ASSERT(name ## _buf == name + sizeof(name) - 1);  /* - 1 for NULL */ \
    /* Test NEED_MORE_DATA. */ \
    if(sizeof(name) > 2) { \
      status = upb_get_v_uint64_t(name, name + sizeof(name) - 2, &name ## _val, &name ## _buf); \
      ASSERT(status == UPB_STATUS_NEED_MORE_DATA); \
    } \
  }

  TEST(zero,   "\x00",                                                      0ULL);
  TEST(one,    "\x01",                                                      1ULL);
  TEST(twob,   "\x81\x03",                                              0x181ULL);
  TEST(threeb, "\x81\x83\x07",                                        0x1c181ULL);
  TEST(fourb,  "\x81\x83\x87\x0f",                                  0x1e1c181ULL);
  TEST(fiveb,  "\x81\x83\x87\x8f\x1f",                            0x1f1e1c181ULL);
  TEST(sixb,   "\x81\x83\x87\x8f\x9f\x3f",                      0x1f9f1e1c181ULL);
  TEST(sevenb, "\x81\x83\x87\x8f\x9f\xbf\x7f",                0x1fdf9f1e1c181ULL);
  TEST(eightb, "\x81\x83\x87\x8f\x9f\xbf\xff\x01",            0x3fdf9f1e1c181ULL);
  TEST(nineb,  "\x81\x83\x87\x8f\x9f\xbf\xff\x81\x03",      0x303fdf9f1e1c181ULL);
  TEST(tenb,   "\x81\x83\x87\x8f\x9f\xbf\xff\x81\x83\x07", 0x8303fdf9f1e1c181ULL);
#undef TEST

  uint8_t twelvebyte[] = {0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x01, 0x01};
  uint8_t *twelvebyte_buf = twelvebyte;
  uint64_t twelvebyte_val = 0;
  upb_status_t status;
  /* A varint that terminates before hitting the end of the provided buffer,
   * but in too many bytes (11 instead of 10). */
  status = upb_get_v_uint64_t(twelvebyte_buf, twelvebyte + 12, &twelvebyte_val, &twelvebyte_buf);
  ASSERT(status == UPB_ERROR_UNTERMINATED_VARINT);

  /* A varint that terminates simultaneously with the end of the provided
   * buffer, but in too many bytes (11 instead of 10). */
  status = upb_get_v_uint64_t(twelvebyte_buf, twelvebyte + 11, &twelvebyte_val, &twelvebyte_buf);
  ASSERT(status == UPB_ERROR_UNTERMINATED_VARINT);

  /* A varint whose buffer ends on exactly the byte where the varint must
   * terminate, but the final byte does not terminate.  The absolutely most
   * correct return code here is UPB_ERROR_UNTERMINATED_VARINT, because we know
   * by this point that the varint does not properly terminate.  But we also
   * allow a return value of UPB_STATUS_NEED_MORE_DATA here, because it does not
   * compromise overall correctness -- clients who supply more data later will
   * then receive a UPB_ERROR_UNTERMINATED_VARINT error; clients who have no
   * more data to supply will (rightly) conclude that their protobuf is corrupt.
   */
  status = upb_get_v_uint64_t(twelvebyte_buf, twelvebyte + 10, &twelvebyte_val, &twelvebyte_buf);
  ASSERT(status == UPB_ERROR_UNTERMINATED_VARINT ||
         status == UPB_STATUS_NEED_MORE_DATA);

  status = upb_get_v_uint64_t(twelvebyte_buf, twelvebyte + 9, &twelvebyte_val, &twelvebyte_buf);
  ASSERT(status == UPB_STATUS_NEED_MORE_DATA);
}

static void test_get_v_uint32_t()
{
#define TEST(name, bytes, val) {\
    upb_status_t status; \
    uint8_t name[] = bytes; \
    uint8_t *name ## _buf = name; \
    uint32_t name ## _val = 0; \
    status = upb_get_v_uint32_t(name, name + sizeof(name), &name ## _val, &name ## _buf); \
    ASSERT(status == UPB_STATUS_OK); \
    ASSERT(name ## _val == val); \
    ASSERT(name ## _buf == name + sizeof(name) - 1);  /* - 1 for NULL */ \
    /* Test NEED_MORE_DATA. */ \
    if(sizeof(name) > 2) { \
      status = upb_get_v_uint32_t(name, name + sizeof(name) - 2, &name ## _val, &name ## _buf); \
      ASSERT(status == UPB_STATUS_NEED_MORE_DATA); \
    } \
  }

  TEST(zero,   "\x00",                                              0UL);
  TEST(one,    "\x01",                                              1UL);
  TEST(twob,   "\x81\x03",                                      0x181UL);
  TEST(threeb, "\x81\x83\x07",                                0x1c181UL);
  TEST(fourb,  "\x81\x83\x87\x0f",                          0x1e1c181UL);
  /* get_v_uint32_t truncates, so all the rest return the same thing. */
  TEST(fiveb,  "\x81\x83\x87\x8f\x1f",                     0xf1e1c181UL);
  TEST(sixb,   "\x81\x83\x87\x8f\x9f\x3f",                 0xf1e1c181UL);
  TEST(sevenb, "\x81\x83\x87\x8f\x9f\xbf\x7f",             0xf1e1c181UL);
  TEST(eightb, "\x81\x83\x87\x8f\x9f\xbf\xff\x01",         0xf1e1c181UL);
  TEST(nineb,  "\x81\x83\x87\x8f\x9f\xbf\xff\x81\x03",     0xf1e1c181UL);
  TEST(tenb,   "\x81\x83\x87\x8f\x9f\xbf\xff\x81\x83\x07", 0xf1e1c181UL);
#undef TEST

  uint8_t twelvebyte[] = {0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x01, 0x01};
  uint8_t *twelvebyte_buf = twelvebyte;
  uint32_t twelvebyte_val = 0;
  upb_status_t status;
  /* A varint that terminates before hitting the end of the provided buffer,
   * but in too many bytes (11 instead of 10). */
  status = upb_get_v_uint32_t(twelvebyte_buf, twelvebyte + 12, &twelvebyte_val, &twelvebyte_buf);
  ASSERT(status == UPB_ERROR_UNTERMINATED_VARINT);

  /* A varint that terminates simultaneously with the end of the provided
   * buffer, but in too many bytes (11 instead of 10). */
  status = upb_get_v_uint32_t(twelvebyte_buf, twelvebyte + 11, &twelvebyte_val, &twelvebyte_buf);
  ASSERT(status == UPB_ERROR_UNTERMINATED_VARINT);

  /* A varint whose buffer ends on exactly the byte where the varint must
   * terminate, but the final byte does not terminate.  The absolutely most
   * correct return code here is UPB_ERROR_UNTERMINATED_VARINT, because we know
   * by this point that the varint does not properly terminate.  But we also
   * allow a return value of UPB_STATUS_NEED_MORE_DATA here, because it does not
   * compromise overall correctness -- clients who supply more data later will
   * then receive a UPB_ERROR_UNTERMINATED_VARINT error; clients who have no
   * more data to supply will (rightly) conclude that their protobuf is corrupt.
   */
  status = upb_get_v_uint32_t(twelvebyte_buf, twelvebyte + 10, &twelvebyte_val, &twelvebyte_buf);
  ASSERT(status == UPB_ERROR_UNTERMINATED_VARINT ||
         status == UPB_STATUS_NEED_MORE_DATA);

  status = upb_get_v_uint32_t(twelvebyte_buf, twelvebyte + 9, &twelvebyte_val, &twelvebyte_buf);
  ASSERT(status == UPB_STATUS_NEED_MORE_DATA);
}

static void test_skip_v_uint64_t()
{
#define TEST(name, bytes) {\
    upb_status_t status; \
    uint8_t name[] = bytes; \
    uint8_t *name ## _buf = name; \
    status = upb_skip_v_uint64_t(name ## _buf, name + sizeof(name), &name ## _buf); \
    ASSERT(status == UPB_STATUS_OK); \
    ASSERT(name ## _buf == name + sizeof(name) - 1);  /* - 1 for NULL */ \
    /* Test NEED_MORE_DATA. */ \
    if(sizeof(name) > 2) { \
      status = upb_skip_v_uint64_t(name, name + sizeof(name) - 2, &name ## _buf); \
      ASSERT(status == UPB_STATUS_NEED_MORE_DATA); \
    } \
  }

  TEST(zero,   "\x00");
  TEST(one,    "\x01");
  TEST(twob,   "\x81\x03");
  TEST(threeb, "\x81\x83\x07");
  TEST(fourb,  "\x81\x83\x87\x0f");
  TEST(fiveb,  "\x81\x83\x87\x8f\x1f");
  TEST(sixb,   "\x81\x83\x87\x8f\x9f\x3f");
  TEST(sevenb, "\x81\x83\x87\x8f\x9f\xbf\x7f");
  TEST(eightb, "\x81\x83\x87\x8f\x9f\xbf\xff\x01");
  TEST(nineb,  "\x81\x83\x87\x8f\x9f\xbf\xff\x81\x03");
  TEST(tenb,   "\x81\x83\x87\x8f\x9f\xbf\xff\x81\x83\x07");
#undef TEST

  uint8_t twelvebyte[] = {0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x01, 0x01};
  uint8_t *twelvebyte_buf = twelvebyte;
  upb_status_t status;
  /* A varint that terminates before hitting the end of the provided buffer,
   * but in too many bytes (11 instead of 10). */
  status = upb_skip_v_uint64_t(twelvebyte_buf, twelvebyte + 12, &twelvebyte_buf);
  ASSERT(status == UPB_ERROR_UNTERMINATED_VARINT);

  /* A varint that terminates simultaneously with the end of the provided
   * buffer, but in too many bytes (11 instead of 10). */
  status = upb_skip_v_uint64_t(twelvebyte_buf, twelvebyte + 11, &twelvebyte_buf);
  ASSERT(status == UPB_ERROR_UNTERMINATED_VARINT);

  /* A varint whose buffer ends on exactly the byte where the varint must
   * terminate, but the final byte does not terminate.  The absolutely most
   * correct return code here is UPB_ERROR_UNTERMINATED_VARINT, because we know
   * by this point that the varint does not properly terminate.  But we also
   * allow a return value of UPB_STATUS_NEED_MORE_DATA here, because it does not
   * compromise overall correctness -- clients who supply more data later will
   * then receive a UPB_ERROR_UNTERMINATED_VARINT error; clients who have no
   * more data to supply will (rightly) conclude that their protobuf is corrupt.
   */
  status = upb_skip_v_uint64_t(twelvebyte_buf, twelvebyte + 10, &twelvebyte_buf);
  ASSERT(status == UPB_ERROR_UNTERMINATED_VARINT ||
         status == UPB_STATUS_NEED_MORE_DATA);

  status = upb_skip_v_uint64_t(twelvebyte_buf, twelvebyte + 9, &twelvebyte_buf);
  ASSERT(status == UPB_STATUS_NEED_MORE_DATA);
}

static void test_get_f_uint32_t()
{
#define TEST(name, bytes, val) {\
    upb_status_t status; \
    uint8_t name[] = bytes; \
    uint8_t *name ## _buf = name; \
    uint32_t name ## _val = 0; \
    status = upb_get_f_uint32_t(name ## _buf, name + sizeof(name), &name ## _val, &name ## _buf); \
    ASSERT(status == UPB_STATUS_OK); \
    ASSERT(name ## _val == val); \
    ASSERT(name ## _buf == name + sizeof(name) - 1);  /* - 1 for NULL */ \
  }

  TEST(zero,  "\x00\x00\x00\x00",                                0x0UL);
  TEST(one,   "\x01\x00\x00\x00",                                0x1UL);

  uint8_t threeb[] = {0x00, 0x00, 0x00};
  uint8_t *threeb_buf = threeb;
  uint32_t threeb_val;
  upb_status_t status = upb_get_f_uint32_t(threeb, threeb + sizeof(threeb), &threeb_val, &threeb_buf);
  ASSERT(status == UPB_STATUS_NEED_MORE_DATA);

#undef TEST
}

static void test_upb_context() {
  struct upb_context c;
  ASSERT(upb_context_init(&c));
  upb_context_unref(&c);
}

int main()
{
#define TEST(func) do { \
  int assertions_before = num_assertions; \
  printf("Running " #func "..."); fflush(stdout); \
  func(); \
  printf("ok (%d assertions).\n", num_assertions - assertions_before); \
  } while (0)

  TEST(test_get_v_uint64_t);
  TEST(test_get_v_uint32_t);
  TEST(test_skip_v_uint64_t);
  TEST(test_get_f_uint32_t);
  TEST(test_upb_context);
  printf("All tests passed (%d assertions).\n", num_assertions);
  return 0;
}
