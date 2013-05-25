/*
 * upb - a minimalist implementation of protocol buffers.
 *
 * Copyright (c) 2011-2012 Google Inc.  See LICENSE for details.
 * Author: Josh Haberman <jhaberman@gmail.com>
 *
 * Inline definitions for handlers.h, which are particularly long and a bit
 * tricky.
 */

#ifndef UPB_HANDLERS_INL_H_
#define UPB_HANDLERS_INL_H_

#ifdef __cplusplus

namespace upb {

// Deleter: class for constructing a function that deletes a pointer type.
template <class T> struct Deleter {
  static void Delete(void* p) { delete static_cast<T*>(p); }
};

template <class T> Deleter<T> MatchDeleter(T* data) {
  UPB_UNUSED(data);
  return Deleter<T>();
}

// Template magic for creating type-safe wrappers around the user's actual
// function.  These convert between the void*'s of the C API and the C++
// user's types.  These also handle conversion between multiple types with
// the same witdh; ie "long long" and "long" are both 64 bits on LP64.

// ValueHandler
template <class C, class T1, class T2 = typename CanonicalType<T1>::Type>
struct ValueHandlerWrapper2 {
  template <bool F(C *, T1)>
  inline static bool Wrapper(void *closure, const void *hd, T2 val) {
    UPB_UNUSED(hd);
    return F(static_cast<C *>(closure), val);
  }
};

template <class C, class D, class T1,
          class T2 = typename CanonicalType<T1>::Type>
struct ValueHandlerWrapper3 {
  template <bool F(C *, const D *, T1)>
  inline static bool Wrapper(void *closure, const void *hd, T2 val) {
    return F(static_cast<C *>(closure), static_cast<const D *>(hd), val);
  }
};

template <class C, class T>
inline ValueHandlerWrapper2<C, T> MatchWrapper(bool (*f)(C *, T)) {
  UPB_UNUSED(f);
  return ValueHandlerWrapper2<C, T>();
}

template <class C, class D, class T>
inline ValueHandlerWrapper3<C, D, T> MatchWrapper(bool (*f)(C *, const D *,
                                                            T)) {
  UPB_UNUSED(f);
  return ValueHandlerWrapper3<C, D, T>();
}

template <class T>
inline typename ValueHandler<T>::H MakeHandler(bool (*wrapper)(void *,
                                                               const void *,
                                                               T)) {
  return ValueHandler<T>::H::Make(wrapper, NULL, NULL);
}

template <class C, class D, class T1, class T2>
inline typename ValueHandler<T1>::H BindHandler(
    bool (*wrapper)(void *, const void *, T1), bool (*h)(C *, const D *, T2),
    D *data) {
  UPB_UNUSED(h);  // Only for making sure function matches "D".
  return ValueHandler<T1>::H::Make(wrapper, data, MatchDeleter(data).Delete);
}

// StartFieldHandler
template <class R, class C> struct StartFieldHandlerWrapper2 {
  template <R *F(C *)> static void *Wrapper(void *closure, const void *hd) {
    UPB_UNUSED(hd);
    return F(static_cast<C *>(closure));
  }
};

template <class R, class C, class D> struct StartFieldHandlerWrapper3 {
  template <R *F(C *, const D *)>
  inline static void *Wrapper(void *closure, const void *hd) {
    return F(static_cast<C *>(closure), static_cast<const D *>(hd));
  }
};

template <class R, class C>
inline StartFieldHandlerWrapper2<R, C> MatchWrapper(R *(*f)(C *)) {
  UPB_UNUSED(f);
  return StartFieldHandlerWrapper2<R, C>();
}

template <class R, class C, class D>
inline StartFieldHandlerWrapper3<R, C, D> MatchWrapper(R *(*f)(C *,
                                                               const D *)) {
  UPB_UNUSED(f);
  return StartFieldHandlerWrapper3<R, C, D>();
}

inline StartFieldHandler MakeHandler(void *(*wrapper)(void *, const void *)) {
  return StartFieldHandler::Make(wrapper, NULL, NULL);
}

template <class R, class C, class D>
inline StartFieldHandler BindHandler(void *(*wrapper)(void *, const void *),
                                     R *(*h)(C *, const D *), D *data) {
  UPB_UNUSED(h);  // Only for making sure function matches "D".
  return StartFieldHandler::Make(wrapper, data, MatchDeleter(data).Delete);
}

// EndFieldHandler
template <class C> struct EndFieldHandlerWrapper2 {
  template <bool F(C *)>
  inline static bool Wrapper(void *closure, const void *hd) {
    UPB_UNUSED(hd);
    return F(static_cast<C *>(closure));
  }
};

template <class C, class D> struct EndFieldHandlerWrapper3 {
  template <bool F(C *, const D *)>
  inline static bool Wrapper(void *closure, const void *hd) {
    return F(static_cast<C *>(closure), static_cast<const D *>(hd));
  }
};

template <class C>
inline EndFieldHandlerWrapper2<C> MatchWrapper(bool (*f)(C *)) {
  UPB_UNUSED(f);
  return EndFieldHandlerWrapper2<C>();
}

template <class C, class D>
inline EndFieldHandlerWrapper3<C, D> MatchWrapper(bool (*f)(C *, const D *)) {
  UPB_UNUSED(f);
  return EndFieldHandlerWrapper3<C, D>();
}

inline EndFieldHandler MakeHandler(bool (*wrapper)(void *, const void *)) {
  return EndFieldHandler::Make(wrapper, NULL, NULL);
}

template <class C, class D>
inline EndFieldHandler BindHandler(bool (*wrapper)(void *, const void *),
                                   bool (*h)(C *, const D *), D *data) {
  UPB_UNUSED(h);  // Only for making sure function matches "D".
  return EndFieldHandler::Make(wrapper, data, MatchDeleter(data).Delete);
}

// StartStringHandler
template <class R, class C> struct StartStringHandlerWrapper2 {
  template <R *F(C *, size_t)>
  inline static void *Wrapper(void *closure, const void *hd, size_t hint) {
    UPB_UNUSED(hd);
    return F(static_cast<C *>(closure), hint);
  }
};

template <class R, class C, class D> struct StartStringHandlerWrapper3 {
  template <R *F(C *, const D *, size_t)>
  inline static void *Wrapper(void *closure, const void *hd, size_t hint) {
    return F(static_cast<C *>(closure), static_cast<const D *>(hd), hint);
  }
};

template <class R, class C>
inline StartStringHandlerWrapper2<R, C> MatchWrapper(R *(*f)(C *, size_t)) {
  UPB_UNUSED(f);
  return StartStringHandlerWrapper2<R, C>();
}

template <class R, class C, class D>
inline StartStringHandlerWrapper3<R, C, D> MatchWrapper(R *(*f)(C *, const D *,
                                                                size_t)) {
  UPB_UNUSED(f);
  return StartStringHandlerWrapper3<R, C, D>();
}

inline StartStringHandler MakeHandler(void *(*wrapper)(void *, const void *,
                                                       size_t)) {
  return StartStringHandler::Make(wrapper, NULL, NULL);
}

template <class R, class C, class D>
inline StartStringHandler BindHandler(void *(*wrapper)(void *, const void *,
                                                       size_t),
                                      R *(*h)(C *, const D *, size_t),
                                      D *data) {
  UPB_UNUSED(h);  // Only for making sure function matches "D".
  return StartStringHandler::Make(wrapper, data, MatchDeleter(data).Delete);
}

// StringHandler
template <class C> struct StringHandlerWrapper2 {
  template <size_t F(C *, const char *buf, size_t len)>
  inline static size_t Wrapper(void *closure, const void *hd, const char *buf,
                               size_t len) {
    UPB_UNUSED(hd);
    return F(static_cast<C *>(closure), buf, len);
  }
};

template <class C, class D> struct StringHandlerWrapper3 {
  template <size_t F(C *, const D *, const char *buf, size_t len)>
  inline static size_t Wrapper(void *closure, const void *hd, const char *buf,
                               size_t len) {
    return F(static_cast<C *>(closure), static_cast<const D *>(hd), buf, len);
  }
};

template <class C>
inline StringHandlerWrapper2<C> MatchWrapper(size_t (*f)(C *, const char *,
                                                         size_t)) {
  UPB_UNUSED(f);
  return StringHandlerWrapper2<C>();
}

template <class C, class D>
inline StringHandlerWrapper3<C, D> MatchWrapper(size_t (*f)(C *, const D *,
                                                            const char *,
                                                            size_t)) {
  UPB_UNUSED(f);
  return StringHandlerWrapper3<C, D>();
}

inline StringHandler MakeHandler(size_t (*wrapper)(void *, const void *,
                                                   const char *, size_t)) {
  return StringHandler::Make(wrapper, NULL, NULL);
}

template <class C, class D>
inline StringHandler BindHandler(
    size_t (*wrapper)(void *, const void *, const char *, size_t),
    size_t (*h)(C *, const D *, const char *, size_t), D *data) {
  UPB_UNUSED(h);  // Only for making sure function matches "D".
  return StringHandler::Make(wrapper, data, MatchDeleter(data).Delete);
}

// utype/ltype are upper/lower-case, ctype is canonical C type, vtype is
// variant C type.
#define TYPE_METHODS(utype, ltype, ctype, vtype)                               \
  template <> struct CanonicalType<vtype> {                                    \
    typedef ctype Type;                                                        \
  };                                                                           \
  template <>                                                                  \
  inline bool Handlers::SetValueHandler<vtype>(                                \
      const FieldDef *f,                                                       \
      const typename ValueHandler<typename CanonicalType<vtype>::Type>::H &    \
          handler) {                                                           \
    handler.registered_ = true;                                                \
    return upb_handlers_set##ltype(this, f, handler.handler_, handler.data_,   \
                                   handler.cleanup_);                          \
  }                                                                            \
  template <>                                                                  \
  inline bool Handlers::SetValueHandler<vtype>(                                \
      const char *f, const typename ValueHandler<                              \
                         typename CanonicalType<vtype>::Type>::H &handler) {   \
    handler.registered_ = true;                                                \
    return upb_handlers_set##ltype##_n(this, f, handler.handler_,              \
                                       handler.data_, handler.cleanup_);       \
  }

