#region Copyright notice and license
// Protocol Buffers - Google's data interchange format
// Copyright 2015 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd
#endregion

using Google.Protobuf.Collections;
using Google.Protobuf.TestProtos;
using Google.Protobuf.WellKnownTypes;
using NUnit.Framework;
using System;
using System.IO;
using System.Linq;

namespace Google.Protobuf
{
    /// <summary>
    /// Tests around the generated TestAllTypes message.
    /// </summary>
    public partial class GeneratedMessageTest
    {
        [Test]
        public void EmptyMessageFieldDistinctFromMissingMessageField()
        {
            // This demonstrates what we're really interested in...
            var message1 = new TestAllTypes { SingleForeignMessage = new ForeignMessage() };
            var message2 = new TestAllTypes(); // SingleForeignMessage is null
            EqualityTester.AssertInequality(message1, message2);
        }

        [Test]
        public void DefaultValues()
        {
            // Single fields
            var message = new TestAllTypes();
            Assert.AreEqual(false, message.SingleBool);
            Assert.AreEqual(ByteString.Empty, message.SingleBytes);
            Assert.AreEqual(0.0, message.SingleDouble);
            Assert.AreEqual(0, message.SingleFixed32);
            Assert.AreEqual(0L, message.SingleFixed64);
            Assert.AreEqual(0.0f, message.SingleFloat);
            Assert.AreEqual(ForeignEnum.ForeignUnspecified, message.SingleForeignEnum);
            Assert.IsNull(message.SingleForeignMessage);
            Assert.AreEqual(ImportEnum.Unspecified, message.SingleImportEnum);
            Assert.IsNull(message.SingleImportMessage);
            Assert.AreEqual(0, message.SingleInt32);
            Assert.AreEqual(0L, message.SingleInt64);
            Assert.AreEqual(TestAllTypes.Types.NestedEnum.Unspecified, message.SingleNestedEnum);
            Assert.IsNull(message.SingleNestedMessage);
            Assert.IsNull(message.SinglePublicImportMessage);
            Assert.AreEqual(0, message.SingleSfixed32);
            Assert.AreEqual(0L, message.SingleSfixed64);
            Assert.AreEqual(0, message.SingleSint32);
            Assert.AreEqual(0L, message.SingleSint64);
            Assert.AreEqual("", message.SingleString);
            Assert.AreEqual(0U, message.SingleUint32);
            Assert.AreEqual(0UL, message.SingleUint64);

            // Repeated fields
            Assert.AreEqual(0, message.RepeatedBool.Count);
            Assert.AreEqual(0, message.RepeatedBytes.Count);
            Assert.AreEqual(0, message.RepeatedDouble.Count);
            Assert.AreEqual(0, message.RepeatedFixed32.Count);
            Assert.AreEqual(0, message.RepeatedFixed64.Count);
            Assert.AreEqual(0, message.RepeatedFloat.Count);
            Assert.AreEqual(0, message.RepeatedForeignEnum.Count);
            Assert.AreEqual(0, message.RepeatedForeignMessage.Count);
            Assert.AreEqual(0, message.RepeatedImportEnum.Count);
            Assert.AreEqual(0, message.RepeatedImportMessage.Count);
            Assert.AreEqual(0, message.RepeatedNestedEnum.Count);
            Assert.AreEqual(0, message.RepeatedNestedMessage.Count);
            Assert.AreEqual(0, message.RepeatedPublicImportMessage.Count);
            Assert.AreEqual(0, message.RepeatedSfixed32.Count);
            Assert.AreEqual(0, message.RepeatedSfixed64.Count);
            Assert.AreEqual(0, message.RepeatedSint32.Count);
            Assert.AreEqual(0, message.RepeatedSint64.Count);
            Assert.AreEqual(0, message.RepeatedString.Count);
            Assert.AreEqual(0, message.RepeatedUint32.Count);
            Assert.AreEqual(0, message.RepeatedUint64.Count);

            // Oneof fields
            Assert.AreEqual(TestAllTypes.OneofFieldOneofCase.None, message.OneofFieldCase);
            Assert.AreEqual(0, message.OneofUint32);
            Assert.AreEqual("", message.OneofString);
            Assert.AreEqual(ByteString.Empty, message.OneofBytes);
            Assert.IsNull(message.OneofNestedMessage);
        }

        [Test]
        public void NullStringAndBytesRejected()
        {
            var message = new TestAllTypes();
            Assert.Throws<ArgumentNullException>(() => message.SingleString = null);
            Assert.Throws<ArgumentNullException>(() => message.OneofString = null);
            Assert.Throws<ArgumentNullException>(() => message.SingleBytes = null);
            Assert.Throws<ArgumentNullException>(() => message.OneofBytes = null);
        }

