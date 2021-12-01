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

namespace Google.Protobuf.Benchmarks
{
    /// <summary>
    /// Benchmarks throughput when parsing raw primitives.
    /// </summary>
    [MemoryDiagnoser]
    public class ParseRawPrimitivesBenchmark
    {
        // key is the encodedSize of varint values
        Dictionary<int, byte[]> varintInputBuffers;

        byte[] doubleInputBuffer;
        byte[] floatInputBuffer;
        byte[] fixedIntInputBuffer;

        // key is the encodedSize of string values
        Dictionary<int, byte[]> stringInputBuffers;
        Dictionary<int, ReadOnlySequence<byte>> stringInputBuffersSegmented;

        Random random = new Random(417384220);  // random but deterministic seed

        public IEnumerable<int> StringEncodedSizes => new[] { 1, 4, 10, 105, 10080 };
        public IEnumerable<int> StringSegmentedEncodedSizes => new[] { 105, 10080 };

        [GlobalSetup]
        public void GlobalSetup()
        {
            // add some extra values that we won't read just to make sure we are far enough from the end of the buffer
            // which allows the parser fastpath to always kick in.
            const int paddingValueCount = 100;

            varintInputBuffers = new Dictionary<int, byte[]>();
            for (int encodedSize = 1; encodedSize <= 10; encodedSize++)
            {
                byte[] buffer = CreateBufferWithRandomVarints(random, BytesToParse / encodedSize, encodedSize, paddingValueCount);
                varintInputBuffers.Add(encodedSize, buffer);
            }

            doubleInputBuffer = CreateBufferWithRandomDoubles(random, BytesToParse / sizeof(double), paddingValueCount);
            floatInputBuffer = CreateBufferWithRandomFloats(random, BytesToParse / sizeof(float), paddingValueCount);
            fixedIntInputBuffer = CreateBufferWithRandomData(random, BytesToParse / sizeof(long), sizeof(long), paddingValueCount);

            stringInputBuffers = new Dictionary<int, byte[]>();
            foreach (var encodedSize in StringEncodedSizes)
            {
                byte[] buffer = CreateBufferWithStrings(BytesToParse / encodedSize, encodedSize, encodedSize < 10 ? 10 : 1 );
                stringInputBuffers.Add(encodedSize, buffer);
            }

            stringInputBuffersSegmented = new Dictionary<int, ReadOnlySequence<byte>>();
            foreach (var encodedSize in StringSegmentedEncodedSizes)
            {
                byte[] buffer = CreateBufferWithStrings(BytesToParse / encodedSize, encodedSize, encodedSize < 10 ? 10 : 1);
                stringInputBuffersSegmented.Add(encodedSize, ReadOnlySequenceFactory.CreateWithContent(buffer, segmentSize: 128, addEmptySegmentDelimiters: false));
            }
        }

        // Total number of bytes that each benchmark will parse.
        // Measuring the time taken to parse buffer of given size makes it easier to compare parsing speed for different
        // types and makes it easy to calculate the througput (in MB/s)
        // 10800 bytes is chosen because it is divisible by all possible encoded sizes for all primitive types {1..10}
        [Params(10080)]
        public int BytesToParse { get; set; }

        [Benchmark]
        [Arguments(1)]
        [Arguments(2)]
        [Arguments(3)]
        [Arguments(4)]
        [Arguments(5)]
        public int ParseRawVarint32_CodedInputStream(int encodedSize)
        {
            CodedInputStream cis = new CodedInputStream(varintInputBuffers[encodedSize]);
            int sum = 0;
            for (int i = 0; i < BytesToParse / encodedSize; i++)
            {
                sum += cis.ReadInt32();
            }
            return sum;
        }

