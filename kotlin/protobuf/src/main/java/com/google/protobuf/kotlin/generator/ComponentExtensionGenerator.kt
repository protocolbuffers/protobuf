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

package com.google.protobuf.kotlin.generator

import com.google.protobuf.Descriptors
import com.google.protobuf.Descriptors.FieldDescriptor
import com.google.protobuf.kotlin.protoc.Declarations
import com.google.protobuf.kotlin.protoc.GeneratorConfig
import com.google.protobuf.kotlin.protoc.MessageComponent
import com.google.protobuf.kotlin.protoc.OneofComponent
import com.google.protobuf.kotlin.protoc.Scope
import com.google.protobuf.kotlin.protoc.declarations
import com.google.protobuf.kotlin.protoc.specialize
import kotlin.reflect.KClass

/** Generates extensions for a specific type of component of a message. */
abstract class ComponentExtensionGenerator<T : MessageComponent>(
  private val componentType: KClass<T>
) : ExtensionGenerator() {
  final override fun GeneratorConfig.shallowExtensions(
    descriptor: Descriptors.Descriptor,
    enclosingScope: Scope,
    extensionScope: Scope
  ): Declarations = declarations {
    val components =
      descriptor.fields.map(FieldDescriptor::specialize) +
        descriptor.oneofs.map(::OneofComponent)
    for (component in components.filterIsInstance(componentType.java)) {
      merge(componentExtensions(component, enclosingScope, extensionScope))
    }
  }

  abstract fun GeneratorConfig.componentExtensions(
    component: T,
    enclosingScope: Scope,
    extensionScope: Scope
  ): Declarations
}
