// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
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

package com.google.protobuf;

import static com.google.common.truth.Truth.assertThat;
import static com.google.common.truth.Truth.assertWithMessage;

import com.google.protobuf.DescriptorProtos.DescriptorProto;
import com.google.protobuf.DescriptorProtos.EnumDescriptorProto;
import com.google.protobuf.DescriptorProtos.EnumValueDescriptorProto;
import com.google.protobuf.DescriptorProtos.FieldDescriptorProto;
import com.google.protobuf.DescriptorProtos.FileDescriptorProto;
import com.google.protobuf.Descriptors.Descriptor;
import com.google.protobuf.Descriptors.DescriptorValidationException;
import com.google.protobuf.Descriptors.EnumDescriptor;
import com.google.protobuf.Descriptors.EnumValueDescriptor;
import com.google.protobuf.Descriptors.FieldDescriptor;
import com.google.protobuf.Descriptors.FileDescriptor;
import com.google.protobuf.Descriptors.MethodDescriptor;
import com.google.protobuf.Descriptors.OneofDescriptor;
import com.google.protobuf.Descriptors.ServiceDescriptor;
import com.google.protobuf.test.UnittestImport;
import com.google.protobuf.test.UnittestImport.ImportEnum;
import com.google.protobuf.test.UnittestImport.ImportEnumForMap;
import protobuf_unittest.TestCustomOptions;
import protobuf_unittest.UnittestCustomOptions;
import protobuf_unittest.UnittestProto;
import protobuf_unittest.UnittestProto.ForeignEnum;
import protobuf_unittest.UnittestProto.ForeignMessage;
import protobuf_unittest.UnittestProto.TestAllExtensions;
import protobuf_unittest.UnittestProto.TestAllTypes;
import protobuf_unittest.UnittestProto.TestExtremeDefaultValues;
import protobuf_unittest.UnittestProto.TestJsonName;
import protobuf_unittest.UnittestProto.TestMultipleExtensionRanges;
import protobuf_unittest.UnittestProto.TestRequired;
import protobuf_unittest.UnittestProto.TestReservedFields;
import protobuf_unittest.UnittestProto.TestService;
import java.util.Collections;
import java.util.List;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;
import protobuf_unittest.NestedExtension;
import protobuf_unittest.NonNestedExtension;

/** Unit test for {@link Descriptors}. */
@RunWith(JUnit4.class)
public class DescriptorsTest {

  // Regression test for bug where referencing a FieldDescriptor.Type value
  // before a FieldDescriptorProto.Type value would yield a
  // ExceptionInInitializerError.
  @SuppressWarnings("unused")
  private static final Object STATIC_INIT_TEST = FieldDescriptor.Type.BOOL;

  @Test
  public void testFieldTypeEnumMapping() throws Exception {
    assertThat(FieldDescriptor.Type.values()).hasLength(FieldDescriptorProto.Type.values().length);
    for (FieldDescriptor.Type type : FieldDescriptor.Type.values()) {
      FieldDescriptorProto.Type protoType = type.toProto();
      assertThat(protoType.name()).isEqualTo("TYPE_" + type.name());
      assertThat(FieldDescriptor.Type.valueOf(protoType)).isEqualTo(type);
    }
  }

  @Test
  public void testFileDescriptor() throws Exception {
    FileDescriptor file = UnittestProto.getDescriptor();

    assertThat(file.getName()).isEqualTo("google/protobuf/unittest.proto");
    assertThat(file.getPackage()).isEqualTo("protobuf_unittest");
    assertThat(file.getOptions().getJavaOuterClassname()).isEqualTo("UnittestProto");
    assertThat(file.toProto().getName()).isEqualTo("google/protobuf/unittest.proto");

    assertThat(file.getDependencies()).containsExactly(UnittestImport.getDescriptor());

    Descriptor messageType = TestAllTypes.getDescriptor();
    assertThat(file.getMessageTypes().get(0)).isEqualTo(messageType);
    assertThat(file.findMessageTypeByName("TestAllTypes")).isEqualTo(messageType);
    assertThat(file.findMessageTypeByName("NoSuchType")).isNull();
    assertThat(file.findMessageTypeByName("protobuf_unittest.TestAllTypes")).isNull();
    for (int i = 0; i < file.getMessageTypes().size(); i++) {
      assertThat(file.getMessageTypes().get(i).getIndex()).isEqualTo(i);
    }

    EnumDescriptor enumType = ForeignEnum.getDescriptor();
    assertThat(file.getEnumTypes().get(0)).isEqualTo(enumType);
    assertThat(file.findEnumTypeByName("ForeignEnum")).isEqualTo(enumType);
    assertThat(file.findEnumTypeByName("NoSuchType")).isNull();
    assertThat(file.findEnumTypeByName("protobuf_unittest.ForeignEnum")).isNull();
    assertThat(UnittestImport.getDescriptor().getEnumTypes())
        .containsExactly(ImportEnum.getDescriptor(), ImportEnumForMap.getDescriptor())
        .inOrder();
    for (int i = 0; i < file.getEnumTypes().size(); i++) {
      assertThat(file.getEnumTypes().get(i).getIndex()).isEqualTo(i);
    }

    ServiceDescriptor service = TestService.getDescriptor();
    assertThat(file.getServices().get(0)).isEqualTo(service);
    assertThat(file.findServiceByName("TestService")).isEqualTo(service);
    assertThat(file.findServiceByName("NoSuchType")).isNull();
    assertThat(file.findServiceByName("protobuf_unittest.TestService")).isNull();
    assertThat(UnittestImport.getDescriptor().getServices()).isEqualTo(Collections.emptyList());
    for (int i = 0; i < file.getServices().size(); i++) {
      assertThat(file.getServices().get(i).getIndex()).isEqualTo(i);
    }

    FieldDescriptor extension = UnittestProto.optionalInt32Extension.getDescriptor();
    assertThat(file.getExtensions().get(0)).isEqualTo(extension);
    assertThat(file.findExtensionByName("optional_int32_extension")).isEqualTo(extension);
    assertThat(file.findExtensionByName("no_such_ext")).isNull();
    assertThat(file.findExtensionByName("protobuf_unittest.optional_int32_extension")).isNull();
    assertThat(UnittestImport.getDescriptor().getExtensions()).isEqualTo(Collections.emptyList());
    for (int i = 0; i < file.getExtensions().size(); i++) {
      assertThat(file.getExtensions().get(i).getIndex()).isEqualTo(i);
    }
  }

