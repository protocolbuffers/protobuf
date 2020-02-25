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
import com.google.protobuf.Descriptors.FieldDescriptor
import com.google.protobuf.kotlin.protoc.testproto.Example2.ExampleProto2Message
import com.google.protobuf.kotlin.protoc.testproto.Example3
import com.google.protobuf.kotlin.protoc.testproto.Example3.ExampleMessage
import org.junit.Test
import org.junit.runner.RunWith
import org.junit.runners.JUnit4

/** Tests for [MessageComponent] and subtypes. */
@RunWith(JUnit4::class)
class MessageComponentTest {
  @Test
  fun singletonFieldProto3Scalar() {
    val fieldDescriptor =
      Example3.ExampleMessage.getDescriptor().findFieldByNumber(ExampleMessage.INT32_FIELD_FIELD_NUMBER)
    val singletonDescriptor = fieldDescriptor.specialize() as SingletonFieldDescriptor
    with(singletonDescriptor) {
      assertThat(fieldName).isEqualTo(ProtoFieldName("int32_field"))
      assertThat(propertySimpleName).isEqualTo(MemberSimpleName("int32Field"))
      assertThat(isRequired()).isFalse()
      assertThat(hasHazzer()).isFalse()
      assertThat(isMessage()).isFalse()
    }
  }

  @Test
  fun singletonFieldProto3Message() {
    val fieldDescriptor =
      ExampleMessage.getDescriptor()
        .findFieldByNumber(ExampleMessage.SUB_MESSAGE_FIELD_FIELD_NUMBER)
    val singletonDescriptor = fieldDescriptor.specialize() as SingletonFieldDescriptor
    with(singletonDescriptor) {
      assertThat(fieldName).isEqualTo(ProtoFieldName("sub_message_field"))
      assertThat(propertySimpleName).isEqualTo(MemberSimpleName("subMessageField"))
      assertThat(isRequired()).isFalse()
      assertThat(hasHazzer()).isTrue()
      assertThat(isMessage()).isTrue()
    }
  }

  @Test
  fun singletonFieldProto2Required() {
    val fieldDescriptor =
      ExampleProto2Message.getDescriptor()
        .findFieldByNumber(ExampleProto2Message.REQUIRED_STRING_FIELD_FIELD_NUMBER)
    val singletonDescriptor = fieldDescriptor.specialize() as SingletonFieldDescriptor
    with(singletonDescriptor) {
      assertThat(fieldName).isEqualTo(ProtoFieldName("required_string_field"))
      assertThat(propertySimpleName).isEqualTo(MemberSimpleName("requiredStringField"))
      assertThat(isRequired()).isTrue()
      assertThat(hasHazzer()).isTrue()
      assertThat(isMessage()).isFalse()
    }
  }
  @Test
  fun singletonFieldProto2OptionalScalar() {
    val fieldDescriptor =
      ExampleProto2Message.getDescriptor()
        .findFieldByNumber(ExampleProto2Message.OPTIONAL_INT32_FIELD_FIELD_NUMBER)
    val singletonDescriptor = fieldDescriptor.specialize() as SingletonFieldDescriptor
    with(singletonDescriptor) {
      assertThat(fieldName).isEqualTo(ProtoFieldName("optional_int32_field"))
      assertThat(propertySimpleName).isEqualTo(MemberSimpleName("optionalInt32Field"))
      assertThat(isRequired()).isFalse()
      assertThat(hasHazzer()).isTrue()
      assertThat(isMessage()).isFalse()
    }
  }

  @Test
  fun mapField() {
    val fieldDescriptor =
      ExampleMessage.getDescriptor().findFieldByNumber(ExampleMessage.MAP_FIELD_FIELD_NUMBER)
    val mapDescriptor = fieldDescriptor.specialize() as MapFieldDescriptor
    with(mapDescriptor) {
      assertThat(fieldName).isEqualTo(ProtoFieldName("map_field"))
      assertThat(propertySimpleName).isEqualTo(MemberSimpleName("mapField"))
      assertThat(keyFieldDescriptor.type).isEqualTo(FieldDescriptor.Type.INT64)
      assertThat(valueFieldDescriptor.type).isEqualTo(FieldDescriptor.Type.MESSAGE)
    }
  }

