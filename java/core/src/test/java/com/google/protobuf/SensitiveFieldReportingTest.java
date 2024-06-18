package com.google.protobuf;

import static com.google.common.truth.Truth.assertThat;

import com.google.common.collect.ImmutableList;
import com.google.common.collect.Sets;
import com.google.common.collect.Streams;
import RedactionProto.TestRedactedMessage;
import RedactionProto.TestRepeatedRedactedNestMessage;
import RedactionProto.TestReportedInsideNest;
import java.time.Duration;
import java.util.List;
import java.util.Set;
import java.util.concurrent.Callable;
import java.util.logging.Level;
import java.util.logging.LogRecord;
import java.util.logging.Logger;
import java.util.regex.Matcher;
import java.util.regex.Pattern;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

@RunWith(JUnit4.class)
public final class SensitiveFieldReportingTest {

  @Before
  public final void startServer() {
    TextFormat.setSilentMarkerSyslogRate(Duration.ZERO);
    TextFormat.setShouldSkipSyslog(false);
    TextFormat.printer().setSensitiveFieldReporting(true);
  }

  private TestUtil.TestLogHandler setupLogger() {
    TestUtil.TestLogHandler logHandler = new TestUtil.TestLogHandler();
    Logger logger = Logger.getLogger(TextFormat.class.getName());
    logger.addHandler(logHandler);
    logHandler.setLevel(Level.ALL);
    return logHandler;
  }

  private static final TestRedactedMessage SENSITIVE_PROTO =
      TestRedactedMessage.newBuilder()
          .setReportedInsideNest(
              TestReportedInsideNest.newBuilder().setFieldReportedInsideNest("foo"))
          .addRepeatedMessageField(TestRepeatedRedactedNestMessage.newBuilder().setBar("bar"))
          .setUnreportedNonMetaDebugRedactField("baz")
          .setAccountCredential("redacted")
          .setMetaAnnotated("redacted")
          .setSecurityKey("redacted")
          .setSecurityMaterial("redacted")
          .setSpiiId("redacted")
          .setAccountCredentialAndSpii("redacted")
          .setPaymentsPcidSad("redacted")
          .setCloudLogging("redacted")
          .build();

  @Test
  public void testSensitiveFieldsLogged() throws Exception {
    StringBuilder sb = new StringBuilder();
    ImmutableList<Callable<String>> functions =
        ImmutableList.of(
            () -> {
              TextFormat.printer().print(SENSITIVE_PROTO, sb);
              return sb.toString();
            },
            () -> TextFormat.printToUnicodeString(SENSITIVE_PROTO),
            () -> TextFormat.shortDebugString(SENSITIVE_PROTO),
            () -> TextFormat.printer().shortDebugString(SENSITIVE_PROTO),
            () -> LegacyUnredactedTextFormat.legacyUnredactedMultilineString(SENSITIVE_PROTO),
            () -> LegacyUnredactedTextFormat.legacyUnredactedSingleLineString(SENSITIVE_PROTO),
            () -> DebugFormat.singleLine().toString(SENSITIVE_PROTO),
            () -> DebugFormat.multiline().toString(SENSITIVE_PROTO),
            () -> TextFormat.printToString(SENSITIVE_PROTO),
            () -> TextFormat.printer().printToString(SENSITIVE_PROTO),
            SENSITIVE_PROTO::toString);
    ImmutableList<String> expectedReporterLevels =
        ImmutableList.of(
            "PRINT",
            "PRINT_UNICODE",
            "SHORT_DEBUG_STRING",
            "SHORT_DEBUG_STRING",
            "LEGACY_MULTILINE",
            "LEGACY_SINGLE_LINE",
            "DEBUG_SINGLE_LINE",
            "DEBUG_MULTILINE",
            "TEXTFORMAT_PRINT_TO_STRING",
            "PRINTER_PRINT_TO_STRING",
            "ABSTRACT_TO_STRING");
    Set<String> expectedFieldList =
        Sets.newHashSet(
            "payments_pcid_sad",
            "security_material",
            "meta_annotated",
            "security_key",
            "account_credential",
            "account_credential_and_spii",
            "unreported_non_meta_debug_redact_field",
            "repeated_message_field",
            "spii_id",
            "cloud_logging");
    var unused =
        Streams.zip(
                functions.stream(),
                expectedReporterLevels.stream(),
                (function, expectedReporterLevel) -> {
                  TestUtil.TestLogHandler logHandler = setupLogger();
                  try {
                    function.call();
                  } catch (Exception e) {
                    throw new RuntimeException(e);
                  }
                  List<LogRecord> logs = logHandler.getStoredLogRecords();
                  String logMessage = logs.get(0).getMessage();
                  assertThat(logMessage).contains(expectedReporterLevel);
                  assertThat(logMessage).contains("mt=net.proto2.internal.TestRedactedMessage");
                  assertThat(logMessage).contains("net.proto2.internal.TestRedactedMessage<");
                  Pattern fieldPattern =
                      Pattern.compile("(net.proto2.internal\\.TestRedactedMessage<([a-z_\\|]+)>)");

                  Matcher matcher = fieldPattern.matcher(logMessage);
                  assertThat(matcher.find()).isTrue();
                  String fieldList = matcher.group(2);
                  Set<String> fieldSet = Sets.newHashSet(fieldList.split("\\|"));
                  assertThat(fieldSet).isEqualTo(expectedFieldList);
                  logHandler.flush();
                  return true;
                })
            .toArray();
  }

  @Test
  public void testFieldValuePrinting_doesNotReportSensitiveFields() throws Exception {
    StringBuilder sb = new StringBuilder();
    TestUtil.TestLogHandler logHandler = setupLogger();
    TextFormat.printer()
        .printFieldValue(
            SENSITIVE_PROTO.getDescriptorForType().findFieldByName("account_credential"),
            SENSITIVE_PROTO.getAccountCredential(),
            sb);
    List<LogRecord> logs = logHandler.getStoredLogRecords();
    assertThat(logs).isEmpty();
  }
}