        [Test]
        public void Roundtrip_UnpairedSurrogate()
        {
            var message = new TestAllTypes { SingleString = "\ud83d" };

            Assert.AreEqual("\ud83d", message.SingleString);

            // The serialized bytes contain the replacement character.
            var bytes = message.ToByteArray();
            CollectionAssert.AreEqual(bytes, new byte[] { 0x72, 3, 0xEF, 0xBF, 0xBD });
        }

        [Test]
        public void ParseInvalidUtf8Rejected()
        {
            var payload = new byte[] { 0x72, 1, 0x80 };
            Assert.Throws<InvalidProtocolBufferException>(() => TestAllTypes.Parser.ParseFrom(payload));
        }

        [Test]
        public void RoundTrip_Empty()
        {
            var message = new TestAllTypes();
            // Without setting any values, there's nothing to write.
            byte[] bytes = message.ToByteArray();
            Assert.AreEqual(0, bytes.Length);

            MessageParsingHelpers.AssertWritingMessage(message);

            MessageParsingHelpers.AssertRoundtrip(TestAllTypes.Parser, message);
        }

        [Test]
        public void RoundTrip_SingleValues()
        {
            var message = new TestAllTypes
            {
                SingleBool = true,
                SingleBytes = ByteString.CopyFrom(1, 2, 3, 4),
                SingleDouble = 23.5,
                SingleFixed32 = 23,
                SingleFixed64 = 1234567890123,
                SingleFloat = 12.25f,
                SingleForeignEnum = ForeignEnum.ForeignBar,
                SingleForeignMessage = new ForeignMessage { C = 10 },
                SingleImportEnum = ImportEnum.ImportBaz,
                SingleImportMessage = new ImportMessage { D = 20 },
                SingleInt32 = 100,
                SingleInt64 = 3210987654321,
                SingleNestedEnum = TestAllTypes.Types.NestedEnum.Foo,
                SingleNestedMessage = new TestAllTypes.Types.NestedMessage { Bb = 35 },
                SinglePublicImportMessage = new PublicImportMessage { E = 54 },
                SingleSfixed32 = -123,
                SingleSfixed64 = -12345678901234,
                SingleSint32 = -456,
                SingleSint64 = -12345678901235,
                SingleString = "test",
                SingleUint32 = uint.MaxValue,
                SingleUint64 = ulong.MaxValue
            };

            MessageParsingHelpers.AssertWritingMessage(message);

            MessageParsingHelpers.AssertRoundtrip(TestAllTypes.Parser, message);
        }

        [Test]
        public void RoundTrip_RepeatedValues()
        {
            var message = new TestAllTypes
            {
                RepeatedBool = { true, false },
                RepeatedBytes = { ByteString.CopyFrom(1, 2, 3, 4), ByteString.CopyFrom(5, 6) },
                RepeatedDouble = { -12.25, 23.5 },
                RepeatedFixed32 = { uint.MaxValue, 23 },
                RepeatedFixed64 = { ulong.MaxValue, 1234567890123 },
                RepeatedFloat = { 100f, 12.25f },
                RepeatedForeignEnum = { ForeignEnum.ForeignFoo, ForeignEnum.ForeignBar },
                RepeatedForeignMessage = { new ForeignMessage(), new ForeignMessage { C = 10 } },
                RepeatedImportEnum = { ImportEnum.ImportBaz, ImportEnum.Unspecified },
                RepeatedImportMessage = { new ImportMessage { D = 20 }, new ImportMessage { D = 25 } },
                RepeatedInt32 = { 100, 200 },
                RepeatedInt64 = { 3210987654321, long.MaxValue },
                RepeatedNestedEnum = { TestAllTypes.Types.NestedEnum.Foo, TestAllTypes.Types.NestedEnum.Neg },
                RepeatedNestedMessage = { new TestAllTypes.Types.NestedMessage { Bb = 35 }, new TestAllTypes.Types.NestedMessage { Bb = 10 } },
                RepeatedPublicImportMessage = { new PublicImportMessage { E = 54 }, new PublicImportMessage { E = -1 } },
                RepeatedSfixed32 = { -123, 123 },
                RepeatedSfixed64 = { -12345678901234, 12345678901234 },
                RepeatedSint32 = { -456, 100 },
                RepeatedSint64 = { -12345678901235, 123 },
                RepeatedString = { "foo", "bar" },
                RepeatedUint32 = { uint.MaxValue, uint.MinValue },
                RepeatedUint64 = { ulong.MaxValue, uint.MinValue }
            };

            MessageParsingHelpers.AssertWritingMessage(message);

            MessageParsingHelpers.AssertRoundtrip(TestAllTypes.Parser, message);
        }

