#region Copyright notice and license
// Protocol Buffers - Google's data interchange format
// Copyright 2017 Google Inc.  All rights reserved.
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

namespace Google.Protobuf.Collections
{
    /// <summary>
    /// Provides a central place to implement equality comparisons, primarily for bitwise float/double equality.
    /// </summary>
    public static class ProtobufEqualityComparers
    {
        /// <summary>
        /// Returns an equality comparer for <typeparamref name="T"/> suitable for Protobuf equality comparisons.
        /// This is usually just the default equality comparer for the type, but floating point numbers are compared
        /// bitwise.
        /// </summary>
        /// <typeparam name="T">The type of equality comparer to return.</typeparam>
        /// <returns>The equality comparer.</returns>
        public static EqualityComparer<T> GetEqualityComparer<T>()
        {
            return typeof(T) == typeof(double) ? (EqualityComparer<T>) (object) BitwiseDoubleEqualityComparer
                : typeof(T) == typeof(float) ? (EqualityComparer<T>) (object) BitwiseSingleEqualityComparer
                : typeof(T) == typeof(double?) ? (EqualityComparer<T>) (object) BitwiseNullableDoubleEqualityComparer
                : typeof(T) == typeof(float?) ? (EqualityComparer<T>) (object) BitwiseNullableSingleEqualityComparer
                : EqualityComparer<T>.Default;
        }

        /// <summary>
        /// Returns an equality comparer suitable for comparing 64-bit floating point values, by bitwise comparison.
        /// (NaN values are considered equal, but only when they have the same representation.)
        /// </summary>
        public static EqualityComparer<double> BitwiseDoubleEqualityComparer { get; } = new BitwiseDoubleEqualityComparerImpl();

        /// <summary>
        /// Returns an equality comparer suitable for comparing 32-bit floating point values, by bitwise comparison.
        /// (NaN values are considered equal, but only when they have the same representation.)
        /// </summary>
        public static EqualityComparer<float> BitwiseSingleEqualityComparer { get; } = new BitwiseSingleEqualityComparerImpl();

        /// <summary>
        /// Returns an equality comparer suitable for comparing nullable 64-bit floating point values, by bitwise comparison.
        /// (NaN values are considered equal, but only when they have the same representation.)
        /// </summary>
        public static EqualityComparer<double?> BitwiseNullableDoubleEqualityComparer { get; } = new BitwiseNullableDoubleEqualityComparerImpl();

        /// <summary>
        /// Returns an equality comparer suitable for comparing nullable 32-bit floating point values, by bitwise comparison.
        /// (NaN values are considered equal, but only when they have the same representation.)
        /// </summary>
        public static EqualityComparer<float?> BitwiseNullableSingleEqualityComparer { get; } = new BitwiseNullableSingleEqualityComparerImpl();

        private class BitwiseDoubleEqualityComparerImpl : EqualityComparer<double>
        {
            public override bool Equals(double x, double y) =>
                BitConverter.DoubleToInt64Bits(x) == BitConverter.DoubleToInt64Bits(y);

            public override int GetHashCode(double obj) =>
                BitConverter.DoubleToInt64Bits(obj).GetHashCode();
        }

        private class BitwiseSingleEqualityComparerImpl : EqualityComparer<float>
        {
            // Just promote values to double and use BitConverter.DoubleToInt64Bits,
            // as there's no BitConverter.SingleToInt32Bits, unfortunately.

            public override bool Equals(float x, float y) =>
                BitConverter.DoubleToInt64Bits(x) == BitConverter.DoubleToInt64Bits(y);

            public override int GetHashCode(float obj) =>
                BitConverter.DoubleToInt64Bits(obj).GetHashCode();
        }

        private class BitwiseNullableDoubleEqualityComparerImpl : EqualityComparer<double?>
        {
            public override bool Equals(double? x, double? y) =>
                x == null && y == null ? true
                : x == null || y == null ? false
                : BitwiseDoubleEqualityComparer.Equals(x.Value, y.Value);

            // The hash code for null is just a constant which is at least *unlikely* to be used
            // elsewhere. (Compared with 0, say.)
            public override int GetHashCode(double? obj) =>
                obj == null ? 293864 : BitwiseDoubleEqualityComparer.GetHashCode(obj.Value);
        }

        private class BitwiseNullableSingleEqualityComparerImpl : EqualityComparer<float?>
        {
            public override bool Equals(float? x, float? y) =>
                x == null && y == null ? true
                : x == null || y == null ? false
                : BitwiseSingleEqualityComparer.Equals(x.Value, y.Value);

            // The hash code for null is just a constant which is at least *unlikely* to be used
            // elsewhere. (Compared with 0, say.)
            public override int GetHashCode(float? obj) =>
                obj == null ? 293864 : BitwiseSingleEqualityComparer.GetHashCode(obj.Value);
        }
    }
}
