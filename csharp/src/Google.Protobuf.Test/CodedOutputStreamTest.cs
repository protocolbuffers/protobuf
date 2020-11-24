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
using Google.Protobuf.Buffers;
using NUnit.Framework;

namespace Google.Protobuf
{
    public class CodedOutputStreamTest
    {
        /// <summary>
        /// Writes the given value using WriteRawVarint32() and WriteRawVarint64() and
        /// checks that the result matches the given bytes
        /// </summary>
        private static void AssertWriteVarint(byte[] data, ulong value)
        {
            // Only do 32-bit write if the value fits in 32 bits.
            if ((value >> 32) == 0)
            {
                // CodedOutputStream
                MemoryStream rawOutput = new MemoryStream();
                CodedOutputStream output = new CodedOutputStream(rawOutput);
                output.WriteRawVarint32((uint) value);
                output.Flush();
                Assert.AreEqual(data, rawOutput.ToArray());

                // IBufferWriter
                var bufferWriter = new ArrayBufferWriter<byte>();
                WriteContext.Initialize(bufferWriter, out WriteContext ctx);
                ctx.WriteUInt32((uint) value);
                ctx.Flush();
                Assert.AreEqual(data, bufferWriter.WrittenSpan.ToArray());

                // Also try computing size.
                Assert.AreEqual(data.Length, CodedOutputStream.ComputeRawVarint32Size((uint) value));
            }

            {
                // CodedOutputStream
                MemoryStream rawOutput = new MemoryStream();
                CodedOutputStream output = new CodedOutputStream(rawOutput);
                output.WriteRawVarint64(value);
                output.Flush();
                Assert.AreEqual(data, rawOutput.ToArray());

                // IBufferWriter
                var bufferWriter = new ArrayBufferWriter<byte>();
                WriteContext.Initialize(bufferWriter, out WriteContext ctx);
                ctx.WriteUInt64(value);
                ctx.Flush();
                Assert.AreEqual(data, bufferWriter.WrittenSpan.ToArray());

                // Also try computing size.
                Assert.AreEqual(data.Length, CodedOutputStream.ComputeRawVarint64Size(value));
            }

            // Try different buffer sizes.
            for (int bufferSize = 1; bufferSize <= 16; bufferSize *= 2)
            {
                // Only do 32-bit write if the value fits in 32 bits.
                if ((value >> 32) == 0)
                {
                    MemoryStream rawOutput = new MemoryStream();
                    CodedOutputStream output =
                        new CodedOutputStream(rawOutput, bufferSize);
                    output.WriteRawVarint32((uint) value);
                    output.Flush();
                    Assert.AreEqual(data, rawOutput.ToArray());

                    var bufferWriter = new ArrayBufferWriter<byte>();
                    bufferWriter.MaxGrowBy = bufferSize;
                    WriteContext.Initialize(bufferWriter, out WriteContext ctx);
                    ctx.WriteUInt32((uint) value);
                    ctx.Flush();
                    Assert.AreEqual(data, bufferWriter.WrittenSpan.ToArray());
                }

                {
                    MemoryStream rawOutput = new MemoryStream();
                    CodedOutputStream output = new CodedOutputStream(rawOutput, bufferSize);
                    output.WriteRawVarint64(value);
                    output.Flush();
                    Assert.AreEqual(data, rawOutput.ToArray());

                    var bufferWriter = new ArrayBufferWriter<byte>();
                    bufferWriter.MaxGrowBy = bufferSize;
                    WriteContext.Initialize(bufferWriter, out WriteContext ctx);
                    ctx.WriteUInt64(value);
                    ctx.Flush();
                    Assert.AreEqual(data, bufferWriter.WrittenSpan.ToArray());
                }

            }
        }

