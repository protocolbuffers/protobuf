// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
// https://developers.google.com/protocol-buffers/
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

package com.google.protobuf;

import static com.google.common.truth.Truth.assertThat;
import static com.google.common.truth.Truth.assertWithMessage;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.fail;

import com.google.protobuf.Descriptors.FieldDescriptor;
import map_test.MapForProto2TestProto.BizarroTestMap;
import map_test.MapForProto2TestProto.ReservedAsMapField;
import map_test.MapForProto2TestProto.ReservedAsMapFieldWithEnumValue;
import map_test.MapForProto2TestProto.TestMap;
import map_test.MapForProto2TestProto.TestMap.MessageValue;
import map_test.MapForProto2TestProto.TestMap.MessageWithRequiredFields;
import map_test.MapForProto2TestProto.TestMapOrBuilder;
import map_test.MapForProto2TestProto.TestRecursiveMap;
import map_test.MapForProto2TestProto.TestUnknownEnumValue;
import map_test.Message1;
import map_test.RedactAllTypes;
import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import java.util.TreeMap;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

/** Unit tests for map fields in proto2 protos. */
@RunWith(JUnit4.class)
public class MapForProto2Test {

  private void setMapValuesUsingMutableMap(TestMap.Builder builder) {
    builder.getMutableInt32ToInt32Field().put(1, 11);
    builder.getMutableInt32ToInt32Field().put(2, 22);
    builder.getMutableInt32ToInt32Field().put(3, 33);

    builder.getMutableInt32ToStringField().put(1, "11");
    builder.getMutableInt32ToStringField().put(2, "22");
    builder.getMutableInt32ToStringField().put(3, "33");

    builder.getMutableInt32ToBytesField().put(1, TestUtil.toBytes("11"));
    builder.getMutableInt32ToBytesField().put(2, TestUtil.toBytes("22"));
    builder.getMutableInt32ToBytesField().put(3, TestUtil.toBytes("33"));

    builder.getMutableInt32ToEnumField().put(1, TestMap.EnumValue.FOO);
    builder.getMutableInt32ToEnumField().put(2, TestMap.EnumValue.BAR);
    builder.getMutableInt32ToEnumField().put(3, TestMap.EnumValue.BAZ);

    builder.getMutableInt32ToMessageField().put(
        1, MessageValue.newBuilder().setValue(11).build());
    builder.getMutableInt32ToMessageField().put(
        2, MessageValue.newBuilder().setValue(22).build());
    builder.getMutableInt32ToMessageField().put(
        3, MessageValue.newBuilder().setValue(33).build());

    builder.getMutableStringToInt32Field().put("1", 11);
    builder.getMutableStringToInt32Field().put("2", 22);
    builder.getMutableStringToInt32Field().put("3", 33);
  }

  private void setMapValuesUsingAccessors(TestMap.Builder builder) {
    builder
        .putInt32ToInt32Field(1, 11)
        .putInt32ToInt32Field(2, 22)
        .putInt32ToInt32Field(3, 33)
        .putInt32ToStringField(1, "11")
        .putInt32ToStringField(2, "22")
        .putInt32ToStringField(3, "33")
        .putInt32ToBytesField(1, TestUtil.toBytes("11"))
        .putInt32ToBytesField(2, TestUtil.toBytes("22"))
        .putInt32ToBytesField(3, TestUtil.toBytes("33"))
        .putInt32ToEnumField(1, TestMap.EnumValue.FOO)
        .putInt32ToEnumField(2, TestMap.EnumValue.BAR)
        .putInt32ToEnumField(3, TestMap.EnumValue.BAZ)
        .putInt32ToMessageField(1, MessageValue.newBuilder().setValue(11).build())
        .putInt32ToMessageField(2, MessageValue.newBuilder().setValue(22).build())
        .putInt32ToMessageField(3, MessageValue.newBuilder().setValue(33).build())
        .putStringToInt32Field("1", 11)
        .putStringToInt32Field("2", 22)
        .putStringToInt32Field("3", 33);
  }

  @Test
  public void testSetMapValues() {
    TestMap.Builder usingMutableMapBuilder = TestMap.newBuilder();
    setMapValuesUsingMutableMap(usingMutableMapBuilder);
    TestMap usingMutableMap = usingMutableMapBuilder.build();
    assertMapValuesSet(usingMutableMap);

    TestMap.Builder usingAccessorsBuilder = TestMap.newBuilder();
    setMapValuesUsingAccessors(usingAccessorsBuilder);
    TestMap usingAccessors = usingAccessorsBuilder.build();
    assertMapValuesSet(usingAccessors);

    assertThat(usingAccessors).isEqualTo(usingMutableMap);
  }

  private void copyMapValues(TestMap source, TestMap.Builder destination) {
    destination
        .putAllInt32ToInt32Field(source.getInt32ToInt32FieldMap())
        .putAllInt32ToStringField(source.getInt32ToStringFieldMap())
        .putAllInt32ToBytesField(source.getInt32ToBytesFieldMap())
        .putAllInt32ToEnumField(source.getInt32ToEnumFieldMap())
        .putAllInt32ToMessageField(source.getInt32ToMessageFieldMap())
        .putAllStringToInt32Field(source.getStringToInt32FieldMap());
  }

  private void assertMapValuesSet(TestMapOrBuilder message) {
    assertThat(message.getInt32ToInt32FieldMap()).hasSize(3);
    assertThat(message.getInt32ToInt32FieldMap().get(1).intValue()).isEqualTo(11);
    assertThat(message.getInt32ToInt32FieldMap().get(2).intValue()).isEqualTo(22);
    assertThat(message.getInt32ToInt32FieldMap().get(3).intValue()).isEqualTo(33);

    assertThat(message.getInt32ToStringFieldMap()).hasSize(3);
    assertThat(message.getInt32ToStringFieldMap()).containsEntry(1, "11");
    assertThat(message.getInt32ToStringFieldMap()).containsEntry(2, "22");
    assertThat(message.getInt32ToStringFieldMap()).containsEntry(3, "33");

    assertThat(message.getInt32ToBytesFieldMap()).hasSize(3);
    assertThat(message.getInt32ToBytesFieldMap()).containsEntry(1, TestUtil.toBytes("11"));
    assertThat(message.getInt32ToBytesFieldMap()).containsEntry(2, TestUtil.toBytes("22"));
    assertThat(message.getInt32ToBytesFieldMap()).containsEntry(3, TestUtil.toBytes("33"));

    assertThat(message.getInt32ToEnumFieldMap()).hasSize(3);
    assertThat(message.getInt32ToEnumFieldMap()).containsEntry(1, TestMap.EnumValue.FOO);
    assertThat(message.getInt32ToEnumFieldMap()).containsEntry(2, TestMap.EnumValue.BAR);
    assertThat(message.getInt32ToEnumFieldMap()).containsEntry(3, TestMap.EnumValue.BAZ);

    assertThat(message.getInt32ToMessageFieldMap()).hasSize(3);
    assertThat(message.getInt32ToMessageFieldMap().get(1).getValue()).isEqualTo(11);
    assertThat(message.getInt32ToMessageFieldMap().get(2).getValue()).isEqualTo(22);
    assertThat(message.getInt32ToMessageFieldMap().get(3).getValue()).isEqualTo(33);

    assertThat(message.getStringToInt32FieldMap()).hasSize(3);
    assertThat(message.getStringToInt32FieldMap().get("1").intValue()).isEqualTo(11);
    assertThat(message.getStringToInt32FieldMap().get("2").intValue()).isEqualTo(22);
    assertThat(message.getStringToInt32FieldMap().get("3").intValue()).isEqualTo(33);
  }

