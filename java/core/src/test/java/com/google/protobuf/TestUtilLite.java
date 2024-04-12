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

import static com.google.protobuf.UnittestLite.defaultBoolExtensionLite;
import static com.google.protobuf.UnittestLite.defaultBytesExtensionLite;
import static com.google.protobuf.UnittestLite.defaultCordExtensionLite;
import static com.google.protobuf.UnittestLite.defaultDoubleExtensionLite;
import static com.google.protobuf.UnittestLite.defaultFixed32ExtensionLite;
import static com.google.protobuf.UnittestLite.defaultFixed64ExtensionLite;
import static com.google.protobuf.UnittestLite.defaultFloatExtensionLite;
import static com.google.protobuf.UnittestLite.defaultForeignEnumExtensionLite;
import static com.google.protobuf.UnittestLite.defaultImportEnumExtensionLite;
import static com.google.protobuf.UnittestLite.defaultInt32ExtensionLite;
import static com.google.protobuf.UnittestLite.defaultInt64ExtensionLite;
import static com.google.protobuf.UnittestLite.defaultNestedEnumExtensionLite;
import static com.google.protobuf.UnittestLite.defaultSfixed32ExtensionLite;
import static com.google.protobuf.UnittestLite.defaultSfixed64ExtensionLite;
import static com.google.protobuf.UnittestLite.defaultSint32ExtensionLite;
import static com.google.protobuf.UnittestLite.defaultSint64ExtensionLite;
import static com.google.protobuf.UnittestLite.defaultStringExtensionLite;
import static com.google.protobuf.UnittestLite.defaultStringPieceExtensionLite;
import static com.google.protobuf.UnittestLite.defaultUint32ExtensionLite;
import static com.google.protobuf.UnittestLite.defaultUint64ExtensionLite;
import static com.google.protobuf.UnittestLite.oneofBytesExtensionLite;
import static com.google.protobuf.UnittestLite.oneofNestedMessageExtensionLite;
import static com.google.protobuf.UnittestLite.oneofStringExtensionLite;
import static com.google.protobuf.UnittestLite.oneofUint32ExtensionLite;
import static com.google.protobuf.UnittestLite.optionalBoolExtensionLite;
import static com.google.protobuf.UnittestLite.optionalBytesExtensionLite;
import static com.google.protobuf.UnittestLite.optionalCordExtensionLite;
import static com.google.protobuf.UnittestLite.optionalDoubleExtensionLite;
import static com.google.protobuf.UnittestLite.optionalFixed32ExtensionLite;
import static com.google.protobuf.UnittestLite.optionalFixed64ExtensionLite;
import static com.google.protobuf.UnittestLite.optionalFloatExtensionLite;
import static com.google.protobuf.UnittestLite.optionalForeignEnumExtensionLite;
import static com.google.protobuf.UnittestLite.optionalForeignMessageExtensionLite;
import static com.google.protobuf.UnittestLite.optionalGroupExtensionLite;
import static com.google.protobuf.UnittestLite.optionalImportEnumExtensionLite;
import static com.google.protobuf.UnittestLite.optionalImportMessageExtensionLite;
import static com.google.protobuf.UnittestLite.optionalInt32ExtensionLite;
import static com.google.protobuf.UnittestLite.optionalInt64ExtensionLite;
import static com.google.protobuf.UnittestLite.optionalLazyMessageExtensionLite;
import static com.google.protobuf.UnittestLite.optionalNestedEnumExtensionLite;
import static com.google.protobuf.UnittestLite.optionalNestedMessageExtensionLite;
import static com.google.protobuf.UnittestLite.optionalPublicImportMessageExtensionLite;
import static com.google.protobuf.UnittestLite.optionalSfixed32ExtensionLite;
import static com.google.protobuf.UnittestLite.optionalSfixed64ExtensionLite;
import static com.google.protobuf.UnittestLite.optionalSint32ExtensionLite;
import static com.google.protobuf.UnittestLite.optionalSint64ExtensionLite;
import static com.google.protobuf.UnittestLite.optionalStringExtensionLite;
import static com.google.protobuf.UnittestLite.optionalStringPieceExtensionLite;
import static com.google.protobuf.UnittestLite.optionalUint32ExtensionLite;
import static com.google.protobuf.UnittestLite.optionalUint64ExtensionLite;
import static com.google.protobuf.UnittestLite.optionalUnverifiedLazyMessageExtensionLite;
import static com.google.protobuf.UnittestLite.packedBoolExtensionLite;
import static com.google.protobuf.UnittestLite.packedDoubleExtensionLite;
import static com.google.protobuf.UnittestLite.packedEnumExtensionLite;
import static com.google.protobuf.UnittestLite.packedFixed32ExtensionLite;
import static com.google.protobuf.UnittestLite.packedFixed64ExtensionLite;
import static com.google.protobuf.UnittestLite.packedFloatExtensionLite;
import static com.google.protobuf.UnittestLite.packedInt32ExtensionLite;
import static com.google.protobuf.UnittestLite.packedInt64ExtensionLite;
import static com.google.protobuf.UnittestLite.packedSfixed32ExtensionLite;
import static com.google.protobuf.UnittestLite.packedSfixed64ExtensionLite;
import static com.google.protobuf.UnittestLite.packedSint32ExtensionLite;
import static com.google.protobuf.UnittestLite.packedSint64ExtensionLite;
import static com.google.protobuf.UnittestLite.packedUint32ExtensionLite;
import static com.google.protobuf.UnittestLite.packedUint64ExtensionLite;
import static com.google.protobuf.UnittestLite.repeatedBoolExtensionLite;
import static com.google.protobuf.UnittestLite.repeatedBytesExtensionLite;
import static com.google.protobuf.UnittestLite.repeatedCordExtensionLite;
import static com.google.protobuf.UnittestLite.repeatedDoubleExtensionLite;
import static com.google.protobuf.UnittestLite.repeatedFixed32ExtensionLite;
import static com.google.protobuf.UnittestLite.repeatedFixed64ExtensionLite;
import static com.google.protobuf.UnittestLite.repeatedFloatExtensionLite;
import static com.google.protobuf.UnittestLite.repeatedForeignEnumExtensionLite;
import static com.google.protobuf.UnittestLite.repeatedForeignMessageExtensionLite;
import static com.google.protobuf.UnittestLite.repeatedGroupExtensionLite;
import static com.google.protobuf.UnittestLite.repeatedImportEnumExtensionLite;
import static com.google.protobuf.UnittestLite.repeatedImportMessageExtensionLite;
import static com.google.protobuf.UnittestLite.repeatedInt32ExtensionLite;
import static com.google.protobuf.UnittestLite.repeatedInt64ExtensionLite;
import static com.google.protobuf.UnittestLite.repeatedLazyMessageExtensionLite;
import static com.google.protobuf.UnittestLite.repeatedNestedEnumExtensionLite;
import static com.google.protobuf.UnittestLite.repeatedNestedMessageExtensionLite;
import static com.google.protobuf.UnittestLite.repeatedSfixed32ExtensionLite;
import static com.google.protobuf.UnittestLite.repeatedSfixed64ExtensionLite;
import static com.google.protobuf.UnittestLite.repeatedSint32ExtensionLite;
import static com.google.protobuf.UnittestLite.repeatedSint64ExtensionLite;
import static com.google.protobuf.UnittestLite.repeatedStringExtensionLite;
import static com.google.protobuf.UnittestLite.repeatedStringPieceExtensionLite;
import static com.google.protobuf.UnittestLite.repeatedUint32ExtensionLite;
import static com.google.protobuf.UnittestLite.repeatedUint64ExtensionLite;

