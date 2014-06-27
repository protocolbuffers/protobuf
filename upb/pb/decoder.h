/*
 * upb - a minimalist implementation of protocol buffers.
 *
 * Copyright (c) 2009-2014 Google Inc.  See LICENSE for details.
 * Author: Josh Haberman <jhaberman@gmail.com>
 *
 * upb::pb::Decoder implements a high performance, streaming, resumable decoder
 * for the binary protobuf format.
 *
 * This interface works the same regardless of what decoder backend is being
 * used.  A client of this class does not need to know whether decoding is using
 * a JITted decoder (DynASM, LLVM, etc) or an interpreted decoder.  By default,
 * it will always use the fastest available decoder.  However, you can call
 * set_allow_jit(false) to disable any JIT decoder that might be available.
 * This is primarily useful for testing purposes.
 */

#ifndef UPB_DECODER_H_
#define UPB_DECODER_H_

#include "upb/table.int.h"
#include "upb/sink.h"

#ifdef __cplusplus
namespace upb {
namespace pb {
class CodeCache;
class Decoder;
class DecoderMethod;
class DecoderMethodOptions;
}  // namespace pb
}  // namespace upb

typedef upb::pb::CodeCache upb_pbcodecache;
typedef upb::pb::Decoder upb_pbdecoder;
typedef upb::pb::DecoderMethod upb_pbdecodermethod;
typedef upb::pb::DecoderMethodOptions upb_pbdecodermethodopts;
#else
struct upb_pbdecoder;
struct upb_pbdecodermethod;
struct upb_pbdecodermethodopts;
struct upb_pbcodecache;

typedef struct upb_pbdecoder upb_pbdecoder;
typedef struct upb_pbdecodermethod upb_pbdecodermethod;
typedef struct upb_pbdecodermethodopts upb_pbdecodermethodopts;
typedef struct upb_pbcodecache upb_pbcodecache;
#endif

// The maximum that any submessages can be nested.  Matches proto2's limit.
// This specifies the size of the decoder's statically-sized array and therefore
// setting it high will cause the upb::pb::Decoder object to be larger.
//
// If necessary we can add a runtime-settable property to Decoder that allow
// this to be larger than the compile-time setting, but this would add
// complexity, particularly since we would have to decide how/if to give users
// the ability to set a custom memory allocation function.
#define UPB_DECODER_MAX_NESTING 64

// Internal-only struct used by the decoder.
typedef struct {
#ifdef __cplusplus
 private:
#endif
  // Space optimization note: we store two pointers here that the JIT
  // doesn't need at all; the upb_handlers* inside the sink and
  // the dispatch table pointer.  We can optimze so that the JIT uses
  // smaller stack frames than the interpreter.  The only thing we need
  // to guarantee is that the fallback routines can find end_ofs.

#ifdef __cplusplus
  char sink[sizeof(upb_sink)];
#else
  upb_sink sink;
#endif
  // The absolute stream offset of the end-of-frame delimiter.
  // Non-delimited frames (groups and non-packed repeated fields) reuse the
  // delimiter of their parent, even though the frame may not end there.
  //
  // NOTE: the JIT stores a slightly different value here for non-top frames.
  // It stores the value relative to the end of the enclosed message.  But the
  // top frame is still stored the same way, which is important for ensuring
  // that calls from the JIT into C work correctly.
  uint64_t end_ofs;
  const uint32_t *base;

  // 0 indicates a length-delimited field.
  // A positive number indicates a known group.
  // A negative number indicates an unknown group.
  int32_t groupnum;
  upb_inttable *dispatch;  // Not used by the JIT.
} upb_pbdecoder_frame;

#ifdef __cplusplus

// The parameters one uses to construct a DecoderMethod.
// TODO(haberman): move allowjit here?  Seems more convenient for users.
class upb::pb::DecoderMethodOptions {
 public:
  // Parameter represents the destination handlers that this method will push
  // to.
  explicit DecoderMethodOptions(const Handlers* dest_handlers);

