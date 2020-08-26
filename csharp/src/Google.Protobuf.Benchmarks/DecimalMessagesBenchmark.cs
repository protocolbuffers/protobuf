#region Copyright notice and license
// Protocol Buffers - Google's data interchange format
// Copyright 2019 Google Inc.  All rights reserved.
// https://github.com/protocolbuffers/protobuf
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
using System.Buffers;
using BenchmarkDotNet.Attributes;
using Google.Protobuf.WellKnownTypes;

namespace Google.Protobuf.Benchmarks
{
    [MemoryDiagnoser]
    public class DecimalMessagesBenchmark
    {
        byte[] buffer = new byte[1024];

        [GlobalSetup]
        public void GlobalSetup()
        {
        }

        [Benchmark]
        public decimal DecimalString_Zero()
        {
            return DecimalStringBufferRoundtrip(decimal.Zero);
        }

        [Benchmark]
        public decimal DecimalString_MaxValue()
        {
            return DecimalStringBufferRoundtrip(decimal.MaxValue);
        }

        [Benchmark]
        public decimal Decimal64_Zero()
        {
            return Decimal64BufferRoundtrip(decimal.Zero);
        }

        [Benchmark]
        public decimal Decimal64_MaxValue()
        {
            return Decimal64BufferRoundtrip(100000000000000000 - 1);
        }

        [Benchmark]
        public decimal Decimal128_Zero()
        {
            return Decimal128BufferRoundtrip(decimal.Zero);
        }

        [Benchmark]
        public decimal Decimal128_MaxValue()
        {
            return Decimal128BufferRoundtrip(decimal.MaxValue);
        }

        private decimal DecimalStringBufferRoundtrip(decimal d)
        {
            var m1 = new DecimalString { Value = d.ToString() };

            var span = buffer.AsSpan(0, m1.CalculateSize());  // use pre-existing output buffer
            m1.WriteTo(span);

            var m2 = DecimalString.Parser.ParseFrom(new ReadOnlySequence<byte>(buffer, 0, span.Length));

            return decimal.Parse(m2.Value);
        }

        private decimal Decimal64BufferRoundtrip(decimal d)
        {
            var m1 = Decimal64.FromDecimal(d);

            var span = buffer.AsSpan(0, m1.CalculateSize());  // use pre-existing output buffer
            m1.WriteTo(span);

            var m2 = Decimal64.Parser.ParseFrom(new ReadOnlySequence<byte>(buffer, 0, span.Length));

            return m2.ToDecimal();
        }

        private decimal Decimal128BufferRoundtrip(decimal d)
        {
            var m1 = Decimal128.FromDecimal(d);

            var span = buffer.AsSpan(0, m1.CalculateSize());  // use pre-existing output buffer
            m1.WriteTo(span);

            var m2 = Decimal128.Parser.ParseFrom(new ReadOnlySequence<byte>(buffer, 0, span.Length));

            return m2.ToDecimal();
        }
    }
}
