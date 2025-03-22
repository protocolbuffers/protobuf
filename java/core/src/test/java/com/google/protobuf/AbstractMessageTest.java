// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

package com.google.protobuf;

import static com.google.common.truth.Truth.assertThat;
import static com.google.common.truth.Truth.assertWithMessage;
import static com.google.protobuf.TestUtil.TEST_REQUIRED_INITIALIZED;
import static com.google.protobuf.TestUtil.TEST_REQUIRED_UNINITIALIZED;

import com.google.protobuf.Descriptors.FieldDescriptor;
import proto2_unittest.UnittestOptimizeFor.TestOptimizedForSize;
import proto2_unittest.UnittestProto;
import proto2_unittest.UnittestProto.ForeignMessage;
import proto2_unittest.UnittestProto.TestAllExtensions;
import proto2_unittest.UnittestProto.TestAllTypes;
import proto2_unittest.UnittestProto.TestPackedTypes;
import proto2_unittest.UnittestProto.TestRequired;
import proto2_unittest.UnittestProto.TestRequiredForeign;
import proto2_unittest.UnittestProto.TestUnpackedTypes;
import java.util.Map;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

/** Unit test for {@link AbstractMessage}. */
@RunWith(JUnit4.class)
public class AbstractMessageTest {
  /**
   * Extends AbstractMessage and wraps some other message object. The methods of the Message
   * interface which aren't explicitly implemented by AbstractMessage are forwarded to the wrapped
   * object. This allows us to test that AbstractMessage's implementations work even if the wrapped
   * object does not use them.
   */
  private static class AbstractMessageWrapper extends AbstractMessage {
    private final Message wrappedMessage;

    public AbstractMessageWrapper(Message wrappedMessage) {
      this.wrappedMessage = wrappedMessage;
    }

    @Override
    public Descriptors.Descriptor getDescriptorForType() {
      return wrappedMessage.getDescriptorForType();
    }

    @Override
    public AbstractMessageWrapper getDefaultInstanceForType() {
      return new AbstractMessageWrapper(wrappedMessage.getDefaultInstanceForType());
    }

    @Override
    public Map<Descriptors.FieldDescriptor, Object> getAllFields() {
      return wrappedMessage.getAllFields();
    }

    @Override
    public boolean hasField(Descriptors.FieldDescriptor field) {
      return wrappedMessage.hasField(field);
    }

    @Override
    public Object getField(Descriptors.FieldDescriptor field) {
      return wrappedMessage.getField(field);
    }

    @Override
    public int getRepeatedFieldCount(Descriptors.FieldDescriptor field) {
      return wrappedMessage.getRepeatedFieldCount(field);
    }

    @Override
    public Object getRepeatedField(Descriptors.FieldDescriptor field, int index) {
      return wrappedMessage.getRepeatedField(field, index);
    }

    @Override
    public UnknownFieldSet getUnknownFields() {
      return wrappedMessage.getUnknownFields();
    }

    @Override
    public Builder newBuilderForType() {
      return new Builder(wrappedMessage.newBuilderForType());
    }

    @Override
    public Builder toBuilder() {
      return new Builder(wrappedMessage.toBuilder());
    }

    static class Builder extends AbstractMessage.Builder<Builder> {
      private final Message.Builder wrappedBuilder;

      public Builder(Message.Builder wrappedBuilder) {
        this.wrappedBuilder = wrappedBuilder;
      }

      @Override
      public AbstractMessageWrapper build() {
        return new AbstractMessageWrapper(wrappedBuilder.build());
      }

      @Override
      public AbstractMessageWrapper buildPartial() {
        return new AbstractMessageWrapper(wrappedBuilder.buildPartial());
      }

      @Override
      public Builder clone() {
        return new Builder(wrappedBuilder.clone());
      }

      @Override
      public boolean isInitialized() {
        return clone().buildPartial().isInitialized();
      }

      @Override
      public Descriptors.Descriptor getDescriptorForType() {
        return wrappedBuilder.getDescriptorForType();
      }

      @Override
      public AbstractMessageWrapper getDefaultInstanceForType() {
        return new AbstractMessageWrapper(wrappedBuilder.getDefaultInstanceForType());
      }

      @Override
      public Map<Descriptors.FieldDescriptor, Object> getAllFields() {
        return wrappedBuilder.getAllFields();
      }

      @Override
      public Builder newBuilderForField(Descriptors.FieldDescriptor field) {
        return new Builder(wrappedBuilder.newBuilderForField(field));
      }

