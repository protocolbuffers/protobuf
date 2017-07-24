#region Copyright notice and license
// Protocol Buffers - Google's data interchange format
// Copyright 2017 Google Inc.  All rights reserved.
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

// Tests in this file are direct equivalents of synchronous tests in CodedOutputStreamTest

#if !PROTOBUF_NO_ASYNC
using System;
using System.IO;
using System.Threading;
using System.Threading.Tasks;
using Google.Protobuf.TestProtos;
using NUnit.Framework;

namespace Google.Protobuf
{
    public class CodedOutputStreamAsyncTest
    {
        /// <summary>
        /// Writes the given value using WriteRawVarint32() and WriteRawVarint64() and
        /// checks that the result matches the given bytes
        /// </summary>
        private static async Task AssertWriteVarint(byte[] data, ulong value)
        {
            // Only do 32-bit write if the value fits in 32 bits.
            if ((value >> 32) == 0)
            {
                MemoryStream rawOutput = new MemoryStream();
                CodedOutputStream output = new CodedOutputStream(new AsyncOnlyStreamWrapper(rawOutput));
                await output.WriteRawVarint32Async((uint) value, CancellationToken.None);
                await output.FlushAsync(CancellationToken.None);
                Assert.AreEqual(data, rawOutput.ToArray());
                // Also try computing size.
                Assert.AreEqual(data.Length, CodedOutputStream.ComputeRawVarint32Size((uint) value));
            }

            {
                MemoryStream rawOutput = new MemoryStream();
                CodedOutputStream output = new CodedOutputStream(new AsyncOnlyStreamWrapper(rawOutput));
                await output.WriteRawVarint64Async(value, CancellationToken.None);
                await output.FlushAsync(CancellationToken.None);
                Assert.AreEqual(data, rawOutput.ToArray());

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
                        new CodedOutputStream(new AsyncOnlyStreamWrapper(rawOutput), bufferSize);
                    await output.WriteRawVarint32Async((uint) value, CancellationToken.None);
                    await output.FlushAsync(CancellationToken.None);
                    Assert.AreEqual(data, rawOutput.ToArray());
                }

                {
                    MemoryStream rawOutput = new MemoryStream();
                    CodedOutputStream output = new CodedOutputStream(new AsyncOnlyStreamWrapper(rawOutput), bufferSize);
                    await output.WriteRawVarint64Async(value, CancellationToken.None);
                    await output.FlushAsync(CancellationToken.None);
                    Assert.AreEqual(data, rawOutput.ToArray());
                }
            }
        }