  @Test
  public void testDescriptor() throws Exception {
    Descriptor messageType = TestAllTypes.getDescriptor();
    Descriptor nestedType = TestAllTypes.NestedMessage.getDescriptor();

    assertThat(messageType.getName()).isEqualTo("TestAllTypes");
    assertThat(messageType.getFullName()).isEqualTo("protobuf_unittest.TestAllTypes");
    assertThat(messageType.getFile()).isEqualTo(UnittestProto.getDescriptor());
    assertThat(messageType.getContainingType()).isNull();
    assertThat(messageType.getOptions())
        .isEqualTo(DescriptorProtos.MessageOptions.getDefaultInstance());
    assertThat(messageType.toProto().getName()).isEqualTo("TestAllTypes");

    assertThat(nestedType.getName()).isEqualTo("NestedMessage");
    assertThat(nestedType.getFullName()).isEqualTo("protobuf_unittest.TestAllTypes.NestedMessage");
    assertThat(nestedType.getFile()).isEqualTo(UnittestProto.getDescriptor());
    assertThat(nestedType.getContainingType()).isEqualTo(messageType);

    FieldDescriptor field = messageType.getFields().get(0);
    assertThat(field.getName()).isEqualTo("optional_int32");
    assertThat(messageType.findFieldByName("optional_int32")).isEqualTo(field);
    assertThat(messageType.findFieldByName("no_such_field")).isNull();
    assertThat(messageType.findFieldByNumber(1)).isEqualTo(field);
    assertThat(messageType.findFieldByNumber(571283)).isNull();
    for (int i = 0; i < messageType.getFields().size(); i++) {
      assertThat(messageType.getFields().get(i).getIndex()).isEqualTo(i);
    }

    assertThat(messageType.getNestedTypes().get(0)).isEqualTo(nestedType);
    assertThat(messageType.findNestedTypeByName("NestedMessage")).isEqualTo(nestedType);
    assertThat(messageType.findNestedTypeByName("NoSuchType")).isNull();
    for (int i = 0; i < messageType.getNestedTypes().size(); i++) {
      assertThat(messageType.getNestedTypes().get(i).getIndex()).isEqualTo(i);
    }

    EnumDescriptor enumType = TestAllTypes.NestedEnum.getDescriptor();
    assertThat(messageType.getEnumTypes().get(0)).isEqualTo(enumType);
    assertThat(messageType.findEnumTypeByName("NestedEnum")).isEqualTo(enumType);
    assertThat(messageType.findEnumTypeByName("NoSuchType")).isNull();
    for (int i = 0; i < messageType.getEnumTypes().size(); i++) {
      assertThat(messageType.getEnumTypes().get(i).getIndex()).isEqualTo(i);
    }
  }

