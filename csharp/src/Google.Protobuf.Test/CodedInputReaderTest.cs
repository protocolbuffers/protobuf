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

#if GOOGLE_PROTOBUF_SUPPORT_SYSTEM_MEMORY
using System;
using System.Buffers;
using System.IO;
using Google.Protobuf.Buffers;
using Google.Protobuf.TestProtos;
using NUnit.Framework;

namespace Google.Protobuf
{
    public class CodedInputReaderTest : CodedInputTestBase
    {
        [Test]
        public void ReadWholeMessage_VaryingSegmentSizes()
        {
            TestAllTypes message = SampleMessages.CreateFullTestAllTypes();

            byte[] rawBytes = message.ToByteArray();
            Assert.AreEqual(rawBytes.Length, message.CalculateSize());
            TestAllTypes message2 = TestAllTypes.Parser.ParseFrom(new ReadOnlySequence<byte>(rawBytes));
            Assert.AreEqual(message, message2);

            // Try different block sizes.
            for (int blockSize = 1; blockSize < 256; blockSize *= 2)
            {
                message2 = TestAllTypes.Parser.ParseFrom(ReadOnlySequenceFactory.CreateWithContent(rawBytes, blockSize));
                Assert.AreEqual(message, message2);
            }
        }

        [Test]
        public void ReadMaliciouslyLargeBlob()
        {
            MemoryStream ms = new MemoryStream();
            CodedOutputStream output = new CodedOutputStream(ms);

            uint tag = WireFormat.MakeTag(1, WireFormat.WireType.LengthDelimited);
            output.WriteRawVarint32(tag);
            output.WriteRawVarint32(0x7FFFFFFF);
            output.WriteRawBytes(new byte[32]); // Pad with a few random bytes.
            output.Flush();
            ms.Position = 0;

            CodedInputReader input = new CodedInputReader(new ReadOnlySequence<byte>(ms.ToArray()));
            Assert.AreEqual(tag, input.ReadTag());

            try
            {
                input.ReadBytes();
                Assert.Fail();
            }
            catch (InvalidProtocolBufferException)
            {
            }
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

            CodedInputReader input = new CodedInputReader(new ReadOnlySequence<byte>(ms.ToArray()));

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

            CodedInputReader input = new CodedInputReader(new ReadOnlySequence<byte>(ms.ToArray()));

            Assert.AreEqual(tag, input.ReadTag());
            try
            {
                input.ReadString();
                Assert.Fail();
            }
            catch (InvalidProtocolBufferException)
            {
            }
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

            CodedInputReader input = new CodedInputReader(new ReadOnlySequence<byte>(ms.ToArray()));

            Assert.AreEqual(tag, input.ReadTag());
            try
            {
                input.ReadBytes();
                Assert.Fail();
            }
            catch (InvalidProtocolBufferException)
            {
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
            var input = new CodedInputReader(new ReadOnlySequence<byte>(stream.ToArray()));
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
            var input = new CodedInputReader(new ReadOnlySequence<byte>(stream.ToArray()));
            Assert.AreEqual(WireFormat.MakeTag(1, WireFormat.WireType.LengthDelimited), input.ReadTag());
            Assert.AreEqual("field 1", input.ReadString());
            Assert.AreEqual(WireFormat.MakeTag(2, WireFormat.WireType.StartGroup), input.ReadTag());

            try
            {
                input.SkipLastField();
                Assert.Fail();
            }
            catch (InvalidProtocolBufferException)
            {
            }
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

            var input = new CodedInputReader(new ReadOnlySequence<byte>(stream.ToArray()));
            Assert.AreEqual(WireFormat.MakeTag(1, WireFormat.WireType.EndGroup), input.ReadTag());

            try
            {
                input.SkipLastField();
                Assert.Fail();
            }
            catch (InvalidProtocolBufferException)
            {
            }
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
            var input = new CodedInputReader(new ReadOnlySequence<byte>(stream.ToArray()));
            input.ReadTag();

            try
            {
                input.SkipLastField();
                Assert.Fail();
            }
            catch (InvalidProtocolBufferException)
            {
            }
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
            var input = new CodedInputReader(new ReadOnlySequence<byte>(stream.ToArray()));
            Assert.AreEqual(WireFormat.MakeTag(1, WireFormat.WireType.StartGroup), input.ReadTag());

            try
            {
                input.SkipLastField();
                Assert.Fail();
            }
            catch (InvalidProtocolBufferException)
            {
            }
        }

        protected override void AssertReadVarint(byte[] data, ulong value)
        {
            CodedInputReader input = new CodedInputReader(new ReadOnlySequence<byte>(data));
            Assert.AreEqual((uint)value, input.ReadRawVarint32());

            Assert.AreEqual(true, input.IsAtEnd);

            input = new CodedInputReader(new ReadOnlySequence<byte>(data));
            Assert.AreEqual(value, input.ReadRawVarint64());

            Assert.AreEqual(true, input.IsAtEnd);
        }

        protected override void AssertReadVarintFailure(InvalidProtocolBufferException expected, byte[] data)
        {
            var exception = Assert.Throws<InvalidProtocolBufferException>(() =>
            {
                CodedInputReader input = new CodedInputReader(new ReadOnlySequence<byte>(data));
                input.ReadRawVarint32();
            });
            Assert.AreEqual(expected.Message, exception.Message);

            exception = Assert.Throws<InvalidProtocolBufferException>(() =>
            {
                CodedInputReader input = new CodedInputReader(new ReadOnlySequence<byte>(data));
                input.ReadRawVarint64();
            });
            Assert.AreEqual(expected.Message, exception.Message);
        }

        protected override void AssertReadLittleEndian32(byte[] data, uint value)
        {
            CodedInputReader input = new CodedInputReader(new ReadOnlySequence<byte>(data));
            Assert.AreEqual(value, input.ReadRawLittleEndian32());
            Assert.IsTrue(input.IsAtEnd);
        }

        protected override void AssertReadLittleEndian64(byte[] data, ulong value)
        {
            CodedInputReader input = new CodedInputReader(new ReadOnlySequence<byte>(data));
            Assert.AreEqual(value, input.ReadRawLittleEndian64());
            Assert.IsTrue(input.IsAtEnd);
        }

        protected override void ReadTag(byte[] data)
        {
            CodedInputReader input = new CodedInputReader(new ReadOnlySequence<byte>(data));
            input.ReadTag();
        }

        protected override void AssertEnum(byte[] data, int value)
        {
            CodedInputReader input = new CodedInputReader(new ReadOnlySequence<byte>(data));
            Assert.AreEqual(value, input.ReadEnum());
            Assert.IsTrue(input.IsAtEnd);
        }

        protected override void AssertReadFloat(byte[] data, float value)
        {
            CodedInputReader input = new CodedInputReader(new ReadOnlySequence<byte>(data));

            Assert.AreEqual(value, input.ReadFloat());
            Assert.IsTrue(input.IsAtEnd);
        }

        protected override void AssertReadDouble(byte[] data, double value)
        {
            CodedInputReader input = new CodedInputReader(new ReadOnlySequence<byte>(data));

            Assert.AreEqual(value, input.ReadDouble());
            Assert.IsTrue(input.IsAtEnd);
        }

        protected override T ParseFrom<T>(MessageParser<T> messageParser, ByteString bytes)
        {
            CodedInputReader input = new CodedInputReader(new ReadOnlySequence<byte>(bytes.ToByteArray()));
            return messageParser.ParseFrom(ref input);
        }

        protected override T ParseFromWithLimits<T>(MessageParser<T> messageParser, ByteString bytes, int sizeLimit, int resursionLimits)
        {
            CodedInputReader input = CodedInputReader.CreateWithLimits(new ReadOnlySequence<byte>(bytes.ToByteArray()), resursionLimits);
            return messageParser.ParseFrom(ref input);
        }
    }
}
#endif