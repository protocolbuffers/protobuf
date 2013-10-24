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

#include <limits.h>

// Type detection and typedefs for integer types.
// For platforms where there are multiple 32-bit or 64-bit types, we need to be
// able to enumerate them so we can properly create overloads for all variants.
//
// If any platform existed where there were three integer types with the same
// size, this would have to become more complicated.  For example, short, int,
// and long could all be 32-bits.  Even more diabolically, short, int, long,
// and long long could all be 64 bits and still be standard-compliant.
// However, few platforms are this strange, and it's unlikely that upb will be
// used on the strangest ones.

// Can't count on stdint.h limits like INT32_MAX, because in C++ these are
// only defined when __STDC_LIMIT_MACROS are defined before the *first* include
// of stdint.h.  We can't guarantee that someone else didn't include these first
// without defining __STDC_LIMIT_MACROS.
#define UPB_INT32_MAX 0x7fffffffLL
#define UPB_INT32_MIN (-UPB_INT32_MAX - 1)
#define UPB_INT64_MAX 0x7fffffffffffffffLL
#define UPB_INT64_MIN (-UPB_INT64_MAX - 1)

#if INT_MAX == UPB_INT32_MAX && INT_MIN == UPB_INT32_MIN
#define UPB_INT_IS_32BITS 1
#endif

#if LONG_MAX == UPB_INT32_MAX && LONG_MIN == UPB_INT32_MIN
#define UPB_LONG_IS_32BITS 1
#endif

#if LONG_MAX == UPB_INT64_MAX && LONG_MIN == UPB_INT64_MIN
#define UPB_LONG_IS_64BITS 1
#endif

#if LLONG_MAX == UPB_INT64_MAX && LLONG_MIN == UPB_INT64_MIN
#define UPB_LLONG_IS_64BITS 1
#endif

// We use macros instead of typedefs so we can undefine them later and avoid
// leaking them outside this header file.
#if UPB_INT_IS_32BITS
#define UPB_INT32_T int
#define UPB_UINT32_T unsigned int

#if UPB_LONG_IS_32BITS
#define UPB_TWO_32BIT_TYPES 1
#define UPB_INT32ALT_T long
#define UPB_UINT32ALT_T unsigned long
#endif  // UPB_LONG_IS_32BITS

#elif UPB_LONG_IS_32BITS  // && !UPB_INT_IS_32BITS
#define UPB_INT32_T long
#define UPB_UINT32_T unsigned long
#endif  // UPB_INT_IS_32BITS


#if UPB_LONG_IS_64BITS
#define UPB_INT64_T long
#define UPB_UINT64_T unsigned long

#if UPB_LLONG_IS_64BITS
#define UPB_TWO_64BIT_TYPES 1
#define UPB_INT64ALT_T long long
#define UPB_UINT64ALT_T unsigned long long
#endif  // UPB_LLONG_IS_64BITS

#elif UPB_LLONG_IS_64BITS  // && !UPB_LONG_IS_64BITS
#define UPB_INT64_T long long
#define UPB_UINT64_T unsigned long long
#endif  // UPB_LONG_IS_64BITS

#undef UPB_INT32_MAX
#undef UPB_INT32_MIN
#undef UPB_INT64_MAX
#undef UPB_INT64_MIN
#undef UPB_INT_IS_32BITS
#undef UPB_LONG_IS_32BITS
#undef UPB_LONG_IS_64BITS
#undef UPB_LLONG_IS_64BITS

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

// EndMessageHandler
template <class C> struct EndMessageHandlerWrapper2 {
  template <bool F(C *, Status *)>
  static bool Wrapper(void *closure, const void *hd, Status* s) {
    UPB_UNUSED(hd);
    return F(static_cast<C *>(closure), s);
  }
};

template <class C, class D> struct EndMessageHandlerWrapper3 {
  template <bool F(C *, const D *, Status*)>
  inline static bool Wrapper(void *closure, const void *hd, Status* s) {
    return F(static_cast<C *>(closure), static_cast<const D *>(hd), s);
  }
};

template <class C>
inline EndMessageHandlerWrapper2<C> MatchWrapper(bool (*f)(C *, Status *)) {
  UPB_UNUSED(f);
  return EndMessageHandlerWrapper2<C>();
}

template <class C, class D>
inline EndMessageHandlerWrapper3<C, D> MatchWrapper(bool (*f)(C *, const D *,
                                                              Status *)) {
  UPB_UNUSED(f);
  return EndMessageHandlerWrapper3<C, D>();
}

