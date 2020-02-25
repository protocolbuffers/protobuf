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

import com.google.common.truth.Truth.assertThat
import com.google.protobuf.Descriptors.FileDescriptor
import com.google.protobuf.kotlin.protoc.testproto.Example3
import com.google.protobuf.kotlin.protoc.testproto.HasOuterClassNameConflictOuterClass
import com.google.protobuf.kotlin.protoc.testproto.MyExplicitOuterClassName
import com.squareup.kotlinpoet.BOOLEAN
import com.squareup.kotlinpoet.FileSpec
import com.squareup.kotlinpoet.INT
import com.squareup.kotlinpoet.MemberName.Companion.member
import com.squareup.kotlinpoet.ParameterSpec
import com.squareup.kotlinpoet.PropertySpec
import com.squareup.kotlinpoet.asClassName
import com.squareup.kotlinpoet.asTypeName
import kotlin.reflect.KClass
import kotlin.reflect.full.memberFunctions
import org.junit.Test
import org.junit.runner.RunWith
import org.junit.runners.JUnit4
import testing.ImplicitJavaPackage

/** Tests for [GeneratorConfig]. */
@RunWith(JUnit4::class)
class GeneratorConfigTest {
  private val javaPackagePolicy = JavaPackagePolicy.OPEN_SOURCE
  private val defaultConfig = GeneratorConfig(javaPackagePolicy, false)

  private fun generateFile(block: Declarations.Builder.() -> Unit): String {
    return FileSpec.builder("com.google", "FooBar.kt")
      .apply {
        declarations(block).writeAllAtTopLevel(this)
      }
      .build()
      .toString()
      .trim()
  }

  @Test
  fun funSpecBuilder() {
    with(GeneratorConfig(javaPackagePolicy, aggressiveInlining = false)) {
      assertThat(
        generateFile {
          addFunction(funSpecBuilder(MemberSimpleName("fooBar")).build())
        }
      ).isEqualTo(
        """
        package com.google

        fun fooBar() {
        }
        """.trimIndent()
      )
    }
    with(GeneratorConfig(javaPackagePolicy, aggressiveInlining = true)) {
      assertThat(
        generateFile {
          addFunction(funSpecBuilder(MemberSimpleName("fooBar")).build())
        }
      ).isEqualTo(
        """
        package com.google

        inline fun fooBar() {
        }
        """.trimIndent()
      )
    }
  }

  @Test
  fun getterBuilder() {
    with(GeneratorConfig(javaPackagePolicy, aggressiveInlining = false)) {
      assertThat(
        generateFile {
          addProperty(
            PropertySpec.builder("someProp", INT)
              .getter(getterBuilder().addStatement("return 1").build())
              .build()
          )
        }
      ).isEqualTo(
        """
        package com.google

        import kotlin.Int

        val someProp: Int
          get() = 1
        """.trimIndent()
      )
    }

    with(GeneratorConfig(javaPackagePolicy, aggressiveInlining = true)) {
      assertThat(
        generateFile {
          addProperty(
            PropertySpec.builder("someProp", INT)
              .getter(getterBuilder().addStatement("return 1").build())
              .build()
          )
        }
      ).isEqualTo(
        """
        package com.google

        import kotlin.Int

        val someProp: Int
          inline get() = 1
        """.trimIndent()
      )
    }
  }

  @Test
  fun setterBuilder() {
    val param = ParameterSpec.builder("newValue", INT).build()
    with(GeneratorConfig(javaPackagePolicy, aggressiveInlining = false)) {
      assertThat(
        generateFile {
          addProperty(
            PropertySpec.builder("someProp", INT)
              .mutable(true)
              .setter(setterBuilder().addParameter(param).build())
              .build()
          )
        }
      ).isEqualTo(
        """
        package com.google

        import kotlin.Int

        var someProp: Int
          set(newValue) {
          }
        """.trimIndent()
      )
    }

    with(GeneratorConfig(javaPackagePolicy, aggressiveInlining = true)) {
      assertThat(
        generateFile {
          addProperty(
            PropertySpec.builder("someProp", INT)
              .mutable(true)
              .setter(setterBuilder().addParameter(param).build())
              .build()
          )
        }
      ).isEqualTo(
        """
        package com.google

        import kotlin.Int

        var someProp: Int
          inline set(newValue) {
          }
        """.trimIndent()
      )
    }
  }

