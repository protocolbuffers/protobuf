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

#include <gtest/gtest.h>
#include <gtest/gtest-spi.h>

#include <ctype.h>
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef GTEST_OS_LINUX

// TODO(kenton):  Use autoconf to detect availability of gettimeofday().
#define HAS_GETTIMEOFDAY

#include <fcntl.h>
#include <limits.h>
#include <sched.h>
// Declares vsnprintf().  This header is not available on Windows.
#include <strings.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <unistd.h>
#include <string>
#include <vector>

#elif defined(_WIN32_WCE)  // We are on Windows CE.

#include <windows.h>  // NOLINT

#elif defined(_WIN32)  // We are on Windows proper.

#include <io.h>  // NOLINT
#include <sys/timeb.h>  // NOLINT
#include <sys/types.h>  // NOLINT
#include <sys/stat.h>  // NOLINT

#if defined(__MINGW__) || defined(__MINGW32__)
// MinGW has gettimeofday() but not _ftime64()
// TODO(kenton):  Use autoconf to detect availability of gettimeofday().
// TODO(kenton):  There are other ways to get the time on Windows, like
//   GetTickCount() or GetSystemTimeAsFileTime().  MinGW supports these.
//   consider using them instead.
#define HAS_GETTIMEOFDAY
#include <sys/time.h>  // NOLINT
#endif

// cpplint thinks that the header is already included, so we want to
// silence it.
#include <windows.h>  // NOLINT

#else

// Assume other platforms have gettimeofday().
// TODO(kenton):  Use autoconf to detect availability of gettimeofday().
#define HAS_GETTIMEOFDAY

// cpplint thinks that the header is already included, so we want to
// silence it.
#include <sys/time.h>  // NOLINT
#include <unistd.h>  // NOLINT

#endif

// Indicates that this translation unit is part of Google Test's
// implementation.  It must come before gtest-internal-inl.h is
// included, or there will be a compiler error.  This trick is to
// prevent a user from accidentally including gtest-internal-inl.h in
// his code.
#define GTEST_IMPLEMENTATION
#include <gtest/gtest-internal-inl.h>
#undef GTEST_IMPLEMENTATION

#ifdef GTEST_OS_WINDOWS
#define fileno _fileno
#define isatty _isatty
#define vsnprintf _vsnprintf
#endif  // GTEST_OS_WINDOWS

namespace testing {

// Calling ForEach(internal::Delete<T>) doesn't work on HP C++ / Tru64.  So,
// we must define a separate non-template function for each type.
static void DeleteTestCase(TestCase *x)
{
 delete x;
}
static void DeleteEnvironment(Environment *x)
{
 delete x;
}
static void DeleteTestInfo(TestInfo *x)
{
 delete x;
}

// Constants.

// A test that matches this pattern is disabled and not run.
static const char kDisableTestPattern[] = "DISABLED_*";

// A test filter that matches everything.
static const char kUniversalFilter[] = "*";

// The default output file for XML output.
static const char kDefaultOutputFile[] = "test_detail.xml";

GTEST_DEFINE_bool(
    break_on_failure,
    internal::BoolFromGTestEnv("break_on_failure", false),
    "True iff a failed assertion should be a debugger break-point.");

GTEST_DEFINE_bool(
    catch_exceptions,
    internal::BoolFromGTestEnv("catch_exceptions", false),
    "True iff " GTEST_NAME
    " should catch exceptions and treat them as test failures.");

GTEST_DEFINE_string(
    color,
    internal::StringFromGTestEnv("color", "auto"),
    "Whether to use colors in the output.  Valid values: yes, no, "
    "and auto.  'auto' means to use colors if the output is "
    "being sent to a terminal and the TERM environment variable "
    "is set to xterm or xterm-color.");

GTEST_DEFINE_string(
    filter,
    internal::StringFromGTestEnv("filter", kUniversalFilter),
    "A colon-separated list of glob (not regex) patterns "
    "for filtering the tests to run, optionally followed by a "
    "'-' and a : separated list of negative patterns (tests to "
    "exclude).  A test is run if it matches one of the positive "
    "patterns and does not match any of the negative patterns.");

GTEST_DEFINE_bool(list_tests, false,
                  "List all tests without running them.");

GTEST_DEFINE_string(
    output,
    internal::StringFromGTestEnv("output", ""),
    "A format (currently must be \"xml\"), optionally followed "
    "by a colon and an output file name or directory. A directory "
    "is indicated by a trailing pathname separator. "
    "Examples: \"xml:filename.xml\", \"xml::directoryname/\". "
    "If a directory is specified, output files will be created "
    "within that directory, with file-names based on the test "
    "executable's name and, if necessary, made unique by adding "
    "digits.");

GTEST_DEFINE_int32(
    repeat,
    internal::Int32FromGTestEnv("repeat", 1),
    "How many times to repeat each test.  Specify a negative number "
    "for repeating forever.  Useful for shaking out flaky tests.");

GTEST_DEFINE_int32(
    stack_trace_depth,
        internal::Int32FromGTestEnv("stack_trace_depth", kMaxStackTraceDepth),
    "The maximum number of stack frames to print when an "
    "assertion fails.  The valid range is 0 through 100, inclusive.");

GTEST_DEFINE_bool(
    show_internal_stack_frames, false,
    "True iff " GTEST_NAME " should include internal stack frames when "
    "printing test failure stack traces.");

namespace internal {

// GTestIsInitialized() returns true iff the user has initialized
// Google Test.  Useful for catching the user mistake of not initializing
// Google Test before calling RUN_ALL_TESTS().

// A user must call testing::ParseGTestFlags() to initialize Google
// Test.  g_parse_gtest_flags_called is set to true iff
// ParseGTestFlags() has been called.  We don't protect this variable
// under a mutex as it is only accessed in the main thread.
static bool g_parse_gtest_flags_called = false;
static bool GTestIsInitialized() { return g_parse_gtest_flags_called; }

// Iterates over a list of TestCases, keeping a running sum of the
// results of calling a given int-returning method on each.
// Returns the sum.
static int SumOverTestCaseList(const internal::List<TestCase*>& case_list,
                               int (TestCase::*method)() const) {
  int sum = 0;
  for (const internal::ListNode<TestCase*>* node = case_list.Head();
       node != NULL;
       node = node->next()) {
    sum += (node->element()->*method)();
  }
  return sum;
}

// Returns true iff the test case passed.
static bool TestCasePassed(const TestCase* test_case) {
  return test_case->should_run() && test_case->Passed();
}

// Returns true iff the test case failed.
static bool TestCaseFailed(const TestCase* test_case) {
  return test_case->should_run() && test_case->Failed();
}

// Returns true iff test_case contains at least one test that should
// run.
static bool ShouldRunTestCase(const TestCase* test_case) {
  return test_case->should_run();
}

#ifdef _WIN32_WCE
// Windows CE has no C library. The abort() function is used in
// several places in Google Test. This implementation provides a reasonable
// imitation of standard behaviour.
static void abort() {
  DebugBreak();
  TerminateProcess(GetCurrentProcess(), 1);
}
#endif  // _WIN32_WCE

// AssertHelper constructor.
AssertHelper::AssertHelper(TestPartResultType type, const char* file,
                           int line, const char* message)
    : type_(type), file_(file), line_(line), message_(message) {
}

// Message assignment, for assertion streaming support.
void AssertHelper::operator=(const Message& message) const {
  UnitTest::GetInstance()->
    AddTestPartResult(type_, file_, line_,
                      AppendUserMessage(message_, message),
                      UnitTest::GetInstance()->impl()
                      ->CurrentOsStackTraceExceptTop(1)
                      // Skips the stack frame for this function itself.
                      );  // NOLINT
}

// Application pathname gotten in ParseGTestFlags.
String g_executable_path;

// Returns the current application's name, removing directory path if that
// is present.
FilePath GetCurrentExecutableName() {
  FilePath result;

#if defined(_WIN32_WCE) || defined(_WIN32)
  result.Set(FilePath(g_executable_path).RemoveExtension("exe"));
#else
  result.Set(FilePath(g_executable_path));
#endif  // _WIN32_WCE || _WIN32

  return result.RemoveDirectoryName();
}

// Functions for processing the gtest_output flag.

// Returns the output format, or "" for normal printed output.
String UnitTestOptions::GetOutputFormat() {
  const char* const gtest_output_flag = GTEST_FLAG(output).c_str();
  if (gtest_output_flag == NULL) return String("");

  const char* const colon = strchr(gtest_output_flag, ':');
  return (colon == NULL) ?
      String(gtest_output_flag) :
      String(gtest_output_flag, colon - gtest_output_flag);
}

// Returns the name of the requested output file, or the default if none
// was explicitly specified.
String UnitTestOptions::GetOutputFile() {
  const char* const gtest_output_flag = GTEST_FLAG(output).c_str();
  if (gtest_output_flag == NULL)
    return String("");

  const char* const colon = strchr(gtest_output_flag, ':');
  if (colon == NULL)
    return String(kDefaultOutputFile);

  internal::FilePath output_name(colon + 1);
  if (!output_name.IsDirectory())
    return output_name.ToString();

  internal::FilePath result(internal::FilePath::GenerateUniqueFileName(
      output_name, internal::GetCurrentExecutableName(),
      GetOutputFormat().c_str()));
  return result.ToString();
}

// Returns true iff the wildcard pattern matches the string.  The
// first ':' or '\0' character in pattern marks the end of it.
//
// This recursive algorithm isn't very efficient, but is clear and
// works well enough for matching test names, which are short.
bool UnitTestOptions::PatternMatchesString(const char *pattern,
                                           const char *str) {
  switch (*pattern) {
    case '\0':
    case ':':  // Either ':' or '\0' marks the end of the pattern.
      return *str == '\0';
    case '?':  // Matches any single character.
      return *str != '\0' && PatternMatchesString(pattern + 1, str + 1);
    case '*':  // Matches any string (possibly empty) of characters.
      return (*str != '\0' && PatternMatchesString(pattern, str + 1)) ||
          PatternMatchesString(pattern + 1, str);
    default:  // Non-special character.  Matches itself.
      return *pattern == *str &&
          PatternMatchesString(pattern + 1, str + 1);
  }
}

bool UnitTestOptions::MatchesFilter(const String& name, const char* filter) {
  const char *cur_pattern = filter;
  while (true) {
    if (PatternMatchesString(cur_pattern, name.c_str())) {
      return true;
    }

    // Finds the next pattern in the filter.
    cur_pattern = strchr(cur_pattern, ':');

    // Returns if no more pattern can be found.
    if (cur_pattern == NULL) {
      return false;
    }

    // Skips the pattern separater (the ':' character).
    cur_pattern++;
  }
}

// TODO(keithray): move String function implementations to gtest-string.cc.

// Returns true iff the user-specified filter matches the test case
// name and the test name.
bool UnitTestOptions::FilterMatchesTest(const String &test_case_name,
                                        const String &test_name) {
  const String& full_name = String::Format("%s.%s",
                                           test_case_name.c_str(),
                                           test_name.c_str());

  // Split --gtest_filter at '-', if there is one, to separate into
  // positive filter and negative filter portions
  const char* const p = GTEST_FLAG(filter).c_str();
  const char* const dash = strchr(p, '-');
  String positive;
  String negative;
  if (dash == NULL) {
    positive = GTEST_FLAG(filter).c_str();  // Whole string is a positive filter
    negative = String("");
  } else {
    positive.Set(p, dash - p);       // Everything up to the dash
    negative = String(dash+1);       // Everything after the dash
    if (positive.empty()) {
      // Treat '-test1' as the same as '*-test1'
      positive = kUniversalFilter;
    }
  }

  // A filter is a colon-separated list of patterns.  It matches a
  // test if any pattern in it matches the test.
  return (MatchesFilter(full_name, positive.c_str()) &&
          !MatchesFilter(full_name, negative.c_str()));
}

#ifdef GTEST_OS_WINDOWS
// Returns EXCEPTION_EXECUTE_HANDLER if Google Test should handle the
// given SEH exception, or EXCEPTION_CONTINUE_SEARCH otherwise.
// This function is useful as an __except condition.
int UnitTestOptions::GTestShouldProcessSEH(DWORD exception_code) {
  // Google Test should handle an exception if:
  //   1. the user wants it to, AND
  //   2. this is not a breakpoint exception.
  return (GTEST_FLAG(catch_exceptions) &&
          exception_code != EXCEPTION_BREAKPOINT) ?
      EXCEPTION_EXECUTE_HANDLER :
      EXCEPTION_CONTINUE_SEARCH;
}
#endif  // GTEST_OS_WINDOWS

}  // namespace internal

// The interface for printing the result of a UnitTest
class UnitTestEventListenerInterface {
 public:
  // The d'tor is pure virtual as this is an abstract class.
  virtual ~UnitTestEventListenerInterface() = 0;

  // Called before the unit test starts.
  virtual void OnUnitTestStart(const UnitTest*) {}

  // Called after the unit test ends.
  virtual void OnUnitTestEnd(const UnitTest*) {}

  // Called before the test case starts.
  virtual void OnTestCaseStart(const TestCase*) {}

  // Called after the test case ends.
  virtual void OnTestCaseEnd(const TestCase*) {}

  // Called before the global set-up starts.
  virtual void OnGlobalSetUpStart(const UnitTest*) {}

  // Called after the global set-up ends.
  virtual void OnGlobalSetUpEnd(const UnitTest*) {}

  // Called before the global tear-down starts.
  virtual void OnGlobalTearDownStart(const UnitTest*) {}

  // Called after the global tear-down ends.
  virtual void OnGlobalTearDownEnd(const UnitTest*) {}

  // Called before the test starts.
  virtual void OnTestStart(const TestInfo*) {}

  // Called after the test ends.
  virtual void OnTestEnd(const TestInfo*) {}

