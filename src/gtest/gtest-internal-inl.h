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

// Utility functions and classes used by the Google C++ testing framework.
//
// Author: wan@google.com (Zhanyong Wan)
//
// This file contains purely Google Test's internal implementation.  Please
// DO NOT #INCLUDE IT IN A USER PROGRAM.

#ifndef GTEST_SRC_GTEST_INTERNAL_INL_H_
#define GTEST_SRC_GTEST_INTERNAL_INL_H_

// GTEST_IMPLEMENTATION is defined iff the current translation unit is
// part of Google Test's implementation.
#ifndef GTEST_IMPLEMENTATION
// A user is trying to include this from his code - just say no.
#error "gtest-internal-inl.h is part of Google Test's internal implementation."
#error "It must not be included except by Google Test itself."
#endif  // GTEST_IMPLEMENTATION

#include <stddef.h>

#include <gtest/internal/gtest-port.h>

#ifdef GTEST_OS_WINDOWS
#include <windows.h>  // NOLINT
#endif  // GTEST_OS_WINDOWS

#include <ostream>  // NOLINT
#include <gtest/gtest.h>
#include <gtest/gtest-spi.h>

namespace testing {

// Declares the flags.
//
// We don't want the users to modify these flags in the code, but want
// Google Test's own unit tests to be able to access them.  Therefore we
// declare them here as opposed to in gtest.h.
GTEST_DECLARE_bool(break_on_failure);
GTEST_DECLARE_bool(catch_exceptions);
GTEST_DECLARE_string(color);
GTEST_DECLARE_string(filter);
GTEST_DECLARE_bool(list_tests);
GTEST_DECLARE_string(output);
GTEST_DECLARE_int32(repeat);
GTEST_DECLARE_int32(stack_trace_depth);
GTEST_DECLARE_bool(show_internal_stack_frames);

namespace internal {

// Names of the flags (needed for parsing Google Test flags).
const char kBreakOnFailureFlag[] = "break_on_failure";
const char kCatchExceptionsFlag[] = "catch_exceptions";
const char kFilterFlag[] = "filter";
const char kListTestsFlag[] = "list_tests";
const char kOutputFlag[] = "output";
const char kColorFlag[] = "color";
const char kRepeatFlag[] = "repeat";

// This class saves the values of all Google Test flags in its c'tor, and
// restores them in its d'tor.
class GTestFlagSaver {
 public:
  // The c'tor.
  GTestFlagSaver() {
    break_on_failure_ = GTEST_FLAG(break_on_failure);
    catch_exceptions_ = GTEST_FLAG(catch_exceptions);
    color_ = GTEST_FLAG(color);
    death_test_style_ = GTEST_FLAG(death_test_style);
    filter_ = GTEST_FLAG(filter);
    internal_run_death_test_ = GTEST_FLAG(internal_run_death_test);
    list_tests_ = GTEST_FLAG(list_tests);
    output_ = GTEST_FLAG(output);
    repeat_ = GTEST_FLAG(repeat);
  }

  // The d'tor is not virtual.  DO NOT INHERIT FROM THIS CLASS.
  ~GTestFlagSaver() {
    GTEST_FLAG(break_on_failure) = break_on_failure_;
    GTEST_FLAG(catch_exceptions) = catch_exceptions_;
    GTEST_FLAG(color) = color_;
    GTEST_FLAG(death_test_style) = death_test_style_;
    GTEST_FLAG(filter) = filter_;
    GTEST_FLAG(internal_run_death_test) = internal_run_death_test_;
    GTEST_FLAG(list_tests) = list_tests_;
    GTEST_FLAG(output) = output_;
    GTEST_FLAG(repeat) = repeat_;
  }
 private:
  // Fields for saving the original values of flags.
  bool break_on_failure_;
  bool catch_exceptions_;
  String color_;
  String death_test_style_;
  String filter_;
  String internal_run_death_test_;
  bool list_tests_;
  String output_;
  bool pretty_;
  internal::Int32 repeat_;
} GTEST_ATTRIBUTE_UNUSED;

// Converts a Unicode code-point to its UTF-8 encoding.
String ToUtf8String(wchar_t wchar);

// Returns the number of active threads, or 0 when there is an error.
size_t GetThreadCount();

// List is a simple singly-linked list container.
//
// We cannot use std::list as Microsoft's implementation of STL has
// problems when exception is disabled.  There is a hack to work
// around this, but we've seen cases where the hack fails to work.
//
// TODO(wan): switch to std::list when we have a reliable fix for the
// STL problem, e.g. when we upgrade to the next version of Visual
// C++, or (more likely) switch to STLport.
//
// The element type must support copy constructor.

// Forward declare List
template <typename E>  // E is the element type.
class List;

// ListNode is a node in a singly-linked list.  It consists of an
// element and a pointer to the next node.  The last node in the list
// has a NULL value for its next pointer.
template <typename E>  // E is the element type.
class ListNode {
  friend class List<E>;

