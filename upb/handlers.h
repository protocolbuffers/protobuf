/*
 * upb - a minimalist implementation of protocol buffers.
 *
 * Copyright (c) 2010-2012 Google Inc.  See LICENSE for details.
 * Author: Josh Haberman <jhaberman@gmail.com>
 *
 * A upb_handlers is like a virtual table for a upb_msgdef.  Each field of the
 * message can have associated functions that will be called when we are
 * parsing or visiting a stream of data.  This is similar to how handlers work
 * in SAX (the Simple API for XML).
 *
 * The handlers have no idea where the data is coming from, so a single set of
 * handlers could be used with two completely different data sources (for
 * example, a parser and a visitor over in-memory objects).  This decoupling is
 * the most important feature of upb, because it allows parsers and serializers
 * to be highly reusable.
 *
 * This is a mixed C/C++ interface that offers a full API to both languages.
 * See the top-level README for more information.
 */

#ifndef UPB_HANDLERS_H
#define UPB_HANDLERS_H

#include "upb/def.h"

#ifdef __cplusplus
namespace upb { class Handlers; }
typedef upb::Handlers upb_handlers;
#else
struct upb_handlers;
typedef struct upb_handlers upb_handlers;
#endif

// All the different types of handlers that can be registered.
// Only needed for the advanced functions in upb::Handlers.
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
  UPB_HANDLER_ENDSEQ,
} upb_handlertype_t;

#define UPB_HANDLER_MAX (UPB_HANDLER_ENDSEQ+1)

#define UPB_BREAK NULL

// A convenient definition for when no closure is needed.
extern char _upb_noclosure;
#define UPB_NO_CLOSURE &_upb_noclosure

// A selector refers to a specific field handler in the Handlers object
// (for example: the STARTSUBMSG handler for field "field15").
typedef uint32_t upb_selector_t;

#ifdef __cplusplus

// A upb::Handlers object represents the set of handlers associated with a
// message in the graph of messages.  You can think of it as a big virtual
// table with functions corresponding to all the events that can fire while
// parsing or visiting a message of a specific type.
//
// Any handlers that are not set behave as if they had successfully consumed
// the value.  For start* handlers that return a void* closure, an unset handler
// will propagate the existing closure.
class upb::Handlers {
 public:
  typedef upb_selector_t Selector;
  typedef upb_handlertype_t Type;

  typedef bool   StartMessageHandler(void* closure);
  typedef void   EndMessageHandler(void* closure, Status* status);
  typedef void*  StartFieldHandler(void* closure, void* data);
  typedef bool   EndFieldHandler(void *closure, void *data);
  typedef void*  StartStringHandler(void *c, void *d, size_t size_hint);
  typedef size_t StringHandler(void *c, void *d, const char *buf, size_t len);

  template <class T> struct Value {
    typedef bool Handler(void* closure, void* data, T val);
  };

  typedef Value<int32_t>::Handler     Int32Handler;
  typedef Value<int64_t>::Handler     Int64Handler;
  typedef Value<uint32_t>::Handler    Uint32Handler;
  typedef Value<uint64_t>::Handler    Uint64Handler;
  typedef Value<float>::Handler       FloatHandler;
  typedef Value<double>::Handler      DoubleHandler;
  typedef Value<bool>::Handler        BoolHandler;

  // Any function pointer can be converted to this and converted back to its
  // correct type.
  typedef void GenericFunction();

  // For freeing handler data.
  typedef void Free(void *data);

  typedef void HandlersCallback(void *closure, upb_handlers *h);

  // Returns a new handlers object for the given frozen msgdef.  A single ref
  // will belong to the given owner.
  // Returns NULL if memory allocation failed.
  static Handlers* New(const MessageDef* m, const void *owner);

  // Convenience function for registering a graph of handlers that mirrors the
  // graph of msgdefs for some message.  For "m" and all its children a new set
  // of handlers will be created and the given callback will be invoked,
  // allowing the client to register handlers for this message.  Note that any
  // subhandlers set by the callback will be overwritten.
  static const Handlers* NewFrozen(const MessageDef *m, const void *owner,
                                   HandlersCallback *callback, void *closure);

  // Functionality from upb::RefCounted.
  bool IsFrozen() const;
  void Ref(const void* owner) const;
  void Unref(const void* owner) const;
  void DonateRef(const void *from, const void *to) const;
  void CheckRef(const void *owner) const;

