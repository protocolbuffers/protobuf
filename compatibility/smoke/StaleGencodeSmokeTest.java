// Protocol Buffers - Google's data interchange format
// Copyright 2025 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

package smoke;

import static com.google.common.truth.Truth.assertThat;

import legacy_gencode_test.proto3.Proto3GencodeTestProto.ForeignEnum;
import legacy_gencode_test.proto3.Proto3GencodeTestProto.ForeignMessage;
import legacy_gencode_test.proto3.Proto3GencodeTestProto.TestMessage;
import legacy_gencode_test.proto3.Proto3GencodeTestProto.TestMostTypesProto3;
import legacy_gencode_test.proto3.Proto3GencodeTestProto.TestMostTypesProto3.AliasedEnum;
import legacy_gencode_test.proto3.Proto3GencodeTestProto.TestMostTypesProto3.NestedEnum;
import legacy_gencode_test.proto3.Proto3GencodeTestProto.TestMostTypesProto3.NestedMessage;

import com.google.protobuf.ByteString;
import com.google.protobuf.Descriptors.FieldDescriptor;
import com.google.protobuf.TextFormat;
import com.google.protobuf.util.JsonFormat;
import java.util.Map;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

/** Unit test that touches a few behaviors on old versions of generated code. */
@RunWith(JUnit4.class)
public class StaleGencodeSmokeTest {
  @Test
  public void testBuilder() {
    TestMessage.Builder b = TestMessage.newBuilder();
    b.setX("hello");
    assertThat(b.getX()).isEqualTo("hello");
    assertThat(b.getY().getZCount()).isEqualTo(0);
    b.getYBuilder().addZ(4);
    assertThat(b.getY().getZCount()).isEqualTo(1);
    assertThat(b.getY().getZList().get(0)).isEqualTo(4);

    TestMessage msg = b.build();
    assertThat(msg.getX()).isEqualTo("hello");
    assertThat(msg.getY().getZList().get(0)).isEqualTo(4);
  }

  @Test
  public void testReflection() {
    TestMessage.Builder b = TestMessage.newBuilder();
    b.setX("hello");
    TestMessage msg = b.build();

    Map<FieldDescriptor, Object> fields = msg.getAllFields();
    assertThat(fields.size()).isEqualTo(1);
    assertThat(fields.values().contains("hello")).isTrue();
  }

  @Test
  public void testSerializeParse() throws Exception {
    TestMessage.Builder b = TestMessage.newBuilder();
    b.setX("hello");
    TestMessage msg = b.build();
    TestMessage roundTrip = TestMessage.parseFrom(msg.toByteArray());
    assertThat(roundTrip).isEqualTo(msg);
  }

  @Test
  public void testSerializeParseJson() throws Exception {
    TestMessage.Builder b = TestMessage.newBuilder();
    b.setX("hello");
    TestMessage msg = b.build();
    String json = JsonFormat.printer().print(msg);

    TestMessage.Builder roundTrip = TestMessage.newBuilder();
    JsonFormat.parser().merge(json, roundTrip);
    assertThat(roundTrip.build()).isEqualTo(msg);
  }

  @Test
  public void testSerializeParseText() throws Exception {
    TestMessage.Builder b = TestMessage.newBuilder();
    b.setX("hello");
    TestMessage msg = b.build();
    String text = TextFormat.printer().printToString(msg);

    TestMessage.Builder roundTrip = TestMessage.newBuilder();
    TextFormat.getParser().merge(text, roundTrip);
    assertThat(roundTrip.build()).isEqualTo(msg);
  }

  void check(TestMostTypesProto3.Builder b) throws Exception {
    TestMostTypesProto3 msg = b.build();
    TestMostTypesProto3 roundTrip = TestMostTypesProto3.parseFrom(msg.toByteArray());
    assertThat(roundTrip).isEqualTo(msg);
  }

  @Test
  public void testRoundtripScalar() throws Exception {
    TestMostTypesProto3.Builder b = TestMostTypesProto3.newBuilder();
    b.setOptionalInt32(1001);
    b.setOptionalInt64(1002);
    b.setOptionalUint32(1003);
    b.setOptionalUint64(1004);
    b.setOptionalSint32(1005);
    b.setOptionalSint64(1006);
    b.setOptionalFixed32(1007);
    b.setOptionalFixed64(1008);
    b.setOptionalSfixed32(1009);
    b.setOptionalSfixed64(1010);
    b.setOptionalFloat(1011.0f);
    b.setOptionalDouble(1012.0);
    b.setOptionalBool(true);
    b.setOptionalString("1014");
    b.setOptionalBytes(ByteString.copyFromUtf8("1015"));

    NestedMessage.Builder b2 = NestedMessage.newBuilder();
    b2.setA(1018);
    b.setOptionalNestedMessage(b2.build());

    ForeignMessage.Builder b3 = ForeignMessage.newBuilder();
    b3.setC(1019);
    b.setOptionalForeignMessage(b3.build());

    b.setOptionalNestedEnum(NestedEnum.BAZ);
    b.setOptionalForeignEnum(ForeignEnum.FOREIGN_BAZ);
    b.setOptionalAliasedEnum(AliasedEnum.ALIAS_BAZ);
    check(b);

    TestMostTypesProto3.Builder c = TestMostTypesProto3.newBuilder();
    c.setRecursiveMessage(b);
    check(c);
  }

