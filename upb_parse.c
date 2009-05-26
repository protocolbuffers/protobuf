/*
 * upb - a minimalist implementation of protocol buffers.
 *
 * Copyright (c) 2008-2009 Joshua Haberman.  See LICENSE for details.
 */

#include "upb_parse.h"

#include <assert.h>
#include <string.h>

/* Branch prediction hints for GCC. */
#ifdef __GNUC__
#define likely(x)       __builtin_expect((x),1)
#define unlikely(x)     __builtin_expect((x),0)
#else
#define likely(x)       (x)
#define unlikely(x)     (x)
#endif

#define CHECK(func) do { \
  upb_status_t status = func; \
  if(status != UPB_STATUS_OK) return status; \
  } while (0)

/* Lowest-level functions -- these read integers from the input buffer.
 * To avoid branches, none of these do bounds checking.  So we force clients
 * to overallocate their buffers by >=9 bytes. */

static upb_status_t get_v_uint64_t(uint8_t *restrict *buf,
                                   uint64_t *restrict val)
{
  uint8_t *ptr = *buf, b;
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
  return UPB_ERROR_UNTERMINATED_VARINT;

done:
  *buf = ptr;
  *val = (uint64_t)part0 | ((uint64_t)part1 << 28) | ((uint64_t)part2 << 56);
  return UPB_STATUS_OK;
}

static upb_status_t skip_v_uint64_t(uint8_t **buf)
{
  uint8_t *ptr = *buf, b;
  b = *(ptr++); if (!(b & 0x80)) goto done;
  b = *(ptr++); if (!(b & 0x80)) goto done;
  b = *(ptr++); if (!(b & 0x80)) goto done;
  b = *(ptr++); if (!(b & 0x80)) goto done;
  b = *(ptr++); if (!(b & 0x80)) goto done;
  b = *(ptr++); if (!(b & 0x80)) goto done;
  b = *(ptr++); if (!(b & 0x80)) goto done;
  b = *(ptr++); if (!(b & 0x80)) goto done;
  b = *(ptr++); if (!(b & 0x80)) goto done;
  b = *(ptr++); if (!(b & 0x80)) goto done;
  return UPB_ERROR_UNTERMINATED_VARINT;

done:
  *buf = (uint8_t*)ptr;
  return UPB_STATUS_OK;
}

static upb_status_t get_v_uint32_t(uint8_t *restrict *buf,
                                   uint32_t *restrict val)
{
  uint8_t *ptr = *buf, b;
  uint32_t result;

  /* From the original proto2 implementation. */
  b = *(ptr++); result  = (b & 0x7F)      ; if (!(b & 0x80)) goto done;
  b = *(ptr++); result |= (b & 0x7F) <<  7; if (!(b & 0x80)) goto done;
  b = *(ptr++); result |= (b & 0x7F) << 14; if (!(b & 0x80)) goto done;
  b = *(ptr++); result |= (b & 0x7F) << 21; if (!(b & 0x80)) goto done;
  b = *(ptr++); result  = (b & 0x7F) << 28; if (!(b & 0x80)) goto done;
  return UPB_ERROR_UNTERMINATED_VARINT;

done:
  *buf = ptr;
  *val = result;
  return UPB_STATUS_OK;
}

static upb_status_t get_f_uint32_t(uint8_t *restrict *buf,
                                   uint32_t *restrict val)
{
  uint8_t *b = *buf;
#define SHL(val, bits) ((uint32_t)val << bits)
  *val = SHL(b[0], 0) | SHL(b[1], 8) | SHL(b[2], 16) | SHL(b[3], 24);
#undef SHL
  *buf += sizeof(uint32_t);
  return UPB_STATUS_OK;
}

static upb_status_t skip_f_uint32_t(uint8_t **buf)
{
  *buf += sizeof(uint32_t);
  return UPB_STATUS_OK;
}

static upb_status_t get_f_uint64_t(uint8_t *restrict *buf,
                                   uint64_t *restrict val)
{
  uint8_t *b = *buf;
  /* TODO: is this worth 32/64 specializing? */
#define SHL(val, bits) ((uint64_t)val << bits)
  *val = SHL(b[0], 0)  | SHL(b[1], 8)  | SHL(b[2], 16) | SHL(b[3], 24) |
         SHL(b[4], 32) | SHL(b[5], 40) | SHL(b[6], 48) | SHL(b[7], 56);
#undef SHL
  *buf += sizeof(uint64_t);
  return UPB_STATUS_OK;
}

static upb_status_t skip_f_uint64_t(uint8_t **buf)
{
  *buf += sizeof(uint64_t);
  return UPB_STATUS_OK;
}