  // Freezes the given set of handlers.  You may not freeze a handler without
  // also freezing any handlers they point to.  In the future we may want to
  // require that all fields of the submessage have had subhandlers set for
  // them.
  static bool Freeze(Handlers*const* handlers, int n, Status* s);

  // Returns the msgdef associated with this handlers object.
  const MessageDef* message_def() const;

  // Sets the startmsg handler for the message, which is defined as follows:
  //
  //   bool startmsg(void *closure) {
  //     // Called when the message begins.  Returns true if processing should
  //     // continue.
  //     return true;
  //   }
  void SetStartMessageHandler(StartMessageHandler *handler);
  StartMessageHandler *GetStartMessageHandler() const;

  // Sets the endmsg handler for the message, which is defined as follows:
  //
  //   void endmsg(void *closure, upb_status *status) {
  //     // Called when processing of this message ends, whether in success or
  //     // failure.  "status" indicates the final status of processing, and
  //     // can also be modified in-place to update the final status.
  //   }
  void SetEndMessageHandler(EndMessageHandler *handler);
  EndMessageHandler *GetEndMessageHandler() const;

  // Sets the value handler for the given field, which is defined as follows
  // (this is for an int32 field; other field types will pass their native
  // C/C++ type for "val"):
  //
  //   bool value(void *closure, void *d, int32_t val) {
  //     // Called when the field's value is encountered.  "d" contains
  //     // whatever data was bound to this field when it was registered.
  //     // Returns true if processing should continue.
  //     return true;
  //   }
  //
  // The value type must exactly match f->type().
  // For example, SetInt32Handler() may only be used for fields of type
  // UPB_TYPE_INT32, UPB_TYPE_SINT32, UPB_TYPE_SFIXED32, and UPB_TYPE_ENUM.
  //
  // "d" is the data that will be bound to this callback and passed to it.
  // If "fr" is non-NULL it will be run when the data is no longer needed.
  //
  // Returns "false" if "f" does not belong to this message or has the wrong
  // type for this handler.
  bool SetInt32Handler (const FieldDef* f,  Int32Handler* h, void* d, Free* fr);
  bool SetInt64Handler (const FieldDef* f,  Int64Handler* h, void* d, Free* fr);
  bool SetUint32Handler(const FieldDef* f, Uint32Handler* h, void* d, Free* fr);
  bool SetUint64Handler(const FieldDef* f, Uint64Handler* h, void* d, Free* fr);
  bool SetFloatHandler (const FieldDef* f,  FloatHandler* h, void* d, Free* fr);
  bool SetDoubleHandler(const FieldDef* f, DoubleHandler* h, void* d, Free* fr);
  bool SetBoolHandler  (const FieldDef* f,   BoolHandler* h, void* d, Free* fr);

  // Sets handlers for a string field, which are defined as follows:
  //
  //   void* startstr(void *closure, void *data, size_t size_hint) {
  //     // Called when a string value begins.  The return value indicates the
  //     // closure for the string.  "size_hint" indicates the size of the
  //     // string if it is known, however if the string is length-delimited
  //     // and the end-of-string is not available size_hint will be zero.
  //     // This case is indistinguishable from the case where the size is
  //     // known to be zero.
  //     //
  //     // TODO(haberman): is it important to distinguish these cases?
  //     // If we had ssize_t as a type we could make -1 "unknown", but
  //     // ssize_t is POSIX (not ANSI) and therefore less portable.
  //     // In practice I suspect it won't be important to distinguish.
  //     return closure;
  //   }
  //
  //   size_t str(void *closure, void *data, const char *str, size_t len) {
  //     // Called for each buffer of string data; the multiple physical buffers
  //     // are all part of the same logical string.  The return value indicates
  //     // how many bytes were consumed.  If this number is less than "len",
  //     // this will also indicate that processing should be halted for now,
  //     // like returning false or UPB_BREAK from any other callback.  If
  //     // number is greater than "len", the excess bytes will be skipped over
  //     // and not passed to the callback.
  //     return len;
  //   }
  //
  //   bool endstr(void *closure, void *data) {
  //     // Called when a string value ends.
  //     return true;
  //   }
  bool SetStartStringHandler(const FieldDef* f, StartStringHandler* h,
                             void* d, Free* fr);
  bool SetStringHandler(const FieldDef* f, StringHandler* h, void* d, Free* fr);
  bool SetEndStringHandler(const FieldDef* f, EndFieldHandler* h,
                           void* d, Free* fr);

