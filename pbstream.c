/*
 * pbstream - a stream-oriented implementation of protocol buffers.
 *
 * Copyright (c) 2008 Joshua Haberman.  See LICENSE for details.
 */

#include <limits.h>
#include <string.h>
#include "pbstream.h"

/* Branch prediction hints for GCC. */
#ifdef __GNUC__
#define likely(x)       __builtin_expect((x),1)
#define unlikely(x)     __builtin_expect((x),0)
#else
#define likely(x)
#define unlikely(x)
#endif

/* An array, indexed by pbstream_type, that indicates what wire type is
 * expected for the given pbstream type. */
static enum pbstream_wire_type expected_wire_type[] = {
  PBSTREAM_WIRE_TYPE_64BIT,    // PBSTREAM_TYPE_DOUBLE,
  PBSTREAM_WIRE_TYPE_32BIT,    // PBSTREAM_TYPE_FLOAT,
  PBSTREAM_WIRE_TYPE_VARINT,   // PBSTREAM_TYPE_INT32,
  PBSTREAM_WIRE_TYPE_VARINT,   // PBSTREAM_TYPE_INT64,
  PBSTREAM_WIRE_TYPE_VARINT,   // PBSTREAM_TYPE_UINT32,
  PBSTREAM_WIRE_TYPE_VARINT,   // PBSTREAM_TYPE_UINT64,
  PBSTREAM_WIRE_TYPE_VARINT,   // PBSTREAM_TYPE_SINT32,
  PBSTREAM_WIRE_TYPE_VARINT,   // PBSTREAM_TYPE_SINT64,
  PBSTREAM_WIRE_TYPE_32BIT,    // PBSTREAM_TYPE_FIXED32,
  PBSTREAM_WIRE_TYPE_64BIT,    // PBSTREAM_TYPE_FIXED64,
  PBSTREAM_WIRE_TYPE_32BIT,    // PBSTREAM_TYPE_SFIXED32,
  PBSTREAM_WIRE_TYPE_64BIT,    // PBSTREAM_TYPE_SFIXED64,
  PBSTREAM_WIRE_TYPE_VARINT,   // PBSTREAM_TYPE_BOOL,
  PBSTREAM_WIRE_TYPE_STRING,   // PBSTREAM_TYPE_STRING,
  PBSTREAM_WIRE_TYPE_STRING,   // PBSTREAM_TYPE_BYTES,
  PBSTREAM_WIRE_TYPE_VARINT,   // PBSTREAM_TYPE_ENUM,
  PBSTREAM_WIRE_TYPE_STRING,   // PBSTREAM_TYPE_MESSAGE
};

/* Reads a varint starting at buf (but not past end), storing the result
 * in out_value.  Returns whether the operation was successful.  */
enum pbstream_status get_varint(char **buf, char *end, uint64_t *out_value)
{
  *out_value = 0;
  int bitpos = 0;
  char *b = *buf;

  /* Because we don't check for buffer overrun inside the loop, we require
   * that callers use a buffer that is overallocated by at least 9 bytes
   * (the maximum we can overrun before the bitpos check catches the problem). */
  for(; *b & 0x80 && bitpos < 64; bitpos += 7, b++)
      *out_value |= (*b & 0x7F) << bitpos;

  if(unlikely(bitpos >= 64)) {
    return PBSTREAM_ERROR_UNTERMINATED_VARINT;
  }
  if(unlikely(b > end)) {
    return PBSTREAM_STATUS_INCOMPLETE;
  }

  *out_value |= (*b & 0x7F) << bitpos;
  *buf = b;
  return PBSTREAM_STATUS_OK;
}

/* TODO: the little-endian versions of these functions don't respect alignment.
 * While it's hard to believe that this could be less efficient than the
 * alternative (the big endian implementation), this deserves some tests and
 * measurements to be sure. */
enum pbstream_status get_32_le(char **buf, char *end, uint32_t *out_value)
{
  char *b = *buf;
  char *int32_end = b+4;
  if(unlikely(int32_end > end))
    return PBSTREAM_STATUS_INCOMPLETE;

#if __BYTE_ORDER == __LITTLE_ENDIAN
  *out_value = *(uint32_t*)b;
#else
  *out_value = b[0] | (b[1] << 8) | (b[2] << 16) | (b[3] << 24);
#endif
  *buf = int32_end;
  return PBSTREAM_STATUS_OK;
}

