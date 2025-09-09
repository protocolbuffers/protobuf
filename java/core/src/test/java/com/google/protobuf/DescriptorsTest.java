// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

package com.google.protobuf;

import static com.google.common.truth.Truth.assertThat;
import static com.google.common.truth.Truth.assertWithMessage;
import static org.junit.Assert.assertThrows;

import com.google.protobuf.DescriptorProtos.DescriptorProto;
import com.google.protobuf.DescriptorProtos.DescriptorProto.ExtensionRange;
import com.google.protobuf.DescriptorProtos.Edition;
import com.google.protobuf.DescriptorProtos.EnumDescriptorProto;
import com.google.protobuf.DescriptorProtos.EnumValueDescriptorProto;
import com.google.protobuf.DescriptorProtos.FeatureSetDefaults;
import com.google.protobuf.DescriptorProtos.FeatureSetDefaults.FeatureSetEditionDefault;
import com.google.protobuf.DescriptorProtos.FieldDescriptorProto;
import com.google.protobuf.DescriptorProtos.FieldOptions;
import com.google.protobuf.DescriptorProtos.FileDescriptorProto;
import com.google.protobuf.DescriptorProtos.FileOptions;
import com.google.protobuf.DescriptorProtos.MethodDescriptorProto;
import com.google.protobuf.DescriptorProtos.OneofDescriptorProto;
import com.google.protobuf.DescriptorProtos.ServiceDescriptorProto;
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
import legacy_features_unittest.UnittestLegacyFeatures;
import pb.UnittestFeatures;
import proto2_unittest.TestCustomOptions;
import proto2_unittest.UnittestCustomOptions;
import proto2_unittest.UnittestProto;
import proto2_unittest.UnittestProto.ForeignEnum;
import proto2_unittest.UnittestProto.ForeignMessage;
import proto2_unittest.UnittestProto.TestAllExtensions;
import proto2_unittest.UnittestProto.TestAllTypes;
import proto2_unittest.UnittestProto.TestExtremeDefaultValues;
import proto2_unittest.UnittestProto.TestJsonName;
import proto2_unittest.UnittestProto.TestMultipleExtensionRanges;
import proto2_unittest.UnittestProto.TestRequired;
import proto2_unittest.UnittestProto.TestReservedEnumFields;
import proto2_unittest.UnittestProto.TestReservedFields;
import proto2_unittest.UnittestProto.TestService;
import proto2_unittest.UnittestRetention;
import protobuf_unittest.UnittestProto3Extensions.Proto3FileExtensions;
import java.util.Collections;
import java.util.List;
import org.junit.Before;
import org.junit.Test;
import org.junit.experimental.runners.Enclosed;
import org.junit.runner.RunWith;

import proto2_unittest.NestedExtension;
import proto2_unittest.NonNestedExtension;

/** Unit test for {@link Descriptors}. */
@RunWith(Enclosed.class)
public class DescriptorsTest {

  public static class GeneralDescriptorsTest {
    // Regression test for bug where referencing a FieldDescriptor.Type value
    // before a FieldDescriptorProto.Type value would yield a
    // ExceptionInInitializerError.
    @SuppressWarnings("unused")
    private static final Object STATIC_INIT_TEST = FieldDescriptor.Type.BOOL;

    @Test
    public void testFieldTypeEnumMapping() throws Exception {
      assertThat(FieldDescriptor.Type.values())
          .hasLength(FieldDescriptorProto.Type.values().length);
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
      assertThat(file.getPackage()).isEqualTo("proto2_unittest");
      assertThat(file.getOptions().getJavaOuterClassname()).isEqualTo("UnittestProto");
      assertThat(file.toProto().getName()).isEqualTo("google/protobuf/unittest.proto");

      assertThat(file.getDependencies()).containsExactly(UnittestImport.getDescriptor());

      Descriptor messageType = TestAllTypes.getDescriptor();
      assertThat(file.getMessageTypes().get(0)).isEqualTo(messageType);
      assertThat(file.findMessageTypeByName("TestAllTypes")).isEqualTo(messageType);
      assertThat(file.findMessageTypeByName("NoSuchType")).isNull();
      assertThat(file.findMessageTypeByName("proto2_unittest.TestAllTypes")).isNull();
      for (int i = 0; i < file.getMessageTypes().size(); i++) {
        assertThat(file.getMessageTypes().get(i).getIndex()).isEqualTo(i);
      }

      EnumDescriptor enumType = ForeignEnum.getDescriptor();
      assertThat(file.getEnumTypes().get(0)).isEqualTo(enumType);
      assertThat(file.findEnumTypeByName("ForeignEnum")).isEqualTo(enumType);
      assertThat(file.findEnumTypeByName("NoSuchType")).isNull();
      assertThat(file.findEnumTypeByName("proto2_unittest.ForeignEnum")).isNull();
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
      assertThat(file.findServiceByName("proto2_unittest.TestService")).isNull();
      assertThat(UnittestImport.getDescriptor().getServices()).isEqualTo(Collections.emptyList());
      for (int i = 0; i < file.getServices().size(); i++) {
        assertThat(file.getServices().get(i).getIndex()).isEqualTo(i);
      }

      FieldDescriptor extension = UnittestProto.optionalInt32Extension.getDescriptor();
      assertThat(file.getExtensions().get(0)).isEqualTo(extension);
      assertThat(file.findExtensionByName("optional_int32_extension")).isEqualTo(extension);
      assertThat(file.findExtensionByName("no_such_ext")).isNull();
      assertThat(file.findExtensionByName("proto2_unittest.optional_int32_extension")).isNull();
      assertThat(UnittestImport.getDescriptor().getExtensions()).isEqualTo(Collections.emptyList());
      for (int i = 0; i < file.getExtensions().size(); i++) {
        assertThat(file.getExtensions().get(i).getIndex()).isEqualTo(i);
      }
    }

    @Test
    public void testFileDescriptorGetEdition() throws Exception {
      FileDescriptorProto proto2 = FileDescriptorProto.newBuilder().setSyntax("proto2").build();
      FileDescriptor file2 = Descriptors.FileDescriptor.buildFrom(proto2, new FileDescriptor[0]);
      assertThat(file2.getEdition()).isEqualTo(Edition.EDITION_PROTO2);

      FileDescriptorProto proto3 = FileDescriptorProto.newBuilder().setSyntax("proto3").build();
      FileDescriptor file3 = Descriptors.FileDescriptor.buildFrom(proto3, new FileDescriptor[0]);
      assertThat(file3.getEdition()).isEqualTo(Edition.EDITION_PROTO3);

      FileDescriptorProto protoEdition =
          FileDescriptorProto.newBuilder()
              .setSyntax("editions")
              .setEdition(Edition.EDITION_2023)
              .build();
      FileDescriptor fileEdition =
          Descriptors.FileDescriptor.buildFrom(protoEdition, new FileDescriptor[0]);
      assertThat(fileEdition.getEdition()).isEqualTo(Edition.EDITION_2023);

      FileDescriptorProto protoMissingEdition =
          FileDescriptorProto.newBuilder().setSyntax("editions").build();
      IllegalArgumentException exception =
          assertThrows(
              IllegalArgumentException.class,
              () ->
                  Descriptors.FileDescriptor.buildFrom(protoMissingEdition, new FileDescriptor[0]));
      assertThat(exception)
          .hasMessageThat()
          .contains("Edition EDITION_UNKNOWN is lower than the minimum supported edition");
    }

