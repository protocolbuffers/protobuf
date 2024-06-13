// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

package com.google.protobuf.util;

import static com.google.common.truth.Truth.assertThat;
import static com.google.common.truth.Truth.assertWithMessage;
import static com.google.protobuf.util.Durations.toSecondsAsDouble;
import static org.junit.Assert.fail;

import com.google.common.collect.Lists;
import com.google.protobuf.Duration;
import com.google.protobuf.Timestamp;
import java.text.ParseException;
import java.util.Arrays;
import java.util.Collections;
import java.util.List;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

/** Unit tests for {@link Durations}. */
@RunWith(JUnit4.class)
public class DurationsTest {

  private static final Duration INVALID_MAX =
      Duration.newBuilder().setSeconds(Long.MAX_VALUE).setNanos(Integer.MAX_VALUE).build();
  private static final Duration INVALID_MIN =
      Duration.newBuilder().setSeconds(Long.MIN_VALUE).setNanos(Integer.MIN_VALUE).build();

  @Test
  public void testIsPositive() {
    assertThat(Durations.isPositive(Durations.ZERO)).isFalse();
    assertThat(Durations.isPositive(Durations.fromNanos(-1))).isFalse();
    assertThat(Durations.isPositive(Durations.fromNanos(1))).isTrue();
  }

  @Test
  public void testIsNegative() {
    assertThat(Durations.isNegative(Durations.ZERO)).isFalse();
    assertThat(Durations.isNegative(Durations.fromNanos(1))).isFalse();
    assertThat(Durations.isNegative(Durations.fromNanos(-1))).isTrue();
  }

  @Test
  public void testCheckNotNegative() {
    Durations.checkNotNegative(Durations.ZERO);
    Durations.checkNotNegative(Durations.fromNanos(1));
    Durations.checkNotNegative(Durations.fromSeconds(1));

    try {
      Durations.checkNotNegative(Durations.fromNanos(-1));
      fail();
    } catch (IllegalArgumentException expected) {
      assertThat(expected).hasMessageThat().isEqualTo(
          "duration (-0.000000001s) must not be negative");
    }
    try {
      Durations.checkNotNegative(Durations.fromSeconds(-1));
      fail();
    } catch (IllegalArgumentException expected) {
      assertThat(expected).hasMessageThat().isEqualTo("duration (-1s) must not be negative");
    }
  }

  @Test
  public void testCheckPositive() {
    Durations.checkPositive(Durations.fromNanos(1));
    Durations.checkPositive(Durations.fromSeconds(1));

    try {
      Durations.checkPositive(Durations.ZERO);
      fail();
    } catch (IllegalArgumentException expected) {
      assertThat(expected).hasMessageThat().isEqualTo("duration (0s) must be positive");
    }

    try {
      Durations.checkPositive(Durations.fromNanos(-1));
      fail();
    } catch (IllegalArgumentException expected) {
      assertThat(expected).hasMessageThat().isEqualTo("duration (-0.000000001s) must be positive");
    }

    try {
      Durations.checkPositive(Durations.fromSeconds(-1));
      fail();
    } catch (IllegalArgumentException expected) {
      assertThat(expected).hasMessageThat().isEqualTo("duration (-1s) must be positive");
    }
  }

  @Test
  public void testToSecondsAsDouble() {
    assertThat(toSecondsAsDouble(duration(0, 1))).isEqualTo(1.0e-9);
    assertThat(toSecondsAsDouble(Durations.fromMillis(999))).isEqualTo(0.999);
    assertThat(toSecondsAsDouble(duration(1, 0))).isEqualTo(1.0);
    assertWithMessage("Precision loss detected")
        .that(toSecondsAsDouble(Durations.fromNanos(1234567890987654321L)))
        .isWithin(1.0e-6)
        .of(1234567890.9876544);
  }

  @Test
  public void testRoundtripConversions() {
    for (long value : Arrays.asList(-123456, -42, -1, 0, 1, 42, 123456)) {
      assertThat(Durations.toDays(Durations.fromDays(value))).isEqualTo(value);
      assertThat(Durations.toHours(Durations.fromHours(value))).isEqualTo(value);
      assertThat(Durations.toMinutes(Durations.fromMinutes(value))).isEqualTo(value);
      assertThat(Durations.toSeconds(Durations.fromSeconds(value))).isEqualTo(value);
      assertThat(Durations.toMillis(Durations.fromMillis(value))).isEqualTo(value);
      assertThat(Durations.toMicros(Durations.fromMicros(value))).isEqualTo(value);
      assertThat(Durations.toNanos(Durations.fromNanos(value))).isEqualTo(value);
    }
  }

