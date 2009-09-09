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

using System.Text;
using Google.ProtocolBuffers.Descriptors;
using Google.ProtocolBuffers.TestProtos;
using NUnit.Framework;

namespace Google.ProtocolBuffers {
  /// <summary>
  /// Tests for descriptors. (Not in its own namespace or broken up into individual classes as the
  /// size doesn't warrant it. On the other hand, this makes me feel a bit dirty...)
  /// </summary>
  [TestFixture]
  public class DescriptorsTest {
    
    [Test]
    public void FileDescriptor()  {
      FileDescriptor file = UnitTestProtoFile.Descriptor;

      Assert.AreEqual("google/protobuf/unittest.proto", file.Name);
      Assert.AreEqual("protobuf_unittest", file.Package);

      Assert.AreEqual("UnittestProto", file.Options.JavaOuterClassname);
      Assert.AreEqual("google/protobuf/unittest.proto", file.Proto.Name);

// TODO(jonskeet): Either change to expect 2 dependencies, or don't emit them.
//      Assert.AreEqual(2, file.Dependencies.Count);
      Assert.AreEqual(UnitTestImportProtoFile.Descriptor, file.Dependencies[1]);

      MessageDescriptor messageType = TestAllTypes.Descriptor;
      Assert.AreEqual(messageType, file.MessageTypes[0]);
      Assert.AreEqual(messageType, file.FindTypeByName<MessageDescriptor>("TestAllTypes"));
      Assert.IsNull(file.FindTypeByName<MessageDescriptor>("NoSuchType"));
      Assert.IsNull(file.FindTypeByName<MessageDescriptor>("protobuf_unittest.TestAllTypes"));
      for (int i = 0; i < file.MessageTypes.Count; i++) {
        Assert.AreEqual(i, file.MessageTypes[i].Index);
      }

      Assert.AreEqual(file.EnumTypes[0], file.FindTypeByName<EnumDescriptor>("ForeignEnum"));
      Assert.IsNull(file.FindTypeByName<EnumDescriptor>("NoSuchType"));
      Assert.IsNull(file.FindTypeByName<EnumDescriptor>("protobuf_unittest.ForeignEnum"));
      Assert.AreEqual(1, UnitTestImportProtoFile.Descriptor.EnumTypes.Count);
      Assert.AreEqual("ImportEnum", UnitTestImportProtoFile.Descriptor.EnumTypes[0].Name);
      for (int i = 0; i < file.EnumTypes.Count; i++) {
        Assert.AreEqual(i, file.EnumTypes[i].Index);
      }

      ServiceDescriptor service = TestService.Descriptor;
      Assert.AreEqual(service, file.Services[0]);
      Assert.AreEqual(service, file.FindTypeByName<ServiceDescriptor>("TestService"));
      Assert.IsNull(file.FindTypeByName<ServiceDescriptor>("NoSuchType"));
      Assert.IsNull(file.FindTypeByName<ServiceDescriptor>("protobuf_unittest.TestService"));
      Assert.AreEqual(0, UnitTestImportProtoFile.Descriptor.Services.Count);
      for (int i = 0; i < file.Services.Count; i++) {
        Assert.AreEqual(i, file.Services[i].Index);
      }

      FieldDescriptor extension = UnitTestProtoFile.OptionalInt32Extension.Descriptor;
      Assert.AreEqual(extension, file.Extensions[0]);
      Assert.AreEqual(extension, file.FindTypeByName<FieldDescriptor>("optional_int32_extension"));
      Assert.IsNull(file.FindTypeByName<FieldDescriptor>("no_such_ext"));
      Assert.IsNull(file.FindTypeByName<FieldDescriptor>("protobuf_unittest.optional_int32_extension"));
      Assert.AreEqual(0, UnitTestImportProtoFile.Descriptor.Extensions.Count);
      for (int i = 0; i < file.Extensions.Count; i++) {
        Assert.AreEqual(i, file.Extensions[i].Index);
      }
    }

