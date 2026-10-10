// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

package com.google.protobuf;

import static com.google.common.truth.Truth.assertThat;
import static org.junit.Assert.assertThrows;

import com.google.protobuf.PackedFieldTestProto.TestAllTypes;
import com.google.protobuf.PackedFieldTestProto.TestAllTypes.NestedEnum;
import com.google.protobuf.PackedFieldTestProto.TestUnpackedTypes;
import proto2_unittest.UnittestProto.ForeignEnum;
import proto2_unittest.UnittestProto.TestPackedTypes;
import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

/** Tests primitive repeated fields in proto3 are packed in wire format. */
@RunWith(JUnit4.class)
public class PackedFieldTest {
  static final ByteString expectedPackedRawBytes =
      ByteString.copyFrom(
          new byte[] {
            (byte) 0xFA,
            0x01,
            0x01,
            0x01, // repeated int32
            (byte) 0x82,
            0x02,
            0x01,
            0x01, // repeated int64
            (byte) 0x8A,
            0x02,
            0x01,
            0x01, // repeated uint32
            (byte) 0x92,
            0x02,
            0x01,
            0x01, // repeated uint64
            (byte) 0x9A,
            0x02,
            0x01,
            0x02, // repeated sint32
            (byte) 0xA2,
            0x02,
            0x01,
            0x02, // repeated sint64
            (byte) 0xAA,
            0x02,
            0x04,
            0x01,
            0x00,
            0x00,
            0x00, // repeated fixed32
            (byte) 0xB2,
            0x02,
            0x08,
            0x01,
            0x00,
            0x00,
            0x00, // repeated fixed64
            0x00,
            0x00,
            0x00,
            0x00,
            (byte) 0xBA,
            0x02,
            0x04,
            0x01,
            0x00,
            0x00,
            0x00, // repeated sfixed32
            (byte) 0xC2,
            0x02,
            0x08,
            0x01,
            0x00,
            0x00,
            0x00, // repeated sfixed64
            0x00,
            0x00,
            0x00,
            0x00,
            (byte) 0xCA,
            0x02,
            0x04,
            0x00,
            0x00,
            (byte) 0x80,
            0x3f, // repeated float
            (byte) 0xD2,
            0x02,
            0x08,
            0x00,
            0x00,
            0x00,
            0x00, // repeated double
            0x00,
            0x00,
            (byte) 0xf0,
            0x3f,
            (byte) 0xDA,
            0x02,
            0x01,
            0x01, // repeated bool
            (byte) 0x9A,
            0x03,
            0x01,
            0x01 // repeated nested enum
          });

  static final ByteString expectedUnpackedRawBytes =
      ByteString.copyFrom(
          new byte[] {
            0x08,
            0x01, // repeated int32
            0x10,
            0x01, // repeated int64
            0x18,
            0x01, // repeated uint32
            0x20,
            0x01, // repeated uint64
            0x28,
            0x02, // repeated sint32
            0x30,
            0x02, // repeated sint64
            0x3D,
            0x01,
            0x00,
            0x00,
            0x00, // repeated fixed32
            0x41,
            0x01,
            0x00,
            0x00,
            0x00, // repeated fixed64
            0x00,
            0x00,
            0x00,
            0x00,
            0x4D,
            0x01,
            0x00,
            0x00,
            0x00, // repeated sfixed32
            0x51,
            0x01,
            0x00,
            0x00,
            0x00, // repeated sfixed64
            0x00,
            0x00,
            0x00,
            0x00,
            0x5D,
            0x00,
            0x00,
            (byte) 0x80,
            0x3f, // repeated float
            0x61,
            0x00,
            0x00,
            0x00,
            0x00, // repeated double
            0x00,
            0x00,
            (byte) 0xf0,
            0x3f,
            0x68,
            0x01, // repeated bool
            0x70,
            0x01, // repeated nested enum
          });

  @Test
  public void testPackedGeneratedMessage() throws Exception {
    TestAllTypes message =
        TestAllTypes.parseFrom(expectedPackedRawBytes, ExtensionRegistry.getEmptyRegistry());
    assertThat(message.toByteString()).isEqualTo(expectedPackedRawBytes);
  }

  @Test
  public void testPackedDynamicMessageSerialize() throws Exception {
    DynamicMessage message =
        DynamicMessage.parseFrom(
            TestAllTypes.getDescriptor(),
            expectedPackedRawBytes,
            ExtensionRegistry.getEmptyRegistry());
    assertThat(message.toByteString()).isEqualTo(expectedPackedRawBytes);
  }

