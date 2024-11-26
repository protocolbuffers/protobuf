#region Copyright notice and license
// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd
#endregion

using Google.Protobuf.TestProtos;
using Proto2 = Google.Protobuf.TestProtos.Proto2;
using NUnit.Framework;
using System;
using System.Buffers;
using System.IO;

namespace Google.Protobuf
{
    public class CodedInputStreamTest
    {
        /// <summary>
        /// Helper to construct a byte array from a bunch of bytes.  The inputs are
        /// actually ints so that I can use hex notation and not get stupid errors
        /// about precision.
        /// </summary>
        private static byte[] Bytes(params int[] bytesAsInts)
        {
            byte[] bytes = new byte[bytesAsInts.Length];
            for (int i = 0; i < bytesAsInts.Length; i++)
            {
                bytes[i] = (byte) bytesAsInts[i];
            }
            return bytes;
        }

        /// <summary>
        /// Parses the given bytes using ReadRawVarint32() and ReadRawVarint64()
        /// </summary>
        private static void AssertReadVarint(byte[] data, ulong value)
        {
            CodedInputStream input = new CodedInputStream(data);
            Assert.AreEqual((uint) value, input.ReadRawVarint32());
            Assert.IsTrue(input.IsAtEnd);

            input = new CodedInputStream(data);
            Assert.AreEqual(value, input.ReadRawVarint64());
            Assert.IsTrue(input.IsAtEnd);

            AssertReadFromParseContext(new ReadOnlySequence<byte>(data), (ref ParseContext ctx) =>
            {
                Assert.AreEqual((uint) value, ctx.ReadUInt32());
            }, true);

            AssertReadFromParseContext(new ReadOnlySequence<byte>(data), (ref ParseContext ctx) =>
            {
                Assert.AreEqual(value, ctx.ReadUInt64());
            }, true);

            // Try different block sizes.
            for (int bufferSize = 1; bufferSize <= 16; bufferSize *= 2)
            {
                input = new CodedInputStream(new SmallBlockInputStream(data, bufferSize));
                Assert.AreEqual((uint) value, input.ReadRawVarint32());

                input = new CodedInputStream(new SmallBlockInputStream(data, bufferSize));
                Assert.AreEqual(value, input.ReadRawVarint64());
                Assert.IsTrue(input.IsAtEnd);

                AssertReadFromParseContext(ReadOnlySequenceFactory.CreateWithContent(data, bufferSize), (ref ParseContext ctx) =>
                {
                    Assert.AreEqual((uint) value, ctx.ReadUInt32());
                }, true);

                AssertReadFromParseContext(ReadOnlySequenceFactory.CreateWithContent(data, bufferSize), (ref ParseContext ctx) =>
                {
                    Assert.AreEqual(value, ctx.ReadUInt64());
                }, true);
            }

            // Try reading directly from a MemoryStream. We want to verify that it
            // doesn't read past the end of the input, so write an extra byte - this
            // lets us test the position at the end.
            MemoryStream memoryStream = new MemoryStream();
            memoryStream.Write(data, 0, data.Length);
            memoryStream.WriteByte(0);
            memoryStream.Position = 0;
            Assert.AreEqual((uint) value, CodedInputStream.ReadRawVarint32(memoryStream));
            Assert.AreEqual(data.Length, memoryStream.Position);
        }

