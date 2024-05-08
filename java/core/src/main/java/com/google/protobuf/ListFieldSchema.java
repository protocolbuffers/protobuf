// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

package com.google.protobuf;

import java.util.List;

/**
 * Utility class that aids in properly manipulating list fields for either the lite or full runtime.
 */
@CheckReturnValue
abstract class ListFieldSchema {
  // Disallow construction.
  ListFieldSchema() {}

  abstract <L> List<L> mutableListAt(Object msg, long offset);

  abstract void makeImmutableListAt(Object msg, long offset);

  abstract <L> void mergeListsAt(Object msg, Object otherMsg, long offset);
}
