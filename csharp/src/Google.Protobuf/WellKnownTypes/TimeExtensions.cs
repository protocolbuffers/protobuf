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
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

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
