#region Copyright notice and license
// Protocol Buffers - Google's data interchange format
// Copyright 2015 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd
#endregion

using System;
using System.Collections;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using Google.Protobuf.TestProtos;
using Google.Protobuf.WellKnownTypes;
using NUnit.Framework;

namespace Google.Protobuf.Collections
{
    public class RepeatedFieldTest
    {
        [Test]
        public void NullValuesRejected()
        {
            var list = new RepeatedField<string>();
            Assert.Throws<ArgumentNullException>(() => list.Add((string)null));
            Assert.Throws<ArgumentNullException>(() => list.Add((IEnumerable<string>)null));
            Assert.Throws<ArgumentNullException>(() => list.Add((RepeatedField<string>)null));
            Assert.Throws<ArgumentNullException>(() => list.Contains(null));
            Assert.Throws<ArgumentNullException>(() => list.IndexOf(null));
        }

        [Test]
        public void Add_SingleItem()
        {
            var list = new RepeatedField<string> { "foo" };
            Assert.AreEqual(1, list.Count);
            Assert.AreEqual("foo", list[0]);
        }

        [Test]
        public void Add_Sequence()
        {
            var list = new RepeatedField<string> { new[] { "foo", "bar" } };
            Assert.AreEqual(2, list.Count);
            Assert.AreEqual("foo", list[0]);
            Assert.AreEqual("bar", list[1]);
        }

        [Test]
        public void AddRange_SlowPath()
        {
            var list = new RepeatedField<string>();
            list.AddRange(new[] { "foo", "bar" }.Select(x => x));
            Assert.AreEqual(2, list.Count);
            Assert.AreEqual("foo", list[0]);
            Assert.AreEqual("bar", list[1]);
        }

        [Test]
        public void AddRange_SlowPath_NullsProhibited_ReferenceType()
        {
            var list = new RepeatedField<string>();
            // It's okay for this to throw ArgumentNullException if necessary.
            // It's not ideal, but not awful.
            Assert.Catch<ArgumentException>(() => list.AddRange(new[] { "foo", null }.Select(x => x)));
        }

        [Test]
        public void AddRange_SlowPath_NullsProhibited_NullableValueType()
        {
            var list = new RepeatedField<int?>();
            // It's okay for this to throw ArgumentNullException if necessary.
            // It's not ideal, but not awful.
            Assert.Catch<ArgumentException>(() => list.AddRange(new[] { 20, (int?)null }.Select(x => x)));
        }

        [Test]
        public void AddRange_Optimized_NonNullableValueType()
        {
            var list = new RepeatedField<int>();
            list.AddRange(new List<int> { 20, 30 });
            Assert.AreEqual(2, list.Count);
            Assert.AreEqual(20, list[0]);
            Assert.AreEqual(30, list[1]);
        }

        [Test]
        public void AddRange_Optimized_ReferenceType()
        {
            var list = new RepeatedField<string>();
            list.AddRange(new List<string> { "foo", "bar" });
            Assert.AreEqual(2, list.Count);
            Assert.AreEqual("foo", list[0]);
            Assert.AreEqual("bar", list[1]);
        }

        [Test]
        public void AddRange_Optimized_NullableValueType()
        {
            var list = new RepeatedField<int?>();
            list.AddRange(new List<int?> { 20, 30 });
            Assert.AreEqual(2, list.Count);
            Assert.AreEqual((int?) 20, list[0]);
            Assert.AreEqual((int?) 30, list[1]);
        }

        [Test]
        public void AddRange_Optimized_NullsProhibited_ReferenceType()
        {
            // We don't just trust that a collection with a nullable element type doesn't contain nulls
            var list = new RepeatedField<string>();
            // It's okay for this to throw ArgumentNullException if necessary.
            // It's not ideal, but not awful.
            Assert.Catch<ArgumentException>(() => list.AddRange(new List<string> { "foo", null }));
        }

        [Test]
        public void AddRange_Optimized_NullsProhibited_NullableValueType()
        {
            // We don't just trust that a collection with a nullable element type doesn't contain nulls
            var list = new RepeatedField<int?>();
            // It's okay for this to throw ArgumentNullException if necessary.
            // It's not ideal, but not awful.
            Assert.Catch<ArgumentException>(() => list.AddRange(new List<int?> { 20, null }));
        }

