// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.
// http://code.google.com/p/protobuf/
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
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

  }
}
