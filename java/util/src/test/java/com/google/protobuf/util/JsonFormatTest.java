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

package com.google.protobuf.util;

import com.google.protobuf.Any;
import com.google.protobuf.BoolValue;
import com.google.protobuf.ByteString;
import com.google.protobuf.BytesValue;
import com.google.protobuf.Descriptors.Descriptor;
import com.google.protobuf.Descriptors.FieldDescriptor;
import com.google.protobuf.DoubleValue;
import com.google.protobuf.FloatValue;
import com.google.protobuf.Int32Value;
import com.google.protobuf.Int64Value;
import com.google.protobuf.InvalidProtocolBufferException;
import com.google.protobuf.ListValue;
import com.google.protobuf.Message;
import com.google.protobuf.NullValue;
import com.google.protobuf.StringValue;
import com.google.protobuf.Struct;
import com.google.protobuf.UInt32Value;
import com.google.protobuf.UInt64Value;
import com.google.protobuf.Value;
import com.google.protobuf.util.JsonFormat.TypeRegistry;
import com.google.protobuf.util.proto.JsonTestProto.TestAllTypes;
import com.google.protobuf.util.proto.JsonTestProto.TestAllTypes.AliasedEnum;
import com.google.protobuf.util.proto.JsonTestProto.TestAllTypes.NestedEnum;
import com.google.protobuf.util.proto.JsonTestProto.TestAllTypes.NestedMessage;
import com.google.protobuf.util.proto.JsonTestProto.TestAny;
import com.google.protobuf.util.proto.JsonTestProto.TestCustomJsonName;
import com.google.protobuf.util.proto.JsonTestProto.TestDuration;
import com.google.protobuf.util.proto.JsonTestProto.TestFieldMask;
import com.google.protobuf.util.proto.JsonTestProto.TestMap;
import com.google.protobuf.util.proto.JsonTestProto.TestOneof;
import com.google.protobuf.util.proto.JsonTestProto.TestRecursive;
import com.google.protobuf.util.proto.JsonTestProto.TestStruct;
import com.google.protobuf.util.proto.JsonTestProto.TestTimestamp;
import com.google.protobuf.util.proto.JsonTestProto.TestWrappers;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.Reader;
import java.io.StringReader;
import java.math.BigDecimal;
import java.math.BigInteger;
import java.util.Collections;
import java.util.HashSet;
import java.util.Locale;
import java.util.Set;
import junit.framework.TestCase;

public class JsonFormatTest extends TestCase {
  public JsonFormatTest() {
    // Test that locale does not affect JsonFormat.
    Locale.setDefault(Locale.forLanguageTag("hi-IN"));
  }

  private void setAllFields(TestAllTypes.Builder builder) {
    builder.setOptionalInt32(1234);
    builder.setOptionalInt64(1234567890123456789L);
    builder.setOptionalUint32(5678);
    builder.setOptionalUint64(2345678901234567890L);
    builder.setOptionalSint32(9012);
    builder.setOptionalSint64(3456789012345678901L);
    builder.setOptionalFixed32(3456);
    builder.setOptionalFixed64(4567890123456789012L);
    builder.setOptionalSfixed32(7890);
    builder.setOptionalSfixed64(5678901234567890123L);
    builder.setOptionalFloat(1.5f);
    builder.setOptionalDouble(1.25);
    builder.setOptionalBool(true);
    builder.setOptionalString("Hello world!");
    builder.setOptionalBytes(ByteString.copyFrom(new byte[] {0, 1, 2}));
    builder.setOptionalNestedEnum(NestedEnum.BAR);
    builder.getOptionalNestedMessageBuilder().setValue(100);

    builder.addRepeatedInt32(1234);
    builder.addRepeatedInt64(1234567890123456789L);
    builder.addRepeatedUint32(5678);
    builder.addRepeatedUint64(2345678901234567890L);
    builder.addRepeatedSint32(9012);
    builder.addRepeatedSint64(3456789012345678901L);
    builder.addRepeatedFixed32(3456);
    builder.addRepeatedFixed64(4567890123456789012L);
    builder.addRepeatedSfixed32(7890);
    builder.addRepeatedSfixed64(5678901234567890123L);
    builder.addRepeatedFloat(1.5f);
    builder.addRepeatedDouble(1.25);
    builder.addRepeatedBool(true);
    builder.addRepeatedString("Hello world!");
    builder.addRepeatedBytes(ByteString.copyFrom(new byte[] {0, 1, 2}));
    builder.addRepeatedNestedEnum(NestedEnum.BAR);
    builder.addRepeatedNestedMessageBuilder().setValue(100);

    builder.addRepeatedInt32(234);
    builder.addRepeatedInt64(234567890123456789L);
    builder.addRepeatedUint32(678);
    builder.addRepeatedUint64(345678901234567890L);
    builder.addRepeatedSint32(012);
    builder.addRepeatedSint64(456789012345678901L);
    builder.addRepeatedFixed32(456);
    builder.addRepeatedFixed64(567890123456789012L);
    builder.addRepeatedSfixed32(890);
    builder.addRepeatedSfixed64(678901234567890123L);
    builder.addRepeatedFloat(11.5f);
    builder.addRepeatedDouble(11.25);
    builder.addRepeatedBool(true);
    builder.addRepeatedString("ello world!");
    builder.addRepeatedBytes(ByteString.copyFrom(new byte[] {1, 2}));
    builder.addRepeatedNestedEnum(NestedEnum.BAZ);
    builder.addRepeatedNestedMessageBuilder().setValue(200);
  }

  private void assertRoundTripEquals(Message message) throws Exception {
    assertRoundTripEquals(message, TypeRegistry.getEmptyTypeRegistry());
  }

  private void assertRoundTripEquals(Message message, TypeRegistry registry) throws Exception {
    JsonFormat.Printer printer = JsonFormat.printer().usingTypeRegistry(registry);
    JsonFormat.Parser parser = JsonFormat.parser().usingTypeRegistry(registry);
    Message.Builder builder = message.newBuilderForType();
    parser.merge(printer.print(message), builder);
    Message parsedMessage = builder.build();
    assertEquals(message.toString(), parsedMessage.toString());
  }

  private void assertRoundTripEquals(Message message, com.google.protobuf.TypeRegistry registry)
      throws Exception {
    JsonFormat.Printer printer = JsonFormat.printer().usingTypeRegistry(registry);
    JsonFormat.Parser parser = JsonFormat.parser().usingTypeRegistry(registry);
    Message.Builder builder = message.newBuilderForType();
    parser.merge(printer.print(message), builder);
    Message parsedMessage = builder.build();
    assertEquals(message.toString(), parsedMessage.toString());
  }

  private String toJsonString(Message message) throws IOException {
    return JsonFormat.printer().print(message);
  }
  private String toCompactJsonString(Message message) throws IOException {
    return JsonFormat.printer().omittingInsignificantWhitespace().print(message);
  }
  private String toSortedJsonString(Message message) throws IOException {
    return JsonFormat.printer().sortingMapKeys().print(message);
  }

  private void mergeFromJson(String json, Message.Builder builder) throws IOException {
    JsonFormat.parser().merge(json, builder);
  }

  private void mergeFromJsonIgnoringUnknownFields(String json, Message.Builder builder)
      throws IOException {
    JsonFormat.parser().ignoringUnknownFields().merge(json, builder);
  }

  public void testAllFields() throws Exception {
    TestAllTypes.Builder builder = TestAllTypes.newBuilder();
    setAllFields(builder);
    TestAllTypes message = builder.build();

    assertEquals(
        "{\n"
            + "  \"optionalInt32\": 1234,\n"
            + "  \"optionalInt64\": \"1234567890123456789\",\n"
            + "  \"optionalUint32\": 5678,\n"
            + "  \"optionalUint64\": \"2345678901234567890\",\n"
            + "  \"optionalSint32\": 9012,\n"
            + "  \"optionalSint64\": \"3456789012345678901\",\n"
            + "  \"optionalFixed32\": 3456,\n"
            + "  \"optionalFixed64\": \"4567890123456789012\",\n"
            + "  \"optionalSfixed32\": 7890,\n"
            + "  \"optionalSfixed64\": \"5678901234567890123\",\n"
            + "  \"optionalFloat\": 1.5,\n"
            + "  \"optionalDouble\": 1.25,\n"
            + "  \"optionalBool\": true,\n"
            + "  \"optionalString\": \"Hello world!\",\n"
            + "  \"optionalBytes\": \"AAEC\",\n"
            + "  \"optionalNestedMessage\": {\n"
            + "    \"value\": 100\n"
            + "  },\n"
            + "  \"optionalNestedEnum\": \"BAR\",\n"
            + "  \"repeatedInt32\": [1234, 234],\n"
            + "  \"repeatedInt64\": [\"1234567890123456789\", \"234567890123456789\"],\n"
            + "  \"repeatedUint32\": [5678, 678],\n"
            + "  \"repeatedUint64\": [\"2345678901234567890\", \"345678901234567890\"],\n"
            + "  \"repeatedSint32\": [9012, 10],\n"
            + "  \"repeatedSint64\": [\"3456789012345678901\", \"456789012345678901\"],\n"
            + "  \"repeatedFixed32\": [3456, 456],\n"
            + "  \"repeatedFixed64\": [\"4567890123456789012\", \"567890123456789012\"],\n"
            + "  \"repeatedSfixed32\": [7890, 890],\n"
            + "  \"repeatedSfixed64\": [\"5678901234567890123\", \"678901234567890123\"],\n"
            + "  \"repeatedFloat\": [1.5, 11.5],\n"
            + "  \"repeatedDouble\": [1.25, 11.25],\n"
            + "  \"repeatedBool\": [true, true],\n"
            + "  \"repeatedString\": [\"Hello world!\", \"ello world!\"],\n"
            + "  \"repeatedBytes\": [\"AAEC\", \"AQI=\"],\n"
            + "  \"repeatedNestedMessage\": [{\n"
            + "    \"value\": 100\n"
            + "  }, {\n"
            + "    \"value\": 200\n"
            + "  }],\n"
            + "  \"repeatedNestedEnum\": [\"BAR\", \"BAZ\"]\n"
            + "}",
        toJsonString(message));

    assertRoundTripEquals(message);
  }

