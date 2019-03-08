#region Copyright notice and license
// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
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

using System;
using System.IO;
using Google.Protobuf.TestProtos;
using NUnit.Framework;

namespace Google.Protobuf
{
    public class CodedInputStreamTest : CodedInputTestBase
    {
        [Test]
        public void ReadWholeMessage_VaryingBlockSizes()
        {
            TestAllTypes message = SampleMessages.CreateFullTestAllTypes();

            byte[] rawBytes = message.ToByteArray();
            Assert.AreEqual(rawBytes.Length, message.CalculateSize());
            TestAllTypes message2 = TestAllTypes.Parser.ParseFrom(rawBytes);
            Assert.AreEqual(message, message2);

            // Try different block sizes.
            for (int blockSize = 1; blockSize < 256; blockSize *= 2)
            {
                message2 = TestAllTypes.Parser.ParseFrom(new SmallBlockInputStream(rawBytes, blockSize));
                Assert.AreEqual(message, message2);
            }
        }

        [Test]
        public void SizeLimit()
        {
            var data = ByteString.CopyFrom(SampleMessages.CreateFullTestAllTypes().ToByteArray());

            Assert.Throws<InvalidProtocolBufferException>(() => ParseFromWithLimits(TestAllTypes.Parser, data, sizeLimit: 16, resursionLimits: 100));
        }

        /// <summary>
        /// Tests that if we read an string that contains invalid UTF-8, no exception
        /// is thrown.  Instead, the invalid bytes are replaced with the Unicode
        /// "replacement character" U+FFFD.
        /// </summary>
        [Test]
        public void ReadInvalidUtf8()
        {
            MemoryStream ms = new MemoryStream();
            CodedOutputStream output = new CodedOutputStream(ms);

            uint tag = WireFormat.MakeTag(1, WireFormat.WireType.LengthDelimited);
            output.WriteRawVarint32(tag);
            output.WriteRawVarint32(1);
            output.WriteRawBytes(new byte[] {0x80});
            output.Flush();
            ms.Position = 0;

            CodedInputStream input = new CodedInputStream(ms);

            Assert.AreEqual(tag, input.ReadTag());
            string text = input.ReadString();
            Assert.AreEqual('\ufffd', text[0]);
        }

        [Test]
        public void ReadNegativeSizedStringThrowsInvalidProtocolBufferException()
        {
            MemoryStream ms = new MemoryStream();
            CodedOutputStream output = new CodedOutputStream(ms);

            uint tag = WireFormat.MakeTag(1, WireFormat.WireType.LengthDelimited);
            output.WriteRawVarint32(tag);
            output.WriteLength(-1);
            output.Flush();
            ms.Position = 0;

            CodedInputStream input = new CodedInputStream(ms);

            Assert.AreEqual(tag, input.ReadTag());
            Assert.Throws<InvalidProtocolBufferException>(() => input.ReadString());
        }

        [Test]
        public void ReadNegativeSizedBytesThrowsInvalidProtocolBufferException()
        {
            MemoryStream ms = new MemoryStream();
            CodedOutputStream output = new CodedOutputStream(ms);

            uint tag = WireFormat.MakeTag(1, WireFormat.WireType.LengthDelimited);
            output.WriteRawVarint32(tag);
            output.WriteLength(-1);
            output.Flush();
            ms.Position = 0;

            CodedInputStream input = new CodedInputStream(ms);

            Assert.AreEqual(tag, input.ReadTag());
            Assert.Throws<InvalidProtocolBufferException>(() => input.ReadBytes());
        }

        /// <summary>
        /// A stream which limits the number of bytes it reads at a time.
        /// We use this to make sure that CodedInputStream doesn't screw up when
        /// reading in small blocks.
        /// </summary>
        private sealed class SmallBlockInputStream : MemoryStream
        {
            private readonly int blockSize;

            public SmallBlockInputStream(byte[] data, int blockSize)
                : base(data)
            {
                this.blockSize = blockSize;
            }

            public override int Read(byte[] buffer, int offset, int count)
            {
                return base.Read(buffer, offset, Math.Min(count, blockSize));
            }
        }

