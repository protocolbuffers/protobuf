// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

package com.google.protobuf;

import static com.google.common.truth.Truth.assertThat;
import static com.google.common.truth.Truth.assertWithMessage;

import com.google.protobuf.Descriptors.Descriptor;
import com.google.protobuf.Descriptors.FieldDescriptor;
import com.google.protobuf.test.UnittestImport;
import protobuf_unittest.EnumWithNoOuter;
import protobuf_unittest.MessageWithNoOuter;
import protobuf_unittest.MultipleFilesTestProto;
import protobuf_unittest.NestedExtension.MyNestedExtension;
import protobuf_unittest.NonNestedExtension;
import protobuf_unittest.NonNestedExtension.MessageToBeExtended;
import protobuf_unittest.NonNestedExtension.MyNonNestedExtension;
import protobuf_unittest.OuterClassNameTest2OuterClass;
import protobuf_unittest.OuterClassNameTest3OuterClass;
import protobuf_unittest.OuterClassNameTestOuterClass;
import protobuf_unittest.ServiceWithNoOuter;
import protobuf_unittest.UnittestOptimizeFor.TestOptimizedForSize;
import protobuf_unittest.UnittestOptimizeFor.TestOptionalOptimizedForSize;
import protobuf_unittest.UnittestOptimizeFor.TestRequiredOptimizedForSize;
import protobuf_unittest.UnittestProto;
import protobuf_unittest.UnittestProto.ForeignEnum;
import protobuf_unittest.UnittestProto.ForeignMessage;
import protobuf_unittest.UnittestProto.ForeignMessageOrBuilder;
import protobuf_unittest.UnittestProto.NestedTestAllTypes;
import protobuf_unittest.UnittestProto.TestAllExtensions;
import protobuf_unittest.UnittestProto.TestAllTypes;
import protobuf_unittest.UnittestProto.TestAllTypes.NestedMessage;
import protobuf_unittest.UnittestProto.TestAllTypesOrBuilder;
import protobuf_unittest.UnittestProto.TestExtremeDefaultValues;
import protobuf_unittest.UnittestProto.TestOneof2;
import protobuf_unittest.UnittestProto.TestPackedTypes;
import protobuf_unittest.UnittestProto.TestUnpackedTypes;
import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.ObjectInputStream;
import java.io.ObjectOutputStream;
import java.util.Arrays;
import java.util.Collections;
import java.util.Iterator;
import java.util.List;
import org.junit.After;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

/**
 * Unit test for generated messages and generated code. See also {@link MessageTest}, which tests
 * some generated message functionality.
 */
@SuppressWarnings({"ProtoBuilderReturnValueIgnored", "ReturnValueIgnored"})
@RunWith(JUnit4.class)
public class GeneratedMessageTest {

  private static final TestOneof2 EXPECTED_MERGED_MESSAGE =
      TestOneof2.newBuilder()
          .setFooMessage(TestOneof2.NestedMessage.newBuilder().addCorgeInt(1).addCorgeInt(2))
          .build();

  private static final TestOneof2 MESSAGE_TO_MERGE_FROM =
      TestOneof2.newBuilder()
          .setFooMessage(TestOneof2.NestedMessage.newBuilder().addCorgeInt(2))
          .build();

  private static final FieldDescriptor NESTED_MESSAGE_BB_FIELD =
      UnittestProto.TestAllTypes.NestedMessage.getDescriptor().findFieldByName("bb");

  TestUtil.ReflectionTester reflectionTester =
      new TestUtil.ReflectionTester(TestAllTypes.getDescriptor(), null);

  @After
  public void tearDown() {
    GeneratedMessageV3.setAlwaysUseFieldBuildersForTesting(false);
  }

  @Test
  public void testGetFieldBuilderForExtensionField() {
    TestAllExtensions.Builder builder = TestAllExtensions.newBuilder();
    Message.Builder fieldBuilder =
        builder.getFieldBuilder(UnittestProto.optionalNestedMessageExtension.getDescriptor());
    int expected = 7432;
    FieldDescriptor field =
        NestedMessage.getDescriptor().findFieldByNumber(NestedMessage.BB_FIELD_NUMBER);
    fieldBuilder.setField(field, expected);
    assertThat(builder.build().getExtension(UnittestProto.optionalNestedMessageExtension).getBb())
        .isEqualTo(expected);

    // fieldBuilder still updates the builder after builder build() has been called.
    expected += 100;
    fieldBuilder.setField(field, expected);
    assertThat(builder.build().getExtension(UnittestProto.optionalNestedMessageExtension).getBb())
        .isEqualTo(expected);
  }

  @Test
  public void testGetFieldBuilderWithExistingMessage() {
    TestAllExtensions.Builder builder = TestAllExtensions.newBuilder();
    builder.setExtension(
        UnittestProto.optionalNestedMessageExtension,
        NestedMessage.newBuilder().setBb(123).build());
    Message.Builder fieldBuilder =
        builder.getFieldBuilder(UnittestProto.optionalNestedMessageExtension.getDescriptor());
    int expected = 7432;
    FieldDescriptor field =
        NestedMessage.getDescriptor().findFieldByNumber(NestedMessage.BB_FIELD_NUMBER);
    fieldBuilder.setField(field, expected);
    assertThat(builder.build().getExtension(UnittestProto.optionalNestedMessageExtension).getBb())
        .isEqualTo(expected);

    // fieldBuilder still updates the builder after builder build() has been called.
    expected += 100;
    fieldBuilder.setField(field, expected);
    assertThat(builder.build().getExtension(UnittestProto.optionalNestedMessageExtension).getBb())
        .isEqualTo(expected);
  }

  @Test
  public void testGetFieldBuilderWithExistingBuilder() {
    TestAllExtensions.Builder builder = TestAllExtensions.newBuilder();
    NestedMessage.Builder nestedMessageBuilder = NestedMessage.newBuilder().setBb(123);
    builder.setField(
        UnittestProto.optionalNestedMessageExtension.getDescriptor(), nestedMessageBuilder);
    Message.Builder fieldBuilder =
        builder.getFieldBuilder(UnittestProto.optionalNestedMessageExtension.getDescriptor());
    int expected = 7432;
    FieldDescriptor field =
        NestedMessage.getDescriptor().findFieldByNumber(NestedMessage.BB_FIELD_NUMBER);
    fieldBuilder.setField(field, expected);
    assertThat(builder.build().getExtension(UnittestProto.optionalNestedMessageExtension).getBb())
        .isEqualTo(expected);

    // Existing nestedMessageBuilder will also update builder.
    expected += 100;
    nestedMessageBuilder.setBb(expected);
    assertThat(builder.build().getExtension(UnittestProto.optionalNestedMessageExtension).getBb())
        .isEqualTo(expected);

    // fieldBuilder still updates the builder.
    expected += 100;
    fieldBuilder.setField(field, expected);
    assertThat(builder.build().getExtension(UnittestProto.optionalNestedMessageExtension).getBb())
        .isEqualTo(expected);
  }

  @Test
  public void testGetRepeatedFieldBuilderForExtensionField() {
    TestAllExtensions.Builder builder = TestAllExtensions.newBuilder();
    builder.addExtension(
        UnittestProto.repeatedNestedMessageExtension,
        NestedMessage.newBuilder().setBb(123).build());
    Message.Builder fieldBuilder =
        builder.getRepeatedFieldBuilder(
            UnittestProto.repeatedNestedMessageExtension.getDescriptor(), 0);
    int expected = 7432;
    FieldDescriptor field =
        NestedMessage.getDescriptor().findFieldByNumber(NestedMessage.BB_FIELD_NUMBER);
    fieldBuilder.setField(field, expected);
    assertThat(
            builder.build().getExtension(UnittestProto.repeatedNestedMessageExtension, 0).getBb())
        .isEqualTo(expected);

    // fieldBuilder still updates the builder after builder build() has been called.
    expected += 100;
    fieldBuilder.setField(field, expected);
    assertThat(
            builder.build().getExtension(UnittestProto.repeatedNestedMessageExtension, 0).getBb())
        .isEqualTo(expected);
  }

  @Test
  public void testGetRepeatedFieldBuilderForExistingBuilder() {
    TestAllExtensions.Builder builder = TestAllExtensions.newBuilder();
    NestedMessage.Builder nestedMessageBuilder = NestedMessage.newBuilder().setBb(123);
    builder.addRepeatedField(
        UnittestProto.repeatedNestedMessageExtension.getDescriptor(), nestedMessageBuilder);
    Message.Builder fieldBuilder =
        builder.getRepeatedFieldBuilder(
            UnittestProto.repeatedNestedMessageExtension.getDescriptor(), 0);
    int expected = 7432;
    FieldDescriptor field =
        NestedMessage.getDescriptor().findFieldByNumber(NestedMessage.BB_FIELD_NUMBER);
    fieldBuilder.setField(field, expected);
    assertThat(
            builder.build().getExtension(UnittestProto.repeatedNestedMessageExtension, 0).getBb())
        .isEqualTo(expected);

    // Existing nestedMessageBuilder will also update builder.
    expected += 100;
    nestedMessageBuilder.setBb(expected);
    assertThat(
            builder.build().getExtension(UnittestProto.repeatedNestedMessageExtension, 0).getBb())
        .isEqualTo(expected);

    // fieldBuilder still updates the builder.
    expected += 100;
    fieldBuilder.setField(field, expected);
    assertThat(
            builder.build().getExtension(UnittestProto.repeatedNestedMessageExtension, 0).getBb())
        .isEqualTo(expected);
  }

  @Test
  public void testGetExtensionFieldOutOfBound() {
    TestAllExtensions.Builder builder = TestAllExtensions.newBuilder();
    try {
      builder.getRepeatedField(UnittestProto.repeatedNestedMessageExtension.getDescriptor(), 0);
      assertWithMessage("Expected IndexOutOfBoundsException to be thrown").fail();
    } catch (IndexOutOfBoundsException expected) {
    }
    try {
      builder.getExtension(UnittestProto.repeatedNestedMessageExtension, 0);
      assertWithMessage("Expected IndexOutOfBoundsException to be thrown").fail();
    } catch (IndexOutOfBoundsException expected) {
    }
    TestAllExtensions extensionsMessage = builder.build();
    try {
      extensionsMessage.getRepeatedField(
          UnittestProto.repeatedNestedMessageExtension.getDescriptor(), 0);
      assertWithMessage("Expected IndexOutOfBoundsException to be thrown").fail();
    } catch (IndexOutOfBoundsException expected) {
    }
    try {
      extensionsMessage.getExtension(UnittestProto.repeatedNestedMessageExtension, 0);
      assertWithMessage("Expected IndexOutOfBoundsException to be thrown").fail();
    } catch (IndexOutOfBoundsException expected) {
    }
  }

  @Test
  public void testDefaultInstance() throws Exception {
    assertThat(TestAllTypes.getDefaultInstance())
        .isSameInstanceAs(TestAllTypes.getDefaultInstance().getDefaultInstanceForType());
    assertThat(TestAllTypes.getDefaultInstance())
        .isSameInstanceAs(TestAllTypes.newBuilder().getDefaultInstanceForType());
  }

  @Test
  public void testMessageOrBuilder() throws Exception {
    TestAllTypes.Builder builder = TestAllTypes.newBuilder();
    TestUtil.setAllFields(builder);
    TestAllTypes message = builder.build();
    TestUtil.assertAllFieldsSet(message);
  }

