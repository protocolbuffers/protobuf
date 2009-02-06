/*
 * pbstream - a stream-oriented implementation of protocol buffers.
 *
 * Copyright (c) 2008 Joshua Haberman.  See LICENSE for details.
 */

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include "dynarray.h"

/* A list of types as they can appear in a .proto file. */
enum pbstream_type {
  PBSTREAM_TYPE_DOUBLE,
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
};

/* A list of types as they are encoded on-the-wire. */
enum pbstream_wire_type {
  PBSTREAM_WIRE_TYPE_VARINT      = 0,
  PBSTREAM_WIRE_TYPE_64BIT       = 1,
  PBSTREAM_WIRE_TYPE_STRING      = 2,
  PBSTREAM_WIRE_TYPE_START_GROUP = 3,
  PBSTREAM_WIRE_TYPE_END_GROUP   = 4,
  PBSTREAM_WIRE_TYPE_32BIT       = 5,
};

/* Each field must have a cardinality that is one of the following. */
enum pbstream_cardinality {
  PBSTREAM_CARDINALITY_OPTIONAL,  /* must appear 0 or 1 times */
  PBSTREAM_CARDINALITY_REQUIRED,  /* must appear exactly 1 time */
  PBSTREAM_CARDINALITY_REPEATED,  /* may appear 0 or more times */
};

typedef int32_t pbstream_field_number_t;

/* A deserialized value as described in a .proto file. */
struct pbstream_value {
  enum pbstream_type type;
  union {
    double _double;
    float  _float;
    int32_t int32;
    int64_t int64;
    uint32_t uint32;
    uint64_t uint64;
    bool _bool;
    struct {
      char *data;  /* This will be a pointer to the buffer of data the client provided. */
      int len;
    } string;
    struct {
      char *data;  /* This will be a pointer to the buffer of data the client provided. */
      int len;
    } bytes;
    int32_t _enum;
  } v;
};

/* A value as it is encoded on-the-wire */
struct pbstream_wire_value {
  enum pbstream_wire_type type;
  union {
    uint64_t varint;
    uint64_t _64bit;
    struct {
      char *data;  /* This will be a pointer to the buffer of data the client provided. */
      int len;
    } string;
    uint32_t _32bit;
  } v;
};

/* The definition of an enum as defined in a pbstream.  For example:
 * Corpus {
 *   UNIVERSAL = 0;
 *   WEB = 1;
 *   IMAGES = 2;
 *   LOCAL = 3;
 *   NEWS = 4;
 * }
 */
struct pbstream_enum_descriptor {
  char *name;
  struct enum_value {
    char *name;
    int value;
  } value;
  DEFINE_DYNARRAY(values, struct enum_value);
};

/* The definition of a field as defined in a pbstream (within a message).
 * For example:
 *   required int32 a = 1;
 */
struct pbstream_field_descriptor {
  pbstream_field_number_t field_number;
  char *name;
  enum pbstream_type type;
  enum pbstream_cardinality cardinality;
  struct pbstream_value *default_value;  /* NULL if none */

  /* Index into the "seen" list for the message.  -1 for repeated fields (for
   * which we have no need to track whether it's been seen). */
  int seen_field_num;

  union extra_data {
    struct pbstream_enum_descriptor *_enum;
    struct pbstream_message_descriptor *message;
  } d;
};

/* A message as defined by the "message" construct in a .proto file. */
struct pbstream_message_descriptor {
  char *name;  /* does not include package name or parent message names. */
  char *full_name;
  int num_seen_fields;  /* How many fields we have to track "seen" information for. */
  DEFINE_DYNARRAY(fields, struct pbstream_field_descriptor);
  DEFINE_DYNARRAY(messages, struct pbstream_message_descriptor);
  DEFINE_DYNARRAY(enums, struct pbstream_enum_descriptor);
};

/* Callback for when a value is parsed that matches a field in the .proto file.
 * */
typedef void (*pbstream_value_callback_t)(
    struct pbstream_field_descriptor *field_descriptor,
    struct pbstream_value            value,
    void *user_data);

/* Callback for when a value is parsed for which no field was defined in the
 * .proto file. */
