// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

package com.google.protobuf;

import static com.google.common.truth.Truth.assertThat;
import static com.google.common.truth.Truth.assertWithMessage;

import proto2_unittest.UnittestOptimizeFor;
import proto2_unittest.UnittestOptimizeFor.TestOptimizedForSize;
import proto2_unittest.UnittestOptimizeFor.TestRequiredOptimizedForSize;
import proto2_unittest.UnittestProto;
import proto2_unittest.UnittestProto.ForeignMessage;
import proto2_unittest.UnittestProto.TestAllTypes;
import proto2_unittest.UnittestProto.TestEmptyMessage;
import proto2_unittest.UnittestProto.TestMergeException;
import proto2_unittest.UnittestProto.TestParsingMerge;
import proto2_unittest.UnittestProto.TestRequired;
import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.InterruptedIOException;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

/** Unit test for {@link Parser}. */
@RunWith(JUnit4.class)
public class ParserTest {

  @Test
  public void testGeneratedMessageParserSingleton() throws Exception {
    for (int i = 0; i < 10; i++) {
      assertThat(TestUtil.getAllSet().getParserForType()).isEqualTo(TestAllTypes.parser());
    }
  }

  private void assertRoundTripEquals(MessageLite message, ExtensionRegistryLite registry)
      throws Exception {
    final byte[] data = message.toByteArray();
    final int offset = 20;
    final int length = data.length;
    final int padding = 30;
    Parser<? extends MessageLite> parser = message.getParserForType();
    assertMessageEquals(message, parser.parseFrom(data, registry));
    assertMessageEquals(
        message,
        parser.parseFrom(generatePaddingArray(data, offset, padding), offset, length, registry));
    assertMessageEquals(message, parser.parseFrom(message.toByteString(), registry));
    assertMessageEquals(message, parser.parseFrom(new ByteArrayInputStream(data), registry));
    assertMessageEquals(message, parser.parseFrom(CodedInputStream.newInstance(data), registry));
    assertMessageEquals(
        message, parser.parseFrom(message.toByteString().asReadOnlyByteBuffer(), registry));
  }

  @SuppressWarnings("unchecked")
  private void assertRoundTripEquals(MessageLite message) throws Exception {
    final byte[] data = message.toByteArray();
    final int offset = 20;
    final int length = data.length;
    final int padding = 30;

    Parser<MessageLite> parser = (Parser<MessageLite>) message.getParserForType();
    assertMessageEquals(message, parser.parseFrom(data));
    assertMessageEquals(
        message, parser.parseFrom(generatePaddingArray(data, offset, padding), offset, length));
    assertMessageEquals(message, parser.parseFrom(message.toByteString()));
    assertMessageEquals(message, parser.parseFrom(new ByteArrayInputStream(data)));
    assertMessageEquals(message, parser.parseFrom(CodedInputStream.newInstance(data)));
    assertMessageEquals(message, parser.parseFrom(message.toByteString().asReadOnlyByteBuffer()));
  }

  private void assertMessageEquals(MessageLite expected, MessageLite actual) throws Exception {
    if (expected instanceof Message) {
      assertThat(actual).isEqualTo(expected);
    } else {
      assertThat(actual.toByteString()).isEqualTo(expected.toByteString());
    }
  }

  private byte[] generatePaddingArray(byte[] data, int offset, int padding) {
    byte[] result = new byte[offset + data.length + padding];
    System.arraycopy(data, 0, result, offset, data.length);
    return result;
  }

  @Test
  public void testNormalMessage() throws Exception {
    assertRoundTripEquals(TestUtil.getAllSet());
  }

  @Test
  public void testParsePartial() throws Exception {
    assertParsePartial(TestRequired.parser(), TestRequired.newBuilder().setA(1).buildPartial());
  }

  private <T extends MessageLite> void assertParsePartial(Parser<T> parser, T partialMessage)
      throws Exception {
    final String errorString = "Should throw exceptions when the parsed message isn't initialized.";

    // parsePartialFrom should pass.
    byte[] data = partialMessage.toByteArray();
    assertThat(parser.parsePartialFrom(data)).isEqualTo(partialMessage);
    assertThat(parser.parsePartialFrom(partialMessage.toByteString())).isEqualTo(partialMessage);
    assertThat(parser.parsePartialFrom(new ByteArrayInputStream(data))).isEqualTo(partialMessage);
    assertThat(parser.parsePartialFrom(CodedInputStream.newInstance(data)))
        .isEqualTo(partialMessage);

    // parseFrom(ByteArray)
    try {
      parser.parseFrom(partialMessage.toByteArray());
      assertWithMessage(errorString).fail();
    } catch (InvalidProtocolBufferException e) {
      // pass.
    }

    // parseFrom(ByteString)
    try {
      parser.parseFrom(partialMessage.toByteString());
      assertWithMessage(errorString).fail();
    } catch (InvalidProtocolBufferException e) {
      // pass.
    }

    // parseFrom(InputStream)
    try {
      parser.parseFrom(new ByteArrayInputStream(partialMessage.toByteArray()));
      assertWithMessage(errorString).fail();
    } catch (IOException e) {
      // pass.
    }

    // parseFrom(CodedInputStream)
    try {
      parser.parseFrom(CodedInputStream.newInstance(partialMessage.toByteArray()));
      assertWithMessage(errorString).fail();
    } catch (IOException e) {
      // pass.
    }
  }

