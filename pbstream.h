/*
 * pbstream - a stream-oriented implementation of protocol buffers.
 *
 * Copyright (c) 2008 Joshua Haberman.  See LICENSE for details.
 */

#ifndef PBSTREAM_H_
#define PBSTREAM_H_

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* The maximum that any submessages can be nested.  Matches proto2's limit. */
#define PBSTREAM_MAX_STACK 64

/* A list of types as they can appear in a .proto file. */
typedef enum pbstream_type {
  PBSTREAM_TYPE_DOUBLE = 0,
  PBSTREAM_TYPE_FLOAT,
  PBSTREAM_TYPE_INT32,
  PBSTREAM_TYPE_INT64,
  PBSTREAM_TYPE_UINT32,
  PBSTREAM_TYPE_UINT64,
  PBSTREAM_TYPE_SINT32,
  PBSTREAM_TYPE_SINT64,
  PBSTREAM_TYPE_FIXED32,
  PBSTREAM_TYPE_FIXED64,
  PBSTREAM_TYPE_SFIXED32,
  PBSTREAM_TYPE_SFIXED64,
  PBSTREAM_TYPE_BOOL,
  PBSTREAM_TYPE_STRING,
  PBSTREAM_TYPE_BYTES,
  PBSTREAM_TYPE_ENUM,
  PBSTREAM_TYPE_MESSAGE
} pbstream_type_t;

/* A list of types as they are encoded on-the-wire. */
typedef enum pbstream_wire_type {
  PBSTREAM_WIRE_TYPE_VARINT      = 0,
  PBSTREAM_WIRE_TYPE_64BIT       = 1,
  PBSTREAM_WIRE_TYPE_DELIMITED   = 2,
  PBSTREAM_WIRE_TYPE_START_GROUP = 3,
  PBSTREAM_WIRE_TYPE_END_GROUP   = 4,
  PBSTREAM_WIRE_TYPE_32BIT       = 5,
} pbstream_wire_type_t;

typedef int32_t pbstream_field_number_t;

/* A deserialized value as described in a .proto file. */
struct pbstream_tagged_value {
  struct pbstream_field *field;
  union pbstream_value {
    double _double;
    float  _float;
    int32_t int32;
    int64_t int64;
    uint32_t uint32;
    uint64_t uint64;
    bool _bool;
    struct pbstream_delimited {
      size_t offset;  /* relative to the beginning of the stream. */
      uint32_t len;
    } delimited;
  } v;
};

/* A value as it is encoded on-the-wire, before it has been interpreted as
 * any particular .proto type. */
struct pbstream_tagged_wire_value {
  pbstream_wire_type_t type;
  union pbstream_wire_value {
    uint64_t varint;
    uint64_t _64bit;
    struct {
      size_t offset;  /* relative to the beginning of the stream. */
      uint32_t len;
    } delimited;
    uint32_t _32bit;
  } v;
};

/* Definition of a single field in a message.  Note that this does not include
 * nearly all of the information that can be specified about a field in a
 * .proto file.  For example, we don't even know the field's name.  We keep
 * only the information necessary to parse the field. */
struct pbstream_field {
  pbstream_field_number_t field_number;
  pbstream_type_t type;
  struct pbstream_fieldset *fieldset;  /* if type == MESSAGE */
};

/* A fieldset is a data structure that supports fast lookup of fields by number.
 * It is logically a map of {field_number -> struct pbstream_field*}.  Fast
 * lookup is important, because it is in the critical path of parsing. */
struct pbstream_fieldset {
  int num_fields;
  struct pbstream_field *fields;
  int array_size;
  struct pbstream_field **array;
  /* TODO: the hashtable part. */
};

/* Takes an array of num_fields fields and builds an optimized table for fast
 * lookup of fields by number.  The input fields need not be sorted.  This
 * fieldset must be freed with pbstream_free_fieldset(). */
void pbstream_init_fieldset(struct pbstream_fieldset *fieldset,
                            struct pbstream_field *fields,
                            int num_fields);
void pbstream_free_fieldset(struct pbstream_fieldset *fieldset);

struct pbstream_parse_stack_frame {
  struct pbstream_fieldset *fieldset;
  size_t end_offset;  /* unknown for the top frame, so we set to SIZE_MAX */
};

/* The stream parser's state. */
struct pbstream_parse_state {
  size_t offset;
  struct pbstream_parse_stack_frame stack[PBSTREAM_MAX_STACK];
  struct pbstream_parse_stack_frame *top, *limit;
};

/* Call this once before parsing to initialize the data structures.
 * message_type can be NULL, in which case all fields will be reported as
 * unknown. */
void pbstream_init_parser(
    struct pbstream_parse_state *state,
    struct pbstream_fieldset *toplevel_fieldset);

/* Status as returned by pbstream_parse().  Status codes <0 are fatal errors
 * that cannot be recovered.  Status codes >0 are unusual but nonfatal events,
 * which nonetheless must be handled differently since they do not return data
 * in val. */
typedef enum pbstream_status {
  PBSTREAM_STATUS_OK = 0,
  PBSTREAM_STATUS_SUBMESSAGE_END = 1,  // No data is stored in val or wv.

  /** FATAL ERRORS: these indicate corruption, and cannot be recovered. */

  // A varint did not terminate before hitting 64 bits.
  PBSTREAM_ERROR_UNTERMINATED_VARINT = -1,

  // A submessage ended in the middle of data.
  PBSTREAM_ERROR_BAD_SUBMESSAGE_END = -2,

  // Encountered a "group" on the wire (deprecated and unsupported).
  PBSTREAM_ERROR_GROUP = -3,

  // Input was nested more than PBSTREAM_MAX_STACK deep.
  PBSTREAM_ERROR_STACK_OVERFLOW = -4,

  // The input data caused the pb's offset (a size_t) to overflow.
  PBSTREAM_ERROR_OVERFLOW = -5,

  /** NONFATAL ERRORS: the input was invalid, but we can continue if desired. */

  // A value was encountered that was not defined in the .proto file.  The
  // unknown value is stored in wv.
  PBSTREAM_ERROR_UNKNOWN_VALUE = 2,

  // A field was encoded with the wrong wire type.  The wire value is stored in
  // wv.
  PBSTREAM_ERROR_MISMATCHED_TYPE = 3,
} pbstream_status_t;
struct pbstream_parse_state;

/* The main parsing function.  Parses the next value from buf, storing the
 * parsed value in val.  If val is of type PBSTREAM_TYPE_MESSAGE, then a
 * submessage was entered.
 *
 * IMPORTANT NOTE: for efficiency, the parsing routines do not do bounds checks,
 * and may read as much as far as buf+10.  So the caller must ensure that buf is
 * not within 10 bytes of unmapped memory, or the program will segfault. Clients
 * are encouraged to overallocate their buffers by ten bytes to compensate. */
pbstream_status_t pbstream_parse_field(struct pbstream_parse_state *s,
                                       uint8_t *buf,
                                       pbstream_field_number_t *fieldnum,
                                       struct pbstream_tagged_value *val,
                                       struct pbstream_tagged_wire_value *wv);

#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif  /* PBSTREAM_H_ */
