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
import java.io.ObjectInputStream;
import java.io.ObjectOutputStream;
import java.util.Arrays;
import java.util.Collections;
import java.util.Iterator;
import java.util.List;
import junit.framework.TestCase;

/**
 * Unit test for generated messages and generated code. See also {@link MessageTest}, which tests
 * some generated message functionality.
 *
 * @author kenton@google.com Kenton Varda
 */
@SuppressWarnings({"ProtoBuilderReturnValueIgnored", "ReturnValueIgnored"})
public class GeneratedMessageTest extends TestCase {
  TestUtil.ReflectionTester reflectionTester =
      new TestUtil.ReflectionTester(TestAllTypes.getDescriptor(), null);

  @Override
  public void tearDown() {
    GeneratedMessageV3.setAlwaysUseFieldBuildersForTesting(false);
  }

  public void testGetFieldBuilderForExtensionField() {
    TestAllExtensions.Builder builder = TestAllExtensions.newBuilder();
    Message.Builder fieldBuilder =
        builder.getFieldBuilder(UnittestProto.optionalNestedMessageExtension.getDescriptor());
    int expected = 7432;
    FieldDescriptor field =
        NestedMessage.getDescriptor().findFieldByNumber(NestedMessage.BB_FIELD_NUMBER);
    fieldBuilder.setField(field, expected);
    assertEquals(
        expected,
        builder.build().getExtension(UnittestProto.optionalNestedMessageExtension).getBb());

    // fieldBuilder still updates the builder after builder build() has been called.
    expected += 100;
    fieldBuilder.setField(field, expected);
    assertEquals(
        expected,
        builder.build().getExtension(UnittestProto.optionalNestedMessageExtension).getBb());
  }

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
    assertEquals(
        expected,
        builder.build().getExtension(UnittestProto.optionalNestedMessageExtension).getBb());

