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

import com.google.protobuf.FieldPresenceTestProto.TestAllTypes;
import com.google.protobuf.UnittestImportLite.ImportEnumLite;
import com.google.protobuf.UnittestImportPublicLite.PublicImportMessageLite;
import com.google.protobuf.UnittestLite.ForeignEnumLite;
import com.google.protobuf.UnittestLite.ForeignMessageLite;
import com.google.protobuf.UnittestLite.TestAllExtensionsLite;
import com.google.protobuf.UnittestLite.TestAllTypesLite;
import com.google.protobuf.UnittestLite.TestAllTypesLite.NestedEnum;
import com.google.protobuf.UnittestLite.TestAllTypesLite.NestedMessage;
import com.google.protobuf.UnittestLite.TestAllTypesLite.NestedMessage2;
import com.google.protobuf.UnittestLite.TestAllTypesLite.OneofFieldCase;
import com.google.protobuf.UnittestLite.TestAllTypesLite.OptionalGroup;
import com.google.protobuf.UnittestLite.TestAllTypesLite.RepeatedGroup;
import com.google.protobuf.UnittestLite.TestAllTypesLiteOrBuilder;
import com.google.protobuf.UnittestLite.TestHugeFieldNumbersLite;
import com.google.protobuf.UnittestLite.TestNestedExtensionLite;
import map_lite_test.MapTestProto.TestMap;
import map_lite_test.MapTestProto.TestMap.MessageValue;
import protobuf_unittest.NestedExtensionLite;
import protobuf_unittest.NonNestedExtensionLite;
import protobuf_unittest.lite_equals_and_hash.LiteEqualsAndHash.Bar;
import protobuf_unittest.lite_equals_and_hash.LiteEqualsAndHash.BarPrime;
import protobuf_unittest.lite_equals_and_hash.LiteEqualsAndHash.Foo;
import protobuf_unittest.lite_equals_and_hash.LiteEqualsAndHash.TestOneofEquals;
import protobuf_unittest.lite_equals_and_hash.LiteEqualsAndHash.TestRecursiveOneof;
import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.OutputStream;
import java.lang.reflect.Field;
import java.nio.ByteBuffer;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Iterator;
import java.util.List;
import junit.framework.TestCase;

/**
 * Test lite runtime.
 *
 * @author kenton@google.com Kenton Varda
 */
public class LiteTest extends TestCase {
  @Override
  public void setUp() throws Exception {
    // Test that nested extensions are initialized correctly even if the outer
    // class has not been accessed directly.  This was once a bug with lite
    // messages.
    //
    // We put this in setUp() rather than in its own test method because we
    // need to make sure it runs before any actual tests.
    assertNotNull(TestNestedExtensionLite.nestedExtension);
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
            .setOptionalNestedMessage(NestedMessage.newBuilder().setBb(7))
            .build();

    ByteString data = message.toByteString();

    TestAllTypesLite message2 = TestAllTypesLite.parseFrom(data);

    assertEquals(123, message2.getOptionalInt32());
    assertEquals(1, message2.getRepeatedStringCount());
    assertEquals("hello", message2.getRepeatedString(0));
    assertEquals(7, message2.getOptionalNestedMessage().getBb());
  }

  public void testLite_unknownEnumAtListBoundary() throws Exception {
    ByteArrayOutputStream byteStream = new ByteArrayOutputStream();
    CodedOutputStream output = CodedOutputStream.newInstance(byteStream);
    for (int i = 0; i < AbstractProtobufList.DEFAULT_CAPACITY; i++) {
      output.writeInt32(TestAllTypesLite.REPEATED_NESTED_ENUM_FIELD_NUMBER, 1);
    }
    // 0 is not a valid enum value for NestedEnum
    output.writeInt32(TestAllTypesLite.REPEATED_NESTED_ENUM_FIELD_NUMBER, 0);
    output.flush();
    // This tests a bug we had once with removal right at the boundary of the array. It would throw
    // at runtime so no need to assert.
    TestAllTypesLite.parseFrom(new ByteArrayInputStream(byteStream.toByteArray()));
  }

  public void testLiteExtensions() throws Exception {
    // TODO(kenton):  Unlike other features of the lite library, extensions are
    //   implemented completely differently from the regular library.  We
    //   should probably test them more thoroughly.

    TestAllExtensionsLite message =
        TestAllExtensionsLite.newBuilder()
            .setExtension(UnittestLite.optionalInt32ExtensionLite, 123)
            .addExtension(UnittestLite.repeatedStringExtensionLite, "hello")
            .setExtension(UnittestLite.optionalNestedEnumExtensionLite, NestedEnum.BAZ)
            .setExtension(
                UnittestLite.optionalNestedMessageExtensionLite,
                NestedMessage.newBuilder().setBb(7).build())
            .build();

    // Test copying a message, since coping extensions actually does use a
    // different code path between lite and regular libraries, and as of this
    // writing, parsing hasn't been implemented yet.
    TestAllExtensionsLite message2 = message.toBuilder().build();

    assertEquals(123, (int) message2.getExtension(UnittestLite.optionalInt32ExtensionLite));
    assertEquals(1, message2.getExtensionCount(UnittestLite.repeatedStringExtensionLite));
    assertEquals(1, message2.getExtension(UnittestLite.repeatedStringExtensionLite).size());
    assertEquals("hello", message2.getExtension(UnittestLite.repeatedStringExtensionLite, 0));
    assertEquals(
        NestedEnum.BAZ, message2.getExtension(UnittestLite.optionalNestedEnumExtensionLite));
    assertEquals(7, message2.getExtension(UnittestLite.optionalNestedMessageExtensionLite).getBb());
  }

  public void testClone() {
    TestAllTypesLite.Builder expected = TestAllTypesLite.newBuilder().setOptionalInt32(123);
    assertEquals(expected.getOptionalInt32(), expected.clone().getOptionalInt32());

    TestAllExtensionsLite.Builder expected2 =
        TestAllExtensionsLite.newBuilder()
            .setExtension(UnittestLite.optionalInt32ExtensionLite, 123);
    assertEquals(
        expected2.getExtension(UnittestLite.optionalInt32ExtensionLite),
        expected2.clone().getExtension(UnittestLite.optionalInt32ExtensionLite));
  }

  public void testAddAll() {
    try {
      TestAllTypesLite.newBuilder().addAllRepeatedBytes(null);
      fail();
    } catch (NullPointerException e) {
      // expected.
    }
  }