  public void testUnknownEnumValues() throws Exception {
    TestAllTypes message =
        TestAllTypes.newBuilder()
            .setOptionalNestedEnumValue(12345)
            .addRepeatedNestedEnumValue(12345)
            .addRepeatedNestedEnumValue(0)
            .build();
    assertEquals(
        "{\n"
            + "  \"optionalNestedEnum\": 12345,\n"
            + "  \"repeatedNestedEnum\": [12345, \"FOO\"]\n"
            + "}",
        toJsonString(message));
    assertRoundTripEquals(message);

    TestMap.Builder mapBuilder = TestMap.newBuilder();
    mapBuilder.putInt32ToEnumMapValue(1, 0);
    mapBuilder.putInt32ToEnumMapValue(2, 12345);
    TestMap mapMessage = mapBuilder.build();
    assertEquals(
        "{\n"
            + "  \"int32ToEnumMap\": {\n"
            + "    \"1\": \"FOO\",\n"
            + "    \"2\": 12345\n"
            + "  }\n"
            + "}",
        toJsonString(mapMessage));
    assertRoundTripEquals(mapMessage);
  }

  public void testSpecialFloatValues() throws Exception {
    TestAllTypes message =
        TestAllTypes.newBuilder()
            .addRepeatedFloat(Float.NaN)
            .addRepeatedFloat(Float.POSITIVE_INFINITY)
            .addRepeatedFloat(Float.NEGATIVE_INFINITY)
            .addRepeatedDouble(Double.NaN)
            .addRepeatedDouble(Double.POSITIVE_INFINITY)
            .addRepeatedDouble(Double.NEGATIVE_INFINITY)
            .build();
    assertEquals(
        "{\n"
            + "  \"repeatedFloat\": [\"NaN\", \"Infinity\", \"-Infinity\"],\n"
            + "  \"repeatedDouble\": [\"NaN\", \"Infinity\", \"-Infinity\"]\n"
            + "}",
        toJsonString(message));

    assertRoundTripEquals(message);
  }

  public void testParserAcceptStringForNumericField() throws Exception {
    TestAllTypes.Builder builder = TestAllTypes.newBuilder();
    mergeFromJson(
        "{\n"
            + "  \"optionalInt32\": \"1234\",\n"
            + "  \"optionalUint32\": \"5678\",\n"
            + "  \"optionalSint32\": \"9012\",\n"
            + "  \"optionalFixed32\": \"3456\",\n"
            + "  \"optionalSfixed32\": \"7890\",\n"
            + "  \"optionalFloat\": \"1.5\",\n"
            + "  \"optionalDouble\": \"1.25\",\n"
            + "  \"optionalBool\": \"true\"\n"
            + "}",
        builder);
    TestAllTypes message = builder.build();
    assertEquals(1234, message.getOptionalInt32());
    assertEquals(5678, message.getOptionalUint32());
    assertEquals(9012, message.getOptionalSint32());
    assertEquals(3456, message.getOptionalFixed32());
    assertEquals(7890, message.getOptionalSfixed32());
    assertEquals(1.5f, message.getOptionalFloat(), 0.0f);
    assertEquals(1.25, message.getOptionalDouble(), 0.0);
    assertEquals(true, message.getOptionalBool());
  }

  public void testParserAcceptFloatingPointValueForIntegerField() throws Exception {
    // Test that numeric values like "1.000", "1e5" will also be accepted.
    TestAllTypes.Builder builder = TestAllTypes.newBuilder();
    mergeFromJson(
        "{\n"
            + "  \"repeatedInt32\": [1.000, 1e5, \"1.000\", \"1e5\"],\n"
            + "  \"repeatedUint32\": [1.000, 1e5, \"1.000\", \"1e5\"],\n"
            + "  \"repeatedInt64\": [1.000, 1e5, \"1.000\", \"1e5\"],\n"
            + "  \"repeatedUint64\": [1.000, 1e5, \"1.000\", \"1e5\"]\n"
            + "}",
        builder);
    int[] expectedValues = new int[] {1, 100000, 1, 100000};
    assertEquals(4, builder.getRepeatedInt32Count());
    assertEquals(4, builder.getRepeatedUint32Count());
    assertEquals(4, builder.getRepeatedInt64Count());
    assertEquals(4, builder.getRepeatedUint64Count());
    for (int i = 0; i < 4; ++i) {
      assertEquals(expectedValues[i], builder.getRepeatedInt32(i));
      assertEquals(expectedValues[i], builder.getRepeatedUint32(i));
      assertEquals(expectedValues[i], builder.getRepeatedInt64(i));
      assertEquals(expectedValues[i], builder.getRepeatedUint64(i));
    }

    // Non-integers will still be rejected.
    assertRejects("optionalInt32", "1.5");
    assertRejects("optionalUint32", "1.5");
    assertRejects("optionalInt64", "1.5");
    assertRejects("optionalUint64", "1.5");
  }

  private void assertRejects(String name, String value) {
    TestAllTypes.Builder builder = TestAllTypes.newBuilder();
    try {
      // Numeric form is rejected.
      mergeFromJson("{\"" + name + "\":" + value + "}", builder);
      fail("Exception is expected.");
    } catch (IOException e) {
      // Expected.
    }
    try {
      // String form is also rejected.
      mergeFromJson("{\"" + name + "\":\"" + value + "\"}", builder);
      fail("Exception is expected.");
    } catch (IOException e) {
      // Expected.
    }
  }

  private void assertAccepts(String name, String value) throws IOException {
    TestAllTypes.Builder builder = TestAllTypes.newBuilder();
    // Both numeric form and string form are accepted.
    mergeFromJson("{\"" + name + "\":" + value + "}", builder);
    builder.clear();
    mergeFromJson("{\"" + name + "\":\"" + value + "\"}", builder);
  }

  public void testParserRejectOutOfRangeNumericValues() throws Exception {
    assertAccepts("optionalInt32", String.valueOf(Integer.MAX_VALUE));
    assertAccepts("optionalInt32", String.valueOf(Integer.MIN_VALUE));
    assertRejects("optionalInt32", String.valueOf(Integer.MAX_VALUE + 1L));
    assertRejects("optionalInt32", String.valueOf(Integer.MIN_VALUE - 1L));

    assertAccepts("optionalUint32", String.valueOf(Integer.MAX_VALUE + 1L));
    assertRejects("optionalUint32", "123456789012345");
    assertRejects("optionalUint32", "-1");

    BigInteger one = new BigInteger("1");
    BigInteger maxLong = new BigInteger(String.valueOf(Long.MAX_VALUE));
    BigInteger minLong = new BigInteger(String.valueOf(Long.MIN_VALUE));
    assertAccepts("optionalInt64", maxLong.toString());
    assertAccepts("optionalInt64", minLong.toString());
    assertRejects("optionalInt64", maxLong.add(one).toString());
    assertRejects("optionalInt64", minLong.subtract(one).toString());

    assertAccepts("optionalUint64", maxLong.add(one).toString());
    assertRejects("optionalUint64", "1234567890123456789012345");
    assertRejects("optionalUint64", "-1");

    assertAccepts("optionalBool", "true");
    assertRejects("optionalBool", "1");
    assertRejects("optionalBool", "0");

    assertAccepts("optionalFloat", String.valueOf(Float.MAX_VALUE));
    assertAccepts("optionalFloat", String.valueOf(-Float.MAX_VALUE));
    assertRejects("optionalFloat", String.valueOf(Double.MAX_VALUE));
    assertRejects("optionalFloat", String.valueOf(-Double.MAX_VALUE));

    BigDecimal moreThanOne = new BigDecimal("1.000001");
    BigDecimal maxDouble = new BigDecimal(Double.MAX_VALUE);
    BigDecimal minDouble = new BigDecimal(-Double.MAX_VALUE);
    assertAccepts("optionalDouble", maxDouble.toString());
    assertAccepts("optionalDouble", minDouble.toString());
    assertRejects("optionalDouble", maxDouble.multiply(moreThanOne).toString());
    assertRejects("optionalDouble", minDouble.multiply(moreThanOne).toString());
  }

