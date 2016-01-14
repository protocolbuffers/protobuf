// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
// https://developers.google.com/protocol-buffers/
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

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertSame;
import static org.junit.Assert.assertTrue;
import static org.junit.Assert.fail;

import com.google.protobuf.UnittestLite.TestAllExtensionsLite;
import com.google.protobuf.UnittestLite.TestAllTypesLite;

import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

import protobuf_unittest.lite_equals_and_hash.LiteEqualsAndHash;
import protobuf_unittest.lite_equals_and_hash.LiteEqualsAndHash.Bar;
import protobuf_unittest.lite_equals_and_hash.LiteEqualsAndHash.Foo;

import java.io.ByteArrayOutputStream;
import java.io.IOException;

/**
 * Tests for {@link UnknownFieldSetLite}.
 *
 * @author dweis@google.com (Daniel Weis)
 */
@RunWith(JUnit4.class)
public class UnknownFieldSetLiteTest {

  @Test
  public void testNoDataIsDefaultInstance() {
    assertSame(
        UnknownFieldSetLite.getDefaultInstance(),
        UnknownFieldSetLite.newBuilder()
            .build());
  }
  
  @Test
  public void testBuilderReuse() throws IOException {
    UnknownFieldSetLite.Builder builder = UnknownFieldSetLite.newBuilder();
    builder.mergeVarintField(10, 2);
    builder.build();

    try {
      builder.build();
      fail();
    } catch (UnsupportedOperationException e) {
      // Expected.
    }

    try {
      builder.mergeFieldFrom(0, CodedInputStream.newInstance(new byte[0]));
      fail();
    } catch (UnsupportedOperationException e) {
      // Expected.
    }

    try {
      builder.mergeVarintField(5, 1);
      fail();
    } catch (UnsupportedOperationException e) {
      // Expected.
    }
  }

  @Test
  public void testBuilderReuse_empty() {
    UnknownFieldSetLite.Builder builder = UnknownFieldSetLite.newBuilder();
    builder.build();
    builder.build();
  }
  
  @Test
  public void testDefaultInstance() {
    UnknownFieldSetLite unknownFields = UnknownFieldSetLite.getDefaultInstance();

    assertEquals(0, unknownFields.getSerializedSize());
    assertEquals(ByteString.EMPTY, toByteString(unknownFields));
  }

  @Test
  public void testMergeFieldFrom() throws IOException {
    Foo foo = Foo.newBuilder()
      .setValue(2)
      .build();

    CodedInputStream input = CodedInputStream.newInstance(foo.toByteArray());

    UnknownFieldSetLite instance = UnknownFieldSetLite.newInstance();
    instance.mergeFieldFrom(input.readTag(), input);

    assertEquals(foo.toByteString(), toByteString(instance));
  }

  @Test
  public void testSerializedSize() throws IOException {
    Foo foo = Foo.newBuilder()
      .setValue(2)
      .build();

    CodedInputStream input = CodedInputStream.newInstance(foo.toByteArray());

    UnknownFieldSetLite instance = UnknownFieldSetLite.newInstance();
    instance.mergeFieldFrom(input.readTag(), input);

    assertEquals(foo.toByteString().size(), instance.getSerializedSize());
  }

  @Test
  public void testMergeVarintField() throws IOException {
    UnknownFieldSetLite unknownFields = UnknownFieldSetLite.newInstance();
    unknownFields.mergeVarintField(10, 2);

    CodedInputStream input =
        CodedInputStream.newInstance(toByteString(unknownFields).toByteArray());

    int tag = input.readTag();
    assertEquals(10, WireFormat.getTagFieldNumber(tag));
    assertEquals(WireFormat.WIRETYPE_VARINT, WireFormat.getTagWireType(tag));
    assertEquals(2, input.readUInt64());
    assertTrue(input.isAtEnd());
  }

  @Test
  public void testMergeVarintField_negative() throws IOException {
    UnknownFieldSetLite builder = UnknownFieldSetLite.newInstance();
    builder.mergeVarintField(10, -6);

    CodedInputStream input =
        CodedInputStream.newInstance(toByteString(builder).toByteArray());

    int tag = input.readTag();
    assertEquals(10, WireFormat.getTagFieldNumber(tag));
    assertEquals(WireFormat.WIRETYPE_VARINT, WireFormat.getTagWireType(tag));
    assertEquals(-6, input.readUInt64());
    assertTrue(input.isAtEnd());
  }

