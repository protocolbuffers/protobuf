package com.google.protobuf.kotlin

import com.google.protobuf.ExtensionLite
import com.google.protobuf.GeneratedMessageV3

/** Sets the current value of the proto extension in this builder.*/
operator fun <
  M : GeneratedMessageV3.ExtendableMessage<M>,
  B : GeneratedMessageV3.ExtendableBuilder<M, B>,
  T : Any
  > B.set(extension: ExtensionLite<M, T>, value: T) {
  setExtension(extension, value)
}

/** Gets the current value of the proto extension. */
operator fun <
  M : GeneratedMessageV3.ExtendableMessage<M>,
  MorBT : GeneratedMessageV3.ExtendableMessageOrBuilder<M>,
  T : Any
  > MorBT.get(extension: ExtensionLite<M, T>): T = getExtension(extension)

/** Returns true if the specified extension is set on this builder. */
operator fun <
  M : GeneratedMessageV3.ExtendableMessage<M>,
  MorBT : GeneratedMessageV3.ExtendableMessageOrBuilder<M>
  > MorBT.contains(extension: ExtensionLite<M, *>): Boolean = hasExtension(extension)