  // A setter that is templated on the type of the value.
  template<class T> bool SetValueHandler(
      const FieldDef* f, typename Value<T>::Handler* h, void* d, Free* fr);

  // Sets the startseq handler, which is defined as follows:
  //
  //   void *startseq(void *closure, void *data) {
  //     // Called when a sequence (repeated field) begins.  The returned
  //     // pointer indicates the closure for the sequence (or UPB_BREAK
  //     // to interrupt processing).
  //     return closure;
  //   }
  //
  // Returns "false" if "f" does not belong to this message or is not a
  // repeated field.
  //
  // "data" is the data that will be bound to this callback and passed to it.
  // If "cleanup" is non-NULL it will be run when the data is no longer needed.
  bool SetStartSequenceHandler(const FieldDef* f, StartFieldHandler *handler,
                               void* data, Free* cleanup);

  // Sets the startsubmsg handler for the given field, which is defined as
  // follows:
  //
  //   void *startsubmsg(void *closure, void *data) {
  //     // Called when a submessage begins.  The returned pointer indicates the
  //     // closure for the sequence (or UPB_BREAK to interrupt processing).
  //     return closure;
  //   }
  //
  // "data" is the data that will be bound to this callback and passed to it.
  // If "cleanup" is non-NULL it will be run when the data is no longer needed.
  //
  // Returns "false" if "f" does not belong to this message or is not a
  // submessage/group field.
  bool SetStartSubMessageHandler(const FieldDef* f, StartFieldHandler *handler,
                                 void* data, Free* cleanup);

  // Sets the endsubmsg handler for the given field, which is defined as
  // follows:
  //
  //   bool endsubmsg(void *closure, void *data) {
  //     // Called when a submessage ends.  Returns true to continue processing.
  //     return true;
  //   }
  //
  // "data" is the data that will be bound to this callback and passed to it.
  // If "cleanup" is non-NULL it will be run when the data is no longer needed.
  //
  // Returns "false" if "f" does not belong to this message or is not a
  // submessage/group field.
  bool SetEndSubMessageHandler(const FieldDef* f, EndFieldHandler *handler,
                               void* data, Free* cleanup);

  // Starts the endsubseq handler for the given field, which is defined as
  // follows:
  //
  //   bool endseq(void *closure, void *data) {
  //     // Called when a sequence ends.  Returns true continue processing.
  //     return true;
  //   }
  //
  // "data" is the data that will be bound to this callback and passed to it.
  // If "cleanup" is non-NULL it will be run when the data is no longer needed.
  //
  // Returns "false" if "f" does not belong to this message or is not a
  // repeated field.
  bool SetEndSequenceHandler(const FieldDef* f, EndFieldHandler *handler,
                             void* data, Free* cleanup);

  // Sets or gets the object that specifies handlers for the given field, which
  // must be a submessage or group.  Returns NULL if no handlers are set.
  bool SetSubHandlers(const FieldDef* f, const Handlers* sub);
  const Handlers* GetSubHandlers(const FieldDef* f) const;

  // NOTE: The remaining functions in this class are mostly of interest to
  // byte-code/JIT compilers (or upb internals); most users will not need them.
  // These functions also require more care, since passing a selector that
  // does not match the type of these handlers yields undefined behavior.

  // A selector refers to a specific field handler in the Handlers object
  // (for example: the STARTSUBMSG handler for field "field15").
  // On success, returns true and stores the selector in "s".
  // If the FieldDef or Type are invalid, returns false.
  // The returned selector is ONLY valid for Handlers whose MessageDef
  // contains this FieldDef.
  static bool GetSelector(const FieldDef* f, Type type, Selector* s);

  // Returns the function pointer for this handler.  It is the client's
  // responsibility to cast to the correct function type before calling it.
  GenericFunction* GetHandler(Selector selector);

  // Returns the handler data that was registered with this handler.
  void* GetHandlerData(Selector selector);

  // Gets the byte offset from a Handlers* where the given handler can be found.
  // Useful for JITs that want to read the pointer in their fast path.
  static size_t GetHandlerOffset(Selector selector);

