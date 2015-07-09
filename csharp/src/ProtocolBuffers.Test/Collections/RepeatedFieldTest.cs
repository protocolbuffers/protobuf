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
using System.Collections.Generic;
using System.IO;
using System.Linq;
using Google.Protobuf.TestProtos;
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
            var list = new RepeatedField<string>();
            list.Add("foo");
            Assert.AreEqual(1, list.Count);
            Assert.AreEqual("foo", list[0]);
        }

        [Test]
        public void Add_Sequence()
        {
            var list = new RepeatedField<string>();
            list.Add(new[] { "foo", "bar" });
            Assert.AreEqual(2, list.Count);
            Assert.AreEqual("foo", list[0]);
            Assert.AreEqual("bar", list[1]);
        }

        [Test]
        public void Add_RepeatedField()
        {
            var list = new RepeatedField<string> { "original" };
            list.Add(new RepeatedField<string> { "foo", "bar" });
            Assert.AreEqual(3, list.Count);
            Assert.AreEqual("original", list[0]);
            Assert.AreEqual("foo", list[1]);
            Assert.AreEqual("bar", list[2]);
        }

        [Test]
        public void Freeze_FreezesElements()
        {
            var list = new RepeatedField<TestAllTypes> { new TestAllTypes() };
            Assert.IsFalse(list[0].IsFrozen);
            list.Freeze();
            Assert.IsTrue(list[0].IsFrozen);
        }

        [Test]
        public void Freeze_PreventsMutations()
        {
            var list = new RepeatedField<int> { 0 };
            list.Freeze();
            Assert.Throws<InvalidOperationException>(() => list.Add(1));
            Assert.Throws<InvalidOperationException>(() => list[0] = 1);
            Assert.Throws<InvalidOperationException>(() => list.Clear());
            Assert.Throws<InvalidOperationException>(() => list.RemoveAt(0));
            Assert.Throws<InvalidOperationException>(() => list.Remove(0));
            Assert.Throws<InvalidOperationException>(() => list.Insert(0, 0));
        }

        [Test]
        public void Freeze_ReportsFrozen()
        {
            var list = new RepeatedField<int> { 0 };
            Assert.IsFalse(list.IsFrozen);
            Assert.IsFalse(list.IsReadOnly);
            list.Freeze();
            Assert.IsTrue(list.IsFrozen);
            Assert.IsTrue(list.IsReadOnly);
        }

        [Test]
        public void Clone_ReturnsMutable()
        {
            var list = new RepeatedField<int> { 0 };
            list.Freeze();
            var clone = list.Clone();
            clone[0] = 1;
        }

        [Test]
        public void AddEntriesFrom_PackedInt32()
        {
            uint packedTag = WireFormat.MakeTag(10, WireFormat.WireType.LengthDelimited);
            var stream = new MemoryStream();
            var output = CodedOutputStream.CreateInstance(stream);
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
            var input = CodedInputStream.CreateInstance(stream);
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
            var output = CodedOutputStream.CreateInstance(stream);
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
            var input = CodedInputStream.CreateInstance(stream);
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
            var output = CodedOutputStream.CreateInstance(stream);
            output.WriteTag(tag);
            output.WriteString("Foo");
            output.WriteTag(tag);
            output.WriteString("");
            output.WriteTag(tag);
            output.WriteString("Bar");
            output.Flush();
            stream.Position = 0;

            var field = new RepeatedField<string>();
            var input = CodedInputStream.CreateInstance(stream);
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
            var output = CodedOutputStream.CreateInstance(stream);
            output.WriteTag(tag);
            output.WriteMessage(message1);
            output.WriteTag(tag);
            output.WriteMessage(message2);
            output.Flush();
            stream.Position = 0;

            var field = new RepeatedField<ForeignMessage>();
            var input = CodedInputStream.CreateInstance(stream);
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
            var output = CodedOutputStream.CreateInstance(stream);
            field.WriteTo(output, FieldCodec.ForInt32(tag));
            output.Flush();
            stream.Position = 0;

            var input = CodedInputStream.CreateInstance(stream);
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
            var output = CodedOutputStream.CreateInstance(stream);
            field.WriteTo(output, FieldCodec.ForInt32(tag));
            output.Flush();
            stream.Position = 0;

            var input = CodedInputStream.CreateInstance(stream);
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
            var output = CodedOutputStream.CreateInstance(stream);
            field.WriteTo(output, FieldCodec.ForString(tag));
            output.Flush();
            stream.Position = 0;

            var input = CodedInputStream.CreateInstance(stream);
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
            var output = CodedOutputStream.CreateInstance(stream);
            field.WriteTo(output, FieldCodec.ForMessage(tag, ForeignMessage.Parser));
            output.Flush();
            stream.Position = 0;

            var input = CodedInputStream.CreateInstance(stream);
            input.AssertNextTag(tag);
            Assert.AreEqual(message1, input.ReadMessage(ForeignMessage.Parser));
            input.AssertNextTag(tag);
            Assert.AreEqual(message2, input.ReadMessage(ForeignMessage.Parser));
            Assert.IsTrue(input.IsAtEnd);
        }


        [Test]
        public void TestNegativeEnumArray()
        {
            int arraySize = 1 + 1 + (11 * 5);
            int msgSize = arraySize;
            byte[] bytes = new byte[msgSize];
            CodedOutputStream output = CodedOutputStream.CreateInstance(bytes);
            uint tag = WireFormat.MakeTag(8, WireFormat.WireType.Varint);
            for (int i = 0; i >= -5; i--)
            {
                output.WriteTag(tag);
                output.WriteEnum(i);
            }

            Assert.AreEqual(0, output.SpaceLeft);

            CodedInputStream input = CodedInputStream.CreateInstance(bytes);
            Assert.IsTrue(input.ReadTag(out tag));

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
            CodedOutputStream output = CodedOutputStream.CreateInstance(bytes);
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

            CodedInputStream input = CodedInputStream.CreateInstance(bytes);
            Assert.IsTrue(input.ReadTag(out tag));

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
    }
}
