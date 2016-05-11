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

import com.google.protobuf.Descriptors.Descriptor;
import com.google.protobuf.Descriptors.EnumValueDescriptor;
import com.google.protobuf.Descriptors.FieldDescriptor;

import java.io.IOException;
import java.util.Collections;
import java.util.Map;
import java.util.TreeMap;

/**
 * Implements MapEntry messages.
 * 
 * In reflection API, map fields will be treated as repeated message fields and
 * each map entry is accessed as a message. This MapEntry class is used to
 * represent these map entry messages in reflection API.
 * 
 * Protobuf internal. Users shouldn't use this class.
 */
public final class MapEntry<K, V> extends AbstractMessage {
  private static class Metadata<K, V> {
    public final Descriptor descriptor;  
    public final MapEntry<K, V> defaultInstance;
    public final AbstractParser<MapEntry<K, V>> parser;
    
    public Metadata(
        final Descriptor descriptor, final MapEntry<K, V> defaultInstance) {
      this.descriptor = descriptor;
      this.defaultInstance = defaultInstance;
      final Metadata<K, V> thisMetadata = this;
      this.parser = new AbstractParser<MapEntry<K, V>>() {
        private final Parser<MapEntryLite<K, V>> dataParser =
            defaultInstance.data.getParserForType();
        @Override
        public MapEntry<K, V> parsePartialFrom(
            CodedInputStream input, ExtensionRegistryLite extensionRegistry)
            throws InvalidProtocolBufferException {
          MapEntryLite<K, V> data =
              dataParser.parsePartialFrom(input, extensionRegistry);
          return new MapEntry<K, V>(thisMetadata, data);
        }
        
      };
    }
  }
  
  private final Metadata<K, V> metadata;
  private final MapEntryLite<K, V> data;
  
  /** Create a default MapEntry instance. */
  private MapEntry(Descriptor descriptor,
      WireFormat.FieldType keyType, K defaultKey,
      WireFormat.FieldType valueType, V defaultValue) {
    this.data = MapEntryLite.newDefaultInstance(
        keyType, defaultKey, valueType, defaultValue);
    this.metadata = new Metadata<K, V>(descriptor, this); 
  }
  
  /** Create a new MapEntry message. */
  private MapEntry(Metadata<K, V> metadata, MapEntryLite<K, V> data) {
    this.metadata = metadata;
    this.data = data;
  }
  
  /**
   * Create a default MapEntry instance. A default MapEntry instance should be
   * created only once for each map entry message type. Generated code should
   * store the created default instance and use it later to create new MapEntry
   * messages of the same type. 
   */
  public static <K, V> MapEntry<K, V> newDefaultInstance(
      Descriptor descriptor,
      WireFormat.FieldType keyType, K defaultKey,
      WireFormat.FieldType valueType, V defaultValue) {
    return new MapEntry<K, V>(
        descriptor, keyType, defaultKey, valueType, defaultValue);
  }
  
  public K getKey() {
    return data.getKey();
  }
  
  public V getValue() {
    return data.getValue();
  }
  
  @Override
  public int getSerializedSize() {
    return data.getSerializedSize();
  }
  
  @Override
  public void writeTo(CodedOutputStream output) throws IOException {
    data.writeTo(output);
  }
  
  @Override
  public boolean isInitialized() {
    return data.isInitialized();
  }
  
  @Override
  public Parser<MapEntry<K, V>> getParserForType() {
    return metadata.parser;
  }

  @Override
  public Builder<K, V> newBuilderForType() {
    return new Builder<K, V>(metadata);
  }
  
  @Override
  public Builder<K, V> toBuilder() {
    return new Builder<K, V>(metadata, data);
  }

  @Override
  public MapEntry<K, V> getDefaultInstanceForType() {
    return metadata.defaultInstance;
  }

  @Override
  public Descriptor getDescriptorForType() {
    return metadata.descriptor;
  }

