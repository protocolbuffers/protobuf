// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

package com.google.protobuf.util;

import static com.google.common.truth.Truth.assertThat;
import static com.google.protobuf.util.DurationsTest.duration;
import static org.junit.Assert.fail;

import com.google.common.annotations.GwtIncompatible;
import com.google.common.collect.Lists;
import com.google.j2objc.annotations.J2ObjCIncompatible;
import com.google.protobuf.Duration;
import com.google.protobuf.Timestamp;
import java.text.ParseException;
import java.util.ArrayList;
import java.util.Calendar;
import java.util.Collections;
import java.util.Date;
import java.util.List;
import java.util.TimeZone;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

/** Unit tests for {@link Timestamps}. */
@RunWith(JUnit4.class)
@SuppressWarnings("JavaUtilDate")
public class TimestampsTest {
  private static final int MILLIS_PER_SECOND = 1000;
  private static final long MILLIS = 1409130915111L;
  private static final long SECONDS = MILLIS / MILLIS_PER_SECOND;
  private static final long MICROS = MILLIS * 1000;
  private static final long NANOS = MICROS * 1000;

  @SuppressWarnings("ConstantOverflow")
  private static final long MAX_VALUE = Long.MAX_VALUE * MILLIS_PER_SECOND + MILLIS_PER_SECOND;

  @SuppressWarnings("ConstantOverflow")
  private static final long MIN_VALUE = Long.MIN_VALUE * MILLIS_PER_SECOND;

  private static final Timestamp TIMESTAMP = timestamp(1409130915, 111000000);
  private static final Timestamp ZERO_TIMESTAMP = timestamp(0, 0);
  private static final Timestamp ONE_OF_TIMESTAMP = timestamp(-1, 999000000);

  private static final Timestamp INVALID_MAX =
      Timestamp.newBuilder().setSeconds(Long.MAX_VALUE).setNanos(Integer.MAX_VALUE).build();
  private static final Timestamp INVALID_MIN =
      Timestamp.newBuilder().setSeconds(Long.MIN_VALUE).setNanos(Integer.MIN_VALUE).build();

  @Test
  @GwtIncompatible("Uses reflection to access methods of java.time.Instant")
  @J2ObjCIncompatible
  public void testNow() {
    Timestamp now = Timestamps.now();
    long epochSeconds = System.currentTimeMillis() / 1000;
    assertThat(now.getSeconds()).isAtLeast(epochSeconds - 1);
    assertThat(now.getSeconds()).isAtMost(epochSeconds + 1);
  }

  @Test
  @GwtIncompatible("Uses reflection to access methods of java.time.Instant")
  @J2ObjCIncompatible
  public void testNowWithSubMillisecondPrecision() {
    try {
      // throws if we're not on Java9+
      Class.forName("java.lang.Runtime$Version");
    } catch (ClassNotFoundException e) {
      // ignored; we're not on Java 9+
      return;
    }

    // grab 100 timestamps, and ensure that at least 1 of them has sub-millisecond precision
    for (int i = 0; i < 100; i++) {
      Timestamp now = Timestamps.now();
      Timestamp nowWithMilliPrecision = Timestamps.fromMillis(Timestamps.toMillis(now));
      if (!now.equals(nowWithMilliPrecision)) {
        return;
      }
    }
    fail("no timestamp had sub-millisecond precision");
  }

  @Test
  public void testMinMaxAreValid() {
    assertThat(Timestamps.isValid(Timestamps.MAX_VALUE)).isTrue();
    assertThat(Timestamps.isValid(Timestamps.MIN_VALUE)).isTrue();
  }

  @Test
  public void testIsValid_false() {
    assertThat(Timestamps.isValid(0L, -1)).isFalse();
    assertThat(Timestamps.isValid(1L, -1)).isFalse();
    assertThat(Timestamps.isValid(1L, (int) Timestamps.NANOS_PER_SECOND)).isFalse();
    assertThat(Timestamps.isValid(-62135596801L, 0)).isFalse();
    assertThat(Timestamps.isValid(253402300800L, 0)).isFalse();
  }

  @Test
  public void testIsValid_true() {
    assertThat(Timestamps.isValid(0L, 0)).isTrue();
    assertThat(Timestamps.isValid(1L, 0)).isTrue();
    assertThat(Timestamps.isValid(1L, 1)).isTrue();
    assertThat(Timestamps.isValid(42L, 0)).isTrue();
    assertThat(Timestamps.isValid(42L, 42)).isTrue();
    assertThat(Timestamps.isValid(-62135596800L, 0)).isTrue();
    assertThat(Timestamps.isValid(-62135596800L, 1)).isTrue();
    assertThat(Timestamps.isValid(62135596799L, 1)).isTrue();
    assertThat(Timestamps.isValid(253402300799L, 0)).isTrue();
    assertThat(Timestamps.isValid(253402300798L, 1)).isTrue();
    assertThat(Timestamps.isValid(253402300798L, (int) (Timestamps.NANOS_PER_SECOND - 1))).isTrue();
  }

