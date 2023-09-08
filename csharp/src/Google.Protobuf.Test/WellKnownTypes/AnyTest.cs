#region Copyright notice and license
// Protocol Buffers - Google's data interchange format
// Copyright 2015 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd
#endregion

using Google.Protobuf.Reflection;
using Google.Protobuf.TestProtos;
using NUnit.Framework;
using System.Linq;
using UnitTest.Issues.TestProtos;

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

        [Test]
        public void IsWrongType()
        {
            var any = Any.Pack(SampleMessages.CreateFullTestAllTypes());
            Assert.False(any.Is(TestOneof.Descriptor));
        }

        [Test]
        public void IsRightType()
        {
            var any = Any.Pack(SampleMessages.CreateFullTestAllTypes());
            Assert.True(any.Is(TestAllTypes.Descriptor));
        }

        [Test]
        public void Unpack_TypeRegistry()
        {
            var messages = new IMessage[]
            {
                SampleMessages.CreateFullTestAllTypes(),
                new TestWellKnownTypes { BoolField = true },
                new MoreString { Data = { "x" } },
                new MoreBytes { Data = ByteString.CopyFromUtf8("xyz") },
                new ReservedNames { Descriptor_ = 10 }
            };
            var anyMessages = messages.Select(Any.Pack);

            // The type registry handles the first four of the packed messages, but not the final one.
            var registry = TypeRegistry.FromFiles(
                UnittestWellKnownTypesReflection.Descriptor,
                UnittestProto3Reflection.Descriptor);
            var unpacked = anyMessages.Select(any => any.Unpack(registry)).ToList();
            var expected = (IMessage[]) messages.Clone();
            expected[4] = null;
            Assert.AreEqual(expected, unpacked);
        }
    }
}
