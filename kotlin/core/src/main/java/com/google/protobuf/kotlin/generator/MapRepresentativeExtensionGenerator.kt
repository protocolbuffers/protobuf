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

import com.google.protobuf.kotlin.protoc.Declarations
import com.google.protobuf.kotlin.protoc.GeneratorConfig
import com.google.protobuf.kotlin.protoc.MapFieldDescriptor
import com.google.protobuf.kotlin.protoc.MemberSimpleName
import com.google.protobuf.kotlin.protoc.declarations
import com.google.protobuf.kotlin.protoc.of
import com.squareup.kotlinpoet.CodeBlock
import com.squareup.kotlinpoet.FunSpec
import com.squareup.kotlinpoet.KModifier
import com.squareup.kotlinpoet.ParameterSpec
import com.squareup.kotlinpoet.ParameterizedTypeName.Companion.parameterizedBy
import com.squareup.kotlinpoet.TypeName
import com.squareup.kotlinpoet.asTypeName

/** A generator for extension methods on a type representing a map field of a proto message. */
abstract class MapRepresentativeExtensionGenerator private constructor() {
  companion object {
    private val PAIR = Pair::class.asTypeName()
    private val ITERABLE = Iterable::class.asTypeName()
    private val MAP = Map::class.asTypeName()
  }

  /**
   * Generates an extension function that delegates its implementation to modifying the appropriate
   * repeated field in a backing builder.
   *
   * @param funSpec Creates a [FunSpec.Builder] for a method with the specified name, with an
   *        appropriate receiver type.
   * @param mapField Indicates which repeated field to modify in the backing builder.
   * @param builderCode Code to access the backing builder.
   */
  abstract fun GeneratorConfig.generate(
    funSpec: (MemberSimpleName) -> FunSpec.Builder,
    mapField: MapFieldDescriptor,
    builderCode: CodeBlock,
    proxyPropertyName: MemberSimpleName
  ): Declarations

  protected fun GeneratorConfig.keyParameter(field: MapFieldDescriptor): ParameterSpec =
    ParameterSpec.of("key", field.keyType())

  /** Generates a set operator to put an entry in the map field. */
  class Put private constructor(
    private val name: MemberSimpleName,
    /**
     * A CodeBlock template string that takes an %L parameter and generates code to put the pair
     * (k, v) into it.
     */
    private val putKV: String,
    private vararg val modifiers: KModifier
  ) : MapRepresentativeExtensionGenerator() {
    companion object {
      /** Generates a normal function named `put`. */
      val put = Put(MemberSimpleName("put"), "%L.put(k, v)")

      /** Generates the `set` operator (`map[key] = value`). */
      val setOperator = Put(MemberSimpleName.OPERATOR_SET, "%L[k] = v", KModifier.OPERATOR)
    }

    override fun GeneratorConfig.generate(
      funSpec: (MemberSimpleName) -> FunSpec.Builder,
      mapField: MapFieldDescriptor,
      builderCode: CodeBlock,
      proxyPropertyName: MemberSimpleName
    ): Declarations = declarations {
      val keyParameter = keyParameter(mapField)
      val newValueParameter = ParameterSpec.of("newValue", mapField.valueType())
      addFunction(
        funSpec(name)
          .addKdoc(
            """
            Puts the specified key-value pair into the map field %L, overwriting any previous
            value associated with the key.

            For example, `$putKV` maps the key `k` to the value `v` in %L.

            Calling this method with any receiver but [%L] may have undefined behavior.
            """.trimIndent(),
            mapField.fieldName,
            proxyPropertyName,
            mapField.fieldName,
            proxyPropertyName
          )
          .addModifiers(*modifiers)
          .addParameter(keyParameter)
          .addParameter(newValueParameter)
          .addStatement(
            "%L.%L(%N, %N)",
            builderCode,
            mapField.putterSimpleName,
            keyParameter,
            newValueParameter
          )
          .build()
      )
    }
  }

  /** Generates a function to remove a key from the map field. */
  class RemoveKey private constructor(
    private val name: MemberSimpleName,
    /**
     * A CodeBlock template string that takes an %L parameter and generates code to remove the key
     * k from it.
     */
    private val removeK: String,
    private vararg val modifiers: KModifier
  ) : MapRepresentativeExtensionGenerator() {
    companion object {
      /** Generates a normal method named `remove`. */
      val remove: RemoveKey = RemoveKey(MemberSimpleName("remove"), "%L.remove(k)")

      /** Generates a `-=` operator to remove a key. */
      val minusAssign: RemoveKey =
        RemoveKey(MemberSimpleName.OPERATOR_MINUS_ASSIGN, "%L -= k", KModifier.OPERATOR)
    }

    override fun GeneratorConfig.generate(
      funSpec: (MemberSimpleName) -> FunSpec.Builder,
      mapField: MapFieldDescriptor,
      builderCode: CodeBlock,
      proxyPropertyName: MemberSimpleName
    ): Declarations = declarations {
      val keyParameter = keyParameter(mapField)
      addFunction(
        funSpec(this@RemoveKey.name)
          .addKdoc(
            """
            Removes the entry for the specified key from the map field %L, if one exists.
            Otherwise, does nothing.

            For example, `$removeK` removes the key `k` from %L.

            Calling this method with any receiver but [%L] may have undefined behavior.
            """.trimIndent(),
            mapField.fieldName,
            proxyPropertyName,
            mapField.fieldName,
            proxyPropertyName
          )
          .addModifiers(*modifiers)
          .addParameter(keyParameter)
          .addStatement(
            "%L.%L(%N)",
            builderCode,
            mapField.removerSimpleName,
            keyParameter
          )
          .build()
      )
    }
  }

