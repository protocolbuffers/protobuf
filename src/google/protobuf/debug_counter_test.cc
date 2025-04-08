// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd
//
#include <cstdlib>
#include <string>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "google/protobuf/port.h"

// Must be included last.
#include "google/protobuf/port_def.inc"

namespace {

using testing::AllOf;
using testing::ExitedWithCode;
using testing::HasSubstr;
using testing::Not;

auto MatchOutput(bool expect_output) {
  const auto header = HasSubstr("Protobuf debug counters:");
  const auto all = AllOf(header,  //
                         HasSubstr("Foo         :"),
                         HasSubstr("  Bar       :          1 (33.33%)"),
                         HasSubstr("  Baz       :          2 (66.67%)"),
                         HasSubstr("  Total     :          3"),  //
                         HasSubstr("Num         :"),
                         HasSubstr("         32 :          3 (75.00%)"),
                         HasSubstr("        128 :          1 (25.00%)"),
                         HasSubstr("  Total     :          4"));
  return expect_output ? testing::Matcher<const std::string&>(all)
                       : testing::Matcher<const std::string&>(Not(header));
}

#ifdef GTEST_HAS_DEATH_TEST
TEST(DebugCounterTest, RealProvidesReportAtExit) {
  EXPECT_EXIT(
      {
        static google::protobuf::internal::RealDebugCounter counter1("Foo.Bar");
        static google::protobuf::internal::RealDebugCounter counter2("Foo.Baz");
        static google::protobuf::internal::RealDebugCounter counter3("Num.32");
        static google::protobuf::internal::RealDebugCounter counter4("Num.128");
        counter1.Inc();
        counter2.Inc();
        counter2.Inc();
        counter3.Inc();
        counter3.Inc();
        counter3.Inc();
        counter4.Inc();
        exit(0);
      },
      ExitedWithCode(0), MatchOutput(true));
}

TEST(DebugCounterTest, NoopDoesNotProvidesReportAtExit) {
  EXPECT_EXIT(
      {
        static google::protobuf::internal::NoopDebugCounter counter1;
        static google::protobuf::internal::NoopDebugCounter counter2;
        counter1.Inc();
        counter2.Inc();
        counter2.Inc();
        exit(0);
      },
      ExitedWithCode(0), MatchOutput(false));

  // and test that the operations have no side effects.
  static_assert((google::protobuf::internal::NoopDebugCounter().Inc(), true), "");
}

TEST(DebugCounterTest, MacroProvidesReportAtExitDependingOnBuild) {
#if defined(PROTOBUF_INTERNAL_ENABLE_DEBUG_COUNTERS)
  constexpr bool match_output = true;
#else
  constexpr bool match_output = false;

  // and test that the operations have no side effects.
  static_assert((PROTOBUF_DEBUG_COUNTER("Foo.Bar").Inc(), true), "");
#endif

  EXPECT_EXIT(
      {
        PROTOBUF_DEBUG_COUNTER("Foo.Bar").Inc();
        for (int i = 0; i < 2; ++i) {
          PROTOBUF_DEBUG_COUNTER("Foo.Baz").Inc();
        }
        for (int i = 0; i < 3; ++i) {
          PROTOBUF_DEBUG_COUNTER("Num.32").Inc();
        }
        PROTOBUF_DEBUG_COUNTER("Num.128").Inc();
        exit(0);
      },
      ExitedWithCode(0), MatchOutput(match_output));
}
#endif  // GTEST_HAS_DEATH_TEST

template <typename T>
void CounterOnATemplate() {
  static google::protobuf::internal::RealDebugCounter counter("Foo.Bar");
  counter.Inc();
}

// Regression test for counters on templates.
// It used to be that duplicate names would clobber each other so the total for
// the counter would only take one instantiation into account and undercount.
TEST(DebugCounterTest, DuplicateNamesWorkTogether) {
  EXPECT_EXIT(
      {
        static google::protobuf::internal::RealDebugCounter counter("Foo.Baz");
        CounterOnATemplate<int>();
        CounterOnATemplate<int>();
        CounterOnATemplate<double>();
        counter.Inc();
        counter.Inc();
        exit(0);
      },
      ExitedWithCode(0),
      AllOf(HasSubstr("  Bar       :          3 (60.00%)"),
            HasSubstr("  Baz       :          2 (40.00%)"),
            HasSubstr("  Total     :          5")));
}

}  // namespace

#include "google/protobuf/port_undef.inc"
