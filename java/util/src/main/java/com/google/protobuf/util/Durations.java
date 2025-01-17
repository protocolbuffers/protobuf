// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

package com.google.protobuf.util;

import static com.google.common.base.Preconditions.checkArgument;
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

import com.google.common.annotations.GwtIncompatible;
import com.google.common.base.Strings;
import com.google.errorprone.annotations.CanIgnoreReturnValue;
import com.google.errorprone.annotations.CompileTimeConstant;
import com.google.j2objc.annotations.J2ObjCIncompatible;
import com.google.protobuf.Duration;
import java.io.Serializable;
import java.text.ParseException;
import java.util.Comparator;

/**
 * Utilities to help create/manipulate {@code protobuf/duration.proto}. All operations throw an
 * {@link IllegalArgumentException} if the input(s) are not {@linkplain #isValid(Duration) valid}.
 */
public final class Durations {
  static final long DURATION_SECONDS_MIN = -315576000000L;
  static final long DURATION_SECONDS_MAX = 315576000000L;

  private static final long SECONDS_PER_MINUTE = 60L;
  private static final long SECONDS_PER_HOUR = SECONDS_PER_MINUTE * 60;
  private static final long SECONDS_PER_DAY = SECONDS_PER_HOUR * 24;

  /** A constant holding the minimum valid {@link Duration}, approximately {@code -10,000} years. */
  public static final Duration MIN_VALUE =
      Duration.newBuilder().setSeconds(DURATION_SECONDS_MIN).setNanos(-999999999).build();

  /** A constant holding the maximum valid {@link Duration}, approximately {@code +10,000} years. */
  public static final Duration MAX_VALUE =
      Duration.newBuilder().setSeconds(DURATION_SECONDS_MAX).setNanos(999999999).build();

  /** A constant holding the duration of zero. */
  public static final Duration ZERO = Duration.newBuilder().setSeconds(0L).setNanos(0).build();

  private Durations() {}

  private static enum DurationComparator implements Comparator<Duration>, Serializable {
    INSTANCE;

    @Override
    public int compare(Duration d1, Duration d2) {
      checkValid(d1);
      checkValid(d2);
      int secDiff = Long.compare(d1.getSeconds(), d2.getSeconds());
      return (secDiff != 0) ? secDiff : Integer.compare(d1.getNanos(), d2.getNanos());
    }
  }

  /**
   * Returns a {@link Comparator} for {@link Duration}s which sorts in increasing chronological
   * order. Nulls and invalid {@link Duration}s are not allowed (see {@link #isValid}). The returned
   * comparator is serializable.
   */
  public static Comparator<Duration> comparator() {
    return DurationComparator.INSTANCE;
  }

