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

/* Callback parser callbacks. *************************************************/

// The value callback is called when a regular value (ie. not a string or
// submessage) is encountered which was defined in the upb_msgdef.  The client
// returns true to continue the parse or false to halt it.
//
// Note that this callback can be called several times in a row for a single
// call to tag_cb in the case of packed arrays.
typedef bool (*upb_value_cb)(void *udata, struct upb_fielddef *f,
                             union upb_value val);

// The string callback is called when a string that was defined in the
// upb_msgdef is parsed.  "str" is the protobuf data that is being parsed (NOT
// the string in question); "start" and "end" are the start and end offset of
// the string we parsed *within* str.  The data is supplied this way to give
// you the opportunity to reference this data instead of copying it (perhaps
// using upb_strslice), or to minimize copying if it is unavoidable.
//
// Note that if you are parsing in a streaming fashion, start could be <0 and
// "end" could be >upb_strlen(str).
typedef bool (*upb_str_cb)(void *udata, struct upb_fielddef *f, upb_strptr str,
                           int32_t start, uint32_t end);

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
size_t upb_cbparser_parse(struct upb_cbparser *p, upb_strptr str,
                          struct upb_status *status);

/* Pick parser interface. ************************************************/

// The pick parser provides a convenient interface for extracting a given set
// of fields from a protobuf.  This is especially useful in the case that you
// want only a few fields from a large protobuf, because the pick parser can be
// much more efficient by aggressively skipping data and stopping when it has
// all the fields you asked for.  The requested fields may be nested
// submessages of the top-level message.
//
// The selection parser currently does not yet support repeated fields -- this
// would involve either letting the user specify an index of the record they
// wanted, or repeatedly delivering values for the same field number.  The
// latter would make it impossible to bail out of processing a message early,
// because there could always be more values for that field.
//
// This parser is layered on top of the callback parser.

// Callbacks for the pick parser.  The semantics are the same as for the
// callback parser, excet that field numbers are provided instead of msgdefs
// and fieldefs.
typedef void (*upb_pp_value_cb)(void *udata, int fieldnum, union upb_value val);
typedef void (*upb_pp_str_cb)(void *udata, int fieldnum, uint8_t *str,
                              size_t avail_len, size_t total_len);

// The pickparser methods all have the same semantics as the cbparser, except
// that there are no start or end callbacks and the constructor needs a list
// of fields.  The fields are in dotted notation, so "foo.bar" expects that the
// top-level message contains a field foo, which contains a field bar.  The
// new function will return NULL if any of the field names are invalid, or are
// repeated fields.
struct upb_pickparser *upb_pickparser_new(struct upb_msgdef *msgdef,
                                          char *fields[]);
void upb_pickparser_free(struct upb_pickparser *p);
void upb_pickparser_reset(struct upb_pickparser *p,
                          bool found[], union upb_value vals[]);
size_t upb_pickparser_parse(struct upb_pickparser *p, upb_strptr str,
                            struct upb_status *status);

#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif  /* UPB_PARSE_H_ */
