// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

package com.google.protobuf;

/** A factory that creates {@link MessageInfo} instances for message types. */
@ExperimentalApi
@CheckReturnValue
interface MessageInfoFactory {
  /** Whether the message class is supported by this factory. */
  boolean isSupported(Class<?> clazz);

  /** Returns a information of the message class. */
  MessageInfo messageInfoFor(Class<?> clazz);
}
