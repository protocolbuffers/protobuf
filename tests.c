#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include "upb_parse.c"

void test_get_v_uint64_t()
{
  upb_status_t status;

  uint8_t zero[] = {0x00};
  void *zero_buf = zero;
  uint64_t zero_val = 0;
  status = get_v_uint64_t(&zero_buf, sizeof(zero), &zero_val);
  assert(status == UPB_STATUS_OK);
  assert(zero_val == 0);
  assert(zero_buf == zero + sizeof(zero));

  uint8_t one[] = {0x01};
  void *one_buf = one;
  uint64_t one_val = 0;
  status = get_v_uint64_t(&one_buf, sizeof(one), &one_val);
  assert(status == UPB_STATUS_OK);
  assert(one_val == 1);
  assert(one_buf == one + sizeof(one));

  uint8_t twobyte[] = {0xAC, 0x02};
  void *twobyte_buf = twobyte;
  uint64_t twobyte_val = 0;
  status = get_v_uint64_t(&twobyte_buf, sizeof(twobyte), &twobyte_val);
  assert(status == UPB_STATUS_OK);
  assert(twobyte_val == 300);
  assert(twobyte_buf == twobyte + sizeof(twobyte));

  uint8_t tenbyte[] = {0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x7F};
  void *tenbyte_buf = tenbyte;
  uint64_t tenbyte_val = 0;
  status = get_v_uint64_t(&tenbyte_buf, sizeof(tenbyte), &tenbyte_val);
  assert(status == UPB_STATUS_OK);
  assert(tenbyte_val == 0x89101c305080c101);
  assert(tenbyte_buf == tenbyte + sizeof(tenbyte));

  uint8_t elevenbyte[] = {0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x01};
  void *elevenbyte_buf = elevenbyte;
  uint64_t elevenbyte_val = 0;
  status = get_v_uint64_t(&elevenbyte_buf, sizeof(elevenbyte), &elevenbyte_val);
  assert(status == UPB_ERROR_UNTERMINATED_VARINT);
  status = get_v_uint64_t(&elevenbyte_buf, sizeof(elevenbyte)-1, &elevenbyte_val);
  /* Byte 10 is 0x80, so we know it's unterminated. */
  assert(status == UPB_ERROR_UNTERMINATED_VARINT);
  status = get_v_uint64_t(&elevenbyte_buf, sizeof(elevenbyte)-2, &elevenbyte_val);
  assert(status == UPB_STATUS_NEED_MORE_DATA);
}

int main()
{
  test_get_v_uint64_t();
  printf("All tests passed.\n");
  return 0;
}
