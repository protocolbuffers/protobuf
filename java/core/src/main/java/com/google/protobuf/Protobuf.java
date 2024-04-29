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

/**
 * Main runtime interface for protobuf. Applications should interact with this interface (rather
 * than directly accessing internal APIs) in order to perform operations on protobuf messages.
 */
@ExperimentalApi
@CheckReturnValue
final class Protobuf {
  private static final Protobuf INSTANCE = new Protobuf();

  private final SchemaFactory schemaFactory;

  // TODO: Consider using ClassValue instead.
  private final ConcurrentMap<Class<?>, Schema<?>> schemaCache =
      new ConcurrentHashMap<Class<?>, Schema<?>>();

  /** Gets the singleton instance of the Protobuf runtime. */
  public static Protobuf getInstance() {
    return INSTANCE;
  }

  /** Writes the given message to the target {@link Writer}. */
  public <T> void writeTo(T message, Writer writer) throws IOException {
    schemaFor(message).writeTo(message, writer);
  }

  /** Reads fields from the given {@link Reader} and merges them into the message. */
  public <T> void mergeFrom(T message, Reader reader) throws IOException {
    mergeFrom(message, reader, ExtensionRegistryLite.getEmptyRegistry());
  }

  /** Reads fields from the given {@link Reader} and merges them into the message. */
  public <T> void mergeFrom(T message, Reader reader, ExtensionRegistryLite extensionRegistry)
      throws IOException {
    schemaFor(message).mergeFrom(message, reader, extensionRegistry);
  }

  /** Marks repeated/map/extension/unknown fields as immutable. */
  public <T> void makeImmutable(T message) {
    schemaFor(message).makeImmutable(message);
  }

  /** Checks if all required fields are set. */
  <T> boolean isInitialized(T message) {
    return schemaFor(message).isInitialized(message);
  }

  /** Gets the schema for the given message type. */
  public <T> Schema<T> schemaFor(Class<T> messageType) {
    checkNotNull(messageType, "messageType");
    @SuppressWarnings("unchecked")
    Schema<T> schema = (Schema<T>) schemaCache.get(messageType);
    if (schema == null) {
      schema = schemaFactory.createSchema(messageType);
      @SuppressWarnings("unchecked")
      Schema<T> previous = (Schema<T>) registerSchema(messageType, schema);
      if (previous != null) {
        // A new schema was registered by another thread.
        schema = previous;
      }
    }
    return schema;
  }

  /** Gets the schema for the given message. */
  @SuppressWarnings("unchecked")
  public <T> Schema<T> schemaFor(T message) {
    return schemaFor((Class<T>) message.getClass());
  }

  /**
   * Registers the given schema for the message type only if a schema was not already registered.
   *
   * @param messageType the type of message on which the schema operates.
   * @param schema the schema for the message type.
   * @return the previously registered schema, or {@code null} if the given schema was successfully
   *     registered.
   */
  public Schema<?> registerSchema(Class<?> messageType, Schema<?> schema) {
    checkNotNull(messageType, "messageType");
    checkNotNull(schema, "schema");
    return schemaCache.putIfAbsent(messageType, schema);
  }

  /**
   * Visible for testing only. Registers the given schema for the message type. If a schema was
   * previously registered, it will be replaced by the provided schema.
   *
   * @param messageType the type of message on which the schema operates.
   * @param schema the schema for the message type.
   * @return the previously registered schema, or {@code null} if no schema was registered
   *     previously.
   */
  @CanIgnoreReturnValue
  public Schema<?> registerSchemaOverride(Class<?> messageType, Schema<?> schema) {
    checkNotNull(messageType, "messageType");
    checkNotNull(schema, "schema");
    return schemaCache.put(messageType, schema);
  }

  private Protobuf() {
    schemaFactory = new ManifestSchemaFactory();
  }

  int getTotalSchemaSize() {
    int result = 0;
    for (Schema<?> schema : schemaCache.values()) {
      if (schema instanceof MessageSchema) {
        result += ((MessageSchema) schema).getSchemaSize();
      }
    }
    return result;
  }
}