  @Test
  public void testUnpackedGeneratedMessage() throws Exception {
    TestUnpackedTypes message =
        TestUnpackedTypes.parseFrom(expectedUnpackedRawBytes, ExtensionRegistry.getEmptyRegistry());
    assertThat(message.toByteString()).isEqualTo(expectedUnpackedRawBytes);
  }

  @Test
  public void testUnPackedDynamicMessageSerialize() throws Exception {
    DynamicMessage message =
        DynamicMessage.parseFrom(
            TestUnpackedTypes.getDescriptor(),
            expectedUnpackedRawBytes,
            ExtensionRegistry.getEmptyRegistry());
    assertThat(message.toByteString()).isEqualTo(expectedUnpackedRawBytes);
  }

  // Make sure we haven't screwed up the code generation for packing fields by default.
  @Test
  public void testPackedSerialization() throws Exception {
    TestAllTypes message =
        TestAllTypes.newBuilder()
            .addRepeatedInt32(1234)
            .addRepeatedNestedEnum(NestedEnum.BAR)
            .build();

    CodedInputStream in = CodedInputStream.newInstance(message.toByteArray());

    while (!in.isAtEnd()) {
      int tag = in.readTag();
      assertThat(WireFormat.getTagWireType(tag)).isEqualTo(WireFormat.WIRETYPE_LENGTH_DELIMITED);
      in.skipField(tag);
    }
  }

  // The tests below cover parsing of packed payloads. Each case runs through both the array-backed
  // and the stream-backed CodedInputStream, since they decode packed payloads differently.

  private static final int INT32 = TestAllTypes.REPEATED_INT32_FIELD_NUMBER;
  private static final int INT64 = TestAllTypes.REPEATED_INT64_FIELD_NUMBER;
  private static final int UINT32 = TestAllTypes.REPEATED_UINT32_FIELD_NUMBER;
  private static final int UINT64 = TestAllTypes.REPEATED_UINT64_FIELD_NUMBER;
  private static final int FLOAT = TestAllTypes.REPEATED_FLOAT_FIELD_NUMBER;
  private static final int DOUBLE = TestAllTypes.REPEATED_DOUBLE_FIELD_NUMBER;
  private static final int BOOL = TestAllTypes.REPEATED_BOOL_FIELD_NUMBER;
  private static final int ENUM = TestAllTypes.REPEATED_NESTED_ENUM_FIELD_NUMBER;
  private static final int[] VARINT_FIELDS = {INT32, INT64, UINT32, UINT64, BOOL, ENUM};

  @Test
  public void testPackedParse_oneByteValues() throws Exception {
    TestAllTypes expected =
        TestAllTypes.newBuilder()
            .addAllRepeatedInt32(Arrays.asList(0, 1, 127))
            .addAllRepeatedInt64(Arrays.asList(0L, 1L, 127L))
            .addAllRepeatedUint32(Arrays.asList(0, 1, 127))
            .addAllRepeatedUint64(Arrays.asList(0L, 1L, 127L))
            .addAllRepeatedBool(Arrays.asList(true, false, true))
            .addAllRepeatedNestedEnumValue(Arrays.asList(0, 1, 2, 127))
            .build();
    assertParsesAllWays(expected.toByteArray(), expected);
  }

  @Test
  public void testPackedParse_multiByteValues() throws Exception {
    TestAllTypes expected =
        TestAllTypes.newBuilder()
            .addAllRepeatedInt32(
                Arrays.asList(0, 1, 127, 128, 300, Integer.MAX_VALUE, -1, Integer.MIN_VALUE))
            .addAllRepeatedInt64(
                Arrays.asList(0L, 1L, 128L, 1L << 35, Long.MAX_VALUE, -1L, Long.MIN_VALUE))
            .addAllRepeatedUint32(Arrays.asList(0, 128, Integer.MIN_VALUE, -1))
            .addAllRepeatedUint64(Arrays.asList(0L, 128L, Long.MIN_VALUE, -1L))
            .addAllRepeatedNestedEnumValue(Arrays.asList(1, 300, -1, Integer.MAX_VALUE))
            .addAllRepeatedFloat(
                Arrays.asList(
                    0f,
                    -0f,
                    1.5f,
                    Float.NaN,
                    Float.POSITIVE_INFINITY,
                    Float.NEGATIVE_INFINITY,
                    Float.MIN_VALUE))
            .addAllRepeatedDouble(
                Arrays.asList(
                    0d,
                    -0d,
                    1.5d,
                    Double.NaN,
                    Double.POSITIVE_INFINITY,
                    Double.NEGATIVE_INFINITY,
                    Double.MIN_VALUE))
            .build();
    assertParsesAllWays(expected.toByteArray(), expected);
  }

