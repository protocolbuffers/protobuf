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

import com.google.protobuf.MapFieldLite.MutatabilityAwareMap;

import java.util.ArrayList;
import java.util.Collections;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;

/**
 * Internal representation of map fields in generated messages.
 * 
 * This class supports accessing the map field as a {@link Map} to be used in
 * generated API and also supports accessing the field as a {@link List} to be
 * used in reflection API. It keeps track of where the data is currently stored
 * and do necessary conversions between map and list.  
 * 
 * This class is a protobuf implementation detail. Users shouldn't use this
 * class directly.
 * 
 * THREAD-SAFETY NOTE: Read-only access is thread-safe. Users can call getMap()
 * and getList() concurrently in multiple threads. If write-access is needed,
 * all access must be synchronized.
 */
public class MapField<K, V> implements MutabilityOracle {
  /**
   * Indicates where the data of this map field is currently stored.
   * 
   * MAP: Data is stored in mapData.
   * LIST: Data is stored in listData.
   * BOTH: mapData and listData have the same data.
   *
   * When the map field is accessed (through generated API or reflection API),
   * it will shift between these 3 modes:
   * 
   *          getMap()   getList()   getMutableMap()   getMutableList()
   *   MAP      MAP        BOTH          MAP               LIST
   *   LIST     BOTH       LIST          MAP               LIST
   *   BOTH     BOTH       BOTH          MAP               LIST
   *   
   * As the map field changes its mode, the list/map reference returned in a
   * previous method call may be invalidated. 
   */
  private enum StorageMode {MAP, LIST, BOTH}

  private volatile boolean isMutable;
  private volatile StorageMode mode;
  private MutatabilityAwareMap<K, V> mapData;
  private List<Message> listData;
  
  // Convert between a map entry Message and a key-value pair.
  private static interface Converter<K, V> {
    Message convertKeyAndValueToMessage(K key, V value);
    void convertMessageToKeyAndValue(Message message, Map<K, V> map);
    
    Message getMessageDefaultInstance();
  }
  
  private static class ImmutableMessageConverter<K, V> implements Converter<K, V> {
    private final MapEntry<K, V> defaultEntry;
    public ImmutableMessageConverter(MapEntry<K, V> defaultEntry) {
      this.defaultEntry = defaultEntry;
    }
    
    @Override
    public Message convertKeyAndValueToMessage(K key, V value) {
      return defaultEntry.newBuilderForType().setKey(key).setValue(value).buildPartial();
    }
    
    @Override
    public void convertMessageToKeyAndValue(Message message, Map<K, V> map) {
      MapEntry<K, V> entry = (MapEntry<K, V>) message;
      map.put(entry.getKey(), entry.getValue());
    }

    @Override
    public Message getMessageDefaultInstance() {
      return defaultEntry;
    }
  }
  

  private final Converter<K, V> converter;
  
  private MapField(
      Converter<K, V> converter,
      StorageMode mode,
      Map<K, V> mapData) {
    this.converter = converter;
    this.isMutable = true;
    this.mode = mode;
    this.mapData = new MutatabilityAwareMap<K, V>(this, mapData);
    this.listData = null;
  }
    
  private MapField(
      MapEntry<K, V> defaultEntry,
      StorageMode mode,
      Map<K, V> mapData) {
    this(new ImmutableMessageConverter<K, V>(defaultEntry), mode, mapData);
  }
  
  
  /** Returns an immutable empty MapField. */
  public static <K, V> MapField<K, V> emptyMapField(
      MapEntry<K, V> defaultEntry) {
    return new MapField<K, V>(
        defaultEntry, StorageMode.MAP, Collections.<K, V>emptyMap());
  }
  
  
  /** Creates a new mutable empty MapField. */
  public static <K, V> MapField<K, V> newMapField(MapEntry<K, V> defaultEntry) {
    return new MapField<K, V>(
        defaultEntry, StorageMode.MAP, new LinkedHashMap<K, V>());
  }
  
  
  private Message convertKeyAndValueToMessage(K key, V value) {
    return converter.convertKeyAndValueToMessage(key, value);
  }
  