 private:

  E element_;
  ListNode * next_;

  // The c'tor is private s.t. only in the ListNode class and in its
  // friend class List we can create a ListNode object.
  //
  // Creates a node with a given element value.  The next pointer is
  // set to NULL.
  //
  // ListNode does NOT have a default constructor.  Always use this
  // constructor (with parameter) to create a ListNode object.
  explicit ListNode(const E & element) : element_(element), next_(NULL) {}

  // We disallow copying ListNode
  GTEST_DISALLOW_COPY_AND_ASSIGN(ListNode);

 public:

  // Gets the element in this node.
  E & element() { return element_; }
  const E & element() const { return element_; }

  // Gets the next node in the list.
  ListNode * next() { return next_; }
  const ListNode * next() const { return next_; }
};


// List is a simple singly-linked list container.
template <typename E>  // E is the element type.
class List {
 public:

  // Creates an empty list.
  List() : head_(NULL), last_(NULL), size_(0) {}

  // D'tor.
  virtual ~List();

  // Clears the list.
  void Clear() {
    if ( size_ > 0 ) {
      // 1. Deletes every node.
      ListNode<E> * node = head_;
      ListNode<E> * next = node->next();
      for ( ; ; ) {
        delete node;
        node = next;
        if ( node == NULL ) break;
        next = node->next();
      }

      // 2. Resets the member variables.
      head_ = last_ = NULL;
      size_ = 0;
    }
  }

  // Gets the number of elements.
  int size() const { return size_; }

  // Returns true if the list is empty.
  bool IsEmpty() const { return size() == 0; }

  // Gets the first element of the list, or NULL if the list is empty.
  ListNode<E> * Head() { return head_; }
  const ListNode<E> * Head() const { return head_; }

  // Gets the last element of the list, or NULL if the list is empty.
  ListNode<E> * Last() { return last_; }
  const ListNode<E> * Last() const { return last_; }

  // Adds an element to the end of the list.  A copy of the element is
  // created using the copy constructor, and then stored in the list.
  // Changes made to the element in the list doesn't affect the source
  // object, and vice versa.
  void PushBack(const E & element) {
    ListNode<E> * new_node = new ListNode<E>(element);

    if ( size_ == 0 ) {
      head_ = last_ = new_node;
      size_ = 1;
    } else {
      last_->next_ = new_node;
      last_ = new_node;
      size_++;
    }
  }

  // Adds an element to the beginning of this list.
  void PushFront(const E& element) {
    ListNode<E>* const new_node = new ListNode<E>(element);

    if ( size_ == 0 ) {
      head_ = last_ = new_node;
      size_ = 1;
    } else {
      new_node->next_ = head_;
      head_ = new_node;
      size_++;
    }
  }

  // Removes an element from the beginning of this list.  If the
  // result argument is not NULL, the removed element is stored in the
  // memory it points to.  Otherwise the element is thrown away.
  // Returns true iff the list wasn't empty before the operation.
  bool PopFront(E* result) {
    if (size_ == 0) return false;

    if (result != NULL) {
      *result = head_->element_;
    }

    ListNode<E>* const old_head = head_;
    size_--;
    if (size_ == 0) {
      head_ = last_ = NULL;
    } else {
      head_ = head_->next_;
    }
    delete old_head;

    return true;
  }

  // Inserts an element after a given node in the list.  It's the
  // caller's responsibility to ensure that the given node is in the
  // list.  If the given node is NULL, inserts the element at the
  // front of the list.
  ListNode<E>* InsertAfter(ListNode<E>* node, const E& element) {
    if (node == NULL) {
      PushFront(element);
      return Head();
    }

    ListNode<E>* const new_node = new ListNode<E>(element);
    new_node->next_ = node->next_;
    node->next_ = new_node;
    size_++;
    if (node == last_) {
      last_ = new_node;
    }

    return new_node;
  }

