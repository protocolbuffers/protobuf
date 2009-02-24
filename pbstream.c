/*
 * pbstream - a stream-oriented implementation of protocol buffers.
 *
 * Copyright (c) 2008-2009 Joshua Haberman.  See LICENSE for details.
 */

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

/* Lowest-level functions -- these read integers from the input buffer.
 * To avoid branches, none of these do bounds checking.  So we force clients
 * to overallocate their buffers by >=9 bytes. */

static pbstream_status_t get_v_uint64_t(char **buf, char *end, uint64_t *val)
{
  uint8_t* ptr = (uint8_t*)*buf;
  uint32_t b;
  uint32_t part0 = 0, part1 = 0, part2 = 0;

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
  return unlikely(*buf > end) ? PBSTREAM_STATUS_INCOMPLETE : PBSTREAM_STATUS_OK;
}

static pbstream_status_t get_v_uint32_t(char **buf, char *end, uint32_t *val)
{
  uint8_t* ptr = (uint8_t*)*buf;
  uint32_t b;
  uint32_t result;

  b = *(ptr++); result  = (b & 0x7F)      ; if (!(b & 0x80)) goto done;
  b = *(ptr++); result |= (b & 0x7F) <<  7; if (!(b & 0x80)) goto done;
  b = *(ptr++); result |= (b & 0x7F) << 14; if (!(b & 0x80)) goto done;
  b = *(ptr++); result |= (b & 0x7F) << 21; if (!(b & 0x80)) goto done;
  b = *(ptr++); result  = (b & 0x7F) << 28; if (!(b & 0x80)) goto done;
  return PBSTREAM_ERROR_UNTERMINATED_VARINT;

done:
  *buf = (char*)ptr;
  *val = result;
  return unlikely(*buf > end) ? PBSTREAM_STATUS_INCOMPLETE: PBSTREAM_STATUS_OK;
}

static pbstream_status_t get_f_uint32_t(char **buf, char *end, uint32_t *val)
{
  uint8_t *b = (uint8_t*)*buf;
#if __BYTE_ORDER == __LITTLE_ENDIAN
  *val = *(uint32_t*)b;  /* likely unaligned, TODO: verify performance. */
#else
  *val = b[0] | (b[1] << 8) | (b[2] << 16) | (b[3] << 24);
#endif
  *buf = (char*)b + sizeof(uint32_t);
  return unlikely(*buf > end) ? PBSTREAM_STATUS_INCOMPLETE : PBSTREAM_STATUS_OK;
}

static pbstream_status_t get_f_uint64_t(char **buf, char *end, uint64_t *val)
{
  uint8_t *b = (uint8_t*)*buf;
#if __BYTE_ORDER == __LITTLE_ENDIAN
  *val = *(uint64_t*)buf;  /* likely unaligned, TODO: verify performance. */
#else
  *val = (b[0])       | (b[1] << 8 ) | (b[2] << 16) | (b[3] << 24) |
         (b[4] << 32) | (b[5] << 40) | (b[6] << 48) | (b[7] << 56);
#endif
  *buf = (char*)b + sizeof(uint64_t);
  return unlikely(*buf > end) ? PBSTREAM_STATUS_INCOMPLETE : PBSTREAM_STATUS_OK;
}

static int32_t zz_decode_32(uint32_t n) { return (n >> 1) ^ -(int32_t)(n & 1); }
static int64_t zz_decode_64(uint64_t n) { return (n >> 1) ^ -(int64_t)(n & 1); }

#define CHECK(func) do { \
  pbstream_wire_type_t status = func; \
  if(status != PBSTREAM_STATUS_OK) return status; \
  } while (0)

/* WVTOV() generates a function:
 *   void wvtov_TYPE(wire_t src, val_t *dst, size_t offset)
 * (macro invoker defines the body of the function).  */
#define WVTOV(type, wire_t, val_t) \
  static void wvtov_ ## type(wire_t s, val_t *d, size_t offset)

/* GET() generates a function:
 *   pbstream_status_t get_TYPE(char **buf, char *end, size_t offset,
 *                              pbstream_value *dst) */
