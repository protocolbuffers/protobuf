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
using System.Text;
using Google.Protobuf.Buffers;
using Google.Protobuf.TestProtos;
using NUnit.Framework;

namespace Google.Protobuf
{
    public class CodedOutputWriterTest : CodedOutputTestBase
    {
        [Test]
        public void WriteWholeMessage_VaryingSpanSizes()
        {
            TestAllTypes message = SampleMessages.CreateFullTestAllTypes();

            byte[] rawBytes = message.ToByteArray();

            // Try different block sizes.
            for (int blockSize = 1; blockSize < 256; blockSize *= 2)
            {
                ArrayBufferWriter<byte> rawOutput = new ArrayBufferWriter<byte>(1024);
                CodedOutputWriter output = new CodedOutputWriter(new MaxSizeHintBufferWriter<byte>(rawOutput, blockSize));

                message.WriteTo(ref output);
                output.Flush();
                Assert.AreEqual(rawBytes, rawOutput.WrittenSpan.ToArray());
            }
        }

        [Test]
        public void TestCodedInputOutputPosition()
        {
            byte[] content = new byte[110];
            for (int i = 0; i < content.Length; i++)
                content[i] = (byte)i;

            byte[] child;
            {
                ArrayBufferWriter<byte> rawOutput = new ArrayBufferWriter<byte>(1024);
                CodedOutputWriter cout = new CodedOutputWriter(rawOutput);
                // Field 11: numeric value: 500
                cout.WriteTag(11, WireFormat.WireType.Varint);
                cout.Flush();
                Assert.AreEqual(1, rawOutput.WrittenCount);
                cout.WriteInt32(500);
                cout.Flush();
                Assert.AreEqual(3, rawOutput.WrittenCount);
                //Field 12: length delimited 120 bytes
                cout.WriteTag(12, WireFormat.WireType.LengthDelimited);
                cout.Flush();
                Assert.AreEqual(4, rawOutput.WrittenCount);
                cout.WriteBytes(ByteString.CopyFrom(content));
                cout.Flush();
                Assert.AreEqual(115, rawOutput.WrittenCount);
                // Field 13: fixed numeric value: 501
                cout.WriteTag(13, WireFormat.WireType.Fixed32);
                cout.Flush();
                Assert.AreEqual(116, rawOutput.WrittenCount);
                cout.WriteSFixed32(501);
                cout.Flush();
                Assert.AreEqual(120, rawOutput.WrittenCount);

                child = rawOutput.WrittenSpan.ToArray();
            }

            byte[] bytes;
            {
                ArrayBufferWriter<byte> rawOutput = new ArrayBufferWriter<byte>(1024);
                CodedOutputWriter cout = new CodedOutputWriter(rawOutput);
                // Field 1: numeric value: 500
                cout.WriteTag(1, WireFormat.WireType.Varint);
                cout.Flush();
                Assert.AreEqual(1, rawOutput.WrittenCount);
                cout.WriteInt32(500);
                cout.Flush();
                Assert.AreEqual(3, rawOutput.WrittenCount);
                //Field 2: length delimited 120 bytes
                cout.WriteTag(2, WireFormat.WireType.LengthDelimited);
                cout.Flush();
                Assert.AreEqual(4, rawOutput.WrittenCount);
                cout.WriteBytes(ByteString.CopyFrom(child));
                cout.Flush();
                Assert.AreEqual(125, rawOutput.WrittenCount);
                // Field 3: fixed numeric value: 500
                cout.WriteTag(3, WireFormat.WireType.Fixed32);
                cout.Flush();
                Assert.AreEqual(126, rawOutput.WrittenCount);
                cout.WriteSFixed32(501);
                cout.Flush();
                Assert.AreEqual(130, rawOutput.WrittenCount);

                bytes = rawOutput.WrittenSpan.ToArray();
            }

            // Now test Input stream:
            {
                CodedInputReader cin = new CodedInputReader(new ReadOnlySequence<byte>(bytes));
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
                }
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

        protected override void AssertWriteVarint(byte[] data, ulong value)
        {
            // Only do 32-bit write if the value fits in 32 bits.
            if ((value >> 32) == 0)
            {
                ArrayBufferWriter<byte> rawOutput = new ArrayBufferWriter<byte>(1024);
                CodedOutputWriter output = new CodedOutputWriter(rawOutput);
                output.WriteRawVarint32((uint)value);
                output.Flush();
                Assert.AreEqual(data, rawOutput.WrittenSpan.ToArray());
                // Also try computing size.
                Assert.AreEqual(data.Length, CodedOutputStream.ComputeRawVarint32Size((uint)value));
            }

            {
                ArrayBufferWriter<byte> rawOutput = new ArrayBufferWriter<byte>(1024);
                CodedOutputWriter output = new CodedOutputWriter(rawOutput);
                output.WriteRawVarint64(value);
                output.Flush();
                Assert.AreEqual(data, rawOutput.WrittenSpan.ToArray());

                // Also try computing size.
                Assert.AreEqual(data.Length, CodedOutputStream.ComputeRawVarint64Size(value));
            }
        }

        protected override void AssertWriteLittleEndian32(byte[] data, uint value)
        {
            ArrayBufferWriter<byte> rawOutput = new ArrayBufferWriter<byte>(1024);
            CodedOutputWriter output = new CodedOutputWriter(rawOutput);
            output.WriteRawLittleEndian32(value);
            output.Flush();
            Assert.AreEqual(data, rawOutput.WrittenSpan.ToArray());
        }

        protected override void AssertWriteLittleEndian64(byte[] data, ulong value)
        {
            ArrayBufferWriter<byte> rawOutput = new ArrayBufferWriter<byte>(1024);
            CodedOutputWriter output = new CodedOutputWriter(rawOutput);
            output.WriteRawLittleEndian64(value);
            output.Flush();
            Assert.AreEqual(data, rawOutput.WrittenSpan.ToArray());
        }

        protected override void AssertWriteEnum(byte[] data, int value)
        {
            ArrayBufferWriter<byte> rawOutput = new ArrayBufferWriter<byte>(10);
            CodedOutputWriter output = new CodedOutputWriter(rawOutput);
            output.WriteEnum(value);
            output.Flush();

            Assert.AreEqual(data, rawOutput.WrittenSpan.ToArray());
        }

        protected override void AssertWriteString(string value)
        {
            ArrayBufferWriter<byte> rawOutput = new ArrayBufferWriter<byte>(1024);
            // Limit the max span size to force string to be written in multiple parts
            CodedOutputWriter output = new CodedOutputWriter(new MaxSizeHintBufferWriter<byte>(rawOutput, 128));
            output.WriteString(value);
            output.Flush();

            CodedInputReader input = new CodedInputReader(ReadOnlySequenceFactory.CreateWithContent(rawOutput.WrittenSpan.ToArray()));
            Assert.AreEqual(value, input.ReadString());
        }

        protected override void AssertWriteBytes(ByteString value)
        {
            ArrayBufferWriter<byte> rawOutput = new ArrayBufferWriter<byte>(1024);
            // Limit the max span size to force bytes to be written in multiple parts
            CodedOutputWriter output = new CodedOutputWriter(new MaxSizeHintBufferWriter<byte>(rawOutput, 128));
            output.WriteBytes(value);
            output.Flush();

            CodedInputReader input = new CodedInputReader(ReadOnlySequenceFactory.CreateWithContent(rawOutput.WrittenSpan.ToArray()));
            Assert.AreEqual(value, input.ReadBytes());
        }

        protected override void AssertWriteFloat(byte[] data, float value)
        {
            ArrayBufferWriter<byte> rawOutput = new ArrayBufferWriter<byte>(1024);
            CodedOutputWriter output = new CodedOutputWriter(rawOutput);
            output.WriteFloat(value);
            output.Flush();

            Assert.AreEqual(data, rawOutput.WrittenSpan.ToArray());
        }

        protected override void AssertWriteDouble(byte[] data, double value)
        {
            ArrayBufferWriter<byte> rawOutput = new ArrayBufferWriter<byte>(1024);
            CodedOutputWriter output = new CodedOutputWriter(rawOutput);
            output.WriteDouble(value);
            output.Flush();

            Assert.AreEqual(data, rawOutput.WrittenSpan.ToArray());
        }

        protected override void AssertWriteRawTag(byte[] data, int value)
        {
            ArrayBufferWriter<byte> rawOutput = new ArrayBufferWriter<byte>(1024);
            CodedOutputWriter output = new CodedOutputWriter(rawOutput);

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

            CodedInputReader input = new CodedInputReader(new ReadOnlySequence<byte>(rawOutput.WrittenSpan.ToArray()));
            Assert.AreEqual(value, input.ReadTag());
        }
    }
}
#endif