  public void testParserAcceptNull() throws Exception {
    TestAllTypes.Builder builder = TestAllTypes.newBuilder();
    mergeFromJson(
        "{\n"
            + "  \"optionalInt32\": null,\n"
            + "  \"optionalInt64\": null,\n"
            + "  \"optionalUint32\": null,\n"
            + "  \"optionalUint64\": null,\n"
            + "  \"optionalSint32\": null,\n"
            + "  \"optionalSint64\": null,\n"
            + "  \"optionalFixed32\": null,\n"
            + "  \"optionalFixed64\": null,\n"
            + "  \"optionalSfixed32\": null,\n"
            + "  \"optionalSfixed64\": null,\n"
            + "  \"optionalFloat\": null,\n"
            + "  \"optionalDouble\": null,\n"
            + "  \"optionalBool\": null,\n"
            + "  \"optionalString\": null,\n"
            + "  \"optionalBytes\": null,\n"
            + "  \"optionalNestedMessage\": null,\n"
            + "  \"optionalNestedEnum\": null,\n"
            + "  \"repeatedInt32\": null,\n"
            + "  \"repeatedInt64\": null,\n"
            + "  \"repeatedUint32\": null,\n"
            + "  \"repeatedUint64\": null,\n"
            + "  \"repeatedSint32\": null,\n"
            + "  \"repeatedSint64\": null,\n"
            + "  \"repeatedFixed32\": null,\n"
            + "  \"repeatedFixed64\": null,\n"
            + "  \"repeatedSfixed32\": null,\n"
            + "  \"repeatedSfixed64\": null,\n"
            + "  \"repeatedFloat\": null,\n"
            + "  \"repeatedDouble\": null,\n"
            + "  \"repeatedBool\": null,\n"
            + "  \"repeatedString\": null,\n"
            + "  \"repeatedBytes\": null,\n"
            + "  \"repeatedNestedMessage\": null,\n"
            + "  \"repeatedNestedEnum\": null\n"
            + "}",
        builder);
    TestAllTypes message = builder.build();
    assertEquals(TestAllTypes.getDefaultInstance(), message);

    // Repeated field elements cannot be null.
    try {
      builder = TestAllTypes.newBuilder();
      mergeFromJson("{\n" + "  \"repeatedInt32\": [null, null],\n" + "}", builder);
      fail();
    } catch (InvalidProtocolBufferException e) {
      // Exception expected.
    }

    try {
      builder = TestAllTypes.newBuilder();
      mergeFromJson("{\n" + "  \"repeatedNestedMessage\": [null, null],\n" + "}", builder);
      fail();
    } catch (InvalidProtocolBufferException e) {
      // Exception expected.
    }
  }

  public void testNullInOneof() throws Exception {
    TestOneof.Builder builder = TestOneof.newBuilder();
    mergeFromJson("{\n" + "  \"oneofNullValue\": null \n" + "}", builder);
    TestOneof message = builder.build();
    assertEquals(TestOneof.OneofFieldCase.ONEOF_NULL_VALUE, message.getOneofFieldCase());
    assertEquals(NullValue.NULL_VALUE, message.getOneofNullValue());
  }

  public void testNullFirstInDuplicateOneof() throws Exception {
    TestOneof.Builder builder = TestOneof.newBuilder();
    mergeFromJson("{\"oneofNestedMessage\": null, \"oneofInt32\": 1}", builder);
    TestOneof message = builder.build();
    assertEquals(1, message.getOneofInt32());
  }

  public void testNullLastInDuplicateOneof() throws Exception {
    TestOneof.Builder builder = TestOneof.newBuilder();
    mergeFromJson("{\"oneofInt32\": 1, \"oneofNestedMessage\": null}", builder);
    TestOneof message = builder.build();
    assertEquals(1, message.getOneofInt32());
  }

  public void testParserRejectDuplicatedFields() throws Exception {
    // TODO(xiaofeng): The parser we are currently using (GSON) will accept and keep the last
    // one if multiple entries have the same name. This is not the desired behavior but it can
    // only be fixed by using our own parser. Here we only test the cases where the names are
    // different but still referring to the same field.

    // Duplicated optional fields.
    try {
      TestAllTypes.Builder builder = TestAllTypes.newBuilder();
      mergeFromJson(
          "{\n"
              + "  \"optionalNestedMessage\": {},\n"
              + "  \"optional_nested_message\": {}\n"
              + "}",
          builder);
      fail();
    } catch (InvalidProtocolBufferException e) {
      // Exception expected.
    }

    // Duplicated repeated fields.
    try {
      TestAllTypes.Builder builder = TestAllTypes.newBuilder();
      mergeFromJson(
          "{\n"
              + "  \"repeatedInt32\": [1, 2],\n"
              + "  \"repeated_int32\": [5, 6]\n"
              + "}",
          builder);
      fail();
    } catch (InvalidProtocolBufferException e) {
      // Exception expected.
    }

    // Duplicated oneof fields, same name.
    try {
      TestOneof.Builder builder = TestOneof.newBuilder();
      mergeFromJson("{\n" + "  \"oneofInt32\": 1,\n" + "  \"oneof_int32\": 2\n" + "}", builder);
      fail();
    } catch (InvalidProtocolBufferException e) {
      // Exception expected.
    }

    // Duplicated oneof fields, different name.
    try {
      TestOneof.Builder builder = TestOneof.newBuilder();
      mergeFromJson(
          "{\n" + "  \"oneofInt32\": 1,\n" + "  \"oneofNullValue\": null\n" + "}", builder);
      fail();
    } catch (InvalidProtocolBufferException e) {
      // Exception expected.
    }
  }

  public void testMapFields() throws Exception {
    TestMap.Builder builder = TestMap.newBuilder();
    builder.putInt32ToInt32Map(1, 10);
    builder.putInt64ToInt32Map(1234567890123456789L, 10);
    builder.putUint32ToInt32Map(2, 20);
    builder.putUint64ToInt32Map(2234567890123456789L, 20);
    builder.putSint32ToInt32Map(3, 30);
    builder.putSint64ToInt32Map(3234567890123456789L, 30);
    builder.putFixed32ToInt32Map(4, 40);
    builder.putFixed64ToInt32Map(4234567890123456789L, 40);
    builder.putSfixed32ToInt32Map(5, 50);
    builder.putSfixed64ToInt32Map(5234567890123456789L, 50);
    builder.putBoolToInt32Map(false, 6);
    builder.putStringToInt32Map("Hello", 10);

    builder.putInt32ToInt64Map(1, 1234567890123456789L);
    builder.putInt32ToUint32Map(2, 20);
    builder.putInt32ToUint64Map(2, 2234567890123456789L);
    builder.putInt32ToSint32Map(3, 30);
    builder.putInt32ToSint64Map(3, 3234567890123456789L);
    builder.putInt32ToFixed32Map(4, 40);
    builder.putInt32ToFixed64Map(4, 4234567890123456789L);
    builder.putInt32ToSfixed32Map(5, 50);
    builder.putInt32ToSfixed64Map(5, 5234567890123456789L);
    builder.putInt32ToFloatMap(6, 1.5f);
    builder.putInt32ToDoubleMap(6, 1.25);
    builder.putInt32ToBoolMap(7, false);
    builder.putInt32ToStringMap(7, "World");
    builder.putInt32ToBytesMap(8, ByteString.copyFrom(new byte[] {1, 2, 3}));
    builder.putInt32ToMessageMap(8, NestedMessage.newBuilder().setValue(1234).build());
    builder.putInt32ToEnumMap(9, NestedEnum.BAR);
    TestMap message = builder.build();

    assertEquals(
        "{\n"
            + "  \"int32ToInt32Map\": {\n"
            + "    \"1\": 10\n"
            + "  },\n"
            + "  \"int64ToInt32Map\": {\n"
            + "    \"1234567890123456789\": 10\n"
            + "  },\n"
            + "  \"uint32ToInt32Map\": {\n"
            + "    \"2\": 20\n"
            + "  },\n"
            + "  \"uint64ToInt32Map\": {\n"
            + "    \"2234567890123456789\": 20\n"
            + "  },\n"
            + "  \"sint32ToInt32Map\": {\n"
            + "    \"3\": 30\n"
            + "  },\n"
            + "  \"sint64ToInt32Map\": {\n"
            + "    \"3234567890123456789\": 30\n"
            + "  },\n"
            + "  \"fixed32ToInt32Map\": {\n"
            + "    \"4\": 40\n"
            + "  },\n"
            + "  \"fixed64ToInt32Map\": {\n"
            + "    \"4234567890123456789\": 40\n"
            + "  },\n"
            + "  \"sfixed32ToInt32Map\": {\n"
            + "    \"5\": 50\n"
            + "  },\n"
            + "  \"sfixed64ToInt32Map\": {\n"
            + "    \"5234567890123456789\": 50\n"
            + "  },\n"
            + "  \"boolToInt32Map\": {\n"
            + "    \"false\": 6\n"
            + "  },\n"
            + "  \"stringToInt32Map\": {\n"
            + "    \"Hello\": 10\n"
            + "  },\n"
            + "  \"int32ToInt64Map\": {\n"
            + "    \"1\": \"1234567890123456789\"\n"
            + "  },\n"
            + "  \"int32ToUint32Map\": {\n"
            + "    \"2\": 20\n"
            + "  },\n"
            + "  \"int32ToUint64Map\": {\n"
            + "    \"2\": \"2234567890123456789\"\n"
            + "  },\n"
            + "  \"int32ToSint32Map\": {\n"
            + "    \"3\": 30\n"
            + "  },\n"
            + "  \"int32ToSint64Map\": {\n"
            + "    \"3\": \"3234567890123456789\"\n"
            + "  },\n"
            + "  \"int32ToFixed32Map\": {\n"
            + "    \"4\": 40\n"
            + "  },\n"
            + "  \"int32ToFixed64Map\": {\n"
            + "    \"4\": \"4234567890123456789\"\n"
            + "  },\n"
            + "  \"int32ToSfixed32Map\": {\n"
            + "    \"5\": 50\n"
            + "  },\n"
            + "  \"int32ToSfixed64Map\": {\n"
            + "    \"5\": \"5234567890123456789\"\n"
            + "  },\n"
            + "  \"int32ToFloatMap\": {\n"
            + "    \"6\": 1.5\n"
            + "  },\n"
            + "  \"int32ToDoubleMap\": {\n"
            + "    \"6\": 1.25\n"
            + "  },\n"
            + "  \"int32ToBoolMap\": {\n"
            + "    \"7\": false\n"
            + "  },\n"
            + "  \"int32ToStringMap\": {\n"
            + "    \"7\": \"World\"\n"
            + "  },\n"
            + "  \"int32ToBytesMap\": {\n"
            + "    \"8\": \"AQID\"\n"
            + "  },\n"
            + "  \"int32ToMessageMap\": {\n"
            + "    \"8\": {\n"
            + "      \"value\": 1234\n"
            + "    }\n"
            + "  },\n"
            + "  \"int32ToEnumMap\": {\n"
            + "    \"9\": \"BAR\"\n"
            + "  }\n"
            + "}",
        toJsonString(message));
    assertRoundTripEquals(message);

    // Test multiple entries.
    builder = TestMap.newBuilder();
    builder.putInt32ToInt32Map(1, 2);
    builder.putInt32ToInt32Map(3, 4);
    message = builder.build();

    assertEquals(
        "{\n" + "  \"int32ToInt32Map\": {\n" + "    \"1\": 2,\n" + "    \"3\": 4\n" + "  }\n" + "}",
        toJsonString(message));
    assertRoundTripEquals(message);
  }

