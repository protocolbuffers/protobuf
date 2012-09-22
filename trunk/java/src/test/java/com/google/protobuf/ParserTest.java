// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
// http://code.google.com/p/protobuf/
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

package com.google.protobuf;

import com.google.protobuf.UnittestLite.TestAllTypesLite;
import com.google.protobuf.UnittestLite.TestPackedExtensionsLite;
import com.google.protobuf.UnittestLite.TestParsingMergeLite;
import com.google.protobuf.UnittestLite;
import protobuf_unittest.UnittestOptimizeFor.TestOptimizedForSize;
import protobuf_unittest.UnittestOptimizeFor.TestRequiredOptimizedForSize;
import protobuf_unittest.UnittestOptimizeFor;
import protobuf_unittest.UnittestProto.ForeignMessage;
import protobuf_unittest.UnittestProto.TestAllTypes;
import protobuf_unittest.UnittestProto.TestEmptyMessage;
import protobuf_unittest.UnittestProto.TestRequired;
import protobuf_unittest.UnittestProto.TestParsingMerge;
import protobuf_unittest.UnittestProto;

import junit.framework.TestCase;

import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.InputStream;

/**
 * Unit test for {@link Parser}.
 *
 * @author liujisi@google.com (Pherl Liu)
 */
public class ParserTest extends TestCase {
  public void testGeneratedMessageParserSingleton() throws Exception {
    for (int i = 0; i < 10; i++) {
      assertEquals(TestAllTypes.PARSER,
                   TestUtil.getAllSet().getParserForType());
    }
  }

  private void assertRoundTripEquals(MessageLite message,
                                     ExtensionRegistryLite registry)
      throws Exception {
    final byte[] data = message.toByteArray();
    final int offset = 20;
    final int length = data.length;
    final int padding = 30;
    Parser<? extends MessageLite> parser = message.getParserForType();
    assertMessageEquals(message, parser.parseFrom(data, registry));
    assertMessageEquals(message, parser.parseFrom(
        generatePaddingArray(data, offset, padding),
        offset, length, registry));
    assertMessageEquals(message, parser.parseFrom(
        message.toByteString(), registry));
    assertMessageEquals(message, parser.parseFrom(
        new ByteArrayInputStream(data), registry));
    assertMessageEquals(message, parser.parseFrom(
        CodedInputStream.newInstance(data), registry));
  }

  private void assertRoundTripEquals(MessageLite message) throws Exception {
    final byte[] data = message.toByteArray();
    final int offset = 20;
    final int length = data.length;
    final int padding = 30;
    Parser<? extends MessageLite> parser = message.getParserForType();
    assertMessageEquals(message, parser.parseFrom(data));
    assertMessageEquals(message, parser.parseFrom(
        generatePaddingArray(data, offset, padding),
        offset, length));
    assertMessageEquals(message, parser.parseFrom(message.toByteString()));
    assertMessageEquals(message, parser.parseFrom(
        new ByteArrayInputStream(data)));
    assertMessageEquals(message, parser.parseFrom(
        CodedInputStream.newInstance(data)));
  }

  private void assertMessageEquals(MessageLite expected, MessageLite actual)
      throws Exception {
    if (expected instanceof Message) {
      assertEquals(expected, actual);
    } else {
      assertEquals(expected.toByteString(), actual.toByteString());
    }
  }

  private byte[] generatePaddingArray(byte[] data, int offset, int padding) {
    byte[] result = new byte[offset + data.length + padding];
    System.arraycopy(data, 0, result, offset, data.length);
    return result;
  }

  public void testNormalMessage() throws Exception {
    assertRoundTripEquals(TestUtil.getAllSet());
  }

