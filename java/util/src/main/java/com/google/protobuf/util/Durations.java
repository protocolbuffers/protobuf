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

import static com.google.common.math.IntMath.checkedAdd;
import static com.google.common.math.IntMath.checkedSubtract;
import static com.google.common.math.LongMath.checkedAdd;
import static com.google.common.math.LongMath.checkedMultiply;
import static com.google.common.math.LongMath.checkedSubtract;
import static com.google.protobuf.util.Timestamps.MICROS_PER_SECOND;
import static com.google.protobuf.util.Timestamps.MILLIS_PER_SECOND;
import static com.google.protobuf.util.Timestamps.NANOS_PER_MICROSECOND;
import static com.google.protobuf.util.Timestamps.NANOS_PER_MILLISECOND;
import static com.google.protobuf.util.Timestamps.NANOS_PER_SECOND;

import com.google.protobuf.Duration;
import java.text.ParseException;
import java.util.Comparator;

/**
 * Utilities to help create/manipulate {@code protobuf/duration.proto}. All operations throw an
 * {@link IllegalArgumentException} if the input(s) are not {@linkplain #isValid(Duration) valid}.
 */
public final class Durations {
  static final long DURATION_SECONDS_MIN = -315576000000L;
  static final long DURATION_SECONDS_MAX = 315576000000L;

  /** A constant holding the minimum valid {@link Duration}, approximately {@code -10,000} years. */
  public static final Duration MIN_VALUE =
      Duration.newBuilder().setSeconds(DURATION_SECONDS_MIN).setNanos(-999999999).build();

  /** A constant holding the maximum valid {@link Duration}, approximately {@code +10,000} years. */
  public static final Duration MAX_VALUE =
      Duration.newBuilder().setSeconds(DURATION_SECONDS_MAX).setNanos(999999999).build();

  private Durations() {}

  private static final Comparator<Duration> COMPARATOR =
      new Comparator<Duration>() {
        @Override
        public int compare(Duration d1, Duration d2) {
          checkValid(d1);
          checkValid(d2);
          int secDiff = Long.compare(d1.getSeconds(), d2.getSeconds());
          return (secDiff != 0) ? secDiff : Integer.compare(d1.getNanos(), d2.getNanos());
        }
      };

  /**
   * Returns a {@link Comparator} for {@link Duration}s which sorts in increasing chronological
   * order. Nulls and invalid {@link Duration}s are not allowed (see {@link #isValid}).
   */
  public static Comparator<Duration> comparator() {
    return COMPARATOR;
  }

  /**
   * Compares two durations. The value returned is identical to what would be returned by:
   * {@code Durations.comparator().compare(x, y)}.
   *
   * @return the value {@code 0} if {@code x == y}; a value less than {@code 0} if {@code x < y};
   *     and a value greater than {@code 0} if {@code x > y}
   */
  public static int compare(Duration x, Duration y) {
    return COMPARATOR.compare(x, y);
  }

  /**
   * Returns true if the given {@link Duration} is valid. The {@code seconds} value must be in the
   * range [-315,576,000,000, +315,576,000,000]. The {@code nanos} value must be in the range
   * [-999,999,999, +999,999,999].
   *
   * <p><b>Note:</b> Durations less than one second are represented with a 0 {@code seconds} field
   * and a positive or negative {@code nanos} field. For durations of one second or more, a non-zero
   * value for the {@code nanos} field must be of the same sign as the {@code seconds} field.
   */
  public static boolean isValid(Duration duration) {
    return isValid(duration.getSeconds(), duration.getNanos());
  }

  /**
   * Returns true if the given number of seconds and nanos is a valid {@link Duration}. The {@code
   * seconds} value must be in the range [-315,576,000,000, +315,576,000,000]. The {@code nanos}
   * value must be in the range [-999,999,999, +999,999,999].
   *
   * <p><b>Note:</b> Durations less than one second are represented with a 0 {@code seconds} field
   * and a positive or negative {@code nanos} field. For durations of one second or more, a non-zero
   * value for the {@code nanos} field must be of the same sign as the {@code seconds} field.
   */
  public static boolean isValid(long seconds, int nanos) {
    if (seconds < DURATION_SECONDS_MIN || seconds > DURATION_SECONDS_MAX) {
      return false;
    }
    if (nanos < -999999999L || nanos >= NANOS_PER_SECOND) {
      return false;
    }
    if (seconds < 0 || nanos < 0) {
      if (seconds > 0 || nanos > 0) {
        return false;
      }
    }
    return true;
  }

  /** Throws an {@link IllegalArgumentException} if the given {@link Duration} is not valid. */
  public static Duration checkValid(Duration duration) {
    long seconds = duration.getSeconds();
    int nanos = duration.getNanos();
    if (!isValid(seconds, nanos)) {
        throw new IllegalArgumentException(String.format(
            "Duration is not valid. See proto definition for valid values. "
            + "Seconds (%s) must be in range [-315,576,000,000, +315,576,000,000]. "
            + "Nanos (%s) must be in range [-999,999,999, +999,999,999]. "
            + "Nanos must have the same sign as seconds", seconds, nanos));
    }
    return duration;
  }

  /**
   * Convert Duration to string format. The string format will contains 3, 6, or 9 fractional digits
   * depending on the precision required to represent the exact Duration value. For example: "1s",
   * "1.010s", "1.000000100s", "-3.100s" The range that can be represented by Duration is from
   * -315,576,000,000 to +315,576,000,000 inclusive (in seconds).
   *
   * @return The string representation of the given duration.
   * @throws IllegalArgumentException if the given duration is not in the valid range.
   */
  public static String toString(Duration duration) {
    checkValid(duration);

    long seconds = duration.getSeconds();
    int nanos = duration.getNanos();

    StringBuilder result = new StringBuilder();
    if (seconds < 0 || nanos < 0) {
      result.append("-");
      seconds = -seconds;
      nanos = -nanos;
    }
    result.append(seconds);
    if (nanos != 0) {
      result.append(".");
      result.append(Timestamps.formatNanos(nanos));
    }
    result.append("s");
    return result.toString();
  }

