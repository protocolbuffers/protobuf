package com.google.protobuf;

import static com.google.common.truth.Truth.assertThat;
import static org.junit.Assert.assertThrows;

import proto2_unittest.UnittestProto.TestAllExtensions;
import proto2_unittest.UnittestProto.TestAllTypes;
import proto2_unittest.UnittestProto.TestParsingMerge;
import proto2_unittest.UnittestProto.TestRecursiveMessage;
import proto2_unittest.UnittestProto.TestRequired;
import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

@RunWith(JUnit4.class)
public final class LazyExtensionFieldsTest {

  private ExtensionRegistryLite.LazyExtensionFieldsExperimentMode lazyExtensionFieldsExperimentMode;
  private final ExtensionRegistryLite extensionRegistry = ExtensionRegistryLite.newInstance();

  @Before
  public void setUp() {
    lazyExtensionFieldsExperimentMode =
        ExtensionRegistryLite.getLazyExtensionFieldsExperimentMode();
    ExtensionRegistryLite.setLazyExtensionFieldsExperimentMode(
        ExtensionRegistryLite.LazyExtensionFieldsExperimentMode.VERIFIED_LAZY);
    extensionRegistry.add(TestRequired.single);
    extensionRegistry.add(TestParsingMerge.optionalExt);
    extensionRegistry.add(TestParsingMerge.recursiveMessage);
    extensionRegistry.add(TestParsingMerge.allExtensions);
    extensionRegistry.add(TestParsingMerge.recursiveMessageWithRequiredFields);
    extensionRegistry.add(TestParsingMerge.hugeFieldNumbers);
  }

  @After
  public void tearDown() {
    ExtensionRegistryLite.setLazyExtensionFieldsExperimentMode(lazyExtensionFieldsExperimentMode);
  }

  private byte[] bytes(int... bytesAsInts) {
    byte[] bytes = new byte[bytesAsInts.length];
    for (int i = 0; i < bytesAsInts.length; i++) {
      bytes[i] = (byte) bytesAsInts[i];
    }
    return bytes;
  }

  private byte[] createFieldData(int fieldNumber, int wireType, byte[] data) throws Exception {
    ByteString.Output rawOutput = ByteString.newOutput();
    CodedOutputStream outputStream = CodedOutputStream.newInstance(rawOutput);
    outputStream.writeTag(fieldNumber, wireType);
    if (wireType == WireFormat.WIRETYPE_LENGTH_DELIMITED) {
      outputStream.writeUInt32NoTag(data.length);
    }
    outputStream.writeRawBytes(data);
    outputStream.flush();
    return rawOutput.toByteString().toByteArray();
  }

  @Test
  public void testAllFieldsSet() throws Exception {
    TestAllTypes allTypes = TestUtil.getAllSet();
    TestParsingMerge message =
        TestParsingMerge.newBuilder()
            .setRequiredAllTypes(allTypes)
            .setExtension(TestParsingMerge.optionalExt, allTypes)
            .build();
    byte[] data = message.toByteArray();

    TestParsingMerge proto = TestParsingMerge.parseFrom(data, extensionRegistry);

    assertThat(proto.getUnknownFields().isEmpty()).isTrue();
    assertThat(data).hasLength(proto.toByteArray().length);
    assertThat(proto.getExtension(TestParsingMerge.optionalExt)).isEqualTo(allTypes);
  }

  @Test
  public void testMergeAndBuildMissingRequiredField() throws Exception {
    TestAllExtensions.Builder message =
        TestAllExtensions.newBuilder()
            .setExtension(TestRequired.single, TestRequired.getDefaultInstance());
    byte[] data = message.buildPartial().toByteArray();
    TestAllExtensions.Builder proto =
        TestAllExtensions.newBuilder().mergeFrom(data, extensionRegistry);

    UninitializedMessageException throwable =
        assertThrows(UninitializedMessageException.class, () -> proto.build());

    assertThat(throwable).hasMessageThat().contains("Message missing required fields:");
  }

  @Test
  public void testParseMissingRequiredField() throws Exception {
    TestAllExtensions.Builder message =
        TestAllExtensions.newBuilder()
            .setExtension(TestRequired.single, TestRequired.getDefaultInstance());
    byte[] data = message.buildPartial().toByteArray();

    InvalidProtocolBufferException throwable =
        assertThrows(
            InvalidProtocolBufferException.class,
            () -> TestAllExtensions.parseFrom(data, extensionRegistry));

    assertThat(throwable).hasMessageThat().contains("Message missing required fields:");
  }