  private void updateMapValuesUsingMutableMap(TestMap.Builder builder) {
    builder.getMutableInt32ToInt32Field().put(1, 111);
    builder.getMutableInt32ToInt32Field().remove(2);
    builder.getMutableInt32ToInt32Field().put(4, 44);

    builder.getMutableInt32ToStringField().put(1, "111");
    builder.getMutableInt32ToStringField().remove(2);
    builder.getMutableInt32ToStringField().put(4, "44");

    builder.getMutableInt32ToBytesField().put(1, TestUtil.toBytes("111"));
    builder.getMutableInt32ToBytesField().remove(2);
    builder.getMutableInt32ToBytesField().put(4, TestUtil.toBytes("44"));

    builder.getMutableInt32ToEnumField().put(1, TestMap.EnumValue.BAR);
    builder.getMutableInt32ToEnumField().remove(2);
    builder.getMutableInt32ToEnumField().put(4, TestMap.EnumValue.QUX);

    builder.getMutableInt32ToMessageField().put(
        1, MessageValue.newBuilder().setValue(111).build());
    builder.getMutableInt32ToMessageField().remove(2);
    builder.getMutableInt32ToMessageField().put(
        4, MessageValue.newBuilder().setValue(44).build());

    builder.getMutableStringToInt32Field().put("1", 111);
    builder.getMutableStringToInt32Field().remove("2");
    builder.getMutableStringToInt32Field().put("4", 44);
  }

  private void updateMapValuesUsingAccessors(TestMap.Builder builder) {
    builder
        .putInt32ToInt32Field(1, 111)
        .removeInt32ToInt32Field(2)
        .putInt32ToInt32Field(4, 44)
        .putInt32ToStringField(1, "111")
        .removeInt32ToStringField(2)
        .putInt32ToStringField(4, "44")
        .putInt32ToBytesField(1, TestUtil.toBytes("111"))
        .removeInt32ToBytesField(2)
        .putInt32ToBytesField(4, TestUtil.toBytes("44"))
        .putInt32ToEnumField(1, TestMap.EnumValue.BAR)
        .removeInt32ToEnumField(2)
        .putInt32ToEnumField(4, TestMap.EnumValue.QUX)
        .putInt32ToMessageField(1, MessageValue.newBuilder().setValue(111).build())
        .removeInt32ToMessageField(2)
        .putInt32ToMessageField(4, MessageValue.newBuilder().setValue(44).build())
        .putStringToInt32Field("1", 111)
        .removeStringToInt32Field("2")
        .putStringToInt32Field("4", 44);
  }

  @Test
  public void testUpdateMapValues() {
    TestMap.Builder usingMutableMapBuilder = TestMap.newBuilder();
    setMapValuesUsingMutableMap(usingMutableMapBuilder);
    TestMap usingMutableMap = usingMutableMapBuilder.build();
    assertMapValuesSet(usingMutableMap);

    TestMap.Builder usingAccessorsBuilder = TestMap.newBuilder();
    setMapValuesUsingAccessors(usingAccessorsBuilder);
    TestMap usingAccessors = usingAccessorsBuilder.build();
    assertMapValuesSet(usingAccessors);

    assertThat(usingAccessors).isEqualTo(usingMutableMap);

    usingMutableMapBuilder = usingMutableMap.toBuilder();
    updateMapValuesUsingMutableMap(usingMutableMapBuilder);
    usingMutableMap = usingMutableMapBuilder.build();
    assertMapValuesUpdated(usingMutableMap);

    usingAccessorsBuilder = usingAccessors.toBuilder();
    updateMapValuesUsingAccessors(usingAccessorsBuilder);
    usingAccessors = usingAccessorsBuilder.build();
    assertMapValuesUpdated(usingAccessors);

    assertThat(usingAccessors).isEqualTo(usingMutableMap);
  }

  private void assertMapValuesUpdated(TestMap message) {
    assertThat(message.getInt32ToInt32FieldMap()).hasSize(3);
    assertThat(message.getInt32ToInt32FieldMap().get(1).intValue()).isEqualTo(111);
    assertThat(message.getInt32ToInt32FieldMap().get(3).intValue()).isEqualTo(33);
    assertThat(message.getInt32ToInt32FieldMap().get(4).intValue()).isEqualTo(44);

    assertThat(message.getInt32ToStringFieldMap()).hasSize(3);
    assertThat(message.getInt32ToStringFieldMap()).containsEntry(1, "111");
    assertThat(message.getInt32ToStringFieldMap()).containsEntry(3, "33");
    assertThat(message.getInt32ToStringFieldMap()).containsEntry(4, "44");

    assertThat(message.getInt32ToBytesFieldMap()).hasSize(3);
    assertThat(message.getInt32ToBytesFieldMap()).containsEntry(1, TestUtil.toBytes("111"));
    assertThat(message.getInt32ToBytesFieldMap()).containsEntry(3, TestUtil.toBytes("33"));
    assertThat(message.getInt32ToBytesFieldMap()).containsEntry(4, TestUtil.toBytes("44"));

    assertThat(message.getInt32ToEnumFieldMap()).hasSize(3);
    assertThat(message.getInt32ToEnumFieldMap()).containsEntry(1, TestMap.EnumValue.BAR);
    assertThat(message.getInt32ToEnumFieldMap()).containsEntry(3, TestMap.EnumValue.BAZ);
    assertThat(message.getInt32ToEnumFieldMap()).containsEntry(4, TestMap.EnumValue.QUX);

    assertThat(message.getInt32ToMessageFieldMap()).hasSize(3);
    assertThat(message.getInt32ToMessageFieldMap().get(1).getValue()).isEqualTo(111);
    assertThat(message.getInt32ToMessageFieldMap().get(3).getValue()).isEqualTo(33);
    assertThat(message.getInt32ToMessageFieldMap().get(4).getValue()).isEqualTo(44);

    assertThat(message.getStringToInt32FieldMap()).hasSize(3);
    assertThat(message.getStringToInt32FieldMap().get("1").intValue()).isEqualTo(111);
    assertThat(message.getStringToInt32FieldMap().get("3").intValue()).isEqualTo(33);
    assertThat(message.getStringToInt32FieldMap().get("4").intValue()).isEqualTo(44);
  }

