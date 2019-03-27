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

#include "upb/port_def.inc"

#ifdef __cplusplus
namespace upb {
class BytesSink;
class Sink;
}
#endif

/* upb_sink *******************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  const upb_handlers *handlers;
  void *closure;
} upb_sink;

#define PUTVAL(type, ctype)                                           \
  UPB_INLINE bool upb_sink_put##type(upb_sink s, upb_selector_t sel,  \
                                     ctype val) {                     \
    typedef upb_##type##_handlerfunc functype;                        \
    functype *func;                                                   \
    const void *hd;                                                   \
    if (!s.handlers) return true;                                     \
    func = (functype *)upb_handlers_gethandler(s.handlers, sel, &hd); \
    if (!func) return true;                                           \
    return func(s.closure, hd, val);                                  \
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

UPB_INLINE size_t upb_sink_putstring(upb_sink s, upb_selector_t sel,
                                     const char *buf, size_t n,
                                     const upb_bufhandle *handle) {
  typedef upb_string_handlerfunc func;
  func *handler;
  const void *hd;
  if (!s.handlers) return n;
  handler = (func *)upb_handlers_gethandler(s.handlers, sel, &hd);

  if (!handler) return n;
  return handler(s.closure, hd, buf, n, handle);
}

UPB_INLINE bool upb_sink_putunknown(upb_sink s, const char *buf, size_t n) {
  typedef upb_unknown_handlerfunc func;
  func *handler;
  const void *hd;
  if (!s.handlers) return true;
  handler =
      (func *)upb_handlers_gethandler(s.handlers, UPB_UNKNOWN_SELECTOR, &hd);

  if (!handler) return n;
  return handler(s.closure, hd, buf, n);
}

UPB_INLINE bool upb_sink_startmsg(upb_sink s) {
  typedef upb_startmsg_handlerfunc func;
  func *startmsg;
  const void *hd;
  if (!s.handlers) return true;
  startmsg =
      (func *)upb_handlers_gethandler(s.handlers, UPB_STARTMSG_SELECTOR, &hd);

  if (!startmsg) return true;
  return startmsg(s.closure, hd);
}

UPB_INLINE bool upb_sink_endmsg(upb_sink s, upb_status *status) {
  typedef upb_endmsg_handlerfunc func;
  func *endmsg;
  const void *hd;
  if (!s.handlers) return true;
  endmsg =
      (func *)upb_handlers_gethandler(s.handlers, UPB_ENDMSG_SELECTOR, &hd);

  if (!endmsg) return true;
  return endmsg(s.closure, hd, status);
}

UPB_INLINE bool upb_sink_startseq(upb_sink s, upb_selector_t sel,
                                  upb_sink *sub) {
  typedef upb_startfield_handlerfunc func;
  func *startseq;
  const void *hd;
  sub->closure = s.closure;
  sub->handlers = s.handlers;
  if (!s.handlers) return true;
  startseq = (func*)upb_handlers_gethandler(s.handlers, sel, &hd);

  if (!startseq) return true;
  sub->closure = startseq(s.closure, hd);
  return sub->closure ? true : false;
}

UPB_INLINE bool upb_sink_endseq(upb_sink s, upb_selector_t sel) {
  typedef upb_endfield_handlerfunc func;
  func *endseq;
  const void *hd;
  if (!s.handlers) return true;
  endseq = (func*)upb_handlers_gethandler(s.handlers, sel, &hd);

  if (!endseq) return true;
  return endseq(s.closure, hd);
}

UPB_INLINE bool upb_sink_startstr(upb_sink s, upb_selector_t sel,
                                  size_t size_hint, upb_sink *sub) {
  typedef upb_startstr_handlerfunc func;
  func *startstr;
  const void *hd;
  sub->closure = s.closure;
  sub->handlers = s.handlers;
  if (!s.handlers) return true;
  startstr = (func*)upb_handlers_gethandler(s.handlers, sel, &hd);

  if (!startstr) return true;
  sub->closure = startstr(s.closure, hd, size_hint);
  return sub->closure ? true : false;
}

UPB_INLINE bool upb_sink_endstr(upb_sink s, upb_selector_t sel) {
  typedef upb_endfield_handlerfunc func;
  func *endstr;
  const void *hd;
  if (!s.handlers) return true;
  endstr = (func*)upb_handlers_gethandler(s.handlers, sel, &hd);

  if (!endstr) return true;
  return endstr(s.closure, hd);
}

UPB_INLINE bool upb_sink_startsubmsg(upb_sink s, upb_selector_t sel,
                                     upb_sink *sub) {
  typedef upb_startfield_handlerfunc func;
  func *startsubmsg;
  const void *hd;
  sub->closure = s.closure;
  if (!s.handlers) {
    sub->handlers = NULL;
    return true;
  }
  sub->handlers = upb_handlers_getsubhandlers_sel(s.handlers, sel);
  startsubmsg = (func*)upb_handlers_gethandler(s.handlers, sel, &hd);

  if (!startsubmsg) return true;
  sub->closure = startsubmsg(s.closure, hd);
  return sub->closure ? true : false;
}

UPB_INLINE bool upb_sink_endsubmsg(upb_sink s, upb_selector_t sel) {
  typedef upb_endfield_handlerfunc func;
  func *endsubmsg;
  const void *hd;
  if (!s.handlers) return true;
  endsubmsg = (func*)upb_handlers_gethandler(s.handlers, sel, &hd);

  if (!endsubmsg) return s.closure;
  return endsubmsg(s.closure, hd);
}

#ifdef __cplusplus
}  /* extern "C" */

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

  Sink(const Sink&) = default;
  Sink& operator=(const Sink&) = default;

  Sink(const upb_sink& sink) : sink_(sink) {}
  Sink &operator=(const upb_sink &sink) {
    sink_ = sink;
    return *this;
  }

  upb_sink sink() { return sink_; }

  /* Constructs a new sink for the given frozen handlers and closure.
   *
   * TODO: once the Handlers know the expected closure type, verify that T
   * matches it. */
  template <class T> Sink(const upb_handlers* handlers, T* closure) {
    Reset(handlers, closure);
  }

  upb_sink* ptr() { return &sink_; }

  /* Resets the value of the sink. */
  template <class T> void Reset(const upb_handlers* handlers, T* closure) {
    upb_sink_reset(&sink_, handlers, closure);
  }

  /* Returns the top-level object that is bound to this sink.
   *
   * TODO: once the Handlers know the expected closure type, verify that T
   * matches it. */
  template <class T> T* GetObject() const {
    return static_cast<T*>(sink_.closure);
  }

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
  bool StartMessage() { return upb_sink_startmsg(sink_); }
  bool EndMessage(upb_status *status) {
    return upb_sink_endmsg(sink_, status);
  }

  /* Putting of individual values.  These work for both repeated and
   * non-repeated fields, but for repeated fields you must wrap them in
   * calls to StartSequence()/EndSequence(). */
  bool PutInt32(HandlersPtr::Selector s, int32_t val) {
    return upb_sink_putint32(sink_, s, val);
  }

  bool PutInt64(HandlersPtr::Selector s, int64_t val) {
    return upb_sink_putint64(sink_, s, val);
  }

  bool PutUInt32(HandlersPtr::Selector s, uint32_t val) {
    return upb_sink_putuint32(sink_, s, val);
  }

  bool PutUInt64(HandlersPtr::Selector s, uint64_t val) {
    return upb_sink_putuint64(sink_, s, val);
  }

  bool PutFloat(HandlersPtr::Selector s, float val) {
    return upb_sink_putfloat(sink_, s, val);
  }

  bool PutDouble(HandlersPtr::Selector s, double val) {
    return upb_sink_putdouble(sink_, s, val);
  }

  bool PutBool(HandlersPtr::Selector s, bool val) {
    return upb_sink_putbool(sink_, s, val);
  }

  /* Putting of string/bytes values.  Each string can consist of zero or more
   * non-contiguous buffers of data.
   *
   * For StartString(), the function will write a sink for the string to "sub."
   * The sub-sink must be used for any/all PutStringBuffer() calls. */
  bool StartString(HandlersPtr::Selector s, size_t size_hint, Sink* sub) {
    upb_sink sub_c;
    bool ret = upb_sink_startstr(sink_, s, size_hint, &sub_c);
    *sub = sub_c;
    return ret;
  }

  size_t PutStringBuffer(HandlersPtr::Selector s, const char *buf, size_t len,
                         const upb_bufhandle *handle) {
    return upb_sink_putstring(sink_, s, buf, len, handle);
  }

  bool EndString(HandlersPtr::Selector s) {
    return upb_sink_endstr(sink_, s);
  }

  /* For submessage fields.
   *
   * For StartSubMessage(), the function will write a sink for the string to
   * "sub." The sub-sink must be used for any/all handlers called within the
   * submessage. */
  bool StartSubMessage(HandlersPtr::Selector s, Sink* sub) {
    upb_sink sub_c;
    bool ret = upb_sink_startsubmsg(sink_, s, &sub_c);
    *sub = sub_c;
    return ret;
  }

  bool EndSubMessage(HandlersPtr::Selector s) {
    return upb_sink_endsubmsg(sink_, s);
  }

  /* For repeated fields of any type, the sequence of values must be wrapped in
   * these calls.
   *
   * For StartSequence(), the function will write a sink for the string to
   * "sub." The sub-sink must be used for any/all handlers called within the
   * sequence. */
  bool StartSequence(HandlersPtr::Selector s, Sink* sub) {
    upb_sink sub_c;
    bool ret = upb_sink_startseq(sink_, s, &sub_c);
    *sub = sub_c;
    return ret;
  }

  bool EndSequence(HandlersPtr::Selector s) {
    return upb_sink_endseq(sink_, s);
  }

  /* Copy and assign specifically allowed.
   * We don't even bother making these members private because so many
   * functions need them and this is mainly just a dumb data container anyway.
   */

 private:
  upb_sink sink_;
};

