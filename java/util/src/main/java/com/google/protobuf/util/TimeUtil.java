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

import java.math.BigInteger;
import java.text.ParseException;

/**
 * Utilities to help create/manipulate Timestamp/Duration
 *
 * @deprecated Use {@link Durations} and {@link Timestamps} instead.
 */
@Deprecated
public final class TimeUtil {
  // Timestamp for "0001-01-01T00:00:00Z"
  public static final long TIMESTAMP_SECONDS_MIN = -62135596800L;

  // Timestamp for "9999-12-31T23:59:59Z"
  public static final long TIMESTAMP_SECONDS_MAX = 253402300799L;
  public static final long DURATION_SECONDS_MIN = -315576000000L;
  public static final long DURATION_SECONDS_MAX = 315576000000L;

  private static final long NANOS_PER_SECOND = 1000000000;

  private TimeUtil() {}

  /**
   * Convert Timestamp to RFC 3339 date string format. The output will always
   * be Z-normalized and uses 3, 6 or 9 fractional digits as required to
   * represent the exact value. Note that Timestamp can only represent time
   * from 0001-01-01T00:00:00Z to 9999-12-31T23:59:59.999999999Z. See
   * https://www.ietf.org/rfc/rfc3339.txt
   *
   * <p>Example of generated format: "1972-01-01T10:00:20.021Z"
   *
   * @return The string representation of the given timestamp.
   * @throws IllegalArgumentException if the given timestamp is not in the
   *         valid range.
   * @deprecated Use {@link Timestamps#toString} instead.
   */
  @Deprecated
  public static String toString(Timestamp timestamp) {
    return Timestamps.toString(timestamp);
  }

  /**
   * Parse from RFC 3339 date string to Timestamp. This method accepts all
   * outputs of {@link #toString(Timestamp)} and it also accepts any fractional
   * digits (or none) and any offset as long as they fit into nano-seconds
   * precision.
   *
   * <p>Example of accepted format: "1972-01-01T10:00:20.021-05:00"
   *
   * @return A Timestamp parsed from the string.
   * @throws ParseException if parsing fails.
   * @deprecated Use {@link Timestamps#parse} instead.
   */
  @Deprecated
  public static Timestamp parseTimestamp(String value) throws ParseException {
    return Timestamps.parse(value);
  }

  /**
   * Convert Duration to string format. The string format will contains 3, 6,
   * or 9 fractional digits depending on the precision required to represent
   * the exact Duration value. For example: "1s", "1.010s", "1.000000100s",
   * "-3.100s" The range that can be represented by Duration is from
   * -315,576,000,000 to +315,576,000,000 inclusive (in seconds).
   *
   * @return The string representation of the given duration.
   * @throws IllegalArgumentException if the given duration is not in the valid
   *         range.
   * @deprecated Use {@link Durations#toString} instead.
   */
  @Deprecated
  public static String toString(Duration duration) {
    return Durations.toString(duration);
  }

  /**
   * Parse from a string to produce a duration.
   *
   * @return A Duration parsed from the string.
   * @throws ParseException if parsing fails.
   * @deprecated Use {@link Durations#parse} instead.
   */
  @Deprecated
  public static Duration parseDuration(String value) throws ParseException {
    return Durations.parse(value);
  }

  /**
   * Create a Timestamp from the number of milliseconds elapsed from the epoch.
   *
   * @deprecated Use {@link Timestamps#fromMillis} instead.
   */
  @Deprecated
  public static Timestamp createTimestampFromMillis(long milliseconds) {
    return Timestamps.fromMillis(milliseconds);
  }

  /**
   * Create a Duration from the number of milliseconds.
   *
   * @deprecated Use {@link Durations#fromMillis} instead.
   */
  @Deprecated
  public static Duration createDurationFromMillis(long milliseconds) {
    return Durations.fromMillis(milliseconds);
  }

  /**
   * Convert a Timestamp to the number of milliseconds elapsed from the epoch.
   *
   * <p>The result will be rounded down to the nearest millisecond. E.g., if the
   * timestamp represents "1969-12-31T23:59:59.999999999Z", it will be rounded
   * to -1 millisecond.
   *
   * @deprecated Use {@link Timestamps#toMillis} instead.
   */
  @Deprecated
  public static long toMillis(Timestamp timestamp) {
    return Timestamps.toMillis(timestamp);
  }