  // Returns the number of elements that satisfy a given predicate.
  // The parameter 'predicate' is a Boolean function or functor that
  // accepts a 'const E &', where E is the element type.
  template <typename P>  // P is the type of the predicate function/functor
  int CountIf(P predicate) const {
    int count = 0;
    for ( const ListNode<E> * node = Head();
          node != NULL;
          node = node->next() ) {
      if ( predicate(node->element()) ) {
        count++;
      }
    }

    return count;
  }

  // Applies a function/functor to each element in the list.  The
  // parameter 'functor' is a function/functor that accepts a 'const
  // E &', where E is the element type.  This method does not change
  // the elements.
  template <typename F>  // F is the type of the function/functor
  void ForEach(F functor) const {
    for ( const ListNode<E> * node = Head();
          node != NULL;
          node = node->next() ) {
      functor(node->element());
    }
  }

  // Returns the first node whose element satisfies a given predicate,
  // or NULL if none is found.  The parameter 'predicate' is a
  // function/functor that accepts a 'const E &', where E is the
  // element type.  This method does not change the elements.
  template <typename P>  // P is the type of the predicate function/functor.
  const ListNode<E> * FindIf(P predicate) const {
    for ( const ListNode<E> * node = Head();
          node != NULL;
          node = node->next() ) {
      if ( predicate(node->element()) ) {
        return node;
      }
    }

    return NULL;
  }

  template <typename P>
  ListNode<E> * FindIf(P predicate) {
    for ( ListNode<E> * node = Head();
          node != NULL;
          node = node->next() ) {
      if ( predicate(node->element() ) ) {
        return node;
      }
    }

    return NULL;
  }

 private:
  ListNode<E>* head_;  // The first node of the list.
  ListNode<E>* last_;  // The last node of the list.
  int size_;           // The number of elements in the list.

  // We disallow copying List.
  GTEST_DISALLOW_COPY_AND_ASSIGN(List);
};

// The virtual destructor of List.
template <typename E>
List<E>::~List() {
  Clear();
}

// A function for deleting an object.  Handy for being used as a
// functor.
template <typename T>
static void Delete(T * x) {
  delete x;
}

// A copyable object representing a user specified test property which can be
// output as a key/value string pair.
//
// Don't inherit from TestProperty as its destructor is not virtual.
class TestProperty {
 public:
  // C'tor.  TestProperty does NOT have a default constructor.
  // Always use this constructor (with parameters) to create a
  // TestProperty object.
  TestProperty(const char* key, const char* value) :
    key_(key), value_(value) {
  }

  // Gets the user supplied key.
  const char* key() const {
    return key_.c_str();
  }

  // Gets the user supplied value.
  const char* value() const {
    return value_.c_str();
  }

  // Sets a new value, overriding the one supplied in the constructor.
  void SetValue(const char* new_value) {
    value_ = new_value;
  }

 private:
  // The key supplied by the user.
  String key_;
  // The value supplied by the user.
  String value_;
};

// A predicate that checks the key of a TestProperty against a known key.
//
// TestPropertyKeyIs is copyable.
class TestPropertyKeyIs {
 public:
  // Constructor.
  //
  // TestPropertyKeyIs has NO default constructor.
  explicit TestPropertyKeyIs(const char* key)
      : key_(key) {}

  // Returns true iff the test name of test property matches on key_.
  bool operator()(const TestProperty& test_property) const {
    return String(test_property.key()).Compare(key_) == 0;
  }

 private:
  String key_;
};

// The result of a single Test.  This includes a list of
// TestPartResults, a list of TestProperties, a count of how many
// death tests there are in the Test, and how much time it took to run
// the Test.
//
// TestResult is not copyable.
class TestResult {
 public:
  // Creates an empty TestResult.
  TestResult();

  // D'tor.  Do not inherit from TestResult.
  ~TestResult();

  // Gets the list of TestPartResults.
  const internal::List<TestPartResult> & test_part_results() const {
    return test_part_results_;
  }

  // Gets the list of TestProperties.
  const internal::List<internal::TestProperty> & test_properties() const {
    return test_properties_;
  }

  // Gets the number of successful test parts.
  int successful_part_count() const;

  // Gets the number of failed test parts.
  int failed_part_count() const;

  // Gets the number of all test parts.  This is the sum of the number
  // of successful test parts and the number of failed test parts.
  int total_part_count() const;

