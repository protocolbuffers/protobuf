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
// This header file declares functions and macros used internally by
// Google Test.  They are subject to change without notice.

#ifndef GTEST_INCLUDE_GTEST_INTERNAL_GTEST_INTERNAL_H_
#define GTEST_INCLUDE_GTEST_INTERNAL_GTEST_INTERNAL_H_

#if defined(__APPLE__) && !defined(GTEST_NOT_MAC_FRAMEWORK_MODE)
// When using Google Test on the Mac as a framework, all the includes will be
// in the framework headers folder along with gtest.h.
// Define GTEST_NOT_MAC_FRAMEWORK_MODE if you are building Google Test on
// the Mac and are not using it as a framework.
// More info on frameworks available here:
// http://developer.apple.com/documentation/MacOSX/Conceptual/BPFrameworks/
// Concepts/WhatAreFrameworks.html.
#include "gtest-port.h"  // NOLINT
#else
#include <gtest/internal/gtest-port.h>
#endif  // defined(__APPLE__) && !defined(GTEST_NOT_MAC_FRAMEWORK_MODE)

#ifdef GTEST_OS_LINUX
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#endif  // GTEST_OS_LINUX

#include <iomanip>  // NOLINT
#include <limits>   // NOLINT

#if defined(__APPLE__) && !defined(GTEST_NOT_MAC_FRAMEWORK_MODE)
// When using Google Test on the Mac as a framework, all the includes will be
// in the framework headers folder along with gtest.h.
// Define GTEST_NOT_MAC_FRAMEWORK_MODE if you are building Google Test on
// the Mac and are not using it as a framework.
// More info on frameworks available here:
// http://developer.apple.com/documentation/MacOSX/Conceptual/BPFrameworks/
// Concepts/WhatAreFrameworks.html.
#include "gtest-string.h"  // NOLINT
#include "gtest-filepath.h"  // NOLINT
#else
#include <gtest/internal/gtest-string.h>
#include <gtest/internal/gtest-filepath.h>
#endif  // defined(__APPLE__) && !defined(GTEST_NOT_MAC_FRAMEWORK_MODE)

// Due to C++ preprocessor weirdness, we need double indirection to
// concatenate two tokens when one of them is __LINE__.  Writing
//
//   foo ## __LINE__
//
// will result in the token foo__LINE__, instead of foo followed by
// the current line number.  For more details, see
// http://www.parashift.com/c++-faq-lite/misc-technical-issues.html#faq-39.6
#define GTEST_CONCAT_TOKEN(foo, bar) GTEST_CONCAT_TOKEN_IMPL(foo, bar)
#define GTEST_CONCAT_TOKEN_IMPL(foo, bar) foo ## bar

// Google Test defines the testing::Message class to allow construction of
// test messages via the << operator.  The idea is that anything
// streamable to std::ostream can be streamed to a testing::Message.
// This allows a user to use his own types in Google Test assertions by
// overloading the << operator.
//
// util/gtl/stl_logging-inl.h overloads << for STL containers.  These
// overloads cannot be defined in the std namespace, as that will be
// undefined behavior.  Therefore, they are defined in the global
// namespace instead.
//
// C++'s symbol lookup rule (i.e. Koenig lookup) says that these
// overloads are visible in either the std namespace or the global
// namespace, but not other namespaces, including the testing
// namespace which Google Test's Message class is in.
//
// To allow STL containers (and other types that has a << operator
// defined in the global namespace) to be used in Google Test assertions,
// testing::Message must access the custom << operator from the global
// namespace.  Hence this helper function.
//
// Note: Jeffrey Yasskin suggested an alternative fix by "using
// ::operator<<;" in the definition of Message's operator<<.  That fix
// doesn't require a helper function, but unfortunately doesn't
// compile with MSVC.
template <typename T>
inline void GTestStreamToHelper(std::ostream* os, const T& val) {
  *os << val;
}

