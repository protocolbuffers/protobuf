// Copyright 2005, Google Inc.
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
//
// Authors: wan@google.com (Zhanyong Wan)
//
// Low-level types and utilities for porting Google Test to various
// platforms.  They are subject to change without notice.  DO NOT USE
// THEM IN USER CODE.

#ifndef GTEST_INCLUDE_GTEST_INTERNAL_GTEST_PORT_H_
#define GTEST_INCLUDE_GTEST_INTERNAL_GTEST_PORT_H_

#ifndef GTEST_NOT_MAC_FRAMEWORK_MODE
// Protobuf never uses gTest in "mac framework mode".
#define GTEST_NOT_MAC_FRAMEWORK_MODE
#endif

// The user can define the following macros in the build script to
// control Google Test's behavior:
//
//   GTEST_HAS_STD_STRING    - Define it to 1/0 to indicate that
//                             std::string does/doesn't work (Google Test can be
//                             used where std::string is unavailable).  Leave
//                             it undefined to let Google Test define it.
//   GTEST_HAS_GLOBAL_STRING - Define it to 1/0 to indicate that ::string
//                             is/isn't available (some systems define ::string,
//                             which is different to std::string).  Leave it
//                             undefined to let Google Test define it.

// This header defines the following utilities:
//
// Macros indicating the name of the Google C++ Testing Framework project:
//   GTEST_NAME              - a string literal of the project name.
//   GTEST_FLAG_PREFIX       - a string literal of the prefix all Google
//                             Test flag names share.
//   GTEST_FLAG_PREFIX_UPPER - a string literal of the prefix all Google
//                             Test flag names share, in upper case.
//
// Macros indicating the current platform:
//   GTEST_OS_LINUX    - defined iff compiled on Linux.
//   GTEST_OS_MAC      - defined iff compiled on Mac OS X.
//   GTEST_OS_WINDOWS  - defined iff compiled on Windows.
// Note that it is possible that none of the GTEST_OS_ macros are defined.
//
// Macros indicating available Google Test features:
//   GTEST_HAS_DEATH_TEST  - defined iff death tests are supported.
//
// Macros for basic C++ coding:
//   GTEST_AMBIGUOUS_ELSE_BLOCKER - for disabling a gcc warning.
//   GTEST_ATTRIBUTE_UNUSED  - declares that a class' instances don't have to
//                             be used.
//   GTEST_DISALLOW_COPY_AND_ASSIGN()  - disables copy ctor and operator=.
//   GTEST_MUST_USE_RESULT   - declares that a function's result must be used.
//
// Synchronization:
//   Mutex, MutexLock, ThreadLocal, GetThreadCount()
//                  - synchronization primitives.
//
// Template meta programming:
//   is_pointer     - as in TR1; needed on Symbian only.
//
// Smart pointers:
//   scoped_ptr     - as in TR2.
//
// Regular expressions:
//   RE             - a simple regular expression class using the POSIX
//                    Extended Regular Expression syntax.  Not available on
//                    Windows.
//
// Logging:
//   GTEST_LOG()    - logs messages at the specified severity level.
//   LogToStderr()  - directs all log messages to stderr.
//   FlushInfoLog() - flushes informational log messages.
//
// Stderr capturing:
//   CaptureStderr()     - starts capturing stderr.
//   GetCapturedStderr() - stops capturing stderr and returns the captured
//                         string.
//
// Integer types:
//   TypeWithSize   - maps an integer to a int type.
//   Int32, UInt32, Int64, UInt64, TimeInMillis
//                  - integers of known sizes.
//   BiggestInt     - the biggest signed integer type.
//
// Command-line utilities:
//   GTEST_FLAG()       - references a flag.
//   GTEST_DECLARE_*()  - declares a flag.
//   GTEST_DEFINE_*()   - defines a flag.
//   GetArgvs()         - returns the command line as a vector of strings.
//
// Environment variable utilities:
//   GetEnv()             - gets the value of an environment variable.
//   BoolFromGTestEnv()   - parses a bool environment variable.
//   Int32FromGTestEnv()  - parses an Int32 environment variable.
//   StringFromGTestEnv() - parses a string environment variable.

#include <sys/types.h>

#include <stdlib.h>
#include <stdio.h>

