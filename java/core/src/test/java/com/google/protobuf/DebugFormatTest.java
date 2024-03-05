package com.google.protobuf;

import static com.google.common.truth.Truth.assertThat;

import com.google.protobuf.Descriptors.FieldDescriptor;
import protobuf_unittest.UnittestProto.RedactedFields;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

@RunWith(JUnit4.class)
public final class DebugFormatTest {

  private static final String REDACTED_REGEX = getRedactedRegex();
  private static final String UNSTABLE_PREFIX_SINGLE_LINE = getUnstablePrefix(true);
  private static final String UNSTABLE_PREFIX_MULTILINE = getUnstablePrefix(false);

  private static String getRedactedRegex() {
    String redactedRegex = "\\[REDACTED\\]";
    return redactedRegex;
  }

  private static String getUnstablePrefix(boolean singleLine) {
    String unstablePrefix = "";
    return unstablePrefix;
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

}