  public void testParsePartial() throws Exception {
    Parser<TestRequired> parser = TestRequired.PARSER;
    final String errorString =
        "Should throw exceptions when the parsed message isn't initialized.";

    // TestRequired.b and TestRequired.c are not set.
    TestRequired partialMessage = TestRequired.newBuilder()
        .setA(1).buildPartial();

    // parsePartialFrom should pass.
    byte[] data = partialMessage.toByteArray();
    assertEquals(partialMessage, parser.parsePartialFrom(data));
    assertEquals(partialMessage, parser.parsePartialFrom(
        partialMessage.toByteString()));
    assertEquals(partialMessage, parser.parsePartialFrom(
        new ByteArrayInputStream(data)));
    assertEquals(partialMessage, parser.parsePartialFrom(
        CodedInputStream.newInstance(data)));

    // parseFrom(ByteArray)
    try {
      parser.parseFrom(partialMessage.toByteArray());
      fail(errorString);
    } catch (InvalidProtocolBufferException e) {
      // pass.
    }

    // parseFrom(ByteString)
    try {
      parser.parseFrom(partialMessage.toByteString());
      fail(errorString);
    } catch (InvalidProtocolBufferException e) {
      // pass.
    }

    // parseFrom(InputStream)
    try {
      parser.parseFrom(new ByteArrayInputStream(partialMessage.toByteArray()));
      fail(errorString);
    } catch (IOException e) {
      // pass.
    }

    // parseFrom(CodedInputStream)
    try {
      parser.parseFrom(CodedInputStream.newInstance(
          partialMessage.toByteArray()));
      fail(errorString);
    } catch (IOException e) {
      // pass.
    }
  }

  public void testParseExtensions() throws Exception {
    assertRoundTripEquals(TestUtil.getAllExtensionsSet(),
                          TestUtil.getExtensionRegistry());
    assertRoundTripEquals(TestUtil.getAllLiteExtensionsSet(),
                          TestUtil.getExtensionRegistryLite());
  }

  public void testParsePacked() throws Exception {
    assertRoundTripEquals(TestUtil.getPackedSet());
    assertRoundTripEquals(TestUtil.getPackedExtensionsSet(),
                          TestUtil.getExtensionRegistry());
    assertRoundTripEquals(TestUtil.getLitePackedExtensionsSet(),
                          TestUtil.getExtensionRegistryLite());
  }

  public void testParseDelimitedTo() throws Exception {
    // Write normal Message.
    TestAllTypes normalMessage = TestUtil.getAllSet();
    ByteArrayOutputStream output = new ByteArrayOutputStream();
    normalMessage.writeDelimitedTo(output);

    // Write MessageLite with packed extension fields.
    TestPackedExtensionsLite packedMessage =
        TestUtil.getLitePackedExtensionsSet();
    packedMessage.writeDelimitedTo(output);

    InputStream input = new ByteArrayInputStream(output.toByteArray());
    assertMessageEquals(
        normalMessage,
        normalMessage.getParserForType().parseDelimitedFrom(input));
    assertMessageEquals(
        packedMessage,
        packedMessage.getParserForType().parseDelimitedFrom(
            input, TestUtil.getExtensionRegistryLite()));
  }

  public void testParseUnknownFields() throws Exception {
    // All fields will be treated as unknown fields in emptyMessage.
    TestEmptyMessage emptyMessage = TestEmptyMessage.PARSER.parseFrom(
        TestUtil.getAllSet().toByteString());
    assertEquals(
        TestUtil.getAllSet().toByteString(),
        emptyMessage.toByteString());
  }

  public void testOptimizeForSize() throws Exception {
    TestOptimizedForSize.Builder builder = TestOptimizedForSize.newBuilder();
    builder.setI(12).setMsg(ForeignMessage.newBuilder().setC(34).build());
    builder.setExtension(TestOptimizedForSize.testExtension, 56);
    builder.setExtension(TestOptimizedForSize.testExtension2,
        TestRequiredOptimizedForSize.newBuilder().setX(78).build());

    TestOptimizedForSize message = builder.build();
    ExtensionRegistry registry = ExtensionRegistry.newInstance();
    UnittestOptimizeFor.registerAllExtensions(registry);

    assertRoundTripEquals(message, registry);
  }

