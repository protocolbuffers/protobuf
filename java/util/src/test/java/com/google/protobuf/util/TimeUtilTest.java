// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
// https://developers.google.com/protocol-buffers/
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

package com.google.protobuf.util;

import com.google.protobuf.Duration;
import com.google.protobuf.Timestamp;
import java.text.ParseException;
import java.util.ArrayList;
import java.util.List;
import junit.framework.TestCase;
import org.junit.Assert;

/** Unit tests for {@link TimeUtil}. */
public class TimeUtilTest extends TestCase {
  public void testTimestampStringFormat() throws Exception {
    Timestamp start = TimeUtil.parseTimestamp("0001-01-01T00:00:00Z");
    Timestamp end = TimeUtil.parseTimestamp("9999-12-31T23:59:59.999999999Z");
    assertEquals(TimeUtil.TIMESTAMP_SECONDS_MIN, start.getSeconds());
    assertEquals(0, start.getNanos());
    assertEquals(TimeUtil.TIMESTAMP_SECONDS_MAX, end.getSeconds());
    assertEquals(999999999, end.getNanos());
    assertEquals("0001-01-01T00:00:00Z", TimeUtil.toString(start));
    assertEquals("9999-12-31T23:59:59.999999999Z", TimeUtil.toString(end));

    Timestamp value = TimeUtil.parseTimestamp("1970-01-01T00:00:00Z");
    assertEquals(0, value.getSeconds());
    assertEquals(0, value.getNanos());

    // Test negative timestamps.
    value = TimeUtil.parseTimestamp("1969-12-31T23:59:59.999Z");
    assertEquals(-1, value.getSeconds());
    // Nano part is in the range of [0, 999999999] for Timestamp.
    assertEquals(999000000, value.getNanos());

    // Test that 3, 6, or 9 digits are used for the fractional part.
    value = Timestamp.newBuilder().setNanos(10).build();
    assertEquals("1970-01-01T00:00:00.000000010Z", TimeUtil.toString(value));
    value = Timestamp.newBuilder().setNanos(10000).build();
    assertEquals("1970-01-01T00:00:00.000010Z", TimeUtil.toString(value));
    value = Timestamp.newBuilder().setNanos(10000000).build();
    assertEquals("1970-01-01T00:00:00.010Z", TimeUtil.toString(value));

    // Test that parsing accepts timezone offsets.
    value = TimeUtil.parseTimestamp("1970-01-01T00:00:00.010+08:00");
    assertEquals("1969-12-31T16:00:00.010Z", TimeUtil.toString(value));
    value = TimeUtil.parseTimestamp("1970-01-01T00:00:00.010-08:00");
    assertEquals("1970-01-01T08:00:00.010Z", TimeUtil.toString(value));
  }

  private volatile boolean stopParsingThreads = false;
  private volatile String errorMessage = "";

  private class ParseTimestampThread extends Thread {
    private final String[] strings;
    private final Timestamp[] values;

    public ParseTimestampThread(String[] strings, Timestamp[] values) {
      this.strings = strings;
      this.values = values;
    }