        [Test]
        public void AddRange_AlreadyNotEmpty()
        {
            var list = new RepeatedField<int> { 1, 2, 3 };
            list.AddRange(new List<int> { 4, 5, 6 });
            CollectionAssert.AreEqual(new[] { 1, 2, 3, 4, 5, 6 }, list);
        }

        [Test]
        public void AddRange_RepeatedField()
        {
            var list = new RepeatedField<string> { "original" };
            list.AddRange(new RepeatedField<string> { "foo", "bar" });
            Assert.AreEqual(3, list.Count);
            Assert.AreEqual("original", list[0]);
            Assert.AreEqual("foo", list[1]);
            Assert.AreEqual("bar", list[2]);
        }

        [Test]
        public void RemoveAt_Valid()
        {
            var list = new RepeatedField<string> { "first", "second", "third" };
            list.RemoveAt(1);
            CollectionAssert.AreEqual(new[] { "first", "third" }, list);
            // Just check that these don't throw...
            list.RemoveAt(list.Count - 1); // Now the count will be 1...
            list.RemoveAt(0);
            Assert.AreEqual(0, list.Count);
        }

        [Test]
        public void RemoveAt_Invalid()
        {
            var list = new RepeatedField<string> { "first", "second", "third" };
            Assert.Throws<ArgumentOutOfRangeException>(() => list.RemoveAt(-1));
            Assert.Throws<ArgumentOutOfRangeException>(() => list.RemoveAt(3));
        }

        [Test]
        public void Insert_Valid()
        {
            var list = new RepeatedField<string> { "first", "second" };
            list.Insert(1, "middle");
            CollectionAssert.AreEqual(new[] { "first", "middle", "second" }, list);
            list.Insert(3, "end");
            CollectionAssert.AreEqual(new[] { "first", "middle", "second", "end" }, list);
            list.Insert(0, "start");
            CollectionAssert.AreEqual(new[] { "start", "first", "middle", "second", "end" }, list);
        }

        [Test]
        public void Insert_Invalid()
        {
            var list = new RepeatedField<string> { "first", "second" };
            Assert.Throws<ArgumentOutOfRangeException>(() => list.Insert(-1, "foo"));
            Assert.Throws<ArgumentOutOfRangeException>(() => list.Insert(3, "foo"));
            Assert.Throws<ArgumentNullException>(() => list.Insert(0, null));
        }

        [Test]
        public void Equals_RepeatedField()
        {
            var list = new RepeatedField<string> { "first", "second" };
            Assert.IsFalse(list.Equals((RepeatedField<string>) null));
            Assert.IsTrue(list.Equals(list));
            Assert.IsFalse(list.Equals(new RepeatedField<string> { "first", "third" }));
            Assert.IsFalse(list.Equals(new RepeatedField<string> { "first" }));
            Assert.IsTrue(list.Equals(new RepeatedField<string> { "first", "second" }));
        }

        [Test]
        public void Equals_Object()
        {
            var list = new RepeatedField<string> { "first", "second" };
            Assert.IsFalse(list.Equals((object) null));
            Assert.IsTrue(list.Equals((object) list));
            Assert.IsFalse(list.Equals((object) new RepeatedField<string> { "first", "third" }));
            Assert.IsFalse(list.Equals((object) new RepeatedField<string> { "first" }));
            Assert.IsTrue(list.Equals((object) new RepeatedField<string> { "first", "second" }));
            Assert.IsFalse(list.Equals(new object()));
        }

        [Test]
        public void GetEnumerator_GenericInterface()
        {
            IEnumerable<string> list = new RepeatedField<string> { "first", "second" };
            // Select gets rid of the optimizations in ToList...
            CollectionAssert.AreEqual(new[] { "first", "second" }, list.Select(x => x).ToList());
        }

        [Test]
        public void GetEnumerator_NonGenericInterface()
        {
            IEnumerable list = new RepeatedField<string> { "first", "second" };
            CollectionAssert.AreEqual(new[] { "first", "second" }, list.Cast<object>().ToList());
        }

