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
class CodeCache;
class Parser;
class ParserMethod;
}  /* namespace json */
}  /* namespace upb */
#endif

UPB_DECLARE_TYPE(upb::json::Parser, upb_json_parser)
UPB_DECLARE_TYPE(upb::json::ParserMethod, upb_json_parsermethod)
UPB_DECLARE_TYPE(upb::json::CodeCache, upb_json_codecache)

/* upb::json::Parser **********************************************************/

/* Preallocation hint: parser won't allocate more bytes than this when first
 * constructed.  This hint may be an overestimate for some build configurations.
 * But if the parser library is upgraded without recompiling the application,
 * it may be an underestimate. */
#define UPB_JSON_PARSER_SIZE 5712

#ifdef __cplusplus

/* Parses an incoming BytesStream, pushing the results to the destination
 * sink. */
class upb::json::Parser {
 public:
  static Parser* Create(Environment* env, const ParserMethod* method,
                        const SymbolTable* symtab,
                        Sink* output, bool ignore_json_unknown);

  BytesSink* input();

 private:
  UPB_DISALLOW_POD_OPS(Parser, upb::json::Parser)
};

class upb::json::ParserMethod {
 public:
  /* The input handlers for this decoder method. */
  const BytesHandler* input_handler() const;

 private:
  UPB_DISALLOW_POD_OPS(ParserMethod, upb::json::ParserMethod)
};

class upb::json::CodeCache {
 public:
  static CodeCache* New();
  static void Free(CodeCache* cache);

  /* Returns a DecoderMethod that can push data to the given handlers.
   * If a suitable method already exists, it will be returned from the cache. */
  const ParserMethod *Get(const MessageDef* md);

 private:
  UPB_DISALLOW_POD_OPS(CodeCache, upb::json::CodeCache)
};

#endif

UPB_BEGIN_EXTERN_C

upb_json_parser* upb_json_parser_create(upb_env* e,
                                        const upb_json_parsermethod* m,
                                        const upb_symtab* symtab,
                                        upb_sink* output,
                                        bool ignore_json_unknown);
upb_bytessink *upb_json_parser_input(upb_json_parser *p);

const upb_byteshandler *upb_json_parsermethod_inputhandler(
    const upb_json_parsermethod *m);

upb_json_codecache *upb_json_codecache_new();
void upb_json_codecache_free(upb_json_codecache *cache);
upb_json_parsermethod* upb_json_codecache_get(upb_json_codecache* cache,
                                              const upb_msgdef* md);

UPB_END_EXTERN_C

#ifdef __cplusplus

namespace upb {
namespace json {
inline Parser* Parser::Create(Environment* env, const ParserMethod* method,
                              const SymbolTable* symtab,
                              Sink* output, bool ignore_json_unknown) {
  return upb_json_parser_create(
      env, method, symtab, output, ignore_json_unknown);
}
inline BytesSink* Parser::input() {
  return upb_json_parser_input(this);
}

inline const BytesHandler* ParserMethod::input_handler() const {
  return upb_json_parsermethod_inputhandler(this);
}
/* static */
inline const ParserMethod* CodeCache::Get(const MessageDef* md) {
  return upb_json_codecache_get(this, md);
}

}  /* namespace json */
}  /* namespace upb */

#endif

#endif  /* UPB_JSON_PARSER_H_ */
