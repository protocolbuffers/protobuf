// Protocol Buffers - Google's data interchange format
// Copyright 2026 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

package com.google.protobuf;

import static com.google.common.truth.Truth.assertThat;
import static org.junit.Assert.assertThrows;

import proto2_unittest.UnittestProto;
import proto2_unittest.UnittestProto.TestAllExtensions;
import proto2_unittest.UnittestProto.TestAllTypes;
import proto2_unittest.UnittestProto.TestRequired;
import java.io.ByteArrayOutputStream;
import java.util.Arrays;
import java.util.List;
import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.Parameterized;
import org.junit.runners.Parameterized.Parameters;

@RunWith(Parameterized.class)
public final class InternalLazyFieldTest {

  private static final ExtensionRegistryLite EXTENSION_REGISTRY = TestUtil.getExtensionRegistry();

  private final ExtensionRegistryLite.LazyExtensionMode mode;
  private final ExtensionRegistryLite.LazyExtensionMode defaultMode;

  public InternalLazyFieldTest(ExtensionRegistryLite.LazyExtensionMode mode) {
    this.defaultMode = ExtensionRegistryLite.getLazyExtensionMode();
    this.mode = mode;
  }

  @Parameters
  public static List<Object[]> data() {
    return Arrays.asList(
        new Object[][] {
          {ExtensionRegistryLite.LazyExtensionMode.EAGER},
          {ExtensionRegistryLite.LazyExtensionMode.LAZY_VERIFY_ON_ACCESS}
        });
  }

  @Before
  public void setUp() {
    ExtensionRegistryLite.setLazyExtensionMode(mode);
  }

  @After
  public void tearDown() {
    ExtensionRegistryLite.setLazyExtensionMode(defaultMode);
  }

  @Test
  public void testGetValue() {
    MessageLite message = TestUtil.getAllSet();
    InternalLazyField lazyField = createLazyFieldWithBytesFromMessage(message);

    assertThat(lazyField.getValue()).isEqualTo(message);
  }

  @Test
  public void testGetValueContainingExtensions() {
    TestAllExtensions message = TestUtil.getAllExtensionsSet();
    InternalLazyField lazyField = createLazyFieldWithBytesFromMessage(message);

    assertThat(lazyField.getValue()).isEqualTo(message);
  }

  @Test
  public void testGetValueMemoized() {
    MessageLite message = TestUtil.getAllSet();
    InternalLazyField lazyField = createLazyFieldWithBytesFromMessage(message);
    MessageLite lazyFieldMessage1 = lazyField.getValue();
    MessageLite lazyFieldMessage2 = lazyField.getValue();

    assertThat(lazyFieldMessage1).isEqualTo(message);
    assertThat(lazyFieldMessage1).isSameInstanceAs(lazyFieldMessage2);
  }

  @Test
  public void testGetValueOnInvalidData() {
    InternalLazyField lazyField =
        new InternalLazyField(
            TestAllTypes.getDefaultInstance(),
            EXTENSION_REGISTRY,
            ByteString.copyFromUtf8("invalid"));

    if (mode == ExtensionRegistryLite.LazyExtensionMode.LAZY_VERIFY_ON_ACCESS) {
      assertThrows(InvalidProtobufRuntimeException.class, () -> lazyField.getValue());
    } else {
      assertThat(lazyField.getValue()).isEqualTo(TestAllTypes.getDefaultInstance());
    }
  }

  @Test
  public void testGetValueDataWtihInvalidExtension() {
    TestAllExtensions message =
        TestAllExtensions.newBuilder()
            .setExtension(TestRequired.single, TestRequired.getDefaultInstance())
            .buildPartial();
    InternalLazyField lazyField = createLazyFieldWithBytesFromMessage(message);

    if (mode == ExtensionRegistryLite.LazyExtensionMode.LAZY_VERIFY_ON_ACCESS) {
      TestAllExtensions parent = (TestAllExtensions) lazyField.getValue();
      assertThrows(
          InvalidProtobufRuntimeException.class, () -> parent.getExtension(TestRequired.single));
    } else {
      assertThat(lazyField.getValue()).isEqualTo(TestAllExtensions.getDefaultInstance());
    }
  }