import com.google.protobuf.UnittestImportLite.ImportEnumLite;
import com.google.protobuf.UnittestImportLite.ImportMessageLite;
import com.google.protobuf.UnittestImportPublicLite.PublicImportMessageLite;
import com.google.protobuf.UnittestLite.ForeignEnumLite;
import com.google.protobuf.UnittestLite.ForeignMessageLite;
import com.google.protobuf.UnittestLite.OptionalGroup_extension_lite;
import com.google.protobuf.UnittestLite.RepeatedGroup_extension_lite;
import com.google.protobuf.UnittestLite.TestAllExtensionsLite;
import com.google.protobuf.UnittestLite.TestAllTypesLite;
import com.google.protobuf.UnittestLite.TestPackedExtensionsLite;

/**
 * Contains methods for setting fields of {@code TestAllTypesLite}, {@code TestAllExtensionsLite},
 * and {@code TestPackedExtensionsLite}. This is analogous to the functionality in TestUtil.java but
 * does not depend on the presence of any non-lite protos.
 *
 * <p>This code is not to be used outside of {@code com.google.protobuf} and subpackages.
 */
public final class TestUtilLite {
  private TestUtilLite() {}

  /** Helper to convert a String to ByteString. */
  public static ByteString toBytes(String str) {
    return ByteString.copyFromUtf8(str);
  }

