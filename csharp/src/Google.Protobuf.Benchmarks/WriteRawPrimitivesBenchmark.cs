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

using BenchmarkDotNet.Attributes;
using System;
using System.Buffers.Binary;
using System.Collections.Generic;
using System.IO;
using System.Buffers;
using System.Text;

namespace Google.Protobuf.Benchmarks
{
    /// <summary>
    /// Benchmarks throughput when writing raw primitives.
    /// </summary>
    [MemoryDiagnoser]
    public class WriteRawPrimitivesBenchmark
    {
        // key is the encodedSize of varint values
        Dictionary<int, uint[]> varint32Values;
        Dictionary<int, ulong[]> varint64Values;

        double[] doubleValues;
        float[] floatValues;

        // key is the encodedSize of string values
        Dictionary<int, string[]> stringValues;

        // key is the encodedSize of string values
        Dictionary<int, string[]> nonAsciiStringValues;

        // key is the encodedSize of string values
        Dictionary<int, ByteString[]> byteStringValues;

        // the buffer to which all the data will be written
        byte[] outputBuffer;

        Random random = new Random(417384220);  // random but deterministic seed

        public IEnumerable<int> StringEncodedSizes => new[] { 1, 4, 10, 105, 10080 };

        public IEnumerable<int> NonAsciiStringEncodedSizes => new[] { 4, 10, 105, 10080 };

        [GlobalSetup]
        public void GlobalSetup()
        {
            outputBuffer = new byte[BytesToWrite];

            varint32Values = new Dictionary<int, uint[]>();
            varint64Values = new Dictionary<int, ulong[]>();
            for (int encodedSize = 1; encodedSize <= 10; encodedSize++)
            {
                if (encodedSize <= 5)
                {
                    varint32Values.Add(encodedSize, CreateRandomVarints32(random, BytesToWrite / encodedSize, encodedSize));
                }
                varint64Values.Add(encodedSize, CreateRandomVarints64(random, BytesToWrite / encodedSize, encodedSize));
            }

            doubleValues = CreateRandomDoubles(random, BytesToWrite / sizeof(double));
            floatValues = CreateRandomFloats(random, BytesToWrite / sizeof(float));

            stringValues = new Dictionary<int, string[]>();

            byteStringValues = new Dictionary<int, ByteString[]>();
            foreach(var encodedSize in StringEncodedSizes)
            {
                stringValues.Add(encodedSize, CreateStrings(BytesToWrite / encodedSize, encodedSize));
                byteStringValues.Add(encodedSize, CreateByteStrings(BytesToWrite / encodedSize, encodedSize));
            }

            nonAsciiStringValues = new Dictionary<int, string[]>();
            foreach(var encodedSize in NonAsciiStringEncodedSizes)
            {
                nonAsciiStringValues.Add(encodedSize, CreateNonAsciiStrings(BytesToWrite / encodedSize, encodedSize));
            }
        }

        // Total number of bytes that each benchmark will write.
        // Measuring the time taken to write buffer of given size makes it easier to compare parsing speed for different
        // types and makes it easy to calculate the througput (in MB/s)
        // 10800 bytes is chosen because it is divisible by all possible encoded sizes for all primitive types {1..10}
        [Params(10080)]
        public int BytesToWrite { get; set; }

        [Benchmark]
        [Arguments(1)]
        [Arguments(2)]
        [Arguments(3)]
        [Arguments(4)]
        [Arguments(5)]
        public void WriteRawVarint32_CodedOutputStream(int encodedSize)
        {
            var values = varint32Values[encodedSize];
            var cos = new CodedOutputStream(outputBuffer);
            for (int i = 0; i < values.Length; i++)
            {
                cos.WriteRawVarint32(values[i]);
            }
            cos.Flush();
            cos.CheckNoSpaceLeft();
        }

