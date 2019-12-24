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

import static com.google.protobuf.FieldInfo.forField;
import static com.google.protobuf.FieldInfo.forMapField;
import static com.google.protobuf.FieldInfo.forOneofMemberField;
import static com.google.protobuf.FieldInfo.forRepeatedMessageField;

import com.google.protobuf.testing.Proto3TestingLite.Proto3EmptyLite;
import com.google.protobuf.testing.Proto3TestingLite.Proto3MessageLite;
import com.google.protobuf.testing.Proto3TestingLite.Proto3MessageLiteWithMaps;
import java.lang.reflect.Field;

/** A factory that generates a hard-coded info for {@link Proto3MessageLite}. */
public final class Proto3MessageLiteInfoFactory implements MessageInfoFactory {
  private static final Proto3MessageLiteInfoFactory instanceForRawMessageInfo =
      new Proto3MessageLiteInfoFactory(true);
  private static final Proto3MessageLiteInfoFactory instanceForStructuralMessageInfo =
      new Proto3MessageLiteInfoFactory(false);

  public static Proto3MessageLiteInfoFactory getInstanceForRawMessageInfo() {
    return instanceForRawMessageInfo;
  }

  public static Proto3MessageLiteInfoFactory getInstanceForStructuralMessageInfo() {
    return instanceForStructuralMessageInfo;
  }

  private final boolean produceRawMessageInfo;

  private Proto3MessageLiteInfoFactory(boolean produceRawMessageInfo) {
    this.produceRawMessageInfo = produceRawMessageInfo;
  }

  @Override
  public boolean isSupported(Class<?> clazz) {
    return true;
  }

  @Override
  public MessageInfo messageInfoFor(Class<?> clazz) {
    return produceRawMessageInfo ? rawMessageInfoFor(clazz) : structuralMessageInfoFor(clazz);
  }

  private MessageInfo rawMessageInfoFor(Class<?> clazz) {
    if (Proto3MessageLite.class.isAssignableFrom(clazz)) {
      return newRawMessageInfoForProto3MessageLite();
    } else {
      throw new IllegalArgumentException("Unsupported class: " + clazz.getName());
    }
  }

  private MessageInfo newRawMessageInfoForProto3MessageLite() {
    java.lang.Object[] objects =
        new java.lang.Object[] {
          "testOneof_",
          "testOneofCase_",
          "fieldDouble1_",
          "fieldFloat2_",
          "fieldInt643_",
          "fieldUint644_",
          "fieldInt325_",
          "fieldFixed646_",
          "fieldFixed327_",
          "fieldBool8_",
          "fieldString9_",
          "fieldMessage10_",
          "fieldBytes11_",
          "fieldUint3212_",
          "fieldEnum13_",
          "fieldSfixed3214_",
          "fieldSfixed6415_",
          "fieldSint3216_",
          "fieldSint6417_",
          "fieldDoubleList18_",
          "fieldFloatList19_",
          "fieldInt64List20_",
          "fieldUint64List21_",
          "fieldInt32List22_",
          "fieldFixed64List23_",
          "fieldFixed32List24_",
          "fieldBoolList25_",
          "fieldStringList26_",
          "fieldMessageList27_",
          Proto3MessageLite.class,
          "fieldBytesList28_",
          "fieldUint32List29_",
          "fieldEnumList30_",
          "fieldSfixed32List31_",
          "fieldSfixed64List32_",
          "fieldSint32List33_",
          "fieldSint64List34_",
          "fieldDoubleListPacked35_",
          "fieldFloatListPacked36_",
          "fieldInt64ListPacked37_",
          "fieldUint64ListPacked38_",
          "fieldInt32ListPacked39_",
          "fieldFixed64ListPacked40_",
          "fieldFixed32ListPacked41_",
          "fieldBoolListPacked42_",
          "fieldUint32ListPacked43_",
          "fieldEnumListPacked44_",
          "fieldSfixed32ListPacked45_",
          "fieldSfixed64ListPacked46_",
          "fieldSint32ListPacked47_",
          "fieldSint64ListPacked48_",
          Proto3MessageLite.class,
        };
    // To update this after a proto change, run protoc on proto3_message_lite.proto and copy over
    // the content of the generated buildMessageInfo() method here.
    java.lang.String info =
        "\u0000@\u0001\u0000\u0001D@\u0000\u001f\u0000\u0001\u0000\u0002\u0001\u0003\u0002"
            + "\u0004\u0003\u0005\u0004\u0006\u0005\u0007\u0006\b\u0007\t\u0208\n\t\u000b\n\f\u000b"
            + "\r\f\u000e\r\u000f\u000e\u0010\u000f\u0011\u0010\u0012\u0012\u0013\u0013\u0014\u0014"
            + "\u0015\u0015\u0016\u0016\u0017\u0017\u0018\u0018\u0019\u0019\u001a\u021a\u001b\u001b"
            + "\u001c\u001c\u001d\u001d\u001e\u001e\u001f\u001f  !!\"\"##$$%%&&\'\'(())**++,,--"
            + "..//0053\u000064\u000075\u000086\u000097\u0000:8\u0000;9\u0000<:\u0000=\u023b\u0000"
            + "><\u0000?=\u0000@>\u0000A@\u0000BA\u0000CB\u0000DC\u0000";
    return new RawMessageInfo(Proto3MessageLite.getDefaultInstance(), info, objects);
  }

