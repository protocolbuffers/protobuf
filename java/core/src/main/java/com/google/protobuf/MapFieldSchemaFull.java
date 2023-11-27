// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

package com.google.protobuf;

import com.google.protobuf.MapEntryLite.Metadata;
import java.util.Map;

class MapFieldSchemaFull implements MapFieldSchema {
  @Override
  public Map<?, ?> forMutableMapData(Object mapField) {
    return ((MapField<?, ?>) mapField).getMutableMap();
  }

  @Override
  public Map<?, ?> forMapData(Object mapField) {
    return ((MapField<?, ?>) mapField).getMap();
  }

  @Override
  public boolean isImmutable(Object mapField) {
    return !((MapField<?, ?>) mapField).isMutable();
  }

  @Override
  public Object toImmutable(Object mapField) {
    ((MapField<?, ?>) mapField).makeImmutable();
    return mapField;
  }

  @Override
  public Object newMapField(Object mapDefaultEntry) {
    return MapField.newMapField((MapEntry<?, ?>) mapDefaultEntry);
  }

  @Override
  public Metadata<?, ?> forMapMetadata(Object mapDefaultEntry) {
    return ((MapEntry<?, ?>) mapDefaultEntry).getMetadata();
  }

  @Override
  public Object mergeFrom(Object destMapField, Object srcMapField) {
    return mergeFromFull(destMapField, srcMapField);
  }

  @SuppressWarnings("unchecked")
  private static <K, V> Object mergeFromFull(Object destMapField, Object srcMapField) {
    MapField<K, V> mine = (MapField<K, V>) destMapField;
    MapField<K, V> other = (MapField<K, V>) srcMapField;
    if (!mine.isMutable()) {
      mine.copy();
    }
    mine.mergeFrom(other);
    return mine;
  }

  @Override
  public int getSerializedSize(int number, Object mapField, Object mapDefaultEntry) {
    return getSerializedSizeFull(number, mapField, mapDefaultEntry);
  }

  @SuppressWarnings("unchecked")
  private static <K, V> int getSerializedSizeFull(
      int number, Object mapField, Object defaultEntryObject) {
    // Full runtime allocates map fields lazily.
    if (mapField == null) {
      return 0;
    }

    Map<K, V> map = ((MapField<K, V>) mapField).getMap();
    MapEntry<K, V> defaultEntry = (MapEntry<K, V>) defaultEntryObject;
    if (map.isEmpty()) {
      return 0;
    }
    int size = 0;
    for (Map.Entry<K, V> entry : map.entrySet()) {
      size +=
          CodedOutputStream.computeTagSize(number)
              + CodedOutputStream.computeLengthDelimitedFieldSize(
                  MapEntryLite.computeSerializedSize(
                      defaultEntry.getMetadata(), entry.getKey(), entry.getValue()));
    }
    return size;
  }
}
