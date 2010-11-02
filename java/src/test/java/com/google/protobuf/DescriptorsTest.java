// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
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

package com.google.protobuf;

import com.google.protobuf.DescriptorProtos.DescriptorProto;
import com.google.protobuf.DescriptorProtos.FieldDescriptorProto;
import com.google.protobuf.DescriptorProtos.FileDescriptorProto;
import com.google.protobuf.Descriptors.DescriptorValidationException;
import com.google.protobuf.Descriptors.FileDescriptor;
import com.google.protobuf.Descriptors.Descriptor;
import com.google.protobuf.Descriptors.FieldDescriptor;
import com.google.protobuf.Descriptors.EnumDescriptor;
import com.google.protobuf.Descriptors.EnumValueDescriptor;
import com.google.protobuf.Descriptors.ServiceDescriptor;
import com.google.protobuf.Descriptors.MethodDescriptor;

import com.google.protobuf.test.UnittestImport;
import com.google.protobuf.test.UnittestImport.ImportEnum;
import com.google.protobuf.test.UnittestImport.ImportMessage;
import protobuf_unittest.UnittestProto;
import protobuf_unittest.UnittestProto.ForeignEnum;
import protobuf_unittest.UnittestProto.ForeignMessage;
import protobuf_unittest.UnittestProto.TestAllTypes;
import protobuf_unittest.UnittestProto.TestAllExtensions;
import protobuf_unittest.UnittestProto.TestExtremeDefaultValues;
import protobuf_unittest.UnittestProto.TestRequired;
import protobuf_unittest.UnittestProto.TestService;
import protobuf_unittest.UnittestCustomOptions;


import junit.framework.TestCase;

import java.util.Arrays;
import java.util.Collections;

/**
 * Unit test for {@link Descriptors}.
 *
 * @author kenton@google.com Kenton Varda
 */
public class DescriptorsTest extends TestCase {

  // Regression test for bug where referencing a FieldDescriptor.Type value
  // before a FieldDescriptorProto.Type value would yield a
  // ExceptionInInitializerError.
  @SuppressWarnings("unused")
  private static final Object STATIC_INIT_TEST = FieldDescriptor.Type.BOOL;

  public void testFieldTypeEnumMapping() throws Exception {
    assertEquals(FieldDescriptor.Type.values().length,
        FieldDescriptorProto.Type.values().length);
    for (FieldDescriptor.Type type : FieldDescriptor.Type.values()) {
      FieldDescriptorProto.Type protoType = type.toProto();
      assertEquals("TYPE_" + type.name(), protoType.name());
      assertEquals(type, FieldDescriptor.Type.valueOf(protoType));
    }
  }