      @Override
      public boolean hasField(Descriptors.FieldDescriptor field) {
        return wrappedBuilder.hasField(field);
      }

      @Override
      public Object getField(Descriptors.FieldDescriptor field) {
        return wrappedBuilder.getField(field);
      }

      @Override
      public Builder setField(Descriptors.FieldDescriptor field, Object value) {
        wrappedBuilder.setField(field, value);
        return this;
      }

      @Override
      public Builder clearField(Descriptors.FieldDescriptor field) {
        wrappedBuilder.clearField(field);
        return this;
      }

      @Override
      public int getRepeatedFieldCount(Descriptors.FieldDescriptor field) {
        return wrappedBuilder.getRepeatedFieldCount(field);
      }

      @Override
      public Object getRepeatedField(Descriptors.FieldDescriptor field, int index) {
        return wrappedBuilder.getRepeatedField(field, index);
      }

      @Override
      public Builder setRepeatedField(Descriptors.FieldDescriptor field, int index, Object value) {
        wrappedBuilder.setRepeatedField(field, index, value);
        return this;
      }

      @Override
      public Builder addRepeatedField(Descriptors.FieldDescriptor field, Object value) {
        wrappedBuilder.addRepeatedField(field, value);
        return this;
      }

      @Override
      public UnknownFieldSet getUnknownFields() {
        return wrappedBuilder.getUnknownFields();
      }

      @Override
      public Builder setUnknownFields(UnknownFieldSet unknownFields) {
        wrappedBuilder.setUnknownFields(unknownFields);
        return this;
      }

      @Override
      public Message.Builder getFieldBuilder(FieldDescriptor field) {
        return wrappedBuilder.getFieldBuilder(field);
      }
    }

    @Override
    public Parser<? extends Message> getParserForType() {
      return wrappedMessage.getParserForType();
    }
  }

  // =================================================================

  TestUtil.ReflectionTester reflectionTester =
      new TestUtil.ReflectionTester(TestAllTypes.getDescriptor(), null);

  TestUtil.ReflectionTester extensionsReflectionTester =
      new TestUtil.ReflectionTester(
          TestAllExtensions.getDescriptor(), TestUtil.getFullExtensionRegistry());

  @Test
  public void testClear() throws Exception {
    AbstractMessageWrapper message =
        new AbstractMessageWrapper.Builder(TestAllTypes.newBuilder(TestUtil.getAllSet()))
            .clear()
            .build();
    TestUtil.assertClear((TestAllTypes) message.wrappedMessage);
  }

  @Test
  public void testCopy() throws Exception {
    AbstractMessageWrapper message =
        new AbstractMessageWrapper.Builder(TestAllTypes.newBuilder())
            .mergeFrom(TestUtil.getAllSet())
            .build();
    TestUtil.assertAllFieldsSet((TestAllTypes) message.wrappedMessage);
  }

  @Test
  public void testSerializedSize() throws Exception {
    TestAllTypes message = TestUtil.getAllSet();
    Message abstractMessage = new AbstractMessageWrapper(TestUtil.getAllSet());
    assertThat(message.getSerializedSize()).isEqualTo(abstractMessage.getSerializedSize());
  }

  @Test
  public void testSerialization() throws Exception {
    Message abstractMessage = new AbstractMessageWrapper(TestUtil.getAllSet());
    TestUtil.assertAllFieldsSet(
        TestAllTypes.parseFrom(
            abstractMessage.toByteString(), ExtensionRegistryLite.getEmptyRegistry()));
    assertThat(TestUtil.getAllSet().toByteString()).isEqualTo(abstractMessage.toByteString());
  }

  @Test
  public void testParsing() throws Exception {
    AbstractMessageWrapper.Builder builder =
        new AbstractMessageWrapper.Builder(TestAllTypes.newBuilder());
    AbstractMessageWrapper message =
        builder
            .mergeFrom(
                TestUtil.getAllSet().toByteString(), ExtensionRegistryLite.getEmptyRegistry())
            .build();
    TestUtil.assertAllFieldsSet((TestAllTypes) message.wrappedMessage);
  }

