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
class BufferSource;
class BytesSink;
class Sink;
}
typedef upb::BufferSource upb_bufsrc;
typedef upb::BytesSink upb_bytessink;
typedef upb::Sink upb_sink;
#else
struct upb_sink;
struct upb_bufsrc;
struct upb_bytessink;
typedef struct upb_sink upb_sink;
typedef struct upb_bytessink upb_bytessink;
typedef struct upb_bufsrc upb_bufsrc;
#endif

// Internal-only struct for the sink.
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

// The maximum nesting depth that upb::Sink will allow.  Matches proto2's limit.
// TODO: make this a runtime-settable property of Sink.
#define UPB_SINK_MAX_NESTING 64

#ifdef __cplusplus

// A upb::Sink is an object that binds a upb::Handlers object to some runtime
// state.  It represents an endpoint to which data can be sent.
class upb::Sink {
 public:
  // Constructor with no initialization; must be Reset() before use.
  Sink() {}

  // Constructs a new sink for the given frozen handlers and closure.
  //
  // TODO: once the Handlers know the expected closure type, verify that T
  // matches it.
  template <class T> Sink(const Handlers* handlers, T* closure);

  // Resets the value of the sink.
  template <class T> void Reset(const Handlers* handlers, T* closure);

  // Returns the top-level object that is bound to this sink.
  //
  // TODO: once the Handlers know the expected closure type, verify that T
  // matches it.
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
  //   sink->EndMessage(&status);
  //   sink->EndSubMessage(endsubmsg_selector);
  bool StartMessage();
  bool EndMessage(Status* status);

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
  //
  // For StartString(), the function will write a sink for the string to "sub."
  // The sub-sink must be used for any/all PutStringBuffer() calls.
  bool StartString(Handlers::Selector s, size_t size_hint, Sink* sub);
  size_t PutStringBuffer(Handlers::Selector s, const char *buf, size_t len);
  bool EndString(Handlers::Selector s);

  // For submessage fields.
  //
  // For StartSubMessage(), the function will write a sink for the string to
  // "sub." The sub-sink must be used for any/all handlers called within the
  // submessage.
  bool StartSubMessage(Handlers::Selector s, Sink* sub);
  bool EndSubMessage(Handlers::Selector s);

  // For repeated fields of any type, the sequence of values must be wrapped in
  // these calls.
  //
  // For StartSequence(), the function will write a sink for the string to
  // "sub." The sub-sink must be used for any/all handlers called within the
  // sequence.
  bool StartSequence(Handlers::Selector s, Sink* sub);
  bool EndSequence(Handlers::Selector s);

  // Copy and assign specifically allowed.
  // We don't even bother making these members private because so many
  // functions need them and this is mainly just a dumb data container anyway.
#else
struct upb_sink {
#endif
  const upb_handlers *handlers;
  void *closure;
};

#ifdef __cplusplus

class upb::BytesSink {
 public:
  BytesSink() {}

  // Constructs a new sink for the given frozen handlers and closure.
  //
  // TODO(haberman): once the Handlers know the expected closure type, verify
  // that T matches it.
  template <class T> BytesSink(const Handlers* handlers, T* closure);

  // Resets the value of the sink.
  template <class T> void Reset(const Handlers* handlers, T* closure);

  bool Start(size_t size_hint, void **subc);
  size_t PutBuffer(void *subc, const char *buf, size_t len);
  bool End();

#else
struct upb_bytessink {
#endif
  const upb_byteshandler *handler;
  void *closure;
};

#ifdef __cplusplus

// A class for pushing a flat buffer of data to a BytesSink.
// You can construct an instance of this to get a resumable source,
// or just call the static PutBuffer() to do a non-resumable push all in one go.
class upb::BufferSource {
 public:
  BufferSource();
  BufferSource(const char* buf, size_t len, BytesSink* sink);

  // Returns true if the entire buffer was pushed successfully.  Otherwise the
  // next call to PutNext() will resume where the previous one left off.
  // TODO(haberman): implement this.
  bool PutNext();

  // A static version; with this version is it not possible to resume in the
  // case of failure or a partially-consumed buffer.
  static bool PutBuffer(const char* buf, size_t len, BytesSink* sink);

  template <class T> static bool PutBuffer(const T& str, BytesSink* sink) {
    return PutBuffer(str.c_str(), str.size(), sink);
  }