        // Note that not every map within map_unittest_proto3 is used. They all go through very
        // similar code paths. The fact that all maps are present is validation that we have codecs
        // for every type.
        [Test]
        public void RoundTrip_Maps()
        {
            var message = new TestMap
            {
                MapBoolBool = {
                    { false, true },
                    { true, false }
                },
                MapInt32Bytes = {
                    { 5, ByteString.CopyFrom(6, 7, 8) },
                    { 25, ByteString.CopyFrom(1, 2, 3, 4, 5) },
                    { 10, ByteString.Empty }
                },
                MapInt32ForeignMessage = {
                    { 0, new ForeignMessage { C = 10 } },
                    { 5, new ForeignMessage() },
                },
                MapInt32Enum = {
                    { 1, MapEnum.Bar },
                    { 2000, MapEnum.Foo }
                }
            };

            MessageParsingHelpers.AssertWritingMessage(message);

            MessageParsingHelpers.AssertRoundtrip(TestMap.Parser, message);
        }

        [Test]
        public void MapWithEmptyEntry()
        {
            var message = new TestMap
            {
                MapInt32Bytes = { { 0, ByteString.Empty } }
            };

            byte[] bytes = message.ToByteArray();
            Assert.AreEqual(2, bytes.Length); // Tag for field entry (1 byte), length of entry (0; 1 byte)

            MessageParsingHelpers.AssertWritingMessage(message);

            MessageParsingHelpers.AssertReadingMessage(
                TestMap.Parser,
                bytes,
                parsed=>
                {
                    Assert.AreEqual(1, parsed.MapInt32Bytes.Count);
                    Assert.AreEqual(ByteString.Empty, parsed.MapInt32Bytes[0]);
                });
        }

        [Test]
        public void MapWithOnlyValue()
        {
            // Hand-craft the stream to contain a single entry with just a value.
            var memoryStream = new MemoryStream();
            var output = new CodedOutputStream(memoryStream);
            output.WriteTag(TestMap.MapInt32ForeignMessageFieldNumber, WireFormat.WireType.LengthDelimited);
            var nestedMessage = new ForeignMessage { C = 20 };
            // Size of the entry (tag, size written by WriteMessage, data written by WriteMessage)
            output.WriteLength(2 + nestedMessage.CalculateSize());
            output.WriteTag(2, WireFormat.WireType.LengthDelimited);
            output.WriteMessage(nestedMessage);
            output.Flush();

            MessageParsingHelpers.AssertReadingMessage(
                TestMap.Parser,
                memoryStream.ToArray(),
                parsed =>
                {
                    Assert.AreEqual(nestedMessage, parsed.MapInt32ForeignMessage[0]);
                });
        }

        [Test]
        public void MapWithOnlyKey_PrimitiveValue()
        {
            // Hand-craft the stream to contain a single entry with just a key.
            var memoryStream = new MemoryStream();
            var output = new CodedOutputStream(memoryStream);
            output.WriteTag(TestMap.MapInt32DoubleFieldNumber, WireFormat.WireType.LengthDelimited);
            int key = 10;
            output.WriteLength(1 + CodedOutputStream.ComputeInt32Size(key));
            output.WriteTag(1, WireFormat.WireType.Varint);
            output.WriteInt32(key);
            output.Flush();

            MessageParsingHelpers.AssertReadingMessage(
                TestMap.Parser,
                memoryStream.ToArray(),
                parsed =>
                {
                    Assert.AreEqual(0.0, parsed.MapInt32Double[key]);
                });
        }

        [Test]
        public void MapWithOnlyKey_MessageValue()
        {
            // Hand-craft the stream to contain a single entry with just a key.
            var memoryStream = new MemoryStream();
            var output = new CodedOutputStream(memoryStream);
            output.WriteTag(TestMap.MapInt32ForeignMessageFieldNumber, WireFormat.WireType.LengthDelimited);
            int key = 10;
            output.WriteLength(1 + CodedOutputStream.ComputeInt32Size(key));
            output.WriteTag(1, WireFormat.WireType.Varint);
            output.WriteInt32(key);
            output.Flush();

            MessageParsingHelpers.AssertReadingMessage(
                TestMap.Parser,
                memoryStream.ToArray(),
                parsed =>
                {
                    Assert.AreEqual(new ForeignMessage(), parsed.MapInt32ForeignMessage[key]);
                });
        }

