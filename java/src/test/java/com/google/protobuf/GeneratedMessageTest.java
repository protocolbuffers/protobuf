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

import protobuf_unittest.UnittestOptimizeFor.TestOptimizedForSize;
import protobuf_unittest.UnittestOptimizeFor.TestOptionalOptimizedForSize;
import protobuf_unittest.UnittestOptimizeFor.TestRequiredOptimizedForSize;
import protobuf_unittest.UnittestProto;
import protobuf_unittest.UnittestProto.ForeignMessage;
import protobuf_unittest.UnittestProto.ForeignEnum;
import protobuf_unittest.UnittestProto.TestAllTypes;
import protobuf_unittest.UnittestProto.TestAllExtensions;
import protobuf_unittest.UnittestProto.TestExtremeDefaultValues;
import protobuf_unittest.MultipleFilesTestProto;
import protobuf_unittest.MessageWithNoOuter;
import protobuf_unittest.EnumWithNoOuter;
import protobuf_unittest.ServiceWithNoOuter;

import junit.framework.TestCase;
import java.util.Arrays;

/**
 * Unit test for generated messages and generated code.  See also
 * {@link MessageTest}, which tests some generated message functionality.
 *
 * @author kenton@google.com Kenton Varda
 */
public class GeneratedMessageTest extends TestCase {
  TestUtil.ReflectionTester reflectionTester =
    new TestUtil.ReflectionTester(TestAllTypes.getDescriptor(), null);

  public void testDefaultInstance() throws Exception {
    assertSame(TestAllTypes.getDefaultInstance(),
               TestAllTypes.getDefaultInstance().getDefaultInstanceForType());
    assertSame(TestAllTypes.getDefaultInstance(),
               TestAllTypes.newBuilder().getDefaultInstanceForType());
  }

