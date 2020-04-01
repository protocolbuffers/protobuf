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