    @Override
    public void run() {
      int index = 0;
      while (!stopParsingThreads) {
        Timestamp result;
        try {
          result = TimeUtil.parseTimestamp(strings[index]);
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
      timestampValues[i] = TimeUtil.parseTimestamp(timestampStrings[i]);
    }

    final int THREAD_COUNT = 16;
    final int RUNNING_TIME = 5000; // in milliseconds.
    final List<Thread> threads = new ArrayList<Thread>();

    stopParsingThreads = false;
    errorMessage = "";
    for (int i = 0; i < THREAD_COUNT; i++) {
      Thread thread = new ParseTimestampThread(timestampStrings, timestampValues);
      thread.start();
      threads.add(thread);
    }
    Thread.sleep(RUNNING_TIME);
    stopParsingThreads = true;
    for (Thread thread : threads) {
      thread.join();
    }
    Assert.assertEquals("", errorMessage);
  }

  public void testTimetampInvalidFormat() throws Exception {
    try {
      // Value too small.
      Timestamp value =
          Timestamp.newBuilder().setSeconds(TimeUtil.TIMESTAMP_SECONDS_MIN - 1).build();
      TimeUtil.toString(value);
      Assert.fail("Exception is expected.");
    } catch (IllegalArgumentException e) {
      // Expected.
    }

    try {
      // Value too large.
      Timestamp value =
          Timestamp.newBuilder().setSeconds(TimeUtil.TIMESTAMP_SECONDS_MAX + 1).build();
      TimeUtil.toString(value);
      Assert.fail("Exception is expected.");
    } catch (IllegalArgumentException e) {
      // Expected.
    }

    try {
      // Invalid nanos value.
      Timestamp value = Timestamp.newBuilder().setNanos(-1).build();
      TimeUtil.toString(value);
      Assert.fail("Exception is expected.");
    } catch (IllegalArgumentException e) {
      // Expected.
    }

    try {
      // Invalid nanos value.
      Timestamp value = Timestamp.newBuilder().setNanos(1000000000).build();
      TimeUtil.toString(value);
      Assert.fail("Exception is expected.");
    } catch (IllegalArgumentException e) {
      // Expected.
    }

    try {
      // Value to small.
      TimeUtil.parseTimestamp("0000-01-01T00:00:00Z");
      Assert.fail("Exception is expected.");
    } catch (ParseException e) {
      // Expected.
    }

    try {
      // Value to large.
      TimeUtil.parseTimestamp("10000-01-01T00:00:00Z");
      Assert.fail("Exception is expected.");
    } catch (ParseException e) {
      // Expected.
    }

    try {
      // Missing 'T'.
      TimeUtil.parseTimestamp("1970-01-01 00:00:00Z");
      Assert.fail("Exception is expected.");
    } catch (ParseException e) {
      // Expected.
    }

    try {
      // Missing 'Z'.
      TimeUtil.parseTimestamp("1970-01-01T00:00:00");
      Assert.fail("Exception is expected.");
    } catch (ParseException e) {
      // Expected.
    }

    try {
      // Invalid offset.
      TimeUtil.parseTimestamp("1970-01-01T00:00:00+0000");
      Assert.fail("Exception is expected.");
    } catch (ParseException e) {
      // Expected.
    }

    try {
      // Trailing text.
      TimeUtil.parseTimestamp("1970-01-01T00:00:00Z0");
      Assert.fail("Exception is expected.");
    } catch (ParseException e) {
      // Expected.
    }

    try {
      // Invalid nanosecond value.
      TimeUtil.parseTimestamp("1970-01-01T00:00:00.ABCZ");
      Assert.fail("Exception is expected.");
    } catch (ParseException e) {
      // Expected.
    }
  }

  public void testDurationStringFormat() throws Exception {
    Timestamp start = TimeUtil.parseTimestamp("0001-01-01T00:00:00Z");
    Timestamp end = TimeUtil.parseTimestamp("9999-12-31T23:59:59.999999999Z");
    Duration duration = TimeUtil.distance(start, end);
    assertEquals("315537897599.999999999s", TimeUtil.toString(duration));
    duration = TimeUtil.distance(end, start);
    assertEquals("-315537897599.999999999s", TimeUtil.toString(duration));

    // Generated output should contain 3, 6, or 9 fractional digits.
    duration = Duration.newBuilder().setSeconds(1).build();
    assertEquals("1s", TimeUtil.toString(duration));
    duration = Duration.newBuilder().setNanos(10000000).build();
    assertEquals("0.010s", TimeUtil.toString(duration));
    duration = Duration.newBuilder().setNanos(10000).build();
    assertEquals("0.000010s", TimeUtil.toString(duration));
    duration = Duration.newBuilder().setNanos(10).build();
    assertEquals("0.000000010s", TimeUtil.toString(duration));

    // Parsing accepts an fractional digits as long as they fit into nano
    // precision.
    duration = TimeUtil.parseDuration("0.1s");
    assertEquals(100000000, duration.getNanos());
    duration = TimeUtil.parseDuration("0.0001s");
    assertEquals(100000, duration.getNanos());
    duration = TimeUtil.parseDuration("0.0000001s");
    assertEquals(100, duration.getNanos());

    // Duration must support range from -315,576,000,000s to +315576000000s
    // which includes negative values.
    duration = TimeUtil.parseDuration("315576000000.999999999s");
    assertEquals(315576000000L, duration.getSeconds());
    assertEquals(999999999, duration.getNanos());
    duration = TimeUtil.parseDuration("-315576000000.999999999s");
    assertEquals(-315576000000L, duration.getSeconds());
    assertEquals(-999999999, duration.getNanos());
  }

  public void testDurationInvalidFormat() throws Exception {
    try {
      // Value too small.
      Duration value = Duration.newBuilder().setSeconds(TimeUtil.DURATION_SECONDS_MIN - 1).build();
      TimeUtil.toString(value);
      Assert.fail("Exception is expected.");
    } catch (IllegalArgumentException e) {
      // Expected.
    }

    try {
      // Value too large.
      Duration value = Duration.newBuilder().setSeconds(TimeUtil.DURATION_SECONDS_MAX + 1).build();
      TimeUtil.toString(value);
      Assert.fail("Exception is expected.");
    } catch (IllegalArgumentException e) {
      // Expected.
    }

    try {
      // Invalid nanos value.
      Duration value = Duration.newBuilder().setSeconds(1).setNanos(-1).build();
      TimeUtil.toString(value);
      Assert.fail("Exception is expected.");
    } catch (IllegalArgumentException e) {
      // Expected.
    }

    try {
      // Invalid nanos value.
      Duration value = Duration.newBuilder().setSeconds(-1).setNanos(1).build();
      TimeUtil.toString(value);
      Assert.fail("Exception is expected.");
    } catch (IllegalArgumentException e) {
      // Expected.
    }

    try {
      // Value too small.
      TimeUtil.parseDuration("-315576000001s");
      Assert.fail("Exception is expected.");
    } catch (ParseException e) {
      // Expected.
    }

    try {
      // Value too large.
      TimeUtil.parseDuration("315576000001s");
      Assert.fail("Exception is expected.");
    } catch (ParseException e) {
      // Expected.
    }

    try {
      // Empty.
      TimeUtil.parseDuration("");
      Assert.fail("Exception is expected.");
    } catch (ParseException e) {
      // Expected.
    }

    try {
      // Missing "s".
      TimeUtil.parseDuration("0");
      Assert.fail("Exception is expected.");
    } catch (ParseException e) {
      // Expected.
    }

    try {
      // Invalid trailing data.
      TimeUtil.parseDuration("0s0");
      Assert.fail("Exception is expected.");
    } catch (ParseException e) {
      // Expected.
    }

    try {
      // Invalid prefix.
      TimeUtil.parseDuration("--1s");
      Assert.fail("Exception is expected.");
    } catch (ParseException e) {
      // Expected.
    }
  }

  public void testTimestampConversion() throws Exception {
    Timestamp timestamp = TimeUtil.parseTimestamp("1970-01-01T00:00:01.111111111Z");
    assertEquals(1111111111, TimeUtil.toNanos(timestamp));
    assertEquals(1111111, TimeUtil.toMicros(timestamp));
    assertEquals(1111, TimeUtil.toMillis(timestamp));
    timestamp = TimeUtil.createTimestampFromNanos(1111111111);
    assertEquals("1970-01-01T00:00:01.111111111Z", TimeUtil.toString(timestamp));
    timestamp = TimeUtil.createTimestampFromMicros(1111111);
    assertEquals("1970-01-01T00:00:01.111111Z", TimeUtil.toString(timestamp));
    timestamp = TimeUtil.createTimestampFromMillis(1111);
    assertEquals("1970-01-01T00:00:01.111Z", TimeUtil.toString(timestamp));

    timestamp = TimeUtil.parseTimestamp("1969-12-31T23:59:59.111111111Z");
    assertEquals(-888888889, TimeUtil.toNanos(timestamp));
    assertEquals(-888889, TimeUtil.toMicros(timestamp));
    assertEquals(-889, TimeUtil.toMillis(timestamp));
    timestamp = TimeUtil.createTimestampFromNanos(-888888889);
    assertEquals("1969-12-31T23:59:59.111111111Z", TimeUtil.toString(timestamp));
    timestamp = TimeUtil.createTimestampFromMicros(-888889);
    assertEquals("1969-12-31T23:59:59.111111Z", TimeUtil.toString(timestamp));
    timestamp = TimeUtil.createTimestampFromMillis(-889);
    assertEquals("1969-12-31T23:59:59.111Z", TimeUtil.toString(timestamp));
  }

  public void testDurationConversion() throws Exception {
    Duration duration = TimeUtil.parseDuration("1.111111111s");
    assertEquals(1111111111, TimeUtil.toNanos(duration));
    assertEquals(1111111, TimeUtil.toMicros(duration));
    assertEquals(1111, TimeUtil.toMillis(duration));
    duration = TimeUtil.createDurationFromNanos(1111111111);
    assertEquals("1.111111111s", TimeUtil.toString(duration));
    duration = TimeUtil.createDurationFromMicros(1111111);
    assertEquals("1.111111s", TimeUtil.toString(duration));
    duration = TimeUtil.createDurationFromMillis(1111);
    assertEquals("1.111s", TimeUtil.toString(duration));

    duration = TimeUtil.parseDuration("-1.111111111s");
    assertEquals(-1111111111, TimeUtil.toNanos(duration));
    assertEquals(-1111111, TimeUtil.toMicros(duration));
    assertEquals(-1111, TimeUtil.toMillis(duration));
    duration = TimeUtil.createDurationFromNanos(-1111111111);
    assertEquals("-1.111111111s", TimeUtil.toString(duration));
    duration = TimeUtil.createDurationFromMicros(-1111111);
    assertEquals("-1.111111s", TimeUtil.toString(duration));
    duration = TimeUtil.createDurationFromMillis(-1111);
    assertEquals("-1.111s", TimeUtil.toString(duration));
  }

  public void testTimeOperations() throws Exception {
    Timestamp start = TimeUtil.parseTimestamp("0001-01-01T00:00:00Z");
    Timestamp end = TimeUtil.parseTimestamp("9999-12-31T23:59:59.999999999Z");

    Duration duration = TimeUtil.distance(start, end);
    assertEquals("315537897599.999999999s", TimeUtil.toString(duration));
    Timestamp value = TimeUtil.add(start, duration);
    assertEquals(end, value);
    value = TimeUtil.subtract(end, duration);
    assertEquals(start, value);

    duration = TimeUtil.distance(end, start);
    assertEquals("-315537897599.999999999s", TimeUtil.toString(duration));
    value = TimeUtil.add(end, duration);
    assertEquals(start, value);
    value = TimeUtil.subtract(start, duration);
    assertEquals(end, value);

    // Result is larger than Long.MAX_VALUE.
    try {
      duration = TimeUtil.parseDuration("315537897599.999999999s");
      duration = TimeUtil.multiply(duration, 315537897599.999999999);
      Assert.fail("Exception is expected.");
    } catch (IllegalArgumentException e) {
      // Expected.
    }

    // Result is lesser than Long.MIN_VALUE.
    try {
      duration = TimeUtil.parseDuration("315537897599.999999999s");
      duration = TimeUtil.multiply(duration, -315537897599.999999999);
      Assert.fail("Exception is expected.");
    } catch (IllegalArgumentException e) {
      // Expected.
    }

    duration = TimeUtil.parseDuration("-1.125s");
    duration = TimeUtil.divide(duration, 2.0);
    assertEquals("-0.562500s", TimeUtil.toString(duration));
    duration = TimeUtil.multiply(duration, 2.0);
    assertEquals("-1.125s", TimeUtil.toString(duration));

    duration = TimeUtil.add(duration, duration);
    assertEquals("-2.250s", TimeUtil.toString(duration));

    duration = TimeUtil.subtract(duration, TimeUtil.parseDuration("-1s"));
    assertEquals("-1.250s", TimeUtil.toString(duration));

    // Multiplications (with results larger than Long.MAX_VALUE in nanoseconds).
    duration = TimeUtil.parseDuration("0.999999999s");
    assertEquals(
        "315575999684.424s", TimeUtil.toString(TimeUtil.multiply(duration, 315576000000L)));
    duration = TimeUtil.parseDuration("-0.999999999s");
    assertEquals(
        "-315575999684.424s", TimeUtil.toString(TimeUtil.multiply(duration, 315576000000L)));
    assertEquals(
        "315575999684.424s", TimeUtil.toString(TimeUtil.multiply(duration, -315576000000L)));

    // Divisions (with values larger than Long.MAX_VALUE in nanoseconds).
    Duration d1 = TimeUtil.parseDuration("315576000000s");
    Duration d2 = TimeUtil.subtract(d1, TimeUtil.createDurationFromNanos(1));
    assertEquals(1, TimeUtil.divide(d1, d2));
    assertEquals(0, TimeUtil.divide(d2, d1));
    assertEquals("0.000000001s", TimeUtil.toString(TimeUtil.remainder(d1, d2)));
    assertEquals("315575999999.999999999s", TimeUtil.toString(TimeUtil.remainder(d2, d1)));

    // Divisions involving negative values.
    //
    // (-5) / 2 = -2, remainder = -1
    d1 = TimeUtil.parseDuration("-5s");
    d2 = TimeUtil.parseDuration("2s");
    assertEquals(-2, TimeUtil.divide(d1, d2));
    assertEquals(-2, TimeUtil.divide(d1, 2).getSeconds());
    assertEquals(-1, TimeUtil.remainder(d1, d2).getSeconds());
    // (-5) / (-2) = 2, remainder = -1
    d1 = TimeUtil.parseDuration("-5s");
    d2 = TimeUtil.parseDuration("-2s");
    assertEquals(2, TimeUtil.divide(d1, d2));
    assertEquals(2, TimeUtil.divide(d1, -2).getSeconds());
    assertEquals(-1, TimeUtil.remainder(d1, d2).getSeconds());
    // 5 / (-2) = -2, remainder = 1
    d1 = TimeUtil.parseDuration("5s");
    d2 = TimeUtil.parseDuration("-2s");
    assertEquals(-2, TimeUtil.divide(d1, d2));
    assertEquals(-2, TimeUtil.divide(d1, -2).getSeconds());
    assertEquals(1, TimeUtil.remainder(d1, d2).getSeconds());
  }
}
