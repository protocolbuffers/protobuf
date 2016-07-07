// Amalgamated source file
/*
** Defs are upb's internal representation of the constructs that can appear
** in a .proto file:
**
** - upb::MessageDef (upb_msgdef): describes a "message" construct.
** - upb::FieldDef (upb_fielddef): describes a message field.
** - upb::EnumDef (upb_enumdef): describes an enum.
** - upb::OneofDef (upb_oneofdef): describes a oneof.
** - upb::Def (upb_def): base class of all the others.
**
** TODO: definitions of services.
**
** Like upb_refcounted objects, defs are mutable only until frozen, and are
** only thread-safe once frozen.
**
** This is a mixed C/C++ interface that offers a full API to both languages.
** See the top-level README for more information.
*/

#ifndef UPB_DEF_H_
#define UPB_DEF_H_

/*
** upb::RefCounted (upb_refcounted)
**
** A refcounting scheme that supports circular refs.  It accomplishes this by
** partitioning the set of objects into groups such that no cycle spans groups;
** we can then reference-count the group as a whole and ignore refs within the
** group.  When objects are mutable, these groups are computed very
** conservatively; we group any objects that have ever had a link between them.
** When objects are frozen, we compute strongly-connected components which
** allows us to be precise and only group objects that are actually cyclic.
**
** This is a mixed C/C++ interface that offers a full API to both languages.
** See the top-level README for more information.
*/

#ifndef UPB_REFCOUNTED_H_
#define UPB_REFCOUNTED_H_

/*
** upb_table
**
** This header is INTERNAL-ONLY!  Its interfaces are not public or stable!
** This file defines very fast int->upb_value (inttable) and string->upb_value
** (strtable) hash tables.
**
** The table uses chained scatter with Brent's variation (inspired by the Lua
** implementation of hash tables).  The hash function for strings is Austin
** Appleby's "MurmurHash."
**
** The inttable uses uintptr_t as its key, which guarantees it can be used to
** store pointers or integers of at least 32 bits (upb isn't really useful on
** systems where sizeof(void*) < 4).
**
** The table must be homogeneous (all values of the same type).  In debug
** mode, we check this on insert and lookup.
*/

#ifndef UPB_TABLE_H_
#define UPB_TABLE_H_

#include <assert.h>
#include <stdint.h>
#include <string.h>
/*
** This file contains shared definitions that are widely used across upb.
**
** This is a mixed C/C++ interface that offers a full API to both languages.
** See the top-level README for more information.
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

#ifdef __cplusplus
extern "C" {
#endif


/* upb_value ******************************************************************/

/* A tagged union (stored untagged inside the table) so that we can check that
 * clients calling table accessors are correctly typed without having to have
 * an explosion of accessors. */
typedef enum {
  UPB_CTYPE_INT32    = 1,
  UPB_CTYPE_INT64    = 2,
  UPB_CTYPE_UINT32   = 3,
  UPB_CTYPE_UINT64   = 4,
  UPB_CTYPE_BOOL     = 5,
  UPB_CTYPE_CSTR     = 6,
  UPB_CTYPE_PTR      = 7,
  UPB_CTYPE_CONSTPTR = 8,
  UPB_CTYPE_FPTR     = 9
} upb_ctype_t;

typedef struct {
  uint64_t val;
#ifndef NDEBUG
  /* In debug mode we carry the value type around also so we can check accesses
   * to be sure the right member is being read. */
  upb_ctype_t ctype;
#endif
} upb_value;

#ifdef NDEBUG
#define SET_TYPE(dest, val)      UPB_UNUSED(val)
#else
#define SET_TYPE(dest, val) dest = val
#endif

/* Like strdup(), which isn't always available since it's not ANSI C. */
char *upb_strdup(const char *s);
/* Variant that works with a length-delimited rather than NULL-delimited string,
 * as supported by strtable. */
char *upb_strdup2(const char *s, size_t len);

UPB_INLINE void _upb_value_setval(upb_value *v, uint64_t val,
                                  upb_ctype_t ctype) {
  v->val = val;
  SET_TYPE(v->ctype, ctype);
}

UPB_INLINE upb_value _upb_value_val(uint64_t val, upb_ctype_t ctype) {
  upb_value ret;
  _upb_value_setval(&ret, val, ctype);
  return ret;
}

/* For each value ctype, define the following set of functions:
 *
 * // Get/set an int32 from a upb_value.
 * int32_t upb_value_getint32(upb_value val);
 * void upb_value_setint32(upb_value *val, int32_t cval);
 *
 * // Construct a new upb_value from an int32.
 * upb_value upb_value_int32(int32_t val); */
#define FUNCS(name, membername, type_t, converter, proto_type) \
  UPB_INLINE void upb_value_set ## name(upb_value *val, type_t cval) { \
    val->val = (converter)cval; \
    SET_TYPE(val->ctype, proto_type); \
  } \
  UPB_INLINE upb_value upb_value_ ## name(type_t val) { \
    upb_value ret; \
    upb_value_set ## name(&ret, val); \
    return ret; \
  } \
  UPB_INLINE type_t upb_value_get ## name(upb_value val) { \
    assert(val.ctype == proto_type); \
    return (type_t)(converter)val.val; \
  }

FUNCS(int32,    int32,        int32_t,      int32_t,    UPB_CTYPE_INT32)
FUNCS(int64,    int64,        int64_t,      int64_t,    UPB_CTYPE_INT64)
FUNCS(uint32,   uint32,       uint32_t,     uint32_t,   UPB_CTYPE_UINT32)
FUNCS(uint64,   uint64,       uint64_t,     uint64_t,   UPB_CTYPE_UINT64)
FUNCS(bool,     _bool,        bool,         bool,       UPB_CTYPE_BOOL)
FUNCS(cstr,     cstr,         char*,        uintptr_t,  UPB_CTYPE_CSTR)
FUNCS(ptr,      ptr,          void*,        uintptr_t,  UPB_CTYPE_PTR)
FUNCS(constptr, constptr,     const void*,  uintptr_t,  UPB_CTYPE_CONSTPTR)
FUNCS(fptr,     fptr,         upb_func*,    uintptr_t,  UPB_CTYPE_FPTR)

#undef FUNCS
#undef SET_TYPE


/* upb_tabkey *****************************************************************/

/* Either:
 *   1. an actual integer key, or
 *   2. a pointer to a string prefixed by its uint32_t length, owned by us.
 *
 * ...depending on whether this is a string table or an int table.  We would
 * make this a union of those two types, but C89 doesn't support statically
 * initializing a non-first union member. */
typedef uintptr_t upb_tabkey;

#define UPB_TABKEY_NUM(n) n
#define UPB_TABKEY_NONE 0
/* The preprocessor isn't quite powerful enough to turn the compile-time string
 * length into a byte-wise string representation, so code generation needs to
 * help it along.
 *
 * "len1" is the low byte and len4 is the high byte. */
#ifdef UPB_BIG_ENDIAN
#define UPB_TABKEY_STR(len1, len2, len3, len4, strval) \
    (uintptr_t)(len4 len3 len2 len1 strval)
#else
#define UPB_TABKEY_STR(len1, len2, len3, len4, strval) \
    (uintptr_t)(len1 len2 len3 len4 strval)
#endif

UPB_INLINE char *upb_tabstr(upb_tabkey key, uint32_t *len) {
  char* mem = (char*)key;
  if (len) memcpy(len, mem, sizeof(*len));
  return mem + sizeof(*len);
}


/* upb_tabval *****************************************************************/

#ifdef __cplusplus

/* Status initialization not supported.
 *
 * This separate definition is necessary because in C++, UINTPTR_MAX isn't
 * reliably available. */
typedef struct {
  uint64_t val;
} upb_tabval;

#else

/* C -- supports static initialization, but to support static initialization of
 * both integers and points for both 32 and 64 bit targets, it takes a little
 * bit of doing. */

#if UINTPTR_MAX == 0xffffffffffffffffULL
#define UPB_PTR_IS_64BITS
#elif UINTPTR_MAX != 0xffffffff
#error Could not determine how many bits pointers are.
#endif

typedef union {
  /* For static initialization.
   *
   * Unfortunately this ugliness is necessary -- it is the only way that we can,
   * with -std=c89 -pedantic, statically initialize this to either a pointer or
   * an integer on 32-bit platforms. */
  struct {
#ifdef UPB_PTR_IS_64BITS
    uintptr_t val;
#else
    uintptr_t val1;
    uintptr_t val2;
#endif
  } staticinit;

  /* The normal accessor that we use for everything at runtime. */
  uint64_t val;
} upb_tabval;

#ifdef UPB_PTR_IS_64BITS
#define UPB_TABVALUE_INT_INIT(v) {{v}}
#define UPB_TABVALUE_EMPTY_INIT  {{-1}}
#else

/* 32-bit pointers */

#ifdef UPB_BIG_ENDIAN
#define UPB_TABVALUE_INT_INIT(v) {{0, v}}
#define UPB_TABVALUE_EMPTY_INIT  {{-1, -1}}
#else
#define UPB_TABVALUE_INT_INIT(v) {{v, 0}}
#define UPB_TABVALUE_EMPTY_INIT  {{-1, -1}}
#endif

#endif

#define UPB_TABVALUE_PTR_INIT(v) UPB_TABVALUE_INT_INIT((uintptr_t)v)

#undef UPB_PTR_IS_64BITS

#endif  /* __cplusplus */


/* upb_table ******************************************************************/

typedef struct _upb_tabent {
  upb_tabkey key;
  upb_tabval val;

  /* Internal chaining.  This is const so we can create static initializers for
   * tables.  We cast away const sometimes, but *only* when the containing
   * upb_table is known to be non-const.  This requires a bit of care, but
   * the subtlety is confined to table.c. */
  const struct _upb_tabent *next;
} upb_tabent;

typedef struct {
  size_t count;          /* Number of entries in the hash part. */
  size_t mask;           /* Mask to turn hash value -> bucket. */
  upb_ctype_t ctype;     /* Type of all values. */
  uint8_t size_lg2;      /* Size of the hashtable part is 2^size_lg2 entries. */

  /* Hash table entries.
   * Making this const isn't entirely accurate; what we really want is for it to
   * have the same const-ness as the table it's inside.  But there's no way to
   * declare that in C.  So we have to make it const so that we can statically
   * initialize const hash tables.  Then we cast away const when we have to.
   */
  const upb_tabent *entries;
} upb_table;

typedef struct {
  upb_table t;
} upb_strtable;

#define UPB_STRTABLE_INIT(count, mask, ctype, size_lg2, entries) \
  {{count, mask, ctype, size_lg2, entries}}

#define UPB_EMPTY_STRTABLE_INIT(ctype)                           \
  UPB_STRTABLE_INIT(0, 0, ctype, 0, NULL)

typedef struct {
  upb_table t;              /* For entries that don't fit in the array part. */
  const upb_tabval *array;  /* Array part of the table. See const note above. */
  size_t array_size;        /* Array part size. */
  size_t array_count;       /* Array part number of elements. */
} upb_inttable;

#define UPB_INTTABLE_INIT(count, mask, ctype, size_lg2, ent, a, asize, acount) \
  {{count, mask, ctype, size_lg2, ent}, a, asize, acount}

#define UPB_EMPTY_INTTABLE_INIT(ctype) \
  UPB_INTTABLE_INIT(0, 0, ctype, 0, NULL, NULL, 0, 0)

#define UPB_ARRAY_EMPTYENT -1

UPB_INLINE size_t upb_table_size(const upb_table *t) {
  if (t->size_lg2 == 0)
    return 0;
  else
    return 1 << t->size_lg2;
}

/* Internal-only functions, in .h file only out of necessity. */
UPB_INLINE bool upb_tabent_isempty(const upb_tabent *e) {
  return e->key == 0;
}

/* Used by some of the unit tests for generic hashing functionality. */
uint32_t MurmurHash2(const void * key, size_t len, uint32_t seed);

UPB_INLINE uintptr_t upb_intkey(uintptr_t key) {
  return key;
}

UPB_INLINE uint32_t upb_inthash(uintptr_t key) {
  return (uint32_t)key;
}

static const upb_tabent *upb_getentry(const upb_table *t, uint32_t hash) {
  return t->entries + (hash & t->mask);
}

UPB_INLINE bool upb_arrhas(upb_tabval key) {
  return key.val != (uint64_t)-1;
}

/* Initialize and uninitialize a table, respectively.  If memory allocation
 * failed, false is returned that the table is uninitialized. */
bool upb_inttable_init(upb_inttable *table, upb_ctype_t ctype);
bool upb_strtable_init(upb_strtable *table, upb_ctype_t ctype);
void upb_inttable_uninit(upb_inttable *table);
void upb_strtable_uninit(upb_strtable *table);

/* Returns the number of values in the table. */
size_t upb_inttable_count(const upb_inttable *t);
UPB_INLINE size_t upb_strtable_count(const upb_strtable *t) {
  return t->t.count;
}

/* Inserts the given key into the hashtable with the given value.  The key must
 * not already exist in the hash table.  For string tables, the key must be
 * NULL-terminated, and the table will make an internal copy of the key.
 * Inttables must not insert a value of UINTPTR_MAX.
 *
 * If a table resize was required but memory allocation failed, false is
 * returned and the table is unchanged. */
bool upb_inttable_insert(upb_inttable *t, uintptr_t key, upb_value val);
bool upb_strtable_insert2(upb_strtable *t, const char *key, size_t len,
                          upb_value val);

/* For NULL-terminated strings. */
UPB_INLINE bool upb_strtable_insert(upb_strtable *t, const char *key,
                                    upb_value val) {
  return upb_strtable_insert2(t, key, strlen(key), val);
}

/* Looks up key in this table, returning "true" if the key was found.
 * If v is non-NULL, copies the value for this key into *v. */
bool upb_inttable_lookup(const upb_inttable *t, uintptr_t key, upb_value *v);
bool upb_strtable_lookup2(const upb_strtable *t, const char *key, size_t len,
                          upb_value *v);

/* For NULL-terminated strings. */
UPB_INLINE bool upb_strtable_lookup(const upb_strtable *t, const char *key,
                                    upb_value *v) {
  return upb_strtable_lookup2(t, key, strlen(key), v);
}

/* Removes an item from the table.  Returns true if the remove was successful,
 * and stores the removed item in *val if non-NULL. */
bool upb_inttable_remove(upb_inttable *t, uintptr_t key, upb_value *val);
bool upb_strtable_remove2(upb_strtable *t, const char *key, size_t len,
                          upb_value *val);

/* For NULL-terminated strings. */
UPB_INLINE bool upb_strtable_remove(upb_strtable *t, const char *key,
                                    upb_value *v) {
  return upb_strtable_remove2(t, key, strlen(key), v);
}

/* Updates an existing entry in an inttable.  If the entry does not exist,
 * returns false and does nothing.  Unlike insert/remove, this does not
 * invalidate iterators. */
bool upb_inttable_replace(upb_inttable *t, uintptr_t key, upb_value val);

/* Handy routines for treating an inttable like a stack.  May not be mixed with
 * other insert/remove calls. */
bool upb_inttable_push(upb_inttable *t, upb_value val);
upb_value upb_inttable_pop(upb_inttable *t);

/* Convenience routines for inttables with pointer keys. */
bool upb_inttable_insertptr(upb_inttable *t, const void *key, upb_value val);
bool upb_inttable_removeptr(upb_inttable *t, const void *key, upb_value *val);
bool upb_inttable_lookupptr(
    const upb_inttable *t, const void *key, upb_value *val);

/* Optimizes the table for the current set of entries, for both memory use and
 * lookup time.  Client should call this after all entries have been inserted;
 * inserting more entries is legal, but will likely require a table resize. */
void upb_inttable_compact(upb_inttable *t);

/* A special-case inlinable version of the lookup routine for 32-bit
 * integers. */
UPB_INLINE bool upb_inttable_lookup32(const upb_inttable *t, uint32_t key,
                                      upb_value *v) {
  *v = upb_value_int32(0);  /* Silence compiler warnings. */
  if (key < t->array_size) {
    upb_tabval arrval = t->array[key];
    if (upb_arrhas(arrval)) {
      _upb_value_setval(v, arrval.val, t->t.ctype);
      return true;
    } else {
      return false;
    }
  } else {
    const upb_tabent *e;
    if (t->t.entries == NULL) return false;
    for (e = upb_getentry(&t->t, upb_inthash(key)); true; e = e->next) {
      if ((uint32_t)e->key == key) {
        _upb_value_setval(v, e->val.val, t->t.ctype);
        return true;
      }
      if (e->next == NULL) return false;
    }
  }
}

/* Exposed for testing only. */
bool upb_strtable_resize(upb_strtable *t, size_t size_lg2);

/* Iterators ******************************************************************/

/* Iterators for int and string tables.  We are subject to some kind of unusual
 * design constraints:
 *
 * For high-level languages:
 *  - we must be able to guarantee that we don't crash or corrupt memory even if
 *    the program accesses an invalidated iterator.
 *
 * For C++11 range-based for:
 *  - iterators must be copyable
 *  - iterators must be comparable
 *  - it must be possible to construct an "end" value.
 *
 * Iteration order is undefined.
 *
 * Modifying the table invalidates iterators.  upb_{str,int}table_done() is
 * guaranteed to work even on an invalidated iterator, as long as the table it
 * is iterating over has not been freed.  Calling next() or accessing data from
 * an invalidated iterator yields unspecified elements from the table, but it is
 * guaranteed not to crash and to return real table elements (except when done()
 * is true). */


/* upb_strtable_iter **********************************************************/

/*   upb_strtable_iter i;
 *   upb_strtable_begin(&i, t);
 *   for(; !upb_strtable_done(&i); upb_strtable_next(&i)) {
 *     const char *key = upb_strtable_iter_key(&i);
 *     const upb_value val = upb_strtable_iter_value(&i);
 *     // ...
 *   }
 */

typedef struct {
  const upb_strtable *t;
  size_t index;
} upb_strtable_iter;

void upb_strtable_begin(upb_strtable_iter *i, const upb_strtable *t);
void upb_strtable_next(upb_strtable_iter *i);
bool upb_strtable_done(const upb_strtable_iter *i);
const char *upb_strtable_iter_key(upb_strtable_iter *i);
size_t upb_strtable_iter_keylength(upb_strtable_iter *i);
upb_value upb_strtable_iter_value(const upb_strtable_iter *i);
void upb_strtable_iter_setdone(upb_strtable_iter *i);
bool upb_strtable_iter_isequal(const upb_strtable_iter *i1,
                               const upb_strtable_iter *i2);


/* upb_inttable_iter **********************************************************/

/*   upb_inttable_iter i;
 *   upb_inttable_begin(&i, t);
 *   for(; !upb_inttable_done(&i); upb_inttable_next(&i)) {
 *     uintptr_t key = upb_inttable_iter_key(&i);
 *     upb_value val = upb_inttable_iter_value(&i);
 *     // ...
 *   }
 */

typedef struct {
  const upb_inttable *t;
  size_t index;
  bool array_part;
} upb_inttable_iter;

void upb_inttable_begin(upb_inttable_iter *i, const upb_inttable *t);
void upb_inttable_next(upb_inttable_iter *i);
bool upb_inttable_done(const upb_inttable_iter *i);
uintptr_t upb_inttable_iter_key(const upb_inttable_iter *i);
upb_value upb_inttable_iter_value(const upb_inttable_iter *i);
void upb_inttable_iter_setdone(upb_inttable_iter *i);
bool upb_inttable_iter_isequal(const upb_inttable_iter *i1,
                               const upb_inttable_iter *i2);


#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif  /* UPB_TABLE_H_ */

/* Reference tracking will check ref()/unref() operations to make sure the
 * ref ownership is correct.  Where possible it will also make tools like
 * Valgrind attribute ref leaks to the code that took the leaked ref, not
 * the code that originally created the object.
 *
 * Enabling this requires the application to define upb_lock()/upb_unlock()
 * functions that acquire/release a global mutex (or #define UPB_THREAD_UNSAFE).
 * For this reason we don't enable it by default, even in debug builds.
 */

/* #define UPB_DEBUG_REFS */

#ifdef __cplusplus
namespace upb { class RefCounted; }
#endif

UPB_DECLARE_TYPE(upb::RefCounted, upb_refcounted)

struct upb_refcounted_vtbl;

#ifdef __cplusplus

class upb::RefCounted {
 public:
  /* Returns true if the given object is frozen. */
  bool IsFrozen() const;

  /* Increases the ref count, the new ref is owned by "owner" which must not
   * already own a ref (and should not itself be a refcounted object if the ref
   * could possibly be circular; see below).
   * Thread-safe iff "this" is frozen. */
  void Ref(const void *owner) const;

  /* Release a ref that was acquired from upb_refcounted_ref() and collects any
   * objects it can. */
  void Unref(const void *owner) const;

  /* Moves an existing ref from "from" to "to", without changing the overall
   * ref count.  DonateRef(foo, NULL, owner) is the same as Ref(foo, owner),
   * but "to" may not be NULL. */
  void DonateRef(const void *from, const void *to) const;

  /* Verifies that a ref to the given object is currently held by the given
   * owner.  Only effective in UPB_DEBUG_REFS builds. */
  void CheckRef(const void *owner) const;

 private:
  UPB_DISALLOW_POD_OPS(RefCounted, upb::RefCounted)
#else
struct upb_refcounted {
#endif
  /* TODO(haberman): move the actual structure definition to structdefs.int.h.
   * The only reason they are here is because inline functions need to see the
   * definition of upb_handlers, which needs to see this definition.  But we
   * can change the upb_handlers inline functions to deal in raw offsets
   * instead.
   */

  /* A single reference count shared by all objects in the group. */
  uint32_t *group;

  /* A singly-linked list of all objects in the group. */
  upb_refcounted *next;

  /* Table of function pointers for this type. */
  const struct upb_refcounted_vtbl *vtbl;

  /* Maintained only when mutable, this tracks the number of refs (but not
   * ref2's) to this object.  *group should be the sum of all individual_count
   * in the group. */
  uint32_t individual_count;

  bool is_frozen;

#ifdef UPB_DEBUG_REFS
  upb_inttable *refs;  /* Maps owner -> trackedref for incoming refs. */
  upb_inttable *ref2s; /* Set of targets for outgoing ref2s. */
#endif
};

#ifdef UPB_DEBUG_REFS
#define UPB_REFCOUNT_INIT(refs, ref2s) \
    {&static_refcount, NULL, NULL, 0, true, refs, ref2s}
#else
#define UPB_REFCOUNT_INIT(refs, ref2s) {&static_refcount, NULL, NULL, 0, true}
#endif

UPB_BEGIN_EXTERN_C

/* It is better to use tracked refs when possible, for the extra debugging
 * capability.  But if this is not possible (because you don't have easy access
 * to a stable pointer value that is associated with the ref), you can pass
 * UPB_UNTRACKED_REF instead.  */
extern const void *UPB_UNTRACKED_REF;

/* Native C API. */
bool upb_refcounted_isfrozen(const upb_refcounted *r);
void upb_refcounted_ref(const upb_refcounted *r, const void *owner);
void upb_refcounted_unref(const upb_refcounted *r, const void *owner);
void upb_refcounted_donateref(
    const upb_refcounted *r, const void *from, const void *to);
void upb_refcounted_checkref(const upb_refcounted *r, const void *owner);

#define UPB_REFCOUNTED_CMETHODS(type, upcastfunc) \
  UPB_INLINE bool type ## _isfrozen(const type *v) { \
    return upb_refcounted_isfrozen(upcastfunc(v)); \
  } \
  UPB_INLINE void type ## _ref(const type *v, const void *owner) { \
    upb_refcounted_ref(upcastfunc(v), owner); \
  } \
  UPB_INLINE void type ## _unref(const type *v, const void *owner) { \
    upb_refcounted_unref(upcastfunc(v), owner); \
  } \
  UPB_INLINE void type ## _donateref(const type *v, const void *from, const void *to) { \
    upb_refcounted_donateref(upcastfunc(v), from, to); \
  } \
  UPB_INLINE void type ## _checkref(const type *v, const void *owner) { \
    upb_refcounted_checkref(upcastfunc(v), owner); \
  }

#define UPB_REFCOUNTED_CPPMETHODS \
  bool IsFrozen() const { \
    return upb::upcast_to<const upb::RefCounted>(this)->IsFrozen(); \
  } \
  void Ref(const void *owner) const { \
    return upb::upcast_to<const upb::RefCounted>(this)->Ref(owner); \
  } \
  void Unref(const void *owner) const { \
    return upb::upcast_to<const upb::RefCounted>(this)->Unref(owner); \
  } \
  void DonateRef(const void *from, const void *to) const { \
    return upb::upcast_to<const upb::RefCounted>(this)->DonateRef(from, to); \
  } \
  void CheckRef(const void *owner) const { \
    return upb::upcast_to<const upb::RefCounted>(this)->CheckRef(owner); \
  }

/* Internal-to-upb Interface **************************************************/

typedef void upb_refcounted_visit(const upb_refcounted *r,
                                  const upb_refcounted *subobj,
                                  void *closure);

struct upb_refcounted_vtbl {
  /* Must visit all subobjects that are currently ref'd via upb_refcounted_ref2.
   * Must be longjmp()-safe. */
  void (*visit)(const upb_refcounted *r, upb_refcounted_visit *visit, void *c);

  /* Must free the object and release all references to other objects. */
  void (*free)(upb_refcounted *r);
};

/* Initializes the refcounted with a single ref for the given owner.  Returns
 * false if memory could not be allocated. */
bool upb_refcounted_init(upb_refcounted *r,
                         const struct upb_refcounted_vtbl *vtbl,
                         const void *owner);

/* Adds a ref from one refcounted object to another ("from" must not already
 * own a ref).  These refs may be circular; cycles will be collected correctly
 * (if conservatively).  These refs do not need to be freed in from's free()
 * function. */
void upb_refcounted_ref2(const upb_refcounted *r, upb_refcounted *from);

/* Removes a ref that was acquired from upb_refcounted_ref2(), and collects any
 * object it can.  This is only necessary when "from" no longer points to "r",
 * and not from from's "free" function. */
void upb_refcounted_unref2(const upb_refcounted *r, upb_refcounted *from);

#define upb_ref2(r, from) \
    upb_refcounted_ref2((const upb_refcounted*)r, (upb_refcounted*)from)
#define upb_unref2(r, from) \
    upb_refcounted_unref2((const upb_refcounted*)r, (upb_refcounted*)from)

/* Freezes all mutable object reachable by ref2() refs from the given roots.
 * This will split refcounting groups into precise SCC groups, so that
 * refcounting of frozen objects can be more aggressive.  If memory allocation
 * fails, or if more than 2**31 mutable objects are reachable from "roots", or
 * if the maximum depth of the graph exceeds "maxdepth", false is returned and
 * the objects are unchanged.
 *
 * After this operation succeeds, the objects are frozen/const, and may not be
 * used through non-const pointers.  In particular, they may not be passed as
 * the second parameter of upb_refcounted_{ref,unref}2().  On the upside, all
 * operations on frozen refcounteds are threadsafe, and objects will be freed
 * at the precise moment that they become unreachable.
 *
 * Caller must own refs on each object in the "roots" list. */
bool upb_refcounted_freeze(upb_refcounted *const*roots, int n, upb_status *s,
                           int maxdepth);

/* Shared by all compiled-in refcounted objects. */
extern uint32_t static_refcount;

UPB_END_EXTERN_C

#ifdef __cplusplus
/* C++ Wrappers. */
namespace upb {
inline bool RefCounted::IsFrozen() const {
  return upb_refcounted_isfrozen(this);
}
inline void RefCounted::Ref(const void *owner) const {
  upb_refcounted_ref(this, owner);
}
inline void RefCounted::Unref(const void *owner) const {
  upb_refcounted_unref(this, owner);
}
inline void RefCounted::DonateRef(const void *from, const void *to) const {
  upb_refcounted_donateref(this, from, to);
}
inline void RefCounted::CheckRef(const void *owner) const {
  upb_refcounted_checkref(this, owner);
}
}  /* namespace upb */
#endif

#endif  /* UPB_REFCOUNT_H_ */

#ifdef __cplusplus
#include <cstring>
#include <string>
#include <vector>

namespace upb {
class Def;
class EnumDef;
class FieldDef;
class MessageDef;
class OneofDef;
}
#endif

UPB_DECLARE_DERIVED_TYPE(upb::Def, upb::RefCounted, upb_def, upb_refcounted)

/* The maximum message depth that the type graph can have.  This is a resource
 * limit for the C stack since we sometimes need to recursively traverse the
 * graph.  Cycles are ok; the traversal will stop when it detects a cycle, but
 * we must hit the cycle before the maximum depth is reached.
 *
 * If having a single static limit is too inflexible, we can add another variant
 * of Def::Freeze that allows specifying this as a parameter. */
#define UPB_MAX_MESSAGE_DEPTH 64


/* upb::Def: base class for defs  *********************************************/

/* All the different kind of defs we support.  These correspond 1:1 with
 * declarations in a .proto file. */
typedef enum {
  UPB_DEF_MSG,
  UPB_DEF_FIELD,
  UPB_DEF_ENUM,
  UPB_DEF_ONEOF,
  UPB_DEF_SERVICE,   /* Not yet implemented. */
  UPB_DEF_ANY = -1   /* Wildcard for upb_symtab_get*() */
} upb_deftype_t;

#ifdef __cplusplus

/* The base class of all defs.  Its base is upb::RefCounted (use upb::upcast()
 * to convert). */
class upb::Def {
 public:
  typedef upb_deftype_t Type;

  Def* Dup(const void *owner) const;

  /* upb::RefCounted methods like Ref()/Unref(). */
  UPB_REFCOUNTED_CPPMETHODS

  Type def_type() const;

  /* "fullname" is the def's fully-qualified name (eg. foo.bar.Message). */
  const char *full_name() const;

  /* The def must be mutable.  Caller retains ownership of fullname.  Defs are
   * not required to have a name; if a def has no name when it is frozen, it
   * will remain an anonymous def.  On failure, returns false and details in "s"
   * if non-NULL. */
  bool set_full_name(const char* fullname, upb::Status* s);
  bool set_full_name(const std::string &fullname, upb::Status* s);

  /* Freezes the given defs; this validates all constraints and marks the defs
   * as frozen (read-only).  "defs" may not contain any fielddefs, but fields
   * of any msgdefs will be frozen.
   *
   * Symbolic references to sub-types and enum defaults must have already been
   * resolved.  Any mutable defs reachable from any of "defs" must also be in
   * the list; more formally, "defs" must be a transitive closure of mutable
   * defs.
   *
   * After this operation succeeds, the finalized defs must only be accessed
   * through a const pointer! */
  static bool Freeze(Def* const* defs, int n, Status* status);
  static bool Freeze(const std::vector<Def*>& defs, Status* status);

 private:
  UPB_DISALLOW_POD_OPS(Def, upb::Def)
};

#endif  /* __cplusplus */

UPB_BEGIN_EXTERN_C

/* Native C API. */
upb_def *upb_def_dup(const upb_def *def, const void *owner);

/* Include upb_refcounted methods like upb_def_ref()/upb_def_unref(). */
UPB_REFCOUNTED_CMETHODS(upb_def, upb_def_upcast)

upb_deftype_t upb_def_type(const upb_def *d);
const char *upb_def_fullname(const upb_def *d);
bool upb_def_setfullname(upb_def *def, const char *fullname, upb_status *s);
bool upb_def_freeze(upb_def *const *defs, int n, upb_status *s);

UPB_END_EXTERN_C


/* upb::Def casts *************************************************************/

#ifdef __cplusplus
#define UPB_CPP_CASTS(cname, cpptype)                                          \
  namespace upb {                                                              \
  template <>                                                                  \
  inline cpptype *down_cast<cpptype *, Def>(Def * def) {                       \
    return upb_downcast_##cname##_mutable(def);                                \
  }                                                                            \
  template <>                                                                  \
  inline cpptype *dyn_cast<cpptype *, Def>(Def * def) {                        \
    return upb_dyncast_##cname##_mutable(def);                                 \
  }                                                                            \
  template <>                                                                  \
  inline const cpptype *down_cast<const cpptype *, const Def>(                 \
      const Def *def) {                                                        \
    return upb_downcast_##cname(def);                                          \
  }                                                                            \
  template <>                                                                  \
  inline const cpptype *dyn_cast<const cpptype *, const Def>(const Def *def) { \
    return upb_dyncast_##cname(def);                                           \
  }                                                                            \
  template <>                                                                  \
  inline const cpptype *down_cast<const cpptype *, Def>(Def * def) {           \
    return upb_downcast_##cname(def);                                          \
  }                                                                            \
  template <>                                                                  \
  inline const cpptype *dyn_cast<const cpptype *, Def>(Def * def) {            \
    return upb_dyncast_##cname(def);                                           \
  }                                                                            \
  }  /* namespace upb */
#else
#define UPB_CPP_CASTS(cname, cpptype)
#endif  /* __cplusplus */

/* Dynamic casts, for determining if a def is of a particular type at runtime.
 * Downcasts, for when some wants to assert that a def is of a particular type.
 * These are only checked if we are building debug. */
