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

import protobuf_unittest.UnittestProto.ForeignMessage;
import protobuf_unittest.UnittestProto.TestAllExtensions;
import protobuf_unittest.UnittestProto.TestAllTypes;
import protobuf_unittest.UnittestProto.TestRequired;
import protobuf_unittest.UnittestProto.TestRequiredForeign;
import java.util.List;
import junit.framework.TestCase;

/**
 * Misc. unit tests for message operations that apply to both generated and dynamic messages.
 *
 * @author kenton@google.com Kenton Varda
 */
public class MessageTest extends TestCase {
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

  public void testParsingWithNullExtensionRegistry() throws Exception {
    try {
      TestAllTypes.parseFrom(new byte[] {}, null);
      fail();
    } catch (NullPointerException expected) {
    }
  }

  public void testMergeFrom() throws Exception {
    TestAllTypes result = TestAllTypes.newBuilder(MERGE_DEST).mergeFrom(MERGE_SOURCE).build();

    assertEquals(MERGE_RESULT_TEXT, result.toString());
  }

  /**
   * Test merging a DynamicMessage into a GeneratedMessage. As long as they have the same
   * descriptor, this should work, but it is an entirely different code path.
   */
  public void testMergeFromDynamic() throws Exception {
    TestAllTypes result =
        TestAllTypes.newBuilder(MERGE_DEST)
            .mergeFrom(DynamicMessage.newBuilder(MERGE_SOURCE).build())
            .build();

    assertEquals(MERGE_RESULT_TEXT, result.toString());
  }

  /** Test merging two DynamicMessages. */
  public void testDynamicMergeFrom() throws Exception {
    DynamicMessage result =
        DynamicMessage.newBuilder(MERGE_DEST)
            .mergeFrom(DynamicMessage.newBuilder(MERGE_SOURCE).build())
            .build();

    assertEquals(MERGE_RESULT_TEXT, result.toString());
  }

  // =================================================================
  // Required-field-related tests.

  private static final TestRequired TEST_REQUIRED_UNINITIALIZED = TestRequired.getDefaultInstance();
  private static final TestRequired TEST_REQUIRED_INITIALIZED =
      TestRequired.newBuilder().setA(1).setB(2).setC(3).build();

  public void testRequired() throws Exception {
    TestRequired.Builder builder = TestRequired.newBuilder();

    assertFalse(builder.isInitialized());
    builder.setA(1);
    assertFalse(builder.isInitialized());
    builder.setB(1);
    assertFalse(builder.isInitialized());
    builder.setC(1);
    assertTrue(builder.isInitialized());
  }

  public void testRequiredForeign() throws Exception {
    TestRequiredForeign.Builder builder = TestRequiredForeign.newBuilder();

    assertTrue(builder.isInitialized());

    builder.setOptionalMessage(TEST_REQUIRED_UNINITIALIZED);
    assertFalse(builder.isInitialized());

    builder.setOptionalMessage(TEST_REQUIRED_INITIALIZED);
    assertTrue(builder.isInitialized());

    builder.addRepeatedMessage(TEST_REQUIRED_UNINITIALIZED);
    assertFalse(builder.isInitialized());

    builder.setRepeatedMessage(0, TEST_REQUIRED_INITIALIZED);
    assertTrue(builder.isInitialized());
  }

  public void testRequiredExtension() throws Exception {
    TestAllExtensions.Builder builder = TestAllExtensions.newBuilder();

    assertTrue(builder.isInitialized());

    builder.setExtension(TestRequired.single, TEST_REQUIRED_UNINITIALIZED);
    assertFalse(builder.isInitialized());

    builder.setExtension(TestRequired.single, TEST_REQUIRED_INITIALIZED);
    assertTrue(builder.isInitialized());

    builder.addExtension(TestRequired.multi, TEST_REQUIRED_UNINITIALIZED);
    assertFalse(builder.isInitialized());

    builder.setExtension(TestRequired.multi, 0, TEST_REQUIRED_INITIALIZED);
    assertTrue(builder.isInitialized());
  }

  public void testRequiredDynamic() throws Exception {
    Descriptors.Descriptor descriptor = TestRequired.getDescriptor();
    DynamicMessage.Builder builder = DynamicMessage.newBuilder(descriptor);

    assertFalse(builder.isInitialized());
    builder.setField(descriptor.findFieldByName("a"), 1);
    assertFalse(builder.isInitialized());
    builder.setField(descriptor.findFieldByName("b"), 1);
    assertFalse(builder.isInitialized());
    builder.setField(descriptor.findFieldByName("c"), 1);
    assertTrue(builder.isInitialized());
  }

  public void testRequiredDynamicForeign() throws Exception {
    Descriptors.Descriptor descriptor = TestRequiredForeign.getDescriptor();
    DynamicMessage.Builder builder = DynamicMessage.newBuilder(descriptor);

    assertTrue(builder.isInitialized());

    builder.setField(descriptor.findFieldByName("optional_message"), TEST_REQUIRED_UNINITIALIZED);
    assertFalse(builder.isInitialized());

    builder.setField(descriptor.findFieldByName("optional_message"), TEST_REQUIRED_INITIALIZED);
    assertTrue(builder.isInitialized());

    builder.addRepeatedField(
        descriptor.findFieldByName("repeated_message"), TEST_REQUIRED_UNINITIALIZED);
    assertFalse(builder.isInitialized());

    builder.setRepeatedField(
        descriptor.findFieldByName("repeated_message"), 0, TEST_REQUIRED_INITIALIZED);
    assertTrue(builder.isInitialized());
  }

