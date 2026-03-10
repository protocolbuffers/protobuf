#region Copyright notice and license
// Protocol Buffers - Google's data interchange format
// Copyright 2015 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd
#endregion

using System;
using System.Globalization;
using System.Text;

namespace Google.Protobuf.WellKnownTypes
{
    public partial class Timestamp : ICustomDiagnosticMessage, IComparable<Timestamp>
    {
        private static readonly DateTime UnixEpoch = new DateTime(1970, 1, 1, 0, 0, 0, DateTimeKind.Utc);
        // Constants determined programmatically, but then hard-coded so they can be constant expressions.
        private const long BclSecondsAtUnixEpoch = 62135596800;
        internal const long UnixSecondsAtBclMaxValue = 253402300799;
        internal const long UnixSecondsAtBclMinValue = -BclSecondsAtUnixEpoch;
        internal const int MaxNanos = Duration.NanosecondsPerSecond - 1;

        private static bool IsNormalized(long seconds, int nanoseconds) =>
            nanoseconds >= 0 &&
            nanoseconds <= MaxNanos &&
            seconds >= UnixSecondsAtBclMinValue &&
            seconds <= UnixSecondsAtBclMaxValue;

        /// <summary>
        /// Returns the difference between one <see cref="Timestamp"/> and another, as a <see cref="Duration"/>.
        /// </summary>
        /// <param name="lhs">The timestamp to subtract from. Must not be null.</param>
        /// <param name="rhs">The timestamp to subtract. Must not be null.</param>
        /// <returns>The difference between the two specified timestamps.</returns>
        public static Duration operator -(Timestamp lhs, Timestamp rhs)
        {
            ProtoPreconditions.CheckNotNull(lhs, nameof(lhs));
            ProtoPreconditions.CheckNotNull(rhs, nameof(rhs));
            checked
            {
                return Duration.Normalize(lhs.Seconds - rhs.Seconds, lhs.Nanos - rhs.Nanos);
            }
        }

        /// <summary>
        /// Adds a <see cref="Duration"/> to a <see cref="Timestamp"/>, to obtain another <c>Timestamp</c>.
        /// </summary>
        /// <param name="lhs">The timestamp to add the duration to. Must not be null.</param>
        /// <param name="rhs">The duration to add. Must not be null.</param>
        /// <returns>The result of adding the duration to the timestamp.</returns>
        public static Timestamp operator +(Timestamp lhs, Duration rhs)
        {
            ProtoPreconditions.CheckNotNull(lhs, nameof(lhs));
            ProtoPreconditions.CheckNotNull(rhs, nameof(rhs));
            checked
            {
                return Normalize(lhs.Seconds + rhs.Seconds, lhs.Nanos + rhs.Nanos);
            }
        }

        /// <summary>
        /// Subtracts a <see cref="Duration"/> from a <see cref="Timestamp"/>, to obtain another <c>Timestamp</c>.
        /// </summary>
        /// <param name="lhs">The timestamp to subtract the duration from. Must not be null.</param>
        /// <param name="rhs">The duration to subtract.</param>
        /// <returns>The result of subtracting the duration from the timestamp.</returns>
        public static Timestamp operator -(Timestamp lhs, Duration rhs)
        {
            ProtoPreconditions.CheckNotNull(lhs, nameof(lhs));
            ProtoPreconditions.CheckNotNull(rhs, nameof(rhs));
            checked
            {
                return Normalize(lhs.Seconds - rhs.Seconds, lhs.Nanos - rhs.Nanos);
            }
        }

        /// <summary>
        /// Converts this timestamp into a <see cref="DateTime"/>.
        /// </summary>
        /// <remarks>
        /// The resulting <c>DateTime</c> will always have a <c>Kind</c> of <c>Utc</c>.
        /// If the timestamp is not a precise number of ticks, it will be truncated towards the start
        /// of time. For example, a timestamp with a <see cref="Nanos"/> value of 99 will result in a
        /// <see cref="DateTime"/> value precisely on a second.
        /// </remarks>
        /// <returns>This timestamp as a <c>DateTime</c>.</returns>
        /// <exception cref="InvalidOperationException">The timestamp contains invalid values; either it is
        /// incorrectly normalized or is outside the valid range.</exception>
        public DateTime ToDateTime()
        {
            if (!IsNormalized(Seconds, Nanos))
            {
                throw new InvalidOperationException(@"Timestamp contains invalid values: Seconds={Seconds}; Nanos={Nanos}");
            }
            return UnixEpoch.AddSeconds(Seconds).AddTicks(Nanos / Duration.NanosecondsPerTick);
        }

