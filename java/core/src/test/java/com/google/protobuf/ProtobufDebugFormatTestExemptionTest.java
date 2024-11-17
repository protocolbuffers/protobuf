package com.google.protobuf;

import static com.google.common.truth.Truth.assertThat;

import com.google.apps.framework.testing.junit.ExpectedLogMessages;
import com.google.common.flogger.GoogleLogger;
import protobuf_unittest.UnittestProto.TestAllTypes;
import java.util.logging.Level;
import org.junit.BeforeClass;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

@RunWith(JUnit4.class)
public final class ProtobufDebugFormatTestExemptionTest {

  private static Message message;
  // The randomized prefix is per-process, toString() output of an empty message is the prefix for
  // the process.
  private static String randomizedPrefix;
  private static ProtoWrapper protoWrapper;
  private static final GoogleLogger logger = GoogleLogger.forEnclosingClass();

  @Rule
  public final ExpectedLogMessages logged = ExpectedLogMessages.none().setMinimumLevel(Level.INFO);

  @Rule
  public final ProtobufDebugFormatTestExemptionRule exemptionRule =
      new ProtobufDebugFormatTestExemptionRule();

  // Currently Flogger does not use ProtobufToStringOutput.callWithDebugFormat, and
  // ProtobufDebugFormatTestExemption only works when ProtobufToStringOutput.callWithDebugFormat
  // is invoked under Flogger. As a workaround, we create this wrapper class so that Flogger
  // calls toString of this class, which calls ProtobufToStringOutput.callWithDebugFormat.
  private static class ProtoWrapper {
    Message message;

    public ProtoWrapper(Message message) {
      this.message = message;
    }

    @Override
    public String toString() {
      String[] result = new String[1];
      ProtobufToStringOutput.callWithDebugFormat(() -> result[0] = message.toString());
      return result[0];
    }
  }

  @BeforeClass
  public static void setupTestClass() {
    message = TestAllTypes.newBuilder().setOptionalInt32(123).build();
    randomizedPrefix = DebugFormat.multiline().toString(TestAllTypes.newBuilder().build());
    protoWrapper = new ProtoWrapper(message);
  }

  @Test
  public void withoutAnnotation_debugFormatInLog() {
    logged.expectRegex("protoWrapper: " + randomizedPrefix + "optional_int32: 123");
    logger.atInfo().log("protoWrapper: %s", protoWrapper);
  }

  @Test
  @ProtobufDebugFormatTestExemption
  public void withAnnotation_textFormatInLog() {
    logged.expectRegex("protoWrapper: optional_int32: 123");
    logger.atInfo().log("protoWrapper: %s", protoWrapper);
  }

  @Test
  @ProtobufDebugFormatTestExemption
  public void withAnnotation_callWithDebugFormatReturnsDebugFormat() {
    assertThat(protoWrapper.toString())
        .contains(randomizedPrefix + "optional_int32: 123");
  }
}
