/*
** upb::Handlers (upb_handlers)
**
** A upb_handlers is like a virtual table for a upb_msgdef.  Each field of the
** message can have associated functions that will be called when we are
** parsing or visiting a stream of data.  This is similar to how handlers work
** in SAX (the Simple API for XML).
**
** The handlers have no idea where the data is coming from, so a single set of
** handlers could be used with two completely different data sources (for
** example, a parser and a visitor over in-memory objects).  This decoupling is
** the most important feature of upb, because it allows parsers and serializers
** to be highly reusable.
**
** This is a mixed C/C++ interface that offers a full API to both languages.
** See the top-level README for more information.
*/

#ifndef UPB_HANDLERS_H
#define UPB_HANDLERS_H

#include "upb/def.h"
#include "upb/table.int.h"

#include "upb/port_def.inc"

#ifdef __cplusplus
namespace upb {
class HandlersPtr;
class HandlerCache;
template <class T> class Handler;
template <class T> struct CanonicalType;
}  /* namespace upb */
#endif


/* The maximum depth that the handler graph can have.  This is a resource limit
 * for the C stack since we sometimes need to recursively traverse the graph.
 * Cycles are ok; the traversal will stop when it detects a cycle, but we must
 * hit the cycle before the maximum depth is reached.
 *
 * If having a single static limit is too inflexible, we can add another variant
 * of Handlers::Freeze that allows specifying this as a parameter. */
#define UPB_MAX_HANDLER_DEPTH 64

/* All the different types of handlers that can be registered.
 * Only needed for the advanced functions in upb::Handlers. */
typedef enum {
  UPB_HANDLER_INT32,
  UPB_HANDLER_INT64,
  UPB_HANDLER_UINT32,
  UPB_HANDLER_UINT64,
  UPB_HANDLER_FLOAT,
  UPB_HANDLER_DOUBLE,
  UPB_HANDLER_BOOL,
  UPB_HANDLER_STARTSTR,
  UPB_HANDLER_STRING,
  UPB_HANDLER_ENDSTR,
  UPB_HANDLER_STARTSUBMSG,
  UPB_HANDLER_ENDSUBMSG,
  UPB_HANDLER_STARTSEQ,
  UPB_HANDLER_ENDSEQ
} upb_handlertype_t;

#define UPB_HANDLER_MAX (UPB_HANDLER_ENDSEQ+1)

#define UPB_BREAK NULL

/* A convenient definition for when no closure is needed. */
extern char _upb_noclosure;
#define UPB_NO_CLOSURE &_upb_noclosure

/* A selector refers to a specific field handler in the Handlers object
 * (for example: the STARTSUBMSG handler for field "field15"). */
typedef int32_t upb_selector_t;

/* Static selectors for upb::Handlers. */
#define UPB_STARTMSG_SELECTOR 0
#define UPB_ENDMSG_SELECTOR 1
#define UPB_UNKNOWN_SELECTOR 2
#define UPB_STATIC_SELECTOR_COUNT 3  /* Warning: also in upb/def.c. */

/* Static selectors for upb::BytesHandler. */
#define UPB_STARTSTR_SELECTOR 0
#define UPB_STRING_SELECTOR 1
#define UPB_ENDSTR_SELECTOR 2

#ifdef __cplusplus
template<class T> const void *UniquePtrForType() {
  static const char ch = 0;
  return &ch;
}
#endif

/* upb_handlers ************************************************************/

/* Handler attributes, to be registered with the handler itself. */
typedef struct {
  const void *handler_data;
  const void *closure_type;
  const void *return_closure_type;
  bool alwaysok;
} upb_handlerattr;

#define UPB_HANDLERATTR_INIT {NULL, NULL, NULL, false}

/* Bufhandle, data passed along with a buffer to indicate its provenance. */
typedef struct {
  /* The beginning of the buffer.  This may be different than the pointer
   * passed to a StringBuf handler because the handler may receive data
   * that is from the middle or end of a larger buffer. */
  const char *buf;

  /* The offset within the attached object where this buffer begins.  Only
   * meaningful if there is an attached object. */
  size_t objofs;

  /* The attached object (if any) and a pointer representing its type. */
  const void *obj;
  const void *objtype;

#ifdef __cplusplus
  template <class T>
  void SetAttachedObject(const T* _obj) {
    obj = _obj;
    objtype = UniquePtrForType<T>();
  }

  template <class T>
  const T *GetAttachedObject() const {
    return objtype == UniquePtrForType<T>() ? static_cast<const T *>(obj)
                                            : NULL;
  }
#endif
} upb_bufhandle;

