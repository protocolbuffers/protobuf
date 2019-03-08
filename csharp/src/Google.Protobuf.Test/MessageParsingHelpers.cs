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
#if GOOGLE_PROTOBUF_SUPPORT_SYSTEM_MEMORY
using Google.Protobuf.Buffers;
#endif
using System.Buffers;

namespace Google.Protobuf
{
    public static class MessageParsingHelpers
    {
        public static void AssertWritingMessage<T>(MessageParser<T> parser, T message, byte[] expectedData) where T : IMessage<T>
        {
            var bytes = message.ToByteArray();

#if GOOGLE_PROTOBUF_SUPPORT_SYSTEM_MEMORY
            var bufferMessage = (IBufferMessage)message;
            var bufferWriter = new ArrayBufferWriter<byte>();
            var output = new CodedOutputWriter(bufferWriter);
            bufferMessage.WriteTo(ref output);
            output.Flush();

            Assert.AreEqual(expectedData, bufferWriter.WrittenMemory.ToArray());
#endif
            Assert.AreEqual(expectedData, bytes);
        }

        public static void AssertReadingMessage<T>(MessageParser<T> parser, byte[] bytes, Action<T> assert) where T : IMessage<T>
        {
            var parsedStream = parser.ParseFrom(bytes);

#if GOOGLE_PROTOBUF_SUPPORT_SYSTEM_MEMORY
            // Load content as single segment
            var parsedBuffer = parser.ParseFrom(new ReadOnlySequence<byte>(bytes));
            assert(parsedBuffer);

            // Load content as multiple segments
            parsedBuffer = parser.ParseFrom(ReadOnlySequenceFactory.SegmentPerByteFactory.CreateWithContent(bytes));
            assert(parsedBuffer);
#endif
            assert(parsedStream);
        }

        public static void AssertReadingMessage(MessageParser parser, byte[] bytes, Action<IMessage> assert)
        {
            var parsedStream = parser.ParseFrom(bytes);

#if GOOGLE_PROTOBUF_SUPPORT_SYSTEM_MEMORY
            // Load content as single segment
            var parsedBuffer = parser.ParseFrom(new ReadOnlySequence<byte>(bytes));
            assert(parsedBuffer);

            // Load content as multiple segments
            parsedBuffer = parser.ParseFrom(ReadOnlySequenceFactory.SegmentPerByteFactory.CreateWithContent(bytes));
            assert(parsedBuffer);
#endif
            assert(parsedStream);
        }

        public static void AssertReadingMessageThrows<TMessage, TException>(MessageParser<TMessage> parser, byte[] bytes)
            where TMessage : IMessage<TMessage>
            where TException : Exception
        {
            Assert.Throws<TException>(() => parser.ParseFrom(bytes));

#if GOOGLE_PROTOBUF_SUPPORT_SYSTEM_MEMORY
            Assert.Throws<TException>(() => parser.ParseFrom(new ReadOnlySequence<byte>(bytes)));
#endif
        }

        public static void AssertMergingMessage<T>(MessageParser<T> parser, Func<T> prepareMessage, byte[] bytes, Action<T> assert) where T : IMessage<T>
        {
            var streamMergeMessage = prepareMessage();
            streamMergeMessage.MergeFrom(bytes);

#if GOOGLE_PROTOBUF_SUPPORT_SYSTEM_MEMORY
            var bufferMergeMessage = (IBufferMessage)prepareMessage();
            var reader = new CodedInputReader(new ReadOnlySequence<byte>(bytes));
            bufferMergeMessage.MergeFrom(ref reader);
            assert((T)bufferMergeMessage);
#endif
            assert(streamMergeMessage);
        }

        public static void AssertRoundtrip<T>(MessageParser<T> parser, T message, Action<T> additionalAssert = null) where T : IMessage<T>
        {
            var bytes = message.ToByteArray();

#if GOOGLE_PROTOBUF_SUPPORT_SYSTEM_MEMORY
            var bufferMessage = (IBufferMessage)message;
            var bufferWriter = new ArrayBufferWriter<byte>();
            var output = new CodedOutputWriter(bufferWriter);
            bufferMessage.WriteTo(ref output);
            output.Flush();

            // Load content as single segment
            var parsedBuffer = parser.ParseFrom(new ReadOnlySequence<byte>(bufferWriter.WrittenMemory));
            Assert.AreEqual(message, parsedBuffer);
            additionalAssert?.Invoke(parsedBuffer);

            // Load content as multiple segments
            parsedBuffer = parser.ParseFrom(ReadOnlySequenceFactory.SegmentPerByteFactory.CreateWithContent(bufferWriter.WrittenMemory.ToArray()));
            Assert.AreEqual(message, parsedBuffer);
            additionalAssert?.Invoke(parsedBuffer);
#endif

            var parsedStream = parser.ParseFrom(bytes);

            Assert.AreEqual(message, parsedStream);
            additionalAssert?.Invoke(parsedStream);
        }
    }
}
