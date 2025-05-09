package com.google.protobuf;

import static com.google.common.truth.Truth.assertThat;
import static org.junit.Assert.assertThrows;

import protobuf_unittest.UnittestProto.TestAllExtensions;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

@RunWith(JUnit4.class)
public class GeneratedMessagePre22ErrorTest {
  @Test
  public void generatedMessage_makeExtensionsImmutableShouldError() {
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
    Throwable e = assertThrows(UnsupportedOperationException.class, () -> msg.makeExtensionsImmutable());
    assertThat(e).hasMessageThat().contains(GeneratedMessage.PRE22_GENCODE_VULNERABILITY_MESSAGE);
  }

  @Test
  public void extendableMessage_makeExtensionsImmutableShouldError() {
    GeneratedMessageV3.ExtendableMessage<TestAllExtensions> msg =
        TestAllExtensions.newBuilder().build();
    Throwable e = assertThrows(UnsupportedOperationException.class, () -> msg.makeExtensionsImmutable());
    assertThat(e).hasMessageThat().contains(GeneratedMessage.PRE22_GENCODE_VULNERABILITY_MESSAGE);
  }
}
