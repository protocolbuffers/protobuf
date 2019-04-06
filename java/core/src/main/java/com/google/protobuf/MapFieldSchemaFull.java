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
