#region Copyright notice and license
// Protocol Buffers - Google's data interchange format
// Copyright 2015 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd
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