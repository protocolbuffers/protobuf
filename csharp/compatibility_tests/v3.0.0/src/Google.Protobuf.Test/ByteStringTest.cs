#region Copyright notice and license
// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd
#endregion

using System;
using System.Text;
using NUnit.Framework;

namespace Google.Protobuf
{
    public class ByteStringTest
    {
        [Test]
        public void Equality()
        {
            ByteString b1 = ByteString.CopyFrom(1, 2, 3);
            ByteString b2 = ByteString.CopyFrom(1, 2, 3);
            ByteString b3 = ByteString.CopyFrom(1, 2, 4);
            ByteString b4 = ByteString.CopyFrom(1, 2, 3, 4);
            EqualityTester.AssertEquality(b1, b1);
            EqualityTester.AssertEquality(b1, b2);
            EqualityTester.AssertInequality(b1, b3);
            EqualityTester.AssertInequality(b1, b4);
            EqualityTester.AssertInequality(b1, null);
#pragma warning disable 1718 // Deliberately calling ==(b1, b1) and !=(b1, b1)
            Assert.IsTrue(b1 == b1);
            Assert.IsTrue(b1 == b2);
            Assert.IsFalse(b1 == b3);
            Assert.IsFalse(b1 == b4);
            Assert.IsFalse(b1 == null);
            Assert.IsTrue((ByteString) null == null);
            Assert.IsFalse(b1 != b1);
            Assert.IsFalse(b1 != b2);
#pragma warning disable 1718
            Assert.IsTrue(b1 != b3);
            Assert.IsTrue(b1 != b4);
            Assert.IsTrue(b1 != null);
            Assert.IsFalse((ByteString) null != null);
        }

        [Test]
        public void EmptyByteStringHasZeroSize()
        {
            Assert.AreEqual(0, ByteString.Empty.Length);
        }

        [Test]
        public void CopyFromStringWithExplicitEncoding()
        {
            ByteString bs = ByteString.CopyFrom("AB", Encoding.Unicode);
            Assert.AreEqual(4, bs.Length);
            Assert.AreEqual(65, bs[0]);
            Assert.AreEqual(0, bs[1]);
            Assert.AreEqual(66, bs[2]);
            Assert.AreEqual(0, bs[3]);
        }

        [Test]
        public void IsEmptyWhenEmpty()
        {
            Assert.IsTrue(ByteString.CopyFromUtf8("").IsEmpty);
        }

        [Test]
        public void IsEmptyWhenNotEmpty()
        {
            Assert.IsFalse(ByteString.CopyFromUtf8("X").IsEmpty);
        }

        [Test]
        public void CopyFromByteArrayCopiesContents()
        {
            byte[] data = new byte[1];
            data[0] = 10;
            ByteString bs = ByteString.CopyFrom(data);
            Assert.AreEqual(10, bs[0]);
            data[0] = 5;
            Assert.AreEqual(10, bs[0]);
        }

        [Test]
        public void ToByteArrayCopiesContents()
        {
            ByteString bs = ByteString.CopyFromUtf8("Hello");
            byte[] data = bs.ToByteArray();
            Assert.AreEqual((byte)'H', data[0]);
            Assert.AreEqual((byte)'H', bs[0]);
            data[0] = 0;
            Assert.AreEqual(0, data[0]);
            Assert.AreEqual((byte)'H', bs[0]);
        }

        [Test]
        public void CopyFromUtf8UsesUtf8()
        {
            ByteString bs = ByteString.CopyFromUtf8("\u20ac");
            Assert.AreEqual(3, bs.Length);
            Assert.AreEqual(0xe2, bs[0]);
            Assert.AreEqual(0x82, bs[1]);
            Assert.AreEqual(0xac, bs[2]);
        }

        [Test]
        public void CopyFromPortion()
        {
            byte[] data = new byte[] {0, 1, 2, 3, 4, 5, 6};
            ByteString bs = ByteString.CopyFrom(data, 2, 3);
            Assert.AreEqual(3, bs.Length);
            Assert.AreEqual(2, bs[0]);
            Assert.AreEqual(3, bs[1]);
        }

        [Test]
        public void ToStringUtf8()
        {
            ByteString bs = ByteString.CopyFromUtf8("\u20ac");
            Assert.AreEqual("\u20ac", bs.ToStringUtf8());
        }

        [Test]
        public void ToStringWithExplicitEncoding()
        {
            ByteString bs = ByteString.CopyFrom("\u20ac", Encoding.Unicode);
            Assert.AreEqual("\u20ac", bs.ToString(Encoding.Unicode));
        }

        [Test]
        public void FromBase64_WithText()
        {
            byte[] data = new byte[] {0, 1, 2, 3, 4, 5, 6};
            string base64 = Convert.ToBase64String(data);
            ByteString bs = ByteString.FromBase64(base64);
            Assert.AreEqual(data, bs.ToByteArray());
        }

        [Test]
        public void FromBase64_Empty()
        {
            // Optimization which also fixes issue 61.
            Assert.AreSame(ByteString.Empty, ByteString.FromBase64(""));
        }
    }
}