  public void testUninitializedException() throws Exception {
    try {
      TestRequired.newBuilder().build();
      fail("Should have thrown an exception.");
    } catch (UninitializedMessageException e) {
      assertEquals("Message missing required fields: a, b, c", e.getMessage());
    }
  }

  public void testBuildPartial() throws Exception {
    // We're mostly testing that no exception is thrown.
    TestRequired message = TestRequired.newBuilder().buildPartial();
    assertFalse(message.isInitialized());
  }

  public void testNestedUninitializedException() throws Exception {
    try {
      TestRequiredForeign.newBuilder()
          .setOptionalMessage(TEST_REQUIRED_UNINITIALIZED)
          .addRepeatedMessage(TEST_REQUIRED_UNINITIALIZED)
          .addRepeatedMessage(TEST_REQUIRED_UNINITIALIZED)
          .build();
      fail("Should have thrown an exception.");
    } catch (UninitializedMessageException e) {
      assertEquals(
          "Message missing required fields: "
              + "optional_message.a, "
              + "optional_message.b, "
              + "optional_message.c, "
              + "repeated_message[0].a, "
              + "repeated_message[0].b, "
              + "repeated_message[0].c, "
              + "repeated_message[1].a, "
              + "repeated_message[1].b, "
              + "repeated_message[1].c",
          e.getMessage());
    }
  }

  public void testBuildNestedPartial() throws Exception {
    // We're mostly testing that no exception is thrown.
    TestRequiredForeign message =
        TestRequiredForeign.newBuilder()
            .setOptionalMessage(TEST_REQUIRED_UNINITIALIZED)
            .addRepeatedMessage(TEST_REQUIRED_UNINITIALIZED)
            .addRepeatedMessage(TEST_REQUIRED_UNINITIALIZED)
            .buildPartial();
    assertFalse(message.isInitialized());
  }

  public void testParseUnititialized() throws Exception {
    try {
      TestRequired.parseFrom(ByteString.EMPTY);
      fail("Should have thrown an exception.");
    } catch (InvalidProtocolBufferException e) {
      assertEquals("Message missing required fields: a, b, c", e.getMessage());
    }
  }

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
      fail("Should have thrown an exception.");
    } catch (InvalidProtocolBufferException e) {
      assertEquals(
          "Message missing required fields: "
              + "optional_message.a, "
              + "optional_message.b, "
              + "optional_message.c, "
              + "repeated_message[0].a, "
              + "repeated_message[0].b, "
              + "repeated_message[0].c, "
              + "repeated_message[1].a, "
              + "repeated_message[1].b, "
              + "repeated_message[1].c",
          e.getMessage());
    }
  }

  public void testDynamicUninitializedException() throws Exception {
    try {
      DynamicMessage.newBuilder(TestRequired.getDescriptor()).build();
      fail("Should have thrown an exception.");
    } catch (UninitializedMessageException e) {
      assertEquals("Message missing required fields: a, b, c", e.getMessage());
    }
  }

  public void testDynamicBuildPartial() throws Exception {
    // We're mostly testing that no exception is thrown.
    DynamicMessage message = DynamicMessage.newBuilder(TestRequired.getDescriptor()).buildPartial();
    assertFalse(message.isInitialized());
  }

  public void testDynamicParseUnititialized() throws Exception {
    try {
      Descriptors.Descriptor descriptor = TestRequired.getDescriptor();
      DynamicMessage.parseFrom(descriptor, ByteString.EMPTY);
      fail("Should have thrown an exception.");
    } catch (InvalidProtocolBufferException e) {
      assertEquals("Message missing required fields: a, b, c", e.getMessage());
    }
  }

  /** Test reading unset repeated message from DynamicMessage. */
  public void testDynamicRepeatedMessageNull() throws Exception {
    TestRequired.getDescriptor();
    DynamicMessage result =
        DynamicMessage.newBuilder(TestAllTypes.getDescriptor())
            .mergeFrom(DynamicMessage.newBuilder(MERGE_SOURCE).build())
            .build();

    assertTrue(
        result.getField(result.getDescriptorForType().findFieldByName("repeated_foreign_message"))
            instanceof List<?>);
    assertEquals(
        0,
        result.getRepeatedFieldCount(
            result.getDescriptorForType().findFieldByName("repeated_foreign_message")));
  }

  /** Test reading repeated message from DynamicMessage. */
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
    TestRequired.getDescriptor();
    DynamicMessage result =
        DynamicMessage.newBuilder(TestAllTypes.getDescriptor())
            .mergeFrom(DynamicMessage.newBuilder(repeatedNested).build())
            .build();

    assertTrue(
        result.getField(result.getDescriptorForType().findFieldByName("repeated_foreign_message"))
            instanceof List<?>);
    assertEquals(
        2,
        result.getRepeatedFieldCount(
            result.getDescriptorForType().findFieldByName("repeated_foreign_message")));
  }
}
