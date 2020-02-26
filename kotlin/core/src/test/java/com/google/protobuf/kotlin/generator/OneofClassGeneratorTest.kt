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

import com.google.protobuf.kotlin.protoc.ClassSimpleName
import com.google.protobuf.kotlin.protoc.GeneratorConfig
import com.google.protobuf.kotlin.protoc.JavaPackagePolicy
import com.google.protobuf.kotlin.protoc.OneofComponent
import com.google.protobuf.kotlin.protoc.OneofOptionFieldDescriptor
import com.google.protobuf.kotlin.protoc.UnqualifiedScope
import com.google.protobuf.kotlin.protoc.classBuilder
import com.google.protobuf.kotlin.protoc.specialize
import com.google.protobuf.kotlin.protoc.testing.assertThat
import com.google.protobuf.kotlin.generator.Example3
import com.google.protobuf.kotlin.generator.Example3.ExampleMessage
import com.squareup.kotlinpoet.ClassName
import com.squareup.kotlinpoet.CodeBlock
import com.squareup.kotlinpoet.TypeSpec
import java.nio.file.Files
import java.nio.file.Paths
import org.junit.Test
import org.junit.runner.RunWith
import org.junit.runners.JUnit4

/** Tests for [OneofClassGenerator]. */
@RunWith(JUnit4::class)
class OneofClassGeneratorTest {
  private val javaPackagePolicy = JavaPackagePolicy.OPEN_SOURCE
  private val config = GeneratorConfig(javaPackagePolicy, aggressiveInlining = false)
  private val oneofDescriptor =
    ExampleMessage.getDescriptor().oneofs.single { it.name == "my_oneof" }
  private val generatorUnderTest = OneofClassGenerator(
    oneofClassSuffix = "OneofClass",
    oneofSubclassSuffix = "OneofClass",
    notSetSimpleName = ClassSimpleName("NotSetName"),
    propertySuffix = "Property",
    extensionFactories =
      *arrayOf(OneofClassGenerator::builderVar, OneofClassGenerator::orBuilderVal)
  )
  private val oneofClassName: ClassName =
    UnqualifiedScope.nestedClass(ClassSimpleName("MyOneofClass"))
  private val example3: String = Example3::class.qualifiedName!!
  private val exampleMessage: String = ExampleMessage::class.qualifiedName!!

  @Test
  fun writeTo() {
    val writeToFun = generatorUnderTest.run {
      config.writeToFunction(oneofDescriptor)
    }
    assertThat(writeToFun).generates(
      """
internal fun writeTo(builder: com.google.protobuf.kotlin.generator.Example3.ExampleMessage.Builder) {
  when (case) {
    com.google.protobuf.kotlin.generator.Example3.ExampleMessage.MyOneofCase.STRING_ONEOF_OPTION -> builder.stringOneofOption = value as kotlin.String
    com.google.protobuf.kotlin.generator.Example3.ExampleMessage.MyOneofCase.SUB_MESSAGE_ONEOF_OPTION -> builder.subMessageOneofOption = value as com.google.protobuf.kotlin.generator.Example3.ExampleMessage.SubMessage
    else -> builder.clearMyOneof()
  }
}
      """
    )
  }

  @Test
  fun readFrom() {
    val readFromFun = generatorUnderTest.run {
      config.readFromFunction(oneofDescriptor, oneofClassName)
    }
    assertThat(readFromFun).generates(
      """
/**
 * Extract an object representation of the set field of the oneof my_oneof.
 */
@kotlin.PublishedApi
internal fun readFrom(input: $example3.ExampleMessageOrBuilder): MyOneofClass<*> {
  val _theCase: $exampleMessage.MyOneofCase = input.myOneofCase
  return when (_theCase) {
    $exampleMessage.MyOneofCase.STRING_ONEOF_OPTION -> MyOneofClass.StringOneofOptionOneofClass(input.stringOneofOption)
    $exampleMessage.MyOneofCase.SUB_MESSAGE_ONEOF_OPTION -> MyOneofClass.SubMessageOneofOptionOneofClass(input.subMessageOneofOption)
    else -> MyOneofClass.NotSetName
  }
}
    """
    )
  }

