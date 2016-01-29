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

import static java.util.Collections.emptyList;
import static java.util.Collections.singletonList;

import com.google.protobuf.UnittestLite;
import com.google.protobuf.UnittestLite.ForeignEnumLite;
import com.google.protobuf.UnittestLite.ForeignMessageLite;
import com.google.protobuf.UnittestLite.TestAllExtensionsLite;
import com.google.protobuf.UnittestLite.TestAllTypesLite;
import com.google.protobuf.UnittestLite.TestAllTypesLite.NestedMessage;
import com.google.protobuf.UnittestLite.TestAllTypesLite.OneofFieldCase;
import com.google.protobuf.UnittestLite.TestAllTypesLite.OptionalGroup;
import com.google.protobuf.UnittestLite.TestAllTypesLite.RepeatedGroup;
import com.google.protobuf.UnittestLite.TestAllTypesLiteOrBuilder;
import com.google.protobuf.UnittestLite.TestNestedExtensionLite;

import junit.framework.TestCase;

import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.ObjectInputStream;
import java.io.ObjectOutputStream;

/**
 * Test lite runtime.
 *
 * @author kenton@google.com Kenton Varda
 */
public class LiteTest extends TestCase {
  public void setUp() throws Exception {
    // Test that nested extensions are initialized correctly even if the outer
    // class has not been accessed directly.  This was once a bug with lite
    // messages.
    //
    // We put this in setUp() rather than in its own test method because we
    // need to make sure it runs before any actual tests.
    assertTrue(TestNestedExtensionLite.nestedExtension != null);
  }

  public void testLite() throws Exception {
    // Since lite messages are a subset of regular messages, we can mostly
    // assume that the functionality of lite messages is already thoroughly
    // tested by the regular tests.  All this test really verifies is that
    // a proto with optimize_for = LITE_RUNTIME compiles correctly when
    // linked only against the lite library.  That is all tested at compile
    // time, leaving not much to do in this method.  Let's just do some random
    // stuff to make sure the lite message is actually here and usable.

    TestAllTypesLite message =
      TestAllTypesLite.newBuilder()
                      .setOptionalInt32(123)
                      .addRepeatedString("hello")
                      .setOptionalNestedMessage(
                          TestAllTypesLite.NestedMessage.newBuilder().setBb(7))
                      .build();

    ByteString data = message.toByteString();

    TestAllTypesLite message2 = TestAllTypesLite.parseFrom(data);

    assertEquals(123, message2.getOptionalInt32());
    assertEquals(1, message2.getRepeatedStringCount());
    assertEquals("hello", message2.getRepeatedString(0));
    assertEquals(7, message2.getOptionalNestedMessage().getBb());
  }

  public void testLiteExtensions() throws Exception {
    // TODO(kenton):  Unlike other features of the lite library, extensions are
    //   implemented completely differently from the regular library.  We
    //   should probably test them more thoroughly.

    TestAllExtensionsLite message =
      TestAllExtensionsLite.newBuilder()
        .setExtension(UnittestLite.optionalInt32ExtensionLite, 123)
        .addExtension(UnittestLite.repeatedStringExtensionLite, "hello")
        .setExtension(UnittestLite.optionalNestedEnumExtensionLite,
            TestAllTypesLite.NestedEnum.BAZ)
        .setExtension(UnittestLite.optionalNestedMessageExtensionLite,
            TestAllTypesLite.NestedMessage.newBuilder().setBb(7).build())
        .build();

    // Test copying a message, since coping extensions actually does use a
    // different code path between lite and regular libraries, and as of this
    // writing, parsing hasn't been implemented yet.
    TestAllExtensionsLite message2 = message.toBuilder().build();

    assertEquals(123, (int) message2.getExtension(
        UnittestLite.optionalInt32ExtensionLite));
    assertEquals(1, message2.getExtensionCount(
        UnittestLite.repeatedStringExtensionLite));
    assertEquals(1, message2.getExtension(
        UnittestLite.repeatedStringExtensionLite).size());
    assertEquals("hello", message2.getExtension(
        UnittestLite.repeatedStringExtensionLite, 0));
    assertEquals(TestAllTypesLite.NestedEnum.BAZ, message2.getExtension(
        UnittestLite.optionalNestedEnumExtensionLite));
    assertEquals(7, message2.getExtension(
        UnittestLite.optionalNestedMessageExtensionLite).getBb());
  }

  public void testSerialize() throws Exception {
    ByteArrayOutputStream baos = new ByteArrayOutputStream();
    TestAllTypesLite expected =
      TestAllTypesLite.newBuilder()
                      .setOptionalInt32(123)
                      .addRepeatedString("hello")
                      .setOptionalNestedMessage(
                          TestAllTypesLite.NestedMessage.newBuilder().setBb(7))
                      .build();
    ObjectOutputStream out = new ObjectOutputStream(baos);
    try {
      out.writeObject(expected);
    } finally {
      out.close();
    }
    ByteArrayInputStream bais = new ByteArrayInputStream(baos.toByteArray());
    ObjectInputStream in = new ObjectInputStream(bais);
    TestAllTypesLite actual = (TestAllTypesLite) in.readObject();
    assertEquals(expected.getOptionalInt32(), actual.getOptionalInt32());
    assertEquals(expected.getRepeatedStringCount(),
        actual.getRepeatedStringCount());
    assertEquals(expected.getRepeatedString(0),
        actual.getRepeatedString(0));
    assertEquals(expected.getOptionalNestedMessage().getBb(),
        actual.getOptionalNestedMessage().getBb());
  }
  
  public void testClone() {
    TestAllTypesLite.Builder expected = TestAllTypesLite.newBuilder()
        .setOptionalInt32(123);
    assertEquals(
        expected.getOptionalInt32(), expected.clone().getOptionalInt32());
   
    TestAllExtensionsLite.Builder expected2 = TestAllExtensionsLite.newBuilder()
        .setExtension(UnittestLite.optionalInt32ExtensionLite, 123);
    assertEquals(
        expected2.getExtension(UnittestLite.optionalInt32ExtensionLite),
        expected2.clone().getExtension(UnittestLite.optionalInt32ExtensionLite));
  }
  
  public void testAddAll() {
    try {
      TestAllTypesLite.newBuilder()
          .addAllRepeatedBytes(null);
      fail();
    } catch (NullPointerException e) {
      // expected.
    }
  }
  