    [Test]
    public void MessageDescriptor() {
      MessageDescriptor messageType = TestAllTypes.Descriptor;
      MessageDescriptor nestedType = TestAllTypes.Types.NestedMessage.Descriptor;

      Assert.AreEqual("TestAllTypes", messageType.Name);
      Assert.AreEqual("protobuf_unittest.TestAllTypes", messageType.FullName);
      Assert.AreEqual(UnitTestProtoFile.Descriptor, messageType.File);
      Assert.IsNull(messageType.ContainingType);
      Assert.AreEqual(DescriptorProtos.MessageOptions.DefaultInstance, messageType.Options);
      Assert.AreEqual("TestAllTypes", messageType.Proto.Name);

      Assert.AreEqual("NestedMessage", nestedType.Name);
      Assert.AreEqual("protobuf_unittest.TestAllTypes.NestedMessage", nestedType.FullName);
      Assert.AreEqual(UnitTestProtoFile.Descriptor, nestedType.File);
      Assert.AreEqual(messageType, nestedType.ContainingType);

      FieldDescriptor field = messageType.Fields[0];
      Assert.AreEqual("optional_int32", field.Name);
      Assert.AreEqual(field, messageType.FindDescriptor<FieldDescriptor>("optional_int32"));
      Assert.IsNull(messageType.FindDescriptor<FieldDescriptor>("no_such_field"));
      Assert.AreEqual(field, messageType.FindFieldByNumber(1));
      Assert.IsNull(messageType.FindFieldByNumber(571283));
      for (int i = 0; i < messageType.Fields.Count; i++) {
        Assert.AreEqual(i, messageType.Fields[i].Index);
      }

      Assert.AreEqual(nestedType, messageType.NestedTypes[0]);
      Assert.AreEqual(nestedType, messageType.FindDescriptor<MessageDescriptor>("NestedMessage"));
      Assert.IsNull(messageType.FindDescriptor<MessageDescriptor>("NoSuchType"));
      for (int i = 0; i < messageType.NestedTypes.Count; i++) {
        Assert.AreEqual(i, messageType.NestedTypes[i].Index);
      }

      Assert.AreEqual(messageType.EnumTypes[0], messageType.FindDescriptor<EnumDescriptor>("NestedEnum"));
      Assert.IsNull(messageType.FindDescriptor<EnumDescriptor>("NoSuchType"));
      for (int i = 0; i < messageType.EnumTypes.Count; i++) {
        Assert.AreEqual(i, messageType.EnumTypes[i].Index);
      }
    }

    [Test]
    public void FieldDescriptor() {
      MessageDescriptor messageType = TestAllTypes.Descriptor;
      FieldDescriptor primitiveField = messageType.FindDescriptor<FieldDescriptor>("optional_int32");
      FieldDescriptor enumField = messageType.FindDescriptor<FieldDescriptor>("optional_nested_enum");
      FieldDescriptor messageField = messageType.FindDescriptor<FieldDescriptor>("optional_foreign_message");
      FieldDescriptor cordField = messageType.FindDescriptor<FieldDescriptor>("optional_cord");
      FieldDescriptor extension = UnitTestProtoFile.OptionalInt32Extension.Descriptor;
      FieldDescriptor nestedExtension = TestRequired.Single.Descriptor;

      Assert.AreEqual("optional_int32", primitiveField.Name);
      Assert.AreEqual("protobuf_unittest.TestAllTypes.optional_int32",
                   primitiveField.FullName);
      Assert.AreEqual(1, primitiveField.FieldNumber);
      Assert.AreEqual(messageType, primitiveField.ContainingType);
      Assert.AreEqual(UnitTestProtoFile.Descriptor, primitiveField.File);
      Assert.AreEqual(FieldType.Int32, primitiveField.FieldType);
      Assert.AreEqual(MappedType.Int32, primitiveField.MappedType);
      Assert.AreEqual(DescriptorProtos.FieldOptions.DefaultInstance, primitiveField.Options);
      Assert.IsFalse(primitiveField.IsExtension);
      Assert.AreEqual("optional_int32", primitiveField.Proto.Name);

      Assert.AreEqual("optional_nested_enum", enumField.Name);
      Assert.AreEqual(FieldType.Enum, enumField.FieldType);
      Assert.AreEqual(MappedType.Enum, enumField.MappedType);
      // Assert.AreEqual(TestAllTypes.Types.NestedEnum.Descriptor, enumField.EnumType);

      Assert.AreEqual("optional_foreign_message", messageField.Name);
      Assert.AreEqual(FieldType.Message, messageField.FieldType);
      Assert.AreEqual(MappedType.Message, messageField.MappedType);
      Assert.AreEqual(ForeignMessage.Descriptor, messageField.MessageType);

      Assert.AreEqual("optional_cord", cordField.Name);
      Assert.AreEqual(FieldType.String, cordField.FieldType);
      Assert.AreEqual(MappedType.String, cordField.MappedType);
      Assert.AreEqual(DescriptorProtos.FieldOptions.Types.CType.CORD, cordField.Options.Ctype);

      Assert.AreEqual("optional_int32_extension", extension.Name);
      Assert.AreEqual("protobuf_unittest.optional_int32_extension", extension.FullName);
      Assert.AreEqual(1, extension.FieldNumber);
      Assert.AreEqual(TestAllExtensions.Descriptor, extension.ContainingType);
      Assert.AreEqual(UnitTestProtoFile.Descriptor, extension.File);
      Assert.AreEqual(FieldType.Int32, extension.FieldType);
      Assert.AreEqual(MappedType.Int32, extension.MappedType);
      Assert.AreEqual(DescriptorProtos.FieldOptions.DefaultInstance,
                   extension.Options);
      Assert.IsTrue(extension.IsExtension);
      Assert.AreEqual(null, extension.ExtensionScope);
      Assert.AreEqual("optional_int32_extension", extension.Proto.Name);

      Assert.AreEqual("single", nestedExtension.Name);
      Assert.AreEqual("protobuf_unittest.TestRequired.single",
                   nestedExtension.FullName);
      Assert.AreEqual(TestRequired.Descriptor,
                   nestedExtension.ExtensionScope);
    }