  @Test
  public void testUsingBuilderMultipleTimes() throws Exception {
    TestAllTypes.Builder builder = TestAllTypes.newBuilder();
    // primitive field scalar and repeated
    builder.setOptionalSfixed64(100);
    builder.addRepeatedInt32(100);
    // enum field scalar and repeated
    builder.setOptionalImportEnum(UnittestImport.ImportEnum.IMPORT_BAR);
    builder.addRepeatedImportEnum(UnittestImport.ImportEnum.IMPORT_BAR);
    // proto field scalar and repeated
    builder.setOptionalForeignMessage(ForeignMessage.newBuilder().setC(1));
    builder.addRepeatedForeignMessage(ForeignMessage.newBuilder().setC(1));

    TestAllTypes value1 = builder.build();

    assertThat(value1.getOptionalSfixed64()).isEqualTo(100);
    assertThat(value1.getRepeatedInt32(0)).isEqualTo(100);
    assertThat(value1.getOptionalImportEnum()).isEqualTo(UnittestImport.ImportEnum.IMPORT_BAR);
    assertThat(value1.getRepeatedImportEnum(0)).isEqualTo(UnittestImport.ImportEnum.IMPORT_BAR);
    assertThat(value1.getOptionalForeignMessage().getC()).isEqualTo(1);
    assertThat(value1.getRepeatedForeignMessage(0).getC()).isEqualTo(1);

    // Make sure that builder didn't update previously created values
    builder.setOptionalSfixed64(200);
    builder.setRepeatedInt32(0, 200);
    builder.setOptionalImportEnum(UnittestImport.ImportEnum.IMPORT_FOO);
    builder.setRepeatedImportEnum(0, UnittestImport.ImportEnum.IMPORT_FOO);
    builder.setOptionalForeignMessage(ForeignMessage.newBuilder().setC(2));
    builder.setRepeatedForeignMessage(0, ForeignMessage.newBuilder().setC(2));

    TestAllTypes value2 = builder.build();

    // Make sure value1 didn't change.
    assertThat(value1.getOptionalSfixed64()).isEqualTo(100);
    assertThat(value1.getRepeatedInt32(0)).isEqualTo(100);
    assertThat(value1.getOptionalImportEnum()).isEqualTo(UnittestImport.ImportEnum.IMPORT_BAR);
    assertThat(value1.getRepeatedImportEnum(0)).isEqualTo(UnittestImport.ImportEnum.IMPORT_BAR);
    assertThat(value1.getOptionalForeignMessage().getC()).isEqualTo(1);
    assertThat(value1.getRepeatedForeignMessage(0).getC()).isEqualTo(1);

    // Make sure value2 is correct
    assertThat(value2.getOptionalSfixed64()).isEqualTo(200);
    assertThat(value2.getRepeatedInt32(0)).isEqualTo(200);
    assertThat(value2.getOptionalImportEnum()).isEqualTo(UnittestImport.ImportEnum.IMPORT_FOO);
    assertThat(value2.getRepeatedImportEnum(0)).isEqualTo(UnittestImport.ImportEnum.IMPORT_FOO);
    assertThat(value2.getOptionalForeignMessage().getC()).isEqualTo(2);
    assertThat(value2.getRepeatedForeignMessage(0).getC()).isEqualTo(2);
  }

  @Test
  public void testProtosShareRepeatedArraysIfDidntChange() throws Exception {
    TestAllTypes.Builder builder = TestAllTypes.newBuilder();
    builder.addRepeatedInt32(100);
    builder.addRepeatedForeignMessage(ForeignMessage.getDefaultInstance());

    TestAllTypes value1 = builder.build();
    TestAllTypes value2 = value1.toBuilder().build();

    assertThat(value1.getRepeatedInt32List()).isSameInstanceAs(value2.getRepeatedInt32List());
    assertThat(value1.getRepeatedForeignMessageList())
        .isSameInstanceAs(value2.getRepeatedForeignMessageList());
  }

  @Test
  public void testRepeatedArraysAreImmutable() throws Exception {
    TestAllTypes.Builder builder = TestAllTypes.newBuilder();
    builder.addRepeatedInt32(100);
    builder.addRepeatedImportEnum(UnittestImport.ImportEnum.IMPORT_BAR);
    builder.addRepeatedForeignMessage(ForeignMessage.getDefaultInstance());
    assertIsUnmodifiable(builder.getRepeatedInt32List());
    assertIsUnmodifiable(builder.getRepeatedImportEnumList());
    assertIsUnmodifiable(builder.getRepeatedForeignMessageList());
    assertIsUnmodifiable(builder.getRepeatedFloatList());

    TestAllTypes value = builder.build();
    assertIsUnmodifiable(value.getRepeatedInt32List());
    assertIsUnmodifiable(value.getRepeatedImportEnumList());
    assertIsUnmodifiable(value.getRepeatedForeignMessageList());
    assertIsUnmodifiable(value.getRepeatedFloatList());
  }

  @Test
  public void testParsedMessagesAreImmutable() throws Exception {
    TestAllTypes value = TestAllTypes.parseFrom(TestUtil.getAllSet().toByteString());
    assertIsUnmodifiable(value.getRepeatedInt32List());
    assertIsUnmodifiable(value.getRepeatedInt64List());
    assertIsUnmodifiable(value.getRepeatedUint32List());
    assertIsUnmodifiable(value.getRepeatedUint64List());
    assertIsUnmodifiable(value.getRepeatedSint32List());
    assertIsUnmodifiable(value.getRepeatedSint64List());
    assertIsUnmodifiable(value.getRepeatedFixed32List());
    assertIsUnmodifiable(value.getRepeatedFixed64List());
    assertIsUnmodifiable(value.getRepeatedSfixed32List());
    assertIsUnmodifiable(value.getRepeatedSfixed64List());
    assertIsUnmodifiable(value.getRepeatedFloatList());
    assertIsUnmodifiable(value.getRepeatedDoubleList());
    assertIsUnmodifiable(value.getRepeatedBoolList());
    assertIsUnmodifiable(value.getRepeatedStringList());
    assertIsUnmodifiable(value.getRepeatedBytesList());
    assertIsUnmodifiable(value.getRepeatedGroupList());
    assertIsUnmodifiable(value.getRepeatedNestedMessageList());
    assertIsUnmodifiable(value.getRepeatedForeignMessageList());
    assertIsUnmodifiable(value.getRepeatedImportMessageList());
    assertIsUnmodifiable(value.getRepeatedNestedEnumList());
    assertIsUnmodifiable(value.getRepeatedForeignEnumList());
    assertIsUnmodifiable(value.getRepeatedImportEnumList());
  }

  private void assertIsUnmodifiable(List<?> list) {
    if (list == Collections.emptyList()) {
      // OKAY -- Need to check this b/c EmptyList allows you to call clear.
    } else {
      try {
        list.clear();
        assertWithMessage("List wasn't immutable").fail();
      } catch (UnsupportedOperationException e) {
        // good
      }
    }
  }

  @Test
  public void testSettersRejectNull() throws Exception {
    TestAllTypes.Builder builder = TestAllTypes.newBuilder();
    try {
      builder.setOptionalString(null);
      assertWithMessage("Exception was not thrown").fail();
    } catch (NullPointerException e) {
      // We expect this exception.
    }
    try {
      builder.setOptionalBytes(null);
      assertWithMessage("Exception was not thrown").fail();
    } catch (NullPointerException e) {
      // We expect this exception.
    }
    try {
      builder.setOptionalNestedMessage((TestAllTypes.NestedMessage) null);
      assertWithMessage("Exception was not thrown").fail();
    } catch (NullPointerException e) {
      // We expect this exception.
    }
    try {
      builder.setOptionalNestedMessage((TestAllTypes.NestedMessage.Builder) null);
      assertWithMessage("Exception was not thrown").fail();
    } catch (NullPointerException e) {
      // We expect this exception.
    }
    try {
      builder.setOptionalNestedEnum(null);
      assertWithMessage("Exception was not thrown").fail();
    } catch (NullPointerException e) {
      // We expect this exception.
    }
    try {
      builder.addRepeatedString(null);
      assertWithMessage("Exception was not thrown").fail();
    } catch (NullPointerException e) {
      // We expect this exception.
    }
    try {
      builder.addRepeatedBytes(null);
      assertWithMessage("Exception was not thrown").fail();
    } catch (NullPointerException e) {
      // We expect this exception.
    }
    try {
      builder.addRepeatedNestedMessage((TestAllTypes.NestedMessage) null);
      assertWithMessage("Exception was not thrown").fail();
    } catch (NullPointerException e) {
      // We expect this exception.
    }
    try {
      builder.addRepeatedNestedMessage((TestAllTypes.NestedMessage.Builder) null);
      assertWithMessage("Exception was not thrown").fail();
    } catch (NullPointerException e) {
      // We expect this exception.
    }
    try {
      builder.addRepeatedNestedEnum(null);
      assertWithMessage("Exception was not thrown").fail();
    } catch (NullPointerException e) {
      // We expect this exception.
    }
  }

  @Test
  public void testRepeatedSetters() throws Exception {
    TestAllTypes.Builder builder = TestAllTypes.newBuilder();
    TestUtil.setAllFields(builder);
    TestUtil.modifyRepeatedFields(builder);
    TestAllTypes message = builder.build();
    TestUtil.assertRepeatedFieldsModified(message);
  }

  @Test
  public void testRepeatedSettersRejectNull() throws Exception {
    TestAllTypes.Builder builder = TestAllTypes.newBuilder();

    builder.addRepeatedString("one");
    builder.addRepeatedString("two");
    try {
      builder.setRepeatedString(1, null);
      assertWithMessage("Exception was not thrown").fail();
    } catch (NullPointerException e) {
      // We expect this exception.
    }

    builder.addRepeatedBytes(TestUtil.toBytes("one"));
    builder.addRepeatedBytes(TestUtil.toBytes("two"));
    try {
      builder.setRepeatedBytes(1, null);
      assertWithMessage("Exception was not thrown").fail();
    } catch (NullPointerException e) {
      // We expect this exception.
    }

    builder.addRepeatedNestedMessage(TestAllTypes.NestedMessage.newBuilder().setBb(218).build());
    builder.addRepeatedNestedMessage(TestAllTypes.NestedMessage.newBuilder().setBb(456).build());
    try {
      builder.setRepeatedNestedMessage(1, (TestAllTypes.NestedMessage) null);
      assertWithMessage("Exception was not thrown").fail();
    } catch (NullPointerException e) {
      // We expect this exception.
    }
    try {
      builder.setRepeatedNestedMessage(1, (TestAllTypes.NestedMessage.Builder) null);
      assertWithMessage("Exception was not thrown").fail();
    } catch (NullPointerException e) {
      // We expect this exception.
    }

    builder.addRepeatedNestedEnum(TestAllTypes.NestedEnum.FOO);
    builder.addRepeatedNestedEnum(TestAllTypes.NestedEnum.BAR);
    try {
      builder.setRepeatedNestedEnum(1, null);
      assertWithMessage("Exception was not thrown").fail();
    } catch (NullPointerException e) {
      // We expect this exception.
    }
  }

  @Test
  public void testRepeatedAppend() throws Exception {
    TestAllTypes.Builder builder = TestAllTypes.newBuilder();

    builder.addAllRepeatedInt32(Arrays.asList(1, 2, 3, 4));
    builder.addAllRepeatedForeignEnum(Arrays.asList(ForeignEnum.FOREIGN_BAZ));

    ForeignMessage foreignMessage = ForeignMessage.newBuilder().setC(12).build();
    builder.addAllRepeatedForeignMessage(Arrays.asList(foreignMessage));

    TestAllTypes message = builder.build();
    assertThat(Arrays.asList(1, 2, 3, 4)).isEqualTo(message.getRepeatedInt32List());
    assertThat(Arrays.asList(ForeignEnum.FOREIGN_BAZ))
        .isEqualTo(message.getRepeatedForeignEnumList());
    assertThat(message.getRepeatedForeignMessageCount()).isEqualTo(1);
    assertThat(message.getRepeatedForeignMessage(0).getC()).isEqualTo(12);
  }