  /**
   * Get a {@code TestAllTypesLite.Builder} with all fields set as they would be by {@link
   * #setAllFields(TestAllTypesLite.Builder)}.
   */
  public static TestAllTypesLite.Builder getAllLiteSetBuilder() {
    TestAllTypesLite.Builder builder = TestAllTypesLite.newBuilder();
    setAllFields(builder);
    return builder;
  }

  /**
   * Get a {@code TestAllExtensionsLite} with all fields set as they would be by {@link
   * #setAllExtensions(TestAllExtensionsLite.Builder)}.
   */
  public static TestAllExtensionsLite getAllLiteExtensionsSet() {
    TestAllExtensionsLite.Builder builder = TestAllExtensionsLite.newBuilder();
    setAllExtensions(builder);
    return builder.build();
  }

  public static TestPackedExtensionsLite getLitePackedExtensionsSet() {
    TestPackedExtensionsLite.Builder builder = TestPackedExtensionsLite.newBuilder();
    setPackedExtensions(builder);
    return builder.build();
  }

  /** Set every field of {@code builder} to the values expected by {@code assertAllFieldsSet()}. */
  public static void setAllFields(TestAllTypesLite.Builder builder) {
    builder.setOptionalInt32(101);
    builder.setOptionalInt64(102);
    builder.setOptionalUint32(103);
    builder.setOptionalUint64(104);
    builder.setOptionalSint32(105);
    builder.setOptionalSint64(106);
    builder.setOptionalFixed32(107);
    builder.setOptionalFixed64(108);
    builder.setOptionalSfixed32(109);
    builder.setOptionalSfixed64(110);
    builder.setOptionalFloat(111);
    builder.setOptionalDouble(112);
    builder.setOptionalBool(true);
    builder.setOptionalString("115");
    builder.setOptionalBytes(toBytes("116"));

    builder.setOptionalGroup(TestAllTypesLite.OptionalGroup.newBuilder().setA(117).build());
    builder.setOptionalNestedMessage(
        TestAllTypesLite.NestedMessage.newBuilder().setBb(118).build());
    builder.setOptionalForeignMessage(ForeignMessageLite.newBuilder().setC(119).build());
    builder.setOptionalImportMessage(ImportMessageLite.newBuilder().setD(120).build());
    builder.setOptionalPublicImportMessage(PublicImportMessageLite.newBuilder().setE(126).build());
    builder.setOptionalLazyMessage(TestAllTypesLite.NestedMessage.newBuilder().setBb(127).build());
    builder.setOptionalUnverifiedLazyMessage(
        TestAllTypesLite.NestedMessage.newBuilder().setBb(128).build());

    builder.setOptionalNestedEnum(TestAllTypesLite.NestedEnum.BAZ);
    builder.setOptionalForeignEnum(ForeignEnumLite.FOREIGN_LITE_BAZ);
    builder.setOptionalImportEnum(ImportEnumLite.IMPORT_LITE_BAZ);

    builder.setOptionalStringPiece("124");
    builder.setOptionalCord("125");

    // -----------------------------------------------------------------

    builder.addRepeatedInt32(201);
    builder.addRepeatedInt64(202);
    builder.addRepeatedUint32(203);
    builder.addRepeatedUint64(204);
    builder.addRepeatedSint32(205);
    builder.addRepeatedSint64(206);
    builder.addRepeatedFixed32(207);
    builder.addRepeatedFixed64(208);
    builder.addRepeatedSfixed32(209);
    builder.addRepeatedSfixed64(210);
    builder.addRepeatedFloat(211);
    builder.addRepeatedDouble(212);
    builder.addRepeatedBool(true);
    builder.addRepeatedString("215");
    builder.addRepeatedBytes(toBytes("216"));

    builder.addRepeatedGroup(TestAllTypesLite.RepeatedGroup.newBuilder().setA(217).build());
    builder.addRepeatedNestedMessage(
        TestAllTypesLite.NestedMessage.newBuilder().setBb(218).build());
    builder.addRepeatedForeignMessage(ForeignMessageLite.newBuilder().setC(219).build());
    builder.addRepeatedImportMessage(ImportMessageLite.newBuilder().setD(220).build());
    builder.addRepeatedLazyMessage(TestAllTypesLite.NestedMessage.newBuilder().setBb(227).build());

    builder.addRepeatedNestedEnum(TestAllTypesLite.NestedEnum.BAR);
    builder.addRepeatedForeignEnum(ForeignEnumLite.FOREIGN_LITE_BAR);
    builder.addRepeatedImportEnum(ImportEnumLite.IMPORT_LITE_BAR);

    builder.addRepeatedStringPiece("224");
    builder.addRepeatedCord("225");

    // Add a second one of each field.
    builder.addRepeatedInt32(301);
    builder.addRepeatedInt64(302);
    builder.addRepeatedUint32(303);
    builder.addRepeatedUint64(304);
    builder.addRepeatedSint32(305);
    builder.addRepeatedSint64(306);
    builder.addRepeatedFixed32(307);
    builder.addRepeatedFixed64(308);
    builder.addRepeatedSfixed32(309);
    builder.addRepeatedSfixed64(310);
    builder.addRepeatedFloat(311);
    builder.addRepeatedDouble(312);
    builder.addRepeatedBool(false);
    builder.addRepeatedString("315");
    builder.addRepeatedBytes(toBytes("316"));

    builder.addRepeatedGroup(TestAllTypesLite.RepeatedGroup.newBuilder().setA(317).build());
    builder.addRepeatedNestedMessage(
        TestAllTypesLite.NestedMessage.newBuilder().setBb(318).build());
    builder.addRepeatedForeignMessage(ForeignMessageLite.newBuilder().setC(319).build());
    builder.addRepeatedImportMessage(ImportMessageLite.newBuilder().setD(320).build());
    builder.addRepeatedLazyMessage(TestAllTypesLite.NestedMessage.newBuilder().setBb(327).build());

    builder.addRepeatedNestedEnum(TestAllTypesLite.NestedEnum.BAZ);
    builder.addRepeatedForeignEnum(ForeignEnumLite.FOREIGN_LITE_BAZ);
    builder.addRepeatedImportEnum(ImportEnumLite.IMPORT_LITE_BAZ);

    builder.addRepeatedStringPiece("324");
    builder.addRepeatedCord("325");

    // -----------------------------------------------------------------

    builder.setDefaultInt32(401);
    builder.setDefaultInt64(402);
    builder.setDefaultUint32(403);
    builder.setDefaultUint64(404);
    builder.setDefaultSint32(405);
    builder.setDefaultSint64(406);
    builder.setDefaultFixed32(407);
    builder.setDefaultFixed64(408);
    builder.setDefaultSfixed32(409);
    builder.setDefaultSfixed64(410);
    builder.setDefaultFloat(411);
    builder.setDefaultDouble(412);
    builder.setDefaultBool(false);
    builder.setDefaultString("415");
    builder.setDefaultBytes(toBytes("416"));

    builder.setDefaultNestedEnum(TestAllTypesLite.NestedEnum.FOO);
    builder.setDefaultForeignEnum(ForeignEnumLite.FOREIGN_LITE_FOO);
    builder.setDefaultImportEnum(ImportEnumLite.IMPORT_LITE_FOO);

    builder.setDefaultStringPiece("424");
    builder.setDefaultCord("425");

    builder.setOneofUint32(601);
    builder.setOneofNestedMessage(TestAllTypesLite.NestedMessage.newBuilder().setBb(602).build());
    builder.setOneofString("603");
    builder.setOneofBytes(toBytes("604"));
  }