  private void assertMapValuesCleared(TestMapOrBuilder testMapOrBuilder) {
    assertThat(testMapOrBuilder.getInt32ToInt32FieldMap()).isEmpty();
    assertThat(testMapOrBuilder.getInt32ToInt32FieldCount()).isEqualTo(0);
    assertThat(testMapOrBuilder.getInt32ToStringFieldMap()).isEmpty();
    assertThat(testMapOrBuilder.getInt32ToStringFieldCount()).isEqualTo(0);
    assertThat(testMapOrBuilder.getInt32ToBytesFieldMap()).isEmpty();
    assertThat(testMapOrBuilder.getInt32ToBytesFieldCount()).isEqualTo(0);
    assertThat(testMapOrBuilder.getInt32ToEnumFieldMap()).isEmpty();
    assertThat(testMapOrBuilder.getInt32ToEnumFieldCount()).isEqualTo(0);
    assertThat(testMapOrBuilder.getInt32ToMessageFieldMap()).isEmpty();
    assertThat(testMapOrBuilder.getInt32ToMessageFieldCount()).isEqualTo(0);
    assertThat(testMapOrBuilder.getStringToInt32FieldMap()).isEmpty();
    assertThat(testMapOrBuilder.getStringToInt32FieldCount()).isEqualTo(0);
  }

  @Test
  public void testGetMapIsImmutable() {
    TestMap.Builder builder = TestMap.newBuilder();
    assertMapsAreImmutable(builder);
    assertMapsAreImmutable(builder.build());

    setMapValuesUsingAccessors(builder);
    assertMapsAreImmutable(builder);
    assertMapsAreImmutable(builder.build());
  }

  private void assertMapsAreImmutable(TestMapOrBuilder testMapOrBuilder) {
    assertImmutable(testMapOrBuilder.getInt32ToInt32FieldMap(), 1, 2);
    assertImmutable(testMapOrBuilder.getInt32ToStringFieldMap(), 1, "2");
    assertImmutable(testMapOrBuilder.getInt32ToBytesFieldMap(), 1, TestUtil.toBytes("2"));
    assertImmutable(testMapOrBuilder.getInt32ToEnumFieldMap(), 1, TestMap.EnumValue.FOO);
    assertImmutable(
        testMapOrBuilder.getInt32ToMessageFieldMap(), 1, MessageValue.getDefaultInstance());
    assertImmutable(testMapOrBuilder.getStringToInt32FieldMap(), "1", 2);
  }

  private <K, V> void assertImmutable(Map<K, V> map, K key, V value) {
    try {
      map.put(key, value);
      assertWithMessage("expected exception").fail();
    } catch (UnsupportedOperationException e) {
      // expected
    }
  }

  @Test
  public void testMutableMapLifecycle() {
    TestMap.Builder builder = TestMap.newBuilder();
    Map<Integer, Integer> intMap = builder.getMutableInt32ToInt32Field();
    intMap.put(1, 2);
    assertThat(builder.build().getInt32ToInt32Field()).isEqualTo(newMap(1, 2));
    try {
      intMap.put(2, 3);
      assertWithMessage("expected exception").fail();
    } catch (UnsupportedOperationException e) {
      // expected
    }
    assertThat(builder.getInt32ToInt32Field()).isEqualTo(newMap(1, 2));
    builder.getMutableInt32ToInt32Field().put(2, 3);
    assertThat(builder.getInt32ToInt32Field()).isEqualTo(newMap(1, 2, 2, 3));

    Map<Integer, TestMap.EnumValue> enumMap = builder.getMutableInt32ToEnumField();
    enumMap.put(1, TestMap.EnumValue.BAR);
    assertThat(builder.build().getInt32ToEnumField())
        .isEqualTo(newMap(1, TestMap.EnumValue.BAR));
    try {
      enumMap.put(2, TestMap.EnumValue.FOO);
      assertWithMessage("expected exception").fail();
    } catch (UnsupportedOperationException e) {
      // expected
    }
    assertThat(builder.getInt32ToEnumField()).isEqualTo(newMap(1, TestMap.EnumValue.BAR));
    builder.getMutableInt32ToEnumField().put(2, TestMap.EnumValue.FOO);
    assertThat(builder.getInt32ToEnumField())
            .isEqualTo(newMap(1, TestMap.EnumValue.BAR, 2, TestMap.EnumValue.FOO));

    Map<Integer, String> stringMap = builder.getMutableInt32ToStringField();
    stringMap.put(1, "1");
    assertThat(builder.build().getInt32ToStringField()).isEqualTo(newMap(1, "1"));
    try {
      stringMap.put(2, "2");
      assertWithMessage("expected exception").fail();
    } catch (UnsupportedOperationException e) {
      // expected
    }
    assertThat(builder.getInt32ToStringField()).isEqualTo(newMap(1, "1"));
    builder.getMutableInt32ToStringField().put(2, "2");
    assertThat(builder.getInt32ToStringField()).isEqualTo(newMap(1, "1", 2, "2"));

    Map<Integer, TestMap.MessageValue> messageMap = builder.getMutableInt32ToMessageField();
    messageMap.put(1, TestMap.MessageValue.getDefaultInstance());
    assertThat(builder.build().getInt32ToMessageField())
        .isEqualTo(newMap(1, TestMap.MessageValue.getDefaultInstance()));
    try {
      messageMap.put(2, TestMap.MessageValue.getDefaultInstance());
      assertWithMessage("expected exception").fail();
    } catch (UnsupportedOperationException e) {
      // expected
    }
    assertThat(builder.getInt32ToMessageField())
        .isEqualTo(newMap(1, TestMap.MessageValue.getDefaultInstance()));
    builder.getMutableInt32ToMessageField().put(2, TestMap.MessageValue.getDefaultInstance());
    assertThat(builder.getInt32ToMessageField()).isEqualTo(
        newMap(1, TestMap.MessageValue.getDefaultInstance(),
            2, TestMap.MessageValue.getDefaultInstance()));
  }

  @Test
  public void testMutableMapLifecycle_collections() {
    TestMap.Builder builder = TestMap.newBuilder();
    Map<Integer, Integer> intMap = builder.getMutableInt32ToInt32Field();
    intMap.put(1, 2);
    assertThat(builder.build().getInt32ToInt32Field()).isEqualTo(newMap(1, 2));
    try {
      intMap.remove(2);
      assertWithMessage("expected exception").fail();
    } catch (UnsupportedOperationException e) {
      // expected
    }
    try {
      intMap.entrySet().remove(new Object());
      assertWithMessage("expected exception").fail();
    } catch (UnsupportedOperationException e) {
      // expected
    }
    try {
      intMap.entrySet().iterator().remove();
      assertWithMessage("expected exception").fail();
    } catch (UnsupportedOperationException e) {
      // expected
    }
    try {
      intMap.keySet().remove(new Object());
      assertWithMessage("expected exception").fail();
    } catch (UnsupportedOperationException e) {
      // expected
    }
    try {
      intMap.values().remove(new Object());
      assertWithMessage("expected exception").fail();
    } catch (UnsupportedOperationException e) {
      // expected
    }
    try {
      intMap.values().iterator().remove();
      assertWithMessage("expected exception").fail();
    } catch (UnsupportedOperationException e) {
      // expected
    }
    assertThat(intMap).isEqualTo(newMap(1, 2));
    assertThat(builder.getInt32ToInt32Field()).isEqualTo(newMap(1, 2));
    assertThat(builder.build().getInt32ToInt32Field()).isEqualTo(newMap(1, 2));
  }

  private static <K, V> Map<K, V> newMap(K key1, V value1) {
    Map<K, V> map = new HashMap<K, V>();
    map.put(key1, value1);
    return map;
  }

  private static <K, V> Map<K, V> newMap(K key1, V value1, K key2, V value2) {
    Map<K, V> map = new HashMap<K, V>();
    map.put(key1, value1);
    map.put(key2, value2);
    return map;
  }