#define UPB_BUFHANDLE_INIT {NULL, 0, NULL, NULL}

/* Handler function typedefs. */
typedef void upb_handlerfree(void *d);
typedef bool upb_unknown_handlerfunc(void *c, const void *hd, const char *buf,
                                     size_t n);
typedef bool upb_startmsg_handlerfunc(void *c, const void*);
typedef bool upb_endmsg_handlerfunc(void *c, const void *, upb_status *status);
typedef void* upb_startfield_handlerfunc(void *c, const void *hd);
typedef bool upb_endfield_handlerfunc(void *c, const void *hd);
typedef bool upb_int32_handlerfunc(void *c, const void *hd, int32_t val);
typedef bool upb_int64_handlerfunc(void *c, const void *hd, int64_t val);
typedef bool upb_uint32_handlerfunc(void *c, const void *hd, uint32_t val);
typedef bool upb_uint64_handlerfunc(void *c, const void *hd, uint64_t val);
typedef bool upb_float_handlerfunc(void *c, const void *hd, float val);
typedef bool upb_double_handlerfunc(void *c, const void *hd, double val);
typedef bool upb_bool_handlerfunc(void *c, const void *hd, bool val);
typedef void *upb_startstr_handlerfunc(void *c, const void *hd,
                                       size_t size_hint);
typedef size_t upb_string_handlerfunc(void *c, const void *hd, const char *buf,
                                      size_t n, const upb_bufhandle* handle);

struct upb_handlers;
typedef struct upb_handlers upb_handlers;

#ifdef __cplusplus
extern "C" {
#endif

/* Mutating accessors. */
const upb_status *upb_handlers_status(upb_handlers *h);
void upb_handlers_clearerr(upb_handlers *h);
const upb_msgdef *upb_handlers_msgdef(const upb_handlers *h);
bool upb_handlers_addcleanup(upb_handlers *h, void *p, upb_handlerfree *hfree);
bool upb_handlers_setunknown(upb_handlers *h, upb_unknown_handlerfunc *func,
                             const upb_handlerattr *attr);
bool upb_handlers_setstartmsg(upb_handlers *h, upb_startmsg_handlerfunc *func,
                              const upb_handlerattr *attr);
bool upb_handlers_setendmsg(upb_handlers *h, upb_endmsg_handlerfunc *func,
                            const upb_handlerattr *attr);
bool upb_handlers_setint32(upb_handlers *h, const upb_fielddef *f,
                           upb_int32_handlerfunc *func,
                           const upb_handlerattr *attr);
bool upb_handlers_setint64(upb_handlers *h, const upb_fielddef *f,
                           upb_int64_handlerfunc *func,
                           const upb_handlerattr *attr);
bool upb_handlers_setuint32(upb_handlers *h, const upb_fielddef *f,
                            upb_uint32_handlerfunc *func,
                            const upb_handlerattr *attr);
bool upb_handlers_setuint64(upb_handlers *h, const upb_fielddef *f,
                            upb_uint64_handlerfunc *func,
                            const upb_handlerattr *attr);
bool upb_handlers_setfloat(upb_handlers *h, const upb_fielddef *f,
                           upb_float_handlerfunc *func,
                           const upb_handlerattr *attr);
bool upb_handlers_setdouble(upb_handlers *h, const upb_fielddef *f,
                            upb_double_handlerfunc *func,
                            const upb_handlerattr *attr);
bool upb_handlers_setbool(upb_handlers *h, const upb_fielddef *f,
                          upb_bool_handlerfunc *func,
                          const upb_handlerattr *attr);
bool upb_handlers_setstartstr(upb_handlers *h, const upb_fielddef *f,
                              upb_startstr_handlerfunc *func,
                              const upb_handlerattr *attr);
bool upb_handlers_setstring(upb_handlers *h, const upb_fielddef *f,
                            upb_string_handlerfunc *func,
                            const upb_handlerattr *attr);
bool upb_handlers_setendstr(upb_handlers *h, const upb_fielddef *f,
                            upb_endfield_handlerfunc *func,
                            const upb_handlerattr *attr);
bool upb_handlers_setstartseq(upb_handlers *h, const upb_fielddef *f,
                              upb_startfield_handlerfunc *func,
                              const upb_handlerattr *attr);
bool upb_handlers_setstartsubmsg(upb_handlers *h, const upb_fielddef *f,
                                 upb_startfield_handlerfunc *func,
                                 const upb_handlerattr *attr);
bool upb_handlers_setendsubmsg(upb_handlers *h, const upb_fielddef *f,
                               upb_endfield_handlerfunc *func,
                               const upb_handlerattr *attr);
bool upb_handlers_setendseq(upb_handlers *h, const upb_fielddef *f,
                            upb_endfield_handlerfunc *func,
                            const upb_handlerattr *attr);

/* Read-only accessors. */
const upb_handlers *upb_handlers_getsubhandlers(const upb_handlers *h,
                                                const upb_fielddef *f);
const upb_handlers *upb_handlers_getsubhandlers_sel(const upb_handlers *h,
                                                    upb_selector_t sel);
upb_func *upb_handlers_gethandler(const upb_handlers *h, upb_selector_t s,
                                  const void **handler_data);
bool upb_handlers_getattr(const upb_handlers *h, upb_selector_t s,
                          upb_handlerattr *attr);

/* "Static" methods */
upb_handlertype_t upb_handlers_getprimitivehandlertype(const upb_fielddef *f);
bool upb_handlers_getselector(const upb_fielddef *f, upb_handlertype_t type,
                              upb_selector_t *s);
UPB_INLINE upb_selector_t upb_handlers_getendselector(upb_selector_t start) {
  return start + 1;
}

#ifdef __cplusplus
}  /* extern "C" */