        /// <summary>
        /// Converts this timestamp into a <see cref="DateTimeOffset"/>.
        /// </summary>
        /// <remarks>
        /// The resulting <c>DateTimeOffset</c> will always have an <c>Offset</c> of zero.
        /// If the timestamp is not a precise number of ticks, it will be truncated towards the start
        /// of time. For example, a timestamp with a <see cref="Nanos"/> value of 99 will result in a
        /// <see cref="DateTimeOffset"/> value precisely on a second.
        /// </remarks>
        /// <returns>This timestamp as a <c>DateTimeOffset</c>.</returns>
        /// <exception cref="InvalidOperationException">The timestamp contains invalid values; either it is
        /// incorrectly normalized or is outside the valid range.</exception>
        public DateTimeOffset ToDateTimeOffset()
        {
            return new DateTimeOffset(ToDateTime(), TimeSpan.Zero);
        }

        /// <summary>
        /// Converts the specified <see cref="DateTime"/> to a <see cref="Timestamp"/>.
        /// </summary>
        /// <param name="dateTime"></param>
        /// <exception cref="ArgumentException">The <c>Kind</c> of <paramref name="dateTime"/> is not <c>DateTimeKind.Utc</c>.</exception>
        /// <returns>The converted timestamp.</returns>
        public static Timestamp FromDateTime(DateTime dateTime)
        {
            if (dateTime.Kind != DateTimeKind.Utc)
            {
                throw new ArgumentException("Conversion from DateTime to Timestamp requires the DateTime kind to be Utc", "dateTime");
            }
            // Do the arithmetic using DateTime.Ticks, which is always non-negative, making things simpler.
            long secondsSinceBclEpoch = dateTime.Ticks / TimeSpan.TicksPerSecond;
            int nanoseconds = (int)  (dateTime.Ticks % TimeSpan.TicksPerSecond) * Duration.NanosecondsPerTick;
            return new Timestamp { Seconds = secondsSinceBclEpoch - BclSecondsAtUnixEpoch, Nanos = nanoseconds };
        }

        /// <summary>
        /// Converts the given <see cref="DateTimeOffset"/> to a <see cref="Timestamp"/>
        /// </summary>
        /// <remarks>The offset is taken into consideration when converting the value (so the same instant in time
        /// is represented) but is not a separate part of the resulting value. In other words, there is no
        /// roundtrip operation to retrieve the original <c>DateTimeOffset</c>.</remarks>
        /// <param name="dateTimeOffset">The date and time (with UTC offset) to convert to a timestamp.</param>
        /// <returns>The converted timestamp.</returns>
        public static Timestamp FromDateTimeOffset(DateTimeOffset dateTimeOffset)
        {
            // We don't need to worry about this having negative ticks: DateTimeOffset is constrained to handle
            // values whose *UTC* value is in the range of DateTime.
            return FromDateTime(dateTimeOffset.UtcDateTime);
        }

        internal static Timestamp Normalize(long seconds, int nanoseconds)
        {
            int extraSeconds = nanoseconds / Duration.NanosecondsPerSecond;
            seconds += extraSeconds;
            nanoseconds -= extraSeconds * Duration.NanosecondsPerSecond;

            if (nanoseconds < 0)
            {
                nanoseconds += Duration.NanosecondsPerSecond;
                seconds--;
            }
            return new Timestamp { Seconds = seconds, Nanos = nanoseconds };
        }

        /// <summary>
        /// Converts a timestamp specified in seconds/nanoseconds to a string.
        /// </summary>
        /// <remarks>
        /// If the value is a normalized duration in the range described in <c>timestamp.proto</c>,
        /// <paramref name="diagnosticOnly"/> is ignored. Otherwise, if the parameter is <c>true</c>,
        /// a JSON object with a warning is returned; if it is <c>false</c>, an <see cref="InvalidOperationException"/> is thrown.
        /// </remarks>
        /// <param name="seconds">Seconds portion of the duration.</param>
        /// <param name="nanoseconds">Nanoseconds portion of the duration.</param>
        /// <param name="diagnosticOnly">Determines the handling of non-normalized values</param>
        /// <exception cref="InvalidOperationException">The represented duration is invalid, and <paramref name="diagnosticOnly"/> is <c>false</c>.</exception>
        internal static string ToJson(long seconds, int nanoseconds, bool diagnosticOnly)
        {
            if (IsNormalized(seconds, nanoseconds))
            {
                // Use .NET's formatting for the value down to the second, including an opening double quote (as it's a string value)
                DateTime dateTime = UnixEpoch.AddSeconds(seconds);
                var builder = new StringBuilder();
                builder.Append('"');
                builder.Append(dateTime.ToString("yyyy'-'MM'-'dd'T'HH:mm:ss", CultureInfo.InvariantCulture));
                Duration.AppendNanoseconds(builder, nanoseconds);
                builder.Append("Z\"");
                return builder.ToString();
            }
            if (diagnosticOnly)
            {
                return string.Format(CultureInfo.InvariantCulture,
                    "{{ \"@warning\": \"Invalid Timestamp\", \"seconds\": \"{0}\", \"nanos\": {1} }}",
                    seconds,
                    nanoseconds);
            }
            else
            {
                throw new InvalidOperationException("Non-normalized timestamp value");
            }
        }