        //Issue 71:	CodedInputStream.ReadBytes go to slow path unnecessarily
        [Test]
        public void TestSlowPathAvoidance()
        {
            using (var ms = new MemoryStream())
            {
                CodedOutputStream output = new CodedOutputStream(ms);
                output.WriteTag(1, WireFormat.WireType.LengthDelimited);
                output.WriteBytes(ByteString.CopyFrom(new byte[100]));
                output.WriteTag(2, WireFormat.WireType.LengthDelimited);
                output.WriteBytes(ByteString.CopyFrom(new byte[100]));
                output.Flush();

                ms.Position = 0;
                CodedInputStream input = new CodedInputStream(ms, new byte[ms.Length / 2], 0, 0, false);

                uint tag = input.ReadTag();
                Assert.AreEqual(1, WireFormat.GetTagFieldNumber(tag));
                Assert.AreEqual(100, input.ReadBytes().Length);

                tag = input.ReadTag();
                Assert.AreEqual(2, WireFormat.GetTagFieldNumber(tag));
                Assert.AreEqual(100, input.ReadBytes().Length);
            }
        }

        [Test]
        public void SkipGroup()
        {
            // Create an output stream with a group in:
            // Field 1: string "field 1"
            // Field 2: group containing:
            //   Field 1: fixed int32 value 100
            //   Field 2: string "ignore me"
            //   Field 3: nested group containing
            //      Field 1: fixed int64 value 1000
            // Field 3: string "field 3"
            var stream = new MemoryStream();
            var output = new CodedOutputStream(stream);
            output.WriteTag(1, WireFormat.WireType.LengthDelimited);
            output.WriteString("field 1");
            
            // The outer group...
            output.WriteTag(2, WireFormat.WireType.StartGroup);
            output.WriteTag(1, WireFormat.WireType.Fixed32);
            output.WriteFixed32(100);
            output.WriteTag(2, WireFormat.WireType.LengthDelimited);
            output.WriteString("ignore me");
            // The nested group...
            output.WriteTag(3, WireFormat.WireType.StartGroup);
            output.WriteTag(1, WireFormat.WireType.Fixed64);
            output.WriteFixed64(1000);
            // Note: Not sure the field number is relevant for end group...
            output.WriteTag(3, WireFormat.WireType.EndGroup);

            // End the outer group
            output.WriteTag(2, WireFormat.WireType.EndGroup);

            output.WriteTag(3, WireFormat.WireType.LengthDelimited);
            output.WriteString("field 3");
            output.Flush();
            stream.Position = 0;

            // Now act like a generated client
            var input = new CodedInputStream(stream);
            Assert.AreEqual(WireFormat.MakeTag(1, WireFormat.WireType.LengthDelimited), input.ReadTag());
            Assert.AreEqual("field 1", input.ReadString());
            Assert.AreEqual(WireFormat.MakeTag(2, WireFormat.WireType.StartGroup), input.ReadTag());
            input.SkipLastField(); // Should consume the whole group, including the nested one.
            Assert.AreEqual(WireFormat.MakeTag(3, WireFormat.WireType.LengthDelimited), input.ReadTag());
            Assert.AreEqual("field 3", input.ReadString());
        }

        [Test]
        public void SkipGroup_WrongEndGroupTag()
        {
            // Create an output stream with:
            // Field 1: string "field 1"
            // Start group 2
            //   Field 3: fixed int32
            // End group 4 (should give an error)
            var stream = new MemoryStream();
            var output = new CodedOutputStream(stream);
            output.WriteTag(1, WireFormat.WireType.LengthDelimited);
            output.WriteString("field 1");

            // The outer group...
            output.WriteTag(2, WireFormat.WireType.StartGroup);
            output.WriteTag(3, WireFormat.WireType.Fixed32);
            output.WriteFixed32(100);
            output.WriteTag(4, WireFormat.WireType.EndGroup);
            output.Flush();
            stream.Position = 0;

            // Now act like a generated client
            var input = new CodedInputStream(stream);
            Assert.AreEqual(WireFormat.MakeTag(1, WireFormat.WireType.LengthDelimited), input.ReadTag());
            Assert.AreEqual("field 1", input.ReadString());
            Assert.AreEqual(WireFormat.MakeTag(2, WireFormat.WireType.StartGroup), input.ReadTag());
            Assert.Throws<InvalidProtocolBufferException>(input.SkipLastField);
        }

