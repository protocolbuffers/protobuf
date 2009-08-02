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

/* The callback that is called when a string is parsed.  Note that the data
 * for the string might not all be available -- we could be streaming, and
 * the current buffer might end right in the middle of the string.  So we
 * pass both the available length and the total length. */
typedef void (*upb_str_cb)(void *udata, uint8_t *str,
                           size_t avail_len, size_t total_len,
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
 * The function returns a status indicating the success of the operation.  Data
 * is parsed until no more data can be read from buf, or the callback returns an
 * error like UPB_STATUS_USER_CANCELLED, or an error occurs.
 *
 * *read is set to the number of bytes consumed.  Note that this can be greater
 * than len in the case that a string was recognized that spans beyond the end
 * of the currently provided data.
 *
 * The next call to upb_parse must be the first byte after buf + *read, even in
 * the case that *read > len. */
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

/* Functions to read wire values. *********************************************/

/* In general, these should never be called directly from any code outside upb.
 * They are included here only because we expect them to get inlined inside the
 * value-reading functions below. */

upb_status_t upb_get_v_uint64_t_full(uint8_t *buf, uint8_t *end, uint64_t *val,
                                     uint8_t **outbuf);

/* Gets a varint (wire type: UPB_WIRE_TYPE_VARINT). */
INLINE upb_status_t upb_get_v_uint64_t(uint8_t *buf, uint8_t *end, uint64_t *val,
                                       uint8_t **outbuf)
{
  /* We inline this common case (1-byte varints), if that fails we dispatch to
   * the full (non-inlined) version. */
  if((*buf & 0x80) == 0) {
    *val = *buf & 0x7f;
    *outbuf = buf + 1;
    return UPB_STATUS_OK;
  } else {
    return upb_get_v_uint64_t_full(buf, end, val, outbuf);
  }
}

/* Gets a varint -- called when we only need 32 bits of it. */
INLINE upb_status_t upb_get_v_uint32_t(uint8_t *buf, uint8_t *end,
                                       uint32_t *val, uint8_t **outbuf)
{
  uint64_t val64;
  UPB_CHECK(upb_get_v_uint64_t(buf, end, &val64, outbuf));
  *val = (uint32_t)val64;  /* Discard the high bits. */
  return UPB_STATUS_OK;
}

/* Gets a fixed-length 32-bit integer (wire type: UPB_WIRE_TYPE_32BIT). */
INLINE upb_status_t upb_get_f_uint32_t(uint8_t *buf, uint8_t *end,
                                       uint32_t *val, uint8_t **outbuf)
{
  uint8_t *uint32_end = buf + sizeof(uint32_t);
  if(uint32_end > end) return UPB_STATUS_NEED_MORE_DATA;
#if UPB_UNALIGNED_READS_OK
  *val = *(uint32_t*)buf;
#else
#define SHL(val, bits) ((uint32_t)val << bits)
  *val = SHL(buf[0], 0) | SHL(buf[1], 8) | SHL(buf[2], 16) | SHL(buf[3], 24);
#undef SHL
#endif
  *outbuf = uint32_end;
  return UPB_STATUS_OK;
}

/* Gets a fixed-length 64-bit integer (wire type: UPB_WIRE_TYPE_64BIT). */
INLINE upb_status_t upb_get_f_uint64_t(uint8_t *buf, uint8_t *end,
                                       uint64_t *val, uint8_t **outbuf)
{
  uint8_t *uint64_end = buf + sizeof(uint64_t);
  if(uint64_end > end) return UPB_STATUS_NEED_MORE_DATA;
#if UPB_UNALIGNED_READS_OK
  *val = *(uint64_t*)buf;
#else
#define SHL(val, bits) ((uint64_t)val << bits)
  *val = SHL(buf[0],  0) | SHL(buf[1],  8) | SHL(buf[2], 16) | SHL(buf[3], 24) |
         SHL(buf[4], 32) | SHL(buf[5], 40) | SHL(buf[6], 48) | SHL(buf[7], 56);
#undef SHL
#endif
  *outbuf = uint64_end;
  return UPB_STATUS_OK;
}

/* Functions to read .proto values. *******************************************/

/* These functions read the appropriate wire value for a given .proto type
 * and then convert it based on the .proto type.  These are the most efficient
 * functions to call if you want to decode a value for a known type. */

/* Performs zig-zag decoding, which is used by sint32 and sint64. */
INLINE int32_t upb_zzdec_32(uint32_t n) { return (n >> 1) ^ -(int32_t)(n & 1); }
INLINE int64_t upb_zzdec_64(uint64_t n) { return (n >> 1) ^ -(int64_t)(n & 1); }

/* Use macros to define a set of two functions for each .proto type:
 *
 *  // Reads and converts a .proto value from buf, placing it in d.
 *  // "end" indicates the end of the current buffer (if the buffer does
 *  // not contain the entire value UPB_STATUS_NEED_MORE_DATA is returned).
 *  // On success, *outbuf will point to the first byte that was not consumed.
 *  upb_status_t upb_get_INT32(uint8_t *buf, uint8_t *end, int32_t *d,
 *                             uint8_t **outbuf);
 *
 *  // Given an already read wire value s (source), convert it to a .proto
 *  // value and return it.
 *  int32_t upb_wvtov_INT32(uint32_t s);
 */

#define WVTOV(type, wire_t, val_t) \
  INLINE val_t upb_wvtov_ ## type(wire_t s)

#define GET(type, v_or_f, wire_t, val_t, member_name) \
  INLINE upb_status_t upb_get_ ## type(uint8_t *buf, uint8_t *end, val_t *d, \
                                       uint8_t **outbuf) { \
    wire_t tmp; \
    UPB_CHECK(upb_get_ ## v_or_f ## _ ## wire_t(buf, end, &tmp, outbuf)); \
    *d = upb_wvtov_ ## type(tmp); \
    return UPB_STATUS_OK; \
  }

#define T(type, v_or_f, wire_t, val_t, member_name) \
  WVTOV(type, wire_t, val_t);  /* prototype for GET below */ \
  GET(type, v_or_f, wire_t, val_t, member_name) \
  WVTOV(type, wire_t, val_t)

T(INT32,    v, uint32_t, int32_t,  int32)   { return (int32_t)s;      }
T(INT64,    v, uint64_t, int64_t,  int64)   { return (int64_t)s;      }
T(UINT32,   v, uint32_t, uint32_t, uint32)  { return s;               }
T(UINT64,   v, uint64_t, uint64_t, uint64)  { return s;               }
T(SINT32,   v, uint32_t, int32_t,  int32)   { return upb_zzdec_32(s); }
T(SINT64,   v, uint64_t, int64_t,  int64)   { return upb_zzdec_64(s); }
T(FIXED32,  f, uint32_t, uint32_t, uint32)  { return s;               }
T(FIXED64,  f, uint64_t, uint64_t, uint64)  { return s;               }
T(SFIXED32, f, uint32_t, int32_t,  int32)   { return (int32_t)s;      }
T(SFIXED64, f, uint64_t, int64_t,  int64)   { return (int64_t)s;      }
T(BOOL,     v, uint32_t, bool,     _bool)   { return (bool)s;         }
T(ENUM,     v, uint32_t, int32_t,  int32)   { return (int32_t)s;      }
T(DOUBLE,   f, uint64_t, double,   _double) {
  union upb_value v;
  v.uint64 = s;
  return v._double;
}
T(FLOAT,    f, uint32_t, float,    _float)  {
  union upb_value v;
  v.uint32 = s;
  return v._float;
}

#undef WVTOV
#undef GET
#undef T

/* Parses a tag, places the result in *tag. */
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