  @Test
  public void testFieldDescriptor() throws Exception {
    Descriptor messageType = TestAllTypes.getDescriptor();
    FieldDescriptor primitiveField = messageType.findFieldByName("optional_int32");
    FieldDescriptor enumField = messageType.findFieldByName("optional_nested_enum");
    FieldDescriptor messageField = messageType.findFieldByName("optional_foreign_message");
    FieldDescriptor cordField = messageType.findFieldByName("optional_cord");
    FieldDescriptor extension = UnittestProto.optionalInt32Extension.getDescriptor();
    FieldDescriptor nestedExtension = TestRequired.single.getDescriptor();

    assertThat(primitiveField.getName()).isEqualTo("optional_int32");
    assertThat(primitiveField.getFullName())
        .isEqualTo("protobuf_unittest.TestAllTypes.optional_int32");
    assertThat(primitiveField.getNumber()).isEqualTo(1);
    assertThat(primitiveField.getContainingType()).isEqualTo(messageType);
    assertThat(primitiveField.getFile()).isEqualTo(UnittestProto.getDescriptor());
    assertThat(primitiveField.getType()).isEqualTo(FieldDescriptor.Type.INT32);
    assertThat(primitiveField.getJavaType()).isEqualTo(FieldDescriptor.JavaType.INT);
    assertThat(primitiveField.getOptions())
        .isEqualTo(DescriptorProtos.FieldOptions.getDefaultInstance());
    assertThat(primitiveField.isExtension()).isFalse();
    assertThat(primitiveField.toProto().getName()).isEqualTo("optional_int32");

    assertThat(enumField.getName()).isEqualTo("optional_nested_enum");
    assertThat(enumField.getType()).isEqualTo(FieldDescriptor.Type.ENUM);
    assertThat(enumField.getJavaType()).isEqualTo(FieldDescriptor.JavaType.ENUM);
    assertThat(enumField.getEnumType()).isEqualTo(TestAllTypes.NestedEnum.getDescriptor());

    assertThat(messageField.getName()).isEqualTo("optional_foreign_message");
    assertThat(messageField.getType()).isEqualTo(FieldDescriptor.Type.MESSAGE);
    assertThat(messageField.getJavaType()).isEqualTo(FieldDescriptor.JavaType.MESSAGE);
    assertThat(messageField.getMessageType()).isEqualTo(ForeignMessage.getDescriptor());

    assertThat(cordField.getName()).isEqualTo("optional_cord");
    assertThat(cordField.getType()).isEqualTo(FieldDescriptor.Type.STRING);
    assertThat(cordField.getJavaType()).isEqualTo(FieldDescriptor.JavaType.STRING);
    assertThat(cordField.getOptions().getCtype())
        .isEqualTo(DescriptorProtos.FieldOptions.CType.CORD);

    assertThat(extension.getName()).isEqualTo("optional_int32_extension");
    assertThat(extension.getFullName()).isEqualTo("protobuf_unittest.optional_int32_extension");
    assertThat(extension.getNumber()).isEqualTo(1);
    assertThat(extension.getContainingType()).isEqualTo(TestAllExtensions.getDescriptor());
    assertThat(extension.getFile()).isEqualTo(UnittestProto.getDescriptor());
    assertThat(extension.getType()).isEqualTo(FieldDescriptor.Type.INT32);
    assertThat(extension.getJavaType()).isEqualTo(FieldDescriptor.JavaType.INT);
    assertThat(extension.getOptions())
        .isEqualTo(DescriptorProtos.FieldOptions.getDefaultInstance());
    assertThat(extension.isExtension()).isTrue();
    assertThat(extension.getExtensionScope()).isNull();
    assertThat(extension.toProto().getName()).isEqualTo("optional_int32_extension");

    assertThat(nestedExtension.getName()).isEqualTo("single");
    assertThat(nestedExtension.getFullName()).isEqualTo("protobuf_unittest.TestRequired.single");
    assertThat(nestedExtension.getExtensionScope()).isEqualTo(TestRequired.getDescriptor());
  }

  @Test
  public void testFieldDescriptorLabel() throws Exception {
    FieldDescriptor requiredField = TestRequired.getDescriptor().findFieldByName("a");
    FieldDescriptor optionalField = TestAllTypes.getDescriptor().findFieldByName("optional_int32");
    FieldDescriptor repeatedField = TestAllTypes.getDescriptor().findFieldByName("repeated_int32");

    assertThat(requiredField.isRequired()).isTrue();
    assertThat(requiredField.isRepeated()).isFalse();
    assertThat(optionalField.isRequired()).isFalse();
    assertThat(optionalField.isRepeated()).isFalse();
    assertThat(repeatedField.isRequired()).isFalse();
    assertThat(repeatedField.isRepeated()).isTrue();
  }

  @Test
  public void testFieldDescriptorJsonName() throws Exception {
    FieldDescriptor requiredField = TestRequired.getDescriptor().findFieldByName("a");
    FieldDescriptor optionalField = TestAllTypes.getDescriptor().findFieldByName("optional_int32");
    FieldDescriptor repeatedField = TestAllTypes.getDescriptor().findFieldByName("repeated_int32");
    assertThat(requiredField.getJsonName()).isEqualTo("a");
    assertThat(optionalField.getJsonName()).isEqualTo("optionalInt32");
    assertThat(repeatedField.getJsonName()).isEqualTo("repeatedInt32");
  }

  @Test
  public void testFieldDescriptorDefault() throws Exception {
    Descriptor d = TestAllTypes.getDescriptor();
    assertThat(d.findFieldByName("optional_int32").hasDefaultValue()).isFalse();
    assertThat(d.findFieldByName("optional_int32").getDefaultValue()).isEqualTo(0);
    assertThat(d.findFieldByName("default_int32").hasDefaultValue()).isTrue();
    assertThat(d.findFieldByName("default_int32").getDefaultValue()).isEqualTo(41);

    d = TestExtremeDefaultValues.getDescriptor();
    assertThat(d.findFieldByName("escaped_bytes").getDefaultValue())
        .isEqualTo(
            ByteString.copyFrom(
                "\0\001\007\b\f\n\r\t\013\\\'\"\u00fe".getBytes(Internal.ISO_8859_1)));
    assertThat(d.findFieldByName("large_uint32").getDefaultValue()).isEqualTo(-1);
    assertThat(d.findFieldByName("large_uint64").getDefaultValue()).isEqualTo(-1L);
  }

