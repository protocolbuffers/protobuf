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

/* upb_parser *****************************************************************/

// A upb_parser parses the binary protocol buffer format, writing the data it
// parses to a upb_sink.
struct upb_parser;
typedef struct upb_parser upb_parser;

// Allocates and frees a upb_parser, respectively.
upb_parser *upb_parser_new(struct upb_msgdef *md);
void upb_parser_free(upb_parser *p);

// Resets the internal state of an already-allocated parser.  This puts it in a
// state where it has not seen any data, and expects the next data to be from
// the beginning of a new protobuf.  Parsers must be reset before they can be
// used.  A parser can be reset multiple times.
void upb_parser_reset(upb_parser *p, upb_sink *sink);

// Parses protobuf data out of str, returning how much data was parsed.  The
// next call to upb_parser_parse should begin with the first byte that was
// not parsed.  "status" indicates whether an error occurred.
//
// TODO: provide the following guarantee:
//   retval will always be >= len. */
size_t upb_parser_parse(upb_parser *p, upb_strptr str,
                        struct upb_status *status);

#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif  /* UPB_PARSE_H_ */