  /**
   * Parse from a string to produce a duration.
   *
   * @return A Duration parsed from the string.
   * @throws ParseException if parsing fails.
   */
  public static Duration parse(String value) throws ParseException {
    // Must ended with "s".
    if (value.isEmpty() || value.charAt(value.length() - 1) != 's') {
      throw new ParseException("Invalid duration string: " + value, 0);
    }
    boolean negative = false;
    if (value.charAt(0) == '-') {
      negative = true;
      value = value.substring(1);
    }
    String secondValue = value.substring(0, value.length() - 1);
    String nanoValue = "";
    int pointPosition = secondValue.indexOf('.');
    if (pointPosition != -1) {
      nanoValue = secondValue.substring(pointPosition + 1);
      secondValue = secondValue.substring(0, pointPosition);
    }
    long seconds = Long.parseLong(secondValue);
    int nanos = nanoValue.isEmpty() ? 0 : Timestamps.parseNanos(nanoValue);
    if (seconds < 0) {
      throw new ParseException("Invalid duration string: " + value, 0);
    }
    if (negative) {
      seconds = -seconds;
      nanos = -nanos;
    }
    try {
      return normalizedDuration(seconds, nanos);
    } catch (IllegalArgumentException e) {
      throw new ParseException("Duration value is out of range.", 0);
    }
  }

  /** Create a Duration from the number of seconds. */
  public static Duration fromSeconds(long seconds) {
    return normalizedDuration(seconds, 0);
  }

  /**
   * Convert a Duration to the number of seconds. The result will be rounded towards 0 to the
   * nearest second. E.g., if the duration represents -1 nanosecond, it will be rounded to 0.
   */
  public static long toSeconds(Duration duration) {
    return checkValid(duration).getSeconds();
  }

  /** Create a Duration from the number of milliseconds. */
  public static Duration fromMillis(long milliseconds) {
    return normalizedDuration(
        milliseconds / MILLIS_PER_SECOND,
        (int) (milliseconds % MILLIS_PER_SECOND * NANOS_PER_MILLISECOND));
  }

  /**
   * Convert a Duration to the number of milliseconds. The result will be rounded towards 0 to the
   * nearest millisecond. E.g., if the duration represents -1 nanosecond, it will be rounded to 0.
   */
  public static long toMillis(Duration duration) {
    checkValid(duration);
    return checkedAdd(
        checkedMultiply(duration.getSeconds(), MILLIS_PER_SECOND),
        duration.getNanos() / NANOS_PER_MILLISECOND);
  }

  /** Create a Duration from the number of microseconds. */
  public static Duration fromMicros(long microseconds) {
    return normalizedDuration(
        microseconds / MICROS_PER_SECOND,
        (int) (microseconds % MICROS_PER_SECOND * NANOS_PER_MICROSECOND));
  }

  /**
   * Convert a Duration to the number of microseconds. The result will be rounded towards 0 to the
   * nearest microseconds. E.g., if the duration represents -1 nanosecond, it will be rounded to 0.
   */
  public static long toMicros(Duration duration) {
    checkValid(duration);
    return checkedAdd(
        checkedMultiply(duration.getSeconds(), MICROS_PER_SECOND),
        duration.getNanos() / NANOS_PER_MICROSECOND);
  }

  /** Create a Duration from the number of nanoseconds. */
  public static Duration fromNanos(long nanoseconds) {
    return normalizedDuration(
        nanoseconds / NANOS_PER_SECOND, (int) (nanoseconds % NANOS_PER_SECOND));
  }

  /** Convert a Duration to the number of nanoseconds. */
  public static long toNanos(Duration duration) {
    checkValid(duration);
    return checkedAdd(
        checkedMultiply(duration.getSeconds(), NANOS_PER_SECOND), duration.getNanos());
  }

  /** Add two durations. */
  public static Duration add(Duration d1, Duration d2) {
    checkValid(d1);
    checkValid(d2);
    return normalizedDuration(
        checkedAdd(d1.getSeconds(), d2.getSeconds()), checkedAdd(d1.getNanos(), d2.getNanos()));
  }

  /** Subtract a duration from another. */
  public static Duration subtract(Duration d1, Duration d2) {
    checkValid(d1);
    checkValid(d2);
    return normalizedDuration(
        checkedSubtract(d1.getSeconds(), d2.getSeconds()),
        checkedSubtract(d1.getNanos(), d2.getNanos()));
  }

  static Duration normalizedDuration(long seconds, int nanos) {
    if (nanos <= -NANOS_PER_SECOND || nanos >= NANOS_PER_SECOND) {
      seconds = checkedAdd(seconds, nanos / NANOS_PER_SECOND);
      nanos %= NANOS_PER_SECOND;
    }
    if (seconds > 0 && nanos < 0) {
      nanos += NANOS_PER_SECOND; // no overflow since nanos is negative (and we're adding)
      seconds--; // no overflow since seconds is positive (and we're decrementing)
    }
    if (seconds < 0 && nanos > 0) {
      nanos -= NANOS_PER_SECOND; // no overflow since nanos is positive (and we're subtracting)
      seconds++; // no overflow since seconds is negative (and we're incrementing)
    }
    Duration duration = Duration.newBuilder().setSeconds(seconds).setNanos(nanos).build();
    return checkValid(duration);
  }
}