        [Test]
        public void RogueEndGroupTag()
        {
            // If we have an end-group tag without a leading start-group tag, generated
            // code will just call SkipLastField... so that should fail.

            var stream = new MemoryStream();
            var output = new CodedOutputStream(stream);
            output.WriteTag(1, WireFormat.WireType.EndGroup);
            output.Flush();
            stream.Position = 0;

            var input = new CodedInputStream(stream);
            Assert.AreEqual(WireFormat.MakeTag(1, WireFormat.WireType.EndGroup), input.ReadTag());
            Assert.Throws<InvalidProtocolBufferException>(input.SkipLastField);
        }

        [Test]
        public void EndOfStreamReachedWhileSkippingGroup()
        {
            var stream = new MemoryStream();
            var output = new CodedOutputStream(stream);
            output.WriteTag(1, WireFormat.WireType.StartGroup);
            output.WriteTag(2, WireFormat.WireType.StartGroup);
            output.WriteTag(2, WireFormat.WireType.EndGroup);

            output.Flush();
            stream.Position = 0;

            // Now act like a generated client
            var input = new CodedInputStream(stream);
            input.ReadTag();
            Assert.Throws<InvalidProtocolBufferException>(input.SkipLastField);
        }

        [Test]
        public void RecursionLimitAppliedWhileSkippingGroup()
        {
            var stream = new MemoryStream();
            var output = new CodedOutputStream(stream);
            for (int i = 0; i < CodedInputStream.DefaultRecursionLimit + 1; i++)
            {
                output.WriteTag(1, WireFormat.WireType.StartGroup);
            }
            for (int i = 0; i < CodedInputStream.DefaultRecursionLimit + 1; i++)
            {
                output.WriteTag(1, WireFormat.WireType.EndGroup);
            }
            output.Flush();
            stream.Position = 0;

            // Now act like a generated client
            var input = new CodedInputStream(stream);
            Assert.AreEqual(WireFormat.MakeTag(1, WireFormat.WireType.StartGroup), input.ReadTag());
            Assert.Throws<InvalidProtocolBufferException>(input.SkipLastField);
        }

        [Test]
        public void Construction_Invalid()
        {
            Assert.Throws<ArgumentNullException>(() => new CodedInputStream((byte[]) null));
            Assert.Throws<ArgumentNullException>(() => new CodedInputStream(null, 0, 0));
            Assert.Throws<ArgumentNullException>(() => new CodedInputStream((Stream) null));
            Assert.Throws<ArgumentOutOfRangeException>(() => new CodedInputStream(new byte[10], 100, 0));
            Assert.Throws<ArgumentOutOfRangeException>(() => new CodedInputStream(new byte[10], 5, 10));
        }

        [Test]
        public void CreateWithLimits_InvalidLimits()
        {
            var stream = new MemoryStream();
            Assert.Throws<ArgumentOutOfRangeException>(() => CodedInputStream.CreateWithLimits(stream, 0, 1));
            Assert.Throws<ArgumentOutOfRangeException>(() => CodedInputStream.CreateWithLimits(stream, 1, 0));
        }

        [Test]
        public void Dispose_DisposesUnderlyingStream()
        {
            var memoryStream = new MemoryStream();
            Assert.IsTrue(memoryStream.CanRead);
            using (var cis = new CodedInputStream(memoryStream))
            {
            }
            Assert.IsFalse(memoryStream.CanRead); // Disposed
        }

        [Test]
        public void Dispose_WithLeaveOpen()
        {
            var memoryStream = new MemoryStream();
            Assert.IsTrue(memoryStream.CanRead);
            using (var cis = new CodedInputStream(memoryStream, true))
            {
            }
            Assert.IsTrue(memoryStream.CanRead); // We left the stream open
        }

