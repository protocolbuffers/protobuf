/*
 * pbstream - a stream-oriented implementation of protocol buffers.
 *
 * Copyright (c) 2008 Joshua Haberman.  See LICENSE for details.
 */

#include <stdint.h>
#include <stdbool.h>

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
struct pbstream_value {
  struct pbstream_field *field;
  union {
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
    int32_t _enum;
  } v;
};

/* A tag occurs before each value on-the-wire. */
struct pbstream_tag {
  pbstream_field_number_t field_number;
  pbstream_wire_type_t wire_type;
};

/* A value as it is encoded on-the-wire */
struct pbstream_wire_value {
  pbstream_wire_type_t type;
  union {
    uint64_t varint;
    uint64_t _64bit;
    struct {
      size_t offset;  /* relative to the beginning of the stream. */
      uint32_t len;
    } delimited;
    uint32_t _32bit;
  } v;
};

/* Definition of a single field in a message. */
struct pbstream_field {
  pbstream_field_number_t field_number;
  pbstream_type_t type;
  struct pbstream_fieldset *fieldset;  /* if type == MESSAGE */
};

/* The set of fields corresponding to a message definition. */
struct pbstream_fieldset {
  /* TODO: a hybrid array/hashtable structure. */
  int num_fields;
  struct pbstream_field fields[];
};

struct pbstream_parse_stack_frame {
  struct pbstream_fieldset *fieldset;
  size_t end_offset;  /* unknown for the top frame, so we set to SIZE_MAX */
};

/* The stream parser's state. */
struct pbstream_parse_state {
  size_t offset;
  struct pbstream_parse_stack_frame *base, *top, *limit;
};

/* Call this once before parsing to initialize the data structures.
 * message_type can be NULL, in which case all fields will be reported as
 * unknown. */
void pbstream_init_parser(
    struct pbstream_parse_state *state,
    struct pbstream_fieldset *toplevel_fieldset);

void pbstream_free_parser(struct pbstream_parse_state *state);

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
                                       char *buf,
                                       pbstream_field_number_t *fieldnum,
                                       struct pbstream_value *val,
                                       struct pbstream_wire_value *wv);