        /// <summary>
        /// Tests WriteRawVarint32() and WriteRawVarint64()
        /// </summary>
        [Test]
        public void WriteVarint()
        {
            AssertWriteVarint(new byte[] {0x00}, 0);
            AssertWriteVarint(new byte[] {0x01}, 1);
            AssertWriteVarint(new byte[] {0x7f}, 127);
            // 14882
            AssertWriteVarint(new byte[] {0xa2, 0x74}, (0x22 << 0) | (0x74 << 7));
            // 2961488830
            AssertWriteVarint(new byte[] {0xbe, 0xf7, 0x92, 0x84, 0x0b},
                              (0x3e << 0) | (0x77 << 7) | (0x12 << 14) | (0x04 << 21) |
                              (0x0bL << 28));

            // 64-bit
            // 7256456126
            AssertWriteVarint(new byte[] {0xbe, 0xf7, 0x92, 0x84, 0x1b},
                              (0x3e << 0) | (0x77 << 7) | (0x12 << 14) | (0x04 << 21) |
                              (0x1bL << 28));
            // 41256202580718336
            AssertWriteVarint(
                new byte[] {0x80, 0xe6, 0xeb, 0x9c, 0xc3, 0xc9, 0xa4, 0x49},
                (0x00 << 0) | (0x66 << 7) | (0x6b << 14) | (0x1c << 21) |
                (0x43UL << 28) | (0x49L << 35) | (0x24UL << 42) | (0x49UL << 49));
            // 11964378330978735131
            AssertWriteVarint(
                new byte[] {0x9b, 0xa8, 0xf9, 0xc2, 0xbb, 0xd6, 0x80, 0x85, 0xa6, 0x01},
                unchecked((ulong)
                          ((0x1b << 0) | (0x28 << 7) | (0x79 << 14) | (0x42 << 21) |
                           (0x3bL << 28) | (0x56L << 35) | (0x00L << 42) |
                           (0x05L << 49) | (0x26L << 56) | (0x01L << 63))));
        }

        /// <summary>
        /// Parses the given bytes using WriteRawLittleEndian32() and checks
        /// that the result matches the given value.
        /// </summary>
        private static void AssertWriteLittleEndian32(byte[] data, uint value)
        {
            {
                var rawOutput = new MemoryStream();
                var output = new CodedOutputStream(rawOutput);
                output.WriteRawLittleEndian32(value);
                output.Flush();
                Assert.AreEqual(data, rawOutput.ToArray());

                var bufferWriter = new ArrayBufferWriter<byte>();
                WriteContext.Initialize(bufferWriter, out WriteContext ctx);
                ctx.WriteFixed32(value);
                ctx.Flush();
                Assert.AreEqual(data, bufferWriter.WrittenSpan.ToArray());
            }

            // Try different buffer sizes.
            for (int bufferSize = 1; bufferSize <= 16; bufferSize *= 2)
            {
                var rawOutput = new MemoryStream();
                var output = new CodedOutputStream(rawOutput, bufferSize);
                output.WriteRawLittleEndian32(value);
                output.Flush();
                Assert.AreEqual(data, rawOutput.ToArray());

                var bufferWriter = new ArrayBufferWriter<byte>();
                bufferWriter.MaxGrowBy = bufferSize;
                WriteContext.Initialize(bufferWriter, out WriteContext ctx);
                ctx.WriteFixed32(value);
                ctx.Flush();
                Assert.AreEqual(data, bufferWriter.WrittenSpan.ToArray());
            }
        }

        /// <summary>
        /// Parses the given bytes using WriteRawLittleEndian64() and checks
        /// that the result matches the given value.
        /// </summary>
        private static void AssertWriteLittleEndian64(byte[] data, ulong value)
        {
            {
                var rawOutput = new MemoryStream();
                var output = new CodedOutputStream(rawOutput);
                output.WriteRawLittleEndian64(value);
                output.Flush();
                Assert.AreEqual(data, rawOutput.ToArray());

                var bufferWriter = new ArrayBufferWriter<byte>();
                WriteContext.Initialize(bufferWriter, out WriteContext ctx);
                ctx.WriteFixed64(value);
                ctx.Flush();
                Assert.AreEqual(data, bufferWriter.WrittenSpan.ToArray());
            }

            // Try different block sizes.
            for (int blockSize = 1; blockSize <= 16; blockSize *= 2)
            {
                var rawOutput = new MemoryStream();
                var output = new CodedOutputStream(rawOutput, blockSize);
                output.WriteRawLittleEndian64(value);
                output.Flush();
                Assert.AreEqual(data, rawOutput.ToArray());

                var bufferWriter = new ArrayBufferWriter<byte>();
                bufferWriter.MaxGrowBy = blockSize;
                WriteContext.Initialize(bufferWriter, out WriteContext ctx);
                ctx.WriteFixed64(value);
                ctx.Flush();
                Assert.AreEqual(data, bufferWriter.WrittenSpan.ToArray());
            }
        }