  public void testFileDescriptor() throws Exception {
    FileDescriptor file = UnittestProto.getDescriptor();

    assertEquals("google/protobuf/unittest.proto", file.getName());
    assertEquals("protobuf_unittest", file.getPackage());

    assertEquals("UnittestProto", file.getOptions().getJavaOuterClassname());
    assertEquals("google/protobuf/unittest.proto",
                 file.toProto().getName());

    assertEquals(Arrays.asList(UnittestImport.getDescriptor()),
                 file.getDependencies());

    Descriptor messageType = TestAllTypes.getDescriptor();
    assertEquals(messageType, file.getMessageTypes().get(0));
    assertEquals(messageType, file.findMessageTypeByName("TestAllTypes"));
    assertNull(file.findMessageTypeByName("NoSuchType"));
    assertNull(file.findMessageTypeByName("protobuf_unittest.TestAllTypes"));
    for (int i = 0; i < file.getMessageTypes().size(); i++) {
      assertEquals(i, file.getMessageTypes().get(i).getIndex());
    }

    EnumDescriptor enumType = ForeignEnum.getDescriptor();
    assertEquals(enumType, file.getEnumTypes().get(0));
    assertEquals(enumType, file.findEnumTypeByName("ForeignEnum"));
    assertNull(file.findEnumTypeByName("NoSuchType"));
    assertNull(file.findEnumTypeByName("protobuf_unittest.ForeignEnum"));
    assertEquals(Arrays.asList(ImportEnum.getDescriptor()),
                 UnittestImport.getDescriptor().getEnumTypes());
    for (int i = 0; i < file.getEnumTypes().size(); i++) {
      assertEquals(i, file.getEnumTypes().get(i).getIndex());
    }

    ServiceDescriptor service = TestService.getDescriptor();
    assertEquals(service, file.getServices().get(0));
    assertEquals(service, file.findServiceByName("TestService"));
    assertNull(file.findServiceByName("NoSuchType"));
    assertNull(file.findServiceByName("protobuf_unittest.TestService"));
    assertEquals(Collections.emptyList(),
                 UnittestImport.getDescriptor().getServices());
    for (int i = 0; i < file.getServices().size(); i++) {
      assertEquals(i, file.getServices().get(i).getIndex());
    }

    FieldDescriptor extension =
      UnittestProto.optionalInt32Extension.getDescriptor();
    assertEquals(extension, file.getExtensions().get(0));
    assertEquals(extension,
                 file.findExtensionByName("optional_int32_extension"));
    assertNull(file.findExtensionByName("no_such_ext"));
    assertNull(file.findExtensionByName(
      "protobuf_unittest.optional_int32_extension"));
    assertEquals(Collections.emptyList(),
                 UnittestImport.getDescriptor().getExtensions());
    for (int i = 0; i < file.getExtensions().size(); i++) {
      assertEquals(i, file.getExtensions().get(i).getIndex());
    }
  }

  public void testDescriptor() throws Exception {
    Descriptor messageType = TestAllTypes.getDescriptor();
    Descriptor nestedType = TestAllTypes.NestedMessage.getDescriptor();

    assertEquals("TestAllTypes", messageType.getName());
    assertEquals("protobuf_unittest.TestAllTypes", messageType.getFullName());
    assertEquals(UnittestProto.getDescriptor(), messageType.getFile());
    assertNull(messageType.getContainingType());
    assertEquals(DescriptorProtos.MessageOptions.getDefaultInstance(),
                 messageType.getOptions());
    assertEquals("TestAllTypes", messageType.toProto().getName());

    assertEquals("NestedMessage", nestedType.getName());
    assertEquals("protobuf_unittest.TestAllTypes.NestedMessage",
                 nestedType.getFullName());
    assertEquals(UnittestProto.getDescriptor(), nestedType.getFile());
    assertEquals(messageType, nestedType.getContainingType());

    FieldDescriptor field = messageType.getFields().get(0);
    assertEquals("optional_int32", field.getName());
    assertEquals(field, messageType.findFieldByName("optional_int32"));
    assertNull(messageType.findFieldByName("no_such_field"));
    assertEquals(field, messageType.findFieldByNumber(1));
    assertNull(messageType.findFieldByNumber(571283));
    for (int i = 0; i < messageType.getFields().size(); i++) {
      assertEquals(i, messageType.getFields().get(i).getIndex());
    }

    assertEquals(nestedType, messageType.getNestedTypes().get(0));
    assertEquals(nestedType, messageType.findNestedTypeByName("NestedMessage"));
    assertNull(messageType.findNestedTypeByName("NoSuchType"));
    for (int i = 0; i < messageType.getNestedTypes().size(); i++) {
      assertEquals(i, messageType.getNestedTypes().get(i).getIndex());
    }

    EnumDescriptor enumType = TestAllTypes.NestedEnum.getDescriptor();
    assertEquals(enumType, messageType.getEnumTypes().get(0));
    assertEquals(enumType, messageType.findEnumTypeByName("NestedEnum"));
    assertNull(messageType.findEnumTypeByName("NoSuchType"));
    for (int i = 0; i < messageType.getEnumTypes().size(); i++) {
      assertEquals(i, messageType.getEnumTypes().get(i).getIndex());
    }
  }

