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

class MapFieldSchemaLite implements MapFieldSchema {

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
