/* Amalgamated source file */

#if UINTPTR_MAX == 0xffffffff
#define UPB_SIZE(size32, size64) size32
#else
#define UPB_SIZE(size32, size64) size64
#endif

#define UPB_FIELD_AT(msg, fieldtype, offset) \
  *(fieldtype*)((const char*)(msg) + offset)

#define UPB_READ_ONEOF(msg, fieldtype, offset, case_offset, case_val, default) \
  UPB_FIELD_AT(msg, int, case_offset) == case_val                              \
      ? UPB_FIELD_AT(msg, fieldtype, offset)                                   \
      : default

#define UPB_WRITE_ONEOF(msg, fieldtype, offset, value, case_offset, case_val) \
  UPB_FIELD_AT(msg, int, case_offset) = case_val;                             \
  UPB_FIELD_AT(msg, fieldtype, offset) = value;
/*
** upb::Message is a representation for protobuf messages.
**
** However it differs from other common representations like
** google::protobuf::Message in one key way: it does not prescribe any
** ownership between messages and submessages, and it relies on the
** client to ensure that each submessage/array/map outlives its parent.
**
** All messages, arrays, and maps live in an Arena.  If the entire message
** tree is in the same arena, ensuring proper lifetimes is simple.  However
** the client can mix arenas as long as they ensure that there are no
** dangling pointers.
**
** A client can access a upb::Message without knowing anything about
** ownership semantics, but to create or mutate a message a user needs
** to implement the memory management themselves.
**
** TODO: UTF-8 checking?
**/

#ifndef UPB_MSG_H_
#define UPB_MSG_H_

/*
** Defs are upb's internal representation of the constructs that can appear
** in a .proto file:
**
** - upb::MessageDef (upb_msgdef): describes a "message" construct.
** - upb::FieldDef (upb_fielddef): describes a message field.
** - upb::FileDef (upb_filedef): describes a .proto file and its defs.
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
** The table must be homogenous (all values of the same type).  In debug
** mode, we check this on insert and lookup.
*/

#ifndef UPB_TABLE_H_
#define UPB_TABLE_H_

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
  UPB_CTYPE_FPTR     = 9,
  UPB_CTYPE_FLOAT    = 10,
  UPB_CTYPE_DOUBLE   = 11
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
char *upb_strdup(const char *s, upb_alloc *a);
/* Variant that works with a length-delimited rather than NULL-delimited string,
 * as supported by strtable. */
char *upb_strdup2(const char *s, size_t len, upb_alloc *a);

UPB_INLINE char *upb_gstrdup(const char *s) {
  return upb_strdup(s, &upb_alloc_global);
}

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
    UPB_ASSERT_DEBUGVAR(val.ctype == proto_type); \
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

UPB_INLINE void upb_value_setfloat(upb_value *val, float cval) {
  memcpy(&val->val, &cval, sizeof(cval));
  SET_TYPE(val->ctype, UPB_CTYPE_FLOAT);
}

UPB_INLINE void upb_value_setdouble(upb_value *val, double cval) {
  memcpy(&val->val, &cval, sizeof(cval));
  SET_TYPE(val->ctype, UPB_CTYPE_DOUBLE);
}

UPB_INLINE upb_value upb_value_float(float cval) {
  upb_value ret;
  upb_value_setfloat(&ret, cval);
  return ret;
}

UPB_INLINE upb_value upb_value_double(double cval) {
  upb_value ret;
  upb_value_setdouble(&ret, cval);
  return ret;
}

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

#ifndef NDEBUG
  /* This table's allocator.  We make the user pass it in to every relevant
   * function and only use this to check it in debug mode.  We do this solely
   * to keep upb_table as small as possible.  This might seem slightly paranoid
   * but the plan is to use upb_table for all map fields and extension sets in
   * a forthcoming message representation, so there could be a lot of these.
   * If this turns out to be too annoying later, we can change it (since this
   * is an internal-only header file). */
  upb_alloc *alloc;
#endif
} upb_table;

#ifdef NDEBUG
#  define UPB_TABLE_INIT(count, mask, ctype, size_lg2, entries) \
     {count, mask, ctype, size_lg2, entries}
#else
#  ifdef UPB_DEBUG_REFS
/* At the moment the only mutable tables we statically initialize are debug
 * ref tables. */
#    define UPB_TABLE_INIT(count, mask, ctype, size_lg2, entries) \
       {count, mask, ctype, size_lg2, entries, &upb_alloc_debugrefs}
#  else
#    define UPB_TABLE_INIT(count, mask, ctype, size_lg2, entries) \
       {count, mask, ctype, size_lg2, entries, NULL}
#  endif
#endif

typedef struct {
  upb_table t;
} upb_strtable;

#define UPB_STRTABLE_INIT(count, mask, ctype, size_lg2, entries) \
  {UPB_TABLE_INIT(count, mask, ctype, size_lg2, entries)}

#define UPB_EMPTY_STRTABLE_INIT(ctype)                           \
  UPB_STRTABLE_INIT(0, 0, ctype, 0, NULL)

typedef struct {
  upb_table t;              /* For entries that don't fit in the array part. */
  const upb_tabval *array;  /* Array part of the table. See const note above. */
  size_t array_size;        /* Array part size. */
  size_t array_count;       /* Array part number of elements. */
} upb_inttable;

#define UPB_INTTABLE_INIT(count, mask, ctype, size_lg2, ent, a, asize, acount) \
  {UPB_TABLE_INIT(count, mask, ctype, size_lg2, ent), a, asize, acount}

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
bool upb_inttable_init2(upb_inttable *table, upb_ctype_t ctype, upb_alloc *a);
bool upb_strtable_init2(upb_strtable *table, upb_ctype_t ctype, upb_alloc *a);
void upb_inttable_uninit2(upb_inttable *table, upb_alloc *a);
void upb_strtable_uninit2(upb_strtable *table, upb_alloc *a);

UPB_INLINE bool upb_inttable_init(upb_inttable *table, upb_ctype_t ctype) {
  return upb_inttable_init2(table, ctype, &upb_alloc_global);
}

UPB_INLINE bool upb_strtable_init(upb_strtable *table, upb_ctype_t ctype) {
  return upb_strtable_init2(table, ctype, &upb_alloc_global);
}

UPB_INLINE void upb_inttable_uninit(upb_inttable *table) {
  upb_inttable_uninit2(table, &upb_alloc_global);
}

UPB_INLINE void upb_strtable_uninit(upb_strtable *table) {
  upb_strtable_uninit2(table, &upb_alloc_global);
}

/* Returns the number of values in the table. */
size_t upb_inttable_count(const upb_inttable *t);
UPB_INLINE size_t upb_strtable_count(const upb_strtable *t) {
  return t->t.count;
}

void upb_inttable_packedsize(const upb_inttable *t, size_t *size);
void upb_strtable_packedsize(const upb_strtable *t, size_t *size);
upb_inttable *upb_inttable_pack(const upb_inttable *t, void *p, size_t *ofs,
                                size_t size);
upb_strtable *upb_strtable_pack(const upb_strtable *t, void *p, size_t *ofs,
                                size_t size);

/* Inserts the given key into the hashtable with the given value.  The key must
 * not already exist in the hash table.  For string tables, the key must be
 * NULL-terminated, and the table will make an internal copy of the key.
 * Inttables must not insert a value of UINTPTR_MAX.
 *
 * If a table resize was required but memory allocation failed, false is
 * returned and the table is unchanged. */
bool upb_inttable_insert2(upb_inttable *t, uintptr_t key, upb_value val,
                          upb_alloc *a);
bool upb_strtable_insert3(upb_strtable *t, const char *key, size_t len,
                          upb_value val, upb_alloc *a);

UPB_INLINE bool upb_inttable_insert(upb_inttable *t, uintptr_t key,
                                    upb_value val) {
  return upb_inttable_insert2(t, key, val, &upb_alloc_global);
}

UPB_INLINE bool upb_strtable_insert2(upb_strtable *t, const char *key,
                                     size_t len, upb_value val) {
  return upb_strtable_insert3(t, key, len, val, &upb_alloc_global);
}

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
bool upb_strtable_remove3(upb_strtable *t, const char *key, size_t len,
                          upb_value *val, upb_alloc *alloc);

UPB_INLINE bool upb_strtable_remove2(upb_strtable *t, const char *key,
                                     size_t len, upb_value *val) {
  return upb_strtable_remove3(t, key, len, val, &upb_alloc_global);
}

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
bool upb_inttable_push2(upb_inttable *t, upb_value val, upb_alloc *a);
upb_value upb_inttable_pop(upb_inttable *t);

UPB_INLINE bool upb_inttable_push(upb_inttable *t, upb_value val) {
  return upb_inttable_push2(t, val, &upb_alloc_global);
}

/* Convenience routines for inttables with pointer keys. */
bool upb_inttable_insertptr2(upb_inttable *t, const void *key, upb_value val,
                             upb_alloc *a);
bool upb_inttable_removeptr(upb_inttable *t, const void *key, upb_value *val);
bool upb_inttable_lookupptr(
    const upb_inttable *t, const void *key, upb_value *val);

UPB_INLINE bool upb_inttable_insertptr(upb_inttable *t, const void *key,
                                       upb_value val) {
  return upb_inttable_insertptr2(t, key, val, &upb_alloc_global);
}

/* Optimizes the table for the current set of entries, for both memory use and
 * lookup time.  Client should call this after all entries have been inserted;
 * inserting more entries is legal, but will likely require a table resize. */
void upb_inttable_compact2(upb_inttable *t, upb_alloc *a);

UPB_INLINE void upb_inttable_compact(upb_inttable *t) {
  upb_inttable_compact2(t, &upb_alloc_global);
}

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
bool upb_strtable_resize(upb_strtable *t, size_t size_lg2, upb_alloc *a);

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
const char *upb_strtable_iter_key(const upb_strtable_iter *i);
size_t upb_strtable_iter_keylength(const upb_strtable_iter *i);
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
namespace upb {
class RefCounted;
template <class T> class reffed_ptr;
}
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
extern upb_alloc upb_alloc_debugrefs;
#define UPB_REFCOUNT_INIT(vtbl, refs, ref2s) \
    {&static_refcount, NULL, vtbl, 0, true, refs, ref2s}
#else
#define UPB_REFCOUNT_INIT(vtbl, refs, ref2s) \
    {&static_refcount, NULL, vtbl, 0, true}
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


/* upb::reffed_ptr ************************************************************/

#ifdef __cplusplus

#include <algorithm>  /* For std::swap(). */

/* Provides RAII semantics for upb refcounted objects.  Each reffed_ptr owns a
 * ref on whatever object it points to (if any). */
