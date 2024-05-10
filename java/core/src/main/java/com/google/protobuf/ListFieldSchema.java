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
interface ListFieldSchema {

  <L> List<L> mutableListAt(Object msg, long offset);

  void makeImmutableListAt(Object msg, long offset);

  <L> void mergeListsAt(Object msg, Object otherMsg, long offset);
}
