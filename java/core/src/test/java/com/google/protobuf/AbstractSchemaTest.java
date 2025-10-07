// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

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