  /**
   * Convert a Duration to the number of milliseconds.The result will be
   * rounded towards 0 to the nearest millisecond. E.g., if the duration
   * represents -1 nanosecond, it will be rounded to 0.
   *
   * @deprecated Use {@link Durations#toMillis} instead.
   */
  @Deprecated
  public static long toMillis(Duration duration) {
    return Durations.toMillis(duration);
  }

  /**
   * Create a Timestamp from the number of microseconds elapsed from the epoch.
   *
   * @deprecated Use {@link Timestamps#fromMicros} instead.
   */
  @Deprecated
  public static Timestamp createTimestampFromMicros(long microseconds) {
    return Timestamps.fromMicros(microseconds);
  }

  /**
   * Create a Duration from the number of microseconds.
   *
   * @deprecated Use {@link Durations#fromMicros} instead.
   */
  @Deprecated
  public static Duration createDurationFromMicros(long microseconds) {
    return Durations.fromMicros(microseconds);
  }

  /**
   * Convert a Timestamp to the number of microseconds elapsed from the epoch.
   *
   * <p>The result will be rounded down to the nearest microsecond. E.g., if the
   * timestamp represents "1969-12-31T23:59:59.999999999Z", it will be rounded
   * to -1 millisecond.
   *
   * @deprecated Use {@link Timestamps#toMicros} instead.
   */
  @Deprecated
  public static long toMicros(Timestamp timestamp) {
    return Timestamps.toMicros(timestamp);
  }

  /**
   * Convert a Duration to the number of microseconds.The result will be
   * rounded towards 0 to the nearest microseconds. E.g., if the duration
   * represents -1 nanosecond, it will be rounded to 0.
   *
   * @deprecated Use {@link Durations#toMicros} instead.
   */
  @Deprecated
  public static long toMicros(Duration duration) {
    return Durations.toMicros(duration);
  }

  /**
   * Create a Timestamp from the number of nanoseconds elapsed from the epoch.
   *
   * @deprecated Use {@link Timestamps#fromNanos} instead.
   */
  @Deprecated
  public static Timestamp createTimestampFromNanos(long nanoseconds) {
    return Timestamps.fromNanos(nanoseconds);
  }

  /**
   * Create a Duration from the number of nanoseconds.
   *
   * @deprecated Use {@link Durations#fromNanos} instead.
   */
  @Deprecated
  public static Duration createDurationFromNanos(long nanoseconds) {
    return Durations.fromNanos(nanoseconds);
  }

  /**
   * Convert a Timestamp to the number of nanoseconds elapsed from the epoch.
   *
   * @deprecated Use {@link Timestamps#toNanos} instead.
   */
  @Deprecated
  public static long toNanos(Timestamp timestamp) {
    return Timestamps.toNanos(timestamp);
  }

  /**
   * Convert a Duration to the number of nanoseconds.
   *
   * @deprecated Use {@link Durations#toNanos} instead.
   */
  @Deprecated
  public static long toNanos(Duration duration) {
    return Durations.toNanos(duration);
  }

  /**
   * Get the current time.
   *
   * @deprecated Use {@code Timestamps.fromMillis(System.currentTimeMillis())} instead.
   */
  @Deprecated
  public static Timestamp getCurrentTime() {
    return Timestamps.fromMillis(System.currentTimeMillis());
  }

  /**
   * Get the epoch.
   *
   * @deprecated Use {@code Timestamps.fromMillis(0)} instead.
   */
  @Deprecated
  public static Timestamp getEpoch() {
    return Timestamp.getDefaultInstance();
  }

  /**
   * Calculate the difference between two timestamps.
   *
   * @deprecated Use {@link Timestamps#between} instead.
   */
  @Deprecated
  public static Duration distance(Timestamp from, Timestamp to) {
    return Timestamps.between(from, to);
  }

