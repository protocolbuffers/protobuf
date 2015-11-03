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

using System;
using Google.Protobuf.TestProtos;
using NUnit.Framework;
using System.Collections;
using System.IO;

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
        public void NonDefaultSingleValues()
        {
            var message = new TestWellKnownTypes
            {
                StringField = "x",
                BytesField = ByteString.CopyFrom(1, 2, 3),
                BoolField = true,
                FloatField = 12.5f,
                DoubleField = 12.25d,
                Int32Field = 1,
                Int64Field = 2,
                Uint32Field = 3,
                Uint64Field = 4
            };

            var bytes = message.ToByteArray();
            var parsed = TestWellKnownTypes.Parser.ParseFrom(bytes);

            Assert.AreEqual("x", parsed.StringField);
            Assert.AreEqual(ByteString.CopyFrom(1, 2, 3), parsed.BytesField);
            Assert.AreEqual(true, parsed.BoolField);
            Assert.AreEqual(12.5f, parsed.FloatField);
            Assert.AreEqual(12.25d, parsed.DoubleField);
            Assert.AreEqual(1, parsed.Int32Field);
            Assert.AreEqual(2L, parsed.Int64Field);
            Assert.AreEqual(3U, parsed.Uint32Field);
            Assert.AreEqual(4UL, parsed.Uint64Field);
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

            Assert.AreEqual("", parsed.StringField);
            Assert.AreEqual(ByteString.Empty, parsed.BytesField);
            Assert.AreEqual(false, parsed.BoolField);
            Assert.AreEqual(0f, parsed.FloatField);
            Assert.AreEqual(0d, parsed.DoubleField);
            Assert.AreEqual(0, parsed.Int32Field);
            Assert.AreEqual(0L, parsed.Int64Field);
            Assert.AreEqual(0U, parsed.Uint32Field);
            Assert.AreEqual(0UL, parsed.Uint64Field);
        }

        [Test]
        public void RepeatedWrappersProhibitNullItems()
        {
            var message = new RepeatedWellKnownTypes();
            Assert.Throws<ArgumentNullException>(() => message.BoolField.Add((bool?) null));
            Assert.Throws<ArgumentNullException>(() => message.Int32Field.Add((int?) null));
            Assert.Throws<ArgumentNullException>(() => message.StringField.Add((string) null));
            Assert.Throws<ArgumentNullException>(() => message.BytesField.Add((ByteString) null));
        }

        [Test]
        public void RepeatedWrappersSerializeDeserialize()
        {
            var message = new RepeatedWellKnownTypes
            {
                BoolField = { true, false },
                BytesField = { ByteString.CopyFrom(1, 2, 3), ByteString.CopyFrom(4, 5, 6), ByteString.Empty },
                DoubleField = { 12.5, -1.5, 0d },
                FloatField = { 123.25f, -20f, 0f },
                Int32Field = { int.MaxValue, int.MinValue, 0 },
                Int64Field = { long.MaxValue, long.MinValue, 0L },                
                StringField = { "First", "Second", "" },
                Uint32Field = { uint.MaxValue, uint.MinValue, 0U },
                Uint64Field = { ulong.MaxValue, ulong.MinValue, 0UL },
            };
            var bytes = message.ToByteArray();
            var parsed = RepeatedWellKnownTypes.Parser.ParseFrom(bytes);

            Assert.AreEqual(message, parsed);
            // Just to test a single value for sanity...
            Assert.AreEqual("Second", message.StringField[1]);
        }

        [Test]
        public void MapWrappersSerializeDeserialize()
        {
            var message = new MapWellKnownTypes
            {
                BoolField = { { 10, false }, { 20, true } },
                BytesField = {
                    { -1, ByteString.CopyFrom(1, 2, 3) },
                    { 10, ByteString.CopyFrom(4, 5, 6) },
                    { 1000, ByteString.Empty },
                    { 10000, null }
                },
                DoubleField = { { 1, 12.5 }, { 10, -1.5 }, { 20, 0d } },
                FloatField = { { 2, 123.25f }, { 3, -20f }, { 4, 0f } },
                Int32Field = { { 5, int.MaxValue }, { 6, int.MinValue }, { 7, 0 } },
                Int64Field = { { 8, long.MaxValue }, { 9, long.MinValue }, { 10, 0L } },
                StringField = { { 11, "First" }, { 12, "Second" }, { 13, "" }, { 14, null } },
                Uint32Field = { { 15, uint.MaxValue }, { 16, uint.MinValue }, { 17, 0U } },
                Uint64Field = { { 18, ulong.MaxValue }, { 19, ulong.MinValue }, { 20, 0UL } },
            };

            var bytes = message.ToByteArray();
            var parsed = MapWellKnownTypes.Parser.ParseFrom(bytes);

            Assert.AreEqual(message, parsed);
            // Just to test a single value for sanity...
            Assert.AreEqual("Second", message.StringField[12]);
        }

        [Test]
        public void Reflection_SingleValues()
        {
            var message = new TestWellKnownTypes
            {
                StringField = "x",
                BytesField = ByteString.CopyFrom(1, 2, 3),
                BoolField = true,
                FloatField = 12.5f,
                DoubleField = 12.25d,
                Int32Field = 1,
                Int64Field = 2,
                Uint32Field = 3,
                Uint64Field = 4
            };
            var fields = TestWellKnownTypes.Descriptor.Fields;

            Assert.AreEqual("x", fields[TestWellKnownTypes.StringFieldFieldNumber].Accessor.GetValue(message));
            Assert.AreEqual(ByteString.CopyFrom(1, 2, 3), fields[TestWellKnownTypes.BytesFieldFieldNumber].Accessor.GetValue(message));
            Assert.AreEqual(true, fields[TestWellKnownTypes.BoolFieldFieldNumber].Accessor.GetValue(message));
            Assert.AreEqual(12.5f, fields[TestWellKnownTypes.FloatFieldFieldNumber].Accessor.GetValue(message));
            Assert.AreEqual(12.25d, fields[TestWellKnownTypes.DoubleFieldFieldNumber].Accessor.GetValue(message));
            Assert.AreEqual(1, fields[TestWellKnownTypes.Int32FieldFieldNumber].Accessor.GetValue(message));
            Assert.AreEqual(2L, fields[TestWellKnownTypes.Int64FieldFieldNumber].Accessor.GetValue(message));
            Assert.AreEqual(3U, fields[TestWellKnownTypes.Uint32FieldFieldNumber].Accessor.GetValue(message));
            Assert.AreEqual(4UL, fields[TestWellKnownTypes.Uint64FieldFieldNumber].Accessor.GetValue(message));

            // And a couple of null fields...
            message.StringField = null;
            message.FloatField = null;
            Assert.IsNull(fields[TestWellKnownTypes.StringFieldFieldNumber].Accessor.GetValue(message));
            Assert.IsNull(fields[TestWellKnownTypes.FloatFieldFieldNumber].Accessor.GetValue(message));
        }

        [Test]
        public void Reflection_RepeatedFields()
        {
            // Just a single example... note that we can't have a null value here
            var message = new RepeatedWellKnownTypes { Int32Field = { 1, 2 } };
            var fields = RepeatedWellKnownTypes.Descriptor.Fields;
            var list = (IList) fields[RepeatedWellKnownTypes.Int32FieldFieldNumber].Accessor.GetValue(message);
            CollectionAssert.AreEqual(new[] { 1, 2 }, list);
        }

        [Test]
        public void Reflection_MapFields()
        {
            // Just a single example... note that we can't have a null value here
            var message = new MapWellKnownTypes { Int32Field = { { 1, 2 }, { 3, null } } };
            var fields = MapWellKnownTypes.Descriptor.Fields;
            var dictionary = (IDictionary) fields[MapWellKnownTypes.Int32FieldFieldNumber].Accessor.GetValue(message);
            Assert.AreEqual(2, dictionary[1]);
            Assert.IsNull(dictionary[3]);
            Assert.IsTrue(dictionary.Contains(3));
        }

        [Test]
        public void Oneof()
        {
            var message = new OneofWellKnownTypes { EmptyField = new Empty() };
            // Start off with a non-wrapper
            Assert.AreEqual(OneofWellKnownTypes.OneofFieldOneofCase.EmptyField, message.OneofFieldCase);
            AssertOneofRoundTrip(message);

            message.StringField = "foo";
            Assert.AreEqual(OneofWellKnownTypes.OneofFieldOneofCase.StringField, message.OneofFieldCase);
            AssertOneofRoundTrip(message);

            message.StringField = "foo";
            Assert.AreEqual(OneofWellKnownTypes.OneofFieldOneofCase.StringField, message.OneofFieldCase);
            AssertOneofRoundTrip(message);

            message.DoubleField = 0.0f;
            Assert.AreEqual(OneofWellKnownTypes.OneofFieldOneofCase.DoubleField, message.OneofFieldCase);
            AssertOneofRoundTrip(message);

            message.DoubleField = 1.0f;
            Assert.AreEqual(OneofWellKnownTypes.OneofFieldOneofCase.DoubleField, message.OneofFieldCase);
            AssertOneofRoundTrip(message);

            message.ClearOneofField();
            Assert.AreEqual(OneofWellKnownTypes.OneofFieldOneofCase.None, message.OneofFieldCase);
            AssertOneofRoundTrip(message);
        }

        private void AssertOneofRoundTrip(OneofWellKnownTypes message)
        {
            // Normal roundtrip, but explicitly checking the case...
            var bytes = message.ToByteArray();
            var parsed = OneofWellKnownTypes.Parser.ParseFrom(bytes);
            Assert.AreEqual(message, parsed);
            Assert.AreEqual(message.OneofFieldCase, parsed.OneofFieldCase);
        }

        [Test]
        [TestCase("x", "y", "y")]
        [TestCase("x", "", "x")]
        [TestCase("x", null, "x")]
        [TestCase("", "y", "y")]
        [TestCase("", "", "")]
        [TestCase("", null, "")]
        [TestCase(null, "y", "y")]
        [TestCase(null, "", "")]
        [TestCase(null, null, null)]
        public void Merging(string original, string merged, string expected)
        {
            var originalMessage = new TestWellKnownTypes { StringField = original };
            var mergingMessage = new TestWellKnownTypes { StringField = merged };
            originalMessage.MergeFrom(mergingMessage);
            Assert.AreEqual(expected, originalMessage.StringField);

            // Try it using MergeFrom(CodedInputStream) too...
            originalMessage = new TestWellKnownTypes { StringField = original };
            originalMessage.MergeFrom(mergingMessage.ToByteArray());
            Assert.AreEqual(expected, originalMessage.StringField);
        }

        // Merging is odd with wrapper types, due to the way that default values aren't emitted in
        // the binary stream. In fact we cheat a little bit - a message with an explicitly present default
        // value will have that default value ignored.
        [Test]
        public void MergingCornerCase()
        {
            var message = new TestWellKnownTypes { Int32Field = 5 };

            // Create a byte array which has the data of an Int32Value explicitly containing a value of 0.
            // This wouldn't normally happen.
            byte[] bytes;
            var wrapperTag = WireFormat.MakeTag(TestWellKnownTypes.Int32FieldFieldNumber, WireFormat.WireType.LengthDelimited);
            var valueTag = WireFormat.MakeTag(Int32Value.ValueFieldNumber, WireFormat.WireType.Varint);
            using (var stream = new MemoryStream())
            {
                var coded = new CodedOutputStream(stream);
                coded.WriteTag(wrapperTag);
                coded.WriteLength(2); // valueTag + a value 0, each one byte
                coded.WriteTag(valueTag);
                coded.WriteInt32(0);
                coded.Flush();
                bytes = stream.ToArray();
            }

            message.MergeFrom(bytes);
            // A normal implementation would have 0 now, as the explicit default would have been overwritten the 5.
            Assert.AreEqual(5, message.Int32Field);
        }

        [Test]
        public void UnknownFieldInWrapper()
        {
            var stream = new MemoryStream();
            var output = new CodedOutputStream(stream);
            var wrapperTag = WireFormat.MakeTag(TestWellKnownTypes.Int32FieldFieldNumber, WireFormat.WireType.LengthDelimited);
            var unknownTag = WireFormat.MakeTag(15, WireFormat.WireType.Varint);
            var valueTag = WireFormat.MakeTag(Int32Value.ValueFieldNumber, WireFormat.WireType.Varint);

            output.WriteTag(wrapperTag);
            output.WriteLength(4); // unknownTag + value 5 + valueType + value 6, each 1 byte
            output.WriteTag(unknownTag);
            output.WriteInt32((int) valueTag); // Sneakily "pretend" it's a tag when it's really a value
            output.WriteTag(valueTag);
            output.WriteInt32(6);

            output.Flush();
            stream.Position = 0;
            
            var message = TestWellKnownTypes.Parser.ParseFrom(stream);
            Assert.AreEqual(6, message.Int32Field);
        }

        [Test]
        public void ClearWithReflection()
        {
            // String and Bytes are the tricky ones here, as the CLR type of the property
            // is the same between the wrapper and non-wrapper types.
            var message = new TestWellKnownTypes { StringField = "foo" };
            TestWellKnownTypes.Descriptor.Fields[TestWellKnownTypes.StringFieldFieldNumber].Accessor.Clear(message);
            Assert.IsNull(message.StringField);
        }
    }
}