  @Test
  public void testGettersAndSetters() throws Exception {
    TestMap.Builder builder = TestMap.newBuilder();
    TestMap message = builder.build();
    assertMapValuesCleared(message);

    builder = message.toBuilder();
    setMapValuesUsingAccessors(builder);
    message = builder.build();
    assertMapValuesSet(message);

    builder = message.toBuilder();
    updateMapValuesUsingAccessors(builder);
    message = builder.build();
    assertMapValuesUpdated(message);

    builder = message.toBuilder();
    builder.clear();
    assertMapValuesCleared(builder);
    message = builder.build();
    assertMapValuesCleared(message);
  }

  @Test
  public void testPutAll() throws Exception {
    TestMap.Builder sourceBuilder = TestMap.newBuilder();
    setMapValuesUsingAccessors(sourceBuilder);
    TestMap source = sourceBuilder.build();
    assertMapValuesSet(source);

    TestMap.Builder destination = TestMap.newBuilder();
    copyMapValues(source, destination);
    assertMapValuesSet(destination.build());

    assertThat(destination.getInt32ToEnumFieldCount()).isEqualTo(3);
  }

  @Test
  public void testPutChecksNullValues() throws Exception {
    TestMap.Builder builder = TestMap.newBuilder();

    try {
      builder.putInt32ToBytesField(1, null);
      assertWithMessage("expected exception").fail();
    } catch (NullPointerException e) {
      // expected.
    }

    try {
      builder.putInt32ToEnumField(1, null);
      assertWithMessage("expected exception").fail();
    } catch (NullPointerException e) {
      // expected.
    }

    try {
      builder.putInt32ToMessageField(1, null);
      assertWithMessage("expected exception").fail();
    } catch (NullPointerException e) {
      // expected.
    }

    try {
      builder.putStringToInt32Field(null, 1);
      assertWithMessage("expected exception").fail();
    } catch (NullPointerException e) {
      // expected.
    }
  }

  @Test
  public void testPutChecksNullKey() throws Exception {
    TestMap.Builder builder = TestMap.newBuilder();

    try {
      builder.putStringToInt32Field(null, 1);
      assertWithMessage("expected exception").fail();
    } catch (NullPointerException expected) {
    }
  }

  @Test
  public void testSerializeAndParse() throws Exception {
    TestMap.Builder builder = TestMap.newBuilder();
    setMapValuesUsingAccessors(builder);
    TestMap message = builder.build();
    assertThat(message.toByteString().size()).isEqualTo(message.getSerializedSize());
    message = TestMap.parseFrom(message.toByteString());
    assertMapValuesSet(message);

    builder = message.toBuilder();
    updateMapValuesUsingAccessors(builder);
    message = builder.build();
    assertThat(message.toByteString().size()).isEqualTo(message.getSerializedSize());
    message = TestMap.parseFrom(message.toByteString());
    assertMapValuesUpdated(message);

    builder = message.toBuilder();
    builder.clear();
    message = builder.build();
    assertThat(message.toByteString().size()).isEqualTo(message.getSerializedSize());
    message = TestMap.parseFrom(message.toByteString());
    assertMapValuesCleared(message);
  }

  private TestMap tryParseTestMap(BizarroTestMap bizarroMap) throws IOException {
    ByteArrayOutputStream byteArrayOutputStream = new ByteArrayOutputStream();
    CodedOutputStream output = CodedOutputStream.newInstance(byteArrayOutputStream);
    bizarroMap.writeTo(output);
    output.flush();
    return TestMap.parseFrom(ByteString.copyFrom(byteArrayOutputStream.toByteArray()));
  }

  @Test
  public void testParseError() throws Exception {
    ByteString bytes = TestUtil.toBytes("SOME BYTES");
    String stringKey = "a string key";

    TestMap map =
        tryParseTestMap(BizarroTestMap.newBuilder().putInt32ToInt32Field(5, bytes).build());
    assertThat(map.getInt32ToInt32FieldOrDefault(5, -1)).isEqualTo(0);

    map = tryParseTestMap(BizarroTestMap.newBuilder().putInt32ToStringField(stringKey, 5).build());
    assertThat(map.getInt32ToStringFieldOrDefault(0, null)).isEmpty();

    map = tryParseTestMap(BizarroTestMap.newBuilder().putInt32ToBytesField(stringKey, 5).build());
    assertThat(ByteString.EMPTY).isEqualTo(map.getInt32ToBytesFieldOrDefault(0, null));

    map =
        tryParseTestMap(BizarroTestMap.newBuilder().putInt32ToEnumField(stringKey, bytes).build());
    assertThat(map.getInt32ToEnumFieldOrDefault(0, null)).isEqualTo(TestMap.EnumValue.FOO);

    try {
      tryParseTestMap(BizarroTestMap.newBuilder().putInt32ToMessageField(stringKey, bytes).build());
      assertWithMessage("expected exception").fail();
    } catch (InvalidProtocolBufferException expected) {
      assertThat(expected.getUnfinishedMessage()).isInstanceOf(TestMap.class);
      map = (TestMap) expected.getUnfinishedMessage();
      assertThat(map.getInt32ToMessageFieldMap()).isEmpty();
    }

    map =
        tryParseTestMap(
            BizarroTestMap.newBuilder().putStringToInt32Field(stringKey, bytes).build());
    assertThat(map.getStringToInt32FieldOrDefault(stringKey, -1)).isEqualTo(0);
  }

  @Test
  public void testMergeFrom() throws Exception {
    TestMap.Builder builder = TestMap.newBuilder();
    setMapValuesUsingAccessors(builder);
    TestMap message = builder.build();

    TestMap.Builder other = TestMap.newBuilder();
    other.mergeFrom(message);
    assertMapValuesSet(other.build());
  }

  @Test
  public void testEqualsAndHashCode() throws Exception {
    // Test that generated equals() and hashCode() will disregard the order
    // of map entries when comparing/hashing map fields.

    // We can't control the order of elements in a HashMap. The best we can do
    // here is to add elements in different order.
    TestMap.Builder b1 = TestMap.newBuilder();
    b1.putInt32ToInt32Field(1, 2);
    b1.putInt32ToInt32Field(3, 4);
    b1.putInt32ToInt32Field(5, 6);
    TestMap m1 = b1.build();

    TestMap.Builder b2 = TestMap.newBuilder();
    b2.putInt32ToInt32Field(5, 6);
    b2.putInt32ToInt32Field(1, 2);
    b2.putInt32ToInt32Field(3, 4);
    TestMap m2 = b2.build();

    assertThat(m2).isEqualTo(m1);
    assertThat(m2.hashCode()).isEqualTo(m1.hashCode());

    // Make sure we did compare map fields.
    b2.putInt32ToInt32Field(1, 0);
    m2 = b2.build();
    assertThat(m1.equals(m2)).isFalse();
    // Don't check m1.hashCode() != m2.hashCode() because it's not guaranteed
    // to be different.
  }

  // The following methods are used to test reflection API.

  private static FieldDescriptor f(String name) {
    return TestMap.getDescriptor().findFieldByName(name);
  }

