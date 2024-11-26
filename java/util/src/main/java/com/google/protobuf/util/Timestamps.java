// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

package com.google.protobuf.util;

import static com.google.common.math.IntMath.checkedAdd;
import static com.google.common.math.IntMath.checkedSubtract;
import static com.google.common.math.LongMath.checkedAdd;
import static com.google.common.math.LongMath.checkedMultiply;
import static com.google.common.math.LongMath.checkedSubtract;

import com.google.common.annotations.GwtIncompatible;
import com.google.common.base.Strings;
import com.google.errorprone.annotations.CanIgnoreReturnValue;
import com.google.errorprone.annotations.CompileTimeConstant;
import com.google.j2objc.annotations.J2ObjCIncompatible;
import com.google.protobuf.Duration;
import com.google.protobuf.Timestamp;
import java.io.Serializable;
import java.lang.reflect.Method;
import java.text.ParseException;
import java.text.SimpleDateFormat;
import java.util.Comparator;
import java.util.Date;
import java.util.GregorianCalendar;
import java.util.Locale;
import java.util.TimeZone;
import javax.annotation.Nullable;

/**
 * Utilities to help create/manipulate {@code protobuf/timestamp.proto}. All operations throw an
 * {@link IllegalArgumentException} if the input(s) are not {@linkplain #isValid(Timestamp) valid}.
 */
public final class Timestamps {

  // Timestamp for "0001-01-01T00:00:00Z"
  static final long TIMESTAMP_SECONDS_MIN = -62135596800L;

  // Timestamp for "9999-12-31T23:59:59Z"
  static final long TIMESTAMP_SECONDS_MAX = 253402300799L;

  static final int NANOS_PER_SECOND = 1000000000;
  static final int NANOS_PER_MILLISECOND = 1000000;
  static final int NANOS_PER_MICROSECOND = 1000;
  static final int MILLIS_PER_SECOND = 1000;
  static final int MICROS_PER_SECOND = 1000000;

  /** A constant holding the minimum valid {@link Timestamp}, {@code 0001-01-01T00:00:00Z}. */
  public static final Timestamp MIN_VALUE =
      Timestamp.newBuilder().setSeconds(TIMESTAMP_SECONDS_MIN).setNanos(0).build();

  /**
   * A constant holding the maximum valid {@link Timestamp}, {@code 9999-12-31T23:59:59.999999999Z}.
   */
  public static final Timestamp MAX_VALUE =
      Timestamp.newBuilder().setSeconds(TIMESTAMP_SECONDS_MAX).setNanos(999999999).build();

  /**
   * A constant holding the {@link Timestamp} of epoch time, {@code 1970-01-01T00:00:00.000000000Z}.
   */
  public static final Timestamp EPOCH = Timestamp.newBuilder().setSeconds(0).setNanos(0).build();

  @GwtIncompatible("Date formatting is not supported in non JVM java.time")
  @J2ObjCIncompatible
  private static final ThreadLocal<SimpleDateFormat> timestampFormat =
      new ThreadLocal<SimpleDateFormat>() {
        @Override
        protected SimpleDateFormat initialValue() {
          return createTimestampFormat();
        }
      };

  @GwtIncompatible("Date formatting is not supported in non JVM java.time")
  @J2ObjCIncompatible
  private static SimpleDateFormat createTimestampFormat() {
    SimpleDateFormat sdf = new SimpleDateFormat("yyyy-MM-dd'T'HH:mm:ss", Locale.ENGLISH);
    GregorianCalendar calendar = new GregorianCalendar(TimeZone.getTimeZone("UTC"));
    // We use Proleptic Gregorian Calendar (i.e., Gregorian calendar extends
    // backwards to year one) for timestamp formatting.
    calendar.setGregorianChange(new Date(Long.MIN_VALUE));
    sdf.setCalendar(calendar);
    return sdf;
  }

  private Timestamps() {}

  private static enum TimestampComparator implements Comparator<Timestamp>, Serializable {
    INSTANCE;

