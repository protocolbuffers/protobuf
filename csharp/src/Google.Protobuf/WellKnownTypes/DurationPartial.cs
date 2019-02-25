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
using System.Globalization;
using System.Text;

namespace Google.Protobuf.WellKnownTypes
{
    // Manually-written partial class for the Duration well-known type,
    // providing a conversion to TimeSpan and convenience operators.
    public partial class Duration : ICustomDiagnosticMessage
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

        internal const int MaxNanoseconds = NanosecondsPerSecond - 1;
        internal const int MinNanoseconds = -NanosecondsPerSecond + 1;

        internal static bool IsNormalized(long seconds, int nanoseconds)
        {
            // Simple boundaries
            if (seconds < MinSeconds || seconds > MaxSeconds ||
                nanoseconds < MinNanoseconds || nanoseconds > MaxNanoseconds)
            {
                return false;
            }
            // We only have a problem is one is strictly negative and the other is
            // strictly positive.
            return Math.Sign(seconds) * Math.Sign(nanoseconds) != -1;
        }

        /// <summary>
        /// Converts this <see cref="Duration"/> to a <see cref="TimeSpan"/>.
        /// </summary>
        /// <remarks>If the duration is not a precise number of ticks, it is truncated towards 0.</remarks>
        /// <returns>The value of this duration, as a <c>TimeSpan</c>.</returns>
        /// <exception cref="InvalidOperationException">This value isn't a valid normalized duration, as
        /// described in the documentation.</exception>
        public TimeSpan ToTimeSpan()
        {
            checked
            {
                if (!IsNormalized(Seconds, Nanos))
                {
                    throw new InvalidOperationException("Duration was not a valid normalized duration");
                }
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
            ProtoPreconditions.CheckNotNull(value, "value");
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
            ProtoPreconditions.CheckNotNull(lhs, "lhs");
            ProtoPreconditions.CheckNotNull(rhs, "rhs");
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
            ProtoPreconditions.CheckNotNull(lhs, "lhs");
            ProtoPreconditions.CheckNotNull(rhs, "rhs");
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

        /// <summary>
        /// Converts a duration specified in seconds/nanoseconds to a string.
        /// </summary>
        /// <remarks>
        /// If the value is a normalized duration in the range described in <c>duration.proto</c>,
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
                var builder = new StringBuilder();
                builder.Append('"');
                // The seconds part will normally provide the minus sign if we need it, but not if it's 0...
                if (seconds == 0 && nanoseconds < 0)
                {
                    builder.Append('-');
                }

                builder.Append(seconds.ToString("d", CultureInfo.InvariantCulture));
                AppendNanoseconds(builder, Math.Abs(nanoseconds));
                builder.Append("s\"");
                return builder.ToString();
            }
            if (diagnosticOnly)
            {
                // Note: the double braces here are escaping for braces in format strings.
                return string.Format(CultureInfo.InvariantCulture,
                    "{{ \"@warning\": \"Invalid Duration\", \"seconds\": \"{0}\", \"nanos\": {1} }}",
                    seconds,
                    nanoseconds);
            }
            else
            {
                throw new InvalidOperationException("Non-normalized duration value");
            }
        }

        /// <summary>
        /// Returns a string representation of this <see cref="Duration"/> for diagnostic purposes.
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

        /// <summary>
        /// Appends a number of nanoseconds to a StringBuilder. Either 0 digits are added (in which
        /// case no "." is appended), or 3 6 or 9 digits. This is internal for use in Timestamp as well
        /// as Duration.
        /// </summary>
        internal static void AppendNanoseconds(StringBuilder builder, int nanos)
        {
            if (nanos != 0)
            {
                builder.Append('.');
                // Output to 3, 6 or 9 digits.
                if (nanos % 1000000 == 0)
                {
                    builder.Append((nanos / 1000000).ToString("d3", CultureInfo.InvariantCulture));
                }
                else if (nanos % 1000 == 0)
                {
                    builder.Append((nanos / 1000).ToString("d6", CultureInfo.InvariantCulture));
                }
                else
                {
                    builder.Append(nanos.ToString("d9", CultureInfo.InvariantCulture));
                }
            }
        }
    }
}