  public void testMapNullValueIsRejected() throws Exception {
    try {
      TestMap.Builder builder = TestMap.newBuilder();
      mergeFromJson(
          "{\n"
              + "  \"int32ToInt32Map\": {null: 1},\n"
              + "  \"int32ToMessageMap\": {null: 2}\n"
              + "}",
          builder);
      fail();
    } catch (InvalidProtocolBufferException e) {
      // Exception expected.
    }

    try {
      TestMap.Builder builder = TestMap.newBuilder();
      mergeFromJson(
          "{\n"
              + "  \"int32ToInt32Map\": {\"1\": null},\n"
              + "  \"int32ToMessageMap\": {\"2\": null}\n"
              + "}",
          builder);
      fail();

    } catch (InvalidProtocolBufferException e) {
      // Exception expected.
    }
  }

  public void testMapEnumNullValueIsIgnored() throws Exception {
    TestMap.Builder builder = TestMap.newBuilder();
    mergeFromJsonIgnoringUnknownFields(
        "{\n" + "  \"int32ToEnumMap\": {\"1\": null}\n" + "}", builder);
    TestMap map = builder.build();
    assertEquals(0, map.getInt32ToEnumMapMap().size());
  }

  public void testParserAcceptNonQuotedObjectKey() throws Exception {
    TestMap.Builder builder = TestMap.newBuilder();
    mergeFromJson(
        "{\n" + "  int32ToInt32Map: {1: 2},\n" + "  stringToInt32Map: {hello: 3}\n" + "}", builder);
    TestMap message = builder.build();
    assertEquals(2, message.getInt32ToInt32Map().get(1).intValue());
    assertEquals(3, message.getStringToInt32Map().get("hello").intValue());
  }

  public void testWrappers() throws Exception {
    TestWrappers.Builder builder = TestWrappers.newBuilder();
    builder.getBoolValueBuilder().setValue(false);
    builder.getInt32ValueBuilder().setValue(0);
    builder.getInt64ValueBuilder().setValue(0);
    builder.getUint32ValueBuilder().setValue(0);
    builder.getUint64ValueBuilder().setValue(0);
    builder.getFloatValueBuilder().setValue(0.0f);
    builder.getDoubleValueBuilder().setValue(0.0);
    builder.getStringValueBuilder().setValue("");
    builder.getBytesValueBuilder().setValue(ByteString.EMPTY);
    TestWrappers message = builder.build();

    assertEquals(
        "{\n"
            + "  \"int32Value\": 0,\n"
            + "  \"uint32Value\": 0,\n"
            + "  \"int64Value\": \"0\",\n"
            + "  \"uint64Value\": \"0\",\n"
            + "  \"floatValue\": 0.0,\n"
            + "  \"doubleValue\": 0.0,\n"
            + "  \"boolValue\": false,\n"
            + "  \"stringValue\": \"\",\n"
            + "  \"bytesValue\": \"\"\n"
            + "}",
        toJsonString(message));
    assertRoundTripEquals(message);

    builder = TestWrappers.newBuilder();
    builder.getBoolValueBuilder().setValue(true);
    builder.getInt32ValueBuilder().setValue(1);
    builder.getInt64ValueBuilder().setValue(2);
    builder.getUint32ValueBuilder().setValue(3);
    builder.getUint64ValueBuilder().setValue(4);
    builder.getFloatValueBuilder().setValue(5.0f);
    builder.getDoubleValueBuilder().setValue(6.0);
    builder.getStringValueBuilder().setValue("7");
    builder.getBytesValueBuilder().setValue(ByteString.copyFrom(new byte[] {8}));
    message = builder.build();

    assertEquals(
        "{\n"
            + "  \"int32Value\": 1,\n"
            + "  \"uint32Value\": 3,\n"
            + "  \"int64Value\": \"2\",\n"
            + "  \"uint64Value\": \"4\",\n"
            + "  \"floatValue\": 5.0,\n"
            + "  \"doubleValue\": 6.0,\n"
            + "  \"boolValue\": true,\n"
            + "  \"stringValue\": \"7\",\n"
            + "  \"bytesValue\": \"CA==\"\n"
            + "}",
        toJsonString(message));
    assertRoundTripEquals(message);
  }

  public void testTimestamp() throws Exception {
    TestTimestamp message =
        TestTimestamp.newBuilder()
            .setTimestampValue(Timestamps.parse("1970-01-01T00:00:00Z"))
            .build();

    assertEquals(
        "{\n" + "  \"timestampValue\": \"1970-01-01T00:00:00Z\"\n" + "}", toJsonString(message));
    assertRoundTripEquals(message);
  }

  public void testDuration() throws Exception {
    TestDuration message =
        TestDuration.newBuilder().setDurationValue(Durations.parse("12345s")).build();

    assertEquals("{\n" + "  \"durationValue\": \"12345s\"\n" + "}", toJsonString(message));
    assertRoundTripEquals(message);
  }

  public void testFieldMask() throws Exception {
    TestFieldMask message =
        TestFieldMask.newBuilder()
            .setFieldMaskValue(FieldMaskUtil.fromString("foo.bar,baz,foo_bar.baz"))
            .build();

    assertEquals(
        "{\n" + "  \"fieldMaskValue\": \"foo.bar,baz,fooBar.baz\"\n" + "}", toJsonString(message));
    assertRoundTripEquals(message);
  }

  public void testStruct() throws Exception {
    // Build a struct with all possible values.
    TestStruct.Builder builder = TestStruct.newBuilder();
    Struct.Builder structBuilder = builder.getStructValueBuilder();
    structBuilder.putFields("null_value", Value.newBuilder().setNullValueValue(0).build());
    structBuilder.putFields("number_value", Value.newBuilder().setNumberValue(1.25).build());
    structBuilder.putFields("string_value", Value.newBuilder().setStringValue("hello").build());
    Struct.Builder subStructBuilder = Struct.newBuilder();
    subStructBuilder.putFields("number_value", Value.newBuilder().setNumberValue(1234).build());
    structBuilder.putFields(
        "struct_value", Value.newBuilder().setStructValue(subStructBuilder.build()).build());
    ListValue.Builder listBuilder = ListValue.newBuilder();
    listBuilder.addValues(Value.newBuilder().setNumberValue(1.125).build());
    listBuilder.addValues(Value.newBuilder().setNullValueValue(0).build());
    structBuilder.putFields(
        "list_value", Value.newBuilder().setListValue(listBuilder.build()).build());
    TestStruct message = builder.build();

    assertEquals(
        "{\n"
            + "  \"structValue\": {\n"
            + "    \"null_value\": null,\n"
            + "    \"number_value\": 1.25,\n"
            + "    \"string_value\": \"hello\",\n"
            + "    \"struct_value\": {\n"
            + "      \"number_value\": 1234.0\n"
            + "    },\n"
            + "    \"list_value\": [1.125, null]\n"
            + "  }\n"
            + "}",
        toJsonString(message));
    assertRoundTripEquals(message);

    builder = TestStruct.newBuilder();
    builder.setValue(Value.newBuilder().setNullValueValue(0).build());
    message = builder.build();
    assertEquals("{\n" + "  \"value\": null\n" + "}", toJsonString(message));
    assertRoundTripEquals(message);

    builder = TestStruct.newBuilder();
    listBuilder = builder.getListValueBuilder();
    listBuilder.addValues(Value.newBuilder().setNumberValue(31831.125).build());
    listBuilder.addValues(Value.newBuilder().setNullValueValue(0).build());
    message = builder.build();
    assertEquals("{\n" + "  \"listValue\": [31831.125, null]\n" + "}", toJsonString(message));
    assertRoundTripEquals(message);
  }


  public void testAnyFieldsWithCustomAddedTypeRegistry() throws Exception {
    TestAllTypes content = TestAllTypes.newBuilder().setOptionalInt32(1234).build();
    TestAny message = TestAny.newBuilder().setAnyValue(Any.pack(content)).build();

    com.google.protobuf.TypeRegistry registry =
        com.google.protobuf.TypeRegistry.newBuilder().add(content.getDescriptorForType()).build();
    JsonFormat.Printer printer = JsonFormat.printer().usingTypeRegistry(registry);

    assertEquals(
        "{\n"
            + "  \"anyValue\": {\n"
            + "    \"@type\": \"type.googleapis.com/json_test.TestAllTypes\",\n"
            + "    \"optionalInt32\": 1234\n"
            + "  }\n"
            + "}",
        printer.print(message));
    assertRoundTripEquals(message, registry);

    TestAny messageWithDefaultAnyValue =
        TestAny.newBuilder().setAnyValue(Any.getDefaultInstance()).build();
    assertEquals("{\n" + "  \"anyValue\": {}\n" + "}", printer.print(messageWithDefaultAnyValue));
    assertRoundTripEquals(messageWithDefaultAnyValue, registry);

    // Well-known types have a special formatting when embedded in Any.
    //
    // 1. Any in Any.
    Any anyMessage = Any.pack(Any.pack(content));
    assertEquals(
        "{\n"
            + "  \"@type\": \"type.googleapis.com/google.protobuf.Any\",\n"
            + "  \"value\": {\n"
            + "    \"@type\": \"type.googleapis.com/json_test.TestAllTypes\",\n"
            + "    \"optionalInt32\": 1234\n"
            + "  }\n"
            + "}",
        printer.print(anyMessage));
    assertRoundTripEquals(anyMessage, registry);
  }