    @Override
    public int compare(Timestamp t1, Timestamp t2) {
      checkValid(t1);
      checkValid(t2);
      int secDiff = Long.compare(t1.getSeconds(), t2.getSeconds());
      return (secDiff != 0) ? secDiff : Integer.compare(t1.getNanos(), t2.getNanos());
    }
  }

  /**
   * Returns a {@link Comparator} for {@link Timestamp Timestamps} which sorts in increasing
   * chronological order. Nulls and invalid {@link Timestamp Timestamps} are not allowed (see {@link
   * #isValid}). The returned comparator is serializable.
   */
  public static Comparator<Timestamp> comparator() {
    return TimestampComparator.INSTANCE;
  }

  /**
   * Compares two timestamps. The value returned is identical to what would be returned by: {@code
   * Timestamps.comparator().compare(x, y)}.
   *
   * @return the value {@code 0} if {@code x == y}; a value less than {@code 0} if {@code x < y};
   *     and a value greater than {@code 0} if {@code x > y}
   */
  public static int compare(Timestamp x, Timestamp y) {
    return TimestampComparator.INSTANCE.compare(x, y);
  }

  /**
   * Returns true if the given {@link Timestamp} is valid. The {@code seconds} value must be in the
   * range [-62,135,596,800, +253,402,300,799] (i.e., between 0001-01-01T00:00:00Z and
   * 9999-12-31T23:59:59Z). The {@code nanos} value must be in the range [0, +999,999,999].
   *
   * <p><b>Note:</b> Negative second values with fractional seconds must still have non-negative
   * nanos values that count forward in time.
   */
  public static boolean isValid(Timestamp timestamp) {
    return isValid(timestamp.getSeconds(), timestamp.getNanos());
  }

  /**
   * Returns true if the given number of seconds and nanos is a valid {@link Timestamp}. The {@code
   * seconds} value must be in the range [-62,135,596,800, +253,402,300,799] (i.e., between
   * 0001-01-01T00:00:00Z and 9999-12-31T23:59:59Z). The {@code nanos} value must be in the range
   * [0, +999,999,999].
   *
   * <p><b>Note:</b> Negative second values with fractional seconds must still have non-negative
   * nanos values that count forward in time.
   */
  @SuppressWarnings("GoodTime") // this is a legacy conversion API
  public static boolean isValid(long seconds, int nanos) {
    if (!isValidSeconds(seconds)) {
      return false;
    }
    if (nanos < 0 || nanos >= NANOS_PER_SECOND) {
      return false;
    }
    return true;
  }

  /**
   * Returns true if the given number of seconds is valid, if combined with a valid number of nanos.
   * The {@code seconds} value must be in the range [-62,135,596,800, +253,402,300,799] (i.e.,
   * between 0001-01-01T00:00:00Z and 9999-12-31T23:59:59Z).
   */
  @SuppressWarnings("GoodTime") // this is a legacy conversion API
  private static boolean isValidSeconds(long seconds) {
    return seconds >= TIMESTAMP_SECONDS_MIN && seconds <= TIMESTAMP_SECONDS_MAX;
  }

  /** Throws an {@link IllegalArgumentException} if the given {@link Timestamp} is not valid. */
  @CanIgnoreReturnValue
  public static Timestamp checkValid(Timestamp timestamp) {
    long seconds = timestamp.getSeconds();
    int nanos = timestamp.getNanos();
    if (!isValid(seconds, nanos)) {
      throw new IllegalArgumentException(
          Strings.lenientFormat(
              "Timestamp is not valid. See proto definition for valid values. "
                  + "Seconds (%s) must be in range [-62,135,596,800, +253,402,300,799]. "
                  + "Nanos (%s) must be in range [0, +999,999,999].",
              seconds, nanos));
    }
    return timestamp;
  }

