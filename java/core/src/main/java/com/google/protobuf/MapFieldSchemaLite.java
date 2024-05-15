// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

package com.google.protobuf;

import com.google.protobuf.MapEntryLite.Metadata;
import java.util.Map;

@CheckReturnValue
final class MapFieldSchemaLite implements MapFieldSchema {

  @Override
  public Map<?, ?> forMutableMapData(Object mapField) {
    return (MapFieldLite<?, ?>) mapField;
  }

  @Override
  public Metadata<?, ?> forMapMetadata(Object mapDefaultEntry) {
    return ((MapEntryLite<?, ?>) mapDefaultEntry).getMetadata();
  }

  @Override
  public Map<?, ?> forMapData(Object mapField) {
    return (MapFieldLite<?, ?>) mapField;
  }

  @Override
  public boolean isImmutable(Object mapField) {
    return !((MapFieldLite<?, ?>) mapField).isMutable();
  }

  @Override
  public Object toImmutable(Object mapField) {
    ((MapFieldLite<?, ?>) mapField).makeImmutable();
    return mapField;
  }

  @Override
  public Object newMapField(Object unused) {
    return MapFieldLite.emptyMapField().mutableCopy();
  }

  @Override
  public Object mergeFrom(Object destMapField, Object srcMapField) {
    return mergeFromLite(destMapField, srcMapField);
  }

  @SuppressWarnings("unchecked")
  private static <K, V> MapFieldLite<K, V> mergeFromLite(Object destMapField, Object srcMapField) {
    MapFieldLite<K, V> mine = (MapFieldLite<K, V>) destMapField;
    MapFieldLite<K, V> other = (MapFieldLite<K, V>) srcMapField;
    if (!other.isEmpty()) {
      if (!mine.isMutable()) {
        mine = mine.mutableCopy();
      }
      mine.mergeFrom(other);
    }
    return mine;
  }

  @Override
  public int getSerializedSize(int fieldNumber, Object mapField, Object mapDefaultEntry) {
    return getSerializedSizeLite(fieldNumber, mapField, mapDefaultEntry);
  }

  @SuppressWarnings("unchecked")
  private static <K, V> int getSerializedSizeLite(
      int fieldNumber, Object mapField, Object defaultEntry) {
    MapFieldLite<K, V> mapFieldLite = (MapFieldLite<K, V>) mapField;
    MapEntryLite<K, V> defaultEntryLite = (MapEntryLite<K, V>) defaultEntry;

    if (mapFieldLite.isEmpty()) {
      return 0;
    }
    int size = 0;
    for (Map.Entry<K, V> entry : mapFieldLite.entrySet()) {
      size += defaultEntryLite.computeMessageSize(fieldNumber, entry.getKey(), entry.getValue());
    }
    return size;
  }
}
