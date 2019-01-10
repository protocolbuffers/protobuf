/*
** upb::pb::Decoder
**
** A high performance, streaming, resumable decoder for the binary protobuf
** format.
**
** This interface works the same regardless of what decoder backend is being
** used.  A client of this class does not need to know whether decoding is using
** a JITted decoder (DynASM, LLVM, etc) or an interpreted decoder.  By default,
** it will always use the fastest available decoder.  However, you can call
** set_allow_jit(false) to disable any JIT decoder that might be available.
** This is primarily useful for testing purposes.
*/

#ifndef UPB_DECODER_H_
#define UPB_DECODER_H_

#include "upb/sink.h"

#ifdef __cplusplus
namespace upb {
namespace pb {
class CodeCache;
class Decoder;
class DecoderMethod;
class DecoderMethodOptions;
}  /* namespace pb */
}  /* namespace upb */
#endif

UPB_DECLARE_TYPE(upb::pb::CodeCache, upb_pbcodecache)
UPB_DECLARE_TYPE(upb::pb::Decoder, upb_pbdecoder)
UPB_DECLARE_TYPE(upb::pb::DecoderMethod, upb_pbdecodermethod)

/* The maximum number of bytes we are required to buffer internally between
 * calls to the decoder.  The value is 14: a 5 byte unknown tag plus ten-byte
 * varint, less one because we are buffering an incomplete value.
 *
 * Should only be used by unit tests. */
#define UPB_DECODER_MAX_RESIDUAL_BYTES 14

#ifdef __cplusplus

/* Represents the code to parse a protobuf according to a destination
 * Handlers. */
class upb::pb::DecoderMethod {
 public:
  /* The destination handlers that are statically bound to this method.
   * This method is only capable of outputting to a sink that uses these
   * handlers. */
  const Handlers* dest_handlers() const;

  /* The input handlers for this decoder method. */
  const BytesHandler* input_handler() const;

  /* Whether this method is native. */
  bool is_native() const;

 private:
  UPB_DISALLOW_POD_OPS(DecoderMethod, upb::pb::DecoderMethod)
};

#endif

/* Preallocation hint: decoder won't allocate more bytes than this when first
 * constructed.  This hint may be an overestimate for some build configurations.
 * But if the decoder library is upgraded without recompiling the application,
 * it may be an underestimate. */
#define UPB_PB_DECODER_SIZE 4416

#ifdef __cplusplus

/* A Decoder receives binary protobuf data on its input sink and pushes the
 * decoded data to its output sink. */
class upb::pb::Decoder {
 public:
  /* Constructs a decoder instance for the given method, which must outlive this
   * decoder.  Any errors during parsing will be set on the given status, which
   * must also outlive this decoder.
   *
   * The sink must match the given method. */
  static Decoder* Create(Environment* env, const DecoderMethod* method,
                         Sink* output);

  /* Returns the DecoderMethod this decoder is parsing from. */
  const DecoderMethod* method() const;

  /* The sink on which this decoder receives input. */
  BytesSink* input();

  /* Returns number of bytes successfully parsed.
   *
   * This can be useful for determining the stream position where an error
   * occurred.
   *
   * This value may not be up-to-date when called from inside a parsing
   * callback. */
  uint64_t BytesParsed() const;

  /* Gets/sets the parsing nexting limit.  If the total number of nested
   * submessages and repeated fields hits this limit, parsing will fail.  This
   * is a resource limit that controls the amount of memory used by the parsing
   * stack.
   *
   * Setting the limit will fail if the parser is currently suspended at a depth
   * greater than this, or if memory allocation of the stack fails. */
  size_t max_nesting() const;
  bool set_max_nesting(size_t max);

  void Reset();

  static const size_t kSize = UPB_PB_DECODER_SIZE;

 private:
  UPB_DISALLOW_POD_OPS(Decoder, upb::pb::Decoder)
};

/* A class for caching protobuf processing code, whether bytecode for the
 * interpreted decoder or machine code for the JIT.
 *
 * This class is not thread-safe. */
class upb::pb::CodeCache {
 public:
  static CodeCache* New(HandlerCache* dest);
  static void Free(CodeCache* cache);

  /* Whether the cache is allowed to generate machine code.  Defaults to true.
   * There is no real reason to turn it off except for testing or if you are
   * having a specific problem with the JIT.
   *
   * Note that allow_jit = true does not *guarantee* that the code will be JIT
   * compiled.  If this platform is not supported or the JIT was not compiled
   * in, the code may still be interpreted. */
  bool allow_jit() const;

