// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

package com.google.protobuf;

import static com.google.common.truth.Truth.assertThat;
import static com.google.common.truth.Truth.assertWithMessage;
import static java.util.Collections.singletonList;

import com.google.protobuf.FieldPresenceTestProto.TestAllTypes;
import com.google.protobuf.UnittestImportLite.ImportEnumLite;
import com.google.protobuf.UnittestImportPublicLite.PublicImportMessageLite;
import com.google.protobuf.UnittestLite.ForeignEnumLite;
import com.google.protobuf.UnittestLite.ForeignMessageLite;
import com.google.protobuf.UnittestLite.RecursiveMessage;
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
import com.google.protobuf.testing.Proto3TestingLite.Proto3MessageLite;
import map_lite_test.MapTestProto.TestMap;
import map_lite_test.MapTestProto.TestMap.MessageValue;
import protobuf_unittest.NestedExtensionLite;
import protobuf_unittest.NonNestedExtensionLite;
import protobuf_unittest.UnittestProto.TestOneof2;
import protobuf_unittest.lite_equals_and_hash.LiteEqualsAndHash.Bar;
import protobuf_unittest.lite_equals_and_hash.LiteEqualsAndHash.BarPrime;
import protobuf_unittest.lite_equals_and_hash.LiteEqualsAndHash.Foo;
import protobuf_unittest.lite_equals_and_hash.LiteEqualsAndHash.TestOneofEquals;
import protobuf_unittest.lite_equals_and_hash.LiteEqualsAndHash.TestRecursiveOneof;
import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.lang.reflect.Field;
import java.nio.ByteBuffer;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Iterator;
import java.util.List;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

/** Test lite runtime. */
@RunWith(JUnit4.class)
public class LiteTest {

  @Before
  public void setUp() throws Exception {
    // Test that nested extensions are initialized correctly even if the outer
    // class has not been accessed directly.  This was once a bug with lite
    // messages.
    //
    // We put this in setUp() rather than in its own test method because we
    // need to make sure it runs before any actual tests.
    assertThat(TestNestedExtensionLite.nestedExtension).isNotNull();
  }

  @Test
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