  @Test
  public void testParsingUninitialized() throws Exception {
    TestRequiredForeign.Builder builder = TestRequiredForeign.newBuilder();
    builder.getOptionalMessageBuilder().setDummy2(10);
    ByteString bytes = builder.buildPartial().toByteString();
    Message.Builder abstractMessageBuilder =
        new AbstractMessageWrapper.Builder(TestRequiredForeign.newBuilder());
    // mergeFrom() should not throw initialization error.
    Message unused1 =
        abstractMessageBuilder
            .mergeFrom(bytes, ExtensionRegistryLite.getEmptyRegistry())
            .buildPartial();
    try {
      abstractMessageBuilder.mergeFrom(bytes, ExtensionRegistryLite.getEmptyRegistry()).build();
      assertWithMessage("shouldn't pass").fail();
    } catch (UninitializedMessageException ex) {
      // pass
    }

    // test DynamicMessage directly.
    Message.Builder dynamicMessageBuilder =
        DynamicMessage.newBuilder(TestRequiredForeign.getDescriptor());
    // mergeFrom() should not throw initialization error.
    Message unused2 =
        dynamicMessageBuilder
            .mergeFrom(bytes, ExtensionRegistryLite.getEmptyRegistry())
            .buildPartial();
    try {
      dynamicMessageBuilder.mergeFrom(bytes, ExtensionRegistryLite.getEmptyRegistry()).build();
      assertWithMessage("shouldn't pass").fail();
    } catch (UninitializedMessageException ex) {
      // pass
    }
  }

  @Test
  public void testPackedSerialization() throws Exception {
    Message abstractMessage = new AbstractMessageWrapper(TestUtil.getPackedSet());
    TestUtil.assertPackedFieldsSet(
        TestPackedTypes.parseFrom(
            abstractMessage.toByteString(), ExtensionRegistryLite.getEmptyRegistry()));
    assertThat(TestUtil.getPackedSet().toByteString()).isEqualTo(abstractMessage.toByteString());
  }

  @Test
  public void testPackedParsing() throws Exception {
    AbstractMessageWrapper.Builder builder =
        new AbstractMessageWrapper.Builder(TestPackedTypes.newBuilder());
    AbstractMessageWrapper message =
        builder
            .mergeFrom(
                TestUtil.getPackedSet().toByteString(), ExtensionRegistryLite.getEmptyRegistry())
            .build();
    TestUtil.assertPackedFieldsSet((TestPackedTypes) message.wrappedMessage);
  }

  @Test
  public void testUnpackedSerialization() throws Exception {
    Message abstractMessage = new AbstractMessageWrapper(TestUtil.getUnpackedSet());
    TestUtil.assertUnpackedFieldsSet(
        TestUnpackedTypes.parseFrom(
            abstractMessage.toByteString(), ExtensionRegistryLite.getEmptyRegistry()));
    assertThat(TestUtil.getUnpackedSet().toByteString()).isEqualTo(abstractMessage.toByteString());
  }

  @Test
  public void testParsePackedToUnpacked() throws Exception {
    AbstractMessageWrapper.Builder builder =
        new AbstractMessageWrapper.Builder(TestUnpackedTypes.newBuilder());
    AbstractMessageWrapper message =
        builder
            .mergeFrom(
                TestUtil.getPackedSet().toByteString(), ExtensionRegistryLite.getEmptyRegistry())
            .build();
    TestUtil.assertUnpackedFieldsSet((TestUnpackedTypes) message.wrappedMessage);
  }

  @Test
  public void testParseUnpackedToPacked() throws Exception {
    AbstractMessageWrapper.Builder builder =
        new AbstractMessageWrapper.Builder(TestPackedTypes.newBuilder());
    AbstractMessageWrapper message =
        builder
            .mergeFrom(
                TestUtil.getUnpackedSet().toByteString(), ExtensionRegistryLite.getEmptyRegistry())
            .build();
    TestUtil.assertPackedFieldsSet((TestPackedTypes) message.wrappedMessage);
  }

  @Test
  public void testUnpackedParsing() throws Exception {
    AbstractMessageWrapper.Builder builder =
        new AbstractMessageWrapper.Builder(TestUnpackedTypes.newBuilder());
    AbstractMessageWrapper message =
        builder
            .mergeFrom(
                TestUtil.getUnpackedSet().toByteString(), ExtensionRegistryLite.getEmptyRegistry())
            .build();
    TestUtil.assertUnpackedFieldsSet((TestUnpackedTypes) message.wrappedMessage);
  }

  @Test
  public void testOptimizedForSize() throws Exception {
    // We're mostly only checking that this class was compiled successfully.
    TestOptimizedForSize message = TestOptimizedForSize.newBuilder().setI(1).build();
    message =
        TestOptimizedForSize.parseFrom(
            message.toByteString(), ExtensionRegistryLite.getEmptyRegistry());
    assertThat(message.getSerializedSize()).isEqualTo(2);
  }

