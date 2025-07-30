// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef UPB_PORT_THREAD_H_
#define UPB_PORT_THREAD_H_

#include "upb/port/def.inc"

#if !defined(NDEBUG) && !defined(THREAD_SANITIZER)
// Debug build: Use real locks if available.
#if defined(UPB_USE_PTHREADS)
#include <pthread.h>
static pthread_mutex_t upb_global_lock = PTHREAD_MUTEX_INITIALIZER;
UPB_INLINE void upb_lock_global() { pthread_mutex_lock(&upb_global_lock); }
UPB_INLINE void upb_unlock_global() { pthread_mutex_unlock(&upb_global_lock); }
#elif defined(UPB_USE_C11_THREADS)
#include <threads.h>
static mtx_t upb_global_lock;
static once_flag upb_global_lock_init_flag = ONCE_FLAG_INIT;
static void upb_initialize_global_lock() {
  if (mtx_init(&upb_global_lock, mtx_plain) != thrd_success) abort();
}
UPB_INLINE void upb_lock_global() {
  call_once(&upb_global_lock_init_flag, upb_initialize_global_lock);
  mtx_lock(&upb_global_lock);
}
UPB_INLINE void upb_unlock_global() { mtx_unlock(&upb_global_lock); }
#else
// No-op locks for debug builds when no threading is supported.
UPB_INLINE void upb_lock_global() {}
UPB_INLINE void upb_unlock_global() {}
#endif  // defined(UPB_USE_PTHREADS) || defined(UPB_USE_C11_THREADS)
#else
// Release build: Always use no-op locks.
#define upb_lock_global() ((void)0)
#define upb_unlock_global() ((void)0)
#endif  // NDEBUG && THREAD_SANITIZER

#include "upb/port/undef.inc"

#endif  // UPB_PORT_THREAD_H_
