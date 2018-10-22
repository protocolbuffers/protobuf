/*
** upb::json::Parser (upb_json_parser)
**
** Parses JSON according to a specific schema.
** Support for parsing arbitrary JSON (schema-less) will be added later.
*/

#ifndef UPB_JSON_PARSER_H_
#define UPB_JSON_PARSER_H_

#include "upb/sink.h"

#ifdef __cplusplus
namespace upb {
namespace json {
class Parser;
class ParserMethod;
}  /* namespace json */
}  /* namespace upb */
#endif

UPB_DECLARE_TYPE(upb::json::Parser, upb_json_parser)
UPB_DECLARE_DERIVED_TYPE(upb::json::ParserMethod, upb::RefCounted,
                         upb_json_parsermethod, upb_refcounted)

/* upb::json::Parser **********************************************************/

/* Preallocation hint: parser won't allocate more bytes than this when first
 * constructed.  This hint may be an overestimate for some build configurations.
 * But if the parser library is upgraded without recompiling the application,
 * it may be an underestimate. */
#define UPB_JSON_PARSER_SIZE 4672

#ifdef __cplusplus

/* Parses an incoming BytesStream, pushing the results to the destination
 * sink. */
class upb::json::Parser {
 public:
  static Parser* Create(Environment* env, const ParserMethod* method,
                        Sink* output, bool ignore_json_unknown);

  BytesSink* input();

 private:
  UPB_DISALLOW_POD_OPS(Parser, upb::json::Parser)
};

class upb::json::ParserMethod {
 public:
  /* Include base methods from upb::ReferenceCounted. */
  UPB_REFCOUNTED_CPPMETHODS

  /* Returns handlers for parsing according to the specified schema. */
  static reffed_ptr<const ParserMethod> New(const upb::MessageDef* md);

  /* The destination handlers that are statically bound to this method.
   * This method is only capable of outputting to a sink that uses these
   * handlers. */
  const Handlers* dest_handlers() const;

  /* The input handlers for this decoder method. */
  const BytesHandler* input_handler() const;

 private:
  UPB_DISALLOW_POD_OPS(ParserMethod, upb::json::ParserMethod)
};

#endif

UPB_BEGIN_EXTERN_C

upb_json_parser* upb_json_parser_create(upb_env* e,
                                        const upb_json_parsermethod* m,
                                        upb_sink* output,
                                        bool ignore_json_unknown);
upb_bytessink *upb_json_parser_input(upb_json_parser *p);

upb_json_parsermethod* upb_json_parsermethod_new(const upb_msgdef* md,
                                                 const void* owner);
const upb_handlers *upb_json_parsermethod_desthandlers(
    const upb_json_parsermethod *m);
const upb_byteshandler *upb_json_parsermethod_inputhandler(
    const upb_json_parsermethod *m);

/* Include refcounted methods like upb_json_parsermethod_ref(). */
UPB_REFCOUNTED_CMETHODS(upb_json_parsermethod, upb_json_parsermethod_upcast)

UPB_END_EXTERN_C

#ifdef __cplusplus

namespace upb {
namespace json {
inline Parser* Parser::Create(Environment* env, const ParserMethod* method,
                              Sink* output, bool ignore_json_unknown) {
  return upb_json_parser_create(env, method, output, ignore_json_unknown);
}
inline BytesSink* Parser::input() {
  return upb_json_parser_input(this);
}

inline const Handlers* ParserMethod::dest_handlers() const {
  return upb_json_parsermethod_desthandlers(this);
}
inline const BytesHandler* ParserMethod::input_handler() const {
  return upb_json_parsermethod_inputhandler(this);
}
/* static */
inline reffed_ptr<const ParserMethod> ParserMethod::New(
    const MessageDef* md) {
  const upb_json_parsermethod *m = upb_json_parsermethod_new(md, &m);
  return reffed_ptr<const ParserMethod>(m, &m);
}

}  /* namespace json */
}  /* namespace upb */

#endif


#endif  /* UPB_JSON_PARSER_H_ */