    [Test]
    public void FieldDescriptorLabel() {
      FieldDescriptor requiredField =
        TestRequired.Descriptor.FindDescriptor<FieldDescriptor>("a");
      FieldDescriptor optionalField =
        TestAllTypes.Descriptor.FindDescriptor<FieldDescriptor>("optional_int32");
      FieldDescriptor repeatedField =
        TestAllTypes.Descriptor.FindDescriptor<FieldDescriptor>("repeated_int32");

      Assert.IsTrue(requiredField.IsRequired);
      Assert.IsFalse(requiredField.IsRepeated);
      Assert.IsFalse(optionalField.IsRequired);
      Assert.IsFalse(optionalField.IsRepeated);
      Assert.IsFalse(repeatedField.IsRequired);
      Assert.IsTrue(repeatedField.IsRepeated);
    }

    [Test]
    public void FieldDescriptorDefault() {
      MessageDescriptor d = TestAllTypes.Descriptor;
      Assert.IsFalse(d.FindDescriptor<FieldDescriptor>("optional_int32").HasDefaultValue);
      Assert.AreEqual(0, d.FindDescriptor<FieldDescriptor>("optional_int32").DefaultValue);
      Assert.IsTrue(d.FindDescriptor<FieldDescriptor>("default_int32").HasDefaultValue);
      Assert.AreEqual(41, d.FindDescriptor<FieldDescriptor>("default_int32").DefaultValue);

      d = TestExtremeDefaultValues.Descriptor;
      Assert.AreEqual(ByteString.CopyFrom("\u0000\u0001\u0007\b\f\n\r\t\u000b\\\'\"\u00fe", Encoding.GetEncoding(28591)),
        d.FindDescriptor<FieldDescriptor>("escaped_bytes").DefaultValue);
      Assert.AreEqual(uint.MaxValue, d.FindDescriptor<FieldDescriptor>("large_uint32").DefaultValue);
      Assert.AreEqual(ulong.MaxValue, d.FindDescriptor<FieldDescriptor>("large_uint64").DefaultValue);
    }