        /// <summary>
        /// Parses the given bytes using ReadRawVarint32() and ReadRawVarint64() and
        /// expects them to fail with an InvalidProtocolBufferException whose
        /// description matches the given one.
        /// </summary>
        private static void AssertReadVarintFailure(InvalidProtocolBufferException expected, byte[] data)
        {
            CodedInputStream input = new CodedInputStream(data);
            var exception = Assert.Throws<InvalidProtocolBufferException>(() => input.ReadRawVarint32());
            Assert.AreEqual(expected.Message, exception.Message);

            input = new CodedInputStream(data);
            exception = Assert.Throws<InvalidProtocolBufferException>(() => input.ReadRawVarint64());
            Assert.AreEqual(expected.Message, exception.Message);

            AssertReadFromParseContext(new ReadOnlySequence<byte>(data), (ref ParseContext ctx) =>
            {
                try
                {
                    ctx.ReadUInt32();
                    Assert.Fail();
                }
                catch (InvalidProtocolBufferException ex)
                {
                    Assert.AreEqual(expected.Message, ex.Message);
                }
            }, false);

            AssertReadFromParseContext(new ReadOnlySequence<byte>(data), (ref ParseContext ctx) =>
            {
                try
                {
                    ctx.ReadUInt64();
                    Assert.Fail();
                }
                catch (InvalidProtocolBufferException ex)
                {
                    Assert.AreEqual(expected.Message, ex.Message);
                }
            }, false);

            // Make sure we get the same error when reading directly from a Stream.
            exception = Assert.Throws<InvalidProtocolBufferException>(() => CodedInputStream.ReadRawVarint32(new MemoryStream(data)));
            Assert.AreEqual(expected.Message, exception.Message);
        }

        private delegate void ParseContextAssertAction(ref ParseContext ctx);

        private static void AssertReadFromParseContext(ReadOnlySequence<byte> input, ParseContextAssertAction assertAction, bool assertIsAtEnd)
        {
            // Check as ReadOnlySequence<byte>
            ParseContext.Initialize(input, out ParseContext parseCtx);
            assertAction(ref parseCtx);
            if (assertIsAtEnd)
            {
                Assert.IsTrue(SegmentedBufferHelper.IsAtEnd(ref parseCtx.buffer, ref parseCtx.state));
            }

            // Check as ReadOnlySpan<byte>
            ParseContext.Initialize(input.ToArray().AsSpan(), out ParseContext spanParseContext);
            assertAction(ref spanParseContext);
            if (assertIsAtEnd)
            {
                Assert.IsTrue(SegmentedBufferHelper.IsAtEnd(ref spanParseContext.buffer, ref spanParseContext.state));
            }
        }

        [Test]
        public void ReadVarint()
        {
            AssertReadVarint(Bytes(0x00), 0);
            AssertReadVarint(Bytes(0x01), 1);
            AssertReadVarint(Bytes(0x7f), 127);
            // 14882
            AssertReadVarint(Bytes(0xa2, 0x74), (0x22 << 0) | (0x74 << 7));
            // 2961488830
            AssertReadVarint(Bytes(0xbe, 0xf7, 0x92, 0x84, 0x0b),
                             (0x3e << 0) | (0x77 << 7) | (0x12 << 14) | (0x04 << 21) |
                             (0x0bL << 28));

            // 64-bit
            // 7256456126
            AssertReadVarint(Bytes(0xbe, 0xf7, 0x92, 0x84, 0x1b),
                             (0x3e << 0) | (0x77 << 7) | (0x12 << 14) | (0x04 << 21) |
                             (0x1bL << 28));
            // 41256202580718336
            AssertReadVarint(Bytes(0x80, 0xe6, 0xeb, 0x9c, 0xc3, 0xc9, 0xa4, 0x49),
                             (0x00 << 0) | (0x66 << 7) | (0x6b << 14) | (0x1c << 21) |
                             (0x43L << 28) | (0x49L << 35) | (0x24L << 42) | (0x49L << 49));
            // 11964378330978735131
            AssertReadVarint(Bytes(0x9b, 0xa8, 0xf9, 0xc2, 0xbb, 0xd6, 0x80, 0x85, 0xa6, 0x01),
                             (0x1b << 0) | (0x28 << 7) | (0x79 << 14) | (0x42 << 21) |
                             (0x3bUL << 28) | (0x56UL << 35) | (0x00UL << 42) |
                             (0x05UL << 49) | (0x26UL << 56) | (0x01UL << 63));

            // Failures
            AssertReadVarintFailure(
                InvalidProtocolBufferException.MalformedVarint(),
                Bytes(0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
                      0x00));
            AssertReadVarintFailure(
                InvalidProtocolBufferException.TruncatedMessage(),
                Bytes(0x80));
        }