  @Test
  public void testGetValueOnInvalidDataTwice() {
    InternalLazyField lazyField =
        new InternalLazyField(
            TestAllTypes.getDefaultInstance(),
            EXTENSION_REGISTRY,
            ByteString.copyFromUtf8("invalid"));

    if (mode == ExtensionRegistryLite.LazyExtensionMode.LAZY_VERIFY_ON_ACCESS) {
      assertThrows(InvalidProtobufRuntimeException.class, () -> lazyField.getValue());
      Throwable exception =
          assertThrows(InvalidProtobufRuntimeException.class, () -> lazyField.getValue());
      assertThat(exception).hasMessageThat().contains("Repeat access to corrupted lazy field");
    } else {
      MessageLite firstValue = lazyField.getValue();
      MessageLite secondValue = lazyField.getValue();
      assertThat(firstValue).isEqualTo(TestAllTypes.getDefaultInstance());
      assertThat(firstValue).isSameInstanceAs(secondValue);
    }
  }

  @Test
  public void testGetSerializedSize() {
    TestAllExtensions message = TestUtil.getAllExtensionsSet();
    InternalLazyField lazyField = createLazyFieldWithBytesFromMessage(message);

    assertThat(message.getSerializedSize()).isEqualTo(lazyField.getSerializedSize());
  }

  @Test
  public void testGetSerializedSizeContainingExtensions() {
    TestAllExtensions message = TestUtil.getAllExtensionsSet();
    InternalLazyField lazyField = createLazyFieldWithBytesFromMessage(message);

    assertThat(lazyField.getSerializedSize()).isEqualTo(message.getSerializedSize());
  }

  @Test
  public void testGetByteString() {
    MessageLite message = TestUtil.getAllSet();
    InternalLazyField lazyField = createLazyFieldWithBytesFromMessage(message);

    assertThat(lazyField.toByteString()).isEqualTo(message.toByteString());
  }

  @Test
  public void testGetByteStringContainingExtensions() {
    TestAllExtensions message = TestUtil.getAllExtensionsSet();
    InternalLazyField lazyField = createLazyFieldWithBytesFromMessage(message);

    assertThat(lazyField.toByteString()).isEqualTo(message.toByteString());
  }

  @Test
  public void testGetByteStringMemoized() {
    MessageLite message = TestUtil.getAllSet();
    InternalLazyField lazyField = createLazyFieldWithBytesFromMessage(message);
    ByteString lazyFieldByteString1 = lazyField.toByteString();
    ByteString lazyFieldByteString2 = lazyField.toByteString();

    assertThat(lazyFieldByteString1).isEqualTo(message.toByteString());
    assertThat(lazyFieldByteString1).isSameInstanceAs(lazyFieldByteString2);
  }

  @Test
  public void testHashCode() {
    MessageLite message = TestUtil.getAllSet();
    InternalLazyField lazyField = createLazyFieldWithBytesFromMessage(message);
    assertThat(message.hashCode()).isEqualTo(lazyField.hashCode());
    MessageLite unused = lazyField.getValue();
    assertThat(message.hashCode()).isEqualTo(lazyField.hashCode());
  }

  @Test
  public void testHashCodeContainingExtensions() throws Exception {
    TestAllExtensions message = TestUtil.getAllExtensionsSet();
    InternalLazyField lazyField = createLazyFieldWithBytesFromMessage(message);
    assertThat(message.hashCode()).isEqualTo(lazyField.hashCode());
    MessageLite unused = lazyField.getValue();
    assertThat(message.hashCode()).isEqualTo(lazyField.hashCode());
  }

  @Test
  public void testEqualsObject() {
    MessageLite message = TestUtil.getAllSet();
    InternalLazyField lazyField = createLazyFieldWithBytesFromMessage(message);
    assertThat(lazyField).isEqualTo(message);
  }

  @Test
  @SuppressWarnings("TruthIncompatibleType")
  public void testEqualsObjectContainingExtensions() throws Exception {
    TestAllExtensions message = TestUtil.getAllExtensionsSet();
    InternalLazyField lazyField = createLazyFieldWithBytesFromMessage(message);
    assertThat(lazyField).isEqualTo(message);
  }

  // Tests for mergeFrom(InternalLazyField lazyField, CodedInputStream input,
  // ExtensionRegistryLite extensionRegistry)