        [Benchmark]
        [Arguments(1)]
        [Arguments(2)]
        [Arguments(3)]
        [Arguments(4)]
        [Arguments(5)]
        public void WriteRawVarint32_WriteContext(int encodedSize)
        {
            var values = varint32Values[encodedSize];
            var span = new Span<byte>(outputBuffer);
            WriteContext.Initialize(ref span, out WriteContext ctx);
            for (int i = 0; i < values.Length; i++)
            {
                ctx.WriteUInt32(values[i]);
            }
            ctx.Flush();
            ctx.CheckNoSpaceLeft();
        }

        [Benchmark]
        [Arguments(1)]
        [Arguments(2)]
        [Arguments(3)]
        [Arguments(4)]
        [Arguments(5)]
        [Arguments(6)]
        [Arguments(7)]
        [Arguments(8)]
        [Arguments(9)]
        [Arguments(10)]
        public void WriteRawVarint64_CodedOutputStream(int encodedSize)
        {
            var values = varint64Values[encodedSize];
            var cos = new CodedOutputStream(outputBuffer);
            for (int i = 0; i < values.Length; i++)
            {
                cos.WriteRawVarint64(values[i]);
            }
            cos.Flush();
            cos.CheckNoSpaceLeft();
        }

        [Benchmark]
        [Arguments(1)]
        [Arguments(2)]
        [Arguments(3)]
        [Arguments(4)]
        [Arguments(5)]
        [Arguments(6)]
        [Arguments(7)]
        [Arguments(8)]
        [Arguments(9)]
        [Arguments(10)]
        public void WriteRawVarint64_WriteContext(int encodedSize)
        {
            var values = varint64Values[encodedSize];
            var span = new Span<byte>(outputBuffer);
            WriteContext.Initialize(ref span, out WriteContext ctx);
            for (int i = 0; i < values.Length; i++)
            {
                ctx.WriteUInt64(values[i]);
            }
            ctx.Flush();
            ctx.CheckNoSpaceLeft();
        }

        [Benchmark]
        public void WriteFixed32_CodedOutputStream()
        {
            const int encodedSize = sizeof(uint);
            var cos = new CodedOutputStream(outputBuffer);
            for (int i = 0; i < BytesToWrite / encodedSize; i++)
            {
                cos.WriteFixed32(12345);
            }
            cos.Flush();
            cos.CheckNoSpaceLeft();
        }

        [Benchmark]
        public void WriteFixed32_WriteContext()
        {
            const int encodedSize = sizeof(uint);
            var span = new Span<byte>(outputBuffer);
            WriteContext.Initialize(ref span, out WriteContext ctx);
            for (uint i = 0; i < BytesToWrite / encodedSize; i++)
            {
                ctx.WriteFixed32(12345);
            }
            ctx.Flush();
            ctx.CheckNoSpaceLeft();
        }

        [Benchmark]
        public void WriteFixed64_CodedOutputStream()
        {
            const int encodedSize = sizeof(ulong);
            var cos = new CodedOutputStream(outputBuffer);
            for(int i = 0; i < BytesToWrite / encodedSize; i++)
            {
                cos.WriteFixed64(123456789);
            }
            cos.Flush();
            cos.CheckNoSpaceLeft();
        }

        [Benchmark]
        public void WriteFixed64_WriteContext()
        {
            const int encodedSize = sizeof(ulong);
            var span = new Span<byte>(outputBuffer);
            WriteContext.Initialize(ref span, out WriteContext ctx);
            for (uint i = 0; i < BytesToWrite / encodedSize; i++)
            {
                ctx.WriteFixed64(123456789);
            }
            ctx.Flush();
            ctx.CheckNoSpaceLeft();
        }

        [Benchmark]
        public void WriteRawTag_OneByte_WriteContext()
        {
            const int encodedSize = 1;
            var span = new Span<byte>(outputBuffer);
            WriteContext.Initialize(ref span, out WriteContext ctx);
            for (uint i = 0; i < BytesToWrite / encodedSize; i++)
            {
                ctx.WriteRawTag(16);
            }
            ctx.Flush();
            ctx.CheckNoSpaceLeft();
        }

