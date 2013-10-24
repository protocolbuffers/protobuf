/*
 * upb - a minimalist implementation of protocol buffers.
 *
 * Copyright (c) 2010-2012 Google Inc.  See LICENSE for details.
 * Author: Josh Haberman <jhaberman@gmail.com>
 *
 * A upb_sink is an object that binds a upb_handlers object to some runtime
 * state.  It is the object that can actually receive data via the upb_handlers
 * interface.
 *
 * Unlike upb_def and upb_handlers, upb_sink is never frozen, immutable, or
 * thread-safe.  You can create as many of them as you want, but each one may
 * only be used in a single thread at a time.
 *
 * If we compare with class-based OOP, a you can think of a upb_def as an
 * abstract base class, a upb_handlers as a concrete derived class, and a
 * upb_sink as an object (class instance).
 */

#ifndef UPB_SINK_H
#define UPB_SINK_H

#include "upb/handlers.h"

#ifdef __cplusplus
namespace upb {
class Pipeline;
class Sink;
template <int size> class SeededPipeline;
}
typedef upb::Pipeline upb_pipeline;
typedef upb::Sink upb_sink;
#else
struct upb_pipeline;
struct upb_sink;
typedef struct upb_pipeline upb_pipeline;
typedef struct upb_sink upb_sink;
#endif
struct upb_sinkframe;

// The maximum nesting depth that upb::Sink will allow.  Matches proto2's limit.
// TODO: make this a runtime-settable property of Sink.
#define UPB_SINK_MAX_NESTING 64

#ifdef __cplusplus

// A upb::Pipeline is a set of sinks that can send data to each other.  The
// pipeline object also contains an arena allocator that the sinks and their
// associated processing state can use for fast memory allocation.  This makes
// pipelines very fast to construct and destroy, especially if the arena is
// supplied with an initial block of memory.  If this initial block of memory
// is from the C stack and is large enough, then actual heap allocation can be
// avoided entirely which significantly reduces overhead in some cases.
//
// All sinks and processing state are automatically freed when the pipeline is
// destroyed, so Free() is not necessary or possible.  Allocated objects can
// optionally specify a Reset() callback that will be called when whenever the
// pipeline is Reset() or destroyed.  This can be used to free any outside
// resources the object is holding.
//
// Pipelines (and sinks/objects allocated from them) are not thread-safe!
class upb::Pipeline {
 public:
  // Initializes the pipeline's arena with the given initial memory that will
  // be used before allocating memory using the given allocation function.
  // The "ud" pointer will be passed as the first parameter to the realloc
  // callback, and can be used to pass user-specific state.
  Pipeline(void *initial_mem, size_t initial_size,
        void *(*realloc)(void *ud, void *ptr, size_t size), void *ud);
  ~Pipeline();

  // Returns a newly-allocated Sink for the given handlers.  The sink is will
  // live as long as the pipeline does.  Caller retains ownership of the
  // handlers object, which must outlive the pipeline.
  //
  // TODO(haberman): add an option for the sink to take a ref, so the handlers
  // don't have to outlive?  This would be simpler but imposes a minimum cost.
  // Taking an atomic ref is not *so* bad in the single-threaded case, but this
  // can degrade heavily under contention, so we need a way to avoid it in
  // cases where this overhead would be significant and the caller can easily
  // guarantee the outlive semantics.
  Sink* NewSink(const Handlers* handlers);

  // Accepts a ref donated from the given owner.  Will unref the Handlers when
  // the Pipeline is destroyed.
  void DonateRef(const Handlers* h, const void* owner);

  // The current error status for the pipeline.
  const upb::Status& status() const;

  // Calls "reset" on all Sinks and resettable state objects in the arena, and
  // resets the error status.  Useful for resetting processing state so new
  // input can be accepted.
  void Reset();

  // Allocates/reallocates memory of the given size, or returns NULL if no
  // memory is available.  It is not necessary (or possible) to manually free
  // the memory obtained from these functions.
  void* Alloc(size_t size);
  void* Realloc(void* ptr, size_t old_size, size_t size);

  // Allocates an object with the given FrameType.  Note that this object may
  // *not* be resized with Realloc().
  void* AllocObject(const FrameType* type);

 private:
#else
struct upb_pipeline {
#endif
  void *(*realloc)(void *ud, void *ptr, size_t size);
  void *ud;
  void *bump_top;    // Current alloc offset, either from initial or dyn region.
  void *bump_limit;  // Limit of current alloc block.
  void *obj_head;    // Linked list of objects with "reset" functions.
  void *region_head; // Linked list of dyn regions we got from user's realloc().
  void *last_alloc;
  upb_status status_;
};

struct upb_frametype {
  size_t size;
  void (*init)(void* obj, upb_pipeline *p);
  void (*uninit)(void* obj);
  void (*reset)(void* obj);
};

#ifdef __cplusplus

// For convenience, a template for a pipeline with an array of initial memory.
template <int initial_size>
class upb::SeededPipeline : public upb::Pipeline {
 public:
  SeededPipeline(void *(*realloc)(void *ud, void *ptr, size_t size), void *ud)
      : Pipeline(mem_, initial_size, realloc, ud) {
  }