#define GTEST_NAME "Google Test"
#define GTEST_FLAG_PREFIX "gtest_"
#define GTEST_FLAG_PREFIX_UPPER "GTEST_"

// Determines the platform on which Google Test is compiled.
#ifdef _MSC_VER
// TODO(kenton):  GTEST_OS_WINDOWS is currently used to mean both "The OS is
//   Windows" and "The compiler is MSVC".  These meanings really should be
//   separated in order to better support Windows compilers other than MSVC.
//   Then again, the macro _WIN32 is already a good way to check for the first
//   case and _MSC_VER is a good way to check for the latter, so maybe
//   GTEST_OS_WINDOWS should be removed?
#define GTEST_OS_WINDOWS
#elif defined __APPLE__
#define GTEST_OS_MAC
#elif defined __linux__
#define GTEST_OS_LINUX
#endif  // _MSC_VER

// Determines whether ::std::string and ::string are available.

#ifndef GTEST_HAS_STD_STRING
// The user didn't tell us whether ::std::string is available, so we
// need to figure it out.

#ifdef GTEST_OS_WINDOWS
// Assumes that exceptions are enabled by default.
#ifndef _HAS_EXCEPTIONS
#define _HAS_EXCEPTIONS 1
#endif  // _HAS_EXCEPTIONS
// GTEST_HAS_EXCEPTIONS is non-zero iff exceptions are enabled.  It is
// always defined, while _HAS_EXCEPTIONS is defined only on Windows.
#define GTEST_HAS_EXCEPTIONS _HAS_EXCEPTIONS
// On Windows, we can use ::std::string if the compiler version is VS
// 2005 or above, or if exceptions are enabled.
#define GTEST_HAS_STD_STRING ((_MSC_VER >= 1400) || GTEST_HAS_EXCEPTIONS)
#else  // We are on Linux or Mac OS.
#define GTEST_HAS_EXCEPTIONS 0
#define GTEST_HAS_STD_STRING 1
#endif  // GTEST_OS_WINDOWS

#endif  // GTEST_HAS_STD_STRING

#ifndef GTEST_HAS_GLOBAL_STRING
// The user didn't tell us whether ::string is available, so we need
// to figure it out.

#define GTEST_HAS_GLOBAL_STRING 0

#endif  // GTEST_HAS_GLOBAL_STRING

#if GTEST_HAS_STD_STRING || GTEST_HAS_GLOBAL_STRING
#include <string>  // NOLINT
#endif  // GTEST_HAS_STD_STRING || GTEST_HAS_GLOBAL_STRING

#if GTEST_HAS_STD_STRING
#include <sstream>  // NOLINT
#else
#include <strstream>  // NOLINT
#endif  // GTEST_HAS_STD_STRING

// Determines whether to support death tests.
#if GTEST_HAS_STD_STRING && defined(GTEST_OS_LINUX)
#define GTEST_HAS_DEATH_TEST
// On some platforms, <regex.h> needs someone to define size_t, and
// won't compile if being #included first.  Therefore it's important
// that we #include it after <sys/types.h>.
#include <regex.h>
#include <vector>
#include <fcntl.h>
#include <sys/mman.h>
#endif  // GTEST_HAS_STD_STRING && defined(GTEST_OS_LINUX)

// Defines some utility macros.

// The GNU compiler emits a warning if nested "if" statements are followed by
// an "else" statement and braces are not used to explicitly disambiguate the
// "else" binding.  This leads to problems with code like:
//
//   if (gate)
//     ASSERT_*(condition) << "Some message";
//
// The "switch (0) case 0:" idiom is used to suppress this.
#ifdef __INTEL_COMPILER
#define GTEST_AMBIGUOUS_ELSE_BLOCKER
#else
#define GTEST_AMBIGUOUS_ELSE_BLOCKER switch (0) case 0:  // NOLINT
#endif

// Use this annotation at the end of a struct / class definition to
// prevent the compiler from optimizing away instances that are never
// used.  This is useful when all interesting logic happens inside the
// c'tor and / or d'tor.  Example:
//
//   struct Foo {
//     Foo() { ... }
//   } GTEST_ATTRIBUTE_UNUSED;
#ifdef __GNUC__
#define GTEST_ATTRIBUTE_UNUSED __attribute__ ((unused))
#else
#define GTEST_ATTRIBUTE_UNUSED
#endif  // GTEST_OS_WINDOWS || (GTEST_OS_LINUX && SWIG)