  public void testAnyFields() throws Exception {
    TestAllTypes content = TestAllTypes.newBuilder().setOptionalInt32(1234).build();
    TestAny message = TestAny.newBuilder().setAnyValue(Any.pack(content)).build();

    // A TypeRegistry must be provided in order to convert Any types.
    try {
      toJsonString(message);
      fail("Exception is expected.");
    } catch (IOException e) {
      // Expected.
    }

    JsonFormat.TypeRegistry registry =
        JsonFormat.TypeRegistry.newBuilder().add(TestAllTypes.getDescriptor()).build();
    JsonFormat.Printer printer = JsonFormat.printer().usingTypeRegistry(registry);

    assertEquals(
        "{\n"
            + "  \"anyValue\": {\n"
            + "    \"@type\": \"type.googleapis.com/json_test.TestAllTypes\",\n"
            + "    \"optionalInt32\": 1234\n"
            + "  }\n"
            + "}",
        printer.print(message));
    assertRoundTripEquals(message, registry);

    TestAny messageWithDefaultAnyValue =
        TestAny.newBuilder().setAnyValue(Any.getDefaultInstance()).build();
    assertEquals(
        "{\n"
            + "  \"anyValue\": {}\n"
            + "}",
        printer.print(messageWithDefaultAnyValue));
    assertRoundTripEquals(messageWithDefaultAnyValue, registry);

    // Well-known types have a special formatting when embedded in Any.
    //
    // 1. Any in Any.
    Any anyMessage = Any.pack(Any.pack(content));
    assertEquals(
        "{\n"
            + "  \"@type\": \"type.googleapis.com/google.protobuf.Any\",\n"
            + "  \"value\": {\n"
            + "    \"@type\": \"type.googleapis.com/json_test.TestAllTypes\",\n"
            + "    \"optionalInt32\": 1234\n"
            + "  }\n"
            + "}",
        printer.print(anyMessage));
    assertRoundTripEquals(anyMessage, registry);

    // 2. Wrappers in Any.
    anyMessage = Any.pack(Int32Value.newBuilder().setValue(12345).build());
    assertEquals(
        "{\n"
            + "  \"@type\": \"type.googleapis.com/google.protobuf.Int32Value\",\n"
            + "  \"value\": 12345\n"
            + "}",
        printer.print(anyMessage));
    assertRoundTripEquals(anyMessage, registry);
    anyMessage = Any.pack(UInt32Value.newBuilder().setValue(12345).build());
    assertEquals(
        "{\n"
            + "  \"@type\": \"type.googleapis.com/google.protobuf.UInt32Value\",\n"
            + "  \"value\": 12345\n"
            + "}",
        printer.print(anyMessage));
    assertRoundTripEquals(anyMessage, registry);
    anyMessage = Any.pack(Int64Value.newBuilder().setValue(12345).build());
    assertEquals(
        "{\n"
            + "  \"@type\": \"type.googleapis.com/google.protobuf.Int64Value\",\n"
            + "  \"value\": \"12345\"\n"
            + "}",
        printer.print(anyMessage));
    assertRoundTripEquals(anyMessage, registry);
    anyMessage = Any.pack(UInt64Value.newBuilder().setValue(12345).build());
    assertEquals(
        "{\n"
            + "  \"@type\": \"type.googleapis.com/google.protobuf.UInt64Value\",\n"
            + "  \"value\": \"12345\"\n"
            + "}",
        printer.print(anyMessage));
    assertRoundTripEquals(anyMessage, registry);
    anyMessage = Any.pack(FloatValue.newBuilder().setValue(12345).build());
    assertEquals(
        "{\n"
            + "  \"@type\": \"type.googleapis.com/google.protobuf.FloatValue\",\n"
            + "  \"value\": 12345.0\n"
            + "}",
        printer.print(anyMessage));
    assertRoundTripEquals(anyMessage, registry);
    anyMessage = Any.pack(DoubleValue.newBuilder().setValue(12345).build());
    assertEquals(
        "{\n"
            + "  \"@type\": \"type.googleapis.com/google.protobuf.DoubleValue\",\n"
            + "  \"value\": 12345.0\n"
            + "}",
        printer.print(anyMessage));
    assertRoundTripEquals(anyMessage, registry);
    anyMessage = Any.pack(BoolValue.newBuilder().setValue(true).build());
    assertEquals(
        "{\n"
            + "  \"@type\": \"type.googleapis.com/google.protobuf.BoolValue\",\n"
            + "  \"value\": true\n"
            + "}",
        printer.print(anyMessage));
    assertRoundTripEquals(anyMessage, registry);
    anyMessage = Any.pack(StringValue.newBuilder().setValue("Hello").build());
    assertEquals(
        "{\n"
            + "  \"@type\": \"type.googleapis.com/google.protobuf.StringValue\",\n"
            + "  \"value\": \"Hello\"\n"
            + "}",
        printer.print(anyMessage));
    assertRoundTripEquals(anyMessage, registry);
    anyMessage =
        Any.pack(BytesValue.newBuilder().setValue(ByteString.copyFrom(new byte[] {1, 2})).build());
    assertEquals(
        "{\n"
            + "  \"@type\": \"type.googleapis.com/google.protobuf.BytesValue\",\n"
            + "  \"value\": \"AQI=\"\n"
            + "}",
        printer.print(anyMessage));
    assertRoundTripEquals(anyMessage, registry);

    // 3. Timestamp in Any.
    anyMessage = Any.pack(Timestamps.parse("1969-12-31T23:59:59Z"));
    assertEquals(
        "{\n"
            + "  \"@type\": \"type.googleapis.com/google.protobuf.Timestamp\",\n"
            + "  \"value\": \"1969-12-31T23:59:59Z\"\n"
            + "}",
        printer.print(anyMessage));
    assertRoundTripEquals(anyMessage, registry);

    // 4. Duration in Any
    anyMessage = Any.pack(Durations.parse("12345.10s"));
    assertEquals(
        "{\n"
            + "  \"@type\": \"type.googleapis.com/google.protobuf.Duration\",\n"
            + "  \"value\": \"12345.100s\"\n"
            + "}",
        printer.print(anyMessage));
    assertRoundTripEquals(anyMessage, registry);

    // 5. FieldMask in Any
    anyMessage = Any.pack(FieldMaskUtil.fromString("foo.bar,baz"));
    assertEquals(
        "{\n"
            + "  \"@type\": \"type.googleapis.com/google.protobuf.FieldMask\",\n"
            + "  \"value\": \"foo.bar,baz\"\n"
            + "}",
        printer.print(anyMessage));
    assertRoundTripEquals(anyMessage, registry);

    // 6. Struct in Any
    Struct.Builder structBuilder = Struct.newBuilder();
    structBuilder.putFields("number", Value.newBuilder().setNumberValue(1.125).build());
    anyMessage = Any.pack(structBuilder.build());
    assertEquals(
        "{\n"
            + "  \"@type\": \"type.googleapis.com/google.protobuf.Struct\",\n"
            + "  \"value\": {\n"
            + "    \"number\": 1.125\n"
            + "  }\n"
            + "}",
        printer.print(anyMessage));
    assertRoundTripEquals(anyMessage, registry);

    // 7. Value (number type) in Any
    Value.Builder valueBuilder = Value.newBuilder();
    valueBuilder.setNumberValue(1);
    anyMessage = Any.pack(valueBuilder.build());
    assertEquals(
        "{\n"
            + "  \"@type\": \"type.googleapis.com/google.protobuf.Value\",\n"
            + "  \"value\": 1.0\n"
            + "}",
        printer.print(anyMessage));
    assertRoundTripEquals(anyMessage, registry);

    // 8. Value (null type) in Any
    anyMessage = Any.pack(Value.newBuilder().setNullValue(NullValue.NULL_VALUE).build());
    assertEquals(
        "{\n"
            + "  \"@type\": \"type.googleapis.com/google.protobuf.Value\",\n"
            + "  \"value\": null\n"
            + "}",
        printer.print(anyMessage));
    assertRoundTripEquals(anyMessage, registry);
  }

