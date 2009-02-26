/*
 * pbstream - a stream-oriented implementation of protocol buffers.
 *
 * Copyright (c) 2008-2009 Joshua Haberman.  See LICENSE for details.
 */

#include <stdlib.h>
#include <string.h>
#include "pbstream.h"

/* Branch prediction hints for GCC. */
#ifdef __GNUC__
#define likely(x)       __builtin_expect((x),1)
#define unlikely(x)     __builtin_expect((x),0)
#else
#define likely(x)       (x)
#define unlikely(x)     (x)
#endif

#define CHECK(func) do { \
  pbstream_wire_type_t status = func; \
  if(status != PBSTREAM_STATUS_OK) return status; \
  } while (0)

/* Lowest-level functions -- these read integers from the input buffer.
 * To avoid branches, none of these do bounds checking.  So we force clients
 * to overallocate their buffers by >=9 bytes. */

static pbstream_status_t get_v_uint64_t(char **buf, uint64_t *val)
{
  uint8_t* ptr = (uint8_t*)*buf;
  uint32_t b;
  uint32_t part0 = 0, part1 = 0, part2 = 0;

  /* From the original proto2 implementation. */
  b = *(ptr++); part0  = (b & 0x7F)      ; if (!(b & 0x80)) goto done;
  b = *(ptr++); part0 |= (b & 0x7F) <<  7; if (!(b & 0x80)) goto done;
  b = *(ptr++); part0 |= (b & 0x7F) << 14; if (!(b & 0x80)) goto done;
  b = *(ptr++); part0 |= (b & 0x7F) << 21; if (!(b & 0x80)) goto done;
  b = *(ptr++); part1  = (b & 0x7F)      ; if (!(b & 0x80)) goto done;
  b = *(ptr++); part1 |= (b & 0x7F) <<  7; if (!(b & 0x80)) goto done;
  b = *(ptr++); part1 |= (b & 0x7F) << 14; if (!(b & 0x80)) goto done;
  b = *(ptr++); part1 |= (b & 0x7F) << 21; if (!(b & 0x80)) goto done;
  b = *(ptr++); part2  = (b & 0x7F)      ; if (!(b & 0x80)) goto done;
  b = *(ptr++); part2 |= (b & 0x7F) <<  7; if (!(b & 0x80)) goto done;
  return PBSTREAM_ERROR_UNTERMINATED_VARINT;

done:
  *buf = (char*)ptr;
  *val = (uint64_t)part0 | ((uint64_t)part1 << 28) | ((uint64_t)part2 << 56);
  return PBSTREAM_STATUS_OK;
}

static pbstream_status_t get_v_uint32_t(char **buf, uint32_t *val)
{
  uint8_t* ptr = (uint8_t*)*buf;
  uint32_t b;
  uint32_t result;

  /* From the original proto2 implementation. */
  b = *(ptr++); result  = (b & 0x7F)      ; if (!(b & 0x80)) goto done;
  b = *(ptr++); result |= (b & 0x7F) <<  7; if (!(b & 0x80)) goto done;
  b = *(ptr++); result |= (b & 0x7F) << 14; if (!(b & 0x80)) goto done;
  b = *(ptr++); result |= (b & 0x7F) << 21; if (!(b & 0x80)) goto done;
  b = *(ptr++); result  = (b & 0x7F) << 28; if (!(b & 0x80)) goto done;
  return PBSTREAM_ERROR_UNTERMINATED_VARINT;

done:
  *buf = (char*)ptr;
  *val = result;
  return PBSTREAM_STATUS_OK;
}

static pbstream_status_t get_f_uint32_t(char **buf, uint32_t *val)
{
  uint8_t *b = (uint8_t*)*buf;
#if __BYTE_ORDER == __LITTLE_ENDIAN
  *val = *(uint32_t*)b;  /* likely unaligned, TODO: verify performance. */
#else
  *val = b[0] | (b[1] << 8) | (b[2] << 16) | (b[3] << 24);
#endif
  *buf = (char*)b + sizeof(uint32_t);
  return PBSTREAM_STATUS_OK;
}

static pbstream_status_t get_f_uint64_t(char **buf, uint64_t *val)
{
  uint8_t *b = (uint8_t*)*buf;
#if __BYTE_ORDER == __LITTLE_ENDIAN
  *val = *(uint64_t*)buf;  /* likely unaligned, TODO: verify performance. */
#else
  *val = (b[0])       | (b[1] << 8 ) | (b[2] << 16) | (b[3] << 24) |
         (b[4] << 32) | (b[5] << 40) | (b[6] << 48) | (b[7] << 56);
#endif
  *buf = (char*)b + sizeof(uint64_t);
  return PBSTREAM_STATUS_OK;
}

static int32_t zz_decode_32(uint32_t n) { return (n >> 1) ^ -(int32_t)(n & 1); }
static int64_t zz_decode_64(uint64_t n) { return (n >> 1) ^ -(int64_t)(n & 1); }

/* Functions for reading wire values and converting them to values.  These
 * are generated with macros because they follow a higly consistent pattern. */