  @Test
  public void testEnumDescriptor() throws Exception {
    EnumDescriptor enumType = ForeignEnum.getDescriptor();
    EnumDescriptor nestedType = TestAllTypes.NestedEnum.getDescriptor();

    assertThat(enumType.getName()).isEqualTo("ForeignEnum");
    assertThat(enumType.getFullName()).isEqualTo("protobuf_unittest.ForeignEnum");
    assertThat(enumType.getFile()).isEqualTo(UnittestProto.getDescriptor());
    assertThat(enumType.getContainingType()).isNull();
    assertThat(enumType.getOptions()).isEqualTo(DescriptorProtos.EnumOptions.getDefaultInstance());

    assertThat(nestedType.getName()).isEqualTo("NestedEnum");
    assertThat(nestedType.getFullName()).isEqualTo("protobuf_unittest.TestAllTypes.NestedEnum");
    assertThat(nestedType.getFile()).isEqualTo(UnittestProto.getDescriptor());
    assertThat(nestedType.getContainingType()).isEqualTo(TestAllTypes.getDescriptor());

    EnumValueDescriptor value = ForeignEnum.FOREIGN_FOO.getValueDescriptor();
    assertThat(enumType.getValues().get(0)).isEqualTo(value);
    assertThat(value.getName()).isEqualTo("FOREIGN_FOO");
    assertThat(value.toString()).isEqualTo("FOREIGN_FOO");
    assertThat(value.getNumber()).isEqualTo(4);
    assertThat(enumType.findValueByName("FOREIGN_FOO")).isEqualTo(value);
    assertThat(enumType.findValueByNumber(4)).isEqualTo(value);
    assertThat(enumType.findValueByName("NO_SUCH_VALUE")).isNull();
    for (int i = 0; i < enumType.getValues().size(); i++) {
      assertThat(enumType.getValues().get(i).getIndex()).isEqualTo(i);
    }
  }

  @Test
  public void testServiceDescriptor() throws Exception {
    ServiceDescriptor service = TestService.getDescriptor();

    assertThat(service.getName()).isEqualTo("TestService");
    assertThat(service.getFullName()).isEqualTo("protobuf_unittest.TestService");
    assertThat(service.getFile()).isEqualTo(UnittestProto.getDescriptor());


    MethodDescriptor fooMethod = service.getMethods().get(0);
    assertThat(fooMethod.getName()).isEqualTo("Foo");
    assertThat(fooMethod.getInputType()).isEqualTo(UnittestProto.FooRequest.getDescriptor());
    assertThat(fooMethod.getOutputType()).isEqualTo(UnittestProto.FooResponse.getDescriptor());
    assertThat(service.findMethodByName("Foo")).isEqualTo(fooMethod);

    MethodDescriptor barMethod = service.getMethods().get(1);
    assertThat(barMethod.getName()).isEqualTo("Bar");
    assertThat(barMethod.getInputType()).isEqualTo(UnittestProto.BarRequest.getDescriptor());
    assertThat(barMethod.getOutputType()).isEqualTo(UnittestProto.BarResponse.getDescriptor());
    assertThat(service.findMethodByName("Bar")).isEqualTo(barMethod);


    assertThat(service.findMethodByName("NoSuchMethod")).isNull();

    for (int i = 0; i < service.getMethods().size(); i++) {
      assertThat(service.getMethods().get(i).getIndex()).isEqualTo(i);
    }
  }


  @Test
  public void testCustomOptions() throws Exception {
    // Get the descriptor indirectly from a dependent proto class. This is to
    // ensure that when a proto class is loaded, custom options defined in its
    // dependencies are also properly initialized.
    Descriptor descriptor =
        TestCustomOptions.TestMessageWithCustomOptionsContainer.getDescriptor()
            .findFieldByName("field")
            .getMessageType();

    assertThat(descriptor.getOptions().hasExtension(UnittestCustomOptions.messageOpt1)).isTrue();
    assertThat(descriptor.getOptions().getExtension(UnittestCustomOptions.messageOpt1))
        .isEqualTo(Integer.valueOf(-56));

    FieldDescriptor field = descriptor.findFieldByName("field1");
    assertThat(field).isNotNull();

    assertThat(field.getOptions().hasExtension(UnittestCustomOptions.fieldOpt1)).isTrue();
    assertThat(field.getOptions().getExtension(UnittestCustomOptions.fieldOpt1))
        .isEqualTo(Long.valueOf(8765432109L));

    OneofDescriptor oneof = descriptor.getOneofs().get(0);
    assertThat(oneof).isNotNull();

    assertThat(oneof.getOptions().hasExtension(UnittestCustomOptions.oneofOpt1)).isTrue();
    assertThat(oneof.getOptions().getExtension(UnittestCustomOptions.oneofOpt1))
        .isEqualTo(Integer.valueOf(-99));

    EnumDescriptor enumType =
        UnittestCustomOptions.TestMessageWithCustomOptions.AnEnum.getDescriptor();

    assertThat(enumType.getOptions().hasExtension(UnittestCustomOptions.enumOpt1)).isTrue();
    assertThat(enumType.getOptions().getExtension(UnittestCustomOptions.enumOpt1))
        .isEqualTo(Integer.valueOf(-789));

    ServiceDescriptor service = UnittestCustomOptions.TestServiceWithCustomOptions.getDescriptor();

    assertThat(service.getOptions().hasExtension(UnittestCustomOptions.serviceOpt1)).isTrue();
    assertThat(service.getOptions().getExtension(UnittestCustomOptions.serviceOpt1))
        .isEqualTo(Long.valueOf(-9876543210L));

    MethodDescriptor method = service.findMethodByName("Foo");
    assertThat(method).isNotNull();

    assertThat(method.getOptions().hasExtension(UnittestCustomOptions.methodOpt1)).isTrue();
    assertThat(method.getOptions().getExtension(UnittestCustomOptions.methodOpt1))
        .isEqualTo(UnittestCustomOptions.MethodOpt1.METHODOPT1_VAL2);
  }