  // Called after an assertion.
  virtual void OnNewTestPartResult(const TestPartResult*) {}
};

// Constructs an empty TestPartResultArray.
TestPartResultArray::TestPartResultArray()
    : list_(new internal::List<TestPartResult>) {
}

// Destructs a TestPartResultArray.
TestPartResultArray::~TestPartResultArray() {
  delete list_;
}

// Appends a TestPartResult to the array.
void TestPartResultArray::Append(const TestPartResult& result) {
  list_->PushBack(result);
}

// Returns the TestPartResult at the given index (0-based).
const TestPartResult& TestPartResultArray::GetTestPartResult(int index) const {
  if (index < 0 || index >= size()) {
    printf("\nInvalid index (%d) into TestPartResultArray.\n", index);
    abort();
  }

  const internal::ListNode<TestPartResult>* p = list_->Head();
  for (int i = 0; i < index; i++) {
    p = p->next();
  }

  return p->element();
}

// Returns the number of TestPartResult objects in the array.
int TestPartResultArray::size() const {
  return list_->size();
}

// The c'tor sets this object as the test part result reporter used by
// Google Test.  The 'result' parameter specifies where to report the
// results.
ScopedFakeTestPartResultReporter::ScopedFakeTestPartResultReporter(
    TestPartResultArray* result)
    : old_reporter_(UnitTest::GetInstance()->impl()->
                    test_part_result_reporter()),
      result_(result) {
  internal::UnitTestImpl* const impl = UnitTest::GetInstance()->impl();
  impl->set_test_part_result_reporter(this);
}

// The d'tor restores the test part result reporter used by Google Test
// before.
ScopedFakeTestPartResultReporter::~ScopedFakeTestPartResultReporter() {
  UnitTest::GetInstance()->impl()->
      set_test_part_result_reporter(old_reporter_);
}

// Increments the test part result count and remembers the result.
// This method is from the TestPartResultReporterInterface interface.
void ScopedFakeTestPartResultReporter::ReportTestPartResult(
    const TestPartResult& result) {
  result_->Append(result);
}

namespace internal {

// This predicate-formatter checks that 'results' contains a test part
// failure of the given type and that the failure message contains the
// given substring.
AssertionResult HasOneFailure(const char* /* results_expr */,
                              const char* /* type_expr */,
                              const char* /* substr_expr */,
                              const TestPartResultArray& results,
                              TestPartResultType type,
                              const char* substr) {
  const String expected(
      type == TPRT_FATAL_FAILURE ? "1 fatal failure" :
      "1 non-fatal failure");
  Message msg;
  if (results.size() != 1) {
    msg << "Expected: " << expected << "\n"
        << "  Actual: " << results.size() << " failures";
    for (int i = 0; i < results.size(); i++) {
      msg << "\n" << results.GetTestPartResult(i);
    }
    return AssertionFailure(msg);
  }

  const TestPartResult& r = results.GetTestPartResult(0);
  if (r.type() != type) {
    msg << "Expected: " << expected << "\n"
        << "  Actual:\n"
        << r;
    return AssertionFailure(msg);
  }

  if (strstr(r.message(), substr) == NULL) {
    msg << "Expected: " << expected << " containing \""
        << substr << "\"\n"
        << "  Actual:\n"
        << r;
    return AssertionFailure(msg);
  }

  return AssertionSuccess();
}

// The constructor of SingleFailureChecker remembers where to look up
// test part results, what type of failure we expect, and what
// substring the failure message should contain.
SingleFailureChecker:: SingleFailureChecker(
    const TestPartResultArray* results,
    TestPartResultType type,
    const char* substr)
    : results_(results),
      type_(type),
      substr_(substr) {}

// The destructor of SingleFailureChecker verifies that the given
// TestPartResultArray contains exactly one failure that has the given
// type and contains the given substring.  If that's not the case, a
// non-fatal failure will be generated.
SingleFailureChecker::~SingleFailureChecker() {
  EXPECT_PRED_FORMAT3(HasOneFailure, *results_, type_, substr_.c_str());
}

// Reports a test part result.
void UnitTestImpl::ReportTestPartResult(const TestPartResult& result) {
  current_test_result()->AddTestPartResult(result);
  result_printer()->OnNewTestPartResult(&result);
}

// Returns the current test part result reporter.
TestPartResultReporterInterface* UnitTestImpl::test_part_result_reporter() {
  return test_part_result_reporter_;
}

// Sets the current test part result reporter.
void UnitTestImpl::set_test_part_result_reporter(
    TestPartResultReporterInterface* reporter) {
  test_part_result_reporter_ = reporter;
}

// Gets the number of successful test cases.
int UnitTestImpl::successful_test_case_count() const {
  return test_cases_.CountIf(TestCasePassed);
}

// Gets the number of failed test cases.
int UnitTestImpl::failed_test_case_count() const {
  return test_cases_.CountIf(TestCaseFailed);
}

// Gets the number of all test cases.
int UnitTestImpl::total_test_case_count() const {
  return test_cases_.size();
}

// Gets the number of all test cases that contain at least one test
// that should run.
int UnitTestImpl::test_case_to_run_count() const {
  return test_cases_.CountIf(ShouldRunTestCase);
}

// Gets the number of successful tests.
int UnitTestImpl::successful_test_count() const {
  return SumOverTestCaseList(test_cases_, &TestCase::successful_test_count);
}

// Gets the number of failed tests.
int UnitTestImpl::failed_test_count() const {
  return SumOverTestCaseList(test_cases_, &TestCase::failed_test_count);
}

// Gets the number of disabled tests.
int UnitTestImpl::disabled_test_count() const {
  return SumOverTestCaseList(test_cases_, &TestCase::disabled_test_count);
}

// Gets the number of all tests.
int UnitTestImpl::total_test_count() const {
  return SumOverTestCaseList(test_cases_, &TestCase::total_test_count);
}

// Gets the number of tests that should run.
int UnitTestImpl::test_to_run_count() const {
  return SumOverTestCaseList(test_cases_, &TestCase::test_to_run_count);
}

// Returns the current OS stack trace as a String.
//
// The maximum number of stack frames to be included is specified by
// the gtest_stack_trace_depth flag.  The skip_count parameter
// specifies the number of top frames to be skipped, which doesn't
// count against the number of frames to be included.
//
// For example, if Foo() calls Bar(), which in turn calls
// CurrentOsStackTraceExceptTop(1), Foo() will be included in the
// trace but Bar() and CurrentOsStackTraceExceptTop() won't.
String UnitTestImpl::CurrentOsStackTraceExceptTop(int skip_count) {
  (void)skip_count;
  return String("");
}

static TimeInMillis GetTimeInMillis() {
#ifdef _WIN32_WCE  // We are on Windows CE
  // Difference between 1970-01-01 and 1601-01-01 in miliseconds.
  // http://analogous.blogspot.com/2005/04/epoch.html
  const TimeInMillis kJavaEpochToWinFileTimeDelta = 11644473600000UL;
  const DWORD kTenthMicrosInMilliSecond = 10000;

  SYSTEMTIME now_systime;
  FILETIME now_filetime;
  ULARGE_INTEGER now_int64;
  // TODO(kenton):  Shouldn't this just use GetSystemTimeAsFileTime()?
  GetSystemTime(&now_systime);
  if (SystemTimeToFileTime(&now_systime, &now_filetime)) {
    now_int64.LowPart = now_filetime.dwLowDateTime;
    now_int64.HighPart = now_filetime.dwHighDateTime;
    now_int64.QuadPart = (now_int64.QuadPart / kTenthMicrosInMilliSecond) -
      kJavaEpochToWinFileTimeDelta;
    return now_int64.QuadPart;
  }
  return 0;
#elif defined(_WIN32) && !defined(HAS_GETTIMEOFDAY)
  __timeb64 now;
#ifdef _MSC_VER
  // MSVC 8 deprecates _ftime64(), so we want to suppress warning 4996
  // (deprecated function) there.
  // TODO(kenton):  Use GetTickCount()?  Or use SystemTimeToFileTime()
#pragma warning(push)          // Saves the current warning state.
#pragma warning(disable:4996)  // Temporarily disables warning 4996.
  _ftime64(&now);
#pragma warning(pop)           // Restores the warning state.
#else
  _ftime64(&now);
#endif  // _MSC_VER
  return static_cast<TimeInMillis>(now.time) * 1000 + now.millitm;
#elif defined(HAS_GETTIMEOFDAY)
  struct timeval now;
  gettimeofday(&now, NULL);
  return static_cast<TimeInMillis>(now.tv_sec) * 1000 + now.tv_usec / 1000;
#else
#error "Don't know how to get the current time on your system."
  return 0;
#endif
}

// Utilities

// class String

// Returns the input enclosed in double quotes if it's not NULL;
// otherwise returns "(null)".  For example, "\"Hello\"" is returned
// for input "Hello".
//
// This is useful for printing a C string in the syntax of a literal.
//
// Known issue: escape sequences are not handled yet.
String String::ShowCStringQuoted(const char* c_str) {
  return c_str ? String::Format("\"%s\"", c_str) : String("(null)");
}

// Copies at most length characters from str into a newly-allocated
// piece of memory of size length+1.  The memory is allocated with new[].
// A terminating null byte is written to the memory, and a pointer to it
// is returned.  If str is NULL, NULL is returned.
static char* CloneString(const char* str, size_t length) {
  if (str == NULL) {
    return NULL;
  } else {
    char* const clone = new char[length + 1];
    // MSVC 8 deprecates strncpy(), so we want to suppress warning
    // 4996 (deprecated function) there.
#ifdef GTEST_OS_WINDOWS  // We are on Windows.
#pragma warning(push)          // Saves the current warning state.
#pragma warning(disable:4996)  // Temporarily disables warning 4996.
    strncpy(clone, str, length);
#pragma warning(pop)           // Restores the warning state.
#else  // We are on Linux or Mac OS.
    strncpy(clone, str, length);
#endif  // GTEST_OS_WINDOWS
    clone[length] = '\0';
    return clone;
  }
}

// Clones a 0-terminated C string, allocating memory using new.  The
// caller is responsible for deleting[] the return value.  Returns the
// cloned string, or NULL if the input is NULL.
const char * String::CloneCString(const char* c_str) {
  return (c_str == NULL) ?
                    NULL : CloneString(c_str, strlen(c_str));
}

// Compares two C strings.  Returns true iff they have the same content.
//
// Unlike strcmp(), this function can handle NULL argument(s).  A NULL
// C string is considered different to any non-NULL C string,
// including the empty string.
bool String::CStringEquals(const char * lhs, const char * rhs) {
  if ( lhs == NULL ) return rhs == NULL;

  if ( rhs == NULL ) return false;

  return strcmp(lhs, rhs) == 0;
}

#if GTEST_HAS_STD_WSTRING || GTEST_HAS_GLOBAL_WSTRING

// Converts an array of wide chars to a narrow string using the UTF-8
// encoding, and streams the result to the given Message object.
static void StreamWideCharsToMessage(const wchar_t* wstr, size_t len,
                                     Message* msg) {
  for (size_t i = 0; i != len; i++) {
    // TODO(wan): consider allowing a testing::String object to
    // contain '\0'.  This will make it behave more like std::string,
    // and will allow ToUtf8String() to return the correct encoding
    // for '\0' s.t. we can get rid of the conditional here (and in
    // several other places).
    if (wstr[i]) {
      *msg << internal::ToUtf8String(wstr[i]);
    } else {
      *msg << '\0';
    }
  }
}

#endif  // GTEST_HAS_STD_WSTRING || GTEST_HAS_GLOBAL_WSTRING

}  // namespace internal

#if GTEST_HAS_STD_WSTRING
// Converts the given wide string to a narrow string using the UTF-8
// encoding, and streams the result to this Message object.
Message& Message::operator <<(const ::std::wstring& wstr) {
  internal::StreamWideCharsToMessage(wstr.c_str(), wstr.length(), this);
  return *this;
}
#endif  // GTEST_HAS_STD_WSTRING

#if GTEST_HAS_GLOBAL_WSTRING
// Converts the given wide string to a narrow string using the UTF-8
// encoding, and streams the result to this Message object.
Message& Message::operator <<(const ::wstring& wstr) {
  internal::StreamWideCharsToMessage(wstr.c_str(), wstr.length(), this);
  return *this;
}
#endif  // GTEST_HAS_GLOBAL_WSTRING

namespace internal {

// Formats a value to be used in a failure message.

// For a char value, we print it as a C++ char literal and as an
// unsigned integer (both in decimal and in hexadecimal).
String FormatForFailureMessage(char ch) {
  const unsigned int ch_as_uint = ch;
  // A String object cannot contain '\0', so we print "\\0" when ch is
  // '\0'.
  return String::Format("'%s' (%u, 0x%X)",
                        ch ? String::Format("%c", ch).c_str() : "\\0",
                        ch_as_uint, ch_as_uint);
}


}  // namespace internal

// AssertionResult constructor.
AssertionResult::AssertionResult(const internal::String& failure_message)
    : failure_message_(failure_message) {
}


// Makes a successful assertion result.
AssertionResult AssertionSuccess() {
  return AssertionResult();
}


// Makes a failed assertion result with the given failure message.
AssertionResult AssertionFailure(const Message& message) {
  return AssertionResult(message.GetString());
}

namespace internal {

// Constructs and returns the message for an equality assertion
// (e.g. ASSERT_EQ, EXPECT_STREQ, etc) failure.
//
// The first four parameters are the expressions used in the assertion
// and their values, as strings.  For example, for ASSERT_EQ(foo, bar)
// where foo is 5 and bar is 6, we have:
//
//   expected_expression: "foo"
//   actual_expression:   "bar"
//   expected_value:      "5"
//   actual_value:        "6"
//
// The ignoring_case parameter is true iff the assertion is a
// *_STRCASEEQ*.  When it's true, the string " (ignoring case)" will
// be inserted into the message.
AssertionResult EqFailure(const char* expected_expression,
                          const char* actual_expression,
                          const String& expected_value,
                          const String& actual_value,
                          bool ignoring_case) {
  Message msg;
  msg << "Value of: " << actual_expression;
  if (actual_value != actual_expression) {
    msg << "\n  Actual: " << actual_value;
  }

  msg << "\nExpected: " << expected_expression;
  if (ignoring_case) {
    msg << " (ignoring case)";
  }
  if (expected_value != expected_expression) {
    msg << "\nWhich is: " << expected_value;
  }

  return AssertionFailure(msg);
}


// Helper function for implementing ASSERT_NEAR.
AssertionResult DoubleNearPredFormat(const char* expr1,
                                     const char* expr2,
                                     const char* abs_error_expr,
                                     double val1,
                                     double val2,
                                     double abs_error) {
  const double diff = fabs(val1 - val2);
  if (diff <= abs_error) return AssertionSuccess();

  // TODO(wan): do not print the value of an expression if it's
  // already a literal.
  Message msg;
  msg << "The difference between " << expr1 << " and " << expr2
      << " is " << diff << ", which exceeds " << abs_error_expr << ", where\n"
      << expr1 << " evaluates to " << val1 << ",\n"
      << expr2 << " evaluates to " << val2 << ", and\n"
      << abs_error_expr << " evaluates to " << abs_error << ".";
  return AssertionFailure(msg);
}


// Helper template for implementing FloatLE() and DoubleLE().
template <typename RawType>
AssertionResult FloatingPointLE(const char* expr1,
                                const char* expr2,
                                RawType val1,
                                RawType val2) {
  // Returns success if val1 is less than val2,
  if (val1 < val2) {
    return AssertionSuccess();
  }

  // or if val1 is almost equal to val2.
  const FloatingPoint<RawType> lhs(val1), rhs(val2);
  if (lhs.AlmostEquals(rhs)) {
    return AssertionSuccess();
  }

  // Note that the above two checks will both fail if either val1 or
  // val2 is NaN, as the IEEE floating-point standard requires that
  // any predicate involving a NaN must return false.

  StrStream val1_ss;
  val1_ss << std::setprecision(std::numeric_limits<RawType>::digits10 + 2)
          << val1;

  StrStream val2_ss;
  val2_ss << std::setprecision(std::numeric_limits<RawType>::digits10 + 2)
          << val2;

  Message msg;
  msg << "Expected: (" << expr1 << ") <= (" << expr2 << ")\n"
      << "  Actual: " << StrStreamToString(&val1_ss) << " vs "
      << StrStreamToString(&val2_ss);

  return AssertionFailure(msg);
}

}  // namespace internal

// Asserts that val1 is less than, or almost equal to, val2.  Fails
// otherwise.  In particular, it fails if either val1 or val2 is NaN.
AssertionResult FloatLE(const char* expr1, const char* expr2,
                        float val1, float val2) {
  return internal::FloatingPointLE<float>(expr1, expr2, val1, val2);
}

// Asserts that val1 is less than, or almost equal to, val2.  Fails
// otherwise.  In particular, it fails if either val1 or val2 is NaN.
AssertionResult DoubleLE(const char* expr1, const char* expr2,
                         double val1, double val2) {
  return internal::FloatingPointLE<double>(expr1, expr2, val1, val2);
}

namespace internal {

// The helper function for {ASSERT|EXPECT}_EQ with int or enum
// arguments.
AssertionResult CmpHelperEQ(const char* expected_expression,
                            const char* actual_expression,
                            BiggestInt expected,
                            BiggestInt actual) {
  if (expected == actual) {
    return AssertionSuccess();
  }

  return EqFailure(expected_expression,
                   actual_expression,
                   FormatForComparisonFailureMessage(expected, actual),
                   FormatForComparisonFailureMessage(actual, expected),
                   false);
}

// A macro for implementing the helper functions needed to implement
// ASSERT_?? and EXPECT_?? with integer or enum arguments.  It is here
// just to avoid copy-and-paste of similar code.
#define GTEST_IMPL_CMP_HELPER(op_name, op)\
AssertionResult CmpHelper##op_name(const char* expr1, const char* expr2, \
                                   BiggestInt val1, BiggestInt val2) {\
  if (val1 op val2) {\
    return AssertionSuccess();\
  } else {\
    Message msg;\
    msg << "Expected: (" << expr1 << ") " #op " (" << expr2\
        << "), actual: " << FormatForComparisonFailureMessage(val1, val2)\
        << " vs " << FormatForComparisonFailureMessage(val2, val1);\
    return AssertionFailure(msg);\
  }\
}

// Implements the helper function for {ASSERT|EXPECT}_NE with int or
// enum arguments.
GTEST_IMPL_CMP_HELPER(NE, !=)
// Implements the helper function for {ASSERT|EXPECT}_LE with int or
// enum arguments.
GTEST_IMPL_CMP_HELPER(LE, <=)
// Implements the helper function for {ASSERT|EXPECT}_LT with int or
// enum arguments.
GTEST_IMPL_CMP_HELPER(LT, < )
// Implements the helper function for {ASSERT|EXPECT}_GE with int or
// enum arguments.
GTEST_IMPL_CMP_HELPER(GE, >=)
// Implements the helper function for {ASSERT|EXPECT}_GT with int or
// enum arguments.
GTEST_IMPL_CMP_HELPER(GT, > )

#undef GTEST_IMPL_CMP_HELPER

// The helper function for {ASSERT|EXPECT}_STREQ.
AssertionResult CmpHelperSTREQ(const char* expected_expression,
                               const char* actual_expression,
                               const char* expected,
                               const char* actual) {
  if (String::CStringEquals(expected, actual)) {
    return AssertionSuccess();
  }

  return EqFailure(expected_expression,
                   actual_expression,
                   String::ShowCStringQuoted(expected),
                   String::ShowCStringQuoted(actual),
                   false);
}

// The helper function for {ASSERT|EXPECT}_STRCASEEQ.
AssertionResult CmpHelperSTRCASEEQ(const char* expected_expression,
                                   const char* actual_expression,
                                   const char* expected,
                                   const char* actual) {
  if (String::CaseInsensitiveCStringEquals(expected, actual)) {
    return AssertionSuccess();
  }

  return EqFailure(expected_expression,
                   actual_expression,
                   String::ShowCStringQuoted(expected),
                   String::ShowCStringQuoted(actual),
                   true);
}

// The helper function for {ASSERT|EXPECT}_STRNE.
AssertionResult CmpHelperSTRNE(const char* s1_expression,
                               const char* s2_expression,
                               const char* s1,
                               const char* s2) {
  if (!String::CStringEquals(s1, s2)) {
    return AssertionSuccess();
  } else {
    Message msg;
    msg << "Expected: (" << s1_expression << ") != ("
        << s2_expression << "), actual: \""
        << s1 << "\" vs \"" << s2 << "\"";
    return AssertionFailure(msg);
  }
}

// The helper function for {ASSERT|EXPECT}_STRCASENE.
AssertionResult CmpHelperSTRCASENE(const char* s1_expression,
                                   const char* s2_expression,
                                   const char* s1,
                                   const char* s2) {
  if (!String::CaseInsensitiveCStringEquals(s1, s2)) {
    return AssertionSuccess();
  } else {
    Message msg;
    msg << "Expected: (" << s1_expression << ") != ("
        << s2_expression << ") (ignoring case), actual: \""
        << s1 << "\" vs \"" << s2 << "\"";
    return AssertionFailure(msg);
  }
}

}  // namespace internal