  @Test
  public void testRoundtripRepeated() throws Exception {
    TestMostTypesProto3.Builder b = TestMostTypesProto3.newBuilder();
    b.addRepeatedInt32(1031);
    b.addRepeatedInt64(1032);
    b.addRepeatedUint32(1033);
    b.addRepeatedUint64(1034);
    b.addRepeatedSint32(1035);
    b.addRepeatedSint64(1036);
    b.addRepeatedFixed32(1037);
    b.addRepeatedFixed64(1038);
    b.addRepeatedSfixed32(1039);
    b.addRepeatedSfixed64(1040);
    b.addRepeatedFloat(1041.0f);
    b.addRepeatedDouble(1042.0);
    b.addRepeatedBool(true);
    b.addRepeatedString("1044");
    b.addRepeatedBytes(ByteString.copyFromUtf8("1045"));

    NestedMessage.Builder b2 = NestedMessage.newBuilder();
    b2.setA(1048);
    b.addRepeatedNestedMessage(b2.build());

    ForeignMessage.Builder b3 = ForeignMessage.newBuilder();
    b3.setC(1049);
    b.addRepeatedForeignMessage(b3.build());

    b.addRepeatedNestedEnum(NestedEnum.BAZ);
    b.addRepeatedForeignEnum(ForeignEnum.FOREIGN_BAZ);
    check(b);
  }

  @Test
  public void testRoundtripPacked() throws Exception {
    TestMostTypesProto3.Builder b = TestMostTypesProto3.newBuilder();
    b.addPackedInt32(1075);
    b.addPackedInt64(1076);
    b.addPackedUint32(1077);
    b.addPackedUint64(1078);
    b.addPackedSint32(1079);
    b.addPackedSint64(1080);
    b.addPackedFixed32(1081);
    b.addPackedFixed64(1082);
    b.addPackedSfixed32(1083);
    b.addPackedSfixed64(1084);
    b.addPackedFloat(1085.0f);
    b.addPackedDouble(1086.0);
    b.addPackedBool(true);
    b.addPackedNestedEnum(NestedEnum.BAZ);
    check(b);
  }

  @Test
  public void testRoundtripUnpacked() throws Exception {
    TestMostTypesProto3.Builder b = TestMostTypesProto3.newBuilder();
    b.addUnpackedInt32(1089);
    b.addUnpackedInt64(1090);
    b.addUnpackedUint32(1091);
    b.addUnpackedUint64(1092);
    b.addUnpackedSint32(1093);
    b.addUnpackedSint64(1094);
    b.addUnpackedFixed32(1095);
    b.addUnpackedFixed64(1096);
    b.addUnpackedSfixed32(1097);
    b.addUnpackedSfixed64(1098);
    b.addUnpackedFloat(1099.0f);
    b.addUnpackedDouble(1100.0);
    b.addUnpackedBool(true);
    b.addUnpackedNestedEnum(NestedEnum.BAZ);
    check(b);
  }

  @Test
  public void testRoundtripMap() throws Exception {
    TestMostTypesProto3.Builder b = TestMostTypesProto3.newBuilder();
    b.putMapInt32Int32(1056, 1056);
    b.putMapInt64Int64(1057, 1057);
    b.putMapUint32Uint32(1058, 1058);
    b.putMapUint64Uint64(1059, 1059);
    b.putMapSint32Sint32(1060, 1060);
    b.putMapSint64Sint64(1061, 1061);
    b.putMapFixed32Fixed32(1062, 1062);
    b.putMapFixed64Fixed64(1063, 1063);
    b.putMapSfixed32Sfixed32(1064, 1064);
    b.putMapSfixed64Sfixed64(1065, 1065);
    b.putMapInt32Float(1066, 1066.0f);
    b.putMapInt32Double(1067, 1067.0);
    b.putMapBoolBool(true, true);
    b.putMapStringString("1069", "1069");
    b.putMapStringBytes("1070", ByteString.copyFromUtf8("1070"));

    NestedMessage.Builder b2 = NestedMessage.newBuilder();
    b2.setA(1071);
    b.putMapStringNestedMessage("1071", b2.build());

    ForeignMessage.Builder b3 = ForeignMessage.newBuilder();
    b3.setC(1072);
    b.putMapStringForeignMessage("1072", b3.build());

    b.putMapStringNestedEnum("1073", NestedEnum.BAZ);
    b.putMapStringForeignEnum("1074", ForeignEnum.FOREIGN_BAZ);
    check(b);
  }

  @Test
  public void testRoundtripOneof() throws Exception {
    {
      TestMostTypesProto3.Builder b = TestMostTypesProto3.newBuilder();
      b.setOneofUint32(1111);
      check(b);
    }
    {
      NestedMessage.Builder b2 = NestedMessage.newBuilder();
      b2.setA(1112);

      TestMostTypesProto3.Builder b = TestMostTypesProto3.newBuilder();
      b.setOneofNestedMessage(b2);
      check(b);
    }
    {
      TestMostTypesProto3.Builder b = TestMostTypesProto3.newBuilder();
      b.setOneofString("1113");
      check(b);
    }
    {
      TestMostTypesProto3.Builder b = TestMostTypesProto3.newBuilder();
      b.setOneofBytes(ByteString.copyFromUtf8("1114"));
      check(b);
    }
    {
      TestMostTypesProto3.Builder b = TestMostTypesProto3.newBuilder();
      b.setOneofBool(true);
      check(b);
    }
    {
      TestMostTypesProto3.Builder b = TestMostTypesProto3.newBuilder();
      b.setOneofUint64(1116);
      check(b);
    }
    {
      TestMostTypesProto3.Builder b = TestMostTypesProto3.newBuilder();
      b.setOneofFloat(1117.0f);
      check(b);
    }
    {
      TestMostTypesProto3.Builder b = TestMostTypesProto3.newBuilder();
      b.setOneofDouble(1118.0);
      check(b);
    }
  }
}