  /** Generates a function to remove zero or more keys from the map field. */
  class RemoveAll private constructor(
    private val name: MemberSimpleName,
    private val makeParameter: (keyType: TypeName) -> ParameterSpec,
    /**
     * A CodeBlock template string that takes an %L parameter and generates code to remove the keys
     * k1, k2, and k3 from it.
     */
    private val removeK123: String,
    private vararg val modifiers: KModifier
  ) : MapRepresentativeExtensionGenerator() {
    companion object {
      /** Generates a function named `removeAll` to remove an `Iterable<Key>`. */
      val removeAllIterable = RemoveAll(
        MemberSimpleName("removeAll"),
        { iterableKeysParam(it) },
        "%L.removeAll(listOf(k1, k2, k3))"
      )

      /** Generates a function named `removeAll` to remove a vararg of keys. */
      val removeAllVararg = RemoveAll(
        MemberSimpleName("removeAll"),
        { ParameterSpec.of("keys", it, KModifier.VARARG) },
        "%L.removeAll(k1, k2, k3)"
      )

      /** Generates a `-=` operator function to remove an `Iterable<Key>`. */
      val minusAssign = RemoveAll(
        MemberSimpleName.OPERATOR_MINUS_ASSIGN,
        { iterableKeysParam(it) },
        "%L -= listOf(k1, k2, k3)",
        KModifier.OPERATOR
      )

      private fun iterableKeysParam(keyType: TypeName) =
        ParameterSpec.of("keys", ITERABLE.parameterizedBy(keyType))
    }

    override fun GeneratorConfig.generate(
      funSpec: (MemberSimpleName) -> FunSpec.Builder,
      mapField: MapFieldDescriptor,
      builderCode: CodeBlock,
      proxyPropertyName: MemberSimpleName
    ): Declarations = declarations {
      val keysParameter = makeParameter(mapField.keyType())
      addFunction(
        funSpec(this@RemoveAll.name)
          .addModifiers(*modifiers)
          .addKdoc(
            """
            Removes the entries for each of the specified keys from the map field %L.  If there is
            no entry for a given key, it is skipped.

            For example, `$removeK123` removes entries for `k1`, `k2`, and `k3` from %L.

            Calling this method with any receiver but [%L] may have undefined behavior.
            """.trimIndent(),
            mapField.fieldName,
            proxyPropertyName,
            mapField.fieldName,
            proxyPropertyName
          )
          .addParameter(keysParameter)
          .beginControlFlow("for (value in %N)", keysParameter)
          .addStatement(
            "%L.%L(%N)",
            builderCode,
            mapField.removerSimpleName,
            keysParameter
          )
          .endControlFlow()
          .build()
      )
    }
  }

  /** Generates a function to put the contents of a `Map` into a map field. */
  class PutAllMap private constructor(
    private val name: MemberSimpleName,
    /**
     * A CodeBlock template string that takes an %L parameter and generates code to put the entries
     * (k1, v1) and (k2, v2) into it.
     */
    private val putAllKV12: String,
    private vararg val modifiers: KModifier
  ) : MapRepresentativeExtensionGenerator() {
    companion object {
      /** Generates a normal function named `putAll`. */
      val putAll: PutAllMap = PutAllMap(
        MemberSimpleName("putAll"),
        "%L.putAll(mapOf(k1 to v1, k2 to v2))"
      )

      /** Generates a `+=` operator function accepting a map. */
      val plusAssign: PutAllMap =
        PutAllMap(
          MemberSimpleName.OPERATOR_PLUS_ASSIGN,
          "%L += mapOf(k1 to v1, k2 to v2)",
          KModifier.OPERATOR
        )
    }

    override fun GeneratorConfig.generate(
      funSpec: (MemberSimpleName) -> FunSpec.Builder,
      mapField: MapFieldDescriptor,
      builderCode: CodeBlock,
      proxyPropertyName: MemberSimpleName
    ): Declarations = declarations {
      val putAllParameter =
        ParameterSpec.of(
          "map",
          MAP.parameterizedBy(mapField.keyType(), mapField.valueType())
        )
      addFunction(
        funSpec(this@PutAllMap.name)
          .addModifiers(*modifiers)
          .addKdoc(
            """
            Puts the entries in the given map into the map field %L.  If there is already an entry
            for a key in the map, it is overwritten.

            For example, `$putAllKV12` is equivalent to `%L[k1] = v1; %L[k2] = v2`.

            Calling this method with any receiver but [%L] may have undefined behavior.
            """.trimIndent(),
            mapField.fieldName,
            proxyPropertyName,
            proxyPropertyName,
            proxyPropertyName,
            proxyPropertyName
          )
          .addParameter(putAllParameter)
          .addStatement(
            "%L.%L(%N)",
            builderCode,
            mapField.putAllSimpleName,
            putAllParameter
          )
          .build()
      )
    }
  }

