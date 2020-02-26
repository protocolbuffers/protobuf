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

package com.google.protobuf.kotlin

import com.google.protobuf.ExtensionLite
import com.google.protobuf.GeneratedMessageV3

/** An unmodifiable view of a repeated extension field in a proto builder. */
@Suppress("FINITE_BOUNDS_VIOLATION_IN_JAVA", "unused")
class ExtensionListV3<
  E,
  M : GeneratedMessageV3.ExtendableMessage<M>,
  B : GeneratedMessageV3.ExtendableBuilder<M, B>> internal constructor(
  internal val builder: B,
  internal val extension: ExtensionLite<M, List<E>>
) : AbstractList<E>(), RandomAccess {
  override val size: Int
    get() = builder.getExtensionCount(extension)

  override fun get(index: Int): E = builder.getExtension(extension, index)

  internal fun addElement(e: E) {
    builder.addExtension(extension, e)
  }

  internal fun setElement(index: Int, e: E) {
    builder.setExtension(extension, index, e)
  }

  internal fun clearElements() {
    builder.clearExtension(extension)
  }
}