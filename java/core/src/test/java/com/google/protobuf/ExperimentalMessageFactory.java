// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

package com.google.protobuf;

/** Interface for a test factory for messages. */
public interface ExperimentalMessageFactory<T extends MessageLite> {
  /** Creates a new random message instance. */
  T newMessage();

  /** Gets the underlying data provider. */
  ExperimentalTestDataProvider dataProvider();
}
