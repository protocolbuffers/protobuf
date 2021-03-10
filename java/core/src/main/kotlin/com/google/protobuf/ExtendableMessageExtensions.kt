package com.google.protobuf.kotlin

import com.google.protobuf.ExtensionLite
import com.google.protobuf.GeneratedMessage

/** Sets the current value of the proto extension in this builder.*/
operator fun <
  M : GeneratedMessage.ExtendableMessage<M>,
  B : GeneratedMessage.ExtendableBuilder<M, B>,
  T
  > B.set(extension: ExtensionLite<M, T>, value: T) {
  setExtension(extension, value)
}

/** Gets the current value of the proto extension. */
operator fun <
  M : GeneratedMessage.ExtendableMessage<M>,
  MorBT : GeneratedMessage.ExtendableMessageOrBuilder<M>,
  T
  > MorBT.get(extension: ExtensionLite<M, T>): T = getExtension(extension)

/** Returns true if the specified extension is set on this builder. */
operator fun <
  M : GeneratedMessage.ExtendableMessage<M>,
  MorBT : GeneratedMessage.ExtendableMessageOrBuilder<M>
  > MorBT.contains(extension: ExtensionLite<M, *>): Boolean = hasExtension(extension)
