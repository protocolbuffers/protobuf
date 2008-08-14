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