  @Test
  public void testEmptyLazyFieldMergeFromBytes() throws Exception {
    MessageLite message = TestUtil.getAllSet();
    InternalLazyField emptyLazyField =
        new InternalLazyField(
            TestAllTypes.getDefaultInstance(), EXTENSION_REGISTRY, ByteString.EMPTY);
    CodedInputStream input = createCodedInputStreamFromMessage(message);

    InternalLazyField mergedLazyField =
        InternalLazyField.mergeFrom(emptyLazyField, input, EXTENSION_REGISTRY);

    assertThat(mergedLazyField.getValue()).isEqualTo(message);
  }

  @Test
  public void testMergeFromEmptyBytes() throws Exception {
    MessageLite message = TestUtil.getAllSet();
    InternalLazyField lazyField = createLazyFieldWithBytesFromMessage(message);
    CodedInputStream input = createCodedInputStreamFromBytes(ByteString.EMPTY);

    InternalLazyField mergedLazyField =
        InternalLazyField.mergeFrom(lazyField, input, EXTENSION_REGISTRY);

    assertThat(mergedLazyField.getValue()).isEqualTo(message);
    assertThat(mergedLazyField).isSameInstanceAs(lazyField);
  }

  @Test
  public void testMergeFromBytesSameExtensionRegistry() throws Exception {
    InternalLazyField lazyField =
        createLazyFieldWithBytesFromMessage(TestAllTypes.newBuilder().setOptionalInt32(1).build());
    CodedInputStream input =
        createCodedInputStreamFromMessage(TestAllTypes.newBuilder().setOptionalInt64(2).build());

    InternalLazyField mergedLazyField =
        InternalLazyField.mergeFrom(lazyField, input, EXTENSION_REGISTRY);

    assertThat(mergedLazyField.getValue())
        .isEqualTo(TestAllTypes.newBuilder().setOptionalInt32(1).setOptionalInt64(2).build());
  }

  @Test
  public void testMergeExtensionsFromBytesSameExtensionRegistry() throws Exception {
    InternalLazyField lazyField =
        createLazyFieldWithBytesFromMessage(
            TestAllExtensions.newBuilder()
                .setExtension(UnittestProto.optionalInt32Extension, 1)
                .build());
    CodedInputStream input =
        createCodedInputStreamFromMessage(
            TestAllExtensions.newBuilder()
                .setExtension(UnittestProto.optionalInt64Extension, 2L)
                .build());

    InternalLazyField mergedLazyField =
        InternalLazyField.mergeFrom(lazyField, input, EXTENSION_REGISTRY);

    assertThat(mergedLazyField.getValue())
        .isEqualTo(
            TestAllExtensions.newBuilder()
                .setExtension(UnittestProto.optionalInt32Extension, 1)
                .setExtension(UnittestProto.optionalInt64Extension, 2L)
                .build());
  }

  @Test
  public void testMergeFromBytesDifferentExtensionRegistry() throws Exception {
    InternalLazyField lazyField =
        createLazyFieldWithBytesFromMessage(TestAllTypes.newBuilder().setOptionalInt32(1).build());
    CodedInputStream input =
        createCodedInputStreamFromMessage(TestAllTypes.newBuilder().setOptionalInt64(2).build());

    InternalLazyField mergedLazyField =
        InternalLazyField.mergeFrom(lazyField, input, TestUtil.getExtensionRegistry());

    assertThat(mergedLazyField.getValue())
        .isEqualTo(TestAllTypes.newBuilder().setOptionalInt32(1).setOptionalInt64(2).build());
  }

  @Test
  public void testMergeExtensionsFromBytesDifferentExtensionRegistry() throws Exception {
    ExtensionRegistryLite extensionRegistry1 = ExtensionRegistryLite.newInstance();
    extensionRegistry1.add(UnittestProto.optionalInt32Extension);
    InternalLazyField lazyField =
        createLazyFieldWithBytesFromMessage(
            TestAllExtensions.newBuilder()
                .setExtension(UnittestProto.optionalInt32Extension, 1)
                .build(),
            extensionRegistry1);
    ExtensionRegistryLite extensionRegistry2 = ExtensionRegistryLite.newInstance();
    extensionRegistry2.add(UnittestProto.optionalInt64Extension);
    CodedInputStream input =
        createCodedInputStreamFromMessage(
            TestAllExtensions.newBuilder()
                .setExtension(UnittestProto.optionalInt64Extension, 2L)
                .build());

    InternalLazyField mergedLazyField =
        InternalLazyField.mergeFrom(lazyField, input, extensionRegistry2);

    assertThat(mergedLazyField.getValue())
        .isEqualTo(
            TestAllExtensions.newBuilder()
                .setExtension(UnittestProto.optionalInt32Extension, 1)
                .setExtension(UnittestProto.optionalInt64Extension, 2L)
                .build());
  }

