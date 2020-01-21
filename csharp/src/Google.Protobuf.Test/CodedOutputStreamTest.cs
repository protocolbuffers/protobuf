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
using System.Text;
using Google.Protobuf.TestProtos;
using NUnit.Framework;

namespace Google.Protobuf
{
    public class CodedOutputStreamTest : CodedOutputTestBase
    {
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
            }
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
                cos.WriteRawByte(0);
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
                cos.WriteRawByte(0);
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

        protected override void AssertWriteVarint(byte[] data, ulong value)
        {
            // Only do 32-bit write if the value fits in 32 bits.
            if ((value >> 32) == 0)
            {
                MemoryStream rawOutput = new MemoryStream();
                CodedOutputStream output = new CodedOutputStream(rawOutput);
                output.WriteRawVarint32((uint)value);
                output.Flush();
                Assert.AreEqual(data, rawOutput.ToArray());
                // Also try computing size.
                Assert.AreEqual(data.Length, CodedOutputStream.ComputeRawVarint32Size((uint)value));
            }

            {
                MemoryStream rawOutput = new MemoryStream();
                CodedOutputStream output = new CodedOutputStream(rawOutput);
                output.WriteRawVarint64(value);
                output.Flush();
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
                        new CodedOutputStream(rawOutput, bufferSize);
                    output.WriteRawVarint32((uint)value);
                    output.Flush();
                    Assert.AreEqual(data, rawOutput.ToArray());
                }

                {
                    MemoryStream rawOutput = new MemoryStream();
                    CodedOutputStream output = new CodedOutputStream(rawOutput, bufferSize);
                    output.WriteRawVarint64(value);
                    output.Flush();
                    Assert.AreEqual(data, rawOutput.ToArray());
                }
            }
        }

        protected override void AssertWriteLittleEndian32(byte[] data, uint value)
        {
            MemoryStream rawOutput = new MemoryStream();
            CodedOutputStream output = new CodedOutputStream(rawOutput);
            output.WriteRawLittleEndian32(value);
            output.Flush();
            Assert.AreEqual(data, rawOutput.ToArray());

            // Try different buffer sizes.
            for (int bufferSize = 1; bufferSize <= 16; bufferSize *= 2)
            {
                rawOutput = new MemoryStream();
                output = new CodedOutputStream(rawOutput, bufferSize);
                output.WriteRawLittleEndian32(value);
                output.Flush();
                Assert.AreEqual(data, rawOutput.ToArray());
            }
        }

        protected override void AssertWriteLittleEndian64(byte[] data, ulong value)
        {
            MemoryStream rawOutput = new MemoryStream();
            CodedOutputStream output = new CodedOutputStream(rawOutput);
            output.WriteRawLittleEndian64(value);
            output.Flush();
            Assert.AreEqual(data, rawOutput.ToArray());

            // Try different block sizes.
            for (int blockSize = 1; blockSize <= 16; blockSize *= 2)
            {
                rawOutput = new MemoryStream();
                output = new CodedOutputStream(rawOutput, blockSize);
                output.WriteRawLittleEndian64(value);
                output.Flush();
                Assert.AreEqual(data, rawOutput.ToArray());
            }
        }

        protected override void AssertWriteEnum(byte[] data, int value)
        {
            MemoryStream rawOutput = new MemoryStream();
            CodedOutputStream output = new CodedOutputStream(rawOutput);
            output.WriteEnum(value);
            output.Flush();

            Assert.AreEqual(data, rawOutput.ToArray());
        }

        protected override void AssertWriteBytes(ByteString value)
        {
            MemoryStream rawOutput = new MemoryStream();
            CodedOutputStream output = new CodedOutputStream(rawOutput);
            output.WriteBytes(value);
            output.Flush();

            CodedInputStream input = new CodedInputStream(rawOutput.ToArray());
            Assert.AreEqual(value, input.ReadBytes());
        }

        protected override void AssertWriteString(string value)
        {
            MemoryStream rawOutput = new MemoryStream();
            CodedOutputStream output = new CodedOutputStream(rawOutput);
            output.WriteString(value);
            output.Flush();

            CodedInputStream input = new CodedInputStream(rawOutput.ToArray());
            Assert.AreEqual(value, input.ReadString());
        }

        protected override void AssertWriteFloat(byte[] data, float value)
        {
            MemoryStream rawOutput = new MemoryStream();
            CodedOutputStream output = new CodedOutputStream(rawOutput);
            output.WriteFloat(value);
            output.Flush();
            Assert.AreEqual(data, rawOutput.ToArray());
        }

        protected override void AssertWriteDouble(byte[] data, double value)
        {
            MemoryStream rawOutput = new MemoryStream();
            CodedOutputStream output = new CodedOutputStream(rawOutput);
            output.WriteDouble(value);
            output.Flush();
            Assert.AreEqual(data, rawOutput.ToArray());
        }

        protected override void AssertWriteRawTag(byte[] data, int value)
        {
            MemoryStream rawOutput = new MemoryStream();
            CodedOutputStream output = new CodedOutputStream(rawOutput);

            switch (data.Length)
            {
                case 1:
                    output.WriteRawTag(data[0]);
                    break;
                case 2:
                    output.WriteRawTag(data[0], data[1]);
                    break;
                case 3:
                    output.WriteRawTag(data[0], data[1], data[2]);
                    break;
                case 4:
                    output.WriteRawTag(data[0], data[1], data[2], data[3]);
                    break;
                case 5:
                    output.WriteRawTag(data[0], data[1], data[2], data[3], data[4]);
                    break;
                default:
                    throw new ArgumentException();
            }

            output.Flush();

            CodedInputStream input = new CodedInputStream(rawOutput.ToArray());
            Assert.AreEqual(value, input.ReadTag());
        }
    }
}