        [Test]
        public void MapIgnoresExtraFieldsWithinEntryMessages()
        {
            // Hand-craft the stream to contain a single entry with three fields
            var memoryStream = new MemoryStream();
            var output = new CodedOutputStream(memoryStream);

            output.WriteTag(TestMap.MapInt32Int32FieldNumber, WireFormat.WireType.LengthDelimited);

            var key = 10; // Field 1
            var value = 20; // Field 2
            var extra = 30; // Field 3

            // Each field can be represented in a single byte, with a single byte tag.
            // Total message size: 6 bytes.
            output.WriteLength(6);
            output.WriteTag(1, WireFormat.WireType.Varint);
            output.WriteInt32(key);
            output.WriteTag(2, WireFormat.WireType.Varint);
            output.WriteInt32(value);
            output.WriteTag(3, WireFormat.WireType.Varint);
            output.WriteInt32(extra);
            output.Flush();

            MessageParsingHelpers.AssertReadingMessage(
                TestMap.Parser,
                memoryStream.ToArray(),
                parsed =>
                {
                    Assert.AreEqual(value, parsed.MapInt32Int32[key]);
                });
        }

        [Test]
        public void MapFieldOrderIsIrrelevant()
        {
            var memoryStream = new MemoryStream();
            var output = new CodedOutputStream(memoryStream);

            output.WriteTag(TestMap.MapInt32Int32FieldNumber, WireFormat.WireType.LengthDelimited);

            var key = 10;
            var value = 20;

            // Each field can be represented in a single byte, with a single byte tag.
            // Total message size: 4 bytes.
            output.WriteLength(4);
            output.WriteTag(2, WireFormat.WireType.Varint);
            output.WriteInt32(value);
            output.WriteTag(1, WireFormat.WireType.Varint);
            output.WriteInt32(key);
            output.Flush();

            MessageParsingHelpers.AssertReadingMessage(
                TestMap.Parser,
                memoryStream.ToArray(),
                parsed =>
                {
                    Assert.AreEqual(value, parsed.MapInt32Int32[key]);
                });
        }

        [Test]
        public void MapNonContiguousEntries()
        {
            var memoryStream = new MemoryStream();
            var output = new CodedOutputStream(memoryStream);

            // Message structure:
            // Entry for MapInt32Int32
            // Entry for MapStringString
            // Entry for MapInt32Int32

            // First entry
            var key1 = 10;
            var value1 = 20;
            output.WriteTag(TestMap.MapInt32Int32FieldNumber, WireFormat.WireType.LengthDelimited);
            output.WriteLength(4);
            output.WriteTag(1, WireFormat.WireType.Varint);
            output.WriteInt32(key1);
            output.WriteTag(2, WireFormat.WireType.Varint);
            output.WriteInt32(value1);

            // Second entry
            var key2 = "a";
            var value2 = "b";
            output.WriteTag(TestMap.MapStringStringFieldNumber, WireFormat.WireType.LengthDelimited);
            output.WriteLength(6); // 3 bytes per entry: tag, size, character
            output.WriteTag(1, WireFormat.WireType.LengthDelimited);
            output.WriteString(key2);
            output.WriteTag(2, WireFormat.WireType.LengthDelimited);
            output.WriteString(value2);

            // Third entry
            var key3 = 15;
            var value3 = 25;
            output.WriteTag(TestMap.MapInt32Int32FieldNumber, WireFormat.WireType.LengthDelimited);
            output.WriteLength(4);
            output.WriteTag(1, WireFormat.WireType.Varint);
            output.WriteInt32(key3);
            output.WriteTag(2, WireFormat.WireType.Varint);
            output.WriteInt32(value3);

            output.Flush();

            MessageParsingHelpers.AssertReadingMessage(
                TestMap.Parser,
                memoryStream.ToArray(),
                parsed =>
                {
                    var expected = new TestMap
                    {
                        MapInt32Int32 = { { key1, value1 }, { key3, value3 } },
                        MapStringString = { { key2, value2 } }
                    };
                    Assert.AreEqual(expected, parsed);
                });
        }

