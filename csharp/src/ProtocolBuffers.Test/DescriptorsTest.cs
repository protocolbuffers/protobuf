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

using Google.ProtocolBuffers.Descriptors;
using Google.ProtocolBuffers.TestProtos;
using Xunit;

namespace Google.ProtocolBuffers
{
    /// <summary>
    /// Tests for descriptors. (Not in its own namespace or broken up into individual classes as the
    /// size doesn't warrant it. On the other hand, this makes me feel a bit dirty...)
    /// </summary>
    public class DescriptorsTest
    {
        [Fact]
        public void FileDescriptor()
        {
            FileDescriptor file = Unittest.Descriptor;

            Assert.Equal("google/protobuf/unittest.proto", file.Name);
            Assert.Equal("protobuf_unittest", file.Package);

            Assert.Equal("UnittestProto", file.Options.JavaOuterClassname);
            Assert.Equal("google/protobuf/unittest.proto", file.Proto.Name);

            // unittest.proto doesn't have any public imports, but unittest_import.proto does.
            Assert.Equal(0, file.PublicDependencies.Count);
            Assert.Equal(1, UnittestImport.Descriptor.PublicDependencies.Count);
            Assert.Equal(UnittestImportPublic.Descriptor, UnittestImport.Descriptor.PublicDependencies[0]);

            Assert.Equal(1, file.Dependencies.Count);
            Assert.Equal(UnittestImport.Descriptor, file.Dependencies[0]);

            MessageDescriptor messageType = TestAllTypes.Descriptor;
            Assert.Equal(messageType, file.MessageTypes[0]);
            Assert.Equal(messageType, file.FindTypeByName<MessageDescriptor>("TestAllTypes"));
            Assert.Null(file.FindTypeByName<MessageDescriptor>("NoSuchType"));
            Assert.Null(file.FindTypeByName<MessageDescriptor>("protobuf_unittest.TestAllTypes"));
            for (int i = 0; i < file.MessageTypes.Count; i++)
            {
                Assert.Equal(i, file.MessageTypes[i].Index);
            }

            Assert.Equal(file.EnumTypes[0], file.FindTypeByName<EnumDescriptor>("ForeignEnum"));
            Assert.Null(file.FindTypeByName<EnumDescriptor>("NoSuchType"));
            Assert.Null(file.FindTypeByName<EnumDescriptor>("protobuf_unittest.ForeignEnum"));
            Assert.Equal(1, UnittestImport.Descriptor.EnumTypes.Count);
            Assert.Equal("ImportEnum", UnittestImport.Descriptor.EnumTypes[0].Name);
            for (int i = 0; i < file.EnumTypes.Count; i++)
            {
                Assert.Equal(i, file.EnumTypes[i].Index);
            }

            FieldDescriptor extension = Unittest.OptionalInt32Extension.Descriptor;
            Assert.Equal(extension, file.Extensions[0]);
            Assert.Equal(extension, file.FindTypeByName<FieldDescriptor>("optional_int32_extension"));
            Assert.Null(file.FindTypeByName<FieldDescriptor>("no_such_ext"));
            Assert.Null(file.FindTypeByName<FieldDescriptor>("protobuf_unittest.optional_int32_extension"));
            Assert.Equal(0, UnittestImport.Descriptor.Extensions.Count);
            for (int i = 0; i < file.Extensions.Count; i++)
            {
                Assert.Equal(i, file.Extensions[i].Index);
            }
        }