  // Could add any of the following functions as-needed, with some minor
  // implementation changes:
  //
  // const FieldDef* GetFieldDef(Selector selector);
  // static bool IsSequence(Selector selector);
  // Selector GetEndSelector(Selector start_selector);

 private:
  UPB_DISALLOW_POD_OPS(Handlers);

#else
struct upb_handlers {
#endif
  upb_refcounted base;
  const upb_msgdef *msg;
  bool (*startmsg)(void*);
  void (*endmsg)(void*, upb_status*);
  void *fh_base[1];  // Start of dynamically-sized field handler array.
};

// Native C API.
#ifdef __cplusplus
extern "C" {
#endif
typedef bool upb_startmsg_handler(void *c);
typedef void upb_endmsg_handler(void *c, upb_status *status);
typedef void* upb_startfield_handler(void *closure, void *d);
typedef bool upb_endfield_handler(void *closure, void *d);
typedef void upb_handlers_callback(void *closure, upb_handlers *h);
typedef void upb_handlerfree(void *d);
typedef void upb_func();

typedef bool upb_int32_handler(void *c, void *d, int32_t val);
typedef bool upb_int64_handler(void *c, void *d, int64_t val);
typedef bool upb_uint32_handler(void *c, void *d, uint32_t val);
typedef bool upb_uint64_handler(void *c, void *d, uint64_t val);
typedef bool upb_float_handler(void *c, void *d, float val);
typedef bool upb_double_handler(void *c, void *d, double val);
typedef bool upb_bool_handler(void *c, void *d, bool val);
typedef void* upb_startstr_handler(void *closure, void *d, size_t size_hint);
typedef size_t upb_string_handler(void *c, void *d, const char *buf, size_t n);

upb_handlers *upb_handlers_new(const upb_msgdef *m, const void *owner);
const upb_handlers *upb_handlers_newfrozen(const upb_msgdef *m,
                                           const void *owner,
                                           upb_handlers_callback *callback,
                                           void *closure);

// From upb_refcounted.
void upb_handlers_unref(const upb_handlers *h, const void *owner);
bool upb_handlers_isfrozen(const upb_handlers *h);
void upb_handlers_ref(const upb_handlers *h, const void *owner);
void upb_handlers_donateref(
    const upb_handlers *h, const void *from, const void *to);
void upb_handlers_checkref(const upb_handlers *h, const void *owner);

bool upb_handlers_freeze(upb_handlers *const*handlers, int n, upb_status *s);
const upb_msgdef *upb_handlers_msgdef(const upb_handlers *h);
void upb_handlers_setstartmsg(upb_handlers *h, upb_startmsg_handler *handler);
upb_startmsg_handler *upb_handlers_getstartmsg(const upb_handlers *h);
void upb_handlers_setendmsg(upb_handlers *h, upb_endmsg_handler *handler);
upb_endmsg_handler *upb_handlers_getendmsg(const upb_handlers *h);
bool upb_handlers_setint32(
    upb_handlers *h, const upb_fielddef *f, upb_int32_handler *handler,
    void *d, upb_handlerfree *fr);
bool upb_handlers_setint64(
    upb_handlers *h, const upb_fielddef *f, upb_int64_handler *handler,
    void *d, upb_handlerfree *fr);
bool upb_handlers_setuint32(
    upb_handlers *h, const upb_fielddef *f, upb_uint32_handler *handler,
    void *d, upb_handlerfree *fr);
bool upb_handlers_setuint64(
    upb_handlers *h, const upb_fielddef *f, upb_uint64_handler *handler,
    void *d, upb_handlerfree *fr);
bool upb_handlers_setfloat(
    upb_handlers *h, const upb_fielddef *f, upb_float_handler *handler,
    void *d, upb_handlerfree *fr);
bool upb_handlers_setdouble(
    upb_handlers *h, const upb_fielddef *f, upb_double_handler *handler,
    void *d, upb_handlerfree *fr);
bool upb_handlers_setbool(
    upb_handlers *h, const upb_fielddef *f, upb_bool_handler *handler,
    void *d, upb_handlerfree *fr);
bool upb_handlers_setstartstr(
    upb_handlers *h, const upb_fielddef *f, upb_startstr_handler *handler,
    void *d, upb_handlerfree *fr);
bool upb_handlers_setstring(
    upb_handlers *h, const upb_fielddef *f, upb_string_handler *handler,
    void *d, upb_handlerfree *fr);
bool upb_handlers_setendstr(
    upb_handlers *h, const upb_fielddef *f, upb_endfield_handler *handler,
    void *d, upb_handlerfree *fr);
bool upb_handlers_setstartseq(
    upb_handlers *h, const upb_fielddef *f, upb_startfield_handler *handler,
    void *d, upb_handlerfree *fr);
bool upb_handlers_setstartsubmsg(
    upb_handlers *h, const upb_fielddef *f, upb_startfield_handler *handler,
    void *d, upb_handlerfree *fr);
bool upb_handlers_setendsubmsg(
    upb_handlers *h, const upb_fielddef *f, upb_endfield_handler *handler,
    void *d, upb_handlerfree *fr);
bool upb_handlers_setendseq(
    upb_handlers *h, const upb_fielddef *f, upb_endfield_handler *handler,
    void *d, upb_handlerfree *fr);
bool upb_handlers_setsubhandlers(
    upb_handlers *h, const upb_fielddef *f, const upb_handlers *sub);
const upb_handlers *upb_handlers_getsubhandlers(
    const upb_handlers *h, const upb_fielddef *f);
upb_handlertype_t upb_handlers_getprimitivehandlertype(const upb_fielddef *f);
bool upb_getselector(
    const upb_fielddef *f, upb_handlertype_t type, upb_selector_t *s);
upb_func *upb_handlers_gethandler(const upb_handlers *h, upb_selector_t s);
void *upb_handlers_gethandlerdata(const upb_handlers *h, upb_selector_t s);
size_t upb_gethandleroffset(upb_selector_t s);

// Internal-only.
uint32_t upb_handlers_selectorbaseoffset(const upb_fielddef *f);
uint32_t upb_handlers_selectorcount(const upb_fielddef *f);
#ifdef __cplusplus
}  // extern "C"
#endif

// Convenience versions of the above that first look up the field by name.
#define DEFINE_NAME_SETTER(slot, type) \
  INLINE void upb_handlers_set ## slot ## _n( \
      upb_handlers *h, const char *name, type val, \
      void *d, upb_handlerfree *fr) { \
    upb_handlers_set ## slot(h, upb_msgdef_ntof( \
        upb_handlers_msgdef(h), name), val, d, fr); \
  }