  @Test
  public void testPackedParse_emptyPayloads() throws Exception {
    byte[] bytes =
        concat(
            packed(INT32, new byte[0]),
            packed(INT64, new byte[0]),
            packed(UINT32, new byte[0]),
            packed(UINT64, new byte[0]),
            packed(FLOAT, new byte[0]),
            packed(DOUBLE, new byte[0]),
            packed(BOOL, new byte[0]),
            packed(ENUM, new byte[0]));
    assertParsesAllWays(bytes, TestAllTypes.getDefaultInstance());
  }

  @Test
  public void testPackedParse_tenByteVarints() throws Exception {
    // Only the low bit of the tenth byte is kept.
    byte[] payload =
        concat(
            tenByteVarint(0xFF, 0x01), // -1
            tenByteVarint(0xFF, 0x00), // Long.MAX_VALUE
            tenByteVarint(0x80, 0x01), // Long.MIN_VALUE
            tenByteVarint(0x80, 0x00), // 0
            tenByteVarint(0xFF, 0x7E), // Long.MAX_VALUE
            tenByteVarint(0xFF, 0x7F)); // -1
    byte[] bytes =
        concat(
            packed(INT64, payload),
            packed(UINT64, payload),
            packed(INT32, payload),
            packed(UINT32, payload),
            packed(BOOL, payload),
            packed(ENUM, payload));
    // int32 fields keep the low 32 bits.
    List<Integer> truncated = Arrays.asList(-1, -1, 0, 0, -1, -1);
    List<Long> longs = Arrays.asList(-1L, Long.MAX_VALUE, Long.MIN_VALUE, 0L, Long.MAX_VALUE, -1L);
    TestAllTypes expected =
        TestAllTypes.newBuilder()
            .addAllRepeatedInt64(longs)
            .addAllRepeatedUint64(longs)
            .addAllRepeatedInt32(truncated)
            .addAllRepeatedUint32(truncated)
            .addAllRepeatedBool(Arrays.asList(true, true, true, false, true, true))
            .addAllRepeatedNestedEnumValue(truncated)
            .build();
    assertParsesAllWays(bytes, expected);
  }

  @Test
  public void testPackedParse_int32TruncatesWideVarints() throws Exception {
    byte[] bytes = packed(INT32, varints(0x1_0000_0001L, 0xFFFF_FFFF_0000_0002L, 5));
    assertParsesAllWays(
        bytes, TestAllTypes.newBuilder().addAllRepeatedInt32(Arrays.asList(1, 2, 5)).build());
  }

  @Test
  public void testPackedParse_multiByteBools() throws Exception {
    // Writers always emit one byte per bool, but any varint is valid.
    byte[] bytes = packed(BOOL, bytes(0x01, 0x80, 0x00, 0x00, 0x80, 0x01, 0xFF, 0xFF, 0x03, 0x01));
    assertParsesAllWays(
        bytes,
        TestAllTypes.newBuilder()
            .addAllRepeatedBool(Arrays.asList(true, false, false, true, true, true))
            .build());
  }

  @Test
  public void testPackedParse_fieldsAfterMultiByteBoolsAreParsed() throws Exception {
    // The multi-byte bool payload must not leave its limit in place for the following fields.
    byte[] bytes = concat(packed(BOOL, bytes(0x80, 0x01, 0x00)), packed(INT32, varints(7, 8)));
    assertParsesAllWays(
        bytes,
        TestAllTypes.newBuilder()
            .addAllRepeatedBool(Arrays.asList(true, false))
            .addAllRepeatedInt32(Arrays.asList(7, 8))
            .build());
  }

  @Test
  public void testPackedParse_closedEnumUnknownValuesGoToUnknownFields() throws Exception {
    byte[] bytes = packed(TestPackedTypes.PACKED_ENUM_FIELD_NUMBER, varints(4, 99, 5, 123456));
    for (CodedInputStream in : inputsFor(bytes)) {
      TestPackedTypes message = TestPackedTypes.parseFrom(in, ExtensionRegistry.getEmptyRegistry());
      assertThat(message.getPackedEnumList())
          .containsExactly(
              ForeignEnum.FOREIGN_FOO, ForeignEnum.FOREIGN_BAR, ForeignEnum.FOREIGN_LARGE)
          .inOrder();
      assertThat(
              message
                  .getUnknownFields()
                  .getField(TestPackedTypes.PACKED_ENUM_FIELD_NUMBER)
                  .getVarintList())
          .containsExactly(99L);
    }
  }