        [Test]
        public void DuplicateKeys_LastEntryWins()
        {
            var memoryStream = new MemoryStream();
            var output = new CodedOutputStream(memoryStream);

            var key = 10;
            var value1 = 20;
            var value2 = 30;

            // First entry
            output.WriteTag(TestMap.MapInt32Int32FieldNumber, WireFormat.WireType.LengthDelimited);
            output.WriteLength(4);
            output.WriteTag(1, WireFormat.WireType.Varint);
            output.WriteInt32(key);
            output.WriteTag(2, WireFormat.WireType.Varint);
            output.WriteInt32(value1);

            // Second entry - same key, different value
            output.WriteTag(TestMap.MapInt32Int32FieldNumber, WireFormat.WireType.LengthDelimited);
            output.WriteLength(4);
            output.WriteTag(1, WireFormat.WireType.Varint);
            output.WriteInt32(key);
            output.WriteTag(2, WireFormat.WireType.Varint);
            output.WriteInt32(value2);
            output.Flush();

            MessageParsingHelpers.AssertReadingMessage(
                TestMap.Parser,
                memoryStream.ToArray(),
                parsed =>
                {
                    Assert.AreEqual(value2, parsed.MapInt32Int32[key]);
                });
        }

        [Test]
        public void CloneSingleNonMessageValues()
        {
            var original = new TestAllTypes
            {
                SingleBool = true,
                SingleBytes = ByteString.CopyFrom(1, 2, 3, 4),
                SingleDouble = 23.5,
                SingleFixed32 = 23,
                SingleFixed64 = 1234567890123,
                SingleFloat = 12.25f,
                SingleInt32 = 100,
                SingleInt64 = 3210987654321,
                SingleNestedEnum = TestAllTypes.Types.NestedEnum.Foo,
                SingleSfixed32 = -123,
                SingleSfixed64 = -12345678901234,
                SingleSint32 = -456,
                SingleSint64 = -12345678901235,
                SingleString = "test",
                SingleUint32 = uint.MaxValue,
                SingleUint64 = ulong.MaxValue
            };
            var clone = original.Clone();
            Assert.AreNotSame(original, clone);
            Assert.AreEqual(original, clone);
            // Just as a single example
            clone.SingleInt32 = 150;
            Assert.AreNotEqual(original, clone);
        }

        [Test]
        public void CloneRepeatedNonMessageValues()
        {
            var original = new TestAllTypes
            {
                RepeatedBool = { true, false },
                RepeatedBytes = { ByteString.CopyFrom(1, 2, 3, 4), ByteString.CopyFrom(5, 6) },
                RepeatedDouble = { -12.25, 23.5 },
                RepeatedFixed32 = { uint.MaxValue, 23 },
                RepeatedFixed64 = { ulong.MaxValue, 1234567890123 },
                RepeatedFloat = { 100f, 12.25f },
                RepeatedInt32 = { 100, 200 },
                RepeatedInt64 = { 3210987654321, long.MaxValue },
                RepeatedNestedEnum = { TestAllTypes.Types.NestedEnum.Foo, TestAllTypes.Types.NestedEnum.Neg },
                RepeatedSfixed32 = { -123, 123 },
                RepeatedSfixed64 = { -12345678901234, 12345678901234 },
                RepeatedSint32 = { -456, 100 },
                RepeatedSint64 = { -12345678901235, 123 },
                RepeatedString = { "foo", "bar" },
                RepeatedUint32 = { uint.MaxValue, uint.MinValue },
                RepeatedUint64 = { ulong.MaxValue, uint.MinValue }
            };

            var clone = original.Clone();
            Assert.AreNotSame(original, clone);
            Assert.AreEqual(original, clone);
            // Just as a single example
            clone.RepeatedDouble.Add(25.5);
            Assert.AreNotEqual(original, clone);
        }

        [Test]
        public void CloneSingleMessageField()
        {
            var original = new TestAllTypes
            {
                SingleNestedMessage = new TestAllTypes.Types.NestedMessage { Bb = 20 }
            };

            var clone = original.Clone();
            Assert.AreNotSame(original, clone);
            Assert.AreNotSame(original.SingleNestedMessage, clone.SingleNestedMessage);
            Assert.AreEqual(original, clone);

            clone.SingleNestedMessage.Bb = 30;
            Assert.AreNotEqual(original, clone);
        }

        [Test]
        public void CloneRepeatedMessageField()
        {
            var original = new TestAllTypes
            {
                RepeatedNestedMessage = { new TestAllTypes.Types.NestedMessage { Bb = 20 } }
            };

            var clone = original.Clone();
            Assert.AreNotSame(original, clone);
            Assert.AreNotSame(original.RepeatedNestedMessage, clone.RepeatedNestedMessage);
            Assert.AreNotSame(original.RepeatedNestedMessage[0], clone.RepeatedNestedMessage[0]);
            Assert.AreEqual(original, clone);

            clone.RepeatedNestedMessage[0].Bb = 30;
            Assert.AreNotEqual(original, clone);
        }

