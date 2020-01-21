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

using System.Text;
using NUnit.Framework;

namespace Google.Protobuf
{
    public abstract class CodedOutputTestBase
    {
        /// <summary>
        /// Writes the given value using WriteRawVarint32() and WriteRawVarint64() and
        /// checks that the result matches the given bytes
        /// </summary>
        protected abstract void AssertWriteVarint(byte[] data, ulong value);

        /// <summary>
        /// Parses the given bytes using WriteRawLittleEndian32() and checks
        /// that the result matches the given value.
        /// </summary>
        protected abstract void AssertWriteLittleEndian32(byte[] data, uint value);

        /// <summary>
        /// Parses the given bytes using WriteRawLittleEndian64() and checks
        /// that the result matches the given value.
        /// </summary>
        protected abstract void AssertWriteLittleEndian64(byte[] data, ulong value);

        protected abstract void AssertWriteEnum(byte[] data, int value);

        protected abstract void AssertWriteString(string value);

        protected abstract void AssertWriteBytes(ByteString value);

        protected abstract void AssertWriteFloat(byte[] data, float value);

        protected abstract void AssertWriteDouble(byte[] data, double value);

        protected abstract void AssertWriteRawTag(byte[] data, int value);

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
            Assert.AreEqual(10, CodedOutputStream.ComputeInt32Size(-2));
            Assert.AreEqual(10, CodedOutputStream.ComputeEnumSize((int) SampleEnum.NegativeValue));

            byte[] bytes = { 0xFE, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x01 };

            AssertWriteEnum(bytes, (int)SampleEnum.NegativeValue);
        }

        [Test]
        public void WriteSmallBytes()
        {
            byte[] data = new byte[32];
            for (int i = 0; i < data.Length; i++)
            {
                data[i] = (byte)i;
            }

            AssertWriteBytes(ByteString.CopyFrom(data));
        }

        [Test]
        public void WriteLargeBytes()
        {
            byte[] data = new byte[2048];
            for (int i = 0; i < data.Length; i++)
            {
                data[i] = (byte)i;
            }

            AssertWriteBytes(ByteString.CopyFrom(data));
        }

        [Test]
        public void WriteAsciiSmallString()
        {
            StringBuilder sb = new StringBuilder();
            for (int i = 0; i < 32; i++)
            {
                sb.AppendLine(i.ToString());
            }

            AssertWriteString(sb.ToString());
        }

        [Test]
        public void WriteUnicodeSmallString()
        {
            StringBuilder sb = new StringBuilder();
            for (int i = 0; i < 32; i++)
            {
                sb.AppendLine(i.ToString().Replace("0", "\x1369"));
            }

            AssertWriteString(sb.ToString());
        }

        [Test]
        public void WriteAsciiLargeString()
        {
            StringBuilder sb = new StringBuilder();
            for (int i = 0; i < 2048; i++)
            {
                sb.AppendLine(i.ToString());
            }

            AssertWriteString(sb.ToString());
        }

        [Test]
        public void WriteUnicodeLargeString()
        {
            StringBuilder sb = new StringBuilder();
            for (int i = 0; i < 2048; i++)
            {
                sb.AppendLine(i.ToString().Replace("0", "\x1369"));
            }

            AssertWriteString(sb.ToString());
        }

        [Test]
        public void WriteFloat()
        {
            AssertWriteFloat(new byte[] { 0, 0, 0, 0 }, 0f);
            AssertWriteFloat(new byte[] { 205, 204, 140, 63 }, 1.1f);
            AssertWriteFloat(new byte[] { 255, 255, 127, 127 }, float.MaxValue);
            AssertWriteFloat(new byte[] { 255, 255, 127, 255 }, float.MinValue);
        }

        [Test]
        public void WriteDouble()
        {
            AssertWriteDouble(new byte[] { 0, 0, 0, 0, 0, 0, 0, 0 }, 0f);
            AssertWriteDouble(new byte[] { 0, 0, 0, 160, 153, 153, 241, 63 }, 1.1f);
            AssertWriteDouble(new byte[] { 255, 255, 255, 255, 255, 255, 239, 127 }, double.MaxValue);
            AssertWriteDouble(new byte[] { 255, 255, 255, 255, 255, 255, 239, 255 }, double.MinValue);
        }

        [Test]
        public void WriteTag()
        {
            AssertWriteRawTag(new byte[] { 128, 128, 128, 128, 1 }, 268435456);
            AssertWriteRawTag(new byte[] { 128, 128, 128, 1 }, 2097152);
            AssertWriteRawTag(new byte[] { 128, 128, 1 }, 16384);
            AssertWriteRawTag(new byte[] { 128, 1 }, 128);
            AssertWriteRawTag(new byte[] { 127 }, 127);
        }
    }
}