  public void testAnyInMaps() throws Exception {
    JsonFormat.TypeRegistry registry =
        JsonFormat.TypeRegistry.newBuilder().add(TestAllTypes.getDescriptor()).build();
    JsonFormat.Printer printer = JsonFormat.printer().usingTypeRegistry(registry);

    TestAny.Builder testAny = TestAny.newBuilder();
    testAny.putAnyMap("int32_wrapper", Any.pack(Int32Value.newBuilder().setValue(123).build()));
    testAny.putAnyMap("int64_wrapper", Any.pack(Int64Value.newBuilder().setValue(456).build()));
    testAny.putAnyMap("timestamp", Any.pack(Timestamps.parse("1969-12-31T23:59:59Z")));
    testAny.putAnyMap("duration", Any.pack(Durations.parse("12345.1s")));
    testAny.putAnyMap("field_mask", Any.pack(FieldMaskUtil.fromString("foo.bar,baz")));
    Value numberValue = Value.newBuilder().setNumberValue(1.125).build();
    Struct.Builder struct = Struct.newBuilder();
    struct.putFields("number", numberValue);
    testAny.putAnyMap("struct", Any.pack(struct.build()));
    Value nullValue = Value.newBuilder().setNullValue(NullValue.NULL_VALUE).build();
    testAny.putAnyMap(
        "list_value",
        Any.pack(ListValue.newBuilder().addValues(numberValue).addValues(nullValue).build()));
    testAny.putAnyMap("number_value", Any.pack(numberValue));
    testAny.putAnyMap("any_value_number", Any.pack(Any.pack(numberValue)));
    testAny.putAnyMap("any_value_default", Any.pack(Any.getDefaultInstance()));
    testAny.putAnyMap("default", Any.getDefaultInstance());

    assertEquals(
        "{\n"
            + "  \"anyMap\": {\n"
            + "    \"int32_wrapper\": {\n"
            + "      \"@type\": \"type.googleapis.com/google.protobuf.Int32Value\",\n"
            + "      \"value\": 123\n"
            + "    },\n"
            + "    \"int64_wrapper\": {\n"
            + "      \"@type\": \"type.googleapis.com/google.protobuf.Int64Value\",\n"
            + "      \"value\": \"456\"\n"
            + "    },\n"
            + "    \"timestamp\": {\n"
            + "      \"@type\": \"type.googleapis.com/google.protobuf.Timestamp\",\n"
            + "      \"value\": \"1969-12-31T23:59:59Z\"\n"
            + "    },\n"
            + "    \"duration\": {\n"
            + "      \"@type\": \"type.googleapis.com/google.protobuf.Duration\",\n"
            + "      \"value\": \"12345.100s\"\n"
            + "    },\n"
            + "    \"field_mask\": {\n"
            + "      \"@type\": \"type.googleapis.com/google.protobuf.FieldMask\",\n"
            + "      \"value\": \"foo.bar,baz\"\n"
            + "    },\n"
            + "    \"struct\": {\n"
            + "      \"@type\": \"type.googleapis.com/google.protobuf.Struct\",\n"
            + "      \"value\": {\n"
            + "        \"number\": 1.125\n"
            + "      }\n"
            + "    },\n"
            + "    \"list_value\": {\n"
            + "      \"@type\": \"type.googleapis.com/google.protobuf.ListValue\",\n"
            + "      \"value\": [1.125, null]\n"
            + "    },\n"
            + "    \"number_value\": {\n"
            + "      \"@type\": \"type.googleapis.com/google.protobuf.Value\",\n"
            + "      \"value\": 1.125\n"
            + "    },\n"
            + "    \"any_value_number\": {\n"
            + "      \"@type\": \"type.googleapis.com/google.protobuf.Any\",\n"
            + "      \"value\": {\n"
            + "        \"@type\": \"type.googleapis.com/google.protobuf.Value\",\n"
            + "        \"value\": 1.125\n"
            + "      }\n"
            + "    },\n"
            + "    \"any_value_default\": {\n"
            + "      \"@type\": \"type.googleapis.com/google.protobuf.Any\",\n"
            + "      \"value\": {}\n"
            + "    },\n"
            + "    \"default\": {}\n"
            + "  }\n"
            + "}",
        printer.print(testAny.build()));
    assertRoundTripEquals(testAny.build(), registry);
  }

  public void testParserMissingTypeUrl() throws Exception {
    try {
      Any.Builder builder = Any.newBuilder();
      mergeFromJson("{\n" + "  \"optionalInt32\": 1234\n" + "}", builder);
      fail("Exception is expected.");
    } catch (IOException e) {
      // Expected.
    }
  }

  public void testParserUnexpectedTypeUrl() throws Exception {
    try {
      Any.Builder builder = Any.newBuilder();
      mergeFromJson(
          "{\n"
              + "  \"@type\": \"type.googleapis.com/json_test.UnexpectedTypes\",\n"
              + "  \"optionalInt32\": 12345\n"
              + "}",
          builder);
      fail("Exception is expected.");
    } catch (IOException e) {
      // Expected.
    }
  }

  public void testParserRejectTrailingComma() throws Exception {
    try {
      TestAllTypes.Builder builder = TestAllTypes.newBuilder();
      mergeFromJson("{\n" + "  \"optionalInt32\": 12345,\n" + "}", builder);
      fail("Exception is expected.");
    } catch (IOException e) {
      // Expected.
    }

    // TODO(xiaofeng): GSON allows trailing comma in arrays even after I set
    // the JsonReader to non-lenient mode. If we want to enforce strict JSON
    // compliance, we might want to switch to a different JSON parser or
    // implement one by ourselves.
    // try {
    //   TestAllTypes.Builder builder = TestAllTypes.newBuilder();
    //   JsonFormat.merge(
    //       "{\n"
    //       + "  \"repeatedInt32\": [12345,]\n"
    //       + "}", builder);
    //   fail("Exception is expected.");
    // } catch (IOException e) {
    //   // Expected.
    // }
  }

  public void testParserRejectInvalidBase64() throws Exception {
    assertRejects("optionalBytes", "!@#$");
  }

  public void testParserAcceptBase64Variants() throws Exception {
    assertAccepts("optionalBytes", "AQI"); // No padding
    assertAccepts("optionalBytes", "-_w"); // base64Url, no padding
  }

  public void testParserRejectInvalidEnumValue() throws Exception {
    try {
      TestAllTypes.Builder builder = TestAllTypes.newBuilder();
      mergeFromJson("{\n" + "  \"optionalNestedEnum\": \"XXX\"\n" + "}", builder);
      fail("Exception is expected.");
    } catch (InvalidProtocolBufferException e) {
      // Expected.
    }
  }

  public void testParserUnknownFields() throws Exception {
    try {
      TestAllTypes.Builder builder = TestAllTypes.newBuilder();
      String json = "{\n" + "  \"unknownField\": \"XXX\"\n" + "}";
      JsonFormat.parser().merge(json, builder);
      fail("Exception is expected.");
    } catch (InvalidProtocolBufferException e) {
      // Expected.
    }
  }

  public void testParserIgnoringUnknownFields() throws Exception {
    TestAllTypes.Builder builder = TestAllTypes.newBuilder();
    String json = "{\n" + "  \"unknownField\": \"XXX\"\n" + "}";
    JsonFormat.parser().ignoringUnknownFields().merge(json, builder);
  }

  public void testParserIgnoringUnknownEnums() throws Exception {
    TestAllTypes.Builder builder = TestAllTypes.newBuilder();
    String json = "{\n" + "  \"optionalNestedEnum\": \"XXX\"\n" + "}";
    JsonFormat.parser().ignoringUnknownFields().merge(json, builder);
    assertEquals(0, builder.getOptionalNestedEnumValue());
  }

  public void testParserSupportAliasEnums() throws Exception {
    TestAllTypes.Builder builder = TestAllTypes.newBuilder();
    String json = "{\n" + "  \"optionalAliasedEnum\": \"QUX\"\n" + "}";
    JsonFormat.parser().merge(json, builder);
    assertEquals(AliasedEnum.ALIAS_BAZ, builder.getOptionalAliasedEnum());

    builder = TestAllTypes.newBuilder();
    json = "{\n" + "  \"optionalAliasedEnum\": \"qux\"\n" + "}";
    JsonFormat.parser().merge(json, builder);
    assertEquals(AliasedEnum.ALIAS_BAZ, builder.getOptionalAliasedEnum());

    builder = TestAllTypes.newBuilder();
    json = "{\n" + "  \"optionalAliasedEnum\": \"bAz\"\n" + "}";
    JsonFormat.parser().merge(json, builder);
    assertEquals(AliasedEnum.ALIAS_BAZ, builder.getOptionalAliasedEnum());
  }

  public void testUnknownEnumMap() throws Exception {
    TestMap.Builder builder = TestMap.newBuilder();
    JsonFormat.parser()
        .ignoringUnknownFields()
        .merge("{\n" + "  \"int32ToEnumMap\": {1: XXX, 2: FOO}" + "}", builder);

    assertEquals(NestedEnum.FOO, builder.getInt32ToEnumMapMap().get(2));
    assertEquals(1, builder.getInt32ToEnumMapMap().size());
  }

  public void testRepeatedUnknownEnum() throws Exception {
    TestAllTypes.Builder builder = TestAllTypes.newBuilder();
    JsonFormat.parser()
        .ignoringUnknownFields()
        .merge("{\n" + "  \"repeatedNestedEnum\": [XXX, FOO, BAR, BAZ]" + "}", builder);

    assertEquals(NestedEnum.FOO, builder.getRepeatedNestedEnum(0));
    assertEquals(NestedEnum.BAR, builder.getRepeatedNestedEnum(1));
    assertEquals(NestedEnum.BAZ, builder.getRepeatedNestedEnum(2));
    assertEquals(3, builder.getRepeatedNestedEnumList().size());
  }

  public void testParserIntegerEnumValue() throws Exception {
    TestAllTypes.Builder actualBuilder = TestAllTypes.newBuilder();
    mergeFromJson("{\n" + "  \"optionalNestedEnum\": 2\n" + "}", actualBuilder);

    TestAllTypes expected = TestAllTypes.newBuilder().setOptionalNestedEnum(NestedEnum.BAZ).build();
    assertEquals(expected, actualBuilder.build());
  }

  public void testCustomJsonName() throws Exception {
    TestCustomJsonName message = TestCustomJsonName.newBuilder().setValue(12345).build();
    assertEquals("{\n" + "  \"@value\": 12345\n" + "}", JsonFormat.printer().print(message));
    assertRoundTripEquals(message);
  }

  // Regression test for b/73832901. Make sure html tags are escaped.
  public void testHtmlEscape() throws Exception {
    TestAllTypes message = TestAllTypes.newBuilder().setOptionalString("</script>").build();
    assertEquals("{\n  \"optionalString\": \"\\u003c/script\\u003e\"\n}", toJsonString(message));

    TestAllTypes.Builder builder = TestAllTypes.newBuilder();
    JsonFormat.parser().merge(toJsonString(message), builder);
    assertEquals(message.getOptionalString(), builder.getOptionalString());
  }