  @Override
  public Map<FieldDescriptor, Object> getAllFields() {
    final TreeMap<FieldDescriptor, Object> result =
        new TreeMap<FieldDescriptor, Object>();
    for (final FieldDescriptor field : metadata.descriptor.getFields()) {
      if (hasField(field)) {
        result.put(field, getField(field));
      }
    }
    return Collections.unmodifiableMap(result);
  }
  
  private void checkFieldDescriptor(FieldDescriptor field) {
    if (field.getContainingType() != metadata.descriptor) {
      throw new RuntimeException(
          "Wrong FieldDescriptor \"" + field.getFullName()
          + "\" used in message \"" + metadata.descriptor.getFullName()); 
    }
  }

  @Override
  public boolean hasField(FieldDescriptor field) {
    checkFieldDescriptor(field);;
    // A MapEntry always contains two fields.
    return true;
  }

  @Override
  public Object getField(FieldDescriptor field) {
    checkFieldDescriptor(field);
    Object result = field.getNumber() == 1 ? getKey() : getValue();
    // Convert enums to EnumValueDescriptor.
    if (field.getType() == FieldDescriptor.Type.ENUM) {
      result = field.getEnumType().findValueByNumberCreatingIfUnknown(
          (java.lang.Integer) result);
    }
    return result;
  }

  @Override
  public int getRepeatedFieldCount(FieldDescriptor field) {
    throw new RuntimeException(
        "There is no repeated field in a map entry message.");
  }

  @Override
  public Object getRepeatedField(FieldDescriptor field, int index) {
    throw new RuntimeException(
        "There is no repeated field in a map entry message.");
  }

  @Override
  public UnknownFieldSet getUnknownFields() {
    return UnknownFieldSet.getDefaultInstance();
  }