inline Handlers::EndMessageHandler MakeHandler(bool (*wrapper)(void *,
                                                               const void *,
                                                               Status *)) {
  return Handlers::EndMessageHandler::Make(wrapper, NULL, NULL);
}

template <class C, class D>
inline Handlers::EndMessageHandler BindHandler(
    bool (*wrapper)(void *, const void *, Status *),
    bool (*h)(C *, const D *, Status *), D *data) {
  UPB_UNUSED(h);  // Only for making sure function matches "D".
  return Handlers::EndMessageHandler::Make(wrapper, data,
                                           MatchDeleter(data).Delete);
}

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
inline typename Handlers::ValueHandler<T>::H MakeHandler(
    bool (*wrapper)(void *, const void *, T)) {
  return Handlers::ValueHandler<T>::H::Make(wrapper, NULL, NULL);
}

template <class C, class D, class T1, class T2>
inline typename Handlers::ValueHandler<T1>::H BindHandler(
    bool (*wrapper)(void *, const void *, T1), bool (*h)(C *, const D *, T2),
    D *data) {
  UPB_UNUSED(h);  // Only for making sure function matches "D".
  return Handlers::ValueHandler<T1>::H::Make(wrapper, data,
                                             MatchDeleter(data).Delete);
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

inline Handlers::StartFieldHandler MakeHandler(void *(*wrapper)(void *,
                                                                const void *)) {
  return Handlers::StartFieldHandler::Make(wrapper, NULL, NULL);
}

template <class R, class C, class D>
inline Handlers::StartFieldHandler BindHandler(
    void *(*wrapper)(void *, const void *), R *(*h)(C *, const D *), D *data) {
  UPB_UNUSED(h);  // Only for making sure function matches "D".
  return Handlers::StartFieldHandler::Make(wrapper, data,
                                           MatchDeleter(data).Delete);
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

inline Handlers::EndFieldHandler MakeHandler(bool (*wrapper)(void *,
                                                             const void *)) {
  return Handlers::EndFieldHandler::Make(wrapper, NULL, NULL);
}

template <class C, class D>
inline Handlers::EndFieldHandler BindHandler(
    bool (*wrapper)(void *, const void *), bool (*h)(C *, const D *), D *data) {
  UPB_UNUSED(h);  // Only for making sure function matches "D".
  return Handlers::EndFieldHandler::Make(wrapper, data,
                                         MatchDeleter(data).Delete);
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

inline Handlers::StartStringHandler MakeHandler(void *(*wrapper)(void *,
                                                                 const void *,
                                                                 size_t)) {
  return Handlers::StartStringHandler::Make(wrapper, NULL, NULL);
}

template <class R, class C, class D>
inline Handlers::StartStringHandler BindHandler(
    void *(*wrapper)(void *, const void *, size_t),
    R *(*h)(C *, const D *, size_t), D *data) {
  UPB_UNUSED(h);  // Only for making sure function matches "D".
  return Handlers::StartStringHandler::Make(wrapper, data,
                                            MatchDeleter(data).Delete);
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

inline Handlers::StringHandler MakeHandler(
    size_t (*wrapper)(void *, const void *, const char *, size_t)) {
  return Handlers::StringHandler::Make(wrapper, NULL, NULL);
}

template <class C, class D>
inline Handlers::StringHandler BindHandler(
    size_t (*wrapper)(void *, const void *, const char *, size_t),
    size_t (*h)(C *, const D *, const char *, size_t), D *data) {
  UPB_UNUSED(h);  // Only for making sure function matches "D".
  return Handlers::StringHandler::Make(wrapper, data,
                                       MatchDeleter(data).Delete);
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
      const Handlers::utype ## Handler& handler) {                             \
    assert(!handler.registered_);                                              \
    handler.registered_ = true;                                                \
    return upb_handlers_set##ltype(this, f, handler.handler_, handler.data_,   \
                                   handler.cleanup_);                          \
  }                                                                            \

TYPE_METHODS(Double, double, double,   double);
TYPE_METHODS(Float,  float,  float,    float);
TYPE_METHODS(UInt64, uint64, uint64_t, UPB_UINT64_T);
TYPE_METHODS(UInt32, uint32, uint32_t, UPB_UINT32_T);
TYPE_METHODS(Int64,  int64,  int64_t,  UPB_INT64_T);
TYPE_METHODS(Int32,  int32,  int32_t,  UPB_INT32_T);
TYPE_METHODS(Bool,   bool,   bool,     bool);

#ifdef UPB_TWO_32BIT_TYPES
TYPE_METHODS(Int32,  int32,  int32_t,  UPB_INT32ALT_T);
TYPE_METHODS(UInt32, uint32, uint32_t, UPB_UINT32ALT_T);
#endif

#ifdef UPB_TWO_64BIT_TYPES
TYPE_METHODS(Int64,  int64,  int64_t,  UPB_INT64ALT_T);
TYPE_METHODS(UInt64, uint64, uint64_t, UPB_UINT64ALT_T);
#endif
#undef TYPE_METHODS

// Type methods that are only one-per-canonical-type and not one-per-cvariant.

#define TYPE_METHODS(utype, ctype) \
    inline bool Handlers::Set##utype##Handler(const FieldDef *f, \
                                              const utype##Handler &h) { \
      return SetValueHandler<ctype>(f, h); \
    } \

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
inline const Status* Handlers::status() {
  return upb_handlers_status(this);
}
inline void Handlers::ClearError() {
  return upb_handlers_clearerr(this);
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
inline bool Handlers::SetStartMessageHandler(
    const Handlers::StartMessageHandler &handler) {
  assert(!handler.registered_);
  handler.registered_ = true;
  return upb_handlers_setstartmsg(this, handler.handler_, handler.data_,
                                  handler.cleanup_);
}
inline bool Handlers::SetEndMessageHandler(
    const Handlers::EndMessageHandler &handler) {
  assert(!handler.registered_);
  handler.registered_ = true;
  return upb_handlers_setendmsg(this, handler.handler_, handler.data_,
                                handler.cleanup_);
}
inline bool Handlers::SetStartStringHandler(const FieldDef *f,
                                            const StartStringHandler &handler) {
  assert(!handler.registered_);
  handler.registered_ = true;
  return upb_handlers_setstartstr(this, f, handler.handler_, handler.data_,
                                  handler.cleanup_);
}
inline bool Handlers::SetEndStringHandler(const FieldDef *f,
                                          const EndFieldHandler &handler) {
  assert(!handler.registered_);
  handler.registered_ = true;
  return upb_handlers_setendstr(this, f, handler.handler_, handler.data_,
                                handler.cleanup_);
}
inline bool Handlers::SetStringHandler(const FieldDef *f,
                                       const StringHandler& handler) {
  assert(!handler.registered_);
  handler.registered_ = true;
  return upb_handlers_setstring(this, f, handler.handler_, handler.data_,
                                handler.cleanup_);
}
inline bool Handlers::SetStartSequenceHandler(
    const FieldDef *f, const StartFieldHandler &handler) {
  assert(!handler.registered_);
  handler.registered_ = true;
  return upb_handlers_setstartseq(this, f, handler.handler_, handler.data_,
                                  handler.cleanup_);
}
inline bool Handlers::SetStartSubMessageHandler(
    const FieldDef *f, const StartFieldHandler &handler) {
  assert(!handler.registered_);
  handler.registered_ = true;
  return upb_handlers_setstartsubmsg(this, f, handler.handler_, handler.data_,
                                     handler.cleanup_);
}
inline bool Handlers::SetEndSubMessageHandler(const FieldDef *f,
                                              const EndFieldHandler &handler) {
  assert(!handler.registered_);
  handler.registered_ = true;
  return upb_handlers_setendsubmsg(this, f, handler.handler_, handler.data_,
                                   handler.cleanup_);
}
inline bool Handlers::SetEndSequenceHandler(const FieldDef *f,
                                            const EndFieldHandler &handler) {
  assert(!handler.registered_);
  handler.registered_ = true;
  return upb_handlers_setendseq(this, f, handler.handler_, handler.data_,
                                handler.cleanup_);
}
inline bool Handlers::SetSubHandlers(const FieldDef *f, const Handlers *sub) {
  return upb_handlers_setsubhandlers(this, f, sub);
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

#undef UPB_TWO_32BIT_TYPES
#undef UPB_TWO_64BIT_TYPES
#undef UPB_INT32_T
#undef UPB_UINT32_T
#undef UPB_INT32ALT_T
#undef UPB_UINT32ALT_T
#undef UPB_INT64_T
#undef UPB_UINT64_T
#undef UPB_INT64ALT_T
#undef UPB_UINT64ALT_T

#endif  // UPB_HANDLERS_INL_H_