namespace {

// Helper functions for implementing IsSubString() and IsNotSubstring().

// This group of overloaded functions return true iff needle is a
// substring of haystack.  NULL is considered a substring of itself
// only.

bool IsSubstringPred(const char* needle, const char* haystack) {
  if (needle == NULL || haystack == NULL)
    return needle == haystack;

  return strstr(haystack, needle) != NULL;
}

bool IsSubstringPred(const wchar_t* needle, const wchar_t* haystack) {
  if (needle == NULL || haystack == NULL)
    return needle == haystack;

  return wcsstr(haystack, needle) != NULL;
}

// StringType here can be either ::std::string or ::std::wstring.
template <typename StringType>
bool IsSubstringPred(const StringType& needle,
                     const StringType& haystack) {
  return haystack.find(needle) != StringType::npos;
}

// This function implements either IsSubstring() or IsNotSubstring(),
// depending on the value of the expected_to_be_substring parameter.
// StringType here can be const char*, const wchar_t*, ::std::string,
// or ::std::wstring.
template <typename StringType>
AssertionResult IsSubstringImpl(
    bool expected_to_be_substring,
    const char* needle_expr, const char* haystack_expr,
    const StringType& needle, const StringType& haystack) {
  if (IsSubstringPred(needle, haystack) == expected_to_be_substring)
    return AssertionSuccess();

  const bool is_wide_string = sizeof(needle[0]) > 1;
  const char* const begin_string_quote = is_wide_string ? "L\"" : "\"";
  return AssertionFailure(
      Message()
      << "Value of: " << needle_expr << "\n"
      << "  Actual: " << begin_string_quote << needle << "\"\n"
      << "Expected: " << (expected_to_be_substring ? "" : "not ")
      << "a substring of " << haystack_expr << "\n"
      << "Which is: " << begin_string_quote << haystack << "\"");
}

}  // namespace

// IsSubstring() and IsNotSubstring() check whether needle is a
// substring of haystack (NULL is considered a substring of itself
// only), and return an appropriate error message when they fail.

AssertionResult IsSubstring(
    const char* needle_expr, const char* haystack_expr,
    const char* needle, const char* haystack) {
  return IsSubstringImpl(true, needle_expr, haystack_expr, needle, haystack);
}

AssertionResult IsSubstring(
    const char* needle_expr, const char* haystack_expr,
    const wchar_t* needle, const wchar_t* haystack) {
  return IsSubstringImpl(true, needle_expr, haystack_expr, needle, haystack);
}

AssertionResult IsNotSubstring(
    const char* needle_expr, const char* haystack_expr,
    const char* needle, const char* haystack) {
  return IsSubstringImpl(false, needle_expr, haystack_expr, needle, haystack);
}

AssertionResult IsNotSubstring(
    const char* needle_expr, const char* haystack_expr,
    const wchar_t* needle, const wchar_t* haystack) {
  return IsSubstringImpl(false, needle_expr, haystack_expr, needle, haystack);
}

#if GTEST_HAS_STD_STRING
AssertionResult IsSubstring(
    const char* needle_expr, const char* haystack_expr,
    const ::std::string& needle, const ::std::string& haystack) {
  return IsSubstringImpl(true, needle_expr, haystack_expr, needle, haystack);
}

AssertionResult IsNotSubstring(
    const char* needle_expr, const char* haystack_expr,
    const ::std::string& needle, const ::std::string& haystack) {
  return IsSubstringImpl(false, needle_expr, haystack_expr, needle, haystack);
}
#endif  // GTEST_HAS_STD_STRING

#if GTEST_HAS_STD_WSTRING
AssertionResult IsSubstring(
    const char* needle_expr, const char* haystack_expr,
    const ::std::wstring& needle, const ::std::wstring& haystack) {
  return IsSubstringImpl(true, needle_expr, haystack_expr, needle, haystack);
}

AssertionResult IsNotSubstring(
    const char* needle_expr, const char* haystack_expr,
    const ::std::wstring& needle, const ::std::wstring& haystack) {
  return IsSubstringImpl(false, needle_expr, haystack_expr, needle, haystack);
}
#endif  // GTEST_HAS_STD_WSTRING