  @Test
  public void testParseExtensions() throws Exception {
    assertRoundTripEquals(TestUtil.getAllExtensionsSet(), TestUtil.getExtensionRegistry());
  }

  @Test
  public void testParsePacked() throws Exception {
    assertRoundTripEquals(TestUtil.getPackedSet());
    assertRoundTripEquals(TestUtil.getPackedExtensionsSet(), TestUtil.getExtensionRegistry());
  }

  @Test
  public void testParseDelimitedTo() throws Exception {
    // Write normal Message.
    TestAllTypes normalMessage = TestUtil.getAllSet();
    ByteArrayOutputStream output = new ByteArrayOutputStream();
    normalMessage.writeDelimitedTo(output);
    normalMessage.writeDelimitedTo(output);

    InputStream input = new ByteArrayInputStream(output.toByteArray());
    assertMessageEquals(normalMessage, normalMessage.getParserForType().parseDelimitedFrom(input));
    assertMessageEquals(normalMessage, normalMessage.getParserForType().parseDelimitedFrom(input));
  }

  @Test
  public void testParseUnknownFields() throws Exception {
    // All fields will be treated as unknown fields in emptyMessage.
    TestEmptyMessage emptyMessage = TestEmptyMessage.parseFrom(TestUtil.getAllSet().toByteString());
    assertThat(emptyMessage.toByteString()).isEqualTo(TestUtil.getAllSet().toByteString());
  }

  @Test
  public void testOptimizeForSize() throws Exception {
    TestOptimizedForSize.Builder builder = TestOptimizedForSize.newBuilder();
    builder.setI(12).setMsg(ForeignMessage.newBuilder().setC(34).build());
    builder.setExtension(TestOptimizedForSize.testExtension, 56);
    builder.setExtension(
        TestOptimizedForSize.testExtension2,
        TestRequiredOptimizedForSize.newBuilder().setX(78).build());

    TestOptimizedForSize message = builder.build();
    ExtensionRegistry registry = ExtensionRegistry.newInstance();
    UnittestOptimizeFor.registerAllExtensions(registry);

    assertRoundTripEquals(message, registry);
  }

  /** Helper method for {@link #testParsingMerge()}. */
  private void assertMessageMerged(TestAllTypes allTypes) throws Exception {
    assertThat(allTypes.getOptionalInt32()).isEqualTo(3);
    assertThat(allTypes.getOptionalInt64()).isEqualTo(2);
    assertThat(allTypes.getOptionalString()).isEqualTo("hello");
  }

  @Test
  public void testParsingMerge() throws Exception {
    // Build messages.
    TestAllTypes.Builder builder = TestAllTypes.newBuilder();
    TestAllTypes msg1 = builder.setOptionalInt32(1).build();
    builder.clear();
    TestAllTypes msg2 = builder.setOptionalInt64(2).build();
    builder.clear();
    TestAllTypes msg3 = builder.setOptionalInt32(3).setOptionalString("hello").build();

    // Build groups.
    TestParsingMerge.RepeatedFieldsGenerator.Group1 optionalG1 =
        TestParsingMerge.RepeatedFieldsGenerator.Group1.newBuilder().setField1(msg1).build();
    TestParsingMerge.RepeatedFieldsGenerator.Group1 optionalG2 =
        TestParsingMerge.RepeatedFieldsGenerator.Group1.newBuilder().setField1(msg2).build();
    TestParsingMerge.RepeatedFieldsGenerator.Group1 optionalG3 =
        TestParsingMerge.RepeatedFieldsGenerator.Group1.newBuilder().setField1(msg3).build();
    TestParsingMerge.RepeatedFieldsGenerator.Group2 repeatedG1 =
        TestParsingMerge.RepeatedFieldsGenerator.Group2.newBuilder().setField1(msg1).build();
    TestParsingMerge.RepeatedFieldsGenerator.Group2 repeatedG2 =
        TestParsingMerge.RepeatedFieldsGenerator.Group2.newBuilder().setField1(msg2).build();
    TestParsingMerge.RepeatedFieldsGenerator.Group2 repeatedG3 =
        TestParsingMerge.RepeatedFieldsGenerator.Group2.newBuilder().setField1(msg3).build();

    // Assign and serialize RepeatedFieldsGenerator.
    ByteString data =
        TestParsingMerge.RepeatedFieldsGenerator.newBuilder()
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

    // Parse TestParsingMerge.
    ExtensionRegistry registry = ExtensionRegistry.newInstance();
    UnittestProto.registerAllExtensions(registry);
    TestParsingMerge parsingMerge = TestParsingMerge.parseFrom(data, registry);

    // Required and optional fields should be merged.
    assertMessageMerged(parsingMerge.getRequiredAllTypes());
    assertMessageMerged(parsingMerge.getOptionalAllTypes());
    assertMessageMerged(parsingMerge.getOptionalGroup().getOptionalGroupAllTypes());
    assertMessageMerged(parsingMerge.getExtension(TestParsingMerge.optionalExt));

    // Repeated fields should not be merged.
    assertThat(parsingMerge.getRepeatedAllTypesCount()).isEqualTo(3);
    assertThat(parsingMerge.getRepeatedGroupCount()).isEqualTo(3);
    assertThat(parsingMerge.getExtensionCount(TestParsingMerge.repeatedExt)).isEqualTo(3);
  }

