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

#include "upb/env.h"
#include "upb/sink.h"

#ifdef __cplusplus
namespace upb {
namespace json {
class Parser;
}  /* namespace json */
}  /* namespace upb */
#endif

UPB_DECLARE_TYPE(upb::json::Parser, upb_json_parser)

/* upb::json::Parser **********************************************************/

/* Preallocation hint: parser won't allocate more bytes than this when first
 * constructed.  This hint may be an overestimate for some build configurations.
 * But if the parser library is upgraded without recompiling the application,
 * it may be an underestimate. */
#define UPB_JSON_PARSER_SIZE 3568

#ifdef __cplusplus

/* Parses an incoming BytesStream, pushing the results to the destination
 * sink. */
class upb::json::Parser {
 public:
  static Parser* Create(Environment* env, Sink* output);

  BytesSink* input();

 private:
  UPB_DISALLOW_POD_OPS(Parser, upb::json::Parser)
};

#endif

UPB_BEGIN_EXTERN_C

upb_json_parser *upb_json_parser_create(upb_env *e, upb_sink *output);
upb_bytessink *upb_json_parser_input(upb_json_parser *p);

UPB_END_EXTERN_C

#ifdef __cplusplus

namespace upb {
namespace json {
inline Parser* Parser::Create(Environment* env, Sink* output) {
  return upb_json_parser_create(env, output);
}
inline BytesSink* Parser::input() {
  return upb_json_parser_input(this);
}
}  /* namespace json */
}  /* namespace upb */

#endif


#endif  /* UPB_JSON_PARSER_H_ */