  @Test
  public void testBuildAndParsePartial() throws Exception {
    TestAllExtensions.Builder message =
        TestAllExtensions.newBuilder()
            .setExtension(TestRequired.single, TestRequired.newBuilder().setA(1).buildPartial());
    byte[] data = message.buildPartial().toByteArray();
    TestAllExtensions proto = TestAllExtensions.parser().parsePartialFrom(data, extensionRegistry);

    assertThat(data).isNotEmpty();
    assertThat(message.buildPartial().getExtension(TestRequired.single).getA()).isEqualTo(1);
    assertThat(proto.getExtension(TestRequired.single).getA()).isEqualTo(1);
  }

  @Test
  public void testInvalidVarint() throws Exception {
    byte[] fieldData =
        createFieldData(
            1,
            WireFormat.WIRETYPE_VARINT,
            bytes(0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x00));
    byte[] extData = createFieldData(1000, WireFormat.WIRETYPE_LENGTH_DELIMITED, fieldData);

    TestParsingMerge.Builder proto = TestParsingMerge.newBuilder();

    InvalidProtocolBufferException throwable =
        assertThrows(
            InvalidProtocolBufferException.class,
            () -> proto.mergeFrom(extData, extensionRegistry));
    assertThat(throwable).hasMessageThat().contains("malformed varint");
  }

  @Test
  public void testInvalidWireType() throws Exception {
    // 7 is an invalid wire type.
    byte[] fieldData = createFieldData(1, 7, bytes(0x01));
    byte[] extData = createFieldData(1000, WireFormat.WIRETYPE_LENGTH_DELIMITED, fieldData);
    TestParsingMerge.Builder proto = TestParsingMerge.newBuilder();

    InvalidProtocolBufferException throwable =
        assertThrows(
            InvalidProtocolBufferException.class,
            () -> proto.mergeFrom(extData, extensionRegistry));

    assertThat(throwable).hasMessageThat().contains("invalid wire type");
  }

  @Test
  public void invalidWireTypeInNestedMessage() throws Exception {
    byte[] nestedFieldData =
        createFieldData(
            1,
            WireFormat.WIRETYPE_VARINT,
            bytes(0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x00));
    byte[] nestedData = createFieldData(18, WireFormat.WIRETYPE_LENGTH_DELIMITED, nestedFieldData);
    byte[] data = createFieldData(1000, WireFormat.WIRETYPE_LENGTH_DELIMITED, nestedData);

    TestParsingMerge.Builder proto = TestParsingMerge.newBuilder();

    InvalidProtocolBufferException throwable =
        assertThrows(
            InvalidProtocolBufferException.class, () -> proto.mergeFrom(data, extensionRegistry));

    assertThat(throwable).hasMessageThat().contains("malformed varint");
  }

  @Test
  public void missingEndGroupTag() throws Exception {
    ByteString.Output fieldRawOutput = ByteString.newOutput();
    CodedOutputStream fieldOutput = CodedOutputStream.newInstance(fieldRawOutput);
    fieldOutput.writeTag(16, WireFormat.WIRETYPE_START_GROUP);
    fieldOutput.writeInt32(17, 123);
    fieldOutput.flush();
    byte[] fieldData = fieldRawOutput.toByteString().toByteArray();
    byte[] data = createFieldData(1000, WireFormat.WIRETYPE_LENGTH_DELIMITED, fieldData);
    TestParsingMerge.Builder proto = TestParsingMerge.newBuilder();

    InvalidProtocolBufferException throwable =
        assertThrows(
            InvalidProtocolBufferException.class, () -> proto.mergeFrom(data, extensionRegistry));
    assertThat(throwable).hasMessageThat().contains("end-group tag did not match expected tag");
  }

  @Test
  public void invalidWireTypeInNestedGroup() throws Exception {
    byte[] fieldData =
        createFieldData(
            17,
            WireFormat.WIRETYPE_VARINT,
            bytes(0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x00));
    ByteString.Output groupRawOutput = ByteString.newOutput();
    CodedOutputStream groupOutput = CodedOutputStream.newInstance(groupRawOutput);
    groupOutput.writeTag(16, WireFormat.WIRETYPE_START_GROUP);
    groupOutput.writeRawBytes(fieldData);
    groupOutput.writeTag(16, WireFormat.WIRETYPE_END_GROUP);
    groupOutput.flush();
    byte[] groupData = groupRawOutput.toByteString().toByteArray();
    byte[] data = createFieldData(1000, WireFormat.WIRETYPE_LENGTH_DELIMITED, groupData);

    TestParsingMerge.Builder proto = TestParsingMerge.newBuilder();

    InvalidProtocolBufferException throwable =
        assertThrows(
            InvalidProtocolBufferException.class, () -> proto.mergeFrom(data, extensionRegistry));

    assertThat(throwable).hasMessageThat().contains("malformed varint");
  }