  @Test
  public void testRepeatedAppendRejectsNull() throws Exception {
    TestAllTypes.Builder builder = TestAllTypes.newBuilder();

    ForeignMessage foreignMessage = ForeignMessage.newBuilder().setC(12).build();
    try {
      builder.addAllRepeatedForeignMessage(Arrays.asList(foreignMessage, (ForeignMessage) null));
      assertWithMessage("Exception was not thrown").fail();
    } catch (NullPointerException e) {
      // We expect this exception.
    }

    try {
      builder.addAllRepeatedForeignEnum(Arrays.asList(ForeignEnum.FOREIGN_BAZ, null));
      assertWithMessage("Exception was not thrown").fail();
    } catch (NullPointerException e) {
      // We expect this exception.
    }

    try {
      builder.addAllRepeatedString(Arrays.asList("one", null));
      assertWithMessage("Exception was not thrown").fail();
    } catch (NullPointerException e) {
      // We expect this exception.
    }

    try {
      builder.addAllRepeatedBytes(Arrays.asList(TestUtil.toBytes("one"), null));
      assertWithMessage("Exception was not thrown").fail();
    } catch (NullPointerException e) {
      // We expect this exception.
    }
  }

  @Test
  public void testRepeatedAppendIterateOnlyOnce() throws Exception {
    // Create a Iterable that can only be iterated once.
    Iterable<String> stringIterable =
        new Iterable<String>() {
          private boolean called = false;

          @Override
          public Iterator<String> iterator() {
            if (called) {
              throw new IllegalStateException();
            }
            called = true;
            return Arrays.asList("one", "two", "three").iterator();
          }
        };
    TestAllTypes.Builder builder = TestAllTypes.newBuilder();
    builder.addAllRepeatedString(stringIterable);
    assertThat(builder.getRepeatedStringCount()).isEqualTo(3);
    assertThat(builder.getRepeatedString(0)).isEqualTo("one");
    assertThat(builder.getRepeatedString(1)).isEqualTo("two");
    assertThat(builder.getRepeatedString(2)).isEqualTo("three");

    try {
      builder.addAllRepeatedString(stringIterable);
      assertWithMessage("Exception was not thrown").fail();
    } catch (IllegalStateException e) {
      // We expect this exception.
    }
  }

  @Test
  public void testMergeFromOtherRejectsNull() throws Exception {
    try {
      TestAllTypes.Builder builder = TestAllTypes.newBuilder();
      builder.mergeFrom((TestAllTypes) null);
      assertWithMessage("Exception was not thrown").fail();
    } catch (NullPointerException e) {
      // We expect this exception.
    }
  }

  @Test
  public void testSettingForeignMessageUsingBuilder() throws Exception {
    TestAllTypes message =
        TestAllTypes.newBuilder()
            // Pass builder for foreign message instance.
            .setOptionalForeignMessage(ForeignMessage.newBuilder().setC(123))
            .build();
    TestAllTypes expectedMessage =
        TestAllTypes.newBuilder()
            // Create expected version passing foreign message instance explicitly.
            .setOptionalForeignMessage(ForeignMessage.newBuilder().setC(123).build())
            .build();
    // TODO: Upgrade to using real #equals method once implemented
    assertThat(message.toString()).isEqualTo(expectedMessage.toString());
  }

  @Test
  public void testSettingRepeatedForeignMessageUsingBuilder() throws Exception {
    TestAllTypes message =
        TestAllTypes.newBuilder()
            // Pass builder for foreign message instance.
            .addRepeatedForeignMessage(ForeignMessage.newBuilder().setC(456))
            .build();
    TestAllTypes expectedMessage =
        TestAllTypes.newBuilder()
            // Create expected version passing foreign message instance explicitly.
            .addRepeatedForeignMessage(ForeignMessage.newBuilder().setC(456).build())
            .build();
    assertThat(message.toString()).isEqualTo(expectedMessage.toString());
  }

  @Test
  public void testDefaults() throws Exception {
    TestUtil.assertClear(TestAllTypes.getDefaultInstance());
    TestUtil.assertClear(TestAllTypes.newBuilder().build());

    TestExtremeDefaultValues message = TestExtremeDefaultValues.getDefaultInstance();
    assertThat(message.getUtf8String()).isEqualTo("\u1234");
    assertThat(message.getInfDouble()).isEqualTo(Double.POSITIVE_INFINITY);
    assertThat(message.getNegInfDouble()).isEqualTo(Double.NEGATIVE_INFINITY);
    assertThat(Double.isNaN(message.getNanDouble())).isTrue();
    assertThat(message.getInfFloat()).isEqualTo(Float.POSITIVE_INFINITY);
    assertThat(message.getNegInfFloat()).isEqualTo(Float.NEGATIVE_INFINITY);
    assertThat(Float.isNaN(message.getNanFloat())).isTrue();
    assertThat(message.getCppTrigraph()).isEqualTo("? ? ?? ?? ??? ??/ ??-");
  }

  @Test
  public void testClear() throws Exception {
    TestAllTypes.Builder builder = TestAllTypes.newBuilder();
    TestUtil.assertClear(builder);
    TestUtil.setAllFields(builder);
    builder.clear();
    TestUtil.assertClear(builder);
  }

  @Test
  public void testReflectionGetters() throws Exception {
    TestAllTypes.Builder builder = TestAllTypes.newBuilder();
    TestUtil.setAllFields(builder);
    reflectionTester.assertAllFieldsSetViaReflection(builder);

    TestAllTypes message = builder.build();
    reflectionTester.assertAllFieldsSetViaReflection(message);
  }

  @Test
  public void testReflectionSetters() throws Exception {
    TestAllTypes.Builder builder = TestAllTypes.newBuilder();
    reflectionTester.setAllFieldsViaReflection(builder);
    TestUtil.assertAllFieldsSet(builder);

    TestAllTypes message = builder.build();
    TestUtil.assertAllFieldsSet(message);
  }

  @Test
  public void testReflectionSettersRejectNull() throws Exception {
    TestAllTypes.Builder builder = TestAllTypes.newBuilder();
    reflectionTester.assertReflectionSettersRejectNull(builder);
  }

  @Test
  public void testReflectionRepeatedSetters() throws Exception {
    TestAllTypes.Builder builder = TestAllTypes.newBuilder();
    reflectionTester.setAllFieldsViaReflection(builder);
    reflectionTester.modifyRepeatedFieldsViaReflection(builder);
    TestUtil.assertRepeatedFieldsModified(builder);

    TestAllTypes message = builder.build();
    TestUtil.assertRepeatedFieldsModified(message);
  }

  @Test
  public void testReflectionRepeatedSettersRejectNull() throws Exception {
    TestAllTypes.Builder builder = TestAllTypes.newBuilder();
    reflectionTester.assertReflectionRepeatedSettersRejectNull(builder);
  }

  @Test
  public void testReflectionDefaults() throws Exception {
    reflectionTester.assertClearViaReflection(TestAllTypes.getDefaultInstance());
    reflectionTester.assertClearViaReflection(TestAllTypes.newBuilder().build());
  }

  @Test
  public void testReflectionGetOneof() throws Exception {
    TestAllTypes.Builder builder = TestAllTypes.newBuilder();
    reflectionTester.setAllFieldsViaReflection(builder);
    Descriptors.OneofDescriptor oneof = TestAllTypes.getDescriptor().getOneofs().get(0);
    Descriptors.FieldDescriptor field = TestAllTypes.getDescriptor().findFieldByName("oneof_bytes");
    assertThat(field).isSameInstanceAs(builder.getOneofFieldDescriptor(oneof));

    TestAllTypes message = builder.build();
    assertThat(field).isSameInstanceAs(message.getOneofFieldDescriptor(oneof));
  }

  @Test
  public void testReflectionClearOneof() throws Exception {
    TestAllTypes.Builder builder = TestAllTypes.newBuilder();
    reflectionTester.setAllFieldsViaReflection(builder);
    Descriptors.OneofDescriptor oneof = TestAllTypes.getDescriptor().getOneofs().get(0);
    Descriptors.FieldDescriptor field = TestAllTypes.getDescriptor().findFieldByName("oneof_bytes");

    assertThat(builder.hasOneof(oneof)).isTrue();
    assertThat(builder.hasField(field)).isTrue();
    builder.clearOneof(oneof);
    assertThat(builder.hasOneof(oneof)).isFalse();
    assertThat(builder.hasField(field)).isFalse();
  }

  @Test
  public void testEnumInterface() throws Exception {
    assertThat(TestAllTypes.getDefaultInstance().getDefaultNestedEnum())
        .isInstanceOf(ProtocolMessageEnum.class);
  }

  @Test
  public void testEnumMap() throws Exception {
    Internal.EnumLiteMap<ForeignEnum> map = ForeignEnum.internalGetValueMap();

    for (ForeignEnum value : ForeignEnum.values()) {
      assertThat(map.findValueByNumber(value.getNumber())).isEqualTo(value);
    }

    assertThat(map.findValueByNumber(12345) == null).isTrue();
  }

  @Test
  public void testParsePackedToUnpacked() throws Exception {
    TestUnpackedTypes.Builder builder = TestUnpackedTypes.newBuilder();
    TestUnpackedTypes message = builder.mergeFrom(TestUtil.getPackedSet().toByteString()).build();
    TestUtil.assertUnpackedFieldsSet(message);
  }

  @Test
  public void testParseUnpackedToPacked() throws Exception {
    TestPackedTypes.Builder builder = TestPackedTypes.newBuilder();
    TestPackedTypes message = builder.mergeFrom(TestUtil.getUnpackedSet().toByteString()).build();
    TestUtil.assertPackedFieldsSet(message);
  }

  // =================================================================
  // Extensions.

  TestUtil.ReflectionTester extensionsReflectionTester =
      new TestUtil.ReflectionTester(
          TestAllExtensions.getDescriptor(), TestUtil.getFullExtensionRegistry());

  @Test
  public void testExtensionMessageOrBuilder() throws Exception {
    TestAllExtensions.Builder builder = TestAllExtensions.newBuilder();
    TestUtil.setAllExtensions(builder);
    TestAllExtensions message = builder.build();
    TestUtil.assertAllExtensionsSet(message);
  }

  @Test
  public void testGetBuilderForExtensionField() {
    TestAllExtensions.Builder builder = TestAllExtensions.newBuilder();
    Message.Builder fieldBuilder =
        builder.newBuilderForField(UnittestProto.optionalNestedMessageExtension.getDescriptor());
    final int expected = 7432;
    FieldDescriptor field =
        NestedMessage.getDescriptor().findFieldByNumber(NestedMessage.BB_FIELD_NUMBER);
    fieldBuilder.setField(field, expected);
    assertThat(fieldBuilder.build().getField(field)).isEqualTo(expected);
  }

  @Test
  public void testGetBuilderForNonMessageExtensionField() {
    TestAllExtensions.Builder builder = TestAllExtensions.newBuilder();
    try {
      // This should throw an exception because the extension field is not a message.
      builder.newBuilderForField(UnittestProto.optionalInt32Extension.getDescriptor());
      assertWithMessage("Exception was not thrown").fail();
    } catch (UnsupportedOperationException e) {
      // This exception is expected.
    }
  }

  @Test
  public void testExtensionRepeatedSetters() throws Exception {
    TestAllExtensions.Builder builder = TestAllExtensions.newBuilder();
    TestUtil.setAllExtensions(builder);
    TestUtil.modifyRepeatedExtensions(builder);
    TestAllExtensions message = builder.build();
    TestUtil.assertRepeatedExtensionsModified(message);
  }

  @Test
  public void testExtensionDefaults() throws Exception {
    TestUtil.assertExtensionsClear(TestAllExtensions.getDefaultInstance());
    TestUtil.assertExtensionsClear(TestAllExtensions.newBuilder().build());
  }

  @Test
  public void testUnsetRepeatedExtensionGetField() {
    TestAllExtensions message = TestAllExtensions.getDefaultInstance();
    Object value;

    value = message.getField(UnittestProto.repeatedStringExtension.getDescriptor());
    assertThat(value instanceof List).isTrue();
    assertThat(((List<?>) value).isEmpty()).isTrue();
    assertIsUnmodifiable((List<?>) value);

    value = message.getField(UnittestProto.repeatedNestedMessageExtension.getDescriptor());
    assertThat(value instanceof List).isTrue();
    assertThat(((List<?>) value).isEmpty()).isTrue();
    assertIsUnmodifiable((List<?>) value);
  }