  public void testFieldDescriptor() throws Exception {
    Descriptor messageType = TestAllTypes.getDescriptor();
    FieldDescriptor primitiveField =
      messageType.findFieldByName("optional_int32");
    FieldDescriptor enumField =
      messageType.findFieldByName("optional_nested_enum");
    FieldDescriptor messageField =
      messageType.findFieldByName("optional_foreign_message");
    FieldDescriptor cordField =
      messageType.findFieldByName("optional_cord");
    FieldDescriptor extension =
      UnittestProto.optionalInt32Extension.getDescriptor();
    FieldDescriptor nestedExtension = TestRequired.single.getDescriptor();

    assertEquals("optional_int32", primitiveField.getName());
    assertEquals("protobuf_unittest.TestAllTypes.optional_int32",
                 primitiveField.getFullName());
    assertEquals(1, primitiveField.getNumber());
    assertEquals(messageType, primitiveField.getContainingType());
    assertEquals(UnittestProto.getDescriptor(), primitiveField.getFile());
    assertEquals(FieldDescriptor.Type.INT32, primitiveField.getType());
    assertEquals(FieldDescriptor.JavaType.INT, primitiveField.getJavaType());
    assertEquals(DescriptorProtos.FieldOptions.getDefaultInstance(),
                 primitiveField.getOptions());
    assertFalse(primitiveField.isExtension());
    assertEquals("optional_int32", primitiveField.toProto().getName());

    assertEquals("optional_nested_enum", enumField.getName());
    assertEquals(FieldDescriptor.Type.ENUM, enumField.getType());
    assertEquals(FieldDescriptor.JavaType.ENUM, enumField.getJavaType());
    assertEquals(TestAllTypes.NestedEnum.getDescriptor(),
                 enumField.getEnumType());

    assertEquals("optional_foreign_message", messageField.getName());
    assertEquals(FieldDescriptor.Type.MESSAGE, messageField.getType());
    assertEquals(FieldDescriptor.JavaType.MESSAGE, messageField.getJavaType());
    assertEquals(ForeignMessage.getDescriptor(), messageField.getMessageType());

    assertEquals("optional_cord", cordField.getName());
    assertEquals(FieldDescriptor.Type.STRING, cordField.getType());
    assertEquals(FieldDescriptor.JavaType.STRING, cordField.getJavaType());
    assertEquals(DescriptorProtos.FieldOptions.CType.CORD,
                 cordField.getOptions().getCtype());

    assertEquals("optional_int32_extension", extension.getName());
    assertEquals("protobuf_unittest.optional_int32_extension",
                 extension.getFullName());
    assertEquals(1, extension.getNumber());
    assertEquals(TestAllExtensions.getDescriptor(),
                 extension.getContainingType());
    assertEquals(UnittestProto.getDescriptor(), extension.getFile());
    assertEquals(FieldDescriptor.Type.INT32, extension.getType());
    assertEquals(FieldDescriptor.JavaType.INT, extension.getJavaType());
    assertEquals(DescriptorProtos.FieldOptions.getDefaultInstance(),
                 extension.getOptions());
    assertTrue(extension.isExtension());
    assertEquals(null, extension.getExtensionScope());
    assertEquals("optional_int32_extension", extension.toProto().getName());

    assertEquals("single", nestedExtension.getName());
    assertEquals("protobuf_unittest.TestRequired.single",
                 nestedExtension.getFullName());
    assertEquals(TestRequired.getDescriptor(),
                 nestedExtension.getExtensionScope());
  }

  public void testFieldDescriptorLabel() throws Exception {
    FieldDescriptor requiredField =
      TestRequired.getDescriptor().findFieldByName("a");
    FieldDescriptor optionalField =
      TestAllTypes.getDescriptor().findFieldByName("optional_int32");
    FieldDescriptor repeatedField =
      TestAllTypes.getDescriptor().findFieldByName("repeated_int32");

    assertTrue(requiredField.isRequired());
    assertFalse(requiredField.isRepeated());
    assertFalse(optionalField.isRequired());
    assertFalse(optionalField.isRepeated());
    assertFalse(repeatedField.isRequired());
    assertTrue(repeatedField.isRepeated());
  }

