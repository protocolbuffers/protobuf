// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

package com.google.protobuf;

import static com.google.common.truth.Truth.assertThat;

import com.google.protobuf.testing.Proto2TestingLite.Proto2MessageLite;
import java.util.ArrayList;
import java.util.List;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

/**
 * Runs the {@link Proto2MessageLite} schema tests against a schema whose {@code objects[]}
 * identifies every sub-message type by its default instance instead of its class, the way a code
 * shrinker that merges message classes rewrites the generated {@code BUILD_MESSAGE_INFO}: it adds
 * the default instance after a singular message or group field's name, and replaces the class
 * literal of a repeated or oneof message or group field.
 */
@RunWith(JUnit4.class)
public class Proto2LiteDefaultInstanceObjectsSchemaTest extends AbstractProto2LiteSchemaTest {

  // Copies of the private MessageSchema constants for the extra bits of a field's type.
  private static final int LEGACY_ENUM_IS_CLOSED_BIT = 0x800;
  private static final int HAS_HAS_BIT = 0x1000;
  private static final int FIRST_ONEOF_TYPE = 51;

  /** How many entries of each kind {@link #identifySubmessagesByDefaultInstance} rewrote. */
  private static final class RewriteCounts {
    int singular;
    int repeated;
    int oneof;
  }

  @Override
  protected Schema<Proto2MessageLite> schema() {
    return newSchema(identifySubmessagesByDefaultInstance(generatedMessageInfo(), null));
  }

  @Test
  public void rewriteCoversEveryKindOfMessageField() throws Exception {
    RewriteCounts counts = new RewriteCounts();
    RawMessageInfo rewritten = identifySubmessagesByDefaultInstance(generatedMessageInfo(), counts);

    // field_message_10, fieldgroup49, field_required_message_80, fieldrequiredgroup88.
    assertThat(counts.singular).isEqualTo(4);
    // field_message_list_27, fieldgrouplist51.
    assertThat(counts.repeated).isEqualTo(2);
    // field_message_62, fieldgroup69.
    assertThat(counts.oneof).isEqualTo(2);
    assertThat(rewritten.getObjects())
        .hasLength(generatedMessageInfo().getObjects().length + counts.singular);
    for (Object object : rewritten.getObjects()) {
      assertThat(object).isNotInstanceOf(Class.class);
    }
  }

  @Test
  public void singularDefaultInstanceIsNotTakenForTheNextFieldName() throws Exception {
    // Every field after a singular message field must still find its own name; set the fields
    // right after each of them.
    Proto2MessageLite message =
        Proto2MessageLite.newBuilder()
            .setFieldMessage10(Proto2MessageLite.newBuilder().setFieldInt325(1).buildPartial())
            .setFieldBytes11(ByteString.copyFromUtf8("after message"))
            .setFieldGroup49(
                Proto2MessageLite.FieldGroup49.newBuilder().setFieldInt3250(2).buildPartial())
            .addFieldGroupList51(
                Proto2MessageLite.FieldGroupList51.newBuilder().setFieldInt3252(3).buildPartial())
            .buildPartial();
    roundtrip("singular message fields", message);
  }

  @Test
  public void defaultInstancesIdentifyTheSubmessageSchemas() throws Exception {
    Proto2MessageLite nested =
        Proto2MessageLite.newBuilder().setFieldInt325(7).setFieldString9("nested").buildPartial();
    Proto2MessageLite message =
        Proto2MessageLite.newBuilder()
            .setFieldMessage10(nested)
            .addFieldMessageList27(nested)
            .addFieldMessageList27(Proto2MessageLite.newBuilder().setFieldInt643(8).buildPartial())
            .setFieldMessage62(nested)
            .buildPartial();
    Schema<Proto2MessageLite> schema = schema();
    byte[] bytes = message.toByteArray();

    Proto2MessageLite parsed = schema.newInstance();
    schema.mergeFrom(
        parsed,
        CodedInputStreamReader.forCodedInput(CodedInputStream.newInstance(bytes)),
        ExtensionRegistryLite.getEmptyRegistry());
    schema.makeImmutable(parsed);

    assertThat(parsed).isEqualTo(message);
    assertThat(schema.equals(parsed, message)).isTrue();
    assertThat(schema.hashCode(parsed)).isEqualTo(schema.hashCode(message));
    assertThat(schema.getSerializedSize(parsed)).isEqualTo(bytes.length);
    assertThat(parsed.getFieldMessage10().getDefaultInstanceForType())
        .isSameInstanceAs(Proto2MessageLite.getDefaultInstance());
    assertThat(parsed.getFieldMessageList27(1).getDefaultInstanceForType())
        .isSameInstanceAs(Proto2MessageLite.getDefaultInstance());
    assertThat(parsed.getFieldMessage62().getDefaultInstanceForType())
        .isSameInstanceAs(Proto2MessageLite.getDefaultInstance());

    Proto2MessageLite merged = schema.newInstance();
    schema.mergeFrom(merged, parsed);
    schema.mergeFrom(merged, parsed);
    assertThat(merged.getFieldMessageList27Count()).isEqualTo(4);
    assertThat(merged.getFieldMessage10()).isEqualTo(nested);
  }

