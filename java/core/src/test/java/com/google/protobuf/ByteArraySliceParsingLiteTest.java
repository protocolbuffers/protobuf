// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

package com.google.protobuf;

import static com.google.protobuf.UnittestLite.optionalBytesExtensionLite;
import static com.google.protobuf.UnittestLite.optionalStringExtensionLite;
import static com.google.protobuf.UnittestLite.packedFixed64ExtensionLite;
import static com.google.protobuf.UnittestLite.packedInt32ExtensionLite;
import static org.junit.Assert.assertThrows;

import com.google.protobuf.UnittestLite.ForeignEnumLite;
import com.google.protobuf.UnittestLite.TestAllExtensionsLite;
import com.google.protobuf.UnittestLite.TestAllTypesLite;
import com.google.protobuf.UnittestLite.TestEmptyMessageLite;
import com.google.protobuf.UnittestLite.TestPackedExtensionsLite;
import com.google.protobuf.UnittestLite.TestPackedTypesLite;
import com.google.protobuf.testing.Proto3TestingLite.Proto3MessageLite;
import map_lite_test.MapTestProto.TestMap;
import proto2_unittest.UnittestMset.TestMessageSetExtension1;
import proto2_wireformat_unittest.UnittestMsetWireFormat.TestMessageSet;
import java.io.IOException;
import java.util.Arrays;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

/**
 * Tests that parsing a Lite message from a slice of a byte array ({@link Parser#parseFrom(byte[],
 * int, int)} and {@link MessageLite.Builder#mergeFrom(byte[], int, int)}) fails if a length prefix
 * in the message points past the end of the slice, even if the parser reads the bytes there.
 *
 * <p>Each test serializes a real message into the start of a larger array, increases a length
 * prefix in it so that it points past the end of the message, and parses just the message.
 */
@RunWith(JUnit4.class)
public class ByteArraySliceParsingLiteTest {

  private static final ExtensionRegistryLite NO_EXTENSIONS =
      ExtensionRegistryLite.getEmptyRegistry();

  /** How many bytes past the end of the message each increased length prefix points. */
  private static final int BYTES_PAST_LIMIT = 8;

  @Test
  public void stringAndBytesLengthPastLimit_failsToParse() throws Exception {
    assertFailsWithLengthPastLimit(
        TestAllTypesLite.newBuilder().setOptionalString("abc").build(),
        TestAllTypesLite.newBuilder().setOptionalBytes(ByteString.copyFromUtf8("abc")).build(),
        TestAllTypesLite.newBuilder().addRepeatedString("abc").build(),
        TestAllTypesLite.newBuilder().setOneofString("abc").build(),
        TestAllTypesLite.newBuilder().setOneofBytes(ByteString.copyFromUtf8("abc")).build(),
        // UTF-8 validated strings.
        Proto3MessageLite.newBuilder().setFieldString9("abc").build(),
        Proto3MessageLite.newBuilder().addFieldStringList26("abc").build());
  }

  @Test
  public void packedLengthPastLimit_failsToParse() throws Exception {
    assertFailsWithLengthPastLimit(
        TestPackedTypesLite.newBuilder().addPackedInt32(1).addPackedInt32(300).build(),
        TestPackedTypesLite.newBuilder().addPackedSint64(1).addPackedSint64(300).build(),
        TestPackedTypesLite.newBuilder().addPackedFixed32(1).addPackedFixed32(2).build(),
        TestPackedTypesLite.newBuilder().addPackedDouble(1).addPackedDouble(2).build(),
        TestPackedTypesLite.newBuilder().addPackedBool(true).addPackedBool(false).build(),
        TestPackedTypesLite.newBuilder()
            .addPackedEnum(ForeignEnumLite.FOREIGN_LITE_FOO)
            .addPackedEnum(ForeignEnumLite.FOREIGN_LITE_BAR)
            .build());
  }

  @Test
  public void extensionLengthPastLimit_failsToParse() throws Exception {
    assertFailsWithLengthPastLimit(
        TestUtilLite.getExtensionRegistryLite(),
        TestAllExtensionsLite.newBuilder().setExtension(optionalStringExtensionLite, "abc").build(),
        TestAllExtensionsLite.newBuilder()
            .setExtension(optionalBytesExtensionLite, ByteString.copyFromUtf8("abc"))
            .build(),
        TestPackedExtensionsLite.newBuilder()
            .addExtension(packedInt32ExtensionLite, 1)
            .addExtension(packedInt32ExtensionLite, 300)
            .build(),
        TestPackedExtensionsLite.newBuilder()
            .addExtension(packedFixed64ExtensionLite, 1L)
            .addExtension(packedFixed64ExtensionLite, 2L)
            .build());
  }

