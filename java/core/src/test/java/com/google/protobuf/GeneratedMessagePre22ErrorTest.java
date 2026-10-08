// Protocol Buffers - Google's data interchange format
// Copyright 2025 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

package com.google.protobuf;

import static com.google.common.truth.Truth.assertThat;
import static com.google.common.truth.Truth.assertWithMessage;

import proto2_unittest.UnittestProto.TestAllExtensions;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

@RunWith(JUnit4.class)
public class GeneratedMessagePre22ErrorTest {
  @Test
  public void generatedMessage_makeExtensionsImmutableShouldError() {
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
    try {
      msg.makeExtensionsImmutable();
      assertWithMessage("Expected UnsupportedOperationException").fail();
    } catch (UnsupportedOperationException e) {
      // Expected
      assertThat(e).hasMessageThat().contains(GeneratedMessage.PRE22_GENCODE_VULNERABILITY_MESSAGE);
    }
  }

  @Test
  public void extendableMessage_makeExtensionsImmutableShouldError() {
    GeneratedMessage.ExtendableMessage<TestAllExtensions> msg =
        TestAllExtensions.newBuilder().build();
    try {
      msg.makeExtensionsImmutable();
      assertWithMessage("Expected UnsupportedOperationException").fail();
    } catch (UnsupportedOperationException e) {
      // Expected
      assertThat(e).hasMessageThat().contains(GeneratedMessage.PRE22_GENCODE_VULNERABILITY_MESSAGE);
    }
  }
}