  @Test
  public void testExtensionReflectionGetters() throws Exception {
    TestAllExtensions.Builder builder = TestAllExtensions.newBuilder();
    TestUtil.setAllExtensions(builder);
    extensionsReflectionTester.assertAllFieldsSetViaReflection(builder);

    TestAllExtensions message = builder.build();
    extensionsReflectionTester.assertAllFieldsSetViaReflection(message);
  }

  @Test
  public void testExtensionReflectionSetters() throws Exception {
    TestAllExtensions.Builder builder = TestAllExtensions.newBuilder();
    extensionsReflectionTester.setAllFieldsViaReflection(builder);
    TestUtil.assertAllExtensionsSet(builder);

    TestAllExtensions message = builder.build();
    TestUtil.assertAllExtensionsSet(message);
  }

  @Test
  public void testExtensionReflectionSettersRejectNull() throws Exception {
    TestAllExtensions.Builder builder = TestAllExtensions.newBuilder();
    extensionsReflectionTester.assertReflectionSettersRejectNull(builder);
  }

  @Test
  public void testExtensionReflectionRepeatedSetters() throws Exception {
    TestAllExtensions.Builder builder = TestAllExtensions.newBuilder();
    extensionsReflectionTester.setAllFieldsViaReflection(builder);
    extensionsReflectionTester.modifyRepeatedFieldsViaReflection(builder);
    TestUtil.assertRepeatedExtensionsModified(builder);

    TestAllExtensions message = builder.build();
    TestUtil.assertRepeatedExtensionsModified(message);
  }

  @Test
  public void testExtensionReflectionRepeatedSettersRejectNull() throws Exception {
    TestAllExtensions.Builder builder = TestAllExtensions.newBuilder();
    extensionsReflectionTester.assertReflectionRepeatedSettersRejectNull(builder);
  }

  @Test
  public void testExtensionReflectionDefaults() throws Exception {
    extensionsReflectionTester.assertClearViaReflection(TestAllExtensions.getDefaultInstance());
    extensionsReflectionTester.assertClearViaReflection(TestAllExtensions.newBuilder().build());
  }

  @Test
  public void testClearExtension() throws Exception {
    // clearExtension() is not actually used in TestUtil, so try it manually.
    assertThat(
            TestAllExtensions.newBuilder()
                .setExtension(UnittestProto.optionalInt32Extension, 1)
                .clearExtension(UnittestProto.optionalInt32Extension)
                .hasExtension(UnittestProto.optionalInt32Extension))
        .isFalse();
    assertThat(
            TestAllExtensions.newBuilder()
                .addExtension(UnittestProto.repeatedInt32Extension, 1)
                .clearExtension(UnittestProto.repeatedInt32Extension)
                .getExtensionCount(UnittestProto.repeatedInt32Extension))
        .isEqualTo(0);
  }

  @Test
  public void testExtensionCopy() throws Exception {
    TestAllExtensions original = TestUtil.getAllExtensionsSet();
    TestAllExtensions copy = TestAllExtensions.newBuilder(original).build();
    TestUtil.assertAllExtensionsSet(copy);
  }

  @Test
  public void testExtensionMergeFrom() throws Exception {
    TestAllExtensions original =
        TestAllExtensions.newBuilder()
            .setExtension(UnittestProto.optionalInt32Extension, 1)
            .build();
    TestAllExtensions merged = TestAllExtensions.newBuilder().mergeFrom(original).build();
    assertThat(merged.hasExtension(UnittestProto.optionalInt32Extension)).isTrue();
    assertThat((int) merged.getExtension(UnittestProto.optionalInt32Extension)).isEqualTo(1);
  }

  // =================================================================
  // multiple_files_test

  // Test that custom options of an file level enum are properly initialized.
  // This test needs to be put before any other access to MultipleFilesTestProto
  // or messages defined in multiple_files_test.proto because the class loading
  // order affects initialization process of custom options.
  @Test
  public void testEnumValueOptionsInMultipleFilesMode() throws Exception {
    assertThat(
            EnumWithNoOuter.FOO
                .getValueDescriptor()
                .getOptions()
                .getExtension(MultipleFilesTestProto.enumValueOption)
                .intValue())
        .isEqualTo(12345);
  }

  @Test
  public void testMultipleFilesOption() throws Exception {
    // We mostly just want to check that things compile.
    MessageWithNoOuter message =
        MessageWithNoOuter.newBuilder()
            .setNested(MessageWithNoOuter.NestedMessage.newBuilder().setI(1))
            .addForeign(TestAllTypes.newBuilder().setOptionalInt32(1))
            .setNestedEnum(MessageWithNoOuter.NestedEnum.BAZ)
            .setForeignEnum(EnumWithNoOuter.BAR)
            .build();
    assertThat(MessageWithNoOuter.parseFrom(message.toByteString())).isEqualTo(message);

    assertThat(MessageWithNoOuter.getDescriptor().getFile())
        .isEqualTo(MultipleFilesTestProto.getDescriptor());

    Descriptors.FieldDescriptor field =
        MessageWithNoOuter.getDescriptor().findFieldByName("foreign_enum");
    assertThat(message.getField(field)).isEqualTo(EnumWithNoOuter.BAR.getValueDescriptor());

    assertThat(ServiceWithNoOuter.getDescriptor().getFile())
        .isEqualTo(MultipleFilesTestProto.getDescriptor());

    assertThat(
            TestAllExtensions.getDefaultInstance()
                .hasExtension(MultipleFilesTestProto.extensionWithOuter))
        .isFalse();
  }

  @Test
  public void testOptionalFieldWithRequiredSubfieldsOptimizedForSize() throws Exception {
    TestOptionalOptimizedForSize message = TestOptionalOptimizedForSize.getDefaultInstance();
    assertThat(message.isInitialized()).isTrue();

    message =
        TestOptionalOptimizedForSize.newBuilder()
            .setO(TestRequiredOptimizedForSize.newBuilder().buildPartial())
            .buildPartial();
    assertThat(message.isInitialized()).isFalse();

    message =
        TestOptionalOptimizedForSize.newBuilder()
            .setO(TestRequiredOptimizedForSize.newBuilder().setX(5).buildPartial())
            .buildPartial();
    assertThat(message.isInitialized()).isTrue();
  }

  @Test
  public void testUninitializedExtensionInOptimizedForSize() throws Exception {
    TestOptimizedForSize.Builder builder = TestOptimizedForSize.newBuilder();
    builder.setExtension(
        TestOptimizedForSize.testExtension2,
        TestRequiredOptimizedForSize.newBuilder().buildPartial());
    assertThat(builder.isInitialized()).isFalse();
    assertThat(builder.buildPartial().isInitialized()).isFalse();

    builder = TestOptimizedForSize.newBuilder();
    builder.setExtension(
        TestOptimizedForSize.testExtension2,
        TestRequiredOptimizedForSize.newBuilder().setX(10).buildPartial());
    assertThat(builder.isInitialized()).isTrue();
    assertThat(builder.buildPartial().isInitialized()).isTrue();
  }

  @Test
  public void testToBuilder() throws Exception {
    TestAllTypes.Builder builder = TestAllTypes.newBuilder();
    TestUtil.setAllFields(builder);
    TestAllTypes message = builder.build();
    TestUtil.assertAllFieldsSet(message);
    TestUtil.assertAllFieldsSet(message.toBuilder().build());
  }

  @Test
  public void testFieldConstantValues() throws Exception {
    assertThat(TestAllTypes.NestedMessage.BB_FIELD_NUMBER).isEqualTo(1);
    assertThat(TestAllTypes.OPTIONAL_INT32_FIELD_NUMBER).isEqualTo(1);
    assertThat(TestAllTypes.OPTIONALGROUP_FIELD_NUMBER).isEqualTo(16);
    assertThat(TestAllTypes.OPTIONAL_NESTED_MESSAGE_FIELD_NUMBER).isEqualTo(18);
    assertThat(TestAllTypes.OPTIONAL_NESTED_ENUM_FIELD_NUMBER).isEqualTo(21);
    assertThat(TestAllTypes.REPEATED_INT32_FIELD_NUMBER).isEqualTo(31);
    assertThat(TestAllTypes.REPEATEDGROUP_FIELD_NUMBER).isEqualTo(46);
    assertThat(TestAllTypes.REPEATED_NESTED_MESSAGE_FIELD_NUMBER).isEqualTo(48);
    assertThat(TestAllTypes.REPEATED_NESTED_ENUM_FIELD_NUMBER).isEqualTo(51);
  }

  @Test
  public void testExtensionConstantValues() throws Exception {
    assertThat(UnittestProto.TestRequired.SINGLE_FIELD_NUMBER).isEqualTo(1000);
    assertThat(UnittestProto.TestRequired.MULTI_FIELD_NUMBER).isEqualTo(1001);
    assertThat(UnittestProto.OPTIONAL_INT32_EXTENSION_FIELD_NUMBER).isEqualTo(1);
    assertThat(UnittestProto.OPTIONALGROUP_EXTENSION_FIELD_NUMBER).isEqualTo(16);
    assertThat(UnittestProto.OPTIONAL_NESTED_MESSAGE_EXTENSION_FIELD_NUMBER).isEqualTo(18);
    assertThat(UnittestProto.OPTIONAL_NESTED_ENUM_EXTENSION_FIELD_NUMBER).isEqualTo(21);
    assertThat(UnittestProto.REPEATED_INT32_EXTENSION_FIELD_NUMBER).isEqualTo(31);
    assertThat(UnittestProto.REPEATEDGROUP_EXTENSION_FIELD_NUMBER).isEqualTo(46);
    assertThat(UnittestProto.REPEATED_NESTED_MESSAGE_EXTENSION_FIELD_NUMBER).isEqualTo(48);
    assertThat(UnittestProto.REPEATED_NESTED_ENUM_EXTENSION_FIELD_NUMBER).isEqualTo(51);
  }

  @Test
  public void testRecursiveMessageDefaultInstance() throws Exception {
    UnittestProto.TestRecursiveMessage message =
        UnittestProto.TestRecursiveMessage.getDefaultInstance();
    assertThat(message).isNotNull();
    assertThat(message.getA()).isEqualTo(message);
  }

  @Test
  public void testSerialize() throws Exception {
    ByteArrayOutputStream baos = new ByteArrayOutputStream();
    TestAllTypes.Builder builder = TestAllTypes.newBuilder();
    TestUtil.setAllFields(builder);
    TestAllTypes expected = builder.build();
    try (ObjectOutputStream out = new ObjectOutputStream(baos)) {
      out.writeObject(expected);
    }
    ByteArrayInputStream bais = new ByteArrayInputStream(baos.toByteArray());
    ObjectInputStream in = new ObjectInputStream(bais);
    TestAllTypes actual = (TestAllTypes) in.readObject();
    assertThat(actual).isEqualTo(expected);
  }

  @Test
  public void testSerializePartial() throws Exception {
    ByteArrayOutputStream baos = new ByteArrayOutputStream();
    TestAllTypes.Builder builder = TestAllTypes.newBuilder();
    TestAllTypes expected = builder.buildPartial();
    ObjectOutputStream out = new ObjectOutputStream(baos);
    try {
      out.writeObject(expected);
    } finally {
      out.close();
    }
    ByteArrayInputStream bais = new ByteArrayInputStream(baos.toByteArray());
    ObjectInputStream in = new ObjectInputStream(bais);
    TestAllTypes actual = (TestAllTypes) in.readObject();
    assertThat(actual).isEqualTo(expected);
  }