  public void testAccessors() throws Exception {
    TestAllTypes.Builder builder = TestAllTypes.newBuilder();
    TestUtil.setAllFields(builder);
    TestAllTypes message = builder.build();
    TestUtil.assertAllFieldsSet(message);
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
      builder.setOptionalNestedMessage(
          (TestAllTypes.NestedMessage.Builder) null);
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
      builder.addRepeatedNestedMessage(
          (TestAllTypes.NestedMessage.Builder) null);
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

    builder.addRepeatedNestedMessage(
      TestAllTypes.NestedMessage.newBuilder().setBb(218).build());
    builder.addRepeatedNestedMessage(
      TestAllTypes.NestedMessage.newBuilder().setBb(456).build());
    try {
      builder.setRepeatedNestedMessage(1, (TestAllTypes.NestedMessage) null);
      fail("Exception was not thrown");
    } catch (NullPointerException e) {
      // We expect this exception.
    }
    try {
      builder.setRepeatedNestedMessage(
          1, (TestAllTypes.NestedMessage.Builder) null);
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

    ForeignMessage foreignMessage =
        ForeignMessage.newBuilder().setC(12).build();
    builder.addAllRepeatedForeignMessage(Arrays.asList(foreignMessage));

    TestAllTypes message = builder.build();
    assertEquals(message.getRepeatedInt32List(), Arrays.asList(1, 2, 3, 4));
    assertEquals(message.getRepeatedForeignEnumList(),
        Arrays.asList(ForeignEnum.FOREIGN_BAZ));
    assertEquals(1, message.getRepeatedForeignMessageCount());
    assertEquals(12, message.getRepeatedForeignMessage(0).getC());
  }

  public void testRepeatedAppendRejectsNull() throws Exception {
    TestAllTypes.Builder builder = TestAllTypes.newBuilder();

    ForeignMessage foreignMessage =
        ForeignMessage.newBuilder().setC(12).build();
    try {
      builder.addAllRepeatedForeignMessage(
          Arrays.asList(foreignMessage, (ForeignMessage) null));
      fail("Exception was not thrown");
    } catch (NullPointerException e) {
      // We expect this exception.
    }

    try {
      builder.addAllRepeatedForeignEnum(
          Arrays.asList(ForeignEnum.FOREIGN_BAZ, null));
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

  public void testSettingForeignMessageUsingBuilder() throws Exception {
    TestAllTypes message = TestAllTypes.newBuilder()
        // Pass builder for foreign message instance.
        .setOptionalForeignMessage(ForeignMessage.newBuilder().setC(123))
        .build();
    TestAllTypes expectedMessage = TestAllTypes.newBuilder()
        // Create expected version passing foreign message instance explicitly.
        .setOptionalForeignMessage(
            ForeignMessage.newBuilder().setC(123).build())
        .build();
    // TODO(ngd): Upgrade to using real #equals method once implemented
    assertEquals(expectedMessage.toString(), message.toString());
  }

  public void testSettingRepeatedForeignMessageUsingBuilder() throws Exception {
    TestAllTypes message = TestAllTypes.newBuilder()
        // Pass builder for foreign message instance.
        .addRepeatedForeignMessage(ForeignMessage.newBuilder().setC(456))
        .build();
    TestAllTypes expectedMessage = TestAllTypes.newBuilder()
        // Create expected version passing foreign message instance explicitly.
        .addRepeatedForeignMessage(
            ForeignMessage.newBuilder().setC(456).build())
        .build();
    assertEquals(expectedMessage.toString(), message.toString());
  }

  public void testDefaults() throws Exception {
    TestUtil.assertClear(TestAllTypes.getDefaultInstance());
    TestUtil.assertClear(TestAllTypes.newBuilder().build());

    assertEquals("\u1234",
                 TestExtremeDefaultValues.getDefaultInstance().getUtf8String());
  }

  public void testReflectionGetters() throws Exception {
    TestAllTypes.Builder builder = TestAllTypes.newBuilder();
    TestUtil.setAllFields(builder);
    TestAllTypes message = builder.build();
    reflectionTester.assertAllFieldsSetViaReflection(message);
  }

  public void testReflectionSetters() throws Exception {
    TestAllTypes.Builder builder = TestAllTypes.newBuilder();
    reflectionTester.setAllFieldsViaReflection(builder);
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
    TestAllTypes message = builder.build();
    TestUtil.assertRepeatedFieldsModified(message);
  }

  public void testReflectionRepeatedSettersRejectNull() throws Exception {
    TestAllTypes.Builder builder = TestAllTypes.newBuilder();
    reflectionTester.assertReflectionRepeatedSettersRejectNull(builder);
  }

  public void testReflectionDefaults() throws Exception {
    reflectionTester.assertClearViaReflection(
      TestAllTypes.getDefaultInstance());
    reflectionTester.assertClearViaReflection(
      TestAllTypes.newBuilder().build());
  }

  public void testEnumInterface() throws Exception {
    assertTrue(TestAllTypes.getDefaultInstance().getDefaultNestedEnum()
        instanceof ProtocolMessageEnum);
  }

  // =================================================================
  // Extensions.

  TestUtil.ReflectionTester extensionsReflectionTester =
    new TestUtil.ReflectionTester(TestAllExtensions.getDescriptor(),
                                  TestUtil.getExtensionRegistry());

  public void testExtensionAccessors() throws Exception {
    TestAllExtensions.Builder builder = TestAllExtensions.newBuilder();
    TestUtil.setAllExtensions(builder);
    TestAllExtensions message = builder.build();
    TestUtil.assertAllExtensionsSet(message);
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

  public void testExtensionReflectionGetters() throws Exception {
    TestAllExtensions.Builder builder = TestAllExtensions.newBuilder();
    TestUtil.setAllExtensions(builder);
    TestAllExtensions message = builder.build();
    extensionsReflectionTester.assertAllFieldsSetViaReflection(message);
  }

  public void testExtensionReflectionSetters() throws Exception {
    TestAllExtensions.Builder builder = TestAllExtensions.newBuilder();
    extensionsReflectionTester.setAllFieldsViaReflection(builder);
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
    TestAllExtensions message = builder.build();
    TestUtil.assertRepeatedExtensionsModified(message);
  }

  public void testExtensionReflectionRepeatedSettersRejectNull()
      throws Exception {
    TestAllExtensions.Builder builder = TestAllExtensions.newBuilder();
    extensionsReflectionTester.assertReflectionRepeatedSettersRejectNull(
        builder);
  }

  public void testExtensionReflectionDefaults() throws Exception {
    extensionsReflectionTester.assertClearViaReflection(
      TestAllExtensions.getDefaultInstance());
    extensionsReflectionTester.assertClearViaReflection(
      TestAllExtensions.newBuilder().build());
  }

  public void testClearExtension() throws Exception {
    // clearExtension() is not actually used in TestUtil, so try it manually.
    assertFalse(
      TestAllExtensions.newBuilder()
        .setExtension(UnittestProto.optionalInt32Extension, 1)
        .clearExtension(UnittestProto.optionalInt32Extension)
        .hasExtension(UnittestProto.optionalInt32Extension));
    assertEquals(0,
      TestAllExtensions.newBuilder()
        .addExtension(UnittestProto.repeatedInt32Extension, 1)
        .clearExtension(UnittestProto.repeatedInt32Extension)
        .getExtensionCount(UnittestProto.repeatedInt32Extension));
  }

  public void testExtensionMergeFrom() throws Exception {
    TestAllExtensions original =
      TestAllExtensions.newBuilder()
        .setExtension(UnittestProto.optionalInt32Extension, 1).build();
    TestAllExtensions merged =
        TestAllExtensions.newBuilder().mergeFrom(original).build();
    assertTrue(merged.hasExtension(UnittestProto.optionalInt32Extension));
    assertEquals(
        1, (int) merged.getExtension(UnittestProto.optionalInt32Extension));
  }

  // =================================================================
  // multiple_files_test

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

    assertEquals(MultipleFilesTestProto.getDescriptor(),
                 MessageWithNoOuter.getDescriptor().getFile());

    Descriptors.FieldDescriptor field =
      MessageWithNoOuter.getDescriptor().findFieldByName("foreign_enum");
    assertEquals(EnumWithNoOuter.BAR.getValueDescriptor(),
                 message.getField(field));

    assertEquals(MultipleFilesTestProto.getDescriptor(),
                 ServiceWithNoOuter.getDescriptor().getFile());

    assertFalse(
      TestAllExtensions.getDefaultInstance().hasExtension(
        MultipleFilesTestProto.extensionWithOuter));
  }

  public void testOptionalFieldWithRequiredSubfieldsOptimizedForSize()
    throws Exception {
    TestOptionalOptimizedForSize message =
        TestOptionalOptimizedForSize.getDefaultInstance();
    assertTrue(message.isInitialized());

    message = TestOptionalOptimizedForSize.newBuilder().setO(
        TestRequiredOptimizedForSize.newBuilder().buildPartial()
        ).buildPartial();
    assertFalse(message.isInitialized());

    message = TestOptionalOptimizedForSize.newBuilder().setO(
        TestRequiredOptimizedForSize.newBuilder().setX(5).buildPartial()
        ).buildPartial();
    assertTrue(message.isInitialized());
  }

  public void testUninitializedExtensionInOptimizedForSize()
      throws Exception {
    TestOptimizedForSize.Builder builder = TestOptimizedForSize.newBuilder();
    builder.setExtension(TestOptimizedForSize.testExtension2,
        TestRequiredOptimizedForSize.newBuilder().buildPartial());
    assertFalse(builder.isInitialized());
    assertFalse(builder.buildPartial().isInitialized());

    builder = TestOptimizedForSize.newBuilder();
    builder.setExtension(TestOptimizedForSize.testExtension2,
        TestRequiredOptimizedForSize.newBuilder().setX(10).buildPartial());
    assertTrue(builder.isInitialized());
    assertTrue(builder.buildPartial().isInitialized());
  }

  public void testToBuilder() throws Exception {
    TestAllTypes.Builder builder = TestAllTypes.newBuilder();
    TestUtil.setAllFields(builder);
    TestAllTypes message = builder.build();
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
    assertEquals(
      UnittestProto.OPTIONAL_NESTED_MESSAGE_EXTENSION_FIELD_NUMBER, 18);
    assertEquals(UnittestProto.OPTIONAL_NESTED_ENUM_EXTENSION_FIELD_NUMBER, 21);
    assertEquals(UnittestProto.REPEATED_INT32_EXTENSION_FIELD_NUMBER, 31);
    assertEquals(UnittestProto.REPEATEDGROUP_EXTENSION_FIELD_NUMBER, 46);
    assertEquals(
      UnittestProto.REPEATED_NESTED_MESSAGE_EXTENSION_FIELD_NUMBER, 48);
    assertEquals(UnittestProto.REPEATED_NESTED_ENUM_EXTENSION_FIELD_NUMBER, 51);
  }
}
