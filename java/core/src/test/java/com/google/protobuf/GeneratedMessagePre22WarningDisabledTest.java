// Protocol Buffers - Google's data interchange format
// Copyright 2025 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

package com.google.protobuf;

import static com.google.common.truth.Truth.assertThat;

import proto2_unittest.UnittestProto.TestAllExtensions;
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
    GeneratedMessage msg =
        new GeneratedMessage() {
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
    GeneratedMessage.ExtendableMessage<TestAllExtensions> msg =
        TestAllExtensions.newBuilder().build();
    msg.makeExtensionsImmutable();
    assertThat(logHandler.getStoredLogRecords()).isEmpty();
  }
}
