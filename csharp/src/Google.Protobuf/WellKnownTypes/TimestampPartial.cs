#region Copyright notice and license
// Protocol Buffers - Google's data interchange format
// Copyright 2015 Google Inc.  All rights reserved.
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
#endregion

using System;

namespace Google.Protobuf.WellKnownTypes
{
    public partial class Timestamp
    {
        private static readonly DateTime UnixEpoch = new DateTime(1970, 1, 1, 0, 0, 0, DateTimeKind.Utc);
        private static readonly long BclSecondsAtUnixEpoch = UnixEpoch.Ticks / TimeSpan.TicksPerSecond;
        internal static readonly long UnixSecondsAtBclMinValue = -BclSecondsAtUnixEpoch;
        internal static readonly long UnixSecondsAtBclMaxValue = (DateTime.MaxValue.Ticks / TimeSpan.TicksPerSecond) - BclSecondsAtUnixEpoch;

        /// <summary>
        /// Returns the difference between one <see cref="Timestamp"/> and another, as a <see cref="Duration"/>.
        /// </summary>
        /// <param name="lhs">The timestamp to subtract from. Must not be null.</param>
        /// <param name="rhs">The timestamp to subtract. Must not be null.</param>
        /// <returns>The difference between the two specified timestamps.</returns>
        public static Duration operator -(Timestamp lhs, Timestamp rhs)
        {
            Preconditions.CheckNotNull(lhs, "lhs");
            Preconditions.CheckNotNull(rhs, "rhs");
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
            Preconditions.CheckNotNull(lhs, "lhs");
            Preconditions.CheckNotNull(rhs, "rhs");
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
            Preconditions.CheckNotNull(lhs, "lhs");
            Preconditions.CheckNotNull(rhs, "rhs");
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
        public DateTime ToDateTime()
        {
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
    }
}
