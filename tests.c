#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include "upb_parse.c"

void test_get_v_uint64_t()
{
  upb_status_t status;

  uint8_t zero[] = {0x00};
  uint8_t *zero_buf = zero;
  uint64_t zero_val = 0;
  status = get_v_uint64_t(&zero_buf, &zero_val);
  assert(status == UPB_STATUS_OK);
  assert(zero_val == 0);
  assert(zero_buf == zero + sizeof(zero));

  uint8_t one[] = {0x01};
  uint8_t *one_buf = one;
  uint64_t one_val = 0;
  status = get_v_uint64_t(&one_buf, &one_val);
  assert(status == UPB_STATUS_OK);
  assert(one_val == 1);

  uint8_t twobyte[] = {0xAC, 0x02};
  uint8_t *twobyte_buf = twobyte;
  uint64_t twobyte_val = 0;
  status = get_v_uint64_t(&twobyte_buf, &twobyte_val);
  assert(status == UPB_STATUS_OK);
  assert(twobyte_val == 300);

  uint8_t tenbyte[] = {0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x7F};
  uint8_t *tenbyte_buf = tenbyte;
  uint64_t tenbyte_val = 0;
  status = get_v_uint64_t(&tenbyte_buf, &tenbyte_val);
  assert(status == UPB_STATUS_OK);
  assert(tenbyte_val == 0x89101c305080c101);

  uint8_t elevenbyte[] = {0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x01};
  uint8_t *elevenbyte_buf = elevenbyte;
  uint64_t elevenbyte_val = 0;
  status = get_v_uint64_t(&elevenbyte_buf, &elevenbyte_val);
  assert(status == UPB_ERROR_UNTERMINATED_VARINT);
}

int main()
{
  test_get_v_uint64_t();
  printf("All tests passed.\n");
  return 0;
}
