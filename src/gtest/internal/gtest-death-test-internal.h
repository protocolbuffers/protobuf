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
// Authors: wan@google.com (Zhanyong Wan), eefacm@gmail.com (Sean Mcafee)
//
// The Google C++ Testing Framework (Google Test)
//
// This header file defines internal utilities needed for implementing
// death tests.  They are subject to change without notice.

#ifndef GTEST_INCLUDE_GTEST_INTERNAL_GTEST_DEATH_TEST_INTERNAL_H_
#define GTEST_INCLUDE_GTEST_INTERNAL_GTEST_DEATH_TEST_INTERNAL_H_

#include <gtest/internal/gtest-internal.h>

namespace testing {
namespace internal {

GTEST_DECLARE_string(internal_run_death_test);

// Names of the flags (needed for parsing Google Test flags).
const char kDeathTestStyleFlag[] = "death_test_style";
const char kInternalRunDeathTestFlag[] = "internal_run_death_test";

#ifdef GTEST_HAS_DEATH_TEST

// DeathTest is a class that hides much of the complexity of the
// GTEST_DEATH_TEST macro.  It is abstract; its static Create method
// returns a concrete class that depends on the prevailing death test
// style, as defined by the --gtest_death_test_style and/or
// --gtest_internal_run_death_test flags.

// In describing the results of death tests, these terms are used with
// the corresponding definitions:
//
// exit status:  The integer exit information in the format specified
//               by wait(2)
// exit code:    The integer code passed to exit(3), _exit(2), or
//               returned from main()
class DeathTest {
 public:
  // Create returns false if there was an error determining the
  // appropriate action to take for the current death test; for example,
  // if the gtest_death_test_style flag is set to an invalid value.
  // The LastMessage method will return a more detailed message in that
  // case.  Otherwise, the DeathTest pointer pointed to by the "test"
  // argument is set.  If the death test should be skipped, the pointer
  // is set to NULL; otherwise, it is set to the address of a new concrete
  // DeathTest object that controls the execution of the current test.
  static bool Create(const char* statement, const RE* regex,
                     const char* file, int line, DeathTest** test);
  DeathTest();
  virtual ~DeathTest() { }

  // A helper class that aborts a death test when it's deleted.
  class ReturnSentinel {
   public:
    explicit ReturnSentinel(DeathTest* test) : test_(test) { }
    ~ReturnSentinel() { test_->Abort(TEST_ENCOUNTERED_RETURN_STATEMENT); }
   private:
    DeathTest* const test_;
    GTEST_DISALLOW_COPY_AND_ASSIGN(ReturnSentinel);
  } GTEST_ATTRIBUTE_UNUSED;

  // An enumeration of possible roles that may be taken when a death
  // test is encountered.  EXECUTE means that the death test logic should
  // be executed immediately.  OVERSEE means that the program should prepare
  // the appropriate environment for a child process to execute the death
  // test, then wait for it to complete.
  enum TestRole { OVERSEE_TEST, EXECUTE_TEST };

  // An enumeration of the two reasons that a test might be aborted.
  enum AbortReason { TEST_ENCOUNTERED_RETURN_STATEMENT, TEST_DID_NOT_DIE };

  // Assumes one of the above roles.
  virtual TestRole AssumeRole() = 0;

  // Waits for the death test to finish and returns its status.
  virtual int Wait() = 0;

  // Returns true if the death test passed; that is, the test process
  // exited during the test, its exit status matches a user-supplied
  // predicate, and its stderr output matches a user-supplied regular
  // expression.
  // The user-supplied predicate may be a macro expression rather
  // than a function pointer or functor, or else Wait and Passed could
  // be combined.
  virtual bool Passed(bool exit_status_ok) = 0;

  // Signals that the death test did not die as expected.
  virtual void Abort(AbortReason reason) = 0;

  // Returns a human-readable outcome message regarding the outcome of
  // the last death test.
  static const char* LastMessage();

 private:
  GTEST_DISALLOW_COPY_AND_ASSIGN(DeathTest);
};

// Factory interface for death tests.  May be mocked out for testing.
class DeathTestFactory {
 public:
  virtual ~DeathTestFactory() { }
  virtual bool Create(const char* statement, const RE* regex,
                      const char* file, int line, DeathTest** test) = 0;
};

// A concrete DeathTestFactory implementation for normal use.
class DefaultDeathTestFactory : public DeathTestFactory {
 public:
  virtual bool Create(const char* statement, const RE* regex,
                      const char* file, int line, DeathTest** test);
};

// Returns true if exit_status describes a process that was terminated
// by a signal, or exited normally with a nonzero exit code.
bool ExitedUnsuccessfully(int exit_status);

// This macro is for implementing ASSERT_DEATH*, EXPECT_DEATH*,
// ASSERT_EXIT*, and EXPECT_EXIT*.
#define GTEST_DEATH_TEST(statement, predicate, regex, fail) \
  GTEST_AMBIGUOUS_ELSE_BLOCKER \
  if (true) { \
    const ::testing::internal::RE& gtest_regex = (regex); \
    ::testing::internal::DeathTest* gtest_dt; \
    if (!::testing::internal::DeathTest::Create(#statement, &gtest_regex, \
        __FILE__, __LINE__, &gtest_dt)) { \
      goto GTEST_CONCAT_TOKEN(gtest_label_, __LINE__); \
    } \
    if (gtest_dt != NULL) { \
      ::testing::internal::scoped_ptr< ::testing::internal::DeathTest> \
          gtest_dt_ptr(gtest_dt); \
      switch (gtest_dt->AssumeRole()) { \
        case ::testing::internal::DeathTest::OVERSEE_TEST: \
          if (!gtest_dt->Passed(predicate(gtest_dt->Wait()))) { \
            goto GTEST_CONCAT_TOKEN(gtest_label_, __LINE__); \
          } \
          break; \
        case ::testing::internal::DeathTest::EXECUTE_TEST: { \
          ::testing::internal::DeathTest::ReturnSentinel \
              gtest_sentinel(gtest_dt); \
          { statement; } \
          gtest_dt->Abort(::testing::internal::DeathTest::TEST_DID_NOT_DIE); \
          break; \
        } \
      } \
    } \
  } else \
    GTEST_CONCAT_TOKEN(gtest_label_, __LINE__): \
      fail(::testing::internal::DeathTest::LastMessage())
// The symbol "fail" here expands to something into which a message
// can be streamed.

// A struct representing the parsed contents of the
// --gtest_internal_run_death_test flag, as it existed when
// RUN_ALL_TESTS was called.
struct InternalRunDeathTestFlag {
  String file;
  int line;
  int index;
  int status_fd;
};

// Returns a newly created InternalRunDeathTestFlag object with fields
// initialized from the GTEST_FLAG(internal_run_death_test) flag if
// the flag is specified; otherwise returns NULL.
InternalRunDeathTestFlag* ParseInternalRunDeathTestFlag();

#endif  // GTEST_HAS_DEATH_TEST

}  // namespace internal
}  // namespace testing

#endif  // GTEST_INCLUDE_GTEST_INTERNAL_GTEST_DEATH_TEST_INTERNAL_H_
