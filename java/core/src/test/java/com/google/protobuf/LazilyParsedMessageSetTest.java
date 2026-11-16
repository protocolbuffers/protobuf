// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

package com.google.protobuf;

import static com.google.common.truth.Truth.assertThat;
import static org.junit.Assert.assertThrows;

import com.google.protobuf.ExtensionRegistryLite.LazyExtensionMode;
import proto2_unittest.UnittestMset.RawMessageSet;
import proto2_unittest.UnittestMset.TestMessageSetExtension1;
import proto2_unittest.UnittestMset.TestMessageSetExtension2;
import proto2_unittest.UnittestMset.TestMessageSetExtension3;
import proto2_wireformat_unittest.UnittestMsetWireFormat.TestMessageSet;
import proto2_wireformat_unittest.UnittestMsetWireFormat.TestMessageSetWireFormatContainer;
import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.util.Arrays;
import java.util.List;
import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.Parameterized;
import org.junit.runners.Parameterized.Parameters;

/** Tests related to handling of MessageSets with lazily parsed extensions. */
@RunWith(Parameterized.class)
public class LazilyParsedMessageSetTest {

  @Parameters(name = "mode={0}")
  public static List<Object[]> data() {
    return Arrays.asList(
        new Object[][] {{LazyExtensionMode.EAGER}, {LazyExtensionMode.LAZY_VERIFY_ON_ACCESS}});
  }

  private final LazyExtensionMode mode;
  private LazyExtensionMode originalMode;

