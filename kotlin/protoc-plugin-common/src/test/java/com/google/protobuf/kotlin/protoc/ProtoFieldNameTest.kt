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
import com.google.protobuf.kotlin.protoc.testproto.Example3
import org.junit.Test
import org.junit.runner.RunWith
import org.junit.runners.JUnit4

/** Tests for [ProtoFieldName]. */
@RunWith(JUnit4::class)
class ProtoFieldNameTest {
  @Test
  fun propertySimpleName() {
    assertThat(ProtoFieldName("foo_bar_baz").propertySimpleName)
      .isEqualTo(MemberSimpleName("fooBarBaz"))
  }

  @Test
  fun propertySimpleNameCharactersFollowDigit() {
    assertThat(ProtoFieldName("has_underbar_preceeding_numeric_1foo").propertySimpleName)
      .isEqualTo(MemberSimpleName("hasUnderbarPreceedingNumeric1Foo"))
    assertThat(ProtoFieldName("has_underbar_preceeding_numeric_42bar").propertySimpleName)
      .isEqualTo(MemberSimpleName("hasUnderbarPreceedingNumeric42Bar"))
    assertThat(ProtoFieldName("has_underbar_preceeding_numeric_123foo42bar_baz").propertySimpleName)
      .isEqualTo(MemberSimpleName("hasUnderbarPreceedingNumeric123Foo42BarBaz"))
  }

  @Test
  fun toSimpleClassNameWithSuffix() {
    assertThat(ProtoFieldName("foo_bar_baz").toClassSimpleNameWithSuffix("Suffix"))
      .isEqualTo(ClassSimpleName("FooBarBazSuffix"))
    assertThat(ProtoFieldName("fooBar").toClassSimpleNameWithSuffix("Suffix"))
      .isEqualTo(ClassSimpleName("FooBarSuffix"))
  }

  @Test
  fun toEnumConstantName() {
    assertThat(ProtoFieldName("foo_bar_baz").toEnumConstantName())
      .isEqualTo(ConstantName("FOO_BAR_BAZ"))
  }

  @Test
  fun fieldDescriptorName() {
    val fieldDescriptor =
      Example3.ExampleMessage.getDescriptor().findFieldByName("string_oneof_option")
    assertThat(fieldDescriptor.fieldName)
      .isEqualTo(ProtoFieldName("string_oneof_option"))
  }

  @Test
  fun oneofName() {
    val oneofDescriptor =
      Example3.ExampleMessage
        .getDescriptor()
        .oneofs
        .find { it.name == "my_oneof" }!!
    assertThat(oneofDescriptor.oneofName)
      .isEqualTo(ProtoFieldName("my_oneof"))
  }

  @Test
  fun namingEdgeCases() {
    assertThat(ProtoFieldName("has_has_foo").propertySimpleName)
      .isEqualTo(MemberSimpleName("hasHasFoo"))
    assertThat(ProtoFieldName("enum_testEnum").propertySimpleName)
      .isEqualTo(MemberSimpleName("enumTestEnum"))
    assertThat(ProtoFieldName("Enum_test").propertySimpleName)
      .isEqualTo(MemberSimpleName("enumTest"))
  }

  @Test
  fun javaSimpleName() {
    assertThat(ProtoFieldName("has_has_foo").javaSimpleName)
      .isEqualTo(MemberSimpleName("hasHasFoo"))
    assertThat(ProtoFieldName("class").javaSimpleName).isEqualTo(MemberSimpleName("class_"))
    assertThat(ProtoFieldName("cached_size").javaSimpleName)
      .isEqualTo(MemberSimpleName("cachedSize_"))
    assertThat(ProtoFieldName("serialized_size").javaSimpleName)
      .isEqualTo(MemberSimpleName("serializedSize_"))
  }
}
