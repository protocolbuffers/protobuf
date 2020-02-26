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
import com.google.protobuf.kotlin.protoc.MapFieldDescriptor
import com.google.protobuf.kotlin.protoc.MemberSimpleName
import com.google.protobuf.kotlin.protoc.UnqualifiedScope
import com.google.protobuf.kotlin.protoc.builder
import com.google.protobuf.kotlin.protoc.specialize
import com.google.protobuf.kotlin.protoc.testing.assertThat
import com.google.protobuf.kotlin.generator.Example3
import com.squareup.kotlinpoet.CodeBlock
import com.squareup.kotlinpoet.FunSpec
import org.junit.Test
import org.junit.runner.RunWith
import org.junit.runners.JUnit4

/** Tests for [MapRepresentativeExtensionGenerator]. */
@RunWith(JUnit4::class)
class MapRepresentativeExtensionGeneratorTest {
  private val javaPackagePolicy = JavaPackagePolicy.OPEN_SOURCE
  private val config = GeneratorConfig(javaPackagePolicy, aggressiveInlining = false)
  private val proxyDummyType = UnqualifiedScope.nestedClass(ClassSimpleName("Proxy"))

  private val mapFieldDescriptor: MapFieldDescriptor =
    Example3.ExampleMessage.getDescriptor().findFieldByNumber(
      Example3.ExampleMessage.MAP_FIELD_FIELD_NUMBER
    ).specialize() as MapFieldDescriptor

  private fun declarations(generator: MapRepresentativeExtensionGenerator): Declarations {
    return with(config) {
      generator.run {
        generate(
          { FunSpec.builder(it).receiver(proxyDummyType) },
          mapFieldDescriptor,
          CodeBlock.of("myBuilder"),
          MemberSimpleName("mapProxy")
        )
      }
    }
  }

  @Test
  fun put() {
    val declarations = declarations(MapRepresentativeExtensionGenerator.Put.put)

    assertThat(declarations).generatesNoTopLevelMembers()
    assertThat(declarations).generatesEnclosed(
      """
      import com.google.protobuf.kotlin.generator.Example3
      import kotlin.Long

      /**
       * Puts the specified key-value pair into the map field map_field, overwriting any previous
       * value associated with the key.
       *
       * For example, `mapProxy.put(k, v)` maps the key `k` to the value `v` in map_field.
       *
       * Calling this method with any receiver but [mapProxy] may have undefined behavior.
       */
      fun Proxy.put(key: Long, newValue: Example3.ExampleMessage.SubMessage) {
        myBuilder.putMapField(key, newValue)
      }
    """
    )
  }

  @Test
  fun setOperator() {
    val declarations = declarations(MapRepresentativeExtensionGenerator.Put.setOperator)

    assertThat(declarations).generatesNoTopLevelMembers()
    assertThat(declarations).generatesEnclosed(
      """
      import com.google.protobuf.kotlin.generator.Example3
      import kotlin.Long

      /**
       * Puts the specified key-value pair into the map field map_field, overwriting any previous
       * value associated with the key.
       *
       * For example, `mapProxy[k] = v` maps the key `k` to the value `v` in map_field.
       *
       * Calling this method with any receiver but [mapProxy] may have undefined behavior.
       */
      operator fun Proxy.set(key: Long, newValue: Example3.ExampleMessage.SubMessage) {
        myBuilder.putMapField(key, newValue)
      }
    """
    )
  }

  @Test
  fun removeKey() {
    val declarations = declarations(MapRepresentativeExtensionGenerator.RemoveKey.remove)

    assertThat(declarations).generatesNoTopLevelMembers()
    assertThat(declarations).generatesEnclosed(
      """
      import kotlin.Long

      /**
       * Removes the entry for the specified key from the map field map_field, if one exists.
       * Otherwise, does nothing.
       *
       * For example, `mapProxy.remove(k)` removes the key `k` from map_field.
       *
       * Calling this method with any receiver but [mapProxy] may have undefined behavior.
       */
      fun Proxy.remove(key: Long) {
        myBuilder.removeMapField(key)
      }
    """
    )
  }