  private static Object getFieldValue(Message mapEntry, String name) {
    FieldDescriptor field = mapEntry.getDescriptorForType().findFieldByName(name);
    return mapEntry.getField(field);
  }

  private static Message.Builder setFieldValue(
      Message.Builder mapEntry, String name, Object value) {
    FieldDescriptor field = mapEntry.getDescriptorForType().findFieldByName(name);
    mapEntry.setField(field, value);
    return mapEntry;
  }

  private static void assertHasMapValues(Message message, String name, Map<?, ?> values) {
    FieldDescriptor field = f(name);
    for (Object entry : (List<?>) message.getField(field)) {
      Message mapEntry = (Message) entry;
      Object key = getFieldValue(mapEntry, "key");
      Object value = getFieldValue(mapEntry, "value");
      assertThat(values.containsKey(key)).isTrue();
      assertThat(values.get(key)).isEqualTo(value);
    }
    assertThat(message.getRepeatedFieldCount(field)).isEqualTo(values.size());
    for (int i = 0; i < message.getRepeatedFieldCount(field); i++) {
      Message mapEntry = (Message) message.getRepeatedField(field, i);
      Object key = getFieldValue(mapEntry, "key");
      Object value = getFieldValue(mapEntry, "value");
      assertThat(values.containsKey(key)).isTrue();
      assertThat(values.get(key)).isEqualTo(value);
    }
  }

  private static <K, V> Message newMapEntry(Message.Builder builder, String name, K key, V value) {
    FieldDescriptor field = builder.getDescriptorForType().findFieldByName(name);
    Message.Builder entryBuilder = builder.newBuilderForField(field);
    FieldDescriptor keyField = entryBuilder.getDescriptorForType().findFieldByName("key");
    FieldDescriptor valueField = entryBuilder.getDescriptorForType().findFieldByName("value");
    entryBuilder.setField(keyField, key);
    entryBuilder.setField(valueField, value);
    return entryBuilder.build();
  }

  private static void setMapValues(Message.Builder builder, String name, Map<?, ?> values) {
    List<Message> entryList = new ArrayList<>();
    for (Map.Entry<?, ?> entry : values.entrySet()) {
      entryList.add(newMapEntry(builder, name, entry.getKey(), entry.getValue()));
    }
    FieldDescriptor field = builder.getDescriptorForType().findFieldByName(name);
    builder.setField(field, entryList);
  }

  private static <K, V> Map<K, V> mapForValues(K key1, V value1, K key2, V value2) {
    Map<K, V> map = new HashMap<>();
    map.put(key1, value1);
    map.put(key2, value2);
    return map;
  }

  @Test
  public void testReflectionApi() throws Exception {
    // In reflection API, map fields are just repeated message fields.
    TestMap.Builder builder =
        TestMap.newBuilder()
            .putInt32ToInt32Field(1, 2)
            .putInt32ToInt32Field(3, 4)
            .putInt32ToMessageField(11, MessageValue.newBuilder().setValue(22).build())
            .putInt32ToMessageField(33, MessageValue.newBuilder().setValue(44).build());
    TestMap message = builder.build();

    // Test getField(), getRepeatedFieldCount(), getRepeatedField().
    assertHasMapValues(message, "int32_to_int32_field", mapForValues(1, 2, 3, 4));
    assertHasMapValues(
        message,
        "int32_to_message_field",
        mapForValues(
            11, MessageValue.newBuilder().setValue(22).build(),
            33, MessageValue.newBuilder().setValue(44).build()));

    // Test clearField()
    builder.clearField(f("int32_to_int32_field"));
    builder.clearField(f("int32_to_message_field"));
    message = builder.build();
    assertThat(message.getInt32ToInt32FieldMap()).isEmpty();
    assertThat(message.getInt32ToMessageFieldMap()).isEmpty();

    // Test setField()
    setMapValues(builder, "int32_to_int32_field", mapForValues(11, 22, 33, 44));
    setMapValues(
        builder,
        "int32_to_message_field",
        mapForValues(
            111, MessageValue.newBuilder().setValue(222).build(),
            333, MessageValue.newBuilder().setValue(444).build()));
    message = builder.build();
    assertThat(message.getInt32ToInt32FieldMap().get(11).intValue()).isEqualTo(22);
    assertThat(message.getInt32ToInt32FieldMap().get(33).intValue()).isEqualTo(44);
    assertThat(message.getInt32ToMessageFieldMap().get(111).getValue()).isEqualTo(222);
    assertThat(message.getInt32ToMessageFieldMap().get(333).getValue()).isEqualTo(444);

    // Test addRepeatedField
    builder.addRepeatedField(
        f("int32_to_int32_field"), newMapEntry(builder, "int32_to_int32_field", 55, 66));
    builder.addRepeatedField(
        f("int32_to_message_field"),
        newMapEntry(
            builder,
            "int32_to_message_field",
            555,
            MessageValue.newBuilder().setValue(666).build()));
    message = builder.build();
    assertThat(message.getInt32ToInt32FieldMap().get(55).intValue()).isEqualTo(66);
    assertThat(message.getInt32ToMessageFieldMap().get(555).getValue()).isEqualTo(666);

    // Test addRepeatedField (overriding existing values)
    builder.addRepeatedField(
        f("int32_to_int32_field"), newMapEntry(builder, "int32_to_int32_field", 55, 55));
    builder.addRepeatedField(
        f("int32_to_message_field"),
        newMapEntry(
            builder,
            "int32_to_message_field",
            555,
            MessageValue.newBuilder().setValue(555).build()));
    message = builder.build();
    assertThat(message.getInt32ToInt32FieldMap().get(55).intValue()).isEqualTo(55);
    assertThat(message.getInt32ToMessageFieldMap().get(555).getValue()).isEqualTo(555);

    // Test setRepeatedField
    for (int i = 0; i < builder.getRepeatedFieldCount(f("int32_to_int32_field")); i++) {
      Message mapEntry = (Message) builder.getRepeatedField(f("int32_to_int32_field"), i);
      int oldKey = ((Integer) getFieldValue(mapEntry, "key")).intValue();
      int oldValue = ((Integer) getFieldValue(mapEntry, "value")).intValue();
      // Swap key with value for each entry.
      Message.Builder mapEntryBuilder = mapEntry.toBuilder();
      setFieldValue(mapEntryBuilder, "key", oldValue);
      setFieldValue(mapEntryBuilder, "value", oldKey);
      builder.setRepeatedField(f("int32_to_int32_field"), i, mapEntryBuilder.build());
    }
    message = builder.build();
    assertThat(message.getInt32ToInt32FieldMap().get(22).intValue()).isEqualTo(11);
    assertThat(message.getInt32ToInt32FieldMap().get(44).intValue()).isEqualTo(33);
    assertThat(message.getInt32ToInt32FieldMap().get(55).intValue()).isEqualTo(55);
  }

  // See additional coverage in TextFormatTest.java.
  @Test
  public void testTextFormat() throws Exception {
    TestMap.Builder builder = TestMap.newBuilder();
    setMapValuesUsingAccessors(builder);
    TestMap message = builder.build();

    String textData = TextFormat.printer().printToString(message);

    builder = TestMap.newBuilder();
    TextFormat.merge(textData, builder);
    message = builder.build();

    assertMapValuesSet(message);
  }