#endif  /* __cplusplus */

/* upb_bytessink **************************************************************/

typedef struct {
  const upb_byteshandler *handler;
  void *closure;
} upb_bytessink ;

UPB_INLINE void upb_bytessink_reset(upb_bytessink* s, const upb_byteshandler *h,
                                    void *closure) {
  s->handler = h;
  s->closure = closure;
}

UPB_INLINE bool upb_bytessink_start(upb_bytessink s, size_t size_hint,
                                    void **subc) {
  typedef upb_startstr_handlerfunc func;
  func *start;
  *subc = s.closure;
  if (!s.handler) return true;
  start = (func *)s.handler->table[UPB_STARTSTR_SELECTOR].func;

  if (!start) return true;
  *subc = start(s.closure,
                s.handler->table[UPB_STARTSTR_SELECTOR].attr.handler_data,
                size_hint);
  return *subc != NULL;
}

UPB_INLINE size_t upb_bytessink_putbuf(upb_bytessink s, void *subc,
                                       const char *buf, size_t size,
                                       const upb_bufhandle* handle) {
  typedef upb_string_handlerfunc func;
  func *putbuf;
  if (!s.handler) return true;
  putbuf = (func *)s.handler->table[UPB_STRING_SELECTOR].func;

  if (!putbuf) return true;
  return putbuf(subc, s.handler->table[UPB_STRING_SELECTOR].attr.handler_data,
                buf, size, handle);
}

