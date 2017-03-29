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
import java.util.AbstractMap;
import java.util.Map;

/**
 * Implements the lite version of map entry messages.
 *
 * This class serves as an utility class to help do serialization/parsing of
 * map entries. It's used in generated code and also in the full version
 * MapEntry message.
 *
 * Protobuf internal. Users shouldn't use.
 */
public class MapEntryLite<K, V> {

  static class Metadata<K, V> {
    public final WireFormat.FieldType keyType;
    public final K defaultKey;
    public final WireFormat.FieldType valueType;
    public final V defaultValue;

    public Metadata(
        WireFormat.FieldType keyType, K defaultKey,
        WireFormat.FieldType valueType, V defaultValue) {
      this.keyType = keyType;
      this.defaultKey = defaultKey;
      this.valueType = valueType;
      this.defaultValue = defaultValue;
    }
  }

  private static final int KEY_FIELD_NUMBER = 1;
  private static final int VALUE_FIELD_NUMBER = 2;

  private final Metadata<K, V> metadata;
  private final K key;
  private final V value;

  /** Creates a default MapEntryLite message instance. */
  private MapEntryLite(
      WireFormat.FieldType keyType, K defaultKey,
      WireFormat.FieldType valueType, V defaultValue) {
    this.metadata = new Metadata<K, V>(keyType, defaultKey, valueType, defaultValue);
    this.key = defaultKey;
    this.value = defaultValue;
  }

  /** Creates a new MapEntryLite message. */
  private MapEntryLite(Metadata<K, V> metadata, K key, V value) {
    this.metadata = metadata;
    this.key = key;
    this.value = value;
  }

  public K getKey() {
    return key;
  }

  public V getValue() {
    return value;
  }

  /**
   * Creates a default MapEntryLite message instance.
   *
   * This method is used by generated code to create the default instance for
   * a map entry message. The created default instance should be used to create
   * new map entry messages of the same type. For each map entry message, only
   * one default instance should be created.
   */
  public static <K, V> MapEntryLite<K, V> newDefaultInstance(
      WireFormat.FieldType keyType, K defaultKey,
      WireFormat.FieldType valueType, V defaultValue) {
    return new MapEntryLite<K, V>(
        keyType, defaultKey, valueType, defaultValue);
  }

  static <K, V> void writeTo(CodedOutputStream output, Metadata<K, V> metadata, K key, V value)
      throws IOException {
    FieldSet.writeElement(output, metadata.keyType, KEY_FIELD_NUMBER, key);
    FieldSet.writeElement(output, metadata.valueType, VALUE_FIELD_NUMBER, value);
  }

  static <K, V> int computeSerializedSize(Metadata<K, V> metadata, K key, V value) {
    return FieldSet.computeElementSize(metadata.keyType, KEY_FIELD_NUMBER, key)
        + FieldSet.computeElementSize(metadata.valueType, VALUE_FIELD_NUMBER, value);
  }

  @SuppressWarnings("unchecked")
  static <T> T parseField(
      CodedInputStream input, ExtensionRegistryLite extensionRegistry,
      WireFormat.FieldType type, T value) throws IOException {
    switch (type) {
      case MESSAGE:
        MessageLite.Builder subBuilder = ((MessageLite) value).toBuilder();
        input.readMessage(subBuilder, extensionRegistry);
        return (T) subBuilder.buildPartial();
      case ENUM:
        return (T) (java.lang.Integer) input.readEnum();
      case GROUP:
        throw new RuntimeException("Groups are not allowed in maps.");
      default:
        return (T) FieldSet.readPrimitiveField(input, type, true);
    }
  }

  /**
   * Serializes the provided key and value as though they were wrapped by a {@link MapEntryLite}
   * to the output stream. This helper method avoids allocation of a {@link MapEntryLite}
   * built with a key and value and is called from generated code directly.
   */
  public void serializeTo(CodedOutputStream output, int fieldNumber, K key, V value)
      throws IOException {
    output.writeTag(fieldNumber, WireFormat.WIRETYPE_LENGTH_DELIMITED);
    output.writeUInt32NoTag(computeSerializedSize(metadata, key, value));
    writeTo(output, metadata, key, value);
  }

  /**
   * Computes the message size for the provided key and value as though they were wrapped
   * by a {@link MapEntryLite}. This helper method avoids allocation of a {@link MapEntryLite}
   * built with a key and value and is called from generated code directly.
   */
  public int computeMessageSize(int fieldNumber, K key, V value) {
    return CodedOutputStream.computeTagSize(fieldNumber)
        + CodedOutputStream.computeLengthDelimitedFieldSize(
            computeSerializedSize(metadata, key, value));
  }

  /**
   * Parses an entry off of the input as a {@link Map.Entry}. This helper requires an allocation
   * so using {@link #parseInto} is preferred if possible.
   */
  public Map.Entry<K, V> parseEntry(ByteString bytes, ExtensionRegistryLite extensionRegistry)
      throws IOException {
    return parseEntry(bytes.newCodedInput(), metadata, extensionRegistry);
  }

  static <K, V> Map.Entry<K, V> parseEntry(
      CodedInputStream input, Metadata<K, V> metadata, ExtensionRegistryLite extensionRegistry)
          throws IOException{
    K key = metadata.defaultKey;
    V value = metadata.defaultValue;
    while (true) {
      int tag = input.readTag();
      if (tag == 0) {
        break;
      }
      if (tag == WireFormat.makeTag(KEY_FIELD_NUMBER, metadata.keyType.getWireType())) {
        key = parseField(input, extensionRegistry, metadata.keyType, key);
      } else if (tag == WireFormat.makeTag(VALUE_FIELD_NUMBER, metadata.valueType.getWireType())) {
        value = parseField(input, extensionRegistry, metadata.valueType, value);
      } else {
        if (!input.skipField(tag)) {
          break;
        }
      }
    }
    return new AbstractMap.SimpleImmutableEntry<K, V>(key, value);
  }

  /**
   * Parses an entry off of the input into the map. This helper avoids allocaton of a
   * {@link MapEntryLite} by parsing directly into the provided {@link MapFieldLite}.
   */
  public void parseInto(
      MapFieldLite<K, V> map, CodedInputStream input, ExtensionRegistryLite extensionRegistry)
          throws IOException {
    int length = input.readRawVarint32();
    final int oldLimit = input.pushLimit(length);
    K key = metadata.defaultKey;
    V value = metadata.defaultValue;

    while (true) {
      int tag = input.readTag();
      if (tag == 0) {
        break;
      }
      if (tag == WireFormat.makeTag(KEY_FIELD_NUMBER, metadata.keyType.getWireType())) {
        key = parseField(input, extensionRegistry, metadata.keyType, key);
      } else if (tag == WireFormat.makeTag(VALUE_FIELD_NUMBER, metadata.valueType.getWireType())) {
        value = parseField(input, extensionRegistry, metadata.valueType, value);
      } else {
        if (!input.skipField(tag)) {
          break;
        }
      }
    }

    input.checkLastTagWas(0);
    input.popLimit(oldLimit);
    map.put(key, value);
  }

  /** For experimental runtime internal use only. */
  Metadata<K, V> getMetadata() {
    return metadata;
  }
}