  @Test
  fun minusAssignKey() {
    val declarations = declarations(MapRepresentativeExtensionGenerator.RemoveKey.minusAssign)

    assertThat(declarations).generatesNoTopLevelMembers()
    assertThat(declarations).generatesEnclosed(
      """
      import kotlin.Long

      /**
       * Removes the entry for the specified key from the map field map_field, if one exists.
       * Otherwise, does nothing.
       *
       * For example, `mapProxy -= k` removes the key `k` from map_field.
       *
       * Calling this method with any receiver but [mapProxy] may have undefined behavior.
       */
      operator fun Proxy.minusAssign(key: Long) {
        myBuilder.removeMapField(key)
      }
    """
    )
  }

  @Test
  fun removeAllIterable() {
    val declarations = declarations(MapRepresentativeExtensionGenerator.RemoveAll.removeAllIterable)

    assertThat(declarations).generatesNoTopLevelMembers()
    assertThat(declarations).generatesEnclosed(
      """
      import kotlin.Long
      import kotlin.collections.Iterable

      /**
       * Removes the entries for each of the specified keys from the map field map_field.  If there is
       * no entry for a given key, it is skipped.
       *
       * For example, `mapProxy.removeAll(listOf(k1, k2, k3))` removes entries for `k1`, `k2`, and `k3`
       * from map_field.
       *
       * Calling this method with any receiver but [mapProxy] may have undefined behavior.
       */
      fun Proxy.removeAll(keys: Iterable<Long>) {
        for (value in keys) {
          myBuilder.removeMapField(keys)
        }
      }
    """
    )
  }

  @Test
  fun removeAllVararg() {
    val declarations = declarations(MapRepresentativeExtensionGenerator.RemoveAll.removeAllVararg)

    assertThat(declarations).generatesNoTopLevelMembers()
    assertThat(declarations).generatesEnclosed(
      """
      import kotlin.Long

      /**
       * Removes the entries for each of the specified keys from the map field map_field.  If there is
       * no entry for a given key, it is skipped.
       *
       * For example, `mapProxy.removeAll(k1, k2, k3)` removes entries for `k1`, `k2`, and `k3` from
       * map_field.
       *
       * Calling this method with any receiver but [mapProxy] may have undefined behavior.
       */
      fun Proxy.removeAll(vararg keys: Long) {
        for (value in keys) {
          myBuilder.removeMapField(keys)
        }
      }
    """
    )
  }

  @Test
  fun minusAssignIterable() {
    val declarations = declarations(MapRepresentativeExtensionGenerator.RemoveAll.minusAssign)

    assertThat(declarations).generatesNoTopLevelMembers()
    assertThat(declarations).generatesEnclosed(
      """
      import kotlin.Long
      import kotlin.collections.Iterable

      /**
       * Removes the entries for each of the specified keys from the map field map_field.  If there is
       * no entry for a given key, it is skipped.
       *
       * For example, `mapProxy -= listOf(k1, k2, k3)` removes entries for `k1`, `k2`, and `k3` from
       * map_field.
       *
       * Calling this method with any receiver but [mapProxy] may have undefined behavior.
       */
      operator fun Proxy.minusAssign(keys: Iterable<Long>) {
        for (value in keys) {
          myBuilder.removeMapField(keys)
        }
      }
    """
    )
  }

  @Test
  fun putAllMap() {
    val declarations = declarations(MapRepresentativeExtensionGenerator.PutAllMap.putAll)

    assertThat(declarations).generatesNoTopLevelMembers()
    assertThat(declarations).generatesEnclosed(
      """
      import com.google.protobuf.kotlin.generator.Example3
      import kotlin.Long
      import kotlin.collections.Map

      /**
       * Puts the entries in the given map into the map field map_field.  If there is already an entry
       * for a key in the map, it is overwritten.
       *
       * For example, `mapProxy.putAll(mapOf(k1 to v1, k2 to v2))` is equivalent to `mapProxy[k1] = v1;
       * mapProxy[k2] = v2`.
       *
       * Calling this method with any receiver but [mapProxy] may have undefined behavior.
       */
      fun Proxy.putAll(map: Map<Long, Example3.ExampleMessage.SubMessage>) {
        myBuilder.putAllMapField(map)
      }
    """
    )
  }

