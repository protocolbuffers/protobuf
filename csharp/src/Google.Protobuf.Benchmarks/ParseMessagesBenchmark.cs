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
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Buffers;
using Google.Protobuf.WellKnownTypes;
using Benchmarks.Proto3;

namespace Google.Protobuf.Benchmarks
{
    /// <summary>
    /// Benchmark that tests parsing performance for various messages.
    /// </summary>
    [MemoryDiagnoser]
    public class ParseMessagesBenchmark
    {
        const int MaxMessages = 100;

        SubTest manyWrapperFieldsTest = new SubTest(CreateManyWrapperFieldsMessage(), ManyWrapperFieldsMessage.Parser, () => new ManyWrapperFieldsMessage(), MaxMessages);
        SubTest manyPrimitiveFieldsTest = new SubTest(CreateManyPrimitiveFieldsMessage(), ManyPrimitiveFieldsMessage.Parser, () => new ManyPrimitiveFieldsMessage(), MaxMessages);
        SubTest repeatedFieldTest = new SubTest(CreateRepeatedFieldMessage(), GoogleMessage1.Parser, () => new GoogleMessage1(), MaxMessages);
        SubTest emptyMessageTest = new SubTest(new Empty(), Empty.Parser, () => new Empty(), MaxMessages);

        public IEnumerable<int> MessageCountValues => new[] { 10, 100 };

        [GlobalSetup]
        public void GlobalSetup()
        {
        }

        [Benchmark]
        public IMessage ManyWrapperFieldsMessage_ParseFromByteArray()
        {
            return manyWrapperFieldsTest.ParseFromByteArray();
        }

        [Benchmark]
        public IMessage ManyWrapperFieldsMessage_ParseFromReadOnlySequence()
        {
            return manyWrapperFieldsTest.ParseFromReadOnlySequence();
        }

        [Benchmark]
        public IMessage ManyPrimitiveFieldsMessage_ParseFromByteArray()
        {
            return manyPrimitiveFieldsTest.ParseFromByteArray();
        }

        [Benchmark]
        public IMessage ManyPrimitiveFieldsMessage_ParseFromReadOnlySequence()
        {
            return manyPrimitiveFieldsTest.ParseFromReadOnlySequence();
        }

        [Benchmark]
        public IMessage RepeatedFieldMessage_ParseFromByteArray()
        {
            return repeatedFieldTest.ParseFromByteArray();
        }

        [Benchmark]
        public IMessage RepeatedFieldMessage_ParseFromReadOnlySequence()
        {
            return repeatedFieldTest.ParseFromReadOnlySequence();
        }

        [Benchmark]
        public IMessage EmptyMessage_ParseFromByteArray()
        {
            return emptyMessageTest.ParseFromByteArray();
        }

        [Benchmark]
        public IMessage EmptyMessage_ParseFromReadOnlySequence()
        {
            return emptyMessageTest.ParseFromReadOnlySequence();
        }

        [Benchmark]
        [ArgumentsSource(nameof(MessageCountValues))]
        public void ManyWrapperFieldsMessage_ParseDelimitedMessagesFromByteArray(int messageCount)
        {
            manyWrapperFieldsTest.ParseDelimitedMessagesFromByteArray(messageCount);
        }

        [Benchmark]
        [ArgumentsSource(nameof(MessageCountValues))]
        public void ManyWrapperFieldsMessage_ParseDelimitedMessagesFromReadOnlySequence(int messageCount)
        {
            manyWrapperFieldsTest.ParseDelimitedMessagesFromReadOnlySequence(messageCount);
        }

        [Benchmark]
        [ArgumentsSource(nameof(MessageCountValues))]
        public void ManyPrimitiveFieldsMessage_ParseDelimitedMessagesFromByteArray(int messageCount)
        {
            manyPrimitiveFieldsTest.ParseDelimitedMessagesFromByteArray(messageCount);
        }

        [Benchmark]
        [ArgumentsSource(nameof(MessageCountValues))]
        public void ManyPrimitiveFieldsMessage_ParseDelimitedMessagesFromReadOnlySequence(int messageCount)
        {
            manyPrimitiveFieldsTest.ParseDelimitedMessagesFromReadOnlySequence(messageCount);
        }

        [Benchmark]
        [ArgumentsSource(nameof(MessageCountValues))]
        public void RepeatedFieldMessage_ParseDelimitedMessagesFromByteArray(int messageCount)
        {
            repeatedFieldTest.ParseDelimitedMessagesFromByteArray(messageCount);
        }

        [Benchmark]
        [ArgumentsSource(nameof(MessageCountValues))]
        public void RepeatedFieldMessage_ParseDelimitedMessagesFromReadOnlySequence(int messageCount)
        {
            repeatedFieldTest.ParseDelimitedMessagesFromReadOnlySequence(messageCount);
        }

        public static ManyWrapperFieldsMessage CreateManyWrapperFieldsMessage()
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

        public static ManyPrimitiveFieldsMessage CreateManyPrimitiveFieldsMessage()
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

        public static GoogleMessage1 CreateRepeatedFieldMessage()
        {
            // Message with a repeated fixed length item collection
            var message = new GoogleMessage1();
            for (ulong i = 0; i < 1000; i++)
            {
                message.Field5.Add(i);
            }
            return message;
        }

        private class SubTest
        {
            private readonly IMessage message;
            private readonly MessageParser parser;
            private readonly Func<IMessage> factory;
            private readonly byte[] data;
            private readonly byte[] multipleMessagesData;

            private ReadOnlySequence<byte> dataSequence;
            private ReadOnlySequence<byte> multipleMessagesDataSequence;

            public SubTest(IMessage message, MessageParser parser, Func<IMessage> factory, int maxMessageCount)
            {
                this.message = message;
                this.parser = parser;
                this.factory = factory;
                this.data = message.ToByteArray();
                this.multipleMessagesData = CreateBufferWithMultipleMessages(message, maxMessageCount);
                this.dataSequence = new ReadOnlySequence<byte>(this.data);
                this.multipleMessagesDataSequence = new ReadOnlySequence<byte>(this.multipleMessagesData);
            }

            public IMessage ParseFromByteArray() => parser.ParseFrom(data);

            public IMessage ParseFromReadOnlySequence() => parser.ParseFrom(dataSequence);

            public void ParseDelimitedMessagesFromByteArray(int messageCount)
            {
                var input = new CodedInputStream(multipleMessagesData);
                for (int i = 0; i < messageCount; i++)
                {
                    var msg = factory();
                    input.ReadMessage(msg);
                }
            }

            public void ParseDelimitedMessagesFromReadOnlySequence(int messageCount)
            {
                ParseContext.Initialize(multipleMessagesDataSequence, out ParseContext ctx);
                for (int i = 0; i < messageCount; i++)
                {
                    var msg = factory();
                    ctx.ReadMessage(msg);
                }
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
}