namespace testing {

// Forward declaration of classes.

class Message;                         // Represents a failure message.
class TestCase;                        // A collection of related tests.
class TestPartResult;                  // Result of a test part.
class TestInfo;                        // Information about a test.
class UnitTest;                        // A collection of test cases.
class UnitTestEventListenerInterface;  // Listens to Google Test events.
class AssertionResult;                 // Result of an assertion.

namespace internal {

struct TraceInfo;                      // Information about a trace point.
class ScopedTrace;                     // Implements scoped trace.
class TestInfoImpl;                    // Opaque implementation of TestInfo
class TestResult;                      // Result of a single Test.
class UnitTestImpl;                    // Opaque implementation of UnitTest

template <typename E> class List;      // A generic list.
template <typename E> class ListNode;  // A node in a generic list.

// A secret type that Google Test users don't know about.  It has no
// definition on purpose.  Therefore it's impossible to create a
// Secret object, which is what we want.
class Secret;

// Two overloaded helpers for checking at compile time whether an
// expression is a null pointer literal (i.e. NULL or any 0-valued
// compile-time integral constant).  Their return values have
// different sizes, so we can use sizeof() to test which version is
// picked by the compiler.  These helpers have no implementations, as
// we only need their signatures.
//
// Given IsNullLiteralHelper(x), the compiler will pick the first
// version if x can be implicitly converted to Secret*, and pick the
// second version otherwise.  Since Secret is a secret and incomplete
// type, the only expression a user can write that has type Secret* is
// a null pointer literal.  Therefore, we know that x is a null
// pointer literal if and only if the first version is picked by the
// compiler.
char IsNullLiteralHelper(Secret* p);
char (&IsNullLiteralHelper(...))[2];  // NOLINT

// A compile-time bool constant that is true if and only if x is a
// null pointer literal (i.e. NULL or any 0-valued compile-time
// integral constant).
#ifdef __SYMBIAN32__  // Symbian
// Passing non-POD classes through ellipsis (...) crashes the ARM compiler.
// The Nokia Symbian compiler tries to instantiate a copy constructor for
// objects passed through ellipsis (...), failing for uncopyable objects.
// Hence we define this to false (and lose support for NULL detection).
#define GTEST_IS_NULL_LITERAL(x) false
#else  // ! __SYMBIAN32__
#define GTEST_IS_NULL_LITERAL(x) \
    (sizeof(::testing::internal::IsNullLiteralHelper(x)) == 1)
#endif  // __SYMBIAN32__

// Appends the user-supplied message to the Google-Test-generated message.
String AppendUserMessage(const String& gtest_msg,
                         const Message& user_msg);

// A helper class for creating scoped traces in user programs.
class ScopedTrace {
 public:
  // The c'tor pushes the given source file location and message onto
  // a trace stack maintained by Google Test.
  ScopedTrace(const char* file, int line, const Message& message);

  // The d'tor pops the info pushed by the c'tor.
  //
  // Note that the d'tor is not virtual in order to be efficient.
  // Don't inherit from ScopedTrace!
  ~ScopedTrace();

