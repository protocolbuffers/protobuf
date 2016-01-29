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

/**
 * Implements the lite version of map entry messages.
 * 
 * This class serves as an utility class to help do serialization/parsing of
 * map entries. It's used in generated code and also in the full version
 * MapEntry message.
 * 
 * Protobuf internal. Users shouldn't use.
 */
public class MapEntryLite<K, V> extends AbstractMessageLite {
  private static class Metadata<K, V> {
    public final MapEntryLite<K, V> defaultInstance;
    public final WireFormat.FieldType keyType;
    public final WireFormat.FieldType valueType;
    public final Parser<MapEntryLite<K, V>> parser;
    public Metadata(
        MapEntryLite<K, V> defaultInstance,
        WireFormat.FieldType keyType,
        WireFormat.FieldType valueType) {
      this.defaultInstance = defaultInstance; 
      this.keyType = keyType;
      this.valueType = valueType;
      final Metadata<K, V> finalThis = this;
      this.parser = new AbstractParser<MapEntryLite<K, V>>() {
        @Override
        public MapEntryLite<K, V> parsePartialFrom(
            CodedInputStream input, ExtensionRegistryLite extensionRegistry)
            throws InvalidProtocolBufferException {
          return new MapEntryLite<K, V>(finalThis, input, extensionRegistry);
        }
      };
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
    this.metadata = new Metadata<K, V>(this, keyType, valueType);
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
  
  @Override
  public void writeTo(CodedOutputStream output) throws IOException {
    writeField(KEY_FIELD_NUMBER, metadata.keyType, key, output);
    writeField(VALUE_FIELD_NUMBER, metadata.valueType, value, output);
  }

  private void writeField(
      int number, WireFormat.FieldType type, Object value,
      CodedOutputStream output) throws IOException {
    output.writeTag(number, type.getWireType());
    FieldSet.writeElementNoTag(output, type, value);
  }

  private volatile int cachedSerializedSize = -1;
  @Override
  public int getSerializedSize() {
    if (cachedSerializedSize != -1) {
      return cachedSerializedSize;
    }
    int size = 0;
    size += getFieldSize(KEY_FIELD_NUMBER, metadata.keyType, key);
    size += getFieldSize(VALUE_FIELD_NUMBER, metadata.valueType, value);
    cachedSerializedSize = size;
    return size;
  }

  private int getFieldSize(
      int number, WireFormat.FieldType type, Object value) {
    return CodedOutputStream.computeTagSize(number)
        + FieldSet.computeElementSizeNoTag(type, value);
  }
  
  /** Parsing constructor. */
  private MapEntryLite(
      Metadata<K, V> metadata,
      CodedInputStream input,
      ExtensionRegistryLite extensionRegistry)
      throws InvalidProtocolBufferException {
    try {
      K key = metadata.defaultInstance.key;
      V value = metadata.defaultInstance.value;
      while (true) {
        int tag = input.readTag();
        if (tag == 0) {
          break;
        }
        if (tag == WireFormat.makeTag(
                KEY_FIELD_NUMBER, metadata.keyType.getWireType())) {
          key = mergeField(
              input, extensionRegistry, metadata.keyType, key);
        } else if (tag == WireFormat.makeTag(
                       VALUE_FIELD_NUMBER, metadata.valueType.getWireType())) {
          value = mergeField(
              input, extensionRegistry, metadata.valueType, value);
        } else {
          if (!input.skipField(tag)) {
            break;
          }
        }
      }
      this.metadata = metadata;
      this.key = key;
      this.value = value;
    } catch (InvalidProtocolBufferException e) {
      throw e.setUnfinishedMessage(this);
    } catch (IOException e) {
      throw new InvalidProtocolBufferException(e.getMessage())
          .setUnfinishedMessage(this);
    }
  }
  
  @SuppressWarnings("unchecked")
  private <T> T mergeField(
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

  @Override
  public Parser<MapEntryLite<K, V>> getParserForType() {
    return metadata.parser;
  }

  @Override
  public Builder<K, V> newBuilderForType() {
    return new Builder<K, V>(metadata);
  }

  @Override
  public Builder<K, V> toBuilder() {
    return new Builder<K, V>(metadata, key, value);
  }

  @Override
  public MapEntryLite<K, V> getDefaultInstanceForType() {
    return metadata.defaultInstance;
  }

  @Override
  public boolean isInitialized() {
    if (metadata.valueType.getJavaType() == WireFormat.JavaType.MESSAGE) {
      return ((MessageLite) value).isInitialized();
    }
    return true;
  }

  /**
   * Builder used to create {@link MapEntryLite} messages.
   */
  public static class Builder<K, V>
      extends AbstractMessageLite.Builder<Builder<K, V>> {
    private final Metadata<K, V> metadata;
    private K key;
    private V value;
    
    private Builder(Metadata<K, V> metadata) {
      this.metadata = metadata;
      this.key = metadata.defaultInstance.key;
      this.value = metadata.defaultInstance.value;
    }
    
    public K getKey() {
      return key;
    }
    
    public V getValue() {
      return value;
    }
    
    public Builder<K, V> setKey(K key) {
      this.key = key;
      return this;
    }
    
    public Builder<K, V> setValue(V value) {
      this.value = value;
      return this;
    }
    
    public Builder<K, V> clearKey() {
      this.key = metadata.defaultInstance.key;
      return this;
    }
    
    public Builder<K, V> clearValue() {
      this.value = metadata.defaultInstance.value;
      return this;
    }

    @Override
    public Builder<K, V> clear() {
      this.key = metadata.defaultInstance.key;
      this.value = metadata.defaultInstance.value;
      return this;
    }

    @Override
    public MapEntryLite<K, V> build() {
      MapEntryLite<K, V> result = buildPartial();
      if (!result.isInitialized()) {
        throw newUninitializedMessageException(result);
      }
      return result;
    }

    @Override
    public MapEntryLite<K, V> buildPartial() {
      return new MapEntryLite<K, V>(metadata, key, value);
    }

    @Override
    public MessageLite getDefaultInstanceForType() {
      return metadata.defaultInstance;
    }

    @Override
    public boolean isInitialized() {
      if (metadata.valueType.getJavaType() == WireFormat.JavaType.MESSAGE) {
        return ((MessageLite) value).isInitialized();
      }
      return true;
    }

    private Builder(Metadata<K, V> metadata, K key, V value) {
      this.metadata = metadata;
      this.key = key;
      this.value = value;
    }
    
    @Override
    public Builder<K, V> clone() {
      return new Builder<K, V>(metadata, key, value);
    }

    @Override
    public Builder<K, V> mergeFrom(
        CodedInputStream input, ExtensionRegistryLite extensionRegistry)
        throws IOException {
      MapEntryLite<K, V> entry =
          new MapEntryLite<K, V>(metadata, input, extensionRegistry);
      this.key = entry.key;
      this.value = entry.value;
      return this;
    }
  }
}
