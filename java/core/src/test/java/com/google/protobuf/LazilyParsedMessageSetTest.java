// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

package com.google.protobuf;

import static com.google.common.truth.Truth.assertThat;

import proto2_unittest.UnittestMset.RawMessageSet;
import proto2_unittest.UnittestMset.TestMessageSetExtension1;
import proto2_unittest.UnittestMset.TestMessageSetExtension2;
import proto2_unittest.UnittestMset.TestMessageSetExtension3;
import proto2_wireformat_unittest.UnittestMsetWireFormat.TestMessageSet;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

/** Tests related to handling of MessageSets with lazily parsed extensions. */
@RunWith(JUnit4.class)
public class LazilyParsedMessageSetTest {
  private static final int TYPE_ID_1 =
      TestMessageSetExtension1.getDescriptor().getExtensions().get(0).getNumber();
  private static final int TYPE_ID_2 =
      TestMessageSetExtension2.getDescriptor().getExtensions().get(0).getNumber();
  private static final int TYPE_ID_3 =
      TestMessageSetExtension3.getDescriptor().getExtensions().get(0).getNumber();
  private static final ByteString CORRUPTED_MESSAGE_PAYLOAD =
      ByteString.copyFrom(new byte[] {(byte) 0xff});

  @Before
  public void setUp() {
    ExtensionRegistryLite.setEagerlyParseMessageSets(false);
  }

  @Test
  public void testParseAndUpdateMessageSet_unaccessedLazyFieldsAreNotLoaded() throws Exception {
    ExtensionRegistry extensionRegistry = ExtensionRegistry.newInstance();
    extensionRegistry.add(TestMessageSetExtension1.messageSetExtension);
    extensionRegistry.add(TestMessageSetExtension2.messageSetExtension);
    extensionRegistry.add(TestMessageSetExtension3.messageSetExtension);

    // Set up a TestMessageSet with 2 extensions. The first extension has corrupted payload
    // data. The test below makes sure that we never load this extension. If we ever do, then we
    // will handle the exception and replace the value with the default empty message (this behavior
    // is tested below in testLoadCorruptedLazyField_getsReplacedWithEmptyMessage). Later on we
    // check that when we serialize the message set, we still have corrupted payload for the first
    // extension.
    RawMessageSet inputRaw =
        RawMessageSet.newBuilder()
            .addItem(
                RawMessageSet.Item.newBuilder()
                    .setTypeId(TYPE_ID_1)
                    .setMessage(CORRUPTED_MESSAGE_PAYLOAD))
            .addItem(
                RawMessageSet.Item.newBuilder()
                    .setTypeId(TYPE_ID_2)
                    .setMessage(
                        TestMessageSetExtension2.newBuilder().setStr("foo").build().toByteString()))
            .build();

    ByteString inputData = inputRaw.toByteString();

    // Re-parse as a TestMessageSet, so that all extensions are lazy
    TestMessageSet messageSet = TestMessageSet.parseFrom(inputData, extensionRegistry);

    // Update one extension and add a new one.
    TestMessageSet.Builder builder = messageSet.toBuilder();
    builder.setExtension(
        TestMessageSetExtension2.messageSetExtension,
        TestMessageSetExtension2.newBuilder().setStr("bar").build());

    // Call .build() in the middle of updating the builder. This triggers a codepath that we want to
    // make sure preserves lazy fields.
    TestMessageSet unusedIntermediateMessageSet = builder.build();

    builder.setExtension(
        TestMessageSetExtension3.messageSetExtension,
        TestMessageSetExtension3.newBuilder().setRequiredInt(666).build());

    TestMessageSet updatedMessageSet = builder.build();

    // Check that hasExtension call does not load lazy fields.
    assertThat(updatedMessageSet.hasExtension(TestMessageSetExtension1.messageSetExtension))
        .isTrue();

    // Serialize. The first extension should still be unloaded and will get serialized using the
    // same corrupted byte array.
    ByteString outputData = updatedMessageSet.toByteString();

    // Re-parse as RawMessageSet
    RawMessageSet actualRaw =
        RawMessageSet.parseFrom(outputData, ExtensionRegistry.getEmptyRegistry());

    RawMessageSet expectedRaw =
        RawMessageSet.newBuilder()
            .addItem(
                RawMessageSet.Item.newBuilder()
                    .setTypeId(TYPE_ID_1)
                    // This is the important part -- we want to make sure that the payload of the
                    // 1st extensions is the same corrupted byte array. If we ever load the
                    // extension during our manipulations above, then we would have replaced it with
                    // the default empty message.
                    .setMessage(CORRUPTED_MESSAGE_PAYLOAD))
            .addItem(
                RawMessageSet.Item.newBuilder()
                    .setTypeId(TYPE_ID_2)
                    .setMessage(
                        TestMessageSetExtension2.newBuilder().setStr("bar").build().toByteString()))
            .addItem(
                RawMessageSet.Item.newBuilder()
                    .setTypeId(TYPE_ID_3)
                    .setMessage(
                        TestMessageSetExtension3.newBuilder()
                            .setRequiredInt(666)
                            .build()
                            .toByteString()))
            .build();

    assertThat(actualRaw).isEqualTo(expectedRaw);
  }