  // Returns true iff the test passed (i.e. no test part failed).
  bool Passed() const { return !Failed(); }

  // Returns true iff the test failed.
  bool Failed() const { return failed_part_count() > 0; }

  // Returns true iff the test fatally failed.
  bool HasFatalFailure() const;

  // Returns the elapsed time, in milliseconds.
  TimeInMillis elapsed_time() const { return elapsed_time_; }

  // Sets the elapsed time.
  void set_elapsed_time(TimeInMillis elapsed) { elapsed_time_ = elapsed; }

  // Adds a test part result to the list.
  void AddTestPartResult(const TestPartResult& test_part_result);

  // Adds a test property to the list. The property is validated and may add
  // a non-fatal failure if invalid (e.g., if it conflicts with reserved
  // key names). If a property is already recorded for the same key, the
  // value will be updated, rather than storing multiple values for the same
  // key.
  void RecordProperty(const internal::TestProperty& test_property);

  // Adds a failure if the key is a reserved attribute of Google Test
  // testcase tags.  Returns true if the property is valid.
  // TODO(russr): Validate attribute names are legal and human readable.
  static bool ValidateTestProperty(const internal::TestProperty& test_property);

  // Returns the death test count.
  int death_test_count() const { return death_test_count_; }

  // Increments the death test count, returning the new count.
  int increment_death_test_count() { return ++death_test_count_; }

  // Clears the object.
  void Clear();
 private:
  // Protects mutable state of the property list and of owned properties, whose
  // values may be updated.
  internal::Mutex test_properites_mutex_;

  // The list of TestPartResults
  internal::List<TestPartResult> test_part_results_;
  // The list of TestProperties
  internal::List<internal::TestProperty> test_properties_;
  // Running count of death tests.
  int death_test_count_;
  // The elapsed time, in milliseconds.
  TimeInMillis elapsed_time_;

  // We disallow copying TestResult.
  GTEST_DISALLOW_COPY_AND_ASSIGN(TestResult);
};  // class TestResult

class TestInfoImpl {
 public:
  TestInfoImpl(TestInfo* parent, const char* test_case_name,
               const char* name, TypeId fixture_class_id,
               TestMaker maker);
  ~TestInfoImpl();

  // Returns true if this test should run.
  bool should_run() const { return should_run_; }

  // Sets the should_run member.
  void set_should_run(bool should) { should_run_ = should; }

  // Returns true if this test is disabled. Disabled tests are not run.
  bool is_disabled() const { return is_disabled_; }

  // Sets the is_disabled member.
  void set_is_disabled(bool is) { is_disabled_ = is; }

  // Returns the test case name.
  const char* test_case_name() const { return test_case_name_.c_str(); }

  // Returns the test name.
  const char* name() const { return name_.c_str(); }

  // Returns the ID of the test fixture class.
  TypeId fixture_class_id() const { return fixture_class_id_; }

  // Returns the test result.
  internal::TestResult* result() { return &result_; }
  const internal::TestResult* result() const { return &result_; }

  // Creates the test object, runs it, records its result, and then
  // deletes it.
  void Run();

  // Calls the given TestInfo object's Run() method.
  static void RunTest(TestInfo * test_info) {
    test_info->impl()->Run();
  }

  // Clears the test result.
  void ClearResult() { result_.Clear(); }

  // Clears the test result in the given TestInfo object.
  static void ClearTestResult(TestInfo * test_info) {
    test_info->impl()->ClearResult();
  }

 private:
  // These fields are immutable properties of the test.
  TestInfo* const parent_;         // The owner of this object
  const String test_case_name_;    // Test case name
  const String name_;              // Test name
  const TypeId fixture_class_id_;  // ID of the test fixture class
  bool should_run_;                // True iff this test should run
  bool is_disabled_;               // True iff this test is disabled
  const TestMaker maker_;          // The function that creates the test object

  // This field is mutable and needs to be reset before running the
  // test for the second time.
  internal::TestResult result_;

  GTEST_DISALLOW_COPY_AND_ASSIGN(TestInfoImpl);
};

}  // namespace internal

// A test case, which consists of a list of TestInfos.
//
// TestCase is not copyable.
class TestCase {
 public:
  // Creates a TestCase with the given name.
  //
  // TestCase does NOT have a default constructor.  Always use this
  // constructor to create a TestCase object.
  //
  // Arguments:
  //
  //   name:         name of the test case
  //   set_up_tc:    pointer to the function that sets up the test case
  //   tear_down_tc: pointer to the function that tears down the test case
  TestCase(const char* name,
           Test::SetUpTestCaseFunc set_up_tc,
           Test::TearDownTestCaseFunc tear_down_tc);