  @Test
  public void testInvalidFieldMergeFromBytesBecomesValid() throws Exception {
    // missing c
    InternalLazyField lazyField =
        createLazyFieldWithBytesFromMessage(
            TestRequired.newBuilder().setA(1).setB(2).buildPartial());
    // missing a
    CodedInputStream input =
        createCodedInputStreamFromMessage(
            TestRequired.newBuilder().setB(123).setC(3).buildPartial());

    InternalLazyField mergedLazyField =
        InternalLazyField.mergeFrom(lazyField, input, EXTENSION_REGISTRY);

    assertThat(mergedLazyField.getValue())
        .isEqualTo(TestRequired.newBuilder().setA(1).setB(123).setC(3).build());
  }

  @Test
  public void testInvalidFieldMergeFromBytesException() throws Exception {
    InternalLazyField lazyField =
        new InternalLazyField(
            TestAllTypes.getDefaultInstance(),
            EXTENSION_REGISTRY,
            ByteString.copyFromUtf8("invalid"));
    CodedInputStream input = createCodedInputStreamFromMessage(TestUtil.getAllSet());
    ExtensionRegistryLite extensionRegistry = TestUtil.getExtensionRegistry();

    Throwable exception =
        assertThrows(
            InvalidProtobufRuntimeException.class,
            () -> InternalLazyField.mergeFrom(lazyField, input, extensionRegistry));

    assertThat(exception).hasMessageThat().contains("Cannot merge invalid lazy field");
  }

  @Test
  public void testInvalidFieldMergeFromBytesKeepsInvalid() throws Exception {
    InternalLazyField lazyField =
        new InternalLazyField(
            TestAllTypes.getDefaultInstance(),
            EXTENSION_REGISTRY,
            ByteString.copyFromUtf8("invalidinvalid"));
    CodedInputStream input =
        createCodedInputStreamFromMessage(
            TestAllTypes.newBuilder().setOptionalString("and valid").build());

    InternalLazyField mergedLazyField =
        InternalLazyField.mergeFrom(lazyField, input, EXTENSION_REGISTRY);

    if (mode == ExtensionRegistryLite.LazyExtensionMode.LAZY_VERIFY_ON_ACCESS) {
      assertThrows(InvalidProtobufRuntimeException.class, () -> mergedLazyField.getValue());
    } else {
      assertThat(mergedLazyField.getValue()).isEqualTo(TestAllTypes.getDefaultInstance());
    }
  }

  @Test
  public void testMergeFromInvalidBytesException() throws Exception {
    InternalLazyField lazyField = createLazyFieldWithBytesFromMessage(TestUtil.getAllSet());
    CodedInputStream input = createCodedInputStreamFromBytes(ByteString.copyFromUtf8("invalid"));
    ExtensionRegistryLite extensionRegistry = TestUtil.getExtensionRegistry();

    Throwable exception =
        assertThrows(
            InvalidProtobufRuntimeException.class,
            () -> InternalLazyField.mergeFrom(lazyField, input, extensionRegistry));
    assertThat(exception).hasMessageThat().contains("Cannot merge lazy field from invalid bytes");
  }

  // Tests for mergeFrom(InternalLazyField self, InternalLazyField other)

  @Test
  public void testMergeFromDifferentDefaultInstances() throws Exception {
    InternalLazyField lazyField1 =
        new InternalLazyField(
            TestAllTypes.getDefaultInstance(), EXTENSION_REGISTRY, ByteString.EMPTY);
    InternalLazyField lazyField2 =
        new InternalLazyField(
            TestAllExtensions.getDefaultInstance(), EXTENSION_REGISTRY, ByteString.EMPTY);

    Throwable exception =
        assertThrows(
            IllegalArgumentException.class,
            () -> InternalLazyField.mergeFrom(lazyField1, lazyField2));
    assertThat(exception)
        .hasMessageThat()
        .contains("LazyFields with different default instances cannot be merged.");
  }

