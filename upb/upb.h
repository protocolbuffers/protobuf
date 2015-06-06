/*
 * upb - a minimalist implementation of protocol buffers.
 *
 * Copyright (c) 2009 Google Inc.  See LICENSE for details.
 * Author: Josh Haberman <jhaberman@gmail.com>
 *
 * This file contains shared definitions that are widely used across upb.
 *
 * This is a mixed C/C++ interface that offers a full API to both languages.
 * See the top-level README for more information.
 */

#ifndef UPB_H_
#define UPB_H_

#include <assert.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>

/* UPB_INLINE: inline if possible, emit standalone code if required. */
#ifdef __cplusplus
#define UPB_INLINE inline
#elif defined (__GNUC__)
#define UPB_INLINE static __inline__
#else
#define UPB_INLINE static
#endif

/* Define UPB_BIG_ENDIAN manually if you're on big endian and your compiler
 * doesn't provide these preprocessor symbols. */
#if defined(__BYTE_ORDER__) && (__BYTE_ORDER__ == __ORDER_BIG_ENDIAN__)
#define UPB_BIG_ENDIAN
#endif

/* Macros for function attributes on compilers that support them. */
#ifdef __GNUC__
#define UPB_FORCEINLINE __inline__ __attribute__((always_inline))
#define UPB_NOINLINE __attribute__((noinline))
#define UPB_NORETURN __attribute__((__noreturn__))
#else  /* !defined(__GNUC__) */
#define UPB_FORCEINLINE
#define UPB_NOINLINE
#define UPB_NORETURN
#endif

/* A few hacky workarounds for functions not in C89.
 * For internal use only!
 * TODO(haberman): fix these by including our own implementations, or finding
 * another workaround.
 */
#ifdef __GNUC__
#define _upb_snprintf __builtin_snprintf
#define _upb_vsnprintf __builtin_vsnprintf
#define _upb_va_copy(a, b) __va_copy(a, b)
#elif __STDC_VERSION__ >= 199901L
/* C99 versions. */
#define _upb_snprintf snprintf
#define _upb_vsnprintf vsnprintf
#define _upb_va_copy(a, b) va_copy(a, b)
#else
#error Need implementations of [v]snprintf and va_copy
#endif


#if ((defined(__cplusplus) && __cplusplus >= 201103L) || \
      defined(__GXX_EXPERIMENTAL_CXX0X__)) && !defined(UPB_NO_CXX11)
#define UPB_CXX11
#endif

/* UPB_DISALLOW_COPY_AND_ASSIGN()
 * UPB_DISALLOW_POD_OPS()
 *
 * Declare these in the "private" section of a C++ class to forbid copy/assign
 * or all POD ops (construct, destruct, copy, assign) on that class. */
#ifdef UPB_CXX11
#include <type_traits>
#define UPB_DISALLOW_COPY_AND_ASSIGN(class_name) \
  class_name(const class_name&) = delete; \
  void operator=(const class_name&) = delete;
#define UPB_DISALLOW_POD_OPS(class_name, full_class_name) \
  class_name() = delete; \
  ~class_name() = delete; \
  UPB_DISALLOW_COPY_AND_ASSIGN(class_name)
