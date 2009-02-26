#include <assert.h>
#include <stdio.h>
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
  struct pbstream_fieldset *fieldset1 = malloc(sizeof(*fieldset1) +
                                               2*sizeof(struct pbstream_field));
  fieldset1->num_fields = 2;
  fieldset1->fields[0].field_number = 1;
  fieldset1->fields[0].type = PBSTREAM_TYPE_INT32;
  fieldset1->fields[1].field_number = 2;
  fieldset1->fields[1].type = PBSTREAM_TYPE_STRING;

  char message1[] = {0x08, 0x96, 0x01};
  struct pbstream_parse_state s;
  pbstream_init_parser(&s, fieldset1);
  assert(s.offset == 0);
  pbstream_field_number_t fieldnum;
  struct pbstream_value val;
  struct pbstream_wire_value wv;
  assert(pbstream_parse_field(&s, message1, &fieldnum, &val, &wv) ==
         PBSTREAM_STATUS_OK);
  assert(val.field == &fieldset1->fields[0]);
  assert(val.v.int32 == 150);
  assert(s.offset == 3);
  pbstream_free_parser(&s);

  char message2[] = {0x12, 0x07, 0x74, 0x65, 0x73, 0x74, 0x69, 0x6e, 0x67};
  pbstream_init_parser(&s, fieldset1);
  assert(pbstream_parse_field(&s, message2, &fieldnum, &val, &wv) ==
         PBSTREAM_STATUS_OK);
  assert(val.field == &fieldset1->fields[1]);
  assert(val.v.delimited.offset == 2);
  assert(val.v.delimited.len == 7);
  assert(s.offset == 9);
  pbstream_free_parser(&s);

  struct pbstream_fieldset *fieldset2 = malloc(sizeof(*fieldset1) +
                                               3*sizeof(struct pbstream_field));
  fieldset2->num_fields = 3;
  fieldset2->fields[2].field_number = 3;
  fieldset2->fields[2].type = PBSTREAM_TYPE_MESSAGE;
  fieldset2->fields[2].fieldset = fieldset1;
  char message3[] = {0x1a, 0x03, 0x08, 0x96, 0x01};
  pbstream_init_parser(&s, fieldset2);
  assert(pbstream_parse_field(&s, message3, &fieldnum, &val, &wv) ==
         PBSTREAM_STATUS_OK);
  assert(val.field == &fieldset2->fields[2]);
  assert(val.v.delimited.offset == 2);
  assert(val.v.delimited.len == 3);
  assert(s.offset == 2);
  assert(s.top-1 == s.base);
  assert(s.top->fieldset == fieldset1);
  assert(s.top->end_offset == 5);

  assert(pbstream_parse_field(&s, message3+s.offset, &fieldnum, &val, &wv) ==
         PBSTREAM_STATUS_OK);
  assert(val.field == &fieldset1->fields[0]);
  assert(val.v.int32 == 150);
  assert(s.offset == 5);

  assert(pbstream_parse_field(&s, NULL /* shouldn't be read */,
                              &fieldnum, &val, &wv) ==
         PBSTREAM_STATUS_SUBMESSAGE_END);
  assert(s.top == s.base);
  pbstream_free_parser(&s);

  free(fieldset1);
  free(fieldset2);
}

int main()
{
  test_get_v_uint64_t();
  test_simple_proto();
  printf("All tests passed.\n");
  return 0;
}
