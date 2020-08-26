#region Copyright notice and license
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
#endregion

using Google.Protobuf.WellKnownTypes;
using NUnit.Framework;

namespace Google.Protobuf
{
    public class Decimal128Test
    {
        [Test]
        public void LargeValue()
        {
            decimal d = (long)uint.MaxValue + 1;

            var d64 = Decimal128.FromDecimal(d);
            //Assert.AreEqual((long)uint.MaxValue + 1, d64.Significand);
            Assert.AreEqual(0, d64.Exponent);

            var d2 = d64.ToDecimal();
            Assert.AreEqual(d, d2);
        }

        [Test]
        public void Decimal128_MaxValue()
        {
            decimal d = 100000000000000000 - 1;

            var d64 = Decimal128.FromDecimal(d);
            //Assert.AreEqual(100000000000000000 - 1, d64.Significand);
            Assert.AreEqual(0, d64.Exponent);

            var d2 = d64.ToDecimal();
            Assert.AreEqual(d, d2);
        }

        [Test]
        public void Decimal128_MinValue()
        {
            decimal d = -100000000000000000 + 1;

            var d64 = Decimal128.FromDecimal(d);
            //Assert.AreEqual(-100000000000000000 + 1, d64.Significand);
            Assert.AreEqual(0, d64.Exponent);

            var d2 = d64.ToDecimal();
            Assert.AreEqual(d, d2);
        }

        [Test]
        public void Decimal_MaxValue()
        {
            decimal d = decimal.MaxValue;

            var d64 = Decimal128.FromDecimal(d);
            //Assert.AreEqual(100000000000000000 - 1, d64.Significand);
            Assert.AreEqual(0, d64.Exponent);

            var d2 = d64.ToDecimal();
            Assert.AreEqual(d, d2);
        }

        [Test]
        public void Decimal_MinValue()
        {
            decimal d = decimal.MinValue;

            var d64 = Decimal128.FromDecimal(d);
            //Assert.AreEqual(-100000000000000000 + 1, d64.Significand);
            Assert.AreEqual(0, d64.Exponent);

            var d2 = d64.ToDecimal();
            Assert.AreEqual(d, d2);
        }

        [Test]
        public void NegativeOne()
        {
            decimal d = -1;

            var d64 = Decimal128.FromDecimal(d);
            Assert.AreEqual(1, d64.SignificandLow);
            Assert.AreEqual(0, d64.SignificandHigh);
            Assert.AreEqual(0, d64.Exponent);
            Assert.AreEqual(-1, d64.Sign);

            var d2 = d64.ToDecimal();
            Assert.AreEqual(d, d2);
        }

        [Test]
        public void NegativeDecimal()
        {
            decimal d = -1.1m;

            var d64 = Decimal128.FromDecimal(d);
            Assert.AreEqual(11, d64.SignificandLow);
            Assert.AreEqual(0, d64.SignificandHigh);
            Assert.AreEqual(-1, d64.Exponent);
            Assert.AreEqual(-1, d64.Sign);

            var d2 = d64.ToDecimal();
            Assert.AreEqual(d, d2);
        }

        [Test]
        public void PositiveDecimal()
        {
            decimal d = 1.1m;

            var d64 = Decimal128.FromDecimal(d);
            Assert.AreEqual(11, d64.SignificandLow);
            Assert.AreEqual(0, d64.SignificandHigh);
            Assert.AreEqual(1, d64.Exponent);
            Assert.AreEqual(1, d64.Sign);

            var d2 = d64.ToDecimal();
            Assert.AreEqual(d, d2);
        }
    }
}