  // -----------------------------------------------------------------
  // Tests for isInitialized().

  @Test
  public void testIsInitialized() throws Exception {
    TestRequired.Builder builder = TestRequired.newBuilder();
    AbstractMessageWrapper.Builder abstractBuilder = new AbstractMessageWrapper.Builder(builder);

    assertThat(abstractBuilder.isInitialized()).isFalse();
    assertThat(abstractBuilder.getInitializationErrorString()).isEqualTo("a, b, c");
    builder.setA(1);
    assertThat(abstractBuilder.isInitialized()).isFalse();
    assertThat(abstractBuilder.getInitializationErrorString()).isEqualTo("b, c");
    builder.setB(1);
    assertThat(abstractBuilder.isInitialized()).isFalse();
    assertThat(abstractBuilder.getInitializationErrorString()).isEqualTo("c");
    builder.setC(1);
    assertThat(abstractBuilder.isInitialized()).isTrue();
    assertThat(abstractBuilder.getInitializationErrorString()).isEmpty();
  }

  @Test
  public void testForeignIsInitialized() throws Exception {
    TestRequiredForeign.Builder builder = TestRequiredForeign.newBuilder();
    AbstractMessageWrapper.Builder abstractBuilder = new AbstractMessageWrapper.Builder(builder);

    assertThat(abstractBuilder.isInitialized()).isTrue();
    assertThat(abstractBuilder.getInitializationErrorString()).isEmpty();

    builder.setOptionalMessage(TEST_REQUIRED_UNINITIALIZED);
    assertThat(abstractBuilder.isInitialized()).isFalse();
    assertThat(abstractBuilder.getInitializationErrorString())
        .isEqualTo("optional_message.b, optional_message.c");

    builder.setOptionalMessage(TEST_REQUIRED_INITIALIZED);
    assertThat(abstractBuilder.isInitialized()).isTrue();
    assertThat(abstractBuilder.getInitializationErrorString()).isEmpty();

    builder.addRepeatedMessage(TEST_REQUIRED_UNINITIALIZED);
    assertThat(abstractBuilder.isInitialized()).isFalse();
    assertThat(abstractBuilder.getInitializationErrorString())
        .isEqualTo("repeated_message[0].b, repeated_message[0].c");

    builder.setRepeatedMessage(0, TEST_REQUIRED_INITIALIZED);
    assertThat(abstractBuilder.isInitialized()).isTrue();
    assertThat(abstractBuilder.getInitializationErrorString()).isEmpty();
  }

  // -----------------------------------------------------------------
  // Tests for mergeFrom

  static final TestAllTypes MERGE_SOURCE =
      TestAllTypes.newBuilder()
          .setOptionalInt32(1)
          .setOptionalString("foo")
          .setOptionalForeignMessage(ForeignMessage.getDefaultInstance())
          .addRepeatedString("bar")
          .build();

  static final TestAllTypes MERGE_DEST =
      TestAllTypes.newBuilder()
          .setOptionalInt64(2)
          .setOptionalString("baz")
          .setOptionalForeignMessage(ForeignMessage.newBuilder().setC(3).build())
          .addRepeatedString("qux")
          .build();

  static final String MERGE_RESULT_TEXT =
      ""
          + "optional_int32: 1\n"
          + "optional_int64: 2\n"
          + "optional_string: \"foo\"\n"
          + "optional_foreign_message {\n"
          + "  c: 3\n"
          + "}\n"
          + "repeated_string: \"qux\"\n"
          + "repeated_string: \"bar\"\n";

  @Test
  public void testMergeFrom() throws Exception {
    AbstractMessageWrapper result =
        new AbstractMessageWrapper.Builder(TestAllTypes.newBuilder(MERGE_DEST))
            .mergeFrom(MERGE_SOURCE)
            .build();

    assertThat(result.toString()).isEqualTo(MERGE_RESULT_TEXT);
  }

  // -----------------------------------------------------------------
  // Tests for equals and hashCode

