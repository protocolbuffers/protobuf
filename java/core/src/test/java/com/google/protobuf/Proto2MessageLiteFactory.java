// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

package com.google.protobuf;

import com.google.protobuf.testing.Proto2TestingLite.Proto2MessageLite;
import com.google.protobuf.testing.Proto2TestingLite.Proto2MessageLiteWithMaps;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

/** Creates instances of {@link Proto2MessageLite} based on the tree configuration. */
public final class Proto2MessageLiteFactory
    implements ExperimentalMessageFactory<Proto2MessageLite> {
  private final int numRepeatedFields;
  private final int branchingFactor;
  private final Proto2MessageLiteFactory nextLevel;
  private final ExperimentalTestDataProvider data;

  public Proto2MessageLiteFactory(
      int numRepeatedFields, int stringLength, int branchingFactor, int treeDepth) {
    this(
        new ExperimentalTestDataProvider(stringLength),
        numRepeatedFields,
        branchingFactor,
        treeDepth);
  }

  private Proto2MessageLiteFactory(
      ExperimentalTestDataProvider data,
      int numRepeatedFields,
      int branchingFactor,
      int treeDepth) {
    this.numRepeatedFields = numRepeatedFields;
    this.branchingFactor = branchingFactor;
    this.data = data;
    if (treeDepth > 0) {
      nextLevel =
          new Proto2MessageLiteFactory(data, numRepeatedFields, branchingFactor, treeDepth - 1);
    } else {
      nextLevel = null;
    }
  }

  @Override
  public ExperimentalTestDataProvider dataProvider() {
    return data;
  }

  @Override
  public Proto2MessageLite newMessage() {
    Proto2MessageLite.Builder builder = Proto2MessageLite.newBuilder();
    builder.setFieldDouble1(data.getDouble());
    builder.setFieldFloat2(data.getFloat());
    builder.setFieldInt643(data.getLong());
    builder.setFieldUint644(data.getLong());
    builder.setFieldInt325(data.getInt());
    builder.setFieldFixed646(data.getLong());
    builder.setFieldFixed327(data.getInt());
    builder.setFieldBool8(data.getBool());
    builder.setFieldString9(data.getString());
    // We don't populate the message field. Instead we apply the branching factor to the
    // repeated message field below.
    builder.setFieldBytes11(data.getBytes());
    builder.setFieldUint3212(data.getInt());
    builder.setFieldEnum13(Proto2MessageLite.TestEnum.forNumber(data.getEnum()));
    builder.setFieldSfixed3214(data.getInt());
    builder.setFieldSfixed6415(data.getLong());
    builder.setFieldSint3216(data.getInt());
    builder.setFieldSint6417(data.getLong());

    for (int i = 0; i < numRepeatedFields; ++i) {
      builder.addFieldDoubleList18(data.getDouble());
      builder.addFieldFloatList19(data.getFloat());
      builder.addFieldInt64List20(data.getLong());
      builder.addFieldUint64List21(data.getLong());
      builder.addFieldInt32List22(data.getInt());
      builder.addFieldFixed64List23(data.getLong());
      builder.addFieldFixed32List24(data.getInt());
      builder.addFieldBoolList25(data.getBool());
      builder.addFieldStringList26(data.getString());
      // Repeated message field is controlled by the branching factor below.
      builder.addFieldBytesList28(data.getBytes());
      builder.addFieldUint32List29(data.getInt());
      builder.addFieldEnumList30(Proto2MessageLite.TestEnum.forNumber(data.getEnum()));
      builder.addFieldSfixed32List31(data.getInt());
      builder.addFieldSfixed64List32(data.getLong());
      builder.addFieldSint32List33(data.getInt());
      builder.addFieldSint64List34(data.getLong());

      builder.addFieldDoubleListPacked35(data.getDouble());
      builder.addFieldFloatListPacked36(data.getFloat());
      builder.addFieldInt64ListPacked37(data.getLong());
      builder.addFieldUint64ListPacked38(data.getLong());
      builder.addFieldInt32ListPacked39(data.getInt());
      builder.addFieldFixed64ListPacked40(data.getLong());
      builder.addFieldFixed32ListPacked41(data.getInt());
      builder.addFieldBoolListPacked42(data.getBool());
      builder.addFieldUint32ListPacked43(data.getInt());
      builder.addFieldEnumListPacked44(Proto2MessageLite.TestEnum.forNumber(data.getEnum()));
      builder.addFieldSfixed32ListPacked45(data.getInt());
      builder.addFieldSfixed64ListPacked46(data.getLong());
      builder.addFieldSint32ListPacked47(data.getInt());
      builder.addFieldSint64ListPacked48(data.getLong());
    }

    builder.setFieldGroup49(
        Proto2MessageLite.FieldGroup49.newBuilder().setFieldInt3250(data.getInt()));

    for (int i = 0; i < branchingFactor; ++i) {
      builder.addFieldGroupList51(
          Proto2MessageLite.FieldGroupList51.newBuilder().setFieldInt3252(data.getInt()));
    }

    // Set all required fields.
    populateRequiredFields(builder, INCLUDE_ALL_REQUIRED_FIELDS);

    // Handle the branching factor.
    if (nextLevel != null) {
      for (int i = 0; i < branchingFactor; ++i) {
        builder.addFieldMessageList27(nextLevel.newMessage());
      }
    }

    return builder.build();
  }

  private interface MapValueProvider<T> {
    public T getValue();
  }

  private final MapValueProvider<Integer> integerProvider =
      new MapValueProvider<Integer>() {
        @Override
        public Integer getValue() {
          return data.getInt();
        }
      };
  private final MapValueProvider<Long> longProvider =
      new MapValueProvider<Long>() {
        @Override
        public Long getValue() {
          return data.getLong();
        }
      };
  private final MapValueProvider<String> stringProvider =
      new MapValueProvider<String>() {
        @Override
        public String getValue() {
          return data.getString();
        }
      };
  private final MapValueProvider<ByteString> bytesProvider =
      new MapValueProvider<ByteString>() {
        @Override
        public ByteString getValue() {
          return data.getBytes();
        }
      };
  private final MapValueProvider<Boolean> booleanProvider =
      new MapValueProvider<Boolean>() {
        @Override
        public Boolean getValue() {
          return data.getBool();
        }
      };
  private final MapValueProvider<Float> floatProvider =
      new MapValueProvider<Float>() {
        @Override
        public Float getValue() {
          return data.getFloat();
        }
      };
  private final MapValueProvider<Double> doubleProvider =
      new MapValueProvider<Double>() {
        @Override
        public Double getValue() {
          return data.getDouble();
        }
      };
  private final MapValueProvider<Proto2MessageLite> messageProvider =
      new MapValueProvider<Proto2MessageLite>() {
        @Override
        public Proto2MessageLite getValue() {
          return newMessage();
        }
      };
  private final MapValueProvider<Proto2MessageLite.TestEnum> enumProvider =
      new MapValueProvider<Proto2MessageLite.TestEnum>() {
        @Override
        public Proto2MessageLite.TestEnum getValue() {
          return Proto2MessageLite.TestEnum.forNumber(data.getEnum());
        }
      };

  private <V> Map<Integer, V> populateIntegerMap(MapValueProvider<V> provider) {
    Map<Integer, V> map = new HashMap<>();
    for (int i = 0; i < numRepeatedFields; ++i) {
      map.put(data.getInt(), provider.getValue());
    }
    return map;
  }

  private <V> Map<Long, V> populateLongMap(MapValueProvider<V> provider) {
    Map<Long, V> map = new HashMap<>();
    for (int i = 0; i < numRepeatedFields; ++i) {
      map.put(data.getLong(), provider.getValue());
    }
    return map;
  }

  private <V> Map<String, V> populateStringMap(MapValueProvider<V> provider) {
    Map<String, V> map = new HashMap<>();
    for (int i = 0; i < numRepeatedFields; ++i) {
      map.put(data.getString(), provider.getValue());
    }
    return map;
  }

  private <V> Map<Boolean, V> populateBooleanMap(MapValueProvider<V> provider) {
    Map<Boolean, V> map = new HashMap<>();
    map.put(false, provider.getValue());
    map.put(true, provider.getValue());
    return map;
  }

  public Proto2MessageLiteWithMaps newMessageWithMaps() {
    Proto2MessageLiteWithMaps.Builder builder = Proto2MessageLiteWithMaps.newBuilder();

    builder.putAllFieldMapBoolBool1(populateBooleanMap(booleanProvider));
    builder.putAllFieldMapBoolBytes2(populateBooleanMap(bytesProvider));
    builder.putAllFieldMapBoolDouble3(populateBooleanMap(doubleProvider));
    builder.putAllFieldMapBoolEnum4(populateBooleanMap(enumProvider));
    builder.putAllFieldMapBoolFixed325(populateBooleanMap(integerProvider));
    builder.putAllFieldMapBoolFixed646(populateBooleanMap(longProvider));
    builder.putAllFieldMapBoolFloat7(populateBooleanMap(floatProvider));
    builder.putAllFieldMapBoolInt328(populateBooleanMap(integerProvider));
    builder.putAllFieldMapBoolInt649(populateBooleanMap(longProvider));
    builder.putAllFieldMapBoolMessage10(populateBooleanMap(messageProvider));
    builder.putAllFieldMapBoolSfixed3211(populateBooleanMap(integerProvider));
    builder.putAllFieldMapBoolSfixed6412(populateBooleanMap(longProvider));
    builder.putAllFieldMapBoolSint3213(populateBooleanMap(integerProvider));
    builder.putAllFieldMapBoolSint6414(populateBooleanMap(longProvider));
    builder.putAllFieldMapBoolString15(populateBooleanMap(stringProvider));
    builder.putAllFieldMapBoolUint3216(populateBooleanMap(integerProvider));
    builder.putAllFieldMapBoolUint6417(populateBooleanMap(longProvider));
    builder.putAllFieldMapFixed32Bool18(populateIntegerMap(booleanProvider));
    builder.putAllFieldMapFixed32Bytes19(populateIntegerMap(bytesProvider));
    builder.putAllFieldMapFixed32Double20(populateIntegerMap(doubleProvider));
    builder.putAllFieldMapFixed32Enum21(populateIntegerMap(enumProvider));
    builder.putAllFieldMapFixed32Fixed3222(populateIntegerMap(integerProvider));
    builder.putAllFieldMapFixed32Fixed6423(populateIntegerMap(longProvider));
    builder.putAllFieldMapFixed32Float24(populateIntegerMap(floatProvider));
    builder.putAllFieldMapFixed32Int3225(populateIntegerMap(integerProvider));
    builder.putAllFieldMapFixed32Int6426(populateIntegerMap(longProvider));
    builder.putAllFieldMapFixed32Message27(populateIntegerMap(messageProvider));
    builder.putAllFieldMapFixed32Sfixed3228(populateIntegerMap(integerProvider));
    builder.putAllFieldMapFixed32Sfixed6429(populateIntegerMap(longProvider));
    builder.putAllFieldMapFixed32Sint3230(populateIntegerMap(integerProvider));
    builder.putAllFieldMapFixed32Sint6431(populateIntegerMap(longProvider));
    builder.putAllFieldMapFixed32String32(populateIntegerMap(stringProvider));
    builder.putAllFieldMapFixed32Uint3233(populateIntegerMap(integerProvider));
    builder.putAllFieldMapFixed32Uint6434(populateIntegerMap(longProvider));
    builder.putAllFieldMapFixed64Bool35(populateLongMap(booleanProvider));
    builder.putAllFieldMapFixed64Bytes36(populateLongMap(bytesProvider));
    builder.putAllFieldMapFixed64Double37(populateLongMap(doubleProvider));
    builder.putAllFieldMapFixed64Enum38(populateLongMap(enumProvider));
    builder.putAllFieldMapFixed64Fixed3239(populateLongMap(integerProvider));
    builder.putAllFieldMapFixed64Fixed6440(populateLongMap(longProvider));
    builder.putAllFieldMapFixed64Float41(populateLongMap(floatProvider));
    builder.putAllFieldMapFixed64Int3242(populateLongMap(integerProvider));
    builder.putAllFieldMapFixed64Int6443(populateLongMap(longProvider));
    builder.putAllFieldMapFixed64Message44(populateLongMap(messageProvider));
    builder.putAllFieldMapFixed64Sfixed3245(populateLongMap(integerProvider));
    builder.putAllFieldMapFixed64Sfixed6446(populateLongMap(longProvider));
    builder.putAllFieldMapFixed64Sint3247(populateLongMap(integerProvider));
    builder.putAllFieldMapFixed64Sint6448(populateLongMap(longProvider));
    builder.putAllFieldMapFixed64String49(populateLongMap(stringProvider));
    builder.putAllFieldMapFixed64Uint3250(populateLongMap(integerProvider));
    builder.putAllFieldMapFixed64Uint6451(populateLongMap(longProvider));
    builder.putAllFieldMapInt32Bool52(populateIntegerMap(booleanProvider));
    builder.putAllFieldMapInt32Bytes53(populateIntegerMap(bytesProvider));
    builder.putAllFieldMapInt32Double54(populateIntegerMap(doubleProvider));
    builder.putAllFieldMapInt32Enum55(populateIntegerMap(enumProvider));
    builder.putAllFieldMapInt32Fixed3256(populateIntegerMap(integerProvider));
    builder.putAllFieldMapInt32Fixed6457(populateIntegerMap(longProvider));
    builder.putAllFieldMapInt32Float58(populateIntegerMap(floatProvider));
    builder.putAllFieldMapInt32Int3259(populateIntegerMap(integerProvider));
    builder.putAllFieldMapInt32Int6460(populateIntegerMap(longProvider));
    builder.putAllFieldMapInt32Message61(populateIntegerMap(messageProvider));
    builder.putAllFieldMapInt32Sfixed3262(populateIntegerMap(integerProvider));
    builder.putAllFieldMapInt32Sfixed6463(populateIntegerMap(longProvider));
    builder.putAllFieldMapInt32Sint3264(populateIntegerMap(integerProvider));
    builder.putAllFieldMapInt32Sint6465(populateIntegerMap(longProvider));
    builder.putAllFieldMapInt32String66(populateIntegerMap(stringProvider));
    builder.putAllFieldMapInt32Uint3267(populateIntegerMap(integerProvider));
    builder.putAllFieldMapInt32Uint6468(populateIntegerMap(longProvider));
    builder.putAllFieldMapInt64Bool69(populateLongMap(booleanProvider));
    builder.putAllFieldMapInt64Bytes70(populateLongMap(bytesProvider));
    builder.putAllFieldMapInt64Double71(populateLongMap(doubleProvider));
    builder.putAllFieldMapInt64Enum72(populateLongMap(enumProvider));
    builder.putAllFieldMapInt64Fixed3273(populateLongMap(integerProvider));
    builder.putAllFieldMapInt64Fixed6474(populateLongMap(longProvider));
    builder.putAllFieldMapInt64Float75(populateLongMap(floatProvider));
    builder.putAllFieldMapInt64Int3276(populateLongMap(integerProvider));
    builder.putAllFieldMapInt64Int6477(populateLongMap(longProvider));
    builder.putAllFieldMapInt64Message78(populateLongMap(messageProvider));
    builder.putAllFieldMapInt64Sfixed3279(populateLongMap(integerProvider));
    builder.putAllFieldMapInt64Sfixed6480(populateLongMap(longProvider));
    builder.putAllFieldMapInt64Sint3281(populateLongMap(integerProvider));
    builder.putAllFieldMapInt64Sint6482(populateLongMap(longProvider));
    builder.putAllFieldMapInt64String83(populateLongMap(stringProvider));
    builder.putAllFieldMapInt64Uint3284(populateLongMap(integerProvider));
    builder.putAllFieldMapInt64Uint6485(populateLongMap(longProvider));
    builder.putAllFieldMapSfixed32Bool86(populateIntegerMap(booleanProvider));
    builder.putAllFieldMapSfixed32Bytes87(populateIntegerMap(bytesProvider));
    builder.putAllFieldMapSfixed32Double88(populateIntegerMap(doubleProvider));
    builder.putAllFieldMapSfixed32Enum89(populateIntegerMap(enumProvider));
    builder.putAllFieldMapSfixed32Fixed3290(populateIntegerMap(integerProvider));
    builder.putAllFieldMapSfixed32Fixed6491(populateIntegerMap(longProvider));
    builder.putAllFieldMapSfixed32Float92(populateIntegerMap(floatProvider));
    builder.putAllFieldMapSfixed32Int3293(populateIntegerMap(integerProvider));
    builder.putAllFieldMapSfixed32Int6494(populateIntegerMap(longProvider));
    builder.putAllFieldMapSfixed32Message95(populateIntegerMap(messageProvider));
    builder.putAllFieldMapSfixed32Sfixed3296(populateIntegerMap(integerProvider));
    builder.putAllFieldMapSfixed32Sfixed6497(populateIntegerMap(longProvider));
    builder.putAllFieldMapSfixed32Sint3298(populateIntegerMap(integerProvider));
    builder.putAllFieldMapSfixed32Sint6499(populateIntegerMap(longProvider));
    builder.putAllFieldMapSfixed32String100(populateIntegerMap(stringProvider));
    builder.putAllFieldMapSfixed32Uint32101(populateIntegerMap(integerProvider));
    builder.putAllFieldMapSfixed32Uint64102(populateIntegerMap(longProvider));
    builder.putAllFieldMapSfixed64Bool103(populateLongMap(booleanProvider));
    builder.putAllFieldMapSfixed64Bytes104(populateLongMap(bytesProvider));
    builder.putAllFieldMapSfixed64Double105(populateLongMap(doubleProvider));
    builder.putAllFieldMapSfixed64Enum106(populateLongMap(enumProvider));
    builder.putAllFieldMapSfixed64Fixed32107(populateLongMap(integerProvider));
    builder.putAllFieldMapSfixed64Fixed64108(populateLongMap(longProvider));
    builder.putAllFieldMapSfixed64Float109(populateLongMap(floatProvider));
    builder.putAllFieldMapSfixed64Int32110(populateLongMap(integerProvider));
    builder.putAllFieldMapSfixed64Int64111(populateLongMap(longProvider));
    builder.putAllFieldMapSfixed64Message112(populateLongMap(messageProvider));
    builder.putAllFieldMapSfixed64Sfixed32113(populateLongMap(integerProvider));
    builder.putAllFieldMapSfixed64Sfixed64114(populateLongMap(longProvider));
    builder.putAllFieldMapSfixed64Sint32115(populateLongMap(integerProvider));
    builder.putAllFieldMapSfixed64Sint64116(populateLongMap(longProvider));
    builder.putAllFieldMapSfixed64String117(populateLongMap(stringProvider));
    builder.putAllFieldMapSfixed64Uint32118(populateLongMap(integerProvider));
    builder.putAllFieldMapSfixed64Uint64119(populateLongMap(longProvider));
    builder.putAllFieldMapSint32Bool120(populateIntegerMap(booleanProvider));
    builder.putAllFieldMapSint32Bytes121(populateIntegerMap(bytesProvider));
    builder.putAllFieldMapSint32Double122(populateIntegerMap(doubleProvider));
    builder.putAllFieldMapSint32Enum123(populateIntegerMap(enumProvider));
    builder.putAllFieldMapSint32Fixed32124(populateIntegerMap(integerProvider));
    builder.putAllFieldMapSint32Fixed64125(populateIntegerMap(longProvider));
    builder.putAllFieldMapSint32Float126(populateIntegerMap(floatProvider));
    builder.putAllFieldMapSint32Int32127(populateIntegerMap(integerProvider));
    builder.putAllFieldMapSint32Int64128(populateIntegerMap(longProvider));
    builder.putAllFieldMapSint32Message129(populateIntegerMap(messageProvider));
    builder.putAllFieldMapSint32Sfixed32130(populateIntegerMap(integerProvider));
    builder.putAllFieldMapSint32Sfixed64131(populateIntegerMap(longProvider));
    builder.putAllFieldMapSint32Sint32132(populateIntegerMap(integerProvider));
    builder.putAllFieldMapSint32Sint64133(populateIntegerMap(longProvider));
    builder.putAllFieldMapSint32String134(populateIntegerMap(stringProvider));
    builder.putAllFieldMapSint32Uint32135(populateIntegerMap(integerProvider));
    builder.putAllFieldMapSint32Uint64136(populateIntegerMap(longProvider));
    builder.putAllFieldMapSint64Bool137(populateLongMap(booleanProvider));
    builder.putAllFieldMapSint64Bytes138(populateLongMap(bytesProvider));
    builder.putAllFieldMapSint64Double139(populateLongMap(doubleProvider));
    builder.putAllFieldMapSint64Enum140(populateLongMap(enumProvider));
    builder.putAllFieldMapSint64Fixed32141(populateLongMap(integerProvider));
    builder.putAllFieldMapSint64Fixed64142(populateLongMap(longProvider));
    builder.putAllFieldMapSint64Float143(populateLongMap(floatProvider));
    builder.putAllFieldMapSint64Int32144(populateLongMap(integerProvider));
    builder.putAllFieldMapSint64Int64145(populateLongMap(longProvider));
    builder.putAllFieldMapSint64Message146(populateLongMap(messageProvider));
    builder.putAllFieldMapSint64Sfixed32147(populateLongMap(integerProvider));
    builder.putAllFieldMapSint64Sfixed64148(populateLongMap(longProvider));
    builder.putAllFieldMapSint64Sint32149(populateLongMap(integerProvider));
    builder.putAllFieldMapSint64Sint64150(populateLongMap(longProvider));
    builder.putAllFieldMapSint64String151(populateLongMap(stringProvider));
    builder.putAllFieldMapSint64Uint32152(populateLongMap(integerProvider));
    builder.putAllFieldMapSint64Uint64153(populateLongMap(longProvider));
    builder.putAllFieldMapStringBool154(populateStringMap(booleanProvider));
    builder.putAllFieldMapStringBytes155(populateStringMap(bytesProvider));
    builder.putAllFieldMapStringDouble156(populateStringMap(doubleProvider));
    builder.putAllFieldMapStringEnum157(populateStringMap(enumProvider));
    builder.putAllFieldMapStringFixed32158(populateStringMap(integerProvider));
    builder.putAllFieldMapStringFixed64159(populateStringMap(longProvider));
    builder.putAllFieldMapStringFloat160(populateStringMap(floatProvider));
    builder.putAllFieldMapStringInt32161(populateStringMap(integerProvider));
    builder.putAllFieldMapStringInt64162(populateStringMap(longProvider));
    builder.putAllFieldMapStringMessage163(populateStringMap(messageProvider));
    builder.putAllFieldMapStringSfixed32164(populateStringMap(integerProvider));
    builder.putAllFieldMapStringSfixed64165(populateStringMap(longProvider));
    builder.putAllFieldMapStringSint32166(populateStringMap(integerProvider));
    builder.putAllFieldMapStringSint64167(populateStringMap(longProvider));
    builder.putAllFieldMapStringString168(populateStringMap(stringProvider));
    builder.putAllFieldMapStringUint32169(populateStringMap(integerProvider));
    builder.putAllFieldMapStringUint64170(populateStringMap(longProvider));
    builder.putAllFieldMapUint32Bool171(populateIntegerMap(booleanProvider));
    builder.putAllFieldMapUint32Bytes172(populateIntegerMap(bytesProvider));
    builder.putAllFieldMapUint32Double173(populateIntegerMap(doubleProvider));
    builder.putAllFieldMapUint32Enum174(populateIntegerMap(enumProvider));
    builder.putAllFieldMapUint32Fixed32175(populateIntegerMap(integerProvider));
    builder.putAllFieldMapUint32Fixed64176(populateIntegerMap(longProvider));
    builder.putAllFieldMapUint32Float177(populateIntegerMap(floatProvider));
    builder.putAllFieldMapUint32Int32178(populateIntegerMap(integerProvider));
    builder.putAllFieldMapUint32Int64179(populateIntegerMap(longProvider));
    builder.putAllFieldMapUint32Message180(populateIntegerMap(messageProvider));
    builder.putAllFieldMapUint32Sfixed32181(populateIntegerMap(integerProvider));
    builder.putAllFieldMapUint32Sfixed64182(populateIntegerMap(longProvider));
    builder.putAllFieldMapUint32Sint32183(populateIntegerMap(integerProvider));
    builder.putAllFieldMapUint32Sint64184(populateIntegerMap(longProvider));
    builder.putAllFieldMapUint32String185(populateIntegerMap(stringProvider));
    builder.putAllFieldMapUint32Uint32186(populateIntegerMap(integerProvider));
    builder.putAllFieldMapUint32Uint64187(populateIntegerMap(longProvider));
    builder.putAllFieldMapUint64Bool188(populateLongMap(booleanProvider));
    builder.putAllFieldMapUint64Bytes189(populateLongMap(bytesProvider));
    builder.putAllFieldMapUint64Double190(populateLongMap(doubleProvider));
    builder.putAllFieldMapUint64Enum191(populateLongMap(enumProvider));
    builder.putAllFieldMapUint64Fixed32192(populateLongMap(integerProvider));
    builder.putAllFieldMapUint64Fixed64193(populateLongMap(longProvider));
    builder.putAllFieldMapUint64Float194(populateLongMap(floatProvider));
    builder.putAllFieldMapUint64Int32195(populateLongMap(integerProvider));
    builder.putAllFieldMapUint64Int64196(populateLongMap(longProvider));
    builder.putAllFieldMapUint64Message197(populateLongMap(messageProvider));
    builder.putAllFieldMapUint64Sfixed32198(populateLongMap(integerProvider));
    builder.putAllFieldMapUint64Sfixed64199(populateLongMap(longProvider));
    builder.putAllFieldMapUint64Sint32200(populateLongMap(integerProvider));
    builder.putAllFieldMapUint64Sint64201(populateLongMap(longProvider));
    builder.putAllFieldMapUint64String202(populateLongMap(stringProvider));
    builder.putAllFieldMapUint64Uint32203(populateLongMap(integerProvider));
    builder.putAllFieldMapUint64Uint64204(populateLongMap(longProvider));

    return builder.build();
  }

  public List<Proto2MessageLite> newMessagesMissingRequiredFields() {
    List<Proto2MessageLite> results = new ArrayList<>();
    for (int i = 71; i <= 88; ++i) {
      Proto2MessageLite.Builder builder = Proto2MessageLite.newBuilder();
      populateRequiredFields(builder, i);
      results.add(builder.buildPartial());
    }
    {
      // A nested optional message field is missing required fields.
      Proto2MessageLite.Builder builder = Proto2MessageLite.newBuilder();
      populateRequiredFields(builder, INCLUDE_ALL_REQUIRED_FIELDS);
      builder.setFieldMessage10(Proto2MessageLite.getDefaultInstance());
      results.add(builder.buildPartial());
    }
    {
      // A nested repeated message field is missing required fields.
      Proto2MessageLite.Builder builder = Proto2MessageLite.newBuilder();
      populateRequiredFields(builder, INCLUDE_ALL_REQUIRED_FIELDS);
      builder.addFieldMessageList27(Proto2MessageLite.getDefaultInstance());
      results.add(builder.buildPartial());
    }
    {
      // A nested oneof message field is missing required fields.
      Proto2MessageLite.Builder builder = Proto2MessageLite.newBuilder();
      populateRequiredFields(builder, INCLUDE_ALL_REQUIRED_FIELDS);
      builder.setFieldMessage62(Proto2MessageLite.getDefaultInstance());
      results.add(builder.buildPartial());
    }
    return results;
  }

  // 0 is not a valid field number so we use it to mean no fields are excluded.
  private static final int INCLUDE_ALL_REQUIRED_FIELDS = 0;

  private void populateRequiredFields(Proto2MessageLite.Builder builder, int excludedFieldNumber) {
    if (excludedFieldNumber != 71) {
      builder.setFieldRequiredDouble71(data.getDouble());
    }
    if (excludedFieldNumber != 72) {
      builder.setFieldRequiredFloat72(data.getFloat());
    }
    if (excludedFieldNumber != 73) {
      builder.setFieldRequiredInt6473(data.getLong());
    }
    if (excludedFieldNumber != 74) {
      builder.setFieldRequiredUint6474(data.getLong());
    }
    if (excludedFieldNumber != 75) {
      builder.setFieldRequiredInt3275(data.getInt());
    }
    if (excludedFieldNumber != 76) {
      builder.setFieldRequiredFixed6476(data.getLong());
    }
    if (excludedFieldNumber != 77) {
      builder.setFieldRequiredFixed3277(data.getInt());
    }
    if (excludedFieldNumber != 78) {
      builder.setFieldRequiredBool78(data.getBool());
    }
    if (excludedFieldNumber != 79) {
      builder.setFieldRequiredString79(data.getString());
    }
    if (excludedFieldNumber != 80) {
      builder.setFieldRequiredMessage80(
          Proto2MessageLite.RequiredNestedMessage.newBuilder().setValue(data.getInt()));
    }
    if (excludedFieldNumber != 81) {
      builder.setFieldRequiredBytes81(data.getBytes());
    }
    if (excludedFieldNumber != 82) {
      builder.setFieldRequiredUint3282(data.getInt());
    }
    if (excludedFieldNumber != 83) {
      builder.setFieldRequiredEnum83(Proto2MessageLite.TestEnum.forNumber(data.getEnum()));
    }
    if (excludedFieldNumber != 84) {
      builder.setFieldRequiredSfixed3284(data.getInt());
    }
    if (excludedFieldNumber != 85) {
      builder.setFieldRequiredSfixed6485(data.getLong());
    }
    if (excludedFieldNumber != 86) {
      builder.setFieldRequiredSint3286(data.getInt());
    }
    if (excludedFieldNumber != 87) {
      builder.setFieldRequiredSint6487(data.getLong());
    }
    if (excludedFieldNumber != 88) {
      builder.setFieldRequiredGroup88(
          Proto2MessageLite.FieldRequiredGroup88.newBuilder().setFieldInt3289(data.getInt()));
    }
  }
}
