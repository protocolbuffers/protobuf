#region Copyright notice and license
// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
// http://github.com/jskeet/dotnet-protobufs/
// Original C++/Java/Python code:
// http://code.google.com/p/protobuf/
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
using Google.ProtocolBuffers.Descriptors;
using Google.ProtocolBuffers.TestProtos;
using NUnit.Framework;

namespace Google.ProtocolBuffers {
  [TestFixture]
  public class UnknownFieldSetTest {

    private MessageDescriptor descriptor;
    private TestAllTypes allFields;
    private ByteString allFieldsData;

    /// <summary>
    /// An empty message that has been parsed from allFieldsData.  So, it has
    /// unknown fields of every type.
    /// </summary>
    private TestEmptyMessage emptyMessage;
    private UnknownFieldSet unknownFields;

    [SetUp]
    public void SetUp() {
      descriptor = TestAllTypes.Descriptor;
      allFields = TestUtil.GetAllSet();
      allFieldsData = allFields.ToByteString();
      emptyMessage = TestEmptyMessage.ParseFrom(allFieldsData);
      unknownFields = emptyMessage.UnknownFields;
    }

    private UnknownField GetField(String name) {
      FieldDescriptor field = descriptor.FindDescriptor<FieldDescriptor>(name);
      Assert.IsNotNull(field);
      return unknownFields.FieldDictionary[field.FieldNumber];
    }

    /// <summary>
    /// Constructs a protocol buffer which contains fields with all the same
    /// numbers as allFieldsData except that each field is some other wire
    /// type.
    /// </summary>
    private ByteString GetBizarroData() {
      UnknownFieldSet.Builder bizarroFields = UnknownFieldSet.CreateBuilder();

      UnknownField varintField = UnknownField.CreateBuilder().AddVarint(1).Build();
      UnknownField fixed32Field = UnknownField.CreateBuilder().AddFixed32(1).Build();

      foreach (KeyValuePair<int, UnknownField> entry in unknownFields.FieldDictionary) {
        if (entry.Value.VarintList.Count == 0) {
          // Original field is not a varint, so use a varint.
          bizarroFields.AddField(entry.Key, varintField);
        } else {
          // Original field *is* a varint, so use something else.
          bizarroFields.AddField(entry.Key, fixed32Field);
        }
      }

      return bizarroFields.Build().ToByteString();
    }

    // =================================================================

    [Test]
    public void Varint() {
      UnknownField field = GetField("optional_int32");
      Assert.AreEqual(1, field.VarintList.Count);
      Assert.AreEqual(allFields.OptionalInt32, (long) field.VarintList[0]);
    }

    [Test]
    public void Fixed32() {
      UnknownField field = GetField("optional_fixed32");
      Assert.AreEqual(1, field.Fixed32List.Count);
      Assert.AreEqual(allFields.OptionalFixed32, (int) field.Fixed32List[0]);
    }

    [Test]
    public void Fixed64() {
      UnknownField field = GetField("optional_fixed64");
      Assert.AreEqual(1, field.Fixed64List.Count);
      Assert.AreEqual(allFields.OptionalFixed64, (long) field.Fixed64List[0]);
    }

    [Test]
    public void LengthDelimited() {
      UnknownField field = GetField("optional_bytes");
      Assert.AreEqual(1, field.LengthDelimitedList.Count);
      Assert.AreEqual(allFields.OptionalBytes, field.LengthDelimitedList[0]);
    }

    [Test]
    public void Group() {
      FieldDescriptor nestedFieldDescriptor =
        TestAllTypes.Types.OptionalGroup.Descriptor.FindDescriptor<FieldDescriptor>("a");
      Assert.IsNotNull(nestedFieldDescriptor);

      UnknownField field = GetField("optionalgroup");
      Assert.AreEqual(1, field.GroupList.Count);

      UnknownFieldSet group = field.GroupList[0];
      Assert.AreEqual(1, group.FieldDictionary.Count);
      Assert.IsTrue(group.HasField(nestedFieldDescriptor.FieldNumber));

      UnknownField nestedField = group[nestedFieldDescriptor.FieldNumber];
      Assert.AreEqual(1, nestedField.VarintList.Count);
      Assert.AreEqual(allFields.OptionalGroup.A, (long) nestedField.VarintList[0]);
    }

    [Test]
    public void Serialize() {
      // Check that serializing the UnknownFieldSet produces the original data again.
      ByteString data = emptyMessage.ToByteString();
      Assert.AreEqual(allFieldsData, data);
    }

    [Test]
    public void CopyFrom() {
      TestEmptyMessage message =
        TestEmptyMessage.CreateBuilder().MergeFrom(emptyMessage).Build();

      Assert.AreEqual(emptyMessage.ToString(), message.ToString());
    }

