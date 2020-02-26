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

import com.google.protobuf.Descriptors.FileDescriptor
import com.google.protobuf.kotlin.protoc.GeneratorConfig
import com.google.protobuf.kotlin.protoc.builder
import com.google.protobuf.kotlin.protoc.declarations
import com.google.protobuf.kotlin.protoc.objectBuilder
import com.google.protobuf.kotlin.protoc.outerClassSimpleName
import com.squareup.kotlinpoet.AnnotationSpec
import com.squareup.kotlinpoet.AnnotationSpec.UseSiteTarget
import com.squareup.kotlinpoet.FileSpec
import com.squareup.kotlinpoet.TypeSpec

/**
 * Logic to generate an entire file of Kotlin extensions for a proto file specified as a
 * FileDescriptor.
 */
class ProtoFileKotlinApiGenerator(
  val generator: RecursiveExtensionGenerator,
  val config: GeneratorConfig
) {

  fun generate(fileDescriptor: FileDescriptor): FileSpec {
    val packageName = config.javaPackage(fileDescriptor)
    val multipleFiles = fileDescriptor.options.javaMultipleFiles
    val outerExtensionClassName =
      fileDescriptor.outerClassSimpleName.withSuffix(generator.extensionSuffix)
    val outerExtensionClassScope = packageName.nestedScope(outerExtensionClassName)

    val builder = FileSpec.builder(packageName, outerExtensionClassName)

    if (config.aggressiveInlining) {
      builder.addAnnotation(
        AnnotationSpec.builder(Suppress::class).addMember("%S", "NOTHING_TO_INLINE").build()
      )
    }

    builder.addAnnotation(
      AnnotationSpec
        .builder(Suppress::class)
        .useSiteTarget(UseSiteTarget.FILE)
        .addMember("%S", "DEPRECATION")
        .build()
    )

    if (multipleFiles) {
      for (descriptor in fileDescriptor.messageTypes) {
        generator.generate(config, descriptor, packageName).writeAllAtTopLevel(builder)
      }
    } else {
      val declarations = declarations {
        for (descriptor in fileDescriptor.messageTypes) {
          merge(generator.generate(config, descriptor, outerExtensionClassScope))
        }
      }

      builder.addType(
        TypeSpec.objectBuilder(outerExtensionClassName)
          .addKdoc("Kotlin extensions for protos defined in the file %L.", fileDescriptor.fullName)
          .apply { declarations.writeToEnclosingType(this) }
          .build()
      )

      declarations.writeOnlyTopLevel(builder)
    }

    return builder.build()
  }
}
