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
UPB_DECLARE_TYPE(upb::pb::DecoderMethodOptions, upb_pbdecodermethodopts)

UPB_DECLARE_DERIVED_TYPE(upb::pb::DecoderMethod, upb::RefCounted,
                         upb_pbdecodermethod, upb_refcounted)

/* The maximum number of bytes we are required to buffer internally between
 * calls to the decoder.  The value is 14: a 5 byte unknown tag plus ten-byte
 * varint, less one because we are buffering an incomplete value.
 *
 * Should only be used by unit tests. */
#define UPB_DECODER_MAX_RESIDUAL_BYTES 14

#ifdef __cplusplus

/* The parameters one uses to construct a DecoderMethod.
 * TODO(haberman): move allowjit here?  Seems more convenient for users.
 * TODO(haberman): move this to be heap allocated for ABI stability. */
class upb::pb::DecoderMethodOptions {
 public:
  /* Parameter represents the destination handlers that this method will push
   * to. */
  explicit DecoderMethodOptions(const Handlers* dest_handlers);

  /* Should the decoder push submessages to lazy handlers for fields that have
   * them?  The caller should set this iff the lazy handlers expect data that is
   * in protobuf binary format and the caller wishes to lazy parse it. */
  void set_lazy(bool lazy);
#else
struct upb_pbdecodermethodopts {
#endif
  const upb_handlers *handlers;
  bool lazy;
};

#ifdef __cplusplus

/* Represents the code to parse a protobuf according to a destination
 * Handlers. */
class upb::pb::DecoderMethod {
 public:
  /* Include base methods from upb::ReferenceCounted. */
  UPB_REFCOUNTED_CPPMETHODS

  /* The destination handlers that are statically bound to this method.
   * This method is only capable of outputting to a sink that uses these
   * handlers. */
  const Handlers* dest_handlers() const;

  /* The input handlers for this decoder method. */
  const BytesHandler* input_handler() const;

  /* Whether this method is native. */
  bool is_native() const;

  /* Convenience method for generating a DecoderMethod without explicitly
   * creating a CodeCache. */
  static reffed_ptr<const DecoderMethod> New(const DecoderMethodOptions& opts);

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

#endif  /* __cplusplus */

#ifdef __cplusplus

/* A class for caching protobuf processing code, whether bytecode for the
 * interpreted decoder or machine code for the JIT.
 *
 * This class is not thread-safe.
 *
 * TODO(haberman): move this to be heap allocated for ABI stability. */
class upb::pb::CodeCache {
 public:
  CodeCache();
  ~CodeCache();

  /* Whether the cache is allowed to generate machine code.  Defaults to true.
   * There is no real reason to turn it off except for testing or if you are
   * having a specific problem with the JIT.
   *
   * Note that allow_jit = true does not *guarantee* that the code will be JIT
   * compiled.  If this platform is not supported or the JIT was not compiled
   * in, the code may still be interpreted. */
  bool allow_jit() const;

  /* This may only be called when the object is first constructed, and prior to
   * any code generation, otherwise returns false and does nothing. */
  bool set_allow_jit(bool allow);

  /* Returns a DecoderMethod that can push data to the given handlers.
   * If a suitable method already exists, it will be returned from the cache.
   *
   * Specifying the destination handlers here allows the DecoderMethod to be
   * statically bound to the destination handlers if possible, which can allow
   * more efficient decoding.  However the returned method may or may not
   * actually be statically bound.  But in all cases, the returned method can
   * push data to the given handlers. */
  const DecoderMethod *GetDecoderMethod(const DecoderMethodOptions& opts);

  /* If/when someone needs to explicitly create a dynamically-bound
   * DecoderMethod*, we can add a method to get it here. */

 private:
  UPB_DISALLOW_COPY_AND_ASSIGN(CodeCache)
#else
struct upb_pbcodecache {
#endif
  bool allow_jit_;

  /* Array of mgroups. */
  upb_inttable groups;
};

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

void upb_pbdecodermethodopts_init(upb_pbdecodermethodopts *opts,
                                  const upb_handlers *h);
void upb_pbdecodermethodopts_setlazy(upb_pbdecodermethodopts *opts, bool lazy);


/* Include refcounted methods like upb_pbdecodermethod_ref(). */
UPB_REFCOUNTED_CMETHODS(upb_pbdecodermethod, upb_pbdecodermethod_upcast)

const upb_handlers *upb_pbdecodermethod_desthandlers(
    const upb_pbdecodermethod *m);
const upb_byteshandler *upb_pbdecodermethod_inputhandler(
    const upb_pbdecodermethod *m);
bool upb_pbdecodermethod_isnative(const upb_pbdecodermethod *m);
const upb_pbdecodermethod *upb_pbdecodermethod_new(
    const upb_pbdecodermethodopts *opts, const void *owner);

void upb_pbcodecache_init(upb_pbcodecache *c);
void upb_pbcodecache_uninit(upb_pbcodecache *c);
bool upb_pbcodecache_allowjit(const upb_pbcodecache *c);
bool upb_pbcodecache_setallowjit(upb_pbcodecache *c, bool allow);
const upb_pbdecodermethod *upb_pbcodecache_getdecodermethod(
    upb_pbcodecache *c, const upb_pbdecodermethodopts *opts);

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

inline DecoderMethodOptions::DecoderMethodOptions(const Handlers* h) {
  upb_pbdecodermethodopts_init(this, h);
}
inline void DecoderMethodOptions::set_lazy(bool lazy) {
  upb_pbdecodermethodopts_setlazy(this, lazy);
}

inline const Handlers* DecoderMethod::dest_handlers() const {
  return upb_pbdecodermethod_desthandlers(this);
}
inline const BytesHandler* DecoderMethod::input_handler() const {
  return upb_pbdecodermethod_inputhandler(this);
}
inline bool DecoderMethod::is_native() const {
  return upb_pbdecodermethod_isnative(this);
}
/* static */
inline reffed_ptr<const DecoderMethod> DecoderMethod::New(
    const DecoderMethodOptions &opts) {
  const upb_pbdecodermethod *m = upb_pbdecodermethod_new(&opts, &m);
  return reffed_ptr<const DecoderMethod>(m, &m);
}

inline CodeCache::CodeCache() {
  upb_pbcodecache_init(this);
}
inline CodeCache::~CodeCache() {
  upb_pbcodecache_uninit(this);
}
inline bool CodeCache::allow_jit() const {
  return upb_pbcodecache_allowjit(this);
}
inline bool CodeCache::set_allow_jit(bool allow) {
  return upb_pbcodecache_setallowjit(this, allow);
}
inline const DecoderMethod *CodeCache::GetDecoderMethod(
    const DecoderMethodOptions& opts) {
  return upb_pbcodecache_getdecodermethod(this, &opts);
}

}  /* namespace pb */
}  /* namespace upb */

#endif  /* __cplusplus */

#endif  /* UPB_DECODER_H_ */