  @Test
  public void testStaticFactories() {
    Duration[] durations = {
      Durations.fromDays(1),
      Durations.fromHours(24),
      Durations.fromMinutes(1440),
      Durations.fromSeconds(86_400L),
      Durations.fromMillis(86_400_000L),
      Durations.fromMicros(86_400_000_000L),
      Durations.fromNanos(86_400_000_000_000L)
    };

    for (Duration d1 : durations) {
      assertThat(d1).isNotEqualTo(null);
      for (Duration d2 : durations) {
        assertThat(d1).isEqualTo(d2);
      }
    }
  }

  @Test
  public void testMinMaxAreValid() {
    assertThat(Durations.isValid(Durations.MAX_VALUE)).isTrue();
    assertThat(Durations.isValid(Durations.MIN_VALUE)).isTrue();
  }

  @Test
  public void testIsValid_false() {
    assertThat(Durations.isValid(-315576000001L, 0)).isFalse();
    assertThat(Durations.isValid(315576000001L, 0)).isFalse();
    assertThat(Durations.isValid(42L, -42)).isFalse();
    assertThat(Durations.isValid(-42L, 42)).isFalse();
  }

  @Test
  public void testIsValid_true() {
    assertThat(Durations.isValid(0L, 42)).isTrue();
    assertThat(Durations.isValid(0L, -42)).isTrue();
    assertThat(Durations.isValid(42L, 0)).isTrue();
    assertThat(Durations.isValid(42L, 42)).isTrue();
    assertThat(Durations.isValid(-315576000000L, 0)).isTrue();
    assertThat(Durations.isValid(315576000000L, 0)).isTrue();
  }

  @Test
  public void testParse_outOfRange() throws ParseException {
    try {
      Durations.parse("316576000000.123456789123456789s");
      fail("expected ParseException");
    } catch (ParseException expected) {
      assertThat(expected).hasMessageThat().isEqualTo("Duration value is out of range.");
      assertThat(expected).hasCauseThat().isNotNull();
    }
  }

  @Test
  public void testDurationStringFormat() throws Exception {
    Timestamp start = Timestamps.parse("0001-01-01T00:00:00Z");
    Timestamp end = Timestamps.parse("9999-12-31T23:59:59.999999999Z");
    Duration duration = Timestamps.between(start, end);
    assertThat(Durations.toString(duration)).isEqualTo("315537897599.999999999s");
    duration = Timestamps.between(end, start);
    assertThat(Durations.toString(duration)).isEqualTo("-315537897599.999999999s");

    // Generated output should contain 3, 6, or 9 fractional digits.
    duration = Duration.newBuilder().setSeconds(1).build();
    assertThat(Durations.toString(duration)).isEqualTo("1s");
    duration = Duration.newBuilder().setNanos(10000000).build();
    assertThat(Durations.toString(duration)).isEqualTo("0.010s");
    duration = Duration.newBuilder().setNanos(10000).build();
    assertThat(Durations.toString(duration)).isEqualTo("0.000010s");
    duration = Duration.newBuilder().setNanos(10).build();
    assertThat(Durations.toString(duration)).isEqualTo("0.000000010s");

    // Parsing accepts an fractional digits as long as they fit into nano
    // precision.
    duration = Durations.parse("0.1s");
    assertThat(duration.getNanos()).isEqualTo(100000000);
    duration = Durations.parse("0.0001s");
    assertThat(duration.getNanos()).isEqualTo(100000);
    duration = Durations.parse("0.0000001s");
    assertThat(duration.getNanos()).isEqualTo(100);
    // Repeat using parseUnchecked().
    duration = Durations.parseUnchecked("0.1s");
    assertThat(duration.getNanos()).isEqualTo(100000000);
    duration = Durations.parseUnchecked("0.0001s");
    assertThat(duration.getNanos()).isEqualTo(100000);
    duration = Durations.parseUnchecked("0.0000001s");
    assertThat(duration.getNanos()).isEqualTo(100);

    // Duration must support range from -315,576,000,000s to +315576000000s
    // which includes negative values.
    duration = Durations.parse("315576000000.999999999s");
    assertThat(duration.getSeconds()).isEqualTo(315576000000L);
    assertThat(duration.getNanos()).isEqualTo(999999999);
    duration = Durations.parse("-315576000000.999999999s");
    assertThat(duration.getSeconds()).isEqualTo(-315576000000L);
    assertThat(duration.getNanos()).isEqualTo(-999999999);
    // Repeat using parseUnchecked().
    duration = Durations.parseUnchecked("315576000000.999999999s");
    assertThat(duration.getSeconds()).isEqualTo(315576000000L);
    assertThat(duration.getNanos()).isEqualTo(999999999);
    duration = Durations.parseUnchecked("-315576000000.999999999s");
    assertThat(duration.getSeconds()).isEqualTo(-315576000000L);
    assertThat(duration.getNanos()).isEqualTo(-999999999);
  }

