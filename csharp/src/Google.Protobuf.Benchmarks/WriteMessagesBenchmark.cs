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

namespace Google.Protobuf.Benchmarks
{
    /// <summary>
    /// Benchmark that tests writing performance for various messages.
    /// </summary>
    [MemoryDiagnoser]
    public class WriteMessagesBenchmark
    {
        const int MaxMessages = 100;

        SubTest manyWrapperFieldsTest = new SubTest(ParseMessagesBenchmark.CreateManyWrapperFieldsMessage(), MaxMessages);
        SubTest manyPrimitiveFieldsTest = new SubTest(ParseMessagesBenchmark.CreateManyPrimitiveFieldsMessage(), MaxMessages);
        SubTest emptyMessageTest = new SubTest(new Empty(), MaxMessages);

        public IEnumerable<int> MessageCountValues => new[] { 10, 100 };

        [GlobalSetup]
        public void GlobalSetup()
        {
        }

        [Benchmark]
        public byte[] ManyWrapperFieldsMessage_ToByteArray()
        {
            return manyWrapperFieldsTest.ToByteArray();
        }

        [Benchmark]
        public byte[] ManyWrapperFieldsMessage_WriteToCodedOutputStream()
        {
            return manyWrapperFieldsTest.WriteToCodedOutputStream_PreAllocatedBuffer();
        }

        [Benchmark]
        public byte[] ManyWrapperFieldsMessage_WriteToSpan()
        {
            return manyWrapperFieldsTest.WriteToSpan_PreAllocatedBuffer();
        }


        [Benchmark]
        public byte[] ManyPrimitiveFieldsMessage_ToByteArray()
        {
            return manyPrimitiveFieldsTest.ToByteArray();
        }

        [Benchmark]
        public byte[] ManyPrimitiveFieldsMessage_WriteToCodedOutputStream()
        {
            return manyPrimitiveFieldsTest.WriteToCodedOutputStream_PreAllocatedBuffer();
        }

        [Benchmark]
        public byte[] ManyPrimitiveFieldsMessage_WriteToSpan()
        {
            return manyPrimitiveFieldsTest.WriteToSpan_PreAllocatedBuffer();
        }

        [Benchmark]
        public byte[] EmptyMessage_ToByteArray()
        {
            return emptyMessageTest.ToByteArray();
        }

        [Benchmark]
        public byte[] EmptyMessage_WriteToCodedOutputStream()
        {
            return emptyMessageTest.WriteToCodedOutputStream_PreAllocatedBuffer();
        }

        [Benchmark]
        public byte[] EmptyMessage_WriteToSpan()
        {
            return emptyMessageTest.WriteToSpan_PreAllocatedBuffer();
        }

        [Benchmark]
        [ArgumentsSource(nameof(MessageCountValues))]
        public void ManyWrapperFieldsMessage_WriteDelimitedMessagesToCodedOutputStream(int messageCount)
        {
            manyWrapperFieldsTest.WriteDelimitedMessagesToCodedOutputStream_PreAllocatedBuffer(messageCount);
        }

        [Benchmark]
        [ArgumentsSource(nameof(MessageCountValues))]
        public void ManyWrapperFieldsMessage_WriteDelimitedMessagesToSpan(int messageCount)
        {
            manyWrapperFieldsTest.WriteDelimitedMessagesToSpan_PreAllocatedBuffer(messageCount);
        }

        [Benchmark]
        [ArgumentsSource(nameof(MessageCountValues))]
        public void ManyPrimitiveFieldsMessage_WriteDelimitedMessagesToCodedOutputStream(int messageCount)
        {
            manyPrimitiveFieldsTest.WriteDelimitedMessagesToCodedOutputStream_PreAllocatedBuffer(messageCount);
        }

        [Benchmark]
        [ArgumentsSource(nameof(MessageCountValues))]
        public void ManyPrimitiveFieldsMessage_WriteDelimitedMessagesToSpan(int messageCount)
        {
            manyPrimitiveFieldsTest.WriteDelimitedMessagesToSpan_PreAllocatedBuffer(messageCount);
        }

        private class SubTest
        {
            private readonly IMessage message;
            private readonly byte[] outputBuffer;
            private readonly byte[] multipleMessagesOutputBuffer;

            public SubTest(IMessage message, int maxMessageCount)
            {
                this.message = message;

                int messageSize = message.CalculateSize();
                this.outputBuffer = new byte[messageSize];
                this.multipleMessagesOutputBuffer = new byte[maxMessageCount * (messageSize + CodedOutputStream.ComputeLengthSize(messageSize))];
            }

            public byte[] ToByteArray() => message.ToByteArray();

            public byte[] WriteToCodedOutputStream_PreAllocatedBuffer()
            {
                var cos = new CodedOutputStream(outputBuffer);  // use pre-existing output buffer
                message.WriteTo(cos);
                return outputBuffer;
            }

            public byte[] WriteToSpan_PreAllocatedBuffer()
            {
                var span = new Span<byte>(outputBuffer);  // use pre-existing output buffer
                message.WriteTo(span);
                return outputBuffer;
            }

            public byte[] WriteDelimitedMessagesToCodedOutputStream_PreAllocatedBuffer(int messageCount)
            {
                var cos = new CodedOutputStream(multipleMessagesOutputBuffer);  // use pre-existing output buffer
                for (int i = 0; i < messageCount; i++)
                {
                    cos.WriteMessage(message);
                }
                return multipleMessagesOutputBuffer;
            }

            public byte[] WriteDelimitedMessagesToSpan_PreAllocatedBuffer(int messageCount)
            {
                var span = new Span<byte>(multipleMessagesOutputBuffer);  // use pre-existing output buffer
                WriteContext.Initialize(ref span, out WriteContext ctx);
                for (int i = 0; i < messageCount; i++)
                {
                    ctx.WriteMessage(message);
                }
                return multipleMessagesOutputBuffer;
            }
        }
    }
}