  /**
   * Builder to create {@link MapEntry} messages.
   */
  public static class Builder<K, V>
      extends AbstractMessage.Builder<Builder<K, V>> {
    private final Metadata<K, V> metadata;
    private MapEntryLite<K, V> data;
    private MapEntryLite.Builder<K, V> dataBuilder;
    
    private Builder(Metadata<K, V> metadata) {
      this.metadata = metadata;
      this.data = metadata.defaultInstance.data;
      this.dataBuilder = null;
    }
    
    private Builder(Metadata<K, V> metadata, MapEntryLite<K, V> data) {
      this.metadata = metadata;
      this.data = data;
      this.dataBuilder = null;
    }
    
    public K getKey() {
      return dataBuilder == null ? data.getKey() : dataBuilder.getKey();
    }
    
    public V getValue() {
      return dataBuilder == null ? data.getValue() : dataBuilder.getValue();
    }
    
    private void ensureMutable() {
      if (dataBuilder == null) {
        dataBuilder = data.toBuilder();
      }
    }
    
    public Builder<K, V> setKey(K key) {
      ensureMutable();
      dataBuilder.setKey(key);
      return this;
    }
    
    public Builder<K, V> clearKey() {
      ensureMutable();
      dataBuilder.clearKey();
      return this;
    }
    
    public Builder<K, V> setValue(V value) {
      ensureMutable();
      dataBuilder.setValue(value);
      return this;
    }
    
    public Builder<K, V> clearValue() {
      ensureMutable();
      dataBuilder.clearValue();
      return this;
    }

    @Override
    public MapEntry<K, V> build() {
      MapEntry<K, V> result = buildPartial();
      if (!result.isInitialized()) {
        throw newUninitializedMessageException(result);
      }
      return result;
    }

    @Override
    public MapEntry<K, V> buildPartial() {
      if (dataBuilder != null) {
        data = dataBuilder.buildPartial();
        dataBuilder = null;
      }
      return new MapEntry<K, V>(metadata, data);
    }

    @Override
    public Descriptor getDescriptorForType() {
      return metadata.descriptor;
    }
    
    private void checkFieldDescriptor(FieldDescriptor field) {
      if (field.getContainingType() != metadata.descriptor) {
        throw new RuntimeException(
            "Wrong FieldDescriptor \"" + field.getFullName()
            + "\" used in message \"" + metadata.descriptor.getFullName()); 
      }
    }

    @Override
    public com.google.protobuf.Message.Builder newBuilderForField(
        FieldDescriptor field) {
      checkFieldDescriptor(field);;
      // This method should be called for message fields and in a MapEntry
      // message only the value field can possibly be a message field.
      if (field.getNumber() != 2
          || field.getJavaType() != FieldDescriptor.JavaType.MESSAGE) {
        throw new RuntimeException(
            "\"" + field.getFullName() + "\" is not a message value field.");
      }
      return ((Message) data.getValue()).newBuilderForType();
    }

    @SuppressWarnings("unchecked")
    @Override
    public Builder<K, V> setField(FieldDescriptor field, Object value) {
      checkFieldDescriptor(field);
      if (field.getNumber() == 1) {
        setKey((K) value);
      } else {
        if (field.getType() == FieldDescriptor.Type.ENUM) {
          value = ((EnumValueDescriptor) value).getNumber();
        }
        setValue((V) value);
      }
      return this;
    }

    @Override
    public Builder<K, V> clearField(FieldDescriptor field) {
      checkFieldDescriptor(field);
      if (field.getNumber() == 1) {
        clearKey();
      } else {
        clearValue();
      }
      return this;
    }

    @Override
    public Builder<K, V> setRepeatedField(FieldDescriptor field, int index,
        Object value) {
      throw new RuntimeException(
          "There is no repeated field in a map entry message.");
    }

    @Override
    public Builder<K, V> addRepeatedField(FieldDescriptor field, Object value) {
      throw new RuntimeException(
          "There is no repeated field in a map entry message.");
    }

    @Override
    public Builder<K, V> setUnknownFields(UnknownFieldSet unknownFields) {
      // Unknown fields are discarded for MapEntry message.
      return this;
    }

    @Override
    public MapEntry<K, V> getDefaultInstanceForType() {
      return metadata.defaultInstance;
    }

    @Override
    public boolean isInitialized() {
      if (dataBuilder != null) {
        return dataBuilder.isInitialized();
      } else {
        return data.isInitialized();
      }
    }

    @Override
    public Map<FieldDescriptor, Object> getAllFields() {
      final TreeMap<FieldDescriptor, Object> result =
          new TreeMap<FieldDescriptor, Object>();
      for (final FieldDescriptor field : metadata.descriptor.getFields()) {
        if (hasField(field)) {
          result.put(field, getField(field));
        }
      }
      return Collections.unmodifiableMap(result);
    }

    @Override
    public boolean hasField(FieldDescriptor field) {
      checkFieldDescriptor(field);
      return true;
    }

    @Override
    public Object getField(FieldDescriptor field) {
      checkFieldDescriptor(field);
      Object result = field.getNumber() == 1 ? getKey() : getValue();
      // Convert enums to EnumValueDescriptor.
      if (field.getType() == FieldDescriptor.Type.ENUM) {
        result = field.getEnumType().findValueByNumberCreatingIfUnknown(
            (java.lang.Integer) result);
      }
      return result;
    }

    @Override
    public int getRepeatedFieldCount(FieldDescriptor field) {
      throw new RuntimeException(
          "There is no repeated field in a map entry message.");
    }
    
    @Override
    public Object getRepeatedField(FieldDescriptor field, int index) {
      throw new RuntimeException(
          "There is no repeated field in a map entry message.");
    }
    
    @Override
    public UnknownFieldSet getUnknownFields() {
      return UnknownFieldSet.getDefaultInstance();
    }

    @Override
    public Builder<K, V> clone() {
      if (dataBuilder == null) {
        return new Builder<K, V>(metadata, data);
      } else {
        return new Builder<K, V>(metadata, dataBuilder.build());
      }
    }
  }
}
