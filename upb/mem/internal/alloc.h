#ifndef GOOGLE_UPB_UPB_MEM_INTERNAL_ALLOC_H__
#define GOOGLE_UPB_UPB_MEM_INTERNAL_ALLOC_H__

#include <stddef.h>

// Must be last.
#include "upb/port/def.inc"

#ifdef __cplusplus
extern "C" {
#endif

#if defined(__cplusplus)
#define UPB_THREAD_LOCAL thread_local
#elif defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 201112L) && \
    !defined(__STDC_NO_THREADS__)
#define UPB_THREAD_LOCAL _Thread_local
#elif defined(_MSC_VER)
#define UPB_THREAD_LOCAL __declspec(thread)
#elif defined(__GNUC__) || defined(__clang__)
#define UPB_THREAD_LOCAL __thread
#else
#define UPB_THREAD_LOCAL
#endif

#if !defined(NDEBUG)
#define UPB_ALLOCATION_COUNT
#endif

#ifdef UPB_ALLOCATION_COUNT
extern UPB_THREAD_LOCAL size_t upb_arena_alloc_count;
extern UPB_THREAD_LOCAL size_t upb_arena_alloc_fail_on;

#undef UPB_THREAD_LOCAL

#endif

UPB_NODISCARD UPB_FORCEINLINE bool upb_AllocationCount_IncrementAndCheck(void) {
#ifdef UPB_ALLOCATION_COUNT
  bool ok = (upb_arena_alloc_count != upb_arena_alloc_fail_on);
  upb_arena_alloc_count++;
  return ok;
#else
  return true;
#endif
}

#ifdef __cplusplus
} /* extern "C" */
#endif

#include "upb/port/undef.inc"

#endif  // GOOGLE_UPB_UPB_MEM_INTERNAL_ALLOC_H__