  @Test
  public void unknownFieldLengthPastLimit_failsToParse() {
    // TestEmptyMessageLite has no fields, so it parses optional_string as an unknown field.
    // Bytes: optional_string tag, length 3, "abc".
    assertFailsWithLengthPastLimit(
        TestEmptyMessageLite.getDefaultInstance(),
        TestAllTypesLite.newBuilder().setOptionalString("abc").build().toByteArray(),
        /* lengthIndex= */ 1,
        NO_EXTENSIONS);
  }

  @Test
  public void lengthInNestedMessagePastLimit_failsToParse() {
    // The length of a nested message or map entry is checked against the end of the slice, so
    // increase the length of the string in it instead.

    // Bytes: field_message_10 tag, length 5, field_string_9 tag, length 3, "abc".
    assertFailsWithLengthPastLimit(
        Proto3MessageLite.getDefaultInstance(),
        Proto3MessageLite.newBuilder()
            .setFieldMessage10(Proto3MessageLite.newBuilder().setFieldString9("abc").build())
            .build()
            .toByteArray(),
        /* lengthIndex= */ 3,
        NO_EXTENSIONS);
    // Bytes: int32_to_string_field tag, length 7, key tag, 1, value tag, length 3, "abc".
    assertFailsWithLengthPastLimit(
        TestMap.getDefaultInstance(),
        TestMap.newBuilder().putInt32ToStringField(1, "abc").build().toByteArray(),
        /* lengthIndex= */ 5,
        NO_EXTENSIONS);
  }

  @Test
  public void messageSetItemLengthPastLimit_failsToParse() {
    // Without the extension in the registry, the item's message is read as bytes.
    // Bytes: item start tag, type id tag, type id (3 bytes), message tag, length 3, message,
    // item end tag.
    assertFailsWithLengthPastLimit(
        TestMessageSet.getDefaultInstance(),
        TestMessageSet.newBuilder()
            .setExtension(
                TestMessageSetExtension1.messageSetExtension,
                TestMessageSetExtension1.newBuilder().setI(300).build())
            .build()
            .toByteArray(),
        /* lengthIndex= */ 6,
        NO_EXTENSIONS);
  }

  /**
   * Asserts that parsing each message fails if the length prefix of its only field, which is
   * length-delimited, points past the end of the message.
   */
  private static void assertFailsWithLengthPastLimit(MessageLite... messages) throws IOException {
    assertFailsWithLengthPastLimit(NO_EXTENSIONS, messages);
  }

  private static void assertFailsWithLengthPastLimit(
      ExtensionRegistryLite registry, MessageLite... messages) throws IOException {
    for (MessageLite message : messages) {
      byte[] bytes = message.toByteArray();
      // The length prefix follows the field's tag.
      CodedInputStream input = CodedInputStream.newInstance(bytes);
      input.readTag();
      assertFailsWithLengthPastLimit(
          message.getDefaultInstanceForType(), bytes, input.getTotalBytesRead(), registry);
    }
  }

  /**
   * Copies {@code message} into the start of a larger array, increases the length prefix at {@code
   * message[lengthIndex]} so that it points {@link #BYTES_PAST_LIMIT} bytes past the end of the
   * message, and asserts that parsing just the message from the array as {@code type} fails.
   */
  private static void assertFailsWithLengthPastLimit(
      MessageLite type, byte[] message, int lengthIndex, ExtensionRegistryLite registry) {
    int limit = message.length;
    byte[] data = Arrays.copyOf(message, limit + BYTES_PAST_LIMIT);
    data[lengthIndex] = (byte) (data[lengthIndex] + BYTES_PAST_LIMIT);
    Parser<? extends MessageLite> parser = parserFor(type);
    String context = Arrays.toString(data) + " with limit " + limit;

    assertThrows(
        context + ", parseFrom",
        InvalidProtocolBufferException.class,
        () -> parser.parseFrom(data, 0, limit, registry));
    assertThrows(
        context + ", Builder.mergeFrom",
        InvalidProtocolBufferException.class,
        () -> type.newBuilderForType().mergeFrom(data, 0, limit, registry));
  }

  /**
   * Returns the parser that {@link MessageLite#getParserForType()} returns for {@code type},
   * without registering it globally. Tests such as {@code LiteTest.testLazyParserRegistration}
   * expect the parser not to be registered before they run.
   */
  @SuppressWarnings({"unchecked", "rawtypes"})
  private static Parser<? extends MessageLite> parserFor(MessageLite type) {
    return new GeneratedMessageLite.DefaultInstanceBasedParser((GeneratedMessageLite) type);
  }
}
