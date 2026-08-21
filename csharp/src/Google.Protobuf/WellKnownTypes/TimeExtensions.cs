#region Copyright notice and license
// Protocol Buffers - Google's data interchange format
// Copyright 2015 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd
#endregion

using System;

namespace Google.Protobuf.WellKnownTypes
{
    /// <summary>
    /// Extension methods on BCL time-related types, converting to protobuf types.
    /// </summary>
    public static class TimeExtensions
    {
        /// <summary>
        /// Converts the given <see cref="DateTime"/> to a <see cref="Timestamp"/>.
        /// </summary>
        /// <param name="dateTime">The date and time to convert to a timestamp.</param>
        /// <exception cref="ArgumentException">The <paramref name="dateTime"/> value has a <see cref="DateTime.Kind"/>other than <c>Utc</c>.</exception>
        /// <returns>The converted timestamp.</returns>
        public static Timestamp ToTimestamp(this DateTime dateTime)
        {
            return Timestamp.FromDateTime(dateTime);
        }

        /// <summary>
        /// Converts the given <see cref="DateTimeOffset"/> to a <see cref="Timestamp"/>
        /// </summary>
        /// <remarks>The offset is taken into consideration when converting the value (so the same instant in time
        /// is represented) but is not a separate part of the resulting value. In other words, there is no
        /// roundtrip operation to retrieve the original <c>DateTimeOffset</c>.</remarks>
        /// <param name="dateTimeOffset">The date and time (with UTC offset) to convert to a timestamp.</param>
        /// <returns>The converted timestamp.</returns>
        public static Timestamp ToTimestamp(this DateTimeOffset dateTimeOffset)
        {
            return Timestamp.FromDateTimeOffset(dateTimeOffset);
        }

        /// <summary>
        /// Converts the given <see cref="TimeSpan"/> to a <see cref="Duration"/>.
        /// </summary>
        /// <param name="timeSpan">The time span to convert.</param>
        /// <returns>The converted duration.</returns>
        public static Duration ToDuration(this TimeSpan timeSpan)
        {
            return Duration.FromTimeSpan(timeSpan);
        }
    }
}