        [Test]
        public void CopyTo()
        {
            var list = new RepeatedField<string> { "first", "second" };
            string[] stringArray = new string[4];
            list.CopyTo(stringArray, 1);
            CollectionAssert.AreEqual(new[] { null, "first", "second", null }, stringArray);
        }

        [Test]
        public void Indexer_Get()
        {
            var list = new RepeatedField<string> { "first", "second" };
            Assert.AreEqual("first", list[0]);
            Assert.AreEqual("second", list[1]);
            Assert.Throws<ArgumentOutOfRangeException>(() => list[-1].GetHashCode());
            Assert.Throws<ArgumentOutOfRangeException>(() => list[2].GetHashCode());
        }

        [Test]
        public void Indexer_Set()
        {
            var list = new RepeatedField<string> { "first", "second" };
            list[0] = "changed";
            Assert.AreEqual("changed", list[0]);
            Assert.Throws<ArgumentNullException>(() => list[0] = null);
            Assert.Throws<ArgumentOutOfRangeException>(() => list[-1] = "bad");
            Assert.Throws<ArgumentOutOfRangeException>(() => list[2] = "bad");
        }

        [Test]
        public void Clone_ReturnsMutable()
        {
            var list = new RepeatedField<int> { 0 };
            var clone = list.Clone();
            clone[0] = 1;
        }

        [Test]
        public void Enumerator()
        {
            var list = new RepeatedField<string> { "first", "second" };
            using var enumerator = list.GetEnumerator();
            Assert.IsTrue(enumerator.MoveNext());
            Assert.AreEqual("first", enumerator.Current);
            Assert.IsTrue(enumerator.MoveNext());
            Assert.AreEqual("second", enumerator.Current);
            Assert.IsFalse(enumerator.MoveNext());
            Assert.IsFalse(enumerator.MoveNext());
        }

        [Test]
        public void AddEntriesFrom_PackedInt32()
        {
            uint packedTag = WireFormat.MakeTag(10, WireFormat.WireType.LengthDelimited);
            var stream = new MemoryStream();
            var output = new CodedOutputStream(stream);
            var length = CodedOutputStream.ComputeInt32Size(10)
                + CodedOutputStream.ComputeInt32Size(999)
                + CodedOutputStream.ComputeInt32Size(-1000);
            output.WriteTag(packedTag);
            output.WriteRawVarint32((uint) length);
            output.WriteInt32(10);
            output.WriteInt32(999);
            output.WriteInt32(-1000);
            output.Flush();
            stream.Position = 0;

            // Deliberately "expecting" a non-packed tag, but we detect that the data is
            // actually packed.
            uint nonPackedTag = WireFormat.MakeTag(10, WireFormat.WireType.LengthDelimited);
            var field = new RepeatedField<int>();
            var input = new CodedInputStream(stream);
            input.AssertNextTag(packedTag);
            field.AddEntriesFrom(input, FieldCodec.ForInt32(nonPackedTag));
            CollectionAssert.AreEqual(new[] { 10, 999, -1000 }, field);
            Assert.IsTrue(input.IsAtEnd);
        }

        [Test]
        public void AddEntriesFrom_NonPackedInt32()
        {
            uint nonPackedTag = WireFormat.MakeTag(10, WireFormat.WireType.Varint);
            var stream = new MemoryStream();
            var output = new CodedOutputStream(stream);
            output.WriteTag(nonPackedTag);
            output.WriteInt32(10);
            output.WriteTag(nonPackedTag);
            output.WriteInt32(999);
            output.WriteTag(nonPackedTag);
            output.WriteInt32(-1000); // Just for variety...
            output.Flush();
            stream.Position = 0;

            // Deliberately "expecting" a packed tag, but we detect that the data is
            // actually not packed.
            uint packedTag = WireFormat.MakeTag(10, WireFormat.WireType.LengthDelimited);
            var field = new RepeatedField<int>();
            var input = new CodedInputStream(stream);
            input.AssertNextTag(nonPackedTag);
            field.AddEntriesFrom(input, FieldCodec.ForInt32(packedTag));
            CollectionAssert.AreEqual(new[] { 10, 999, -1000 }, field);
            Assert.IsTrue(input.IsAtEnd);
        }