UPB_INLINE bool upb_bytessink_end(upb_bytessink s) {
  typedef upb_endfield_handlerfunc func;
  func *end;
  if (!s.handler) return true;
  end = (func *)s.handler->table[UPB_ENDSTR_SELECTOR].func;

  if (!end) return true;
  return end(s.closure,
             s.handler->table[UPB_ENDSTR_SELECTOR].attr.handler_data);
}

#ifdef __cplusplus

class upb::BytesSink {
 public:
  BytesSink() {}

  BytesSink(const BytesSink&) = default;
  BytesSink& operator=(const BytesSink&) = default;

  BytesSink(const upb_bytessink& sink) : sink_(sink) {}
  BytesSink &operator=(const upb_bytessink &sink) {
    sink_ = sink;
    return *this;
  }

  upb_bytessink sink() { return sink_; }

  /* Constructs a new sink for the given frozen handlers and closure.
   *
   * TODO(haberman): once the Handlers know the expected closure type, verify
   * that T matches it. */
  template <class T> BytesSink(const upb_byteshandler* handler, T* closure) {
    upb_bytessink_reset(sink_, handler, closure);
  }

  /* Resets the value of the sink. */
  template <class T> void Reset(const upb_byteshandler* handler, T* closure) {
    upb_bytessink_reset(&sink_, handler, closure);
  }

  bool Start(size_t size_hint, void **subc) {
    return upb_bytessink_start(sink_, size_hint, subc);
  }

  size_t PutBuffer(void *subc, const char *buf, size_t len,
                   const upb_bufhandle *handle) {
    return upb_bytessink_putbuf(sink_, subc, buf, len, handle);
  }

  bool End() {
    return upb_bytessink_end(sink_);
  }

 private:
  upb_bytessink sink_;
};

#endif  /* __cplusplus */

/* upb_bufsrc *****************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

bool upb_bufsrc_putbuf(const char *buf, size_t len, upb_bytessink sink);

#ifdef __cplusplus
}  /* extern "C" */

namespace upb {
template <class T> bool PutBuffer(const T& str, BytesSink sink) {
  return upb_bufsrc_putbuf(str.data(), str.size(), sink.sink());
}
}

#endif  /* __cplusplus */

#include "upb/port_undef.inc"

#endif
