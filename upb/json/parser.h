/*
 * upb - a minimalist implementation of protocol buffers.
 *
 * Copyright (c) 2014 Google Inc.  See LICENSE for details.
 * Author: Josh Haberman <jhaberman@gmail.com>
 *
 * upb::json::Parser can parse JSON according to a specific schema.
 * Support for parsing arbitrary JSON (schema-less) will be added later.
 */

#ifndef UPB_JSON_PARSER_H_
#define UPB_JSON_PARSER_H_

#include "upb/sink.h"

#ifdef __cplusplus
namespace upb {
namespace json {
class Parser;
}  // namespace json
}  // namespace upb
#endif

UPB_DECLARE_TYPE(upb::json::Parser, upb_json_parser);

// Internal-only struct used by the parser.
typedef struct {
 UPB_PRIVATE_FOR_CPP
  upb_sink sink;
  const upb_msgdef *m;
  const upb_fielddef *f;
} upb_jsonparser_frame;


/* upb::json::Parser **********************************************************/

#define UPB_JSON_MAX_DEPTH 64

// Parses an incoming BytesStream, pushing the results to the destination sink.
UPB_DEFINE_CLASS0(upb::json::Parser,
 public:
  Parser(Status* status);
  ~Parser();

  // Resets the state of the printer, so that it will expect to begin a new
  // document.
  void Reset();

  // Resets the output pointer which will serve as our closure.  Implies
  // Reset().
  void ResetOutput(Sink* output);

  // The input to the printer.
  BytesSink* input();
,
UPB_DEFINE_STRUCT0(upb_json_parser,
  upb_byteshandler input_handler_;
  upb_bytessink input_;

  // Stack to track the JSON scopes we are in.
  upb_jsonparser_frame stack[UPB_JSON_MAX_DEPTH];
  upb_jsonparser_frame *top;
  upb_jsonparser_frame *limit;

  upb_status *status;

  // Ragel's internal parsing stack for the parsing state machine.
  int current_state;
  int parser_stack[UPB_JSON_MAX_DEPTH];
  int parser_top;

  // A pointer to the beginning of whatever text we are currently parsing.
  const char *text_begin;

  // We have to accumulate text for member names, integers, unicode escapes, and
  // base64 partial results.
  const char *accumulated;
  size_t accumulated_len;
  // TODO: add members and code for allocating a buffer when necessary (when the
  // member spans input buffers or contains escapes).
));

UPB_BEGIN_EXTERN_C

void upb_json_parser_init(upb_json_parser *p, upb_status *status);
void upb_json_parser_uninit(upb_json_parser *p);
void upb_json_parser_reset(upb_json_parser *p);
void upb_json_parser_resetoutput(upb_json_parser *p, upb_sink *output);
upb_bytessink *upb_json_parser_input(upb_json_parser *p);

UPB_END_EXTERN_C

#ifdef __cplusplus

namespace upb {
namespace json {
inline Parser::Parser(Status* status) { upb_json_parser_init(this, status); }
inline Parser::~Parser() { upb_json_parser_uninit(this); }
inline void Parser::Reset() { upb_json_parser_reset(this); }
inline void Parser::ResetOutput(Sink* output) {
  upb_json_parser_resetoutput(this, output);
}
inline BytesSink* Parser::input() {
  return upb_json_parser_input(this);
}
}  // namespace json
}  // namespace upb

#endif


#endif  // UPB_JSON_PARSER_H_