  @Test
  fun repeatedField() {
    val fieldDescriptor =
      ExampleMessage.getDescriptor()
        .findFieldByNumber(ExampleMessage.REPEATED_STRING_FIELD_FIELD_NUMBER)
    assertThat(fieldDescriptor.specialize()).isInstanceOf(RepeatedFieldDescriptor::class.java)
  }

  @Test
  fun oneofField() {
    val fieldDescriptor =
      ExampleMessage.getDescriptor()
        .findFieldByNumber(ExampleMessage.STRING_ONEOF_OPTION_FIELD_NUMBER)
    val oneofOptionDescriptor = fieldDescriptor.specialize() as OneofOptionFieldDescriptor
    with(oneofOptionDescriptor) {
      assertThat(oneof.oneofName).isEqualTo(ProtoFieldName("my_oneof"))
      assertThat(enumConstantName)
        .isEqualTo(ConstantName(ExampleMessage.MyOneofCase.STRING_ONEOF_OPTION.name))
      assertThat(propertySimpleName).isEqualTo(MemberSimpleName("stringOneofOption"))
      assertThat(hasHazzer()).isFalse()
      assertThat(isMessage()).isFalse()
      assertThat(isRequired()).isFalse()
    }
  }

  @Test
  fun singletonFieldReservedWord() {
    val fieldDescriptor =
      ExampleMessage.getDescriptor().findFieldByNumber(ExampleMessage.CLASS_FIELD_NUMBER)
    val singletonFieldDescriptor = fieldDescriptor.specialize() as SingletonFieldDescriptor
    with(singletonFieldDescriptor) {
      assertThat(propertySimpleName).isEqualTo(MemberSimpleName("class"))
      assertThat(javaSimpleName).isEqualTo(MemberSimpleName("class_"))
      assertThat(getterSimpleName).isEqualTo(MemberSimpleName("getClass_"))
      assertThat(setterSimpleName).isEqualTo(MemberSimpleName("setClass_"))
      assertThat(clearerSimpleName).isEqualTo(MemberSimpleName("clearClass_"))
      assertThat(hazzerSimpleName).isEqualTo(MemberSimpleName("hasClass_"))
    }
  }

  @Test
  fun repeatedFieldReservedWord() {
    val fieldDescriptor =
      ExampleMessage.getDescriptor().findFieldByNumber(ExampleMessage.CACHED_SIZE_FIELD_NUMBER)
    val repeatedFieldDescriptor = fieldDescriptor.specialize() as RepeatedFieldDescriptor
    with(repeatedFieldDescriptor) {
      assertThat(propertySimpleName).isEqualTo(MemberSimpleName("cachedSize"))
      assertThat(javaSimpleName).isEqualTo(MemberSimpleName("cachedSize_"))
      assertThat(listGetterSimpleName()).isEqualTo(MemberSimpleName("getCachedSize_List"))
      assertThat(setterSimpleName).isEqualTo(MemberSimpleName("setCachedSize_"))
      assertThat(adderSimpleName).isEqualTo(MemberSimpleName("addCachedSize_"))
      assertThat(addAllSimpleName).isEqualTo(MemberSimpleName("addAllCachedSize_"))
      assertThat(clearerSimpleName).isEqualTo(MemberSimpleName("clearCachedSize_"))
    }
  }

  @Test
  fun mapFieldReservedWord() {
    val fieldDescriptor =
      ExampleMessage.getDescriptor().findFieldByNumber(ExampleMessage.SERIALIZED_SIZE_FIELD_NUMBER)
    val mapFieldDescriptor = fieldDescriptor.specialize() as MapFieldDescriptor
    with(mapFieldDescriptor) {
      assertThat(propertySimpleName).isEqualTo(MemberSimpleName("serializedSize"))
      assertThat(javaSimpleName).isEqualTo(MemberSimpleName("serializedSize_"))
      assertThat(mapGetterSimpleName).isEqualTo(MemberSimpleName("getSerializedSize_Map"))
      assertThat(putterSimpleName).isEqualTo(MemberSimpleName("putSerializedSize_"))
      assertThat(putAllSimpleName).isEqualTo(MemberSimpleName("putAllSerializedSize_"))
      assertThat(removerSimpleName).isEqualTo(MemberSimpleName("removeSerializedSize_"))
      assertThat(clearerSimpleName).isEqualTo(MemberSimpleName("clearSerializedSize_"))
    }
  }
}