  private TestRecursiveMessage makeRecursiveMessage(int depth) {
    if (depth == 0) {
      return TestRecursiveMessage.newBuilder().setI(5).build();
    } else {
      return TestRecursiveMessage.newBuilder()
          .setI(5100)
          .setA(makeRecursiveMessage(depth - 1))
          .build();
    }
  }

  @Test
  public void testTooManyNestedMessages() throws Exception {
    byte[] recursiveMessageData = makeRecursiveMessage(101).toByteArray();
    byte[] data = createFieldData(1002, WireFormat.WIRETYPE_LENGTH_DELIMITED, recursiveMessageData);
    TestParsingMerge.Builder proto = TestParsingMerge.newBuilder();

    InvalidProtocolBufferException throwable =
        assertThrows(
            InvalidProtocolBufferException.class, () -> proto.mergeFrom(data, extensionRegistry));

    assertThat(throwable).hasMessageThat().contains("message had too many levels of nesting");
  }

  @Test
  public void testInvalidUtf8String() throws Exception {
    byte[] fieldData =
        createFieldData(
            536870019, WireFormat.WIRETYPE_LENGTH_DELIMITED, bytes(0xFF, 0xFF, 0xFF, 0xFF));
    byte[] extDataTop = createFieldData(1003, WireFormat.WIRETYPE_LENGTH_DELIMITED, fieldData);
    TestParsingMerge.Builder proto = TestParsingMerge.newBuilder();

    InvalidProtocolBufferException throwable =
        assertThrows(
            InvalidProtocolBufferException.class,
            () -> proto.mergeFrom(extDataTop, extensionRegistry));

    assertThat(throwable).hasMessageThat().contains("invalid UTF-8");
  }

  @Test
  public void testUnknownLengthDelimitedInMessageExtensionField() throws Exception {
    // We cannot validate this because we don't know whether the field is a string, bytes, or
    // message.
    byte[] bytes = bytes(0xFF, 0xFF, 0xFF, 0xFF);
    byte[] fieldData = createFieldData(99999, WireFormat.WIRETYPE_LENGTH_DELIMITED, bytes);
    byte[] extDataTop = createFieldData(1002, WireFormat.WIRETYPE_LENGTH_DELIMITED, fieldData);

    TestParsingMerge proto =
        TestParsingMerge.newBuilder().mergeFrom(extDataTop, extensionRegistry).buildPartial();

    assertThat(proto.hasExtension(TestParsingMerge.recursiveMessage)).isTrue();
    assertThat(
            proto
                .getExtension(TestParsingMerge.recursiveMessage)
                .getUnknownFields()
                .getField(99999)
                .getLengthDelimitedList()
                .get(0))
        .isEqualTo(ByteString.copyFrom(bytes));
  }

  @Test
  public void testMalformedDataInMessageExtensionOfMessageExtension() throws Exception {
    byte[] fieldData =
        createFieldData(
            1,
            WireFormat.WIRETYPE_VARINT,
            bytes(0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x00));
    byte[] data = createFieldData(1004, WireFormat.WIRETYPE_LENGTH_DELIMITED, fieldData);
    TestParsingMerge.Builder proto = TestParsingMerge.newBuilder();

    InvalidProtocolBufferException throwable =
        assertThrows(
            InvalidProtocolBufferException.class, () -> proto.mergeFrom(data, extensionRegistry));

    assertThat(throwable).hasMessageThat().contains("malformed varint");
  }

  @Test
  public void testUnknownMalfromedDataInMessageExtensionOfMessageExtension() throws Exception {
    byte[] bytes = bytes(0xFF, 0xFF, 0xFF, 0xFF);
    byte[] fieldData = createFieldData(1, WireFormat.WIRETYPE_LENGTH_DELIMITED, bytes);
    byte[] extDataTop = createFieldData(1004, WireFormat.WIRETYPE_LENGTH_DELIMITED, fieldData);
    ExtensionRegistry extensionRegistry = ExtensionRegistry.newInstance();
    extensionRegistry.add(TestParsingMerge.allExtensions);

    TestParsingMerge proto =
        TestParsingMerge.newBuilder().mergeFrom(extDataTop, extensionRegistry).buildPartial();
    assertThat(
            proto
                .getExtension(TestParsingMerge.allExtensions)
                .getUnknownFields()
                .getField(1)
                .getLengthDelimitedList()
                .get(0))
        .isEqualTo(ByteString.copyFrom(bytes));
  }
}
