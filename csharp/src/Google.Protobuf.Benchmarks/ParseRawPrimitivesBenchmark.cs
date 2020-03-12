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

        Random random = new Random(417384220);  // random but deterministic seed

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
        public int ParseRawVarint32(int encodedSize)
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
        [Arguments(6)]
        [Arguments(7)]
        [Arguments(8)]
        [Arguments(9)]
        [Arguments(10)]
        public long ParseRawVarint64(int encodedSize)
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
        public uint ParseFixed32()
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
        public ulong ParseFixed64()
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
        public float ParseRawFloat()
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
        public double ParseRawDouble()
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

        private static byte[] CreateBufferWithRandomVarints(Random random, int valueCount, int encodedSize, int paddingValueCount)
        {
            MemoryStream ms = new MemoryStream();
            CodedOutputStream cos = new CodedOutputStream(ms);
            for (int i = 0; i < valueCount + paddingValueCount; i++)
            {
                cos.WriteUInt64(RandomUnsignedVarint(random, encodedSize));
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
        private static ulong RandomUnsignedVarint(Random random, int encodedSize)
        {
            Span<byte> randomBytesBuffer = stackalloc byte[8];

            if (encodedSize < 1 || encodedSize > 10)
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
    }
}