        [Test]
        public void AddEntriesFrom_String()
        {
            uint tag = WireFormat.MakeTag(10, WireFormat.WireType.LengthDelimited);
            var stream = new MemoryStream();
            var output = new CodedOutputStream(stream);
            output.WriteTag(tag);
            output.WriteString("Foo");
            output.WriteTag(tag);
            output.WriteString("");
            output.WriteTag(tag);
            output.WriteString("Bar");
            output.Flush();
            stream.Position = 0;

            var field = new RepeatedField<string>();
            var input = new CodedInputStream(stream);
            input.AssertNextTag(tag);
            field.AddEntriesFrom(input, FieldCodec.ForString(tag));
            CollectionAssert.AreEqual(new[] { "Foo", "", "Bar" }, field);
            Assert.IsTrue(input.IsAtEnd);
        }

        [Test]
        public void AddEntriesFrom_Message()
        {
            var message1 = new ForeignMessage { C = 2000 };
            var message2 = new ForeignMessage { C = -250 };

            uint tag = WireFormat.MakeTag(10, WireFormat.WireType.LengthDelimited);
            var stream = new MemoryStream();
            var output = new CodedOutputStream(stream);
            output.WriteTag(tag);
            output.WriteMessage(message1);
            output.WriteTag(tag);
            output.WriteMessage(message2);
            output.Flush();
            stream.Position = 0;

            var field = new RepeatedField<ForeignMessage>();
            var input = new CodedInputStream(stream);
            input.AssertNextTag(tag);
            field.AddEntriesFrom(input, FieldCodec.ForMessage(tag, ForeignMessage.Parser));
            CollectionAssert.AreEqual(new[] { message1, message2}, field);
            Assert.IsTrue(input.IsAtEnd);
        }

        [Test]
        public void WriteTo_PackedInt32()
        {
            uint tag = WireFormat.MakeTag(10, WireFormat.WireType.LengthDelimited);
            var field = new RepeatedField<int> { 10, 1000, 1000000 };
            var stream = new MemoryStream();
            var output = new CodedOutputStream(stream);
            field.WriteTo(output, FieldCodec.ForInt32(tag));
            output.Flush();
            stream.Position = 0;

            var input = new CodedInputStream(stream);
            input.AssertNextTag(tag);
            var length = input.ReadLength();
            Assert.AreEqual(10, input.ReadInt32());
            Assert.AreEqual(1000, input.ReadInt32());
            Assert.AreEqual(1000000, input.ReadInt32());
            Assert.IsTrue(input.IsAtEnd);
            Assert.AreEqual(1 + CodedOutputStream.ComputeLengthSize(length) + length, stream.Length);
        }

        [Test]
        public void WriteTo_NonPackedInt32()
        {
            uint tag = WireFormat.MakeTag(10, WireFormat.WireType.Varint);
            var field = new RepeatedField<int> { 10, 1000, 1000000};
            var stream = new MemoryStream();
            var output = new CodedOutputStream(stream);
            field.WriteTo(output, FieldCodec.ForInt32(tag));
            output.Flush();
            stream.Position = 0;

            var input = new CodedInputStream(stream);
            input.AssertNextTag(tag);
            Assert.AreEqual(10, input.ReadInt32());
            input.AssertNextTag(tag);
            Assert.AreEqual(1000, input.ReadInt32());
            input.AssertNextTag(tag);
            Assert.AreEqual(1000000, input.ReadInt32());
            Assert.IsTrue(input.IsAtEnd);
        }

        [Test]
        public void WriteTo_String()
        {
            uint tag = WireFormat.MakeTag(10, WireFormat.WireType.LengthDelimited);
            var field = new RepeatedField<string> { "Foo", "", "Bar" };
            var stream = new MemoryStream();
            var output = new CodedOutputStream(stream);
            field.WriteTo(output, FieldCodec.ForString(tag));
            output.Flush();
            stream.Position = 0;

            var input = new CodedInputStream(stream);
            input.AssertNextTag(tag);
            Assert.AreEqual("Foo", input.ReadString());
            input.AssertNextTag(tag);
            Assert.AreEqual("", input.ReadString());
            input.AssertNextTag(tag);
            Assert.AreEqual("Bar", input.ReadString());
            Assert.IsTrue(input.IsAtEnd);
        }