        [Test]
        public void CloneOneofField()
        {
            var original = new TestAllTypes
            {
                OneofNestedMessage = new TestAllTypes.Types.NestedMessage { Bb = 20 }
            };

            var clone = original.Clone();
            Assert.AreNotSame(original, clone);
            Assert.AreEqual(original, clone);

            // We should have cloned the message
            original.OneofNestedMessage.Bb = 30;
            Assert.AreNotEqual(original, clone);
        }

        [Test]
        public void OneofProperties()
        {
            // Switch the oneof case between each of the different options, and check everything behaves
            // as expected in each case.
            var message = new TestAllTypes();
            Assert.AreEqual("", message.OneofString);
            Assert.AreEqual(0, message.OneofUint32);
            Assert.AreEqual(ByteString.Empty, message.OneofBytes);
            Assert.IsNull(message.OneofNestedMessage);
            Assert.AreEqual(TestAllTypes.OneofFieldOneofCase.None, message.OneofFieldCase);

            message.OneofString = "sample";
            Assert.AreEqual("sample", message.OneofString);
            Assert.AreEqual(0, message.OneofUint32);
            Assert.AreEqual(ByteString.Empty, message.OneofBytes);
            Assert.IsNull(message.OneofNestedMessage);
            Assert.AreEqual(TestAllTypes.OneofFieldOneofCase.OneofString, message.OneofFieldCase);

            var bytes = ByteString.CopyFrom(1, 2, 3);
            message.OneofBytes = bytes;
            Assert.AreEqual("", message.OneofString);
            Assert.AreEqual(0, message.OneofUint32);
            Assert.AreEqual(bytes, message.OneofBytes);
            Assert.IsNull(message.OneofNestedMessage);
            Assert.AreEqual(TestAllTypes.OneofFieldOneofCase.OneofBytes, message.OneofFieldCase);

            message.OneofUint32 = 20;
            Assert.AreEqual("", message.OneofString);
            Assert.AreEqual(20, message.OneofUint32);
            Assert.AreEqual(ByteString.Empty, message.OneofBytes);
            Assert.IsNull(message.OneofNestedMessage);
            Assert.AreEqual(TestAllTypes.OneofFieldOneofCase.OneofUint32, message.OneofFieldCase);

            var nestedMessage = new TestAllTypes.Types.NestedMessage { Bb = 25 };
            message.OneofNestedMessage = nestedMessage;
            Assert.AreEqual("", message.OneofString);
            Assert.AreEqual(0, message.OneofUint32);
            Assert.AreEqual(ByteString.Empty, message.OneofBytes);
            Assert.AreEqual(nestedMessage, message.OneofNestedMessage);
            Assert.AreEqual(TestAllTypes.OneofFieldOneofCase.OneofNestedMessage, message.OneofFieldCase);

            message.ClearOneofField();
            Assert.AreEqual("", message.OneofString);
            Assert.AreEqual(0, message.OneofUint32);
            Assert.AreEqual(ByteString.Empty, message.OneofBytes);
            Assert.IsNull(message.OneofNestedMessage);
            Assert.AreEqual(TestAllTypes.OneofFieldOneofCase.None, message.OneofFieldCase);
        }

        [Test]
        public void Oneof_DefaultValuesNotEqual()
        {
            var message1 = new TestAllTypes { OneofString = "" };
            var message2 = new TestAllTypes { OneofUint32 = 0 };
            Assert.AreEqual(TestAllTypes.OneofFieldOneofCase.OneofString, message1.OneofFieldCase);
            Assert.AreEqual(TestAllTypes.OneofFieldOneofCase.OneofUint32, message2.OneofFieldCase);
            Assert.AreNotEqual(message1, message2);
        }

        [Test]
        public void OneofSerialization_NonDefaultValue()
        {
            var message = new TestAllTypes
            {
                OneofString = "this would take a bit of space",
                OneofUint32 = 10
            };
            var bytes = message.ToByteArray();
            Assert.AreEqual(3, bytes.Length); // 2 bytes for the tag + 1 for the value - no string!

            MessageParsingHelpers.AssertWritingMessage(message);

            MessageParsingHelpers.AssertRoundtrip(TestAllTypes.Parser, message, parsedMessage =>
            {
                Assert.AreEqual(TestAllTypes.OneofFieldOneofCase.OneofUint32, parsedMessage.OneofFieldCase);
            });
        }

