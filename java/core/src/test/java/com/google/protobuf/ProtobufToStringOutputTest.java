package com.google.protobuf;

import static com.google.common.truth.Truth.assertThat;

import protobuf_unittest.UnittestProto.RedactedFields;
import protobuf_unittest.UnittestProto.TestNestedMessageRedaction;
import java.util.ArrayList;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

@RunWith(JUnit4.class)
public final class ProtobufToStringOutputTest extends DebugFormatTest {
  RedactedFields message;

  @Before
  public void setupTest() {
    message =
        RedactedFields.newBuilder()
            .setOptionalUnredactedString("foo")
            .setOptionalRedactedString("bar")
            .setOptionalRedactedMessage(
                TestNestedMessageRedaction.newBuilder().setOptionalUnredactedNestedString("foobar"))
            .build();
  }

  @Test
  public void toStringFormat_defaultFormat() {
    assertThat(message.toString())
        .matches(
            "optional_redacted_string: \"bar\"\n"
                + "optional_unredacted_string: \"foo\"\n"
                + "optional_redacted_message \\{\n"
                + "  optional_unredacted_nested_string: \"foobar\"\n"
                + "\\}\n");
  }

  @Test
  public void runWithDebugFormat_testDebugFormat() {
    ProtobufToStringOutput.runWithDebugFormat(
        () ->
            assertThat(message.toString())
                .matches(
                    String.format(
                        "%soptional_redacted_string: %s\n"
                            + "optional_unredacted_string: \"foo\"\n"
                            + "optional_redacted_message \\{\n"
                            + "  %s\n"
                            + "\\}\n",
                        UNSTABLE_PREFIX_MULTILINE, REDACTED_REGEX, REDACTED_REGEX)));
  }

  @Test
  public void callWithDebugFormatWithoutException_testDebugFormat() {
    String protoStr =
        ProtobufToStringOutput.callWithDebugFormatWithoutException(
            () -> {
              return message.toString();
            });
    assertThat(protoStr)
        .matches(
            String.format(
                "%soptional_redacted_string: %s\n"
                    + "optional_unredacted_string: \"foo\"\n"
                    + "optional_redacted_message \\{\n"
                    + "  %s\n"
                    + "\\}\n",
                UNSTABLE_PREFIX_MULTILINE, REDACTED_REGEX, REDACTED_REGEX));
  }

  @Test
  public void callWithDebugFormatWithException_testDebugFormat() throws Exception {
    String protoStr =
        ProtobufToStringOutput.callWithDebugFormatWithoutException(
            () -> {
              return message.toString();
            });
    assertThat(protoStr)
        .matches(
            String.format(
                "%soptional_redacted_string: %s\n"
                    + "optional_unredacted_string: \"foo\"\n"
                    + "optional_redacted_message \\{\n"
                    + "  %s\n"
                    + "\\}\n",
                UNSTABLE_PREFIX_MULTILINE, REDACTED_REGEX, REDACTED_REGEX));
  }

  @Test
  public void runWithTextFormat_testTextFormat() {
    ProtobufToStringOutput.runWithTextFormat(
        () -> {
          assertThat(message.toString())
              .matches(
                  "optional_redacted_string: \"bar\"\n"
                      + "optional_unredacted_string: \"foo\"\n"
                      + "optional_redacted_message \\{\n"
                      + "  optional_unredacted_nested_string: \"foobar\"\n"
                      + "\\}\n");
        });
  }

  @Test
  public void callWithDebugFormatWithoutException_testTextFormat() {
    String protoStr =
        ProtobufToStringOutput.callWithTextFormatWithoutException(
            () -> {
              return message.toString();
            });
    assertThat(protoStr)
        .matches(
            "optional_redacted_string: \"bar\"\n"
                + "optional_unredacted_string: \"foo\"\n"
                + "optional_redacted_message \\{\n"
                + "  optional_unredacted_nested_string: \"foobar\"\n"
                + "\\}\n");
  }

  @Test
  public void callWithDebugFormatWithException_testTextFormat() throws Exception {
    String protoStr =
        ProtobufToStringOutput.callWithTextFormatWithoutException(
            () -> {
              return message.toString();
            });
    assertThat(protoStr)
        .matches(
            "optional_redacted_string: \"bar\"\n"
                + "optional_unredacted_string: \"foo\"\n"
                + "optional_redacted_message \\{\n"
                + "  optional_unredacted_nested_string: \"foobar\"\n"
                + "\\}\n");
  }