/* WVTOV() generates a function:
 *   void wvtov_TYPE(wire_t src, val_t *dst, size_t offset)
 * (macro invoker defines the body of the function).  */
#define WVTOV(type, wire_t, val_t) \
  static void wvtov_ ## type(wire_t s, val_t *d, size_t offset)

/* GET() generates a function:
 *   pbstream_status_t get_TYPE(char **buf, char *end, size_t offset,
 *                              pbstream_value *dst) */
#define GET(type, v_or_f, wire_t, val_t, member_name) \
  static pbstream_status_t get_ ## type(struct pbstream_parse_state *s, \
                                        char *buf, \
                                        struct pbstream_value *d) { \
    wire_t tmp; \
    char *b = buf; \
    CHECK(get_ ## v_or_f ## _ ## wire_t(&b, &tmp)); \
    wvtov_ ## type(tmp, &d->v.member_name, s->offset); \
    s->offset += (b-buf); \
    return PBSTREAM_STATUS_OK; \
  }

#define T(type, v_or_f, wire_t, val_t, member_name) \
  WVTOV(type, wire_t, val_t);  /* prototype for GET below */ \
  GET(type, v_or_f, wire_t, val_t, member_name) \
  WVTOV(type, wire_t, val_t)

T(DOUBLE,   f, uint64_t, double,   _double) { memcpy(d, &s, sizeof(double)); }
T(FLOAT,    f, uint32_t, float,    _float)  { memcpy(d, &s, sizeof(float));  }
T(INT32,    v, uint32_t, int32_t,  int32)   { *d = (int32_t)s;               }
T(INT64,    v, uint64_t, int64_t,  int64)   { *d = (int64_t)s;               }
T(UINT32,   v, uint32_t, uint32_t, uint32)  { *d = s;                        }
T(UINT64,   v, uint64_t, uint64_t, uint64)  { *d = s;                        }
T(SINT32,   v, uint32_t, int32_t,  int32)   { *d = zz_decode_32(s);          }
T(SINT64,   v, uint64_t, int64_t,  int64)   { *d = zz_decode_64(s);          }
T(FIXED32,  f, uint32_t, uint32_t, uint32)  { *d = s;                        }
T(FIXED64,  f, uint64_t, uint64_t, uint64)  { *d = s;                        }
T(SFIXED32, f, uint32_t, int32_t,  int32)   { *d = (int32_t)s;               }
T(SFIXED64, f, uint64_t, int64_t,  int64)   { *d = (int64_t)s;               }
T(BOOL,     v, uint32_t, bool,     _bool)   { *d = (bool)s;                  }
T(ENUM,     v, uint32_t, int32_t,  _enum)   { *d = (int32_t)s;               }
#undef WVTOV
#undef GET
#undef T

static void wvtov_delimited(uint32_t s, struct pbstream_delimited *d, size_t o)
{
  d->offset = o;
  d->len = s;
}

/* Use BYTES version for both STRING and BYTES, leave UTF-8 checks to client. */
static pbstream_status_t get_BYTES(struct pbstream_parse_state *s, char *buf,
                                   struct pbstream_value *d) {
  uint32_t tmp;
  char *b = buf;
  CHECK(get_v_uint32_t(&b, &tmp));
  s->offset += (b-buf);  /* advance past length varint. */
  wvtov_delimited(tmp, &d->v.delimited, s->offset);
  s->offset = d->v.delimited.offset + d->v.delimited.len; /* skip bytes */
  return PBSTREAM_STATUS_OK;
}

static pbstream_status_t get_MESSAGE(struct pbstream_parse_state *s, char *buf,
                                     struct pbstream_value *d) {
  /* We're entering a sub-message. */
  uint32_t tmp;
  char *b = buf;
  CHECK(get_v_uint32_t(&b, &tmp));
  s->offset += (b-buf);  /* advance past length varint. */
  wvtov_delimited(tmp, &d->v.delimited, s->offset);
  /* Unlike STRING and BYTES, we *don't* advance past delimited here. */
  if (unlikely(++s->top == s->limit)) return PBSTREAM_ERROR_STACK_OVERFLOW;
  s->top->fieldset = d->field->fieldset;
  s->top->end_offset = d->v.delimited.offset + d->v.delimited.len;
  return PBSTREAM_STATUS_OK;
}

struct pbstream_type_info {
  pbstream_wire_type_t expected_wire_type;
  pbstream_status_t (*get)(struct pbstream_parse_state *s, char *buf,
                           struct pbstream_value *d);
};
static struct pbstream_type_info type_info[] = {
  {PBSTREAM_WIRE_TYPE_64BIT,     get_DOUBLE},
  {PBSTREAM_WIRE_TYPE_32BIT,     get_FLOAT},
  {PBSTREAM_WIRE_TYPE_VARINT,    get_INT32},
  {PBSTREAM_WIRE_TYPE_VARINT,    get_INT64},
  {PBSTREAM_WIRE_TYPE_VARINT,    get_UINT32},
  {PBSTREAM_WIRE_TYPE_VARINT,    get_UINT64},
  {PBSTREAM_WIRE_TYPE_VARINT,    get_SINT32},
  {PBSTREAM_WIRE_TYPE_VARINT,    get_SINT64},
  {PBSTREAM_WIRE_TYPE_32BIT,     get_FIXED32},
  {PBSTREAM_WIRE_TYPE_64BIT,     get_FIXED64},
  {PBSTREAM_WIRE_TYPE_32BIT,     get_SFIXED32},
  {PBSTREAM_WIRE_TYPE_64BIT,     get_SFIXED64},
  {PBSTREAM_WIRE_TYPE_VARINT,    get_BOOL},
  {PBSTREAM_WIRE_TYPE_DELIMITED, get_BYTES},
  {PBSTREAM_WIRE_TYPE_DELIMITED, get_BYTES},
  {PBSTREAM_WIRE_TYPE_VARINT,    get_ENUM},
  {PBSTREAM_WIRE_TYPE_DELIMITED, get_MESSAGE}
};

static pbstream_status_t parse_tag(char **buf, struct pbstream_tag *tag)
{
  uint32_t tag_int;
  CHECK(get_v_uint32_t(buf, &tag_int));
  tag->wire_type    = tag_int & 0x07;
  tag->field_number = tag_int >> 3;
  return PBSTREAM_STATUS_OK;
}

static pbstream_status_t parse_unknown_value(
    char **buf, int buf_offset, struct pbstream_wire_value *wv)
{
  switch(wv->type) {
    case PBSTREAM_WIRE_TYPE_VARINT:
      CHECK(get_v_uint64_t(buf, &wv->v.varint)); break;
    case PBSTREAM_WIRE_TYPE_64BIT:
      CHECK(get_f_uint64_t(buf, &wv->v._64bit)); break;
    case PBSTREAM_WIRE_TYPE_32BIT:
      CHECK(get_f_uint32_t(buf, &wv->v._32bit)); break;
    case PBSTREAM_WIRE_TYPE_DELIMITED:
      wv->v.delimited.offset = buf_offset;
      CHECK(get_v_uint32_t(buf, &wv->v.delimited.len));
      break;
    case PBSTREAM_WIRE_TYPE_START_GROUP:
    case PBSTREAM_WIRE_TYPE_END_GROUP:
      return PBSTREAM_ERROR_GROUP;  /* deprecated, no plans to support. */
  }
  return PBSTREAM_STATUS_OK;
}

static struct pbstream_field *find_field(struct pbstream_fieldset* fs,
                                         pbstream_field_number_t num)
{
  /* TODO: a hybrid array/hashtable structure. */
  /* TODO: can zero be a tag number? */
  if(num <= fs->num_fields) return &fs->fields[num-1];
  else return NULL;
}

/* Parses and processes the next value from buf. */
pbstream_status_t pbstream_parse_field(struct pbstream_parse_state *s,
                                       char *buf,
                                       pbstream_field_number_t *fieldnum,
                                       struct pbstream_value *val,
                                       struct pbstream_wire_value *wv)
{
  char *b = buf;
  /* Check for end-of-message at the current stack depth. */
  if(unlikely(s->offset >= s->top->end_offset)) {
    /* If the end offset isn't an exact field boundary, the pb is corrupt. */
    if(unlikely(s->offset != s->top->end_offset))
      return PBSTREAM_ERROR_BAD_SUBMESSAGE_END;
    s->top--;
    return PBSTREAM_STATUS_SUBMESSAGE_END;
  }

  struct pbstream_tag tag;
  CHECK(parse_tag(&b, &tag));
  s->offset += (b-buf);
  struct pbstream_field *fd = find_field(s->top->fieldset, tag.field_number);
  pbstream_status_t unknown_value_status;
  if(unlikely(!fd)) {
    unknown_value_status = PBSTREAM_ERROR_UNKNOWN_VALUE;
    goto unknown_value;
  }
  struct pbstream_type_info *info = &type_info[fd->type];
  if(unlikely(tag.wire_type != info->expected_wire_type)) {
    unknown_value_status = PBSTREAM_ERROR_MISMATCHED_TYPE;
    goto unknown_value;
  }

  *fieldnum = tag.field_number;
  val->field = fd;
  CHECK(info->get(s, b, val));
  return PBSTREAM_STATUS_OK;

unknown_value:
  wv->type = tag.wire_type;
  CHECK(parse_unknown_value(&b, s->offset, wv));
  s->offset += (b-buf);
  return unknown_value_status;
}

void pbstream_init_parser(
    struct pbstream_parse_state *state,
    struct pbstream_fieldset *toplevel_fieldset)
{
  state->offset = 0;
  /* We match proto2's limit of 64 for maximum stack depth. */
  state->top = state->base = malloc(64*sizeof(*state->base));
  state->limit = state->base + 64;
  state->top->fieldset = toplevel_fieldset;
  state->top->end_offset = SIZE_MAX;
}

void pbstream_free_parser(struct pbstream_parse_state *state)
{
  free(state->base);
}