// A macro to disallow the evil copy constructor and operator= functions
// This should be used in the private: declarations for a class.
#define GTEST_DISALLOW_COPY_AND_ASSIGN(type)\
  type(const type &);\
  void operator=(const type &)

// Tell the compiler to warn about unused return values for functions declared
// with this macro.  The macro should be used on function declarations
// following the argument list:
//
//   Sprocket* AllocateSprocket() GTEST_MUST_USE_RESULT;
#if defined(__GNUC__) \
  && (__GNUC__ > 3 || (__GNUC__ == 3 && __GNUC_MINOR__ >= 4)) \
  && !defined(COMPILER_ICC)
#define GTEST_MUST_USE_RESULT __attribute__ ((warn_unused_result))
#else
#define GTEST_MUST_USE_RESULT
#endif  // (__GNUC__ > 3 || __GNUC__ == 3 && __GNUC_MINOR__ >= 4)

namespace testing {

class Message;

namespace internal {

class String;

// std::strstream is deprecated.  However, we have to use it on
// Windows as std::stringstream won't compile on Windows when
// exceptions are disabled.  We use std::stringstream on other
// platforms to avoid compiler warnings there.
#if GTEST_HAS_STD_STRING
typedef ::std::stringstream StrStream;
#else
typedef ::std::strstream StrStream;
#endif  // GTEST_HAS_STD_STRING

// Defines scoped_ptr.

// This implementation of scoped_ptr is PARTIAL - it only contains
// enough stuff to satisfy Google Test's need.
template <typename T>
class scoped_ptr {
 public:
  explicit scoped_ptr(T* p = NULL) : ptr_(p) {}
  ~scoped_ptr() { reset(); }

  T& operator*() const { return *ptr_; }
  T* operator->() const { return ptr_; }
  T* get() const { return ptr_; }

  T* release() {
    T* const ptr = ptr_;
    ptr_ = NULL;
    return ptr;
  }

  void reset(T* p = NULL) {
    if (p != ptr_) {
      if (sizeof(T) > 0) {  // Makes sure T is a complete type.
        delete ptr_;
      }
      ptr_ = p;
    }
  }
 private:
  T* ptr_;

  GTEST_DISALLOW_COPY_AND_ASSIGN(scoped_ptr);
};

#ifdef GTEST_HAS_DEATH_TEST

// Defines RE.  Currently only needed for death tests.

// A simple C++ wrapper for <regex.h>.  It uses the POSIX Enxtended
// Regular Expression syntax.
class RE {
 public:
  // Constructs an RE from a string.
#if GTEST_HAS_STD_STRING
  RE(const ::std::string& regex) { Init(regex.c_str()); }  // NOLINT
#endif  // GTEST_HAS_STD_STRING

#if GTEST_HAS_GLOBAL_STRING
  RE(const ::string& regex) { Init(regex.c_str()); }  // NOLINT
#endif  // GTEST_HAS_GLOBAL_STRING

  RE(const char* regex) { Init(regex); }  // NOLINT
  ~RE();

  // Returns the string representation of the regex.
  const char* pattern() const { return pattern_; }

  // Returns true iff str contains regular expression re.

  // TODO(wan): make PartialMatch() work when str contains NUL
  // characters.
#if GTEST_HAS_STD_STRING
  static bool PartialMatch(const ::std::string& str, const RE& re) {
    return PartialMatch(str.c_str(), re);
  }
#endif  // GTEST_HAS_STD_STRING

#if GTEST_HAS_GLOBAL_STRING
  static bool PartialMatch(const ::string& str, const RE& re) {
    return PartialMatch(str.c_str(), re);
  }
#endif  // GTEST_HAS_GLOBAL_STRING

  static bool PartialMatch(const char* str, const RE& re);

 private:
  void Init(const char* regex);

