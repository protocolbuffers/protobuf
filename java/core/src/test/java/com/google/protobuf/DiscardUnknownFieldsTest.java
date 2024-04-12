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