  /**
   * Compares two durations. The value returned is identical to what would be returned by: {@code
   * Durations.comparator().compare(x, y)}.
   *
   * @return the value {@code 0} if {@code x == y}; a value less than {@code 0} if {@code x < y};
   *     and a value greater than {@code 0} if {@code x > y}
   */
  public static int compare(Duration x, Duration y) {
    return DurationComparator.INSTANCE.compare(x, y);
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
  @SuppressWarnings("GoodTime") // this is a legacy conversion API
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

  /** Returns whether the given {@link Duration} is negative or not. */
  public static boolean isNegative(Duration duration) {
    checkValid(duration);
    return (duration.getSeconds() == 0) ? duration.getNanos() < 0 : duration.getSeconds() < 0;
  }

  /** Returns whether the given {@link Duration} is positive or not. */
  public static boolean isPositive(Duration duration) {
    checkValid(duration);
    return !isNegative(duration) && !duration.equals(ZERO);
  }

  /**
   * Ensures that the given {@link Duration} is not negative.
   *
   * @throws IllegalArgumentException if {@code duration} is negative or invalid
   * @throws NullPointerException if {@code duration} is {@code null}
   */
  @CanIgnoreReturnValue
  @GwtIncompatible("Depends on String.format which is not supported in Xplat.")
  @J2ObjCIncompatible
  public static Duration checkNotNegative(Duration duration) {
    checkArgument(!isNegative(duration), "duration (%s) must not be negative", toString(duration));
    return duration;
  }

  /**
   * Ensures that the given {@link Duration} is positive.
   *
   * @throws IllegalArgumentException if {@code duration} is negative, {@code ZERO}, or invalid
   * @throws NullPointerException if {@code duration} is {@code null}
   */
  @CanIgnoreReturnValue
  @GwtIncompatible("Depends on String.format which is not supported in Xplat.")
  @J2ObjCIncompatible
  public static Duration checkPositive(Duration duration) {
    checkArgument(isPositive(duration), "duration (%s) must be positive", toString(duration));
    return duration;
  }

  /** Throws an {@link IllegalArgumentException} if the given {@link Duration} is not valid. */
  @CanIgnoreReturnValue
  public static Duration checkValid(Duration duration) {
    long seconds = duration.getSeconds();
    int nanos = duration.getNanos();
    if (!isValid(seconds, nanos)) {
      throw new IllegalArgumentException(
          Strings.lenientFormat(
              "Duration is not valid. See proto definition for valid values. "
                  + "Seconds (%s) must be in range [-315,576,000,000, +315,576,000,000]. "
                  + "Nanos (%s) must be in range [-999,999,999, +999,999,999]. "
                  + "Nanos must have the same sign as seconds",
              seconds, nanos));
    }
    return duration;
  }

  /**
   * Builds the given builder and throws an {@link IllegalArgumentException} if it is not valid. See
   * {@link #checkValid(Duration)}.
   *
   * @return A valid, built {@link Duration}.
   */
  public static Duration checkValid(Duration.Builder durationBuilder) {
    return checkValid(durationBuilder.build());
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
  @GwtIncompatible("Depends on String.format which is not supported in Xplat.")
  @J2ObjCIncompatible
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
   * Parse a string to produce a duration.
   *
   * @return a Duration parsed from the string
   * @throws ParseException if the string is not in the duration format
   */
  @GwtIncompatible("ParseException is not supported in Xplat")
  @J2ObjCIncompatible
  public static Duration parse(String value) throws ParseException {
    // Must end with "s".
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
    long seconds;
    try {
      seconds = Long.parseLong(secondValue);
    } catch (NumberFormatException e) {
      throw new ParseException("Invalid duration string: " + value, 0);
    }
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
      ParseException ex = new ParseException("Duration value is out of range.", 0);
      ex.initCause(e);
      throw ex;
    }
  }

  /**
   * Parses a string in RFC 3339 format into a {@link Duration}.
   *
   * <p>Identical to {@link #parse(String)}, but throws an {@link IllegalArgumentException} instead
   * of a {@link ParseException} if parsing fails.
   *
   * @return a {@link Duration} parsed from the string
   * @throws IllegalArgumentException if parsing fails
   */
  @GwtIncompatible("ParseException is not supported in Xplat")
  @J2ObjCIncompatible
  public static Duration parseUnchecked(@CompileTimeConstant String value) {
    try {
      return parse(value);
    } catch (ParseException e) {
      // While `java.time.format.DateTimeParseException` is a more accurate representation of the
      // failure, this library is currently not JDK8 ready because of Android dependencies.
      throw new IllegalArgumentException(e);
    }
  }

  // Static factories

  /** Create a Duration from the number of days. */
  @SuppressWarnings("GoodTime") // this is a legacy conversion API
  public static Duration fromDays(long days) {
    return Duration.newBuilder()
        .setSeconds(checkedMultiply(days, SECONDS_PER_DAY))
        .setNanos(0)
        .build();
  }

  /** Create a Duration from the number of hours. */
  @SuppressWarnings("GoodTime") // this is a legacy conversion API
  public static Duration fromHours(long hours) {
    return Duration.newBuilder()
        .setSeconds(checkedMultiply(hours, SECONDS_PER_HOUR))
        .setNanos(0)
        .build();
  }

  /** Create a Duration from the number of minutes. */
  @SuppressWarnings("GoodTime") // this is a legacy conversion API
  public static Duration fromMinutes(long minutes) {
    return Duration.newBuilder()
        .setSeconds(checkedMultiply(minutes, SECONDS_PER_MINUTE))
        .setNanos(0)
        .build();
  }

  /** Create a Duration from the number of seconds. */
  @SuppressWarnings("GoodTime") // this is a legacy conversion API
  public static Duration fromSeconds(long seconds) {
    return normalizedDuration(seconds, 0);
  }

  /** Create a Duration from the number of milliseconds. */
  @SuppressWarnings("GoodTime") // this is a legacy conversion API
  public static Duration fromMillis(long milliseconds) {
    return normalizedDuration(
        milliseconds / MILLIS_PER_SECOND,
        (int) (milliseconds % MILLIS_PER_SECOND * NANOS_PER_MILLISECOND));
  }

  /** Create a Duration from the number of microseconds. */
  @SuppressWarnings("GoodTime") // this is a legacy conversion API
  public static Duration fromMicros(long microseconds) {
    return normalizedDuration(
        microseconds / MICROS_PER_SECOND,
        (int) (microseconds % MICROS_PER_SECOND * NANOS_PER_MICROSECOND));
  }

  /** Create a Duration from the number of nanoseconds. */
  @SuppressWarnings("GoodTime") // this is a legacy conversion API
  public static Duration fromNanos(long nanoseconds) {
    return normalizedDuration(
        nanoseconds / NANOS_PER_SECOND, (int) (nanoseconds % NANOS_PER_SECOND));
  }

  // Conversion APIs

  /**
   * Convert a Duration to the number of days. The result will be rounded towards 0 to the nearest
   * day.
   */
  @SuppressWarnings("GoodTime") // this is a legacy conversion API
  public static long toDays(Duration duration) {
    return checkValid(duration).getSeconds() / SECONDS_PER_DAY;
  }

  /**
   * Convert a Duration to the number of hours. The result will be rounded towards 0 to the nearest
   * hour.
   */
  @SuppressWarnings("GoodTime") // this is a legacy conversion API
  public static long toHours(Duration duration) {
    return checkValid(duration).getSeconds() / SECONDS_PER_HOUR;
  }

  /**
   * Convert a Duration to the number of minutes. The result will be rounded towards 0 to the
   * nearest minute.
   */
  @SuppressWarnings("GoodTime") // this is a legacy conversion API
  public static long toMinutes(Duration duration) {
    return checkValid(duration).getSeconds() / SECONDS_PER_MINUTE;
  }

  /**
   * Convert a Duration to the number of seconds. The result will be rounded towards 0 to the
   * nearest second. E.g., if the duration represents -1 nanosecond, it will be rounded to 0.
   */
  @SuppressWarnings("GoodTime") // this is a legacy conversion API
  public static long toSeconds(Duration duration) {
    return checkValid(duration).getSeconds();
  }

  /**
   * Returns the number of seconds of the given duration as a {@code double}. This method should be
   * used to accommodate APIs that <b>only</b> accept durations as {@code double} values.
   *
   * <p>This conversion may lose precision.
   *
   * <p>If you need the number of seconds in this duration as a {@code long} (not a {@code double}),
   * simply use {@code duration.getSeconds()} or {@link #toSeconds} (which includes validation).
   */
  @SuppressWarnings({
    "DurationSecondsToDouble", // that's the whole point of this method
    "GoodTime" // this is a legacy conversion API
  })
  public static double toSecondsAsDouble(Duration duration) {
    checkValid(duration);
    return duration.getSeconds() + duration.getNanos() / 1e9;
  }

  /**
   * Convert a Duration to the number of milliseconds. The result will be rounded towards 0 to the
   * nearest millisecond. E.g., if the duration represents -1 nanosecond, it will be rounded to 0.
   */
  @SuppressWarnings("GoodTime") // this is a legacy conversion API
  public static long toMillis(Duration duration) {
    checkValid(duration);
    return checkedAdd(
        checkedMultiply(duration.getSeconds(), MILLIS_PER_SECOND),
        duration.getNanos() / NANOS_PER_MILLISECOND);
  }

  /**
   * Convert a Duration to the number of microseconds. The result will be rounded towards 0 to the
   * nearest microseconds. E.g., if the duration represents -1 nanosecond, it will be rounded to 0.
   */
  @SuppressWarnings("GoodTime") // this is a legacy conversion API
  public static long toMicros(Duration duration) {
    checkValid(duration);
    return checkedAdd(
        checkedMultiply(duration.getSeconds(), MICROS_PER_SECOND),
        duration.getNanos() / NANOS_PER_MICROSECOND);
  }

  /** Convert a Duration to the number of nanoseconds. */
  @SuppressWarnings("GoodTime") // this is a legacy conversion API
  public static long toNanos(Duration duration) {
    checkValid(duration);
    return checkedAdd(
        checkedMultiply(duration.getSeconds(), NANOS_PER_SECOND), duration.getNanos());
  }

  // Math operations

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