namespace internal {

#ifdef GTEST_OS_WINDOWS

namespace {

// Helper function for IsHRESULT{SuccessFailure} predicates
AssertionResult HRESULTFailureHelper(const char* expr,
                                     const char* expected,
                                     long hr) {  // NOLINT
#ifdef _WIN32_WCE
  // Windows CE doesn't support FormatMessage.
  const char error_text[] = "";
#else
  // Looks up the human-readable system message for the HRESULT code
  // and since we're not passing any params to FormatMessage, we don't
  // want inserts expanded.
  const DWORD kFlags = FORMAT_MESSAGE_FROM_SYSTEM |
                       FORMAT_MESSAGE_IGNORE_INSERTS;
  const DWORD kBufSize = 4096;  // String::Format can't exceed this length.
  // Gets the system's human readable message string for this HRESULT.
  char error_text[kBufSize] = { '\0' };
  DWORD message_length = ::FormatMessageA(kFlags,
                                          0,  // no source, we're asking system
                                          hr,  // the error
                                          0,  // no line width restrictions
                                          error_text,  // output buffer
                                          kBufSize,  // buf size
                                          NULL);  // no arguments for inserts
  // Trims tailing white space (FormatMessage leaves a trailing cr-lf)
  for (; message_length && isspace(error_text[message_length - 1]);
          --message_length) {
    error_text[message_length - 1] = '\0';
  }
#endif  // _WIN32_WCE

  const String error_hex(String::Format("0x%08X ", hr));
  Message msg;
  msg << "Expected: " << expr << " " << expected << ".\n"
      << "  Actual: " << error_hex << error_text << "\n";

  return ::testing::AssertionFailure(msg);
}

}  // namespace

AssertionResult IsHRESULTSuccess(const char* expr, long hr) {  // NOLINT
  if (SUCCEEDED(hr)) {
    return AssertionSuccess();
  }
  return HRESULTFailureHelper(expr, "succeeds", hr);
}

AssertionResult IsHRESULTFailure(const char* expr, long hr) {  // NOLINT
  if (FAILED(hr)) {
    return AssertionSuccess();
  }
  return HRESULTFailureHelper(expr, "fails", hr);
}

#endif  // GTEST_OS_WINDOWS

// Utility functions for encoding Unicode text (wide strings) in
// UTF-8.

// A Unicode code-point can have upto 21 bits, and is encoded in UTF-8
// like this:
//
// Code-point length   Encoding
//   0 -  7 bits       0xxxxxxx
//   8 - 11 bits       110xxxxx 10xxxxxx
//  12 - 16 bits       1110xxxx 10xxxxxx 10xxxxxx
//  17 - 21 bits       11110xxx 10xxxxxx 10xxxxxx 10xxxxxx

// The maximum code-point a one-byte UTF-8 sequence can represent.
const UInt32 kMaxCodePoint1 = (static_cast<UInt32>(1) <<  7) - 1;

// The maximum code-point a two-byte UTF-8 sequence can represent.
const UInt32 kMaxCodePoint2 = (static_cast<UInt32>(1) << (5 + 6)) - 1;

// The maximum code-point a three-byte UTF-8 sequence can represent.
const UInt32 kMaxCodePoint3 = (static_cast<UInt32>(1) << (4 + 2*6)) - 1;

// The maximum code-point a four-byte UTF-8 sequence can represent.
const UInt32 kMaxCodePoint4 = (static_cast<UInt32>(1) << (3 + 3*6)) - 1;

// Chops off the n lowest bits from a bit pattern.  Returns the n
// lowest bits.  As a side effect, the original bit pattern will be
// shifted to the right by n bits.
inline UInt32 ChopLowBits(UInt32* bits, int n) {
  const UInt32 low_bits = *bits & ((static_cast<UInt32>(1) << n) - 1);
  *bits >>= n;
  return low_bits;
}

// Converts a Unicode code-point to its UTF-8 encoding.
String ToUtf8String(wchar_t wchar) {
  char str[5] = {};  // Initializes str to all '\0' characters.

  UInt32 code = static_cast<UInt32>(wchar);
  if (code <= kMaxCodePoint1) {
    str[0] = static_cast<char>(code);                          // 0xxxxxxx
  } else if (code <= kMaxCodePoint2) {
    str[1] = static_cast<char>(0x80 | ChopLowBits(&code, 6));  // 10xxxxxx
    str[0] = static_cast<char>(0xC0 | code);                   // 110xxxxx
  } else if (code <= kMaxCodePoint3) {
    str[2] = static_cast<char>(0x80 | ChopLowBits(&code, 6));  // 10xxxxxx
    str[1] = static_cast<char>(0x80 | ChopLowBits(&code, 6));  // 10xxxxxx
    str[0] = static_cast<char>(0xE0 | code);                   // 1110xxxx
  } else if (code <= kMaxCodePoint4) {
    str[3] = static_cast<char>(0x80 | ChopLowBits(&code, 6));  // 10xxxxxx
    str[2] = static_cast<char>(0x80 | ChopLowBits(&code, 6));  // 10xxxxxx
    str[1] = static_cast<char>(0x80 | ChopLowBits(&code, 6));  // 10xxxxxx
    str[0] = static_cast<char>(0xF0 | code);                   // 11110xxx
  } else {
    return String::Format("(Invalid Unicode 0x%llX)",
                          static_cast<UInt64>(wchar));
  }

  return String(str);
}

// Converts a wide C string to a String using the UTF-8 encoding.
// NULL will be converted to "(null)".
String String::ShowWideCString(const wchar_t * wide_c_str) {
  if (wide_c_str == NULL) return String("(null)");

  StrStream ss;
  while (*wide_c_str) {
    ss << internal::ToUtf8String(*wide_c_str++);
  }

  return internal::StrStreamToString(&ss);
}

// Similar to ShowWideCString(), except that this function encloses
// the converted string in double quotes.
String String::ShowWideCStringQuoted(const wchar_t* wide_c_str) {
  if (wide_c_str == NULL) return String("(null)");

  return String::Format("L\"%s\"",
                        String::ShowWideCString(wide_c_str).c_str());
}

// Compares two wide C strings.  Returns true iff they have the same
// content.
//
// Unlike wcscmp(), this function can handle NULL argument(s).  A NULL
// C string is considered different to any non-NULL C string,
// including the empty string.
bool String::WideCStringEquals(const wchar_t * lhs, const wchar_t * rhs) {
  if (lhs == NULL) return rhs == NULL;

  if (rhs == NULL) return false;

  return wcscmp(lhs, rhs) == 0;
}

// Helper function for *_STREQ on wide strings.
AssertionResult CmpHelperSTREQ(const char* expected_expression,
                               const char* actual_expression,
                               const wchar_t* expected,
                               const wchar_t* actual) {
  if (String::WideCStringEquals(expected, actual)) {
    return AssertionSuccess();
  }

  return EqFailure(expected_expression,
                   actual_expression,
                   String::ShowWideCStringQuoted(expected),
                   String::ShowWideCStringQuoted(actual),
                   false);
}

// Helper function for *_STRNE on wide strings.
AssertionResult CmpHelperSTRNE(const char* s1_expression,
                               const char* s2_expression,
                               const wchar_t* s1,
                               const wchar_t* s2) {
  if (!String::WideCStringEquals(s1, s2)) {
    return AssertionSuccess();
  }

  Message msg;
  msg << "Expected: (" << s1_expression << ") != ("
      << s2_expression << "), actual: "
      << String::ShowWideCStringQuoted(s1)
      << " vs " << String::ShowWideCStringQuoted(s2);
  return AssertionFailure(msg);
}

// Compares two C strings, ignoring case.  Returns true iff they have
// the same content.
//
// Unlike strcasecmp(), this function can handle NULL argument(s).  A
// NULL C string is considered different to any non-NULL C string,
// including the empty string.
bool String::CaseInsensitiveCStringEquals(const char * lhs, const char * rhs) {
  if ( lhs == NULL ) return rhs == NULL;

  if ( rhs == NULL ) return false;

#ifdef GTEST_OS_WINDOWS
  return _stricmp(lhs, rhs) == 0;
#else  // GTEST_OS_WINDOWS
  return strcasecmp(lhs, rhs) == 0;
#endif  // GTEST_OS_WINDOWS
}

// Constructs a String by copying a given number of chars from a
// buffer.  E.g. String("hello", 3) will create the string "hel".
String::String(const char * buffer, size_t len) {
  char * const temp = new char[ len + 1 ];
  memcpy(temp, buffer, len);
  temp[ len ] = '\0';
  c_str_ = temp;
}

// Compares this with another String.
// Returns < 0 if this is less than rhs, 0 if this is equal to rhs, or > 0
// if this is greater than rhs.
int String::Compare(const String & rhs) const {
  if ( c_str_ == NULL ) {
    return rhs.c_str_ == NULL ? 0 : -1;  // NULL < anything except NULL
  }

  return rhs.c_str_ == NULL ? 1 : strcmp(c_str_, rhs.c_str_);
}

// Returns true iff this String ends with the given suffix.  *Any*
// String is considered to end with a NULL or empty suffix.
bool String::EndsWith(const char* suffix) const {
  if (suffix == NULL || CStringEquals(suffix, "")) return true;

  if (c_str_ == NULL) return false;

  const size_t this_len = strlen(c_str_);
  const size_t suffix_len = strlen(suffix);
  return (this_len >= suffix_len) &&
         CStringEquals(c_str_ + this_len - suffix_len, suffix);
}

// Returns true iff this String ends with the given suffix, ignoring case.
// Any String is considered to end with a NULL or empty suffix.
bool String::EndsWithCaseInsensitive(const char* suffix) const {
  if (suffix == NULL || CStringEquals(suffix, "")) return true;

  if (c_str_ == NULL) return false;

  const size_t this_len = strlen(c_str_);
  const size_t suffix_len = strlen(suffix);
  return (this_len >= suffix_len) &&
         CaseInsensitiveCStringEquals(c_str_ + this_len - suffix_len, suffix);
}

// Sets the 0-terminated C string this String object represents.  The
// old string in this object is deleted, and this object will own a
// clone of the input string.  This function copies only up to length
// bytes (plus a terminating null byte), or until the first null byte,
// whichever comes first.
//
// This function works even when the c_str parameter has the same
// value as that of the c_str_ field.
void String::Set(const char * c_str, size_t length) {
  // Makes sure this works when c_str == c_str_
  const char* const temp = CloneString(c_str, length);
  delete[] c_str_;
  c_str_ = temp;
}

// Assigns a C string to this object.  Self-assignment works.
const String& String::operator=(const char* c_str) {
  // Makes sure this works when c_str == c_str_
  if (c_str != c_str_) {
    delete[] c_str_;
    c_str_ = CloneCString(c_str);
  }
  return *this;
}

// Formats a list of arguments to a String, using the same format
// spec string as for printf.
//
// We do not use the StringPrintf class as it is not universally
// available.
//
// The result is limited to 4096 characters (including the tailing 0).
// If 4096 characters are not enough to format the input,
// "<buffer exceeded>" is returned.
String String::Format(const char * format, ...) {
  va_list args;
  va_start(args, format);

  char buffer[4096];
  // MSVC 8 deprecates vsnprintf(), so we want to suppress warning
  // 4996 (deprecated function) there.
#ifdef GTEST_OS_WINDOWS  // We are on Windows.
#pragma warning(push)          // Saves the current warning state.
#pragma warning(disable:4996)  // Temporarily disables warning 4996.
  const int size =
    vsnprintf(buffer, sizeof(buffer)/sizeof(buffer[0]) - 1, format, args);
#pragma warning(pop)           // Restores the warning state.
#else  // We are on Linux or Mac OS.
  const int size =
    vsnprintf(buffer, sizeof(buffer)/sizeof(buffer[0]) - 1, format, args);
#endif  // GTEST_OS_WINDOWS
  va_end(args);

  return String(size >= 0 ? buffer : "<buffer exceeded>");
}

// Converts the buffer in a StrStream to a String, converting NUL
// bytes to "\\0" along the way.
String StrStreamToString(StrStream* ss) {
#if GTEST_HAS_STD_STRING
  const ::std::string& str = ss->str();
  const char* const start = str.c_str();
  const char* const end = start + str.length();
#else
  const char* const start = ss->str();
  const char* const end = start + ss->pcount();
#endif  // GTEST_HAS_STD_STRING

  // We need to use a helper StrStream to do this transformation
  // because String doesn't support push_back().
  StrStream helper;
  for (const char* ch = start; ch != end; ++ch) {
    if (*ch == '\0') {
      helper << "\\0";  // Replaces NUL with "\\0";
    } else {
      helper.put(*ch);
    }
  }

#if GTEST_HAS_STD_STRING
  return String(helper.str().c_str());
#else
  const String str(helper.str(), helper.pcount());
  helper.freeze(false);
  ss->freeze(false);
  return str;
#endif  // GTEST_HAS_STD_STRING
}

// Appends the user-supplied message to the Google-Test-generated message.
String AppendUserMessage(const String& gtest_msg,
                         const Message& user_msg) {
  // Appends the user message if it's non-empty.
  const String user_msg_string = user_msg.GetString();
  if (user_msg_string.empty()) {
    return gtest_msg;
  }

  Message msg;
  msg << gtest_msg << "\n" << user_msg_string;

  return msg.GetString();
}

}  // namespace internal

// Prints a TestPartResult object.
std::ostream& operator<<(std::ostream& os, const TestPartResult& result) {
  return os << result.file_name() << ":"
            << result.line_number() << ": "
            << (result.type() == TPRT_SUCCESS ? "Success" :
                result.type() == TPRT_FATAL_FAILURE ? "Fatal failure" :
                "Non-fatal failure") << ":\n"
            << result.message() << std::endl;
}

namespace internal {
// class TestResult

// Creates an empty TestResult.
TestResult::TestResult()
    : death_test_count_(0),
      elapsed_time_(0) {
}

// D'tor.
TestResult::~TestResult() {
}

// Adds a test part result to the list.
void TestResult::AddTestPartResult(const TestPartResult& test_part_result) {
  test_part_results_.PushBack(test_part_result);
}

// Adds a test property to the list. If a property with the same key as the
// supplied property is already represented, the value of this test_property
// replaces the old value for that key.
void TestResult::RecordProperty(const TestProperty& test_property) {
  if (!ValidateTestProperty(test_property)) {
    return;
  }
  MutexLock lock(&test_properites_mutex_);
  ListNode<TestProperty>* const node_with_matching_key =
      test_properties_.FindIf(TestPropertyKeyIs(test_property.key()));
  if (node_with_matching_key == NULL) {
    test_properties_.PushBack(test_property);
    return;
  }
  TestProperty& property_with_matching_key = node_with_matching_key->element();
  property_with_matching_key.SetValue(test_property.value());
}

// Adds a failure if the key is a reserved attribute of Google Test testcase tags.
// Returns true if the property is valid.
bool TestResult::ValidateTestProperty(const TestProperty& test_property) {
  String key(test_property.key());
  if (key == "name" || key == "status" || key == "time" || key == "classname") {
    ADD_FAILURE()
        << "Reserved key used in RecordProperty(): "
        << key
        << " ('name', 'status', 'time', and 'classname' are reserved by "
        << GTEST_NAME << ")";
    return false;
  }
  return true;
}

// Clears the object.
void TestResult::Clear() {
  test_part_results_.Clear();
  test_properties_.Clear();
  death_test_count_ = 0;
  elapsed_time_ = 0;
}

// Returns true iff the test part passed.
static bool TestPartPassed(const TestPartResult & result) {
  return result.passed();
}

// Gets the number of successful test parts.
int TestResult::successful_part_count() const {
  return test_part_results_.CountIf(TestPartPassed);
}

// Returns true iff the test part failed.
static bool TestPartFailed(const TestPartResult & result) {
  return result.failed();
}

// Gets the number of failed test parts.
int TestResult::failed_part_count() const {
  return test_part_results_.CountIf(TestPartFailed);
}

// Returns true iff the test part fatally failed.
static bool TestPartFatallyFailed(const TestPartResult & result) {
  return result.fatally_failed();
}

// Returns true iff the test fatally failed.
bool TestResult::HasFatalFailure() const {
  return test_part_results_.CountIf(TestPartFatallyFailed) > 0;
}

// Gets the number of all test parts.  This is the sum of the number
// of successful test parts and the number of failed test parts.
int TestResult::total_part_count() const {
  return test_part_results_.size();
}

}  // namespace internal

// class Test

// Creates a Test object.

// The c'tor saves the values of all Google Test flags.
Test::Test()
    : gtest_flag_saver_(new internal::GTestFlagSaver) {
}

// The d'tor restores the values of all Google Test flags.
Test::~Test() {
  delete gtest_flag_saver_;
}

// Sets up the test fixture.
//
// A sub-class may override this.
void Test::SetUp() {
}

// Tears down the test fixture.
//
// A sub-class may override this.
void Test::TearDown() {
}

// Allows user supplied key value pairs to be recorded for later output.
void Test::RecordProperty(const char* key, const char* value) {
  UnitTest::GetInstance()->RecordPropertyForCurrentTest(key, value);
}

// Allows user supplied key value pairs to be recorded for later output.
void Test::RecordProperty(const char* key, int value) {
  Message value_message;
  value_message << value;
  RecordProperty(key, value_message.GetString().c_str());
}

#ifdef GTEST_OS_WINDOWS
// We are on Windows.

// Adds an "exception thrown" fatal failure to the current test.
static void AddExceptionThrownFailure(DWORD exception_code,
                                      const char* location) {
  Message message;
  message << "Exception thrown with code 0x" << std::setbase(16) <<
    exception_code << std::setbase(10) << " in " << location << ".";

  UnitTest* const unit_test = UnitTest::GetInstance();
  unit_test->AddTestPartResult(
      TPRT_FATAL_FAILURE,
      static_cast<const char *>(NULL),
           // We have no info about the source file where the exception
           // occurred.
      -1,  // We have no info on which line caused the exception.
      message.GetString(),
      internal::String(""));
}

#endif  // GTEST_OS_WINDOWS

// Google Test requires all tests in the same test case to use the same test
// fixture class.  This function checks if the current test has the
// same fixture class as the first test in the current test case.  If
// yes, it returns true; otherwise it generates a Google Test failure and
// returns false.
bool Test::HasSameFixtureClass() {
  internal::UnitTestImpl* const impl = internal::GetUnitTestImpl();
  const TestCase* const test_case = impl->current_test_case();

  // Info about the first test in the current test case.
  const internal::TestInfoImpl* const first_test_info =
      test_case->test_info_list().Head()->element()->impl();
  const internal::TypeId first_fixture_id = first_test_info->fixture_class_id();
  const char* const first_test_name = first_test_info->name();

  // Info about the current test.
  const internal::TestInfoImpl* const this_test_info =
      impl->current_test_info()->impl();
  const internal::TypeId this_fixture_id = this_test_info->fixture_class_id();
  const char* const this_test_name = this_test_info->name();

  if (this_fixture_id != first_fixture_id) {
    // Is the first test defined using TEST?
    const bool first_is_TEST = first_fixture_id == internal::GetTypeId<Test>();
    // Is this test defined using TEST?
    const bool this_is_TEST = this_fixture_id == internal::GetTypeId<Test>();

    if (first_is_TEST || this_is_TEST) {
      // The user mixed TEST and TEST_F in this test case - we'll tell
      // him/her how to fix it.

      // Gets the name of the TEST and the name of the TEST_F.  Note
      // that first_is_TEST and this_is_TEST cannot both be true, as
      // the fixture IDs are different for the two tests.
      const char* const TEST_name =
          first_is_TEST ? first_test_name : this_test_name;
      const char* const TEST_F_name =
          first_is_TEST ? this_test_name : first_test_name;

      ADD_FAILURE()
          << "All tests in the same test case must use the same test fixture\n"
          << "class, so mixing TEST_F and TEST in the same test case is\n"
          << "illegal.  In test case " << this_test_info->test_case_name()
          << ",\n"
          << "test " << TEST_F_name << " is defined using TEST_F but\n"
          << "test " << TEST_name << " is defined using TEST.  You probably\n"
          << "want to change the TEST to TEST_F or move it to another test\n"
          << "case.";
    } else {
      // The user defined two fixture classes with the same name in
      // two namespaces - we'll tell him/her how to fix it.
      ADD_FAILURE()
          << "All tests in the same test case must use the same test fixture\n"
          << "class.  However, in test case "
          << this_test_info->test_case_name() << ",\n"
          << "you defined test " << first_test_name
          << " and test " << this_test_name << "\n"
          << "using two different test fixture classes.  This can happen if\n"
          << "the two classes are from different namespaces or translation\n"
          << "units and have the same name.  You should probably rename one\n"
          << "of the classes to put the tests into different test cases.";
    }
    return false;
  }

  return true;
}