        [Benchmark]
        public void WriteRawTag_TwoBytes_WriteContext()
        {
            const int encodedSize = 2;
            var span = new Span<byte>(outputBuffer);
            WriteContext.Initialize(ref span, out WriteContext ctx);
            for (uint i = 0; i < BytesToWrite / encodedSize; i++)
            {
                ctx.WriteRawTag(137, 6);
            }
            ctx.Flush();
            ctx.CheckNoSpaceLeft();
        }

        [Benchmark]
        public void WriteRawTag_ThreeBytes_WriteContext()
        {
            const int encodedSize = 3;
            var span = new Span<byte>(outputBuffer);
            WriteContext.Initialize(ref span, out WriteContext ctx);
            for (uint i = 0; i < BytesToWrite / encodedSize; i++)
            {
                ctx.WriteRawTag(160, 131, 1);
            }
            ctx.Flush();
            ctx.CheckNoSpaceLeft();
        }

        [Benchmark]
        public void Baseline_WriteContext()
        {
            var span = new Span<byte>(outputBuffer);
            WriteContext.Initialize(ref span, out WriteContext ctx);
            ctx.state.position = outputBuffer.Length;
            ctx.Flush();
            ctx.CheckNoSpaceLeft();
        }

        [Benchmark]
        public void WriteRawFloat_CodedOutputStream()
        {
            var cos = new CodedOutputStream(outputBuffer);
            foreach (var value in floatValues)
            {
                cos.WriteFloat(value);
            }
            cos.Flush();
            cos.CheckNoSpaceLeft();
        }

        [Benchmark]
        public void WriteRawFloat_WriteContext()
        {
            var span = new Span<byte>(outputBuffer);
            WriteContext.Initialize(ref span, out WriteContext ctx);
            foreach (var value in floatValues)
            {
                ctx.WriteFloat(value);
            }
            ctx.Flush();
            ctx.CheckNoSpaceLeft();
        }

        [Benchmark]
        public void WriteRawDouble_CodedOutputStream()
        {
            var cos = new CodedOutputStream(outputBuffer);
            foreach (var value in doubleValues)
            {
                cos.WriteDouble(value);
            }
            cos.Flush();
            cos.CheckNoSpaceLeft();
        }

        [Benchmark]
        public void WriteRawDouble_WriteContext()
        {
            var span = new Span<byte>(outputBuffer);
            WriteContext.Initialize(ref span, out WriteContext ctx);
            foreach (var value in doubleValues)
            {
                ctx.WriteDouble(value);
            }
            ctx.Flush();
            ctx.CheckNoSpaceLeft();
        }

        [Benchmark]
        [ArgumentsSource(nameof(StringEncodedSizes))]
        public void WriteString_CodedOutputStream(int encodedSize)
        {
            var values = stringValues[encodedSize];
            var cos = new CodedOutputStream(outputBuffer);
            foreach (var value in values)
            {
                cos.WriteString(value);
            }
            cos.Flush();
            cos.CheckNoSpaceLeft();
        }

        [Benchmark]
        [ArgumentsSource(nameof(StringEncodedSizes))]
        public void WriteString_WriteContext(int encodedSize)
        {
            var values = stringValues[encodedSize];
            var span = new Span<byte>(outputBuffer);
            WriteContext.Initialize(ref span, out WriteContext ctx);
            foreach (var value in values)
            {
                ctx.WriteString(value);
            }
            ctx.Flush();
            ctx.CheckNoSpaceLeft();
        }

        [Benchmark]
        [ArgumentsSource(nameof(NonAsciiStringEncodedSizes))]
        public void WriteNonAsciiString_CodedOutputStream(int encodedSize)
        {
            var values = nonAsciiStringValues[encodedSize];
            var cos = new CodedOutputStream(outputBuffer);
            foreach (var value in values)
            {
                cos.WriteString(value);
            }
            cos.Flush();
            cos.CheckNoSpaceLeft();
        }

