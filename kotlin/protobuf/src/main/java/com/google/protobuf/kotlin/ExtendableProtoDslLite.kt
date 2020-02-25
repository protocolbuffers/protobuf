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

import com.google.protobuf.ByteString
import com.google.protobuf.ExtensionLite
import com.google.protobuf.GeneratedMessageLite
import com.google.protobuf.MessageLite

/** Superclass of DSLs for the creation of protos in the lite runtime. */
abstract class ExtendableProtoDslLite<
  M : GeneratedMessageLite.ExtendableMessage<M, B>,
  B : GeneratedMessageLite.ExtendableBuilder<M, B>>(
  builder: B
) : ProtoDsl<B>(builder) {
  // expose the builder to the below extension functions
  internal val builder: B get() = _proto_dsl_builder

  /** Get the current value of the proto extension in this DSL. */
  operator fun <T> get(extension: ExtensionLite<M, T>): T {
    return if (extension.isRepeated) {
      @Suppress("UNCHECKED_CAST")
      ExtensionListLite(builder, extension as ExtensionLite<M, List<*>>) as T
    } else {
      builder.getExtension(extension)
    }
  }

  /** Gets the values of the repeated proto extension in this DSL. */
  @JvmName("getRepeatedExtension") // to avoid erasure issues
  operator fun <T> get(extension: ExtensionLite<M, List<T>>): ExtensionListLite<T, M, B> =
    ExtensionListLite(builder, extension)

  /** Analog to [GeneratedMessageLite.ExtendableBuilder.hasExtension]. */
  operator fun contains(extension: ExtensionLite<M, *>): Boolean =
    builder.hasExtension(extension)

  /** Adds a value to this repeated extension. */
  fun <M : GeneratedMessageLite.ExtendableMessage<M, B>, E>
      ExtensionListLite<E, M, *>.add(element: E) {
    addElement(element)
  }

  /** Adds the values to this repeated extension. */
  fun <M : GeneratedMessageLite.ExtendableMessage<M, B>, E>
      ExtensionListLite<E, M, *>.addAll(elements: Iterable<E>) {
    elements.forEach { add(it) }
  }

  /** Adds a value to this repeated extension. */
  operator fun <M : GeneratedMessageLite.ExtendableMessage<M, B>, E>
      ExtensionListLite<E, M, *>.plusAssign(element: E) {
    add(element)
  }

  /** Adds the values to this repeated extension. */
  operator fun <M : GeneratedMessageLite.ExtendableMessage<M, B>, E>
      ExtensionListLite<E, M, *>.plusAssign(elements: Iterable<E>) {
    elements.forEach { add(it) }
  }

  /** Sets the value of one element of this repeated extension. */
  operator fun <M : GeneratedMessageLite.ExtendableMessage<M, B>, E>
      ExtensionListLite<E, M, *>.set(index: Int, element: E) {
    setElement(index, element)
  }

  /** Clears all elements of this repeated extension. */
  fun <M : GeneratedMessageLite.ExtendableMessage<M, B>, E>
      ExtensionListLite<E, M, *>.clear() {
    clearElements()
  }

  /** Sets the value of the proto extension. */
  @JvmName("setMessageExtension") // to avoid issues with erasure
  operator fun <T : MessageLite> set(extension: ExtensionLite<M, T>, value: T) {
    builder.setExtension(extension, value)
  }

  /** Sets the value of the proto extension. */
  @JvmName("setByteStringExtension") // to avoid issues with erasure
  operator fun set(extension: ExtensionLite<M, ByteString>, value: ByteString) {
    builder.setExtension(extension, value)
  }

  /** Sets the value of the proto extension. */
  @JvmName("setComparableExtension") // to avoid issues with erasure
  // Comparable covers String, Enum, and all primitives
  operator fun <T : Comparable<T>> set(extension: ExtensionLite<M, T>, value: T) {
    builder.setExtension(extension, value)
  }

  /** Clears the specified extension. */
  fun <B> clearExtension(extension: ExtensionLite<M, *>) {
    builder.clearExtension(extension)
  }
}