  @Test
  public void runWithDebugFormat_testProtoWrapperWithDebugFormat() {
    ProtobufToStringOutput.runWithDebugFormat(
        () -> {
          ArrayList<RedactedFields> list = new ArrayList<>();
          list.add(message);
          assertThat(list.toString())
              .matches(
                  String.format(
                      "\\[%soptional_redacted_string: %s\n"
                          + "optional_unredacted_string: \"foo\"\n"
                          + "optional_redacted_message \\{\n"
                          + "  %s\n"
                          + "\\}\n"
                          + "\\]",
                      UNSTABLE_PREFIX_MULTILINE, REDACTED_REGEX, REDACTED_REGEX));
        });
  }

  @Test
  public void callWithDebugFormatWithoutException_testProtoWrapperWithDebugFormat() {
    ArrayList<RedactedFields> list = new ArrayList<>();
    list.add(message);
    String objStr =
        ProtobufToStringOutput.callWithDebugFormatWithoutException(
            () -> {
              return list.toString();
            });
    assertThat(objStr)
        .matches(
            String.format(
                "\\[%soptional_redacted_string: %s\n"
                    + "optional_unredacted_string: \"foo\"\n"
                    + "optional_redacted_message \\{\n"
                    + "  %s\n"
                    + "\\}\n"
                    + "\\]",
                UNSTABLE_PREFIX_MULTILINE, REDACTED_REGEX, REDACTED_REGEX));
  }

  @Test
  public void callWithDebugFormatWithException_testProtoWrapperWithDebugFormat() throws Exception {
    ArrayList<RedactedFields> list = new ArrayList<>();
    list.add(message);
    String objStr =
        ProtobufToStringOutput.callWithDebugFormatWithoutException(
            () -> {
              return list.toString();
            });
    assertThat(objStr)
        .matches(
            String.format(
                "\\[%soptional_redacted_string: %s\n"
                    + "optional_unredacted_string: \"foo\"\n"
                    + "optional_redacted_message \\{\n"
                    + "  %s\n"
                    + "\\}\n"
                    + "\\]",
                UNSTABLE_PREFIX_MULTILINE, REDACTED_REGEX, REDACTED_REGEX));
  }

  @Test
  public void runWithTextFormat_testProtoWrapperWithTextFormat() {
    ProtobufToStringOutput.runWithTextFormat(
        () -> {
          ArrayList<RedactedFields> list = new ArrayList<>();
          list.add(message);
          assertThat(list.toString())
              .matches(
                  "\\[optional_redacted_string: \"bar\"\n"
                      + "optional_unredacted_string: \"foo\"\n"
                      + "optional_redacted_message \\{\n"
                      + "  optional_unredacted_nested_string: \"foobar\"\n"
                      + "\\}\n"
                      + "\\]");
        });
  }

  @Test
  public void callWithDebugFormatWithoutException_testProtoWrapperWithTextFormat() {
    ArrayList<RedactedFields> list = new ArrayList<>();
    list.add(message);
    String objStr =
        ProtobufToStringOutput.callWithTextFormatWithoutException(
            () -> {
              return list.toString();
            });
    assertThat(objStr)
        .matches(
            "\\[optional_redacted_string: \"bar\"\n"
                + "optional_unredacted_string: \"foo\"\n"
                + "optional_redacted_message \\{\n"
                + "  optional_unredacted_nested_string: \"foobar\"\n"
                + "\\}\n"
                + "\\]");
  }

  @Test
  public void callWithDebugFormatWithException_testProtoWrapperWithTextFormat() throws Exception {
    ArrayList<RedactedFields> list = new ArrayList<>();
    list.add(message);
    String objStr =
        ProtobufToStringOutput.callWithTextFormatWithoutException(
            () -> {
              return list.toString();
            });
    assertThat(objStr)
        .matches(
            "\\[optional_redacted_string: \"bar\"\n"
                + "optional_unredacted_string: \"foo\"\n"
                + "optional_redacted_message \\{\n"
                + "  optional_unredacted_nested_string: \"foobar\"\n"
                + "\\}\n"
                + "\\]");
  }
}