  /**
   * Builds the given builder and throws an {@link IllegalArgumentException} if it is not valid. See
   * {@link #checkValid(Timestamp)}.
   *
   * @return A valid, built {@link Timestamp}.
   */
  public static Timestamp checkValid(Timestamp.Builder timestampBuilder) {
    return checkValid(timestampBuilder.build());
  }

  /**
   * Convert Timestamp to RFC 3339 date string format. The output will always be Z-normalized and
   * uses 0, 3, 6 or 9 fractional digits as required to represent the exact value. Note that
   * Timestamp can only represent time from 0001-01-01T00:00:00Z to 9999-12-31T23:59:59.999999999Z.
   * See https://www.ietf.org/rfc/rfc3339.txt
   *
   * <p>Example of generated format: "1972-01-01T10:00:20.021Z"
   *
   * @return The string representation of the given timestamp.
   * @throws IllegalArgumentException if the given timestamp is not in the valid range.
   */
  @GwtIncompatible("Depends on String.format which is not supported in Xplat.")
  @J2ObjCIncompatible
  public static String toString(Timestamp timestamp) {
    checkValid(timestamp);

    long seconds = timestamp.getSeconds();
    int nanos = timestamp.getNanos();

    StringBuilder result = new StringBuilder();
    // Format the seconds part.
    Date date = new Date(seconds * MILLIS_PER_SECOND);
    result.append(timestampFormat.get().format(date));
    // Format the nanos part.
    if (nanos != 0) {
      result.append(".");
      result.append(formatNanos(nanos));
    }
    result.append("Z");
    return result.toString();
  }

  /**
   * Parse from RFC 3339 date string to Timestamp. This method accepts all outputs of {@link
   * #toString(Timestamp)} and it also accepts any fractional digits (or none) and any offset as
   * long as they fit into nano-seconds precision.
   *
   * <p>Example of accepted format: "1972-01-01T10:00:20.021-05:00"
   *
   * @return a Timestamp parsed from the string
   * @throws ParseException if parsing fails
   */
  @GwtIncompatible("ParseException is not supported in Xplat")
  @J2ObjCIncompatible
  public static Timestamp parse(String value) throws ParseException {
    int dayOffset = value.indexOf('T');
    if (dayOffset == -1) {
      throw new ParseException("Failed to parse timestamp: invalid timestamp \"" + value + "\"", 0);
    }
    int timezoneOffsetPosition = value.indexOf('Z', dayOffset);
    if (timezoneOffsetPosition == -1) {
      timezoneOffsetPosition = value.indexOf('+', dayOffset);
    }
    if (timezoneOffsetPosition == -1) {
      timezoneOffsetPosition = value.indexOf('-', dayOffset);
    }
    if (timezoneOffsetPosition == -1) {
      throw new ParseException("Failed to parse timestamp: missing valid timezone offset.", 0);
    }
    // Parse seconds and nanos.
    String timeValue = value.substring(0, timezoneOffsetPosition);
    String secondValue = timeValue;
    String nanoValue = "";
    int pointPosition = timeValue.indexOf('.');
    if (pointPosition != -1) {
      secondValue = timeValue.substring(0, pointPosition);
      nanoValue = timeValue.substring(pointPosition + 1);
    }
    Date date = timestampFormat.get().parse(secondValue);
    long seconds = date.getTime() / MILLIS_PER_SECOND;
    int nanos = nanoValue.isEmpty() ? 0 : parseNanos(nanoValue);
    // Parse timezone offsets.
    if (value.charAt(timezoneOffsetPosition) == 'Z') {
      if (value.length() != timezoneOffsetPosition + 1) {
        throw new ParseException(
            "Failed to parse timestamp: invalid trailing data \""
                + value.substring(timezoneOffsetPosition)
                + "\"",
            0);
      }
    } else {
      String offsetValue = value.substring(timezoneOffsetPosition + 1);
      long offset = parseTimezoneOffset(offsetValue);
      if (value.charAt(timezoneOffsetPosition) == '+') {
        seconds -= offset;
      } else {
        seconds += offset;
      }
    }
    try {
      return normalizedTimestamp(seconds, nanos);
    } catch (IllegalArgumentException e) {
      ParseException ex =
          new ParseException(
              "Failed to parse timestamp " + value + " Timestamp is out of range.", 0);
      ex.initCause(e);
      throw ex;
    }
  }