  @Test
  public void testDynamicMessage() throws Exception {
    TestMap.Builder builder = TestMap.newBuilder();
    setMapValuesUsingAccessors(builder);
    TestMap message = builder.build();

    Message dynamicDefaultInstance = DynamicMessage.getDefaultInstance(TestMap.getDescriptor());
    Message dynamicMessage =
        dynamicDefaultInstance
            .newBuilderForType()
            .mergeFrom(message.toByteString(), ExtensionRegistry.getEmptyRegistry())
            .build();

    assertThat(dynamicMessage).isEqualTo(message);
    assertThat(dynamicMessage.hashCode()).isEqualTo(message.hashCode());
  }

  // Check that DynamicMessage handles map field serialization the same way as generated code
  // regarding unset key and value field in a map entry.
  @Test
  public void testDynamicMessageUnsetKeyAndValue() throws Exception {
    FieldDescriptor field = f("int32_to_int32_field");

    Message dynamicDefaultInstance = DynamicMessage.getDefaultInstance(TestMap.getDescriptor());
    Message.Builder builder = dynamicDefaultInstance.newBuilderForType();
    // Add an entry without key and value.
    builder.addRepeatedField(field, builder.newBuilderForField(field).build());
    Message message = builder.build();
    ByteString bytes = message.toByteString();
    // Parse it back to the same generated type.
    Message generatedMessage = TestMap.parseFrom(bytes, ExtensionRegistry.getEmptyRegistry());
    // Assert the serialized bytes are equivalent.
    assertThat(bytes).isEqualTo(generatedMessage.toByteString());
  }

  @Test
  public void testReflectionEqualsAndHashCode() throws Exception {
    // Test that generated equals() and hashCode() will disregard the order
    // of map entries when comparing/hashing map fields.

    // We use DynamicMessage to test reflection based equals()/hashCode().
    Message dynamicDefaultInstance = DynamicMessage.getDefaultInstance(TestMap.getDescriptor());
    FieldDescriptor field = f("int32_to_int32_field");

    Message.Builder b1 = dynamicDefaultInstance.newBuilderForType();
    b1.addRepeatedField(field, newMapEntry(b1, "int32_to_int32_field", 1, 2));
    b1.addRepeatedField(field, newMapEntry(b1, "int32_to_int32_field", 3, 4));
    b1.addRepeatedField(field, newMapEntry(b1, "int32_to_int32_field", 5, 6));
    Message m1 = b1.build();

    Message.Builder b2 = dynamicDefaultInstance.newBuilderForType();
    b2.addRepeatedField(field, newMapEntry(b2, "int32_to_int32_field", 5, 6));
    b2.addRepeatedField(field, newMapEntry(b2, "int32_to_int32_field", 1, 2));
    b2.addRepeatedField(field, newMapEntry(b2, "int32_to_int32_field", 3, 4));
    Message m2 = b2.build();

    assertThat(m2).isEqualTo(m1);
    assertThat(m2.hashCode()).isEqualTo(m1.hashCode());

    // Make sure we did compare map fields.
    b2.setRepeatedField(field, 0, newMapEntry(b1, "int32_to_int32_field", 0, 0));
    m2 = b2.build();
    assertThat(m1.equals(m2)).isFalse();
    // Don't check m1.hashCode() != m2.hashCode() because it's not guaranteed
    // to be different.
  }

  @Test
  public void testUnknownEnumValues() throws Exception {
    TestUnknownEnumValue builder =
        TestUnknownEnumValue.newBuilder()
            .putInt32ToInt32Field(1, 1)
            .putInt32ToInt32Field(2, 54321)
            .build();
    ByteString data = builder.toByteString();

    TestMap message = TestMap.parseFrom(data, ExtensionRegistry.getEmptyRegistry());
    // Entries with unknown enum values will be stored into UnknownFieldSet so
    // there is only one entry in the map.
    assertThat(message.getInt32ToEnumFieldMap()).hasSize(1);
    assertThat(message.getInt32ToEnumFieldMap()).containsEntry(1, TestMap.EnumValue.BAR);
    // UnknownFieldSet should not be empty.
    assertThat(message.getUnknownFields().asMap()).isNotEmpty();
    // Serializing and parsing should preserve the unknown entry.
    data = message.toByteString();
    TestUnknownEnumValue messageWithUnknownEnums =
        TestUnknownEnumValue.parseFrom(data, ExtensionRegistry.getEmptyRegistry());
    assertThat(messageWithUnknownEnums.getInt32ToInt32FieldMap()).hasSize(2);
    assertThat(messageWithUnknownEnums.getInt32ToInt32FieldMap().get(1).intValue()).isEqualTo(1);
    assertThat(messageWithUnknownEnums.getInt32ToInt32FieldMap().get(2).intValue())
        .isEqualTo(54321);
  }

  @Test
  public void testRequiredMessage() throws Exception {
    TestMap.Builder builder = TestMap.newBuilder();
    builder.putRequiredMessageMap(0, MessageWithRequiredFields.newBuilder().buildPartial());
    TestMap message = builder.buildPartial();
    assertThat(message.isInitialized()).isFalse();

    builder.putRequiredMessageMap(0, MessageWithRequiredFields.newBuilder().setValue(1).build());
    message = builder.build();
    assertThat(message.isInitialized()).isTrue();
  }

  @Test
  public void testRecursiveMap() throws Exception {
    TestRecursiveMap.Builder builder = TestRecursiveMap.newBuilder();
    builder.putRecursiveMapField(1, TestRecursiveMap.newBuilder().setValue(2).build());
    builder.putRecursiveMapField(3, TestRecursiveMap.newBuilder().setValue(4).build());
    ByteString data = builder.build().toByteString();

    TestRecursiveMap message =
        TestRecursiveMap.parseFrom(data, ExtensionRegistry.getEmptyRegistry());
    assertThat(message.getRecursiveMapFieldMap().get(1).getValue()).isEqualTo(2);
    assertThat(message.getRecursiveMapFieldMap().get(3).getValue()).isEqualTo(4);
  }

  @Test
  public void testIterationOrder() throws Exception {
    TestMap.Builder builder = TestMap.newBuilder();
    setMapValuesUsingAccessors(builder);
    TestMap message = builder.build();

    assertThat(new ArrayList<String>(message.getStringToInt32FieldMap().keySet()))
        .containsExactly("1", "2", "3")
        .inOrder();
  }

  @Test
  public void testContains() {
    TestMap.Builder builder = TestMap.newBuilder();
    setMapValuesUsingAccessors(builder);
    assertMapContainsSetValues(builder);
    assertMapContainsSetValues(builder.build());
  }

