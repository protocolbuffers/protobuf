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
import com.google.protobuf.GeneratedMessageLite


/** Gets the value of the proto extension. */
operator fun <
  M : GeneratedMessageLite.ExtendableMessage<M, *>,
  MOrBT : GeneratedMessageLite.ExtendableMessageOrBuilder<M, *>,
  T
  > MOrBT.get(extension: ExtensionLite<M, T>): T = getExtension(extension)

/** Sets the current value of the proto extension in this builder. */
operator fun <
  M : GeneratedMessageLite.ExtendableMessage<M, B>,
  B : GeneratedMessageLite.ExtendableBuilder<M, B>,
  T
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