static int32_t zz_decode_32(uint32_t n) { return (n >> 1) ^ -(int32_t)(n & 1); }
static int64_t zz_decode_64(uint64_t n) { return (n >> 1) ^ -(int64_t)(n & 1); }

/* Functions for reading wire values and converting them to values.  These
 * are generated with macros because they follow a higly consistent pattern. */

#define WVTOV(type, wire_t, val_t) \
  static void wvtov_ ## type(wire_t s, val_t *d)

#define GET(type, v_or_f, wire_t, val_t, member_name) \
  static upb_status_t get_ ## type(struct upb_parse_state *s, \
                                        uint8_t *buf, \
                                        struct upb_tagged_value *d) { \
    wire_t tmp; \
    uint8_t *b = buf; \
    CHECK(get_ ## v_or_f ## _ ## wire_t(&b, &tmp)); \
    wvtov_ ## type(tmp, &d->v.member_name); \
    s->offset += (b-buf); \
    return UPB_STATUS_OK; \
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
T(ENUM,     v, uint32_t, int32_t,  int32)   { *d = (int32_t)s;               }
#undef WVTOV
#undef GET
#undef T

static void wvtov_delimited(uint32_t s, struct upb_delimited *d, size_t o)
{
  d->offset = o;
  d->len = s;
}

/* Use BYTES version for both STRING and BYTES, leave UTF-8 checks to client. */
static upb_status_t get_BYTES(struct upb_parse_state *s, uint8_t *buf,
                              struct upb_tagged_value *d) {
  uint32_t tmp;
  uint8_t *b = buf;
  CHECK(get_v_uint32_t(&b, &tmp));
  s->offset += (b-buf);  /* advance past length varint. */
  wvtov_delimited(tmp, &d->v.delimited, s->offset);
  size_t new_offset = s->offset + d->v.delimited.len; /* skip bytes */
  if (unlikely(new_offset < s->offset)) return UPB_ERROR_OVERFLOW;
  s->offset = new_offset;
  return UPB_STATUS_OK;
}

static upb_status_t get_MESSAGE(struct upb_parse_state *s, uint8_t *buf,
                                struct upb_tagged_value *d) {
  /* We're entering a sub-message. */
  uint32_t tmp;
  uint8_t *b = buf;
  CHECK(get_v_uint32_t(&b, &tmp));
  s->offset += (b-buf);  /* advance past length varint. */
  wvtov_delimited(tmp, &d->v.delimited, s->offset);
  /* Unlike STRING and BYTES, we *don't* advance past delimited here. */
  if (unlikely(++s->top == s->limit)) return UPB_ERROR_STACK_OVERFLOW;
  s->top->fieldset = d->field->fieldset;
  s->top->end_offset = d->v.delimited.offset + d->v.delimited.len;
  if (unlikely(s->top->end_offset < s->offset)) return UPB_ERROR_OVERFLOW;
  return UPB_STATUS_OK;
}

struct upb_type_info {
  upb_wire_type_t expected_wire_type;
  upb_status_t (*get)(struct upb_parse_state *s, uint8_t *buf,
                           struct upb_tagged_value *d);
};
static struct upb_type_info type_info[] = {
  {UPB_WIRE_TYPE_64BIT,     get_DOUBLE},
  {UPB_WIRE_TYPE_32BIT,     get_FLOAT},
  {UPB_WIRE_TYPE_VARINT,    get_INT32},
  {UPB_WIRE_TYPE_VARINT,    get_INT64},
  {UPB_WIRE_TYPE_VARINT,    get_UINT32},
  {UPB_WIRE_TYPE_VARINT,    get_UINT64},
  {UPB_WIRE_TYPE_VARINT,    get_SINT32},
  {UPB_WIRE_TYPE_VARINT,    get_SINT64},
  {UPB_WIRE_TYPE_32BIT,     get_FIXED32},
  {UPB_WIRE_TYPE_64BIT,     get_FIXED64},
  {UPB_WIRE_TYPE_32BIT,     get_SFIXED32},
  {UPB_WIRE_TYPE_64BIT,     get_SFIXED64},
  {UPB_WIRE_TYPE_VARINT,    get_BOOL},
  {UPB_WIRE_TYPE_DELIMITED, get_BYTES},
  {UPB_WIRE_TYPE_DELIMITED, get_BYTES},
  {UPB_WIRE_TYPE_VARINT,    get_ENUM},
  {UPB_WIRE_TYPE_DELIMITED, get_MESSAGE}
};

upb_status_t parse_tag(uint8_t **buf, struct upb_tag *tag)
{
  uint32_t tag_int;
  CHECK(get_v_uint32_t(buf, &tag_int));
  tag->wire_type    = (upb_wire_type_t)(tag_int & 0x07);
  tag->field_number = tag_int >> 3;
  return UPB_STATUS_OK;
}

upb_status_t parse_wire_value(uint8_t *buf, size_t *offset,
                              upb_wire_type_t wt,
                              union upb_wire_value *wv)
{
#define READ(expr) CHECK(expr); *offset += (b-buf)
  uint8_t *b = buf;
  switch(wt) {
    case UPB_WIRE_TYPE_VARINT:
      READ(get_v_uint64_t(&b, &wv->varint)); break;
    case UPB_WIRE_TYPE_64BIT:
      READ(get_f_uint64_t(&b, &wv->_64bit)); break;
    case UPB_WIRE_TYPE_32BIT:
      READ(get_f_uint32_t(&b, &wv->_32bit)); break;
    case UPB_WIRE_TYPE_DELIMITED:
      wv->delimited.offset = *offset;
      READ(get_v_uint32_t(&b, &wv->delimited.len));
      size_t new_offset = *offset + wv->delimited.len;
      if (new_offset < *offset) return UPB_ERROR_OVERFLOW;
      *offset += new_offset;
      break;
    case UPB_WIRE_TYPE_START_GROUP:
    case UPB_WIRE_TYPE_END_GROUP:
      return UPB_ERROR_GROUP;  /* deprecated, no plans to support. */
  }
  return UPB_STATUS_OK;
}

upb_status_t skip_wire_value(uint8_t *buf, size_t *offset,
                             upb_wire_type_t wt)
{
  uint8_t *b = buf;
  switch(wt) {
    case UPB_WIRE_TYPE_VARINT:
      READ(skip_v_uint64_t(&b)); break;
    case UPB_WIRE_TYPE_64BIT:
      READ(skip_f_uint64_t(&b)); break;
    case UPB_WIRE_TYPE_32BIT:
      READ(skip_f_uint32_t(&b)); break;
    case UPB_WIRE_TYPE_DELIMITED: {
      /* Have to get (not skip) the length to skip the bytes. */
      uint32_t len;
      READ(get_v_uint32_t(&b, &len));
      size_t new_offset = *offset + len;
      if (new_offset < *offset) return UPB_ERROR_OVERFLOW;
      *offset += new_offset;
      break;
    }
    case UPB_WIRE_TYPE_START_GROUP:
    case UPB_WIRE_TYPE_END_GROUP:
      return UPB_ERROR_GROUP;  /* deprecated, no plans to support. */
  }
  return UPB_STATUS_OK;
#undef READ
}

/* Parses and processes the next value from buf. */
upb_status_t upb_parse_field(struct upb_parse_state *s,
                             uint8_t *buf,
                             upb_field_number_t *fieldnum,
                             struct upb_tagged_value *val,
                             struct upb_tagged_wire_value *wv)
{
  /* Check for end-of-message at the current stack depth. */
  if(unlikely(s->offset >= s->top->end_offset)) {
    /* If the end offset isn't an exact field boundary, the pb is corrupt. */
    if(unlikely(s->offset != s->top->end_offset))
      return UPB_ERROR_BAD_SUBMESSAGE_END;
    s->top--;
    return UPB_STATUS_SUBMESSAGE_END;
  }

  struct upb_tag tag;
  uint8_t *b = buf;
  CHECK(parse_tag(&b, &tag));
  s->offset += (b-buf);
  struct upb_field *fd = upb_find_field(s->top->fieldset,
                                                  tag.field_number);
  upb_status_t unknown_value_status;
  if(unlikely(!fd)) {
    unknown_value_status = UPB_ERROR_UNKNOWN_VALUE;
    goto unknown_value;
  }
  struct upb_type_info *info = &type_info[fd->type];
  if(unlikely(tag.wire_type != info->expected_wire_type)) {
    unknown_value_status = UPB_ERROR_MISMATCHED_TYPE;
    goto unknown_value;
  }

  *fieldnum = tag.field_number;
  val->field = fd;
  CHECK(info->get(s, b, val));
  return UPB_STATUS_OK;

unknown_value:
  wv->type = tag.wire_type;
  CHECK(parse_wire_value(buf, &s->offset, tag.wire_type, &wv->v));
  return unknown_value_status;
}

void upb_init_parser(
    struct upb_parse_state *state,
    struct upb_fieldset *toplevel_fieldset)
{
  state->offset = 0;
  state->top = state->stack;
  state->limit = state->top + UPB_MAX_STACK;
  state->top->fieldset = toplevel_fieldset;
  state->top->end_offset = SIZE_MAX;
}

static int compare_fields(const void *f1, const void *f2)
{
  return ((struct upb_field*)f1)->field_number -
         ((struct upb_field*)f2)->field_number;
}