  /**
   * Get an unmodifiable {@link ExtensionRegistryLite} containing all the extensions of {@code
   * TestAllExtensionsLite}.
   */
  public static ExtensionRegistryLite getExtensionRegistryLite() {
    ExtensionRegistryLite registry = ExtensionRegistryLite.newInstance();
    registerAllExtensionsLite(registry);
    return registry.getUnmodifiable();
  }

  /**
   * Register all of {@code TestAllExtensionsLite}'s extensions with the given {@link
   * ExtensionRegistryLite}.
   */
  public static void registerAllExtensionsLite(ExtensionRegistryLite registry) {
    UnittestLite.registerAllExtensions(registry);
  }

  // ===================================================================
  // Lite extensions

  /**
   * Set every field of {@code message} to the values expected by {@code assertAllExtensionsSet()}.
   */
  public static void setAllExtensions(TestAllExtensionsLite.Builder message) {
    message.setExtension(optionalInt32ExtensionLite, 101);
    message.setExtension(optionalInt64ExtensionLite, 102L);
    message.setExtension(optionalUint32ExtensionLite, 103);
    message.setExtension(optionalUint64ExtensionLite, 104L);
    message.setExtension(optionalSint32ExtensionLite, 105);
    message.setExtension(optionalSint64ExtensionLite, 106L);
    message.setExtension(optionalFixed32ExtensionLite, 107);
    message.setExtension(optionalFixed64ExtensionLite, 108L);
    message.setExtension(optionalSfixed32ExtensionLite, 109);
    message.setExtension(optionalSfixed64ExtensionLite, 110L);
    message.setExtension(optionalFloatExtensionLite, 111F);
    message.setExtension(optionalDoubleExtensionLite, 112D);
    message.setExtension(optionalBoolExtensionLite, true);
    message.setExtension(optionalStringExtensionLite, "115");
    message.setExtension(optionalBytesExtensionLite, toBytes("116"));

    message.setExtension(
        optionalGroupExtensionLite, OptionalGroup_extension_lite.newBuilder().setA(117).build());
    message.setExtension(
        optionalNestedMessageExtensionLite,
        TestAllTypesLite.NestedMessage.newBuilder().setBb(118).build());
    message.setExtension(
        optionalForeignMessageExtensionLite, ForeignMessageLite.newBuilder().setC(119).build());
    message.setExtension(
        optionalImportMessageExtensionLite, ImportMessageLite.newBuilder().setD(120).build());
    message.setExtension(
        optionalPublicImportMessageExtensionLite,
        PublicImportMessageLite.newBuilder().setE(126).build());
    message.setExtension(
        optionalLazyMessageExtensionLite,
        TestAllTypesLite.NestedMessage.newBuilder().setBb(127).build());
    message.setExtension(
        optionalUnverifiedLazyMessageExtensionLite,
        TestAllTypesLite.NestedMessage.newBuilder().setBb(128).build());

    message.setExtension(optionalNestedEnumExtensionLite, TestAllTypesLite.NestedEnum.BAZ);
    message.setExtension(optionalForeignEnumExtensionLite, ForeignEnumLite.FOREIGN_LITE_BAZ);
    message.setExtension(optionalImportEnumExtensionLite, ImportEnumLite.IMPORT_LITE_BAZ);

    message.setExtension(optionalStringPieceExtensionLite, "124");
    message.setExtension(optionalCordExtensionLite, "125");

    // -----------------------------------------------------------------

    message.addExtension(repeatedInt32ExtensionLite, 201);
    message.addExtension(repeatedInt64ExtensionLite, 202L);
    message.addExtension(repeatedUint32ExtensionLite, 203);
    message.addExtension(repeatedUint64ExtensionLite, 204L);
    message.addExtension(repeatedSint32ExtensionLite, 205);
    message.addExtension(repeatedSint64ExtensionLite, 206L);
    message.addExtension(repeatedFixed32ExtensionLite, 207);
    message.addExtension(repeatedFixed64ExtensionLite, 208L);
    message.addExtension(repeatedSfixed32ExtensionLite, 209);
    message.addExtension(repeatedSfixed64ExtensionLite, 210L);
    message.addExtension(repeatedFloatExtensionLite, 211F);
    message.addExtension(repeatedDoubleExtensionLite, 212D);
    message.addExtension(repeatedBoolExtensionLite, true);
    message.addExtension(repeatedStringExtensionLite, "215");
    message.addExtension(repeatedBytesExtensionLite, toBytes("216"));

    message.addExtension(
        repeatedGroupExtensionLite, RepeatedGroup_extension_lite.newBuilder().setA(217).build());
    message.addExtension(
        repeatedNestedMessageExtensionLite,
        TestAllTypesLite.NestedMessage.newBuilder().setBb(218).build());
    message.addExtension(
        repeatedForeignMessageExtensionLite, ForeignMessageLite.newBuilder().setC(219).build());
    message.addExtension(
        repeatedImportMessageExtensionLite, ImportMessageLite.newBuilder().setD(220).build());
    message.addExtension(
        repeatedLazyMessageExtensionLite,
        TestAllTypesLite.NestedMessage.newBuilder().setBb(227).build());

    message.addExtension(repeatedNestedEnumExtensionLite, TestAllTypesLite.NestedEnum.BAR);
    message.addExtension(repeatedForeignEnumExtensionLite, ForeignEnumLite.FOREIGN_LITE_BAR);
    message.addExtension(repeatedImportEnumExtensionLite, ImportEnumLite.IMPORT_LITE_BAR);

    message.addExtension(repeatedStringPieceExtensionLite, "224");
    message.addExtension(repeatedCordExtensionLite, "225");

    // Add a second one of each field.
    message.addExtension(repeatedInt32ExtensionLite, 301);
    message.addExtension(repeatedInt64ExtensionLite, 302L);
    message.addExtension(repeatedUint32ExtensionLite, 303);
    message.addExtension(repeatedUint64ExtensionLite, 304L);
    message.addExtension(repeatedSint32ExtensionLite, 305);
    message.addExtension(repeatedSint64ExtensionLite, 306L);
    message.addExtension(repeatedFixed32ExtensionLite, 307);
    message.addExtension(repeatedFixed64ExtensionLite, 308L);
    message.addExtension(repeatedSfixed32ExtensionLite, 309);
    message.addExtension(repeatedSfixed64ExtensionLite, 310L);
    message.addExtension(repeatedFloatExtensionLite, 311F);
    message.addExtension(repeatedDoubleExtensionLite, 312D);
    message.addExtension(repeatedBoolExtensionLite, false);
    message.addExtension(repeatedStringExtensionLite, "315");
    message.addExtension(repeatedBytesExtensionLite, toBytes("316"));

    message.addExtension(
        repeatedGroupExtensionLite, RepeatedGroup_extension_lite.newBuilder().setA(317).build());
    message.addExtension(
        repeatedNestedMessageExtensionLite,
        TestAllTypesLite.NestedMessage.newBuilder().setBb(318).build());
    message.addExtension(
        repeatedForeignMessageExtensionLite, ForeignMessageLite.newBuilder().setC(319).build());
    message.addExtension(
        repeatedImportMessageExtensionLite, ImportMessageLite.newBuilder().setD(320).build());
    message.addExtension(
        repeatedLazyMessageExtensionLite,
        TestAllTypesLite.NestedMessage.newBuilder().setBb(327).build());

    message.addExtension(repeatedNestedEnumExtensionLite, TestAllTypesLite.NestedEnum.BAZ);
    message.addExtension(repeatedForeignEnumExtensionLite, ForeignEnumLite.FOREIGN_LITE_BAZ);
    message.addExtension(repeatedImportEnumExtensionLite, ImportEnumLite.IMPORT_LITE_BAZ);

    message.addExtension(repeatedStringPieceExtensionLite, "324");
    message.addExtension(repeatedCordExtensionLite, "325");

    // -----------------------------------------------------------------

    message.setExtension(defaultInt32ExtensionLite, 401);
    message.setExtension(defaultInt64ExtensionLite, 402L);
    message.setExtension(defaultUint32ExtensionLite, 403);
    message.setExtension(defaultUint64ExtensionLite, 404L);
    message.setExtension(defaultSint32ExtensionLite, 405);
    message.setExtension(defaultSint64ExtensionLite, 406L);
    message.setExtension(defaultFixed32ExtensionLite, 407);
    message.setExtension(defaultFixed64ExtensionLite, 408L);
    message.setExtension(defaultSfixed32ExtensionLite, 409);
    message.setExtension(defaultSfixed64ExtensionLite, 410L);
    message.setExtension(defaultFloatExtensionLite, 411F);
    message.setExtension(defaultDoubleExtensionLite, 412D);
    message.setExtension(defaultBoolExtensionLite, false);
    message.setExtension(defaultStringExtensionLite, "415");
    message.setExtension(defaultBytesExtensionLite, toBytes("416"));

    message.setExtension(defaultNestedEnumExtensionLite, TestAllTypesLite.NestedEnum.FOO);
    message.setExtension(defaultForeignEnumExtensionLite, ForeignEnumLite.FOREIGN_LITE_FOO);
    message.setExtension(defaultImportEnumExtensionLite, ImportEnumLite.IMPORT_LITE_FOO);

    message.setExtension(defaultStringPieceExtensionLite, "424");
    message.setExtension(defaultCordExtensionLite, "425");

    message.setExtension(oneofUint32ExtensionLite, 601);
    message.setExtension(
        oneofNestedMessageExtensionLite,
        TestAllTypesLite.NestedMessage.newBuilder().setBb(602).build());
    message.setExtension(oneofStringExtensionLite, "603");
    message.setExtension(oneofBytesExtensionLite, toBytes("604"));
  }