  /** Test that the FieldDescriptor.Type enum is the same as the WireFormat.FieldType enum. */
  @Test
  public void testFieldTypeTablesMatch() throws Exception {
    FieldDescriptor.Type[] values1 = FieldDescriptor.Type.values();
    WireFormat.FieldType[] values2 = WireFormat.FieldType.values();

    assertThat(values1).hasLength(values2.length);

    for (int i = 0; i < values1.length; i++) {
      assertThat(values1[i].toString()).isEqualTo(values2[i].toString());
    }
  }

  /** Test that the FieldDescriptor.JavaType enum is the same as the WireFormat.JavaType enum. */
  @Test
  public void testJavaTypeTablesMatch() throws Exception {
    FieldDescriptor.JavaType[] values1 = FieldDescriptor.JavaType.values();
    WireFormat.JavaType[] values2 = WireFormat.JavaType.values();

    assertThat(values1).hasLength(values2.length);

    for (int i = 0; i < values1.length; i++) {
      assertThat(values1[i].toString()).isEqualTo(values2[i].toString());
    }
  }

  @Test
  public void testEnormousDescriptor() throws Exception {
    // The descriptor for this file is larger than 64k, yet it did not cause
    // a compiler error due to an over-long string literal.
    assertThat(UnittestEnormousDescriptor.getDescriptor().toProto().getSerializedSize())
        .isGreaterThan(65536);
  }

  /** Tests that the DescriptorValidationException works as intended. */
  @Test
  public void testDescriptorValidatorException() throws Exception {
    FileDescriptorProto fileDescriptorProto =
        FileDescriptorProto.newBuilder()
            .setName("foo.proto")
            .addMessageType(
                DescriptorProto.newBuilder()
                    .setName("Foo")
                    .addField(
                        FieldDescriptorProto.newBuilder()
                            .setLabel(FieldDescriptorProto.Label.LABEL_OPTIONAL)
                            .setType(FieldDescriptorProto.Type.TYPE_INT32)
                            .setName("foo")
                            .setNumber(1)
                            .setDefaultValue("invalid")
                            .build())
                    .build())
            .build();
    try {
      Descriptors.FileDescriptor.buildFrom(fileDescriptorProto, new FileDescriptor[0]);
      assertWithMessage("DescriptorValidationException expected").fail();
    } catch (DescriptorValidationException e) {
      // Expected; check that the error message contains some useful hints
      assertThat(e).hasMessageThat().contains("foo");
      assertThat(e).hasMessageThat().contains("Foo");
      assertThat(e).hasMessageThat().contains("invalid");
      assertThat(e).hasCauseThat().isInstanceOf(NumberFormatException.class);
      assertThat(e).hasCauseThat().hasMessageThat().contains("invalid");
    }
  }

  /**
   * Tests the translate/crosslink for an example where a message field's name and type name are the
   * same.
   */
  @Test
  public void testDescriptorComplexCrosslink() throws Exception {
    FileDescriptorProto fileDescriptorProto =
        FileDescriptorProto.newBuilder()
            .setName("foo.proto")
            .addMessageType(
                DescriptorProto.newBuilder()
                    .setName("Foo")
                    .addField(
                        FieldDescriptorProto.newBuilder()
                            .setLabel(FieldDescriptorProto.Label.LABEL_OPTIONAL)
                            .setType(FieldDescriptorProto.Type.TYPE_INT32)
                            .setName("foo")
                            .setNumber(1)
                            .build())
                    .build())
            .addMessageType(
                DescriptorProto.newBuilder()
                    .setName("Bar")
                    .addField(
                        FieldDescriptorProto.newBuilder()
                            .setLabel(FieldDescriptorProto.Label.LABEL_OPTIONAL)
                            .setTypeName("Foo")
                            .setName("Foo")
                            .setNumber(1)
                            .build())
                    .build())
            .build();
    // translate and crosslink
    FileDescriptor file =
        Descriptors.FileDescriptor.buildFrom(fileDescriptorProto, new FileDescriptor[0]);
    // verify resulting descriptors
    assertThat(file).isNotNull();
    List<Descriptor> msglist = file.getMessageTypes();
    assertThat(msglist).isNotNull();
    assertThat(msglist).hasSize(2);
    boolean barFound = false;
    for (Descriptor desc : msglist) {
      if (desc.getName().equals("Bar")) {
        barFound = true;
        assertThat(desc.getFields()).isNotNull();
        List<FieldDescriptor> fieldlist = desc.getFields();
        assertThat(fieldlist).isNotNull();
        assertThat(fieldlist).hasSize(1);
        assertThat(fieldlist.get(0).getType()).isSameInstanceAs(FieldDescriptor.Type.MESSAGE);
        assertThat(fieldlist.get(0).getMessageType().getName().equals("Foo")).isTrue();
      }
    }
    assertThat(barFound).isTrue();
  }