    [Test]
    public void EnumDescriptor()  {
      // Note: this test is a bit different to the Java version because there's no static way of getting to the descriptor
      EnumDescriptor enumType = UnitTestProtoFile.Descriptor.FindTypeByName<EnumDescriptor>("ForeignEnum");
      EnumDescriptor nestedType = TestAllTypes.Descriptor.FindDescriptor<EnumDescriptor>("NestedEnum");

      Assert.AreEqual("ForeignEnum", enumType.Name);
      Assert.AreEqual("protobuf_unittest.ForeignEnum", enumType.FullName);
      Assert.AreEqual(UnitTestProtoFile.Descriptor, enumType.File);
      Assert.IsNull(enumType.ContainingType);
      Assert.AreEqual(DescriptorProtos.EnumOptions.DefaultInstance,
                   enumType.Options);

      Assert.AreEqual("NestedEnum", nestedType.Name);
      Assert.AreEqual("protobuf_unittest.TestAllTypes.NestedEnum",
                   nestedType.FullName);
      Assert.AreEqual(UnitTestProtoFile.Descriptor, nestedType.File);
      Assert.AreEqual(TestAllTypes.Descriptor, nestedType.ContainingType);

      EnumValueDescriptor value = enumType.FindValueByName("FOREIGN_FOO");
      Assert.AreEqual(value, enumType.Values[0]);
      Assert.AreEqual("FOREIGN_FOO", value.Name);
      Assert.AreEqual(4, value.Number);
      Assert.AreEqual((int) ForeignEnum.FOREIGN_FOO, value.Number);
      Assert.AreEqual(value, enumType.FindValueByNumber(4));
      Assert.IsNull(enumType.FindValueByName("NO_SUCH_VALUE"));
      for (int i = 0; i < enumType.Values.Count; i++) {
        Assert.AreEqual(i, enumType.Values[i].Index);
      }
    }

    [Test]
    public void ServiceDescriptor() {
      ServiceDescriptor service = TestService.Descriptor;

      Assert.AreEqual("TestService", service.Name);
      Assert.AreEqual("protobuf_unittest.TestService", service.FullName);
      Assert.AreEqual(UnitTestProtoFile.Descriptor, service.File);

      Assert.AreEqual(2, service.Methods.Count);

      MethodDescriptor fooMethod = service.Methods[0];
      Assert.AreEqual("Foo", fooMethod.Name);
      Assert.AreEqual(FooRequest.Descriptor, fooMethod.InputType);
      Assert.AreEqual(FooResponse.Descriptor, fooMethod.OutputType);
      Assert.AreEqual(fooMethod, service.FindMethodByName("Foo"));

      MethodDescriptor barMethod = service.Methods[1];
      Assert.AreEqual("Bar", barMethod.Name);
      Assert.AreEqual(BarRequest.Descriptor, barMethod.InputType);
      Assert.AreEqual(BarResponse.Descriptor, barMethod.OutputType);
      Assert.AreEqual(barMethod, service.FindMethodByName("Bar"));

      Assert.IsNull(service.FindMethodByName("NoSuchMethod"));

      for (int i = 0; i < service.Methods.Count; i++) {
        Assert.AreEqual(i, service.Methods[i].Index);
      }
    }

    [Test]
    public void CustomOptions() {
      MessageDescriptor descriptor = TestMessageWithCustomOptions.Descriptor;
      Assert.IsTrue(descriptor.Options.HasExtension(UnitTestCustomOptionsProtoFile.MessageOpt1));
      Assert.AreEqual(-56, descriptor.Options.GetExtension(UnitTestCustomOptionsProtoFile.MessageOpt1));


      FieldDescriptor field = descriptor.FindFieldByName("field1");
      Assert.IsNotNull(field);

      Assert.IsTrue(field.Options.HasExtension(UnitTestCustomOptionsProtoFile.FieldOpt1));
      Assert.AreEqual(8765432109L, field.Options.GetExtension(UnitTestCustomOptionsProtoFile.FieldOpt1));

      // TODO: Write out enum descriptors
      /*
      EnumDescriptor enumType = TestMessageWithCustomOptions.Types.
        UnittestCustomOptions.TestMessageWithCustomOptions.AnEnum.getDescriptor();

      Assert.IsTrue(
        enumType.getOptions().hasExtension(UnittestCustomOptions.enumOpt1));
      Assert.AreEqual(Integer.valueOf(-789),
        enumType.getOptions().getExtension(UnittestCustomOptions.enumOpt1));
        */

      ServiceDescriptor service = TestServiceWithCustomOptions.Descriptor;

      Assert.IsTrue(service.Options.HasExtension(UnitTestCustomOptionsProtoFile.ServiceOpt1));
      Assert.AreEqual(-9876543210L, service.Options.GetExtension(UnitTestCustomOptionsProtoFile.ServiceOpt1));

      MethodDescriptor method = service.FindMethodByName("Foo");
      Assert.IsNotNull(method);

      Assert.IsTrue(method.Options.HasExtension(UnitTestCustomOptionsProtoFile.MethodOpt1));
      Assert.AreEqual(MethodOpt1.METHODOPT1_VAL2, method.Options.GetExtension(UnitTestCustomOptionsProtoFile.MethodOpt1));
    }
  }
}
