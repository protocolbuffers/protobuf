// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

package com.google.protobuf;

/** A MessageInfo object describes a proto message type. */
@CheckReturnValue
interface MessageInfo {
  /** Gets syntax for this type. */
  ProtoSyntax getSyntax();

  /** Whether this type is MessageSet. */
  boolean isMessageSetWireFormat();

  /** Gets the default instance of this type. */
  MessageLite getDefaultInstance();
}
