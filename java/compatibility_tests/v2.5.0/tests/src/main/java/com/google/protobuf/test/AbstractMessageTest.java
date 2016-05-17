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

package com.google.protobuf.test;
import com.google.protobuf.*;

import com.google.protobuf.Descriptors.FieldDescriptor;
import protobuf_unittest.UnittestOptimizeFor.TestOptimizedForSize;
import protobuf_unittest.UnittestProto;
import protobuf_unittest.UnittestProto.ForeignMessage;
import protobuf_unittest.UnittestProto.TestAllExtensions;
import protobuf_unittest.UnittestProto.TestAllTypes;
import protobuf_unittest.UnittestProto.TestPackedTypes;
import protobuf_unittest.UnittestProto.TestRequired;
import protobuf_unittest.UnittestProto.TestRequiredForeign;
import protobuf_unittest.UnittestProto.TestUnpackedTypes;

import junit.framework.TestCase;

import java.util.Map;

/**
 * Unit test for {@link AbstractMessage}.
 *
 * @author kenton@google.com Kenton Varda
 */
public class AbstractMessageTest extends TestCase {
  /**
   * Extends AbstractMessage and wraps some other message object.  The methods
   * of the Message interface which aren't explicitly implemented by
   * AbstractMessage are forwarded to the wrapped object.  This allows us to
   * test that AbstractMessage's implementations work even if the wrapped
   * object does not use them.
   */
  private static class AbstractMessageWrapper extends AbstractMessage {
    private final Message wrappedMessage;

    public AbstractMessageWrapper(Message wrappedMessage) {
      this.wrappedMessage = wrappedMessage;
    }

    public Descriptors.Descriptor getDescriptorForType() {
      return wrappedMessage.getDescriptorForType();
    }
    public AbstractMessageWrapper getDefaultInstanceForType() {
      return new AbstractMessageWrapper(
        wrappedMessage.getDefaultInstanceForType());
    }
    public Map<Descriptors.FieldDescriptor, Object> getAllFields() {
      return wrappedMessage.getAllFields();
    }
    public boolean hasField(Descriptors.FieldDescriptor field) {
      return wrappedMessage.hasField(field);
    }
    public Object getField(Descriptors.FieldDescriptor field) {
      return wrappedMessage.getField(field);
    }
    public int getRepeatedFieldCount(Descriptors.FieldDescriptor field) {
      return wrappedMessage.getRepeatedFieldCount(field);
    }
    public Object getRepeatedField(
        Descriptors.FieldDescriptor field, int index) {
      return wrappedMessage.getRepeatedField(field, index);
    }
    public UnknownFieldSet getUnknownFields() {
      return wrappedMessage.getUnknownFields();
    }
    public Builder newBuilderForType() {
      return new Builder(wrappedMessage.newBuilderForType());
    }
    public Builder toBuilder() {
      return new Builder(wrappedMessage.toBuilder());
    }

    static class Builder extends AbstractMessage.Builder<Builder> {
      private final Message.Builder wrappedBuilder;

      public Builder(Message.Builder wrappedBuilder) {
        this.wrappedBuilder = wrappedBuilder;
      }