  public void testIncludingDefaultValueFields() throws Exception {
    TestAllTypes message = TestAllTypes.getDefaultInstance();
    assertEquals("{\n}", JsonFormat.printer().print(message));
    assertEquals(
        "{\n"
            + "  \"optionalInt32\": 0,\n"
            + "  \"optionalInt64\": \"0\",\n"
            + "  \"optionalUint32\": 0,\n"
            + "  \"optionalUint64\": \"0\",\n"
            + "  \"optionalSint32\": 0,\n"
            + "  \"optionalSint64\": \"0\",\n"
            + "  \"optionalFixed32\": 0,\n"
            + "  \"optionalFixed64\": \"0\",\n"
            + "  \"optionalSfixed32\": 0,\n"
            + "  \"optionalSfixed64\": \"0\",\n"
            + "  \"optionalFloat\": 0.0,\n"
            + "  \"optionalDouble\": 0.0,\n"
            + "  \"optionalBool\": false,\n"
            + "  \"optionalString\": \"\",\n"
            + "  \"optionalBytes\": \"\",\n"
            + "  \"optionalNestedEnum\": \"FOO\",\n"
            + "  \"repeatedInt32\": [],\n"
            + "  \"repeatedInt64\": [],\n"
            + "  \"repeatedUint32\": [],\n"
            + "  \"repeatedUint64\": [],\n"
            + "  \"repeatedSint32\": [],\n"
            + "  \"repeatedSint64\": [],\n"
            + "  \"repeatedFixed32\": [],\n"
            + "  \"repeatedFixed64\": [],\n"
            + "  \"repeatedSfixed32\": [],\n"
            + "  \"repeatedSfixed64\": [],\n"
            + "  \"repeatedFloat\": [],\n"
            + "  \"repeatedDouble\": [],\n"
            + "  \"repeatedBool\": [],\n"
            + "  \"repeatedString\": [],\n"
            + "  \"repeatedBytes\": [],\n"
            + "  \"repeatedNestedMessage\": [],\n"
            + "  \"repeatedNestedEnum\": [],\n"
            + "  \"optionalAliasedEnum\": \"ALIAS_FOO\"\n"
            + "}",
        JsonFormat.printer().includingDefaultValueFields().print(message));

    Set<FieldDescriptor> fixedFields = new HashSet<FieldDescriptor>();
    for (FieldDescriptor fieldDesc : TestAllTypes.getDescriptor().getFields()) {
      if (fieldDesc.getName().contains("_fixed")) {
        fixedFields.add(fieldDesc);
      }
    }

    assertEquals(
        "{\n"
            + "  \"optionalFixed32\": 0,\n"
            + "  \"optionalFixed64\": \"0\",\n"
            + "  \"repeatedFixed32\": [],\n"
            + "  \"repeatedFixed64\": []\n"
            + "}",
        JsonFormat.printer().includingDefaultValueFields(fixedFields).print(message));

    TestAllTypes messageNonDefaults =
        message.toBuilder().setOptionalInt64(1234).setOptionalFixed32(3232).build();
    assertEquals(
        "{\n"
            + "  \"optionalInt64\": \"1234\",\n"
            + "  \"optionalFixed32\": 3232,\n"
            + "  \"optionalFixed64\": \"0\",\n"
            + "  \"repeatedFixed32\": [],\n"
            + "  \"repeatedFixed64\": []\n"
            + "}",
        JsonFormat.printer().includingDefaultValueFields(fixedFields).print(messageNonDefaults));

    try {
      JsonFormat.printer().includingDefaultValueFields().includingDefaultValueFields();
      fail("IllegalStateException is expected.");
    } catch (IllegalStateException e) {
      // Expected.
      assertTrue(
          "Exception message should mention includingDefaultValueFields.",
          e.getMessage().contains("includingDefaultValueFields"));
    }

    try {
      JsonFormat.printer().includingDefaultValueFields().includingDefaultValueFields(fixedFields);
      fail("IllegalStateException is expected.");
    } catch (IllegalStateException e) {
      // Expected.
      assertTrue(
          "Exception message should mention includingDefaultValueFields.",
          e.getMessage().contains("includingDefaultValueFields"));
    }

    try {
      JsonFormat.printer().includingDefaultValueFields(fixedFields).includingDefaultValueFields();
      fail("IllegalStateException is expected.");
    } catch (IllegalStateException e) {
      // Expected.
      assertTrue(
          "Exception message should mention includingDefaultValueFields.",
          e.getMessage().contains("includingDefaultValueFields"));
    }

    try {
      JsonFormat.printer()
          .includingDefaultValueFields(fixedFields)
          .includingDefaultValueFields(fixedFields);
      fail("IllegalStateException is expected.");
    } catch (IllegalStateException e) {
      // Expected.
      assertTrue(
          "Exception message should mention includingDefaultValueFields.",
          e.getMessage().contains("includingDefaultValueFields"));
    }

    Set<FieldDescriptor> intFields = new HashSet<FieldDescriptor>();
    for (FieldDescriptor fieldDesc : TestAllTypes.getDescriptor().getFields()) {
      if (fieldDesc.getName().contains("_int")) {
        intFields.add(fieldDesc);
      }
    }

    try {
      JsonFormat.printer()
          .includingDefaultValueFields(intFields)
          .includingDefaultValueFields(fixedFields);
      fail("IllegalStateException is expected.");
    } catch (IllegalStateException e) {
      // Expected.
      assertTrue(
          "Exception message should mention includingDefaultValueFields.",
          e.getMessage().contains("includingDefaultValueFields"));
    }

    try {
      JsonFormat.printer().includingDefaultValueFields(null);
      fail("IllegalArgumentException is expected.");
    } catch (IllegalArgumentException e) {
      // Expected.
      assertTrue(
          "Exception message should mention includingDefaultValueFields.",
          e.getMessage().contains("includingDefaultValueFields"));
    }

    try {
      JsonFormat.printer().includingDefaultValueFields(Collections.<FieldDescriptor>emptySet());
      fail("IllegalArgumentException is expected.");
    } catch (IllegalArgumentException e) {
      // Expected.
      assertTrue(
          "Exception message should mention includingDefaultValueFields.",
          e.getMessage().contains("includingDefaultValueFields"));
    }

    TestMap mapMessage = TestMap.getDefaultInstance();
    assertEquals("{\n}", JsonFormat.printer().print(mapMessage));
    assertEquals(
        "{\n"
            + "  \"int32ToInt32Map\": {\n"
            + "  },\n"
            + "  \"int64ToInt32Map\": {\n"
            + "  },\n"
            + "  \"uint32ToInt32Map\": {\n"
            + "  },\n"
            + "  \"uint64ToInt32Map\": {\n"
            + "  },\n"
            + "  \"sint32ToInt32Map\": {\n"
            + "  },\n"
            + "  \"sint64ToInt32Map\": {\n"
            + "  },\n"
            + "  \"fixed32ToInt32Map\": {\n"
            + "  },\n"
            + "  \"fixed64ToInt32Map\": {\n"
            + "  },\n"
            + "  \"sfixed32ToInt32Map\": {\n"
            + "  },\n"
            + "  \"sfixed64ToInt32Map\": {\n"
            + "  },\n"
            + "  \"boolToInt32Map\": {\n"
            + "  },\n"
            + "  \"stringToInt32Map\": {\n"
            + "  },\n"
            + "  \"int32ToInt64Map\": {\n"
            + "  },\n"
            + "  \"int32ToUint32Map\": {\n"
            + "  },\n"
            + "  \"int32ToUint64Map\": {\n"
            + "  },\n"
            + "  \"int32ToSint32Map\": {\n"
            + "  },\n"
            + "  \"int32ToSint64Map\": {\n"
            + "  },\n"
            + "  \"int32ToFixed32Map\": {\n"
            + "  },\n"
            + "  \"int32ToFixed64Map\": {\n"
            + "  },\n"
            + "  \"int32ToSfixed32Map\": {\n"
            + "  },\n"
            + "  \"int32ToSfixed64Map\": {\n"
            + "  },\n"
            + "  \"int32ToFloatMap\": {\n"
            + "  },\n"
            + "  \"int32ToDoubleMap\": {\n"
            + "  },\n"
            + "  \"int32ToBoolMap\": {\n"
            + "  },\n"
            + "  \"int32ToStringMap\": {\n"
            + "  },\n"
            + "  \"int32ToBytesMap\": {\n"
            + "  },\n"
            + "  \"int32ToMessageMap\": {\n"
            + "  },\n"
            + "  \"int32ToEnumMap\": {\n"
            + "  }\n"
            + "}",
        JsonFormat.printer().includingDefaultValueFields().print(mapMessage));

    TestOneof oneofMessage = TestOneof.getDefaultInstance();
    assertEquals("{\n}", JsonFormat.printer().print(oneofMessage));
    assertEquals("{\n}", JsonFormat.printer().includingDefaultValueFields().print(oneofMessage));

    oneofMessage = TestOneof.newBuilder().setOneofInt32(42).build();
    assertEquals("{\n  \"oneofInt32\": 42\n}", JsonFormat.printer().print(oneofMessage));
    assertEquals(
        "{\n  \"oneofInt32\": 42\n}",
        JsonFormat.printer().includingDefaultValueFields().print(oneofMessage));

    TestOneof.Builder oneofBuilder = TestOneof.newBuilder();
    mergeFromJson("{\n" + "  \"oneofNullValue\": null \n" + "}", oneofBuilder);
    oneofMessage = oneofBuilder.build();
    assertEquals("{\n  \"oneofNullValue\": null\n}", JsonFormat.printer().print(oneofMessage));
    assertEquals(
        "{\n  \"oneofNullValue\": null\n}",
        JsonFormat.printer().includingDefaultValueFields().print(oneofMessage));
  }