        /// <summary>
        /// Tests WriteRawVarint32() and WriteRawVarint64()
        /// </summary>
        [Test]
        public async Task WriteVarint()
        {
            await AssertWriteVarint(new byte[] {0x00}, 0);
            await AssertWriteVarint(new byte[] {0x01}, 1);
            await AssertWriteVarint(new byte[] {0x7f}, 127);
            // 14882
            await AssertWriteVarint(new byte[] {0xa2, 0x74}, (0x22 << 0) | (0x74 << 7));
            // 2961488830
            await AssertWriteVarint(new byte[] {0xbe, 0xf7, 0x92, 0x84, 0x0b},
                              (0x3e << 0) | (0x77 << 7) | (0x12 << 14) | (0x04 << 21) |
                              (0x0bL << 28));

            // 64-bit
            // 7256456126
            await AssertWriteVarint(new byte[] {0xbe, 0xf7, 0x92, 0x84, 0x1b},
                              (0x3e << 0) | (0x77 << 7) | (0x12 << 14) | (0x04 << 21) |
                              (0x1bL << 28));
            // 41256202580718336
            await AssertWriteVarint(
                new byte[] {0x80, 0xe6, 0xeb, 0x9c, 0xc3, 0xc9, 0xa4, 0x49},
                (0x00 << 0) | (0x66 << 7) | (0x6b << 14) | (0x1c << 21) |
                (0x43UL << 28) | (0x49L << 35) | (0x24UL << 42) | (0x49UL << 49));
            // 11964378330978735131
            await AssertWriteVarint(
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
        private static async Task AssertWriteLittleEndian32(byte[] data, uint value)
        {
            MemoryStream rawOutput = new MemoryStream();
            CodedOutputStream output = new CodedOutputStream(new AsyncOnlyStreamWrapper(rawOutput));
            await output.WriteRawLittleEndian32Async(value, CancellationToken.None);
            await output.FlushAsync(CancellationToken.None);
            Assert.AreEqual(data, rawOutput.ToArray());

            // Try different buffer sizes.
            for (int bufferSize = 1; bufferSize <= 16; bufferSize *= 2)
            {
                rawOutput = new MemoryStream();
                output = new CodedOutputStream(new AsyncOnlyStreamWrapper(rawOutput), bufferSize);
                await output.WriteRawLittleEndian32Async(value, CancellationToken.None);
                await output.FlushAsync(CancellationToken.None);
                Assert.AreEqual(data, rawOutput.ToArray());
            }
        }

        /// <summary>
        /// Parses the given bytes using WriteRawLittleEndian64() and checks
        /// that the result matches the given value.
        /// </summary>
        private static async Task AssertWriteLittleEndian64(byte[] data, ulong value)
        {
            MemoryStream rawOutput = new MemoryStream();
            CodedOutputStream output = new CodedOutputStream(new AsyncOnlyStreamWrapper(rawOutput));
            await output.WriteRawLittleEndian64Async(value, CancellationToken.None);
            await output.FlushAsync(CancellationToken.None);
            Assert.AreEqual(data, rawOutput.ToArray());

            // Try different block sizes.
            for (int blockSize = 1; blockSize <= 16; blockSize *= 2)
            {
                rawOutput = new MemoryStream();
                output = new CodedOutputStream(new AsyncOnlyStreamWrapper(rawOutput), blockSize);
                await output.WriteRawLittleEndian64Async(value, CancellationToken.None);
                await output.FlushAsync(CancellationToken.None);
                Assert.AreEqual(data, rawOutput.ToArray());
            }
        }

        /// <summary>
        /// Tests writeRawLittleEndian32() and writeRawLittleEndian64().
        /// </summary>
        [Test]
        public async Task WriteLittleEndian()
        {
            await AssertWriteLittleEndian32(new byte[] {0x78, 0x56, 0x34, 0x12}, 0x12345678);
            await AssertWriteLittleEndian32(new byte[] {0xf0, 0xde, 0xbc, 0x9a}, 0x9abcdef0);

            await AssertWriteLittleEndian64(
                new byte[] {0xf0, 0xde, 0xbc, 0x9a, 0x78, 0x56, 0x34, 0x12},
                0x123456789abcdef0L);
            await AssertWriteLittleEndian64(
                new byte[] {0x78, 0x56, 0x34, 0x12, 0xf0, 0xde, 0xbc, 0x9a},
                0x9abcdef012345678UL);
        }

        [Test]
        public async Task WriteWholeMessage_VaryingBlockSizes()
        {
            TestAllTypes message = SampleMessages.CreateFullTestAllTypes();

            byte[] rawBytes = message.ToByteArray();

            // Try different block sizes.
            for (int blockSize = 1; blockSize < 256; blockSize *= 2)
            {
                MemoryStream rawOutput = new MemoryStream();
                CodedOutputStream output = new CodedOutputStream(new AsyncOnlyStreamWrapper(rawOutput), blockSize);
                await message.WriteToAsync(output, CancellationToken.None);
                await output.FlushAsync(CancellationToken.None);
                Assert.AreEqual(rawBytes, rawOutput.ToArray());
            }
        }

        [Test]
        public async Task TestNegativeEnumNoTag()
        {
            Assert.AreEqual(10, CodedOutputStream.ComputeInt32Size(-2));
            Assert.AreEqual(10, CodedOutputStream.ComputeEnumSize((int) SampleEnum.NegativeValue));

            byte[] bytes = new byte[10];
            // No AsyncOnlyStreamWrapper, because SpaceLeft can be called only on streams writing to a flat array
            CodedOutputStream output = new CodedOutputStream(bytes);
            await output.WriteEnumAsync((int) SampleEnum.NegativeValue, CancellationToken.None);

            Assert.AreEqual(0, output.SpaceLeft);
            Assert.AreEqual("FE-FF-FF-FF-FF-FF-FF-FF-FF-01", BitConverter.ToString(bytes));
        }

        [Test]
        public async Task TestCodedInputOutputPosition()
        {
            byte[] content = new byte[110];
            for (int i = 0; i < content.Length; i++)
                content[i] = (byte)i;

            byte[] child = new byte[120];
            {
                MemoryStream ms = new MemoryStream(child);
                CodedOutputStream cout = new CodedOutputStream(new AsyncOnlyStreamWrapper(ms), 20);
                // Field 11: numeric value: 500
                await cout.WriteTagAsync(11, WireFormat.WireType.Varint, CancellationToken.None);
                Assert.AreEqual(1, cout.Position);
                await cout.WriteInt32Async(500, CancellationToken.None);
                Assert.AreEqual(3, cout.Position);
                //Field 12: length delimited 120 bytes
                await cout.WriteTagAsync(12, WireFormat.WireType.LengthDelimited, CancellationToken.None);
                Assert.AreEqual(4, cout.Position);
                await cout.WriteBytesAsync(ByteString.CopyFrom(content), CancellationToken.None);
                Assert.AreEqual(115, cout.Position);
                // Field 13: fixed numeric value: 501
                await cout.WriteTagAsync(13, WireFormat.WireType.Fixed32, CancellationToken.None);
                Assert.AreEqual(116, cout.Position);
                await cout.WriteSFixed32Async(501, CancellationToken.None);
                Assert.AreEqual(120, cout.Position);
                await cout.FlushAsync(CancellationToken.None);
            }

            byte[] bytes = new byte[130];
            {
                CodedOutputStream cout = new CodedOutputStream(new AsyncOnlyStreamWrapper(bytes));
                // Field 1: numeric value: 500
                await cout.WriteTagAsync(1, WireFormat.WireType.Varint, CancellationToken.None);
                Assert.AreEqual(1, cout.Position);
                await cout.WriteInt32Async(500, CancellationToken.None);
                Assert.AreEqual(3, cout.Position);
                //Field 2: length delimited 120 bytes
                await cout.WriteTagAsync(2, WireFormat.WireType.LengthDelimited, CancellationToken.None);
                Assert.AreEqual(4, cout.Position);
                await cout.WriteBytesAsync(ByteString.CopyFrom(child), CancellationToken.None);
                Assert.AreEqual(125, cout.Position);
                // Field 3: fixed numeric value: 500
                await cout.WriteTagAsync(3, WireFormat.WireType.Fixed32, CancellationToken.None);
                Assert.AreEqual(126, cout.Position);
                await cout.WriteSFixed32Async(501, CancellationToken.None);
                Assert.AreEqual(130, cout.Position);
                await cout.FlushAsync(CancellationToken.None);
            }
            // Now test Input stream:
            {
                CodedInputStream cin = new CodedInputStream(new AsyncOnlyStreamWrapper(new MemoryStream(bytes)), new byte[50], 0, 0, false);
                Assert.AreEqual(0, cin.Position);
                // Field 1:
                uint tag = await cin.ReadTagAsync(CancellationToken.None);
                Assert.AreEqual(1, tag >> 3);
                Assert.AreEqual(1, cin.Position);
                Assert.AreEqual(500, await cin.ReadInt32Async(CancellationToken.None));
                Assert.AreEqual(3, cin.Position);
                //Field 2:
                tag = cin.ReadTag();
                Assert.AreEqual(2, tag >> 3);
                Assert.AreEqual(4, cin.Position);
                int childlen = await cin.ReadLengthAsync(CancellationToken.None);
                Assert.AreEqual(120, childlen);
                Assert.AreEqual(5, cin.Position);
                int oldlimit = cin.PushLimit((int)childlen);
                Assert.AreEqual(5, cin.Position);
                // Now we are reading child message
                {
                    // Field 11: numeric value: 500
                    tag = await cin.ReadTagAsync(CancellationToken.None);
                    Assert.AreEqual(11, tag >> 3);
                    Assert.AreEqual(6, cin.Position);
                    Assert.AreEqual(500, cin.ReadInt32());
                    Assert.AreEqual(8, cin.Position);
                    //Field 12: length delimited 120 bytes
                    tag = await cin.ReadTagAsync(CancellationToken.None);
                    Assert.AreEqual(12, tag >> 3);
                    Assert.AreEqual(9, cin.Position);
                    ByteString bstr = await cin.ReadBytesAsync(CancellationToken.None);
                    Assert.AreEqual(110, bstr.Length);
                    Assert.AreEqual((byte) 109, bstr[109]);
                    Assert.AreEqual(120, cin.Position);
                    // Field 13: fixed numeric value: 501
                    tag = await cin.ReadTagAsync(CancellationToken.None);
                    Assert.AreEqual(13, tag >> 3);
                    // ROK - Previously broken here, this returned 126 failing to account for bufferSizeAfterLimit
                    Assert.AreEqual(121, cin.Position);
                    Assert.AreEqual(501, await cin.ReadSFixed32Async(CancellationToken.None));
                    Assert.AreEqual(125, cin.Position);
                    Assert.IsTrue(await cin.IsAtEndAsync(CancellationToken.None));
                }
                cin.PopLimit(oldlimit);
                Assert.AreEqual(125, cin.Position);
                // Field 3: fixed numeric value: 501
                tag = await cin.ReadTagAsync(CancellationToken.None);
                Assert.AreEqual(3, tag >> 3);
                Assert.AreEqual(126, cin.Position);
                Assert.AreEqual(501, await cin.ReadSFixed32Async(CancellationToken.None));
                Assert.AreEqual(130, cin.Position);
                Assert.IsTrue(await cin.IsAtEndAsync(CancellationToken.None));
            }
        }

        [Test]
        public async Task Dispose_DisposesUnderlyingStream()
        {
            var memoryStream = new MemoryStream();
            Assert.IsTrue(memoryStream.CanWrite);
            // No AsyncOnlyStreamWrapper, because Dispose is synchronous by design
            using (var cos = new CodedOutputStream(memoryStream))
            {
                await cos.WriteRawByteAsync(0, CancellationToken.None);
                Assert.AreEqual(0, memoryStream.Position); // Not flushed yet
            }
            Assert.AreEqual(1, memoryStream.ToArray().Length); // Flushed data from CodedOutputStream to MemoryStream
            Assert.IsFalse(memoryStream.CanWrite); // Disposed
        }

        [Test]
        public async Task Dispose_WithLeaveOpen()
        {
            var memoryStream = new MemoryStream();
            Assert.IsTrue(memoryStream.CanWrite);
            // No AsyncOnlyStreamWrapper, because Dispose is synchronous by design
            using (var cos = new CodedOutputStream(memoryStream, true))
            {
                await cos.WriteRawByteAsync(0, CancellationToken.None);
                Assert.AreEqual(0, memoryStream.Position); // Not flushed yet
            }
            Assert.AreEqual(1, memoryStream.Position); // Flushed data from CodedOutputStream to MemoryStream
            Assert.IsTrue(memoryStream.CanWrite); // We left the stream open
        }
    }
}
#endif