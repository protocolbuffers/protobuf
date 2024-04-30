package com.google.protobuf;

import static com.google.common.truth.Truth.assertThat;

import protobuf_unittest.UnittestProto;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

@RunWith(JUnit4.class)
public final class LegacyUnredactedTextFormatTest {

  @Test
  public void legacyUnredactedTextFormatMessage_defaultMessageIsEmpty() {
    UnittestProto.TestEmptyMessage message = UnittestProto.TestEmptyMessage.getDefaultInstance();
    assertThat(LegacyUnredactedTextFormat.legacyUnredactedTextFormat(message)).isEmpty();

    UnittestProto.TestEmptyMessage singleLineMessage =
        UnittestProto.TestEmptyMessage.getDefaultInstance();
    assertThat(LegacyUnredactedTextFormat.legacyUnredactedSingleLineTextFormat(singleLineMessage))
        .isEmpty();
  }

  @Test
  public void legacyUnredactedTextFormatMessage_hasExpectedFormats() {
    UnittestProto.TestAllTypes message =
        UnittestProto.TestAllTypes.newBuilder()
            .setOptionalInt32(1)
            .setOptionalString("hello")
            .setOptionalNestedMessage(
                UnittestProto.TestAllTypes.NestedMessage.newBuilder().setBb(2).build())
            .setOptionalForeignMessage(UnittestProto.ForeignMessage.getDefaultInstance())
            .build();
    assertThat(LegacyUnredactedTextFormat.legacyUnredactedTextFormat(message))
        .isEqualTo(
            "optional_int32: 1\n"
                + "optional_string: \"hello\"\n"
                + "optional_nested_message {\n"
                + "  bb: 2\n"
                + "}\n"
                + "optional_foreign_message {\n"
                + "}\n");
    assertThat(LegacyUnredactedTextFormat.legacyUnredactedSingleLineTextFormat(message))
        .isEqualTo(
            "optional_int32: 1"
                + " optional_string: \"hello\""
                + " optional_nested_message {"
                + " bb: 2"
                + " }"
                + " optional_foreign_message {"
                + " }");
  }

  private UnknownFieldSet makeUnknownFieldSet() {
    return UnknownFieldSet.newBuilder()
        .addField(
            5,
            UnknownFieldSet.Field.newBuilder()
                .addVarint(1)
                .addFixed32(2)
                .addFixed64(3)
                .addLengthDelimited(ByteString.copyFromUtf8("4"))
                .addLengthDelimited(
                    UnknownFieldSet.newBuilder()
                        .addField(12, UnknownFieldSet.Field.newBuilder().addVarint(6).build())
                        .build()
                        .toByteString())
                .addGroup(
                    UnknownFieldSet.newBuilder()
                        .addField(10, UnknownFieldSet.Field.newBuilder().addVarint(5).build())
                        .build())
                .build())
        .addField(
            8, UnknownFieldSet.Field.newBuilder().addVarint(1).addVarint(2).addVarint(3).build())
        .addField(
            15,
            UnknownFieldSet.Field.newBuilder()
                .addVarint(0xABCDEF1234567890L)
                .addFixed32(0xABCD1234)
                .addFixed64(0xABCDEF1234567890L)
                .build())
        .build();
  }

  @Test
  public void legacyUnredactedTextFormatUnknownFields_hasExpectedFormats() {
    UnknownFieldSet unknownFields = makeUnknownFieldSet();
    assertThat(LegacyUnredactedTextFormat.legacyUnredactedTextFormat(unknownFields))
        .isEqualTo(
            "5: 1\n"
                + "5: 0x00000002\n"
                + "5: 0x0000000000000003\n"
                + "5: \"4\"\n"
                + "5: {\n"
                + "  12: 6\n"
                + "}\n"
                + "5 {\n"
                + "  10: 5\n"
                + "}\n"
                + "8: 1\n"
                + "8: 2\n"
                + "8: 3\n"
                + "15: 12379813812177893520\n"
                + "15: 0xabcd1234\n"
                + "15: 0xabcdef1234567890\n");
    assertThat(LegacyUnredactedTextFormat.legacyUnredactedSingleLineTextFormat(unknownFields))
        .isEqualTo(
            "5: 1"
                + " 5: 0x00000002"
                + " 5: 0x0000000000000003"
                + " 5: \"4\""
                + " 5: {"
                + " 12: 6"
                + " }"
                + " 5 {"
                + " 10: 5"
                + " }"
                + " 8: 1"
                + " 8: 2"
                + " 8: 3"
                + " 15: 12379813812177893520"
                + " 15: 0xabcd1234"
                + " 15: 0xabcdef1234567890");
  }
}