 private:
  GTEST_DISALLOW_COPY_AND_ASSIGN(ScopedTrace);
} GTEST_ATTRIBUTE_UNUSED;  // A ScopedTrace object does its job in its
                           // c'tor and d'tor.  Therefore it doesn't
                           // need to be used otherwise.

// Converts a streamable value to a String.  A NULL pointer is
// converted to "(null)".  When the input value is a ::string,
// ::std::string, ::wstring, or ::std::wstring object, each NUL
// character in it is replaced with "\\0".
// Declared here but defined in gtest.h, so that it has access
// to the definition of the Message class, required by the ARM
// compiler.
template <typename T>
String StreamableToString(const T& streamable);

// Formats a value to be used in a failure message.

#ifdef __SYMBIAN32__

// These are needed as the Nokia Symbian Compiler cannot decide between
// const T& and const T* in a function template. The Nokia compiler _can_
// decide between class template specializations for T and T*, so a
// tr1::type_traits-like is_pointer works, and we can overload on that.

// This overload makes sure that all pointers (including
// those to char or wchar_t) are printed as raw pointers.
template <typename T>
inline String FormatValueForFailureMessage(internal::true_type dummy,
                                           T* pointer) {
  return StreamableToString(static_cast<const void*>(pointer));
}

template <typename T>
inline String FormatValueForFailureMessage(internal::false_type dummy,
                                           const T& value) {
  return StreamableToString(value);
}

template <typename T>
inline String FormatForFailureMessage(const T& value) {
  return FormatValueForFailureMessage(
      typename internal::is_pointer<T>::type(), value);
}

#else

template <typename T>
inline String FormatForFailureMessage(const T& value) {
  return StreamableToString(value);
}

// This overload makes sure that all pointers (including
// those to char or wchar_t) are printed as raw pointers.
template <typename T>
inline String FormatForFailureMessage(T* pointer) {
  return StreamableToString(static_cast<const void*>(pointer));
}

#endif  // __SYMBIAN32__

// These overloaded versions handle narrow and wide characters.
String FormatForFailureMessage(char ch);
String FormatForFailureMessage(wchar_t wchar);

// When this operand is a const char* or char*, and the other operand
// is a ::std::string or ::string, we print this operand as a C string
// rather than a pointer.  We do the same for wide strings.

// This internal macro is used to avoid duplicated code.
#define GTEST_FORMAT_IMPL(operand2_type, operand1_printer)\
inline String FormatForComparisonFailureMessage(\
    operand2_type::value_type* str, const operand2_type& operand2) {\
  return operand1_printer(str);\
}\
inline String FormatForComparisonFailureMessage(\
    const operand2_type::value_type* str, const operand2_type& operand2) {\
  return operand1_printer(str);\
}

#if GTEST_HAS_STD_STRING
GTEST_FORMAT_IMPL(::std::string, String::ShowCStringQuoted)
#endif  // GTEST_HAS_STD_STRING
#if GTEST_HAS_STD_WSTRING
GTEST_FORMAT_IMPL(::std::wstring, String::ShowWideCStringQuoted)
#endif  // GTEST_HAS_STD_WSTRING

#if GTEST_HAS_GLOBAL_STRING
GTEST_FORMAT_IMPL(::string, String::ShowCStringQuoted)
#endif  // GTEST_HAS_GLOBAL_STRING
#if GTEST_HAS_GLOBAL_WSTRING
GTEST_FORMAT_IMPL(::wstring, String::ShowWideCStringQuoted)
#endif  // GTEST_HAS_GLOBAL_WSTRING

#undef GTEST_FORMAT_IMPL

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
                          bool ignoring_case);


// This template class represents an IEEE floating-point number
// (either single-precision or double-precision, depending on the
// template parameters).
//
// The purpose of this class is to do more sophisticated number
// comparison.  (Due to round-off error, etc, it's very unlikely that
// two floating-points will be equal exactly.  Hence a naive
// comparison by the == operation often doesn't work.)
//
// Format of IEEE floating-point:
//
//   The most-significant bit being the leftmost, an IEEE
//   floating-point looks like
//
//     sign_bit exponent_bits fraction_bits
//
//   Here, sign_bit is a single bit that designates the sign of the
//   number.
//
//   For float, there are 8 exponent bits and 23 fraction bits.
//
//   For double, there are 11 exponent bits and 52 fraction bits.
//
//   More details can be found at
//   http://en.wikipedia.org/wiki/IEEE_floating-point_standard.
//
// Template parameter:
//
//   RawType: the raw floating-point type (either float or double)
template <typename RawType>
class FloatingPoint {
 public:
  // Defines the unsigned integer type that has the same size as the
  // floating point number.
  typedef typename TypeWithSize<sizeof(RawType)>::UInt Bits;

  // Constants.

  // # of bits in a number.
  static const size_t kBitCount = 8*sizeof(RawType);

  // # of fraction bits in a number.
  static const size_t kFractionBitCount =
    std::numeric_limits<RawType>::digits - 1;

  // # of exponent bits in a number.
  static const size_t kExponentBitCount = kBitCount - 1 - kFractionBitCount;

  // The mask for the sign bit.
  static const Bits kSignBitMask = static_cast<Bits>(1) << (kBitCount - 1);