  @Test
  public void testEqualsAndHashCode() {
    UnknownFieldSetLite unknownFields1 = UnknownFieldSetLite.newInstance();
    unknownFields1.mergeVarintField(10, 2);

    UnknownFieldSetLite unknownFields2 = UnknownFieldSetLite.newInstance();
    unknownFields2.mergeVarintField(10, 2);

    assertEquals(unknownFields1, unknownFields2);
    assertEquals(unknownFields1.hashCode(), unknownFields2.hashCode());
    assertFalse(unknownFields1.equals(UnknownFieldSetLite.getDefaultInstance()));
    assertFalse(unknownFields1.hashCode() == UnknownFieldSetLite.getDefaultInstance().hashCode());
  }

  @Test
  public void testMutableCopyOf() throws IOException {
    UnknownFieldSetLite unknownFields = UnknownFieldSetLite.newInstance();
    unknownFields.mergeVarintField(10, 2);
    unknownFields = UnknownFieldSetLite.mutableCopyOf(unknownFields, unknownFields);
    unknownFields.checkMutable();

    CodedInputStream input =
        CodedInputStream.newInstance(toByteString(unknownFields).toByteArray());

    int tag = input.readTag();
    assertEquals(10, WireFormat.getTagFieldNumber(tag));
    assertEquals(WireFormat.WIRETYPE_VARINT, WireFormat.getTagWireType(tag));
    assertEquals(2, input.readUInt64());
    assertFalse(input.isAtEnd());
    input.readTag();
    assertEquals(10, WireFormat.getTagFieldNumber(tag));
    assertEquals(WireFormat.WIRETYPE_VARINT, WireFormat.getTagWireType(tag));
    assertEquals(2, input.readUInt64());
    assertTrue(input.isAtEnd());
  }

  @Test
  public void testMutableCopyOf_empty() {
    UnknownFieldSetLite unknownFields = UnknownFieldSetLite.mutableCopyOf(
        UnknownFieldSetLite.getDefaultInstance(), UnknownFieldSetLite.getDefaultInstance());
    unknownFields.checkMutable();

    assertEquals(0, unknownFields.getSerializedSize());
    assertEquals(ByteString.EMPTY, toByteString(unknownFields));
  }

  @Test
  public void testRoundTrips() throws InvalidProtocolBufferException {
    Foo foo = Foo.newBuilder()
        .setValue(1)
        .setExtension(Bar.fooExt, Bar.newBuilder()
            .setName("name")
            .build())
        .setExtension(LiteEqualsAndHash.varint, 22)
        .setExtension(LiteEqualsAndHash.fixed32, 44)
        .setExtension(LiteEqualsAndHash.fixed64, 66L)
        .setExtension(LiteEqualsAndHash.myGroup, LiteEqualsAndHash.MyGroup.newBuilder()
            .setGroupValue("value")
            .build())
        .build();

    Foo copy = Foo.parseFrom(foo.toByteArray());

    assertEquals(foo.getSerializedSize(), copy.getSerializedSize());
    assertFalse(foo.equals(copy));

    Foo secondCopy = Foo.parseFrom(foo.toByteArray());
    assertEquals(copy, secondCopy);

    ExtensionRegistryLite extensionRegistry = ExtensionRegistryLite.newInstance();
    LiteEqualsAndHash.registerAllExtensions(extensionRegistry);
    Foo copyOfCopy = Foo.parseFrom(copy.toByteArray(), extensionRegistry);

    assertEquals(foo, copyOfCopy);
  }

  @Test
  public void testMalformedBytes() throws Exception {
    try {
      Foo.parseFrom("this is a malformed protocol buffer".getBytes(Internal.UTF_8));
      fail();
    } catch (InvalidProtocolBufferException e) {
      // Expected.
    }
  }

  @Test
  public void testMissingStartGroupTag() throws IOException {
    ByteString.Output byteStringOutput = ByteString.newOutput();
    CodedOutputStream output = CodedOutputStream.newInstance(byteStringOutput);
    output.writeGroupNoTag(Foo.newBuilder().setValue(11).build());
    output.writeTag(100, WireFormat.WIRETYPE_END_GROUP);
    output.flush();

    try {
      Foo.parseFrom(byteStringOutput.toByteString());
      fail();
    } catch (InvalidProtocolBufferException e) {
      // Expected.
    }
  }