namespace upb {
typedef upb_handlers Handlers;
}

/* Convenience macros for creating a Handler object that is wrapped with a
 * type-safe wrapper function that converts the "void*" parameters/returns
 * of the underlying C API into nice C++ function.
 *
 * Sample usage:
 *   void OnValue1(MyClosure* c, const MyHandlerData* d, int32_t val) {
 *     // do stuff ...
 *   }
 *
 *   // Handler that doesn't need any data bound to it.
 *   void OnValue2(MyClosure* c, int32_t val) {
 *     // do stuff ...
 *   }
 *
 *   // Handler that returns bool so it can return failure if necessary.
 *   bool OnValue3(MyClosure* c, int32_t val) {
 *     // do stuff ...
 *     return ok;
 *   }
 *
 *   // Member function handler.
 *   class MyClosure {
 *    public:
 *     void OnValue(int32_t val) {
 *       // do stuff ...
 *     }
 *   };
 *
 *   // Takes ownership of the MyHandlerData.
 *   handlers->SetInt32Handler(f1, UpbBind(OnValue1, new MyHandlerData(...)));
 *   handlers->SetInt32Handler(f2, UpbMakeHandler(OnValue2));
 *   handlers->SetInt32Handler(f1, UpbMakeHandler(OnValue3));
 *   handlers->SetInt32Handler(f2, UpbMakeHandler(&MyClosure::OnValue));
 */

/* In C++11, the "template" disambiguator can appear even outside templates,
 * so all calls can safely use this pair of macros. */

#define UpbMakeHandler(f) upb::MatchFunc(f).template GetFunc<f>()

/* We have to be careful to only evaluate "d" once. */
#define UpbBind(f, d) upb::MatchFunc(f).template GetFunc<f>((d))

/* Handler: a struct that contains the (handler, data, deleter) tuple that is
 * used to register all handlers.  Users can Make() these directly but it's
 * more convenient to use the UpbMakeHandler/UpbBind macros above. */