        /// <summary>
        /// Given another timestamp, returns 0 if the timestamps are equivalent, -1 if this timestamp precedes the other, and 1 otherwise
        /// </summary>
        /// <remarks>
        /// Make sure the timestamps are normalized. Comparing non-normalized timestamps is not specified and may give unexpected results.
        /// </remarks>
        /// <param name="other">Timestamp to compare</param>
        /// <returns>an integer indicating whether this timestamp precedes or follows the other</returns>
        public int CompareTo(Timestamp other)
        {
            return other == null ? 1
                : Seconds < other.Seconds ? -1
                : Seconds > other.Seconds ? 1
                : Nanos < other.Nanos ? -1
                : Nanos > other.Nanos ? 1
                : 0;
        }

        /// <summary>
        /// Compares two timestamps and returns whether the first is less than (chronologically precedes) the second
        /// </summary>
        /// <remarks>
        /// Make sure the timestamps are normalized. Comparing non-normalized timestamps is not specified and may give unexpected results.
        /// </remarks>
        /// <param name="a"></param>
        /// <param name="b"></param>
        /// <returns>true if a precedes b</returns>
        public static bool operator <(Timestamp a, Timestamp b)
        {
            return a.CompareTo(b) < 0;
        }

        /// <summary>
        /// Compares two timestamps and returns whether the first is greater than (chronologically follows) the second
        /// </summary>
        /// <remarks>
        /// Make sure the timestamps are normalized. Comparing non-normalized timestamps is not specified and may give unexpected results.
        /// </remarks>
        /// <param name="a"></param>
        /// <param name="b"></param>
        /// <returns>true if a follows b</returns>
        public static bool operator >(Timestamp a, Timestamp b)
        {
            return a.CompareTo(b) > 0;
        }

        /// <summary>
        /// Compares two timestamps and returns whether the first is less than (chronologically precedes) the second
        /// </summary>
        /// <remarks>
        /// Make sure the timestamps are normalized. Comparing non-normalized timestamps is not specified and may give unexpected results.
        /// </remarks>
        /// <param name="a"></param>
        /// <param name="b"></param>
        /// <returns>true if a precedes b</returns>
        public static bool operator <=(Timestamp a, Timestamp b)
        {
            return a.CompareTo(b) <= 0;
        }

        /// <summary>
        /// Compares two timestamps and returns whether the first is greater than (chronologically follows) the second
        /// </summary>
        /// <remarks>
        /// Make sure the timestamps are normalized. Comparing non-normalized timestamps is not specified and may give unexpected results.
        /// </remarks>
        /// <param name="a"></param>
        /// <param name="b"></param>
        /// <returns>true if a follows b</returns>
        public static bool operator >=(Timestamp a, Timestamp b)
        {
            return a.CompareTo(b) >= 0;
        }


        /// <summary>
        /// Returns whether two timestamps are equivalent
        /// </summary>
        /// <remarks>
        /// Make sure the timestamps are normalized. Comparing non-normalized timestamps is not specified and may give unexpected results.
        /// </remarks>
        /// <param name="a"></param>
        /// <param name="b"></param>
        /// <returns>true if the two timestamps refer to the same nanosecond</returns>
        public static bool operator ==(Timestamp a, Timestamp b)
        {
            return ReferenceEquals(a, b) || (a is null ? (b is null) : a.Equals(b));
        }

        /// <summary>
        /// Returns whether two timestamps differ
        /// </summary>
        /// <remarks>
        /// Make sure the timestamps are normalized. Comparing non-normalized timestamps is not specified and may give unexpected results.
        /// </remarks>
        /// <param name="a"></param>
        /// <param name="b"></param>
        /// <returns>true if the two timestamps differ</returns>
        public static bool operator !=(Timestamp a, Timestamp b)
        {
            return !(a == b);
        }

        /// <summary>
        /// Returns a string representation of this <see cref="Timestamp"/> for diagnostic purposes.
        /// </summary>
        /// <remarks>
        /// Normally the returned value will be a JSON string value (including leading and trailing quotes) but
        /// when the value is non-normalized or out of range, a JSON object representation will be returned
        /// instead, including a warning. This is to avoid exceptions being thrown when trying to
        /// diagnose problems - the regular JSON formatter will still throw an exception for non-normalized
        /// values.
        /// </remarks>
        /// <returns>A string representation of this value.</returns>
        public string ToDiagnosticString()
        {
            return ToJson(Seconds, Nanos, true);
        }
    }
}