 private:
#else
struct upb_bufsrc {
#endif
};

#ifdef __cplusplus
extern "C" {
#endif

// Inline definitions.

UPB_INLINE void upb_bytessink_reset(upb_bytessink *s, const upb_byteshandler *h,
                                    void *closure) {
  s->handler = h;
  s->closure = closure;
}

UPB_INLINE bool upb_bytessink_start(upb_bytessink *s, size_t size_hint,
                                    void **subc) {
  if (!s->handler) return true;
  upb_startstr_handlerfunc *start =
      (upb_startstr_handlerfunc *)s->handler->table[UPB_STARTSTR_SELECTOR].func;

  if (!start) return true;
  *subc = start(s->closure, upb_handlerattr_handlerdata(
                                &s->handler->table[UPB_STARTSTR_SELECTOR].attr),
                size_hint);
  return *subc != NULL;
}

UPB_INLINE size_t upb_bytessink_putbuf(upb_bytessink *s, void *subc,
                                       const char *buf, size_t size) {
  if (!s->handler) return true;
  upb_string_handlerfunc *putbuf =
      (upb_string_handlerfunc *)s->handler->table[UPB_STRING_SELECTOR].func;

  if (!putbuf) return true;
  return putbuf(subc, upb_handlerattr_handlerdata(
                          &s->handler->table[UPB_STRING_SELECTOR].attr),
                buf, size);
}

UPB_INLINE bool upb_bytessink_end(upb_bytessink *s) {
  if (!s->handler) return true;
  upb_endfield_handlerfunc *end =
      (upb_endfield_handlerfunc *)s->handler->table[UPB_ENDSTR_SELECTOR].func;

  if (!end) return true;
  return end(s->closure,
             upb_handlerattr_handlerdata(
                 &s->handler->table[UPB_ENDSTR_SELECTOR].attr));
}

UPB_INLINE bool upb_bufsrc_putbuf(const char *buf, size_t len,
                                  upb_bytessink *sink) {
  void *subc;
  return
      upb_bytessink_start(sink, len, &subc) &&
      (len == 0 || upb_bytessink_putbuf(sink, subc, buf, len) == len) &&
      upb_bytessink_end(sink);
}

#define PUTVAL(type, ctype)                                                    \
  UPB_INLINE bool upb_sink_put##type(upb_sink *s, upb_selector_t sel,          \
                                     ctype val) {                              \
    if (!s->handlers) return true;                                             \
    upb_##type##_handlerfunc *func =                                           \
        (upb_##type##_handlerfunc *)upb_handlers_gethandler(s->handlers, sel); \
    if (!func) return true;                                                    \
    const void *hd = upb_handlers_gethandlerdata(s->handlers, sel);            \
    return func(s->closure, hd, val);                                          \
  }

PUTVAL(int32,  int32_t);
PUTVAL(int64,  int64_t);
PUTVAL(uint32, uint32_t);
PUTVAL(uint64, uint64_t);
PUTVAL(float,  float);
PUTVAL(double, double);
PUTVAL(bool,   bool);
#undef PUTVAL

UPB_INLINE void upb_sink_reset(upb_sink *s, const upb_handlers *h, void *c) {
  s->handlers = h;
  s->closure = c;
}

UPB_INLINE size_t
upb_sink_putstring(upb_sink *s, upb_selector_t sel, const char *buf, size_t n) {
  if (!s->handlers) return n;
  upb_string_handlerfunc *handler =
      (upb_string_handlerfunc *)upb_handlers_gethandler(s->handlers, sel);

  if (!handler) return n;
  const void *hd = upb_handlers_gethandlerdata(s->handlers, sel);
  return handler(s->closure, hd, buf, n);
}

UPB_INLINE bool upb_sink_startmsg(upb_sink *s) {
  if (!s->handlers) return true;
  upb_startmsg_handlerfunc *startmsg =
      (upb_startmsg_handlerfunc *)upb_handlers_gethandler(s->handlers,
                                                      UPB_STARTMSG_SELECTOR);
  if (!startmsg) return true;
  const void *hd =
      upb_handlers_gethandlerdata(s->handlers, UPB_STARTMSG_SELECTOR);
  return startmsg(s->closure, hd);
}

UPB_INLINE bool upb_sink_endmsg(upb_sink *s, upb_status *status) {
  if (!s->handlers) return true;
  upb_endmsg_handlerfunc *endmsg =
      (upb_endmsg_handlerfunc *)upb_handlers_gethandler(s->handlers,
                                                        UPB_ENDMSG_SELECTOR);

  if (!endmsg) return true;
  const void *hd =
      upb_handlers_gethandlerdata(s->handlers, UPB_ENDMSG_SELECTOR);
  return endmsg(s->closure, hd, status);
}

UPB_INLINE bool upb_sink_startseq(upb_sink *s, upb_selector_t sel,
                                  upb_sink *sub) {
  sub->closure = s->closure;
  sub->handlers = s->handlers;
  if (!s->handlers) return true;
  upb_startfield_handlerfunc *startseq =
      (upb_startfield_handlerfunc*)upb_handlers_gethandler(s->handlers, sel);

  if (!startseq) return true;
  const void *hd = upb_handlers_gethandlerdata(s->handlers, sel);
  sub->closure = startseq(s->closure, hd);
  return sub->closure ? true : false;
}

UPB_INLINE bool upb_sink_endseq(upb_sink *s, upb_selector_t sel) {
  if (!s->handlers) return true;
  upb_endfield_handlerfunc *endseq =
      (upb_endfield_handlerfunc*)upb_handlers_gethandler(s->handlers, sel);

  if (!endseq) return true;
  const void *hd = upb_handlers_gethandlerdata(s->handlers, sel);
  return endseq(s->closure, hd);
}

UPB_INLINE bool upb_sink_startstr(upb_sink *s, upb_selector_t sel,
                                  size_t size_hint, upb_sink *sub) {
  sub->closure = s->closure;
  sub->handlers = s->handlers;
  if (!s->handlers) return true;
  upb_startstr_handlerfunc *startstr =
      (upb_startstr_handlerfunc*)upb_handlers_gethandler(s->handlers, sel);

  if (!startstr) return true;
  const void *hd = upb_handlers_gethandlerdata(s->handlers, sel);
  sub->closure = startstr(s->closure, hd, size_hint);
  return sub->closure ? true : false;
}

UPB_INLINE bool upb_sink_endstr(upb_sink *s, upb_selector_t sel) {
  if (!s->handlers) return true;
  upb_endfield_handlerfunc *endstr =
      (upb_endfield_handlerfunc*)upb_handlers_gethandler(s->handlers, sel);

  if (!endstr) return true;
  const void *hd = upb_handlers_gethandlerdata(s->handlers, sel);
  return endstr(s->closure, hd);
}

UPB_INLINE bool upb_sink_startsubmsg(upb_sink *s, upb_selector_t sel,
                                     upb_sink *sub) {
  sub->closure = s->closure;
  if (!s->handlers) {
    sub->handlers = NULL;
    return true;
  }
  sub->handlers = upb_handlers_getsubhandlers_sel(s->handlers, sel);
  upb_startfield_handlerfunc *startsubmsg =
      (upb_startfield_handlerfunc*)upb_handlers_gethandler(s->handlers, sel);

  if (!startsubmsg) return true;
  const void *hd = upb_handlers_gethandlerdata(s->handlers, sel);
  sub->closure = startsubmsg(s->closure, hd);
  return sub->closure ? true : false;
}

UPB_INLINE bool upb_sink_endsubmsg(upb_sink *s, upb_selector_t sel) {
  if (!s->handlers) return true;
  upb_endfield_handlerfunc *endsubmsg =
      (upb_endfield_handlerfunc*)upb_handlers_gethandler(s->handlers, sel);

  if (!endsubmsg) return s->closure;
  const void *hd = upb_handlers_gethandlerdata(s->handlers, sel);
  return endsubmsg(s->closure, hd);
}

#ifdef __cplusplus
}  /* extern "C" */
#endif

#ifdef __cplusplus

namespace upb {

template <class T> Sink::Sink(const Handlers* handlers, T* closure) {
  upb_sink_reset(this, handlers, closure);
}
template <class T>
inline void Sink::Reset(const Handlers* handlers, T* closure) {
  upb_sink_reset(this, handlers, closure);
}
inline bool Sink::StartMessage() {
  return upb_sink_startmsg(this);
}
inline bool Sink::EndMessage(Status* status) {
  return upb_sink_endmsg(this, status);
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
inline bool Sink::StartString(Handlers::Selector sel, size_t size_hint,
                              Sink *sub) {
  return upb_sink_startstr(this, sel, size_hint, sub);
}
inline size_t Sink::PutStringBuffer(Handlers::Selector sel, const char *buf,
                                  size_t len) {
  return upb_sink_putstring(this, sel, buf, len);
}
inline bool Sink::EndString(Handlers::Selector sel) {
  return upb_sink_endstr(this, sel);
}
inline bool Sink::StartSubMessage(Handlers::Selector sel, Sink* sub) {
  return upb_sink_startsubmsg(this, sel, sub);
}
inline bool Sink::EndSubMessage(Handlers::Selector sel) {
  return upb_sink_endsubmsg(this, sel);
}
inline bool Sink::StartSequence(Handlers::Selector sel, Sink* sub) {
  return upb_sink_startseq(this, sel, sub);
}
inline bool Sink::EndSequence(Handlers::Selector sel) {
  return upb_sink_endseq(this, sel);
}

inline bool BytesSink::Start(size_t size_hint, void **subc) {
  return upb_bytessink_start(this, size_hint, subc);
}
inline size_t BytesSink::PutBuffer(void *subc, const char *buf, size_t len) {
  return upb_bytessink_putbuf(this, subc, buf, len);
}
inline bool BytesSink::End() {
  return upb_bytessink_end(this);
}

inline bool BufferSource::PutBuffer(const char *buf, size_t len,
                                    BytesSink *sink) {
  return upb_bufsrc_putbuf(buf, len, sink);
}

}  // namespace upb
#endif

#endif