        [Test]
        public void WriteTo_Message()
        {
            var message1 = new ForeignMessage { C = 20 };
            var message2 = new ForeignMessage { C = 25 };
            uint tag = WireFormat.MakeTag(10, WireFormat.WireType.LengthDelimited);
            var field = new RepeatedField<ForeignMessage> { message1, message2 };
            var stream = new MemoryStream();
            var output = new CodedOutputStream(stream);
            field.WriteTo(output, FieldCodec.ForMessage(tag, ForeignMessage.Parser));
            output.Flush();
            stream.Position = 0;

            var input = new CodedInputStream(stream);
            input.AssertNextTag(tag);
            Assert.AreEqual(message1, input.ReadMessage(ForeignMessage.Parser));
            input.AssertNextTag(tag);
            Assert.AreEqual(message2, input.ReadMessage(ForeignMessage.Parser));
            Assert.IsTrue(input.IsAtEnd);
        }

        [Test]
        public void CalculateSize_VariableSizeNonPacked()
        {
            var list = new RepeatedField<int> { 1, 500, 1 };
            var tag = WireFormat.MakeTag(1, WireFormat.WireType.Varint);
            // 2 bytes for the first entry, 3 bytes for the second, 2 bytes for the third
            Assert.AreEqual(7, list.CalculateSize(FieldCodec.ForInt32(tag)));
        }

        [Test]
        public void CalculateSize_FixedSizeNonPacked()
        {
            var list = new RepeatedField<int> { 1, 500, 1 };
            var tag = WireFormat.MakeTag(1, WireFormat.WireType.Fixed32);
            // 5 bytes for the each entry
            Assert.AreEqual(15, list.CalculateSize(FieldCodec.ForSFixed32(tag)));
        }

        [Test]
        public void CalculateSize_VariableSizePacked()
        {
            var list = new RepeatedField<int> { 1, 500, 1};
            var tag = WireFormat.MakeTag(1, WireFormat.WireType.LengthDelimited);
            // 1 byte for the tag, 1 byte for the length,
            // 1 byte for the first entry, 2 bytes for the second, 1 byte for the third
            Assert.AreEqual(6, list.CalculateSize(FieldCodec.ForInt32(tag)));
        }

        [Test]
        public void CalculateSize_FixedSizePacked()
        {
            var list = new RepeatedField<int> { 1, 500, 1 };
            var tag = WireFormat.MakeTag(1, WireFormat.WireType.LengthDelimited);
            // 1 byte for the tag, 1 byte for the length, 4 bytes per entry
            Assert.AreEqual(14, list.CalculateSize(FieldCodec.ForSFixed32(tag)));
        }

        [Test]
        public void TestNegativeEnumArray()
        {
            int arraySize = 1 + 1 + (11 * 5);
            int msgSize = arraySize;
            byte[] bytes = new byte[msgSize];
            CodedOutputStream output = new CodedOutputStream(bytes);
            uint tag = WireFormat.MakeTag(8, WireFormat.WireType.Varint);
            for (int i = 0; i >= -5; i--)
            {
                output.WriteTag(tag);
                output.WriteEnum(i);
            }

            Assert.AreEqual(0, output.SpaceLeft);

            CodedInputStream input = new CodedInputStream(bytes);
            tag = input.ReadTag();

            RepeatedField<SampleEnum> values = new RepeatedField<SampleEnum>();
            values.AddEntriesFrom(input, FieldCodec.ForEnum(tag, x => (int)x, x => (SampleEnum)x));

            Assert.AreEqual(6, values.Count);
            Assert.AreEqual(SampleEnum.None, values[0]);
            Assert.AreEqual(((SampleEnum)(-1)), values[1]);
            Assert.AreEqual(SampleEnum.NegativeValue, values[2]);
            Assert.AreEqual(((SampleEnum)(-3)), values[3]);
            Assert.AreEqual(((SampleEnum)(-4)), values[4]);
            Assert.AreEqual(((SampleEnum)(-5)), values[5]);
        }


