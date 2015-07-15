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
    public class WrappersTest
    {
        [Test]
        public void NullIsDefault()
        {
            var message = new TestWellKnownTypes();
            Assert.IsNull(message.StringField);
            Assert.IsNull(message.BytesField);
            Assert.IsNull(message.BoolField);
            Assert.IsNull(message.FloatField);
            Assert.IsNull(message.DoubleField);
            Assert.IsNull(message.Int32Field);
            Assert.IsNull(message.Int64Field);
            Assert.IsNull(message.Uint32Field);
            Assert.IsNull(message.Uint64Field);
        }

        [Test]
        public void NonNullDefaultIsPreservedThroughSerialization()
        {
            var message = new TestWellKnownTypes
            {
                StringField = "",
                BytesField = ByteString.Empty,
                BoolField = false,
                FloatField = 0f,
                DoubleField = 0d,
                Int32Field = 0,
                Int64Field = 0,
                Uint32Field = 0,
                Uint64Field = 0
            };

            var bytes = message.ToByteArray();
            var parsed = TestWellKnownTypes.Parser.ParseFrom(bytes);

            Assert.AreEqual("", message.StringField);
            Assert.AreEqual(ByteString.Empty, message.BytesField);
            Assert.AreEqual(false, message.BoolField);
            Assert.AreEqual(0f, message.FloatField);
            Assert.AreEqual(0d, message.DoubleField);
            Assert.AreEqual(0, message.Int32Field);
            Assert.AreEqual(0L, message.Int64Field);
            Assert.AreEqual(0U, message.Uint32Field);
            Assert.AreEqual(0UL, message.Uint64Field);
        }
    }
}