  public void testMemoization() throws Exception {
    TestAllExtensionsLite message = TestUtilLite.getAllLiteExtensionsSet();

    // Test serialized size is memoized
    message.memoizedSerializedSize = -1;
    int size = message.getSerializedSize();
    assertTrue(size > 0);
    assertEquals(size, message.memoizedSerializedSize);

    // Test hashCode is memoized
    assertEquals(0, message.memoizedHashCode);
    int hashCode = message.hashCode();
    assertTrue(hashCode != 0);
    assertEquals(hashCode, message.memoizedHashCode);

    // Test isInitialized is memoized
    Field memo = message.getClass().getDeclaredField("memoizedIsInitialized");
    memo.setAccessible(true);
    memo.set(message, (byte) -1);
    boolean initialized = message.isInitialized();
    assertTrue(initialized);
    // We have to cast to Byte first. Casting to byte causes a type error
    assertEquals(1, ((Byte) memo.get(message)).intValue());
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
    assertEquals(ByteString.copyFromUtf8("hi"), messageAfterBuild.getOptionalBytes());
    assertEquals(ByteString.EMPTY, message.getOptionalBytes());
    builder.clearOptionalBytes();
    assertEquals(ByteString.EMPTY, builder.getOptionalBytes());
    assertEquals(ByteString.copyFromUtf8("hi"), messageAfterBuild.getOptionalBytes());

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
    assertEquals(ByteString.copyFromUtf8("no"), messageAfterBuild.getOptionalCordBytes());
    assertEquals(ByteString.EMPTY, message.getOptionalCordBytes());
    builder.clearOptionalCord();
    assertEquals(ByteString.EMPTY, builder.getOptionalCordBytes());
    assertEquals(ByteString.copyFromUtf8("no"), messageAfterBuild.getOptionalCordBytes());

    message = builder.build();
    builder.setOptionalDouble(1);
    assertEquals(0D, message.getOptionalDouble(), 0.0);
    assertEquals(1D, builder.getOptionalDouble(), 0.0);
    messageAfterBuild = builder.build();
    assertEquals(1D, messageAfterBuild.getOptionalDouble(), 0.0);
    assertEquals(0D, message.getOptionalDouble(), 0.0);
    builder.clearOptionalDouble();
    assertEquals(0D, builder.getOptionalDouble(), 0.0);
    assertEquals(1D, messageAfterBuild.getOptionalDouble(), 0.0);

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
    assertEquals(0F, message.getOptionalFloat(), 0.0f);
    assertEquals(1F, builder.getOptionalFloat(), 0.0f);
    messageAfterBuild = builder.build();
    assertEquals(1F, messageAfterBuild.getOptionalFloat(), 0.0f);
    assertEquals(0F, message.getOptionalFloat(), 0.0f);
    builder.clearOptionalFloat();
    assertEquals(0F, builder.getOptionalFloat(), 0.0f);
    assertEquals(1F, messageAfterBuild.getOptionalFloat(), 0.0f);

    message = builder.build();
    builder.setOptionalForeignEnum(ForeignEnumLite.FOREIGN_LITE_BAR);
    assertEquals(ForeignEnumLite.FOREIGN_LITE_FOO, message.getOptionalForeignEnum());
    assertEquals(ForeignEnumLite.FOREIGN_LITE_BAR, builder.getOptionalForeignEnum());
    messageAfterBuild = builder.build();
    assertEquals(ForeignEnumLite.FOREIGN_LITE_BAR, messageAfterBuild.getOptionalForeignEnum());
    assertEquals(ForeignEnumLite.FOREIGN_LITE_FOO, message.getOptionalForeignEnum());
    builder.clearOptionalForeignEnum();
    assertEquals(ForeignEnumLite.FOREIGN_LITE_FOO, builder.getOptionalForeignEnum());
    assertEquals(ForeignEnumLite.FOREIGN_LITE_BAR, messageAfterBuild.getOptionalForeignEnum());

    message = builder.build();
    ForeignMessageLite foreignMessage = ForeignMessageLite.newBuilder().setC(1).build();
    builder.setOptionalForeignMessage(foreignMessage);
    assertEquals(ForeignMessageLite.getDefaultInstance(), message.getOptionalForeignMessage());
    assertEquals(foreignMessage, builder.getOptionalForeignMessage());
    messageAfterBuild = builder.build();
    assertEquals(foreignMessage, messageAfterBuild.getOptionalForeignMessage());
    assertEquals(ForeignMessageLite.getDefaultInstance(), message.getOptionalForeignMessage());
    builder.clearOptionalForeignMessage();
    assertEquals(ForeignMessageLite.getDefaultInstance(), builder.getOptionalForeignMessage());
    assertEquals(foreignMessage, messageAfterBuild.getOptionalForeignMessage());

    message = builder.build();
    ForeignMessageLite foreignMessageC3 = ForeignMessageLite.newBuilder().setC(3).build();
    builder.setOptionalForeignMessage(foreignMessageC3);
    assertEquals(ForeignMessageLite.getDefaultInstance(), message.getOptionalForeignMessage());
    assertEquals(foreignMessageC3, builder.getOptionalForeignMessage());
    messageAfterBuild = builder.build();
    assertEquals(foreignMessageC3, messageAfterBuild.getOptionalForeignMessage());
    assertEquals(ForeignMessageLite.getDefaultInstance(), message.getOptionalForeignMessage());
    builder.clearOptionalForeignMessage();
    assertEquals(ForeignMessageLite.getDefaultInstance(), builder.getOptionalForeignMessage());
    assertEquals(foreignMessageC3, messageAfterBuild.getOptionalForeignMessage());

    message = builder.build();
    OptionalGroup optionalGroup = OptionalGroup.newBuilder().setA(1).build();
    builder.setOptionalGroup(optionalGroup);
    assertEquals(OptionalGroup.getDefaultInstance(), message.getOptionalGroup());
    assertEquals(optionalGroup, builder.getOptionalGroup());
    messageAfterBuild = builder.build();
    assertEquals(optionalGroup, messageAfterBuild.getOptionalGroup());
    assertEquals(OptionalGroup.getDefaultInstance(), message.getOptionalGroup());
    builder.clearOptionalGroup();
    assertEquals(OptionalGroup.getDefaultInstance(), builder.getOptionalGroup());
    assertEquals(optionalGroup, messageAfterBuild.getOptionalGroup());

    message = builder.build();
    OptionalGroup.Builder optionalGroupBuilder = OptionalGroup.newBuilder().setA(3);
    builder.setOptionalGroup(optionalGroupBuilder);
    assertEquals(OptionalGroup.getDefaultInstance(), message.getOptionalGroup());
    assertEquals(optionalGroupBuilder.build(), builder.getOptionalGroup());
    messageAfterBuild = builder.build();
    assertEquals(optionalGroupBuilder.build(), messageAfterBuild.getOptionalGroup());
    assertEquals(OptionalGroup.getDefaultInstance(), message.getOptionalGroup());
    builder.clearOptionalGroup();
    assertEquals(OptionalGroup.getDefaultInstance(), builder.getOptionalGroup());
    assertEquals(optionalGroupBuilder.build(), messageAfterBuild.getOptionalGroup());

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
    NestedMessage nestedMessage = NestedMessage.newBuilder().setBb(1).build();
    builder.setOptionalLazyMessage(nestedMessage);
    assertEquals(NestedMessage.getDefaultInstance(), message.getOptionalLazyMessage());
    assertEquals(nestedMessage, builder.getOptionalLazyMessage());
    messageAfterBuild = builder.build();
    assertEquals(nestedMessage, messageAfterBuild.getOptionalLazyMessage());
    assertEquals(NestedMessage.getDefaultInstance(), message.getOptionalLazyMessage());
    builder.clearOptionalLazyMessage();
    assertEquals(NestedMessage.getDefaultInstance(), builder.getOptionalLazyMessage());
    assertEquals(nestedMessage, messageAfterBuild.getOptionalLazyMessage());

    message = builder.build();
    NestedMessage.Builder nestedMessageBuilder = NestedMessage.newBuilder().setBb(3);
    builder.setOptionalLazyMessage(nestedMessageBuilder);
    assertEquals(NestedMessage.getDefaultInstance(), message.getOptionalLazyMessage());
    assertEquals(nestedMessageBuilder.build(), builder.getOptionalLazyMessage());
    messageAfterBuild = builder.build();
    assertEquals(nestedMessageBuilder.build(), messageAfterBuild.getOptionalLazyMessage());
    assertEquals(NestedMessage.getDefaultInstance(), message.getOptionalLazyMessage());
    builder.clearOptionalLazyMessage();
    assertEquals(NestedMessage.getDefaultInstance(), builder.getOptionalLazyMessage());
    assertEquals(nestedMessageBuilder.build(), messageAfterBuild.getOptionalLazyMessage());

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
    assertEquals(ByteString.copyFromUtf8("no"), builder.getOptionalStringBytes());
    messageAfterBuild = builder.build();
    assertEquals(ByteString.copyFromUtf8("no"), messageAfterBuild.getOptionalStringBytes());
    assertEquals(ByteString.EMPTY, message.getOptionalStringBytes());
    builder.clearOptionalString();
    assertEquals(ByteString.EMPTY, builder.getOptionalStringBytes());
    assertEquals(ByteString.copyFromUtf8("no"), messageAfterBuild.getOptionalStringBytes());

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
    assertEquals(ByteString.copyFromUtf8("no"), builder.getOptionalStringPieceBytes());
    messageAfterBuild = builder.build();
    assertEquals(ByteString.copyFromUtf8("no"), messageAfterBuild.getOptionalStringPieceBytes());
    assertEquals(ByteString.EMPTY, message.getOptionalStringPieceBytes());
    builder.clearOptionalStringPiece();
    assertEquals(ByteString.EMPTY, builder.getOptionalStringPieceBytes());
    assertEquals(ByteString.copyFromUtf8("no"), messageAfterBuild.getOptionalStringPieceBytes());

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
    assertEquals(singletonList(ByteString.copyFromUtf8("hi")), builder.getRepeatedBytesList());
    assertEquals(emptyList(), message.getRepeatedBytesList());
    messageAfterBuild = builder.build();
    builder.clearRepeatedBytes();
    assertEquals(emptyList(), builder.getRepeatedBytesList());
    assertEquals(
        singletonList(ByteString.copyFromUtf8("hi")), messageAfterBuild.getRepeatedBytesList());

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
    builder.addAllRepeatedForeignEnum(singletonList(ForeignEnumLite.FOREIGN_LITE_BAR));
    assertEquals(emptyList(), message.getRepeatedForeignEnumList());
    assertEquals(
        singletonList(ForeignEnumLite.FOREIGN_LITE_BAR), builder.getRepeatedForeignEnumList());
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
    assertEquals(singletonList(foreignMessage), builder.getRepeatedForeignMessageList());
    assertEquals(emptyList(), message.getRepeatedForeignMessageList());
    messageAfterBuild = builder.build();
    builder.clearRepeatedForeignMessage();
    assertEquals(emptyList(), builder.getRepeatedForeignMessageList());
    assertEquals(singletonList(foreignMessage), messageAfterBuild.getRepeatedForeignMessageList());

    message = builder.build();
    builder.addAllRepeatedGroup(singletonList(RepeatedGroup.getDefaultInstance()));
    assertEquals(emptyList(), message.getRepeatedGroupList());
    assertEquals(singletonList(RepeatedGroup.getDefaultInstance()), builder.getRepeatedGroupList());
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
    assertEquals(singletonList(nestedMessage), builder.getRepeatedLazyMessageList());
    assertEquals(emptyList(), message.getRepeatedLazyMessageList());
    messageAfterBuild = builder.build();
    builder.clearRepeatedLazyMessage();
    assertEquals(emptyList(), builder.getRepeatedLazyMessageList());
    assertEquals(singletonList(nestedMessage), messageAfterBuild.getRepeatedLazyMessageList());

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
    assertEquals(singletonList(1L), messageAfterBuild.getRepeatedSfixed64List());

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
    assertEquals(singletonList("hi"), messageAfterBuild.getRepeatedStringList());

    message = builder.build();
    builder.addAllRepeatedStringPiece(singletonList("hi"));
    assertEquals(emptyList(), message.getRepeatedStringPieceList());
    assertEquals(singletonList("hi"), builder.getRepeatedStringPieceList());
    assertEquals(emptyList(), message.getRepeatedStringPieceList());
    messageAfterBuild = builder.build();
    builder.clearRepeatedStringPiece();
    assertEquals(emptyList(), builder.getRepeatedStringPieceList());
    assertEquals(singletonList("hi"), messageAfterBuild.getRepeatedStringPieceList());

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
    assertEquals(singletonList(ByteString.copyFromUtf8("hi")), builder.getRepeatedBytesList());
    assertEquals(emptyList(), message.getRepeatedBytesList());
    messageAfterBuild = builder.build();
    builder.clearRepeatedBytes();
    assertEquals(emptyList(), builder.getRepeatedBytesList());
    assertEquals(
        singletonList(ByteString.copyFromUtf8("hi")), messageAfterBuild.getRepeatedBytesList());

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
        singletonList(ForeignEnumLite.FOREIGN_LITE_BAR), builder.getRepeatedForeignEnumList());
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
    assertEquals(singletonList(foreignMessage), builder.getRepeatedForeignMessageList());
    assertEquals(emptyList(), message.getRepeatedForeignMessageList());
    messageAfterBuild = builder.build();
    builder.removeRepeatedForeignMessage(0);
    assertEquals(emptyList(), builder.getRepeatedForeignMessageList());
    assertEquals(singletonList(foreignMessage), messageAfterBuild.getRepeatedForeignMessageList());

    message = builder.build();
    builder.addRepeatedGroup(RepeatedGroup.getDefaultInstance());
    assertEquals(emptyList(), message.getRepeatedGroupList());
    assertEquals(singletonList(RepeatedGroup.getDefaultInstance()), builder.getRepeatedGroupList());
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
    assertEquals(singletonList(nestedMessage), builder.getRepeatedLazyMessageList());
    assertEquals(emptyList(), message.getRepeatedLazyMessageList());
    messageAfterBuild = builder.build();
    builder.removeRepeatedLazyMessage(0);
    assertEquals(emptyList(), builder.getRepeatedLazyMessageList());
    assertEquals(singletonList(nestedMessage), messageAfterBuild.getRepeatedLazyMessageList());

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
    assertEquals(singletonList(1L), messageAfterBuild.getRepeatedSfixed64List());

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
    assertEquals(singletonList("hi"), messageAfterBuild.getRepeatedStringList());

    message = builder.build();
    builder.addRepeatedStringPiece("hi");
    assertEquals(emptyList(), message.getRepeatedStringPieceList());
    assertEquals(singletonList("hi"), builder.getRepeatedStringPieceList());
    assertEquals(emptyList(), message.getRepeatedStringPieceList());
    messageAfterBuild = builder.build();
    builder.clearRepeatedStringPiece();
    assertEquals(emptyList(), builder.getRepeatedStringPieceList());
    assertEquals(singletonList("hi"), messageAfterBuild.getRepeatedStringPieceList());

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
    assertEquals(ByteString.copyFromUtf8("hi"), messageAfterBuild.getRepeatedBytes(0));
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
    assertEquals(ByteString.copyFromUtf8("hi"), messageAfterBuild.getRepeatedCordBytes(0));
    assertEquals(ByteString.EMPTY, builder.getRepeatedCordBytes(0));
    builder.clearRepeatedCord();

    message = builder.build();
    builder.addRepeatedDouble(1D);
    messageAfterBuild = builder.build();
    assertEquals(0, message.getRepeatedDoubleCount());
    builder.setRepeatedDouble(0, 0D);
    assertEquals(1D, messageAfterBuild.getRepeatedDouble(0), 0.0);
    assertEquals(0D, builder.getRepeatedDouble(0), 0.0);
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
    assertEquals(1F, messageAfterBuild.getRepeatedFloat(0), 0.0f);
    assertEquals(0F, builder.getRepeatedFloat(0), 0.0f);
    builder.clearRepeatedFloat();

    message = builder.build();
    builder.addRepeatedForeignEnum(ForeignEnumLite.FOREIGN_LITE_BAR);
    messageAfterBuild = builder.build();
    assertEquals(0, message.getRepeatedForeignEnumCount());
    builder.setRepeatedForeignEnum(0, ForeignEnumLite.FOREIGN_LITE_FOO);
    assertEquals(ForeignEnumLite.FOREIGN_LITE_BAR, messageAfterBuild.getRepeatedForeignEnum(0));
    assertEquals(ForeignEnumLite.FOREIGN_LITE_FOO, builder.getRepeatedForeignEnum(0));
    builder.clearRepeatedForeignEnum();