  // We use a const char* instead of a string, as Google Test may be used
  // where string is not available.  We also do not use Google Test's own
  // String type here, in order to simplify dependencies between the
  // files.
  const char* pattern_;
  regex_t regex_;
  bool is_valid_;
};

#endif  // GTEST_HAS_DEATH_TEST

// Defines logging utilities:
//   GTEST_LOG()    - logs messages at the specified severity level.
//   LogToStderr()  - directs all log messages to stderr.
//   FlushInfoLog() - flushes informational log messages.

enum GTestLogSeverity {
  GTEST_INFO,
  GTEST_WARNING,
  GTEST_ERROR,
  GTEST_FATAL
};

void GTestLog(GTestLogSeverity severity, const char* file,
              int line, const char* msg);

#define GTEST_LOG(severity, msg)\
    ::testing::internal::GTestLog(\
        ::testing::internal::GTEST_##severity, __FILE__, __LINE__, \
        (::testing::Message() << (msg)).GetString().c_str())

inline void LogToStderr() {}
inline void FlushInfoLog() { fflush(NULL); }

// Defines the stderr capturer:
//   CaptureStderr     - starts capturing stderr.
//   GetCapturedStderr - stops capturing stderr and returns the captured string.

#ifdef GTEST_HAS_DEATH_TEST

// A copy of all command line arguments.  Set by ParseGTestFlags().
extern ::std::vector<String> g_argvs;

void CaptureStderr();
// GTEST_HAS_DEATH_TEST implies we have ::std::string.
::std::string GetCapturedStderr();
const ::std::vector<String>& GetArgvs();

#endif  // GTEST_HAS_DEATH_TEST

// Defines synchronization primitives.

// A dummy implementation of synchronization primitives (mutex, lock,
// and thread-local variable).  Necessary for compiling Google Test where
// mutex is not supported - using Google Test in multiple threads is not
// supported on such platforms.

class Mutex {
 public:
  Mutex() {}
  explicit Mutex(int unused) {}
  void AssertHeld() const {}
  enum { NO_CONSTRUCTOR_NEEDED_FOR_STATIC_MUTEX = 0 };
};

// We cannot call it MutexLock directly as the ctor declaration would
// conflict with a macro named MutexLock, which is defined on some
// platforms.  Hence the typedef trick below.
class GTestMutexLock {
 public:
  explicit GTestMutexLock(Mutex*) {}  // NOLINT
};

typedef GTestMutexLock MutexLock;

template <typename T>
class ThreadLocal {
 public:
  T* pointer() { return &value_; }
  const T* pointer() const { return &value_; }
  const T& get() const { return value_; }
  void set(const T& value) { value_ = value; }
 private:
  T value_;
};

// There's no portable way to detect the number of threads, so we just
// return 0 to indicate that we cannot detect it.
// CHANGED FOR PROTOBUF:  The protobuf tests do not use multiple threads,
// so we know there is one thread.
inline size_t GetThreadCount() { return 1; }

// Defines tr1::is_pointer (only needed for Symbian).

#if defined(__SYMBIAN32__) || (defined (__DECCXX) && defined(__osf__))

// Symbian and HP C++ on Tru64 do not have tr1::type_traits, so we define
// our own is_pointer.  These are needed as these compilers cannot decide
// between const T& and const T* in a function template.

template <bool bool_value>
struct bool_constant {
  typedef bool_constant<bool_value> type;
  static const bool value = bool_value;
};
template <bool bool_value> const bool bool_constant<bool_value>::value;

typedef bool_constant<false> false_type;
typedef bool_constant<true> true_type;

template <typename T>
struct is_pointer : public false_type {};

template <typename T>
struct is_pointer<T*> : public true_type {};

#endif  // __SYMBIAN32__

// Defines BiggestInt as the biggest signed integer type the compiler
// supports.

#ifdef GTEST_OS_WINDOWS
typedef __int64 BiggestInt;
#else
typedef long long BiggestInt;  // NOLINT
#endif  // GTEST_OS_WINDOWS

// The maximum number a BiggestInt can represent.  This definition
// works no matter BiggestInt is represented in one's complement or
// two's complement.
//
// We cannot rely on numeric_limits in STL, as __int64 and long long
// are not part of standard C++ and numeric_limits doesn't need to be
// defined for them.
const BiggestInt kMaxBiggestInt =
    ~(static_cast<BiggestInt>(1) << (8*sizeof(BiggestInt) - 1));

// This template class serves as a compile-time function from size to
// type.  It maps a size in bytes to a primitive type with that
// size. e.g.
//
//   TypeWithSize<4>::UInt
//
// is typedef-ed to be unsigned int (unsigned integer made up of 4
// bytes).
//
// Such functionality should belong to STL, but I cannot find it
// there.
//
// Google Test uses this class in the implementation of floating-point
// comparison.
//
// For now it only handles UInt (unsigned int) as that's all Google Test
// needs.  Other types can be easily added in the future if need
// arises.
template <size_t size>
class TypeWithSize {
 public:
  // This prevents the user from using TypeWithSize<N> with incorrect
  // values of N.
  typedef void UInt;
};

// The specialization for size 4.
template <>
class TypeWithSize<4> {
 public:
  // unsigned int has size 4 in both gcc and MSVC.
  //
  // As base/basictypes.h doesn't compile on Windows, we cannot use
  // uint32, uint64, and etc here.
  typedef int Int;
  typedef unsigned int UInt;
};

// The specialization for size 8.
template <>
class TypeWithSize<8> {
 public:
#ifdef GTEST_OS_WINDOWS
  typedef __int64 Int;
  typedef unsigned __int64 UInt;
#else
  typedef long long Int;  // NOLINT
  typedef unsigned long long UInt;  // NOLINT
#endif  // GTEST_OS_WINDOWS
};

// Integer types of known sizes.
typedef TypeWithSize<4>::Int Int32;
typedef TypeWithSize<4>::UInt UInt32;
typedef TypeWithSize<8>::Int Int64;
typedef TypeWithSize<8>::UInt UInt64;
typedef TypeWithSize<8>::Int TimeInMillis;  // Represents time in milliseconds.

// Utilities for command line flags and environment variables.

// A wrapper for getenv() that works on Linux, Windows, and Mac OS.
inline const char* GetEnv(const char* name) {
#ifdef _WIN32_WCE  // We are on Windows CE.
  // CE has no environment variables.
  return NULL;
#elif defined(GTEST_OS_WINDOWS)  // We are on Windows proper.
  // MSVC 8 deprecates getenv(), so we want to suppress warning 4996
  // (deprecated function) there.
#pragma warning(push)          // Saves the current warning state.
#pragma warning(disable:4996)  // Temporarily disables warning 4996.
  return getenv(name);
#pragma warning(pop)           // Restores the warning state.
#else  // We are on Linux or Mac OS.
  return getenv(name);
#endif
}

// Macro for referencing flags.
#define GTEST_FLAG(name) FLAGS_gtest_##name

// Macros for declaring flags.
#define GTEST_DECLARE_bool(name) extern bool GTEST_FLAG(name)
#define GTEST_DECLARE_int32(name) \
    extern ::testing::internal::Int32 GTEST_FLAG(name)
#define GTEST_DECLARE_string(name) \
    extern ::testing::internal::String GTEST_FLAG(name)

// Macros for defining flags.
#define GTEST_DEFINE_bool(name, default_val, doc) \
    bool GTEST_FLAG(name) = (default_val)
#define GTEST_DEFINE_int32(name, default_val, doc) \
    ::testing::internal::Int32 GTEST_FLAG(name) = (default_val)
#define GTEST_DEFINE_string(name, default_val, doc) \
    ::testing::internal::String GTEST_FLAG(name) = (default_val)

// Parses 'str' for a 32-bit signed integer.  If successful, writes the result
// to *value and returns true; otherwise leaves *value unchanged and returns
// false.
// TODO(chandlerc): Find a better way to refactor flag and environment parsing
// out of both gtest-port.cc and gtest.cc to avoid exporting this utility
// function.
bool ParseInt32(const Message& src_text, const char* str, Int32* value);

// Parses a bool/Int32/string from the environment variable
// corresponding to the given Google Test flag.
bool BoolFromGTestEnv(const char* flag, bool default_val);
Int32 Int32FromGTestEnv(const char* flag, Int32 default_val);
const char* StringFromGTestEnv(const char* flag, const char* default_val);

}  // namespace internal
}  // namespace testing

#endif  // GTEST_INCLUDE_GTEST_INTERNAL_GTEST_PORT_H_