typedef void (*pbstream_unknown_value_callback_t)(
    pbstream_field_number_t field_number,
    struct pbstream_wire_value *wire_value,
    void *user_data);

/* Callback for when a nested message is beginning. */
typedef void (*pbstream_begin_message_callback_t)(
    struct pbstream_message_descriptor *message_descriptor,
    void *user_data);

/* Callback for when a nested message is ending. */
typedef void (*pbstream_end_message_callback_t)(void *user_data);

/* Callback for when an error occurred. */
enum pbstream_error {
  PBSTREAM_ERROR_UNTERMINATED_VARINT,     /* A varint did not terminate before hitting 64 bits. Fatal. */
  PBSTREAM_ERROR_MISSING_REQUIRED_FIELD,  /* A field marked "required" was not present. */
  PBSTREAM_ERROR_DUPLICATE_FIELD,         /* An optional or required field appeared more than once. */
  PBSTREAM_ERROR_MISMATCHED_TYPE,         /* A field was encoded with the wrong wire type. */
  PBSTREAM_ERROR_BAD_SUBMESSAGE_END,      /* A submessage ended in the middle of data.  Indicates corruption. */
};
/* The description is a static buffer which the client must not free.  The
 * offset is the location in the input where the error was detected (this
 * offset is relative to the beginning of the stream).  If is_fatal is true,
 * parsing cannot continue. */
typedef void (*pbstream_error_callback_t)(enum pbstream_error error, char *description,
                                          int offset, bool is_fatal);

struct pbstream_callbacks {
  pbstream_value_callback_t          value_callback;
  pbstream_unknown_value_callback_t  unknown_value_callback;
  pbstream_begin_message_callback_t  begin_message_callback;
  pbstream_end_message_callback_t    end_message_callback;
  pbstream_error_callback_t          error_callback;
};

struct pbstream_parse_stack_frame {
  struct pbstream_message_descriptor *message_descriptor;
  int end_offset;  /* We don't know this for the outermost frame, and set it to INT_MAX. */

  /* For every field except repeated ones we track whether we have seen it or
   * not.  This lets us detect three important conditions:
   * 1. the field has a default, but we did not see it anywhere (action: emit the default)
   * 2. the field is required, but we did not see it anywhere (action: error)
   * 3. the field is required or optional, but we saw it more than once (action: error) */
  DEFINE_DYNARRAY(seen_fields, bool);
};

/* The stream parser keeps this as its state. */
struct pbstream_parse_state {
  struct pbstream_callbacks callbacks;
  int offset;
  bool fatal_error;
  void *user_data;
  DEFINE_DYNARRAY(stack, struct pbstream_parse_stack_frame);
};

/* Call this once before parsing to initialize the data structures.
 * message_type can be NULL, in which case all fields will be reported as
 * unknown. */
void pbstream_init_parser(struct pbstream_parse_state *state,
                          struct pbstream_message_descriptor *message_descriptor,
                          struct pbstream_callbacks *callbacks,
                          void *user_data);

/* Call this to parse as much of buf as possible, calling callbacks as
 * appropriate.  buf need not be a complete pbstream.  Returns the number of
 * bytes consumed.  In subsequent calls, buf should point to the first byte not
 * consumed by previous calls.
 *
 * If need_more_bytes is non-zero when parse() returns, this indicates that the
 * beginning of a string or sub-message was recognized, but not all bytes of
 * the string were in memory.  The string will not be successfully parsed (and
 * thus parsing of the pbstream cannot proceed) unless need_more_bytes more
 * data is available upon the next call to parse.  The caller may need to
 * increase its buffer size. */
enum pbstream_status {
  PBSTREAM_STATUS_OK = 0,
  PBSTREAM_STATUS_INCOMPLETE = 1,  /* buffer ended in the middle of a field.  */
  PBSTREAM_STATUS_ERROR = 2,       /* fatal error in the file, cannot recover. */
};

enum pbstream_status pbstream_parse(struct pbstream_parse_state *state,
                                    char *buf, int buf_len,
                                    int *consumed_bytes, int *need_more_bytes);