        [Test]
        public void Dispose_FromByteArray()
        {
            var stream = new CodedInputStream(new byte[10]);
            stream.Dispose();
        }

        [Test]
        public void TestParseMessagesCloseTo2G()
        {
            byte[] serializedMessage = GenerateBigSerializedMessage();
            // How many of these big messages do we need to take us near our 2GB limit?
            int count = Int32.MaxValue / serializedMessage.Length;
            // Now make a MemoryStream that will fake a near-2GB stream of messages by returning
            // our big serialized message 'count' times.
            using (RepeatingMemoryStream stream = new RepeatingMemoryStream(serializedMessage, count))
            {
                Assert.DoesNotThrow(()=>TestAllTypes.Parser.ParseFrom(stream));
            }
        }

        [Test]
        public void TestParseMessagesOver2G()
        {
            byte[] serializedMessage = GenerateBigSerializedMessage();
            // How many of these big messages do we need to take us near our 2GB limit?
            int count = Int32.MaxValue / serializedMessage.Length;
            // Now add one to take us over the 2GB limit
            count++;
            // Now make a MemoryStream that will fake a near-2GB stream of messages by returning
            // our big serialized message 'count' times.
            using (RepeatingMemoryStream stream = new RepeatingMemoryStream(serializedMessage, count))
            {
                Assert.Throws<InvalidProtocolBufferException>(() => TestAllTypes.Parser.ParseFrom(stream),
                    "Protocol message was too large.  May be malicious.  " +
                    "Use CodedInputStream.SetSizeLimit() to increase the size limit.");
            }
        }

        /// <returns>A serialized big message</returns>
        private static byte[] GenerateBigSerializedMessage()
        {
            byte[] value = new byte[16 * 1024 * 1024];
            TestAllTypes message = SampleMessages.CreateFullTestAllTypes();
            message.SingleBytes = ByteString.CopyFrom(value);
            return message.ToByteArray();
        }

        protected override void AssertReadVarint(byte[] data, ulong value)
        {
            CodedInputStream input = new CodedInputStream(data);
            Assert.AreEqual((uint)value, input.ReadRawVarint32());

            input = new CodedInputStream(data);
            Assert.AreEqual(value, input.ReadRawVarint64());
            Assert.IsTrue(input.IsAtEnd);

            // Try different block sizes.
            for (int bufferSize = 1; bufferSize <= 16; bufferSize *= 2)
            {
                input = new CodedInputStream(new SmallBlockInputStream(data, bufferSize));
                Assert.AreEqual((uint)value, input.ReadRawVarint32());

                input = new CodedInputStream(new SmallBlockInputStream(data, bufferSize));
                Assert.AreEqual(value, input.ReadRawVarint64());
                Assert.IsTrue(input.IsAtEnd);
            }

            // Try reading directly from a MemoryStream. We want to verify that it
            // doesn't read past the end of the input, so write an extra byte - this
            // lets us test the position at the end.
            MemoryStream memoryStream = new MemoryStream();
            memoryStream.Write(data, 0, data.Length);
            memoryStream.WriteByte(0);
            memoryStream.Position = 0;
            Assert.AreEqual((uint)value, CodedInputStream.ReadRawVarint32(memoryStream));
            Assert.AreEqual(data.Length, memoryStream.Position);
        }

        protected override void AssertReadVarintFailure(InvalidProtocolBufferException expected, byte[] data)
        {
            CodedInputStream input = new CodedInputStream(data);
            var exception = Assert.Throws<InvalidProtocolBufferException>(() => input.ReadRawVarint32());
            Assert.AreEqual(expected.Message, exception.Message);

            input = new CodedInputStream(data);
            exception = Assert.Throws<InvalidProtocolBufferException>(() => input.ReadRawVarint64());
            Assert.AreEqual(expected.Message, exception.Message);

            // Make sure we get the same error when reading directly from a Stream.
            exception = Assert.Throws<InvalidProtocolBufferException>(() => CodedInputStream.ReadRawVarint32(new MemoryStream(data)));
            Assert.AreEqual(expected.Message, exception.Message);
        }

