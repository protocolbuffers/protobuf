#region Copyright notice and license
// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd
#endregion

using NUnit.Framework;
using ProtobufUnittest;
using System;
using System.IO;
using UnitTest.Issues.TestProtos;

namespace Google.Protobuf.Test
{
    public class Proto3OptionalTest
    {
        [Test]
        public void OptionalInt32FieldLifecycle()
        {
            var message = new TestProto3Optional();
            Assert.IsFalse(message.HasOptionalInt32);
            Assert.AreEqual(0, message.OptionalInt32);

            message.OptionalInt32 = 5;
            Assert.IsTrue(message.HasOptionalInt32);
            Assert.AreEqual(5, message.OptionalInt32);

            message.OptionalInt32 = 0;
            Assert.IsTrue(message.HasOptionalInt32);
            Assert.AreEqual(0, message.OptionalInt32);

            message.ClearOptionalInt32();
            Assert.IsFalse(message.HasOptionalInt32);
            Assert.AreEqual(0, message.OptionalInt32);
        }

        [Test]
        public void OptionalStringFieldLifecycle()
        {
            var message = new TestProto3Optional();
            Assert.IsFalse(message.HasOptionalString);
            Assert.AreEqual("", message.OptionalString);

            message.OptionalString = "x";
            Assert.IsTrue(message.HasOptionalString);
            Assert.AreEqual("x", message.OptionalString);

            message.OptionalString = "";
            Assert.IsTrue(message.HasOptionalString);
            Assert.AreEqual("", message.OptionalString);

            message.ClearOptionalString();
            Assert.IsFalse(message.HasOptionalString);
            Assert.AreEqual("", message.OptionalString);

            Assert.Throws<ArgumentNullException>(() => message.OptionalString = null);
        }

        [Test]
        public void Clone()
        {
            var original = new TestProto3Optional { OptionalInt64 = 0L };

            var clone = original.Clone();
            Assert.False(clone.HasOptionalInt32);
            Assert.AreEqual(0, clone.OptionalInt32);
            Assert.True(clone.HasOptionalInt64);
            Assert.AreEqual(0L, clone.OptionalInt64);
        }

        [Test]
        public void Serialization_NotSet()
        {
            var stream = new MemoryStream();
            var message = new TestProto3Optional();
            message.WriteTo(stream);
            Assert.AreEqual(0, stream.Length);
        }

        [Test]
        public void Serialization_SetToDefault()
        {
            var stream = new MemoryStream();
            var message = new TestProto3Optional { OptionalInt32 = 0 };
            message.WriteTo(stream);
            Assert.AreEqual(2, stream.Length); // Tag and value
        }

        [Test]
        public void Serialization_Roundtrip()
        {
            var original = new TestProto3Optional { OptionalInt64 = 0L, OptionalFixed32 = 5U };
            var stream = new MemoryStream();
            original.WriteTo(stream);
            stream.Position = 0;
            var deserialized = TestProto3Optional.Parser.ParseFrom(stream);

            Assert.AreEqual(0, deserialized.OptionalInt32);
            Assert.IsFalse(deserialized.HasOptionalInt32);

            Assert.AreEqual(0L, deserialized.OptionalInt64);
            Assert.IsTrue(deserialized.HasOptionalInt64);

            Assert.AreEqual(5U, deserialized.OptionalFixed32);
            Assert.IsTrue(deserialized.HasOptionalFixed32);
        }

        [Test]
        public void Equality_IgnoresPresence()
        {
            var message1 = new TestProto3Optional { OptionalInt32 = 0 };
            var message2 = new TestProto3Optional();

            Assert.IsTrue(message1.Equals(message2));
            message1.ClearOptionalInt32();
        }

        [Test]
        public void MixedFields()
        {
            var descriptor = MixedRegularAndOptional.Descriptor;
            Assert.AreEqual(1, descriptor.Oneofs.Count);
            Assert.AreEqual(0, descriptor.RealOneofCount);
            Assert.True(descriptor.Oneofs[0].IsSynthetic);
        }
    }
}