bool get_64_le(char **buf, char *end, uint64_t *out_value)
{
  char *b = *buf;
  char *int64_end = b+8;
  if(unlikely(int64_end > end))
    return PBSTREAM_STATUS_INCOMPLETE;

#if __BYTE_ORDER == __LITTLE_ENDIAN
  *out_value = *(uint64_t*)buf;
#else
  *out_value = (b[0])       | (b[1] << 8 ) | (b[2] << 16) | (b[3] << 24) |
               (b[4] << 32) | (b[5] << 40) | (b[6] << 48) | (b[7] << 56);
#endif
  *buf = int64_end;
  return PBSTREAM_STATUS_OK;
}

int32_t zigzag_decode_32(uint64_t src)
{
  return 0;  /* TODO */
}

int64_t zigzag_decode_64(uint64_t src)
{
  return 0;  /* TODO */
}

/* Parses the next field-number/wire-value pair from the stream of bytes
 * starting at *buf, without reading past end.  Stores the parsed and wire
 * value in *field_number and *wire_value, respectively.
 *
 * Returns a status indicating whether the operation was successful.  If the
 * return status is STATUS_INCOMPLETE, returns the number of additional bytes
 * requred in *need_more_bytes.  Updates *buf to point past the end of the
 * parsed data if the operation was successful.
 */
enum pbstream_status pbstream_parse_wire_value(char **buf, char *end,
                                               pbstream_field_number_t *field_number,
                                               struct pbstream_wire_value *wire_value,
                                               int *need_more_bytes)
{
  char *b = *buf;  /* Our local buf pointer -- only update buf if we succeed. */

#define DECODE(dest, func) \
  do { \
    enum pbstream_status status = func(&b, end, &dest); \
    if(unlikely(status != PBSTREAM_STATUS_OK)) { \
      *need_more_bytes = 0;  /* This only arises below in this function. */ \
      return status; \
    } \
  } while (0)

  uint64_t key;
  DECODE(key, get_varint);

  *field_number    = key >> 3;
  wire_value->type = key & 0x07;

  switch(wire_value->type)
  {
    case PBSTREAM_WIRE_TYPE_VARINT:
      DECODE(wire_value->v.varint, get_varint);
      break;

    case PBSTREAM_WIRE_TYPE_64BIT:
      DECODE(wire_value->v._64bit, get_64_le);
      break;

    case PBSTREAM_WIRE_TYPE_STRING:
    {
      uint64_t string_len;
      DECODE(string_len, get_varint);
      if (string_len > INT_MAX) {
        /* TODO: notice this and fail. */
      }
      wire_value->v.string.len = (int)string_len;
      if(b + wire_value->v.string.len > end) {
        *need_more_bytes = b + wire_value->v.string.len - end;
        return PBSTREAM_STATUS_INCOMPLETE;
      }
      wire_value->v.string.data = b;
      b += wire_value->v.string.len;
      break;
    }

    case PBSTREAM_WIRE_TYPE_START_GROUP:
    case PBSTREAM_WIRE_TYPE_END_GROUP:
      /* TODO (though these are deprecated, so not high priority). */
      break;

    case PBSTREAM_WIRE_TYPE_32BIT:
      DECODE(wire_value->v._32bit, get_32_le);
      break;
  }

  *buf = b;
  return true;
}

/* Translates from a wire value to a .proto value.  The caller should have
 * already checked that the wire_value is of the correct type.  The pbstream
 * type must not be PBSTREAM_TYPE_MESSAGE.  This operation always succeeds. */
