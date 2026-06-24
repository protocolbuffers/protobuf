// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

package com.google.protobuf;

import static com.google.protobuf.Internal.checkNotNull;

import java.io.IOException;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.ConcurrentMap;

@ExperimentalApi
@CheckReturnValue
final class Protobuf {
  private static final Protobuf INSTANCE = new Protobuf();

  private final ManifestSchemaFactory schemaFactory;

  // TODO: b/341207042 - Consider using ClassValue instead.
  private final ConcurrentMap<Class<?>, Schema<?>> schemaCache =
      new ConcurrentHashMap<Class<?>, Schema<?>>();

  /** Gets the singleton instance of the Protobuf runtime. */
  static Protobuf getInstance() {
    return INSTANCE;
  }

  /** Writes the given message to the target {@link CodedOutputStreamWriter}. */
  <T extends GeneratedMessageLite<?, ?>> void writeTo(T message, CodedOutputStreamWriter writer)
      throws IOException {
    schemaFor(message).writeTo(message, writer);
  }

  /**
   * Reads fields from the given {@link CodedInputStreamReader} and merges them into the message.
   */
  <T extends GeneratedMessageLite<?, ?>> void mergeFrom(
      T message, CodedInputStreamReader reader, ExtensionRegistryLite extensionRegistry)
      throws IOException {
    schemaFor(message).mergeFrom(message, reader, extensionRegistry);
  }

  /** Checks if all required fields are set. */
  <T extends GeneratedMessageLite<?, ?>> boolean isInitialized(T message) {
    return schemaFor(message).isInitialized(message);
  }

  /** Gets the schema for the given message type. */
  @SuppressWarnings("unchecked")
  <T extends GeneratedMessageLite<?, ?>> Schema<T> schemaFor(Class<T> messageType) {
    Object schema = schemaCache.get(messageType);
    if (schema == null) {
      return registerSchema(messageType);
    }
    return (Schema<T>) schema;
  }

  @DoNotInline
  private <T extends GeneratedMessageLite<?, ?>> Schema<T> registerSchema(Class<T> messageType) {
    Schema<T> schema = schemaFactory.createSchema(messageType);
    checkNotNull(schema, "schema");
    @SuppressWarnings("unchecked")
    Schema<T> previous = (Schema<T>) schemaCache.putIfAbsent(messageType, schema);
    if (previous != null) {
      // A new schema was registered by another thread.
      schema = previous;
    }
    return schema;
  }

  /** Gets the schema for the given message. */
  @SuppressWarnings("unchecked")
  <T extends GeneratedMessageLite<?, ?>> Schema<T> schemaFor(T message) {
    return schemaFor((Class<T>) message.getClass());
  }

  private Protobuf() {
    schemaFactory = new ManifestSchemaFactory();
  }
}
