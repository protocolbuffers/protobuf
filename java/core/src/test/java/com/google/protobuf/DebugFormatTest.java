package com.google.protobuf;

import static com.google.common.truth.Truth.assertThat;
import static proto2_unittest.UnittestProto.redactedExtension;

import com.google.protobuf.Descriptors.FieldDescriptor;
import proto2_unittest.UnittestProto.RedactedFields;
import proto2_unittest.UnittestProto.TestEmptyMessage;
import proto2_unittest.UnittestProto.TestNestedMessageRedaction;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

@RunWith(JUnit4.class)
public class DebugFormatTest {

  static final String REDACTED_REGEX = "\\[REDACTED\\]";
  static final String UNSTABLE_PREFIX_SINGLE_LINE = getUnstablePrefix(true);
  static final String UNSTABLE_PREFIX_MULTILINE = getUnstablePrefix(false);

  private static String getUnstablePrefix(boolean singleLine) {
    return "";
  }

  @Test
  public void multilineMessageFormat_returnsMultiline() {
    RedactedFields message = RedactedFields.newBuilder().setOptionalUnredactedString("foo").build();

    String result = DebugFormat.multiline().toString(message);
    assertThat(result)
        .matches(
            String.format("%soptional_unredacted_string: \"foo\"\n", UNSTABLE_PREFIX_MULTILINE));
  }

  @Test
  public void singleLineMessageFormat_returnsSingleLine() {
    RedactedFields message = RedactedFields.newBuilder().setOptionalUnredactedString("foo").build();

    String result = DebugFormat.singleLine().toString(message);
    assertThat(result)
        .matches(
            String.format("%soptional_unredacted_string: \"foo\"", UNSTABLE_PREFIX_SINGLE_LINE));
  }

  @Test
  public void messageFormat_debugRedactFieldIsRedacted() {
    RedactedFields message = RedactedFields.newBuilder().setOptionalRedactedString("foo").build();

    String result = DebugFormat.multiline().toString(message);

    assertThat(result)
        .matches(
            String.format(
                "%soptional_redacted_string: %s\n", UNSTABLE_PREFIX_MULTILINE, REDACTED_REGEX));
  }

  @Test
  public void messageFormat_debugRedactMessageIsRedacted() {
    RedactedFields message =
        RedactedFields.newBuilder()
            .setOptionalRedactedMessage(
                TestNestedMessageRedaction.newBuilder().setOptionalUnredactedNestedString("foo"))
            .build();

    String result = DebugFormat.multiline().toString(message);

    assertThat(result)
        .matches(
            String.format(
                "%soptional_redacted_message \\{\n  %s\n\\}\n",
                UNSTABLE_PREFIX_MULTILINE, REDACTED_REGEX));
  }

  @Test
  public void messageFormat_debugRedactMapIsRedacted() {
    RedactedFields message = RedactedFields.newBuilder().putMapRedactedString("foo", "bar").build();

    String result = DebugFormat.multiline().toString(message);

    assertThat(result)
        .matches(
            String.format(
                "%smap_redacted_string \\{\\n  %s\n\\}\n",
                UNSTABLE_PREFIX_MULTILINE, REDACTED_REGEX));
  }

  @Test
  public void messageFormat_debugRedactExtensionIsRedacted() {
    RedactedFields message =
        RedactedFields.newBuilder().setExtension(redactedExtension, "foo").build();

    String result = DebugFormat.multiline().toString(message);

    assertThat(result)
        .matches(
            String.format(
                "%s\\[proto2_unittest\\.redacted_extension\\]: %s\n",
                UNSTABLE_PREFIX_MULTILINE, REDACTED_REGEX));
  }

  @Test
  public void messageFormat_redactFalseIsNotRedacted() {
    RedactedFields message =
        RedactedFields.newBuilder().setOptionalRedactedFalseString("foo").build();

    String result = DebugFormat.multiline().toString(message);
    assertThat(result)
        .matches(
            String.format(
                "%soptional_redacted_false_string: \"foo\"\n", UNSTABLE_PREFIX_MULTILINE));
  }

  @Test
  public void messageFormat_nonSensitiveFieldIsNotRedacted() {
    RedactedFields message = RedactedFields.newBuilder().setOptionalUnredactedString("foo").build();

    String result = DebugFormat.multiline().toString(message);

    assertThat(result)
        .matches(
            String.format("%soptional_unredacted_string: \"foo\"\n", UNSTABLE_PREFIX_MULTILINE));
  }

  @Test
  public void descriptorDebugFormat_returnsExpectedFormat() {
    FieldDescriptor field =
        RedactedFields.getDescriptor().findFieldByName("optional_redacted_string");
    String result = DebugFormat.multiline().toString(field, "foo");
    assertThat(result)
        .matches(
            String.format(
                "%soptional_redacted_string: %s\n", UNSTABLE_PREFIX_MULTILINE, REDACTED_REGEX));
  }

  @Test
  public void unstableFormat_isStablePerProcess() {
    RedactedFields message1 =
        RedactedFields.newBuilder().setOptionalUnredactedString("foo").build();
    RedactedFields message2 =
        RedactedFields.newBuilder().setOptionalUnredactedString("foo").build();
    for (int i = 0; i < 5; i++) {
      String result1 = DebugFormat.multiline().toString(message1);
      String result2 = DebugFormat.multiline().toString(message2);
      assertThat(result1).isEqualTo(result2);
    }
  }

  @Test
  public void lazyDebugFormatMessage_supportsImplicitFormatting() {
    RedactedFields message = RedactedFields.newBuilder().setOptionalUnredactedString("foo").build();

    Object lazyDebug = DebugFormat.singleLine().lazyToString(message);

    assertThat(String.format("%s", lazyDebug))
        .matches(
            String.format("%soptional_unredacted_string: \"foo\"", UNSTABLE_PREFIX_SINGLE_LINE));
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
  public void unknownFieldsDebugFormat_returnsExpectedFormat() {
    TestEmptyMessage unknownFields =
        TestEmptyMessage.newBuilder().setUnknownFields(makeUnknownFieldSet()).build();

    assertThat(DebugFormat.multiline().toString(unknownFields))
        .matches(
            String.format("%s5: UNKNOWN_VARINT %s\n", UNSTABLE_PREFIX_MULTILINE, REDACTED_REGEX)
                + String.format("5: UNKNOWN_FIXED32 %s\n", REDACTED_REGEX)
                + String.format("5: UNKNOWN_FIXED64 %s\n", REDACTED_REGEX)
                + String.format("5: UNKNOWN_STRING %s\n", REDACTED_REGEX)
                + String.format("5: \\{\n  12: UNKNOWN_VARINT %s\n\\}\n", REDACTED_REGEX)
                + String.format("5 \\{\n  10: UNKNOWN_VARINT %s\n\\}\n", REDACTED_REGEX)
                + String.format("8: UNKNOWN_VARINT %s\n", REDACTED_REGEX)
                + String.format("8: UNKNOWN_VARINT %s\n", REDACTED_REGEX)
                + String.format("8: UNKNOWN_VARINT %s\n", REDACTED_REGEX)
                + String.format("15: UNKNOWN_VARINT %s\n", REDACTED_REGEX)
                + String.format("15: UNKNOWN_FIXED32 %s\n", REDACTED_REGEX)
                + String.format("15: UNKNOWN_FIXED64 %s\n", REDACTED_REGEX));
  }
}