// Runs the test and updates the test result.
void Test::Run() {
  if (!HasSameFixtureClass()) return;

  internal::UnitTestImpl* const impl = internal::GetUnitTestImpl();
#ifdef GTEST_OS_WINDOWS
  // We are on Windows.
  impl->os_stack_trace_getter()->UponLeavingGTest();
  __try {
    SetUp();
  } __except(internal::UnitTestOptions::GTestShouldProcessSEH(
      GetExceptionCode())) {
    AddExceptionThrownFailure(GetExceptionCode(), "SetUp()");
  }

  // We will run the test only if SetUp() had no fatal failure.
  if (!HasFatalFailure()) {
    impl->os_stack_trace_getter()->UponLeavingGTest();
    __try {
      TestBody();
    } __except(internal::UnitTestOptions::GTestShouldProcessSEH(
        GetExceptionCode())) {
      AddExceptionThrownFailure(GetExceptionCode(), "the test body");
    }
  }

  // However, we want to clean up as much as possible.  Hence we will
  // always call TearDown(), even if SetUp() or the test body has
  // failed.
  impl->os_stack_trace_getter()->UponLeavingGTest();
  __try {
    TearDown();
  } __except(internal::UnitTestOptions::GTestShouldProcessSEH(
      GetExceptionCode())) {
    AddExceptionThrownFailure(GetExceptionCode(), "TearDown()");
  }

#else  // We are on Linux or Mac - exceptions are disabled.
  impl->os_stack_trace_getter()->UponLeavingGTest();
  SetUp();

  // We will run the test only if SetUp() was successful.
  if (!HasFatalFailure()) {
    impl->os_stack_trace_getter()->UponLeavingGTest();
    TestBody();
  }

  // However, we want to clean up as much as possible.  Hence we will
  // always call TearDown(), even if SetUp() or the test body has
  // failed.
  impl->os_stack_trace_getter()->UponLeavingGTest();
  TearDown();
#endif  // GTEST_OS_WINDOWS
}


// Returns true iff the current test has a fatal failure.
bool Test::HasFatalFailure() {
  return internal::GetUnitTestImpl()->current_test_result()->HasFatalFailure();
}

// class TestInfo

// Constructs a TestInfo object.
TestInfo::TestInfo(const char* test_case_name,
                   const char* name,
                   internal::TypeId fixture_class_id,
                   TestMaker maker) {
  impl_ = new internal::TestInfoImpl(this, test_case_name, name,
                                     fixture_class_id, maker);
}

// Destructs a TestInfo object.
TestInfo::~TestInfo() {
  delete impl_;
}

// Creates a TestInfo object and registers it with the UnitTest
// singleton; returns the created object.
//
// Arguments:
//
//   test_case_name: name of the test case
//   name:           name of the test
//   set_up_tc:      pointer to the function that sets up the test case
//   tear_down_tc:   pointer to the function that tears down the test case
//   maker:          pointer to the function that creates a test object
TestInfo* TestInfo::MakeAndRegisterInstance(
    const char* test_case_name,
    const char* name,
    internal::TypeId fixture_class_id,
    Test::SetUpTestCaseFunc set_up_tc,
    Test::TearDownTestCaseFunc tear_down_tc,
    TestMaker maker) {
  TestInfo* const test_info =
      new TestInfo(test_case_name, name, fixture_class_id, maker);
  internal::GetUnitTestImpl()->AddTestInfo(set_up_tc, tear_down_tc, test_info);
  return test_info;
}

// Returns the test case name.
const char* TestInfo::test_case_name() const {
  return impl_->test_case_name();
}

// Returns the test name.
const char* TestInfo::name() const {
  return impl_->name();
}

// Returns true if this test should run.
bool TestInfo::should_run() const { return impl_->should_run(); }

// Returns the result of the test.
const internal::TestResult* TestInfo::result() const { return impl_->result(); }

// Increments the number of death tests encountered in this test so
// far.
int TestInfo::increment_death_test_count() {
  return impl_->result()->increment_death_test_count();
}

namespace {

// A predicate that checks the test name of a TestInfo against a known
// value.
//
// This is used for implementation of the TestCase class only.  We put
// it in the anonymous namespace to prevent polluting the outer
// namespace.
//
// TestNameIs is copyable.
class TestNameIs {
 public:
  // Constructor.
  //
  // TestNameIs has NO default constructor.
  explicit TestNameIs(const char* name)
      : name_(name) {}

  // Returns true iff the test name of test_info matches name_.
  bool operator()(const TestInfo * test_info) const {
    return test_info && internal::String(test_info->name()).Compare(name_) == 0;
  }

 private:
  internal::String name_;
};

}  // namespace

// Finds and returns a TestInfo with the given name.  If one doesn't
// exist, returns NULL.
TestInfo * TestCase::GetTestInfo(const char* test_name) {
  // Can we find a TestInfo with the given name?
  internal::ListNode<TestInfo *> * const node = test_info_list_->FindIf(
      TestNameIs(test_name));

  // Returns the TestInfo found.
  return node ? node->element() : NULL;
}

namespace internal {

// Creates the test object, runs it, records its result, and then
// deletes it.
void TestInfoImpl::Run() {
  if (!should_run_) return;

  // Tells UnitTest where to store test result.
  UnitTestImpl* const impl = internal::GetUnitTestImpl();
  impl->set_current_test_info(parent_);

  // Notifies the unit test event listener that a test is about to
  // start.
  UnitTestEventListenerInterface* const result_printer =
    impl->result_printer();
  result_printer->OnTestStart(parent_);

  const TimeInMillis start = GetTimeInMillis();

  impl->os_stack_trace_getter()->UponLeavingGTest();
#ifdef GTEST_OS_WINDOWS
  // We are on Windows.
  Test* test = NULL;

  __try {
    // Creates the test object.
    test = (*maker_)();
  } __except(internal::UnitTestOptions::GTestShouldProcessSEH(
      GetExceptionCode())) {
    AddExceptionThrownFailure(GetExceptionCode(),
                              "the test fixture's constructor");
    return;
  }
#else  // We are on Linux or Mac OS - exceptions are disabled.

  // TODO(wan): If test->Run() throws, test won't be deleted.  This is
  // not a problem now as we don't use exceptions.  If we were to
  // enable exceptions, we should revise the following to be
  // exception-safe.

  // Creates the test object.
  Test* test = (*maker_)();
#endif  // GTEST_OS_WINDOWS

  // Runs the test only if the constructor of the test fixture didn't
  // generate a fatal failure.
  if (!Test::HasFatalFailure()) {
    test->Run();
  }

  // Deletes the test object.
  impl->os_stack_trace_getter()->UponLeavingGTest();
  delete test;
  test = NULL;

  result_.set_elapsed_time(GetTimeInMillis() - start);

  // Notifies the unit test event listener that a test has just finished.
  result_printer->OnTestEnd(parent_);

  // Tells UnitTest to stop associating assertion results to this
  // test.
  impl->set_current_test_info(NULL);
}

}  // namespace internal

// class TestCase

// Gets the number of successful tests in this test case.
int TestCase::successful_test_count() const {
  return test_info_list_->CountIf(TestPassed);
}

// Gets the number of failed tests in this test case.
int TestCase::failed_test_count() const {
  return test_info_list_->CountIf(TestFailed);
}

int TestCase::disabled_test_count() const {
  return test_info_list_->CountIf(TestDisabled);
}

// Get the number of tests in this test case that should run.
int TestCase::test_to_run_count() const {
  return test_info_list_->CountIf(ShouldRunTest);
}

// Gets the number of all tests.
int TestCase::total_test_count() const {
  return test_info_list_->size();
}

// Creates a TestCase with the given name.
//
// Arguments:
//
//   name:         name of the test case
//   set_up_tc:    pointer to the function that sets up the test case
//   tear_down_tc: pointer to the function that tears down the test case
TestCase::TestCase(const char* name,
                   Test::SetUpTestCaseFunc set_up_tc,
                   Test::TearDownTestCaseFunc tear_down_tc)
    : name_(name),
      set_up_tc_(set_up_tc),
      tear_down_tc_(tear_down_tc),
      should_run_(false),
      elapsed_time_(0) {
  test_info_list_ = new internal::List<TestInfo *>;
}

// Destructor of TestCase.
TestCase::~TestCase() {
  // Deletes every Test in the collection.
  test_info_list_->ForEach(DeleteTestInfo);

  // Then deletes the Test collection.
  delete test_info_list_;
  test_info_list_ = NULL;
}

// Adds a test to this test case.  Will delete the test upon
// destruction of the TestCase object.
void TestCase::AddTestInfo(TestInfo * test_info) {
  test_info_list_->PushBack(test_info);
}

// Runs every test in this TestCase.
void TestCase::Run() {
  if (!should_run_) return;

  internal::UnitTestImpl* const impl = internal::GetUnitTestImpl();
  impl->set_current_test_case(this);

  UnitTestEventListenerInterface * const result_printer =
      impl->result_printer();

  result_printer->OnTestCaseStart(this);
  impl->os_stack_trace_getter()->UponLeavingGTest();
  set_up_tc_();

  const internal::TimeInMillis start = internal::GetTimeInMillis();
  test_info_list_->ForEach(internal::TestInfoImpl::RunTest);
  elapsed_time_ = internal::GetTimeInMillis() - start;

  impl->os_stack_trace_getter()->UponLeavingGTest();
  tear_down_tc_();
  result_printer->OnTestCaseEnd(this);
  impl->set_current_test_case(NULL);
}

// Clears the results of all tests in this test case.
void TestCase::ClearResult() {
  test_info_list_->ForEach(internal::TestInfoImpl::ClearTestResult);
}


// class UnitTestEventListenerInterface

// The virtual d'tor.
UnitTestEventListenerInterface::~UnitTestEventListenerInterface() {
}

// A result printer that never prints anything.  Used in the child process
// of an exec-style death test to avoid needless output clutter.
class NullUnitTestResultPrinter : public UnitTestEventListenerInterface {};

// Formats a countable noun.  Depending on its quantity, either the
// singular form or the plural form is used. e.g.
//
// FormatCountableNoun(1, "formula", "formuli") returns "1 formula".
// FormatCountableNoun(5, "book", "books") returns "5 books".
static internal::String FormatCountableNoun(int count,
                                            const char * singular_form,
                                            const char * plural_form) {
  return internal::String::Format("%d %s", count,
                                  count == 1 ? singular_form : plural_form);
}

// Formats the count of tests.
static internal::String FormatTestCount(int test_count) {
  return FormatCountableNoun(test_count, "test", "tests");
}

// Formats the count of test cases.
static internal::String FormatTestCaseCount(int test_case_count) {
  return FormatCountableNoun(test_case_count, "test case", "test cases");
}

// Converts a TestPartResultType enum to human-friendly string
// representation.  Both TPRT_NONFATAL_FAILURE and TPRT_FATAL_FAILURE
// are translated to "Failure", as the user usually doesn't care about
// the difference between the two when viewing the test result.
static const char * TestPartResultTypeToString(TestPartResultType type) {
  switch (type) {
    case TPRT_SUCCESS:
      return "Success";

    case TPRT_NONFATAL_FAILURE:
    case TPRT_FATAL_FAILURE:
      return "Failure";
  }

  return "Unknown result type";
}

// Prints a TestPartResult.
static void PrintTestPartResult(
    const TestPartResult & test_part_result) {
  const char * const file_name = test_part_result.file_name();

  printf("%s", file_name == NULL ? "unknown file" : file_name);
  if (test_part_result.line_number() >= 0) {
    printf(":%d", test_part_result.line_number());
  }
  printf(": %s\n", TestPartResultTypeToString(test_part_result.type()));
  printf("%s\n", test_part_result.message());
  fflush(stdout);
}

// class PrettyUnitTestResultPrinter

namespace internal {

enum GTestColor {
  COLOR_RED,
  COLOR_GREEN,
  COLOR_YELLOW
};

#ifdef _WIN32

// Returns the character attribute for the given color.
WORD GetColorAttribute(GTestColor color) {
  switch (color) {
    case COLOR_RED:    return FOREGROUND_RED;
    case COLOR_GREEN:  return FOREGROUND_GREEN;
    case COLOR_YELLOW: return FOREGROUND_RED | FOREGROUND_GREEN;
  }
  return 0;
}

#else

// Returns the ANSI color code for the given color.
const char* GetAnsiColorCode(GTestColor color) {
  switch (color) {
    case COLOR_RED:     return "1";
    case COLOR_GREEN:   return "2";
    case COLOR_YELLOW:  return "3";
  };
  return NULL;
}

#endif  // _WIN32

// Returns true iff Google Test should use colors in the output.
bool ShouldUseColor(bool stdout_is_tty) {
  const char* const gtest_color = GTEST_FLAG(color).c_str();

  if (String::CaseInsensitiveCStringEquals(gtest_color, "auto")) {
#ifdef _WIN32
    // On Windows the TERM variable is usually not set, but the
    // console there does support colors.
    return stdout_is_tty;
#else
    // On non-Windows platforms, we rely on the TERM variable.
    const char* const term = GetEnv("TERM");
    const bool term_supports_color =
        String::CStringEquals(term, "xterm") ||
        String::CStringEquals(term, "xterm-color") ||
        String::CStringEquals(term, "cygwin");
    return stdout_is_tty && term_supports_color;
#endif  // _WIN32
  }

  return String::CaseInsensitiveCStringEquals(gtest_color, "yes") ||
      String::CaseInsensitiveCStringEquals(gtest_color, "true") ||
      String::CaseInsensitiveCStringEquals(gtest_color, "t") ||
      String::CStringEquals(gtest_color, "1");
  // We take "yes", "true", "t", and "1" as meaning "yes".  If the
  // value is neither one of these nor "auto", we treat it as "no" to
  // be conservative.
}

// Helpers for printing colored strings to stdout. Note that on Windows, we
// cannot simply emit special characters and have the terminal change colors.
// This routine must actually emit the characters rather than return a string
// that would be colored when printed, as can be done on Linux.
void ColoredPrintf(GTestColor color, const char* fmt, ...) {
  va_list args;
  va_start(args, fmt);

  static const bool use_color = ShouldUseColor(isatty(fileno(stdout)) != 0);
  // The '!= 0' comparison is necessary to satisfy MSVC 7.1.

  if (!use_color) {
    vprintf(fmt, args);
    va_end(args);
    return;
  }

#ifdef _WIN32
  const HANDLE stdout_handle = GetStdHandle(STD_OUTPUT_HANDLE);

  // Gets the current text color.
  CONSOLE_SCREEN_BUFFER_INFO buffer_info;
  GetConsoleScreenBufferInfo(stdout_handle, &buffer_info);
  const WORD old_color_attrs = buffer_info.wAttributes;

  SetConsoleTextAttribute(stdout_handle,
                          GetColorAttribute(color) | FOREGROUND_INTENSITY);
  vprintf(fmt, args);

  // Restores the text color.
  SetConsoleTextAttribute(stdout_handle, old_color_attrs);
#else
  printf("\033[0;3%sm", GetAnsiColorCode(color));
  vprintf(fmt, args);
  printf("\033[m");  // Resets the terminal to default.
#endif  // _WIN32
  va_end(args);
}

}  // namespace internal