  @Test
  @GwtIncompatible("Depends on String.format which is not supported in Xplat.")
  @J2ObjCIncompatible
  public void testTimestampStringFormat() throws Exception {
    Timestamp start = Timestamps.parse("0001-01-01T00:00:00Z");
    Timestamp end = Timestamps.parse("9999-12-31T23:59:59.999999999Z");
    assertThat(start.getSeconds()).isEqualTo(Timestamps.TIMESTAMP_SECONDS_MIN);
    assertThat(start.getNanos()).isEqualTo(0);
    assertThat(end.getSeconds()).isEqualTo(Timestamps.TIMESTAMP_SECONDS_MAX);
    assertThat(end.getNanos()).isEqualTo(999999999);
    assertThat(Timestamps.toString(start)).isEqualTo("0001-01-01T00:00:00Z");
    assertThat(Timestamps.toString(end)).isEqualTo("9999-12-31T23:59:59.999999999Z");

    Timestamp value = Timestamps.parse("1970-01-01T00:00:00Z");
    assertThat(value.getSeconds()).isEqualTo(0);
    assertThat(value.getNanos()).isEqualTo(0);
    value = Timestamps.parseUnchecked("1970-01-01T00:00:00Z");
    assertThat(value.getSeconds()).isEqualTo(0);
    assertThat(value.getNanos()).isEqualTo(0);

    // Test negative timestamps.
    value = Timestamps.parse("1969-12-31T23:59:59.999Z");
    assertThat(value.getSeconds()).isEqualTo(-1);
    // Nano part is in the range of [0, 999999999] for Timestamp.
    assertThat(value.getNanos()).isEqualTo(999000000);
    value = Timestamps.parseUnchecked("1969-12-31T23:59:59.999Z");
    assertThat(value.getSeconds()).isEqualTo(-1);
    // Nano part is in the range of [0, 999999999] for Timestamp.
    assertThat(value.getNanos()).isEqualTo(999000000);

    // Test that 3, 6, or 9 digits are used for the fractional part.
    assertThat(Timestamps.toString(Timestamp.newBuilder().setNanos(10).build()))
        .isEqualTo("1970-01-01T00:00:00.000000010Z");
    assertThat(Timestamps.toString(Timestamp.newBuilder().setNanos(10000).build()))
        .isEqualTo("1970-01-01T00:00:00.000010Z");
    assertThat(Timestamps.toString(Timestamp.newBuilder().setNanos(10000000).build()))
        .isEqualTo("1970-01-01T00:00:00.010Z");

    // Test that parsing accepts timezone offsets.
    assertThat(Timestamps.toString(Timestamps.parse("1970-01-01T00:00:00.010+08:00")))
        .isEqualTo("1969-12-31T16:00:00.010Z");
    assertThat(Timestamps.toString(Timestamps.parse("1970-01-01T00:00:00.010-08:00")))
        .isEqualTo("1970-01-01T08:00:00.010Z");
    assertThat(Timestamps.toString(Timestamps.parseUnchecked("1970-01-01T00:00:00.010+08:00")))
        .isEqualTo("1969-12-31T16:00:00.010Z");
    assertThat(Timestamps.toString(Timestamps.parseUnchecked("1970-01-01T00:00:00.010-08:00")))
        .isEqualTo("1970-01-01T08:00:00.010Z");
  }

  private volatile boolean stopParsingThreads = false;
  private volatile String errorMessage = "";

  @GwtIncompatible("ParseException is not supported in Xplat")
  @J2ObjCIncompatible
  private class ParseTimestampThread extends Thread {
    private final String[] strings;
    private final Timestamp[] values;

    public ParseTimestampThread(String[] strings, Timestamp[] values) {
      this.strings = strings;
      this.values = values;
    }

    @Override
    @SuppressWarnings("ProtoTimestampGetSecondsGetNano")
    public void run() {
      int index = 0;
      while (!stopParsingThreads) {
        Timestamp result;
        try {
          result = Timestamps.parse(strings[index]);
        } catch (ParseException e) {
          errorMessage = "Failed to parse timestamp: " + strings[index];
          break;
        }
        if (result.getSeconds() != values[index].getSeconds()
            || result.getNanos() != values[index].getNanos()) {
          errorMessage =
              "Actual result: " + result.toString() + ", expected: " + values[index].toString();
          break;
        }
        index = (index + 1) % strings.length;
      }
    }
  }