void pbstream_translate_field(struct pbstream_wire_value *wire_value,
                              enum pbstream_type         type,
                              struct pbstream_value      *out_value)
{
  out_value->type = type;
  switch(type) {
    case PBSTREAM_TYPE_DOUBLE:
      memcpy(&out_value->v._double, &wire_value->v._64bit, sizeof(double));
      break;

    case PBSTREAM_TYPE_FLOAT:
      memcpy(&out_value->v._float, &wire_value->v._32bit, sizeof(float));
      break;

    case PBSTREAM_TYPE_INT32:
      out_value->v.int32 = (int32_t)wire_value->v.varint;
      break;

    case PBSTREAM_TYPE_INT64:
      out_value->v.int64 = (int64_t)zigzag_decode_64(wire_value->v.varint);
      break;

    case PBSTREAM_TYPE_UINT32:
      out_value->v.uint32 = (uint32_t)wire_value->v.varint;
      break;

    case PBSTREAM_TYPE_UINT64:
      out_value->v.uint64 = (uint64_t)wire_value->v.varint;
      break;

    case PBSTREAM_TYPE_SINT32:
      out_value->v.int32 = zigzag_decode_32(wire_value->v.varint);
      break;

    case PBSTREAM_TYPE_SINT64:
      out_value->v.int64 = zigzag_decode_64(wire_value->v.varint);
      break;

    case PBSTREAM_TYPE_FIXED32:
      out_value->v.int32 = wire_value->v._32bit;
      break;

    case PBSTREAM_TYPE_FIXED64:
      out_value->v.int64 = wire_value->v._64bit;
      break;

    case PBSTREAM_TYPE_SFIXED32:
      out_value->v.int32 = (int32_t)wire_value->v._32bit;
      break;

    case PBSTREAM_TYPE_SFIXED64:
      out_value->v.int64 = (int64_t)wire_value->v._64bit;
      break;

    case PBSTREAM_TYPE_BOOL:
      out_value->v._bool = (bool)wire_value->v.varint;
      break;

    case PBSTREAM_TYPE_STRING:
      out_value->v.string.data = wire_value->v.string.data;
      out_value->v.string.len = wire_value->v.string.len;
      /* TODO: validate UTF-8? */
      break;

    case PBSTREAM_TYPE_BYTES:
      out_value->v.bytes.data = wire_value->v.string.data;
      out_value->v.bytes.len = wire_value->v.string.len;
      break;

    case PBSTREAM_TYPE_ENUM:
      out_value->v._enum = (bool)wire_value->v.varint;
      break;

    case PBSTREAM_TYPE_MESSAGE:
      /* Should never happen. */
      break;
  }
}

/* Given a wire value that was just parsed and a matching field descriptor,
 * processes the given value and performs the appropriate actions.  These
 * actions include:
 * - checking that the wire type is as expected
 * - converting the wire type to a .proto type
 * - entering a sub-message, if that is in fact what this field implies.
 *
 * This function also calls user callbacks pertaining to any of the above at
 * the appropriate times. */
void process_value(struct pbstream_parse_state *s,
                   struct pbstream_wire_value *wire_value,
                   struct pbstream_field_descriptor *field_descriptor)
{
  /* Check that the wire type is appropriate for this .proto type. */
  if(unlikely(wire_value->type != expected_wire_type[field_descriptor->type])) {
    /* Report the type mismatch error. */
    if(s->callbacks.error_callback) {
      /* TODO: a nice formatted message. */
      s->callbacks.error_callback(PBSTREAM_ERROR_MISMATCHED_TYPE, NULL,
                                  s->offset, false);
    }

    /* Report the wire value we parsed as an unknown value. */
    if(s->callbacks.unknown_value_callback) {
      s->callbacks.unknown_value_callback(field_descriptor->field_number, wire_value,
                                          s->user_data);
    }
    return;
  }

  if(field_descriptor->type == PBSTREAM_TYPE_MESSAGE) {
    /* We're entering a sub-message. */
    if(s->callbacks.begin_message_callback) {
      s->callbacks.begin_message_callback(field_descriptor->d.message, s->user_data);
    }

    /* Push and initialize a new stack frame. */
    RESIZE_DYNARRAY(s->stack, s->stack_len+1);
    struct pbstream_parse_stack_frame *frame = DYNARRAY_GET_TOP(s->stack);
    frame->message_descriptor = field_descriptor->d.message;
    frame->end_offset = 0;  /* TODO: set this correctly. */
    int num_seen_fields = frame->message_descriptor->num_seen_fields;
    INIT_DYNARRAY(frame->seen_fields, num_seen_fields, num_seen_fields);
  }
  else {
    /* This is a scalar value. */
    struct pbstream_value value;
    pbstream_translate_field(wire_value, field_descriptor->type, &value);
    if(s->callbacks.value_callback) {
      s->callbacks.value_callback(field_descriptor, value, s->user_data);
    }
  }
}

struct pbstream_field_descriptor *find_field_descriptor_by_number(
    struct pbstream_message_descriptor* message_descriptor,
    pbstream_field_number_t field_number)
{
  /* Currently a linear search -- could be optimized to do a binary search,
   * hash table lookup, or any other number of clever things you might imagine. */
  for (int i = 0; i < message_descriptor->fields_len; i++)
    if (message_descriptor->fields[i].field_number == field_number)
      return &message_descriptor->fields[i];
  return NULL;
}

/* Parses and processes the next value from *buf (but not past end), returning
 * a status indicating whether the operation succeeded, and calling appropriate
 * callbacks.  If more data is needed to parse the last partial field, returns
 * how many more bytes are needed in need_more_bytes.  Updates *buf to point
 * past the parsed value if the operation succeeds. */