  // Should the decoder push submessages to lazy handlers for fields that have
  // them?  The caller should set this iff the lazy handlers expect data that is
  // in protobuf binary format and the caller wishes to lazy parse it.
  void set_lazy(bool lazy);

 private:
#else
struct upb_pbdecodermethodopts {
#endif
  const upb_handlers *handlers;
  bool lazy;
};

#ifdef __cplusplus

// Represents the code to parse a protobuf according to a destination Handlers.
class upb::pb::DecoderMethod /* : public upb::RefCounted */ {
 public:
  // From upb::ReferenceCounted.
  void Ref(const void* owner) const;
  void Unref(const void* owner) const;
  void DonateRef(const void* from, const void* to) const;
  void CheckRef(const void* owner) const;

  // The destination handlers that are statically bound to this method.
  // This method is only capable of outputting to a sink that uses these
  // handlers.
  const Handlers* dest_handlers() const;

  // The input handlers for this decoder method.
  const BytesHandler* input_handler() const;

  // Whether this method is native.
  bool is_native() const;

  // Convenience method for generating a DecoderMethod without explicitly
  // creating a CodeCache.
  static reffed_ptr<const DecoderMethod> New(const DecoderMethodOptions& opts);

 private:
  UPB_DISALLOW_POD_OPS(DecoderMethod, upb::pb::DecoderMethod);
#else
struct upb_pbdecodermethod {
#endif
  upb_refcounted base;

  // While compiling, the base is relative in "ofs", after compiling it is
  // absolute in "ptr".
  union {
    uint32_t ofs;     // PC offset of method.
    void *ptr;        // Pointer to bytecode or machine code for this method.
  } code_base;

  // The decoder method group to which this method belongs.  We own a ref.
  // Owning a ref on the entire group is more coarse-grained than is strictly
  // necessary; all we truly require is that methods we directly reference
  // outlive us, while the group could contain many other messages we don't
  // require.  But the group represents the messages that were
  // allocated+compiled together, so it makes the most sense to free them
  // together also.
  const upb_refcounted *group;

  // Whether this method is native code or bytecode.
  bool is_native_;

  // The handler one calls to invoke this method.
  upb_byteshandler input_handler_;

  // The destination handlers this method is bound to.  We own a ref.
  const upb_handlers *dest_handlers_;

  // The dispatch table layout is:
  //   [field number] -> [ 48-bit offset ][ 8-bit wt2 ][ 8-bit wt1 ]
  //
  // If wt1 matches, jump to the 48-bit offset.  If wt2 matches, lookup
  // (UPB_MAX_FIELDNUMBER + fieldnum) and jump there.
  //
  // We need two wire types because of packed/non-packed compatibility.  A
  // primitive repeated field can use either wire type and be valid.  While we
  // could key the table on fieldnum+wiretype, the table would be 8x sparser.
  //
  // Storing two wire types in the primary value allows us to quickly rule out
  // the second wire type without needing to do a separate lookup (this case is
  // less common than an unknown field).
  upb_inttable dispatch;
};

#ifdef __cplusplus

// A Decoder receives binary protobuf data on its input sink and pushes the
// decoded data to its output sink.
class upb::pb::Decoder {
 public:
  // Constructs a decoder instance for the given method, which must outlive this
  // decoder.  Any errors during parsing will be set on the given status, which
  // must also outlive this decoder.
  Decoder(const DecoderMethod* method, Status* status);
  ~Decoder();

  // Returns the DecoderMethod this decoder is parsing from.
  // TODO(haberman): Do users need to be able to rebind this?
  const DecoderMethod* method() const;

  // Resets the state of the decoder.
  void Reset();

  // Returns number of bytes successfully parsed.
  //
  // This can be useful for determining the stream position where an error
  // occurred.
  //
  // This value may not be up-to-date when called from inside a parsing
  // callback.
  uint64_t BytesParsed() const;

