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
import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.GregorianCalendar;
import java.util.TimeZone;

/**
 * Utilities to help create/manipulate Timestamp/Duration
 */
public class TimeUtil {
  // Timestamp for "0001-01-01T00:00:00Z"
  public static final long TIMESTAMP_SECONDS_MIN = -62135596800L;

  // Timestamp for "9999-12-31T23:59:59Z"
  public static final long TIMESTAMP_SECONDS_MAX = 253402300799L;
  public static final long DURATION_SECONDS_MIN = -315576000000L;
  public static final long DURATION_SECONDS_MAX = 315576000000L;

  private static final long NANOS_PER_SECOND = 1000000000;
  private static final long NANOS_PER_MILLISECOND = 1000000;
  private static final long NANOS_PER_MICROSECOND = 1000;
  private static final long MILLIS_PER_SECOND = 1000;
  private static final long MICROS_PER_SECOND = 1000000;

  private static final ThreadLocal<SimpleDateFormat> timestampFormat =
      new ThreadLocal<SimpleDateFormat>() {
        protected SimpleDateFormat initialValue() {
          return createTimestampFormat();
        }
      };

  private static SimpleDateFormat createTimestampFormat() {
    SimpleDateFormat sdf = new SimpleDateFormat("yyyy-MM-dd'T'HH:mm:ss");
    GregorianCalendar calendar =
      new GregorianCalendar(TimeZone.getTimeZone("UTC"));
    // We use Proleptic Gregorian Calendar (i.e., Gregorian calendar extends
    // backwards to year one) for timestamp formating.
    calendar.setGregorianChange(new Date(Long.MIN_VALUE));
    sdf.setCalendar(calendar);
    return sdf;
  }

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
   */
  public static String toString(Timestamp timestamp)
    throws IllegalArgumentException {
    StringBuilder result = new StringBuilder();
    // Format the seconds part.
    if (timestamp.getSeconds() < TIMESTAMP_SECONDS_MIN
        || timestamp.getSeconds() > TIMESTAMP_SECONDS_MAX) {
      throw new IllegalArgumentException("Timestamp is out of range.");
    }
    Date date = new Date(timestamp.getSeconds() * MILLIS_PER_SECOND);
    result.append(timestampFormat.get().format(date));
    // Format the nanos part.
    if (timestamp.getNanos() < 0 || timestamp.getNanos() >= NANOS_PER_SECOND) {
      throw new IllegalArgumentException("Timestamp has invalid nanos value.");
    }
    if (timestamp.getNanos() != 0) {
      result.append(".");
      result.append(formatNanos(timestamp.getNanos()));
    }
    result.append("Z");
    return result.toString();
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
   */

  public static Timestamp parseTimestamp(String value) throws ParseException {
    int dayOffset = value.indexOf('T');
    if (dayOffset == -1) {
      throw new ParseException(
        "Failed to parse timestamp: invalid timestamp \"" + value + "\"", 0);
    }
    int timezoneOffsetPosition = value.indexOf('Z', dayOffset);
    if (timezoneOffsetPosition == -1) {
      timezoneOffsetPosition = value.indexOf('+', dayOffset);
    }
    if (timezoneOffsetPosition == -1) {
      timezoneOffsetPosition = value.indexOf('-', dayOffset);
    }
    if (timezoneOffsetPosition == -1) {
      throw new ParseException(
        "Failed to parse timestamp: missing valid timezone offset.", 0);
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
          + value.substring(timezoneOffsetPosition) + "\"", 0);
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
      throw new ParseException(
        "Failed to parse timestmap: timestamp is out of range.", 0);
    }
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
   */
  public static String toString(Duration duration)
    throws IllegalArgumentException {
    if (duration.getSeconds() < DURATION_SECONDS_MIN
      || duration.getSeconds() > DURATION_SECONDS_MAX) {
      throw new IllegalArgumentException("Duration is out of valid range.");
    }
    StringBuilder result = new StringBuilder();
    long seconds = duration.getSeconds();
    int nanos = duration.getNanos();
    if (seconds < 0 || nanos < 0) {
      if (seconds > 0 || nanos > 0) {
        throw new IllegalArgumentException(
            "Invalid duration: seconds value and nanos value must have the same"
            + "sign.");
      }
      result.append("-");
      seconds = -seconds;
      nanos = -nanos;
    }
    result.append(seconds);
    if (nanos != 0) {
      result.append(".");
      result.append(formatNanos(nanos));
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
  public static Duration parseDuration(String value) throws ParseException {
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
    int nanos = nanoValue.isEmpty() ? 0 : parseNanos(nanoValue);
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

  /**
   * Create a Timestamp from the number of milliseconds elapsed from the epoch.
   */
  public static Timestamp createTimestampFromMillis(long milliseconds) {
    return normalizedTimestamp(milliseconds / MILLIS_PER_SECOND,
      (int) (milliseconds % MILLIS_PER_SECOND * NANOS_PER_MILLISECOND));
  }

  /**
   * Create a Duration from the number of milliseconds.
   */
  public static Duration createDurationFromMillis(long milliseconds) {
    return normalizedDuration(milliseconds / MILLIS_PER_SECOND,
      (int) (milliseconds % MILLIS_PER_SECOND * NANOS_PER_MILLISECOND));
  }

  /**
   * Convert a Timestamp to the number of milliseconds elapsed from the epoch.
   *
   * <p>The result will be rounded down to the nearest millisecond. E.g., if the
   * timestamp represents "1969-12-31T23:59:59.999999999Z", it will be rounded
   * to -1 millisecond.
   */
  public static long toMillis(Timestamp timestamp) {
    return timestamp.getSeconds() * MILLIS_PER_SECOND + timestamp.getNanos()
      / NANOS_PER_MILLISECOND;
  }

  /**
   * Convert a Duration to the number of milliseconds.The result will be
   * rounded towards 0 to the nearest millisecond. E.g., if the duration
   * represents -1 nanosecond, it will be rounded to 0.
   */
  public static long toMillis(Duration duration) {
    return duration.getSeconds() * MILLIS_PER_SECOND + duration.getNanos()
      / NANOS_PER_MILLISECOND;
  }

  /**
   * Create a Timestamp from the number of microseconds elapsed from the epoch.
   */
  public static Timestamp createTimestampFromMicros(long microseconds) {
    return normalizedTimestamp(microseconds / MICROS_PER_SECOND,
      (int) (microseconds % MICROS_PER_SECOND * NANOS_PER_MICROSECOND));
  }

  /**
   * Create a Duration from the number of microseconds.
   */
  public static Duration createDurationFromMicros(long microseconds) {
    return normalizedDuration(microseconds / MICROS_PER_SECOND,
      (int) (microseconds % MICROS_PER_SECOND * NANOS_PER_MICROSECOND));
  }

  /**
   * Convert a Timestamp to the number of microseconds elapsed from the epoch.
   *
   * <p>The result will be rounded down to the nearest microsecond. E.g., if the
   * timestamp represents "1969-12-31T23:59:59.999999999Z", it will be rounded
   * to -1 millisecond.
   */
  public static long toMicros(Timestamp timestamp) {
    return timestamp.getSeconds() * MICROS_PER_SECOND + timestamp.getNanos()
      / NANOS_PER_MICROSECOND;
  }

  /**
   * Convert a Duration to the number of microseconds.The result will be
   * rounded towards 0 to the nearest microseconds. E.g., if the duration
   * represents -1 nanosecond, it will be rounded to 0.
   */
  public static long toMicros(Duration duration) {
    return duration.getSeconds() * MICROS_PER_SECOND + duration.getNanos()
      / NANOS_PER_MICROSECOND;
  }

  /**
   * Create a Timestamp from the number of nanoseconds elapsed from the epoch.
   */
  public static Timestamp createTimestampFromNanos(long nanoseconds) {
    return normalizedTimestamp(nanoseconds / NANOS_PER_SECOND,
      (int) (nanoseconds % NANOS_PER_SECOND));
  }

  /**
   * Create a Duration from the number of nanoseconds.
   */
  public static Duration createDurationFromNanos(long nanoseconds) {
    return normalizedDuration(nanoseconds / NANOS_PER_SECOND,
      (int) (nanoseconds % NANOS_PER_SECOND));
  }

  /**
   * Convert a Timestamp to the number of nanoseconds elapsed from the epoch.
   */
  public static long toNanos(Timestamp timestamp) {
    return timestamp.getSeconds() * NANOS_PER_SECOND + timestamp.getNanos();
  }

  /**
   * Convert a Duration to the number of nanoseconds.
   */
  public static long toNanos(Duration duration) {
    return duration.getSeconds() * NANOS_PER_SECOND + duration.getNanos();
  }

  /**
   * Get the current time.
   */
  public static Timestamp getCurrentTime() {
    return createTimestampFromMillis(System.currentTimeMillis());
  }

  /**
   * Get the epoch.
   */
  public static Timestamp getEpoch() {
    return Timestamp.getDefaultInstance();
  }

  /**
   * Calculate the difference between two timestamps.
   */
  public static Duration distance(Timestamp from, Timestamp to) {
    return normalizedDuration(to.getSeconds() - from.getSeconds(),
      to.getNanos() - from.getNanos());
  }

  /**
   * Add a duration to a timestamp.
   */
  public static Timestamp add(Timestamp start, Duration length) {
    return normalizedTimestamp(start.getSeconds() + length.getSeconds(),
      start.getNanos() + length.getNanos());
  }

  /**
   * Subtract a duration from a timestamp.
   */
  public static Timestamp subtract(Timestamp start, Duration length) {
    return normalizedTimestamp(start.getSeconds() - length.getSeconds(),
      start.getNanos() - length.getNanos());
  }

  /**
   * Add two durations.
   */
  public static Duration add(Duration d1, Duration d2) {
    return normalizedDuration(d1.getSeconds() + d2.getSeconds(),
      d1.getNanos() + d2.getNanos());
  }

  /**
   * Subtract a duration from another.
   */
  public static Duration subtract(Duration d1, Duration d2) {
    return normalizedDuration(d1.getSeconds() - d2.getSeconds(),
      d1.getNanos() - d2.getNanos());
  }

  // Multiplications and divisions.

  public static Duration multiply(Duration duration, double times) {
    double result = duration.getSeconds() * times + duration.getNanos() * times
      / 1000000000.0;
    if (result < Long.MIN_VALUE || result > Long.MAX_VALUE) {
      throw new IllegalArgumentException("Result is out of valid range.");
    }
    long seconds = (long) result;
    int nanos = (int) ((result - seconds) * 1000000000);
    return normalizedDuration(seconds, nanos);
  }
  
  public static Duration divide(Duration duration, double value) {
    return multiply(duration, 1.0 / value);
  }
  
  public static Duration multiply(Duration duration, long times) {
    return createDurationFromBigInteger(
      toBigInteger(duration).multiply(toBigInteger(times)));
  }
  
  public static Duration divide(Duration duration, long times) {
    return createDurationFromBigInteger(
      toBigInteger(duration).divide(toBigInteger(times)));
  }
  
  public static long divide(Duration d1, Duration d2) {
    return toBigInteger(d1).divide(toBigInteger(d2)).longValue();
  }
  
  public static Duration remainder(Duration d1, Duration d2) {
    return createDurationFromBigInteger(
      toBigInteger(d1).remainder(toBigInteger(d2)));
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
    long seconds = value.divide(
      new BigInteger(String.valueOf(NANOS_PER_SECOND))).longValue();
    int nanos = value.remainder(
      new BigInteger(String.valueOf(NANOS_PER_SECOND))).intValue();
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

  private static Timestamp normalizedTimestamp(long seconds, int nanos) {
    if (nanos <= -NANOS_PER_SECOND || nanos >= NANOS_PER_SECOND) {
      seconds += nanos / NANOS_PER_SECOND;
      nanos %= NANOS_PER_SECOND;
    }
    if (nanos < 0) {
      nanos += NANOS_PER_SECOND;
      seconds -= 1;
    }
    if (seconds < TIMESTAMP_SECONDS_MIN || seconds > TIMESTAMP_SECONDS_MAX) {
      throw new IllegalArgumentException("Timestamp is out of valid range.");
    }
    return Timestamp.newBuilder().setSeconds(seconds).setNanos(nanos).build();
  }

  /**
   * Format the nano part of a timestamp or a duration.
   */
  private static String formatNanos(int nanos) {
    assert nanos >= 1 && nanos <= 999999999;
    // Determine whether to use 3, 6, or 9 digits for the nano part.
    if (nanos % NANOS_PER_MILLISECOND == 0) {
      return String.format("%1$03d", nanos / NANOS_PER_MILLISECOND);
    } else if (nanos % NANOS_PER_MICROSECOND == 0) {
      return String.format("%1$06d", nanos / NANOS_PER_MICROSECOND);
    } else {
      return String.format("%1$09d", nanos);
    }
  }

  private static int parseNanos(String value) throws ParseException {
    int result = 0;
    for (int i = 0; i < 9; ++i) {
      result = result * 10;
      if (i < value.length()) {
        if (value.charAt(i) < '0' || value.charAt(i) > '9') {
          throw new ParseException("Invalid nanosecnds.", 0);
        }
        result += value.charAt(i) - '0';
      }
    }
    return result;
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
}
