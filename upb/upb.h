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

// inline if possible, emit standalone code if required.
#ifdef __cplusplus
#define UPB_INLINE inline
#else
#define UPB_INLINE static inline
#endif

#if __STDC_VERSION__ >= 199901L
#define UPB_C99
#endif

#if ((defined(__cplusplus) && __cplusplus >= 201103L) || \
      defined(__GXX_EXPERIMENTAL_CXX0X__)) && !defined(UPB_NO_CXX11)
#define UPB_CXX11
#endif

#ifdef UPB_CXX11
#define UPB_DISALLOW_COPY_AND_ASSIGN(class_name) \
  class_name(const class_name&) = delete; \
  void operator=(const class_name&) = delete;
#define UPB_DISALLOW_POD_OPS(class_name, full_class_name) \
  class_name() = delete; \
  ~class_name() = delete; \
  /* Friend Pointer<T> so it can access base class. */ \
  friend class Pointer<full_class_name>; \
  friend class Pointer<const full_class_name>; \
  UPB_DISALLOW_COPY_AND_ASSIGN(class_name)
#else
#define UPB_DISALLOW_COPY_AND_ASSIGN(class_name) \
  class_name(const class_name&); \
  void operator=(const class_name&);
#define UPB_DISALLOW_POD_OPS(class_name, full_class_name) \
  class_name(); \
  ~class_name(); \
  /* Friend Pointer<T> so it can access base class. */ \
  friend class Pointer<full_class_name>; \
  friend class Pointer<const full_class_name>; \
  UPB_DISALLOW_COPY_AND_ASSIGN(class_name)
#endif

#ifdef __cplusplus
#define UPB_PRIVATE_FOR_CPP private:
#else
#define UPB_PRIVATE_FOR_CPP
#endif

#ifdef __GNUC__
#define UPB_NORETURN __attribute__((__noreturn__))
#else
#define UPB_NORETURN
#endif

#define UPB_MAX(x, y) ((x) > (y) ? (x) : (y))
#define UPB_MIN(x, y) ((x) < (y) ? (x) : (y))

#define UPB_UNUSED(var) (void)var

// For asserting something about a variable when the variable is not used for
// anything else.  This prevents "unused variable" warnings when compiling in
// debug mode.
#define UPB_ASSERT_VAR(var, predicate) UPB_UNUSED(var); assert(predicate)


/* Casts **********************************************************************/

// Upcasts for C.  For downcasts see the definitions of the subtypes.
#define UPB_UPCAST(obj) (&(obj)->base)
#define UPB_UPCAST2(obj) UPB_UPCAST(UPB_UPCAST(obj))

#ifdef __cplusplus

// Downcasts for C++.  We can't use C++ inheritance directly and maintain
// compatibility with C.  So our inheritance is undeclared in C++.
// Specializations of these casting functions are defined for appropriate type
// pairs, and perform the necessary checks.
//
// Example:
//   upb::Def* def = <...>;
//   upb::MessageDef* = upb::dyn_cast<upb::MessageDef*>(def);

namespace upb {

// Casts to a direct subclass.  The caller must know that cast is correct; an
// incorrect cast will throw an assertion failure in debug mode.
template<class To, class From> To down_cast(From* f);

// Casts to a direct subclass.  If the class does not actually match the given
// subtype, returns NULL.
template<class To, class From> To dyn_cast(From* f);

// Pointer<T> is a simple wrapper around a T*.  It is only constructed for
// upcast() below, and its sole purpose is to be implicitly convertable to T* or
// pointers to base classes, just as a pointer would be in regular C++ if the
// inheritance were directly expressed as C++ inheritance.
template <class T> class Pointer;

// Casts to any base class, or the type itself (ie. can be a no-op).
template <class T> inline Pointer<T> upcast(T *f) { return Pointer<T>(f); }
}

#endif


/* upb::reffed_ptr ************************************************************/

#ifdef __cplusplus

#include <algorithm>  // For std::swap().

