// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

package com.google.protobuf;

import java.util.ArrayList;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;

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

  // We need a List<Message> for reflection.
  //
  // messageList elements are always MapEntry<KeyT, SomeT extends Message>, where SomeT and MessageT
  // have the same descriptor (i.e. SomeT can be DynamicMessage)
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
  private List<MapEntry<KeyT, MessageT>> getMapEntryList() {
    ArrayList<MapEntry<KeyT, MessageT>> list = new ArrayList<>(messageList.size());
    Class<?> valueClass = converter.defaultEntry().getValue().getClass();
    for (Message entry : messageList) {
      MapEntry<KeyT, ?> typedEntry = (MapEntry<KeyT, ?>) entry;
      if (valueClass.isInstance(typedEntry.getValue())) {
        list.add((MapEntry<KeyT, MessageT>) typedEntry);
      } else {
        // This needs to use mergeFrom to allow MapEntry<KeyT, DynamicMessage> to be used.
        list.add(converter.defaultEntry().toBuilder().mergeFrom(entry).build());
      }
    }
    return list;
  }

  public Map<KeyT, MessageOrBuilderT> ensureBuilderMap() {
    if (builderMap != null) {
      return builderMap;
    }
    if (messageMap != null) {
      builderMap = new LinkedHashMap<>(messageMap.size());
      for (Map.Entry<KeyT, MessageT> entry : messageMap.entrySet()) {
        builderMap.put(entry.getKey(), entry.getValue());
      }
      messageMap = null;
      return builderMap;
    }
    builderMap = new LinkedHashMap<>(messageList.size());
    for (MapEntry<KeyT, MessageT> entry : getMapEntryList()) {
      builderMap.put(entry.getKey(), entry.getValue());
    }
    messageList = null;
    return builderMap;
  }

  public List<Message> ensureMessageList() {
    if (messageList != null) {
      return messageList;
    }
    if (builderMap != null) {
      messageList = new ArrayList<>(builderMap.size());
      for (Map.Entry<KeyT, MessageOrBuilderT> entry : builderMap.entrySet()) {
        messageList.add(
            converter.defaultEntry().toBuilder()
                .setKey(entry.getKey())
                .setValue(converter.build(entry.getValue()))
                .build());
      }
      builderMap = null;
      return messageList;
    }
    messageList = new ArrayList<>(messageMap.size());
    for (Map.Entry<KeyT, MessageT> entry : messageMap.entrySet()) {
      messageList.add(
          converter.defaultEntry().toBuilder()
              .setKey(entry.getKey())
              .setValue(entry.getValue())
              .build());
    }
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
      for (Map.Entry<KeyT, MessageOrBuilderT> entry : builderMap.entrySet()) {
        toReturn.put(entry.getKey(), converter.build(entry.getValue()));
      }
      return toReturn;
    }
    Map<KeyT, MessageT> toReturn = new LinkedHashMap<>(messageList.size());
    for (MapEntry<KeyT, MessageT> entry : getMapEntryList()) {
      toReturn.put(entry.getKey(), entry.getValue());
    }
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
    for (Map.Entry<KeyT, MessageOrBuilderT> entry : ensureBuilderMap().entrySet()) {
      map.put(entry.getKey(), converter.build(entry.getValue()));
    }
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
