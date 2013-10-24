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
#define UPB_DISALLOW_POD_OPS(class_name) \
  class_name() = delete; \
  ~class_name() = delete; \
  class_name(const class_name&) = delete; \
  void operator=(const class_name&) = delete;
#else
#define UPB_DISALLOW_POD_OPS(class_name) \
  class_name(); \
  ~class_name(); \
  class_name(const class_name&); \
  void operator=(const class_name&);
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
//
// For upcasts, see the Upcast() method in the types themselves.

namespace upb {

// Casts to a direct subclass.  The caller must know that cast is correct; an
// incorrect cast will throw an assertion failure.
template<class To, class From> To down_cast(From* f);

// Casts to a direct subclass.  If the class does not actually match the given
// subtype, returns NULL.
template<class To, class From> To dyn_cast(From* f);

}

#endif


/* upb::Status ****************************************************************/

#ifdef __cplusplus
namespace upb { class Status; }
typedef upb::Status upb_status;
#else
struct upb_status;
typedef struct upb_status upb_status;
#endif

typedef enum {
  UPB_OK,          // The operation completed successfully.
  UPB_SUSPENDED,   // The operation was suspended and may be resumed later.
  UPB_ERROR,       // An error occurred.
} upb_success_t;

typedef struct {
  const char *name;
  // Writes a NULL-terminated string to "buf" containing an error message for
  // the given error code, returning false if the message was too large to fit.
  bool (*code_to_string)(int code, char *buf, size_t len);
} upb_errorspace;

#ifdef __cplusplus

class upb::Status {
 public:
  typedef upb_success_t Success;

  Status();
  ~Status();

  bool ok() const;
  bool eof() const;

  const char *GetString() const;
  void SetEof();
  void SetErrorLiteral(const char* msg);
  void Clear();

 private:
#else
struct upb_status {
#endif
  bool error;
  bool eof_;

  // Specific status code defined by some error space (optional).
  int code;
  upb_errorspace *space;

  // Error message (optional).
  const char *str;  // NULL when no message is present.  NULL-terminated.
  char *buf;        // Owned by the status.
  size_t bufsize;
};

#define UPB_STATUS_INIT {UPB_OK, false, 0, NULL, NULL, NULL, 0}

#ifdef __cplusplus
extern "C" {
#endif

void upb_status_init(upb_status *status);
void upb_status_uninit(upb_status *status);

bool upb_ok(const upb_status *status);
bool upb_eof(const upb_status *status);

// Any of the functions that write to a status object allow status to be NULL,
// to support use cases where the function's caller does not care about the
// status message.
void upb_status_clear(upb_status *status);
void upb_status_seterrliteral(upb_status *status, const char *msg);
void upb_status_seterrf(upb_status *status, const char *msg, ...);
void upb_status_setcode(upb_status *status, upb_errorspace *space, int code);
void upb_status_seteof(upb_status *status);
// The returned string is invalidated by any other call into the status.
const char *upb_status_getstr(const upb_status *status);
void upb_status_copy(upb_status *to, const upb_status *from);

#ifdef __cplusplus
}  // extern "C"

namespace upb {

// C++ Wrappers
inline Status::Status() { upb_status_init(this); }
inline Status::~Status() { upb_status_uninit(this); }
inline bool Status::ok() const { return upb_ok(this); }
inline bool Status::eof() const { return upb_eof(this); }
inline const char *Status::GetString() const { return upb_status_getstr(this); }
inline void Status::SetEof() { upb_status_seteof(this); }
inline void Status::SetErrorLiteral(const char* msg) {
  upb_status_seterrliteral(this, msg);
}
inline void Status::Clear() { upb_status_clear(this); }

}  // namespace upb

#endif

#endif  /* UPB_H_ */
