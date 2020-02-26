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
import com.google.protobuf.kotlin.protoc.testproto.Example3.ExampleEnum
import com.google.protobuf.kotlin.protoc.testproto.Example3.ExampleMessage
import com.google.protobuf.kotlin.protoc.testproto.HasNestedClassNameConflictOuterClass
import com.google.protobuf.kotlin.protoc.testproto.HasOuterClassNameConflictOuterClass
import com.google.protobuf.kotlin.protoc.testproto.MyExplicitOuterClassName
import com.google.protos.testing.ProtoFileWithHyphen
import org.junit.jupiter.api.Assertions.assertThrows
import org.junit.Test
import org.junit.runner.RunWith
import org.junit.runners.JUnit4

/** Tests for the methods in DescriptorUtil.kt. */
@RunWith(JUnit4::class)
class DescriptorUtilTest {
  @Test
  fun isJavaPrimitiveIntField() {
    val int32Field = ExampleMessage.getDescriptor().findFieldByName("int32_field")
    assertThat(int32Field.isJavaPrimitive).isTrue()
  }

  @Test
  fun isJavaPrimitiveStringField() {
    val stringField = ExampleMessage.getDescriptor().findFieldByName("string_field")
    assertThat(stringField.isJavaPrimitive).isFalse()
  }

  @Test
  fun isJavaPrimitiveMessageField() {
    val subMessageField = ExampleMessage.getDescriptor().findFieldByName("sub_message_field")
    assertThat(subMessageField.isJavaPrimitive).isFalse()
  }

  @Test
  fun scalarTypeIntField() {
    val int32Field = ExampleMessage.getDescriptor().findFieldByName("int32_field")
    assertThat(int32Field.scalarType.toString()).isEqualTo("kotlin.Int")
  }

  @Test
  fun scalarTypeStringField() {
    val stringField = ExampleMessage.getDescriptor().findFieldByName("string_field")
    assertThat(stringField.scalarType.toString()).isEqualTo("kotlin.String")
  }

  @Test
  fun scalarTypeSubMessageField() {
    val subMessageField =
      ExampleMessage.getDescriptor().findFieldByName("sub_message_field")
    assertThrows(IllegalArgumentException::class.java) {
      subMessageField.scalarType
    }
  }

  @Test
  fun oneofPropertySimpleName() {
    val oneofDescriptor = ExampleMessage.getDescriptor().oneofs.find { it.name == "my_oneof" }!!
    assertThat(oneofDescriptor.propertySimpleName).isEqualTo(MemberSimpleName("myOneof"))
  }

  @Test
  fun oneofCaseEnumSimpleName() {
    val oneofDescriptor = ExampleMessage.getDescriptor().oneofs.find { it.name == "my_oneof" }!!
    assertThat(oneofDescriptor.caseEnumSimpleName)
      .isEqualTo(ClassSimpleName(ExampleMessage.MyOneofCase::class.simpleName!!))
  }

  @Test
  fun oneofCasePropertySimpleName() {
    val oneofDescriptor = ExampleMessage.getDescriptor().oneofs.find { it.name == "my_oneof" }!!

    // demonstrate that the inferred property actually exists with this name
    assertThat(ExampleMessage.getDefaultInstance().myOneofCase)
      .isEqualTo(ExampleMessage.MyOneofCase.MYONEOF_NOT_SET)

    assertThat(oneofDescriptor.casePropertySimpleName).isEqualTo(MemberSimpleName("myOneofCase"))
  }

  @Test
  fun oneofNotSetCaseSimpleName() {
    val oneofDescriptor = ExampleMessage.getDescriptor().oneofs.find { it.name == "my_oneof" }!!
    assertThat(oneofDescriptor.notSetCaseSimpleName)
      .isEqualTo(ConstantName(ExampleMessage.MyOneofCase.MYONEOF_NOT_SET.name))
  }

  @Test
  fun messageClassSimpleName() {
    val messageDescriptor = ExampleMessage.getDescriptor()
    assertThat(messageDescriptor.messageClassSimpleName)
      .isEqualTo(ClassSimpleName(ExampleMessage::class.simpleName!!))
  }

  @Test
  fun enumClassSimpleName() {
    val enumDescriptor = ExampleEnum.getDescriptor()
    assertThat(enumDescriptor.enumClassSimpleName)
      .isEqualTo(ClassSimpleName(ExampleEnum::class.simpleName!!))
  }

  @Test
  fun outerClassSimpleName_simple() {
    val fileDescriptor = Example3.getDescriptor()
    assertThat(fileDescriptor.outerClassSimpleName)
      .isEqualTo(ClassSimpleName(Example3::class.simpleName!!))
  }

  @Test
  fun outerClassSimpleName_hasOuterClassNameConflict() {
    val fileDescriptor = HasOuterClassNameConflictOuterClass.getDescriptor()
    assertThat(fileDescriptor.fileName.name).isEqualTo("has_outer_class_name_conflict")
    assertThat(fileDescriptor.outerClassSimpleName)
      .isEqualTo(ClassSimpleName(HasOuterClassNameConflictOuterClass::class.simpleName!!))
  }

  @Test
  fun outerClassSimpleName_hasHyphenInFileName() {
    val fileDescriptor = ProtoFileWithHyphen.getDescriptor()
    assertThat(fileDescriptor.fileName.name).isEqualTo("proto-file-with-hyphen")
    assertThat(fileDescriptor.outerClassSimpleName)
      .isEqualTo(ClassSimpleName(ProtoFileWithHyphen::class.simpleName!!))
  }

  @Test
  fun outerClassSimpleName_hasNestedClassNameConflict() {
    val fileDescriptor = HasNestedClassNameConflictOuterClass.getDescriptor()
    assertThat(fileDescriptor.fileName.name).isEqualTo("has_nested_class_name_conflict")
    assertThat(fileDescriptor.outerClassSimpleName)
      .isEqualTo(ClassSimpleName(HasNestedClassNameConflictOuterClass::class.simpleName!!))
  }

  @Test
  fun outerClassSimpleName_hasExplicitOuterClassName() {
    val fileDescriptor = MyExplicitOuterClassName.getDescriptor()
    assertThat(fileDescriptor.fileName.name).isEqualTo("has_explicit_outer_class_name")
    assertThat(fileDescriptor.outerClassSimpleName)
      .isEqualTo(ClassSimpleName(MyExplicitOuterClassName::class.simpleName!!))
  }
}
