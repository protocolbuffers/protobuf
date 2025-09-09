// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

package com.google.protobuf;

import java.util.List;

/**
 * A base class for package private shared methods between MapField and MapFieldBuilder to allow
 * reflection to access both.
 */
public abstract class MapFieldReflectionAccessor {
  /** Gets the content of this MapField as a list of read-only values. */
  abstract List<Message> getList();

  /** Gets a mutable List view of this MapField. */
  abstract List<Message> getMutableList();

  /** Gets the default instance of the message stored in the list view of this map field. */
  abstract Message getMapEntryMessageDefaultInstance();
}
