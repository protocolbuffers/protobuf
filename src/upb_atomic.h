/*
 * upb - a minimalist implementation of protocol buffers.
 *
 * Copyright (c) 2009 Joshua Haberman.  See LICENSE for details.
 *
 * Only a very small part of upb is thread-safe.  Notably, individual
 * messages, arrays, and strings are *not* thread safe for mutating.
 * However, we do make message *metadata* such as upb_msgdef and
 * upb_context thread-safe, and their ownership is tracked via atomic
 * refcounting.  This header implements the small number of atomic
 * primitives required to support this.  The primitives we implement
 * are:
 *
 * - a reader/writer lock (wrappers around platform-provided mutexes).
 * - an atomic refcount.
 */

#ifndef UPB_ATOMIC_H_
#define UPB_ATOMIC_H_

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* inline if possible, emit standalone code if required. */
#ifndef INLINE
#define INLINE static inline
#endif

// Until this stuff is actually working, make thread-unsafe the default.
#define UPB_THREAD_UNSAFE

#ifdef UPB_THREAD_UNSAFE

/* Non-thread-safe implementations. ******************************************/

typedef struct {
  int v;
} upb_atomic_refcount_t;

INLINE void upb_atomic_refcount_init(upb_atomic_refcount_t *a, int val) {
  a->v = val;
}

INLINE bool upb_atomic_ref(upb_atomic_refcount_t *a) {
  return a->v++ == 0;
}

INLINE bool upb_atomic_unref(upb_atomic_refcount_t *a) {
  return --a->v == 0;
}

INLINE int upb_atomic_read(upb_atomic_refcount_t *a) {
  return a->v;
}

INLINE bool upb_atomic_add(upb_atomic_refcount_t *a, int val) {
  a->v += val;
  return a->v == 0;
}

INLINE int upb_atomic_fetch_and_add(upb_atomic_refcount_t *a, int val) {
  int ret = a->v;
  a->v += val;
  return ret;
}

#endif

/* Atomic refcount ************************************************************/

#ifdef UPB_THREAD_UNSAFE

/* Already defined above. */

#elif (__GNUC__ == 4 && __GNUC_MINOR__ >= 1) || __GNUC__ > 4

/* GCC includes atomic primitives. */

typedef struct {
  volatile int v;
} upb_atomic_refcount_t;

INLINE void upb_atomic_refcount_init(upb_atomic_refcount_t *a, int val) {
  a->v = val;
  __sync_synchronize();   /* Ensure the initialized value is visible. */
}

INLINE bool upb_atomic_ref(upb_atomic_refcount_t *a) {
  return __sync_fetch_and_add(&a->v, 1) == 0;
}

INLINE bool upb_atomic_add(upb_atomic_refcount_t *a, int n) {
  return __sync_add_and_fetch(&a->v, n) == 0;
}

INLINE bool upb_atomic_unref(upb_atomic_refcount_t *a) {
  return __sync_sub_and_fetch(&a->v, 1) == 0;
}

INLINE bool upb_atomic_read(upb_atomic_refcount_t *a) {
  return __sync_fetch_and_add(&a->v, 0);
}

#elif defined(WIN32)

/* Windows defines atomic increment/decrement. */
#include <Windows.h>

typedef struct {
  volatile LONG val;
} upb_atomic_refcount_t;

INLINE void upb_atomic_refcount_init(upb_atomic_refcount_t *a, int val) {
  InterlockedExchange(&a->val, val);
}

INLINE bool upb_atomic_ref(upb_atomic_refcount_t *a) {
  return InterlockedIncrement(&a->val) == 1;
}

INLINE bool upb_atomic_unref(upb_atomic_refcount_t *a) {
  return InterlockedDecrement(&a->val) == 0;
}

#else
#error Atomic primitives not defined for your platform/CPU.  \
       Implement them or compile with UPB_THREAD_UNSAFE.
#endif

INLINE bool upb_atomic_only(upb_atomic_refcount_t *a) {
  return upb_atomic_read(a) == 1;
}

/* Reader/Writer lock. ********************************************************/

#ifdef UPB_THREAD_UNSAFE

typedef struct {
} upb_rwlock_t;

INLINE void upb_rwlock_init(upb_rwlock_t *l) { (void)l; }
INLINE void upb_rwlock_destroy(upb_rwlock_t *l) { (void)l; }
INLINE void upb_rwlock_rdlock(upb_rwlock_t *l) { (void)l; }
INLINE void upb_rwlock_wrlock(upb_rwlock_t *l) { (void)l; }
INLINE void upb_rwlock_unlock(upb_rwlock_t *l) { (void)l; }

#elif defined(UPB_USE_PTHREADS)

#include <pthread.h>

typedef struct {
  pthread_rwlock_t lock;
} upb_rwlock_t;

INLINE void upb_rwlock_init(upb_rwlock_t *l) {
  /* TODO: check return value. */
  pthread_rwlock_init(&l->lock, NULL);
}

INLINE void upb_rwlock_destroy(upb_rwlock_t *l) {
  /* TODO: check return value. */
  pthread_rwlock_destroy(&l->lock);
}

INLINE void upb_rwlock_rdlock(upb_rwlock_t *l) {
  /* TODO: check return value. */
  pthread_rwlock_rdlock(&l->lock);
}

INLINE void upb_rwlock_wrlock(upb_rwlock_t *l) {
  /* TODO: check return value. */
  pthread_rwlock_wrlock(&l->lock);
}

INLINE void upb_rwlock_unlock(upb_rwlock_t *l) {
  /* TODO: check return value. */
  pthread_rwlock_unlock(&l->lock);
}

#else
#error Reader/writer lock is not defined for your platform/CPU.  \
       Implement it or compile with UPB_THREAD_UNSAFE.
#endif

#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif  /* UPB_ATOMIC_H_ */