  /** Helper method for {@link #testParsingMerge()}.*/
  private void assertMessageMerged(TestAllTypes allTypes)
      throws Exception {
    assertEquals(3, allTypes.getOptionalInt32());
    assertEquals(2, allTypes.getOptionalInt64());
    assertEquals("hello", allTypes.getOptionalString());
  }

  /** Helper method for {@link #testParsingMergeLite()}.*/
  private void assertMessageMerged(TestAllTypesLite allTypes)
      throws Exception {
    assertEquals(3, allTypes.getOptionalInt32());
    assertEquals(2, allTypes.getOptionalInt64());
    assertEquals("hello", allTypes.getOptionalString());
  }

  public void testParsingMerge() throws Exception {
    // Build messages.
    TestAllTypes.Builder builder = TestAllTypes.newBuilder();
    TestAllTypes msg1 = builder.setOptionalInt32(1).build();
    builder.clear();
    TestAllTypes msg2 = builder.setOptionalInt64(2).build();
    builder.clear();
    TestAllTypes msg3 = builder.setOptionalInt32(3)
        .setOptionalString("hello").build();

    // Build groups.
    TestParsingMerge.RepeatedFieldsGenerator.Group1 optionalG1 =
        TestParsingMerge.RepeatedFieldsGenerator.Group1.newBuilder()
        .setField1(msg1).build();
    TestParsingMerge.RepeatedFieldsGenerator.Group1 optionalG2 =
        TestParsingMerge.RepeatedFieldsGenerator.Group1.newBuilder()
        .setField1(msg2).build();
    TestParsingMerge.RepeatedFieldsGenerator.Group1 optionalG3 =
        TestParsingMerge.RepeatedFieldsGenerator.Group1.newBuilder()
        .setField1(msg3).build();
    TestParsingMerge.RepeatedFieldsGenerator.Group2 repeatedG1 =
        TestParsingMerge.RepeatedFieldsGenerator.Group2.newBuilder()
        .setField1(msg1).build();
    TestParsingMerge.RepeatedFieldsGenerator.Group2 repeatedG2 =
        TestParsingMerge.RepeatedFieldsGenerator.Group2.newBuilder()
        .setField1(msg2).build();
    TestParsingMerge.RepeatedFieldsGenerator.Group2 repeatedG3 =
        TestParsingMerge.RepeatedFieldsGenerator.Group2.newBuilder()
        .setField1(msg3).build();

    // Assign and serialize RepeatedFieldsGenerator.
    ByteString data = TestParsingMerge.RepeatedFieldsGenerator.newBuilder()
        .addField1(msg1).addField1(msg2).addField1(msg3)
        .addField2(msg1).addField2(msg2).addField2(msg3)
        .addField3(msg1).addField3(msg2).addField3(msg3)
        .addGroup1(optionalG1).addGroup1(optionalG2).addGroup1(optionalG3)
        .addGroup2(repeatedG1).addGroup2(repeatedG2).addGroup2(repeatedG3)
        .addExt1(msg1).addExt1(msg2).addExt1(msg3)
        .addExt2(msg1).addExt2(msg2).addExt2(msg3)
        .build().toByteString();

    // Parse TestParsingMerge.
    ExtensionRegistry registry = ExtensionRegistry.newInstance();
    UnittestProto.registerAllExtensions(registry);
    TestParsingMerge parsingMerge =
        TestParsingMerge.PARSER.parseFrom(data, registry);

    // Required and optional fields should be merged.
    assertMessageMerged(parsingMerge.getRequiredAllTypes());
    assertMessageMerged(parsingMerge.getOptionalAllTypes());
    assertMessageMerged(
        parsingMerge.getOptionalGroup().getOptionalGroupAllTypes());
    assertMessageMerged(parsingMerge.getExtension(
        TestParsingMerge.optionalExt));

    // Repeated fields should not be merged.
    assertEquals(3, parsingMerge.getRepeatedAllTypesCount());
    assertEquals(3, parsingMerge.getRepeatedGroupCount());
    assertEquals(3, parsingMerge.getExtensionCount(
        TestParsingMerge.repeatedExt));
  }

