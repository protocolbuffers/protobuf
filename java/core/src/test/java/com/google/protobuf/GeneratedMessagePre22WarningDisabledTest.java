package com.google.protobuf;

import protobuf_unittest.UnittestProto.TestAllExtensions;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

@RunWith(JUnit4.class)
public class GeneratedMessagePre22WarningDisabledTest {
  @Test
  public void generatedMessage_makeExtensionsImmutableShouldNotThrow() {
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
  }

  @Test
  public void extendableMessage_makeExtensionsImmutableShouldNotThrow() {
    GeneratedMessageV3.ExtendableMessage<TestAllExtensions> msg =
        TestAllExtensions.newBuilder().build();
    msg.makeExtensionsImmutable();
  }
}