        [Benchmark]
        [Arguments(1)]
        [Arguments(2)]
        [Arguments(3)]
        [Arguments(4)]
        [Arguments(5)]
        public int ParseRawVarint32_ParseContext(int encodedSize)
        {
            InitializeParseContext(varintInputBuffers[encodedSize], out ParseContext ctx);
            int sum = 0;
            for (int i = 0; i < BytesToParse / encodedSize; i++)
            {
                sum += ctx.ReadInt32();
            }
            return sum;
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
        public long ParseRawVarint64_CodedInputStream(int encodedSize)
        {
            CodedInputStream cis = new CodedInputStream(varintInputBuffers[encodedSize]);
            long sum = 0;
            for (int i = 0; i < BytesToParse / encodedSize; i++)
            {
                sum += cis.ReadInt64();
            }
            return sum;
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
        public long ParseRawVarint64_ParseContext(int encodedSize)
        {
            InitializeParseContext(varintInputBuffers[encodedSize], out ParseContext ctx);
            long sum = 0;
            for (int i = 0; i < BytesToParse / encodedSize; i++)
            {
                sum += ctx.ReadInt64();
            }
            return sum;
        }

        [Benchmark]
        public uint ParseFixed32_CodedInputStream()
        {
            const int encodedSize = sizeof(uint);
            CodedInputStream cis = new CodedInputStream(fixedIntInputBuffer);
            uint sum = 0;
            for (uint i = 0; i < BytesToParse / encodedSize; i++)
            {
                sum += cis.ReadFixed32();
            }
            return sum;
        }

        [Benchmark]
        public uint ParseFixed32_ParseContext()
        {
            const int encodedSize = sizeof(uint);
            InitializeParseContext(fixedIntInputBuffer, out ParseContext ctx);
            uint sum = 0;
            for (uint i = 0; i < BytesToParse / encodedSize; i++)
            {
                sum += ctx.ReadFixed32();
            }
            return sum;
        }

        [Benchmark]
        public ulong ParseFixed64_CodedInputStream()
        {
            const int encodedSize = sizeof(ulong);
            CodedInputStream cis = new CodedInputStream(fixedIntInputBuffer);
            ulong sum = 0;
            for (int i = 0; i < BytesToParse / encodedSize; i++)
            {
                sum += cis.ReadFixed64();
            }
            return sum;
        }

        [Benchmark]
        public ulong ParseFixed64_ParseContext()
        {
            const int encodedSize = sizeof(ulong);
            InitializeParseContext(fixedIntInputBuffer, out ParseContext ctx);
            ulong sum = 0;
            for (int i = 0; i < BytesToParse / encodedSize; i++)
            {
                sum += ctx.ReadFixed64();
            }
            return sum;
        }

        [Benchmark]
        public float ParseRawFloat_CodedInputStream()
        {
            const int encodedSize = sizeof(float);
            CodedInputStream cis = new CodedInputStream(floatInputBuffer);
            float sum = 0;
            for (int i = 0; i < BytesToParse / encodedSize; i++)
            {
               sum += cis.ReadFloat();
            }
            return sum;
        }

        [Benchmark]
        public float ParseRawFloat_ParseContext()
        {
            const int encodedSize = sizeof(float);
            InitializeParseContext(floatInputBuffer, out ParseContext ctx);
            float sum = 0;
            for (int i = 0; i < BytesToParse / encodedSize; i++)
            {
               sum += ctx.ReadFloat();
            }
            return sum;
        }

        [Benchmark]
        public double ParseRawDouble_CodedInputStream()
        {
            const int encodedSize = sizeof(double);
            CodedInputStream cis = new CodedInputStream(doubleInputBuffer);
            double sum = 0;
            for (int i = 0; i < BytesToParse / encodedSize; i++)
            {
                sum += cis.ReadDouble();
            }
            return sum;
        }

        [Benchmark]
        public double ParseRawDouble_ParseContext()
        {
            const int encodedSize = sizeof(double);
            InitializeParseContext(doubleInputBuffer, out ParseContext ctx);
            double sum = 0;
            for (int i = 0; i < BytesToParse / encodedSize; i++)
            {
                sum += ctx.ReadDouble();
            }
            return sum;
        }

        [Benchmark]
        [ArgumentsSource(nameof(StringEncodedSizes))]
        public int ParseString_CodedInputStream(int encodedSize)
        {
            CodedInputStream cis = new CodedInputStream(stringInputBuffers[encodedSize]);
            int sum = 0;
            for (int i = 0; i < BytesToParse / encodedSize; i++)
            {
                sum += cis.ReadString().Length;
            }
            return sum;
        }

        [Benchmark]
        [ArgumentsSource(nameof(StringEncodedSizes))]
        public int ParseString_ParseContext(int encodedSize)
        {
            InitializeParseContext(stringInputBuffers[encodedSize], out ParseContext ctx);
            int sum = 0;
            for (int i = 0; i < BytesToParse / encodedSize; i++)
            {
                sum += ctx.ReadString().Length;
            }
            return sum;
        }

        [Benchmark]
        [ArgumentsSource(nameof(StringSegmentedEncodedSizes))]
        public int ParseString_ParseContext_MultipleSegments(int encodedSize)
        {
            InitializeParseContext(stringInputBuffersSegmented[encodedSize], out ParseContext ctx);
            int sum = 0;
            for (int i = 0; i < BytesToParse / encodedSize; i++)
            {
                sum += ctx.ReadString().Length;
            }
            return sum;
        }

        [Benchmark]
        [ArgumentsSource(nameof(StringEncodedSizes))]
        public int ParseBytes_CodedInputStream(int encodedSize)
        {
            CodedInputStream cis = new CodedInputStream(stringInputBuffers[encodedSize]);
            int sum = 0;
            for (int i = 0; i < BytesToParse / encodedSize; i++)
            {
                sum += cis.ReadBytes().Length;
            }
            return sum;
        }

        [Benchmark]
        [ArgumentsSource(nameof(StringEncodedSizes))]
        public int ParseBytes_ParseContext(int encodedSize)
        {
            InitializeParseContext(stringInputBuffers[encodedSize], out ParseContext ctx);
            int sum = 0;
            for (int i = 0; i < BytesToParse / encodedSize; i++)
            {
                sum += ctx.ReadBytes().Length;
            }
            return sum;
        }

        [Benchmark]
        [ArgumentsSource(nameof(StringSegmentedEncodedSizes))]
        public int ParseBytes_ParseContext_MultipleSegments(int encodedSize)
        {
            InitializeParseContext(stringInputBuffersSegmented[encodedSize], out ParseContext ctx);
            int sum = 0;
            for (int i = 0; i < BytesToParse / encodedSize; i++)
            {
                sum += ctx.ReadBytes().Length;
            }
            return sum;
        }

        private static void InitializeParseContext(byte[] buffer, out ParseContext ctx)
        {
            ParseContext.Initialize(new ReadOnlySequence<byte>(buffer), out ctx);
        }

        private static void InitializeParseContext(ReadOnlySequence<byte> buffer, out ParseContext ctx)
        {
            ParseContext.Initialize(buffer, out ctx);
        }

        private static byte[] CreateBufferWithRandomVarints(Random random, int valueCount, int encodedSize, int paddingValueCount)
        {
            MemoryStream ms = new MemoryStream();
            CodedOutputStream cos = new CodedOutputStream(ms);
            for (int i = 0; i < valueCount + paddingValueCount; i++)
            {
                cos.WriteUInt64(RandomUnsignedVarint(random, encodedSize, false));
            }
            cos.Flush();
            var buffer = ms.ToArray();
            
            if (buffer.Length != encodedSize * (valueCount + paddingValueCount))
            {
                throw new InvalidOperationException($"Unexpected output buffer length {buffer.Length}"); 
            }
            return buffer;
        }

        private static byte[] CreateBufferWithRandomFloats(Random random, int valueCount, int paddingValueCount)
        {
            MemoryStream ms = new MemoryStream();
            CodedOutputStream cos = new CodedOutputStream(ms);
            for (int i = 0; i < valueCount + paddingValueCount; i++)
            {
                cos.WriteFloat((float)random.NextDouble());
            }
            cos.Flush();
            var buffer = ms.ToArray();
            return buffer;
        }

        private static byte[] CreateBufferWithRandomDoubles(Random random, int valueCount, int paddingValueCount)
        {
            MemoryStream ms = new MemoryStream();
            CodedOutputStream cos = new CodedOutputStream(ms);
            for (int i = 0; i < valueCount + paddingValueCount; i++)
            {
                cos.WriteDouble(random.NextDouble());
            }
            cos.Flush();
            var buffer = ms.ToArray();
            return buffer;
        }

        private static byte[] CreateBufferWithRandomData(Random random, int valueCount, int encodedSize, int paddingValueCount)
        {
            int bufferSize = (valueCount + paddingValueCount) * encodedSize;
            byte[] buffer = new byte[bufferSize];
            random.NextBytes(buffer);
            return buffer;
        }

        /// <summary>
        /// Generate a random value that will take exactly "encodedSize" bytes when varint-encoded.
        /// </summary>
        public static ulong RandomUnsignedVarint(Random random, int encodedSize, bool fitsIn32Bits)
        {
            Span<byte> randomBytesBuffer = stackalloc byte[8];

            if (encodedSize < 1 || encodedSize > 10 || (fitsIn32Bits && encodedSize > 5))
            {
                throw new ArgumentException("Illegal encodedSize value requested", nameof(encodedSize));
            }
            const int bitsPerByte = 7;
            
            ulong result = 0;
            while (true)
            {
                random.NextBytes(randomBytesBuffer);
                ulong randomValue = BinaryPrimitives.ReadUInt64LittleEndian(randomBytesBuffer);

                // only use the number of random bits we need
                ulong bitmask = encodedSize < 10 ? ((1UL << (encodedSize * bitsPerByte)) - 1) : ulong.MaxValue;
                result = randomValue & bitmask;

                if (fitsIn32Bits)
                {
                    // make sure the resulting value is representable by a uint.
                    result &= uint.MaxValue;
                }

                if (encodedSize == 10)
                {
                    // for 10-byte values the highest bit always needs to be set (7*9=63)
                    result |= ulong.MaxValue;
                    break;
                }

                // some random values won't require the full "encodedSize" bytes, check that at least
                // one of the top 7 bits is set. Retrying is fine since it only happens rarely
                if (encodedSize == 1 || (result & (0x7FUL << ((encodedSize - 1) * bitsPerByte))) != 0)
                {
                    break;
                }
            }
            return result;
        }

        private static byte[] CreateBufferWithStrings(int valueCount, int encodedSize, int paddingValueCount)
        {
            var str = CreateStringWithEncodedSize(encodedSize);

            MemoryStream ms = new MemoryStream();
            CodedOutputStream cos = new CodedOutputStream(ms);
            for (int i = 0; i < valueCount + paddingValueCount; i++)
            {
                cos.WriteString(str);
            }
            cos.Flush();
            var buffer = ms.ToArray();

            if (buffer.Length != encodedSize * (valueCount + paddingValueCount))
            {
                throw new InvalidOperationException($"Unexpected output buffer length {buffer.Length}");
            }
            return buffer;
        }

        public static string CreateStringWithEncodedSize(int encodedSize)
        {
            var str = new string('a', encodedSize);
            while (CodedOutputStream.ComputeStringSize(str) > encodedSize)
            {
                str = str.Substring(1);
            }

            if (CodedOutputStream.ComputeStringSize(str) != encodedSize)
            {
                throw new InvalidOperationException($"Generated string with wrong encodedSize");
            }
            return str;
        }

        public static string CreateNonAsciiStringWithEncodedSize(int encodedSize)
        {
            if (encodedSize < 3)
            {
                throw new ArgumentException("Illegal encoded size for a string with non-ascii chars.");
            }
            var twoByteChar = '\u00DC';  // U-umlaut, UTF8 encoding has 2 bytes
            var str = new string(twoByteChar, encodedSize / 2);
            while (CodedOutputStream.ComputeStringSize(str) > encodedSize)
            {
                str = str.Substring(1);
            }

            // add padding of ascii characters to reach the desired encoded size.
            while (CodedOutputStream.ComputeStringSize(str) < encodedSize)
            {
                str += 'a';
            }

            // Note that for a few specific encodedSize values, it might be impossible to generate
            // the string with the desired encodedSize using the algorithm above. For testing purposes, checking that
            // the encoded size we got is actually correct is good enough.
            if (CodedOutputStream.ComputeStringSize(str) != encodedSize)
            {
                throw new InvalidOperationException($"Generated string with wrong encodedSize");
            }
            return str;
        }
    }
}
