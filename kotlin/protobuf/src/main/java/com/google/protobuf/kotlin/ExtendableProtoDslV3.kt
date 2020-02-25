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

// not our choice
@file:Suppress("FINITE_BOUNDS_VIOLATION_IN_JAVA")

package com.google.protobuf.kotlin

import com.google.protobuf.ByteString
import com.google.protobuf.ExtensionLite
import com.google.protobuf.GeneratedMessageV3
import com.google.protobuf.Message

/** Superclass of DSLs for the creation of extendable protos in the full runtime. */
abstract class ExtendableProtoDslV3<
  M : GeneratedMessageV3.ExtendableMessage<M>,
  B : GeneratedMessageV3.ExtendableBuilder<M, B>
  >(builder: B) : ProtoDsl<B>(builder) {
  // expose the builder to the below extension functions
  internal val builder: B get() = _proto_dsl_builder

  /** Get the current value of the proto extension in this DSL. */
  operator fun <T> get(extension: ExtensionLite<M, T>): T {
    return if (extension.isRepeated) {
      @Suppress("UNCHECKED_CAST")
      ExtensionListV3(builder, extension as ExtensionLite<M, List<*>>) as T
    } else {
      builder.getExtension(extension)
    }
  }

  /** Gets the values of the repeated proto extension in this DSL. */
  @JvmName("getRepeatedExtension") // to avoid issues with erasure
  operator fun <T> get(extension: ExtensionLite<M, List<T>>): ExtensionListV3<T, M, B> =
    ExtensionListV3(builder, extension)

  /** Analog to [GeneratedMessageV3.ExtendableBuilder.hasExtension]. */
  operator fun contains(extension: ExtensionLite<M, *>): Boolean = builder.hasExtension(extension)

  /** Adds an element to this proto extension. */
  fun <E> ExtensionListV3<E, M, B>.add(element: E) {
    addElement(element)
  }

  /** Adds elements to this proto extension. */
  fun <E> ExtensionListV3<E, M, B>.addAll(elements: Iterable<E>) {
    elements.forEach { add(it) }
  }

  /** Adds an element to this proto extension. */
  operator fun <E> ExtensionListV3<E, M, B>.plusAssign(element: E) {
    addElement(element)
  }

  /** Adds elements to this proto extension. */
  operator fun <E> ExtensionListV3<E, M, B>.plusAssign(elements: Iterable<E>) {
    elements.forEach { add(it) }
  }

  /** Sets the specified element of the proto extension. */
  operator fun <E> ExtensionListV3<E, M, B>.set(index: Int, element: E) {
    setElement(index, element)
  }

  /** Clears the elements of the proto extension. */
  fun <E> ExtensionListV3<E, M, B>.clear() {
    clearElements()
  }

  // everything except repeated fields

  /** Sets the value of the proto extension. */
  @JvmName("setMessageExtension") // to avoid issues with erasure
  operator fun <T : Message> set(extension: ExtensionLite<M, T>, value: T) {
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

  /** Clears the value of the proto extension. */
  fun clearExtension(extension: ExtensionLite<M, *>) {
    builder.clearExtension(extension)
  }
}