  // Resets the output sink of the Decoder.
  // The given sink must match method()->dest_handlers().
  //
  // This must be called at least once before the decoder can be used.  It may
  // only be called with the decoder is in a state where it was just created or
  // reset with pipeline.Reset().  The given sink must be from the same pipeline
  // as this decoder.
  bool ResetOutput(Sink* sink);

  // The sink on which this decoder receives input.
  BytesSink* input();

 private:
  UPB_DISALLOW_COPY_AND_ASSIGN(Decoder);
#else
struct upb_pbdecoder {
#endif
  // Our input sink.
  upb_bytessink input_;

  // The decoder method we are parsing with (owned).
  const upb_pbdecodermethod *method_;

  size_t call_len;
  const uint32_t *pc, *last;

  // Current input buffer and its stream offset.
  const char *buf, *ptr, *end, *checkpoint;

  // End of the delimited region, relative to ptr, or NULL if not in this buf.
  const char *delim_end;

  // End of the delimited region, relative to ptr, or end if not in this buf.
  const char *data_end;

  // Overall stream offset of "buf."
  uint64_t bufstart_ofs;

  // Buffer for residual bytes not parsed from the previous buffer.
  // The maximum number of residual bytes we require is 12; a five-byte
  // unknown tag plus an eight-byte value, less one because the value
  // is only a partial value.
  char residual[12];
  char *residual_end;

  // Stores the user buffer passed to our decode function.
  const char *buf_param;
  size_t size_param;
  const upb_bufhandle *handle;

#ifdef UPB_USE_JIT_X64
  // Used momentarily by the generated code to store a value while a user
  // function is called.
  uint32_t tmp_len;

  const void *saved_rsp;
#endif

  upb_status *status;

  // Our internal stack.
  upb_pbdecoder_frame *top, *limit;
  upb_pbdecoder_frame stack[UPB_DECODER_MAX_NESTING];
#ifdef UPB_USE_JIT_X64
  // Each native stack frame needs two pointers, plus we need a few frames for
  // the enter/exit trampolines.
  const uint32_t *callstack[(UPB_DECODER_MAX_NESTING * 2) + 10];
#else
  const uint32_t *callstack[UPB_DECODER_MAX_NESTING];
#endif
};

#ifdef __cplusplus

// A class for caching protobuf processing code, whether bytecode for the
// interpreted decoder or machine code for the JIT.
//
// This class is not thread-safe.
class upb::pb::CodeCache {
 public:
  CodeCache();
  ~CodeCache();

  // Whether the cache is allowed to generate machine code.  Defaults to true.
  // There is no real reason to turn it off except for testing or if you are
  // having a specific problem with the JIT.
  //
  // Note that allow_jit = true does not *guarantee* that the code will be JIT
  // compiled.  If this platform is not supported or the JIT was not compiled
  // in, the code may still be interpreted.
  bool allow_jit() const;

  // This may only be called when the object is first constructed, and prior to
  // any code generation, otherwise returns false and does nothing.
  bool set_allow_jit(bool allow);

  // Returns a DecoderMethod that can push data to the given handlers.
  // If a suitable method already exists, it will be returned from the cache.
  //
  // Specifying the destination handlers here allows the DecoderMethod to be
  // statically bound to the destination handlers if possible, which can allow
  // more efficient decoding.  However the returned method may or may not
  // actually be statically bound.  But in all cases, the returned method can
  // push data to the given handlers.
  const DecoderMethod *GetDecoderMethod(const DecoderMethodOptions& opts);

  // If/when someone needs to explicitly create a dynamically-bound
  // DecoderMethod*, we can add a method to get it here.

 private:
  UPB_DISALLOW_COPY_AND_ASSIGN(CodeCache);
#else
struct upb_pbcodecache {
#endif
  bool allow_jit_;

