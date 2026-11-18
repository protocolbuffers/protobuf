// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

package com.google.protobuf.kotlin

import com.google.protobuf.ExtensionLite
import com.google.protobuf.GeneratedMessage

/** Sets the current value of the proto extension in this builder. */
operator fun <
  M : GeneratedMessage.ExtendableMessage<M>,
  B : GeneratedMessage.ExtendableBuilder<M, B>,
  T : Any,
> B.set(extension: ExtensionLite<M, T>, value: T) {
  setExtension(extension, value)
}

/** Gets the current value of the proto extension. */
operator fun <
  M : GeneratedMessage.ExtendableMessage<M>,
  MorBT : GeneratedMessage.ExtendableMessageOrBuilder<M>,
  T : Any,
> MorBT.get(extension: ExtensionLite<M, T>): T = getExtension(extension)

/** Returns true if the specified extension is set on this builder. */
operator fun <
  M : GeneratedMessage.ExtendableMessage<M>,
  MorBT : GeneratedMessage.ExtendableMessageOrBuilder<M>,
> MorBT.contains(extension: ExtensionLite<M, *>): Boolean = hasExtension(extension)
