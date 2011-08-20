/*
 * upb - a minimalist implementation of protocol buffers.
 *
 * Copyright (c) 2009 Google Inc.  See LICENSE for details.
 * Author: Josh Haberman <jhaberman@gmail.com>
 */

#include "upb/pb/encoder.h"

#include <stdlib.h>
#include "upb/descriptor.h"

/* Functions for calculating sizes of wire values. ****************************/

static size_t upb_v_uint64_t_size(uint64_t val) {
#ifdef __GNUC__
  int high_bit = 63 - __builtin_clzll(val);  // 0-based, undef if val == 0.
#else
  int high_bit = 0;
  uint64_t tmp = val;
  while(tmp >>= 1) high_bit++;
#endif
  return val == 0 ? 1 : high_bit / 7 + 1;
}

static size_t upb_v_int32_t_size(int32_t val) {
  // v_uint32's are sign-extended to maintain wire compatibility with int64s.
  return upb_v_uint64_t_size((int64_t)val);
}
static size_t upb_v_uint32_t_size(uint32_t val) {
  return upb_v_uint64_t_size(val);
}
static size_t upb_f_uint64_t_size(uint64_t val) {
  (void)val;  // Length is independent of value.
  return sizeof(uint64_t);
}
static size_t upb_f_uint32_t_size(uint32_t val) {
  (void)val;  // Length is independent of value.
  return sizeof(uint32_t);
}


/* Functions to write wire values. ********************************************/

// Since we know in advance the longest that the value could be, we always make
// sure that our buffer is long enough.  This saves us from having to perform
// bounds checks.

// Puts a varint (wire type: UPB_WIRE_TYPE_VARINT).
static uint8_t *upb_put_v_uint64_t(uint8_t *buf, uint64_t val)
{
  do {
    uint8_t byte = val & 0x7f;
    val >>= 7;
    if(val) byte |= 0x80;
    *buf++ = byte;
  } while(val);
  return buf;
}

// Puts an unsigned 32-bit varint, verbatim.  Never uses the high 64 bits.
static uint8_t *upb_put_v_uint32_t(uint8_t *buf, uint32_t val)
{
  return upb_put_v_uint64_t(buf, val);
}

// Puts a signed 32-bit varint, first sign-extending to 64-bits.  We do this to
// maintain wire-compatibility with 64-bit signed integers.
static uint8_t *upb_put_v_int32_t(uint8_t *buf, int32_t val)
{
  return upb_put_v_uint64_t(buf, (int64_t)val);
}

static void upb_put32(uint8_t *buf, uint32_t val) {
  buf[0] = val & 0xff;
  buf[1] = (val >> 8) & 0xff;
  buf[2] = (val >> 16) & 0xff;
  buf[3] = (val >> 24);
}

// Puts a fixed-length 32-bit integer (wire type: UPB_WIRE_TYPE_32BIT).
static uint8_t *upb_put_f_uint32_t(uint8_t *buf, uint32_t val)
{
  uint8_t *uint32_end = buf + sizeof(uint32_t);
#if UPB_UNALIGNED_READS_OK
  *(uint32_t*)buf = val;
#else
  upb_put32(buf, val);
#endif
  return uint32_end;
}

// Puts a fixed-length 64-bit integer (wire type: UPB_WIRE_TYPE_64BIT).
static uint8_t *upb_put_f_uint64_t(uint8_t *buf, uint64_t val)
{
  uint8_t *uint64_end = buf + sizeof(uint64_t);
#if UPB_UNALIGNED_READS_OK
  *(uint64_t*)buf = val;
#else
  upb_put32(buf, (uint32_t)val);
  upb_put32(buf, (uint32_t)(val >> 32));
#endif
  return uint64_end;
}

/* Functions to write and calculate sizes for .proto values. ******************/

// Performs zig-zag encoding, which is used by sint32 and sint64.
static uint32_t upb_zzenc_32(int32_t n) { return (n << 1) ^ (n >> 31); }
static uint64_t upb_zzenc_64(int64_t n) { return (n << 1) ^ (n >> 63); }

