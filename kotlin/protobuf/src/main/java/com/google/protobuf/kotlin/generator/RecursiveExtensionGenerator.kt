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

import com.google.protobuf.Descriptors.Descriptor
import com.google.protobuf.kotlin.protoc.Declarations
import com.google.protobuf.kotlin.protoc.GeneratorConfig
import com.google.protobuf.kotlin.protoc.Scope
import com.google.protobuf.kotlin.protoc.declarations
import com.google.protobuf.kotlin.protoc.messageClassSimpleName
import com.google.protobuf.kotlin.protoc.objectBuilder
import com.squareup.kotlinpoet.TypeSpec

/**
 * Given generators for extension APIs on a single message, generates the APIs for all nested
 * messages, within appropriately named objects.
 */
class RecursiveExtensionGenerator(
  val extensionSuffix: String,
  private vararg val generators: ExtensionGenerator
) {
  fun generate(
    config: GeneratorConfig,
    descriptor: Descriptor,
    scope: Scope
  ): Declarations = declarations {
    if (descriptor.options.mapEntry) {
      return@declarations
    }
    with(config) {
      val nestedSimpleName = descriptor.messageClassSimpleName.withSuffix(extensionSuffix)
      val nestedScope = scope.nestedScope(nestedSimpleName)
      for (generator in generators) {
        if (!generator.generateWithinExtensionClass) {
          generator.run {
            merge(shallowExtensions(descriptor, scope, nestedScope))
          }
        }
      }

      val nestedDeclarations = declarations {
        for (nestedType in descriptor.nestedTypes) {
          merge(generate(config, nestedType, nestedScope))
        }
      }

      mergeTopLevelOnly(nestedDeclarations)

      addType(
        TypeSpec
          .objectBuilder(nestedSimpleName)
          .addKdoc("Kotlin extensions around the proto message type %L.", descriptor.fullName)
          .apply {
            for (generator in generators) {
              if (generator.generateWithinExtensionClass) {
                val decls = generator.run {
                  shallowExtensions(descriptor, scope, nestedScope)
                }
                mergeTopLevelOnly(decls)
                decls.writeToEnclosingType(this)
              }
            }
            nestedDeclarations.writeToEnclosingType(this)
          }
          .build()
      )
    }
  }
}
