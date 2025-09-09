// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "google/protobuf/util/time_util.h"

#include <cstdint>
#include <ctime>

#include "google/protobuf/duration.pb.h"
#include "google/protobuf/timestamp.pb.h"
#include "google/protobuf/testing/googletest.h"
#include <gtest/gtest.h>

namespace google {
namespace protobuf {
namespace util {

using google::protobuf::Duration;
using google::protobuf::Timestamp;

namespace {

TEST(TimeUtilTest, TimestampStringFormat) {
  // These these are out of bounds for 32-bit architectures.
  if(sizeof(time_t) >= sizeof(uint64_t)) {
    Timestamp begin, end;
    EXPECT_TRUE(TimeUtil::FromString("0001-01-01T00:00:00Z", &begin));
    EXPECT_EQ(TimeUtil::kTimestampMinSeconds, begin.seconds());
    EXPECT_EQ(0, begin.nanos());
    EXPECT_TRUE(TimeUtil::FromString("9999-12-31T23:59:59.999999999Z", &end));
    EXPECT_EQ(TimeUtil::kTimestampMaxSeconds, end.seconds());
    EXPECT_EQ(999999999, end.nanos());
    EXPECT_EQ("0001-01-01T00:00:00Z", TimeUtil::ToString(begin));
    EXPECT_EQ("9999-12-31T23:59:59.999999999Z", TimeUtil::ToString(end));
  }

  // Test negative timestamps.
  Timestamp time = TimeUtil::NanosecondsToTimestamp(-1);
  EXPECT_EQ(-1, time.seconds());
  // Timestamp's nano part is always non-negative.
  EXPECT_EQ(999999999, time.nanos());
  EXPECT_EQ("1969-12-31T23:59:59.999999999Z", TimeUtil::ToString(time));

  // Generated output should contain 3, 6, or 9 fractional digits.
  EXPECT_EQ("1970-01-01T00:00:00Z",
            TimeUtil::ToString(TimeUtil::NanosecondsToTimestamp(0)));
  EXPECT_EQ("1970-01-01T00:00:00.010Z",
            TimeUtil::ToString(TimeUtil::NanosecondsToTimestamp(10000000)));
  EXPECT_EQ("1970-01-01T00:00:00.000010Z",
            TimeUtil::ToString(TimeUtil::NanosecondsToTimestamp(10000)));
  EXPECT_EQ("1970-01-01T00:00:00.000000010Z",
            TimeUtil::ToString(TimeUtil::NanosecondsToTimestamp(10)));

  // Parsing accepts an fractional digits as long as they fit into nano
  // precision.
  EXPECT_TRUE(TimeUtil::FromString("1970-01-01T00:00:00.1Z", &time));
  EXPECT_EQ(100000000, TimeUtil::TimestampToNanoseconds(time));
  EXPECT_TRUE(TimeUtil::FromString("1970-01-01T00:00:00.0001Z", &time));
  EXPECT_EQ(100000, TimeUtil::TimestampToNanoseconds(time));
  EXPECT_TRUE(TimeUtil::FromString("1970-01-01T00:00:00.0000001Z", &time));
  EXPECT_EQ(100, TimeUtil::TimestampToNanoseconds(time));

  // Also accepts offsets.
  EXPECT_TRUE(TimeUtil::FromString("1970-01-01T00:00:00-08:00", &time));
  EXPECT_EQ(8 * 3600, TimeUtil::TimestampToSeconds(time));
}

TEST(TimeUtilTest, DurationStringFormat) {
  Timestamp begin, end;
  EXPECT_TRUE(TimeUtil::FromString("0001-01-01T00:00:00Z", &begin));
  EXPECT_TRUE(TimeUtil::FromString("9999-12-31T23:59:59.999999999Z", &end));

  // These these are out of bounds for 32-bit architectures.
  if(sizeof(time_t) >= sizeof(uint64_t)) {
    EXPECT_EQ("315537897599.999999999s", TimeUtil::ToString(end - begin));
    EXPECT_EQ("-315537897599.999999999s", TimeUtil::ToString(begin - end));
  }
  EXPECT_EQ(999999999, (end - begin).nanos());
  EXPECT_EQ(-999999999, (begin - end).nanos());

  // Generated output should contain 3, 6, or 9 fractional digits.
  EXPECT_EQ("1s", TimeUtil::ToString(TimeUtil::SecondsToDuration(1)));
  EXPECT_EQ("0.010s", TimeUtil::ToString(TimeUtil::MillisecondsToDuration(10)));
  EXPECT_EQ("0.000010s",
            TimeUtil::ToString(TimeUtil::MicrosecondsToDuration(10)));
  EXPECT_EQ("0.000000010s",
            TimeUtil::ToString(TimeUtil::NanosecondsToDuration(10)));

  // Parsing accepts an fractional digits as long as they fit into nano
  // precision.
  Duration d;
  EXPECT_TRUE(TimeUtil::FromString("0.1s", &d));
  EXPECT_EQ(100, TimeUtil::DurationToMilliseconds(d));
  EXPECT_TRUE(TimeUtil::FromString("0.0001s", &d));
  EXPECT_EQ(100, TimeUtil::DurationToMicroseconds(d));
  EXPECT_TRUE(TimeUtil::FromString("0.0000001s", &d));
  EXPECT_EQ(100, TimeUtil::DurationToNanoseconds(d));

  // Duration must support range from -315,576,000,000s to +315576000000s
  // which includes negative values.
  EXPECT_TRUE(TimeUtil::FromString("315576000000.999999999s", &d));
  EXPECT_EQ(int64_t{315576000000}, d.seconds());
  EXPECT_EQ(999999999, d.nanos());
  EXPECT_TRUE(TimeUtil::FromString("-315576000000.999999999s", &d));
  EXPECT_EQ(int64_t{-315576000000}, d.seconds());
  EXPECT_EQ(-999999999, d.nanos());
}

TEST(TimeUtilTest, GetEpoch) {
  EXPECT_EQ(0, TimeUtil::TimestampToNanoseconds(TimeUtil::GetEpoch()));
}

TEST(TimeUtilTest, DurationIntegerConversion) {
  EXPECT_EQ("0.000000001s",
            TimeUtil::ToString(TimeUtil::NanosecondsToDuration(1)));
  EXPECT_EQ("-0.000000001s",
            TimeUtil::ToString(TimeUtil::NanosecondsToDuration(-1)));
  EXPECT_EQ("0.000001s",
            TimeUtil::ToString(TimeUtil::MicrosecondsToDuration(1)));
  EXPECT_EQ("-0.000001s",
            TimeUtil::ToString(TimeUtil::MicrosecondsToDuration(-1)));
  EXPECT_EQ("0.001s", TimeUtil::ToString(TimeUtil::MillisecondsToDuration(1)));
  EXPECT_EQ("-0.001s",
            TimeUtil::ToString(TimeUtil::MillisecondsToDuration(-1)));
  EXPECT_EQ("1s", TimeUtil::ToString(TimeUtil::SecondsToDuration(1)));
  EXPECT_EQ("-1s", TimeUtil::ToString(TimeUtil::SecondsToDuration(-1)));
  EXPECT_EQ("60s", TimeUtil::ToString(TimeUtil::MinutesToDuration(1)));
  EXPECT_EQ("-60s", TimeUtil::ToString(TimeUtil::MinutesToDuration(-1)));
  EXPECT_EQ("3600s", TimeUtil::ToString(TimeUtil::HoursToDuration(1)));
  EXPECT_EQ("-3600s", TimeUtil::ToString(TimeUtil::HoursToDuration(-1)));

  EXPECT_EQ(
      1, TimeUtil::DurationToNanoseconds(TimeUtil::NanosecondsToDuration(1)));
  EXPECT_EQ(
      -1, TimeUtil::DurationToNanoseconds(TimeUtil::NanosecondsToDuration(-1)));
  EXPECT_EQ(
      1, TimeUtil::DurationToMicroseconds(TimeUtil::MicrosecondsToDuration(1)));
  EXPECT_EQ(-1, TimeUtil::DurationToMicroseconds(
                    TimeUtil::MicrosecondsToDuration(-1)));
  EXPECT_EQ(
      1, TimeUtil::DurationToMilliseconds(TimeUtil::MillisecondsToDuration(1)));
  EXPECT_EQ(-1, TimeUtil::DurationToMilliseconds(
                    TimeUtil::MillisecondsToDuration(-1)));
  // Test overflow issue
  EXPECT_EQ(315576000000000, TimeUtil::DurationToMilliseconds(
                                 TimeUtil::SecondsToDuration(315576000000)));
  // Test overflow issue
  EXPECT_EQ(315576000000000000, TimeUtil::DurationToMicroseconds(
                                    TimeUtil::SecondsToDuration(315576000000)));
  EXPECT_EQ(1, TimeUtil::DurationToSeconds(TimeUtil::SecondsToDuration(1)));
  EXPECT_EQ(-1, TimeUtil::DurationToSeconds(TimeUtil::SecondsToDuration(-1)));
  EXPECT_EQ(1, TimeUtil::DurationToMinutes(TimeUtil::MinutesToDuration(1)));
  EXPECT_EQ(-1, TimeUtil::DurationToMinutes(TimeUtil::MinutesToDuration(-1)));
  EXPECT_EQ(1, TimeUtil::DurationToHours(TimeUtil::HoursToDuration(1)));
  EXPECT_EQ(-1, TimeUtil::DurationToHours(TimeUtil::HoursToDuration(-1)));

  // Test truncation behavior.
  EXPECT_EQ(1, TimeUtil::DurationToMicroseconds(
                   TimeUtil::NanosecondsToDuration(1999)));
  // For negative values, Duration will be rounded towards 0.
  EXPECT_EQ(-1, TimeUtil::DurationToMicroseconds(
                    TimeUtil::NanosecondsToDuration(-1999)));
}

TEST(TestUtilTest, TimestampIntegerConversion) {
  EXPECT_EQ("1970-01-01T00:00:00.000000001Z",
            TimeUtil::ToString(TimeUtil::NanosecondsToTimestamp(1)));
  EXPECT_EQ("1969-12-31T23:59:59.999999999Z",
            TimeUtil::ToString(TimeUtil::NanosecondsToTimestamp(-1)));
  EXPECT_EQ("1970-01-01T00:00:00.000001Z",
            TimeUtil::ToString(TimeUtil::MicrosecondsToTimestamp(1)));
  EXPECT_EQ("1969-12-31T23:59:59.999999Z",
            TimeUtil::ToString(TimeUtil::MicrosecondsToTimestamp(-1)));
  EXPECT_EQ("1970-01-01T00:00:00.001Z",
            TimeUtil::ToString(TimeUtil::MillisecondsToTimestamp(1)));
  EXPECT_EQ("1969-12-31T23:59:59.999Z",
            TimeUtil::ToString(TimeUtil::MillisecondsToTimestamp(-1)));
  EXPECT_EQ("1970-01-01T00:00:01Z",
            TimeUtil::ToString(TimeUtil::SecondsToTimestamp(1)));
  EXPECT_EQ("1969-12-31T23:59:59Z",
            TimeUtil::ToString(TimeUtil::SecondsToTimestamp(-1)));

  EXPECT_EQ(
      1, TimeUtil::TimestampToNanoseconds(TimeUtil::NanosecondsToTimestamp(1)));
  EXPECT_EQ(-1, TimeUtil::TimestampToNanoseconds(
                    TimeUtil::NanosecondsToTimestamp(-1)));
  EXPECT_EQ(1, TimeUtil::TimestampToMicroseconds(
                   TimeUtil::MicrosecondsToTimestamp(1)));
  EXPECT_EQ(-1, TimeUtil::TimestampToMicroseconds(
                    TimeUtil::MicrosecondsToTimestamp(-1)));
  EXPECT_EQ(1, TimeUtil::TimestampToMilliseconds(
                   TimeUtil::MillisecondsToTimestamp(1)));
  EXPECT_EQ(-1, TimeUtil::TimestampToMilliseconds(
                    TimeUtil::MillisecondsToTimestamp(-1)));
  EXPECT_EQ(1, TimeUtil::TimestampToSeconds(TimeUtil::SecondsToTimestamp(1)));
  EXPECT_EQ(-1, TimeUtil::TimestampToSeconds(TimeUtil::SecondsToTimestamp(-1)));

  // Test truncation behavior.
  EXPECT_EQ(1, TimeUtil::TimestampToMicroseconds(
                   TimeUtil::NanosecondsToTimestamp(1999)));
  // For negative values, Timestamp will be rounded down.
  // For example, "1969-12-31T23:59:59.5Z" (i.e., -0.5s) rounded to seconds
  // will be "1969-12-31T23:59:59Z" (i.e., -1s) rather than
  // "1970-01-01T00:00:00Z" (i.e., 0s).
  EXPECT_EQ(-2, TimeUtil::TimestampToMicroseconds(
                    TimeUtil::NanosecondsToTimestamp(-1999)));
}

TEST(TimeUtilTest, TimeTConversion) {
  time_t value = time(nullptr);
  EXPECT_EQ(value,
            TimeUtil::TimestampToTimeT(TimeUtil::TimeTToTimestamp(value)));
  EXPECT_EQ(
      1, TimeUtil::TimestampToTimeT(TimeUtil::MillisecondsToTimestamp(1999)));
}

TEST(TimeUtilTest, TimevalConversion) {
  timeval value = TimeUtil::TimestampToTimeval(
      TimeUtil::NanosecondsToTimestamp(1999999999));
  EXPECT_EQ(1, value.tv_sec);
  EXPECT_EQ(999999, value.tv_usec);
  value = TimeUtil::TimestampToTimeval(
      TimeUtil::NanosecondsToTimestamp(-1999999999));
  EXPECT_EQ(-2, value.tv_sec);
  EXPECT_EQ(0, value.tv_usec);

  value =
      TimeUtil::DurationToTimeval(TimeUtil::NanosecondsToDuration(1999999999));
  EXPECT_EQ(1, value.tv_sec);
  EXPECT_EQ(999999, value.tv_usec);
  value =
      TimeUtil::DurationToTimeval(TimeUtil::NanosecondsToDuration(-1999999999));
  EXPECT_EQ(-2, value.tv_sec);
  EXPECT_EQ(1, value.tv_usec);
}

TEST(TimeUtilTest, DurationOperators) {
  Duration one_second = TimeUtil::SecondsToDuration(1);
  Duration one_nano = TimeUtil::NanosecondsToDuration(1);

  // Test +/-
  Duration a = one_second;
  a += one_second;
  a -= one_nano;
  EXPECT_EQ("1.999999999s", TimeUtil::ToString(a));
  Duration b = -a;
  EXPECT_EQ("-1.999999999s", TimeUtil::ToString(b));
  EXPECT_EQ("3.999999998s", TimeUtil::ToString(a + a));
  EXPECT_EQ("0s", TimeUtil::ToString(a + b));
  EXPECT_EQ("0s", TimeUtil::ToString(b + a));
  EXPECT_EQ("-3.999999998s", TimeUtil::ToString(b + b));
  EXPECT_EQ("3.999999998s", TimeUtil::ToString(a - b));
  EXPECT_EQ("0s", TimeUtil::ToString(a - a));
  EXPECT_EQ("0s", TimeUtil::ToString(b - b));
  EXPECT_EQ("-3.999999998s", TimeUtil::ToString(b - a));

  // Test *
  EXPECT_EQ(a + a, a * 2);
  EXPECT_EQ(b + b, a * (-2));
  EXPECT_EQ(b + b, b * 2);
  EXPECT_EQ(a + a, b * (-2));
  EXPECT_EQ("0.999999999s", TimeUtil::ToString(a * 0.5));
  EXPECT_EQ("-0.999999999s", TimeUtil::ToString(b * 0.5));
  // Multiplication should not overflow if the result fits into the supported
  // range of Duration (intermediate result may be larger than int64).
  EXPECT_EQ("315575999684.424s", TimeUtil::ToString((one_second - one_nano) *
                                                    int64_t{315576000000}));
  EXPECT_EQ("-315575999684.424s", TimeUtil::ToString((one_nano - one_second) *
                                                     int64_t{315576000000}));
  EXPECT_EQ("-315575999684.424s", TimeUtil::ToString((one_second - one_nano) *
                                                     (int64_t{-315576000000})));

  // Test / and %
  EXPECT_EQ("0.999999999s", TimeUtil::ToString(a / 2));
  EXPECT_EQ("-0.999999999s", TimeUtil::ToString(b / 2));
  Duration large =
      TimeUtil::SecondsToDuration(int64_t{315576000000}) - one_nano;
  // We have to handle division with values beyond 64 bits.
  EXPECT_EQ("0.999999999s", TimeUtil::ToString(large / int64_t{315576000000}));
  EXPECT_EQ("-0.999999999s",
            TimeUtil::ToString((-large) / int64_t{315576000000}));
  EXPECT_EQ("-0.999999999s",
            TimeUtil::ToString(large / (int64_t{-315576000000})));
  Duration large2 = large + one_nano;
  EXPECT_EQ(large, large % large2);
  EXPECT_EQ(-large, (-large) % large2);
  EXPECT_EQ(large, large % (-large2));
  EXPECT_EQ(one_nano, large2 % large);
  EXPECT_EQ(-one_nano, (-large2) % large);
  EXPECT_EQ(one_nano, large2 % (-large));
  // Some corner cases about negative values.
  //
  // (-5) / 2 = -2, remainder = -1
  // (-5) / (-2) = 2, remainder = -1
  a = TimeUtil::NanosecondsToDuration(-5);
  EXPECT_EQ(TimeUtil::NanosecondsToDuration(-2), a / 2);
  EXPECT_EQ(TimeUtil::NanosecondsToDuration(2), a / (-2));
  b = TimeUtil::NanosecondsToDuration(2);
  EXPECT_EQ(-2, a / b);
  EXPECT_EQ(TimeUtil::NanosecondsToDuration(-1), a % b);
  EXPECT_EQ(2, a / (-b));
  EXPECT_EQ(TimeUtil::NanosecondsToDuration(-1), a % (-b));

  // Test relational operators.
  EXPECT_TRUE(one_nano < one_second);
  EXPECT_FALSE(one_second < one_second);
  EXPECT_FALSE(one_second < one_nano);
  EXPECT_FALSE(-one_nano < -one_second);
  EXPECT_FALSE(-one_second < -one_second);
  EXPECT_TRUE(-one_second < -one_nano);
  EXPECT_TRUE(-one_nano < one_nano);
  EXPECT_FALSE(one_nano < -one_nano);

  EXPECT_FALSE(one_nano > one_second);
  EXPECT_FALSE(one_nano > one_nano);
  EXPECT_TRUE(one_second > one_nano);

  EXPECT_FALSE(one_nano >= one_second);
  EXPECT_TRUE(one_nano >= one_nano);
  EXPECT_TRUE(one_second >= one_nano);

  EXPECT_TRUE(one_nano <= one_second);
  EXPECT_TRUE(one_nano <= one_nano);
  EXPECT_FALSE(one_second <= one_nano);

  EXPECT_TRUE(one_nano == one_nano);
  EXPECT_FALSE(one_nano == one_second);

  EXPECT_FALSE(one_nano != one_nano);
  EXPECT_TRUE(one_nano != one_second);
}

TEST(TimeUtilTest, TimestampOperators) {
  Timestamp begin, end;
  EXPECT_TRUE(TimeUtil::FromString("0001-01-01T00:00:00Z", &begin));
  EXPECT_TRUE(TimeUtil::FromString("9999-12-31T23:59:59.999999999Z", &end));
  Duration d = end - begin;
  EXPECT_TRUE(end == begin + d);
  EXPECT_TRUE(end == d + begin);
  EXPECT_TRUE(begin == end - d);

  // Test relational operators
  Timestamp t1 = begin + d / 4;
  Timestamp t2 = end - d / 4;
  EXPECT_TRUE(t1 < t2);
  EXPECT_FALSE(t1 < t1);
  EXPECT_FALSE(t2 < t1);
  EXPECT_FALSE(t1 > t2);
  EXPECT_FALSE(t1 > t1);
  EXPECT_TRUE(t2 > t1);
  EXPECT_FALSE(t1 >= t2);
  EXPECT_TRUE(t1 >= t1);
  EXPECT_TRUE(t2 >= t1);
  EXPECT_TRUE(t1 <= t2);
  EXPECT_TRUE(t1 <= t1);
  EXPECT_FALSE(t2 <= t1);

  EXPECT_FALSE(t1 == t2);
  EXPECT_TRUE(t1 == t1);
  EXPECT_FALSE(t2 == t1);
  EXPECT_TRUE(t1 != t2);
  EXPECT_FALSE(t1 != t1);
  EXPECT_TRUE(t2 != t1);
}

TEST(TimeUtilTest, IsDurationValid) {
  Duration valid;
  Duration overflow;
  overflow.set_seconds(TimeUtil::kDurationMaxSeconds + 1);
  Duration underflow;
  underflow.set_seconds(TimeUtil::kDurationMinSeconds - 1);
  Duration overflow_nanos;
  overflow_nanos.set_nanos(TimeUtil::kDurationMaxNanoseconds + 1);
  Duration underflow_nanos;
  underflow_nanos.set_nanos(TimeUtil::kDurationMinNanoseconds - 1);
  Duration positive_seconds_negative_nanos;
  positive_seconds_negative_nanos.set_seconds(1);
  positive_seconds_negative_nanos.set_nanos(-1);
  Duration negative_seconds_positive_nanos;
  negative_seconds_positive_nanos.set_seconds(-1);
  negative_seconds_positive_nanos.set_nanos(1);

  EXPECT_TRUE(TimeUtil::IsDurationValid(valid));
  EXPECT_FALSE(TimeUtil::IsDurationValid(overflow));
  EXPECT_FALSE(TimeUtil::IsDurationValid(underflow));
  EXPECT_FALSE(TimeUtil::IsDurationValid(overflow_nanos));
  EXPECT_FALSE(TimeUtil::IsDurationValid(underflow_nanos));
  EXPECT_FALSE(TimeUtil::IsDurationValid(positive_seconds_negative_nanos));
  EXPECT_FALSE(TimeUtil::IsDurationValid(negative_seconds_positive_nanos));
}

TEST(TimeUtilTest, IsTimestampValid) {
  Timestamp valid;
  Timestamp overflow;
  overflow.set_seconds(TimeUtil::kTimestampMaxSeconds + 1);
  Timestamp underflow;
  underflow.set_seconds(TimeUtil::kTimestampMinSeconds - 1);
  Timestamp overflow_nanos;
  overflow_nanos.set_nanos(TimeUtil::kTimestampMaxNanoseconds + 1);
  Timestamp underflow_nanos;
  underflow_nanos.set_nanos(TimeUtil::kTimestampMinNanoseconds - 1);

  EXPECT_TRUE(TimeUtil::IsTimestampValid(valid));
  EXPECT_FALSE(TimeUtil::IsTimestampValid(overflow));
  EXPECT_FALSE(TimeUtil::IsTimestampValid(underflow));
  EXPECT_FALSE(TimeUtil::IsTimestampValid(overflow_nanos));
  EXPECT_FALSE(TimeUtil::IsTimestampValid(underflow_nanos));
}

#if GTEST_HAS_DEATH_TEST  // death tests do not work on Windows yet.
#ifndef NDEBUG

TEST(TimeUtilTest, DurationBounds) {
  Duration overflow;
  overflow.set_seconds(TimeUtil::kDurationMaxSeconds + 1);
  Duration underflow;
  underflow.set_seconds(TimeUtil::kDurationMinSeconds - 1);
  Duration overflow_nanos;
  overflow_nanos.set_nanos(TimeUtil::kDurationMaxNanoseconds + 1);
  Duration underflow_nanos;
  underflow_nanos.set_nanos(TimeUtil::kDurationMinNanoseconds - 1);

  EXPECT_DEATH({ TimeUtil::SecondsToDuration(overflow.seconds()); },
                     "Duration seconds");
  EXPECT_DEATH({ TimeUtil::SecondsToDuration(underflow.seconds()); },
                     "Duration seconds");
  EXPECT_DEATH(
      { TimeUtil::MinutesToDuration(overflow.seconds() / 60 + 1); },
      "Duration minutes");
  EXPECT_DEATH(
      { TimeUtil::MinutesToDuration(underflow.seconds() / 60 - 1); },
      "Duration minutes");
  EXPECT_DEATH(
      { TimeUtil::HoursToDuration(overflow.seconds() / 60 + 1); },
      "Duration hours");
  EXPECT_DEATH(
      { TimeUtil::HoursToDuration(underflow.seconds() / 60 - 1); },
      "Duration hours");

  EXPECT_DEATH({ TimeUtil::DurationToNanoseconds(overflow); },
                     "outside of the valid range");
  EXPECT_DEATH({ TimeUtil::DurationToNanoseconds(underflow); },
                     "outside of the valid range");
  EXPECT_DEATH({ TimeUtil::DurationToNanoseconds(overflow_nanos); },
                     "outside of the valid range");
  EXPECT_DEATH({ TimeUtil::DurationToNanoseconds(underflow_nanos); },
                     "outside of the valid range");
  EXPECT_DEATH({ TimeUtil::DurationToSeconds(overflow); },
                     "outside of the valid range");
  EXPECT_DEATH({ TimeUtil::DurationToSeconds(underflow); },
                     "outside of the valid range");
  EXPECT_DEATH({ TimeUtil::DurationToSeconds(overflow_nanos); },
                     "outside of the valid range");
  EXPECT_DEATH({ TimeUtil::DurationToSeconds(underflow_nanos); },
                     "outside of the valid range");
}

TEST(TimeUtilTest, TimestampBounds) {
  Timestamp overflow;
  overflow.set_seconds(TimeUtil::kDurationMaxSeconds + 1);
  Timestamp underflow;
  underflow.set_seconds(TimeUtil::kDurationMinSeconds - 1);
  Timestamp overflow_nanos;
  overflow_nanos.set_nanos(TimeUtil::kDurationMaxNanoseconds + 1);
  Timestamp underflow_nanos;
  underflow_nanos.set_nanos(TimeUtil::kDurationMinNanoseconds - 1);

  EXPECT_DEATH({ TimeUtil::TimestampToNanoseconds(overflow); },
                     "outside of the valid range");
  EXPECT_DEATH({ TimeUtil::TimestampToNanoseconds(underflow); },
                     "outside of the valid range");
  EXPECT_DEATH({ TimeUtil::TimestampToNanoseconds(overflow_nanos); },
                     "outside of the valid range");
  EXPECT_DEATH({ TimeUtil::TimestampToNanoseconds(underflow_nanos); },
                     "outside of the valid range");

  EXPECT_DEATH({ TimeUtil::TimestampToMicroseconds(overflow); },
                     "outside of the valid range");
  EXPECT_DEATH({ TimeUtil::TimestampToMicroseconds(underflow); },
                     "outside of the valid range");
  EXPECT_DEATH({ TimeUtil::TimestampToMicroseconds(overflow_nanos); },
                     "outside of the valid range");
  EXPECT_DEATH({ TimeUtil::TimestampToMicroseconds(underflow_nanos); },
                     "outside of the valid range");

  EXPECT_DEATH({ TimeUtil::TimestampToMilliseconds(overflow); },
                     "outside of the valid range");
  EXPECT_DEATH({ TimeUtil::TimestampToMilliseconds(underflow); },
                     "outside of the valid range");
  EXPECT_DEATH({ TimeUtil::TimestampToMilliseconds(overflow_nanos); },
                     "outside of the valid range");
  EXPECT_DEATH({ TimeUtil::TimestampToMilliseconds(underflow_nanos); },
                     "outside of the valid range");

  EXPECT_DEATH({ TimeUtil::TimestampToSeconds(overflow); },
                     "outside of the valid range");
  EXPECT_DEATH({ TimeUtil::TimestampToSeconds(underflow); },
                     "outside of the valid range");
  EXPECT_DEATH({ TimeUtil::TimestampToSeconds(overflow_nanos); },
                     "outside of the valid range");
  EXPECT_DEATH({ TimeUtil::TimestampToSeconds(underflow_nanos); },
                     "outside of the valid range");
}
#endif  // !NDEBUG
#endif  // GTEST_HAS_DEATH_TEST

}  // namespace
}  // namespace util
}  // namespace protobuf
}  // namespace google
