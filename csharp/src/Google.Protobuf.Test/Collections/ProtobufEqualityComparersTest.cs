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

using NUnit.Framework;
using System.Collections.Generic;
using System.Linq;
using static Google.Protobuf.Collections.ProtobufEqualityComparers;

namespace Google.Protobuf.Collections
{
    public class ProtobufEqualityComparersTest
    {
        private static readonly double[] doubles =
        {
            0,
            1,
            1.5,
            -1.5,
            double.PositiveInfinity,
            double.NegativeInfinity,
            // Three different types of NaN...
            SampleNaNs.Regular,
            SampleNaNs.SignallingFlipped,
            SampleNaNs.PayloadFlipped
        };

        [Test]
        public void GetEqualityComparer_Default()
        {
            // It's more pain than it's worth to try to parameterize these tests.
            Assert.AreSame(EqualityComparer<object>.Default, GetEqualityComparer<object>());
            Assert.AreSame(EqualityComparer<string>.Default, GetEqualityComparer<string>());
            Assert.AreSame(EqualityComparer<int>.Default, GetEqualityComparer<int>());
            Assert.AreSame(EqualityComparer<int?>.Default, GetEqualityComparer<int?>());
        }

        [Test]
        public void GetEqualityComparer_NotDefault()
        {
            // It's more pain than it's worth to try to parameterize these tests.
            Assert.AreSame(BitwiseDoubleEqualityComparer, GetEqualityComparer<double>());
            Assert.AreSame(BitwiseSingleEqualityComparer, GetEqualityComparer<float>());
            Assert.AreSame(BitwiseNullableDoubleEqualityComparer, GetEqualityComparer<double?>());
            Assert.AreSame(BitwiseNullableSingleEqualityComparer, GetEqualityComparer<float?>());
        }

        [Test]
        public void DoubleComparisons()
        {
            ValidateEqualityComparer(BitwiseDoubleEqualityComparer, doubles);
        }

        [Test]
        public void NullableDoubleComparisons()
        {
            ValidateEqualityComparer(BitwiseNullableDoubleEqualityComparer, doubles.Select(d => (double?) d).Concat(new double?[] { null }));
        }

        [Test]
        public void SingleComparisons()
        {
            ValidateEqualityComparer(BitwiseSingleEqualityComparer, doubles.Select(d => (float) d));
        }

        [Test]
        public void NullableSingleComparisons()
        {
            ValidateEqualityComparer(BitwiseNullableSingleEqualityComparer, doubles.Select(d => (float?) d).Concat(new float?[] { null }));
        }

        private static void ValidateEqualityComparer<T>(EqualityComparer<T> comparer, IEnumerable<T> values)
        {
            var array = values.ToArray();
            // Each value should be equal to itself, but not to any other value.
            for (int i = 0; i < array.Length; i++)
            {
                for (int j = 0; j < array.Length; j++)
                {
                    if (i == j)
                    {
                        Assert.IsTrue(comparer.Equals(array[i], array[j]),
                            "{0} should be equal to itself", array[i], array[j]);
                    }
                    else
                    {
                        Assert.IsFalse(comparer.Equals(array[i], array[j]),
                            "{0} and {1} should not be equal", array[i], array[j]);
                        Assert.AreNotEqual(comparer.GetHashCode(array[i]), comparer.GetHashCode(array[j]),
                            "Hash codes for {0} and {1} should not be equal", array[i], array[j]);
                    }
                }
            }
        }
    }    
}