  // Destructor of TestCase.
  virtual ~TestCase();

  // Gets the name of the TestCase.
  const char* name() const { return name_.c_str(); }

  // Returns true if any test in this test case should run.
  bool should_run() const { return should_run_; }

  // Sets the should_run member.
  void set_should_run(bool should) { should_run_ = should; }

  // Gets the (mutable) list of TestInfos in this TestCase.
  internal::List<TestInfo*>& test_info_list() { return *test_info_list_; }

  // Gets the (immutable) list of TestInfos in this TestCase.
  const internal::List<TestInfo *> & test_info_list() const {
    return *test_info_list_;
  }

  // Gets the number of successful tests in this test case.
  int successful_test_count() const;

  // Gets the number of failed tests in this test case.
  int failed_test_count() const;

  // Gets the number of disabled tests in this test case.
  int disabled_test_count() const;

  // Get the number of tests in this test case that should run.
  int test_to_run_count() const;

  // Gets the number of all tests in this test case.
  int total_test_count() const;

  // Returns true iff the test case passed.
  bool Passed() const { return !Failed(); }

  // Returns true iff the test case failed.
  bool Failed() const { return failed_test_count() > 0; }

  // Returns the elapsed time, in milliseconds.
  internal::TimeInMillis elapsed_time() const { return elapsed_time_; }

  // Adds a TestInfo to this test case.  Will delete the TestInfo upon
  // destruction of the TestCase object.
  void AddTestInfo(TestInfo * test_info);

  // Finds and returns a TestInfo with the given name.  If one doesn't
  // exist, returns NULL.
  TestInfo* GetTestInfo(const char* test_name);

  // Clears the results of all tests in this test case.
  void ClearResult();

  // Clears the results of all tests in the given test case.
  static void ClearTestCaseResult(TestCase* test_case) {
    test_case->ClearResult();
  }

  // Runs every test in this TestCase.
  void Run();

  // Runs every test in the given TestCase.
  static void RunTestCase(TestCase * test_case) { test_case->Run(); }

  // Returns true iff test passed.
  static bool TestPassed(const TestInfo * test_info) {
    const internal::TestInfoImpl* const impl = test_info->impl();
    return impl->should_run() && impl->result()->Passed();
  }

  // Returns true iff test failed.
  static bool TestFailed(const TestInfo * test_info) {
    const internal::TestInfoImpl* const impl = test_info->impl();
    return impl->should_run() && impl->result()->Failed();
  }

  // Returns true iff test is disabled.
  static bool TestDisabled(const TestInfo * test_info) {
    return test_info->impl()->is_disabled();
  }

  // Returns true if the given test should run.
  static bool ShouldRunTest(const TestInfo *test_info) {
    return test_info->impl()->should_run();
  }

 private:
  // Name of the test case.
  internal::String name_;
  // List of TestInfos.
  internal::List<TestInfo*>* test_info_list_;
  // Pointer to the function that sets up the test case.
  Test::SetUpTestCaseFunc set_up_tc_;
  // Pointer to the function that tears down the test case.
  Test::TearDownTestCaseFunc tear_down_tc_;
  // True iff any test in this test case should run.
  bool should_run_;
  // Elapsed time, in milliseconds.
  internal::TimeInMillis elapsed_time_;

  // We disallow copying TestCases.
  GTEST_DISALLOW_COPY_AND_ASSIGN(TestCase);
};

namespace internal {

// Class UnitTestOptions.
//
// This class contains functions for processing options the user
// specifies when running the tests.  It has only static members.
//
// In most cases, the user can specify an option using either an
// environment variable or a command line flag.  E.g. you can set the
// test filter using either GTEST_FILTER or --gtest_filter.  If both
// the variable and the flag are present, the latter overrides the
// former.
class UnitTestOptions {
 public:
  // Functions for processing the gtest_output flag.

  // Returns the output format, or "" for normal printed output.
  static String GetOutputFormat();

  // Returns the name of the requested output file, or the default if none
  // was explicitly specified.
  static String GetOutputFile();

  // Functions for processing the gtest_filter flag.