  @Test
  public void testEqualsAndHashCode() throws Exception {
    TestAllTypes a = TestUtil.getAllSet();
    TestAllTypes b = TestAllTypes.getDefaultInstance();
    TestAllTypes c = TestAllTypes.newBuilder(b).addRepeatedString("x").build();
    TestAllTypes d = TestAllTypes.newBuilder(c).addRepeatedString("y").build();
    TestAllExtensions e = TestUtil.getAllExtensionsSet();
    TestAllExtensions f =
        TestAllExtensions.newBuilder(e)
            .addExtension(UnittestProto.repeatedInt32Extension, 999)
            .build();

    checkEqualsIsConsistent(a);
    checkEqualsIsConsistent(b);
    checkEqualsIsConsistent(c);
    checkEqualsIsConsistent(d);
    checkEqualsIsConsistent(e);
    checkEqualsIsConsistent(f);

    checkNotEqual(a, b);
    checkNotEqual(a, c);
    checkNotEqual(a, d);
    checkNotEqual(a, e);
    checkNotEqual(a, f);

    checkNotEqual(b, c);
    checkNotEqual(b, d);
    checkNotEqual(b, e);
    checkNotEqual(b, f);

    checkNotEqual(c, d);
    checkNotEqual(c, e);
    checkNotEqual(c, f);

    checkNotEqual(d, e);
    checkNotEqual(d, f);

    checkNotEqual(e, f);

    // Deserializing into the TestEmptyMessage such that every field
    // is an {@link UnknownFieldSet.Field}.
    UnittestProto.TestEmptyMessage eUnknownFields =
        UnittestProto.TestEmptyMessage.parseFrom(
            e.toByteArray(), ExtensionRegistryLite.getEmptyRegistry());
    UnittestProto.TestEmptyMessage fUnknownFields =
        UnittestProto.TestEmptyMessage.parseFrom(
            f.toByteArray(), ExtensionRegistryLite.getEmptyRegistry());
    checkNotEqual(eUnknownFields, fUnknownFields);
    checkEqualsIsConsistent(eUnknownFields);
    checkEqualsIsConsistent(fUnknownFields);

    // Subsequent reconstitutions should be identical
    UnittestProto.TestEmptyMessage eUnknownFields2 =
        UnittestProto.TestEmptyMessage.parseFrom(
            e.toByteArray(), ExtensionRegistryLite.getEmptyRegistry());
    checkEqualsIsConsistent(eUnknownFields, eUnknownFields2);
  }

  /** Asserts that the given proto has symmetric equals and hashCode methods. */
  private void checkEqualsIsConsistent(Message message) {
    // Test equals explicitly.
    assertThat(message.equals(message)).isTrue();

    // Object should be equal to a dynamic copy of itself.
    DynamicMessage dynamic = DynamicMessage.newBuilder(message).build();
    checkEqualsIsConsistent(message, dynamic);
  }

  /** Asserts that the given protos are equal and have the same hash code. */
  private void checkEqualsIsConsistent(Message message1, Message message2) {
    assertThat(message1).isEqualTo(message2);
    assertThat(message2).isEqualTo(message1);
    assertThat(message2.hashCode()).isEqualTo(message1.hashCode());
  }

  /**
   * Asserts that the given protos are not equal and have different hash codes.
   *
   * <p><b>Note:</b> It's valid for non-equal objects to have the same hash code, so this test is
   * stricter than it needs to be. However, this should happen relatively rarely.
   */
  private void checkNotEqual(Message m1, Message m2) {
    String equalsError = String.format("%s should not be equal to %s", m1, m2);
    assertWithMessage(equalsError).that(m1.equals(m2)).isFalse();
    assertWithMessage(equalsError).that(m2.equals(m1)).isFalse();

    assertWithMessage(String.format("%s should have a different hash code from %s", m1, m2))
        .that(m1.hashCode())
        .isNotEqualTo(m2.hashCode());
  }

  @Test
  public void testCheckByteStringIsUtf8OnUtf8() {
    ByteString byteString = ByteString.copyFromUtf8("some text");
    AbstractMessageLite.checkByteStringIsUtf8(byteString);
    // No exception thrown.
  }

  @Test
  public void testCheckByteStringIsUtf8OnNonUtf8() {
    ByteString byteString =
        ByteString.copyFrom(new byte[] {(byte) 0x80}); // A lone continuation byte.
    try {
      AbstractMessageLite.checkByteStringIsUtf8(byteString);
      assertWithMessage(
              "Expected AbstractMessageLite.checkByteStringIsUtf8 to throw"
                  + " IllegalArgumentException")
          .fail();
    } catch (IllegalArgumentException exception) {
      assertThat(exception).hasMessageThat().isEqualTo("Byte string is not UTF-8.");
    }
  }
}