        /// <summary>
        /// Parses the given bytes using ReadRawLittleEndian32() and checks
        /// that the result matches the given value.
        /// </summary>
        private static void AssertReadLittleEndian32(byte[] data, uint value)
        {
            CodedInputStream input = new CodedInputStream(data);
            Assert.AreEqual(value, input.ReadRawLittleEndian32());
            Assert.IsTrue(input.IsAtEnd);

            AssertReadFromParseContext(new ReadOnlySequence<byte>(data), (ref ParseContext ctx) =>
            {
                Assert.AreEqual(value, ctx.ReadFixed32());
            }, true);

            // Try different block sizes.
            for (int blockSize = 1; blockSize <= 16; blockSize *= 2)
            {
                input = new CodedInputStream(
                    new SmallBlockInputStream(data, blockSize));
                Assert.AreEqual(value, input.ReadRawLittleEndian32());
                Assert.IsTrue(input.IsAtEnd);

                AssertReadFromParseContext(ReadOnlySequenceFactory.CreateWithContent(data, blockSize), (ref ParseContext ctx) =>
                {
                    Assert.AreEqual(value, ctx.ReadFixed32());
                }, true);
            }
        }

        /// <summary>
        /// Parses the given bytes using ReadRawLittleEndian64() and checks
        /// that the result matches the given value.
        /// </summary>
        private static void AssertReadLittleEndian64(byte[] data, ulong value)
        {
            CodedInputStream input = new CodedInputStream(data);
            Assert.AreEqual(value, input.ReadRawLittleEndian64());
            Assert.IsTrue(input.IsAtEnd);

            AssertReadFromParseContext(new ReadOnlySequence<byte>(data), (ref ParseContext ctx) =>
            {
                Assert.AreEqual(value, ctx.ReadFixed64());
            }, true);

            // Try different block sizes.
            for (int blockSize = 1; blockSize <= 16; blockSize *= 2)
            {
                input = new CodedInputStream(
                    new SmallBlockInputStream(data, blockSize));
                Assert.AreEqual(value, input.ReadRawLittleEndian64());
                Assert.IsTrue(input.IsAtEnd);

                AssertReadFromParseContext(ReadOnlySequenceFactory.CreateWithContent(data, blockSize), (ref ParseContext ctx) =>
                {
                    Assert.AreEqual(value, ctx.ReadFixed64());
                }, true);
            }
        }

        [Test]
        public void ReadLittleEndian()
        {
            AssertReadLittleEndian32(Bytes(0x78, 0x56, 0x34, 0x12), 0x12345678);
            AssertReadLittleEndian32(Bytes(0xf0, 0xde, 0xbc, 0x9a), 0x9abcdef0);

            AssertReadLittleEndian64(Bytes(0xf0, 0xde, 0xbc, 0x9a, 0x78, 0x56, 0x34, 0x12),
                                     0x123456789abcdef0L);
            AssertReadLittleEndian64(
                Bytes(0x78, 0x56, 0x34, 0x12, 0xf0, 0xde, 0xbc, 0x9a), 0x9abcdef012345678UL);
        }

        [Test]
        public void DecodeZigZag32()
        {
            Assert.AreEqual(0, ParsingPrimitives.DecodeZigZag32(0));
            Assert.AreEqual(-1, ParsingPrimitives.DecodeZigZag32(1));
            Assert.AreEqual(1, ParsingPrimitives.DecodeZigZag32(2));
            Assert.AreEqual(-2, ParsingPrimitives.DecodeZigZag32(3));
            Assert.AreEqual(0x3FFFFFFF, ParsingPrimitives.DecodeZigZag32(0x7FFFFFFE));
            Assert.AreEqual(unchecked((int) 0xC0000000), ParsingPrimitives.DecodeZigZag32(0x7FFFFFFF));
            Assert.AreEqual(0x7FFFFFFF, ParsingPrimitives.DecodeZigZag32(0xFFFFFFFE));
            Assert.AreEqual(unchecked((int) 0x80000000), ParsingPrimitives.DecodeZigZag32(0xFFFFFFFF));
        }

