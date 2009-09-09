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

using System.IO;
using System.Reflection;
using Google.ProtocolBuffers.Descriptors;
using Google.ProtocolBuffers.TestProtos;
using NUnit.Framework;

namespace Google.ProtocolBuffers {
  [TestFixture]
  public class WireFormatTest {

    /// <summary>
    /// Keeps the attributes on FieldType and the switch statement in WireFormat in sync.
    /// </summary>
    [Test]
    public void FieldTypeToWireTypeMapping() {
      foreach (FieldInfo field in typeof(FieldType).GetFields(BindingFlags.Static | BindingFlags.Public)) {
        FieldType fieldType = (FieldType)field.GetValue(null);
        FieldMappingAttribute mapping = (FieldMappingAttribute)field.GetCustomAttributes(typeof(FieldMappingAttribute), false)[0];
        Assert.AreEqual(mapping.WireType, WireFormat.GetWireType(fieldType));
      }
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
    public void SerializationPacked() {
      TestPackedTypes message = TestUtil.GetPackedSet();
      ByteString rawBytes = message.ToByteString();
      Assert.AreEqual(rawBytes.Length, message.SerializedSize);
      TestPackedTypes message2 = TestPackedTypes.ParseFrom(rawBytes);
      TestUtil.AssertPackedFieldsSet(message2);
    }

    [Test]
    public void SerializeExtensions() {
      // TestAllTypes and TestAllExtensions should have compatible wire formats,
      // so if we serialize a TestAllExtensions then parse it as TestAllTypes
      // it should work.
      TestAllExtensions message = TestUtil.GetAllExtensionsSet();
      ByteString rawBytes = message.ToByteString();
      Assert.AreEqual(rawBytes.Length, message.SerializedSize);

      TestAllTypes message2 = TestAllTypes.ParseFrom(rawBytes);

      TestUtil.AssertAllFieldsSet(message2);
    }

    [Test]
    public void SerializePackedExtensions() {
      // TestPackedTypes and TestPackedExtensions should have compatible wire
      // formats; check that they serialize to the same string.
      TestPackedExtensions message = TestUtil.GetPackedExtensionsSet();
      ByteString rawBytes = message.ToByteString();

      TestPackedTypes message2 = TestUtil.GetPackedSet();
      ByteString rawBytes2 = message2.ToByteString();

      Assert.AreEqual(rawBytes, rawBytes2);
    }

    [Test]
    public void SerializeDelimited() {
      MemoryStream stream = new MemoryStream();
      TestUtil.GetAllSet().WriteDelimitedTo(stream);
      stream.WriteByte(12);
      TestUtil.GetPackedSet().WriteDelimitedTo(stream);
      stream.WriteByte(34);

      stream.Position = 0;

      TestUtil.AssertAllFieldsSet(TestAllTypes.ParseDelimitedFrom(stream));
      Assert.AreEqual(12, stream.ReadByte());
      TestUtil.AssertPackedFieldsSet(TestPackedTypes.ParseDelimitedFrom(stream));
      Assert.AreEqual(34, stream.ReadByte());
      Assert.AreEqual(-1, stream.ReadByte());
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

      TestAllExtensions message2 = TestAllExtensions.ParseFrom(rawBytes, registry);

      TestUtil.AssertAllExtensionsSet(message2);
    }

    [Test]
    public void ParsePackedExtensions() {
      // Ensure that packed extensions can be properly parsed.
      TestPackedExtensions message = TestUtil.GetPackedExtensionsSet();
      ByteString rawBytes = message.ToByteString();

      ExtensionRegistry registry = TestUtil.CreateExtensionRegistry();

      TestPackedExtensions message2 = TestPackedExtensions.ParseFrom(rawBytes, registry);
      TestUtil.AssertPackedExtensionsSet(message2);
    }

    [Test]
    public void ExtensionsSerializedSize() {
      Assert.AreEqual(TestUtil.GetAllSet().SerializedSize, TestUtil.GetAllExtensionsSet().SerializedSize);
    }

    private static void AssertFieldsInOrder(ByteString data) {
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
            TestMessageSetExtension1.MessageSetExtension,
            TestMessageSetExtension1.CreateBuilder().SetI(123).Build())
          .SetExtension(
            TestMessageSetExtension2.MessageSetExtension,
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
      extensionRegistry.Add(TestMessageSetExtension1.MessageSetExtension);
      extensionRegistry.Add(TestMessageSetExtension2.MessageSetExtension);

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

      Assert.AreEqual(123, messageSet.GetExtension(TestMessageSetExtension1.MessageSetExtension).I);
      Assert.AreEqual("foo", messageSet.GetExtension(TestMessageSetExtension2.MessageSetExtension).Str);

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
