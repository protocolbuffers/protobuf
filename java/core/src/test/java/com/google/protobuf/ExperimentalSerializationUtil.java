// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

package com.google.protobuf;

import java.io.IOException;
import java.lang.reflect.Method;

/**
 * Legacy utility that was used for serialization on the now defunct experimental runtime.
 *
 * <p>This now uses only the standard parse/serialize methods and should be removed.
 */
@SuppressWarnings("unchecked")
public class ExperimentalSerializationUtil {
  private ExperimentalSerializationUtil() {}

  /** Serializes the given message to a byte array. */
  @InlineMe(replacement = "msg.toByteArray()")
  public static <T extends MessageLite> byte[] toByteArray(T msg) throws IOException {
    return msg.toByteArray();
  }

  /** Serializes the given message to a byte array. */
  @InlineMe(replacement = "msg.toByteArray()")
  public static <T extends MessageLite> byte[] toByteArray(T msg, Schema<T> schema)
      throws IOException {
    return msg.toByteArray();
  }

  /** Deserializes a message from the given byte array. */
  public static <T extends MessageLite> T fromByteArray(byte[] data, Class<T> messageType) {
    return ExperimentalSerializationUtil.fromByteArray(
        data, messageType, ExtensionRegistryLite.getEmptyRegistry());
  }

  /**
   * Deserializes a message from the given byte array using {@link com.google.protobuf.BinaryReader}
   * with an extension registry and a customized Schema.
   */
  public static <T extends MessageLite> T fromByteArray(
      byte[] data, Class<T> messageType, ExtensionRegistryLite extensionRegistry) {
    try {
      Method method = messageType.getMethod("getDefaultInstance");
      T defaultInstance = (T) method.invoke(null);
      return (T)
          defaultInstance.newBuilderForType().mergeFrom(data, extensionRegistry).buildPartial();
    } catch (Exception e) {
      throw new RuntimeException(e);
    }
  }
}
