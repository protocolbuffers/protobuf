// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

package com.google.protobuf;

import static com.google.protobuf.Internal.checkNotNull;

import com.google.protobuf.Internal.EnumLite;
import java.util.Arrays;
import java.util.Collections;
import java.util.LinkedHashMap;
import java.util.Map;
import java.util.Set;

/**
 * Internal representation of map fields in generated lite-runtime messages.
 *
 * <p>This class is a protobuf implementation detail. Users shouldn't use this class directly.
 */
public final class MapFieldLite<K, V> extends LinkedHashMap<K, V> {

  private boolean isMutable;

  private MapFieldLite() {
    this.isMutable = true;
  }

  private MapFieldLite(Map<K, V> mapData) {
    super(mapData);
    this.isMutable = true;
  }

  private static final MapFieldLite<?, ?> EMPTY_MAP_FIELD = new MapFieldLite<>();

  static {
    EMPTY_MAP_FIELD.makeImmutable();
  }

  /** Returns a singleton immutable empty MapFieldLite instance. */
  @SuppressWarnings("unchecked")
  public static <K, V> MapFieldLite<K, V> emptyMapField() {
    return (MapFieldLite<K, V>) EMPTY_MAP_FIELD;
  }

  public void mergeFrom(MapFieldLite<K, V> other) {
    ensureMutable();
    if (!other.isEmpty()) {
      putAll(other);
    }
  }

  @Override
  public Set<Map.Entry<K, V>> entrySet() {
    return isEmpty() ? Collections.<Map.Entry<K, V>>emptySet() : super.entrySet();
  }

  @Override
  public void clear() {
    ensureMutable();
    super.clear();
  }

  @Override
  public V put(K key, V value) {
    ensureMutable();
    checkNotNull(key);

    checkNotNull(value);
    return super.put(key, value);
  }

  public V put(Map.Entry<K, V> entry) {
    return put(entry.getKey(), entry.getValue());
  }

  @Override
  public void putAll(Map<? extends K, ? extends V> m) {
    ensureMutable();
    checkForNullKeysAndValues(m);
    super.putAll(m);
  }

  @Override
  public V remove(Object key) {
    ensureMutable();
    return super.remove(key);
  }

  private static void checkForNullKeysAndValues(Map<?, ?> m) {
    for (Object key : m.keySet()) {
      checkNotNull(key);
      checkNotNull(m.get(key));
    }
  }

  private static boolean equals(
          Object a,
      Object b) {
    if (a instanceof byte[] && b instanceof byte[]) {
      return Arrays.equals((byte[]) a, (byte[]) b);
    }
    return a.equals(b);
  }

  /**
   * Checks whether two {@link Map}s are equal. We don't use the default equals method of {@link
   * Map} because it compares by identity not by content for byte arrays.
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

  /** Checks whether two map fields are equal. */
  @SuppressWarnings("unchecked")
  @Override
  public boolean equals(
          Object object) {
    return (object instanceof Map) && equals(this, (Map<K, V>) object);
  }

  private static int calculateHashCodeForObject(Object a) {
    if (a instanceof byte[]) {
      return Internal.hashCode((byte[]) a);
    }
    // Enums should be stored as integers internally.
    if (a instanceof EnumLite) {
      throw new UnsupportedOperationException();
    }
    return a.hashCode();
  }

  /**
   * Calculates the hash code for a {@link Map}. We don't use the default hash code method of {@link
   * Map} because for byte arrays and protobuf enums it use {@link Object#hashCode()}.
   */
  static <K, V> int calculateHashCodeForMap(Map<K, V> a) {
    int result = 0;
    for (Map.Entry<K, V> entry : a.entrySet()) {
      result +=
          calculateHashCodeForObject(entry.getKey()) ^ calculateHashCodeForObject(entry.getValue());
    }
    return result;
  }

  @Override
  public int hashCode() {
    return calculateHashCodeForMap(this);
  }

  private static Object copy(Object object) {
    if (object instanceof byte[]) {
      byte[] data = (byte[]) object;
      return Arrays.copyOf(data, data.length);
    }
    return object;
  }

  /**
   * Makes a deep copy of a {@link Map}. Immutable objects in the map will be shared (e.g.,
   * integers, strings, immutable messages) and mutable ones will have a copy (e.g., byte arrays,
   * mutable messages).
   */
  @SuppressWarnings("unchecked")
  static <K, V> Map<K, V> copy(Map<K, V> map) {
    Map<K, V> result = new LinkedHashMap<K, V>(map.size() * 4 / 3 + 1);
    for (Map.Entry<K, V> entry : map.entrySet()) {
      result.put(entry.getKey(), (V) copy(entry.getValue()));
    }
    return result;
  }

  /** Returns a deep copy of this map field. */
  public MapFieldLite<K, V> mutableCopy() {
    return isEmpty() ? new MapFieldLite<K, V>() : new MapFieldLite<K, V>(this);
  }

  /**
   * Makes this field immutable. All subsequent modifications will throw an {@link
   * UnsupportedOperationException}.
   */
  public void makeImmutable() {
    isMutable = false;
  }

  /** Returns whether this field can be modified. */
  public boolean isMutable() {
    return isMutable;
  }

  private void ensureMutable() {
    if (!isMutable()) {
      throw new UnsupportedOperationException();
    }
  }
}
