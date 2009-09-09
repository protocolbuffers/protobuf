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

using System.Text;
using NUnit.Framework;

namespace Google.ProtocolBuffers {
  [TestFixture]
  public class ByteStringTest {
    [Test]
    public void EmptyByteStringHasZeroSize() {
      Assert.AreEqual(0, ByteString.Empty.Length);
    }

    [Test]
    public void CopyFromStringWithExplicitEncoding() {
      ByteString bs = ByteString.CopyFrom("AB", Encoding.Unicode);
      Assert.AreEqual(4, bs.Length);
      Assert.AreEqual(65, bs[0]);
      Assert.AreEqual(0, bs[1]);
      Assert.AreEqual(66, bs[2]);
      Assert.AreEqual(0, bs[3]);
    }

    [Test]
    public void IsEmptyWhenEmpty() {
      Assert.IsTrue(ByteString.CopyFromUtf8("").IsEmpty);
    }

    [Test]
    public void IsEmptyWhenNotEmpty() {
      Assert.IsFalse(ByteString.CopyFromUtf8("X").IsEmpty);
    }

    [Test]
    public void CopyFromByteArrayCopiesContents() {
      byte[] data = new byte[1];
      data[0] = 10;
      ByteString bs = ByteString.CopyFrom(data);
      Assert.AreEqual(10, bs[0]);
      data[0] = 5;
      Assert.AreEqual(10, bs[0]);
    }

    [Test]
    public void ToByteArrayCopiesContents() {
      ByteString bs = ByteString.CopyFromUtf8("Hello");
      byte[] data = bs.ToByteArray();
      Assert.AreEqual('H', data[0]);
      Assert.AreEqual('H', bs[0]);
      data[0] = 0;
      Assert.AreEqual(0, data[0]);
      Assert.AreEqual('H', bs[0]);
    }

    [Test]
    public void CopyFromUtf8UsesUtf8() {
      ByteString bs = ByteString.CopyFromUtf8("\u20ac");
      Assert.AreEqual(3, bs.Length);
      Assert.AreEqual(0xe2, bs[0]);
      Assert.AreEqual(0x82, bs[1]);
      Assert.AreEqual(0xac, bs[2]);
    }

    [Test]
    public void CopyFromPortion() {
      byte[] data = new byte[]{0, 1, 2, 3, 4, 5, 6};
      ByteString bs = ByteString.CopyFrom(data, 2, 3);
      Assert.AreEqual(3, bs.Length);
      Assert.AreEqual(2, bs[0]);
      Assert.AreEqual(3, bs[1]);
    }

    [Test]
    public void ToStringUtf8() {
      ByteString bs = ByteString.CopyFromUtf8("\u20ac");
      Assert.AreEqual("\u20ac", bs.ToStringUtf8());
    }

    [Test]
    public void ToStringWithExplicitEncoding() {
      ByteString bs = ByteString.CopyFrom("\u20ac", Encoding.Unicode);
      Assert.AreEqual("\u20ac", bs.ToString(Encoding.Unicode));
    }
  }
}