#define GET(type, v_or_f, wire_t, val_t, member_name) \
  static pbstream_status_t get_ ## type(char **buf, char *end, size_t offset, \
                                        struct pbstream_value *d) { \
    wire_t tmp; \
    CHECK(get_ ## v_or_f ## _ ## wire_t(buf, end, &tmp)); \
    wvtov_ ## type(tmp, &d->v.member_name, offset); \
    return PBSTREAM_STATUS_OK; \
  }

#define T(type, v_or_f, wire_t, val_t, member_name) \
  WVTOV(type, wire_t, val_t);  /* prototype for GET below */ \
  GET(type, v_or_f, wire_t, val_t, member_name) \
  WVTOV(type, wire_t, val_t)

T(DOUBLE,   f, uint64_t, double,   _double){ memcpy(d, &s, sizeof(double)); }
T(FLOAT,    f, uint32_t, float,    _float) { memcpy(d, &s, sizeof(float));  }
T(INT32,    v, uint32_t, int32_t,  int32)  { *d = (int32_t)s;               }
T(INT64,    v, uint64_t, int64_t,  int64)  { *d = (int64_t)s;               }
T(UINT32,   v, uint32_t, uint32_t, uint32) { *d = s;                        }
T(UINT64,   v, uint64_t, uint64_t, uint64) { *d = s;                        }
T(SINT32,   v, uint32_t, int32_t,  int32)  { *d = zz_decode_32(s);          }
T(SINT64,   v, uint64_t, int64_t,  int64)  { *d = zz_decode_64(s);          }
T(FIXED32,  f, uint32_t, uint32_t, uint32) { *d = s;                        }
T(FIXED64,  f, uint64_t, uint64_t, uint64) { *d = s;                        }
T(SFIXED32, f, uint32_t, int32_t,  int32)  { *d = (int32_t)s;               }
T(SFIXED64, f, uint64_t, int64_t,  int64)  { *d = (int64_t)s;               }
T(BOOL,     v, uint32_t, bool,     _bool)  { *d = (bool)s;                  }
T(ENUM,     v, uint32_t, int32_t,  _enum)  { *d = (int32_t)s;               }

#define T_DELIMITED(type) \
  T(type, v, uint32_t, struct pbstream_delimited, delimited) { \
    d->offset = offset; \
    d->len = s; \
  }
T_DELIMITED(STRING);  /* We leave UTF-8 validation to the client. */
T_DELIMITED(BYTES);
T_DELIMITED(MESSAGE);
#undef WVTOV
#undef GET
#undef T
#undef T_DELIMITED

struct pbstream_type_info {
  pbstream_wire_type_t expected_wire_type;
  pbstream_status_t (*get)(char **buf, char *end, size_t offset,
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
  {PBSTREAM_WIRE_TYPE_DELIMITED, get_STRING},
  {PBSTREAM_WIRE_TYPE_DELIMITED, get_BYTES},
  {PBSTREAM_WIRE_TYPE_VARINT,    get_ENUM},
  {PBSTREAM_WIRE_TYPE_DELIMITED, get_MESSAGE}
};

static pbstream_status_t parse_tag(char **buf, char *end, struct pbstream_tag *tag)
{
  uint32_t tag_int;
  CHECK(get_v_uint32_t(buf, end, &tag_int));
  tag->wire_type    = tag_int & 0x07;
  tag->field_number = tag_int >> 3;
  return PBSTREAM_STATUS_OK;
}

static pbstream_status_t parse_unknown_value(
    char **buf, char *end, int buf_offset,
    struct pbstream_wire_value *wv)
{
#define DECODE(dest, func) CHECK(func(buf, end, &dest))
  switch(wv->type) {
    case PBSTREAM_WIRE_TYPE_VARINT:
      DECODE(wv->v.varint, get_v_uint64_t); break;
    case PBSTREAM_WIRE_TYPE_64BIT:
      DECODE(wv->v._64bit, get_f_uint64_t); break;
    case PBSTREAM_WIRE_TYPE_32BIT:
      DECODE(wv->v._32bit, get_f_uint32_t); break;
    case PBSTREAM_WIRE_TYPE_DELIMITED: {
      uint32_t len;
      wv->v.delimited.offset = buf_offset;
      DECODE(len, get_v_uint32_t);
      wv->v.delimited.len = (size_t)len;
      break;
    }
    case PBSTREAM_WIRE_TYPE_START_GROUP:
    case PBSTREAM_WIRE_TYPE_END_GROUP:
      /* TODO (though these are deprecated, so not high priority). */
      break;
  }
  return PBSTREAM_STATUS_OK;
#undef DECODE
}

#define CALLBACK(s, func, ...) do { \
  if(s->callbacks.func) s->callbacks.func(__VA_ARGS__); \
  } while (0)

#define NONFATAL_ERROR(s, code) do { \
  if(s->ignore_nonfatal_errors) CALLBACK(s, error_callback, code); \
  else return code; \
  } while (0)

