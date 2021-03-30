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

import java.util.Map;

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
  Object mergeFrom(Object destMapField, Object srcMapField);

  /** Compute the serialized size for the map with a given field number. */
  int getSerializedSize(int fieldNumber, Object mapField, Object mapDefaultEntry);
}