enum pbstream_status pbstream_parse_field(struct pbstream_parse_state *s,
                                          char **buf, char *end,
                                          int *need_more_bytes)
{
  struct pbstream_parse_stack_frame *frame = DYNARRAY_GET_TOP(s->stack);
  struct pbstream_message_descriptor *message_descriptor = frame->message_descriptor;

  pbstream_field_number_t field_number;
  struct pbstream_wire_value wire_value;
  enum pbstream_status status;

  /* Decode the raw wire data. */
  status = pbstream_parse_wire_value(buf, end, &field_number, &wire_value,
                                     need_more_bytes);

  if(unlikely(status != PBSTREAM_STATUS_OK)) {
    if(status == PBSTREAM_ERROR_UNTERMINATED_VARINT && s->callbacks.error_callback) {
      /* TODO: a nice formatted message. */
      s->callbacks.error_callback(PBSTREAM_ERROR_UNTERMINATED_VARINT, NULL,
                                  s->offset, true);
    }
    s->fatal_error = true;
    return status;
  }

  /* Find the corresponding field definition from the .proto file. */
  struct pbstream_field_descriptor *field_descriptor;
  field_descriptor = find_field_descriptor_by_number(message_descriptor, field_number);


  if(likely(field_descriptor != NULL)) {
    if(field_descriptor->seen_field_num > 0) {
      /* Check that this field has not been seen before (unless it's a repeated field) */
      if(frame->seen_fields[field_descriptor->seen_field_num] &&
         field_descriptor->cardinality != PBSTREAM_CARDINALITY_REPEATED) {
        if(s->callbacks.error_callback) {
          s->callbacks.error_callback(PBSTREAM_ERROR_DUPLICATE_FIELD, NULL,
                                      s->offset, false);
        }
        return PBSTREAM_STATUS_ERROR;
      }

      /* Mark the field as seen. */
      frame->seen_fields[field_descriptor->seen_field_num] = true;
    }
    process_value(s, &wire_value, field_descriptor);
  } else {
    /* This field was not defined in the .proto file. */
    if(s->callbacks.unknown_value_callback) {
      s->callbacks.unknown_value_callback(field_number, &wire_value, s->user_data);
    }
  }
  return PBSTREAM_STATUS_OK;
}

/* Process actions associated with the end of a submessage.  This includes:
 * - emittin default values for all optional elements (either explicit
 *   defaults or implicit defaults).
 * - emitting errors for any required fields that were not seen.
 * - calling the user's callback.
 * - popping the stack frame. */
void process_submessage_end(struct pbstream_parse_state *s)
{
  /* TODO: emit default values for optional elements.  either explicit defaults
   * (specified in the .proto file) or implicit defaults (which are specified
   * in the pbstream definition, by type. */

  /* TODO: emit errors for required fields that were not seen. */

  /* Process the end of message by calling the user's callback and popping
   * our stack frame. */
  if(s->callbacks.end_message_callback) {
    s->callbacks.end_message_callback(s->user_data);
  }

  /* Pop the stack frame associated with this submessage. */
  RESIZE_DYNARRAY(s->stack, s->stack_len-1);
}

/* The user-exposed parsing function -- see the header file for documentation. */
enum pbstream_status pbstream_parse(struct pbstream_parse_state *s,
                                    char *buf_start, int buf_len,
                                    int *consumed_bytes, int *need_more_bytes)
{
  char *buf = buf_start;
  char *end = buf_start + buf_len;
  int buf_start_offset = s->offset;
  enum pbstream_status status = PBSTREAM_STATUS_OK;

  while(buf < end) {
    /* Check for a submessage ending. */
    while(s->offset >= DYNARRAY_GET_TOP(s->stack)->end_offset) {
      /* A submessage should end exactly at a field boundary.  If we find that
       * the submessage length indicated an end in the middle of a field, that
       * is an error that indicates data corruption, and we refuse to proceed. */
      if(unlikely(s->offset != DYNARRAY_GET_TOP(s->stack)->end_offset)) {
        if(s->callbacks.error_callback) {
          s->callbacks.error_callback(PBSTREAM_ERROR_BAD_SUBMESSAGE_END, NULL,
                                      s->offset, true);
        }
        s->fatal_error = true;
        break;
      }
      process_submessage_end(s);
    }

    status = pbstream_parse_field(s, &buf, end, need_more_bytes);
    if(status != PBSTREAM_STATUS_OK)
      break;

    s->offset = buf_start_offset + (buf - buf_start);
  }
  return status;
}