  @Test
  public void testPackedParse_multipleChunksAreConcatenated() throws Exception {
    byte[] bytes =
        concat(
            packed(INT32, varints(1, 2)),
            packed(INT64, varints(10)),
            packed(INT32, varints(300)),
            packed(INT32, new byte[0]),
            packed(INT32, varints(3)),
            packed(INT64, varints(20, 1L << 40)),
            packed(BOOL, varints(1, 0)),
            packed(BOOL, varints(1)),
            packed(FLOAT, floats(1f, 2f)),
            packed(FLOAT, floats(3f)),
            packed(DOUBLE, doubles(1d)),
            packed(DOUBLE, doubles(2d, 3d)),
            packed(ENUM, varints(1)),
            packed(ENUM, varints(99, 2)));
    TestAllTypes expected =
        TestAllTypes.newBuilder()
            .addAllRepeatedInt32(Arrays.asList(1, 2, 300, 3))
            .addAllRepeatedInt64(Arrays.asList(10L, 20L, 1L << 40))
            .addAllRepeatedBool(Arrays.asList(true, false, true))
            .addAllRepeatedFloat(Arrays.asList(1f, 2f, 3f))
            .addAllRepeatedDouble(Arrays.asList(1d, 2d, 3d))
            .addAllRepeatedNestedEnumValue(Arrays.asList(1, 99, 2))
            .build();
    assertParsesAllWays(bytes, expected);
  }

  @Test
  public void testPackedParse_manySmallChunks() throws Exception {
    ByteArrayOutputStream bytes = new ByteArrayOutputStream();
    TestAllTypes.Builder expected = TestAllTypes.newBuilder();
    for (int i = 0; i < 10000; i++) {
      bytes.write(packed(INT32, varints(i)));
      bytes.write(packed(DOUBLE, doubles(i)));
      expected.addRepeatedInt32(i).addRepeatedDouble(i);
    }
    assertParsesAllWays(bytes.toByteArray(), expected.build());
  }

  @Test
  public void testPackedParse_appendsWithoutMutatingSharedLists() throws Exception {
    TestAllTypes first = TestAllTypes.newBuilder().addRepeatedInt32(1).addRepeatedFloat(1f).build();
    // toBuilder() shares first's immutable lists with the builder.
    TestAllTypes.Builder builder =
        first.toBuilder()
            .mergeFrom(
                concat(packed(INT32, varints(2, 3)), packed(FLOAT, floats(2f))),
                ExtensionRegistry.getEmptyRegistry());
    TestAllTypes second = builder.build();
    // After build() the builder's lists are shared with second.
    builder.mergeFrom(
        new ByteArrayInputStream(packed(INT32, varints(4))), ExtensionRegistry.getEmptyRegistry());

    assertThat(first.getRepeatedInt32List()).containsExactly(1);
    assertThat(first.getRepeatedFloatList()).containsExactly(1f);
    assertThat(second.getRepeatedInt32List()).containsExactly(1, 2, 3).inOrder();
    assertThat(second.getRepeatedFloatList()).containsExactly(1f, 2f).inOrder();
    assertThat(builder.getRepeatedInt32List()).containsExactly(1, 2, 3, 4).inOrder();
  }

  @Test
  public void testPackedParse_malformedVarintPayloads() throws Exception {
    for (int field : VARINT_FIELDS) {
      // Payload ends in the middle of a varint.
      assertFailsAllWays(packed(field, bytes(0x01, 0x80)));
      // Payload contains no complete varint.
      assertFailsAllWays(packed(field, bytes(0x80, 0x80, 0x80)));
      // Eleven-byte varint.
      assertFailsAllWays(
          packed(field, bytes(0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x01)));
      // Unterminated run longer than ten bytes at the end of the payload.
      assertFailsAllWays(
          packed(
              field,
              bytes(0x01, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80)));
    }
  }

  @Test
  public void testPackedParse_fixedPayloadNotMultipleOfWidth() throws Exception {
    assertFailsAllWays(packed(FLOAT, bytes(1, 2, 3)));
    assertFailsAllWays(packed(FLOAT, bytes(1, 2, 3, 4, 5)));
    assertFailsAllWays(packed(DOUBLE, bytes(1, 2, 3, 4)));
    assertFailsAllWays(packed(DOUBLE, bytes(1, 2, 3, 4, 5, 6, 7, 8, 9)));
  }