  public void testParsingMergeLite() throws Exception {
    // Build messages.
    TestAllTypesLite.Builder builder =
        TestAllTypesLite.newBuilder();
    TestAllTypesLite msg1 = builder.setOptionalInt32(1).build();
    builder.clear();
    TestAllTypesLite msg2 = builder.setOptionalInt64(2).build();
    builder.clear();
    TestAllTypesLite msg3 = builder.setOptionalInt32(3)
        .setOptionalString("hello").build();

    // Build groups.
    TestParsingMergeLite.RepeatedFieldsGenerator.Group1 optionalG1 =
        TestParsingMergeLite.RepeatedFieldsGenerator.Group1.newBuilder()
        .setField1(msg1).build();
    TestParsingMergeLite.RepeatedFieldsGenerator.Group1 optionalG2 =
        TestParsingMergeLite.RepeatedFieldsGenerator.Group1.newBuilder()
        .setField1(msg2).build();
    TestParsingMergeLite.RepeatedFieldsGenerator.Group1 optionalG3 =
        TestParsingMergeLite.RepeatedFieldsGenerator.Group1.newBuilder()
        .setField1(msg3).build();
    TestParsingMergeLite.RepeatedFieldsGenerator.Group2 repeatedG1 =
        TestParsingMergeLite.RepeatedFieldsGenerator.Group2.newBuilder()
        .setField1(msg1).build();
    TestParsingMergeLite.RepeatedFieldsGenerator.Group2 repeatedG2 =
        TestParsingMergeLite.RepeatedFieldsGenerator.Group2.newBuilder()
        .setField1(msg2).build();
    TestParsingMergeLite.RepeatedFieldsGenerator.Group2 repeatedG3 =
        TestParsingMergeLite.RepeatedFieldsGenerator.Group2.newBuilder()
        .setField1(msg3).build();

    // Assign and serialize RepeatedFieldsGenerator.
    ByteString data = TestParsingMergeLite.RepeatedFieldsGenerator.newBuilder()
        .addField1(msg1).addField1(msg2).addField1(msg3)
        .addField2(msg1).addField2(msg2).addField2(msg3)
        .addField3(msg1).addField3(msg2).addField3(msg3)
        .addGroup1(optionalG1).addGroup1(optionalG2).addGroup1(optionalG3)
        .addGroup2(repeatedG1).addGroup2(repeatedG2).addGroup2(repeatedG3)
        .addExt1(msg1).addExt1(msg2).addExt1(msg3)
        .addExt2(msg1).addExt2(msg2).addExt2(msg3)
        .build().toByteString();

    // Parse TestParsingMergeLite.
    ExtensionRegistry registry = ExtensionRegistry.newInstance();
    UnittestLite.registerAllExtensions(registry);
    TestParsingMergeLite parsingMerge =
        TestParsingMergeLite.PARSER.parseFrom(data, registry);

    // Required and optional fields should be merged.
    assertMessageMerged(parsingMerge.getRequiredAllTypes());
    assertMessageMerged(parsingMerge.getOptionalAllTypes());
    assertMessageMerged(
        parsingMerge.getOptionalGroup().getOptionalGroupAllTypes());
    assertMessageMerged(parsingMerge.getExtension(
        TestParsingMergeLite.optionalExt));

    // Repeated fields should not be merged.
    assertEquals(3, parsingMerge.getRepeatedAllTypesCount());
    assertEquals(3, parsingMerge.getRepeatedGroupCount());
    assertEquals(3, parsingMerge.getExtensionCount(
        TestParsingMergeLite.repeatedExt));
  }
}
