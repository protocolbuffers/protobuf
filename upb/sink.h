/*
** upb::Sink (upb_sink)
** upb::BytesSink (upb_bytessink)
**
** A upb_sink is an object that binds a upb_handlers object to some runtime
** state.  It is the object that can actually receive data via the upb_handlers
** interface.
**
** Unlike upb_def and upb_handlers, upb_sink is never frozen, immutable, or
** thread-safe.  You can create as many of them as you want, but each one may
** only be used in a single thread at a time.
**
** If we compare with class-based OOP, a you can think of a upb_def as an
** abstract base class, a upb_handlers as a concrete derived class, and a
** upb_sink as an object (class instance).
*/

#ifndef UPB_SINK_H
#define UPB_SINK_H

#include "upb/handlers.h"

#ifdef __cplusplus
namespace upb {
class BufferSink;
class BufferSource;
class BytesSink;
class Sink;
}
#endif

UPB_DECLARE_TYPE(upb::BufferSink, upb_bufsink)
UPB_DECLARE_TYPE(upb::BufferSource, upb_bufsrc)
UPB_DECLARE_TYPE(upb::BytesSink, upb_bytessink)
UPB_DECLARE_TYPE(upb::Sink, upb_sink)

#ifdef __cplusplus

/* A upb::Sink is an object that binds a upb::Handlers object to some runtime
 * state.  It represents an endpoint to which data can be sent.
 *
 * TODO(haberman): right now all of these functions take selectors.  Should they
 * take selectorbase instead?
 *
 * ie. instead of calling:
 *   sink->StartString(FOO_FIELD_START_STRING, ...)
 * a selector base would let you say:
 *   sink->StartString(FOO_FIELD, ...)
 *
 * This would make call sites a little nicer and require emitting fewer selector
 * definitions in .h files.
 *
 * But the current scheme has the benefit that you can retrieve a function
 * pointer for any handler with handlers->GetHandler(selector), without having
 * to have a separate GetHandler() function for each handler type.  The JIT
 * compiler uses this.  To accommodate we'd have to expose a separate
 * GetHandler() for every handler type.
 *
 * Also to ponder: selectors right now are independent of a specific Handlers
 * instance.  In other words, they allocate a number to every possible handler
 * that *could* be registered, without knowing anything about what handlers
 * *are* registered.  That means that using selectors as table offsets prohibits
 * us from compacting the handler table at Freeze() time.  If the table is very
 * sparse, this could be wasteful.
 *
 * Having another selector-like thing that is specific to a Handlers instance
 * would allow this compacting, but then it would be impossible to write code
 * ahead-of-time that can be bound to any Handlers instance at runtime.  For
 * example, a .proto file parser written as straight C will not know what
 * Handlers it will be bound to, so when it calls sink->StartString() what
 * selector will it pass?  It needs a selector like we have today, that is
 * independent of any particular upb::Handlers.
 *
 * Is there a way then to allow Handlers table compaction? */
class upb::Sink {
 public:
  /* Constructor with no initialization; must be Reset() before use. */
  Sink() {}

  /* Constructs a new sink for the given frozen handlers and closure.
   *
   * TODO: once the Handlers know the expected closure type, verify that T
   * matches it. */
  template <class T> Sink(const Handlers* handlers, T* closure);

  /* Resets the value of the sink. */
  template <class T> void Reset(const Handlers* handlers, T* closure);

  /* Returns the top-level object that is bound to this sink.
   *
   * TODO: once the Handlers know the expected closure type, verify that T
   * matches it. */
  template <class T> T* GetObject() const;

  /* Functions for pushing data into the sink.
   *
   * These return false if processing should stop (either due to error or just
   * to suspend).
   *
   * These may not be called from within one of the same sink's handlers (in
   * other words, handlers are not re-entrant). */

  /* Should be called at the start and end of every message; both the top-level
   * message and submessages.  This means that submessages should use the
   * following sequence:
   *   sink->StartSubMessage(startsubmsg_selector);
   *   sink->StartMessage();
   *   // ...
   *   sink->EndMessage(&status);
   *   sink->EndSubMessage(endsubmsg_selector); */
  bool StartMessage();
  bool EndMessage(Status* status);