      public AbstractMessageWrapper build() {
        return new AbstractMessageWrapper(wrappedBuilder.build());
      }
      public AbstractMessageWrapper buildPartial() {
        return new AbstractMessageWrapper(wrappedBuilder.buildPartial());
      }
      public Builder clone() {
        return new Builder(wrappedBuilder.clone());
      }
      public boolean isInitialized() {
        return clone().buildPartial().isInitialized();
      }
      public Descriptors.Descriptor getDescriptorForType() {
        return wrappedBuilder.getDescriptorForType();
      }
      public AbstractMessageWrapper getDefaultInstanceForType() {
        return new AbstractMessageWrapper(
          wrappedBuilder.getDefaultInstanceForType());
      }
      public Map<Descriptors.FieldDescriptor, Object> getAllFields() {
        return wrappedBuilder.getAllFields();
      }
      public Builder newBuilderForField(Descriptors.FieldDescriptor field) {
        return new Builder(wrappedBuilder.newBuilderForField(field));
      }
      public boolean hasField(Descriptors.FieldDescriptor field) {
        return wrappedBuilder.hasField(field);
      }
      public Object getField(Descriptors.FieldDescriptor field) {
        return wrappedBuilder.getField(field);
      }
      public Builder setField(Descriptors.FieldDescriptor field, Object value) {
        wrappedBuilder.setField(field, value);
        return this;
      }
      public Builder clearField(Descriptors.FieldDescriptor field) {
        wrappedBuilder.clearField(field);
        return this;
      }
      public int getRepeatedFieldCount(Descriptors.FieldDescriptor field) {
        return wrappedBuilder.getRepeatedFieldCount(field);
      }
      public Object getRepeatedField(
          Descriptors.FieldDescriptor field, int index) {
        return wrappedBuilder.getRepeatedField(field, index);
      }
      public Builder setRepeatedField(Descriptors.FieldDescriptor field,
                                      int index, Object value) {
        wrappedBuilder.setRepeatedField(field, index, value);
        return this;
      }
      public Builder addRepeatedField(
          Descriptors.FieldDescriptor field, Object value) {
        wrappedBuilder.addRepeatedField(field, value);
        return this;
      }
      public UnknownFieldSet getUnknownFields() {
        return wrappedBuilder.getUnknownFields();
      }
      public Builder setUnknownFields(UnknownFieldSet unknownFields) {
        wrappedBuilder.setUnknownFields(unknownFields);
        return this;
      }
      @Override
      public Message.Builder getFieldBuilder(FieldDescriptor field) {
        return wrappedBuilder.getFieldBuilder(field);
      }
    }
    public Parser<? extends Message> getParserForType() {
      return wrappedMessage.getParserForType();
    }
  }

  // =================================================================

  TestUtil.ReflectionTester reflectionTester =
    new TestUtil.ReflectionTester(TestAllTypes.getDescriptor(), null);

  TestUtil.ReflectionTester extensionsReflectionTester =
    new TestUtil.ReflectionTester(TestAllExtensions.getDescriptor(),
                                  TestUtil.getExtensionRegistry());

  public void testClear() throws Exception {
    AbstractMessageWrapper message =
      new AbstractMessageWrapper.Builder(
          TestAllTypes.newBuilder(TestUtil.getAllSet()))
        .clear().build();
    TestUtil.assertClear((TestAllTypes) message.wrappedMessage);
  }

  public void testCopy() throws Exception {
    AbstractMessageWrapper message =
      new AbstractMessageWrapper.Builder(TestAllTypes.newBuilder())
        .mergeFrom(TestUtil.getAllSet()).build();
    TestUtil.assertAllFieldsSet((TestAllTypes) message.wrappedMessage);
  }

  public void testSerializedSize() throws Exception {
    TestAllTypes message = TestUtil.getAllSet();
    Message abstractMessage = new AbstractMessageWrapper(TestUtil.getAllSet());

    assertEquals(message.getSerializedSize(),
                 abstractMessage.getSerializedSize());
  }

  public void testSerialization() throws Exception {
    Message abstractMessage = new AbstractMessageWrapper(TestUtil.getAllSet());

    TestUtil.assertAllFieldsSet(
      TestAllTypes.parseFrom(abstractMessage.toByteString()));

    assertEquals(TestUtil.getAllSet().toByteString(),
                 abstractMessage.toByteString());
  }

  public void testParsing() throws Exception {
    AbstractMessageWrapper.Builder builder =
      new AbstractMessageWrapper.Builder(TestAllTypes.newBuilder());
    AbstractMessageWrapper message =
      builder.mergeFrom(TestUtil.getAllSet().toByteString()).build();
    TestUtil.assertAllFieldsSet((TestAllTypes) message.wrappedMessage);
  }

  public void testParsingUninitialized() throws Exception {
    TestRequiredForeign.Builder builder = TestRequiredForeign.newBuilder();
    builder.getOptionalMessageBuilder().setDummy2(10);
    ByteString bytes = builder.buildPartial().toByteString();
    Message.Builder abstractMessageBuilder =
        new AbstractMessageWrapper.Builder(TestRequiredForeign.newBuilder());
    // mergeFrom() should not throw initialization error.
    abstractMessageBuilder.mergeFrom(bytes).buildPartial();
    try {
      abstractMessageBuilder.mergeFrom(bytes).build();
      fail();
    } catch (UninitializedMessageException ex) {
      // pass
    }

    // test DynamicMessage directly.
    Message.Builder dynamicMessageBuilder = DynamicMessage.newBuilder(
        TestRequiredForeign.getDescriptor());
    // mergeFrom() should not throw initialization error.
    dynamicMessageBuilder.mergeFrom(bytes).buildPartial();
    try {
      dynamicMessageBuilder.mergeFrom(bytes).build();
      fail();
    } catch (UninitializedMessageException ex) {
      // pass
    }
  }