  /** Generates a function to put a collection of key-value pairs into a map field. */
  class PutAllPairs private constructor(
    val name: MemberSimpleName,
    val makeParameter: (TypeName, TypeName) -> ParameterSpec,
    /**
     * A CodeBlock template string that takes an %L parameter and generates code to put the entries
     * (k1, v1) and (k2, v2) into it.
     */
    private val putAllKV12: String,
    val modifiers: Array<KModifier> = arrayOf()
  ) : MapRepresentativeExtensionGenerator() {
    companion object {
      /** Generates a function named `putAll` accepting a vararg of key-value pairs. */
      val putAllVarargs: PutAllPairs = PutAllPairs(
        name = MemberSimpleName("putAll"),
        makeParameter = { keyTy, valueTy ->
          ParameterSpec
            .of("pairs", PAIR.parameterizedBy(keyTy, valueTy), KModifier.VARARG)
        },
        putAllKV12 = "%L.putAll(k1 to v1, k2 to v2)"
      )

      /** Generates a function named `putAll` accepting an `Iterable<Pair<Key, Value>>`. */
      val putAllIterable: PutAllPairs = PutAllPairs(
        name = MemberSimpleName("putAll"),
        makeParameter = { keyTy, valueTy ->
          ParameterSpec.of(
            "pairs",
            ITERABLE.parameterizedBy(PAIR.parameterizedBy(keyTy, valueTy))
          )
        },
        putAllKV12 = "%L.putAll(listOf(k1 to v1, k2 to v2))"
      )

      /** Generates a `+=` operator accepting an `Iterable<Pair<Key, Value>>`. */
      val plusAssignIterable: PutAllPairs = PutAllPairs(
        name = MemberSimpleName.OPERATOR_PLUS_ASSIGN,
        makeParameter = { keyTy, valueTy ->
          ParameterSpec.of(
            "pairs",
            ITERABLE.parameterizedBy(PAIR.parameterizedBy(keyTy, valueTy))
          )
        },
        putAllKV12 = "%L += listOf(k1 to v1, k2 to v2)",
        modifiers = arrayOf(KModifier.OPERATOR)
      )
    }

    override fun GeneratorConfig.generate(
      funSpec: (MemberSimpleName) -> FunSpec.Builder,
      mapField: MapFieldDescriptor,
      builderCode: CodeBlock,
      proxyPropertyName: MemberSimpleName
    ): Declarations = declarations {
      val parameter = makeParameter(mapField.keyType(), mapField.valueType())

      addFunction(
        funSpec(this@PutAllPairs.name)
          .addModifiers(*modifiers)
          .addParameter(parameter)
          .addKdoc(
            """
            Puts the specified key-value pairs into the map field %L.  If there is already an entry
            for a key in the map, it is overwritten.

            For example, `$putAllKV12` is equivalent to `%L[k1] = v1; %L[k2] = v2`.

            Calling this method with any receiver but [%L] may have undefined behavior.
            """.trimIndent(),
            mapField.fieldName,
            proxyPropertyName,
            proxyPropertyName,
            proxyPropertyName,
            proxyPropertyName
          )
          .beginControlFlow("for ((k, v) in %N)", parameter)
          .addStatement(
            "%L.%L(k, v)", builderCode, mapField.putterSimpleName
          )
          .endControlFlow()
          .build()
      )
    }
  }

  /** Generates a `clear` function for the map field. */
  object Clear : MapRepresentativeExtensionGenerator() {
    private val CLEAR_SIMPLE_NAME = MemberSimpleName("clear")

    override fun GeneratorConfig.generate(
      funSpec: (MemberSimpleName) -> FunSpec.Builder,
      mapField: MapFieldDescriptor,
      builderCode: CodeBlock,
      proxyPropertyName: MemberSimpleName
    ): Declarations = declarations {
      addFunction(
        funSpec(CLEAR_SIMPLE_NAME)
          .addKdoc(
            """
            Removes all entries from the map field %L.

            For example, `%L.clear()` deletes all entries previously part of %L.

            Calling this method with any receiver but [%L] may have undefined behavior.
            """.trimIndent(),
            mapField.fieldName,
            proxyPropertyName,
            mapField.fieldName,
            proxyPropertyName
          )
          .addStatement(
            "%L.%L()",
            builderCode,
            mapField.clearerSimpleName
          )
          .build()
      )
    }
  }
}