  /**
   * Parses a string in RFC 3339 format into a {@link Timestamp}.
   *
   * <p>Identical to {@link #parse(String)}, but throws an {@link IllegalArgumentException} instead
   * of a {@link ParseException} if parsing fails.
   *
   * @return a {@link Timestamp} parsed from the string
   * @throws IllegalArgumentException if parsing fails
   */
  @GwtIncompatible("Depends on String.format which is not supported in Xplat.")
  @J2ObjCIncompatible
  public static Timestamp parseUnchecked(@CompileTimeConstant String value) {
    try {
      return parse(value);
    } catch (ParseException e) {
      // While `java.time.format.DateTimeParseException` is a more accurate representation of the
      // failure, this library is currently not JDK8 ready because of Android dependencies.
      throw new IllegalArgumentException(e);
    }
  }

  // the following 3 constants contain references to java.time.Instant methods (if that class is
  // available at runtime); otherwise, they are null.
  @GwtIncompatible("Uses reflection to access methods of java.time.Instant")
  @J2ObjCIncompatible
  @Nullable
  private static final Method INSTANT_NOW = instantMethod("now");

  @GwtIncompatible("Uses reflection to access methods of java.time.Instant")
  @J2ObjCIncompatible
  @Nullable
  private static final Method INSTANT_GET_EPOCH_SECOND = instantMethod("getEpochSecond");

  @GwtIncompatible("Uses reflection to access methods of java.time.Instant")
  @J2ObjCIncompatible
  @Nullable
  private static final Method INSTANT_GET_NANO = instantMethod("getNano");

  @GwtIncompatible("Uses reflection to access methods of java.time.Instant")
  @J2ObjCIncompatible
  @Nullable
  private static Method instantMethod(String methodName) {
    try {
      return Class.forName("java.time.Instant").getMethod(methodName);
    } catch (Exception e) {
      return null;
    }
  }

  /**
   * Create a {@link Timestamp} using the best-available (in terms of precision) system clock.
   *
   * <p><b>Note:</b> that while this API is convenient, it may harm the testability of your code, as
   * you're unable to mock the current time. Instead, you may want to consider injecting a clock
   * instance to read the current time.
   */
  @GwtIncompatible("Uses reflection to access methods of java.time.Instant")
  @J2ObjCIncompatible
  public static Timestamp now() {
    if (INSTANT_NOW != null) {
      try {
        Object now = INSTANT_NOW.invoke(null);
        long epochSecond = (long) INSTANT_GET_EPOCH_SECOND.invoke(now);
        int nanoAdjustment = (int) INSTANT_GET_NANO.invoke(now);
        return normalizedTimestamp(epochSecond, nanoAdjustment);
      } catch (Throwable fallThrough) {
        throw new AssertionError(fallThrough);
      }
    }
    // otherwise, fall back on millisecond precision
    return fromMillis(System.currentTimeMillis());
  }

  /** Create a Timestamp from the number of seconds elapsed from the epoch. */
  @SuppressWarnings("GoodTime") // this is a legacy conversion API
  public static Timestamp fromSeconds(long seconds) {
    return normalizedTimestamp(seconds, 0);
  }

  /**
   * Convert a Timestamp to the number of seconds elapsed from the epoch.
   *
   * <p>The result will be rounded down to the nearest second. E.g., if the timestamp represents
   * "1969-12-31T23:59:59.999999999Z", it will be rounded to -1 second.
   */
  @SuppressWarnings("GoodTime") // this is a legacy conversion API
  public static long toSeconds(Timestamp timestamp) {
    return checkValid(timestamp).getSeconds();
  }