        [Test]
        public void TestNegativeEnumPackedArray()
        {
            int arraySize = 1 + (10 * 5);
            int msgSize = 1 + 1 + arraySize;
            byte[] bytes = new byte[msgSize];
            CodedOutputStream output = new CodedOutputStream(bytes);
            // Length-delimited to show we want the packed representation
            uint tag = WireFormat.MakeTag(8, WireFormat.WireType.LengthDelimited);
            output.WriteTag(tag);
            int size = 0;
            for (int i = 0; i >= -5; i--)
            {
                size += CodedOutputStream.ComputeEnumSize(i);
            }
            output.WriteRawVarint32((uint)size);
            for (int i = 0; i >= -5; i--)
            {
                output.WriteEnum(i);
            }
            Assert.AreEqual(0, output.SpaceLeft);

            CodedInputStream input = new CodedInputStream(bytes);
            tag = input.ReadTag();

            RepeatedField<SampleEnum> values = new RepeatedField<SampleEnum>();
            values.AddEntriesFrom(input, FieldCodec.ForEnum(tag, x => (int)x, x => (SampleEnum)x));

            Assert.AreEqual(6, values.Count);
            Assert.AreEqual(SampleEnum.None, values[0]);
            Assert.AreEqual(((SampleEnum)(-1)), values[1]);
            Assert.AreEqual(SampleEnum.NegativeValue, values[2]);
            Assert.AreEqual(((SampleEnum)(-3)), values[3]);
            Assert.AreEqual(((SampleEnum)(-4)), values[4]);
            Assert.AreEqual(((SampleEnum)(-5)), values[5]);
        }

        [Test]
        public void TestPackedRepeatedFieldCollectionNonDivisibleLength()
        {
            uint tag = WireFormat.MakeTag(10, WireFormat.WireType.LengthDelimited);
            var codec = FieldCodec.ForFixed32(tag);
            var stream = new MemoryStream();
            var output = new CodedOutputStream(stream);
            output.WriteTag(tag);
            output.WriteString("A long string");
            output.WriteTag(codec.Tag);
            output.WriteRawVarint32((uint)codec.FixedSize - 1); // Length not divisible by FixedSize
            output.WriteFixed32(uint.MaxValue);
            output.Flush();
            stream.Position = 0;

            var input = new CodedInputStream(stream);
            input.ReadTag();
            input.ReadString();
            input.ReadTag();
            var field = new RepeatedField<uint>();
            Assert.Throws<InvalidProtocolBufferException>(() => field.AddEntriesFrom(input, codec));

            // Collection was not pre-initialized
            Assert.AreEqual(0, field.Count);
        }

        [Test]
        public void TestPackedRepeatedFieldCollectionNotAllocatedWhenLengthExceedsBuffer()
        {
            uint tag = WireFormat.MakeTag(10, WireFormat.WireType.LengthDelimited);
            var codec = FieldCodec.ForFixed32(tag);
            var stream = new MemoryStream();
            var output = new CodedOutputStream(stream);
            output.WriteTag(tag);
            output.WriteString("A long string");
            output.WriteTag(codec.Tag);
            output.WriteRawVarint32((uint)codec.FixedSize);
            // Note that there is no content for the packed field.
            // The field length exceeds the remaining length of content.
            output.Flush();
            stream.Position = 0;

            var input = new CodedInputStream(stream);
            input.ReadTag();
            input.ReadString();
            input.ReadTag();
            var field = new RepeatedField<uint>();
            Assert.Throws<InvalidProtocolBufferException>(() => field.AddEntriesFrom(input, codec));

            // Collection was not pre-initialized
            Assert.AreEqual(0, field.Count);
        }

        [Test]
        public void TestPackedRepeatedFieldCollectionNotAllocatedWhenLengthExceedsRemainingData()
        {
            uint tag = WireFormat.MakeTag(10, WireFormat.WireType.LengthDelimited);
            var codec = FieldCodec.ForFixed32(tag);
            var stream = new MemoryStream();
            var output = new CodedOutputStream(stream);
            output.WriteTag(tag);
            output.WriteString("A long string");
            output.WriteTag(codec.Tag);
            output.WriteRawVarint32((uint)codec.FixedSize);
            // Note that there is no content for the packed field.
            // The field length exceeds the remaining length of the buffer.
            output.Flush();
            stream.Position = 0;

            var sequence = ReadOnlySequenceFactory.CreateWithContent(stream.ToArray());
            ParseContext.Initialize(sequence, out ParseContext ctx);

            ctx.ReadTag();
            ctx.ReadString();
            ctx.ReadTag();
            var field = new RepeatedField<uint>();
            try
            {
                field.AddEntriesFrom(ref ctx, codec);
                Assert.Fail();
            }
            catch (InvalidProtocolBufferException)
            {
            }

            // Collection was not pre-initialized
            Assert.AreEqual(0, field.Count);
        }