  @Test
  public void testDependencyOrder() throws Exception {
    FileDescriptorProto fooProto = FileDescriptorProto.newBuilder().setName("foo.proto").build();
    FileDescriptorProto barProto =
        FileDescriptorProto.newBuilder().setName("bar.proto").addDependency("foo.proto").build();
    FileDescriptorProto bazProto =
        FileDescriptorProto.newBuilder()
            .setName("baz.proto")
            .addDependency("foo.proto")
            .addDependency("bar.proto")
            .addPublicDependency(0)
            .addPublicDependency(1)
            .build();
    FileDescriptor fooFile = Descriptors.FileDescriptor.buildFrom(fooProto, new FileDescriptor[0]);
    FileDescriptor barFile =
        Descriptors.FileDescriptor.buildFrom(barProto, new FileDescriptor[] {fooFile});

    // Items in the FileDescriptor array can be in any order.
    Descriptors.FileDescriptor.buildFrom(bazProto, new FileDescriptor[] {fooFile, barFile});
    Descriptors.FileDescriptor.buildFrom(bazProto, new FileDescriptor[] {barFile, fooFile});
  }

  @Test
  public void testInvalidPublicDependency() throws Exception {
    FileDescriptorProto fooProto = FileDescriptorProto.newBuilder().setName("foo.proto").build();
    FileDescriptorProto barProto =
        FileDescriptorProto.newBuilder()
            .setName("boo.proto")
            .addDependency("foo.proto")
            .addPublicDependency(1) // Error, should be 0.
            .build();
    FileDescriptor fooFile = Descriptors.FileDescriptor.buildFrom(fooProto, new FileDescriptor[0]);
    try {
      Descriptors.FileDescriptor.buildFrom(barProto, new FileDescriptor[] {fooFile});
      assertWithMessage("DescriptorValidationException expected").fail();
    } catch (DescriptorValidationException e) {
      assertThat(e).hasMessageThat().contains("Invalid public dependency index.");
    }
  }

  @Test
  public void testUnknownFieldsDenied() throws Exception {
    FileDescriptorProto fooProto =
        FileDescriptorProto.newBuilder()
            .setName("foo.proto")
            .addMessageType(
                DescriptorProto.newBuilder()
                    .setName("Foo")
                    .addField(
                        FieldDescriptorProto.newBuilder()
                            .setLabel(FieldDescriptorProto.Label.LABEL_OPTIONAL)
                            .setTypeName("Bar")
                            .setName("bar")
                            .setNumber(1)))
            .build();

    try {
      Descriptors.FileDescriptor.buildFrom(fooProto, new FileDescriptor[0]);
      assertWithMessage("DescriptorValidationException expected").fail();
    } catch (DescriptorValidationException e) {
      assertThat(e).hasMessageThat().contains("Bar");
      assertThat(e).hasMessageThat().contains("is not defined");
    }
  }

  @Test
  public void testUnknownFieldsAllowed() throws Exception {
    FileDescriptorProto fooProto =
        FileDescriptorProto.newBuilder()
            .setName("foo.proto")
            .addDependency("bar.proto")
            .addMessageType(
                DescriptorProto.newBuilder()
                    .setName("Foo")
                    .addField(
                        FieldDescriptorProto.newBuilder()
                            .setLabel(FieldDescriptorProto.Label.LABEL_OPTIONAL)
                            .setTypeName("Bar")
                            .setName("bar")
                            .setNumber(1)))
            .build();
    Descriptors.FileDescriptor.buildFrom(fooProto, new FileDescriptor[0], true);
  }

  @Test
  public void testHiddenDependency() throws Exception {
    FileDescriptorProto barProto =
        FileDescriptorProto.newBuilder()
            .setName("bar.proto")
            .addMessageType(DescriptorProto.newBuilder().setName("Bar"))
            .build();
    FileDescriptorProto forwardProto =
        FileDescriptorProto.newBuilder()
            .setName("forward.proto")
            .addDependency("bar.proto")
            .build();
    FileDescriptorProto fooProto =
        FileDescriptorProto.newBuilder()
            .setName("foo.proto")
            .addDependency("forward.proto")
            .addMessageType(
                DescriptorProto.newBuilder()
                    .setName("Foo")
                    .addField(
                        FieldDescriptorProto.newBuilder()
                            .setLabel(FieldDescriptorProto.Label.LABEL_OPTIONAL)
                            .setTypeName("Bar")
                            .setName("bar")
                            .setNumber(1)))
            .build();
    FileDescriptor barFile = Descriptors.FileDescriptor.buildFrom(barProto, new FileDescriptor[0]);
    FileDescriptor forwardFile =
        Descriptors.FileDescriptor.buildFrom(forwardProto, new FileDescriptor[] {barFile});

    try {
      Descriptors.FileDescriptor.buildFrom(fooProto, new FileDescriptor[] {forwardFile});
      assertWithMessage("DescriptorValidationException expected").fail();
    } catch (DescriptorValidationException e) {
      assertThat(e).hasMessageThat().contains("Bar");
      assertThat(e).hasMessageThat().contains("is not defined");
    }
  }