  // Returns true iff the wildcard pattern matches the string.  The
  // first ':' or '\0' character in pattern marks the end of it.
  //
  // This recursive algorithm isn't very efficient, but is clear and
  // works well enough for matching test names, which are short.
  static bool PatternMatchesString(const char *pattern, const char *str);

  // Returns true iff the user-specified filter matches the test case
  // name and the test name.
  static bool FilterMatchesTest(const String &test_case_name,
                                const String &test_name);

#ifdef GTEST_OS_WINDOWS
  // Function for supporting the gtest_catch_exception flag.

  // Returns EXCEPTION_EXECUTE_HANDLER if Google Test should handle the
  // given SEH exception, or EXCEPTION_CONTINUE_SEARCH otherwise.
  // This function is useful as an __except condition.
  static int GTestShouldProcessSEH(DWORD exception_code);
#endif  // GTEST_OS_WINDOWS
 private:
  // Returns true if "name" matches the ':' separated list of glob-style
  // filters in "filter".
  static bool MatchesFilter(const String& name, const char* filter);
};

// Returns the current application's name, removing directory path if that
// is present.  Used by UnitTestOptions::GetOutputFile.
FilePath GetCurrentExecutableName();

// The role interface for getting the OS stack trace as a string.
class OsStackTraceGetterInterface {
 public:
  OsStackTraceGetterInterface() {}
  virtual ~OsStackTraceGetterInterface() {}

  // Returns the current OS stack trace as a String.  Parameters:
  //
  //   max_depth  - the maximum number of stack frames to be included
  //                in the trace.
  //   skip_count - the number of top frames to be skipped; doesn't count
  //                against max_depth.
  virtual String CurrentStackTrace(int max_depth, int skip_count) = 0;

  // UponLeavingGTest() should be called immediately before Google Test calls
  // user code. It saves some information about the current stack that
  // CurrentStackTrace() will use to find and hide Google Test stack frames.
  virtual void UponLeavingGTest() = 0;

 private:
  GTEST_DISALLOW_COPY_AND_ASSIGN(OsStackTraceGetterInterface);
};

// A working implemenation of the OsStackTraceGetterInterface interface.
class OsStackTraceGetter : public OsStackTraceGetterInterface {
 public:
  OsStackTraceGetter() {}
  virtual String CurrentStackTrace(int max_depth, int skip_count);
  virtual void UponLeavingGTest();

  // This string is inserted in place of stack frames that are part of
  // Google Test's implementation.
  static const char* const kElidedFramesMarker;

 private:
  Mutex mutex_;  // protects all internal state

  // We save the stack frame below the frame that calls user code.
  // We do this because the address of the frame immediately below
  // the user code changes between the call to UponLeavingGTest()
  // and any calls to CurrentStackTrace() from within the user code.
  void* caller_frame_;

  GTEST_DISALLOW_COPY_AND_ASSIGN(OsStackTraceGetter);
};

// Information about a Google Test trace point.
struct TraceInfo {
  const char* file;
  int line;
  String message;
};

// The private implementation of the UnitTest class.  We don't protect
// the methods under a mutex, as this class is not accessible by a
// user and the UnitTest class that delegates work to this class does
// proper locking.
class UnitTestImpl : public TestPartResultReporterInterface {
 public:
  explicit UnitTestImpl(UnitTest* parent);
  virtual ~UnitTestImpl();

  // Reports a test part result.  This method is from the
  // TestPartResultReporterInterface interface.
  virtual void ReportTestPartResult(const TestPartResult& result);

  // Returns the current test part result reporter.
  TestPartResultReporterInterface* test_part_result_reporter();

  // Sets the current test part result reporter.
  void set_test_part_result_reporter(TestPartResultReporterInterface* reporter);

  // Gets the number of successful test cases.
  int successful_test_case_count() const;

  // Gets the number of failed test cases.
  int failed_test_case_count() const;

  // Gets the number of all test cases.
  int total_test_case_count() const;

  // Gets the number of all test cases that contain at least one test
  // that should run.
  int test_case_to_run_count() const;

  // Gets the number of successful tests.
  int successful_test_count() const;

  // Gets the number of failed tests.
  int failed_test_count() const;

  // Gets the number of disabled tests.
  int disabled_test_count() const;

  // Gets the number of all tests.
  int total_test_count() const;

  // Gets the number of tests that should run.
  int test_to_run_count() const;

