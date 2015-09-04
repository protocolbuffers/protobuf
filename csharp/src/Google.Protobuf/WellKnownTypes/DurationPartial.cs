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
    // Manually-written partial class for the Duration well-known type,
    // providing a conversion to TimeSpan and convenience operators.
    public partial class Duration
    {
        /// <summary>
        /// The number of nanoseconds in a second.
        /// </summary>
        public const int NanosecondsPerSecond = 1000000000;
        /// <summary>
        /// The number of nanoseconds in a BCL tick (as used by <see cref="TimeSpan"/> and <see cref="DateTime"/>).
        /// </summary>
        public const int NanosecondsPerTick = 100;

        /// <summary>
        /// The maximum permitted number of seconds.
        /// </summary>
        public const long MaxSeconds = 315576000000L;

        /// <summary>
        /// The minimum permitted number of seconds.
        /// </summary>
        public const long MinSeconds = -315576000000L;

        /// <summary>
        /// Converts this <see cref="Duration"/> to a <see cref="TimeSpan"/>.
        /// </summary>
        /// <remarks>If the duration is not a precise number of ticks, it is truncated towards 0.</remarks>
        /// <returns>The value of this duration, as a <c>TimeSpan</c>.</returns>
        public TimeSpan ToTimeSpan()
        {
            checked
            {
                long ticks = Seconds * TimeSpan.TicksPerSecond + Nanos / NanosecondsPerTick;
                return TimeSpan.FromTicks(ticks);
            }
        }

        /// <summary>
        /// Converts the given <see cref="TimeSpan"/> to a <see cref="Duration"/>.
        /// </summary>
        /// <param name="timeSpan">The <c>TimeSpan</c> to convert.</param>
        /// <returns>The value of the given <c>TimeSpan</c>, as a <c>Duration</c>.</returns>
        public static Duration FromTimeSpan(TimeSpan timeSpan)
        {
            checked
            {
                long ticks = timeSpan.Ticks;
                long seconds = ticks / TimeSpan.TicksPerSecond;
                int nanos = (int) (ticks % TimeSpan.TicksPerSecond) * NanosecondsPerTick;
                return new Duration { Seconds = seconds, Nanos = nanos };
            }
        }

        /// <summary>
        /// Returns the result of negating the duration. For example, the negation of 5 minutes is -5 minutes.
        /// </summary>
        /// <param name="value">The duration to negate. Must not be null.</param>
        /// <returns>The negated value of this duration.</returns>
        public static Duration operator -(Duration value)
        {
            Preconditions.CheckNotNull(value, "value");
            checked
            {
                return Normalize(-value.Seconds, -value.Nanos);
            }
        }

        /// <summary>
        /// Adds the two specified <see cref="Duration"/> values together.
        /// </summary>
        /// <param name="lhs">The first value to add. Must not be null.</param>
        /// <param name="rhs">The second value to add. Must not be null.</param>
        /// <returns></returns>
        public static Duration operator +(Duration lhs, Duration rhs)
        {
            Preconditions.CheckNotNull(lhs, "lhs");
            Preconditions.CheckNotNull(rhs, "rhs");
            checked
            {
                return Normalize(lhs.Seconds + rhs.Seconds, lhs.Nanos + rhs.Nanos);
            }
        }

        /// <summary>
        /// Subtracts one <see cref="Duration"/> from another.
        /// </summary>
        /// <param name="lhs">The duration to subtract from. Must not be null.</param>
        /// <param name="rhs">The duration to subtract. Must not be null.</param>
        /// <returns>The difference between the two specified durations.</returns>
        public static Duration operator -(Duration lhs, Duration rhs)
        {
            Preconditions.CheckNotNull(lhs, "lhs");
            Preconditions.CheckNotNull(rhs, "rhs");
            checked
            {
                return Normalize(lhs.Seconds - rhs.Seconds, lhs.Nanos - rhs.Nanos);
            }
        }
        
        /// <summary>
        /// Creates a duration with the normalized values from the given number of seconds and
        /// nanoseconds, conforming with the description in the proto file.
        /// </summary>
        internal static Duration Normalize(long seconds, int nanoseconds)
        {
            // Ensure that nanoseconds is in the range (-1,000,000,000, +1,000,000,000)
            int extraSeconds = nanoseconds / NanosecondsPerSecond;
            seconds += extraSeconds;
            nanoseconds -= extraSeconds * NanosecondsPerSecond;

            // Now make sure that Sign(seconds) == Sign(nanoseconds) if Sign(seconds) != 0.
            if (seconds < 0 && nanoseconds > 0)
            {
                seconds += 1;
                nanoseconds -= NanosecondsPerSecond;
            }
            else if (seconds > 0 && nanoseconds < 0)
            {
                seconds -= 1;
                nanoseconds += NanosecondsPerSecond;
            }
            return new Duration { Seconds = seconds, Nanos = nanoseconds };
        }
    }
}