    message = builder.build();
    builder.addRepeatedForeignMessage(foreignMessage);
    messageAfterBuild = builder.build();
    assertEquals(0, message.getRepeatedForeignMessageCount());
    builder.setRepeatedForeignMessage(0, ForeignMessageLite.getDefaultInstance());
    assertEquals(foreignMessage, messageAfterBuild.getRepeatedForeignMessage(0));
    assertEquals(ForeignMessageLite.getDefaultInstance(), builder.getRepeatedForeignMessage(0));
    builder.clearRepeatedForeignMessage();

    message = builder.build();
    builder.addRepeatedForeignMessage(foreignMessageC3);
    messageAfterBuild = builder.build();
    assertEquals(0, message.getRepeatedForeignMessageCount());
    builder.setRepeatedForeignMessage(0, ForeignMessageLite.getDefaultInstance());
    assertEquals(foreignMessageC3, messageAfterBuild.getRepeatedForeignMessage(0));
    assertEquals(ForeignMessageLite.getDefaultInstance(), builder.getRepeatedForeignMessage(0));
    builder.clearRepeatedForeignMessage();

    message = builder.build();
    builder.addRepeatedForeignMessage(0, foreignMessage);
    messageAfterBuild = builder.build();
    assertEquals(0, message.getRepeatedForeignMessageCount());
    builder.setRepeatedForeignMessage(0, foreignMessageC3);
    assertEquals(foreignMessage, messageAfterBuild.getRepeatedForeignMessage(0));
    assertEquals(foreignMessageC3, builder.getRepeatedForeignMessage(0));
    builder.clearRepeatedForeignMessage();

    message = builder.build();
    RepeatedGroup repeatedGroup = RepeatedGroup.newBuilder().setA(1).build();
    builder.addRepeatedGroup(repeatedGroup);
    messageAfterBuild = builder.build();
    assertEquals(0, message.getRepeatedGroupCount());
    builder.setRepeatedGroup(0, RepeatedGroup.getDefaultInstance());
    assertEquals(repeatedGroup, messageAfterBuild.getRepeatedGroup(0));
    assertEquals(RepeatedGroup.getDefaultInstance(), builder.getRepeatedGroup(0));
    builder.clearRepeatedGroup();

    message = builder.build();
    builder.addRepeatedGroup(0, repeatedGroup);
    messageAfterBuild = builder.build();
    assertEquals(0, message.getRepeatedGroupCount());
    builder.setRepeatedGroup(0, RepeatedGroup.getDefaultInstance());
    assertEquals(repeatedGroup, messageAfterBuild.getRepeatedGroup(0));
    assertEquals(RepeatedGroup.getDefaultInstance(), builder.getRepeatedGroup(0));
    builder.clearRepeatedGroup();

    message = builder.build();
    RepeatedGroup.Builder repeatedGroupBuilder = RepeatedGroup.newBuilder().setA(3);
    builder.addRepeatedGroup(repeatedGroupBuilder);
    messageAfterBuild = builder.build();
    assertEquals(0, message.getRepeatedGroupCount());
    builder.setRepeatedGroup(0, RepeatedGroup.getDefaultInstance());
    assertEquals(repeatedGroupBuilder.build(), messageAfterBuild.getRepeatedGroup(0));
    assertEquals(RepeatedGroup.getDefaultInstance(), builder.getRepeatedGroup(0));
    builder.clearRepeatedGroup();

    message = builder.build();
    builder.addRepeatedGroup(0, repeatedGroupBuilder);
    messageAfterBuild = builder.build();
    assertEquals(0, message.getRepeatedGroupCount());
    builder.setRepeatedGroup(0, RepeatedGroup.getDefaultInstance());
    assertEquals(repeatedGroupBuilder.build(), messageAfterBuild.getRepeatedGroup(0));
    assertEquals(RepeatedGroup.getDefaultInstance(), builder.getRepeatedGroup(0));
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
    assertEquals(NestedMessage.getDefaultInstance(), builder.getRepeatedLazyMessage(0));
    builder.clearRepeatedLazyMessage();

    message = builder.build();
    builder.addRepeatedLazyMessage(0, nestedMessage);
    messageAfterBuild = builder.build();
    assertEquals(0, message.getRepeatedLazyMessageCount());
    builder.setRepeatedLazyMessage(0, NestedMessage.getDefaultInstance());
    assertEquals(nestedMessage, messageAfterBuild.getRepeatedLazyMessage(0));
    assertEquals(NestedMessage.getDefaultInstance(), builder.getRepeatedLazyMessage(0));
    builder.clearRepeatedLazyMessage();

    message = builder.build();
    builder.addRepeatedLazyMessage(nestedMessageBuilder);
    messageAfterBuild = builder.build();
    assertEquals(0, message.getRepeatedLazyMessageCount());
    builder.setRepeatedLazyMessage(0, NestedMessage.getDefaultInstance());
    assertEquals(nestedMessageBuilder.build(), messageAfterBuild.getRepeatedLazyMessage(0));
    assertEquals(NestedMessage.getDefaultInstance(), builder.getRepeatedLazyMessage(0));
    builder.clearRepeatedLazyMessage();

    message = builder.build();
    builder.addRepeatedLazyMessage(0, nestedMessageBuilder);
    messageAfterBuild = builder.build();
    assertEquals(0, message.getRepeatedLazyMessageCount());
    builder.setRepeatedLazyMessage(0, NestedMessage.getDefaultInstance());
    assertEquals(nestedMessageBuilder.build(), messageAfterBuild.getRepeatedLazyMessage(0));
    assertEquals(NestedMessage.getDefaultInstance(), builder.getRepeatedLazyMessage(0));
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
    assertEquals(ByteString.copyFromUtf8("hi"), messageAfterBuild.getRepeatedStringBytes(0));
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
    assertEquals(ByteString.copyFromUtf8("hi"), messageAfterBuild.getRepeatedStringPieceBytes(0));
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
    builder.mergeFrom(TestAllTypesLite.newBuilder().setOptionalBool(true).build());
    assertEquals(0, message.getSerializedSize());
    assertEquals(true, builder.build().getOptionalBool());
    builder.clearOptionalBool();

    message = builder.build();
    assertEquals(0, message.getSerializedSize());
    builder.mergeFrom(TestAllTypesLite.newBuilder().setOptionalBool(true).build());
    assertEquals(0, message.getSerializedSize());
    assertEquals(true, builder.build().getOptionalBool());
    builder.clear();
    assertEquals(0, builder.build().getSerializedSize());

    message = builder.build();
    assertEquals(0, message.getSerializedSize());
    builder.mergeOptionalForeignMessage(foreignMessage);
    assertEquals(0, message.getSerializedSize());
    assertEquals(foreignMessage.getC(), builder.build().getOptionalForeignMessage().getC());
    builder.clearOptionalForeignMessage();

    message = builder.build();
    assertEquals(0, message.getSerializedSize());
    builder.mergeOptionalLazyMessage(nestedMessage);
    assertEquals(0, message.getSerializedSize());
    assertEquals(nestedMessage.getBb(), builder.build().getOptionalLazyMessage().getBb());
    builder.clearOptionalLazyMessage();

    message = builder.build();
    builder.setOneofString("hi");
    assertEquals(OneofFieldCase.ONEOFFIELD_NOT_SET, message.getOneofFieldCase());
    assertEquals(OneofFieldCase.ONEOF_STRING, builder.getOneofFieldCase());
    assertEquals("hi", builder.getOneofString());
    messageAfterBuild = builder.build();
    assertEquals(OneofFieldCase.ONEOF_STRING, messageAfterBuild.getOneofFieldCase());
    assertEquals("hi", messageAfterBuild.getOneofString());
    builder.setOneofUint32(1);
    assertEquals(OneofFieldCase.ONEOF_STRING, messageAfterBuild.getOneofFieldCase());
    assertEquals("hi", messageAfterBuild.getOneofString());
    assertEquals(OneofFieldCase.ONEOF_UINT32, builder.getOneofFieldCase());
    assertEquals(1, builder.getOneofUint32());
    TestAllTypesLiteOrBuilder messageOrBuilder = builder;
    assertEquals(OneofFieldCase.ONEOF_UINT32, messageOrBuilder.getOneofFieldCase());

    TestAllExtensionsLite.Builder extendableMessageBuilder = TestAllExtensionsLite.newBuilder();
    TestAllExtensionsLite extendableMessage = extendableMessageBuilder.build();
    extendableMessageBuilder.setExtension(UnittestLite.optionalInt32ExtensionLite, 1);
    assertFalse(extendableMessage.hasExtension(UnittestLite.optionalInt32ExtensionLite));
    extendableMessage = extendableMessageBuilder.build();
    assertEquals(
        1, (int) extendableMessageBuilder.getExtension(UnittestLite.optionalInt32ExtensionLite));
    assertEquals(1, (int) extendableMessage.getExtension(UnittestLite.optionalInt32ExtensionLite));
    extendableMessageBuilder.setExtension(UnittestLite.optionalInt32ExtensionLite, 3);
    assertEquals(
        3, (int) extendableMessageBuilder.getExtension(UnittestLite.optionalInt32ExtensionLite));
    assertEquals(1, (int) extendableMessage.getExtension(UnittestLite.optionalInt32ExtensionLite));
    extendableMessage = extendableMessageBuilder.build();
    assertEquals(
        3, (int) extendableMessageBuilder.getExtension(UnittestLite.optionalInt32ExtensionLite));
    assertEquals(3, (int) extendableMessage.getExtension(UnittestLite.optionalInt32ExtensionLite));

    // No extension registry, so it should be in unknown fields.
    extendableMessage = TestAllExtensionsLite.parseFrom(extendableMessage.toByteArray());
    assertFalse(extendableMessage.hasExtension(UnittestLite.optionalInt32ExtensionLite));

    extendableMessageBuilder = extendableMessage.toBuilder();
    extendableMessageBuilder.mergeFrom(
        TestAllExtensionsLite.newBuilder()
            .setExtension(UnittestLite.optionalFixed32ExtensionLite, 11)
            .build());

    extendableMessage = extendableMessageBuilder.build();
    ExtensionRegistryLite registry = ExtensionRegistryLite.newInstance();
    UnittestLite.registerAllExtensions(registry);
    extendableMessage = TestAllExtensionsLite.parseFrom(extendableMessage.toByteArray(), registry);