    // fieldBuilder still updates the builder after builder build() has been called.
    expected += 100;
    fieldBuilder.setField(field, expected);
    assertEquals(
        expected,
        builder.build().getExtension(UnittestProto.optionalNestedMessageExtension).getBb());
  }

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
    assertEquals(
        expected,
        builder.build().getExtension(UnittestProto.optionalNestedMessageExtension).getBb());

    // Existing nestedMessageBuilder will also update builder.
    expected += 100;
    nestedMessageBuilder.setBb(expected);
    assertEquals(
        expected,
        builder.build().getExtension(UnittestProto.optionalNestedMessageExtension).getBb());

    // fieldBuilder still updates the builder.
    expected += 100;
    fieldBuilder.setField(field, expected);
    assertEquals(
        expected,
        builder.build().getExtension(UnittestProto.optionalNestedMessageExtension).getBb());
  }

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
    assertEquals(
        expected,
        builder.build().getExtension(UnittestProto.repeatedNestedMessageExtension, 0).getBb());

    // fieldBuilder still updates the builder after builder build() has been called.
    expected += 100;
    fieldBuilder.setField(field, expected);
    assertEquals(
        expected,
        builder.build().getExtension(UnittestProto.repeatedNestedMessageExtension, 0).getBb());
  }

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
    assertEquals(
        expected,
        builder.build().getExtension(UnittestProto.repeatedNestedMessageExtension, 0).getBb());

    // Existing nestedMessageBuilder will also update builder.
    expected += 100;
    nestedMessageBuilder.setBb(expected);
    assertEquals(
        expected,
        builder.build().getExtension(UnittestProto.repeatedNestedMessageExtension, 0).getBb());

    // fieldBuilder still updates the builder.
    expected += 100;
    fieldBuilder.setField(field, expected);
    assertEquals(
        expected,
        builder.build().getExtension(UnittestProto.repeatedNestedMessageExtension, 0).getBb());
  }

  public void testGetExtensionFieldOutOfBound() {
    TestAllExtensions.Builder builder = TestAllExtensions.newBuilder();
    try {
      builder.getRepeatedField(UnittestProto.repeatedNestedMessageExtension.getDescriptor(), 0);
      fail("Expected IndexOutOfBoundsException to be thrown");
    } catch (IndexOutOfBoundsException expected) {
    }
    try {
      builder.getExtension(UnittestProto.repeatedNestedMessageExtension, 0);
      fail("Expected IndexOutOfBoundsException to be thrown");
    } catch (IndexOutOfBoundsException expected) {
    }
    TestAllExtensions extensionsMessage = builder.build();
    try {
      extensionsMessage.getRepeatedField(
          UnittestProto.repeatedNestedMessageExtension.getDescriptor(), 0);
      fail("Expected IndexOutOfBoundsException to be thrown");
    } catch (IndexOutOfBoundsException expected) {
    }
    try {
      extensionsMessage.getExtension(UnittestProto.repeatedNestedMessageExtension, 0);
      fail("Expected IndexOutOfBoundsException to be thrown");
    } catch (IndexOutOfBoundsException expected) {
    }
  }

  public void testDefaultInstance() throws Exception {
    assertSame(
        TestAllTypes.getDefaultInstance(),
        TestAllTypes.getDefaultInstance().getDefaultInstanceForType());
    assertSame(
        TestAllTypes.getDefaultInstance(), TestAllTypes.newBuilder().getDefaultInstanceForType());
  }

  public void testMessageOrBuilder() throws Exception {
    TestAllTypes.Builder builder = TestAllTypes.newBuilder();
    TestUtil.setAllFields(builder);
    TestAllTypes message = builder.build();
    TestUtil.assertAllFieldsSet(message);
  }

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

    assertEquals(100, value1.getOptionalSfixed64());
    assertEquals(100, value1.getRepeatedInt32(0));
    assertEquals(UnittestImport.ImportEnum.IMPORT_BAR, value1.getOptionalImportEnum());
    assertEquals(UnittestImport.ImportEnum.IMPORT_BAR, value1.getRepeatedImportEnum(0));
    assertEquals(1, value1.getOptionalForeignMessage().getC());
    assertEquals(1, value1.getRepeatedForeignMessage(0).getC());

    // Make sure that builder didn't update previously created values
    builder.setOptionalSfixed64(200);
    builder.setRepeatedInt32(0, 200);
    builder.setOptionalImportEnum(UnittestImport.ImportEnum.IMPORT_FOO);
    builder.setRepeatedImportEnum(0, UnittestImport.ImportEnum.IMPORT_FOO);
    builder.setOptionalForeignMessage(ForeignMessage.newBuilder().setC(2));
    builder.setRepeatedForeignMessage(0, ForeignMessage.newBuilder().setC(2));

    TestAllTypes value2 = builder.build();

    // Make sure value1 didn't change.
    assertEquals(100, value1.getOptionalSfixed64());
    assertEquals(100, value1.getRepeatedInt32(0));
    assertEquals(UnittestImport.ImportEnum.IMPORT_BAR, value1.getOptionalImportEnum());
    assertEquals(UnittestImport.ImportEnum.IMPORT_BAR, value1.getRepeatedImportEnum(0));
    assertEquals(1, value1.getOptionalForeignMessage().getC());
    assertEquals(1, value1.getRepeatedForeignMessage(0).getC());

    // Make sure value2 is correct
    assertEquals(200, value2.getOptionalSfixed64());
    assertEquals(200, value2.getRepeatedInt32(0));
    assertEquals(UnittestImport.ImportEnum.IMPORT_FOO, value2.getOptionalImportEnum());
    assertEquals(UnittestImport.ImportEnum.IMPORT_FOO, value2.getRepeatedImportEnum(0));
    assertEquals(2, value2.getOptionalForeignMessage().getC());
    assertEquals(2, value2.getRepeatedForeignMessage(0).getC());
  }

  public void testProtosShareRepeatedArraysIfDidntChange() throws Exception {
    TestAllTypes.Builder builder = TestAllTypes.newBuilder();
    builder.addRepeatedInt32(100);
    builder.addRepeatedForeignMessage(ForeignMessage.getDefaultInstance());

    TestAllTypes value1 = builder.build();
    TestAllTypes value2 = value1.toBuilder().build();

    assertSame(value1.getRepeatedInt32List(), value2.getRepeatedInt32List());
    assertSame(value1.getRepeatedForeignMessageList(), value2.getRepeatedForeignMessageList());
  }

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

  public void testParsedMessagesAreImmutable() throws Exception {
    TestAllTypes value = TestAllTypes.parser().parseFrom(TestUtil.getAllSet().toByteString());
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
        fail("List wasn't immutable");
      } catch (UnsupportedOperationException e) {
        // good
      }
    }
  }

  public void testSettersRejectNull() throws Exception {
    TestAllTypes.Builder builder = TestAllTypes.newBuilder();
    try {
      builder.setOptionalString(null);
      fail("Exception was not thrown");
    } catch (NullPointerException e) {
      // We expect this exception.
    }
    try {
      builder.setOptionalBytes(null);
      fail("Exception was not thrown");
    } catch (NullPointerException e) {
      // We expect this exception.
    }
    try {
      builder.setOptionalNestedMessage((TestAllTypes.NestedMessage) null);
      fail("Exception was not thrown");
    } catch (NullPointerException e) {
      // We expect this exception.
    }
    try {
      builder.setOptionalNestedMessage((TestAllTypes.NestedMessage.Builder) null);
      fail("Exception was not thrown");
    } catch (NullPointerException e) {
      // We expect this exception.
    }
    try {
      builder.setOptionalNestedEnum(null);
      fail("Exception was not thrown");
    } catch (NullPointerException e) {
      // We expect this exception.
    }
    try {
      builder.addRepeatedString(null);
      fail("Exception was not thrown");
    } catch (NullPointerException e) {
      // We expect this exception.
    }
    try {
      builder.addRepeatedBytes(null);
      fail("Exception was not thrown");
    } catch (NullPointerException e) {
      // We expect this exception.
    }
    try {
      builder.addRepeatedNestedMessage((TestAllTypes.NestedMessage) null);
      fail("Exception was not thrown");
    } catch (NullPointerException e) {
      // We expect this exception.
    }
    try {
      builder.addRepeatedNestedMessage((TestAllTypes.NestedMessage.Builder) null);
      fail("Exception was not thrown");
    } catch (NullPointerException e) {
      // We expect this exception.
    }
    try {
      builder.addRepeatedNestedEnum(null);
      fail("Exception was not thrown");
    } catch (NullPointerException e) {
      // We expect this exception.
    }
  }

  public void testRepeatedSetters() throws Exception {
    TestAllTypes.Builder builder = TestAllTypes.newBuilder();
    TestUtil.setAllFields(builder);
    TestUtil.modifyRepeatedFields(builder);
    TestAllTypes message = builder.build();
    TestUtil.assertRepeatedFieldsModified(message);
  }

  public void testRepeatedSettersRejectNull() throws Exception {
    TestAllTypes.Builder builder = TestAllTypes.newBuilder();

    builder.addRepeatedString("one");
    builder.addRepeatedString("two");
    try {
      builder.setRepeatedString(1, null);
      fail("Exception was not thrown");
    } catch (NullPointerException e) {
      // We expect this exception.
    }

    builder.addRepeatedBytes(TestUtil.toBytes("one"));
    builder.addRepeatedBytes(TestUtil.toBytes("two"));
    try {
      builder.setRepeatedBytes(1, null);
      fail("Exception was not thrown");
    } catch (NullPointerException e) {
      // We expect this exception.
    }

    builder.addRepeatedNestedMessage(TestAllTypes.NestedMessage.newBuilder().setBb(218).build());
    builder.addRepeatedNestedMessage(TestAllTypes.NestedMessage.newBuilder().setBb(456).build());
    try {
      builder.setRepeatedNestedMessage(1, (TestAllTypes.NestedMessage) null);
      fail("Exception was not thrown");
    } catch (NullPointerException e) {
      // We expect this exception.
    }
    try {
      builder.setRepeatedNestedMessage(1, (TestAllTypes.NestedMessage.Builder) null);
      fail("Exception was not thrown");
    } catch (NullPointerException e) {
      // We expect this exception.
    }

    builder.addRepeatedNestedEnum(TestAllTypes.NestedEnum.FOO);
    builder.addRepeatedNestedEnum(TestAllTypes.NestedEnum.BAR);
    try {
      builder.setRepeatedNestedEnum(1, null);
      fail("Exception was not thrown");
    } catch (NullPointerException e) {
      // We expect this exception.
    }
  }

  public void testRepeatedAppend() throws Exception {
    TestAllTypes.Builder builder = TestAllTypes.newBuilder();

    builder.addAllRepeatedInt32(Arrays.asList(1, 2, 3, 4));
    builder.addAllRepeatedForeignEnum(Arrays.asList(ForeignEnum.FOREIGN_BAZ));

    ForeignMessage foreignMessage = ForeignMessage.newBuilder().setC(12).build();
    builder.addAllRepeatedForeignMessage(Arrays.asList(foreignMessage));

    TestAllTypes message = builder.build();
    assertEquals(message.getRepeatedInt32List(), Arrays.asList(1, 2, 3, 4));
    assertEquals(message.getRepeatedForeignEnumList(), Arrays.asList(ForeignEnum.FOREIGN_BAZ));
    assertEquals(1, message.getRepeatedForeignMessageCount());
    assertEquals(12, message.getRepeatedForeignMessage(0).getC());
  }

  public void testRepeatedAppendRejectsNull() throws Exception {
    TestAllTypes.Builder builder = TestAllTypes.newBuilder();

    ForeignMessage foreignMessage = ForeignMessage.newBuilder().setC(12).build();
    try {
      builder.addAllRepeatedForeignMessage(Arrays.asList(foreignMessage, (ForeignMessage) null));
      fail("Exception was not thrown");
    } catch (NullPointerException e) {
      // We expect this exception.
    }

    try {
      builder.addAllRepeatedForeignEnum(Arrays.asList(ForeignEnum.FOREIGN_BAZ, null));
      fail("Exception was not thrown");
    } catch (NullPointerException e) {
      // We expect this exception.
    }

    try {
      builder.addAllRepeatedString(Arrays.asList("one", null));
      fail("Exception was not thrown");
    } catch (NullPointerException e) {
      // We expect this exception.
    }

    try {
      builder.addAllRepeatedBytes(Arrays.asList(TestUtil.toBytes("one"), null));
      fail("Exception was not thrown");
    } catch (NullPointerException e) {
      // We expect this exception.
    }
  }

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
    assertEquals(3, builder.getRepeatedStringCount());
    assertEquals("one", builder.getRepeatedString(0));
    assertEquals("two", builder.getRepeatedString(1));
    assertEquals("three", builder.getRepeatedString(2));

    try {
      builder.addAllRepeatedString(stringIterable);
      fail("Exception was not thrown");
    } catch (IllegalStateException e) {
      // We expect this exception.
    }
  }

  public void testMergeFromOtherRejectsNull() throws Exception {
    try {
      TestAllTypes.Builder builder = TestAllTypes.newBuilder();
      builder.mergeFrom((TestAllTypes) null);
      fail("Exception was not thrown");
    } catch (NullPointerException e) {
      // We expect this exception.
    }
  }

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
    // TODO(ngd): Upgrade to using real #equals method once implemented
    assertEquals(expectedMessage.toString(), message.toString());
  }

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
    assertEquals(expectedMessage.toString(), message.toString());
  }

  public void testDefaults() throws Exception {
    TestUtil.assertClear(TestAllTypes.getDefaultInstance());
    TestUtil.assertClear(TestAllTypes.newBuilder().build());

    TestExtremeDefaultValues message = TestExtremeDefaultValues.getDefaultInstance();
    assertEquals("\u1234", message.getUtf8String());
    assertEquals(Double.POSITIVE_INFINITY, message.getInfDouble(), 0.0);
    assertEquals(Double.NEGATIVE_INFINITY, message.getNegInfDouble(), 0.0);
    assertTrue(Double.isNaN(message.getNanDouble()));
    assertEquals(Float.POSITIVE_INFINITY, message.getInfFloat(), 0.0f);
    assertEquals(Float.NEGATIVE_INFINITY, message.getNegInfFloat(), 0.0f);
    assertTrue(Float.isNaN(message.getNanFloat()));
    assertEquals("? ? ?? ?? ??? ??/ ??-", message.getCppTrigraph());
  }

  public void testClear() throws Exception {
    TestAllTypes.Builder builder = TestAllTypes.newBuilder();
    TestUtil.assertClear(builder);
    TestUtil.setAllFields(builder);
    builder.clear();
    TestUtil.assertClear(builder);
  }

  public void testReflectionGetters() throws Exception {
    TestAllTypes.Builder builder = TestAllTypes.newBuilder();
    TestUtil.setAllFields(builder);
    reflectionTester.assertAllFieldsSetViaReflection(builder);

    TestAllTypes message = builder.build();
    reflectionTester.assertAllFieldsSetViaReflection(message);
  }

  public void testReflectionSetters() throws Exception {
    TestAllTypes.Builder builder = TestAllTypes.newBuilder();
    reflectionTester.setAllFieldsViaReflection(builder);
    TestUtil.assertAllFieldsSet(builder);

    TestAllTypes message = builder.build();
    TestUtil.assertAllFieldsSet(message);
  }

  public void testReflectionSettersRejectNull() throws Exception {
    TestAllTypes.Builder builder = TestAllTypes.newBuilder();
    reflectionTester.assertReflectionSettersRejectNull(builder);
  }

  public void testReflectionRepeatedSetters() throws Exception {
    TestAllTypes.Builder builder = TestAllTypes.newBuilder();
    reflectionTester.setAllFieldsViaReflection(builder);
    reflectionTester.modifyRepeatedFieldsViaReflection(builder);
    TestUtil.assertRepeatedFieldsModified(builder);

    TestAllTypes message = builder.build();
    TestUtil.assertRepeatedFieldsModified(message);
  }

  public void testReflectionRepeatedSettersRejectNull() throws Exception {
    TestAllTypes.Builder builder = TestAllTypes.newBuilder();
    reflectionTester.assertReflectionRepeatedSettersRejectNull(builder);
  }

  public void testReflectionDefaults() throws Exception {
    reflectionTester.assertClearViaReflection(TestAllTypes.getDefaultInstance());
    reflectionTester.assertClearViaReflection(TestAllTypes.newBuilder().build());
  }

  public void testReflectionGetOneof() throws Exception {
    TestAllTypes.Builder builder = TestAllTypes.newBuilder();
    reflectionTester.setAllFieldsViaReflection(builder);
    Descriptors.OneofDescriptor oneof = TestAllTypes.getDescriptor().getOneofs().get(0);
    Descriptors.FieldDescriptor field = TestAllTypes.getDescriptor().findFieldByName("oneof_bytes");
    assertSame(field, builder.getOneofFieldDescriptor(oneof));

    TestAllTypes message = builder.build();
    assertSame(field, message.getOneofFieldDescriptor(oneof));
  }

  public void testReflectionClearOneof() throws Exception {
    TestAllTypes.Builder builder = TestAllTypes.newBuilder();
    reflectionTester.setAllFieldsViaReflection(builder);
    Descriptors.OneofDescriptor oneof = TestAllTypes.getDescriptor().getOneofs().get(0);
    Descriptors.FieldDescriptor field = TestAllTypes.getDescriptor().findFieldByName("oneof_bytes");

    assertTrue(builder.hasOneof(oneof));
    assertTrue(builder.hasField(field));
    builder.clearOneof(oneof);
    assertFalse(builder.hasOneof(oneof));
    assertFalse(builder.hasField(field));
  }

  public void testEnumInterface() throws Exception {
    assertTrue(
        TestAllTypes.getDefaultInstance().getDefaultNestedEnum() instanceof ProtocolMessageEnum);
  }

  public void testEnumMap() throws Exception {
    Internal.EnumLiteMap<ForeignEnum> map = ForeignEnum.internalGetValueMap();

    for (ForeignEnum value : ForeignEnum.values()) {
      assertEquals(value, map.findValueByNumber(value.getNumber()));
    }

    assertTrue(map.findValueByNumber(12345) == null);
  }

  public void testParsePackedToUnpacked() throws Exception {
    TestUnpackedTypes.Builder builder = TestUnpackedTypes.newBuilder();
    TestUnpackedTypes message = builder.mergeFrom(TestUtil.getPackedSet().toByteString()).build();
    TestUtil.assertUnpackedFieldsSet(message);
  }

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

  public void testExtensionMessageOrBuilder() throws Exception {
    TestAllExtensions.Builder builder = TestAllExtensions.newBuilder();
    TestUtil.setAllExtensions(builder);
    TestAllExtensions message = builder.build();
    TestUtil.assertAllExtensionsSet(message);
  }

  public void testGetBuilderForExtensionField() {
    TestAllExtensions.Builder builder = TestAllExtensions.newBuilder();
    Message.Builder fieldBuilder =
        builder.newBuilderForField(UnittestProto.optionalNestedMessageExtension.getDescriptor());
    final int expected = 7432;
    FieldDescriptor field =
        NestedMessage.getDescriptor().findFieldByNumber(NestedMessage.BB_FIELD_NUMBER);
    fieldBuilder.setField(field, expected);
    assertEquals(expected, fieldBuilder.build().getField(field));
  }


  public void testGetBuilderForNonMessageExtensionField() {
    TestAllExtensions.Builder builder = TestAllExtensions.newBuilder();
    try {
      // This should throw an exception because the extension field is not a message.
      builder.newBuilderForField(UnittestProto.optionalInt32Extension.getDescriptor());
      fail("Exception was not thrown");
    } catch (UnsupportedOperationException e) {
      // This exception is expected.
    }
  }

  public void testExtensionRepeatedSetters() throws Exception {
    TestAllExtensions.Builder builder = TestAllExtensions.newBuilder();
    TestUtil.setAllExtensions(builder);
    TestUtil.modifyRepeatedExtensions(builder);
    TestAllExtensions message = builder.build();
    TestUtil.assertRepeatedExtensionsModified(message);
  }

  public void testExtensionDefaults() throws Exception {
    TestUtil.assertExtensionsClear(TestAllExtensions.getDefaultInstance());
    TestUtil.assertExtensionsClear(TestAllExtensions.newBuilder().build());
  }

  public void testUnsetRepeatedExtensionGetField() {
    TestAllExtensions message = TestAllExtensions.getDefaultInstance();
    Object value;

    value = message.getField(UnittestProto.repeatedStringExtension.getDescriptor());
    assertTrue(value instanceof List);
    assertTrue(((List<?>) value).isEmpty());
    assertIsUnmodifiable((List<?>) value);

    value = message.getField(UnittestProto.repeatedNestedMessageExtension.getDescriptor());
    assertTrue(value instanceof List);
    assertTrue(((List<?>) value).isEmpty());
    assertIsUnmodifiable((List<?>) value);
  }

  public void testExtensionReflectionGetters() throws Exception {
    TestAllExtensions.Builder builder = TestAllExtensions.newBuilder();
    TestUtil.setAllExtensions(builder);
    extensionsReflectionTester.assertAllFieldsSetViaReflection(builder);

    TestAllExtensions message = builder.build();
    extensionsReflectionTester.assertAllFieldsSetViaReflection(message);
  }

  public void testExtensionReflectionSetters() throws Exception {
    TestAllExtensions.Builder builder = TestAllExtensions.newBuilder();
    extensionsReflectionTester.setAllFieldsViaReflection(builder);
    TestUtil.assertAllExtensionsSet(builder);

    TestAllExtensions message = builder.build();
    TestUtil.assertAllExtensionsSet(message);
  }

  public void testExtensionReflectionSettersRejectNull() throws Exception {
    TestAllExtensions.Builder builder = TestAllExtensions.newBuilder();
    extensionsReflectionTester.assertReflectionSettersRejectNull(builder);
  }

  public void testExtensionReflectionRepeatedSetters() throws Exception {
    TestAllExtensions.Builder builder = TestAllExtensions.newBuilder();
    extensionsReflectionTester.setAllFieldsViaReflection(builder);
    extensionsReflectionTester.modifyRepeatedFieldsViaReflection(builder);
    TestUtil.assertRepeatedExtensionsModified(builder);

    TestAllExtensions message = builder.build();
    TestUtil.assertRepeatedExtensionsModified(message);
  }

  public void testExtensionReflectionRepeatedSettersRejectNull() throws Exception {
    TestAllExtensions.Builder builder = TestAllExtensions.newBuilder();
    extensionsReflectionTester.assertReflectionRepeatedSettersRejectNull(builder);
  }

  public void testExtensionReflectionDefaults() throws Exception {
    extensionsReflectionTester.assertClearViaReflection(TestAllExtensions.getDefaultInstance());
    extensionsReflectionTester.assertClearViaReflection(TestAllExtensions.newBuilder().build());
  }

  public void testClearExtension() throws Exception {
    // clearExtension() is not actually used in TestUtil, so try it manually.
    assertFalse(
        TestAllExtensions.newBuilder()
            .setExtension(UnittestProto.optionalInt32Extension, 1)
            .clearExtension(UnittestProto.optionalInt32Extension)
            .hasExtension(UnittestProto.optionalInt32Extension));
    assertEquals(
        0,
        TestAllExtensions.newBuilder()
            .addExtension(UnittestProto.repeatedInt32Extension, 1)
            .clearExtension(UnittestProto.repeatedInt32Extension)
            .getExtensionCount(UnittestProto.repeatedInt32Extension));
  }

  public void testExtensionCopy() throws Exception {
    TestAllExtensions original = TestUtil.getAllExtensionsSet();
    TestAllExtensions copy = TestAllExtensions.newBuilder(original).build();
    TestUtil.assertAllExtensionsSet(copy);
  }

  public void testExtensionMergeFrom() throws Exception {
    TestAllExtensions original =
        TestAllExtensions.newBuilder()
            .setExtension(UnittestProto.optionalInt32Extension, 1)
            .build();
    TestAllExtensions merged = TestAllExtensions.newBuilder().mergeFrom(original).build();
    assertTrue(merged.hasExtension(UnittestProto.optionalInt32Extension));
    assertEquals(1, (int) merged.getExtension(UnittestProto.optionalInt32Extension));
  }

  // =================================================================
  // multiple_files_test

  // Test that custom options of an file level enum are properly initialized.
  // This test needs to be put before any other access to MultipleFilesTestProto
  // or messages defined in multiple_files_test.proto because the class loading
  // order affects initialization process of custom options.
  public void testEnumValueOptionsInMultipleFilesMode() throws Exception {
    assertEquals(
        12345,
        EnumWithNoOuter.FOO
            .getValueDescriptor()
            .getOptions()
            .getExtension(MultipleFilesTestProto.enumValueOption)
            .intValue());
  }

  public void testMultipleFilesOption() throws Exception {
    // We mostly just want to check that things compile.
    MessageWithNoOuter message =
        MessageWithNoOuter.newBuilder()
            .setNested(MessageWithNoOuter.NestedMessage.newBuilder().setI(1))
            .addForeign(TestAllTypes.newBuilder().setOptionalInt32(1))
            .setNestedEnum(MessageWithNoOuter.NestedEnum.BAZ)
            .setForeignEnum(EnumWithNoOuter.BAR)
            .build();
    assertEquals(message, MessageWithNoOuter.parseFrom(message.toByteString()));

    assertEquals(
        MultipleFilesTestProto.getDescriptor(), MessageWithNoOuter.getDescriptor().getFile());

    Descriptors.FieldDescriptor field =
        MessageWithNoOuter.getDescriptor().findFieldByName("foreign_enum");
    assertEquals(EnumWithNoOuter.BAR.getValueDescriptor(), message.getField(field));

    assertEquals(
        MultipleFilesTestProto.getDescriptor(), ServiceWithNoOuter.getDescriptor().getFile());

    assertFalse(
        TestAllExtensions.getDefaultInstance()
            .hasExtension(MultipleFilesTestProto.extensionWithOuter));
  }

  public void testOptionalFieldWithRequiredSubfieldsOptimizedForSize() throws Exception {
    TestOptionalOptimizedForSize message = TestOptionalOptimizedForSize.getDefaultInstance();
    assertTrue(message.isInitialized());

    message =
        TestOptionalOptimizedForSize.newBuilder()
            .setO(TestRequiredOptimizedForSize.newBuilder().buildPartial())
            .buildPartial();
    assertFalse(message.isInitialized());

    message =
        TestOptionalOptimizedForSize.newBuilder()
            .setO(TestRequiredOptimizedForSize.newBuilder().setX(5).buildPartial())
            .buildPartial();
    assertTrue(message.isInitialized());
  }

  public void testUninitializedExtensionInOptimizedForSize() throws Exception {
    TestOptimizedForSize.Builder builder = TestOptimizedForSize.newBuilder();
    builder.setExtension(
        TestOptimizedForSize.testExtension2,
        TestRequiredOptimizedForSize.newBuilder().buildPartial());
    assertFalse(builder.isInitialized());
    assertFalse(builder.buildPartial().isInitialized());

    builder = TestOptimizedForSize.newBuilder();
    builder.setExtension(
        TestOptimizedForSize.testExtension2,
        TestRequiredOptimizedForSize.newBuilder().setX(10).buildPartial());
    assertTrue(builder.isInitialized());
    assertTrue(builder.buildPartial().isInitialized());
  }

  public void testToBuilder() throws Exception {
    TestAllTypes.Builder builder = TestAllTypes.newBuilder();
    TestUtil.setAllFields(builder);
    TestAllTypes message = builder.build();
    TestUtil.assertAllFieldsSet(message);
    TestUtil.assertAllFieldsSet(message.toBuilder().build());
  }

  public void testFieldConstantValues() throws Exception {
    assertEquals(TestAllTypes.NestedMessage.BB_FIELD_NUMBER, 1);
    assertEquals(TestAllTypes.OPTIONAL_INT32_FIELD_NUMBER, 1);
    assertEquals(TestAllTypes.OPTIONALGROUP_FIELD_NUMBER, 16);
    assertEquals(TestAllTypes.OPTIONAL_NESTED_MESSAGE_FIELD_NUMBER, 18);
    assertEquals(TestAllTypes.OPTIONAL_NESTED_ENUM_FIELD_NUMBER, 21);
    assertEquals(TestAllTypes.REPEATED_INT32_FIELD_NUMBER, 31);
    assertEquals(TestAllTypes.REPEATEDGROUP_FIELD_NUMBER, 46);
    assertEquals(TestAllTypes.REPEATED_NESTED_MESSAGE_FIELD_NUMBER, 48);
    assertEquals(TestAllTypes.REPEATED_NESTED_ENUM_FIELD_NUMBER, 51);
  }

  public void testExtensionConstantValues() throws Exception {
    assertEquals(UnittestProto.TestRequired.SINGLE_FIELD_NUMBER, 1000);
    assertEquals(UnittestProto.TestRequired.MULTI_FIELD_NUMBER, 1001);
    assertEquals(UnittestProto.OPTIONAL_INT32_EXTENSION_FIELD_NUMBER, 1);
    assertEquals(UnittestProto.OPTIONALGROUP_EXTENSION_FIELD_NUMBER, 16);
    assertEquals(UnittestProto.OPTIONAL_NESTED_MESSAGE_EXTENSION_FIELD_NUMBER, 18);
    assertEquals(UnittestProto.OPTIONAL_NESTED_ENUM_EXTENSION_FIELD_NUMBER, 21);
    assertEquals(UnittestProto.REPEATED_INT32_EXTENSION_FIELD_NUMBER, 31);
    assertEquals(UnittestProto.REPEATEDGROUP_EXTENSION_FIELD_NUMBER, 46);
    assertEquals(UnittestProto.REPEATED_NESTED_MESSAGE_EXTENSION_FIELD_NUMBER, 48);
    assertEquals(UnittestProto.REPEATED_NESTED_ENUM_EXTENSION_FIELD_NUMBER, 51);
  }

  public void testRecursiveMessageDefaultInstance() throws Exception {
    UnittestProto.TestRecursiveMessage message =
        UnittestProto.TestRecursiveMessage.getDefaultInstance();
    assertTrue(message != null);
    assertNotNull(message.getA());
    assertTrue(message.getA().equals(message));
  }

  public void testSerialize() throws Exception {
    ByteArrayOutputStream baos = new ByteArrayOutputStream();
    TestAllTypes.Builder builder = TestAllTypes.newBuilder();
    TestUtil.setAllFields(builder);
    TestAllTypes expected = builder.build();
    ObjectOutputStream out = new ObjectOutputStream(baos);
    try {
      out.writeObject(expected);
    } finally {
      out.close();
    }
    ByteArrayInputStream bais = new ByteArrayInputStream(baos.toByteArray());
    ObjectInputStream in = new ObjectInputStream(bais);
    TestAllTypes actual = (TestAllTypes) in.readObject();
    assertEquals(expected, actual);
  }

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
    assertEquals(expected, actual);
  }

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
    assertEquals(123, int32Value.getValue());
  }

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
    assertEquals(123, int32Value.getValue());
  }

  public void testEnumValues() {
    assertEquals(TestAllTypes.NestedEnum.BAR_VALUE, TestAllTypes.NestedEnum.BAR.getNumber());
    assertEquals(TestAllTypes.NestedEnum.BAZ_VALUE, TestAllTypes.NestedEnum.BAZ.getNumber());
    assertEquals(TestAllTypes.NestedEnum.FOO_VALUE, TestAllTypes.NestedEnum.FOO.getNumber());
  }

  public void testNonNestedExtensionInitialization() {
    assertTrue(
        NonNestedExtension.nonNestedExtension.getMessageDefaultInstance()
            instanceof MyNonNestedExtension);
    assertEquals(
        "nonNestedExtension", NonNestedExtension.nonNestedExtension.getDescriptor().getName());
  }

  public void testNestedExtensionInitialization() {
    assertTrue(
        MyNestedExtension.recursiveExtension.getMessageDefaultInstance()
            instanceof MessageToBeExtended);
    assertEquals(
        "recursiveExtension", MyNestedExtension.recursiveExtension.getDescriptor().getName());
  }

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
    assertEquals(0, mockParent.getInvalidationCount());

    // Now tell it we want changes and make sure it's only fired once
    // And do this for each flavor

    // primitive single
    builder.buildPartial();
    builder.setOptionalInt32(2);
    builder.setOptionalInt32(3);
    assertEquals(1, mockParent.getInvalidationCount());

    // enum single
    builder.buildPartial();
    builder.setOptionalNestedEnum(TestAllTypes.NestedEnum.BAZ);
    builder.setOptionalNestedEnum(TestAllTypes.NestedEnum.BAR);
    assertEquals(2, mockParent.getInvalidationCount());

    // message single
    builder.buildPartial();
    builder.setOptionalNestedMessage(nestedMessage2);
    builder.setOptionalNestedMessage(nestedMessage1);
    assertEquals(3, mockParent.getInvalidationCount());

    // primitive repeated
    builder.buildPartial();
    builder.addRepeatedInt32(2);
    builder.addRepeatedInt32(3);
    assertEquals(4, mockParent.getInvalidationCount());

    // enum repeated
    builder.buildPartial();
    builder.addRepeatedNestedEnum(TestAllTypes.NestedEnum.BAZ);
    builder.addRepeatedNestedEnum(TestAllTypes.NestedEnum.BAZ);
    assertEquals(5, mockParent.getInvalidationCount());

    // message repeated
    builder.buildPartial();
    builder.addRepeatedNestedMessage(nestedMessage2);
    builder.addRepeatedNestedMessage(nestedMessage1);
    assertEquals(6, mockParent.getInvalidationCount());
  }

  public void testInvalidations_Extensions() throws Exception {
    TestUtil.MockBuilderParent mockParent = new TestUtil.MockBuilderParent();

    TestAllExtensions.Builder builder =
        (TestAllExtensions.Builder)
            ((AbstractMessage) TestAllExtensions.getDefaultInstance())
                .newBuilderForType(mockParent);

    builder.addExtension(UnittestProto.repeatedInt32Extension, 1);
    builder.setExtension(UnittestProto.repeatedInt32Extension, 0, 2);
    builder.clearExtension(UnittestProto.repeatedInt32Extension);
    assertEquals(0, mockParent.getInvalidationCount());

    // Now tell it we want changes and make sure it's only fired once
    builder.buildPartial();
    builder.addExtension(UnittestProto.repeatedInt32Extension, 2);
    builder.addExtension(UnittestProto.repeatedInt32Extension, 3);
    assertEquals(1, mockParent.getInvalidationCount());

    builder.buildPartial();
    builder.setExtension(UnittestProto.repeatedInt32Extension, 0, 4);
    builder.setExtension(UnittestProto.repeatedInt32Extension, 1, 5);
    assertEquals(2, mockParent.getInvalidationCount());

    builder.buildPartial();
    builder.clearExtension(UnittestProto.repeatedInt32Extension);
    builder.clearExtension(UnittestProto.repeatedInt32Extension);
    assertEquals(3, mockParent.getInvalidationCount());
  }

  public void testBaseMessageOrBuilder() {
    // Mostly just makes sure the base interface exists and has some methods.
    TestAllTypes.Builder builder = TestAllTypes.newBuilder();
    TestAllTypes message = builder.buildPartial();
    TestAllTypesOrBuilder messageAsInterface = (TestAllTypesOrBuilder) message;

    assertEquals(messageAsInterface.getDefaultBool(), messageAsInterface.getDefaultBool());
    assertEquals(
        messageAsInterface.getOptionalDouble(), messageAsInterface.getOptionalDouble(), 0.0);
  }

  public void testMessageOrBuilderGetters() {
    TestAllTypes.Builder builder = TestAllTypes.newBuilder();

    // single fields
    assertSame(ForeignMessage.getDefaultInstance(), builder.getOptionalForeignMessageOrBuilder());
    ForeignMessage.Builder subBuilder = builder.getOptionalForeignMessageBuilder();
    assertSame(subBuilder, builder.getOptionalForeignMessageOrBuilder());

    // repeated fields
    ForeignMessage m0 = ForeignMessage.newBuilder().buildPartial();
    ForeignMessage m1 = ForeignMessage.newBuilder().buildPartial();
    ForeignMessage m2 = ForeignMessage.newBuilder().buildPartial();
    builder.addRepeatedForeignMessage(m0);
    builder.addRepeatedForeignMessage(m1);
    builder.addRepeatedForeignMessage(m2);
    assertSame(m0, builder.getRepeatedForeignMessageOrBuilder(0));
    assertSame(m1, builder.getRepeatedForeignMessageOrBuilder(1));
    assertSame(m2, builder.getRepeatedForeignMessageOrBuilder(2));
    ForeignMessage.Builder b0 = builder.getRepeatedForeignMessageBuilder(0);
    ForeignMessage.Builder b1 = builder.getRepeatedForeignMessageBuilder(1);
    assertSame(b0, builder.getRepeatedForeignMessageOrBuilder(0));
    assertSame(b1, builder.getRepeatedForeignMessageOrBuilder(1));
    assertSame(m2, builder.getRepeatedForeignMessageOrBuilder(2));

    List<? extends ForeignMessageOrBuilder> messageOrBuilderList =
        builder.getRepeatedForeignMessageOrBuilderList();
    assertSame(b0, messageOrBuilderList.get(0));
    assertSame(b1, messageOrBuilderList.get(1));
    assertSame(m2, messageOrBuilderList.get(2));
  }

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
    FieldDescriptor subFieldDescriptor1 =
        fieldBuilder1.getDescriptorForType().findFieldByName("bb");
    fieldBuilder1.setField(subFieldDescriptor1, 1);
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
    FieldDescriptor subFieldDescriptor2 =
        fieldBuilder2.getDescriptorForType().findFieldByName("bb");
    fieldBuilder2.setField(subFieldDescriptor2, 1);
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
    assertEquals(newMessage1, newMessage2);
  }

  public void testGetFieldBuilderWithInitializedValue() {
    Descriptor descriptor = TestAllTypes.getDescriptor();
    FieldDescriptor fieldDescriptor = descriptor.findFieldByName("optional_nested_message");

    // Before setting field, builder is initialized by default value.
    TestAllTypes.Builder builder = TestAllTypes.newBuilder();
    NestedMessage.Builder fieldBuilder =
        (NestedMessage.Builder) builder.getFieldBuilder(fieldDescriptor);
    assertEquals(0, fieldBuilder.getBb());

    // Setting field value with new field builder instance.
    builder = TestAllTypes.newBuilder();
    NestedMessage.Builder newFieldBuilder = builder.getOptionalNestedMessageBuilder();
    newFieldBuilder.setBb(2);
    // Then get the field builder instance by getFieldBuilder().
    fieldBuilder = (NestedMessage.Builder) builder.getFieldBuilder(fieldDescriptor);
    // It should contain new value.
    assertEquals(2, fieldBuilder.getBb());
    // These two builder should be equal.
    assertSame(fieldBuilder, newFieldBuilder);
  }

  public void testGetFieldBuilderNotSupportedException() {
    Descriptor descriptor = TestAllTypes.getDescriptor();
    TestAllTypes.Builder builder = TestAllTypes.newBuilder();
    try {
      builder.getFieldBuilder(descriptor.findFieldByName("optional_int32"));
      fail("Exception was not thrown");
    } catch (UnsupportedOperationException e) {
      // We expect this exception.
    }
    try {
      builder.getFieldBuilder(descriptor.findFieldByName("optional_nested_enum"));
      fail("Exception was not thrown");
    } catch (UnsupportedOperationException e) {
      // We expect this exception.
    }
    try {
      builder.getFieldBuilder(descriptor.findFieldByName("repeated_int32"));
      fail("Exception was not thrown");
    } catch (UnsupportedOperationException e) {
      // We expect this exception.
    }
    try {
      builder.getFieldBuilder(descriptor.findFieldByName("repeated_nested_enum"));
      fail("Exception was not thrown");
    } catch (UnsupportedOperationException e) {
      // We expect this exception.
    }
    try {
      builder.getFieldBuilder(descriptor.findFieldByName("repeated_nested_message"));
      fail("Exception was not thrown");
    } catch (UnsupportedOperationException e) {
      // We expect this exception.
    }
  }

  // Test that when the default outer class name conflicts with another type
  // defined in the proto the compiler will append a suffix to avoid the
  // conflict.
  public void testConflictingOuterClassName() {
    // We just need to make sure we can refer to the outer class with the
    // expected name. There is nothing else to test.
    OuterClassNameTestOuterClass.OuterClassNameTest message =
        OuterClassNameTestOuterClass.OuterClassNameTest.newBuilder().build();
    assertTrue(
        message.getDescriptorForType()
            == OuterClassNameTestOuterClass.OuterClassNameTest.getDescriptor());

    OuterClassNameTest2OuterClass.TestMessage2.NestedMessage.OuterClassNameTest2 message2 =
        OuterClassNameTest2OuterClass.TestMessage2.NestedMessage.OuterClassNameTest2.newBuilder()
            .build();
    assertEquals(0, message2.getSerializedSize());

    OuterClassNameTest3OuterClass.TestMessage3.NestedMessage.OuterClassNameTest3 enumValue =
        OuterClassNameTest3OuterClass.TestMessage3.NestedMessage.OuterClassNameTest3.DUMMY_VALUE;
    assertEquals(1, enumValue.getNumber());
  }

  // =================================================================
  // oneof generated code test
  public void testOneofEnumCase() throws Exception {
    TestOneof2 message =
        TestOneof2.newBuilder().setFooInt(123).setFooString("foo").setFooCord("bar").build();
    TestUtil.assertAtMostOneFieldSetOneof(message);
  }

  public void testClearOneof() throws Exception {
    TestOneof2.Builder builder = TestOneof2.newBuilder().setFooInt(123);
    assertEquals(TestOneof2.FooCase.FOO_INT, builder.getFooCase());
    builder.clearFoo();
    assertEquals(TestOneof2.FooCase.FOO_NOT_SET, builder.getFooCase());
  }

  public void testSetOneofClearsOthers() throws Exception {
    TestOneof2.Builder builder = TestOneof2.newBuilder();
    TestOneof2 message = builder.setFooInt(123).setFooString("foo").buildPartial();
    assertTrue(message.hasFooString());
    TestUtil.assertAtMostOneFieldSetOneof(message);

    message = builder.setFooCord("bar").buildPartial();
    assertTrue(message.hasFooCord());
    TestUtil.assertAtMostOneFieldSetOneof(message);

    message = builder.setFooStringPiece("baz").buildPartial();
    assertTrue(message.hasFooStringPiece());
    TestUtil.assertAtMostOneFieldSetOneof(message);

    message = builder.setFooBytes(TestUtil.toBytes("qux")).buildPartial();
    assertTrue(message.hasFooBytes());
    TestUtil.assertAtMostOneFieldSetOneof(message);

    message = builder.setFooEnum(TestOneof2.NestedEnum.FOO).buildPartial();
    assertTrue(message.hasFooEnum());
    TestUtil.assertAtMostOneFieldSetOneof(message);

    message =
        builder
            .setFooMessage(TestOneof2.NestedMessage.newBuilder().setQuxInt(234).build())
            .buildPartial();
    assertTrue(message.hasFooMessage());
    TestUtil.assertAtMostOneFieldSetOneof(message);

    message = builder.setFooInt(123).buildPartial();
    assertTrue(message.hasFooInt());
    TestUtil.assertAtMostOneFieldSetOneof(message);
  }

  public void testOneofTypes() throws Exception {
    // Primitive
    {
      TestOneof2.Builder builder = TestOneof2.newBuilder();
      assertEquals(builder.getFooInt(), 0);
      assertFalse(builder.hasFooInt());
      assertTrue(builder.setFooInt(123).hasFooInt());
      assertEquals(builder.getFooInt(), 123);
      TestOneof2 message = builder.buildPartial();
      assertTrue(message.hasFooInt());
      assertEquals(message.getFooInt(), 123);

      assertFalse(builder.clearFooInt().hasFooInt());
      TestOneof2 message2 = builder.build();
      assertFalse(message2.hasFooInt());
      assertEquals(0, message2.getFooInt());
    }

    // Enum
    {
      TestOneof2.Builder builder = TestOneof2.newBuilder();
      assertEquals(TestOneof2.NestedEnum.FOO, builder.getFooEnum());
      assertTrue(builder.setFooEnum(TestOneof2.NestedEnum.BAR).hasFooEnum());
      assertEquals(TestOneof2.NestedEnum.BAR, builder.getFooEnum());
      TestOneof2 message = builder.buildPartial();
      assertTrue(message.hasFooEnum());
      assertEquals(TestOneof2.NestedEnum.BAR, message.getFooEnum());

      assertFalse(builder.clearFooEnum().hasFooEnum());
      TestOneof2 message2 = builder.build();
      assertFalse(message2.hasFooEnum());
      assertEquals(TestOneof2.NestedEnum.FOO, message2.getFooEnum());
    }

    // String
    {
      TestOneof2.Builder builder = TestOneof2.newBuilder();
      assertEquals("", builder.getFooString());
      builder.setFooString("foo");
      assertTrue(builder.hasFooString());
      assertEquals("foo", builder.getFooString());
      TestOneof2 message = builder.buildPartial();
      assertTrue(message.hasFooString());
      assertEquals("foo", message.getFooString());
      assertEquals(message.getFooStringBytes(), TestUtil.toBytes("foo"));

      assertFalse(builder.clearFooString().hasFooString());
      TestOneof2 message2 = builder.buildPartial();
      assertFalse(message2.hasFooString());
      assertEquals("", message2.getFooString());
      assertEquals(message2.getFooStringBytes(), TestUtil.toBytes(""));

      // Get method should not change the oneof value.
      builder.setFooInt(123);
      assertEquals("", builder.getFooString());
      assertEquals(builder.getFooStringBytes(), TestUtil.toBytes(""));
      assertEquals(123, builder.getFooInt());

      message = builder.build();
      assertEquals("", message.getFooString());
      assertEquals(message.getFooStringBytes(), TestUtil.toBytes(""));
      assertEquals(123, message.getFooInt());
    }

    // Cord
    {
      TestOneof2.Builder builder = TestOneof2.newBuilder();
      assertEquals("", builder.getFooCord());
      builder.setFooCord("foo");
      assertTrue(builder.hasFooCord());
      assertEquals("foo", builder.getFooCord());
      TestOneof2 message = builder.buildPartial();
      assertTrue(message.hasFooCord());
      assertEquals("foo", message.getFooCord());
      assertEquals(message.getFooCordBytes(), TestUtil.toBytes("foo"));

      assertFalse(builder.clearFooCord().hasFooCord());
      TestOneof2 message2 = builder.build();
      assertFalse(message2.hasFooCord());
      assertEquals("", message2.getFooCord());
      assertEquals(message2.getFooCordBytes(), TestUtil.toBytes(""));
    }

    // StringPiece
    {
      TestOneof2.Builder builder = TestOneof2.newBuilder();
      assertEquals("", builder.getFooStringPiece());
      builder.setFooStringPiece("foo");
      assertTrue(builder.hasFooStringPiece());
      assertEquals("foo", builder.getFooStringPiece());
      TestOneof2 message = builder.buildPartial();
      assertTrue(message.hasFooStringPiece());
      assertEquals("foo", message.getFooStringPiece());
      assertEquals(message.getFooStringPieceBytes(), TestUtil.toBytes("foo"));

      assertFalse(builder.clearFooStringPiece().hasFooStringPiece());
      TestOneof2 message2 = builder.build();
      assertFalse(message2.hasFooStringPiece());
      assertEquals("", message2.getFooStringPiece());
      assertEquals(message2.getFooStringPieceBytes(), TestUtil.toBytes(""));
    }

    // Message
    {
      // set
      TestOneof2.Builder builder = TestOneof2.newBuilder();
      assertEquals(0, builder.getFooMessage().getQuxInt());
      builder.setFooMessage(TestOneof2.NestedMessage.newBuilder().setQuxInt(234).build());
      assertTrue(builder.hasFooMessage());
      assertEquals(234, builder.getFooMessage().getQuxInt());
      TestOneof2 message = builder.buildPartial();
      assertTrue(message.hasFooMessage());
      assertEquals(234, message.getFooMessage().getQuxInt());

      // clear
      assertFalse(builder.clearFooMessage().hasFooString());
      message = builder.build();
      assertFalse(message.hasFooMessage());
      assertEquals(0, message.getFooMessage().getQuxInt());

      // nested builder
      builder = TestOneof2.newBuilder();
      assertSame(builder.getFooMessageOrBuilder(), TestOneof2.NestedMessage.getDefaultInstance());
      assertFalse(builder.hasFooMessage());
      builder.getFooMessageBuilder().setQuxInt(123);
      assertTrue(builder.hasFooMessage());
      assertEquals(123, builder.getFooMessage().getQuxInt());
      message = builder.build();
      assertTrue(message.hasFooMessage());
      assertEquals(123, message.getFooMessage().getQuxInt());
    }

    // LazyMessage is tested in LazyMessageLiteTest.java
  }

  public void testOneofMerge() throws Exception {
    // Primitive Type
    {
      TestOneof2.Builder builder = TestOneof2.newBuilder();
      TestOneof2 message = builder.setFooInt(123).build();
      TestOneof2 message2 = TestOneof2.newBuilder().mergeFrom(message).build();
      assertTrue(message2.hasFooInt());
      assertEquals(123, message2.getFooInt());
    }

    // String
    {
      TestOneof2.Builder builder = TestOneof2.newBuilder();
      TestOneof2 message = builder.setFooString("foo").build();
      TestOneof2 message2 = TestOneof2.newBuilder().mergeFrom(message).build();
      assertTrue(message2.hasFooString());
      assertEquals("foo", message2.getFooString());
    }

    // Enum
    {
      TestOneof2.Builder builder = TestOneof2.newBuilder();
      TestOneof2 message = builder.setFooEnum(TestOneof2.NestedEnum.BAR).build();
      TestOneof2 message2 = TestOneof2.newBuilder().mergeFrom(message).build();
      assertTrue(message2.hasFooEnum());
      assertEquals(TestOneof2.NestedEnum.BAR, message2.getFooEnum());
    }

    // Message
    {
      TestOneof2.Builder builder = TestOneof2.newBuilder();
      TestOneof2 message =
          builder
              .setFooMessage(TestOneof2.NestedMessage.newBuilder().setQuxInt(234).build())
              .build();
      TestOneof2 message2 = TestOneof2.newBuilder().mergeFrom(message).build();
      assertTrue(message2.hasFooMessage());
      assertEquals(234, message2.getFooMessage().getQuxInt());
    }
  }

  public void testOneofSerialization() throws Exception {
    // Primitive Type
    {
      TestOneof2.Builder builder = TestOneof2.newBuilder();
      TestOneof2 message = builder.setFooInt(123).build();
      ByteString serialized = message.toByteString();
      TestOneof2 message2 = TestOneof2.parseFrom(serialized);
      assertTrue(message2.hasFooInt());
      assertEquals(123, message2.getFooInt());
    }

    // String
    {
      TestOneof2.Builder builder = TestOneof2.newBuilder();
      TestOneof2 message = builder.setFooString("foo").build();
      ByteString serialized = message.toByteString();
      TestOneof2 message2 = TestOneof2.parseFrom(serialized);
      assertTrue(message2.hasFooString());
      assertEquals("foo", message2.getFooString());
    }

    // Enum
    {
      TestOneof2.Builder builder = TestOneof2.newBuilder();
      TestOneof2 message = builder.setFooEnum(TestOneof2.NestedEnum.BAR).build();
      ByteString serialized = message.toByteString();
      TestOneof2 message2 = TestOneof2.parseFrom(serialized);
      assertTrue(message2.hasFooEnum());
      assertEquals(TestOneof2.NestedEnum.BAR, message2.getFooEnum());
    }

    // Message
    {
      TestOneof2.Builder builder = TestOneof2.newBuilder();
      TestOneof2 message =
          builder
              .setFooMessage(TestOneof2.NestedMessage.newBuilder().setQuxInt(234).build())
              .build();
      ByteString serialized = message.toByteString();
      TestOneof2 message2 = TestOneof2.parseFrom(serialized);
      assertTrue(message2.hasFooMessage());
      assertEquals(234, message2.getFooMessage().getQuxInt());
    }
  }

  public void testOneofNestedBuilderOnChangePropagation() {
    NestedTestAllTypes.Builder parentBuilder = NestedTestAllTypes.newBuilder();
    TestAllTypes.Builder builder = parentBuilder.getPayloadBuilder();
    builder.getOneofNestedMessageBuilder();
    assertTrue(builder.hasOneofNestedMessage());
    assertTrue(parentBuilder.hasPayload());
    NestedTestAllTypes message = parentBuilder.build();
    assertTrue(message.hasPayload());
    assertTrue(message.getPayload().hasOneofNestedMessage());
  }

  public void testGetRepeatedFieldBuilder() {
    Descriptor descriptor = TestAllTypes.getDescriptor();

    FieldDescriptor fieldDescriptor = descriptor.findFieldByName("repeated_nested_message");
    FieldDescriptor foreignFieldDescriptor = descriptor.findFieldByName("repeated_foreign_message");
    FieldDescriptor importFieldDescriptor = descriptor.findFieldByName("repeated_import_message");

    // Mutate the message with new field builder
    // Mutate nested message
    TestAllTypes.Builder builder1 = TestAllTypes.newBuilder();
    Message.Builder fieldBuilder1 = builder1.newBuilderForField(fieldDescriptor);
    FieldDescriptor subFieldDescriptor1 =
        fieldBuilder1.getDescriptorForType().findFieldByName("bb");
    fieldBuilder1.setField(subFieldDescriptor1, 1);
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
    FieldDescriptor subFieldDescriptor2 =
        fieldBuilder2.getDescriptorForType().findFieldByName("bb");
    fieldBuilder2.setField(subFieldDescriptor2, 1);

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
    assertEquals(newMessage1, newMessage2);
  }

  public void testGetRepeatedFieldBuilderWithInitializedValue() {
    Descriptor descriptor = TestAllTypes.getDescriptor();
    FieldDescriptor fieldDescriptor = descriptor.findFieldByName("repeated_nested_message");

    // Before setting field, builder is initialized by default value.
    TestAllTypes.Builder builder = TestAllTypes.newBuilder();
    builder.addRepeatedNestedMessageBuilder();
    NestedMessage.Builder fieldBuilder =
        (NestedMessage.Builder) builder.getRepeatedFieldBuilder(fieldDescriptor, 0);
    assertEquals(0, fieldBuilder.getBb());

    // Setting field value with new field builder instance.
    builder = TestAllTypes.newBuilder();
    NestedMessage.Builder newFieldBuilder = builder.addRepeatedNestedMessageBuilder();
    newFieldBuilder.setBb(2);
    // Then get the field builder instance by getRepeatedFieldBuilder().
    fieldBuilder = (NestedMessage.Builder) builder.getRepeatedFieldBuilder(fieldDescriptor, 0);
    // It should contain new value.
    assertEquals(2, fieldBuilder.getBb());
    // These two builder should be equal.
    assertSame(fieldBuilder, newFieldBuilder);
  }

  public void testGetRepeatedFieldBuilderNotSupportedException() {
    Descriptor descriptor = TestAllTypes.getDescriptor();
    TestAllTypes.Builder builder = TestAllTypes.newBuilder();
    try {
      builder.getRepeatedFieldBuilder(descriptor.findFieldByName("repeated_int32"), 0);
      fail("Exception was not thrown");
    } catch (UnsupportedOperationException e) {
      // We expect this exception.
    }
    try {
      builder.getRepeatedFieldBuilder(descriptor.findFieldByName("repeated_nested_enum"), 0);
      fail("Exception was not thrown");
    } catch (UnsupportedOperationException e) {
      // We expect this exception.
    }
    try {
      builder.getRepeatedFieldBuilder(descriptor.findFieldByName("optional_int32"), 0);
      fail("Exception was not thrown");
    } catch (UnsupportedOperationException e) {
      // We expect this exception.
    }
    try {
      builder.getRepeatedFieldBuilder(descriptor.findFieldByName("optional_nested_enum"), 0);
      fail("Exception was not thrown");
    } catch (UnsupportedOperationException e) {
      // We expect this exception.
    }
    try {
      builder.getRepeatedFieldBuilder(descriptor.findFieldByName("optional_nested_message"), 0);
      fail("Exception was not thrown");
    } catch (UnsupportedOperationException e) {
      // We expect this exception.
    }
  }
}
