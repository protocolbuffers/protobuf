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
using System.IO;
using Google.ProtocolBuffers.TestProtos;
using NUnit.Framework;
using System.Diagnostics;

namespace Google.ProtocolBuffers {
  [TestFixture]
  public class CodedInputStreamTest {

    /// <summary>
    /// Helper to construct a byte array from a bunch of bytes.  The inputs are
    /// actually ints so that I can use hex notation and not get stupid errors
    /// about precision.
    /// </summary>
    private static byte[] Bytes(params int[] bytesAsInts) {
      byte[] bytes = new byte[bytesAsInts.Length];
      for (int i = 0; i < bytesAsInts.Length; i++) {
        bytes[i] = (byte)bytesAsInts[i];
      }
      return bytes;
    }

    /// <summary>
    /// Parses the given bytes using ReadRawVarint32() and ReadRawVarint64() and
    /// </summary>
    private static void AssertReadVarint(byte[] data, ulong value) {
      CodedInputStream input = CodedInputStream.CreateInstance(data);
      Assert.AreEqual((uint)value, input.ReadRawVarint32());

      input = CodedInputStream.CreateInstance(data);
      Assert.AreEqual(value, input.ReadRawVarint64());
      Assert.IsTrue(input.IsAtEnd);

      // Try different block sizes.
      for (int bufferSize = 1; bufferSize <= 16; bufferSize *= 2) {
        input = CodedInputStream.CreateInstance(new SmallBlockInputStream(data, bufferSize));
        Assert.AreEqual((uint)value, input.ReadRawVarint32());

        input = CodedInputStream.CreateInstance(new SmallBlockInputStream(data, bufferSize));
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

    /// <summary>
    /// Parses the given bytes using ReadRawVarint32() and ReadRawVarint64() and
    /// expects them to fail with an InvalidProtocolBufferException whose
    /// description matches the given one.
    /// </summary>
    private static void AssertReadVarintFailure(InvalidProtocolBufferException expected, byte[] data) {
      CodedInputStream input = CodedInputStream.CreateInstance(data);
      try {
        input.ReadRawVarint32();
        Assert.Fail("Should have thrown an exception.");
      } catch (InvalidProtocolBufferException e) {
        Assert.AreEqual(expected.Message, e.Message);
      }

      input = CodedInputStream.CreateInstance(data);
      try {
        input.ReadRawVarint64();
        Assert.Fail("Should have thrown an exception.");
      } catch (InvalidProtocolBufferException e) {
        Assert.AreEqual(expected.Message, e.Message);
      }

      // Make sure we get the same error when reading directly from a Stream.
      try {
        CodedInputStream.ReadRawVarint32(new MemoryStream(data));
        Assert.Fail("Should have thrown an exception.");
      } catch (InvalidProtocolBufferException e) {
        Assert.AreEqual(expected.Message, e.Message);
      }
    }

    [Test]
    public void ReadVarint() {
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
    private static void AssertReadLittleEndian32(byte[] data, uint value) {
      CodedInputStream input = CodedInputStream.CreateInstance(data);
      Assert.AreEqual(value, input.ReadRawLittleEndian32());
      Assert.IsTrue(input.IsAtEnd);

      // Try different block sizes.
      for (int blockSize = 1; blockSize <= 16; blockSize *= 2) {
        input = CodedInputStream.CreateInstance(
          new SmallBlockInputStream(data, blockSize));
        Assert.AreEqual(value, input.ReadRawLittleEndian32());
        Assert.IsTrue(input.IsAtEnd);
      }
    }

    /// <summary>
    /// Parses the given bytes using ReadRawLittleEndian64() and checks
    /// that the result matches the given value.
    /// </summary>
    private static void AssertReadLittleEndian64(byte[] data, ulong value) {
      CodedInputStream input = CodedInputStream.CreateInstance(data);
      Assert.AreEqual(value, input.ReadRawLittleEndian64());
      Assert.IsTrue(input.IsAtEnd);

      // Try different block sizes.
      for (int blockSize = 1; blockSize <= 16; blockSize *= 2) {
        input = CodedInputStream.CreateInstance(
          new SmallBlockInputStream(data, blockSize));
        Assert.AreEqual(value, input.ReadRawLittleEndian64());
        Assert.IsTrue(input.IsAtEnd);
      }
    }

    [Test]
    public void ReadLittleEndian() {
      AssertReadLittleEndian32(Bytes(0x78, 0x56, 0x34, 0x12), 0x12345678);
      AssertReadLittleEndian32(Bytes(0xf0, 0xde, 0xbc, 0x9a), 0x9abcdef0);

      AssertReadLittleEndian64(Bytes(0xf0, 0xde, 0xbc, 0x9a, 0x78, 0x56, 0x34, 0x12),
        0x123456789abcdef0L);
      AssertReadLittleEndian64(
        Bytes(0x78, 0x56, 0x34, 0x12, 0xf0, 0xde, 0xbc, 0x9a), 0x9abcdef012345678UL);
    }

    [Test]
    public void DecodeZigZag32() {
      Assert.AreEqual(0, CodedInputStream.DecodeZigZag32(0));
      Assert.AreEqual(-1, CodedInputStream.DecodeZigZag32(1));
      Assert.AreEqual(1, CodedInputStream.DecodeZigZag32(2));
      Assert.AreEqual(-2, CodedInputStream.DecodeZigZag32(3));
      Assert.AreEqual(0x3FFFFFFF, CodedInputStream.DecodeZigZag32(0x7FFFFFFE));
      Assert.AreEqual(unchecked((int)0xC0000000), CodedInputStream.DecodeZigZag32(0x7FFFFFFF));
      Assert.AreEqual(0x7FFFFFFF, CodedInputStream.DecodeZigZag32(0xFFFFFFFE));
      Assert.AreEqual(unchecked((int)0x80000000), CodedInputStream.DecodeZigZag32(0xFFFFFFFF));
    }

    [Test]
    public void DecodeZigZag64() {
      Assert.AreEqual(0, CodedInputStream.DecodeZigZag64(0));
      Assert.AreEqual(-1, CodedInputStream.DecodeZigZag64(1));
      Assert.AreEqual(1, CodedInputStream.DecodeZigZag64(2));
      Assert.AreEqual(-2, CodedInputStream.DecodeZigZag64(3));
      Assert.AreEqual(0x000000003FFFFFFFL, CodedInputStream.DecodeZigZag64(0x000000007FFFFFFEL));
      Assert.AreEqual(unchecked((long)0xFFFFFFFFC0000000L), CodedInputStream.DecodeZigZag64(0x000000007FFFFFFFL));
      Assert.AreEqual(0x000000007FFFFFFFL, CodedInputStream.DecodeZigZag64(0x00000000FFFFFFFEL));
      Assert.AreEqual(unchecked((long)0xFFFFFFFF80000000L), CodedInputStream.DecodeZigZag64(0x00000000FFFFFFFFL));
      Assert.AreEqual(0x7FFFFFFFFFFFFFFFL, CodedInputStream.DecodeZigZag64(0xFFFFFFFFFFFFFFFEL));
      Assert.AreEqual(unchecked((long)0x8000000000000000L), CodedInputStream.DecodeZigZag64(0xFFFFFFFFFFFFFFFFL));
    }

    [Test]
    public void ReadWholeMessage() {
      TestAllTypes message = TestUtil.GetAllSet();

      byte[] rawBytes = message.ToByteArray();
      Assert.AreEqual(rawBytes.Length, message.SerializedSize);
      TestAllTypes message2 = TestAllTypes.ParseFrom(rawBytes);
      TestUtil.AssertAllFieldsSet(message2);

      // Try different block sizes.
      for (int blockSize = 1; blockSize < 256; blockSize *= 2) {
        message2 = TestAllTypes.ParseFrom(new SmallBlockInputStream(rawBytes, blockSize));
        TestUtil.AssertAllFieldsSet(message2);
      }
    }

    [Test]
    public void SkipWholeMessage() {
      TestAllTypes message = TestUtil.GetAllSet();
      byte[] rawBytes = message.ToByteArray();

      // Create two parallel inputs.  Parse one as unknown fields while using
      // skipField() to skip each field on the other.  Expect the same tags.
      CodedInputStream input1 = CodedInputStream.CreateInstance(rawBytes);
      CodedInputStream input2 = CodedInputStream.CreateInstance(rawBytes);
      UnknownFieldSet.Builder unknownFields = UnknownFieldSet.CreateBuilder();

      while (true) {
        uint tag = input1.ReadTag();
        Assert.AreEqual(tag, input2.ReadTag());
        if (tag == 0) {
          break;
        }
        unknownFields.MergeFieldFrom(tag, input1);
        input2.SkipField(tag);
      }
    }

    /// <summary>
    /// Test that a bug in SkipRawBytes has been fixed: if the skip
    /// skips exactly up to a limit, this should bnot break things
    /// </summary>
    [Test]
    public void SkipRawBytesBug() {
      byte[] rawBytes = new byte[] { 1, 2 };
      CodedInputStream input = CodedInputStream.CreateInstance(rawBytes);

      int limit = input.PushLimit(1);
      input.SkipRawBytes(1);
      input.PopLimit(limit);
      Assert.AreEqual(2, input.ReadRawByte());
    }

    public void ReadHugeBlob() {
      // Allocate and initialize a 1MB blob.
      byte[] blob = new byte[1 << 20];
      for (int i = 0; i < blob.Length; i++) {
        blob[i] = (byte)i;
      }

      // Make a message containing it.
      TestAllTypes.Builder builder = TestAllTypes.CreateBuilder();
      TestUtil.SetAllFields(builder);
      builder.SetOptionalBytes(ByteString.CopyFrom(blob));
      TestAllTypes message = builder.Build();

      // Serialize and parse it.  Make sure to parse from an InputStream, not
      // directly from a ByteString, so that CodedInputStream uses buffered
      // reading.
      TestAllTypes message2 = TestAllTypes.ParseFrom(message.ToByteString().CreateCodedInput());

      Assert.AreEqual(message.OptionalBytes, message2.OptionalBytes);

      // Make sure all the other fields were parsed correctly.
      TestAllTypes message3 = TestAllTypes.CreateBuilder(message2)
        .SetOptionalBytes(TestUtil.GetAllSet().OptionalBytes)
        .Build();
      TestUtil.AssertAllFieldsSet(message3);
    }

    [Test]
    public void ReadMaliciouslyLargeBlob() {
      MemoryStream ms = new MemoryStream();
      CodedOutputStream output = CodedOutputStream.CreateInstance(ms);

      uint tag = WireFormat.MakeTag(1, WireFormat.WireType.LengthDelimited);
      output.WriteRawVarint32(tag);
      output.WriteRawVarint32(0x7FFFFFFF);
      output.WriteRawBytes(new byte[32]);  // Pad with a few random bytes.
      output.Flush();
      ms.Position = 0;

      CodedInputStream input = CodedInputStream.CreateInstance(ms);
      Assert.AreEqual(tag, input.ReadTag());

      try {
        input.ReadBytes();
        Assert.Fail("Should have thrown an exception!");
      } catch (InvalidProtocolBufferException) {
        // success.
      }
    }
    
    private static TestRecursiveMessage MakeRecursiveMessage(int depth) {
      if (depth == 0) {
        return TestRecursiveMessage.CreateBuilder().SetI(5).Build();
      } else {
        return TestRecursiveMessage.CreateBuilder()
          .SetA(MakeRecursiveMessage(depth - 1)).Build();
      }
    }

    private static void AssertMessageDepth(TestRecursiveMessage message, int depth) {
      if (depth == 0) {
        Assert.IsFalse(message.HasA);
        Assert.AreEqual(5, message.I);
      } else {
        Assert.IsTrue(message.HasA);
        AssertMessageDepth(message.A, depth - 1);
      }
    }

    [Test]
    public void MaliciousRecursion() {
      ByteString data64 = MakeRecursiveMessage(64).ToByteString();
      ByteString data65 = MakeRecursiveMessage(65).ToByteString();

      AssertMessageDepth(TestRecursiveMessage.ParseFrom(data64), 64);

      try {
        TestRecursiveMessage.ParseFrom(data65);
        Assert.Fail("Should have thrown an exception!");
      } catch (InvalidProtocolBufferException) {
        // success.
      }

      CodedInputStream input = data64.CreateCodedInput();
      input.SetRecursionLimit(8);
      try {
        TestRecursiveMessage.ParseFrom(input);
        Assert.Fail("Should have thrown an exception!");
      } catch (InvalidProtocolBufferException) {
        // success.
      }
    }

    [Test]
    public void SizeLimit() {
      // Have to use a Stream rather than ByteString.CreateCodedInput as SizeLimit doesn't
      // apply to the latter case.
      MemoryStream ms = new MemoryStream(TestUtil.GetAllSet().ToByteString().ToByteArray());
      CodedInputStream input = CodedInputStream.CreateInstance(ms);
      input.SetSizeLimit(16);

      try {
        TestAllTypes.ParseFrom(input);
        Assert.Fail("Should have thrown an exception!");
      } catch (InvalidProtocolBufferException) {
        // success.
      }
    }

    [Test]
    public void ResetSizeCounter() {
      CodedInputStream input = CodedInputStream.CreateInstance(
          new SmallBlockInputStream(new byte[256], 8));
      input.SetSizeLimit(16);
      input.ReadRawBytes(16);

      try {
        input.ReadRawByte();
        Assert.Fail("Should have thrown an exception!");
      } catch (InvalidProtocolBufferException) {
        // Success.
      }

      input.ResetSizeCounter();
      input.ReadRawByte();  // No exception thrown.

      try {
        input.ReadRawBytes(16);  // Hits limit again.
        Assert.Fail("Should have thrown an exception!");
      } catch (InvalidProtocolBufferException) {
        // Success.
      }
    }

    /// <summary>
    /// Tests that if we read an string that contains invalid UTF-8, no exception
    /// is thrown.  Instead, the invalid bytes are replaced with the Unicode
    /// "replacement character" U+FFFD.
    /// </summary>
    [Test]
    public void ReadInvalidUtf8()  {
      MemoryStream ms = new MemoryStream();
      CodedOutputStream output = CodedOutputStream.CreateInstance(ms);

      uint tag = WireFormat.MakeTag(1, WireFormat.WireType.LengthDelimited);
      output.WriteRawVarint32(tag);
      output.WriteRawVarint32(1);
      output.WriteRawBytes(new byte[] { 0x80 });
      output.Flush();
      ms.Position = 0;

      CodedInputStream input = CodedInputStream.CreateInstance(ms);
      Assert.AreEqual(tag, input.ReadTag());
      string text = input.ReadString();
      Assert.AreEqual('\ufffd', text[0]);
    }

    /// <summary>
    /// A stream which limits the number of bytes it reads at a time.
    /// We use this to make sure that CodedInputStream doesn't screw up when
    /// reading in small blocks.
    /// </summary>
    private sealed class SmallBlockInputStream : MemoryStream {
      private readonly int blockSize;

      public SmallBlockInputStream(byte[] data, int blockSize)
        : base(data) {
        this.blockSize = blockSize;
      }

      public override int Read(byte[] buffer, int offset, int count) {
        return base.Read(buffer, offset, Math.Min(count, blockSize));
      }
    }
  }
}