  // -------------------------------------------------------------------

  /**
   * Modify the repeated extensions of {@code message} to contain the values expected by {@code
   * assertRepeatedExtensionsModified()}.
   */
  public static void modifyRepeatedExtensions(TestAllExtensionsLite.Builder message) {
    message.setExtension(repeatedInt32ExtensionLite, 1, 501);
    message.setExtension(repeatedInt64ExtensionLite, 1, 502L);
    message.setExtension(repeatedUint32ExtensionLite, 1, 503);
    message.setExtension(repeatedUint64ExtensionLite, 1, 504L);
    message.setExtension(repeatedSint32ExtensionLite, 1, 505);
    message.setExtension(repeatedSint64ExtensionLite, 1, 506L);
    message.setExtension(repeatedFixed32ExtensionLite, 1, 507);
    message.setExtension(repeatedFixed64ExtensionLite, 1, 508L);
    message.setExtension(repeatedSfixed32ExtensionLite, 1, 509);
    message.setExtension(repeatedSfixed64ExtensionLite, 1, 510L);
    message.setExtension(repeatedFloatExtensionLite, 1, 511F);
    message.setExtension(repeatedDoubleExtensionLite, 1, 512D);
    message.setExtension(repeatedBoolExtensionLite, 1, true);
    message.setExtension(repeatedStringExtensionLite, 1, "515");
    message.setExtension(repeatedBytesExtensionLite, 1, toBytes("516"));

    message.setExtension(
        repeatedGroupExtensionLite, 1, RepeatedGroup_extension_lite.newBuilder().setA(517).build());
    message.setExtension(
        repeatedNestedMessageExtensionLite,
        1,
        TestAllTypesLite.NestedMessage.newBuilder().setBb(518).build());
    message.setExtension(
        repeatedForeignMessageExtensionLite, 1, ForeignMessageLite.newBuilder().setC(519).build());
    message.setExtension(
        repeatedImportMessageExtensionLite, 1, ImportMessageLite.newBuilder().setD(520).build());
    message.setExtension(
        repeatedLazyMessageExtensionLite,
        1,
        TestAllTypesLite.NestedMessage.newBuilder().setBb(527).build());

    message.setExtension(repeatedNestedEnumExtensionLite, 1, TestAllTypesLite.NestedEnum.FOO);
    message.setExtension(repeatedForeignEnumExtensionLite, 1, ForeignEnumLite.FOREIGN_LITE_FOO);
    message.setExtension(repeatedImportEnumExtensionLite, 1, ImportEnumLite.IMPORT_LITE_FOO);

    message.setExtension(repeatedStringPieceExtensionLite, 1, "524");
    message.setExtension(repeatedCordExtensionLite, 1, "525");
  }

