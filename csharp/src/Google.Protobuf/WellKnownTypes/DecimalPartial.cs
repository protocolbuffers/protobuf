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
    public partial class Decimal64
    {
        public static Decimal64 FromDecimal(decimal d)
        {
            if (d == decimal.Zero)
            {
                return new Decimal64();
            }

            // TODO(JamesNK): Can multitarget to net5.0 and use overload that uses
            // Span to avoid byte array allocation.
            var bits = decimal.GetBits(d);

            uint sign = (uint)bits[3] >> 31;
            uint scale = ((uint)bits[3] >> 16) & 31;

            const ulong max = 100000000000000000;

            if (bits[2] != 0)
            {
                throw new ArgumentException($"Decimal value {d} doesn't fit in Decimal64");
            }

            var significand = ((ulong)bits[1] << 32) + (uint)bits[0];

            // Exactly min/max mean positive/negative infinity.
            // System.Decimal doesn't have that concept so throw for those values.
            if (significand >= max)
            {
                throw new ArgumentException($"Decimal value {d} doesn't fit in Decimal64");
            }

            Decimal64 decimal64 = new Decimal64
            {
                Significand = sign == 0 ? (long)significand : -(long)significand,
                Exponent = sign == 0 ? (int)scale : -(int)scale
            };
            return decimal64;
        }

        public decimal ToDecimal()
        {
            var significand = (ulong)Math.Abs(Significand);
            var mid = (int)((significand & 0xFFFFFFFF00000000L) >> 32);
            var low = (int)(significand & 0xffffffff);

            // TODO(JamesNK): Validate exponent is not too large
            return new decimal(low, mid, hi: 0, Significand < 0, (byte)Math.Abs(Exponent));
        }
    }

    public partial class Decimal128
    {
        public static Decimal128 FromDecimal(decimal d)
        {
            if (d == decimal.Zero)
            {
                return new Decimal128();
            }

            // TODO(JamesNK): Can multitarget to net5.0 and use overload that uses
            // Span to avoid byte array allocation.
            var bits = decimal.GetBits(d);

            uint sign = (uint)bits[3] >> 31;
            uint scale = ((uint)bits[3] >> 16) & 31;

            var significandLow = ((ulong)bits[1] << 32) + (uint)bits[0];

            Decimal128 decimal128 = new Decimal128
            {
                SignificandLow = significandLow,
                SignificandHigh = (ulong)bits[2],
                Sign = sign == 1 ? -1 : 1,
                Exponent = sign == 0 ? (int)scale : -(int)scale,
            };
            return decimal128;
        }

        public decimal ToDecimal()
        {
            // TODO(JamesNK): Validate Decimal128 is not too large
            var hi = (int)SignificandHigh;
            var mid = (int)((SignificandLow & 0xFFFFFFFF00000000L) >> 32);
            var lo = (int)(SignificandLow & 0xffffffff);

            // TODO(JamesNK): Validate exponent is not too large
            return new decimal(lo, mid, hi, Sign == -1, (byte)Math.Abs(Exponent));
        }
    }
}