  private val fileDescriptorToTopLevelClass: List<Pair<FileDescriptor, KClass<out Any>>> =
    listOf(
      Example3.getDescriptor() to Example3::class,
      MyExplicitOuterClassName.getDescriptor() to MyExplicitOuterClassName::class,
      HasOuterClassNameConflictOuterClass.getDescriptor() to
        HasOuterClassNameConflictOuterClass::class,
      ImplicitJavaPackage.getDescriptor() to ImplicitJavaPackage::class
    )

  @Test
  fun javaPackage() {
    with(defaultConfig) {
      for ((descriptor, clazz) in fileDescriptorToTopLevelClass) {
        assertThat(javaPackage(descriptor)).isEqualTo(PackageScope(clazz.java.`package`.name))
      }
    }
  }

  @Test
  fun outerClass() {
    with(defaultConfig) {
      for ((descriptor, clazz) in fileDescriptorToTopLevelClass) {
        assertThat(descriptor.outerClass()).isEqualTo(clazz.asClassName())
      }
    }
  }

  @Test
  fun enumClass() {
    with(defaultConfig) {
      assertThat(Example3.ExampleEnum.getDescriptor().enumClass())
        .isEqualTo(Example3.ExampleEnum::class.asClassName())
    }
  }

  @Test
  fun messageClass() {
    with(defaultConfig) {
      assertThat(Example3.ExampleMessage.getDescriptor().messageClass())
        .isEqualTo(Example3.ExampleMessage::class.asClassName())
    }
  }

  @Test
  fun builderClass() {
    with(defaultConfig) {
      assertThat(Example3.ExampleMessage.getDescriptor().builderClass())
        .isEqualTo(Example3.ExampleMessage.Builder::class.asClassName())
    }
  }

  @Test
  fun orBuilderClass() {
    with(defaultConfig) {
      assertThat(Example3.ExampleMessage.getDescriptor().orBuilderClass())
        .isEqualTo(Example3.ExampleMessageOrBuilder::class.asClassName())
    }
  }

  @Test
  fun singletonFieldTypeScalar() {
    val fieldDescriptor = Example3.ExampleMessage.getDescriptor().findFieldByNumber(
      Example3.ExampleMessage.INT32_FIELD_FIELD_NUMBER
    ).specialize() as SingletonFieldDescriptor
    with(defaultConfig) {
      assertThat(fieldDescriptor.type()).isEqualTo(Int::class.asClassName())
    }
  }

  @Test
  fun singletonFieldTypeMessage() {
    val fieldDescriptor = Example3.ExampleMessage.getDescriptor().findFieldByNumber(
      Example3.ExampleMessage.SUB_MESSAGE_FIELD_FIELD_NUMBER
    ).specialize() as SingletonFieldDescriptor
    with(defaultConfig) {
      assertThat(fieldDescriptor.type())
        .isEqualTo(Example3.ExampleMessage.SubMessage::class.asClassName())
    }
  }

  @Test
  fun singletonFieldProperty() {
    val fieldDescriptor = Example3.ExampleMessage.getDescriptor().findFieldByNumber(
      Example3.ExampleMessage.INT32_FIELD_FIELD_NUMBER
    ).specialize() as SingletonFieldDescriptor
    with(defaultConfig) {
      // demonstrate the property with the name
      assertThat(Example3.ExampleMessage.getDefaultInstance().int32Field).isEqualTo(0)
      assertThat(fieldDescriptor.property().name).isEqualTo("int32Field")
      assertThat(fieldDescriptor.property().receiverType)
        .isEqualTo(Example3.ExampleMessageOrBuilder::class.asClassName())
      assertThat(fieldDescriptor.property().type).isEqualTo(INT)
    }
  }

  @Test
  fun singletonFieldHazzerAbsent() {
    val fieldDescriptor = Example3.ExampleMessage.getDescriptor().findFieldByNumber(
      Example3.ExampleMessage.INT32_FIELD_FIELD_NUMBER
    ).specialize() as SingletonFieldDescriptor
    with(defaultConfig) {
      assertThat(fieldDescriptor.hazzer()).isNull()
      assertThat(Example3.ExampleMessage::class.memberFunctions.find { it.name == "hasInt32Field" })
        .isNull()
    }
  }

