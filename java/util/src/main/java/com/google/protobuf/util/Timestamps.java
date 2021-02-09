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

import com.google.errorprone.annotations.CanIgnoreReturnValue;
import com.google.protobuf.Duration;
import com.google.protobuf.Timestamp;
import java.text.ParseException;
import java.text.SimpleDateFormat;
import java.util.Comparator;
import java.util.Date;
import java.util.GregorianCalendar;
import java.util.Locale;
import java.util.TimeZone;

/**
 * Utilities to help create/manipulate {@code protobuf/timestamp.proto}. All operations throw an
 * {@link IllegalArgumentException} if the input(s) are not {@linkplain #isValid(Timestamp) valid}.
 */
public final class Timestamps {

  // Timestamp for "0001-01-01T00:00:00Z"
  static final long TIMESTAMP_SECONDS_MIN = -62135596800L;

  // Timestamp for "9999-12-31T23:59:59Z"
  static final long TIMESTAMP_SECONDS_MAX = 253402300799L;

  static final long NANOS_PER_SECOND = 1000000000;
  static final long NANOS_PER_MILLISECOND = 1000000;
  static final long NANOS_PER_MICROSECOND = 1000;
  static final long MILLIS_PER_SECOND = 1000;
  static final long MICROS_PER_SECOND = 1000000;

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

  private static final ThreadLocal<SimpleDateFormat> timestampFormat =
      new ThreadLocal<SimpleDateFormat>() {
        @Override
        protected SimpleDateFormat initialValue() {
          return createTimestampFormat();
        }
      };

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

  private static final Comparator<Timestamp> COMPARATOR =
      new Comparator<Timestamp>() {
        @Override
        public int compare(Timestamp t1, Timestamp t2) {
          checkValid(t1);
          checkValid(t2);
          int secDiff = Long.compare(t1.getSeconds(), t2.getSeconds());
          return (secDiff != 0) ? secDiff : Integer.compare(t1.getNanos(), t2.getNanos());
        }
      };

  /**
   * Returns a {@link Comparator} for {@link Timestamp Timestamps} which sorts in increasing
   * chronological order. Nulls and invalid {@link Timestamp Timestamps} are not allowed (see
   * {@link #isValid}).
   */
  public static Comparator<Timestamp> comparator() {
    return COMPARATOR;
  }

  /**
   * Compares two timestamps. The value returned is identical to what would be returned by: {@code
   * Timestamps.comparator().compare(x, y)}.
   *
   * @return the value {@code 0} if {@code x == y}; a value less than {@code 0} if {@code x < y};
   *     and a value greater than {@code 0} if {@code x > y}
   */
  public static int compare(Timestamp x, Timestamp y) {
    return COMPARATOR.compare(x, y);
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
    if (seconds < TIMESTAMP_SECONDS_MIN || seconds > TIMESTAMP_SECONDS_MAX) {
      return false;
    }
    if (nanos < 0 || nanos >= NANOS_PER_SECOND) {
      return false;
    }
    return true;
  }

  /** Throws an {@link IllegalArgumentException} if the given {@link Timestamp} is not valid. */
  @CanIgnoreReturnValue
  public static Timestamp checkValid(Timestamp timestamp) {
    long seconds = timestamp.getSeconds();
    int nanos = timestamp.getNanos();
    if (!isValid(seconds, nanos)) {
      throw new IllegalArgumentException(
          String.format(
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
   * uses 3, 6 or 9 fractional digits as required to represent the exact value. Note that Timestamp
   * can only represent time from 0001-01-01T00:00:00Z to 9999-12-31T23:59:59.999999999Z. See
   * https://www.ietf.org/rfc/rfc3339.txt
   *
   * <p>Example of generated format: "1972-01-01T10:00:20.021Z"
   *
   * @return The string representation of the given timestamp.
   * @throws IllegalArgumentException if the given timestamp is not in the valid range.
   */
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
   * @return A Timestamp parsed from the string.
   * @throws ParseException if parsing fails.
   */
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
      throw new ParseException("Failed to parse timestamp: timestamp is out of range.", 0);
    }
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
   * Convert a Timestamp to the number of milliseconds elapsed from the epoch.
   *
   * <p>The result will be rounded down to the nearest millisecond. E.g., if the timestamp
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

  private static long parseTimezoneOffset(String value) throws ParseException {
    int pos = value.indexOf(':');
    if (pos == -1) {
      throw new ParseException("Invalid offset value: " + value, 0);
    }
    String hours = value.substring(0, pos);
    String minutes = value.substring(pos + 1);
    return (Long.parseLong(hours) * 60 + Long.parseLong(minutes)) * 60;
  }

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