  @Test
  @GwtIncompatible("ParseException is not supported in Xplat")
  @J2ObjCIncompatible
  public void testTimestampConcurrentParsing() throws Exception {
    String[] timestampStrings =
        new String[] {
          "0001-01-01T00:00:00Z",
          "9999-12-31T23:59:59.999999999Z",
          "1970-01-01T00:00:00Z",
          "1969-12-31T23:59:59.999Z",
        };
    Timestamp[] timestampValues = new Timestamp[timestampStrings.length];
    for (int i = 0; i < timestampStrings.length; i++) {
      timestampValues[i] = Timestamps.parse(timestampStrings[i]);
    }

    final int threadCount = 16;
    final int runningTime = 5000; // in milliseconds.
    final List<Thread> threads = new ArrayList<>();

    stopParsingThreads = false;
    errorMessage = "";
    for (int i = 0; i < threadCount; i++) {
      Thread thread = new ParseTimestampThread(timestampStrings, timestampValues);
      thread.start();
      threads.add(thread);
    }
    Thread.sleep(runningTime);
    stopParsingThreads = true;
    for (Thread thread : threads) {
      thread.join();
    }
    assertThat(errorMessage).isEmpty();
  }

  @Test
  @GwtIncompatible("Depends on String.format which is not supported in Xplat.")
  @J2ObjCIncompatible
  public void testTimestampInvalidFormatValueTooSmall() throws Exception {
    // Value too small.
    Timestamp value =
        Timestamp.newBuilder().setSeconds(Timestamps.TIMESTAMP_SECONDS_MIN - 1).build();
    try {
      Timestamps.toString(value);
      fail("IllegalArgumentException is expected.");
    } catch (IllegalArgumentException expected) {
    }
  }

  @Test
  @GwtIncompatible("Depends on String.format which is not supported in Xplat.")
  @J2ObjCIncompatible
  public void testTimestampInvalidFormatValueTooLarge() throws Exception {
    // Value too large.
    Timestamp value =
        Timestamp.newBuilder().setSeconds(Timestamps.TIMESTAMP_SECONDS_MAX + 1).build();
    try {
      Timestamps.toString(value);
      fail("IllegalArgumentException is expected.");
    } catch (IllegalArgumentException expected) {
    }
  }

  @Test
  @GwtIncompatible("Depends on String.format which is not supported in Xplat.")
  @J2ObjCIncompatible
  public void testTimestampInvalidFormatNanosTooSmall() throws Exception {
    // Invalid nanos value.
    Timestamp value = Timestamp.newBuilder().setNanos(-1).build();
    try {
      Timestamps.toString(value);
      fail("IllegalArgumentException is expected.");
    } catch (IllegalArgumentException expected) {
    }
  }

  @Test
  @GwtIncompatible("Depends on String.format which is not supported in Xplat.")
  @J2ObjCIncompatible
  public void testTimestampInvalidFormatNanosTooLarge() throws Exception {
    // Invalid nanos value.
    Timestamp value = Timestamp.newBuilder().setNanos(1000000000).build();
    try {
      Timestamps.toString(value);
      fail("IllegalArgumentException is expected.");
    } catch (IllegalArgumentException expected) {
    }
  }

  @Test
  @GwtIncompatible("ParseException is not supported in Xplat")
  @J2ObjCIncompatible
  public void testTimestampInvalidFormatDateTooSmall() {
    final String value = "0000-01-01T00:00:00Z";
    try {
      Timestamps.parse(value);
      fail();
    } catch (ParseException expected) {
      assertThat(expected).hasMessageThat().isNotNull();
      assertThat(expected).hasCauseThat().isNotNull();
    }
    try {
      Timestamps.parseUnchecked(value);
      fail("IllegalArgumentException is expected.");
    } catch (IllegalArgumentException expected) {
      assertThat(expected).hasMessageThat().isNotNull();
    }
  }

  @Test
  @GwtIncompatible("ParseException is not supported in Xplat")
  @J2ObjCIncompatible
  public void testTimestampInvalidFormatDateTooLarge() {
    assertParseFails("10000-01-01T00:00:00Z");
  }

  @Test
  @GwtIncompatible("ParseException is not supported in Xplat")
  @J2ObjCIncompatible
  public void testTimestampInvalidFormatMissingT() {
    assertParseFails("1970-01-01 00:00:00Z");
  }

  @Test
  @GwtIncompatible("ParseException is not supported in Xplat")
  @J2ObjCIncompatible
  public void testTimestampInvalidFormatMissingZ() {
    assertParseFails("1970-01-01T00:00:00");
  }

  @Test
  @GwtIncompatible("ParseException is not supported in Xplat")
  @J2ObjCIncompatible
  public void testTimestampInvalidOffset() {
    assertParseFails("1970-01-01T00:00:00+0000");
  }

  @Test
  @GwtIncompatible("ParseException is not supported in Xplat")
  @J2ObjCIncompatible
  public void testTimestampInvalidOffsetWithDot() {
    assertParseFails("2021-08-19T10:24:25-07.:00");
  }

  @Test
  @GwtIncompatible("ParseException is not supported in Xplat")
  @J2ObjCIncompatible
  public void testTimestampInvalidTrailingText() {
    assertParseFails("1970-01-01T00:00:00Z0");
  }

  @Test
  @GwtIncompatible("ParseException is not supported in Xplat")
  @J2ObjCIncompatible
  public void testTimestampInvalidNanoSecond() {
    assertParseFails("1970-01-01T00:00:00.ABCZ");
  }