  public void testFieldDescriptorDefault() throws Exception {
    Descriptor d = TestAllTypes.getDescriptor();
    assertFalse(d.findFieldByName("optional_int32").hasDefaultValue());
    assertEquals(0, d.findFieldByName("optional_int32").getDefaultValue());
    assertTrue(d.findFieldByName("default_int32").hasDefaultValue());
    assertEquals(41, d.findFieldByName("default_int32").getDefaultValue());

    d = TestExtremeDefaultValues.getDescriptor();
    assertEquals(
      ByteString.copyFrom(
        "\0\001\007\b\f\n\r\t\013\\\'\"\u00fe".getBytes("ISO-8859-1")),
      d.findFieldByName("escaped_bytes").getDefaultValue());
    assertEquals(-1, d.findFieldByName("large_uint32").getDefaultValue());
    assertEquals(-1L, d.findFieldByName("large_uint64").getDefaultValue());
  }

  public void testEnumDescriptor() throws Exception {
    EnumDescriptor enumType = ForeignEnum.getDescriptor();
    EnumDescriptor nestedType = TestAllTypes.NestedEnum.getDescriptor();

    assertEquals("ForeignEnum", enumType.getName());
    assertEquals("protobuf_unittest.ForeignEnum", enumType.getFullName());
    assertEquals(UnittestProto.getDescriptor(), enumType.getFile());
    assertNull(enumType.getContainingType());
    assertEquals(DescriptorProtos.EnumOptions.getDefaultInstance(),
                 enumType.getOptions());

    assertEquals("NestedEnum", nestedType.getName());
    assertEquals("protobuf_unittest.TestAllTypes.NestedEnum",
                 nestedType.getFullName());
    assertEquals(UnittestProto.getDescriptor(), nestedType.getFile());
    assertEquals(TestAllTypes.getDescriptor(), nestedType.getContainingType());

    EnumValueDescriptor value = ForeignEnum.FOREIGN_FOO.getValueDescriptor();
    assertEquals(value, enumType.getValues().get(0));
    assertEquals("FOREIGN_FOO", value.getName());
    assertEquals(4, value.getNumber());
    assertEquals(value, enumType.findValueByName("FOREIGN_FOO"));
    assertEquals(value, enumType.findValueByNumber(4));
    assertNull(enumType.findValueByName("NO_SUCH_VALUE"));
    for (int i = 0; i < enumType.getValues().size(); i++) {
      assertEquals(i, enumType.getValues().get(i).getIndex());
    }
  }

  public void testServiceDescriptor() throws Exception {
    ServiceDescriptor service = TestService.getDescriptor();

    assertEquals("TestService", service.getName());
    assertEquals("protobuf_unittest.TestService", service.getFullName());
    assertEquals(UnittestProto.getDescriptor(), service.getFile());

    assertEquals(2, service.getMethods().size());

    MethodDescriptor fooMethod = service.getMethods().get(0);
    assertEquals("Foo", fooMethod.getName());
    assertEquals(UnittestProto.FooRequest.getDescriptor(),
                 fooMethod.getInputType());
    assertEquals(UnittestProto.FooResponse.getDescriptor(),
                 fooMethod.getOutputType());
    assertEquals(fooMethod, service.findMethodByName("Foo"));

    MethodDescriptor barMethod = service.getMethods().get(1);
    assertEquals("Bar", barMethod.getName());
    assertEquals(UnittestProto.BarRequest.getDescriptor(),
                 barMethod.getInputType());
    assertEquals(UnittestProto.BarResponse.getDescriptor(),
                 barMethod.getOutputType());
    assertEquals(barMethod, service.findMethodByName("Bar"));

    assertNull(service.findMethodByName("NoSuchMethod"));

    for (int i = 0; i < service.getMethods().size(); i++) {
      assertEquals(i, service.getMethods().get(i).getIndex());
    }
  }