/* Use macros to define a set of two functions for each .proto type:
 *
 *  // Converts and writes a .proto value into buf.  "end" indicates the end
 *  // of the current available buffer (if the buffer does not contain enough
 *  // space UPB_STATUS_NEED_MORE_DATA is returned).  On success, *outbuf will
 *  // point one past the data that was written.
 *  uint8_t *upb_put_INT32(uint8_t *buf, int32_t val);
 *
 *  // Returns the number of bytes required to encode val.
 *  size_t upb_get_INT32_size(int32_t val);
 *
 *  // Given a .proto value s (source) convert it to a wire value.
 *  uint32_t upb_vtowv_INT32(int32_t s);
 */

#define VTOWV(type, wire_t, val_t) \
  static wire_t upb_vtowv_ ## type(val_t s)

#define PUT(type, v_or_f, wire_t, val_t, member_name) \
  static uint8_t *upb_put_ ## type(uint8_t *buf, val_t val) { \
    wire_t tmp = upb_vtowv_ ## type(val); \
    return upb_put_ ## v_or_f ## _ ## wire_t(buf, tmp); \
  }

#define T(type, v_or_f, wire_t, val_t, member_name) \
  static size_t upb_get_ ## type ## _size(val_t val) { \
    return upb_ ## v_or_f ## _ ## wire_t ## _size(val); \
  } \
  VTOWV(type, wire_t, val_t);  /* prototype for PUT below */ \
  PUT(type, v_or_f, wire_t, val_t, member_name) \
  VTOWV(type, wire_t, val_t)

T(INT32,    v,  int32_t, int32_t,  int32)   { return (uint32_t)s;     }
T(INT64,    v, uint64_t, int64_t,  int64)   { return (uint64_t)s;     }
T(UINT32,   v, uint32_t, uint32_t, uint32)  { return s;               }
T(UINT64,   v, uint64_t, uint64_t, uint64)  { return s;               }
T(SINT32,   v, uint32_t, int32_t,  int32)   { return upb_zzenc_32(s); }
T(SINT64,   v, uint64_t, int64_t,  int64)   { return upb_zzenc_64(s); }
T(FIXED32,  f, uint32_t, uint32_t, uint32)  { return s;               }
T(FIXED64,  f, uint64_t, uint64_t, uint64)  { return s;               }
T(SFIXED32, f, uint32_t, int32_t,  int32)   { return (uint32_t)s;     }
T(SFIXED64, f, uint64_t, int64_t,  int64)   { return (uint64_t)s;     }
T(BOOL,     v, uint32_t, bool,     _bool)   { return (uint32_t)s;     }
T(ENUM,     v, uint32_t, int32_t,  int32)   { return (uint32_t)s;     }
T(DOUBLE,   f, uint64_t, double,   _double) {
  upb_value v;
  v._double = s;
  return v.uint64;
}
T(FLOAT,    f, uint32_t, float,    _float)  {
  upb_value v;
  v._float = s;
  return v.uint32;
}
#undef VTOWV
#undef PUT
#undef T

static uint8_t *upb_encode_value(uint8_t *buf, upb_field_type_t ft, upb_value v)
{
#define CASE(t, member_name) \
  case UPB_TYPE(t): return upb_put_ ## t(buf, v.member_name);
  switch(ft) {
    CASE(DOUBLE,   _double)
    CASE(FLOAT,    _float)
    CASE(INT32,    int32)
    CASE(INT64,    int64)
    CASE(UINT32,   uint32)
    CASE(UINT64,   uint64)
    CASE(SINT32,   int32)
    CASE(SINT64,   int64)
    CASE(FIXED32,  uint32)
    CASE(FIXED64,  uint64)
    CASE(SFIXED32, int32)
    CASE(SFIXED64, int64)
    CASE(BOOL,     _bool)
    CASE(ENUM,     int32)
    default: assert(false); return buf;
  }
#undef CASE
}