  private void assertMapContainsSetValues(TestMapOrBuilder testMapOrBuilder) {
    assertThat(testMapOrBuilder.containsInt32ToInt32Field(1)).isTrue();
    assertThat(testMapOrBuilder.containsInt32ToInt32Field(2)).isTrue();
    assertThat(testMapOrBuilder.containsInt32ToInt32Field(3)).isTrue();
    assertThat(testMapOrBuilder.containsInt32ToInt32Field(-1)).isFalse();

    assertThat(testMapOrBuilder.containsInt32ToStringField(1)).isTrue();
    assertThat(testMapOrBuilder.containsInt32ToStringField(2)).isTrue();
    assertThat(testMapOrBuilder.containsInt32ToStringField(3)).isTrue();
    assertThat(testMapOrBuilder.containsInt32ToStringField(-1)).isFalse();

    assertThat(testMapOrBuilder.containsInt32ToBytesField(1)).isTrue();
    assertThat(testMapOrBuilder.containsInt32ToBytesField(2)).isTrue();
    assertThat(testMapOrBuilder.containsInt32ToBytesField(3)).isTrue();
    assertThat(testMapOrBuilder.containsInt32ToBytesField(-1)).isFalse();

    assertThat(testMapOrBuilder.containsInt32ToEnumField(1)).isTrue();
    assertThat(testMapOrBuilder.containsInt32ToEnumField(2)).isTrue();
    assertThat(testMapOrBuilder.containsInt32ToEnumField(3)).isTrue();
    assertThat(testMapOrBuilder.containsInt32ToEnumField(-1)).isFalse();

    assertThat(testMapOrBuilder.containsInt32ToMessageField(1)).isTrue();
    assertThat(testMapOrBuilder.containsInt32ToMessageField(2)).isTrue();
    assertThat(testMapOrBuilder.containsInt32ToMessageField(3)).isTrue();
    assertThat(testMapOrBuilder.containsInt32ToMessageField(-1)).isFalse();

    assertThat(testMapOrBuilder.containsStringToInt32Field("1")).isTrue();
    assertThat(testMapOrBuilder.containsStringToInt32Field("2")).isTrue();
    assertThat(testMapOrBuilder.containsStringToInt32Field("3")).isTrue();
    assertThat(testMapOrBuilder.containsStringToInt32Field("-1")).isFalse();
  }

  @Test
  public void testCount() {
    TestMap.Builder builder = TestMap.newBuilder();
    assertMapCounts(0, builder);

    setMapValuesUsingAccessors(builder);
    assertMapCounts(3, builder);

    TestMap message = builder.build();
    assertMapCounts(3, message);

    builder = message.toBuilder().putInt32ToInt32Field(4, 44);
    assertThat(builder.getInt32ToInt32FieldCount()).isEqualTo(4);
    assertThat(builder.build().getInt32ToInt32FieldCount()).isEqualTo(4);

    // already present - should be unchanged
    builder.putInt32ToInt32Field(4, 44);
    assertThat(builder.getInt32ToInt32FieldCount()).isEqualTo(4);
  }

  private void assertMapCounts(int expectedCount, TestMapOrBuilder testMapOrBuilder) {
    assertThat(testMapOrBuilder.getInt32ToInt32FieldCount()).isEqualTo(expectedCount);
    assertThat(testMapOrBuilder.getInt32ToStringFieldCount()).isEqualTo(expectedCount);
    assertThat(testMapOrBuilder.getInt32ToBytesFieldCount()).isEqualTo(expectedCount);
    assertThat(testMapOrBuilder.getInt32ToEnumFieldCount()).isEqualTo(expectedCount);
    assertThat(testMapOrBuilder.getInt32ToMessageFieldCount()).isEqualTo(expectedCount);
    assertThat(testMapOrBuilder.getStringToInt32FieldCount()).isEqualTo(expectedCount);
  }

  @Test
  public void testGetOrDefault() {
    TestMap.Builder builder = TestMap.newBuilder();
    assertMapCounts(0, builder);
    setMapValuesUsingAccessors(builder);
    doTestGetOrDefault(builder);
    doTestGetOrDefault(builder.build());
  }

  public void doTestGetOrDefault(TestMapOrBuilder testMapOrBuilder) {
    assertThat(testMapOrBuilder.getInt32ToInt32FieldOrDefault(1, -11)).isEqualTo(11);
    assertThat(testMapOrBuilder.getInt32ToInt32FieldOrDefault(-1, -11)).isEqualTo(-11);

    assertThat(testMapOrBuilder.getInt32ToStringFieldOrDefault(1, "-11")).isEqualTo("11");
    assertWithMessage("-11")
        .that(testMapOrBuilder.getInt32ToStringFieldOrDefault(-1, null))
        .isNull();

    assertThat(testMapOrBuilder.getInt32ToBytesFieldOrDefault(1, null))
        .isEqualTo(TestUtil.toBytes("11"));
    assertThat(testMapOrBuilder.getInt32ToBytesFieldOrDefault(-1, null)).isNull();

    assertThat(testMapOrBuilder.getInt32ToEnumFieldOrDefault(1, null))
        .isEqualTo(TestMap.EnumValue.FOO);
    assertThat(testMapOrBuilder.getInt32ToEnumFieldOrDefault(-1, null)).isNull();

    assertThat(testMapOrBuilder.getInt32ToMessageFieldOrDefault(1, null))
        .isEqualTo(MessageValue.newBuilder().setValue(11).build());
    assertThat(testMapOrBuilder.getInt32ToMessageFieldOrDefault(-1, null)).isNull();

    assertThat(testMapOrBuilder.getStringToInt32FieldOrDefault("1", -11)).isEqualTo(11);
    assertThat(testMapOrBuilder.getStringToInt32FieldOrDefault("-1", -11)).isEqualTo(-11);

    try {
      testMapOrBuilder.getStringToInt32FieldOrDefault(null, -11);
      assertWithMessage("expected exception").fail();
    } catch (NullPointerException e) {
      // expected
    }
  }

  @Test
  public void testGetOrThrow() {
    TestMap.Builder builder = TestMap.newBuilder();
    assertMapCounts(0, builder);
    setMapValuesUsingAccessors(builder);
    doTestGetOrDefault(builder);
    doTestGetOrDefault(builder.build());
  }

  public void doTestGetOrThrow(TestMapOrBuilder testMapOrBuilder) {
    assertThat(testMapOrBuilder.getInt32ToInt32FieldOrThrow(1)).isEqualTo(11);
    try {
      testMapOrBuilder.getInt32ToInt32FieldOrThrow(-1);
      assertWithMessage("expected exception").fail();
    } catch (IllegalArgumentException e) {
      // expected
    }

    assertThat(testMapOrBuilder.getInt32ToStringFieldOrThrow(1)).isEqualTo("11");

    try {
      testMapOrBuilder.getInt32ToStringFieldOrThrow(-1);
      assertWithMessage("expected exception").fail();
    } catch (IllegalArgumentException e) {
      // expected
    }

    assertThat(testMapOrBuilder.getInt32ToBytesFieldOrThrow(1)).isEqualTo(TestUtil.toBytes("11"));

    try {
      testMapOrBuilder.getInt32ToBytesFieldOrThrow(-1);
      assertWithMessage("expected exception").fail();
    } catch (IllegalArgumentException e) {
      // expected
    }

    assertThat(testMapOrBuilder.getInt32ToEnumFieldOrThrow(1)).isEqualTo(TestMap.EnumValue.FOO);
    try {
      testMapOrBuilder.getInt32ToEnumFieldOrThrow(-1);
      assertWithMessage("expected exception").fail();
    } catch (IllegalArgumentException e) {
      // expected
    }

    assertThat(testMapOrBuilder.getInt32ToMessageFieldOrThrow(1))
        .isEqualTo(MessageValue.newBuilder().setValue(11).build());
    try {
      testMapOrBuilder.getInt32ToMessageFieldOrThrow(-1);
      assertWithMessage("expected exception").fail();
    } catch (IllegalArgumentException e) {
      // expected
    }

    assertThat(testMapOrBuilder.getStringToInt32FieldOrThrow("1")).isEqualTo(11);
    try {
      testMapOrBuilder.getStringToInt32FieldOrThrow("-1");
      assertWithMessage("expected exception").fail();
    } catch (IllegalArgumentException e) {
      // expected
    }

    try {
      testMapOrBuilder.getStringToInt32FieldOrThrow(null);
      assertWithMessage("expected exception").fail();
    } catch (NullPointerException e) {
      // expected
    }
  }

