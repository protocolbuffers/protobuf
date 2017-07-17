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

#ifdef __cplusplus
namespace upb {
class Allocator;
class Arena;
class Environment;
class ErrorSpace;
class Status;
template <int N> class InlinedArena;
template <int N> class InlinedEnvironment;
}
#endif

/* UPB_INLINE: inline if possible, emit standalone code if required. */
#ifdef __cplusplus
#define UPB_INLINE inline
#elif defined (__GNUC__)
#define UPB_INLINE static __inline__
#else
#define UPB_INLINE static
#endif

/* Hints to the compiler about likely/unlikely branches. */
#define UPB_LIKELY(x) __builtin_expect((x),1)

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

#if __STDC_VERSION__ >= 199901L || __cplusplus >= 201103L
/* C99/C++11 versions. */
#include <stdio.h>
#define _upb_snprintf snprintf
#define _upb_vsnprintf vsnprintf
#define _upb_va_copy(a, b) va_copy(a, b)
#elif defined __GNUC__
/* A few hacky workarounds for functions not in C89.
 * For internal use only!
 * TODO(haberman): fix these by including our own implementations, or finding
 * another workaround.
 */
#define _upb_snprintf __builtin_snprintf
#define _upb_vsnprintf __builtin_vsnprintf
#define _upb_va_copy(a, b) __va_copy(a, b)
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
#define UPB_FINAL final
#else  /* !defined(UPB_CXX11) */
#define UPB_DISALLOW_COPY_AND_ASSIGN(class_name) \
  class_name(const class_name&); \
  void operator=(const class_name&);
#define UPB_DISALLOW_POD_OPS(class_name, full_class_name) \
  class_name(); \
  ~class_name(); \
  UPB_DISALLOW_COPY_AND_ASSIGN(class_name)