template <class T> class upb::reffed_ptr {
 public:
  reffed_ptr() : ptr_(NULL) {}

  /* If ref_donor is NULL, takes a new ref, otherwise adopts from ref_donor. */
  template <class U>
  reffed_ptr(U* val, const void* ref_donor = NULL)
      : ptr_(upb::upcast(val)) {
    if (ref_donor) {
      UPB_ASSERT(ptr_);
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

  reffed_ptr(const reffed_ptr& other)
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
    UPB_ASSERT(ptr_);
    return *ptr_;
  }

  T* operator->() const {
    UPB_ASSERT(ptr_);
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

#endif  /* __cplusplus */

#endif  /* UPB_REFCOUNT_H_ */

#ifdef __cplusplus
#include <cstring>
#include <string>
#include <vector>

namespace upb {
class Def;
class EnumDef;
class FieldDef;
class FileDef;
class MessageDef;
class OneofDef;
class SymbolTable;
}
#endif

UPB_DECLARE_DERIVED_TYPE(upb::Def, upb::RefCounted, upb_def, upb_refcounted)
UPB_DECLARE_DERIVED_TYPE(upb::OneofDef, upb::RefCounted, upb_oneofdef,
                         upb_refcounted)
UPB_DECLARE_DERIVED_TYPE(upb::FileDef, upb::RefCounted, upb_filedef,
                         upb_refcounted)
UPB_DECLARE_TYPE(upb::SymbolTable, upb_symtab)


/* The maximum message depth that the type graph can have.  This is a resource
 * limit for the C stack since we sometimes need to recursively traverse the
 * graph.  Cycles are ok; the traversal will stop when it detects a cycle, but
 * we must hit the cycle before the maximum depth is reached.
 *
 * If having a single static limit is too inflexible, we can add another variant
 * of Def::Freeze that allows specifying this as a parameter. */
#define UPB_MAX_MESSAGE_DEPTH 64


/* upb::Def: base class for top-level defs  ***********************************/

/* All the different kind of defs that can be defined at the top-level and put
 * in a SymbolTable or appear in a FileDef::defs() list.  This excludes some
 * defs (like oneofs and files).  It only includes fields because they can be
 * defined as extensions. */
typedef enum {
  UPB_DEF_MSG,
  UPB_DEF_FIELD,
  UPB_DEF_ENUM,
  UPB_DEF_SERVICE,   /* Not yet implemented. */
  UPB_DEF_ANY = -1   /* Wildcard for upb_symtab_get*() */
} upb_deftype_t;

#ifdef __cplusplus

/* The base class of all defs.  Its base is upb::RefCounted (use upb::upcast()
 * to convert). */
class upb::Def {
 public:
  typedef upb_deftype_t Type;

  /* upb::RefCounted methods like Ref()/Unref(). */
  UPB_REFCOUNTED_CPPMETHODS

  Type def_type() const;

  /* "fullname" is the def's fully-qualified name (eg. foo.bar.Message). */
  const char *full_name() const;

  /* The final part of a def's name (eg. Message). */
  const char *name() const;

  /* The def must be mutable.  Caller retains ownership of fullname.  Defs are
   * not required to have a name; if a def has no name when it is frozen, it
   * will remain an anonymous def.  On failure, returns false and details in "s"
   * if non-NULL. */
  bool set_full_name(const char* fullname, upb::Status* s);
  bool set_full_name(const std::string &fullname, upb::Status* s);

  /* The file in which this def appears.  It is not necessary to add a def to a
   * file (and consequently the accessor may return NULL).  Set this by calling
   * file->Add(def). */
  FileDef* file() const;

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
  static bool Freeze(Def* const* defs, size_t n, Status* status);
  static bool Freeze(const std::vector<Def*>& defs, Status* status);

 private:
  UPB_DISALLOW_POD_OPS(Def, upb::Def)
#else
struct upb_def {
  upb_refcounted base;

  const char *fullname;
  const upb_filedef* file;
  char type;  /* A upb_deftype_t (char to save space) */

  /* Used as a flag during the def's mutable stage.  Must be false unless
   * it is currently being used by a function on the stack.  This allows
   * us to easily determine which defs were passed into the function's
   * current invocation. */
  bool came_from_user;
#endif
};

#define UPB_DEF_INIT(name, type, vtbl, refs, ref2s) \
    { UPB_REFCOUNT_INIT(vtbl, refs, ref2s), name, NULL, type, false }

UPB_BEGIN_EXTERN_C

/* Include upb_refcounted methods like upb_def_ref()/upb_def_unref(). */
UPB_REFCOUNTED_CMETHODS(upb_def, upb_def_upcast)

upb_deftype_t upb_def_type(const upb_def *d);
const char *upb_def_fullname(const upb_def *d);
const char *upb_def_name(const upb_def *d);
const upb_filedef *upb_def_file(const upb_def *d);
bool upb_def_setfullname(upb_def *def, const char *fullname, upb_status *s);
bool upb_def_freeze(upb_def *const *defs, size_t n, upb_status *s);

/* Temporary API: for internal use only. */
bool _upb_def_validate(upb_def *const*defs, size_t n, upb_status *s);

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
    UPB_ASSERT(upb_def_type(def) == UPB_DEF_##upper);                          \
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

#undef UPB_DECLARE_DEF_TYPE
#undef UPB_DEF_CASTS
#undef UPB_CPP_CASTS


/* upb::FieldDef **************************************************************/

/* The types a field can have.  Note that this list is not identical to the
 * types defined in descriptor.proto, which gives INT32 and SINT32 separate
 * types (we distinguish the two with the "integer encoding" enum below). */
typedef enum {
  /* Types stored in 1 byte. */
  UPB_TYPE_BOOL     = 1,
  /* Types stored in 4 bytes. */
  UPB_TYPE_FLOAT    = 2,
  UPB_TYPE_INT32    = 3,
  UPB_TYPE_UINT32   = 4,
  UPB_TYPE_ENUM     = 5,  /* Enum values are int32. */
  /* Types stored as pointers (probably 4 or 8 bytes). */
  UPB_TYPE_STRING   = 6,
  UPB_TYPE_BYTES    = 7,
  UPB_TYPE_MESSAGE  = 8,
  /* Types stored as 8 bytes. */
  UPB_TYPE_DOUBLE   = 9,
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

typedef enum {
  UPB_SYNTAX_PROTO2 = 2,
  UPB_SYNTAX_PROTO3 = 3
} upb_syntax_t;

/* All the different kind of well known type messages. For simplicity of check,
 * number wrappers and string wrappers are grouped together. Make sure the
 * order and merber of these groups are not changed.
 */
typedef enum {
  UPB_WELLKNOWN_UNSPECIFIED,
  UPB_WELLKNOWN_ANY,
  UPB_WELLKNOWN_FIELDMASK,
  UPB_WELLKNOWN_DURATION,
  UPB_WELLKNOWN_TIMESTAMP,
  /* number wrappers */
  UPB_WELLKNOWN_DOUBLEVALUE,
  UPB_WELLKNOWN_FLOATVALUE,
  UPB_WELLKNOWN_INT64VALUE,
  UPB_WELLKNOWN_UINT64VALUE,
  UPB_WELLKNOWN_INT32VALUE,
  UPB_WELLKNOWN_UINT32VALUE,
  /* string wrappers */
  UPB_WELLKNOWN_STRINGVALUE,
  UPB_WELLKNOWN_BYTESVALUE,
  UPB_WELLKNOWN_BOOLVALUE,
  UPB_WELLKNOWN_VALUE,
  UPB_WELLKNOWN_LISTVALUE,
  UPB_WELLKNOWN_STRUCT
} upb_wellknowntype_t;


/* Maps descriptor type -> upb field type.  */
extern const uint8_t upb_desctype_to_fieldtype[];

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

  /* Copies the JSON name for this field into the given buffer.  Returns the
   * actual size of the JSON name, including the NULL terminator.  If the
   * return value is 0, the JSON name is unset.  If the return value is
   * greater than len, the JSON name was truncated.  The buffer is always
   * NULL-terminated if len > 0.
   *
   * The JSON name always defaults to a camelCased version of the regular
   * name.  However if the regular name is unset, the JSON name will be unset
   * also.
   */
  size_t GetJsonName(char* buf, size_t len) const;

  /* Convenience version of the above function which copies the JSON name
   * into the given string, returning false if the name is not set. */
  template <class T>
  bool GetJsonName(T* str) {
    str->resize(GetJsonName(NULL, 0));
    GetJsonName(&(*str)[0], str->size());
    return str->size() > 0;
  }

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
  uint32_t index() const;

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

  /* Returns whether this field explicitly represents presence.
   *
   * For proto2 messages: Returns true for any scalar (non-repeated) field.
   * For proto3 messages: Returns true for scalar submessage or oneof fields. */
  bool HasPresence() const;

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

  /* Sets the JSON name to the given string. */
  /* TODO(haberman): implement.  Right now only default json_name (camelCase)
   * is supported. */
  bool set_json_name(const char* json_name, upb::Status* s);
  bool set_json_name(const std::string& name, upb::Status* s);

  /* Clears the JSON name. This will make it revert to its default, which is
   * a camelCased version of the regular field name. */
  void clear_json_name();

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
#else
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
# endif  /* defined(__cplusplus) */
};

UPB_BEGIN_EXTERN_C

extern const struct upb_refcounted_vtbl upb_fielddef_vtbl;

#define UPB_FIELDDEF_INIT(label, type, intfmt, tagdelim, is_extension, lazy,   \
                          packed, name, num, msgdef, subdef, selector_base,    \
                          index, defaultval, refs, ref2s)                      \
  {                                                                            \
    UPB_DEF_INIT(name, UPB_DEF_FIELD, &upb_fielddef_vtbl, refs, ref2s),        \
        defaultval, {msgdef}, {subdef}, NULL, false, false,                    \
        type == UPB_TYPE_STRING || type == UPB_TYPE_BYTES, true, is_extension, \
        lazy, packed, intfmt, tagdelim, type, label, num, selector_base, index \
  }

/* Native C API. */
upb_fielddef *upb_fielddef_new(const void *owner);

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
size_t upb_fielddef_getjsonname(const upb_fielddef *f, char *buf, size_t len);
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
bool upb_fielddef_haspresence(const upb_fielddef *f);
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
bool upb_fielddef_setjsonname(upb_fielddef *f, const char *name, upb_status *s);
bool upb_fielddef_clearjsonname(upb_fielddef *f);
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

/* Well-known field tag numbers for map-entry messages. */
#define UPB_MAPENTRY_KEY   1
#define UPB_MAPENTRY_VALUE 2

/* Well-known field tag numbers for Any messages. */
#define UPB_ANY_TYPE 1
#define UPB_ANY_VALUE 2

/* Well-known field tag numbers for timestamp messages. */
#define UPB_DURATION_SECONDS 1
#define UPB_DURATION_NANOS 2

/* Well-known field tag numbers for duration messages. */
#define UPB_TIMESTAMP_SECONDS 1
#define UPB_TIMESTAMP_NANOS 2

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
  const char* name() const;
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

  upb_syntax_t syntax() const;

  /* Returns false if we don't support this syntax value. */
  bool set_syntax(upb_syntax_t syntax);

  /* Set this to false to indicate that primitive fields should not have
   * explicit presence information associated with them.  This will affect all
   * fields added to this message.  Defaults to true. */
  void SetPrimitivesHavePresence(bool have_presence);

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

  /* Is this message a map entry? */
  void setmapentry(bool map_entry);
  bool mapentry() const;

  /* Return the type of well known type message. UPB_WELLKNOWN_UNSPECIFIED for
   * non-well-known message. */
  upb_wellknowntype_t wellknowntype() const;

  /* Whether is a number wrapper. */
  bool isnumberwrapper() const;

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
#else
struct upb_msgdef {
  upb_def base;

  size_t selector_count;
  uint32_t submsg_field_count;

  /* Tables for looking up fields by number and name. */
  upb_inttable itof;  /* int to field */
  upb_strtable ntof;  /* name to field/oneof */

  /* Is this a map-entry message? */
  bool map_entry;

  /* Whether this message has proto2 or proto3 semantics. */
  upb_syntax_t syntax;

  /* Type of well known type message. UPB_WELLKNOWN_UNSPECIFIED for
   * non-well-known message. */
  upb_wellknowntype_t well_known_type;

  /* TODO(haberman): proper extension ranges (there can be multiple). */
#endif  /* __cplusplus */
};

UPB_BEGIN_EXTERN_C

extern const struct upb_refcounted_vtbl upb_msgdef_vtbl;

/* TODO: also support static initialization of the oneofs table. This will be
 * needed if we compile in descriptors that contain oneofs. */
#define UPB_MSGDEF_INIT(name, selector_count, submsg_field_count, itof, ntof, \
                        map_entry, syntax, well_known_type, refs, ref2s)      \
  {                                                                           \
    UPB_DEF_INIT(name, UPB_DEF_MSG, &upb_fielddef_vtbl, refs, ref2s),         \
        selector_count, submsg_field_count, itof, ntof, map_entry, syntax,    \
        well_known_type                                                       \
  }

/* Returns NULL if memory allocation failed. */
upb_msgdef *upb_msgdef_new(const void *owner);

/* Include upb_refcounted methods like upb_msgdef_ref(). */
UPB_REFCOUNTED_CMETHODS(upb_msgdef, upb_msgdef_upcast2)

bool upb_msgdef_freeze(upb_msgdef *m, upb_status *status);

const char *upb_msgdef_fullname(const upb_msgdef *m);
const char *upb_msgdef_name(const upb_msgdef *m);
int upb_msgdef_numoneofs(const upb_msgdef *m);
upb_syntax_t upb_msgdef_syntax(const upb_msgdef *m);

bool upb_msgdef_addfield(upb_msgdef *m, upb_fielddef *f, const void *ref_donor,
                         upb_status *s);
bool upb_msgdef_addoneof(upb_msgdef *m, upb_oneofdef *o, const void *ref_donor,
                         upb_status *s);
bool upb_msgdef_setfullname(upb_msgdef *m, const char *fullname, upb_status *s);
void upb_msgdef_setmapentry(upb_msgdef *m, bool map_entry);
bool upb_msgdef_mapentry(const upb_msgdef *m);
upb_wellknowntype_t upb_msgdef_wellknowntype(const upb_msgdef *m);
bool upb_msgdef_isnumberwrapper(const upb_msgdef *m);
bool upb_msgdef_setsyntax(upb_msgdef *m, upb_syntax_t syntax);

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

/* Lookup of either field or oneof by name.  Returns whether either was found.
 * If the return is true, then the found def will be set, and the non-found
 * one set to NULL. */
bool upb_msgdef_lookupname(const upb_msgdef *m, const char *name, size_t len,
                           const upb_fielddef **f, const upb_oneofdef **o);

UPB_INLINE bool upb_msgdef_lookupnamez(const upb_msgdef *m, const char *name,
                                       const upb_fielddef **f,
                                       const upb_oneofdef **o) {
  return upb_msgdef_lookupname(m, name, strlen(name), f, o);
}

/* Iteration over fields and oneofs.  For example:
 *
 * upb_msg_field_iter i;
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
  const char* name() const;
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
#else
struct upb_enumdef {
  upb_def base;

  upb_strtable ntoi;
  upb_inttable iton;
  int32_t defaultval;
#endif  /* __cplusplus */
};

UPB_BEGIN_EXTERN_C

extern const struct upb_refcounted_vtbl upb_enumdef_vtbl;

#define UPB_ENUMDEF_INIT(name, ntoi, iton, defaultval, refs, ref2s) \
  { UPB_DEF_INIT(name, UPB_DEF_ENUM, &upb_enumdef_vtbl, refs, ref2s), ntoi,    \
    iton, defaultval }

/* Native C API. */
upb_enumdef *upb_enumdef_new(const void *owner);

/* Include upb_refcounted methods like upb_enumdef_ref(). */
UPB_REFCOUNTED_CMETHODS(upb_enumdef, upb_enumdef_upcast2)

bool upb_enumdef_freeze(upb_enumdef *e, upb_status *status);

/* From upb_def. */
const char *upb_enumdef_fullname(const upb_enumdef *e);
const char *upb_enumdef_name(const upb_enumdef *e);
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

/* Class that represents a oneof. */
class upb::OneofDef {
 public:
  /* Returns NULL if memory allocation failed. */
  static reffed_ptr<OneofDef> New();

  /* upb::RefCounted methods like Ref()/Unref(). */
  UPB_REFCOUNTED_CPPMETHODS

  /* Returns the MessageDef that owns this OneofDef. */
  const MessageDef* containing_type() const;

  /* Returns the name of this oneof. This is the name used to look up the oneof
   * by name once added to a message def. */
  const char* name() const;
  bool set_name(const char* name, Status* s);
  bool set_name(const std::string& name, Status* s);

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
#else
struct upb_oneofdef {
  upb_refcounted base;

  uint32_t index;  /* Index within oneofs. */
  const char *name;
  upb_strtable ntof;
  upb_inttable itof;
  const upb_msgdef *parent;
#endif  /* __cplusplus */
};

UPB_BEGIN_EXTERN_C

extern const struct upb_refcounted_vtbl upb_oneofdef_vtbl;

#define UPB_ONEOFDEF_INIT(name, ntof, itof, refs, ref2s) \
  { UPB_REFCOUNT_INIT(&upb_oneofdef_vtbl, refs, ref2s), 0, name, ntof, itof }

/* Native C API. */
upb_oneofdef *upb_oneofdef_new(const void *owner);

/* Include upb_refcounted methods like upb_oneofdef_ref(). */
UPB_REFCOUNTED_CMETHODS(upb_oneofdef, upb_oneofdef_upcast)

const char *upb_oneofdef_name(const upb_oneofdef *o);
const upb_msgdef *upb_oneofdef_containingtype(const upb_oneofdef *o);
int upb_oneofdef_numfields(const upb_oneofdef *o);
uint32_t upb_oneofdef_index(const upb_oneofdef *o);

bool upb_oneofdef_setname(upb_oneofdef *o, const char *name, upb_status *s);
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


/* upb::FileDef ***************************************************************/

#ifdef __cplusplus

/* Class that represents a .proto file with some things defined in it.
 *
 * Many users won't care about FileDefs, but they are necessary if you want to
 * read the values of file-level options. */
class upb::FileDef {
 public:
  /* Returns NULL if memory allocation failed. */
  static reffed_ptr<FileDef> New();

  /* upb::RefCounted methods like Ref()/Unref(). */
  UPB_REFCOUNTED_CPPMETHODS

  /* Get/set name of the file (eg. "foo/bar.proto"). */
  const char* name() const;
  bool set_name(const char* name, Status* s);
  bool set_name(const std::string& name, Status* s);

  /* Package name for definitions inside the file (eg. "foo.bar"). */
  const char* package() const;
  bool set_package(const char* package, Status* s);

  /* Sets the php class prefix which is prepended to all php generated classes
   * from this .proto. Default is empty. */
  const char* phpprefix() const;
  bool set_phpprefix(const char* phpprefix, Status* s);

  /* Use this option to change the namespace of php generated classes. Default
   * is empty. When this option is empty, the package name will be used for
   * determining the namespace. */
  const char* phpnamespace() const;
  bool set_phpnamespace(const char* phpnamespace, Status* s);

  /* Syntax for the file.  Defaults to proto2. */
  upb_syntax_t syntax() const;
  void set_syntax(upb_syntax_t syntax);

  /* Get the list of defs from the file.  These are returned in the order that
   * they were added to the FileDef. */
  int def_count() const;
  const Def* def(int index) const;
  Def* def(int index);

  /* Get the list of dependencies from the file.  These are returned in the
   * order that they were added to the FileDef. */
  int dependency_count() const;
  const FileDef* dependency(int index) const;

  /* Adds defs to this file.  The def must not already belong to another
   * file.
   *
   * Note: this does *not* ensure that this def's name is unique in this file!
   * Use a SymbolTable if you want to check this property.  Especially since
   * properly checking uniqueness would require a check across *all* files
   * (including dependencies). */
  bool AddDef(Def* def, Status* s);
  bool AddMessage(MessageDef* m, Status* s);
  bool AddEnum(EnumDef* e, Status* s);
  bool AddExtension(FieldDef* f, Status* s);

  /* Adds a dependency of this file. */
  bool AddDependency(const FileDef* file);

  /* Freezes this FileDef and all messages/enums under it.  All subdefs must be
   * resolved and all messages/enums must validate.  Returns true if this
   * succeeded.
   *
   * TODO(haberman): should we care whether the file's dependencies are frozen
   * already? */
  bool Freeze(Status* s);

 private:
  UPB_DISALLOW_POD_OPS(FileDef, upb::FileDef)
#else
struct upb_filedef {
  upb_refcounted base;

  const char *name;
  const char *package;
  const char *phpprefix;
  const char *phpnamespace;
  upb_syntax_t syntax;

  upb_inttable defs;
  upb_inttable deps;
#endif
};

UPB_BEGIN_EXTERN_C

extern const struct upb_refcounted_vtbl upb_filedef_vtbl;

upb_filedef *upb_filedef_new(const void *owner);

/* Include upb_refcounted methods like upb_msgdef_ref(). */
UPB_REFCOUNTED_CMETHODS(upb_filedef, upb_filedef_upcast)

const char *upb_filedef_name(const upb_filedef *f);
const char *upb_filedef_package(const upb_filedef *f);
const char *upb_filedef_phpprefix(const upb_filedef *f);
const char *upb_filedef_phpnamespace(const upb_filedef *f);
upb_syntax_t upb_filedef_syntax(const upb_filedef *f);
size_t upb_filedef_defcount(const upb_filedef *f);
size_t upb_filedef_depcount(const upb_filedef *f);
const upb_def *upb_filedef_def(const upb_filedef *f, size_t i);
const upb_filedef *upb_filedef_dep(const upb_filedef *f, size_t i);

bool upb_filedef_freeze(upb_filedef *f, upb_status *s);
bool upb_filedef_setname(upb_filedef *f, const char *name, upb_status *s);
bool upb_filedef_setpackage(upb_filedef *f, const char *package, upb_status *s);
bool upb_filedef_setphpprefix(upb_filedef *f, const char *phpprefix,
                              upb_status *s);
bool upb_filedef_setphpnamespace(upb_filedef *f, const char *phpnamespace,
                                 upb_status *s);
bool upb_filedef_setsyntax(upb_filedef *f, upb_syntax_t syntax, upb_status *s);

bool upb_filedef_adddef(upb_filedef *f, upb_def *def, const void *ref_donor,
                        upb_status *s);
bool upb_filedef_adddep(upb_filedef *f, const upb_filedef *dep);

UPB_INLINE bool upb_filedef_addmsg(upb_filedef *f, upb_msgdef *m,
                                   const void *ref_donor, upb_status *s) {
  return upb_filedef_adddef(f, upb_msgdef_upcast_mutable(m), ref_donor, s);
}

UPB_INLINE bool upb_filedef_addenum(upb_filedef *f, upb_enumdef *e,
                                    const void *ref_donor, upb_status *s) {
  return upb_filedef_adddef(f, upb_enumdef_upcast_mutable(e), ref_donor, s);
}

UPB_INLINE bool upb_filedef_addext(upb_filedef *file, upb_fielddef *f,
                                   const void *ref_donor, upb_status *s) {
  return upb_filedef_adddef(file, upb_fielddef_upcast_mutable(f), ref_donor, s);
}
UPB_INLINE upb_def *upb_filedef_mutabledef(upb_filedef *f, int i) {
  return (upb_def*)upb_filedef_def(f, i);
}

UPB_END_EXTERN_C

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
  static SymbolTable* New();
  static void Free(upb::SymbolTable* table);

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

  /* Adds the given mutable defs to the symtab, resolving all symbols (including
   * enum default values) and finalizing the defs.  Only one def per name may be
   * in the list, and the defs may not duplicate any name already in the symtab.
   * All defs must have a name -- anonymous defs are not allowed.  Anonymous
   * defs can still be frozen by calling upb_def_freeze() directly.
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
   * succeed. */
  bool Add(Def*const* defs, size_t n, void* ref_donor, Status* status);

  bool Add(const std::vector<Def*>& defs, void *owner, Status* status) {
    return Add((Def*const*)&defs[0], defs.size(), owner, status);
  }

  /* Resolves all subdefs for messages in this file and attempts to freeze the
   * file.  If this succeeds, adds all the symbols to this SymbolTable
   * (replacing any existing ones with the same names). */
  bool AddFile(FileDef* file, Status* s);

 private:
  UPB_DISALLOW_POD_OPS(SymbolTable, upb::SymbolTable)
#else
struct upb_symtab {
  upb_refcounted base;

  upb_strtable symtab;
#endif  /* __cplusplus */
};

UPB_BEGIN_EXTERN_C

/* Native C API. */

upb_symtab *upb_symtab_new();
void upb_symtab_free(upb_symtab* s);
const upb_def *upb_symtab_resolve(const upb_symtab *s, const char *base,
                                  const char *sym);
const upb_def *upb_symtab_lookup(const upb_symtab *s, const char *sym);
const upb_msgdef *upb_symtab_lookupmsg(const upb_symtab *s, const char *sym);
const upb_msgdef *upb_symtab_lookupmsg2(
    const upb_symtab *s, const char *sym, size_t len);
const upb_enumdef *upb_symtab_lookupenum(const upb_symtab *s, const char *sym);
bool upb_symtab_add(upb_symtab *s, upb_def *const*defs, size_t n,
                    void *ref_donor, upb_status *status);
bool upb_symtab_addfile(upb_symtab *s, upb_filedef *file, upb_status* status);

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
inline SymbolTable* SymbolTable::New() {
  return upb_symtab_new();
}
inline void SymbolTable::Free(SymbolTable* s) {
  upb_symtab_free(s);
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
    Def*const* defs, size_t n, void* ref_donor, Status* status) {
  return upb_symtab_add(this, (upb_def*const*)defs, n, ref_donor, status);
}
inline bool SymbolTable::AddFile(FileDef* file, Status* s) {
  return upb_symtab_addfile(this, file, s);
}
}  /* namespace upb */
#endif

#ifdef __cplusplus

UPB_INLINE const char* upb_safecstr(const std::string& str) {
  UPB_ASSERT(str.size() == std::strlen(str.c_str()));
  return str.c_str();
}

/* Inline C++ wrappers. */
namespace upb {

inline Def::Type Def::def_type() const { return upb_def_type(this); }
inline const char* Def::full_name() const { return upb_def_fullname(this); }
inline const char* Def::name() const { return upb_def_name(this); }
inline bool Def::set_full_name(const char* fullname, Status* s) {
  return upb_def_setfullname(this, fullname, s);
}
inline bool Def::set_full_name(const std::string& fullname, Status* s) {
  return upb_def_setfullname(this, upb_safecstr(fullname), s);
}
inline bool Def::Freeze(Def* const* defs, size_t n, Status* status) {
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
  UPB_ASSERT(CheckType(val));
  return static_cast<FieldDef::Type>(val);
}
inline FieldDef::Label FieldDef::ConvertLabel(int32_t val) {
  UPB_ASSERT(CheckLabel(val));
  return static_cast<FieldDef::Label>(val);
}
inline FieldDef::DescriptorType FieldDef::ConvertDescriptorType(int32_t val) {
  UPB_ASSERT(CheckDescriptorType(val));
  return static_cast<FieldDef::DescriptorType>(val);
}
inline FieldDef::IntegerFormat FieldDef::ConvertIntegerFormat(int32_t val) {
  UPB_ASSERT(CheckIntegerFormat(val));
  return static_cast<FieldDef::IntegerFormat>(val);
}

inline reffed_ptr<FieldDef> FieldDef::New() {
  upb_fielddef *f = upb_fielddef_new(&f);
  return reffed_ptr<FieldDef>(f, &f);
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
inline size_t FieldDef::GetJsonName(char* buf, size_t len) const {
  return upb_fielddef_getjsonname(this, buf, len);
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
inline uint32_t FieldDef::index() const {
  return upb_fielddef_index(this);
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
inline bool FieldDef::set_json_name(const char *name, Status* s) {
  return upb_fielddef_setjsonname(this, name, s);
}
inline bool FieldDef::set_json_name(const std::string& name, Status* s) {
  return upb_fielddef_setjsonname(this, upb_safecstr(name), s);
}
inline void FieldDef::clear_json_name() {
  upb_fielddef_clearjsonname(this);
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
inline const char *MessageDef::name() const {
  return upb_msgdef_name(this);
}
inline upb_syntax_t MessageDef::syntax() const {
  return upb_msgdef_syntax(this);
}
inline bool MessageDef::set_full_name(const char* fullname, Status* s) {
  return upb_msgdef_setfullname(this, fullname, s);
}
inline bool MessageDef::set_full_name(const std::string& fullname, Status* s) {
  return upb_msgdef_setfullname(this, upb_safecstr(fullname), s);
}
inline bool MessageDef::set_syntax(upb_syntax_t syntax) {
  return upb_msgdef_setsyntax(this, syntax);
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
inline void MessageDef::setmapentry(bool map_entry) {
  upb_msgdef_setmapentry(this, map_entry);
}
inline bool MessageDef::mapentry() const {
  return upb_msgdef_mapentry(this);
}
inline upb_wellknowntype_t MessageDef::wellknowntype() const {
  return upb_msgdef_wellknowntype(this);
}
inline bool MessageDef::isnumberwrapper() const {
  return upb_msgdef_isnumberwrapper(this);
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
inline const char* EnumDef::name() const {
  return upb_enumdef_name(this);
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

inline const MessageDef* OneofDef::containing_type() const {
  return upb_oneofdef_containingtype(this);
}
inline const char* OneofDef::name() const {
  return upb_oneofdef_name(this);
}
inline bool OneofDef::set_name(const char* name, Status* s) {
  return upb_oneofdef_setname(this, name, s);
}
inline bool OneofDef::set_name(const std::string& name, Status* s) {
  return upb_oneofdef_setname(this, upb_safecstr(name), s);
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

inline reffed_ptr<FileDef> FileDef::New() {
  upb_filedef *f = upb_filedef_new(&f);
  return reffed_ptr<FileDef>(f, &f);
}

inline const char* FileDef::name() const {
  return upb_filedef_name(this);
}
inline bool FileDef::set_name(const char* name, Status* s) {
  return upb_filedef_setname(this, name, s);
}
inline bool FileDef::set_name(const std::string& name, Status* s) {
  return upb_filedef_setname(this, upb_safecstr(name), s);
}
inline const char* FileDef::package() const {
  return upb_filedef_package(this);
}
inline bool FileDef::set_package(const char* package, Status* s) {
  return upb_filedef_setpackage(this, package, s);
}
inline const char* FileDef::phpprefix() const {
  return upb_filedef_phpprefix(this);
}
inline bool FileDef::set_phpprefix(const char* phpprefix, Status* s) {
  return upb_filedef_setphpprefix(this, phpprefix, s);
}
inline const char* FileDef::phpnamespace() const {
  return upb_filedef_phpnamespace(this);
}
inline bool FileDef::set_phpnamespace(const char* phpnamespace, Status* s) {
  return upb_filedef_setphpnamespace(this, phpnamespace, s);
}
inline int FileDef::def_count() const {
  return upb_filedef_defcount(this);
}
inline const Def* FileDef::def(int index) const {
  return upb_filedef_def(this, index);
}
inline Def* FileDef::def(int index) {
  return const_cast<Def*>(upb_filedef_def(this, index));
}
inline int FileDef::dependency_count() const {
  return upb_filedef_depcount(this);
}
inline const FileDef* FileDef::dependency(int index) const {
  return upb_filedef_dep(this, index);
}
inline bool FileDef::AddDef(Def* def, Status* s) {
  return upb_filedef_adddef(this, def, NULL, s);
}
inline bool FileDef::AddMessage(MessageDef* m, Status* s) {
  return upb_filedef_addmsg(this, m, NULL, s);
}
inline bool FileDef::AddEnum(EnumDef* e, Status* s) {
  return upb_filedef_addenum(this, e, NULL, s);
}
inline bool FileDef::AddExtension(FieldDef* f, Status* s) {
  return upb_filedef_addext(this, f, NULL, s);
}
inline bool FileDef::AddDependency(const FileDef* file) {
  return upb_filedef_adddep(this, file);
}

}  /* namespace upb */
#endif

#endif /* UPB_DEF_H_ */
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
#define UPB_UNKNOWN_SELECTOR 2
#define UPB_STATIC_SELECTOR_COUNT 3

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
      UPB_ASSERT(ok);
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
bool upb_handlers_setunknown(upb_handlers *h, upb_unknown_handlerfunc *func,
                             upb_handlerattr *attr);

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


/** Message handlers ******************************************************************/

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
    UPB_ASSERT(!handler.registered_);                                              \
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
  UPB_ASSERT(registered_);
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
  UPB_ASSERT(!handler.registered_);
  handler.registered_ = true;
  handler.AddCleanup(this);
  return upb_handlers_setstartmsg(this, handler.handler_, &handler.attr_);
}
inline bool Handlers::SetEndMessageHandler(
    const Handlers::EndMessageHandler &handler) {
  UPB_ASSERT(!handler.registered_);
  handler.registered_ = true;
  handler.AddCleanup(this);
  return upb_handlers_setendmsg(this, handler.handler_, &handler.attr_);
}
inline bool Handlers::SetStartStringHandler(const FieldDef *f,
                                            const StartStringHandler &handler) {
  UPB_ASSERT(!handler.registered_);
  handler.registered_ = true;
  handler.AddCleanup(this);
  return upb_handlers_setstartstr(this, f, handler.handler_, &handler.attr_);
}
inline bool Handlers::SetEndStringHandler(const FieldDef *f,
                                          const EndFieldHandler &handler) {
  UPB_ASSERT(!handler.registered_);
  handler.registered_ = true;
  handler.AddCleanup(this);
  return upb_handlers_setendstr(this, f, handler.handler_, &handler.attr_);
}
inline bool Handlers::SetStringHandler(const FieldDef *f,
                                       const StringHandler& handler) {
  UPB_ASSERT(!handler.registered_);
  handler.registered_ = true;
  handler.AddCleanup(this);
  return upb_handlers_setstring(this, f, handler.handler_, &handler.attr_);
}
inline bool Handlers::SetStartSequenceHandler(
    const FieldDef *f, const StartFieldHandler &handler) {
  UPB_ASSERT(!handler.registered_);
  handler.registered_ = true;
  handler.AddCleanup(this);
  return upb_handlers_setstartseq(this, f, handler.handler_, &handler.attr_);
}
inline bool Handlers::SetStartSubMessageHandler(
    const FieldDef *f, const StartFieldHandler &handler) {
  UPB_ASSERT(!handler.registered_);
  handler.registered_ = true;
  handler.AddCleanup(this);
  return upb_handlers_setstartsubmsg(this, f, handler.handler_, &handler.attr_);
}
inline bool Handlers::SetEndSubMessageHandler(const FieldDef *f,
                                              const EndFieldHandler &handler) {
  UPB_ASSERT(!handler.registered_);
  handler.registered_ = true;
  handler.AddCleanup(this);
  return upb_handlers_setendsubmsg(this, f, handler.handler_, &handler.attr_);
}
inline bool Handlers::SetEndSequenceHandler(const FieldDef *f,
                                            const EndFieldHandler &handler) {
  UPB_ASSERT(!handler.registered_);
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
class BufferSink;
class BufferSource;
class BytesSink;
class Sink;
}
#endif

UPB_DECLARE_TYPE(upb::BufferSink, upb_bufsink)
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

/* A class for accumulating output string data in a flat buffer. */

upb_bufsink *upb_bufsink_new(upb_env *env);
void upb_bufsink_free(upb_bufsink *sink);
upb_bytessink *upb_bufsink_sink(upb_bufsink *sink);
const char *upb_bufsink_getdata(const upb_bufsink *sink, size_t *len);

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

bool upb_bufsrc_putbuf(const char *buf, size_t len, upb_bytessink *sink);

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

UPB_INLINE bool upb_sink_putunknown(upb_sink *s, const char *buf, size_t n) {
  typedef upb_unknown_handlerfunc func;
  func *handler;
  const void *hd;
  if (!s->handlers) return true;
  handler = (func *)upb_handlers_gethandler(s->handlers, UPB_UNKNOWN_SELECTOR);

  if (!handler) return n;
  hd = upb_handlers_gethandlerdata(s->handlers, UPB_UNKNOWN_SELECTOR);
  return handler(s->closure, hd, buf, n);
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

#ifdef __cplusplus

namespace upb {
class Array;
class Map;
class MapIterator;
class MessageLayout;
}

#endif

UPB_DECLARE_TYPE(upb::Map, upb_map)
UPB_DECLARE_TYPE(upb::MapIterator, upb_mapiter)

struct upb_array;
typedef struct upb_array upb_array;

/* TODO(haberman): C++ accessors */

UPB_BEGIN_EXTERN_C

typedef void upb_msg;


/** upb_msglayout *************************************************************/

/* upb_msglayout represents the memory layout of a given upb_msgdef.  The
 * members are public so generated code can initialize them, but users MUST NOT
 * read or write any of its members. */

typedef struct {
  uint32_t number;
  uint16_t offset;
  int16_t presence;      /* If >0, hasbit_index+1.  If <0, oneof_index+1. */
  uint16_t submsg_index;  /* undefined if descriptortype != MESSAGE or GROUP. */
  uint8_t descriptortype;
  uint8_t label;
} upb_msglayout_field;

typedef struct upb_msglayout {
  const struct upb_msglayout *const* submsgs;
  const upb_msglayout_field *fields;
  /* Must be aligned to sizeof(void*).  Doesn't include internal members like
   * unknown fields, extension dict, pointer to msglayout, etc. */
  uint16_t size;
  uint16_t field_count;
  bool extendable;
} upb_msglayout;


/** upb_strview ************************************************************/

typedef struct {
  const char *data;
  size_t size;
} upb_strview;

UPB_INLINE upb_strview upb_strview_make(const char *data, size_t size) {
  upb_strview ret;
  ret.data = data;
  ret.size = size;
  return ret;
}

#define UPB_STRVIEW_INIT(ptr, len) {ptr, len}


/** upb_msgval ****************************************************************/

/* A union representing all possible protobuf values.  Used for generic get/set
 * operations. */

typedef union {
  bool b;
  float flt;
  double dbl;
  int32_t i32;
  int64_t i64;
  uint32_t u32;
  uint64_t u64;
  const upb_map* map;
  const upb_msg* msg;
  const upb_array* arr;
  const void* ptr;
  upb_strview str;
} upb_msgval;

#define ACCESSORS(name, membername, ctype) \
  UPB_INLINE ctype upb_msgval_get ## name(upb_msgval v) { \
    return v.membername; \
  } \
  UPB_INLINE void upb_msgval_set ## name(upb_msgval *v, ctype cval) { \
    v->membername = cval; \
  } \
  UPB_INLINE upb_msgval upb_msgval_ ## name(ctype v) { \
    upb_msgval ret; \
    ret.membername = v; \
    return ret; \
  }

ACCESSORS(bool,   b,   bool)
ACCESSORS(float,  flt, float)
ACCESSORS(double, dbl, double)
ACCESSORS(int32,  i32, int32_t)
ACCESSORS(int64,  i64, int64_t)
ACCESSORS(uint32, u32, uint32_t)
ACCESSORS(uint64, u64, uint64_t)
ACCESSORS(map,    map, const upb_map*)
ACCESSORS(msg,    msg, const upb_msg*)
ACCESSORS(ptr,    ptr, const void*)
ACCESSORS(arr,    arr, const upb_array*)
ACCESSORS(str,    str, upb_strview)

#undef ACCESSORS

UPB_INLINE upb_msgval upb_msgval_makestr(const char *data, size_t size) {
  return upb_msgval_str(upb_strview_make(data, size));
}


/** upb_msg *******************************************************************/

/* A upb_msg represents a protobuf message.  It always corresponds to a specific
 * upb_msglayout, which describes how it is laid out in memory.  */

/* Creates a new message of the given type/layout in this arena. */
upb_msg *upb_msg_new(const upb_msglayout *l, upb_arena *a);

/* Returns the arena for the given message. */
upb_arena *upb_msg_arena(const upb_msg *msg);

void upb_msg_addunknown(upb_msg *msg, const char *data, size_t len);
const char *upb_msg_getunknown(const upb_msg *msg, size_t *len);

/* Read-only message API.  Can be safely called by anyone. */

/* Returns the value associated with this field:
 *   - for scalar fields (including strings), the value directly.
 *   - return upb_msg*, or upb_map* for msg/map.
 *     If the field is unset for these field types, returns NULL.
 *
 * TODO(haberman): should we let users store cached array/map/msg
 * pointers here for fields that are unset?  Could be useful for the
 * strongly-owned submessage model (ie. generated C API that doesn't use
 * arenas).
 */
upb_msgval upb_msg_get(const upb_msg *msg,
                       int field_index,
                       const upb_msglayout *l);

/* May only be called for fields where upb_fielddef_haspresence(f) == true. */
bool upb_msg_has(const upb_msg *msg,
                 int field_index,
                 const upb_msglayout *l);

/* Mutable message API.  May only be called by the owner of the message who
 * knows its ownership scheme and how to keep it consistent. */

/* Sets the given field to the given value.  Does not perform any memory
 * management: if you overwrite a pointer to a msg/array/map/string without
 * cleaning it up (or using an arena) it will leak.
 */
void upb_msg_set(upb_msg *msg,
                 int field_index,
                 upb_msgval val,
                 const upb_msglayout *l);

/* For a primitive field, set it back to its default. For repeated, string, and
 * submessage fields set it back to NULL.  This could involve releasing some
 * internal memory (for example, from an extension dictionary), but it is not
 * recursive in any way and will not recover any memory that may be used by
 * arrays/maps/strings/msgs that this field may have pointed to.
 */
bool upb_msg_clearfield(upb_msg *msg,
                        int field_index,
                        const upb_msglayout *l);

/* TODO(haberman): copyfrom()/mergefrom()? */


/** upb_array *****************************************************************/

/* A upb_array stores data for a repeated field.  The memory management
 * semantics are the same as upb_msg.  A upb_array allocates dynamic
 * memory internally for the array elements. */

upb_array *upb_array_new(upb_fieldtype_t type, upb_arena *a);
upb_fieldtype_t upb_array_type(const upb_array *arr);

/* Read-only interface.  Safe for anyone to call. */

size_t upb_array_size(const upb_array *arr);
upb_msgval upb_array_get(const upb_array *arr, size_t i);

/* Write interface.  May only be called by the message's owner who can enforce
 * its memory management invariants. */

bool upb_array_set(upb_array *arr, size_t i, upb_msgval val);


/** upb_map *******************************************************************/

/* A upb_map stores data for a map field.  The memory management semantics are
 * the same as upb_msg, with one notable exception.  upb_map will internally
 * store a copy of all string keys, but *not* any string values or submessages.
 * So you must ensure that any string or message values outlive the map, and you
 * must delete them manually when they are no longer required. */

upb_map *upb_map_new(upb_fieldtype_t ktype, upb_fieldtype_t vtype,
                     upb_arena *a);

/* Read-only interface.  Safe for anyone to call. */

size_t upb_map_size(const upb_map *map);
upb_fieldtype_t upb_map_keytype(const upb_map *map);
upb_fieldtype_t upb_map_valuetype(const upb_map *map);
bool upb_map_get(const upb_map *map, upb_msgval key, upb_msgval *val);

/* Write interface.  May only be called by the message's owner who can enforce
 * its memory management invariants. */

/* Sets or overwrites an entry in the map.  Return value indicates whether
 * the operation succeeded or failed with OOM, and also whether an existing
 * key was replaced or not. */
bool upb_map_set(upb_map *map,
                 upb_msgval key, upb_msgval val,
                 upb_msgval *valremoved);

/* Deletes an entry in the map.  Returns true if the key was present. */
bool upb_map_del(upb_map *map, upb_msgval key);


/** upb_mapiter ***************************************************************/

/* For iterating over a map.  Map iterators are invalidated by mutations to the
 * map, but an invalidated iterator will never return junk or crash the process.
 * An invalidated iterator may return entries that were already returned though,
 * and if you keep invalidating the iterator during iteration, the program may
 * enter an infinite loop. */

size_t upb_mapiter_sizeof();

void upb_mapiter_begin(upb_mapiter *i, const upb_map *t);
upb_mapiter *upb_mapiter_new(const upb_map *t, upb_alloc *a);
void upb_mapiter_free(upb_mapiter *i, upb_alloc *a);
void upb_mapiter_next(upb_mapiter *i);
bool upb_mapiter_done(const upb_mapiter *i);

upb_msgval upb_mapiter_key(const upb_mapiter *i);
upb_msgval upb_mapiter_value(const upb_mapiter *i);
void upb_mapiter_setdone(upb_mapiter *i);
bool upb_mapiter_isequal(const upb_mapiter *i1, const upb_mapiter *i2);

UPB_END_EXTERN_C

#endif /* UPB_MSG_H_ */
/* This file was generated by upbc (the upb compiler) from the input
 * file:
 *
 *     google/protobuf/descriptor.proto
 *
 * Do not edit -- your changes will be discarded when the file is
 * regenerated. */

#ifndef GOOGLE_PROTOBUF_DESCRIPTOR_PROTO_UPB_H_
#define GOOGLE_PROTOBUF_DESCRIPTOR_PROTO_UPB_H_

/*
** Functions for use by generated code.  These are not public and users must
** not call them directly.
*/

#ifndef UPB_GENERATED_UTIL_H_
#define UPB_GENERATED_UTIL_H_

#include <stdint.h>
/*
** structs.int.h: structures definitions that are internal to upb.
*/

#ifndef UPB_STRUCTS_H_
#define UPB_STRUCTS_H_


struct upb_array {
  upb_fieldtype_t type;
  uint8_t element_size;
  void *data;   /* Each element is element_size. */
  size_t len;   /* Measured in elements. */
  size_t size;  /* Measured in elements. */
  upb_arena *arena;
};

#endif  /* UPB_STRUCTS_H_ */


#define PTR_AT(msg, ofs, type) (type*)((const char*)msg + ofs)

UPB_INLINE const void *_upb_array_accessor(const void *msg, size_t ofs,
                                           size_t *size) {
  const upb_array *arr = *PTR_AT(msg, ofs, const upb_array*);
  if (arr) {
    if (size) *size = arr->size;
    return arr->data;
  } else {
    if (size) *size = 0;
    return NULL;
  }
}

UPB_INLINE void *_upb_array_mutable_accessor(void *msg, size_t ofs,
                                             size_t *size) {
  upb_array *arr = *PTR_AT(msg, ofs, upb_array*);
  if (arr) {
    if (size) *size = arr->size;
    return arr->data;
  } else {
    if (size) *size = 0;
    return NULL;
  }
}

/* TODO(haberman): this is a mess.  It will improve when upb_array no longer
 * carries reflective state (type, elem_size). */
UPB_INLINE void *_upb_array_resize_accessor(void *msg, size_t ofs, size_t size,
                                            size_t elem_size,
                                            upb_fieldtype_t type,
                                            upb_arena *arena) {
  upb_array *arr = *PTR_AT(msg, ofs, upb_array*);

  if (!arr) {
    arr = upb_array_new(type, arena);
    if (!arr) return NULL;
    *PTR_AT(msg, ofs, upb_array*) = arr;
  }

  if (size > arr->size) {
    size_t new_size = UPB_MAX(arr->size, 4);
    size_t old_bytes = arr->size * elem_size;
    size_t new_bytes;
    upb_alloc *alloc = upb_arena_alloc(arr->arena);
    while (new_size < size) new_size *= 2;
    new_bytes = new_size * elem_size;
    arr->data = upb_realloc(alloc, arr->data, old_bytes, new_bytes);
    if (!arr->data) {
      return NULL;
    }
    arr->size = new_size;
  }

  arr->len = size;
  return arr->data;
}

UPB_INLINE bool _upb_array_append_accessor(void *msg, size_t ofs,
                                           size_t elem_size,
                                           upb_fieldtype_t type,
                                           const void *value,
                                           upb_arena *arena) {
  upb_array *arr = *PTR_AT(msg, ofs, upb_array*);
  size_t i = arr ? arr->len : 0;
  void *data =
      _upb_array_resize_accessor(msg, ofs, i + 1, elem_size, type, arena);
  if (!data) return false;
  memcpy(PTR_AT(data, i * elem_size, char), value, elem_size);
  return true;
}

UPB_INLINE bool _upb_has_field(const void *msg, size_t idx) {
  return (*PTR_AT(msg, idx / 8, const char) & (idx % 8)) != 0;
}

UPB_INLINE bool _upb_sethas(const void *msg, size_t idx) {
  return (*PTR_AT(msg, idx / 8, char)) |= (1 << (idx % 8));
}

UPB_INLINE bool _upb_clearhas(const void *msg, size_t idx) {
  return (*PTR_AT(msg, idx / 8, char)) &= ~(1 << (idx % 8));
}

UPB_INLINE bool _upb_has_oneof_field(const void *msg, size_t case_ofs, int32_t num) {
  return *PTR_AT(msg, case_ofs, int32_t) == num;
}

#undef PTR_AT

#endif  /* UPB_GENERATED_UTIL_H_ */


/*
** upb_decode: parsing into a upb_msg using a upb_msglayout.
*/

#ifndef UPB_DECODE_H_
#define UPB_DECODE_H_


UPB_BEGIN_EXTERN_C

bool upb_decode(upb_strview buf, upb_msg *msg, const upb_msglayout *l);

UPB_END_EXTERN_C

#endif  /* UPB_DECODE_H_ */
/*
** upb_encode: parsing into a upb_msg using a upb_msglayout.
*/

#ifndef UPB_ENCODE_H_
#define UPB_ENCODE_H_


UPB_BEGIN_EXTERN_C

char *upb_encode(const void *msg, const upb_msglayout *l, upb_arena *arena,
                 size_t *size);

UPB_END_EXTERN_C

#endif  /* UPB_ENCODE_H_ */
UPB_BEGIN_EXTERN_C

struct google_protobuf_FileDescriptorSet;
struct google_protobuf_FileDescriptorProto;
struct google_protobuf_DescriptorProto;
struct google_protobuf_DescriptorProto_ExtensionRange;
struct google_protobuf_DescriptorProto_ReservedRange;
struct google_protobuf_ExtensionRangeOptions;
struct google_protobuf_FieldDescriptorProto;
struct google_protobuf_OneofDescriptorProto;
struct google_protobuf_EnumDescriptorProto;
struct google_protobuf_EnumDescriptorProto_EnumReservedRange;
struct google_protobuf_EnumValueDescriptorProto;
struct google_protobuf_ServiceDescriptorProto;
struct google_protobuf_MethodDescriptorProto;
struct google_protobuf_FileOptions;
struct google_protobuf_MessageOptions;
struct google_protobuf_FieldOptions;
struct google_protobuf_OneofOptions;
struct google_protobuf_EnumOptions;
struct google_protobuf_EnumValueOptions;
struct google_protobuf_ServiceOptions;
struct google_protobuf_MethodOptions;
struct google_protobuf_UninterpretedOption;
struct google_protobuf_UninterpretedOption_NamePart;
struct google_protobuf_SourceCodeInfo;
struct google_protobuf_SourceCodeInfo_Location;
struct google_protobuf_GeneratedCodeInfo;
struct google_protobuf_GeneratedCodeInfo_Annotation;
typedef struct google_protobuf_FileDescriptorSet google_protobuf_FileDescriptorSet;
typedef struct google_protobuf_FileDescriptorProto google_protobuf_FileDescriptorProto;
typedef struct google_protobuf_DescriptorProto google_protobuf_DescriptorProto;
typedef struct google_protobuf_DescriptorProto_ExtensionRange google_protobuf_DescriptorProto_ExtensionRange;
typedef struct google_protobuf_DescriptorProto_ReservedRange google_protobuf_DescriptorProto_ReservedRange;
typedef struct google_protobuf_ExtensionRangeOptions google_protobuf_ExtensionRangeOptions;
typedef struct google_protobuf_FieldDescriptorProto google_protobuf_FieldDescriptorProto;
typedef struct google_protobuf_OneofDescriptorProto google_protobuf_OneofDescriptorProto;
typedef struct google_protobuf_EnumDescriptorProto google_protobuf_EnumDescriptorProto;
typedef struct google_protobuf_EnumDescriptorProto_EnumReservedRange google_protobuf_EnumDescriptorProto_EnumReservedRange;
typedef struct google_protobuf_EnumValueDescriptorProto google_protobuf_EnumValueDescriptorProto;
typedef struct google_protobuf_ServiceDescriptorProto google_protobuf_ServiceDescriptorProto;
typedef struct google_protobuf_MethodDescriptorProto google_protobuf_MethodDescriptorProto;
typedef struct google_protobuf_FileOptions google_protobuf_FileOptions;
typedef struct google_protobuf_MessageOptions google_protobuf_MessageOptions;
typedef struct google_protobuf_FieldOptions google_protobuf_FieldOptions;
typedef struct google_protobuf_OneofOptions google_protobuf_OneofOptions;
typedef struct google_protobuf_EnumOptions google_protobuf_EnumOptions;
typedef struct google_protobuf_EnumValueOptions google_protobuf_EnumValueOptions;
typedef struct google_protobuf_ServiceOptions google_protobuf_ServiceOptions;
typedef struct google_protobuf_MethodOptions google_protobuf_MethodOptions;
typedef struct google_protobuf_UninterpretedOption google_protobuf_UninterpretedOption;
typedef struct google_protobuf_UninterpretedOption_NamePart google_protobuf_UninterpretedOption_NamePart;
typedef struct google_protobuf_SourceCodeInfo google_protobuf_SourceCodeInfo;
typedef struct google_protobuf_SourceCodeInfo_Location google_protobuf_SourceCodeInfo_Location;
typedef struct google_protobuf_GeneratedCodeInfo google_protobuf_GeneratedCodeInfo;
typedef struct google_protobuf_GeneratedCodeInfo_Annotation google_protobuf_GeneratedCodeInfo_Annotation;
extern const upb_msglayout google_protobuf_FileDescriptorSet_msginit;
extern const upb_msglayout google_protobuf_FileDescriptorProto_msginit;
extern const upb_msglayout google_protobuf_DescriptorProto_msginit;
extern const upb_msglayout google_protobuf_DescriptorProto_ExtensionRange_msginit;
extern const upb_msglayout google_protobuf_DescriptorProto_ReservedRange_msginit;
extern const upb_msglayout google_protobuf_ExtensionRangeOptions_msginit;
extern const upb_msglayout google_protobuf_FieldDescriptorProto_msginit;
extern const upb_msglayout google_protobuf_OneofDescriptorProto_msginit;
extern const upb_msglayout google_protobuf_EnumDescriptorProto_msginit;
extern const upb_msglayout google_protobuf_EnumDescriptorProto_EnumReservedRange_msginit;
extern const upb_msglayout google_protobuf_EnumValueDescriptorProto_msginit;
extern const upb_msglayout google_protobuf_ServiceDescriptorProto_msginit;
extern const upb_msglayout google_protobuf_MethodDescriptorProto_msginit;
extern const upb_msglayout google_protobuf_FileOptions_msginit;
extern const upb_msglayout google_protobuf_MessageOptions_msginit;
extern const upb_msglayout google_protobuf_FieldOptions_msginit;
extern const upb_msglayout google_protobuf_OneofOptions_msginit;
extern const upb_msglayout google_protobuf_EnumOptions_msginit;
extern const upb_msglayout google_protobuf_EnumValueOptions_msginit;
extern const upb_msglayout google_protobuf_ServiceOptions_msginit;
extern const upb_msglayout google_protobuf_MethodOptions_msginit;
extern const upb_msglayout google_protobuf_UninterpretedOption_msginit;
extern const upb_msglayout google_protobuf_UninterpretedOption_NamePart_msginit;
extern const upb_msglayout google_protobuf_SourceCodeInfo_msginit;
extern const upb_msglayout google_protobuf_SourceCodeInfo_Location_msginit;
extern const upb_msglayout google_protobuf_GeneratedCodeInfo_msginit;
extern const upb_msglayout google_protobuf_GeneratedCodeInfo_Annotation_msginit;

/* Enums */

typedef enum {
  google_protobuf_FieldDescriptorProto_LABEL_OPTIONAL = 1,
  google_protobuf_FieldDescriptorProto_LABEL_REQUIRED = 2,
  google_protobuf_FieldDescriptorProto_LABEL_REPEATED = 3
} google_protobuf_FieldDescriptorProto_Label;

typedef enum {
  google_protobuf_FieldDescriptorProto_TYPE_DOUBLE = 1,
  google_protobuf_FieldDescriptorProto_TYPE_FLOAT = 2,
  google_protobuf_FieldDescriptorProto_TYPE_INT64 = 3,
  google_protobuf_FieldDescriptorProto_TYPE_UINT64 = 4,
  google_protobuf_FieldDescriptorProto_TYPE_INT32 = 5,
  google_protobuf_FieldDescriptorProto_TYPE_FIXED64 = 6,
  google_protobuf_FieldDescriptorProto_TYPE_FIXED32 = 7,
  google_protobuf_FieldDescriptorProto_TYPE_BOOL = 8,
  google_protobuf_FieldDescriptorProto_TYPE_STRING = 9,
  google_protobuf_FieldDescriptorProto_TYPE_GROUP = 10,
  google_protobuf_FieldDescriptorProto_TYPE_MESSAGE = 11,
  google_protobuf_FieldDescriptorProto_TYPE_BYTES = 12,
  google_protobuf_FieldDescriptorProto_TYPE_UINT32 = 13,
  google_protobuf_FieldDescriptorProto_TYPE_ENUM = 14,
  google_protobuf_FieldDescriptorProto_TYPE_SFIXED32 = 15,
  google_protobuf_FieldDescriptorProto_TYPE_SFIXED64 = 16,
  google_protobuf_FieldDescriptorProto_TYPE_SINT32 = 17,
  google_protobuf_FieldDescriptorProto_TYPE_SINT64 = 18
} google_protobuf_FieldDescriptorProto_Type;

typedef enum {
  google_protobuf_FieldOptions_STRING = 0,
  google_protobuf_FieldOptions_CORD = 1,
  google_protobuf_FieldOptions_STRING_PIECE = 2
} google_protobuf_FieldOptions_CType;

typedef enum {
  google_protobuf_FieldOptions_JS_NORMAL = 0,
  google_protobuf_FieldOptions_JS_STRING = 1,
  google_protobuf_FieldOptions_JS_NUMBER = 2
} google_protobuf_FieldOptions_JSType;

typedef enum {
  google_protobuf_FileOptions_SPEED = 1,
  google_protobuf_FileOptions_CODE_SIZE = 2,
  google_protobuf_FileOptions_LITE_RUNTIME = 3
} google_protobuf_FileOptions_OptimizeMode;

typedef enum {
  google_protobuf_MethodOptions_IDEMPOTENCY_UNKNOWN = 0,
  google_protobuf_MethodOptions_NO_SIDE_EFFECTS = 1,
  google_protobuf_MethodOptions_IDEMPOTENT = 2
} google_protobuf_MethodOptions_IdempotencyLevel;

/* google.protobuf.FileDescriptorSet */

UPB_INLINE google_protobuf_FileDescriptorSet *google_protobuf_FileDescriptorSet_new(upb_arena *arena) {
  return (google_protobuf_FileDescriptorSet *)upb_msg_new(&google_protobuf_FileDescriptorSet_msginit, arena);
}
UPB_INLINE google_protobuf_FileDescriptorSet *google_protobuf_FileDescriptorSet_parsenew(upb_strview buf, upb_arena *arena) {
  google_protobuf_FileDescriptorSet *ret = google_protobuf_FileDescriptorSet_new(arena);
  return (ret && upb_decode(buf, ret, &google_protobuf_FileDescriptorSet_msginit)) ? ret : NULL;
}
UPB_INLINE char *google_protobuf_FileDescriptorSet_serialize(const google_protobuf_FileDescriptorSet *msg, upb_arena *arena, size_t *len) {
  return upb_encode(msg, &google_protobuf_FileDescriptorSet_msginit, arena, len);
}

UPB_INLINE const google_protobuf_FileDescriptorProto* const* google_protobuf_FileDescriptorSet_file(const google_protobuf_FileDescriptorSet *msg, size_t *len) { return (const google_protobuf_FileDescriptorProto* const*)_upb_array_accessor(msg, UPB_SIZE(0, 0), len); }

UPB_INLINE google_protobuf_FileDescriptorProto** google_protobuf_FileDescriptorSet_mutable_file(google_protobuf_FileDescriptorSet *msg, size_t *len) {
  return (google_protobuf_FileDescriptorProto**)_upb_array_mutable_accessor(msg, UPB_SIZE(0, 0), len);
}
UPB_INLINE google_protobuf_FileDescriptorProto** google_protobuf_FileDescriptorSet_resize_file(google_protobuf_FileDescriptorSet *msg, size_t len, upb_arena *arena) {
  return (google_protobuf_FileDescriptorProto**)_upb_array_resize_accessor(msg, UPB_SIZE(0, 0), len, UPB_SIZE(4, 8), UPB_TYPE_MESSAGE, arena);
}
UPB_INLINE struct google_protobuf_FileDescriptorProto* google_protobuf_FileDescriptorSet_add_file(google_protobuf_FileDescriptorSet *msg, upb_arena *arena) {
  struct google_protobuf_FileDescriptorProto* sub = (struct google_protobuf_FileDescriptorProto*)upb_msg_new(&google_protobuf_FileDescriptorProto_msginit, arena);
  bool ok = _upb_array_append_accessor(
      msg, UPB_SIZE(0, 0), UPB_SIZE(4, 8), UPB_TYPE_MESSAGE, &sub, arena);
  if (!ok) return NULL;
  return sub;
}


/* google.protobuf.FileDescriptorProto */

UPB_INLINE google_protobuf_FileDescriptorProto *google_protobuf_FileDescriptorProto_new(upb_arena *arena) {
  return (google_protobuf_FileDescriptorProto *)upb_msg_new(&google_protobuf_FileDescriptorProto_msginit, arena);
}
UPB_INLINE google_protobuf_FileDescriptorProto *google_protobuf_FileDescriptorProto_parsenew(upb_strview buf, upb_arena *arena) {
  google_protobuf_FileDescriptorProto *ret = google_protobuf_FileDescriptorProto_new(arena);
  return (ret && upb_decode(buf, ret, &google_protobuf_FileDescriptorProto_msginit)) ? ret : NULL;
}
UPB_INLINE char *google_protobuf_FileDescriptorProto_serialize(const google_protobuf_FileDescriptorProto *msg, upb_arena *arena, size_t *len) {
  return upb_encode(msg, &google_protobuf_FileDescriptorProto_msginit, arena, len);
}

UPB_INLINE bool google_protobuf_FileDescriptorProto_has_name(const google_protobuf_FileDescriptorProto *msg) { return _upb_has_field(msg, 0); }
UPB_INLINE upb_strview google_protobuf_FileDescriptorProto_name(const google_protobuf_FileDescriptorProto *msg) { return UPB_FIELD_AT(msg, upb_strview, UPB_SIZE(4, 8)); }
UPB_INLINE bool google_protobuf_FileDescriptorProto_has_package(const google_protobuf_FileDescriptorProto *msg) { return _upb_has_field(msg, 1); }
UPB_INLINE upb_strview google_protobuf_FileDescriptorProto_package(const google_protobuf_FileDescriptorProto *msg) { return UPB_FIELD_AT(msg, upb_strview, UPB_SIZE(12, 24)); }
UPB_INLINE upb_strview const* google_protobuf_FileDescriptorProto_dependency(const google_protobuf_FileDescriptorProto *msg, size_t *len) { return (upb_strview const*)_upb_array_accessor(msg, UPB_SIZE(36, 72), len); }
UPB_INLINE const google_protobuf_DescriptorProto* const* google_protobuf_FileDescriptorProto_message_type(const google_protobuf_FileDescriptorProto *msg, size_t *len) { return (const google_protobuf_DescriptorProto* const*)_upb_array_accessor(msg, UPB_SIZE(40, 80), len); }
UPB_INLINE const google_protobuf_EnumDescriptorProto* const* google_protobuf_FileDescriptorProto_enum_type(const google_protobuf_FileDescriptorProto *msg, size_t *len) { return (const google_protobuf_EnumDescriptorProto* const*)_upb_array_accessor(msg, UPB_SIZE(44, 88), len); }
UPB_INLINE const google_protobuf_ServiceDescriptorProto* const* google_protobuf_FileDescriptorProto_service(const google_protobuf_FileDescriptorProto *msg, size_t *len) { return (const google_protobuf_ServiceDescriptorProto* const*)_upb_array_accessor(msg, UPB_SIZE(48, 96), len); }
UPB_INLINE const google_protobuf_FieldDescriptorProto* const* google_protobuf_FileDescriptorProto_extension(const google_protobuf_FileDescriptorProto *msg, size_t *len) { return (const google_protobuf_FieldDescriptorProto* const*)_upb_array_accessor(msg, UPB_SIZE(52, 104), len); }
UPB_INLINE bool google_protobuf_FileDescriptorProto_has_options(const google_protobuf_FileDescriptorProto *msg) { return _upb_has_field(msg, 3); }
UPB_INLINE const google_protobuf_FileOptions* google_protobuf_FileDescriptorProto_options(const google_protobuf_FileDescriptorProto *msg) { return UPB_FIELD_AT(msg, const google_protobuf_FileOptions*, UPB_SIZE(28, 56)); }
UPB_INLINE bool google_protobuf_FileDescriptorProto_has_source_code_info(const google_protobuf_FileDescriptorProto *msg) { return _upb_has_field(msg, 4); }
UPB_INLINE const google_protobuf_SourceCodeInfo* google_protobuf_FileDescriptorProto_source_code_info(const google_protobuf_FileDescriptorProto *msg) { return UPB_FIELD_AT(msg, const google_protobuf_SourceCodeInfo*, UPB_SIZE(32, 64)); }
UPB_INLINE int32_t const* google_protobuf_FileDescriptorProto_public_dependency(const google_protobuf_FileDescriptorProto *msg, size_t *len) { return (int32_t const*)_upb_array_accessor(msg, UPB_SIZE(56, 112), len); }
UPB_INLINE int32_t const* google_protobuf_FileDescriptorProto_weak_dependency(const google_protobuf_FileDescriptorProto *msg, size_t *len) { return (int32_t const*)_upb_array_accessor(msg, UPB_SIZE(60, 120), len); }
UPB_INLINE bool google_protobuf_FileDescriptorProto_has_syntax(const google_protobuf_FileDescriptorProto *msg) { return _upb_has_field(msg, 2); }
UPB_INLINE upb_strview google_protobuf_FileDescriptorProto_syntax(const google_protobuf_FileDescriptorProto *msg) { return UPB_FIELD_AT(msg, upb_strview, UPB_SIZE(20, 40)); }

UPB_INLINE void google_protobuf_FileDescriptorProto_set_name(google_protobuf_FileDescriptorProto *msg, upb_strview value) {
  _upb_sethas(msg, 0);
  UPB_FIELD_AT(msg, upb_strview, UPB_SIZE(4, 8)) = value;
}
UPB_INLINE void google_protobuf_FileDescriptorProto_set_package(google_protobuf_FileDescriptorProto *msg, upb_strview value) {
  _upb_sethas(msg, 1);
  UPB_FIELD_AT(msg, upb_strview, UPB_SIZE(12, 24)) = value;
}
UPB_INLINE upb_strview* google_protobuf_FileDescriptorProto_mutable_dependency(google_protobuf_FileDescriptorProto *msg, size_t *len) {
  return (upb_strview*)_upb_array_mutable_accessor(msg, UPB_SIZE(36, 72), len);
}
UPB_INLINE upb_strview* google_protobuf_FileDescriptorProto_resize_dependency(google_protobuf_FileDescriptorProto *msg, size_t len, upb_arena *arena) {
  return (upb_strview*)_upb_array_resize_accessor(msg, UPB_SIZE(36, 72), len, UPB_SIZE(8, 16), UPB_TYPE_STRING, arena);
}
UPB_INLINE bool google_protobuf_FileDescriptorProto_add_dependency(google_protobuf_FileDescriptorProto *msg, upb_strview val, upb_arena *arena) {
  return _upb_array_append_accessor(
      msg, UPB_SIZE(36, 72), UPB_SIZE(8, 16), UPB_TYPE_STRING, &val, arena);
}
UPB_INLINE google_protobuf_DescriptorProto** google_protobuf_FileDescriptorProto_mutable_message_type(google_protobuf_FileDescriptorProto *msg, size_t *len) {
  return (google_protobuf_DescriptorProto**)_upb_array_mutable_accessor(msg, UPB_SIZE(40, 80), len);
}
UPB_INLINE google_protobuf_DescriptorProto** google_protobuf_FileDescriptorProto_resize_message_type(google_protobuf_FileDescriptorProto *msg, size_t len, upb_arena *arena) {
  return (google_protobuf_DescriptorProto**)_upb_array_resize_accessor(msg, UPB_SIZE(40, 80), len, UPB_SIZE(4, 8), UPB_TYPE_MESSAGE, arena);
}
UPB_INLINE struct google_protobuf_DescriptorProto* google_protobuf_FileDescriptorProto_add_message_type(google_protobuf_FileDescriptorProto *msg, upb_arena *arena) {
  struct google_protobuf_DescriptorProto* sub = (struct google_protobuf_DescriptorProto*)upb_msg_new(&google_protobuf_DescriptorProto_msginit, arena);
  bool ok = _upb_array_append_accessor(
      msg, UPB_SIZE(40, 80), UPB_SIZE(4, 8), UPB_TYPE_MESSAGE, &sub, arena);
  if (!ok) return NULL;
  return sub;
}
UPB_INLINE google_protobuf_EnumDescriptorProto** google_protobuf_FileDescriptorProto_mutable_enum_type(google_protobuf_FileDescriptorProto *msg, size_t *len) {
  return (google_protobuf_EnumDescriptorProto**)_upb_array_mutable_accessor(msg, UPB_SIZE(44, 88), len);
}
UPB_INLINE google_protobuf_EnumDescriptorProto** google_protobuf_FileDescriptorProto_resize_enum_type(google_protobuf_FileDescriptorProto *msg, size_t len, upb_arena *arena) {
  return (google_protobuf_EnumDescriptorProto**)_upb_array_resize_accessor(msg, UPB_SIZE(44, 88), len, UPB_SIZE(4, 8), UPB_TYPE_MESSAGE, arena);
}
UPB_INLINE struct google_protobuf_EnumDescriptorProto* google_protobuf_FileDescriptorProto_add_enum_type(google_protobuf_FileDescriptorProto *msg, upb_arena *arena) {
  struct google_protobuf_EnumDescriptorProto* sub = (struct google_protobuf_EnumDescriptorProto*)upb_msg_new(&google_protobuf_EnumDescriptorProto_msginit, arena);
  bool ok = _upb_array_append_accessor(
      msg, UPB_SIZE(44, 88), UPB_SIZE(4, 8), UPB_TYPE_MESSAGE, &sub, arena);
  if (!ok) return NULL;
  return sub;
}
UPB_INLINE google_protobuf_ServiceDescriptorProto** google_protobuf_FileDescriptorProto_mutable_service(google_protobuf_FileDescriptorProto *msg, size_t *len) {
  return (google_protobuf_ServiceDescriptorProto**)_upb_array_mutable_accessor(msg, UPB_SIZE(48, 96), len);
}
UPB_INLINE google_protobuf_ServiceDescriptorProto** google_protobuf_FileDescriptorProto_resize_service(google_protobuf_FileDescriptorProto *msg, size_t len, upb_arena *arena) {
  return (google_protobuf_ServiceDescriptorProto**)_upb_array_resize_accessor(msg, UPB_SIZE(48, 96), len, UPB_SIZE(4, 8), UPB_TYPE_MESSAGE, arena);
}
UPB_INLINE struct google_protobuf_ServiceDescriptorProto* google_protobuf_FileDescriptorProto_add_service(google_protobuf_FileDescriptorProto *msg, upb_arena *arena) {
  struct google_protobuf_ServiceDescriptorProto* sub = (struct google_protobuf_ServiceDescriptorProto*)upb_msg_new(&google_protobuf_ServiceDescriptorProto_msginit, arena);
  bool ok = _upb_array_append_accessor(
      msg, UPB_SIZE(48, 96), UPB_SIZE(4, 8), UPB_TYPE_MESSAGE, &sub, arena);
  if (!ok) return NULL;
  return sub;
}
UPB_INLINE google_protobuf_FieldDescriptorProto** google_protobuf_FileDescriptorProto_mutable_extension(google_protobuf_FileDescriptorProto *msg, size_t *len) {
  return (google_protobuf_FieldDescriptorProto**)_upb_array_mutable_accessor(msg, UPB_SIZE(52, 104), len);
}
UPB_INLINE google_protobuf_FieldDescriptorProto** google_protobuf_FileDescriptorProto_resize_extension(google_protobuf_FileDescriptorProto *msg, size_t len, upb_arena *arena) {
  return (google_protobuf_FieldDescriptorProto**)_upb_array_resize_accessor(msg, UPB_SIZE(52, 104), len, UPB_SIZE(4, 8), UPB_TYPE_MESSAGE, arena);
}
UPB_INLINE struct google_protobuf_FieldDescriptorProto* google_protobuf_FileDescriptorProto_add_extension(google_protobuf_FileDescriptorProto *msg, upb_arena *arena) {
  struct google_protobuf_FieldDescriptorProto* sub = (struct google_protobuf_FieldDescriptorProto*)upb_msg_new(&google_protobuf_FieldDescriptorProto_msginit, arena);
  bool ok = _upb_array_append_accessor(
      msg, UPB_SIZE(52, 104), UPB_SIZE(4, 8), UPB_TYPE_MESSAGE, &sub, arena);
  if (!ok) return NULL;
  return sub;
}
UPB_INLINE void google_protobuf_FileDescriptorProto_set_options(google_protobuf_FileDescriptorProto *msg, google_protobuf_FileOptions* value) {
  _upb_sethas(msg, 3);
  UPB_FIELD_AT(msg, google_protobuf_FileOptions*, UPB_SIZE(28, 56)) = value;
}
UPB_INLINE struct google_protobuf_FileOptions* google_protobuf_FileDescriptorProto_mutable_options(google_protobuf_FileDescriptorProto *msg, upb_arena *arena) {
  struct google_protobuf_FileOptions* sub = (struct google_protobuf_FileOptions*)google_protobuf_FileDescriptorProto_options(msg);
  if (sub == NULL) {
    sub = (struct google_protobuf_FileOptions*)upb_msg_new(&google_protobuf_FileOptions_msginit, arena);
    if (!sub) return NULL;
    google_protobuf_FileDescriptorProto_set_options(msg, sub);
  }
  return sub;
}
UPB_INLINE void google_protobuf_FileDescriptorProto_set_source_code_info(google_protobuf_FileDescriptorProto *msg, google_protobuf_SourceCodeInfo* value) {
  _upb_sethas(msg, 4);
  UPB_FIELD_AT(msg, google_protobuf_SourceCodeInfo*, UPB_SIZE(32, 64)) = value;
}
UPB_INLINE struct google_protobuf_SourceCodeInfo* google_protobuf_FileDescriptorProto_mutable_source_code_info(google_protobuf_FileDescriptorProto *msg, upb_arena *arena) {
  struct google_protobuf_SourceCodeInfo* sub = (struct google_protobuf_SourceCodeInfo*)google_protobuf_FileDescriptorProto_source_code_info(msg);
  if (sub == NULL) {
    sub = (struct google_protobuf_SourceCodeInfo*)upb_msg_new(&google_protobuf_SourceCodeInfo_msginit, arena);
    if (!sub) return NULL;
    google_protobuf_FileDescriptorProto_set_source_code_info(msg, sub);
  }
  return sub;
}
UPB_INLINE int32_t* google_protobuf_FileDescriptorProto_mutable_public_dependency(google_protobuf_FileDescriptorProto *msg, size_t *len) {
  return (int32_t*)_upb_array_mutable_accessor(msg, UPB_SIZE(56, 112), len);
}
UPB_INLINE int32_t* google_protobuf_FileDescriptorProto_resize_public_dependency(google_protobuf_FileDescriptorProto *msg, size_t len, upb_arena *arena) {
  return (int32_t*)_upb_array_resize_accessor(msg, UPB_SIZE(56, 112), len, UPB_SIZE(4, 4), UPB_TYPE_INT32, arena);
}
UPB_INLINE bool google_protobuf_FileDescriptorProto_add_public_dependency(google_protobuf_FileDescriptorProto *msg, int32_t val, upb_arena *arena) {
  return _upb_array_append_accessor(
      msg, UPB_SIZE(56, 112), UPB_SIZE(4, 4), UPB_TYPE_INT32, &val, arena);
}
UPB_INLINE int32_t* google_protobuf_FileDescriptorProto_mutable_weak_dependency(google_protobuf_FileDescriptorProto *msg, size_t *len) {
  return (int32_t*)_upb_array_mutable_accessor(msg, UPB_SIZE(60, 120), len);
}
UPB_INLINE int32_t* google_protobuf_FileDescriptorProto_resize_weak_dependency(google_protobuf_FileDescriptorProto *msg, size_t len, upb_arena *arena) {
  return (int32_t*)_upb_array_resize_accessor(msg, UPB_SIZE(60, 120), len, UPB_SIZE(4, 4), UPB_TYPE_INT32, arena);
}
UPB_INLINE bool google_protobuf_FileDescriptorProto_add_weak_dependency(google_protobuf_FileDescriptorProto *msg, int32_t val, upb_arena *arena) {
  return _upb_array_append_accessor(
      msg, UPB_SIZE(60, 120), UPB_SIZE(4, 4), UPB_TYPE_INT32, &val, arena);
}
UPB_INLINE void google_protobuf_FileDescriptorProto_set_syntax(google_protobuf_FileDescriptorProto *msg, upb_strview value) {
  _upb_sethas(msg, 2);
  UPB_FIELD_AT(msg, upb_strview, UPB_SIZE(20, 40)) = value;
}


/* google.protobuf.DescriptorProto */

UPB_INLINE google_protobuf_DescriptorProto *google_protobuf_DescriptorProto_new(upb_arena *arena) {
  return (google_protobuf_DescriptorProto *)upb_msg_new(&google_protobuf_DescriptorProto_msginit, arena);
}
UPB_INLINE google_protobuf_DescriptorProto *google_protobuf_DescriptorProto_parsenew(upb_strview buf, upb_arena *arena) {
  google_protobuf_DescriptorProto *ret = google_protobuf_DescriptorProto_new(arena);
  return (ret && upb_decode(buf, ret, &google_protobuf_DescriptorProto_msginit)) ? ret : NULL;
}
UPB_INLINE char *google_protobuf_DescriptorProto_serialize(const google_protobuf_DescriptorProto *msg, upb_arena *arena, size_t *len) {
  return upb_encode(msg, &google_protobuf_DescriptorProto_msginit, arena, len);
}

UPB_INLINE bool google_protobuf_DescriptorProto_has_name(const google_protobuf_DescriptorProto *msg) { return _upb_has_field(msg, 0); }
UPB_INLINE upb_strview google_protobuf_DescriptorProto_name(const google_protobuf_DescriptorProto *msg) { return UPB_FIELD_AT(msg, upb_strview, UPB_SIZE(4, 8)); }
UPB_INLINE const google_protobuf_FieldDescriptorProto* const* google_protobuf_DescriptorProto_field(const google_protobuf_DescriptorProto *msg, size_t *len) { return (const google_protobuf_FieldDescriptorProto* const*)_upb_array_accessor(msg, UPB_SIZE(16, 32), len); }
UPB_INLINE const google_protobuf_DescriptorProto* const* google_protobuf_DescriptorProto_nested_type(const google_protobuf_DescriptorProto *msg, size_t *len) { return (const google_protobuf_DescriptorProto* const*)_upb_array_accessor(msg, UPB_SIZE(20, 40), len); }
UPB_INLINE const google_protobuf_EnumDescriptorProto* const* google_protobuf_DescriptorProto_enum_type(const google_protobuf_DescriptorProto *msg, size_t *len) { return (const google_protobuf_EnumDescriptorProto* const*)_upb_array_accessor(msg, UPB_SIZE(24, 48), len); }
UPB_INLINE const google_protobuf_DescriptorProto_ExtensionRange* const* google_protobuf_DescriptorProto_extension_range(const google_protobuf_DescriptorProto *msg, size_t *len) { return (const google_protobuf_DescriptorProto_ExtensionRange* const*)_upb_array_accessor(msg, UPB_SIZE(28, 56), len); }
UPB_INLINE const google_protobuf_FieldDescriptorProto* const* google_protobuf_DescriptorProto_extension(const google_protobuf_DescriptorProto *msg, size_t *len) { return (const google_protobuf_FieldDescriptorProto* const*)_upb_array_accessor(msg, UPB_SIZE(32, 64), len); }
UPB_INLINE bool google_protobuf_DescriptorProto_has_options(const google_protobuf_DescriptorProto *msg) { return _upb_has_field(msg, 1); }
UPB_INLINE const google_protobuf_MessageOptions* google_protobuf_DescriptorProto_options(const google_protobuf_DescriptorProto *msg) { return UPB_FIELD_AT(msg, const google_protobuf_MessageOptions*, UPB_SIZE(12, 24)); }
UPB_INLINE const google_protobuf_OneofDescriptorProto* const* google_protobuf_DescriptorProto_oneof_decl(const google_protobuf_DescriptorProto *msg, size_t *len) { return (const google_protobuf_OneofDescriptorProto* const*)_upb_array_accessor(msg, UPB_SIZE(36, 72), len); }
UPB_INLINE const google_protobuf_DescriptorProto_ReservedRange* const* google_protobuf_DescriptorProto_reserved_range(const google_protobuf_DescriptorProto *msg, size_t *len) { return (const google_protobuf_DescriptorProto_ReservedRange* const*)_upb_array_accessor(msg, UPB_SIZE(40, 80), len); }
UPB_INLINE upb_strview const* google_protobuf_DescriptorProto_reserved_name(const google_protobuf_DescriptorProto *msg, size_t *len) { return (upb_strview const*)_upb_array_accessor(msg, UPB_SIZE(44, 88), len); }

UPB_INLINE void google_protobuf_DescriptorProto_set_name(google_protobuf_DescriptorProto *msg, upb_strview value) {
  _upb_sethas(msg, 0);
  UPB_FIELD_AT(msg, upb_strview, UPB_SIZE(4, 8)) = value;
}
UPB_INLINE google_protobuf_FieldDescriptorProto** google_protobuf_DescriptorProto_mutable_field(google_protobuf_DescriptorProto *msg, size_t *len) {
  return (google_protobuf_FieldDescriptorProto**)_upb_array_mutable_accessor(msg, UPB_SIZE(16, 32), len);
}
UPB_INLINE google_protobuf_FieldDescriptorProto** google_protobuf_DescriptorProto_resize_field(google_protobuf_DescriptorProto *msg, size_t len, upb_arena *arena) {
  return (google_protobuf_FieldDescriptorProto**)_upb_array_resize_accessor(msg, UPB_SIZE(16, 32), len, UPB_SIZE(4, 8), UPB_TYPE_MESSAGE, arena);
}
UPB_INLINE struct google_protobuf_FieldDescriptorProto* google_protobuf_DescriptorProto_add_field(google_protobuf_DescriptorProto *msg, upb_arena *arena) {
  struct google_protobuf_FieldDescriptorProto* sub = (struct google_protobuf_FieldDescriptorProto*)upb_msg_new(&google_protobuf_FieldDescriptorProto_msginit, arena);
  bool ok = _upb_array_append_accessor(
      msg, UPB_SIZE(16, 32), UPB_SIZE(4, 8), UPB_TYPE_MESSAGE, &sub, arena);
  if (!ok) return NULL;
  return sub;
}
UPB_INLINE google_protobuf_DescriptorProto** google_protobuf_DescriptorProto_mutable_nested_type(google_protobuf_DescriptorProto *msg, size_t *len) {
  return (google_protobuf_DescriptorProto**)_upb_array_mutable_accessor(msg, UPB_SIZE(20, 40), len);
}
UPB_INLINE google_protobuf_DescriptorProto** google_protobuf_DescriptorProto_resize_nested_type(google_protobuf_DescriptorProto *msg, size_t len, upb_arena *arena) {
  return (google_protobuf_DescriptorProto**)_upb_array_resize_accessor(msg, UPB_SIZE(20, 40), len, UPB_SIZE(4, 8), UPB_TYPE_MESSAGE, arena);
}
UPB_INLINE struct google_protobuf_DescriptorProto* google_protobuf_DescriptorProto_add_nested_type(google_protobuf_DescriptorProto *msg, upb_arena *arena) {
  struct google_protobuf_DescriptorProto* sub = (struct google_protobuf_DescriptorProto*)upb_msg_new(&google_protobuf_DescriptorProto_msginit, arena);
  bool ok = _upb_array_append_accessor(
      msg, UPB_SIZE(20, 40), UPB_SIZE(4, 8), UPB_TYPE_MESSAGE, &sub, arena);
  if (!ok) return NULL;
  return sub;
}
UPB_INLINE google_protobuf_EnumDescriptorProto** google_protobuf_DescriptorProto_mutable_enum_type(google_protobuf_DescriptorProto *msg, size_t *len) {
  return (google_protobuf_EnumDescriptorProto**)_upb_array_mutable_accessor(msg, UPB_SIZE(24, 48), len);
}
UPB_INLINE google_protobuf_EnumDescriptorProto** google_protobuf_DescriptorProto_resize_enum_type(google_protobuf_DescriptorProto *msg, size_t len, upb_arena *arena) {
  return (google_protobuf_EnumDescriptorProto**)_upb_array_resize_accessor(msg, UPB_SIZE(24, 48), len, UPB_SIZE(4, 8), UPB_TYPE_MESSAGE, arena);
}
UPB_INLINE struct google_protobuf_EnumDescriptorProto* google_protobuf_DescriptorProto_add_enum_type(google_protobuf_DescriptorProto *msg, upb_arena *arena) {
  struct google_protobuf_EnumDescriptorProto* sub = (struct google_protobuf_EnumDescriptorProto*)upb_msg_new(&google_protobuf_EnumDescriptorProto_msginit, arena);
  bool ok = _upb_array_append_accessor(
      msg, UPB_SIZE(24, 48), UPB_SIZE(4, 8), UPB_TYPE_MESSAGE, &sub, arena);
  if (!ok) return NULL;
  return sub;
}
UPB_INLINE google_protobuf_DescriptorProto_ExtensionRange** google_protobuf_DescriptorProto_mutable_extension_range(google_protobuf_DescriptorProto *msg, size_t *len) {
  return (google_protobuf_DescriptorProto_ExtensionRange**)_upb_array_mutable_accessor(msg, UPB_SIZE(28, 56), len);
}
UPB_INLINE google_protobuf_DescriptorProto_ExtensionRange** google_protobuf_DescriptorProto_resize_extension_range(google_protobuf_DescriptorProto *msg, size_t len, upb_arena *arena) {
  return (google_protobuf_DescriptorProto_ExtensionRange**)_upb_array_resize_accessor(msg, UPB_SIZE(28, 56), len, UPB_SIZE(4, 8), UPB_TYPE_MESSAGE, arena);
}
UPB_INLINE struct google_protobuf_DescriptorProto_ExtensionRange* google_protobuf_DescriptorProto_add_extension_range(google_protobuf_DescriptorProto *msg, upb_arena *arena) {
  struct google_protobuf_DescriptorProto_ExtensionRange* sub = (struct google_protobuf_DescriptorProto_ExtensionRange*)upb_msg_new(&google_protobuf_DescriptorProto_ExtensionRange_msginit, arena);
  bool ok = _upb_array_append_accessor(
      msg, UPB_SIZE(28, 56), UPB_SIZE(4, 8), UPB_TYPE_MESSAGE, &sub, arena);
  if (!ok) return NULL;
  return sub;
}
UPB_INLINE google_protobuf_FieldDescriptorProto** google_protobuf_DescriptorProto_mutable_extension(google_protobuf_DescriptorProto *msg, size_t *len) {
  return (google_protobuf_FieldDescriptorProto**)_upb_array_mutable_accessor(msg, UPB_SIZE(32, 64), len);
}
UPB_INLINE google_protobuf_FieldDescriptorProto** google_protobuf_DescriptorProto_resize_extension(google_protobuf_DescriptorProto *msg, size_t len, upb_arena *arena) {
  return (google_protobuf_FieldDescriptorProto**)_upb_array_resize_accessor(msg, UPB_SIZE(32, 64), len, UPB_SIZE(4, 8), UPB_TYPE_MESSAGE, arena);
}
UPB_INLINE struct google_protobuf_FieldDescriptorProto* google_protobuf_DescriptorProto_add_extension(google_protobuf_DescriptorProto *msg, upb_arena *arena) {
  struct google_protobuf_FieldDescriptorProto* sub = (struct google_protobuf_FieldDescriptorProto*)upb_msg_new(&google_protobuf_FieldDescriptorProto_msginit, arena);
  bool ok = _upb_array_append_accessor(
      msg, UPB_SIZE(32, 64), UPB_SIZE(4, 8), UPB_TYPE_MESSAGE, &sub, arena);
  if (!ok) return NULL;
  return sub;
}
UPB_INLINE void google_protobuf_DescriptorProto_set_options(google_protobuf_DescriptorProto *msg, google_protobuf_MessageOptions* value) {
  _upb_sethas(msg, 1);
  UPB_FIELD_AT(msg, google_protobuf_MessageOptions*, UPB_SIZE(12, 24)) = value;
}
UPB_INLINE struct google_protobuf_MessageOptions* google_protobuf_DescriptorProto_mutable_options(google_protobuf_DescriptorProto *msg, upb_arena *arena) {
  struct google_protobuf_MessageOptions* sub = (struct google_protobuf_MessageOptions*)google_protobuf_DescriptorProto_options(msg);
  if (sub == NULL) {
    sub = (struct google_protobuf_MessageOptions*)upb_msg_new(&google_protobuf_MessageOptions_msginit, arena);
    if (!sub) return NULL;
    google_protobuf_DescriptorProto_set_options(msg, sub);
  }
  return sub;
}
UPB_INLINE google_protobuf_OneofDescriptorProto** google_protobuf_DescriptorProto_mutable_oneof_decl(google_protobuf_DescriptorProto *msg, size_t *len) {
  return (google_protobuf_OneofDescriptorProto**)_upb_array_mutable_accessor(msg, UPB_SIZE(36, 72), len);
}
UPB_INLINE google_protobuf_OneofDescriptorProto** google_protobuf_DescriptorProto_resize_oneof_decl(google_protobuf_DescriptorProto *msg, size_t len, upb_arena *arena) {
  return (google_protobuf_OneofDescriptorProto**)_upb_array_resize_accessor(msg, UPB_SIZE(36, 72), len, UPB_SIZE(4, 8), UPB_TYPE_MESSAGE, arena);
}
UPB_INLINE struct google_protobuf_OneofDescriptorProto* google_protobuf_DescriptorProto_add_oneof_decl(google_protobuf_DescriptorProto *msg, upb_arena *arena) {
  struct google_protobuf_OneofDescriptorProto* sub = (struct google_protobuf_OneofDescriptorProto*)upb_msg_new(&google_protobuf_OneofDescriptorProto_msginit, arena);
  bool ok = _upb_array_append_accessor(
      msg, UPB_SIZE(36, 72), UPB_SIZE(4, 8), UPB_TYPE_MESSAGE, &sub, arena);
  if (!ok) return NULL;
  return sub;
}
UPB_INLINE google_protobuf_DescriptorProto_ReservedRange** google_protobuf_DescriptorProto_mutable_reserved_range(google_protobuf_DescriptorProto *msg, size_t *len) {
  return (google_protobuf_DescriptorProto_ReservedRange**)_upb_array_mutable_accessor(msg, UPB_SIZE(40, 80), len);
}
UPB_INLINE google_protobuf_DescriptorProto_ReservedRange** google_protobuf_DescriptorProto_resize_reserved_range(google_protobuf_DescriptorProto *msg, size_t len, upb_arena *arena) {
  return (google_protobuf_DescriptorProto_ReservedRange**)_upb_array_resize_accessor(msg, UPB_SIZE(40, 80), len, UPB_SIZE(4, 8), UPB_TYPE_MESSAGE, arena);
}
UPB_INLINE struct google_protobuf_DescriptorProto_ReservedRange* google_protobuf_DescriptorProto_add_reserved_range(google_protobuf_DescriptorProto *msg, upb_arena *arena) {
  struct google_protobuf_DescriptorProto_ReservedRange* sub = (struct google_protobuf_DescriptorProto_ReservedRange*)upb_msg_new(&google_protobuf_DescriptorProto_ReservedRange_msginit, arena);
  bool ok = _upb_array_append_accessor(
      msg, UPB_SIZE(40, 80), UPB_SIZE(4, 8), UPB_TYPE_MESSAGE, &sub, arena);
  if (!ok) return NULL;
  return sub;
}
UPB_INLINE upb_strview* google_protobuf_DescriptorProto_mutable_reserved_name(google_protobuf_DescriptorProto *msg, size_t *len) {
  return (upb_strview*)_upb_array_mutable_accessor(msg, UPB_SIZE(44, 88), len);
}
UPB_INLINE upb_strview* google_protobuf_DescriptorProto_resize_reserved_name(google_protobuf_DescriptorProto *msg, size_t len, upb_arena *arena) {
  return (upb_strview*)_upb_array_resize_accessor(msg, UPB_SIZE(44, 88), len, UPB_SIZE(8, 16), UPB_TYPE_STRING, arena);
}
UPB_INLINE bool google_protobuf_DescriptorProto_add_reserved_name(google_protobuf_DescriptorProto *msg, upb_strview val, upb_arena *arena) {
  return _upb_array_append_accessor(
      msg, UPB_SIZE(44, 88), UPB_SIZE(8, 16), UPB_TYPE_STRING, &val, arena);
}


/* google.protobuf.DescriptorProto.ExtensionRange */

UPB_INLINE google_protobuf_DescriptorProto_ExtensionRange *google_protobuf_DescriptorProto_ExtensionRange_new(upb_arena *arena) {
  return (google_protobuf_DescriptorProto_ExtensionRange *)upb_msg_new(&google_protobuf_DescriptorProto_ExtensionRange_msginit, arena);
}
UPB_INLINE google_protobuf_DescriptorProto_ExtensionRange *google_protobuf_DescriptorProto_ExtensionRange_parsenew(upb_strview buf, upb_arena *arena) {
  google_protobuf_DescriptorProto_ExtensionRange *ret = google_protobuf_DescriptorProto_ExtensionRange_new(arena);
  return (ret && upb_decode(buf, ret, &google_protobuf_DescriptorProto_ExtensionRange_msginit)) ? ret : NULL;
}
UPB_INLINE char *google_protobuf_DescriptorProto_ExtensionRange_serialize(const google_protobuf_DescriptorProto_ExtensionRange *msg, upb_arena *arena, size_t *len) {
  return upb_encode(msg, &google_protobuf_DescriptorProto_ExtensionRange_msginit, arena, len);
}

UPB_INLINE bool google_protobuf_DescriptorProto_ExtensionRange_has_start(const google_protobuf_DescriptorProto_ExtensionRange *msg) { return _upb_has_field(msg, 0); }
UPB_INLINE int32_t google_protobuf_DescriptorProto_ExtensionRange_start(const google_protobuf_DescriptorProto_ExtensionRange *msg) { return UPB_FIELD_AT(msg, int32_t, UPB_SIZE(4, 4)); }
UPB_INLINE bool google_protobuf_DescriptorProto_ExtensionRange_has_end(const google_protobuf_DescriptorProto_ExtensionRange *msg) { return _upb_has_field(msg, 1); }
UPB_INLINE int32_t google_protobuf_DescriptorProto_ExtensionRange_end(const google_protobuf_DescriptorProto_ExtensionRange *msg) { return UPB_FIELD_AT(msg, int32_t, UPB_SIZE(8, 8)); }
UPB_INLINE bool google_protobuf_DescriptorProto_ExtensionRange_has_options(const google_protobuf_DescriptorProto_ExtensionRange *msg) { return _upb_has_field(msg, 2); }
UPB_INLINE const google_protobuf_ExtensionRangeOptions* google_protobuf_DescriptorProto_ExtensionRange_options(const google_protobuf_DescriptorProto_ExtensionRange *msg) { return UPB_FIELD_AT(msg, const google_protobuf_ExtensionRangeOptions*, UPB_SIZE(12, 16)); }

UPB_INLINE void google_protobuf_DescriptorProto_ExtensionRange_set_start(google_protobuf_DescriptorProto_ExtensionRange *msg, int32_t value) {
  _upb_sethas(msg, 0);
  UPB_FIELD_AT(msg, int32_t, UPB_SIZE(4, 4)) = value;
}
UPB_INLINE void google_protobuf_DescriptorProto_ExtensionRange_set_end(google_protobuf_DescriptorProto_ExtensionRange *msg, int32_t value) {
  _upb_sethas(msg, 1);
  UPB_FIELD_AT(msg, int32_t, UPB_SIZE(8, 8)) = value;
}
UPB_INLINE void google_protobuf_DescriptorProto_ExtensionRange_set_options(google_protobuf_DescriptorProto_ExtensionRange *msg, google_protobuf_ExtensionRangeOptions* value) {
  _upb_sethas(msg, 2);
  UPB_FIELD_AT(msg, google_protobuf_ExtensionRangeOptions*, UPB_SIZE(12, 16)) = value;
}
UPB_INLINE struct google_protobuf_ExtensionRangeOptions* google_protobuf_DescriptorProto_ExtensionRange_mutable_options(google_protobuf_DescriptorProto_ExtensionRange *msg, upb_arena *arena) {
  struct google_protobuf_ExtensionRangeOptions* sub = (struct google_protobuf_ExtensionRangeOptions*)google_protobuf_DescriptorProto_ExtensionRange_options(msg);
  if (sub == NULL) {
    sub = (struct google_protobuf_ExtensionRangeOptions*)upb_msg_new(&google_protobuf_ExtensionRangeOptions_msginit, arena);
    if (!sub) return NULL;
    google_protobuf_DescriptorProto_ExtensionRange_set_options(msg, sub);
  }
  return sub;
}


/* google.protobuf.DescriptorProto.ReservedRange */

UPB_INLINE google_protobuf_DescriptorProto_ReservedRange *google_protobuf_DescriptorProto_ReservedRange_new(upb_arena *arena) {
  return (google_protobuf_DescriptorProto_ReservedRange *)upb_msg_new(&google_protobuf_DescriptorProto_ReservedRange_msginit, arena);
}
UPB_INLINE google_protobuf_DescriptorProto_ReservedRange *google_protobuf_DescriptorProto_ReservedRange_parsenew(upb_strview buf, upb_arena *arena) {
  google_protobuf_DescriptorProto_ReservedRange *ret = google_protobuf_DescriptorProto_ReservedRange_new(arena);
  return (ret && upb_decode(buf, ret, &google_protobuf_DescriptorProto_ReservedRange_msginit)) ? ret : NULL;
}
UPB_INLINE char *google_protobuf_DescriptorProto_ReservedRange_serialize(const google_protobuf_DescriptorProto_ReservedRange *msg, upb_arena *arena, size_t *len) {
  return upb_encode(msg, &google_protobuf_DescriptorProto_ReservedRange_msginit, arena, len);
}

UPB_INLINE bool google_protobuf_DescriptorProto_ReservedRange_has_start(const google_protobuf_DescriptorProto_ReservedRange *msg) { return _upb_has_field(msg, 0); }
UPB_INLINE int32_t google_protobuf_DescriptorProto_ReservedRange_start(const google_protobuf_DescriptorProto_ReservedRange *msg) { return UPB_FIELD_AT(msg, int32_t, UPB_SIZE(4, 4)); }
UPB_INLINE bool google_protobuf_DescriptorProto_ReservedRange_has_end(const google_protobuf_DescriptorProto_ReservedRange *msg) { return _upb_has_field(msg, 1); }
UPB_INLINE int32_t google_protobuf_DescriptorProto_ReservedRange_end(const google_protobuf_DescriptorProto_ReservedRange *msg) { return UPB_FIELD_AT(msg, int32_t, UPB_SIZE(8, 8)); }

UPB_INLINE void google_protobuf_DescriptorProto_ReservedRange_set_start(google_protobuf_DescriptorProto_ReservedRange *msg, int32_t value) {
  _upb_sethas(msg, 0);
  UPB_FIELD_AT(msg, int32_t, UPB_SIZE(4, 4)) = value;
}
UPB_INLINE void google_protobuf_DescriptorProto_ReservedRange_set_end(google_protobuf_DescriptorProto_ReservedRange *msg, int32_t value) {
  _upb_sethas(msg, 1);
  UPB_FIELD_AT(msg, int32_t, UPB_SIZE(8, 8)) = value;
}


/* google.protobuf.ExtensionRangeOptions */

UPB_INLINE google_protobuf_ExtensionRangeOptions *google_protobuf_ExtensionRangeOptions_new(upb_arena *arena) {
  return (google_protobuf_ExtensionRangeOptions *)upb_msg_new(&google_protobuf_ExtensionRangeOptions_msginit, arena);
}
UPB_INLINE google_protobuf_ExtensionRangeOptions *google_protobuf_ExtensionRangeOptions_parsenew(upb_strview buf, upb_arena *arena) {
  google_protobuf_ExtensionRangeOptions *ret = google_protobuf_ExtensionRangeOptions_new(arena);
  return (ret && upb_decode(buf, ret, &google_protobuf_ExtensionRangeOptions_msginit)) ? ret : NULL;
}
UPB_INLINE char *google_protobuf_ExtensionRangeOptions_serialize(const google_protobuf_ExtensionRangeOptions *msg, upb_arena *arena, size_t *len) {
  return upb_encode(msg, &google_protobuf_ExtensionRangeOptions_msginit, arena, len);
}

UPB_INLINE const google_protobuf_UninterpretedOption* const* google_protobuf_ExtensionRangeOptions_uninterpreted_option(const google_protobuf_ExtensionRangeOptions *msg, size_t *len) { return (const google_protobuf_UninterpretedOption* const*)_upb_array_accessor(msg, UPB_SIZE(0, 0), len); }

UPB_INLINE google_protobuf_UninterpretedOption** google_protobuf_ExtensionRangeOptions_mutable_uninterpreted_option(google_protobuf_ExtensionRangeOptions *msg, size_t *len) {
  return (google_protobuf_UninterpretedOption**)_upb_array_mutable_accessor(msg, UPB_SIZE(0, 0), len);
}
UPB_INLINE google_protobuf_UninterpretedOption** google_protobuf_ExtensionRangeOptions_resize_uninterpreted_option(google_protobuf_ExtensionRangeOptions *msg, size_t len, upb_arena *arena) {
  return (google_protobuf_UninterpretedOption**)_upb_array_resize_accessor(msg, UPB_SIZE(0, 0), len, UPB_SIZE(4, 8), UPB_TYPE_MESSAGE, arena);
}
UPB_INLINE struct google_protobuf_UninterpretedOption* google_protobuf_ExtensionRangeOptions_add_uninterpreted_option(google_protobuf_ExtensionRangeOptions *msg, upb_arena *arena) {
  struct google_protobuf_UninterpretedOption* sub = (struct google_protobuf_UninterpretedOption*)upb_msg_new(&google_protobuf_UninterpretedOption_msginit, arena);
  bool ok = _upb_array_append_accessor(
      msg, UPB_SIZE(0, 0), UPB_SIZE(4, 8), UPB_TYPE_MESSAGE, &sub, arena);
  if (!ok) return NULL;
  return sub;
}


/* google.protobuf.FieldDescriptorProto */

UPB_INLINE google_protobuf_FieldDescriptorProto *google_protobuf_FieldDescriptorProto_new(upb_arena *arena) {
  return (google_protobuf_FieldDescriptorProto *)upb_msg_new(&google_protobuf_FieldDescriptorProto_msginit, arena);
}
UPB_INLINE google_protobuf_FieldDescriptorProto *google_protobuf_FieldDescriptorProto_parsenew(upb_strview buf, upb_arena *arena) {
  google_protobuf_FieldDescriptorProto *ret = google_protobuf_FieldDescriptorProto_new(arena);
  return (ret && upb_decode(buf, ret, &google_protobuf_FieldDescriptorProto_msginit)) ? ret : NULL;
}
UPB_INLINE char *google_protobuf_FieldDescriptorProto_serialize(const google_protobuf_FieldDescriptorProto *msg, upb_arena *arena, size_t *len) {
  return upb_encode(msg, &google_protobuf_FieldDescriptorProto_msginit, arena, len);
}

UPB_INLINE bool google_protobuf_FieldDescriptorProto_has_name(const google_protobuf_FieldDescriptorProto *msg) { return _upb_has_field(msg, 4); }
UPB_INLINE upb_strview google_protobuf_FieldDescriptorProto_name(const google_protobuf_FieldDescriptorProto *msg) { return UPB_FIELD_AT(msg, upb_strview, UPB_SIZE(32, 32)); }
UPB_INLINE bool google_protobuf_FieldDescriptorProto_has_extendee(const google_protobuf_FieldDescriptorProto *msg) { return _upb_has_field(msg, 5); }
UPB_INLINE upb_strview google_protobuf_FieldDescriptorProto_extendee(const google_protobuf_FieldDescriptorProto *msg) { return UPB_FIELD_AT(msg, upb_strview, UPB_SIZE(40, 48)); }
UPB_INLINE bool google_protobuf_FieldDescriptorProto_has_number(const google_protobuf_FieldDescriptorProto *msg) { return _upb_has_field(msg, 2); }
UPB_INLINE int32_t google_protobuf_FieldDescriptorProto_number(const google_protobuf_FieldDescriptorProto *msg) { return UPB_FIELD_AT(msg, int32_t, UPB_SIZE(24, 24)); }
UPB_INLINE bool google_protobuf_FieldDescriptorProto_has_label(const google_protobuf_FieldDescriptorProto *msg) { return _upb_has_field(msg, 0); }
UPB_INLINE google_protobuf_FieldDescriptorProto_Label google_protobuf_FieldDescriptorProto_label(const google_protobuf_FieldDescriptorProto *msg) { return UPB_FIELD_AT(msg, google_protobuf_FieldDescriptorProto_Label, UPB_SIZE(8, 8)); }
UPB_INLINE bool google_protobuf_FieldDescriptorProto_has_type(const google_protobuf_FieldDescriptorProto *msg) { return _upb_has_field(msg, 1); }
UPB_INLINE google_protobuf_FieldDescriptorProto_Type google_protobuf_FieldDescriptorProto_type(const google_protobuf_FieldDescriptorProto *msg) { return UPB_FIELD_AT(msg, google_protobuf_FieldDescriptorProto_Type, UPB_SIZE(16, 16)); }
UPB_INLINE bool google_protobuf_FieldDescriptorProto_has_type_name(const google_protobuf_FieldDescriptorProto *msg) { return _upb_has_field(msg, 6); }
UPB_INLINE upb_strview google_protobuf_FieldDescriptorProto_type_name(const google_protobuf_FieldDescriptorProto *msg) { return UPB_FIELD_AT(msg, upb_strview, UPB_SIZE(48, 64)); }
UPB_INLINE bool google_protobuf_FieldDescriptorProto_has_default_value(const google_protobuf_FieldDescriptorProto *msg) { return _upb_has_field(msg, 7); }
UPB_INLINE upb_strview google_protobuf_FieldDescriptorProto_default_value(const google_protobuf_FieldDescriptorProto *msg) { return UPB_FIELD_AT(msg, upb_strview, UPB_SIZE(56, 80)); }
UPB_INLINE bool google_protobuf_FieldDescriptorProto_has_options(const google_protobuf_FieldDescriptorProto *msg) { return _upb_has_field(msg, 9); }
UPB_INLINE const google_protobuf_FieldOptions* google_protobuf_FieldDescriptorProto_options(const google_protobuf_FieldDescriptorProto *msg) { return UPB_FIELD_AT(msg, const google_protobuf_FieldOptions*, UPB_SIZE(72, 112)); }
UPB_INLINE bool google_protobuf_FieldDescriptorProto_has_oneof_index(const google_protobuf_FieldDescriptorProto *msg) { return _upb_has_field(msg, 3); }
UPB_INLINE int32_t google_protobuf_FieldDescriptorProto_oneof_index(const google_protobuf_FieldDescriptorProto *msg) { return UPB_FIELD_AT(msg, int32_t, UPB_SIZE(28, 28)); }
UPB_INLINE bool google_protobuf_FieldDescriptorProto_has_json_name(const google_protobuf_FieldDescriptorProto *msg) { return _upb_has_field(msg, 8); }
UPB_INLINE upb_strview google_protobuf_FieldDescriptorProto_json_name(const google_protobuf_FieldDescriptorProto *msg) { return UPB_FIELD_AT(msg, upb_strview, UPB_SIZE(64, 96)); }

UPB_INLINE void google_protobuf_FieldDescriptorProto_set_name(google_protobuf_FieldDescriptorProto *msg, upb_strview value) {
  _upb_sethas(msg, 4);
  UPB_FIELD_AT(msg, upb_strview, UPB_SIZE(32, 32)) = value;
}
UPB_INLINE void google_protobuf_FieldDescriptorProto_set_extendee(google_protobuf_FieldDescriptorProto *msg, upb_strview value) {
  _upb_sethas(msg, 5);
  UPB_FIELD_AT(msg, upb_strview, UPB_SIZE(40, 48)) = value;
}
UPB_INLINE void google_protobuf_FieldDescriptorProto_set_number(google_protobuf_FieldDescriptorProto *msg, int32_t value) {
  _upb_sethas(msg, 2);
  UPB_FIELD_AT(msg, int32_t, UPB_SIZE(24, 24)) = value;
}
UPB_INLINE void google_protobuf_FieldDescriptorProto_set_label(google_protobuf_FieldDescriptorProto *msg, google_protobuf_FieldDescriptorProto_Label value) {
  _upb_sethas(msg, 0);
  UPB_FIELD_AT(msg, google_protobuf_FieldDescriptorProto_Label, UPB_SIZE(8, 8)) = value;
}
UPB_INLINE void google_protobuf_FieldDescriptorProto_set_type(google_protobuf_FieldDescriptorProto *msg, google_protobuf_FieldDescriptorProto_Type value) {
  _upb_sethas(msg, 1);
  UPB_FIELD_AT(msg, google_protobuf_FieldDescriptorProto_Type, UPB_SIZE(16, 16)) = value;
}
UPB_INLINE void google_protobuf_FieldDescriptorProto_set_type_name(google_protobuf_FieldDescriptorProto *msg, upb_strview value) {
  _upb_sethas(msg, 6);
  UPB_FIELD_AT(msg, upb_strview, UPB_SIZE(48, 64)) = value;
}
UPB_INLINE void google_protobuf_FieldDescriptorProto_set_default_value(google_protobuf_FieldDescriptorProto *msg, upb_strview value) {
  _upb_sethas(msg, 7);
  UPB_FIELD_AT(msg, upb_strview, UPB_SIZE(56, 80)) = value;
}
UPB_INLINE void google_protobuf_FieldDescriptorProto_set_options(google_protobuf_FieldDescriptorProto *msg, google_protobuf_FieldOptions* value) {
  _upb_sethas(msg, 9);
  UPB_FIELD_AT(msg, google_protobuf_FieldOptions*, UPB_SIZE(72, 112)) = value;
}
UPB_INLINE struct google_protobuf_FieldOptions* google_protobuf_FieldDescriptorProto_mutable_options(google_protobuf_FieldDescriptorProto *msg, upb_arena *arena) {
  struct google_protobuf_FieldOptions* sub = (struct google_protobuf_FieldOptions*)google_protobuf_FieldDescriptorProto_options(msg);
  if (sub == NULL) {
    sub = (struct google_protobuf_FieldOptions*)upb_msg_new(&google_protobuf_FieldOptions_msginit, arena);
    if (!sub) return NULL;
    google_protobuf_FieldDescriptorProto_set_options(msg, sub);
  }
  return sub;
}
UPB_INLINE void google_protobuf_FieldDescriptorProto_set_oneof_index(google_protobuf_FieldDescriptorProto *msg, int32_t value) {
  _upb_sethas(msg, 3);
  UPB_FIELD_AT(msg, int32_t, UPB_SIZE(28, 28)) = value;
}
UPB_INLINE void google_protobuf_FieldDescriptorProto_set_json_name(google_protobuf_FieldDescriptorProto *msg, upb_strview value) {
  _upb_sethas(msg, 8);
  UPB_FIELD_AT(msg, upb_strview, UPB_SIZE(64, 96)) = value;
}


/* google.protobuf.OneofDescriptorProto */

UPB_INLINE google_protobuf_OneofDescriptorProto *google_protobuf_OneofDescriptorProto_new(upb_arena *arena) {
  return (google_protobuf_OneofDescriptorProto *)upb_msg_new(&google_protobuf_OneofDescriptorProto_msginit, arena);
}
UPB_INLINE google_protobuf_OneofDescriptorProto *google_protobuf_OneofDescriptorProto_parsenew(upb_strview buf, upb_arena *arena) {
  google_protobuf_OneofDescriptorProto *ret = google_protobuf_OneofDescriptorProto_new(arena);
  return (ret && upb_decode(buf, ret, &google_protobuf_OneofDescriptorProto_msginit)) ? ret : NULL;
}
UPB_INLINE char *google_protobuf_OneofDescriptorProto_serialize(const google_protobuf_OneofDescriptorProto *msg, upb_arena *arena, size_t *len) {
  return upb_encode(msg, &google_protobuf_OneofDescriptorProto_msginit, arena, len);
}

UPB_INLINE bool google_protobuf_OneofDescriptorProto_has_name(const google_protobuf_OneofDescriptorProto *msg) { return _upb_has_field(msg, 0); }
UPB_INLINE upb_strview google_protobuf_OneofDescriptorProto_name(const google_protobuf_OneofDescriptorProto *msg) { return UPB_FIELD_AT(msg, upb_strview, UPB_SIZE(4, 8)); }
UPB_INLINE bool google_protobuf_OneofDescriptorProto_has_options(const google_protobuf_OneofDescriptorProto *msg) { return _upb_has_field(msg, 1); }
UPB_INLINE const google_protobuf_OneofOptions* google_protobuf_OneofDescriptorProto_options(const google_protobuf_OneofDescriptorProto *msg) { return UPB_FIELD_AT(msg, const google_protobuf_OneofOptions*, UPB_SIZE(12, 24)); }

UPB_INLINE void google_protobuf_OneofDescriptorProto_set_name(google_protobuf_OneofDescriptorProto *msg, upb_strview value) {
  _upb_sethas(msg, 0);
  UPB_FIELD_AT(msg, upb_strview, UPB_SIZE(4, 8)) = value;
}
UPB_INLINE void google_protobuf_OneofDescriptorProto_set_options(google_protobuf_OneofDescriptorProto *msg, google_protobuf_OneofOptions* value) {
  _upb_sethas(msg, 1);
  UPB_FIELD_AT(msg, google_protobuf_OneofOptions*, UPB_SIZE(12, 24)) = value;
}
UPB_INLINE struct google_protobuf_OneofOptions* google_protobuf_OneofDescriptorProto_mutable_options(google_protobuf_OneofDescriptorProto *msg, upb_arena *arena) {
  struct google_protobuf_OneofOptions* sub = (struct google_protobuf_OneofOptions*)google_protobuf_OneofDescriptorProto_options(msg);
  if (sub == NULL) {
    sub = (struct google_protobuf_OneofOptions*)upb_msg_new(&google_protobuf_OneofOptions_msginit, arena);
    if (!sub) return NULL;
    google_protobuf_OneofDescriptorProto_set_options(msg, sub);
  }
  return sub;
}


/* google.protobuf.EnumDescriptorProto */

UPB_INLINE google_protobuf_EnumDescriptorProto *google_protobuf_EnumDescriptorProto_new(upb_arena *arena) {
  return (google_protobuf_EnumDescriptorProto *)upb_msg_new(&google_protobuf_EnumDescriptorProto_msginit, arena);
}
UPB_INLINE google_protobuf_EnumDescriptorProto *google_protobuf_EnumDescriptorProto_parsenew(upb_strview buf, upb_arena *arena) {
  google_protobuf_EnumDescriptorProto *ret = google_protobuf_EnumDescriptorProto_new(arena);
  return (ret && upb_decode(buf, ret, &google_protobuf_EnumDescriptorProto_msginit)) ? ret : NULL;
}
UPB_INLINE char *google_protobuf_EnumDescriptorProto_serialize(const google_protobuf_EnumDescriptorProto *msg, upb_arena *arena, size_t *len) {
  return upb_encode(msg, &google_protobuf_EnumDescriptorProto_msginit, arena, len);
}

UPB_INLINE bool google_protobuf_EnumDescriptorProto_has_name(const google_protobuf_EnumDescriptorProto *msg) { return _upb_has_field(msg, 0); }
UPB_INLINE upb_strview google_protobuf_EnumDescriptorProto_name(const google_protobuf_EnumDescriptorProto *msg) { return UPB_FIELD_AT(msg, upb_strview, UPB_SIZE(4, 8)); }
UPB_INLINE const google_protobuf_EnumValueDescriptorProto* const* google_protobuf_EnumDescriptorProto_value(const google_protobuf_EnumDescriptorProto *msg, size_t *len) { return (const google_protobuf_EnumValueDescriptorProto* const*)_upb_array_accessor(msg, UPB_SIZE(16, 32), len); }
UPB_INLINE bool google_protobuf_EnumDescriptorProto_has_options(const google_protobuf_EnumDescriptorProto *msg) { return _upb_has_field(msg, 1); }
UPB_INLINE const google_protobuf_EnumOptions* google_protobuf_EnumDescriptorProto_options(const google_protobuf_EnumDescriptorProto *msg) { return UPB_FIELD_AT(msg, const google_protobuf_EnumOptions*, UPB_SIZE(12, 24)); }
UPB_INLINE const google_protobuf_EnumDescriptorProto_EnumReservedRange* const* google_protobuf_EnumDescriptorProto_reserved_range(const google_protobuf_EnumDescriptorProto *msg, size_t *len) { return (const google_protobuf_EnumDescriptorProto_EnumReservedRange* const*)_upb_array_accessor(msg, UPB_SIZE(20, 40), len); }
UPB_INLINE upb_strview const* google_protobuf_EnumDescriptorProto_reserved_name(const google_protobuf_EnumDescriptorProto *msg, size_t *len) { return (upb_strview const*)_upb_array_accessor(msg, UPB_SIZE(24, 48), len); }

UPB_INLINE void google_protobuf_EnumDescriptorProto_set_name(google_protobuf_EnumDescriptorProto *msg, upb_strview value) {
  _upb_sethas(msg, 0);
  UPB_FIELD_AT(msg, upb_strview, UPB_SIZE(4, 8)) = value;
}
UPB_INLINE google_protobuf_EnumValueDescriptorProto** google_protobuf_EnumDescriptorProto_mutable_value(google_protobuf_EnumDescriptorProto *msg, size_t *len) {
  return (google_protobuf_EnumValueDescriptorProto**)_upb_array_mutable_accessor(msg, UPB_SIZE(16, 32), len);
}
UPB_INLINE google_protobuf_EnumValueDescriptorProto** google_protobuf_EnumDescriptorProto_resize_value(google_protobuf_EnumDescriptorProto *msg, size_t len, upb_arena *arena) {
  return (google_protobuf_EnumValueDescriptorProto**)_upb_array_resize_accessor(msg, UPB_SIZE(16, 32), len, UPB_SIZE(4, 8), UPB_TYPE_MESSAGE, arena);
}
UPB_INLINE struct google_protobuf_EnumValueDescriptorProto* google_protobuf_EnumDescriptorProto_add_value(google_protobuf_EnumDescriptorProto *msg, upb_arena *arena) {
  struct google_protobuf_EnumValueDescriptorProto* sub = (struct google_protobuf_EnumValueDescriptorProto*)upb_msg_new(&google_protobuf_EnumValueDescriptorProto_msginit, arena);
  bool ok = _upb_array_append_accessor(
      msg, UPB_SIZE(16, 32), UPB_SIZE(4, 8), UPB_TYPE_MESSAGE, &sub, arena);
  if (!ok) return NULL;
  return sub;
}
UPB_INLINE void google_protobuf_EnumDescriptorProto_set_options(google_protobuf_EnumDescriptorProto *msg, google_protobuf_EnumOptions* value) {
  _upb_sethas(msg, 1);
  UPB_FIELD_AT(msg, google_protobuf_EnumOptions*, UPB_SIZE(12, 24)) = value;
}
UPB_INLINE struct google_protobuf_EnumOptions* google_protobuf_EnumDescriptorProto_mutable_options(google_protobuf_EnumDescriptorProto *msg, upb_arena *arena) {
  struct google_protobuf_EnumOptions* sub = (struct google_protobuf_EnumOptions*)google_protobuf_EnumDescriptorProto_options(msg);
  if (sub == NULL) {
    sub = (struct google_protobuf_EnumOptions*)upb_msg_new(&google_protobuf_EnumOptions_msginit, arena);
    if (!sub) return NULL;
    google_protobuf_EnumDescriptorProto_set_options(msg, sub);
  }
  return sub;
}
UPB_INLINE google_protobuf_EnumDescriptorProto_EnumReservedRange** google_protobuf_EnumDescriptorProto_mutable_reserved_range(google_protobuf_EnumDescriptorProto *msg, size_t *len) {
  return (google_protobuf_EnumDescriptorProto_EnumReservedRange**)_upb_array_mutable_accessor(msg, UPB_SIZE(20, 40), len);
}
UPB_INLINE google_protobuf_EnumDescriptorProto_EnumReservedRange** google_protobuf_EnumDescriptorProto_resize_reserved_range(google_protobuf_EnumDescriptorProto *msg, size_t len, upb_arena *arena) {
  return (google_protobuf_EnumDescriptorProto_EnumReservedRange**)_upb_array_resize_accessor(msg, UPB_SIZE(20, 40), len, UPB_SIZE(4, 8), UPB_TYPE_MESSAGE, arena);
}
UPB_INLINE struct google_protobuf_EnumDescriptorProto_EnumReservedRange* google_protobuf_EnumDescriptorProto_add_reserved_range(google_protobuf_EnumDescriptorProto *msg, upb_arena *arena) {
  struct google_protobuf_EnumDescriptorProto_EnumReservedRange* sub = (struct google_protobuf_EnumDescriptorProto_EnumReservedRange*)upb_msg_new(&google_protobuf_EnumDescriptorProto_EnumReservedRange_msginit, arena);
  bool ok = _upb_array_append_accessor(
      msg, UPB_SIZE(20, 40), UPB_SIZE(4, 8), UPB_TYPE_MESSAGE, &sub, arena);
  if (!ok) return NULL;
  return sub;
}
UPB_INLINE upb_strview* google_protobuf_EnumDescriptorProto_mutable_reserved_name(google_protobuf_EnumDescriptorProto *msg, size_t *len) {
  return (upb_strview*)_upb_array_mutable_accessor(msg, UPB_SIZE(24, 48), len);
}
UPB_INLINE upb_strview* google_protobuf_EnumDescriptorProto_resize_reserved_name(google_protobuf_EnumDescriptorProto *msg, size_t len, upb_arena *arena) {
  return (upb_strview*)_upb_array_resize_accessor(msg, UPB_SIZE(24, 48), len, UPB_SIZE(8, 16), UPB_TYPE_STRING, arena);
}
UPB_INLINE bool google_protobuf_EnumDescriptorProto_add_reserved_name(google_protobuf_EnumDescriptorProto *msg, upb_strview val, upb_arena *arena) {
  return _upb_array_append_accessor(
      msg, UPB_SIZE(24, 48), UPB_SIZE(8, 16), UPB_TYPE_STRING, &val, arena);
}


/* google.protobuf.EnumDescriptorProto.EnumReservedRange */

UPB_INLINE google_protobuf_EnumDescriptorProto_EnumReservedRange *google_protobuf_EnumDescriptorProto_EnumReservedRange_new(upb_arena *arena) {
  return (google_protobuf_EnumDescriptorProto_EnumReservedRange *)upb_msg_new(&google_protobuf_EnumDescriptorProto_EnumReservedRange_msginit, arena);
}
UPB_INLINE google_protobuf_EnumDescriptorProto_EnumReservedRange *google_protobuf_EnumDescriptorProto_EnumReservedRange_parsenew(upb_strview buf, upb_arena *arena) {
  google_protobuf_EnumDescriptorProto_EnumReservedRange *ret = google_protobuf_EnumDescriptorProto_EnumReservedRange_new(arena);
  return (ret && upb_decode(buf, ret, &google_protobuf_EnumDescriptorProto_EnumReservedRange_msginit)) ? ret : NULL;
}
UPB_INLINE char *google_protobuf_EnumDescriptorProto_EnumReservedRange_serialize(const google_protobuf_EnumDescriptorProto_EnumReservedRange *msg, upb_arena *arena, size_t *len) {
  return upb_encode(msg, &google_protobuf_EnumDescriptorProto_EnumReservedRange_msginit, arena, len);
}

UPB_INLINE bool google_protobuf_EnumDescriptorProto_EnumReservedRange_has_start(const google_protobuf_EnumDescriptorProto_EnumReservedRange *msg) { return _upb_has_field(msg, 0); }
UPB_INLINE int32_t google_protobuf_EnumDescriptorProto_EnumReservedRange_start(const google_protobuf_EnumDescriptorProto_EnumReservedRange *msg) { return UPB_FIELD_AT(msg, int32_t, UPB_SIZE(4, 4)); }
UPB_INLINE bool google_protobuf_EnumDescriptorProto_EnumReservedRange_has_end(const google_protobuf_EnumDescriptorProto_EnumReservedRange *msg) { return _upb_has_field(msg, 1); }
UPB_INLINE int32_t google_protobuf_EnumDescriptorProto_EnumReservedRange_end(const google_protobuf_EnumDescriptorProto_EnumReservedRange *msg) { return UPB_FIELD_AT(msg, int32_t, UPB_SIZE(8, 8)); }

UPB_INLINE void google_protobuf_EnumDescriptorProto_EnumReservedRange_set_start(google_protobuf_EnumDescriptorProto_EnumReservedRange *msg, int32_t value) {
  _upb_sethas(msg, 0);
  UPB_FIELD_AT(msg, int32_t, UPB_SIZE(4, 4)) = value;
}
UPB_INLINE void google_protobuf_EnumDescriptorProto_EnumReservedRange_set_end(google_protobuf_EnumDescriptorProto_EnumReservedRange *msg, int32_t value) {
  _upb_sethas(msg, 1);
  UPB_FIELD_AT(msg, int32_t, UPB_SIZE(8, 8)) = value;
}


/* google.protobuf.EnumValueDescriptorProto */

UPB_INLINE google_protobuf_EnumValueDescriptorProto *google_protobuf_EnumValueDescriptorProto_new(upb_arena *arena) {
  return (google_protobuf_EnumValueDescriptorProto *)upb_msg_new(&google_protobuf_EnumValueDescriptorProto_msginit, arena);
}
UPB_INLINE google_protobuf_EnumValueDescriptorProto *google_protobuf_EnumValueDescriptorProto_parsenew(upb_strview buf, upb_arena *arena) {
  google_protobuf_EnumValueDescriptorProto *ret = google_protobuf_EnumValueDescriptorProto_new(arena);
  return (ret && upb_decode(buf, ret, &google_protobuf_EnumValueDescriptorProto_msginit)) ? ret : NULL;
}
UPB_INLINE char *google_protobuf_EnumValueDescriptorProto_serialize(const google_protobuf_EnumValueDescriptorProto *msg, upb_arena *arena, size_t *len) {
  return upb_encode(msg, &google_protobuf_EnumValueDescriptorProto_msginit, arena, len);
}

UPB_INLINE bool google_protobuf_EnumValueDescriptorProto_has_name(const google_protobuf_EnumValueDescriptorProto *msg) { return _upb_has_field(msg, 1); }
UPB_INLINE upb_strview google_protobuf_EnumValueDescriptorProto_name(const google_protobuf_EnumValueDescriptorProto *msg) { return UPB_FIELD_AT(msg, upb_strview, UPB_SIZE(8, 8)); }
UPB_INLINE bool google_protobuf_EnumValueDescriptorProto_has_number(const google_protobuf_EnumValueDescriptorProto *msg) { return _upb_has_field(msg, 0); }
UPB_INLINE int32_t google_protobuf_EnumValueDescriptorProto_number(const google_protobuf_EnumValueDescriptorProto *msg) { return UPB_FIELD_AT(msg, int32_t, UPB_SIZE(4, 4)); }
UPB_INLINE bool google_protobuf_EnumValueDescriptorProto_has_options(const google_protobuf_EnumValueDescriptorProto *msg) { return _upb_has_field(msg, 2); }
UPB_INLINE const google_protobuf_EnumValueOptions* google_protobuf_EnumValueDescriptorProto_options(const google_protobuf_EnumValueDescriptorProto *msg) { return UPB_FIELD_AT(msg, const google_protobuf_EnumValueOptions*, UPB_SIZE(16, 24)); }

UPB_INLINE void google_protobuf_EnumValueDescriptorProto_set_name(google_protobuf_EnumValueDescriptorProto *msg, upb_strview value) {
  _upb_sethas(msg, 1);
  UPB_FIELD_AT(msg, upb_strview, UPB_SIZE(8, 8)) = value;
}
UPB_INLINE void google_protobuf_EnumValueDescriptorProto_set_number(google_protobuf_EnumValueDescriptorProto *msg, int32_t value) {
  _upb_sethas(msg, 0);
  UPB_FIELD_AT(msg, int32_t, UPB_SIZE(4, 4)) = value;
}
UPB_INLINE void google_protobuf_EnumValueDescriptorProto_set_options(google_protobuf_EnumValueDescriptorProto *msg, google_protobuf_EnumValueOptions* value) {
  _upb_sethas(msg, 2);
  UPB_FIELD_AT(msg, google_protobuf_EnumValueOptions*, UPB_SIZE(16, 24)) = value;
}
UPB_INLINE struct google_protobuf_EnumValueOptions* google_protobuf_EnumValueDescriptorProto_mutable_options(google_protobuf_EnumValueDescriptorProto *msg, upb_arena *arena) {
  struct google_protobuf_EnumValueOptions* sub = (struct google_protobuf_EnumValueOptions*)google_protobuf_EnumValueDescriptorProto_options(msg);
  if (sub == NULL) {
    sub = (struct google_protobuf_EnumValueOptions*)upb_msg_new(&google_protobuf_EnumValueOptions_msginit, arena);
    if (!sub) return NULL;
    google_protobuf_EnumValueDescriptorProto_set_options(msg, sub);
  }
  return sub;
}


/* google.protobuf.ServiceDescriptorProto */

UPB_INLINE google_protobuf_ServiceDescriptorProto *google_protobuf_ServiceDescriptorProto_new(upb_arena *arena) {
  return (google_protobuf_ServiceDescriptorProto *)upb_msg_new(&google_protobuf_ServiceDescriptorProto_msginit, arena);
}
UPB_INLINE google_protobuf_ServiceDescriptorProto *google_protobuf_ServiceDescriptorProto_parsenew(upb_strview buf, upb_arena *arena) {
  google_protobuf_ServiceDescriptorProto *ret = google_protobuf_ServiceDescriptorProto_new(arena);
  return (ret && upb_decode(buf, ret, &google_protobuf_ServiceDescriptorProto_msginit)) ? ret : NULL;
}
UPB_INLINE char *google_protobuf_ServiceDescriptorProto_serialize(const google_protobuf_ServiceDescriptorProto *msg, upb_arena *arena, size_t *len) {
  return upb_encode(msg, &google_protobuf_ServiceDescriptorProto_msginit, arena, len);
}

UPB_INLINE bool google_protobuf_ServiceDescriptorProto_has_name(const google_protobuf_ServiceDescriptorProto *msg) { return _upb_has_field(msg, 0); }
UPB_INLINE upb_strview google_protobuf_ServiceDescriptorProto_name(const google_protobuf_ServiceDescriptorProto *msg) { return UPB_FIELD_AT(msg, upb_strview, UPB_SIZE(4, 8)); }
UPB_INLINE const google_protobuf_MethodDescriptorProto* const* google_protobuf_ServiceDescriptorProto_method(const google_protobuf_ServiceDescriptorProto *msg, size_t *len) { return (const google_protobuf_MethodDescriptorProto* const*)_upb_array_accessor(msg, UPB_SIZE(16, 32), len); }
UPB_INLINE bool google_protobuf_ServiceDescriptorProto_has_options(const google_protobuf_ServiceDescriptorProto *msg) { return _upb_has_field(msg, 1); }
UPB_INLINE const google_protobuf_ServiceOptions* google_protobuf_ServiceDescriptorProto_options(const google_protobuf_ServiceDescriptorProto *msg) { return UPB_FIELD_AT(msg, const google_protobuf_ServiceOptions*, UPB_SIZE(12, 24)); }

UPB_INLINE void google_protobuf_ServiceDescriptorProto_set_name(google_protobuf_ServiceDescriptorProto *msg, upb_strview value) {
  _upb_sethas(msg, 0);
  UPB_FIELD_AT(msg, upb_strview, UPB_SIZE(4, 8)) = value;
}
UPB_INLINE google_protobuf_MethodDescriptorProto** google_protobuf_ServiceDescriptorProto_mutable_method(google_protobuf_ServiceDescriptorProto *msg, size_t *len) {
  return (google_protobuf_MethodDescriptorProto**)_upb_array_mutable_accessor(msg, UPB_SIZE(16, 32), len);
}
UPB_INLINE google_protobuf_MethodDescriptorProto** google_protobuf_ServiceDescriptorProto_resize_method(google_protobuf_ServiceDescriptorProto *msg, size_t len, upb_arena *arena) {
  return (google_protobuf_MethodDescriptorProto**)_upb_array_resize_accessor(msg, UPB_SIZE(16, 32), len, UPB_SIZE(4, 8), UPB_TYPE_MESSAGE, arena);
}
UPB_INLINE struct google_protobuf_MethodDescriptorProto* google_protobuf_ServiceDescriptorProto_add_method(google_protobuf_ServiceDescriptorProto *msg, upb_arena *arena) {
  struct google_protobuf_MethodDescriptorProto* sub = (struct google_protobuf_MethodDescriptorProto*)upb_msg_new(&google_protobuf_MethodDescriptorProto_msginit, arena);
  bool ok = _upb_array_append_accessor(
      msg, UPB_SIZE(16, 32), UPB_SIZE(4, 8), UPB_TYPE_MESSAGE, &sub, arena);
  if (!ok) return NULL;
  return sub;
}
UPB_INLINE void google_protobuf_ServiceDescriptorProto_set_options(google_protobuf_ServiceDescriptorProto *msg, google_protobuf_ServiceOptions* value) {
  _upb_sethas(msg, 1);
  UPB_FIELD_AT(msg, google_protobuf_ServiceOptions*, UPB_SIZE(12, 24)) = value;
}
UPB_INLINE struct google_protobuf_ServiceOptions* google_protobuf_ServiceDescriptorProto_mutable_options(google_protobuf_ServiceDescriptorProto *msg, upb_arena *arena) {
  struct google_protobuf_ServiceOptions* sub = (struct google_protobuf_ServiceOptions*)google_protobuf_ServiceDescriptorProto_options(msg);
  if (sub == NULL) {
    sub = (struct google_protobuf_ServiceOptions*)upb_msg_new(&google_protobuf_ServiceOptions_msginit, arena);
    if (!sub) return NULL;
    google_protobuf_ServiceDescriptorProto_set_options(msg, sub);
  }
  return sub;
}


/* google.protobuf.MethodDescriptorProto */

UPB_INLINE google_protobuf_MethodDescriptorProto *google_protobuf_MethodDescriptorProto_new(upb_arena *arena) {
  return (google_protobuf_MethodDescriptorProto *)upb_msg_new(&google_protobuf_MethodDescriptorProto_msginit, arena);
}
UPB_INLINE google_protobuf_MethodDescriptorProto *google_protobuf_MethodDescriptorProto_parsenew(upb_strview buf, upb_arena *arena) {
  google_protobuf_MethodDescriptorProto *ret = google_protobuf_MethodDescriptorProto_new(arena);
  return (ret && upb_decode(buf, ret, &google_protobuf_MethodDescriptorProto_msginit)) ? ret : NULL;
}
UPB_INLINE char *google_protobuf_MethodDescriptorProto_serialize(const google_protobuf_MethodDescriptorProto *msg, upb_arena *arena, size_t *len) {
  return upb_encode(msg, &google_protobuf_MethodDescriptorProto_msginit, arena, len);
}

UPB_INLINE bool google_protobuf_MethodDescriptorProto_has_name(const google_protobuf_MethodDescriptorProto *msg) { return _upb_has_field(msg, 2); }
UPB_INLINE upb_strview google_protobuf_MethodDescriptorProto_name(const google_protobuf_MethodDescriptorProto *msg) { return UPB_FIELD_AT(msg, upb_strview, UPB_SIZE(4, 8)); }
UPB_INLINE bool google_protobuf_MethodDescriptorProto_has_input_type(const google_protobuf_MethodDescriptorProto *msg) { return _upb_has_field(msg, 3); }
UPB_INLINE upb_strview google_protobuf_MethodDescriptorProto_input_type(const google_protobuf_MethodDescriptorProto *msg) { return UPB_FIELD_AT(msg, upb_strview, UPB_SIZE(12, 24)); }
UPB_INLINE bool google_protobuf_MethodDescriptorProto_has_output_type(const google_protobuf_MethodDescriptorProto *msg) { return _upb_has_field(msg, 4); }
UPB_INLINE upb_strview google_protobuf_MethodDescriptorProto_output_type(const google_protobuf_MethodDescriptorProto *msg) { return UPB_FIELD_AT(msg, upb_strview, UPB_SIZE(20, 40)); }
UPB_INLINE bool google_protobuf_MethodDescriptorProto_has_options(const google_protobuf_MethodDescriptorProto *msg) { return _upb_has_field(msg, 5); }
UPB_INLINE const google_protobuf_MethodOptions* google_protobuf_MethodDescriptorProto_options(const google_protobuf_MethodDescriptorProto *msg) { return UPB_FIELD_AT(msg, const google_protobuf_MethodOptions*, UPB_SIZE(28, 56)); }
UPB_INLINE bool google_protobuf_MethodDescriptorProto_has_client_streaming(const google_protobuf_MethodDescriptorProto *msg) { return _upb_has_field(msg, 0); }
UPB_INLINE bool google_protobuf_MethodDescriptorProto_client_streaming(const google_protobuf_MethodDescriptorProto *msg) { return UPB_FIELD_AT(msg, bool, UPB_SIZE(1, 1)); }
UPB_INLINE bool google_protobuf_MethodDescriptorProto_has_server_streaming(const google_protobuf_MethodDescriptorProto *msg) { return _upb_has_field(msg, 1); }
UPB_INLINE bool google_protobuf_MethodDescriptorProto_server_streaming(const google_protobuf_MethodDescriptorProto *msg) { return UPB_FIELD_AT(msg, bool, UPB_SIZE(2, 2)); }

UPB_INLINE void google_protobuf_MethodDescriptorProto_set_name(google_protobuf_MethodDescriptorProto *msg, upb_strview value) {
  _upb_sethas(msg, 2);
  UPB_FIELD_AT(msg, upb_strview, UPB_SIZE(4, 8)) = value;
}
UPB_INLINE void google_protobuf_MethodDescriptorProto_set_input_type(google_protobuf_MethodDescriptorProto *msg, upb_strview value) {
  _upb_sethas(msg, 3);
  UPB_FIELD_AT(msg, upb_strview, UPB_SIZE(12, 24)) = value;
}
UPB_INLINE void google_protobuf_MethodDescriptorProto_set_output_type(google_protobuf_MethodDescriptorProto *msg, upb_strview value) {
  _upb_sethas(msg, 4);
  UPB_FIELD_AT(msg, upb_strview, UPB_SIZE(20, 40)) = value;
}
UPB_INLINE void google_protobuf_MethodDescriptorProto_set_options(google_protobuf_MethodDescriptorProto *msg, google_protobuf_MethodOptions* value) {
  _upb_sethas(msg, 5);
  UPB_FIELD_AT(msg, google_protobuf_MethodOptions*, UPB_SIZE(28, 56)) = value;
}
UPB_INLINE struct google_protobuf_MethodOptions* google_protobuf_MethodDescriptorProto_mutable_options(google_protobuf_MethodDescriptorProto *msg, upb_arena *arena) {
  struct google_protobuf_MethodOptions* sub = (struct google_protobuf_MethodOptions*)google_protobuf_MethodDescriptorProto_options(msg);
  if (sub == NULL) {
    sub = (struct google_protobuf_MethodOptions*)upb_msg_new(&google_protobuf_MethodOptions_msginit, arena);
    if (!sub) return NULL;
    google_protobuf_MethodDescriptorProto_set_options(msg, sub);
  }
  return sub;
}
UPB_INLINE void google_protobuf_MethodDescriptorProto_set_client_streaming(google_protobuf_MethodDescriptorProto *msg, bool value) {
  _upb_sethas(msg, 0);
  UPB_FIELD_AT(msg, bool, UPB_SIZE(1, 1)) = value;
}
UPB_INLINE void google_protobuf_MethodDescriptorProto_set_server_streaming(google_protobuf_MethodDescriptorProto *msg, bool value) {
  _upb_sethas(msg, 1);
  UPB_FIELD_AT(msg, bool, UPB_SIZE(2, 2)) = value;
}


/* google.protobuf.FileOptions */

UPB_INLINE google_protobuf_FileOptions *google_protobuf_FileOptions_new(upb_arena *arena) {
  return (google_protobuf_FileOptions *)upb_msg_new(&google_protobuf_FileOptions_msginit, arena);
}
UPB_INLINE google_protobuf_FileOptions *google_protobuf_FileOptions_parsenew(upb_strview buf, upb_arena *arena) {
  google_protobuf_FileOptions *ret = google_protobuf_FileOptions_new(arena);
  return (ret && upb_decode(buf, ret, &google_protobuf_FileOptions_msginit)) ? ret : NULL;
}
UPB_INLINE char *google_protobuf_FileOptions_serialize(const google_protobuf_FileOptions *msg, upb_arena *arena, size_t *len) {
  return upb_encode(msg, &google_protobuf_FileOptions_msginit, arena, len);
}

UPB_INLINE bool google_protobuf_FileOptions_has_java_package(const google_protobuf_FileOptions *msg) { return _upb_has_field(msg, 10); }
UPB_INLINE upb_strview google_protobuf_FileOptions_java_package(const google_protobuf_FileOptions *msg) { return UPB_FIELD_AT(msg, upb_strview, UPB_SIZE(28, 32)); }
UPB_INLINE bool google_protobuf_FileOptions_has_java_outer_classname(const google_protobuf_FileOptions *msg) { return _upb_has_field(msg, 11); }
UPB_INLINE upb_strview google_protobuf_FileOptions_java_outer_classname(const google_protobuf_FileOptions *msg) { return UPB_FIELD_AT(msg, upb_strview, UPB_SIZE(36, 48)); }
UPB_INLINE bool google_protobuf_FileOptions_has_optimize_for(const google_protobuf_FileOptions *msg) { return _upb_has_field(msg, 0); }
UPB_INLINE google_protobuf_FileOptions_OptimizeMode google_protobuf_FileOptions_optimize_for(const google_protobuf_FileOptions *msg) { return UPB_FIELD_AT(msg, google_protobuf_FileOptions_OptimizeMode, UPB_SIZE(8, 8)); }
UPB_INLINE bool google_protobuf_FileOptions_has_java_multiple_files(const google_protobuf_FileOptions *msg) { return _upb_has_field(msg, 1); }
UPB_INLINE bool google_protobuf_FileOptions_java_multiple_files(const google_protobuf_FileOptions *msg) { return UPB_FIELD_AT(msg, bool, UPB_SIZE(16, 16)); }
UPB_INLINE bool google_protobuf_FileOptions_has_go_package(const google_protobuf_FileOptions *msg) { return _upb_has_field(msg, 12); }
UPB_INLINE upb_strview google_protobuf_FileOptions_go_package(const google_protobuf_FileOptions *msg) { return UPB_FIELD_AT(msg, upb_strview, UPB_SIZE(44, 64)); }
UPB_INLINE bool google_protobuf_FileOptions_has_cc_generic_services(const google_protobuf_FileOptions *msg) { return _upb_has_field(msg, 2); }
UPB_INLINE bool google_protobuf_FileOptions_cc_generic_services(const google_protobuf_FileOptions *msg) { return UPB_FIELD_AT(msg, bool, UPB_SIZE(17, 17)); }
UPB_INLINE bool google_protobuf_FileOptions_has_java_generic_services(const google_protobuf_FileOptions *msg) { return _upb_has_field(msg, 3); }
UPB_INLINE bool google_protobuf_FileOptions_java_generic_services(const google_protobuf_FileOptions *msg) { return UPB_FIELD_AT(msg, bool, UPB_SIZE(18, 18)); }
UPB_INLINE bool google_protobuf_FileOptions_has_py_generic_services(const google_protobuf_FileOptions *msg) { return _upb_has_field(msg, 4); }
UPB_INLINE bool google_protobuf_FileOptions_py_generic_services(const google_protobuf_FileOptions *msg) { return UPB_FIELD_AT(msg, bool, UPB_SIZE(19, 19)); }
UPB_INLINE bool google_protobuf_FileOptions_has_java_generate_equals_and_hash(const google_protobuf_FileOptions *msg) { return _upb_has_field(msg, 5); }
UPB_INLINE bool google_protobuf_FileOptions_java_generate_equals_and_hash(const google_protobuf_FileOptions *msg) { return UPB_FIELD_AT(msg, bool, UPB_SIZE(20, 20)); }
UPB_INLINE bool google_protobuf_FileOptions_has_deprecated(const google_protobuf_FileOptions *msg) { return _upb_has_field(msg, 6); }
UPB_INLINE bool google_protobuf_FileOptions_deprecated(const google_protobuf_FileOptions *msg) { return UPB_FIELD_AT(msg, bool, UPB_SIZE(21, 21)); }
UPB_INLINE bool google_protobuf_FileOptions_has_java_string_check_utf8(const google_protobuf_FileOptions *msg) { return _upb_has_field(msg, 7); }
UPB_INLINE bool google_protobuf_FileOptions_java_string_check_utf8(const google_protobuf_FileOptions *msg) { return UPB_FIELD_AT(msg, bool, UPB_SIZE(22, 22)); }
UPB_INLINE bool google_protobuf_FileOptions_has_cc_enable_arenas(const google_protobuf_FileOptions *msg) { return _upb_has_field(msg, 8); }
UPB_INLINE bool google_protobuf_FileOptions_cc_enable_arenas(const google_protobuf_FileOptions *msg) { return UPB_FIELD_AT(msg, bool, UPB_SIZE(23, 23)); }
UPB_INLINE bool google_protobuf_FileOptions_has_objc_class_prefix(const google_protobuf_FileOptions *msg) { return _upb_has_field(msg, 13); }
UPB_INLINE upb_strview google_protobuf_FileOptions_objc_class_prefix(const google_protobuf_FileOptions *msg) { return UPB_FIELD_AT(msg, upb_strview, UPB_SIZE(52, 80)); }
UPB_INLINE bool google_protobuf_FileOptions_has_csharp_namespace(const google_protobuf_FileOptions *msg) { return _upb_has_field(msg, 14); }
UPB_INLINE upb_strview google_protobuf_FileOptions_csharp_namespace(const google_protobuf_FileOptions *msg) { return UPB_FIELD_AT(msg, upb_strview, UPB_SIZE(60, 96)); }
UPB_INLINE bool google_protobuf_FileOptions_has_swift_prefix(const google_protobuf_FileOptions *msg) { return _upb_has_field(msg, 15); }
UPB_INLINE upb_strview google_protobuf_FileOptions_swift_prefix(const google_protobuf_FileOptions *msg) { return UPB_FIELD_AT(msg, upb_strview, UPB_SIZE(68, 112)); }
UPB_INLINE bool google_protobuf_FileOptions_has_php_class_prefix(const google_protobuf_FileOptions *msg) { return _upb_has_field(msg, 16); }
UPB_INLINE upb_strview google_protobuf_FileOptions_php_class_prefix(const google_protobuf_FileOptions *msg) { return UPB_FIELD_AT(msg, upb_strview, UPB_SIZE(76, 128)); }
UPB_INLINE bool google_protobuf_FileOptions_has_php_namespace(const google_protobuf_FileOptions *msg) { return _upb_has_field(msg, 17); }
UPB_INLINE upb_strview google_protobuf_FileOptions_php_namespace(const google_protobuf_FileOptions *msg) { return UPB_FIELD_AT(msg, upb_strview, UPB_SIZE(84, 144)); }
UPB_INLINE bool google_protobuf_FileOptions_has_php_generic_services(const google_protobuf_FileOptions *msg) { return _upb_has_field(msg, 9); }
UPB_INLINE bool google_protobuf_FileOptions_php_generic_services(const google_protobuf_FileOptions *msg) { return UPB_FIELD_AT(msg, bool, UPB_SIZE(24, 24)); }
UPB_INLINE const google_protobuf_UninterpretedOption* const* google_protobuf_FileOptions_uninterpreted_option(const google_protobuf_FileOptions *msg, size_t *len) { return (const google_protobuf_UninterpretedOption* const*)_upb_array_accessor(msg, UPB_SIZE(92, 160), len); }

UPB_INLINE void google_protobuf_FileOptions_set_java_package(google_protobuf_FileOptions *msg, upb_strview value) {
  _upb_sethas(msg, 10);
  UPB_FIELD_AT(msg, upb_strview, UPB_SIZE(28, 32)) = value;
}
UPB_INLINE void google_protobuf_FileOptions_set_java_outer_classname(google_protobuf_FileOptions *msg, upb_strview value) {
  _upb_sethas(msg, 11);
  UPB_FIELD_AT(msg, upb_strview, UPB_SIZE(36, 48)) = value;
}
UPB_INLINE void google_protobuf_FileOptions_set_optimize_for(google_protobuf_FileOptions *msg, google_protobuf_FileOptions_OptimizeMode value) {
  _upb_sethas(msg, 0);
  UPB_FIELD_AT(msg, google_protobuf_FileOptions_OptimizeMode, UPB_SIZE(8, 8)) = value;
}
UPB_INLINE void google_protobuf_FileOptions_set_java_multiple_files(google_protobuf_FileOptions *msg, bool value) {
  _upb_sethas(msg, 1);
  UPB_FIELD_AT(msg, bool, UPB_SIZE(16, 16)) = value;
}
UPB_INLINE void google_protobuf_FileOptions_set_go_package(google_protobuf_FileOptions *msg, upb_strview value) {
  _upb_sethas(msg, 12);
  UPB_FIELD_AT(msg, upb_strview, UPB_SIZE(44, 64)) = value;
}
UPB_INLINE void google_protobuf_FileOptions_set_cc_generic_services(google_protobuf_FileOptions *msg, bool value) {
  _upb_sethas(msg, 2);
  UPB_FIELD_AT(msg, bool, UPB_SIZE(17, 17)) = value;
}
UPB_INLINE void google_protobuf_FileOptions_set_java_generic_services(google_protobuf_FileOptions *msg, bool value) {
  _upb_sethas(msg, 3);
  UPB_FIELD_AT(msg, bool, UPB_SIZE(18, 18)) = value;
}
UPB_INLINE void google_protobuf_FileOptions_set_py_generic_services(google_protobuf_FileOptions *msg, bool value) {
  _upb_sethas(msg, 4);
  UPB_FIELD_AT(msg, bool, UPB_SIZE(19, 19)) = value;
}
UPB_INLINE void google_protobuf_FileOptions_set_java_generate_equals_and_hash(google_protobuf_FileOptions *msg, bool value) {
  _upb_sethas(msg, 5);
  UPB_FIELD_AT(msg, bool, UPB_SIZE(20, 20)) = value;
}
UPB_INLINE void google_protobuf_FileOptions_set_deprecated(google_protobuf_FileOptions *msg, bool value) {
  _upb_sethas(msg, 6);
  UPB_FIELD_AT(msg, bool, UPB_SIZE(21, 21)) = value;
}
UPB_INLINE void google_protobuf_FileOptions_set_java_string_check_utf8(google_protobuf_FileOptions *msg, bool value) {
  _upb_sethas(msg, 7);
  UPB_FIELD_AT(msg, bool, UPB_SIZE(22, 22)) = value;
}
UPB_INLINE void google_protobuf_FileOptions_set_cc_enable_arenas(google_protobuf_FileOptions *msg, bool value) {
  _upb_sethas(msg, 8);
  UPB_FIELD_AT(msg, bool, UPB_SIZE(23, 23)) = value;
}
UPB_INLINE void google_protobuf_FileOptions_set_objc_class_prefix(google_protobuf_FileOptions *msg, upb_strview value) {
  _upb_sethas(msg, 13);
  UPB_FIELD_AT(msg, upb_strview, UPB_SIZE(52, 80)) = value;
}
UPB_INLINE void google_protobuf_FileOptions_set_csharp_namespace(google_protobuf_FileOptions *msg, upb_strview value) {
  _upb_sethas(msg, 14);
  UPB_FIELD_AT(msg, upb_strview, UPB_SIZE(60, 96)) = value;
}
UPB_INLINE void google_protobuf_FileOptions_set_swift_prefix(google_protobuf_FileOptions *msg, upb_strview value) {
  _upb_sethas(msg, 15);
  UPB_FIELD_AT(msg, upb_strview, UPB_SIZE(68, 112)) = value;
}
UPB_INLINE void google_protobuf_FileOptions_set_php_class_prefix(google_protobuf_FileOptions *msg, upb_strview value) {
  _upb_sethas(msg, 16);
  UPB_FIELD_AT(msg, upb_strview, UPB_SIZE(76, 128)) = value;
}
UPB_INLINE void google_protobuf_FileOptions_set_php_namespace(google_protobuf_FileOptions *msg, upb_strview value) {
  _upb_sethas(msg, 17);
  UPB_FIELD_AT(msg, upb_strview, UPB_SIZE(84, 144)) = value;
}
UPB_INLINE void google_protobuf_FileOptions_set_php_generic_services(google_protobuf_FileOptions *msg, bool value) {
  _upb_sethas(msg, 9);
  UPB_FIELD_AT(msg, bool, UPB_SIZE(24, 24)) = value;
}
UPB_INLINE google_protobuf_UninterpretedOption** google_protobuf_FileOptions_mutable_uninterpreted_option(google_protobuf_FileOptions *msg, size_t *len) {
  return (google_protobuf_UninterpretedOption**)_upb_array_mutable_accessor(msg, UPB_SIZE(92, 160), len);
}
UPB_INLINE google_protobuf_UninterpretedOption** google_protobuf_FileOptions_resize_uninterpreted_option(google_protobuf_FileOptions *msg, size_t len, upb_arena *arena) {
  return (google_protobuf_UninterpretedOption**)_upb_array_resize_accessor(msg, UPB_SIZE(92, 160), len, UPB_SIZE(4, 8), UPB_TYPE_MESSAGE, arena);
}
UPB_INLINE struct google_protobuf_UninterpretedOption* google_protobuf_FileOptions_add_uninterpreted_option(google_protobuf_FileOptions *msg, upb_arena *arena) {
  struct google_protobuf_UninterpretedOption* sub = (struct google_protobuf_UninterpretedOption*)upb_msg_new(&google_protobuf_UninterpretedOption_msginit, arena);
  bool ok = _upb_array_append_accessor(
      msg, UPB_SIZE(92, 160), UPB_SIZE(4, 8), UPB_TYPE_MESSAGE, &sub, arena);
  if (!ok) return NULL;
  return sub;
}


/* google.protobuf.MessageOptions */

UPB_INLINE google_protobuf_MessageOptions *google_protobuf_MessageOptions_new(upb_arena *arena) {
  return (google_protobuf_MessageOptions *)upb_msg_new(&google_protobuf_MessageOptions_msginit, arena);
}
UPB_INLINE google_protobuf_MessageOptions *google_protobuf_MessageOptions_parsenew(upb_strview buf, upb_arena *arena) {
  google_protobuf_MessageOptions *ret = google_protobuf_MessageOptions_new(arena);
  return (ret && upb_decode(buf, ret, &google_protobuf_MessageOptions_msginit)) ? ret : NULL;
}
UPB_INLINE char *google_protobuf_MessageOptions_serialize(const google_protobuf_MessageOptions *msg, upb_arena *arena, size_t *len) {
  return upb_encode(msg, &google_protobuf_MessageOptions_msginit, arena, len);
}

UPB_INLINE bool google_protobuf_MessageOptions_has_message_set_wire_format(const google_protobuf_MessageOptions *msg) { return _upb_has_field(msg, 0); }
UPB_INLINE bool google_protobuf_MessageOptions_message_set_wire_format(const google_protobuf_MessageOptions *msg) { return UPB_FIELD_AT(msg, bool, UPB_SIZE(1, 1)); }
UPB_INLINE bool google_protobuf_MessageOptions_has_no_standard_descriptor_accessor(const google_protobuf_MessageOptions *msg) { return _upb_has_field(msg, 1); }
UPB_INLINE bool google_protobuf_MessageOptions_no_standard_descriptor_accessor(const google_protobuf_MessageOptions *msg) { return UPB_FIELD_AT(msg, bool, UPB_SIZE(2, 2)); }
UPB_INLINE bool google_protobuf_MessageOptions_has_deprecated(const google_protobuf_MessageOptions *msg) { return _upb_has_field(msg, 2); }
UPB_INLINE bool google_protobuf_MessageOptions_deprecated(const google_protobuf_MessageOptions *msg) { return UPB_FIELD_AT(msg, bool, UPB_SIZE(3, 3)); }
UPB_INLINE bool google_protobuf_MessageOptions_has_map_entry(const google_protobuf_MessageOptions *msg) { return _upb_has_field(msg, 3); }
UPB_INLINE bool google_protobuf_MessageOptions_map_entry(const google_protobuf_MessageOptions *msg) { return UPB_FIELD_AT(msg, bool, UPB_SIZE(4, 4)); }
UPB_INLINE const google_protobuf_UninterpretedOption* const* google_protobuf_MessageOptions_uninterpreted_option(const google_protobuf_MessageOptions *msg, size_t *len) { return (const google_protobuf_UninterpretedOption* const*)_upb_array_accessor(msg, UPB_SIZE(8, 8), len); }

UPB_INLINE void google_protobuf_MessageOptions_set_message_set_wire_format(google_protobuf_MessageOptions *msg, bool value) {
  _upb_sethas(msg, 0);
  UPB_FIELD_AT(msg, bool, UPB_SIZE(1, 1)) = value;
}
UPB_INLINE void google_protobuf_MessageOptions_set_no_standard_descriptor_accessor(google_protobuf_MessageOptions *msg, bool value) {
  _upb_sethas(msg, 1);
  UPB_FIELD_AT(msg, bool, UPB_SIZE(2, 2)) = value;
}
UPB_INLINE void google_protobuf_MessageOptions_set_deprecated(google_protobuf_MessageOptions *msg, bool value) {
  _upb_sethas(msg, 2);
  UPB_FIELD_AT(msg, bool, UPB_SIZE(3, 3)) = value;
}
UPB_INLINE void google_protobuf_MessageOptions_set_map_entry(google_protobuf_MessageOptions *msg, bool value) {
  _upb_sethas(msg, 3);
  UPB_FIELD_AT(msg, bool, UPB_SIZE(4, 4)) = value;
}
UPB_INLINE google_protobuf_UninterpretedOption** google_protobuf_MessageOptions_mutable_uninterpreted_option(google_protobuf_MessageOptions *msg, size_t *len) {
  return (google_protobuf_UninterpretedOption**)_upb_array_mutable_accessor(msg, UPB_SIZE(8, 8), len);
}
UPB_INLINE google_protobuf_UninterpretedOption** google_protobuf_MessageOptions_resize_uninterpreted_option(google_protobuf_MessageOptions *msg, size_t len, upb_arena *arena) {
  return (google_protobuf_UninterpretedOption**)_upb_array_resize_accessor(msg, UPB_SIZE(8, 8), len, UPB_SIZE(4, 8), UPB_TYPE_MESSAGE, arena);
}
UPB_INLINE struct google_protobuf_UninterpretedOption* google_protobuf_MessageOptions_add_uninterpreted_option(google_protobuf_MessageOptions *msg, upb_arena *arena) {
  struct google_protobuf_UninterpretedOption* sub = (struct google_protobuf_UninterpretedOption*)upb_msg_new(&google_protobuf_UninterpretedOption_msginit, arena);
  bool ok = _upb_array_append_accessor(
      msg, UPB_SIZE(8, 8), UPB_SIZE(4, 8), UPB_TYPE_MESSAGE, &sub, arena);
  if (!ok) return NULL;
  return sub;
}


/* google.protobuf.FieldOptions */

UPB_INLINE google_protobuf_FieldOptions *google_protobuf_FieldOptions_new(upb_arena *arena) {
  return (google_protobuf_FieldOptions *)upb_msg_new(&google_protobuf_FieldOptions_msginit, arena);
}
UPB_INLINE google_protobuf_FieldOptions *google_protobuf_FieldOptions_parsenew(upb_strview buf, upb_arena *arena) {
  google_protobuf_FieldOptions *ret = google_protobuf_FieldOptions_new(arena);
  return (ret && upb_decode(buf, ret, &google_protobuf_FieldOptions_msginit)) ? ret : NULL;
}
UPB_INLINE char *google_protobuf_FieldOptions_serialize(const google_protobuf_FieldOptions *msg, upb_arena *arena, size_t *len) {
  return upb_encode(msg, &google_protobuf_FieldOptions_msginit, arena, len);
}

UPB_INLINE bool google_protobuf_FieldOptions_has_ctype(const google_protobuf_FieldOptions *msg) { return _upb_has_field(msg, 0); }
UPB_INLINE google_protobuf_FieldOptions_CType google_protobuf_FieldOptions_ctype(const google_protobuf_FieldOptions *msg) { return UPB_FIELD_AT(msg, google_protobuf_FieldOptions_CType, UPB_SIZE(8, 8)); }
UPB_INLINE bool google_protobuf_FieldOptions_has_packed(const google_protobuf_FieldOptions *msg) { return _upb_has_field(msg, 2); }
UPB_INLINE bool google_protobuf_FieldOptions_packed(const google_protobuf_FieldOptions *msg) { return UPB_FIELD_AT(msg, bool, UPB_SIZE(24, 24)); }
UPB_INLINE bool google_protobuf_FieldOptions_has_deprecated(const google_protobuf_FieldOptions *msg) { return _upb_has_field(msg, 3); }
UPB_INLINE bool google_protobuf_FieldOptions_deprecated(const google_protobuf_FieldOptions *msg) { return UPB_FIELD_AT(msg, bool, UPB_SIZE(25, 25)); }
UPB_INLINE bool google_protobuf_FieldOptions_has_lazy(const google_protobuf_FieldOptions *msg) { return _upb_has_field(msg, 4); }
UPB_INLINE bool google_protobuf_FieldOptions_lazy(const google_protobuf_FieldOptions *msg) { return UPB_FIELD_AT(msg, bool, UPB_SIZE(26, 26)); }
UPB_INLINE bool google_protobuf_FieldOptions_has_jstype(const google_protobuf_FieldOptions *msg) { return _upb_has_field(msg, 1); }
UPB_INLINE google_protobuf_FieldOptions_JSType google_protobuf_FieldOptions_jstype(const google_protobuf_FieldOptions *msg) { return UPB_FIELD_AT(msg, google_protobuf_FieldOptions_JSType, UPB_SIZE(16, 16)); }
UPB_INLINE bool google_protobuf_FieldOptions_has_weak(const google_protobuf_FieldOptions *msg) { return _upb_has_field(msg, 5); }
UPB_INLINE bool google_protobuf_FieldOptions_weak(const google_protobuf_FieldOptions *msg) { return UPB_FIELD_AT(msg, bool, UPB_SIZE(27, 27)); }
UPB_INLINE const google_protobuf_UninterpretedOption* const* google_protobuf_FieldOptions_uninterpreted_option(const google_protobuf_FieldOptions *msg, size_t *len) { return (const google_protobuf_UninterpretedOption* const*)_upb_array_accessor(msg, UPB_SIZE(28, 32), len); }

UPB_INLINE void google_protobuf_FieldOptions_set_ctype(google_protobuf_FieldOptions *msg, google_protobuf_FieldOptions_CType value) {
  _upb_sethas(msg, 0);
  UPB_FIELD_AT(msg, google_protobuf_FieldOptions_CType, UPB_SIZE(8, 8)) = value;
}
UPB_INLINE void google_protobuf_FieldOptions_set_packed(google_protobuf_FieldOptions *msg, bool value) {
  _upb_sethas(msg, 2);
  UPB_FIELD_AT(msg, bool, UPB_SIZE(24, 24)) = value;
}
UPB_INLINE void google_protobuf_FieldOptions_set_deprecated(google_protobuf_FieldOptions *msg, bool value) {
  _upb_sethas(msg, 3);
  UPB_FIELD_AT(msg, bool, UPB_SIZE(25, 25)) = value;
}
UPB_INLINE void google_protobuf_FieldOptions_set_lazy(google_protobuf_FieldOptions *msg, bool value) {
  _upb_sethas(msg, 4);
  UPB_FIELD_AT(msg, bool, UPB_SIZE(26, 26)) = value;
}
UPB_INLINE void google_protobuf_FieldOptions_set_jstype(google_protobuf_FieldOptions *msg, google_protobuf_FieldOptions_JSType value) {
  _upb_sethas(msg, 1);
  UPB_FIELD_AT(msg, google_protobuf_FieldOptions_JSType, UPB_SIZE(16, 16)) = value;
}
UPB_INLINE void google_protobuf_FieldOptions_set_weak(google_protobuf_FieldOptions *msg, bool value) {
  _upb_sethas(msg, 5);
  UPB_FIELD_AT(msg, bool, UPB_SIZE(27, 27)) = value;
}
UPB_INLINE google_protobuf_UninterpretedOption** google_protobuf_FieldOptions_mutable_uninterpreted_option(google_protobuf_FieldOptions *msg, size_t *len) {
  return (google_protobuf_UninterpretedOption**)_upb_array_mutable_accessor(msg, UPB_SIZE(28, 32), len);
}
UPB_INLINE google_protobuf_UninterpretedOption** google_protobuf_FieldOptions_resize_uninterpreted_option(google_protobuf_FieldOptions *msg, size_t len, upb_arena *arena) {
  return (google_protobuf_UninterpretedOption**)_upb_array_resize_accessor(msg, UPB_SIZE(28, 32), len, UPB_SIZE(4, 8), UPB_TYPE_MESSAGE, arena);
}
UPB_INLINE struct google_protobuf_UninterpretedOption* google_protobuf_FieldOptions_add_uninterpreted_option(google_protobuf_FieldOptions *msg, upb_arena *arena) {
  struct google_protobuf_UninterpretedOption* sub = (struct google_protobuf_UninterpretedOption*)upb_msg_new(&google_protobuf_UninterpretedOption_msginit, arena);
  bool ok = _upb_array_append_accessor(
      msg, UPB_SIZE(28, 32), UPB_SIZE(4, 8), UPB_TYPE_MESSAGE, &sub, arena);
  if (!ok) return NULL;
  return sub;
}


/* google.protobuf.OneofOptions */

UPB_INLINE google_protobuf_OneofOptions *google_protobuf_OneofOptions_new(upb_arena *arena) {
  return (google_protobuf_OneofOptions *)upb_msg_new(&google_protobuf_OneofOptions_msginit, arena);
}
UPB_INLINE google_protobuf_OneofOptions *google_protobuf_OneofOptions_parsenew(upb_strview buf, upb_arena *arena) {
  google_protobuf_OneofOptions *ret = google_protobuf_OneofOptions_new(arena);
  return (ret && upb_decode(buf, ret, &google_protobuf_OneofOptions_msginit)) ? ret : NULL;
}
UPB_INLINE char *google_protobuf_OneofOptions_serialize(const google_protobuf_OneofOptions *msg, upb_arena *arena, size_t *len) {
  return upb_encode(msg, &google_protobuf_OneofOptions_msginit, arena, len);
}

UPB_INLINE const google_protobuf_UninterpretedOption* const* google_protobuf_OneofOptions_uninterpreted_option(const google_protobuf_OneofOptions *msg, size_t *len) { return (const google_protobuf_UninterpretedOption* const*)_upb_array_accessor(msg, UPB_SIZE(0, 0), len); }

UPB_INLINE google_protobuf_UninterpretedOption** google_protobuf_OneofOptions_mutable_uninterpreted_option(google_protobuf_OneofOptions *msg, size_t *len) {
  return (google_protobuf_UninterpretedOption**)_upb_array_mutable_accessor(msg, UPB_SIZE(0, 0), len);
}
UPB_INLINE google_protobuf_UninterpretedOption** google_protobuf_OneofOptions_resize_uninterpreted_option(google_protobuf_OneofOptions *msg, size_t len, upb_arena *arena) {
  return (google_protobuf_UninterpretedOption**)_upb_array_resize_accessor(msg, UPB_SIZE(0, 0), len, UPB_SIZE(4, 8), UPB_TYPE_MESSAGE, arena);
}
UPB_INLINE struct google_protobuf_UninterpretedOption* google_protobuf_OneofOptions_add_uninterpreted_option(google_protobuf_OneofOptions *msg, upb_arena *arena) {
  struct google_protobuf_UninterpretedOption* sub = (struct google_protobuf_UninterpretedOption*)upb_msg_new(&google_protobuf_UninterpretedOption_msginit, arena);
  bool ok = _upb_array_append_accessor(
      msg, UPB_SIZE(0, 0), UPB_SIZE(4, 8), UPB_TYPE_MESSAGE, &sub, arena);
  if (!ok) return NULL;
  return sub;
}


/* google.protobuf.EnumOptions */

UPB_INLINE google_protobuf_EnumOptions *google_protobuf_EnumOptions_new(upb_arena *arena) {
  return (google_protobuf_EnumOptions *)upb_msg_new(&google_protobuf_EnumOptions_msginit, arena);
}
UPB_INLINE google_protobuf_EnumOptions *google_protobuf_EnumOptions_parsenew(upb_strview buf, upb_arena *arena) {
  google_protobuf_EnumOptions *ret = google_protobuf_EnumOptions_new(arena);
  return (ret && upb_decode(buf, ret, &google_protobuf_EnumOptions_msginit)) ? ret : NULL;
}
UPB_INLINE char *google_protobuf_EnumOptions_serialize(const google_protobuf_EnumOptions *msg, upb_arena *arena, size_t *len) {
  return upb_encode(msg, &google_protobuf_EnumOptions_msginit, arena, len);
}

UPB_INLINE bool google_protobuf_EnumOptions_has_allow_alias(const google_protobuf_EnumOptions *msg) { return _upb_has_field(msg, 0); }
UPB_INLINE bool google_protobuf_EnumOptions_allow_alias(const google_protobuf_EnumOptions *msg) { return UPB_FIELD_AT(msg, bool, UPB_SIZE(1, 1)); }
UPB_INLINE bool google_protobuf_EnumOptions_has_deprecated(const google_protobuf_EnumOptions *msg) { return _upb_has_field(msg, 1); }
UPB_INLINE bool google_protobuf_EnumOptions_deprecated(const google_protobuf_EnumOptions *msg) { return UPB_FIELD_AT(msg, bool, UPB_SIZE(2, 2)); }
UPB_INLINE const google_protobuf_UninterpretedOption* const* google_protobuf_EnumOptions_uninterpreted_option(const google_protobuf_EnumOptions *msg, size_t *len) { return (const google_protobuf_UninterpretedOption* const*)_upb_array_accessor(msg, UPB_SIZE(4, 8), len); }

UPB_INLINE void google_protobuf_EnumOptions_set_allow_alias(google_protobuf_EnumOptions *msg, bool value) {
  _upb_sethas(msg, 0);
  UPB_FIELD_AT(msg, bool, UPB_SIZE(1, 1)) = value;
}
UPB_INLINE void google_protobuf_EnumOptions_set_deprecated(google_protobuf_EnumOptions *msg, bool value) {
  _upb_sethas(msg, 1);
  UPB_FIELD_AT(msg, bool, UPB_SIZE(2, 2)) = value;
}
UPB_INLINE google_protobuf_UninterpretedOption** google_protobuf_EnumOptions_mutable_uninterpreted_option(google_protobuf_EnumOptions *msg, size_t *len) {
  return (google_protobuf_UninterpretedOption**)_upb_array_mutable_accessor(msg, UPB_SIZE(4, 8), len);
}
UPB_INLINE google_protobuf_UninterpretedOption** google_protobuf_EnumOptions_resize_uninterpreted_option(google_protobuf_EnumOptions *msg, size_t len, upb_arena *arena) {
  return (google_protobuf_UninterpretedOption**)_upb_array_resize_accessor(msg, UPB_SIZE(4, 8), len, UPB_SIZE(4, 8), UPB_TYPE_MESSAGE, arena);
}
UPB_INLINE struct google_protobuf_UninterpretedOption* google_protobuf_EnumOptions_add_uninterpreted_option(google_protobuf_EnumOptions *msg, upb_arena *arena) {
  struct google_protobuf_UninterpretedOption* sub = (struct google_protobuf_UninterpretedOption*)upb_msg_new(&google_protobuf_UninterpretedOption_msginit, arena);
  bool ok = _upb_array_append_accessor(
      msg, UPB_SIZE(4, 8), UPB_SIZE(4, 8), UPB_TYPE_MESSAGE, &sub, arena);
  if (!ok) return NULL;
  return sub;
}


/* google.protobuf.EnumValueOptions */

UPB_INLINE google_protobuf_EnumValueOptions *google_protobuf_EnumValueOptions_new(upb_arena *arena) {
  return (google_protobuf_EnumValueOptions *)upb_msg_new(&google_protobuf_EnumValueOptions_msginit, arena);
}
UPB_INLINE google_protobuf_EnumValueOptions *google_protobuf_EnumValueOptions_parsenew(upb_strview buf, upb_arena *arena) {
  google_protobuf_EnumValueOptions *ret = google_protobuf_EnumValueOptions_new(arena);
  return (ret && upb_decode(buf, ret, &google_protobuf_EnumValueOptions_msginit)) ? ret : NULL;
}
UPB_INLINE char *google_protobuf_EnumValueOptions_serialize(const google_protobuf_EnumValueOptions *msg, upb_arena *arena, size_t *len) {
  return upb_encode(msg, &google_protobuf_EnumValueOptions_msginit, arena, len);
}

UPB_INLINE bool google_protobuf_EnumValueOptions_has_deprecated(const google_protobuf_EnumValueOptions *msg) { return _upb_has_field(msg, 0); }
UPB_INLINE bool google_protobuf_EnumValueOptions_deprecated(const google_protobuf_EnumValueOptions *msg) { return UPB_FIELD_AT(msg, bool, UPB_SIZE(1, 1)); }
UPB_INLINE const google_protobuf_UninterpretedOption* const* google_protobuf_EnumValueOptions_uninterpreted_option(const google_protobuf_EnumValueOptions *msg, size_t *len) { return (const google_protobuf_UninterpretedOption* const*)_upb_array_accessor(msg, UPB_SIZE(4, 8), len); }

UPB_INLINE void google_protobuf_EnumValueOptions_set_deprecated(google_protobuf_EnumValueOptions *msg, bool value) {
  _upb_sethas(msg, 0);
  UPB_FIELD_AT(msg, bool, UPB_SIZE(1, 1)) = value;
}
UPB_INLINE google_protobuf_UninterpretedOption** google_protobuf_EnumValueOptions_mutable_uninterpreted_option(google_protobuf_EnumValueOptions *msg, size_t *len) {
  return (google_protobuf_UninterpretedOption**)_upb_array_mutable_accessor(msg, UPB_SIZE(4, 8), len);
}
UPB_INLINE google_protobuf_UninterpretedOption** google_protobuf_EnumValueOptions_resize_uninterpreted_option(google_protobuf_EnumValueOptions *msg, size_t len, upb_arena *arena) {
  return (google_protobuf_UninterpretedOption**)_upb_array_resize_accessor(msg, UPB_SIZE(4, 8), len, UPB_SIZE(4, 8), UPB_TYPE_MESSAGE, arena);
}
UPB_INLINE struct google_protobuf_UninterpretedOption* google_protobuf_EnumValueOptions_add_uninterpreted_option(google_protobuf_EnumValueOptions *msg, upb_arena *arena) {
  struct google_protobuf_UninterpretedOption* sub = (struct google_protobuf_UninterpretedOption*)upb_msg_new(&google_protobuf_UninterpretedOption_msginit, arena);
  bool ok = _upb_array_append_accessor(
      msg, UPB_SIZE(4, 8), UPB_SIZE(4, 8), UPB_TYPE_MESSAGE, &sub, arena);
  if (!ok) return NULL;
  return sub;
}


/* google.protobuf.ServiceOptions */

UPB_INLINE google_protobuf_ServiceOptions *google_protobuf_ServiceOptions_new(upb_arena *arena) {
  return (google_protobuf_ServiceOptions *)upb_msg_new(&google_protobuf_ServiceOptions_msginit, arena);
}
UPB_INLINE google_protobuf_ServiceOptions *google_protobuf_ServiceOptions_parsenew(upb_strview buf, upb_arena *arena) {
  google_protobuf_ServiceOptions *ret = google_protobuf_ServiceOptions_new(arena);
  return (ret && upb_decode(buf, ret, &google_protobuf_ServiceOptions_msginit)) ? ret : NULL;
}
UPB_INLINE char *google_protobuf_ServiceOptions_serialize(const google_protobuf_ServiceOptions *msg, upb_arena *arena, size_t *len) {
  return upb_encode(msg, &google_protobuf_ServiceOptions_msginit, arena, len);
}

UPB_INLINE bool google_protobuf_ServiceOptions_has_deprecated(const google_protobuf_ServiceOptions *msg) { return _upb_has_field(msg, 0); }
UPB_INLINE bool google_protobuf_ServiceOptions_deprecated(const google_protobuf_ServiceOptions *msg) { return UPB_FIELD_AT(msg, bool, UPB_SIZE(1, 1)); }
UPB_INLINE const google_protobuf_UninterpretedOption* const* google_protobuf_ServiceOptions_uninterpreted_option(const google_protobuf_ServiceOptions *msg, size_t *len) { return (const google_protobuf_UninterpretedOption* const*)_upb_array_accessor(msg, UPB_SIZE(4, 8), len); }

UPB_INLINE void google_protobuf_ServiceOptions_set_deprecated(google_protobuf_ServiceOptions *msg, bool value) {
  _upb_sethas(msg, 0);
  UPB_FIELD_AT(msg, bool, UPB_SIZE(1, 1)) = value;
}
UPB_INLINE google_protobuf_UninterpretedOption** google_protobuf_ServiceOptions_mutable_uninterpreted_option(google_protobuf_ServiceOptions *msg, size_t *len) {
  return (google_protobuf_UninterpretedOption**)_upb_array_mutable_accessor(msg, UPB_SIZE(4, 8), len);
}
UPB_INLINE google_protobuf_UninterpretedOption** google_protobuf_ServiceOptions_resize_uninterpreted_option(google_protobuf_ServiceOptions *msg, size_t len, upb_arena *arena) {
  return (google_protobuf_UninterpretedOption**)_upb_array_resize_accessor(msg, UPB_SIZE(4, 8), len, UPB_SIZE(4, 8), UPB_TYPE_MESSAGE, arena);
}
UPB_INLINE struct google_protobuf_UninterpretedOption* google_protobuf_ServiceOptions_add_uninterpreted_option(google_protobuf_ServiceOptions *msg, upb_arena *arena) {
  struct google_protobuf_UninterpretedOption* sub = (struct google_protobuf_UninterpretedOption*)upb_msg_new(&google_protobuf_UninterpretedOption_msginit, arena);
  bool ok = _upb_array_append_accessor(
      msg, UPB_SIZE(4, 8), UPB_SIZE(4, 8), UPB_TYPE_MESSAGE, &sub, arena);
  if (!ok) return NULL;
  return sub;
}


/* google.protobuf.MethodOptions */

UPB_INLINE google_protobuf_MethodOptions *google_protobuf_MethodOptions_new(upb_arena *arena) {
  return (google_protobuf_MethodOptions *)upb_msg_new(&google_protobuf_MethodOptions_msginit, arena);
}
UPB_INLINE google_protobuf_MethodOptions *google_protobuf_MethodOptions_parsenew(upb_strview buf, upb_arena *arena) {
  google_protobuf_MethodOptions *ret = google_protobuf_MethodOptions_new(arena);
  return (ret && upb_decode(buf, ret, &google_protobuf_MethodOptions_msginit)) ? ret : NULL;
}
UPB_INLINE char *google_protobuf_MethodOptions_serialize(const google_protobuf_MethodOptions *msg, upb_arena *arena, size_t *len) {
  return upb_encode(msg, &google_protobuf_MethodOptions_msginit, arena, len);
}

UPB_INLINE bool google_protobuf_MethodOptions_has_deprecated(const google_protobuf_MethodOptions *msg) { return _upb_has_field(msg, 1); }
UPB_INLINE bool google_protobuf_MethodOptions_deprecated(const google_protobuf_MethodOptions *msg) { return UPB_FIELD_AT(msg, bool, UPB_SIZE(16, 16)); }
UPB_INLINE bool google_protobuf_MethodOptions_has_idempotency_level(const google_protobuf_MethodOptions *msg) { return _upb_has_field(msg, 0); }
UPB_INLINE google_protobuf_MethodOptions_IdempotencyLevel google_protobuf_MethodOptions_idempotency_level(const google_protobuf_MethodOptions *msg) { return UPB_FIELD_AT(msg, google_protobuf_MethodOptions_IdempotencyLevel, UPB_SIZE(8, 8)); }
UPB_INLINE const google_protobuf_UninterpretedOption* const* google_protobuf_MethodOptions_uninterpreted_option(const google_protobuf_MethodOptions *msg, size_t *len) { return (const google_protobuf_UninterpretedOption* const*)_upb_array_accessor(msg, UPB_SIZE(20, 24), len); }

UPB_INLINE void google_protobuf_MethodOptions_set_deprecated(google_protobuf_MethodOptions *msg, bool value) {
  _upb_sethas(msg, 1);
  UPB_FIELD_AT(msg, bool, UPB_SIZE(16, 16)) = value;
}
UPB_INLINE void google_protobuf_MethodOptions_set_idempotency_level(google_protobuf_MethodOptions *msg, google_protobuf_MethodOptions_IdempotencyLevel value) {
  _upb_sethas(msg, 0);
  UPB_FIELD_AT(msg, google_protobuf_MethodOptions_IdempotencyLevel, UPB_SIZE(8, 8)) = value;
}
UPB_INLINE google_protobuf_UninterpretedOption** google_protobuf_MethodOptions_mutable_uninterpreted_option(google_protobuf_MethodOptions *msg, size_t *len) {
  return (google_protobuf_UninterpretedOption**)_upb_array_mutable_accessor(msg, UPB_SIZE(20, 24), len);
}
UPB_INLINE google_protobuf_UninterpretedOption** google_protobuf_MethodOptions_resize_uninterpreted_option(google_protobuf_MethodOptions *msg, size_t len, upb_arena *arena) {
  return (google_protobuf_UninterpretedOption**)_upb_array_resize_accessor(msg, UPB_SIZE(20, 24), len, UPB_SIZE(4, 8), UPB_TYPE_MESSAGE, arena);
}
UPB_INLINE struct google_protobuf_UninterpretedOption* google_protobuf_MethodOptions_add_uninterpreted_option(google_protobuf_MethodOptions *msg, upb_arena *arena) {
  struct google_protobuf_UninterpretedOption* sub = (struct google_protobuf_UninterpretedOption*)upb_msg_new(&google_protobuf_UninterpretedOption_msginit, arena);
  bool ok = _upb_array_append_accessor(
      msg, UPB_SIZE(20, 24), UPB_SIZE(4, 8), UPB_TYPE_MESSAGE, &sub, arena);
  if (!ok) return NULL;
  return sub;
}


/* google.protobuf.UninterpretedOption */

UPB_INLINE google_protobuf_UninterpretedOption *google_protobuf_UninterpretedOption_new(upb_arena *arena) {
  return (google_protobuf_UninterpretedOption *)upb_msg_new(&google_protobuf_UninterpretedOption_msginit, arena);
}
UPB_INLINE google_protobuf_UninterpretedOption *google_protobuf_UninterpretedOption_parsenew(upb_strview buf, upb_arena *arena) {
  google_protobuf_UninterpretedOption *ret = google_protobuf_UninterpretedOption_new(arena);
  return (ret && upb_decode(buf, ret, &google_protobuf_UninterpretedOption_msginit)) ? ret : NULL;
}
UPB_INLINE char *google_protobuf_UninterpretedOption_serialize(const google_protobuf_UninterpretedOption *msg, upb_arena *arena, size_t *len) {
  return upb_encode(msg, &google_protobuf_UninterpretedOption_msginit, arena, len);
}

UPB_INLINE const google_protobuf_UninterpretedOption_NamePart* const* google_protobuf_UninterpretedOption_name(const google_protobuf_UninterpretedOption *msg, size_t *len) { return (const google_protobuf_UninterpretedOption_NamePart* const*)_upb_array_accessor(msg, UPB_SIZE(56, 80), len); }
UPB_INLINE bool google_protobuf_UninterpretedOption_has_identifier_value(const google_protobuf_UninterpretedOption *msg) { return _upb_has_field(msg, 3); }
UPB_INLINE upb_strview google_protobuf_UninterpretedOption_identifier_value(const google_protobuf_UninterpretedOption *msg) { return UPB_FIELD_AT(msg, upb_strview, UPB_SIZE(32, 32)); }
UPB_INLINE bool google_protobuf_UninterpretedOption_has_positive_int_value(const google_protobuf_UninterpretedOption *msg) { return _upb_has_field(msg, 0); }
UPB_INLINE uint64_t google_protobuf_UninterpretedOption_positive_int_value(const google_protobuf_UninterpretedOption *msg) { return UPB_FIELD_AT(msg, uint64_t, UPB_SIZE(8, 8)); }
UPB_INLINE bool google_protobuf_UninterpretedOption_has_negative_int_value(const google_protobuf_UninterpretedOption *msg) { return _upb_has_field(msg, 1); }
UPB_INLINE int64_t google_protobuf_UninterpretedOption_negative_int_value(const google_protobuf_UninterpretedOption *msg) { return UPB_FIELD_AT(msg, int64_t, UPB_SIZE(16, 16)); }
UPB_INLINE bool google_protobuf_UninterpretedOption_has_double_value(const google_protobuf_UninterpretedOption *msg) { return _upb_has_field(msg, 2); }
UPB_INLINE double google_protobuf_UninterpretedOption_double_value(const google_protobuf_UninterpretedOption *msg) { return UPB_FIELD_AT(msg, double, UPB_SIZE(24, 24)); }
UPB_INLINE bool google_protobuf_UninterpretedOption_has_string_value(const google_protobuf_UninterpretedOption *msg) { return _upb_has_field(msg, 4); }
UPB_INLINE upb_strview google_protobuf_UninterpretedOption_string_value(const google_protobuf_UninterpretedOption *msg) { return UPB_FIELD_AT(msg, upb_strview, UPB_SIZE(40, 48)); }
UPB_INLINE bool google_protobuf_UninterpretedOption_has_aggregate_value(const google_protobuf_UninterpretedOption *msg) { return _upb_has_field(msg, 5); }
UPB_INLINE upb_strview google_protobuf_UninterpretedOption_aggregate_value(const google_protobuf_UninterpretedOption *msg) { return UPB_FIELD_AT(msg, upb_strview, UPB_SIZE(48, 64)); }

UPB_INLINE google_protobuf_UninterpretedOption_NamePart** google_protobuf_UninterpretedOption_mutable_name(google_protobuf_UninterpretedOption *msg, size_t *len) {
  return (google_protobuf_UninterpretedOption_NamePart**)_upb_array_mutable_accessor(msg, UPB_SIZE(56, 80), len);
}
UPB_INLINE google_protobuf_UninterpretedOption_NamePart** google_protobuf_UninterpretedOption_resize_name(google_protobuf_UninterpretedOption *msg, size_t len, upb_arena *arena) {
  return (google_protobuf_UninterpretedOption_NamePart**)_upb_array_resize_accessor(msg, UPB_SIZE(56, 80), len, UPB_SIZE(4, 8), UPB_TYPE_MESSAGE, arena);
}
UPB_INLINE struct google_protobuf_UninterpretedOption_NamePart* google_protobuf_UninterpretedOption_add_name(google_protobuf_UninterpretedOption *msg, upb_arena *arena) {
  struct google_protobuf_UninterpretedOption_NamePart* sub = (struct google_protobuf_UninterpretedOption_NamePart*)upb_msg_new(&google_protobuf_UninterpretedOption_NamePart_msginit, arena);
  bool ok = _upb_array_append_accessor(
      msg, UPB_SIZE(56, 80), UPB_SIZE(4, 8), UPB_TYPE_MESSAGE, &sub, arena);
  if (!ok) return NULL;
  return sub;
}
UPB_INLINE void google_protobuf_UninterpretedOption_set_identifier_value(google_protobuf_UninterpretedOption *msg, upb_strview value) {
  _upb_sethas(msg, 3);
  UPB_FIELD_AT(msg, upb_strview, UPB_SIZE(32, 32)) = value;
}
UPB_INLINE void google_protobuf_UninterpretedOption_set_positive_int_value(google_protobuf_UninterpretedOption *msg, uint64_t value) {
  _upb_sethas(msg, 0);
  UPB_FIELD_AT(msg, uint64_t, UPB_SIZE(8, 8)) = value;
}
UPB_INLINE void google_protobuf_UninterpretedOption_set_negative_int_value(google_protobuf_UninterpretedOption *msg, int64_t value) {
  _upb_sethas(msg, 1);
  UPB_FIELD_AT(msg, int64_t, UPB_SIZE(16, 16)) = value;
}
UPB_INLINE void google_protobuf_UninterpretedOption_set_double_value(google_protobuf_UninterpretedOption *msg, double value) {
  _upb_sethas(msg, 2);
  UPB_FIELD_AT(msg, double, UPB_SIZE(24, 24)) = value;
}
UPB_INLINE void google_protobuf_UninterpretedOption_set_string_value(google_protobuf_UninterpretedOption *msg, upb_strview value) {
  _upb_sethas(msg, 4);
  UPB_FIELD_AT(msg, upb_strview, UPB_SIZE(40, 48)) = value;
}
UPB_INLINE void google_protobuf_UninterpretedOption_set_aggregate_value(google_protobuf_UninterpretedOption *msg, upb_strview value) {
  _upb_sethas(msg, 5);
  UPB_FIELD_AT(msg, upb_strview, UPB_SIZE(48, 64)) = value;
}


/* google.protobuf.UninterpretedOption.NamePart */

UPB_INLINE google_protobuf_UninterpretedOption_NamePart *google_protobuf_UninterpretedOption_NamePart_new(upb_arena *arena) {
  return (google_protobuf_UninterpretedOption_NamePart *)upb_msg_new(&google_protobuf_UninterpretedOption_NamePart_msginit, arena);
}
UPB_INLINE google_protobuf_UninterpretedOption_NamePart *google_protobuf_UninterpretedOption_NamePart_parsenew(upb_strview buf, upb_arena *arena) {
  google_protobuf_UninterpretedOption_NamePart *ret = google_protobuf_UninterpretedOption_NamePart_new(arena);
  return (ret && upb_decode(buf, ret, &google_protobuf_UninterpretedOption_NamePart_msginit)) ? ret : NULL;
}
UPB_INLINE char *google_protobuf_UninterpretedOption_NamePart_serialize(const google_protobuf_UninterpretedOption_NamePart *msg, upb_arena *arena, size_t *len) {
  return upb_encode(msg, &google_protobuf_UninterpretedOption_NamePart_msginit, arena, len);
}

UPB_INLINE bool google_protobuf_UninterpretedOption_NamePart_has_name_part(const google_protobuf_UninterpretedOption_NamePart *msg) { return _upb_has_field(msg, 1); }
UPB_INLINE upb_strview google_protobuf_UninterpretedOption_NamePart_name_part(const google_protobuf_UninterpretedOption_NamePart *msg) { return UPB_FIELD_AT(msg, upb_strview, UPB_SIZE(4, 8)); }
UPB_INLINE bool google_protobuf_UninterpretedOption_NamePart_has_is_extension(const google_protobuf_UninterpretedOption_NamePart *msg) { return _upb_has_field(msg, 0); }
UPB_INLINE bool google_protobuf_UninterpretedOption_NamePart_is_extension(const google_protobuf_UninterpretedOption_NamePart *msg) { return UPB_FIELD_AT(msg, bool, UPB_SIZE(1, 1)); }

UPB_INLINE void google_protobuf_UninterpretedOption_NamePart_set_name_part(google_protobuf_UninterpretedOption_NamePart *msg, upb_strview value) {
  _upb_sethas(msg, 1);
  UPB_FIELD_AT(msg, upb_strview, UPB_SIZE(4, 8)) = value;
}
UPB_INLINE void google_protobuf_UninterpretedOption_NamePart_set_is_extension(google_protobuf_UninterpretedOption_NamePart *msg, bool value) {
  _upb_sethas(msg, 0);
  UPB_FIELD_AT(msg, bool, UPB_SIZE(1, 1)) = value;
}


/* google.protobuf.SourceCodeInfo */

UPB_INLINE google_protobuf_SourceCodeInfo *google_protobuf_SourceCodeInfo_new(upb_arena *arena) {
  return (google_protobuf_SourceCodeInfo *)upb_msg_new(&google_protobuf_SourceCodeInfo_msginit, arena);
}
UPB_INLINE google_protobuf_SourceCodeInfo *google_protobuf_SourceCodeInfo_parsenew(upb_strview buf, upb_arena *arena) {
  google_protobuf_SourceCodeInfo *ret = google_protobuf_SourceCodeInfo_new(arena);
  return (ret && upb_decode(buf, ret, &google_protobuf_SourceCodeInfo_msginit)) ? ret : NULL;
}
UPB_INLINE char *google_protobuf_SourceCodeInfo_serialize(const google_protobuf_SourceCodeInfo *msg, upb_arena *arena, size_t *len) {
  return upb_encode(msg, &google_protobuf_SourceCodeInfo_msginit, arena, len);
}

UPB_INLINE const google_protobuf_SourceCodeInfo_Location* const* google_protobuf_SourceCodeInfo_location(const google_protobuf_SourceCodeInfo *msg, size_t *len) { return (const google_protobuf_SourceCodeInfo_Location* const*)_upb_array_accessor(msg, UPB_SIZE(0, 0), len); }

UPB_INLINE google_protobuf_SourceCodeInfo_Location** google_protobuf_SourceCodeInfo_mutable_location(google_protobuf_SourceCodeInfo *msg, size_t *len) {
  return (google_protobuf_SourceCodeInfo_Location**)_upb_array_mutable_accessor(msg, UPB_SIZE(0, 0), len);
}
UPB_INLINE google_protobuf_SourceCodeInfo_Location** google_protobuf_SourceCodeInfo_resize_location(google_protobuf_SourceCodeInfo *msg, size_t len, upb_arena *arena) {
  return (google_protobuf_SourceCodeInfo_Location**)_upb_array_resize_accessor(msg, UPB_SIZE(0, 0), len, UPB_SIZE(4, 8), UPB_TYPE_MESSAGE, arena);
}
UPB_INLINE struct google_protobuf_SourceCodeInfo_Location* google_protobuf_SourceCodeInfo_add_location(google_protobuf_SourceCodeInfo *msg, upb_arena *arena) {
  struct google_protobuf_SourceCodeInfo_Location* sub = (struct google_protobuf_SourceCodeInfo_Location*)upb_msg_new(&google_protobuf_SourceCodeInfo_Location_msginit, arena);
  bool ok = _upb_array_append_accessor(
      msg, UPB_SIZE(0, 0), UPB_SIZE(4, 8), UPB_TYPE_MESSAGE, &sub, arena);
  if (!ok) return NULL;
  return sub;
}


/* google.protobuf.SourceCodeInfo.Location */

UPB_INLINE google_protobuf_SourceCodeInfo_Location *google_protobuf_SourceCodeInfo_Location_new(upb_arena *arena) {
  return (google_protobuf_SourceCodeInfo_Location *)upb_msg_new(&google_protobuf_SourceCodeInfo_Location_msginit, arena);
}
UPB_INLINE google_protobuf_SourceCodeInfo_Location *google_protobuf_SourceCodeInfo_Location_parsenew(upb_strview buf, upb_arena *arena) {
  google_protobuf_SourceCodeInfo_Location *ret = google_protobuf_SourceCodeInfo_Location_new(arena);
  return (ret && upb_decode(buf, ret, &google_protobuf_SourceCodeInfo_Location_msginit)) ? ret : NULL;
}
UPB_INLINE char *google_protobuf_SourceCodeInfo_Location_serialize(const google_protobuf_SourceCodeInfo_Location *msg, upb_arena *arena, size_t *len) {
  return upb_encode(msg, &google_protobuf_SourceCodeInfo_Location_msginit, arena, len);
}

UPB_INLINE int32_t const* google_protobuf_SourceCodeInfo_Location_path(const google_protobuf_SourceCodeInfo_Location *msg, size_t *len) { return (int32_t const*)_upb_array_accessor(msg, UPB_SIZE(20, 40), len); }
UPB_INLINE int32_t const* google_protobuf_SourceCodeInfo_Location_span(const google_protobuf_SourceCodeInfo_Location *msg, size_t *len) { return (int32_t const*)_upb_array_accessor(msg, UPB_SIZE(24, 48), len); }
UPB_INLINE bool google_protobuf_SourceCodeInfo_Location_has_leading_comments(const google_protobuf_SourceCodeInfo_Location *msg) { return _upb_has_field(msg, 0); }
UPB_INLINE upb_strview google_protobuf_SourceCodeInfo_Location_leading_comments(const google_protobuf_SourceCodeInfo_Location *msg) { return UPB_FIELD_AT(msg, upb_strview, UPB_SIZE(4, 8)); }
UPB_INLINE bool google_protobuf_SourceCodeInfo_Location_has_trailing_comments(const google_protobuf_SourceCodeInfo_Location *msg) { return _upb_has_field(msg, 1); }
UPB_INLINE upb_strview google_protobuf_SourceCodeInfo_Location_trailing_comments(const google_protobuf_SourceCodeInfo_Location *msg) { return UPB_FIELD_AT(msg, upb_strview, UPB_SIZE(12, 24)); }
UPB_INLINE upb_strview const* google_protobuf_SourceCodeInfo_Location_leading_detached_comments(const google_protobuf_SourceCodeInfo_Location *msg, size_t *len) { return (upb_strview const*)_upb_array_accessor(msg, UPB_SIZE(28, 56), len); }

UPB_INLINE int32_t* google_protobuf_SourceCodeInfo_Location_mutable_path(google_protobuf_SourceCodeInfo_Location *msg, size_t *len) {
  return (int32_t*)_upb_array_mutable_accessor(msg, UPB_SIZE(20, 40), len);
}
UPB_INLINE int32_t* google_protobuf_SourceCodeInfo_Location_resize_path(google_protobuf_SourceCodeInfo_Location *msg, size_t len, upb_arena *arena) {
  return (int32_t*)_upb_array_resize_accessor(msg, UPB_SIZE(20, 40), len, UPB_SIZE(4, 4), UPB_TYPE_INT32, arena);
}
UPB_INLINE bool google_protobuf_SourceCodeInfo_Location_add_path(google_protobuf_SourceCodeInfo_Location *msg, int32_t val, upb_arena *arena) {
  return _upb_array_append_accessor(
      msg, UPB_SIZE(20, 40), UPB_SIZE(4, 4), UPB_TYPE_INT32, &val, arena);
}
UPB_INLINE int32_t* google_protobuf_SourceCodeInfo_Location_mutable_span(google_protobuf_SourceCodeInfo_Location *msg, size_t *len) {
  return (int32_t*)_upb_array_mutable_accessor(msg, UPB_SIZE(24, 48), len);
}
UPB_INLINE int32_t* google_protobuf_SourceCodeInfo_Location_resize_span(google_protobuf_SourceCodeInfo_Location *msg, size_t len, upb_arena *arena) {
  return (int32_t*)_upb_array_resize_accessor(msg, UPB_SIZE(24, 48), len, UPB_SIZE(4, 4), UPB_TYPE_INT32, arena);
}
UPB_INLINE bool google_protobuf_SourceCodeInfo_Location_add_span(google_protobuf_SourceCodeInfo_Location *msg, int32_t val, upb_arena *arena) {
  return _upb_array_append_accessor(
      msg, UPB_SIZE(24, 48), UPB_SIZE(4, 4), UPB_TYPE_INT32, &val, arena);
}
UPB_INLINE void google_protobuf_SourceCodeInfo_Location_set_leading_comments(google_protobuf_SourceCodeInfo_Location *msg, upb_strview value) {
  _upb_sethas(msg, 0);
  UPB_FIELD_AT(msg, upb_strview, UPB_SIZE(4, 8)) = value;
}
UPB_INLINE void google_protobuf_SourceCodeInfo_Location_set_trailing_comments(google_protobuf_SourceCodeInfo_Location *msg, upb_strview value) {
  _upb_sethas(msg, 1);
  UPB_FIELD_AT(msg, upb_strview, UPB_SIZE(12, 24)) = value;
}
UPB_INLINE upb_strview* google_protobuf_SourceCodeInfo_Location_mutable_leading_detached_comments(google_protobuf_SourceCodeInfo_Location *msg, size_t *len) {
  return (upb_strview*)_upb_array_mutable_accessor(msg, UPB_SIZE(28, 56), len);
}
UPB_INLINE upb_strview* google_protobuf_SourceCodeInfo_Location_resize_leading_detached_comments(google_protobuf_SourceCodeInfo_Location *msg, size_t len, upb_arena *arena) {
  return (upb_strview*)_upb_array_resize_accessor(msg, UPB_SIZE(28, 56), len, UPB_SIZE(8, 16), UPB_TYPE_STRING, arena);
}
UPB_INLINE bool google_protobuf_SourceCodeInfo_Location_add_leading_detached_comments(google_protobuf_SourceCodeInfo_Location *msg, upb_strview val, upb_arena *arena) {
  return _upb_array_append_accessor(
      msg, UPB_SIZE(28, 56), UPB_SIZE(8, 16), UPB_TYPE_STRING, &val, arena);
}


/* google.protobuf.GeneratedCodeInfo */

UPB_INLINE google_protobuf_GeneratedCodeInfo *google_protobuf_GeneratedCodeInfo_new(upb_arena *arena) {
  return (google_protobuf_GeneratedCodeInfo *)upb_msg_new(&google_protobuf_GeneratedCodeInfo_msginit, arena);
}
UPB_INLINE google_protobuf_GeneratedCodeInfo *google_protobuf_GeneratedCodeInfo_parsenew(upb_strview buf, upb_arena *arena) {
  google_protobuf_GeneratedCodeInfo *ret = google_protobuf_GeneratedCodeInfo_new(arena);
  return (ret && upb_decode(buf, ret, &google_protobuf_GeneratedCodeInfo_msginit)) ? ret : NULL;
}
UPB_INLINE char *google_protobuf_GeneratedCodeInfo_serialize(const google_protobuf_GeneratedCodeInfo *msg, upb_arena *arena, size_t *len) {
  return upb_encode(msg, &google_protobuf_GeneratedCodeInfo_msginit, arena, len);
}

UPB_INLINE const google_protobuf_GeneratedCodeInfo_Annotation* const* google_protobuf_GeneratedCodeInfo_annotation(const google_protobuf_GeneratedCodeInfo *msg, size_t *len) { return (const google_protobuf_GeneratedCodeInfo_Annotation* const*)_upb_array_accessor(msg, UPB_SIZE(0, 0), len); }

UPB_INLINE google_protobuf_GeneratedCodeInfo_Annotation** google_protobuf_GeneratedCodeInfo_mutable_annotation(google_protobuf_GeneratedCodeInfo *msg, size_t *len) {
  return (google_protobuf_GeneratedCodeInfo_Annotation**)_upb_array_mutable_accessor(msg, UPB_SIZE(0, 0), len);
}
UPB_INLINE google_protobuf_GeneratedCodeInfo_Annotation** google_protobuf_GeneratedCodeInfo_resize_annotation(google_protobuf_GeneratedCodeInfo *msg, size_t len, upb_arena *arena) {
  return (google_protobuf_GeneratedCodeInfo_Annotation**)_upb_array_resize_accessor(msg, UPB_SIZE(0, 0), len, UPB_SIZE(4, 8), UPB_TYPE_MESSAGE, arena);
}
UPB_INLINE struct google_protobuf_GeneratedCodeInfo_Annotation* google_protobuf_GeneratedCodeInfo_add_annotation(google_protobuf_GeneratedCodeInfo *msg, upb_arena *arena) {
  struct google_protobuf_GeneratedCodeInfo_Annotation* sub = (struct google_protobuf_GeneratedCodeInfo_Annotation*)upb_msg_new(&google_protobuf_GeneratedCodeInfo_Annotation_msginit, arena);
  bool ok = _upb_array_append_accessor(
      msg, UPB_SIZE(0, 0), UPB_SIZE(4, 8), UPB_TYPE_MESSAGE, &sub, arena);
  if (!ok) return NULL;
  return sub;
}


/* google.protobuf.GeneratedCodeInfo.Annotation */

UPB_INLINE google_protobuf_GeneratedCodeInfo_Annotation *google_protobuf_GeneratedCodeInfo_Annotation_new(upb_arena *arena) {
  return (google_protobuf_GeneratedCodeInfo_Annotation *)upb_msg_new(&google_protobuf_GeneratedCodeInfo_Annotation_msginit, arena);
}
UPB_INLINE google_protobuf_GeneratedCodeInfo_Annotation *google_protobuf_GeneratedCodeInfo_Annotation_parsenew(upb_strview buf, upb_arena *arena) {
  google_protobuf_GeneratedCodeInfo_Annotation *ret = google_protobuf_GeneratedCodeInfo_Annotation_new(arena);
  return (ret && upb_decode(buf, ret, &google_protobuf_GeneratedCodeInfo_Annotation_msginit)) ? ret : NULL;
}
UPB_INLINE char *google_protobuf_GeneratedCodeInfo_Annotation_serialize(const google_protobuf_GeneratedCodeInfo_Annotation *msg, upb_arena *arena, size_t *len) {
  return upb_encode(msg, &google_protobuf_GeneratedCodeInfo_Annotation_msginit, arena, len);
}

UPB_INLINE int32_t const* google_protobuf_GeneratedCodeInfo_Annotation_path(const google_protobuf_GeneratedCodeInfo_Annotation *msg, size_t *len) { return (int32_t const*)_upb_array_accessor(msg, UPB_SIZE(20, 32), len); }
UPB_INLINE bool google_protobuf_GeneratedCodeInfo_Annotation_has_source_file(const google_protobuf_GeneratedCodeInfo_Annotation *msg) { return _upb_has_field(msg, 2); }
UPB_INLINE upb_strview google_protobuf_GeneratedCodeInfo_Annotation_source_file(const google_protobuf_GeneratedCodeInfo_Annotation *msg) { return UPB_FIELD_AT(msg, upb_strview, UPB_SIZE(12, 16)); }
UPB_INLINE bool google_protobuf_GeneratedCodeInfo_Annotation_has_begin(const google_protobuf_GeneratedCodeInfo_Annotation *msg) { return _upb_has_field(msg, 0); }
UPB_INLINE int32_t google_protobuf_GeneratedCodeInfo_Annotation_begin(const google_protobuf_GeneratedCodeInfo_Annotation *msg) { return UPB_FIELD_AT(msg, int32_t, UPB_SIZE(4, 4)); }
UPB_INLINE bool google_protobuf_GeneratedCodeInfo_Annotation_has_end(const google_protobuf_GeneratedCodeInfo_Annotation *msg) { return _upb_has_field(msg, 1); }
UPB_INLINE int32_t google_protobuf_GeneratedCodeInfo_Annotation_end(const google_protobuf_GeneratedCodeInfo_Annotation *msg) { return UPB_FIELD_AT(msg, int32_t, UPB_SIZE(8, 8)); }

UPB_INLINE int32_t* google_protobuf_GeneratedCodeInfo_Annotation_mutable_path(google_protobuf_GeneratedCodeInfo_Annotation *msg, size_t *len) {
  return (int32_t*)_upb_array_mutable_accessor(msg, UPB_SIZE(20, 32), len);
}
UPB_INLINE int32_t* google_protobuf_GeneratedCodeInfo_Annotation_resize_path(google_protobuf_GeneratedCodeInfo_Annotation *msg, size_t len, upb_arena *arena) {
  return (int32_t*)_upb_array_resize_accessor(msg, UPB_SIZE(20, 32), len, UPB_SIZE(4, 4), UPB_TYPE_INT32, arena);
}
UPB_INLINE bool google_protobuf_GeneratedCodeInfo_Annotation_add_path(google_protobuf_GeneratedCodeInfo_Annotation *msg, int32_t val, upb_arena *arena) {
  return _upb_array_append_accessor(
      msg, UPB_SIZE(20, 32), UPB_SIZE(4, 4), UPB_TYPE_INT32, &val, arena);
}
UPB_INLINE void google_protobuf_GeneratedCodeInfo_Annotation_set_source_file(google_protobuf_GeneratedCodeInfo_Annotation *msg, upb_strview value) {
  _upb_sethas(msg, 2);
  UPB_FIELD_AT(msg, upb_strview, UPB_SIZE(12, 16)) = value;
}
UPB_INLINE void google_protobuf_GeneratedCodeInfo_Annotation_set_begin(google_protobuf_GeneratedCodeInfo_Annotation *msg, int32_t value) {
  _upb_sethas(msg, 0);
  UPB_FIELD_AT(msg, int32_t, UPB_SIZE(4, 4)) = value;
}
UPB_INLINE void google_protobuf_GeneratedCodeInfo_Annotation_set_end(google_protobuf_GeneratedCodeInfo_Annotation *msg, int32_t value) {
  _upb_sethas(msg, 1);
  UPB_FIELD_AT(msg, int32_t, UPB_SIZE(8, 8)) = value;
}


UPB_END_EXTERN_C


#endif  /* GOOGLE_PROTOBUF_DESCRIPTOR_PROTO_UPB_H_ */


#ifndef UPB_MSGFACTORY_H_
#define UPB_MSGFACTORY_H_

#ifdef __cplusplus
namespace upb {
class MessageFactory;
}
#endif

UPB_DECLARE_TYPE(upb::MessageFactory, upb_msgfactory)

/** upb_msgfactory ************************************************************/

/* A upb_msgfactory contains a cache of upb_msglayout, upb_handlers, and
 * upb_visitorplan objects.  These are the objects necessary to represent,
 * populate, and and visit upb_msg objects.
 *
 * These caches are all populated by upb_msgdef, and lazily created on demand.
 */

/* Creates and destroys a msgfactory, respectively.  The messages for this
 * msgfactory must come from |symtab| (which should outlive the msgfactory). */
upb_msgfactory *upb_msgfactory_new(const upb_symtab *symtab);
void upb_msgfactory_free(upb_msgfactory *f);

const upb_symtab *upb_msgfactory_symtab(const upb_msgfactory *f);

/* The functions to get cached objects, lazily creating them on demand.  These
 * all require:
 *
 * - m is in upb_msgfactory_symtab(f)
 * - upb_msgdef_mapentry(m) == false (since map messages can't have layouts).
 *
 * The returned objects will live for as long as the msgfactory does.
 *
 * TODO(haberman): consider making this thread-safe and take a const
 * upb_msgfactory. */
const upb_msglayout *upb_msgfactory_getlayout(upb_msgfactory *f,
                                              const upb_msgdef *m);


#endif /* UPB_MSGFACTORY_H_ */
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

  /* Use to get the FileDefs that have been parsed. */
  size_t file_count() const;
  FileDef* file(size_t i) const;

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
size_t upb_descreader_filecount(const upb_descreader *r);
upb_filedef *upb_descreader_file(const upb_descreader *r, size_t i);
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
inline size_t Reader::file_count() const {
  return upb_descreader_filecount(this);
}
inline FileDef* Reader::file(size_t i) const {
  return upb_descreader_file(this, i);
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
/* This file was generated by upbc (the upb compiler) from the input
 * file:
 *
 *     upb/descriptor/descriptor.proto
 *
 * Do not edit -- your changes will be discarded when the file is
 * regenerated. */

#ifndef UPB_DESCRIPTOR_DESCRIPTOR_PROTO_UPB_H_
#define UPB_DESCRIPTOR_DESCRIPTOR_PROTO_UPB_H_


UPB_BEGIN_EXTERN_C

/* MessageDefs: call these functions to get a ref to a msgdef. */
const upb_msgdef *upbdefs_google_protobuf_DescriptorProto_get(const void *owner);
const upb_msgdef *upbdefs_google_protobuf_DescriptorProto_ExtensionRange_get(const void *owner);
const upb_msgdef *upbdefs_google_protobuf_DescriptorProto_ReservedRange_get(const void *owner);
const upb_msgdef *upbdefs_google_protobuf_EnumDescriptorProto_get(const void *owner);
const upb_msgdef *upbdefs_google_protobuf_EnumOptions_get(const void *owner);
const upb_msgdef *upbdefs_google_protobuf_EnumValueDescriptorProto_get(const void *owner);
const upb_msgdef *upbdefs_google_protobuf_EnumValueOptions_get(const void *owner);
const upb_msgdef *upbdefs_google_protobuf_FieldDescriptorProto_get(const void *owner);
const upb_msgdef *upbdefs_google_protobuf_FieldOptions_get(const void *owner);
const upb_msgdef *upbdefs_google_protobuf_FileDescriptorProto_get(const void *owner);
const upb_msgdef *upbdefs_google_protobuf_FileDescriptorSet_get(const void *owner);
const upb_msgdef *upbdefs_google_protobuf_FileOptions_get(const void *owner);
const upb_msgdef *upbdefs_google_protobuf_MessageOptions_get(const void *owner);
const upb_msgdef *upbdefs_google_protobuf_MethodDescriptorProto_get(const void *owner);
const upb_msgdef *upbdefs_google_protobuf_MethodOptions_get(const void *owner);
const upb_msgdef *upbdefs_google_protobuf_OneofDescriptorProto_get(const void *owner);
const upb_msgdef *upbdefs_google_protobuf_ServiceDescriptorProto_get(const void *owner);
const upb_msgdef *upbdefs_google_protobuf_ServiceOptions_get(const void *owner);
const upb_msgdef *upbdefs_google_protobuf_SourceCodeInfo_get(const void *owner);
const upb_msgdef *upbdefs_google_protobuf_SourceCodeInfo_Location_get(const void *owner);
const upb_msgdef *upbdefs_google_protobuf_UninterpretedOption_get(const void *owner);
const upb_msgdef *upbdefs_google_protobuf_UninterpretedOption_NamePart_get(const void *owner);

/* EnumDefs: call these functions to get a ref to an enumdef. */
const upb_enumdef *upbdefs_google_protobuf_FieldDescriptorProto_Label_get(const void *owner);
const upb_enumdef *upbdefs_google_protobuf_FieldDescriptorProto_Type_get(const void *owner);
const upb_enumdef *upbdefs_google_protobuf_FieldOptions_CType_get(const void *owner);
const upb_enumdef *upbdefs_google_protobuf_FieldOptions_JSType_get(const void *owner);
const upb_enumdef *upbdefs_google_protobuf_FileOptions_OptimizeMode_get(const void *owner);

/* Functions to test whether this message is of a certain type. */
UPB_INLINE bool upbdefs_google_protobuf_DescriptorProto_is(const upb_msgdef *m) {
  return strcmp(upb_msgdef_fullname(m), "google.protobuf.DescriptorProto") == 0;
}
UPB_INLINE bool upbdefs_google_protobuf_DescriptorProto_ExtensionRange_is(const upb_msgdef *m) {
  return strcmp(upb_msgdef_fullname(m), "google.protobuf.DescriptorProto.ExtensionRange") == 0;
}
UPB_INLINE bool upbdefs_google_protobuf_DescriptorProto_ReservedRange_is(const upb_msgdef *m) {
  return strcmp(upb_msgdef_fullname(m), "google.protobuf.DescriptorProto.ReservedRange") == 0;
}
UPB_INLINE bool upbdefs_google_protobuf_EnumDescriptorProto_is(const upb_msgdef *m) {
  return strcmp(upb_msgdef_fullname(m), "google.protobuf.EnumDescriptorProto") == 0;
}
UPB_INLINE bool upbdefs_google_protobuf_EnumOptions_is(const upb_msgdef *m) {
  return strcmp(upb_msgdef_fullname(m), "google.protobuf.EnumOptions") == 0;
}
UPB_INLINE bool upbdefs_google_protobuf_EnumValueDescriptorProto_is(const upb_msgdef *m) {
  return strcmp(upb_msgdef_fullname(m), "google.protobuf.EnumValueDescriptorProto") == 0;
}
UPB_INLINE bool upbdefs_google_protobuf_EnumValueOptions_is(const upb_msgdef *m) {
  return strcmp(upb_msgdef_fullname(m), "google.protobuf.EnumValueOptions") == 0;
}
UPB_INLINE bool upbdefs_google_protobuf_FieldDescriptorProto_is(const upb_msgdef *m) {
  return strcmp(upb_msgdef_fullname(m), "google.protobuf.FieldDescriptorProto") == 0;
}
UPB_INLINE bool upbdefs_google_protobuf_FieldOptions_is(const upb_msgdef *m) {
  return strcmp(upb_msgdef_fullname(m), "google.protobuf.FieldOptions") == 0;
}
UPB_INLINE bool upbdefs_google_protobuf_FileDescriptorProto_is(const upb_msgdef *m) {
  return strcmp(upb_msgdef_fullname(m), "google.protobuf.FileDescriptorProto") == 0;
}
UPB_INLINE bool upbdefs_google_protobuf_FileDescriptorSet_is(const upb_msgdef *m) {
  return strcmp(upb_msgdef_fullname(m), "google.protobuf.FileDescriptorSet") == 0;
}
UPB_INLINE bool upbdefs_google_protobuf_FileOptions_is(const upb_msgdef *m) {
  return strcmp(upb_msgdef_fullname(m), "google.protobuf.FileOptions") == 0;
}
UPB_INLINE bool upbdefs_google_protobuf_MessageOptions_is(const upb_msgdef *m) {
  return strcmp(upb_msgdef_fullname(m), "google.protobuf.MessageOptions") == 0;
}
UPB_INLINE bool upbdefs_google_protobuf_MethodDescriptorProto_is(const upb_msgdef *m) {
  return strcmp(upb_msgdef_fullname(m), "google.protobuf.MethodDescriptorProto") == 0;
}
UPB_INLINE bool upbdefs_google_protobuf_MethodOptions_is(const upb_msgdef *m) {
  return strcmp(upb_msgdef_fullname(m), "google.protobuf.MethodOptions") == 0;
}
UPB_INLINE bool upbdefs_google_protobuf_OneofDescriptorProto_is(const upb_msgdef *m) {
  return strcmp(upb_msgdef_fullname(m), "google.protobuf.OneofDescriptorProto") == 0;
}
UPB_INLINE bool upbdefs_google_protobuf_ServiceDescriptorProto_is(const upb_msgdef *m) {
  return strcmp(upb_msgdef_fullname(m), "google.protobuf.ServiceDescriptorProto") == 0;
}
UPB_INLINE bool upbdefs_google_protobuf_ServiceOptions_is(const upb_msgdef *m) {
  return strcmp(upb_msgdef_fullname(m), "google.protobuf.ServiceOptions") == 0;
}
UPB_INLINE bool upbdefs_google_protobuf_SourceCodeInfo_is(const upb_msgdef *m) {
  return strcmp(upb_msgdef_fullname(m), "google.protobuf.SourceCodeInfo") == 0;
}
UPB_INLINE bool upbdefs_google_protobuf_SourceCodeInfo_Location_is(const upb_msgdef *m) {
  return strcmp(upb_msgdef_fullname(m), "google.protobuf.SourceCodeInfo.Location") == 0;
}
UPB_INLINE bool upbdefs_google_protobuf_UninterpretedOption_is(const upb_msgdef *m) {
  return strcmp(upb_msgdef_fullname(m), "google.protobuf.UninterpretedOption") == 0;
}
UPB_INLINE bool upbdefs_google_protobuf_UninterpretedOption_NamePart_is(const upb_msgdef *m) {
  return strcmp(upb_msgdef_fullname(m), "google.protobuf.UninterpretedOption.NamePart") == 0;
}

/* Functions to test whether this enum is of a certain type. */
UPB_INLINE bool upbdefs_google_protobuf_FieldDescriptorProto_Label_is(const upb_enumdef *e) {
  return strcmp(upb_enumdef_fullname(e), "google.protobuf.FieldDescriptorProto.Label") == 0;
}
UPB_INLINE bool upbdefs_google_protobuf_FieldDescriptorProto_Type_is(const upb_enumdef *e) {
  return strcmp(upb_enumdef_fullname(e), "google.protobuf.FieldDescriptorProto.Type") == 0;
}
UPB_INLINE bool upbdefs_google_protobuf_FieldOptions_CType_is(const upb_enumdef *e) {
  return strcmp(upb_enumdef_fullname(e), "google.protobuf.FieldOptions.CType") == 0;
}
UPB_INLINE bool upbdefs_google_protobuf_FieldOptions_JSType_is(const upb_enumdef *e) {
  return strcmp(upb_enumdef_fullname(e), "google.protobuf.FieldOptions.JSType") == 0;
}
UPB_INLINE bool upbdefs_google_protobuf_FileOptions_OptimizeMode_is(const upb_enumdef *e) {
  return strcmp(upb_enumdef_fullname(e), "google.protobuf.FileOptions.OptimizeMode") == 0;
}


/* Functions to get a fielddef from a msgdef reference. */
UPB_INLINE const upb_fielddef *upbdefs_google_protobuf_DescriptorProto_ExtensionRange_f_end(const upb_msgdef *m) { UPB_ASSERT(upbdefs_google_protobuf_DescriptorProto_ExtensionRange_is(m)); return upb_msgdef_itof(m, 2); }
UPB_INLINE const upb_fielddef *upbdefs_google_protobuf_DescriptorProto_ExtensionRange_f_start(const upb_msgdef *m) { UPB_ASSERT(upbdefs_google_protobuf_DescriptorProto_ExtensionRange_is(m)); return upb_msgdef_itof(m, 1); }
UPB_INLINE const upb_fielddef *upbdefs_google_protobuf_DescriptorProto_ReservedRange_f_end(const upb_msgdef *m) { UPB_ASSERT(upbdefs_google_protobuf_DescriptorProto_ReservedRange_is(m)); return upb_msgdef_itof(m, 2); }
UPB_INLINE const upb_fielddef *upbdefs_google_protobuf_DescriptorProto_ReservedRange_f_start(const upb_msgdef *m) { UPB_ASSERT(upbdefs_google_protobuf_DescriptorProto_ReservedRange_is(m)); return upb_msgdef_itof(m, 1); }
UPB_INLINE const upb_fielddef *upbdefs_google_protobuf_DescriptorProto_f_enum_type(const upb_msgdef *m) { UPB_ASSERT(upbdefs_google_protobuf_DescriptorProto_is(m)); return upb_msgdef_itof(m, 4); }
UPB_INLINE const upb_fielddef *upbdefs_google_protobuf_DescriptorProto_f_extension(const upb_msgdef *m) { UPB_ASSERT(upbdefs_google_protobuf_DescriptorProto_is(m)); return upb_msgdef_itof(m, 6); }
UPB_INLINE const upb_fielddef *upbdefs_google_protobuf_DescriptorProto_f_extension_range(const upb_msgdef *m) { UPB_ASSERT(upbdefs_google_protobuf_DescriptorProto_is(m)); return upb_msgdef_itof(m, 5); }
UPB_INLINE const upb_fielddef *upbdefs_google_protobuf_DescriptorProto_f_field(const upb_msgdef *m) { UPB_ASSERT(upbdefs_google_protobuf_DescriptorProto_is(m)); return upb_msgdef_itof(m, 2); }
UPB_INLINE const upb_fielddef *upbdefs_google_protobuf_DescriptorProto_f_name(const upb_msgdef *m) { UPB_ASSERT(upbdefs_google_protobuf_DescriptorProto_is(m)); return upb_msgdef_itof(m, 1); }
UPB_INLINE const upb_fielddef *upbdefs_google_protobuf_DescriptorProto_f_nested_type(const upb_msgdef *m) { UPB_ASSERT(upbdefs_google_protobuf_DescriptorProto_is(m)); return upb_msgdef_itof(m, 3); }
UPB_INLINE const upb_fielddef *upbdefs_google_protobuf_DescriptorProto_f_oneof_decl(const upb_msgdef *m) { UPB_ASSERT(upbdefs_google_protobuf_DescriptorProto_is(m)); return upb_msgdef_itof(m, 8); }
UPB_INLINE const upb_fielddef *upbdefs_google_protobuf_DescriptorProto_f_options(const upb_msgdef *m) { UPB_ASSERT(upbdefs_google_protobuf_DescriptorProto_is(m)); return upb_msgdef_itof(m, 7); }
UPB_INLINE const upb_fielddef *upbdefs_google_protobuf_DescriptorProto_f_reserved_name(const upb_msgdef *m) { UPB_ASSERT(upbdefs_google_protobuf_DescriptorProto_is(m)); return upb_msgdef_itof(m, 10); }
UPB_INLINE const upb_fielddef *upbdefs_google_protobuf_DescriptorProto_f_reserved_range(const upb_msgdef *m) { UPB_ASSERT(upbdefs_google_protobuf_DescriptorProto_is(m)); return upb_msgdef_itof(m, 9); }
UPB_INLINE const upb_fielddef *upbdefs_google_protobuf_EnumDescriptorProto_f_name(const upb_msgdef *m) { UPB_ASSERT(upbdefs_google_protobuf_EnumDescriptorProto_is(m)); return upb_msgdef_itof(m, 1); }
UPB_INLINE const upb_fielddef *upbdefs_google_protobuf_EnumDescriptorProto_f_options(const upb_msgdef *m) { UPB_ASSERT(upbdefs_google_protobuf_EnumDescriptorProto_is(m)); return upb_msgdef_itof(m, 3); }
UPB_INLINE const upb_fielddef *upbdefs_google_protobuf_EnumDescriptorProto_f_value(const upb_msgdef *m) { UPB_ASSERT(upbdefs_google_protobuf_EnumDescriptorProto_is(m)); return upb_msgdef_itof(m, 2); }
UPB_INLINE const upb_fielddef *upbdefs_google_protobuf_EnumOptions_f_allow_alias(const upb_msgdef *m) { UPB_ASSERT(upbdefs_google_protobuf_EnumOptions_is(m)); return upb_msgdef_itof(m, 2); }
UPB_INLINE const upb_fielddef *upbdefs_google_protobuf_EnumOptions_f_deprecated(const upb_msgdef *m) { UPB_ASSERT(upbdefs_google_protobuf_EnumOptions_is(m)); return upb_msgdef_itof(m, 3); }
UPB_INLINE const upb_fielddef *upbdefs_google_protobuf_EnumOptions_f_uninterpreted_option(const upb_msgdef *m) { UPB_ASSERT(upbdefs_google_protobuf_EnumOptions_is(m)); return upb_msgdef_itof(m, 999); }
UPB_INLINE const upb_fielddef *upbdefs_google_protobuf_EnumValueDescriptorProto_f_name(const upb_msgdef *m) { UPB_ASSERT(upbdefs_google_protobuf_EnumValueDescriptorProto_is(m)); return upb_msgdef_itof(m, 1); }
UPB_INLINE const upb_fielddef *upbdefs_google_protobuf_EnumValueDescriptorProto_f_number(const upb_msgdef *m) { UPB_ASSERT(upbdefs_google_protobuf_EnumValueDescriptorProto_is(m)); return upb_msgdef_itof(m, 2); }
UPB_INLINE const upb_fielddef *upbdefs_google_protobuf_EnumValueDescriptorProto_f_options(const upb_msgdef *m) { UPB_ASSERT(upbdefs_google_protobuf_EnumValueDescriptorProto_is(m)); return upb_msgdef_itof(m, 3); }
UPB_INLINE const upb_fielddef *upbdefs_google_protobuf_EnumValueOptions_f_deprecated(const upb_msgdef *m) { UPB_ASSERT(upbdefs_google_protobuf_EnumValueOptions_is(m)); return upb_msgdef_itof(m, 1); }
UPB_INLINE const upb_fielddef *upbdefs_google_protobuf_EnumValueOptions_f_uninterpreted_option(const upb_msgdef *m) { UPB_ASSERT(upbdefs_google_protobuf_EnumValueOptions_is(m)); return upb_msgdef_itof(m, 999); }
UPB_INLINE const upb_fielddef *upbdefs_google_protobuf_FieldDescriptorProto_f_default_value(const upb_msgdef *m) { UPB_ASSERT(upbdefs_google_protobuf_FieldDescriptorProto_is(m)); return upb_msgdef_itof(m, 7); }
UPB_INLINE const upb_fielddef *upbdefs_google_protobuf_FieldDescriptorProto_f_extendee(const upb_msgdef *m) { UPB_ASSERT(upbdefs_google_protobuf_FieldDescriptorProto_is(m)); return upb_msgdef_itof(m, 2); }
UPB_INLINE const upb_fielddef *upbdefs_google_protobuf_FieldDescriptorProto_f_json_name(const upb_msgdef *m) { UPB_ASSERT(upbdefs_google_protobuf_FieldDescriptorProto_is(m)); return upb_msgdef_itof(m, 10); }
UPB_INLINE const upb_fielddef *upbdefs_google_protobuf_FieldDescriptorProto_f_label(const upb_msgdef *m) { UPB_ASSERT(upbdefs_google_protobuf_FieldDescriptorProto_is(m)); return upb_msgdef_itof(m, 4); }
UPB_INLINE const upb_fielddef *upbdefs_google_protobuf_FieldDescriptorProto_f_name(const upb_msgdef *m) { UPB_ASSERT(upbdefs_google_protobuf_FieldDescriptorProto_is(m)); return upb_msgdef_itof(m, 1); }
UPB_INLINE const upb_fielddef *upbdefs_google_protobuf_FieldDescriptorProto_f_number(const upb_msgdef *m) { UPB_ASSERT(upbdefs_google_protobuf_FieldDescriptorProto_is(m)); return upb_msgdef_itof(m, 3); }
UPB_INLINE const upb_fielddef *upbdefs_google_protobuf_FieldDescriptorProto_f_oneof_index(const upb_msgdef *m) { UPB_ASSERT(upbdefs_google_protobuf_FieldDescriptorProto_is(m)); return upb_msgdef_itof(m, 9); }
UPB_INLINE const upb_fielddef *upbdefs_google_protobuf_FieldDescriptorProto_f_options(const upb_msgdef *m) { UPB_ASSERT(upbdefs_google_protobuf_FieldDescriptorProto_is(m)); return upb_msgdef_itof(m, 8); }
UPB_INLINE const upb_fielddef *upbdefs_google_protobuf_FieldDescriptorProto_f_type(const upb_msgdef *m) { UPB_ASSERT(upbdefs_google_protobuf_FieldDescriptorProto_is(m)); return upb_msgdef_itof(m, 5); }
UPB_INLINE const upb_fielddef *upbdefs_google_protobuf_FieldDescriptorProto_f_type_name(const upb_msgdef *m) { UPB_ASSERT(upbdefs_google_protobuf_FieldDescriptorProto_is(m)); return upb_msgdef_itof(m, 6); }
UPB_INLINE const upb_fielddef *upbdefs_google_protobuf_FieldOptions_f_ctype(const upb_msgdef *m) { UPB_ASSERT(upbdefs_google_protobuf_FieldOptions_is(m)); return upb_msgdef_itof(m, 1); }
UPB_INLINE const upb_fielddef *upbdefs_google_protobuf_FieldOptions_f_deprecated(const upb_msgdef *m) { UPB_ASSERT(upbdefs_google_protobuf_FieldOptions_is(m)); return upb_msgdef_itof(m, 3); }
UPB_INLINE const upb_fielddef *upbdefs_google_protobuf_FieldOptions_f_jstype(const upb_msgdef *m) { UPB_ASSERT(upbdefs_google_protobuf_FieldOptions_is(m)); return upb_msgdef_itof(m, 6); }
UPB_INLINE const upb_fielddef *upbdefs_google_protobuf_FieldOptions_f_lazy(const upb_msgdef *m) { UPB_ASSERT(upbdefs_google_protobuf_FieldOptions_is(m)); return upb_msgdef_itof(m, 5); }
UPB_INLINE const upb_fielddef *upbdefs_google_protobuf_FieldOptions_f_packed(const upb_msgdef *m) { UPB_ASSERT(upbdefs_google_protobuf_FieldOptions_is(m)); return upb_msgdef_itof(m, 2); }
UPB_INLINE const upb_fielddef *upbdefs_google_protobuf_FieldOptions_f_uninterpreted_option(const upb_msgdef *m) { UPB_ASSERT(upbdefs_google_protobuf_FieldOptions_is(m)); return upb_msgdef_itof(m, 999); }
UPB_INLINE const upb_fielddef *upbdefs_google_protobuf_FieldOptions_f_weak(const upb_msgdef *m) { UPB_ASSERT(upbdefs_google_protobuf_FieldOptions_is(m)); return upb_msgdef_itof(m, 10); }
UPB_INLINE const upb_fielddef *upbdefs_google_protobuf_FileDescriptorProto_f_dependency(const upb_msgdef *m) { UPB_ASSERT(upbdefs_google_protobuf_FileDescriptorProto_is(m)); return upb_msgdef_itof(m, 3); }
UPB_INLINE const upb_fielddef *upbdefs_google_protobuf_FileDescriptorProto_f_enum_type(const upb_msgdef *m) { UPB_ASSERT(upbdefs_google_protobuf_FileDescriptorProto_is(m)); return upb_msgdef_itof(m, 5); }
UPB_INLINE const upb_fielddef *upbdefs_google_protobuf_FileDescriptorProto_f_extension(const upb_msgdef *m) { UPB_ASSERT(upbdefs_google_protobuf_FileDescriptorProto_is(m)); return upb_msgdef_itof(m, 7); }
UPB_INLINE const upb_fielddef *upbdefs_google_protobuf_FileDescriptorProto_f_message_type(const upb_msgdef *m) { UPB_ASSERT(upbdefs_google_protobuf_FileDescriptorProto_is(m)); return upb_msgdef_itof(m, 4); }
UPB_INLINE const upb_fielddef *upbdefs_google_protobuf_FileDescriptorProto_f_name(const upb_msgdef *m) { UPB_ASSERT(upbdefs_google_protobuf_FileDescriptorProto_is(m)); return upb_msgdef_itof(m, 1); }
UPB_INLINE const upb_fielddef *upbdefs_google_protobuf_FileDescriptorProto_f_options(const upb_msgdef *m) { UPB_ASSERT(upbdefs_google_protobuf_FileDescriptorProto_is(m)); return upb_msgdef_itof(m, 8); }
UPB_INLINE const upb_fielddef *upbdefs_google_protobuf_FileDescriptorProto_f_package(const upb_msgdef *m) { UPB_ASSERT(upbdefs_google_protobuf_FileDescriptorProto_is(m)); return upb_msgdef_itof(m, 2); }
UPB_INLINE const upb_fielddef *upbdefs_google_protobuf_FileDescriptorProto_f_public_dependency(const upb_msgdef *m) { UPB_ASSERT(upbdefs_google_protobuf_FileDescriptorProto_is(m)); return upb_msgdef_itof(m, 10); }
UPB_INLINE const upb_fielddef *upbdefs_google_protobuf_FileDescriptorProto_f_service(const upb_msgdef *m) { UPB_ASSERT(upbdefs_google_protobuf_FileDescriptorProto_is(m)); return upb_msgdef_itof(m, 6); }
UPB_INLINE const upb_fielddef *upbdefs_google_protobuf_FileDescriptorProto_f_source_code_info(const upb_msgdef *m) { UPB_ASSERT(upbdefs_google_protobuf_FileDescriptorProto_is(m)); return upb_msgdef_itof(m, 9); }
UPB_INLINE const upb_fielddef *upbdefs_google_protobuf_FileDescriptorProto_f_syntax(const upb_msgdef *m) { UPB_ASSERT(upbdefs_google_protobuf_FileDescriptorProto_is(m)); return upb_msgdef_itof(m, 12); }
UPB_INLINE const upb_fielddef *upbdefs_google_protobuf_FileDescriptorProto_f_weak_dependency(const upb_msgdef *m) { UPB_ASSERT(upbdefs_google_protobuf_FileDescriptorProto_is(m)); return upb_msgdef_itof(m, 11); }
UPB_INLINE const upb_fielddef *upbdefs_google_protobuf_FileDescriptorSet_f_file(const upb_msgdef *m) { UPB_ASSERT(upbdefs_google_protobuf_FileDescriptorSet_is(m)); return upb_msgdef_itof(m, 1); }
UPB_INLINE const upb_fielddef *upbdefs_google_protobuf_FileOptions_f_cc_enable_arenas(const upb_msgdef *m) { UPB_ASSERT(upbdefs_google_protobuf_FileOptions_is(m)); return upb_msgdef_itof(m, 31); }
UPB_INLINE const upb_fielddef *upbdefs_google_protobuf_FileOptions_f_cc_generic_services(const upb_msgdef *m) { UPB_ASSERT(upbdefs_google_protobuf_FileOptions_is(m)); return upb_msgdef_itof(m, 16); }
UPB_INLINE const upb_fielddef *upbdefs_google_protobuf_FileOptions_f_csharp_namespace(const upb_msgdef *m) { UPB_ASSERT(upbdefs_google_protobuf_FileOptions_is(m)); return upb_msgdef_itof(m, 37); }
UPB_INLINE const upb_fielddef *upbdefs_google_protobuf_FileOptions_f_deprecated(const upb_msgdef *m) { UPB_ASSERT(upbdefs_google_protobuf_FileOptions_is(m)); return upb_msgdef_itof(m, 23); }
UPB_INLINE const upb_fielddef *upbdefs_google_protobuf_FileOptions_f_go_package(const upb_msgdef *m) { UPB_ASSERT(upbdefs_google_protobuf_FileOptions_is(m)); return upb_msgdef_itof(m, 11); }
UPB_INLINE const upb_fielddef *upbdefs_google_protobuf_FileOptions_f_java_generate_equals_and_hash(const upb_msgdef *m) { UPB_ASSERT(upbdefs_google_protobuf_FileOptions_is(m)); return upb_msgdef_itof(m, 20); }
UPB_INLINE const upb_fielddef *upbdefs_google_protobuf_FileOptions_f_java_generic_services(const upb_msgdef *m) { UPB_ASSERT(upbdefs_google_protobuf_FileOptions_is(m)); return upb_msgdef_itof(m, 17); }
UPB_INLINE const upb_fielddef *upbdefs_google_protobuf_FileOptions_f_java_multiple_files(const upb_msgdef *m) { UPB_ASSERT(upbdefs_google_protobuf_FileOptions_is(m)); return upb_msgdef_itof(m, 10); }
UPB_INLINE const upb_fielddef *upbdefs_google_protobuf_FileOptions_f_java_outer_classname(const upb_msgdef *m) { UPB_ASSERT(upbdefs_google_protobuf_FileOptions_is(m)); return upb_msgdef_itof(m, 8); }
UPB_INLINE const upb_fielddef *upbdefs_google_protobuf_FileOptions_f_java_package(const upb_msgdef *m) { UPB_ASSERT(upbdefs_google_protobuf_FileOptions_is(m)); return upb_msgdef_itof(m, 1); }
UPB_INLINE const upb_fielddef *upbdefs_google_protobuf_FileOptions_f_java_string_check_utf8(const upb_msgdef *m) { UPB_ASSERT(upbdefs_google_protobuf_FileOptions_is(m)); return upb_msgdef_itof(m, 27); }
UPB_INLINE const upb_fielddef *upbdefs_google_protobuf_FileOptions_f_javanano_use_deprecated_package(const upb_msgdef *m) { UPB_ASSERT(upbdefs_google_protobuf_FileOptions_is(m)); return upb_msgdef_itof(m, 38); }
UPB_INLINE const upb_fielddef *upbdefs_google_protobuf_FileOptions_f_objc_class_prefix(const upb_msgdef *m) { UPB_ASSERT(upbdefs_google_protobuf_FileOptions_is(m)); return upb_msgdef_itof(m, 36); }
UPB_INLINE const upb_fielddef *upbdefs_google_protobuf_FileOptions_f_optimize_for(const upb_msgdef *m) { UPB_ASSERT(upbdefs_google_protobuf_FileOptions_is(m)); return upb_msgdef_itof(m, 9); }
UPB_INLINE const upb_fielddef *upbdefs_google_protobuf_FileOptions_f_php_class_prefix(const upb_msgdef *m) { UPB_ASSERT(upbdefs_google_protobuf_FileOptions_is(m)); return upb_msgdef_itof(m, 40); }
UPB_INLINE const upb_fielddef *upbdefs_google_protobuf_FileOptions_f_php_namespace(const upb_msgdef *m) { UPB_ASSERT(upbdefs_google_protobuf_FileOptions_is(m)); return upb_msgdef_itof(m, 41); }
UPB_INLINE const upb_fielddef *upbdefs_google_protobuf_FileOptions_f_py_generic_services(const upb_msgdef *m) { UPB_ASSERT(upbdefs_google_protobuf_FileOptions_is(m)); return upb_msgdef_itof(m, 18); }
UPB_INLINE const upb_fielddef *upbdefs_google_protobuf_FileOptions_f_uninterpreted_option(const upb_msgdef *m) { UPB_ASSERT(upbdefs_google_protobuf_FileOptions_is(m)); return upb_msgdef_itof(m, 999); }
UPB_INLINE const upb_fielddef *upbdefs_google_protobuf_MessageOptions_f_deprecated(const upb_msgdef *m) { UPB_ASSERT(upbdefs_google_protobuf_MessageOptions_is(m)); return upb_msgdef_itof(m, 3); }
UPB_INLINE const upb_fielddef *upbdefs_google_protobuf_MessageOptions_f_map_entry(const upb_msgdef *m) { UPB_ASSERT(upbdefs_google_protobuf_MessageOptions_is(m)); return upb_msgdef_itof(m, 7); }
UPB_INLINE const upb_fielddef *upbdefs_google_protobuf_MessageOptions_f_message_set_wire_format(const upb_msgdef *m) { UPB_ASSERT(upbdefs_google_protobuf_MessageOptions_is(m)); return upb_msgdef_itof(m, 1); }
UPB_INLINE const upb_fielddef *upbdefs_google_protobuf_MessageOptions_f_no_standard_descriptor_accessor(const upb_msgdef *m) { UPB_ASSERT(upbdefs_google_protobuf_MessageOptions_is(m)); return upb_msgdef_itof(m, 2); }
UPB_INLINE const upb_fielddef *upbdefs_google_protobuf_MessageOptions_f_uninterpreted_option(const upb_msgdef *m) { UPB_ASSERT(upbdefs_google_protobuf_MessageOptions_is(m)); return upb_msgdef_itof(m, 999); }
UPB_INLINE const upb_fielddef *upbdefs_google_protobuf_MethodDescriptorProto_f_client_streaming(const upb_msgdef *m) { UPB_ASSERT(upbdefs_google_protobuf_MethodDescriptorProto_is(m)); return upb_msgdef_itof(m, 5); }
UPB_INLINE const upb_fielddef *upbdefs_google_protobuf_MethodDescriptorProto_f_input_type(const upb_msgdef *m) { UPB_ASSERT(upbdefs_google_protobuf_MethodDescriptorProto_is(m)); return upb_msgdef_itof(m, 2); }
UPB_INLINE const upb_fielddef *upbdefs_google_protobuf_MethodDescriptorProto_f_name(const upb_msgdef *m) { UPB_ASSERT(upbdefs_google_protobuf_MethodDescriptorProto_is(m)); return upb_msgdef_itof(m, 1); }
UPB_INLINE const upb_fielddef *upbdefs_google_protobuf_MethodDescriptorProto_f_options(const upb_msgdef *m) { UPB_ASSERT(upbdefs_google_protobuf_MethodDescriptorProto_is(m)); return upb_msgdef_itof(m, 4); }
UPB_INLINE const upb_fielddef *upbdefs_google_protobuf_MethodDescriptorProto_f_output_type(const upb_msgdef *m) { UPB_ASSERT(upbdefs_google_protobuf_MethodDescriptorProto_is(m)); return upb_msgdef_itof(m, 3); }
UPB_INLINE const upb_fielddef *upbdefs_google_protobuf_MethodDescriptorProto_f_server_streaming(const upb_msgdef *m) { UPB_ASSERT(upbdefs_google_protobuf_MethodDescriptorProto_is(m)); return upb_msgdef_itof(m, 6); }
UPB_INLINE const upb_fielddef *upbdefs_google_protobuf_MethodOptions_f_deprecated(const upb_msgdef *m) { UPB_ASSERT(upbdefs_google_protobuf_MethodOptions_is(m)); return upb_msgdef_itof(m, 33); }
UPB_INLINE const upb_fielddef *upbdefs_google_protobuf_MethodOptions_f_uninterpreted_option(const upb_msgdef *m) { UPB_ASSERT(upbdefs_google_protobuf_MethodOptions_is(m)); return upb_msgdef_itof(m, 999); }
UPB_INLINE const upb_fielddef *upbdefs_google_protobuf_OneofDescriptorProto_f_name(const upb_msgdef *m) { UPB_ASSERT(upbdefs_google_protobuf_OneofDescriptorProto_is(m)); return upb_msgdef_itof(m, 1); }
UPB_INLINE const upb_fielddef *upbdefs_google_protobuf_ServiceDescriptorProto_f_method(const upb_msgdef *m) { UPB_ASSERT(upbdefs_google_protobuf_ServiceDescriptorProto_is(m)); return upb_msgdef_itof(m, 2); }
UPB_INLINE const upb_fielddef *upbdefs_google_protobuf_ServiceDescriptorProto_f_name(const upb_msgdef *m) { UPB_ASSERT(upbdefs_google_protobuf_ServiceDescriptorProto_is(m)); return upb_msgdef_itof(m, 1); }
UPB_INLINE const upb_fielddef *upbdefs_google_protobuf_ServiceDescriptorProto_f_options(const upb_msgdef *m) { UPB_ASSERT(upbdefs_google_protobuf_ServiceDescriptorProto_is(m)); return upb_msgdef_itof(m, 3); }
UPB_INLINE const upb_fielddef *upbdefs_google_protobuf_ServiceOptions_f_deprecated(const upb_msgdef *m) { UPB_ASSERT(upbdefs_google_protobuf_ServiceOptions_is(m)); return upb_msgdef_itof(m, 33); }
UPB_INLINE const upb_fielddef *upbdefs_google_protobuf_ServiceOptions_f_uninterpreted_option(const upb_msgdef *m) { UPB_ASSERT(upbdefs_google_protobuf_ServiceOptions_is(m)); return upb_msgdef_itof(m, 999); }
UPB_INLINE const upb_fielddef *upbdefs_google_protobuf_SourceCodeInfo_Location_f_leading_comments(const upb_msgdef *m) { UPB_ASSERT(upbdefs_google_protobuf_SourceCodeInfo_Location_is(m)); return upb_msgdef_itof(m, 3); }
UPB_INLINE const upb_fielddef *upbdefs_google_protobuf_SourceCodeInfo_Location_f_leading_detached_comments(const upb_msgdef *m) { UPB_ASSERT(upbdefs_google_protobuf_SourceCodeInfo_Location_is(m)); return upb_msgdef_itof(m, 6); }
UPB_INLINE const upb_fielddef *upbdefs_google_protobuf_SourceCodeInfo_Location_f_path(const upb_msgdef *m) { UPB_ASSERT(upbdefs_google_protobuf_SourceCodeInfo_Location_is(m)); return upb_msgdef_itof(m, 1); }
UPB_INLINE const upb_fielddef *upbdefs_google_protobuf_SourceCodeInfo_Location_f_span(const upb_msgdef *m) { UPB_ASSERT(upbdefs_google_protobuf_SourceCodeInfo_Location_is(m)); return upb_msgdef_itof(m, 2); }
UPB_INLINE const upb_fielddef *upbdefs_google_protobuf_SourceCodeInfo_Location_f_trailing_comments(const upb_msgdef *m) { UPB_ASSERT(upbdefs_google_protobuf_SourceCodeInfo_Location_is(m)); return upb_msgdef_itof(m, 4); }
UPB_INLINE const upb_fielddef *upbdefs_google_protobuf_SourceCodeInfo_f_location(const upb_msgdef *m) { UPB_ASSERT(upbdefs_google_protobuf_SourceCodeInfo_is(m)); return upb_msgdef_itof(m, 1); }
UPB_INLINE const upb_fielddef *upbdefs_google_protobuf_UninterpretedOption_NamePart_f_is_extension(const upb_msgdef *m) { UPB_ASSERT(upbdefs_google_protobuf_UninterpretedOption_NamePart_is(m)); return upb_msgdef_itof(m, 2); }
UPB_INLINE const upb_fielddef *upbdefs_google_protobuf_UninterpretedOption_NamePart_f_name_part(const upb_msgdef *m) { UPB_ASSERT(upbdefs_google_protobuf_UninterpretedOption_NamePart_is(m)); return upb_msgdef_itof(m, 1); }
UPB_INLINE const upb_fielddef *upbdefs_google_protobuf_UninterpretedOption_f_aggregate_value(const upb_msgdef *m) { UPB_ASSERT(upbdefs_google_protobuf_UninterpretedOption_is(m)); return upb_msgdef_itof(m, 8); }
UPB_INLINE const upb_fielddef *upbdefs_google_protobuf_UninterpretedOption_f_double_value(const upb_msgdef *m) { UPB_ASSERT(upbdefs_google_protobuf_UninterpretedOption_is(m)); return upb_msgdef_itof(m, 6); }
UPB_INLINE const upb_fielddef *upbdefs_google_protobuf_UninterpretedOption_f_identifier_value(const upb_msgdef *m) { UPB_ASSERT(upbdefs_google_protobuf_UninterpretedOption_is(m)); return upb_msgdef_itof(m, 3); }
UPB_INLINE const upb_fielddef *upbdefs_google_protobuf_UninterpretedOption_f_name(const upb_msgdef *m) { UPB_ASSERT(upbdefs_google_protobuf_UninterpretedOption_is(m)); return upb_msgdef_itof(m, 2); }
UPB_INLINE const upb_fielddef *upbdefs_google_protobuf_UninterpretedOption_f_negative_int_value(const upb_msgdef *m) { UPB_ASSERT(upbdefs_google_protobuf_UninterpretedOption_is(m)); return upb_msgdef_itof(m, 5); }
UPB_INLINE const upb_fielddef *upbdefs_google_protobuf_UninterpretedOption_f_positive_int_value(const upb_msgdef *m) { UPB_ASSERT(upbdefs_google_protobuf_UninterpretedOption_is(m)); return upb_msgdef_itof(m, 4); }
UPB_INLINE const upb_fielddef *upbdefs_google_protobuf_UninterpretedOption_f_string_value(const upb_msgdef *m) { UPB_ASSERT(upbdefs_google_protobuf_UninterpretedOption_is(m)); return upb_msgdef_itof(m, 7); }

UPB_END_EXTERN_C

#ifdef __cplusplus

namespace upbdefs {
namespace google {
namespace protobuf {

class DescriptorProto : public ::upb::reffed_ptr<const ::upb::MessageDef> {
 public:
  DescriptorProto(const ::upb::MessageDef* m, const void *ref_donor = NULL)
      : reffed_ptr(m, ref_donor) {
    UPB_ASSERT(upbdefs_google_protobuf_DescriptorProto_is(m));
  }

  static DescriptorProto get() {
    const ::upb::MessageDef* m = upbdefs_google_protobuf_DescriptorProto_get(&m);
    return DescriptorProto(m, &m);
  }

  class ExtensionRange : public ::upb::reffed_ptr<const ::upb::MessageDef> {
   public:
    ExtensionRange(const ::upb::MessageDef* m, const void *ref_donor = NULL)
        : reffed_ptr(m, ref_donor) {
      UPB_ASSERT(upbdefs_google_protobuf_DescriptorProto_ExtensionRange_is(m));
    }

    static ExtensionRange get() {
      const ::upb::MessageDef* m = upbdefs_google_protobuf_DescriptorProto_ExtensionRange_get(&m);
      return ExtensionRange(m, &m);
    }
  };

  class ReservedRange : public ::upb::reffed_ptr<const ::upb::MessageDef> {
   public:
    ReservedRange(const ::upb::MessageDef* m, const void *ref_donor = NULL)
        : reffed_ptr(m, ref_donor) {
      UPB_ASSERT(upbdefs_google_protobuf_DescriptorProto_ReservedRange_is(m));
    }

    static ReservedRange get() {
      const ::upb::MessageDef* m = upbdefs_google_protobuf_DescriptorProto_ReservedRange_get(&m);
      return ReservedRange(m, &m);
    }
  };
};

class EnumDescriptorProto : public ::upb::reffed_ptr<const ::upb::MessageDef> {
 public:
  EnumDescriptorProto(const ::upb::MessageDef* m, const void *ref_donor = NULL)
      : reffed_ptr(m, ref_donor) {
    UPB_ASSERT(upbdefs_google_protobuf_EnumDescriptorProto_is(m));
  }

  static EnumDescriptorProto get() {
    const ::upb::MessageDef* m = upbdefs_google_protobuf_EnumDescriptorProto_get(&m);
    return EnumDescriptorProto(m, &m);
  }
};

class EnumOptions : public ::upb::reffed_ptr<const ::upb::MessageDef> {
 public:
  EnumOptions(const ::upb::MessageDef* m, const void *ref_donor = NULL)
      : reffed_ptr(m, ref_donor) {
    UPB_ASSERT(upbdefs_google_protobuf_EnumOptions_is(m));
  }

  static EnumOptions get() {
    const ::upb::MessageDef* m = upbdefs_google_protobuf_EnumOptions_get(&m);
    return EnumOptions(m, &m);
  }
};

class EnumValueDescriptorProto : public ::upb::reffed_ptr<const ::upb::MessageDef> {
 public:
  EnumValueDescriptorProto(const ::upb::MessageDef* m, const void *ref_donor = NULL)
      : reffed_ptr(m, ref_donor) {
    UPB_ASSERT(upbdefs_google_protobuf_EnumValueDescriptorProto_is(m));
  }

  static EnumValueDescriptorProto get() {
    const ::upb::MessageDef* m = upbdefs_google_protobuf_EnumValueDescriptorProto_get(&m);
    return EnumValueDescriptorProto(m, &m);
  }
};

class EnumValueOptions : public ::upb::reffed_ptr<const ::upb::MessageDef> {
 public:
  EnumValueOptions(const ::upb::MessageDef* m, const void *ref_donor = NULL)
      : reffed_ptr(m, ref_donor) {
    UPB_ASSERT(upbdefs_google_protobuf_EnumValueOptions_is(m));
  }

  static EnumValueOptions get() {
    const ::upb::MessageDef* m = upbdefs_google_protobuf_EnumValueOptions_get(&m);
    return EnumValueOptions(m, &m);
  }
};

class FieldDescriptorProto : public ::upb::reffed_ptr<const ::upb::MessageDef> {
 public:
  FieldDescriptorProto(const ::upb::MessageDef* m, const void *ref_donor = NULL)
      : reffed_ptr(m, ref_donor) {
    UPB_ASSERT(upbdefs_google_protobuf_FieldDescriptorProto_is(m));
  }

  static FieldDescriptorProto get() {
    const ::upb::MessageDef* m = upbdefs_google_protobuf_FieldDescriptorProto_get(&m);
    return FieldDescriptorProto(m, &m);
  }

  class Label : public ::upb::reffed_ptr<const ::upb::EnumDef> {
   public:
    Label(const ::upb::EnumDef* e, const void *ref_donor = NULL)
        : reffed_ptr(e, ref_donor) {
      UPB_ASSERT(upbdefs_google_protobuf_FieldDescriptorProto_Label_is(e));
    }
    static Label get() {
      const ::upb::EnumDef* e = upbdefs_google_protobuf_FieldDescriptorProto_Label_get(&e);
      return Label(e, &e);
    }
  };

  class Type : public ::upb::reffed_ptr<const ::upb::EnumDef> {
   public:
    Type(const ::upb::EnumDef* e, const void *ref_donor = NULL)
        : reffed_ptr(e, ref_donor) {
      UPB_ASSERT(upbdefs_google_protobuf_FieldDescriptorProto_Type_is(e));
    }
    static Type get() {
      const ::upb::EnumDef* e = upbdefs_google_protobuf_FieldDescriptorProto_Type_get(&e);
      return Type(e, &e);
    }
  };
};

class FieldOptions : public ::upb::reffed_ptr<const ::upb::MessageDef> {
 public:
  FieldOptions(const ::upb::MessageDef* m, const void *ref_donor = NULL)
      : reffed_ptr(m, ref_donor) {
    UPB_ASSERT(upbdefs_google_protobuf_FieldOptions_is(m));
  }

  static FieldOptions get() {
    const ::upb::MessageDef* m = upbdefs_google_protobuf_FieldOptions_get(&m);
    return FieldOptions(m, &m);
  }

  class CType : public ::upb::reffed_ptr<const ::upb::EnumDef> {
   public:
    CType(const ::upb::EnumDef* e, const void *ref_donor = NULL)
        : reffed_ptr(e, ref_donor) {
      UPB_ASSERT(upbdefs_google_protobuf_FieldOptions_CType_is(e));
    }
    static CType get() {
      const ::upb::EnumDef* e = upbdefs_google_protobuf_FieldOptions_CType_get(&e);
      return CType(e, &e);
    }
  };

  class JSType : public ::upb::reffed_ptr<const ::upb::EnumDef> {
   public:
    JSType(const ::upb::EnumDef* e, const void *ref_donor = NULL)
        : reffed_ptr(e, ref_donor) {
      UPB_ASSERT(upbdefs_google_protobuf_FieldOptions_JSType_is(e));
    }
    static JSType get() {
      const ::upb::EnumDef* e = upbdefs_google_protobuf_FieldOptions_JSType_get(&e);
      return JSType(e, &e);
    }
  };
};

class FileDescriptorProto : public ::upb::reffed_ptr<const ::upb::MessageDef> {
 public:
  FileDescriptorProto(const ::upb::MessageDef* m, const void *ref_donor = NULL)
      : reffed_ptr(m, ref_donor) {
    UPB_ASSERT(upbdefs_google_protobuf_FileDescriptorProto_is(m));
  }

  static FileDescriptorProto get() {
    const ::upb::MessageDef* m = upbdefs_google_protobuf_FileDescriptorProto_get(&m);
    return FileDescriptorProto(m, &m);
  }
};

class FileDescriptorSet : public ::upb::reffed_ptr<const ::upb::MessageDef> {
 public:
  FileDescriptorSet(const ::upb::MessageDef* m, const void *ref_donor = NULL)
      : reffed_ptr(m, ref_donor) {
    UPB_ASSERT(upbdefs_google_protobuf_FileDescriptorSet_is(m));
  }

  static FileDescriptorSet get() {
    const ::upb::MessageDef* m = upbdefs_google_protobuf_FileDescriptorSet_get(&m);
    return FileDescriptorSet(m, &m);
  }
};

class FileOptions : public ::upb::reffed_ptr<const ::upb::MessageDef> {
 public:
  FileOptions(const ::upb::MessageDef* m, const void *ref_donor = NULL)
      : reffed_ptr(m, ref_donor) {
    UPB_ASSERT(upbdefs_google_protobuf_FileOptions_is(m));
  }

  static FileOptions get() {
    const ::upb::MessageDef* m = upbdefs_google_protobuf_FileOptions_get(&m);
    return FileOptions(m, &m);
  }

  class OptimizeMode : public ::upb::reffed_ptr<const ::upb::EnumDef> {
   public:
    OptimizeMode(const ::upb::EnumDef* e, const void *ref_donor = NULL)
        : reffed_ptr(e, ref_donor) {
      UPB_ASSERT(upbdefs_google_protobuf_FileOptions_OptimizeMode_is(e));
    }
    static OptimizeMode get() {
      const ::upb::EnumDef* e = upbdefs_google_protobuf_FileOptions_OptimizeMode_get(&e);
      return OptimizeMode(e, &e);
    }
  };
};

class MessageOptions : public ::upb::reffed_ptr<const ::upb::MessageDef> {
 public:
  MessageOptions(const ::upb::MessageDef* m, const void *ref_donor = NULL)
      : reffed_ptr(m, ref_donor) {
    UPB_ASSERT(upbdefs_google_protobuf_MessageOptions_is(m));
  }

  static MessageOptions get() {
    const ::upb::MessageDef* m = upbdefs_google_protobuf_MessageOptions_get(&m);
    return MessageOptions(m, &m);
  }
};

class MethodDescriptorProto : public ::upb::reffed_ptr<const ::upb::MessageDef> {
 public:
  MethodDescriptorProto(const ::upb::MessageDef* m, const void *ref_donor = NULL)
      : reffed_ptr(m, ref_donor) {
    UPB_ASSERT(upbdefs_google_protobuf_MethodDescriptorProto_is(m));
  }

  static MethodDescriptorProto get() {
    const ::upb::MessageDef* m = upbdefs_google_protobuf_MethodDescriptorProto_get(&m);
    return MethodDescriptorProto(m, &m);
  }
};

class MethodOptions : public ::upb::reffed_ptr<const ::upb::MessageDef> {
 public:
  MethodOptions(const ::upb::MessageDef* m, const void *ref_donor = NULL)
      : reffed_ptr(m, ref_donor) {
    UPB_ASSERT(upbdefs_google_protobuf_MethodOptions_is(m));
  }

  static MethodOptions get() {
    const ::upb::MessageDef* m = upbdefs_google_protobuf_MethodOptions_get(&m);
    return MethodOptions(m, &m);
  }
};

class OneofDescriptorProto : public ::upb::reffed_ptr<const ::upb::MessageDef> {
 public:
  OneofDescriptorProto(const ::upb::MessageDef* m, const void *ref_donor = NULL)
      : reffed_ptr(m, ref_donor) {
    UPB_ASSERT(upbdefs_google_protobuf_OneofDescriptorProto_is(m));
  }

  static OneofDescriptorProto get() {
    const ::upb::MessageDef* m = upbdefs_google_protobuf_OneofDescriptorProto_get(&m);
    return OneofDescriptorProto(m, &m);
  }
};

class ServiceDescriptorProto : public ::upb::reffed_ptr<const ::upb::MessageDef> {
 public:
  ServiceDescriptorProto(const ::upb::MessageDef* m, const void *ref_donor = NULL)
      : reffed_ptr(m, ref_donor) {
    UPB_ASSERT(upbdefs_google_protobuf_ServiceDescriptorProto_is(m));
  }

  static ServiceDescriptorProto get() {
    const ::upb::MessageDef* m = upbdefs_google_protobuf_ServiceDescriptorProto_get(&m);
    return ServiceDescriptorProto(m, &m);
  }
};

class ServiceOptions : public ::upb::reffed_ptr<const ::upb::MessageDef> {
 public:
  ServiceOptions(const ::upb::MessageDef* m, const void *ref_donor = NULL)
      : reffed_ptr(m, ref_donor) {
    UPB_ASSERT(upbdefs_google_protobuf_ServiceOptions_is(m));
  }

  static ServiceOptions get() {
    const ::upb::MessageDef* m = upbdefs_google_protobuf_ServiceOptions_get(&m);
    return ServiceOptions(m, &m);
  }
};

class SourceCodeInfo : public ::upb::reffed_ptr<const ::upb::MessageDef> {
 public:
  SourceCodeInfo(const ::upb::MessageDef* m, const void *ref_donor = NULL)
      : reffed_ptr(m, ref_donor) {
    UPB_ASSERT(upbdefs_google_protobuf_SourceCodeInfo_is(m));
  }

  static SourceCodeInfo get() {
    const ::upb::MessageDef* m = upbdefs_google_protobuf_SourceCodeInfo_get(&m);
    return SourceCodeInfo(m, &m);
  }

  class Location : public ::upb::reffed_ptr<const ::upb::MessageDef> {
   public:
    Location(const ::upb::MessageDef* m, const void *ref_donor = NULL)
        : reffed_ptr(m, ref_donor) {
      UPB_ASSERT(upbdefs_google_protobuf_SourceCodeInfo_Location_is(m));
    }

    static Location get() {
      const ::upb::MessageDef* m = upbdefs_google_protobuf_SourceCodeInfo_Location_get(&m);
      return Location(m, &m);
    }
  };
};

class UninterpretedOption : public ::upb::reffed_ptr<const ::upb::MessageDef> {
 public:
  UninterpretedOption(const ::upb::MessageDef* m, const void *ref_donor = NULL)
      : reffed_ptr(m, ref_donor) {
    UPB_ASSERT(upbdefs_google_protobuf_UninterpretedOption_is(m));
  }

  static UninterpretedOption get() {
    const ::upb::MessageDef* m = upbdefs_google_protobuf_UninterpretedOption_get(&m);
    return UninterpretedOption(m, &m);
  }

  class NamePart : public ::upb::reffed_ptr<const ::upb::MessageDef> {
   public:
    NamePart(const ::upb::MessageDef* m, const void *ref_donor = NULL)
        : reffed_ptr(m, ref_donor) {
      UPB_ASSERT(upbdefs_google_protobuf_UninterpretedOption_NamePart_is(m));
    }

    static NamePart get() {
      const ::upb::MessageDef* m = upbdefs_google_protobuf_UninterpretedOption_NamePart_get(&m);
      return NamePart(m, &m);
    }
  };
};

}  /* namespace protobuf */
}  /* namespace google */
}  /* namespace upbdefs */

#endif  /* __cplusplus */

#endif  /* UPB_DESCRIPTOR_DESCRIPTOR_PROTO_UPB_H_ */
/*
** Internal-only definitions for the decoder.
*/

#ifndef UPB_DECODER_INT_H_
#define UPB_DECODER_INT_H_

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

/* The maximum number of bytes we are required to buffer internally between
 * calls to the decoder.  The value is 14: a 5 byte unknown tag plus ten-byte
 * varint, less one because we are buffering an incomplete value.
 *
 * Should only be used by unit tests. */
#define UPB_DECODER_MAX_RESIDUAL_BYTES 14

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
#define UPB_PB_DECODER_SIZE 4416

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

#ifndef __cplusplus

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

UPB_INLINE opcode getop(uint32_t instr) { return (opcode)(instr & 0xff); }

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

  /* Buffer for residual bytes not parsed from the previous buffer. */
  char residual[UPB_DECODER_MAX_RESIDUAL_BYTES];
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

#endif  /* __cplusplus */

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

#define UPB_MAX_WIRE_TYPE 5

/* The maximum number of bytes that it takes to encode a 64-bit varint. */
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

upb_decoderet upb_vdecode_max8_branch32(upb_decoderet r);
upb_decoderet upb_vdecode_max8_branch64(upb_decoderet r);

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
#undef UPB_VARINT_DECODER_CHECK2

/* Our canonical functions for decoding varints, based on the currently
 * favored best-performing implementations. */
UPB_INLINE upb_decoderet upb_vdecode_fast(const char *p) {
  if (sizeof(long) == 8)
    return upb_vdecode_check2_branch64(p);
  else
    return upb_vdecode_check2_branch32(p);
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
  UPB_ASSERT(bytes <= 5);
  memcpy(&ret, buf, bytes);
  UPB_ASSERT(ret <= 0xffffffffffU);
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
#include <vector>

extern "C" {
#endif

/* Loads a binary descriptor and returns a NULL-terminated array of unfrozen
 * filedefs.  The caller owns the returned array, which must be freed with
 * upb_gfree(). */
upb_filedef **upb_loaddescriptor(const char *buf, size_t n, const void *owner,
                                 upb_status *status);

#ifdef __cplusplus
}  /* extern "C" */

namespace upb {

inline bool LoadDescriptor(const char* buf, size_t n, Status* status,
                           std::vector<reffed_ptr<FileDef> >* files) {
  FileDef** parsed_files = upb_loaddescriptor(buf, n, &parsed_files, status);

  if (parsed_files) {
    FileDef** p = parsed_files;
    while (*p) {
      files->push_back(reffed_ptr<FileDef>(*p, &parsed_files));
      ++p;
    }
    free(parsed_files);
    return true;
  } else {
    return false;
  }
}

/* Templated so it can accept both string and std::string. */
template <typename T>
bool LoadDescriptor(const T& desc, Status* status,
                    std::vector<reffed_ptr<FileDef> >* files) {
  return LoadDescriptor(desc.c_str(), desc.size(), status, files);
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
class ParserMethod;
}  /* namespace json */
}  /* namespace upb */
#endif

UPB_DECLARE_TYPE(upb::json::Parser, upb_json_parser)
UPB_DECLARE_DERIVED_TYPE(upb::json::ParserMethod, upb::RefCounted,
                         upb_json_parsermethod, upb_refcounted)

/* upb::json::Parser **********************************************************/

/* Preallocation hint: parser won't allocate more bytes than this when first
 * constructed.  This hint may be an overestimate for some build configurations.
 * But if the parser library is upgraded without recompiling the application,
 * it may be an underestimate. */
#define UPB_JSON_PARSER_SIZE 5712

#ifdef __cplusplus

/* Parses an incoming BytesStream, pushing the results to the destination
 * sink. */
class upb::json::Parser {
 public:
  static Parser* Create(Environment* env, const ParserMethod* method,
                        const SymbolTable* symtab,
                        Sink* output, bool ignore_json_unknown);

  BytesSink* input();

 private:
  UPB_DISALLOW_POD_OPS(Parser, upb::json::Parser)
};

class upb::json::ParserMethod {
 public:
  /* Include base methods from upb::ReferenceCounted. */
  UPB_REFCOUNTED_CPPMETHODS

  /* Returns handlers for parsing according to the specified schema. */
  static reffed_ptr<const ParserMethod> New(const upb::MessageDef* md);

  /* The destination handlers that are statically bound to this method.
   * This method is only capable of outputting to a sink that uses these
   * handlers. */
  const Handlers* dest_handlers() const;

  /* The input handlers for this decoder method. */
  const BytesHandler* input_handler() const;

 private:
  UPB_DISALLOW_POD_OPS(ParserMethod, upb::json::ParserMethod)
};

#endif

UPB_BEGIN_EXTERN_C

upb_json_parser* upb_json_parser_create(upb_env* e,
                                        const upb_json_parsermethod* m,
                                        const upb_symtab* symtab,
                                        upb_sink* output,
                                        bool ignore_json_unknown);
upb_bytessink *upb_json_parser_input(upb_json_parser *p);

upb_json_parsermethod* upb_json_parsermethod_new(const upb_msgdef* md,
                                                 const void* owner);
const upb_handlers *upb_json_parsermethod_desthandlers(
    const upb_json_parsermethod *m);
const upb_byteshandler *upb_json_parsermethod_inputhandler(
    const upb_json_parsermethod *m);

/* Include refcounted methods like upb_json_parsermethod_ref(). */
UPB_REFCOUNTED_CMETHODS(upb_json_parsermethod, upb_json_parsermethod_upcast)

UPB_END_EXTERN_C

#ifdef __cplusplus

namespace upb {
namespace json {
inline Parser* Parser::Create(Environment* env, const ParserMethod* method,
                              const SymbolTable* symtab,
                              Sink* output, bool ignore_json_unknown) {
  return upb_json_parser_create(
      env, method, symtab, output, ignore_json_unknown);
}
inline BytesSink* Parser::input() {
  return upb_json_parser_input(this);
}

inline const Handlers* ParserMethod::dest_handlers() const {
  return upb_json_parsermethod_desthandlers(this);
}
inline const BytesHandler* ParserMethod::input_handler() const {
  return upb_json_parsermethod_inputhandler(this);
}
/* static */
inline reffed_ptr<const ParserMethod> ParserMethod::New(
    const MessageDef* md) {
  const upb_json_parsermethod *m = upb_json_parsermethod_new(md, &m);
  return reffed_ptr<const ParserMethod>(m, &m);
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

#define UPB_JSON_PRINTER_SIZE 192

#ifdef __cplusplus

/* Prints an incoming stream of data to a BytesSink in JSON format. */
class upb::json::Printer {
 public:
  static Printer* Create(Environment* env, const upb::Handlers* handlers,
                         BytesSink* output);

  /* The input to the printer. */
  Sink* input();

  /* Returns handlers for printing according to the specified schema.
   * If preserve_proto_fieldnames is true, the output JSON will use the
   * original .proto field names (ie. {"my_field":3}) instead of using
   * camelCased names, which is the default: (eg. {"myField":3}). */
  static reffed_ptr<const Handlers> NewHandlers(const upb::MessageDef* md,
                                                bool preserve_proto_fieldnames);

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
                                                 bool preserve_fieldnames,
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
    const upb::MessageDef *md, bool preserve_proto_fieldnames) {
  const Handlers* h = upb_json_printer_newhandlers(
      md, preserve_proto_fieldnames, &h);
  return reffed_ptr<const Handlers>(h, &h);
}
}  /* namespace json */
}  /* namespace upb */

#endif

#endif  /* UPB_JSON_TYPED_PRINTER_H_ */

#undef UPB_SIZE
#undef UPB_FIELD_AT
#undef UPB_READ_ONEOF
#undef UPB_WRITE_ONEOF
