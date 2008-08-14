using System;
using System.Collections.Generic;
using System.Text;
using Google.ProtocolBuffers.TestProtos;
using NUnit.Framework;
using Google.ProtocolBuffers.Descriptors;

namespace Google.ProtocolBuffers {
  [TestFixture]
  public class WireFormatTest {

    [Test]
    public void FieldTypeToWireTypeMapping() {

      // Just test a few values
      Assert.AreEqual(WireFormat.WireType.Fixed64, WireFormat.FieldTypeToWireFormatMap[FieldType.SFixed64]);
      Assert.AreEqual(WireFormat.WireType.LengthDelimited, WireFormat.FieldTypeToWireFormatMap[FieldType.String]);
      Assert.AreEqual(WireFormat.WireType.LengthDelimited, WireFormat.FieldTypeToWireFormatMap[FieldType.Message]);
    }

    [Test]
    public void Serialization() {
      TestAllTypes message = TestUtil.GetAllSet();

      ByteString rawBytes = message.ToByteString();
      Assert.AreEqual(rawBytes.Length, message.SerializedSize);

      TestAllTypes message2 = TestAllTypes.ParseFrom(rawBytes);

      TestUtil.AssertAllFieldsSet(message2);
    }

    [Test]
    public void SerializeExtensions() {
      // TestAllTypes and TestAllExtensions should have compatible wire formats,
      // so if we serealize a TestAllExtensions then parse it as TestAllTypes
      // it should work.

      TestAllExtensions message = TestUtil.GetAllExtensionsSet();
      ByteString rawBytes = message.ToByteString();
      Assert.AreEqual(rawBytes.Length, message.SerializedSize);

      TestAllTypes message2 = TestAllTypes.ParseFrom(rawBytes);

      TestUtil.AssertAllFieldsSet(message2);
    }

    [Test]
    public void ParseExtensions() {
      // TestAllTypes and TestAllExtensions should have compatible wire formats,
      // so if we serealize a TestAllTypes then parse it as TestAllExtensions
      // it should work.

      TestAllTypes message = TestUtil.GetAllSet();
      ByteString rawBytes = message.ToByteString();

      ExtensionRegistry registry = ExtensionRegistry.CreateInstance();
      TestUtil.RegisterAllExtensions(registry);
      registry = registry.AsReadOnly();

      TestAllExtensions message2 =
        TestAllExtensions.ParseFrom(rawBytes, registry);

      TestUtil.AssertAllExtensionsSet(message2);
    }

    [Test]
    public void ExtensionsSerializedSize() {
      Assert.AreEqual(TestUtil.GetAllSet().SerializedSize, TestUtil.GetAllExtensionsSet().SerializedSize);
    }

    private void AssertFieldsInOrder(ByteString data) {
      CodedInputStream input = data.CreateCodedInput();
      uint previousTag = 0;

      while (true) {
        uint tag = input.ReadTag();
        if (tag == 0) {
          break;
        }

        Assert.IsTrue(tag > previousTag);
        previousTag = tag;
        input.SkipField(tag);
      }
    }

    [Test]
    public void InterleavedFieldsAndExtensions() {
      // Tests that fields are written in order even when extension ranges
      // are interleaved with field numbers.
      ByteString data =
        TestFieldOrderings.CreateBuilder()
          .SetMyInt(1)
          .SetMyString("foo")
          .SetMyFloat(1.0F)
          .SetExtension(UnitTestProtoFile.MyExtensionInt, 23)
          .SetExtension(UnitTestProtoFile.MyExtensionString, "bar")
          .Build().ToByteString();
      AssertFieldsInOrder(data);

      MessageDescriptor descriptor = TestFieldOrderings.Descriptor;
      ByteString dynamic_data =
        DynamicMessage.CreateBuilder(TestFieldOrderings.Descriptor)
          .SetField(descriptor.FindDescriptor<FieldDescriptor>("my_int"), 1L)
          .SetField(descriptor.FindDescriptor<FieldDescriptor>("my_string"), "foo")
          .SetField(descriptor.FindDescriptor<FieldDescriptor>("my_float"), 1.0F)
          .SetField(UnitTestProtoFile.MyExtensionInt.Descriptor, 23)
          .SetField(UnitTestProtoFile.MyExtensionString.Descriptor, "bar")
          .WeakBuild().ToByteString();
      AssertFieldsInOrder(dynamic_data);
    }