  // Gets the elapsed time, in milliseconds.
  TimeInMillis elapsed_time() const { return elapsed_time_; }

  // Returns true iff the unit test passed (i.e. all test cases passed).
  bool Passed() const { return !Failed(); }

  // Returns true iff the unit test failed (i.e. some test case failed
  // or something outside of all tests failed).
  bool Failed() const {
    return failed_test_case_count() > 0 || ad_hoc_test_result()->Failed();
  }

  // Returns the TestResult for the test that's currently running, or
  // the TestResult for the ad hoc test if no test is running.
  internal::TestResult* current_test_result();

  // Returns the TestResult for the ad hoc test.
  const internal::TestResult* ad_hoc_test_result() const {
    return &ad_hoc_test_result_;
  }

  // Sets the unit test result printer.
  //
  // Does nothing if the input and the current printer object are the
  // same; otherwise, deletes the old printer object and makes the
  // input the current printer.
  void set_result_printer(UnitTestEventListenerInterface * result_printer);

  // Returns the current unit test result printer if it is not NULL;
  // otherwise, creates an appropriate result printer, makes it the
  // current printer, and returns it.
  UnitTestEventListenerInterface* result_printer();

  // Sets the OS stack trace getter.
  //
  // Does nothing if the input and the current OS stack trace getter
  // are the same; otherwise, deletes the old getter and makes the
  // input the current getter.
  void set_os_stack_trace_getter(OsStackTraceGetterInterface* getter);

  // Returns the current OS stack trace getter if it is not NULL;
  // otherwise, creates an OsStackTraceGetter, makes it the current
  // getter, and returns it.
  OsStackTraceGetterInterface* os_stack_trace_getter();

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
  String CurrentOsStackTraceExceptTop(int skip_count);

  // Finds and returns a TestCase with the given name.  If one doesn't
  // exist, creates one and returns it.
  //
  // Arguments:
  //
  //   test_case_name: name of the test case
  //   set_up_tc:      pointer to the function that sets up the test case
  //   tear_down_tc:   pointer to the function that tears down the test case
  TestCase* GetTestCase(const char* test_case_name,
                        Test::SetUpTestCaseFunc set_up_tc,
                        Test::TearDownTestCaseFunc tear_down_tc);

  // Adds a TestInfo to the unit test.
  //
  // Arguments:
  //
  //   set_up_tc:    pointer to the function that sets up the test case
  //   tear_down_tc: pointer to the function that tears down the test case
  //   test_info:    the TestInfo object
  void AddTestInfo(Test::SetUpTestCaseFunc set_up_tc,
                   Test::TearDownTestCaseFunc tear_down_tc,
                   TestInfo * test_info) {
    GetTestCase(test_info->test_case_name(),
                set_up_tc,
                tear_down_tc)->AddTestInfo(test_info);
  }

  // Sets the TestCase object for the test that's currently running.
  void set_current_test_case(TestCase* current_test_case) {
    current_test_case_ = current_test_case;
  }

  // Sets the TestInfo object for the test that's currently running.  If
  // current_test_info is NULL, the assertion results will be stored in
  // ad_hoc_test_result_.
  void set_current_test_info(TestInfo* current_test_info) {
    current_test_info_ = current_test_info;
  }

  // Runs all tests in this UnitTest object, prints the result, and
  // returns 0 if all tests are successful, or 1 otherwise.  If any
  // exception is thrown during a test on Windows, this test is
  // considered to be failed, but the rest of the tests will still be
  // run.  (We disable exceptions on Linux and Mac OS X, so the issue
  // doesn't apply there.)
  int RunAllTests();

  // Clears the results of all tests, including the ad hoc test.
  void ClearResult() {
    test_cases_.ForEach(TestCase::ClearTestCaseResult);
    ad_hoc_test_result_.Clear();
  }

  // Matches the full name of each test against the user-specified
  // filter to decide whether the test should run, then records the
  // result in each TestCase and TestInfo object.
  // Returns the number of tests that should run.
  int FilterTests();

  // Lists all the tests by name.
  void ListAllTests();

  const TestCase* current_test_case() const { return current_test_case_; }
  TestInfo* current_test_info() { return current_test_info_; }
  const TestInfo* current_test_info() const { return current_test_info_; }

  // Returns the list of environments that need to be set-up/torn-down
  // before/after the tests are run.
  internal::List<Environment*>* environments() { return &environments_; }
  internal::List<Environment*>* environments_in_reverse_order() {
    return &environments_in_reverse_order_;
  }

