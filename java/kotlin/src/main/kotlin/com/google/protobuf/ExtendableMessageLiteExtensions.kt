package com.google.protobuf.kotlin

import com.google.protobuf.ExtensionLite
import com.google.protobuf.GeneratedMessageLite

/** Gets the value of the proto extension. */
operator fun <
  M : GeneratedMessageLite.ExtendableMessage<M, *>,
  MOrBT : GeneratedMessageLite.ExtendableMessageOrBuilder<M, *>,
  T : Any
  > MOrBT.get(extension: ExtensionLite<M, T>): T = getExtension(extension)

/** Sets the current value of the proto extension in this builder. */
operator fun <
  M : GeneratedMessageLite.ExtendableMessage<M, B>,
  B : GeneratedMessageLite.ExtendableBuilder<M, B>,
  T : Any
  > B.set(extension: ExtensionLite<M, T>, value: T) {
  setExtension(extension, value)
}

/** Returns true if the specified extension is set. */
operator fun <
  M : GeneratedMessageLite.ExtendableMessage<M, *>,
  MorBT : GeneratedMessageLite.ExtendableMessageOrBuilder<M, *>
  > MorBT.contains(
  extension: ExtensionLite<M, *>
): Boolean = hasExtension(extension)