TYPE_METHODS(Double, double, double,   double);
TYPE_METHODS(Float,  float,  float,    float);
TYPE_METHODS(UInt64, uint64, uint64_t, upb_uint64_t);
TYPE_METHODS(UInt32, uint32, uint32_t, upb_uint32_t);
TYPE_METHODS(Int64,  int64,  int64_t,  upb_int64_t);
TYPE_METHODS(Int32,  int32,  int32_t,  upb_int32_t);
TYPE_METHODS(Bool,   bool,   bool,     bool);

#ifdef UPB_TWO_32BIT_TYPES
TYPE_METHODS(Int32,  int32,  int32_t,  upb_int32alt_t);
TYPE_METHODS(Uint32, uint32, uint32_t, upb_uint32alt_t);
#endif

#ifdef UPB_TWO_64BIT_TYPES
TYPE_METHODS(Int64,  int64,  int64_t,  upb_int64alt_t);
TYPE_METHODS(UInt64, uint64, uint64_t, upb_uint64alt_t);
#endif
#undef TYPE_METHODS

// Type methods that are only one-per-canonical-type and not one-per-cvariant.

#define TYPE_METHODS(utype, ctype) \
    inline bool Handlers::Set##utype##Handler(const FieldDef *f, \
                                              const utype##Handler &h) { \
      return SetValueHandler<ctype>(f, h); \
    } \
    inline bool Handlers::Set##utype##Handler(const char *f,     \
                                              const utype##Handler &h) { \
      return SetValueHandler<ctype>(f, h); \
    }