        /// <summary>
        /// Tests writeRawLittleEndian32() and writeRawLittleEndian64().
        /// </summary>
        [Test]
        public void WriteLittleEndian()
        {
            AssertWriteLittleEndian32(new byte[] {0x78, 0x56, 0x34, 0x12}, 0x12345678);
            AssertWriteLittleEndian32(new byte[] {0xf0, 0xde, 0xbc, 0x9a}, 0x9abcdef0);

            AssertWriteLittleEndian64(
                new byte[] {0xf0, 0xde, 0xbc, 0x9a, 0x78, 0x56, 0x34, 0x12},
                0x123456789abcdef0L);
            AssertWriteLittleEndian64(
                new byte[] {0x78, 0x56, 0x34, 0x12, 0xf0, 0xde, 0xbc, 0x9a},
                0x9abcdef012345678UL);
        }

        [Test]
        public void WriteWholeMessage_VaryingBlockSizes()
        {
            TestAllTypes message = SampleMessages.CreateFullTestAllTypes();

            byte[] rawBytes = message.ToByteArray();

            // Try different block sizes.
            for (int blockSize = 1; blockSize < 256; blockSize *= 2)
            {
                MemoryStream rawOutput = new MemoryStream();
                CodedOutputStream output = new CodedOutputStream(rawOutput, blockSize);
                message.WriteTo(output);
                output.Flush();
                Assert.AreEqual(rawBytes, rawOutput.ToArray());

                var bufferWriter = new ArrayBufferWriter<byte>();
                bufferWriter.MaxGrowBy = blockSize;
                message.WriteTo(bufferWriter);
                Assert.AreEqual(rawBytes, bufferWriter.WrittenSpan.ToArray()); 
            }
        }

        [Test]
        public void WriteContext_WritesWithFlushes()
        {
            TestAllTypes message = SampleMessages.CreateFullTestAllTypes();

            MemoryStream expectedOutput = new MemoryStream();
            CodedOutputStream output = new CodedOutputStream(expectedOutput);
            output.WriteMessage(message);
            output.Flush();
            byte[] expectedBytes1 = expectedOutput.ToArray();

            output.WriteMessage(message);
            output.Flush();
            byte[] expectedBytes2 = expectedOutput.ToArray();

            var bufferWriter = new ArrayBufferWriter<byte>();
            WriteContext.Initialize(bufferWriter, out WriteContext ctx);
            ctx.WriteMessage(message);
            ctx.Flush();
            Assert.AreEqual(expectedBytes1, bufferWriter.WrittenSpan.ToArray());

            ctx.WriteMessage(message);
            ctx.Flush();
            Assert.AreEqual(expectedBytes2, bufferWriter.WrittenSpan.ToArray());
        }
        
        [Test]
        public void EncodeZigZag32()
        {
            Assert.AreEqual(0u, WritingPrimitives.EncodeZigZag32(0));
            Assert.AreEqual(1u, WritingPrimitives.EncodeZigZag32(-1));
            Assert.AreEqual(2u, WritingPrimitives.EncodeZigZag32(1));
            Assert.AreEqual(3u, WritingPrimitives.EncodeZigZag32(-2));
            Assert.AreEqual(0x7FFFFFFEu, WritingPrimitives.EncodeZigZag32(0x3FFFFFFF));
            Assert.AreEqual(0x7FFFFFFFu, WritingPrimitives.EncodeZigZag32(unchecked((int) 0xC0000000)));
            Assert.AreEqual(0xFFFFFFFEu, WritingPrimitives.EncodeZigZag32(0x7FFFFFFF));
            Assert.AreEqual(0xFFFFFFFFu, WritingPrimitives.EncodeZigZag32(unchecked((int) 0x80000000)));
        }

        [Test]
        public void EncodeZigZag64()
        {
            Assert.AreEqual(0u, WritingPrimitives.EncodeZigZag64(0));
            Assert.AreEqual(1u, WritingPrimitives.EncodeZigZag64(-1));
            Assert.AreEqual(2u, WritingPrimitives.EncodeZigZag64(1));
            Assert.AreEqual(3u, WritingPrimitives.EncodeZigZag64(-2));
            Assert.AreEqual(0x000000007FFFFFFEuL,
                            WritingPrimitives.EncodeZigZag64(unchecked((long) 0x000000003FFFFFFFUL)));
            Assert.AreEqual(0x000000007FFFFFFFuL,
                            WritingPrimitives.EncodeZigZag64(unchecked((long) 0xFFFFFFFFC0000000UL)));
            Assert.AreEqual(0x00000000FFFFFFFEuL,
                            WritingPrimitives.EncodeZigZag64(unchecked((long) 0x000000007FFFFFFFUL)));
            Assert.AreEqual(0x00000000FFFFFFFFuL,
                            WritingPrimitives.EncodeZigZag64(unchecked((long) 0xFFFFFFFF80000000UL)));
            Assert.AreEqual(0xFFFFFFFFFFFFFFFEL,
                            WritingPrimitives.EncodeZigZag64(unchecked((long) 0x7FFFFFFFFFFFFFFFUL)));
            Assert.AreEqual(0xFFFFFFFFFFFFFFFFL,
                            WritingPrimitives.EncodeZigZag64(unchecked((long) 0x8000000000000000UL)));
        }

