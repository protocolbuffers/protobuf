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

import static org.junit.Assert.assertArrayEquals;

import map_lite_test.MapTestProto.BizarroTestMap;
import map_lite_test.MapTestProto.TestMap;
import map_lite_test.MapTestProto.TestMap.MessageValue;
import map_lite_test.MapTestProto.TestMapOrBuilder;
import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashMap;
import java.util.Map;
import junit.framework.TestCase;

/** Unit tests for map fields. */
public final class MapLiteTest extends TestCase {

  private void setMapValues(TestMap.Builder builder) {
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

  public void testSetMapValues() {
    TestMap.Builder usingMutableMapBuilder = TestMap.newBuilder();
    setMapValues(usingMutableMapBuilder);
    TestMap usingMutableMap = usingMutableMapBuilder.build();
    assertMapValuesSet(usingMutableMap);

    TestMap.Builder usingAccessorsBuilder = TestMap.newBuilder();
    setMapValues(usingAccessorsBuilder);
    TestMap usingAccessors = usingAccessorsBuilder.build();
    assertMapValuesSet(usingAccessors);
    assertEquals(usingAccessors, usingMutableMap);
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

  public void testUpdateMapValues() {
    TestMap.Builder mapBuilder = TestMap.newBuilder();
    setMapValues(mapBuilder);
    TestMap map = mapBuilder.build();
    assertMapValuesSet(map);

    mapBuilder = map.toBuilder();
    updateMapValues(mapBuilder);
    map = mapBuilder.build();
    assertMapValuesUpdated(map);
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

  private void assertMapValuesCleared(TestMapOrBuilder testMapOrBuilder) {
    assertEquals(0, testMapOrBuilder.getInt32ToInt32Field().size());
    assertEquals(0, testMapOrBuilder.getInt32ToInt32FieldCount());
    assertEquals(0, testMapOrBuilder.getInt32ToStringField().size());
    assertEquals(0, testMapOrBuilder.getInt32ToStringFieldCount());
    assertEquals(0, testMapOrBuilder.getInt32ToBytesField().size());
    assertEquals(0, testMapOrBuilder.getInt32ToBytesFieldCount());
    assertEquals(0, testMapOrBuilder.getInt32ToEnumField().size());
    assertEquals(0, testMapOrBuilder.getInt32ToEnumFieldCount());
    assertEquals(0, testMapOrBuilder.getInt32ToMessageField().size());
    assertEquals(0, testMapOrBuilder.getInt32ToMessageFieldCount());
    assertEquals(0, testMapOrBuilder.getStringToInt32Field().size());
    assertEquals(0, testMapOrBuilder.getStringToInt32FieldCount());
  }

  public void testSanityCopyOnWrite() throws InvalidProtocolBufferException {
    // Since builders are implemented as a thin wrapper around a message
    // instance, we attempt to verify that we can't cause the builder to modify
    // a produced message.

    TestMap.Builder builder = TestMap.newBuilder();
    TestMap message = builder.build();
    builder.putInt32ToInt32Field(1, 2);
    assertTrue(message.getInt32ToInt32Field().isEmpty());
    assertEquals(newMap(1, 2), builder.getInt32ToInt32Field());
    message = builder.build();
    builder.putInt32ToInt32Field(2, 3);
    assertEquals(newMap(1, 2), message.getInt32ToInt32Field());
    assertEquals(newMap(1, 2, 2, 3), builder.getInt32ToInt32Field());
  }

  public void testGetMapIsImmutable() {
    TestMap.Builder builder = TestMap.newBuilder();
    assertMapsAreImmutable(builder);
    assertMapsAreImmutable(builder.build());

    setMapValues(builder);
    assertMapsAreImmutable(builder);
    assertMapsAreImmutable(builder.build());
  }

  private void assertMapsAreImmutable(TestMapOrBuilder testMapOrBuilder) {
    assertImmutable(testMapOrBuilder.getInt32ToInt32Field(), 1, 2);
    assertImmutable(testMapOrBuilder.getInt32ToStringField(), 1, "2");
    assertImmutable(testMapOrBuilder.getInt32ToBytesField(), 1, TestUtil.toBytes("2"));
    assertImmutable(testMapOrBuilder.getInt32ToEnumField(), 1, TestMap.EnumValue.FOO);
    assertImmutable(
        testMapOrBuilder.getInt32ToMessageField(), 1, MessageValue.getDefaultInstance());
    assertImmutable(testMapOrBuilder.getStringToInt32Field(), "1", 2);
  }

  private <K, V> void assertImmutable(Map<K, V> map, K key, V value) {
    try {
      map.put(key, value);
      fail();
    } catch (UnsupportedOperationException e) {
      // expected
    }
    if (!map.isEmpty()) {
      try {
        map.entrySet().remove(map.entrySet().iterator().next());
        fail();
      } catch (UnsupportedOperationException e) {
        // expected
      }
    }
  }

  public void testMapFieldClear() {
    TestMap.Builder builder = TestMap.newBuilder().putInt32ToInt32Field(1, 2);
    builder.clearInt32ToInt32Field();
    assertEquals(0, builder.getInt32ToInt32FieldCount());
  }

  public void testMutableMapLifecycle() {
    TestMap.Builder builder = TestMap.newBuilder().putInt32ToInt32Field(1, 2);
    assertEquals(newMap(1, 2), builder.build().getInt32ToInt32Field());
    assertEquals(newMap(1, 2), builder.getInt32ToInt32Field());
    builder.putInt32ToInt32Field(2, 3);
    assertEquals(newMap(1, 2, 2, 3), builder.getInt32ToInt32Field());

    builder.putInt32ToEnumField(1, TestMap.EnumValue.BAR);
    assertEquals(newMap(1, TestMap.EnumValue.BAR), builder.build().getInt32ToEnumField());
    assertEquals(newMap(1, TestMap.EnumValue.BAR), builder.getInt32ToEnumField());
    builder.putInt32ToEnumField(2, TestMap.EnumValue.FOO);
    assertEquals(
        newMap(1, TestMap.EnumValue.BAR, 2, TestMap.EnumValue.FOO), builder.getInt32ToEnumField());

    builder.putInt32ToStringField(1, "1");
    assertEquals(newMap(1, "1"), builder.build().getInt32ToStringField());
    assertEquals(newMap(1, "1"), builder.getInt32ToStringField());
    builder.putInt32ToStringField(2, "2");
    assertEquals(newMap(1, "1", 2, "2"), builder.getInt32ToStringField());

    builder.putInt32ToMessageField(1, TestMap.MessageValue.getDefaultInstance());
    assertEquals(
        newMap(1, TestMap.MessageValue.getDefaultInstance()),
        builder.build().getInt32ToMessageField());
    assertEquals(
        newMap(1, TestMap.MessageValue.getDefaultInstance()), builder.getInt32ToMessageField());
    builder.putInt32ToMessageField(2, TestMap.MessageValue.getDefaultInstance());
    assertEquals(
        newMap(
            1,
            TestMap.MessageValue.getDefaultInstance(),
            2,
            TestMap.MessageValue.getDefaultInstance()),
        builder.getInt32ToMessageField());
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
    assertMapValuesCleared(builder);
    message = builder.build();
    assertMapValuesCleared(message);
  }

  public void testPutAll() throws Exception {
    TestMap.Builder sourceBuilder = TestMap.newBuilder();
    setMapValues(sourceBuilder);
    TestMap source = sourceBuilder.build();
    assertMapValuesSet(source);

    TestMap.Builder destination = TestMap.newBuilder();
    copyMapValues(source, destination);
    assertMapValuesSet(destination.build());
  }

  public void testPutAllForUnknownEnumValues() throws Exception {
    TestMap.Builder sourceBuilder =
        TestMap.newBuilder()
            .putInt32ToEnumFieldValue(0, 0)
            .putInt32ToEnumFieldValue(1, 1)
            .putAllInt32ToEnumFieldValue(newMap(2, 1000)); // unknown value.
    TestMap source = sourceBuilder.build();

    TestMap.Builder destinationBuilder = TestMap.newBuilder();
    destinationBuilder.putAllInt32ToEnumFieldValue(source.getInt32ToEnumFieldValue());
    TestMap destination = destinationBuilder.build();

    assertEquals(0, destination.getInt32ToEnumFieldValue().get(0).intValue());
    assertEquals(1, destination.getInt32ToEnumFieldValue().get(1).intValue());
    assertEquals(1000, destination.getInt32ToEnumFieldValue().get(2).intValue());
    assertEquals(3, destination.getInt32ToEnumFieldCount());
  }

  public void testPutForUnknownEnumValues() throws Exception {
    TestMap.Builder builder =
        TestMap.newBuilder()
            .putInt32ToEnumFieldValue(0, 0)
            .putInt32ToEnumFieldValue(1, 1)
            .putInt32ToEnumFieldValue(2, 1000); // unknown value.
    TestMap message = builder.build();

    assertEquals(0, message.getInt32ToEnumFieldValueOrThrow(0));
    assertEquals(1, message.getInt32ToEnumFieldValueOrThrow(1));
    assertEquals(1000, message.getInt32ToEnumFieldValueOrThrow(2));
    assertEquals(3, message.getInt32ToEnumFieldCount());
  }

  public void testPutChecksNullKeysAndValues() throws Exception {
    TestMap.Builder builder = TestMap.newBuilder();

    try {
      builder.putInt32ToStringField(1, null);
      fail();
    } catch (NullPointerException e) {
      // expected.
    }

    try {
      builder.putInt32ToBytesField(1, null);
      fail();
    } catch (NullPointerException e) {
      // expected.
    }

    try {
      builder.putInt32ToEnumField(1, null);
      fail();
    } catch (NullPointerException e) {
      // expected.
    }

    try {
      builder.putInt32ToMessageField(1, null);
      fail();
    } catch (NullPointerException e) {
      // expected.
    }

    try {
      builder.putStringToInt32Field(null, 1);
      fail();
    } catch (NullPointerException e) {
      // expected.
    }
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

  private TestMap tryParseTestMap(BizarroTestMap bizarroMap) throws IOException {
    ByteArrayOutputStream byteArrayOutputStream = new ByteArrayOutputStream();
    CodedOutputStream output = CodedOutputStream.newInstance(byteArrayOutputStream);
    bizarroMap.writeTo(output);
    output.flush();
    return TestMap.parser().parseFrom(ByteString.copyFrom(byteArrayOutputStream.toByteArray()));
  }

  public void testParseError() throws Exception {
    ByteString bytes = TestUtil.toBytes("SOME BYTES");
    String stringKey = "a string key";

    TestMap map =
        tryParseTestMap(BizarroTestMap.newBuilder().putInt32ToInt32Field(5, bytes).build());
    assertEquals(0, map.getInt32ToInt32FieldOrDefault(5, -1));

    map = tryParseTestMap(BizarroTestMap.newBuilder().putInt32ToStringField(stringKey, 5).build());
    assertEquals("", map.getInt32ToStringFieldOrDefault(0, null));

    map = tryParseTestMap(BizarroTestMap.newBuilder().putInt32ToBytesField(stringKey, 5).build());
    assertEquals(map.getInt32ToBytesFieldOrDefault(0, null), ByteString.EMPTY);

    map =
        tryParseTestMap(BizarroTestMap.newBuilder().putInt32ToEnumField(stringKey, bytes).build());
    assertEquals(TestMap.EnumValue.FOO, map.getInt32ToEnumFieldOrDefault(0, null));

    try {
      tryParseTestMap(BizarroTestMap.newBuilder().putInt32ToMessageField(stringKey, bytes).build());
      fail();
    } catch (InvalidProtocolBufferException expected) {
      assertTrue(expected.getUnfinishedMessage() instanceof TestMap);
      map = (TestMap) expected.getUnfinishedMessage();
      assertTrue(map.getInt32ToMessageField().isEmpty());
    }

    map =
        tryParseTestMap(
            BizarroTestMap.newBuilder().putStringToInt32Field(stringKey, bytes).build());
    assertEquals(0, map.getStringToInt32FieldOrDefault(stringKey, -1));
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
    TestMap.Builder b1 =
        TestMap.newBuilder()
            .putInt32ToInt32Field(1, 2)
            .putInt32ToInt32Field(3, 4)
            .putInt32ToInt32Field(5, 6);
    TestMap m1 = b1.build();

    TestMap.Builder b2 =
        TestMap.newBuilder()
            .putInt32ToInt32Field(5, 6)
            .putInt32ToInt32Field(1, 2)
            .putInt32ToInt32Field(3, 4);
    TestMap m2 = b2.build();

    assertEquals(m1, m2);
    assertEquals(m1.hashCode(), m2.hashCode());

    // Make sure we did compare map fields.
    b2.putInt32ToInt32Field(1, 0);
    m2 = b2.build();
    assertFalse(m1.equals(m2));
    // Don't check m1.hashCode() != m2.hashCode() because it's not guaranteed
    // to be different.

    // Regression test for b/18549190: if a map is a subset of the other map,
    // equals() should return false.
    b2.removeInt32ToInt32Field(1);
    m2 = b2.build();
    assertFalse(m1.equals(m2));
    assertFalse(m2.equals(m1));
  }

  public void testUnknownEnumValues() throws Exception {
    TestMap.Builder builder =
        TestMap.newBuilder()
            .putInt32ToEnumFieldValue(0, 0)
            .putInt32ToEnumFieldValue(1, 1)
            .putInt32ToEnumFieldValue(2, 1000); // unknown value.
    TestMap message = builder.build();

    assertEquals(TestMap.EnumValue.FOO, message.getInt32ToEnumField().get(0));
    assertEquals(TestMap.EnumValue.BAR, message.getInt32ToEnumField().get(1));
    assertEquals(TestMap.EnumValue.UNRECOGNIZED, message.getInt32ToEnumField().get(2));

    builder.putAllInt32ToEnumFieldValue(newMap(2, 1000)); // unknown value.
    message = builder.build();
    assertEquals(TestMap.EnumValue.UNRECOGNIZED, message.getInt32ToEnumField().get(2));

    // Unknown enum values should be preserved after:
    //   1. Serialization and parsing.
    //   2. toBuild().
    //   3. mergeFrom().
    message = TestMap.parseFrom(message.toByteString());
    assertEquals(1000, message.getInt32ToEnumFieldValue().get(2).intValue());
    builder = message.toBuilder();
    assertEquals(1000, builder.getInt32ToEnumFieldValue().get(2).intValue());
    builder = TestMap.newBuilder().mergeFrom(message);
    assertEquals(1000, builder.getInt32ToEnumFieldValue().get(2).intValue());

    // hashCode()/equals() should take unknown enum values into account.
    builder.putAllInt32ToEnumFieldValue(newMap(2, 1001));
    TestMap message2 = builder.build();
    assertFalse(message.hashCode() == message2.hashCode());
    assertFalse(message.equals(message2));
    // Unknown values will be converted to UNRECOGNIZED so the resulted enum map
    // should be the same.
    assertEquals(message2.getInt32ToEnumField(), message.getInt32ToEnumField());
  }

  public void testIterationOrder() throws Exception {
    TestMap.Builder builder = TestMap.newBuilder();
    setMapValues(builder);
    TestMap message = builder.build();

    assertEquals(
        Arrays.asList("1", "2", "3"), new ArrayList<>(message.getStringToInt32Field().keySet()));
  }

  public void testGetMap() {
    TestMap.Builder builder = TestMap.newBuilder();
    setMapValues(builder);
    TestMap message = builder.build();
    assertEquals(message.getStringToInt32Field(), message.getStringToInt32FieldMap());
    assertEquals(message.getInt32ToBytesField(), message.getInt32ToBytesFieldMap());
    assertEquals(message.getInt32ToEnumField(), message.getInt32ToEnumFieldMap());
    assertEquals(message.getInt32ToEnumFieldValue(), message.getInt32ToEnumFieldValueMap());
    assertEquals(message.getInt32ToMessageField(), message.getInt32ToMessageFieldMap());
  }

  public void testContains() {
    TestMap.Builder builder = TestMap.newBuilder();
    setMapValues(builder);
    assertMapContainsSetValues(builder);
    assertMapContainsSetValues(builder.build());
  }

  private void assertMapContainsSetValues(TestMapOrBuilder testMapOrBuilder) {
    assertTrue(testMapOrBuilder.containsInt32ToInt32Field(1));
    assertTrue(testMapOrBuilder.containsInt32ToInt32Field(2));
    assertTrue(testMapOrBuilder.containsInt32ToInt32Field(3));
    assertFalse(testMapOrBuilder.containsInt32ToInt32Field(-1));

    assertTrue(testMapOrBuilder.containsInt32ToStringField(1));
    assertTrue(testMapOrBuilder.containsInt32ToStringField(2));
    assertTrue(testMapOrBuilder.containsInt32ToStringField(3));
    assertFalse(testMapOrBuilder.containsInt32ToStringField(-1));

    assertTrue(testMapOrBuilder.containsInt32ToBytesField(1));
    assertTrue(testMapOrBuilder.containsInt32ToBytesField(2));
    assertTrue(testMapOrBuilder.containsInt32ToBytesField(3));
    assertFalse(testMapOrBuilder.containsInt32ToBytesField(-1));

    assertTrue(testMapOrBuilder.containsInt32ToEnumField(1));
    assertTrue(testMapOrBuilder.containsInt32ToEnumField(2));
    assertTrue(testMapOrBuilder.containsInt32ToEnumField(3));
    assertFalse(testMapOrBuilder.containsInt32ToEnumField(-1));

    assertTrue(testMapOrBuilder.containsInt32ToMessageField(1));
    assertTrue(testMapOrBuilder.containsInt32ToMessageField(2));
    assertTrue(testMapOrBuilder.containsInt32ToMessageField(3));
    assertFalse(testMapOrBuilder.containsInt32ToMessageField(-1));

    assertTrue(testMapOrBuilder.containsStringToInt32Field("1"));
    assertTrue(testMapOrBuilder.containsStringToInt32Field("2"));
    assertTrue(testMapOrBuilder.containsStringToInt32Field("3"));
    assertFalse(testMapOrBuilder.containsStringToInt32Field("-1"));
  }

  public void testCount() {
    TestMap.Builder builder = TestMap.newBuilder();
    assertMapCounts(0, builder);

    setMapValues(builder);
    assertMapCounts(3, builder);

    TestMap message = builder.build();
    assertMapCounts(3, message);

    builder = message.toBuilder().putInt32ToInt32Field(4, 44);
    assertEquals(4, builder.getInt32ToInt32FieldCount());
    assertEquals(4, builder.build().getInt32ToInt32FieldCount());

    // already present - should be unchanged
    builder.putInt32ToInt32Field(4, 44);
    assertEquals(4, builder.getInt32ToInt32FieldCount());
  }

  private void assertMapCounts(int expectedCount, TestMapOrBuilder testMapOrBuilder) {
    assertEquals(expectedCount, testMapOrBuilder.getInt32ToInt32FieldCount());
    assertEquals(expectedCount, testMapOrBuilder.getInt32ToStringFieldCount());
    assertEquals(expectedCount, testMapOrBuilder.getInt32ToBytesFieldCount());
    assertEquals(expectedCount, testMapOrBuilder.getInt32ToEnumFieldCount());
    assertEquals(expectedCount, testMapOrBuilder.getInt32ToMessageFieldCount());
    assertEquals(expectedCount, testMapOrBuilder.getStringToInt32FieldCount());
  }

  public void testGetOrDefault() {
    TestMap.Builder builder = TestMap.newBuilder();
    assertMapCounts(0, builder);
    setMapValues(builder);
    doTestGetOrDefault(builder);
    doTestGetOrDefault(builder.build());
  }

  public void doTestGetOrDefault(TestMapOrBuilder testMapOrBuilder) {
    assertEquals(11, testMapOrBuilder.getInt32ToInt32FieldOrDefault(1, -11));
    assertEquals(-11, testMapOrBuilder.getInt32ToInt32FieldOrDefault(-1, -11));

    assertEquals("11", testMapOrBuilder.getInt32ToStringFieldOrDefault(1, "-11"));
    assertNull("-11", testMapOrBuilder.getInt32ToStringFieldOrDefault(-1, null));

    assertEquals(TestUtil.toBytes("11"), testMapOrBuilder.getInt32ToBytesFieldOrDefault(1, null));
    assertNull(testMapOrBuilder.getInt32ToBytesFieldOrDefault(-1, null));

    assertEquals(TestMap.EnumValue.FOO, testMapOrBuilder.getInt32ToEnumFieldOrDefault(1, null));
    assertNull(testMapOrBuilder.getInt32ToEnumFieldOrDefault(-1, null));

    assertEquals(
        TestMap.EnumValue.BAR.getNumber(),
        testMapOrBuilder.getInt32ToEnumFieldValueOrDefault(2, -1));
    assertEquals(-1, testMapOrBuilder.getInt32ToEnumFieldValueOrDefault(-1000, -1));

    assertEquals(
        MessageValue.newBuilder().setValue(11).build(),
        testMapOrBuilder.getInt32ToMessageFieldOrDefault(1, null));
    assertNull(testMapOrBuilder.getInt32ToMessageFieldOrDefault(-1, null));

    assertEquals(11, testMapOrBuilder.getStringToInt32FieldOrDefault("1", -11));
    assertEquals(-11, testMapOrBuilder.getStringToInt32FieldOrDefault("-1", -11));

    try {
      testMapOrBuilder.getStringToInt32FieldOrDefault(null, -11);
      fail();
    } catch (NullPointerException e) {
      // expected
    }
  }

  public void testGetOrThrow() {
    TestMap.Builder builder = TestMap.newBuilder();
    assertMapCounts(0, builder);
    setMapValues(builder);
    doTestGetOrDefault(builder);
    doTestGetOrDefault(builder.build());
  }

  public void doTestGetOrThrow(TestMapOrBuilder testMapOrBuilder) {
    assertEquals(11, testMapOrBuilder.getInt32ToInt32FieldOrThrow(1));
    try {
      testMapOrBuilder.getInt32ToInt32FieldOrThrow(-1);
      fail();
    } catch (IllegalArgumentException e) {
      // expected
    }

    assertEquals("11", testMapOrBuilder.getInt32ToStringFieldOrThrow(1));

    try {
      testMapOrBuilder.getInt32ToStringFieldOrThrow(-1);
      fail();
    } catch (IllegalArgumentException e) {
      // expected
    }

    assertEquals(TestUtil.toBytes("11"), testMapOrBuilder.getInt32ToBytesFieldOrThrow(1));

    try {
      testMapOrBuilder.getInt32ToBytesFieldOrThrow(-1);
      fail();
    } catch (IllegalArgumentException e) {
      // expected
    }

    assertEquals(TestMap.EnumValue.FOO, testMapOrBuilder.getInt32ToEnumFieldOrThrow(1));
    try {
      testMapOrBuilder.getInt32ToEnumFieldOrThrow(-1);
      fail();
    } catch (IllegalArgumentException e) {
      // expected
    }

    assertEquals(
        TestMap.EnumValue.BAR.getNumber(), testMapOrBuilder.getInt32ToEnumFieldValueOrThrow(2));
    try {
      testMapOrBuilder.getInt32ToEnumFieldValueOrThrow(-1);
      fail();
    } catch (IllegalArgumentException e) {
      // expected
    }

    assertEquals(
        MessageValue.newBuilder().setValue(11).build(),
        testMapOrBuilder.getInt32ToMessageFieldOrThrow(1));
    try {
      testMapOrBuilder.getInt32ToMessageFieldOrThrow(-1);
      fail();
    } catch (IllegalArgumentException e) {
      // expected
    }

    assertEquals(11, testMapOrBuilder.getStringToInt32FieldOrThrow("1"));
    try {
      testMapOrBuilder.getStringToInt32FieldOrThrow("-1");
      fail();
    } catch (IllegalArgumentException e) {
      // expected
    }

    try {
      testMapOrBuilder.getStringToInt32FieldOrThrow(null);
      fail();
    } catch (NullPointerException e) {
      // expected
    }
  }

  public void testPut() {
    TestMap.Builder builder = TestMap.newBuilder();
    builder.putInt32ToInt32Field(1, 11);
    assertEquals(11, builder.getInt32ToInt32FieldOrThrow(1));

    builder.putInt32ToStringField(1, "a");
    assertEquals("a", builder.getInt32ToStringFieldOrThrow(1));
    try {
      builder.putInt32ToStringField(1, null);
      fail();
    } catch (NullPointerException e) {
      // expected
    }

    builder.putInt32ToBytesField(1, TestUtil.toBytes("11"));
    assertEquals(TestUtil.toBytes("11"), builder.getInt32ToBytesFieldOrThrow(1));
    try {
      builder.putInt32ToBytesField(1, null);
      fail();
    } catch (NullPointerException e) {
      // expected
    }

    builder.putInt32ToEnumField(1, TestMap.EnumValue.FOO);
    assertEquals(TestMap.EnumValue.FOO, builder.getInt32ToEnumFieldOrThrow(1));
    try {
      builder.putInt32ToEnumField(1, null);
      fail();
    } catch (NullPointerException e) {
      // expected
    }

    builder.putStringToInt32Field("a", 1);
    assertEquals(1, builder.getStringToInt32FieldOrThrow("a"));
    try {
      builder.putStringToInt32Field(null, -1);
    } catch (NullPointerException e) {
      // expected
    }
  }

  public void testRemove() {
    TestMap.Builder builder = TestMap.newBuilder();
    setMapValues(builder);
    assertEquals(11, builder.getInt32ToInt32FieldOrThrow(1));
    for (int times = 0; times < 2; times++) {
      builder.removeInt32ToInt32Field(1);
      assertEquals(-1, builder.getInt32ToInt32FieldOrDefault(1, -1));
    }

    assertEquals("11", builder.getInt32ToStringFieldOrThrow(1));
    for (int times = 0; times < 2; times++) {
      builder.removeInt32ToStringField(1);
      assertNull(builder.getInt32ToStringFieldOrDefault(1, null));
    }

    assertEquals(TestUtil.toBytes("11"), builder.getInt32ToBytesFieldOrThrow(1));
    for (int times = 0; times < 2; times++) {
      builder.removeInt32ToBytesField(1);
      assertNull(builder.getInt32ToBytesFieldOrDefault(1, null));
    }

    assertEquals(TestMap.EnumValue.FOO, builder.getInt32ToEnumFieldOrThrow(1));
    for (int times = 0; times < 2; times++) {
      builder.removeInt32ToEnumField(1);
      assertNull(builder.getInt32ToEnumFieldOrDefault(1, null));
    }

    assertEquals(11, builder.getStringToInt32FieldOrThrow("1"));
    for (int times = 0; times < 2; times++) {
      builder.removeStringToInt32Field("1");
      assertEquals(-1, builder.getStringToInt32FieldOrDefault("1", -1));
    }

    try {
      builder.removeStringToInt32Field(null);
      fail();
    } catch (NullPointerException e) {
      // expected
    }
  }

  private static <K, V> Map<K, V> newMap(K key1, V value1) {
    Map<K, V> map = new HashMap<>();
    map.put(key1, value1);
    return map;
  }

  private static <K, V> Map<K, V> newMap(K key1, V value1, K key2, V value2) {
    Map<K, V> map = new HashMap<>();
    map.put(key1, value1);
    map.put(key2, value2);
    return map;
  }

  public void testMap_withNulls() {
    TestMap.Builder builder = TestMap.newBuilder();

    try {
      builder.putStringToInt32Field(null, 3);
      fail();
    } catch (NullPointerException expected) {
    }

    try {
      builder.putAllStringToInt32Field(newMap(null, 3, "hi", 4));
      fail();
    } catch (NullPointerException expected) {
    }

    try {
      builder.putInt32ToMessageField(3, null);
      fail();
    } catch (NullPointerException expected) {
    }

    try {
      builder.putAllInt32ToMessageField(
          MapLiteTest.<Integer, MessageValue>newMap(4, null, 5, null));
      fail();
    } catch (NullPointerException expected) {
    }

    try {
      builder.putAllInt32ToMessageField(null);
      fail();
    } catch (NullPointerException expected) {
    }

    assertArrayEquals(new byte[0], builder.build().toByteArray());
  }
}
