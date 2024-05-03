#region Copyright notice and license
// Protocol Buffers - Google's data interchange format
// Copyright 2017 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd
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
