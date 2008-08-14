// Copyright 2007, Google Inc.
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
// Utilities for testing Google Test itself and code that uses Google Test
// (e.g. frameworks built on top of Google Test).

#ifndef GTEST_INCLUDE_GTEST_GTEST_SPI_H_
#define GTEST_INCLUDE_GTEST_GTEST_SPI_H_

#include <gtest/gtest.h>

namespace testing {

// A copyable object representing the result of a test part (i.e. an
// assertion or an explicit FAIL(), ADD_FAILURE(), or SUCCESS()).
//
// Don't inherit from TestPartResult as its destructor is not virtual.
class TestPartResult {
 public:
  // C'tor.  TestPartResult does NOT have a default constructor.
  // Always use this constructor (with parameters) to create a
  // TestPartResult object.
  TestPartResult(TestPartResultType type,
                 const char* file_name,
                 int line_number,
                 const char* message)
      : type_(type),
        file_name_(file_name),
        line_number_(line_number),
        message_(message) {
  }

  // Gets the outcome of the test part.
  TestPartResultType type() const { return type_; }

  // Gets the name of the source file where the test part took place, or
  // NULL if it's unknown.
  const char* file_name() const { return file_name_.c_str(); }

  // Gets the line in the source file where the test part took place,
  // or -1 if it's unknown.
  int line_number() const { return line_number_; }

  // Gets the message associated with the test part.
  const char* message() const { return message_.c_str(); }

  // Returns true iff the test part passed.
  bool passed() const { return type_ == TPRT_SUCCESS; }

  // Returns true iff the test part failed.
  bool failed() const { return type_ != TPRT_SUCCESS; }

  // Returns true iff the test part non-fatally failed.
  bool nonfatally_failed() const { return type_ == TPRT_NONFATAL_FAILURE; }

  // Returns true iff the test part fatally failed.
  bool fatally_failed() const { return type_ == TPRT_FATAL_FAILURE; }
 private:
  TestPartResultType type_;

  // The name of the source file where the test part took place, or
  // NULL if the source file is unknown.
  internal::String file_name_;
  // The line in the source file where the test part took place, or -1
  // if the line number is unknown.
  int line_number_;
  internal::String message_;  // The test failure message.
};

// Prints a TestPartResult object.
std::ostream& operator<<(std::ostream& os, const TestPartResult& result);

// An array of TestPartResult objects.
//
// We define this class as we cannot use STL containers when compiling
// Google Test with MSVC 7.1 and exceptions disabled.
//
// Don't inherit from TestPartResultArray as its destructor is not
// virtual.
class TestPartResultArray {
 public:
  TestPartResultArray();
  ~TestPartResultArray();

  // Appends the given TestPartResult to the array.
  void Append(const TestPartResult& result);

  // Returns the TestPartResult at the given index (0-based).
  const TestPartResult& GetTestPartResult(int index) const;

  // Returns the number of TestPartResult objects in the array.
  int size() const;
 private:
  // Internally we use a list to simulate the array.  Yes, this means
  // that random access is O(N) in time, but it's OK for its purpose.
  internal::List<TestPartResult>* const list_;

  GTEST_DISALLOW_COPY_AND_ASSIGN(TestPartResultArray);
};

// This interface knows how to report a test part result.
class TestPartResultReporterInterface {
 public:
  virtual ~TestPartResultReporterInterface() {}

  virtual void ReportTestPartResult(const TestPartResult& result) = 0;
};

// This helper class can be used to mock out Google Test failure reporting
// so that we can test Google Test or code that builds on Google Test.
//
// An object of this class appends a TestPartResult object to the
// TestPartResultArray object given in the constructor whenever a
// Google Test failure is reported.
class ScopedFakeTestPartResultReporter
    : public TestPartResultReporterInterface {
 public:
  // The c'tor sets this object as the test part result reporter used
  // by Google Test.  The 'result' parameter specifies where to report the
  // results.
  explicit ScopedFakeTestPartResultReporter(TestPartResultArray* result);

  // The d'tor restores the previous test part result reporter.
  virtual ~ScopedFakeTestPartResultReporter();

  // Appends the TestPartResult object to the TestPartResultArray
  // received in the constructor.
  //
  // This method is from the TestPartResultReporterInterface
  // interface.
  virtual void ReportTestPartResult(const TestPartResult& result);
 private:
  TestPartResultReporterInterface* const old_reporter_;
  TestPartResultArray* const result_;

  GTEST_DISALLOW_COPY_AND_ASSIGN(ScopedFakeTestPartResultReporter);
};

namespace internal {

// A helper class for implementing EXPECT_FATAL_FAILURE() and
// EXPECT_NONFATAL_FAILURE().  Its destructor verifies that the given
// TestPartResultArray contains exactly one failure that has the given
// type and contains the given substring.  If that's not the case, a
// non-fatal failure will be generated.
class SingleFailureChecker {
 public:
  // The constructor remembers the arguments.
  SingleFailureChecker(const TestPartResultArray* results,
                       TestPartResultType type,
                       const char* substr);
  ~SingleFailureChecker();
 private:
  const TestPartResultArray* const results_;
  const TestPartResultType type_;
  const String substr_;

  GTEST_DISALLOW_COPY_AND_ASSIGN(SingleFailureChecker);
};

}  // namespace internal

}  // namespace testing

// A macro for testing Google Test assertions or code that's expected to
// generate Google Test fatal failures.  It verifies that the given
// statement will cause exactly one fatal Google Test failure with 'substr'
// being part of the failure message.
//
// Implementation note: The verification is done in the destructor of
// SingleFailureChecker, to make sure that it's done even when
// 'statement' throws an exception.
//
// Known restrictions:
//   - 'statement' cannot reference local non-static variables or
//     non-static members of the current object.
//   - 'statement' cannot return a value.
//   - You cannot stream a failure message to this macro.
#define EXPECT_FATAL_FAILURE(statement, substr) do {\
    class GTestExpectFatalFailureHelper {\
     public:\
      static void Execute() { statement; }\
    };\
    ::testing::TestPartResultArray gtest_failures;\
    ::testing::internal::SingleFailureChecker gtest_checker(\
        &gtest_failures, ::testing::TPRT_FATAL_FAILURE, (substr));\
    {\
      ::testing::ScopedFakeTestPartResultReporter gtest_reporter(\
          &gtest_failures);\
      GTestExpectFatalFailureHelper::Execute();\
    }\
  } while (false)

// A macro for testing Google Test assertions or code that's expected to
// generate Google Test non-fatal failures.  It asserts that the given
// statement will cause exactly one non-fatal Google Test failure with
// 'substr' being part of the failure message.
//
// 'statement' is allowed to reference local variables and members of
// the current object.
//
// Implementation note: The verification is done in the destructor of
// SingleFailureChecker, to make sure that it's done even when
// 'statement' throws an exception or aborts the function.
//
// Known restrictions:
//   - You cannot stream a failure message to this macro.
#define EXPECT_NONFATAL_FAILURE(statement, substr) do {\
    ::testing::TestPartResultArray gtest_failures;\
    ::testing::internal::SingleFailureChecker gtest_checker(\
        &gtest_failures, ::testing::TPRT_NONFATAL_FAILURE, (substr));\
    {\
      ::testing::ScopedFakeTestPartResultReporter gtest_reporter(\
          &gtest_failures);\
      statement;\
    }\
  } while (false)

#endif  // GTEST_INCLUDE_GTEST_GTEST_SPI_H_