  @Test
  public void testPut() {
    TestMap.Builder builder = TestMap.newBuilder();
    builder.putInt32ToInt32Field(1, 11);
    assertThat(builder.getInt32ToInt32FieldOrThrow(1)).isEqualTo(11);

    builder.putInt32ToStringField(1, "a");
    assertThat(builder.getInt32ToStringFieldOrThrow(1)).isEqualTo("a");
    try {
      builder.putInt32ToStringField(1, null);
      assertWithMessage("expected exception").fail();
    } catch (NullPointerException e) {
      // expected
    }

    builder.putInt32ToBytesField(1, TestUtil.toBytes("11"));
    assertThat(builder.getInt32ToBytesFieldOrThrow(1)).isEqualTo(TestUtil.toBytes("11"));
    try {
      builder.putInt32ToBytesField(1, null);
      assertWithMessage("expected exception").fail();
    } catch (NullPointerException e) {
      // expected
    }

    builder.putInt32ToEnumField(1, TestMap.EnumValue.FOO);
    assertThat(builder.getInt32ToEnumFieldOrThrow(1)).isEqualTo(TestMap.EnumValue.FOO);
    try {
      builder.putInt32ToEnumField(1, null);
      assertWithMessage("expected exception").fail();
    } catch (NullPointerException e) {
      // expected
    }

    builder.putStringToInt32Field("a", 1);
    assertThat(builder.getStringToInt32FieldOrThrow("a")).isEqualTo(1);
    try {
      builder.putStringToInt32Field(null, -1);
    } catch (NullPointerException e) {
      // expected
    }
  }

  @Test
  public void testRemove() {
    TestMap.Builder builder = TestMap.newBuilder();
    setMapValuesUsingAccessors(builder);
    assertThat(builder.getInt32ToInt32FieldOrThrow(1)).isEqualTo(11);
    for (int times = 0; times < 2; times++) {
      builder.removeInt32ToInt32Field(1);
      assertThat(builder.getInt32ToInt32FieldOrDefault(1, -1)).isEqualTo(-1);
    }

    assertThat(builder.getInt32ToStringFieldOrThrow(1)).isEqualTo("11");
    for (int times = 0; times < 2; times++) {
      builder.removeInt32ToStringField(1);
      assertThat(builder.getInt32ToStringFieldOrDefault(1, null)).isNull();
    }

    assertThat(builder.getInt32ToBytesFieldOrThrow(1)).isEqualTo(TestUtil.toBytes("11"));
    for (int times = 0; times < 2; times++) {
      builder.removeInt32ToBytesField(1);
      assertThat(builder.getInt32ToBytesFieldOrDefault(1, null)).isNull();
    }

    assertThat(builder.getInt32ToEnumFieldOrThrow(1)).isEqualTo(TestMap.EnumValue.FOO);
    for (int times = 0; times < 2; times++) {
      builder.removeInt32ToEnumField(1);
      assertThat(builder.getInt32ToEnumFieldOrDefault(1, null)).isNull();
    }

    assertThat(builder.getStringToInt32FieldOrThrow("1")).isEqualTo(11);
    for (int times = 0; times < 2; times++) {
      builder.removeStringToInt32Field("1");
      assertThat(builder.getStringToInt32FieldOrDefault("1", -1)).isEqualTo(-1);
    }

    try {
      builder.removeStringToInt32Field(null);
      assertWithMessage("expected exception").fail();
    } catch (NullPointerException e) {
      // expected
    }
  }

  // Regression test for b/20494788
  @Test
  public void testMapInitializationOrder() throws Exception {
    assertThat(RedactAllTypes.getDefaultInstance().getDescriptorForType().getName())
        .isEqualTo("RedactAllTypes");

    Message1.Builder builder = Message1.newBuilder();
    builder.putMapField("key", true);
    Message1 message = builder.build();
    Message mapEntry =
        (Message)
            message.getRepeatedField(
                message.getDescriptorForType().findFieldByName("map_field"), 0);
    assertThat(mapEntry.getAllFields()).hasSize(2);
  }

  @Test
  public void testReservedWordsFieldNames() {
    ReservedAsMapField unused1 = ReservedAsMapField.getDefaultInstance();
    ReservedAsMapFieldWithEnumValue unused2 = ReservedAsMapFieldWithEnumValue.getDefaultInstance();
  }

  @Test
  public void testGetMap() {
    TestMap.Builder builder = TestMap.newBuilder();
    setMapValuesUsingAccessors(builder);
    assertMapValuesSet(builder);
    TestMap message = builder.build();
    assertThat(message.getStringToInt32FieldMap()).isEqualTo(message.getStringToInt32FieldMap());
    assertThat(message.getInt32ToBytesFieldMap()).isEqualTo(message.getInt32ToBytesFieldMap());
    assertThat(message.getInt32ToEnumFieldMap()).isEqualTo(message.getInt32ToEnumFieldMap());
    assertThat(message.getInt32ToMessageFieldMap()).isEqualTo(message.getInt32ToMessageFieldMap());
  }

  @Test
  public void testPutAllWithNullStringValue() throws Exception {
    TestMap.Builder sourceBuilder = TestMap.newBuilder();

    // order preserving map used here to help test rollback
    Map<Integer, String> data = new TreeMap<>();
    data.put(7, "foo");
    data.put(8, "bar");
    data.put(9, null);
    try {
      sourceBuilder.putAllInt32ToStringField(data);
      fail("allowed null string value");
    } catch (NullPointerException expected) {
      // Verify rollback of previously added values.
      // They all go in or none do.
      assertThat(sourceBuilder.getInt32ToStringFieldMap()).isEmpty();
    }
  }

  @Test
  public void testPutAllWithNullStringKey() throws Exception {
    TestMap.Builder sourceBuilder = TestMap.newBuilder();

    // order preserving map used here to help test rollback
    Map<Integer, String> data = new LinkedHashMap<>();
    data.put(7, "foo");
    data.put(null, "bar");
    data.put(9, "baz");
    try {
      sourceBuilder.putAllInt32ToStringField(data);
      fail("allowed null string key");
    } catch (NullPointerException expected) {
      // Verify rollback of previously added values.
      // They all go in or none do.
      assertThat(sourceBuilder.getInt32ToStringFieldMap()).isEmpty();
    }
  }

  @Test
  public void testPutNullStringValue() throws Exception {
    TestMap.Builder sourceBuilder = TestMap.newBuilder();

    try {
      sourceBuilder.putInt32ToStringField(8, null);
      fail("allowed null string value");
    } catch (NullPointerException expected) {
      assertNotNull(expected.getMessage());
    }
  }
}