  @Test
  @GwtIncompatible("ParseException is not supported in Xplat")
  @J2ObjCIncompatible
  public void testTimestampParseInvalidMonth() throws Exception {
    final String value = "2000-40-01T00:00:00Z";
    final String expected = "2003-04-01T00:00:00Z";
    // TODO: b/379874415 - this shouldn't parse successfully
    assertThat(Timestamps.parse(value)).isEqualTo(Timestamps.parse(expected));
    assertThat(Timestamps.parseUnchecked(value)).isEqualTo(Timestamps.parse(expected));
  }

  @GwtIncompatible("ParseException is not supported in Xplat")
  @J2ObjCIncompatible
  private static void assertParseFails(String value) {
    try {
      Timestamps.parse(value);
      fail("ParseException is expected.");
    } catch (ParseException expected) {
      assertThat(expected).hasMessageThat().isNotNull();
    }
  }

  @Test
  @GwtIncompatible("Depends on String.format which is not supported in Xplat.")
  @J2ObjCIncompatible
  public void testTimestampConversion() throws Exception {
    Timestamp timestamp = Timestamps.parse("1970-01-01T00:00:01.111111111Z");
    assertThat(Timestamps.toNanos(timestamp)).isEqualTo(1111111111);
    assertThat(Timestamps.toMicros(timestamp)).isEqualTo(1111111);
    assertThat(Timestamps.toMillis(timestamp)).isEqualTo(1111);
    assertThat(Timestamps.toSeconds(timestamp)).isEqualTo(1);

    assertThat(Timestamps.toString(Timestamps.fromNanos(1111111111)))
        .isEqualTo("1970-01-01T00:00:01.111111111Z");
    assertThat(Timestamps.toString(Timestamps.fromMicros(1111111)))
        .isEqualTo("1970-01-01T00:00:01.111111Z");
    assertThat(Timestamps.toString(Timestamps.fromMillis(1111)))
        .isEqualTo("1970-01-01T00:00:01.111Z");
    assertThat(Timestamps.toString(Timestamps.fromSeconds(1))).isEqualTo("1970-01-01T00:00:01Z");

    timestamp = Timestamps.parse("1969-12-31T23:59:59.111111111Z");
    assertThat(Timestamps.toNanos(timestamp)).isEqualTo(-888888889);
    assertThat(Timestamps.toMicros(timestamp)).isEqualTo(-888889);
    assertThat(Timestamps.toMillis(timestamp)).isEqualTo(-889);
    assertThat(Timestamps.toSeconds(timestamp)).isEqualTo(-1);

    assertThat(Timestamps.toString(Timestamps.fromNanos(-888888889)))
        .isEqualTo("1969-12-31T23:59:59.111111111Z");
    assertThat(Timestamps.toString(Timestamps.fromMicros(-888889)))
        .isEqualTo("1969-12-31T23:59:59.111111Z");
    assertThat(Timestamps.toString(Timestamps.fromMillis(-889)))
        .isEqualTo("1969-12-31T23:59:59.111Z");
    assertThat(Timestamps.toString(Timestamps.fromSeconds(-1))).isEqualTo("1969-12-31T23:59:59Z");
  }

  @Test
  @GwtIncompatible("Depends on String.format which is not supported in Xplat.")
  @J2ObjCIncompatible
  public void testFromDate() {
    Date date = new Date(1111);
    Timestamp timestamp = Timestamps.fromDate(date);
    assertThat(Timestamps.toString(timestamp)).isEqualTo("1970-01-01T00:00:01.111Z");
  }

  @Test
  @GwtIncompatible("Calendar is not supported in non JVM java.time")
  @J2ObjCIncompatible
  public void testFromDate_after9999CE() {
    // protobuf still requires Java 7 so no java.time :-(
    Calendar calendar = Calendar.getInstance();
    calendar.clear(); // avoid random number of milliseconds
    calendar.setTimeZone(TimeZone.getTimeZone("GMT-0"));
    calendar.set(20000, Calendar.OCTOBER, 20, 5, 4, 3);
    Date date = calendar.getTime();
    try {
      Timestamps.fromDate(date);
      fail("should have thrown IllegalArgumentException");
    } catch (IllegalArgumentException expected) {
      assertThat(expected).hasMessageThat().startsWith("Timestamp is not valid.");
    }
  }

  @Test
  @GwtIncompatible("Calendar is not supported in non JVM java.time")
  @J2ObjCIncompatible
  public void testFromDate_beforeYear1() {
    // protobuf still requires Java 7 so no java.time :-(
    Calendar calendar = Calendar.getInstance();
    calendar.clear(); // avoid random number of milliseconds
    calendar.setTimeZone(TimeZone.getTimeZone("GMT-0"));
    calendar.set(-32, Calendar.OCTOBER, 20, 5, 4, 3);
    Date date = calendar.getTime();
    try {
      Timestamps.fromDate(date);
      fail("should have thrown IllegalArgumentException");
    } catch (IllegalArgumentException expected) {
      assertThat(expected).hasMessageThat().startsWith("Timestamp is not valid.");
    }
  }

