/*
 * Copyright 2020 Google LLC
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.google.protobuf.kotlin.extensions

import com.google.protobuf.ExtensionLite
import com.google.protobuf.GeneratedMessageV3

/** Sets the current value of the proto extension in this builder.*/
operator fun <
  M : GeneratedMessageV3.ExtendableMessage<M>,
  B : GeneratedMessageV3.ExtendableBuilder<M, B>,
  T
  > B.set(extension: ExtensionLite<M, T>, value: T) {
  setExtension(extension, value)
}

/** Gets the current value of the proto extension. */
operator fun <
  M : GeneratedMessageV3.ExtendableMessage<M>,
  MorBT : GeneratedMessageV3.ExtendableMessageOrBuilder<M>,
  T
  > MorBT.get(extension: ExtensionLite<M, T>): T = getExtension(extension)

/** Returns true if the specified extension is set on this builder. */
operator fun <
  M : GeneratedMessageV3.ExtendableMessage<M>,
  MorBT : GeneratedMessageV3.ExtendableMessageOrBuilder<M>
  > MorBT.contains(extension: ExtensionLite<M, *>): Boolean = hasExtension(extension)
