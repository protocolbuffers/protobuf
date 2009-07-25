/*
 * upb - a minimalist implementation of protocol buffers.
 *
 * upb_parse implements a high performance, callback-based, stream-oriented
 * parser (comparable to the SAX model in XML parsers).  For parsing protobufs
 * into in-memory messages (a more DOM-like model), see the routines in
 * upb_msg.h, which are layered on top of this parser.
 *
 * Copyright (c) 2009 Joshua Haberman.  See LICENSE for details.
 */

#ifndef UPB_PARSE_H_
#define UPB_PARSE_H_

#include <stdbool.h>
#include <stdint.h>
#include "upb.h"
#include "descriptor.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Definitions. ***************************************************************/

/* A list of types as they are encoded on-the-wire. */
enum upb_wire_type {
  UPB_WIRE_TYPE_VARINT      = 0,
  UPB_WIRE_TYPE_64BIT       = 1,
  UPB_WIRE_TYPE_DELIMITED   = 2,
  UPB_WIRE_TYPE_START_GROUP = 3,
  UPB_WIRE_TYPE_END_GROUP   = 4,
  UPB_WIRE_TYPE_32BIT       = 5
};
typedef uint8_t upb_wire_type_t;

/* A value as it is encoded on-the-wire, except delimited, which is handled
 * separately. */
union upb_wire_value {
  uint64_t varint;
  uint64_t _64bit;
  uint32_t _32bit;
};

/* A tag occurs before each value on-the-wire. */
struct upb_tag {
  upb_field_number_t field_number;
  upb_wire_type_t wire_type;
};

INLINE bool upb_issubmsgtype(upb_field_type_t type) {
  return type == GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_TYPE_GROUP  ||
         type == GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_TYPE_MESSAGE;
}

INLINE bool upb_isstringtype(upb_field_type_t type) {
  return type == GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_TYPE_STRING  ||
         type == GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_TYPE_BYTES;
}

/* High-level parsing interface. **********************************************/

/* The general scheme is that the client registers callbacks that will be
 * called at the appropriate times.  These callbacks provide the client with
 * data and let the client make decisions (like whether to parse or to skip
 * a value).
 *
 * After initializing the parse state, the client can repeatedly call upb_parse
 * as data becomes available.  The parser is fully streaming-capable, so the
 * data need not all be available at the same time. */

struct upb_parse_state;

/* Initialize and free (respectively) the given parse state, which must have
 * been previously allocated.  udata_size specifies how much space will be
 * available at parse_stack_frame.user_data in each frame for user data. */
void upb_parse_init(struct upb_parse_state *state, void *udata);
void upb_parse_reset(struct upb_parse_state *state, void *udata);
void upb_parse_free(struct upb_parse_state *state);

/* The callback that is called immediately after a tag has been parsed.  The
 * client should determine whether it wants to parse or skip the corresponding
 * value.  If it wants to parse it, it must discover and return the correct
 * .proto type (the tag only contains the wire type) and check that the wire
 * type is appropriate for the .proto type.  To skip the value (which means
 * skipping all submessages, in the case of a submessage), the callback should
 * return zero. */
typedef upb_field_type_t (*upb_tag_cb)(void *udata,
                                       struct upb_tag *tag,
                                       void **user_field_desc);

/* The callback that is called when a regular value (ie. not a string or
 * submessage) is encountered which the client has opted to parse (by not
 * returning 0 from the tag_cb).  The client must parse the value and update
 * buf accordingly, returning success or failure.
 *
 * Note that this callback can be called several times in a row for a single
 * call to tag_cb in the case of packed arrays. */
typedef upb_status_t (*upb_value_cb)(void *udata, uint8_t *buf, uint8_t *end,
                                     void *user_field_desc, uint8_t **outbuf);

/* The callback that is called when a string is parsed. */
typedef void (*upb_str_cb)(void *udata, struct upb_string *str,
                           void *user_field_desc);

/* Callbacks that are called when a submessage begins and ends, respectively.
 * Both are called with the submessage's stack frame at the top of the stack. */
typedef void (*upb_submsg_start_cb)(void *udata,
                                    void *user_field_desc);
typedef void (*upb_submsg_end_cb)(void *udata);

struct upb_parse_state {
  /* For delimited submsgs, counts from the submsg len down to zero.
   * For group submsgs, counts from zero down to the negative len. */
  uint32_t stack[UPB_MAX_NESTING], *top, *limit;
  size_t completed_offset;
  void *udata;
  upb_tag_cb          tag_cb;
  upb_value_cb        value_cb;
  upb_str_cb          str_cb;
  upb_submsg_start_cb submsg_start_cb;
  upb_submsg_end_cb   submsg_end_cb;
};

/* Parses up to len bytes of protobuf data out of buf, calling cb as needed.
 * The function returns how many bytes were consumed from buf.  Data is parsed
 * until no more data can be read from buf, or the callback sets *done=true,
 * or an error occured.  Sets *read to the number of bytes consumed. */
upb_status_t upb_parse(struct upb_parse_state *s, void *buf, size_t len,
                       size_t *read);