        [Fact]
        public void MessageDescriptor()
        {
            MessageDescriptor messageType = TestAllTypes.Descriptor;
            MessageDescriptor nestedType = TestAllTypes.Types.NestedMessage.Descriptor;

            Assert.Equal("TestAllTypes", messageType.Name);
            Assert.Equal("protobuf_unittest.TestAllTypes", messageType.FullName);
            Assert.Equal(Unittest.Descriptor, messageType.File);
            Assert.Null(messageType.ContainingType);
            Assert.Equal(DescriptorProtos.MessageOptions.DefaultInstance, messageType.Options);
            Assert.Equal("TestAllTypes", messageType.Proto.Name);

            Assert.Equal("NestedMessage", nestedType.Name);
            Assert.Equal("protobuf_unittest.TestAllTypes.NestedMessage", nestedType.FullName);
            Assert.Equal(Unittest.Descriptor, nestedType.File);
            Assert.Equal(messageType, nestedType.ContainingType);

            FieldDescriptor field = messageType.Fields[0];
            Assert.Equal("optional_int32", field.Name);
            Assert.Equal(field, messageType.FindDescriptor<FieldDescriptor>("optional_int32"));
            Assert.Null(messageType.FindDescriptor<FieldDescriptor>("no_such_field"));
            Assert.Equal(field, messageType.FindFieldByNumber(1));
            Assert.Null(messageType.FindFieldByNumber(571283));
            for (int i = 0; i < messageType.Fields.Count; i++)
            {
                Assert.Equal(i, messageType.Fields[i].Index);
            }

            Assert.Equal(nestedType, messageType.NestedTypes[0]);
            Assert.Equal(nestedType, messageType.FindDescriptor<MessageDescriptor>("NestedMessage"));
            Assert.Null(messageType.FindDescriptor<MessageDescriptor>("NoSuchType"));
            for (int i = 0; i < messageType.NestedTypes.Count; i++)
            {
                Assert.Equal(i, messageType.NestedTypes[i].Index);
            }

            Assert.Equal(messageType.EnumTypes[0], messageType.FindDescriptor<EnumDescriptor>("NestedEnum"));
            Assert.Null(messageType.FindDescriptor<EnumDescriptor>("NoSuchType"));
            for (int i = 0; i < messageType.EnumTypes.Count; i++)
            {
                Assert.Equal(i, messageType.EnumTypes[i].Index);
            }
        }

        [Fact]
        public void FieldDescriptor()
        {
            MessageDescriptor messageType = TestAllTypes.Descriptor;
            FieldDescriptor primitiveField = messageType.FindDescriptor<FieldDescriptor>("optional_int32");
            FieldDescriptor enumField = messageType.FindDescriptor<FieldDescriptor>("optional_nested_enum");
            FieldDescriptor messageField = messageType.FindDescriptor<FieldDescriptor>("optional_foreign_message");
            FieldDescriptor cordField = messageType.FindDescriptor<FieldDescriptor>("optional_cord");
            FieldDescriptor extension = Unittest.OptionalInt32Extension.Descriptor;
            FieldDescriptor nestedExtension = TestRequired.Single.Descriptor;

            Assert.Equal("optional_int32", primitiveField.Name);
            Assert.Equal("protobuf_unittest.TestAllTypes.optional_int32",
                            primitiveField.FullName);
            Assert.Equal(1, primitiveField.FieldNumber);
            Assert.Equal(messageType, primitiveField.ContainingType);
            Assert.Equal(Unittest.Descriptor, primitiveField.File);
            Assert.Equal(FieldType.Int32, primitiveField.FieldType);
            Assert.Equal(MappedType.Int32, primitiveField.MappedType);
            Assert.Equal(DescriptorProtos.FieldOptions.DefaultInstance, primitiveField.Options);
            Assert.False(primitiveField.IsExtension);
            Assert.Equal("optional_int32", primitiveField.Proto.Name);

            Assert.Equal("optional_nested_enum", enumField.Name);
            Assert.Equal(FieldType.Enum, enumField.FieldType);
            Assert.Equal(MappedType.Enum, enumField.MappedType);
            // Assert.Equal(TestAllTypes.Types.NestedEnum.DescriptorProtoFile, enumField.EnumType);

            Assert.Equal("optional_foreign_message", messageField.Name);
            Assert.Equal(FieldType.Message, messageField.FieldType);
            Assert.Equal(MappedType.Message, messageField.MappedType);
            Assert.Equal(ForeignMessage.Descriptor, messageField.MessageType);

            Assert.Equal("optional_cord", cordField.Name);
            Assert.Equal(FieldType.String, cordField.FieldType);
            Assert.Equal(MappedType.String, cordField.MappedType);
            Assert.Equal(DescriptorProtos.FieldOptions.Types.CType.CORD, cordField.Options.Ctype);

            Assert.Equal("optional_int32_extension", extension.Name);
            Assert.Equal("protobuf_unittest.optional_int32_extension", extension.FullName);
            Assert.Equal(1, extension.FieldNumber);
            Assert.Equal(TestAllExtensions.Descriptor, extension.ContainingType);
            Assert.Equal(Unittest.Descriptor, extension.File);
            Assert.Equal(FieldType.Int32, extension.FieldType);
            Assert.Equal(MappedType.Int32, extension.MappedType);
            Assert.Equal(DescriptorProtos.FieldOptions.DefaultInstance,
                            extension.Options);
            Assert.True(extension.IsExtension);
            Assert.Equal(null, extension.ExtensionScope);
            Assert.Equal("optional_int32_extension", extension.Proto.Name);

            Assert.Equal("single", nestedExtension.Name);
            Assert.Equal("protobuf_unittest.TestRequired.single",
                            nestedExtension.FullName);
            Assert.Equal(TestRequired.Descriptor,
                            nestedExtension.ExtensionScope);
        }