  @Test
  public void testDurationInvalidFormat() {
    // Value too small.
    try {
      Durations.toString(
                Duration.newBuilder().setSeconds(Durations.DURATION_SECONDS_MIN - 1).build());
      fail();
    } catch (IllegalArgumentException expected) {
      assertThat(expected).hasMessageThat().isNotNull();
    }

    try {
      Durations.toString(
                Duration.newBuilder().setSeconds(Durations.DURATION_SECONDS_MAX + 1).build());
      fail();
    } catch (IllegalArgumentException expected) {
      assertThat(expected).hasMessageThat().isNotNull();
    }

    // Invalid nanos value.
    try {
      Durations.toString(Duration.newBuilder().setSeconds(1).setNanos(-1).build());
      fail();
    } catch (IllegalArgumentException expected) {
      assertThat(expected).hasMessageThat().isNotNull();
    }

    // Invalid seconds value.
    try {
      Durations.toString(Duration.newBuilder().setSeconds(-1).setNanos(1).build());
      fail();
    } catch (IllegalArgumentException expected) {
      assertThat(expected).hasMessageThat().isNotNull();
    }

    // Value too small.
    try {
      Durations.parse("-315576000001s");
      fail();
    } catch (ParseException expected) {
      assertThat(expected).hasMessageThat().isNotNull();
    }
    try {
      Durations.parseUnchecked("-315576000001s");
      fail();
    } catch (IllegalArgumentException expected) {
      assertThat(expected).hasMessageThat().isNotNull();
    }

    // Value too large.
    try {
      Durations.parse("315576000001s");
      fail();
    } catch (ParseException expected) {
      assertThat(expected).hasMessageThat().isNotNull();
    }
    try {
      Durations.parseUnchecked("315576000001s");
      fail();
    } catch (IllegalArgumentException expected) {
      assertThat(expected).hasMessageThat().isNotNull();
    }

    // Empty.
    try {
      Durations.parse("");
      fail();
    } catch (ParseException expected) {
      assertThat(expected).hasMessageThat().isNotNull();
    }
    try {
      Durations.parseUnchecked("");
      fail();
    } catch (IllegalArgumentException expected) {
      assertThat(expected).hasMessageThat().isNotNull();
    }

    // Missing "s".
    try {
      Durations.parse("0");
      fail();
    } catch (ParseException expected) {
      assertThat(expected).hasMessageThat().isNotNull();
    }
    try {
      Durations.parseUnchecked("0");
      fail();
    } catch (IllegalArgumentException expected) {
      assertThat(expected).hasMessageThat().isNotNull();
    }

    // Invalid trailing data.
    try {
      Durations.parse("0s0");
      fail();
    } catch (ParseException expected) {
      assertThat(expected).hasMessageThat().isNotNull();
    }
    try {
      Durations.parseUnchecked("0s0");
      fail();
    } catch (IllegalArgumentException expected) {
      assertThat(expected).hasMessageThat().isNotNull();
    }

    // Invalid prefix.
    try {
      Durations.parse("--1s");
      fail();
    } catch (ParseException expected) {
      assertThat(expected).hasMessageThat().isNotNull();
    }
    try {
      Durations.parseUnchecked("--1s");
      fail();
    } catch (IllegalArgumentException expected) {
      assertThat(expected).hasMessageThat().isNotNull();
    }
  }

