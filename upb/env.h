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

#include "upb/upb.h"

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