namespace upb {

// Provides RAII semantics for upb refcounted objects.  Each reffed_ptr owns a
// ref on whatever object it points to (if any).
template <class T> class reffed_ptr {
 public:
  reffed_ptr() : ptr_(NULL) {}

  // If ref_donor is NULL, takes a new ref, otherwise adopts from ref_donor.
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

  // TODO(haberman): add C++11 move construction/assignment for greater
  // efficiency.

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

  // If ref_donor is NULL, takes a new ref, otherwise adopts from ref_donor.
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

  // Plain release() is unsafe; if we were the only owner, it would leak the
  // object.  Instead we provide this:
  T* ReleaseTo(const void* new_owner) {
    T* ret = NULL;
    ptr_->DonateRef(this, new_owner);
    std::swap(ret, ptr_);
    return ret;
  }

 private:
  T* ptr_;
};

}  // namespace upb

#endif  // __cplusplus


/* upb::Status ****************************************************************/

#ifdef __cplusplus
namespace upb { class Status; }
typedef upb::Status upb_status;
#else
struct upb_status;
typedef struct upb_status upb_status;
#endif

// The maximum length of an error message before it will get truncated.
#define UPB_STATUS_MAX_MESSAGE 128

// An error callback function is used to report errors from some component.
// The function can return "true" to indicate that the component should try
// to recover and proceed, but this is not always possible.
typedef bool upb_errcb_t(void *closure, const upb_status* status);

typedef struct {
  const char *name;
  // Should the error message in the status object according to this code.
  void (*set_message)(upb_status* status, int code);
} upb_errorspace;

#ifdef __cplusplus

typedef upb_errorspace ErrorSpace;

// Object representing a success or failure status.
// It owns no resources and allocates no memory, so it should work
// even in OOM situations.
class upb::Status {
 public:
  Status();

  // Returns true if there is no error.
  bool ok() const;

  // Optional error space and code, useful if the caller wants to
  // programmatically check the specific kind of error.
  ErrorSpace* error_space();
  int code() const;

  const char *error_message() const;

  // The error message will be truncated if it is longer than
  // UPB_STATUS_MAX_MESSAGE-4.
  void SetErrorMessage(const char* msg);
  void SetFormattedErrorMessage(const char* fmt, ...);

  // If there is no error message already, this will use the ErrorSpace to
  // populate the error message for this code.  The caller can still call
  // SetErrorMessage() to give a more specific message.
  void SetErrorCode(ErrorSpace* space, int code);

  // Resets the status to a successful state with no message.
  void Clear();

  void CopyFrom(const Status& other);

 private:
  UPB_DISALLOW_COPY_AND_ASSIGN(Status);
#else
struct upb_status {
#endif
  bool ok_;

  // Specific status code defined by some error space (optional).
  int code_;
  upb_errorspace *error_space_;

  // Error message; NULL-terminated.
  char msg[UPB_STATUS_MAX_MESSAGE];
};

#define UPB_STATUS_INIT {true, 0, NULL, {0}}

#ifdef __cplusplus
extern "C" {
#endif

// The returned string is invalidated by any other call into the status.
const char *upb_status_errmsg(const upb_status *status);
bool upb_ok(const upb_status *status);
upb_errorspace *upb_status_errspace(const upb_status *status);
int upb_status_errcode(const upb_status *status);

// Any of the functions that write to a status object allow status to be NULL,
// to support use cases where the function's caller does not care about the
// status message.
void upb_status_clear(upb_status *status);
void upb_status_seterrmsg(upb_status *status, const char *msg);
void upb_status_seterrf(upb_status *status, const char *fmt, ...);
void upb_status_vseterrf(upb_status *status, const char *fmt, va_list args);
void upb_status_seterrcode(upb_status *status, upb_errorspace *space, int code);
void upb_status_copy(upb_status *to, const upb_status *from);

#ifdef __cplusplus
}  // extern "C"

namespace upb {

// C++ Wrappers
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

}  // namespace upb

#endif

#endif  /* UPB_H_ */