  private MessageInfo structuralMessageInfoFor(Class<?> clazz) {
    if (Proto3MessageLite.class.isAssignableFrom(clazz)) {
      return newMessageInfoForProto3MessageLite();
    } else if (Proto3EmptyLite.class.isAssignableFrom(clazz)) {
      return newMessageInfoForProto3EmptyLite();
    } else if (Proto3MessageLiteWithMaps.class.isAssignableFrom(clazz)) {
      return newMessageInfoForProto3MessageLiteWithMaps();
    } else {
      throw new IllegalArgumentException("Unsupported class: " + clazz.getName());
    }
  }

  /**
   * Creates a new hard-coded info for {@link Proto3MessageLite}. Each time this is called, we
   * manually go through the entire process of what a message would do if it self-registered its own
   * info, including looking up each field by name. This is done for benchmarking purposes, so that
   * we get a more accurate representation of the time it takes to perform this process.
   */
  private static StructuralMessageInfo newMessageInfoForProto3MessageLite() {
    StructuralMessageInfo.Builder builder = StructuralMessageInfo.newBuilder(48);
    lookupFieldsByName(builder);
    return builder.build();
  }

  private static void lookupFieldsByName(StructuralMessageInfo.Builder builder) {
    builder.withDefaultInstance(Proto3MessageLite.getDefaultInstance());
    builder.withSyntax(ProtoSyntax.PROTO3);
    builder.withField(
        forField(field(Proto3MessageLite.class, "fieldDouble1_"), 1, FieldType.DOUBLE, true));
    builder.withField(
        forField(field(Proto3MessageLite.class, "fieldFloat2_"), 2, FieldType.FLOAT, true));
    builder.withField(
        forField(field(Proto3MessageLite.class, "fieldInt643_"), 3, FieldType.INT64, true));
    builder.withField(
        forField(field(Proto3MessageLite.class, "fieldUint644_"), 4, FieldType.UINT64, true));
    builder.withField(
        forField(field(Proto3MessageLite.class, "fieldInt325_"), 5, FieldType.INT32, true));
    builder.withField(
        forField(field(Proto3MessageLite.class, "fieldFixed646_"), 6, FieldType.FIXED64, true));
    builder.withField(
        forField(field(Proto3MessageLite.class, "fieldFixed327_"), 7, FieldType.FIXED32, true));
    builder.withField(
        forField(field(Proto3MessageLite.class, "fieldBool8_"), 8, FieldType.BOOL, true));
    builder.withField(
        forField(field(Proto3MessageLite.class, "fieldString9_"), 9, FieldType.STRING, true));
    builder.withField(
        forField(field(Proto3MessageLite.class, "fieldMessage10_"), 10, FieldType.MESSAGE, true));
    builder.withField(
        forField(field(Proto3MessageLite.class, "fieldBytes11_"), 11, FieldType.BYTES, true));
    builder.withField(
        forField(field(Proto3MessageLite.class, "fieldUint3212_"), 12, FieldType.UINT32, true));
    builder.withField(
        forField(field(Proto3MessageLite.class, "fieldEnum13_"), 13, FieldType.ENUM, true));
    builder.withField(
        forField(field(Proto3MessageLite.class, "fieldSfixed3214_"), 14, FieldType.SFIXED32, true));
    builder.withField(
        forField(field(Proto3MessageLite.class, "fieldSfixed6415_"), 15, FieldType.SFIXED64, true));
    builder.withField(
        forField(field(Proto3MessageLite.class, "fieldSint3216_"), 16, FieldType.SINT32, true));
    builder.withField(
        forField(field(Proto3MessageLite.class, "fieldSint6417_"), 17, FieldType.SINT64, true));
    builder.withField(
        forField(
            field(Proto3MessageLite.class, "fieldDoubleList18_"), 18, FieldType.DOUBLE_LIST, true));
    builder.withField(
        forField(
            field(Proto3MessageLite.class, "fieldFloatList19_"), 19, FieldType.FLOAT_LIST, true));
    builder.withField(
        forField(
            field(Proto3MessageLite.class, "fieldInt64List20_"), 20, FieldType.INT64_LIST, true));
    builder.withField(
        forField(
            field(Proto3MessageLite.class, "fieldUint64List21_"), 21, FieldType.UINT64_LIST, true));
    builder.withField(
        forField(
            field(Proto3MessageLite.class, "fieldInt32List22_"), 22, FieldType.INT32_LIST, true));
    builder.withField(
        forField(
            field(Proto3MessageLite.class, "fieldFixed64List23_"),
            23,
            FieldType.FIXED64_LIST,
            true));
    builder.withField(
        forField(
            field(Proto3MessageLite.class, "fieldFixed32List24_"),
            24,
            FieldType.FIXED32_LIST,
            true));
    builder.withField(
        forField(
            field(Proto3MessageLite.class, "fieldBoolList25_"), 25, FieldType.BOOL_LIST, true));
    builder.withField(
        forField(
            field(Proto3MessageLite.class, "fieldStringList26_"), 26, FieldType.STRING_LIST, true));
    builder.withField(
        forRepeatedMessageField(
            field(Proto3MessageLite.class, "fieldMessageList27_"),
            27,
            FieldType.MESSAGE_LIST,
            Proto3MessageLite.class));
    builder.withField(
        forField(
            field(Proto3MessageLite.class, "fieldBytesList28_"), 28, FieldType.BYTES_LIST, true));
    builder.withField(
        forField(
            field(Proto3MessageLite.class, "fieldUint32List29_"), 29, FieldType.UINT32_LIST, true));
    builder.withField(
        forField(
            field(Proto3MessageLite.class, "fieldEnumList30_"), 30, FieldType.ENUM_LIST, true));
    builder.withField(
        forField(
            field(Proto3MessageLite.class, "fieldSfixed32List31_"),
            31,
            FieldType.SFIXED32_LIST,
            true));
    builder.withField(
        forField(
            field(Proto3MessageLite.class, "fieldSfixed64List32_"),
            32,
            FieldType.SFIXED64_LIST,
            true));
    builder.withField(
        forField(
            field(Proto3MessageLite.class, "fieldSint32List33_"), 33, FieldType.SINT32_LIST, true));
    builder.withField(
        forField(
            field(Proto3MessageLite.class, "fieldSint64List34_"), 34, FieldType.SINT64_LIST, true));
    builder.withField(
        forField(
            field(Proto3MessageLite.class, "fieldDoubleListPacked35_"),
            35,
            FieldType.DOUBLE_LIST_PACKED,
            true));
    builder.withField(
        forField(
            field(Proto3MessageLite.class, "fieldFloatListPacked36_"),
            36,
            FieldType.FLOAT_LIST_PACKED,
            true));
    builder.withField(
        forField(
            field(Proto3MessageLite.class, "fieldInt64ListPacked37_"),
            37,
            FieldType.INT64_LIST_PACKED,
            true));
    builder.withField(
        forField(
            field(Proto3MessageLite.class, "fieldUint64ListPacked38_"),
            38,
            FieldType.UINT64_LIST_PACKED,
            true));
    builder.withField(
        forField(
            field(Proto3MessageLite.class, "fieldInt32ListPacked39_"),
            39,
            FieldType.INT32_LIST_PACKED,
            true));
    builder.withField(
        forField(
            field(Proto3MessageLite.class, "fieldFixed64ListPacked40_"),
            40,
            FieldType.FIXED64_LIST_PACKED,
            true));
    builder.withField(
        forField(
            field(Proto3MessageLite.class, "fieldFixed32ListPacked41_"),
            41,
            FieldType.FIXED32_LIST_PACKED,
            true));
    builder.withField(
        forField(
            field(Proto3MessageLite.class, "fieldBoolListPacked42_"),
            42,
            FieldType.BOOL_LIST_PACKED,
            true));
    builder.withField(
        forField(
            field(Proto3MessageLite.class, "fieldUint32ListPacked43_"),
            43,
            FieldType.UINT32_LIST_PACKED,
            true));
    builder.withField(
        forField(
            field(Proto3MessageLite.class, "fieldEnumListPacked44_"),
            44,
            FieldType.ENUM_LIST_PACKED,
            true));
    builder.withField(
        forField(
            field(Proto3MessageLite.class, "fieldSfixed32ListPacked45_"),
            45,
            FieldType.SFIXED32_LIST_PACKED,
            true));
    builder.withField(
        forField(
            field(Proto3MessageLite.class, "fieldSfixed64ListPacked46_"),
            46,
            FieldType.SFIXED64_LIST_PACKED,
            true));
    builder.withField(
        forField(
            field(Proto3MessageLite.class, "fieldSint32ListPacked47_"),
            47,
            FieldType.SINT32_LIST_PACKED,
            true));
    builder.withField(
        forField(
            field(Proto3MessageLite.class, "fieldSint64ListPacked48_"),
            48,
            FieldType.SINT64_LIST_PACKED,
            true));

    OneofInfo oneof =
        new OneofInfo(
            0,
            field(Proto3MessageLite.class, "testOneofCase_"),
            field(Proto3MessageLite.class, "testOneof_"));
    builder.withField(forOneofMemberField(53, FieldType.DOUBLE, oneof, Double.class, true, null));
    builder.withField(forOneofMemberField(54, FieldType.FLOAT, oneof, Float.class, true, null));
    builder.withField(forOneofMemberField(55, FieldType.INT64, oneof, Long.class, true, null));
    builder.withField(forOneofMemberField(56, FieldType.UINT64, oneof, Long.class, true, null));
    builder.withField(forOneofMemberField(57, FieldType.INT32, oneof, Integer.class, true, null));
    builder.withField(forOneofMemberField(58, FieldType.FIXED64, oneof, Long.class, true, null));
    builder.withField(forOneofMemberField(59, FieldType.FIXED32, oneof, Integer.class, true, null));
    builder.withField(forOneofMemberField(60, FieldType.BOOL, oneof, Boolean.class, true, null));
    builder.withField(forOneofMemberField(61, FieldType.STRING, oneof, String.class, true, null));
    builder.withField(
        forOneofMemberField(62, FieldType.MESSAGE, oneof, Proto3MessageLite.class, true, null));
    builder.withField(
        forOneofMemberField(63, FieldType.BYTES, oneof, ByteString.class, true, null));
    builder.withField(forOneofMemberField(64, FieldType.UINT32, oneof, Integer.class, true, null));
    builder.withField(
        forOneofMemberField(65, FieldType.SFIXED32, oneof, Integer.class, true, null));
    builder.withField(forOneofMemberField(66, FieldType.SFIXED64, oneof, Long.class, true, null));
    builder.withField(forOneofMemberField(67, FieldType.SINT32, oneof, Integer.class, true, null));
    builder.withField(forOneofMemberField(68, FieldType.SINT64, oneof, Long.class, true, null));
  }