    assertThat(message2.getOptionalInt32()).isEqualTo(123);
    assertThat(message2.getRepeatedStringCount()).isEqualTo(1);
    assertThat(message2.getRepeatedString(0)).isEqualTo("hello");
    assertThat(message2.getOptionalNestedMessage().getBb()).isEqualTo(7);
  }

  @Test
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
    TestAllTypesLite unused =
        TestAllTypesLite.parseFrom(new ByteArrayInputStream(byteStream.toByteArray()));
  }

  @Test
  public void testLiteExtensions() throws Exception {
    // TODO:  Unlike other features of the lite library, extensions are
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

    assertThat((int) message2.getExtension(UnittestLite.optionalInt32ExtensionLite)).isEqualTo(123);
    assertThat(message2.getExtensionCount(UnittestLite.repeatedStringExtensionLite)).isEqualTo(1);
    assertThat(message2.getExtension(UnittestLite.repeatedStringExtensionLite)).hasSize(1);
    assertThat(message2.getExtension(UnittestLite.repeatedStringExtensionLite, 0))
        .isEqualTo("hello");
    assertThat(message2.getExtension(UnittestLite.optionalNestedEnumExtensionLite))
        .isEqualTo(NestedEnum.BAZ);
    assertThat(message2.getExtension(UnittestLite.optionalNestedMessageExtensionLite).getBb())
        .isEqualTo(7);
  }

  @Test
  public void testClone() {
    TestAllTypesLite.Builder expected = TestAllTypesLite.newBuilder().setOptionalInt32(123);
    assertThat(expected.getOptionalInt32()).isEqualTo(expected.clone().getOptionalInt32());

    TestAllExtensionsLite.Builder expected2 =
        TestAllExtensionsLite.newBuilder()
            .setExtension(UnittestLite.optionalInt32ExtensionLite, 123);
    assertThat(expected2.getExtension(UnittestLite.optionalInt32ExtensionLite))
        .isEqualTo(expected2.clone().getExtension(UnittestLite.optionalInt32ExtensionLite));
  }

  @Test
  public void testAddAll() {
    try {
      TestAllTypesLite.newBuilder().addAllRepeatedBytes(null);
      assertWithMessage("expected exception").fail();
    } catch (NullPointerException e) {
      // expected.
    }
  }

  @Test
  public void testParsedOneofSubMessageIsImmutable() throws InvalidProtocolBufferException {
    TestAllTypesLite message =
        TestAllTypesLite.parseFrom(
            TestAllTypesLite.newBuilder()
                .setOneofNestedMessage(NestedMessage.newBuilder().addDd(1234).build())
                .build()
                .toByteArray());
    IntArrayList subList = (IntArrayList) message.getOneofNestedMessage().getDdList();
    assertThat(subList.isModifiable()).isFalse();
  }

  @Test
  public void testMemoization() throws Exception {
    GeneratedMessageLite<?, ?> message = TestUtilLite.getAllLiteExtensionsSet();

    // This built message should not be mutable
    assertThat(message.isMutable()).isFalse();

    // Test serialized size is memoized
    assertThat(message.getMemoizedSerializedSize())
        .isEqualTo(GeneratedMessageLite.UNINITIALIZED_SERIALIZED_SIZE);
    int size = message.getSerializedSize();
    assertThat(size).isGreaterThan(0);
    assertThat(message.getMemoizedSerializedSize()).isEqualTo(size);
    message.clearMemoizedSerializedSize();
    assertThat(message.getMemoizedSerializedSize())
        .isEqualTo(GeneratedMessageLite.UNINITIALIZED_SERIALIZED_SIZE);

    // Test hashCode is memoized
    assertThat(message.hashCodeIsNotMemoized()).isTrue();
    int hashCode = message.hashCode();
    assertThat(message.hashCodeIsNotMemoized()).isFalse();
    assertThat(message.getMemoizedHashCode()).isEqualTo(hashCode);
    message.clearMemoizedHashCode();
    assertThat(message.hashCodeIsNotMemoized()).isTrue();

    // Test isInitialized is memoized
    Field memo = message.getClass().getDeclaredField("memoizedIsInitialized");
    memo.setAccessible(true);
    memo.set(message, (byte) -1);
    boolean initialized = message.isInitialized();
    assertThat(initialized).isTrue();
    // We have to cast to Byte first. Casting to byte causes a type error
    assertThat(((Byte) memo.get(message)).intValue()).isEqualTo(1);
  }

  @Test
  public void testProto3EnumListValueCopyOnWrite() {
    Proto3MessageLite.Builder builder = Proto3MessageLite.newBuilder();

    Proto3MessageLite message = builder.build();
    builder.addFieldEnumList30Value(Proto3MessageLite.TestEnum.ONE_VALUE);
    assertThat(message.getFieldEnumList30List()).isEmpty();
    assertThat(builder.getFieldEnumList30List()).containsExactly(Proto3MessageLite.TestEnum.ONE);
    assertThat(message.getFieldEnumList30List()).isEmpty();
    Proto3MessageLite messageAfterBuild = builder.build();
    builder.clearFieldEnumList30();
    assertThat(builder.getFieldEnumList30List()).isEmpty();
    assertThat(messageAfterBuild.getFieldEnumList30List())
        .containsExactly(Proto3MessageLite.TestEnum.ONE);
  }

  @Test
  public void testSanityCopyOnWrite() throws InvalidProtocolBufferException {
    // Since builders are implemented as a thin wrapper around a message
    // instance, we attempt to verify that we can't cause the builder to modify
    // a produced message.

    TestAllTypesLite.Builder builder = TestAllTypesLite.newBuilder();
    TestAllTypesLite message = builder.build();
    TestAllTypesLite messageAfterBuild;
    builder.setOptionalBool(true);
    assertThat(message.getOptionalBool()).isFalse();
    assertThat(builder.getOptionalBool()).isTrue();
    messageAfterBuild = builder.build();
    assertThat(messageAfterBuild.getOptionalBool()).isTrue();
    assertThat(message.getOptionalBool()).isFalse();
    builder.clearOptionalBool();
    assertThat(builder.getOptionalBool()).isFalse();
    assertThat(messageAfterBuild.getOptionalBool()).isTrue();

    message = builder.build();
    builder.setOptionalBytes(ByteString.copyFromUtf8("hi"));
    assertThat(message.getOptionalBytes()).isEqualTo(ByteString.EMPTY);
    assertThat(builder.getOptionalBytes()).isEqualTo(ByteString.copyFromUtf8("hi"));
    messageAfterBuild = builder.build();
    assertThat(messageAfterBuild.getOptionalBytes()).isEqualTo(ByteString.copyFromUtf8("hi"));
    assertThat(message.getOptionalBytes()).isEqualTo(ByteString.EMPTY);
    builder.clearOptionalBytes();
    assertThat(builder.getOptionalBytes()).isEqualTo(ByteString.EMPTY);
    assertThat(messageAfterBuild.getOptionalBytes()).isEqualTo(ByteString.copyFromUtf8("hi"));

    message = builder.build();
    builder.setOptionalCord("hi");
    assertThat(message.getOptionalCord()).isEmpty();
    assertThat(builder.getOptionalCord()).isEqualTo("hi");
    messageAfterBuild = builder.build();
    assertThat(messageAfterBuild.getOptionalCord()).isEqualTo("hi");
    assertThat(message.getOptionalCord()).isEmpty();
    builder.clearOptionalCord();
    assertThat(builder.getOptionalCord()).isEmpty();
    assertThat(messageAfterBuild.getOptionalCord()).isEqualTo("hi");

    message = builder.build();
    builder.setOptionalCordBytes(ByteString.copyFromUtf8("no"));
    assertThat(message.getOptionalCordBytes()).isEqualTo(ByteString.EMPTY);
    assertThat(builder.getOptionalCordBytes()).isEqualTo(ByteString.copyFromUtf8("no"));
    messageAfterBuild = builder.build();
    assertThat(messageAfterBuild.getOptionalCordBytes()).isEqualTo(ByteString.copyFromUtf8("no"));
    assertThat(message.getOptionalCordBytes()).isEqualTo(ByteString.EMPTY);
    builder.clearOptionalCord();
    assertThat(builder.getOptionalCordBytes()).isEqualTo(ByteString.EMPTY);
    assertThat(messageAfterBuild.getOptionalCordBytes()).isEqualTo(ByteString.copyFromUtf8("no"));

    message = builder.build();
    builder.setOptionalDouble(1);
    assertThat(message.getOptionalDouble()).isEqualTo(0D);
    assertThat(builder.getOptionalDouble()).isEqualTo(1D);
    messageAfterBuild = builder.build();
    assertThat(messageAfterBuild.getOptionalDouble()).isEqualTo(1D);
    assertThat(message.getOptionalDouble()).isEqualTo(0D);
    builder.clearOptionalDouble();
    assertThat(builder.getOptionalDouble()).isEqualTo(0D);
    assertThat(messageAfterBuild.getOptionalDouble()).isEqualTo(1D);

    message = builder.build();
    builder.setOptionalFixed32(1);
    assertThat(message.getOptionalFixed32()).isEqualTo(0);
    assertThat(builder.getOptionalFixed32()).isEqualTo(1);
    messageAfterBuild = builder.build();
    assertThat(messageAfterBuild.getOptionalFixed32()).isEqualTo(1);
    assertThat(message.getOptionalFixed32()).isEqualTo(0);
    builder.clearOptionalFixed32();
    assertThat(builder.getOptionalFixed32()).isEqualTo(0);
    assertThat(messageAfterBuild.getOptionalFixed32()).isEqualTo(1);

    message = builder.build();
    builder.setOptionalFixed64(1);
    assertThat(message.getOptionalFixed64()).isEqualTo(0L);
    assertThat(builder.getOptionalFixed64()).isEqualTo(1L);
    messageAfterBuild = builder.build();
    assertThat(messageAfterBuild.getOptionalFixed64()).isEqualTo(1L);
    assertThat(message.getOptionalFixed64()).isEqualTo(0L);
    builder.clearOptionalFixed64();
    assertThat(builder.getOptionalFixed64()).isEqualTo(0L);
    assertThat(messageAfterBuild.getOptionalFixed64()).isEqualTo(1L);

    message = builder.build();
    builder.setOptionalFloat(1);
    assertThat(message.getOptionalFloat()).isEqualTo(0F);
    assertThat(builder.getOptionalFloat()).isEqualTo(1F);
    messageAfterBuild = builder.build();
    assertThat(messageAfterBuild.getOptionalFloat()).isEqualTo(1F);
    assertThat(message.getOptionalFloat()).isEqualTo(0F);
    builder.clearOptionalFloat();
    assertThat(builder.getOptionalFloat()).isEqualTo(0F);
    assertThat(messageAfterBuild.getOptionalFloat()).isEqualTo(1F);

    message = builder.build();
    builder.setOptionalForeignEnum(ForeignEnumLite.FOREIGN_LITE_BAR);
    assertThat(message.getOptionalForeignEnum()).isEqualTo(ForeignEnumLite.FOREIGN_LITE_FOO);
    assertThat(builder.getOptionalForeignEnum()).isEqualTo(ForeignEnumLite.FOREIGN_LITE_BAR);
    messageAfterBuild = builder.build();
    assertThat(messageAfterBuild.getOptionalForeignEnum())
        .isEqualTo(ForeignEnumLite.FOREIGN_LITE_BAR);
    assertThat(message.getOptionalForeignEnum()).isEqualTo(ForeignEnumLite.FOREIGN_LITE_FOO);
    builder.clearOptionalForeignEnum();
    assertThat(builder.getOptionalForeignEnum()).isEqualTo(ForeignEnumLite.FOREIGN_LITE_FOO);
    assertThat(messageAfterBuild.getOptionalForeignEnum())
        .isEqualTo(ForeignEnumLite.FOREIGN_LITE_BAR);

    message = builder.build();
    ForeignMessageLite foreignMessage = ForeignMessageLite.newBuilder().setC(1).build();
    builder.setOptionalForeignMessage(foreignMessage);
    assertThat(message.getOptionalForeignMessage())
        .isEqualTo(ForeignMessageLite.getDefaultInstance());
    assertThat(builder.getOptionalForeignMessage()).isEqualTo(foreignMessage);
    messageAfterBuild = builder.build();
    assertThat(messageAfterBuild.getOptionalForeignMessage()).isEqualTo(foreignMessage);
    assertThat(message.getOptionalForeignMessage())
        .isEqualTo(ForeignMessageLite.getDefaultInstance());
    builder.clearOptionalForeignMessage();
    assertThat(builder.getOptionalForeignMessage())
        .isEqualTo(ForeignMessageLite.getDefaultInstance());
    assertThat(messageAfterBuild.getOptionalForeignMessage()).isEqualTo(foreignMessage);

    message = builder.build();
    ForeignMessageLite foreignMessageC3 = ForeignMessageLite.newBuilder().setC(3).build();
    builder.setOptionalForeignMessage(foreignMessageC3);
    assertThat(message.getOptionalForeignMessage())
        .isEqualTo(ForeignMessageLite.getDefaultInstance());
    assertThat(builder.getOptionalForeignMessage()).isEqualTo(foreignMessageC3);
    messageAfterBuild = builder.build();
    assertThat(messageAfterBuild.getOptionalForeignMessage()).isEqualTo(foreignMessageC3);
    assertThat(message.getOptionalForeignMessage())
        .isEqualTo(ForeignMessageLite.getDefaultInstance());
    builder.clearOptionalForeignMessage();
    assertThat(builder.getOptionalForeignMessage())
        .isEqualTo(ForeignMessageLite.getDefaultInstance());
    assertThat(messageAfterBuild.getOptionalForeignMessage()).isEqualTo(foreignMessageC3);

    message = builder.build();
    OptionalGroup optionalGroup = OptionalGroup.newBuilder().setA(1).build();
    builder.setOptionalGroup(optionalGroup);
    assertThat(message.getOptionalGroup()).isEqualTo(OptionalGroup.getDefaultInstance());
    assertThat(builder.getOptionalGroup()).isEqualTo(optionalGroup);
    messageAfterBuild = builder.build();
    assertThat(messageAfterBuild.getOptionalGroup()).isEqualTo(optionalGroup);
    assertThat(message.getOptionalGroup()).isEqualTo(OptionalGroup.getDefaultInstance());
    builder.clearOptionalGroup();
    assertThat(builder.getOptionalGroup()).isEqualTo(OptionalGroup.getDefaultInstance());
    assertThat(messageAfterBuild.getOptionalGroup()).isEqualTo(optionalGroup);

    message = builder.build();
    OptionalGroup.Builder optionalGroupBuilder = OptionalGroup.newBuilder().setA(3);
    builder.setOptionalGroup(optionalGroupBuilder);
    assertThat(message.getOptionalGroup()).isEqualTo(OptionalGroup.getDefaultInstance());
    assertThat(builder.getOptionalGroup()).isEqualTo(optionalGroupBuilder.build());
    messageAfterBuild = builder.build();
    assertThat(messageAfterBuild.getOptionalGroup()).isEqualTo(optionalGroupBuilder.build());
    assertThat(message.getOptionalGroup()).isEqualTo(OptionalGroup.getDefaultInstance());
    builder.clearOptionalGroup();
    assertThat(builder.getOptionalGroup()).isEqualTo(OptionalGroup.getDefaultInstance());
    assertThat(messageAfterBuild.getOptionalGroup()).isEqualTo(optionalGroupBuilder.build());

    message = builder.build();
    builder.setOptionalInt32(1);
    assertThat(message.getOptionalInt32()).isEqualTo(0);
    assertThat(builder.getOptionalInt32()).isEqualTo(1);
    messageAfterBuild = builder.build();
    assertThat(messageAfterBuild.getOptionalInt32()).isEqualTo(1);
    assertThat(message.getOptionalInt32()).isEqualTo(0);
    builder.clearOptionalInt32();
    assertThat(builder.getOptionalInt32()).isEqualTo(0);
    assertThat(messageAfterBuild.getOptionalInt32()).isEqualTo(1);

    message = builder.build();
    builder.setOptionalInt64(1);
    assertThat(message.getOptionalInt64()).isEqualTo(0L);
    assertThat(builder.getOptionalInt64()).isEqualTo(1L);
    messageAfterBuild = builder.build();
    assertThat(messageAfterBuild.getOptionalInt64()).isEqualTo(1L);
    assertThat(message.getOptionalInt64()).isEqualTo(0L);
    builder.clearOptionalInt64();
    assertThat(builder.getOptionalInt64()).isEqualTo(0L);
    assertThat(messageAfterBuild.getOptionalInt64()).isEqualTo(1L);

    message = builder.build();
    NestedMessage nestedMessage = NestedMessage.newBuilder().setBb(1).build();
    builder.setOptionalLazyMessage(nestedMessage);
    assertThat(message.getOptionalLazyMessage()).isEqualTo(NestedMessage.getDefaultInstance());
    assertThat(builder.getOptionalLazyMessage()).isEqualTo(nestedMessage);
    messageAfterBuild = builder.build();
    assertThat(messageAfterBuild.getOptionalLazyMessage()).isEqualTo(nestedMessage);
    assertThat(message.getOptionalLazyMessage()).isEqualTo(NestedMessage.getDefaultInstance());
    builder.clearOptionalLazyMessage();
    assertThat(builder.getOptionalLazyMessage()).isEqualTo(NestedMessage.getDefaultInstance());
    assertThat(messageAfterBuild.getOptionalLazyMessage()).isEqualTo(nestedMessage);

    message = builder.build();
    NestedMessage.Builder nestedMessageBuilder = NestedMessage.newBuilder().setBb(3);
    builder.setOptionalLazyMessage(nestedMessageBuilder);
    assertThat(message.getOptionalLazyMessage()).isEqualTo(NestedMessage.getDefaultInstance());
    assertThat(builder.getOptionalLazyMessage()).isEqualTo(nestedMessageBuilder.build());
    messageAfterBuild = builder.build();
    assertThat(messageAfterBuild.getOptionalLazyMessage()).isEqualTo(nestedMessageBuilder.build());
    assertThat(message.getOptionalLazyMessage()).isEqualTo(NestedMessage.getDefaultInstance());
    builder.clearOptionalLazyMessage();
    assertThat(builder.getOptionalLazyMessage()).isEqualTo(NestedMessage.getDefaultInstance());
    assertThat(messageAfterBuild.getOptionalLazyMessage()).isEqualTo(nestedMessageBuilder.build());

    message = builder.build();
    builder.setOptionalSfixed32(1);
    assertThat(message.getOptionalSfixed32()).isEqualTo(0);
    assertThat(builder.getOptionalSfixed32()).isEqualTo(1);
    messageAfterBuild = builder.build();
    assertThat(messageAfterBuild.getOptionalSfixed32()).isEqualTo(1);
    assertThat(message.getOptionalSfixed32()).isEqualTo(0);
    builder.clearOptionalSfixed32();
    assertThat(builder.getOptionalSfixed32()).isEqualTo(0);
    assertThat(messageAfterBuild.getOptionalSfixed32()).isEqualTo(1);

    message = builder.build();
    builder.setOptionalSfixed64(1);
    assertThat(message.getOptionalSfixed64()).isEqualTo(0L);
    assertThat(builder.getOptionalSfixed64()).isEqualTo(1L);
    messageAfterBuild = builder.build();
    assertThat(messageAfterBuild.getOptionalSfixed64()).isEqualTo(1L);
    assertThat(message.getOptionalSfixed64()).isEqualTo(0L);
    builder.clearOptionalSfixed64();
    assertThat(builder.getOptionalSfixed64()).isEqualTo(0L);
    assertThat(messageAfterBuild.getOptionalSfixed64()).isEqualTo(1L);

    message = builder.build();
    builder.setOptionalSint32(1);
    assertThat(message.getOptionalSint32()).isEqualTo(0);
    assertThat(builder.getOptionalSint32()).isEqualTo(1);
    messageAfterBuild = builder.build();
    assertThat(messageAfterBuild.getOptionalSint32()).isEqualTo(1);
    builder.clearOptionalSint32();
    assertThat(builder.getOptionalSint32()).isEqualTo(0);
    assertThat(messageAfterBuild.getOptionalSint32()).isEqualTo(1);

    message = builder.build();
    builder.setOptionalSint64(1);
    assertThat(message.getOptionalSint64()).isEqualTo(0L);
    assertThat(builder.getOptionalSint64()).isEqualTo(1L);
    messageAfterBuild = builder.build();
    assertThat(messageAfterBuild.getOptionalSint64()).isEqualTo(1L);
    assertThat(message.getOptionalSint64()).isEqualTo(0L);
    builder.clearOptionalSint64();
    assertThat(builder.getOptionalSint64()).isEqualTo(0L);
    assertThat(messageAfterBuild.getOptionalSint64()).isEqualTo(1L);

    message = builder.build();
    builder.setOptionalString("hi");
    assertThat(message.getOptionalString()).isEmpty();
    assertThat(builder.getOptionalString()).isEqualTo("hi");
    messageAfterBuild = builder.build();
    assertThat(messageAfterBuild.getOptionalString()).isEqualTo("hi");
    assertThat(message.getOptionalString()).isEmpty();
    builder.clearOptionalString();
    assertThat(builder.getOptionalString()).isEmpty();
    assertThat(messageAfterBuild.getOptionalString()).isEqualTo("hi");

    message = builder.build();
    builder.setOptionalStringBytes(ByteString.copyFromUtf8("no"));
    assertThat(message.getOptionalStringBytes()).isEqualTo(ByteString.EMPTY);
    assertThat(builder.getOptionalStringBytes()).isEqualTo(ByteString.copyFromUtf8("no"));
    messageAfterBuild = builder.build();
    assertThat(messageAfterBuild.getOptionalStringBytes()).isEqualTo(ByteString.copyFromUtf8("no"));
    assertThat(message.getOptionalStringBytes()).isEqualTo(ByteString.EMPTY);
    builder.clearOptionalString();
    assertThat(builder.getOptionalStringBytes()).isEqualTo(ByteString.EMPTY);
    assertThat(messageAfterBuild.getOptionalStringBytes()).isEqualTo(ByteString.copyFromUtf8("no"));

    message = builder.build();
    builder.setOptionalStringPiece("hi");
    assertThat(message.getOptionalStringPiece()).isEmpty();
    assertThat(builder.getOptionalStringPiece()).isEqualTo("hi");
    messageAfterBuild = builder.build();
    assertThat(messageAfterBuild.getOptionalStringPiece()).isEqualTo("hi");
    assertThat(message.getOptionalStringPiece()).isEmpty();
    builder.clearOptionalStringPiece();
    assertThat(builder.getOptionalStringPiece()).isEmpty();
    assertThat(messageAfterBuild.getOptionalStringPiece()).isEqualTo("hi");

    message = builder.build();
    builder.setOptionalStringPieceBytes(ByteString.copyFromUtf8("no"));
    assertThat(message.getOptionalStringPieceBytes()).isEqualTo(ByteString.EMPTY);
    assertThat(builder.getOptionalStringPieceBytes()).isEqualTo(ByteString.copyFromUtf8("no"));
    messageAfterBuild = builder.build();
    assertThat(messageAfterBuild.getOptionalStringPieceBytes())
        .isEqualTo(ByteString.copyFromUtf8("no"));
    assertThat(message.getOptionalStringPieceBytes()).isEqualTo(ByteString.EMPTY);
    builder.clearOptionalStringPiece();
    assertThat(builder.getOptionalStringPieceBytes()).isEqualTo(ByteString.EMPTY);
    assertThat(messageAfterBuild.getOptionalStringPieceBytes())
        .isEqualTo(ByteString.copyFromUtf8("no"));

    message = builder.build();
    builder.setOptionalUint32(1);
    assertThat(message.getOptionalUint32()).isEqualTo(0);
    assertThat(builder.getOptionalUint32()).isEqualTo(1);
    messageAfterBuild = builder.build();
    assertThat(messageAfterBuild.getOptionalUint32()).isEqualTo(1);
    assertThat(message.getOptionalUint32()).isEqualTo(0);
    builder.clearOptionalUint32();
    assertThat(builder.getOptionalUint32()).isEqualTo(0);
    assertThat(messageAfterBuild.getOptionalUint32()).isEqualTo(1);

    message = builder.build();
    builder.setOptionalUint64(1);
    assertThat(message.getOptionalUint64()).isEqualTo(0L);
    assertThat(builder.getOptionalUint64()).isEqualTo(1L);
    messageAfterBuild = builder.build();
    assertThat(messageAfterBuild.getOptionalUint64()).isEqualTo(1L);
    assertThat(message.getOptionalUint64()).isEqualTo(0L);
    builder.clearOptionalUint64();
    assertThat(builder.getOptionalUint64()).isEqualTo(0L);
    assertThat(messageAfterBuild.getOptionalUint64()).isEqualTo(1L);

    message = builder.build();
    builder.addAllRepeatedBool(singletonList(true));
    assertThat(message.getRepeatedBoolList()).isEmpty();
    assertThat(builder.getRepeatedBoolList()).containsExactly(true);
    assertThat(message.getRepeatedBoolList()).isEmpty();
    messageAfterBuild = builder.build();
    builder.clearRepeatedBool();
    assertThat(builder.getRepeatedBoolList()).isEmpty();
    assertThat(messageAfterBuild.getRepeatedBoolList()).containsExactly(true);

    message = builder.build();
    builder.addAllRepeatedBytes(singletonList(ByteString.copyFromUtf8("hi")));
    assertThat(message.getRepeatedBytesList()).isEmpty();
    assertThat(builder.getRepeatedBytesList()).containsExactly(ByteString.copyFromUtf8("hi"));
    assertThat(message.getRepeatedBytesList()).isEmpty();
    messageAfterBuild = builder.build();
    builder.clearRepeatedBytes();
    assertThat(builder.getRepeatedBytesList()).isEmpty();
    assertThat(messageAfterBuild.getRepeatedBytesList())
        .containsExactly(ByteString.copyFromUtf8("hi"));

    message = builder.build();
    builder.addAllRepeatedCord(singletonList("hi"));
    assertThat(message.getRepeatedCordList()).isEmpty();
    assertThat(builder.getRepeatedCordList()).containsExactly("hi");
    assertThat(message.getRepeatedCordList()).isEmpty();
    messageAfterBuild = builder.build();
    builder.clearRepeatedCord();
    assertThat(builder.getRepeatedCordList()).isEmpty();
    assertThat(messageAfterBuild.getRepeatedCordList()).containsExactly("hi");

    message = builder.build();
    builder.addAllRepeatedDouble(singletonList(1D));
    assertThat(message.getRepeatedDoubleList()).isEmpty();
    assertThat(builder.getRepeatedDoubleList()).containsExactly(1D);
    assertThat(message.getRepeatedDoubleList()).isEmpty();
    messageAfterBuild = builder.build();
    builder.clearRepeatedDouble();
    assertThat(builder.getRepeatedDoubleList()).isEmpty();
    assertThat(messageAfterBuild.getRepeatedDoubleList()).containsExactly(1D);

    message = builder.build();
    builder.addAllRepeatedFixed32(singletonList(1));
    assertThat(message.getRepeatedFixed32List()).isEmpty();
    assertThat(builder.getRepeatedFixed32List()).containsExactly(1);
    assertThat(message.getRepeatedFixed32List()).isEmpty();
    messageAfterBuild = builder.build();
    builder.clearRepeatedFixed32();
    assertThat(builder.getRepeatedFixed32List()).isEmpty();
    assertThat(messageAfterBuild.getRepeatedFixed32List()).containsExactly(1);

    message = builder.build();
    builder.addAllRepeatedFixed64(singletonList(1L));
    assertThat(message.getRepeatedFixed64List()).isEmpty();
    assertThat(builder.getRepeatedFixed64List()).containsExactly(1L);
    assertThat(message.getRepeatedFixed64List()).isEmpty();
    messageAfterBuild = builder.build();
    builder.clearRepeatedFixed64();
    assertThat(builder.getRepeatedFixed64List()).isEmpty();
    assertThat(messageAfterBuild.getRepeatedFixed64List()).containsExactly(1L);

    message = builder.build();
    builder.addAllRepeatedFloat(singletonList(1F));
    assertThat(message.getRepeatedFloatList()).isEmpty();
    assertThat(builder.getRepeatedFloatList()).containsExactly(1F);
    assertThat(message.getRepeatedFloatList()).isEmpty();
    messageAfterBuild = builder.build();
    builder.clearRepeatedFloat();
    assertThat(builder.getRepeatedFloatList()).isEmpty();
    assertThat(messageAfterBuild.getRepeatedFloatList()).containsExactly(1F);

    message = builder.build();
    builder.addAllRepeatedForeignEnum(singletonList(ForeignEnumLite.FOREIGN_LITE_BAR));
    assertThat(message.getRepeatedForeignEnumList()).isEmpty();
    assertThat(builder.getRepeatedForeignEnumList())
        .containsExactly(ForeignEnumLite.FOREIGN_LITE_BAR);
    assertThat(message.getRepeatedForeignEnumList()).isEmpty();
    messageAfterBuild = builder.build();
    builder.clearRepeatedForeignEnum();
    assertThat(builder.getRepeatedForeignEnumList()).isEmpty();
    assertThat(messageAfterBuild.getRepeatedForeignEnumList())
        .containsExactly(ForeignEnumLite.FOREIGN_LITE_BAR);

    message = builder.build();
    builder.addAllRepeatedForeignMessage(singletonList(foreignMessage));
    assertThat(message.getRepeatedForeignMessageList()).isEmpty();
    assertThat(builder.getRepeatedForeignMessageList()).containsExactly(foreignMessage);
    assertThat(message.getRepeatedForeignMessageList()).isEmpty();
    messageAfterBuild = builder.build();
    builder.clearRepeatedForeignMessage();
    assertThat(builder.getRepeatedForeignMessageList()).isEmpty();
    assertThat(messageAfterBuild.getRepeatedForeignMessageList()).containsExactly(foreignMessage);

    message = builder.build();
    builder.addAllRepeatedGroup(singletonList(RepeatedGroup.getDefaultInstance()));
    assertThat(message.getRepeatedGroupList()).isEmpty();
    assertThat(builder.getRepeatedGroupList()).containsExactly(RepeatedGroup.getDefaultInstance());
    assertThat(message.getRepeatedGroupList()).isEmpty();
    messageAfterBuild = builder.build();
    builder.clearRepeatedGroup();
    assertThat(builder.getRepeatedGroupList()).isEmpty();
    assertThat(messageAfterBuild.getRepeatedGroupList())
        .containsExactly(RepeatedGroup.getDefaultInstance());

    message = builder.build();
    builder.addAllRepeatedInt32(singletonList(1));
    assertThat(message.getRepeatedInt32List()).isEmpty();
    assertThat(builder.getRepeatedInt32List()).containsExactly(1);
    assertThat(message.getRepeatedInt32List()).isEmpty();
    messageAfterBuild = builder.build();
    builder.clearRepeatedInt32();
    assertThat(builder.getRepeatedInt32List()).isEmpty();
    assertThat(messageAfterBuild.getRepeatedInt32List()).containsExactly(1);

    message = builder.build();
    builder.addAllRepeatedInt64(singletonList(1L));
    assertThat(message.getRepeatedInt64List()).isEmpty();
    assertThat(builder.getRepeatedInt64List()).containsExactly(1L);
    assertThat(message.getRepeatedInt64List()).isEmpty();
    messageAfterBuild = builder.build();
    builder.clearRepeatedInt64();
    assertThat(builder.getRepeatedInt64List()).isEmpty();
    assertThat(messageAfterBuild.getRepeatedInt64List()).containsExactly(1L);

    message = builder.build();
    builder.addAllRepeatedLazyMessage(singletonList(nestedMessage));
    assertThat(message.getRepeatedLazyMessageList()).isEmpty();
    assertThat(builder.getRepeatedLazyMessageList()).containsExactly(nestedMessage);
    assertThat(message.getRepeatedLazyMessageList()).isEmpty();
    messageAfterBuild = builder.build();
    builder.clearRepeatedLazyMessage();
    assertThat(builder.getRepeatedLazyMessageList()).isEmpty();
    assertThat(messageAfterBuild.getRepeatedLazyMessageList()).containsExactly(nestedMessage);

    message = builder.build();
    builder.addAllRepeatedSfixed32(singletonList(1));
    assertThat(message.getRepeatedSfixed32List()).isEmpty();
    assertThat(builder.getRepeatedSfixed32List()).containsExactly(1);
    assertThat(message.getRepeatedSfixed32List()).isEmpty();
    messageAfterBuild = builder.build();
    builder.clearRepeatedSfixed32();
    assertThat(builder.getRepeatedSfixed32List()).isEmpty();
    assertThat(messageAfterBuild.getRepeatedSfixed32List()).containsExactly(1);

    message = builder.build();
    builder.addAllRepeatedSfixed64(singletonList(1L));
    assertThat(message.getRepeatedSfixed64List()).isEmpty();
    assertThat(builder.getRepeatedSfixed64List()).containsExactly(1L);
    assertThat(message.getRepeatedSfixed64List()).isEmpty();
    messageAfterBuild = builder.build();
    builder.clearRepeatedSfixed64();
    assertThat(builder.getRepeatedSfixed64List()).isEmpty();
    assertThat(messageAfterBuild.getRepeatedSfixed64List()).containsExactly(1L);

    message = builder.build();
    builder.addAllRepeatedSint32(singletonList(1));
    assertThat(message.getRepeatedSint32List()).isEmpty();
    assertThat(builder.getRepeatedSint32List()).containsExactly(1);
    assertThat(message.getRepeatedSint32List()).isEmpty();
    messageAfterBuild = builder.build();
    builder.clearRepeatedSint32();
    assertThat(builder.getRepeatedSint32List()).isEmpty();
    assertThat(messageAfterBuild.getRepeatedSint32List()).containsExactly(1);

    message = builder.build();
    builder.addAllRepeatedSint64(singletonList(1L));
    assertThat(message.getRepeatedSint64List()).isEmpty();
    assertThat(builder.getRepeatedSint64List()).containsExactly(1L);
    assertThat(message.getRepeatedSint64List()).isEmpty();
    messageAfterBuild = builder.build();
    builder.clearRepeatedSint64();
    assertThat(builder.getRepeatedSint64List()).isEmpty();
    assertThat(messageAfterBuild.getRepeatedSint64List()).containsExactly(1L);

    message = builder.build();
    builder.addAllRepeatedString(singletonList("hi"));
    assertThat(message.getRepeatedStringList()).isEmpty();
    assertThat(builder.getRepeatedStringList()).containsExactly("hi");
    assertThat(message.getRepeatedStringList()).isEmpty();
    messageAfterBuild = builder.build();
    builder.clearRepeatedString();
    assertThat(builder.getRepeatedStringList()).isEmpty();
    assertThat(messageAfterBuild.getRepeatedStringList()).containsExactly("hi");

    message = builder.build();
    builder.addAllRepeatedStringPiece(singletonList("hi"));
    assertThat(message.getRepeatedStringPieceList()).isEmpty();
    assertThat(builder.getRepeatedStringPieceList()).containsExactly("hi");
    assertThat(message.getRepeatedStringPieceList()).isEmpty();
    messageAfterBuild = builder.build();
    builder.clearRepeatedStringPiece();
    assertThat(builder.getRepeatedStringPieceList()).isEmpty();
    assertThat(messageAfterBuild.getRepeatedStringPieceList()).containsExactly("hi");

    message = builder.build();
    builder.addAllRepeatedUint32(singletonList(1));
    assertThat(message.getRepeatedUint32List()).isEmpty();
    assertThat(builder.getRepeatedUint32List()).containsExactly(1);
    assertThat(message.getRepeatedUint32List()).isEmpty();
    messageAfterBuild = builder.build();
    builder.clearRepeatedUint32();
    assertThat(builder.getRepeatedUint32List()).isEmpty();
    assertThat(messageAfterBuild.getRepeatedUint32List()).containsExactly(1);

    message = builder.build();
    builder.addAllRepeatedUint64(singletonList(1L));
    assertThat(message.getRepeatedUint64List()).isEmpty();
    assertThat(builder.getRepeatedUint64List()).containsExactly(1L);
    assertThat(message.getRepeatedUint64List()).isEmpty();
    messageAfterBuild = builder.build();
    builder.clearRepeatedUint64();
    assertThat(builder.getRepeatedUint64List()).isEmpty();
    assertThat(messageAfterBuild.getRepeatedUint64List()).containsExactly(1L);

    message = builder.build();
    builder.addRepeatedBool(true);
    assertThat(message.getRepeatedBoolList()).isEmpty();
    assertThat(builder.getRepeatedBoolList()).containsExactly(true);
    assertThat(message.getRepeatedBoolList()).isEmpty();
    messageAfterBuild = builder.build();
    builder.clearRepeatedBool();
    assertThat(builder.getRepeatedBoolList()).isEmpty();
    assertThat(messageAfterBuild.getRepeatedBoolList()).containsExactly(true);

    message = builder.build();
    builder.addRepeatedBytes(ByteString.copyFromUtf8("hi"));
    assertThat(message.getRepeatedBytesList()).isEmpty();
    assertThat(builder.getRepeatedBytesList()).containsExactly(ByteString.copyFromUtf8("hi"));
    assertThat(message.getRepeatedBytesList()).isEmpty();
    messageAfterBuild = builder.build();
    builder.clearRepeatedBytes();
    assertThat(builder.getRepeatedBytesList()).isEmpty();
    assertThat(messageAfterBuild.getRepeatedBytesList())
        .containsExactly(ByteString.copyFromUtf8("hi"));

    message = builder.build();
    builder.addRepeatedCord("hi");
    assertThat(message.getRepeatedCordList()).isEmpty();
    assertThat(builder.getRepeatedCordList()).containsExactly("hi");
    assertThat(message.getRepeatedCordList()).isEmpty();
    messageAfterBuild = builder.build();
    builder.clearRepeatedCord();
    assertThat(builder.getRepeatedCordList()).isEmpty();
    assertThat(messageAfterBuild.getRepeatedCordList()).containsExactly("hi");

    message = builder.build();
    builder.addRepeatedDouble(1D);
    assertThat(message.getRepeatedDoubleList()).isEmpty();
    assertThat(builder.getRepeatedDoubleList()).containsExactly(1D);
    assertThat(message.getRepeatedDoubleList()).isEmpty();
    messageAfterBuild = builder.build();
    builder.clearRepeatedDouble();
    assertThat(builder.getRepeatedDoubleList()).isEmpty();
    assertThat(messageAfterBuild.getRepeatedDoubleList()).containsExactly(1D);

    message = builder.build();
    builder.addRepeatedFixed32(1);
    assertThat(message.getRepeatedFixed32List()).isEmpty();
    assertThat(builder.getRepeatedFixed32List()).containsExactly(1);
    assertThat(message.getRepeatedFixed32List()).isEmpty();
    messageAfterBuild = builder.build();
    builder.clearRepeatedFixed32();
    assertThat(builder.getRepeatedFixed32List()).isEmpty();
    assertThat(messageAfterBuild.getRepeatedFixed32List()).containsExactly(1);

    message = builder.build();
    builder.addRepeatedFixed64(1L);
    assertThat(message.getRepeatedFixed64List()).isEmpty();
    assertThat(builder.getRepeatedFixed64List()).containsExactly(1L);
    assertThat(message.getRepeatedFixed64List()).isEmpty();
    messageAfterBuild = builder.build();
    builder.clearRepeatedFixed64();
    assertThat(builder.getRepeatedFixed64List()).isEmpty();
    assertThat(messageAfterBuild.getRepeatedFixed64List()).containsExactly(1L);

    message = builder.build();
    builder.addRepeatedFloat(1F);
    assertThat(message.getRepeatedFloatList()).isEmpty();
    assertThat(builder.getRepeatedFloatList()).containsExactly(1F);
    assertThat(message.getRepeatedFloatList()).isEmpty();
    messageAfterBuild = builder.build();
    builder.clearRepeatedFloat();
    assertThat(builder.getRepeatedFloatList()).isEmpty();
    assertThat(messageAfterBuild.getRepeatedFloatList()).containsExactly(1F);

    message = builder.build();
    builder.addRepeatedForeignEnum(ForeignEnumLite.FOREIGN_LITE_BAR);
    assertThat(message.getRepeatedForeignEnumList()).isEmpty();
    assertThat(builder.getRepeatedForeignEnumList())
        .containsExactly(ForeignEnumLite.FOREIGN_LITE_BAR);
    assertThat(message.getRepeatedForeignEnumList()).isEmpty();
    messageAfterBuild = builder.build();
    builder.clearRepeatedForeignEnum();
    assertThat(builder.getRepeatedForeignEnumList()).isEmpty();
    assertThat(messageAfterBuild.getRepeatedForeignEnumList())
        .containsExactly(ForeignEnumLite.FOREIGN_LITE_BAR);

    message = builder.build();
    builder.addRepeatedForeignMessage(foreignMessage);
    assertThat(message.getRepeatedForeignMessageList()).isEmpty();
    assertThat(builder.getRepeatedForeignMessageList()).containsExactly(foreignMessage);
    assertThat(message.getRepeatedForeignMessageList()).isEmpty();
    messageAfterBuild = builder.build();
    builder.removeRepeatedForeignMessage(0);
    assertThat(builder.getRepeatedForeignMessageList()).isEmpty();
    assertThat(messageAfterBuild.getRepeatedForeignMessageList()).containsExactly(foreignMessage);

    message = builder.build();
    builder.addRepeatedGroup(RepeatedGroup.getDefaultInstance());
    assertThat(message.getRepeatedGroupList()).isEmpty();
    assertThat(builder.getRepeatedGroupList()).containsExactly(RepeatedGroup.getDefaultInstance());
    assertThat(message.getRepeatedGroupList()).isEmpty();
    messageAfterBuild = builder.build();
    builder.removeRepeatedGroup(0);
    assertThat(builder.getRepeatedGroupList()).isEmpty();
    assertThat(messageAfterBuild.getRepeatedGroupList())
        .containsExactly(RepeatedGroup.getDefaultInstance());

    message = builder.build();
    builder.addRepeatedInt32(1);
    assertThat(message.getRepeatedInt32List()).isEmpty();
    assertThat(builder.getRepeatedInt32List()).containsExactly(1);
    assertThat(message.getRepeatedInt32List()).isEmpty();
    messageAfterBuild = builder.build();
    builder.clearRepeatedInt32();
    assertThat(builder.getRepeatedInt32List()).isEmpty();
    assertThat(messageAfterBuild.getRepeatedInt32List()).containsExactly(1);

    message = builder.build();
    builder.addRepeatedInt64(1L);
    assertThat(message.getRepeatedInt64List()).isEmpty();
    assertThat(builder.getRepeatedInt64List()).containsExactly(1L);
    assertThat(message.getRepeatedInt64List()).isEmpty();
    messageAfterBuild = builder.build();
    builder.clearRepeatedInt64();
    assertThat(builder.getRepeatedInt64List()).isEmpty();
    assertThat(messageAfterBuild.getRepeatedInt64List()).containsExactly(1L);

    message = builder.build();
    builder.addRepeatedLazyMessage(nestedMessage);
    assertThat(message.getRepeatedLazyMessageList()).isEmpty();
    assertThat(builder.getRepeatedLazyMessageList()).containsExactly(nestedMessage);
    assertThat(message.getRepeatedLazyMessageList()).isEmpty();
    messageAfterBuild = builder.build();
    builder.removeRepeatedLazyMessage(0);
    assertThat(builder.getRepeatedLazyMessageList()).isEmpty();
    assertThat(messageAfterBuild.getRepeatedLazyMessageList()).containsExactly(nestedMessage);

    message = builder.build();
    builder.addRepeatedSfixed32(1);
    assertThat(message.getRepeatedSfixed32List()).isEmpty();
    assertThat(builder.getRepeatedSfixed32List()).containsExactly(1);
    assertThat(message.getRepeatedSfixed32List()).isEmpty();
    messageAfterBuild = builder.build();
    builder.clearRepeatedSfixed32();
    assertThat(builder.getRepeatedSfixed32List()).isEmpty();
    assertThat(messageAfterBuild.getRepeatedSfixed32List()).containsExactly(1);

    message = builder.build();
    builder.addRepeatedSfixed64(1L);
    assertThat(message.getRepeatedSfixed64List()).isEmpty();
    assertThat(builder.getRepeatedSfixed64List()).containsExactly(1L);
    assertThat(message.getRepeatedSfixed64List()).isEmpty();
    messageAfterBuild = builder.build();
    builder.clearRepeatedSfixed64();
    assertThat(builder.getRepeatedSfixed64List()).isEmpty();
    assertThat(messageAfterBuild.getRepeatedSfixed64List()).containsExactly(1L);

    message = builder.build();
    builder.addRepeatedSint32(1);
    assertThat(message.getRepeatedSint32List()).isEmpty();
    assertThat(builder.getRepeatedSint32List()).containsExactly(1);
    assertThat(message.getRepeatedSint32List()).isEmpty();
    messageAfterBuild = builder.build();
    builder.clearRepeatedSint32();
    assertThat(builder.getRepeatedSint32List()).isEmpty();
    assertThat(messageAfterBuild.getRepeatedSint32List()).containsExactly(1);

    message = builder.build();
    builder.addRepeatedSint64(1L);
    assertThat(message.getRepeatedSint64List()).isEmpty();
    assertThat(builder.getRepeatedSint64List()).containsExactly(1L);
    assertThat(message.getRepeatedSint64List()).isEmpty();
    messageAfterBuild = builder.build();
    builder.clearRepeatedSint64();
    assertThat(builder.getRepeatedSint64List()).isEmpty();
    assertThat(messageAfterBuild.getRepeatedSint64List()).containsExactly(1L);

    message = builder.build();
    builder.addRepeatedString("hi");
    assertThat(message.getRepeatedStringList()).isEmpty();
    assertThat(builder.getRepeatedStringList()).containsExactly("hi");
    assertThat(message.getRepeatedStringList()).isEmpty();
    messageAfterBuild = builder.build();
    builder.clearRepeatedString();
    assertThat(builder.getRepeatedStringList()).isEmpty();
    assertThat(messageAfterBuild.getRepeatedStringList()).containsExactly("hi");

    message = builder.build();
    builder.addRepeatedStringPiece("hi");
    assertThat(message.getRepeatedStringPieceList()).isEmpty();
    assertThat(builder.getRepeatedStringPieceList()).containsExactly("hi");
    assertThat(message.getRepeatedStringPieceList()).isEmpty();
    messageAfterBuild = builder.build();
    builder.clearRepeatedStringPiece();
    assertThat(builder.getRepeatedStringPieceList()).isEmpty();
    assertThat(messageAfterBuild.getRepeatedStringPieceList()).containsExactly("hi");

    message = builder.build();
    builder.addRepeatedUint32(1);
    assertThat(message.getRepeatedUint32List()).isEmpty();
    assertThat(builder.getRepeatedUint32List()).containsExactly(1);
    assertThat(message.getRepeatedUint32List()).isEmpty();
    messageAfterBuild = builder.build();
    builder.clearRepeatedUint32();
    assertThat(builder.getRepeatedUint32List()).isEmpty();
    assertThat(messageAfterBuild.getRepeatedUint32List()).containsExactly(1);

    message = builder.build();
    builder.addRepeatedUint64(1L);
    assertThat(message.getRepeatedUint64List()).isEmpty();
    assertThat(builder.getRepeatedUint64List()).containsExactly(1L);
    assertThat(message.getRepeatedUint64List()).isEmpty();
    messageAfterBuild = builder.build();
    builder.clearRepeatedUint64();
    assertThat(builder.getRepeatedUint64List()).isEmpty();
    assertThat(messageAfterBuild.getRepeatedUint64List()).containsExactly(1L);

    message = builder.build();
    builder.addRepeatedBool(true);
    messageAfterBuild = builder.build();
    assertThat(message.getRepeatedBoolCount()).isEqualTo(0);
    builder.setRepeatedBool(0, false);
    assertThat(messageAfterBuild.getRepeatedBool(0)).isTrue();
    assertThat(builder.getRepeatedBool(0)).isFalse();
    builder.clearRepeatedBool();

    message = builder.build();
    builder.addRepeatedBytes(ByteString.copyFromUtf8("hi"));
    messageAfterBuild = builder.build();
    assertThat(message.getRepeatedBytesCount()).isEqualTo(0);
    builder.setRepeatedBytes(0, ByteString.EMPTY);
    assertThat(messageAfterBuild.getRepeatedBytes(0)).isEqualTo(ByteString.copyFromUtf8("hi"));
    assertThat(builder.getRepeatedBytes(0)).isEqualTo(ByteString.EMPTY);
    builder.clearRepeatedBytes();

    message = builder.build();
    builder.addRepeatedCord("hi");
    messageAfterBuild = builder.build();
    assertThat(message.getRepeatedCordCount()).isEqualTo(0);
    builder.setRepeatedCord(0, "");
    assertThat(messageAfterBuild.getRepeatedCord(0)).isEqualTo("hi");
    assertThat(builder.getRepeatedCord(0)).isEmpty();
    builder.clearRepeatedCord();
    message = builder.build();

    builder.addRepeatedCordBytes(ByteString.copyFromUtf8("hi"));
    messageAfterBuild = builder.build();
    assertThat(message.getRepeatedCordCount()).isEqualTo(0);
    builder.setRepeatedCord(0, "");
    assertThat(messageAfterBuild.getRepeatedCordBytes(0)).isEqualTo(ByteString.copyFromUtf8("hi"));
    assertThat(builder.getRepeatedCordBytes(0)).isEqualTo(ByteString.EMPTY);
    builder.clearRepeatedCord();

    message = builder.build();
    builder.addRepeatedDouble(1D);
    messageAfterBuild = builder.build();
    assertThat(message.getRepeatedDoubleCount()).isEqualTo(0);
    builder.setRepeatedDouble(0, 0D);
    assertThat(messageAfterBuild.getRepeatedDouble(0)).isEqualTo(1D);
    assertThat(builder.getRepeatedDouble(0)).isEqualTo(0D);
    builder.clearRepeatedDouble();

    message = builder.build();
    builder.addRepeatedFixed32(1);
    messageAfterBuild = builder.build();
    assertThat(message.getRepeatedFixed32Count()).isEqualTo(0);
    builder.setRepeatedFixed32(0, 0);
    assertThat(messageAfterBuild.getRepeatedFixed32(0)).isEqualTo(1);
    assertThat(builder.getRepeatedFixed32(0)).isEqualTo(0);
    builder.clearRepeatedFixed32();

    message = builder.build();
    builder.addRepeatedFixed64(1L);
    messageAfterBuild = builder.build();
    assertThat(message.getRepeatedFixed64Count()).isEqualTo(0);
    builder.setRepeatedFixed64(0, 0L);
    assertThat(messageAfterBuild.getRepeatedFixed64(0)).isEqualTo(1L);
    assertThat(builder.getRepeatedFixed64(0)).isEqualTo(0L);
    builder.clearRepeatedFixed64();

    message = builder.build();
    builder.addRepeatedFloat(1F);
    messageAfterBuild = builder.build();
    assertThat(message.getRepeatedFloatCount()).isEqualTo(0);
    builder.setRepeatedFloat(0, 0F);
    assertThat(messageAfterBuild.getRepeatedFloat(0)).isEqualTo(1F);
    assertThat(builder.getRepeatedFloat(0)).isEqualTo(0F);
    builder.clearRepeatedFloat();

    message = builder.build();
    builder.addRepeatedForeignEnum(ForeignEnumLite.FOREIGN_LITE_BAR);
    messageAfterBuild = builder.build();
    assertThat(message.getRepeatedForeignEnumCount()).isEqualTo(0);
    builder.setRepeatedForeignEnum(0, ForeignEnumLite.FOREIGN_LITE_FOO);
    assertThat(messageAfterBuild.getRepeatedForeignEnum(0))
        .isEqualTo(ForeignEnumLite.FOREIGN_LITE_BAR);
    assertThat(builder.getRepeatedForeignEnum(0)).isEqualTo(ForeignEnumLite.FOREIGN_LITE_FOO);
    builder.clearRepeatedForeignEnum();

    message = builder.build();
    builder.addRepeatedForeignMessage(foreignMessage);
    messageAfterBuild = builder.build();
    assertThat(message.getRepeatedForeignMessageCount()).isEqualTo(0);
    builder.setRepeatedForeignMessage(0, ForeignMessageLite.getDefaultInstance());
    assertThat(messageAfterBuild.getRepeatedForeignMessage(0)).isEqualTo(foreignMessage);
    assertThat(builder.getRepeatedForeignMessage(0))
        .isEqualTo(ForeignMessageLite.getDefaultInstance());
    builder.clearRepeatedForeignMessage();

    message = builder.build();
    builder.addRepeatedForeignMessage(foreignMessageC3);
    messageAfterBuild = builder.build();
    assertThat(message.getRepeatedForeignMessageCount()).isEqualTo(0);
    builder.setRepeatedForeignMessage(0, ForeignMessageLite.getDefaultInstance());
    assertThat(messageAfterBuild.getRepeatedForeignMessage(0)).isEqualTo(foreignMessageC3);
    assertThat(builder.getRepeatedForeignMessage(0))
        .isEqualTo(ForeignMessageLite.getDefaultInstance());
    builder.clearRepeatedForeignMessage();

    message = builder.build();
    builder.addRepeatedForeignMessage(0, foreignMessage);
    messageAfterBuild = builder.build();
    assertThat(message.getRepeatedForeignMessageCount()).isEqualTo(0);
    builder.setRepeatedForeignMessage(0, foreignMessageC3);
    assertThat(messageAfterBuild.getRepeatedForeignMessage(0)).isEqualTo(foreignMessage);
    assertThat(builder.getRepeatedForeignMessage(0)).isEqualTo(foreignMessageC3);
    builder.clearRepeatedForeignMessage();

    message = builder.build();
    RepeatedGroup repeatedGroup = RepeatedGroup.newBuilder().setA(1).build();
    builder.addRepeatedGroup(repeatedGroup);
    messageAfterBuild = builder.build();
    assertThat(message.getRepeatedGroupCount()).isEqualTo(0);
    builder.setRepeatedGroup(0, RepeatedGroup.getDefaultInstance());
    assertThat(messageAfterBuild.getRepeatedGroup(0)).isEqualTo(repeatedGroup);
    assertThat(builder.getRepeatedGroup(0)).isEqualTo(RepeatedGroup.getDefaultInstance());
    builder.clearRepeatedGroup();

    message = builder.build();
    builder.addRepeatedGroup(0, repeatedGroup);
    messageAfterBuild = builder.build();
    assertThat(message.getRepeatedGroupCount()).isEqualTo(0);
    builder.setRepeatedGroup(0, RepeatedGroup.getDefaultInstance());
    assertThat(messageAfterBuild.getRepeatedGroup(0)).isEqualTo(repeatedGroup);
    assertThat(builder.getRepeatedGroup(0)).isEqualTo(RepeatedGroup.getDefaultInstance());
    builder.clearRepeatedGroup();

    message = builder.build();
    RepeatedGroup.Builder repeatedGroupBuilder = RepeatedGroup.newBuilder().setA(3);
    builder.addRepeatedGroup(repeatedGroupBuilder);
    messageAfterBuild = builder.build();
    assertThat(message.getRepeatedGroupCount()).isEqualTo(0);
    builder.setRepeatedGroup(0, RepeatedGroup.getDefaultInstance());
    assertThat(messageAfterBuild.getRepeatedGroup(0)).isEqualTo(repeatedGroupBuilder.build());
    assertThat(builder.getRepeatedGroup(0)).isEqualTo(RepeatedGroup.getDefaultInstance());
    builder.clearRepeatedGroup();

    message = builder.build();
    builder.addRepeatedGroup(0, repeatedGroupBuilder);
    messageAfterBuild = builder.build();
    assertThat(message.getRepeatedGroupCount()).isEqualTo(0);
    builder.setRepeatedGroup(0, RepeatedGroup.getDefaultInstance());
    assertThat(messageAfterBuild.getRepeatedGroup(0)).isEqualTo(repeatedGroupBuilder.build());
    assertThat(builder.getRepeatedGroup(0)).isEqualTo(RepeatedGroup.getDefaultInstance());
    builder.clearRepeatedGroup();

    message = builder.build();
    builder.addRepeatedInt32(1);
    messageAfterBuild = builder.build();
    assertThat(message.getRepeatedInt32Count()).isEqualTo(0);
    builder.setRepeatedInt32(0, 0);
    assertThat(messageAfterBuild.getRepeatedInt32(0)).isEqualTo(1);
    assertThat(builder.getRepeatedInt32(0)).isEqualTo(0);
    builder.clearRepeatedInt32();

    message = builder.build();
    builder.addRepeatedInt64(1L);
    messageAfterBuild = builder.build();
    assertThat(message.getRepeatedInt64Count()).isEqualTo(0L);
    builder.setRepeatedInt64(0, 0L);
    assertThat(messageAfterBuild.getRepeatedInt64(0)).isEqualTo(1L);
    assertThat(builder.getRepeatedInt64(0)).isEqualTo(0L);
    builder.clearRepeatedInt64();

    message = builder.build();
    builder.addRepeatedLazyMessage(nestedMessage);
    messageAfterBuild = builder.build();
    assertThat(message.getRepeatedLazyMessageCount()).isEqualTo(0);
    builder.setRepeatedLazyMessage(0, NestedMessage.getDefaultInstance());
    assertThat(messageAfterBuild.getRepeatedLazyMessage(0)).isEqualTo(nestedMessage);
    assertThat(builder.getRepeatedLazyMessage(0)).isEqualTo(NestedMessage.getDefaultInstance());
    builder.clearRepeatedLazyMessage();

    message = builder.build();
    builder.addRepeatedLazyMessage(0, nestedMessage);
    messageAfterBuild = builder.build();
    assertThat(message.getRepeatedLazyMessageCount()).isEqualTo(0);
    builder.setRepeatedLazyMessage(0, NestedMessage.getDefaultInstance());
    assertThat(messageAfterBuild.getRepeatedLazyMessage(0)).isEqualTo(nestedMessage);
    assertThat(builder.getRepeatedLazyMessage(0)).isEqualTo(NestedMessage.getDefaultInstance());
    builder.clearRepeatedLazyMessage();

    message = builder.build();
    builder.addRepeatedLazyMessage(nestedMessageBuilder);
    messageAfterBuild = builder.build();
    assertThat(message.getRepeatedLazyMessageCount()).isEqualTo(0);
    builder.setRepeatedLazyMessage(0, NestedMessage.getDefaultInstance());
    assertThat(messageAfterBuild.getRepeatedLazyMessage(0)).isEqualTo(nestedMessageBuilder.build());
    assertThat(builder.getRepeatedLazyMessage(0)).isEqualTo(NestedMessage.getDefaultInstance());
    builder.clearRepeatedLazyMessage();

    message = builder.build();
    builder.addRepeatedLazyMessage(0, nestedMessageBuilder);
    messageAfterBuild = builder.build();
    assertThat(message.getRepeatedLazyMessageCount()).isEqualTo(0);
    builder.setRepeatedLazyMessage(0, NestedMessage.getDefaultInstance());
    assertThat(messageAfterBuild.getRepeatedLazyMessage(0)).isEqualTo(nestedMessageBuilder.build());
    assertThat(builder.getRepeatedLazyMessage(0)).isEqualTo(NestedMessage.getDefaultInstance());
    builder.clearRepeatedLazyMessage();

    message = builder.build();
    builder.addRepeatedSfixed32(1);
    messageAfterBuild = builder.build();
    assertThat(message.getRepeatedSfixed32Count()).isEqualTo(0);
    builder.setRepeatedSfixed32(0, 0);
    assertThat(messageAfterBuild.getRepeatedSfixed32(0)).isEqualTo(1);
    assertThat(builder.getRepeatedSfixed32(0)).isEqualTo(0);
    builder.clearRepeatedSfixed32();

    message = builder.build();
    builder.addRepeatedSfixed64(1L);
    messageAfterBuild = builder.build();
    assertThat(message.getRepeatedSfixed64Count()).isEqualTo(0L);
    builder.setRepeatedSfixed64(0, 0L);
    assertThat(messageAfterBuild.getRepeatedSfixed64(0)).isEqualTo(1L);
    assertThat(builder.getRepeatedSfixed64(0)).isEqualTo(0L);
    builder.clearRepeatedSfixed64();

    message = builder.build();
    builder.addRepeatedSint32(1);
    messageAfterBuild = builder.build();
    assertThat(message.getRepeatedSint32Count()).isEqualTo(0);
    builder.setRepeatedSint32(0, 0);
    assertThat(messageAfterBuild.getRepeatedSint32(0)).isEqualTo(1);
    assertThat(builder.getRepeatedSint32(0)).isEqualTo(0);
    builder.clearRepeatedSint32();

    message = builder.build();
    builder.addRepeatedSint64(1L);
    messageAfterBuild = builder.build();
    assertThat(message.getRepeatedSint64Count()).isEqualTo(0L);
    builder.setRepeatedSint64(0, 0L);
    assertThat(messageAfterBuild.getRepeatedSint64(0)).isEqualTo(1L);
    assertThat(builder.getRepeatedSint64(0)).isEqualTo(0L);
    builder.clearRepeatedSint64();

    message = builder.build();
    builder.addRepeatedString("hi");
    messageAfterBuild = builder.build();
    assertThat(message.getRepeatedStringCount()).isEqualTo(0L);
    builder.setRepeatedString(0, "");
    assertThat(messageAfterBuild.getRepeatedString(0)).isEqualTo("hi");
    assertThat(builder.getRepeatedString(0)).isEmpty();
    builder.clearRepeatedString();

    message = builder.build();
    builder.addRepeatedStringBytes(ByteString.copyFromUtf8("hi"));
    messageAfterBuild = builder.build();
    assertThat(message.getRepeatedStringCount()).isEqualTo(0L);
    builder.setRepeatedString(0, "");
    assertThat(messageAfterBuild.getRepeatedStringBytes(0))
        .isEqualTo(ByteString.copyFromUtf8("hi"));
    assertThat(builder.getRepeatedStringBytes(0)).isEqualTo(ByteString.EMPTY);
    builder.clearRepeatedString();

    message = builder.build();
    builder.addRepeatedStringPiece("hi");
    messageAfterBuild = builder.build();
    assertThat(message.getRepeatedStringPieceCount()).isEqualTo(0L);
    builder.setRepeatedStringPiece(0, "");
    assertThat(messageAfterBuild.getRepeatedStringPiece(0)).isEqualTo("hi");
    assertThat(builder.getRepeatedStringPiece(0)).isEmpty();
    builder.clearRepeatedStringPiece();

    message = builder.build();
    builder.addRepeatedStringPieceBytes(ByteString.copyFromUtf8("hi"));
    messageAfterBuild = builder.build();
    assertThat(message.getRepeatedStringPieceCount()).isEqualTo(0L);
    builder.setRepeatedStringPiece(0, "");
    assertThat(messageAfterBuild.getRepeatedStringPieceBytes(0))
        .isEqualTo(ByteString.copyFromUtf8("hi"));
    assertThat(builder.getRepeatedStringPieceBytes(0)).isEqualTo(ByteString.EMPTY);
    builder.clearRepeatedStringPiece();

    message = builder.build();
    builder.addRepeatedUint32(1);
    messageAfterBuild = builder.build();
    assertThat(message.getRepeatedUint32Count()).isEqualTo(0);
    builder.setRepeatedUint32(0, 0);
    assertThat(messageAfterBuild.getRepeatedUint32(0)).isEqualTo(1);
    assertThat(builder.getRepeatedUint32(0)).isEqualTo(0);
    builder.clearRepeatedUint32();

    message = builder.build();
    builder.addRepeatedUint64(1L);
    messageAfterBuild = builder.build();
    assertThat(message.getRepeatedUint64Count()).isEqualTo(0L);
    builder.setRepeatedUint64(0, 0L);
    assertThat(messageAfterBuild.getRepeatedUint64(0)).isEqualTo(1L);
    assertThat(builder.getRepeatedUint64(0)).isEqualTo(0L);
    builder.clearRepeatedUint64();

    message = builder.build();
    assertThat(message.getSerializedSize()).isEqualTo(0);
    builder.mergeFrom(TestAllTypesLite.newBuilder().setOptionalBool(true).build());
    assertThat(message.getSerializedSize()).isEqualTo(0);
    assertThat(builder.build().getOptionalBool()).isTrue();
    builder.clearOptionalBool();

    message = builder.build();
    assertThat(message.getSerializedSize()).isEqualTo(0);
    builder.mergeFrom(TestAllTypesLite.newBuilder().setOptionalBool(true).build());
    assertThat(message.getSerializedSize()).isEqualTo(0);
    assertThat(builder.build().getOptionalBool()).isTrue();
    builder.clear();
    assertThat(builder.build().getSerializedSize()).isEqualTo(0);

    message = builder.build();
    assertThat(message.getSerializedSize()).isEqualTo(0);
    builder.mergeOptionalForeignMessage(foreignMessage);
    assertThat(message.getSerializedSize()).isEqualTo(0);
    assertThat(builder.build().getOptionalForeignMessage().getC()).isEqualTo(foreignMessage.getC());
    builder.clearOptionalForeignMessage();

    message = builder.build();
    assertThat(message.getSerializedSize()).isEqualTo(0);
    builder.mergeOptionalLazyMessage(nestedMessage);
    assertThat(message.getSerializedSize()).isEqualTo(0);
    assertThat(builder.build().getOptionalLazyMessage().getBb()).isEqualTo(nestedMessage.getBb());
    builder.clearOptionalLazyMessage();

    message = builder.build();
    builder.setOneofString("hi");
    assertThat(message.getOneofFieldCase()).isEqualTo(OneofFieldCase.ONEOFFIELD_NOT_SET);
    assertThat(builder.getOneofFieldCase()).isEqualTo(OneofFieldCase.ONEOF_STRING);
    assertThat(builder.getOneofString()).isEqualTo("hi");
    messageAfterBuild = builder.build();
    assertThat(messageAfterBuild.getOneofFieldCase()).isEqualTo(OneofFieldCase.ONEOF_STRING);
    assertThat(messageAfterBuild.getOneofString()).isEqualTo("hi");
    builder.setOneofUint32(1);
    assertThat(messageAfterBuild.getOneofFieldCase()).isEqualTo(OneofFieldCase.ONEOF_STRING);
    assertThat(messageAfterBuild.getOneofString()).isEqualTo("hi");
    assertThat(builder.getOneofFieldCase()).isEqualTo(OneofFieldCase.ONEOF_UINT32);
    assertThat(builder.getOneofUint32()).isEqualTo(1);
    TestAllTypesLiteOrBuilder messageOrBuilder = builder;
    assertThat(messageOrBuilder.getOneofFieldCase()).isEqualTo(OneofFieldCase.ONEOF_UINT32);

    TestAllExtensionsLite.Builder extendableMessageBuilder = TestAllExtensionsLite.newBuilder();
    TestAllExtensionsLite extendableMessage = extendableMessageBuilder.build();
    extendableMessageBuilder.setExtension(UnittestLite.optionalInt32ExtensionLite, 1);
    assertThat(extendableMessage.hasExtension(UnittestLite.optionalInt32ExtensionLite)).isFalse();
    extendableMessage = extendableMessageBuilder.build();
    assertThat((int) extendableMessageBuilder.getExtension(UnittestLite.optionalInt32ExtensionLite))
        .isEqualTo(1);
    assertThat((int) extendableMessage.getExtension(UnittestLite.optionalInt32ExtensionLite))
        .isEqualTo(1);
    extendableMessageBuilder.setExtension(UnittestLite.optionalInt32ExtensionLite, 3);
    assertThat((int) extendableMessageBuilder.getExtension(UnittestLite.optionalInt32ExtensionLite))
        .isEqualTo(3);
    assertThat((int) extendableMessage.getExtension(UnittestLite.optionalInt32ExtensionLite))
        .isEqualTo(1);
    extendableMessage = extendableMessageBuilder.build();
    assertThat((int) extendableMessageBuilder.getExtension(UnittestLite.optionalInt32ExtensionLite))
        .isEqualTo(3);
    assertThat((int) extendableMessage.getExtension(UnittestLite.optionalInt32ExtensionLite))
        .isEqualTo(3);

    // No extension registry, so it should be in unknown fields.
    extendableMessage = TestAllExtensionsLite.parseFrom(extendableMessage.toByteArray());
    assertThat(extendableMessage.hasExtension(UnittestLite.optionalInt32ExtensionLite)).isFalse();

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
    assertThat((int) extendableMessage.getExtension(UnittestLite.optionalInt32ExtensionLite))
        .isEqualTo(3);
    assertThat((int) extendableMessage.getExtension(UnittestLite.optionalFixed32ExtensionLite))
        .isEqualTo(11);
  }

  @Test
  @SuppressWarnings("ProtoNewBuilderMergeFrom")
  public void testBuilderMergeFromNull() throws Exception {
    try {
      TestAllTypesLite.newBuilder().mergeFrom((TestAllTypesLite) null);
      assertWithMessage("expected exception").fail();
    } catch (NullPointerException e) {
      // Pass.
    }
  }

  // Builder.mergeFrom() should keep existing extensions.
  @Test
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
    assertThat(result.getExtensionCount(UnittestLite.repeatedInt32ExtensionLite)).isEqualTo(2);
    assertThat(result.getExtension(UnittestLite.repeatedInt32ExtensionLite, 0).intValue())
        .isEqualTo(12);
    assertThat(result.getExtension(UnittestLite.repeatedInt32ExtensionLite, 1).intValue())
        .isEqualTo(12);
  }

  // Builder.mergeFrom() should keep existing unknown fields.
  @Test
  public void testBuilderMergeFromWithUnknownFields() throws Exception {
    TestAllTypesLite message = TestAllTypesLite.newBuilder().addRepeatedInt32(1).build();

    NestedMessage.Builder builder = NestedMessage.newBuilder();
    builder.mergeFrom(message.toByteArray());
    builder.mergeFrom(message.toByteArray());
    NestedMessage result = builder.build();
    assertThat(result.getSerializedSize()).isEqualTo(message.getSerializedSize() * 2);
  }

  @Test
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

  @Test
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

  @Test
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

  @Test
  public void testMergeFrom_failureWhenReadingValue_propagatesOriginalException() {
    final byte[] bytes = TestOneof2.newBuilder().setFooInt(123).build().toByteArray();
    final IOException injectedException = new IOException("oh no");
    CodedInputStream failingInputStream =
        CodedInputStream.newInstance(
            new InputStream() {
              boolean first = true;

              @Override
              public int read(byte[] b, int off, int len) throws IOException {
                if (!first) {
                  throw injectedException;
                }
                first = false;
                System.arraycopy(bytes, 0, b, off, len);
                return len;
              }

              @Override
              public int read() {
                throw new UnsupportedOperationException();
              }
            },
            bytes.length - 1);
    TestOneof2.Builder builder = TestOneof2.newBuilder();

    try {
      builder.mergeFrom(failingInputStream, ExtensionRegistryLite.getEmptyRegistry());
      assertWithMessage("Expected mergeFrom to fail").fail();
    } catch (IOException e) {
      assertThat(e).isSameInstanceAs(injectedException);
    }
  }

  @Test
  public void testToStringDefaultInstance() throws Exception {
    assertToStringEquals("", TestAllTypesLite.getDefaultInstance());
  }

  @Test
  public void testToStringScalarFieldsSuffixedWithList() throws Exception {
    assertToStringEquals(
        "deceptively_named_list: 7",
        TestAllTypesLite.newBuilder().setDeceptivelyNamedList(7).build());
  }

  @Test
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

  @Test
  public void testToStringStringFields() throws Exception {
    TestAllTypesLite proto =
        TestAllTypesLite.newBuilder().setOptionalString("foo\"bar\nbaz\\").build();
    assertToStringEquals("optional_string: \"foo\\\"bar\\nbaz\\\\\"", proto);

    proto = TestAllTypesLite.newBuilder().setOptionalString("\u6587").build();
    assertToStringEquals("optional_string: \"\\346\\226\\207\"", proto);
  }

  @Test
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

  @Test
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

  @Test
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

  @Test
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

  @Test
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

  @Test
  public void testToStringLazyMessage() throws Exception {
    TestAllTypesLite message =
        TestAllTypesLite.newBuilder()
            .setOptionalLazyMessage(NestedMessage.newBuilder().setBb(1).build())
            .build();
    assertToStringEquals("optional_lazy_message {\n  bb: 1\n}", message);
  }

  @Test
  public void testToStringGroup() throws Exception {
    TestAllTypesLite message =
        TestAllTypesLite.newBuilder()
            .setOptionalGroup(OptionalGroup.newBuilder().setA(1).build())
            .build();
    assertToStringEquals("optional_group {\n  a: 1\n}", message);
  }

  @Test
  public void testToStringOneof() throws Exception {
    TestAllTypesLite message = TestAllTypesLite.newBuilder().setOneofString("hello").build();
    assertToStringEquals("oneof_string: \"hello\"", message);
  }

  @Test
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
    assertThat(toString.charAt(0)).isEqualTo('#');
    if (toString.contains("\n")) {
      toString = toString.substring(toString.indexOf("\n") + 1);
    } else {
      toString = "";
    }
    assertThat(toString).isEqualTo(expected);
  }

  @Test
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

    assertThat(message.getOptionalLazyMessage().getBb()).isEqualTo(11);
    assertThat(message.getOptionalLazyMessage().getCc()).isEqualTo(22L);
  }

  @Test
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

    assertThat(message.getOneofLazyNestedMessage().getBb()).isEqualTo(11);
    assertThat(message.getOneofLazyNestedMessage().getCc()).isEqualTo(22L);
  }

  @Test
  public void testMergeFromStream_repeatedField() throws Exception {
    TestAllTypesLite.Builder builder = TestAllTypesLite.newBuilder().addRepeatedString("hello");
    builder.mergeFrom(CodedInputStream.newInstance(builder.build().toByteArray()));

    assertThat(builder.getRepeatedStringCount()).isEqualTo(2);
  }

  @Test
  public void testMergeFromStream_invalidBytes() throws Exception {
    TestAllTypesLite.Builder builder = TestAllTypesLite.newBuilder().setDefaultBool(true);
    try {
      builder.mergeFrom(CodedInputStream.newInstance("Invalid bytes".getBytes(Internal.UTF_8)));
      assertWithMessage("expected exception").fail();
    } catch (InvalidProtocolBufferException expected) {
    }
  }

  @Test
  public void testParseFromStream_IOExceptionNotLost() throws Exception {
    final IOException readException = new IOException();
    try {
      TestAllTypesLite.parseFrom(
          CodedInputStream.newInstance(
              new InputStream() {
                @Override
                public int read() throws IOException {
                  throw readException;
                }
              }));
      assertWithMessage("expected exception").fail();
    } catch (InvalidProtocolBufferException expected) {
      boolean found = false;
      for (Throwable exception = expected; exception != null; exception = exception.getCause()) {
        if (exception == readException) {
          found = true;
          break;
        }
      }
      if (!found) {
        throw new AssertionError("Lost cause of parsing error", expected);
      }
    }
  }

  @Test
  public void testParseDelimitedFromStream_IOExceptionNotLost() throws Exception {
    final IOException readException = new IOException();
    try {
      TestAllTypesLite.parseDelimitedFrom(
          new InputStream() {
            @Override
            public int read() throws IOException {
              throw readException;
            }
          });
      assertWithMessage("expected exception").fail();
    } catch (InvalidProtocolBufferException expected) {
      boolean found = false;
      for (Throwable exception = expected; exception != null; exception = exception.getCause()) {
        if (exception == readException) {
          found = true;
          break;
        }
      }
      if (!found) {
        throw new AssertionError("Lost cause of parsing error", expected);
      }
    }
  }

  @Test
  public void testParseFromArray_manyNestedMessagesError() throws Exception {
    RecursiveMessage.Builder recursiveMessage =
        RecursiveMessage.newBuilder().setPayload(ByteString.copyFrom(new byte[1]));
    for (int i = 0; i < 20; i++) {
      recursiveMessage = RecursiveMessage.newBuilder().setRecurse(recursiveMessage.build());
    }
    byte[] result = recursiveMessage.build().toByteArray();
    result[
            result.length
                - CodedOutputStream.computeTagSize(RecursiveMessage.PAYLOAD_FIELD_NUMBER)
                - CodedOutputStream.computeLengthDelimitedFieldSize(1)] =
        0; // Set invalid tag
    try {
      RecursiveMessage.parseFrom(result);
      assertWithMessage("Result was: " + Arrays.toString(result)).fail();
    } catch (InvalidProtocolBufferException expected) {
      boolean found = false;
      int exceptionCount = 0;
      for (Throwable exception = expected; exception != null; exception = exception.getCause()) {
        if (exception instanceof InvalidProtocolBufferException) {
          exceptionCount++;
        }
        for (StackTraceElement element : exception.getStackTrace()) {
          if (InvalidProtocolBufferException.class.getName().equals(element.getClassName())
              && "invalidTag".equals(element.getMethodName())) {
            found = true;
          } else if (Android.isOnAndroidDevice()
              && "decodeUnknownField".equals(element.getMethodName())) {
            // Android is missing the first element of the stack trace - b/181147885
            found = true;
          }
        }
      }
      if (!found) {
        throw new AssertionError("Lost cause of parsing error", expected);
      }
      if (exceptionCount > 1) {
        throw new AssertionError(exceptionCount + " nested parsing exceptions", expected);
      }
    }
  }

  @Test
  public void testParseFromStream_manyNestedMessagesError() throws Exception {
    RecursiveMessage.Builder recursiveMessage =
        RecursiveMessage.newBuilder().setPayload(ByteString.copyFrom(new byte[1]));
    for (int i = 0; i < 20; i++) {
      recursiveMessage = RecursiveMessage.newBuilder().setRecurse(recursiveMessage.build());
    }
    byte[] result = recursiveMessage.build().toByteArray();
    result[
            result.length
                - CodedOutputStream.computeTagSize(RecursiveMessage.PAYLOAD_FIELD_NUMBER)
                - CodedOutputStream.computeLengthDelimitedFieldSize(1)] =
        0; // Set invalid tag
    try {
      RecursiveMessage.parseFrom(CodedInputStream.newInstance(new ByteArrayInputStream(result)));
      assertWithMessage("Result was: " + Arrays.toString(result)).fail();
    } catch (InvalidProtocolBufferException expected) {
      boolean found = false;
      int exceptionCount = 0;
      for (Throwable exception = expected; exception != null; exception = exception.getCause()) {
        if (exception instanceof InvalidProtocolBufferException) {
          exceptionCount++;
        }
        for (StackTraceElement element : exception.getStackTrace()) {
          if (InvalidProtocolBufferException.class.getName().equals(element.getClassName())
              && "invalidTag".equals(element.getMethodName())) {
            found = true;
          } else if (Android.isOnAndroidDevice() && "readTag".equals(element.getMethodName())) {
            // Android is missing the first element of the stack trace - b/181147885
            found = true;
          }
        }
      }
      if (!found) {
        throw new AssertionError("Lost cause of parsing error", expected);
      }
      if (exceptionCount > 1) {
        throw new AssertionError(exceptionCount + " nested parsing exceptions", expected);
      }
    }
  }

  @Test
  public void testParseFromStream_sneakyNestedException() throws Exception {
    final InvalidProtocolBufferException sketchy =
        new InvalidProtocolBufferException("Created in a sketchy way!")
            .setUnfinishedMessage(TestAllTypesLite.getDefaultInstance());
    try {
      RecursiveMessage.parseFrom(
          CodedInputStream.newInstance(
              new InputStream() {
                @Override
                public int read() throws IOException {
                  throw sketchy;
                }
              }));
      assertWithMessage("expected exception").fail();
    } catch (InvalidProtocolBufferException expected) {
      assertThat(expected).isNotSameInstanceAs(sketchy);
    }
    assertThat(sketchy.getUnfinishedMessage()).isEqualTo(TestAllTypesLite.getDefaultInstance());
  }

  @Test
  public void testMergeFrom_sanity() throws Exception {
    TestAllTypesLite one = TestUtilLite.getAllLiteSetBuilder().build();
    byte[] bytes = one.toByteArray();
    TestAllTypesLite two = TestAllTypesLite.parseFrom(bytes);

    one = one.toBuilder().mergeFrom(one).build();
    two = two.toBuilder().mergeFrom(bytes).build();
    assertThat(one).isEqualTo(two);
    assertThat(two).isEqualTo(one);
    assertThat(one.hashCode()).isEqualTo(two.hashCode());
  }

  @Test
  @SuppressWarnings("ProtoNewBuilderMergeFrom")
  public void testMergeFromNoLazyFieldSharing() throws Exception {
    TestAllTypesLite.Builder sourceBuilder =
        TestAllTypesLite.newBuilder().setOptionalLazyMessage(NestedMessage.newBuilder().setBb(1));
    TestAllTypesLite.Builder targetBuilder =
        TestAllTypesLite.newBuilder().mergeFrom(sourceBuilder.build());
    assertThat(sourceBuilder.getOptionalLazyMessage().getBb()).isEqualTo(1);
    // now change the sourceBuilder, and target value shouldn't be affected.
    sourceBuilder.setOptionalLazyMessage(NestedMessage.newBuilder().setBb(2));
    assertThat(targetBuilder.getOptionalLazyMessage().getBb()).isEqualTo(1);
  }

  @Test
  public void testEquals_notEqual() throws Exception {
    TestAllTypesLite one = TestUtilLite.getAllLiteSetBuilder().build();
    byte[] bytes = one.toByteArray();
    TestAllTypesLite two = one.toBuilder().mergeFrom(one).mergeFrom(bytes).build();

    assertThat(one.equals(two)).isFalse();
    assertThat(two.equals(one)).isFalse();

    assertThat(one.equals(TestAllTypesLite.getDefaultInstance())).isFalse();
    assertThat(TestAllTypesLite.getDefaultInstance().equals(one)).isFalse();

    TestAllTypesLite oneFieldSet = TestAllTypesLite.newBuilder().setDefaultBool(true).build();
    assertThat(oneFieldSet.equals(TestAllTypesLite.getDefaultInstance())).isFalse();
    assertThat(TestAllTypesLite.getDefaultInstance().equals(oneFieldSet)).isFalse();

    oneFieldSet = TestAllTypesLite.newBuilder().setDefaultBytes(ByteString.EMPTY).build();
    assertThat(oneFieldSet.equals(TestAllTypesLite.getDefaultInstance())).isFalse();
    assertThat(TestAllTypesLite.getDefaultInstance().equals(oneFieldSet)).isFalse();

    oneFieldSet = TestAllTypesLite.newBuilder().setDefaultCord("").build();
    assertThat(oneFieldSet.equals(TestAllTypesLite.getDefaultInstance())).isFalse();
    assertThat(TestAllTypesLite.getDefaultInstance().equals(oneFieldSet)).isFalse();

    oneFieldSet = TestAllTypesLite.newBuilder().setDefaultCordBytes(ByteString.EMPTY).build();
    assertThat(oneFieldSet.equals(TestAllTypesLite.getDefaultInstance())).isFalse();
    assertThat(TestAllTypesLite.getDefaultInstance().equals(oneFieldSet)).isFalse();

    oneFieldSet = TestAllTypesLite.newBuilder().setDefaultDouble(0).build();
    assertThat(oneFieldSet.equals(TestAllTypesLite.getDefaultInstance())).isFalse();
    assertThat(TestAllTypesLite.getDefaultInstance().equals(oneFieldSet)).isFalse();

    oneFieldSet = TestAllTypesLite.newBuilder().setDefaultFixed32(0).build();
    assertThat(oneFieldSet.equals(TestAllTypesLite.getDefaultInstance())).isFalse();
    assertThat(TestAllTypesLite.getDefaultInstance().equals(oneFieldSet)).isFalse();

    oneFieldSet = TestAllTypesLite.newBuilder().setDefaultFixed64(0).build();
    assertThat(oneFieldSet.equals(TestAllTypesLite.getDefaultInstance())).isFalse();
    assertThat(TestAllTypesLite.getDefaultInstance().equals(oneFieldSet)).isFalse();

    oneFieldSet = TestAllTypesLite.newBuilder().setDefaultFloat(0).build();
    assertThat(oneFieldSet.equals(TestAllTypesLite.getDefaultInstance())).isFalse();
    assertThat(TestAllTypesLite.getDefaultInstance().equals(oneFieldSet)).isFalse();

    oneFieldSet =
        TestAllTypesLite.newBuilder()
            .setDefaultForeignEnum(ForeignEnumLite.FOREIGN_LITE_BAR)
            .build();
    assertThat(oneFieldSet.equals(TestAllTypesLite.getDefaultInstance())).isFalse();
    assertThat(TestAllTypesLite.getDefaultInstance().equals(oneFieldSet)).isFalse();

    oneFieldSet =
        TestAllTypesLite.newBuilder().setDefaultImportEnum(ImportEnumLite.IMPORT_LITE_BAR).build();
    assertThat(oneFieldSet.equals(TestAllTypesLite.getDefaultInstance())).isFalse();
    assertThat(TestAllTypesLite.getDefaultInstance().equals(oneFieldSet)).isFalse();

    oneFieldSet = TestAllTypesLite.newBuilder().setDefaultInt32(0).build();
    assertThat(oneFieldSet.equals(TestAllTypesLite.getDefaultInstance())).isFalse();
    assertThat(TestAllTypesLite.getDefaultInstance().equals(oneFieldSet)).isFalse();

    oneFieldSet = TestAllTypesLite.newBuilder().setDefaultInt64(0).build();
    assertThat(oneFieldSet.equals(TestAllTypesLite.getDefaultInstance())).isFalse();
    assertThat(TestAllTypesLite.getDefaultInstance().equals(oneFieldSet)).isFalse();

    oneFieldSet = TestAllTypesLite.newBuilder().setDefaultNestedEnum(NestedEnum.BAR).build();
    assertThat(oneFieldSet.equals(TestAllTypesLite.getDefaultInstance())).isFalse();
    assertThat(TestAllTypesLite.getDefaultInstance().equals(oneFieldSet)).isFalse();

    oneFieldSet = TestAllTypesLite.newBuilder().setDefaultSfixed32(0).build();
    assertThat(oneFieldSet.equals(TestAllTypesLite.getDefaultInstance())).isFalse();
    assertThat(TestAllTypesLite.getDefaultInstance().equals(oneFieldSet)).isFalse();

    oneFieldSet = TestAllTypesLite.newBuilder().setDefaultSfixed64(0).build();
    assertThat(oneFieldSet.equals(TestAllTypesLite.getDefaultInstance())).isFalse();
    assertThat(TestAllTypesLite.getDefaultInstance().equals(oneFieldSet)).isFalse();

    oneFieldSet = TestAllTypesLite.newBuilder().setDefaultSint32(0).build();
    assertThat(oneFieldSet.equals(TestAllTypesLite.getDefaultInstance())).isFalse();
    assertThat(TestAllTypesLite.getDefaultInstance().equals(oneFieldSet)).isFalse();

    oneFieldSet = TestAllTypesLite.newBuilder().setDefaultSint64(0).build();
    assertThat(oneFieldSet.equals(TestAllTypesLite.getDefaultInstance())).isFalse();
    assertThat(TestAllTypesLite.getDefaultInstance().equals(oneFieldSet)).isFalse();

    oneFieldSet = TestAllTypesLite.newBuilder().setDefaultString("").build();
    assertThat(oneFieldSet.equals(TestAllTypesLite.getDefaultInstance())).isFalse();
    assertThat(TestAllTypesLite.getDefaultInstance().equals(oneFieldSet)).isFalse();

    oneFieldSet = TestAllTypesLite.newBuilder().setDefaultStringBytes(ByteString.EMPTY).build();
    assertThat(oneFieldSet.equals(TestAllTypesLite.getDefaultInstance())).isFalse();
    assertThat(TestAllTypesLite.getDefaultInstance().equals(oneFieldSet)).isFalse();

    oneFieldSet = TestAllTypesLite.newBuilder().setDefaultStringPiece("").build();
    assertThat(oneFieldSet.equals(TestAllTypesLite.getDefaultInstance())).isFalse();
    assertThat(TestAllTypesLite.getDefaultInstance().equals(oneFieldSet)).isFalse();

    oneFieldSet =
        TestAllTypesLite.newBuilder().setDefaultStringPieceBytes(ByteString.EMPTY).build();
    assertThat(oneFieldSet.equals(TestAllTypesLite.getDefaultInstance())).isFalse();
    assertThat(TestAllTypesLite.getDefaultInstance().equals(oneFieldSet)).isFalse();

    oneFieldSet = TestAllTypesLite.newBuilder().setDefaultUint32(0).build();
    assertThat(oneFieldSet.equals(TestAllTypesLite.getDefaultInstance())).isFalse();
    assertThat(TestAllTypesLite.getDefaultInstance().equals(oneFieldSet)).isFalse();

    oneFieldSet = TestAllTypesLite.newBuilder().setDefaultUint64(0).build();
    assertThat(oneFieldSet.equals(TestAllTypesLite.getDefaultInstance())).isFalse();
    assertThat(TestAllTypesLite.getDefaultInstance().equals(oneFieldSet)).isFalse();

    oneFieldSet = TestAllTypesLite.newBuilder().addRepeatedBool(true).build();
    assertThat(oneFieldSet.equals(TestAllTypesLite.getDefaultInstance())).isFalse();
    assertThat(TestAllTypesLite.getDefaultInstance().equals(oneFieldSet)).isFalse();

    oneFieldSet = TestAllTypesLite.newBuilder().addRepeatedBytes(ByteString.EMPTY).build();
    assertThat(oneFieldSet.equals(TestAllTypesLite.getDefaultInstance())).isFalse();
    assertThat(TestAllTypesLite.getDefaultInstance().equals(oneFieldSet)).isFalse();

    oneFieldSet = TestAllTypesLite.newBuilder().addRepeatedCord("").build();
    assertThat(oneFieldSet.equals(TestAllTypesLite.getDefaultInstance())).isFalse();
    assertThat(TestAllTypesLite.getDefaultInstance().equals(oneFieldSet)).isFalse();

    oneFieldSet = TestAllTypesLite.newBuilder().addRepeatedCordBytes(ByteString.EMPTY).build();
    assertThat(oneFieldSet.equals(TestAllTypesLite.getDefaultInstance())).isFalse();
    assertThat(TestAllTypesLite.getDefaultInstance().equals(oneFieldSet)).isFalse();

    oneFieldSet = TestAllTypesLite.newBuilder().addRepeatedDouble(0).build();
    assertThat(oneFieldSet.equals(TestAllTypesLite.getDefaultInstance())).isFalse();
    assertThat(TestAllTypesLite.getDefaultInstance().equals(oneFieldSet)).isFalse();

    oneFieldSet = TestAllTypesLite.newBuilder().addRepeatedFixed32(0).build();
    assertThat(oneFieldSet.equals(TestAllTypesLite.getDefaultInstance())).isFalse();
    assertThat(TestAllTypesLite.getDefaultInstance().equals(oneFieldSet)).isFalse();

    oneFieldSet = TestAllTypesLite.newBuilder().addRepeatedFixed64(0).build();
    assertThat(oneFieldSet.equals(TestAllTypesLite.getDefaultInstance())).isFalse();
    assertThat(TestAllTypesLite.getDefaultInstance().equals(oneFieldSet)).isFalse();

    oneFieldSet = TestAllTypesLite.newBuilder().addRepeatedFloat(0).build();
    assertThat(oneFieldSet.equals(TestAllTypesLite.getDefaultInstance())).isFalse();
    assertThat(TestAllTypesLite.getDefaultInstance().equals(oneFieldSet)).isFalse();

    oneFieldSet =
        TestAllTypesLite.newBuilder()
            .addRepeatedForeignEnum(ForeignEnumLite.FOREIGN_LITE_BAR)
            .build();
    assertThat(oneFieldSet.equals(TestAllTypesLite.getDefaultInstance())).isFalse();
    assertThat(TestAllTypesLite.getDefaultInstance().equals(oneFieldSet)).isFalse();

    oneFieldSet =
        TestAllTypesLite.newBuilder().addRepeatedImportEnum(ImportEnumLite.IMPORT_LITE_BAR).build();
    assertThat(oneFieldSet.equals(TestAllTypesLite.getDefaultInstance())).isFalse();
    assertThat(TestAllTypesLite.getDefaultInstance().equals(oneFieldSet)).isFalse();

    oneFieldSet = TestAllTypesLite.newBuilder().addRepeatedInt32(0).build();
    assertThat(oneFieldSet.equals(TestAllTypesLite.getDefaultInstance())).isFalse();
    assertThat(TestAllTypesLite.getDefaultInstance().equals(oneFieldSet)).isFalse();

    oneFieldSet = TestAllTypesLite.newBuilder().addRepeatedInt64(0).build();
    assertThat(oneFieldSet.equals(TestAllTypesLite.getDefaultInstance())).isFalse();
    assertThat(TestAllTypesLite.getDefaultInstance().equals(oneFieldSet)).isFalse();

    oneFieldSet = TestAllTypesLite.newBuilder().addRepeatedNestedEnum(NestedEnum.BAR).build();
    assertThat(oneFieldSet.equals(TestAllTypesLite.getDefaultInstance())).isFalse();
    assertThat(TestAllTypesLite.getDefaultInstance().equals(oneFieldSet)).isFalse();

    oneFieldSet = TestAllTypesLite.newBuilder().addRepeatedSfixed32(0).build();
    assertThat(oneFieldSet.equals(TestAllTypesLite.getDefaultInstance())).isFalse();
    assertThat(TestAllTypesLite.getDefaultInstance().equals(oneFieldSet)).isFalse();

    oneFieldSet = TestAllTypesLite.newBuilder().addRepeatedSfixed64(0).build();
    assertThat(oneFieldSet.equals(TestAllTypesLite.getDefaultInstance())).isFalse();
    assertThat(TestAllTypesLite.getDefaultInstance().equals(oneFieldSet)).isFalse();

    oneFieldSet = TestAllTypesLite.newBuilder().addRepeatedSint32(0).build();
    assertThat(oneFieldSet.equals(TestAllTypesLite.getDefaultInstance())).isFalse();
    assertThat(TestAllTypesLite.getDefaultInstance().equals(oneFieldSet)).isFalse();

    oneFieldSet = TestAllTypesLite.newBuilder().addRepeatedSint64(0).build();
    assertThat(oneFieldSet.equals(TestAllTypesLite.getDefaultInstance())).isFalse();
    assertThat(TestAllTypesLite.getDefaultInstance().equals(oneFieldSet)).isFalse();

    oneFieldSet = TestAllTypesLite.newBuilder().addRepeatedString("").build();
    assertThat(oneFieldSet.equals(TestAllTypesLite.getDefaultInstance())).isFalse();
    assertThat(TestAllTypesLite.getDefaultInstance().equals(oneFieldSet)).isFalse();

    oneFieldSet = TestAllTypesLite.newBuilder().addRepeatedStringBytes(ByteString.EMPTY).build();
    assertThat(oneFieldSet.equals(TestAllTypesLite.getDefaultInstance())).isFalse();
    assertThat(TestAllTypesLite.getDefaultInstance().equals(oneFieldSet)).isFalse();

    oneFieldSet = TestAllTypesLite.newBuilder().addRepeatedStringPiece("").build();
    assertThat(oneFieldSet.equals(TestAllTypesLite.getDefaultInstance())).isFalse();
    assertThat(TestAllTypesLite.getDefaultInstance().equals(oneFieldSet)).isFalse();

    oneFieldSet =
        TestAllTypesLite.newBuilder().addRepeatedStringPieceBytes(ByteString.EMPTY).build();
    assertThat(oneFieldSet.equals(TestAllTypesLite.getDefaultInstance())).isFalse();
    assertThat(TestAllTypesLite.getDefaultInstance().equals(oneFieldSet)).isFalse();

    oneFieldSet = TestAllTypesLite.newBuilder().addRepeatedUint32(0).build();
    assertThat(oneFieldSet.equals(TestAllTypesLite.getDefaultInstance())).isFalse();
    assertThat(TestAllTypesLite.getDefaultInstance().equals(oneFieldSet)).isFalse();

    oneFieldSet = TestAllTypesLite.newBuilder().addRepeatedUint64(0).build();
    assertThat(oneFieldSet.equals(TestAllTypesLite.getDefaultInstance())).isFalse();
    assertThat(TestAllTypesLite.getDefaultInstance().equals(oneFieldSet)).isFalse();

    oneFieldSet = TestAllTypesLite.newBuilder().setOptionalBool(true).build();
    assertThat(oneFieldSet.equals(TestAllTypesLite.getDefaultInstance())).isFalse();
    assertThat(TestAllTypesLite.getDefaultInstance().equals(oneFieldSet)).isFalse();

    oneFieldSet = TestAllTypesLite.newBuilder().setOptionalBytes(ByteString.EMPTY).build();
    assertThat(oneFieldSet.equals(TestAllTypesLite.getDefaultInstance())).isFalse();
    assertThat(TestAllTypesLite.getDefaultInstance().equals(oneFieldSet)).isFalse();

    oneFieldSet = TestAllTypesLite.newBuilder().setOptionalCord("").build();
    assertThat(oneFieldSet.equals(TestAllTypesLite.getDefaultInstance())).isFalse();
    assertThat(TestAllTypesLite.getDefaultInstance().equals(oneFieldSet)).isFalse();

    oneFieldSet = TestAllTypesLite.newBuilder().setOptionalCordBytes(ByteString.EMPTY).build();
    assertThat(oneFieldSet.equals(TestAllTypesLite.getDefaultInstance())).isFalse();
    assertThat(TestAllTypesLite.getDefaultInstance().equals(oneFieldSet)).isFalse();

    oneFieldSet = TestAllTypesLite.newBuilder().setOptionalDouble(0).build();
    assertThat(oneFieldSet.equals(TestAllTypesLite.getDefaultInstance())).isFalse();
    assertThat(TestAllTypesLite.getDefaultInstance().equals(oneFieldSet)).isFalse();

    oneFieldSet = TestAllTypesLite.newBuilder().setOptionalFixed32(0).build();
    assertThat(oneFieldSet.equals(TestAllTypesLite.getDefaultInstance())).isFalse();
    assertThat(TestAllTypesLite.getDefaultInstance().equals(oneFieldSet)).isFalse();

    oneFieldSet = TestAllTypesLite.newBuilder().setOptionalFixed64(0).build();
    assertThat(oneFieldSet.equals(TestAllTypesLite.getDefaultInstance())).isFalse();
    assertThat(TestAllTypesLite.getDefaultInstance().equals(oneFieldSet)).isFalse();

    oneFieldSet = TestAllTypesLite.newBuilder().setOptionalFloat(0).build();
    assertThat(oneFieldSet.equals(TestAllTypesLite.getDefaultInstance())).isFalse();
    assertThat(TestAllTypesLite.getDefaultInstance().equals(oneFieldSet)).isFalse();

    oneFieldSet =
        TestAllTypesLite.newBuilder()
            .setOptionalForeignEnum(ForeignEnumLite.FOREIGN_LITE_BAR)
            .build();
    assertThat(oneFieldSet.equals(TestAllTypesLite.getDefaultInstance())).isFalse();
    assertThat(TestAllTypesLite.getDefaultInstance().equals(oneFieldSet)).isFalse();

    oneFieldSet =
        TestAllTypesLite.newBuilder().setOptionalImportEnum(ImportEnumLite.IMPORT_LITE_BAR).build();
    assertThat(oneFieldSet.equals(TestAllTypesLite.getDefaultInstance())).isFalse();
    assertThat(TestAllTypesLite.getDefaultInstance().equals(oneFieldSet)).isFalse();

    oneFieldSet = TestAllTypesLite.newBuilder().setOptionalInt32(0).build();
    assertThat(oneFieldSet.equals(TestAllTypesLite.getDefaultInstance())).isFalse();
    assertThat(TestAllTypesLite.getDefaultInstance().equals(oneFieldSet)).isFalse();

    oneFieldSet = TestAllTypesLite.newBuilder().setOptionalInt64(0).build();
    assertThat(oneFieldSet.equals(TestAllTypesLite.getDefaultInstance())).isFalse();
    assertThat(TestAllTypesLite.getDefaultInstance().equals(oneFieldSet)).isFalse();

    oneFieldSet = TestAllTypesLite.newBuilder().setOptionalNestedEnum(NestedEnum.BAR).build();
    assertThat(oneFieldSet.equals(TestAllTypesLite.getDefaultInstance())).isFalse();
    assertThat(TestAllTypesLite.getDefaultInstance().equals(oneFieldSet)).isFalse();

    oneFieldSet = TestAllTypesLite.newBuilder().setOptionalSfixed32(0).build();
    assertThat(oneFieldSet.equals(TestAllTypesLite.getDefaultInstance())).isFalse();
    assertThat(TestAllTypesLite.getDefaultInstance().equals(oneFieldSet)).isFalse();

    oneFieldSet = TestAllTypesLite.newBuilder().setOptionalSfixed64(0).build();
    assertThat(oneFieldSet.equals(TestAllTypesLite.getDefaultInstance())).isFalse();
    assertThat(TestAllTypesLite.getDefaultInstance().equals(oneFieldSet)).isFalse();

    oneFieldSet = TestAllTypesLite.newBuilder().setOptionalSint32(0).build();
    assertThat(oneFieldSet.equals(TestAllTypesLite.getDefaultInstance())).isFalse();
    assertThat(TestAllTypesLite.getDefaultInstance().equals(oneFieldSet)).isFalse();

    oneFieldSet = TestAllTypesLite.newBuilder().setOptionalSint64(0).build();
    assertThat(oneFieldSet.equals(TestAllTypesLite.getDefaultInstance())).isFalse();
    assertThat(TestAllTypesLite.getDefaultInstance().equals(oneFieldSet)).isFalse();

    oneFieldSet = TestAllTypesLite.newBuilder().setOptionalString("").build();
    assertThat(oneFieldSet.equals(TestAllTypesLite.getDefaultInstance())).isFalse();
    assertThat(TestAllTypesLite.getDefaultInstance().equals(oneFieldSet)).isFalse();

    oneFieldSet = TestAllTypesLite.newBuilder().setOptionalStringBytes(ByteString.EMPTY).build();
    assertThat(oneFieldSet.equals(TestAllTypesLite.getDefaultInstance())).isFalse();
    assertThat(TestAllTypesLite.getDefaultInstance().equals(oneFieldSet)).isFalse();

    oneFieldSet = TestAllTypesLite.newBuilder().setOptionalStringPiece("").build();
    assertThat(oneFieldSet.equals(TestAllTypesLite.getDefaultInstance())).isFalse();
    assertThat(TestAllTypesLite.getDefaultInstance().equals(oneFieldSet)).isFalse();

    oneFieldSet =
        TestAllTypesLite.newBuilder().setOptionalStringPieceBytes(ByteString.EMPTY).build();
    assertThat(oneFieldSet.equals(TestAllTypesLite.getDefaultInstance())).isFalse();
    assertThat(TestAllTypesLite.getDefaultInstance().equals(oneFieldSet)).isFalse();

    oneFieldSet = TestAllTypesLite.newBuilder().setOptionalUint32(0).build();
    assertThat(oneFieldSet.equals(TestAllTypesLite.getDefaultInstance())).isFalse();
    assertThat(TestAllTypesLite.getDefaultInstance().equals(oneFieldSet)).isFalse();

    oneFieldSet = TestAllTypesLite.newBuilder().setOptionalUint64(0).build();
    assertThat(oneFieldSet.equals(TestAllTypesLite.getDefaultInstance())).isFalse();
    assertThat(TestAllTypesLite.getDefaultInstance().equals(oneFieldSet)).isFalse();

    oneFieldSet = TestAllTypesLite.newBuilder().setOneofBytes(ByteString.EMPTY).build();
    assertThat(oneFieldSet.equals(TestAllTypesLite.getDefaultInstance())).isFalse();
    assertThat(TestAllTypesLite.getDefaultInstance().equals(oneFieldSet)).isFalse();

    oneFieldSet =
        TestAllTypesLite.newBuilder()
            .setOneofLazyNestedMessage(NestedMessage.getDefaultInstance())
            .build();
    assertThat(oneFieldSet.equals(TestAllTypesLite.getDefaultInstance())).isFalse();
    assertThat(TestAllTypesLite.getDefaultInstance().equals(oneFieldSet)).isFalse();

    oneFieldSet =
        TestAllTypesLite.newBuilder()
            .setOneofNestedMessage(NestedMessage.getDefaultInstance())
            .build();
    assertThat(oneFieldSet.equals(TestAllTypesLite.getDefaultInstance())).isFalse();
    assertThat(TestAllTypesLite.getDefaultInstance().equals(oneFieldSet)).isFalse();

    oneFieldSet = TestAllTypesLite.newBuilder().setOneofString("").build();
    assertThat(oneFieldSet.equals(TestAllTypesLite.getDefaultInstance())).isFalse();
    assertThat(TestAllTypesLite.getDefaultInstance().equals(oneFieldSet)).isFalse();

    oneFieldSet = TestAllTypesLite.newBuilder().setOneofStringBytes(ByteString.EMPTY).build();
    assertThat(oneFieldSet.equals(TestAllTypesLite.getDefaultInstance())).isFalse();
    assertThat(TestAllTypesLite.getDefaultInstance().equals(oneFieldSet)).isFalse();

    oneFieldSet = TestAllTypesLite.newBuilder().setOneofUint32(0).build();
    assertThat(oneFieldSet.equals(TestAllTypesLite.getDefaultInstance())).isFalse();
    assertThat(TestAllTypesLite.getDefaultInstance().equals(oneFieldSet)).isFalse();

    oneFieldSet =
        TestAllTypesLite.newBuilder()
            .setOptionalForeignMessage(ForeignMessageLite.getDefaultInstance())
            .build();
    assertThat(oneFieldSet.equals(TestAllTypesLite.getDefaultInstance())).isFalse();
    assertThat(TestAllTypesLite.getDefaultInstance().equals(oneFieldSet)).isFalse();

    oneFieldSet =
        TestAllTypesLite.newBuilder().setOptionalGroup(OptionalGroup.getDefaultInstance()).build();
    assertThat(oneFieldSet.equals(TestAllTypesLite.getDefaultInstance())).isFalse();
    assertThat(TestAllTypesLite.getDefaultInstance().equals(oneFieldSet)).isFalse();

    oneFieldSet =
        TestAllTypesLite.newBuilder()
            .setOptionalPublicImportMessage(PublicImportMessageLite.getDefaultInstance())
            .build();
    assertThat(oneFieldSet.equals(TestAllTypesLite.getDefaultInstance())).isFalse();
    assertThat(TestAllTypesLite.getDefaultInstance().equals(oneFieldSet)).isFalse();

    oneFieldSet =
        TestAllTypesLite.newBuilder()
            .setOptionalLazyMessage(NestedMessage.getDefaultInstance())
            .build();
    assertThat(oneFieldSet.equals(TestAllTypesLite.getDefaultInstance())).isFalse();

    assertThat(TestAllTypesLite.getDefaultInstance().equals(oneFieldSet)).isFalse();
    oneFieldSet =
        TestAllTypesLite.newBuilder()
            .addRepeatedLazyMessage(NestedMessage.getDefaultInstance())
            .build();
    assertThat(oneFieldSet.equals(TestAllTypesLite.getDefaultInstance())).isFalse();
    assertThat(TestAllTypesLite.getDefaultInstance().equals(oneFieldSet)).isFalse();
  }

  @Test
  public void testEquals() throws Exception {
    // Check that two identical objs are equal.
    Foo foo1a = Foo.newBuilder().setValue(1).addBar(Bar.newBuilder().setName("foo1")).build();
    Foo foo1b = Foo.newBuilder().setValue(1).addBar(Bar.newBuilder().setName("foo1")).build();
    Foo foo2 = Foo.newBuilder().setValue(1).addBar(Bar.newBuilder().setName("foo2")).build();

    // Check that equals is doing value rather than object equality.
    assertThat(foo1a).isEqualTo(foo1b);
    assertThat(foo1a.hashCode()).isEqualTo(foo1b.hashCode());

    // Check that a different object is not equal.
    assertThat(foo1a.equals(foo2)).isFalse();

    // Check that two objects which have different types but the same field values are not
    // considered to be equal.
    Bar bar = Bar.newBuilder().setName("bar").build();
    BarPrime barPrime = BarPrime.newBuilder().setName("bar").build();
    assertThat(bar).isNotEqualTo((Object) barPrime); // compiler already won't let this happen.
  }

  @Test
  public void testEqualsAndHashCodeForTrickySchemaTypes() {
    Foo foo1 = Foo.getDefaultInstance();
    Foo foo2 = Foo.newBuilder().setSint64(1).build();
    Foo foo3 = Foo.newBuilder().putMyMap("key", "value2").build();
    Foo foo4 = Foo.newBuilder().setMyGroup(Foo.MyGroup.newBuilder().setValue(4).build()).build();

    assertEqualsAndHashCodeAreFalse(foo1, foo2);
    assertEqualsAndHashCodeAreFalse(foo1, foo3);
    assertEqualsAndHashCodeAreFalse(foo1, foo4);
  }

  @Test
  public void testOneofEquals() throws Exception {
    TestOneofEquals.Builder builder = TestOneofEquals.newBuilder();
    TestOneofEquals message1 = builder.build();
    // Set message2's name field to default value. The two messages should be different when we
    // check with the oneof case.
    builder.setName("");
    TestOneofEquals message2 = builder.build();
    assertThat(message1.equals(message2)).isFalse();
  }

  @Test
  public void testEquals_sanity() throws Exception {
    TestAllTypesLite one = TestUtilLite.getAllLiteSetBuilder().build();
    TestAllTypesLite two = TestAllTypesLite.parseFrom(one.toByteArray());
    assertThat(one).isEqualTo(two);
    assertThat(one.hashCode()).isEqualTo(two.hashCode());

    assertThat(one.toBuilder().mergeFrom(two).build())
        .isEqualTo(two.toBuilder().mergeFrom(two.toByteArray()).build());
  }

  @Test
  public void testEqualsAndHashCodeWithUnknownFields() throws InvalidProtocolBufferException {
    Foo fooWithOnlyValue = Foo.newBuilder().setValue(1).build();

    Foo fooWithValueAndExtension =
        fooWithOnlyValue.toBuilder()
            .setValue(1)
            .setExtension(Bar.fooExt, Bar.newBuilder().setName("name").build())
            .build();

    Foo fooWithValueAndUnknownFields = Foo.parseFrom(fooWithValueAndExtension.toByteArray());

    assertEqualsAndHashCodeAreFalse(fooWithOnlyValue, fooWithValueAndUnknownFields);
    assertEqualsAndHashCodeAreFalse(fooWithValueAndExtension, fooWithValueAndUnknownFields);
  }

  @Test
  public void testEqualsAndHashCodeWithExtensions() throws InvalidProtocolBufferException {
    Foo fooWithOnlyValue = Foo.newBuilder().setValue(1).build();

    Foo fooWithValueAndExtension =
        fooWithOnlyValue.toBuilder()
            .setValue(1)
            .setExtension(Bar.fooExt, Bar.newBuilder().setName("name").build())
            .build();

    assertEqualsAndHashCodeAreFalse(fooWithOnlyValue, fooWithValueAndExtension);
  }

  // Test to ensure we avoid a class cast exception with oneofs.
  @Test
  public void testEquals_oneOfMessages() {
    TestAllTypesLite mine = TestAllTypesLite.newBuilder().setOneofString("Hello").build();

    TestAllTypesLite other =
        TestAllTypesLite.newBuilder()
            .setOneofNestedMessage(NestedMessage.getDefaultInstance())
            .build();

    assertThat(mine.equals(other)).isFalse();
    assertThat(other.equals(mine)).isFalse();
  }

  @Test
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
    assertThat(parsedMessage.getOptionalInt32()).isEqualTo(1);
    assertThat(parsedMessage.getRepeatedInt32(0)).isEqualTo(2);
    assertThat(parsedMessage.getOptionalEnum()).isEqualTo(ForeignEnumLite.FOREIGN_LITE_FOO);
    assertThat(parsedMessage.getOptionalString()).isEqualTo("xyz");
    assertThat(parsedMessage.getOptionalMessage().getC()).isEqualTo(3);
  }

  private void assertEqualsAndHashCodeAreFalse(Object o1, Object o2) {
    assertThat(o1.equals(o2)).isFalse();
    assertThat(o1.hashCode()).isNotEqualTo(o2.hashCode());
  }

  @Test
  public void testRecursiveHashcode() {
    // This tests that we don't infinite loop.
    int unused = TestRecursiveOneof.getDefaultInstance().hashCode();
  }

  @Test
  public void testParseFromByteBuffer() throws Exception {
    TestAllTypesLite message =
        TestAllTypesLite.newBuilder()
            .setOptionalInt32(123)
            .addRepeatedString("hello")
            .setOptionalNestedMessage(NestedMessage.newBuilder().setBb(7))
            .build();

    TestAllTypesLite copy =
        TestAllTypesLite.parseFrom(message.toByteString().asReadOnlyByteBuffer());

    assertThat(message).isEqualTo(copy);
  }

  @Test
  public void testParseFromByteBufferThrows() {
    try {
      TestAllTypesLite.parseFrom(ByteBuffer.wrap(new byte[] {0x5}));
      assertWithMessage("expected exception").fail();
    } catch (InvalidProtocolBufferException expected) {
    }

    TestAllTypesLite message =
        TestAllTypesLite.newBuilder().setOptionalInt32(123).addRepeatedString("hello").build();

    ByteBuffer buffer = ByteBuffer.wrap(message.toByteArray(), 0, message.getSerializedSize() - 1);
    try {
      TestAllTypesLite.parseFrom(buffer);
      assertWithMessage("expected exception").fail();
    } catch (InvalidProtocolBufferException expected) {
      assertThat(TestAllTypesLite.newBuilder().setOptionalInt32(123).build())
          .isEqualTo(expected.getUnfinishedMessage());
    }
  }

  @Test
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

    assertThat(message).isEqualTo(copy);
  }

  @Test
  public void testParseFromByteBufferThrows_extensions() {
    ExtensionRegistryLite registry = ExtensionRegistryLite.newInstance();
    UnittestLite.registerAllExtensions(registry);
    try {
      TestAllExtensionsLite.parseFrom(ByteBuffer.wrap(new byte[] {0x5}), registry);
      assertWithMessage("expected exception").fail();
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
      assertWithMessage("expected exception").fail();
    } catch (InvalidProtocolBufferException expected) {
      assertThat(
              TestAllExtensionsLite.newBuilder()
                  .setExtension(UnittestLite.optionalInt32ExtensionLite, 123)
                  .build())
          .isEqualTo(expected.getUnfinishedMessage());
    }
  }

  // Make sure we haven't screwed up the code generation for packing fields by default.
  @Test
  public void testPackedSerialization() throws Exception {
    TestAllTypes.Builder builder = TestAllTypes.newBuilder();
    builder.addRepeatedInt32(4321);
    builder.addRepeatedNestedEnum(TestAllTypes.NestedEnum.BAZ);
    TestAllTypes message = builder.build();

    CodedInputStream in = CodedInputStream.newInstance(message.toByteArray());

    while (!in.isAtEnd()) {
      int tag = in.readTag();
      assertThat(WireFormat.getTagWireType(tag)).isEqualTo(WireFormat.WIRETYPE_LENGTH_DELIMITED);
      in.skipField(tag);
    }
  }

  @Test
  public void testAddAllIteratesOnce() {
    TestAllTypesLite unused =
        TestAllTypesLite.newBuilder()
            .addAllRepeatedBool(new OneTimeIterableList<>(false))
            .addAllRepeatedInt32(new OneTimeIterableList<>(0))
            .addAllRepeatedInt64(new OneTimeIterableList<>(0L))
            .addAllRepeatedFloat(new OneTimeIterableList<>(0f))
            .addAllRepeatedDouble(new OneTimeIterableList<>(0d))
            .addAllRepeatedBytes(new OneTimeIterableList<>(ByteString.EMPTY))
            .addAllRepeatedString(new OneTimeIterableList<>(""))
            .addAllRepeatedNestedMessage(
                new OneTimeIterableList<>(NestedMessage.getDefaultInstance()))
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

  @Test
  public void testAddAllIteratesOnce_throwsOnNull() {
    TestAllTypesLite.Builder builder = TestAllTypesLite.newBuilder();
    try {
      builder.addAllRepeatedBool(new OneTimeIterableList<>(true, false, null));
      assertWithMessage("expected exception").fail();
    } catch (NullPointerException expected) {
      assertThat(expected).hasMessageThat().isEqualTo("Element at index 2 is null.");
      assertThat(builder.getRepeatedBoolCount()).isEqualTo(0);
    }

    try {
      builder.addAllRepeatedBool(new OneTimeIterable<>(true, false, null));
      assertWithMessage("expected exception").fail();
    } catch (NullPointerException expected) {
      assertThat(expected).hasMessageThat().isEqualTo("Element at index 2 is null.");
      assertThat(builder.getRepeatedBoolCount()).isEqualTo(0);
    }

    try {
      builder = TestAllTypesLite.newBuilder();
      builder.addAllRepeatedBool(new OneTimeIterableList<>((Boolean) null));
      assertWithMessage("expected exception").fail();
    } catch (NullPointerException expected) {
      assertThat(expected).hasMessageThat().isEqualTo("Element at index 0 is null.");
      assertThat(builder.getRepeatedBoolCount()).isEqualTo(0);
    }

    try {
      builder = TestAllTypesLite.newBuilder();
      builder.addAllRepeatedInt32(new OneTimeIterableList<>((Integer) null));
      assertWithMessage("expected exception").fail();
    } catch (NullPointerException expected) {
      assertThat(expected).hasMessageThat().isEqualTo("Element at index 0 is null.");
      assertThat(builder.getRepeatedInt32Count()).isEqualTo(0);
    }

    try {
      builder = TestAllTypesLite.newBuilder();
      builder.addAllRepeatedInt64(new OneTimeIterableList<>((Long) null));
      assertWithMessage("expected exception").fail();
    } catch (NullPointerException expected) {
      assertThat(expected).hasMessageThat().isEqualTo("Element at index 0 is null.");
      assertThat(builder.getRepeatedInt64Count()).isEqualTo(0);
    }

    try {
      builder = TestAllTypesLite.newBuilder();
      builder.addAllRepeatedFloat(new OneTimeIterableList<>((Float) null));
      assertWithMessage("expected exception").fail();
    } catch (NullPointerException expected) {
      assertThat(expected).hasMessageThat().isEqualTo("Element at index 0 is null.");
      assertThat(builder.getRepeatedFloatCount()).isEqualTo(0);
    }

    try {
      builder = TestAllTypesLite.newBuilder();
      builder.addAllRepeatedDouble(new OneTimeIterableList<>((Double) null));
      assertWithMessage("expected exception").fail();
    } catch (NullPointerException expected) {
      assertThat(expected).hasMessageThat().isEqualTo("Element at index 0 is null.");
      assertThat(builder.getRepeatedDoubleCount()).isEqualTo(0);
    }

    try {
      builder = TestAllTypesLite.newBuilder();
      builder.addAllRepeatedBytes(new OneTimeIterableList<>((ByteString) null));
      assertWithMessage("expected exception").fail();
    } catch (NullPointerException expected) {
      assertThat(expected).hasMessageThat().isEqualTo("Element at index 0 is null.");
      assertThat(builder.getRepeatedBytesCount()).isEqualTo(0);
    }

    try {
      builder = TestAllTypesLite.newBuilder();
      builder.addAllRepeatedString(new OneTimeIterableList<>("", "", null, ""));
      assertWithMessage("expected exception").fail();
    } catch (NullPointerException expected) {
      assertThat(expected).hasMessageThat().isEqualTo("Element at index 2 is null.");
      assertThat(builder.getRepeatedStringCount()).isEqualTo(0);
    }

    try {
      builder = TestAllTypesLite.newBuilder();
      builder.addAllRepeatedString(new OneTimeIterable<>("", "", null, ""));
      assertWithMessage("expected exception").fail();
    } catch (NullPointerException expected) {
      assertThat(expected).hasMessageThat().isEqualTo("Element at index 2 is null.");
      assertThat(builder.getRepeatedStringCount()).isEqualTo(0);
    }

    try {
      builder = TestAllTypesLite.newBuilder();
      builder.addAllRepeatedString(new OneTimeIterableList<>((String) null));
      assertWithMessage("expected exception").fail();
    } catch (NullPointerException expected) {
      assertThat(expected).hasMessageThat().isEqualTo("Element at index 0 is null.");
      assertThat(builder.getRepeatedStringCount()).isEqualTo(0);
    }

    try {
      builder = TestAllTypesLite.newBuilder();
      builder.addAllRepeatedNestedMessage(new OneTimeIterableList<>((NestedMessage) null));
      assertWithMessage("expected exception").fail();
    } catch (NullPointerException expected) {
      assertThat(expected).hasMessageThat().isEqualTo("Element at index 0 is null.");
      assertThat(builder.getRepeatedNestedMessageCount()).isEqualTo(0);
    }
  }

  @Test
  public void testExtensionRenamesKeywords() {
    assertThat(NonNestedExtensionLite.package_)
        .isInstanceOf(GeneratedMessageLite.GeneratedExtension.class);
    assertThat(NestedExtensionLite.MyNestedExtensionLite.private_)
        .isInstanceOf(GeneratedMessageLite.GeneratedExtension.class);

    NonNestedExtensionLite.MessageLiteToBeExtended msg =
        NonNestedExtensionLite.MessageLiteToBeExtended.newBuilder()
            .setExtension(NonNestedExtensionLite.package_, true)
            .build();
    assertThat(msg.getExtension(NonNestedExtensionLite.package_)).isTrue();

    msg =
        NonNestedExtensionLite.MessageLiteToBeExtended.newBuilder()
            .setExtension(NestedExtensionLite.MyNestedExtensionLite.private_, 2.4)
            .build();
    assertThat(msg.getExtension(NestedExtensionLite.MyNestedExtensionLite.private_)).isEqualTo(2.4);
  }

  private static final class OneTimeIterableList<T> extends ArrayList<T> {
    private boolean wasIterated = false;

    OneTimeIterableList(T... contents) {
      addAll(Arrays.asList(contents));
    }

    @Override
    public Iterator<T> iterator() {
      if (wasIterated) {
        assertWithMessage("expected exception").fail();
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
        assertWithMessage("expected exception").fail();
      }
      wasIterated = true;
      return list.iterator();
    }
  }

  @Test
  public void testNullExtensionRegistry() throws Exception {
    try {
      TestAllTypesLite.parseFrom(new byte[] {}, null);
      assertWithMessage("expected exception").fail();
    } catch (NullPointerException expected) {
    }
  }

  @Test
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
      assertWithMessage("expected exception").fail();
    } catch (IOException expected) {
    }
  }

  @Test
  public void testUnpairedSurrogatesReplacedByQuestionMark() throws InvalidProtocolBufferException {
    String testString = "foo \ud83d bar";
    String expectedString = "foo ? bar";

    TestAllTypesLite testMessage =
        TestAllTypesLite.newBuilder().setOptionalString(testString).build();
    ByteString serializedMessage = testMessage.toByteString();

    // Behavior is compatible with String.getBytes("UTF-8"), which replaces
    // unpaired surrogates with a question mark.
    TestAllTypesLite parsedMessage = TestAllTypesLite.parseFrom(serializedMessage);
    assertThat(parsedMessage.getOptionalString()).isEqualTo(expectedString);

    // Conversion happens during serialization.
    ByteString expectedBytes = ByteString.copyFromUtf8(expectedString);
    assertWithMessage(
            String.format(
                "Expected serializedMessage (%s) to contain \"%s\" (%s).",
                encodeHex(serializedMessage), expectedString, encodeHex(expectedBytes)))
        .that(contains(serializedMessage, expectedBytes))
        .isTrue();
  }

  @Test
  public void testPreservesFloatingPointNegative0() throws Exception {
    proto3_unittest.UnittestProto3.TestAllTypes message =
        proto3_unittest.UnittestProto3.TestAllTypes.newBuilder()
            .setOptionalFloat(-0.0f)
            .setOptionalDouble(-0.0)
            .build();
    assertThat(
            proto3_unittest.UnittestProto3.TestAllTypes.parseFrom(
                message.toByteString(), ExtensionRegistryLite.getEmptyRegistry()))
        .isEqualTo(message);
  }

  @Test
  public void testNegative0FloatingPointEquality() throws Exception {
    // Like Double#equals and Float#equals, we treat -0.0 as not being equal to +0.0 even though
    // IEEE 754 mandates that they are equivalent. This test asserts that behavior.
    proto3_unittest.UnittestProto3.TestAllTypes message1 =
        proto3_unittest.UnittestProto3.TestAllTypes.newBuilder()
            .setOptionalFloat(-0.0f)
            .setOptionalDouble(-0.0)
            .build();
    proto3_unittest.UnittestProto3.TestAllTypes message2 =
        proto3_unittest.UnittestProto3.TestAllTypes.newBuilder()
            .setOptionalFloat(0.0f)
            .setOptionalDouble(0.0)
            .build();
    assertThat(message1).isNotEqualTo(message2);
  }

  private static String encodeHex(ByteString bytes) {
    String hexDigits = "0123456789abcdef";
    StringBuilder stringBuilder = new StringBuilder(bytes.size() * 2);
    for (byte b : bytes) {
      stringBuilder.append(hexDigits.charAt((b & 0xf0) >> 4));
      stringBuilder.append(hexDigits.charAt(b & 0x0f));
    }
    return stringBuilder.toString();
  }

  private static boolean contains(ByteString a, ByteString b) {
    for (int i = 0; i <= a.size() - b.size(); ++i) {
      if (a.substring(i, i + b.size()).equals(b)) {
        return true;
      }
    }
    return false;
  }
}
