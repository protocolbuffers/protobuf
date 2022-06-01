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

import protobuf_unittest.UnittestProto.ForeignMessage;
import protobuf_unittest.UnittestProto.TestAllExtensions;
import protobuf_unittest.UnittestProto.TestAllTypes;
import protobuf_unittest.UnittestProto.TestRequired;
import protobuf_unittest.UnittestProto.TestRequiredForeign;
import java.util.List;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

/** Misc. unit tests for message operations that apply to both generated and dynamic messages. */
@RunWith(JUnit4.class)
public class MessageTest {
  // =================================================================
  // Message-merging tests.

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
  public void testParsingWithNullExtensionRegistry() throws Exception {
    try {
      TestAllTypes.parseFrom(new byte[] {}, null);
      assertWithMessage("Expected exception").fail();
    } catch (NullPointerException expected) {
    }
  }

  @Test
  public void testMergeFrom() throws Exception {
    TestAllTypes result = TestAllTypes.newBuilder(MERGE_DEST).mergeFrom(MERGE_SOURCE).build();

    assertThat(result.toString()).isEqualTo(MERGE_RESULT_TEXT);
  }

  /**
   * Test merging a DynamicMessage into a GeneratedMessage. As long as they have the same
   * descriptor, this should work, but it is an entirely different code path.
   */
  @Test
  public void testMergeFromDynamic() throws Exception {
    TestAllTypes result =
        TestAllTypes.newBuilder(MERGE_DEST)
            .mergeFrom(DynamicMessage.newBuilder(MERGE_SOURCE).build())
            .build();

    assertThat(result.toString()).isEqualTo(MERGE_RESULT_TEXT);
  }

  /** Test merging two DynamicMessages. */
  @Test
  public void testDynamicMergeFrom() throws Exception {
    DynamicMessage result =
        DynamicMessage.newBuilder(MERGE_DEST)
            .mergeFrom(DynamicMessage.newBuilder(MERGE_SOURCE).build())
            .build();

    assertThat(result.toString()).isEqualTo(MERGE_RESULT_TEXT);
  }

  // =================================================================
  // Required-field-related tests.

  private static final TestRequired TEST_REQUIRED_UNINITIALIZED = TestRequired.getDefaultInstance();
  private static final TestRequired TEST_REQUIRED_INITIALIZED =
      TestRequired.newBuilder().setA(1).setB(2).setC(3).build();

  @Test
  public void testRequired() throws Exception {
    TestRequired.Builder builder = TestRequired.newBuilder();

    assertThat(builder.isInitialized()).isFalse();
    builder.setA(1);
    assertThat(builder.isInitialized()).isFalse();
    builder.setB(1);
    assertThat(builder.isInitialized()).isFalse();
    builder.setC(1);
    assertThat(builder.isInitialized()).isTrue();
  }

  @Test
  public void testRequiredForeign() throws Exception {
    TestRequiredForeign.Builder builder = TestRequiredForeign.newBuilder();

    assertThat(builder.isInitialized()).isTrue();

    builder.setOptionalMessage(TEST_REQUIRED_UNINITIALIZED);
    assertThat(builder.isInitialized()).isFalse();

    builder.setOptionalMessage(TEST_REQUIRED_INITIALIZED);
    assertThat(builder.isInitialized()).isTrue();

    builder.addRepeatedMessage(TEST_REQUIRED_UNINITIALIZED);
    assertThat(builder.isInitialized()).isFalse();

    builder.setRepeatedMessage(0, TEST_REQUIRED_INITIALIZED);
    assertThat(builder.isInitialized()).isTrue();
  }

  @Test
  public void testRequiredExtension() throws Exception {
    TestAllExtensions.Builder builder = TestAllExtensions.newBuilder();

    assertThat(builder.isInitialized()).isTrue();

    builder.setExtension(TestRequired.single, TEST_REQUIRED_UNINITIALIZED);
    assertThat(builder.isInitialized()).isFalse();

    builder.setExtension(TestRequired.single, TEST_REQUIRED_INITIALIZED);
    assertThat(builder.isInitialized()).isTrue();

    builder.addExtension(TestRequired.multi, TEST_REQUIRED_UNINITIALIZED);
    assertThat(builder.isInitialized()).isFalse();

    builder.setExtension(TestRequired.multi, 0, TEST_REQUIRED_INITIALIZED);
    assertThat(builder.isInitialized()).isTrue();
  }