using internal::ColoredPrintf;
using internal::COLOR_RED;
using internal::COLOR_GREEN;
using internal::COLOR_YELLOW;

// This class implements the UnitTestEventListenerInterface interface.
//
// Class PrettyUnitTestResultPrinter is copyable.
class PrettyUnitTestResultPrinter : public UnitTestEventListenerInterface {
 public:
  PrettyUnitTestResultPrinter() {}
  static void PrintTestName(const char * test_case, const char * test) {
    printf("%s.%s", test_case, test);
  }

  // The following methods override what's in the
  // UnitTestEventListenerInterface class.
  virtual void OnUnitTestStart(const UnitTest * unit_test);
  virtual void OnGlobalSetUpStart(const UnitTest*);
  virtual void OnTestCaseStart(const TestCase * test_case);
  virtual void OnTestStart(const TestInfo * test_info);
  virtual void OnNewTestPartResult(const TestPartResult * result);
  virtual void OnTestEnd(const TestInfo * test_info);
  virtual void OnGlobalTearDownStart(const UnitTest*);
  virtual void OnUnitTestEnd(const UnitTest * unit_test);

 private:
  internal::String test_case_name_;
};

// Called before the unit test starts.
void PrettyUnitTestResultPrinter::OnUnitTestStart(
    const UnitTest * unit_test) {
  const char * const filter = GTEST_FLAG(filter).c_str();

  // Prints the filter if it's not *.  This reminds the user that some
  // tests may be skipped.
  if (!internal::String::CStringEquals(filter, kUniversalFilter)) {
    ColoredPrintf(COLOR_YELLOW,
                  "Note: %s filter = %s\n", GTEST_NAME, filter);
  }

  const internal::UnitTestImpl* const impl = unit_test->impl();
  ColoredPrintf(COLOR_GREEN,  "[==========] ");
  printf("Running %s from %s.\n",
         FormatTestCount(impl->test_to_run_count()).c_str(),
         FormatTestCaseCount(impl->test_case_to_run_count()).c_str());
  fflush(stdout);
}

void PrettyUnitTestResultPrinter::OnGlobalSetUpStart(const UnitTest*) {
  ColoredPrintf(COLOR_GREEN,  "[----------] ");
  printf("Global test environment set-up.\n");
  fflush(stdout);
}

void PrettyUnitTestResultPrinter::OnTestCaseStart(
    const TestCase * test_case) {
  test_case_name_ = test_case->name();
  const internal::String counts =
      FormatCountableNoun(test_case->test_to_run_count(), "test", "tests");
  ColoredPrintf(COLOR_GREEN, "[----------] ");
  printf("%s from %s\n", counts.c_str(), test_case_name_.c_str());
  fflush(stdout);
}

void PrettyUnitTestResultPrinter::OnTestStart(const TestInfo * test_info) {
  ColoredPrintf(COLOR_GREEN,  "[ RUN      ] ");
  PrintTestName(test_case_name_.c_str(), test_info->name());
  printf("\n");
  fflush(stdout);
}

void PrettyUnitTestResultPrinter::OnTestEnd(const TestInfo * test_info) {
  if (test_info->result()->Passed()) {
    ColoredPrintf(COLOR_GREEN, "[       OK ] ");
  } else {
    ColoredPrintf(COLOR_RED, "[  FAILED  ] ");
  }
  PrintTestName(test_case_name_.c_str(), test_info->name());
  printf("\n");
  fflush(stdout);
}

// Called after an assertion failure.
void PrettyUnitTestResultPrinter::OnNewTestPartResult(
    const TestPartResult * result) {
  // If the test part succeeded, we don't need to do anything.
  if (result->type() == TPRT_SUCCESS)
    return;

  // Print failure message from the assertion (e.g. expected this and got that).
  PrintTestPartResult(*result);
  fflush(stdout);
}

void PrettyUnitTestResultPrinter::OnGlobalTearDownStart(const UnitTest*) {
  ColoredPrintf(COLOR_GREEN,  "[----------] ");
  printf("Global test environment tear-down\n");
  fflush(stdout);
}

namespace internal {

// Internal helper for printing the list of failed tests.
static void PrintFailedTestsPretty(const UnitTestImpl* impl) {
  const int failed_test_count = impl->failed_test_count();
  if (failed_test_count == 0) {
    return;
  }

  for (const internal::ListNode<TestCase*>* node = impl->test_cases()->Head();
       node != NULL; node = node->next()) {
    const TestCase* const tc = node->element();
    if (!tc->should_run() || (tc->failed_test_count() == 0)) {
      continue;
    }
    for (const internal::ListNode<TestInfo*>* tinode =
         tc->test_info_list().Head();
         tinode != NULL; tinode = tinode->next()) {
      const TestInfo* const ti = tinode->element();
      if (!tc->ShouldRunTest(ti) || tc->TestPassed(ti)) {
        continue;
      }
      ColoredPrintf(COLOR_RED, "[  FAILED  ] ");
      printf("%s.%s\n", ti->test_case_name(), ti->name());
    }
  }
}

}  // namespace internal

void PrettyUnitTestResultPrinter::OnUnitTestEnd(
    const UnitTest * unit_test) {
  const internal::UnitTestImpl* const impl = unit_test->impl();

  ColoredPrintf(COLOR_GREEN,  "[==========] ");
  printf("%s from %s ran.\n",
         FormatTestCount(impl->test_to_run_count()).c_str(),
         FormatTestCaseCount(impl->test_case_to_run_count()).c_str());
  ColoredPrintf(COLOR_GREEN,  "[  PASSED  ] ");
  printf("%s.\n", FormatTestCount(impl->successful_test_count()).c_str());

  int num_failures = impl->failed_test_count();
  if (!impl->Passed()) {
    const int failed_test_count = impl->failed_test_count();
    ColoredPrintf(COLOR_RED,  "[  FAILED  ] ");
    printf("%s, listed below:\n", FormatTestCount(failed_test_count).c_str());
    internal::PrintFailedTestsPretty(impl);
    printf("\n%2d FAILED %s\n", num_failures,
                        num_failures == 1 ? "TEST" : "TESTS");
  }

  int num_disabled = impl->disabled_test_count();
  if (num_disabled) {
    if (!num_failures) {
      printf("\n");  // Add a spacer if no FAILURE banner is displayed.
    }
    ColoredPrintf(COLOR_YELLOW,
                  "  YOU HAVE %d DISABLED %s\n\n",
                  num_disabled,
                  num_disabled == 1 ? "TEST" : "TESTS");
  }
  // Ensure that Google Test output is printed before, e.g., heapchecker output.
  fflush(stdout);
}

// End PrettyUnitTestResultPrinter

// class UnitTestEventsRepeater
//
// This class forwards events to other event listeners.
class UnitTestEventsRepeater : public UnitTestEventListenerInterface {
 public:
  typedef internal::List<UnitTestEventListenerInterface *> Listeners;
  typedef internal::ListNode<UnitTestEventListenerInterface *> ListenersNode;
  UnitTestEventsRepeater() {}
  virtual ~UnitTestEventsRepeater();
  void AddListener(UnitTestEventListenerInterface *listener);

  virtual void OnUnitTestStart(const UnitTest* unit_test);
  virtual void OnUnitTestEnd(const UnitTest* unit_test);
  virtual void OnGlobalSetUpStart(const UnitTest* unit_test);
  virtual void OnGlobalSetUpEnd(const UnitTest* unit_test);
  virtual void OnGlobalTearDownStart(const UnitTest* unit_test);
  virtual void OnGlobalTearDownEnd(const UnitTest* unit_test);
  virtual void OnTestCaseStart(const TestCase* test_case);
  virtual void OnTestCaseEnd(const TestCase* test_case);
  virtual void OnTestStart(const TestInfo* test_info);
  virtual void OnTestEnd(const TestInfo* test_info);
  virtual void OnNewTestPartResult(const TestPartResult* result);

 private:
  Listeners listeners_;

  GTEST_DISALLOW_COPY_AND_ASSIGN(UnitTestEventsRepeater);
};

UnitTestEventsRepeater::~UnitTestEventsRepeater() {
  for (ListenersNode* listener = listeners_.Head();
       listener != NULL;
       listener = listener->next()) {
    delete listener->element();
  }
}

void UnitTestEventsRepeater::AddListener(
    UnitTestEventListenerInterface *listener) {
  listeners_.PushBack(listener);
}

// Since the methods are identical, use a macro to reduce boilerplate.
// This defines a member that repeats the call to all listeners.
#define GTEST_REPEATER_METHOD(Name, Type) \
void UnitTestEventsRepeater::Name(const Type* parameter) { \
  for (ListenersNode* listener = listeners_.Head(); \
       listener != NULL; \
       listener = listener->next()) { \
    listener->element()->Name(parameter); \
  } \
}

GTEST_REPEATER_METHOD(OnUnitTestStart, UnitTest)
GTEST_REPEATER_METHOD(OnUnitTestEnd, UnitTest)
GTEST_REPEATER_METHOD(OnGlobalSetUpStart, UnitTest)
GTEST_REPEATER_METHOD(OnGlobalSetUpEnd, UnitTest)
GTEST_REPEATER_METHOD(OnGlobalTearDownStart, UnitTest)
GTEST_REPEATER_METHOD(OnGlobalTearDownEnd, UnitTest)
GTEST_REPEATER_METHOD(OnTestCaseStart, TestCase)
GTEST_REPEATER_METHOD(OnTestCaseEnd, TestCase)
GTEST_REPEATER_METHOD(OnTestStart, TestInfo)
GTEST_REPEATER_METHOD(OnTestEnd, TestInfo)
GTEST_REPEATER_METHOD(OnNewTestPartResult, TestPartResult)

#undef GTEST_REPEATER_METHOD

// End PrettyUnitTestResultPrinter

// This class generates an XML output file.
class XmlUnitTestResultPrinter : public UnitTestEventListenerInterface {
 public:
  explicit XmlUnitTestResultPrinter(const char* output_file);

  virtual void OnUnitTestEnd(const UnitTest* unit_test);

 private:
  // Is c a whitespace character that is normalized to a space character
  // when it appears in an XML attribute value?
  static bool IsNormalizableWhitespace(char c) {
    return c == 0x9 || c == 0xA || c == 0xD;
  }

  // May c appear in a well-formed XML document?
  static bool IsValidXmlCharacter(char c) {
    return IsNormalizableWhitespace(c) || c >= 0x20;
  }

  // Returns an XML-escaped copy of the input string str.  If
  // is_attribute is true, the text is meant to appear as an attribute
  // value, and normalizable whitespace is preserved by replacing it
  // with character references.
  static internal::String EscapeXml(const char* str,
                                    bool is_attribute);

  // Convenience wrapper around EscapeXml when str is an attribute value.
  static internal::String EscapeXmlAttribute(const char* str) {
    return EscapeXml(str, true);
  }

  // Convenience wrapper around EscapeXml when str is not an attribute value.
  static internal::String EscapeXmlText(const char* str) {
    return EscapeXml(str, false);
  }

  // Prints an XML representation of a TestInfo object.
  static void PrintXmlTestInfo(FILE* out,
                               const char* test_case_name,
                               const TestInfo* test_info);

  // Prints an XML representation of a TestCase object
  static void PrintXmlTestCase(FILE* out, const TestCase* test_case);

  // Prints an XML summary of unit_test to output stream out.
  static void PrintXmlUnitTest(FILE* out, const UnitTest* unit_test);

  // Produces a string representing the test properties in a result as space
  // delimited XML attributes based on the property key="value" pairs.
  // When the String is not empty, it includes a space at the beginning,
  // to delimit this attribute from prior attributes.
  static internal::String TestPropertiesAsXmlAttributes(
      const internal::TestResult* result);

  // The output file.
  const internal::String output_file_;

  GTEST_DISALLOW_COPY_AND_ASSIGN(XmlUnitTestResultPrinter);
};

// Creates a new XmlUnitTestResultPrinter.
XmlUnitTestResultPrinter::XmlUnitTestResultPrinter(const char* output_file)
    : output_file_(output_file) {
  if (output_file_.c_str() == NULL || output_file_.empty()) {
    fprintf(stderr, "XML output file may not be null\n");
    fflush(stderr);
    exit(EXIT_FAILURE);
  }
}

// Called after the unit test ends.
void XmlUnitTestResultPrinter::OnUnitTestEnd(const UnitTest* unit_test) {
  FILE* xmlout = NULL;
  internal::FilePath output_file(output_file_);
  internal::FilePath output_dir(output_file.RemoveFileName());

  if (output_dir.CreateDirectoriesRecursively()) {
  // MSVC 8 deprecates fopen(), so we want to suppress warning 4996
  // (deprecated function) there.
#ifdef GTEST_OS_WINDOWS
  // We are on Windows.
#pragma warning(push)          // Saves the current warning state.
#pragma warning(disable:4996)  // Temporarily disables warning 4996.
    xmlout = fopen(output_file_.c_str(), "w");
#pragma warning(pop)           // Restores the warning state.
#else  // We are on Linux or Mac OS.
    xmlout = fopen(output_file_.c_str(), "w");
#endif  // GTEST_OS_WINDOWS
  }
  if (xmlout == NULL) {
    // TODO(wan): report the reason of the failure.
    //
    // We don't do it for now as:
    //
    //   1. There is no urgent need for it.
    //   2. It's a bit involved to make the errno variable thread-safe on
    //      all three operating systems (Linux, Windows, and Mac OS).
    //   3. To interpret the meaning of errno in a thread-safe way,
    //      we need the strerror_r() function, which is not available on
    //      Windows.
    fprintf(stderr,
            "Unable to open file \"%s\"\n",
            output_file_.c_str());
    fflush(stderr);
    exit(EXIT_FAILURE);
  }
  PrintXmlUnitTest(xmlout, unit_test);
  fclose(xmlout);
}

// Returns an XML-escaped copy of the input string str.  If is_attribute
// is true, the text is meant to appear as an attribute value, and
// normalizable whitespace is preserved by replacing it with character
// references.
//
// Invalid XML characters in str, if any, are stripped from the output.
// It is expected that most, if not all, of the text processed by this
// module will consist of ordinary English text.
// If this module is ever modified to produce version 1.1 XML output,
// most invalid characters can be retained using character references.
// TODO(wan): It might be nice to have a minimally invasive, human-readable
// escaping scheme for invalid characters, rather than dropping them.
internal::String XmlUnitTestResultPrinter::EscapeXml(const char* str,
                                                     bool is_attribute) {
  Message m;

  if (str != NULL) {
    for (const char* src = str; *src; ++src) {
      switch (*src) {
        case '<':
          m << "&lt;";
          break;
        case '>':
          m << "&gt;";
          break;
        case '&':
          m << "&amp;";
          break;
        case '\'':
          if (is_attribute)
            m << "&apos;";
          else
            m << '\'';
          break;
        case '"':
          if (is_attribute)
            m << "&quot;";
          else
            m << '"';
          break;
        default:
          if (IsValidXmlCharacter(*src)) {
            if (is_attribute && IsNormalizableWhitespace(*src))
              m << internal::String::Format("&#x%02X;", unsigned(*src));
            else
              m << *src;
          }
          break;
      }
    }
  }

  return m.GetString();
}


