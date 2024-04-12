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

using NUnit.Framework;
using System;
using System.Buffers;
using Google.Protobuf.Buffers;

namespace Google.Protobuf
{
    public static class MessageParsingHelpers
    {
        public static void AssertReadingMessage<T>(MessageParser<T> parser, byte[] bytes, Action<T> assert) where T : IMessage<T>
        {
            var parsedMsg = parser.ParseFrom(bytes);
            assert(parsedMsg);

            // Load content as single segment
            parsedMsg = parser.ParseFrom(new ReadOnlySequence<byte>(bytes));
            assert(parsedMsg);

            // Load content as multiple segments
            parsedMsg = parser.ParseFrom(ReadOnlySequenceFactory.CreateWithContent(bytes));
            assert(parsedMsg);

            // Load content as ReadOnlySpan
            parsedMsg = parser.ParseFrom(new ReadOnlySpan<byte>(bytes));
            assert(parsedMsg);
        }

        public static void AssertReadingMessage(MessageParser parser, byte[] bytes, Action<IMessage> assert)
        {
            var parsedMsg = parser.ParseFrom(bytes);
            assert(parsedMsg);

            // Load content as single segment
            parsedMsg = parser.ParseFrom(new ReadOnlySequence<byte>(bytes));
            assert(parsedMsg);

            // Load content as multiple segments
            parsedMsg = parser.ParseFrom(ReadOnlySequenceFactory.CreateWithContent(bytes));
            assert(parsedMsg);

            // Load content as ReadOnlySpan
            parsedMsg = parser.ParseFrom(new ReadOnlySpan<byte>(bytes));
            assert(parsedMsg);
        }

        public static void AssertReadingMessageThrows<TMessage, TException>(MessageParser<TMessage> parser, byte[] bytes)
            where TMessage : IMessage<TMessage>
            where TException : Exception
        {
            Assert.Throws<TException>(() => parser.ParseFrom(bytes));

            Assert.Throws<TException>(() => parser.ParseFrom(new ReadOnlySequence<byte>(bytes)));

            Assert.Throws<TException>(() => parser.ParseFrom(new ReadOnlySpan<byte>(bytes)));
        }

        public static void AssertRoundtrip<T>(MessageParser<T> parser, T message, Action<T> additionalAssert = null) where T : IMessage<T>
        {
            var bytes = message.ToByteArray();

            // also serialize using IBufferWriter and check it leads to the same data
            var bufferWriter = new TestArrayBufferWriter<byte>();
            message.WriteTo(bufferWriter);
            Assert.AreEqual(bytes, bufferWriter.WrittenSpan.ToArray(), "Both serialization approaches need to result in the same data.");

            var parsedMsg = parser.ParseFrom(bytes);
            Assert.AreEqual(message, parsedMsg);
            additionalAssert?.Invoke(parsedMsg);

            // Load content as single segment
            parsedMsg = parser.ParseFrom(new ReadOnlySequence<byte>(bytes));
            Assert.AreEqual(message, parsedMsg);
            additionalAssert?.Invoke(parsedMsg);

            // Load content as multiple segments
            parsedMsg = parser.ParseFrom(ReadOnlySequenceFactory.CreateWithContent(bytes));
            Assert.AreEqual(message, parsedMsg);
            additionalAssert?.Invoke(parsedMsg);

            // Load content as ReadOnlySpan
            parsedMsg = parser.ParseFrom(new ReadOnlySpan<byte>(bytes));
            Assert.AreEqual(message, parsedMsg);
            additionalAssert?.Invoke(parsedMsg);
        }

        public static void AssertWritingMessage(IMessage message)
        {
            // serialize using CodedOutputStream
            var bytes = message.ToByteArray();

            int messageSize = message.CalculateSize(); 
            Assert.AreEqual(message.CalculateSize(), bytes.Length);

            // serialize using IBufferWriter and check it leads to the same output
            var bufferWriter = new TestArrayBufferWriter<byte>();
            message.WriteTo(bufferWriter);
            Assert.AreEqual(bytes, bufferWriter.WrittenSpan.ToArray());

            // serialize into a single span and check it leads to the same output
            var singleSpan = new Span<byte>(new byte[messageSize]);
            message.WriteTo(singleSpan);
            Assert.AreEqual(bytes, singleSpan.ToArray());

            // test for different IBufferWriter.GetSpan() segment sizes
            for (int blockSize = 1; blockSize < 256; blockSize *= 2)
            {
                var segmentedBufferWriter = new TestArrayBufferWriter<byte> { MaxGrowBy = blockSize };
                message.WriteTo(segmentedBufferWriter);
                Assert.AreEqual(bytes, segmentedBufferWriter.WrittenSpan.ToArray());
            }

            // if the full message is small enough, try serializing directly into stack-allocated buffer
            if (bytes.Length <= 256)
            {
                Span<byte> stackAllocBuffer = stackalloc byte[bytes.Length];
                message.WriteTo(stackAllocBuffer);
                Assert.AreEqual(bytes, stackAllocBuffer.ToArray());
            }
        }
    }
}