  @Test
  public void testMissingEndGroupTag() throws IOException {
    ByteString.Output byteStringOutput = ByteString.newOutput();
    CodedOutputStream output = CodedOutputStream.newInstance(byteStringOutput);
    output.writeTag(100, WireFormat.WIRETYPE_START_GROUP);
    output.writeGroupNoTag(Foo.newBuilder().setValue(11).build());
    output.flush();

    try {
      Foo.parseFrom(byteStringOutput.toByteString());
      fail();
    } catch (InvalidProtocolBufferException e) {
      // Expected.
    }
  }

  @Test
  public void testMismatchingGroupTags() throws IOException {
    ByteString.Output byteStringOutput = ByteString.newOutput();
    CodedOutputStream output = CodedOutputStream.newInstance(byteStringOutput);
    output.writeTag(100, WireFormat.WIRETYPE_START_GROUP);
    output.writeGroupNoTag(Foo.newBuilder().setValue(11).build());
    output.writeTag(101, WireFormat.WIRETYPE_END_GROUP);
    output.flush();

    try {
      Foo.parseFrom(byteStringOutput.toByteString());
      fail();
    } catch (InvalidProtocolBufferException e) {
      // Expected.
    }
  }

  @Test
  public void testTruncatedInput() {
    Foo foo = Foo.newBuilder()
        .setValue(1)
        .setExtension(Bar.fooExt, Bar.newBuilder()
            .setName("name")
            .build())
        .setExtension(LiteEqualsAndHash.varint, 22)
        .setExtension(LiteEqualsAndHash.myGroup, LiteEqualsAndHash.MyGroup.newBuilder()
            .setGroupValue("value")
            .build())
        .build();

    try {
      Foo.parseFrom(foo.toByteString().substring(0, foo.toByteString().size() - 10));
      fail();
    } catch (InvalidProtocolBufferException e) {
      // Expected.
    }
  }
  
  @Test
  public void testMakeImmutable() throws Exception {
    UnknownFieldSetLite unknownFields = UnknownFieldSetLite.newInstance();
    unknownFields.makeImmutable();
    
    try {
      unknownFields.mergeVarintField(1, 1);
      fail();
    } catch (UnsupportedOperationException expected) {
      // Expected
    }
    
    try {
      unknownFields.mergeLengthDelimitedField(2, ByteString.copyFromUtf8("hello"));
      fail();
    } catch (UnsupportedOperationException expected) {
      // Expected.
    }
    
    try {
      unknownFields.mergeFieldFrom(1, CodedInputStream.newInstance(new byte[0]));
      fail();
    } catch (UnsupportedOperationException expected) {
      // Expected
    }
  }
  
  @Test
  public void testEndToEnd() throws Exception {
    TestAllTypesLite testAllTypes = TestAllTypesLite.getDefaultInstance();
    try {
      testAllTypes.unknownFields.checkMutable();
      fail();
    } catch (UnsupportedOperationException expected) {
      // Expected
    }
    
    testAllTypes = TestAllTypesLite.parseFrom(new byte[0]);
    try {
      testAllTypes.unknownFields.checkMutable();
      fail();
    } catch (UnsupportedOperationException expected) {
      // Expected
    }
    
    testAllTypes = TestAllTypesLite.newBuilder().build();
    try {
      testAllTypes.unknownFields.checkMutable();
      fail();
    } catch (UnsupportedOperationException expected) {
      // Expected
    }
    
    testAllTypes = TestAllTypesLite.newBuilder()
        .setDefaultBool(true)
        .build();
    try {
      testAllTypes.unknownFields.checkMutable();
      fail();
    } catch (UnsupportedOperationException expected) {
      // Expected
    }
    
    TestAllExtensionsLite testAllExtensions = TestAllExtensionsLite.newBuilder()
        .mergeFrom(TestAllExtensionsLite.newBuilder()
            .setExtension(UnittestLite.optionalInt32ExtensionLite, 2)
            .build().toByteArray())
        .build();
    try {
      testAllExtensions.unknownFields.checkMutable();
      fail();
    } catch (UnsupportedOperationException expected) {
      // Expected
    }
  }

  private ByteString toByteString(UnknownFieldSetLite unknownFields) {
    ByteArrayOutputStream byteArrayOutputStream = new ByteArrayOutputStream();
    CodedOutputStream output = CodedOutputStream.newInstance(byteArrayOutputStream);
    try {
      unknownFields.writeTo(output);
      output.flush();
    } catch (IOException e) {
      throw new RuntimeException(e);
    }
    return ByteString.copyFrom(byteArrayOutputStream.toByteArray());
  }
}