        [Test]
        public void DecodeZigZag64()
        {
            Assert.AreEqual(0, ParsingPrimitives.DecodeZigZag64(0));
            Assert.AreEqual(-1, ParsingPrimitives.DecodeZigZag64(1));
            Assert.AreEqual(1, ParsingPrimitives.DecodeZigZag64(2));
            Assert.AreEqual(-2, ParsingPrimitives.DecodeZigZag64(3));
            Assert.AreEqual(0x000000003FFFFFFFL, ParsingPrimitives.DecodeZigZag64(0x000000007FFFFFFEL));
            Assert.AreEqual(unchecked((long) 0xFFFFFFFFC0000000L), ParsingPrimitives.DecodeZigZag64(0x000000007FFFFFFFL));
            Assert.AreEqual(0x000000007FFFFFFFL, ParsingPrimitives.DecodeZigZag64(0x00000000FFFFFFFEL));
            Assert.AreEqual(unchecked((long) 0xFFFFFFFF80000000L), ParsingPrimitives.DecodeZigZag64(0x00000000FFFFFFFFL));
            Assert.AreEqual(0x7FFFFFFFFFFFFFFFL, ParsingPrimitives.DecodeZigZag64(0xFFFFFFFFFFFFFFFEL));
            Assert.AreEqual(unchecked((long) 0x8000000000000000L), ParsingPrimitives.DecodeZigZag64(0xFFFFFFFFFFFFFFFFL));
        }
        
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
        public void ReadWholeMessage_VaryingBlockSizes_FromSequence()
        {
            TestAllTypes message = SampleMessages.CreateFullTestAllTypes();

            byte[] rawBytes = message.ToByteArray();
            Assert.AreEqual(rawBytes.Length, message.CalculateSize());
            TestAllTypes message2 = TestAllTypes.Parser.ParseFrom(rawBytes);
            Assert.AreEqual(message, message2);

            // Try different block sizes.
            for (int blockSize = 1; blockSize < 256; blockSize *= 2)
            {
                message2 = TestAllTypes.Parser.ParseFrom(ReadOnlySequenceFactory.CreateWithContent(rawBytes, blockSize));
                Assert.AreEqual(message, message2);
            }
        }

        [Test]
        public void ReadInt32Wrapper_VariableBlockSizes()
        {
            byte[] rawBytes = new byte[] { 202, 1, 11, 8, 254, 255, 255, 255, 255, 255, 255, 255, 255, 1 };

            for (int blockSize = 1; blockSize <= rawBytes.Length; blockSize++)
            {
                ReadOnlySequence<byte> data = ReadOnlySequenceFactory.CreateWithContent(rawBytes, blockSize);
                AssertReadFromParseContext(data, (ref ParseContext ctx) =>
                {
                    ctx.ReadTag();

                    var value = ParsingPrimitivesWrappers.ReadInt32Wrapper(ref ctx);

                    Assert.AreEqual(-2, value);
                }, true);
            }
        }

        [Test]
        public void ReadHugeBlob()
        {
            // Allocate and initialize a 1MB blob.
            byte[] blob = new byte[1 << 20];
            for (int i = 0; i < blob.Length; i++)
            {
                blob[i] = (byte) i;
            }

            // Make a message containing it.
            var message = new TestAllTypes { SingleBytes = ByteString.CopyFrom(blob) };

            // Serialize and parse it.  Make sure to parse from an InputStream, not
            // directly from a ByteString, so that CodedInputStream uses buffered
            // reading.
            TestAllTypes message2 = TestAllTypes.Parser.ParseFrom(message.ToByteString());

            Assert.AreEqual(message, message2);
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

            CodedInputStream input = new CodedInputStream(ms);
            Assert.AreEqual(tag, input.ReadTag());

            Assert.Throws<InvalidProtocolBufferException>(() => input.ReadBytes());
        }

