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
import com.google.protobuf.kotlin.protoc.AbstractGeneratorRunner
import com.google.protobuf.kotlin.protoc.ClassSimpleName
import com.google.protobuf.kotlin.protoc.GeneratorConfig
import com.google.protobuf.kotlin.protoc.JavaPackagePolicy
import com.google.protobuf.kotlin.protoc.MemberSimpleName
import com.squareup.kotlinpoet.ClassName
import com.squareup.kotlinpoet.FileSpec

/** Runner for the proto API generator. */
class GeneratorRunner(runtimeVariant: RuntimeVariant) : AbstractGeneratorRunner() {
  enum class RuntimeVariant(
    packageName: String,
    dslSuperclassSimpleName: String
  ) {
    FULL("com.google.protobuf.kotlin", "ExtendableProtoDslV3"),
    LITE("com.google.protobuf.kotlin", "ExtendableProtoDslLite");

    val extendableDslSuperclass: ClassName = ClassName(packageName, dslSuperclassSimpleName)
  }

  companion object {
    @JvmStatic
    fun main(args: Array<String>) {
      var runtimeVariant = RuntimeVariant.FULL
      if (args.size == 3) {
        runtimeVariant = when (args[2]) {
          "lite" -> RuntimeVariant.LITE
          "full" -> RuntimeVariant.FULL
          else -> throw IllegalArgumentException(
            "Expected first command line arg to be 'lite' or 'full' but was '${args[0]}'"
          )
        }
        GeneratorRunner(runtimeVariant).doMain(args.drop(1).toTypedArray())
      } else {
        GeneratorRunner(runtimeVariant).doMain(args)
      }
    }
  }

  override fun generateCodeForFile(file: Descriptors.FileDescriptor): List<FileSpec> =
    listOf(generator.generate(file))

  private val config = GeneratorConfig(
    javaPackagePolicy = JavaPackagePolicy.OPEN_SOURCE,
    aggressiveInlining = false
  )

  private val oneofClassGenerator = OneofClassGenerator(
    oneofClassSuffix = "Oneof",
    oneofSubclassSuffix = "",
    notSetSimpleName = ClassSimpleName("NotSet"),
    propertySuffix = "",
    extensionFactories = *arrayOf(
      OneofClassGenerator::orBuilderVal,
      OneofClassGenerator::builderVar
    )
  )

  val generator = ProtoFileKotlinApiGenerator(
    config = config,
    generator = RecursiveExtensionGenerator(
      extensionSuffix = "Kt",
      generators = *arrayOf(
        oneofClassGenerator,
        DslGenerator(
          extendableDslSuperclass = runtimeVariant.extendableDslSuperclass,
          dslClassNameSuffix = "Dsl",
          dslFactoryNamePrefix = "",
          copyName = MemberSimpleName("copy"),
          componentGenerators = *arrayOf(
            SingletonFieldPropertyDslComponentGenerator,
            SingletonFieldHazzerDslComponentGenerator,
            SingletonFieldClearDslComponentGenerator,
            OneofCaseDslComponentGenerator,
            oneofClassGenerator.dslPropertyGenerator,
            RepeatedFieldListProxyDslComponentGenerator(
              proxyTypeNameSuffix = "Proxy",
              extensionGenerators = *arrayOf(
                RepeatedRepresentativeExtensionGenerator.AddElement.add,
                RepeatedRepresentativeExtensionGenerator.AddElement.plusAssign,
                RepeatedRepresentativeExtensionGenerator.AddAllIterable.addAll,
                RepeatedRepresentativeExtensionGenerator.AddAllIterable.plusAssign,
                RepeatedRepresentativeExtensionGenerator.SetOperator,
                RepeatedRepresentativeExtensionGenerator.Clear
              )
            ),
            MapFieldMapProxyDslComponentGenerator(
              proxyTypeNameSuffix = "Proxy",
              extensionGenerators = *arrayOf(
                MapRepresentativeExtensionGenerator.Put.put,
                MapRepresentativeExtensionGenerator.Put.setOperator,
                MapRepresentativeExtensionGenerator.RemoveKey.remove,
                MapRepresentativeExtensionGenerator.PutAllMap.putAll,
                MapRepresentativeExtensionGenerator.Clear
              )
            )
          )
        )
      )
    )
  )
}