  /* This may only be called when the object is first constructed, and prior to
   * any code generation. */
  void set_allow_jit(bool allow);

  /* Should the decoder push submessages to lazy handlers for fields that have
   * them?  The caller should set this iff the lazy handlers expect data that is
   * in protobuf binary format and the caller wishes to lazy parse it. */
  void set_lazy(bool lazy);

  /* Returns a DecoderMethod that can push data to the given handlers.
   * If a suitable method already exists, it will be returned from the cache. */
  const DecoderMethod *Get(const MessageDef* md);

 private:
  UPB_DISALLOW_POD_OPS(CodeCache, upb::pb::CodeCache)
};

#endif

UPB_BEGIN_EXTERN_C

upb_pbdecoder *upb_pbdecoder_create(upb_env *e,
                                    const upb_pbdecodermethod *method,
                                    upb_sink *output);
const upb_pbdecodermethod *upb_pbdecoder_method(const upb_pbdecoder *d);
upb_bytessink *upb_pbdecoder_input(upb_pbdecoder *d);
uint64_t upb_pbdecoder_bytesparsed(const upb_pbdecoder *d);
size_t upb_pbdecoder_maxnesting(const upb_pbdecoder *d);
bool upb_pbdecoder_setmaxnesting(upb_pbdecoder *d, size_t max);
void upb_pbdecoder_reset(upb_pbdecoder *d);



const upb_handlers *upb_pbdecodermethod_desthandlers(
    const upb_pbdecodermethod *m);
const upb_byteshandler *upb_pbdecodermethod_inputhandler(
    const upb_pbdecodermethod *m);
bool upb_pbdecodermethod_isnative(const upb_pbdecodermethod *m);

upb_pbcodecache *upb_pbcodecache_new(upb_handlercache *dest);
void upb_pbcodecache_free(upb_pbcodecache *c);
bool upb_pbcodecache_allowjit(const upb_pbcodecache *c);
void upb_pbcodecache_setallowjit(upb_pbcodecache *c, bool allow);
void upb_pbcodecache_setlazy(upb_pbcodecache *c, bool lazy);
const upb_pbdecodermethod *upb_pbcodecache_get(upb_pbcodecache *c,
                                               const upb_msgdef *md);

UPB_END_EXTERN_C

#ifdef __cplusplus

namespace upb {

namespace pb {

/* static */
inline Decoder* Decoder::Create(Environment* env, const DecoderMethod* m,
                                Sink* sink) {
  return upb_pbdecoder_create(env, m, sink);
}
inline const DecoderMethod* Decoder::method() const {
  return upb_pbdecoder_method(this);
}
inline BytesSink* Decoder::input() {
  return upb_pbdecoder_input(this);
}
inline uint64_t Decoder::BytesParsed() const {
  return upb_pbdecoder_bytesparsed(this);
}
inline size_t Decoder::max_nesting() const {
  return upb_pbdecoder_maxnesting(this);
}
inline bool Decoder::set_max_nesting(size_t max) {
  return upb_pbdecoder_setmaxnesting(this, max);
}
inline void Decoder::Reset() { upb_pbdecoder_reset(this); }

inline const Handlers* DecoderMethod::dest_handlers() const {
  return upb_pbdecodermethod_desthandlers(this);
}
inline const BytesHandler* DecoderMethod::input_handler() const {
  return upb_pbdecodermethod_inputhandler(this);
}
inline bool DecoderMethod::is_native() const {
  return upb_pbdecodermethod_isnative(this);
}

inline CodeCache* CodeCache::New(HandlerCache* dest) {
  return upb_pbcodecache_new(dest);
}
inline void CodeCache::Free(CodeCache* cache) {
  upb_pbcodecache_free(cache);
}
inline bool CodeCache::allow_jit() const {
  return upb_pbcodecache_allowjit(this);
}
inline void CodeCache::set_allow_jit(bool allow) {
  upb_pbcodecache_setallowjit(this, allow);
}
inline const DecoderMethod *CodeCache::Get(const MessageDef *md) {
  return upb_pbcodecache_get(this, md);
}

}  /* namespace pb */
}  /* namespace upb */

#endif  /* __cplusplus */

#endif  /* UPB_DECODER_H_ */
