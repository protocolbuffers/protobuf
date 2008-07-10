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
// Author: wan@google.com (Zhanyong Wan)
//
// The Google C++ Testing Framework (Google Test)
//
// This header file defines the public API for death tests.  It is
// #included by gtest.h so a user doesn't need to include this
// directly.

#ifndef GTEST_INCLUDE_GTEST_GTEST_DEATH_TEST_H_
#define GTEST_INCLUDE_GTEST_GTEST_DEATH_TEST_H_

#include <gtest/internal/gtest-death-test-internal.h>

namespace testing {

// This flag controls the style of death tests.  Valid values are "threadsafe",
// meaning that the death test child process will re-execute the test binary
// from the start, running only a single death test, or "fast",
// meaning that the child process will execute the test logic immediately
// after forking.
GTEST_DECLARE_string(death_test_style);

#ifdef GTEST_HAS_DEATH_TEST

// The following macros are useful for writing death tests.

// Here's what happens when an ASSERT_DEATH* or EXPECT_DEATH* is
// executed:
//
//   1. The assertion fails immediately if there are more than one
//   active threads.  This is because it's safe to fork() only when
//   there is a single thread.
//
//   2. The parent process forks a sub-process and runs the death test
//   in it; the sub-process exits with code 0 at the end of the death
//   test, if it hasn't exited already.
//
//   3. The parent process waits for the sub-process to terminate.
//
//   4. The parent process checks the exit code and error message of
//   the sub-process.
//
// Note:
//
// It's not safe to call exit() if the current process is forked from
// a multi-threaded process, so people usually call _exit() instead in
// such a case.  However, we are not concerned with this as we run
// death tests only when there is a single thread.  Since exit() has a
// cleaner semantics (it also calls functions registered with atexit()
// and on_exit()), this macro calls exit() instead of _exit() to
// terminate the child process.
//
// Examples:
//
//   ASSERT_DEATH(server.SendMessage(56, "Hello"), "Invalid port number");
//   for (int i = 0; i < 5; i++) {
//     EXPECT_DEATH(server.ProcessRequest(i),
//                  "Invalid request .* in ProcessRequest()")
//         << "Failed to die on request " << i);
//   }
//
//   ASSERT_EXIT(server.ExitNow(), ::testing::ExitedWithCode(0), "Exiting");
//
//   bool KilledBySIGHUP(int exit_code) {
//     return WIFSIGNALED(exit_code) && WTERMSIG(exit_code) == SIGHUP;
//   }
//
//   ASSERT_EXIT(client.HangUpServer(), KilledBySIGHUP, "Hanging up!");

// Asserts that a given statement causes the program to exit, with an
// integer exit status that satisfies predicate, and emitting error output
// that matches regex.
#define ASSERT_EXIT(statement, predicate, regex) \
  GTEST_DEATH_TEST(statement, predicate, regex, GTEST_FATAL_FAILURE)

// Like ASSERT_EXIT, but continues on to successive tests in the
// test case, if any:
#define EXPECT_EXIT(statement, predicate, regex) \
  GTEST_DEATH_TEST(statement, predicate, regex, GTEST_NONFATAL_FAILURE)

// Asserts that a given statement causes the program to exit, either by
// explicitly exiting with a nonzero exit code or being killed by a
// signal, and emitting error output that matches regex.
#define ASSERT_DEATH(statement, regex) \
  ASSERT_EXIT(statement, ::testing::internal::ExitedUnsuccessfully, regex)

// Like ASSERT_DEATH, but continues on to successive tests in the
// test case, if any:
#define EXPECT_DEATH(statement, regex) \
  EXPECT_EXIT(statement, ::testing::internal::ExitedUnsuccessfully, regex)

// Two predicate classes that can be used in {ASSERT,EXPECT}_EXIT*:

// Tests that an exit code describes a normal exit with a given exit code.
class ExitedWithCode {
 public:
  explicit ExitedWithCode(int exit_code);
  bool operator()(int exit_status) const;
 private:
  const int exit_code_;
};

// Tests that an exit code describes an exit due to termination by a
// given signal.
class KilledBySignal {
 public:
  explicit KilledBySignal(int signum);
  bool operator()(int exit_status) const;
 private:
  const int signum_;
};

// EXPECT_DEBUG_DEATH asserts that the given statements die in debug mode.
// The death testing framework causes this to have interesting semantics,
// since the sideeffects of the call are only visible in opt mode, and not
// in debug mode.
//
// In practice, this can be used to test functions that utilize the
// LOG(DFATAL) macro using the following style:
//
// int DieInDebugOr12(int* sideeffect) {
//   if (sideeffect) {
//     *sideeffect = 12;
//   }
//   LOG(DFATAL) << "death";
//   return 12;
// }
//
// TEST(TestCase, TestDieOr12WorksInDgbAndOpt) {
//   int sideeffect = 0;
//   // Only asserts in dbg.
//   EXPECT_DEBUG_DEATH(DieInDebugOr12(&sideeffect), "death");
//
// #ifdef NDEBUG
//   // opt-mode has sideeffect visible.
//   EXPECT_EQ(12, sideeffect);
// #else
//   // dbg-mode no visible sideeffect.
//   EXPECT_EQ(0, sideeffect);
// #endif
// }
//
// This will assert that DieInDebugReturn12InOpt() crashes in debug
// mode, usually due to a DCHECK or LOG(DFATAL), but returns the
// appropriate fallback value (12 in this case) in opt mode. If you
// need to test that a function has appropriate side-effects in opt
// mode, include assertions against the side-effects.  A general
// pattern for this is:
//
// EXPECT_DEBUG_DEATH({
//   // Side-effects here will have an effect after this statement in
//   // opt mode, but none in debug mode.
//   EXPECT_EQ(12, DieInDebugOr12(&sideeffect));
// }, "death");
//
#ifdef NDEBUG

#define EXPECT_DEBUG_DEATH(statement, regex) \
  do { statement; } while (false)

#define ASSERT_DEBUG_DEATH(statement, regex) \
  do { statement; } while (false)

#else

#define EXPECT_DEBUG_DEATH(statement, regex) \
  EXPECT_DEATH(statement, regex)

#define ASSERT_DEBUG_DEATH(statement, regex) \
  ASSERT_DEATH(statement, regex)

#endif  // NDEBUG for EXPECT_DEBUG_DEATH
#endif  // GTEST_HAS_DEATH_TEST
}  // namespace testing

#endif  // GTEST_INCLUDE_GTEST_GTEST_DEATH_TEST_H_