  /** Create a Timestamp from the number of milliseconds elapsed from the epoch. */
  @SuppressWarnings("GoodTime") // this is a legacy conversion API
  public static Timestamp fromMillis(long milliseconds) {
    return normalizedTimestamp(
        milliseconds / MILLIS_PER_SECOND,
        (int) (milliseconds % MILLIS_PER_SECOND * NANOS_PER_MILLISECOND));
  }

  /**
   * Create a Timestamp from a {@link Date}. If the {@link Date} is a {@link java.sql.Timestamp},
   * full nanonsecond precision is retained.
   *
   * @throws IllegalArgumentException if the year is before 1 CE or after 9999 CE
   */
  @SuppressWarnings("GoodTime") // this is a legacy conversion API
  @J2ObjCIncompatible
  public static Timestamp fromDate(Date date) {
    if (date instanceof java.sql.Timestamp) {
      java.sql.Timestamp sqlTimestamp = (java.sql.Timestamp) date;
      long time = sqlTimestamp.getTime();
      long integralSeconds =
          (time < 0 && time % 1000 != 0)
              ? time / 1000L - 1
              : time / 1000L; // truncate the fractional seconds
      return Timestamp.newBuilder()
          .setSeconds(integralSeconds)
          .setNanos(sqlTimestamp.getNanos())
          .build();
    } else {
      return fromMillis(date.getTime());
    }
  }

  /**
   * Convert a Timestamp to the number of milliseconds elapsed from the epoch.
   *
   * <p>The result will be rounded down to the nearest millisecond. For instance, if the timestamp
   * represents "1969-12-31T23:59:59.999999999Z", it will be rounded to -1 millisecond.
   */
  @SuppressWarnings("GoodTime") // this is a legacy conversion API
  public static long toMillis(Timestamp timestamp) {
    checkValid(timestamp);
    return checkedAdd(
        checkedMultiply(timestamp.getSeconds(), MILLIS_PER_SECOND),
        timestamp.getNanos() / NANOS_PER_MILLISECOND);
  }

  /** Create a Timestamp from the number of microseconds elapsed from the epoch. */
  @SuppressWarnings("GoodTime") // this is a legacy conversion API
  public static Timestamp fromMicros(long microseconds) {
    return normalizedTimestamp(
        microseconds / MICROS_PER_SECOND,
        (int) (microseconds % MICROS_PER_SECOND * NANOS_PER_MICROSECOND));
  }

  /**
   * Convert a Timestamp to the number of microseconds elapsed from the epoch.
   *
   * <p>The result will be rounded down to the nearest microsecond. E.g., if the timestamp
   * represents "1969-12-31T23:59:59.999999999Z", it will be rounded to -1 microsecond.
   */
  @SuppressWarnings("GoodTime") // this is a legacy conversion API
  public static long toMicros(Timestamp timestamp) {
    checkValid(timestamp);
    return checkedAdd(
        checkedMultiply(timestamp.getSeconds(), MICROS_PER_SECOND),
        timestamp.getNanos() / NANOS_PER_MICROSECOND);
  }

  /** Create a Timestamp from the number of nanoseconds elapsed from the epoch. */
  @SuppressWarnings("GoodTime") // this is a legacy conversion API
  public static Timestamp fromNanos(long nanoseconds) {
    return normalizedTimestamp(
        nanoseconds / NANOS_PER_SECOND, (int) (nanoseconds % NANOS_PER_SECOND));
  }

  /** Convert a Timestamp to the number of nanoseconds elapsed from the epoch. */
  @SuppressWarnings("GoodTime") // this is a legacy conversion API
  public static long toNanos(Timestamp timestamp) {
    checkValid(timestamp);
    return checkedAdd(
        checkedMultiply(timestamp.getSeconds(), NANOS_PER_SECOND), timestamp.getNanos());
  }