template <class T> class upb::Handler {
 public:
  /* The underlying, handler function signature that upb uses internally. */
  typedef T FuncPtr;

  /* Intentionally implicit. */
  template <class F> Handler(F func);
  ~Handler() { UPB_ASSERT(registered_); }

  void AddCleanup(upb_handlers* h) const;
  FuncPtr handler() const { return handler_; }
  const upb_handlerattr& attr() const { return attr_; }

 private:
  Handler(const Handler&) = delete;
  Handler& operator=(const Handler&) = delete;

  FuncPtr handler_;
  mutable upb_handlerattr attr_;
  mutable bool registered_;
  void *cleanup_data_;
  upb_handlerfree *cleanup_func_;
};

/* A upb::Handlers object represents the set of handlers associated with a
 * message in the graph of messages.  You can think of it as a big virtual
 * table with functions corresponding to all the events that can fire while
 * parsing or visiting a message of a specific type.
 *
 * Any handlers that are not set behave as if they had successfully consumed
 * the value.  Any unset Start* handlers will propagate their closure to the
 * inner frame.
 *
 * The easiest way to create the *Handler objects needed by the Set* methods is
 * with the UpbBind() and UpbMakeHandler() macros; see below. */
class upb::HandlersPtr {
 public:
  HandlersPtr(upb_handlers* ptr) : ptr_(ptr) {}

  upb_handlers* ptr() const { return ptr_; }

  typedef upb_selector_t Selector;
  typedef upb_handlertype_t Type;

  typedef Handler<void *(*)(void *, const void *)> StartFieldHandler;
  typedef Handler<bool (*)(void *, const void *)> EndFieldHandler;
  typedef Handler<bool (*)(void *, const void *)> StartMessageHandler;
  typedef Handler<bool (*)(void *, const void *, upb_status *)>
      EndMessageHandler;
  typedef Handler<void *(*)(void *, const void *, size_t)> StartStringHandler;
  typedef Handler<size_t (*)(void *, const void *, const char *, size_t,
                             const upb_bufhandle *)>
      StringHandler;

  template <class T> struct ValueHandler {
    typedef Handler<bool(*)(void *, const void *, T)> H;
  };

  typedef ValueHandler<int32_t>::H     Int32Handler;
  typedef ValueHandler<int64_t>::H     Int64Handler;
  typedef ValueHandler<uint32_t>::H    UInt32Handler;
  typedef ValueHandler<uint64_t>::H    UInt64Handler;
  typedef ValueHandler<float>::H       FloatHandler;
  typedef ValueHandler<double>::H      DoubleHandler;
  typedef ValueHandler<bool>::H        BoolHandler;

  /* Any function pointer can be converted to this and converted back to its
   * correct type. */
  typedef void GenericFunction();

  typedef void HandlersCallback(const void *closure, upb_handlers *h);

  /* Returns the msgdef associated with this handlers object. */
  MessageDefPtr message_def() const {
    return MessageDefPtr(upb_handlers_msgdef(ptr()));
  }

  /* Adds the given pointer and function to the list of cleanup functions that
   * will be run when these handlers are freed.  If this pointer has previously
   * been registered, the function returns false and does nothing. */
  bool AddCleanup(void *ptr, upb_handlerfree *cleanup) {
    return upb_handlers_addcleanup(ptr_, ptr, cleanup);
  }

  /* Sets the startmsg handler for the message, which is defined as follows:
   *
   *   bool startmsg(MyType* closure) {
   *     // Called when the message begins.  Returns true if processing should
   *     // continue.
   *     return true;
   *   }
   */
  bool SetStartMessageHandler(const StartMessageHandler &h) {
    h.AddCleanup(ptr());
    return upb_handlers_setstartmsg(ptr(), h.handler(), &h.attr());
  }

  /* Sets the endmsg handler for the message, which is defined as follows:
   *
   *   bool endmsg(MyType* closure, upb_status *status) {
   *     // Called when processing of this message ends, whether in success or
   *     // failure.  "status" indicates the final status of processing, and
   *     // can also be modified in-place to update the final status.
   *   }
   */
  bool SetEndMessageHandler(const EndMessageHandler& h) {
    h.AddCleanup(ptr());
    return upb_handlers_setendmsg(ptr(), h.handler(), &h.attr());
  }

