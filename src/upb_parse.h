/*
 * upb - a minimalist implementation of protocol buffers.
 *
 * upb_parse implements a high performance, callback-based, stream-oriented
 * parser (comparable to the SAX model in XML parsers).  For parsing protobufs
 * into in-memory messages (a more DOM-like model), see the routines in
 * upb_msg.h, which are layered on top of this parser.
 *
 * Copyright (c) 2009 Joshua Haberman.  See LICENSE for details.
 */

#ifndef UPB_PARSE_H_
#define UPB_PARSE_H_

#include <stdbool.h>
#include <stdint.h>
#include "upb.h"
#include "descriptor.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Event Callbacks. ***********************************************************/

// The tag callback is called immediately after a tag has been parsed.  The
// client should determine whether it wants to parse or skip the corresponding
// value.  If it wants to parse it, it must discover and return the correct
// .proto type (the tag only contains the wire type) and check that the wire
// type is appropriate for the .proto type.  Returning a type for which
// upb_check_type(tag->wire_type, type) == false invokes undefined behavior.
//
// To skip the value (which means skipping all submessages, in the case of a
// submessage), the callback should return zero.
//
// The client can store a void* in *user_field_desc; this will be passed to
// the value callback or the string callback.
typedef upb_field_type_t (*upb_tag_cb)(void *udata, struct upb_tag *tag,
                                       void **user_field_desc);

// The value callback is called when a regular value (ie. not a string or
// submessage) is encountered which the client has opted to parse (by not
// returning 0 from the tag_cb).  The client must parse the value by calling
// upb_parse_value(), returning success or failure accordingly.
//
// Note that this callback can be called several times in a row for a single
// call to tag_cb in the case of packed arrays.
typedef void *(*upb_value_cb)(void *udata, uint8_t *buf, uint8_t *end,
                              void *user_field_desc, struct upb_status *status);

// The string callback is called when a string is parsed.  avail_len is the
// number of bytes that are currently available at str.  If the client is
// streaming and the current buffer ends in the middle of the string, this
// number could be less than total_len.
typedef void (*upb_str_cb)(void *udata, uint8_t *str, size_t avail_len,
                           size_t total_len, void *user_field_desc);

// The start and end callbacks are called when a submessage begins and ends,
// respectively.
typedef void (*upb_start_cb)(void *udata, void *user_field_desc);
typedef void (*upb_end_cb)(void *udata);

/* Callback parser interface. *************************************************/

// Allocates and frees a upb_cbparser, respectively.
struct upb_cbparser *upb_cbparser_new(void);
void upb_cbparser_free(struct upb_cbparser *p);

// Resets the internal state of an already-allocated parser.  Parsers must be
// reset before they can be used.  A parser can be reset multiple times.  udata
// will be passed as the first argument to callbacks.
//
// tagcb must be set, but all other callbacks can be NULL, in which case they
// will just be skipped.
void upb_cbparser_reset(struct upb_cbparser *p, void *udata,
                        upb_tag_cb tagcb,
                        upb_value_cb valuecb,
                        upb_str_cb strcb,
                        upb_start_cb startcb,
                        upb_end_cb endcb);


// Parses up to len bytes of protobuf data out of buf, calling the appropriate
// callbacks as values are parsed.
//
// The function returns a status indicating the success of the operation.  Data
// is parsed until no more data can be read from buf, or the callback returns an
// error like UPB_STATUS_USER_CANCELLED, or an error occurs.
//
// *read is set to the number of bytes consumed.  Note that this can be greater
// than len in the case that a string was recognized that spans beyond the end
// of the currently provided data.
//
// The next call to upb_parse must be the first byte after buf + *read, even in
// the case that *read > len.
//
// TODO: see if we can provide the following guarantee efficiently:
//   *read will always be >= len. */
size_t upb_cbparser_parse(struct upb_cbparser *p, void *buf, size_t len,
                          struct upb_status *status);

extern upb_wire_type_t upb_expected_wire_types[];
// Returns true if wt is the correct on-the-wire type for ft.
INLINE bool upb_check_type(upb_wire_type_t wt, upb_field_type_t ft) {
  // This doesn't currently support packed arrays.
  return upb_type_info[ft].expected_wire_type == wt;
}

/* Data-consuming functions (to be called from value cb). *********************/

// Parses and converts a value from the character data starting at buf (but not
// past end).  Returns a pointer that is one past the data that was read.  The
// caller must have previously checked that the wire type is appropriate for
// this field type.
uint8_t *upb_parse_value(uint8_t *buf, uint8_t *end, upb_field_type_t ft,
                         union upb_value_ptr v, struct upb_status *status);

// Parses a wire value with the given type (which must have been obtained from
// a tag that was just parsed) and returns a pointer to one past the data that
// was read.
uint8_t *upb_parse_wire_value(uint8_t *buf, uint8_t *end, upb_wire_type_t wt,
                              union upb_wire_value *wv,
                              struct upb_status *status);

#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif  /* UPB_PARSE_H_ */