// The following routines generate an XML representation of a UnitTest
// object.
//
// This is how Google Test concepts map to the DTD:
//
// <testsuite name="AllTests">         <-- corresponds to a UnitTest object
//   <testsuite name="testcase-name">  <-- corresponds to a TestCase object
//     <testcase name="test-name">     <-- corresponds to a TestInfo object
//       <failure message="..." />
//       <failure message="..." />     <-- individual assertion failures
//       <failure message="..." />
//     </testcase>
//   </testsuite>
// </testsuite>

// Prints an XML representation of a TestInfo object.
// TODO(wan): There is also value in printing properties with the plain printer.
void XmlUnitTestResultPrinter::PrintXmlTestInfo(FILE* out,
                                                const char* test_case_name,
                                                const TestInfo* test_info) {
  const internal::TestResult * const result = test_info->result();
  const internal::List<TestPartResult> &results = result->test_part_results();
  fprintf(out,
          "    <testcase name=\"%s\" status=\"%s\" time=\"%s\" "
          "classname=\"%s\"%s",
          EscapeXmlAttribute(test_info->name()).c_str(),
          test_info->should_run() ? "run" : "notrun",
          internal::StreamableToString(result->elapsed_time()).c_str(),
          EscapeXmlAttribute(test_case_name).c_str(),
          TestPropertiesAsXmlAttributes(result).c_str());

  int failures = 0;
  for (const internal::ListNode<TestPartResult>* part_node = results.Head();
       part_node != NULL;
       part_node = part_node->next()) {
    const TestPartResult& part = part_node->element();
    if (part.failed()) {
      const internal::String message =
          internal::String::Format("%s:%d\n%s", part.file_name(),
                                   part.line_number(), part.message());
      if (++failures == 1)
        fprintf(out, ">\n");
      fprintf(out,
              "      <failure message=\"%s\" type=\"\"/>\n",
              EscapeXmlAttribute(message.c_str()).c_str());
    }
  }

  if (failures == 0)
    fprintf(out, " />\n");
  else
    fprintf(out, "    </testcase>\n");
}

// Prints an XML representation of a TestCase object
void XmlUnitTestResultPrinter::PrintXmlTestCase(FILE* out,
                                                const TestCase* test_case) {
  fprintf(out,
          "  <testsuite name=\"%s\" tests=\"%d\" failures=\"%d\" "
          "disabled=\"%d\" ",
          EscapeXmlAttribute(test_case->name()).c_str(),
          test_case->total_test_count(),
          test_case->failed_test_count(),
          test_case->disabled_test_count());
  fprintf(out,
          "errors=\"0\" time=\"%s\">\n",
          internal::StreamableToString(test_case->elapsed_time()).c_str());
  for (const internal::ListNode<TestInfo*>* info_node =
         test_case->test_info_list().Head();
       info_node != NULL;
       info_node = info_node->next()) {
    PrintXmlTestInfo(out, test_case->name(), info_node->element());
  }
  fprintf(out, "  </testsuite>\n");
}

// Prints an XML summary of unit_test to output stream out.
void XmlUnitTestResultPrinter::PrintXmlUnitTest(FILE* out,
                                                const UnitTest* unit_test) {
  const internal::UnitTestImpl* const impl = unit_test->impl();
  fprintf(out, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
  fprintf(out,
          "<testsuite tests=\"%d\" failures=\"%d\" disabled=\"%d\" "
          "errors=\"0\" time=\"%s\" ",
          impl->total_test_count(),
          impl->failed_test_count(),
          impl->disabled_test_count(),
          internal::StreamableToString(impl->elapsed_time()).c_str());
  fprintf(out, "name=\"AllTests\">\n");
  for (const internal::ListNode<TestCase*>* case_node =
       impl->test_cases()->Head();
       case_node != NULL;
       case_node = case_node->next()) {
    PrintXmlTestCase(out, case_node->element());
  }
  fprintf(out, "</testsuite>\n");
}

// Produces a string representing the test properties in a result as space
// delimited XML attributes based on the property key="value" pairs.
internal::String XmlUnitTestResultPrinter::TestPropertiesAsXmlAttributes(
    const internal::TestResult* result) {
  using internal::TestProperty;
  Message attributes;
  const internal::List<TestProperty>& properties = result->test_properties();
  for (const internal::ListNode<TestProperty>* property_node =
       properties.Head();
       property_node != NULL;
       property_node = property_node->next()) {
    const TestProperty& property = property_node->element();
    attributes << " " << property.key() << "="
        << "\"" << EscapeXmlAttribute(property.value()) << "\"";
  }
  return attributes.GetString();
}

// End XmlUnitTestResultPrinter

namespace internal {

// Class ScopedTrace

// Pushes the given source file location and message onto a per-thread
// trace stack maintained by Google Test.
// L < UnitTest::mutex_
ScopedTrace::ScopedTrace(const char* file, int line, const Message& message) {
  TraceInfo trace;
  trace.file = file;
  trace.line = line;
  trace.message = message.GetString();

  UnitTest::GetInstance()->PushGTestTrace(trace);
}

// Pops the info pushed by the c'tor.
// L < UnitTest::mutex_
ScopedTrace::~ScopedTrace() {
  UnitTest::GetInstance()->PopGTestTrace();
}


// class OsStackTraceGetter

// Returns the current OS stack trace as a String.  Parameters:
//
//   max_depth  - the maximum number of stack frames to be included
//                in the trace.
//   skip_count - the number of top frames to be skipped; doesn't count
//                against max_depth.
//
// L < mutex_
// We use "L < mutex_" to denote that the function may acquire mutex_.
String OsStackTraceGetter::CurrentStackTrace(int, int) {
  return String("");
}

// L < mutex_
void OsStackTraceGetter::UponLeavingGTest() {
}

const char* const
OsStackTraceGetter::kElidedFramesMarker =
    "... " GTEST_NAME " internal frames ...";

}  // namespace internal

// class UnitTest

// Gets the singleton UnitTest object.  The first time this method is
// called, a UnitTest object is constructed and returned.  Consecutive
// calls will return the same object.
//
// We don't protect this under mutex_ as a user is not supposed to
// call this before main() starts, from which point on the return
// value will never change.
UnitTest * UnitTest::GetInstance() {
  // When compiled with MSVC 7.1 in optimized mode, destroying the
  // UnitTest object upon exiting the program messes up the exit code,
  // causing successful tests to appear failed.  We have to use a
  // different implementation in this case to bypass the compiler bug.
  // This implementation makes the compiler happy, at the cost of
  // leaking the UnitTest object.
#if _MSC_VER == 1310 && !defined(_DEBUG)  // MSVC 7.1 and optimized build.
  static UnitTest* const instance = new UnitTest;
  return instance;
#else
  static UnitTest instance;
  return &instance;
#endif  // _MSC_VER==1310 && !defined(_DEBUG)
}

// Registers and returns a global test environment.  When a test
// program is run, all global test environments will be set-up in the
// order they were registered.  After all tests in the program have
// finished, all global test environments will be torn-down in the
// *reverse* order they were registered.
//
// The UnitTest object takes ownership of the given environment.
//
// We don't protect this under mutex_, as we only support calling it
// from the main thread.
Environment* UnitTest::AddEnvironment(Environment* env) {
  if (env == NULL) {
    return NULL;
  }

  impl_->environments()->PushBack(env);
  impl_->environments_in_reverse_order()->PushFront(env);
  return env;
}

// Adds a TestPartResult to the current TestResult object.  All Google Test
// assertion macros (e.g. ASSERT_TRUE, EXPECT_EQ, etc) eventually call
// this to report their results.  The user code should use the
// assertion macros instead of calling this directly.
// L < mutex_
void UnitTest::AddTestPartResult(TestPartResultType result_type,
                                 const char* file_name,
                                 int line_number,
                                 const internal::String& message,
                                 const internal::String& os_stack_trace) {
  Message msg;
  msg << message;

  internal::MutexLock lock(&mutex_);
  if (impl_->gtest_trace_stack()->size() > 0) {
    msg << "\n" << GTEST_NAME << " trace:";

    for (internal::ListNode<internal::TraceInfo>* node =
         impl_->gtest_trace_stack()->Head();
         node != NULL;
         node = node->next()) {
      const internal::TraceInfo& trace = node->element();
      msg << "\n" << trace.file << ":" << trace.line << ": " << trace.message;
    }
  }

  if (os_stack_trace.c_str() != NULL && !os_stack_trace.empty()) {
    msg << "\nStack trace:\n" << os_stack_trace;
  }

  const TestPartResult result =
    TestPartResult(result_type, file_name, line_number,
                   msg.GetString().c_str());
  impl_->test_part_result_reporter()->ReportTestPartResult(result);

  // If this is a failure and the user wants the debugger to break on
  // failures ...
  if (result_type != TPRT_SUCCESS && GTEST_FLAG(break_on_failure)) {
    // ... then we generate a seg fault.
    *static_cast<int*>(NULL) = 1;
  }
}

// Creates and adds a property to the current TestResult. If a property matching
// the supplied value already exists, updates its value instead.
void UnitTest::RecordPropertyForCurrentTest(const char* key,
                                            const char* value) {
  const internal::TestProperty test_property(key, value);
  impl_->current_test_result()->RecordProperty(test_property);
}

// Runs all tests in this UnitTest object and prints the result.
// Returns 0 if successful, or 1 otherwise.
//
// We don't protect this under mutex_, as we only support calling it
// from the main thread.
int UnitTest::Run() {
#ifdef GTEST_OS_WINDOWS

#if !defined(_WIN32_WCE)
  // SetErrorMode doesn't exist on CE.
  if (GTEST_FLAG(catch_exceptions)) {
    // The user wants Google Test to catch exceptions thrown by the tests.

    // This lets fatal errors be handled by us, instead of causing pop-ups.
    SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOALIGNMENTFAULTEXCEPT |
                 SEM_NOGPFAULTERRORBOX | SEM_NOOPENFILEERRORBOX);
  }
#endif  // _WIN32_WCE

  __try {
    return impl_->RunAllTests();
  } __except(internal::UnitTestOptions::GTestShouldProcessSEH(
      GetExceptionCode())) {
    printf("Exception thrown with code 0x%x.\nFAIL\n", GetExceptionCode());
    fflush(stdout);
    return 1;
  }

#else
  // We are on Linux or Mac OS.  There is no exception of any kind.

  return impl_->RunAllTests();
#endif  // GTEST_OS_WINDOWS
}

// Returns the TestCase object for the test that's currently running,
// or NULL if no test is running.
// L < mutex_
const TestCase* UnitTest::current_test_case() const {
  internal::MutexLock lock(&mutex_);
  return impl_->current_test_case();
}

// Returns the TestInfo object for the test that's currently running,
// or NULL if no test is running.
// L < mutex_
const TestInfo* UnitTest::current_test_info() const {
  internal::MutexLock lock(&mutex_);
  return impl_->current_test_info();
}

// Creates an empty UnitTest.
UnitTest::UnitTest() {
  impl_ = new internal::UnitTestImpl(this);
}

// Destructor of UnitTest.
UnitTest::~UnitTest() {
  delete impl_;
}

// Pushes a trace defined by SCOPED_TRACE() on to the per-thread
// Google Test trace stack.
// L < mutex_
void UnitTest::PushGTestTrace(const internal::TraceInfo& trace) {
  internal::MutexLock lock(&mutex_);
  impl_->gtest_trace_stack()->PushFront(trace);
}

// Pops a trace from the per-thread Google Test trace stack.
// L < mutex_
void UnitTest::PopGTestTrace() {
  internal::MutexLock lock(&mutex_);
  impl_->gtest_trace_stack()->PopFront(NULL);
}

