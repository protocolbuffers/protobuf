// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

package com.google.protobuf;

import static com.google.common.truth.Truth.assertThat;
import static com.google.common.truth.Truth.assertWithMessage;

import com.google.protobuf.UnittestLite.TestAllExtensionsLite;
import com.google.protobuf.UnittestLite.TestAllTypesLite;
import com.google.protobuf.UnittestLite.TestMergeExceptionLite;
import com.google.protobuf.UnittestLite.TestPackedExtensionsLite;
import com.google.protobuf.UnittestLite.TestParsingMergeLite;
import protobuf_unittest.MapLiteUnittest;
import protobuf_unittest.MapLiteUnittest.TestRequiredLite;
import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.InputStream;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

@RunWith(JUnit4.class)
public class ParserLiteTest {

  private void assertRoundTripEquals(MessageLite message, ExtensionRegistryLite registry)
      throws Exception {
    final byte[] data = message.toByteArray();
    final int offset = 20;
    final int length = data.length;
    final int padding = 30;
    Parser<? extends MessageLite> parser = message.getParserForType();
    assertThat(message).isEqualTo(parser.parseFrom(data, registry));
    assertThat(message)
        .isEqualTo(
            parser.parseFrom(
                generatePaddingArray(data, offset, padding), offset, length, registry));
    assertThat(message).isEqualTo(parser.parseFrom(message.toByteString(), registry));
    assertThat(message).isEqualTo(parser.parseFrom(new ByteArrayInputStream(data), registry));
    assertThat(message).isEqualTo(parser.parseFrom(CodedInputStream.newInstance(data), registry));
    assertThat(message)
        .isEqualTo(parser.parseFrom(message.toByteString().asReadOnlyByteBuffer(), registry));
  }

  @SuppressWarnings("unchecked")
  private void assertRoundTripEquals(MessageLite message) throws Exception {
    final byte[] data = message.toByteArray();
    final int offset = 20;
    final int length = data.length;
    final int padding = 30;

    Parser<MessageLite> parser = (Parser<MessageLite>) message.getParserForType();
    assertThat(message).isEqualTo(parser.parseFrom(data));
    assertThat(message)
        .isEqualTo(parser.parseFrom(generatePaddingArray(data, offset, padding), offset, length));
    assertThat(message).isEqualTo(parser.parseFrom(message.toByteString()));
    assertThat(message).isEqualTo(parser.parseFrom(new ByteArrayInputStream(data)));
    assertThat(message).isEqualTo(parser.parseFrom(CodedInputStream.newInstance(data)));
    assertThat(message).isEqualTo(parser.parseFrom(message.toByteString().asReadOnlyByteBuffer()));
  }

  private byte[] generatePaddingArray(byte[] data, int offset, int padding) {
    byte[] result = new byte[offset + data.length + padding];
    System.arraycopy(data, 0, result, offset, data.length);
    return result;
  }

  @Test
  public void testParseExtensionsLite() throws Exception {
    assertRoundTripEquals(
        TestUtilLite.getAllLiteExtensionsSet(), TestUtilLite.getExtensionRegistryLite());
  }

  @Test
  public void testParsePacked() throws Exception {
    assertRoundTripEquals(TestUtil.getPackedSet());
    assertRoundTripEquals(TestUtil.getPackedExtensionsSet(), TestUtil.getExtensionRegistry());
  }

  @Test
  public void testParsePackedLite() throws Exception {
    assertRoundTripEquals(
        TestUtilLite.getLitePackedExtensionsSet(), TestUtilLite.getExtensionRegistryLite());
  }

  @Test
  public void testParseDelimitedToLite() throws Exception {
    // Write MessageLite with packed extension fields.
    TestPackedExtensionsLite packedMessage = TestUtilLite.getLitePackedExtensionsSet();
    ByteArrayOutputStream output = new ByteArrayOutputStream();
    packedMessage.writeDelimitedTo(output);
    packedMessage.writeDelimitedTo(output);

    InputStream input = new ByteArrayInputStream(output.toByteArray());
    assertThat(packedMessage)
        .isEqualTo(
            packedMessage
                .getParserForType()
                .parseDelimitedFrom(input, TestUtilLite.getExtensionRegistryLite()));
    assertThat(packedMessage)
        .isEqualTo(
            packedMessage
                .getParserForType()
                .parseDelimitedFrom(input, TestUtilLite.getExtensionRegistryLite()));
  }

  /** Helper method for {@link #testParsingMergeLite()}. */
  private void assertMessageMerged(TestAllTypesLite allTypes) throws Exception {
    assertThat(allTypes.getOptionalInt32()).isEqualTo(3);
    assertThat(allTypes.getOptionalInt64()).isEqualTo(2);
    assertThat(allTypes.getOptionalString()).isEqualTo("hello");
  }

