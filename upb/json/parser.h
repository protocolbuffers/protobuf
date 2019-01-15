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
class ParserPtr;
class ParserMethodPtr;
}  /* namespace json */
}  /* namespace upb */
#endif

/* upb_json_parsermethod ******************************************************/

struct upb_json_parsermethod;
typedef struct upb_json_parsermethod upb_json_parsermethod;

#ifdef __cplusplus
extern "C" {
#endif

const upb_byteshandler* upb_json_parsermethod_inputhandler(
    const upb_json_parsermethod* m);

#ifdef __cplusplus
}  /* extern "C" */

class upb::json::ParserMethodPtr {
 public:
  ParserMethodPtr() : ptr_(nullptr) {}
  ParserMethodPtr(const upb_json_parsermethod* ptr) : ptr_(ptr) {}

  const upb_json_parsermethod* ptr() const { return ptr_; }

  const BytesHandler* input_handler() const {
    return upb_json_parsermethod_inputhandler(ptr());
  }

 private:
  const upb_json_parsermethod* ptr_;
};

#endif  /* __cplusplus */

/* upb_json_parser ************************************************************/

/* Preallocation hint: parser won't allocate more bytes than this when first
 * constructed.  This hint may be an overestimate for some build configurations.
 * But if the parser library is upgraded without recompiling the application,
 * it may be an underestimate. */
#define UPB_JSON_PARSER_SIZE 5712

struct upb_json_parser;
typedef struct upb_json_parser upb_json_parser;

#ifdef __cplusplus
extern "C" {
#endif

upb_json_parser* upb_json_parser_create(upb_arena* a,
                                        const upb_json_parsermethod* m,
                                        const upb_symtab* symtab,
                                        upb_sink output,
                                        upb_status *status,
                                        bool ignore_json_unknown);
upb_bytessink upb_json_parser_input(upb_json_parser* p);

#ifdef __cplusplus
}  /* extern "C" */

/* Parses an incoming BytesStream, pushing the results to the destination
 * sink. */
class upb::json::ParserPtr {
 public:
  ParserPtr(upb_json_parser* ptr) : ptr_(ptr) {}

  static ParserPtr Create(Arena* arena, ParserMethodPtr method,
                          SymbolTable* symtab, Sink output, Status* status,
                          bool ignore_json_unknown) {
    upb_symtab* symtab_ptr = symtab ? symtab->ptr() : nullptr;
    return ParserPtr(upb_json_parser_create(
        arena->ptr(), method.ptr(), symtab_ptr, output.sink(), status->ptr(),
        ignore_json_unknown));
  }

  BytesSink input() { return upb_json_parser_input(ptr_); }

 private:
  upb_json_parser* ptr_;
};

#endif  /* __cplusplus */

/* upb_json_codecache *********************************************************/

/* Lazily builds and caches decoder methods that will push data to the given
 * handlers.  The upb_symtab object(s) must outlive this object. */

struct upb_json_codecache;
typedef struct upb_json_codecache upb_json_codecache;

#ifdef __cplusplus
extern "C" {
#endif

upb_json_codecache *upb_json_codecache_new();
void upb_json_codecache_free(upb_json_codecache *cache);
const upb_json_parsermethod* upb_json_codecache_get(upb_json_codecache* cache,
                                                    const upb_msgdef* md);

#ifdef __cplusplus
}  /* extern "C" */

class upb::json::CodeCache {
 public:
  CodeCache() : ptr_(upb_json_codecache_new(), upb_json_codecache_free) {}

  /* Returns a DecoderMethod that can push data to the given handlers.
   * If a suitable method already exists, it will be returned from the cache. */
  ParserMethodPtr Get(MessageDefPtr md) {
    return upb_json_codecache_get(ptr_.get(), md.ptr());
  }

 private:
  std::unique_ptr<upb_json_codecache, decltype(&upb_json_codecache_free)> ptr_;
};

#endif

#endif  /* UPB_JSON_PARSER_H_ */