  @Test
  public void testLoadCorruptedLazyField_getsReplacedWithEmptyMessage() throws Exception {
    ExtensionRegistry extensionRegistry = ExtensionRegistry.newInstance();
    extensionRegistry.add(TestMessageSetExtension1.messageSetExtension);

    RawMessageSet inputRaw =
        RawMessageSet.newBuilder()
            .addItem(
                RawMessageSet.Item.newBuilder()
                    .setTypeId(TYPE_ID_1)
                    .setMessage(CORRUPTED_MESSAGE_PAYLOAD))
            .build();

    ByteString inputData = inputRaw.toByteString();

    // Re-parse as a TestMessageSet, so that all extensions are lazy
    TestMessageSet messageSet = TestMessageSet.parseFrom(inputData, extensionRegistry);

    assertThat(messageSet.getExtension(TestMessageSetExtension1.messageSetExtension))
        .isEqualTo(TestMessageSetExtension1.getDefaultInstance());

    // Serialize. The first extension should be serialized as an empty message.
    ByteString outputData = messageSet.toByteString();

    // Re-parse as RawMessageSet
    RawMessageSet actualRaw =
        RawMessageSet.parseFrom(outputData, ExtensionRegistry.getEmptyRegistry());

    RawMessageSet expectedRaw =
        RawMessageSet.newBuilder()
            .addItem(
                RawMessageSet.Item.newBuilder().setTypeId(TYPE_ID_1).setMessage(ByteString.empty()))
            .build();

    assertThat(actualRaw).isEqualTo(expectedRaw);
  }

  @Test
  public void testLoadCorruptedLazyField_getSerializedSize() throws Exception {
    ExtensionRegistry extensionRegistry = ExtensionRegistry.newInstance();
    extensionRegistry.add(TestMessageSetExtension1.messageSetExtension);
    RawMessageSet inputRaw =
        RawMessageSet.newBuilder()
            .addItem(
                RawMessageSet.Item.newBuilder()
                    .setTypeId(TYPE_ID_1)
                    .setMessage(CORRUPTED_MESSAGE_PAYLOAD))
            .build();
    ByteString inputData = inputRaw.toByteString();
    TestMessageSet messageSet = TestMessageSet.parseFrom(inputData, extensionRegistry);

    // Effectively cache the serialized size of the message set.
    assertThat(messageSet.getSerializedSize()).isEqualTo(9);

    // getExtension should mark the memoized size as "dirty" (i.e. -1).
    assertThat(messageSet.getExtension(TestMessageSetExtension1.messageSetExtension))
        .isEqualTo(TestMessageSetExtension1.getDefaultInstance());

    // toByteString calls getSerializedSize() which should re-compute the serialized size as the
    // message contains lazy fields.
    ByteString outputData = messageSet.toByteString();

    // Re-parse as RawMessageSet
    RawMessageSet actualRaw =
        RawMessageSet.parseFrom(outputData, ExtensionRegistry.getEmptyRegistry());

    RawMessageSet expectedRaw =
        RawMessageSet.newBuilder()
            .addItem(
                RawMessageSet.Item.newBuilder().setTypeId(TYPE_ID_1).setMessage(ByteString.empty()))
            .build();

    assertThat(actualRaw).isEqualTo(expectedRaw);
  }
}