  @Test
  public void testParsingMergeLite() throws Exception {
    // Build messages.
    TestAllTypesLite.Builder builder = TestAllTypesLite.newBuilder();
    TestAllTypesLite msg1 = builder.setOptionalInt32(1).build();
    builder.clear();
    TestAllTypesLite msg2 = builder.setOptionalInt64(2).build();
    builder.clear();
    TestAllTypesLite msg3 = builder.setOptionalInt32(3).setOptionalString("hello").build();

    // Build groups.
    TestParsingMergeLite.RepeatedFieldsGenerator.Group1 optionalG1 =
        TestParsingMergeLite.RepeatedFieldsGenerator.Group1.newBuilder().setField1(msg1).build();
    TestParsingMergeLite.RepeatedFieldsGenerator.Group1 optionalG2 =
        TestParsingMergeLite.RepeatedFieldsGenerator.Group1.newBuilder().setField1(msg2).build();
    TestParsingMergeLite.RepeatedFieldsGenerator.Group1 optionalG3 =
        TestParsingMergeLite.RepeatedFieldsGenerator.Group1.newBuilder().setField1(msg3).build();
    TestParsingMergeLite.RepeatedFieldsGenerator.Group2 repeatedG1 =
        TestParsingMergeLite.RepeatedFieldsGenerator.Group2.newBuilder().setField1(msg1).build();
    TestParsingMergeLite.RepeatedFieldsGenerator.Group2 repeatedG2 =
        TestParsingMergeLite.RepeatedFieldsGenerator.Group2.newBuilder().setField1(msg2).build();
    TestParsingMergeLite.RepeatedFieldsGenerator.Group2 repeatedG3 =
        TestParsingMergeLite.RepeatedFieldsGenerator.Group2.newBuilder().setField1(msg3).build();

    // Assign and serialize RepeatedFieldsGenerator.
    ByteString data =
        TestParsingMergeLite.RepeatedFieldsGenerator.newBuilder()
            .addField1(msg1)
            .addField1(msg2)
            .addField1(msg3)
            .addField2(msg1)
            .addField2(msg2)
            .addField2(msg3)
            .addField3(msg1)
            .addField3(msg2)
            .addField3(msg3)
            .addGroup1(optionalG1)
            .addGroup1(optionalG2)
            .addGroup1(optionalG3)
            .addGroup2(repeatedG1)
            .addGroup2(repeatedG2)
            .addGroup2(repeatedG3)
            .addExt1(msg1)
            .addExt1(msg2)
            .addExt1(msg3)
            .addExt2(msg1)
            .addExt2(msg2)
            .addExt2(msg3)
            .build()
            .toByteString();

    // Parse TestParsingMergeLite.
    ExtensionRegistryLite registry = ExtensionRegistryLite.newInstance();
    UnittestLite.registerAllExtensions(registry);
    TestParsingMergeLite parsingMerge = TestParsingMergeLite.parseFrom(data, registry);

    // Required and optional fields should be merged.
    assertMessageMerged(parsingMerge.getRequiredAllTypes());
    assertMessageMerged(parsingMerge.getOptionalAllTypes());
    assertMessageMerged(parsingMerge.getOptionalGroup().getOptionalGroupAllTypes());
    assertMessageMerged(parsingMerge.getExtension(TestParsingMergeLite.optionalExt));

    // Repeated fields should not be merged.
    assertThat(parsingMerge.getRepeatedAllTypesCount()).isEqualTo(3);
    assertThat(parsingMerge.getRepeatedGroupCount()).isEqualTo(3);
    assertThat(parsingMerge.getExtensionCount(TestParsingMergeLite.repeatedExt)).isEqualTo(3);
  }

  @Test
  public void testExceptionWhenMergingExtendedMessagesMissingRequiredFieldsLite() {
    // create a TestMergeExceptionLite message (missing required fields) that looks like
    //   all_extensions {
    //     [TestRequiredLite.single] {
    //     }
    //   }
    TestMergeExceptionLite.Builder message = TestMergeExceptionLite.newBuilder();
    message.setAllExtensions(
        TestAllExtensionsLite.newBuilder()
            .setExtension(TestRequiredLite.single, TestRequiredLite.newBuilder().buildPartial())
            .buildPartial());
    ByteString byteString = message.buildPartial().toByteString();

    // duplicate the bytestring to make the `all_extensions` field repeat twice, so that it will
    // need merging when parsing back
    ByteString duplicatedByteString = byteString.concat(byteString);

    byte[] bytes = duplicatedByteString.toByteArray();
    ExtensionRegistryLite registry = ExtensionRegistryLite.newInstance();
    MapLiteUnittest.registerAllExtensions(registry);

    // `parseFrom` should throw InvalidProtocolBufferException, not UninitializedMessageException,
    // for each of the 5 possible input types:

    // parseFrom(ByteString)
    try {
      TestMergeExceptionLite.parseFrom(duplicatedByteString, registry);
      assertWithMessage("Expected InvalidProtocolBufferException").fail();
    } catch (Exception e) {
      assertThat(e.getClass()).isEqualTo(InvalidProtocolBufferException.class);
    }

    // parseFrom(ByteArray)
    try {
      TestMergeExceptionLite.parseFrom(bytes, registry);
      assertWithMessage("Expected InvalidProtocolBufferException").fail();
    } catch (Exception e) {
      assertThat(e.getClass()).isEqualTo(InvalidProtocolBufferException.class);
    }

    // parseFrom(InputStream)
    try {
      TestMergeExceptionLite.parseFrom(new ByteArrayInputStream(bytes), registry);
      assertWithMessage("Expected InvalidProtocolBufferException").fail();
    } catch (Exception e) {
      assertThat(e.getClass()).isEqualTo(InvalidProtocolBufferException.class);
    }

    // parseFrom(CodedInputStream)
    try {
      TestMergeExceptionLite.parseFrom(CodedInputStream.newInstance(bytes), registry);
      assertWithMessage("Expected InvalidProtocolBufferException").fail();
    } catch (Exception e) {
      assertThat(e.getClass()).isEqualTo(InvalidProtocolBufferException.class);
    }

    // parseFrom(ByteBuffer)
    try {
      TestMergeExceptionLite.parseFrom(duplicatedByteString.asReadOnlyByteBuffer(), registry);
      assertWithMessage("Expected InvalidProtocolBufferException").fail();
    } catch (Exception e) {
      assertThat(e.getClass()).isEqualTo(InvalidProtocolBufferException.class);
    }
  }
}