    @Test
    public void testFileDescriptorCopyHeadingTo() throws Exception {
      FileDescriptorProto.Builder protoBuilder =
          FileDescriptorProto.newBuilder()
              .setName("foo.proto")
              .setPackage("foo.bar.baz")
              .setSyntax("proto2")
              .setOptions(FileOptions.newBuilder().setJavaPackage("foo.bar.baz").build())
              // Won't be copied.
              .addMessageType(DescriptorProto.newBuilder().setName("Foo").build());
      FileDescriptor file2 =
          Descriptors.FileDescriptor.buildFrom(protoBuilder.build(), new FileDescriptor[0]);
      FileDescriptorProto.Builder protoBuilder2 = FileDescriptorProto.newBuilder();
      file2.copyHeadingTo(protoBuilder2);
      FileDescriptorProto toProto2 = protoBuilder2.build();
      assertThat(toProto2.getName()).isEqualTo("foo.proto");
      assertThat(toProto2.getPackage()).isEqualTo("foo.bar.baz");
      assertThat(toProto2.getSyntax()).isEqualTo("proto2");
      assertThat(toProto2.getOptions().getJavaPackage()).isEqualTo("foo.bar.baz");
      assertThat(toProto2.getMessageTypeList()).isEmpty();

      protoBuilder.setSyntax("proto3");
      FileDescriptor file3 =
          Descriptors.FileDescriptor.buildFrom(protoBuilder.build(), new FileDescriptor[0]);
      FileDescriptorProto.Builder protoBuilder3 = FileDescriptorProto.newBuilder();
      file3.copyHeadingTo(protoBuilder3);
      FileDescriptorProto toProto3 = protoBuilder3.build();
      assertThat(toProto3.getName()).isEqualTo("foo.proto");
      assertThat(toProto3.getPackage()).isEqualTo("foo.bar.baz");
      assertThat(toProto3.getSyntax()).isEqualTo("proto3");
      assertThat(toProto2.getOptions().getJavaPackage()).isEqualTo("foo.bar.baz");
      assertThat(toProto3.getMessageTypeList()).isEmpty();
    }

    @Test
    public void testDescriptor() throws Exception {
      Descriptor messageType = TestAllTypes.getDescriptor();
      Descriptor nestedType = TestAllTypes.NestedMessage.getDescriptor();

      assertThat(messageType.getName()).isEqualTo("TestAllTypes");
      assertThat(messageType.getFullName()).isEqualTo("proto2_unittest.TestAllTypes");
      assertThat(messageType.getFile()).isEqualTo(UnittestProto.getDescriptor());
      assertThat(messageType.getContainingType()).isNull();
      assertThat(messageType.getOptions())
          .isEqualTo(DescriptorProtos.MessageOptions.getDefaultInstance());
      assertThat(messageType.toProto().getName()).isEqualTo("TestAllTypes");

      assertThat(nestedType.getName()).isEqualTo("NestedMessage");
      assertThat(nestedType.getFullName()).isEqualTo("proto2_unittest.TestAllTypes.NestedMessage");
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
          .isEqualTo("proto2_unittest.TestAllTypes.optional_int32");
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
      assertThat(extension.getFullName()).isEqualTo("proto2_unittest.optional_int32_extension");
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
      assertThat(nestedExtension.getFullName()).isEqualTo("proto2_unittest.TestRequired.single");
      assertThat(nestedExtension.getExtensionScope()).isEqualTo(TestRequired.getDescriptor());
    }

