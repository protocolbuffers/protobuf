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
import com.google.protobuf.kotlin.protoc.Declarations
import com.google.protobuf.kotlin.protoc.GeneratorConfig
import com.google.protobuf.kotlin.protoc.JavaPackagePolicy
import com.google.protobuf.kotlin.protoc.MemberSimpleName
import com.google.protobuf.kotlin.protoc.RepeatedFieldDescriptor
import com.google.protobuf.kotlin.protoc.UnqualifiedScope
import com.google.protobuf.kotlin.protoc.builder
import com.google.protobuf.kotlin.protoc.specialize
import com.google.protobuf.kotlin.protoc.testing.assertThat
import com.google.protobuf.kotlin.generator.Example3.ExampleMessage
import com.squareup.kotlinpoet.CodeBlock
import com.squareup.kotlinpoet.FunSpec
import org.junit.Test
import org.junit.runner.RunWith
import org.junit.runners.JUnit4

/** Tests for [RepeatedRepresentativeExtensionGenerator]. */
@RunWith(JUnit4::class)
class RepeatedRepresentativeExtensionGeneratorTest {
  private val javaPackagePolicy = JavaPackagePolicy.OPEN_SOURCE
  private val config = GeneratorConfig(javaPackagePolicy, aggressiveInlining = false)
  private val proxyDummyType = UnqualifiedScope.nestedClass(ClassSimpleName("Proxy"))

  private val repeatedFieldDescriptor: RepeatedFieldDescriptor =
    ExampleMessage.getDescriptor().findFieldByNumber(
      ExampleMessage.REPEATED_STRING_FIELD_FIELD_NUMBER
    ).specialize() as RepeatedFieldDescriptor

  private fun declarations(generator: RepeatedRepresentativeExtensionGenerator): Declarations {
    return with(config) {
      generator.run {
        generate(
          { FunSpec.builder(it).receiver(proxyDummyType) },
          repeatedFieldDescriptor,
          CodeBlock.of("myBuilder"),
          MemberSimpleName("repeatedFieldProperty")
        )
      }
    }
  }

  @Test
  fun setOperator() {
    val declarations = declarations(RepeatedRepresentativeExtensionGenerator.SetOperator)

    assertThat(declarations).generatesNoTopLevelMembers()
    assertThat(declarations).generatesEnclosed(
      """
      import kotlin.Int
      import kotlin.String

      /**
       * Sets the element of the repeated field repeated_string_field at a given index.
       *
       * For example, `repeatedFieldProperty[i] = value` sets the `i`th element of repeated_string_field
       * to `value`.
       */
      operator fun Proxy.set(index: Int, newValue: String) {
        myBuilder.setRepeatedStringField(index, newValue)
      }
    """
    )
  }

  @Test
  fun addElement() {
    val declarations = declarations(RepeatedRepresentativeExtensionGenerator.AddElement.add)

    assertThat(declarations).generatesNoTopLevelMembers()
    assertThat(declarations).generatesEnclosed(
      """
      import kotlin.String

      /**
       * Adds an element to the repeated field repeated_string_field.
       *
       * For example, `repeatedFieldProperty.add(value)` adds `value` to repeated_string_field.
       */
      fun Proxy.add(newValue: String) {
        myBuilder.addRepeatedStringField(newValue)
      }
    """
    )
  }

  @Test
  fun plusAssignElement() {
    val declarations = declarations(RepeatedRepresentativeExtensionGenerator.AddElement.plusAssign)

    assertThat(declarations).generatesNoTopLevelMembers()
    assertThat(declarations).generatesEnclosed(
      """
      import kotlin.String

      /**
       * Adds an element to the repeated field repeated_string_field.
       *
       * For example, `repeatedFieldProperty += value` adds `value` to repeated_string_field.
       */
      operator fun Proxy.plusAssign(newValue: String) {
        myBuilder.addRepeatedStringField(newValue)
      }
    """
    )
  }

  @Test
  fun addAllIterable() {
    val declarations = declarations(RepeatedRepresentativeExtensionGenerator.AddAllIterable.addAll)

    assertThat(declarations).generatesNoTopLevelMembers()
    assertThat(declarations).generatesEnclosed(
      """
      import kotlin.String
      import kotlin.collections.Iterable

      /**
       * Adds elements to the repeated field repeated_string_field.
       *
       * For example, `repeatedFieldProperty.addAll(values)` adds each of the elements of `values`, in
       * order, to repeated_string_field.
       */
      fun Proxy.addAll(newValues: Iterable<String>) {
        myBuilder.addAllRepeatedStringField(newValues)
      }
    """
    )
  }

  @Test
  fun plusAssignIterable() {
    val declarations =
      declarations(RepeatedRepresentativeExtensionGenerator.AddAllIterable.plusAssign)

    assertThat(declarations).generatesNoTopLevelMembers()
    assertThat(declarations).generatesEnclosed(
      """
      import kotlin.String
      import kotlin.collections.Iterable

      /**
       * Adds elements to the repeated field repeated_string_field.
       *
       * For example, `repeatedFieldProperty += values` adds each of the elements of `values`, in order,
       * to repeated_string_field.
       */
      operator fun Proxy.plusAssign(newValues: Iterable<String>) {
        myBuilder.addAllRepeatedStringField(newValues)
      }
    """
    )
  }

  @Test
  fun addAllVararg() {
    val declarations = declarations(RepeatedRepresentativeExtensionGenerator.AddAllVararg)

    assertThat(declarations).generatesNoTopLevelMembers()
    assertThat(declarations).generatesEnclosed(
      """
  import kotlin.String

  /**
   * Add some elements to the repeated field repeated_string_field.
   *
   * For example, `repeatedFieldProperty.addAll(v1, v2, v3)` adds `v1`, `v2`, and `v3`, in order, to
   * repeated_string_field.
   */
  fun Proxy.addAll(vararg newValues: String) {
    myBuilder.addAllRepeatedStringField(newValues.asList())
  }
    """
    )
  }

  @Test
  fun clear() {
    val declarations = declarations(RepeatedRepresentativeExtensionGenerator.Clear)

    assertThat(declarations).generatesNoTopLevelMembers()
    assertThat(declarations).generatesEnclosed(
      """
      /**
       * Removes all elements from the repeated field repeated_string_field.
       *
       * For example, `repeatedFieldProperty.clear()` deletes all elements from repeated_string_field.
       */
      fun Proxy.clear() {
        myBuilder.clearRepeatedStringField()
      }
    """
    )
  }
}
