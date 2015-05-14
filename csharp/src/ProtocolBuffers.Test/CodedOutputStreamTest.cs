#region Copyright notice and license

// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
// http://github.com/jskeet/dotnet-protobufs/
// Original C++/Java/Python code:
// http://code.google.com/p/protobuf/
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
using System.Collections.Generic;
using System.IO;
using Google.ProtocolBuffers.TestProtos;
using NUnit.Framework;

namespace Google.ProtocolBuffers
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
                MemoryStream rawOutput = new MemoryStream();
                CodedOutputStream output = CodedOutputStream.CreateInstance(rawOutput);
                output.WriteRawVarint32((uint) value);
                output.Flush();
                Assert.AreEqual(data, rawOutput.ToArray());
                // Also try computing size.
                Assert.AreEqual(data.Length, CodedOutputStream.ComputeRawVarint32Size((uint) value));
            }

            {
                MemoryStream rawOutput = new MemoryStream();
                CodedOutputStream output = CodedOutputStream.CreateInstance(rawOutput);
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
                        CodedOutputStream.CreateInstance(rawOutput, bufferSize);
                    output.WriteRawVarint32((uint) value);
                    output.Flush();
                    Assert.AreEqual(data, rawOutput.ToArray());
                }

                {
                    MemoryStream rawOutput = new MemoryStream();
                    CodedOutputStream output = CodedOutputStream.CreateInstance(rawOutput, bufferSize);
                    output.WriteRawVarint64(value);
                    output.Flush();
                    Assert.AreEqual(data, rawOutput.ToArray());
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
            MemoryStream rawOutput = new MemoryStream();
            CodedOutputStream output = CodedOutputStream.CreateInstance(rawOutput);
            output.WriteRawLittleEndian32(value);
            output.Flush();
            Assert.AreEqual(data, rawOutput.ToArray());

            // Try different buffer sizes.
            for (int bufferSize = 1; bufferSize <= 16; bufferSize *= 2)
            {
                rawOutput = new MemoryStream();
                output = CodedOutputStream.CreateInstance(rawOutput, bufferSize);
                output.WriteRawLittleEndian32(value);
                output.Flush();
                Assert.AreEqual(data, rawOutput.ToArray());
            }
        }

        /// <summary>
        /// Parses the given bytes using WriteRawLittleEndian64() and checks
        /// that the result matches the given value.
        /// </summary>
        private static void AssertWriteLittleEndian64(byte[] data, ulong value)
        {
            MemoryStream rawOutput = new MemoryStream();
            CodedOutputStream output = CodedOutputStream.CreateInstance(rawOutput);
            output.WriteRawLittleEndian64(value);
            output.Flush();
            Assert.AreEqual(data, rawOutput.ToArray());

            // Try different block sizes.
            for (int blockSize = 1; blockSize <= 16; blockSize *= 2)
            {
                rawOutput = new MemoryStream();
                output = CodedOutputStream.CreateInstance(rawOutput, blockSize);
                output.WriteRawLittleEndian64(value);
                output.Flush();
                Assert.AreEqual(data, rawOutput.ToArray());
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
        public void WriteWholeMessage()
        {
            TestAllTypes message = TestUtil.GetAllSet();

            byte[] rawBytes = message.ToByteArray();
            TestUtil.AssertEqualBytes(TestUtil.GoldenMessage.ToByteArray(), rawBytes);

            // Try different block sizes.
            for (int blockSize = 1; blockSize < 256; blockSize *= 2)
            {
                MemoryStream rawOutput = new MemoryStream();
                CodedOutputStream output =
                    CodedOutputStream.CreateInstance(rawOutput, blockSize);
                message.WriteTo(output);
                output.Flush();
                TestUtil.AssertEqualBytes(rawBytes, rawOutput.ToArray());
            }
        }

        /// <summary>
        /// Tests writing a whole message with every packed field type. Ensures the
        /// wire format of packed fields is compatible with C++.
        /// </summary>
        [Test]
        public void WriteWholePackedFieldsMessage()
        {
            TestPackedTypes message = TestUtil.GetPackedSet();

            byte[] rawBytes = message.ToByteArray();
            TestUtil.AssertEqualBytes(TestUtil.GetGoldenPackedFieldsMessage().ToByteArray(),
                                      rawBytes);
        }

        [Test]
        public void EncodeZigZag32()
        {
            Assert.AreEqual(0u, CodedOutputStream.EncodeZigZag32(0));
            Assert.AreEqual(1u, CodedOutputStream.EncodeZigZag32(-1));
            Assert.AreEqual(2u, CodedOutputStream.EncodeZigZag32(1));
            Assert.AreEqual(3u, CodedOutputStream.EncodeZigZag32(-2));
            Assert.AreEqual(0x7FFFFFFEu, CodedOutputStream.EncodeZigZag32(0x3FFFFFFF));
            Assert.AreEqual(0x7FFFFFFFu, CodedOutputStream.EncodeZigZag32(unchecked((int) 0xC0000000)));
            Assert.AreEqual(0xFFFFFFFEu, CodedOutputStream.EncodeZigZag32(0x7FFFFFFF));
            Assert.AreEqual(0xFFFFFFFFu, CodedOutputStream.EncodeZigZag32(unchecked((int) 0x80000000)));
        }

        [Test]
        public void EncodeZigZag64()
        {
            Assert.AreEqual(0u, CodedOutputStream.EncodeZigZag64(0));
            Assert.AreEqual(1u, CodedOutputStream.EncodeZigZag64(-1));
            Assert.AreEqual(2u, CodedOutputStream.EncodeZigZag64(1));
            Assert.AreEqual(3u, CodedOutputStream.EncodeZigZag64(-2));
            Assert.AreEqual(0x000000007FFFFFFEuL,
                            CodedOutputStream.EncodeZigZag64(unchecked((long) 0x000000003FFFFFFFUL)));
            Assert.AreEqual(0x000000007FFFFFFFuL,
                            CodedOutputStream.EncodeZigZag64(unchecked((long) 0xFFFFFFFFC0000000UL)));
            Assert.AreEqual(0x00000000FFFFFFFEuL,
                            CodedOutputStream.EncodeZigZag64(unchecked((long) 0x000000007FFFFFFFUL)));
            Assert.AreEqual(0x00000000FFFFFFFFuL,
                            CodedOutputStream.EncodeZigZag64(unchecked((long) 0xFFFFFFFF80000000UL)));
            Assert.AreEqual(0xFFFFFFFFFFFFFFFEL,
                            CodedOutputStream.EncodeZigZag64(unchecked((long) 0x7FFFFFFFFFFFFFFFUL)));
            Assert.AreEqual(0xFFFFFFFFFFFFFFFFL,
                            CodedOutputStream.EncodeZigZag64(unchecked((long) 0x8000000000000000UL)));
        }

        [Test]
        public void RoundTripZigZag32()
        {
            // Some easier-to-verify round-trip tests.  The inputs (other than 0, 1, -1)
            // were chosen semi-randomly via keyboard bashing.
            Assert.AreEqual(0, CodedInputStream.DecodeZigZag32(CodedOutputStream.EncodeZigZag32(0)));
            Assert.AreEqual(1, CodedInputStream.DecodeZigZag32(CodedOutputStream.EncodeZigZag32(1)));
            Assert.AreEqual(-1, CodedInputStream.DecodeZigZag32(CodedOutputStream.EncodeZigZag32(-1)));
            Assert.AreEqual(14927, CodedInputStream.DecodeZigZag32(CodedOutputStream.EncodeZigZag32(14927)));
            Assert.AreEqual(-3612, CodedInputStream.DecodeZigZag32(CodedOutputStream.EncodeZigZag32(-3612)));
        }

        [Test]
        public void RoundTripZigZag64()
        {
            Assert.AreEqual(0, CodedInputStream.DecodeZigZag64(CodedOutputStream.EncodeZigZag64(0)));
            Assert.AreEqual(1, CodedInputStream.DecodeZigZag64(CodedOutputStream.EncodeZigZag64(1)));
            Assert.AreEqual(-1, CodedInputStream.DecodeZigZag64(CodedOutputStream.EncodeZigZag64(-1)));
            Assert.AreEqual(14927, CodedInputStream.DecodeZigZag64(CodedOutputStream.EncodeZigZag64(14927)));
            Assert.AreEqual(-3612, CodedInputStream.DecodeZigZag64(CodedOutputStream.EncodeZigZag64(-3612)));

            Assert.AreEqual(856912304801416L,
                            CodedInputStream.DecodeZigZag64(CodedOutputStream.EncodeZigZag64(856912304801416L)));
            Assert.AreEqual(-75123905439571256L,
                            CodedInputStream.DecodeZigZag64(CodedOutputStream.EncodeZigZag64(-75123905439571256L)));
        }

        [Test]
        public void TestNegativeEnumNoTag()
        {
            Assert.AreEqual(10, CodedOutputStream.ComputeInt32SizeNoTag(-2));
            Assert.AreEqual(10, CodedOutputStream.ComputeEnumSizeNoTag(-2));

            byte[] bytes = new byte[10];
            CodedOutputStream output = CodedOutputStream.CreateInstance(bytes);
            output.WriteEnumNoTag(-2);

            Assert.AreEqual(0, output.SpaceLeft);
            Assert.AreEqual("FE-FF-FF-FF-FF-FF-FF-FF-FF-01", BitConverter.ToString(bytes));
        }

        [Test]
        public void TestNegativeEnumWithTag()
        {
            Assert.AreEqual(11, CodedOutputStream.ComputeInt32Size(8, -2));
            Assert.AreEqual(11, CodedOutputStream.ComputeEnumSize(8, -2));

            byte[] bytes = new byte[11];
            CodedOutputStream output = CodedOutputStream.CreateInstance(bytes);
            output.WriteEnum(8, "", -2, -2);

            Assert.AreEqual(0, output.SpaceLeft);
            //fyi, 0x40 == 0x08 << 3 + 0, field num + wire format shift
            Assert.AreEqual("40-FE-FF-FF-FF-FF-FF-FF-FF-FF-01", BitConverter.ToString(bytes));
        }

        [Test]
        public void TestNegativeEnumArrayPacked()
        {
            int arraySize = 1 + (10 * 5);
            int msgSize = 1 + 1 + arraySize;
            byte[] bytes = new byte[msgSize];
            CodedOutputStream output = CodedOutputStream.CreateInstance(bytes);
            output.WritePackedEnumArray(8, "", arraySize, new int[] { 0, -1, -2, -3, -4, -5 });

            Assert.AreEqual(0, output.SpaceLeft);

            CodedInputStream input = CodedInputStream.CreateInstance(bytes);
            uint tag;
            string name;
            Assert.IsTrue(input.ReadTag(out tag, out name));

            List<int> values = new List<int>();
            input.ReadInt32Array(tag, name, values);

            Assert.AreEqual(6, values.Count);
            for (int i = 0; i > -6; i--)
                Assert.AreEqual(i, values[Math.Abs(i)]);
        }

        [Test]
        public void TestNegativeEnumArray()
        {
            int arraySize = 1 + 1 + (11 * 5);
            int msgSize = arraySize;
            byte[] bytes = new byte[msgSize];
            CodedOutputStream output = CodedOutputStream.CreateInstance(bytes);
            output.WriteEnumArray(8, "", new int[] { 0, -1, -2, -3, -4, -5 });

            Assert.AreEqual(0, output.SpaceLeft);

            CodedInputStream input = CodedInputStream.CreateInstance(bytes);
            uint tag;
            string name;
            Assert.IsTrue(input.ReadTag(out tag, out name));

            List<int> values = new List<int>();
            input.ReadInt32Array(tag, name, values);

            Assert.AreEqual(6, values.Count);
            for (int i = 0; i > -6; i--)
                Assert.AreEqual(i, values[Math.Abs(i)]);
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
                CodedOutputStream cout = CodedOutputStream.CreateInstance(ms, 20);
                // Field 11: numeric value: 500
                cout.WriteTag(11, WireFormat.WireType.Varint);
                Assert.AreEqual(1, cout.Position);
                cout.WriteInt32NoTag(500);
                Assert.AreEqual(3, cout.Position);
                //Field 12: length delimited 120 bytes
                cout.WriteTag(12, WireFormat.WireType.LengthDelimited);
                Assert.AreEqual(4, cout.Position);
                cout.WriteBytesNoTag(ByteString.CopyFrom(content));
                Assert.AreEqual(115, cout.Position);
                // Field 13: fixed numeric value: 501
                cout.WriteTag(13, WireFormat.WireType.Fixed32);
                Assert.AreEqual(116, cout.Position);
                cout.WriteSFixed32NoTag(501);
                Assert.AreEqual(120, cout.Position);
                cout.Flush();
            }

            byte[] bytes = new byte[130];
            {
                CodedOutputStream cout = CodedOutputStream.CreateInstance(bytes);
                // Field 1: numeric value: 500
                cout.WriteTag(1, WireFormat.WireType.Varint);
                Assert.AreEqual(1, cout.Position);
                cout.WriteInt32NoTag(500);
                Assert.AreEqual(3, cout.Position);
                //Field 2: length delimited 120 bytes
                cout.WriteTag(2, WireFormat.WireType.LengthDelimited);
                Assert.AreEqual(4, cout.Position);
                cout.WriteBytesNoTag(ByteString.CopyFrom(child));
                Assert.AreEqual(125, cout.Position);
                // Field 3: fixed numeric value: 500
                cout.WriteTag(3, WireFormat.WireType.Fixed32);
                Assert.AreEqual(126, cout.Position);
                cout.WriteSFixed32NoTag(501);
                Assert.AreEqual(130, cout.Position);
                cout.Flush();
            }
            //Now test Input stream:
            {
                CodedInputStream cin = CodedInputStream.CreateInstance(new MemoryStream(bytes), new byte[50]);
                uint tag;
                int intValue = 0;
                string ignore;
                Assert.AreEqual(0, cin.Position);
                // Field 1:
                Assert.IsTrue(cin.ReadTag(out tag, out ignore) && tag >> 3 == 1);
                Assert.AreEqual(1, cin.Position);
                Assert.IsTrue(cin.ReadInt32(ref intValue) && intValue == 500);
                Assert.AreEqual(3, cin.Position);
                //Field 2:
                Assert.IsTrue(cin.ReadTag(out tag, out ignore) && tag >> 3 == 2);
                Assert.AreEqual(4, cin.Position);
                uint childlen = cin.ReadRawVarint32();
                Assert.AreEqual(120u, childlen);
                Assert.AreEqual(5, cin.Position);
                int oldlimit = cin.PushLimit((int)childlen);
                Assert.AreEqual(5, cin.Position);
                // Now we are reading child message
                {
                    // Field 11: numeric value: 500
                    Assert.IsTrue(cin.ReadTag(out tag, out ignore) && tag >> 3 == 11);
                    Assert.AreEqual(6, cin.Position);
                    Assert.IsTrue(cin.ReadInt32(ref intValue) && intValue == 500);
                    Assert.AreEqual(8, cin.Position);
                    //Field 12: length delimited 120 bytes
                    Assert.IsTrue(cin.ReadTag(out tag, out ignore) && tag >> 3 == 12);
                    Assert.AreEqual(9, cin.Position);
                    ByteString bstr = null;
                    Assert.IsTrue(cin.ReadBytes(ref bstr) && bstr.Length == 110 && bstr.ToByteArray()[109] == 109);
                    Assert.AreEqual(120, cin.Position);
                    // Field 13: fixed numeric value: 501
                    Assert.IsTrue(cin.ReadTag(out tag, out ignore) && tag >> 3 == 13);
                    // ROK - Previously broken here, this returned 126 failing to account for bufferSizeAfterLimit
                    Assert.AreEqual(121, cin.Position);
                    Assert.IsTrue(cin.ReadSFixed32(ref intValue) && intValue == 501);
                    Assert.AreEqual(125, cin.Position);
                    Assert.IsTrue(cin.IsAtEnd);
                }
                cin.PopLimit(oldlimit);
                Assert.AreEqual(125, cin.Position);
                // Field 3: fixed numeric value: 501
                Assert.IsTrue(cin.ReadTag(out tag, out ignore) && tag >> 3 == 3);
                Assert.AreEqual(126, cin.Position);
                Assert.IsTrue(cin.ReadSFixed32(ref intValue) && intValue == 501);
                Assert.AreEqual(130, cin.Position);
                Assert.IsTrue(cin.IsAtEnd);
            }
        }
    }
}