  @Test
  public void testPackedParse_badLengths() throws Exception {
    for (int field : new int[] {INT32, INT64, UINT32, UINT64, FLOAT, DOUBLE, BOOL, ENUM}) {
      // Length runs past the end of the input.
      assertFailsAllWays(concat(lengthDelimitedTag(field), varints(8), bytes(1, 2)));
      // Negative length.
      assertFailsAllWays(concat(lengthDelimitedTag(field), bytes(0xFF, 0xFF, 0xFF, 0xFF, 0x0F)));
    }
  }

  @Test
  public void testPackedParse_lengthPastPushedLimit() throws Exception {
    byte[][] inputs = {
      packed(INT32, varints(1, 2, 300)),
      packed(INT64, varints(1, 2, 300)),
      packed(BOOL, varints(1, 0, 1)),
      packed(FLOAT, floats(1f, 2f)),
      packed(DOUBLE, doubles(1d, 2d)),
      packed(ENUM, varints(1, 2, 300)),
    };
    for (byte[] bytes : inputs) {
      for (CodedInputStream in : inputsFor(bytes)) {
        in.pushLimit(bytes.length - 1);
        assertThrows(
            InvalidProtocolBufferException.class,
            () -> TestAllTypes.newBuilder().mergeFrom(in, ExtensionRegistry.getEmptyRegistry()));
      }
    }
  }

  private static List<CodedInputStream> inputsFor(byte[] bytes) {
    return Arrays.asList(
        CodedInputStream.newInstance(bytes),
        CodedInputStream.newInstance(new ByteArrayInputStream(bytes)),
        // The minimum buffer size forces refills in the middle of packed payloads.
        CodedInputStream.newInstance(new ByteArrayInputStream(bytes), 1));
  }

  private static void assertParsesAllWays(byte[] bytes, TestAllTypes expected) throws IOException {
    List<TestAllTypes> parsed = new ArrayList<>();
    for (CodedInputStream in : inputsFor(bytes)) {
      parsed.add(TestAllTypes.parseFrom(in, ExtensionRegistry.getEmptyRegistry()));
    }
    for (TestAllTypes message : parsed) {
      assertThat(message).isEqualTo(expected);
    }
  }

  private static void assertFailsAllWays(byte[] bytes) {
    for (CodedInputStream in : inputsFor(bytes)) {
      assertThrows(
          InvalidProtocolBufferException.class,
          () -> TestAllTypes.parseFrom(in, ExtensionRegistry.getEmptyRegistry()));
    }
  }

  private static byte[] packed(int fieldNumber, byte[] payload) throws IOException {
    return concat(lengthDelimitedTag(fieldNumber), varints(payload.length), payload);
  }

  private static byte[] lengthDelimitedTag(int fieldNumber) throws IOException {
    return varints(WireFormat.makeTag(fieldNumber, WireFormat.WIRETYPE_LENGTH_DELIMITED));
  }

  private static byte[] varints(long... values) throws IOException {
    ByteArrayOutputStream bytes = new ByteArrayOutputStream();
    CodedOutputStream out = CodedOutputStream.newInstance(bytes);
    for (long value : values) {
      out.writeUInt64NoTag(value);
    }
    out.flush();
    return bytes.toByteArray();
  }

  private static byte[] floats(float... values) throws IOException {
    ByteArrayOutputStream bytes = new ByteArrayOutputStream();
    CodedOutputStream out = CodedOutputStream.newInstance(bytes);
    for (float value : values) {
      out.writeFloatNoTag(value);
    }
    out.flush();
    return bytes.toByteArray();
  }

  private static byte[] doubles(double... values) throws IOException {
    ByteArrayOutputStream bytes = new ByteArrayOutputStream();
    CodedOutputStream out = CodedOutputStream.newInstance(bytes);
    for (double value : values) {
      out.writeDoubleNoTag(value);
    }
    out.flush();
    return bytes.toByteArray();
  }

  /** Returns a ten-byte varint: nine copies of {@code fill} followed by {@code last}. */
  private static byte[] tenByteVarint(int fill, int last) {
    byte[] varint = new byte[10];
    Arrays.fill(varint, (byte) fill);
    varint[9] = (byte) last;
    return varint;
  }

  private static byte[] bytes(int... values) {
    byte[] bytes = new byte[values.length];
    for (int i = 0; i < values.length; i++) {
      bytes[i] = (byte) values[i];
    }
    return bytes;
  }

  private static byte[] concat(byte[]... parts) throws IOException {
    ByteArrayOutputStream bytes = new ByteArrayOutputStream();
    for (byte[] part : parts) {
      bytes.write(part);
    }
    return bytes.toByteArray();
  }
}