namespace internal {

UnitTestImpl::UnitTestImpl(UnitTest* parent)
    : parent_(parent),
      test_cases_(),
      last_death_test_case_(NULL),
      current_test_case_(NULL),
      current_test_info_(NULL),
      ad_hoc_test_result_(),
      result_printer_(NULL),
      os_stack_trace_getter_(NULL),
#ifdef GTEST_HAS_DEATH_TEST
      elapsed_time_(0),
      internal_run_death_test_flag_(NULL),
      death_test_factory_(new DefaultDeathTestFactory) {
#else
      elapsed_time_(0) {
#endif  // GTEST_HAS_DEATH_TEST
  // We do the assignment here instead of in the initializer list, as
  // doing that latter causes MSVC to issue a warning about using
  // 'this' in initializers.
  test_part_result_reporter_ = this;
}

UnitTestImpl::~UnitTestImpl() {
  // Deletes every TestCase.
  test_cases_.ForEach(DeleteTestCase);

  // Deletes every Environment.
  environments_.ForEach(DeleteEnvironment);

  // Deletes the current test result printer.
  delete result_printer_;

  delete os_stack_trace_getter_;
}

// A predicate that checks the name of a TestCase against a known
// value.
//
// This is used for implementation of the UnitTest class only.  We put
// it in the anonymous namespace to prevent polluting the outer
// namespace.
//
// TestCaseNameIs is copyable.
class TestCaseNameIs {
 public:
  // Constructor.
  explicit TestCaseNameIs(const String& name)
      : name_(name) {}

  // Returns true iff the name of test_case matches name_.
  bool operator()(const TestCase* test_case) const {
    return test_case != NULL && strcmp(test_case->name(), name_.c_str()) == 0;
  }

 private:
  String name_;
};

// Finds and returns a TestCase with the given name.  If one doesn't
// exist, creates one and returns it.
//
// Arguments:
//
//   test_case_name: name of the test case
//   set_up_tc:      pointer to the function that sets up the test case
//   tear_down_tc:   pointer to the function that tears down the test case
TestCase* UnitTestImpl::GetTestCase(const char* test_case_name,
                                    Test::SetUpTestCaseFunc set_up_tc,
                                    Test::TearDownTestCaseFunc tear_down_tc) {
  // Can we find a TestCase with the given name?
  internal::ListNode<TestCase*>* node = test_cases_.FindIf(
      TestCaseNameIs(test_case_name));

  if (node == NULL) {
    // No.  Let's create one.
    TestCase* const test_case =
      new TestCase(test_case_name, set_up_tc, tear_down_tc);

    // Is this a death test case?
    if (String(test_case_name).EndsWith("DeathTest")) {
      // Yes.  Inserts the test case after the last death test case
      // defined so far.
      node = test_cases_.InsertAfter(last_death_test_case_, test_case);
      last_death_test_case_ = node;
    } else {
      // No.  Appends to the end of the list.
      test_cases_.PushBack(test_case);
      node = test_cases_.Last();
    }
  }

  // Returns the TestCase found.
  return node->element();
}

// Helpers for setting up / tearing down the given environment.  They
// are for use in the List::ForEach() method.
static void SetUpEnvironment(Environment* env) { env->SetUp(); }
static void TearDownEnvironment(Environment* env) { env->TearDown(); }

// Runs all tests in this UnitTest object, prints the result, and
// returns 0 if all tests are successful, or 1 otherwise.  If any
// exception is thrown during a test on Windows, this test is
// considered to be failed, but the rest of the tests will still be
// run.  (We disable exceptions on Linux and Mac OS X, so the issue
// doesn't apply there.)
int UnitTestImpl::RunAllTests() {
  // True iff Google Test is initialized before RUN_ALL_TESTS() is called.
  const bool gtest_is_initialized_before_run_all_tests = GTestIsInitialized();

  // Lists all the tests and exits if the --gtest_list_tests
  // flag was specified.
  if (GTEST_FLAG(list_tests)) {
    ListAllTests();
    return 0;
  }

  // True iff we are in a subprocess for running a thread-safe-style
  // death test.
  bool in_subprocess_for_death_test = false;

#ifdef GTEST_HAS_DEATH_TEST
  internal_run_death_test_flag_.reset(ParseInternalRunDeathTestFlag());
  in_subprocess_for_death_test = (internal_run_death_test_flag_.get() != NULL);
#endif  // GTEST_HAS_DEATH_TEST

  UnitTestEventListenerInterface * const printer = result_printer();

  // Compares the full test names with the filter to decide which
  // tests to run.
  const bool has_tests_to_run = FilterTests() > 0;
  // True iff at least one test has failed.
  bool failed = false;

  // How many times to repeat the tests?  We don't want to repeat them
  // when we are inside the subprocess of a death test.
  const int repeat = in_subprocess_for_death_test ? 1 : GTEST_FLAG(repeat);
  // Repeats forever if the repeat count is negative.
  const bool forever = repeat < 0;
  for (int i = 0; forever || i != repeat; i++) {
    if (repeat != 1) {
      printf("\nRepeating all tests (iteration %d) . . .\n\n", i + 1);
    }

    // Tells the unit test event listener that the tests are about to
    // start.
    printer->OnUnitTestStart(parent_);

    const TimeInMillis start = GetTimeInMillis();

    // Runs each test case if there is at least one test to run.
    if (has_tests_to_run) {
      // Sets up all environments beforehand.
      printer->OnGlobalSetUpStart(parent_);
      environments_.ForEach(SetUpEnvironment);
      printer->OnGlobalSetUpEnd(parent_);

      // Runs the tests only if there was no fatal failure during global
      // set-up.
      if (!Test::HasFatalFailure()) {
        test_cases_.ForEach(TestCase::RunTestCase);
      }

      // Tears down all environments in reverse order afterwards.
      printer->OnGlobalTearDownStart(parent_);
      environments_in_reverse_order_.ForEach(TearDownEnvironment);
      printer->OnGlobalTearDownEnd(parent_);
    }

    elapsed_time_ = GetTimeInMillis() - start;

    // Tells the unit test event listener that the tests have just
    // finished.
    printer->OnUnitTestEnd(parent_);

    // Gets the result and clears it.
    if (!Passed()) {
      failed = true;
    }
    ClearResult();
  }

  if (!gtest_is_initialized_before_run_all_tests) {
    ColoredPrintf(
        COLOR_RED, "\nIMPORTANT NOTICE - DO NOT IGNORE:\n"
        "This test program did NOT call %s() before calling RUN_ALL_TESTS(). "
        "This is INVALID. Soon " GTEST_NAME
        " will start to enforce the valid usage. "
        "Please fix it ASAP, or IT WILL START TO FAIL.\n",
        "testing::ParseGTestFlags"
        );  // NOLINT
  }

  // Returns 0 if all tests passed, or 1 other wise.
  return failed ? 1 : 0;
}

// Compares the name of each test with the user-specified filter to
// decide whether the test should be run, then records the result in
// each TestCase and TestInfo object.
// Returns the number of tests that should run.
int UnitTestImpl::FilterTests() {
  int num_runnable_tests = 0;
  for (const internal::ListNode<TestCase *> *test_case_node =
       test_cases_.Head();
       test_case_node != NULL;
       test_case_node = test_case_node->next()) {
    TestCase * const test_case = test_case_node->element();
    const String &test_case_name = test_case->name();
    test_case->set_should_run(false);

    for (const internal::ListNode<TestInfo *> *test_info_node =
           test_case->test_info_list().Head();
         test_info_node != NULL;
         test_info_node = test_info_node->next()) {
      TestInfo * const test_info = test_info_node->element();
      const String test_name(test_info->name());
      // A test is disabled if test case name or test name matches
      // kDisableTestPattern.
      const bool is_disabled =
        internal::UnitTestOptions::PatternMatchesString(kDisableTestPattern,
            test_case_name.c_str()) ||
        internal::UnitTestOptions::PatternMatchesString(kDisableTestPattern,
            test_name.c_str());
      test_info->impl()->set_is_disabled(is_disabled);

      const bool should_run = !is_disabled &&
          internal::UnitTestOptions::FilterMatchesTest(test_case_name,
                                                       test_name);
      test_info->impl()->set_should_run(should_run);
      test_case->set_should_run(test_case->should_run() || should_run);
      if (should_run) {
        num_runnable_tests++;
      }
    }
  }
  return num_runnable_tests;
}

// Lists all tests by name.
void UnitTestImpl::ListAllTests() {
  for (const internal::ListNode<TestCase*>* test_case_node = test_cases_.Head();
       test_case_node != NULL;
       test_case_node = test_case_node->next()) {
    const TestCase* const test_case = test_case_node->element();

    // Prints the test case name following by an indented list of test nodes.
    printf("%s.\n", test_case->name());

    for (const internal::ListNode<TestInfo*>* test_info_node =
         test_case->test_info_list().Head();
         test_info_node != NULL;
         test_info_node = test_info_node->next()) {
      const TestInfo* const test_info = test_info_node->element();

      printf("  %s\n", test_info->name());
    }
  }
  fflush(stdout);
}

// Sets the unit test result printer.
//
// Does nothing if the input and the current printer object are the
// same; otherwise, deletes the old printer object and makes the
// input the current printer.
void UnitTestImpl::set_result_printer(
    UnitTestEventListenerInterface* result_printer) {
  if (result_printer_ != result_printer) {
    delete result_printer_;
    result_printer_ = result_printer;
  }
}

// Returns the current unit test result printer if it is not NULL;
// otherwise, creates an appropriate result printer, makes it the
// current printer, and returns it.
UnitTestEventListenerInterface* UnitTestImpl::result_printer() {
  if (result_printer_ != NULL) {
    return result_printer_;
  }

#ifdef GTEST_HAS_DEATH_TEST
  if (internal_run_death_test_flag_.get() != NULL) {
    result_printer_ = new NullUnitTestResultPrinter;
    return result_printer_;
  }
#endif  // GTEST_HAS_DEATH_TEST

  UnitTestEventsRepeater *repeater = new UnitTestEventsRepeater;
  const String& output_format = internal::UnitTestOptions::GetOutputFormat();
  if (output_format == "xml") {
    repeater->AddListener(new XmlUnitTestResultPrinter(
        internal::UnitTestOptions::GetOutputFile().c_str()));
  } else if (output_format != "") {
      printf("WARNING: unrecognized output format \"%s\" ignored.\n",
             output_format.c_str());
      fflush(stdout);
  }
  repeater->AddListener(new PrettyUnitTestResultPrinter);
  result_printer_ = repeater;
  return result_printer_;
}

// Sets the OS stack trace getter.
//
// Does nothing if the input and the current OS stack trace getter are
// the same; otherwise, deletes the old getter and makes the input the
// current getter.
void UnitTestImpl::set_os_stack_trace_getter(
    OsStackTraceGetterInterface* getter) {
  if (os_stack_trace_getter_ != getter) {
    delete os_stack_trace_getter_;
    os_stack_trace_getter_ = getter;
  }
}

// Returns the current OS stack trace getter if it is not NULL;
// otherwise, creates an OsStackTraceGetter, makes it the current
// getter, and returns it.
OsStackTraceGetterInterface* UnitTestImpl::os_stack_trace_getter() {
  if (os_stack_trace_getter_ == NULL) {
    os_stack_trace_getter_ = new OsStackTraceGetter;
  }

  return os_stack_trace_getter_;
}

// Returns the TestResult for the test that's currently running, or
// the TestResult for the ad hoc test if no test is running.
internal::TestResult* UnitTestImpl::current_test_result() {
  return current_test_info_ ?
    current_test_info_->impl()->result() : &ad_hoc_test_result_;
}

// TestInfoImpl constructor.
TestInfoImpl::TestInfoImpl(TestInfo* parent,
                           const char* test_case_name,
                           const char* name,
                           TypeId fixture_class_id,
                           TestMaker maker) :
    parent_(parent),
    test_case_name_(String(test_case_name)),
    name_(String(name)),
    fixture_class_id_(fixture_class_id),
    should_run_(false),
    is_disabled_(false),
    maker_(maker) {
}

// TestInfoImpl destructor.
TestInfoImpl::~TestInfoImpl() {
}

}  // namespace internal

namespace internal {

// Parses a string as a command line flag.  The string should have
// the format "--flag=value".  When def_optional is true, the "=value"
// part can be omitted.
//
// Returns the value of the flag, or NULL if the parsing failed.
const char* ParseFlagValue(const char* str,
                           const char* flag,
                           bool def_optional) {
  // str and flag must not be NULL.
  if (str == NULL || flag == NULL) return NULL;

  // The flag must start with "--" followed by GTEST_FLAG_PREFIX.
  const String flag_str = String::Format("--%s%s", GTEST_FLAG_PREFIX, flag);
  const size_t flag_len = flag_str.GetLength();
  if (strncmp(str, flag_str.c_str(), flag_len) != 0) return NULL;

  // Skips the flag name.
  const char* flag_end = str + flag_len;

  // When def_optional is true, it's OK to not have a "=value" part.
  if (def_optional && (flag_end[0] == '\0')) {
    return flag_end;
  }

  // If def_optional is true and there are more characters after the
  // flag name, or if def_optional is false, there must be a '=' after
  // the flag name.
  if (flag_end[0] != '=') return NULL;

  // Returns the string after "=".
  return flag_end + 1;
}

// Parses a string for a bool flag, in the form of either
// "--flag=value" or "--flag".
//
// In the former case, the value is taken as true as long as it does
// not start with '0', 'f', or 'F'.
//
// In the latter case, the value is taken as true.
//
// On success, stores the value of the flag in *value, and returns
// true.  On failure, returns false without changing *value.
bool ParseBoolFlag(const char* str, const char* flag, bool* value) {
  // Gets the value of the flag as a string.
  const char* const value_str = ParseFlagValue(str, flag, true);

  // Aborts if the parsing failed.
  if (value_str == NULL) return false;

  // Converts the string value to a bool.
  *value = !(*value_str == '0' || *value_str == 'f' || *value_str == 'F');
  return true;
}

// Parses a string for an Int32 flag, in the form of
// "--flag=value".
//
// On success, stores the value of the flag in *value, and returns
// true.  On failure, returns false without changing *value.
bool ParseInt32Flag(const char* str, const char* flag, Int32* value) {
  // Gets the value of the flag as a string.
  const char* const value_str = ParseFlagValue(str, flag, false);

  // Aborts if the parsing failed.
  if (value_str == NULL) return false;

  // Sets *value to the value of the flag.
  return ParseInt32(Message() << "The value of flag --" << flag,
                    value_str, value);
}

// Parses a string for a string flag, in the form of
// "--flag=value".
//
// On success, stores the value of the flag in *value, and returns
// true.  On failure, returns false without changing *value.
bool ParseStringFlag(const char* str, const char* flag, String* value) {
  // Gets the value of the flag as a string.
  const char* const value_str = ParseFlagValue(str, flag, false);

  // Aborts if the parsing failed.
  if (value_str == NULL) return false;

  // Sets *value to the value of the flag.
  *value = value_str;
  return true;
}

// The internal implementation of ParseGTestFlags().
//
// The type parameter CharType can be instantiated to either char or
// wchar_t.
template <typename CharType>
void ParseGTestFlagsImpl(int* argc, CharType** argv) {
  g_parse_gtest_flags_called = true;
  if (*argc <= 0) return;

#ifdef GTEST_HAS_DEATH_TEST
  g_argvs.clear();
  for (int i = 0; i != *argc; i++) {
    g_argvs.push_back(StreamableToString(argv[i]));
  }
#endif  // GTEST_HAS_DEATH_TEST

  for (int i = 1; i != *argc; i++) {
    const String arg_string = StreamableToString(argv[i]);
    const char* const arg = arg_string.c_str();

    using internal::ParseBoolFlag;
    using internal::ParseInt32Flag;
    using internal::ParseStringFlag;

    // Do we see a Google Test flag?
    if (ParseBoolFlag(arg, kBreakOnFailureFlag,
                      &GTEST_FLAG(break_on_failure)) ||
        ParseBoolFlag(arg, kCatchExceptionsFlag,
                      &GTEST_FLAG(catch_exceptions)) ||
        ParseStringFlag(arg, kColorFlag, &GTEST_FLAG(color)) ||
        ParseStringFlag(arg, kDeathTestStyleFlag,
                        &GTEST_FLAG(death_test_style)) ||
        ParseStringFlag(arg, kFilterFlag, &GTEST_FLAG(filter)) ||
        ParseStringFlag(arg, kInternalRunDeathTestFlag,
                        &GTEST_FLAG(internal_run_death_test)) ||
        ParseBoolFlag(arg, kListTestsFlag, &GTEST_FLAG(list_tests)) ||
        ParseStringFlag(arg, kOutputFlag, &GTEST_FLAG(output)) ||
        ParseInt32Flag(arg, kRepeatFlag, &GTEST_FLAG(repeat))
        ) {
      // Yes.  Shift the remainder of the argv list left by one.  Note
      // that argv has (*argc + 1) elements, the last one always being
      // NULL.  The following loop moves the trailing NULL element as
      // well.
      for (int j = i; j != *argc; j++) {
        argv[j] = argv[j + 1];
      }

      // Decrements the argument count.
      (*argc)--;

      // We also need to decrement the iterator as we just removed
      // an element.
      i--;
    }
  }
}

}  // namespace internal

// Parses a command line for the flags that Google Test recognizes.
// Whenever a Google Test flag is seen, it is removed from argv, and *argc
// is decremented.
//
// No value is returned.  Instead, the Google Test flag variables are
// updated.
void ParseGTestFlags(int* argc, char** argv) {
  internal::g_executable_path = argv[0];
  internal::ParseGTestFlagsImpl(argc, argv);
}

// This overloaded version can be used in Windows programs compiled in
// UNICODE mode.
#ifdef GTEST_OS_WINDOWS
void ParseGTestFlags(int* argc, wchar_t** argv) {
  // g_executable_path uses normal characters rather than wide chars, so call
  // StreamableToString to convert argv[0] to normal characters (utf8 encoding).
  internal::g_executable_path = internal::StreamableToString(argv[0]);
  internal::ParseGTestFlagsImpl(argc, argv);
}
#endif  // GTEST_OS_WINDOWS

}  // namespace testing