  public void testPreservingProtoFieldNames() throws Exception {
    TestAllTypes message = TestAllTypes.newBuilder().setOptionalInt32(12345).build();
    assertEquals("{\n" + "  \"optionalInt32\": 12345\n" + "}", JsonFormat.printer().print(message));
    assertEquals(
        "{\n" + "  \"optional_int32\": 12345\n" + "}",
        JsonFormat.printer().preservingProtoFieldNames().print(message));

    // The json_name field option is ignored when configured to use original proto field names.
    TestCustomJsonName messageWithCustomJsonName =
        TestCustomJsonName.newBuilder().setValue(12345).build();
    assertEquals(
        "{\n" + "  \"value\": 12345\n" + "}",
        JsonFormat.printer().preservingProtoFieldNames().print(messageWithCustomJsonName));

    // Parsers accept both original proto field names and lowerCamelCase names.
    TestAllTypes.Builder builder = TestAllTypes.newBuilder();
    JsonFormat.parser().merge("{\"optionalInt32\": 12345}", builder);
    assertEquals(12345, builder.getOptionalInt32());
    builder.clear();
    JsonFormat.parser().merge("{\"optional_int32\": 54321}", builder);
    assertEquals(54321, builder.getOptionalInt32());
  }

  public void testPrintingEnumsAsInts() throws Exception {
    TestAllTypes message = TestAllTypes.newBuilder().setOptionalNestedEnum(NestedEnum.BAR).build();
    assertEquals(
        "{\n" + "  \"optionalNestedEnum\": 1\n" + "}",
        JsonFormat.printer().printingEnumsAsInts().print(message));
  }

  public void testOmittingInsignificantWhiteSpace() throws Exception {
    TestAllTypes message = TestAllTypes.newBuilder().setOptionalInt32(12345).build();
    assertEquals(
        "{" + "\"optionalInt32\":12345" + "}",
        JsonFormat.printer().omittingInsignificantWhitespace().print(message));
    TestAllTypes message1 = TestAllTypes.getDefaultInstance();
    assertEquals("{}", JsonFormat.printer().omittingInsignificantWhitespace().print(message1));
    TestAllTypes.Builder builder = TestAllTypes.newBuilder();
    setAllFields(builder);
    TestAllTypes message2 = builder.build();
    assertEquals(
        "{"
            + "\"optionalInt32\":1234,"
            + "\"optionalInt64\":\"1234567890123456789\","
            + "\"optionalUint32\":5678,"
            + "\"optionalUint64\":\"2345678901234567890\","
            + "\"optionalSint32\":9012,"
            + "\"optionalSint64\":\"3456789012345678901\","
            + "\"optionalFixed32\":3456,"
            + "\"optionalFixed64\":\"4567890123456789012\","
            + "\"optionalSfixed32\":7890,"
            + "\"optionalSfixed64\":\"5678901234567890123\","
            + "\"optionalFloat\":1.5,"
            + "\"optionalDouble\":1.25,"
            + "\"optionalBool\":true,"
            + "\"optionalString\":\"Hello world!\","
            + "\"optionalBytes\":\"AAEC\","
            + "\"optionalNestedMessage\":{"
            + "\"value\":100"
            + "},"
            + "\"optionalNestedEnum\":\"BAR\","
            + "\"repeatedInt32\":[1234,234],"
            + "\"repeatedInt64\":[\"1234567890123456789\",\"234567890123456789\"],"
            + "\"repeatedUint32\":[5678,678],"
            + "\"repeatedUint64\":[\"2345678901234567890\",\"345678901234567890\"],"
            + "\"repeatedSint32\":[9012,10],"
            + "\"repeatedSint64\":[\"3456789012345678901\",\"456789012345678901\"],"
            + "\"repeatedFixed32\":[3456,456],"
            + "\"repeatedFixed64\":[\"4567890123456789012\",\"567890123456789012\"],"
            + "\"repeatedSfixed32\":[7890,890],"
            + "\"repeatedSfixed64\":[\"5678901234567890123\",\"678901234567890123\"],"
            + "\"repeatedFloat\":[1.5,11.5],"
            + "\"repeatedDouble\":[1.25,11.25],"
            + "\"repeatedBool\":[true,true],"
            + "\"repeatedString\":[\"Hello world!\",\"ello world!\"],"
            + "\"repeatedBytes\":[\"AAEC\",\"AQI=\"],"
            + "\"repeatedNestedMessage\":[{"
            + "\"value\":100"
            + "},{"
            + "\"value\":200"
            + "}],"
            + "\"repeatedNestedEnum\":[\"BAR\",\"BAZ\"]"
            + "}",
        toCompactJsonString(message2));
  }

  // Regression test for b/29892357
  public void testEmptyWrapperTypesInAny() throws Exception {
    JsonFormat.TypeRegistry registry =
        JsonFormat.TypeRegistry.newBuilder().add(TestAllTypes.getDescriptor()).build();
    JsonFormat.Parser parser = JsonFormat.parser().usingTypeRegistry(registry);

    Any.Builder builder = Any.newBuilder();
    parser.merge(
        "{\n"
            + "  \"@type\": \"type.googleapis.com/google.protobuf.BoolValue\",\n"
            + "  \"value\": false\n"
            + "}\n",
        builder);
    Any any = builder.build();
    assertEquals(0, any.getValue().size());
  }

  public void testRecursionLimit() throws Exception {
    String input =
        "{\n"
            + "  \"nested\": {\n"
            + "    \"nested\": {\n"
            + "      \"nested\": {\n"
            + "        \"nested\": {\n"
            + "          \"value\": 1234\n"
            + "        }\n"
            + "      }\n"
            + "    }\n"
            + "  }\n"
            + "}\n";

    JsonFormat.Parser parser = JsonFormat.parser();
    TestRecursive.Builder builder = TestRecursive.newBuilder();
    parser.merge(input, builder);
    TestRecursive message = builder.build();
    assertEquals(1234, message.getNested().getNested().getNested().getNested().getValue());

    parser = JsonFormat.parser().usingRecursionLimit(3);
    builder = TestRecursive.newBuilder();
    try {
      parser.merge(input, builder);
      fail("Exception is expected.");
    } catch (InvalidProtocolBufferException e) {
      // Expected.
    }
  }

  // Test that we are not leaking out JSON exceptions.
  public void testJsonException() throws Exception {
    InputStream throwingInputStream =
        new InputStream() {
          public int read() throws IOException {
            throw new IOException("12345");
          }
        };
    InputStreamReader throwingReader = new InputStreamReader(throwingInputStream);
    // When the underlying reader throws IOException, JsonFormat should forward
    // through this IOException.
    try {
      TestAllTypes.Builder builder = TestAllTypes.newBuilder();
      JsonFormat.parser().merge(throwingReader, builder);
      fail("Exception is expected.");
    } catch (IOException e) {
      assertEquals("12345", e.getMessage());
    }

    Reader invalidJsonReader = new StringReader("{ xxx - yyy }");
    // When the JSON parser throws parser exceptions, JsonFormat should turn
    // that into InvalidProtocolBufferException.
    try {
      TestAllTypes.Builder builder = TestAllTypes.newBuilder();
      JsonFormat.parser().merge(invalidJsonReader, builder);
      fail("Exception is expected.");
    } catch (InvalidProtocolBufferException e) {
      // Expected.
    }
  }

  public void testSortedMapKeys() throws Exception {
    TestMap.Builder mapBuilder = TestMap.newBuilder();
    mapBuilder.putStringToInt32Map("\ud834\udd20", 3); // utf-8 F0 9D 84 A0
    mapBuilder.putStringToInt32Map("foo", 99);
    mapBuilder.putStringToInt32Map("xxx", 123);
    mapBuilder.putStringToInt32Map("\u20ac", 1); // utf-8 E2 82 AC
    mapBuilder.putStringToInt32Map("abc", 20);
    mapBuilder.putStringToInt32Map("19", 19);
    mapBuilder.putStringToInt32Map("8", 8);
    mapBuilder.putStringToInt32Map("\ufb00", 2); // utf-8 EF AC 80
    mapBuilder.putInt32ToInt32Map(3, 3);
    mapBuilder.putInt32ToInt32Map(10, 10);
    mapBuilder.putInt32ToInt32Map(5, 5);
    mapBuilder.putInt32ToInt32Map(4, 4);
    mapBuilder.putInt32ToInt32Map(1, 1);
    mapBuilder.putInt32ToInt32Map(2, 2);
    mapBuilder.putInt32ToInt32Map(-3, -3);
    TestMap mapMessage = mapBuilder.build();
    assertEquals(
        "{\n"
            + "  \"int32ToInt32Map\": {\n"
            + "    \"-3\": -3,\n"
            + "    \"1\": 1,\n"
            + "    \"2\": 2,\n"
            + "    \"3\": 3,\n"
            + "    \"4\": 4,\n"
            + "    \"5\": 5,\n"
            + "    \"10\": 10\n"
            + "  },\n"
            + "  \"stringToInt32Map\": {\n"
            + "    \"19\": 19,\n"
            + "    \"8\": 8,\n"
            + "    \"abc\": 20,\n"
            + "    \"foo\": 99,\n"
            + "    \"xxx\": 123,\n"
            + "    \"\u20ac\": 1,\n"
            + "    \"\ufb00\": 2,\n"
            + "    \"\ud834\udd20\": 3\n"
            + "  }\n"
            + "}",
        toSortedJsonString(mapMessage));

    TestMap emptyMap = TestMap.getDefaultInstance();
    assertEquals("{\n}", toSortedJsonString(emptyMap));
  }
}