  @Test
  @GwtIncompatible("Calendar is not supported in non JVM java.time")
  @J2ObjCIncompatible
  public void testFromDate_after2262CE() {
    // protobuf still requires Java 7 so no java.time :-(
    Calendar calendar = Calendar.getInstance();
    calendar.clear(); // avoid random number of milliseconds
    calendar.setTimeZone(TimeZone.getTimeZone("GMT-0"));
    calendar.set(2299, Calendar.OCTOBER, 20, 5, 4, 3);
    Date date = calendar.getTime();
    Timestamp timestamp = Timestamps.fromDate(date);
    assertThat(Timestamps.toString(timestamp)).isEqualTo("2299-10-20T05:04:03Z");
  }

  /* Timestamp only stores integral seconds in the Date parent class and stores the nanosecond
   * adjustment in the Timestamp class. */
  @Test
  @GwtIncompatible("Depends on String.format which is not supported in Xplat.")
  @J2ObjCIncompatible
  public void testFromSqlTimestampSubMillisecondPrecision() {
    java.sql.Timestamp sqlTimestamp = new java.sql.Timestamp(1111);
    sqlTimestamp.setNanos(sqlTimestamp.getNanos() + 234567);
    Timestamp timestamp = Timestamps.fromDate(sqlTimestamp);
    assertThat(Timestamps.toString(timestamp)).isEqualTo("1970-01-01T00:00:01.111234567Z");
  }

  @Test
  @GwtIncompatible("Depends on String.format which is not supported in Xplat.")
  @J2ObjCIncompatible
  public void testFromSqlTimestamp() {
    Date date = new java.sql.Timestamp(1111);
    Timestamp timestamp = Timestamps.fromDate(date);
    assertThat(Timestamps.toString(timestamp)).isEqualTo("1970-01-01T00:00:01.111Z");
  }

  @Test
  @GwtIncompatible("Depends on String.format which is not supported in Xplat.")
  @J2ObjCIncompatible
  public void testFromSqlTimestamp_beforeEpoch() {
    Date date = new java.sql.Timestamp(-1111);
    Timestamp timestamp = Timestamps.fromDate(date);
    assertThat(Timestamps.toString(timestamp)).isEqualTo("1969-12-31T23:59:58.889Z");
  }

  @Test
  @GwtIncompatible("Depends on String.format which is not supported in Xplat.")
  @J2ObjCIncompatible
  public void testFromSqlTimestamp_beforeEpochWholeSecond() {
    Date date = new java.sql.Timestamp(-2000);
    Timestamp timestamp = Timestamps.fromDate(date);
    assertThat(Timestamps.toString(timestamp)).isEqualTo("1969-12-31T23:59:58Z");
  }

  @Test
  @GwtIncompatible("ParseException is not supported in Xplat")
  @J2ObjCIncompatible
  public void testTimeOperations() throws Exception {
    Timestamp start = Timestamps.parse("0001-01-01T00:00:00Z");
    Timestamp end = Timestamps.parse("9999-12-31T23:59:59.999999999Z");

    Duration duration = Timestamps.between(start, end);
    assertThat(Durations.toString(duration)).isEqualTo("315537897599.999999999s");
    Timestamp value = Timestamps.add(start, duration);
    assertThat(value).isEqualTo(end);
    value = Timestamps.subtract(end, duration);
    assertThat(value).isEqualTo(start);

    duration = Timestamps.between(end, start);
    assertThat(Durations.toString(duration)).isEqualTo("-315537897599.999999999s");
    value = Timestamps.add(end, duration);
    assertThat(value).isEqualTo(start);
    value = Timestamps.subtract(start, duration);
    assertThat(value).isEqualTo(end);
  }