  @Test
  public void testDurationConversion() throws Exception {
    Duration duration = Durations.parse("1.111111111s");
    assertThat(Durations.toNanos(duration)).isEqualTo(1111111111);
    assertThat(Durations.toMicros(duration)).isEqualTo(1111111);
    assertThat(Durations.toMillis(duration)).isEqualTo(1111);
    assertThat(Durations.toSeconds(duration)).isEqualTo(1);
    duration = Durations.fromNanos(1111111111);
    assertThat(Durations.toString(duration)).isEqualTo("1.111111111s");
    duration = Durations.fromMicros(1111111);
    assertThat(Durations.toString(duration)).isEqualTo("1.111111s");
    duration = Durations.fromMillis(1111);
    assertThat(Durations.toString(duration)).isEqualTo("1.111s");
    duration = Durations.fromSeconds(1);
    assertThat(Durations.toString(duration)).isEqualTo("1s");

    duration = Durations.parse("-1.111111111s");
    assertThat(Durations.toNanos(duration)).isEqualTo(-1111111111);
    assertThat(Durations.toMicros(duration)).isEqualTo(-1111111);
    assertThat(Durations.toMillis(duration)).isEqualTo(-1111);
    assertThat(Durations.toSeconds(duration)).isEqualTo(-1);
    duration = Durations.fromNanos(-1111111111);
    assertThat(Durations.toString(duration)).isEqualTo("-1.111111111s");
    duration = Durations.fromMicros(-1111111);
    assertThat(Durations.toString(duration)).isEqualTo("-1.111111s");
    duration = Durations.fromMillis(-1111);
    assertThat(Durations.toString(duration)).isEqualTo("-1.111s");
    duration = Durations.fromSeconds(-1);
    assertThat(Durations.toString(duration)).isEqualTo("-1s");
  }

  @Test
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

    duration = Durations.parse("-1.125s");
    assertThat(Durations.toString(duration)).isEqualTo("-1.125s");

    duration = Durations.add(duration, duration);
    assertThat(Durations.toString(duration)).isEqualTo("-2.250s");

