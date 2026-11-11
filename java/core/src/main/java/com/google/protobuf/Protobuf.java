// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

package com.google.protobuf;

import static com.google.protobuf.Internal.checkNotNull;

import java.io.IOException;

/**
 * This class is for Lite runtime use only.
 *
 * <p>For details on what this means regarding performance and security characteristics, see {@link
 * ForLiteOnly}.
 */
@ExperimentalApi
@CheckReturnValue
@ForLiteOnly
final class Protobuf {
  private static final Protobuf INSTANCE = new Protobuf();

  private final ManifestSchemaFactory schemaFactory;

  /**
   * Schemas keyed by their message type's default instance.
   *
   * <p>The default instance, not the {@link Class}, identifies a message type: a code shrinker may
   * merge several generated message classes into one, after which only the default instance tells
   * the types apart.
   */
  private final IdentityCache<Schema<?>> schemaCache = new IdentityCache<Schema<?>>();

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

  /**
   * Gets the schema for the given message type.
   *
   * <p>Prefer {@link #schemaFor(GeneratedMessageLite)} or {@link #schemaForDefaultInstance}: a
   * {@link Class} only identifies a message type if no code shrinker merged it with other types.
   */
  <T extends GeneratedMessageLite<?, ?>> Schema<T> schemaFor(Class<T> messageType) {
    return schemaForDefaultInstance(GeneratedMessageLite.getDefaultInstance(messageType));
  }

  /**
   * Gets the schema for the given message.
   *
   * <p>Not to be inlined: optimizers inline this lookup's callers (such as {@link
   * GeneratedMessageLite#hashCode()}) at thousands of call sites, and the lookup itself would
   * otherwise be copied into each of them.
   */
  @SuppressWarnings("unchecked")
  @DoNotInline
  <T extends GeneratedMessageLite<?, ?>> Schema<T> schemaFor(T message) {
    return schemaForDefaultInstance((T) message.getDefaultInstanceForType());
  }

  /** Gets the schema of the message type whose default instance is {@code defaultInstance}. */
  @SuppressWarnings("unchecked")
  <T extends GeneratedMessageLite<?, ?>> Schema<T> schemaForDefaultInstance(T defaultInstance) {
    Object schema = schemaCache.get(defaultInstance);
    if (schema == null) {
      return registerSchema(defaultInstance);
    }
    return (Schema<T>) schema;
  }

  @DoNotInline
  private <T extends GeneratedMessageLite<?, ?>> Schema<T> registerSchema(T defaultInstance) {
    Schema<T> schema = schemaFactory.createSchemaForDefaultInstance(defaultInstance);
    checkNotNull(schema, "schema");
    @SuppressWarnings("unchecked")
    Schema<T> registered = (Schema<T>) schemaCache.putIfAbsent(defaultInstance, schema);
    return registered;
  }

  private Protobuf() {
    schemaFactory = new ManifestSchemaFactory();
  }

  /**
   * A map keyed by object identity, for caches keyed by default instance. Entries are never
   * removed.
   *
   * <p>Unlike a {@link java.util.concurrent.ConcurrentHashMap}, this never calls the keys' {@code
   * hashCode()} or {@code equals()}, which for messages need the schema. Lookups take no lock.
   */
  private static final class IdentityCache<V> {
    /** Open addressing with linear probing; keys at even indices, their values right after. */
    private volatile Object[] table = new Object[2 * 64];

    private int size;

    /**
     * Returns the value for {@code key}, or null if there is none. May also return null while
     * another thread adds {@code key}; callers then fall back to {@link #putIfAbsent}.
     */
    Object get(Object key) {
      Object[] table = this.table;
      int mask = table.length - 2;
      for (int i = (System.identityHashCode(key) << 1) & mask; ; i = (i + 2) & mask) {
        Object candidate = table[i];
        if (candidate == key) {
          return table[i + 1];
        }
        if (candidate == null) {
          return null;
        }
      }
    }

    /** Maps {@code key} to {@code value} unless it is mapped already; returns the mapped value. */
    synchronized V putIfAbsent(Object key, V value) {
      @SuppressWarnings("unchecked")
      V existing = (V) get(key);
      if (existing != null) {
        return existing;
      }
      Object[] table = this.table;
      if (4 * (size + 1) > table.length) {
        // Keep the load factor at most 1/2. Readers may still be probing the old table, so fill in
        // a new one and publish it once complete.
        Object[] grown = new Object[2 * table.length];
        for (int i = 0; i < table.length; i += 2) {
          if (table[i] != null) {
            insert(grown, table[i], table[i + 1]);
          }
        }
        insert(grown, key, value);
        this.table = grown;
      } else {
        // Readers racing with this may see the key without its value; see get().
        insert(table, key, value);
      }
      size++;
      return value;
    }

    private static void insert(Object[] table, Object key, Object value) {
      int mask = table.length - 2;
      int i = (System.identityHashCode(key) << 1) & mask;
      while (table[i] != null) {
        i = (i + 2) & mask;
      }
      table[i + 1] = value;
      table[i] = key;
    }
  }
}
