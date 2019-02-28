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

using System;
using System.IO;
using Google.Protobuf.TestProtos;
using NUnit.Framework;

namespace Google.Protobuf
{
    public class UnknownFieldSetTest
    {
        [Test]
        public void EmptyUnknownFieldSet()
        {
            UnknownFieldSet unknownFields = new UnknownFieldSet();
            Assert.AreEqual(0, unknownFields.CalculateSize());
        }

        [Test]
        public void MergeUnknownFieldSet()
        {
            UnknownFieldSet unknownFields = new UnknownFieldSet();
            UnknownField field = new UnknownField();
            field.AddFixed32(123);
            unknownFields.AddOrReplaceField(1, field);
            UnknownFieldSet otherUnknownFields = new UnknownFieldSet();
            Assert.IsFalse(otherUnknownFields.HasField(1));
            UnknownFieldSet.MergeFrom(otherUnknownFields, unknownFields);
            Assert.IsTrue(otherUnknownFields.HasField(1));
        }

        [Test]
        public void TestMergeCodedInput()
        {
            var message = SampleMessages.CreateFullTestAllTypes();
            var emptyMessage = new TestEmptyMessage();
            emptyMessage.MergeFrom(message.ToByteArray());
            Assert.AreEqual(message.CalculateSize(), emptyMessage.CalculateSize());
            Assert.AreEqual(message.ToByteArray(), emptyMessage.ToByteArray());

            var newMessage = new TestAllTypes();
            newMessage.MergeFrom(emptyMessage.ToByteArray());
            Assert.AreEqual(message, newMessage);
            Assert.AreEqual(message.CalculateSize(), newMessage.CalculateSize());
        }

        [Test]
        public void TestMergeMessage()
        {
            var message = SampleMessages.CreateFullTestAllTypes();
            var emptyMessage = new TestEmptyMessage();
            var otherEmptyMessage = new TestEmptyMessage();
            emptyMessage.MergeFrom(message.ToByteArray());
            otherEmptyMessage.MergeFrom(emptyMessage);

            Assert.AreEqual(message.CalculateSize(), otherEmptyMessage.CalculateSize());
            Assert.AreEqual(message.ToByteArray(), otherEmptyMessage.ToByteArray());
        }

        [Test]
        public void TestEquals()
        {
            var message = SampleMessages.CreateFullTestAllTypes();
            var emptyMessage = new TestEmptyMessage();
            var otherEmptyMessage = new TestEmptyMessage();
            Assert.AreEqual(emptyMessage, otherEmptyMessage);
            emptyMessage.MergeFrom(message.ToByteArray());
            Assert.AreNotEqual(emptyMessage.CalculateSize(),
                               otherEmptyMessage.CalculateSize());
            Assert.AreNotEqual(emptyMessage, otherEmptyMessage);
        }

        [Test]
        public void TestHashCode()
        {
            var message = SampleMessages.CreateFullTestAllTypes();
            var emptyMessage = new TestEmptyMessage();
            int hashCode = emptyMessage.GetHashCode();
            emptyMessage.MergeFrom(message.ToByteArray());
            Assert.AreNotEqual(hashCode, emptyMessage.GetHashCode());
        }

        [Test]
        public void TestClone()
        {
            var emptyMessage = new TestEmptyMessage();
            var otherEmptyMessage = new TestEmptyMessage();
            otherEmptyMessage = emptyMessage.Clone();
            Assert.AreEqual(emptyMessage.CalculateSize(), otherEmptyMessage.CalculateSize());
            Assert.AreEqual(emptyMessage.ToByteArray(), otherEmptyMessage.ToByteArray());

            var message = SampleMessages.CreateFullTestAllTypes();
            emptyMessage.MergeFrom(message.ToByteArray());
            otherEmptyMessage = emptyMessage.Clone();
            Assert.AreEqual(message.CalculateSize(), otherEmptyMessage.CalculateSize());
            Assert.AreEqual(message.ToByteArray(), otherEmptyMessage.ToByteArray());
        }

        [Test]
        public void TestDiscardUnknownFields()
        {
            var message = SampleMessages.CreateFullTestAllTypes();
            var goldenEmptyMessage = new TestEmptyMessage();
            byte[] data = message.ToByteArray();
            int fullSize = message.CalculateSize();

            Action<IMessage> assertEmpty = msg =>
            {
                Assert.AreEqual(0, msg.CalculateSize());
                Assert.AreEqual(goldenEmptyMessage, msg);
            };

            Action<IMessage> assertFull = msg => Assert.AreEqual(fullSize, msg.CalculateSize());

            // Test the behavior of the parsers with and without discarding, both generic and non-generic.
            MessageParser<TestEmptyMessage> retainingParser1 = TestEmptyMessage.Parser;
            MessageParser retainingParser2 = retainingParser1;
            MessageParser<TestEmptyMessage> discardingParser1 = retainingParser1.WithDiscardUnknownFields(true);
            MessageParser discardingParser2 = retainingParser2.WithDiscardUnknownFields(true);

            // Test parse from byte[]
            assertFull(retainingParser1.ParseFrom(data));
            assertFull(retainingParser2.ParseFrom(data));
            assertEmpty(discardingParser1.ParseFrom(data));
            assertEmpty(discardingParser2.ParseFrom(data));

            // Test parse from byte[] with offset
            assertFull(retainingParser1.ParseFrom(data, 0, data.Length));
            assertFull(retainingParser2.ParseFrom(data, 0, data.Length));
            assertEmpty(discardingParser1.ParseFrom(data, 0, data.Length));
            assertEmpty(discardingParser2.ParseFrom(data, 0, data.Length));

            // Test parse from CodedInputStream
            assertFull(retainingParser1.ParseFrom(new CodedInputStream(data)));
            assertFull(retainingParser2.ParseFrom(new CodedInputStream(data)));
            assertEmpty(discardingParser1.ParseFrom(new CodedInputStream(data)));
            assertEmpty(discardingParser2.ParseFrom(new CodedInputStream(data)));

            // Test parse from Stream
            assertFull(retainingParser1.ParseFrom(new MemoryStream(data)));
            assertFull(retainingParser2.ParseFrom(new MemoryStream(data)));
            assertEmpty(discardingParser1.ParseFrom(new MemoryStream(data)));
            assertEmpty(discardingParser2.ParseFrom(new MemoryStream(data)));
        }

        [Test]
        public void TestReadInvalidWireTypeThrowsInvalidProtocolBufferException()
        {
            MemoryStream ms = new MemoryStream();
            CodedOutputStream output = new CodedOutputStream(ms);

            uint tag = WireFormat.MakeTag(1, (WireFormat.WireType)6);
            output.WriteRawVarint32(tag);
            output.WriteLength(-1);
            output.Flush();
            ms.Position = 0;

            CodedInputStream input = new CodedInputStream(ms);
            Assert.AreEqual(tag, input.ReadTag());

            Assert.Throws<InvalidProtocolBufferException>(() => UnknownFieldSet.MergeFieldFrom(null, input));
        }
    }
}