  @Test
  public void testRequiredDynamic() throws Exception {
    Descriptors.Descriptor descriptor = TestRequired.getDescriptor();
    DynamicMessage.Builder builder = DynamicMessage.newBuilder(descriptor);

    assertThat(builder.isInitialized()).isFalse();
    builder.setField(descriptor.findFieldByName("a"), 1);
    assertThat(builder.isInitialized()).isFalse();
    builder.setField(descriptor.findFieldByName("b"), 1);
    assertThat(builder.isInitialized()).isFalse();
    builder.setField(descriptor.findFieldByName("c"), 1);
    assertThat(builder.isInitialized()).isTrue();
  }

  @Test
  public void testRequiredDynamicForeign() throws Exception {
    Descriptors.Descriptor descriptor = TestRequiredForeign.getDescriptor();
    DynamicMessage.Builder builder = DynamicMessage.newBuilder(descriptor);

    assertThat(builder.isInitialized()).isTrue();

    builder.setField(descriptor.findFieldByName("optional_message"), TEST_REQUIRED_UNINITIALIZED);
    assertThat(builder.isInitialized()).isFalse();

    builder.setField(descriptor.findFieldByName("optional_message"), TEST_REQUIRED_INITIALIZED);
    assertThat(builder.isInitialized()).isTrue();

    builder.addRepeatedField(
        descriptor.findFieldByName("repeated_message"), TEST_REQUIRED_UNINITIALIZED);
    assertThat(builder.isInitialized()).isFalse();

    builder.setRepeatedField(
        descriptor.findFieldByName("repeated_message"), 0, TEST_REQUIRED_INITIALIZED);
    assertThat(builder.isInitialized()).isTrue();
  }

  @Test
  public void testUninitializedException() throws Exception {
    try {
      TestRequired.newBuilder().build();
      assertWithMessage("Should have thrown an exception.").fail();
    } catch (UninitializedMessageException e) {
      assertThat(e).hasMessageThat().isEqualTo("Message missing required fields: a, b, c");
    }
  }

  @Test
  public void testBuildPartial() throws Exception {
    // We're mostly testing that no exception is thrown.
    TestRequired message = TestRequired.newBuilder().buildPartial();
    assertThat(message.isInitialized()).isFalse();
  }

  @Test
  public void testNestedUninitializedException() throws Exception {
    try {
      TestRequiredForeign.newBuilder()
          .setOptionalMessage(TEST_REQUIRED_UNINITIALIZED)
          .addRepeatedMessage(TEST_REQUIRED_UNINITIALIZED)
          .addRepeatedMessage(TEST_REQUIRED_UNINITIALIZED)
          .build();
      assertWithMessage("Should have thrown an exception.").fail();
    } catch (UninitializedMessageException e) {
      assertThat(e)
          .hasMessageThat()
          .isEqualTo(
              "Message missing required fields: "
                  + "optional_message.a, "
                  + "optional_message.b, "
                  + "optional_message.c, "
                  + "repeated_message[0].a, "
                  + "repeated_message[0].b, "
                  + "repeated_message[0].c, "
                  + "repeated_message[1].a, "
                  + "repeated_message[1].b, "
                  + "repeated_message[1].c");
    }
  }

  @Test
  public void testBuildNestedPartial() throws Exception {
    // We're mostly testing that no exception is thrown.
    TestRequiredForeign message =
        TestRequiredForeign.newBuilder()
            .setOptionalMessage(TEST_REQUIRED_UNINITIALIZED)
            .addRepeatedMessage(TEST_REQUIRED_UNINITIALIZED)
            .addRepeatedMessage(TEST_REQUIRED_UNINITIALIZED)
            .buildPartial();
    assertThat(message.isInitialized()).isFalse();
  }

  @Test
  public void testParseUnititialized() throws Exception {
    try {
      TestRequired.parseFrom(ByteString.EMPTY);
      assertWithMessage("Should have thrown an exception.").fail();
    } catch (InvalidProtocolBufferException e) {
      assertThat(e).hasMessageThat().isEqualTo("Message missing required fields: a, b, c");
    }
  }

  @Test
  public void testParseNestedUnititialized() throws Exception {
    ByteString data =
        TestRequiredForeign.newBuilder()
            .setOptionalMessage(TEST_REQUIRED_UNINITIALIZED)
            .addRepeatedMessage(TEST_REQUIRED_UNINITIALIZED)
            .addRepeatedMessage(TEST_REQUIRED_UNINITIALIZED)
            .buildPartial()
            .toByteString();

    try {
      TestRequiredForeign.parseFrom(data);
      assertWithMessage("Should have thrown an exception.").fail();
    } catch (InvalidProtocolBufferException e) {
      assertThat(e)
          .hasMessageThat()
          .isEqualTo(
              "Message missing required fields: "
                  + "optional_message.a, "
                  + "optional_message.b, "
                  + "optional_message.c, "
                  + "repeated_message[0].a, "
                  + "repeated_message[0].b, "
                  + "repeated_message[0].c, "
                  + "repeated_message[1].a, "
                  + "repeated_message[1].b, "
                  + "repeated_message[1].c");
    }
  }

