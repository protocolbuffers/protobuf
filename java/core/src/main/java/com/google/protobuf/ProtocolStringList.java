// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

package com.google.protobuf;

import java.util.List;

/**
 * An interface extending {@code List<String>} used for repeated string fields to provide optional
 * access to the data as a list of ByteStrings. The underlying implementation stores values as
 * either ByteStrings or Strings (see {@link LazyStringArrayList}) depending on how the value was
 * initialized or last read, and it is often more efficient to deal with lists of ByteStrings when
 * handling protos that have been deserialized from bytes.
 */
public interface ProtocolStringList extends List<String> {

  /** Returns a view of the data as a list of ByteStrings. */
  List<ByteString> asByteStringList();
}