        [Test]
        public void ReadBlobGreaterThanCurrentLimit()
        {
            MemoryStream ms = new MemoryStream();
            CodedOutputStream output = new CodedOutputStream(ms);
            uint tag = WireFormat.MakeTag(1, WireFormat.WireType.LengthDelimited);
            output.WriteRawVarint32(tag);
            output.WriteRawVarint32(4);
            output.WriteRawBytes(new byte[4]); // Pad with a few random bytes.
            output.Flush();
            ms.Position = 0;

            CodedInputStream input = new CodedInputStream(ms);
            Assert.AreEqual(tag, input.ReadTag());

            // Specify limit smaller than data length
            input.PushLimit(3);
            Assert.Throws<InvalidProtocolBufferException>(() => input.ReadBytes());

            AssertReadFromParseContext(new ReadOnlySequence<byte>(ms.ToArray()), (ref ParseContext ctx) =>
            {
                Assert.AreEqual(tag, ctx.ReadTag());
                SegmentedBufferHelper.PushLimit(ref ctx.state, 3);
                try
                {
                    ctx.ReadBytes();
                    Assert.Fail();
                }
                catch (InvalidProtocolBufferException) {}
            }, true);
        }

        [Test]
        public void ReadStringGreaterThanCurrentLimit()
        {
            MemoryStream ms = new MemoryStream();
            CodedOutputStream output = new CodedOutputStream(ms);
            uint tag = WireFormat.MakeTag(1, WireFormat.WireType.LengthDelimited);
            output.WriteRawVarint32(tag);
            output.WriteRawVarint32(4);
            output.WriteRawBytes(new byte[4]); // Pad with a few random bytes.
            output.Flush();
            ms.Position = 0;

            CodedInputStream input = new CodedInputStream(ms.ToArray());
            Assert.AreEqual(tag, input.ReadTag());

            // Specify limit smaller than data length
            input.PushLimit(3);
            Assert.Throws<InvalidProtocolBufferException>(() => input.ReadString());

            AssertReadFromParseContext(new ReadOnlySequence<byte>(ms.ToArray()), (ref ParseContext ctx) =>
            {
                Assert.AreEqual(tag, ctx.ReadTag());
                SegmentedBufferHelper.PushLimit(ref ctx.state, 3);
                try
                {
                    ctx.ReadString();
                    Assert.Fail();
                }
                catch (InvalidProtocolBufferException) { }
            }, true);
        }

        // Representations of a tag for field 0 with various wire types
        [Test]
        [TestCase(0)]
        [TestCase(1)]
        [TestCase(2)]
        [TestCase(3)]
        [TestCase(4)]
        [TestCase(5)]
        public void ReadTag_ZeroFieldRejected(byte tag)
        {
            CodedInputStream cis = new CodedInputStream(new byte[] { tag });
            Assert.Throws<InvalidProtocolBufferException>(() => cis.ReadTag());
        }

        internal static TestRecursiveMessage MakeRecursiveMessage(int depth)
        {
            if (depth == 0)
            {
                return new TestRecursiveMessage { I = 5 };
            }
            else
            {
                return new TestRecursiveMessage { A = MakeRecursiveMessage(depth - 1) };
            }
        }

        internal static void AssertMessageDepth(TestRecursiveMessage message, int depth)
        {
            if (depth == 0)
            {
                Assert.IsNull(message.A);
                Assert.AreEqual(5, message.I);
            }
            else
            {
                Assert.IsNotNull(message.A);
                AssertMessageDepth(message.A, depth - 1);
            }
        }