  @SuppressWarnings("unchecked")
  private void convertMessageToKeyAndValue(Message message, Map<K, V> map) {
    converter.convertMessageToKeyAndValue(message, map);
  }

  private List<Message> convertMapToList(MutatabilityAwareMap<K, V> mapData) {
    List<Message> listData = new ArrayList<Message>();
    for (Map.Entry<K, V> entry : mapData.entrySet()) {
      listData.add(
          convertKeyAndValueToMessage(
              entry.getKey(), entry.getValue()));
    }
    return listData;
  }

  private MutatabilityAwareMap<K, V> convertListToMap(List<Message> listData) {
    Map<K, V> mapData = new LinkedHashMap<K, V>();
    for (Message item : listData) {
      convertMessageToKeyAndValue(item, mapData);
    }
    return new MutatabilityAwareMap<K, V>(this, mapData);
  }
  
  /** Returns the content of this MapField as a read-only Map. */
  public Map<K, V> getMap() {
    if (mode == StorageMode.LIST) {
      synchronized (this) {
        if (mode == StorageMode.LIST) {
          mapData = convertListToMap(listData);
          mode = StorageMode.BOTH;
        }
      }
    }
    return Collections.unmodifiableMap(mapData);
  }
  
  /** Gets a mutable Map view of this MapField. */
  public Map<K, V> getMutableMap() {
    if (mode != StorageMode.MAP) {
      if (mode == StorageMode.LIST) {
        mapData = convertListToMap(listData);
      }
      listData = null;
      mode = StorageMode.MAP; 
    }
    return mapData;
  }
  
  public void mergeFrom(MapField<K, V> other) {
    getMutableMap().putAll(MapFieldLite.copy(other.getMap()));
  }
  
  public void clear() {
    mapData = new MutatabilityAwareMap<K, V>(this, new LinkedHashMap<K, V>());
    mode = StorageMode.MAP;
  }
  
  @SuppressWarnings("unchecked")
  @Override
  public boolean equals(Object object) {
    if (!(object instanceof MapField)) {
      return false;
    }
    MapField<K, V> other = (MapField<K, V>) object;
    return MapFieldLite.<K, V>equals(getMap(), other.getMap());
  }
  
  @Override
  public int hashCode() {
    return MapFieldLite.<K, V>calculateHashCodeForMap(getMap());
  }
  
  /** Returns a deep copy of this MapField. */
  public MapField<K, V> copy() {
    return new MapField<K, V>(
        converter, StorageMode.MAP, MapFieldLite.copy(getMap()));
  }
  
  /** Gets the content of this MapField as a read-only List. */
  List<Message> getList() {
    if (mode == StorageMode.MAP) {
      synchronized (this) {
        if (mode == StorageMode.MAP) {
          listData = convertMapToList(mapData);
          mode = StorageMode.BOTH;
        }
      }
    }
    return Collections.unmodifiableList(listData);
  }
  
  /** Gets a mutable List view of this MapField. */
  List<Message> getMutableList() {
    if (mode != StorageMode.LIST) {
      if (mode == StorageMode.MAP) {
        listData = convertMapToList(mapData);
      }
      mapData = null;
      mode = StorageMode.LIST;
    }
    return listData;
  }
  
  /**
   * Gets the default instance of the message stored in the list view of this
   * map field.
   */
  Message getMapEntryMessageDefaultInstance() {
    return converter.getMessageDefaultInstance();
  }
  
  /**
   * Makes this list immutable. All subsequent modifications will throw an
   * {@link UnsupportedOperationException}.
   */
  public void makeImmutable() {
    isMutable = false;
  }
  
  /**
   * Returns whether this field can be modified.
   */
  public boolean isMutable() {
    return isMutable;
  }
  
  /* (non-Javadoc)
   * @see com.google.protobuf.MutabilityOracle#ensureMutable()
   */
  @Override
  public void ensureMutable() {
    if (!isMutable()) {
      throw new UnsupportedOperationException();
    }
  }
}