  /** Calculate the difference between two timestamps. */
  public static Duration between(Timestamp from, Timestamp to) {
    checkValid(from);
    checkValid(to);
    return Durations.normalizedDuration(
        checkedSubtract(to.getSeconds(), from.getSeconds()),
        checkedSubtract(to.getNanos(), from.getNanos()));
  }

  /** Add a duration to a timestamp. */
  public static Timestamp add(Timestamp start, Duration length) {
    checkValid(start);
    Durations.checkValid(length);
    return normalizedTimestamp(
        checkedAdd(start.getSeconds(), length.getSeconds()),
        checkedAdd(start.getNanos(), length.getNanos()));
  }

  /** Subtract a duration from a timestamp. */
  public static Timestamp subtract(Timestamp start, Duration length) {
    checkValid(start);
    Durations.checkValid(length);
    return normalizedTimestamp(
        checkedSubtract(start.getSeconds(), length.getSeconds()),
        checkedSubtract(start.getNanos(), length.getNanos()));
  }

  static Timestamp normalizedTimestamp(long seconds, int nanos) {
    // This only checks seconds, because nanos can intentionally overflow to increment the seconds
    // when normalized.
    if (!isValidSeconds(seconds)) {
      throw new IllegalArgumentException(
          Strings.lenientFormat(
              "Timestamp is not valid. Input seconds is too large. "
                  + "Seconds (%s) must be in range [-62,135,596,800, +253,402,300,799]. ",
              seconds));
    }
    if (nanos <= -NANOS_PER_SECOND || nanos >= NANOS_PER_SECOND) {
      seconds = checkedAdd(seconds, nanos / NANOS_PER_SECOND);
      nanos = (int) (nanos % NANOS_PER_SECOND);
    }
    if (nanos < 0) {
      nanos =
          (int)
              (nanos + NANOS_PER_SECOND); // no overflow since nanos is negative (and we're adding)
      seconds = checkedSubtract(seconds, 1);
    }
    Timestamp timestamp = Timestamp.newBuilder().setSeconds(seconds).setNanos(nanos).build();
    return checkValid(timestamp);
  }

  @GwtIncompatible("Depends on String.format which is not supported in Xplat.")
  @J2ObjCIncompatible
  private static long parseTimezoneOffset(String value) throws ParseException {
    int pos = value.indexOf(':');
    if (pos == -1) {
      throw new ParseException("Invalid offset value: " + value, 0);
    }
    String hours = value.substring(0, pos);
    String minutes = value.substring(pos + 1);
    try {
      return (Long.parseLong(hours) * 60 + Long.parseLong(minutes)) * 60;
    } catch (NumberFormatException e) {
      ParseException ex = new ParseException("Invalid offset value: " + value, 0);
      ex.initCause(e);
      throw ex;
    }
  }

  @GwtIncompatible("Depends on String.format which is not supported in Xplat.")
  @J2ObjCIncompatible
  static int parseNanos(String value) throws ParseException {
    int result = 0;
    for (int i = 0; i < 9; ++i) {
      result = result * 10;
      if (i < value.length()) {
        if (value.charAt(i) < '0' || value.charAt(i) > '9') {
          throw new ParseException("Invalid nanoseconds.", 0);
        }
        result += value.charAt(i) - '0';
      }
    }
    return result;
  }

  /** Format the nano part of a timestamp or a duration. */
  @GwtIncompatible("Depends on String.format which is not supported in Xplat.")
  @J2ObjCIncompatible
  static String formatNanos(int nanos) {
    // Determine whether to use 3, 6, or 9 digits for the nano part.
    if (nanos % NANOS_PER_MILLISECOND == 0) {
      return String.format(Locale.ENGLISH, "%1$03d", nanos / NANOS_PER_MILLISECOND);
    } else if (nanos % NANOS_PER_MICROSECOND == 0) {
      return String.format(Locale.ENGLISH, "%1$06d", nanos / NANOS_PER_MICROSECOND);
    } else {
      return String.format(Locale.ENGLISH, "%1$09d", nanos);
    }
  }
}