  @Test
  public void testDeserializeWithoutClassField() throws Exception {
    // serialized form for version <=3.6.0
    // just includes messageClassName and asBytes

    // Int32Value.newBuilder().setValue(123).build()
    byte[] int32ValueBytes =
        new byte[] {
          -84, -19, 0, 5, 115, 114, 0, 55, 99, 111, 109, 46, 103, 111, 111, 103, 108, 101, 46, 112,
          114, 111, 116, 111, 98, 117, 102, 46, 71, 101, 110, 101, 114, 97, 116, 101, 100, 77, 101,
          115, 115, 97, 103, 101, 76, 105, 116, 101, 36, 83, 101, 114, 105, 97, 108, 105, 122, 101,
          100, 70, 111, 114, 109, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 2, 91, 0, 7, 97, 115, 66, 121, 116,
          101, 115, 116, 0, 2, 91, 66, 76, 0, 16, 109, 101, 115, 115, 97, 103, 101, 67, 108, 97,
          115, 115, 78, 97, 109, 101, 116, 0, 18, 76, 106, 97, 118, 97, 47, 108, 97, 110, 103, 47,
          83, 116, 114, 105, 110, 103, 59, 120, 112, 117, 114, 0, 2, 91, 66, -84, -13, 23, -8, 6, 8,
          84, -32, 2, 0, 0, 120, 112, 0, 0, 0, 2, 8, 123, 116, 0, 30, 99, 111, 109, 46, 103, 111,
          111, 103, 108, 101, 46, 112, 114, 111, 116, 111, 98, 117, 102, 46, 73, 110, 116, 51, 50,
          86, 97, 108, 117, 101
        };

    ByteArrayInputStream bais = new ByteArrayInputStream(int32ValueBytes);
    ObjectInputStream in = new ObjectInputStream(bais);
    Int32Value int32Value = (Int32Value) in.readObject();
    assertThat(int32Value.getValue()).isEqualTo(123);
  }

  @Test
  public void testDeserializeWithClassField() throws Exception {
    // serialized form for version > 3.6.0
    // includes messageClass, messageClassName (for compatibility), and asBytes

    // Int32Value.newBuilder().setValue(123).build()
    byte[] int32ValueBytes =
        new byte[] {
          -84, -19, 0, 5, 115, 114, 0, 55, 99, 111, 109, 46, 103, 111, 111, 103, 108, 101, 46, 112,
          114, 111, 116, 111, 98, 117, 102, 46, 71, 101, 110, 101, 114, 97, 116, 101, 100, 77, 101,
          115, 115, 97, 103, 101, 76, 105, 116, 101, 36, 83, 101, 114, 105, 97, 108, 105, 122, 101,
          100, 70, 111, 114, 109, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 3, 91, 0, 7, 97, 115, 66, 121, 116,
          101, 115, 116, 0, 2, 91, 66, 76, 0, 12, 109, 101, 115, 115, 97, 103, 101, 67, 108, 97,
          115, 115, 116, 0, 17, 76, 106, 97, 118, 97, 47, 108, 97, 110, 103, 47, 67, 108, 97, 115,
          115, 59, 76, 0, 16, 109, 101, 115, 115, 97, 103, 101, 67, 108, 97, 115, 115, 78, 97, 109,
          101, 116, 0, 18, 76, 106, 97, 118, 97, 47, 108, 97, 110, 103, 47, 83, 116, 114, 105, 110,
          103, 59, 120, 112, 117, 114, 0, 2, 91, 66, -84, -13, 23, -8, 6, 8, 84, -32, 2, 0, 0, 120,
          112, 0, 0, 0, 2, 8, 123, 118, 114, 0, 30, 99, 111, 109, 46, 103, 111, 111, 103, 108, 101,
          46, 112, 114, 111, 116, 111, 98, 117, 102, 46, 73, 110, 116, 51, 50, 86, 97, 108, 117,
          101, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 2, 66, 0, 21, 109, 101, 109, 111, 105, 122, 101, 100,
          73, 115, 73, 110, 105, 116, 105, 97, 108, 105, 122, 101, 100, 73, 0, 6, 118, 97, 108, 117,
          101, 95, 120, 114, 0, 38, 99, 111, 109, 46, 103, 111, 111, 103, 108, 101, 46, 112, 114,
          111, 116, 111, 98, 117, 102, 46, 71, 101, 110, 101, 114, 97, 116, 101, 100, 77, 101, 115,
          115, 97, 103, 101, 86, 51, 0, 0, 0, 0, 0, 0, 0, 1, 2, 0, 1, 76, 0, 13, 117, 110, 107, 110,
          111, 119, 110, 70, 105, 101, 108, 100, 115, 116, 0, 37, 76, 99, 111, 109, 47, 103, 111,
          111, 103, 108, 101, 47, 112, 114, 111, 116, 111, 98, 117, 102, 47, 85, 110, 107, 110, 111,
          119, 110, 70, 105, 101, 108, 100, 83, 101, 116, 59, 120, 112, 116, 0, 30, 99, 111, 109,
          46, 103, 111, 111, 103, 108, 101, 46, 112, 114, 111, 116, 111, 98, 117, 102, 46, 73, 110,
          116, 51, 50, 86, 97, 108, 117, 101
        };

    ByteArrayInputStream bais = new ByteArrayInputStream(int32ValueBytes);
    ObjectInputStream in = new ObjectInputStream(bais);
    Int32Value int32Value = (Int32Value) in.readObject();
    assertThat(int32Value.getValue()).isEqualTo(123);
  }

  @Test
  public void testEnumValues() {
    assertThat(TestAllTypes.NestedEnum.BAR.getNumber())
        .isEqualTo(TestAllTypes.NestedEnum.BAR_VALUE);
    assertThat(TestAllTypes.NestedEnum.BAZ.getNumber())
        .isEqualTo(TestAllTypes.NestedEnum.BAZ_VALUE);
    assertThat(TestAllTypes.NestedEnum.FOO.getNumber())
        .isEqualTo(TestAllTypes.NestedEnum.FOO_VALUE);
  }

  @Test
  public void testNonNestedExtensionInitialization() {
    assertThat(NonNestedExtension.nonNestedExtension.getMessageDefaultInstance())
        .isInstanceOf(MyNonNestedExtension.class);
    assertThat(NonNestedExtension.nonNestedExtension.getDescriptor().getName())
        .isEqualTo("nonNestedExtension");
  }

  @Test
  public void testNestedExtensionInitialization() {
    assertThat(MyNestedExtension.recursiveExtension.getMessageDefaultInstance())
        .isInstanceOf(MessageToBeExtended.class);
    assertThat(MyNestedExtension.recursiveExtension.getDescriptor().getName())
        .isEqualTo("recursiveExtension");
  }

  @Test
  public void testInvalidations() throws Exception {
    GeneratedMessageV3.setAlwaysUseFieldBuildersForTesting(true);
    TestAllTypes.NestedMessage nestedMessage1 = TestAllTypes.NestedMessage.newBuilder().build();
    TestAllTypes.NestedMessage nestedMessage2 = TestAllTypes.NestedMessage.newBuilder().build();

    // Set all three flavors (enum, primitive, message and singular/repeated)
    // and verify no invalidations fired
    TestUtil.MockBuilderParent mockParent = new TestUtil.MockBuilderParent();

    TestAllTypes.Builder builder =
        (TestAllTypes.Builder)
            ((AbstractMessage) TestAllTypes.getDefaultInstance()).newBuilderForType(mockParent);
    builder.setOptionalInt32(1);
    builder.setOptionalNestedEnum(TestAllTypes.NestedEnum.BAR);
    builder.setOptionalNestedMessage(nestedMessage1);
    builder.addRepeatedInt32(1);
    builder.addRepeatedNestedEnum(TestAllTypes.NestedEnum.BAR);
    builder.addRepeatedNestedMessage(nestedMessage1);
    assertThat(mockParent.getInvalidationCount()).isEqualTo(0);

    // Now tell it we want changes and make sure it's only fired once
    // And do this for each flavor

    // primitive single
    builder.buildPartial();
    builder.setOptionalInt32(2);
    builder.setOptionalInt32(3);
    assertThat(mockParent.getInvalidationCount()).isEqualTo(1);

    // enum single
    builder.buildPartial();
    builder.setOptionalNestedEnum(TestAllTypes.NestedEnum.BAZ);
    builder.setOptionalNestedEnum(TestAllTypes.NestedEnum.BAR);
    assertThat(mockParent.getInvalidationCount()).isEqualTo(2);

    // message single
    builder.buildPartial();
    builder.setOptionalNestedMessage(nestedMessage2);
    builder.setOptionalNestedMessage(nestedMessage1);
    assertThat(mockParent.getInvalidationCount()).isEqualTo(3);

    // primitive repeated
    builder.buildPartial();
    builder.addRepeatedInt32(2);
    builder.addRepeatedInt32(3);
    assertThat(mockParent.getInvalidationCount()).isEqualTo(4);

    // enum repeated
    builder.buildPartial();
    builder.addRepeatedNestedEnum(TestAllTypes.NestedEnum.BAZ);
    builder.addRepeatedNestedEnum(TestAllTypes.NestedEnum.BAZ);
    assertThat(mockParent.getInvalidationCount()).isEqualTo(5);

    // message repeated
    builder.buildPartial();
    builder.addRepeatedNestedMessage(nestedMessage2);
    builder.addRepeatedNestedMessage(nestedMessage1);
    assertThat(mockParent.getInvalidationCount()).isEqualTo(6);
  }

  @Test
  public void testInvalidations_Extensions() throws Exception {
    TestUtil.MockBuilderParent mockParent = new TestUtil.MockBuilderParent();

    TestAllExtensions.Builder builder =
        (TestAllExtensions.Builder)
            ((AbstractMessage) TestAllExtensions.getDefaultInstance())
                .newBuilderForType(mockParent);

    builder.addExtension(UnittestProto.repeatedInt32Extension, 1);
    builder.setExtension(UnittestProto.repeatedInt32Extension, 0, 2);
    builder.clearExtension(UnittestProto.repeatedInt32Extension);
    assertThat(mockParent.getInvalidationCount()).isEqualTo(0);

    // Now tell it we want changes and make sure it's only fired once
    builder.buildPartial();
    builder.addExtension(UnittestProto.repeatedInt32Extension, 2);
    builder.addExtension(UnittestProto.repeatedInt32Extension, 3);
    assertThat(mockParent.getInvalidationCount()).isEqualTo(1);

    builder.buildPartial();
    builder.setExtension(UnittestProto.repeatedInt32Extension, 0, 4);
    builder.setExtension(UnittestProto.repeatedInt32Extension, 1, 5);
    assertThat(mockParent.getInvalidationCount()).isEqualTo(2);

    builder.buildPartial();
    builder.clearExtension(UnittestProto.repeatedInt32Extension);
    builder.clearExtension(UnittestProto.repeatedInt32Extension);
    assertThat(mockParent.getInvalidationCount()).isEqualTo(3);
  }

  @Test
  public void testBaseMessageOrBuilder() {
    // Mostly just makes sure the base interface exists and has some methods.
    TestAllTypes.Builder builder = TestAllTypes.newBuilder();
    TestAllTypes message = builder.buildPartial();
    TestAllTypesOrBuilder messageAsInterface = (TestAllTypesOrBuilder) message;

    assertThat(messageAsInterface.getDefaultBool()).isEqualTo(messageAsInterface.getDefaultBool());
    assertThat(messageAsInterface.getOptionalDouble())
        .isEqualTo(messageAsInterface.getOptionalDouble());
  }