        [Test]
        public void MaliciousRecursion()
        {
            ByteString atRecursiveLimit = MakeRecursiveMessage(CodedInputStream.DefaultRecursionLimit).ToByteString();
            ByteString beyondRecursiveLimit = MakeRecursiveMessage(CodedInputStream.DefaultRecursionLimit + 1).ToByteString();

            AssertMessageDepth(TestRecursiveMessage.Parser.ParseFrom(atRecursiveLimit), CodedInputStream.DefaultRecursionLimit);

            Assert.Throws<InvalidProtocolBufferException>(() => TestRecursiveMessage.Parser.ParseFrom(beyondRecursiveLimit));

            CodedInputStream input = CodedInputStream.CreateWithLimits(new MemoryStream(atRecursiveLimit.ToByteArray()), 1000000, CodedInputStream.DefaultRecursionLimit - 1);
            Assert.Throws<InvalidProtocolBufferException>(() => TestRecursiveMessage.Parser.ParseFrom(input));
        }
        
        private static byte[] MakeMaliciousRecursionUnknownFieldsPayload(int recursionDepth)
        {
            // generate recursively nested groups that will be parsed as unknown fields
            int unknownFieldNumber = 14;  // an unused field number
            MemoryStream ms = new MemoryStream();
            CodedOutputStream output = new CodedOutputStream(ms);
            for (int i = 0; i < recursionDepth; i++)
            {
                output.WriteTag(WireFormat.MakeTag(unknownFieldNumber, WireFormat.WireType.StartGroup));
            }
            for (int i = 0; i < recursionDepth; i++)
            {
                output.WriteTag(WireFormat.MakeTag(unknownFieldNumber, WireFormat.WireType.EndGroup));
            }
            output.Flush();
            return ms.ToArray();
        }

        [Test]
        public void MaliciousRecursion_UnknownFields()
        {
            byte[] payloadAtRecursiveLimit = MakeMaliciousRecursionUnknownFieldsPayload(CodedInputStream.DefaultRecursionLimit);
            byte[] payloadBeyondRecursiveLimit = MakeMaliciousRecursionUnknownFieldsPayload(CodedInputStream.DefaultRecursionLimit + 1);
            
            Assert.DoesNotThrow(() => TestRecursiveMessage.Parser.ParseFrom(payloadAtRecursiveLimit));
            Assert.Throws<InvalidProtocolBufferException>(() => TestRecursiveMessage.Parser.ParseFrom(payloadBeyondRecursiveLimit));
        }

        [Test]
        public void ReadGroup_WrongEndGroupTag()
        {
            int groupFieldNumber = Proto2.TestAllTypes.OptionalGroupFieldNumber;

            // write Proto2.TestAllTypes with "optional_group" set, but use wrong EndGroup closing tag
            var ms = new MemoryStream();
            var output = new CodedOutputStream(ms);
            output.WriteTag(WireFormat.MakeTag(groupFieldNumber, WireFormat.WireType.StartGroup));
            output.WriteGroup(new Proto2.TestAllTypes.Types.OptionalGroup { A = 12345 });
            // end group with different field number
            output.WriteTag(WireFormat.MakeTag(groupFieldNumber + 1, WireFormat.WireType.EndGroup));
            output.Flush();
            var payload = ms.ToArray();

            Assert.Throws<InvalidProtocolBufferException>(() => Proto2.TestAllTypes.Parser.ParseFrom(payload));
        }

        [Test]
        public void ReadGroup_UnknownFields_WrongEndGroupTag()
        {
            var ms = new MemoryStream();
            var output = new CodedOutputStream(ms);
            output.WriteTag(WireFormat.MakeTag(14, WireFormat.WireType.StartGroup));
            // end group with different field number
            output.WriteTag(WireFormat.MakeTag(15, WireFormat.WireType.EndGroup));
            output.Flush();
            var payload = ms.ToArray();

            Assert.Throws<InvalidProtocolBufferException>(() => TestRecursiveMessage.Parser.ParseFrom(payload));
        }