#define UPB_DEF_CASTS(lower, upper, cpptype)                               \
  UPB_INLINE const upb_##lower *upb_dyncast_##lower(const upb_def *def) {  \
    if (upb_def_type(def) != UPB_DEF_##upper) return NULL;                 \
    return (upb_##lower *)def;                                             \
  }                                                                        \
  UPB_INLINE const upb_##lower *upb_downcast_##lower(const upb_def *def) { \
    assert(upb_def_type(def) == UPB_DEF_##upper);                          \
    return (const upb_##lower *)def;                                       \
  }                                                                        \
  UPB_INLINE upb_##lower *upb_dyncast_##lower##_mutable(upb_def *def) {    \
    return (upb_##lower *)upb_dyncast_##lower(def);                        \
  }                                                                        \
  UPB_INLINE upb_##lower *upb_downcast_##lower##_mutable(upb_def *def) {   \
    return (upb_##lower *)upb_downcast_##lower(def);                       \
  }                                                                        \
  UPB_CPP_CASTS(lower, cpptype)

#define UPB_DEFINE_DEF(cppname, lower, upper, cppmethods, members)             \
  UPB_DEFINE_CLASS2(cppname, upb::Def, upb::RefCounted, cppmethods,            \
                   members)                                                    \
  UPB_DEF_CASTS(lower, upper, cppname)

#define UPB_DECLARE_DEF_TYPE(cppname, lower, upper) \
  UPB_DECLARE_DERIVED_TYPE2(cppname, upb::Def, upb::RefCounted, \
                            upb_ ## lower, upb_def, upb_refcounted) \
  UPB_DEF_CASTS(lower, upper, cppname)

UPB_DECLARE_DEF_TYPE(upb::FieldDef, fielddef, FIELD)
UPB_DECLARE_DEF_TYPE(upb::MessageDef, msgdef, MSG)
UPB_DECLARE_DEF_TYPE(upb::EnumDef, enumdef, ENUM)
UPB_DECLARE_DEF_TYPE(upb::OneofDef, oneofdef, ONEOF)

#undef UPB_DECLARE_DEF_TYPE
#undef UPB_DEF_CASTS
#undef UPB_CPP_CASTS


/* upb::FieldDef **************************************************************/

/* The types a field can have.  Note that this list is not identical to the
 * types defined in descriptor.proto, which gives INT32 and SINT32 separate
 * types (we distinguish the two with the "integer encoding" enum below). */
typedef enum {
  UPB_TYPE_FLOAT    = 1,
  UPB_TYPE_DOUBLE   = 2,
  UPB_TYPE_BOOL     = 3,
  UPB_TYPE_STRING   = 4,
  UPB_TYPE_BYTES    = 5,
  UPB_TYPE_MESSAGE  = 6,
  UPB_TYPE_ENUM     = 7,  /* Enum values are int32. */
  UPB_TYPE_INT32    = 8,
  UPB_TYPE_UINT32   = 9,
  UPB_TYPE_INT64    = 10,
  UPB_TYPE_UINT64   = 11
} upb_fieldtype_t;

/* The repeated-ness of each field; this matches descriptor.proto. */
typedef enum {
  UPB_LABEL_OPTIONAL = 1,
  UPB_LABEL_REQUIRED = 2,
  UPB_LABEL_REPEATED = 3
} upb_label_t;

/* How integers should be encoded in serializations that offer multiple
 * integer encoding methods. */
typedef enum {
  UPB_INTFMT_VARIABLE = 1,
  UPB_INTFMT_FIXED = 2,
  UPB_INTFMT_ZIGZAG = 3   /* Only for signed types (INT32/INT64). */
} upb_intfmt_t;

/* Descriptor types, as defined in descriptor.proto. */
typedef enum {
  UPB_DESCRIPTOR_TYPE_DOUBLE   = 1,
  UPB_DESCRIPTOR_TYPE_FLOAT    = 2,
  UPB_DESCRIPTOR_TYPE_INT64    = 3,
  UPB_DESCRIPTOR_TYPE_UINT64   = 4,
  UPB_DESCRIPTOR_TYPE_INT32    = 5,
  UPB_DESCRIPTOR_TYPE_FIXED64  = 6,
  UPB_DESCRIPTOR_TYPE_FIXED32  = 7,
  UPB_DESCRIPTOR_TYPE_BOOL     = 8,
  UPB_DESCRIPTOR_TYPE_STRING   = 9,
  UPB_DESCRIPTOR_TYPE_GROUP    = 10,
  UPB_DESCRIPTOR_TYPE_MESSAGE  = 11,
  UPB_DESCRIPTOR_TYPE_BYTES    = 12,
  UPB_DESCRIPTOR_TYPE_UINT32   = 13,
  UPB_DESCRIPTOR_TYPE_ENUM     = 14,
  UPB_DESCRIPTOR_TYPE_SFIXED32 = 15,
  UPB_DESCRIPTOR_TYPE_SFIXED64 = 16,
  UPB_DESCRIPTOR_TYPE_SINT32   = 17,
  UPB_DESCRIPTOR_TYPE_SINT64   = 18
} upb_descriptortype_t;

/* Maximum field number allowed for FieldDefs.  This is an inherent limit of the
 * protobuf wire format. */
#define UPB_MAX_FIELDNUMBER ((1 << 29) - 1)

#ifdef __cplusplus

/* A upb_fielddef describes a single field in a message.  It is most often
 * found as a part of a upb_msgdef, but can also stand alone to represent
 * an extension.
 *
 * Its base class is upb::Def (use upb::upcast() to convert). */
class upb::FieldDef {
 public:
  typedef upb_fieldtype_t Type;
  typedef upb_label_t Label;
  typedef upb_intfmt_t IntegerFormat;
  typedef upb_descriptortype_t DescriptorType;

  /* These return true if the given value is a valid member of the enumeration. */
  static bool CheckType(int32_t val);
  static bool CheckLabel(int32_t val);
  static bool CheckDescriptorType(int32_t val);
  static bool CheckIntegerFormat(int32_t val);

  /* These convert to the given enumeration; they require that the value is
   * valid. */
  static Type ConvertType(int32_t val);
  static Label ConvertLabel(int32_t val);
  static DescriptorType ConvertDescriptorType(int32_t val);
  static IntegerFormat ConvertIntegerFormat(int32_t val);

  /* Returns NULL if memory allocation failed. */
  static reffed_ptr<FieldDef> New();

  /* Duplicates the given field, returning NULL if memory allocation failed.
   * When a fielddef is duplicated, the subdef (if any) is made symbolic if it
   * wasn't already.  If the subdef is set but has no name (which is possible
   * since msgdefs are not required to have a name) the new fielddef's subdef
   * will be unset. */
  FieldDef* Dup(const void* owner) const;

  /* upb::RefCounted methods like Ref()/Unref(). */
  UPB_REFCOUNTED_CPPMETHODS

  /* Functionality from upb::Def. */
  const char* full_name() const;

  bool type_is_set() const;  /* set_[descriptor_]type() has been called? */
  Type type() const;         /* Requires that type_is_set() == true. */
  Label label() const;       /* Defaults to UPB_LABEL_OPTIONAL. */
  const char* name() const;  /* NULL if uninitialized. */
  uint32_t number() const;   /* Returns 0 if uninitialized. */
  bool is_extension() const;

  /* For UPB_TYPE_MESSAGE fields only where is_tag_delimited() == false,
   * indicates whether this field should have lazy parsing handlers that yield
   * the unparsed string for the submessage.
   *
   * TODO(haberman): I think we want to move this into a FieldOptions container
   * when we add support for custom options (the FieldOptions struct will
   * contain both regular FieldOptions like "lazy" *and* custom options). */
  bool lazy() const;

  /* For non-string, non-submessage fields, this indicates whether binary
   * protobufs are encoded in packed or non-packed format.
   *
   * TODO(haberman): see note above about putting options like this into a
   * FieldOptions container. */
  bool packed() const;

  /* An integer that can be used as an index into an array of fields for
   * whatever message this field belongs to.  Guaranteed to be less than
   * f->containing_type()->field_count().  May only be accessed once the def has
   * been finalized. */
  int index() const;

  /* The MessageDef to which this field belongs.
   *
   * If this field has been added to a MessageDef, that message can be retrieved
   * directly (this is always the case for frozen FieldDefs).
   *
   * If the field has not yet been added to a MessageDef, you can set the name
   * of the containing type symbolically instead.  This is mostly useful for
   * extensions, where the extension is declared separately from the message. */
  const MessageDef* containing_type() const;
  const char* containing_type_name();

  /* The OneofDef to which this field belongs, or NULL if this field is not part
   * of a oneof. */
  const OneofDef* containing_oneof() const;

  /* The field's type according to the enum in descriptor.proto.  This is not
   * the same as UPB_TYPE_*, because it distinguishes between (for example)
   * INT32 and SINT32, whereas our "type" enum does not.  This return of
   * descriptor_type() is a function of type(), integer_format(), and
   * is_tag_delimited().  Likewise set_descriptor_type() sets all three
   * appropriately. */
  DescriptorType descriptor_type() const;

  /* Convenient field type tests. */
  bool IsSubMessage() const;
  bool IsString() const;
  bool IsSequence() const;
  bool IsPrimitive() const;
  bool IsMap() const;

  /* How integers are encoded.  Only meaningful for integer types.
   * Defaults to UPB_INTFMT_VARIABLE, and is reset when "type" changes. */
  IntegerFormat integer_format() const;

  /* Whether a submessage field is tag-delimited or not (if false, then
   * length-delimited).  May only be set when type() == UPB_TYPE_MESSAGE. */
  bool is_tag_delimited() const;

  /* Returns the non-string default value for this fielddef, which may either
   * be something the client set explicitly or the "default default" (0 for
   * numbers, empty for strings).  The field's type indicates the type of the
   * returned value, except for enum fields that are still mutable.
   *
   * Requires that the given function matches the field's current type. */
  int64_t default_int64() const;
  int32_t default_int32() const;
  uint64_t default_uint64() const;
  uint32_t default_uint32() const;
  bool default_bool() const;
  float default_float() const;
  double default_double() const;

  /* The resulting string is always NULL-terminated.  If non-NULL, the length
   * will be stored in *len. */
  const char *default_string(size_t* len) const;

  /* For frozen UPB_TYPE_ENUM fields, enum defaults can always be read as either
   * string or int32, and both of these methods will always return true.
   *
   * For mutable UPB_TYPE_ENUM fields, the story is a bit more complicated.
   * Enum defaults are unusual. They can be specified either as string or int32,
   * but to be valid the enum must have that value as a member.  And if no
   * default is specified, the "default default" comes from the EnumDef.
   *
   * We allow reading the default as either an int32 or a string, but only if
   * we have a meaningful value to report.  We have a meaningful value if it was
   * set explicitly, or if we could get the "default default" from the EnumDef.
   * Also if you explicitly set the name and we find the number in the EnumDef */
  bool EnumHasStringDefault() const;
  bool EnumHasInt32Default() const;

  /* Submessage and enum fields must reference a "subdef", which is the
   * upb::MessageDef or upb::EnumDef that defines their type.  Note that when
   * the FieldDef is mutable it may not have a subdef *yet*, but this function
   * still returns true to indicate that the field's type requires a subdef. */
  bool HasSubDef() const;

  /* Returns the enum or submessage def for this field, if any.  The field's
   * type must match (ie. you may only call enum_subdef() for fields where
   * type() == UPB_TYPE_ENUM).  Returns NULL if the subdef has not been set or
   * is currently set symbolically. */
  const EnumDef* enum_subdef() const;
  const MessageDef* message_subdef() const;

  /* Returns the generic subdef for this field.  Requires that HasSubDef() (ie.
   * only works for UPB_TYPE_ENUM and UPB_TYPE_MESSAGE fields). */
  const Def* subdef() const;

  /* Returns the symbolic name of the subdef.  If the subdef is currently set
   * unresolved (ie. set symbolically) returns the symbolic name.  If it has
   * been resolved to a specific subdef, returns the name from that subdef. */
  const char* subdef_name() const;

  /* Setters (non-const methods), only valid for mutable FieldDefs! ***********/

  bool set_full_name(const char* fullname, upb::Status* s);
  bool set_full_name(const std::string& fullname, upb::Status* s);

  /* This may only be called if containing_type() == NULL (ie. the field has not
   * been added to a message yet). */
  bool set_containing_type_name(const char *name, Status* status);
  bool set_containing_type_name(const std::string& name, Status* status);

  /* Defaults to false.  When we freeze, we ensure that this can only be true
   * for length-delimited message fields.  Prior to freezing this can be true or
   * false with no restrictions. */
  void set_lazy(bool lazy);

  /* Defaults to true.  Sets whether this field is encoded in packed format. */
  void set_packed(bool packed);

  /* "type" or "descriptor_type" MUST be set explicitly before the fielddef is
   * finalized.  These setters require that the enum value is valid; if the
   * value did not come directly from an enum constant, the caller should
   * validate it first with the functions above (CheckFieldType(), etc). */
  void set_type(Type type);
  void set_label(Label label);
  void set_descriptor_type(DescriptorType type);
  void set_is_extension(bool is_extension);

  /* "number" and "name" must be set before the FieldDef is added to a
   * MessageDef, and may not be set after that.
   *
   * "name" is the same as full_name()/set_full_name(), but since fielddefs
   * most often use simple, non-qualified names, we provide this accessor
   * also.  Generally only extensions will want to think of this name as
   * fully-qualified. */
  bool set_number(uint32_t number, upb::Status* s);
  bool set_name(const char* name, upb::Status* s);
  bool set_name(const std::string& name, upb::Status* s);

  void set_integer_format(IntegerFormat format);
  bool set_tag_delimited(bool tag_delimited, upb::Status* s);

  /* Sets default value for the field.  The call must exactly match the type
   * of the field.  Enum fields may use either setint32 or setstring to set
   * the default numerically or symbolically, respectively, but symbolic
   * defaults must be resolved before finalizing (see ResolveEnumDefault()).
   *
   * Changing the type of a field will reset its default. */
  void set_default_int64(int64_t val);
  void set_default_int32(int32_t val);
  void set_default_uint64(uint64_t val);
  void set_default_uint32(uint32_t val);
  void set_default_bool(bool val);
  void set_default_float(float val);
  void set_default_double(double val);
  bool set_default_string(const void *str, size_t len, Status *s);
  bool set_default_string(const std::string &str, Status *s);
  void set_default_cstr(const char *str, Status *s);

  /* Before a fielddef is frozen, its subdef may be set either directly (with a
   * upb::Def*) or symbolically.  Symbolic refs must be resolved before the
   * containing msgdef can be frozen (see upb_resolve() above).  upb always
   * guarantees that any def reachable from a live def will also be kept alive.
   *
   * Both methods require that upb_hassubdef(f) (so the type must be set prior
   * to calling these methods).  Returns false if this is not the case, or if
   * the given subdef is not of the correct type.  The subdef is reset if the
   * field's type is changed.  The subdef can be set to NULL to clear it. */
  bool set_subdef(const Def* subdef, Status* s);
  bool set_enum_subdef(const EnumDef* subdef, Status* s);
  bool set_message_subdef(const MessageDef* subdef, Status* s);
  bool set_subdef_name(const char* name, Status* s);
  bool set_subdef_name(const std::string &name, Status* s);

 private:
  UPB_DISALLOW_POD_OPS(FieldDef, upb::FieldDef)
};

# endif  /* defined(__cplusplus) */

UPB_BEGIN_EXTERN_C

/* Native C API. */
upb_fielddef *upb_fielddef_new(const void *owner);
upb_fielddef *upb_fielddef_dup(const upb_fielddef *f, const void *owner);

/* Include upb_refcounted methods like upb_fielddef_ref(). */
UPB_REFCOUNTED_CMETHODS(upb_fielddef, upb_fielddef_upcast2)

/* Methods from upb_def. */
const char *upb_fielddef_fullname(const upb_fielddef *f);
bool upb_fielddef_setfullname(upb_fielddef *f, const char *fullname,
                              upb_status *s);

bool upb_fielddef_typeisset(const upb_fielddef *f);
upb_fieldtype_t upb_fielddef_type(const upb_fielddef *f);
upb_descriptortype_t upb_fielddef_descriptortype(const upb_fielddef *f);
upb_label_t upb_fielddef_label(const upb_fielddef *f);
uint32_t upb_fielddef_number(const upb_fielddef *f);
const char *upb_fielddef_name(const upb_fielddef *f);
bool upb_fielddef_isextension(const upb_fielddef *f);
bool upb_fielddef_lazy(const upb_fielddef *f);
bool upb_fielddef_packed(const upb_fielddef *f);
const upb_msgdef *upb_fielddef_containingtype(const upb_fielddef *f);
const upb_oneofdef *upb_fielddef_containingoneof(const upb_fielddef *f);
upb_msgdef *upb_fielddef_containingtype_mutable(upb_fielddef *f);
const char *upb_fielddef_containingtypename(upb_fielddef *f);
upb_intfmt_t upb_fielddef_intfmt(const upb_fielddef *f);
uint32_t upb_fielddef_index(const upb_fielddef *f);
bool upb_fielddef_istagdelim(const upb_fielddef *f);
bool upb_fielddef_issubmsg(const upb_fielddef *f);
bool upb_fielddef_isstring(const upb_fielddef *f);
bool upb_fielddef_isseq(const upb_fielddef *f);
bool upb_fielddef_isprimitive(const upb_fielddef *f);
bool upb_fielddef_ismap(const upb_fielddef *f);
int64_t upb_fielddef_defaultint64(const upb_fielddef *f);
int32_t upb_fielddef_defaultint32(const upb_fielddef *f);
uint64_t upb_fielddef_defaultuint64(const upb_fielddef *f);
uint32_t upb_fielddef_defaultuint32(const upb_fielddef *f);
bool upb_fielddef_defaultbool(const upb_fielddef *f);
float upb_fielddef_defaultfloat(const upb_fielddef *f);
double upb_fielddef_defaultdouble(const upb_fielddef *f);
const char *upb_fielddef_defaultstr(const upb_fielddef *f, size_t *len);
bool upb_fielddef_enumhasdefaultint32(const upb_fielddef *f);
bool upb_fielddef_enumhasdefaultstr(const upb_fielddef *f);
bool upb_fielddef_hassubdef(const upb_fielddef *f);
const upb_def *upb_fielddef_subdef(const upb_fielddef *f);
const upb_msgdef *upb_fielddef_msgsubdef(const upb_fielddef *f);
const upb_enumdef *upb_fielddef_enumsubdef(const upb_fielddef *f);
const char *upb_fielddef_subdefname(const upb_fielddef *f);

void upb_fielddef_settype(upb_fielddef *f, upb_fieldtype_t type);
void upb_fielddef_setdescriptortype(upb_fielddef *f, int type);
void upb_fielddef_setlabel(upb_fielddef *f, upb_label_t label);
bool upb_fielddef_setnumber(upb_fielddef *f, uint32_t number, upb_status *s);
bool upb_fielddef_setname(upb_fielddef *f, const char *name, upb_status *s);
bool upb_fielddef_setcontainingtypename(upb_fielddef *f, const char *name,
                                        upb_status *s);
void upb_fielddef_setisextension(upb_fielddef *f, bool is_extension);
void upb_fielddef_setlazy(upb_fielddef *f, bool lazy);
void upb_fielddef_setpacked(upb_fielddef *f, bool packed);
void upb_fielddef_setintfmt(upb_fielddef *f, upb_intfmt_t fmt);
void upb_fielddef_settagdelim(upb_fielddef *f, bool tag_delim);
void upb_fielddef_setdefaultint64(upb_fielddef *f, int64_t val);
void upb_fielddef_setdefaultint32(upb_fielddef *f, int32_t val);
void upb_fielddef_setdefaultuint64(upb_fielddef *f, uint64_t val);
void upb_fielddef_setdefaultuint32(upb_fielddef *f, uint32_t val);
void upb_fielddef_setdefaultbool(upb_fielddef *f, bool val);
void upb_fielddef_setdefaultfloat(upb_fielddef *f, float val);
void upb_fielddef_setdefaultdouble(upb_fielddef *f, double val);
bool upb_fielddef_setdefaultstr(upb_fielddef *f, const void *str, size_t len,
                                upb_status *s);
void upb_fielddef_setdefaultcstr(upb_fielddef *f, const char *str,
                                 upb_status *s);
bool upb_fielddef_setsubdef(upb_fielddef *f, const upb_def *subdef,
                            upb_status *s);
bool upb_fielddef_setmsgsubdef(upb_fielddef *f, const upb_msgdef *subdef,
                               upb_status *s);
bool upb_fielddef_setenumsubdef(upb_fielddef *f, const upb_enumdef *subdef,
                                upb_status *s);
bool upb_fielddef_setsubdefname(upb_fielddef *f, const char *name,
                                upb_status *s);

bool upb_fielddef_checklabel(int32_t label);
bool upb_fielddef_checktype(int32_t type);
bool upb_fielddef_checkdescriptortype(int32_t type);
bool upb_fielddef_checkintfmt(int32_t fmt);

UPB_END_EXTERN_C


/* upb::MessageDef ************************************************************/

typedef upb_inttable_iter upb_msg_field_iter;
typedef upb_strtable_iter upb_msg_oneof_iter;

#ifdef __cplusplus

/* Structure that describes a single .proto message type.
 *
 * Its base class is upb::Def (use upb::upcast() to convert). */
class upb::MessageDef {
 public:
  /* Returns NULL if memory allocation failed. */
  static reffed_ptr<MessageDef> New();

  /* upb::RefCounted methods like Ref()/Unref(). */
  UPB_REFCOUNTED_CPPMETHODS

  /* Functionality from upb::Def. */
  const char* full_name() const;
  bool set_full_name(const char* fullname, Status* s);
  bool set_full_name(const std::string& fullname, Status* s);

  /* Call to freeze this MessageDef.
   * WARNING: this will fail if this message has any unfrozen submessages!
   * Messages with cycles must be frozen as a batch using upb::Def::Freeze(). */
  bool Freeze(Status* s);

  /* The number of fields that belong to the MessageDef. */
  int field_count() const;

  /* The number of oneofs that belong to the MessageDef. */
  int oneof_count() const;

  /* Adds a field (upb_fielddef object) to a msgdef.  Requires that the msgdef
   * and the fielddefs are mutable.  The fielddef's name and number must be
   * set, and the message may not already contain any field with this name or
   * number, and this fielddef may not be part of another message.  In error
   * cases false is returned and the msgdef is unchanged.
   *
   * If the given field is part of a oneof, this call succeeds if and only if
   * that oneof is already part of this msgdef. (Note that adding a oneof to a
   * msgdef automatically adds all of its fields to the msgdef at the time that
   * the oneof is added, so it is usually more idiomatic to add the oneof's
   * fields first then add the oneof to the msgdef. This case is supported for
   * convenience.)
   *
   * If |f| is already part of this MessageDef, this method performs no action
   * and returns true (success). Thus, this method is idempotent. */
  bool AddField(FieldDef* f, Status* s);
  bool AddField(const reffed_ptr<FieldDef>& f, Status* s);

  /* Adds a oneof (upb_oneofdef object) to a msgdef. Requires that the msgdef,
   * oneof, and any fielddefs are mutable, that the fielddefs contained in the
   * oneof do not have any name or number conflicts with existing fields in the
   * msgdef, and that the oneof's name is unique among all oneofs in the msgdef.
   * If the oneof is added successfully, all of its fields will be added
   * directly to the msgdef as well. In error cases, false is returned and the
   * msgdef is unchanged. */
  bool AddOneof(OneofDef* o, Status* s);
  bool AddOneof(const reffed_ptr<OneofDef>& o, Status* s);

  /* These return NULL if the field is not found. */
  FieldDef* FindFieldByNumber(uint32_t number);
  FieldDef* FindFieldByName(const char *name, size_t len);
  const FieldDef* FindFieldByNumber(uint32_t number) const;
  const FieldDef* FindFieldByName(const char* name, size_t len) const;


  FieldDef* FindFieldByName(const char *name) {
    return FindFieldByName(name, strlen(name));
  }
  const FieldDef* FindFieldByName(const char *name) const {
    return FindFieldByName(name, strlen(name));
  }

  template <class T>
  FieldDef* FindFieldByName(const T& str) {
    return FindFieldByName(str.c_str(), str.size());
  }
  template <class T>
  const FieldDef* FindFieldByName(const T& str) const {
    return FindFieldByName(str.c_str(), str.size());
  }

  OneofDef* FindOneofByName(const char* name, size_t len);
  const OneofDef* FindOneofByName(const char* name, size_t len) const;

  OneofDef* FindOneofByName(const char* name) {
    return FindOneofByName(name, strlen(name));
  }
  const OneofDef* FindOneofByName(const char* name) const {
    return FindOneofByName(name, strlen(name));
  }

  template<class T>
  OneofDef* FindOneofByName(const T& str) {
    return FindOneofByName(str.c_str(), str.size());
  }
  template<class T>
  const OneofDef* FindOneofByName(const T& str) const {
    return FindOneofByName(str.c_str(), str.size());
  }

  /* Returns a new msgdef that is a copy of the given msgdef (and a copy of all
   * the fields) but with any references to submessages broken and replaced
   * with just the name of the submessage.  Returns NULL if memory allocation
   * failed.
   *
   * TODO(haberman): which is more useful, keeping fields resolved or
   * unresolving them?  If there's no obvious answer, Should this functionality
   * just be moved into symtab.c? */
  MessageDef* Dup(const void* owner) const;

  /* Is this message a map entry? */
  void setmapentry(bool map_entry);
  bool mapentry() const;

  /* Iteration over fields.  The order is undefined. */
  class field_iterator
      : public std::iterator<std::forward_iterator_tag, FieldDef*> {
   public:
    explicit field_iterator(MessageDef* md);
    static field_iterator end(MessageDef* md);

    void operator++();
    FieldDef* operator*() const;
    bool operator!=(const field_iterator& other) const;
    bool operator==(const field_iterator& other) const;

   private:
    upb_msg_field_iter iter_;
  };

  class const_field_iterator
      : public std::iterator<std::forward_iterator_tag, const FieldDef*> {
   public:
    explicit const_field_iterator(const MessageDef* md);
    static const_field_iterator end(const MessageDef* md);

    void operator++();
    const FieldDef* operator*() const;
    bool operator!=(const const_field_iterator& other) const;
    bool operator==(const const_field_iterator& other) const;

   private:
    upb_msg_field_iter iter_;
  };

  /* Iteration over oneofs. The order is undefined. */
  class oneof_iterator
      : public std::iterator<std::forward_iterator_tag, FieldDef*> {
   public:
    explicit oneof_iterator(MessageDef* md);
    static oneof_iterator end(MessageDef* md);

    void operator++();
    OneofDef* operator*() const;
    bool operator!=(const oneof_iterator& other) const;
    bool operator==(const oneof_iterator& other) const;

   private:
    upb_msg_oneof_iter iter_;
  };

  class const_oneof_iterator
      : public std::iterator<std::forward_iterator_tag, const FieldDef*> {
   public:
    explicit const_oneof_iterator(const MessageDef* md);
    static const_oneof_iterator end(const MessageDef* md);

    void operator++();
    const OneofDef* operator*() const;
    bool operator!=(const const_oneof_iterator& other) const;
    bool operator==(const const_oneof_iterator& other) const;

   private:
    upb_msg_oneof_iter iter_;
  };

  class FieldAccessor {
   public:
    explicit FieldAccessor(MessageDef* msg) : msg_(msg) {}
    field_iterator begin() { return msg_->field_begin(); }
    field_iterator end() { return msg_->field_end(); }
   private:
    MessageDef* msg_;
  };

  class ConstFieldAccessor {
   public:
    explicit ConstFieldAccessor(const MessageDef* msg) : msg_(msg) {}
    const_field_iterator begin() { return msg_->field_begin(); }
    const_field_iterator end() { return msg_->field_end(); }
   private:
    const MessageDef* msg_;
  };

  class OneofAccessor {
   public:
    explicit OneofAccessor(MessageDef* msg) : msg_(msg) {}
    oneof_iterator begin() { return msg_->oneof_begin(); }
    oneof_iterator end() { return msg_->oneof_end(); }
   private:
    MessageDef* msg_;
  };

  class ConstOneofAccessor {
   public:
    explicit ConstOneofAccessor(const MessageDef* msg) : msg_(msg) {}
    const_oneof_iterator begin() { return msg_->oneof_begin(); }
    const_oneof_iterator end() { return msg_->oneof_end(); }
   private:
    const MessageDef* msg_;
  };

  field_iterator field_begin();
  field_iterator field_end();
  const_field_iterator field_begin() const;
  const_field_iterator field_end() const;

  oneof_iterator oneof_begin();
  oneof_iterator oneof_end();
  const_oneof_iterator oneof_begin() const;
  const_oneof_iterator oneof_end() const;

  FieldAccessor fields() { return FieldAccessor(this); }
  ConstFieldAccessor fields() const { return ConstFieldAccessor(this); }
  OneofAccessor oneofs() { return OneofAccessor(this); }
  ConstOneofAccessor oneofs() const { return ConstOneofAccessor(this); }

 private:
  UPB_DISALLOW_POD_OPS(MessageDef, upb::MessageDef)
};

#endif  /* __cplusplus */

UPB_BEGIN_EXTERN_C

/* Returns NULL if memory allocation failed. */
upb_msgdef *upb_msgdef_new(const void *owner);

/* Include upb_refcounted methods like upb_msgdef_ref(). */
UPB_REFCOUNTED_CMETHODS(upb_msgdef, upb_msgdef_upcast2)

bool upb_msgdef_freeze(upb_msgdef *m, upb_status *status);

const char *upb_msgdef_fullname(const upb_msgdef *m);
bool upb_msgdef_setfullname(upb_msgdef *m, const char *fullname, upb_status *s);

upb_msgdef *upb_msgdef_dup(const upb_msgdef *m, const void *owner);
bool upb_msgdef_addfield(upb_msgdef *m, upb_fielddef *f, const void *ref_donor,
                         upb_status *s);
bool upb_msgdef_addoneof(upb_msgdef *m, upb_oneofdef *o, const void *ref_donor,
                         upb_status *s);

/* Field lookup in a couple of different variations:
 *   - itof = int to field
 *   - ntof = name to field
 *   - ntofz = name to field, null-terminated string. */
const upb_fielddef *upb_msgdef_itof(const upb_msgdef *m, uint32_t i);
const upb_fielddef *upb_msgdef_ntof(const upb_msgdef *m, const char *name,
                                    size_t len);
int upb_msgdef_numfields(const upb_msgdef *m);

UPB_INLINE const upb_fielddef *upb_msgdef_ntofz(const upb_msgdef *m,
                                                const char *name) {
  return upb_msgdef_ntof(m, name, strlen(name));
}

UPB_INLINE upb_fielddef *upb_msgdef_itof_mutable(upb_msgdef *m, uint32_t i) {
  return (upb_fielddef*)upb_msgdef_itof(m, i);
}

UPB_INLINE upb_fielddef *upb_msgdef_ntof_mutable(upb_msgdef *m,
                                                 const char *name, size_t len) {
  return (upb_fielddef *)upb_msgdef_ntof(m, name, len);
}

/* Oneof lookup:
 *   - ntoo = name to oneof
 *   - ntooz = name to oneof, null-terminated string. */
const upb_oneofdef *upb_msgdef_ntoo(const upb_msgdef *m, const char *name,
                                    size_t len);
int upb_msgdef_numoneofs(const upb_msgdef *m);

UPB_INLINE const upb_oneofdef *upb_msgdef_ntooz(const upb_msgdef *m,
                                               const char *name) {
  return upb_msgdef_ntoo(m, name, strlen(name));
}

UPB_INLINE upb_oneofdef *upb_msgdef_ntoo_mutable(upb_msgdef *m,
                                                 const char *name, size_t len) {
  return (upb_oneofdef *)upb_msgdef_ntoo(m, name, len);
}

void upb_msgdef_setmapentry(upb_msgdef *m, bool map_entry);
bool upb_msgdef_mapentry(const upb_msgdef *m);

/* Well-known field tag numbers for map-entry messages. */
#define UPB_MAPENTRY_KEY   1
#define UPB_MAPENTRY_VALUE 2

const upb_oneofdef *upb_msgdef_findoneof(const upb_msgdef *m,
                                          const char *name);
int upb_msgdef_numoneofs(const upb_msgdef *m);

/* upb_msg_field_iter i;
 * for(upb_msg_field_begin(&i, m);
 *     !upb_msg_field_done(&i);
 *     upb_msg_field_next(&i)) {
 *   upb_fielddef *f = upb_msg_iter_field(&i);
 *   // ...
 * }
 *
 * For C we don't have separate iterators for const and non-const.
 * It is the caller's responsibility to cast the upb_fielddef* to
 * const if the upb_msgdef* is const. */
void upb_msg_field_begin(upb_msg_field_iter *iter, const upb_msgdef *m);
void upb_msg_field_next(upb_msg_field_iter *iter);
bool upb_msg_field_done(const upb_msg_field_iter *iter);
upb_fielddef *upb_msg_iter_field(const upb_msg_field_iter *iter);
void upb_msg_field_iter_setdone(upb_msg_field_iter *iter);

/* Similar to above, we also support iterating through the oneofs in a
 * msgdef. */
void upb_msg_oneof_begin(upb_msg_oneof_iter *iter, const upb_msgdef *m);
void upb_msg_oneof_next(upb_msg_oneof_iter *iter);
bool upb_msg_oneof_done(const upb_msg_oneof_iter *iter);
upb_oneofdef *upb_msg_iter_oneof(const upb_msg_oneof_iter *iter);
void upb_msg_oneof_iter_setdone(upb_msg_oneof_iter *iter);

UPB_END_EXTERN_C


/* upb::EnumDef ***************************************************************/

typedef upb_strtable_iter upb_enum_iter;

#ifdef __cplusplus

/* Class that represents an enum.  Its base class is upb::Def (convert with
 * upb::upcast()). */
class upb::EnumDef {
 public:
  /* Returns NULL if memory allocation failed. */
  static reffed_ptr<EnumDef> New();

  /* upb::RefCounted methods like Ref()/Unref(). */
  UPB_REFCOUNTED_CPPMETHODS

  /* Functionality from upb::Def. */
  const char* full_name() const;
  bool set_full_name(const char* fullname, Status* s);
  bool set_full_name(const std::string& fullname, Status* s);

  /* Call to freeze this EnumDef. */
  bool Freeze(Status* s);

  /* The value that is used as the default when no field default is specified.
   * If not set explicitly, the first value that was added will be used.
   * The default value must be a member of the enum.
   * Requires that value_count() > 0. */
  int32_t default_value() const;

  /* Sets the default value.  If this value is not valid, returns false and an
   * error message in status. */
  bool set_default_value(int32_t val, Status* status);

  /* Returns the number of values currently defined in the enum.  Note that
   * multiple names can refer to the same number, so this may be greater than
   * the total number of unique numbers. */
  int value_count() const;

  /* Adds a single name/number pair to the enum.  Fails if this name has
   * already been used by another value. */
  bool AddValue(const char* name, int32_t num, Status* status);
  bool AddValue(const std::string& name, int32_t num, Status* status);

  /* Lookups from name to integer, returning true if found. */
  bool FindValueByName(const char* name, int32_t* num) const;

  /* Finds the name corresponding to the given number, or NULL if none was
   * found.  If more than one name corresponds to this number, returns the
   * first one that was added. */
  const char* FindValueByNumber(int32_t num) const;

  /* Returns a new EnumDef with all the same values.  The new EnumDef will be
   * owned by the given owner. */
  EnumDef* Dup(const void* owner) const;

  /* Iteration over name/value pairs.  The order is undefined.
   * Adding an enum val invalidates any iterators.
   *
   * TODO: make compatible with range-for, with elements as pairs? */
  class Iterator {
   public:
    explicit Iterator(const EnumDef*);

    int32_t number();
    const char *name();
    bool Done();
    void Next();

   private:
    upb_enum_iter iter_;
  };

 private:
  UPB_DISALLOW_POD_OPS(EnumDef, upb::EnumDef)
};

#endif  /* __cplusplus */

UPB_BEGIN_EXTERN_C

/* Native C API. */
upb_enumdef *upb_enumdef_new(const void *owner);
upb_enumdef *upb_enumdef_dup(const upb_enumdef *e, const void *owner);

/* Include upb_refcounted methods like upb_enumdef_ref(). */
UPB_REFCOUNTED_CMETHODS(upb_enumdef, upb_enumdef_upcast2)

bool upb_enumdef_freeze(upb_enumdef *e, upb_status *status);

/* From upb_def. */
const char *upb_enumdef_fullname(const upb_enumdef *e);
bool upb_enumdef_setfullname(upb_enumdef *e, const char *fullname,
                             upb_status *s);

int32_t upb_enumdef_default(const upb_enumdef *e);
bool upb_enumdef_setdefault(upb_enumdef *e, int32_t val, upb_status *s);
int upb_enumdef_numvals(const upb_enumdef *e);
bool upb_enumdef_addval(upb_enumdef *e, const char *name, int32_t num,
                        upb_status *status);

/* Enum lookups:
 * - ntoi:  look up a name with specified length.
 * - ntoiz: look up a name provided as a null-terminated string.
 * - iton:  look up an integer, returning the name as a null-terminated
 *          string. */
bool upb_enumdef_ntoi(const upb_enumdef *e, const char *name, size_t len,
                      int32_t *num);
UPB_INLINE bool upb_enumdef_ntoiz(const upb_enumdef *e,
                                  const char *name, int32_t *num) {
  return upb_enumdef_ntoi(e, name, strlen(name), num);
}
const char *upb_enumdef_iton(const upb_enumdef *e, int32_t num);

/*  upb_enum_iter i;
 *  for(upb_enum_begin(&i, e); !upb_enum_done(&i); upb_enum_next(&i)) {
 *    // ...
 *  }
 */
void upb_enum_begin(upb_enum_iter *iter, const upb_enumdef *e);
void upb_enum_next(upb_enum_iter *iter);
bool upb_enum_done(upb_enum_iter *iter);
const char *upb_enum_iter_name(upb_enum_iter *iter);
int32_t upb_enum_iter_number(upb_enum_iter *iter);

UPB_END_EXTERN_C

/* upb::OneofDef **************************************************************/

typedef upb_inttable_iter upb_oneof_iter;

#ifdef __cplusplus

/* Class that represents a oneof.  Its base class is upb::Def (convert with
 * upb::upcast()). */
class upb::OneofDef {
 public:
  /* Returns NULL if memory allocation failed. */
  static reffed_ptr<OneofDef> New();

  /* upb::RefCounted methods like Ref()/Unref(). */
  UPB_REFCOUNTED_CPPMETHODS

  /* Functionality from upb::Def. */
  const char* full_name() const;

  /* Returns the MessageDef that owns this OneofDef. */
  const MessageDef* containing_type() const;

  /* Returns the name of this oneof. This is the name used to look up the oneof
   * by name once added to a message def. */
  const char* name() const;
  bool set_name(const char* name, Status* s);

  /* Returns the number of fields currently defined in the oneof. */
  int field_count() const;

  /* Adds a field to the oneof. The field must not have been added to any other
   * oneof or msgdef. If the oneof is not yet part of a msgdef, then when the
   * oneof is eventually added to a msgdef, all fields added to the oneof will
   * also be added to the msgdef at that time. If the oneof is already part of a
   * msgdef, the field must either be a part of that msgdef already, or must not
   * be a part of any msgdef; in the latter case, the field is added to the
   * msgdef as a part of this operation.
   *
   * The field may only have an OPTIONAL label, never REQUIRED or REPEATED.
   *
   * If |f| is already part of this MessageDef, this method performs no action
   * and returns true (success). Thus, this method is idempotent. */
  bool AddField(FieldDef* field, Status* s);
  bool AddField(const reffed_ptr<FieldDef>& field, Status* s);

  /* Looks up by name. */
  const FieldDef* FindFieldByName(const char* name, size_t len) const;
  FieldDef* FindFieldByName(const char* name, size_t len);
  const FieldDef* FindFieldByName(const char* name) const {
    return FindFieldByName(name, strlen(name));
  }
  FieldDef* FindFieldByName(const char* name) {
    return FindFieldByName(name, strlen(name));
  }

  template <class T>
  FieldDef* FindFieldByName(const T& str) {
    return FindFieldByName(str.c_str(), str.size());
  }
  template <class T>
  const FieldDef* FindFieldByName(const T& str) const {
    return FindFieldByName(str.c_str(), str.size());
  }

  /* Looks up by tag number. */
  const FieldDef* FindFieldByNumber(uint32_t num) const;

  /* Returns a new OneofDef with all the same fields. The OneofDef will be owned
   * by the given owner. */
  OneofDef* Dup(const void* owner) const;

  /* Iteration over fields.  The order is undefined. */
  class iterator : public std::iterator<std::forward_iterator_tag, FieldDef*> {
   public:
    explicit iterator(OneofDef* md);
    static iterator end(OneofDef* md);

    void operator++();
    FieldDef* operator*() const;
    bool operator!=(const iterator& other) const;
    bool operator==(const iterator& other) const;

   private:
    upb_oneof_iter iter_;
  };

  class const_iterator
      : public std::iterator<std::forward_iterator_tag, const FieldDef*> {
   public:
    explicit const_iterator(const OneofDef* md);
    static const_iterator end(const OneofDef* md);

    void operator++();
    const FieldDef* operator*() const;
    bool operator!=(const const_iterator& other) const;
    bool operator==(const const_iterator& other) const;

   private:
    upb_oneof_iter iter_;
  };

  iterator begin();
  iterator end();
  const_iterator begin() const;
  const_iterator end() const;

 private:
  UPB_DISALLOW_POD_OPS(OneofDef, upb::OneofDef)
};

#endif  /* __cplusplus */

UPB_BEGIN_EXTERN_C

/* Native C API. */
upb_oneofdef *upb_oneofdef_new(const void *owner);
upb_oneofdef *upb_oneofdef_dup(const upb_oneofdef *o, const void *owner);

/* Include upb_refcounted methods like upb_oneofdef_ref(). */
UPB_REFCOUNTED_CMETHODS(upb_oneofdef, upb_oneofdef_upcast2)

const char *upb_oneofdef_name(const upb_oneofdef *o);
bool upb_oneofdef_setname(upb_oneofdef *o, const char *name, upb_status *s);

const upb_msgdef *upb_oneofdef_containingtype(const upb_oneofdef *o);
int upb_oneofdef_numfields(const upb_oneofdef *o);
bool upb_oneofdef_addfield(upb_oneofdef *o, upb_fielddef *f,
                           const void *ref_donor,
                           upb_status *s);

/* Oneof lookups:
 * - ntof:  look up a field by name.
 * - ntofz: look up a field by name (as a null-terminated string).
 * - itof:  look up a field by number. */
const upb_fielddef *upb_oneofdef_ntof(const upb_oneofdef *o,
                                      const char *name, size_t length);
UPB_INLINE const upb_fielddef *upb_oneofdef_ntofz(const upb_oneofdef *o,
                                                  const char *name) {
  return upb_oneofdef_ntof(o, name, strlen(name));
}
const upb_fielddef *upb_oneofdef_itof(const upb_oneofdef *o, uint32_t num);

/*  upb_oneof_iter i;
 *  for(upb_oneof_begin(&i, e); !upb_oneof_done(&i); upb_oneof_next(&i)) {
 *    // ...
 *  }
 */
void upb_oneof_begin(upb_oneof_iter *iter, const upb_oneofdef *o);
void upb_oneof_next(upb_oneof_iter *iter);
bool upb_oneof_done(upb_oneof_iter *iter);
upb_fielddef *upb_oneof_iter_field(const upb_oneof_iter *iter);
void upb_oneof_iter_setdone(upb_oneof_iter *iter);

UPB_END_EXTERN_C

#ifdef __cplusplus

UPB_INLINE const char* upb_safecstr(const std::string& str) {
  assert(str.size() == std::strlen(str.c_str()));
  return str.c_str();
}

/* Inline C++ wrappers. */
namespace upb {

inline Def* Def::Dup(const void* owner) const {
  return upb_def_dup(this, owner);
}
inline Def::Type Def::def_type() const { return upb_def_type(this); }
inline const char* Def::full_name() const { return upb_def_fullname(this); }
inline bool Def::set_full_name(const char* fullname, Status* s) {
  return upb_def_setfullname(this, fullname, s);
}
inline bool Def::set_full_name(const std::string& fullname, Status* s) {
  return upb_def_setfullname(this, upb_safecstr(fullname), s);
}
inline bool Def::Freeze(Def* const* defs, int n, Status* status) {
  return upb_def_freeze(defs, n, status);
}
inline bool Def::Freeze(const std::vector<Def*>& defs, Status* status) {
  return upb_def_freeze((Def* const*)&defs[0], defs.size(), status);
}

inline bool FieldDef::CheckType(int32_t val) {
  return upb_fielddef_checktype(val);
}
inline bool FieldDef::CheckLabel(int32_t val) {
  return upb_fielddef_checklabel(val);
}
inline bool FieldDef::CheckDescriptorType(int32_t val) {
  return upb_fielddef_checkdescriptortype(val);
}
inline bool FieldDef::CheckIntegerFormat(int32_t val) {
  return upb_fielddef_checkintfmt(val);
}
inline FieldDef::Type FieldDef::ConvertType(int32_t val) {
  assert(CheckType(val));
  return static_cast<FieldDef::Type>(val);
}
inline FieldDef::Label FieldDef::ConvertLabel(int32_t val) {
  assert(CheckLabel(val));
  return static_cast<FieldDef::Label>(val);
}
inline FieldDef::DescriptorType FieldDef::ConvertDescriptorType(int32_t val) {
  assert(CheckDescriptorType(val));
  return static_cast<FieldDef::DescriptorType>(val);
}
inline FieldDef::IntegerFormat FieldDef::ConvertIntegerFormat(int32_t val) {
  assert(CheckIntegerFormat(val));
  return static_cast<FieldDef::IntegerFormat>(val);
}

inline reffed_ptr<FieldDef> FieldDef::New() {
  upb_fielddef *f = upb_fielddef_new(&f);
  return reffed_ptr<FieldDef>(f, &f);
}
inline FieldDef* FieldDef::Dup(const void* owner) const {
  return upb_fielddef_dup(this, owner);
}
inline const char* FieldDef::full_name() const {
  return upb_fielddef_fullname(this);
}
inline bool FieldDef::set_full_name(const char* fullname, Status* s) {
  return upb_fielddef_setfullname(this, fullname, s);
}
inline bool FieldDef::set_full_name(const std::string& fullname, Status* s) {
  return upb_fielddef_setfullname(this, upb_safecstr(fullname), s);
}
inline bool FieldDef::type_is_set() const {
  return upb_fielddef_typeisset(this);
}
inline FieldDef::Type FieldDef::type() const { return upb_fielddef_type(this); }
inline FieldDef::DescriptorType FieldDef::descriptor_type() const {
  return upb_fielddef_descriptortype(this);
}
inline FieldDef::Label FieldDef::label() const {
  return upb_fielddef_label(this);
}
inline uint32_t FieldDef::number() const { return upb_fielddef_number(this); }
inline const char* FieldDef::name() const { return upb_fielddef_name(this); }
inline bool FieldDef::is_extension() const {
  return upb_fielddef_isextension(this);
}
inline bool FieldDef::lazy() const {
  return upb_fielddef_lazy(this);
}
inline void FieldDef::set_lazy(bool lazy) {
  upb_fielddef_setlazy(this, lazy);
}
inline bool FieldDef::packed() const {
  return upb_fielddef_packed(this);
}
inline void FieldDef::set_packed(bool packed) {
  upb_fielddef_setpacked(this, packed);
}
inline const MessageDef* FieldDef::containing_type() const {
  return upb_fielddef_containingtype(this);
}
inline const OneofDef* FieldDef::containing_oneof() const {
  return upb_fielddef_containingoneof(this);
}
inline const char* FieldDef::containing_type_name() {
  return upb_fielddef_containingtypename(this);
}
inline bool FieldDef::set_number(uint32_t number, Status* s) {
  return upb_fielddef_setnumber(this, number, s);
}
inline bool FieldDef::set_name(const char *name, Status* s) {
  return upb_fielddef_setname(this, name, s);
}
inline bool FieldDef::set_name(const std::string& name, Status* s) {
  return upb_fielddef_setname(this, upb_safecstr(name), s);
}
inline bool FieldDef::set_containing_type_name(const char *name, Status* s) {
  return upb_fielddef_setcontainingtypename(this, name, s);
}
inline bool FieldDef::set_containing_type_name(const std::string &name,
                                               Status *s) {
  return upb_fielddef_setcontainingtypename(this, upb_safecstr(name), s);
}
inline void FieldDef::set_type(upb_fieldtype_t type) {
  upb_fielddef_settype(this, type);
}
inline void FieldDef::set_is_extension(bool is_extension) {
  upb_fielddef_setisextension(this, is_extension);
}
inline void FieldDef::set_descriptor_type(FieldDef::DescriptorType type) {
  upb_fielddef_setdescriptortype(this, type);
}
inline void FieldDef::set_label(upb_label_t label) {
  upb_fielddef_setlabel(this, label);
}
inline bool FieldDef::IsSubMessage() const {
  return upb_fielddef_issubmsg(this);
}
inline bool FieldDef::IsString() const { return upb_fielddef_isstring(this); }
inline bool FieldDef::IsSequence() const { return upb_fielddef_isseq(this); }
inline bool FieldDef::IsMap() const { return upb_fielddef_ismap(this); }
inline int64_t FieldDef::default_int64() const {
  return upb_fielddef_defaultint64(this);
}
inline int32_t FieldDef::default_int32() const {
  return upb_fielddef_defaultint32(this);
}
inline uint64_t FieldDef::default_uint64() const {
  return upb_fielddef_defaultuint64(this);
}
inline uint32_t FieldDef::default_uint32() const {
  return upb_fielddef_defaultuint32(this);
}
inline bool FieldDef::default_bool() const {
  return upb_fielddef_defaultbool(this);
}
inline float FieldDef::default_float() const {
  return upb_fielddef_defaultfloat(this);
}
inline double FieldDef::default_double() const {
  return upb_fielddef_defaultdouble(this);
}
inline const char* FieldDef::default_string(size_t* len) const {
  return upb_fielddef_defaultstr(this, len);
}
inline void FieldDef::set_default_int64(int64_t value) {
  upb_fielddef_setdefaultint64(this, value);
}
inline void FieldDef::set_default_int32(int32_t value) {
  upb_fielddef_setdefaultint32(this, value);
}
inline void FieldDef::set_default_uint64(uint64_t value) {
  upb_fielddef_setdefaultuint64(this, value);
}
inline void FieldDef::set_default_uint32(uint32_t value) {
  upb_fielddef_setdefaultuint32(this, value);
}
inline void FieldDef::set_default_bool(bool value) {
  upb_fielddef_setdefaultbool(this, value);
}
inline void FieldDef::set_default_float(float value) {
  upb_fielddef_setdefaultfloat(this, value);
}
inline void FieldDef::set_default_double(double value) {
  upb_fielddef_setdefaultdouble(this, value);
}
inline bool FieldDef::set_default_string(const void *str, size_t len,
                                         Status *s) {
  return upb_fielddef_setdefaultstr(this, str, len, s);
}
inline bool FieldDef::set_default_string(const std::string& str, Status* s) {
  return upb_fielddef_setdefaultstr(this, str.c_str(), str.size(), s);
}
inline void FieldDef::set_default_cstr(const char* str, Status* s) {
  return upb_fielddef_setdefaultcstr(this, str, s);
}
inline bool FieldDef::HasSubDef() const { return upb_fielddef_hassubdef(this); }
inline const Def* FieldDef::subdef() const { return upb_fielddef_subdef(this); }
inline const MessageDef *FieldDef::message_subdef() const {
  return upb_fielddef_msgsubdef(this);
}
inline const EnumDef *FieldDef::enum_subdef() const {
  return upb_fielddef_enumsubdef(this);
}
inline const char* FieldDef::subdef_name() const {
  return upb_fielddef_subdefname(this);
}
inline bool FieldDef::set_subdef(const Def* subdef, Status* s) {
  return upb_fielddef_setsubdef(this, subdef, s);
}
inline bool FieldDef::set_enum_subdef(const EnumDef* subdef, Status* s) {
  return upb_fielddef_setenumsubdef(this, subdef, s);
}
inline bool FieldDef::set_message_subdef(const MessageDef* subdef, Status* s) {
  return upb_fielddef_setmsgsubdef(this, subdef, s);
}
inline bool FieldDef::set_subdef_name(const char* name, Status* s) {
  return upb_fielddef_setsubdefname(this, name, s);
}
inline bool FieldDef::set_subdef_name(const std::string& name, Status* s) {
  return upb_fielddef_setsubdefname(this, upb_safecstr(name), s);
}

inline reffed_ptr<MessageDef> MessageDef::New() {
  upb_msgdef *m = upb_msgdef_new(&m);
  return reffed_ptr<MessageDef>(m, &m);
}
inline const char *MessageDef::full_name() const {
  return upb_msgdef_fullname(this);
}
inline bool MessageDef::set_full_name(const char* fullname, Status* s) {
  return upb_msgdef_setfullname(this, fullname, s);
}
inline bool MessageDef::set_full_name(const std::string& fullname, Status* s) {
  return upb_msgdef_setfullname(this, upb_safecstr(fullname), s);
}
inline bool MessageDef::Freeze(Status* status) {
  return upb_msgdef_freeze(this, status);
}
inline int MessageDef::field_count() const {
  return upb_msgdef_numfields(this);
}
inline int MessageDef::oneof_count() const {
  return upb_msgdef_numoneofs(this);
}
inline bool MessageDef::AddField(upb_fielddef* f, Status* s) {
  return upb_msgdef_addfield(this, f, NULL, s);
}
inline bool MessageDef::AddField(const reffed_ptr<FieldDef>& f, Status* s) {
  return upb_msgdef_addfield(this, f.get(), NULL, s);
}
inline bool MessageDef::AddOneof(upb_oneofdef* o, Status* s) {
  return upb_msgdef_addoneof(this, o, NULL, s);
}
inline bool MessageDef::AddOneof(const reffed_ptr<OneofDef>& o, Status* s) {
  return upb_msgdef_addoneof(this, o.get(), NULL, s);
}
inline FieldDef* MessageDef::FindFieldByNumber(uint32_t number) {
  return upb_msgdef_itof_mutable(this, number);
}
inline FieldDef* MessageDef::FindFieldByName(const char* name, size_t len) {
  return upb_msgdef_ntof_mutable(this, name, len);
}
inline const FieldDef* MessageDef::FindFieldByNumber(uint32_t number) const {
  return upb_msgdef_itof(this, number);
}
inline const FieldDef *MessageDef::FindFieldByName(const char *name,
                                                   size_t len) const {
  return upb_msgdef_ntof(this, name, len);
}
inline OneofDef* MessageDef::FindOneofByName(const char* name, size_t len) {
  return upb_msgdef_ntoo_mutable(this, name, len);
}
inline const OneofDef* MessageDef::FindOneofByName(const char* name,
                                                   size_t len) const {
  return upb_msgdef_ntoo(this, name, len);
}
inline MessageDef* MessageDef::Dup(const void *owner) const {
  return upb_msgdef_dup(this, owner);
}
inline void MessageDef::setmapentry(bool map_entry) {
  upb_msgdef_setmapentry(this, map_entry);
}
inline bool MessageDef::mapentry() const {
  return upb_msgdef_mapentry(this);
}
inline MessageDef::field_iterator MessageDef::field_begin() {
  return field_iterator(this);
}
inline MessageDef::field_iterator MessageDef::field_end() {
  return field_iterator::end(this);
}
inline MessageDef::const_field_iterator MessageDef::field_begin() const {
  return const_field_iterator(this);
}
inline MessageDef::const_field_iterator MessageDef::field_end() const {
  return const_field_iterator::end(this);
}

inline MessageDef::oneof_iterator MessageDef::oneof_begin() {
  return oneof_iterator(this);
}
inline MessageDef::oneof_iterator MessageDef::oneof_end() {
  return oneof_iterator::end(this);
}
inline MessageDef::const_oneof_iterator MessageDef::oneof_begin() const {
  return const_oneof_iterator(this);
}
inline MessageDef::const_oneof_iterator MessageDef::oneof_end() const {
  return const_oneof_iterator::end(this);
}

inline MessageDef::field_iterator::field_iterator(MessageDef* md) {
  upb_msg_field_begin(&iter_, md);
}
inline MessageDef::field_iterator MessageDef::field_iterator::end(
    MessageDef* md) {
  MessageDef::field_iterator iter(md);
  upb_msg_field_iter_setdone(&iter.iter_);
  return iter;
}
inline FieldDef* MessageDef::field_iterator::operator*() const {
  return upb_msg_iter_field(&iter_);
}
inline void MessageDef::field_iterator::operator++() {
  return upb_msg_field_next(&iter_);
}
inline bool MessageDef::field_iterator::operator==(
    const field_iterator &other) const {
  return upb_inttable_iter_isequal(&iter_, &other.iter_);
}
inline bool MessageDef::field_iterator::operator!=(
    const field_iterator &other) const {
  return !(*this == other);
}

inline MessageDef::const_field_iterator::const_field_iterator(
    const MessageDef* md) {
  upb_msg_field_begin(&iter_, md);
}
inline MessageDef::const_field_iterator MessageDef::const_field_iterator::end(
    const MessageDef *md) {
  MessageDef::const_field_iterator iter(md);
  upb_msg_field_iter_setdone(&iter.iter_);
  return iter;
}
inline const FieldDef* MessageDef::const_field_iterator::operator*() const {
  return upb_msg_iter_field(&iter_);
}
inline void MessageDef::const_field_iterator::operator++() {
  return upb_msg_field_next(&iter_);
}
inline bool MessageDef::const_field_iterator::operator==(
    const const_field_iterator &other) const {
  return upb_inttable_iter_isequal(&iter_, &other.iter_);
}
inline bool MessageDef::const_field_iterator::operator!=(
    const const_field_iterator &other) const {
  return !(*this == other);
}

inline MessageDef::oneof_iterator::oneof_iterator(MessageDef* md) {
  upb_msg_oneof_begin(&iter_, md);
}
inline MessageDef::oneof_iterator MessageDef::oneof_iterator::end(
    MessageDef* md) {
  MessageDef::oneof_iterator iter(md);
  upb_msg_oneof_iter_setdone(&iter.iter_);
  return iter;
}
inline OneofDef* MessageDef::oneof_iterator::operator*() const {
  return upb_msg_iter_oneof(&iter_);
}
inline void MessageDef::oneof_iterator::operator++() {
  return upb_msg_oneof_next(&iter_);
}
inline bool MessageDef::oneof_iterator::operator==(
    const oneof_iterator &other) const {
  return upb_strtable_iter_isequal(&iter_, &other.iter_);
}
inline bool MessageDef::oneof_iterator::operator!=(
    const oneof_iterator &other) const {
  return !(*this == other);
}

inline MessageDef::const_oneof_iterator::const_oneof_iterator(
    const MessageDef* md) {
  upb_msg_oneof_begin(&iter_, md);
}
inline MessageDef::const_oneof_iterator MessageDef::const_oneof_iterator::end(
    const MessageDef *md) {
  MessageDef::const_oneof_iterator iter(md);
  upb_msg_oneof_iter_setdone(&iter.iter_);
  return iter;
}
inline const OneofDef* MessageDef::const_oneof_iterator::operator*() const {
  return upb_msg_iter_oneof(&iter_);
}
inline void MessageDef::const_oneof_iterator::operator++() {
  return upb_msg_oneof_next(&iter_);
}
inline bool MessageDef::const_oneof_iterator::operator==(
    const const_oneof_iterator &other) const {
  return upb_strtable_iter_isequal(&iter_, &other.iter_);
}
inline bool MessageDef::const_oneof_iterator::operator!=(
    const const_oneof_iterator &other) const {
  return !(*this == other);
}

inline reffed_ptr<EnumDef> EnumDef::New() {
  upb_enumdef *e = upb_enumdef_new(&e);
  return reffed_ptr<EnumDef>(e, &e);
}
inline const char* EnumDef::full_name() const {
  return upb_enumdef_fullname(this);
}
inline bool EnumDef::set_full_name(const char* fullname, Status* s) {
  return upb_enumdef_setfullname(this, fullname, s);
}
inline bool EnumDef::set_full_name(const std::string& fullname, Status* s) {
  return upb_enumdef_setfullname(this, upb_safecstr(fullname), s);
}
inline bool EnumDef::Freeze(Status* status) {
  return upb_enumdef_freeze(this, status);
}
inline int32_t EnumDef::default_value() const {
  return upb_enumdef_default(this);
}
inline bool EnumDef::set_default_value(int32_t val, Status* status) {
  return upb_enumdef_setdefault(this, val, status);
}
inline int EnumDef::value_count() const { return upb_enumdef_numvals(this); }
inline bool EnumDef::AddValue(const char* name, int32_t num, Status* status) {
  return upb_enumdef_addval(this, name, num, status);
}
inline bool EnumDef::AddValue(const std::string& name, int32_t num,
                              Status* status) {
  return upb_enumdef_addval(this, upb_safecstr(name), num, status);
}
inline bool EnumDef::FindValueByName(const char* name, int32_t *num) const {
  return upb_enumdef_ntoiz(this, name, num);
}
inline const char* EnumDef::FindValueByNumber(int32_t num) const {
  return upb_enumdef_iton(this, num);
}
inline EnumDef* EnumDef::Dup(const void* owner) const {
  return upb_enumdef_dup(this, owner);
}

inline EnumDef::Iterator::Iterator(const EnumDef* e) {
  upb_enum_begin(&iter_, e);
}
inline int32_t EnumDef::Iterator::number() {
  return upb_enum_iter_number(&iter_);
}
inline const char* EnumDef::Iterator::name() {
  return upb_enum_iter_name(&iter_);
}
inline bool EnumDef::Iterator::Done() { return upb_enum_done(&iter_); }
inline void EnumDef::Iterator::Next() { return upb_enum_next(&iter_); }

inline reffed_ptr<OneofDef> OneofDef::New() {
  upb_oneofdef *o = upb_oneofdef_new(&o);
  return reffed_ptr<OneofDef>(o, &o);
}
inline const char* OneofDef::full_name() const {
  return upb_oneofdef_name(this);
}

inline const MessageDef* OneofDef::containing_type() const {
  return upb_oneofdef_containingtype(this);
}
inline const char* OneofDef::name() const {
  return upb_oneofdef_name(this);
}
inline bool OneofDef::set_name(const char* name, Status* s) {
  return upb_oneofdef_setname(this, name, s);
}
inline int OneofDef::field_count() const {
  return upb_oneofdef_numfields(this);
}
inline bool OneofDef::AddField(FieldDef* field, Status* s) {
  return upb_oneofdef_addfield(this, field, NULL, s);
}
inline bool OneofDef::AddField(const reffed_ptr<FieldDef>& field, Status* s) {
  return upb_oneofdef_addfield(this, field.get(), NULL, s);
}
inline const FieldDef* OneofDef::FindFieldByName(const char* name,
                                                 size_t len) const {
  return upb_oneofdef_ntof(this, name, len);
}
inline const FieldDef* OneofDef::FindFieldByNumber(uint32_t num) const {
  return upb_oneofdef_itof(this, num);
}
inline OneofDef::iterator OneofDef::begin() { return iterator(this); }
inline OneofDef::iterator OneofDef::end() { return iterator::end(this); }
inline OneofDef::const_iterator OneofDef::begin() const {
  return const_iterator(this);
}
inline OneofDef::const_iterator OneofDef::end() const {
  return const_iterator::end(this);
}

inline OneofDef::iterator::iterator(OneofDef* o) {
  upb_oneof_begin(&iter_, o);
}
inline OneofDef::iterator OneofDef::iterator::end(OneofDef* o) {
  OneofDef::iterator iter(o);
  upb_oneof_iter_setdone(&iter.iter_);
  return iter;
}
inline FieldDef* OneofDef::iterator::operator*() const {
  return upb_oneof_iter_field(&iter_);
}
inline void OneofDef::iterator::operator++() { return upb_oneof_next(&iter_); }
inline bool OneofDef::iterator::operator==(const iterator &other) const {
  return upb_inttable_iter_isequal(&iter_, &other.iter_);
}
inline bool OneofDef::iterator::operator!=(const iterator &other) const {
  return !(*this == other);
}

inline OneofDef::const_iterator::const_iterator(const OneofDef* md) {
  upb_oneof_begin(&iter_, md);
}
inline OneofDef::const_iterator OneofDef::const_iterator::end(
    const OneofDef *md) {
  OneofDef::const_iterator iter(md);
  upb_oneof_iter_setdone(&iter.iter_);
  return iter;
}
inline const FieldDef* OneofDef::const_iterator::operator*() const {
  return upb_msg_iter_field(&iter_);
}
inline void OneofDef::const_iterator::operator++() {
  return upb_oneof_next(&iter_);
}
inline bool OneofDef::const_iterator::operator==(
    const const_iterator &other) const {
  return upb_inttable_iter_isequal(&iter_, &other.iter_);
}
inline bool OneofDef::const_iterator::operator!=(
    const const_iterator &other) const {
  return !(*this == other);
}

}  /* namespace upb */
#endif

#endif /* UPB_DEF_H_ */
/*
** This file contains definitions of structs that should be considered private
** and NOT stable across versions of upb.
**
** The only reason they are declared here and not in .c files is to allow upb
** and the application (if desired) to embed statically-initialized instances
** of structures like defs.
**
** If you include this file, all guarantees of ABI compatibility go out the
** window!  Any code that includes this file needs to recompile against the
** exact same version of upb that they are linking against.
**
** You also need to recompile if you change the value of the UPB_DEBUG_REFS
** flag.
*/


#ifndef UPB_STATICINIT_H_
#define UPB_STATICINIT_H_

#ifdef __cplusplus
/* Because of how we do our typedefs, this header can't be included from C++. */
#error This file cannot be included from C++
#endif

/* upb_refcounted *************************************************************/


/* upb_def ********************************************************************/

struct upb_def {
  upb_refcounted base;

  const char *fullname;
  char type;  /* A upb_deftype_t (char to save space) */

  /* Used as a flag during the def's mutable stage.  Must be false unless
   * it is currently being used by a function on the stack.  This allows
   * us to easily determine which defs were passed into the function's
   * current invocation. */
  bool came_from_user;
};

#define UPB_DEF_INIT(name, type, refs, ref2s) \
    { UPB_REFCOUNT_INIT(refs, ref2s), name, type, false }


/* upb_fielddef ***************************************************************/

struct upb_fielddef {
  upb_def base;

  union {
    int64_t sint;
    uint64_t uint;
    double dbl;
    float flt;
    void *bytes;
  } defaultval;
  union {
    const upb_msgdef *def;  /* If !msg_is_symbolic. */
    char *name;             /* If msg_is_symbolic. */
  } msg;
  union {
    const upb_def *def;  /* If !subdef_is_symbolic. */
    char *name;          /* If subdef_is_symbolic. */
  } sub;  /* The msgdef or enumdef for this field, if upb_hassubdef(f). */
  bool subdef_is_symbolic;
  bool msg_is_symbolic;
  const upb_oneofdef *oneof;
  bool default_is_string;
  bool type_is_set_;     /* False until type is explicitly set. */
  bool is_extension_;
  bool lazy_;
  bool packed_;
  upb_intfmt_t intfmt;
  bool tagdelim;
  upb_fieldtype_t type_;
  upb_label_t label_;
  uint32_t number_;
  uint32_t selector_base;  /* Used to index into a upb::Handlers table. */
  uint32_t index_;
};

#define UPB_FIELDDEF_INIT(label, type, intfmt, tagdelim, is_extension, lazy,   \
                          packed, name, num, msgdef, subdef, selector_base,    \
                          index, defaultval, refs, ref2s)                      \
  {                                                                            \
    UPB_DEF_INIT(name, UPB_DEF_FIELD, refs, ref2s), defaultval, {msgdef},      \
        {subdef}, NULL, false, false,                                          \
        type == UPB_TYPE_STRING || type == UPB_TYPE_BYTES, true, is_extension, \
        lazy, packed, intfmt, tagdelim, type, label, num, selector_base, index \
  }


/* upb_msgdef *****************************************************************/

struct upb_msgdef {
  upb_def base;

  size_t selector_count;
  uint32_t submsg_field_count;

  /* Tables for looking up fields by number and name. */
  upb_inttable itof;  /* int to field */
  upb_strtable ntof;  /* name to field */

  /* Tables for looking up oneofs by name. */
  upb_strtable ntoo;  /* name to oneof */

  /* Is this a map-entry message?
   * TODO: set this flag properly for static descriptors; regenerate
   * descriptor.upb.c. */
  bool map_entry;

  /* TODO(haberman): proper extension ranges (there can be multiple). */
};

/* TODO: also support static initialization of the oneofs table. This will be
 * needed if we compile in descriptors that contain oneofs. */
#define UPB_MSGDEF_INIT(name, selector_count, submsg_field_count, itof, ntof, \
                        refs, ref2s)                                          \
  {                                                                           \
    UPB_DEF_INIT(name, UPB_DEF_MSG, refs, ref2s), selector_count,             \
        submsg_field_count, itof, ntof,                                       \
        UPB_EMPTY_STRTABLE_INIT(UPB_CTYPE_PTR), false                         \
  }


/* upb_enumdef ****************************************************************/

struct upb_enumdef {
  upb_def base;

  upb_strtable ntoi;
  upb_inttable iton;
  int32_t defaultval;
};

#define UPB_ENUMDEF_INIT(name, ntoi, iton, defaultval, refs, ref2s) \
  { UPB_DEF_INIT(name, UPB_DEF_ENUM, refs, ref2s), ntoi, iton, defaultval }


/* upb_oneofdef ***************************************************************/

struct upb_oneofdef {
  upb_def base;

  upb_strtable ntof;
  upb_inttable itof;
  const upb_msgdef *parent;
};

#define UPB_ONEOFDEF_INIT(name, ntof, itof, refs, ref2s) \
  { UPB_DEF_INIT(name, UPB_DEF_ENUM, refs, ref2s), ntof, itof }


/* upb_symtab *****************************************************************/

struct upb_symtab {
  upb_refcounted base;

  upb_strtable symtab;
};

#define UPB_SYMTAB_INIT(symtab, refs, ref2s) \
  { UPB_REFCOUNT_INIT(refs, ref2s), symtab }


#endif  /* UPB_STATICINIT_H_ */
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


#ifdef __cplusplus
namespace upb {
class BufferHandle;
class BytesHandler;
class HandlerAttributes;
class Handlers;
template <class T> class Handler;
template <class T> struct CanonicalType;
}  /* namespace upb */
#endif

UPB_DECLARE_TYPE(upb::BufferHandle, upb_bufhandle)
UPB_DECLARE_TYPE(upb::BytesHandler, upb_byteshandler)
UPB_DECLARE_TYPE(upb::HandlerAttributes, upb_handlerattr)
UPB_DECLARE_DERIVED_TYPE(upb::Handlers, upb::RefCounted,
                         upb_handlers, upb_refcounted)

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

UPB_BEGIN_EXTERN_C

/* Forward-declares for C inline accessors.  We need to declare these here
 * so we can "friend" them in the class declarations in C++. */
UPB_INLINE upb_func *upb_handlers_gethandler(const upb_handlers *h,
                                             upb_selector_t s);
UPB_INLINE const void *upb_handlerattr_handlerdata(const upb_handlerattr *attr);
UPB_INLINE const void *upb_handlers_gethandlerdata(const upb_handlers *h,
                                                   upb_selector_t s);

UPB_INLINE void upb_bufhandle_init(upb_bufhandle *h);
UPB_INLINE void upb_bufhandle_setobj(upb_bufhandle *h, const void *obj,
                                     const void *type);
UPB_INLINE void upb_bufhandle_setbuf(upb_bufhandle *h, const char *buf,
                                     size_t ofs);
UPB_INLINE const void *upb_bufhandle_obj(const upb_bufhandle *h);
UPB_INLINE const void *upb_bufhandle_objtype(const upb_bufhandle *h);
UPB_INLINE const char *upb_bufhandle_buf(const upb_bufhandle *h);

UPB_END_EXTERN_C


/* Static selectors for upb::Handlers. */
#define UPB_STARTMSG_SELECTOR 0
#define UPB_ENDMSG_SELECTOR 1
#define UPB_STATIC_SELECTOR_COUNT 2

/* Static selectors for upb::BytesHandler. */
#define UPB_STARTSTR_SELECTOR 0
#define UPB_STRING_SELECTOR 1
#define UPB_ENDSTR_SELECTOR 2

typedef void upb_handlerfree(void *d);

#ifdef __cplusplus

/* A set of attributes that accompanies a handler's function pointer. */
class upb::HandlerAttributes {
 public:
  HandlerAttributes();
  ~HandlerAttributes();

  /* Sets the handler data that will be passed as the second parameter of the
   * handler.  To free this pointer when the handlers are freed, call
   * Handlers::AddCleanup(). */
  bool SetHandlerData(const void *handler_data);
  const void* handler_data() const;

  /* Use this to specify the type of the closure.  This will be checked against
   * all other closure types for handler that use the same closure.
   * Registration will fail if this does not match all other non-NULL closure
   * types. */
  bool SetClosureType(const void *closure_type);
  const void* closure_type() const;

  /* Use this to specify the type of the returned closure.  Only used for
   * Start*{String,SubMessage,Sequence} handlers.  This must match the closure
   * type of any handlers that use it (for example, the StringBuf handler must
   * match the closure returned from StartString). */
  bool SetReturnClosureType(const void *return_closure_type);
  const void* return_closure_type() const;

  /* Set to indicate that the handler always returns "ok" (either "true" or a
   * non-NULL closure).  This is a hint that can allow code generators to
   * generate more efficient code. */
  bool SetAlwaysOk(bool always_ok);
  bool always_ok() const;

 private:
  friend UPB_INLINE const void * ::upb_handlerattr_handlerdata(
      const upb_handlerattr *attr);
#else
struct upb_handlerattr {
#endif
  const void *handler_data_;
  const void *closure_type_;
  const void *return_closure_type_;
  bool alwaysok_;
};

#define UPB_HANDLERATTR_INITIALIZER {NULL, NULL, NULL, false}

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

#ifdef __cplusplus

/* Extra information about a buffer that is passed to a StringBuf handler.
 * TODO(haberman): allow the handle to be pinned so that it will outlive
 * the handler invocation. */
class upb::BufferHandle {
 public:
  BufferHandle();
  ~BufferHandle();

  /* The beginning of the buffer.  This may be different than the pointer
   * passed to a StringBuf handler because the handler may receive data
   * that is from the middle or end of a larger buffer. */
  const char* buffer() const;

  /* The offset within the attached object where this buffer begins.  Only
   * meaningful if there is an attached object. */
  size_t object_offset() const;

  /* Note that object_offset is the offset of "buf" within the attached
   * object. */
  void SetBuffer(const char* buf, size_t object_offset);

  /* The BufferHandle can have an "attached object", which can be used to
   * tunnel through a pointer to the buffer's underlying representation. */
  template <class T>
  void SetAttachedObject(const T* obj);

  /* Returns NULL if the attached object is not of this type. */
  template <class T>
  const T* GetAttachedObject() const;

 private:
  friend UPB_INLINE void ::upb_bufhandle_init(upb_bufhandle *h);
  friend UPB_INLINE void ::upb_bufhandle_setobj(upb_bufhandle *h,
                                                const void *obj,
                                                const void *type);
  friend UPB_INLINE void ::upb_bufhandle_setbuf(upb_bufhandle *h,
                                                const char *buf, size_t ofs);
  friend UPB_INLINE const void* ::upb_bufhandle_obj(const upb_bufhandle *h);
  friend UPB_INLINE const void* ::upb_bufhandle_objtype(
      const upb_bufhandle *h);
  friend UPB_INLINE const char* ::upb_bufhandle_buf(const upb_bufhandle *h);
#else
struct upb_bufhandle {
#endif
  const char *buf_;
  const void *obj_;
  const void *objtype_;
  size_t objofs_;
};

#ifdef __cplusplus

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
class upb::Handlers {
 public:
  typedef upb_selector_t Selector;
  typedef upb_handlertype_t Type;

  typedef Handler<void *(*)(void *, const void *)> StartFieldHandler;
  typedef Handler<bool (*)(void *, const void *)> EndFieldHandler;
  typedef Handler<bool (*)(void *, const void *)> StartMessageHandler;
  typedef Handler<bool (*)(void *, const void *, Status*)> EndMessageHandler;
  typedef Handler<void *(*)(void *, const void *, size_t)> StartStringHandler;
  typedef Handler<size_t (*)(void *, const void *, const char *, size_t,
                             const BufferHandle *)> StringHandler;

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

  /* Returns a new handlers object for the given frozen msgdef.
   * Returns NULL if memory allocation failed. */
  static reffed_ptr<Handlers> New(const MessageDef *m);

  /* Convenience function for registering a graph of handlers that mirrors the
   * graph of msgdefs for some message.  For "m" and all its children a new set
   * of handlers will be created and the given callback will be invoked,
   * allowing the client to register handlers for this message.  Note that any
   * subhandlers set by the callback will be overwritten. */
  static reffed_ptr<const Handlers> NewFrozen(const MessageDef *m,
                                              HandlersCallback *callback,
                                              const void *closure);

  /* Functionality from upb::RefCounted. */
  UPB_REFCOUNTED_CPPMETHODS

  /* All handler registration functions return bool to indicate success or
   * failure; details about failures are stored in this status object.  If a
   * failure does occur, it must be cleared before the Handlers are frozen,
   * otherwise the freeze() operation will fail.  The functions may *only* be
   * used while the Handlers are mutable. */
  const Status* status();
  void ClearError();

  /* Call to freeze these Handlers.  Requires that any SubHandlers are already
   * frozen.  For cycles, you must use the static version below and freeze the
   * whole graph at once. */
  bool Freeze(Status* s);

  /* Freezes the given set of handlers.  You may not freeze a handler without
   * also freezing any handlers they point to. */
  static bool Freeze(Handlers*const* handlers, int n, Status* s);
  static bool Freeze(const std::vector<Handlers*>& handlers, Status* s);

  /* Returns the msgdef associated with this handlers object. */
  const MessageDef* message_def() const;

  /* Adds the given pointer and function to the list of cleanup functions that
   * will be run when these handlers are freed.  If this pointer has previously
   * been registered, the function returns false and does nothing. */
  bool AddCleanup(void *ptr, upb_handlerfree *cleanup);

  /* Sets the startmsg handler for the message, which is defined as follows:
   *
   *   bool startmsg(MyType* closure) {
   *     // Called when the message begins.  Returns true if processing should
   *     // continue.
   *     return true;
   *   }
   */
  bool SetStartMessageHandler(const StartMessageHandler& handler);

  /* Sets the endmsg handler for the message, which is defined as follows:
   *
   *   bool endmsg(MyType* closure, upb_status *status) {
   *     // Called when processing of this message ends, whether in success or
   *     // failure.  "status" indicates the final status of processing, and
   *     // can also be modified in-place to update the final status.
   *   }
   */
  bool SetEndMessageHandler(const EndMessageHandler& handler);

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
  bool SetInt32Handler (const FieldDef* f,  const Int32Handler& h);
  bool SetInt64Handler (const FieldDef* f,  const Int64Handler& h);
  bool SetUInt32Handler(const FieldDef* f, const UInt32Handler& h);
  bool SetUInt64Handler(const FieldDef* f, const UInt64Handler& h);
  bool SetFloatHandler (const FieldDef* f,  const FloatHandler& h);
  bool SetDoubleHandler(const FieldDef* f, const DoubleHandler& h);
  bool SetBoolHandler  (const FieldDef* f,   const BoolHandler& h);

  /* Like the previous, but templated on the type on the value (ie. int32).
   * This is mostly useful to call from other templates.  To call this you must
   * specify the template parameter explicitly, ie:
   *   h->SetValueHandler<T>(f, UpbBind(MyHandler<T>, MyData)); */
  template <class T>
  bool SetValueHandler(
      const FieldDef *f,
      const typename ValueHandler<typename CanonicalType<T>::Type>::H& handler);

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
  bool SetStartStringHandler(const FieldDef* f, const StartStringHandler& h);
  bool SetStringHandler(const FieldDef* f, const StringHandler& h);
  bool SetEndStringHandler(const FieldDef* f, const EndFieldHandler& h);

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
  bool SetStartSequenceHandler(const FieldDef* f, const StartFieldHandler& h);

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
  bool SetStartSubMessageHandler(const FieldDef* f, const StartFieldHandler& h);

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
  bool SetEndSubMessageHandler(const FieldDef *f, const EndFieldHandler &h);

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
  bool SetEndSequenceHandler(const FieldDef* f, const EndFieldHandler& h);

  /* Sets or gets the object that specifies handlers for the given field, which
   * must be a submessage or group.  Returns NULL if no handlers are set. */
  bool SetSubHandlers(const FieldDef* f, const Handlers* sub);
  const Handlers* GetSubHandlers(const FieldDef* f) const;

  /* Equivalent to GetSubHandlers, but takes the STARTSUBMSG selector for the
   * field. */
  const Handlers* GetSubHandlers(Selector startsubmsg) const;

  /* A selector refers to a specific field handler in the Handlers object
   * (for example: the STARTSUBMSG handler for field "field15").
   * On success, returns true and stores the selector in "s".
   * If the FieldDef or Type are invalid, returns false.
   * The returned selector is ONLY valid for Handlers whose MessageDef
   * contains this FieldDef. */
  static bool GetSelector(const FieldDef* f, Type type, Selector* s);

  /* Given a START selector of any kind, returns the corresponding END selector. */
  static Selector GetEndSelector(Selector start_selector);

  /* Returns the function pointer for this handler.  It is the client's
   * responsibility to cast to the correct function type before calling it. */
  GenericFunction* GetHandler(Selector selector);

  /* Sets the given attributes to the attributes for this selector. */
  bool GetAttributes(Selector selector, HandlerAttributes* attr);

  /* Returns the handler data that was registered with this handler. */
  const void* GetHandlerData(Selector selector);

  /* Could add any of the following functions as-needed, with some minor
   * implementation changes:
   *
   * const FieldDef* GetFieldDef(Selector selector);
   * static bool IsSequence(Selector selector); */

 private:
  UPB_DISALLOW_POD_OPS(Handlers, upb::Handlers)

  friend UPB_INLINE GenericFunction *::upb_handlers_gethandler(
      const upb_handlers *h, upb_selector_t s);
  friend UPB_INLINE const void *::upb_handlers_gethandlerdata(
      const upb_handlers *h, upb_selector_t s);
#else
struct upb_handlers {
#endif
  upb_refcounted base;

  const upb_msgdef *msg;
  const upb_handlers **sub;
  const void *top_closure_type;
  upb_inttable cleanup_;
  upb_status status_;  /* Used only when mutable. */
  upb_handlers_tabent table[1];  /* Dynamically-sized field handler array. */
};

#ifdef __cplusplus

namespace upb {

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

#ifdef UPB_CXX11

/* In C++11, the "template" disambiguator can appear even outside templates,
 * so all calls can safely use this pair of macros. */

#define UpbMakeHandler(f) upb::MatchFunc(f).template GetFunc<f>()

/* We have to be careful to only evaluate "d" once. */
#define UpbBind(f, d) upb::MatchFunc(f).template GetFunc<f>((d))

#else

/* Prior to C++11, the "template" disambiguator may only appear inside a
 * template, so the regular macro must not use "template" */

#define UpbMakeHandler(f) upb::MatchFunc(f).GetFunc<f>()

#define UpbBind(f, d) upb::MatchFunc(f).GetFunc<f>((d))

#endif  /* UPB_CXX11 */

/* This macro must be used in C++98 for calls from inside a template.  But we
 * define this variant in all cases; code that wants to be compatible with both
 * C++98 and C++11 should always use this macro when calling from a template. */
#define UpbMakeHandlerT(f) upb::MatchFunc(f).template GetFunc<f>()

/* We have to be careful to only evaluate "d" once. */
#define UpbBindT(f, d) upb::MatchFunc(f).template GetFunc<f>((d))

/* Handler: a struct that contains the (handler, data, deleter) tuple that is
 * used to register all handlers.  Users can Make() these directly but it's
 * more convenient to use the UpbMakeHandler/UpbBind macros above. */
template <class T> class Handler {
 public:
  /* The underlying, handler function signature that upb uses internally. */
  typedef T FuncPtr;

  /* Intentionally implicit. */
  template <class F> Handler(F func);
  ~Handler();

 private:
  void AddCleanup(Handlers* h) const {
    if (cleanup_func_) {
      bool ok = h->AddCleanup(cleanup_data_, cleanup_func_);
      UPB_ASSERT_VAR(ok, ok);
    }
  }

  UPB_DISALLOW_COPY_AND_ASSIGN(Handler)
  friend class Handlers;
  FuncPtr handler_;
  mutable HandlerAttributes attr_;
  mutable bool registered_;
  void *cleanup_data_;
  upb_handlerfree *cleanup_func_;
};

}  /* namespace upb */

#endif  /* __cplusplus */

UPB_BEGIN_EXTERN_C

/* Native C API. */

/* Handler function typedefs. */
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

/* upb_bufhandle */
size_t upb_bufhandle_objofs(const upb_bufhandle *h);

/* upb_handlerattr */
void upb_handlerattr_init(upb_handlerattr *attr);
void upb_handlerattr_uninit(upb_handlerattr *attr);

bool upb_handlerattr_sethandlerdata(upb_handlerattr *attr, const void *hd);
bool upb_handlerattr_setclosuretype(upb_handlerattr *attr, const void *type);
const void *upb_handlerattr_closuretype(const upb_handlerattr *attr);
bool upb_handlerattr_setreturnclosuretype(upb_handlerattr *attr,
                                          const void *type);
const void *upb_handlerattr_returnclosuretype(const upb_handlerattr *attr);
bool upb_handlerattr_setalwaysok(upb_handlerattr *attr, bool alwaysok);
bool upb_handlerattr_alwaysok(const upb_handlerattr *attr);

UPB_INLINE const void *upb_handlerattr_handlerdata(
    const upb_handlerattr *attr) {
  return attr->handler_data_;
}

/* upb_handlers */
typedef void upb_handlers_callback(const void *closure, upb_handlers *h);
upb_handlers *upb_handlers_new(const upb_msgdef *m,
                               const void *owner);
const upb_handlers *upb_handlers_newfrozen(const upb_msgdef *m,
                                           const void *owner,
                                           upb_handlers_callback *callback,
                                           const void *closure);

/* Include refcounted methods like upb_handlers_ref(). */
UPB_REFCOUNTED_CMETHODS(upb_handlers, upb_handlers_upcast)

const upb_status *upb_handlers_status(upb_handlers *h);
void upb_handlers_clearerr(upb_handlers *h);
const upb_msgdef *upb_handlers_msgdef(const upb_handlers *h);
bool upb_handlers_addcleanup(upb_handlers *h, void *p, upb_handlerfree *hfree);

bool upb_handlers_setstartmsg(upb_handlers *h, upb_startmsg_handlerfunc *func,
                              upb_handlerattr *attr);
bool upb_handlers_setendmsg(upb_handlers *h, upb_endmsg_handlerfunc *func,
                            upb_handlerattr *attr);
bool upb_handlers_setint32(upb_handlers *h, const upb_fielddef *f,
                           upb_int32_handlerfunc *func, upb_handlerattr *attr);
bool upb_handlers_setint64(upb_handlers *h, const upb_fielddef *f,
                           upb_int64_handlerfunc *func, upb_handlerattr *attr);
bool upb_handlers_setuint32(upb_handlers *h, const upb_fielddef *f,
                            upb_uint32_handlerfunc *func,
                            upb_handlerattr *attr);
bool upb_handlers_setuint64(upb_handlers *h, const upb_fielddef *f,
                            upb_uint64_handlerfunc *func,
                            upb_handlerattr *attr);
bool upb_handlers_setfloat(upb_handlers *h, const upb_fielddef *f,
                           upb_float_handlerfunc *func, upb_handlerattr *attr);
bool upb_handlers_setdouble(upb_handlers *h, const upb_fielddef *f,
                            upb_double_handlerfunc *func,
                            upb_handlerattr *attr);
bool upb_handlers_setbool(upb_handlers *h, const upb_fielddef *f,
                          upb_bool_handlerfunc *func,
                          upb_handlerattr *attr);
bool upb_handlers_setstartstr(upb_handlers *h, const upb_fielddef *f,
                              upb_startstr_handlerfunc *func,
                              upb_handlerattr *attr);
bool upb_handlers_setstring(upb_handlers *h, const upb_fielddef *f,
                            upb_string_handlerfunc *func,
                            upb_handlerattr *attr);
bool upb_handlers_setendstr(upb_handlers *h, const upb_fielddef *f,
                            upb_endfield_handlerfunc *func,
                            upb_handlerattr *attr);
bool upb_handlers_setstartseq(upb_handlers *h, const upb_fielddef *f,
                              upb_startfield_handlerfunc *func,
                              upb_handlerattr *attr);
bool upb_handlers_setstartsubmsg(upb_handlers *h, const upb_fielddef *f,
                                 upb_startfield_handlerfunc *func,
                                 upb_handlerattr *attr);
bool upb_handlers_setendsubmsg(upb_handlers *h, const upb_fielddef *f,
                               upb_endfield_handlerfunc *func,
                               upb_handlerattr *attr);
bool upb_handlers_setendseq(upb_handlers *h, const upb_fielddef *f,
                            upb_endfield_handlerfunc *func,
                            upb_handlerattr *attr);

bool upb_handlers_setsubhandlers(upb_handlers *h, const upb_fielddef *f,
                                 const upb_handlers *sub);
const upb_handlers *upb_handlers_getsubhandlers(const upb_handlers *h,
                                                const upb_fielddef *f);
const upb_handlers *upb_handlers_getsubhandlers_sel(const upb_handlers *h,
                                                    upb_selector_t sel);

UPB_INLINE upb_func *upb_handlers_gethandler(const upb_handlers *h,
                                             upb_selector_t s) {
  return (upb_func *)h->table[s].func;
}

bool upb_handlers_getattr(const upb_handlers *h, upb_selector_t s,
                          upb_handlerattr *attr);

UPB_INLINE const void *upb_handlers_gethandlerdata(const upb_handlers *h,
                                                   upb_selector_t s) {
  return upb_handlerattr_handlerdata(&h->table[s].attr);
}

#ifdef __cplusplus

/* Handler types for single fields.
 * Right now we only have one for TYPE_BYTES but ones for other types
 * should follow.
 *
 * These follow the same handlers protocol for fields of a message. */
class upb::BytesHandler {
 public:
  BytesHandler();
  ~BytesHandler();
#else
struct upb_byteshandler {
#endif
  upb_handlers_tabent table[3];
};

void upb_byteshandler_init(upb_byteshandler *h);

/* Caller must ensure that "d" outlives the handlers.
 * TODO(haberman): should this have a "freeze" operation?  It's not necessary
 * for memory management, but could be useful to force immutability and provide
 * a convenient moment to verify that all registration succeeded. */
bool upb_byteshandler_setstartstr(upb_byteshandler *h,
                                  upb_startstr_handlerfunc *func, void *d);
bool upb_byteshandler_setstring(upb_byteshandler *h,
                                upb_string_handlerfunc *func, void *d);
bool upb_byteshandler_setendstr(upb_byteshandler *h,
                                upb_endfield_handlerfunc *func, void *d);

/* "Static" methods */
bool upb_handlers_freeze(upb_handlers *const *handlers, int n, upb_status *s);
upb_handlertype_t upb_handlers_getprimitivehandlertype(const upb_fielddef *f);
bool upb_handlers_getselector(const upb_fielddef *f, upb_handlertype_t type,
                              upb_selector_t *s);
UPB_INLINE upb_selector_t upb_handlers_getendselector(upb_selector_t start) {
  return start + 1;
}

/* Internal-only. */
uint32_t upb_handlers_selectorbaseoffset(const upb_fielddef *f);
uint32_t upb_handlers_selectorcount(const upb_fielddef *f);

UPB_END_EXTERN_C

/*
** Inline definitions for handlers.h, which are particularly long and a bit
** tricky.
*/

#ifndef UPB_HANDLERS_INL_H_
#define UPB_HANDLERS_INL_H_

#include <limits.h>

/* C inline methods. */

/* upb_bufhandle */
UPB_INLINE void upb_bufhandle_init(upb_bufhandle *h) {
  h->obj_ = NULL;
  h->objtype_ = NULL;
  h->buf_ = NULL;
  h->objofs_ = 0;
}
UPB_INLINE void upb_bufhandle_uninit(upb_bufhandle *h) {
  UPB_UNUSED(h);
}
UPB_INLINE void upb_bufhandle_setobj(upb_bufhandle *h, const void *obj,
                                     const void *type) {
  h->obj_ = obj;
  h->objtype_ = type;
}
UPB_INLINE void upb_bufhandle_setbuf(upb_bufhandle *h, const char *buf,
                                     size_t ofs) {
  h->buf_ = buf;
  h->objofs_ = ofs;
}
UPB_INLINE const void *upb_bufhandle_obj(const upb_bufhandle *h) {
  return h->obj_;
}
UPB_INLINE const void *upb_bufhandle_objtype(const upb_bufhandle *h) {
  return h->objtype_;
}
UPB_INLINE const char *upb_bufhandle_buf(const upb_bufhandle *h) {
  return h->buf_;
}


#ifdef __cplusplus

/* Type detection and typedefs for integer types.
 * For platforms where there are multiple 32-bit or 64-bit types, we need to be
 * able to enumerate them so we can properly create overloads for all variants.
 *
 * If any platform existed where there were three integer types with the same
 * size, this would have to become more complicated.  For example, short, int,
 * and long could all be 32-bits.  Even more diabolically, short, int, long,
 * and long long could all be 64 bits and still be standard-compliant.
 * However, few platforms are this strange, and it's unlikely that upb will be
 * used on the strangest ones. */

/* Can't count on stdint.h limits like INT32_MAX, because in C++ these are
 * only defined when __STDC_LIMIT_MACROS are defined before the *first* include
 * of stdint.h.  We can't guarantee that someone else didn't include these first
 * without defining __STDC_LIMIT_MACROS. */
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

/* We use macros instead of typedefs so we can undefine them later and avoid
 * leaking them outside this header file. */
#if UPB_INT_IS_32BITS
#define UPB_INT32_T int
#define UPB_UINT32_T unsigned int

#if UPB_LONG_IS_32BITS
#define UPB_TWO_32BIT_TYPES 1
#define UPB_INT32ALT_T long
#define UPB_UINT32ALT_T unsigned long
#endif  /* UPB_LONG_IS_32BITS */

#elif UPB_LONG_IS_32BITS  /* && !UPB_INT_IS_32BITS */
#define UPB_INT32_T long
#define UPB_UINT32_T unsigned long
#endif  /* UPB_INT_IS_32BITS */


#if UPB_LONG_IS_64BITS
#define UPB_INT64_T long
#define UPB_UINT64_T unsigned long

#if UPB_LLONG_IS_64BITS
#define UPB_TWO_64BIT_TYPES 1
#define UPB_INT64ALT_T long long
#define UPB_UINT64ALT_T unsigned long long
#endif  /* UPB_LLONG_IS_64BITS */

#elif UPB_LLONG_IS_64BITS  /* && !UPB_LONG_IS_64BITS */
#define UPB_INT64_T long long
#define UPB_UINT64_T unsigned long long
#endif  /* UPB_LONG_IS_64BITS */

#undef UPB_INT32_MAX
#undef UPB_INT32_MIN
#undef UPB_INT64_MAX
#undef UPB_INT64_MIN
#undef UPB_INT_IS_32BITS
#undef UPB_LONG_IS_32BITS
#undef UPB_LONG_IS_64BITS
#undef UPB_LLONG_IS_64BITS


namespace upb {

typedef void CleanupFunc(void *ptr);

/* Template to remove "const" from "const T*" and just return "T*".
 *
 * We define a nonsense default because otherwise it will fail to instantiate as
 * a function parameter type even in cases where we don't expect any caller to
 * actually match the overload. */
class CouldntRemoveConst {};
template <class T> struct remove_constptr { typedef CouldntRemoveConst type; };
template <class T> struct remove_constptr<const T *> { typedef T *type; };

/* Template that we use below to remove a template specialization from
 * consideration if it matches a specific type. */
template <class T, class U> struct disable_if_same { typedef void Type; };
template <class T> struct disable_if_same<T, T> {};

template <class T> void DeletePointer(void *p) { delete static_cast<T>(p); }

template <class T1, class T2>
struct FirstUnlessVoidOrBool {
  typedef T1 value;
};

template <class T2>
struct FirstUnlessVoidOrBool<void, T2> {
  typedef T2 value;
};

template <class T2>
struct FirstUnlessVoidOrBool<bool, T2> {
  typedef T2 value;
};

template<class T, class U>
struct is_same {
  static bool value;
};

template<class T>
struct is_same<T, T> {
  static bool value;
};

template<class T, class U>
bool is_same<T, U>::value = false;

template<class T>
bool is_same<T, T>::value = true;

/* FuncInfo *******************************************************************/

/* Info about the user's original, pre-wrapped function. */
template <class C, class R = void>
struct FuncInfo {
  /* The type of the closure that the function takes (its first param). */
  typedef C Closure;

  /* The return type. */
  typedef R Return;
};

/* Func ***********************************************************************/

/* Func1, Func2, Func3: Template classes representing a function and its
 * signature.
 *
 * Since the function is a template parameter, calling the function can be
 * inlined at compile-time and does not require a function pointer at runtime.
 * These functions are not bound to a handler data so have no data or cleanup
 * handler. */
struct UnboundFunc {
  CleanupFunc *GetCleanup() { return NULL; }
  void *GetData() { return NULL; }
};

template <class R, class P1, R F(P1), class I>
struct Func1 : public UnboundFunc {
  typedef R Return;
  typedef I FuncInfo;
  static R Call(P1 p1) { return F(p1); }
};

template <class R, class P1, class P2, R F(P1, P2), class I>
struct Func2 : public UnboundFunc {
  typedef R Return;
  typedef I FuncInfo;
  static R Call(P1 p1, P2 p2) { return F(p1, p2); }
};

template <class R, class P1, class P2, class P3, R F(P1, P2, P3), class I>
struct Func3 : public UnboundFunc {
  typedef R Return;
  typedef I FuncInfo;
  static R Call(P1 p1, P2 p2, P3 p3) { return F(p1, p2, p3); }
};

template <class R, class P1, class P2, class P3, class P4, R F(P1, P2, P3, P4),
          class I>
struct Func4 : public UnboundFunc {
  typedef R Return;
  typedef I FuncInfo;
  static R Call(P1 p1, P2 p2, P3 p3, P4 p4) { return F(p1, p2, p3, p4); }
};

template <class R, class P1, class P2, class P3, class P4, class P5,
          R F(P1, P2, P3, P4, P5), class I>
struct Func5 : public UnboundFunc {
  typedef R Return;
  typedef I FuncInfo;
  static R Call(P1 p1, P2 p2, P3 p3, P4 p4, P5 p5) {
    return F(p1, p2, p3, p4, p5);
  }
};

/* BoundFunc ******************************************************************/

/* BoundFunc2, BoundFunc3: Like Func2/Func3 except also contains a value that
 * shall be bound to the function's second parameter.
 * 
 * Note that the second parameter is a const pointer, but our stored bound value
 * is non-const so we can free it when the handlers are destroyed. */
template <class T>
struct BoundFunc {
  typedef typename remove_constptr<T>::type MutableP2;
  explicit BoundFunc(MutableP2 data_) : data(data_) {}
  CleanupFunc *GetCleanup() { return &DeletePointer<MutableP2>; }
  MutableP2 GetData() { return data; }
  MutableP2 data;
};

template <class R, class P1, class P2, R F(P1, P2), class I>
struct BoundFunc2 : public BoundFunc<P2> {
  typedef BoundFunc<P2> Base;
  typedef I FuncInfo;
  explicit BoundFunc2(typename Base::MutableP2 arg) : Base(arg) {}
};

template <class R, class P1, class P2, class P3, R F(P1, P2, P3), class I>
struct BoundFunc3 : public BoundFunc<P2> {
  typedef BoundFunc<P2> Base;
  typedef I FuncInfo;
  explicit BoundFunc3(typename Base::MutableP2 arg) : Base(arg) {}
};

template <class R, class P1, class P2, class P3, class P4, R F(P1, P2, P3, P4),
          class I>
struct BoundFunc4 : public BoundFunc<P2> {
  typedef BoundFunc<P2> Base;
  typedef I FuncInfo;
  explicit BoundFunc4(typename Base::MutableP2 arg) : Base(arg) {}
};

template <class R, class P1, class P2, class P3, class P4, class P5,
          R F(P1, P2, P3, P4, P5), class I>
struct BoundFunc5 : public BoundFunc<P2> {
  typedef BoundFunc<P2> Base;
  typedef I FuncInfo;
  explicit BoundFunc5(typename Base::MutableP2 arg) : Base(arg) {}
};

/* FuncSig ********************************************************************/

/* FuncSig1, FuncSig2, FuncSig3: template classes reflecting a function
 * *signature*, but without a specific function attached.
 *
 * These classes contain member functions that can be invoked with a
 * specific function to return a Func/BoundFunc class. */
template <class R, class P1>
struct FuncSig1 {
  template <R F(P1)>
  Func1<R, P1, F, FuncInfo<P1, R> > GetFunc() {
    return Func1<R, P1, F, FuncInfo<P1, R> >();
  }
};

template <class R, class P1, class P2>
struct FuncSig2 {
  template <R F(P1, P2)>
  Func2<R, P1, P2, F, FuncInfo<P1, R> > GetFunc() {
    return Func2<R, P1, P2, F, FuncInfo<P1, R> >();
  }

  template <R F(P1, P2)>
  BoundFunc2<R, P1, P2, F, FuncInfo<P1, R> > GetFunc(
      typename remove_constptr<P2>::type param2) {
    return BoundFunc2<R, P1, P2, F, FuncInfo<P1, R> >(param2);
  }
};

template <class R, class P1, class P2, class P3>
struct FuncSig3 {
  template <R F(P1, P2, P3)>
  Func3<R, P1, P2, P3, F, FuncInfo<P1, R> > GetFunc() {
    return Func3<R, P1, P2, P3, F, FuncInfo<P1, R> >();
  }

  template <R F(P1, P2, P3)>
  BoundFunc3<R, P1, P2, P3, F, FuncInfo<P1, R> > GetFunc(
      typename remove_constptr<P2>::type param2) {
    return BoundFunc3<R, P1, P2, P3, F, FuncInfo<P1, R> >(param2);
  }
};

template <class R, class P1, class P2, class P3, class P4>
struct FuncSig4 {
  template <R F(P1, P2, P3, P4)>
  Func4<R, P1, P2, P3, P4, F, FuncInfo<P1, R> > GetFunc() {
    return Func4<R, P1, P2, P3, P4, F, FuncInfo<P1, R> >();
  }

  template <R F(P1, P2, P3, P4)>
  BoundFunc4<R, P1, P2, P3, P4, F, FuncInfo<P1, R> > GetFunc(
      typename remove_constptr<P2>::type param2) {
    return BoundFunc4<R, P1, P2, P3, P4, F, FuncInfo<P1, R> >(param2);
  }
};

template <class R, class P1, class P2, class P3, class P4, class P5>
struct FuncSig5 {
  template <R F(P1, P2, P3, P4, P5)>
  Func5<R, P1, P2, P3, P4, P5, F, FuncInfo<P1, R> > GetFunc() {
    return Func5<R, P1, P2, P3, P4, P5, F, FuncInfo<P1, R> >();
  }

  template <R F(P1, P2, P3, P4, P5)>
  BoundFunc5<R, P1, P2, P3, P4, P5, F, FuncInfo<P1, R> > GetFunc(
      typename remove_constptr<P2>::type param2) {
    return BoundFunc5<R, P1, P2, P3, P4, P5, F, FuncInfo<P1, R> >(param2);
  }
};

/* Overloaded template function that can construct the appropriate FuncSig*
 * class given a function pointer by deducing the template parameters. */
template <class R, class P1>
inline FuncSig1<R, P1> MatchFunc(R (*f)(P1)) {
  UPB_UNUSED(f);  /* Only used for template parameter deduction. */
  return FuncSig1<R, P1>();
}

template <class R, class P1, class P2>
inline FuncSig2<R, P1, P2> MatchFunc(R (*f)(P1, P2)) {
  UPB_UNUSED(f);  /* Only used for template parameter deduction. */
  return FuncSig2<R, P1, P2>();
}

template <class R, class P1, class P2, class P3>
inline FuncSig3<R, P1, P2, P3> MatchFunc(R (*f)(P1, P2, P3)) {
  UPB_UNUSED(f);  /* Only used for template parameter deduction. */
  return FuncSig3<R, P1, P2, P3>();
}

template <class R, class P1, class P2, class P3, class P4>
inline FuncSig4<R, P1, P2, P3, P4> MatchFunc(R (*f)(P1, P2, P3, P4)) {
  UPB_UNUSED(f);  /* Only used for template parameter deduction. */
  return FuncSig4<R, P1, P2, P3, P4>();
}

template <class R, class P1, class P2, class P3, class P4, class P5>
inline FuncSig5<R, P1, P2, P3, P4, P5> MatchFunc(R (*f)(P1, P2, P3, P4, P5)) {
  UPB_UNUSED(f);  /* Only used for template parameter deduction. */
  return FuncSig5<R, P1, P2, P3, P4, P5>();
}

/* MethodSig ******************************************************************/

/* CallMethod*: a function template that calls a given method. */
template <class R, class C, R (C::*F)()>
R CallMethod0(C *obj) {
  return ((*obj).*F)();
}

template <class R, class C, class P1, R (C::*F)(P1)>
R CallMethod1(C *obj, P1 arg1) {
  return ((*obj).*F)(arg1);
}

template <class R, class C, class P1, class P2, R (C::*F)(P1, P2)>
R CallMethod2(C *obj, P1 arg1, P2 arg2) {
  return ((*obj).*F)(arg1, arg2);
}

template <class R, class C, class P1, class P2, class P3, R (C::*F)(P1, P2, P3)>
R CallMethod3(C *obj, P1 arg1, P2 arg2, P3 arg3) {
  return ((*obj).*F)(arg1, arg2, arg3);
}

template <class R, class C, class P1, class P2, class P3, class P4,
          R (C::*F)(P1, P2, P3, P4)>
R CallMethod4(C *obj, P1 arg1, P2 arg2, P3 arg3, P4 arg4) {
  return ((*obj).*F)(arg1, arg2, arg3, arg4);
}

/* MethodSig: like FuncSig, but for member functions.
 *
 * GetFunc() returns a normal FuncN object, so after calling GetFunc() no
 * more logic is required to special-case methods. */
template <class R, class C>
struct MethodSig0 {
  template <R (C::*F)()>
  Func1<R, C *, CallMethod0<R, C, F>, FuncInfo<C *, R> > GetFunc() {
    return Func1<R, C *, CallMethod0<R, C, F>, FuncInfo<C *, R> >();
  }
};

template <class R, class C, class P1>
struct MethodSig1 {
  template <R (C::*F)(P1)>
  Func2<R, C *, P1, CallMethod1<R, C, P1, F>, FuncInfo<C *, R> > GetFunc() {
    return Func2<R, C *, P1, CallMethod1<R, C, P1, F>, FuncInfo<C *, R> >();
  }

  template <R (C::*F)(P1)>
  BoundFunc2<R, C *, P1, CallMethod1<R, C, P1, F>, FuncInfo<C *, R> > GetFunc(
      typename remove_constptr<P1>::type param1) {
    return BoundFunc2<R, C *, P1, CallMethod1<R, C, P1, F>, FuncInfo<C *, R> >(
        param1);
  }
};

template <class R, class C, class P1, class P2>
struct MethodSig2 {
  template <R (C::*F)(P1, P2)>
  Func3<R, C *, P1, P2, CallMethod2<R, C, P1, P2, F>, FuncInfo<C *, R> >
  GetFunc() {
    return Func3<R, C *, P1, P2, CallMethod2<R, C, P1, P2, F>,
                 FuncInfo<C *, R> >();
  }

  template <R (C::*F)(P1, P2)>
  BoundFunc3<R, C *, P1, P2, CallMethod2<R, C, P1, P2, F>, FuncInfo<C *, R> >
  GetFunc(typename remove_constptr<P1>::type param1) {
    return BoundFunc3<R, C *, P1, P2, CallMethod2<R, C, P1, P2, F>,
                      FuncInfo<C *, R> >(param1);
  }
};

template <class R, class C, class P1, class P2, class P3>
struct MethodSig3 {
  template <R (C::*F)(P1, P2, P3)>
  Func4<R, C *, P1, P2, P3, CallMethod3<R, C, P1, P2, P3, F>, FuncInfo<C *, R> >
  GetFunc() {
    return Func4<R, C *, P1, P2, P3, CallMethod3<R, C, P1, P2, P3, F>,
                 FuncInfo<C *, R> >();
  }

  template <R (C::*F)(P1, P2, P3)>
  BoundFunc4<R, C *, P1, P2, P3, CallMethod3<R, C, P1, P2, P3, F>,
             FuncInfo<C *, R> >
  GetFunc(typename remove_constptr<P1>::type param1) {
    return BoundFunc4<R, C *, P1, P2, P3, CallMethod3<R, C, P1, P2, P3, F>,
                      FuncInfo<C *, R> >(param1);
  }
};

template <class R, class C, class P1, class P2, class P3, class P4>
struct MethodSig4 {
  template <R (C::*F)(P1, P2, P3, P4)>
  Func5<R, C *, P1, P2, P3, P4, CallMethod4<R, C, P1, P2, P3, P4, F>,
        FuncInfo<C *, R> >
  GetFunc() {
    return Func5<R, C *, P1, P2, P3, P4, CallMethod4<R, C, P1, P2, P3, P4, F>,
                 FuncInfo<C *, R> >();
  }

  template <R (C::*F)(P1, P2, P3, P4)>
  BoundFunc5<R, C *, P1, P2, P3, P4, CallMethod4<R, C, P1, P2, P3, P4, F>,
             FuncInfo<C *, R> >
  GetFunc(typename remove_constptr<P1>::type param1) {
    return BoundFunc5<R, C *, P1, P2, P3, P4,
                      CallMethod4<R, C, P1, P2, P3, P4, F>, FuncInfo<C *, R> >(
        param1);
  }
};

template <class R, class C>
inline MethodSig0<R, C> MatchFunc(R (C::*f)()) {
  UPB_UNUSED(f);  /* Only used for template parameter deduction. */
  return MethodSig0<R, C>();
}

template <class R, class C, class P1>
inline MethodSig1<R, C, P1> MatchFunc(R (C::*f)(P1)) {
  UPB_UNUSED(f);  /* Only used for template parameter deduction. */
  return MethodSig1<R, C, P1>();
}

template <class R, class C, class P1, class P2>
inline MethodSig2<R, C, P1, P2> MatchFunc(R (C::*f)(P1, P2)) {
  UPB_UNUSED(f);  /* Only used for template parameter deduction. */
  return MethodSig2<R, C, P1, P2>();
}

template <class R, class C, class P1, class P2, class P3>
inline MethodSig3<R, C, P1, P2, P3> MatchFunc(R (C::*f)(P1, P2, P3)) {
  UPB_UNUSED(f);  /* Only used for template parameter deduction. */
  return MethodSig3<R, C, P1, P2, P3>();
}

template <class R, class C, class P1, class P2, class P3, class P4>
inline MethodSig4<R, C, P1, P2, P3, P4> MatchFunc(R (C::*f)(P1, P2, P3, P4)) {
  UPB_UNUSED(f);  /* Only used for template parameter deduction. */
  return MethodSig4<R, C, P1, P2, P3, P4>();
}

/* MaybeWrapReturn ************************************************************/

/* Template class that attempts to wrap the return value of the function so it
 * matches the expected type.  There are two main adjustments it may make:
 *
 *   1. If the function returns void, make it return the expected type and with
 *      a value that always indicates success.
 *   2. If the function returns bool, make it return the expected type with a
 *      value that indicates success or failure.
 *
 * The "expected type" for return is:
 *   1. void* for start handlers.  If the closure parameter has a different type
 *      we will cast it to void* for the return in the success case.
 *   2. size_t for string buffer handlers.
 *   3. bool for everything else. */

/* Template parameters are FuncN type and desired return type. */
template <class F, class R, class Enable = void>
struct MaybeWrapReturn;

/* If the return type matches, return the given function unwrapped. */
template <class F>
struct MaybeWrapReturn<F, typename F::Return> {
  typedef F Func;
};

/* Function wrapper that munges the return value from void to (bool)true. */
template <class P1, class P2, void F(P1, P2)>
bool ReturnTrue2(P1 p1, P2 p2) {
  F(p1, p2);
  return true;
}

template <class P1, class P2, class P3, void F(P1, P2, P3)>
bool ReturnTrue3(P1 p1, P2 p2, P3 p3) {
  F(p1, p2, p3);
  return true;
}

/* Function wrapper that munges the return value from void to (void*)arg1  */
template <class P1, class P2, void F(P1, P2)>
void *ReturnClosure2(P1 p1, P2 p2) {
  F(p1, p2);
  return p1;
}

template <class P1, class P2, class P3, void F(P1, P2, P3)>
void *ReturnClosure3(P1 p1, P2 p2, P3 p3) {
  F(p1, p2, p3);
  return p1;
}

/* Function wrapper that munges the return value from R to void*. */
template <class R, class P1, class P2, R F(P1, P2)>
void *CastReturnToVoidPtr2(P1 p1, P2 p2) {
  return F(p1, p2);
}

template <class R, class P1, class P2, class P3, R F(P1, P2, P3)>
void *CastReturnToVoidPtr3(P1 p1, P2 p2, P3 p3) {
  return F(p1, p2, p3);
}

/* Function wrapper that munges the return value from bool to void*. */
template <class P1, class P2, bool F(P1, P2)>
void *ReturnClosureOrBreak2(P1 p1, P2 p2) {
  return F(p1, p2) ? p1 : UPB_BREAK;
}

template <class P1, class P2, class P3, bool F(P1, P2, P3)>
void *ReturnClosureOrBreak3(P1 p1, P2 p2, P3 p3) {
  return F(p1, p2, p3) ? p1 : UPB_BREAK;
}

/* For the string callback, which takes five params, returns the size param. */
template <class P1, class P2,
          void F(P1, P2, const char *, size_t, const BufferHandle *)>
size_t ReturnStringLen(P1 p1, P2 p2, const char *p3, size_t p4,
                       const BufferHandle *p5) {
  F(p1, p2, p3, p4, p5);
  return p4;
}

/* For the string callback, which takes five params, returns the size param or
 * zero. */
template <class P1, class P2,
          bool F(P1, P2, const char *, size_t, const BufferHandle *)>
size_t ReturnNOr0(P1 p1, P2 p2, const char *p3, size_t p4,
                  const BufferHandle *p5) {
  return F(p1, p2, p3, p4, p5) ? p4 : 0;
}

/* If we have a function returning void but want a function returning bool, wrap
 * it in a function that returns true. */
template <class P1, class P2, void F(P1, P2), class I>
struct MaybeWrapReturn<Func2<void, P1, P2, F, I>, bool> {
  typedef Func2<bool, P1, P2, ReturnTrue2<P1, P2, F>, I> Func;
};

template <class P1, class P2, class P3, void F(P1, P2, P3), class I>
struct MaybeWrapReturn<Func3<void, P1, P2, P3, F, I>, bool> {
  typedef Func3<bool, P1, P2, P3, ReturnTrue3<P1, P2, P3, F>, I> Func;
};

/* If our function returns void but we want one returning void*, wrap it in a
 * function that returns the first argument. */
template <class P1, class P2, void F(P1, P2), class I>
struct MaybeWrapReturn<Func2<void, P1, P2, F, I>, void *> {
  typedef Func2<void *, P1, P2, ReturnClosure2<P1, P2, F>, I> Func;
};

template <class P1, class P2, class P3, void F(P1, P2, P3), class I>
struct MaybeWrapReturn<Func3<void, P1, P2, P3, F, I>, void *> {
  typedef Func3<void *, P1, P2, P3, ReturnClosure3<P1, P2, P3, F>, I> Func;
};

/* If our function returns R* but we want one returning void*, wrap it in a
 * function that casts to void*. */
template <class R, class P1, class P2, R *F(P1, P2), class I>
struct MaybeWrapReturn<Func2<R *, P1, P2, F, I>, void *,
                       typename disable_if_same<R *, void *>::Type> {
  typedef Func2<void *, P1, P2, CastReturnToVoidPtr2<R *, P1, P2, F>, I> Func;
};

template <class R, class P1, class P2, class P3, R *F(P1, P2, P3), class I>
struct MaybeWrapReturn<Func3<R *, P1, P2, P3, F, I>, void *,
                       typename disable_if_same<R *, void *>::Type> {
  typedef Func3<void *, P1, P2, P3, CastReturnToVoidPtr3<R *, P1, P2, P3, F>, I>
      Func;
};

/* If our function returns bool but we want one returning void*, wrap it in a
 * function that returns either the first param or UPB_BREAK. */
template <class P1, class P2, bool F(P1, P2), class I>
struct MaybeWrapReturn<Func2<bool, P1, P2, F, I>, void *> {
  typedef Func2<void *, P1, P2, ReturnClosureOrBreak2<P1, P2, F>, I> Func;
};

template <class P1, class P2, class P3, bool F(P1, P2, P3), class I>
struct MaybeWrapReturn<Func3<bool, P1, P2, P3, F, I>, void *> {
  typedef Func3<void *, P1, P2, P3, ReturnClosureOrBreak3<P1, P2, P3, F>, I>
      Func;
};

/* If our function returns void but we want one returning size_t, wrap it in a
 * function that returns the size argument. */
template <class P1, class P2,
          void F(P1, P2, const char *, size_t, const BufferHandle *), class I>
struct MaybeWrapReturn<
    Func5<void, P1, P2, const char *, size_t, const BufferHandle *, F, I>,
          size_t> {
  typedef Func5<size_t, P1, P2, const char *, size_t, const BufferHandle *,
                ReturnStringLen<P1, P2, F>, I> Func;
};

/* If our function returns bool but we want one returning size_t, wrap it in a
 * function that returns either 0 or the buf size. */
template <class P1, class P2,
          bool F(P1, P2, const char *, size_t, const BufferHandle *), class I>
struct MaybeWrapReturn<
    Func5<bool, P1, P2, const char *, size_t, const BufferHandle *, F, I>,
    size_t> {
  typedef Func5<size_t, P1, P2, const char *, size_t, const BufferHandle *,
                ReturnNOr0<P1, P2, F>, I> Func;
};

/* ConvertParams **************************************************************/

/* Template class that converts the function parameters if necessary, and
 * ignores the HandlerData parameter if appropriate.
 *
 * Template parameter is the are FuncN function type. */
template <class F, class T>
struct ConvertParams;

/* Function that discards the handler data parameter. */
template <class R, class P1, R F(P1)>
R IgnoreHandlerData2(void *p1, const void *hd) {
  UPB_UNUSED(hd);
  return F(static_cast<P1>(p1));
}

template <class R, class P1, class P2Wrapper, class P2Wrapped,
          R F(P1, P2Wrapped)>
R IgnoreHandlerData3(void *p1, const void *hd, P2Wrapper p2) {
  UPB_UNUSED(hd);
  return F(static_cast<P1>(p1), p2);
}

template <class R, class P1, class P2, class P3, R F(P1, P2, P3)>
R IgnoreHandlerData4(void *p1, const void *hd, P2 p2, P3 p3) {
  UPB_UNUSED(hd);
  return F(static_cast<P1>(p1), p2, p3);
}

template <class R, class P1, class P2, class P3, class P4, R F(P1, P2, P3, P4)>
R IgnoreHandlerData5(void *p1, const void *hd, P2 p2, P3 p3, P4 p4) {
  UPB_UNUSED(hd);
  return F(static_cast<P1>(p1), p2, p3, p4);
}

template <class R, class P1, R F(P1, const char*, size_t)>
R IgnoreHandlerDataIgnoreHandle(void *p1, const void *hd, const char *p2,
                                size_t p3, const BufferHandle *handle) {
  UPB_UNUSED(hd);
  UPB_UNUSED(handle);
  return F(static_cast<P1>(p1), p2, p3);
}

/* Function that casts the handler data parameter. */
template <class R, class P1, class P2, R F(P1, P2)>
R CastHandlerData2(void *c, const void *hd) {
  return F(static_cast<P1>(c), static_cast<P2>(hd));
}

template <class R, class P1, class P2, class P3Wrapper, class P3Wrapped,
          R F(P1, P2, P3Wrapped)>
R CastHandlerData3(void *c, const void *hd, P3Wrapper p3) {
  return F(static_cast<P1>(c), static_cast<P2>(hd), p3);
}

template <class R, class P1, class P2, class P3, class P4, class P5,
          R F(P1, P2, P3, P4, P5)>
R CastHandlerData5(void *c, const void *hd, P3 p3, P4 p4, P5 p5) {
  return F(static_cast<P1>(c), static_cast<P2>(hd), p3, p4, p5);
}

template <class R, class P1, class P2, R F(P1, P2, const char *, size_t)>
R CastHandlerDataIgnoreHandle(void *c, const void *hd, const char *p3,
                              size_t p4, const BufferHandle *handle) {
  UPB_UNUSED(handle);
  return F(static_cast<P1>(c), static_cast<P2>(hd), p3, p4);
}

/* For unbound functions, ignore the handler data. */
template <class R, class P1, R F(P1), class I, class T>
struct ConvertParams<Func1<R, P1, F, I>, T> {
  typedef Func2<R, void *, const void *, IgnoreHandlerData2<R, P1, F>, I> Func;
};

template <class R, class P1, class P2, R F(P1, P2), class I,
          class R2, class P1_2, class P2_2, class P3_2>
struct ConvertParams<Func2<R, P1, P2, F, I>,
                     R2 (*)(P1_2, P2_2, P3_2)> {
  typedef Func3<R, void *, const void *, P3_2,
                IgnoreHandlerData3<R, P1, P3_2, P2, F>, I> Func;
};

/* For StringBuffer only; this ignores both the handler data and the
 * BufferHandle. */
template <class R, class P1, R F(P1, const char *, size_t), class I, class T>
struct ConvertParams<Func3<R, P1, const char *, size_t, F, I>, T> {
  typedef Func5<R, void *, const void *, const char *, size_t,
                const BufferHandle *, IgnoreHandlerDataIgnoreHandle<R, P1, F>,
                I> Func;
};

template <class R, class P1, class P2, class P3, class P4, R F(P1, P2, P3, P4),
          class I, class T>
struct ConvertParams<Func4<R, P1, P2, P3, P4, F, I>, T> {
  typedef Func5<R, void *, const void *, P2, P3, P4,
                IgnoreHandlerData5<R, P1, P2, P3, P4, F>, I> Func;
};

/* For bound functions, cast the handler data. */
template <class R, class P1, class P2, R F(P1, P2), class I, class T>
struct ConvertParams<BoundFunc2<R, P1, P2, F, I>, T> {
  typedef Func2<R, void *, const void *, CastHandlerData2<R, P1, P2, F>, I>
      Func;
};

template <class R, class P1, class P2, class P3, R F(P1, P2, P3), class I,
          class R2, class P1_2, class P2_2, class P3_2>
struct ConvertParams<BoundFunc3<R, P1, P2, P3, F, I>,
                     R2 (*)(P1_2, P2_2, P3_2)> {
  typedef Func3<R, void *, const void *, P3_2,
                CastHandlerData3<R, P1, P2, P3_2, P3, F>, I> Func;
};

/* For StringBuffer only; this ignores the BufferHandle. */
template <class R, class P1, class P2, R F(P1, P2, const char *, size_t),
          class I, class T>
struct ConvertParams<BoundFunc4<R, P1, P2, const char *, size_t, F, I>, T> {
  typedef Func5<R, void *, const void *, const char *, size_t,
                const BufferHandle *, CastHandlerDataIgnoreHandle<R, P1, P2, F>,
                I> Func;
};

template <class R, class P1, class P2, class P3, class P4, class P5,
          R F(P1, P2, P3, P4, P5), class I, class T>
struct ConvertParams<BoundFunc5<R, P1, P2, P3, P4, P5, F, I>, T> {
  typedef Func5<R, void *, const void *, P3, P4, P5,
                CastHandlerData5<R, P1, P2, P3, P4, P5, F>, I> Func;
};

/* utype/ltype are upper/lower-case, ctype is canonical C type, vtype is
 * variant C type. */
#define TYPE_METHODS(utype, ltype, ctype, vtype)                               \
  template <> struct CanonicalType<vtype> {                                    \
    typedef ctype Type;                                                        \
  };                                                                           \
  template <>                                                                  \
  inline bool Handlers::SetValueHandler<vtype>(                                \
      const FieldDef *f,                                                       \
      const Handlers::utype ## Handler& handler) {                             \
    assert(!handler.registered_);                                              \
    handler.AddCleanup(this);                                                  \
    handler.registered_ = true;                                                \
    return upb_handlers_set##ltype(this, f, handler.handler_, &handler.attr_); \
  }                                                                            \

TYPE_METHODS(Double, double, double,   double)
TYPE_METHODS(Float,  float,  float,    float)
TYPE_METHODS(UInt64, uint64, uint64_t, UPB_UINT64_T)
TYPE_METHODS(UInt32, uint32, uint32_t, UPB_UINT32_T)
TYPE_METHODS(Int64,  int64,  int64_t,  UPB_INT64_T)
TYPE_METHODS(Int32,  int32,  int32_t,  UPB_INT32_T)
TYPE_METHODS(Bool,   bool,   bool,     bool)

#ifdef UPB_TWO_32BIT_TYPES
TYPE_METHODS(Int32,  int32,  int32_t,  UPB_INT32ALT_T)
TYPE_METHODS(UInt32, uint32, uint32_t, UPB_UINT32ALT_T)
#endif

#ifdef UPB_TWO_64BIT_TYPES
TYPE_METHODS(Int64,  int64,  int64_t,  UPB_INT64ALT_T)
TYPE_METHODS(UInt64, uint64, uint64_t, UPB_UINT64ALT_T)
#endif
#undef TYPE_METHODS

template <> struct CanonicalType<Status*> {
  typedef Status* Type;
};

/* Type methods that are only one-per-canonical-type and not
 * one-per-cvariant. */

#define TYPE_METHODS(utype, ctype) \
    inline bool Handlers::Set##utype##Handler(const FieldDef *f, \
                                              const utype##Handler &h) { \
      return SetValueHandler<ctype>(f, h); \
    } \

TYPE_METHODS(Double, double)
TYPE_METHODS(Float,  float)
TYPE_METHODS(UInt64, uint64_t)
TYPE_METHODS(UInt32, uint32_t)
TYPE_METHODS(Int64,  int64_t)
TYPE_METHODS(Int32,  int32_t)
TYPE_METHODS(Bool,   bool)
#undef TYPE_METHODS

template <class F> struct ReturnOf;

template <class R, class P1, class P2>
struct ReturnOf<R (*)(P1, P2)> {
  typedef R Return;
};

template <class R, class P1, class P2, class P3>
struct ReturnOf<R (*)(P1, P2, P3)> {
  typedef R Return;
};

template <class R, class P1, class P2, class P3, class P4>
struct ReturnOf<R (*)(P1, P2, P3, P4)> {
  typedef R Return;
};

template <class R, class P1, class P2, class P3, class P4, class P5>
struct ReturnOf<R (*)(P1, P2, P3, P4, P5)> {
  typedef R Return;
};

template<class T> const void *UniquePtrForType() {
  static const char ch = 0;
  return &ch;
}

template <class T>
template <class F>
inline Handler<T>::Handler(F func)
    : registered_(false),
      cleanup_data_(func.GetData()),
      cleanup_func_(func.GetCleanup()) {
  upb_handlerattr_sethandlerdata(&attr_, func.GetData());
  typedef typename ReturnOf<T>::Return Return;
  typedef typename ConvertParams<F, T>::Func ConvertedParamsFunc;
  typedef typename MaybeWrapReturn<ConvertedParamsFunc, Return>::Func
      ReturnWrappedFunc;
  handler_ = ReturnWrappedFunc().Call;

  /* Set attributes based on what templates can statically tell us about the
   * user's function. */

  /* If the original function returns void, then we know that we wrapped it to
   * always return ok. */
  bool always_ok = is_same<typename F::FuncInfo::Return, void>::value;
  attr_.SetAlwaysOk(always_ok);

  /* Closure parameter and return type. */
  attr_.SetClosureType(UniquePtrForType<typename F::FuncInfo::Closure>());

  /* We use the closure type (from the first parameter) if the return type is
   * void or bool, since these are the two cases we wrap to return the closure's
   * type anyway.
   *
   * This is all nonsense for non START* handlers, but it doesn't matter because
   * in that case the value will be ignored. */
  typedef typename FirstUnlessVoidOrBool<typename F::FuncInfo::Return,
                                         typename F::FuncInfo::Closure>::value
      EffectiveReturn;
  attr_.SetReturnClosureType(UniquePtrForType<EffectiveReturn>());
}

template <class T>
inline Handler<T>::~Handler() {
  assert(registered_);
}

inline HandlerAttributes::HandlerAttributes() { upb_handlerattr_init(this); }
inline HandlerAttributes::~HandlerAttributes() { upb_handlerattr_uninit(this); }
inline bool HandlerAttributes::SetHandlerData(const void *hd) {
  return upb_handlerattr_sethandlerdata(this, hd);
}
inline const void* HandlerAttributes::handler_data() const {
  return upb_handlerattr_handlerdata(this);
}
inline bool HandlerAttributes::SetClosureType(const void *type) {
  return upb_handlerattr_setclosuretype(this, type);
}
inline const void* HandlerAttributes::closure_type() const {
  return upb_handlerattr_closuretype(this);
}
inline bool HandlerAttributes::SetReturnClosureType(const void *type) {
  return upb_handlerattr_setreturnclosuretype(this, type);
}
inline const void* HandlerAttributes::return_closure_type() const {
  return upb_handlerattr_returnclosuretype(this);
}
inline bool HandlerAttributes::SetAlwaysOk(bool always_ok) {
  return upb_handlerattr_setalwaysok(this, always_ok);
}
inline bool HandlerAttributes::always_ok() const {
  return upb_handlerattr_alwaysok(this);
}

inline BufferHandle::BufferHandle() { upb_bufhandle_init(this); }
inline BufferHandle::~BufferHandle() { upb_bufhandle_uninit(this); }
inline const char* BufferHandle::buffer() const {
  return upb_bufhandle_buf(this);
}
inline size_t BufferHandle::object_offset() const {
  return upb_bufhandle_objofs(this);
}
inline void BufferHandle::SetBuffer(const char* buf, size_t ofs) {
  upb_bufhandle_setbuf(this, buf, ofs);
}
template <class T>
void BufferHandle::SetAttachedObject(const T* obj) {
  upb_bufhandle_setobj(this, obj, UniquePtrForType<T>());
}
template <class T>
const T* BufferHandle::GetAttachedObject() const {
  return upb_bufhandle_objtype(this) == UniquePtrForType<T>()
      ? static_cast<const T *>(upb_bufhandle_obj(this))
                               : NULL;
}

inline reffed_ptr<Handlers> Handlers::New(const MessageDef *m) {
  upb_handlers *h = upb_handlers_new(m, &h);
  return reffed_ptr<Handlers>(h, &h);
}
inline reffed_ptr<const Handlers> Handlers::NewFrozen(
    const MessageDef *m, upb_handlers_callback *callback,
    const void *closure) {
  const upb_handlers *h = upb_handlers_newfrozen(m, &h, callback, closure);
  return reffed_ptr<const Handlers>(h, &h);
}
inline const Status* Handlers::status() {
  return upb_handlers_status(this);
}
inline void Handlers::ClearError() {
  return upb_handlers_clearerr(this);
}
inline bool Handlers::Freeze(Status *s) {
  upb::Handlers* h = this;
  return upb_handlers_freeze(&h, 1, s);
}
inline bool Handlers::Freeze(Handlers *const *handlers, int n, Status *s) {
  return upb_handlers_freeze(handlers, n, s);
}
inline bool Handlers::Freeze(const std::vector<Handlers*>& h, Status* status) {
  return upb_handlers_freeze((Handlers* const*)&h[0], h.size(), status);
}
inline const MessageDef *Handlers::message_def() const {
  return upb_handlers_msgdef(this);
}
inline bool Handlers::AddCleanup(void *p, upb_handlerfree *func) {
  return upb_handlers_addcleanup(this, p, func);
}
inline bool Handlers::SetStartMessageHandler(
    const Handlers::StartMessageHandler &handler) {
  assert(!handler.registered_);
  handler.registered_ = true;
  handler.AddCleanup(this);
  return upb_handlers_setstartmsg(this, handler.handler_, &handler.attr_);
}
inline bool Handlers::SetEndMessageHandler(
    const Handlers::EndMessageHandler &handler) {
  assert(!handler.registered_);
  handler.registered_ = true;
  handler.AddCleanup(this);
  return upb_handlers_setendmsg(this, handler.handler_, &handler.attr_);
}
inline bool Handlers::SetStartStringHandler(const FieldDef *f,
                                            const StartStringHandler &handler) {
  assert(!handler.registered_);
  handler.registered_ = true;
  handler.AddCleanup(this);
  return upb_handlers_setstartstr(this, f, handler.handler_, &handler.attr_);
}
inline bool Handlers::SetEndStringHandler(const FieldDef *f,
                                          const EndFieldHandler &handler) {
  assert(!handler.registered_);
  handler.registered_ = true;
  handler.AddCleanup(this);
  return upb_handlers_setendstr(this, f, handler.handler_, &handler.attr_);
}
inline bool Handlers::SetStringHandler(const FieldDef *f,
                                       const StringHandler& handler) {
  assert(!handler.registered_);
  handler.registered_ = true;
  handler.AddCleanup(this);
  return upb_handlers_setstring(this, f, handler.handler_, &handler.attr_);
}
inline bool Handlers::SetStartSequenceHandler(
    const FieldDef *f, const StartFieldHandler &handler) {
  assert(!handler.registered_);
  handler.registered_ = true;
  handler.AddCleanup(this);
  return upb_handlers_setstartseq(this, f, handler.handler_, &handler.attr_);
}
inline bool Handlers::SetStartSubMessageHandler(
    const FieldDef *f, const StartFieldHandler &handler) {
  assert(!handler.registered_);
  handler.registered_ = true;
  handler.AddCleanup(this);
  return upb_handlers_setstartsubmsg(this, f, handler.handler_, &handler.attr_);
}
inline bool Handlers::SetEndSubMessageHandler(const FieldDef *f,
                                              const EndFieldHandler &handler) {
  assert(!handler.registered_);
  handler.registered_ = true;
  handler.AddCleanup(this);
  return upb_handlers_setendsubmsg(this, f, handler.handler_, &handler.attr_);
}
inline bool Handlers::SetEndSequenceHandler(const FieldDef *f,
                                            const EndFieldHandler &handler) {
  assert(!handler.registered_);
  handler.registered_ = true;
  handler.AddCleanup(this);
  return upb_handlers_setendseq(this, f, handler.handler_, &handler.attr_);
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

inline BytesHandler::BytesHandler() {
  upb_byteshandler_init(this);
}

inline BytesHandler::~BytesHandler() {}

}  /* namespace upb */

#endif  /* __cplusplus */


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

#endif  /* UPB_HANDLERS_INL_H_ */

#endif  /* UPB_HANDLERS_H */
/*
** upb::Environment (upb_env)
**
** A upb::Environment provides a means for injecting malloc and an
** error-reporting callback into encoders/decoders.  This allows them to be
** independent of nearly all assumptions about their actual environment.
**
** It is also a container for allocating the encoders/decoders themselves that
** insulates clients from knowing their actual size.  This provides ABI
** compatibility even if the size of the objects change.  And this allows the
** structure definitions to be in the .c files instead of the .h files, making
** the .h files smaller and more readable.
*/


#ifndef UPB_ENV_H_
#define UPB_ENV_H_

#ifdef __cplusplus
namespace upb {
class Environment;
class SeededAllocator;
}
#endif

UPB_DECLARE_TYPE(upb::Environment, upb_env)
UPB_DECLARE_TYPE(upb::SeededAllocator, upb_seededalloc)

typedef void *upb_alloc_func(void *ud, void *ptr, size_t oldsize, size_t size);
typedef void upb_cleanup_func(void *ud);
typedef bool upb_error_func(void *ud, const upb_status *status);

#ifdef __cplusplus

/* An environment is *not* thread-safe. */
class upb::Environment {
 public:
  Environment();
  ~Environment();

  /* Set a custom memory allocation function for the environment.  May ONLY
   * be called before any calls to Malloc()/Realloc()/AddCleanup() below.
   * If this is not called, the system realloc() function will be used.
   * The given user pointer "ud" will be passed to the allocation function.
   *
   * The allocation function will not receive corresponding "free" calls.  it
   * must ensure that the memory is valid for the lifetime of the Environment,
   * but it may be reclaimed any time thereafter.  The likely usage is that
   * "ud" points to a stateful allocator, and that the allocator frees all
   * memory, arena-style, when it is destroyed.  In this case the allocator must
   * outlive the Environment.  Another possibility is that the allocation
   * function returns GC-able memory that is guaranteed to be GC-rooted for the
   * life of the Environment. */
  void SetAllocationFunction(upb_alloc_func* alloc, void* ud);

  template<class T>
  void SetAllocator(T* allocator) {
    SetAllocationFunction(allocator->GetAllocationFunction(), allocator);
  }

  /* Set a custom error reporting function. */
  void SetErrorFunction(upb_error_func* func, void* ud);

  /* Set the error reporting function to simply copy the status to the given
   * status and abort. */
  void ReportErrorsTo(Status* status);

  /* Returns true if all allocations and AddCleanup() calls have succeeded,
   * and no errors were reported with ReportError() (except ones that recovered
   * successfully). */
  bool ok() const;

  /* Functions for use by encoders/decoders. **********************************/

  /* Reports an error to this environment's callback, returning true if
   * the caller should try to recover. */
  bool ReportError(const Status* status);

  /* Allocate memory.  Uses the environment's allocation function.
   *
   * There is no need to free(). All memory will be freed automatically, but is
   * guaranteed to outlive the Environment. */
  void* Malloc(size_t size);

  /* Reallocate memory.  Preserves "oldsize" bytes from the existing buffer
   * Requires: oldsize <= existing_size.
   *
   * TODO(haberman): should we also enforce that oldsize <= size? */
  void* Realloc(void* ptr, size_t oldsize, size_t size);

  /* Add a cleanup function to run when the environment is destroyed.
   * Returns false on out-of-memory.
   *
   * The first call to AddCleanup() after SetAllocationFunction() is guaranteed
   * to return true -- this makes it possible to robustly set a cleanup handler
   * for a custom allocation function. */
  bool AddCleanup(upb_cleanup_func* func, void* ud);

  /* Total number of bytes that have been allocated.  It is undefined what
   * Realloc() does to this counter. */
  size_t BytesAllocated() const;

 private:
  UPB_DISALLOW_COPY_AND_ASSIGN(Environment)

#else
struct upb_env {
#endif  /* __cplusplus */

  bool ok_;
  size_t bytes_allocated;

  /* Alloc function. */
  upb_alloc_func *alloc;
  void *alloc_ud;

  /* Error-reporting function. */
  upb_error_func *err;
  void *err_ud;

  /* Userdata for default alloc func. */
  void *default_alloc_ud;

  /* Cleanup entries.  Pointer to a cleanup_ent, defined in env.c */
  void *cleanup_head;

  /* For future expansion, since the size of this struct is exposed to users. */
  void *future1;
  void *future2;
};

UPB_BEGIN_EXTERN_C

void upb_env_init(upb_env *e);
void upb_env_uninit(upb_env *e);
void upb_env_setallocfunc(upb_env *e, upb_alloc_func *func, void *ud);
void upb_env_seterrorfunc(upb_env *e, upb_error_func *func, void *ud);
void upb_env_reporterrorsto(upb_env *e, upb_status *status);
bool upb_env_ok(const upb_env *e);
bool upb_env_reporterror(upb_env *e, const upb_status *status);
void *upb_env_malloc(upb_env *e, size_t size);
void *upb_env_realloc(upb_env *e, void *ptr, size_t oldsize, size_t size);
bool upb_env_addcleanup(upb_env *e, upb_cleanup_func *func, void *ud);
size_t upb_env_bytesallocated(const upb_env *e);

UPB_END_EXTERN_C

#ifdef __cplusplus

/* An allocator that allocates from an initial memory region (likely the stack)
 * before falling back to another allocator. */
class upb::SeededAllocator {
 public:
  SeededAllocator(void *mem, size_t len);
  ~SeededAllocator();

  /* Set a custom fallback memory allocation function for the allocator, to use
   * once the initial region runs out.
   *
   * May ONLY be called before GetAllocationFunction().  If this is not
   * called, the system realloc() will be the fallback allocator. */
  void SetFallbackAllocator(upb_alloc_func *alloc, void *ud);

  /* Gets the allocation function for this allocator. */
  upb_alloc_func* GetAllocationFunction();

 private:
  UPB_DISALLOW_COPY_AND_ASSIGN(SeededAllocator)

#else
struct upb_seededalloc {
#endif  /* __cplusplus */

  /* Fallback alloc function.  */
  upb_alloc_func *alloc;
  upb_cleanup_func *alloc_cleanup;
  void *alloc_ud;
  bool need_cleanup;
  bool returned_allocfunc;

  /* Userdata for default alloc func. */
  void *default_alloc_ud;

  /* Pointers for the initial memory region. */
  char *mem_base;
  char *mem_ptr;
  char *mem_limit;

  /* For future expansion, since the size of this struct is exposed to users. */
  void *future1;
  void *future2;
};

UPB_BEGIN_EXTERN_C

void upb_seededalloc_init(upb_seededalloc *a, void *mem, size_t len);
void upb_seededalloc_uninit(upb_seededalloc *a);
void upb_seededalloc_setfallbackalloc(upb_seededalloc *a, upb_alloc_func *func,
                                      void *ud);
upb_alloc_func *upb_seededalloc_getallocfunc(upb_seededalloc *a);

UPB_END_EXTERN_C

#ifdef __cplusplus

namespace upb {

inline Environment::Environment() {
  upb_env_init(this);
}
inline Environment::~Environment() {
  upb_env_uninit(this);
}
inline void Environment::SetAllocationFunction(upb_alloc_func *alloc,
                                               void *ud) {
  upb_env_setallocfunc(this, alloc, ud);
}
inline void Environment::SetErrorFunction(upb_error_func *func, void *ud) {
  upb_env_seterrorfunc(this, func, ud);
}
inline void Environment::ReportErrorsTo(Status* status) {
  upb_env_reporterrorsto(this, status);
}
inline bool Environment::ok() const {
  return upb_env_ok(this);
}
inline bool Environment::ReportError(const Status* status) {
  return upb_env_reporterror(this, status);
}
inline void *Environment::Malloc(size_t size) {
  return upb_env_malloc(this, size);
}
inline void *Environment::Realloc(void *ptr, size_t oldsize, size_t size) {
  return upb_env_realloc(this, ptr, oldsize, size);
}
inline bool Environment::AddCleanup(upb_cleanup_func *func, void *ud) {
  return upb_env_addcleanup(this, func, ud);
}
inline size_t Environment::BytesAllocated() const {
  return upb_env_bytesallocated(this);
}

inline SeededAllocator::SeededAllocator(void *mem, size_t len) {
  upb_seededalloc_init(this, mem, len);
}
inline SeededAllocator::~SeededAllocator() {
  upb_seededalloc_uninit(this);
}
inline void SeededAllocator::SetFallbackAllocator(upb_alloc_func *alloc,
                                                  void *ud) {
  upb_seededalloc_setfallbackalloc(this, alloc, ud);
}
inline upb_alloc_func *SeededAllocator::GetAllocationFunction() {
  return upb_seededalloc_getallocfunc(this);
}

}  /* namespace upb */

#endif  /* __cplusplus */

#endif  /* UPB_ENV_H_ */
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


#ifdef __cplusplus
namespace upb {
class BufferSource;
class BytesSink;
class Sink;
}
#endif

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

UPB_INLINE bool upb_bufsrc_putbuf(const char *buf, size_t len,
                                  upb_bytessink *sink) {
  void *subc;
  bool ret;
  upb_bufhandle handle;
  upb_bufhandle_init(&handle);
  upb_bufhandle_setbuf(&handle, buf, 0);
  ret = upb_bytessink_start(sink, len, &subc);
  if (ret && len != 0) {
    ret = (upb_bytessink_putbuf(sink, subc, buf, len, &handle) >= len);
  }
  if (ret) {
    ret = upb_bytessink_end(sink);
  }
  upb_bufhandle_uninit(&handle);
  return ret;
}

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
/*
** For handlers that do very tiny, very simple operations, the function call
** overhead of calling a handler can be significant.  This file allows the
** user to define handlers that do something very simple like store the value
** to memory and/or set a hasbit.  JIT compilers can then special-case these
** handlers and emit specialized code for them instead of actually calling the
** handler.
**
** The functionality is very simple/limited right now but may expand to be able
** to call another function.
*/

#ifndef UPB_SHIM_H
#define UPB_SHIM_H


typedef struct {
  size_t offset;
  int32_t hasbit;
} upb_shim_data;

#ifdef __cplusplus

namespace upb {

struct Shim {
  typedef upb_shim_data Data;

  /* Sets a handler for the given field that writes the value to the given
   * offset and, if hasbit >= 0, sets a bit at the given bit offset.  Returns
   * true if the handler was set successfully. */
  static bool Set(Handlers *h, const FieldDef *f, size_t ofs, int32_t hasbit);

  /* If this handler is a shim, returns the corresponding upb::Shim::Data and
   * stores the type in "type".  Otherwise returns NULL. */
  static const Data* GetData(const Handlers* h, Handlers::Selector s,
                             FieldDef::Type* type);
};

}  /* namespace upb */

#endif

UPB_BEGIN_EXTERN_C

/* C API. */
bool upb_shim_set(upb_handlers *h, const upb_fielddef *f, size_t offset,
                  int32_t hasbit);
const upb_shim_data *upb_shim_getdata(const upb_handlers *h, upb_selector_t s,
                                      upb_fieldtype_t *type);

UPB_END_EXTERN_C

#ifdef __cplusplus
/* C++ Wrappers. */
namespace upb {
inline bool Shim::Set(Handlers* h, const FieldDef* f, size_t ofs,
                      int32_t hasbit) {
  return upb_shim_set(h, f, ofs, hasbit);
}
inline const Shim::Data* Shim::GetData(const Handlers* h, Handlers::Selector s,
                                       FieldDef::Type* type) {
  return upb_shim_getdata(h, s, type);
}
}  /* namespace upb */
#endif

#endif  /* UPB_SHIM_H */
/*
** upb::SymbolTable (upb_symtab)
**
** A symtab (symbol table) stores a name->def map of upb_defs.  Clients could
** always create such tables themselves, but upb_symtab has logic for resolving
** symbolic references, and in particular, for keeping a whole set of consistent
** defs when replacing some subset of those defs.  This logic is nontrivial.
**
** This is a mixed C/C++ interface that offers a full API to both languages.
** See the top-level README for more information.
*/

#ifndef UPB_SYMTAB_H_
#define UPB_SYMTAB_H_


#ifdef __cplusplus
#include <vector>
namespace upb { class SymbolTable; }
#endif

UPB_DECLARE_DERIVED_TYPE(upb::SymbolTable, upb::RefCounted,
                         upb_symtab, upb_refcounted)

typedef struct {
 UPB_PRIVATE_FOR_CPP
  upb_strtable_iter iter;
  upb_deftype_t type;
} upb_symtab_iter;

#ifdef __cplusplus

/* Non-const methods in upb::SymbolTable are NOT thread-safe. */
class upb::SymbolTable {
 public:
  /* Returns a new symbol table with a single ref owned by "owner."
   * Returns NULL if memory allocation failed. */
  static reffed_ptr<SymbolTable> New();

  /* Include RefCounted base methods. */
  UPB_REFCOUNTED_CPPMETHODS

  /* For all lookup functions, the returned pointer is not owned by the
   * caller; it may be invalidated by any non-const call or unref of the
   * SymbolTable!  To protect against this, take a ref if desired. */

  /* Freezes the symbol table: prevents further modification of it.
   * After the Freeze() operation is successful, the SymbolTable must only be
   * accessed via a const pointer.
   *
   * Unlike with upb::MessageDef/upb::EnumDef/etc, freezing a SymbolTable is not
   * a necessary step in using a SymbolTable.  If you have no need for it to be
   * immutable, there is no need to freeze it ever.  However sometimes it is
   * useful, and SymbolTables that are statically compiled into the binary are
   * always frozen by nature. */
  void Freeze();

  /* Resolves the given symbol using the rules described in descriptor.proto,
   * namely:
   *
   *    If the name starts with a '.', it is fully-qualified.  Otherwise,
   *    C++-like scoping rules are used to find the type (i.e. first the nested
   *    types within this message are searched, then within the parent, on up
   *    to the root namespace).
   *
   * If not found, returns NULL. */
  const Def* Resolve(const char* base, const char* sym) const;

  /* Finds an entry in the symbol table with this exact name.  If not found,
   * returns NULL. */
  const Def* Lookup(const char *sym) const;
  const MessageDef* LookupMessage(const char *sym) const;
  const EnumDef* LookupEnum(const char *sym) const;

  /* TODO: introduce a C++ iterator, but make it nice and templated so that if
   * you ask for an iterator of MessageDef the iterated elements are strongly
   * typed as MessageDef*. */

  /* Adds the given mutable defs to the symtab, resolving all symbols
   * (including enum default values) and finalizing the defs.  Only one def per
   * name may be in the list, but defs can replace existing defs in the symtab.
   * All defs must have a name -- anonymous defs are not allowed.  Anonymous
   * defs can still be frozen by calling upb_def_freeze() directly.
   *
   * Any existing defs that can reach defs that are being replaced will
   * themselves be replaced also, so that the resulting set of defs is fully
   * consistent.
   *
   * This logic implemented in this method is a convenience; ultimately it
   * calls some combination of upb_fielddef_setsubdef(), upb_def_dup(), and
   * upb_freeze(), any of which the client could call themself.  However, since
   * the logic for doing so is nontrivial, we provide it here.
   *
   * The entire operation either succeeds or fails.  If the operation fails,
   * the symtab is unchanged, false is returned, and status indicates the
   * error.  The caller passes a ref on all defs to the symtab (even if the
   * operation fails).
   *
   * TODO(haberman): currently failure will leave the symtab unchanged, but may
   * leave the defs themselves partially resolved.  Does this matter?  If so we
   * could do a prepass that ensures that all symbols are resolvable and bail
   * if not, so we don't mutate anything until we know the operation will
   * succeed.
   *
   * TODO(haberman): since the defs must be mutable, refining a frozen def
   * requires making mutable copies of the entire tree.  This is wasteful if
   * only a few messages are changing.  We may want to add a way of adding a
   * tree of frozen defs to the symtab (perhaps an alternate constructor where
   * you pass the root of the tree?) */
  bool Add(Def*const* defs, int n, void* ref_donor, upb_status* status);

  bool Add(const std::vector<Def*>& defs, void *owner, Status* status) {
    return Add((Def*const*)&defs[0], defs.size(), owner, status);
  }

 private:
  UPB_DISALLOW_POD_OPS(SymbolTable, upb::SymbolTable)
};

#endif  /* __cplusplus */

UPB_BEGIN_EXTERN_C

/* Native C API. */

/* Include refcounted methods like upb_symtab_ref(). */
UPB_REFCOUNTED_CMETHODS(upb_symtab, upb_symtab_upcast)

upb_symtab *upb_symtab_new(const void *owner);
void upb_symtab_freeze(upb_symtab *s);
const upb_def *upb_symtab_resolve(const upb_symtab *s, const char *base,
                                  const char *sym);
const upb_def *upb_symtab_lookup(const upb_symtab *s, const char *sym);
const upb_msgdef *upb_symtab_lookupmsg(const upb_symtab *s, const char *sym);
const upb_enumdef *upb_symtab_lookupenum(const upb_symtab *s, const char *sym);
bool upb_symtab_add(upb_symtab *s, upb_def *const*defs, int n, void *ref_donor,
                    upb_status *status);

/* upb_symtab_iter i;
 * for(upb_symtab_begin(&i, s, type); !upb_symtab_done(&i);
 *     upb_symtab_next(&i)) {
 *   const upb_def *def = upb_symtab_iter_def(&i);
 *    // ...
 * }
 *
 * For C we don't have separate iterators for const and non-const.
 * It is the caller's responsibility to cast the upb_fielddef* to
 * const if the upb_msgdef* is const. */
void upb_symtab_begin(upb_symtab_iter *iter, const upb_symtab *s,
                      upb_deftype_t type);
void upb_symtab_next(upb_symtab_iter *iter);
bool upb_symtab_done(const upb_symtab_iter *iter);
const upb_def *upb_symtab_iter_def(const upb_symtab_iter *iter);

UPB_END_EXTERN_C

#ifdef __cplusplus
/* C++ inline wrappers. */
namespace upb {
inline reffed_ptr<SymbolTable> SymbolTable::New() {
  upb_symtab *s = upb_symtab_new(&s);
  return reffed_ptr<SymbolTable>(s, &s);
}

inline void SymbolTable::Freeze() {
  return upb_symtab_freeze(this);
}
inline const Def *SymbolTable::Resolve(const char *base,
                                       const char *sym) const {
  return upb_symtab_resolve(this, base, sym);
}
inline const Def* SymbolTable::Lookup(const char *sym) const {
  return upb_symtab_lookup(this, sym);
}
inline const MessageDef *SymbolTable::LookupMessage(const char *sym) const {
  return upb_symtab_lookupmsg(this, sym);
}
inline bool SymbolTable::Add(
    Def*const* defs, int n, void* ref_donor, upb_status* status) {
  return upb_symtab_add(this, (upb_def*const*)defs, n, ref_donor, status);
}
}  /* namespace upb */
#endif

#endif  /* UPB_SYMTAB_H_ */
/*
** upb::descriptor::Reader (upb_descreader)
**
** Provides a way of building upb::Defs from data in descriptor.proto format.
*/

#ifndef UPB_DESCRIPTOR_H
#define UPB_DESCRIPTOR_H


#ifdef __cplusplus
namespace upb {
namespace descriptor {
class Reader;
}  /* namespace descriptor */
}  /* namespace upb */
#endif

UPB_DECLARE_TYPE(upb::descriptor::Reader, upb_descreader)

#ifdef __cplusplus

/* Class that receives descriptor data according to the descriptor.proto schema
 * and use it to build upb::Defs corresponding to that schema. */
class upb::descriptor::Reader {
 public:
  /* These handlers must have come from NewHandlers() and must outlive the
   * Reader.
   *
   * TODO: generate the handlers statically (like we do with the
   * descriptor.proto defs) so that there is no need to pass this parameter (or
   * to build/memory-manage the handlers at runtime at all).  Unfortunately this
   * is a bit tricky to implement for Handlers, but necessary to simplify this
   * interface. */
  static Reader* Create(Environment* env, const Handlers* handlers);

  /* The reader's input; this is where descriptor.proto data should be sent. */
  Sink* input();

  /* Returns an array of all defs that have been parsed, and transfers ownership
   * of them to "owner".  The number of defs is stored in *n.  Ownership of the
   * returned array is retained and is invalidated by any other call into
   * Reader.
   *
   * These defs are not frozen or resolved; they are ready to be added to a
   * symtab. */
  upb::Def** GetDefs(void* owner, int* n);

  /* Builds and returns handlers for the reader, owned by "owner." */
  static Handlers* NewHandlers(const void* owner);

 private:
  UPB_DISALLOW_POD_OPS(Reader, upb::descriptor::Reader)
};

#endif

UPB_BEGIN_EXTERN_C

/* C API. */
upb_descreader *upb_descreader_create(upb_env *e, const upb_handlers *h);
upb_sink *upb_descreader_input(upb_descreader *r);
upb_def **upb_descreader_getdefs(upb_descreader *r, void *owner, int *n);
const upb_handlers *upb_descreader_newhandlers(const void *owner);

UPB_END_EXTERN_C

#ifdef __cplusplus
/* C++ implementation details. ************************************************/
namespace upb {
namespace descriptor {
inline Reader* Reader::Create(Environment* e, const Handlers *h) {
  return upb_descreader_create(e, h);
}
inline Sink* Reader::input() { return upb_descreader_input(this); }
inline upb::Def** Reader::GetDefs(void* owner, int* n) {
  return upb_descreader_getdefs(this, owner, n);
}
}  /* namespace descriptor */
}  /* namespace upb */
#endif

#endif  /* UPB_DESCRIPTOR_H */
/* This file contains accessors for a set of compiled-in defs.
 * Note that unlike Google's protobuf, it does *not* define
 * generated classes or any other kind of data structure for
 * actually storing protobufs.  It only contains *defs* which
 * let you reflect over a protobuf *schema*.
 */
/* This file was generated by upbc (the upb compiler).
 * Do not edit -- your changes will be discarded when the file is
 * regenerated. */

#ifndef GOOGLE_PROTOBUF_DESCRIPTOR_UPB_H_
#define GOOGLE_PROTOBUF_DESCRIPTOR_UPB_H_


#ifdef __cplusplus
UPB_BEGIN_EXTERN_C
#endif

/* Enums */

typedef enum {
  GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_LABEL_OPTIONAL = 1,
  GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_LABEL_REQUIRED = 2,
  GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_LABEL_REPEATED = 3
} google_protobuf_FieldDescriptorProto_Label;

typedef enum {
  GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_TYPE_DOUBLE = 1,
  GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_TYPE_FLOAT = 2,
  GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_TYPE_INT64 = 3,
  GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_TYPE_UINT64 = 4,
  GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_TYPE_INT32 = 5,
  GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_TYPE_FIXED64 = 6,
  GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_TYPE_FIXED32 = 7,
  GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_TYPE_BOOL = 8,
  GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_TYPE_STRING = 9,
  GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_TYPE_GROUP = 10,
  GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_TYPE_MESSAGE = 11,
  GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_TYPE_BYTES = 12,
  GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_TYPE_UINT32 = 13,
  GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_TYPE_ENUM = 14,
  GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_TYPE_SFIXED32 = 15,
  GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_TYPE_SFIXED64 = 16,
  GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_TYPE_SINT32 = 17,
  GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_TYPE_SINT64 = 18
} google_protobuf_FieldDescriptorProto_Type;

typedef enum {
  GOOGLE_PROTOBUF_FIELDOPTIONS_STRING = 0,
  GOOGLE_PROTOBUF_FIELDOPTIONS_CORD = 1,
  GOOGLE_PROTOBUF_FIELDOPTIONS_STRING_PIECE = 2
} google_protobuf_FieldOptions_CType;

typedef enum {
  GOOGLE_PROTOBUF_FILEOPTIONS_SPEED = 1,
  GOOGLE_PROTOBUF_FILEOPTIONS_CODE_SIZE = 2,
  GOOGLE_PROTOBUF_FILEOPTIONS_LITE_RUNTIME = 3
} google_protobuf_FileOptions_OptimizeMode;

/* Selectors */

/* google.protobuf.DescriptorProto */
#define SEL_GOOGLE_PROTOBUF_DESCRIPTORPROTO_FIELD_STARTSUBMSG 2
#define SEL_GOOGLE_PROTOBUF_DESCRIPTORPROTO_NESTED_TYPE_STARTSUBMSG 3
#define SEL_GOOGLE_PROTOBUF_DESCRIPTORPROTO_ENUM_TYPE_STARTSUBMSG 4
#define SEL_GOOGLE_PROTOBUF_DESCRIPTORPROTO_EXTENSION_RANGE_STARTSUBMSG 5
#define SEL_GOOGLE_PROTOBUF_DESCRIPTORPROTO_EXTENSION_STARTSUBMSG 6
#define SEL_GOOGLE_PROTOBUF_DESCRIPTORPROTO_OPTIONS_STARTSUBMSG 7
#define SEL_GOOGLE_PROTOBUF_DESCRIPTORPROTO_FIELD_STARTSEQ 8
#define SEL_GOOGLE_PROTOBUF_DESCRIPTORPROTO_FIELD_ENDSEQ 9
#define SEL_GOOGLE_PROTOBUF_DESCRIPTORPROTO_FIELD_ENDSUBMSG 10
#define SEL_GOOGLE_PROTOBUF_DESCRIPTORPROTO_NESTED_TYPE_STARTSEQ 11
#define SEL_GOOGLE_PROTOBUF_DESCRIPTORPROTO_NESTED_TYPE_ENDSEQ 12
#define SEL_GOOGLE_PROTOBUF_DESCRIPTORPROTO_NESTED_TYPE_ENDSUBMSG 13
#define SEL_GOOGLE_PROTOBUF_DESCRIPTORPROTO_ENUM_TYPE_STARTSEQ 14
#define SEL_GOOGLE_PROTOBUF_DESCRIPTORPROTO_ENUM_TYPE_ENDSEQ 15
#define SEL_GOOGLE_PROTOBUF_DESCRIPTORPROTO_ENUM_TYPE_ENDSUBMSG 16
#define SEL_GOOGLE_PROTOBUF_DESCRIPTORPROTO_EXTENSION_RANGE_STARTSEQ 17
#define SEL_GOOGLE_PROTOBUF_DESCRIPTORPROTO_EXTENSION_RANGE_ENDSEQ 18
#define SEL_GOOGLE_PROTOBUF_DESCRIPTORPROTO_EXTENSION_RANGE_ENDSUBMSG 19
#define SEL_GOOGLE_PROTOBUF_DESCRIPTORPROTO_EXTENSION_STARTSEQ 20
#define SEL_GOOGLE_PROTOBUF_DESCRIPTORPROTO_EXTENSION_ENDSEQ 21
#define SEL_GOOGLE_PROTOBUF_DESCRIPTORPROTO_EXTENSION_ENDSUBMSG 22
#define SEL_GOOGLE_PROTOBUF_DESCRIPTORPROTO_OPTIONS_ENDSUBMSG 23
#define SEL_GOOGLE_PROTOBUF_DESCRIPTORPROTO_NAME_STRING 24
#define SEL_GOOGLE_PROTOBUF_DESCRIPTORPROTO_NAME_STARTSTR 25
#define SEL_GOOGLE_PROTOBUF_DESCRIPTORPROTO_NAME_ENDSTR 26

/* google.protobuf.DescriptorProto.ExtensionRange */
#define SEL_GOOGLE_PROTOBUF_DESCRIPTORPROTO_EXTENSIONRANGE_START_INT32 2
#define SEL_GOOGLE_PROTOBUF_DESCRIPTORPROTO_EXTENSIONRANGE_END_INT32 3

/* google.protobuf.EnumDescriptorProto */
#define SEL_GOOGLE_PROTOBUF_ENUMDESCRIPTORPROTO_VALUE_STARTSUBMSG 2
#define SEL_GOOGLE_PROTOBUF_ENUMDESCRIPTORPROTO_OPTIONS_STARTSUBMSG 3
#define SEL_GOOGLE_PROTOBUF_ENUMDESCRIPTORPROTO_VALUE_STARTSEQ 4
#define SEL_GOOGLE_PROTOBUF_ENUMDESCRIPTORPROTO_VALUE_ENDSEQ 5
#define SEL_GOOGLE_PROTOBUF_ENUMDESCRIPTORPROTO_VALUE_ENDSUBMSG 6
#define SEL_GOOGLE_PROTOBUF_ENUMDESCRIPTORPROTO_OPTIONS_ENDSUBMSG 7
#define SEL_GOOGLE_PROTOBUF_ENUMDESCRIPTORPROTO_NAME_STRING 8
#define SEL_GOOGLE_PROTOBUF_ENUMDESCRIPTORPROTO_NAME_STARTSTR 9
#define SEL_GOOGLE_PROTOBUF_ENUMDESCRIPTORPROTO_NAME_ENDSTR 10

/* google.protobuf.EnumOptions */
#define SEL_GOOGLE_PROTOBUF_ENUMOPTIONS_UNINTERPRETED_OPTION_STARTSUBMSG 2
#define SEL_GOOGLE_PROTOBUF_ENUMOPTIONS_UNINTERPRETED_OPTION_STARTSEQ 3
#define SEL_GOOGLE_PROTOBUF_ENUMOPTIONS_UNINTERPRETED_OPTION_ENDSEQ 4
#define SEL_GOOGLE_PROTOBUF_ENUMOPTIONS_UNINTERPRETED_OPTION_ENDSUBMSG 5
#define SEL_GOOGLE_PROTOBUF_ENUMOPTIONS_ALLOW_ALIAS_BOOL 6

/* google.protobuf.EnumValueDescriptorProto */
#define SEL_GOOGLE_PROTOBUF_ENUMVALUEDESCRIPTORPROTO_OPTIONS_STARTSUBMSG 2
#define SEL_GOOGLE_PROTOBUF_ENUMVALUEDESCRIPTORPROTO_OPTIONS_ENDSUBMSG 3
#define SEL_GOOGLE_PROTOBUF_ENUMVALUEDESCRIPTORPROTO_NAME_STRING 4
#define SEL_GOOGLE_PROTOBUF_ENUMVALUEDESCRIPTORPROTO_NAME_STARTSTR 5
#define SEL_GOOGLE_PROTOBUF_ENUMVALUEDESCRIPTORPROTO_NAME_ENDSTR 6
#define SEL_GOOGLE_PROTOBUF_ENUMVALUEDESCRIPTORPROTO_NUMBER_INT32 7

/* google.protobuf.EnumValueOptions */
#define SEL_GOOGLE_PROTOBUF_ENUMVALUEOPTIONS_UNINTERPRETED_OPTION_STARTSUBMSG 2
#define SEL_GOOGLE_PROTOBUF_ENUMVALUEOPTIONS_UNINTERPRETED_OPTION_STARTSEQ 3
#define SEL_GOOGLE_PROTOBUF_ENUMVALUEOPTIONS_UNINTERPRETED_OPTION_ENDSEQ 4
#define SEL_GOOGLE_PROTOBUF_ENUMVALUEOPTIONS_UNINTERPRETED_OPTION_ENDSUBMSG 5

/* google.protobuf.FieldDescriptorProto */
#define SEL_GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_OPTIONS_STARTSUBMSG 2
#define SEL_GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_OPTIONS_ENDSUBMSG 3
#define SEL_GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_NAME_STRING 4
#define SEL_GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_NAME_STARTSTR 5
#define SEL_GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_NAME_ENDSTR 6
#define SEL_GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_EXTENDEE_STRING 7
#define SEL_GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_EXTENDEE_STARTSTR 8
#define SEL_GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_EXTENDEE_ENDSTR 9
#define SEL_GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_NUMBER_INT32 10
#define SEL_GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_LABEL_INT32 11
#define SEL_GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_TYPE_INT32 12
#define SEL_GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_TYPE_NAME_STRING 13
#define SEL_GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_TYPE_NAME_STARTSTR 14
#define SEL_GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_TYPE_NAME_ENDSTR 15
#define SEL_GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_DEFAULT_VALUE_STRING 16
#define SEL_GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_DEFAULT_VALUE_STARTSTR 17
#define SEL_GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_DEFAULT_VALUE_ENDSTR 18

/* google.protobuf.FieldOptions */
#define SEL_GOOGLE_PROTOBUF_FIELDOPTIONS_UNINTERPRETED_OPTION_STARTSUBMSG 2
#define SEL_GOOGLE_PROTOBUF_FIELDOPTIONS_UNINTERPRETED_OPTION_STARTSEQ 3
#define SEL_GOOGLE_PROTOBUF_FIELDOPTIONS_UNINTERPRETED_OPTION_ENDSEQ 4
#define SEL_GOOGLE_PROTOBUF_FIELDOPTIONS_UNINTERPRETED_OPTION_ENDSUBMSG 5
#define SEL_GOOGLE_PROTOBUF_FIELDOPTIONS_CTYPE_INT32 6
#define SEL_GOOGLE_PROTOBUF_FIELDOPTIONS_PACKED_BOOL 7
#define SEL_GOOGLE_PROTOBUF_FIELDOPTIONS_DEPRECATED_BOOL 8
#define SEL_GOOGLE_PROTOBUF_FIELDOPTIONS_LAZY_BOOL 9
#define SEL_GOOGLE_PROTOBUF_FIELDOPTIONS_EXPERIMENTAL_MAP_KEY_STRING 10
#define SEL_GOOGLE_PROTOBUF_FIELDOPTIONS_EXPERIMENTAL_MAP_KEY_STARTSTR 11
#define SEL_GOOGLE_PROTOBUF_FIELDOPTIONS_EXPERIMENTAL_MAP_KEY_ENDSTR 12
#define SEL_GOOGLE_PROTOBUF_FIELDOPTIONS_WEAK_BOOL 13

/* google.protobuf.FileDescriptorProto */
#define SEL_GOOGLE_PROTOBUF_FILEDESCRIPTORPROTO_MESSAGE_TYPE_STARTSUBMSG 2
#define SEL_GOOGLE_PROTOBUF_FILEDESCRIPTORPROTO_ENUM_TYPE_STARTSUBMSG 3
#define SEL_GOOGLE_PROTOBUF_FILEDESCRIPTORPROTO_SERVICE_STARTSUBMSG 4
#define SEL_GOOGLE_PROTOBUF_FILEDESCRIPTORPROTO_EXTENSION_STARTSUBMSG 5
#define SEL_GOOGLE_PROTOBUF_FILEDESCRIPTORPROTO_OPTIONS_STARTSUBMSG 6
#define SEL_GOOGLE_PROTOBUF_FILEDESCRIPTORPROTO_SOURCE_CODE_INFO_STARTSUBMSG 7
#define SEL_GOOGLE_PROTOBUF_FILEDESCRIPTORPROTO_MESSAGE_TYPE_STARTSEQ 8
#define SEL_GOOGLE_PROTOBUF_FILEDESCRIPTORPROTO_MESSAGE_TYPE_ENDSEQ 9
#define SEL_GOOGLE_PROTOBUF_FILEDESCRIPTORPROTO_MESSAGE_TYPE_ENDSUBMSG 10
#define SEL_GOOGLE_PROTOBUF_FILEDESCRIPTORPROTO_ENUM_TYPE_STARTSEQ 11
#define SEL_GOOGLE_PROTOBUF_FILEDESCRIPTORPROTO_ENUM_TYPE_ENDSEQ 12
#define SEL_GOOGLE_PROTOBUF_FILEDESCRIPTORPROTO_ENUM_TYPE_ENDSUBMSG 13
#define SEL_GOOGLE_PROTOBUF_FILEDESCRIPTORPROTO_SERVICE_STARTSEQ 14
#define SEL_GOOGLE_PROTOBUF_FILEDESCRIPTORPROTO_SERVICE_ENDSEQ 15
#define SEL_GOOGLE_PROTOBUF_FILEDESCRIPTORPROTO_SERVICE_ENDSUBMSG 16
#define SEL_GOOGLE_PROTOBUF_FILEDESCRIPTORPROTO_EXTENSION_STARTSEQ 17
#define SEL_GOOGLE_PROTOBUF_FILEDESCRIPTORPROTO_EXTENSION_ENDSEQ 18
#define SEL_GOOGLE_PROTOBUF_FILEDESCRIPTORPROTO_EXTENSION_ENDSUBMSG 19
#define SEL_GOOGLE_PROTOBUF_FILEDESCRIPTORPROTO_OPTIONS_ENDSUBMSG 20
#define SEL_GOOGLE_PROTOBUF_FILEDESCRIPTORPROTO_SOURCE_CODE_INFO_ENDSUBMSG 21
#define SEL_GOOGLE_PROTOBUF_FILEDESCRIPTORPROTO_NAME_STRING 22
#define SEL_GOOGLE_PROTOBUF_FILEDESCRIPTORPROTO_NAME_STARTSTR 23
#define SEL_GOOGLE_PROTOBUF_FILEDESCRIPTORPROTO_NAME_ENDSTR 24
#define SEL_GOOGLE_PROTOBUF_FILEDESCRIPTORPROTO_PACKAGE_STRING 25
#define SEL_GOOGLE_PROTOBUF_FILEDESCRIPTORPROTO_PACKAGE_STARTSTR 26
#define SEL_GOOGLE_PROTOBUF_FILEDESCRIPTORPROTO_PACKAGE_ENDSTR 27
#define SEL_GOOGLE_PROTOBUF_FILEDESCRIPTORPROTO_DEPENDENCY_STARTSEQ 28
#define SEL_GOOGLE_PROTOBUF_FILEDESCRIPTORPROTO_DEPENDENCY_ENDSEQ 29
#define SEL_GOOGLE_PROTOBUF_FILEDESCRIPTORPROTO_DEPENDENCY_STRING 30
#define SEL_GOOGLE_PROTOBUF_FILEDESCRIPTORPROTO_DEPENDENCY_STARTSTR 31
#define SEL_GOOGLE_PROTOBUF_FILEDESCRIPTORPROTO_DEPENDENCY_ENDSTR 32
#define SEL_GOOGLE_PROTOBUF_FILEDESCRIPTORPROTO_PUBLIC_DEPENDENCY_STARTSEQ 33
#define SEL_GOOGLE_PROTOBUF_FILEDESCRIPTORPROTO_PUBLIC_DEPENDENCY_ENDSEQ 34
#define SEL_GOOGLE_PROTOBUF_FILEDESCRIPTORPROTO_PUBLIC_DEPENDENCY_INT32 35
#define SEL_GOOGLE_PROTOBUF_FILEDESCRIPTORPROTO_WEAK_DEPENDENCY_STARTSEQ 36
#define SEL_GOOGLE_PROTOBUF_FILEDESCRIPTORPROTO_WEAK_DEPENDENCY_ENDSEQ 37
#define SEL_GOOGLE_PROTOBUF_FILEDESCRIPTORPROTO_WEAK_DEPENDENCY_INT32 38

/* google.protobuf.FileDescriptorSet */
#define SEL_GOOGLE_PROTOBUF_FILEDESCRIPTORSET_FILE_STARTSUBMSG 2
#define SEL_GOOGLE_PROTOBUF_FILEDESCRIPTORSET_FILE_STARTSEQ 3
#define SEL_GOOGLE_PROTOBUF_FILEDESCRIPTORSET_FILE_ENDSEQ 4
#define SEL_GOOGLE_PROTOBUF_FILEDESCRIPTORSET_FILE_ENDSUBMSG 5

/* google.protobuf.FileOptions */
#define SEL_GOOGLE_PROTOBUF_FILEOPTIONS_UNINTERPRETED_OPTION_STARTSUBMSG 2
#define SEL_GOOGLE_PROTOBUF_FILEOPTIONS_UNINTERPRETED_OPTION_STARTSEQ 3
#define SEL_GOOGLE_PROTOBUF_FILEOPTIONS_UNINTERPRETED_OPTION_ENDSEQ 4
#define SEL_GOOGLE_PROTOBUF_FILEOPTIONS_UNINTERPRETED_OPTION_ENDSUBMSG 5
#define SEL_GOOGLE_PROTOBUF_FILEOPTIONS_JAVA_PACKAGE_STRING 6
#define SEL_GOOGLE_PROTOBUF_FILEOPTIONS_JAVA_PACKAGE_STARTSTR 7
#define SEL_GOOGLE_PROTOBUF_FILEOPTIONS_JAVA_PACKAGE_ENDSTR 8
#define SEL_GOOGLE_PROTOBUF_FILEOPTIONS_JAVA_OUTER_CLASSNAME_STRING 9
#define SEL_GOOGLE_PROTOBUF_FILEOPTIONS_JAVA_OUTER_CLASSNAME_STARTSTR 10
#define SEL_GOOGLE_PROTOBUF_FILEOPTIONS_JAVA_OUTER_CLASSNAME_ENDSTR 11
#define SEL_GOOGLE_PROTOBUF_FILEOPTIONS_OPTIMIZE_FOR_INT32 12
#define SEL_GOOGLE_PROTOBUF_FILEOPTIONS_JAVA_MULTIPLE_FILES_BOOL 13
#define SEL_GOOGLE_PROTOBUF_FILEOPTIONS_GO_PACKAGE_STRING 14
#define SEL_GOOGLE_PROTOBUF_FILEOPTIONS_GO_PACKAGE_STARTSTR 15
#define SEL_GOOGLE_PROTOBUF_FILEOPTIONS_GO_PACKAGE_ENDSTR 16
#define SEL_GOOGLE_PROTOBUF_FILEOPTIONS_CC_GENERIC_SERVICES_BOOL 17
#define SEL_GOOGLE_PROTOBUF_FILEOPTIONS_JAVA_GENERIC_SERVICES_BOOL 18
#define SEL_GOOGLE_PROTOBUF_FILEOPTIONS_PY_GENERIC_SERVICES_BOOL 19
#define SEL_GOOGLE_PROTOBUF_FILEOPTIONS_JAVA_GENERATE_EQUALS_AND_HASH_BOOL 20

/* google.protobuf.MessageOptions */
#define SEL_GOOGLE_PROTOBUF_MESSAGEOPTIONS_UNINTERPRETED_OPTION_STARTSUBMSG 2
#define SEL_GOOGLE_PROTOBUF_MESSAGEOPTIONS_UNINTERPRETED_OPTION_STARTSEQ 3
#define SEL_GOOGLE_PROTOBUF_MESSAGEOPTIONS_UNINTERPRETED_OPTION_ENDSEQ 4
#define SEL_GOOGLE_PROTOBUF_MESSAGEOPTIONS_UNINTERPRETED_OPTION_ENDSUBMSG 5
#define SEL_GOOGLE_PROTOBUF_MESSAGEOPTIONS_MESSAGE_SET_WIRE_FORMAT_BOOL 6
#define SEL_GOOGLE_PROTOBUF_MESSAGEOPTIONS_NO_STANDARD_DESCRIPTOR_ACCESSOR_BOOL 7

/* google.protobuf.MethodDescriptorProto */
#define SEL_GOOGLE_PROTOBUF_METHODDESCRIPTORPROTO_OPTIONS_STARTSUBMSG 2
#define SEL_GOOGLE_PROTOBUF_METHODDESCRIPTORPROTO_OPTIONS_ENDSUBMSG 3
#define SEL_GOOGLE_PROTOBUF_METHODDESCRIPTORPROTO_NAME_STRING 4
#define SEL_GOOGLE_PROTOBUF_METHODDESCRIPTORPROTO_NAME_STARTSTR 5
#define SEL_GOOGLE_PROTOBUF_METHODDESCRIPTORPROTO_NAME_ENDSTR 6
#define SEL_GOOGLE_PROTOBUF_METHODDESCRIPTORPROTO_INPUT_TYPE_STRING 7
#define SEL_GOOGLE_PROTOBUF_METHODDESCRIPTORPROTO_INPUT_TYPE_STARTSTR 8
#define SEL_GOOGLE_PROTOBUF_METHODDESCRIPTORPROTO_INPUT_TYPE_ENDSTR 9
#define SEL_GOOGLE_PROTOBUF_METHODDESCRIPTORPROTO_OUTPUT_TYPE_STRING 10
#define SEL_GOOGLE_PROTOBUF_METHODDESCRIPTORPROTO_OUTPUT_TYPE_STARTSTR 11
#define SEL_GOOGLE_PROTOBUF_METHODDESCRIPTORPROTO_OUTPUT_TYPE_ENDSTR 12

/* google.protobuf.MethodOptions */
#define SEL_GOOGLE_PROTOBUF_METHODOPTIONS_UNINTERPRETED_OPTION_STARTSUBMSG 2
#define SEL_GOOGLE_PROTOBUF_METHODOPTIONS_UNINTERPRETED_OPTION_STARTSEQ 3
#define SEL_GOOGLE_PROTOBUF_METHODOPTIONS_UNINTERPRETED_OPTION_ENDSEQ 4
#define SEL_GOOGLE_PROTOBUF_METHODOPTIONS_UNINTERPRETED_OPTION_ENDSUBMSG 5

/* google.protobuf.ServiceDescriptorProto */
#define SEL_GOOGLE_PROTOBUF_SERVICEDESCRIPTORPROTO_METHOD_STARTSUBMSG 2
#define SEL_GOOGLE_PROTOBUF_SERVICEDESCRIPTORPROTO_OPTIONS_STARTSUBMSG 3
#define SEL_GOOGLE_PROTOBUF_SERVICEDESCRIPTORPROTO_METHOD_STARTSEQ 4
#define SEL_GOOGLE_PROTOBUF_SERVICEDESCRIPTORPROTO_METHOD_ENDSEQ 5
#define SEL_GOOGLE_PROTOBUF_SERVICEDESCRIPTORPROTO_METHOD_ENDSUBMSG 6
#define SEL_GOOGLE_PROTOBUF_SERVICEDESCRIPTORPROTO_OPTIONS_ENDSUBMSG 7
#define SEL_GOOGLE_PROTOBUF_SERVICEDESCRIPTORPROTO_NAME_STRING 8
#define SEL_GOOGLE_PROTOBUF_SERVICEDESCRIPTORPROTO_NAME_STARTSTR 9
#define SEL_GOOGLE_PROTOBUF_SERVICEDESCRIPTORPROTO_NAME_ENDSTR 10

/* google.protobuf.ServiceOptions */
#define SEL_GOOGLE_PROTOBUF_SERVICEOPTIONS_UNINTERPRETED_OPTION_STARTSUBMSG 2
#define SEL_GOOGLE_PROTOBUF_SERVICEOPTIONS_UNINTERPRETED_OPTION_STARTSEQ 3
#define SEL_GOOGLE_PROTOBUF_SERVICEOPTIONS_UNINTERPRETED_OPTION_ENDSEQ 4
#define SEL_GOOGLE_PROTOBUF_SERVICEOPTIONS_UNINTERPRETED_OPTION_ENDSUBMSG 5

/* google.protobuf.SourceCodeInfo */
#define SEL_GOOGLE_PROTOBUF_SOURCECODEINFO_LOCATION_STARTSUBMSG 2
#define SEL_GOOGLE_PROTOBUF_SOURCECODEINFO_LOCATION_STARTSEQ 3
#define SEL_GOOGLE_PROTOBUF_SOURCECODEINFO_LOCATION_ENDSEQ 4
#define SEL_GOOGLE_PROTOBUF_SOURCECODEINFO_LOCATION_ENDSUBMSG 5

/* google.protobuf.SourceCodeInfo.Location */
#define SEL_GOOGLE_PROTOBUF_SOURCECODEINFO_LOCATION_PATH_STARTSEQ 2
#define SEL_GOOGLE_PROTOBUF_SOURCECODEINFO_LOCATION_PATH_ENDSEQ 3
#define SEL_GOOGLE_PROTOBUF_SOURCECODEINFO_LOCATION_PATH_INT32 4
#define SEL_GOOGLE_PROTOBUF_SOURCECODEINFO_LOCATION_SPAN_STARTSEQ 5
#define SEL_GOOGLE_PROTOBUF_SOURCECODEINFO_LOCATION_SPAN_ENDSEQ 6
#define SEL_GOOGLE_PROTOBUF_SOURCECODEINFO_LOCATION_SPAN_INT32 7
#define SEL_GOOGLE_PROTOBUF_SOURCECODEINFO_LOCATION_LEADING_COMMENTS_STRING 8
#define SEL_GOOGLE_PROTOBUF_SOURCECODEINFO_LOCATION_LEADING_COMMENTS_STARTSTR 9
#define SEL_GOOGLE_PROTOBUF_SOURCECODEINFO_LOCATION_LEADING_COMMENTS_ENDSTR 10
#define SEL_GOOGLE_PROTOBUF_SOURCECODEINFO_LOCATION_TRAILING_COMMENTS_STRING 11
#define SEL_GOOGLE_PROTOBUF_SOURCECODEINFO_LOCATION_TRAILING_COMMENTS_STARTSTR 12
#define SEL_GOOGLE_PROTOBUF_SOURCECODEINFO_LOCATION_TRAILING_COMMENTS_ENDSTR 13

/* google.protobuf.UninterpretedOption */
#define SEL_GOOGLE_PROTOBUF_UNINTERPRETEDOPTION_NAME_STARTSUBMSG 2
#define SEL_GOOGLE_PROTOBUF_UNINTERPRETEDOPTION_NAME_STARTSEQ 3
#define SEL_GOOGLE_PROTOBUF_UNINTERPRETEDOPTION_NAME_ENDSEQ 4
#define SEL_GOOGLE_PROTOBUF_UNINTERPRETEDOPTION_NAME_ENDSUBMSG 5
#define SEL_GOOGLE_PROTOBUF_UNINTERPRETEDOPTION_IDENTIFIER_VALUE_STRING 6
#define SEL_GOOGLE_PROTOBUF_UNINTERPRETEDOPTION_IDENTIFIER_VALUE_STARTSTR 7
#define SEL_GOOGLE_PROTOBUF_UNINTERPRETEDOPTION_IDENTIFIER_VALUE_ENDSTR 8
#define SEL_GOOGLE_PROTOBUF_UNINTERPRETEDOPTION_POSITIVE_INT_VALUE_UINT64 9
#define SEL_GOOGLE_PROTOBUF_UNINTERPRETEDOPTION_NEGATIVE_INT_VALUE_INT64 10
#define SEL_GOOGLE_PROTOBUF_UNINTERPRETEDOPTION_DOUBLE_VALUE_DOUBLE 11
#define SEL_GOOGLE_PROTOBUF_UNINTERPRETEDOPTION_STRING_VALUE_STRING 12
#define SEL_GOOGLE_PROTOBUF_UNINTERPRETEDOPTION_STRING_VALUE_STARTSTR 13
#define SEL_GOOGLE_PROTOBUF_UNINTERPRETEDOPTION_STRING_VALUE_ENDSTR 14
#define SEL_GOOGLE_PROTOBUF_UNINTERPRETEDOPTION_AGGREGATE_VALUE_STRING 15
#define SEL_GOOGLE_PROTOBUF_UNINTERPRETEDOPTION_AGGREGATE_VALUE_STARTSTR 16
#define SEL_GOOGLE_PROTOBUF_UNINTERPRETEDOPTION_AGGREGATE_VALUE_ENDSTR 17

/* google.protobuf.UninterpretedOption.NamePart */
#define SEL_GOOGLE_PROTOBUF_UNINTERPRETEDOPTION_NAMEPART_NAME_PART_STRING 2
#define SEL_GOOGLE_PROTOBUF_UNINTERPRETEDOPTION_NAMEPART_NAME_PART_STARTSTR 3
#define SEL_GOOGLE_PROTOBUF_UNINTERPRETEDOPTION_NAMEPART_NAME_PART_ENDSTR 4
#define SEL_GOOGLE_PROTOBUF_UNINTERPRETEDOPTION_NAMEPART_IS_EXTENSION_BOOL 5

const upb_symtab *upbdefs_google_protobuf_descriptor(const void *owner);

/* MessageDefs */
UPB_INLINE const upb_msgdef *upbdefs_google_protobuf_DescriptorProto(const upb_symtab *s) {
  const upb_msgdef *m = upb_symtab_lookupmsg(s, "google.protobuf.DescriptorProto");
  assert(m);
  return m;
}
UPB_INLINE const upb_msgdef *upbdefs_google_protobuf_DescriptorProto_ExtensionRange(const upb_symtab *s) {
  const upb_msgdef *m = upb_symtab_lookupmsg(s, "google.protobuf.DescriptorProto.ExtensionRange");
  assert(m);
  return m;
}
UPB_INLINE const upb_msgdef *upbdefs_google_protobuf_EnumDescriptorProto(const upb_symtab *s) {
  const upb_msgdef *m = upb_symtab_lookupmsg(s, "google.protobuf.EnumDescriptorProto");
  assert(m);
  return m;
}
UPB_INLINE const upb_msgdef *upbdefs_google_protobuf_EnumOptions(const upb_symtab *s) {
  const upb_msgdef *m = upb_symtab_lookupmsg(s, "google.protobuf.EnumOptions");
  assert(m);
  return m;
}
UPB_INLINE const upb_msgdef *upbdefs_google_protobuf_EnumValueDescriptorProto(const upb_symtab *s) {
  const upb_msgdef *m = upb_symtab_lookupmsg(s, "google.protobuf.EnumValueDescriptorProto");
  assert(m);
  return m;
}
UPB_INLINE const upb_msgdef *upbdefs_google_protobuf_EnumValueOptions(const upb_symtab *s) {
  const upb_msgdef *m = upb_symtab_lookupmsg(s, "google.protobuf.EnumValueOptions");
  assert(m);
  return m;
}
UPB_INLINE const upb_msgdef *upbdefs_google_protobuf_FieldDescriptorProto(const upb_symtab *s) {
  const upb_msgdef *m = upb_symtab_lookupmsg(s, "google.protobuf.FieldDescriptorProto");
  assert(m);
  return m;
}
UPB_INLINE const upb_msgdef *upbdefs_google_protobuf_FieldOptions(const upb_symtab *s) {
  const upb_msgdef *m = upb_symtab_lookupmsg(s, "google.protobuf.FieldOptions");
  assert(m);
  return m;
}
UPB_INLINE const upb_msgdef *upbdefs_google_protobuf_FileDescriptorProto(const upb_symtab *s) {
  const upb_msgdef *m = upb_symtab_lookupmsg(s, "google.protobuf.FileDescriptorProto");
  assert(m);
  return m;
}
UPB_INLINE const upb_msgdef *upbdefs_google_protobuf_FileDescriptorSet(const upb_symtab *s) {
  const upb_msgdef *m = upb_symtab_lookupmsg(s, "google.protobuf.FileDescriptorSet");
  assert(m);
  return m;
}
UPB_INLINE const upb_msgdef *upbdefs_google_protobuf_FileOptions(const upb_symtab *s) {
  const upb_msgdef *m = upb_symtab_lookupmsg(s, "google.protobuf.FileOptions");
  assert(m);
  return m;
}
UPB_INLINE const upb_msgdef *upbdefs_google_protobuf_MessageOptions(const upb_symtab *s) {
  const upb_msgdef *m = upb_symtab_lookupmsg(s, "google.protobuf.MessageOptions");
  assert(m);
  return m;
}
UPB_INLINE const upb_msgdef *upbdefs_google_protobuf_MethodDescriptorProto(const upb_symtab *s) {
  const upb_msgdef *m = upb_symtab_lookupmsg(s, "google.protobuf.MethodDescriptorProto");
  assert(m);
  return m;
}
UPB_INLINE const upb_msgdef *upbdefs_google_protobuf_MethodOptions(const upb_symtab *s) {
  const upb_msgdef *m = upb_symtab_lookupmsg(s, "google.protobuf.MethodOptions");
  assert(m);
  return m;
}
UPB_INLINE const upb_msgdef *upbdefs_google_protobuf_ServiceDescriptorProto(const upb_symtab *s) {
  const upb_msgdef *m = upb_symtab_lookupmsg(s, "google.protobuf.ServiceDescriptorProto");
  assert(m);
  return m;
}
UPB_INLINE const upb_msgdef *upbdefs_google_protobuf_ServiceOptions(const upb_symtab *s) {
  const upb_msgdef *m = upb_symtab_lookupmsg(s, "google.protobuf.ServiceOptions");
  assert(m);
  return m;
}
UPB_INLINE const upb_msgdef *upbdefs_google_protobuf_SourceCodeInfo(const upb_symtab *s) {
  const upb_msgdef *m = upb_symtab_lookupmsg(s, "google.protobuf.SourceCodeInfo");
  assert(m);
  return m;
}
UPB_INLINE const upb_msgdef *upbdefs_google_protobuf_SourceCodeInfo_Location(const upb_symtab *s) {
  const upb_msgdef *m = upb_symtab_lookupmsg(s, "google.protobuf.SourceCodeInfo.Location");
  assert(m);
  return m;
}
UPB_INLINE const upb_msgdef *upbdefs_google_protobuf_UninterpretedOption(const upb_symtab *s) {
  const upb_msgdef *m = upb_symtab_lookupmsg(s, "google.protobuf.UninterpretedOption");
  assert(m);
  return m;
}
UPB_INLINE const upb_msgdef *upbdefs_google_protobuf_UninterpretedOption_NamePart(const upb_symtab *s) {
  const upb_msgdef *m = upb_symtab_lookupmsg(s, "google.protobuf.UninterpretedOption.NamePart");
  assert(m);
  return m;
}


/* EnumDefs */
UPB_INLINE const upb_enumdef *upbdefs_google_protobuf_FieldDescriptorProto_Label(const upb_symtab *s) {
  const upb_enumdef *e = upb_symtab_lookupenum(s, "google.protobuf.FieldDescriptorProto.Label");
  assert(e);
  return e;
}
UPB_INLINE const upb_enumdef *upbdefs_google_protobuf_FieldDescriptorProto_Type(const upb_symtab *s) {
  const upb_enumdef *e = upb_symtab_lookupenum(s, "google.protobuf.FieldDescriptorProto.Type");
  assert(e);
  return e;
}
UPB_INLINE const upb_enumdef *upbdefs_google_protobuf_FieldOptions_CType(const upb_symtab *s) {
  const upb_enumdef *e = upb_symtab_lookupenum(s, "google.protobuf.FieldOptions.CType");
  assert(e);
  return e;
}
UPB_INLINE const upb_enumdef *upbdefs_google_protobuf_FileOptions_OptimizeMode(const upb_symtab *s) {
  const upb_enumdef *e = upb_symtab_lookupenum(s, "google.protobuf.FileOptions.OptimizeMode");
  assert(e);
  return e;
}

UPB_INLINE const upb_fielddef *upbdefs_google_protobuf_DescriptorProto_ExtensionRange_end(const upb_symtab *s) { return upb_msgdef_itof(upbdefs_google_protobuf_DescriptorProto_ExtensionRange(s), 2); }
UPB_INLINE const upb_fielddef *upbdefs_google_protobuf_DescriptorProto_ExtensionRange_start(const upb_symtab *s) { return upb_msgdef_itof(upbdefs_google_protobuf_DescriptorProto_ExtensionRange(s), 1); }
UPB_INLINE const upb_fielddef *upbdefs_google_protobuf_DescriptorProto_enum_type(const upb_symtab *s) { return upb_msgdef_itof(upbdefs_google_protobuf_DescriptorProto(s), 4); }
UPB_INLINE const upb_fielddef *upbdefs_google_protobuf_DescriptorProto_extension(const upb_symtab *s) { return upb_msgdef_itof(upbdefs_google_protobuf_DescriptorProto(s), 6); }
UPB_INLINE const upb_fielddef *upbdefs_google_protobuf_DescriptorProto_extension_range(const upb_symtab *s) { return upb_msgdef_itof(upbdefs_google_protobuf_DescriptorProto(s), 5); }
UPB_INLINE const upb_fielddef *upbdefs_google_protobuf_DescriptorProto_field(const upb_symtab *s) { return upb_msgdef_itof(upbdefs_google_protobuf_DescriptorProto(s), 2); }
UPB_INLINE const upb_fielddef *upbdefs_google_protobuf_DescriptorProto_name(const upb_symtab *s) { return upb_msgdef_itof(upbdefs_google_protobuf_DescriptorProto(s), 1); }
UPB_INLINE const upb_fielddef *upbdefs_google_protobuf_DescriptorProto_nested_type(const upb_symtab *s) { return upb_msgdef_itof(upbdefs_google_protobuf_DescriptorProto(s), 3); }
UPB_INLINE const upb_fielddef *upbdefs_google_protobuf_DescriptorProto_options(const upb_symtab *s) { return upb_msgdef_itof(upbdefs_google_protobuf_DescriptorProto(s), 7); }
UPB_INLINE const upb_fielddef *upbdefs_google_protobuf_EnumDescriptorProto_name(const upb_symtab *s) { return upb_msgdef_itof(upbdefs_google_protobuf_EnumDescriptorProto(s), 1); }
UPB_INLINE const upb_fielddef *upbdefs_google_protobuf_EnumDescriptorProto_options(const upb_symtab *s) { return upb_msgdef_itof(upbdefs_google_protobuf_EnumDescriptorProto(s), 3); }
UPB_INLINE const upb_fielddef *upbdefs_google_protobuf_EnumDescriptorProto_value(const upb_symtab *s) { return upb_msgdef_itof(upbdefs_google_protobuf_EnumDescriptorProto(s), 2); }
UPB_INLINE const upb_fielddef *upbdefs_google_protobuf_EnumOptions_allow_alias(const upb_symtab *s) { return upb_msgdef_itof(upbdefs_google_protobuf_EnumOptions(s), 2); }
UPB_INLINE const upb_fielddef *upbdefs_google_protobuf_EnumOptions_uninterpreted_option(const upb_symtab *s) { return upb_msgdef_itof(upbdefs_google_protobuf_EnumOptions(s), 999); }
UPB_INLINE const upb_fielddef *upbdefs_google_protobuf_EnumValueDescriptorProto_name(const upb_symtab *s) { return upb_msgdef_itof(upbdefs_google_protobuf_EnumValueDescriptorProto(s), 1); }
UPB_INLINE const upb_fielddef *upbdefs_google_protobuf_EnumValueDescriptorProto_number(const upb_symtab *s) { return upb_msgdef_itof(upbdefs_google_protobuf_EnumValueDescriptorProto(s), 2); }
UPB_INLINE const upb_fielddef *upbdefs_google_protobuf_EnumValueDescriptorProto_options(const upb_symtab *s) { return upb_msgdef_itof(upbdefs_google_protobuf_EnumValueDescriptorProto(s), 3); }
UPB_INLINE const upb_fielddef *upbdefs_google_protobuf_EnumValueOptions_uninterpreted_option(const upb_symtab *s) { return upb_msgdef_itof(upbdefs_google_protobuf_EnumValueOptions(s), 999); }
UPB_INLINE const upb_fielddef *upbdefs_google_protobuf_FieldDescriptorProto_default_value(const upb_symtab *s) { return upb_msgdef_itof(upbdefs_google_protobuf_FieldDescriptorProto(s), 7); }
UPB_INLINE const upb_fielddef *upbdefs_google_protobuf_FieldDescriptorProto_extendee(const upb_symtab *s) { return upb_msgdef_itof(upbdefs_google_protobuf_FieldDescriptorProto(s), 2); }
UPB_INLINE const upb_fielddef *upbdefs_google_protobuf_FieldDescriptorProto_label(const upb_symtab *s) { return upb_msgdef_itof(upbdefs_google_protobuf_FieldDescriptorProto(s), 4); }
UPB_INLINE const upb_fielddef *upbdefs_google_protobuf_FieldDescriptorProto_name(const upb_symtab *s) { return upb_msgdef_itof(upbdefs_google_protobuf_FieldDescriptorProto(s), 1); }
UPB_INLINE const upb_fielddef *upbdefs_google_protobuf_FieldDescriptorProto_number(const upb_symtab *s) { return upb_msgdef_itof(upbdefs_google_protobuf_FieldDescriptorProto(s), 3); }
UPB_INLINE const upb_fielddef *upbdefs_google_protobuf_FieldDescriptorProto_options(const upb_symtab *s) { return upb_msgdef_itof(upbdefs_google_protobuf_FieldDescriptorProto(s), 8); }
UPB_INLINE const upb_fielddef *upbdefs_google_protobuf_FieldDescriptorProto_type(const upb_symtab *s) { return upb_msgdef_itof(upbdefs_google_protobuf_FieldDescriptorProto(s), 5); }
UPB_INLINE const upb_fielddef *upbdefs_google_protobuf_FieldDescriptorProto_type_name(const upb_symtab *s) { return upb_msgdef_itof(upbdefs_google_protobuf_FieldDescriptorProto(s), 6); }
UPB_INLINE const upb_fielddef *upbdefs_google_protobuf_FieldOptions_ctype(const upb_symtab *s) { return upb_msgdef_itof(upbdefs_google_protobuf_FieldOptions(s), 1); }
UPB_INLINE const upb_fielddef *upbdefs_google_protobuf_FieldOptions_deprecated(const upb_symtab *s) { return upb_msgdef_itof(upbdefs_google_protobuf_FieldOptions(s), 3); }
UPB_INLINE const upb_fielddef *upbdefs_google_protobuf_FieldOptions_experimental_map_key(const upb_symtab *s) { return upb_msgdef_itof(upbdefs_google_protobuf_FieldOptions(s), 9); }
UPB_INLINE const upb_fielddef *upbdefs_google_protobuf_FieldOptions_lazy(const upb_symtab *s) { return upb_msgdef_itof(upbdefs_google_protobuf_FieldOptions(s), 5); }
UPB_INLINE const upb_fielddef *upbdefs_google_protobuf_FieldOptions_packed(const upb_symtab *s) { return upb_msgdef_itof(upbdefs_google_protobuf_FieldOptions(s), 2); }
UPB_INLINE const upb_fielddef *upbdefs_google_protobuf_FieldOptions_uninterpreted_option(const upb_symtab *s) { return upb_msgdef_itof(upbdefs_google_protobuf_FieldOptions(s), 999); }
UPB_INLINE const upb_fielddef *upbdefs_google_protobuf_FieldOptions_weak(const upb_symtab *s) { return upb_msgdef_itof(upbdefs_google_protobuf_FieldOptions(s), 10); }
UPB_INLINE const upb_fielddef *upbdefs_google_protobuf_FileDescriptorProto_dependency(const upb_symtab *s) { return upb_msgdef_itof(upbdefs_google_protobuf_FileDescriptorProto(s), 3); }
UPB_INLINE const upb_fielddef *upbdefs_google_protobuf_FileDescriptorProto_enum_type(const upb_symtab *s) { return upb_msgdef_itof(upbdefs_google_protobuf_FileDescriptorProto(s), 5); }
UPB_INLINE const upb_fielddef *upbdefs_google_protobuf_FileDescriptorProto_extension(const upb_symtab *s) { return upb_msgdef_itof(upbdefs_google_protobuf_FileDescriptorProto(s), 7); }
UPB_INLINE const upb_fielddef *upbdefs_google_protobuf_FileDescriptorProto_message_type(const upb_symtab *s) { return upb_msgdef_itof(upbdefs_google_protobuf_FileDescriptorProto(s), 4); }
UPB_INLINE const upb_fielddef *upbdefs_google_protobuf_FileDescriptorProto_name(const upb_symtab *s) { return upb_msgdef_itof(upbdefs_google_protobuf_FileDescriptorProto(s), 1); }
UPB_INLINE const upb_fielddef *upbdefs_google_protobuf_FileDescriptorProto_options(const upb_symtab *s) { return upb_msgdef_itof(upbdefs_google_protobuf_FileDescriptorProto(s), 8); }
UPB_INLINE const upb_fielddef *upbdefs_google_protobuf_FileDescriptorProto_package(const upb_symtab *s) { return upb_msgdef_itof(upbdefs_google_protobuf_FileDescriptorProto(s), 2); }
UPB_INLINE const upb_fielddef *upbdefs_google_protobuf_FileDescriptorProto_public_dependency(const upb_symtab *s) { return upb_msgdef_itof(upbdefs_google_protobuf_FileDescriptorProto(s), 10); }
UPB_INLINE const upb_fielddef *upbdefs_google_protobuf_FileDescriptorProto_service(const upb_symtab *s) { return upb_msgdef_itof(upbdefs_google_protobuf_FileDescriptorProto(s), 6); }
UPB_INLINE const upb_fielddef *upbdefs_google_protobuf_FileDescriptorProto_source_code_info(const upb_symtab *s) { return upb_msgdef_itof(upbdefs_google_protobuf_FileDescriptorProto(s), 9); }
UPB_INLINE const upb_fielddef *upbdefs_google_protobuf_FileDescriptorProto_weak_dependency(const upb_symtab *s) { return upb_msgdef_itof(upbdefs_google_protobuf_FileDescriptorProto(s), 11); }
UPB_INLINE const upb_fielddef *upbdefs_google_protobuf_FileDescriptorSet_file(const upb_symtab *s) { return upb_msgdef_itof(upbdefs_google_protobuf_FileDescriptorSet(s), 1); }
UPB_INLINE const upb_fielddef *upbdefs_google_protobuf_FileOptions_cc_generic_services(const upb_symtab *s) { return upb_msgdef_itof(upbdefs_google_protobuf_FileOptions(s), 16); }
UPB_INLINE const upb_fielddef *upbdefs_google_protobuf_FileOptions_go_package(const upb_symtab *s) { return upb_msgdef_itof(upbdefs_google_protobuf_FileOptions(s), 11); }
UPB_INLINE const upb_fielddef *upbdefs_google_protobuf_FileOptions_java_generate_equals_and_hash(const upb_symtab *s) { return upb_msgdef_itof(upbdefs_google_protobuf_FileOptions(s), 20); }
UPB_INLINE const upb_fielddef *upbdefs_google_protobuf_FileOptions_java_generic_services(const upb_symtab *s) { return upb_msgdef_itof(upbdefs_google_protobuf_FileOptions(s), 17); }
UPB_INLINE const upb_fielddef *upbdefs_google_protobuf_FileOptions_java_multiple_files(const upb_symtab *s) { return upb_msgdef_itof(upbdefs_google_protobuf_FileOptions(s), 10); }
UPB_INLINE const upb_fielddef *upbdefs_google_protobuf_FileOptions_java_outer_classname(const upb_symtab *s) { return upb_msgdef_itof(upbdefs_google_protobuf_FileOptions(s), 8); }
UPB_INLINE const upb_fielddef *upbdefs_google_protobuf_FileOptions_java_package(const upb_symtab *s) { return upb_msgdef_itof(upbdefs_google_protobuf_FileOptions(s), 1); }
UPB_INLINE const upb_fielddef *upbdefs_google_protobuf_FileOptions_optimize_for(const upb_symtab *s) { return upb_msgdef_itof(upbdefs_google_protobuf_FileOptions(s), 9); }
UPB_INLINE const upb_fielddef *upbdefs_google_protobuf_FileOptions_py_generic_services(const upb_symtab *s) { return upb_msgdef_itof(upbdefs_google_protobuf_FileOptions(s), 18); }
UPB_INLINE const upb_fielddef *upbdefs_google_protobuf_FileOptions_uninterpreted_option(const upb_symtab *s) { return upb_msgdef_itof(upbdefs_google_protobuf_FileOptions(s), 999); }
UPB_INLINE const upb_fielddef *upbdefs_google_protobuf_MessageOptions_message_set_wire_format(const upb_symtab *s) { return upb_msgdef_itof(upbdefs_google_protobuf_MessageOptions(s), 1); }
UPB_INLINE const upb_fielddef *upbdefs_google_protobuf_MessageOptions_no_standard_descriptor_accessor(const upb_symtab *s) { return upb_msgdef_itof(upbdefs_google_protobuf_MessageOptions(s), 2); }
UPB_INLINE const upb_fielddef *upbdefs_google_protobuf_MessageOptions_uninterpreted_option(const upb_symtab *s) { return upb_msgdef_itof(upbdefs_google_protobuf_MessageOptions(s), 999); }
UPB_INLINE const upb_fielddef *upbdefs_google_protobuf_MethodDescriptorProto_input_type(const upb_symtab *s) { return upb_msgdef_itof(upbdefs_google_protobuf_MethodDescriptorProto(s), 2); }
UPB_INLINE const upb_fielddef *upbdefs_google_protobuf_MethodDescriptorProto_name(const upb_symtab *s) { return upb_msgdef_itof(upbdefs_google_protobuf_MethodDescriptorProto(s), 1); }
UPB_INLINE const upb_fielddef *upbdefs_google_protobuf_MethodDescriptorProto_options(const upb_symtab *s) { return upb_msgdef_itof(upbdefs_google_protobuf_MethodDescriptorProto(s), 4); }
UPB_INLINE const upb_fielddef *upbdefs_google_protobuf_MethodDescriptorProto_output_type(const upb_symtab *s) { return upb_msgdef_itof(upbdefs_google_protobuf_MethodDescriptorProto(s), 3); }
UPB_INLINE const upb_fielddef *upbdefs_google_protobuf_MethodOptions_uninterpreted_option(const upb_symtab *s) { return upb_msgdef_itof(upbdefs_google_protobuf_MethodOptions(s), 999); }
UPB_INLINE const upb_fielddef *upbdefs_google_protobuf_ServiceDescriptorProto_method(const upb_symtab *s) { return upb_msgdef_itof(upbdefs_google_protobuf_ServiceDescriptorProto(s), 2); }
UPB_INLINE const upb_fielddef *upbdefs_google_protobuf_ServiceDescriptorProto_name(const upb_symtab *s) { return upb_msgdef_itof(upbdefs_google_protobuf_ServiceDescriptorProto(s), 1); }
UPB_INLINE const upb_fielddef *upbdefs_google_protobuf_ServiceDescriptorProto_options(const upb_symtab *s) { return upb_msgdef_itof(upbdefs_google_protobuf_ServiceDescriptorProto(s), 3); }
UPB_INLINE const upb_fielddef *upbdefs_google_protobuf_ServiceOptions_uninterpreted_option(const upb_symtab *s) { return upb_msgdef_itof(upbdefs_google_protobuf_ServiceOptions(s), 999); }
UPB_INLINE const upb_fielddef *upbdefs_google_protobuf_SourceCodeInfo_Location_leading_comments(const upb_symtab *s) { return upb_msgdef_itof(upbdefs_google_protobuf_SourceCodeInfo_Location(s), 3); }
UPB_INLINE const upb_fielddef *upbdefs_google_protobuf_SourceCodeInfo_Location_path(const upb_symtab *s) { return upb_msgdef_itof(upbdefs_google_protobuf_SourceCodeInfo_Location(s), 1); }
UPB_INLINE const upb_fielddef *upbdefs_google_protobuf_SourceCodeInfo_Location_span(const upb_symtab *s) { return upb_msgdef_itof(upbdefs_google_protobuf_SourceCodeInfo_Location(s), 2); }
UPB_INLINE const upb_fielddef *upbdefs_google_protobuf_SourceCodeInfo_Location_trailing_comments(const upb_symtab *s) { return upb_msgdef_itof(upbdefs_google_protobuf_SourceCodeInfo_Location(s), 4); }
UPB_INLINE const upb_fielddef *upbdefs_google_protobuf_SourceCodeInfo_location(const upb_symtab *s) { return upb_msgdef_itof(upbdefs_google_protobuf_SourceCodeInfo(s), 1); }
UPB_INLINE const upb_fielddef *upbdefs_google_protobuf_UninterpretedOption_NamePart_is_extension(const upb_symtab *s) { return upb_msgdef_itof(upbdefs_google_protobuf_UninterpretedOption_NamePart(s), 2); }
UPB_INLINE const upb_fielddef *upbdefs_google_protobuf_UninterpretedOption_NamePart_name_part(const upb_symtab *s) { return upb_msgdef_itof(upbdefs_google_protobuf_UninterpretedOption_NamePart(s), 1); }
UPB_INLINE const upb_fielddef *upbdefs_google_protobuf_UninterpretedOption_aggregate_value(const upb_symtab *s) { return upb_msgdef_itof(upbdefs_google_protobuf_UninterpretedOption(s), 8); }
UPB_INLINE const upb_fielddef *upbdefs_google_protobuf_UninterpretedOption_double_value(const upb_symtab *s) { return upb_msgdef_itof(upbdefs_google_protobuf_UninterpretedOption(s), 6); }
UPB_INLINE const upb_fielddef *upbdefs_google_protobuf_UninterpretedOption_identifier_value(const upb_symtab *s) { return upb_msgdef_itof(upbdefs_google_protobuf_UninterpretedOption(s), 3); }
UPB_INLINE const upb_fielddef *upbdefs_google_protobuf_UninterpretedOption_name(const upb_symtab *s) { return upb_msgdef_itof(upbdefs_google_protobuf_UninterpretedOption(s), 2); }
UPB_INLINE const upb_fielddef *upbdefs_google_protobuf_UninterpretedOption_negative_int_value(const upb_symtab *s) { return upb_msgdef_itof(upbdefs_google_protobuf_UninterpretedOption(s), 5); }
UPB_INLINE const upb_fielddef *upbdefs_google_protobuf_UninterpretedOption_positive_int_value(const upb_symtab *s) { return upb_msgdef_itof(upbdefs_google_protobuf_UninterpretedOption(s), 4); }
UPB_INLINE const upb_fielddef *upbdefs_google_protobuf_UninterpretedOption_string_value(const upb_symtab *s) { return upb_msgdef_itof(upbdefs_google_protobuf_UninterpretedOption(s), 7); }

UPB_END_EXTERN_C

#ifdef __cplusplus

namespace upbdefs {
namespace google {
namespace protobuf {
namespace descriptor {
inline upb::reffed_ptr<const upb::SymbolTable> SymbolTable() {
  const upb::SymbolTable* s = upbdefs_google_protobuf_descriptor(&s);
  return upb::reffed_ptr<const upb::SymbolTable>(s, &s);
}
}  /* namespace descriptor */
}  /* namespace protobuf */
}  /* namespace google */

#define RETURN_REFFED(type, func) \
    const type* obj = func(upbdefs::google::protobuf::descriptor::SymbolTable().get()); \
    return upb::reffed_ptr<const type>(obj);

namespace google {
namespace protobuf {
namespace DescriptorProto {
inline upb::reffed_ptr<const upb::MessageDef> MessageDef() { RETURN_REFFED(upb::MessageDef, upbdefs_google_protobuf_DescriptorProto) }
inline upb::reffed_ptr<const upb::FieldDef> enum_type() { RETURN_REFFED(upb::FieldDef, upbdefs_google_protobuf_DescriptorProto_enum_type) }
inline upb::reffed_ptr<const upb::FieldDef> extension() { RETURN_REFFED(upb::FieldDef, upbdefs_google_protobuf_DescriptorProto_extension) }
inline upb::reffed_ptr<const upb::FieldDef> extension_range() { RETURN_REFFED(upb::FieldDef, upbdefs_google_protobuf_DescriptorProto_extension_range) }
inline upb::reffed_ptr<const upb::FieldDef> field() { RETURN_REFFED(upb::FieldDef, upbdefs_google_protobuf_DescriptorProto_field) }
inline upb::reffed_ptr<const upb::FieldDef> name() { RETURN_REFFED(upb::FieldDef, upbdefs_google_protobuf_DescriptorProto_name) }
inline upb::reffed_ptr<const upb::FieldDef> nested_type() { RETURN_REFFED(upb::FieldDef, upbdefs_google_protobuf_DescriptorProto_nested_type) }
inline upb::reffed_ptr<const upb::FieldDef> options() { RETURN_REFFED(upb::FieldDef, upbdefs_google_protobuf_DescriptorProto_options) }
}  /* namespace DescriptorProto */
}  /* namespace protobuf */
}  /* namespace google */

namespace google {
namespace protobuf {
namespace DescriptorProto {
namespace ExtensionRange {
inline upb::reffed_ptr<const upb::MessageDef> MessageDef() { RETURN_REFFED(upb::MessageDef, upbdefs_google_protobuf_DescriptorProto_ExtensionRange) }
inline upb::reffed_ptr<const upb::FieldDef> end() { RETURN_REFFED(upb::FieldDef, upbdefs_google_protobuf_DescriptorProto_ExtensionRange_end) }
inline upb::reffed_ptr<const upb::FieldDef> start() { RETURN_REFFED(upb::FieldDef, upbdefs_google_protobuf_DescriptorProto_ExtensionRange_start) }
}  /* namespace ExtensionRange */
}  /* namespace DescriptorProto */
}  /* namespace protobuf */
}  /* namespace google */

namespace google {
namespace protobuf {
namespace EnumDescriptorProto {
inline upb::reffed_ptr<const upb::MessageDef> MessageDef() { RETURN_REFFED(upb::MessageDef, upbdefs_google_protobuf_EnumDescriptorProto) }
inline upb::reffed_ptr<const upb::FieldDef> name() { RETURN_REFFED(upb::FieldDef, upbdefs_google_protobuf_EnumDescriptorProto_name) }
inline upb::reffed_ptr<const upb::FieldDef> options() { RETURN_REFFED(upb::FieldDef, upbdefs_google_protobuf_EnumDescriptorProto_options) }
inline upb::reffed_ptr<const upb::FieldDef> value() { RETURN_REFFED(upb::FieldDef, upbdefs_google_protobuf_EnumDescriptorProto_value) }
}  /* namespace EnumDescriptorProto */
}  /* namespace protobuf */
}  /* namespace google */

namespace google {
namespace protobuf {
namespace EnumOptions {
inline upb::reffed_ptr<const upb::MessageDef> MessageDef() { RETURN_REFFED(upb::MessageDef, upbdefs_google_protobuf_EnumOptions) }
inline upb::reffed_ptr<const upb::FieldDef> allow_alias() { RETURN_REFFED(upb::FieldDef, upbdefs_google_protobuf_EnumOptions_allow_alias) }
inline upb::reffed_ptr<const upb::FieldDef> uninterpreted_option() { RETURN_REFFED(upb::FieldDef, upbdefs_google_protobuf_EnumOptions_uninterpreted_option) }
}  /* namespace EnumOptions */
}  /* namespace protobuf */
}  /* namespace google */

namespace google {
namespace protobuf {
namespace EnumValueDescriptorProto {
inline upb::reffed_ptr<const upb::MessageDef> MessageDef() { RETURN_REFFED(upb::MessageDef, upbdefs_google_protobuf_EnumValueDescriptorProto) }
inline upb::reffed_ptr<const upb::FieldDef> name() { RETURN_REFFED(upb::FieldDef, upbdefs_google_protobuf_EnumValueDescriptorProto_name) }
inline upb::reffed_ptr<const upb::FieldDef> number() { RETURN_REFFED(upb::FieldDef, upbdefs_google_protobuf_EnumValueDescriptorProto_number) }
inline upb::reffed_ptr<const upb::FieldDef> options() { RETURN_REFFED(upb::FieldDef, upbdefs_google_protobuf_EnumValueDescriptorProto_options) }
}  /* namespace EnumValueDescriptorProto */
}  /* namespace protobuf */
}  /* namespace google */

namespace google {
namespace protobuf {
namespace EnumValueOptions {
inline upb::reffed_ptr<const upb::MessageDef> MessageDef() { RETURN_REFFED(upb::MessageDef, upbdefs_google_protobuf_EnumValueOptions) }
inline upb::reffed_ptr<const upb::FieldDef> uninterpreted_option() { RETURN_REFFED(upb::FieldDef, upbdefs_google_protobuf_EnumValueOptions_uninterpreted_option) }
}  /* namespace EnumValueOptions */
}  /* namespace protobuf */
}  /* namespace google */

namespace google {
namespace protobuf {
namespace FieldDescriptorProto {
inline upb::reffed_ptr<const upb::MessageDef> MessageDef() { RETURN_REFFED(upb::MessageDef, upbdefs_google_protobuf_FieldDescriptorProto) }
inline upb::reffed_ptr<const upb::FieldDef> default_value() { RETURN_REFFED(upb::FieldDef, upbdefs_google_protobuf_FieldDescriptorProto_default_value) }
inline upb::reffed_ptr<const upb::FieldDef> extendee() { RETURN_REFFED(upb::FieldDef, upbdefs_google_protobuf_FieldDescriptorProto_extendee) }
inline upb::reffed_ptr<const upb::FieldDef> label() { RETURN_REFFED(upb::FieldDef, upbdefs_google_protobuf_FieldDescriptorProto_label) }
inline upb::reffed_ptr<const upb::FieldDef> name() { RETURN_REFFED(upb::FieldDef, upbdefs_google_protobuf_FieldDescriptorProto_name) }
inline upb::reffed_ptr<const upb::FieldDef> number() { RETURN_REFFED(upb::FieldDef, upbdefs_google_protobuf_FieldDescriptorProto_number) }
inline upb::reffed_ptr<const upb::FieldDef> options() { RETURN_REFFED(upb::FieldDef, upbdefs_google_protobuf_FieldDescriptorProto_options) }
inline upb::reffed_ptr<const upb::FieldDef> type() { RETURN_REFFED(upb::FieldDef, upbdefs_google_protobuf_FieldDescriptorProto_type) }
inline upb::reffed_ptr<const upb::FieldDef> type_name() { RETURN_REFFED(upb::FieldDef, upbdefs_google_protobuf_FieldDescriptorProto_type_name) }
inline upb::reffed_ptr<const upb::EnumDef> Label() { RETURN_REFFED(upb::EnumDef, upbdefs_google_protobuf_FieldDescriptorProto_Label) }
inline upb::reffed_ptr<const upb::EnumDef> Type() { RETURN_REFFED(upb::EnumDef, upbdefs_google_protobuf_FieldDescriptorProto_Type) }
}  /* namespace FieldDescriptorProto */
}  /* namespace protobuf */
}  /* namespace google */

namespace google {
namespace protobuf {
namespace FieldOptions {
inline upb::reffed_ptr<const upb::MessageDef> MessageDef() { RETURN_REFFED(upb::MessageDef, upbdefs_google_protobuf_FieldOptions) }
inline upb::reffed_ptr<const upb::FieldDef> ctype() { RETURN_REFFED(upb::FieldDef, upbdefs_google_protobuf_FieldOptions_ctype) }
inline upb::reffed_ptr<const upb::FieldDef> deprecated() { RETURN_REFFED(upb::FieldDef, upbdefs_google_protobuf_FieldOptions_deprecated) }
inline upb::reffed_ptr<const upb::FieldDef> experimental_map_key() { RETURN_REFFED(upb::FieldDef, upbdefs_google_protobuf_FieldOptions_experimental_map_key) }
inline upb::reffed_ptr<const upb::FieldDef> lazy() { RETURN_REFFED(upb::FieldDef, upbdefs_google_protobuf_FieldOptions_lazy) }
inline upb::reffed_ptr<const upb::FieldDef> packed() { RETURN_REFFED(upb::FieldDef, upbdefs_google_protobuf_FieldOptions_packed) }
inline upb::reffed_ptr<const upb::FieldDef> uninterpreted_option() { RETURN_REFFED(upb::FieldDef, upbdefs_google_protobuf_FieldOptions_uninterpreted_option) }
inline upb::reffed_ptr<const upb::FieldDef> weak() { RETURN_REFFED(upb::FieldDef, upbdefs_google_protobuf_FieldOptions_weak) }
inline upb::reffed_ptr<const upb::EnumDef> CType() { RETURN_REFFED(upb::EnumDef, upbdefs_google_protobuf_FieldOptions_CType) }
}  /* namespace FieldOptions */
}  /* namespace protobuf */
}  /* namespace google */

namespace google {
namespace protobuf {
namespace FileDescriptorProto {
inline upb::reffed_ptr<const upb::MessageDef> MessageDef() { RETURN_REFFED(upb::MessageDef, upbdefs_google_protobuf_FileDescriptorProto) }
inline upb::reffed_ptr<const upb::FieldDef> dependency() { RETURN_REFFED(upb::FieldDef, upbdefs_google_protobuf_FileDescriptorProto_dependency) }
inline upb::reffed_ptr<const upb::FieldDef> enum_type() { RETURN_REFFED(upb::FieldDef, upbdefs_google_protobuf_FileDescriptorProto_enum_type) }
inline upb::reffed_ptr<const upb::FieldDef> extension() { RETURN_REFFED(upb::FieldDef, upbdefs_google_protobuf_FileDescriptorProto_extension) }
inline upb::reffed_ptr<const upb::FieldDef> message_type() { RETURN_REFFED(upb::FieldDef, upbdefs_google_protobuf_FileDescriptorProto_message_type) }
inline upb::reffed_ptr<const upb::FieldDef> name() { RETURN_REFFED(upb::FieldDef, upbdefs_google_protobuf_FileDescriptorProto_name) }
inline upb::reffed_ptr<const upb::FieldDef> options() { RETURN_REFFED(upb::FieldDef, upbdefs_google_protobuf_FileDescriptorProto_options) }
inline upb::reffed_ptr<const upb::FieldDef> package() { RETURN_REFFED(upb::FieldDef, upbdefs_google_protobuf_FileDescriptorProto_package) }
inline upb::reffed_ptr<const upb::FieldDef> public_dependency() { RETURN_REFFED(upb::FieldDef, upbdefs_google_protobuf_FileDescriptorProto_public_dependency) }
inline upb::reffed_ptr<const upb::FieldDef> service() { RETURN_REFFED(upb::FieldDef, upbdefs_google_protobuf_FileDescriptorProto_service) }
inline upb::reffed_ptr<const upb::FieldDef> source_code_info() { RETURN_REFFED(upb::FieldDef, upbdefs_google_protobuf_FileDescriptorProto_source_code_info) }
inline upb::reffed_ptr<const upb::FieldDef> weak_dependency() { RETURN_REFFED(upb::FieldDef, upbdefs_google_protobuf_FileDescriptorProto_weak_dependency) }
}  /* namespace FileDescriptorProto */
}  /* namespace protobuf */
}  /* namespace google */

namespace google {
namespace protobuf {
namespace FileDescriptorSet {
inline upb::reffed_ptr<const upb::MessageDef> MessageDef() { RETURN_REFFED(upb::MessageDef, upbdefs_google_protobuf_FileDescriptorSet) }
inline upb::reffed_ptr<const upb::FieldDef> file() { RETURN_REFFED(upb::FieldDef, upbdefs_google_protobuf_FileDescriptorSet_file) }
}  /* namespace FileDescriptorSet */
}  /* namespace protobuf */
}  /* namespace google */

namespace google {
namespace protobuf {
namespace FileOptions {
inline upb::reffed_ptr<const upb::MessageDef> MessageDef() { RETURN_REFFED(upb::MessageDef, upbdefs_google_protobuf_FileOptions) }
inline upb::reffed_ptr<const upb::FieldDef> cc_generic_services() { RETURN_REFFED(upb::FieldDef, upbdefs_google_protobuf_FileOptions_cc_generic_services) }
inline upb::reffed_ptr<const upb::FieldDef> go_package() { RETURN_REFFED(upb::FieldDef, upbdefs_google_protobuf_FileOptions_go_package) }
inline upb::reffed_ptr<const upb::FieldDef> java_generate_equals_and_hash() { RETURN_REFFED(upb::FieldDef, upbdefs_google_protobuf_FileOptions_java_generate_equals_and_hash) }
inline upb::reffed_ptr<const upb::FieldDef> java_generic_services() { RETURN_REFFED(upb::FieldDef, upbdefs_google_protobuf_FileOptions_java_generic_services) }
inline upb::reffed_ptr<const upb::FieldDef> java_multiple_files() { RETURN_REFFED(upb::FieldDef, upbdefs_google_protobuf_FileOptions_java_multiple_files) }
inline upb::reffed_ptr<const upb::FieldDef> java_outer_classname() { RETURN_REFFED(upb::FieldDef, upbdefs_google_protobuf_FileOptions_java_outer_classname) }
inline upb::reffed_ptr<const upb::FieldDef> java_package() { RETURN_REFFED(upb::FieldDef, upbdefs_google_protobuf_FileOptions_java_package) }
inline upb::reffed_ptr<const upb::FieldDef> optimize_for() { RETURN_REFFED(upb::FieldDef, upbdefs_google_protobuf_FileOptions_optimize_for) }
inline upb::reffed_ptr<const upb::FieldDef> py_generic_services() { RETURN_REFFED(upb::FieldDef, upbdefs_google_protobuf_FileOptions_py_generic_services) }
inline upb::reffed_ptr<const upb::FieldDef> uninterpreted_option() { RETURN_REFFED(upb::FieldDef, upbdefs_google_protobuf_FileOptions_uninterpreted_option) }
inline upb::reffed_ptr<const upb::EnumDef> OptimizeMode() { RETURN_REFFED(upb::EnumDef, upbdefs_google_protobuf_FileOptions_OptimizeMode) }
}  /* namespace FileOptions */
}  /* namespace protobuf */
}  /* namespace google */

namespace google {
namespace protobuf {
namespace MessageOptions {
inline upb::reffed_ptr<const upb::MessageDef> MessageDef() { RETURN_REFFED(upb::MessageDef, upbdefs_google_protobuf_MessageOptions) }
inline upb::reffed_ptr<const upb::FieldDef> message_set_wire_format() { RETURN_REFFED(upb::FieldDef, upbdefs_google_protobuf_MessageOptions_message_set_wire_format) }
inline upb::reffed_ptr<const upb::FieldDef> no_standard_descriptor_accessor() { RETURN_REFFED(upb::FieldDef, upbdefs_google_protobuf_MessageOptions_no_standard_descriptor_accessor) }
inline upb::reffed_ptr<const upb::FieldDef> uninterpreted_option() { RETURN_REFFED(upb::FieldDef, upbdefs_google_protobuf_MessageOptions_uninterpreted_option) }
}  /* namespace MessageOptions */
}  /* namespace protobuf */
}  /* namespace google */

namespace google {
namespace protobuf {
namespace MethodDescriptorProto {
inline upb::reffed_ptr<const upb::MessageDef> MessageDef() { RETURN_REFFED(upb::MessageDef, upbdefs_google_protobuf_MethodDescriptorProto) }
inline upb::reffed_ptr<const upb::FieldDef> input_type() { RETURN_REFFED(upb::FieldDef, upbdefs_google_protobuf_MethodDescriptorProto_input_type) }
inline upb::reffed_ptr<const upb::FieldDef> name() { RETURN_REFFED(upb::FieldDef, upbdefs_google_protobuf_MethodDescriptorProto_name) }
inline upb::reffed_ptr<const upb::FieldDef> options() { RETURN_REFFED(upb::FieldDef, upbdefs_google_protobuf_MethodDescriptorProto_options) }
inline upb::reffed_ptr<const upb::FieldDef> output_type() { RETURN_REFFED(upb::FieldDef, upbdefs_google_protobuf_MethodDescriptorProto_output_type) }
}  /* namespace MethodDescriptorProto */
}  /* namespace protobuf */
}  /* namespace google */

namespace google {
namespace protobuf {
namespace MethodOptions {
inline upb::reffed_ptr<const upb::MessageDef> MessageDef() { RETURN_REFFED(upb::MessageDef, upbdefs_google_protobuf_MethodOptions) }
inline upb::reffed_ptr<const upb::FieldDef> uninterpreted_option() { RETURN_REFFED(upb::FieldDef, upbdefs_google_protobuf_MethodOptions_uninterpreted_option) }
}  /* namespace MethodOptions */
}  /* namespace protobuf */
}  /* namespace google */

namespace google {
namespace protobuf {
namespace ServiceDescriptorProto {
inline upb::reffed_ptr<const upb::MessageDef> MessageDef() { RETURN_REFFED(upb::MessageDef, upbdefs_google_protobuf_ServiceDescriptorProto) }
inline upb::reffed_ptr<const upb::FieldDef> method() { RETURN_REFFED(upb::FieldDef, upbdefs_google_protobuf_ServiceDescriptorProto_method) }
inline upb::reffed_ptr<const upb::FieldDef> name() { RETURN_REFFED(upb::FieldDef, upbdefs_google_protobuf_ServiceDescriptorProto_name) }
inline upb::reffed_ptr<const upb::FieldDef> options() { RETURN_REFFED(upb::FieldDef, upbdefs_google_protobuf_ServiceDescriptorProto_options) }
}  /* namespace ServiceDescriptorProto */
}  /* namespace protobuf */
}  /* namespace google */

namespace google {
namespace protobuf {
namespace ServiceOptions {
inline upb::reffed_ptr<const upb::MessageDef> MessageDef() { RETURN_REFFED(upb::MessageDef, upbdefs_google_protobuf_ServiceOptions) }
inline upb::reffed_ptr<const upb::FieldDef> uninterpreted_option() { RETURN_REFFED(upb::FieldDef, upbdefs_google_protobuf_ServiceOptions_uninterpreted_option) }
}  /* namespace ServiceOptions */
}  /* namespace protobuf */
}  /* namespace google */

namespace google {
namespace protobuf {
namespace SourceCodeInfo {
inline upb::reffed_ptr<const upb::MessageDef> MessageDef() { RETURN_REFFED(upb::MessageDef, upbdefs_google_protobuf_SourceCodeInfo) }
inline upb::reffed_ptr<const upb::FieldDef> location() { RETURN_REFFED(upb::FieldDef, upbdefs_google_protobuf_SourceCodeInfo_location) }
}  /* namespace SourceCodeInfo */
}  /* namespace protobuf */
}  /* namespace google */

namespace google {
namespace protobuf {
namespace SourceCodeInfo {
namespace Location {
inline upb::reffed_ptr<const upb::MessageDef> MessageDef() { RETURN_REFFED(upb::MessageDef, upbdefs_google_protobuf_SourceCodeInfo_Location) }
inline upb::reffed_ptr<const upb::FieldDef> leading_comments() { RETURN_REFFED(upb::FieldDef, upbdefs_google_protobuf_SourceCodeInfo_Location_leading_comments) }
inline upb::reffed_ptr<const upb::FieldDef> path() { RETURN_REFFED(upb::FieldDef, upbdefs_google_protobuf_SourceCodeInfo_Location_path) }
inline upb::reffed_ptr<const upb::FieldDef> span() { RETURN_REFFED(upb::FieldDef, upbdefs_google_protobuf_SourceCodeInfo_Location_span) }
inline upb::reffed_ptr<const upb::FieldDef> trailing_comments() { RETURN_REFFED(upb::FieldDef, upbdefs_google_protobuf_SourceCodeInfo_Location_trailing_comments) }
}  /* namespace Location */
}  /* namespace SourceCodeInfo */
}  /* namespace protobuf */
}  /* namespace google */

namespace google {
namespace protobuf {
namespace UninterpretedOption {
inline upb::reffed_ptr<const upb::MessageDef> MessageDef() { RETURN_REFFED(upb::MessageDef, upbdefs_google_protobuf_UninterpretedOption) }
inline upb::reffed_ptr<const upb::FieldDef> aggregate_value() { RETURN_REFFED(upb::FieldDef, upbdefs_google_protobuf_UninterpretedOption_aggregate_value) }
inline upb::reffed_ptr<const upb::FieldDef> double_value() { RETURN_REFFED(upb::FieldDef, upbdefs_google_protobuf_UninterpretedOption_double_value) }
inline upb::reffed_ptr<const upb::FieldDef> identifier_value() { RETURN_REFFED(upb::FieldDef, upbdefs_google_protobuf_UninterpretedOption_identifier_value) }
inline upb::reffed_ptr<const upb::FieldDef> name() { RETURN_REFFED(upb::FieldDef, upbdefs_google_protobuf_UninterpretedOption_name) }
inline upb::reffed_ptr<const upb::FieldDef> negative_int_value() { RETURN_REFFED(upb::FieldDef, upbdefs_google_protobuf_UninterpretedOption_negative_int_value) }
inline upb::reffed_ptr<const upb::FieldDef> positive_int_value() { RETURN_REFFED(upb::FieldDef, upbdefs_google_protobuf_UninterpretedOption_positive_int_value) }
inline upb::reffed_ptr<const upb::FieldDef> string_value() { RETURN_REFFED(upb::FieldDef, upbdefs_google_protobuf_UninterpretedOption_string_value) }
}  /* namespace UninterpretedOption */
}  /* namespace protobuf */
}  /* namespace google */

namespace google {
namespace protobuf {
namespace UninterpretedOption {
namespace NamePart {
inline upb::reffed_ptr<const upb::MessageDef> MessageDef() { RETURN_REFFED(upb::MessageDef, upbdefs_google_protobuf_UninterpretedOption_NamePart) }
inline upb::reffed_ptr<const upb::FieldDef> is_extension() { RETURN_REFFED(upb::FieldDef, upbdefs_google_protobuf_UninterpretedOption_NamePart_is_extension) }
inline upb::reffed_ptr<const upb::FieldDef> name_part() { RETURN_REFFED(upb::FieldDef, upbdefs_google_protobuf_UninterpretedOption_NamePart_name_part) }
}  /* namespace NamePart */
}  /* namespace UninterpretedOption */
}  /* namespace protobuf */
}  /* namespace google */

}  /* namespace upbdefs */


#undef RETURN_REFFED
#endif /* __cplusplus */

#endif  /* GOOGLE_PROTOBUF_DESCRIPTOR_UPB_H_ */
/*
** Internal-only definitions for the decoder.
*/

#ifndef UPB_DECODER_INT_H_
#define UPB_DECODER_INT_H_

#include <stdlib.h>
/*
** upb::pb::Decoder
**
** A high performance, streaming, resumable decoder for the binary protobuf
** format.
**
** This interface works the same regardless of what decoder backend is being
** used.  A client of this class does not need to know whether decoding is using
** a JITted decoder (DynASM, LLVM, etc) or an interpreted decoder.  By default,
** it will always use the fastest available decoder.  However, you can call
** set_allow_jit(false) to disable any JIT decoder that might be available.
** This is primarily useful for testing purposes.
*/

#ifndef UPB_DECODER_H_
#define UPB_DECODER_H_


#ifdef __cplusplus
namespace upb {
namespace pb {
class CodeCache;
class Decoder;
class DecoderMethod;
class DecoderMethodOptions;
}  /* namespace pb */
}  /* namespace upb */
#endif

UPB_DECLARE_TYPE(upb::pb::CodeCache, upb_pbcodecache)
UPB_DECLARE_TYPE(upb::pb::Decoder, upb_pbdecoder)
UPB_DECLARE_TYPE(upb::pb::DecoderMethodOptions, upb_pbdecodermethodopts)

UPB_DECLARE_DERIVED_TYPE(upb::pb::DecoderMethod, upb::RefCounted,
                         upb_pbdecodermethod, upb_refcounted)

#ifdef __cplusplus

/* The parameters one uses to construct a DecoderMethod.
 * TODO(haberman): move allowjit here?  Seems more convenient for users.
 * TODO(haberman): move this to be heap allocated for ABI stability. */
class upb::pb::DecoderMethodOptions {
 public:
  /* Parameter represents the destination handlers that this method will push
   * to. */
  explicit DecoderMethodOptions(const Handlers* dest_handlers);

  /* Should the decoder push submessages to lazy handlers for fields that have
   * them?  The caller should set this iff the lazy handlers expect data that is
   * in protobuf binary format and the caller wishes to lazy parse it. */
  void set_lazy(bool lazy);
#else
struct upb_pbdecodermethodopts {
#endif
  const upb_handlers *handlers;
  bool lazy;
};

#ifdef __cplusplus

/* Represents the code to parse a protobuf according to a destination
 * Handlers. */
class upb::pb::DecoderMethod {
 public:
  /* Include base methods from upb::ReferenceCounted. */
  UPB_REFCOUNTED_CPPMETHODS

  /* The destination handlers that are statically bound to this method.
   * This method is only capable of outputting to a sink that uses these
   * handlers. */
  const Handlers* dest_handlers() const;

  /* The input handlers for this decoder method. */
  const BytesHandler* input_handler() const;

  /* Whether this method is native. */
  bool is_native() const;

  /* Convenience method for generating a DecoderMethod without explicitly
   * creating a CodeCache. */
  static reffed_ptr<const DecoderMethod> New(const DecoderMethodOptions& opts);

 private:
  UPB_DISALLOW_POD_OPS(DecoderMethod, upb::pb::DecoderMethod)
};

#endif

/* Preallocation hint: decoder won't allocate more bytes than this when first
 * constructed.  This hint may be an overestimate for some build configurations.
 * But if the decoder library is upgraded without recompiling the application,
 * it may be an underestimate. */
#define UPB_PB_DECODER_SIZE 4408

#ifdef __cplusplus

/* A Decoder receives binary protobuf data on its input sink and pushes the
 * decoded data to its output sink. */
class upb::pb::Decoder {
 public:
  /* Constructs a decoder instance for the given method, which must outlive this
   * decoder.  Any errors during parsing will be set on the given status, which
   * must also outlive this decoder.
   *
   * The sink must match the given method. */
  static Decoder* Create(Environment* env, const DecoderMethod* method,
                         Sink* output);

  /* Returns the DecoderMethod this decoder is parsing from. */
  const DecoderMethod* method() const;

  /* The sink on which this decoder receives input. */
  BytesSink* input();

  /* Returns number of bytes successfully parsed.
   *
   * This can be useful for determining the stream position where an error
   * occurred.
   *
   * This value may not be up-to-date when called from inside a parsing
   * callback. */
  uint64_t BytesParsed() const;

  /* Gets/sets the parsing nexting limit.  If the total number of nested
   * submessages and repeated fields hits this limit, parsing will fail.  This
   * is a resource limit that controls the amount of memory used by the parsing
   * stack.
   *
   * Setting the limit will fail if the parser is currently suspended at a depth
   * greater than this, or if memory allocation of the stack fails. */
  size_t max_nesting() const;
  bool set_max_nesting(size_t max);

  void Reset();

  static const size_t kSize = UPB_PB_DECODER_SIZE;

 private:
  UPB_DISALLOW_POD_OPS(Decoder, upb::pb::Decoder)
};

#endif  /* __cplusplus */

#ifdef __cplusplus

/* A class for caching protobuf processing code, whether bytecode for the
 * interpreted decoder or machine code for the JIT.
 *
 * This class is not thread-safe.
 *
 * TODO(haberman): move this to be heap allocated for ABI stability. */
class upb::pb::CodeCache {
 public:
  CodeCache();
  ~CodeCache();

  /* Whether the cache is allowed to generate machine code.  Defaults to true.
   * There is no real reason to turn it off except for testing or if you are
   * having a specific problem with the JIT.
   *
   * Note that allow_jit = true does not *guarantee* that the code will be JIT
   * compiled.  If this platform is not supported or the JIT was not compiled
   * in, the code may still be interpreted. */
  bool allow_jit() const;

  /* This may only be called when the object is first constructed, and prior to
   * any code generation, otherwise returns false and does nothing. */
  bool set_allow_jit(bool allow);

  /* Returns a DecoderMethod that can push data to the given handlers.
   * If a suitable method already exists, it will be returned from the cache.
   *
   * Specifying the destination handlers here allows the DecoderMethod to be
   * statically bound to the destination handlers if possible, which can allow
   * more efficient decoding.  However the returned method may or may not
   * actually be statically bound.  But in all cases, the returned method can
   * push data to the given handlers. */
  const DecoderMethod *GetDecoderMethod(const DecoderMethodOptions& opts);

  /* If/when someone needs to explicitly create a dynamically-bound
   * DecoderMethod*, we can add a method to get it here. */

 private:
  UPB_DISALLOW_COPY_AND_ASSIGN(CodeCache)
#else
struct upb_pbcodecache {
#endif
  bool allow_jit_;

  /* Array of mgroups. */
  upb_inttable groups;
};

UPB_BEGIN_EXTERN_C

upb_pbdecoder *upb_pbdecoder_create(upb_env *e,
                                    const upb_pbdecodermethod *method,
                                    upb_sink *output);
const upb_pbdecodermethod *upb_pbdecoder_method(const upb_pbdecoder *d);
upb_bytessink *upb_pbdecoder_input(upb_pbdecoder *d);
uint64_t upb_pbdecoder_bytesparsed(const upb_pbdecoder *d);
size_t upb_pbdecoder_maxnesting(const upb_pbdecoder *d);
bool upb_pbdecoder_setmaxnesting(upb_pbdecoder *d, size_t max);
void upb_pbdecoder_reset(upb_pbdecoder *d);

void upb_pbdecodermethodopts_init(upb_pbdecodermethodopts *opts,
                                  const upb_handlers *h);
void upb_pbdecodermethodopts_setlazy(upb_pbdecodermethodopts *opts, bool lazy);


/* Include refcounted methods like upb_pbdecodermethod_ref(). */
UPB_REFCOUNTED_CMETHODS(upb_pbdecodermethod, upb_pbdecodermethod_upcast)

const upb_handlers *upb_pbdecodermethod_desthandlers(
    const upb_pbdecodermethod *m);
const upb_byteshandler *upb_pbdecodermethod_inputhandler(
    const upb_pbdecodermethod *m);
bool upb_pbdecodermethod_isnative(const upb_pbdecodermethod *m);
const upb_pbdecodermethod *upb_pbdecodermethod_new(
    const upb_pbdecodermethodopts *opts, const void *owner);

void upb_pbcodecache_init(upb_pbcodecache *c);
void upb_pbcodecache_uninit(upb_pbcodecache *c);
bool upb_pbcodecache_allowjit(const upb_pbcodecache *c);
bool upb_pbcodecache_setallowjit(upb_pbcodecache *c, bool allow);
const upb_pbdecodermethod *upb_pbcodecache_getdecodermethod(
    upb_pbcodecache *c, const upb_pbdecodermethodopts *opts);

UPB_END_EXTERN_C

#ifdef __cplusplus

namespace upb {

namespace pb {

/* static */
inline Decoder* Decoder::Create(Environment* env, const DecoderMethod* m,
                                Sink* sink) {
  return upb_pbdecoder_create(env, m, sink);
}
inline const DecoderMethod* Decoder::method() const {
  return upb_pbdecoder_method(this);
}
inline BytesSink* Decoder::input() {
  return upb_pbdecoder_input(this);
}
inline uint64_t Decoder::BytesParsed() const {
  return upb_pbdecoder_bytesparsed(this);
}
inline size_t Decoder::max_nesting() const {
  return upb_pbdecoder_maxnesting(this);
}
inline bool Decoder::set_max_nesting(size_t max) {
  return upb_pbdecoder_setmaxnesting(this, max);
}
inline void Decoder::Reset() { upb_pbdecoder_reset(this); }

inline DecoderMethodOptions::DecoderMethodOptions(const Handlers* h) {
  upb_pbdecodermethodopts_init(this, h);
}
inline void DecoderMethodOptions::set_lazy(bool lazy) {
  upb_pbdecodermethodopts_setlazy(this, lazy);
}

inline const Handlers* DecoderMethod::dest_handlers() const {
  return upb_pbdecodermethod_desthandlers(this);
}
inline const BytesHandler* DecoderMethod::input_handler() const {
  return upb_pbdecodermethod_inputhandler(this);
}
inline bool DecoderMethod::is_native() const {
  return upb_pbdecodermethod_isnative(this);
}
/* static */
inline reffed_ptr<const DecoderMethod> DecoderMethod::New(
    const DecoderMethodOptions &opts) {
  const upb_pbdecodermethod *m = upb_pbdecodermethod_new(&opts, &m);
  return reffed_ptr<const DecoderMethod>(m, &m);
}

inline CodeCache::CodeCache() {
  upb_pbcodecache_init(this);
}
inline CodeCache::~CodeCache() {
  upb_pbcodecache_uninit(this);
}
inline bool CodeCache::allow_jit() const {
  return upb_pbcodecache_allowjit(this);
}
inline bool CodeCache::set_allow_jit(bool allow) {
  return upb_pbcodecache_setallowjit(this, allow);
}
inline const DecoderMethod *CodeCache::GetDecoderMethod(
    const DecoderMethodOptions& opts) {
  return upb_pbcodecache_getdecodermethod(this, &opts);
}

}  /* namespace pb */
}  /* namespace upb */

#endif  /* __cplusplus */

#endif  /* UPB_DECODER_H_ */

/* C++ names are not actually used since this type isn't exposed to users. */
#ifdef __cplusplus
namespace upb {
namespace pb {
class MessageGroup;
}  /* namespace pb */
}  /* namespace upb */
#endif
UPB_DECLARE_DERIVED_TYPE(upb::pb::MessageGroup, upb::RefCounted,
                         mgroup, upb_refcounted)

/* Opcode definitions.  The canonical meaning of each opcode is its
 * implementation in the interpreter (the JIT is written to match this).
 *
 * All instructions have the opcode in the low byte.
 * Instruction format for most instructions is:
 *
 * +-------------------+--------+
 * |     arg (24)      | op (8) |
 * +-------------------+--------+
 *
 * Exceptions are indicated below.  A few opcodes are multi-word. */
typedef enum {
  /* Opcodes 1-8, 13, 15-18 parse their respective descriptor types.
   * Arg for all of these is the upb selector for this field. */
#define T(type) OP_PARSE_ ## type = UPB_DESCRIPTOR_TYPE_ ## type
  T(DOUBLE), T(FLOAT), T(INT64), T(UINT64), T(INT32), T(FIXED64), T(FIXED32),
  T(BOOL), T(UINT32), T(SFIXED32), T(SFIXED64), T(SINT32), T(SINT64),
#undef T
  OP_STARTMSG       = 9,   /* No arg. */
  OP_ENDMSG         = 10,  /* No arg. */
  OP_STARTSEQ       = 11,
  OP_ENDSEQ         = 12,
  OP_STARTSUBMSG    = 14,
  OP_ENDSUBMSG      = 19,
  OP_STARTSTR       = 20,
  OP_STRING         = 21,
  OP_ENDSTR         = 22,

  OP_PUSHTAGDELIM   = 23,  /* No arg. */
  OP_PUSHLENDELIM   = 24,  /* No arg. */
  OP_POP            = 25,  /* No arg. */
  OP_SETDELIM       = 26,  /* No arg. */
  OP_SETBIGGROUPNUM = 27,  /* two words:
                            *   | unused (24)     | opc (8) |
                            *   |        groupnum (32)      | */
  OP_CHECKDELIM     = 28,
  OP_CALL           = 29,
  OP_RET            = 30,
  OP_BRANCH         = 31,

  /* Different opcodes depending on how many bytes expected. */
  OP_TAG1           = 32,  /* | match tag (16) | jump target (8) | opc (8) | */
  OP_TAG2           = 33,  /* | match tag (16) | jump target (8) | opc (8) | */
  OP_TAGN           = 34,  /* three words: */
                           /*   | unused (16) | jump target(8) | opc (8) | */
                           /*   |           match tag 1 (32)             | */
                           /*   |           match tag 2 (32)             | */

  OP_SETDISPATCH    = 35,  /* N words: */
                           /*   | unused (24)         | opc | */
                           /*   | upb_inttable* (32 or 64)  | */

  OP_DISPATCH       = 36,  /* No arg. */

  OP_HALT           = 37   /* No arg. */
} opcode;

#define OP_MAX OP_HALT

UPB_INLINE opcode getop(uint32_t instr) { return instr & 0xff; }

/* Method group; represents a set of decoder methods that had their code
 * emitted together, and must therefore be freed together.  Immutable once
 * created.  It is possible we may want to expose this to users at some point.
 *
 * Overall ownership of Decoder objects looks like this:
 *
 *                +----------+
 *                |          | <---> DecoderMethod
 *                | method   |
 * CodeCache ---> |  group   | <---> DecoderMethod
 *                |          |
 *                | (mgroup) | <---> DecoderMethod
 *                +----------+
 */
struct mgroup {
  upb_refcounted base;

  /* Maps upb_msgdef/upb_handlers -> upb_pbdecodermethod.  We own refs on the
   * methods. */
  upb_inttable methods;

  /* When we add the ability to link to previously existing mgroups, we'll
   * need an array of mgroups we reference here, and own refs on them. */

  /* The bytecode for our methods, if any exists.  Owned by us. */
  uint32_t *bytecode;
  uint32_t *bytecode_end;

#ifdef UPB_USE_JIT_X64
  /* JIT-generated machine code, if any. */
  upb_string_handlerfunc *jit_code;
  /* The size of the jit_code (required to munmap()). */
  size_t jit_size;
  char *debug_info;
  void *dl;
#endif
};

/* The maximum that any submessages can be nested.  Matches proto2's limit.
 * This specifies the size of the decoder's statically-sized array and therefore
 * setting it high will cause the upb::pb::Decoder object to be larger.
 *
 * If necessary we can add a runtime-settable property to Decoder that allow
 * this to be larger than the compile-time setting, but this would add
 * complexity, particularly since we would have to decide how/if to give users
 * the ability to set a custom memory allocation function. */
#define UPB_DECODER_MAX_NESTING 64

/* Internal-only struct used by the decoder. */
typedef struct {
  /* Space optimization note: we store two pointers here that the JIT
   * doesn't need at all; the upb_handlers* inside the sink and
   * the dispatch table pointer.  We can optimze so that the JIT uses
   * smaller stack frames than the interpreter.  The only thing we need
   * to guarantee is that the fallback routines can find end_ofs. */
  upb_sink sink;

  /* The absolute stream offset of the end-of-frame delimiter.
   * Non-delimited frames (groups and non-packed repeated fields) reuse the
   * delimiter of their parent, even though the frame may not end there.
   *
   * NOTE: the JIT stores a slightly different value here for non-top frames.
   * It stores the value relative to the end of the enclosed message.  But the
   * top frame is still stored the same way, which is important for ensuring
   * that calls from the JIT into C work correctly. */
  uint64_t end_ofs;
  const uint32_t *base;

  /* 0 indicates a length-delimited field.
   * A positive number indicates a known group.
   * A negative number indicates an unknown group. */
  int32_t groupnum;
  upb_inttable *dispatch;  /* Not used by the JIT. */
} upb_pbdecoder_frame;

struct upb_pbdecodermethod {
  upb_refcounted base;

  /* While compiling, the base is relative in "ofs", after compiling it is
   * absolute in "ptr". */
  union {
    uint32_t ofs;     /* PC offset of method. */
    void *ptr;        /* Pointer to bytecode or machine code for this method. */
  } code_base;

  /* The decoder method group to which this method belongs.  We own a ref.
   * Owning a ref on the entire group is more coarse-grained than is strictly
   * necessary; all we truly require is that methods we directly reference
   * outlive us, while the group could contain many other messages we don't
   * require.  But the group represents the messages that were
   * allocated+compiled together, so it makes the most sense to free them
   * together also. */
  const upb_refcounted *group;

  /* Whether this method is native code or bytecode. */
  bool is_native_;

  /* The handler one calls to invoke this method. */
  upb_byteshandler input_handler_;

  /* The destination handlers this method is bound to.  We own a ref. */
  const upb_handlers *dest_handlers_;

  /* Dispatch table -- used by both bytecode decoder and JIT when encountering a
   * field number that wasn't the one we were expecting to see.  See
   * decoder.int.h for the layout of this table. */
  upb_inttable dispatch;
};

struct upb_pbdecoder {
  upb_env *env;

  /* Our input sink. */
  upb_bytessink input_;

  /* The decoder method we are parsing with (owned). */
  const upb_pbdecodermethod *method_;

  size_t call_len;
  const uint32_t *pc, *last;

  /* Current input buffer and its stream offset. */
  const char *buf, *ptr, *end, *checkpoint;

  /* End of the delimited region, relative to ptr, NULL if not in this buf. */
  const char *delim_end;

  /* End of the delimited region, relative to ptr, end if not in this buf. */
  const char *data_end;

  /* Overall stream offset of "buf." */
  uint64_t bufstart_ofs;

  /* Buffer for residual bytes not parsed from the previous buffer.
   * The maximum number of residual bytes we require is 12; a five-byte
   * unknown tag plus an eight-byte value, less one because the value
   * is only a partial value. */
  char residual[12];
  char *residual_end;

  /* Bytes of data that should be discarded from the input beore we start
   * parsing again.  We set this when we internally determine that we can
   * safely skip the next N bytes, but this region extends past the current
   * user buffer. */
  size_t skip;

  /* Stores the user buffer passed to our decode function. */
  const char *buf_param;
  size_t size_param;
  const upb_bufhandle *handle;

  /* Our internal stack. */
  upb_pbdecoder_frame *stack, *top, *limit;
  const uint32_t **callstack;
  size_t stack_size;

  upb_status *status;

#ifdef UPB_USE_JIT_X64
  /* Used momentarily by the generated code to store a value while a user
   * function is called. */
  uint32_t tmp_len;

  const void *saved_rsp;
#endif
};

/* Decoder entry points; used as handlers. */
void *upb_pbdecoder_startbc(void *closure, const void *pc, size_t size_hint);
void *upb_pbdecoder_startjit(void *closure, const void *hd, size_t size_hint);
size_t upb_pbdecoder_decode(void *closure, const void *hd, const char *buf,
                            size_t size, const upb_bufhandle *handle);
bool upb_pbdecoder_end(void *closure, const void *handler_data);

/* Decoder-internal functions that the JIT calls to handle fallback paths. */
int32_t upb_pbdecoder_resume(upb_pbdecoder *d, void *p, const char *buf,
                             size_t size, const upb_bufhandle *handle);
size_t upb_pbdecoder_suspend(upb_pbdecoder *d);
int32_t upb_pbdecoder_skipunknown(upb_pbdecoder *d, int32_t fieldnum,
                                  uint8_t wire_type);
int32_t upb_pbdecoder_checktag_slow(upb_pbdecoder *d, uint64_t expected);
int32_t upb_pbdecoder_decode_varint_slow(upb_pbdecoder *d, uint64_t *u64);
int32_t upb_pbdecoder_decode_f32(upb_pbdecoder *d, uint32_t *u32);
int32_t upb_pbdecoder_decode_f64(upb_pbdecoder *d, uint64_t *u64);
void upb_pbdecoder_seterr(upb_pbdecoder *d, const char *msg);

/* Error messages that are shared between the bytecode and JIT decoders. */
extern const char *kPbDecoderStackOverflow;
extern const char *kPbDecoderSubmessageTooLong;

/* Access to decoderplan members needed by the decoder. */
const char *upb_pbdecoder_getopname(unsigned int op);

/* JIT codegen entry point. */
void upb_pbdecoder_jit(mgroup *group);
void upb_pbdecoder_freejit(mgroup *group);
UPB_REFCOUNTED_CMETHODS(mgroup, mgroup_upcast)

/* A special label that means "do field dispatch for this message and branch to
 * wherever that takes you." */
#define LABEL_DISPATCH 0

/* A special slot in the dispatch table that stores the epilogue (ENDMSG and/or
 * RET) for branching to when we find an appropriate ENDGROUP tag. */
#define DISPATCH_ENDMSG 0

/* It's important to use this invalid wire type instead of 0 (which is a valid
 * wire type). */
#define NO_WIRE_TYPE 0xff

/* The dispatch table layout is:
 *   [field number] -> [ 48-bit offset ][ 8-bit wt2 ][ 8-bit wt1 ]
 *
 * If wt1 matches, jump to the 48-bit offset.  If wt2 matches, lookup
 * (UPB_MAX_FIELDNUMBER + fieldnum) and jump there.
 *
 * We need two wire types because of packed/non-packed compatibility.  A
 * primitive repeated field can use either wire type and be valid.  While we
 * could key the table on fieldnum+wiretype, the table would be 8x sparser.
 *
 * Storing two wire types in the primary value allows us to quickly rule out
 * the second wire type without needing to do a separate lookup (this case is
 * less common than an unknown field). */
UPB_INLINE uint64_t upb_pbdecoder_packdispatch(uint64_t ofs, uint8_t wt1,
                                               uint8_t wt2) {
  return (ofs << 16) | (wt2 << 8) | wt1;
}

UPB_INLINE void upb_pbdecoder_unpackdispatch(uint64_t dispatch, uint64_t *ofs,
                                             uint8_t *wt1, uint8_t *wt2) {
  *wt1 = (uint8_t)dispatch;
  *wt2 = (uint8_t)(dispatch >> 8);
  *ofs = dispatch >> 16;
}

/* All of the functions in decoder.c that return int32_t return values according
 * to the following scheme:
 *   1. negative values indicate a return code from the following list.
 *   2. positive values indicate that error or end of buffer was hit, and
 *      that the decode function should immediately return the given value
 *      (the decoder state has already been suspended and is ready to be
 *      resumed). */
#define DECODE_OK -1
#define DECODE_MISMATCH -2  /* Used only from checktag_slow(). */
#define DECODE_ENDGROUP -3  /* Used only from checkunknown(). */

#define CHECK_RETURN(x) { int32_t ret = x; if (ret >= 0) return ret; }

#endif  /* UPB_DECODER_INT_H_ */
/*
** A number of routines for varint manipulation (we keep them all around to
** have multiple approaches available for benchmarking).
*/

#ifndef UPB_VARINT_DECODER_H_
#define UPB_VARINT_DECODER_H_

#include <assert.h>
#include <stdint.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

/* A list of types as they are encoded on-the-wire. */
typedef enum {
  UPB_WIRE_TYPE_VARINT      = 0,
  UPB_WIRE_TYPE_64BIT       = 1,
  UPB_WIRE_TYPE_DELIMITED   = 2,
  UPB_WIRE_TYPE_START_GROUP = 3,
  UPB_WIRE_TYPE_END_GROUP   = 4,
  UPB_WIRE_TYPE_32BIT       = 5
} upb_wiretype_t;

#define UPB_MAX_WIRE_TYPE 5

/* The maximum number of bytes that it takes to encode a 64-bit varint.
 * Note that with a better encoding this could be 9 (TODO: write up a
 * wiki document about this). */
#define UPB_PB_VARINT_MAX_LEN 10

/* Array of the "native" (ie. non-packed-repeated) wire type for the given a
 * descriptor type (upb_descriptortype_t). */
extern const uint8_t upb_pb_native_wire_types[];

/* Zig-zag encoding/decoding **************************************************/

UPB_INLINE int32_t upb_zzdec_32(uint32_t n) {
  return (n >> 1) ^ -(int32_t)(n & 1);
}
UPB_INLINE int64_t upb_zzdec_64(uint64_t n) {
  return (n >> 1) ^ -(int64_t)(n & 1);
}
UPB_INLINE uint32_t upb_zzenc_32(int32_t n) { return (n << 1) ^ (n >> 31); }
UPB_INLINE uint64_t upb_zzenc_64(int64_t n) { return (n << 1) ^ (n >> 63); }

/* Decoding *******************************************************************/

/* All decoding functions return this struct by value. */
typedef struct {
  const char *p;  /* NULL if the varint was unterminated. */
  uint64_t val;
} upb_decoderet;

UPB_INLINE upb_decoderet upb_decoderet_make(const char *p, uint64_t val) {
  upb_decoderet ret;
  ret.p = p;
  ret.val = val;
  return ret;
}

/* Four functions for decoding a varint of at most eight bytes.  They are all
 * functionally identical, but are implemented in different ways and likely have
 * different performance profiles.  We keep them around for performance testing.
 *
 * Note that these functions may not read byte-by-byte, so they must not be used
 * unless there are at least eight bytes left in the buffer! */
upb_decoderet upb_vdecode_max8_branch32(upb_decoderet r);
upb_decoderet upb_vdecode_max8_branch64(upb_decoderet r);
upb_decoderet upb_vdecode_max8_wright(upb_decoderet r);
upb_decoderet upb_vdecode_max8_massimino(upb_decoderet r);

/* Template for a function that checks the first two bytes with branching
 * and dispatches 2-10 bytes with a separate function.  Note that this may read
 * up to 10 bytes, so it must not be used unless there are at least ten bytes
 * left in the buffer! */
#define UPB_VARINT_DECODER_CHECK2(name, decode_max8_function)                  \
UPB_INLINE upb_decoderet upb_vdecode_check2_ ## name(const char *_p) {         \
  uint8_t *p = (uint8_t*)_p;                                                   \
  upb_decoderet r;                                                             \
  if ((*p & 0x80) == 0) {                                                      \
  /* Common case: one-byte varint. */                                          \
    return upb_decoderet_make(_p + 1, *p & 0x7fU);                             \
  }                                                                            \
  r = upb_decoderet_make(_p + 2, (*p & 0x7fU) | ((*(p + 1) & 0x7fU) << 7));    \
  if ((*(p + 1) & 0x80) == 0) {                                                \
    /* Two-byte varint. */                                                     \
    return r;                                                                  \
  }                                                                            \
  /* Longer varint, fallback to out-of-line function. */                       \
  return decode_max8_function(r);                                              \
}

UPB_VARINT_DECODER_CHECK2(branch32, upb_vdecode_max8_branch32)
UPB_VARINT_DECODER_CHECK2(branch64, upb_vdecode_max8_branch64)
UPB_VARINT_DECODER_CHECK2(wright, upb_vdecode_max8_wright)
UPB_VARINT_DECODER_CHECK2(massimino, upb_vdecode_max8_massimino)
#undef UPB_VARINT_DECODER_CHECK2

/* Our canonical functions for decoding varints, based on the currently
 * favored best-performing implementations. */
UPB_INLINE upb_decoderet upb_vdecode_fast(const char *p) {
  if (sizeof(long) == 8)
    return upb_vdecode_check2_branch64(p);
  else
    return upb_vdecode_check2_branch32(p);
}

UPB_INLINE upb_decoderet upb_vdecode_max8_fast(upb_decoderet r) {
  return upb_vdecode_max8_massimino(r);
}


/* Encoding *******************************************************************/

UPB_INLINE int upb_value_size(uint64_t val) {
#ifdef __GNUC__
  int high_bit = 63 - __builtin_clzll(val);  /* 0-based, undef if val == 0. */
#else
  int high_bit = 0;
  uint64_t tmp = val;
  while(tmp >>= 1) high_bit++;
#endif
  return val == 0 ? 1 : high_bit / 8 + 1;
}

/* Encodes a 64-bit varint into buf (which must be >=UPB_PB_VARINT_MAX_LEN
 * bytes long), returning how many bytes were used.
 *
 * TODO: benchmark and optimize if necessary. */
UPB_INLINE size_t upb_vencode64(uint64_t val, char *buf) {
  size_t i;
  if (val == 0) { buf[0] = 0; return 1; }
  i = 0;
  while (val) {
    uint8_t byte = val & 0x7fU;
    val >>= 7;
    if (val) byte |= 0x80U;
    buf[i++] = byte;
  }
  return i;
}

UPB_INLINE size_t upb_varint_size(uint64_t val) {
  char buf[UPB_PB_VARINT_MAX_LEN];
  return upb_vencode64(val, buf);
}

/* Encodes a 32-bit varint, *not* sign-extended. */
UPB_INLINE uint64_t upb_vencode32(uint32_t val) {
  char buf[UPB_PB_VARINT_MAX_LEN];
  size_t bytes = upb_vencode64(val, buf);
  uint64_t ret = 0;
  assert(bytes <= 5);
  memcpy(&ret, buf, bytes);
  assert(ret <= 0xffffffffffU);
  return ret;
}

#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif  /* UPB_VARINT_DECODER_H_ */
/*
** upb::pb::Encoder (upb_pb_encoder)
**
** Implements a set of upb_handlers that write protobuf data to the binary wire
** format.
**
** This encoder implementation does not have any access to any out-of-band or
** precomputed lengths for submessages, so it must buffer submessages internally
** before it can emit the first byte.
*/

#ifndef UPB_ENCODER_H_
#define UPB_ENCODER_H_


#ifdef __cplusplus
namespace upb {
namespace pb {
class Encoder;
}  /* namespace pb */
}  /* namespace upb */
#endif

UPB_DECLARE_TYPE(upb::pb::Encoder, upb_pb_encoder)

#define UPB_PBENCODER_MAX_NESTING 100

/* upb::pb::Encoder ***********************************************************/

/* Preallocation hint: decoder won't allocate more bytes than this when first
 * constructed.  This hint may be an overestimate for some build configurations.
 * But if the decoder library is upgraded without recompiling the application,
 * it may be an underestimate. */
#define UPB_PB_ENCODER_SIZE 768

#ifdef __cplusplus

class upb::pb::Encoder {
 public:
  /* Creates a new encoder in the given environment.  The Handlers must have
   * come from NewHandlers() below. */
  static Encoder* Create(Environment* env, const Handlers* handlers,
                         BytesSink* output);

  /* The input to the encoder. */
  Sink* input();

  /* Creates a new set of handlers for this MessageDef. */
  static reffed_ptr<const Handlers> NewHandlers(const MessageDef* msg);

  static const size_t kSize = UPB_PB_ENCODER_SIZE;

 private:
  UPB_DISALLOW_POD_OPS(Encoder, upb::pb::Encoder)
};

#endif

UPB_BEGIN_EXTERN_C

const upb_handlers *upb_pb_encoder_newhandlers(const upb_msgdef *m,
                                               const void *owner);
upb_sink *upb_pb_encoder_input(upb_pb_encoder *p);
upb_pb_encoder* upb_pb_encoder_create(upb_env* e, const upb_handlers* h,
                                      upb_bytessink* output);

UPB_END_EXTERN_C

#ifdef __cplusplus

namespace upb {
namespace pb {
inline Encoder* Encoder::Create(Environment* env, const Handlers* handlers,
                                BytesSink* output) {
  return upb_pb_encoder_create(env, handlers, output);
}
inline Sink* Encoder::input() {
  return upb_pb_encoder_input(this);
}
inline reffed_ptr<const Handlers> Encoder::NewHandlers(
    const upb::MessageDef *md) {
  const Handlers* h = upb_pb_encoder_newhandlers(md, &h);
  return reffed_ptr<const Handlers>(h, &h);
}
}  /* namespace pb */
}  /* namespace upb */

#endif

#endif  /* UPB_ENCODER_H_ */
/*
** upb's core components like upb_decoder and upb_msg are carefully designed to
** avoid depending on each other for maximum orthogonality.  In other words,
** you can use a upb_decoder to decode into *any* kind of structure; upb_msg is
** just one such structure.  A upb_msg can be serialized/deserialized into any
** format, protobuf binary format is just one such format.
**
** However, for convenience we provide functions here for doing common
** operations like deserializing protobuf binary format into a upb_msg.  The
** compromise is that this file drags in almost all of upb as a dependency,
** which could be undesirable if you're trying to use a trimmed-down build of
** upb.
**
** While these routines are convenient, they do not reuse any encoding/decoding
** state.  For example, if a decoder is JIT-based, it will be re-JITted every
** time these functions are called.  For this reason, if you are parsing lots
** of data and efficiency is an issue, these may not be the best functions to
** use (though they are useful for prototyping, before optimizing).
*/

#ifndef UPB_GLUE_H
#define UPB_GLUE_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Loads all defs from the given protobuf binary descriptor, setting default
 * accessors and a default layout on all messages.  The caller owns the
 * returned array of defs, which will be of length *n.  On error NULL is
 * returned and status is set (if non-NULL). */
upb_def **upb_load_defs_from_descriptor(const char *str, size_t len, int *n,
                                        void *owner, upb_status *status);

/* Like the previous but also adds the loaded defs to the given symtab. */
bool upb_load_descriptor_into_symtab(upb_symtab *symtab, const char *str,
                                     size_t len, upb_status *status);

/* Like the previous but also reads the descriptor from the given filename. */
bool upb_load_descriptor_file_into_symtab(upb_symtab *symtab, const char *fname,
                                          upb_status *status);

/* Reads the given filename into a character string, returning NULL if there
 * was an error. */
char *upb_readfile(const char *filename, size_t *len);

#ifdef __cplusplus
}  /* extern "C" */

namespace upb {

/* All routines that load descriptors expect the descriptor to be a
 * FileDescriptorSet. */
inline bool LoadDescriptorFileIntoSymtab(SymbolTable* s, const char *fname,
                                         Status* status) {
  return upb_load_descriptor_file_into_symtab(s, fname, status);
}

inline bool LoadDescriptorIntoSymtab(SymbolTable* s, const char* str,
                                     size_t len, Status* status) {
  return upb_load_descriptor_into_symtab(s, str, len, status);
}

/* Templated so it can accept both string and std::string. */
template <typename T>
bool LoadDescriptorIntoSymtab(SymbolTable* s, const T& desc, Status* status) {
  return upb_load_descriptor_into_symtab(s, desc.c_str(), desc.size(), status);
}

}  /* namespace upb */

#endif

#endif  /* UPB_GLUE_H */
/*
** upb::pb::TextPrinter (upb_textprinter)
**
** Handlers for writing to protobuf text format.
*/

#ifndef UPB_TEXT_H_
#define UPB_TEXT_H_


#ifdef __cplusplus
namespace upb {
namespace pb {
class TextPrinter;
}  /* namespace pb */
}  /* namespace upb */
#endif

UPB_DECLARE_TYPE(upb::pb::TextPrinter, upb_textprinter)

#ifdef __cplusplus

class upb::pb::TextPrinter {
 public:
  /* The given handlers must have come from NewHandlers().  It must outlive the
   * TextPrinter. */
  static TextPrinter *Create(Environment *env, const upb::Handlers *handlers,
                             BytesSink *output);

  void SetSingleLineMode(bool single_line);

  Sink* input();

  /* If handler caching becomes a requirement we can add a code cache as in
   * decoder.h */
  static reffed_ptr<const Handlers> NewHandlers(const MessageDef* md);
};

#endif

UPB_BEGIN_EXTERN_C

/* C API. */
upb_textprinter *upb_textprinter_create(upb_env *env, const upb_handlers *h,
                                        upb_bytessink *output);
void upb_textprinter_setsingleline(upb_textprinter *p, bool single_line);
upb_sink *upb_textprinter_input(upb_textprinter *p);

const upb_handlers *upb_textprinter_newhandlers(const upb_msgdef *m,
                                                const void *owner);

UPB_END_EXTERN_C

#ifdef __cplusplus

namespace upb {
namespace pb {
inline TextPrinter *TextPrinter::Create(Environment *env,
                                        const upb::Handlers *handlers,
                                        BytesSink *output) {
  return upb_textprinter_create(env, handlers, output);
}
inline void TextPrinter::SetSingleLineMode(bool single_line) {
  upb_textprinter_setsingleline(this, single_line);
}
inline Sink* TextPrinter::input() {
  return upb_textprinter_input(this);
}
inline reffed_ptr<const Handlers> TextPrinter::NewHandlers(
    const MessageDef *md) {
  const Handlers* h = upb_textprinter_newhandlers(md, &h);
  return reffed_ptr<const Handlers>(h, &h);
}
}  /* namespace pb */
}  /* namespace upb */

#endif

#endif  /* UPB_TEXT_H_ */
/*
** upb::json::Parser (upb_json_parser)
**
** Parses JSON according to a specific schema.
** Support for parsing arbitrary JSON (schema-less) will be added later.
*/

#ifndef UPB_JSON_PARSER_H_
#define UPB_JSON_PARSER_H_


#ifdef __cplusplus
namespace upb {
namespace json {
class Parser;
}  /* namespace json */
}  /* namespace upb */
#endif

UPB_DECLARE_TYPE(upb::json::Parser, upb_json_parser)

/* upb::json::Parser **********************************************************/

/* Preallocation hint: parser won't allocate more bytes than this when first
 * constructed.  This hint may be an overestimate for some build configurations.
 * But if the parser library is upgraded without recompiling the application,
 * it may be an underestimate. */
#define UPB_JSON_PARSER_SIZE 3704

#ifdef __cplusplus

/* Parses an incoming BytesStream, pushing the results to the destination
 * sink. */
class upb::json::Parser {
 public:
  static Parser* Create(Environment* env, Sink* output);

  BytesSink* input();

 private:
  UPB_DISALLOW_POD_OPS(Parser, upb::json::Parser)
};

#endif

UPB_BEGIN_EXTERN_C

upb_json_parser *upb_json_parser_create(upb_env *e, upb_sink *output);
upb_bytessink *upb_json_parser_input(upb_json_parser *p);

UPB_END_EXTERN_C

#ifdef __cplusplus

namespace upb {
namespace json {
inline Parser* Parser::Create(Environment* env, Sink* output) {
  return upb_json_parser_create(env, output);
}
inline BytesSink* Parser::input() {
  return upb_json_parser_input(this);
}
}  /* namespace json */
}  /* namespace upb */

#endif


#endif  /* UPB_JSON_PARSER_H_ */
/*
** upb::json::Printer
**
** Handlers that emit JSON according to a specific protobuf schema.
*/

#ifndef UPB_JSON_TYPED_PRINTER_H_
#define UPB_JSON_TYPED_PRINTER_H_


#ifdef __cplusplus
namespace upb {
namespace json {
class Printer;
}  /* namespace json */
}  /* namespace upb */
#endif

UPB_DECLARE_TYPE(upb::json::Printer, upb_json_printer)


/* upb::json::Printer *********************************************************/

#define UPB_JSON_PRINTER_SIZE 168

#ifdef __cplusplus

/* Prints an incoming stream of data to a BytesSink in JSON format. */
class upb::json::Printer {
 public:
  static Printer* Create(Environment* env, const upb::Handlers* handlers,
                         BytesSink* output);

  /* The input to the printer. */
  Sink* input();

  /* Returns handlers for printing according to the specified schema. */
  static reffed_ptr<const Handlers> NewHandlers(const upb::MessageDef* md);

  static const size_t kSize = UPB_JSON_PRINTER_SIZE;

 private:
  UPB_DISALLOW_POD_OPS(Printer, upb::json::Printer)
};

#endif

UPB_BEGIN_EXTERN_C

/* Native C API. */
upb_json_printer *upb_json_printer_create(upb_env *e, const upb_handlers *h,
                                          upb_bytessink *output);
upb_sink *upb_json_printer_input(upb_json_printer *p);
const upb_handlers *upb_json_printer_newhandlers(const upb_msgdef *md,
                                                 const void *owner);

UPB_END_EXTERN_C

#ifdef __cplusplus

namespace upb {
namespace json {
inline Printer* Printer::Create(Environment* env, const upb::Handlers* handlers,
                                BytesSink* output) {
  return upb_json_printer_create(env, handlers, output);
}
inline Sink* Printer::input() { return upb_json_printer_input(this); }
inline reffed_ptr<const Handlers> Printer::NewHandlers(
    const upb::MessageDef *md) {
  const Handlers* h = upb_json_printer_newhandlers(md, &h);
  return reffed_ptr<const Handlers>(h, &h);
}
}  /* namespace json */
}  /* namespace upb */

#endif

#endif  /* UPB_JSON_TYPED_PRINTER_H_ */