  /* Putting of individual values.  These work for both repeated and
   * non-repeated fields, but for repeated fields you must wrap them in
   * calls to StartSequence()/EndSequence(). */
  bool PutInt32(Handlers::Selector s, int32_t val);
  bool PutInt64(Handlers::Selector s, int64_t val);
  bool PutUInt32(Handlers::Selector s, uint32_t val);
  bool PutUInt64(Handlers::Selector s, uint64_t val);
  bool PutFloat(Handlers::Selector s, float val);
  bool PutDouble(Handlers::Selector s, double val);
  bool PutBool(Handlers::Selector s, bool val);

  /* Putting of string/bytes values.  Each string can consist of zero or more
   * non-contiguous buffers of data.
   *
   * For StartString(), the function will write a sink for the string to "sub."
   * The sub-sink must be used for any/all PutStringBuffer() calls. */
  bool StartString(Handlers::Selector s, size_t size_hint, Sink* sub);
  size_t PutStringBuffer(Handlers::Selector s, const char *buf, size_t len,
                         const BufferHandle *handle);
  bool EndString(Handlers::Selector s);

  /* For submessage fields.
   *
   * For StartSubMessage(), the function will write a sink for the string to
   * "sub." The sub-sink must be used for any/all handlers called within the
   * submessage. */
  bool StartSubMessage(Handlers::Selector s, Sink* sub);
  bool EndSubMessage(Handlers::Selector s);

  /* For repeated fields of any type, the sequence of values must be wrapped in
   * these calls.
   *
   * For StartSequence(), the function will write a sink for the string to
   * "sub." The sub-sink must be used for any/all handlers called within the
   * sequence. */
  bool StartSequence(Handlers::Selector s, Sink* sub);
  bool EndSequence(Handlers::Selector s);

  /* Copy and assign specifically allowed.
   * We don't even bother making these members private because so many
   * functions need them and this is mainly just a dumb data container anyway.
   */
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

  /* Constructs a new sink for the given frozen handlers and closure.
   *
   * TODO(haberman): once the Handlers know the expected closure type, verify
   * that T matches it. */
  template <class T> BytesSink(const BytesHandler* handler, T* closure);

  /* Resets the value of the sink. */
  template <class T> void Reset(const BytesHandler* handler, T* closure);

  bool Start(size_t size_hint, void **subc);
  size_t PutBuffer(void *subc, const char *buf, size_t len,
                   const BufferHandle *handle);
  bool End();
#else
struct upb_bytessink {
#endif
  const upb_byteshandler *handler;
  void *closure;
};

#ifdef __cplusplus

/* A class for pushing a flat buffer of data to a BytesSink.
 * You can construct an instance of this to get a resumable source,
 * or just call the static PutBuffer() to do a non-resumable push all in one
 * go. */
class upb::BufferSource {
 public:
  BufferSource();
  BufferSource(const char* buf, size_t len, BytesSink* sink);

  /* Returns true if the entire buffer was pushed successfully.  Otherwise the
   * next call to PutNext() will resume where the previous one left off.
   * TODO(haberman): implement this. */
  bool PutNext();

  /* A static version; with this version is it not possible to resume in the
   * case of failure or a partially-consumed buffer. */
  static bool PutBuffer(const char* buf, size_t len, BytesSink* sink);

