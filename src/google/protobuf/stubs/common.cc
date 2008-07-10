// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.
// http://code.google.com/p/protobuf/
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

// Author: kenton@google.com (Kenton Varda)

#include <google/protobuf/stubs/common.h>
#include <google/protobuf/stubs/strutil.h>
#include <google/protobuf/stubs/substitute.h>
#include <stdio.h>
#include <errno.h>

#include "config.h"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN  // We only need minimal includes
#include <windows.h>
#elif defined(HAVE_PTHREAD)
#include <pthread.h>
#else
#error "No suitable threading library available."
#endif

namespace google {
namespace protobuf {

namespace internal {

void VerifyVersion(int headerVersion,
                   int minLibraryVersion,
                   const char* filename) {
  if (GOOGLE_PROTOBUF_VERSION < minLibraryVersion) {
    // Library is too old for headers.
    GOOGLE_LOG(FATAL)
      << "This program requires version " << VersionString(minLibraryVersion)
      << " of the Protocol Buffer runtime library, but the installed version "
         "is " << VersionString(GOOGLE_PROTOBUF_VERSION) << ".  Please update "
         "your library.  If you compiled the program yourself, make sure that "
         "your headers are from the same version of Protocol Buffers as your "
         "link-time library.  (Version verification failed in \""
      << filename << "\".)";
  }
  if (headerVersion < kMinHeaderVersionForLibrary) {
    // Headers are too old for library.
    GOOGLE_LOG(FATAL)
      << "This program was compiled against version "
      << VersionString(headerVersion) << " of the Protocol Buffer runtime "
         "library, which is not compatible with the installed version ("
      << VersionString(GOOGLE_PROTOBUF_VERSION) <<  ").  Contact the program "
         "author for an update.  If you compiled the program yourself, make "
         "sure that your headers are from the same version of Protocol Buffers "
         "as your link-time library.  (Version verification failed in \""
      << filename << "\".)";
  }
}

string VersionString(int version) {
  int major = version / 1000000;
  int minor = (version / 1000) % 1000;
  int micro = version % 1000;

  return strings::Substitute("$0.$1.$2", major, minor, micro);
}

}  // namespace internal

// ===================================================================
// emulates google3/base/logging.cc

namespace internal {

void DefaultLogHandler(LogLevel level, const char* filename, int line,
                       const string& message) {
  static const char* level_names[] = { "INFO", "WARNING", "ERROR", "FATAL" };

  // We use fprintf() instead of cerr because we want this to work at static
  // initialization time.
  fprintf(stderr, "libprotobuf %s %s:%d] %s\n",
          level_names[level], filename, line, message.c_str());
  fflush(stderr);  // Needed on MSVC.
}

void NullLogHandler(LogLevel level, const char* filename, int line,
                    const string& message) {
  // Nothing.
}

static LogHandler* log_handler_ = &DefaultLogHandler;
static int log_silencer_count_ = 0;
static Mutex log_silencer_count_mutex_;

static string SimpleCtoa(char c) { return string(1, c); }

#undef DECLARE_STREAM_OPERATOR
#define DECLARE_STREAM_OPERATOR(TYPE, TOSTRING)                     \
  LogMessage& LogMessage::operator<<(TYPE value) {                  \
    message_ += TOSTRING(value);                                    \
    return *this;                                                   \
  }

DECLARE_STREAM_OPERATOR(const string&, )
DECLARE_STREAM_OPERATOR(const char*  , )
DECLARE_STREAM_OPERATOR(char         , SimpleCtoa)
DECLARE_STREAM_OPERATOR(int          , SimpleItoa)
DECLARE_STREAM_OPERATOR(uint         , SimpleItoa)
DECLARE_STREAM_OPERATOR(double       , SimpleDtoa)
#undef DECLARE_STREAM_OPERATOR

LogMessage::LogMessage(LogLevel level, const char* filename, int line)
  : level_(level), filename_(filename), line_(line) {}
LogMessage::~LogMessage() {}

void LogMessage::Finish() {
  bool suppress = false;

  if (level_ != LOGLEVEL_FATAL) {
    MutexLock lock(&internal::log_silencer_count_mutex_);
    suppress = internal::log_silencer_count_ > 0;
  }

  if (!suppress) {
    internal::log_handler_(level_, filename_, line_, message_);
  }

  if (level_ == LOGLEVEL_FATAL) {
    abort();
  }
}

void LogFinisher::operator=(LogMessage& other) {
  other.Finish();
}

}  // namespace internal

LogHandler* SetLogHandler(LogHandler* new_func) {
  LogHandler* old = internal::log_handler_;
  if (old == &internal::NullLogHandler) {
    old = NULL;
  }
  if (new_func == NULL) {
    internal::log_handler_ = &internal::NullLogHandler;
  } else {
    internal::log_handler_ = new_func;
  }
  return old;
}

LogSilencer::LogSilencer() {
  MutexLock lock(&internal::log_silencer_count_mutex_);
  ++internal::log_silencer_count_;
};

LogSilencer::~LogSilencer() {
  MutexLock lock(&internal::log_silencer_count_mutex_);
  --internal::log_silencer_count_;
};

// ===================================================================
// emulates google3/base/callback.cc

Closure::~Closure() {}

namespace internal { FunctionClosure0::~FunctionClosure0() {} }

void DoNothing() {}

// ===================================================================
// emulates google3/base/mutex.cc

#ifdef _WIN32

struct Mutex::Internal {
  CRITICAL_SECTION mutex;
#ifndef NDEBUG
  // Used only to implement AssertHeld().
  DWORD thread_id;
#endif
};

Mutex::Mutex()
  : mInternal(new Internal) {
  InitializeCriticalSection(&mInternal->mutex);
}

Mutex::~Mutex() {
  DeleteCriticalSection(&mInternal->mutex);
  delete mInternal;
}

void Mutex::Lock() {
  EnterCriticalSection(&mInternal->mutex);
#ifndef NDEBUG
  mInternal->thread_id = GetCurrentThreadId();
#endif
}

void Mutex::Unlock() {
#ifndef NDEBUG
  mInternal->thread_id = 0;
#endif
  LeaveCriticalSection(&mInternal->mutex);
}

void Mutex::AssertHeld() {
#ifndef NDEBUG
  GOOGLE_DCHECK_EQ(mInternal->thread_id, GetCurrentThreadId());
#endif
}

#elif defined(HAVE_PTHREAD)

struct Mutex::Internal {
  pthread_mutex_t mutex;
};

Mutex::Mutex()
  : mInternal(new Internal) {
  pthread_mutex_init(&mInternal->mutex, NULL);
}

Mutex::~Mutex() {
  pthread_mutex_destroy(&mInternal->mutex);
  delete mInternal;
}

void Mutex::Lock() {
  int result = pthread_mutex_lock(&mInternal->mutex);
  if (result != 0) {
    GOOGLE_LOG(FATAL) << "pthread_mutex_lock: " << strerror(result);
  }
}

void Mutex::Unlock() {
  int result = pthread_mutex_unlock(&mInternal->mutex);
  if (result != 0) {
    GOOGLE_LOG(FATAL) << "pthread_mutex_unlock: " << strerror(result);
  }
}

void Mutex::AssertHeld() {
  // pthreads dosn't provide a way to check which thread holds the mutex.
  // TODO(kenton):  Maybe keep track of locking thread ID like with WIN32?
}

#endif

}  // namespace protobuf
}  // namespace google
