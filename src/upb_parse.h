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

/* Event Callbacks. ***********************************************************/

// The tag callback is called immediately after a tag has been parsed.  The
// client should determine whether it wants to parse or skip the corresponding
// value.  If it wants to parse it, it must discover and return the correct
// .proto type (the tag only contains the wire type) and check that the wire
// type is appropriate for the .proto type.  Returning a type for which
// upb_check_type(tag->wire_type, type) == false invokes undefined behavior.
//
// To skip the value (which means skipping all submessages, in the case of a
// submessage), the callback should return zero.
//
// The client can store a void* in *user_field_desc; this will be passed to
// the value callback or the string callback.
typedef upb_field_type_t (*upb_tag_cb)(void *udata, struct upb_tag *tag,
                                       void **user_field_desc);

// The value callback is called when a regular value (ie. not a string or
// submessage) is encountered which the client has opted to parse (by not
// returning 0 from the tag_cb).  The client must parse the value by calling
// upb_parse_value(), returning success or failure accordingly.
//
// Note that this callback can be called several times in a row for a single
// call to tag_cb in the case of packed arrays.
typedef void *(*upb_value_cb)(void *udata, uint8_t *buf, uint8_t *end,
                              void *user_field_desc, struct upb_status *status);

// The string callback is called when a string is parsed.  avail_len is the
// number of bytes that are currently available at str.  If the client is
// streaming and the current buffer ends in the middle of the string, this
// number could be less than total_len.
typedef void (*upb_str_cb)(void *udata, uint8_t *str, size_t avail_len,
                           size_t total_len, void *user_field_desc);

// The start and end callbacks are called when a submessage begins and ends,
// respectively.
typedef void (*upb_start_cb)(void *udata, void *user_field_desc);
typedef void (*upb_end_cb)(void *udata);

/* Callback parser interface. *************************************************/

// Allocates and frees a upb_cbparser, respectively.
struct upb_cbparser *upb_cbparser_new(void);
void upb_cbparser_free(struct upb_cbparser *p);

// Resets the internal state of an already-allocated parser.  Parsers must be
// reset before they can be used.  A parser can be reset multiple times.  udata
// will be passed as the first argument to callbacks.
//
// tagcb must be set, but all other callbacks can be NULL, in which case they
// will just be skipped.
void upb_cbparser_reset(struct upb_cbparser *p, void *udata,
                        upb_tag_cb tagcb,
                        upb_value_cb valuecb,
                        upb_str_cb strcb,
                        upb_start_cb startcb,
                        upb_end_cb endcb);


// Parses up to len bytes of protobuf data out of buf, calling the appropriate
// callbacks as values are parsed.
//
// The function returns a status indicating the success of the operation.  Data
// is parsed until no more data can be read from buf, or the callback returns an
// error like UPB_STATUS_USER_CANCELLED, or an error occurs.
//
// *read is set to the number of bytes consumed.  Note that this can be greater
// than len in the case that a string was recognized that spans beyond the end
// of the currently provided data.
//
// The next call to upb_parse must be the first byte after buf + *read, even in
// the case that *read > len.
//
// TODO: see if we can provide the following guarantee efficiently:
//   *read will always be >= len. */
size_t upb_cbparser_parse(struct upb_cbparser *p, void *buf, size_t len,
                          struct upb_status *status);

extern upb_wire_type_t upb_expected_wire_types[];
// Returns true if wt is the correct on-the-wire type for ft.
INLINE bool upb_check_type(upb_wire_type_t wt, upb_field_type_t ft) {
  // This doesn't currently support packed arrays.
  return upb_type_info[ft].expected_wire_type == wt;
}

/* Data-consuming functions (to be called from value cb). *********************/

// Parses and converts a value from the character data starting at buf (but not
// past end).  Returns a pointer that is one past the data that was read.  The
// caller must have previously checked that the wire type is appropriate for
// this field type.
uint8_t *upb_parse_value(uint8_t *buf, uint8_t *end, upb_field_type_t ft,
                         union upb_value_ptr v, struct upb_status *status);

// Parses a wire value with the given type (which must have been obtained from
// a tag that was just parsed) and returns a pointer to one past the data that
// was read.
uint8_t *upb_parse_wire_value(uint8_t *buf, uint8_t *end, upb_wire_type_t wt,
                              union upb_wire_value *wv,
                              struct upb_status *status);

/* Functions to read wire values. *********************************************/

// Most clients will not want to use these directly.

uint8_t *upb_get_v_uint64_t_full(uint8_t *buf, uint8_t *end, uint64_t *val,
                                 struct upb_status *status);

// Gets a varint (wire type: UPB_WIRE_TYPE_VARINT).
INLINE uint8_t *upb_get_v_uint64_t(uint8_t *buf, uint8_t *end, uint64_t *val,
                                   struct upb_status *status)
{
  // We inline this common case (1-byte varints), if that fails we dispatch to
  // the full (non-inlined) version.
  if((*buf & 0x80) == 0) {
    *val = *buf & 0x7f;
    return buf + 1;
  } else {
    return upb_get_v_uint64_t_full(buf, end, val, status);
  }
}

// Gets a varint -- called when we only need 32 bits of it.
INLINE uint8_t *upb_get_v_uint32_t(uint8_t *buf, uint8_t *end,
                                   uint32_t *val, struct upb_status *status)
{
  uint64_t val64;
  uint8_t *ret = upb_get_v_uint64_t(buf, end, &val64, status);
  *val = (uint32_t)val64;  // Discard the high bits.
  return ret;
}

