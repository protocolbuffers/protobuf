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

import java.util.ArrayList;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import java.util.function.BiConsumer;

/**
 * Internal representation of map fields in generated builders.
 *
 * <p>This class supports accessing the map field as a {@link Map} to be used in generated API and
 * also supports accessing the field as a {@link List} to be used in reflection API. It keeps track
 * of where the data is currently stored and do necessary conversions between map and list.
 *
 * <p>This class is a protobuf implementation detail. Users shouldn't use this class directly.
 */
public class MapFieldBuilder<
        KeyT,
        MessageOrBuilderT extends MessageOrBuilder,
        MessageT extends MessageOrBuilderT,
        BuilderT extends MessageOrBuilderT>
    extends MapFieldReflectionAccessor {

  // Only one of the three fields may be non-null at any time.
  /** nullable */
  Map<KeyT, MessageOrBuilderT> builderMap = new LinkedHashMap<>();

  /** nullable */
  Map<KeyT, MessageT> messageMap = null;

  // messageList elements are always MapEntry<KeyT, MessageT>, but we need a List<Message> for
  // reflection.
  /** nullable */
  List<Message> messageList = null;

  Converter<KeyT, MessageOrBuilderT, MessageT> converter;

  /** Convert a MessageOrBuilder to a Message regardless of which it holds. */
  public interface Converter<
      KeyT, MessageOrBuilderT extends MessageOrBuilder, MessageT extends MessageOrBuilderT> {
    MessageT build(MessageOrBuilderT val);

    MapEntry<KeyT, MessageT> defaultEntry();
  }

  public MapFieldBuilder(Converter<KeyT, MessageOrBuilderT, MessageT> converter) {
    this.converter = converter;
  }

  @SuppressWarnings("unchecked")
  private void forEachListEntry(BiConsumer<KeyT, MessageT> f) {
    messageList.forEach(
        entry -> {
          MapEntry<KeyT, MessageT> typedEntry = (MapEntry<KeyT, MessageT>) entry;
          f.accept(typedEntry.getKey(), typedEntry.getValue());
        });
  }

  public Map<KeyT, MessageOrBuilderT> ensureBuilderMap() {
    if (builderMap != null) {
      return builderMap;
    }
    if (messageMap != null) {
      builderMap = new LinkedHashMap<>(messageMap.size());
      messageMap.forEach((key, value) -> builderMap.put(key, value));
      messageMap = null;
      return builderMap;
    }
    builderMap = new LinkedHashMap<>(messageList.size());
    forEachListEntry((key, value) -> builderMap.put(key, value));
    messageList = null;
    return builderMap;
  }

  public List<Message> ensureMessageList() {
    if (messageList != null) {
      return messageList;
    }
    if (builderMap != null) {
      messageList = new ArrayList<>(builderMap.size());
      builderMap.forEach(
          (key, value) ->
              messageList.add(
                  converter.defaultEntry().toBuilder()
                      .setKey(key)
                      .setValue(converter.build(value))
                      .build()));
      builderMap = null;
      return messageList;
    }
    messageList = new ArrayList<>(messageMap.size());
    messageMap.forEach(
        (key, value) ->
            messageList.add(
                converter.defaultEntry().toBuilder().setKey(key).setValue(value).build()));
    messageMap = null;
    return messageList;
  }

  public Map<KeyT, MessageT> ensureMessageMap() {
    messageMap = populateMutableMap();
    builderMap = null;
    messageList = null;
    return messageMap;
  }

  public Map<KeyT, MessageT> getImmutableMap() {
    return new MapField.MutabilityAwareMap<>(MutabilityOracle.IMMUTABLE, populateMutableMap());
  }

  private Map<KeyT, MessageT> populateMutableMap() {
    if (messageMap != null) {
      return messageMap;
    }
    if (builderMap != null) {
      Map<KeyT, MessageT> toReturn = new LinkedHashMap<>(builderMap.size());
      builderMap.forEach((key, value) -> toReturn.put(key, converter.build(value)));
      return toReturn;
    }
    Map<KeyT, MessageT> toReturn = new LinkedHashMap<>(messageList.size());
    forEachListEntry((key, value) -> toReturn.put(key, value));
    return toReturn;
  }

  public void mergeFrom(MapField<KeyT, MessageT> other) {
    ensureBuilderMap().putAll(MapFieldLite.copy(other.getMap()));
  }

  public void clear() {
    builderMap = new LinkedHashMap<>();
    messageMap = null;
    messageList = null;
  }

  private boolean typedEquals(MapFieldBuilder<KeyT, MessageOrBuilderT, MessageT, BuilderT> other) {
    return MapFieldLite.<KeyT, MessageOrBuilderT>equals(
        ensureBuilderMap(), other.ensureBuilderMap());
  }

  @SuppressWarnings("unchecked")
  @Override
  public boolean equals(Object object) {
    if (!(object instanceof MapFieldBuilder)) {
      return false;
    }
    return typedEquals((MapFieldBuilder<KeyT, MessageOrBuilderT, MessageT, BuilderT>) object);
  }

  @Override
  public int hashCode() {
    return MapFieldLite.<KeyT, MessageOrBuilderT>calculateHashCodeForMap(ensureBuilderMap());
  }

  /** Returns a deep copy of this MapFieldBuilder. */
  public MapFieldBuilder<KeyT, MessageOrBuilderT, MessageT, BuilderT> copy() {
    MapFieldBuilder<KeyT, MessageOrBuilderT, MessageT, BuilderT> clone =
        new MapFieldBuilder<>(converter);
    clone.ensureBuilderMap().putAll(ensureBuilderMap());
    return clone;
  }

  /** Converts this MapFieldBuilder to a MapField. */
  public MapField<KeyT, MessageT> build(MapEntry<KeyT, MessageT> defaultEntry) {
    MapField<KeyT, MessageT> mapField = MapField.newMapField(defaultEntry);
    Map<KeyT, MessageT> map = mapField.getMutableMap();
    ensureBuilderMap().forEach((key, value) -> map.put(key, converter.build(value)));
    mapField.makeImmutable();
    return mapField;
  }

  // MapFieldReflectionAccessor implementation.
  /** Gets the content of this MapField as a read-only List. */
  @Override
  List<Message> getList() {
    return ensureMessageList();
  }

  /** Gets a mutable List view of this MapField. */
  @Override
  List<Message> getMutableList() {
    return ensureMessageList();
  }

  /** Gets the default instance of the message stored in the list view of this map field. */
  @Override
  Message getMapEntryMessageDefaultInstance() {
    return converter.defaultEntry();
  }
}