  // The mask for the fraction bits.
  static const Bits kFractionBitMask =
    ~static_cast<Bits>(0) >> (kExponentBitCount + 1);

  // The mask for the exponent bits.
  static const Bits kExponentBitMask = ~(kSignBitMask | kFractionBitMask);

  // How many ULP's (Units in the Last Place) we want to tolerate when
  // comparing two numbers.  The larger the value, the more error we
  // allow.  A 0 value means that two numbers must be exactly the same
  // to be considered equal.
  //
  // The maximum error of a single floating-point operation is 0.5
  // units in the last place.  On Intel CPU's, all floating-point
  // calculations are done with 80-bit precision, while double has 64
  // bits.  Therefore, 4 should be enough for ordinary use.
  //
  // See the following article for more details on ULP:
  // http://www.cygnus-software.com/papers/comparingfloats/comparingfloats.htm.
  static const size_t kMaxUlps = 4;

  // Constructs a FloatingPoint from a raw floating-point number.
  //
  // On an Intel CPU, passing a non-normalized NAN (Not a Number)
  // around may change its bits, although the new value is guaranteed
  // to be also a NAN.  Therefore, don't expect this constructor to
  // preserve the bits in x when x is a NAN.
  explicit FloatingPoint(const RawType& x) : value_(x) {}

  // Static methods

  // Reinterprets a bit pattern as a floating-point number.
  //
  // This function is needed to test the AlmostEquals() method.
  static RawType ReinterpretBits(const Bits bits) {
    FloatingPoint fp(0);
    fp.bits_ = bits;
    return fp.value_;
  }

  // Returns the floating-point number that represent positive infinity.
  static RawType Infinity() {
    return ReinterpretBits(kExponentBitMask);
  }

  // Non-static methods

  // Returns the bits that represents this number.
  const Bits &bits() const { return bits_; }

  // Returns the exponent bits of this number.
  Bits exponent_bits() const { return kExponentBitMask & bits_; }

  // Returns the fraction bits of this number.
  Bits fraction_bits() const { return kFractionBitMask & bits_; }

  // Returns the sign bit of this number.
  Bits sign_bit() const { return kSignBitMask & bits_; }

  // Returns true iff this is NAN (not a number).
  bool is_nan() const {
    // It's a NAN if the exponent bits are all ones and the fraction
    // bits are not entirely zeros.
    return (exponent_bits() == kExponentBitMask) && (fraction_bits() != 0);
  }

  // Returns true iff this number is at most kMaxUlps ULP's away from
  // rhs.  In particular, this function:
  //
  //   - returns false if either number is (or both are) NAN.
  //   - treats really large numbers as almost equal to infinity.
  //   - thinks +0.0 and -0.0 are 0 DLP's apart.
  bool AlmostEquals(const FloatingPoint& rhs) const {
    // The IEEE standard says that any comparison operation involving
    // a NAN must return false.
    if (is_nan() || rhs.is_nan()) return false;

    return DistanceBetweenSignAndMagnitudeNumbers(bits_, rhs.bits_) <= kMaxUlps;
  }

 private:
  // Converts an integer from the sign-and-magnitude representation to
  // the biased representation.  More precisely, let N be 2 to the
  // power of (kBitCount - 1), an integer x is represented by the
  // unsigned number x + N.
  //
  // For instance,
  //
  //   -N + 1 (the most negative number representable using
  //          sign-and-magnitude) is represented by 1;
  //   0      is represented by N; and
  //   N - 1  (the biggest number representable using
  //          sign-and-magnitude) is represented by 2N - 1.
  //
  // Read http://en.wikipedia.org/wiki/Signed_number_representations
  // for more details on signed number representations.
  static Bits SignAndMagnitudeToBiased(const Bits &sam) {
    if (kSignBitMask & sam) {
      // sam represents a negative number.
      return ~sam + 1;
    } else {
      // sam represents a positive number.
      return kSignBitMask | sam;
    }
  }