 private:
  char mem_[initial_size];
};

// A upb::Sink is an object that binds a upb::Handlers object to some runtime
// state.  It is the object that can actually call a set of handlers.
//
// Unlike upb::Def and upb::Handlers, upb::Sink is never frozen, immutable, or
// thread-safe.  You can create as many of them as you want, but each one may
// only be used in a single thread at a time.
//
// If we compare with class-based OOP, a you can think of a upb::Def as an
// abstract base class, a upb::Handlers as a concrete derived class, and a
// upb::Sink as an object (class instance).
//
// Each upb::Sink lives in exactly one pipeline.
class upb::Sink {
 public:

  // Resets the state of the sink so that it is ready to accept new input.
  // Any state from previously received data is discarded.  "Closure" will be
  // used as the top-level closure.
  void Reset(void *closure);

  // Returns the pipeline that this sink comes from.
  Pipeline* pipeline() const;

  // Returns the top-level object that is bound to this sink.
  template <class T> T* GetObject() const;

  // Functions for pushing data into the sink.
  //
  // These return false if processing should stop (either due to error or just
  // to suspend).
  //
  // These may not be called from within one of the same sink's handlers (in
  // other words, handlers are not re-entrant).

  // Should be called at the start and end of every message; both the top-level
  // message and submessages.  This means that submessages should use the
  // following sequence:
  //   sink->StartSubMessage(startsubmsg_selector);
  //   sink->StartMessage();
  //   // ...
  //   sink->EndMessage();
  //   sink->EndSubMessage(endsubmsg_selector);
  bool StartMessage();
  bool EndMessage();

  // Putting of individual values.  These work for both repeated and
  // non-repeated fields, but for repeated fields you must wrap them in
  // calls to StartSequence()/EndSequence().
  bool PutInt32(Handlers::Selector s, int32_t val);
  bool PutInt64(Handlers::Selector s, int64_t val);
  bool PutUInt32(Handlers::Selector s, uint32_t val);
  bool PutUInt64(Handlers::Selector s, uint64_t val);
  bool PutFloat(Handlers::Selector s, float val);
  bool PutDouble(Handlers::Selector s, double val);
  bool PutBool(Handlers::Selector s, bool val);

  // Putting of string/bytes values.  Each string can consist of zero or more
  // non-contiguous buffers of data.
  bool StartString(Handlers::Selector s, size_t size_hint);
  size_t PutStringBuffer(Handlers::Selector s, const char *buf, size_t len);
  bool EndString(Handlers::Selector s);

  // For submessage fields.
  bool StartSubMessage(Handlers::Selector s);
  bool EndSubMessage(Handlers::Selector s);

  // For repeated fields of any type, the sequence of values must be wrapped in
  // these calls.
  bool StartSequence(Handlers::Selector s);
  bool EndSequence(Handlers::Selector s);