DEFINE_NAME_SETTER(int32, upb_int32_handler*);
DEFINE_NAME_SETTER(int64, upb_int64_handler*);
DEFINE_NAME_SETTER(uint32, upb_uint32_handler*);
DEFINE_NAME_SETTER(uint64, upb_uint64_handler*);
DEFINE_NAME_SETTER(float, upb_float_handler*);
DEFINE_NAME_SETTER(double, upb_double_handler*);
DEFINE_NAME_SETTER(bool, upb_bool_handler*);
DEFINE_NAME_SETTER(startstr, upb_startstr_handler*);
DEFINE_NAME_SETTER(string, upb_string_handler*);
DEFINE_NAME_SETTER(endstr, upb_endfield_handler*);
DEFINE_NAME_SETTER(startseq, upb_startfield_handler*);
DEFINE_NAME_SETTER(startsubmsg, upb_startfield_handler*);
DEFINE_NAME_SETTER(endsubmsg, upb_endfield_handler*);
DEFINE_NAME_SETTER(endseq, upb_endfield_handler*);
#undef DEFINE_NAME_SETTER

// Value writers for every in-memory type: write the data to a known offset
// from the closure "c."  These depend on the fval being a pointer to a
// structure that is (or begins with) the upb_stdmsg_fval type.
//
// TODO(haberman): These are hacky; remove them and replace with an API that
// lets you set a simple "writer" handler in a way that can generate
// specialized code right then.

typedef struct upb_stdmsg_fval {
#ifdef __cplusplus
  upb_stdmsg_fval(size_t offset_, int32_t hasbit_)
      : offset(offset_),
        hasbit(hasbit_) {
  }
#endif
  size_t offset;
  int32_t hasbit;
} upb_stdmsg_fval;

