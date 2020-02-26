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

package com.google.protobuf.kotlin.protoc

import com.google.protobuf.DescriptorProtos.FieldOptions
import com.google.protobuf.Descriptors.Descriptor
import com.google.protobuf.Descriptors.FieldDescriptor
import com.google.protobuf.Descriptors.FieldDescriptor.JavaType.MESSAGE
import com.google.protobuf.Descriptors.FileDescriptor.Syntax.PROTO2
import com.google.protobuf.Descriptors.OneofDescriptor

/** A part of a message definition that might have extensions generated for it. */
sealed class MessageComponent {
  abstract val containingType: Descriptor
}

/** A oneof descriptor as a [MessageComponent]. */
class OneofComponent(val descriptor: OneofDescriptor) : MessageComponent() {
  override val containingType: Descriptor
    get() = descriptor.containingType
}

/** A field descriptor, narrowed to a category that might have specialized generation code. */
sealed class SpecializedFieldDescriptor(
  val descriptor: FieldDescriptor
) : MessageComponent() {
  companion object {
    private const val CLEAR_PREFIX = "clear"
  }

  val fieldName: ProtoFieldName
    get() = descriptor.fieldName

  /** The name of the field as a property, e.g. `myField`. */
  val propertySimpleName: MemberSimpleName
    get() = fieldName.propertySimpleName

  val fieldOptions: FieldOptions
    get() = descriptor.options

  val isDeprecated: Boolean
    get() = fieldOptions.deprecated

  override val containingType: Descriptor
    get() = descriptor.containingType

  val javaSimpleName: MemberSimpleName
    get() = fieldName.javaSimpleName

  val clearerSimpleName: MemberSimpleName
    get() = javaSimpleName.withPrefix(CLEAR_PREFIX)

  override fun toString(): String = descriptor.toString()
}

/** Specializes a [FieldDescriptor] to a [SpecializedFieldDescriptor]. */
fun FieldDescriptor.specialize(): SpecializedFieldDescriptor = when {
  isMapField -> MapFieldDescriptor(this)
  isRepeated -> RepeatedFieldDescriptor(this)
  containingOneof != null -> OneofOptionFieldDescriptor(this)
  else -> SingletonFieldDescriptor(this)
}

/** Descriptor for a map field. */
class MapFieldDescriptor(
  descriptor: FieldDescriptor
) : SpecializedFieldDescriptor(descriptor) {
  init {
    check(descriptor.isMapField) { "Expected a map field but got $descriptor" }
  }

  companion object {
    private const val GET_MAP_PREFIX = "get"
    private const val GET_MAP_SUFFIX = "Map"
    private const val PUT_PREFIX = "put"
    private const val REMOVE_PREFIX = "remove"
    private const val PUT_ALL_PREFIX = "putAll"
  }

  /** Descriptor for the map key. */
  val keyFieldDescriptor: FieldDescriptor
    get() =
      descriptor.messageType.findFieldByNumber(1)
        ?: throw IllegalStateException("Could not find key field for map field $descriptor")

  /** Descriptor for the map value. */
  val valueFieldDescriptor: FieldDescriptor
    get() =
      descriptor.messageType.findFieldByNumber(2)
        ?: throw IllegalStateException("Could not find value field for map field $descriptor")

  val mapGetterSimpleName: MemberSimpleName
    get() = javaSimpleName.withPrefix(GET_MAP_PREFIX).withSuffix(GET_MAP_SUFFIX)

  val putterSimpleName: MemberSimpleName
    get() = javaSimpleName.withPrefix(PUT_PREFIX)

  val removerSimpleName: MemberSimpleName
    get() = javaSimpleName.withPrefix(REMOVE_PREFIX)

  val putAllSimpleName: MemberSimpleName
    get() = javaSimpleName.withPrefix(PUT_ALL_PREFIX)
}

/** Descriptor for a repeated field. */
class RepeatedFieldDescriptor(
  descriptor: FieldDescriptor
) : SpecializedFieldDescriptor(descriptor) {
  init {
    check(descriptor.isRepeated) { "Expected a repeated field but got $descriptor" }
    check(!descriptor.isMapField) { "Expected a non-map field but got $descriptor" }
  }

  companion object {
    private const val GET_PREFIX = "get"
    private const val GET_SUFFIX = "List"
    private const val ADD_PREFIX = "add"
    private const val ADD_ALL_PREFIX = "addAll"
    private const val SET_PREFIX = "set"
  }

  fun listGetterSimpleName(): MemberSimpleName =
    javaSimpleName.withPrefix(GET_PREFIX).withSuffix(GET_SUFFIX)

  val adderSimpleName: MemberSimpleName
    get() = javaSimpleName.withPrefix(ADD_PREFIX)

  val addAllSimpleName: MemberSimpleName
    get() = javaSimpleName.withPrefix(ADD_ALL_PREFIX)

  val setterSimpleName: MemberSimpleName
    get() = javaSimpleName.withPrefix(SET_PREFIX)
}

/** Descriptor for a non-map, non-repeated field. */
open class SingletonFieldDescriptor(
  descriptor: FieldDescriptor
) : SpecializedFieldDescriptor(descriptor) {
  init {
    check(!descriptor.isMapField) { "Expected a non-map field but got $descriptor" }
    check(!descriptor.isRepeated) { "Expected a non-repeated field but got $descriptor" }
  }

  companion object {
    private const val GET_PREFIX = "get"
    private const val SET_PREFIX = "set"
    private const val HAS_PREFIX = "has"
  }

  /** True if the Java API for this field includes a hazzer. */
  fun hasHazzer(): Boolean = isMessage() || descriptor.file.syntax == PROTO2

  /** True if the type of this field is another proto message. */
  fun isMessage(): Boolean = descriptor.javaType == MESSAGE

  /** True if this field is required. */
  fun isRequired(): Boolean = descriptor.isRequired

  val getterSimpleName: MemberSimpleName
    get() = javaSimpleName.withPrefix(GET_PREFIX)

  val setterSimpleName: MemberSimpleName
    get() = javaSimpleName.withPrefix(SET_PREFIX)

  val hazzerSimpleName: MemberSimpleName
    get() = javaSimpleName.withPrefix(HAS_PREFIX)
}

/** Descriptor for a field that is an option for a oneof. */
class OneofOptionFieldDescriptor(
  descriptor: FieldDescriptor
) : SingletonFieldDescriptor(descriptor) {
  init {
    check(descriptor.containingOneof != null)
  }

  val oneof: OneofDescriptor
    get() = descriptor.containingOneof

  val enumConstantName: ConstantName
    get() = descriptor.fieldName.toEnumConstantName()
}

/** Descriptors for the options of this oneof. */
fun OneofDescriptor.options(): List<OneofOptionFieldDescriptor> =
  fields.map(::OneofOptionFieldDescriptor)