  @Test
  public void testPublicDependency() throws Exception {
    FileDescriptorProto barProto =
        FileDescriptorProto.newBuilder()
            .setName("bar.proto")
            .addMessageType(DescriptorProto.newBuilder().setName("Bar"))
            .build();
    FileDescriptorProto forwardProto =
        FileDescriptorProto.newBuilder()
            .setName("forward.proto")
            .addDependency("bar.proto")
            .addPublicDependency(0)
            .build();
    FileDescriptorProto fooProto =
        FileDescriptorProto.newBuilder()
            .setName("foo.proto")
            .addDependency("forward.proto")
            .addMessageType(
                DescriptorProto.newBuilder()
                    .setName("Foo")
                    .addField(
                        FieldDescriptorProto.newBuilder()
                            .setLabel(FieldDescriptorProto.Label.LABEL_OPTIONAL)
                            .setTypeName("Bar")
                            .setName("bar")
                            .setNumber(1)))
            .build();
    FileDescriptor barFile = Descriptors.FileDescriptor.buildFrom(barProto, new FileDescriptor[0]);
    FileDescriptor forwardFile =
        Descriptors.FileDescriptor.buildFrom(forwardProto, new FileDescriptor[] {barFile});
    Descriptors.FileDescriptor.buildFrom(fooProto, new FileDescriptor[] {forwardFile});
  }

  /** Tests the translate/crosslink for an example with a more complex namespace referencing. */
  @Test
  public void testComplexNamespacePublicDependency() throws Exception {
    FileDescriptorProto fooProto =
        FileDescriptorProto.newBuilder()
            .setName("bar.proto")
            .setPackage("a.b.c.d.bar.shared")
            .addEnumType(
                EnumDescriptorProto.newBuilder()
                    .setName("MyEnum")
                    .addValue(EnumValueDescriptorProto.newBuilder().setName("BLAH").setNumber(1)))
            .build();
    FileDescriptorProto barProto =
        FileDescriptorProto.newBuilder()
            .setName("foo.proto")
            .addDependency("bar.proto")
            .setPackage("a.b.c.d.foo.shared")
            .addMessageType(
                DescriptorProto.newBuilder()
                    .setName("MyMessage")
                    .addField(
                        FieldDescriptorProto.newBuilder()
                            .setLabel(FieldDescriptorProto.Label.LABEL_REPEATED)
                            .setTypeName("bar.shared.MyEnum")
                            .setName("MyField")
                            .setNumber(1)))
            .build();
    // translate and crosslink
    FileDescriptor fooFile = Descriptors.FileDescriptor.buildFrom(fooProto, new FileDescriptor[0]);
    FileDescriptor barFile =
        Descriptors.FileDescriptor.buildFrom(barProto, new FileDescriptor[] {fooFile});
    // verify resulting descriptors
    assertThat(barFile).isNotNull();
    List<Descriptor> msglist = barFile.getMessageTypes();
    assertThat(msglist).isNotNull();
    assertThat(msglist).hasSize(1);
    Descriptor desc = msglist.get(0);
    if (desc.getName().equals("MyMessage")) {
      assertThat(desc.getFields()).isNotNull();
      List<FieldDescriptor> fieldlist = desc.getFields();
      assertThat(fieldlist).isNotNull();
      assertThat(fieldlist).hasSize(1);
      FieldDescriptor field = fieldlist.get(0);
      assertThat(field.getType()).isSameInstanceAs(FieldDescriptor.Type.ENUM);
      assertThat(field.getEnumType().getName().equals("MyEnum")).isTrue();
      assertThat(field.getEnumType().getFile().getName().equals("bar.proto")).isTrue();
      assertThat(field.getEnumType().getFile().getPackage().equals("a.b.c.d.bar.shared")).isTrue();
    }
  }

  @Test
  public void testOneofDescriptor() throws Exception {
    Descriptor messageType = TestAllTypes.getDescriptor();
    FieldDescriptor field = messageType.findFieldByName("oneof_nested_message");
    OneofDescriptor oneofDescriptor = field.getContainingOneof();
    assertThat(oneofDescriptor).isNotNull();
    assertThat(messageType.getOneofs().get(0)).isSameInstanceAs(oneofDescriptor);
    assertThat(oneofDescriptor.getName()).isEqualTo("oneof_field");

    assertThat(oneofDescriptor.getFieldCount()).isEqualTo(4);
    assertThat(field).isSameInstanceAs(oneofDescriptor.getField(1));

    assertThat(oneofDescriptor.getFields()).hasSize(4);
    assertThat(field).isEqualTo(oneofDescriptor.getFields().get(1));
  }

