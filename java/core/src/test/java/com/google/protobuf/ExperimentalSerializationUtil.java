// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

package com.google.protobuf;

import java.io.IOException;
import java.nio.ByteBuffer;
import java.util.Queue;

/** Utilities for serialization. */
public class ExperimentalSerializationUtil {

  /**
   * Serializes the given message to a byte array using {@link com.google.protobuf.BinaryWriter}.
   */
  public static <T> byte[] toByteArray(T msg) throws IOException {
    return toByteArray(msg, Protobuf.getInstance().schemaFor(msg));
  }

  /**
   * Serializes the given message to a byte array using {@link com.google.protobuf.BinaryWriter}
   * with a customized Schema.
   */
  public static <T> byte[] toByteArray(T msg, Schema<T> schema) throws IOException {
    BinaryWriter writer = BinaryWriter.newHeapInstance(BufferAllocator.unpooled());
    schema.writeTo(msg, writer);

    byte[] out = new byte[writer.getTotalBytesWritten()];
    int outPos = 0;
    Queue<AllocatedBuffer> buffers = writer.complete();
    while (true) {
      AllocatedBuffer buffer = buffers.poll();
      if (buffer == null) {
        break;
      }
      int length = buffer.limit() - buffer.position();
      System.arraycopy(
          buffer.array(), buffer.arrayOffset() + buffer.position(), out, outPos, length);
      outPos += length;
    }
    if (out.length != outPos) {
      throw new IllegalArgumentException("Failed to serialize test message");
    }
    return out;
  }

  /** Deserializes a message from the given byte array. */
  public static <T> T fromByteArray(byte[] data, Class<T> messageType) {
    if (Android.isOnAndroidDevice()) {
      return fromByteArrayFastPath(data, messageType);
    } else {
      return fromByteArray(data, messageType, ExtensionRegistryLite.getEmptyRegistry());
    }
  }

  /**
   * Deserializes a message from the given byte array using {@link com.google.protobuf.BinaryReader}
   * with an extension registry and a customized Schema.
   */
  public static <T> T fromByteArray(
      byte[] data, Class<T> messageType, ExtensionRegistryLite extensionRegistry) {
    try {
      Schema<T> schema = Protobuf.getInstance().schemaFor(messageType);
      T msg = schema.newInstance();
      schema.mergeFrom(
          msg, BinaryReader.newInstance(ByteBuffer.wrap(data), true), extensionRegistry);
      schema.makeImmutable(msg);
      return msg;
    } catch (IOException e) {
      throw new RuntimeException(e);
    }
  }

  /** Deserializes a lite message from the given byte array using fast path. */
  private static <T> T fromByteArrayFastPath(byte[] data, Class<T> messageType) {
    try {
      Schema<T> schema = Protobuf.getInstance().schemaFor(messageType);
      T msg = schema.newInstance();
      schema.mergeFrom(msg, data, 0, data.length, new ArrayDecoders.Registers());
      schema.makeImmutable(msg);
      return msg;
    } catch (IOException e) {
      throw new RuntimeException(e);
    }
  }
}
