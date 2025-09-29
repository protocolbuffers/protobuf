// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

package com.google.protobuf;

import static com.google.common.truth.Truth.assertThat;
import static com.google.common.truth.Truth.assertWithMessage;

import map_lite_test.MapTestProto.BizarroTestMap;
import map_lite_test.MapTestProto.TestMap;
import map_lite_test.MapTestProto.TestMap.MessageValue;
import map_lite_test.MapTestProto.TestMapOrBuilder;
import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.Map;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

/** Unit tests for map fields. */
@RunWith(JUnit4.class)
public final class MapLiteTest {

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

  @Test
  public void testSetMapValues() {
    TestMap.Builder testMapBuilder = TestMap.newBuilder();
    setMapValues(testMapBuilder);
    TestMap testMap = testMapBuilder.build();
    assertMapValuesSet(testMap);
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

  private void assertMapValuesSet(TestMap message) {
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

  @Test
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
  public void testSanityCopyOnWrite() throws InvalidProtocolBufferException {
    // Since builders are implemented as a thin wrapper around a message
    // instance, we attempt to verify that we can't cause the builder to modify
    // a produced message.

    TestMap.Builder builder = TestMap.newBuilder();
    TestMap message = builder.build();
    builder.putInt32ToInt32Field(1, 2);
    assertThat(message.getInt32ToInt32FieldMap()).isEmpty();
    assertThat(builder.getInt32ToInt32FieldMap()).isEqualTo(newMap(1, 2));
    message = builder.build();
    builder.putInt32ToInt32Field(2, 3);
    assertThat(message.getInt32ToInt32FieldMap()).isEqualTo(newMap(1, 2));
    assertThat(builder.getInt32ToInt32FieldMap()).isEqualTo(newMap(1, 2, 2, 3));
  }

  @Test
  public void testGetMapIsImmutable() {
    TestMap.Builder builder = TestMap.newBuilder();
    assertMapsAreImmutable(builder);
    assertMapsAreImmutable(builder.build());

    setMapValues(builder);
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
    if (!map.isEmpty()) {
      try {
        map.entrySet().remove(map.entrySet().iterator().next());
        assertWithMessage("expected exception").fail();
      } catch (UnsupportedOperationException e) {
        // expected
      }
    }
  }

  @Test
  public void testMapFieldClear() {
    TestMap.Builder builder = TestMap.newBuilder().putInt32ToInt32Field(1, 2);
    builder.clearInt32ToInt32Field();
    assertThat(builder.getInt32ToInt32FieldCount()).isEqualTo(0);
  }

  @Test
  public void testMutableMapLifecycle() {
    TestMap.Builder builder = TestMap.newBuilder().putInt32ToInt32Field(1, 2);
    assertThat(builder.build().getInt32ToInt32FieldMap()).isEqualTo(newMap(1, 2));
    assertThat(builder.getInt32ToInt32FieldMap()).isEqualTo(newMap(1, 2));
    builder.putInt32ToInt32Field(2, 3);
    assertThat(builder.getInt32ToInt32FieldMap()).isEqualTo(newMap(1, 2, 2, 3));

    builder.putInt32ToEnumField(1, TestMap.EnumValue.BAR);
    assertThat(builder.build().getInt32ToEnumFieldMap())
        .isEqualTo(newMap(1, TestMap.EnumValue.BAR));
    assertThat(builder.getInt32ToEnumFieldMap()).isEqualTo(newMap(1, TestMap.EnumValue.BAR));
    builder.putInt32ToEnumField(2, TestMap.EnumValue.FOO);
    assertThat(builder.getInt32ToEnumFieldMap())
        .isEqualTo(newMap(1, TestMap.EnumValue.BAR, 2, TestMap.EnumValue.FOO));

    builder.putInt32ToStringField(1, "1");
    assertThat(builder.build().getInt32ToStringFieldMap()).isEqualTo(newMap(1, "1"));
    assertThat(builder.getInt32ToStringFieldMap()).isEqualTo(newMap(1, "1"));
    builder.putInt32ToStringField(2, "2");
    assertThat(builder.getInt32ToStringFieldMap()).isEqualTo(newMap(1, "1", 2, "2"));

    builder.putInt32ToMessageField(1, TestMap.MessageValue.getDefaultInstance());
    assertThat(builder.build().getInt32ToMessageFieldMap())
        .isEqualTo(newMap(1, TestMap.MessageValue.getDefaultInstance()));
    assertThat(builder.getInt32ToMessageFieldMap())
        .isEqualTo(newMap(1, TestMap.MessageValue.getDefaultInstance()));
    builder.putInt32ToMessageField(2, TestMap.MessageValue.getDefaultInstance());
    assertThat(builder.getInt32ToMessageFieldMap())
        .isEqualTo(
            newMap(
                1,
                TestMap.MessageValue.getDefaultInstance(),
                2,
                TestMap.MessageValue.getDefaultInstance()));
  }

  @Test
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

  @Test
  public void testPutAll() throws Exception {
    TestMap.Builder sourceBuilder = TestMap.newBuilder();
    setMapValues(sourceBuilder);
    TestMap source = sourceBuilder.build();
    assertMapValuesSet(source);

    TestMap.Builder destination = TestMap.newBuilder();
    copyMapValues(source, destination);
    assertMapValuesSet(destination.build());
  }

  @Test
  public void testPutAllForUnknownEnumValues() throws Exception {
    TestMap.Builder sourceBuilder =
        TestMap.newBuilder()
            .putInt32ToEnumFieldValue(0, 0)
            .putInt32ToEnumFieldValue(1, 1)
            .putAllInt32ToEnumFieldValue(newMap(2, 1000)); // unknown value.
    TestMap source = sourceBuilder.build();

    TestMap.Builder destinationBuilder = TestMap.newBuilder();
    destinationBuilder.putAllInt32ToEnumFieldValue(source.getInt32ToEnumFieldValueMap());
    TestMap destination = destinationBuilder.build();

    assertThat(destination.getInt32ToEnumFieldValueMap().get(0).intValue()).isEqualTo(0);
    assertThat(destination.getInt32ToEnumFieldValueMap().get(1).intValue()).isEqualTo(1);
    assertThat(destination.getInt32ToEnumFieldValueMap().get(2).intValue()).isEqualTo(1000);
    assertThat(destination.getInt32ToEnumFieldCount()).isEqualTo(3);
  }

  @Test
  public void testPutForUnknownEnumValues() throws Exception {
    TestMap builder =
        TestMap.newBuilder()
            .putInt32ToEnumFieldValue(0, 0)
            .putInt32ToEnumFieldValue(1, 1)
            .putInt32ToEnumFieldValue(2, 1000)
            .build(); // unknown value.
    TestMap message = builder;

    assertThat(message.getInt32ToEnumFieldValueOrThrow(0)).isEqualTo(0);
    assertThat(message.getInt32ToEnumFieldValueOrThrow(1)).isEqualTo(1);
    assertThat(message.getInt32ToEnumFieldValueOrThrow(2)).isEqualTo(1000);
    assertThat(message.getInt32ToEnumFieldCount()).isEqualTo(3);
  }

  @Test
  public void testPutChecksNullKeysAndValues() throws Exception {
    TestMap.Builder builder = TestMap.newBuilder();

    try {
      builder.putInt32ToStringField(1, null);
      assertWithMessage("expected exception").fail();
    } catch (NullPointerException e) {
      // expected.
    }

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
  public void testSerializeAndParse() throws Exception {
    TestMap.Builder builder = TestMap.newBuilder();
    setMapValues(builder);
    TestMap message = builder.build();
    assertThat(message.toByteString().size()).isEqualTo(message.getSerializedSize());
    message = TestMap.parseFrom(message.toByteString());
    assertMapValuesSet(message);

    builder = message.toBuilder();
    updateMapValues(builder);
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
    setMapValues(builder);
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
    TestMap b1 =
        TestMap.newBuilder()
            .putInt32ToInt32Field(1, 2)
            .putInt32ToInt32Field(3, 4)
            .putInt32ToInt32Field(5, 6)
            .build();
    TestMap m1 = b1;

    TestMap.Builder b2 =
        TestMap.newBuilder()
            .putInt32ToInt32Field(5, 6)
            .putInt32ToInt32Field(1, 2)
            .putInt32ToInt32Field(3, 4);
    TestMap m2 = b2.build();

    assertThat(m2).isEqualTo(m1);
    assertThat(m2.hashCode()).isEqualTo(m1.hashCode());

    // Make sure we did compare map fields.
    b2.putInt32ToInt32Field(1, 0);
    m2 = b2.build();
    assertThat(m1.equals(m2)).isFalse();
    // Don't check m1.hashCode() != m2.hashCode() because it's not guaranteed
    // to be different.

    // Regression test for b/18549190: if a map is a subset of the other map,
    // equals() should return false.
    b2.removeInt32ToInt32Field(1);
    m2 = b2.build();
    assertThat(m1.equals(m2)).isFalse();
    assertThat(m2.equals(m1)).isFalse();
  }

  @Test
  @SuppressWarnings("ProtoNewBuilderMergeFrom")
  public void testUnknownEnumValues() throws Exception {
    TestMap.Builder builder =
        TestMap.newBuilder()
            .putInt32ToEnumFieldValue(0, 0)
            .putInt32ToEnumFieldValue(1, 1)
            .putInt32ToEnumFieldValue(2, 1000); // unknown value.
    TestMap message = builder.build();

    assertThat(message.getInt32ToEnumFieldMap()).containsEntry(0, TestMap.EnumValue.FOO);
    assertThat(message.getInt32ToEnumFieldMap()).containsEntry(1, TestMap.EnumValue.BAR);
    assertThat(message.getInt32ToEnumFieldMap()).containsEntry(2, TestMap.EnumValue.UNRECOGNIZED);

    builder.putAllInt32ToEnumFieldValue(newMap(2, 1000)); // unknown value.
    message = builder.build();
    assertThat(message.getInt32ToEnumFieldMap()).containsEntry(2, TestMap.EnumValue.UNRECOGNIZED);

    // Unknown enum values should be preserved after:
    //   1. Serialization and parsing.
    //   2. toBuild().
    //   3. mergeFrom().
    message = TestMap.parseFrom(message.toByteString(), ExtensionRegistryLite.getEmptyRegistry());
    assertThat(message.getInt32ToEnumFieldValueMap().get(2).intValue()).isEqualTo(1000);
    builder = message.toBuilder();
    assertThat(builder.getInt32ToEnumFieldValueMap().get(2).intValue()).isEqualTo(1000);
    builder = TestMap.newBuilder().mergeFrom(message);
    assertThat(builder.getInt32ToEnumFieldValueMap().get(2).intValue()).isEqualTo(1000);

    // hashCode()/equals() should take unknown enum values into account.
    builder.putAllInt32ToEnumFieldValue(newMap(2, 1001));
    TestMap message2 = builder.build();
    assertThat(message.hashCode()).isNotEqualTo(message2.hashCode());
    assertThat(message.equals(message2)).isFalse();
    // Unknown values will be converted to UNRECOGNIZED so the resulted enum map
    // should be the same.
    assertThat(message.getInt32ToEnumFieldMap()).isEqualTo(message2.getInt32ToEnumFieldMap());
  }

  @Test
  public void testIterationOrder() throws Exception {
    TestMap.Builder builder = TestMap.newBuilder();
    setMapValues(builder);
    TestMap message = builder.build();

    assertThat(new ArrayList<>(message.getStringToInt32FieldMap().keySet()))
        .containsExactly("1", "2", "3")
        .inOrder();
  }

  @Test
  @SuppressWarnings("SelfAssertion") // Just testing that the getters execute and are consistent.
  public void testGetMap() {
    TestMap.Builder builder = TestMap.newBuilder();
    setMapValues(builder);
    TestMap message = builder.build();
    assertThat(message.getStringToInt32FieldMap()).isEqualTo(message.getStringToInt32FieldMap());
    assertThat(message.getInt32ToBytesFieldMap()).isEqualTo(message.getInt32ToBytesFieldMap());
    assertThat(message.getInt32ToEnumFieldMap()).isEqualTo(message.getInt32ToEnumFieldMap());
    assertThat(message.getInt32ToEnumFieldValueMap())
        .isEqualTo(message.getInt32ToEnumFieldValueMap());
    assertThat(message.getInt32ToMessageFieldMap()).isEqualTo(message.getInt32ToMessageFieldMap());
  }

  @Test
  public void testContains() {
    TestMap.Builder builder = TestMap.newBuilder();
    setMapValues(builder);
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

    setMapValues(builder);
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
    setMapValues(builder);
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

    assertThat(testMapOrBuilder.getInt32ToEnumFieldValueOrDefault(2, -1))
        .isEqualTo(TestMap.EnumValue.BAR.getNumber());
    assertThat(testMapOrBuilder.getInt32ToEnumFieldValueOrDefault(-1000, -1)).isEqualTo(-1);

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
    setMapValues(builder);
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

    assertThat(testMapOrBuilder.getInt32ToEnumFieldValueOrThrow(2))
        .isEqualTo(TestMap.EnumValue.BAR.getNumber());
    try {
      testMapOrBuilder.getInt32ToEnumFieldValueOrThrow(-1);
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
    setMapValues(builder);
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

  @Test
  public void testMap_withNulls() {
    TestMap.Builder builder = TestMap.newBuilder();

    try {
      builder.putStringToInt32Field(null, 3);
      assertWithMessage("expected exception").fail();
    } catch (NullPointerException expected) {
    }

    try {
      builder.putAllStringToInt32Field(newMap(null, 3, "hi", 4));
      assertWithMessage("expected exception").fail();
    } catch (NullPointerException expected) {
    }

    try {
      builder.putInt32ToMessageField(3, null);
      assertWithMessage("expected exception").fail();
    } catch (NullPointerException expected) {
    }

    try {
      builder.putAllInt32ToMessageField(
          MapLiteTest.<Integer, MessageValue>newMap(4, null, 5, null));
      assertWithMessage("expected exception").fail();
    } catch (NullPointerException expected) {
    }

    try {
      builder.putAllInt32ToMessageField(null);
      assertWithMessage("expected exception").fail();
    } catch (NullPointerException expected) {
    }

    assertThat(builder.build().toByteArray()).isEqualTo(new byte[0]);
  }
}