  @Test
  fun singletonFieldHazzerPresent() {
    val fieldDescriptor = Example3.ExampleMessage.getDescriptor().findFieldByNumber(
      Example3.ExampleMessage.SUB_MESSAGE_FIELD_FIELD_NUMBER
    ).specialize() as SingletonFieldDescriptor
    with(defaultConfig) {
      // demonstrate the hazzer with the correct name
      assertThat(Example3.ExampleMessage.getDefaultInstance().hasSubMessageField()).isFalse()
      assertThat(fieldDescriptor.hazzer()!!.name).isEqualTo("hasSubMessageField")
      assertThat(fieldDescriptor.hazzer()!!.receiverType)
        .isEqualTo(Example3.ExampleMessageOrBuilder::class.asTypeName())
      assertThat(fieldDescriptor.hazzer()!!.returnType).isEqualTo(BOOLEAN)
    }
  }

  @Test
  fun repeatedFieldElementType() {
    val fieldDescriptor = Example3.ExampleMessage.getDescriptor().findFieldByNumber(
      Example3.ExampleMessage.REPEATED_STRING_FIELD_FIELD_NUMBER
    ).specialize() as RepeatedFieldDescriptor
    with(defaultConfig) {
      assertThat(fieldDescriptor.elementType()).isEqualTo(String::class.asClassName())
    }
  }

  @Test
  fun mapFieldKeyType() {
    val fieldDescriptor = Example3.ExampleMessage.getDescriptor().findFieldByNumber(
      Example3.ExampleMessage.MAP_FIELD_FIELD_NUMBER
    ).specialize() as MapFieldDescriptor
    with(defaultConfig) {
      assertThat(fieldDescriptor.keyType()).isEqualTo(Long::class.asClassName())
    }
  }

  @Test
  fun mapFieldValueType() {
    val fieldDescriptor = Example3.ExampleMessage.getDescriptor().findFieldByNumber(
      Example3.ExampleMessage.MAP_FIELD_FIELD_NUMBER
    ).specialize() as MapFieldDescriptor
    with(defaultConfig) {
      assertThat(fieldDescriptor.valueType())
        .isEqualTo(Example3.ExampleMessage.SubMessage::class.asClassName())
    }
  }

  @Test
  fun oneofCaseEnum() {
    val oneofDescriptor = Example3.ExampleMessage.getDescriptor().oneofs.single()
    with(defaultConfig) {
      assertThat(oneofDescriptor.caseEnum())
        .isEqualTo(Example3.ExampleMessage.MyOneofCase::class.asClassName())
    }
  }

  @Test
  fun oneofCaseProperty() {
    val oneofDescriptor = Example3.ExampleMessage.getDescriptor().oneofs.single()
    with(defaultConfig) {
      assertThat(Example3.ExampleMessage.getDefaultInstance().myOneofCase)
        .isEqualTo(Example3.ExampleMessage.MyOneofCase.MYONEOF_NOT_SET)
      assertThat(oneofDescriptor.caseProperty().name).isEqualTo("myOneofCase")
      assertThat(oneofDescriptor.caseProperty().type)
        .isEqualTo(Example3.ExampleMessage.MyOneofCase::class.asTypeName())
      assertThat(oneofDescriptor.caseProperty().receiverType)
        .isEqualTo(Example3.ExampleMessageOrBuilder::class.asTypeName())
    }
  }

  @Test
  fun oneofNotSet() {
    val oneofDescriptor = Example3.ExampleMessage.getDescriptor().oneofs.single()
    with(defaultConfig) {
      assertThat(oneofDescriptor.notSet())
        .isEqualTo(
          Example3.ExampleMessage.MyOneofCase::class
            .member(Example3.ExampleMessage.MyOneofCase.MYONEOF_NOT_SET.name)
        )
    }
  }

  @Test
  fun oneofOptionCaseEnum() {
    val fieldDescriptor = Example3.ExampleMessage.getDescriptor().findFieldByNumber(
      Example3.ExampleMessage.STRING_ONEOF_OPTION_FIELD_NUMBER
    ).specialize() as OneofOptionFieldDescriptor
    with(defaultConfig) {
      assertThat(fieldDescriptor.caseEnumMember())
        .isEqualTo(
          Example3.ExampleMessage.MyOneofCase::class
            .member(Example3.ExampleMessage.MyOneofCase.STRING_ONEOF_OPTION.name)
        )
    }
  }
}
