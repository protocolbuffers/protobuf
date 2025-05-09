package com.google.protobuf;

import static com.google.common.truth.Truth.assertThat;

import protobuf_unittest.UnittestProto.TestAllExtensions;
import java.util.logging.Level;
import java.util.logging.Logger;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

@RunWith(JUnit4.class)
public class GeneratedMessagePre22WarningDisabledTest {
  private TestUtil.TestLogHandler setupLogger() {
    TestUtil.TestLogHandler logHandler = new TestUtil.TestLogHandler();
    Logger logger = Logger.getLogger(GeneratedMessage.class.getName());
    logger.addHandler(logHandler);
    logHandler.setLevel(Level.ALL);
    return logHandler;
  }

  @Test
  public void generatedMessage_makeExtensionsImmutableShouldNotLog() {
    TestUtil.TestLogHandler logHandler = setupLogger();
    GeneratedMessageV3 msg =
        new GeneratedMessageV3() {
          @Override
          protected FieldAccessorTable internalGetFieldAccessorTable() {
            return null;
          }

          @Override
          protected Message.Builder newBuilderForType(BuilderParent parent) {
            return null;
          }

          @Override
          public Message.Builder newBuilderForType() {
            return null;
          }

          @Override
          public Message.Builder toBuilder() {
            return null;
          }

          @Override
          public Message getDefaultInstanceForType() {
            return null;
          }
        };
    msg.makeExtensionsImmutable();
    assertThat(logHandler.getStoredLogRecords()).isEmpty();
  }

  @Test
  public void extendableMessage_makeExtensionsImmutableShouldNotLog() {
    TestUtil.TestLogHandler logHandler = setupLogger();
    GeneratedMessageV3.ExtendableMessage<TestAllExtensions> msg =
        TestAllExtensions.newBuilder().build();
    msg.makeExtensionsImmutable();
    assertThat(logHandler.getStoredLogRecords()).isEmpty();
  }
}

