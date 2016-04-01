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

import map_lite_test.MapForProto2TestProto.TestMap;
import map_lite_test.MapForProto2TestProto.TestMap.MessageValue;
import map_lite_test.MapForProto2TestProto.TestUnknownEnumValue;

import junit.framework.TestCase;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashMap;
import java.util.Map;

/**
 * Unit tests for map fields.
 */
public class MapForProto2LiteTest extends TestCase {
  private void setMapValues(TestMap.Builder builder) {
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

  private void copyMapValues(TestMap source, TestMap.Builder destination) {
    destination
        .putAllInt32ToInt32Field(source.getInt32ToInt32Field())
        .putAllInt32ToStringField(source.getInt32ToStringField())
        .putAllInt32ToBytesField(source.getInt32ToBytesField())
        .putAllInt32ToEnumField(source.getInt32ToEnumField())
        .putAllInt32ToMessageField(source.getInt32ToMessageField())
        .putAllStringToInt32Field(source.getStringToInt32Field());
  }

  private void assertMapValuesSet(TestMap message) {
    assertEquals(3, message.getInt32ToInt32Field().size());
    assertEquals(11, message.getInt32ToInt32Field().get(1).intValue());
    assertEquals(22, message.getInt32ToInt32Field().get(2).intValue());
    assertEquals(33, message.getInt32ToInt32Field().get(3).intValue());

    assertEquals(3, message.getInt32ToStringField().size());
    assertEquals("11", message.getInt32ToStringField().get(1));
    assertEquals("22", message.getInt32ToStringField().get(2));
    assertEquals("33", message.getInt32ToStringField().get(3));
    
    assertEquals(3, message.getInt32ToBytesField().size());
    assertEquals(TestUtil.toBytes("11"), message.getInt32ToBytesField().get(1));
    assertEquals(TestUtil.toBytes("22"), message.getInt32ToBytesField().get(2));
    assertEquals(TestUtil.toBytes("33"), message.getInt32ToBytesField().get(3));
    
    assertEquals(3, message.getInt32ToEnumField().size());
    assertEquals(TestMap.EnumValue.FOO, message.getInt32ToEnumField().get(1));
    assertEquals(TestMap.EnumValue.BAR, message.getInt32ToEnumField().get(2));
    assertEquals(TestMap.EnumValue.BAZ, message.getInt32ToEnumField().get(3));
    
    assertEquals(3, message.getInt32ToMessageField().size());
    assertEquals(11, message.getInt32ToMessageField().get(1).getValue());
    assertEquals(22, message.getInt32ToMessageField().get(2).getValue());
    assertEquals(33, message.getInt32ToMessageField().get(3).getValue());
    
    assertEquals(3, message.getStringToInt32Field().size());
    assertEquals(11, message.getStringToInt32Field().get("1").intValue());
    assertEquals(22, message.getStringToInt32Field().get("2").intValue());
    assertEquals(33, message.getStringToInt32Field().get("3").intValue());
  }

  private void updateMapValues(TestMap.Builder builder) {
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

  private void assertMapValuesUpdated(TestMap message) {
    assertEquals(3, message.getInt32ToInt32Field().size());
    assertEquals(111, message.getInt32ToInt32Field().get(1).intValue());
    assertEquals(33, message.getInt32ToInt32Field().get(3).intValue());
    assertEquals(44, message.getInt32ToInt32Field().get(4).intValue());

    assertEquals(3, message.getInt32ToStringField().size());
    assertEquals("111", message.getInt32ToStringField().get(1));
    assertEquals("33", message.getInt32ToStringField().get(3));
    assertEquals("44", message.getInt32ToStringField().get(4));
    
    assertEquals(3, message.getInt32ToBytesField().size());
    assertEquals(TestUtil.toBytes("111"), message.getInt32ToBytesField().get(1));
    assertEquals(TestUtil.toBytes("33"), message.getInt32ToBytesField().get(3));
    assertEquals(TestUtil.toBytes("44"), message.getInt32ToBytesField().get(4));
    
    assertEquals(3, message.getInt32ToEnumField().size());
    assertEquals(TestMap.EnumValue.BAR, message.getInt32ToEnumField().get(1));
    assertEquals(TestMap.EnumValue.BAZ, message.getInt32ToEnumField().get(3));
    assertEquals(TestMap.EnumValue.QUX, message.getInt32ToEnumField().get(4));
    
    assertEquals(3, message.getInt32ToMessageField().size());
    assertEquals(111, message.getInt32ToMessageField().get(1).getValue());
    assertEquals(33, message.getInt32ToMessageField().get(3).getValue());
    assertEquals(44, message.getInt32ToMessageField().get(4).getValue());
    
    assertEquals(3, message.getStringToInt32Field().size());
    assertEquals(111, message.getStringToInt32Field().get("1").intValue());
    assertEquals(33, message.getStringToInt32Field().get("3").intValue());
    assertEquals(44, message.getStringToInt32Field().get("4").intValue());
  }

  private void assertMapValuesCleared(TestMap message) {
    assertEquals(0, message.getInt32ToInt32Field().size());
    assertEquals(0, message.getInt32ToStringField().size());
    assertEquals(0, message.getInt32ToBytesField().size());
    assertEquals(0, message.getInt32ToEnumField().size());
    assertEquals(0, message.getInt32ToMessageField().size());
    assertEquals(0, message.getStringToInt32Field().size());
  }

  public void testSanityCopyOnWrite() throws InvalidProtocolBufferException {
    // Since builders are implemented as a thin wrapper around a message
    // instance, we attempt to verify that we can't cause the builder to modify
    // a produced message.
    
    TestMap.Builder builder = TestMap.newBuilder();
    TestMap message = builder.build();
    Map<Integer, Integer> intMap = builder.getMutableInt32ToInt32Field();
    intMap.put(1, 2);
    assertTrue(message.getInt32ToInt32Field().isEmpty());
    message = builder.build();
    try {
      intMap.put(2, 3);
      fail();
    } catch (UnsupportedOperationException e) {
      // expected
    }
    assertEquals(newMap(1, 2), message.getInt32ToInt32Field());
    assertEquals(newMap(1, 2), builder.getInt32ToInt32Field());
    builder.getMutableInt32ToInt32Field().put(2, 3);
    assertEquals(newMap(1, 2), message.getInt32ToInt32Field());
    assertEquals(newMap(1, 2, 2, 3), builder.getInt32ToInt32Field());
  }
  
  public void testMutableMapLifecycle() {
    TestMap.Builder builder = TestMap.newBuilder();
    Map<Integer, Integer> intMap = builder.getMutableInt32ToInt32Field();
    intMap.put(1, 2);
    assertEquals(newMap(1, 2), builder.build().getInt32ToInt32Field());
    try {
      intMap.put(2, 3);
      fail();
    } catch (UnsupportedOperationException e) {
      // expected
    }
    assertEquals(newMap(1, 2), builder.getInt32ToInt32Field());
    builder.getMutableInt32ToInt32Field().put(2, 3);
    assertEquals(newMap(1, 2, 2, 3), builder.getInt32ToInt32Field());

    Map<Integer, TestMap.EnumValue> enumMap = builder.getMutableInt32ToEnumField();
    enumMap.put(1, TestMap.EnumValue.BAR);
    assertEquals(newMap(1, TestMap.EnumValue.BAR), builder.build().getInt32ToEnumField());
    try {
      enumMap.put(2, TestMap.EnumValue.FOO);
      fail();
    } catch (UnsupportedOperationException e) {
      // expected
    }
    assertEquals(newMap(1, TestMap.EnumValue.BAR), builder.getInt32ToEnumField());
    builder.getMutableInt32ToEnumField().put(2, TestMap.EnumValue.FOO);
    assertEquals(
        newMap(1, TestMap.EnumValue.BAR, 2, TestMap.EnumValue.FOO),
        builder.getInt32ToEnumField());
    
    Map<Integer, String> stringMap = builder.getMutableInt32ToStringField();
    stringMap.put(1, "1");
    assertEquals(newMap(1, "1"), builder.build().getInt32ToStringField());
    try {
      stringMap.put(2, "2");
      fail();
    } catch (UnsupportedOperationException e) {
      // expected
    }
    assertEquals(newMap(1, "1"), builder.getInt32ToStringField());
    builder.getMutableInt32ToStringField().put(2, "2");
    assertEquals(
        newMap(1, "1", 2, "2"),
        builder.getInt32ToStringField());
    
    Map<Integer, TestMap.MessageValue> messageMap = builder.getMutableInt32ToMessageField();
    messageMap.put(1, TestMap.MessageValue.getDefaultInstance());
    assertEquals(newMap(1, TestMap.MessageValue.getDefaultInstance()),
        builder.build().getInt32ToMessageField());
    try {
      messageMap.put(2, TestMap.MessageValue.getDefaultInstance());
      fail();
    } catch (UnsupportedOperationException e) {
      // expected
    }
    assertEquals(newMap(1, TestMap.MessageValue.getDefaultInstance()),
        builder.getInt32ToMessageField());
    builder.getMutableInt32ToMessageField().put(2, TestMap.MessageValue.getDefaultInstance());
    assertEquals(
        newMap(1, TestMap.MessageValue.getDefaultInstance(),
            2, TestMap.MessageValue.getDefaultInstance()),
        builder.getInt32ToMessageField());
  }

  public void testMutableMapLifecycle_collections() {
    TestMap.Builder builder = TestMap.newBuilder();
    Map<Integer, Integer> intMap = builder.getMutableInt32ToInt32Field();
    intMap.put(1, 2);
    assertEquals(newMap(1, 2), builder.build().getInt32ToInt32Field());
    try {
      intMap.remove(2);
      fail();
    } catch (UnsupportedOperationException e) {
      // expected
    }
    try {
      intMap.entrySet().remove(new Object());
      fail();
    } catch (UnsupportedOperationException e) {
      // expected
    }
    try {
      intMap.entrySet().iterator().remove();
      fail();
    } catch (UnsupportedOperationException e) {
      // expected
    }
    try {
      intMap.keySet().remove(new Object());
      fail();
    } catch (UnsupportedOperationException e) {
      // expected
    }
    try {
      intMap.values().remove(new Object());
      fail();
    } catch (UnsupportedOperationException e) {
      // expected
    }
    try {
      intMap.values().iterator().remove();
      fail();
    } catch (UnsupportedOperationException e) {
      // expected
    }
    assertEquals(newMap(1, 2), intMap);
    assertEquals(newMap(1, 2), builder.getInt32ToInt32Field());
    assertEquals(newMap(1, 2), builder.build().getInt32ToInt32Field());
  }

  public void testGettersAndSetters() throws Exception {
    TestMap.Builder builder = TestMap.newBuilder();
    TestMap message = builder.build();
    assertMapValuesCleared(message);
    
    builder = message.toBuilder();
    setMapValues(builder);
    message = builder.build();
    assertMapValuesSet(message);
    
    builder = message.toBuilder();
    updateMapValues(builder);
    message = builder.build();
    assertMapValuesUpdated(message);
    
    builder = message.toBuilder();
    builder.clear();
    message = builder.build();
    assertMapValuesCleared(message);
  }

  public void testPutAll() throws Exception {
    TestMap.Builder sourceBuilder = TestMap.newBuilder();
    setMapValues(sourceBuilder);
    TestMap source = sourceBuilder.build();

    TestMap.Builder destination = TestMap.newBuilder();
    copyMapValues(source, destination);
    assertMapValuesSet(destination.build());
  }

  public void testSerializeAndParse() throws Exception {
    TestMap.Builder builder = TestMap.newBuilder();
    setMapValues(builder);
    TestMap message = builder.build();
    assertEquals(message.getSerializedSize(), message.toByteString().size());
    message = TestMap.parser().parseFrom(message.toByteString());
    assertMapValuesSet(message);
    
    builder = message.toBuilder();
    updateMapValues(builder);
    message = builder.build();
    assertEquals(message.getSerializedSize(), message.toByteString().size());
    message = TestMap.parser().parseFrom(message.toByteString());
    assertMapValuesUpdated(message);
    
    builder = message.toBuilder();
    builder.clear();
    message = builder.build();
    assertEquals(message.getSerializedSize(), message.toByteString().size());
    message = TestMap.parser().parseFrom(message.toByteString());
    assertMapValuesCleared(message);
  }
  
  public void testMergeFrom() throws Exception {
    TestMap.Builder builder = TestMap.newBuilder();
    setMapValues(builder);
    TestMap message = builder.build();
    
    TestMap.Builder other = TestMap.newBuilder();
    other.mergeFrom(message);
    assertMapValuesSet(other.build());
  }

  public void testEqualsAndHashCode() throws Exception {
    // Test that generated equals() and hashCode() will disregard the order
    // of map entries when comparing/hashing map fields.
    
    // We can't control the order of elements in a HashMap. The best we can do
    // here is to add elements in different order.
    TestMap.Builder b1 = TestMap.newBuilder();
    b1.getMutableInt32ToInt32Field().put(1, 2);
    b1.getMutableInt32ToInt32Field().put(3, 4);
    b1.getMutableInt32ToInt32Field().put(5, 6);
    TestMap m1 = b1.build();
    
    TestMap.Builder b2 = TestMap.newBuilder();
    b2.getMutableInt32ToInt32Field().put(5, 6);
    b2.getMutableInt32ToInt32Field().put(1, 2);
    b2.getMutableInt32ToInt32Field().put(3, 4);
    TestMap m2 = b2.build();
    
    assertEquals(m1, m2);
    assertEquals(m1.hashCode(), m2.hashCode());
    
    // Make sure we did compare map fields.
    b2.getMutableInt32ToInt32Field().put(1, 0);
    m2 = b2.build();
    assertFalse(m1.equals(m2));
    // Don't check m1.hashCode() != m2.hashCode() because it's not guaranteed
    // to be different.
  }

  public void testUnknownEnumValues() throws Exception {
    TestUnknownEnumValue.Builder builder =
        TestUnknownEnumValue.newBuilder();
    builder.getMutableInt32ToInt32Field().put(1, 1);
    builder.getMutableInt32ToInt32Field().put(2, 54321);
    ByteString data = builder.build().toByteString();

    TestMap message = TestMap.parseFrom(data);
    // Entries with unknown enum values will be stored into UnknownFieldSet so
    // there is only one entry in the map.
    assertEquals(1, message.getInt32ToEnumField().size());
    assertEquals(TestMap.EnumValue.BAR, message.getInt32ToEnumField().get(1));
    // Serializing and parsing should preserve the unknown entry.
    data = message.toByteString();
    TestUnknownEnumValue messageWithUnknownEnums =
        TestUnknownEnumValue.parseFrom(data);
    assertEquals(2, messageWithUnknownEnums.getInt32ToInt32Field().size());
    assertEquals(1, messageWithUnknownEnums.getInt32ToInt32Field().get(1).intValue());
    assertEquals(54321, messageWithUnknownEnums.getInt32ToInt32Field().get(2).intValue());
  }


  public void testIterationOrder() throws Exception {
    TestMap.Builder builder = TestMap.newBuilder();
    setMapValues(builder);
    TestMap message = builder.build();

    assertEquals(Arrays.asList("1", "2", "3"),
        new ArrayList<String>(message.getStringToInt32Field().keySet()));
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
}
