// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

package com.google.protobuf;

import java.util.Map;

@CheckReturnValue
interface MapFieldSchema {
  /** Returns the map data for mutation. */
  Map<?, ?> forMutableMapData(Object mapField);

  /** Returns the map data for read. */
  Map<?, ?> forMapData(Object mapField);

  /** Whether toImmutable() has been called on this map field. */
  boolean isImmutable(Object mapField);

  /**
   * Returns an immutable instance of the map field. It may make the parameter immutable and return
   * the parameter, or create an immutable copy. The status of the parameter after the call is
   * undefined.
   */
  Object toImmutable(Object mapField);

  /** Returns a new instance of the map field given a map default entry. */
  Object newMapField(Object mapDefaultEntry);

  /** Returns the metadata from a default entry. */
  MapEntryLite.Metadata<?, ?> forMapMetadata(Object mapDefaultEntry);

  /** Merges {@code srcMapField} into {@code destMapField}, and returns the merged instance. */
  @CanIgnoreReturnValue
  Object mergeFrom(Object destMapField, Object srcMapField);

  /** Compute the serialized size for the map with a given field number. */
  int getSerializedSize(int fieldNumber, Object mapField, Object mapDefaultEntry);
}
