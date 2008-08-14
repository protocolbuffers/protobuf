// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.
// http://code.google.com/p/protobuf/
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
using System.IO;
using Google.ProtocolBuffers.TestProtos;
using NUnit.Framework;

namespace Google.ProtocolBuffers {
  [TestFixture]
  public class CodedOutputStreamTest {

    private static void AssertEqualBytes(byte[] a, byte[] b) {
      Assert.AreEqual(ByteString.CopyFrom(a), ByteString.CopyFrom(b));
    }

    /// <summary>
    /// Writes the given value using WriteRawVarint32() and WriteRawVarint64() and
    /// checks that the result matches the given bytes
    /// </summary>
    private static void AssertWriteVarint(byte[] data, ulong value) {
      // Only do 32-bit write if the value fits in 32 bits.
      if ((value >> 32) == 0) {
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
      for (int bufferSize = 1; bufferSize <= 16; bufferSize *= 2) {
        // Only do 32-bit write if the value fits in 32 bits.
        if ((value >> 32) == 0) {
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
    public void WriteVarint() {
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
    private static void AssertWriteLittleEndian32(byte[] data, uint value) {
      MemoryStream rawOutput = new MemoryStream();
      CodedOutputStream output = CodedOutputStream.CreateInstance(rawOutput);
      output.WriteRawLittleEndian32(value);
      output.Flush();
      Assert.AreEqual(data, rawOutput.ToArray());

      // Try different buffer sizes.
      for (int bufferSize = 1; bufferSize <= 16; bufferSize *= 2) {
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
    private static void AssertWriteLittleEndian64(byte[] data, ulong value) {
      MemoryStream rawOutput = new MemoryStream();
      CodedOutputStream output = CodedOutputStream.CreateInstance(rawOutput);
      output.WriteRawLittleEndian64(value);
      output.Flush();
      Assert.AreEqual(data, rawOutput.ToArray());

      // Try different block sizes.
      for (int blockSize = 1; blockSize <= 16; blockSize *= 2) {
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
    public void WriteLittleEndian() {
      AssertWriteLittleEndian32(new byte[] {0x78, 0x56, 0x34, 0x12}, 0x12345678);
      AssertWriteLittleEndian32(new byte[] {0xf0, 0xde, 0xbc, 0x9a}, 0x9abcdef0);

      AssertWriteLittleEndian64(
        new byte[]{0xf0, 0xde, 0xbc, 0x9a, 0x78, 0x56, 0x34, 0x12},
        0x123456789abcdef0L);
      AssertWriteLittleEndian64(
        new byte[]{0x78, 0x56, 0x34, 0x12, 0xf0, 0xde, 0xbc, 0x9a},
        0x9abcdef012345678UL);
    }

    [Test]
    public void WriteWholeMessage() {
      TestAllTypes message = TestUtil.GetAllSet();

      byte[] rawBytes = message.ToByteArray();
      AssertEqualBytes(TestUtil.GoldenMessage.ToByteArray(), rawBytes);

      // Try different block sizes.
      for (int blockSize = 1; blockSize < 256; blockSize *= 2) {
        MemoryStream rawOutput = new MemoryStream();
        CodedOutputStream output =
          CodedOutputStream.CreateInstance(rawOutput, blockSize);
        message.WriteTo(output);
        output.Flush();
        AssertEqualBytes(rawBytes, rawOutput.ToArray());
      }
    }


    [Test]
    public void EncodeZigZag32() {
      Assert.AreEqual(0, CodedOutputStream.EncodeZigZag32( 0));
      Assert.AreEqual(1, CodedOutputStream.EncodeZigZag32(-1));
      Assert.AreEqual(2, CodedOutputStream.EncodeZigZag32( 1));
      Assert.AreEqual(3, CodedOutputStream.EncodeZigZag32(-2));
      Assert.AreEqual(0x7FFFFFFE, CodedOutputStream.EncodeZigZag32(0x3FFFFFFF));
      Assert.AreEqual(0x7FFFFFFF, CodedOutputStream.EncodeZigZag32(unchecked((int)0xC0000000)));
      Assert.AreEqual(0xFFFFFFFE, CodedOutputStream.EncodeZigZag32(0x7FFFFFFF));
      Assert.AreEqual(0xFFFFFFFF, CodedOutputStream.EncodeZigZag32(unchecked((int)0x80000000)));
    }

    [Test]
    public void EncodeZigZag64() {
      Assert.AreEqual(0, CodedOutputStream.EncodeZigZag64( 0));
      Assert.AreEqual(1, CodedOutputStream.EncodeZigZag64(-1));
      Assert.AreEqual(2, CodedOutputStream.EncodeZigZag64( 1));
      Assert.AreEqual(3, CodedOutputStream.EncodeZigZag64(-2));
      Assert.AreEqual(0x000000007FFFFFFEL,
                   CodedOutputStream.EncodeZigZag64(unchecked((long)0x000000003FFFFFFFUL)));
      Assert.AreEqual(0x000000007FFFFFFFL,
                   CodedOutputStream.EncodeZigZag64(unchecked((long)0xFFFFFFFFC0000000UL)));
      Assert.AreEqual(0x00000000FFFFFFFEL,
                   CodedOutputStream.EncodeZigZag64(unchecked((long)0x000000007FFFFFFFUL)));
      Assert.AreEqual(0x00000000FFFFFFFFL,
                   CodedOutputStream.EncodeZigZag64(unchecked((long)0xFFFFFFFF80000000UL)));
      Assert.AreEqual(0xFFFFFFFFFFFFFFFEL,
                   CodedOutputStream.EncodeZigZag64(unchecked((long)0x7FFFFFFFFFFFFFFFUL)));
      Assert.AreEqual(0xFFFFFFFFFFFFFFFFL,
                   CodedOutputStream.EncodeZigZag64(unchecked((long)0x8000000000000000UL)));
    }

    [Test]
    public void RoundTripZigZag32() {
      // Some easier-to-verify round-trip tests.  The inputs (other than 0, 1, -1)
      // were chosen semi-randomly via keyboard bashing.
      Assert.AreEqual(0, CodedInputStream.DecodeZigZag32(CodedOutputStream.EncodeZigZag32(0)));
      Assert.AreEqual(1, CodedInputStream.DecodeZigZag32(CodedOutputStream.EncodeZigZag32(1)));
      Assert.AreEqual(-1, CodedInputStream.DecodeZigZag32(CodedOutputStream.EncodeZigZag32(-1)));
      Assert.AreEqual(14927, CodedInputStream.DecodeZigZag32(CodedOutputStream.EncodeZigZag32(14927)));
      Assert.AreEqual(-3612, CodedInputStream.DecodeZigZag32(CodedOutputStream.EncodeZigZag32(-3612)));
    }
     
    [Test]
    public void RoundTripZigZag64() {
      Assert.AreEqual(0, CodedInputStream.DecodeZigZag64(CodedOutputStream.EncodeZigZag64(0)));
      Assert.AreEqual(1, CodedInputStream.DecodeZigZag64(CodedOutputStream.EncodeZigZag64(1)));
      Assert.AreEqual(-1, CodedInputStream.DecodeZigZag64(CodedOutputStream.EncodeZigZag64(-1)));
      Assert.AreEqual(14927, CodedInputStream.DecodeZigZag64(CodedOutputStream.EncodeZigZag64(14927)));
      Assert.AreEqual(-3612, CodedInputStream.DecodeZigZag64(CodedOutputStream.EncodeZigZag64(-3612)));

      Assert.AreEqual(856912304801416L, CodedInputStream.DecodeZigZag64(CodedOutputStream.EncodeZigZag64(856912304801416L)));
      Assert.AreEqual(-75123905439571256L, CodedInputStream.DecodeZigZag64(CodedOutputStream.EncodeZigZag64(-75123905439571256L)));
    }
  }
}
