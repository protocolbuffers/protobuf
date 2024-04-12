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

  // TODO(nathanmittler): Consider using ClassValue instead.
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