        // Fairly perfunctory tests for the non-generic IList implementation
        [Test]
        public void IList_Indexer()
        {
            var field = new RepeatedField<string> { "first", "second" };
            IList list = field;
            Assert.AreEqual("first", list[0]);
            list[1] = "changed";
            Assert.AreEqual("changed", field[1]);
        }

        [Test]
        public void IList_Contains()
        {
            IList list = new RepeatedField<string> { "first", "second" };
            Assert.IsTrue(list.Contains("second"));
            Assert.IsFalse(list.Contains("third"));
            Assert.IsFalse(list.Contains(new object()));
        }

        [Test]
        public void IList_Add()
        {
            IList list = new RepeatedField<string> { "first", "second" };
            list.Add("third");
            CollectionAssert.AreEqual(new[] { "first", "second", "third" }, list);
        }

        [Test]
        public void IList_Remove()
        {
            IList list = new RepeatedField<string> { "first", "second" };
            list.Remove("third"); // No-op, no exception
            list.Remove(new object()); // No-op, no exception
            list.Remove("first");
            CollectionAssert.AreEqual(new[] { "second" }, list);
        }

        [Test]
        public void IList_IsFixedSize()
        {
            var field = new RepeatedField<string> { "first", "second" };
            IList list = field;
            Assert.IsFalse(list.IsFixedSize);
        }

        [Test]
        public void IList_IndexOf()
        {
            IList list = new RepeatedField<string> { "first", "second" };
            Assert.AreEqual(1, list.IndexOf("second"));
            Assert.AreEqual(-1, list.IndexOf("third"));
            Assert.AreEqual(-1, list.IndexOf(new object()));
        }

        [Test]
        public void IList_SyncRoot()
        {
            IList list = new RepeatedField<string> { "first", "second" };
            Assert.AreSame(list, list.SyncRoot);
        }

        [Test]
        public void IList_CopyTo()
        {
            IList list = new RepeatedField<string> { "first", "second" };
            string[] stringArray = new string[4];
            list.CopyTo(stringArray, 1);
            CollectionAssert.AreEqual(new[] { null, "first",  "second", null }, stringArray);

            object[] objectArray = new object[4];
            list.CopyTo(objectArray, 1);
            CollectionAssert.AreEqual(new[] { null, "first", "second", null }, objectArray);

            Assert.Throws<ArrayTypeMismatchException>(() => list.CopyTo(new StringBuilder[4], 1));
            Assert.Throws<ArrayTypeMismatchException>(() => list.CopyTo(new int[4], 1));
        }

        [Test]
        public void IList_IsSynchronized()
        {
            IList list = new RepeatedField<string> { "first", "second" };
            Assert.IsFalse(list.IsSynchronized);
        }

        [Test]
        public void IList_Insert()
        {
            IList list = new RepeatedField<string> { "first", "second" };
            list.Insert(1, "middle");
            CollectionAssert.AreEqual(new[] { "first", "middle", "second" }, list);
        }

        [Test]
        public void ToString_Integers()
        {
            var list = new RepeatedField<int> { 5, 10, 20 };
            var text = list.ToString();
            Assert.AreEqual("[ 5, 10, 20 ]", text);
        }

        [Test]
        public void ToString_Strings()
        {
            var list = new RepeatedField<string> { "x", "y", "z" };
            var text = list.ToString();
            Assert.AreEqual("[ \"x\", \"y\", \"z\" ]", text);
        }

        [Test]
        public void ToString_Messages()
        {
            var list = new RepeatedField<TestAllTypes> { new TestAllTypes { SingleDouble = 1.5 }, new TestAllTypes { SingleInt32 = 10 } };
            var text = list.ToString();
            Assert.AreEqual("[ { \"singleDouble\": 1.5 }, { \"singleInt32\": 10 } ]", text);
        }

