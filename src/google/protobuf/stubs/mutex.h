// Copyright (c) 2006, Google Inc.
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#ifndef GOOGLE_PROTOBUF_STUBS_MUTEX_H_
#define GOOGLE_PROTOBUF_STUBS_MUTEX_H_

#include <mutex>

#include <google/protobuf/stubs/macros.h>

// Define thread-safety annotations for use below, if we are building with
// Clang.
#if defined(__clang__) && !defined(SWIG)
#define GOOGLE_PROTOBUF_ACQUIRE(...) \
  __attribute__((acquire_capability(__VA_ARGS__)))
#define GOOGLE_PROTOBUF_RELEASE(...) \
  __attribute__((release_capability(__VA_ARGS__)))
#else
#define GOOGLE_PROTOBUF_ACQUIRE(...)
#define GOOGLE_PROTOBUF_RELEASE(...)
#endif

// ===================================================================
// emulates google3/base/mutex.h
namespace google {
namespace protobuf {
namespace internal {

#define GOOGLE_PROTOBUF_LINKER_INITIALIZED

// Mutex is a natural type to wrap. As both google and other organization have
// specialized mutexes. gRPC also provides an injection mechanism for custom
// mutexes.
class LIBPROTOBUF_EXPORT WrappedMutex {
 public:
  WrappedMutex() = default;
  void Lock() GOOGLE_PROTOBUF_ACQUIRE() { mu_.lock(); }
  void Unlock() GOOGLE_PROTOBUF_RELEASE() { mu_.unlock(); }
  // Crash if this Mutex is not held exclusively by this thread.
  // May fail to crash when it should; will never crash when it should not.
  void AssertHeld() const {}

 private:
  std::mutex mu_;
};

using Mutex = WrappedMutex;

// MutexLock(mu) acquires mu when constructed and releases it when destroyed.
class LIBPROTOBUF_EXPORT MutexLock {
 public:
  explicit MutexLock(Mutex *mu) : mu_(mu) { this->mu_->Lock(); }
  ~MutexLock() { this->mu_->Unlock(); }
 private:
  Mutex *const mu_;
  GOOGLE_DISALLOW_EVIL_CONSTRUCTORS(MutexLock);
};

// TODO(kenton):  Implement these?  Hard to implement portably.
typedef MutexLock ReaderMutexLock;
typedef MutexLock WriterMutexLock;

// MutexLockMaybe is like MutexLock, but is a no-op when mu is nullptr.
class LIBPROTOBUF_EXPORT MutexLockMaybe {
 public:
  explicit MutexLockMaybe(Mutex *mu) :
    mu_(mu) { if (this->mu_ != nullptr) { this->mu_->Lock(); } }
  ~MutexLockMaybe() { if (this->mu_ != nullptr) { this->mu_->Unlock(); } }
 private:
  Mutex *const mu_;
  GOOGLE_DISALLOW_EVIL_CONSTRUCTORS(MutexLockMaybe);
};

#if defined(GOOGLE_PROTOBUF_NO_THREADLOCAL)
template<typename T>
class ThreadLocalStorage {
 public:
  ThreadLocalStorage() {
    pthread_key_create(&key_, &ThreadLocalStorage::Delete);
  }
  ~ThreadLocalStorage() {
    pthread_key_delete(key_);
  }
  T* Get() {
    T* result = static_cast<T*>(pthread_getspecific(key_));
    if (result == nullptr) {
      result = new T();
      pthread_setspecific(key_, result);
    }
    return result;
  }
 private:
  static void Delete(void* value) {
    delete static_cast<T*>(value);
  }
  pthread_key_t key_;

  GOOGLE_DISALLOW_EVIL_CONSTRUCTORS(ThreadLocalStorage);
};
#endif

}  // namespace internal

// We made these internal so that they would show up as such in the docs,
// but we don't want to stick "internal::" in front of them everywhere.
using internal::Mutex;
using internal::MutexLock;
using internal::ReaderMutexLock;
using internal::WriterMutexLock;
using internal::MutexLockMaybe;

}  // namespace protobuf
}  // namespace google

#undef GOOGLE_PROTOBUF_ACQUIRE
#undef GOOGLE_PROTOBUF_RELEASE

#endif  // GOOGLE_PROTOBUF_STUBS_MUTEX_H_