  public LazilyParsedMessageSetTest(LazyExtensionMode mode) {
    this.mode = mode;
  }

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
    originalMode = ExtensionRegistryLite.getLazyExtensionMode();
    ExtensionRegistryLite.setLazyExtensionMode(mode);
  }

  @After
  public void tearDown() {
    ExtensionRegistryLite.setLazyExtensionMode(originalMode);
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

    if (mode == LazyExtensionMode.LAZY_VERIFY_ON_ACCESS) {
      assertThrows(
          InvalidProtobufRuntimeException.class,
          () -> messageSet.getExtension(TestMessageSetExtension1.messageSetExtension));
      return;
    }

    assertThat(messageSet.getExtension(TestMessageSetExtension1.messageSetExtension))
        .isEqualTo(TestMessageSetExtension1.getDefaultInstance());

    // Serialize. The first extension should be serialized as an empty message.
    ByteString outputData = messageSet.toByteString();

    // Round trip and confirm the corrupted payload is preserved.
    RawMessageSet actualRaw =
        RawMessageSet.parseFrom(outputData, ExtensionRegistry.getEmptyRegistry());
    assertThat(actualRaw).isEqualTo(inputRaw);
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

    TestMessageSetWireFormatContainer container =
        TestMessageSetWireFormatContainer.newBuilder().setMessageSet(messageSet).build();

    // Effectively cache the serialized size of the message set.
    assertThat(container.getSerializedSize()).isEqualTo(11);

    if (mode == LazyExtensionMode.LAZY_VERIFY_ON_ACCESS) {
      assertThrows(
          InvalidProtobufRuntimeException.class,
          () ->
              container.getMessageSet().getExtension(TestMessageSetExtension1.messageSetExtension));
      return;
    }

    // getExtension will notice that the extension is corrupted and replace it with the default
    // empty message.
    assertThat(container.getMessageSet().getExtension(TestMessageSetExtension1.messageSetExtension))
        .isEqualTo(TestMessageSetExtension1.getDefaultInstance());

    // Make sure that toByteString() works even though the total size has been cached, and round
    // tripping should keep equals().
    ByteString bytes = container.toByteString();
    assertThat(container)
        .isEqualTo(TestMessageSetWireFormatContainer.parseFrom(bytes, extensionRegistry));
  }

  private ByteString makeRecursivePayload(int depth) throws Exception {
    ByteString payload = ByteString.copyFromUtf8("payload");
    for (int i = 0; i < depth; ++i) {
      ByteString.Output extOut = ByteString.newOutput();
      CodedOutputStream extCout = CodedOutputStream.newInstance(extOut);
      // Write to the `recursive` field.
      extCout.writeBytes(16, payload);
      extCout.flush();
      ByteString extPayload = extOut.toByteString();

      ByteString.Output out = ByteString.newOutput();
      CodedOutputStream cout = CodedOutputStream.newInstance(out);

      // Item 1: normal order, ID first
      cout.writeTag(1, WireFormat.WIRETYPE_START_GROUP);
      cout.writeUInt32(2, TestMessageSetExtension1.messageSetExtension.getNumber());
      cout.writeBytes(3, ByteString.EMPTY);
      cout.writeTag(1, WireFormat.WIRETYPE_END_GROUP);

      // Item 2: reversed order, payload first
      cout.writeTag(1, WireFormat.WIRETYPE_START_GROUP);
      cout.writeBytes(3, extPayload);
      cout.writeUInt32(2, TestMessageSetExtension1.messageSetExtension.getNumber());
      cout.writeTag(1, WireFormat.WIRETYPE_END_GROUP);

      cout.flush();
      payload = out.toByteString();
    }
    return payload;
  }

  private void testMessageSetRecursionLimit(boolean eager) throws Exception {
    boolean originalEagerlyParse = ExtensionRegistryLite.isEagerlyParseMessageSets();
    ExtensionRegistryLite.setEagerlyParseMessageSets(eager);
    try {
      ByteString payload = makeRecursivePayload(2000);

      ExtensionRegistry registry = ExtensionRegistry.newInstance();
      registry.add(TestMessageSetExtension1.messageSetExtension);

      if (!eager) {
        // Item 2 is concatenated to the lazy Item 1 without parsing either, so the nested payload
        // is only parsed, one level at a time, when it is accessed.
        TestMessageSet unused = TestMessageSet.parseFrom(payload, registry);
        return;
      }
      Throwable exception =
          assertThrows(
              InvalidProtocolBufferException.class,
              () -> TestMessageSet.parseFrom(payload, registry));
      assertThat(exception).hasMessageThat().contains("too many levels of nesting");
    } finally {
      ExtensionRegistryLite.setEagerlyParseMessageSets(originalEagerlyParse);
    }
  }

  @Test
  public void testRecursionLimit_eagerMessageSet() throws Exception {
    testMessageSetRecursionLimit(/* eager= */ true);
  }

  @Test
  public void testRecursionLimit_lazyMessageSet() throws Exception {
    testMessageSetRecursionLimit(/* eager= */ false);
  }

  // Returns a TestMessageSet nested `depth` times. Each level has a TestMessageSetExtension1 item,
  // whose `recursive` field holds the next level, followed by `duplicate`, which is merged into
  // that (lazily parsed) item.
  private static ByteString makeRecursiveLazyPayload(int depth, ByteString duplicate)
      throws IOException {
    ByteString payload = ByteString.EMPTY;
    for (int i = 0; i < depth; ++i) {
      ByteString.Output extension = ByteString.newOutput();
      CodedOutputStream extensionOutput = CodedOutputStream.newInstance(extension);
      extensionOutput.writeBytes(16, payload); // TestMessageSetExtension1.recursive
      extensionOutput.flush();
      payload = messageSetItem(extension.toByteString()).concat(duplicate);
    }
    return payload;
  }

  private static ByteString messageSetItem(ByteString message) {
    return RawMessageSet.newBuilder()
        .addItem(RawMessageSet.Item.newBuilder().setTypeId(TYPE_ID_1).setMessage(message))
        .build()
        .toByteString();
  }

  // Returns the TestMessageSetExtension1 extension encoded as a regular field rather than as a
  // MessageSet item.
  private static ByteString extensionField(ByteString message) throws IOException {
    ByteString.Output field = ByteString.newOutput();
    CodedOutputStream fieldOutput = CodedOutputStream.newInstance(field);
    fieldOutput.writeBytes(TYPE_ID_1, message);
    fieldOutput.flush();
    return field.toByteString();
  }

  // Checks that each level of the payload is parsed only when it is accessed.
  private static void assertParsesLevelByLevel(int depth, ByteString duplicate) throws Exception {
    ExtensionRegistry registry = ExtensionRegistry.newInstance();
    registry.add(TestMessageSetExtension1.messageSetExtension);
    TestMessageSet message =
        TestMessageSet.parseFrom(makeRecursiveLazyPayload(depth, duplicate), registry);

    int levels = 0;
    while (message.hasExtension(TestMessageSetExtension1.messageSetExtension)) {
      message = message.getExtension(TestMessageSetExtension1.messageSetExtension).getRecursive();
      levels++;
    }
    assertThat(levels).isEqualTo(depth);
  }

  @Test
  public void testRecursionLimit_duplicateMessageSetItem() throws Exception {
    assertParsesLevelByLevel(2000, messageSetItem(ByteString.EMPTY));
  }

  @Test
  public void testRecursionLimit_duplicateExtensionField() throws Exception {
    assertParsesLevelByLevel(2000, extensionField(ByteString.EMPTY));
  }

  @Test
  public void testRecursionLimit_dynamicMessage() throws Exception {
    Descriptors.FieldDescriptor extension =
        TestMessageSetExtension1.messageSetExtension.getDescriptor();
    Descriptors.FieldDescriptor recursive =
        TestMessageSetExtension1.getDescriptor().findFieldByName("recursive");
    // A DynamicMessage default instance makes the nested levels DynamicMessages too.
    ExtensionRegistry registry = ExtensionRegistry.newInstance();
    registry.add(
        extension, DynamicMessage.getDefaultInstance(TestMessageSetExtension1.getDescriptor()));
    for (ByteString duplicate :
        Arrays.asList(messageSetItem(ByteString.EMPTY), extensionField(ByteString.EMPTY))) {
      DynamicMessage message =
          DynamicMessage.parseFrom(
              TestMessageSet.getDescriptor(), makeRecursiveLazyPayload(2000, duplicate), registry);

      int depth = 0;
      while (message.hasField(extension)) {
        message = (DynamicMessage) ((Message) message.getField(extension)).getField(recursive);
        depth++;
      }
      assertThat(depth).isEqualTo(2000);
    }
  }
}