  @Test
  public void testMessageOrBuilderGetters() {
    TestAllTypes.Builder builder = TestAllTypes.newBuilder();

    // single fields
    assertThat(ForeignMessage.getDefaultInstance())
        .isSameInstanceAs(builder.getOptionalForeignMessageOrBuilder());
    ForeignMessage.Builder subBuilder = builder.getOptionalForeignMessageBuilder();
    assertThat(subBuilder).isSameInstanceAs(builder.getOptionalForeignMessageOrBuilder());

    // repeated fields
    ForeignMessage m0 = ForeignMessage.newBuilder().buildPartial();
    ForeignMessage m1 = ForeignMessage.newBuilder().buildPartial();
    ForeignMessage m2 = ForeignMessage.newBuilder().buildPartial();
    builder.addRepeatedForeignMessage(m0);
    builder.addRepeatedForeignMessage(m1);
    builder.addRepeatedForeignMessage(m2);
    assertThat(m0).isSameInstanceAs(builder.getRepeatedForeignMessageOrBuilder(0));
    assertThat(m1).isSameInstanceAs(builder.getRepeatedForeignMessageOrBuilder(1));
    assertThat(m2).isSameInstanceAs(builder.getRepeatedForeignMessageOrBuilder(2));
    ForeignMessage.Builder b0 = builder.getRepeatedForeignMessageBuilder(0);
    ForeignMessage.Builder b1 = builder.getRepeatedForeignMessageBuilder(1);
    assertThat(b0).isSameInstanceAs(builder.getRepeatedForeignMessageOrBuilder(0));
    assertThat(b1).isSameInstanceAs(builder.getRepeatedForeignMessageOrBuilder(1));
    assertThat(m2).isSameInstanceAs(builder.getRepeatedForeignMessageOrBuilder(2));

    List<? extends ForeignMessageOrBuilder> messageOrBuilderList =
        builder.getRepeatedForeignMessageOrBuilderList();
    assertThat(b0).isSameInstanceAs(messageOrBuilderList.get(0));
    assertThat(b1).isSameInstanceAs(messageOrBuilderList.get(1));
    assertThat(m2).isSameInstanceAs(messageOrBuilderList.get(2));
  }

  @Test
  public void testGetFieldBuilder() {
    Descriptor descriptor = TestAllTypes.getDescriptor();

    FieldDescriptor fieldDescriptor = descriptor.findFieldByName("optional_nested_message");
    FieldDescriptor foreignFieldDescriptor = descriptor.findFieldByName("optional_foreign_message");
    FieldDescriptor importFieldDescriptor = descriptor.findFieldByName("optional_import_message");

    // Mutate the message with new field builder
    // Mutate nested message
    TestAllTypes.Builder builder1 = TestAllTypes.newBuilder();
    Message.Builder fieldBuilder1 =
        builder1
            .newBuilderForField(fieldDescriptor)
            .mergeFrom((Message) builder1.getField(fieldDescriptor));
    fieldBuilder1.setField(NESTED_MESSAGE_BB_FIELD, 1);
    builder1.setField(fieldDescriptor, fieldBuilder1.build());

    // Mutate foreign message
    Message.Builder foreignFieldBuilder1 =
        builder1
            .newBuilderForField(foreignFieldDescriptor)
            .mergeFrom((Message) builder1.getField(foreignFieldDescriptor));
    FieldDescriptor subForeignFieldDescriptor1 =
        foreignFieldBuilder1.getDescriptorForType().findFieldByName("c");
    foreignFieldBuilder1.setField(subForeignFieldDescriptor1, 2);
    builder1.setField(foreignFieldDescriptor, foreignFieldBuilder1.build());

    // Mutate import message
    Message.Builder importFieldBuilder1 =
        builder1
            .newBuilderForField(importFieldDescriptor)
            .mergeFrom((Message) builder1.getField(importFieldDescriptor));
    FieldDescriptor subImportFieldDescriptor1 =
        importFieldBuilder1.getDescriptorForType().findFieldByName("d");
    importFieldBuilder1.setField(subImportFieldDescriptor1, 3);
    builder1.setField(importFieldDescriptor, importFieldBuilder1.build());

    Message newMessage1 = builder1.build();

    // Mutate the message with existing field builder
    // Mutate nested message
    TestAllTypes.Builder builder2 = TestAllTypes.newBuilder();
    Message.Builder fieldBuilder2 = builder2.getFieldBuilder(fieldDescriptor);
    fieldBuilder2.setField(NESTED_MESSAGE_BB_FIELD, 1);
    builder2.setField(fieldDescriptor, fieldBuilder2.build());

    // Mutate foreign message
    Message.Builder foreignFieldBuilder2 =
        builder2
            .newBuilderForField(foreignFieldDescriptor)
            .mergeFrom((Message) builder2.getField(foreignFieldDescriptor));
    FieldDescriptor subForeignFieldDescriptor2 =
        foreignFieldBuilder2.getDescriptorForType().findFieldByName("c");
    foreignFieldBuilder2.setField(subForeignFieldDescriptor2, 2);
    builder2.setField(foreignFieldDescriptor, foreignFieldBuilder2.build());

    // Mutate import message
    Message.Builder importFieldBuilder2 =
        builder2
            .newBuilderForField(importFieldDescriptor)
            .mergeFrom((Message) builder2.getField(importFieldDescriptor));
    FieldDescriptor subImportFieldDescriptor2 =
        importFieldBuilder2.getDescriptorForType().findFieldByName("d");
    importFieldBuilder2.setField(subImportFieldDescriptor2, 3);
    builder2.setField(importFieldDescriptor, importFieldBuilder2.build());

    Message newMessage2 = builder2.build();

    // These two messages should be equal.
    assertThat(newMessage1).isEqualTo(newMessage2);
  }

  @Test
  public void testGetFieldBuilderWithInitializedValue() {
    Descriptor descriptor = TestAllTypes.getDescriptor();
    FieldDescriptor fieldDescriptor = descriptor.findFieldByName("optional_nested_message");

    // Before setting field, builder is initialized by default value.
    TestAllTypes.Builder builder = TestAllTypes.newBuilder();
    NestedMessage.Builder fieldBuilder =
        (NestedMessage.Builder) builder.getFieldBuilder(fieldDescriptor);
    assertThat(fieldBuilder.getBb()).isEqualTo(0);

    // Setting field value with new field builder instance.
    builder = TestAllTypes.newBuilder();
    NestedMessage.Builder newFieldBuilder = builder.getOptionalNestedMessageBuilder();
    newFieldBuilder.setBb(2);
    // Then get the field builder instance by getFieldBuilder().
    fieldBuilder = (NestedMessage.Builder) builder.getFieldBuilder(fieldDescriptor);
    // It should contain new value.
    assertThat(fieldBuilder.getBb()).isEqualTo(2);
    // These two builder should be equal.
    assertThat(fieldBuilder).isSameInstanceAs(newFieldBuilder);
  }

  @Test
  public void testGetFieldBuilderNotSupportedException() {
    Descriptor descriptor = TestAllTypes.getDescriptor();
    TestAllTypes.Builder builder = TestAllTypes.newBuilder();
    try {
      builder.getFieldBuilder(descriptor.findFieldByName("optional_int32"));
      assertWithMessage("Exception was not thrown").fail();
    } catch (UnsupportedOperationException e) {
      // We expect this exception.
    }
    try {
      builder.getFieldBuilder(descriptor.findFieldByName("optional_nested_enum"));
      assertWithMessage("Exception was not thrown").fail();
    } catch (UnsupportedOperationException e) {
      // We expect this exception.
    }
    try {
      builder.getFieldBuilder(descriptor.findFieldByName("repeated_int32"));
      assertWithMessage("Exception was not thrown").fail();
    } catch (UnsupportedOperationException e) {
      // We expect this exception.
    }
    try {
      builder.getFieldBuilder(descriptor.findFieldByName("repeated_nested_enum"));
      assertWithMessage("Exception was not thrown").fail();
    } catch (UnsupportedOperationException e) {
      // We expect this exception.
    }
    try {
      builder.getFieldBuilder(descriptor.findFieldByName("repeated_nested_message"));
      assertWithMessage("Exception was not thrown").fail();
    } catch (UnsupportedOperationException e) {
      // We expect this exception.
    }
  }

  // Test that when the default outer class name conflicts with another type
  // defined in the proto the compiler will append a suffix to avoid the
  // conflict.
  @Test
  public void testConflictingOuterClassName() {
    // We just need to make sure we can refer to the outer class with the
    // expected name. There is nothing else to test.
    OuterClassNameTestOuterClass.OuterClassNameTest message =
        OuterClassNameTestOuterClass.OuterClassNameTest.newBuilder().build();
    assertThat(message.getDescriptorForType())
        .isSameInstanceAs(OuterClassNameTestOuterClass.OuterClassNameTest.getDescriptor());

    OuterClassNameTest2OuterClass.TestMessage2.NestedMessage.OuterClassNameTest2 message2 =
        OuterClassNameTest2OuterClass.TestMessage2.NestedMessage.OuterClassNameTest2.newBuilder()
            .build();
    assertThat(message2.getSerializedSize()).isEqualTo(0);

    OuterClassNameTest3OuterClass.TestMessage3.NestedMessage.OuterClassNameTest3 enumValue =
        OuterClassNameTest3OuterClass.TestMessage3.NestedMessage.OuterClassNameTest3.DUMMY_VALUE;
    assertThat(enumValue.getNumber()).isEqualTo(1);
  }

  // =================================================================
  // oneof generated code test
  @Test
  public void testOneofEnumCase() throws Exception {
    TestOneof2 message =
        TestOneof2.newBuilder().setFooInt(123).setFooString("foo").setFooCord("bar").build();
    TestUtil.assertAtMostOneFieldSetOneof(message);
  }

  @Test
  public void testClearOneof() throws Exception {
    TestOneof2.Builder builder = TestOneof2.newBuilder().setFooInt(123);
    assertThat(builder.getFooCase()).isEqualTo(TestOneof2.FooCase.FOO_INT);
    builder.clearFoo();
    assertThat(builder.getFooCase()).isEqualTo(TestOneof2.FooCase.FOO_NOT_SET);
  }

  @Test
  public void testSetOneofClearsOthers() throws Exception {
    TestOneof2.Builder builder = TestOneof2.newBuilder();
    TestOneof2 message = builder.setFooInt(123).setFooString("foo").buildPartial();
    assertThat(message.hasFooString()).isTrue();
    TestUtil.assertAtMostOneFieldSetOneof(message);

    message = builder.setFooCord("bar").buildPartial();
    assertThat(message.hasFooCord()).isTrue();
    TestUtil.assertAtMostOneFieldSetOneof(message);

    message = builder.setFooStringPiece("baz").buildPartial();
    assertThat(message.hasFooStringPiece()).isTrue();
    TestUtil.assertAtMostOneFieldSetOneof(message);

    message = builder.setFooBytes(TestUtil.toBytes("moo")).buildPartial();
    assertThat(message.hasFooBytes()).isTrue();
    TestUtil.assertAtMostOneFieldSetOneof(message);

    message = builder.setFooEnum(TestOneof2.NestedEnum.FOO).buildPartial();
    assertThat(message.hasFooEnum()).isTrue();
    TestUtil.assertAtMostOneFieldSetOneof(message);

    message =
        builder
            .setFooMessage(TestOneof2.NestedMessage.newBuilder().setMooInt(234).build())
            .buildPartial();
    assertThat(message.hasFooMessage()).isTrue();
    TestUtil.assertAtMostOneFieldSetOneof(message);

    message = builder.setFooInt(123).buildPartial();
    assertThat(message.hasFooInt()).isTrue();
    TestUtil.assertAtMostOneFieldSetOneof(message);
  }