        [Test]
        public void OneofSerialization_DefaultValue()
        {
            var message = new TestAllTypes
            {
                OneofString = "this would take a bit of space",
                OneofUint32 = 0 // This is the default value for UInt32; normally wouldn't be serialized
            };
            var bytes = message.ToByteArray();
            Assert.AreEqual(3, bytes.Length); // 2 bytes for the tag + 1 for the value - it's still serialized

            MessageParsingHelpers.AssertWritingMessage(message);

            MessageParsingHelpers.AssertRoundtrip(TestAllTypes.Parser, message, parsedMessage =>
            {
                Assert.AreEqual(TestAllTypes.OneofFieldOneofCase.OneofUint32, parsedMessage.OneofFieldCase);
            });
        }

        [Test]
        public void MapStringString_DeterministicTrue_ThenBytesIdentical()
        {
            // Define three strings consisting of different versions of the letter I.
            // LATIN CAPITAL LETTER I (U+0049)
            string capitalLetterI = "I";
            // LATIN SMALL LETTER I (U+0069)
            string smallLetterI   = "i";
            // LATIN SMALL LETTER DOTLESS I (U+0131)
            string smallLetterDotlessI = "\u0131";
            var testMap1 = new TestMap();

            testMap1.MapStringString.Add(smallLetterDotlessI, "value_"+smallLetterDotlessI);
            testMap1.MapStringString.Add(smallLetterI, "value_"+smallLetterI);
            testMap1.MapStringString.Add(capitalLetterI, "content_"+capitalLetterI);
            var bytes1 = SerializeTestMap(testMap1, true);

            var testMap2 = new TestMap();
            testMap2.MapStringString.Add(capitalLetterI, "content_"+capitalLetterI);
            testMap2.MapStringString.Add(smallLetterI, "value_"+smallLetterI);
            testMap2.MapStringString.Add(smallLetterDotlessI, "value_"+smallLetterDotlessI);

            var bytes2 = SerializeTestMap(testMap2, true);
            var parsedBytes2 = TestMap.Parser.ParseFrom(bytes2);
            var parsedBytes1 = TestMap.Parser.ParseFrom(bytes1);
            Assert.IsTrue(bytes1.SequenceEqual(bytes2));
        }

        [Test]
        public void MapInt32Bytes_DeterministicTrue_ThenBytesIdentical()
        {
            var testMap1 = new TestMap();
            testMap1.MapInt32Bytes.Add(1, ByteString.CopyFromUtf8("test1"));
            testMap1.MapInt32Bytes.Add(2, ByteString.CopyFromUtf8("test2"));
            var bytes1 = SerializeTestMap(testMap1, true);

            var testMap2 = new TestMap();
            testMap2.MapInt32Bytes.Add(2, ByteString.CopyFromUtf8("test2"));
            testMap2.MapInt32Bytes.Add(1, ByteString.CopyFromUtf8("test1"));
            var bytes2 = SerializeTestMap(testMap2, true);

            Assert.IsTrue(bytes1.SequenceEqual(bytes2));
        }

        [Test]
        public void MapInt32Bytes_DeterministicFalse_ThenBytesDifferent()
        {
            var testMap1 = new TestMap();
            testMap1.MapInt32Bytes.Add(1, ByteString.CopyFromUtf8("test1"));
            testMap1.MapInt32Bytes.Add(2, ByteString.CopyFromUtf8("test2"));
            var bytes1 = SerializeTestMap(testMap1, false);

            var testMap2 = new TestMap();
            testMap2.MapInt32Bytes.Add(2, ByteString.CopyFromUtf8("test2"));
            testMap2.MapInt32Bytes.Add(1, ByteString.CopyFromUtf8("test1"));
            var bytes2 = SerializeTestMap(testMap2, false);

            Assert.IsFalse(bytes1.SequenceEqual(bytes2));
        }

        private byte[] SerializeTestMap(TestMap testMap, bool deterministic)
        {
            using var memoryStream = new MemoryStream();
            var codedOutputStream = new CodedOutputStream(memoryStream);
            codedOutputStream.Deterministic = deterministic;

            testMap.WriteTo(codedOutputStream);
            codedOutputStream.Flush();

            memoryStream.Seek(0, SeekOrigin.Begin);
            return memoryStream.ToArray();
        }

