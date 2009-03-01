/*
 * pbstream - a stream-oriented implementation of protocol buffers.
 *
 * Copyright (c) 2008 Joshua Haberman.  See LICENSE for details.
 *
 * The structures and functions in this file offer more control than what is
 * offered in pbstream.h.  These can be used for more specialized/optimized
 * parsing applications. */

#ifndef PBSTREAM_LOWLEVEL_H_
#define PBSTREAM_LOWLEVEL_H_

#include "pbstream.h"

#ifdef __cplusplus
extern "C" {
#endif

/* A tag occurs before each value on-the-wire. */
struct pbstream_tag {
  pbstream_field_number_t field_number;
  pbstream_wire_type_t wire_type;
};

/* Parses a single tag from the character data starting at buf, and updates
 * buf to point one past the bytes that were consumed. */
pbstream_status_t parse_tag(uint8_t **buf, struct pbstream_tag *tag);

/* Parses a wire value with the given type (which must have been obtained from
 * a tag that was just parsed) and updates buf to point to one past the bytes
 * that were consumed. */
pbstream_status_t parse_wire_value(uint8_t **buf, size_t offset,
                                   pbstream_wire_type_t wt,
                                   union pbstream_wire_value *wv);

/* Like the above, but discards the wire value instead of saving it. */
pbstream_status_t skip_wire_value(uint8_t **buf, pbstream_wire_type_t wt);

/* Looks the given field number up in the fieldset, and returns the
 * corresponding pbstream_field definition (or NULL if this field number
 * does not exist in this fieldset). */
struct pbstream_field *pbstream_find_field(struct pbstream_fieldset *fs,
                                           pbstream_field_number_t num);

#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif  /* PBSTREAM_LOWLEVEL_H_ */