  @Test
  public void testComparator() {
    assertThat(Timestamps.compare(timestamp(3, 2), timestamp(3, 2))).isEqualTo(0);
    assertThat(Timestamps.compare(timestamp(0, 0), timestamp(0, 0))).isEqualTo(0);
    assertThat(Timestamps.compare(timestamp(3, 1), timestamp(1, 1))).isGreaterThan(0);
    assertThat(Timestamps.compare(timestamp(3, 2), timestamp(3, 1))).isGreaterThan(0);
    assertThat(Timestamps.compare(timestamp(1, 1), timestamp(3, 1))).isLessThan(0);
    assertThat(Timestamps.compare(timestamp(3, 1), timestamp(3, 2))).isLessThan(0);
    assertThat(Timestamps.compare(timestamp(-3, 1), timestamp(-1, 1))).isLessThan(0);
    assertThat(Timestamps.compare(timestamp(-3, 2), timestamp(-3, 3))).isLessThan(0);
    assertThat(Timestamps.compare(timestamp(-1, 1), timestamp(-3, 1))).isGreaterThan(0);
    assertThat(Timestamps.compare(timestamp(-3, 1), timestamp(-3, 2))).isLessThan(0);
    assertThat(Timestamps.compare(timestamp(-10, 1), timestamp(1, 1))).isLessThan(0);
    assertThat(Timestamps.compare(timestamp(0, 1), timestamp(0, 1))).isEqualTo(0);
    assertThat(Timestamps.compare(timestamp(0x80000000L, 0), timestamp(0, 0))).isGreaterThan(0);
    assertThat(Timestamps.compare(timestamp(0xFFFFFFFF00000000L, 0), timestamp(0, 0)))
        .isLessThan(0);

    Timestamp timestamp0 = timestamp(-50, 400);
    Timestamp timestamp1 = timestamp(-50, 500);
    Timestamp timestamp2 = timestamp(50, 500);
    Timestamp timestamp3 = timestamp(100, 20);
    Timestamp timestamp4 = timestamp(100, 50);
    Timestamp timestamp5 = timestamp(100, 150);
    Timestamp timestamp6 = timestamp(150, 40);

    List<Timestamp> timestamps =
        Lists.newArrayList(
            timestamp5,
            timestamp3,
            timestamp1,
            timestamp4,
            timestamp6,
            timestamp2,
            timestamp0,
            timestamp3);

    Collections.sort(timestamps, Timestamps.comparator());
    assertThat(timestamps)
        .containsExactly(
            timestamp0,
            timestamp1,
            timestamp2,
            timestamp3,
            timestamp3,
            timestamp4,
            timestamp5,
            timestamp6)
        .inOrder();
  }

  @Test
  public void testCompare() {
    assertThat(Timestamps.compare(timestamp(3, 2), timestamp(3, 2))).isEqualTo(0);
    assertThat(Timestamps.compare(timestamp(0, 0), timestamp(0, 0))).isEqualTo(0);
    assertThat(Timestamps.compare(timestamp(3, 1), timestamp(1, 1))).isGreaterThan(0);
    assertThat(Timestamps.compare(timestamp(3, 2), timestamp(3, 1))).isGreaterThan(0);
    assertThat(Timestamps.compare(timestamp(1, 1), timestamp(3, 1))).isLessThan(0);
    assertThat(Timestamps.compare(timestamp(3, 1), timestamp(3, 2))).isLessThan(0);
    assertThat(Timestamps.compare(timestamp(-3, 1), timestamp(-1, 1))).isLessThan(0);
    assertThat(Timestamps.compare(timestamp(-3, 2), timestamp(-3, 3))).isLessThan(0);
    assertThat(Timestamps.compare(timestamp(-1, 1), timestamp(-3, 1))).isGreaterThan(0);
    assertThat(Timestamps.compare(timestamp(-3, 1), timestamp(-3, 2))).isLessThan(0);
    assertThat(Timestamps.compare(timestamp(-10, 1), timestamp(1, 1))).isLessThan(0);
    assertThat(Timestamps.compare(timestamp(0, 1), timestamp(0, 1))).isEqualTo(0);
    assertThat(Timestamps.compare(timestamp(0x80000000L, 0), timestamp(0, 0))).isGreaterThan(0);
    assertThat(Timestamps.compare(timestamp(0xFFFFFFFF00000000L, 0), timestamp(0, 0)))
        .isLessThan(0);
  }

  @Test
  @GwtIncompatible("ParseException is not supported in Xplat")
  @J2ObjCIncompatible
  public void testOverflowsArithmeticException() throws Exception {
    Timestamp timestamp = Timestamps.parse("9999-12-31T23:59:59.999999999Z");
    try {
      Timestamps.toNanos(timestamp);
      fail("Expected an ArithmeticException to be thrown");
    } catch (ArithmeticException expected) {
    }
  }

  @Test
  public void testPositiveOverflow() {
    try {
      Timestamps.add(Timestamps.MAX_VALUE, Durations.MAX_VALUE);
      fail("Expected an IllegalArgumentException to be thrown");
    } catch (IllegalArgumentException expected) {
    }
  }

  @Test
  public void testNegativeOverflow() {
    try {
      Timestamps.subtract(Timestamps.MIN_VALUE, Durations.MAX_VALUE);
      fail("Expected an IllegalArgumentException to be thrown");
    } catch (IllegalArgumentException expected) {
    }
  }

  @Test
  public void testInvalidMaxNanosecondsOverflow() {
    try {
      Timestamps.toNanos(INVALID_MAX);
      fail("Expected an IllegalArgumentException to be thrown");
    } catch (IllegalArgumentException expected) {
    }
  }

  @Test
  public void testInvalidMaxMicrosecondsOverflow() {
    try {
      Timestamps.toMicros(INVALID_MAX);
      fail("Expected an IllegalArgumentException to be thrown");
    } catch (IllegalArgumentException expected) {
    }
  }

  @Test
  public void testInvalidMaxMillisecondsOverflow() {
    try {
      Timestamps.toMillis(INVALID_MAX);
      fail("Expected an IllegalArgumentException to be thrown");
    } catch (IllegalArgumentException expected) {
    }
  }