        [Test]
        public void RoundTripZigZag32()
        {
            // Some easier-to-verify round-trip tests.  The inputs (other than 0, 1, -1)
            // were chosen semi-randomly via keyboard bashing.
            Assert.AreEqual(0, ParsingPrimitives.DecodeZigZag32(WritingPrimitives.EncodeZigZag32(0)));
            Assert.AreEqual(1, ParsingPrimitives.DecodeZigZag32(WritingPrimitives.EncodeZigZag32(1)));
            Assert.AreEqual(-1, ParsingPrimitives.DecodeZigZag32(WritingPrimitives.EncodeZigZag32(-1)));
            Assert.AreEqual(14927, ParsingPrimitives.DecodeZigZag32(WritingPrimitives.EncodeZigZag32(14927)));
            Assert.AreEqual(-3612, ParsingPrimitives.DecodeZigZag32(WritingPrimitives.EncodeZigZag32(-3612)));
        }

        [Test]
        public void RoundTripZigZag64()
        {
            Assert.AreEqual(0, ParsingPrimitives.DecodeZigZag64(WritingPrimitives.EncodeZigZag64(0)));
            Assert.AreEqual(1, ParsingPrimitives.DecodeZigZag64(WritingPrimitives.EncodeZigZag64(1)));
            Assert.AreEqual(-1, ParsingPrimitives.DecodeZigZag64(WritingPrimitives.EncodeZigZag64(-1)));
            Assert.AreEqual(14927, ParsingPrimitives.DecodeZigZag64(WritingPrimitives.EncodeZigZag64(14927)));
            Assert.AreEqual(-3612, ParsingPrimitives.DecodeZigZag64(WritingPrimitives.EncodeZigZag64(-3612)));

            Assert.AreEqual(856912304801416L,
                            ParsingPrimitives.DecodeZigZag64(WritingPrimitives.EncodeZigZag64(856912304801416L)));
            Assert.AreEqual(-75123905439571256L,
                            ParsingPrimitives.DecodeZigZag64(WritingPrimitives.EncodeZigZag64(-75123905439571256L)));
        }

        [Test]
        public void TestNegativeEnumNoTag()
        {
            Assert.AreEqual(10, CodedOutputStream.ComputeInt32Size(-2));
            Assert.AreEqual(10, CodedOutputStream.ComputeEnumSize((int) SampleEnum.NegativeValue));

            byte[] bytes = new byte[10];
            CodedOutputStream output = new CodedOutputStream(bytes);
            output.WriteEnum((int) SampleEnum.NegativeValue);

            Assert.AreEqual(0, output.SpaceLeft);
            Assert.AreEqual("FE-FF-FF-FF-FF-FF-FF-FF-FF-01", BitConverter.ToString(bytes));
        }

