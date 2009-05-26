/*
 * upb - a minimalist implementation of protocol buffers.
 *
 * This file contains parsing routines; both stream-oriented and tree-oriented
 * models are supported.
 *
 * Copyright (c) 2008 Joshua Haberman.  See LICENSE for details.
 */

#ifndef UPB_PARSE_H_
#define UPB_PARSE_H_

#include <stdint.h>
#include <stdbool.h>
#include "upb.h"

#ifdef __cplusplus
extern "C" {
#endif

/* A deserialized value as described in a .proto file. */
struct upb_tagged_value {
  struct upb_field *field;
  union upb_value v;
};

/* A value as it is encoded on-the-wire, before it has been interpreted as
 * any particular .proto type. */
struct upb_tagged_wire_value {
  upb_wire_type_t type;
  union upb_wire_value v;
};

/* Definition of a single field in a message, for the purposes of the parser's
 * fieldmap.  Note that this does not include nearly all of the information
 * that can be specified about a field in a .proto file.  For example, we don't
 * even know the field's name.  We keep only the information necessary to parse
 * the field. */
struct upb_field {
  upb_field_number_t field_number;
  int32_t type;  /* google_protobuf_FieldDescriptorProto_Type */
  struct upb_fieldset *fieldset;  /* if type == MESSAGE */
};

struct upb_parse_stack_frame {
  struct upb_fieldset *fieldset;
  size_t end_offset;  /* unknown for the top frame, so we set to SIZE_MAX */
};

/* The stream parser's state. */
struct upb_parse_state {
  size_t offset;
  struct upb_parse_stack_frame stack[UPB_MAX_STACK];
  struct upb_parse_stack_frame *top, *limit;
};

/* Call this once before parsing to initialize the data structures.
 * message_type can be NULL, in which case all fields will be reported as
 * unknown. */
void upb_init_parser(struct upb_parse_state *state,
                     struct upb_fieldset *toplevel_fieldset);

/* Status as returned by upb_parse().  Status codes <0 are fatal errors
 * that cannot be recovered.  Status codes >0 are unusual but nonfatal events,
 * which nonetheless must be handled differently since they do not return data
 * in val. */
typedef enum upb_status {
  UPB_STATUS_OK = 0,
  UPB_STATUS_SUBMESSAGE_END = 1,  // No data is stored in val or wv.

  /** FATAL ERRORS: these indicate corruption, and cannot be recovered. */

  // A varint did not terminate before hitting 64 bits.
  UPB_ERROR_UNTERMINATED_VARINT = -1,

  // A submessage ended in the middle of data.
  UPB_ERROR_BAD_SUBMESSAGE_END = -2,

  // Encountered a "group" on the wire (deprecated and unsupported).
  UPB_ERROR_GROUP = -3,

  // Input was nested more than UPB_MAX_NESTING deep.
  UPB_ERROR_STACK_OVERFLOW = -4,

  // The input data caused the pb's offset (a size_t) to overflow.
  UPB_ERROR_OVERFLOW = -5,

  /** NONFATAL ERRORS: the input was invalid, but we can continue if desired. */

  // A value was encountered that was not defined in the .proto file.  The
  // unknown value is stored in wv.
  UPB_ERROR_UNKNOWN_VALUE = 2,

  // A field was encoded with the wrong wire type.  The wire value is stored in
  // wv.
  UPB_ERROR_MISMATCHED_TYPE = 3,
} upb_status_t;
struct upb_parse_state;

/* The main parsing function.  Parses the next value from buf, storing the
 * parsed value in val.  If val is of type UPB_TYPE_MESSAGE, then a
 * submessage was entered.
 *
 * IMPORTANT NOTE: for efficiency, the parsing routines do not do bounds checks,
 * and may read as much as far as buf+10.  So the caller must ensure that buf is
 * not within 10 bytes of unmapped memory, or the program will segfault. Clients
 * are encouraged to overallocate their buffers by ten bytes to compensate. */
upb_status_t upb_parse_field(struct upb_parse_state *s,
                             uint8_t *buf,
                             upb_field_number_t *fieldnum,
                             struct upb_tagged_value *val,
                             struct upb_tagged_wire_value *wv);

/* Low-level parsing functions. ***********************************************/

/* Parses a single tag from the character data starting at buf, and updates
 * buf to point one past the bytes that were consumed.  buf will be incremented
 * by at most ten bytes. */
upb_status_t parse_tag(uint8_t **buf, struct upb_tag *tag);

/* Parses a wire value with the given type (which must have been obtained from
 * a tag that was just parsed) and adds the number of bytes that were consumed
 * to *offset.  For delimited types, offset is advanced past the delimited
 * data.  */
upb_status_t upb_parse_wire_value(uint8_t *buf, size_t *offset,
                                  upb_wire_type_t wt,
                                  union upb_wire_value *wv);

/* Like the above, but discards the wire value instead of saving it. */
upb_status_t skip_wire_value(uint8_t *buf, size_t *offset,
                             upb_wire_type_t wt);

#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif  /* UPB_PARSE_H_ */
