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
  // Whether to check if ClassValue is available and use it if so.
  private static final boolean TRY_CLASS_VALUE_CACHE = true;
  private static final Protobuf INSTANCE = new Protobuf();

  private final SchemaFactory schemaFactory;
  private final ConcurrentMap<Class<?>, Schema<?>> schemaCache;
  private final ClassValueSchemaCache classValueSchemaCache;

  /** Gets the singleton instance of the Protobuf runtime. */
  static Protobuf getInstance() {
    return INSTANCE;
  }

  /** Writes the given message to the target {@link Writer}. */
  <T> void writeTo(T message, Writer writer) throws IOException {
    schemaFor(message).writeTo(message, writer);
  }

  /** Reads fields from the given {@link Reader} and merges them into the message. */
  <T> void mergeFrom(T message, Reader reader, ExtensionRegistryLite extensionRegistry)
      throws IOException {
    schemaFor(message).mergeFrom(message, reader, extensionRegistry);
  }

  /** Checks if all required fields are set. */
  <T> boolean isInitialized(T message) {
    return schemaFor(message).isInitialized(message);
  }

  /** Gets the schema for the given message type. */
  <T> Schema<T> schemaFor(Class<T> messageType) {
    checkNotNull(messageType, "messageType");
    if (classValueSchemaCache != null) {
      return classValueSchemaCache.get(messageType);
    } else {
      @SuppressWarnings("unchecked")
      Schema<T> schema = (Schema<T>) schemaCache.get(messageType);
      if (schema == null) {
        schema = schemaFactory.createSchema(messageType);
        checkNotNull(schema, "schema");
        @SuppressWarnings("unchecked")
        Schema<T> previous = (Schema<T>) schemaCache.putIfAbsent(messageType, schema);
        if (previous != null) {
          // A new schema was registered by another thread.
          schema = previous;
        }
      }
      return schema;
    }
  }

  /** Gets the schema for the given message. */
  @SuppressWarnings("unchecked")
  <T> Schema<T> schemaFor(T message) {
    return schemaFor((Class<T>) message.getClass());
  }

  private Protobuf() {
    boolean hasClassValue = false;
    try {
      Class.forName("java.lang.ClassValue");
      hasClassValue = true;
    } catch (ClassNotFoundException e) {
      // ClassValue is not available
    }

    schemaFactory = new ManifestSchemaFactory();
    if (TRY_CLASS_VALUE_CACHE && hasClassValue) {
      schemaCache = null;
      classValueSchemaCache = new ClassValueSchemaCache(schemaFactory);
    } else {
      schemaCache = new ConcurrentHashMap<>();
      classValueSchemaCache = null;
    }
  }
}
