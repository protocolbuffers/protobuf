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

/* High-level parsing interface. **********************************************/

struct upb_parse_state;

/* Initialize and free (respectively) the given parse state, which must have
 * been previously allocated.  udata_size specifies how much space will be
 * available at parse_stack_frame.user_data in each frame for user data. */
void upb_parse_state_init(struct upb_parse_state *state, size_t udata_size);
void upb_parse_state_free(struct upb_parse_state *state);

/* The callback that is called immediately after a tag has been parsed.  The
 * client uses it to decide if it wants to process this value or skip it.  If
 * it wants to process it, it must determine its specific .proto type (at this
 * point we only know its wire type) and verify that it matches the wire type.
 * The client will then return the .proto type.  To skip the value, the client
 * should return 0 (which is not a valid .proto type).
 *
 * The client can set user_field_desc to a record describing this field -- this
 * pointer will be supplied to the value callback (for simple values) or the
 * submsg_start callback (for submessages).
 *
 * TODO: there needs to be a way to skip a delimited field while still knowing
 * its offset and length.  That could be through this callback or it could be a
 * separate callback. */
typedef upb_field_type_t (*upb_tag_cb)(struct upb_parse_state *s,
                                       struct upb_tag *tag,
                                       void **user_field_desc);

/* The callback that is called for individual values.  This callback is only
 * called when the previously invoked tag_cb has returned nonzero.  It receives
 * the parsed and converted value as well as the user_field_desc that was set
 * by the tag_cb.  Note that this function can be called several times in a row
 * (ie. with no intervening tag_cb) in the case of packed arrays.  For string
 * data (bytes and string) str points to the beginning of the string. */
typedef void (*upb_value_cb)(struct upb_parse_state *s, union upb_value *v,
                             void *str, void *user_field_desc);

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
  bool done;  /* Any callback can abort processing by setting done=true. */
  /* These are only set if we're in the middle of a packed array. */
  size_t packed_end_offset;  /* 0 if not in a packed array. */
  upb_field_type_t packed_type;
  upb_tag_cb tag_cb;
  upb_value_cb value_cb;
  upb_submsg_start_cb submsg_start_cb;
  upb_submsg_end_cb submsg_end_cb;
};

/* Parses up to len bytes of protobuf data out of buf, calling cb as needed.
 * The function returns how many bytes were consumed from buf.  Data is parsed
 * until no more data can be read from buf, or the callback sets *done=true,
 * or an error occured.  Sets *read to the number of bytes consumed. */
upb_status_t upb_parse(struct upb_parse_state *s, void *buf, size_t len,
                       size_t *read);

/* Low-level parsing functions. ***********************************************/

/* Parses a single tag from the character data starting at buf, and updates
 * buf to point one past the bytes that were consumed.  buf will be incremented
 * by at most ten bytes. */
upb_status_t upb_parse_tag(void **buf, struct upb_tag *tag);

extern upb_wire_type_t upb_expected_wire_types[];
/* Returns true if wt is the correct on-the-wire type for ft. */
INLINE bool upb_check_type(upb_wire_type_t wt, upb_field_type_t ft) {
  return upb_expected_wire_types[ft] == wt;
}

/* Parses and converts a value from the character data starting at buf.  The
 * caller must have previously checked that the wire type is appropriate for
 * this field type.  For delimited data, buf is advanced to the beginning of
 * the delimited data, not the end. */
upb_status_t upb_parse_value(void **buf, upb_field_type_t ft,
                             union upb_value *value);

/* Parses a wire value with the given type (which must have been obtained from
 * a tag that was just parsed) and adds the number of bytes that were consumed
 * to *offset.  For delimited types, offset is advanced past the delimited
 * data.  */
upb_status_t upb_parse_wire_value(void *buf, size_t *offset,
                                  upb_wire_type_t wt,
                                  union upb_wire_value *wv);

/* Like the above, but discards the wire value instead of saving it. */
upb_status_t upb_skip_wire_value(void *buf, size_t *offset,
                                 upb_wire_type_t wt);

#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif  /* UPB_PARSE_H_ */