  @Test
  public void testEmptyFromOtherLazyField() {
    InternalLazyField lazyField1 =
        new InternalLazyField(
            TestAllTypes.getDefaultInstance(), EXTENSION_REGISTRY, ByteString.EMPTY);
    InternalLazyField lazyField2 = createLazyFieldWithBytesFromMessage(TestUtil.getAllSet());

    InternalLazyField mergedLazyField = InternalLazyField.mergeFrom(lazyField1, lazyField2);

    assertThat(mergedLazyField.getValue()).isEqualTo(TestUtil.getAllSet());
    assertThat(mergedLazyField).isSameInstanceAs(lazyField2);
  }

  @Test
  public void testMergeFromEmptyOtherLazyField() throws Exception {
    InternalLazyField lazyField1 = createLazyFieldWithBytesFromMessage(TestUtil.getAllSet());
    InternalLazyField lazyField2 =
        new InternalLazyField(
            TestAllTypes.getDefaultInstance(), EXTENSION_REGISTRY, ByteString.EMPTY);

    InternalLazyField mergedLazyField = InternalLazyField.mergeFrom(lazyField1, lazyField2);

    assertThat(mergedLazyField.getValue()).isEqualTo(TestUtil.getAllSet());
    assertThat(mergedLazyField).isSameInstanceAs(lazyField1);
  }

  @Test
  public void testMergeFromOtherBytesSameExtensionRegistryConcat() throws Exception {
    InternalLazyField lazyField1 =
        createLazyFieldWithBytesFromMessage(
            TestAllTypes.newBuilder().setOptionalInt32(123).build());
    InternalLazyField lazyField2 =
        createLazyFieldWithBytesFromMessage(
            TestAllTypes.newBuilder().setOptionalInt64(456).build());

    InternalLazyField mergedLazyField = InternalLazyField.mergeFrom(lazyField1, lazyField2);

    assertThat(mergedLazyField.getValue())
        .isEqualTo(TestAllTypes.newBuilder().setOptionalInt32(123).setOptionalInt64(456).build());
  }

  @Test
  public void testInvalidMergeFromOtherDifferentExtRegException() throws Exception {
    InternalLazyField lazyField1 =
        new InternalLazyField(
            TestAllTypes.getDefaultInstance(),
            EXTENSION_REGISTRY,
            ByteString.copyFromUtf8("invalid"));
    InternalLazyField lazyField2 =
        createLazyFieldWithBytesFromMessage(
            TestAllTypes.newBuilder().setOptionalInt64(456).build(),
            TestUtil.getExtensionRegistry());

    Throwable exception =
        assertThrows(
            InvalidProtobufRuntimeException.class,
            () -> InternalLazyField.mergeFrom(lazyField1, lazyField2));

    assertThat(exception).hasMessageThat().contains("Cannot merge invalid lazy field");
  }

  @Test
  public void testInvalidMergeFromOtherSameExtRegException() throws Exception {
    InternalLazyField lazyField1 =
        new InternalLazyField(
            TestAllTypes.getDefaultInstance(),
            ExtensionRegistryLite.getEmptyRegistry(),
            ByteString.copyFromUtf8("invalid"));
    // Construct lazyField2 with bytes==null.
    InternalLazyField lazyField2 =
        new InternalLazyField(TestAllTypes.newBuilder().setOptionalInt64(456).build());

    Throwable exception =
        assertThrows(
            InvalidProtobufRuntimeException.class,
            () -> InternalLazyField.mergeFrom(lazyField1, lazyField2));

    assertThat(exception).hasMessageThat().contains("Cannot merge invalid lazy field");
  }

  @Test
  public void testInvalidMergeFromOtherBecomesValid() throws Exception {
    // Construct lazyField1 with bytes==null.
    InternalLazyField lazyField1 =
        new InternalLazyField(TestRequired.newBuilder().setA(1).setB(2).buildPartial());
    InternalLazyField lazyField2 =
        createLazyFieldWithBytesFromMessage(
            TestRequired.newBuilder().setB(123).setC(3).buildPartial());

    InternalLazyField mergedLazyField = InternalLazyField.mergeFrom(lazyField1, lazyField2);

    assertThat(mergedLazyField.getValue())
        .isEqualTo(TestRequired.newBuilder().setA(1).setB(123).setC(3).build());
  }