    [Test]
    public void MergeFrom() {
      TestEmptyMessage source =
        TestEmptyMessage.CreateBuilder()
          .SetUnknownFields(
            UnknownFieldSet.CreateBuilder()
              .AddField(2,
                UnknownField.CreateBuilder()
                  .AddVarint(2).Build())
              .AddField(3,
                UnknownField.CreateBuilder()
                  .AddVarint(4).Build())
              .Build())
          .Build();
      TestEmptyMessage destination =
        TestEmptyMessage.CreateBuilder()
          .SetUnknownFields(
            UnknownFieldSet.CreateBuilder()
              .AddField(1,
                UnknownField.CreateBuilder()
                  .AddVarint(1).Build())
              .AddField(3,
                UnknownField.CreateBuilder()
                  .AddVarint(3).Build())
              .Build())
          .MergeFrom(source)
          .Build();

      Assert.AreEqual(
        "1: 1\n" +
        "2: 2\n" +
        "3: 3\n" +
        "3: 4\n",
        destination.ToString());
    }

    [Test]
    public void Clear() {
      UnknownFieldSet fields =
        UnknownFieldSet.CreateBuilder().MergeFrom(unknownFields).Clear().Build();
      Assert.AreEqual(0, fields.FieldDictionary.Count);
    }

    [Test]
    public void ClearMessage() {
      TestEmptyMessage message =
        TestEmptyMessage.CreateBuilder().MergeFrom(emptyMessage).Clear().Build();
      Assert.AreEqual(0, message.SerializedSize);
    }

    [Test]
    public void ParseKnownAndUnknown() {
      // Test mixing known and unknown fields when parsing.

      UnknownFieldSet fields =
        UnknownFieldSet.CreateBuilder(unknownFields)
          .AddField(123456,
            UnknownField.CreateBuilder().AddVarint(654321).Build())
          .Build();

      ByteString data = fields.ToByteString();
      TestAllTypes destination = TestAllTypes.ParseFrom(data);

      TestUtil.AssertAllFieldsSet(destination);
      Assert.AreEqual(1, destination.UnknownFields.FieldDictionary.Count);

      UnknownField field = destination.UnknownFields[123456];
      Assert.AreEqual(1, field.VarintList.Count);
      Assert.AreEqual(654321, (long) field.VarintList[0]);
    }

    [Test]
    public void WrongTypeTreatedAsUnknown() {
      // Test that fields of the wrong wire type are treated like unknown fields
      // when parsing.

      ByteString bizarroData = GetBizarroData();
      TestAllTypes allTypesMessage = TestAllTypes.ParseFrom(bizarroData);
      TestEmptyMessage emptyMessage = TestEmptyMessage.ParseFrom(bizarroData);

      // All fields should have been interpreted as unknown, so the debug strings
      // should be the same.
      Assert.AreEqual(emptyMessage.ToString(), allTypesMessage.ToString());
    }

    [Test]
    public void UnknownExtensions() {
      // Make sure fields are properly parsed to the UnknownFieldSet even when
      // they are declared as extension numbers.

      TestEmptyMessageWithExtensions message =
        TestEmptyMessageWithExtensions.ParseFrom(allFieldsData);

      Assert.AreEqual(unknownFields.FieldDictionary.Count, 
                   message.UnknownFields.FieldDictionary.Count);
      Assert.AreEqual(allFieldsData, message.ToByteString());
    }

    [Test]
    public void WrongExtensionTypeTreatedAsUnknown() {
      // Test that fields of the wrong wire type are treated like unknown fields
      // when parsing extensions.

      ByteString bizarroData = GetBizarroData();
      TestAllExtensions allExtensionsMessage = TestAllExtensions.ParseFrom(bizarroData);
      TestEmptyMessage emptyMessage = TestEmptyMessage.ParseFrom(bizarroData);

      // All fields should have been interpreted as unknown, so the debug strings
      // should be the same.
      Assert.AreEqual(emptyMessage.ToString(),
                   allExtensionsMessage.ToString());
    }

    [Test]
    public void ParseUnknownEnumValue() {
      FieldDescriptor singularField = TestAllTypes.Descriptor.FindDescriptor<FieldDescriptor>("optional_nested_enum");
      FieldDescriptor repeatedField = TestAllTypes.Descriptor.FindDescriptor<FieldDescriptor>("repeated_nested_enum");
      Assert.IsNotNull(singularField);
      Assert.IsNotNull(repeatedField);

      ByteString data =
        UnknownFieldSet.CreateBuilder()
          .AddField(singularField.FieldNumber,
            UnknownField.CreateBuilder()
              .AddVarint((int) TestAllTypes.Types.NestedEnum.BAR)
              .AddVarint(5)   // not valid
              .Build())
          .AddField(repeatedField.FieldNumber,
            UnknownField.CreateBuilder()
              .AddVarint((int) TestAllTypes.Types.NestedEnum.FOO)
              .AddVarint(4)   // not valid
              .AddVarint((int) TestAllTypes.Types.NestedEnum.BAZ)
              .AddVarint(6)   // not valid
              .Build())
          .Build()
          .ToByteString();

      {
        TestAllTypes message = TestAllTypes.ParseFrom(data);
        Assert.AreEqual(TestAllTypes.Types.NestedEnum.BAR,
                     message.OptionalNestedEnum);
        TestUtil.AssertEqual(new [] {TestAllTypes.Types.NestedEnum.FOO, TestAllTypes.Types.NestedEnum.BAZ},
            message.RepeatedNestedEnumList);
        TestUtil.AssertEqual(new[] {5UL}, message.UnknownFields[singularField.FieldNumber].VarintList);
        TestUtil.AssertEqual(new[] {4UL, 6UL}, message.UnknownFields[repeatedField.FieldNumber].VarintList);
      }

      {
        TestAllExtensions message =
          TestAllExtensions.ParseFrom(data, TestUtil.CreateExtensionRegistry());
        Assert.AreEqual(TestAllTypes.Types.NestedEnum.BAR,
          message.GetExtension(UnitTestProtoFile.OptionalNestedEnumExtension));
        TestUtil.AssertEqual(new[] { TestAllTypes.Types.NestedEnum.FOO, TestAllTypes.Types.NestedEnum.BAZ },
          message.GetExtension(UnitTestProtoFile.RepeatedNestedEnumExtension));
        TestUtil.AssertEqual(new[] { 5UL }, message.UnknownFields[singularField.FieldNumber].VarintList);
        TestUtil.AssertEqual(new[] { 4UL, 6UL }, message.UnknownFields[repeatedField.FieldNumber].VarintList);
      }
    }

