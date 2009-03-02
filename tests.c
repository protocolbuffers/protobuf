#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include "pbstream.c"

void test_get_v_uint64_t()
{
  enum pbstream_status status;

  char zero[] = {0x00};
  char *zero_buf = zero;
  uint64_t zero_val = 0;
  status = get_v_uint64_t(&zero_buf, &zero_val);
  assert(status == PBSTREAM_STATUS_OK);
  assert(zero_val == 0);
  assert(zero_buf == zero + sizeof(zero));

  char one[] = {0x01};
  char *one_buf = one;
  uint64_t one_val = 0;
  status = get_v_uint64_t(&one_buf, &one_val);
  assert(status == PBSTREAM_STATUS_OK);
  assert(one_val == 1);

  char twobyte[] = {0xAC, 0x02};
  char *twobyte_buf = twobyte;
  uint64_t twobyte_val = 0;
  status = get_v_uint64_t(&twobyte_buf, &twobyte_val);
  assert(status == PBSTREAM_STATUS_OK);
  assert(twobyte_val == 300);

  char ninebyte[] = {0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x7F};
  char *ninebyte_buf = ninebyte;
  uint64_t ninebyte_val = 0;
  status = get_v_uint64_t(&ninebyte_buf, &ninebyte_val);
  assert(status == PBSTREAM_STATUS_OK);
  assert(ninebyte_val == (1LL<<63));

  char tenbyte[] = {0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x01};
  char *tenbyte_buf = tenbyte;
  uint64_t tenbyte_val = 0;
  status = get_v_uint64_t(&tenbyte_buf, &tenbyte_val);
  assert(status == PBSTREAM_ERROR_UNTERMINATED_VARINT);
}

void test_simple_proto()
{
  /* These are the examples from
   * http://code.google.com/apis/protocolbuffers/docs/encoding.html */
  struct pbstream_field *fields1 = malloc(sizeof(struct pbstream_field[2]));
  fields1[0].field_number = 1;
  fields1[0].type = PBSTREAM_TYPE_INT32;
  fields1[1].field_number = 2;
  fields1[1].type = PBSTREAM_TYPE_STRING;
  struct pbstream_fieldset fieldset1;
  pbstream_init_fieldset(&fieldset1, fields1, 2);

  char message1[] = {0x08, 0x96, 0x01};
  struct pbstream_parse_state s;
  pbstream_init_parser(&s, &fieldset1);
  assert(s.offset == 0);
  pbstream_field_number_t fieldnum;
  struct pbstream_tagged_value val;
  struct pbstream_tagged_wire_value wv;
  assert(pbstream_parse_field(&s, message1, &fieldnum, &val, &wv) ==
         PBSTREAM_STATUS_OK);
  assert(val.field->field_number == 1);
  assert(val.v.int32 == 150);
  assert(s.offset == 3);

  char message2[] = {0x12, 0x07, 0x74, 0x65, 0x73, 0x74, 0x69, 0x6e, 0x67};
  pbstream_init_parser(&s, &fieldset1);
  assert(pbstream_parse_field(&s, message2, &fieldnum, &val, &wv) ==
         PBSTREAM_STATUS_OK);
  assert(val.field->field_number == 2);
  assert(val.v.delimited.offset == 2);
  assert(val.v.delimited.len == 7);
  assert(s.offset == 9);

  struct pbstream_field *fields2 = malloc(sizeof(struct pbstream_field[1]));
  fields2[0].field_number = 3;
  fields2[0].type = PBSTREAM_TYPE_MESSAGE;
  fields2[0].fieldset = &fieldset1;
  struct pbstream_fieldset fieldset2;
  pbstream_init_fieldset(&fieldset2, fields2, 1);
  char message3[] = {0x1a, 0x03, 0x08, 0x96, 0x01};
  pbstream_init_parser(&s, &fieldset2);
  assert(pbstream_parse_field(&s, message3, &fieldnum, &val, &wv) ==
         PBSTREAM_STATUS_OK);
  assert(val.field->field_number == 3);
  assert(val.v.delimited.offset == 2);
  assert(val.v.delimited.len == 3);
  assert(s.offset == 2);
  assert(s.top-1 == s.stack);
  assert(s.top->fieldset == &fieldset1);
  assert(s.top->end_offset == 5);

  assert(pbstream_parse_field(&s, message3+s.offset, &fieldnum, &val, &wv) ==
         PBSTREAM_STATUS_OK);
  assert(val.field->field_number == 1);
  assert(val.v.int32 == 150);
  assert(s.offset == 5);

  assert(pbstream_parse_field(&s, NULL /* shouldn't be read */,
                              &fieldnum, &val, &wv) ==
         PBSTREAM_STATUS_SUBMESSAGE_END);
  assert(s.top == s.stack);

  pbstream_free_fieldset(&fieldset1);
  pbstream_free_fieldset(&fieldset2);
  free(fields1);
  free(fields2);
}

int main()
{
  test_get_v_uint64_t();
  test_simple_proto();
  printf("All tests passed.\n");
  return 0;
}