static struct pbstream_field_descriptor *find_field_descriptor(
    struct pbstream_message_descriptor* md,
    pbstream_field_number_t field_number)
{
  /* Likely will want to replace linear search with something better. */
  for (int i = 0; i < md->fields_len; i++)
    if (md->fields[i].field_number == field_number) return &md->fields[i];
  return NULL;
}

/* Process actions associated with the end of a [sub-]message. */
pbstream_status_t process_message_end(struct pbstream_parse_state *s)
{
  struct pbstream_parse_stack_frame *frame = DYNARRAY_GET_TOP(s->stack);
  /* A submessage that doesn't end exactly on a field boundary indicates
   * corruption. */
  if(unlikely(s->offset != frame->end_offset))
    return PBSTREAM_ERROR_BAD_SUBMESSAGE_END;

  /* Check required fields. */
  struct pbstream_message_descriptor *md = frame->message_descriptor;
  for(int i = 0; i < md->fields_len; i++) {
    struct pbstream_field_descriptor *fd = &md->fields[i];
    if(fd->seen_field_num && !frame->seen_fields[fd->seen_field_num] &&
       fd->cardinality == PBSTREAM_CARDINALITY_REQUIRED) {
      NONFATAL_ERROR(s, PBSTREAM_ERROR_MISSING_REQUIRED_FIELD);
    }
  }
  RESIZE_DYNARRAY(s->stack, s->stack_len-1);
  return PBSTREAM_STATUS_OK;
}

/* Parses and processes the next value from buf (but not past end). */
pbstream_status_t parse_field(struct pbstream_parse_state *s,
                              char *buf, char *end,
                              pbstream_field_number_t *fieldnum,
                              struct pbstream_value *val,
                              struct pbstream_wire_value *wv)
{
  struct pbstream_parse_stack_frame *frame = DYNARRAY_GET_TOP(s->stack);
  struct pbstream_message_descriptor *md = frame->message_descriptor;
  struct pbstream_tag tag;
  struct pbstream_field_descriptor *fd;
  struct pbstream_type_info *info;
  char *b = buf;

  if(unlikely(s->offset >= frame->end_offset)) return process_message_end(s);

  CHECK(parse_tag(&b, end, &tag));
  size_t val_offset = s->offset + (b-buf);
  fd = find_field_descriptor(md, tag.field_number);
  if(unlikely(!fd)) goto unknown_value;
  info = &type_info[fd->type];

  /* Check type and cardinality. */
  if(unlikely(tag.wire_type != info->expected_wire_type)) {
    NONFATAL_ERROR(s, PBSTREAM_ERROR_MISMATCHED_TYPE);
    goto unknown_value;
  }
  if(fd->seen_field_num > 0) {
    if(unlikely(frame->seen_fields[fd->seen_field_num]))
      NONFATAL_ERROR(s, PBSTREAM_ERROR_DUPLICATE_FIELD);
    frame->seen_fields[fd->seen_field_num] = true;
  }

  if(unlikely(fd->type == PBSTREAM_TYPE_MESSAGE)) {
    /* We're entering a sub-message. */
    CHECK(info->get(&b, end, val_offset, val));
    RESIZE_DYNARRAY(s->stack, s->stack_len+1);
    struct pbstream_parse_stack_frame *frame = DYNARRAY_GET_TOP(s->stack);
    frame->message_descriptor = fd->d.message;
    frame->end_offset = val->v.delimited.offset + val->v.delimited.len;
    s->offset = wv->v.delimited.offset;  /* skip past only the tag. */
    int num_seen_fields = frame->message_descriptor->num_seen_fields;
    INIT_DYNARRAY(frame->seen_fields, num_seen_fields, num_seen_fields);
  } else {
    /* This is a scalar value. */
    *fieldnum = tag.field_number;
    val->type = fd->type;
    CHECK(info->get(&b, end, val_offset, val));
    s->offset += (b-buf);
  }
  return PBSTREAM_STATUS_OK;

unknown_value:
  wv->type = tag.wire_type;
  CHECK(parse_unknown_value(&b, end, val_offset, wv));
  s->offset += (b-buf);
  return PBSTREAM_STATUS_OK;
}
