// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

package com.google.protobuf;

import static com.google.common.truth.Truth.assertThat;
import static protobuf_unittest.UnittestProto.optionalInt32Extension;

import protobuf_unittest.UnittestProto.TestAllExtensions;
import protobuf_unittest.UnittestProto.TestAllTypes;
import java.io.IOException;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

/** Unit test for {@link LazyFieldLite}. */
@RunWith(JUnit4.class)
public class LazyFieldLiteTest {

  @Test
  public void testGetValue() {
    MessageLite message = TestUtil.getAllSet();
    LazyFieldLite lazyField = createLazyFieldLiteFromMessage(message);
    assertThat(message).isEqualTo(lazyField.getValue(TestAllTypes.getDefaultInstance()));
    changeValue(lazyField);
    assertNotEqual(message, lazyField.getValue(TestAllTypes.getDefaultInstance()));
  }

  @Test
  public void testGetValueEx() throws Exception {
    TestAllExtensions message = TestUtil.getAllExtensionsSet();
    LazyFieldLite lazyField = createLazyFieldLiteFromMessage(message);
    assertThat(message).isEqualTo(lazyField.getValue(TestAllExtensions.getDefaultInstance()));
    changeValue(lazyField);
    assertNotEqual(message, lazyField.getValue(TestAllExtensions.getDefaultInstance()));
  }

  @Test
  public void testSetValue() {
    MessageLite message = TestUtil.getAllSet();
    LazyFieldLite lazyField = createLazyFieldLiteFromMessage(message);
    changeValue(lazyField);
    assertNotEqual(message, lazyField.getValue(TestAllTypes.getDefaultInstance()));
    message = lazyField.getValue(TestAllTypes.getDefaultInstance());
    changeValue(lazyField);
    assertThat(message).isEqualTo(lazyField.getValue(TestAllTypes.getDefaultInstance()));
  }

  @Test
  public void testSetValueEx() throws Exception {
    TestAllExtensions message = TestUtil.getAllExtensionsSet();
    LazyFieldLite lazyField = createLazyFieldLiteFromMessage(message);
    changeValue(lazyField);
    assertNotEqual(message, lazyField.getValue(TestAllExtensions.getDefaultInstance()));
    MessageLite value = lazyField.getValue(TestAllExtensions.getDefaultInstance());
    changeValue(lazyField);
    assertThat(value).isEqualTo(lazyField.getValue(TestAllExtensions.getDefaultInstance()));
  }

  @Test
  public void testGetSerializedSize() {
    MessageLite message = TestUtil.getAllSet();
    LazyFieldLite lazyField = createLazyFieldLiteFromMessage(message);
    assertThat(message.getSerializedSize()).isEqualTo(lazyField.getSerializedSize());
    changeValue(lazyField);
    assertNotEqual(message.getSerializedSize(), lazyField.getSerializedSize());
  }

  @Test
  public void testGetSerializedSizeEx() throws Exception {
    TestAllExtensions message = TestUtil.getAllExtensionsSet();
    LazyFieldLite lazyField = createLazyFieldLiteFromMessage(message);
    assertThat(message.getSerializedSize()).isEqualTo(lazyField.getSerializedSize());
    changeValue(lazyField);
    assertNotEqual(message.getSerializedSize(), lazyField.getSerializedSize());
  }

  @Test
  public void testGetByteString() {
    MessageLite message = TestUtil.getAllSet();
    LazyFieldLite lazyField = createLazyFieldLiteFromMessage(message);
    assertThat(message.toByteString()).isEqualTo(lazyField.toByteString());
    changeValue(lazyField);
    assertNotEqual(message.toByteString(), lazyField.toByteString());
  }

  @Test
  public void testGetByteStringEx() throws Exception {
    TestAllExtensions message = TestUtil.getAllExtensionsSet();
    LazyFieldLite lazyField = createLazyFieldLiteFromMessage(message);
    assertThat(message.toByteString()).isEqualTo(lazyField.toByteString());
    changeValue(lazyField);
    assertNotEqual(message.toByteString(), lazyField.toByteString());
  }

  @Test
  public void testMergeExtensions() throws Exception {
    TestAllExtensions message = TestUtil.getAllExtensionsSet();
    LazyFieldLite original = createLazyFieldLiteFromMessage(message);
    LazyFieldLite merged = new LazyFieldLite();
    merged.merge(original);
    TestAllExtensions value =
        (TestAllExtensions) merged.getValue(TestAllExtensions.getDefaultInstance());
    assertThat(message).isEqualTo(value);
  }

  @Test
  public void testEmptyLazyField() throws Exception {
    LazyFieldLite field = new LazyFieldLite();
    assertThat(field.getSerializedSize()).isEqualTo(0);
    assertThat(field.toByteString()).isEqualTo(ByteString.EMPTY);
  }

  @Test
  public void testInvalidProto() throws Exception {
    // Silently fails and uses the default instance.
    LazyFieldLite field =
        new LazyFieldLite(TestUtil.getExtensionRegistry(), ByteString.copyFromUtf8("invalid"));
    assertThat(
        field.getValue(TestAllTypes.getDefaultInstance()))
            .isEqualTo(TestAllTypes.getDefaultInstance());
    assertThat(field.getSerializedSize()).isEqualTo(0);
    assertThat(field.toByteString()).isEqualTo(ByteString.EMPTY);
  }

