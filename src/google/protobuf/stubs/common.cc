// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
// http://code.google.com/p/protobuf/
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

// Mutex which protects log_silencer_count_.  We provide a static function to
// get it so that it is initialized on first use, which be during
// initialization time.  If we just allocated it as a global variable, it might
// not be initialized before someone tries to use it.
static Mutex* LogSilencerMutex() {
  static Mutex* log_silencer_count_mutex_ = new Mutex;
  return log_silencer_count_mutex_;
}

// Forces the above mutex to be initialized during startup.  This way we don't
// have to worry about the initialization itself being thread-safe, since no
// threads should exist yet at startup time.  (Otherwise we'd have no way to
// make things thread-safe here because we'd need a Mutex for that, and we'd
// have no way to construct one safely!)
static struct LogSilencerMutexInitializer {
  LogSilencerMutexInitializer() {
    LogSilencerMutex();
  }
} log_silencer_mutex_initializer;


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
    MutexLock lock(internal::LogSilencerMutex());
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
  MutexLock lock(internal::LogSilencerMutex());
  ++internal::log_silencer_count_;
};

LogSilencer::~LogSilencer() {
  MutexLock lock(internal::LogSilencerMutex());
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