  @Test
  fun plusAssignMap() {
    val declarations = declarations(MapRepresentativeExtensionGenerator.PutAllMap.plusAssign)

    assertThat(declarations).generatesNoTopLevelMembers()
    assertThat(declarations).generatesEnclosed(
      """
      import com.google.protobuf.kotlin.generator.Example3
      import kotlin.Long
      import kotlin.collections.Map

      /**
       * Puts the entries in the given map into the map field map_field.  If there is already an entry
       * for a key in the map, it is overwritten.
       *
       * For example, `mapProxy += mapOf(k1 to v1, k2 to v2)` is equivalent to `mapProxy[k1] = v1;
       * mapProxy[k2] = v2`.
       *
       * Calling this method with any receiver but [mapProxy] may have undefined behavior.
       */
      operator fun Proxy.plusAssign(map: Map<Long, Example3.ExampleMessage.SubMessage>) {
        myBuilder.putAllMapField(map)
      }
    """
    )
  }

  @Test
  fun putAllVarargPairs() {
    val declarations = declarations(MapRepresentativeExtensionGenerator.PutAllPairs.putAllVarargs)

    assertThat(declarations).generatesNoTopLevelMembers()
    assertThat(declarations).generatesEnclosed(
      """
      import com.google.protobuf.kotlin.generator.Example3
      import kotlin.Long
      import kotlin.Pair

      /**
       * Puts the specified key-value pairs into the map field map_field.  If there is already an entry
       * for a key in the map, it is overwritten.
       *
       * For example, `mapProxy.putAll(k1 to v1, k2 to v2)` is equivalent to `mapProxy[k1] = v1;
       * mapProxy[k2] = v2`.
       *
       * Calling this method with any receiver but [mapProxy] may have undefined behavior.
       */
      fun Proxy.putAll(vararg pairs: Pair<Long, Example3.ExampleMessage.SubMessage>) {
        for ((k, v) in pairs) {
          myBuilder.putMapField(k, v)
        }
      }
      """
    )
  }

  @Test
  fun putAllIterablePairs() {
    val declarations = declarations(MapRepresentativeExtensionGenerator.PutAllPairs.putAllIterable)

    assertThat(declarations).generatesNoTopLevelMembers()
    assertThat(declarations).generatesEnclosed(
      """
      import com.google.protobuf.kotlin.generator.Example3
      import kotlin.Long
      import kotlin.Pair
      import kotlin.collections.Iterable

      /**
       * Puts the specified key-value pairs into the map field map_field.  If there is already an entry
       * for a key in the map, it is overwritten.
       *
       * For example, `mapProxy.putAll(listOf(k1 to v1, k2 to v2))` is equivalent to `mapProxy[k1] = v1;
       * mapProxy[k2] = v2`.
       *
       * Calling this method with any receiver but [mapProxy] may have undefined behavior.
       */
      fun Proxy.putAll(pairs: Iterable<Pair<Long, Example3.ExampleMessage.SubMessage>>) {
        for ((k, v) in pairs) {
          myBuilder.putMapField(k, v)
        }
      }
      """
    )
  }

  @Test
  fun plusAssignIterablePairs() {
    val declarations =
      declarations(MapRepresentativeExtensionGenerator.PutAllPairs.plusAssignIterable)

    assertThat(declarations).generatesNoTopLevelMembers()
    assertThat(declarations).generatesEnclosed(
      """
      import com.google.protobuf.kotlin.generator.Example3
      import kotlin.Long
      import kotlin.Pair
      import kotlin.collections.Iterable

      /**
       * Puts the specified key-value pairs into the map field map_field.  If there is already an entry
       * for a key in the map, it is overwritten.
       *
       * For example, `mapProxy += listOf(k1 to v1, k2 to v2)` is equivalent to `mapProxy[k1] = v1;
       * mapProxy[k2] = v2`.
       *
       * Calling this method with any receiver but [mapProxy] may have undefined behavior.
       */
      operator fun Proxy.plusAssign(pairs: Iterable<Pair<Long, Example3.ExampleMessage.SubMessage>>) {
        for ((k, v) in pairs) {
          myBuilder.putMapField(k, v)
        }
      }
      """
    )
  }

  @Test
  fun clear() {
    val declarations = declarations(MapRepresentativeExtensionGenerator.Clear)

    assertThat(declarations).generatesNoTopLevelMembers()
    assertThat(declarations).generatesEnclosed(
      """
      /**
       * Removes all entries from the map field map_field.
       *
       * For example, `mapProxy.clear()` deletes all entries previously part of map_field.
       *
       * Calling this method with any receiver but [mapProxy] may have undefined behavior.
       */
      fun Proxy.clear() {
        myBuilder.clearMapField()
      }
    """
    )
  }
}