  template <class T> static bool PutBuffer(const T& str, BytesSink* sink) {
    return PutBuffer(str.c_str(), str.size(), sink);
  }
#else
struct upb_bufsrc {
  char dummy;
#endif
};

UPB_BEGIN_EXTERN_C

/* A class for accumulating output string data in a flat buffer. */

upb_bufsink *upb_bufsink_new(upb_env *env);
void upb_bufsink_free(upb_bufsink *sink);
upb_bytessink *upb_bufsink_sink(upb_bufsink *sink);
const char *upb_bufsink_getdata(const upb_bufsink *sink, size_t *len);

/* Inline definitions. */

UPB_INLINE void upb_bytessink_reset(upb_bytessink *s, const upb_byteshandler *h,
                                    void *closure) {
  s->handler = h;
  s->closure = closure;
}

UPB_INLINE bool upb_bytessink_start(upb_bytessink *s, size_t size_hint,
                                    void **subc) {
  typedef upb_startstr_handlerfunc func;
  func *start;
  *subc = s->closure;
  if (!s->handler) return true;
  start = (func *)s->handler->table[UPB_STARTSTR_SELECTOR].func;

  if (!start) return true;
  *subc = start(s->closure, upb_handlerattr_handlerdata(
                                &s->handler->table[UPB_STARTSTR_SELECTOR].attr),
                size_hint);
  return *subc != NULL;
}

UPB_INLINE size_t upb_bytessink_putbuf(upb_bytessink *s, void *subc,
                                       const char *buf, size_t size,
                                       const upb_bufhandle* handle) {
  typedef upb_string_handlerfunc func;
  func *putbuf;
  if (!s->handler) return true;
  putbuf = (func *)s->handler->table[UPB_STRING_SELECTOR].func;

  if (!putbuf) return true;
  return putbuf(subc, upb_handlerattr_handlerdata(
                          &s->handler->table[UPB_STRING_SELECTOR].attr),
                buf, size, handle);
}

UPB_INLINE bool upb_bytessink_end(upb_bytessink *s) {
  typedef upb_endfield_handlerfunc func;
  func *end;
  if (!s->handler) return true;
  end = (func *)s->handler->table[UPB_ENDSTR_SELECTOR].func;

  if (!end) return true;
  return end(s->closure,
             upb_handlerattr_handlerdata(
                 &s->handler->table[UPB_ENDSTR_SELECTOR].attr));
}

bool upb_bufsrc_putbuf(const char *buf, size_t len, upb_bytessink *sink);

#define PUTVAL(type, ctype)                                                    \
  UPB_INLINE bool upb_sink_put##type(upb_sink *s, upb_selector_t sel,          \
                                     ctype val) {                              \
    typedef upb_##type##_handlerfunc functype;                                 \
    functype *func;                                                            \
    const void *hd;                                                            \
    if (!s->handlers) return true;                                             \
    func = (functype *)upb_handlers_gethandler(s->handlers, sel);              \
    if (!func) return true;                                                    \
    hd = upb_handlers_gethandlerdata(s->handlers, sel);                        \
    return func(s->closure, hd, val);                                          \
  }

PUTVAL(int32,  int32_t)
PUTVAL(int64,  int64_t)
PUTVAL(uint32, uint32_t)
PUTVAL(uint64, uint64_t)
PUTVAL(float,  float)
PUTVAL(double, double)
PUTVAL(bool,   bool)
#undef PUTVAL

UPB_INLINE void upb_sink_reset(upb_sink *s, const upb_handlers *h, void *c) {
  s->handlers = h;
  s->closure = c;
}

UPB_INLINE size_t upb_sink_putstring(upb_sink *s, upb_selector_t sel,
                                     const char *buf, size_t n,
                                     const upb_bufhandle *handle) {
  typedef upb_string_handlerfunc func;
  func *handler;
  const void *hd;
  if (!s->handlers) return n;
  handler = (func *)upb_handlers_gethandler(s->handlers, sel);

  if (!handler) return n;
  hd = upb_handlers_gethandlerdata(s->handlers, sel);
  return handler(s->closure, hd, buf, n, handle);
}

UPB_INLINE bool upb_sink_putunknown(upb_sink *s, const char *buf, size_t n) {
  typedef upb_unknown_handlerfunc func;
  func *handler;
  const void *hd;
  if (!s->handlers) return true;
  handler = (func *)upb_handlers_gethandler(s->handlers, UPB_UNKNOWN_SELECTOR);

  if (!handler) return n;
  hd = upb_handlers_gethandlerdata(s->handlers, UPB_UNKNOWN_SELECTOR);
  return handler(s->closure, hd, buf, n);
}

UPB_INLINE bool upb_sink_startmsg(upb_sink *s) {
  typedef upb_startmsg_handlerfunc func;
  func *startmsg;
  const void *hd;
  if (!s->handlers) return true;
  startmsg = (func*)upb_handlers_gethandler(s->handlers, UPB_STARTMSG_SELECTOR);

  if (!startmsg) return true;
  hd = upb_handlers_gethandlerdata(s->handlers, UPB_STARTMSG_SELECTOR);
  return startmsg(s->closure, hd);
}

UPB_INLINE bool upb_sink_endmsg(upb_sink *s, upb_status *status) {
  typedef upb_endmsg_handlerfunc func;
  func *endmsg;
  const void *hd;
  if (!s->handlers) return true;
  endmsg = (func *)upb_handlers_gethandler(s->handlers, UPB_ENDMSG_SELECTOR);

  if (!endmsg) return true;
  hd = upb_handlers_gethandlerdata(s->handlers, UPB_ENDMSG_SELECTOR);
  return endmsg(s->closure, hd, status);
}

UPB_INLINE bool upb_sink_startseq(upb_sink *s, upb_selector_t sel,
                                  upb_sink *sub) {
  typedef upb_startfield_handlerfunc func;
  func *startseq;
  const void *hd;
  sub->closure = s->closure;
  sub->handlers = s->handlers;
  if (!s->handlers) return true;
  startseq = (func*)upb_handlers_gethandler(s->handlers, sel);

  if (!startseq) return true;
  hd = upb_handlers_gethandlerdata(s->handlers, sel);
  sub->closure = startseq(s->closure, hd);
  return sub->closure ? true : false;
}

UPB_INLINE bool upb_sink_endseq(upb_sink *s, upb_selector_t sel) {
  typedef upb_endfield_handlerfunc func;
  func *endseq;
  const void *hd;
  if (!s->handlers) return true;
  endseq = (func*)upb_handlers_gethandler(s->handlers, sel);

  if (!endseq) return true;
  hd = upb_handlers_gethandlerdata(s->handlers, sel);
  return endseq(s->closure, hd);
}

UPB_INLINE bool upb_sink_startstr(upb_sink *s, upb_selector_t sel,
                                  size_t size_hint, upb_sink *sub) {
  typedef upb_startstr_handlerfunc func;
  func *startstr;
  const void *hd;
  sub->closure = s->closure;
  sub->handlers = s->handlers;
  if (!s->handlers) return true;
  startstr = (func*)upb_handlers_gethandler(s->handlers, sel);

  if (!startstr) return true;
  hd = upb_handlers_gethandlerdata(s->handlers, sel);
  sub->closure = startstr(s->closure, hd, size_hint);
  return sub->closure ? true : false;
}

UPB_INLINE bool upb_sink_endstr(upb_sink *s, upb_selector_t sel) {
  typedef upb_endfield_handlerfunc func;
  func *endstr;
  const void *hd;
  if (!s->handlers) return true;
  endstr = (func*)upb_handlers_gethandler(s->handlers, sel);

  if (!endstr) return true;
  hd = upb_handlers_gethandlerdata(s->handlers, sel);
  return endstr(s->closure, hd);
}

UPB_INLINE bool upb_sink_startsubmsg(upb_sink *s, upb_selector_t sel,
                                     upb_sink *sub) {
  typedef upb_startfield_handlerfunc func;
  func *startsubmsg;
  const void *hd;
  sub->closure = s->closure;
  if (!s->handlers) {
    sub->handlers = NULL;
    return true;
  }
  sub->handlers = upb_handlers_getsubhandlers_sel(s->handlers, sel);
  startsubmsg = (func*)upb_handlers_gethandler(s->handlers, sel);

  if (!startsubmsg) return true;
  hd = upb_handlers_gethandlerdata(s->handlers, sel);
  sub->closure = startsubmsg(s->closure, hd);
  return sub->closure ? true : false;
}

UPB_INLINE bool upb_sink_endsubmsg(upb_sink *s, upb_selector_t sel) {
  typedef upb_endfield_handlerfunc func;
  func *endsubmsg;
  const void *hd;
  if (!s->handlers) return true;
  endsubmsg = (func*)upb_handlers_gethandler(s->handlers, sel);

  if (!endsubmsg) return s->closure;
  hd = upb_handlers_gethandlerdata(s->handlers, sel);
  return endsubmsg(s->closure, hd);
}

UPB_END_EXTERN_C

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
                                    size_t len, const BufferHandle* handle) {
  return upb_sink_putstring(this, sel, buf, len, handle);
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

template <class T>
BytesSink::BytesSink(const BytesHandler* handler, T* closure) {
  Reset(handler, closure);
}

template <class T>
void BytesSink::Reset(const BytesHandler *handler, T *closure) {
  upb_bytessink_reset(this, handler, closure);
}
inline bool BytesSink::Start(size_t size_hint, void **subc) {
  return upb_bytessink_start(this, size_hint, subc);
}
inline size_t BytesSink::PutBuffer(void *subc, const char *buf, size_t len,
                                   const BufferHandle *handle) {
  return upb_bytessink_putbuf(this, subc, buf, len, handle);
}
inline bool BytesSink::End() {
  return upb_bytessink_end(this);
}

inline bool BufferSource::PutBuffer(const char *buf, size_t len,
                                    BytesSink *sink) {
  return upb_bufsrc_putbuf(buf, len, sink);
}

}  /* namespace upb */
#endif

#endif