  private StructuralMessageInfo newMessageInfoForProto3EmptyLite() {
    StructuralMessageInfo.Builder builder = StructuralMessageInfo.newBuilder(1);
    builder.withSyntax(ProtoSyntax.PROTO3);
    return builder.build();
  }

  private static StructuralMessageInfo newMessageInfoForProto3MessageLiteWithMaps() {
    StructuralMessageInfo.Builder builder = StructuralMessageInfo.newBuilder();
    builder.withSyntax(ProtoSyntax.PROTO2);
    builder.withField(mapFieldInfo(Proto3MessageLiteWithMaps.class, "field_map_bool_bool_1", 1));
    builder.withField(mapFieldInfo(Proto3MessageLiteWithMaps.class, "field_map_bool_bytes_2", 2));
    builder.withField(mapFieldInfo(Proto3MessageLiteWithMaps.class, "field_map_bool_double_3", 3));
    builder.withField(mapFieldInfo(Proto3MessageLiteWithMaps.class, "field_map_bool_enum_4", 4));
    builder.withField(mapFieldInfo(Proto3MessageLiteWithMaps.class, "field_map_bool_fixed32_5", 5));
    builder.withField(mapFieldInfo(Proto3MessageLiteWithMaps.class, "field_map_bool_fixed64_6", 6));
    builder.withField(mapFieldInfo(Proto3MessageLiteWithMaps.class, "field_map_bool_float_7", 7));
    builder.withField(mapFieldInfo(Proto3MessageLiteWithMaps.class, "field_map_bool_int32_8", 8));
    builder.withField(mapFieldInfo(Proto3MessageLiteWithMaps.class, "field_map_bool_int64_9", 9));
    builder.withField(
        mapFieldInfo(Proto3MessageLiteWithMaps.class, "field_map_bool_message_10", 10));
    builder.withField(
        mapFieldInfo(Proto3MessageLiteWithMaps.class, "field_map_bool_sfixed32_11", 11));
    builder.withField(
        mapFieldInfo(Proto3MessageLiteWithMaps.class, "field_map_bool_sfixed64_12", 12));
    builder.withField(
        mapFieldInfo(Proto3MessageLiteWithMaps.class, "field_map_bool_sint32_13", 13));
    builder.withField(
        mapFieldInfo(Proto3MessageLiteWithMaps.class, "field_map_bool_sint64_14", 14));
    builder.withField(
        mapFieldInfo(Proto3MessageLiteWithMaps.class, "field_map_bool_string_15", 15));
    builder.withField(
        mapFieldInfo(Proto3MessageLiteWithMaps.class, "field_map_bool_uint32_16", 16));
    builder.withField(
        mapFieldInfo(Proto3MessageLiteWithMaps.class, "field_map_bool_uint64_17", 17));
    builder.withField(
        mapFieldInfo(Proto3MessageLiteWithMaps.class, "field_map_fixed32_bool_18", 18));
    builder.withField(
        mapFieldInfo(Proto3MessageLiteWithMaps.class, "field_map_fixed32_bytes_19", 19));
    builder.withField(
        mapFieldInfo(Proto3MessageLiteWithMaps.class, "field_map_fixed32_double_20", 20));
    builder.withField(
        mapFieldInfo(Proto3MessageLiteWithMaps.class, "field_map_fixed32_enum_21", 21));
    builder.withField(
        mapFieldInfo(Proto3MessageLiteWithMaps.class, "field_map_fixed32_fixed32_22", 22));
    builder.withField(
        mapFieldInfo(Proto3MessageLiteWithMaps.class, "field_map_fixed32_fixed64_23", 23));
    builder.withField(
        mapFieldInfo(Proto3MessageLiteWithMaps.class, "field_map_fixed32_float_24", 24));
    builder.withField(
        mapFieldInfo(Proto3MessageLiteWithMaps.class, "field_map_fixed32_int32_25", 25));
    builder.withField(
        mapFieldInfo(Proto3MessageLiteWithMaps.class, "field_map_fixed32_int64_26", 26));
    builder.withField(
        mapFieldInfo(Proto3MessageLiteWithMaps.class, "field_map_fixed32_message_27", 27));
    builder.withField(
        mapFieldInfo(Proto3MessageLiteWithMaps.class, "field_map_fixed32_sfixed32_28", 28));
    builder.withField(
        mapFieldInfo(Proto3MessageLiteWithMaps.class, "field_map_fixed32_sfixed64_29", 29));
    builder.withField(
        mapFieldInfo(Proto3MessageLiteWithMaps.class, "field_map_fixed32_sint32_30", 30));
    builder.withField(
        mapFieldInfo(Proto3MessageLiteWithMaps.class, "field_map_fixed32_sint64_31", 31));
    builder.withField(
        mapFieldInfo(Proto3MessageLiteWithMaps.class, "field_map_fixed32_string_32", 32));
    builder.withField(
        mapFieldInfo(Proto3MessageLiteWithMaps.class, "field_map_fixed32_uint32_33", 33));
    builder.withField(
        mapFieldInfo(Proto3MessageLiteWithMaps.class, "field_map_fixed32_uint64_34", 34));
    builder.withField(
        mapFieldInfo(Proto3MessageLiteWithMaps.class, "field_map_fixed64_bool_35", 35));
    builder.withField(
        mapFieldInfo(Proto3MessageLiteWithMaps.class, "field_map_fixed64_bytes_36", 36));
    builder.withField(
        mapFieldInfo(Proto3MessageLiteWithMaps.class, "field_map_fixed64_double_37", 37));
    builder.withField(
        mapFieldInfo(Proto3MessageLiteWithMaps.class, "field_map_fixed64_enum_38", 38));
    builder.withField(
        mapFieldInfo(Proto3MessageLiteWithMaps.class, "field_map_fixed64_fixed32_39", 39));
    builder.withField(
        mapFieldInfo(Proto3MessageLiteWithMaps.class, "field_map_fixed64_fixed64_40", 40));
    builder.withField(
        mapFieldInfo(Proto3MessageLiteWithMaps.class, "field_map_fixed64_float_41", 41));
    builder.withField(
        mapFieldInfo(Proto3MessageLiteWithMaps.class, "field_map_fixed64_int32_42", 42));
    builder.withField(
        mapFieldInfo(Proto3MessageLiteWithMaps.class, "field_map_fixed64_int64_43", 43));
    builder.withField(
        mapFieldInfo(Proto3MessageLiteWithMaps.class, "field_map_fixed64_message_44", 44));
    builder.withField(
        mapFieldInfo(Proto3MessageLiteWithMaps.class, "field_map_fixed64_sfixed32_45", 45));
    builder.withField(
        mapFieldInfo(Proto3MessageLiteWithMaps.class, "field_map_fixed64_sfixed64_46", 46));
    builder.withField(
        mapFieldInfo(Proto3MessageLiteWithMaps.class, "field_map_fixed64_sint32_47", 47));
    builder.withField(
        mapFieldInfo(Proto3MessageLiteWithMaps.class, "field_map_fixed64_sint64_48", 48));
    builder.withField(
        mapFieldInfo(Proto3MessageLiteWithMaps.class, "field_map_fixed64_string_49", 49));
    builder.withField(
        mapFieldInfo(Proto3MessageLiteWithMaps.class, "field_map_fixed64_uint32_50", 50));
    builder.withField(
        mapFieldInfo(Proto3MessageLiteWithMaps.class, "field_map_fixed64_uint64_51", 51));
    builder.withField(mapFieldInfo(Proto3MessageLiteWithMaps.class, "field_map_int32_bool_52", 52));
    builder.withField(
        mapFieldInfo(Proto3MessageLiteWithMaps.class, "field_map_int32_bytes_53", 53));
    builder.withField(
        mapFieldInfo(Proto3MessageLiteWithMaps.class, "field_map_int32_double_54", 54));
    builder.withField(mapFieldInfo(Proto3MessageLiteWithMaps.class, "field_map_int32_enum_55", 55));
    builder.withField(
        mapFieldInfo(Proto3MessageLiteWithMaps.class, "field_map_int32_fixed32_56", 56));
    builder.withField(
        mapFieldInfo(Proto3MessageLiteWithMaps.class, "field_map_int32_fixed64_57", 57));
    builder.withField(
        mapFieldInfo(Proto3MessageLiteWithMaps.class, "field_map_int32_float_58", 58));
    builder.withField(
        mapFieldInfo(Proto3MessageLiteWithMaps.class, "field_map_int32_int32_59", 59));
    builder.withField(
        mapFieldInfo(Proto3MessageLiteWithMaps.class, "field_map_int32_int64_60", 60));
    builder.withField(
        mapFieldInfo(Proto3MessageLiteWithMaps.class, "field_map_int32_message_61", 61));
    builder.withField(
        mapFieldInfo(Proto3MessageLiteWithMaps.class, "field_map_int32_sfixed32_62", 62));
    builder.withField(
        mapFieldInfo(Proto3MessageLiteWithMaps.class, "field_map_int32_sfixed64_63", 63));
    builder.withField(
        mapFieldInfo(Proto3MessageLiteWithMaps.class, "field_map_int32_sint32_64", 64));
    builder.withField(
        mapFieldInfo(Proto3MessageLiteWithMaps.class, "field_map_int32_sint64_65", 65));
    builder.withField(
        mapFieldInfo(Proto3MessageLiteWithMaps.class, "field_map_int32_string_66", 66));
    builder.withField(
        mapFieldInfo(Proto3MessageLiteWithMaps.class, "field_map_int32_uint32_67", 67));
    builder.withField(
        mapFieldInfo(Proto3MessageLiteWithMaps.class, "field_map_int32_uint64_68", 68));
    builder.withField(mapFieldInfo(Proto3MessageLiteWithMaps.class, "field_map_int64_bool_69", 69));
    builder.withField(
        mapFieldInfo(Proto3MessageLiteWithMaps.class, "field_map_int64_bytes_70", 70));
    builder.withField(
        mapFieldInfo(Proto3MessageLiteWithMaps.class, "field_map_int64_double_71", 71));
    builder.withField(mapFieldInfo(Proto3MessageLiteWithMaps.class, "field_map_int64_enum_72", 72));
    builder.withField(
        mapFieldInfo(Proto3MessageLiteWithMaps.class, "field_map_int64_fixed32_73", 73));
    builder.withField(
        mapFieldInfo(Proto3MessageLiteWithMaps.class, "field_map_int64_fixed64_74", 74));
    builder.withField(
        mapFieldInfo(Proto3MessageLiteWithMaps.class, "field_map_int64_float_75", 75));
    builder.withField(
        mapFieldInfo(Proto3MessageLiteWithMaps.class, "field_map_int64_int32_76", 76));
    builder.withField(
        mapFieldInfo(Proto3MessageLiteWithMaps.class, "field_map_int64_int64_77", 77));
    builder.withField(
        mapFieldInfo(Proto3MessageLiteWithMaps.class, "field_map_int64_message_78", 78));
    builder.withField(
        mapFieldInfo(Proto3MessageLiteWithMaps.class, "field_map_int64_sfixed32_79", 79));
    builder.withField(
        mapFieldInfo(Proto3MessageLiteWithMaps.class, "field_map_int64_sfixed64_80", 80));
    builder.withField(
        mapFieldInfo(Proto3MessageLiteWithMaps.class, "field_map_int64_sint32_81", 81));
    builder.withField(
        mapFieldInfo(Proto3MessageLiteWithMaps.class, "field_map_int64_sint64_82", 82));
    builder.withField(
        mapFieldInfo(Proto3MessageLiteWithMaps.class, "field_map_int64_string_83", 83));
    builder.withField(
        mapFieldInfo(Proto3MessageLiteWithMaps.class, "field_map_int64_uint32_84", 84));
    builder.withField(
        mapFieldInfo(Proto3MessageLiteWithMaps.class, "field_map_int64_uint64_85", 85));
    builder.withField(
        mapFieldInfo(Proto3MessageLiteWithMaps.class, "field_map_sfixed32_bool_86", 86));
    builder.withField(
        mapFieldInfo(Proto3MessageLiteWithMaps.class, "field_map_sfixed32_bytes_87", 87));
    builder.withField(
        mapFieldInfo(Proto3MessageLiteWithMaps.class, "field_map_sfixed32_double_88", 88));
    builder.withField(
        mapFieldInfo(Proto3MessageLiteWithMaps.class, "field_map_sfixed32_enum_89", 89));
    builder.withField(
        mapFieldInfo(Proto3MessageLiteWithMaps.class, "field_map_sfixed32_fixed32_90", 90));
    builder.withField(
        mapFieldInfo(Proto3MessageLiteWithMaps.class, "field_map_sfixed32_fixed64_91", 91));
    builder.withField(
        mapFieldInfo(Proto3MessageLiteWithMaps.class, "field_map_sfixed32_float_92", 92));
    builder.withField(
        mapFieldInfo(Proto3MessageLiteWithMaps.class, "field_map_sfixed32_int32_93", 93));
    builder.withField(
        mapFieldInfo(Proto3MessageLiteWithMaps.class, "field_map_sfixed32_int64_94", 94));
    builder.withField(
        mapFieldInfo(Proto3MessageLiteWithMaps.class, "field_map_sfixed32_message_95", 95));
    builder.withField(
        mapFieldInfo(Proto3MessageLiteWithMaps.class, "field_map_sfixed32_sfixed32_96", 96));
    builder.withField(
        mapFieldInfo(Proto3MessageLiteWithMaps.class, "field_map_sfixed32_sfixed64_97", 97));
    builder.withField(
        mapFieldInfo(Proto3MessageLiteWithMaps.class, "field_map_sfixed32_sint32_98", 98));
    builder.withField(
        mapFieldInfo(Proto3MessageLiteWithMaps.class, "field_map_sfixed32_sint64_99", 99));
    builder.withField(
        mapFieldInfo(Proto3MessageLiteWithMaps.class, "field_map_sfixed32_string_100", 100));
    builder.withField(
        mapFieldInfo(Proto3MessageLiteWithMaps.class, "field_map_sfixed32_uint32_101", 101));
    builder.withField(
        mapFieldInfo(Proto3MessageLiteWithMaps.class, "field_map_sfixed32_uint64_102", 102));
    builder.withField(
        mapFieldInfo(Proto3MessageLiteWithMaps.class, "field_map_sfixed64_bool_103", 103));
    builder.withField(
        mapFieldInfo(Proto3MessageLiteWithMaps.class, "field_map_sfixed64_bytes_104", 104));
    builder.withField(
        mapFieldInfo(Proto3MessageLiteWithMaps.class, "field_map_sfixed64_double_105", 105));
    builder.withField(
        mapFieldInfo(Proto3MessageLiteWithMaps.class, "field_map_sfixed64_enum_106", 106));
    builder.withField(
        mapFieldInfo(Proto3MessageLiteWithMaps.class, "field_map_sfixed64_fixed32_107", 107));
    builder.withField(
        mapFieldInfo(Proto3MessageLiteWithMaps.class, "field_map_sfixed64_fixed64_108", 108));
    builder.withField(
        mapFieldInfo(Proto3MessageLiteWithMaps.class, "field_map_sfixed64_float_109", 109));
    builder.withField(
        mapFieldInfo(Proto3MessageLiteWithMaps.class, "field_map_sfixed64_int32_110", 110));
    builder.withField(
        mapFieldInfo(Proto3MessageLiteWithMaps.class, "field_map_sfixed64_int64_111", 111));
    builder.withField(
        mapFieldInfo(Proto3MessageLiteWithMaps.class, "field_map_sfixed64_message_112", 112));
    builder.withField(
        mapFieldInfo(Proto3MessageLiteWithMaps.class, "field_map_sfixed64_sfixed32_113", 113));
    builder.withField(
        mapFieldInfo(Proto3MessageLiteWithMaps.class, "field_map_sfixed64_sfixed64_114", 114));
    builder.withField(
        mapFieldInfo(Proto3MessageLiteWithMaps.class, "field_map_sfixed64_sint32_115", 115));
    builder.withField(
        mapFieldInfo(Proto3MessageLiteWithMaps.class, "field_map_sfixed64_sint64_116", 116));
    builder.withField(
        mapFieldInfo(Proto3MessageLiteWithMaps.class, "field_map_sfixed64_string_117", 117));
    builder.withField(
        mapFieldInfo(Proto3MessageLiteWithMaps.class, "field_map_sfixed64_uint32_118", 118));
    builder.withField(
        mapFieldInfo(Proto3MessageLiteWithMaps.class, "field_map_sfixed64_uint64_119", 119));
    builder.withField(
        mapFieldInfo(Proto3MessageLiteWithMaps.class, "field_map_sint32_bool_120", 120));
    builder.withField(
        mapFieldInfo(Proto3MessageLiteWithMaps.class, "field_map_sint32_bytes_121", 121));
    builder.withField(
        mapFieldInfo(Proto3MessageLiteWithMaps.class, "field_map_sint32_double_122", 122));
    builder.withField(
        mapFieldInfo(Proto3MessageLiteWithMaps.class, "field_map_sint32_enum_123", 123));
    builder.withField(
        mapFieldInfo(Proto3MessageLiteWithMaps.class, "field_map_sint32_fixed32_124", 124));
    builder.withField(
        mapFieldInfo(Proto3MessageLiteWithMaps.class, "field_map_sint32_fixed64_125", 125));
    builder.withField(
        mapFieldInfo(Proto3MessageLiteWithMaps.class, "field_map_sint32_float_126", 126));
    builder.withField(
        mapFieldInfo(Proto3MessageLiteWithMaps.class, "field_map_sint32_int32_127", 127));
    builder.withField(
        mapFieldInfo(Proto3MessageLiteWithMaps.class, "field_map_sint32_int64_128", 128));
    builder.withField(
        mapFieldInfo(Proto3MessageLiteWithMaps.class, "field_map_sint32_message_129", 129));
    builder.withField(
        mapFieldInfo(Proto3MessageLiteWithMaps.class, "field_map_sint32_sfixed32_130", 130));
    builder.withField(
        mapFieldInfo(Proto3MessageLiteWithMaps.class, "field_map_sint32_sfixed64_131", 131));
    builder.withField(
        mapFieldInfo(Proto3MessageLiteWithMaps.class, "field_map_sint32_sint32_132", 132));
    builder.withField(
        mapFieldInfo(Proto3MessageLiteWithMaps.class, "field_map_sint32_sint64_133", 133));
    builder.withField(
        mapFieldInfo(Proto3MessageLiteWithMaps.class, "field_map_sint32_string_134", 134));
    builder.withField(
        mapFieldInfo(Proto3MessageLiteWithMaps.class, "field_map_sint32_uint32_135", 135));
    builder.withField(
        mapFieldInfo(Proto3MessageLiteWithMaps.class, "field_map_sint32_uint64_136", 136));
    builder.withField(
        mapFieldInfo(Proto3MessageLiteWithMaps.class, "field_map_sint64_bool_137", 137));
    builder.withField(
        mapFieldInfo(Proto3MessageLiteWithMaps.class, "field_map_sint64_bytes_138", 138));
    builder.withField(
        mapFieldInfo(Proto3MessageLiteWithMaps.class, "field_map_sint64_double_139", 139));
    builder.withField(
        mapFieldInfo(Proto3MessageLiteWithMaps.class, "field_map_sint64_enum_140", 140));
    builder.withField(
        mapFieldInfo(Proto3MessageLiteWithMaps.class, "field_map_sint64_fixed32_141", 141));
    builder.withField(
        mapFieldInfo(Proto3MessageLiteWithMaps.class, "field_map_sint64_fixed64_142", 142));
    builder.withField(
        mapFieldInfo(Proto3MessageLiteWithMaps.class, "field_map_sint64_float_143", 143));
    builder.withField(
        mapFieldInfo(Proto3MessageLiteWithMaps.class, "field_map_sint64_int32_144", 144));
    builder.withField(
        mapFieldInfo(Proto3MessageLiteWithMaps.class, "field_map_sint64_int64_145", 145));
    builder.withField(
        mapFieldInfo(Proto3MessageLiteWithMaps.class, "field_map_sint64_message_146", 146));
    builder.withField(
        mapFieldInfo(Proto3MessageLiteWithMaps.class, "field_map_sint64_sfixed32_147", 147));
    builder.withField(
        mapFieldInfo(Proto3MessageLiteWithMaps.class, "field_map_sint64_sfixed64_148", 148));
    builder.withField(
        mapFieldInfo(Proto3MessageLiteWithMaps.class, "field_map_sint64_sint32_149", 149));
    builder.withField(
        mapFieldInfo(Proto3MessageLiteWithMaps.class, "field_map_sint64_sint64_150", 150));
    builder.withField(
        mapFieldInfo(Proto3MessageLiteWithMaps.class, "field_map_sint64_string_151", 151));
    builder.withField(
        mapFieldInfo(Proto3MessageLiteWithMaps.class, "field_map_sint64_uint32_152", 152));
    builder.withField(
        mapFieldInfo(Proto3MessageLiteWithMaps.class, "field_map_sint64_uint64_153", 153));
    builder.withField(
        mapFieldInfo(Proto3MessageLiteWithMaps.class, "field_map_string_bool_154", 154));
    builder.withField(
        mapFieldInfo(Proto3MessageLiteWithMaps.class, "field_map_string_bytes_155", 155));
    builder.withField(
        mapFieldInfo(Proto3MessageLiteWithMaps.class, "field_map_string_double_156", 156));
    builder.withField(
        mapFieldInfo(Proto3MessageLiteWithMaps.class, "field_map_string_enum_157", 157));
    builder.withField(
        mapFieldInfo(Proto3MessageLiteWithMaps.class, "field_map_string_fixed32_158", 158));
    builder.withField(
        mapFieldInfo(Proto3MessageLiteWithMaps.class, "field_map_string_fixed64_159", 159));
    builder.withField(
        mapFieldInfo(Proto3MessageLiteWithMaps.class, "field_map_string_float_160", 160));
    builder.withField(
        mapFieldInfo(Proto3MessageLiteWithMaps.class, "field_map_string_int32_161", 161));
    builder.withField(
        mapFieldInfo(Proto3MessageLiteWithMaps.class, "field_map_string_int64_162", 162));
    builder.withField(
        mapFieldInfo(Proto3MessageLiteWithMaps.class, "field_map_string_message_163", 163));
    builder.withField(
        mapFieldInfo(Proto3MessageLiteWithMaps.class, "field_map_string_sfixed32_164", 164));
    builder.withField(
        mapFieldInfo(Proto3MessageLiteWithMaps.class, "field_map_string_sfixed64_165", 165));
    builder.withField(
        mapFieldInfo(Proto3MessageLiteWithMaps.class, "field_map_string_sint32_166", 166));
    builder.withField(
        mapFieldInfo(Proto3MessageLiteWithMaps.class, "field_map_string_sint64_167", 167));
    builder.withField(
        mapFieldInfo(Proto3MessageLiteWithMaps.class, "field_map_string_string_168", 168));
    builder.withField(
        mapFieldInfo(Proto3MessageLiteWithMaps.class, "field_map_string_uint32_169", 169));
    builder.withField(
        mapFieldInfo(Proto3MessageLiteWithMaps.class, "field_map_string_uint64_170", 170));
    builder.withField(
        mapFieldInfo(Proto3MessageLiteWithMaps.class, "field_map_uint32_bool_171", 171));
    builder.withField(
        mapFieldInfo(Proto3MessageLiteWithMaps.class, "field_map_uint32_bytes_172", 172));
    builder.withField(
        mapFieldInfo(Proto3MessageLiteWithMaps.class, "field_map_uint32_double_173", 173));
    builder.withField(
        mapFieldInfo(Proto3MessageLiteWithMaps.class, "field_map_uint32_enum_174", 174));
    builder.withField(
        mapFieldInfo(Proto3MessageLiteWithMaps.class, "field_map_uint32_fixed32_175", 175));
    builder.withField(
        mapFieldInfo(Proto3MessageLiteWithMaps.class, "field_map_uint32_fixed64_176", 176));
    builder.withField(
        mapFieldInfo(Proto3MessageLiteWithMaps.class, "field_map_uint32_float_177", 177));
    builder.withField(
        mapFieldInfo(Proto3MessageLiteWithMaps.class, "field_map_uint32_int32_178", 178));
    builder.withField(
        mapFieldInfo(Proto3MessageLiteWithMaps.class, "field_map_uint32_int64_179", 179));
    builder.withField(
        mapFieldInfo(Proto3MessageLiteWithMaps.class, "field_map_uint32_message_180", 180));
    builder.withField(
        mapFieldInfo(Proto3MessageLiteWithMaps.class, "field_map_uint32_sfixed32_181", 181));
    builder.withField(
        mapFieldInfo(Proto3MessageLiteWithMaps.class, "field_map_uint32_sfixed64_182", 182));
    builder.withField(
        mapFieldInfo(Proto3MessageLiteWithMaps.class, "field_map_uint32_sint32_183", 183));
    builder.withField(
        mapFieldInfo(Proto3MessageLiteWithMaps.class, "field_map_uint32_sint64_184", 184));
    builder.withField(
        mapFieldInfo(Proto3MessageLiteWithMaps.class, "field_map_uint32_string_185", 185));
    builder.withField(
        mapFieldInfo(Proto3MessageLiteWithMaps.class, "field_map_uint32_uint32_186", 186));
    builder.withField(
        mapFieldInfo(Proto3MessageLiteWithMaps.class, "field_map_uint32_uint64_187", 187));
    builder.withField(
        mapFieldInfo(Proto3MessageLiteWithMaps.class, "field_map_uint64_bool_188", 188));
    builder.withField(
        mapFieldInfo(Proto3MessageLiteWithMaps.class, "field_map_uint64_bytes_189", 189));
    builder.withField(
        mapFieldInfo(Proto3MessageLiteWithMaps.class, "field_map_uint64_double_190", 190));
    builder.withField(
        mapFieldInfo(Proto3MessageLiteWithMaps.class, "field_map_uint64_enum_191", 191));
    builder.withField(
        mapFieldInfo(Proto3MessageLiteWithMaps.class, "field_map_uint64_fixed32_192", 192));
    builder.withField(
        mapFieldInfo(Proto3MessageLiteWithMaps.class, "field_map_uint64_fixed64_193", 193));
    builder.withField(
        mapFieldInfo(Proto3MessageLiteWithMaps.class, "field_map_uint64_float_194", 194));
    builder.withField(
        mapFieldInfo(Proto3MessageLiteWithMaps.class, "field_map_uint64_int32_195", 195));
    builder.withField(
        mapFieldInfo(Proto3MessageLiteWithMaps.class, "field_map_uint64_int64_196", 196));
    builder.withField(
        mapFieldInfo(Proto3MessageLiteWithMaps.class, "field_map_uint64_message_197", 197));
    builder.withField(
        mapFieldInfo(Proto3MessageLiteWithMaps.class, "field_map_uint64_sfixed32_198", 198));
    builder.withField(
        mapFieldInfo(Proto3MessageLiteWithMaps.class, "field_map_uint64_sfixed64_199", 199));
    builder.withField(
        mapFieldInfo(Proto3MessageLiteWithMaps.class, "field_map_uint64_sint32_200", 200));
    builder.withField(
        mapFieldInfo(Proto3MessageLiteWithMaps.class, "field_map_uint64_sint64_201", 201));
    builder.withField(
        mapFieldInfo(Proto3MessageLiteWithMaps.class, "field_map_uint64_string_202", 202));
    builder.withField(
        mapFieldInfo(Proto3MessageLiteWithMaps.class, "field_map_uint64_uint32_203", 203));
    builder.withField(
        mapFieldInfo(Proto3MessageLiteWithMaps.class, "field_map_uint64_uint64_204", 204));

    return builder.build();
  }

  private static Field field(Class<?> clazz, String name) {
    try {
      return clazz.getDeclaredField(name);
    } catch (NoSuchFieldException | SecurityException e) {
      throw new RuntimeException(e);
    }
  }

  private static FieldInfo mapFieldInfo(Class<?> clazz, String fieldName, int fieldNumber) {
    try {
      return forMapField(
          field(clazz, SchemaUtil.toCamelCase(fieldName, false) + "_"),
          fieldNumber,
          SchemaUtil.getMapDefaultEntry(clazz, fieldName),
          null);
    } catch (Throwable t) {
      throw new RuntimeException(t);
    }
  }
}