  // Array of mgroups.
  upb_inttable groups;
};


#ifdef __cplusplus
extern "C" {
#endif

void upb_pbdecoder_init(upb_pbdecoder *d, const upb_pbdecodermethod *method,
                        upb_status *status);
void upb_pbdecoder_uninit(upb_pbdecoder *d);
void upb_pbdecoder_reset(upb_pbdecoder *d);
const upb_pbdecodermethod *upb_pbdecoder_method(const upb_pbdecoder *d);
bool upb_pbdecoder_resetoutput(upb_pbdecoder *d, upb_sink *sink);
upb_bytessink *upb_pbdecoder_input(upb_pbdecoder *d);
uint64_t upb_pbdecoder_bytesparsed(const upb_pbdecoder *d);

void upb_pbdecodermethodopts_init(upb_pbdecodermethodopts *opts,
                                  const upb_handlers *h);
void upb_pbdecodermethodopts_setlazy(upb_pbdecodermethodopts *opts, bool lazy);

void upb_pbdecodermethod_ref(const upb_pbdecodermethod *m, const void *owner);
void upb_pbdecodermethod_unref(const upb_pbdecodermethod *m, const void *owner);
void upb_pbdecodermethod_donateref(const upb_pbdecodermethod *m,
                                   const void *from, const void *to);
void upb_pbdecodermethod_checkref(const upb_pbdecodermethod *m,
                                  const void *owner);
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

#ifdef __cplusplus
}  /* extern "C" */
#endif

#ifdef __cplusplus

namespace upb {

template<>
class Pointer<pb::DecoderMethod> {
 public:
  explicit Pointer(pb::DecoderMethod* ptr) : ptr_(ptr) {}
  operator pb::DecoderMethod*() { return ptr_; }
  operator RefCounted*() { return UPB_UPCAST(ptr_); }
 private:
  pb::DecoderMethod* ptr_;
};

template<>
class Pointer<const pb::DecoderMethod> {
 public:
  explicit Pointer(const pb::DecoderMethod* ptr) : ptr_(ptr) {}
  operator const pb::DecoderMethod*() { return ptr_; }
  operator const RefCounted*() { return UPB_UPCAST(ptr_); }
 private:
  const pb::DecoderMethod* ptr_;
};

namespace pb {

inline Decoder::Decoder(const DecoderMethod* m, Status* s) {
  upb_pbdecoder_init(this, m, s);
}
inline Decoder::~Decoder() {
  upb_pbdecoder_uninit(this);
}
inline const DecoderMethod* Decoder::method() const {
  return upb_pbdecoder_method(this);
}
inline void Decoder::Reset() {
  upb_pbdecoder_reset(this);
}
inline uint64_t Decoder::BytesParsed() const {
  return upb_pbdecoder_bytesparsed(this);
}
inline bool Decoder::ResetOutput(Sink* sink) {
  return upb_pbdecoder_resetoutput(this, sink);
}
inline BytesSink* Decoder::input() {
  return upb_pbdecoder_input(this);
}

inline DecoderMethodOptions::DecoderMethodOptions(const Handlers* h) {
  upb_pbdecodermethodopts_init(this, h);
}
inline void DecoderMethodOptions::set_lazy(bool lazy) {
  upb_pbdecodermethodopts_setlazy(this, lazy);
}

inline void DecoderMethod::Ref(const void *owner) const {
  upb_pbdecodermethod_ref(this, owner);
}
inline void DecoderMethod::Unref(const void *owner) const {
  upb_pbdecodermethod_unref(this, owner);
}
inline void DecoderMethod::DonateRef(const void *from, const void *to) const {
  upb_pbdecodermethod_donateref(this, from, to);
}
inline void DecoderMethod::CheckRef(const void *owner) const {
  upb_pbdecodermethod_checkref(this, owner);
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
// static
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

}  // namespace pb
}  // namespace upb

#endif  // __cplusplus

#endif  /* UPB_DECODER_H_ */
