// Protocol Buffers - Google's data interchange format
// Copyright 2026 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// The gtest fixture for conformance test suites.
//
// ConformanceTest is the fixture every conformance test should use.  It
// handles edition gating and, for now, performance-test filtering.  A suite
// overrides DefaultPriority() (see TestPriority in testee.h) to say how
// important its tests are.
//
// Its Testee() builds a test against the global testee, at the suite's
// priority or at the one it is given.
//
// Example:
//
//   class DelimitedFieldTest : public ConformanceTest {
//    protected:
//     // SetUp() skips the test when --maximum_edition doesn't cover it.
//     const Descriptor* MessageUnderTest() const override {
//       return TestAllTypesEdition2023::descriptor();
//     }
//   };
//
//   TEST_F(DelimitedFieldTest, ValidNonMessage) {
//     EXPECT_THAT(
//         Testee().ParseBinary(MessageUnderTest(), VarintField(1, 1))
//             .SerializeBinary(),
//         Yields(WhenParsed(EqualsTextProto(R"pb(optional_int32: 1)pb"))));
//   }
//
//   // A kP1 suite, with one test that isn't.
//   class OneofZeroTest : public ConformanceTest {
//    private:
//     TestPriority DefaultPriority() const override { return kP1; }
//   };
//
//   TEST_F(OneofZeroTest, Baseline) {
//     EXPECT_THAT(Testee(kP0).ParseBinary(...).SerializeBinary(), Yields(...));
//   }
//
// The fixture uses the process-global ConformanceEnvironment (see
// test_environment.h), which the test binary installs before RUN_ALL_TESTS().
// Like it, everything here is single-threaded.

#ifndef GOOGLE_PROTOBUF_CONFORMANCE_TEST_FIXTURE_H__
#define GOOGLE_PROTOBUF_CONFORMANCE_TEST_FIXTURE_H__

#include <gtest/gtest.h>
#include "absl/base/nullability.h"
#include "absl/strings/string_view.h"
#include "conformance/testee.h"
#include "google/protobuf/descriptor.h"

namespace google {
namespace protobuf {
namespace conformance {

// The fixture for all conformance tests.  Tests that aren't performance tests
// derive from this directly.  Performance tests derive from the transitional
// PerformanceConformanceTest.  A suite overrides the defaults below as needed:
//
//   class FooTest : public ConformanceTest {
//    private:
//     TestPriority DefaultPriority() const override { return kP1; }
//   };
//
//   TEST_F(FooTest, RoundTrip) {
//     EXPECT_THAT(Testee().ParseBinary(type, input).SerializeBinary(),
//                 Yields(WhenParsed(EqualsBinaryProto(input))));
//   }
//
// Subclasses that override SetUp() must call the base implementation first
// thing.  It may skip the test.  gtest only skips the test body on
// GTEST_SKIP(), so an overriding SetUp() must bail out itself afterwards:
//
//   void SetUp() override {
//     ConformanceTest::SetUp();
//     if (IsSkipped()) return;
//     ...
//   }
class ConformanceTest : public testing::Test {
 protected:
  // The priority of this suite's tests (see TestPriority).  kP0 unless a
  // subclass overrides it:
  //
  //   TestPriority DefaultPriority() const override { return kP1; }
  //
  // A single test can say otherwise with Testee(priority).
  virtual TestPriority DefaultPriority() const { return kP0; }

  // Skips the test if it isn't the kind (performance or regular) selected by
  // the environment or if MessageUnderTest() isn't supported under
  // --maximum_edition.
  void SetUp() override;

  // Whether this is a performance test.  Performance tests only run when the
  // environment was configured with `performance = true`, and regular tests
  // only run when it wasn't.
  // TODO: b/410126673 - Transitional.  Goes away with the in-binary
  // performance mode, see ConformanceEnvironmentOptions::performance.
  virtual bool IsPerformanceTest() const { return false; }

  // The message type this test exercises, if the fixture knows it.  When
  // non-null, SetUp() skips the test unless the message is supported under
  // the current --maximum_edition, so that the test can use it
  // unconditionally.  The file's edition must not be newer than the maximum;
  // EDITION_UNSTABLE is supported whenever the maximum is EDITION_2023 or
  // newer.  The default, null, disables the check.
  //
  //   const Descriptor* MessageUnderTest() const override {
  //     return TestAllTypesEdition2023::descriptor();
  //   }
  //
  // Fixtures parameterized over the message type override it as well (see
  // message_type_fixtures.h).
  virtual const Descriptor* absl_nullable MessageUnderTest() const {
    return nullptr;
  }

  // Creates a test against the global testee.  The name defaults to the
  // current gtest test's name, which must be unique across the binary once
  // combined with the input/output formats and message edition.
  //
  // The test's priority (see TestPriority), which <Level> is derived from, is
  // DefaultPriority().  The overloads taking a priority use it instead, for
  // the tests of a suite that are more or less important than the rest of it:
  // Testee(kP1).
  internal::Test Testee();
  internal::Test Testee(absl::string_view name);
  internal::Test Testee(TestPriority priority);
  internal::Test Testee(TestPriority priority, absl::string_view name);
};

// The fixture for performance conformance tests.
// TODO: b/410126673 - Transitional.  Goes away with the in-binary
// performance mode, see ConformanceEnvironmentOptions::performance.
class PerformanceConformanceTest : public ConformanceTest {
 protected:
  bool IsPerformanceTest() const override { return true; }
};

}  // namespace conformance
}  // namespace protobuf
}  // namespace google

#endif  // GOOGLE_PROTOBUF_CONFORMANCE_TEST_FIXTURE_H__