  public static void setPackedExtensions(TestPackedExtensionsLite.Builder message) {
    message.addExtension(packedInt32ExtensionLite, 601);
    message.addExtension(packedInt64ExtensionLite, 602L);
    message.addExtension(packedUint32ExtensionLite, 603);
    message.addExtension(packedUint64ExtensionLite, 604L);
    message.addExtension(packedSint32ExtensionLite, 605);
    message.addExtension(packedSint64ExtensionLite, 606L);
    message.addExtension(packedFixed32ExtensionLite, 607);
    message.addExtension(packedFixed64ExtensionLite, 608L);
    message.addExtension(packedSfixed32ExtensionLite, 609);
    message.addExtension(packedSfixed64ExtensionLite, 610L);
    message.addExtension(packedFloatExtensionLite, 611F);
    message.addExtension(packedDoubleExtensionLite, 612D);
    message.addExtension(packedBoolExtensionLite, true);
    message.addExtension(packedEnumExtensionLite, ForeignEnumLite.FOREIGN_LITE_BAR);
    // Add a second one of each field.
    message.addExtension(packedInt32ExtensionLite, 701);
    message.addExtension(packedInt64ExtensionLite, 702L);
    message.addExtension(packedUint32ExtensionLite, 703);
    message.addExtension(packedUint64ExtensionLite, 704L);
    message.addExtension(packedSint32ExtensionLite, 705);
    message.addExtension(packedSint64ExtensionLite, 706L);
    message.addExtension(packedFixed32ExtensionLite, 707);
    message.addExtension(packedFixed64ExtensionLite, 708L);
    message.addExtension(packedSfixed32ExtensionLite, 709);
    message.addExtension(packedSfixed64ExtensionLite, 710L);
    message.addExtension(packedFloatExtensionLite, 711F);
    message.addExtension(packedDoubleExtensionLite, 712D);
    message.addExtension(packedBoolExtensionLite, false);
    message.addExtension(packedEnumExtensionLite, ForeignEnumLite.FOREIGN_LITE_BAZ);
  }
}