// Gets a fixed-length 32-bit integer (wire type: UPB_WIRE_TYPE_32BIT).
INLINE uint8_t *upb_get_f_uint32_t(uint8_t *buf, uint8_t *end,
                                   uint32_t *val, struct upb_status *status)
{
  uint8_t *uint32_end = buf + sizeof(uint32_t);
  if(uint32_end > end) {
    status->code = UPB_STATUS_NEED_MORE_DATA;
    return end;
  }
#if UPB_UNALIGNED_READS_OK
  *val = *(uint32_t*)buf;
#else
#define SHL(val, bits) ((uint32_t)val << bits)
  *val = SHL(buf[0], 0) | SHL(buf[1], 8) | SHL(buf[2], 16) | SHL(buf[3], 24);
#undef SHL
#endif
  return uint32_end;
}

// Gets a fixed-length 64-bit integer (wire type: UPB_WIRE_TYPE_64BIT).
INLINE uint8_t *upb_get_f_uint64_t(uint8_t *buf, uint8_t *end,
                                   uint64_t *val, struct upb_status *status)
{
  uint8_t *uint64_end = buf + sizeof(uint64_t);
  if(uint64_end > end) {
    status->code = UPB_STATUS_NEED_MORE_DATA;
    return end;
  }
#if UPB_UNALIGNED_READS_OK
  *val = *(uint64_t*)buf;
#else
#define SHL(val, bits) ((uint64_t)val << bits)
  *val = SHL(buf[0],  0) | SHL(buf[1],  8) | SHL(buf[2], 16) | SHL(buf[3], 24) |
         SHL(buf[4], 32) | SHL(buf[5], 40) | SHL(buf[6], 48) | SHL(buf[7], 56);
#undef SHL
#endif
  return uint64_end;
}

INLINE uint8_t *upb_skip_v_uint64_t(uint8_t *buf, uint8_t *end,
                                    struct upb_status *status)
{
  uint8_t *const maxend = buf + 10;
  uint8_t last = 0x80;
  for(; buf < (uint8_t*)end && (last & 0x80); buf++)
    last = *buf;

  if(buf >= end && buf <= maxend && (last & 0x80)) {
    status->code = UPB_STATUS_NEED_MORE_DATA;
    buf = end;
  } else if(buf > maxend) {
    status->code = UPB_ERROR_UNTERMINATED_VARINT;
    buf = end;
  }
  return buf;
}

INLINE uint8_t *upb_skip_f_uint32_t(uint8_t *buf, uint8_t *end,
                                    struct upb_status *status)
{
  uint8_t *uint32_end = buf + sizeof(uint32_t);
  if(uint32_end > end) {
    status->code = UPB_STATUS_NEED_MORE_DATA;
    return end;
  }
  return uint32_end;
}

INLINE uint8_t *upb_skip_f_uint64_t(uint8_t *buf, uint8_t *end,
                                    struct upb_status *status)
{
  uint8_t *uint64_end = buf + sizeof(uint64_t);
  if(uint64_end > end) {
    status->code = UPB_STATUS_NEED_MORE_DATA;
    return end;
  }
  return uint64_end;
}


/* Functions to read .proto values. *******************************************/


// Performs zig-zag decoding, which is used by sint32 and sint64.
INLINE int32_t upb_zzdec_32(uint32_t n) { return (n >> 1) ^ -(int32_t)(n & 1); }
INLINE int64_t upb_zzdec_64(uint64_t n) { return (n >> 1) ^ -(int64_t)(n & 1); }

// Use macros to define a set of two functions for each .proto type:
//
//  // Reads and converts a .proto value from buf, placing it in d.
//  // "end" indicates the end of the current buffer (if the buffer does
//  // not contain the entire value UPB_STATUS_NEED_MORE_DATA is returned).
//  // On success, a pointer will be returned to the first byte that was
//  // not consumed.
//  uint8_t *upb_get_INT32(uint8_t *buf, uint8_t *end, int32_t *d,
//                         struct upb_status *status);
//
//  // Given an already read wire value s (source), convert it to a .proto
//  // value and return it.
//  int32_t upb_wvtov_INT32(uint32_t s);
//
// These are the most efficient functions to call if you want to decode a value
// for a known type.

#define WVTOV(type, wire_t, val_t) \
  INLINE val_t upb_wvtov_ ## type(wire_t s)

#define GET(type, v_or_f, wire_t, val_t, member_name) \
  INLINE uint8_t *upb_get_ ## type(uint8_t *buf, uint8_t *end, val_t *d, \
                                   struct upb_status *status) { \
    wire_t tmp = 0; \
    uint8_t *ret = upb_get_ ## v_or_f ## _ ## wire_t(buf, end, &tmp, status); \
    *d = upb_wvtov_ ## type(tmp); \
    return ret; \
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

// Parses a tag, places the result in *tag.
INLINE uint8_t *parse_tag(uint8_t *buf, uint8_t *end, struct upb_tag *tag,
                          struct upb_status *status)
{
  uint32_t tag_int;
  uint8_t *ret = upb_get_v_uint32_t(buf, end, &tag_int, status);
  tag->wire_type    = (upb_wire_type_t)(tag_int & 0x07);
  tag->field_number = tag_int >> 3;
  return ret;
}

#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif  /* UPB_PARSE_H_ */