    duration = Durations.subtract(duration, Durations.parse("-1s"));
    assertThat(Durations.toString(duration)).isEqualTo("-1.250s");
  }

  @Test
  public void testToString() {
    assertThat(Durations.toString(duration(1, 1))).isEqualTo("1.000000001s");
    assertThat(Durations.toString(duration(-1, -1))).isEqualTo("-1.000000001s");
    assertThat(Durations.toString(duration(1, 0))).isEqualTo("1s");
    assertThat(Durations.toString(duration(-1, 0))).isEqualTo("-1s");
    assertThat(Durations.toString(duration(0, 1))).isEqualTo("0.000000001s");
    assertThat(Durations.toString(duration(0, -1))).isEqualTo("-0.000000001s");
  }

  @Test
  public void testAdd() {
    assertThat(Durations.add(duration(1, 10), duration(1, 20))).isEqualTo(duration(2, 30));
    assertThat(Durations.add(duration(1, 999999999), duration(1, 2))).isEqualTo(duration(3, 1));
    assertThat(Durations.add(duration(1, 999999999), duration(1, 1))).isEqualTo(duration(3, 0));
    assertThat(Durations.add(duration(1, 999999999), duration(-2, -1))).isEqualTo(duration(0, -2));
    assertThat(Durations.add(duration(1, 999999999), duration(-2, 0))).isEqualTo(duration(0, -1));
    assertThat(Durations.add(duration(-1, -10), duration(-1, -20))).isEqualTo(duration(-2, -30));
    assertThat(Durations.add(duration(-1, -999999999), duration(-1, -2)))
        .isEqualTo(duration(-3, -1));
    assertThat(Durations.add(duration(-1, -999999999), duration(-1, -1)))
        .isEqualTo(duration(-3, 0));
    assertThat(Durations.add(duration(-1, -999999999), duration(2, 1))).isEqualTo(duration(0, 2));
    assertThat(Durations.add(duration(-1, -999999999), duration(2, 0))).isEqualTo(duration(0, 1));
  }

  @Test
  public void testSubtract() {
    assertThat(Durations.subtract(duration(3, 2), duration(1, 1))).isEqualTo(duration(2, 1));
    assertThat(Durations.subtract(duration(3, 10), duration(1, 10))).isEqualTo(duration(2, 0));
    assertThat(Durations.subtract(duration(3, 1), duration(1, 2)))
        .isEqualTo(duration(1, 999999999));
    assertThat(Durations.subtract(duration(3, 2), duration(4, 1)))
        .isEqualTo(duration(0, -999999999));
    assertThat(Durations.subtract(duration(1, 1), duration(3, 2))).isEqualTo(duration(-2, -1));
    assertThat(Durations.subtract(duration(1, 10), duration(3, 10))).isEqualTo(duration(-2, 0));
    assertThat(Durations.subtract(duration(1, 2), duration(3, 1)))
        .isEqualTo(duration(-1, -999999999));
    assertThat(Durations.subtract(duration(4, 1), duration(3, 2)))
        .isEqualTo(duration(0, 999999999));
  }

  @Test
  public void testComparator() {
    assertThat(Durations.comparator().compare(duration(3, 2), duration(3, 2))).isEqualTo(0);
    assertThat(Durations.comparator().compare(duration(0, 0), duration(0, 0))).isEqualTo(0);
    assertThat(Durations.comparator().compare(duration(3, 1), duration(1, 1))).isGreaterThan(0);
    assertThat(Durations.comparator().compare(duration(3, 2), duration(3, 1))).isGreaterThan(0);
    assertThat(Durations.comparator().compare(duration(1, 1), duration(3, 1))).isLessThan(0);
    assertThat(Durations.comparator().compare(duration(3, 1), duration(3, 2))).isLessThan(0);
    assertThat(Durations.comparator().compare(duration(-3, -1), duration(-1, -1))).isLessThan(0);
    assertThat(Durations.comparator().compare(duration(-3, -2), duration(-3, -1))).isLessThan(0);
    assertThat(Durations.comparator().compare(duration(-1, -1), duration(-3, -1))).isGreaterThan(0);
    assertThat(Durations.comparator().compare(duration(-3, -1), duration(-3, -2))).isGreaterThan(0);
    assertThat(Durations.comparator().compare(duration(-10, -1), duration(1, 1))).isLessThan(0);
    assertThat(Durations.comparator().compare(duration(0, -1), duration(0, 1))).isLessThan(0);
    assertThat(Durations.comparator().compare(duration(0x80000000L, 0), duration(0, 0)))
        .isGreaterThan(0);
    assertThat(Durations.comparator().compare(duration(0xFFFFFFFF00000000L, 0), duration(0, 0)))
        .isLessThan(0);

    Duration duration0 = duration(-50, -500);
    Duration duration1 = duration(-50, -400);
    Duration duration2 = duration(50, 500);
    Duration duration3 = duration(100, 20);
    Duration duration4 = duration(100, 50);
    Duration duration5 = duration(100, 150);
    Duration duration6 = duration(150, 40);

    List<Duration> durations =
        Lists.newArrayList(
            duration5, duration3, duration1, duration4, duration6, duration2, duration0, duration3);

    Collections.sort(durations, Durations.comparator());
    assertThat(durations)
        .containsExactly(
            duration0, duration1, duration2, duration3, duration3, duration4, duration5, duration6)
        .inOrder();
  }

  @Test
  public void testCompare() {
    assertThat(Durations.compare(duration(3, 2), duration(3, 2))).isEqualTo(0);
    assertThat(Durations.compare(duration(0, 0), duration(0, 0))).isEqualTo(0);
    assertThat(Durations.compare(duration(3, 1), duration(1, 1))).isGreaterThan(0);
    assertThat(Durations.compare(duration(3, 2), duration(3, 1))).isGreaterThan(0);
    assertThat(Durations.compare(duration(1, 1), duration(3, 1))).isLessThan(0);
    assertThat(Durations.compare(duration(3, 1), duration(3, 2))).isLessThan(0);
    assertThat(Durations.compare(duration(-3, -1), duration(-1, -1))).isLessThan(0);
    assertThat(Durations.compare(duration(-3, -2), duration(-3, -1))).isLessThan(0);
    assertThat(Durations.compare(duration(-1, -1), duration(-3, -1))).isGreaterThan(0);
    assertThat(Durations.compare(duration(-3, -1), duration(-3, -2))).isGreaterThan(0);
    assertThat(Durations.compare(duration(-10, -1), duration(1, 1))).isLessThan(0);
    assertThat(Durations.compare(duration(0, -1), duration(0, 1))).isLessThan(0);
    assertThat(Durations.compare(duration(0x80000000L, 0), duration(0, 0))).isGreaterThan(0);
    assertThat(Durations.compare(duration(0xFFFFFFFF00000000L, 0), duration(0, 0))).isLessThan(0);
  }

  @Test
  public void testOverflows() throws Exception {
    try {
      Durations.toNanos(duration(315576000000L, 999999999));
      fail("Expected an ArithmeticException to be thrown");
    } catch (ArithmeticException expected) {
    }

    try {
      Durations.add(Durations.MAX_VALUE, Durations.MAX_VALUE);
      fail("Expected an IllegalArgumentException to be thrown");
    } catch (IllegalArgumentException expected) {
    }
    try {
      Durations.subtract(Durations.MAX_VALUE, Durations.MIN_VALUE);
      fail("Expected an IllegalArgumentException to be thrown");
    } catch (IllegalArgumentException expected) {
    }
    try {
      Durations.subtract(Durations.MIN_VALUE, Durations.MAX_VALUE);
      fail("Expected an IllegalArgumentException to be thrown");
    } catch (IllegalArgumentException expected) {
    }

    try {
      Durations.toNanos(INVALID_MAX);
      fail("Expected an IllegalArgumentException to be thrown");
    } catch (IllegalArgumentException expected) {
    }
    try {
      Durations.toMicros(INVALID_MAX);
      fail("Expected an IllegalArgumentException to be thrown");
    } catch (IllegalArgumentException expected) {
    }
    try {
      Durations.toMillis(INVALID_MAX);
      fail("Expected an IllegalArgumentException to be thrown");
    } catch (IllegalArgumentException expected) {
    }
    try {
      Durations.toSeconds(INVALID_MAX);
      fail("Expected an IllegalArgumentException to be thrown");
    } catch (IllegalArgumentException expected) {
    }

    try {
      Durations.toNanos(INVALID_MIN);
      fail("Expected an IllegalArgumentException to be thrown");
    } catch (IllegalArgumentException expected) {
    }
    try {
      Durations.toMicros(INVALID_MIN);
      fail("Expected an IllegalArgumentException to be thrown");
    } catch (IllegalArgumentException expected) {
    }
    try {
      Durations.toMillis(INVALID_MIN);
      fail("Expected an IllegalArgumentException to be thrown");
    } catch (IllegalArgumentException expected) {
    }
    try {
      Durations.toSeconds(INVALID_MIN);
      fail("Expected an IllegalArgumentException to be thrown");
    } catch (IllegalArgumentException expected) {
    }

    assertThat(Durations.toString(Durations.fromNanos(Long.MAX_VALUE)))
        .isEqualTo("9223372036.854775807s");
    try {
      Durations.fromMicros(Long.MAX_VALUE);
      fail("Expected an IllegalArgumentException to be thrown");
    } catch (IllegalArgumentException expected) {
    }
    try {
      Durations.fromMillis(Long.MAX_VALUE);
      fail("Expected an IllegalArgumentException to be thrown");
    } catch (IllegalArgumentException expected) {
    }
    try {
      Durations.fromSeconds(Long.MAX_VALUE);
      fail("Expected an IllegalArgumentException to be thrown");
    } catch (IllegalArgumentException expected) {
    }

    assertThat(Durations.toString(Durations.fromNanos(Long.MIN_VALUE)))
        .isEqualTo("-9223372036.854775808s");
    try {
      Durations.fromMicros(Long.MIN_VALUE);
      fail("Expected an IllegalArgumentException to be thrown");
    } catch (IllegalArgumentException expected) {
    }
    try {
      Durations.fromMillis(Long.MIN_VALUE);
      fail("Expected an IllegalArgumentException to be thrown");
    } catch (IllegalArgumentException expected) {
    }
    try {
      Durations.fromSeconds(Long.MIN_VALUE);
      fail("Expected an IllegalArgumentException to be thrown");
    } catch (IllegalArgumentException expected) {
    }
  }

  static Duration duration(long seconds, int nanos) {
    return Durations.checkValid(
        Durations.checkValid(Duration.newBuilder().setSeconds(seconds).setNanos(nanos)));
  }
}