        [Test]
        public void SizeLimit()
        {
            // Have to use a Stream rather than ByteString.CreateCodedInput as SizeLimit doesn't
            // apply to the latter case.
            MemoryStream ms = new MemoryStream(SampleMessages.CreateFullTestAllTypes().ToByteArray());
            CodedInputStream input = CodedInputStream.CreateWithLimits(ms, 16, 100);
            Assert.Throws<InvalidProtocolBufferException>(() => TestAllTypes.Parser.ParseFrom(input));
        }

        /// <summary>
        /// Tests that if we read a string that contains invalid UTF-8, an exception
        /// is thrown.
        /// </summary>
        [Test]
        public void ReadInvalidUtf8ThrowsInvalidProtocolBufferException()
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
            Assert.Throws<InvalidProtocolBufferException>(() => input.ReadString());
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

            var input = new CodedInputStream(ms);

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

        [Test]
        public void TestNegativeEnum()
        {
            byte[] bytes = { 0xFE, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x01 };
            CodedInputStream input = new CodedInputStream(bytes);
            Assert.AreEqual((int)SampleEnum.NegativeValue, input.ReadEnum());
            Assert.IsTrue(input.IsAtEnd);
        }

        //Issue 71:	CodedInputStream.ReadBytes go to slow path unnecessarily
        [Test]
        public void TestSlowPathAvoidance()
        {
            using var ms = new MemoryStream();
            var output = new CodedOutputStream(ms);
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

        [Test]
        public void MaximumFieldNumber()
        {
            MemoryStream ms = new MemoryStream();
            CodedOutputStream output = new CodedOutputStream(ms);

            int fieldNumber = 0x1FFFFFFF;
            uint tag = WireFormat.MakeTag(fieldNumber, WireFormat.WireType.LengthDelimited);
            output.WriteRawVarint32(tag);
            output.WriteString("field 1");
            output.Flush();
            ms.Position = 0;

            CodedInputStream input = new CodedInputStream(ms);

            Assert.AreEqual(tag, input.ReadTag());
            Assert.AreEqual(fieldNumber, WireFormat.GetTagFieldNumber(tag));
        }

        [Test]
        public void Tag0Throws()
        {
            var input = new CodedInputStream(new byte[] { 0 });
            Assert.Throws<InvalidProtocolBufferException>(() => input.ReadTag());
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
            int count = int.MaxValue / serializedMessage.Length;
            // Now make a MemoryStream that will fake a near-2GB stream of messages by returning
            // our big serialized message 'count' times.
            using var stream = new RepeatingMemoryStream(serializedMessage, count);
            Assert.DoesNotThrow(() => TestAllTypes.Parser.ParseFrom(stream));
        }

        [Test]
        public void TestParseMessagesOver2G()
        {
            byte[] serializedMessage = GenerateBigSerializedMessage();
            // How many of these big messages do we need to take us near our 2GB limit?
            int count = int.MaxValue / serializedMessage.Length;
            // Now add one to take us over the 2GB limit
            count++;
            // Now make a MemoryStream that will fake a near-2GB stream of messages by returning
            // our big serialized message 'count' times.
            using var stream = new RepeatingMemoryStream(serializedMessage, count);
            Assert.Throws<InvalidProtocolBufferException>(() => TestAllTypes.Parser.ParseFrom(stream),
                "Protocol message was too large.  May be malicious.  " +
                "Use CodedInputStream.SetSizeLimit() to increase the size limit.");
        }

        /// <returns>A serialized big message</returns>
        private static byte[] GenerateBigSerializedMessage()
        {
            byte[] value = new byte[16 * 1024 * 1024];
            TestAllTypes message = SampleMessages.CreateFullTestAllTypes();
            message.SingleBytes = ByteString.CopyFrom(value);
            return message.ToByteArray();
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