TYPE_METHODS(Double, double);
TYPE_METHODS(Float,  float);
TYPE_METHODS(UInt64, uint64_t);
TYPE_METHODS(UInt32, uint32_t);
TYPE_METHODS(Int64,  int64_t);
TYPE_METHODS(Int32,  int32_t);
TYPE_METHODS(Bool,   bool);
#undef TYPE_METHODS

template <class T1, bool F(T1*)> bool Wrapper1(void *p1) {
  return F(static_cast<T1*>(p1));
}
template <class T1, bool F(T1 *, upb::Status *)>
bool EndMessageWrapper(void *p1, upb::Status *s) {
  return F(static_cast<T1 *>(p1), s);
}
inline Handlers *Handlers::New(const MessageDef *m, const FrameType *ft,
                               const void *owner) {
  return upb_handlers_new(m, ft, owner);
}
inline const Handlers *Handlers::NewFrozen(const MessageDef *m,
                                           const FrameType *ft,
                                           const void *owner,
                                           upb_handlers_callback *callback,
                                           void *closure) {
  return upb_handlers_newfrozen(m, ft, owner, callback, closure);
}
inline bool Handlers::IsFrozen() const { return upb_handlers_isfrozen(this); }
inline void Handlers::Ref(const void *owner) const {
  upb_handlers_ref(this, owner);
}
inline void Handlers::Unref(const void *owner) const {
  upb_handlers_unref(this, owner);
}
inline void Handlers::DonateRef(const void *from, const void *to) const {
  upb_handlers_donateref(this, from, to);
}
inline void Handlers::CheckRef(const void *owner) const {
  upb_handlers_checkref(this, owner);
}
inline bool Handlers::Freeze(Handlers *const *handlers, int n, Status *s) {
  return upb_handlers_freeze(handlers, n, s);
}
inline const FrameType *Handlers::frame_type() const {
  return upb_handlers_frametype(this);
}
inline const MessageDef *Handlers::message_def() const {
  return upb_handlers_msgdef(this);
}
template <class T, bool F(T *)> void Handlers::SetStartMessageHandler() {
  upb_handlers_setstartmsg(this, &Wrapper1<T, F>);
}
template <class T, bool F(T *, upb::Status *)>
void Handlers::SetEndMessageHandler() {
  upb_handlers_setendmsg(this, &EndMessageWrapper<T, F>);
}
inline bool Handlers::SetStartStringHandler(const FieldDef *f,
                                            const StartStringHandler &handler) {
  handler.registered_ = true;
  return upb_handlers_setstartstr(this, f, handler.handler_, handler.data_,
                                  handler.cleanup_);
}
inline bool Handlers::SetEndStringHandler(const FieldDef *f,
                                          const EndFieldHandler &handler) {
  handler.registered_ = true;
  return upb_handlers_setendstr(this, f, handler.handler_, handler.data_,
                                handler.cleanup_);
}
inline bool Handlers::SetStringHandler(const FieldDef *f,
                                       const StringHandler& handler) {
  handler.registered_ = true;
  return upb_handlers_setstring(this, f, handler.handler_, handler.data_,
                                handler.cleanup_);
}
inline bool Handlers::SetStartSequenceHandler(
    const FieldDef *f, const StartFieldHandler &handler) {
  handler.registered_ = true;
  return upb_handlers_setstartseq(this, f, handler.handler_, handler.data_,
                                  handler.cleanup_);
}
inline bool Handlers::SetStartSubMessageHandler(
    const FieldDef *f, const StartFieldHandler &handler) {
  handler.registered_ = true;
  return upb_handlers_setstartsubmsg(this, f, handler.handler_, handler.data_,
                                     handler.cleanup_);
}
inline bool Handlers::SetEndSubMessageHandler(const FieldDef *f,
                                              const EndFieldHandler &handler) {
  handler.registered_ = true;
  return upb_handlers_setendsubmsg(this, f, handler.handler_, handler.data_,
                                   handler.cleanup_);
}
inline bool Handlers::SetEndSequenceHandler(const FieldDef *f,
                                            const EndFieldHandler &handler) {
  handler.registered_ = true;
  return upb_handlers_setendseq(this, f, handler.handler_, handler.data_,
                                handler.cleanup_);
}
inline bool Handlers::SetSubHandlers(const FieldDef *f, const Handlers *sub) {
  return upb_handlers_setsubhandlers(this, f, sub);
}
inline bool Handlers::SetStartStringHandler(const char *name,
                                            const StartStringHandler &handler) {
  handler.registered_ = true;
  return upb_handlers_setstartstr_n(this, name, handler.handler_, handler.data_,
                                    handler.cleanup_);
}
inline bool Handlers::SetEndStringHandler(const char *name,
                                          const EndFieldHandler &handler) {
  handler.registered_ = true;
  return upb_handlers_setendstr_n(this, name, handler.handler_, handler.data_,
                                  handler.cleanup_);
}
inline bool Handlers::SetStringHandler(const char *name,
                                       const StringHandler &handler) {
  handler.registered_ = true;
  return upb_handlers_setstring_n(this, name, handler.handler_, handler.data_,
                                  handler.cleanup_);
}
inline bool Handlers::SetStartSequenceHandler(
    const char *name, const StartFieldHandler &handler) {
  handler.registered_ = true;
  return upb_handlers_setstartseq_n(this, name, handler.handler_, handler.data_,
                                    handler.cleanup_);
}
inline bool Handlers::SetStartSubMessageHandler(
    const char *name, const StartFieldHandler &handler) {
  handler.registered_ = true;
  return upb_handlers_setstartsubmsg_n(this, name, handler.handler_,
                                       handler.data_, handler.cleanup_);
}
inline bool Handlers::SetEndSubMessageHandler(const char *name,
                                              const EndFieldHandler &handler) {
  handler.registered_ = true;
  return upb_handlers_setendsubmsg_n(this, name, handler.handler_,
                                     handler.data_, handler.cleanup_);
}
inline bool Handlers::SetEndSequenceHandler(const char *name,
                                            const EndFieldHandler &handler) {
  handler.registered_ = true;
  return upb_handlers_setendseq_n(this, name, handler.handler_, handler.data_,
                                  handler.cleanup_);
}
inline const Handlers *Handlers::GetSubHandlers(const FieldDef *f) const {
  return upb_handlers_getsubhandlers(this, f);
}
inline const Handlers *Handlers::GetSubHandlers(Handlers::Selector sel) const {
  return upb_handlers_getsubhandlers_sel(this, sel);
}
inline bool Handlers::GetSelector(const FieldDef *f, Handlers::Type type,
                                  Handlers::Selector *s) {
  return upb_handlers_getselector(f, type, s);
}
inline Handlers::Selector Handlers::GetEndSelector(Handlers::Selector start) {
  return upb_handlers_getendselector(start);
}
inline Handlers::GenericFunction *Handlers::GetHandler(
    Handlers::Selector selector) {
  return upb_handlers_gethandler(this, selector);
}
inline const void *Handlers::GetHandlerData(Handlers::Selector selector) {
  return upb_handlers_gethandlerdata(this, selector);
}

}  // namespace upb

#endif  // __cplusplus

#endif  // UPB_HANDLERS_INL_H_
