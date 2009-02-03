#include <assert.h>
#include <stdio.h>
#include "pbstream.c"

void test_get_varint()
{
  enum pbstream_status status;

  char zero[] = {0x00};
  char *zero_buf = zero;
  uint64_t zero_val;
  status = get_varint(&zero_buf, zero_buf+sizeof(zero), &zero_val);
  assert(status == PBSTREAM_STATUS_OK);
  assert(zero_val == 0);
  assert(zero_buf == zero + sizeof(zero));

  char one[] = {0x01};
  char *one_buf = one;
  uint64_t one_val;
  status = get_varint(&one_buf, one_buf+sizeof(one), &one_val);
  assert(status == PBSTREAM_STATUS_OK);
  assert(one_val == 1);

  char twobyte[] = {0xAC, 0x02};
  char *twobyte_buf = twobyte;
  uint64_t twobyte_val;
  status = get_varint(&twobyte_buf, twobyte_buf+sizeof(twobyte), &twobyte_val);
  assert(status == PBSTREAM_STATUS_OK);
  assert(twobyte_val == 300);

  char ninebyte[] = {0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x7F};
  char *ninebyte_buf = ninebyte;
  uint64_t ninebyte_val;
  status = get_varint(&ninebyte_buf, ninebyte_buf+sizeof(ninebyte), &ninebyte_val);
  assert(status == PBSTREAM_STATUS_OK);
  assert(ninebyte_val == (1LL<<63));

  char overrun[] = {0x80, 0x01};
  char *overrun_buf = overrun;
  uint64_t overrun_val;
  status = get_varint(&overrun_buf, overrun_buf+sizeof(overrun)-1, &overrun_val);
  assert(status == PBSTREAM_STATUS_INCOMPLETE);

  char tenbyte[] = {0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x01};
  char *tenbyte_buf = tenbyte;
  uint64_t tenbyte_val;
  status = get_varint(&tenbyte_buf, tenbyte_buf+sizeof(tenbyte), &tenbyte_val);
  assert(status == PBSTREAM_ERROR_UNTERMINATED_VARINT);
}

int main()
{
  test_get_varint();
  printf("All tests passed.\n");
  return 0;
}
