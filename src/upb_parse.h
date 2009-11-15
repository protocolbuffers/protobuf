/*
 * upb - a minimalist implementation of protocol buffers.
 *
 * upb_parse implements a high performance, callback-based, stream-oriented
 * parser (comparable to the SAX model in XML parsers).  For parsing protobufs
 * into in-memory messages (a more DOM-like model), see the routines in
 * upb_msg.h, which are layered on top of this parser.
 *
 * TODO: the parser currently does not support returning unknown values.  This
 * can easily be added when it is needed.
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

// The value callback is called when a regular value (ie. not a string or
// submessage) is encountered which was defined in the upb_msgdef.  The client
// returns true to continue the parse or false to halt it.
//
// Note that this callback can be called several times in a row for a single
// call to tag_cb in the case of packed arrays.
typedef bool (*upb_value_cb)(void *udata, struct upb_msgdef *msgdef,
                             struct upb_fielddef *f, union upb_value val);

// The string callback is called when a string that was defined in the
// upb_msgdef is parsed.  avail_len is the number of bytes that are currently
// available at str.  If the client is streaming and the current buffer ends in
// the middle of the string, this number could be less than total_len.
typedef bool (*upb_str_cb)(void *udata, struct upb_msgdef *msgdef,
                           struct upb_fielddef *f, uint8_t *str,
                           size_t avail_len, size_t total_len);

// The start and end callbacks are called when a submessage begins and ends,
// respectively.
typedef void (*upb_start_cb)(void *udata, struct upb_fielddef *f);
typedef void (*upb_end_cb)(void *udata);

/* Callback parser interface. *************************************************/

// Allocates and frees a upb_cbparser, respectively.  Callbacks may be NULL,
// in which case they will be skipped.
struct upb_cbparser *upb_cbparser_new(struct upb_msgdef *md,
                                      upb_value_cb valuecb, upb_str_cb strcb,
                                      upb_start_cb startcb, upb_end_cb endcb);
void upb_cbparser_free(struct upb_cbparser *p);

// Resets the internal state of an already-allocated parser.  This puts it in a
// state where it has not seen any data, and expects the next data to be from
// the beginning of a new protobuf.  Parsers must be reset before they can be
// used.  A parser can be reset multiple times.  udata will be passed as the
// first argument to callbacks.
void upb_cbparser_reset(struct upb_cbparser *p, void *udata);

// Parses up to len bytes of protobuf data out of buf, calling the appropriate
// callbacks as values are parsed.
//
// The function returns a status indicating the success of the operation.  Data
// is parsed until no more data can be read from buf, or a user callback
// returns false, or an error occurs.
//
// The function returns the number of bytes consumed.  Note that this can be
// greater than len in the case that a string was recognized that spans beyond
// the end of the currently provided data.
//
// The next call to upb_parse must be the first byte after buf + retval, even in
// the case that retval > len.
//
// TODO: see if we can provide the following guarantee efficiently:
//   retval will always be >= len. */
size_t upb_cbparser_parse(struct upb_cbparser *p, void *buf, size_t len,
                          struct upb_status *status);

#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif  /* UPB_PARSE_H_ */