  public void testCustomOptions() throws Exception {
    Descriptor descriptor =
      UnittestCustomOptions.TestMessageWithCustomOptions.getDescriptor();

    assertTrue(
      descriptor.getOptions().hasExtension(UnittestCustomOptions.messageOpt1));
    assertEquals(Integer.valueOf(-56),
      descriptor.getOptions().getExtension(UnittestCustomOptions.messageOpt1));

    FieldDescriptor field = descriptor.findFieldByName("field1");
    assertNotNull(field);

    assertTrue(
      field.getOptions().hasExtension(UnittestCustomOptions.fieldOpt1));
    assertEquals(Long.valueOf(8765432109L),
      field.getOptions().getExtension(UnittestCustomOptions.fieldOpt1));

    EnumDescriptor enumType =
      UnittestCustomOptions.TestMessageWithCustomOptions.AnEnum.getDescriptor();

    assertTrue(
      enumType.getOptions().hasExtension(UnittestCustomOptions.enumOpt1));
    assertEquals(Integer.valueOf(-789),
      enumType.getOptions().getExtension(UnittestCustomOptions.enumOpt1));

    ServiceDescriptor service =
      UnittestCustomOptions.TestServiceWithCustomOptions.getDescriptor();

    assertTrue(
      service.getOptions().hasExtension(UnittestCustomOptions.serviceOpt1));
    assertEquals(Long.valueOf(-9876543210L),
      service.getOptions().getExtension(UnittestCustomOptions.serviceOpt1));

    MethodDescriptor method = service.findMethodByName("Foo");
    assertNotNull(method);

    assertTrue(
      method.getOptions().hasExtension(UnittestCustomOptions.methodOpt1));
    assertEquals(UnittestCustomOptions.MethodOpt1.METHODOPT1_VAL2,
      method.getOptions().getExtension(UnittestCustomOptions.methodOpt1));
  }

  /**
   * Test that the FieldDescriptor.Type enum is the same as the
   * WireFormat.FieldType enum.
   */
  public void testFieldTypeTablesMatch() throws Exception {
    FieldDescriptor.Type[] values1 = FieldDescriptor.Type.values();
    WireFormat.FieldType[] values2 = WireFormat.FieldType.values();

    assertEquals(values1.length, values2.length);

    for (int i = 0; i < values1.length; i++) {
      assertEquals(values1[i].toString(), values2[i].toString());
    }
  }

  /**
   * Test that the FieldDescriptor.JavaType enum is the same as the
   * WireFormat.JavaType enum.
   */
  public void testJavaTypeTablesMatch() throws Exception {
    FieldDescriptor.JavaType[] values1 = FieldDescriptor.JavaType.values();
    WireFormat.JavaType[] values2 = WireFormat.JavaType.values();

    assertEquals(values1.length, values2.length);

    for (int i = 0; i < values1.length; i++) {
      assertEquals(values1[i].toString(), values2[i].toString());
    }
  }

  public void testEnormousDescriptor() throws Exception {
    // The descriptor for this file is larger than 64k, yet it did not cause
    // a compiler error due to an over-long string literal.
    assertTrue(
        UnittestEnormousDescriptor.getDescriptor()
          .toProto().getSerializedSize() > 65536);
  }
  
  /**
   * Tests that the DescriptorValidationException works as intended.
   */
  public void testDescriptorValidatorException() throws Exception {
    FileDescriptorProto fileDescriptorProto = FileDescriptorProto.newBuilder()
      .setName("foo.proto")
      .addMessageType(DescriptorProto.newBuilder()
      .setName("Foo")
        .addField(FieldDescriptorProto.newBuilder()
          .setLabel(FieldDescriptorProto.Label.LABEL_OPTIONAL)
          .setType(FieldDescriptorProto.Type.TYPE_INT32)
          .setName("foo")
          .setNumber(1)
          .setDefaultValue("invalid")
          .build())
        .build())
      .build();
    try {
      Descriptors.FileDescriptor.buildFrom(fileDescriptorProto, 
          new FileDescriptor[0]);
      fail("DescriptorValidationException expected");
    } catch (DescriptorValidationException e) {
      // Expected; check that the error message contains some useful hints
      assertTrue(e.getMessage().indexOf("foo") != -1);
      assertTrue(e.getMessage().indexOf("Foo") != -1);
      assertTrue(e.getMessage().indexOf("invalid") != -1);
      assertTrue(e.getCause() instanceof NumberFormatException);
      assertTrue(e.getCause().getMessage().indexOf("invalid") != -1);
    }
  }
}