        [Test]
        public void ToString_Empty()
        {
            var list = new RepeatedField<TestAllTypes> { };
            var text = list.ToString();
            Assert.AreEqual("[ ]", text);
        }

        [Test]
        public void ToString_InvalidElementType()
        {
            var list = new RepeatedField<decimal> { 15m };
            Assert.Throws<ArgumentException>(() => list.ToString());
        }

        [Test]
        public void ToString_Timestamp()
        {
            var list = new RepeatedField<Timestamp> { Timestamp.FromDateTime(new DateTime(2015, 10, 1, 12, 34, 56, DateTimeKind.Utc)) };
            var text = list.ToString();
            Assert.AreEqual("[ \"2015-10-01T12:34:56Z\" ]", text);
        }

        [Test]
        public void ToString_Struct()
        {
            var message = new Struct { Fields = { { "foo", new Value { NumberValue = 20 } } } };
            var list = new RepeatedField<Struct> { message };
            var text = list.ToString();
            Assert.AreEqual(text, "[ { \"foo\": 20 } ]", message.ToString());
        }

        [Test]
        public void NaNValuesComparedBitwise()
        {
            var list1 = new RepeatedField<double> { SampleNaNs.Regular, SampleNaNs.SignallingFlipped };
            var list2 = new RepeatedField<double> { SampleNaNs.Regular, SampleNaNs.PayloadFlipped };
            var list3 = new RepeatedField<double> { SampleNaNs.Regular, SampleNaNs.SignallingFlipped };

            // All SampleNaNs have the same hashcode under certain targets (e.g. netcoreapp3.1)
            EqualityTester.AssertInequality(list1, list2, checkHashcode: false);
            EqualityTester.AssertEquality(list1, list3);
            Assert.True(list1.Contains(SampleNaNs.SignallingFlipped));
            Assert.False(list2.Contains(SampleNaNs.SignallingFlipped));
        }

        [Test]
        public void Capacity_Increase()
        {
            // Unfortunately this case tests implementation details of RepeatedField.  This is necessary

            var list = new RepeatedField<int>() { 1, 2, 3 };

            Assert.AreEqual(8, list.Capacity);
            Assert.AreEqual(3, list.Count);

            list.Capacity = 10; // Set capacity to a larger value to trigger growth
            Assert.AreEqual(10, list.Capacity, "Capacity increased");
            Assert.AreEqual(3, list.Count);

            CollectionAssert.AreEqual(new int[] {1, 2, 3}, list.ToArray(), "We didn't lose our data in the resize");
        }

        [Test]
        public void Capacity_Decrease()
        {
            var list = new RepeatedField<int>() { 1, 2, 3 };

            Assert.AreEqual(8, list.Capacity);
            Assert.DoesNotThrow(() => list.Capacity = 5, "Can decrease capacity if new capacity is greater than list.Count");
            Assert.AreEqual(5, list.Capacity);

            Assert.DoesNotThrow(() => list.Capacity = 3, "Can set capacity exactly to list.Count" );

            Assert.Throws<ArgumentOutOfRangeException>(() => list.Capacity = 2, "Can't set the capacity smaller than list.Count" );

            Assert.Throws<ArgumentOutOfRangeException>(() => list.Capacity = 0, "Can't set the capacity to zero" );

            Assert.Throws<ArgumentOutOfRangeException>(() => list.Capacity = -1, "Can't set the capacity to negative" );
        }

        [Test]
        public void Capacity_Zero()
        {
            var list = new RepeatedField<int>() { 1 };
            list.RemoveAt(0);
            Assert.AreEqual(0, list.Count);
            Assert.AreEqual(8, list.Capacity);

            Assert.DoesNotThrow(() => list.Capacity = 0, "Can set Capacity to 0");
            Assert.AreEqual(0, list.Capacity);
        }

        [Test]
        public void Clear_CapacityUnaffected()
        {
            var list = new RepeatedField<int> { 1 };
            Assert.AreEqual(1, list.Count);
            Assert.AreEqual(8, list.Capacity);

            list.Clear();
            Assert.AreEqual(0, list.Count);
            Assert.AreEqual(8, list.Capacity);
        }
    }
}
