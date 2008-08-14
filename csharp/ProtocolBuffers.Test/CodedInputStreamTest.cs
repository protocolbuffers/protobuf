using System;
using System.Collections.Generic;
using System.Text;
using NUnit.Framework;

namespace Google.ProtocolBuffers {
  [TestFixture]
  public class CodedInputStreamTest {
    
    [Test]
    public void DecodeZigZag32() {
      Assert.AreEqual( 0, CodedInputStream.DecodeZigZag32(0));
      Assert.AreEqual(-1, CodedInputStream.DecodeZigZag32(1));
      Assert.AreEqual( 1, CodedInputStream.DecodeZigZag32(2));
      Assert.AreEqual(-2, CodedInputStream.DecodeZigZag32(3));
      Assert.AreEqual(0x3FFFFFFF, CodedInputStream.DecodeZigZag32(0x7FFFFFFE));
      Assert.AreEqual(unchecked((int)0xC0000000), CodedInputStream.DecodeZigZag32(0x7FFFFFFF));
      Assert.AreEqual(0x7FFFFFFF, CodedInputStream.DecodeZigZag32(0xFFFFFFFE));
      Assert.AreEqual(unchecked((int)0x80000000), CodedInputStream.DecodeZigZag32(0xFFFFFFFF));
    }

    [Test]
    public void DecodeZigZag64() {
      Assert.AreEqual( 0, CodedInputStream.DecodeZigZag64(0));
      Assert.AreEqual(-1, CodedInputStream.DecodeZigZag64(1));
      Assert.AreEqual( 1, CodedInputStream.DecodeZigZag64(2));
      Assert.AreEqual(-2, CodedInputStream.DecodeZigZag64(3));
      Assert.AreEqual(0x000000003FFFFFFFL, CodedInputStream.DecodeZigZag64(0x000000007FFFFFFEL));
      Assert.AreEqual(unchecked((long)0xFFFFFFFFC0000000L), CodedInputStream.DecodeZigZag64(0x000000007FFFFFFFL));
      Assert.AreEqual(0x000000007FFFFFFFL, CodedInputStream.DecodeZigZag64(0x00000000FFFFFFFEL));
      Assert.AreEqual(unchecked((long)0xFFFFFFFF80000000L), CodedInputStream.DecodeZigZag64(0x00000000FFFFFFFFL));
      Assert.AreEqual(0x7FFFFFFFFFFFFFFFL, CodedInputStream.DecodeZigZag64(0xFFFFFFFFFFFFFFFEL));
      Assert.AreEqual(unchecked((long)0x8000000000000000L),CodedInputStream.DecodeZigZag64(0xFFFFFFFFFFFFFFFFL));
    }
  }
}
