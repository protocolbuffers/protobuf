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
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Buffers;
using Google.Protobuf.WellKnownTypes;

namespace Google.Protobuf.Benchmarks
{
    /// <summary>
    /// Benchmark that tests parsing performance for various messages.
    /// </summary>
    [MemoryDiagnoser]
    public class ParseMessagesBenchmark
    {
        const int MaxMessages = 100;

        byte[] manyWrapperFieldsData;
        byte[] manyPrimitiveFieldsData;

        byte[] manyWrapperFieldsMultipleMessagesData;
        byte[] manyPrimitiveFieldsMultipleMessagesData;

        byte[] emptyData = new byte[0];

        public IEnumerable<int> MessageCountValues => new[] { 10, 100 };

        [GlobalSetup]
        public void GlobalSetup()
        {
            manyWrapperFieldsData = CreateManyWrapperFieldsMessage().ToByteArray();
            manyPrimitiveFieldsData = CreateManyPrimitiveFieldsMessage().ToByteArray();
            manyWrapperFieldsMultipleMessagesData = CreateBufferWithMultipleMessages(CreateManyWrapperFieldsMessage(), MaxMessages);
            manyPrimitiveFieldsMultipleMessagesData = CreateBufferWithMultipleMessages(CreateManyPrimitiveFieldsMessage(), MaxMessages);
        }

        [Benchmark]
        public ManyWrapperFieldsMessage ManyWrapperFieldsMessage_ParseFromByteArray()
        {
            return ManyWrapperFieldsMessage.Parser.ParseFrom(manyWrapperFieldsData);
        }

        [Benchmark]
        public ManyPrimitiveFieldsMessage ManyPrimitiveFieldsMessage_ParseFromByteArray()
        {
            return ManyPrimitiveFieldsMessage.Parser.ParseFrom(manyPrimitiveFieldsData);
        }

        [Benchmark]
        public Empty EmptyMessage_ParseFromByteArray()
        {
            return Empty.Parser.ParseFrom(emptyData);
        }

        [Benchmark]
        [ArgumentsSource(nameof(MessageCountValues))]
        public long ManyWrapperFieldsMessage_ParseDelimitedMessagesFromByteArray(int messageCount)
        {
            long sum = 0;
            var input = new CodedInputStream(manyWrapperFieldsMultipleMessagesData);
            for (int i = 0; i < messageCount; i++)
            {
                var msg = new ManyWrapperFieldsMessage();
                input.ReadMessage(msg);
                sum += msg.Int64Field19.Value;
            }
            return sum;
        }

        [Benchmark]
        [ArgumentsSource(nameof(MessageCountValues))]
        public long ManyPrimitiveFieldsMessage_ParseDelimitedMessagesFromByteArray(int messageCount)
        {
            long sum = 0;
            var input = new CodedInputStream(manyPrimitiveFieldsMultipleMessagesData);
            for (int i = 0; i < messageCount; i++)
            {
                var msg = new ManyPrimitiveFieldsMessage();
                input.ReadMessage(msg);
                sum += msg.Int64Field19;
            }
            return sum;
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

        private static byte[] CreateBufferWithMultipleMessages(IMessage msg, int msgCount)
        {
            var ms = new MemoryStream();
            var cos = new CodedOutputStream(ms);
            for (int i = 0; i < msgCount; i++)
            {
                cos.WriteMessage(msg);
            }
            cos.Flush();
            return ms.ToArray();
        }
    }
}