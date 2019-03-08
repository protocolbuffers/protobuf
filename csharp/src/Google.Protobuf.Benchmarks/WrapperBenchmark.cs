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
using System.Buffers;
using System.Collections.Generic;
using System.IO;
using System.Linq;

namespace Google.Protobuf.Benchmarks
{
    /// <summary>
    /// Benchmark that tests serialization/deserialization of wrapper fields.
    /// </summary>
    [MemoryDiagnoser]
    public class WrapperBenchmark
    {
        byte[] manyWrapperFieldsByteArray;
        ReadOnlySequence<byte> manyWrapperFieldsReadOnlySequence;
        byte[] manyPrimitiveFieldsByteArray;
        ReadOnlySequence<byte> manyPrimitiveFieldsReadOnlySequence;

        [GlobalSetup]
        public void GlobalSetup()
        {
            manyWrapperFieldsByteArray = CreateManyWrapperFieldsMessage().ToByteArray();
            manyWrapperFieldsReadOnlySequence = new ReadOnlySequence<byte>(manyWrapperFieldsByteArray);
            manyPrimitiveFieldsByteArray = CreateManyPrimitiveFieldsMessage().ToByteArray();
            manyPrimitiveFieldsReadOnlySequence = new ReadOnlySequence<byte>(manyPrimitiveFieldsByteArray);
        }

        [Benchmark]
        public ManyWrapperFieldsMessage ParseWrapperFieldsFromByteArray()
        {
            return ManyWrapperFieldsMessage.Parser.ParseFrom(manyWrapperFieldsByteArray);
        }

        [Benchmark]
        public ManyWrapperFieldsMessage ParseWrapperFieldsFromReadOnlySequence()
        {
            var input = new CodedInputReader(manyWrapperFieldsReadOnlySequence);
            return ManyWrapperFieldsMessage.Parser.ParseFrom(ref input);
        }

        [Benchmark]
        public ManyPrimitiveFieldsMessage ParsePrimitiveFieldsFromByteArray()
        {
            return ManyPrimitiveFieldsMessage.Parser.ParseFrom(manyPrimitiveFieldsByteArray);
        }

        [Benchmark]
        public ManyPrimitiveFieldsMessage ParsePrimitiveFieldsFromReadOnlySequence()
        {
            var input = new CodedInputReader(manyPrimitiveFieldsReadOnlySequence);
            return ManyPrimitiveFieldsMessage.Parser.ParseFrom(ref input);
        }

        private static ManyWrapperFieldsMessage CreateManyWrapperFieldsMessage()
        {
            // Example data match data of an internal benchmarks
            return new ManyWrapperFieldsMessage()
            {
                Int64Field19 = 123,
                Int64Field37 = 1000032,
                Int64Field26 = 3453524500,
                DoubleField79 = 1.2,
                DoubleField25 = 234,
                DoubleField9 = 123.3,
                DoubleField28 = 23,
                DoubleField7 = 234,
                DoubleField50 = 2.45
            };
        }

        private static ManyPrimitiveFieldsMessage CreateManyPrimitiveFieldsMessage()
        {
            // Example data match data of an internal benchmarks
            return new ManyPrimitiveFieldsMessage()
            {
                Int64Field19 = 123,
                Int64Field37 = 1000032,
                Int64Field26 = 3453524500,
                DoubleField79 = 1.2,
                DoubleField25 = 234,
                DoubleField9 = 123.3,
                DoubleField28 = 23,
                DoubleField7 = 234,
                DoubleField50 = 2.45
            };
        }
    }
}