#define UPB_ASSERT_STDLAYOUT(type) \
  static_assert(std::is_standard_layout<type>::value, \
                #type " must be standard layout");
#else  /* !defined(UPB_CXX11) */
#define UPB_DISALLOW_COPY_AND_ASSIGN(class_name) \
  class_name(const class_name&); \
  void operator=(const class_name&);
#define UPB_DISALLOW_POD_OPS(class_name, full_class_name) \
  class_name(); \
  ~class_name(); \
  UPB_DISALLOW_COPY_AND_ASSIGN(class_name)
#define UPB_ASSERT_STDLAYOUT(type)
#endif

/* UPB_DECLARE_TYPE()
 * UPB_DECLARE_DERIVED_TYPE()
 * UPB_DECLARE_DERIVED_TYPE2()
 *
 * Macros for declaring C and C++ types both, including inheritance.
 * The inheritance doesn't use real C++ inheritance, to stay compatible with C.
 *
 * These macros also provide upcasts:
 *  - in C: types-specific functions (ie. upb_foo_upcast(foo))
 *  - in C++: upb::upcast(foo) along with implicit conversions
 *
 * Downcasts are not provided, but upb/def.h defines downcasts for upb::Def. */

#define UPB_C_UPCASTS(ty, base)                                      \
  UPB_INLINE base *ty ## _upcast_mutable(ty *p) { return (base*)p; } \
  UPB_INLINE const base *ty ## _upcast(const ty *p) { return (const base*)p; }

#define UPB_C_UPCASTS2(ty, base, base2)                                 \
  UPB_C_UPCASTS(ty, base)                                               \
  UPB_INLINE base2 *ty ## _upcast2_mutable(ty *p) { return (base2*)p; } \
  UPB_INLINE const base2 *ty ## _upcast2(const ty *p) { return (const base2*)p; }

#ifdef __cplusplus

#define UPB_BEGIN_EXTERN_C extern "C" {
#define UPB_END_EXTERN_C }
#define UPB_PRIVATE_FOR_CPP private:
#define UPB_DECLARE_TYPE(cppname, cname) typedef cppname cname;

#define UPB_DECLARE_DERIVED_TYPE(cppname, cppbase, cname, cbase)  \
  UPB_DECLARE_TYPE(cppname, cname)                                \
  UPB_C_UPCASTS(cname, cbase)                                     \
  namespace upb {                                                 \
  template <>                                                     \
  class Pointer<cppname> : public PointerBase<cppname, cppbase> { \
   public:                                                        \
    explicit Pointer(cppname* ptr) : PointerBase(ptr) {}          \
  };                                                              \
  template <>                                                     \
  class Pointer<const cppname>                                    \
      : public PointerBase<const cppname, const cppbase> {        \
   public:                                                        \
    explicit Pointer(const cppname* ptr) : PointerBase(ptr) {}    \
  };                                                              \
  }

#define UPB_DECLARE_DERIVED_TYPE2(cppname, cppbase, cppbase2, cname, cbase,  \
                                  cbase2)                                    \
  UPB_DECLARE_TYPE(cppname, cname)                                           \
  UPB_C_UPCASTS2(cname, cbase, cbase2)                                       \
  namespace upb {                                                            \
  template <>                                                                \
  class Pointer<cppname> : public PointerBase2<cppname, cppbase, cppbase2> { \
   public:                                                                   \
    explicit Pointer(cppname* ptr) : PointerBase2(ptr) {}                    \
  };                                                                         \
  template <>                                                                \
  class Pointer<const cppname>                                               \
      : public PointerBase2<const cppname, const cppbase, const cppbase2> {  \
   public:                                                                   \
    explicit Pointer(const cppname* ptr) : PointerBase2(ptr) {}              \
  };                                                                         \
  }

#else  /* !defined(__cplusplus) */

#define UPB_BEGIN_EXTERN_C
#define UPB_END_EXTERN_C
#define UPB_PRIVATE_FOR_CPP
#define UPB_DECLARE_TYPE(cppname, cname) \
  struct cname;                          \
  typedef struct cname cname;
#define UPB_DECLARE_DERIVED_TYPE(cppname, cppbase, cname, cbase) \
  UPB_DECLARE_TYPE(cppname, cname)                               \
  UPB_C_UPCASTS(cname, cbase)
#define UPB_DECLARE_DERIVED_TYPE2(cppname, cppbase, cppbase2,    \
                                  cname, cbase, cbase2)          \
  UPB_DECLARE_TYPE(cppname, cname)                               \
  UPB_C_UPCASTS2(cname, cbase, cbase2)

#endif  /* defined(__cplusplus) */

#define UPB_MAX(x, y) ((x) > (y) ? (x) : (y))
#define UPB_MIN(x, y) ((x) < (y) ? (x) : (y))

#define UPB_UNUSED(var) (void)var

/* For asserting something about a variable when the variable is not used for
 * anything else.  This prevents "unused variable" warnings when compiling in
 * debug mode. */
#define UPB_ASSERT_VAR(var, predicate) UPB_UNUSED(var); assert(predicate)

/* Generic function type. */
typedef void upb_func();

/* C++ Casts ******************************************************************/

#ifdef __cplusplus