  @Test
  public void testMessageDescriptorExtensions() throws Exception {
    assertThat(TestAllTypes.getDescriptor().isExtendable()).isFalse();
    assertThat(TestAllExtensions.getDescriptor().isExtendable()).isTrue();
    assertThat(TestMultipleExtensionRanges.getDescriptor().isExtendable()).isTrue();

    assertThat(TestAllTypes.getDescriptor().isExtensionNumber(3)).isFalse();
    assertThat(TestAllExtensions.getDescriptor().isExtensionNumber(3)).isTrue();
    assertThat(TestMultipleExtensionRanges.getDescriptor().isExtensionNumber(42)).isTrue();
    assertThat(TestMultipleExtensionRanges.getDescriptor().isExtensionNumber(43)).isFalse();
    assertThat(TestMultipleExtensionRanges.getDescriptor().isExtensionNumber(4142)).isFalse();
    assertThat(TestMultipleExtensionRanges.getDescriptor().isExtensionNumber(4143)).isTrue();
  }

  @Test
  public void testReservedFields() {
    Descriptor d = TestReservedFields.getDescriptor();
    assertThat(d.isReservedNumber(2)).isTrue();
    assertThat(d.isReservedNumber(8)).isFalse();
    assertThat(d.isReservedNumber(9)).isTrue();
    assertThat(d.isReservedNumber(10)).isTrue();
    assertThat(d.isReservedNumber(11)).isTrue();
    assertThat(d.isReservedNumber(12)).isFalse();
    assertThat(d.isReservedName("foo")).isFalse();
    assertThat(d.isReservedName("bar")).isTrue();
    assertThat(d.isReservedName("baz")).isTrue();
  }

  @Test
  public void testToString() {
    assertThat(
            UnittestProto.TestAllTypes.getDescriptor()
                .findFieldByNumber(UnittestProto.TestAllTypes.OPTIONAL_UINT64_FIELD_NUMBER)
                .toString())
        .isEqualTo("protobuf_unittest.TestAllTypes.optional_uint64");
  }

  @Test
  public void testPackedEnumField() throws Exception {
    FileDescriptorProto fileDescriptorProto =
        FileDescriptorProto.newBuilder()
            .setName("foo.proto")
            .addEnumType(
                EnumDescriptorProto.newBuilder()
                    .setName("Enum")
                    .addValue(
                        EnumValueDescriptorProto.newBuilder().setName("FOO").setNumber(1).build())
                    .build())
            .addMessageType(
                DescriptorProto.newBuilder()
                    .setName("Message")
                    .addField(
                        FieldDescriptorProto.newBuilder()
                            .setName("foo")
                            .setTypeName("Enum")
                            .setNumber(1)
                            .setLabel(FieldDescriptorProto.Label.LABEL_REPEATED)
                            .setOptions(
                                DescriptorProtos.FieldOptions.newBuilder().setPacked(true).build())
                            .build())
                    .build())
            .build();
    Descriptors.FileDescriptor.buildFrom(fileDescriptorProto, new FileDescriptor[0]);
  }

  @Test
  public void testFieldJsonName() throws Exception {
    Descriptor d = TestJsonName.getDescriptor();
    assertThat(d.getFields()).hasSize(7);
    assertThat(d.getFields().get(0).getJsonName()).isEqualTo("fieldName1");
    assertThat(d.getFields().get(1).getJsonName()).isEqualTo("fieldName2");
    assertThat(d.getFields().get(2).getJsonName()).isEqualTo("FieldName3");
    assertThat(d.getFields().get(3).getJsonName()).isEqualTo("FieldName4");
    assertThat(d.getFields().get(4).getJsonName()).isEqualTo("FIELDNAME5");
    assertThat(d.getFields().get(5).getJsonName()).isEqualTo("@type");
    assertThat(d.getFields().get(6).getJsonName()).isEqualTo("fieldname7");
  }

  @Test
  public void testExtensionRenamesKeywords() {
    assertThat(NonNestedExtension.if_).isInstanceOf(GeneratedMessage.GeneratedExtension.class);
    assertThat(NestedExtension.MyNestedExtension.default_)
        .isInstanceOf(GeneratedMessage.GeneratedExtension.class);

    NonNestedExtension.MessageToBeExtended msg =
        NonNestedExtension.MessageToBeExtended.newBuilder()
            .setExtension(NonNestedExtension.if_, "!fi")
            .build();
    assertThat(msg.getExtension(NonNestedExtension.if_)).isEqualTo("!fi");

    msg =
        NonNestedExtension.MessageToBeExtended.newBuilder()
            .setExtension(NestedExtension.MyNestedExtension.default_, 8)
            .build();
    assertThat(msg.getExtension(NestedExtension.MyNestedExtension.default_).intValue())
        .isEqualTo(8);
  }

  @Test
  public void testDefaultDescriptorExtensionRange() throws Exception {
    assertThat(new Descriptor("default").isExtensionNumber(1)).isTrue();
  }
}