  @Test
  public void testExceptionWhenMergingExtendedMessagesMissingRequiredFields() {
    // create a TestMergeException message (missing required fields) that looks like
    //   all_extensions {
    //     [TestRequired.single] {
    //     }
    //   }
    TestMergeException.Builder message = TestMergeException.newBuilder();
    message
        .getAllExtensionsBuilder()
        .setExtension(TestRequired.single, TestRequired.newBuilder().buildPartial());
    ByteString byteString = message.buildPartial().toByteString();

    // duplicate the bytestring to make the `all_extensions` field repeat twice, so that it will
    // need merging when parsing back
    ByteString duplicatedByteString = byteString.concat(byteString);

    byte[] bytes = duplicatedByteString.toByteArray();
    ExtensionRegistry registry = ExtensionRegistry.newInstance();
    UnittestProto.registerAllExtensions(registry);

    // `parseFrom` should throw InvalidProtocolBufferException, not UninitializedMessageException,
    // for each of the 5 possible input types:

    // parseFrom(ByteString)
    try {
      TestMergeException.parseFrom(duplicatedByteString, registry);
      assertWithMessage("Expected InvalidProtocolBufferException").fail();
    } catch (Exception e) {
      assertThat(e.getClass()).isEqualTo(InvalidProtocolBufferException.class);
    }

    // parseFrom(ByteArray)
    try {
      TestMergeException.parseFrom(bytes, registry);
      assertWithMessage("Expected InvalidProtocolBufferException").fail();
    } catch (Exception e) {
      assertThat(e.getClass()).isEqualTo(InvalidProtocolBufferException.class);
    }

    // parseFrom(InputStream)
    try {
      TestMergeException.parseFrom(new ByteArrayInputStream(bytes), registry);
      assertWithMessage("Expected InvalidProtocolBufferException").fail();
    } catch (Exception e) {
      assertThat(e.getClass()).isEqualTo(InvalidProtocolBufferException.class);
    }

    // parseFrom(CodedInputStream)
    try {
      TestMergeException.parseFrom(CodedInputStream.newInstance(bytes), registry);
      assertWithMessage("Expected InvalidProtocolBufferException").fail();
    } catch (Exception e) {
      assertThat(e.getClass()).isEqualTo(InvalidProtocolBufferException.class);
    }

    // parseFrom(ByteBuffer)
    try {
      TestMergeException.parseFrom(duplicatedByteString.asReadOnlyByteBuffer(), registry);
      assertWithMessage("Expected InvalidProtocolBufferException").fail();
    } catch (Exception e) {
      assertThat(e.getClass()).isEqualTo(InvalidProtocolBufferException.class);
    }
  }

  @Test
  public void testParseDelimitedFrom_firstByteInterrupted_preservesCause() {
    try {
      TestAllTypes.parseDelimitedFrom(
          new InputStream() {
            @Override
            public int read() throws IOException {
              throw new InterruptedIOException();
            }
          });
      assertWithMessage("Expected InterruptedIOException").fail();
    } catch (Exception e) {
      assertThat(e.getClass()).isEqualTo(InterruptedIOException.class);
    }
  }

  @Test
  public void testParseDelimitedFrom_secondByteInterrupted_preservesCause() {
    try {
      TestAllTypes.parseDelimitedFrom(
          new InputStream() {
            private int i;

            @Override
            public int read() throws IOException {
              switch (i++) {
                case 0:
                  return 1;
                case 1:
                  throw new InterruptedIOException();
                default:
                  throw new AssertionError();
              }
            }
          });
      assertWithMessage("Expected InterruptedIOException").fail();
    } catch (Exception e) {
      assertThat(e.getClass()).isEqualTo(InterruptedIOException.class);
    }
  }
}