namespace upb {

template <class T> class Pointer;

/* Casts to a subclass.  The caller must know that cast is correct; an
 * incorrect cast will throw an assertion failure in debug mode.
 *
 * Example:
 *   upb::Def* def = GetDef();
 *   // Assert-fails if this was not actually a MessageDef.
 *   upb::MessgeDef* md = upb::down_cast<upb::MessageDef>(def);
 *
 * Note that downcasts are only defined for some types (at the moment you can
 * only downcast from a upb::Def to a specific Def type). */
template<class To, class From> To down_cast(From* f);

/* Casts to a subclass.  If the class does not actually match the given To type,
 * returns NULL.
 *
 * Example:
 *   upb::Def* def = GetDef();
 *   // md will be NULL if this was not actually a MessageDef.
 *   upb::MessgeDef* md = upb::down_cast<upb::MessageDef>(def);
 *
 * Note that dynamic casts are only defined for some types (at the moment you
 * can only downcast from a upb::Def to a specific Def type).. */
template<class To, class From> To dyn_cast(From* f);

/* Casts to any base class, or the type itself (ie. can be a no-op).
 *
 * Example:
 *   upb::MessageDef* md = GetDef();
 *   // This will fail to compile if this wasn't actually a base class.
 *   upb::Def* def = upb::upcast(md);
 */
template <class T> inline Pointer<T> upcast(T *f) { return Pointer<T>(f); }

/* Attempt upcast to specific base class.
 *
 * Example:
 *   upb::MessageDef* md = GetDef();
 *   upb::upcast_to<upb::Def>(md)->MethodOnDef();
 */
template <class T, class F> inline T* upcast_to(F *f) {
  return static_cast<T*>(upcast(f));
}

/* PointerBase<T>: implementation detail of upb::upcast().
 * It is implicitly convertable to pointers to the Base class(es).
 */
template <class T, class Base>
class PointerBase {
 public:
  explicit PointerBase(T* ptr) : ptr_(ptr) {}
  operator T*() { return ptr_; }
  operator Base*() { return (Base*)ptr_; }

 private:
  T* ptr_;
};

template <class T, class Base, class Base2>
class PointerBase2 : public PointerBase<T, Base> {
 public:
  explicit PointerBase2(T* ptr) : PointerBase<T, Base>(ptr) {}
  operator Base2*() { return Pointer<Base>(*this); }
};

}

#endif


/* upb::reffed_ptr ************************************************************/

#ifdef __cplusplus

#include <algorithm>  /* For std::swap(). */

namespace upb {

/* Provides RAII semantics for upb refcounted objects.  Each reffed_ptr owns a
 * ref on whatever object it points to (if any). */
template <class T> class reffed_ptr {
 public:
  reffed_ptr() : ptr_(NULL) {}

  /* If ref_donor is NULL, takes a new ref, otherwise adopts from ref_donor. */
  template <class U>
  reffed_ptr(U* val, const void* ref_donor = NULL)
      : ptr_(upb::upcast(val)) {
    if (ref_donor) {
      assert(ptr_);
      ptr_->DonateRef(ref_donor, this);
    } else if (ptr_) {
      ptr_->Ref(this);
    }
  }

  template <class U>
  reffed_ptr(const reffed_ptr<U>& other)
      : ptr_(upb::upcast(other.get())) {
    if (ptr_) ptr_->Ref(this);
  }

  ~reffed_ptr() { if (ptr_) ptr_->Unref(this); }

  template <class U>
  reffed_ptr& operator=(const reffed_ptr<U>& other) {
    reset(other.get());
    return *this;
  }

  reffed_ptr& operator=(const reffed_ptr& other) {
    reset(other.get());
    return *this;
  }

  /* TODO(haberman): add C++11 move construction/assignment for greater
   * efficiency. */

  void swap(reffed_ptr& other) {
    if (ptr_ == other.ptr_) {
      return;
    }

    if (ptr_) ptr_->DonateRef(this, &other);
    if (other.ptr_) other.ptr_->DonateRef(&other, this);
    std::swap(ptr_, other.ptr_);
  }

  T& operator*() const {
    assert(ptr_);
    return *ptr_;
  }

  T* operator->() const {
    assert(ptr_);
    return ptr_;
  }

  T* get() const { return ptr_; }

  /* If ref_donor is NULL, takes a new ref, otherwise adopts from ref_donor. */
  template <class U>
  void reset(U* ptr = NULL, const void* ref_donor = NULL) {
    reffed_ptr(ptr, ref_donor).swap(*this);
  }

  template <class U>
  reffed_ptr<U> down_cast() {
    return reffed_ptr<U>(upb::down_cast<U*>(get()));
  }

  template <class U>
  reffed_ptr<U> dyn_cast() {
    return reffed_ptr<U>(upb::dyn_cast<U*>(get()));
  }

  /* Plain release() is unsafe; if we were the only owner, it would leak the
   * object.  Instead we provide this: */
  T* ReleaseTo(const void* new_owner) {
    T* ret = NULL;
    ptr_->DonateRef(this, new_owner);
    std::swap(ret, ptr_);
    return ret;
  }

 private:
  T* ptr_;
};

}  /* namespace upb */

