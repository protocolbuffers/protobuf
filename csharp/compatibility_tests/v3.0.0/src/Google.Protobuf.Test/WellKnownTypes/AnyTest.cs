#region Copyright notice and license
// Protocol Buffers - Google's data interchange format
// Copyright 2015 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd
#endregion

using Google.Protobuf.TestProtos;
using NUnit.Framework;

namespace Google.Protobuf.WellKnownTypes
{
    public class AnyTest
    {
        [Test]
        public void Pack()
        {
            var message = SampleMessages.CreateFullTestAllTypes();
            var any = Any.Pack(message);
            Assert.AreEqual("type.googleapis.com/protobuf_unittest.TestAllTypes", any.TypeUrl);
            Assert.AreEqual(message.CalculateSize(), any.Value.Length);
        }

        [Test]
        public void Pack_WithCustomPrefix()
        {
            var message = SampleMessages.CreateFullTestAllTypes();
            var any = Any.Pack(message, "foo.bar/baz");
            Assert.AreEqual("foo.bar/baz/protobuf_unittest.TestAllTypes", any.TypeUrl);
            Assert.AreEqual(message.CalculateSize(), any.Value.Length);
        }

        [Test]
        public void Pack_WithCustomPrefixTrailingSlash()
        {
            var message = SampleMessages.CreateFullTestAllTypes();
            var any = Any.Pack(message, "foo.bar/baz/");
            Assert.AreEqual("foo.bar/baz/protobuf_unittest.TestAllTypes", any.TypeUrl);
            Assert.AreEqual(message.CalculateSize(), any.Value.Length);
        }

        [Test]
        public void Unpack_WrongType()
        {
            var message = SampleMessages.CreateFullTestAllTypes();
            var any = Any.Pack(message);
            Assert.Throws<InvalidProtocolBufferException>(() => any.Unpack<TestOneof>());
        }

        [Test]
        public void Unpack_Success()
        {
            var message = SampleMessages.CreateFullTestAllTypes();
            var any = Any.Pack(message);
            var unpacked = any.Unpack<TestAllTypes>();
            Assert.AreEqual(message, unpacked);
        }

        [Test]
        public void Unpack_CustomPrefix_Success()
        {
            var message = SampleMessages.CreateFullTestAllTypes();
            var any = Any.Pack(message, "foo.bar/baz");
            var unpacked = any.Unpack<TestAllTypes>();
            Assert.AreEqual(message, unpacked);
        }

        [Test]
        public void ToString_WithValues()
        {
            var message = SampleMessages.CreateFullTestAllTypes();
            var any = Any.Pack(message);
            var text = any.ToString();
            Assert.That(text, Does.Contain("\"@value\": \"" + message.ToByteString().ToBase64() + "\""));
        }

        [Test]
        public void ToString_Empty()
        {
            var any = new Any();
            Assert.AreEqual("{ \"@type\": \"\", \"@value\": \"\" }", any.ToString());
        }

        [Test]
        public void ToString_MessageContainingAny()
        {
            var message = new TestWellKnownTypes { AnyField = new Any() };
            Assert.AreEqual("{ \"anyField\": { \"@type\": \"\", \"@value\": \"\" } }", message.ToString());
        }
    }
}