#ifdef __cplusplus
extern "C" {
#endif
bool upb_stdmsg_setint32(void *c, void *d, int32_t val);
bool upb_stdmsg_setint64(void *c, void *d, int64_t val);
bool upb_stdmsg_setuint32(void *c, void *d, uint32_t val);
bool upb_stdmsg_setuint64(void *c, void *d, uint64_t val);
bool upb_stdmsg_setfloat(void *c, void *d, float val);
bool upb_stdmsg_setdouble(void *c, void *d, double val);
bool upb_stdmsg_setbool(void *c, void *d, bool val);
#ifdef __cplusplus
}  // extern "C"
#endif

#ifdef __cplusplus

namespace upb {

// C++ Wrappers
inline Handlers* Handlers::New(const MessageDef* m, const void *owner) {
  return upb_handlers_new(m, owner);
}
inline const Handlers* Handlers::NewFrozen(
    const MessageDef *m, const void *owner,
    upb_handlers_callback *callback, void *closure) {
  return upb_handlers_newfrozen(m, owner, callback, closure);
}
inline bool Handlers::IsFrozen() const {
  return upb_handlers_isfrozen(this);
}
inline void Handlers::Ref(const void* owner) const {
  upb_handlers_ref(this, owner);
}
inline void Handlers::Unref(const void* owner) const {
  upb_handlers_unref(this, owner);
}
inline void Handlers::DonateRef(const void *from, const void *to) const {
  upb_handlers_donateref(this, from, to);
}
inline void Handlers::CheckRef(const void *owner) const {
  upb_handlers_checkref(this, owner);
}
inline bool Handlers::Freeze(Handlers*const* handlers, int n, Status* s) {
  return upb_handlers_freeze(handlers, n, s);
}
inline const MessageDef* Handlers::message_def() const {
  return upb_handlers_msgdef(this);
}
inline void Handlers::SetStartMessageHandler(
    Handlers::StartMessageHandler *handler) {
  upb_handlers_setstartmsg(this, handler);
}
inline void Handlers::SetEndMessageHandler(
    Handlers::EndMessageHandler *handler) {
  upb_handlers_setendmsg(this, handler);
}
inline bool Handlers::SetInt32Handler(
    const FieldDef *f, Handlers::Int32Handler *handler,
    void *d, Handlers::Free *fr) {
  return upb_handlers_setint32(this, f, handler, d, fr);
}
inline bool Handlers::SetInt64Handler(
    const FieldDef *f, Handlers::Int64Handler *handler,
    void *d, Handlers::Free *fr) {
  return upb_handlers_setint64(this, f, handler, d, fr);
}
inline bool Handlers::SetUint32Handler(
    const FieldDef *f, Handlers::Uint32Handler *handler,
    void *d, Handlers::Free *fr) {
  return upb_handlers_setuint32(this, f, handler, d, fr);
}
inline bool Handlers::SetUint64Handler(
    const FieldDef *f, Handlers::Uint64Handler *handler,
    void *d, Handlers::Free *fr) {
  return upb_handlers_setuint64(this, f, handler, d, fr);
}
inline bool Handlers::SetFloatHandler(
    const FieldDef *f, Handlers::FloatHandler *handler,
    void *d, Handlers::Free *fr) {
  return upb_handlers_setfloat(this, f, handler, d, fr);
}
inline bool Handlers::SetDoubleHandler(
    const FieldDef *f, Handlers::DoubleHandler *handler,
    void *d, Handlers::Free *fr) {
  return upb_handlers_setdouble(this, f, handler, d, fr);
}
inline bool Handlers::SetBoolHandler(
    const FieldDef *f, Handlers::BoolHandler *handler,
    void *d, Handlers::Free *fr) {
  return upb_handlers_setbool(this, f, handler, d, fr);
}
inline bool Handlers::SetStartStringHandler(
    const FieldDef* f, Handlers::StartStringHandler* handler,
    void* d, Handlers::Free* fr) {
  return upb_handlers_setstartstr(this, f, handler, d, fr);
}
inline bool Handlers::SetEndStringHandler(
    const FieldDef* f, Handlers::EndFieldHandler* handler,
    void* d, Handlers::Free* fr) {
  return upb_handlers_setendstr(this, f, handler, d, fr);
}
inline bool Handlers::SetStringHandler(
    const FieldDef *f, Handlers::StringHandler *handler,
    void *d, Handlers::Free *fr) {
  return upb_handlers_setstring(this, f, handler, d, fr);
}
inline bool Handlers::SetStartSequenceHandler(
    const FieldDef* f, Handlers::StartFieldHandler *handler,
    void *d, Handlers::Free *fr) {
  return upb_handlers_setstartseq(this, f, handler, d, fr);
}
inline bool Handlers::SetStartSubMessageHandler(
    const FieldDef* f, Handlers::StartFieldHandler *handler,
    void *d, Handlers::Free *fr) {
  return upb_handlers_setstartsubmsg(this, f, handler, d, fr);
}
inline bool Handlers::SetEndSubMessageHandler(
    const FieldDef* f, Handlers::EndFieldHandler *handler,
    void *d, Handlers::Free *fr) {
  return upb_handlers_setendsubmsg(this, f, handler, d, fr);
}
inline bool Handlers::SetEndSequenceHandler(
    const FieldDef* f, Handlers::EndFieldHandler *handler,
    void *d, Handlers::Free *fr) {
  return upb_handlers_setendseq(this, f, handler, d, fr);
}
inline bool Handlers::SetSubHandlers(
    const FieldDef* f, const Handlers* sub) {
  return upb_handlers_setsubhandlers(this, f, sub);
}
inline Handlers::StartMessageHandler *Handlers::GetStartMessageHandler() const {
  return upb_handlers_getstartmsg(this);
}
inline Handlers::EndMessageHandler *Handlers::GetEndMessageHandler() const {
  return upb_handlers_getendmsg(this);
}
inline const Handlers* Handlers::GetSubHandlers(
    const FieldDef* f) const {
  return upb_handlers_getsubhandlers(this, f);
}
inline bool Handlers::GetSelector(
    const FieldDef* f, Handlers::Type type, Handlers::Selector* s) {
  return upb_getselector(f, type, s);
}
inline Handlers::GenericFunction* Handlers::GetHandler(
    Handlers::Selector selector) {
  return upb_handlers_gethandler(this, selector);
}
inline void* Handlers::GetHandlerData(Handlers::Selector selector) {
  return upb_handlers_gethandlerdata(this, selector);
}
inline size_t Handlers::GetHandlerOffset(Handlers::Selector selector) {
  return upb_gethandleroffset(selector);
}

#define SET_VALUE_HANDLER(type, ctype) \
    template<> \
    inline bool Handlers::SetValueHandler<ctype>( \
        const FieldDef* f, \
        typename Handlers::Value<ctype>::Handler* handler, \
        void* data, Handlers::Free* cleanup) { \
      return upb_handlers_set ## type(this, f, handler, data, cleanup); \
    }
SET_VALUE_HANDLER(double, double);
SET_VALUE_HANDLER(float, float);
SET_VALUE_HANDLER(uint64, uint64_t);
SET_VALUE_HANDLER(uint32, uint32_t);
SET_VALUE_HANDLER(int64, int64_t);
SET_VALUE_HANDLER(int32, int32_t);
SET_VALUE_HANDLER(bool, bool);
#undef SET_VALUE_HANDLER

template <class T> void DeletePointer(void *p) { delete static_cast<T*>(p); }

template <class T>
void SetStoreValueHandler(
    const FieldDef* f, size_t offset, int32_t hasbit, Handlers* h);

// A handy templated function that will retrieve a value handler for a given
// C++ type.
#define SET_STORE_VALUE_HANDLER(type, ctype) \
    template <> \
    inline void SetStoreValueHandler<ctype>(const FieldDef* f, size_t offset, \
                                            int32_t hasbit, Handlers* h) { \
      h->SetValueHandler<ctype>( \
          f, upb_stdmsg_set ## type, new upb_stdmsg_fval(offset, hasbit), \
          &upb::DeletePointer<upb_stdmsg_fval>); \
    }

SET_STORE_VALUE_HANDLER(double, double);
SET_STORE_VALUE_HANDLER(float, float);
SET_STORE_VALUE_HANDLER(uint64, uint64_t);
SET_STORE_VALUE_HANDLER(uint32, uint32_t);
SET_STORE_VALUE_HANDLER(int64, int64_t);
SET_STORE_VALUE_HANDLER(int32, int32_t);
SET_STORE_VALUE_HANDLER(bool, bool);
#undef GET_VALUE_HANDLER

}  // namespace upb
#endif

#endif