  /* Sets the value handler for the given field, which is defined as follows
   * (this is for an int32 field; other field types will pass their native
   * C/C++ type for "val"):
   *
   *   bool OnValue(MyClosure* c, const MyHandlerData* d, int32_t val) {
   *     // Called when the field's value is encountered.  "d" contains
   *     // whatever data was bound to this field when it was registered.
   *     // Returns true if processing should continue.
   *     return true;
   *   }
   *
   *   handers->SetInt32Handler(f, UpbBind(OnValue, new MyHandlerData(...)));
   *
   * The value type must exactly match f->type().
   * For example, a handler that takes an int32_t parameter may only be used for
   * fields of type UPB_TYPE_INT32 and UPB_TYPE_ENUM.
   *
   * Returns false if the handler failed to register; in this case the cleanup
   * handler (if any) will be called immediately.
   */
  bool SetInt32Handler(FieldDefPtr f, const Int32Handler &h) {
    h.AddCleanup(ptr());
    return upb_handlers_setint32(ptr(), f.ptr(), h.handler(), &h.attr());
  }

  bool SetInt64Handler (FieldDefPtr f,  const Int64Handler& h) {
    h.AddCleanup(ptr());
    return upb_handlers_setint64(ptr(), f.ptr(), h.handler(), &h.attr());
  }

  bool SetUInt32Handler(FieldDefPtr f, const UInt32Handler& h) {
    h.AddCleanup(ptr());
    return upb_handlers_setuint32(ptr(), f.ptr(), h.handler(), &h.attr());
  }

  bool SetUInt64Handler(FieldDefPtr f, const UInt64Handler& h) {
    h.AddCleanup(ptr());
    return upb_handlers_setuint64(ptr(), f.ptr(), h.handler(), &h.attr());
  }

  bool SetFloatHandler (FieldDefPtr f,  const FloatHandler& h) {
    h.AddCleanup(ptr());
    return upb_handlers_setfloat(ptr(), f.ptr(), h.handler(), &h.attr());
  }

  bool SetDoubleHandler(FieldDefPtr f, const DoubleHandler& h) {
    h.AddCleanup(ptr());
    return upb_handlers_setdouble(ptr(), f.ptr(), h.handler(), &h.attr());
  }

  bool SetBoolHandler(FieldDefPtr f, const BoolHandler &h) {
    h.AddCleanup(ptr());
    return upb_handlers_setbool(ptr(), f.ptr(), h.handler(), &h.attr());
  }

  /* Like the previous, but templated on the type on the value (ie. int32).
   * This is mostly useful to call from other templates.  To call this you must
   * specify the template parameter explicitly, ie:
   *   h->SetValueHandler<T>(f, UpbBind(MyHandler<T>, MyData)); */
  template <class T>
  bool SetValueHandler(
      FieldDefPtr f,
      const typename ValueHandler<typename CanonicalType<T>::Type>::H &handler);

  /* Sets handlers for a string field, which are defined as follows:
   *
   *   MySubClosure* startstr(MyClosure* c, const MyHandlerData* d,
   *                          size_t size_hint) {
   *     // Called when a string value begins.  The return value indicates the
   *     // closure for the string.  "size_hint" indicates the size of the
   *     // string if it is known, however if the string is length-delimited
   *     // and the end-of-string is not available size_hint will be zero.
   *     // This case is indistinguishable from the case where the size is
   *     // known to be zero.
   *     //
   *     // TODO(haberman): is it important to distinguish these cases?
   *     // If we had ssize_t as a type we could make -1 "unknown", but
   *     // ssize_t is POSIX (not ANSI) and therefore less portable.
   *     // In practice I suspect it won't be important to distinguish.
   *     return closure;
   *   }
   *
   *   size_t str(MyClosure* closure, const MyHandlerData* d,
   *              const char *str, size_t len) {
   *     // Called for each buffer of string data; the multiple physical buffers
   *     // are all part of the same logical string.  The return value indicates
   *     // how many bytes were consumed.  If this number is less than "len",
   *     // this will also indicate that processing should be halted for now,
   *     // like returning false or UPB_BREAK from any other callback.  If
   *     // number is greater than "len", the excess bytes will be skipped over
   *     // and not passed to the callback.
   *     return len;
   *   }
   *
   *   bool endstr(MyClosure* c, const MyHandlerData* d) {
   *     // Called when a string value ends.  Return value indicates whether
   *     // processing should continue.
   *     return true;
   *   }
   */
  bool SetStartStringHandler(FieldDefPtr f, const StartStringHandler &h) {
    h.AddCleanup(ptr());
    return upb_handlers_setstartstr(ptr(), f.ptr(), h.handler(), &h.attr());
  }