#define UPB_ASSERT_STDLAYOUT(type)
#define UPB_FINAL
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
    explicit Pointer(cppname* ptr)                                \
        : PointerBase<cppname, cppbase>(ptr) {}                   \
  };                                                              \
  template <>                                                     \
  class Pointer<const cppname>                                    \
      : public PointerBase<const cppname, const cppbase> {        \
   public:                                                        \
    explicit Pointer(const cppname* ptr)                          \
        : PointerBase<const cppname, const cppbase>(ptr) {}       \
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
    explicit Pointer(cppname* ptr)                                           \
        : PointerBase2<cppname, cppbase, cppbase2>(ptr) {}                   \
  };                                                                         \
  template <>                                                                \
  class Pointer<const cppname>                                               \
      : public PointerBase2<const cppname, const cppbase, const cppbase2> {  \
   public:                                                                   \
    explicit Pointer(const cppname* ptr)                                     \
        : PointerBase2<const cppname, const cppbase, const cppbase2>(ptr) {} \
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

/* UPB_ASSERT(): in release mode, we use the expression without letting it be
 * evaluated.  This prevents "unused variable" warnings. */
#ifdef NDEBUG
#define UPB_ASSERT(expr) do {} while (false && (expr))
#else
#define UPB_ASSERT(expr) assert(expr)
#endif

/* UPB_ASSERT_DEBUGVAR(): assert that uses functions or variables that only
 * exist in debug mode.  This turns into regular assert. */
#define UPB_ASSERT_DEBUGVAR(expr) assert(expr)

#ifdef __GNUC__
#define UPB_UNREACHABLE() do { assert(0); __builtin_unreachable(); } while(0)
#else
#define UPB_UNREACHABLE() do { assert(0); } while(0)
#endif

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

/* A list of types as they are encoded on-the-wire. */
typedef enum {
  UPB_WIRE_TYPE_VARINT      = 0,
  UPB_WIRE_TYPE_64BIT       = 1,
  UPB_WIRE_TYPE_DELIMITED   = 2,
  UPB_WIRE_TYPE_START_GROUP = 3,
  UPB_WIRE_TYPE_END_GROUP   = 4,
  UPB_WIRE_TYPE_32BIT       = 5
} upb_wiretype_t;


/* upb::ErrorSpace ************************************************************/

/* A upb::ErrorSpace represents some domain of possible error values.  This lets
 * upb::Status attach specific error codes to operations, like POSIX/C errno,
 * Win32 error codes, etc.  Clients who want to know the very specific error
 * code can check the error space and then know the type of the integer code.
 *
 * NOTE: upb::ErrorSpace is currently not used and should be considered
 * experimental.  It is important primarily in cases where upb is performing
 * I/O, but upb doesn't currently have any components that do this. */

UPB_DECLARE_TYPE(upb::ErrorSpace, upb_errorspace)

#ifdef __cplusplus
class upb::ErrorSpace {
#else
struct upb_errorspace {
#endif
  const char *name;
};


/* upb::Status ****************************************************************/

/* upb::Status represents a success or failure status and error message.
 * It owns no resources and allocates no memory, so it should work
 * even in OOM situations. */
UPB_DECLARE_TYPE(upb::Status, upb_status)

/* The maximum length of an error message before it will get truncated. */
#define UPB_STATUS_MAX_MESSAGE 128

UPB_BEGIN_EXTERN_C

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
void upb_status_copy(upb_status *to, const upb_status *from);

UPB_END_EXTERN_C

#ifdef __cplusplus

class upb::Status {
 public:
  Status() { upb_status_clear(this); }

  /* Returns true if there is no error. */
  bool ok() const { return upb_ok(this); }

  /* Optional error space and code, useful if the caller wants to
   * programmatically check the specific kind of error. */
  ErrorSpace* error_space() { return upb_status_errspace(this); }
  int error_code() const { return upb_status_errcode(this); }

  /* The returned string is invalidated by any other call into the status. */
  const char *error_message() const { return upb_status_errmsg(this); }

  /* The error message will be truncated if it is longer than
   * UPB_STATUS_MAX_MESSAGE-4. */
  void SetErrorMessage(const char* msg) { upb_status_seterrmsg(this, msg); }
  void SetFormattedErrorMessage(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    upb_status_vseterrf(this, fmt, args);
    va_end(args);
  }

  /* Resets the status to a successful state with no message. */
  void Clear() { upb_status_clear(this); }

  void CopyFrom(const Status& other) { upb_status_copy(this, &other); }

 private:
  UPB_DISALLOW_COPY_AND_ASSIGN(Status)
#else
struct upb_status {
#endif
  bool ok_;

  /* Specific status code defined by some error space (optional). */
  int code_;
  upb_errorspace *error_space_;

  /* TODO(haberman): add file/line of error? */

  /* Error message; NULL-terminated. */
  char msg[UPB_STATUS_MAX_MESSAGE];
};

#define UPB_STATUS_INIT {true, 0, NULL, {0}}


/** Built-in error spaces. ****************************************************/

/* Errors raised by upb that we want to be able to detect programmatically. */
typedef enum {
  UPB_NOMEM   /* Can't reuse ENOMEM because it is POSIX, not ISO C. */
} upb_errcode_t;

extern upb_errorspace upb_upberr;

void upb_upberr_setoom(upb_status *s);

/* Since errno is defined by standard C, we define an error space for it in
 * core upb.  Other error spaces should be defined in other, platform-specific
 * modules. */

extern upb_errorspace upb_errnoerr;


/** upb::Allocator ************************************************************/

/* A upb::Allocator is a possibly-stateful allocator object.
 *
 * It could either be an arena allocator (which doesn't require individual
 * free() calls) or a regular malloc() (which does).  The client must therefore
 * free memory unless it knows that the allocator is an arena allocator. */
UPB_DECLARE_TYPE(upb::Allocator, upb_alloc)

/* A malloc()/free() function.
 * If "size" is 0 then the function acts like free(), otherwise it acts like
 * realloc().  Only "oldsize" bytes from a previous allocation are preserved. */
typedef void *upb_alloc_func(upb_alloc *alloc, void *ptr, size_t oldsize,
                             size_t size);

#ifdef __cplusplus

class upb::Allocator UPB_FINAL {
 public:
  Allocator() {}

 private:
  UPB_DISALLOW_COPY_AND_ASSIGN(Allocator)

 public:
#else
struct upb_alloc {
#endif  /* __cplusplus */
  upb_alloc_func *func;
};

UPB_INLINE void *upb_malloc(upb_alloc *alloc, size_t size) {
  UPB_ASSERT(alloc);
  return alloc->func(alloc, NULL, 0, size);
}

UPB_INLINE void *upb_realloc(upb_alloc *alloc, void *ptr, size_t oldsize,
                             size_t size) {
  UPB_ASSERT(alloc);
  return alloc->func(alloc, ptr, oldsize, size);
}

UPB_INLINE void upb_free(upb_alloc *alloc, void *ptr) {
  assert(alloc);
  alloc->func(alloc, ptr, 0, 0);
}

/* The global allocator used by upb.  Uses the standard malloc()/free(). */

extern upb_alloc upb_alloc_global;

/* Functions that hard-code the global malloc.
 *
 * We still get benefit because we can put custom logic into our global
 * allocator, like injecting out-of-memory faults in debug/testing builds. */

UPB_INLINE void *upb_gmalloc(size_t size) {
  return upb_malloc(&upb_alloc_global, size);
}

UPB_INLINE void *upb_grealloc(void *ptr, size_t oldsize, size_t size) {
  return upb_realloc(&upb_alloc_global, ptr, oldsize, size);
}

UPB_INLINE void upb_gfree(void *ptr) {
  upb_free(&upb_alloc_global, ptr);
}

/* upb::Arena *****************************************************************/

/* upb::Arena is a specific allocator implementation that uses arena allocation.
 * The user provides an allocator that will be used to allocate the underlying
 * arena blocks.  Arenas by nature do not require the individual allocations
 * to be freed.  However the Arena does allow users to register cleanup
 * functions that will run when the arena is destroyed.
 *
 * A upb::Arena is *not* thread-safe.
 *
 * You could write a thread-safe arena allocator that satisfies the
 * upb::Allocator interface, but it would not be as efficient for the
 * single-threaded case. */
UPB_DECLARE_TYPE(upb::Arena, upb_arena)

typedef void upb_cleanup_func(void *ud);

#define UPB_ARENA_BLOCK_OVERHEAD (sizeof(size_t)*4)

UPB_BEGIN_EXTERN_C

void upb_arena_init(upb_arena *a);
void upb_arena_init2(upb_arena *a, void *mem, size_t n, upb_alloc *alloc);
void upb_arena_uninit(upb_arena *a);
bool upb_arena_addcleanup(upb_arena *a, upb_cleanup_func *func, void *ud);
size_t upb_arena_bytesallocated(const upb_arena *a);
void upb_arena_setnextblocksize(upb_arena *a, size_t size);
void upb_arena_setmaxblocksize(upb_arena *a, size_t size);
UPB_INLINE upb_alloc *upb_arena_alloc(upb_arena *a) { return (upb_alloc*)a; }

UPB_END_EXTERN_C

#ifdef __cplusplus

class upb::Arena {
 public:
  /* A simple arena with no initial memory block and the default allocator. */
  Arena() { upb_arena_init(this); }

  /* Constructs an arena with the given initial block which allocates blocks
   * with the given allocator.  The given allocator must outlive the Arena.
   *
   * If you pass NULL for the allocator it will default to the global allocator
   * upb_alloc_global, and NULL/0 for the initial block will cause there to be
   * no initial block. */
  Arena(void *mem, size_t len, Allocator* a) {
    upb_arena_init2(this, mem, len, a);
  }

  ~Arena() { upb_arena_uninit(this); }

  /* Sets the size of the next block the Arena will request (unless the
   * requested allocation is larger).  Each block will double in size until the
   * max limit is reached. */
  void SetNextBlockSize(size_t size) { upb_arena_setnextblocksize(this, size); }

  /* Sets the maximum block size.  No blocks larger than this will be requested
   * from the underlying allocator unless individual arena allocations are
   * larger. */
  void SetMaxBlockSize(size_t size) { upb_arena_setmaxblocksize(this, size); }

  /* Allows this arena to be used as a generic allocator.
   *
   * The arena does not need free() calls so when using Arena as an allocator
   * it is safe to skip them.  However they are no-ops so there is no harm in
   * calling free() either. */
  Allocator* allocator() { return upb_arena_alloc(this); }

  /* Add a cleanup function to run when the arena is destroyed.
   * Returns false on out-of-memory. */
  bool AddCleanup(upb_cleanup_func* func, void* ud) {
    return upb_arena_addcleanup(this, func, ud);
  }

  /* Total number of bytes that have been allocated.  It is undefined what
   * Realloc() does to this counter. */
  size_t BytesAllocated() const {
    return upb_arena_bytesallocated(this);
  }

 private:
  UPB_DISALLOW_COPY_AND_ASSIGN(Arena)

#else
struct upb_arena {
#endif  /* __cplusplus */
  /* We implement the allocator interface.
   * This must be the first member of upb_arena! */
  upb_alloc alloc;

  /* Allocator to allocate arena blocks.  We are responsible for freeing these
   * when we are destroyed. */
  upb_alloc *block_alloc;

  size_t bytes_allocated;
  size_t next_block_size;
  size_t max_block_size;

  /* Linked list of blocks.  Points to an arena_block, defined in env.c */
  void *block_head;

  /* Cleanup entries.  Pointer to a cleanup_ent, defined in env.c */
  void *cleanup_head;

  /* For future expansion, since the size of this struct is exposed to users. */
  void *future1;
  void *future2;
};


/* upb::Environment ***********************************************************/

/* A upb::Environment provides a means for injecting malloc and an
 * error-reporting callback into encoders/decoders.  This allows them to be
 * independent of nearly all assumptions about their actual environment.
 *
 * It is also a container for allocating the encoders/decoders themselves that
 * insulates clients from knowing their actual size.  This provides ABI
 * compatibility even if the size of the objects change.  And this allows the
 * structure definitions to be in the .c files instead of the .h files, making
 * the .h files smaller and more readable.
 *
 * We might want to consider renaming this to "Pipeline" if/when the concept of
 * a pipeline element becomes more formalized. */
UPB_DECLARE_TYPE(upb::Environment, upb_env)

/* A function that receives an error report from an encoder or decoder.  The
 * callback can return true to request that the error should be recovered, but
 * if the error is not recoverable this has no effect. */
typedef bool upb_error_func(void *ud, const upb_status *status);

UPB_BEGIN_EXTERN_C

void upb_env_init(upb_env *e);
void upb_env_init2(upb_env *e, void *mem, size_t n, upb_alloc *alloc);
void upb_env_uninit(upb_env *e);

void upb_env_initonly(upb_env *e);

UPB_INLINE upb_arena *upb_env_arena(upb_env *e) { return (upb_arena*)e; }
bool upb_env_ok(const upb_env *e);
void upb_env_seterrorfunc(upb_env *e, upb_error_func *func, void *ud);

/* Convenience wrappers around the methods of the contained arena. */
void upb_env_reporterrorsto(upb_env *e, upb_status *s);
bool upb_env_reporterror(upb_env *e, const upb_status *s);
void *upb_env_malloc(upb_env *e, size_t size);
void *upb_env_realloc(upb_env *e, void *ptr, size_t oldsize, size_t size);
void upb_env_free(upb_env *e, void *ptr);
bool upb_env_addcleanup(upb_env *e, upb_cleanup_func *func, void *ud);
size_t upb_env_bytesallocated(const upb_env *e);

UPB_END_EXTERN_C

#ifdef __cplusplus

class upb::Environment {
 public:
  /* The given Arena must outlive this environment. */
  Environment() { upb_env_initonly(this); }

  Environment(void *mem, size_t len, Allocator *a) : arena_(mem, len, a) {
    upb_env_initonly(this);
  }

  Arena* arena() { return upb_env_arena(this); }

  /* Set a custom error reporting function. */
  void SetErrorFunction(upb_error_func* func, void* ud) {
    upb_env_seterrorfunc(this, func, ud);
  }

  /* Set the error reporting function to simply copy the status to the given
   * status and abort. */
  void ReportErrorsTo(Status* status) { upb_env_reporterrorsto(this, status); }

  /* Returns true if all allocations and AddCleanup() calls have succeeded,
   * and no errors were reported with ReportError() (except ones that recovered
   * successfully). */
  bool ok() const { return upb_env_ok(this); }

  /* Reports an error to this environment's callback, returning true if
   * the caller should try to recover. */
  bool ReportError(const Status* status) {
    return upb_env_reporterror(this, status);
  }

 private:
  UPB_DISALLOW_COPY_AND_ASSIGN(Environment)

#else
struct upb_env {
#endif  /* __cplusplus */
  upb_arena arena_;
  upb_error_func *error_func_;
  void *error_ud_;
  bool ok_;
};


/* upb::InlinedArena **********************************************************/
/* upb::InlinedEnvironment ****************************************************/

/* upb::InlinedArena and upb::InlinedEnvironment seed their arenas with a
 * predefined amount of memory.  No heap memory will be allocated until the
 * initial block is exceeded.
 *
 * These types only exist in C++ */

#ifdef __cplusplus

template <int N> class upb::InlinedArena : public upb::Arena {
 public:
  InlinedArena() : Arena(initial_block_, N, NULL) {}
  explicit InlinedArena(Allocator* a) : Arena(initial_block_, N, a) {}

 private:
  UPB_DISALLOW_COPY_AND_ASSIGN(InlinedArena)

  char initial_block_[N + UPB_ARENA_BLOCK_OVERHEAD];
};

template <int N> class upb::InlinedEnvironment : public upb::Environment {
 public:
  InlinedEnvironment() : Environment(initial_block_, N, NULL) {}
  explicit InlinedEnvironment(Allocator *a)
      : Environment(initial_block_, N, a) {}

 private:
  UPB_DISALLOW_COPY_AND_ASSIGN(InlinedEnvironment)

  char initial_block_[N + UPB_ARENA_BLOCK_OVERHEAD];
};

#endif  /* __cplusplus */



#endif  /* UPB_H_ */