static uint32_t _upb_get_value_size(upb_field_type_t ft, upb_value v)
{
#define CASE(t, member_name) \
  case UPB_TYPE(t): return upb_get_ ## t ## _size(v.member_name);
  switch(ft) {
    CASE(DOUBLE,   _double)
    CASE(FLOAT,    _float)
    CASE(INT32,    int32)
    CASE(INT64,    int64)
    CASE(UINT32,   uint32)
    CASE(UINT64,   uint64)
    CASE(SINT32,   int32)
    CASE(SINT64,   int64)
    CASE(FIXED32,  uint32)
    CASE(FIXED64,  uint64)
    CASE(SFIXED32, int32)
    CASE(SFIXED64, int64)
    CASE(BOOL,     _bool)
    CASE(ENUM,     int32)
    default: assert(false); return 0;
  }
#undef CASE
}

static uint8_t *_upb_put_tag(uint8_t *buf, upb_field_number_t num,
                             upb_wire_type_t wt)
{
  return upb_put_UINT32(buf, wt | (num << 3));
}

static uint32_t _upb_get_tag_size(upb_field_number_t num)
{
  return upb_get_UINT32_size(num << 3);
}


/* upb_sizebuilder ************************************************************/

struct upb_sizebuilder {
  // Accumulating size for the current level.
  uint32_t size;

  // Stack of sizes for our current nesting.
  uint32_t stack[UPB_MAX_NESTING], *top;

  // Vector of sizes.
  uint32_t *sizes;
  int sizes_len;
  int sizes_size;

  upb_status status;
};

// upb_sink callbacks.
static upb_sink_status _upb_sizebuilder_valuecb(upb_sink *sink, upb_fielddef *f,
                                                upb_value val,
                                                upb_status *status)
{
  (void)status;
  upb_sizebuilder *sb = (upb_sizebuilder*)sink;
  uint32_t size = 0;
  size += _upb_get_tag_size(f->number);
  size += _upb_get_value_size(f->type, val);
  sb->size += size;
  return UPB_SINK_CONTINUE;
}

static upb_sink_status _upb_sizebuilder_strcb(upb_sink *sink, upb_fielddef *f,
                                              upb_strptr str,
                                              int32_t start, uint32_t end,
                                              upb_status *status)
{
  (void)status;
  (void)str;   // String data itself is not used.
  upb_sizebuilder *sb = (upb_sizebuilder*)sink;
  if(start >= 0) {
    uint32_t size = 0;
    size += _upb_get_tag_size(f->number);
    size += upb_get_UINT32_size(end - start);
    sb->size += size;
  }
  return UPB_SINK_CONTINUE;
}

static upb_sink_status _upb_sizebuilder_startcb(upb_sink *sink, upb_fielddef *f,
                                                upb_status *status)
{
  (void)status;
  (void)f;  // Unused (we calculate tag size and delimiter in endcb).
  upb_sizebuilder *sb = (upb_sizebuilder*)sink;
  if(f->type == UPB_TYPE(MESSAGE)) {
    *sb->top = sb->size;
    sb->top++;
    sb->size = 0;
  } else {
    assert(f->type == UPB_TYPE(GROUP));
    sb->size += _upb_get_tag_size(f->number);
  }
  return UPB_SINK_CONTINUE;
}

static upb_sink_status _upb_sizebuilder_endcb(upb_sink *sink, upb_fielddef *f,
                                              upb_status *status)
{
  (void)status;
  upb_sizebuilder *sb = (upb_sizebuilder*)sink;
  if(f->type == UPB_TYPE(MESSAGE)) {
    sb->top--;
    if(sb->sizes_len == sb->sizes_size) {
      sb->sizes_size *= 2;
      sb->sizes = realloc(sb->sizes, sb->sizes_size * sizeof(*sb->sizes));
    }
    uint32_t child_size = sb->size;
    uint32_t parent_size = *sb->top;
    sb->sizes[sb->sizes_len++] = child_size;
    // The size according to the parent includes the tag size and delimiter of
    // the submessage.
    parent_size += upb_get_UINT32_size(child_size);
    parent_size += _upb_get_tag_size(f->number);
    // Include size accumulated in parent before child began.
    sb->size = child_size + parent_size;
  } else {
    assert(f->type == UPB_TYPE(GROUP));
    // As an optimization, we could just add this number twice in startcb, to
    // avoid having to recalculate it.
    sb->size += _upb_get_tag_size(f->number);
  }
  return UPB_SINK_CONTINUE;
}

