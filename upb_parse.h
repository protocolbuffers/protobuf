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

/* Low-level parsing functions. ***********************************************/

/* Parses a single tag from the character data starting at buf, and updates
 * buf to point one past the bytes that were consumed.  buf will be incremented
 * by at most ten bytes. */
upb_status_t parse_tag(uint8_t **buf, struct upb_tag *tag);

extern upb_wire_type_t upb_expected_wire_types[];
/* Returns true if wt is the correct on-the-wire type for ft. */
INLINE bool upb_check_type(upb_wire_type_t wt, upb_field_type_t ft) {
  return upb_expected_wire_types[ft] == wt;
}

/* Parses and converts a value from the character data starting at buf.  The
 * caller must have previously checked that the wire type is appropriate for
 * this field type.  For delimited data, buf is advanced to the beginning of
 * the delimited data, not the end. */
upb_status_t upb_parse_value(uint8_t **buf, upb_field_type_t ft,
                             union upb_value *value);

/* Parses a wire value with the given type (which must have been obtained from
 * a tag that was just parsed) and adds the number of bytes that were consumed
 * to *offset.  For delimited types, offset is advanced past the delimited
 * data.  */
upb_status_t upb_parse_wire_value(uint8_t *buf, size_t *offset,
                                  upb_wire_type_t wt,
                                  union upb_wire_value *wv);

/* Like the above, but discards the wire value instead of saving it. */
upb_status_t upb_skip_wire_value(uint8_t *buf, size_t *offset,
                                 upb_wire_type_t wt);

#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif  /* UPB_PARSE_H_ */