  // Given two numbers in the sign-and-magnitude representation,
  // returns the distance between them as an unsigned number.
  static Bits DistanceBetweenSignAndMagnitudeNumbers(const Bits &sam1,
                                                     const Bits &sam2) {
    const Bits biased1 = SignAndMagnitudeToBiased(sam1);
    const Bits biased2 = SignAndMagnitudeToBiased(sam2);
    return (biased1 >= biased2) ? (biased1 - biased2) : (biased2 - biased1);
  }

  union {
    RawType value_;  // The raw floating-point number.
    Bits bits_;      // The bits that represent the number.
  };
};

// Typedefs the instances of the FloatingPoint template class that we
// care to use.
typedef FloatingPoint<float> Float;
typedef FloatingPoint<double> Double;

// In order to catch the mistake of putting tests that use different
// test fixture classes in the same test case, we need to assign
// unique IDs to fixture classes and compare them.  The TypeId type is
// used to hold such IDs.  The user should treat TypeId as an opaque
// type: the only operation allowed on TypeId values is to compare
// them for equality using the == operator.
typedef void* TypeId;

// GetTypeId<T>() returns the ID of type T.  Different values will be
// returned for different types.  Calling the function twice with the
// same type argument is guaranteed to return the same ID.
template <typename T>
inline TypeId GetTypeId() {
  static bool dummy = false;
  // The compiler is required to create an instance of the static
  // variable dummy for each T used to instantiate the template.
  // Therefore, the address of dummy is guaranteed to be unique.
  return &dummy;
}

#ifdef GTEST_OS_WINDOWS

// Predicate-formatters for implementing the HRESULT checking macros
// {ASSERT|EXPECT}_HRESULT_{SUCCEEDED|FAILED}
// We pass a long instead of HRESULT to avoid causing an
// include dependency for the HRESULT type.
AssertionResult IsHRESULTSuccess(const char* expr, long hr);  // NOLINT
AssertionResult IsHRESULTFailure(const char* expr, long hr);  // NOLINT

#endif  // GTEST_OS_WINDOWS

}  // namespace internal
}  // namespace testing

#define GTEST_MESSAGE(message, result_type) \
  ::testing::internal::AssertHelper(result_type, __FILE__, __LINE__, message) \
    = ::testing::Message()

#define GTEST_FATAL_FAILURE(message) \
  return GTEST_MESSAGE(message, ::testing::TPRT_FATAL_FAILURE)

#define GTEST_NONFATAL_FAILURE(message) \
  GTEST_MESSAGE(message, ::testing::TPRT_NONFATAL_FAILURE)

#define GTEST_SUCCESS(message) \
  GTEST_MESSAGE(message, ::testing::TPRT_SUCCESS)

#define GTEST_TEST_BOOLEAN(boolexpr, booltext, actual, expected, fail) \
  GTEST_AMBIGUOUS_ELSE_BLOCKER \
  if (boolexpr) \
    ; \
  else \
    fail("Value of: " booltext "\n  Actual: " #actual "\nExpected: " #expected)

// Helper macro for defining tests.
#define GTEST_TEST(test_case_name, test_name, parent_class)\
class test_case_name##_##test_name##_Test : public parent_class {\
 public:\
  test_case_name##_##test_name##_Test() {}\
  static ::testing::Test* NewTest() {\
    return new test_case_name##_##test_name##_Test;\
  }\
 private:\
  virtual void TestBody();\
  static ::testing::TestInfo* const test_info_;\
  GTEST_DISALLOW_COPY_AND_ASSIGN(test_case_name##_##test_name##_Test);\
};\
\
::testing::TestInfo* const test_case_name##_##test_name##_Test::test_info_ =\
  ::testing::TestInfo::MakeAndRegisterInstance(\
    #test_case_name, \
    #test_name, \
    ::testing::internal::GetTypeId< parent_class >(), \
    parent_class::SetUpTestCase, \
    parent_class::TearDownTestCase, \
    test_case_name##_##test_name##_Test::NewTest);\
void test_case_name##_##test_name##_Test::TestBody()


#endif  // GTEST_INCLUDE_GTEST_INTERNAL_GTEST_INTERNAL_H_