upb_sink_callbacks _upb_sizebuilder_sink_vtbl = {
  _upb_sizebuilder_valuecb,
  _upb_sizebuilder_strcb,
  _upb_sizebuilder_startcb,
  _upb_sizebuilder_endcb
};


/* upb_sink callbacks *********************************************************/

struct upb_encoder {
  upb_sink base;
  //upb_bytesink *bytesink;
  uint32_t *sizes;
  int size_offset;
};


// Within one callback we may need to encode up to two separate values.
#define UPB_ENCODER_BUFSIZE (UPB_MAX_ENCODED_SIZE * 2)

static upb_sink_status _upb_encoder_push_buf(upb_encoder *s, const uint8_t *buf,
                                             size_t len, upb_status *status)
{
  // TODO: conjure a upb_strptr that points to buf.
  //upb_strptr ptr;
  (void)s;
  (void)buf;
  (void)status;
  size_t written = 5;// = upb_bytesink_onbytes(s->bytesink, ptr);
  if(written < len) {
    // TODO: mark to skip "written" bytes next time.
    return UPB_SINK_STOP;
  } else {
    return UPB_SINK_CONTINUE;
  }
}

static upb_sink_status _upb_encoder_valuecb(upb_sink *sink, upb_fielddef *f,
                                            upb_value val, upb_status *status)
{
  upb_encoder *s = (upb_encoder*)sink;
  uint8_t buf[UPB_ENCODER_BUFSIZE], *ptr = buf;
  upb_wire_type_t wt = upb_types[f->type].expected_wire_type;
  // TODO: handle packed encoding.
  ptr = _upb_put_tag(ptr, f->number, wt);
  ptr = upb_encode_value(ptr, f->type, val);
  return _upb_encoder_push_buf(s, buf, ptr - buf, status);
}

static upb_sink_status _upb_encoder_strcb(upb_sink *sink, upb_fielddef *f,
                                          upb_strptr str,
                                          int32_t start, uint32_t end,
                                          upb_status *status)
{
  upb_encoder *s = (upb_encoder*)sink;
  uint8_t buf[UPB_ENCODER_BUFSIZE], *ptr = buf;
  if(start >= 0) {
    ptr = _upb_put_tag(ptr, f->number, UPB_WIRE_TYPE_DELIMITED);
    ptr = upb_put_UINT32(ptr, end - start);
  }
  // TODO: properly handle partially consumed strings and partially supplied
  // strings.
  _upb_encoder_push_buf(s, buf, ptr - buf, status);
  return _upb_encoder_push_buf(s, (uint8_t*)upb_string_getrobuf(str), end - start, status);
}

static upb_sink_status _upb_encoder_startcb(upb_sink *sink, upb_fielddef *f,
                                            upb_status *status)
{
  upb_encoder *s = (upb_encoder*)sink;
  uint8_t buf[UPB_ENCODER_BUFSIZE], *ptr = buf;
  if(f->type == UPB_TYPE(GROUP)) {
    ptr = _upb_put_tag(ptr, f->number, UPB_WIRE_TYPE_START_GROUP);
  } else {
    ptr = _upb_put_tag(ptr, f->number, UPB_WIRE_TYPE_DELIMITED);
    ptr = upb_put_UINT32(ptr, s->sizes[--s->size_offset]);
  }
  return _upb_encoder_push_buf(s, buf, ptr - buf, status);
}

static upb_sink_status _upb_encoder_endcb(upb_sink *sink, upb_fielddef *f,
                                          upb_status *status)
{
  upb_encoder *s = (upb_encoder*)sink;
  uint8_t buf[UPB_ENCODER_BUFSIZE], *ptr = buf;
  if(f->type != UPB_TYPE(GROUP)) return UPB_SINK_CONTINUE;
  ptr = _upb_put_tag(ptr, f->number, UPB_WIRE_TYPE_END_GROUP);
  return _upb_encoder_push_buf(s, buf, ptr - buf, status);
}

upb_sink_callbacks _upb_encoder_sink_vtbl = {
  _upb_encoder_valuecb,
  _upb_encoder_strcb,
  _upb_encoder_startcb,
  _upb_encoder_endcb
};

