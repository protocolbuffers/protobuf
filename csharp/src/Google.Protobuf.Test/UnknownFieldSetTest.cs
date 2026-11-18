#region Copyright notice and license
// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd
#endregion

using System.IO;
using Google.Protobuf.TestProtos;
using NUnit.Framework;

namespace Google.Protobuf
{
    public class UnknownFieldSetTest
    {
        public class Data
        {
            public static System.Collections.IEnumerable Messages
            {
                get
                {
                    yield return SampleMessages.CreateFullTestAllTypesProto2();
                    yield return SampleMessages.CreateFullTestAllTypes();
                }
            }
        }

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
        [TestCaseSource(typeof(Data), "Messages")]
        public void TestMergeCodedInput(IMessage message)
        {
            var emptyMessage = new TestEmptyMessage();
            emptyMessage.MergeFrom(message.ToByteArray());
            Assert.AreEqual(message.CalculateSize(), emptyMessage.CalculateSize());
            Assert.AreEqual(message.ToByteArray(), emptyMessage.ToByteArray());

            var newMessage = message.Descriptor.Parser.ParseFrom(emptyMessage.ToByteArray());
            Assert.AreEqual(message, newMessage);
            Assert.AreEqual(message.CalculateSize(), newMessage.CalculateSize());
        }

        [Test]
        [TestCaseSource(typeof(Data), "Messages")]
        public void TestMergeMessage(IMessage message)
        {
            var emptyMessage = new TestEmptyMessage();
            var otherEmptyMessage = new TestEmptyMessage();
            emptyMessage.MergeFrom(message.ToByteArray());
            otherEmptyMessage.MergeFrom(emptyMessage);

            Assert.AreEqual(message.CalculateSize(), otherEmptyMessage.CalculateSize());
            Assert.AreEqual(message.ToByteArray(), otherEmptyMessage.ToByteArray());
        }

        [Test]
        [TestCaseSource(typeof(Data), "Messages")]
        public void TestEquals(IMessage message)
        {
            var emptyMessage = new TestEmptyMessage();
            var otherEmptyMessage = new TestEmptyMessage();
            Assert.AreEqual(emptyMessage, otherEmptyMessage);
            emptyMessage.MergeFrom(message.ToByteArray());
            Assert.AreNotEqual(emptyMessage.CalculateSize(),
                               otherEmptyMessage.CalculateSize());
            Assert.AreNotEqual(emptyMessage, otherEmptyMessage);
        }

        [Test]
        [TestCaseSource(typeof(Data), "Messages")]
        public void TestHashCode(IMessage message)
        {
            var emptyMessage = new TestEmptyMessage();
            int hashCode = emptyMessage.GetHashCode();
            emptyMessage.MergeFrom(message.ToByteArray());
            Assert.AreNotEqual(hashCode, emptyMessage.GetHashCode());
        }

        [Test]
        [TestCaseSource(typeof(Data), "Messages")]
        public void TestClone(IMessage message)
        {
            var emptyMessage = new TestEmptyMessage();
            TestEmptyMessage otherEmptyMessage = emptyMessage.Clone();
            Assert.AreEqual(emptyMessage.CalculateSize(), otherEmptyMessage.CalculateSize());
            Assert.AreEqual(emptyMessage.ToByteArray(), otherEmptyMessage.ToByteArray());

            emptyMessage.MergeFrom(message.ToByteArray());
            otherEmptyMessage = emptyMessage.Clone();
            Assert.AreEqual(message.CalculateSize(), otherEmptyMessage.CalculateSize());
            Assert.AreEqual(message.ToByteArray(), otherEmptyMessage.ToByteArray());
        }

        [Test]
        public void TestClone_LengthDelimited()
        {
            var unknownVarintField = new UnknownField();
            unknownVarintField.AddVarint(99);

            var unknownLengthDelimitedField1 = new UnknownField();
            unknownLengthDelimitedField1.AddLengthDelimited(ByteString.CopyFromUtf8("some data"));

            var unknownLengthDelimitedField2 = new UnknownField();
            unknownLengthDelimitedField2.AddLengthDelimited(ByteString.CopyFromUtf8("some more data"));

            var destUnknownFieldSet = new UnknownFieldSet();
            destUnknownFieldSet.AddOrReplaceField(997, unknownVarintField);
            destUnknownFieldSet.AddOrReplaceField(999, unknownLengthDelimitedField1);
            destUnknownFieldSet.AddOrReplaceField(999, unknownLengthDelimitedField2);

            var clone = UnknownFieldSet.Clone(destUnknownFieldSet);

            Assert.IsTrue(clone.HasField(997));
            Assert.IsTrue(clone.HasField(999));
        }

        [Test]
        [TestCaseSource(typeof(Data), "Messages")]
        public void TestDiscardUnknownFields(IMessage message)
        {
            var goldenEmptyMessage = new TestEmptyMessage();
            byte[] data = message.ToByteArray();
            int fullSize = message.CalculateSize();

            void AssertEmpty(IMessage msg)
            {
                Assert.AreEqual(0, msg.CalculateSize());
                Assert.AreEqual(goldenEmptyMessage, msg);
            }

            void AssertFull(IMessage msg) => Assert.AreEqual(fullSize, msg.CalculateSize());

            // Test the behavior of the parsers with and without discarding, both generic and non-generic.
            MessageParser<TestEmptyMessage> retainingParser1 = TestEmptyMessage.Parser;
            MessageParser retainingParser2 = retainingParser1;
            MessageParser<TestEmptyMessage> discardingParser1 = retainingParser1.WithDiscardUnknownFields(true);
            MessageParser discardingParser2 = retainingParser2.WithDiscardUnknownFields(true);

            // Test parse from byte[]
            MessageParsingHelpers.AssertReadingMessage(retainingParser1, data, m => AssertFull(m));
            MessageParsingHelpers.AssertReadingMessage(retainingParser2, data, m => AssertFull(m));
            MessageParsingHelpers.AssertReadingMessage(discardingParser1, data, m => AssertEmpty(m));
            MessageParsingHelpers.AssertReadingMessage(discardingParser2, data, m => AssertEmpty(m));

            // Test parse from byte[] with offset
            AssertFull(retainingParser1.ParseFrom(data, 0, data.Length));
            AssertFull(retainingParser2.ParseFrom(data, 0, data.Length));
            AssertEmpty(discardingParser1.ParseFrom(data, 0, data.Length));
            AssertEmpty(discardingParser2.ParseFrom(data, 0, data.Length));

            // Test parse from CodedInputStream
            AssertFull(retainingParser1.ParseFrom(new CodedInputStream(data)));
            AssertFull(retainingParser2.ParseFrom(new CodedInputStream(data)));
            AssertEmpty(discardingParser1.ParseFrom(new CodedInputStream(data)));
            AssertEmpty(discardingParser2.ParseFrom(new CodedInputStream(data)));

            // Test parse from Stream
            AssertFull(retainingParser1.ParseFrom(new MemoryStream(data)));
            AssertFull(retainingParser2.ParseFrom(new MemoryStream(data)));
            AssertEmpty(discardingParser1.ParseFrom(new MemoryStream(data)));
            AssertEmpty(discardingParser2.ParseFrom(new MemoryStream(data)));
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