        [Fact]
        public void FieldDescriptorLabel()
        {
            FieldDescriptor requiredField =
                TestRequired.Descriptor.FindDescriptor<FieldDescriptor>("a");
            FieldDescriptor optionalField =
                TestAllTypes.Descriptor.FindDescriptor<FieldDescriptor>("optional_int32");
            FieldDescriptor repeatedField =
                TestAllTypes.Descriptor.FindDescriptor<FieldDescriptor>("repeated_int32");

            Assert.True(requiredField.IsRequired);
            Assert.False(requiredField.IsRepeated);
            Assert.False(optionalField.IsRequired);
            Assert.False(optionalField.IsRepeated);
            Assert.False(repeatedField.IsRequired);
            Assert.True(repeatedField.IsRepeated);
        }
        [Fact]
        public void FieldDescriptorDefault()
        {
            MessageDescriptor d = TestAllTypes.Descriptor;
            Assert.False(d.FindDescriptor<FieldDescriptor>("optional_int32").HasDefaultValue);
            Assert.Equal<object>(0, d.FindDescriptor<FieldDescriptor>("optional_int32").DefaultValue);
            Assert.True(d.FindDescriptor<FieldDescriptor>("default_int32").HasDefaultValue);
            Assert.Equal<object>(41, d.FindDescriptor<FieldDescriptor>("default_int32").DefaultValue);

            d = TestExtremeDefaultValues.Descriptor;
            Assert.Equal<object>(TestExtremeDefaultValues.DefaultInstance.EscapedBytes,
                d.FindDescriptor<FieldDescriptor>("escaped_bytes").DefaultValue);

            Assert.Equal<object>(uint.MaxValue, d.FindDescriptor<FieldDescriptor>("large_uint32").DefaultValue);
            Assert.Equal<object>(ulong.MaxValue, d.FindDescriptor<FieldDescriptor>("large_uint64").DefaultValue);
        }
        [Fact]
        public void EnumDescriptor()
        {
            // Note: this test is a bit different to the Java version because there's no static way of getting to the descriptor
            EnumDescriptor enumType = Unittest.Descriptor.FindTypeByName<EnumDescriptor>("ForeignEnum");
            EnumDescriptor nestedType = TestAllTypes.Descriptor.FindDescriptor<EnumDescriptor>("NestedEnum");

            Assert.Equal("ForeignEnum", enumType.Name);
            Assert.Equal("protobuf_unittest.ForeignEnum", enumType.FullName);
            Assert.Equal(Unittest.Descriptor, enumType.File);
            Assert.Null(enumType.ContainingType);
            Assert.Equal(DescriptorProtos.EnumOptions.DefaultInstance,
                            enumType.Options);

            Assert.Equal("NestedEnum", nestedType.Name);
            Assert.Equal("protobuf_unittest.TestAllTypes.NestedEnum",
                            nestedType.FullName);
            Assert.Equal(Unittest.Descriptor, nestedType.File);
            Assert.Equal(TestAllTypes.Descriptor, nestedType.ContainingType);

            EnumValueDescriptor value = enumType.FindValueByName("FOREIGN_FOO");
            Assert.Equal(value, enumType.Values[0]);
            Assert.Equal("FOREIGN_FOO", value.Name);
            Assert.Equal(4, value.Number);
            Assert.Equal((int) ForeignEnum.FOREIGN_FOO, value.Number);
            Assert.Equal(value, enumType.FindValueByNumber(4));
            Assert.Null(enumType.FindValueByName("NO_SUCH_VALUE"));
            for (int i = 0; i < enumType.Values.Count; i++)
            {
                Assert.Equal(i, enumType.Values[i].Index);
            }
        }
        

        [Fact]
        public void CustomOptions()
        {
            MessageDescriptor descriptor = TestMessageWithCustomOptions.Descriptor;
            Assert.True(descriptor.Options.HasExtension(UnittestCustomOptions.MessageOpt1));
            Assert.Equal(-56, descriptor.Options.GetExtension(UnittestCustomOptions.MessageOpt1));


            FieldDescriptor field = descriptor.FindFieldByName("field1");
            Assert.NotNull(field);

            Assert.True(field.Options.HasExtension(UnittestCustomOptions.FieldOpt1));
            Assert.Equal(8765432109uL, field.Options.GetExtension(UnittestCustomOptions.FieldOpt1));
            
        }
    }
}