  public void testSanityCopyOnWrite() throws InvalidProtocolBufferException {
    // Since builders are implemented as a thin wrapper around a message
    // instance, we attempt to verify that we can't cause the builder to modify
    // a produced message.

    TestAllTypesLite.Builder builder = TestAllTypesLite.newBuilder();
    TestAllTypesLite message = builder.build();
    TestAllTypesLite messageAfterBuild;
    builder.setOptionalBool(true);
    assertEquals(false, message.getOptionalBool());
    assertEquals(true, builder.getOptionalBool());
    messageAfterBuild = builder.build();
    assertEquals(true, messageAfterBuild.getOptionalBool());
    assertEquals(false, message.getOptionalBool());
    builder.clearOptionalBool();
    assertEquals(false, builder.getOptionalBool());
    assertEquals(true, messageAfterBuild.getOptionalBool());

    message = builder.build();
    builder.setOptionalBytes(ByteString.copyFromUtf8("hi"));
    assertEquals(ByteString.EMPTY, message.getOptionalBytes());
    assertEquals(ByteString.copyFromUtf8("hi"), builder.getOptionalBytes());
    messageAfterBuild = builder.build();
    assertEquals(
        ByteString.copyFromUtf8("hi"), messageAfterBuild.getOptionalBytes());
    assertEquals(ByteString.EMPTY, message.getOptionalBytes());
    builder.clearOptionalBytes();
    assertEquals(ByteString.EMPTY, builder.getOptionalBytes());
    assertEquals(
        ByteString.copyFromUtf8("hi"), messageAfterBuild.getOptionalBytes());

    message = builder.build();
    builder.setOptionalCord("hi");
    assertEquals("", message.getOptionalCord());
    assertEquals("hi", builder.getOptionalCord());
    messageAfterBuild = builder.build();
    assertEquals("hi", messageAfterBuild.getOptionalCord());
    assertEquals("", message.getOptionalCord());
    builder.clearOptionalCord();
    assertEquals("", builder.getOptionalCord());
    assertEquals("hi", messageAfterBuild.getOptionalCord());

    message = builder.build();
    builder.setOptionalCordBytes(ByteString.copyFromUtf8("no"));
    assertEquals(ByteString.EMPTY, message.getOptionalCordBytes());
    assertEquals(ByteString.copyFromUtf8("no"), builder.getOptionalCordBytes());
    messageAfterBuild = builder.build();
    assertEquals(
        ByteString.copyFromUtf8("no"),
        messageAfterBuild.getOptionalCordBytes());
    assertEquals(ByteString.EMPTY, message.getOptionalCordBytes());
    builder.clearOptionalCord();
    assertEquals(ByteString.EMPTY, builder.getOptionalCordBytes());
    assertEquals(
        ByteString.copyFromUtf8("no"),
        messageAfterBuild.getOptionalCordBytes());
    
    message = builder.build();
    builder.setOptionalDouble(1);
    assertEquals(0D, message.getOptionalDouble());
    assertEquals(1D, builder.getOptionalDouble());
    messageAfterBuild = builder.build();
    assertEquals(1D, messageAfterBuild.getOptionalDouble());
    assertEquals(0D, message.getOptionalDouble());
    builder.clearOptionalDouble();
    assertEquals(0D, builder.getOptionalDouble());
    assertEquals(1D, messageAfterBuild.getOptionalDouble());
    
    message = builder.build();
    builder.setOptionalFixed32(1);
    assertEquals(0, message.getOptionalFixed32());
    assertEquals(1, builder.getOptionalFixed32());
    messageAfterBuild = builder.build();
    assertEquals(1, messageAfterBuild.getOptionalFixed32());
    assertEquals(0, message.getOptionalFixed32());
    builder.clearOptionalFixed32();
    assertEquals(0, builder.getOptionalFixed32());
    assertEquals(1, messageAfterBuild.getOptionalFixed32());
    
    message = builder.build();
    builder.setOptionalFixed64(1);
    assertEquals(0L, message.getOptionalFixed64());
    assertEquals(1L, builder.getOptionalFixed64());
    messageAfterBuild = builder.build();
    assertEquals(1L, messageAfterBuild.getOptionalFixed64());
    assertEquals(0L, message.getOptionalFixed64());
    builder.clearOptionalFixed64();
    assertEquals(0L, builder.getOptionalFixed64());
    assertEquals(1L, messageAfterBuild.getOptionalFixed64());

    message = builder.build();
    builder.setOptionalFloat(1);
    assertEquals(0F, message.getOptionalFloat());
    assertEquals(1F, builder.getOptionalFloat());
    messageAfterBuild = builder.build();
    assertEquals(1F, messageAfterBuild.getOptionalFloat());
    assertEquals(0F, message.getOptionalFloat());
    builder.clearOptionalFloat();
    assertEquals(0F, builder.getOptionalFloat());
    assertEquals(1F, messageAfterBuild.getOptionalFloat());

    message = builder.build();
    builder.setOptionalForeignEnum(ForeignEnumLite.FOREIGN_LITE_BAR);
    assertEquals(
        ForeignEnumLite.FOREIGN_LITE_FOO, message.getOptionalForeignEnum());
    assertEquals(
        ForeignEnumLite.FOREIGN_LITE_BAR, builder.getOptionalForeignEnum());
    messageAfterBuild = builder.build();
    assertEquals(
        ForeignEnumLite.FOREIGN_LITE_BAR,
        messageAfterBuild.getOptionalForeignEnum());
    assertEquals(
        ForeignEnumLite.FOREIGN_LITE_FOO, message.getOptionalForeignEnum());
    builder.clearOptionalForeignEnum();
    assertEquals(
        ForeignEnumLite.FOREIGN_LITE_FOO, builder.getOptionalForeignEnum());
    assertEquals(
        ForeignEnumLite.FOREIGN_LITE_BAR,
        messageAfterBuild.getOptionalForeignEnum());

    message = builder.build();
    ForeignMessageLite foreignMessage = ForeignMessageLite.newBuilder()
        .setC(1)
        .build();
    builder.setOptionalForeignMessage(foreignMessage);
    assertEquals(
        ForeignMessageLite.getDefaultInstance(),
        message.getOptionalForeignMessage());
    assertEquals(foreignMessage, builder.getOptionalForeignMessage());
    messageAfterBuild = builder.build();
    assertEquals(foreignMessage, messageAfterBuild.getOptionalForeignMessage());
    assertEquals(
        ForeignMessageLite.getDefaultInstance(),
        message.getOptionalForeignMessage());
    builder.clearOptionalForeignMessage();
    assertEquals(
        ForeignMessageLite.getDefaultInstance(),
        builder.getOptionalForeignMessage());
    assertEquals(foreignMessage, messageAfterBuild.getOptionalForeignMessage());

    message = builder.build();
    ForeignMessageLite.Builder foreignMessageBuilder =
        ForeignMessageLite.newBuilder()
            .setC(3);
    builder.setOptionalForeignMessage(foreignMessageBuilder);
    assertEquals(
        ForeignMessageLite.getDefaultInstance(),
        message.getOptionalForeignMessage());
    // LITE_RUNTIME doesn't implement equals so we compare on a property and
    // ensure the property isn't set on foreignMessage.
    assertEquals(3, builder.getOptionalForeignMessage().getC());
    messageAfterBuild = builder.build();
    assertEquals(3, messageAfterBuild.getOptionalForeignMessage().getC());
    assertEquals(
        ForeignMessageLite.getDefaultInstance(),
        message.getOptionalForeignMessage());
    builder.clearOptionalForeignMessage();
    assertEquals(
        ForeignMessageLite.getDefaultInstance(),
        builder.getOptionalForeignMessage());
    assertEquals(3, messageAfterBuild.getOptionalForeignMessage().getC());

    message = builder.build();
    OptionalGroup optionalGroup = OptionalGroup.newBuilder()
        .setA(1)
        .build();
    builder.setOptionalGroup(optionalGroup);
    assertEquals(
        OptionalGroup.getDefaultInstance(), message.getOptionalGroup());
    assertEquals(optionalGroup, builder.getOptionalGroup());
    messageAfterBuild = builder.build();
    assertEquals(optionalGroup, messageAfterBuild.getOptionalGroup());
    assertEquals(
        OptionalGroup.getDefaultInstance(), message.getOptionalGroup());
    builder.clearOptionalGroup();
    assertEquals(
        OptionalGroup.getDefaultInstance(), builder.getOptionalGroup());
    assertEquals(optionalGroup, messageAfterBuild.getOptionalGroup());

    message = builder.build();
    OptionalGroup.Builder optionalGroupBuilder = OptionalGroup.newBuilder()
        .setA(3);
    builder.setOptionalGroup(optionalGroupBuilder);
    assertEquals(
        OptionalGroup.getDefaultInstance(), message.getOptionalGroup());
    // LITE_RUNTIME doesn't implement equals so we compare on a property and
    // ensure the property isn't set on optionalGroup.
    assertEquals(3, builder.getOptionalGroup().getA());
    messageAfterBuild = builder.build();
    assertEquals(3, messageAfterBuild.getOptionalGroup().getA());
    assertEquals(
        OptionalGroup.getDefaultInstance(), message.getOptionalGroup());
    builder.clearOptionalGroup();
    assertEquals(
        OptionalGroup.getDefaultInstance(), builder.getOptionalGroup());
    assertEquals(3, messageAfterBuild.getOptionalGroup().getA());

    message = builder.build();
    builder.setOptionalInt32(1);
    assertEquals(0, message.getOptionalInt32());
    assertEquals(1, builder.getOptionalInt32());
    messageAfterBuild = builder.build();
    assertEquals(1, messageAfterBuild.getOptionalInt32());
    assertEquals(0, message.getOptionalInt32());
    builder.clearOptionalInt32();
    assertEquals(0, builder.getOptionalInt32());
    assertEquals(1, messageAfterBuild.getOptionalInt32());

    message = builder.build();
    builder.setOptionalInt64(1);
    assertEquals(0L, message.getOptionalInt64());
    assertEquals(1L, builder.getOptionalInt64());
    messageAfterBuild = builder.build();
    assertEquals(1L, messageAfterBuild.getOptionalInt64());
    assertEquals(0L, message.getOptionalInt64());
    builder.clearOptionalInt64();
    assertEquals(0L, builder.getOptionalInt64());
    assertEquals(1L, messageAfterBuild.getOptionalInt64());
    
    message = builder.build();
    NestedMessage nestedMessage = NestedMessage.newBuilder()
        .setBb(1)
        .build();
    builder.setOptionalLazyMessage(nestedMessage);
    assertEquals(
        NestedMessage.getDefaultInstance(),
        message.getOptionalLazyMessage());
    assertEquals(nestedMessage, builder.getOptionalLazyMessage());
    messageAfterBuild = builder.build();
    assertEquals(nestedMessage, messageAfterBuild.getOptionalLazyMessage());
    assertEquals(
        NestedMessage.getDefaultInstance(),
        message.getOptionalLazyMessage());
    builder.clearOptionalLazyMessage();
    assertEquals(
        NestedMessage.getDefaultInstance(), builder.getOptionalLazyMessage());
    assertEquals(nestedMessage, messageAfterBuild.getOptionalLazyMessage());

    message = builder.build();
    NestedMessage.Builder nestedMessageBuilder =
        NestedMessage.newBuilder()
            .setBb(3);
    builder.setOptionalLazyMessage(nestedMessageBuilder);
    assertEquals(
        NestedMessage.getDefaultInstance(),
        message.getOptionalLazyMessage());
    // LITE_RUNTIME doesn't implement equals so we compare on a property.
    assertEquals(3, builder.getOptionalLazyMessage().getBb());
    messageAfterBuild = builder.build();
    assertEquals(3, messageAfterBuild.getOptionalLazyMessage().getBb());
    assertEquals(
        NestedMessage.getDefaultInstance(),
        message.getOptionalLazyMessage());
    builder.clearOptionalLazyMessage();
    assertEquals(
        NestedMessage.getDefaultInstance(), builder.getOptionalLazyMessage());
    assertEquals(3, messageAfterBuild.getOptionalLazyMessage().getBb());

    message = builder.build();
    builder.setOptionalSfixed32(1);
    assertEquals(0, message.getOptionalSfixed32());
    assertEquals(1, builder.getOptionalSfixed32());
    messageAfterBuild = builder.build();
    assertEquals(1, messageAfterBuild.getOptionalSfixed32());
    assertEquals(0, message.getOptionalSfixed32());
    builder.clearOptionalSfixed32();
    assertEquals(0, builder.getOptionalSfixed32());
    assertEquals(1, messageAfterBuild.getOptionalSfixed32());

    message = builder.build();
    builder.setOptionalSfixed64(1);
    assertEquals(0L, message.getOptionalSfixed64());
    assertEquals(1L, builder.getOptionalSfixed64());
    messageAfterBuild = builder.build();
    assertEquals(1L, messageAfterBuild.getOptionalSfixed64());
    assertEquals(0L, message.getOptionalSfixed64());
    builder.clearOptionalSfixed64();
    assertEquals(0L, builder.getOptionalSfixed64());
    assertEquals(1L, messageAfterBuild.getOptionalSfixed64());

    message = builder.build();
    builder.setOptionalSint32(1);
    assertEquals(0, message.getOptionalSint32());
    assertEquals(1, builder.getOptionalSint32());
    messageAfterBuild = builder.build();
    assertEquals(1, messageAfterBuild.getOptionalSint32());
    builder.clearOptionalSint32();
    assertEquals(0, builder.getOptionalSint32());
    assertEquals(1, messageAfterBuild.getOptionalSint32());

    message = builder.build();
    builder.setOptionalSint64(1);
    assertEquals(0L, message.getOptionalSint64());
    assertEquals(1L, builder.getOptionalSint64());
    messageAfterBuild = builder.build();
    assertEquals(1L, messageAfterBuild.getOptionalSint64());
    assertEquals(0L, message.getOptionalSint64());
    builder.clearOptionalSint64();
    assertEquals(0L, builder.getOptionalSint64());
    assertEquals(1L, messageAfterBuild.getOptionalSint64());

    message = builder.build();
    builder.setOptionalString("hi");
    assertEquals("", message.getOptionalString());
    assertEquals("hi", builder.getOptionalString());
    messageAfterBuild = builder.build();
    assertEquals("hi", messageAfterBuild.getOptionalString());
    assertEquals("", message.getOptionalString());
    builder.clearOptionalString();
    assertEquals("", builder.getOptionalString());
    assertEquals("hi", messageAfterBuild.getOptionalString());

    message = builder.build();
    builder.setOptionalStringBytes(ByteString.copyFromUtf8("no"));
    assertEquals(ByteString.EMPTY, message.getOptionalStringBytes());
    assertEquals(
        ByteString.copyFromUtf8("no"), builder.getOptionalStringBytes());
    messageAfterBuild = builder.build();
    assertEquals(
        ByteString.copyFromUtf8("no"),
        messageAfterBuild.getOptionalStringBytes());
    assertEquals(ByteString.EMPTY, message.getOptionalStringBytes());
    builder.clearOptionalString();
    assertEquals(ByteString.EMPTY, builder.getOptionalStringBytes());
    assertEquals(
        ByteString.copyFromUtf8("no"),
        messageAfterBuild.getOptionalStringBytes());
    
    message = builder.build();
    builder.setOptionalStringPiece("hi");
    assertEquals("", message.getOptionalStringPiece());
    assertEquals("hi", builder.getOptionalStringPiece());
    messageAfterBuild = builder.build();
    assertEquals("hi", messageAfterBuild.getOptionalStringPiece());
    assertEquals("", message.getOptionalStringPiece());
    builder.clearOptionalStringPiece();
    assertEquals("", builder.getOptionalStringPiece());
    assertEquals("hi", messageAfterBuild.getOptionalStringPiece());

    message = builder.build();
    builder.setOptionalStringPieceBytes(ByteString.copyFromUtf8("no"));
    assertEquals(ByteString.EMPTY, message.getOptionalStringPieceBytes());
    assertEquals(
        ByteString.copyFromUtf8("no"), builder.getOptionalStringPieceBytes());
    messageAfterBuild = builder.build();
    assertEquals(
        ByteString.copyFromUtf8("no"),
        messageAfterBuild.getOptionalStringPieceBytes());
    assertEquals(ByteString.EMPTY, message.getOptionalStringPieceBytes());
    builder.clearOptionalStringPiece();
    assertEquals(ByteString.EMPTY, builder.getOptionalStringPieceBytes());
    assertEquals(
        ByteString.copyFromUtf8("no"),
        messageAfterBuild.getOptionalStringPieceBytes());

    message = builder.build();
    builder.setOptionalUint32(1);
    assertEquals(0, message.getOptionalUint32());
    assertEquals(1, builder.getOptionalUint32());
    messageAfterBuild = builder.build();
    assertEquals(1, messageAfterBuild.getOptionalUint32());
    assertEquals(0, message.getOptionalUint32());
    builder.clearOptionalUint32();
    assertEquals(0, builder.getOptionalUint32());
    assertEquals(1, messageAfterBuild.getOptionalUint32());

    message = builder.build();
    builder.setOptionalUint64(1);
    assertEquals(0L, message.getOptionalUint64());
    assertEquals(1L, builder.getOptionalUint64());
    messageAfterBuild = builder.build();
    assertEquals(1L, messageAfterBuild.getOptionalUint64());
    assertEquals(0L, message.getOptionalUint64());
    builder.clearOptionalUint64();
    assertEquals(0L, builder.getOptionalUint64());
    assertEquals(1L, messageAfterBuild.getOptionalUint64());

    message = builder.build();
    builder.addAllRepeatedBool(singletonList(true));
    assertEquals(emptyList(), message.getRepeatedBoolList());
    assertEquals(singletonList(true), builder.getRepeatedBoolList());
    assertEquals(emptyList(), message.getRepeatedBoolList());
    messageAfterBuild = builder.build();
    builder.clearRepeatedBool();
    assertEquals(emptyList(), builder.getRepeatedBoolList());
    assertEquals(singletonList(true), messageAfterBuild.getRepeatedBoolList());

    message = builder.build();
    builder.addAllRepeatedBytes(singletonList(ByteString.copyFromUtf8("hi")));
    assertEquals(emptyList(), message.getRepeatedBytesList());
    assertEquals(
        singletonList(ByteString.copyFromUtf8("hi")),
        builder.getRepeatedBytesList());
    assertEquals(emptyList(), message.getRepeatedBytesList());
    messageAfterBuild = builder.build();
    builder.clearRepeatedBytes();
    assertEquals(emptyList(), builder.getRepeatedBytesList());
    assertEquals(
        singletonList(ByteString.copyFromUtf8("hi")),
        messageAfterBuild.getRepeatedBytesList());

    message = builder.build();
    builder.addAllRepeatedCord(singletonList("hi"));
    assertEquals(emptyList(), message.getRepeatedCordList());
    assertEquals(singletonList("hi"), builder.getRepeatedCordList());
    assertEquals(emptyList(), message.getRepeatedCordList());
    messageAfterBuild = builder.build();
    builder.clearRepeatedCord();
    assertEquals(emptyList(), builder.getRepeatedCordList());
    assertEquals(singletonList("hi"), messageAfterBuild.getRepeatedCordList());

    message = builder.build();
    builder.addAllRepeatedDouble(singletonList(1D));
    assertEquals(emptyList(), message.getRepeatedDoubleList());
    assertEquals(singletonList(1D), builder.getRepeatedDoubleList());
    assertEquals(emptyList(), message.getRepeatedDoubleList());
    messageAfterBuild = builder.build();
    builder.clearRepeatedDouble();
    assertEquals(emptyList(), builder.getRepeatedDoubleList());
    assertEquals(singletonList(1D), messageAfterBuild.getRepeatedDoubleList());

    message = builder.build();
    builder.addAllRepeatedFixed32(singletonList(1));
    assertEquals(emptyList(), message.getRepeatedFixed32List());
    assertEquals(singletonList(1), builder.getRepeatedFixed32List());
    assertEquals(emptyList(), message.getRepeatedFixed32List());
    messageAfterBuild = builder.build();
    builder.clearRepeatedFixed32();
    assertEquals(emptyList(), builder.getRepeatedFixed32List());
    assertEquals(singletonList(1), messageAfterBuild.getRepeatedFixed32List());

    message = builder.build();
    builder.addAllRepeatedFixed64(singletonList(1L));
    assertEquals(emptyList(), message.getRepeatedFixed64List());
    assertEquals(singletonList(1L), builder.getRepeatedFixed64List());
    assertEquals(emptyList(), message.getRepeatedFixed64List());
    messageAfterBuild = builder.build();
    builder.clearRepeatedFixed64();
    assertEquals(emptyList(), builder.getRepeatedFixed64List());
    assertEquals(singletonList(1L), messageAfterBuild.getRepeatedFixed64List());

    message = builder.build();
    builder.addAllRepeatedFloat(singletonList(1F));
    assertEquals(emptyList(), message.getRepeatedFloatList());
    assertEquals(singletonList(1F), builder.getRepeatedFloatList());
    assertEquals(emptyList(), message.getRepeatedFloatList());
    messageAfterBuild = builder.build();
    builder.clearRepeatedFloat();
    assertEquals(emptyList(), builder.getRepeatedFloatList());
    assertEquals(singletonList(1F), messageAfterBuild.getRepeatedFloatList());

    message = builder.build();
    builder.addAllRepeatedForeignEnum(
        singletonList(ForeignEnumLite.FOREIGN_LITE_BAR));
    assertEquals(emptyList(), message.getRepeatedForeignEnumList());
    assertEquals(
        singletonList(ForeignEnumLite.FOREIGN_LITE_BAR),
        builder.getRepeatedForeignEnumList());
    assertEquals(emptyList(), message.getRepeatedForeignEnumList());
    messageAfterBuild = builder.build();
    builder.clearRepeatedForeignEnum();
    assertEquals(emptyList(), builder.getRepeatedForeignEnumList());
    assertEquals(
        singletonList(ForeignEnumLite.FOREIGN_LITE_BAR),
        messageAfterBuild.getRepeatedForeignEnumList());

    message = builder.build();
    builder.addAllRepeatedForeignMessage(singletonList(foreignMessage));
    assertEquals(emptyList(), message.getRepeatedForeignMessageList());
    assertEquals(
        singletonList(foreignMessage), builder.getRepeatedForeignMessageList());
    assertEquals(emptyList(), message.getRepeatedForeignMessageList());
    messageAfterBuild = builder.build();
    builder.clearRepeatedForeignMessage();
    assertEquals(emptyList(), builder.getRepeatedForeignMessageList());
    assertEquals(
        singletonList(foreignMessage),
        messageAfterBuild.getRepeatedForeignMessageList());

    message = builder.build();
    builder.addAllRepeatedGroup(
        singletonList(RepeatedGroup.getDefaultInstance()));
    assertEquals(emptyList(), message.getRepeatedGroupList());
    assertEquals(
        singletonList(RepeatedGroup.getDefaultInstance()),
        builder.getRepeatedGroupList());
    assertEquals(emptyList(), message.getRepeatedGroupList());
    messageAfterBuild = builder.build();
    builder.clearRepeatedGroup();
    assertEquals(emptyList(), builder.getRepeatedGroupList());
    assertEquals(
        singletonList(RepeatedGroup.getDefaultInstance()),
        messageAfterBuild.getRepeatedGroupList());

    message = builder.build();
    builder.addAllRepeatedInt32(singletonList(1));
    assertEquals(emptyList(), message.getRepeatedInt32List());
    assertEquals(singletonList(1), builder.getRepeatedInt32List());
    assertEquals(emptyList(), message.getRepeatedInt32List());
    messageAfterBuild = builder.build();
    builder.clearRepeatedInt32();
    assertEquals(emptyList(), builder.getRepeatedInt32List());
    assertEquals(singletonList(1), messageAfterBuild.getRepeatedInt32List());

    message = builder.build();
    builder.addAllRepeatedInt64(singletonList(1L));
    assertEquals(emptyList(), message.getRepeatedInt64List());
    assertEquals(singletonList(1L), builder.getRepeatedInt64List());
    assertEquals(emptyList(), message.getRepeatedInt64List());
    messageAfterBuild = builder.build();
    builder.clearRepeatedInt64();
    assertEquals(emptyList(), builder.getRepeatedInt64List());
    assertEquals(singletonList(1L), messageAfterBuild.getRepeatedInt64List());

    message = builder.build();
    builder.addAllRepeatedLazyMessage(singletonList(nestedMessage));
    assertEquals(emptyList(), message.getRepeatedLazyMessageList());
    assertEquals(
        singletonList(nestedMessage), builder.getRepeatedLazyMessageList());
    assertEquals(emptyList(), message.getRepeatedLazyMessageList());
    messageAfterBuild = builder.build();
    builder.clearRepeatedLazyMessage();
    assertEquals(emptyList(), builder.getRepeatedLazyMessageList());
    assertEquals(
        singletonList(nestedMessage),
        messageAfterBuild.getRepeatedLazyMessageList());

    message = builder.build();
    builder.addAllRepeatedSfixed32(singletonList(1));
    assertEquals(emptyList(), message.getRepeatedSfixed32List());
    assertEquals(singletonList(1), builder.getRepeatedSfixed32List());
    assertEquals(emptyList(), message.getRepeatedSfixed32List());
    messageAfterBuild = builder.build();
    builder.clearRepeatedSfixed32();
    assertEquals(emptyList(), builder.getRepeatedSfixed32List());
    assertEquals(singletonList(1), messageAfterBuild.getRepeatedSfixed32List());

    message = builder.build();
    builder.addAllRepeatedSfixed64(singletonList(1L));
    assertEquals(emptyList(), message.getRepeatedSfixed64List());
    assertEquals(singletonList(1L), builder.getRepeatedSfixed64List());
    assertEquals(emptyList(), message.getRepeatedSfixed64List());
    messageAfterBuild = builder.build();
    builder.clearRepeatedSfixed64();
    assertEquals(emptyList(), builder.getRepeatedSfixed64List());
    assertEquals(
        singletonList(1L), messageAfterBuild.getRepeatedSfixed64List());

    message = builder.build();
    builder.addAllRepeatedSint32(singletonList(1));
    assertEquals(emptyList(), message.getRepeatedSint32List());
    assertEquals(singletonList(1), builder.getRepeatedSint32List());
    assertEquals(emptyList(), message.getRepeatedSint32List());
    messageAfterBuild = builder.build();
    builder.clearRepeatedSint32();
    assertEquals(emptyList(), builder.getRepeatedSint32List());
    assertEquals(singletonList(1), messageAfterBuild.getRepeatedSint32List());

    message = builder.build();
    builder.addAllRepeatedSint64(singletonList(1L));
    assertEquals(emptyList(), message.getRepeatedSint64List());
    assertEquals(singletonList(1L), builder.getRepeatedSint64List());
    assertEquals(emptyList(), message.getRepeatedSint64List());
    messageAfterBuild = builder.build();
    builder.clearRepeatedSint64();
    assertEquals(emptyList(), builder.getRepeatedSint64List());
    assertEquals(singletonList(1L), messageAfterBuild.getRepeatedSint64List());

    message = builder.build();
    builder.addAllRepeatedString(singletonList("hi"));
    assertEquals(emptyList(), message.getRepeatedStringList());
    assertEquals(singletonList("hi"), builder.getRepeatedStringList());
    assertEquals(emptyList(), message.getRepeatedStringList());
    messageAfterBuild = builder.build();
    builder.clearRepeatedString();
    assertEquals(emptyList(), builder.getRepeatedStringList());
    assertEquals(
        singletonList("hi"), messageAfterBuild.getRepeatedStringList());

    message = builder.build();
    builder.addAllRepeatedStringPiece(singletonList("hi"));
    assertEquals(emptyList(), message.getRepeatedStringPieceList());
    assertEquals(singletonList("hi"), builder.getRepeatedStringPieceList());
    assertEquals(emptyList(), message.getRepeatedStringPieceList());
    messageAfterBuild = builder.build();
    builder.clearRepeatedStringPiece();
    assertEquals(emptyList(), builder.getRepeatedStringPieceList());
    assertEquals(
        singletonList("hi"), messageAfterBuild.getRepeatedStringPieceList());

    message = builder.build();
    builder.addAllRepeatedUint32(singletonList(1));
    assertEquals(emptyList(), message.getRepeatedUint32List());
    assertEquals(singletonList(1), builder.getRepeatedUint32List());
    assertEquals(emptyList(), message.getRepeatedUint32List());
    messageAfterBuild = builder.build();
    builder.clearRepeatedUint32();
    assertEquals(emptyList(), builder.getRepeatedUint32List());
    assertEquals(singletonList(1), messageAfterBuild.getRepeatedUint32List());

    message = builder.build();
    builder.addAllRepeatedUint64(singletonList(1L));
    assertEquals(emptyList(), message.getRepeatedUint64List());
    assertEquals(singletonList(1L), builder.getRepeatedUint64List());
    assertEquals(emptyList(), message.getRepeatedUint64List());
    messageAfterBuild = builder.build();
    builder.clearRepeatedUint64();
    assertEquals(emptyList(), builder.getRepeatedUint64List());
    assertEquals(singletonList(1L), messageAfterBuild.getRepeatedUint64List());

    message = builder.build();
    builder.addRepeatedBool(true);
    assertEquals(emptyList(), message.getRepeatedBoolList());
    assertEquals(singletonList(true), builder.getRepeatedBoolList());
    assertEquals(emptyList(), message.getRepeatedBoolList());
    messageAfterBuild = builder.build();
    builder.clearRepeatedBool();
    assertEquals(emptyList(), builder.getRepeatedBoolList());
    assertEquals(singletonList(true), messageAfterBuild.getRepeatedBoolList());

    message = builder.build();
    builder.addRepeatedBytes(ByteString.copyFromUtf8("hi"));
    assertEquals(emptyList(), message.getRepeatedBytesList());
    assertEquals(
        singletonList(ByteString.copyFromUtf8("hi")),
        builder.getRepeatedBytesList());
    assertEquals(emptyList(), message.getRepeatedBytesList());
    messageAfterBuild = builder.build();
    builder.clearRepeatedBytes();
    assertEquals(emptyList(), builder.getRepeatedBytesList());
    assertEquals(
        singletonList(ByteString.copyFromUtf8("hi")),
        messageAfterBuild.getRepeatedBytesList());

    message = builder.build();
    builder.addRepeatedCord("hi");
    assertEquals(emptyList(), message.getRepeatedCordList());
    assertEquals(singletonList("hi"), builder.getRepeatedCordList());
    assertEquals(emptyList(), message.getRepeatedCordList());
    messageAfterBuild = builder.build();
    builder.clearRepeatedCord();
    assertEquals(emptyList(), builder.getRepeatedCordList());
    assertEquals(singletonList("hi"), messageAfterBuild.getRepeatedCordList());

    message = builder.build();
    builder.addRepeatedDouble(1D);
    assertEquals(emptyList(), message.getRepeatedDoubleList());
    assertEquals(singletonList(1D), builder.getRepeatedDoubleList());
    assertEquals(emptyList(), message.getRepeatedDoubleList());
    messageAfterBuild = builder.build();
    builder.clearRepeatedDouble();
    assertEquals(emptyList(), builder.getRepeatedDoubleList());
    assertEquals(singletonList(1D), messageAfterBuild.getRepeatedDoubleList());

    message = builder.build();
    builder.addRepeatedFixed32(1);
    assertEquals(emptyList(), message.getRepeatedFixed32List());
    assertEquals(singletonList(1), builder.getRepeatedFixed32List());
    assertEquals(emptyList(), message.getRepeatedFixed32List());
    messageAfterBuild = builder.build();
    builder.clearRepeatedFixed32();
    assertEquals(emptyList(), builder.getRepeatedFixed32List());
    assertEquals(singletonList(1), messageAfterBuild.getRepeatedFixed32List());

    message = builder.build();
    builder.addRepeatedFixed64(1L);
    assertEquals(emptyList(), message.getRepeatedFixed64List());
    assertEquals(singletonList(1L), builder.getRepeatedFixed64List());
    assertEquals(emptyList(), message.getRepeatedFixed64List());
    messageAfterBuild = builder.build();
    builder.clearRepeatedFixed64();
    assertEquals(emptyList(), builder.getRepeatedFixed64List());
    assertEquals(singletonList(1L), messageAfterBuild.getRepeatedFixed64List());

    message = builder.build();
    builder.addRepeatedFloat(1F);
    assertEquals(emptyList(), message.getRepeatedFloatList());
    assertEquals(singletonList(1F), builder.getRepeatedFloatList());
    assertEquals(emptyList(), message.getRepeatedFloatList());
    messageAfterBuild = builder.build();
    builder.clearRepeatedFloat();
    assertEquals(emptyList(), builder.getRepeatedFloatList());
    assertEquals(singletonList(1F), messageAfterBuild.getRepeatedFloatList());

    message = builder.build();
    builder.addRepeatedForeignEnum(ForeignEnumLite.FOREIGN_LITE_BAR);
    assertEquals(emptyList(), message.getRepeatedForeignEnumList());
    assertEquals(
        singletonList(ForeignEnumLite.FOREIGN_LITE_BAR),
        builder.getRepeatedForeignEnumList());
    assertEquals(emptyList(), message.getRepeatedForeignEnumList());
    messageAfterBuild = builder.build();
    builder.clearRepeatedForeignEnum();
    assertEquals(emptyList(), builder.getRepeatedForeignEnumList());
    assertEquals(
        singletonList(ForeignEnumLite.FOREIGN_LITE_BAR),
        messageAfterBuild.getRepeatedForeignEnumList());

    message = builder.build();
    builder.addRepeatedForeignMessage(foreignMessage);
    assertEquals(emptyList(), message.getRepeatedForeignMessageList());
    assertEquals(
        singletonList(foreignMessage), builder.getRepeatedForeignMessageList());
    assertEquals(emptyList(), message.getRepeatedForeignMessageList());
    messageAfterBuild = builder.build();
    builder.removeRepeatedForeignMessage(0);
    assertEquals(emptyList(), builder.getRepeatedForeignMessageList());
    assertEquals(
        singletonList(foreignMessage),
        messageAfterBuild.getRepeatedForeignMessageList());

    message = builder.build();
    builder.addRepeatedGroup(RepeatedGroup.getDefaultInstance());
    assertEquals(emptyList(), message.getRepeatedGroupList());
    assertEquals(
        singletonList(RepeatedGroup.getDefaultInstance()),
        builder.getRepeatedGroupList());
    assertEquals(emptyList(), message.getRepeatedGroupList());
    messageAfterBuild = builder.build();
    builder.removeRepeatedGroup(0);
    assertEquals(emptyList(), builder.getRepeatedGroupList());
    assertEquals(
        singletonList(RepeatedGroup.getDefaultInstance()),
        messageAfterBuild.getRepeatedGroupList());

    message = builder.build();
    builder.addRepeatedInt32(1);
    assertEquals(emptyList(), message.getRepeatedInt32List());
    assertEquals(singletonList(1), builder.getRepeatedInt32List());
    assertEquals(emptyList(), message.getRepeatedInt32List());
    messageAfterBuild = builder.build();
    builder.clearRepeatedInt32();
    assertEquals(emptyList(), builder.getRepeatedInt32List());
    assertEquals(singletonList(1), messageAfterBuild.getRepeatedInt32List());

    message = builder.build();
    builder.addRepeatedInt64(1L);
    assertEquals(emptyList(), message.getRepeatedInt64List());
    assertEquals(singletonList(1L), builder.getRepeatedInt64List());
    assertEquals(emptyList(), message.getRepeatedInt64List());
    messageAfterBuild = builder.build();
    builder.clearRepeatedInt64();
    assertEquals(emptyList(), builder.getRepeatedInt64List());
    assertEquals(singletonList(1L), messageAfterBuild.getRepeatedInt64List());

    message = builder.build();
    builder.addRepeatedLazyMessage(nestedMessage);
    assertEquals(emptyList(), message.getRepeatedLazyMessageList());
    assertEquals(
        singletonList(nestedMessage), builder.getRepeatedLazyMessageList());
    assertEquals(emptyList(), message.getRepeatedLazyMessageList());
    messageAfterBuild = builder.build();
    builder.removeRepeatedLazyMessage(0);
    assertEquals(emptyList(), builder.getRepeatedLazyMessageList());
    assertEquals(
        singletonList(nestedMessage),
        messageAfterBuild.getRepeatedLazyMessageList());

    message = builder.build();
    builder.addRepeatedSfixed32(1);
    assertEquals(emptyList(), message.getRepeatedSfixed32List());
    assertEquals(singletonList(1), builder.getRepeatedSfixed32List());
    assertEquals(emptyList(), message.getRepeatedSfixed32List());
    messageAfterBuild = builder.build();
    builder.clearRepeatedSfixed32();
    assertEquals(emptyList(), builder.getRepeatedSfixed32List());
    assertEquals(singletonList(1), messageAfterBuild.getRepeatedSfixed32List());

    message = builder.build();
    builder.addRepeatedSfixed64(1L);
    assertEquals(emptyList(), message.getRepeatedSfixed64List());
    assertEquals(singletonList(1L), builder.getRepeatedSfixed64List());
    assertEquals(emptyList(), message.getRepeatedSfixed64List());
    messageAfterBuild = builder.build();
    builder.clearRepeatedSfixed64();
    assertEquals(emptyList(), builder.getRepeatedSfixed64List());
    assertEquals(
        singletonList(1L), messageAfterBuild.getRepeatedSfixed64List());

    message = builder.build();
    builder.addRepeatedSint32(1);
    assertEquals(emptyList(), message.getRepeatedSint32List());
    assertEquals(singletonList(1), builder.getRepeatedSint32List());
    assertEquals(emptyList(), message.getRepeatedSint32List());
    messageAfterBuild = builder.build();
    builder.clearRepeatedSint32();
    assertEquals(emptyList(), builder.getRepeatedSint32List());
    assertEquals(singletonList(1), messageAfterBuild.getRepeatedSint32List());

    message = builder.build();
    builder.addRepeatedSint64(1L);
    assertEquals(emptyList(), message.getRepeatedSint64List());
    assertEquals(singletonList(1L), builder.getRepeatedSint64List());
    assertEquals(emptyList(), message.getRepeatedSint64List());
    messageAfterBuild = builder.build();
    builder.clearRepeatedSint64();
    assertEquals(emptyList(), builder.getRepeatedSint64List());
    assertEquals(singletonList(1L), messageAfterBuild.getRepeatedSint64List());

    message = builder.build();
    builder.addRepeatedString("hi");
    assertEquals(emptyList(), message.getRepeatedStringList());
    assertEquals(singletonList("hi"), builder.getRepeatedStringList());
    assertEquals(emptyList(), message.getRepeatedStringList());
    messageAfterBuild = builder.build();
    builder.clearRepeatedString();
    assertEquals(emptyList(), builder.getRepeatedStringList());
    assertEquals(
        singletonList("hi"), messageAfterBuild.getRepeatedStringList());

    message = builder.build();
    builder.addRepeatedStringPiece("hi");
    assertEquals(emptyList(), message.getRepeatedStringPieceList());
    assertEquals(singletonList("hi"), builder.getRepeatedStringPieceList());
    assertEquals(emptyList(), message.getRepeatedStringPieceList());
    messageAfterBuild = builder.build();
    builder.clearRepeatedStringPiece();
    assertEquals(emptyList(), builder.getRepeatedStringPieceList());
    assertEquals(
        singletonList("hi"), messageAfterBuild.getRepeatedStringPieceList());

    message = builder.build();
    builder.addRepeatedUint32(1);
    assertEquals(emptyList(), message.getRepeatedUint32List());
    assertEquals(singletonList(1), builder.getRepeatedUint32List());
    assertEquals(emptyList(), message.getRepeatedUint32List());
    messageAfterBuild = builder.build();
    builder.clearRepeatedUint32();
    assertEquals(emptyList(), builder.getRepeatedUint32List());
    assertEquals(singletonList(1), messageAfterBuild.getRepeatedUint32List());

    message = builder.build();
    builder.addRepeatedUint64(1L);
    assertEquals(emptyList(), message.getRepeatedUint64List());
    assertEquals(singletonList(1L), builder.getRepeatedUint64List());
    assertEquals(emptyList(), message.getRepeatedUint64List());
    messageAfterBuild = builder.build();
    builder.clearRepeatedUint64();
    assertEquals(emptyList(), builder.getRepeatedUint64List());
    assertEquals(singletonList(1L), messageAfterBuild.getRepeatedUint64List());
    
    message = builder.build();
    builder.addRepeatedBool(true);
    messageAfterBuild = builder.build();
    assertEquals(0, message.getRepeatedBoolCount());
    builder.setRepeatedBool(0, false);
    assertEquals(true, messageAfterBuild.getRepeatedBool(0));
    assertEquals(false, builder.getRepeatedBool(0));
    builder.clearRepeatedBool();
    
    message = builder.build();
    builder.addRepeatedBytes(ByteString.copyFromUtf8("hi"));
    messageAfterBuild = builder.build();
    assertEquals(0, message.getRepeatedBytesCount());
    builder.setRepeatedBytes(0, ByteString.EMPTY);
    assertEquals(
        ByteString.copyFromUtf8("hi"), messageAfterBuild.getRepeatedBytes(0));
    assertEquals(ByteString.EMPTY, builder.getRepeatedBytes(0));
    builder.clearRepeatedBytes();

    message = builder.build();
    builder.addRepeatedCord("hi");
    messageAfterBuild = builder.build();
    assertEquals(0, message.getRepeatedCordCount());
    builder.setRepeatedCord(0, "");
    assertEquals("hi", messageAfterBuild.getRepeatedCord(0));
    assertEquals("", builder.getRepeatedCord(0));
    builder.clearRepeatedCord();
    message = builder.build();

    builder.addRepeatedCordBytes(ByteString.copyFromUtf8("hi"));
    messageAfterBuild = builder.build();
    assertEquals(0, message.getRepeatedCordCount());
    builder.setRepeatedCord(0, "");
    assertEquals(
        ByteString.copyFromUtf8("hi"), messageAfterBuild.getRepeatedCordBytes(0));
    assertEquals(ByteString.EMPTY, builder.getRepeatedCordBytes(0));
    builder.clearRepeatedCord();

    message = builder.build();
    builder.addRepeatedDouble(1D);
    messageAfterBuild = builder.build();
    assertEquals(0, message.getRepeatedDoubleCount());
    builder.setRepeatedDouble(0, 0D);
    assertEquals(1D, messageAfterBuild.getRepeatedDouble(0));
    assertEquals(0D, builder.getRepeatedDouble(0));
    builder.clearRepeatedDouble();

    message = builder.build();
    builder.addRepeatedFixed32(1);
    messageAfterBuild = builder.build();
    assertEquals(0, message.getRepeatedFixed32Count());
    builder.setRepeatedFixed32(0, 0);
    assertEquals(1, messageAfterBuild.getRepeatedFixed32(0));
    assertEquals(0, builder.getRepeatedFixed32(0));
    builder.clearRepeatedFixed32();

    message = builder.build();
    builder.addRepeatedFixed64(1L);
    messageAfterBuild = builder.build();
    assertEquals(0, message.getRepeatedFixed64Count());
    builder.setRepeatedFixed64(0, 0L);
    assertEquals(1L, messageAfterBuild.getRepeatedFixed64(0));
    assertEquals(0L, builder.getRepeatedFixed64(0));
    builder.clearRepeatedFixed64();

    message = builder.build();
    builder.addRepeatedFloat(1F);
    messageAfterBuild = builder.build();
    assertEquals(0, message.getRepeatedFloatCount());
    builder.setRepeatedFloat(0, 0F);
    assertEquals(1F, messageAfterBuild.getRepeatedFloat(0));
    assertEquals(0F, builder.getRepeatedFloat(0));
    builder.clearRepeatedFloat();

    message = builder.build();
    builder.addRepeatedForeignEnum(ForeignEnumLite.FOREIGN_LITE_BAR);
    messageAfterBuild = builder.build();
    assertEquals(0, message.getRepeatedForeignEnumCount());
    builder.setRepeatedForeignEnum(0, ForeignEnumLite.FOREIGN_LITE_FOO);
    assertEquals(
        ForeignEnumLite.FOREIGN_LITE_BAR,
        messageAfterBuild.getRepeatedForeignEnum(0));
    assertEquals(
        ForeignEnumLite.FOREIGN_LITE_FOO, builder.getRepeatedForeignEnum(0));
    builder.clearRepeatedForeignEnum();

    message = builder.build();
    builder.addRepeatedForeignMessage(foreignMessage);
    messageAfterBuild = builder.build();
    assertEquals(0, message.getRepeatedForeignMessageCount());
    builder.setRepeatedForeignMessage(
        0, ForeignMessageLite.getDefaultInstance());
    assertEquals(
        foreignMessage, messageAfterBuild.getRepeatedForeignMessage(0));
    assertEquals(
        ForeignMessageLite.getDefaultInstance(),
        builder.getRepeatedForeignMessage(0));
    builder.clearRepeatedForeignMessage();
    
    message = builder.build();
    builder.addRepeatedForeignMessage(foreignMessageBuilder);
    messageAfterBuild = builder.build();
    assertEquals(0, message.getRepeatedForeignMessageCount());
    builder.setRepeatedForeignMessage(
        0, ForeignMessageLite.getDefaultInstance());
    // LITE_RUNTIME doesn't implement equals so we compare on a property.
    assertEquals(3, messageAfterBuild.getRepeatedForeignMessage(0).getC());
    assertEquals(
        ForeignMessageLite.getDefaultInstance(),
        builder.getRepeatedForeignMessage(0));
    builder.clearRepeatedForeignMessage();

    message = builder.build();
    builder.addRepeatedForeignMessage(0, foreignMessage);
    messageAfterBuild = builder.build();
    assertEquals(0, message.getRepeatedForeignMessageCount());
    builder.setRepeatedForeignMessage(0, foreignMessageBuilder);
    assertEquals(
        foreignMessage, messageAfterBuild.getRepeatedForeignMessage(0));
    // LITE_RUNTIME doesn't implement equals so we compare on a property.
    assertEquals(3, builder.getRepeatedForeignMessage(0).getC());
    builder.clearRepeatedForeignMessage();

    message = builder.build();
    RepeatedGroup repeatedGroup = RepeatedGroup.newBuilder()
        .setA(1)
        .build();
    builder.addRepeatedGroup(repeatedGroup);
    messageAfterBuild = builder.build();
    assertEquals(0, message.getRepeatedGroupCount());
    builder.setRepeatedGroup(0, RepeatedGroup.getDefaultInstance());
    assertEquals(repeatedGroup, messageAfterBuild.getRepeatedGroup(0));
    assertEquals(
        RepeatedGroup.getDefaultInstance(), builder.getRepeatedGroup(0));
    builder.clearRepeatedGroup();
    
    message = builder.build();
    builder.addRepeatedGroup(0, repeatedGroup);
    messageAfterBuild = builder.build();
    assertEquals(0, message.getRepeatedGroupCount());
    builder.setRepeatedGroup(0, RepeatedGroup.getDefaultInstance());
    assertEquals(repeatedGroup, messageAfterBuild.getRepeatedGroup(0));
    assertEquals(
        RepeatedGroup.getDefaultInstance(), builder.getRepeatedGroup(0));
    builder.clearRepeatedGroup();
    
    message = builder.build();
    RepeatedGroup.Builder repeatedGroupBuilder = RepeatedGroup.newBuilder()
        .setA(3);
    builder.addRepeatedGroup(repeatedGroupBuilder);
    messageAfterBuild = builder.build();
    assertEquals(0, message.getRepeatedGroupCount());
    builder.setRepeatedGroup(0, RepeatedGroup.getDefaultInstance());
    // LITE_RUNTIME doesn't implement equals so we compare on a property and
    // ensure the property isn't set on repeatedGroup.
    assertEquals(3, messageAfterBuild.getRepeatedGroup(0).getA());
    assertEquals(
        RepeatedGroup.getDefaultInstance(), builder.getRepeatedGroup(0));
    builder.clearRepeatedGroup();
    
    message = builder.build();
    builder.addRepeatedGroup(0, repeatedGroupBuilder);
    messageAfterBuild = builder.build();
    assertEquals(0, message.getRepeatedGroupCount());
    builder.setRepeatedGroup(0, RepeatedGroup.getDefaultInstance());
    // LITE_RUNTIME doesn't implement equals so we compare on a property and
    // ensure the property isn't set on repeatedGroup.
    assertEquals(3, messageAfterBuild.getRepeatedGroup(0).getA());
    assertEquals(
        RepeatedGroup.getDefaultInstance(), builder.getRepeatedGroup(0));
    builder.clearRepeatedGroup();

    message = builder.build();
    builder.addRepeatedInt32(1);
    messageAfterBuild = builder.build();
    assertEquals(0, message.getRepeatedInt32Count());
    builder.setRepeatedInt32(0, 0);
    assertEquals(1, messageAfterBuild.getRepeatedInt32(0));
    assertEquals(0, builder.getRepeatedInt32(0));
    builder.clearRepeatedInt32();

    message = builder.build();
    builder.addRepeatedInt64(1L);
    messageAfterBuild = builder.build();
    assertEquals(0L, message.getRepeatedInt64Count());
    builder.setRepeatedInt64(0, 0L);
    assertEquals(1L, messageAfterBuild.getRepeatedInt64(0));
    assertEquals(0L, builder.getRepeatedInt64(0));
    builder.clearRepeatedInt64();
    
    message = builder.build();
    builder.addRepeatedLazyMessage(nestedMessage);
    messageAfterBuild = builder.build();
    assertEquals(0, message.getRepeatedLazyMessageCount());
    builder.setRepeatedLazyMessage(0, NestedMessage.getDefaultInstance());
    assertEquals(nestedMessage, messageAfterBuild.getRepeatedLazyMessage(0));
    assertEquals(
        NestedMessage.getDefaultInstance(), builder.getRepeatedLazyMessage(0));
    builder.clearRepeatedLazyMessage();
    
    message = builder.build();
    builder.addRepeatedLazyMessage(0, nestedMessage);
    messageAfterBuild = builder.build();
    assertEquals(0, message.getRepeatedLazyMessageCount());
    builder.setRepeatedLazyMessage(0, NestedMessage.getDefaultInstance());
    assertEquals(nestedMessage, messageAfterBuild.getRepeatedLazyMessage(0));
    assertEquals(
        NestedMessage.getDefaultInstance(), builder.getRepeatedLazyMessage(0));
    builder.clearRepeatedLazyMessage();
    
    message = builder.build();
    builder.addRepeatedLazyMessage(nestedMessageBuilder);
    messageAfterBuild = builder.build();
    assertEquals(0, message.getRepeatedLazyMessageCount());
    builder.setRepeatedLazyMessage(0, NestedMessage.getDefaultInstance());
    // LITE_RUNTIME doesn't implement equals so we compare on a property and
    // ensure the property isn't set on repeatedGroup.
    assertEquals(3, messageAfterBuild.getRepeatedLazyMessage(0).getBb());
    assertEquals(
        NestedMessage.getDefaultInstance(), builder.getRepeatedLazyMessage(0));
    builder.clearRepeatedLazyMessage();
    
    message = builder.build();
    builder.addRepeatedLazyMessage(0, nestedMessageBuilder);
    messageAfterBuild = builder.build();
    assertEquals(0, message.getRepeatedLazyMessageCount());
    builder.setRepeatedLazyMessage(0, NestedMessage.getDefaultInstance());
    // LITE_RUNTIME doesn't implement equals so we compare on a property and
    // ensure the property isn't set on repeatedGroup.
    assertEquals(3, messageAfterBuild.getRepeatedLazyMessage(0).getBb());
    assertEquals(
        NestedMessage.getDefaultInstance(), builder.getRepeatedLazyMessage(0));
    builder.clearRepeatedLazyMessage();

    message = builder.build();
    builder.addRepeatedSfixed32(1);
    messageAfterBuild = builder.build();
    assertEquals(0, message.getRepeatedSfixed32Count());
    builder.setRepeatedSfixed32(0, 0);
    assertEquals(1, messageAfterBuild.getRepeatedSfixed32(0));
    assertEquals(0, builder.getRepeatedSfixed32(0));
    builder.clearRepeatedSfixed32();

    message = builder.build();
    builder.addRepeatedSfixed64(1L);
    messageAfterBuild = builder.build();
    assertEquals(0L, message.getRepeatedSfixed64Count());
    builder.setRepeatedSfixed64(0, 0L);
    assertEquals(1L, messageAfterBuild.getRepeatedSfixed64(0));
    assertEquals(0L, builder.getRepeatedSfixed64(0));
    builder.clearRepeatedSfixed64();

    message = builder.build();
    builder.addRepeatedSint32(1);
    messageAfterBuild = builder.build();
    assertEquals(0, message.getRepeatedSint32Count());
    builder.setRepeatedSint32(0, 0);
    assertEquals(1, messageAfterBuild.getRepeatedSint32(0));
    assertEquals(0, builder.getRepeatedSint32(0));
    builder.clearRepeatedSint32();

    message = builder.build();
    builder.addRepeatedSint64(1L);
    messageAfterBuild = builder.build();
    assertEquals(0L, message.getRepeatedSint64Count());
    builder.setRepeatedSint64(0, 0L);
    assertEquals(1L, messageAfterBuild.getRepeatedSint64(0));
    assertEquals(0L, builder.getRepeatedSint64(0));
    builder.clearRepeatedSint64();

    message = builder.build();
    builder.addRepeatedString("hi");
    messageAfterBuild = builder.build();
    assertEquals(0L, message.getRepeatedStringCount());
    builder.setRepeatedString(0, "");
    assertEquals("hi", messageAfterBuild.getRepeatedString(0));
    assertEquals("", builder.getRepeatedString(0));
    builder.clearRepeatedString();

    message = builder.build();
    builder.addRepeatedStringBytes(ByteString.copyFromUtf8("hi"));
    messageAfterBuild = builder.build();
    assertEquals(0L, message.getRepeatedStringCount());
    builder.setRepeatedString(0, "");
    assertEquals(
        ByteString.copyFromUtf8("hi"),
        messageAfterBuild.getRepeatedStringBytes(0));
    assertEquals(ByteString.EMPTY, builder.getRepeatedStringBytes(0));
    builder.clearRepeatedString();

    message = builder.build();
    builder.addRepeatedStringPiece("hi");
    messageAfterBuild = builder.build();
    assertEquals(0L, message.getRepeatedStringPieceCount());
    builder.setRepeatedStringPiece(0, "");
    assertEquals("hi", messageAfterBuild.getRepeatedStringPiece(0));
    assertEquals("", builder.getRepeatedStringPiece(0));
    builder.clearRepeatedStringPiece();

    message = builder.build();
    builder.addRepeatedStringPieceBytes(ByteString.copyFromUtf8("hi"));
    messageAfterBuild = builder.build();
    assertEquals(0L, message.getRepeatedStringPieceCount());
    builder.setRepeatedStringPiece(0, "");
    assertEquals(
        ByteString.copyFromUtf8("hi"),
        messageAfterBuild.getRepeatedStringPieceBytes(0));
    assertEquals(ByteString.EMPTY, builder.getRepeatedStringPieceBytes(0));
    builder.clearRepeatedStringPiece();

    message = builder.build();
    builder.addRepeatedUint32(1);
    messageAfterBuild = builder.build();
    assertEquals(0, message.getRepeatedUint32Count());
    builder.setRepeatedUint32(0, 0);
    assertEquals(1, messageAfterBuild.getRepeatedUint32(0));
    assertEquals(0, builder.getRepeatedUint32(0));
    builder.clearRepeatedUint32();

    message = builder.build();
    builder.addRepeatedUint64(1L);
    messageAfterBuild = builder.build();
    assertEquals(0L, message.getRepeatedUint64Count());
    builder.setRepeatedUint64(0, 0L);
    assertEquals(1L, messageAfterBuild.getRepeatedUint64(0));
    assertEquals(0L, builder.getRepeatedUint64(0));
    builder.clearRepeatedUint64();

    message = builder.build();
    assertEquals(0, message.getSerializedSize());
    builder.mergeFrom(TestAllTypesLite.newBuilder()
        .setOptionalBool(true)
        .build());
    assertEquals(0, message.getSerializedSize());
    assertEquals(true, builder.build().getOptionalBool());
    builder.clearOptionalBool();

    message = builder.build();
    assertEquals(0, message.getSerializedSize());
    builder.mergeFrom(TestAllTypesLite.newBuilder()
        .setOptionalBool(true)
        .build());
    assertEquals(0, message.getSerializedSize());
    assertEquals(true, builder.build().getOptionalBool());
    builder.clear();
    assertEquals(0, builder.build().getSerializedSize());

    message = builder.build();
    assertEquals(0, message.getSerializedSize());
    builder.mergeOptionalForeignMessage(foreignMessage);
    assertEquals(0, message.getSerializedSize());
    assertEquals(
        foreignMessage.getC(),
        builder.build().getOptionalForeignMessage().getC());
    builder.clearOptionalForeignMessage();

    message = builder.build();
    assertEquals(0, message.getSerializedSize());
    builder.mergeOptionalLazyMessage(nestedMessage);
    assertEquals(0, message.getSerializedSize());
    assertEquals(
        nestedMessage.getBb(),
        builder.build().getOptionalLazyMessage().getBb());
    builder.clearOptionalLazyMessage();
    
    message = builder.build();
    builder.setOneofString("hi");
    assertEquals(
        OneofFieldCase.ONEOFFIELD_NOT_SET, message.getOneofFieldCase());
    assertEquals(OneofFieldCase.ONEOF_STRING, builder.getOneofFieldCase());
    assertEquals("hi", builder.getOneofString());
    messageAfterBuild = builder.build();
    assertEquals(
        OneofFieldCase.ONEOF_STRING, messageAfterBuild.getOneofFieldCase());
    assertEquals("hi", messageAfterBuild.getOneofString());
    builder.setOneofUint32(1);
    assertEquals(
        OneofFieldCase.ONEOF_STRING, messageAfterBuild.getOneofFieldCase());
    assertEquals("hi", messageAfterBuild.getOneofString());
    assertEquals(OneofFieldCase.ONEOF_UINT32, builder.getOneofFieldCase());
    assertEquals(1, builder.getOneofUint32());
    TestAllTypesLiteOrBuilder messageOrBuilder = builder;
    assertEquals(OneofFieldCase.ONEOF_UINT32, messageOrBuilder.getOneofFieldCase());
    
    TestAllExtensionsLite.Builder extendableMessageBuilder =
        TestAllExtensionsLite.newBuilder();
    TestAllExtensionsLite extendableMessage = extendableMessageBuilder.build();
    extendableMessageBuilder.setExtension(
        UnittestLite.optionalInt32ExtensionLite, 1);
    assertFalse(extendableMessage.hasExtension(
        UnittestLite.optionalInt32ExtensionLite));
    extendableMessage = extendableMessageBuilder.build();
    assertEquals(
        1, (int) extendableMessageBuilder.getExtension(
            UnittestLite.optionalInt32ExtensionLite));
    assertEquals(
        1, (int) extendableMessage.getExtension(
            UnittestLite.optionalInt32ExtensionLite));
    extendableMessageBuilder.setExtension(
        UnittestLite.optionalInt32ExtensionLite, 3);
    assertEquals(
        3, (int) extendableMessageBuilder.getExtension(
            UnittestLite.optionalInt32ExtensionLite));
    assertEquals(
        1, (int) extendableMessage.getExtension(
            UnittestLite.optionalInt32ExtensionLite));
    extendableMessage = extendableMessageBuilder.build();
    assertEquals(
        3, (int) extendableMessageBuilder.getExtension(
            UnittestLite.optionalInt32ExtensionLite));
    assertEquals(
        3, (int) extendableMessage.getExtension(
            UnittestLite.optionalInt32ExtensionLite));
    
    // No extension registry, so it should be in unknown fields.
    extendableMessage =
        TestAllExtensionsLite.parseFrom(extendableMessage.toByteArray());
    assertFalse(extendableMessage.hasExtension(
        UnittestLite.optionalInt32ExtensionLite));
    
    extendableMessageBuilder = extendableMessage.toBuilder();
    extendableMessageBuilder.mergeFrom(TestAllExtensionsLite.newBuilder()
        .setExtension(UnittestLite.optionalFixed32ExtensionLite, 11)
        .build());
    
    extendableMessage = extendableMessageBuilder.build();
    ExtensionRegistryLite registry = ExtensionRegistryLite.newInstance();
    UnittestLite.registerAllExtensions(registry);
    extendableMessage = TestAllExtensionsLite.parseFrom(
        extendableMessage.toByteArray(), registry);
    
    // The unknown field was preserved.
    assertEquals(
        3, (int) extendableMessage.getExtension(
            UnittestLite.optionalInt32ExtensionLite));
    assertEquals(
        11, (int) extendableMessage.getExtension(
            UnittestLite.optionalFixed32ExtensionLite));
  }
}
