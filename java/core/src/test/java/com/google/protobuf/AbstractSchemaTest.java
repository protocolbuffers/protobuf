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

import java.io.IOException;
import java.nio.ByteBuffer;
import java.util.Arrays;
import java.util.Collections;
import java.util.List;
import org.junit.Before;
import org.junit.Test;

public abstract class AbstractSchemaTest<T extends MessageLite> {
  private Schema<T> schema;

  @Before
  public void setup() {
    schema = schema();
    registerSchemas();
  }

  // Subclass should override this method if it needs to register more than one schemas.
  protected void registerSchemas() {
    // Register this schema with the runtime to support processing of nested messages.
    Protobuf.getInstance().registerSchemaOverride(schema.newInstance().getClass(), schema);
  }

  protected abstract Schema<T> schema();

  protected abstract ExperimentalMessageFactory<? extends T> messageFactory();

  @SuppressWarnings("unused")
  protected List<ByteBuffer> serializedBytesWithInvalidUtf8() throws IOException {
    return Collections.emptyList();
  }

  @Test
  public void randomMessageShouldRoundtrip() throws IOException {
    roundtrip("", messageFactory().newMessage());
  }

  @Test
  public void invalidUtf8StringParsing() throws IOException {
    for (ByteBuffer invalidUtf8Bytes : serializedBytesWithInvalidUtf8()) {
      Reader reader = BinaryReader.newInstance(invalidUtf8Bytes, /* bufferIsImmutable= */ true);

      T newMsg = schema.newInstance();
      try {
        schema.mergeFrom(newMsg, reader, ExtensionRegistryLite.getEmptyRegistry());
        assertWithMessage("should throw invalid").fail();
      } catch (InvalidProtocolBufferException expected) {
      }
    }
  }

  @Test
  public void mergeFromByteArrayFastPathMayThrowIndexOutOfBoundsException() throws IOException {
    if (!Android.isOnAndroidDevice()) {
      // Skip this test if not on Android.
      return;
    }
    byte[] data = messageFactory().newMessage().toByteArray();
    int exceptionCount = 0;
    for (int i = 0; i <= data.length; i++) {
      byte[] truncatedData = Arrays.copyOf(data, i);
      try {
        T message = schema.newInstance();
        // Test that this method throws the expected exceptions.
        schema.mergeFrom(message, truncatedData, 0, i, new ArrayDecoders.Registers());
      } catch (InvalidProtocolBufferException e) {
        // Ignore expected exceptions.
      } catch (IndexOutOfBoundsException e) {
        exceptionCount += 1;
      }
    }
    assertThat(exceptionCount).isNotEqualTo(0);
  }

  protected static final <M extends MessageLite> void roundtrip(
      String failureMessage, M msg, Schema<M> schema) throws IOException {
    byte[] serializedBytes = ExperimentalSerializationUtil.toByteArray(msg, schema);
    assertWithMessage(failureMessage)
        .that(serializedBytes.length)
        .isEqualTo(msg.getSerializedSize());

    // Now read it back in and verify it matches the original.
    if (Android.isOnAndroidDevice()) {
      // Test the fast path on Android.
      M newMsg = schema.newInstance();
      schema.mergeFrom(
          newMsg, serializedBytes, 0, serializedBytes.length, new ArrayDecoders.Registers());
      schema.makeImmutable(newMsg);
      assertWithMessage(failureMessage).that(newMsg).isEqualTo(msg);
    }
    M newMsg = schema.newInstance();
    Reader reader = BinaryReader.newInstance(ByteBuffer.wrap(serializedBytes), true);
    schema.mergeFrom(newMsg, reader, ExtensionRegistryLite.getEmptyRegistry());
    schema.makeImmutable(newMsg);

    assertWithMessage(failureMessage).that(newMsg).isEqualTo(msg);
  }

  protected final void roundtrip(String failureMessage, T msg) throws IOException {
    roundtrip(failureMessage, msg, schema);
  }

  protected final ExperimentalTestDataProvider data() {
    return messageFactory().dataProvider();
  }

  protected List<T> newMessagesMissingRequiredFields() {
    return Collections.emptyList();
  }

  @SuppressWarnings("unchecked")
  @Test
  public void testRequiredFields() throws Exception {
    for (T msg : newMessagesMissingRequiredFields()) {
      if (schema.isInitialized(msg)) {
        assertThat(msg.toString()).isEmpty();
        msg = (T) msg.toBuilder().build();
      }
      assertThat(schema.isInitialized(msg)).isFalse();
    }
  }
}