  @Test
  public void testInvalidMaxSecondsOverflow() {
    try {
      Timestamps.toSeconds(INVALID_MAX);
      fail("Expected an IllegalArgumentException to be thrown");
    } catch (IllegalArgumentException expected) {
    }
  }

  @Test
  public void testInvalidMinNanosecondsOverflow() {
    try {
      Timestamps.toNanos(INVALID_MIN);
      fail("Expected an IllegalArgumentException to be thrown");
    } catch (IllegalArgumentException expected) {
    }
  }

  @Test
  public void testInvalidMicrosecondsMinOverflow() {
    try {
      Timestamps.toMicros(INVALID_MIN);
      fail("Expected an IllegalArgumentException to be thrown");
    } catch (IllegalArgumentException expected) {
    }
  }

  @Test
  public void testInvalidMinMillisecondsOverflow() {
    try {
      Timestamps.toMillis(INVALID_MIN);
      fail("Expected an IllegalArgumentException to be thrown");
    } catch (IllegalArgumentException expected) {
    }
  }

  @Test
  public void testOverInvalidMinSecondsflow() {
    try {
      Timestamps.toSeconds(INVALID_MIN);
      fail("Expected an IllegalArgumentException to be thrown");
    } catch (IllegalArgumentException expected) {
    }
  }

  @Test
  @GwtIncompatible("Depends on String.format which is not supported in Xplat.")
  @J2ObjCIncompatible
  public void testMaxNanosecondsConversion() {
    assertThat(Timestamps.toString(Timestamps.fromNanos(Long.MAX_VALUE)))
        .isEqualTo("2262-04-11T23:47:16.854775807Z");
  }

  @Test
  public void testIllegalArgumentExceptionForMaxMicroseconds() {
    try {
      Timestamps.fromMicros(Long.MAX_VALUE);
      fail("Expected an IllegalArgumentException to be thrown");
    } catch (IllegalArgumentException expected) {
    }
  }

  @Test
  public void testIllegalArgumentExceptionForMaxMilliseconds() {
    try {
      Durations.fromMillis(Long.MAX_VALUE);
      fail("Expected an IllegalArgumentException to be thrown");
    } catch (IllegalArgumentException expected) {
    }
  }

  @Test
  @GwtIncompatible("Depends on String.format which is not supported in Xplat.")
  @J2ObjCIncompatible
  public void testMinNanosecondsConversion() {
    assertThat(Timestamps.toString(Timestamps.fromNanos(Long.MIN_VALUE)))
        .isEqualTo("1677-09-21T00:12:43.145224192Z");
  }

  @Test
  public void testIllegalArgumentExceptionForMinMicroseconds() {
    try {
      Timestamps.fromMicros(Long.MIN_VALUE);
      fail("Expected an IllegalArgumentException to be thrown");
    } catch (IllegalArgumentException expected) {
    }
  }

  @Test
  public void testIllegalArgumentExceptionForMinMilliseconds() {
    try {
      Timestamps.fromMillis(Long.MIN_VALUE);
      fail("Expected an IllegalArgumentException to be thrown");
    } catch (IllegalArgumentException expected) {
    }
  }

  @Test
  public void testConvertFromSeconds() {
    assertThat(Timestamps.fromSeconds(SECONDS).getSeconds()).isEqualTo(TIMESTAMP.getSeconds());
    assertThat(Timestamps.EPOCH).isEqualTo(ZERO_TIMESTAMP);
    assertThat(Timestamps.fromSeconds(MAX_VALUE)).isEqualTo(ZERO_TIMESTAMP);
    assertThat(Timestamps.fromSeconds(MIN_VALUE)).isEqualTo(ZERO_TIMESTAMP);
  }

  @Test
  public void testConvertFromMillis() {
    assertThat(Timestamps.fromMillis(MILLIS)).isEqualTo(TIMESTAMP);
    assertThat(Timestamps.EPOCH).isEqualTo(ZERO_TIMESTAMP);
    assertThat(Timestamps.fromMillis(-1)).isEqualTo(ONE_OF_TIMESTAMP);
    assertThat(Timestamps.fromMillis(MAX_VALUE)).isEqualTo(ZERO_TIMESTAMP);
    assertThat(Timestamps.fromMillis(MIN_VALUE)).isEqualTo(ZERO_TIMESTAMP);
  }

  @Test
  public void testConvertFromMicros() {
    assertThat(Timestamps.fromMicros(MICROS)).isEqualTo(TIMESTAMP);
    assertThat(Timestamps.EPOCH).isEqualTo(ZERO_TIMESTAMP);
    assertThat(Timestamps.fromMicros(-1000)).isEqualTo(ONE_OF_TIMESTAMP);
    assertThat(Timestamps.fromMicros(MAX_VALUE)).isEqualTo(ZERO_TIMESTAMP);
    assertThat(Timestamps.fromMicros(MIN_VALUE)).isEqualTo(ZERO_TIMESTAMP);
  }