    private const int UnknownTypeId = 1550055;
    private static readonly int TypeId1 = TestMessageSetExtension1.Descriptor.Extensions[0].FieldNumber;
    private static readonly int TypeId2 = TestMessageSetExtension2.Descriptor.Extensions[0].FieldNumber;

    [Test]
    public void SerializeMessageSet() {
      // Set up a TestMessageSet with two known messages and an unknown one.
      TestMessageSet messageSet =
        TestMessageSet.CreateBuilder()
          .SetExtension(
            TestMessageSetExtension1.Types.MessageSetExtension,
            TestMessageSetExtension1.CreateBuilder().SetI(123).Build())
          .SetExtension(
            TestMessageSetExtension2.Types.MessageSetExtension,
            TestMessageSetExtension2.CreateBuilder().SetStr("foo").Build())
          .SetUnknownFields(
            UnknownFieldSet.CreateBuilder()
              .AddField(UnknownTypeId,
                UnknownField.CreateBuilder()
                  .AddLengthDelimited(ByteString.CopyFromUtf8("bar"))
                  .Build())
              .Build())
          .Build();

      ByteString data = messageSet.ToByteString();

      // Parse back using RawMessageSet and check the contents.
      RawMessageSet raw = RawMessageSet.ParseFrom(data);

      Assert.AreEqual(0, raw.UnknownFields.FieldDictionary.Count);

      Assert.AreEqual(3, raw.ItemCount);
      Assert.AreEqual(TypeId1, raw.ItemList[0].TypeId);
      Assert.AreEqual(TypeId2, raw.ItemList[1].TypeId);
      Assert.AreEqual(UnknownTypeId, raw.ItemList[2].TypeId);

      TestMessageSetExtension1 message1 = TestMessageSetExtension1.ParseFrom(raw.GetItem(0).Message.ToByteArray());
      Assert.AreEqual(123, message1.I);

      TestMessageSetExtension2 message2 = TestMessageSetExtension2.ParseFrom(raw.GetItem(1).Message.ToByteArray());
      Assert.AreEqual("foo", message2.Str);

      Assert.AreEqual("bar", raw.GetItem(2).Message.ToStringUtf8());
    }
     
    [Test]
    public void ParseMessageSet() {
      ExtensionRegistry extensionRegistry = ExtensionRegistry.CreateInstance();
      extensionRegistry.Add(TestMessageSetExtension1.Types.MessageSetExtension);
      extensionRegistry.Add(TestMessageSetExtension2.Types.MessageSetExtension);

      // Set up a RawMessageSet with two known messages and an unknown one.
      RawMessageSet raw =
        RawMessageSet.CreateBuilder()
          .AddItem(
            RawMessageSet.Types.Item.CreateBuilder()
              .SetTypeId(TypeId1)
              .SetMessage(
                TestMessageSetExtension1.CreateBuilder()
                  .SetI(123)
                  .Build().ToByteString())
              .Build())
          .AddItem(
            RawMessageSet.Types.Item.CreateBuilder()
              .SetTypeId(TypeId2)
              .SetMessage(
                TestMessageSetExtension2.CreateBuilder()
                  .SetStr("foo")
                  .Build().ToByteString())
              .Build())
          .AddItem(
            RawMessageSet.Types.Item.CreateBuilder()
              .SetTypeId(UnknownTypeId)
              .SetMessage(ByteString.CopyFromUtf8("bar"))
              .Build())
          .Build();

      ByteString data = raw.ToByteString();

      // Parse as a TestMessageSet and check the contents.
      TestMessageSet messageSet =
        TestMessageSet.ParseFrom(data, extensionRegistry);

      Assert.AreEqual(123, messageSet.GetExtension(TestMessageSetExtension1.Types.MessageSetExtension).I);
      Assert.AreEqual("foo", messageSet.GetExtension(TestMessageSetExtension2.Types.MessageSetExtension).Str);

      // Check for unknown field with type LENGTH_DELIMITED,
      //   number UNKNOWN_TYPE_ID, and contents "bar".
      UnknownFieldSet unknownFields = messageSet.UnknownFields;
      Assert.AreEqual(1, unknownFields.FieldDictionary.Count);
      Assert.IsTrue(unknownFields.HasField(UnknownTypeId));

      UnknownField field = unknownFields[UnknownTypeId];
      Assert.AreEqual(1, field.LengthDelimitedList.Count);
      Assert.AreEqual("bar", field.LengthDelimitedList[0].ToStringUtf8());
    }

  }
}