        [Test]
        public void DiscardUnknownFields_RealDataStillRead()
        {
            var message = SampleMessages.CreateFullTestAllTypes();
            var stream = new MemoryStream();
            var output = new CodedOutputStream(stream);
            var unusedFieldNumber = 23456;
            Assert.IsFalse(TestAllTypes.Descriptor.Fields.InDeclarationOrder().Select(x => x.FieldNumber).Contains(unusedFieldNumber));
            output.WriteTag(unusedFieldNumber, WireFormat.WireType.LengthDelimited);
            output.WriteString("ignore me");
            message.WriteTo(output);
            output.Flush();

            MessageParsingHelpers.AssertReadingMessage(
                TestAllTypes.Parser,
                stream.ToArray(),
                parsed =>
                {
                    // TODO: Add test back when DiscardUnknownFields API is supported.
                    // Assert.AreEqual(message, parsed);
                });
        }

        [Test]
        public void DiscardUnknownFields_AllTypes()
        {
            // Simple way of ensuring we can skip all kinds of fields.
            var data = SampleMessages.CreateFullTestAllTypes().ToByteArray();
            var empty = Empty.Parser.ParseFrom(data);

            MessageParsingHelpers.AssertReadingMessage(
                Empty.Parser,
                data,
                parsed =>
                {
                    // TODO: Add test back when DiscardUnknownFields API is supported.
                    // Assert.AreNotEqual(new Empty(), empty);
                });
        }

        // This was originally seen as a conformance test failure.
        [Test]
        public void TruncatedMessageFieldThrows()
        {
            // 130, 3 is the message tag
            // 1 is the data length - but there's no data.
            var data = new byte[] { 130, 3, 1 };
            MessageParsingHelpers.AssertReadingMessageThrows<TestAllTypes, InvalidProtocolBufferException>(TestAllTypes.Parser, data);
        }

        /// <summary>
        /// Demonstrates current behaviour with an extraneous end group tag - see issue 688
        /// for details; we may want to change this.
        /// </summary>
        [Test]
        public void ExtraEndGroupThrows()
        {
            var stream = new MemoryStream();
            var output = new CodedOutputStream(stream);

            output.WriteTag(TestAllTypes.SingleFixed32FieldNumber, WireFormat.WireType.Fixed32);
            output.WriteFixed32(123);
            output.WriteTag(100, WireFormat.WireType.EndGroup);

            output.Flush();

            stream.Position = 0;
            MessageParsingHelpers.AssertReadingMessageThrows<TestAllTypes, InvalidProtocolBufferException>(TestAllTypes.Parser, stream.ToArray());
        }

        [Test]
        public void CustomDiagnosticMessage_DirectToStringCall()
        {
            var message = new ForeignMessage { C = 31 };
            Assert.AreEqual("{ \"c\": 31, \"@cInHex\": \"1f\" }", message.ToString());
            Assert.AreEqual("{ \"c\": 31 }", JsonFormatter.Default.Format(message));
        }

        [Test]
        public void CustomDiagnosticMessage_Nested()
        {
            var message = new TestAllTypes { SingleForeignMessage = new ForeignMessage { C = 16 } };
            Assert.AreEqual("{ \"singleForeignMessage\": { \"c\": 16, \"@cInHex\": \"10\" } }", message.ToString());
            Assert.AreEqual("{ \"singleForeignMessage\": { \"c\": 16 } }", JsonFormatter.Default.Format(message));
        }

        [Test]
        public void CustomDiagnosticMessage_DirectToTextWriterCall()
        {
            var message = new ForeignMessage { C = 31 };
            var writer = new StringWriter();
            JsonFormatter.Default.Format(message, writer);
            Assert.AreEqual("{ \"c\": 31 }", writer.ToString());
        }

        [Test]
        public void NaNComparisons()
        {
            var message1 = new TestAllTypes { SingleDouble = SampleNaNs.Regular };
            var message2 = new TestAllTypes { SingleDouble = SampleNaNs.PayloadFlipped };
            var message3 = new TestAllTypes { SingleDouble = SampleNaNs.Regular };

            EqualityTester.AssertInequality(message1, message2);
            EqualityTester.AssertEquality(message1, message3);
        }

        [Test]
        [TestCase(false)]
        [TestCase(true)]
        public void MapFieldMerging(bool direct)
        {
            var message1 = new TestMap
            {
                MapStringString =
                {
                    { "x1", "y1" },
                    { "common", "message1" }
                }
            };
            var message2 = new TestMap
            {
                MapStringString =
                {
                    { "x2", "y2" },
                    { "common", "message2" }
                }
            };
            if (direct)
            {
                message1.MergeFrom(message2);
            }
            else
            {
                message1.MergeFrom(message2.ToByteArray());
            }

            var expected = new MapField<string, string>
            {
                { "x1", "y1" },
                { "x2", "y2" },
                { "common", "message2" }
            };
            Assert.AreEqual(expected, message1.MapStringString);
        }
    }
}
