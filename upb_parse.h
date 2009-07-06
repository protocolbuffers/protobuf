/*
 * upb - a minimalist implementation of protocol buffers.
 *
 * This file contains parsing routines; both stream-oriented and tree-oriented
 * models are supported.
 *
 * Copyright (c) 2009 Joshua Haberman.  See LICENSE for details.
 */

#ifndef UPB_PARSE_H_
#define UPB_PARSE_H_

#include <stdint.h>
#include <stdbool.h>
#include "upb.h"

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

/* High-level parsing interface. **********************************************/

struct upb_parse_state;

/* Initialize and free (respectively) the given parse state, which must have
 * been previously allocated.  udata_size specifies how much space will be
 * available at parse_stack_frame.user_data in each frame for user data. */
void upb_parse_init(struct upb_parse_state *state, size_t udata_size);
void upb_parse_free(struct upb_parse_state *state);

/* The callback that is called immediately after a tag has been parsed.  The
 * client should determine whether it wants to parse or skip the corresponding
 * value.  If it wants to parse it, it must discover and return the correct
 * .proto type (the tag only contains the wire type) and check that the wire
 * type is appropriate for the .proto type.  To skip the value (which means
 * skipping all submessages, in the case of a submessage), the callback should
 * return zero. */
typedef upb_field_type_t (*upb_tag_cb)(struct upb_parse_state *s,
                                       struct upb_tag *tag,
                                       void **user_field_desc);

/* The callback that is called when a regular value (ie. not a string or
 * submessage) is encountered which the client has opted to parse (by not
 * returning 0 from the tag_cb).  The client must parse the value and update
 * buf accordingly, returning success or failure.
 *
 * Note that this callback can be called several times in a row for a single
 * call to tag_cb in the case of packed arrays. */
typedef upb_status_t (*upb_value_cb)(struct upb_parse_state *s,
                                     void **buf, void *end,
                                     void *user_field_desc);

/* The callback that is called when a string is parsed. */
typedef upb_status_t (*upb_str_cb)(struct upb_parse_state *s,
                                   struct upb_string *str,
                                   void *user_field_desc);

/* Callbacks that are called when a submessage begins and ends, respectively.
 * Both are called with the submessage's stack frame at the top of the stack. */
typedef void (*upb_submsg_start_cb)(struct upb_parse_state *s,
                                    void *user_field_desc);
typedef void (*upb_submsg_end_cb)(struct upb_parse_state *s);

/* Each stack frame (one for each level of submessages/groups) has this format,
 * where user_data has as many bytes allocated as specified when initialized. */
struct upb_parse_stack_frame {
  size_t end_offset; /* 0 indicates that this is a group. */
  char user_data[];
};

struct upb_parse_state {
  size_t offset;
  struct upb_parse_stack_frame *stack, *top, *limit;
  size_t udata_size;  /* How many bytes the user gets in each frame. */
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
  return upb_type_info[ft].expected_wire_type == wt;
}

/* Data-consuming functions (to be called from value cb). *********************/

/* Parses and converts a value from the character data starting at buf.  The
 * caller must have previously checked that the wire type is appropriate for
 * this field type.  For delimited data, buf is advanced to the beginning of
 * the delimited data, not the end. */
upb_status_t upb_parse_value(void **buf, void *end, upb_field_type_t ft,
                             union upb_value_ptr v);

/* Parses a wire value with the given type (which must have been obtained from
 * a tag that was just parsed) and adds the number of bytes that were consumed
 * to *offset.  For delimited types, offset is advanced past the delimited
 * data.  */
upb_status_t upb_parse_wire_value(void **buf, void *end, upb_wire_type_t wt,
                                  union upb_wire_value *wv);

#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif  /* UPB_PARSE_H_ */