  @Test
  fun oneofOptionClass() {
    val oneofOption =
      ExampleMessage
        .getDescriptor()
        .findFieldByNumber(ExampleMessage.STRING_ONEOF_OPTION_FIELD_NUMBER)
        .specialize() as OneofOptionFieldDescriptor

    val oneofOptionClass = generatorUnderTest.run {
      config.oneofOptionClass(oneofOption, oneofClassName)
    }
    assertThat(oneofOptionClass).generates(
      """
/**
 * A representation of the string_oneof_option option of the my_oneof oneof.
 */
class StringOneofOptionOneofClass(
  value: kotlin.String
) : MyOneofClass<kotlin.String>(value, com.google.protobuf.kotlin.generator.Example3.ExampleMessage.MyOneofCase.STRING_ONEOF_OPTION)
    """
    )
  }

  @Test
  fun notSetObject() {
    val generatorWithNotSetObject = OneofClassGenerator(
      oneofClassSuffix = "OneofClass",
      notSetSimpleName = ClassSimpleName("NotSetName"),
      propertySuffix = "Property"
    )
    val notSetObject = generatorWithNotSetObject.run {
      config.notSetClass(oneofDescriptor, oneofClassName)
    }
    assertThat(notSetObject).generates(
      """
/**
 * Represents the absence of a value for my_oneof.
 */
object NotSetName : MyOneofClass<kotlin.Unit>(Unit, com.google.protobuf.kotlin.generator.Example3.ExampleMessage.MyOneofCase.MYONEOF_NOT_SET)
      """
    )
  }

  @Test
  fun orBuilderVal() {
    val declarations = generatorUnderTest.orBuilderVal.run {
      config.generate(oneofDescriptor, oneofClassName)
    }

    assertThat(declarations).generatesNoEnclosedMembers()
    assertThat(declarations).generatesTopLevel(
      """
      import $example3

      /**
       * The value of the oneof my_oneof.
       */
      val Example3.ExampleMessageOrBuilder.myOneofProperty: MyOneofClass<*>
        get() = MyOneofClass.readFrom(this)
    """
    )
  }

  @Test
  fun builderVar() {
    val declarations = generatorUnderTest.builderVar.run {
      config.generate(oneofDescriptor, oneofClassName)
    }

    assertThat(declarations).generatesNoEnclosedMembers()
    assertThat(declarations).generatesTopLevel(
      """
      import $example3

      /**
       * The value of the oneof my_oneof.
       */
      var Example3.ExampleMessage.Builder.myOneofProperty: MyOneofClass<*>
        get() = MyOneofClass.readFrom(this)
        set(newOneofParam) {
          newOneofParam.writeTo(this)
        }
    """
    )
  }

  @Test
  fun dslProperty() {
    val dslGenerator = generatorUnderTest.dslPropertyGenerator

    val dslSimpleName = ClassSimpleName("DslClass")
    val dslBuilder = TypeSpec.classBuilder(dslSimpleName)
    val dslCompanionBuilder = TypeSpec.companionObjectBuilder()

    dslGenerator.run {
      config.generate(
        component = OneofComponent(oneofDescriptor),
        dslType = UnqualifiedScope.nestedClass(dslSimpleName),
        extensionScope = UnqualifiedScope,
        enclosingScope = UnqualifiedScope,
        builderCode = CodeBlock.of("myBuilder"),
        dslBuilder = dslBuilder,
        companionBuilder = dslCompanionBuilder
      )
    }

    assertThat(dslCompanionBuilder.build()).generates("companion object")
    assertThat(dslBuilder.build()).generates(
      """
      class DslClass {
        /**
         * An object representing a value assigned to the oneof my_oneof.
         */
        var myOneofProperty: MyOneofOneofClass<*>
          get() = MyOneofOneofClass.readFrom(myBuilder)
          set(newValue) {
            newValue.writeTo(myBuilder)
          }
      }
    """
    )
  }

  @Test
  fun integrationTopLevel() {
    val actual = generatorUnderTest.run {
      config
        .componentExtensions(OneofComponent(oneofDescriptor), UnqualifiedScope, UnqualifiedScope)
    }
    val expectedTopLevel =
      Files.readAllLines(
        Paths.get(
          "src/test/java/com/google/protobuf/kotlin/generator",
          "Example3OneofClass.kt.expected.toplevel"
        )
      ).joinToString("\n")
    assertThat(actual).generatesTopLevel(expectedTopLevel)
  }

  @Test
  fun integrationEnclosed() {
    val actual = generatorUnderTest.run {
      config
        .componentExtensions(OneofComponent(oneofDescriptor), UnqualifiedScope, UnqualifiedScope)
    }
    val expectedEnclosed =
      Files.readAllLines(
        Paths.get(
          "src/test/java/com/google/protobuf/kotlin/generator",
          "Example3OneofClass.kt.expected.enclosed"
        )
      ).joinToString("\n")
    assertThat(actual).generatesEnclosed(expectedEnclosed)
  }
}