        protected override void AssertReadLittleEndian32(byte[] data, uint value)
        {
            CodedInputStream input = new CodedInputStream(data);
            Assert.AreEqual(value, input.ReadRawLittleEndian32());
            Assert.IsTrue(input.IsAtEnd);

            // Try different block sizes.
            for (int blockSize = 1; blockSize <= 16; blockSize *= 2)
            {
                input = new CodedInputStream(
                    new SmallBlockInputStream(data, blockSize));
                Assert.AreEqual(value, input.ReadRawLittleEndian32());
                Assert.IsTrue(input.IsAtEnd);
            }
        }

        protected override void AssertReadLittleEndian64(byte[] data, ulong value)
        {
            CodedInputStream input = new CodedInputStream(data);
            Assert.AreEqual(value, input.ReadRawLittleEndian64());
            Assert.IsTrue(input.IsAtEnd);

            // Try different block sizes.
            for (int blockSize = 1; blockSize <= 16; blockSize *= 2)
            {
                input = new CodedInputStream(
                    new SmallBlockInputStream(data, blockSize));
                Assert.AreEqual(value, input.ReadRawLittleEndian64());
                Assert.IsTrue(input.IsAtEnd);
            }
        }

        protected override void AssertEnum(byte[] data, int value)
        {
            CodedInputStream input = new CodedInputStream(data);
            Assert.AreEqual(value, input.ReadEnum());
            Assert.IsTrue(input.IsAtEnd);
        }

        protected override void ReadTag(byte[] data)
        {
            CodedInputStream input = new CodedInputStream(data);
            input.ReadTag();
        }

        protected override void AssertReadFloat(byte[] data, float value)
        {
            CodedInputStream input = new CodedInputStream(data);

            Assert.AreEqual(value, input.ReadFloat());
            Assert.IsTrue(input.IsAtEnd);
        }

        protected override void AssertReadDouble(byte[] data, double value)
        {
            CodedInputStream input = new CodedInputStream(data);

            Assert.AreEqual(value, input.ReadDouble());
            Assert.IsTrue(input.IsAtEnd);
        }

        protected override T ParseFrom<T>(MessageParser<T> messageParser, ByteString bytes)
        {
            return messageParser.ParseFrom(bytes);
        }

        protected override T ParseFromWithLimits<T>(MessageParser<T> messageParser, ByteString bytes, int sizeLimit, int resursionLimits)
        {
            CodedInputStream input = CodedInputStream.CreateWithLimits(new MemoryStream(bytes.ToByteArray()), sizeLimit, resursionLimits);
            return messageParser.ParseFrom(input);
        }

        /// <summary>
        /// A MemoryStream that repeats a byte arrays' content a number of times.
        /// Simulates really large input without consuming loads of memory. Used above
        /// to test the parsing behavior when the input size exceeds 2GB or close to it.
        /// </summary>
        private class RepeatingMemoryStream: MemoryStream
        {
            private readonly byte[] bytes;
            private readonly int maxIterations;
            private int index = 0;

            public RepeatingMemoryStream(byte[] bytes, int maxIterations)
            {
                this.bytes = bytes;
                this.maxIterations = maxIterations;
            }

            public override int Read(byte[] buffer, int offset, int count)
            {
                if (bytes.Length == 0)
                {
                    return 0;
                }
                int numBytesCopiedTotal = 0;
                while (numBytesCopiedTotal < count && index < maxIterations)
                {
                    int numBytesToCopy = Math.Min(bytes.Length - (int)Position, count);
                    Array.Copy(bytes, (int)Position, buffer, offset, numBytesToCopy);
                    numBytesCopiedTotal += numBytesToCopy;
                    offset += numBytesToCopy;
                    count -= numBytesCopiedTotal;
                    Position += numBytesToCopy;
                    if (Position >= bytes.Length)
                    {
                        Position = 0;
                        index++;
                    }
                }
                return numBytesCopiedTotal;
            }
        }
    }
}