    @Test
    public void testFieldDescriptorLabel() throws Exception {
      FieldDescriptor requiredField = TestRequired.getDescriptor().findFieldByName("a");
      FieldDescriptor optionalField =
          TestAllTypes.getDescriptor().findFieldByName("optional_int32");
      FieldDescriptor repeatedField =
          TestAllTypes.getDescriptor().findFieldByName("repeated_int32");

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
      FieldDescriptor optionalField =
          TestAllTypes.getDescriptor().findFieldByName("optional_int32");
      FieldDescriptor repeatedField =
          TestAllTypes.getDescriptor().findFieldByName("repeated_int32");
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
    public void testFieldDescriptorLegacyEnumFieldTreatedAsOpen() throws Exception {
      // Make an open enum definition and message that treats enum fields as open.

      FileDescriptorProto openEnumFile =
          FileDescriptorProto.newBuilder()
              .setName("open_enum.proto")
              .setSyntax("proto3")
              .addEnumType(
                  EnumDescriptorProto.newBuilder()
                      .setName("TestEnumOpen")
                      .addValue(
                          EnumValueDescriptorProto.newBuilder()
                              .setName("TestEnumOpen_VALUE0")
                              .setNumber(0)
                              .build())
                      .build())
              .addMessageType(
                  DescriptorProto.newBuilder()
                      .setName("TestOpenEnumField")
                      .addField(
                          FieldDescriptorProto.newBuilder()
                              .setName("int_field")
                              .setNumber(1)
                              .setType(FieldDescriptorProto.Type.TYPE_INT32)
                              .setLabel(FieldDescriptorProto.Label.LABEL_OPTIONAL)
                              .build())
                      .addField(
                          FieldDescriptorProto.newBuilder()
                              .setName("open_enum")
                              .setNumber(2)
                              .setType(FieldDescriptorProto.Type.TYPE_ENUM)
                              .setTypeName("TestEnumOpen")
                              .setLabel(FieldDescriptorProto.Label.LABEL_OPTIONAL)
                              .build())
                      .build())
              .build();
      FileDescriptor openEnumFileDescriptor =
          Descriptors.FileDescriptor.buildFrom(openEnumFile, new FileDescriptor[0]);
      Descriptor openMessage = openEnumFileDescriptor.getMessageTypes().get(0);
      EnumDescriptor openEnum = openEnumFileDescriptor.findEnumTypeByName("TestEnumOpen");
      assertThat(openEnum.isClosed()).isFalse();
      assertThat(openMessage.findFieldByName("int_field").legacyEnumFieldTreatedAsClosed())
          .isFalse();
      assertThat(openMessage.findFieldByName("open_enum").legacyEnumFieldTreatedAsClosed())
          .isFalse();
    }

    @Test
    public void testEditionFieldDescriptorLegacyEnumFieldTreatedAsClosedUnknown() throws Exception {
      // Make an open enum definition.
      FileDescriptorProto openEnumFile =
          FileDescriptorProto.newBuilder()
              .setName("open_enum.proto")
              .setSyntax("editions")
              .setEdition(Edition.EDITION_2023)
              .addEnumType(
                  EnumDescriptorProto.newBuilder()
                      .setName("TestEnumOpen")
                      .addValue(
                          EnumValueDescriptorProto.newBuilder()
                              .setName("TestEnumOpen_VALUE0")
                              .setNumber(0)
                              .build())
                      .build())
              .build();
      FileDescriptor openFileDescriptor =
          Descriptors.FileDescriptor.buildFrom(openEnumFile, new FileDescriptor[0]);
      EnumDescriptor openEnum = openFileDescriptor.getEnumTypes().get(0);
      assertThat(openEnum.isClosed()).isFalse();

      // Create a message that treats enum fields as closed.
      FileDescriptorProto editionsClosedEnumFile =
          FileDescriptorProto.newBuilder()
              .setName("editions_closed_enum_field.proto")
              .addDependency("open_enum.proto")
              .setSyntax("editions")
              .setEdition(Edition.EDITION_2023)
              .setOptions(
                  FileOptions.newBuilder()
                      .setFeatures(
                          DescriptorProtos.FeatureSet.newBuilder()
                              .setEnumType(DescriptorProtos.FeatureSet.EnumType.CLOSED)
                              .build())
                      .build())
              .addEnumType(
                  EnumDescriptorProto.newBuilder()
                      .setName("TestEnum")
                      .addValue(
                          EnumValueDescriptorProto.newBuilder()
                              .setName("TestEnum_VALUE0")
                              .setNumber(0)
                              .build())
                      .build())
              .addMessageType(
                  DescriptorProto.newBuilder()
                      .setName("TestClosedEnumField")
                      .addField(
                          FieldDescriptorProto.newBuilder()
                              .setName("int_field")
                              .setNumber(1)
                              .setType(FieldDescriptorProto.Type.TYPE_INT32)
                              .setLabel(FieldDescriptorProto.Label.LABEL_OPTIONAL)
                              .build())
                      .addField(
                          FieldDescriptorProto.newBuilder()
                              .setName("open_enum")
                              .setNumber(2)
                              .setType(FieldDescriptorProto.Type.TYPE_ENUM)
                              .setTypeName("TestEnumOpen")
                              .setLabel(FieldDescriptorProto.Label.LABEL_OPTIONAL)
                              .setOptions(
                                  DescriptorProtos.FieldOptions.newBuilder()
                                      .setFeatures(
                                          DescriptorProtos.FeatureSet.newBuilder()
                                              .setExtension(
                                                  JavaFeaturesProto.java_,
                                                  JavaFeaturesProto.JavaFeatures.newBuilder()
                                                      .setLegacyClosedEnum(true)
                                                      .build())
                                              .build())
                                      .build())
                              .build())
                      .addField(
                          FieldDescriptorProto.newBuilder()
                              .setName("closed_enum")
                              .setNumber(3)
                              .setType(FieldDescriptorProto.Type.TYPE_ENUM)
                              .setTypeName("TestEnum")
                              .setLabel(FieldDescriptorProto.Label.LABEL_OPTIONAL)
                              .build())
                      .build())
              .build();
      // Ensure Java features are in unknown fields.
      editionsClosedEnumFile =
          FileDescriptorProto.parseFrom(
              editionsClosedEnumFile.toByteString(), ExtensionRegistry.getEmptyRegistry());
      Descriptor editionsClosedMessage =
          Descriptors.FileDescriptor.buildFrom(
                  editionsClosedEnumFile, new FileDescriptor[] {openFileDescriptor})
              .getMessageTypes()
              .get(0);
      assertThat(
              editionsClosedMessage.findFieldByName("int_field").legacyEnumFieldTreatedAsClosed())
          .isFalse();
      assertThat(
              editionsClosedMessage.findFieldByName("closed_enum").legacyEnumFieldTreatedAsClosed())
          .isTrue();
      assertThat(
              editionsClosedMessage.findFieldByName("open_enum").legacyEnumFieldTreatedAsClosed())
          .isTrue();
    }

    @Test
    public void testEditionFieldDescriptorLegacyEnumFieldTreatedAsClosedCustomPool()
        throws Exception {

      FileDescriptor javaFeaturesDescriptor =
          Descriptors.FileDescriptor.buildFrom(
              JavaFeaturesProto.getDescriptor().toProto(),
              new FileDescriptor[] {DescriptorProtos.getDescriptor()});
      // Make an open enum definition.
      FileDescriptorProto openEnumFile =
          FileDescriptorProto.newBuilder()
              .setName("open_enum.proto")
              .setSyntax("editions")
              .setEdition(Edition.EDITION_2023)
              .addEnumType(
                  EnumDescriptorProto.newBuilder()
                      .setName("TestEnumOpen")
                      .addValue(
                          EnumValueDescriptorProto.newBuilder()
                              .setName("TestEnumOpen_VALUE0")
                              .setNumber(0)
                              .build())
                      .build())
              .build();
      FileDescriptor openFileDescriptor =
          Descriptors.FileDescriptor.buildFrom(openEnumFile, new FileDescriptor[0]);
      EnumDescriptor openEnum = openFileDescriptor.getEnumTypes().get(0);
      assertThat(openEnum.isClosed()).isFalse();

      // Create a message that treats enum fields as closed.
      FileDescriptorProto editionsClosedEnumFile =
          FileDescriptorProto.newBuilder()
              .setName("editions_closed_enum_field.proto")
              .addDependency("open_enum.proto")
              .setSyntax("editions")
              .setEdition(Edition.EDITION_2023)
              .setOptions(
                  FileOptions.newBuilder()
                      .setFeatures(
                          DescriptorProtos.FeatureSet.newBuilder()
                              .setEnumType(DescriptorProtos.FeatureSet.EnumType.CLOSED)
                              .build())
                      .build())
              .addEnumType(
                  EnumDescriptorProto.newBuilder()
                      .setName("TestEnum")
                      .addValue(
                          EnumValueDescriptorProto.newBuilder()
                              .setName("TestEnum_VALUE0")
                              .setNumber(0)
                              .build())
                      .build())
              .addMessageType(
                  DescriptorProto.newBuilder()
                      .setName("TestClosedEnumField")
                      .addField(
                          FieldDescriptorProto.newBuilder()
                              .setName("int_field")
                              .setNumber(1)
                              .setType(FieldDescriptorProto.Type.TYPE_INT32)
                              .setLabel(FieldDescriptorProto.Label.LABEL_OPTIONAL)
                              .build())
                      .addField(
                          FieldDescriptorProto.newBuilder()
                              .setName("open_enum")
                              .setNumber(2)
                              .setType(FieldDescriptorProto.Type.TYPE_ENUM)
                              .setTypeName("TestEnumOpen")
                              .setLabel(FieldDescriptorProto.Label.LABEL_OPTIONAL)
                              .setOptions(
                                  DescriptorProtos.FieldOptions.newBuilder()
                                      .setFeatures(
                                          DescriptorProtos.FeatureSet.newBuilder()
                                              .setField(
                                                  // Set extension using custom descriptor
                                                  javaFeaturesDescriptor.findExtensionByName(
                                                      JavaFeaturesProto.java_
                                                          .getDescriptor()
                                                          .getName()),
                                                  JavaFeaturesProto.JavaFeatures.newBuilder()
                                                      .setLegacyClosedEnum(true)
                                                      .build())
                                              .build())
                                      .build())
                              .build())
                      .addField(
                          FieldDescriptorProto.newBuilder()
                              .setName("closed_enum")
                              .setNumber(3)
                              .setType(FieldDescriptorProto.Type.TYPE_ENUM)
                              .setTypeName("TestEnum")
                              .setLabel(FieldDescriptorProto.Label.LABEL_OPTIONAL)
                              .build())
                      .build())
              .build();
      Descriptor editionsClosedMessage =
          Descriptors.FileDescriptor.buildFrom(
                  editionsClosedEnumFile,
                  new FileDescriptor[] {openFileDescriptor, javaFeaturesDescriptor})
              .getMessageTypes()
              .get(0);
      assertThat(
              editionsClosedMessage.findFieldByName("int_field").legacyEnumFieldTreatedAsClosed())
          .isFalse();
      assertThat(
              editionsClosedMessage.findFieldByName("closed_enum").legacyEnumFieldTreatedAsClosed())
          .isTrue();
      assertThat(
              editionsClosedMessage.findFieldByName("open_enum").legacyEnumFieldTreatedAsClosed())
          .isTrue();
    }

    @Test
    public void testEnumDescriptor() throws Exception {
      EnumDescriptor enumType = ForeignEnum.getDescriptor();
      EnumDescriptor nestedType = TestAllTypes.NestedEnum.getDescriptor();

      assertThat(enumType.getName()).isEqualTo("ForeignEnum");
      assertThat(enumType.getFullName()).isEqualTo("proto2_unittest.ForeignEnum");
      assertThat(enumType.getFile()).isEqualTo(UnittestProto.getDescriptor());
      assertThat(enumType.isClosed()).isTrue();
      assertThat(enumType.getContainingType()).isNull();
      assertThat(enumType.getOptions())
          .isEqualTo(DescriptorProtos.EnumOptions.getDefaultInstance());

      assertThat(nestedType.getName()).isEqualTo("NestedEnum");
      assertThat(nestedType.getFullName()).isEqualTo("proto2_unittest.TestAllTypes.NestedEnum");
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
      assertThat(service.getFullName()).isEqualTo("proto2_unittest.TestService");
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

      ServiceDescriptor service =
          UnittestCustomOptions.TestServiceWithCustomOptions.getDescriptor();

      assertThat(service.getOptions().hasExtension(UnittestCustomOptions.serviceOpt1)).isTrue();
      assertThat(service.getOptions().getExtension(UnittestCustomOptions.serviceOpt1))
          .isEqualTo(Long.valueOf(-9876543210L));

      MethodDescriptor method = service.findMethodByName("Foo");
      assertThat(method).isNotNull();

      assertThat(method.getOptions().hasExtension(UnittestCustomOptions.methodOpt1)).isTrue();
      assertThat(method.getOptions().getExtension(UnittestCustomOptions.methodOpt1))
          .isEqualTo(UnittestCustomOptions.MethodOpt1.METHODOPT1_VAL2);
    }

    @Test
    public void testOptionRetention() throws Exception {
      // Verify that options with RETENTION_SOURCE are stripped from the
      // generated descriptors.
      FileOptions options = UnittestRetention.getDescriptor().getOptions();
      assertThat(options.hasExtension(UnittestRetention.plainOption)).isTrue();
      assertThat(options.hasExtension(UnittestRetention.runtimeRetentionOption)).isTrue();
      assertThat(options.hasExtension(UnittestRetention.sourceRetentionOption)).isFalse();
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

    /** Tests that parsing an unknown enum throws an exception */
    @Test
    public void testParseUnknownEnum() {
      FieldDescriptorProto.Builder field =
          FieldDescriptorProto.newBuilder()
              .setLabel(FieldDescriptorProto.Label.LABEL_OPTIONAL)
              .setTypeName("UnknownEnum")
              .setType(FieldDescriptorProto.Type.TYPE_ENUM)
              .setName("bar")
              .setNumber(1);
      DescriptorProto.Builder messageType =
          DescriptorProto.newBuilder().setName("Foo").addField(field);
      FileDescriptorProto fooProto =
          FileDescriptorProto.newBuilder()
              .setName("foo.proto")
              .addDependency("bar.proto")
              .addMessageType(messageType)
              .build();
      try {
        Descriptors.FileDescriptor.buildFrom(fooProto, new FileDescriptor[0], true);
        assertWithMessage("DescriptorValidationException expected").fail();
      } catch (DescriptorValidationException expected) {
        assertThat(expected.getMessage()).contains("\"UnknownEnum\" is not an enum type.");
      }
    }

    /**
     * Tests the translate/crosslink for an example where a message field's name and type name are
     * the same.
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
      FileDescriptor fooFile =
          Descriptors.FileDescriptor.buildFrom(fooProto, new FileDescriptor[0]);
      FileDescriptor barFile =
          Descriptors.FileDescriptor.buildFrom(barProto, new FileDescriptor[] {fooFile});

      // Items in the FileDescriptor array can be in any order.
      FileDescriptor unused1 =
          Descriptors.FileDescriptor.buildFrom(bazProto, new FileDescriptor[] {fooFile, barFile});
      FileDescriptor unused2 =
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
      FileDescriptor fooFile =
          Descriptors.FileDescriptor.buildFrom(fooProto, new FileDescriptor[0]);
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
      FileDescriptor unused =
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
      FileDescriptor barFile =
          Descriptors.FileDescriptor.buildFrom(barProto, new FileDescriptor[0]);
      FileDescriptor forwardFile =
          Descriptors.FileDescriptor.buildFrom(forwardProto, new FileDescriptor[] {barFile});

      try {
        FileDescriptor unused =
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
      FileDescriptor barFile =
          Descriptors.FileDescriptor.buildFrom(barProto, new FileDescriptor[0]);
      FileDescriptor forwardFile =
          Descriptors.FileDescriptor.buildFrom(forwardProto, new FileDescriptor[] {barFile});
      FileDescriptor unused =
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
      FileDescriptor fooFile =
          Descriptors.FileDescriptor.buildFrom(fooProto, new FileDescriptor[0]);
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
        assertThat(field.getEnumType().getFile().getPackage().equals("a.b.c.d.bar.shared"))
            .isTrue();
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

      assertThat(oneofDescriptor.getFieldCount()).isEqualTo(7);
      assertThat(field).isSameInstanceAs(oneofDescriptor.getField(1));

      assertThat(oneofDescriptor.getFields()).hasSize(7);
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
    public void testReservedEnumFields() {
      EnumDescriptor d = TestReservedEnumFields.getDescriptor();
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
          .isEqualTo("proto2_unittest.TestAllTypes.optional_uint64");
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
                                  DescriptorProtos.FieldOptions.newBuilder()
                                      .setPacked(true)
                                      .build())
                              .build())
                      .build())
              .build();
      FileDescriptor unused =
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

    @Test
    public void testGetOptionsStripsFeatures() {
      FieldDescriptor field =
          UnittestLegacyFeatures.TestEditionsMessage.getDescriptor()
              .findFieldByName("required_field");
      assertThat(field.getOptions().hasFeatures()).isFalse();
    }

    @Test
    public void testLegacyRequiredTransform() {
      Descriptor descriptor = UnittestLegacyFeatures.TestEditionsMessage.getDescriptor();
      assertThat(descriptor.findFieldByName("required_field").isRequired()).isTrue();
    }

    @Test
    public void testLegacyGroupTransform() {
      Descriptor descriptor = UnittestLegacyFeatures.TestEditionsMessage.getDescriptor();
      assertThat(descriptor.findFieldByName("delimited_field").getType())
          .isEqualTo(FieldDescriptor.Type.GROUP);
    }

    @Test
    public void testLegacyInferRequired() throws Exception {
      FileDescriptor file =
          FileDescriptor.buildFrom(
              FileDescriptorProto.newBuilder()
                  .setName("some/filename/some.proto")
                  .setSyntax("proto2")
                  .addMessageType(
                      DescriptorProto.newBuilder()
                          .setName("Foo")
                          .addField(
                              FieldDescriptorProto.newBuilder()
                                  .setName("a")
                                  .setNumber(1)
                                  .setType(FieldDescriptorProto.Type.TYPE_INT32)
                                  .setLabel(FieldDescriptorProto.Label.LABEL_REQUIRED)
                                  .build())
                          .build())
                  .build(),
              new FileDescriptor[0]);
      FieldDescriptor field = file.findMessageTypeByName("Foo").findFieldByName("a");
      assertThat(field.features.getFieldPresence())
          .isEqualTo(DescriptorProtos.FeatureSet.FieldPresence.LEGACY_REQUIRED);
    }

    @Test
    public void testLegacyInferGroup() throws Exception {
      FileDescriptor file =
          FileDescriptor.buildFrom(
              FileDescriptorProto.newBuilder()
                  .setName("some/filename/some.proto")
                  .setSyntax("proto2")
                  .addMessageType(
                      DescriptorProto.newBuilder()
                          .setName("Foo")
                          .addNestedType(
                              DescriptorProto.newBuilder().setName("OptionalGroup").build())
                          .addField(
                              FieldDescriptorProto.newBuilder()
                                  .setName("optionalgroup")
                                  .setNumber(1)
                                  .setType(FieldDescriptorProto.Type.TYPE_GROUP)
                                  .setLabel(FieldDescriptorProto.Label.LABEL_OPTIONAL)
                                  .setTypeName("Foo.OptionalGroup")
                                  .build())
                          .build())
                  .build(),
              new FileDescriptor[0]);
      FieldDescriptor field = file.findMessageTypeByName("Foo").findFieldByName("optionalgroup");
      assertThat(field.features.getMessageEncoding())
          .isEqualTo(DescriptorProtos.FeatureSet.MessageEncoding.DELIMITED);
    }

    @Test
    public void testLegacyInferProto2Packed() throws Exception {
      FileDescriptor file =
          FileDescriptor.buildFrom(
              FileDescriptorProto.newBuilder()
                  .setName("some/filename/some.proto")
                  .setSyntax("proto2")
                  .addMessageType(
                      DescriptorProto.newBuilder()
                          .setName("Foo")
                          .addField(
                              FieldDescriptorProto.newBuilder()
                                  .setName("a")
                                  .setNumber(1)
                                  .setLabel(FieldDescriptorProto.Label.LABEL_REPEATED)
                                  .setType(FieldDescriptorProto.Type.TYPE_INT32)
                                  .setOptions(FieldOptions.newBuilder().setPacked(true).build())
                                  .build())
                          .build())
                  .build(),
              new FileDescriptor[0]);
      FieldDescriptor field = file.findMessageTypeByName("Foo").findFieldByName("a");
      assertThat(field.features.getRepeatedFieldEncoding())
          .isEqualTo(DescriptorProtos.FeatureSet.RepeatedFieldEncoding.PACKED);
    }

    @Test
    public void testLegacyInferProto3Expanded() throws Exception {
      FileDescriptor file =
          FileDescriptor.buildFrom(
              FileDescriptorProto.newBuilder()
                  .setName("some/filename/some.proto")
                  .setSyntax("proto3")
                  .addMessageType(
                      DescriptorProto.newBuilder()
                          .setName("Foo")
                          .addField(
                              FieldDescriptorProto.newBuilder()
                                  .setName("a")
                                  .setNumber(1)
                                  .setType(FieldDescriptorProto.Type.TYPE_INT32)
                                  .setLabel(FieldDescriptorProto.Label.LABEL_REPEATED)
                                  .setOptions(FieldOptions.newBuilder().setPacked(false).build())
                                  .build())
                          .build())
                  .build(),
              new FileDescriptor[0]);
      FieldDescriptor field = file.findMessageTypeByName("Foo").findFieldByName("a");
      assertThat(field.features.getRepeatedFieldEncoding())
          .isEqualTo(DescriptorProtos.FeatureSet.RepeatedFieldEncoding.EXPANDED);
    }

    @Test
    public void testLegacyInferProto2Utf8Validation() throws Exception {
      FileDescriptor file =
          FileDescriptor.buildFrom(
              FileDescriptorProto.newBuilder()
                  .setName("some/filename/some.proto")
                  .setPackage("proto2_unittest")
                  .setSyntax("proto2")
                  .setOptions(FileOptions.newBuilder().setJavaStringCheckUtf8(true))
                  .build(),
              new FileDescriptor[0]);
      assertThat(file.features.getExtension(JavaFeaturesProto.java_).getUtf8Validation())
          .isEqualTo(JavaFeaturesProto.JavaFeatures.Utf8Validation.VERIFY);
    }

    @Test
    public void testProto2Defaults() throws Exception {
      FileDescriptor proto2File =
          FileDescriptor.buildFrom(
              FileDescriptorProto.newBuilder()
                  .setName("some/filename/some.proto")
                  .setPackage("proto2_unittest")
                  .setSyntax("proto2")
                  .build(),
              new FileDescriptor[0]);
      DescriptorProtos.FeatureSet features = proto2File.features;
      assertThat(features.getFieldPresence())
          .isEqualTo(DescriptorProtos.FeatureSet.FieldPresence.EXPLICIT);
      assertThat(features.getEnumType()).isEqualTo(DescriptorProtos.FeatureSet.EnumType.CLOSED);
      assertThat(features.getRepeatedFieldEncoding())
          .isEqualTo(DescriptorProtos.FeatureSet.RepeatedFieldEncoding.EXPANDED);
      assertThat(features.getUtf8Validation())
          .isEqualTo(DescriptorProtos.FeatureSet.Utf8Validation.NONE);
      assertThat(features.getMessageEncoding())
          .isEqualTo(DescriptorProtos.FeatureSet.MessageEncoding.LENGTH_PREFIXED);
      assertThat(features.getJsonFormat())
          .isEqualTo(DescriptorProtos.FeatureSet.JsonFormat.LEGACY_BEST_EFFORT);

      assertThat(features.getExtension(JavaFeaturesProto.java_).getLegacyClosedEnum()).isTrue();
      assertThat(features.getExtension(JavaFeaturesProto.java_).getUtf8Validation())
          .isEqualTo(JavaFeaturesProto.JavaFeatures.Utf8Validation.DEFAULT);
    }

    @Test
    public void testProto3Defaults() throws Exception {
      FileDescriptor proto3File =
          FileDescriptor.buildFrom(
              FileDescriptorProto.newBuilder()
                  .setName("some/filename/some.proto")
                  .setPackage("proto3_unittest")
                  .setSyntax("proto3")
                  .build(),
              new FileDescriptor[0]);
      DescriptorProtos.FeatureSet features = proto3File.features;
      assertThat(features.getFieldPresence())
          .isEqualTo(DescriptorProtos.FeatureSet.FieldPresence.IMPLICIT);
      assertThat(features.getEnumType()).isEqualTo(DescriptorProtos.FeatureSet.EnumType.OPEN);
      assertThat(features.getRepeatedFieldEncoding())
          .isEqualTo(DescriptorProtos.FeatureSet.RepeatedFieldEncoding.PACKED);
      assertThat(features.getUtf8Validation())
          .isEqualTo(DescriptorProtos.FeatureSet.Utf8Validation.VERIFY);
      assertThat(features.getMessageEncoding())
          .isEqualTo(DescriptorProtos.FeatureSet.MessageEncoding.LENGTH_PREFIXED);

      assertThat(features.getExtension(JavaFeaturesProto.java_).getLegacyClosedEnum()).isFalse();
      assertThat(features.getExtension(JavaFeaturesProto.java_).getUtf8Validation())
          .isEqualTo(JavaFeaturesProto.JavaFeatures.Utf8Validation.DEFAULT);
    }

    @Test
    public void testProto3ExtensionPresence() {
      FileDescriptorProto.Builder file = FileDescriptorProto.newBuilder();

      assertThat(file.getOptions().hasExtension(Proto3FileExtensions.singularInt)).isFalse();
      assertThat(file.getOptions().getExtension(Proto3FileExtensions.singularInt)).isEqualTo(0);

      file.getOptionsBuilder().setExtension(Proto3FileExtensions.singularInt, 1);

      assertThat(file.getOptions().hasExtension(Proto3FileExtensions.singularInt)).isTrue();
      assertThat(file.getOptions().getExtension(Proto3FileExtensions.singularInt)).isEqualTo(1);
    }

    @Test
    public void testProto3ExtensionHasPresence() {
      assertThat(Proto3FileExtensions.singularInt.getDescriptor().hasPresence()).isTrue();
      assertThat(Proto3FileExtensions.repeatedInt.getDescriptor().hasPresence()).isFalse();
    }
  }

  public static class FeatureInheritanceTest {
    FileDescriptorProto.Builder fileProto;
    FieldDescriptorProto.Builder topExtensionProto;
    EnumDescriptorProto.Builder topEnumProto;
    EnumValueDescriptorProto.Builder enumValueProto;
    DescriptorProto.Builder topMessageProto;
    FieldDescriptorProto.Builder fieldProto;
    FieldDescriptorProto.Builder nestedExtensionProto;
    DescriptorProto.Builder nestedMessageProto;
    EnumDescriptorProto.Builder nestedEnumProto;
    OneofDescriptorProto.Builder oneofProto;
    FieldDescriptorProto.Builder oneofFieldProto;
    ServiceDescriptorProto.Builder serviceProto;
    MethodDescriptorProto.Builder methodProto;

    @Before
    public void setUp() {
      FeatureSetDefaults.Builder defaults = Descriptors.getJavaEditionDefaults().toBuilder();
      for (FeatureSetEditionDefault.Builder editionDefaults : defaults.getDefaultsBuilderList()) {
        setTestFeature(editionDefaults.getOverridableFeaturesBuilder(), 1);
      }
      Descriptors.setTestJavaEditionDefaults(defaults.build());

      this.fileProto =
          DescriptorProtos.FileDescriptorProto.newBuilder()
              .setName("some/filename/some.proto")
              .setPackage("proto2_unittest")
              .setEdition(DescriptorProtos.Edition.EDITION_2023)
              .setSyntax("editions");

      this.topExtensionProto =
          this.fileProto
              .addExtensionBuilder()
              .setName("top_extension")
              .setNumber(10)
              .setType(FieldDescriptorProto.Type.TYPE_INT32)
              .setLabel(FieldDescriptorProto.Label.LABEL_OPTIONAL)
              .setExtendee(".proto2_unittest.TopMessage");

      this.topEnumProto = this.fileProto.addEnumTypeBuilder().setName("TopEnum");
      this.enumValueProto = this.topEnumProto.addValueBuilder().setName("TOP_VALUE").setNumber(0);

      this.topMessageProto =
          this.fileProto
              .addMessageTypeBuilder()
              .setName("TopMessage")
              .addExtensionRange(ExtensionRange.newBuilder().setStart(10).setEnd(20).build());

      this.fieldProto =
          this.topMessageProto
              .addFieldBuilder()
              .setName("field")
              .setNumber(1)
              .setType(FieldDescriptorProto.Type.TYPE_INT32)
              .setLabel(FieldDescriptorProto.Label.LABEL_OPTIONAL);
      this.nestedExtensionProto =
          this.topMessageProto
              .addExtensionBuilder()
              .setName("nested_extension")
              .setNumber(11)
              .setType(FieldDescriptorProto.Type.TYPE_INT32)
              .setLabel(FieldDescriptorProto.Label.LABEL_OPTIONAL)
              .setExtendee(".proto2_unittest.TopMessage");
      this.nestedMessageProto =
          this.topMessageProto.addNestedTypeBuilder().setName("NestedMessage");
      this.nestedEnumProto =
          this.topMessageProto
              .addEnumTypeBuilder()
              .setName("NestedEnum")
              .addValue(EnumValueDescriptorProto.newBuilder().setName("NESTED_VALUE").setNumber(0));
      this.oneofProto = this.topMessageProto.addOneofDeclBuilder().setName("Oneof");
      this.oneofFieldProto =
          this.topMessageProto
              .addFieldBuilder()
              .setName("oneof_field")
              .setNumber(2)
              .setType(FieldDescriptorProto.Type.TYPE_INT32)
              .setLabel(FieldDescriptorProto.Label.LABEL_OPTIONAL)
              .setOneofIndex(0);
      this.serviceProto = this.fileProto.addServiceBuilder().setName("TestService");
      this.methodProto =
          this.serviceProto
              .addMethodBuilder()
              .setName("CallMethod")
              .setInputType(".proto2_unittest.TopMessage")
              .setOutputType(".proto2_unittest.TopMessage");
    }

    void setTestFeature(DescriptorProtos.FeatureSet.Builder features, int value) {
      features.setExtension(
          UnittestFeatures.test,
          features.getExtension(UnittestFeatures.test).toBuilder()
              .setMultipleFeature(UnittestFeatures.EnumFeature.forNumber(value))
              .build());
    }

    int getTestFeature(DescriptorProtos.FeatureSet features) {
      return features.getExtension(UnittestFeatures.test).getMultipleFeature().getNumber();
    }

    FileDescriptor buildFrom(FileDescriptorProto fileProto) throws Exception {
      return FileDescriptor.buildFrom(fileProto, new FileDescriptor[0]);
    }

    @Test
    public void testFileDefaults() throws Exception {
      FileDescriptor descriptor = buildFrom(fileProto.build());
      assertThat(getTestFeature(descriptor.features)).isEqualTo(1);
    }

    @Test
    public void testFileOverrides() throws Exception {
      setTestFeature(fileProto.getOptionsBuilder().getFeaturesBuilder(), 3);
      FileDescriptor descriptor = buildFrom(fileProto.build());
      assertThat(getTestFeature(descriptor.features)).isEqualTo(3);
    }

    @Test
    public void testFileMessageInherit() throws Exception {
      setTestFeature(fileProto.getOptionsBuilder().getFeaturesBuilder(), 3);
      FileDescriptor descriptor = buildFrom(fileProto.build());
      assertThat(getTestFeature(descriptor.getMessageTypes().get(0).features)).isEqualTo(3);
    }

    @Test
    public void testFileMessageOverride() throws Exception {
      setTestFeature(fileProto.getOptionsBuilder().getFeaturesBuilder(), 3);
      setTestFeature(topMessageProto.getOptionsBuilder().getFeaturesBuilder(), 5);
      FileDescriptor descriptor = buildFrom(fileProto.build());
      assertThat(getTestFeature(descriptor.getMessageTypes().get(0).features)).isEqualTo(5);
    }

    @Test
    public void testFileEnumInherit() throws Exception {
      setTestFeature(fileProto.getOptionsBuilder().getFeaturesBuilder(), 3);
      FileDescriptor descriptor = buildFrom(fileProto.build());
      assertThat(getTestFeature(descriptor.getEnumTypes().get(0).features)).isEqualTo(3);
    }

    @Test
    public void testFileEnumOverride() throws Exception {
      setTestFeature(fileProto.getOptionsBuilder().getFeaturesBuilder(), 3);
      setTestFeature(topEnumProto.getOptionsBuilder().getFeaturesBuilder(), 5);
      FileDescriptor descriptor = buildFrom(fileProto.build());
      assertThat(getTestFeature(descriptor.getEnumTypes().get(0).features)).isEqualTo(5);
    }

    @Test
    public void testFileExtensionInherit() throws Exception {
      setTestFeature(fileProto.getOptionsBuilder().getFeaturesBuilder(), 3);
      FileDescriptor descriptor = buildFrom(fileProto.build());
      assertThat(getTestFeature(descriptor.getExtensions().get(0).features)).isEqualTo(3);
    }

    @Test
    public void testFileExtensionOverride() throws Exception {
      setTestFeature(fileProto.getOptionsBuilder().getFeaturesBuilder(), 3);
      setTestFeature(topExtensionProto.getOptionsBuilder().getFeaturesBuilder(), 5);
      FileDescriptor descriptor = buildFrom(fileProto.build());
      assertThat(getTestFeature(descriptor.getExtensions().get(0).features)).isEqualTo(5);
    }

    @Test
    public void testFileServiceInherit() throws Exception {
      setTestFeature(fileProto.getOptionsBuilder().getFeaturesBuilder(), 3);
      FileDescriptor descriptor = buildFrom(fileProto.build());
      assertThat(getTestFeature(descriptor.getServices().get(0).features)).isEqualTo(3);
    }

    @Test
    public void testFileServiceOverride() throws Exception {
      setTestFeature(fileProto.getOptionsBuilder().getFeaturesBuilder(), 3);
      setTestFeature(serviceProto.getOptionsBuilder().getFeaturesBuilder(), 5);
      FileDescriptor descriptor = buildFrom(fileProto.build());
      assertThat(getTestFeature(descriptor.getServices().get(0).features)).isEqualTo(5);
    }

    @Test
    public void testMessageFieldInherit() throws Exception {
      setTestFeature(topMessageProto.getOptionsBuilder().getFeaturesBuilder(), 3);
      FileDescriptor descriptor = buildFrom(fileProto.build());
      assertThat(getTestFeature(descriptor.getMessageTypes().get(0).getFields().get(0).features))
          .isEqualTo(3);
    }

    @Test
    public void testMessageFieldOverride() throws Exception {
      setTestFeature(topMessageProto.getOptionsBuilder().getFeaturesBuilder(), 3);
      setTestFeature(fieldProto.getOptionsBuilder().getFeaturesBuilder(), 5);
      FileDescriptor descriptor = buildFrom(fileProto.build());
      assertThat(getTestFeature(descriptor.getMessageTypes().get(0).getFields().get(0).features))
          .isEqualTo(5);
    }

    @Test
    public void testMessageEnumInherit() throws Exception {
      setTestFeature(topMessageProto.getOptionsBuilder().getFeaturesBuilder(), 3);
      FileDescriptor descriptor = buildFrom(fileProto.build());
      assertThat(getTestFeature(descriptor.getMessageTypes().get(0).getEnumTypes().get(0).features))
          .isEqualTo(3);
    }

    @Test
    public void testMessageEnumOverride() throws Exception {
      setTestFeature(topMessageProto.getOptionsBuilder().getFeaturesBuilder(), 3);
      setTestFeature(nestedEnumProto.getOptionsBuilder().getFeaturesBuilder(), 5);
      FileDescriptor descriptor = buildFrom(fileProto.build());
      assertThat(getTestFeature(descriptor.getMessageTypes().get(0).getEnumTypes().get(0).features))
          .isEqualTo(5);
    }

    @Test
    public void testMessageMessageInherit() throws Exception {
      setTestFeature(topMessageProto.getOptionsBuilder().getFeaturesBuilder(), 3);
      FileDescriptor descriptor = buildFrom(fileProto.build());
      assertThat(
              getTestFeature(descriptor.getMessageTypes().get(0).getNestedTypes().get(0).features))
          .isEqualTo(3);
    }

    @Test
    public void testMessageMessageOverride() throws Exception {
      setTestFeature(topMessageProto.getOptionsBuilder().getFeaturesBuilder(), 3);
      setTestFeature(nestedMessageProto.getOptionsBuilder().getFeaturesBuilder(), 5);
      FileDescriptor descriptor = buildFrom(fileProto.build());
      assertThat(
              getTestFeature(descriptor.getMessageTypes().get(0).getNestedTypes().get(0).features))
          .isEqualTo(5);
    }

    @Test
    public void testMessageExtensionInherit() throws Exception {
      setTestFeature(topMessageProto.getOptionsBuilder().getFeaturesBuilder(), 3);
      FileDescriptor descriptor = buildFrom(fileProto.build());
      assertThat(
              getTestFeature(descriptor.getMessageTypes().get(0).getExtensions().get(0).features))
          .isEqualTo(3);
    }

    @Test
    public void testMessageExtensionOverride() throws Exception {
      setTestFeature(topMessageProto.getOptionsBuilder().getFeaturesBuilder(), 3);
      setTestFeature(nestedExtensionProto.getOptionsBuilder().getFeaturesBuilder(), 5);
      FileDescriptor descriptor = buildFrom(fileProto.build());
      assertThat(
              getTestFeature(descriptor.getMessageTypes().get(0).getExtensions().get(0).features))
          .isEqualTo(5);
    }

    @Test
    public void testMessageOneofInherit() throws Exception {
      setTestFeature(topMessageProto.getOptionsBuilder().getFeaturesBuilder(), 3);
      FileDescriptor descriptor = buildFrom(fileProto.build());
      assertThat(getTestFeature(descriptor.getMessageTypes().get(0).getOneofs().get(0).features))
          .isEqualTo(3);
    }

    @Test
    public void testMessageOneofOverride() throws Exception {
      setTestFeature(topMessageProto.getOptionsBuilder().getFeaturesBuilder(), 3);
      setTestFeature(oneofProto.getOptionsBuilder().getFeaturesBuilder(), 5);
      FileDescriptor descriptor = buildFrom(fileProto.build());
      assertThat(getTestFeature(descriptor.getMessageTypes().get(0).getOneofs().get(0).features))
          .isEqualTo(5);
    }

    @Test
    public void testOneofFieldInherit() throws Exception {
      setTestFeature(oneofProto.getOptionsBuilder().getFeaturesBuilder(), 3);
      FileDescriptor descriptor = buildFrom(fileProto.build());
      assertThat(
              getTestFeature(
                  descriptor
                      .getMessageTypes()
                      .get(0)
                      .getOneofs()
                      .get(0)
                      .getFields()
                      .get(0)
                      .features))
          .isEqualTo(3);
    }

    @Test
    public void testOneofFieldOverride() throws Exception {
      setTestFeature(oneofProto.getOptionsBuilder().getFeaturesBuilder(), 3);
      setTestFeature(oneofFieldProto.getOptionsBuilder().getFeaturesBuilder(), 5);
      FileDescriptor descriptor = buildFrom(fileProto.build());
      assertThat(
              getTestFeature(
                  descriptor
                      .getMessageTypes()
                      .get(0)
                      .getOneofs()
                      .get(0)
                      .getFields()
                      .get(0)
                      .features))
          .isEqualTo(5);
    }

    @Test
    public void testEnumValueInherit() throws Exception {
      setTestFeature(topEnumProto.getOptionsBuilder().getFeaturesBuilder(), 3);
      FileDescriptor descriptor = buildFrom(fileProto.build());
      assertThat(getTestFeature(descriptor.getEnumTypes().get(0).getValues().get(0).features))
          .isEqualTo(3);
    }

    @Test
    public void testEnumValueOverride() throws Exception {
      setTestFeature(topEnumProto.getOptionsBuilder().getFeaturesBuilder(), 3);
      setTestFeature(enumValueProto.getOptionsBuilder().getFeaturesBuilder(), 5);
      FileDescriptor descriptor = buildFrom(fileProto.build());
      assertThat(getTestFeature(descriptor.getEnumTypes().get(0).getValues().get(0).features))
          .isEqualTo(5);
    }

    @Test
    public void testServiceMethodInherit() throws Exception {
      setTestFeature(serviceProto.getOptionsBuilder().getFeaturesBuilder(), 3);
      FileDescriptor descriptor = buildFrom(fileProto.build());
      assertThat(getTestFeature(descriptor.getServices().get(0).getMethods().get(0).features))
          .isEqualTo(3);
    }

    @Test
    public void testServiceMethodOverride() throws Exception {
      setTestFeature(serviceProto.getOptionsBuilder().getFeaturesBuilder(), 3);
      setTestFeature(methodProto.getOptionsBuilder().getFeaturesBuilder(), 5);
      FileDescriptor descriptor = buildFrom(fileProto.build());
      assertThat(getTestFeature(descriptor.getServices().get(0).getMethods().get(0).features))
          .isEqualTo(5);
    }
  }
}
