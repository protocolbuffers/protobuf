#region Copyright notice and license
// Protocol Buffers - Google's data interchange format
// Copyright 2015 Google Inc.  All rights reserved.
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
            Assert.AreEqual("type.googleapis.com/protobuf_unittest3.TestAllTypes", any.TypeUrl);
            Assert.AreEqual(message.CalculateSize(), any.Value.Length);
        }

        [Test]
        public void Pack_WithCustomPrefix()
        {
            var message = SampleMessages.CreateFullTestAllTypes();
            var any = Any.Pack(message, "foo.bar/baz");
            Assert.AreEqual("foo.bar/baz/protobuf_unittest3.TestAllTypes", any.TypeUrl);
            Assert.AreEqual(message.CalculateSize(), any.Value.Length);
        }

        [Test]
        public void Pack_WithCustomPrefixTrailingSlash()
        {
            var message = SampleMessages.CreateFullTestAllTypes();
            var any = Any.Pack(message, "foo.bar/baz/");
            Assert.AreEqual("foo.bar/baz/protobuf_unittest3.TestAllTypes", any.TypeUrl);
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
        public void TryUnpack_WrongType()
        {
            var message = SampleMessages.CreateFullTestAllTypes();
            var any = Any.Pack(message);
            Assert.False(any.TryUnpack(out TestOneof unpacked));
            Assert.Null(unpacked);
        }

        [Test]
        public void TryUnpack_RightType()
        {
            var message = SampleMessages.CreateFullTestAllTypes();
            var any = Any.Pack(message);
            Assert.IsTrue(any.TryUnpack(out TestAllTypes unpacked));
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
        [TestCase("proto://foo.bar", "foo.bar")]
        [TestCase("/foo/bar/baz", "baz")]
        [TestCase("foobar", "")]
        public void GetTypeName(string typeUrl, string expectedTypeName)
        {
            var any = new Any { TypeUrl = typeUrl };
            Assert.AreEqual(expectedTypeName, Any.GetTypeName(typeUrl));
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