 private:
  UPB_DISALLOW_POD_OPS(Sink);
#else
struct upb_sink {
#endif
  upb_pipeline *pipeline_;
  struct upb_sinkframe *top, *limit, *stack;
};

#ifdef __cplusplus
extern "C" {
#endif

void *upb_realloc(void *ud, void *ptr, size_t size);
void upb_pipeline_init(upb_pipeline *p, void *initial_mem, size_t initial_size,
                       void *(*realloc)(void *ud, void *ptr, size_t size),
                       void *ud);
void upb_pipeline_uninit(upb_pipeline *p);
void *upb_pipeline_alloc(upb_pipeline *p, size_t size);
void *upb_pipeline_realloc(
    upb_pipeline *p, void *ptr, size_t old_size, size_t size);
void *upb_pipeline_allocobj(upb_pipeline *p, const upb_frametype *type);
void upb_pipeline_reset(upb_pipeline *p);
void upb_pipeline_donateref(
    upb_pipeline *p, const upb_handlers *h, const void *owner);
upb_sink *upb_pipeline_newsink(upb_pipeline *p, const upb_handlers *h);
const upb_status *upb_pipeline_status(const upb_pipeline *p);

void upb_sink_reset(upb_sink *s, void *closure);
upb_pipeline *upb_sink_pipeline(const upb_sink *s);
void *upb_sink_getobj(const upb_sink *s);
bool upb_sink_startmsg(upb_sink *s);
bool upb_sink_endmsg(upb_sink *s);
bool upb_sink_putint32(upb_sink *s, upb_selector_t sel, int32_t val);
bool upb_sink_putint64(upb_sink *s, upb_selector_t sel, int64_t val);
bool upb_sink_putuint32(upb_sink *s, upb_selector_t sel, uint32_t val);
bool upb_sink_putuint64(upb_sink *s, upb_selector_t sel, uint64_t val);
bool upb_sink_putfloat(upb_sink *s, upb_selector_t sel, float val);
bool upb_sink_putdouble(upb_sink *s, upb_selector_t sel, double val);
bool upb_sink_putbool(upb_sink *s, upb_selector_t sel, bool val);
bool upb_sink_startstr(upb_sink *s, upb_selector_t sel, size_t size_hint);
size_t upb_sink_putstring(upb_sink *s, upb_selector_t sel, const char *buf,
                          size_t len);
bool upb_sink_endstr(upb_sink *s, upb_selector_t sel);
bool upb_sink_startsubmsg(upb_sink *s, upb_selector_t sel);
bool upb_sink_endsubmsg(upb_sink *s, upb_selector_t sel);
bool upb_sink_startseq(upb_sink *s, upb_selector_t sel);
bool upb_sink_endseq(upb_sink *s, upb_selector_t sel);

#ifdef __cplusplus
}  /* extern "C" */
#endif

#ifdef __cplusplus

namespace upb {


inline Pipeline::Pipeline(void *initial_mem, size_t initial_size,
                    void *(*realloc)(void *ud, void *ptr, size_t size),
                    void *ud) {
  upb_pipeline_init(this, initial_mem, initial_size, realloc, ud);
}
inline Pipeline::~Pipeline() {
  upb_pipeline_uninit(this);
}
inline void* Pipeline::Alloc(size_t size) {
  return upb_pipeline_alloc(this, size);
}
inline void* Pipeline::Realloc(void* ptr, size_t old_size, size_t size) {
  return upb_pipeline_realloc(this, ptr, old_size, size);
}
inline void* Pipeline::AllocObject(const upb::FrameType* type) {
  return upb_pipeline_allocobj(this, type);
}
inline void Pipeline::Reset() {
  upb_pipeline_reset(this);
}
inline const upb::Status& Pipeline::status() const {
  return *upb_pipeline_status(this);
}
inline Sink* Pipeline::NewSink(const upb::Handlers* handlers) {
  return upb_pipeline_newsink(this, handlers);
}
inline void Pipeline::DonateRef(const upb::Handlers* h, const void *owner) {
  return upb_pipeline_donateref(this, h, owner);
}

inline void Sink::Reset(void *closure) {
  upb_sink_reset(this, closure);
}
inline Pipeline* Sink::pipeline() const {
  return upb_sink_pipeline(this);
}
template <class T>
inline T* Sink::GetObject() const {
  return static_cast<T*>(upb_sink_getobj(this));
}
inline bool Sink::StartMessage() {
  return upb_sink_startmsg(this);
}
inline bool Sink::EndMessage() {
  return upb_sink_endmsg(this);
}
inline bool Sink::PutInt32(Handlers::Selector sel, int32_t val) {
  return upb_sink_putint32(this, sel, val);
}
inline bool Sink::PutInt64(Handlers::Selector sel, int64_t val) {
  return upb_sink_putint64(this, sel, val);
}
inline bool Sink::PutUInt32(Handlers::Selector sel, uint32_t val) {
  return upb_sink_putuint32(this, sel, val);
}
inline bool Sink::PutUInt64(Handlers::Selector sel, uint64_t val) {
  return upb_sink_putuint64(this, sel, val);
}
inline bool Sink::PutFloat(Handlers::Selector sel, float val) {
  return upb_sink_putfloat(this, sel, val);
}
inline bool Sink::PutDouble(Handlers::Selector sel, double val) {
  return upb_sink_putdouble(this, sel, val);
}
inline bool Sink::PutBool(Handlers::Selector sel, bool val) {
  return upb_sink_putbool(this, sel, val);
}
inline bool Sink::StartString(Handlers::Selector sel, size_t size_hint) {
  return upb_sink_startstr(this, sel, size_hint);
}
inline size_t Sink::PutStringBuffer(Handlers::Selector sel, const char *buf,
                                  size_t len) {
  return upb_sink_putstring(this, sel, buf, len);
}
inline bool Sink::EndString(Handlers::Selector sel) {
  return upb_sink_endstr(this, sel);
}
inline bool Sink::StartSubMessage(Handlers::Selector sel) {
  return upb_sink_startsubmsg(this, sel);
}
inline bool Sink::EndSubMessage(Handlers::Selector sel) {
  return upb_sink_endsubmsg(this, sel);
}
inline bool Sink::StartSequence(Handlers::Selector sel) {
  return upb_sink_startseq(this, sel);
}
inline bool Sink::EndSequence(Handlers::Selector sel) {
  return upb_sink_endseq(this, sel);
}

}  // namespace upb
#endif

// TODO(haberman): move this to sink.c.  We keep it here now only because the
// JIT needs to modify it directly, which it only needs to do because it makes
// the interpreter handle fallback cases.  When the JIT is self-sufficient, it
// will no longer need to touch the sink's stack at all.
struct upb_sinkframe {
  const upb_handlers *h;
  void *closure;

  // For any frames besides the top, this is the END* callback that will run
  // when the subframe is popped (for example, for a "sequence" frame the frame
  // above it will be a UPB_HANDLER_ENDSEQ handler).  But this is only
  // necessary for assertion checking inside upb_sink and can be omitted if the
  // sink has only one caller.
  //
  // TODO(haberman): have a mechanism for ensuring that a sink only has one
  // caller.
  upb_selector_t selector;
};

#endif