extern upb_wire_type_t upb_expected_wire_types[];
/* Returns true if wt is the correct on-the-wire type for ft. */
INLINE bool upb_check_type(upb_wire_type_t wt, upb_field_type_t ft) {
  /* This doesn't currently support packed arrays. */
  return upb_type_info[ft].expected_wire_type == wt;
}

/* Data-consuming functions (to be called from value cb). *********************/

/* Parses and converts a value from the character data starting at buf.  The
 * caller must have previously checked that the wire type is appropriate for
 * this field type. */
upb_status_t upb_parse_value(uint8_t *buf, uint8_t *end, upb_field_type_t ft,
                             union upb_value_ptr v, uint8_t **outbuf);

/* Parses a wire value with the given type (which must have been obtained from
 * a tag that was just parsed) and adds the number of bytes that were consumed
 * to *offset. */
upb_status_t upb_parse_wire_value(uint8_t *buf, uint8_t *end, upb_wire_type_t wt,
                                  union upb_wire_value *wv, uint8_t **outbuf);

/* Low-level parsing functions. **********************************************/

upb_status_t upb_get_v_uint64_t_full(uint8_t *buf, uint8_t *end, uint64_t *val,
                                     uint8_t **outbuf);

INLINE upb_status_t upb_get_v_uint64_t(uint8_t *buf, uint8_t *end, uint64_t *val,
                                       uint8_t **outbuf)
{
  /* We inline these two common cases (short varints), if that fails we
   * dispatch to the full (non-inlined) version. */
  if((*buf & 0x80) == 0) {
    /* Single-byte varint -- very common case. */
    *val = *buf & 0x7f;
    *outbuf = buf + 1;
    return UPB_STATUS_OK;
  } else if(buf <= end && (*(buf+1) & 0x80) == 0) {
    /* Two-byte varint. */
    *val = (buf[0] & 0x7f) | ((buf[1] & 0x7f) << 7);
    *outbuf = buf + 2;
    return UPB_STATUS_OK;
  } else {
    return upb_get_v_uint64_t_full(buf, end, val, outbuf);
  }
}

INLINE upb_status_t upb_get_v_uint32_t(uint8_t *buf, uint8_t *end,
                                       uint32_t *val, uint8_t **outbuf)
{
  uint64_t val64;
  UPB_CHECK(upb_get_v_uint64_t(buf, end, &val64, outbuf));
  /* TODO: should we throw an error if any of the high bits in val64 are set? */
  *val = (uint32_t)val64;
  return UPB_STATUS_OK;
}

INLINE upb_status_t upb_get_f_uint32_t(uint8_t *buf, uint8_t *end,
                                       uint32_t *val, uint8_t **outbuf)
{
  uint8_t *uint32_end = buf + sizeof(uint32_t);
  if(uint32_end > end) return UPB_STATUS_NEED_MORE_DATA;
#if UPB_UNALIGNED_READS_OK
  *val = *(uint32_t*)buf;
#else
#define SHL(val, bits) ((uint32_t)val << bits)
  *val = SHL(b[0], 0) | SHL(b[1], 8) | SHL(b[2], 16) | SHL(b[3], 24);
#undef SHL
#endif
  *outbuf = uint32_end;
  return UPB_STATUS_OK;
}

INLINE upb_status_t upb_get_f_uint64_t(uint8_t *buf, uint8_t *end,
                                       uint64_t *val, uint8_t **outbuf)
{
  uint8_t *uint64_end = buf + sizeof(uint64_t);
  if(uint64_end > end) return UPB_STATUS_NEED_MORE_DATA;
#if UPB_UNALIGNED_READS_OK
  *val = *(uint64_t*)buf;
#else
#define SHL(val, bits) ((uint64_t)val << bits)
  *val = SHL(b[0],  0) | SHL(b[1],  8) | SHL(b[2], 16) | SHL(b[3], 24) |
         SHL(b[4], 32) | SHL(b[5], 40) | SHL(b[6], 48) | SHL(b[7], 56) |
#undef SHL
#endif
  *outbuf = uint64_end;
  return UPB_STATUS_OK;
}

INLINE int32_t zz_decode_32(uint32_t n) { return (n >> 1) ^ -(int32_t)(n & 1); }
INLINE int64_t zz_decode_64(uint64_t n) { return (n >> 1) ^ -(int64_t)(n & 1); }


#define WVTOV(type, wire_t, val_t) \
  INLINE void upb_wvtov_ ## type(wire_t s, val_t *d)

#define GET(type, v_or_f, wire_t, val_t, member_name) \
  INLINE upb_status_t upb_get_ ## type(uint8_t *buf, uint8_t *end, val_t *d, uint8_t **outbuf) { \
    wire_t tmp; \
    UPB_CHECK(upb_get_ ## v_or_f ## _ ## wire_t(buf, end, &tmp, outbuf)); \
    upb_wvtov_ ## type(tmp, d); \
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

INLINE upb_status_t parse_tag(uint8_t *buf, uint8_t *end, struct upb_tag *tag,
                              uint8_t **outbuf)
{
  uint32_t tag_int;
  UPB_CHECK(upb_get_v_uint32_t(buf, end, &tag_int, outbuf));
  tag->wire_type    = (upb_wire_type_t)(tag_int & 0x07);
  tag->field_number = tag_int >> 3;
  return UPB_STATUS_OK;
}

#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif  /* UPB_PARSE_H_ */