  public void testPackedSerialization() throws Exception {
    Message abstractMessage =
        new AbstractMessageWrapper(TestUtil.getPackedSet());

    TestUtil.assertPackedFieldsSet(
      TestPackedTypes.parseFrom(abstractMessage.toByteString()));

    assertEquals(TestUtil.getPackedSet().toByteString(),
                 abstractMessage.toByteString());
  }

  public void testPackedParsing() throws Exception {
    AbstractMessageWrapper.Builder builder =
      new AbstractMessageWrapper.Builder(TestPackedTypes.newBuilder());
    AbstractMessageWrapper message =
      builder.mergeFrom(TestUtil.getPackedSet().toByteString()).build();
    TestUtil.assertPackedFieldsSet((TestPackedTypes) message.wrappedMessage);
  }

  public void testUnpackedSerialization() throws Exception {
    Message abstractMessage =
      new AbstractMessageWrapper(TestUtil.getUnpackedSet());

    TestUtil.assertUnpackedFieldsSet(
      TestUnpackedTypes.parseFrom(abstractMessage.toByteString()));

    assertEquals(TestUtil.getUnpackedSet().toByteString(),
                 abstractMessage.toByteString());
  }

  public void testParsePackedToUnpacked() throws Exception {
    AbstractMessageWrapper.Builder builder =
      new AbstractMessageWrapper.Builder(TestUnpackedTypes.newBuilder());
    AbstractMessageWrapper message =
      builder.mergeFrom(TestUtil.getPackedSet().toByteString()).build();
    TestUtil.assertUnpackedFieldsSet(
      (TestUnpackedTypes) message.wrappedMessage);
  }

  public void testParseUnpackedToPacked() throws Exception {
    AbstractMessageWrapper.Builder builder =
      new AbstractMessageWrapper.Builder(TestPackedTypes.newBuilder());
    AbstractMessageWrapper message =
      builder.mergeFrom(TestUtil.getUnpackedSet().toByteString()).build();
    TestUtil.assertPackedFieldsSet((TestPackedTypes) message.wrappedMessage);
  }

  public void testUnpackedParsing() throws Exception {
    AbstractMessageWrapper.Builder builder =
      new AbstractMessageWrapper.Builder(TestUnpackedTypes.newBuilder());
    AbstractMessageWrapper message =
      builder.mergeFrom(TestUtil.getUnpackedSet().toByteString()).build();
    TestUtil.assertUnpackedFieldsSet(
      (TestUnpackedTypes) message.wrappedMessage);
  }

  public void testOptimizedForSize() throws Exception {
    // We're mostly only checking that this class was compiled successfully.
    TestOptimizedForSize message =
      TestOptimizedForSize.newBuilder().setI(1).build();
    message = TestOptimizedForSize.parseFrom(message.toByteString());
    assertEquals(2, message.getSerializedSize());
  }

  // -----------------------------------------------------------------
  // Tests for isInitialized().

  private static final TestRequired TEST_REQUIRED_UNINITIALIZED =
    TestRequired.getDefaultInstance();
  private static final TestRequired TEST_REQUIRED_INITIALIZED =
    TestRequired.newBuilder().setA(1).setB(2).setC(3).build();

  public void testIsInitialized() throws Exception {
    TestRequired.Builder builder = TestRequired.newBuilder();
    AbstractMessageWrapper.Builder abstractBuilder =
      new AbstractMessageWrapper.Builder(builder);

    assertFalse(abstractBuilder.isInitialized());
    assertEquals("a, b, c", abstractBuilder.getInitializationErrorString());
    builder.setA(1);
    assertFalse(abstractBuilder.isInitialized());
    assertEquals("b, c", abstractBuilder.getInitializationErrorString());
    builder.setB(1);
    assertFalse(abstractBuilder.isInitialized());
    assertEquals("c", abstractBuilder.getInitializationErrorString());
    builder.setC(1);
    assertTrue(abstractBuilder.isInitialized());
    assertEquals("", abstractBuilder.getInitializationErrorString());
  }