    // The unknown field was preserved.
    assertEquals(3, (int) extendableMessage.getExtension(UnittestLite.optionalInt32ExtensionLite));
    assertEquals(
        11, (int) extendableMessage.getExtension(UnittestLite.optionalFixed32ExtensionLite));
  }

  public void testBuilderMergeFromNull() throws Exception {
    try {
      TestAllTypesLite.newBuilder().mergeFrom((TestAllTypesLite) null);
      fail("Expected exception");
    } catch (NullPointerException e) {
      // Pass.
    }
  }

  // Builder.mergeFrom() should keep existing extensions.
  public void testBuilderMergeFromWithExtensions() throws Exception {
    TestAllExtensionsLite message =
        TestAllExtensionsLite.newBuilder()
            .addExtension(UnittestLite.repeatedInt32ExtensionLite, 12)
            .build();

    ExtensionRegistryLite registry = ExtensionRegistryLite.newInstance();
    UnittestLite.registerAllExtensions(registry);

    TestAllExtensionsLite.Builder builder = TestAllExtensionsLite.newBuilder();
    builder.mergeFrom(message.toByteArray(), registry);
    builder.mergeFrom(message.toByteArray(), registry);
    TestAllExtensionsLite result = builder.build();
    assertEquals(2, result.getExtensionCount(UnittestLite.repeatedInt32ExtensionLite));
    assertEquals(12, result.getExtension(UnittestLite.repeatedInt32ExtensionLite, 0).intValue());
    assertEquals(12, result.getExtension(UnittestLite.repeatedInt32ExtensionLite, 1).intValue());
  }

  // Builder.mergeFrom() should keep existing unknown fields.
  public void testBuilderMergeFromWithUnknownFields() throws Exception {
    TestAllTypesLite message = TestAllTypesLite.newBuilder().addRepeatedInt32(1).build();

    NestedMessage.Builder builder = NestedMessage.newBuilder();
    builder.mergeFrom(message.toByteArray());
    builder.mergeFrom(message.toByteArray());
    NestedMessage result = builder.build();
    assertEquals(message.getSerializedSize() * 2, result.getSerializedSize());
  }

  public void testMergeFrom_differentFieldsSetWithinOneField() throws Exception {
    TestAllTypesLite result =
        TestAllTypesLite.newBuilder()
            .setOneofNestedMessage(NestedMessage.newBuilder().setBb(2))
            .mergeFrom(
                TestAllTypesLite.newBuilder()
                    .setOneofNestedMessage2(NestedMessage2.newBuilder().setDd(3))
                    .build())
            .build();

    assertToStringEquals("oneof_nested_message2 {\n  dd: 3\n}", result);
  }

  public void testMergeFrom_differentFieldsOfSameTypeSetWithinOneField() throws Exception {
    TestAllTypesLite result =
        TestAllTypesLite.newBuilder()
            .setOneofNestedMessage(NestedMessage.newBuilder().setBb(2))
            .mergeFrom(
                TestAllTypesLite.newBuilder()
                    .setOneofLazyNestedMessage(NestedMessage.newBuilder().setCc(3))
                    .build())
            .build();

    assertToStringEquals("oneof_lazy_nested_message {\n  cc: 3\n}", result);
  }

  public void testMergeFrom_sameFieldSetWithinOneofField() throws Exception {
    TestAllTypesLite result =
        TestAllTypesLite.newBuilder()
            .setOneofNestedMessage(NestedMessage.newBuilder().setBb(2))
            .mergeFrom(
                TestAllTypesLite.newBuilder()
                    .setOneofNestedMessage(NestedMessage.newBuilder().setCc(4))
                    .build())
            .build();

    assertToStringEquals("oneof_nested_message {\n  bb: 2\n  cc: 4\n}", result);
  }

  public void testToStringDefaultInstance() throws Exception {
    assertToStringEquals("", TestAllTypesLite.getDefaultInstance());
  }

  public void testToStringScalarFieldsSuffixedWithList() throws Exception {
    assertToStringEquals(
        "deceptively_named_list: 7",
        TestAllTypesLite.newBuilder().setDeceptivelyNamedList(7).build());
  }

  public void testToStringPrimitives() throws Exception {
    TestAllTypesLite proto =
        TestAllTypesLite.newBuilder()
            .setOptionalInt32(1)
            .setOptionalInt64(9223372036854775807L)
            .build();
    assertToStringEquals("optional_int32: 1\noptional_int64: 9223372036854775807", proto);

    proto =
        TestAllTypesLite.newBuilder()
            .setOptionalBool(true)
            .setOptionalNestedEnum(NestedEnum.BAZ)
            .build();
    assertToStringEquals(
        "optional_bool: true\noptional_nested_enum: " + NestedEnum.BAZ.toString(), proto);

    proto = TestAllTypesLite.newBuilder().setOptionalFloat(2.72f).setOptionalDouble(3.14).build();
    assertToStringEquals("optional_double: 3.14\noptional_float: 2.72", proto);
  }


  public void testToStringStringFields() throws Exception {
    TestAllTypesLite proto =
        TestAllTypesLite.newBuilder().setOptionalString("foo\"bar\nbaz\\").build();
    assertToStringEquals("optional_string: \"foo\\\"bar\\nbaz\\\\\"", proto);

    proto = TestAllTypesLite.newBuilder().setOptionalString("\u6587").build();
    assertToStringEquals("optional_string: \"\\346\\226\\207\"", proto);
  }

  public void testToStringNestedMessage() throws Exception {
    TestAllTypesLite proto =
        TestAllTypesLite.newBuilder()
            .setOptionalNestedMessage(NestedMessage.getDefaultInstance())
            .build();
    assertToStringEquals("optional_nested_message {\n}", proto);

    proto =
        TestAllTypesLite.newBuilder()
            .setOptionalNestedMessage(NestedMessage.newBuilder().setBb(7))
            .build();
    assertToStringEquals("optional_nested_message {\n  bb: 7\n}", proto);
  }

  public void testToStringRepeatedFields() throws Exception {
    TestAllTypesLite proto =
        TestAllTypesLite.newBuilder()
            .addRepeatedInt32(32)
            .addRepeatedInt32(32)
            .addRepeatedInt64(64)
            .build();
    assertToStringEquals("repeated_int32: 32\nrepeated_int32: 32\nrepeated_int64: 64", proto);

    proto =
        TestAllTypesLite.newBuilder()
            .addRepeatedLazyMessage(NestedMessage.newBuilder().setBb(7))
            .addRepeatedLazyMessage(NestedMessage.newBuilder().setBb(8))
            .build();
    assertToStringEquals(
        "repeated_lazy_message {\n  bb: 7\n}\nrepeated_lazy_message {\n  bb: 8\n}", proto);
  }

  public void testToStringForeignFields() throws Exception {
    TestAllTypesLite proto =
        TestAllTypesLite.newBuilder()
            .setOptionalForeignEnum(ForeignEnumLite.FOREIGN_LITE_BAR)
            .setOptionalForeignMessage(ForeignMessageLite.newBuilder().setC(3))
            .build();
    assertToStringEquals(
        "optional_foreign_enum: "
            + ForeignEnumLite.FOREIGN_LITE_BAR
            + "\noptional_foreign_message {\n  c: 3\n}",
        proto);
  }

  public void testToStringExtensions() throws Exception {
    TestAllExtensionsLite message =
        TestAllExtensionsLite.newBuilder()
            .setExtension(UnittestLite.optionalInt32ExtensionLite, 123)
            .addExtension(UnittestLite.repeatedStringExtensionLite, "spam")
            .addExtension(UnittestLite.repeatedStringExtensionLite, "eggs")
            .setExtension(UnittestLite.optionalNestedEnumExtensionLite, NestedEnum.BAZ)
            .setExtension(
                UnittestLite.optionalNestedMessageExtensionLite,
                NestedMessage.newBuilder().setBb(7).build())
            .build();
    assertToStringEquals(
        "[1]: 123\n[18] {\n  bb: 7\n}\n[21]: 3\n[44]: \"spam\"\n[44]: \"eggs\"", message);
  }

  public void testToStringUnknownFields() throws Exception {
    TestAllExtensionsLite messageWithExtensions =
        TestAllExtensionsLite.newBuilder()
            .setExtension(UnittestLite.optionalInt32ExtensionLite, 123)
            .addExtension(UnittestLite.repeatedStringExtensionLite, "spam")
            .addExtension(UnittestLite.repeatedStringExtensionLite, "eggs")
            .setExtension(UnittestLite.optionalNestedEnumExtensionLite, NestedEnum.BAZ)
            .setExtension(
                UnittestLite.optionalNestedMessageExtensionLite,
                NestedMessage.newBuilder().setBb(7).build())
            .build();
    TestAllExtensionsLite messageWithUnknownFields =
        TestAllExtensionsLite.parseFrom(messageWithExtensions.toByteArray());
    assertToStringEquals(
        "1: 123\n18: \"\\b\\a\"\n21: 3\n44: \"spam\"\n44: \"eggs\"", messageWithUnknownFields);
  }

  public void testToStringLazyMessage() throws Exception {
    TestAllTypesLite message =
        TestAllTypesLite.newBuilder()
            .setOptionalLazyMessage(NestedMessage.newBuilder().setBb(1).build())
            .build();
    assertToStringEquals("optional_lazy_message {\n  bb: 1\n}", message);
  }

  public void testToStringGroup() throws Exception {
    TestAllTypesLite message =
        TestAllTypesLite.newBuilder()
            .setOptionalGroup(OptionalGroup.newBuilder().setA(1).build())
            .build();
    assertToStringEquals("optional_group {\n  a: 1\n}", message);
  }

  public void testToStringOneof() throws Exception {
    TestAllTypesLite message = TestAllTypesLite.newBuilder().setOneofString("hello").build();
    assertToStringEquals("oneof_string: \"hello\"", message);
  }

  public void testToStringMapFields() throws Exception {
    TestMap message1 =
        TestMap.newBuilder()
            .putInt32ToStringField(1, "alpha")
            .putInt32ToStringField(2, "beta")
            .build();
    assertToStringEquals(
        "int32_to_string_field {\n"
            + "  key: 1\n"
            + "  value: \"alpha\"\n"
            + "}\n"
            + "int32_to_string_field {\n"
            + "  key: 2\n"
            + "  value: \"beta\"\n"
            + "}",
        message1);

    TestMap message2 =
        TestMap.newBuilder()
            .putInt32ToMessageField(1, MessageValue.newBuilder().setValue(10).build())
            .putInt32ToMessageField(2, MessageValue.newBuilder().setValue(20).build())
            .build();
    assertToStringEquals(
        "int32_to_message_field {\n"
            + "  key: 1\n"
            + "  value {\n"
            + "    value: 10\n"
            + "  }\n"
            + "}\n"
            + "int32_to_message_field {\n"
            + "  key: 2\n"
            + "  value {\n"
            + "    value: 20\n"
            + "  }\n"
            + "}",
        message2);
  }

  // Asserts that the toString() representation of the message matches the expected. This verifies
  // the first line starts with a comment; but, does not factor in said comment as part of the
  // comparison as it contains unstable addresses.
  private static void assertToStringEquals(String expected, MessageLite message) {
    String toString = message.toString();
    assertEquals('#', toString.charAt(0));
    if (toString.contains("\n")) {
      toString = toString.substring(toString.indexOf("\n") + 1);
    } else {
      toString = "";
    }
    assertEquals(expected, toString);
  }

  public void testParseLazy() throws Exception {
    ByteString bb =
        TestAllTypesLite.newBuilder()
            .setOptionalLazyMessage(NestedMessage.newBuilder().setBb(11).build())
            .build()
            .toByteString();
    ByteString cc =
        TestAllTypesLite.newBuilder()
            .setOptionalLazyMessage(NestedMessage.newBuilder().setCc(22).build())
            .build()
            .toByteString();

    ByteString concat = bb.concat(cc);
    TestAllTypesLite message = TestAllTypesLite.parseFrom(concat);

    assertEquals(11, message.getOptionalLazyMessage().getBb());
    assertEquals(22L, message.getOptionalLazyMessage().getCc());
  }

  public void testParseLazy_oneOf() throws Exception {
    ByteString bb =
        TestAllTypesLite.newBuilder()
            .setOneofLazyNestedMessage(NestedMessage.newBuilder().setBb(11).build())
            .build()
            .toByteString();
    ByteString cc =
        TestAllTypesLite.newBuilder()
            .setOneofLazyNestedMessage(NestedMessage.newBuilder().setCc(22).build())
            .build()
            .toByteString();

    ByteString concat = bb.concat(cc);
    TestAllTypesLite message = TestAllTypesLite.parseFrom(concat);

    assertEquals(11, message.getOneofLazyNestedMessage().getBb());
    assertEquals(22L, message.getOneofLazyNestedMessage().getCc());
  }

  public void testMergeFromStream_repeatedField() throws Exception {
    TestAllTypesLite.Builder builder = TestAllTypesLite.newBuilder().addRepeatedString("hello");
    builder.mergeFrom(CodedInputStream.newInstance(builder.build().toByteArray()));

    assertEquals(2, builder.getRepeatedStringCount());
  }

  public void testMergeFromStream_invalidBytes() throws Exception {
    TestAllTypesLite.Builder builder = TestAllTypesLite.newBuilder().setDefaultBool(true);
    try {
      builder.mergeFrom(CodedInputStream.newInstance("Invalid bytes".getBytes(Internal.UTF_8)));
      fail();
    } catch (InvalidProtocolBufferException expected) {
    }
  }

  public void testMergeFrom_sanity() throws Exception {
    TestAllTypesLite one = TestUtilLite.getAllLiteSetBuilder().build();
    byte[] bytes = one.toByteArray();
    TestAllTypesLite two = TestAllTypesLite.parseFrom(bytes);

    one = one.toBuilder().mergeFrom(one).build();
    two = two.toBuilder().mergeFrom(bytes).build();
    assertEquals(one, two);
    assertEquals(two, one);
    assertEquals(one.hashCode(), two.hashCode());
  }

  public void testMergeFromNoLazyFieldSharing() throws Exception {
    TestAllTypesLite.Builder sourceBuilder =
        TestAllTypesLite.newBuilder().setOptionalLazyMessage(NestedMessage.newBuilder().setBb(1));
    TestAllTypesLite.Builder targetBuilder =
        TestAllTypesLite.newBuilder().mergeFrom(sourceBuilder.build());
    assertEquals(1, sourceBuilder.getOptionalLazyMessage().getBb());
    // now change the sourceBuilder, and target value shouldn't be affected.
    sourceBuilder.setOptionalLazyMessage(NestedMessage.newBuilder().setBb(2));
    assertEquals(1, targetBuilder.getOptionalLazyMessage().getBb());
  }

  public void testEquals_notEqual() throws Exception {
    TestAllTypesLite one = TestUtilLite.getAllLiteSetBuilder().build();
    byte[] bytes = one.toByteArray();
    TestAllTypesLite two = one.toBuilder().mergeFrom(one).mergeFrom(bytes).build();

    assertFalse(one.equals(two));
    assertFalse(two.equals(one));

    assertFalse(one.equals(TestAllTypesLite.getDefaultInstance()));
    assertFalse(TestAllTypesLite.getDefaultInstance().equals(one));

    TestAllTypesLite oneFieldSet = TestAllTypesLite.newBuilder().setDefaultBool(true).build();
    assertFalse(oneFieldSet.equals(TestAllTypesLite.getDefaultInstance()));
    assertFalse(TestAllTypesLite.getDefaultInstance().equals(oneFieldSet));

    oneFieldSet = TestAllTypesLite.newBuilder().setDefaultBytes(ByteString.EMPTY).build();
    assertFalse(oneFieldSet.equals(TestAllTypesLite.getDefaultInstance()));
    assertFalse(TestAllTypesLite.getDefaultInstance().equals(oneFieldSet));

    oneFieldSet = TestAllTypesLite.newBuilder().setDefaultCord("").build();
    assertFalse(oneFieldSet.equals(TestAllTypesLite.getDefaultInstance()));
    assertFalse(TestAllTypesLite.getDefaultInstance().equals(oneFieldSet));

    oneFieldSet = TestAllTypesLite.newBuilder().setDefaultCordBytes(ByteString.EMPTY).build();
    assertFalse(oneFieldSet.equals(TestAllTypesLite.getDefaultInstance()));
    assertFalse(TestAllTypesLite.getDefaultInstance().equals(oneFieldSet));

    oneFieldSet = TestAllTypesLite.newBuilder().setDefaultDouble(0).build();
    assertFalse(oneFieldSet.equals(TestAllTypesLite.getDefaultInstance()));
    assertFalse(TestAllTypesLite.getDefaultInstance().equals(oneFieldSet));

    oneFieldSet = TestAllTypesLite.newBuilder().setDefaultFixed32(0).build();
    assertFalse(oneFieldSet.equals(TestAllTypesLite.getDefaultInstance()));
    assertFalse(TestAllTypesLite.getDefaultInstance().equals(oneFieldSet));

    oneFieldSet = TestAllTypesLite.newBuilder().setDefaultFixed64(0).build();
    assertFalse(oneFieldSet.equals(TestAllTypesLite.getDefaultInstance()));
    assertFalse(TestAllTypesLite.getDefaultInstance().equals(oneFieldSet));

    oneFieldSet = TestAllTypesLite.newBuilder().setDefaultFloat(0).build();
    assertFalse(oneFieldSet.equals(TestAllTypesLite.getDefaultInstance()));
    assertFalse(TestAllTypesLite.getDefaultInstance().equals(oneFieldSet));

    oneFieldSet =
        TestAllTypesLite.newBuilder()
            .setDefaultForeignEnum(ForeignEnumLite.FOREIGN_LITE_BAR)
            .build();
    assertFalse(oneFieldSet.equals(TestAllTypesLite.getDefaultInstance()));
    assertFalse(TestAllTypesLite.getDefaultInstance().equals(oneFieldSet));

    oneFieldSet =
        TestAllTypesLite.newBuilder().setDefaultImportEnum(ImportEnumLite.IMPORT_LITE_BAR).build();
    assertFalse(oneFieldSet.equals(TestAllTypesLite.getDefaultInstance()));
    assertFalse(TestAllTypesLite.getDefaultInstance().equals(oneFieldSet));

    oneFieldSet = TestAllTypesLite.newBuilder().setDefaultInt32(0).build();
    assertFalse(oneFieldSet.equals(TestAllTypesLite.getDefaultInstance()));
    assertFalse(TestAllTypesLite.getDefaultInstance().equals(oneFieldSet));

    oneFieldSet = TestAllTypesLite.newBuilder().setDefaultInt64(0).build();
    assertFalse(oneFieldSet.equals(TestAllTypesLite.getDefaultInstance()));
    assertFalse(TestAllTypesLite.getDefaultInstance().equals(oneFieldSet));

    oneFieldSet = TestAllTypesLite.newBuilder().setDefaultNestedEnum(NestedEnum.BAR).build();
    assertFalse(oneFieldSet.equals(TestAllTypesLite.getDefaultInstance()));
    assertFalse(TestAllTypesLite.getDefaultInstance().equals(oneFieldSet));

    oneFieldSet = TestAllTypesLite.newBuilder().setDefaultSfixed32(0).build();
    assertFalse(oneFieldSet.equals(TestAllTypesLite.getDefaultInstance()));
    assertFalse(TestAllTypesLite.getDefaultInstance().equals(oneFieldSet));

    oneFieldSet = TestAllTypesLite.newBuilder().setDefaultSfixed64(0).build();
    assertFalse(oneFieldSet.equals(TestAllTypesLite.getDefaultInstance()));
    assertFalse(TestAllTypesLite.getDefaultInstance().equals(oneFieldSet));

    oneFieldSet = TestAllTypesLite.newBuilder().setDefaultSint32(0).build();
    assertFalse(oneFieldSet.equals(TestAllTypesLite.getDefaultInstance()));
    assertFalse(TestAllTypesLite.getDefaultInstance().equals(oneFieldSet));

    oneFieldSet = TestAllTypesLite.newBuilder().setDefaultSint64(0).build();
    assertFalse(oneFieldSet.equals(TestAllTypesLite.getDefaultInstance()));
    assertFalse(TestAllTypesLite.getDefaultInstance().equals(oneFieldSet));

    oneFieldSet = TestAllTypesLite.newBuilder().setDefaultString("").build();
    assertFalse(oneFieldSet.equals(TestAllTypesLite.getDefaultInstance()));
    assertFalse(TestAllTypesLite.getDefaultInstance().equals(oneFieldSet));

    oneFieldSet = TestAllTypesLite.newBuilder().setDefaultStringBytes(ByteString.EMPTY).build();
    assertFalse(oneFieldSet.equals(TestAllTypesLite.getDefaultInstance()));
    assertFalse(TestAllTypesLite.getDefaultInstance().equals(oneFieldSet));

    oneFieldSet = TestAllTypesLite.newBuilder().setDefaultStringPiece("").build();
    assertFalse(oneFieldSet.equals(TestAllTypesLite.getDefaultInstance()));
    assertFalse(TestAllTypesLite.getDefaultInstance().equals(oneFieldSet));

    oneFieldSet =
        TestAllTypesLite.newBuilder().setDefaultStringPieceBytes(ByteString.EMPTY).build();
    assertFalse(oneFieldSet.equals(TestAllTypesLite.getDefaultInstance()));
    assertFalse(TestAllTypesLite.getDefaultInstance().equals(oneFieldSet));

    oneFieldSet = TestAllTypesLite.newBuilder().setDefaultUint32(0).build();
    assertFalse(oneFieldSet.equals(TestAllTypesLite.getDefaultInstance()));
    assertFalse(TestAllTypesLite.getDefaultInstance().equals(oneFieldSet));

    oneFieldSet = TestAllTypesLite.newBuilder().setDefaultUint64(0).build();
    assertFalse(oneFieldSet.equals(TestAllTypesLite.getDefaultInstance()));
    assertFalse(TestAllTypesLite.getDefaultInstance().equals(oneFieldSet));

    oneFieldSet = TestAllTypesLite.newBuilder().addRepeatedBool(true).build();
    assertFalse(oneFieldSet.equals(TestAllTypesLite.getDefaultInstance()));
    assertFalse(TestAllTypesLite.getDefaultInstance().equals(oneFieldSet));

    oneFieldSet = TestAllTypesLite.newBuilder().addRepeatedBytes(ByteString.EMPTY).build();
    assertFalse(oneFieldSet.equals(TestAllTypesLite.getDefaultInstance()));
    assertFalse(TestAllTypesLite.getDefaultInstance().equals(oneFieldSet));

    oneFieldSet = TestAllTypesLite.newBuilder().addRepeatedCord("").build();
    assertFalse(oneFieldSet.equals(TestAllTypesLite.getDefaultInstance()));
    assertFalse(TestAllTypesLite.getDefaultInstance().equals(oneFieldSet));

    oneFieldSet = TestAllTypesLite.newBuilder().addRepeatedCordBytes(ByteString.EMPTY).build();
    assertFalse(oneFieldSet.equals(TestAllTypesLite.getDefaultInstance()));
    assertFalse(TestAllTypesLite.getDefaultInstance().equals(oneFieldSet));

    oneFieldSet = TestAllTypesLite.newBuilder().addRepeatedDouble(0).build();
    assertFalse(oneFieldSet.equals(TestAllTypesLite.getDefaultInstance()));
    assertFalse(TestAllTypesLite.getDefaultInstance().equals(oneFieldSet));

    oneFieldSet = TestAllTypesLite.newBuilder().addRepeatedFixed32(0).build();
    assertFalse(oneFieldSet.equals(TestAllTypesLite.getDefaultInstance()));
    assertFalse(TestAllTypesLite.getDefaultInstance().equals(oneFieldSet));

    oneFieldSet = TestAllTypesLite.newBuilder().addRepeatedFixed64(0).build();
    assertFalse(oneFieldSet.equals(TestAllTypesLite.getDefaultInstance()));
    assertFalse(TestAllTypesLite.getDefaultInstance().equals(oneFieldSet));

    oneFieldSet = TestAllTypesLite.newBuilder().addRepeatedFloat(0).build();
    assertFalse(oneFieldSet.equals(TestAllTypesLite.getDefaultInstance()));
    assertFalse(TestAllTypesLite.getDefaultInstance().equals(oneFieldSet));

    oneFieldSet =
        TestAllTypesLite.newBuilder()
            .addRepeatedForeignEnum(ForeignEnumLite.FOREIGN_LITE_BAR)
            .build();
    assertFalse(oneFieldSet.equals(TestAllTypesLite.getDefaultInstance()));
    assertFalse(TestAllTypesLite.getDefaultInstance().equals(oneFieldSet));

    oneFieldSet =
        TestAllTypesLite.newBuilder().addRepeatedImportEnum(ImportEnumLite.IMPORT_LITE_BAR).build();
    assertFalse(oneFieldSet.equals(TestAllTypesLite.getDefaultInstance()));
    assertFalse(TestAllTypesLite.getDefaultInstance().equals(oneFieldSet));

    oneFieldSet = TestAllTypesLite.newBuilder().addRepeatedInt32(0).build();
    assertFalse(oneFieldSet.equals(TestAllTypesLite.getDefaultInstance()));
    assertFalse(TestAllTypesLite.getDefaultInstance().equals(oneFieldSet));

    oneFieldSet = TestAllTypesLite.newBuilder().addRepeatedInt64(0).build();
    assertFalse(oneFieldSet.equals(TestAllTypesLite.getDefaultInstance()));
    assertFalse(TestAllTypesLite.getDefaultInstance().equals(oneFieldSet));

    oneFieldSet = TestAllTypesLite.newBuilder().addRepeatedNestedEnum(NestedEnum.BAR).build();
    assertFalse(oneFieldSet.equals(TestAllTypesLite.getDefaultInstance()));
    assertFalse(TestAllTypesLite.getDefaultInstance().equals(oneFieldSet));

    oneFieldSet = TestAllTypesLite.newBuilder().addRepeatedSfixed32(0).build();
    assertFalse(oneFieldSet.equals(TestAllTypesLite.getDefaultInstance()));
    assertFalse(TestAllTypesLite.getDefaultInstance().equals(oneFieldSet));

    oneFieldSet = TestAllTypesLite.newBuilder().addRepeatedSfixed64(0).build();
    assertFalse(oneFieldSet.equals(TestAllTypesLite.getDefaultInstance()));
    assertFalse(TestAllTypesLite.getDefaultInstance().equals(oneFieldSet));

    oneFieldSet = TestAllTypesLite.newBuilder().addRepeatedSint32(0).build();
    assertFalse(oneFieldSet.equals(TestAllTypesLite.getDefaultInstance()));
    assertFalse(TestAllTypesLite.getDefaultInstance().equals(oneFieldSet));

    oneFieldSet = TestAllTypesLite.newBuilder().addRepeatedSint64(0).build();
    assertFalse(oneFieldSet.equals(TestAllTypesLite.getDefaultInstance()));
    assertFalse(TestAllTypesLite.getDefaultInstance().equals(oneFieldSet));

    oneFieldSet = TestAllTypesLite.newBuilder().addRepeatedString("").build();
    assertFalse(oneFieldSet.equals(TestAllTypesLite.getDefaultInstance()));
    assertFalse(TestAllTypesLite.getDefaultInstance().equals(oneFieldSet));

    oneFieldSet = TestAllTypesLite.newBuilder().addRepeatedStringBytes(ByteString.EMPTY).build();
    assertFalse(oneFieldSet.equals(TestAllTypesLite.getDefaultInstance()));
    assertFalse(TestAllTypesLite.getDefaultInstance().equals(oneFieldSet));

    oneFieldSet = TestAllTypesLite.newBuilder().addRepeatedStringPiece("").build();
    assertFalse(oneFieldSet.equals(TestAllTypesLite.getDefaultInstance()));
    assertFalse(TestAllTypesLite.getDefaultInstance().equals(oneFieldSet));

    oneFieldSet =
        TestAllTypesLite.newBuilder().addRepeatedStringPieceBytes(ByteString.EMPTY).build();
    assertFalse(oneFieldSet.equals(TestAllTypesLite.getDefaultInstance()));
    assertFalse(TestAllTypesLite.getDefaultInstance().equals(oneFieldSet));

    oneFieldSet = TestAllTypesLite.newBuilder().addRepeatedUint32(0).build();
    assertFalse(oneFieldSet.equals(TestAllTypesLite.getDefaultInstance()));
    assertFalse(TestAllTypesLite.getDefaultInstance().equals(oneFieldSet));

    oneFieldSet = TestAllTypesLite.newBuilder().addRepeatedUint64(0).build();
    assertFalse(oneFieldSet.equals(TestAllTypesLite.getDefaultInstance()));
    assertFalse(TestAllTypesLite.getDefaultInstance().equals(oneFieldSet));

    oneFieldSet = TestAllTypesLite.newBuilder().setOptionalBool(true).build();
    assertFalse(oneFieldSet.equals(TestAllTypesLite.getDefaultInstance()));
    assertFalse(TestAllTypesLite.getDefaultInstance().equals(oneFieldSet));

    oneFieldSet = TestAllTypesLite.newBuilder().setOptionalBytes(ByteString.EMPTY).build();
    assertFalse(oneFieldSet.equals(TestAllTypesLite.getDefaultInstance()));
    assertFalse(TestAllTypesLite.getDefaultInstance().equals(oneFieldSet));

    oneFieldSet = TestAllTypesLite.newBuilder().setOptionalCord("").build();
    assertFalse(oneFieldSet.equals(TestAllTypesLite.getDefaultInstance()));
    assertFalse(TestAllTypesLite.getDefaultInstance().equals(oneFieldSet));

    oneFieldSet = TestAllTypesLite.newBuilder().setOptionalCordBytes(ByteString.EMPTY).build();
    assertFalse(oneFieldSet.equals(TestAllTypesLite.getDefaultInstance()));
    assertFalse(TestAllTypesLite.getDefaultInstance().equals(oneFieldSet));

    oneFieldSet = TestAllTypesLite.newBuilder().setOptionalDouble(0).build();
    assertFalse(oneFieldSet.equals(TestAllTypesLite.getDefaultInstance()));
    assertFalse(TestAllTypesLite.getDefaultInstance().equals(oneFieldSet));

    oneFieldSet = TestAllTypesLite.newBuilder().setOptionalFixed32(0).build();
    assertFalse(oneFieldSet.equals(TestAllTypesLite.getDefaultInstance()));
    assertFalse(TestAllTypesLite.getDefaultInstance().equals(oneFieldSet));

    oneFieldSet = TestAllTypesLite.newBuilder().setOptionalFixed64(0).build();
    assertFalse(oneFieldSet.equals(TestAllTypesLite.getDefaultInstance()));
    assertFalse(TestAllTypesLite.getDefaultInstance().equals(oneFieldSet));

    oneFieldSet = TestAllTypesLite.newBuilder().setOptionalFloat(0).build();
    assertFalse(oneFieldSet.equals(TestAllTypesLite.getDefaultInstance()));
    assertFalse(TestAllTypesLite.getDefaultInstance().equals(oneFieldSet));

    oneFieldSet =
        TestAllTypesLite.newBuilder()
            .setOptionalForeignEnum(ForeignEnumLite.FOREIGN_LITE_BAR)
            .build();
    assertFalse(oneFieldSet.equals(TestAllTypesLite.getDefaultInstance()));
    assertFalse(TestAllTypesLite.getDefaultInstance().equals(oneFieldSet));

    oneFieldSet =
        TestAllTypesLite.newBuilder().setOptionalImportEnum(ImportEnumLite.IMPORT_LITE_BAR).build();
    assertFalse(oneFieldSet.equals(TestAllTypesLite.getDefaultInstance()));
    assertFalse(TestAllTypesLite.getDefaultInstance().equals(oneFieldSet));

    oneFieldSet = TestAllTypesLite.newBuilder().setOptionalInt32(0).build();
    assertFalse(oneFieldSet.equals(TestAllTypesLite.getDefaultInstance()));
    assertFalse(TestAllTypesLite.getDefaultInstance().equals(oneFieldSet));

    oneFieldSet = TestAllTypesLite.newBuilder().setOptionalInt64(0).build();
    assertFalse(oneFieldSet.equals(TestAllTypesLite.getDefaultInstance()));
    assertFalse(TestAllTypesLite.getDefaultInstance().equals(oneFieldSet));

    oneFieldSet = TestAllTypesLite.newBuilder().setOptionalNestedEnum(NestedEnum.BAR).build();
    assertFalse(oneFieldSet.equals(TestAllTypesLite.getDefaultInstance()));
    assertFalse(TestAllTypesLite.getDefaultInstance().equals(oneFieldSet));

    oneFieldSet = TestAllTypesLite.newBuilder().setOptionalSfixed32(0).build();
    assertFalse(oneFieldSet.equals(TestAllTypesLite.getDefaultInstance()));
    assertFalse(TestAllTypesLite.getDefaultInstance().equals(oneFieldSet));

    oneFieldSet = TestAllTypesLite.newBuilder().setOptionalSfixed64(0).build();
    assertFalse(oneFieldSet.equals(TestAllTypesLite.getDefaultInstance()));
    assertFalse(TestAllTypesLite.getDefaultInstance().equals(oneFieldSet));

    oneFieldSet = TestAllTypesLite.newBuilder().setOptionalSint32(0).build();
    assertFalse(oneFieldSet.equals(TestAllTypesLite.getDefaultInstance()));
    assertFalse(TestAllTypesLite.getDefaultInstance().equals(oneFieldSet));

    oneFieldSet = TestAllTypesLite.newBuilder().setOptionalSint64(0).build();
    assertFalse(oneFieldSet.equals(TestAllTypesLite.getDefaultInstance()));
    assertFalse(TestAllTypesLite.getDefaultInstance().equals(oneFieldSet));

    oneFieldSet = TestAllTypesLite.newBuilder().setOptionalString("").build();
    assertFalse(oneFieldSet.equals(TestAllTypesLite.getDefaultInstance()));
    assertFalse(TestAllTypesLite.getDefaultInstance().equals(oneFieldSet));

    oneFieldSet = TestAllTypesLite.newBuilder().setOptionalStringBytes(ByteString.EMPTY).build();
    assertFalse(oneFieldSet.equals(TestAllTypesLite.getDefaultInstance()));
    assertFalse(TestAllTypesLite.getDefaultInstance().equals(oneFieldSet));

    oneFieldSet = TestAllTypesLite.newBuilder().setOptionalStringPiece("").build();
    assertFalse(oneFieldSet.equals(TestAllTypesLite.getDefaultInstance()));
    assertFalse(TestAllTypesLite.getDefaultInstance().equals(oneFieldSet));

    oneFieldSet =
        TestAllTypesLite.newBuilder().setOptionalStringPieceBytes(ByteString.EMPTY).build();
    assertFalse(oneFieldSet.equals(TestAllTypesLite.getDefaultInstance()));
    assertFalse(TestAllTypesLite.getDefaultInstance().equals(oneFieldSet));

    oneFieldSet = TestAllTypesLite.newBuilder().setOptionalUint32(0).build();
    assertFalse(oneFieldSet.equals(TestAllTypesLite.getDefaultInstance()));
    assertFalse(TestAllTypesLite.getDefaultInstance().equals(oneFieldSet));

    oneFieldSet = TestAllTypesLite.newBuilder().setOptionalUint64(0).build();
    assertFalse(oneFieldSet.equals(TestAllTypesLite.getDefaultInstance()));
    assertFalse(TestAllTypesLite.getDefaultInstance().equals(oneFieldSet));

    oneFieldSet = TestAllTypesLite.newBuilder().setOneofBytes(ByteString.EMPTY).build();
    assertFalse(oneFieldSet.equals(TestAllTypesLite.getDefaultInstance()));
    assertFalse(TestAllTypesLite.getDefaultInstance().equals(oneFieldSet));

    oneFieldSet =
        TestAllTypesLite.newBuilder()
            .setOneofLazyNestedMessage(NestedMessage.getDefaultInstance())
            .build();
    assertFalse(oneFieldSet.equals(TestAllTypesLite.getDefaultInstance()));
    assertFalse(TestAllTypesLite.getDefaultInstance().equals(oneFieldSet));

    oneFieldSet =
        TestAllTypesLite.newBuilder()
            .setOneofNestedMessage(NestedMessage.getDefaultInstance())
            .build();
    assertFalse(oneFieldSet.equals(TestAllTypesLite.getDefaultInstance()));
    assertFalse(TestAllTypesLite.getDefaultInstance().equals(oneFieldSet));

    oneFieldSet = TestAllTypesLite.newBuilder().setOneofString("").build();
    assertFalse(oneFieldSet.equals(TestAllTypesLite.getDefaultInstance()));
    assertFalse(TestAllTypesLite.getDefaultInstance().equals(oneFieldSet));

    oneFieldSet = TestAllTypesLite.newBuilder().setOneofStringBytes(ByteString.EMPTY).build();
    assertFalse(oneFieldSet.equals(TestAllTypesLite.getDefaultInstance()));
    assertFalse(TestAllTypesLite.getDefaultInstance().equals(oneFieldSet));

    oneFieldSet = TestAllTypesLite.newBuilder().setOneofUint32(0).build();
    assertFalse(oneFieldSet.equals(TestAllTypesLite.getDefaultInstance()));
    assertFalse(TestAllTypesLite.getDefaultInstance().equals(oneFieldSet));

    oneFieldSet =
        TestAllTypesLite.newBuilder()
            .setOptionalForeignMessage(ForeignMessageLite.getDefaultInstance())
            .build();
    assertFalse(oneFieldSet.equals(TestAllTypesLite.getDefaultInstance()));
    assertFalse(TestAllTypesLite.getDefaultInstance().equals(oneFieldSet));

    oneFieldSet =
        TestAllTypesLite.newBuilder().setOptionalGroup(OptionalGroup.getDefaultInstance()).build();
    assertFalse(oneFieldSet.equals(TestAllTypesLite.getDefaultInstance()));
    assertFalse(TestAllTypesLite.getDefaultInstance().equals(oneFieldSet));

    oneFieldSet =
        TestAllTypesLite.newBuilder()
            .setOptionalPublicImportMessage(PublicImportMessageLite.getDefaultInstance())
            .build();
    assertFalse(oneFieldSet.equals(TestAllTypesLite.getDefaultInstance()));
    assertFalse(TestAllTypesLite.getDefaultInstance().equals(oneFieldSet));

    oneFieldSet =
        TestAllTypesLite.newBuilder()
            .setOptionalLazyMessage(NestedMessage.getDefaultInstance())
            .build();
    assertFalse(oneFieldSet.equals(TestAllTypesLite.getDefaultInstance()));

    assertFalse(TestAllTypesLite.getDefaultInstance().equals(oneFieldSet));
    oneFieldSet =
        TestAllTypesLite.newBuilder()
            .addRepeatedLazyMessage(NestedMessage.getDefaultInstance())
            .build();
    assertFalse(oneFieldSet.equals(TestAllTypesLite.getDefaultInstance()));
    assertFalse(TestAllTypesLite.getDefaultInstance().equals(oneFieldSet));
  }

  public void testEquals() throws Exception {
    // Check that two identical objs are equal.
    Foo foo1a = Foo.newBuilder().setValue(1).addBar(Bar.newBuilder().setName("foo1")).build();
    Foo foo1b = Foo.newBuilder().setValue(1).addBar(Bar.newBuilder().setName("foo1")).build();
    Foo foo2 = Foo.newBuilder().setValue(1).addBar(Bar.newBuilder().setName("foo2")).build();

    // Check that equals is doing value rather than object equality.
    assertEquals(foo1a, foo1b);
    assertEquals(foo1a.hashCode(), foo1b.hashCode());

    // Check that a different object is not equal.
    assertFalse(foo1a.equals(foo2));

    // Check that two objects which have different types but the same field values are not
    // considered to be equal.
    Bar bar = Bar.newBuilder().setName("bar").build();
    BarPrime barPrime = BarPrime.newBuilder().setName("bar").build();
    assertFalse(bar.equals(barPrime));
  }

  public void testEqualsAndHashCodeForTrickySchemaTypes() {
    Foo foo1 = Foo.getDefaultInstance();
    Foo foo2 = Foo.newBuilder().setSint64(1).build();
    Foo foo3 = Foo.newBuilder().putMyMap("key", "value2").build();
    Foo foo4 = Foo.newBuilder().setMyGroup(Foo.MyGroup.newBuilder().setValue(4).build()).build();

    assertEqualsAndHashCodeAreFalse(foo1, foo2);
    assertEqualsAndHashCodeAreFalse(foo1, foo3);
    assertEqualsAndHashCodeAreFalse(foo1, foo4);
  }

  public void testOneofEquals() throws Exception {
    TestOneofEquals.Builder builder = TestOneofEquals.newBuilder();
    TestOneofEquals message1 = builder.build();
    // Set message2's name field to default value. The two messages should be different when we
    // check with the oneof case.
    builder.setName("");
    TestOneofEquals message2 = builder.build();
    assertFalse(message1.equals(message2));
  }

  public void testEquals_sanity() throws Exception {
    TestAllTypesLite one = TestUtilLite.getAllLiteSetBuilder().build();
    TestAllTypesLite two = TestAllTypesLite.parseFrom(one.toByteArray());
    assertEquals(one, two);
    assertEquals(one.hashCode(), two.hashCode());

    assertEquals(
        one.toBuilder().mergeFrom(two).build(),
        two.toBuilder().mergeFrom(two.toByteArray()).build());
  }

  public void testEqualsAndHashCodeWithUnknownFields() throws InvalidProtocolBufferException {
    Foo fooWithOnlyValue = Foo.newBuilder().setValue(1).build();

    Foo fooWithValueAndExtension =
        fooWithOnlyValue
            .toBuilder()
            .setValue(1)
            .setExtension(Bar.fooExt, Bar.newBuilder().setName("name").build())
            .build();

    Foo fooWithValueAndUnknownFields = Foo.parseFrom(fooWithValueAndExtension.toByteArray());

    assertEqualsAndHashCodeAreFalse(fooWithOnlyValue, fooWithValueAndUnknownFields);
    assertEqualsAndHashCodeAreFalse(fooWithValueAndExtension, fooWithValueAndUnknownFields);
  }

  public void testEqualsAndHashCodeWithExtensions() throws InvalidProtocolBufferException {
    Foo fooWithOnlyValue = Foo.newBuilder().setValue(1).build();

    Foo fooWithValueAndExtension =
        fooWithOnlyValue
            .toBuilder()
            .setValue(1)
            .setExtension(Bar.fooExt, Bar.newBuilder().setName("name").build())
            .build();

    assertEqualsAndHashCodeAreFalse(fooWithOnlyValue, fooWithValueAndExtension);
  }

  // Test to ensure we avoid a class cast exception with oneofs.
  public void testEquals_oneOfMessages() {
    TestAllTypesLite mine = TestAllTypesLite.newBuilder().setOneofString("Hello").build();

    TestAllTypesLite other =
        TestAllTypesLite.newBuilder()
            .setOneofNestedMessage(NestedMessage.getDefaultInstance())
            .build();

    assertFalse(mine.equals(other));
    assertFalse(other.equals(mine));
  }

  public void testHugeFieldNumbers() throws InvalidProtocolBufferException {
    TestHugeFieldNumbersLite message =
        TestHugeFieldNumbersLite.newBuilder()
            .setOptionalInt32(1)
            .addRepeatedInt32(2)
            .setOptionalEnum(ForeignEnumLite.FOREIGN_LITE_FOO)
            .setOptionalString("xyz")
            .setOptionalMessage(ForeignMessageLite.newBuilder().setC(3).build())
            .build();

    TestHugeFieldNumbersLite parsedMessage =
        TestHugeFieldNumbersLite.parseFrom(message.toByteArray());
    assertEquals(1, parsedMessage.getOptionalInt32());
    assertEquals(2, parsedMessage.getRepeatedInt32(0));
    assertEquals(ForeignEnumLite.FOREIGN_LITE_FOO, parsedMessage.getOptionalEnum());
    assertEquals("xyz", parsedMessage.getOptionalString());
    assertEquals(3, parsedMessage.getOptionalMessage().getC());
  }

  private void assertEqualsAndHashCodeAreFalse(Object o1, Object o2) {
    assertFalse(o1.equals(o2));
    assertFalse(o1.hashCode() == o2.hashCode());
  }

  public void testRecursiveHashcode() {
    // This tests that we don't infinite loop.
    TestRecursiveOneof.getDefaultInstance().hashCode();
  }

  public void testParseFromByteBuffer() throws Exception {
    TestAllTypesLite message =
        TestAllTypesLite.newBuilder()
            .setOptionalInt32(123)
            .addRepeatedString("hello")
            .setOptionalNestedMessage(NestedMessage.newBuilder().setBb(7))
            .build();

    TestAllTypesLite copy =
        TestAllTypesLite.parseFrom(message.toByteString().asReadOnlyByteBuffer());

    assertEquals(message, copy);
  }

  public void testParseFromByteBufferThrows() {
    try {
      TestAllTypesLite.parseFrom(ByteBuffer.wrap(new byte[] {0x5}));
      fail();
    } catch (InvalidProtocolBufferException expected) {
    }

    TestAllTypesLite message =
        TestAllTypesLite.newBuilder().setOptionalInt32(123).addRepeatedString("hello").build();

    ByteBuffer buffer = ByteBuffer.wrap(message.toByteArray(), 0, message.getSerializedSize() - 1);
    try {
      TestAllTypesLite.parseFrom(buffer);
      fail();
    } catch (InvalidProtocolBufferException expected) {
      assertEquals(
          TestAllTypesLite.newBuilder().setOptionalInt32(123).build(),
          expected.getUnfinishedMessage());
    }
  }

  public void testParseFromByteBuffer_extensions() throws Exception {
    TestAllExtensionsLite message =
        TestAllExtensionsLite.newBuilder()
            .setExtension(UnittestLite.optionalInt32ExtensionLite, 123)
            .addExtension(UnittestLite.repeatedStringExtensionLite, "hello")
            .setExtension(UnittestLite.optionalNestedEnumExtensionLite, NestedEnum.BAZ)
            .setExtension(
                UnittestLite.optionalNestedMessageExtensionLite,
                NestedMessage.newBuilder().setBb(7).build())
            .build();

    ExtensionRegistryLite registry = ExtensionRegistryLite.newInstance();
    UnittestLite.registerAllExtensions(registry);

    TestAllExtensionsLite copy =
        TestAllExtensionsLite.parseFrom(message.toByteString().asReadOnlyByteBuffer(), registry);

    assertEquals(message, copy);
  }

  public void testParseFromByteBufferThrows_extensions() {
    ExtensionRegistryLite registry = ExtensionRegistryLite.newInstance();
    UnittestLite.registerAllExtensions(registry);
    try {
      TestAllExtensionsLite.parseFrom(ByteBuffer.wrap(new byte[] {0x5}), registry);
      fail();
    } catch (InvalidProtocolBufferException expected) {
    }

    TestAllExtensionsLite message =
        TestAllExtensionsLite.newBuilder()
            .setExtension(UnittestLite.optionalInt32ExtensionLite, 123)
            .addExtension(UnittestLite.repeatedStringExtensionLite, "hello")
            .build();

    ByteBuffer buffer = ByteBuffer.wrap(message.toByteArray(), 0, message.getSerializedSize() - 1);
    try {
      TestAllExtensionsLite.parseFrom(buffer, registry);
      fail();
    } catch (InvalidProtocolBufferException expected) {
      assertEquals(
          TestAllExtensionsLite.newBuilder()
              .setExtension(UnittestLite.optionalInt32ExtensionLite, 123)
              .build(),
          expected.getUnfinishedMessage());
    }
  }

  // Make sure we haven't screwed up the code generation for packing fields by default.
  public void testPackedSerialization() throws Exception {
    TestAllTypes.Builder builder = TestAllTypes.newBuilder();
    builder.addRepeatedInt32(4321);
    builder.addRepeatedNestedEnum(TestAllTypes.NestedEnum.BAZ);
    TestAllTypes message = builder.build();

    CodedInputStream in = CodedInputStream.newInstance(message.toByteArray());

    while (!in.isAtEnd()) {
      int tag = in.readTag();
      assertEquals(WireFormat.WIRETYPE_LENGTH_DELIMITED, WireFormat.getTagWireType(tag));
      in.skipField(tag);
    }
  }

  public void testAddAllIteratesOnce() {
    TestAllTypesLite.newBuilder()
        .addAllRepeatedBool(new OneTimeIterableList<>(false))
        .addAllRepeatedInt32(new OneTimeIterableList<>(0))
        .addAllRepeatedInt64(new OneTimeIterableList<>(0L))
        .addAllRepeatedFloat(new OneTimeIterableList<>(0f))
        .addAllRepeatedDouble(new OneTimeIterableList<>(0d))
        .addAllRepeatedBytes(new OneTimeIterableList<>(ByteString.EMPTY))
        .addAllRepeatedString(new OneTimeIterableList<>(""))
        .addAllRepeatedNestedMessage(new OneTimeIterableList<>(NestedMessage.getDefaultInstance()))
        .addAllRepeatedBool(new OneTimeIterable<>(false))
        .addAllRepeatedInt32(new OneTimeIterable<>(0))
        .addAllRepeatedInt64(new OneTimeIterable<>(0L))
        .addAllRepeatedFloat(new OneTimeIterable<>(0f))
        .addAllRepeatedDouble(new OneTimeIterable<>(0d))
        .addAllRepeatedBytes(new OneTimeIterable<>(ByteString.EMPTY))
        .addAllRepeatedString(new OneTimeIterable<>(""))
        .addAllRepeatedNestedMessage(new OneTimeIterable<>(NestedMessage.getDefaultInstance()))
        .build();
  }

  public void testAddAllIteratesOnce_throwsOnNull() {
    TestAllTypesLite.Builder builder = TestAllTypesLite.newBuilder();
    try {
      builder.addAllRepeatedBool(new OneTimeIterableList<>(true, false, null));
      fail();
    } catch (NullPointerException expected) {
      assertEquals("Element at index 2 is null.", expected.getMessage());
      assertEquals(0, builder.getRepeatedBoolCount());
    }

    try {
      builder.addAllRepeatedBool(new OneTimeIterable<>(true, false, null));
      fail();
    } catch (NullPointerException expected) {
      assertEquals("Element at index 2 is null.", expected.getMessage());
      assertEquals(0, builder.getRepeatedBoolCount());
    }

    try {
      builder = TestAllTypesLite.newBuilder();
      builder.addAllRepeatedBool(new OneTimeIterableList<>((Boolean) null));
      fail();
    } catch (NullPointerException expected) {
      assertEquals("Element at index 0 is null.", expected.getMessage());
      assertEquals(0, builder.getRepeatedBoolCount());
    }

    try {
      builder = TestAllTypesLite.newBuilder();
      builder.addAllRepeatedInt32(new OneTimeIterableList<>((Integer) null));
      fail();
    } catch (NullPointerException expected) {
      assertEquals("Element at index 0 is null.", expected.getMessage());
      assertEquals(0, builder.getRepeatedInt32Count());
    }

    try {
      builder = TestAllTypesLite.newBuilder();
      builder.addAllRepeatedInt64(new OneTimeIterableList<>((Long) null));
      fail();
    } catch (NullPointerException expected) {
      assertEquals("Element at index 0 is null.", expected.getMessage());
      assertEquals(0, builder.getRepeatedInt64Count());
    }

    try {
      builder = TestAllTypesLite.newBuilder();
      builder.addAllRepeatedFloat(new OneTimeIterableList<>((Float) null));
      fail();
    } catch (NullPointerException expected) {
      assertEquals("Element at index 0 is null.", expected.getMessage());
      assertEquals(0, builder.getRepeatedFloatCount());
    }

    try {
      builder = TestAllTypesLite.newBuilder();
      builder.addAllRepeatedDouble(new OneTimeIterableList<>((Double) null));
      fail();
    } catch (NullPointerException expected) {
      assertEquals("Element at index 0 is null.", expected.getMessage());
      assertEquals(0, builder.getRepeatedDoubleCount());
    }

    try {
      builder = TestAllTypesLite.newBuilder();
      builder.addAllRepeatedBytes(new OneTimeIterableList<>((ByteString) null));
      fail();
    } catch (NullPointerException expected) {
      assertEquals("Element at index 0 is null.", expected.getMessage());
      assertEquals(0, builder.getRepeatedBytesCount());
    }

    try {
      builder = TestAllTypesLite.newBuilder();
      builder.addAllRepeatedString(new OneTimeIterableList<>("", "", null, ""));
      fail();
    } catch (NullPointerException expected) {
      assertEquals("Element at index 2 is null.", expected.getMessage());
      assertEquals(0, builder.getRepeatedStringCount());
    }

    try {
      builder = TestAllTypesLite.newBuilder();
      builder.addAllRepeatedString(new OneTimeIterable<>("", "", null, ""));
      fail();
    } catch (NullPointerException expected) {
      assertEquals("Element at index 2 is null.", expected.getMessage());
      assertEquals(0, builder.getRepeatedStringCount());
    }

    try {
      builder = TestAllTypesLite.newBuilder();
      builder.addAllRepeatedString(new OneTimeIterableList<>((String) null));
      fail();
    } catch (NullPointerException expected) {
      assertEquals("Element at index 0 is null.", expected.getMessage());
      assertEquals(0, builder.getRepeatedStringCount());
    }

    try {
      builder = TestAllTypesLite.newBuilder();
      builder.addAllRepeatedNestedMessage(new OneTimeIterableList<>((NestedMessage) null));
      fail();
    } catch (NullPointerException expected) {
      assertEquals("Element at index 0 is null.", expected.getMessage());
      assertEquals(0, builder.getRepeatedNestedMessageCount());
    }
  }

  public void testExtensionRenamesKeywords() {
    assertTrue(NonNestedExtensionLite.package_ instanceof GeneratedMessageLite.GeneratedExtension);
    assertTrue(
        NestedExtensionLite.MyNestedExtensionLite.private_
            instanceof GeneratedMessageLite.GeneratedExtension);

    NonNestedExtensionLite.MessageLiteToBeExtended msg =
        NonNestedExtensionLite.MessageLiteToBeExtended.newBuilder()
            .setExtension(NonNestedExtensionLite.package_, true)
            .build();
    assertTrue(msg.getExtension(NonNestedExtensionLite.package_));

    msg =
        NonNestedExtensionLite.MessageLiteToBeExtended.newBuilder()
            .setExtension(NestedExtensionLite.MyNestedExtensionLite.private_, 2.4)
            .build();
    assertEquals(
        2.4, msg.getExtension(NestedExtensionLite.MyNestedExtensionLite.private_), 0.001);
  }

  private static final class OneTimeIterableList<T> extends ArrayList<T> {
    private boolean wasIterated = false;

    OneTimeIterableList(T... contents) {
      addAll(Arrays.asList(contents));
    }

    @Override
    public Iterator<T> iterator() {
      if (wasIterated) {
        fail();
      }
      wasIterated = true;
      return super.iterator();
    }
  }

  private static final class OneTimeIterable<T> implements Iterable<T> {
    private final List<T> list;
    private boolean wasIterated = false;

    OneTimeIterable(T... contents) {
      list = Arrays.asList(contents);
    }

    @Override
    public Iterator<T> iterator() {
      if (wasIterated) {
        fail();
      }
      wasIterated = true;
      return list.iterator();
    }
  }

  public void testNullExtensionRegistry() throws Exception {
    try {
      TestAllTypesLite.parseFrom(new byte[] {}, null);
      fail();
    } catch (NullPointerException expected) {
    }
  }

  public void testSerializeToOutputStreamThrowsIOException() {
    try {
      TestAllTypesLite.newBuilder()
          .setOptionalBytes(ByteString.copyFromUtf8("hello"))
          .build()
          .writeTo(
              new OutputStream() {

                @Override
                public void write(int b) throws IOException {
                  throw new IOException();
                }
              });
      fail();
    } catch (IOException expected) {
    }
  }

  public void testUnpairedSurrogatesReplacedByQuestionMark() throws InvalidProtocolBufferException {
    String testString = "foo \ud83d bar";
    String expectedString = "foo ? bar";

    TestAllTypesLite testMessage =
        TestAllTypesLite.newBuilder().setOptionalString(testString).build();
    ByteString serializedMessage = testMessage.toByteString();

    // Behavior is compatible with String.getBytes("UTF-8"), which replaces
    // unpaired surrogates with a question mark.
    TestAllTypesLite parsedMessage = TestAllTypesLite.parseFrom(serializedMessage);
    assertEquals(expectedString, parsedMessage.getOptionalString());

    // Conversion happens during serialization.
    ByteString expectedBytes = ByteString.copyFromUtf8(expectedString);
    assertTrue(
        String.format(
            "Expected serializedMessage (%s) to contain \"%s\" (%s).",
            encodeHex(serializedMessage), expectedString, encodeHex(expectedBytes)),
        contains(serializedMessage, expectedBytes));
  }

  private String encodeHex(ByteString bytes) {
    String hexDigits = "0123456789abcdef";
    StringBuilder stringBuilder = new StringBuilder(bytes.size() * 2);
    for (byte b : bytes) {
      stringBuilder.append(hexDigits.charAt((b & 0xf0) >> 4));
      stringBuilder.append(hexDigits.charAt(b & 0x0f));
    }
    return stringBuilder.toString();
  }

  private boolean contains(ByteString a, ByteString b) {
    for (int i = 0; i <= a.size() - b.size(); ++i) {
      if (a.substring(i, i + b.size()).equals(b)) {
        return true;
      }
    }
    return false;
  }
}