  private static RawMessageInfo generatedMessageInfo() {
    try {
      GeneratedMessageLite<?, ?> defaultInstance = Proto2MessageLite.getDefaultInstance();
      return (RawMessageInfo) defaultInstance.buildMessageInfo();
    } catch (Exception e) {
      throw new AssertionError(e);
    }
  }

  private static Schema<Proto2MessageLite> newSchema(RawMessageInfo messageInfo) {
    return MessageSchema.newSchema(
        Proto2MessageLite.class,
        messageInfo,
        NewInstanceSchemas.lite(),
        ListFieldSchemas.lite(),
        SchemaUtil.unknownFieldSetLiteSchema(),
        ExtensionSchemas.lite(),
        MapFieldSchemas.lite());
  }

  /**
   * Returns {@code info} with every sub-message type in {@code objects[]} identified by its default
   * instance. Walks the info string as {@code MessageSchema.newSchemaForRawMessageInfo} does.
   */
  private static RawMessageInfo identifySubmessagesByDefaultInstance(
      RawMessageInfo info, RewriteCounts counts) {
    if (counts == null) {
      counts = new RewriteCounts();
    }
    String string = info.getStringInfo();
    Object[] objects = info.getObjects();
    Class<?> messageClass = info.getDefaultInstance().getClass();
    boolean proto2 = info.getSyntax() == ProtoSyntax.PROTO2;
    int[] position = {0};

    skip(string, position); // flags
    int fieldCount = decode(string, position);
    if (fieldCount == 0) {
      return info;
    }
    int oneofCount = decode(string, position);
    int hasBitsCount = decode(string, position);
    // minFieldNumber, maxFieldNumber, numEntries, mapFieldCount, repeatedFieldCount,
    // checkInitialized.
    for (int i = 0; i < 6; i++) {
      skip(string, position);
    }

    List<Object> rewritten = new ArrayList<>();
    int objectsPosition = 0;
    // (oneof value, oneof case) name pairs and hasbits names.
    while (objectsPosition < oneofCount * 2 + hasBitsCount) {
      rewritten.add(objects[objectsPosition++]);
    }
    while (position[0] < string.length()) {
      skip(string, position); // field number
      int typeWithExtraBits = decode(string, position);
      int type = typeWithExtraBits & 0xFF;
      boolean closedEnum = proto2 || (typeWithExtraBits & LEGACY_ENUM_IS_CLOSED_BIT) != 0;
      if (type >= FIRST_ONEOF_TYPE) {
        skip(string, position); // oneof index
        int oneofType = type - FIRST_ONEOF_TYPE;
        if (oneofType == 9 /* MESSAGE */ || oneofType == 17 /* GROUP */) {
          rewritten.add(defaultInstanceOf((Class<?>) objects[objectsPosition++]));
          counts.oneof++;
        } else if (oneofType == 12 /* ENUM */ && closedEnum) {
          rewritten.add(objects[objectsPosition++]);
        }
        continue;
      }
      String fieldName = (String) objects[objectsPosition++];
      rewritten.add(fieldName);
      if (type == 9 /* MESSAGE */ || type == 17 /* GROUP */) {
        rewritten.add(defaultInstanceOf(declaredFieldType(messageClass, fieldName)));
        counts.singular++;
      } else if (type == 27 /* MESSAGE_LIST */ || type == 49 /* GROUP_LIST */) {
        rewritten.add(defaultInstanceOf((Class<?>) objects[objectsPosition++]));
        counts.repeated++;
      } else if ((type == 12 /* ENUM */ || type == 30 /* ENUM_LIST */ || type == 44)
          && closedEnum) {
        rewritten.add(objects[objectsPosition++]);
      } else if (type == 50 /* MAP */) {
        rewritten.add(objects[objectsPosition++]);
        if ((typeWithExtraBits & LEGACY_ENUM_IS_CLOSED_BIT) != 0) {
          rewritten.add(objects[objectsPosition++]);
        }
      }
      if ((typeWithExtraBits & HAS_HAS_BIT) != 0 && type <= 17 /* GROUP */) {
        skip(string, position); // hasbit index
      }
    }
    assertThat(objectsPosition).isEqualTo(objects.length);
    return new RawMessageInfo(info.getDefaultInstance(), string, rewritten.toArray());
  }

  /** Decodes one int of a RawMessageInfo string, as MessageSchema does. */
  private static int decode(String string, int[] position) {
    int next = string.charAt(position[0]++);
    if (next < 0xD800) {
      return next;
    }
    int result = next & 0x1FFF;
    int shift = 13;
    while ((next = string.charAt(position[0]++)) >= 0xD800) {
      result |= (next & 0x1FFF) << shift;
      shift += 13;
    }
    return result | (next << shift);
  }

  private static void skip(String string, int[] position) {
    int unused = decode(string, position);
  }

  private static Class<?> declaredFieldType(Class<?> messageClass, String fieldName) {
    try {
      return messageClass.getDeclaredField(fieldName).getType();
    } catch (NoSuchFieldException e) {
      throw new AssertionError(e);
    }
  }

  private static GeneratedMessageLite<?, ?> defaultInstanceOf(Class<?> messageClass) {
    GeneratedMessageLite<?, ?> defaultInstance =
        GeneratedMessageLite.getDefaultInstance(
            messageClass.asSubclass(GeneratedMessageLite.class));
    assertThat(defaultInstance).isNotNull();
    return defaultInstance;
  }
}