  @Test
  public void testMergeFromOtherValue() {
    InternalLazyField lazyField1 =
        new InternalLazyField(TestAllTypes.newBuilder().setOptionalInt32(123).build());
    InternalLazyField lazyField2 =
        new InternalLazyField(TestAllTypes.newBuilder().setOptionalInt64(456).build());

    InternalLazyField mergedLazyField = InternalLazyField.mergeFrom(lazyField1, lazyField2);

    assertThat(mergedLazyField.getValue())
        .isEqualTo(TestAllTypes.newBuilder().setOptionalInt32(123).setOptionalInt64(456).build());
  }

  @Test
  public void testMergeFromOtherBytes() {
    InternalLazyField lazyField1 =
        new InternalLazyField(TestAllTypes.newBuilder().setOptionalInt32(123).build());
    InternalLazyField lazyField2 =
        createLazyFieldWithBytesFromMessage(
            TestAllTypes.newBuilder().setOptionalInt64(456).build());

    InternalLazyField mergedLazyField = InternalLazyField.mergeFrom(lazyField1, lazyField2);

    assertThat(mergedLazyField.getValue())
        .isEqualTo(TestAllTypes.newBuilder().setOptionalInt32(123).setOptionalInt64(456).build());
  }

  // Tests for combinations of getValue(), mergeFrom() and toByteString()

  @Test
  public void testGetValueAndMergeFromBytes() throws Exception {
    InternalLazyField lazyField =
        createLazyFieldWithBytesFromMessage(
            TestAllTypes.newBuilder().setOptionalInt32(123).build());
    MessageLite value = lazyField.getValue();

    assertThat(value).isEqualTo(TestAllTypes.newBuilder().setOptionalInt32(123).build());

    InternalLazyField mergedLazyField =
        InternalLazyField.mergeFrom(
            lazyField,
            createCodedInputStreamFromMessage(
                TestAllTypes.newBuilder().setOptionalInt64(456).build()),
            EXTENSION_REGISTRY);

    assertThat(mergedLazyField.getValue())
        .isEqualTo(TestAllTypes.newBuilder().setOptionalInt32(123).setOptionalInt64(456).build());
  }

  @Test
  public void testToByteStringAndMergeFromBytes() throws Exception {
    InternalLazyField lazyField =
        new InternalLazyField(TestAllTypes.newBuilder().setOptionalInt32(123).build());
    ByteString bytes = lazyField.toByteString();
    CodedInputStream input = createCodedInputStreamFromBytes(ByteString.copyFromUtf8("invalid"));

    // lazyField contains non-empty bytes, so merging from other bytes with the same extension
    // registry will concatenate the bytes.
    InternalLazyField mergedLazyField =
        InternalLazyField.mergeFrom(lazyField, input, ExtensionRegistryLite.getEmptyRegistry());

    assertThat(mergedLazyField.toByteString())
        .isEqualTo(bytes.concat(ByteString.copyFromUtf8("invalid")));
  }

  private InternalLazyField createLazyFieldWithBytesFromMessage(MessageLite message) {
    return createLazyFieldWithBytesFromMessage(message, EXTENSION_REGISTRY);
  }

  private InternalLazyField createLazyFieldWithBytesFromMessage(
      MessageLite message, ExtensionRegistryLite extensionRegistry) {
    return new InternalLazyField(
        message.getDefaultInstanceForType(), extensionRegistry, message.toByteString());
  }

  private CodedInputStream createCodedInputStreamFromMessage(MessageLite message) throws Exception {
    ByteArrayOutputStream output = new ByteArrayOutputStream();
    message.writeDelimitedTo(output);
    return CodedInputStream.newInstance(output.toByteArray());
  }

  private CodedInputStream createCodedInputStreamFromBytes(ByteString bytes) throws Exception {
    ByteArrayOutputStream baos = new ByteArrayOutputStream();
    CodedOutputStream output = CodedOutputStream.newInstance(baos);
    // Skip the tag.
    output.writeBytesNoTag(bytes);
    output.flush();
    return CodedInputStream.newInstance(baos.toByteArray());
  }
}
