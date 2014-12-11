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

import java.util.Arrays;
import java.util.Collections;
import java.util.HashMap;
import java.util.Map;

/**
 * Internal representation of map fields in generated lite-runtime messages.
 * 
 * This class is a protobuf implementation detail. Users shouldn't use this
 * class directly.
 */
public class MapFieldLite<K, V> {
  private Map<K, V> mapData;
  
  private MapFieldLite(Map<K, V> mapData) {
    this.mapData = mapData;
  }
  
  @SuppressWarnings({"rawtypes", "unchecked"})
  private static final MapFieldLite EMPTY_MAP_FIELD =
      new MapFieldLite(Collections.emptyMap());
  
  /** Returns an singleton immutable empty MapFieldLite instance. */
  @SuppressWarnings({"unchecked", "cast"})
  public static <K, V> MapFieldLite<K, V> emptyMapField() {
    return (MapFieldLite<K, V>) EMPTY_MAP_FIELD;
  }
  
  /** Creates a new MapFieldLite instance. */
  public static <K, V> MapFieldLite<K, V> newMapField() {
    return new MapFieldLite<K, V>(new HashMap<K, V>());
  }
  
  /** Gets the content of this MapField as a read-only Map. */
  public Map<K, V> getMap() {
    return Collections.unmodifiableMap(mapData);
  }
  
  /** Gets a mutable Map view of this MapField. */
  public Map<K, V> getMutableMap() {
    return mapData;
  }
  
  public void mergeFrom(MapFieldLite<K, V> other) {
    mapData.putAll(copy(other.mapData));
  }
  
  public void clear() {
    mapData.clear();
  }
  
  private static boolean equals(Object a, Object b) {
    if (a instanceof byte[] && b instanceof byte[]) {
      return Arrays.equals((byte[]) a, (byte[]) b);
    }
    return a.equals(b);
  }
  
  /**
   * Checks whether two {@link Map}s are equal. We don't use the default equals
   * method of {@link Map} because it compares by identity not by content for
   * byte arrays.
   */
  static <K, V> boolean equals(Map<K, V> a, Map<K, V> b) {
    if (a == b) {
      return true;
    }
    if (a.size() != b.size()) {
      return false;
    }
    for (Map.Entry<K, V> entry : a.entrySet()) {
      if (!b.containsKey(entry.getKey())) {
        return false;
      }
      if (!equals(entry.getValue(), b.get(entry.getKey()))) {
        return false;
      }
    }
    return true;
  }
  
  /**
   * Checks whether two map fields are equal.
   */
  @SuppressWarnings("unchecked")
  @Override
  public boolean equals(Object object) {
    if (!(object instanceof MapFieldLite)) {
      return false;
    }
    MapFieldLite<K, V> other = (MapFieldLite<K, V>) object;
    return equals(mapData, other.mapData);
  }
  
  private static int calculateHashCodeForObject(Object a) {
    if (a instanceof byte[]) {
      return LiteralByteString.hashCode((byte[]) a);
    }
    if (a instanceof Internal.EnumLite) {
      return Internal.hashEnum((Internal.EnumLite) a);
    }
    return a.hashCode();
  }

  /**
   * Calculates the hash code for a {@link Map}. We don't use the default hash
   * code method of {@link Map} because for byte arrays and protobuf enums it
   * use {@link Object#hashCode()}.
   */
  static <K, V> int calculateHashCodeForMap(Map<K, V> a) {
    int result = 0;
    for (Map.Entry<K, V> entry : a.entrySet()) {
      result += calculateHashCodeForObject(entry.getKey())
          ^ calculateHashCodeForObject(entry.getValue());
    }
    return result;    
  }
  
  @Override
  public int hashCode() {
    return calculateHashCodeForMap(mapData);
  }
  
  private static Object copy(Object object) {
    if (object instanceof byte[]) {
      byte[] data = (byte[]) object;
      return Arrays.copyOf(data, data.length);
    }
    return object;
  }
  
  /**
   * Makes a deep copy of a {@link Map}. Immutable objects in the map will be
   * shared (e.g., integers, strings, immutable messages) and mutable ones will
   * have a copy (e.g., byte arrays, mutable messages).
   */
  @SuppressWarnings("unchecked")
  static <K, V> Map<K, V> copy(Map<K, V> map) {
    Map<K, V> result = new HashMap<K, V>();
    for (Map.Entry<K, V> entry : map.entrySet()) {
      result.put(entry.getKey(), (V) copy(entry.getValue()));
    }
    return result;
  }
  
  /** Returns a deep copy of this map field. */
  public MapFieldLite<K, V> copy() {
    return new MapFieldLite<K, V>(copy(mapData));
  }
}
