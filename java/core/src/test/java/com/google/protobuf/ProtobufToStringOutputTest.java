package com.google.protobuf;

import static com.google.common.truth.Truth.assertThat;

import proto2_unittest.UnittestProto.RedactedFields;
import proto2_unittest.UnittestProto.TestNestedMessageRedaction;
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
  public void toStringFormat_testDebugFormat() {
    ProtobufToStringOutput.callWithDebugFormat(
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
  public void toStringFormat_testTextFormat() {
    ProtobufToStringOutput.callWithTextFormat(
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
  public void toStringFormat_testProtoWrapperWithDebugFormat() {
    ProtobufToStringOutput.callWithDebugFormat(
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
  public void toStringFormat_testProtoWrapperWithTextFormat() {
    ProtobufToStringOutput.callWithTextFormat(
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
}