  bool SetStringHandler(FieldDefPtr f, const StringHandler& h) {
    h.AddCleanup(ptr());
    return upb_handlers_setstring(ptr(), f.ptr(), h.handler(), &h.attr());
  }

  bool SetEndStringHandler(FieldDefPtr f, const EndFieldHandler& h) {
    h.AddCleanup(ptr());
    return upb_handlers_setendstr(ptr(), f.ptr(), h.handler(), &h.attr());
  }

  /* Sets the startseq handler, which is defined as follows:
   *
   *   MySubClosure *startseq(MyClosure* c, const MyHandlerData* d) {
   *     // Called when a sequence (repeated field) begins.  The returned
   *     // pointer indicates the closure for the sequence (or UPB_BREAK
   *     // to interrupt processing).
   *     return closure;
   *   }
   *
   *   h->SetStartSequenceHandler(f, UpbBind(startseq, new MyHandlerData(...)));
   *
   * Returns "false" if "f" does not belong to this message or is not a
   * repeated field.
   */
  bool SetStartSequenceHandler(FieldDefPtr f, const StartFieldHandler &h) {
    h.AddCleanup(ptr());
    return upb_handlers_setstartseq(ptr(), f.ptr(), h.handler(), &h.attr());
  }

  /* Sets the startsubmsg handler for the given field, which is defined as
   * follows:
   *
   *   MySubClosure* startsubmsg(MyClosure* c, const MyHandlerData* d) {
   *     // Called when a submessage begins.  The returned pointer indicates the
   *     // closure for the sequence (or UPB_BREAK to interrupt processing).
   *     return closure;
   *   }
   *
   *   h->SetStartSubMessageHandler(f, UpbBind(startsubmsg,
   *                                           new MyHandlerData(...)));
   *
   * Returns "false" if "f" does not belong to this message or is not a
   * submessage/group field.
   */
  bool SetStartSubMessageHandler(FieldDefPtr f, const StartFieldHandler& h) {
    h.AddCleanup(ptr());
    return upb_handlers_setstartsubmsg(ptr(), f.ptr(), h.handler(), &h.attr());
  }

  /* Sets the endsubmsg handler for the given field, which is defined as
   * follows:
   *
   *   bool endsubmsg(MyClosure* c, const MyHandlerData* d) {
   *     // Called when a submessage ends.  Returns true to continue processing.
   *     return true;
   *   }
   *
   * Returns "false" if "f" does not belong to this message or is not a
   * submessage/group field.
   */
  bool SetEndSubMessageHandler(FieldDefPtr f, const EndFieldHandler &h) {
    h.AddCleanup(ptr());
    return upb_handlers_setendsubmsg(ptr(), f.ptr(), h.handler(), &h.attr());
  }

  /* Starts the endsubseq handler for the given field, which is defined as
   * follows:
   *
   *   bool endseq(MyClosure* c, const MyHandlerData* d) {
   *     // Called when a sequence ends.  Returns true continue processing.
   *     return true;
   *   }
   *
   * Returns "false" if "f" does not belong to this message or is not a
   * repeated field.
   */
  bool SetEndSequenceHandler(FieldDefPtr f, const EndFieldHandler &h) {
    h.AddCleanup(ptr());
    return upb_handlers_setendseq(ptr(), f.ptr(), h.handler(), &h.attr());
  }

 private:
  upb_handlers* ptr_;
};

#endif  /* __cplusplus */

/* upb_handlercache ***********************************************************/

/* A upb_handlercache lazily builds and caches upb_handlers.  You pass it a
 * function (with optional closure) that can build handlers for a given
 * message on-demand, and the cache maintains a map of msgdef->handlers. */