  @Test
  public void testConvertToSeconds() {
    assertThat(Timestamps.toSeconds(TIMESTAMP)).isEqualTo(SECONDS);
    assertThat(Timestamps.toSeconds(ZERO_TIMESTAMP)).isEqualTo(0);
    assertThat(Timestamps.toSeconds(ONE_OF_TIMESTAMP)).isEqualTo(-1);
  }

  @Test
  public void testConvertToMillis() {
    assertThat(Timestamps.toMillis(TIMESTAMP)).isEqualTo(MILLIS);
    assertThat(Timestamps.toMillis(ZERO_TIMESTAMP)).isEqualTo(0);
    assertThat(Timestamps.toMillis(ONE_OF_TIMESTAMP)).isEqualTo(-1);
  }

  @Test
  public void testConvertToMicros() {
    assertThat(Timestamps.toMicros(TIMESTAMP)).isEqualTo(MICROS);
    assertThat(Timestamps.toMicros(ZERO_TIMESTAMP)).isEqualTo(0);
    assertThat(Timestamps.toMicros(ONE_OF_TIMESTAMP)).isEqualTo(-1000);
  }

  @Test
  public void testConvertFromMillisAboveTimestampMaxLimit() {
    long timestampMaxSeconds = 253402300799L;
    try {
      Timestamps.fromMillis((timestampMaxSeconds + 1) * MILLIS_PER_SECOND);
      fail("Expected an IllegalArgumentException to be thrown");
    } catch (IllegalArgumentException expected) {
    }
  }

  @Test
  public void testConvertFromMillisBelowTimestampMaxLimit() {
    long timestampMinSeconds = -62135596800L;
    try {
      Timestamps.fromMillis((timestampMinSeconds - 1) * MILLIS_PER_SECOND);
      fail("Expected an IllegalArgumentException to be thrown");
    } catch (IllegalArgumentException expected) {
    }
  }

  @Test
  public void testConvertFromNanos() {
    assertThat(Timestamps.fromNanos(NANOS)).isEqualTo(TIMESTAMP);
    assertThat(Timestamps.fromNanos(0)).isEqualTo(ZERO_TIMESTAMP);
    assertThat(Timestamps.fromNanos(-1000000)).isEqualTo(ONE_OF_TIMESTAMP);
    assertThat(Timestamps.fromNanos(MAX_VALUE)).isEqualTo(ZERO_TIMESTAMP);
    assertThat(Timestamps.fromNanos(MIN_VALUE)).isEqualTo(ZERO_TIMESTAMP);
  }

  @Test
  public void testConvertToNanos() {
    assertThat(Timestamps.toNanos(TIMESTAMP)).isEqualTo(NANOS);
    assertThat(Timestamps.toNanos(ZERO_TIMESTAMP)).isEqualTo(0);
    assertThat(Timestamps.toNanos(ONE_OF_TIMESTAMP)).isEqualTo(-1000000);
  }

  @Test
  public void testAdd() {
    assertThat(Timestamps.add(timestamp(1, 10), duration(1, 20))).isEqualTo(timestamp(2, 30));
    assertThat(Timestamps.add(timestamp(1, 10), duration(-1, -11)))
        .isEqualTo(timestamp(-1, 999999999));
    assertThat(Timestamps.add(timestamp(10, 10), duration(-1, -11)))
        .isEqualTo(timestamp(8, 999999999));
    assertThat(Timestamps.add(timestamp(1, 1), duration(1, 999999999))).isEqualTo(timestamp(3, 0));
    assertThat(Timestamps.add(timestamp(1, 2), duration(1, 999999999))).isEqualTo(timestamp(3, 1));
  }

  @Test
  public void testSubtractDuration() {
    assertThat(Timestamps.subtract(timestamp(1, 10), duration(-1, -20)))
        .isEqualTo(timestamp(2, 30));
    assertThat(Timestamps.subtract(timestamp(1, 10), duration(1, 11)))
        .isEqualTo(timestamp(-1, 999999999));
    assertThat(Timestamps.subtract(timestamp(10, 10), duration(1, 11)))
        .isEqualTo(timestamp(8, 999999999));
    assertThat(Timestamps.subtract(timestamp(1, 1), duration(-1, -999999999)))
        .isEqualTo(timestamp(3, 0));
    assertThat(Timestamps.subtract(timestamp(1, 2), duration(-1, -999999999)))
        .isEqualTo(timestamp(3, 1));
  }

  @Test
  public void normalizedTimestamp_veryLarge_isInvalidNotOverflow() {
    try {
      Timestamps.normalizedTimestamp(9223372036854775807L, 2);
      fail("should have thrown IllegalArgumentException");
    } catch (IllegalArgumentException expected) {
      assertThat(expected).hasMessageThat().startsWith("Timestamp is not valid.");
    }
  }

  static Timestamp timestamp(long seconds, int nanos) {
    return Timestamps.checkValid(
        Timestamps.checkValid(Timestamp.newBuilder().setSeconds(seconds).setNanos(nanos)));
  }
}
