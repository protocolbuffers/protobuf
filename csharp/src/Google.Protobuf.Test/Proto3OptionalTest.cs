#region Copyright notice and license
// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
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