  /**
   * Add a duration to a timestamp.
   *
   * @deprecated Use {@link Timestamps#add} instead.
   */
  @Deprecated
  public static Timestamp add(Timestamp start, Duration length) {
    return Timestamps.add(start, length);
  }

  /**
   * Subtract a duration from a timestamp.
   *
   * @deprecated Use {@link Timestamps#subtract} instead.
   */
  @Deprecated
  public static Timestamp subtract(Timestamp start, Duration length) {
    return Timestamps.subtract(start, length);
  }

  /**
   * Add two durations.
   *
   * @deprecated Use {@link Durations#add} instead.
   */
  @Deprecated
  public static Duration add(Duration d1, Duration d2) {
    return Durations.add(d1, d2);
  }

  /**
   * Subtract a duration from another.
   *
   * @deprecated Use {@link Durations#subtract} instead.
   */
  @Deprecated
  public static Duration subtract(Duration d1, Duration d2) {
    return Durations.subtract(d1, d2);
  }

  // Multiplications and divisions.

  // TODO(kak): Delete this.
  @SuppressWarnings("DurationSecondsToDouble")
  public static Duration multiply(Duration duration, double times) {
    double result = duration.getSeconds() * times + duration.getNanos() * times / 1000000000.0;
    if (result < Long.MIN_VALUE || result > Long.MAX_VALUE) {
      throw new IllegalArgumentException("Result is out of valid range.");
    }
    long seconds = (long) result;
    int nanos = (int) ((result - seconds) * 1000000000);
    return normalizedDuration(seconds, nanos);
  }

  // TODO(kak): Delete this.
  public static Duration divide(Duration duration, double value) {
    return multiply(duration, 1.0 / value);
  }

  // TODO(kak): Delete this.
  public static Duration multiply(Duration duration, long times) {
    return createDurationFromBigInteger(toBigInteger(duration).multiply(toBigInteger(times)));
  }

  // TODO(kak): Delete this.
  public static Duration divide(Duration duration, long times) {
    return createDurationFromBigInteger(toBigInteger(duration).divide(toBigInteger(times)));
  }

  // TODO(kak): Delete this.
  public static long divide(Duration d1, Duration d2) {
    return toBigInteger(d1).divide(toBigInteger(d2)).longValue();
  }

  // TODO(kak): Delete this.
  public static Duration remainder(Duration d1, Duration d2) {
    return createDurationFromBigInteger(toBigInteger(d1).remainder(toBigInteger(d2)));
  }

  private static final BigInteger NANOS_PER_SECOND_BIG_INTEGER =
      new BigInteger(String.valueOf(NANOS_PER_SECOND));

  private static BigInteger toBigInteger(Duration duration) {
    return toBigInteger(duration.getSeconds())
        .multiply(NANOS_PER_SECOND_BIG_INTEGER)
        .add(toBigInteger(duration.getNanos()));
  }

  private static BigInteger toBigInteger(long value) {
    return new BigInteger(String.valueOf(value));
  }

  private static Duration createDurationFromBigInteger(BigInteger value) {
    long seconds = value.divide(new BigInteger(String.valueOf(NANOS_PER_SECOND))).longValue();
    int nanos = value.remainder(new BigInteger(String.valueOf(NANOS_PER_SECOND))).intValue();
    return normalizedDuration(seconds, nanos);
  }

  private static Duration normalizedDuration(long seconds, int nanos) {
    if (nanos <= -NANOS_PER_SECOND || nanos >= NANOS_PER_SECOND) {
      seconds += nanos / NANOS_PER_SECOND;
      nanos %= NANOS_PER_SECOND;
    }
    if (seconds > 0 && nanos < 0) {
      nanos += NANOS_PER_SECOND;
      seconds -= 1;
    }
    if (seconds < 0 && nanos > 0) {
      nanos -= NANOS_PER_SECOND;
      seconds += 1;
    }
    if (seconds < DURATION_SECONDS_MIN || seconds > DURATION_SECONDS_MAX) {
      throw new IllegalArgumentException("Duration is out of valid range.");
    }
    return Duration.newBuilder().setSeconds(seconds).setNanos(nanos).build();
  }
}
