// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

package com.google.protobuf;

import static com.google.common.truth.Truth.assertWithMessage;

import protobuf_unittest.UnittestProto;
import proto3_unittest.UnittestProto3;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

/** Unit tests for discard or preserve unknown fields. */
@RunWith(JUnit4.class)
public class DiscardUnknownFieldsTest {
  @Test
  public void testProto2() throws Exception {
    testProto2Message(UnittestProto.TestEmptyMessage.getDefaultInstance());
    testProto2Message(UnittestProto.TestEmptyMessageWithExtensions.getDefaultInstance());
    testProto2Message(
        DynamicMessage.getDefaultInstance(UnittestProto.TestEmptyMessage.getDescriptor()));
    testProto2Message(
        DynamicMessage.getDefaultInstance(
            UnittestProto.TestEmptyMessageWithExtensions.getDescriptor()));
  }

  @Test
  public void testProto3() throws Exception {
    testProto3Message(UnittestProto3.TestEmptyMessage.getDefaultInstance());
    testProto3Message(
        DynamicMessage.getDefaultInstance(UnittestProto3.TestEmptyMessage.getDescriptor()));
  }

  private static void testProto2Message(Message message) throws Exception {
    assertUnknownFieldsPreserved(message);
    assertUnknownFieldsExplicitlyDiscarded(message);
    assertReuseCodedInputStreamPreserve(message);
    assertUnknownFieldsInUnknownFieldSetArePreserve(message);
  }

  private static void testProto3Message(Message message) throws Exception {
    assertUnknownFieldsPreserved(message);
    assertUnknownFieldsExplicitlyDiscarded(message);
    assertReuseCodedInputStreamPreserve(message);
    assertUnknownFieldsInUnknownFieldSetArePreserve(message);
  }

  private static void assertReuseCodedInputStreamPreserve(Message message) throws Exception {
    final int messageSize = payload.size();
    byte[] copied = new byte[messageSize * 2];
    payload.copyTo(copied, 0);
    payload.copyTo(copied, messageSize);
    CodedInputStream input = CodedInputStream.newInstance(copied);

    // Use DiscardUnknownFieldsParser to parse the first payload.
    int oldLimit = input.pushLimit(messageSize);
    Message parsed = DiscardUnknownFieldsParser.wrap(message.getParserForType()).parseFrom(input);
    assertWithMessage(message.getClass().getName()).that(parsed.getSerializedSize()).isEqualTo(0);
    input.popLimit(oldLimit);

    // Use the normal parser to parse the remaining payload should have unknown fields preserved.
    parsed = message.getParserForType().parseFrom(input);
    assertWithMessage(message.getClass().getName()).that(parsed.toByteString()).isEqualTo(payload);
  }

  /**
   * {@link Message.Builder#setUnknownFields(UnknownFieldSet)} and {@link
   * Message.Builder#mergeUnknownFields(UnknownFieldSet)} should preserve the unknown fields.
   */
  private static void assertUnknownFieldsInUnknownFieldSetArePreserve(Message message)
      throws Exception {
    UnknownFieldSet unknownFields = UnknownFieldSet.newBuilder().mergeFrom(payload).build();
    Message built = message.newBuilderForType().setUnknownFields(unknownFields).build();
    assertWithMessage(message.getClass().getName()).that(built.toByteString()).isEqualTo(payload);
  }

  private static void assertUnknownFieldsPreserved(MessageLite message) throws Exception {
    MessageLite parsed = message.getParserForType().parseFrom(payload);
    assertWithMessage(message.getClass().getName()).that(parsed.toByteString()).isEqualTo(payload);

    parsed = message.newBuilderForType().mergeFrom(payload).build();
    assertWithMessage(message.getClass().getName()).that(parsed.toByteString()).isEqualTo(payload);
  }

  private static void assertUnknownFieldsExplicitlyDiscarded(Message message) throws Exception {
    Message parsed = DiscardUnknownFieldsParser.wrap(message.getParserForType()).parseFrom(payload);
    assertWithMessage(message.getClass().getName()).that(parsed.getSerializedSize()).isEqualTo(0);
  }

  private static final ByteString payload =
      TestUtilLite.getAllLiteSetBuilder().build().toByteString();
}