    [Test]
    public void LargeVarint() {
      ByteString data =
        UnknownFieldSet.CreateBuilder()
          .AddField(1,
            UnknownField.CreateBuilder()
              .AddVarint(0x7FFFFFFFFFFFFFFFL)
              .Build())
          .Build()
          .ToByteString();
      UnknownFieldSet parsed = UnknownFieldSet.ParseFrom(data);
      UnknownField field = parsed[1];
      Assert.AreEqual(1, field.VarintList.Count);
      Assert.AreEqual(0x7FFFFFFFFFFFFFFFUL, field.VarintList[0]);
    }

    [Test]
    public void EqualsAndHashCode() {
      UnknownField fixed32Field = UnknownField.CreateBuilder().AddFixed32(1).Build();
      UnknownField fixed64Field = UnknownField.CreateBuilder().AddFixed64(1).Build();
      UnknownField varIntField = UnknownField.CreateBuilder().AddVarint(1).Build();
      UnknownField lengthDelimitedField = UnknownField.CreateBuilder().AddLengthDelimited(ByteString.Empty).Build();
      UnknownField groupField = UnknownField.CreateBuilder().AddGroup(unknownFields).Build();

      UnknownFieldSet a = UnknownFieldSet.CreateBuilder().AddField(1, fixed32Field).Build();
      UnknownFieldSet b = UnknownFieldSet.CreateBuilder().AddField(1, fixed64Field).Build();
      UnknownFieldSet c = UnknownFieldSet.CreateBuilder().AddField(1, varIntField).Build();
      UnknownFieldSet d = UnknownFieldSet.CreateBuilder().AddField(1, lengthDelimitedField).Build();
      UnknownFieldSet e = UnknownFieldSet.CreateBuilder().AddField(1, groupField).Build();

      CheckEqualsIsConsistent(a);
      CheckEqualsIsConsistent(b);
      CheckEqualsIsConsistent(c);
      CheckEqualsIsConsistent(d);
      CheckEqualsIsConsistent(e);

      CheckNotEqual(a, b);
      CheckNotEqual(a, c);
      CheckNotEqual(a, d);
      CheckNotEqual(a, e);
      CheckNotEqual(b, c);
      CheckNotEqual(b, d);
      CheckNotEqual(b, e);
      CheckNotEqual(c, d);
      CheckNotEqual(c, e);
      CheckNotEqual(d, e);
    }

    /// <summary>
    /// Asserts that the given field sets are not equal and have different
    /// hash codes.
    /// </summary>
    /// <remarks>
    /// It's valid for non-equal objects to have the same hash code, so
    /// this test is stricter than it needs to be. However, this should happen
    /// relatively rarely.
    /// </remarks>
    /// <param name="s1"></param>
    /// <param name="s2"></param>
    private static void CheckNotEqual(UnknownFieldSet s1, UnknownFieldSet s2) {
      String equalsError = string.Format("{0} should not be equal to {1}", s1, s2);
      Assert.IsFalse(s1.Equals(s2), equalsError);
      Assert.IsFalse(s2.Equals(s1), equalsError);

      Assert.IsFalse(s1.GetHashCode() == s2.GetHashCode(),
          string.Format("{0} should have a different hash code from {1}", s1, s2));
          
    }

    /**
     * Asserts that the given field sets are equal and have identical hash codes.
     */
    private static void CheckEqualsIsConsistent(UnknownFieldSet set) {
      // Object should be equal to itself.
      Assert.AreEqual(set, set);

      // Object should be equal to a copy of itself.
      UnknownFieldSet copy = UnknownFieldSet.CreateBuilder(set).Build();
      Assert.AreEqual(set, copy);
      Assert.AreEqual(copy, set);
      Assert.AreEqual(set.GetHashCode(), copy.GetHashCode());
    }
  }
}