  @Test
  public void testOneofTypes() throws Exception {
    // Primitive
    {
      TestOneof2.Builder builder = TestOneof2.newBuilder();
      assertThat(builder.getFooInt()).isEqualTo(0);
      assertThat(builder.hasFooInt()).isFalse();
      assertThat(builder.setFooInt(123).hasFooInt()).isTrue();
      assertThat(builder.getFooInt()).isEqualTo(123);
      TestOneof2 message = builder.buildPartial();
      assertThat(message.hasFooInt()).isTrue();
      assertThat(message.getFooInt()).isEqualTo(123);

      assertThat(builder.clearFooInt().hasFooInt()).isFalse();
      TestOneof2 message2 = builder.build();
      assertThat(message2.hasFooInt()).isFalse();
      assertThat(message2.getFooInt()).isEqualTo(0);
    }

    // Enum
    {
      TestOneof2.Builder builder = TestOneof2.newBuilder();
      assertThat(builder.getFooEnum()).isEqualTo(TestOneof2.NestedEnum.FOO);
      assertThat(builder.setFooEnum(TestOneof2.NestedEnum.BAR).hasFooEnum()).isTrue();
      assertThat(builder.getFooEnum()).isEqualTo(TestOneof2.NestedEnum.BAR);
      TestOneof2 message = builder.buildPartial();
      assertThat(message.hasFooEnum()).isTrue();
      assertThat(message.getFooEnum()).isEqualTo(TestOneof2.NestedEnum.BAR);

      assertThat(builder.clearFooEnum().hasFooEnum()).isFalse();
      TestOneof2 message2 = builder.build();
      assertThat(message2.hasFooEnum()).isFalse();
      assertThat(message2.getFooEnum()).isEqualTo(TestOneof2.NestedEnum.FOO);
    }

    // String
    {
      TestOneof2.Builder builder = TestOneof2.newBuilder();
      assertThat(builder.getFooString()).isEmpty();
      builder.setFooString("foo");
      assertThat(builder.hasFooString()).isTrue();
      assertThat(builder.getFooString()).isEqualTo("foo");
      TestOneof2 message = builder.buildPartial();
      assertThat(message.hasFooString()).isTrue();
      assertThat(message.getFooString()).isEqualTo("foo");
      assertThat(TestUtil.toBytes("foo")).isEqualTo(message.getFooStringBytes());

      assertThat(builder.clearFooString().hasFooString()).isFalse();
      TestOneof2 message2 = builder.buildPartial();
      assertThat(message2.hasFooString()).isFalse();
      assertThat(message2.getFooString()).isEmpty();
      assertThat(message2.getFooStringBytes()).isEqualTo(TestUtil.toBytes(""));

      // Get method should not change the oneof value.
      builder.setFooInt(123);
      assertThat(builder.getFooString()).isEmpty();
      assertThat(builder.getFooStringBytes()).isEqualTo(TestUtil.toBytes(""));
      assertThat(builder.getFooInt()).isEqualTo(123);

      message = builder.build();
      assertThat(message.getFooString()).isEmpty();
      assertThat(TestUtil.toBytes("")).isEqualTo(message.getFooStringBytes());
      assertThat(message.getFooInt()).isEqualTo(123);
    }

    // Cord
    {
      TestOneof2.Builder builder = TestOneof2.newBuilder();
      assertThat(builder.getFooCord()).isEmpty();
      builder.setFooCord("foo");
      assertThat(builder.hasFooCord()).isTrue();
      assertThat(builder.getFooCord()).isEqualTo("foo");
      TestOneof2 message = builder.buildPartial();
      assertThat(message.hasFooCord()).isTrue();
      assertThat(message.getFooCord()).isEqualTo("foo");
      assertThat(TestUtil.toBytes("foo")).isEqualTo(message.getFooCordBytes());

      assertThat(builder.clearFooCord().hasFooCord()).isFalse();
      TestOneof2 message2 = builder.build();
      assertThat(message2.hasFooCord()).isFalse();
      assertThat(message2.getFooCord()).isEmpty();
      assertThat(message2.getFooCordBytes()).isEqualTo(TestUtil.toBytes(""));
    }

    // StringPiece
    {
      TestOneof2.Builder builder = TestOneof2.newBuilder();
      assertThat(builder.getFooStringPiece()).isEmpty();
      builder.setFooStringPiece("foo");
      assertThat(builder.hasFooStringPiece()).isTrue();
      assertThat(builder.getFooStringPiece()).isEqualTo("foo");
      TestOneof2 message = builder.buildPartial();
      assertThat(message.hasFooStringPiece()).isTrue();
      assertThat(message.getFooStringPiece()).isEqualTo("foo");
      assertThat(TestUtil.toBytes("foo")).isEqualTo(message.getFooStringPieceBytes());

      assertThat(builder.clearFooStringPiece().hasFooStringPiece()).isFalse();
      TestOneof2 message2 = builder.build();
      assertThat(message2.hasFooStringPiece()).isFalse();
      assertThat(message2.getFooStringPiece()).isEmpty();
      assertThat(message2.getFooStringPieceBytes()).isEqualTo(TestUtil.toBytes(""));
    }

    // Message
    {
      // set
      TestOneof2.Builder builder = TestOneof2.newBuilder();
      assertThat(builder.getFooMessage().getMooInt()).isEqualTo(0);
      builder.setFooMessage(TestOneof2.NestedMessage.newBuilder().setMooInt(234).build());
      assertThat(builder.hasFooMessage()).isTrue();
      assertThat(builder.getFooMessage().getMooInt()).isEqualTo(234);
      TestOneof2 message = builder.buildPartial();
      assertThat(message.hasFooMessage()).isTrue();
      assertThat(message.getFooMessage().getMooInt()).isEqualTo(234);

      // clear
      assertThat(builder.clearFooMessage().hasFooString()).isFalse();
      message = builder.build();
      assertThat(message.hasFooMessage()).isFalse();
      assertThat(message.getFooMessage().getMooInt()).isEqualTo(0);

      // nested builder
      builder = TestOneof2.newBuilder();
      assertThat(builder.getFooMessageOrBuilder())
          .isSameInstanceAs(TestOneof2.NestedMessage.getDefaultInstance());
      assertThat(builder.hasFooMessage()).isFalse();
      builder.getFooMessageBuilder().setMooInt(123);
      assertThat(builder.hasFooMessage()).isTrue();
      assertThat(builder.getFooMessage().getMooInt()).isEqualTo(123);
      message = builder.build();
      assertThat(message.hasFooMessage()).isTrue();
      assertThat(message.getFooMessage().getMooInt()).isEqualTo(123);
    }

    // LazyMessage is tested in LazyMessageLiteTest.java
  }

  @Test
  public void testOneofMergeNonMessage() throws Exception {
    // Primitive Type
    {
      TestOneof2.Builder builder = TestOneof2.newBuilder();
      TestOneof2 message = builder.setFooInt(123).build();
      TestOneof2 message2 = TestOneof2.newBuilder().mergeFrom(message).build();
      assertThat(message2.hasFooInt()).isTrue();
      assertThat(message2.getFooInt()).isEqualTo(123);
    }

    // String
    {
      TestOneof2.Builder builder = TestOneof2.newBuilder();
      TestOneof2 message = builder.setFooString("foo").build();
      TestOneof2 message2 = TestOneof2.newBuilder().mergeFrom(message).build();
      assertThat(message2.hasFooString()).isTrue();
      assertThat(message2.getFooString()).isEqualTo("foo");
    }

    // Enum
    {
      TestOneof2.Builder builder = TestOneof2.newBuilder();
      TestOneof2 message = builder.setFooEnum(TestOneof2.NestedEnum.BAR).build();
      TestOneof2 message2 = TestOneof2.newBuilder().mergeFrom(message).build();
      assertThat(message2.hasFooEnum()).isTrue();
      assertThat(message2.getFooEnum()).isEqualTo(TestOneof2.NestedEnum.BAR);
    }
  }

  @Test
  public void testOneofMergeMessage_mergeIntoNewBuilder() {
    TestOneof2.Builder builder = TestOneof2.newBuilder();
    TestOneof2 message =
        builder.setFooMessage(TestOneof2.NestedMessage.newBuilder().setMooInt(234).build()).build();
    TestOneof2 message2 = TestOneof2.newBuilder().mergeFrom(message).build();
    assertThat(message2.hasFooMessage()).isTrue();
    assertThat(message2.getFooMessage().getMooInt()).isEqualTo(234);
  }

  @Test
  public void testOneofMergeMessage_mergeWithGetMessageBuilder() {
    TestOneof2.Builder builder = TestOneof2.newBuilder();
    builder.getFooMessageBuilder().addCorgeInt(1);
    assertThat(builder.mergeFrom(MESSAGE_TO_MERGE_FROM).build()).isEqualTo(EXPECTED_MERGED_MESSAGE);
  }

  @Test
  public void testOneofMergeMessage_mergeIntoMessageBuiltWithGetMessageBuilder() {
    TestOneof2.Builder builder = TestOneof2.newBuilder();
    builder.getFooMessageBuilder().addCorgeInt(1);
    TestOneof2 message = builder.build();
    assertThat(message.toBuilder().mergeFrom(MESSAGE_TO_MERGE_FROM).build())
        .isEqualTo(EXPECTED_MERGED_MESSAGE);
  }

  @Test
  public void testOneofMergeMessage_mergeWithoutGetMessageBuilder() {
    TestOneof2.Builder builder =
        TestOneof2.newBuilder().setFooMessage(TestOneof2.NestedMessage.newBuilder().addCorgeInt(1));
    assertThat(builder.mergeFrom(MESSAGE_TO_MERGE_FROM).build()).isEqualTo(EXPECTED_MERGED_MESSAGE);
  }

  @Test
  public void testOneofSerialization() throws Exception {
    // Primitive Type
    {
      TestOneof2.Builder builder = TestOneof2.newBuilder();
      TestOneof2 message = builder.setFooInt(123).build();
      ByteString serialized = message.toByteString();
      TestOneof2 message2 = TestOneof2.parseFrom(serialized);
      assertThat(message2.hasFooInt()).isTrue();
      assertThat(message2.getFooInt()).isEqualTo(123);
    }

    // String
    {
      TestOneof2.Builder builder = TestOneof2.newBuilder();
      TestOneof2 message = builder.setFooString("foo").build();
      ByteString serialized = message.toByteString();
      TestOneof2 message2 = TestOneof2.parseFrom(serialized);
      assertThat(message2.hasFooString()).isTrue();
      assertThat(message2.getFooString()).isEqualTo("foo");
    }

    // Enum
    {
      TestOneof2.Builder builder = TestOneof2.newBuilder();
      TestOneof2 message = builder.setFooEnum(TestOneof2.NestedEnum.BAR).build();
      ByteString serialized = message.toByteString();
      TestOneof2 message2 = TestOneof2.parseFrom(serialized);
      assertThat(message2.hasFooEnum()).isTrue();
      assertThat(message2.getFooEnum()).isEqualTo(TestOneof2.NestedEnum.BAR);
    }

    // Message
    {
      TestOneof2.Builder builder = TestOneof2.newBuilder();
      TestOneof2 message =
          builder
              .setFooMessage(TestOneof2.NestedMessage.newBuilder().setMooInt(234).build())
              .build();
      ByteString serialized = message.toByteString();
      TestOneof2 message2 = TestOneof2.parseFrom(serialized);
      assertThat(message2.hasFooMessage()).isTrue();
      assertThat(message2.getFooMessage().getMooInt()).isEqualTo(234);
    }
  }

  @Test
  public void testOneofNestedBuilderOnChangePropagation() {
    NestedTestAllTypes.Builder parentBuilder = NestedTestAllTypes.newBuilder();
    TestAllTypes.Builder builder = parentBuilder.getPayloadBuilder();
    builder.getOneofNestedMessageBuilder();
    assertThat(builder.hasOneofNestedMessage()).isTrue();
    assertThat(parentBuilder.hasPayload()).isTrue();
    NestedTestAllTypes message = parentBuilder.build();
    assertThat(message.hasPayload()).isTrue();
    assertThat(message.getPayload().hasOneofNestedMessage()).isTrue();
  }