        [Benchmark]
        [ArgumentsSource(nameof(NonAsciiStringEncodedSizes))]
        public void WriteNonAsciiString_WriteContext(int encodedSize)
        {
            var values = nonAsciiStringValues[encodedSize];
            var span = new Span<byte>(outputBuffer);
            WriteContext.Initialize(ref span, out WriteContext ctx);
            foreach (var value in values)
            {
                ctx.WriteString(value);
            }
            ctx.Flush();
            ctx.CheckNoSpaceLeft();
        }

        [Benchmark]
        [ArgumentsSource(nameof(StringEncodedSizes))]
        public void WriteBytes_CodedOutputStream(int encodedSize)
        {
            var values = byteStringValues[encodedSize];
            var cos = new CodedOutputStream(outputBuffer);
            foreach (var value in values)
            {
                cos.WriteBytes(value);
            }
            cos.Flush();
            cos.CheckNoSpaceLeft();
        }

        [Benchmark]
        [ArgumentsSource(nameof(StringEncodedSizes))]
        public void WriteBytes_WriteContext(int encodedSize)
        {
            var values = byteStringValues[encodedSize];
            var span = new Span<byte>(outputBuffer);
            WriteContext.Initialize(ref span, out WriteContext ctx);
            foreach (var value in values)
            {
                ctx.WriteBytes(value);
            }
            ctx.Flush();
            ctx.CheckNoSpaceLeft();
        }

        private static uint[] CreateRandomVarints32(Random random, int valueCount, int encodedSize)
        {
            var result = new uint[valueCount];
            for (int i = 0; i < valueCount; i++)
            {
                result[i] = (uint) ParseRawPrimitivesBenchmark.RandomUnsignedVarint(random, encodedSize, true);
            }
            return result;
        }

        private static ulong[] CreateRandomVarints64(Random random, int valueCount, int encodedSize)
        {            
            var result = new ulong[valueCount];
            for (int i = 0; i < valueCount; i++)
            {
                result[i] = ParseRawPrimitivesBenchmark.RandomUnsignedVarint(random, encodedSize, false);
            }
            return result;
        }

        private static float[] CreateRandomFloats(Random random, int valueCount)
        {
            var result = new float[valueCount];
            for (int i = 0; i < valueCount; i++)
            {
                result[i] = (float)random.NextDouble();
            }
            return result;
        }

        private static double[] CreateRandomDoubles(Random random, int valueCount)
        {
            var result = new double[valueCount];
            for (int i = 0; i < valueCount; i++)
            {
                result[i] = random.NextDouble();
            }
            return result;
        }

        private static string[] CreateStrings(int valueCount, int encodedSize)
        {
            var str = ParseRawPrimitivesBenchmark.CreateStringWithEncodedSize(encodedSize);

            var result = new string[valueCount];
            for (int i = 0; i < valueCount; i++)
            {
                result[i] = str;
            }
            return result;
        }

        private static string[] CreateNonAsciiStrings(int valueCount, int encodedSize)
        {
            var str = ParseRawPrimitivesBenchmark.CreateNonAsciiStringWithEncodedSize(encodedSize);

            var result = new string[valueCount];
            for (int i = 0; i < valueCount; i++)
            {
                result[i] = str;
            }
            return result;
        }

        private static ByteString[] CreateByteStrings(int valueCount, int encodedSize)
        {
            var str = ParseRawPrimitivesBenchmark.CreateStringWithEncodedSize(encodedSize);

            var result = new ByteString[valueCount];
            for (int i = 0; i < valueCount; i++)
            {
                result[i] = ByteString.CopyFrom(Encoding.UTF8.GetBytes(str));
            }
            return result;
        }
    }
}