  @Test
  public void testDynamicUninitializedException() throws Exception {
    try {
      DynamicMessage.newBuilder(TestRequired.getDescriptor()).build();
      assertWithMessage("Should have thrown an exception.").fail();
    } catch (UninitializedMessageException e) {
      assertThat(e).hasMessageThat().isEqualTo("Message missing required fields: a, b, c");
    }
  }

  @Test
  public void testDynamicBuildPartial() throws Exception {
    // We're mostly testing that no exception is thrown.
    DynamicMessage message = DynamicMessage.newBuilder(TestRequired.getDescriptor()).buildPartial();
    assertThat(message.isInitialized()).isFalse();
  }

  @Test
  public void testDynamicParseUnititialized() throws Exception {
    try {
      Descriptors.Descriptor descriptor = TestRequired.getDescriptor();
      DynamicMessage.parseFrom(descriptor, ByteString.EMPTY);
      assertWithMessage("Should have thrown an exception.").fail();
    } catch (InvalidProtocolBufferException e) {
      assertThat(e).hasMessageThat().isEqualTo("Message missing required fields: a, b, c");
    }
  }

  /** Test reading unset repeated message from DynamicMessage. */
  @Test
  public void testDynamicRepeatedMessageNull() throws Exception {
    Descriptors.Descriptor unused = TestRequired.getDescriptor();
    DynamicMessage result =
        DynamicMessage.newBuilder(TestAllTypes.getDescriptor())
            .mergeFrom(DynamicMessage.newBuilder(MERGE_SOURCE).build())
            .build();

    assertThat(
            result.getField(
                result.getDescriptorForType().findFieldByName("repeated_foreign_message")))
        .isInstanceOf(List.class);
    assertThat(
            result.getRepeatedFieldCount(
                result.getDescriptorForType().findFieldByName("repeated_foreign_message")))
        .isEqualTo(0);
  }

  /** Test reading repeated message from DynamicMessage. */
  @Test
  public void testDynamicRepeatedMessageNotNull() throws Exception {
    TestAllTypes repeatedNested =
        TestAllTypes.newBuilder()
            .setOptionalInt32(1)
            .setOptionalString("foo")
            .setOptionalForeignMessage(ForeignMessage.getDefaultInstance())
            .addRepeatedString("bar")
            .addRepeatedForeignMessage(ForeignMessage.getDefaultInstance())
            .addRepeatedForeignMessage(ForeignMessage.getDefaultInstance())
            .build();
    Descriptors.Descriptor unused = TestRequired.getDescriptor();
    DynamicMessage result =
        DynamicMessage.newBuilder(TestAllTypes.getDescriptor())
            .mergeFrom(DynamicMessage.newBuilder(repeatedNested).build())
            .build();

    assertThat(
            result.getField(
                result.getDescriptorForType().findFieldByName("repeated_foreign_message")))
        .isInstanceOf(List.class);
    assertThat(
            result.getRepeatedFieldCount(
                result.getDescriptorForType().findFieldByName("repeated_foreign_message")))
        .isEqualTo(2);
  }

  @Test
  public void testPreservesFloatingPointNegative0() throws Exception {
    proto3_unittest.UnittestProto3.TestAllTypes message =
        proto3_unittest.UnittestProto3.TestAllTypes.newBuilder()
            .setOptionalFloat(-0.0f)
            .setOptionalDouble(-0.0)
            .build();
    assertThat(
            proto3_unittest.UnittestProto3.TestAllTypes.parseFrom(
                message.toByteString(), ExtensionRegistry.getEmptyRegistry()))
        .isEqualTo(message);
  }

  @Test
  public void testNegative0FloatingPointEquality() throws Exception {
    // Like Double#equals and Float#equals, we treat -0.0 as not being equal to +0.0 even though
    // IEEE 754 mandates that they are equivalent. This test asserts that behavior.
    proto3_unittest.UnittestProto3.TestAllTypes message1 =
        proto3_unittest.UnittestProto3.TestAllTypes.newBuilder()
            .setOptionalFloat(-0.0f)
            .setOptionalDouble(-0.0)
            .build();
    proto3_unittest.UnittestProto3.TestAllTypes message2 =
        proto3_unittest.UnittestProto3.TestAllTypes.newBuilder()
            .setOptionalFloat(0.0f)
            .setOptionalDouble(0.0)
            .build();
    assertThat(message1).isNotEqualTo(message2);
  }
}