        [Test]
        public void TestCodedInputOutputPosition()
        {
            byte[] content = new byte[110];
            for (int i = 0; i < content.Length; i++)
                content[i] = (byte)i;

            byte[] child = new byte[120];
            {
                MemoryStream ms = new MemoryStream(child);
                CodedOutputStream cout = new CodedOutputStream(ms, 20);
                // Field 11: numeric value: 500
                cout.WriteTag(11, WireFormat.WireType.Varint);
                Assert.AreEqual(1, cout.Position);
                cout.WriteInt32(500);
                Assert.AreEqual(3, cout.Position);
                //Field 12: length delimited 120 bytes
                cout.WriteTag(12, WireFormat.WireType.LengthDelimited);
                Assert.AreEqual(4, cout.Position);
                cout.WriteBytes(ByteString.CopyFrom(content));
                Assert.AreEqual(115, cout.Position);
                // Field 13: fixed numeric value: 501
                cout.WriteTag(13, WireFormat.WireType.Fixed32);
                Assert.AreEqual(116, cout.Position);
                cout.WriteSFixed32(501);
                Assert.AreEqual(120, cout.Position);
                cout.Flush();
            }

            byte[] bytes = new byte[130];
            {
                CodedOutputStream cout = new CodedOutputStream(bytes);
                // Field 1: numeric value: 500
                cout.WriteTag(1, WireFormat.WireType.Varint);
                Assert.AreEqual(1, cout.Position);
                cout.WriteInt32(500);
                Assert.AreEqual(3, cout.Position);
                //Field 2: length delimited 120 bytes
                cout.WriteTag(2, WireFormat.WireType.LengthDelimited);
                Assert.AreEqual(4, cout.Position);
                cout.WriteBytes(ByteString.CopyFrom(child));
                Assert.AreEqual(125, cout.Position);
                // Field 3: fixed numeric value: 500
                cout.WriteTag(3, WireFormat.WireType.Fixed32);
                Assert.AreEqual(126, cout.Position);
                cout.WriteSFixed32(501);
                Assert.AreEqual(130, cout.Position);
                cout.Flush();
            }
            // Now test Input stream:
            {
                CodedInputStream cin = new CodedInputStream(new MemoryStream(bytes), new byte[50], 0, 0, false);
                Assert.AreEqual(0, cin.Position);
                // Field 1:
                uint tag = cin.ReadTag();
                Assert.AreEqual(1, tag >> 3);
                Assert.AreEqual(1, cin.Position);
                Assert.AreEqual(500, cin.ReadInt32());
                Assert.AreEqual(3, cin.Position);
                //Field 2:
                tag = cin.ReadTag();
                Assert.AreEqual(2, tag >> 3);
                Assert.AreEqual(4, cin.Position);
                int childlen = cin.ReadLength();
                Assert.AreEqual(120, childlen);
                Assert.AreEqual(5, cin.Position);
                int oldlimit = cin.PushLimit((int)childlen);
                Assert.AreEqual(5, cin.Position);
                // Now we are reading child message
                {
                    // Field 11: numeric value: 500
                    tag = cin.ReadTag();
                    Assert.AreEqual(11, tag >> 3);
                    Assert.AreEqual(6, cin.Position);
                    Assert.AreEqual(500, cin.ReadInt32());
                    Assert.AreEqual(8, cin.Position);
                    //Field 12: length delimited 120 bytes
                    tag = cin.ReadTag();
                    Assert.AreEqual(12, tag >> 3);
                    Assert.AreEqual(9, cin.Position);
                    ByteString bstr = cin.ReadBytes();
                    Assert.AreEqual(110, bstr.Length);
                    Assert.AreEqual((byte) 109, bstr[109]);
                    Assert.AreEqual(120, cin.Position);
                    // Field 13: fixed numeric value: 501
                    tag = cin.ReadTag();
                    Assert.AreEqual(13, tag >> 3);
                    // ROK - Previously broken here, this returned 126 failing to account for bufferSizeAfterLimit
                    Assert.AreEqual(121, cin.Position);
                    Assert.AreEqual(501, cin.ReadSFixed32());
                    Assert.AreEqual(125, cin.Position);
                    Assert.IsTrue(cin.IsAtEnd);
                }
                cin.PopLimit(oldlimit);
                Assert.AreEqual(125, cin.Position);
                // Field 3: fixed numeric value: 501
                tag = cin.ReadTag();
                Assert.AreEqual(3, tag >> 3);
                Assert.AreEqual(126, cin.Position);
                Assert.AreEqual(501, cin.ReadSFixed32());
                Assert.AreEqual(130, cin.Position);
                Assert.IsTrue(cin.IsAtEnd);
            }
        }

        [Test]
        public void Dispose_DisposesUnderlyingStream()
        {
            var memoryStream = new MemoryStream();
            Assert.IsTrue(memoryStream.CanWrite);
            using (var cos = new CodedOutputStream(memoryStream))
            {
                cos.WriteRawBytes(new byte[] {0});
                Assert.AreEqual(0, memoryStream.Position); // Not flushed yet
            }
            Assert.AreEqual(1, memoryStream.ToArray().Length); // Flushed data from CodedOutputStream to MemoryStream
            Assert.IsFalse(memoryStream.CanWrite); // Disposed
        }

        [Test]
        public void Dispose_WithLeaveOpen()
        {
            var memoryStream = new MemoryStream();
            Assert.IsTrue(memoryStream.CanWrite);
            using (var cos = new CodedOutputStream(memoryStream, true))
            {
                cos.WriteRawBytes(new byte[] {0});
                Assert.AreEqual(0, memoryStream.Position); // Not flushed yet
            }
            Assert.AreEqual(1, memoryStream.Position); // Flushed data from CodedOutputStream to MemoryStream
            Assert.IsTrue(memoryStream.CanWrite); // We left the stream open
        }

        [Test]
        public void Dispose_FromByteArray()
        {
            var stream = new CodedOutputStream(new byte[10]);
            stream.Dispose();
        }
    }
}