#endif  /* __cplusplus */


/* upb::Status ****************************************************************/

#ifdef __cplusplus
namespace upb {
class ErrorSpace;
class Status;
}
#endif

UPB_DECLARE_TYPE(upb::ErrorSpace, upb_errorspace)
UPB_DECLARE_TYPE(upb::Status, upb_status)

/* The maximum length of an error message before it will get truncated. */
#define UPB_STATUS_MAX_MESSAGE 128

/* An error callback function is used to report errors from some component.
 * The function can return "true" to indicate that the component should try
 * to recover and proceed, but this is not always possible. */
typedef bool upb_errcb_t(void *closure, const upb_status* status);

#ifdef __cplusplus
class upb::ErrorSpace {
#else
struct upb_errorspace {
#endif
  const char *name;
  /* Should the error message in the status object according to this code. */
  void (*set_message)(upb_status* status, int code);
};

#ifdef __cplusplus

/* Object representing a success or failure status.
 * It owns no resources and allocates no memory, so it should work
 * even in OOM situations. */

class upb::Status {
 public:
  Status();

  /* Returns true if there is no error. */
  bool ok() const;

  /* Optional error space and code, useful if the caller wants to
   * programmatically check the specific kind of error. */
  ErrorSpace* error_space();
  int code() const;

  const char *error_message() const;

  /* The error message will be truncated if it is longer than
   * UPB_STATUS_MAX_MESSAGE-4. */
  void SetErrorMessage(const char* msg);
  void SetFormattedErrorMessage(const char* fmt, ...);

  /* If there is no error message already, this will use the ErrorSpace to
   * populate the error message for this code.  The caller can still call
   * SetErrorMessage() to give a more specific message. */
  void SetErrorCode(ErrorSpace* space, int code);

  /* Resets the status to a successful state with no message. */
  void Clear();

  void CopyFrom(const Status& other);

 private:
  UPB_DISALLOW_COPY_AND_ASSIGN(Status)
#else
struct upb_status {
#endif
  bool ok_;

  /* Specific status code defined by some error space (optional). */
  int code_;
  upb_errorspace *error_space_;

  /* Error message; NULL-terminated. */
  char msg[UPB_STATUS_MAX_MESSAGE];
};

#define UPB_STATUS_INIT {true, 0, NULL, {0}}

#ifdef __cplusplus
extern "C" {
#endif

/* The returned string is invalidated by any other call into the status. */
const char *upb_status_errmsg(const upb_status *status);
bool upb_ok(const upb_status *status);
upb_errorspace *upb_status_errspace(const upb_status *status);
int upb_status_errcode(const upb_status *status);

/* Any of the functions that write to a status object allow status to be NULL,
 * to support use cases where the function's caller does not care about the
 * status message. */
void upb_status_clear(upb_status *status);
void upb_status_seterrmsg(upb_status *status, const char *msg);
void upb_status_seterrf(upb_status *status, const char *fmt, ...);
void upb_status_vseterrf(upb_status *status, const char *fmt, va_list args);
void upb_status_seterrcode(upb_status *status, upb_errorspace *space, int code);
void upb_status_copy(upb_status *to, const upb_status *from);

#ifdef __cplusplus
}  /* extern "C" */

namespace upb {

/* C++ Wrappers */
inline Status::Status() { Clear(); }
inline bool Status::ok() const { return upb_ok(this); }
inline const char* Status::error_message() const {
  return upb_status_errmsg(this);
}
inline void Status::SetErrorMessage(const char* msg) {
  upb_status_seterrmsg(this, msg);
}
inline void Status::SetFormattedErrorMessage(const char* fmt, ...) {
  va_list args;
  va_start(args, fmt);
  upb_status_vseterrf(this, fmt, args);
  va_end(args);
}
inline void Status::SetErrorCode(ErrorSpace* space, int code) {
  upb_status_seterrcode(this, space, code);
}
inline void Status::Clear() { upb_status_clear(this); }
inline void Status::CopyFrom(const Status& other) {
  upb_status_copy(this, &other);
}

}  /* namespace upb */

#endif

#endif  /* UPB_H_ */