  @Test
  public void testOneofParseFailurePropagatesUnderlyingException() {
    final byte[] bytes = TestOneof2.newBuilder().setFooInt(123).build().toByteArray();
    final IOException injectedException = new IOException("oh no");
    CodedInputStream failingInputStream =
        CodedInputStream.newInstance(
            new InputStream() {
              boolean first = true;

              @Override
              public int read(byte[] b, int off, int len) throws IOException {
                if (!first) {
                  throw injectedException;
                }
                first = false;
                System.arraycopy(bytes, 0, b, off, len);
                return len;
              }

              @Override
              public int read() {
                throw new UnsupportedOperationException();
              }
            },
            bytes.length - 1);
    TestOneof2.Builder builder = TestOneof2.newBuilder();

    try {
      builder.mergeFrom(failingInputStream, ExtensionRegistry.getEmptyRegistry());
      assertWithMessage("Expected mergeFrom to fail").fail();
    } catch (IOException e) {
      assertThat(e).isSameInstanceAs(injectedException);
    }
  }

  @Test
  public void testGetRepeatedFieldBuilder() {
    Descriptor descriptor = TestAllTypes.getDescriptor();

    FieldDescriptor fieldDescriptor = descriptor.findFieldByName("repeated_nested_message");
    FieldDescriptor foreignFieldDescriptor = descriptor.findFieldByName("repeated_foreign_message");
    FieldDescriptor importFieldDescriptor = descriptor.findFieldByName("repeated_import_message");

    // Mutate the message with new field builder
    // Mutate nested message
    TestAllTypes.Builder builder1 = TestAllTypes.newBuilder();
    Message.Builder fieldBuilder1 = builder1.newBuilderForField(fieldDescriptor);
    fieldBuilder1.setField(NESTED_MESSAGE_BB_FIELD, 1);
    builder1.addRepeatedField(fieldDescriptor, fieldBuilder1.build());

    // Mutate foreign message
    Message.Builder foreignFieldBuilder1 = builder1.newBuilderForField(foreignFieldDescriptor);
    FieldDescriptor subForeignFieldDescriptor1 =
        foreignFieldBuilder1.getDescriptorForType().findFieldByName("c");
    foreignFieldBuilder1.setField(subForeignFieldDescriptor1, 2);
    builder1.addRepeatedField(foreignFieldDescriptor, foreignFieldBuilder1.build());

    // Mutate import message
    Message.Builder importFieldBuilder1 = builder1.newBuilderForField(importFieldDescriptor);
    FieldDescriptor subImportFieldDescriptor1 =
        importFieldBuilder1.getDescriptorForType().findFieldByName("d");
    importFieldBuilder1.setField(subImportFieldDescriptor1, 3);
    builder1.addRepeatedField(importFieldDescriptor, importFieldBuilder1.build());

    Message newMessage1 = builder1.build();

    // Mutate the message with existing field builder
    // Mutate nested message
    TestAllTypes.Builder builder2 = TestAllTypes.newBuilder();
    builder2.addRepeatedNestedMessageBuilder();
    Message.Builder fieldBuilder2 = builder2.getRepeatedFieldBuilder(fieldDescriptor, 0);
    fieldBuilder2.setField(NESTED_MESSAGE_BB_FIELD, 1);

    // Mutate foreign message
    Message.Builder foreignFieldBuilder2 = builder2.newBuilderForField(foreignFieldDescriptor);
    FieldDescriptor subForeignFieldDescriptor2 =
        foreignFieldBuilder2.getDescriptorForType().findFieldByName("c");
    foreignFieldBuilder2.setField(subForeignFieldDescriptor2, 2);
    builder2.addRepeatedField(foreignFieldDescriptor, foreignFieldBuilder2.build());

    // Mutate import message
    Message.Builder importFieldBuilder2 = builder2.newBuilderForField(importFieldDescriptor);
    FieldDescriptor subImportFieldDescriptor2 =
        importFieldBuilder2.getDescriptorForType().findFieldByName("d");
    importFieldBuilder2.setField(subImportFieldDescriptor2, 3);
    builder2.addRepeatedField(importFieldDescriptor, importFieldBuilder2.build());

    Message newMessage2 = builder2.build();

    // These two messages should be equal.
    assertThat(newMessage1).isEqualTo(newMessage2);
  }

  @Test
  public void testGetRepeatedFieldBuilderWithInitializedValue() {
    Descriptor descriptor = TestAllTypes.getDescriptor();
    FieldDescriptor fieldDescriptor = descriptor.findFieldByName("repeated_nested_message");

    // Before setting field, builder is initialized by default value.
    TestAllTypes.Builder builder = TestAllTypes.newBuilder();
    builder.addRepeatedNestedMessageBuilder();
    NestedMessage.Builder fieldBuilder =
        (NestedMessage.Builder) builder.getRepeatedFieldBuilder(fieldDescriptor, 0);
    assertThat(fieldBuilder.getBb()).isEqualTo(0);

    // Setting field value with new field builder instance.
    builder = TestAllTypes.newBuilder();
    NestedMessage.Builder newFieldBuilder = builder.addRepeatedNestedMessageBuilder();
    newFieldBuilder.setBb(2);
    // Then get the field builder instance by getRepeatedFieldBuilder().
    fieldBuilder = (NestedMessage.Builder) builder.getRepeatedFieldBuilder(fieldDescriptor, 0);
    // It should contain new value.
    assertThat(fieldBuilder.getBb()).isEqualTo(2);
    // These two builder should be equal.
    assertThat(fieldBuilder).isSameInstanceAs(newFieldBuilder);
  }

  @Test
  public void testGetRepeatedFieldBuilderNotSupportedException() {
    Descriptor descriptor = TestAllTypes.getDescriptor();
    TestAllTypes.Builder builder = TestAllTypes.newBuilder();
    try {
      builder.getRepeatedFieldBuilder(descriptor.findFieldByName("repeated_int32"), 0);
      assertWithMessage("Exception was not thrown").fail();
    } catch (UnsupportedOperationException e) {
      // We expect this exception.
    }
    try {
      builder.getRepeatedFieldBuilder(descriptor.findFieldByName("repeated_nested_enum"), 0);
      assertWithMessage("Exception was not thrown").fail();
    } catch (UnsupportedOperationException e) {
      // We expect this exception.
    }
    try {
      builder.getRepeatedFieldBuilder(descriptor.findFieldByName("optional_int32"), 0);
      assertWithMessage("Exception was not thrown").fail();
    } catch (UnsupportedOperationException e) {
      // We expect this exception.
    }
    try {
      builder.getRepeatedFieldBuilder(descriptor.findFieldByName("optional_nested_enum"), 0);
      assertWithMessage("Exception was not thrown").fail();
    } catch (UnsupportedOperationException e) {
      // We expect this exception.
    }
    try {
      builder.getRepeatedFieldBuilder(descriptor.findFieldByName("optional_nested_message"), 0);
      assertWithMessage("Exception was not thrown").fail();
    } catch (UnsupportedOperationException e) {
      // We expect this exception.
    }
  }

  private static final FieldDescriptor OPTIONAL_NESTED_MESSAGE_EXTENSION =
      UnittestProto.getDescriptor().findExtensionByName("optional_nested_message_extension");
  private static final FieldDescriptor REPEATED_NESTED_MESSAGE_EXTENSION =
      UnittestProto.getDescriptor().findExtensionByName("repeated_nested_message_extension");
  // A compile-time check that TestAllExtensions.Builder does in fact extend
  // GeneratedMessageV3.ExtendableBuilder. The tests below assume that it does.
  static {
    @SuppressWarnings("unused")
    Class<? extends GeneratedMessageV3.ExtendableBuilder<?, ?>> ignored =
        TestAllExtensions.Builder.class;
  }

  @Test
  public void
      extendableBuilder_extensionFieldContainingBuilder_setRepeatedFieldOverwritesElement() {
    TestAllExtensions.Builder builder = TestAllExtensions.newBuilder();
    builder.addRepeatedField(REPEATED_NESTED_MESSAGE_EXTENSION, NestedMessage.getDefaultInstance());
    // Calling getRepeatedFieldBuilder and ignoring the returned Builder should have no
    // externally-visible effect, but internally it sets the stored field element to a builder.
    builder.getRepeatedFieldBuilder(REPEATED_NESTED_MESSAGE_EXTENSION, 0);

    NestedMessage setNestedMessage = NestedMessage.newBuilder().setBb(100).build();
    builder.setRepeatedField(REPEATED_NESTED_MESSAGE_EXTENSION, 0, setNestedMessage);

    assertThat(builder.getRepeatedField(REPEATED_NESTED_MESSAGE_EXTENSION, 0))
        .isEqualTo(setNestedMessage);
  }

  @Test
  public void extendableBuilder_extensionFieldContainingBuilder_addRepeatedFieldAppendsToField() {
    TestAllExtensions.Builder builder = TestAllExtensions.newBuilder();
    builder.addRepeatedField(REPEATED_NESTED_MESSAGE_EXTENSION, NestedMessage.getDefaultInstance());
    // Calling getRepeatedFieldBuilder and ignoring the returned Builder should have no
    // externally-visible effect, but internally it sets the stored field element to a builder.
    builder.getRepeatedFieldBuilder(REPEATED_NESTED_MESSAGE_EXTENSION, 0);

    builder.addRepeatedField(REPEATED_NESTED_MESSAGE_EXTENSION, NestedMessage.getDefaultInstance());

    assertThat((List<?>) builder.getField(REPEATED_NESTED_MESSAGE_EXTENSION)).hasSize(2);
  }

  @Test
  public void extendableBuilder_mergeFrom_optionalField_changesReflectedInExistingBuilder() {
    TestAllExtensions.Builder builder = TestAllExtensions.newBuilder();
    Message.Builder nestedMessageBuilder =
        builder.getFieldBuilder(OPTIONAL_NESTED_MESSAGE_EXTENSION);

    builder.mergeFrom(
        TestAllExtensions.newBuilder()
            .setField(OPTIONAL_NESTED_MESSAGE_EXTENSION, NestedMessage.newBuilder().setBb(100))
            .build());

    assertThat(nestedMessageBuilder.getField(NESTED_MESSAGE_BB_FIELD)).isEqualTo(100);
  }

  @Test
  public void extendableBuilder_mergeFrom_optionalField_doesNotInvalidateExistingBuilder() {
    TestAllExtensions.Builder builder = TestAllExtensions.newBuilder();
    Message.Builder nestedMessageBuilder =
        builder.getFieldBuilder(OPTIONAL_NESTED_MESSAGE_EXTENSION);

    builder.mergeFrom(
        TestAllExtensions.newBuilder()
            .setField(OPTIONAL_NESTED_MESSAGE_EXTENSION, NestedMessage.newBuilder().setBb(100))
            .build());

    // Changes to nestedMessageBuilder should still be reflected in the parent builder.
    nestedMessageBuilder.setField(NESTED_MESSAGE_BB_FIELD, 200);

    assertThat(builder.build())
        .isEqualTo(
            TestAllExtensions.newBuilder()
                .setField(OPTIONAL_NESTED_MESSAGE_EXTENSION, NestedMessage.newBuilder().setBb(200))
                .build());
  }

  @Test
  public void extendableBuilder_mergeFrom_repeatedField_doesNotInvalidateExistingBuilder() {
    TestAllExtensions.Builder builder = TestAllExtensions.newBuilder();
    builder.addRepeatedField(REPEATED_NESTED_MESSAGE_EXTENSION, NestedMessage.getDefaultInstance());
    Message.Builder nestedMessageBuilder =
        builder.getRepeatedFieldBuilder(REPEATED_NESTED_MESSAGE_EXTENSION, 0);

    builder.mergeFrom(
        TestAllExtensions.newBuilder()
            .addRepeatedField(REPEATED_NESTED_MESSAGE_EXTENSION, NestedMessage.getDefaultInstance())
            .build());

    // Changes to nestedMessageBuilder should still be reflected in the parent builder.
    nestedMessageBuilder.setField(NESTED_MESSAGE_BB_FIELD, 100);

    assertThat(builder.getRepeatedField(REPEATED_NESTED_MESSAGE_EXTENSION, 0))
        .isEqualTo(NestedMessage.newBuilder().setBb(100).build());
  }
}