#ifdef __cplusplus
extern "C" {
#endif

struct upb_handlercache;
typedef struct upb_handlercache upb_handlercache;

typedef void upb_handlers_callback(const void *closure, upb_handlers *h);

upb_handlercache *upb_handlercache_new(upb_handlers_callback *callback,
                                       const void *closure);
void upb_handlercache_free(upb_handlercache *cache);
const upb_handlers *upb_handlercache_get(upb_handlercache *cache,
                                         const upb_msgdef *md);
bool upb_handlercache_addcleanup(upb_handlercache *h, void *p,
                                 upb_handlerfree *hfree);

#ifdef __cplusplus
}  /* extern "C" */

class upb::HandlerCache {
 public:
  HandlerCache(upb_handlers_callback *callback, const void *closure)
      : ptr_(upb_handlercache_new(callback, closure), upb_handlercache_free) {}
  HandlerCache(HandlerCache&&) = default;
  HandlerCache& operator=(HandlerCache&&) = default;
  HandlerCache(upb_handlercache* c) : ptr_(c, upb_handlercache_free) {}

  upb_handlercache* ptr() { return ptr_.get(); }

  const upb_handlers *Get(MessageDefPtr md) {
    return upb_handlercache_get(ptr_.get(), md.ptr());
  }

 private:
  std::unique_ptr<upb_handlercache, decltype(&upb_handlercache_free)> ptr_;
};

#endif  /* __cplusplus */

/* upb_byteshandler ***********************************************************/

typedef struct {
  upb_func *func;

  /* It is wasteful to include the entire attributes here:
   *
   * * Some of the information is redundant (like storing the closure type
   *   separately for each handler that must match).
   * * Some of the info is only needed prior to freeze() (like closure types).
   * * alignment padding wastes a lot of space for alwaysok_.
   *
   * If/when the size and locality of handlers is an issue, we can optimize this
   * not to store the entire attr like this.  We do not expose the table's
   * layout to allow this optimization in the future. */
  upb_handlerattr attr;
} upb_handlers_tabent;

#define UPB_TABENT_INIT {NULL, UPB_HANDLERATTR_INIT}

typedef struct {
  upb_handlers_tabent table[3];
} upb_byteshandler;

#define UPB_BYTESHANDLER_INIT                             \
  {                                                       \
    { UPB_TABENT_INIT, UPB_TABENT_INIT, UPB_TABENT_INIT } \
  }

UPB_INLINE void upb_byteshandler_init(upb_byteshandler *handler) {
  upb_byteshandler init = UPB_BYTESHANDLER_INIT;
  *handler = init;
}

#ifdef __cplusplus
extern "C" {
#endif

/* Caller must ensure that "d" outlives the handlers. */
bool upb_byteshandler_setstartstr(upb_byteshandler *h,
                                  upb_startstr_handlerfunc *func, void *d);
bool upb_byteshandler_setstring(upb_byteshandler *h,
                                upb_string_handlerfunc *func, void *d);
bool upb_byteshandler_setendstr(upb_byteshandler *h,
                                upb_endfield_handlerfunc *func, void *d);

#ifdef __cplusplus
}  /* extern "C" */

namespace upb {
typedef upb_byteshandler BytesHandler;
}
#endif

/** Message handlers ******************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

/* These are the handlers used internally by upb_msgfactory_getmergehandlers().
 * They write scalar data to a known offset from the message pointer.
 *
 * These would be trivial for anyone to implement themselves, but it's better
 * to use these because some JITs will recognize and specialize these instead
 * of actually calling the function. */

/* Sets a handler for the given primitive field that will write the data at the
 * given offset.  If hasbit > 0, also sets a hasbit at the given bit offset
 * (addressing each byte low to high). */
bool upb_msg_setscalarhandler(upb_handlers *h,
                              const upb_fielddef *f,
                              size_t offset,
                              int32_t hasbit);

/* If the given handler is a msghandlers_primitive field, returns true and sets
 * *type, *offset and *hasbit.  Otherwise returns false. */
bool upb_msg_getscalarhandlerdata(const upb_handlers *h,
                                  upb_selector_t s,
                                  upb_fieldtype_t *type,
                                  size_t *offset,
                                  int32_t *hasbit);



#ifdef __cplusplus
}  /* extern "C" */
#endif

#include "upb/port_undef.inc"

#include "upb/handlers-inl.h"

#endif  /* UPB_HANDLERS_H */