  internal::List<TestCase*>* test_cases() { return &test_cases_; }
  const internal::List<TestCase*>* test_cases() const { return &test_cases_; }

  // Getters for the per-thread Google Test trace stack.
  internal::List<TraceInfo>* gtest_trace_stack() {
    return gtest_trace_stack_.pointer();
  }
  const internal::List<TraceInfo>* gtest_trace_stack() const {
    return gtest_trace_stack_.pointer();
  }

#ifdef GTEST_HAS_DEATH_TEST
  // Returns a pointer to the parsed --gtest_internal_run_death_test
  // flag, or NULL if that flag was not specified.
  // This information is useful only in a death test child process.
  const InternalRunDeathTestFlag* internal_run_death_test_flag() const {
    return internal_run_death_test_flag_.get();
  }

  // Returns a pointer to the current death test factory.
  internal::DeathTestFactory* death_test_factory() {
    return death_test_factory_.get();
  }

  friend class ReplaceDeathTestFactory;
#endif  // GTEST_HAS_DEATH_TEST

 private:
  // The UnitTest object that owns this implementation object.
  UnitTest* const parent_;

  // Points to (but doesn't own) the test part result reporter.
  TestPartResultReporterInterface* test_part_result_reporter_;

  // The list of environments that need to be set-up/torn-down
  // before/after the tests are run.  environments_in_reverse_order_
  // simply mirrors environments_ in reverse order.
  internal::List<Environment*> environments_;
  internal::List<Environment*> environments_in_reverse_order_;

  internal::List<TestCase*> test_cases_;  // The list of TestCases.

  // Points to the last death test case registered.  Initially NULL.
  internal::ListNode<TestCase*>* last_death_test_case_;

  // This points to the TestCase for the currently running test.  It
  // changes as Google Test goes through one test case after another.
  // When no test is running, this is set to NULL and Google Test
  // stores assertion results in ad_hoc_test_result_.  Initally NULL.
  TestCase* current_test_case_;

  // This points to the TestInfo for the currently running test.  It
  // changes as Google Test goes through one test after another.  When
  // no test is running, this is set to NULL and Google Test stores
  // assertion results in ad_hoc_test_result_.  Initially NULL.
  TestInfo* current_test_info_;

  // Normally, a user only writes assertions inside a TEST or TEST_F,
  // or inside a function called by a TEST or TEST_F.  Since Google
  // Test keeps track of which test is current running, it can
  // associate such an assertion with the test it belongs to.
  //
  // If an assertion is encountered when no TEST or TEST_F is running,
  // Google Test attributes the assertion result to an imaginary "ad hoc"
  // test, and records the result in ad_hoc_test_result_.
  internal::TestResult ad_hoc_test_result_;

  // The unit test result printer.  Will be deleted when the UnitTest
  // object is destructed.  By default, a plain text printer is used,
  // but the user can set this field to use a custom printer if that
  // is desired.
  UnitTestEventListenerInterface* result_printer_;

  // The OS stack trace getter.  Will be deleted when the UnitTest
  // object is destructed.  By default, an OsStackTraceGetter is used,
  // but the user can set this field to use a custom getter if that is
  // desired.
  OsStackTraceGetterInterface* os_stack_trace_getter_;

  // How long the test took to run, in milliseconds.
  TimeInMillis elapsed_time_;

#ifdef GTEST_HAS_DEATH_TEST
  // The decomposed components of the gtest_internal_run_death_test flag,
  // parsed when RUN_ALL_TESTS is called.
  internal::scoped_ptr<InternalRunDeathTestFlag> internal_run_death_test_flag_;
  internal::scoped_ptr<internal::DeathTestFactory> death_test_factory_;
#endif  // GTEST_HAS_DEATH_TEST

  // A per-thread stack of traces created by the SCOPED_TRACE() macro.
  internal::ThreadLocal<internal::List<TraceInfo> > gtest_trace_stack_;

  GTEST_DISALLOW_COPY_AND_ASSIGN(UnitTestImpl);
};  // class UnitTestImpl

// Convenience function for accessing the global UnitTest
// implementation object.
inline UnitTestImpl* GetUnitTestImpl() {
  return UnitTest::GetInstance()->impl();
}

}  // namespace internal
}  // namespace testing

#endif  // GTEST_SRC_GTEST_INTERNAL_INL_H_