  @Test
  public void testMergeBeforeParsing() throws Exception {
    TestAllTypes message1 = TestAllTypes.newBuilder().setOptionalInt32(1).build();
    LazyFieldLite field1 = createLazyFieldLiteFromMessage(message1);
    TestAllTypes message2 = TestAllTypes.newBuilder().setOptionalInt64(2).build();
    LazyFieldLite field2 = createLazyFieldLiteFromMessage(message2);

    field1.merge(field2);
    TestAllTypes expected =
        TestAllTypes.newBuilder().setOptionalInt32(1).setOptionalInt64(2).build();
    assertThat(field1.getValue(TestAllTypes.getDefaultInstance())).isEqualTo(expected);
  }

  @Test
  public void testMergeOneNotParsed() throws Exception {
    // Test a few different paths that involve one message that was not parsed.
    TestAllTypes message1 = TestAllTypes.newBuilder().setOptionalInt32(1).build();
    TestAllTypes message2 = TestAllTypes.newBuilder().setOptionalInt64(2).build();
    TestAllTypes expected =
        TestAllTypes.newBuilder().setOptionalInt32(1).setOptionalInt64(2).build();

    LazyFieldLite field1 = LazyFieldLite.fromValue(message1);
    field1.getValue(TestAllTypes.getDefaultInstance()); // Force parsing.
    LazyFieldLite field2 = createLazyFieldLiteFromMessage(message2);
    field1.merge(field2);
    assertThat(field1.getValue(TestAllTypes.getDefaultInstance())).isEqualTo(expected);

    // Now reverse which one is parsed first.
    field1 = LazyFieldLite.fromValue(message1);
    field2 = createLazyFieldLiteFromMessage(message2);
    field2.getValue(TestAllTypes.getDefaultInstance()); // Force parsing.
    field1.merge(field2);
    assertThat(field1.getValue(TestAllTypes.getDefaultInstance())).isEqualTo(expected);
  }

  @Test
  public void testMergeInvalid() throws Exception {
    // Test a few different paths that involve one message that was not parsed.
    TestAllTypes message = TestAllTypes.newBuilder().setOptionalInt32(1).build();
    LazyFieldLite valid = LazyFieldLite.fromValue(message);
    LazyFieldLite invalid =
        new LazyFieldLite(TestUtil.getExtensionRegistry(), ByteString.copyFromUtf8("invalid"));
    invalid.merge(valid);

    // We swallow the exception and just use the set field.
    assertThat(invalid.getValue(TestAllTypes.getDefaultInstance())).isEqualTo(message);
  }

  @Test
  public void testMergeKeepsExtensionsWhenPossible() throws Exception {
    // In this test we attempt to only use the empty registry, which will strip out all extensions
    // when serializing and then parsing. We verify that each code path will attempt to not
    // serialize and parse a message that was set directly without going through the
    // extensionRegistry.
    TestAllExtensions messageWithExtensions =
        TestAllExtensions.newBuilder().setExtension(optionalInt32Extension, 42).build();
    TestAllExtensions emptyMessage = TestAllExtensions.getDefaultInstance();

    ExtensionRegistryLite emptyRegistry = ExtensionRegistryLite.getEmptyRegistry();

    LazyFieldLite field = LazyFieldLite.fromValue(messageWithExtensions);
    field.merge(createLazyFieldLiteFromMessage(emptyRegistry, emptyMessage));
    assertThat(field.getValue(TestAllExtensions.getDefaultInstance()))
        .isEqualTo(messageWithExtensions);

    // Now reverse the order of the merging.
    field = createLazyFieldLiteFromMessage(emptyRegistry, emptyMessage);
    field.merge(LazyFieldLite.fromValue(messageWithExtensions));
    assertThat(field.getValue(TestAllExtensions.getDefaultInstance()))
        .isEqualTo(messageWithExtensions);

    // Now try parsing the empty field first.
    field = LazyFieldLite.fromValue(messageWithExtensions);
    LazyFieldLite other = createLazyFieldLiteFromMessage(emptyRegistry, emptyMessage);
    other.getValue(TestAllExtensions.getDefaultInstance()); // Force parsing.
    field.merge(other);
    assertThat(field.getValue(TestAllExtensions.getDefaultInstance()))
        .isEqualTo(messageWithExtensions);

    // And again reverse.
    field = createLazyFieldLiteFromMessage(emptyRegistry, emptyMessage);
    field.getValue(TestAllExtensions.getDefaultInstance()); // Force parsing.
    other = LazyFieldLite.fromValue(messageWithExtensions);
    field.merge(other);
    assertThat(field.getValue(TestAllExtensions.getDefaultInstance()))
        .isEqualTo(messageWithExtensions);
  }

  // Help methods.

  private LazyFieldLite createLazyFieldLiteFromMessage(MessageLite message) {
    return createLazyFieldLiteFromMessage(TestUtil.getExtensionRegistry(), message);
  }

  private LazyFieldLite createLazyFieldLiteFromMessage(
      ExtensionRegistryLite extensionRegistry, MessageLite message) {
    ByteString bytes = message.toByteString();
    return new LazyFieldLite(extensionRegistry, bytes);
  }

  private void changeValue(LazyFieldLite lazyField) {
    TestAllTypes.Builder builder = TestUtil.getAllSet().toBuilder();
    builder.addRepeatedBool(true);
    MessageLite newMessage = builder.build();
    lazyField.setValue(newMessage);
  }

  private void assertNotEqual(Object unexpected, Object actual) {
    assertThat(unexpected).isNotSameInstanceAs(actual);
    assertThat((unexpected != null && unexpected.equals(actual))).isFalse();
  }
}