  public void testForeignIsInitialized() throws Exception {
    TestRequiredForeign.Builder builder = TestRequiredForeign.newBuilder();
    AbstractMessageWrapper.Builder abstractBuilder =
      new AbstractMessageWrapper.Builder(builder);

    assertTrue(abstractBuilder.isInitialized());
    assertEquals("", abstractBuilder.getInitializationErrorString());

    builder.setOptionalMessage(TEST_REQUIRED_UNINITIALIZED);
    assertFalse(abstractBuilder.isInitialized());
    assertEquals(
        "optional_message.a, optional_message.b, optional_message.c",
        abstractBuilder.getInitializationErrorString());

    builder.setOptionalMessage(TEST_REQUIRED_INITIALIZED);
    assertTrue(abstractBuilder.isInitialized());
    assertEquals("", abstractBuilder.getInitializationErrorString());

    builder.addRepeatedMessage(TEST_REQUIRED_UNINITIALIZED);
    assertFalse(abstractBuilder.isInitialized());
    assertEquals(
        "repeated_message[0].a, repeated_message[0].b, repeated_message[0].c",
        abstractBuilder.getInitializationErrorString());

    builder.setRepeatedMessage(0, TEST_REQUIRED_INITIALIZED);
    assertTrue(abstractBuilder.isInitialized());
    assertEquals("", abstractBuilder.getInitializationErrorString());
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
      "optional_int32: 1\n" +
      "optional_int64: 2\n" +
      "optional_string: \"foo\"\n" +
      "optional_foreign_message {\n" +
      "  c: 3\n" +
      "}\n" +
      "repeated_string: \"qux\"\n" +
      "repeated_string: \"bar\"\n";

  public void testMergeFrom() throws Exception {
    AbstractMessageWrapper result =
      new AbstractMessageWrapper.Builder(
        TestAllTypes.newBuilder(MERGE_DEST))
      .mergeFrom(MERGE_SOURCE).build();

    assertEquals(MERGE_RESULT_TEXT, result.toString());
  }

  // -----------------------------------------------------------------
  // Tests for equals and hashCode

  public void testEqualsAndHashCode() throws Exception {
    TestAllTypes a = TestUtil.getAllSet();
    TestAllTypes b = TestAllTypes.newBuilder().build();
    TestAllTypes c = TestAllTypes.newBuilder(b).addRepeatedString("x").build();
    TestAllTypes d = TestAllTypes.newBuilder(c).addRepeatedString("y").build();
    TestAllExtensions e = TestUtil.getAllExtensionsSet();
    TestAllExtensions f = TestAllExtensions.newBuilder(e)
        .addExtension(UnittestProto.repeatedInt32Extension, 999).build();

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
        UnittestProto.TestEmptyMessage.parseFrom(e.toByteArray());
    UnittestProto.TestEmptyMessage fUnknownFields =
        UnittestProto.TestEmptyMessage.parseFrom(f.toByteArray());
    checkNotEqual(eUnknownFields, fUnknownFields);
    checkEqualsIsConsistent(eUnknownFields);
    checkEqualsIsConsistent(fUnknownFields);

    // Subsequent reconstitutions should be identical
    UnittestProto.TestEmptyMessage eUnknownFields2 =
        UnittestProto.TestEmptyMessage.parseFrom(e.toByteArray());
    checkEqualsIsConsistent(eUnknownFields, eUnknownFields2);
  }


  /**
   * Asserts that the given proto has symmetric equals and hashCode methods.
   */
  private void checkEqualsIsConsistent(Message message) {
    // Object should be equal to itself.
    assertEquals(message, message);

    // Object should be equal to a dynamic copy of itself.
    DynamicMessage dynamic = DynamicMessage.newBuilder(message).build();
    checkEqualsIsConsistent(message, dynamic);
  }

  /**
   * Asserts that the given protos are equal and have the same hash code.
   */
  private void checkEqualsIsConsistent(Message message1, Message message2) {
    assertEquals(message1, message2);
    assertEquals(message2, message1);
    assertEquals(message2.hashCode(), message1.hashCode());
  }

  /**
   * Asserts that the given protos are not equal and have different hash codes.
   *
   * @warning It's valid for non-equal objects to have the same hash code, so
   *   this test is stricter than it needs to be. However, this should happen
   *   relatively rarely.
   */
  private void checkNotEqual(Message m1, Message m2) {
    String equalsError = String.format("%s should not be equal to %s", m1, m2);
    assertFalse(equalsError, m1.equals(m2));
    assertFalse(equalsError, m2.equals(m1));

    assertFalse(
        String.format("%s should have a different hash code from %s", m1, m2),
        m1.hashCode() == m2.hashCode());
  }
}
