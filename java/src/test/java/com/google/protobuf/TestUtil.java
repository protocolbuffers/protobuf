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

// Note:  This file contains many lines over 80 characters.  It even contains
// many lines over 100 characters, which fails a presubmit test.  However,
// given the extremely repetitive nature of the file, I (kenton) feel that
// having similar components of each statement line up is more important than
// avoiding horizontal scrolling.  So, I am bypassing the presubmit check.

package com.google.protobuf;

import protobuf_unittest.UnittestProto;
import protobuf_unittest.UnittestProto.TestAllTypes;
import protobuf_unittest.UnittestProto.TestAllExtensions;
import protobuf_unittest.UnittestProto.ForeignMessage;
import protobuf_unittest.UnittestProto.ForeignEnum;
import com.google.protobuf.test.UnittestImport.ImportMessage;
import com.google.protobuf.test.UnittestImport.ImportEnum;

import junit.framework.Assert;

import java.io.File;
import java.io.IOException;
import java.io.RandomAccessFile;

/**
 * Contains methods for setting all fields of {@code TestAllTypes} to
 * some vaules as well as checking that all the fields are set to those values.
 * These are useful for testing various protocol message features, e.g.
 * set all fields of a message, serialize it, parse it, and check that all
 * fields are set.
 *
 * @author kenton@google.com Kenton Varda
 */
class TestUtil {
  private TestUtil() {}

  /** Helper to convert a String to ByteString. */
  private static ByteString toBytes(String str) {
    try {
      return ByteString.copyFrom(str.getBytes("UTF-8"));
    } catch(java.io.UnsupportedEncodingException e) {
      throw new RuntimeException("UTF-8 not supported.", e);
    }
  }

  /**
   * Get a {@code TestAllTypes} with all fields set as they would be by
   * {@link #setAllFields(TestAllTypes.Builder)}.
   */
  public static TestAllTypes getAllSet() {
    TestAllTypes.Builder builder = TestAllTypes.newBuilder();
    setAllFields(builder);
    return builder.build();
  }

  /**
   * Get a {@code TestAllExtensions} with all fields set as they would be by
   * {@link #setAllExtensions(TestAllExtensions.Builder)}.
   */
  public static TestAllExtensions getAllExtensionsSet() {
    TestAllExtensions.Builder builder = TestAllExtensions.newBuilder();
    setAllExtensions(builder);
    return builder.build();
  }

  /**
   * Set every field of {@code message} to the values expected by
   * {@code assertAllFieldsSet()}.
   */
  public static void setAllFields(TestAllTypes.Builder message) {
    message.setOptionalInt32   (101);
    message.setOptionalInt64   (102);
    message.setOptionalUint32  (103);
    message.setOptionalUint64  (104);
    message.setOptionalSint32  (105);
    message.setOptionalSint64  (106);
    message.setOptionalFixed32 (107);
    message.setOptionalFixed64 (108);
    message.setOptionalSfixed32(109);
    message.setOptionalSfixed64(110);
    message.setOptionalFloat   (111);
    message.setOptionalDouble  (112);
    message.setOptionalBool    (true);
    message.setOptionalString  ("115");
    message.setOptionalBytes   (toBytes("116"));

    message.setOptionalGroup(
      TestAllTypes.OptionalGroup.newBuilder().setA(117).build());
    message.setOptionalNestedMessage(
      TestAllTypes.NestedMessage.newBuilder().setBb(118).build());
    message.setOptionalForeignMessage(
      ForeignMessage.newBuilder().setC(119).build());
    message.setOptionalImportMessage(
      ImportMessage.newBuilder().setD(120).build());

    message.setOptionalNestedEnum (TestAllTypes.NestedEnum.BAZ);
    message.setOptionalForeignEnum(ForeignEnum.FOREIGN_BAZ);
    message.setOptionalImportEnum (ImportEnum.IMPORT_BAZ);

    message.setOptionalStringPiece("124");
    message.setOptionalCord("125");

    // -----------------------------------------------------------------

    message.addRepeatedInt32   (201);
    message.addRepeatedInt64   (202);
    message.addRepeatedUint32  (203);
    message.addRepeatedUint64  (204);
    message.addRepeatedSint32  (205);
    message.addRepeatedSint64  (206);
    message.addRepeatedFixed32 (207);
    message.addRepeatedFixed64 (208);
    message.addRepeatedSfixed32(209);
    message.addRepeatedSfixed64(210);
    message.addRepeatedFloat   (211);
    message.addRepeatedDouble  (212);
    message.addRepeatedBool    (true);
    message.addRepeatedString  ("215");
    message.addRepeatedBytes   (toBytes("216"));

    message.addRepeatedGroup(
      TestAllTypes.RepeatedGroup.newBuilder().setA(217).build());
    message.addRepeatedNestedMessage(
      TestAllTypes.NestedMessage.newBuilder().setBb(218).build());
    message.addRepeatedForeignMessage(
      ForeignMessage.newBuilder().setC(219).build());
    message.addRepeatedImportMessage(
      ImportMessage.newBuilder().setD(220).build());

    message.addRepeatedNestedEnum (TestAllTypes.NestedEnum.BAR);
    message.addRepeatedForeignEnum(ForeignEnum.FOREIGN_BAR);
    message.addRepeatedImportEnum (ImportEnum.IMPORT_BAR);

    message.addRepeatedStringPiece("224");
    message.addRepeatedCord("225");

    // Add a second one of each field.
    message.addRepeatedInt32   (301);
    message.addRepeatedInt64   (302);
    message.addRepeatedUint32  (303);
    message.addRepeatedUint64  (304);
    message.addRepeatedSint32  (305);
    message.addRepeatedSint64  (306);
    message.addRepeatedFixed32 (307);
    message.addRepeatedFixed64 (308);
    message.addRepeatedSfixed32(309);
    message.addRepeatedSfixed64(310);
    message.addRepeatedFloat   (311);
    message.addRepeatedDouble  (312);
    message.addRepeatedBool    (false);
    message.addRepeatedString  ("315");
    message.addRepeatedBytes   (toBytes("316"));

    message.addRepeatedGroup(
      TestAllTypes.RepeatedGroup.newBuilder().setA(317).build());
    message.addRepeatedNestedMessage(
      TestAllTypes.NestedMessage.newBuilder().setBb(318).build());
    message.addRepeatedForeignMessage(
      ForeignMessage.newBuilder().setC(319).build());
    message.addRepeatedImportMessage(
      ImportMessage.newBuilder().setD(320).build());

    message.addRepeatedNestedEnum (TestAllTypes.NestedEnum.BAZ);
    message.addRepeatedForeignEnum(ForeignEnum.FOREIGN_BAZ);
    message.addRepeatedImportEnum (ImportEnum.IMPORT_BAZ);

    message.addRepeatedStringPiece("324");
    message.addRepeatedCord("325");

    // -----------------------------------------------------------------

    message.setDefaultInt32   (401);
    message.setDefaultInt64   (402);
    message.setDefaultUint32  (403);
    message.setDefaultUint64  (404);
    message.setDefaultSint32  (405);
    message.setDefaultSint64  (406);
    message.setDefaultFixed32 (407);
    message.setDefaultFixed64 (408);
    message.setDefaultSfixed32(409);
    message.setDefaultSfixed64(410);
    message.setDefaultFloat   (411);
    message.setDefaultDouble  (412);
    message.setDefaultBool    (false);
    message.setDefaultString  ("415");
    message.setDefaultBytes   (toBytes("416"));

    message.setDefaultNestedEnum (TestAllTypes.NestedEnum.FOO);
    message.setDefaultForeignEnum(ForeignEnum.FOREIGN_FOO);
    message.setDefaultImportEnum (ImportEnum.IMPORT_FOO);

    message.setDefaultStringPiece("424");
    message.setDefaultCord("425");
  }

  // -------------------------------------------------------------------

  /**
   * Modify the repeated fields of {@code message} to contain the values
   * expected by {@code assertRepeatedFieldsModified()}.
   */
  public static void modifyRepeatedFields(TestAllTypes.Builder message) {
    message.setRepeatedInt32   (1, 501);
    message.setRepeatedInt64   (1, 502);
    message.setRepeatedUint32  (1, 503);
    message.setRepeatedUint64  (1, 504);
    message.setRepeatedSint32  (1, 505);
    message.setRepeatedSint64  (1, 506);
    message.setRepeatedFixed32 (1, 507);
    message.setRepeatedFixed64 (1, 508);
    message.setRepeatedSfixed32(1, 509);
    message.setRepeatedSfixed64(1, 510);
    message.setRepeatedFloat   (1, 511);
    message.setRepeatedDouble  (1, 512);
    message.setRepeatedBool    (1, true);
    message.setRepeatedString  (1, "515");
    message.setRepeatedBytes   (1, toBytes("516"));

    message.setRepeatedGroup(1,
      TestAllTypes.RepeatedGroup.newBuilder().setA(517).build());
    message.setRepeatedNestedMessage(1,
      TestAllTypes.NestedMessage.newBuilder().setBb(518).build());
    message.setRepeatedForeignMessage(1,
      ForeignMessage.newBuilder().setC(519).build());
    message.setRepeatedImportMessage(1,
      ImportMessage.newBuilder().setD(520).build());

    message.setRepeatedNestedEnum (1, TestAllTypes.NestedEnum.FOO);
    message.setRepeatedForeignEnum(1, ForeignEnum.FOREIGN_FOO);
    message.setRepeatedImportEnum (1, ImportEnum.IMPORT_FOO);

    message.setRepeatedStringPiece(1, "524");
    message.setRepeatedCord(1, "525");
  }

  // -------------------------------------------------------------------

  /**
   * Assert (using {@code junit.framework.Assert}} that all fields of
   * {@code message} are set to the values assigned by {@code setAllFields}.
   */
  public static void assertAllFieldsSet(TestAllTypes message) {
    Assert.assertTrue(message.hasOptionalInt32   ());
    Assert.assertTrue(message.hasOptionalInt64   ());
    Assert.assertTrue(message.hasOptionalUint32  ());
    Assert.assertTrue(message.hasOptionalUint64  ());
    Assert.assertTrue(message.hasOptionalSint32  ());
    Assert.assertTrue(message.hasOptionalSint64  ());
    Assert.assertTrue(message.hasOptionalFixed32 ());
    Assert.assertTrue(message.hasOptionalFixed64 ());
    Assert.assertTrue(message.hasOptionalSfixed32());
    Assert.assertTrue(message.hasOptionalSfixed64());
    Assert.assertTrue(message.hasOptionalFloat   ());
    Assert.assertTrue(message.hasOptionalDouble  ());
    Assert.assertTrue(message.hasOptionalBool    ());
    Assert.assertTrue(message.hasOptionalString  ());
    Assert.assertTrue(message.hasOptionalBytes   ());

    Assert.assertTrue(message.hasOptionalGroup         ());
    Assert.assertTrue(message.hasOptionalNestedMessage ());
    Assert.assertTrue(message.hasOptionalForeignMessage());
    Assert.assertTrue(message.hasOptionalImportMessage ());

    Assert.assertTrue(message.getOptionalGroup         ().hasA());
    Assert.assertTrue(message.getOptionalNestedMessage ().hasBb());
    Assert.assertTrue(message.getOptionalForeignMessage().hasC());
    Assert.assertTrue(message.getOptionalImportMessage ().hasD());

    Assert.assertTrue(message.hasOptionalNestedEnum ());
    Assert.assertTrue(message.hasOptionalForeignEnum());
    Assert.assertTrue(message.hasOptionalImportEnum ());

    Assert.assertTrue(message.hasOptionalStringPiece());
    Assert.assertTrue(message.hasOptionalCord());

    Assert.assertEquals(101  , message.getOptionalInt32   ());
    Assert.assertEquals(102  , message.getOptionalInt64   ());
    Assert.assertEquals(103  , message.getOptionalUint32  ());
    Assert.assertEquals(104  , message.getOptionalUint64  ());
    Assert.assertEquals(105  , message.getOptionalSint32  ());
    Assert.assertEquals(106  , message.getOptionalSint64  ());
    Assert.assertEquals(107  , message.getOptionalFixed32 ());
    Assert.assertEquals(108  , message.getOptionalFixed64 ());
    Assert.assertEquals(109  , message.getOptionalSfixed32());
    Assert.assertEquals(110  , message.getOptionalSfixed64());
    Assert.assertEquals(111  , message.getOptionalFloat   (), 0.0);
    Assert.assertEquals(112  , message.getOptionalDouble  (), 0.0);
    Assert.assertEquals(true , message.getOptionalBool    ());
    Assert.assertEquals("115", message.getOptionalString  ());
    Assert.assertEquals(toBytes("116"), message.getOptionalBytes());

    Assert.assertEquals(117, message.getOptionalGroup         ().getA());
    Assert.assertEquals(118, message.getOptionalNestedMessage ().getBb());
    Assert.assertEquals(119, message.getOptionalForeignMessage().getC());
    Assert.assertEquals(120, message.getOptionalImportMessage ().getD());

    Assert.assertEquals(TestAllTypes.NestedEnum.BAZ, message.getOptionalNestedEnum());
    Assert.assertEquals(ForeignEnum.FOREIGN_BAZ, message.getOptionalForeignEnum());
    Assert.assertEquals(ImportEnum.IMPORT_BAZ, message.getOptionalImportEnum());

    Assert.assertEquals("124", message.getOptionalStringPiece());
    Assert.assertEquals("125", message.getOptionalCord());

    // -----------------------------------------------------------------

    Assert.assertEquals(2, message.getRepeatedInt32Count   ());
    Assert.assertEquals(2, message.getRepeatedInt64Count   ());
    Assert.assertEquals(2, message.getRepeatedUint32Count  ());
    Assert.assertEquals(2, message.getRepeatedUint64Count  ());
    Assert.assertEquals(2, message.getRepeatedSint32Count  ());
    Assert.assertEquals(2, message.getRepeatedSint64Count  ());
    Assert.assertEquals(2, message.getRepeatedFixed32Count ());
    Assert.assertEquals(2, message.getRepeatedFixed64Count ());
    Assert.assertEquals(2, message.getRepeatedSfixed32Count());
    Assert.assertEquals(2, message.getRepeatedSfixed64Count());
    Assert.assertEquals(2, message.getRepeatedFloatCount   ());
    Assert.assertEquals(2, message.getRepeatedDoubleCount  ());
    Assert.assertEquals(2, message.getRepeatedBoolCount    ());
    Assert.assertEquals(2, message.getRepeatedStringCount  ());
    Assert.assertEquals(2, message.getRepeatedBytesCount   ());

    Assert.assertEquals(2, message.getRepeatedGroupCount         ());
    Assert.assertEquals(2, message.getRepeatedNestedMessageCount ());
    Assert.assertEquals(2, message.getRepeatedForeignMessageCount());
    Assert.assertEquals(2, message.getRepeatedImportMessageCount ());
    Assert.assertEquals(2, message.getRepeatedNestedEnumCount    ());
    Assert.assertEquals(2, message.getRepeatedForeignEnumCount   ());
    Assert.assertEquals(2, message.getRepeatedImportEnumCount    ());

    Assert.assertEquals(2, message.getRepeatedStringPieceCount());
    Assert.assertEquals(2, message.getRepeatedCordCount());

    Assert.assertEquals(201  , message.getRepeatedInt32   (0));
    Assert.assertEquals(202  , message.getRepeatedInt64   (0));
    Assert.assertEquals(203  , message.getRepeatedUint32  (0));
    Assert.assertEquals(204  , message.getRepeatedUint64  (0));
    Assert.assertEquals(205  , message.getRepeatedSint32  (0));
    Assert.assertEquals(206  , message.getRepeatedSint64  (0));
    Assert.assertEquals(207  , message.getRepeatedFixed32 (0));
    Assert.assertEquals(208  , message.getRepeatedFixed64 (0));
    Assert.assertEquals(209  , message.getRepeatedSfixed32(0));
    Assert.assertEquals(210  , message.getRepeatedSfixed64(0));
    Assert.assertEquals(211  , message.getRepeatedFloat   (0), 0.0);
    Assert.assertEquals(212  , message.getRepeatedDouble  (0), 0.0);
    Assert.assertEquals(true , message.getRepeatedBool    (0));
    Assert.assertEquals("215", message.getRepeatedString  (0));
    Assert.assertEquals(toBytes("216"), message.getRepeatedBytes(0));

    Assert.assertEquals(217, message.getRepeatedGroup         (0).getA());
    Assert.assertEquals(218, message.getRepeatedNestedMessage (0).getBb());
    Assert.assertEquals(219, message.getRepeatedForeignMessage(0).getC());
    Assert.assertEquals(220, message.getRepeatedImportMessage (0).getD());

    Assert.assertEquals(TestAllTypes.NestedEnum.BAR, message.getRepeatedNestedEnum (0));
    Assert.assertEquals(ForeignEnum.FOREIGN_BAR, message.getRepeatedForeignEnum(0));
    Assert.assertEquals(ImportEnum.IMPORT_BAR, message.getRepeatedImportEnum(0));

    Assert.assertEquals("224", message.getRepeatedStringPiece(0));
    Assert.assertEquals("225", message.getRepeatedCord(0));

    Assert.assertEquals(301  , message.getRepeatedInt32   (1));
    Assert.assertEquals(302  , message.getRepeatedInt64   (1));
    Assert.assertEquals(303  , message.getRepeatedUint32  (1));
    Assert.assertEquals(304  , message.getRepeatedUint64  (1));
    Assert.assertEquals(305  , message.getRepeatedSint32  (1));
    Assert.assertEquals(306  , message.getRepeatedSint64  (1));
    Assert.assertEquals(307  , message.getRepeatedFixed32 (1));
    Assert.assertEquals(308  , message.getRepeatedFixed64 (1));
    Assert.assertEquals(309  , message.getRepeatedSfixed32(1));
    Assert.assertEquals(310  , message.getRepeatedSfixed64(1));
    Assert.assertEquals(311  , message.getRepeatedFloat   (1), 0.0);
    Assert.assertEquals(312  , message.getRepeatedDouble  (1), 0.0);
    Assert.assertEquals(false, message.getRepeatedBool    (1));
    Assert.assertEquals("315", message.getRepeatedString  (1));
    Assert.assertEquals(toBytes("316"), message.getRepeatedBytes(1));

    Assert.assertEquals(317, message.getRepeatedGroup         (1).getA());
    Assert.assertEquals(318, message.getRepeatedNestedMessage (1).getBb());
    Assert.assertEquals(319, message.getRepeatedForeignMessage(1).getC());
    Assert.assertEquals(320, message.getRepeatedImportMessage (1).getD());

    Assert.assertEquals(TestAllTypes.NestedEnum.BAZ, message.getRepeatedNestedEnum (1));
    Assert.assertEquals(ForeignEnum.FOREIGN_BAZ, message.getRepeatedForeignEnum(1));
    Assert.assertEquals(ImportEnum.IMPORT_BAZ, message.getRepeatedImportEnum(1));

    Assert.assertEquals("324", message.getRepeatedStringPiece(1));
    Assert.assertEquals("325", message.getRepeatedCord(1));

    // -----------------------------------------------------------------

    Assert.assertTrue(message.hasDefaultInt32   ());
    Assert.assertTrue(message.hasDefaultInt64   ());
    Assert.assertTrue(message.hasDefaultUint32  ());
    Assert.assertTrue(message.hasDefaultUint64  ());
    Assert.assertTrue(message.hasDefaultSint32  ());
    Assert.assertTrue(message.hasDefaultSint64  ());
    Assert.assertTrue(message.hasDefaultFixed32 ());
    Assert.assertTrue(message.hasDefaultFixed64 ());
    Assert.assertTrue(message.hasDefaultSfixed32());
    Assert.assertTrue(message.hasDefaultSfixed64());
    Assert.assertTrue(message.hasDefaultFloat   ());
    Assert.assertTrue(message.hasDefaultDouble  ());
    Assert.assertTrue(message.hasDefaultBool    ());
    Assert.assertTrue(message.hasDefaultString  ());
    Assert.assertTrue(message.hasDefaultBytes   ());

    Assert.assertTrue(message.hasDefaultNestedEnum ());
    Assert.assertTrue(message.hasDefaultForeignEnum());
    Assert.assertTrue(message.hasDefaultImportEnum ());

    Assert.assertTrue(message.hasDefaultStringPiece());
    Assert.assertTrue(message.hasDefaultCord());

    Assert.assertEquals(401  , message.getDefaultInt32   ());
    Assert.assertEquals(402  , message.getDefaultInt64   ());
    Assert.assertEquals(403  , message.getDefaultUint32  ());
    Assert.assertEquals(404  , message.getDefaultUint64  ());
    Assert.assertEquals(405  , message.getDefaultSint32  ());
    Assert.assertEquals(406  , message.getDefaultSint64  ());
    Assert.assertEquals(407  , message.getDefaultFixed32 ());
    Assert.assertEquals(408  , message.getDefaultFixed64 ());
    Assert.assertEquals(409  , message.getDefaultSfixed32());
    Assert.assertEquals(410  , message.getDefaultSfixed64());
    Assert.assertEquals(411  , message.getDefaultFloat   (), 0.0);
    Assert.assertEquals(412  , message.getDefaultDouble  (), 0.0);
    Assert.assertEquals(false, message.getDefaultBool    ());
    Assert.assertEquals("415", message.getDefaultString  ());
    Assert.assertEquals(toBytes("416"), message.getDefaultBytes());

    Assert.assertEquals(TestAllTypes.NestedEnum.FOO, message.getDefaultNestedEnum ());
    Assert.assertEquals(ForeignEnum.FOREIGN_FOO, message.getDefaultForeignEnum());
    Assert.assertEquals(ImportEnum.IMPORT_FOO, message.getDefaultImportEnum());

    Assert.assertEquals("424", message.getDefaultStringPiece());
    Assert.assertEquals("425", message.getDefaultCord());
  }

  // -------------------------------------------------------------------

  /**
   * Assert (using {@code junit.framework.Assert}} that all fields of
   * {@code message} are cleared, and that getting the fields returns their
   * default values.
   */
  public static void assertClear(TestAllTypes message) {
    // hasBlah() should initially be false for all optional fields.
    Assert.assertFalse(message.hasOptionalInt32   ());
    Assert.assertFalse(message.hasOptionalInt64   ());
    Assert.assertFalse(message.hasOptionalUint32  ());
    Assert.assertFalse(message.hasOptionalUint64  ());
    Assert.assertFalse(message.hasOptionalSint32  ());
    Assert.assertFalse(message.hasOptionalSint64  ());
    Assert.assertFalse(message.hasOptionalFixed32 ());
    Assert.assertFalse(message.hasOptionalFixed64 ());
    Assert.assertFalse(message.hasOptionalSfixed32());
    Assert.assertFalse(message.hasOptionalSfixed64());
    Assert.assertFalse(message.hasOptionalFloat   ());
    Assert.assertFalse(message.hasOptionalDouble  ());
    Assert.assertFalse(message.hasOptionalBool    ());
    Assert.assertFalse(message.hasOptionalString  ());
    Assert.assertFalse(message.hasOptionalBytes   ());

    Assert.assertFalse(message.hasOptionalGroup         ());
    Assert.assertFalse(message.hasOptionalNestedMessage ());
    Assert.assertFalse(message.hasOptionalForeignMessage());
    Assert.assertFalse(message.hasOptionalImportMessage ());

    Assert.assertFalse(message.hasOptionalNestedEnum ());
    Assert.assertFalse(message.hasOptionalForeignEnum());
    Assert.assertFalse(message.hasOptionalImportEnum ());

    Assert.assertFalse(message.hasOptionalStringPiece());
    Assert.assertFalse(message.hasOptionalCord());

    // Optional fields without defaults are set to zero or something like it.
    Assert.assertEquals(0    , message.getOptionalInt32   ());
    Assert.assertEquals(0    , message.getOptionalInt64   ());
    Assert.assertEquals(0    , message.getOptionalUint32  ());
    Assert.assertEquals(0    , message.getOptionalUint64  ());
    Assert.assertEquals(0    , message.getOptionalSint32  ());
    Assert.assertEquals(0    , message.getOptionalSint64  ());
    Assert.assertEquals(0    , message.getOptionalFixed32 ());
    Assert.assertEquals(0    , message.getOptionalFixed64 ());
    Assert.assertEquals(0    , message.getOptionalSfixed32());
    Assert.assertEquals(0    , message.getOptionalSfixed64());
    Assert.assertEquals(0    , message.getOptionalFloat   (), 0.0);
    Assert.assertEquals(0    , message.getOptionalDouble  (), 0.0);
    Assert.assertEquals(false, message.getOptionalBool    ());
    Assert.assertEquals(""   , message.getOptionalString  ());
    Assert.assertEquals(ByteString.EMPTY, message.getOptionalBytes());

    // Embedded messages should also be clear.
    Assert.assertFalse(message.getOptionalGroup         ().hasA());
    Assert.assertFalse(message.getOptionalNestedMessage ().hasBb());
    Assert.assertFalse(message.getOptionalForeignMessage().hasC());
    Assert.assertFalse(message.getOptionalImportMessage ().hasD());

    Assert.assertEquals(0, message.getOptionalGroup         ().getA());
    Assert.assertEquals(0, message.getOptionalNestedMessage ().getBb());
    Assert.assertEquals(0, message.getOptionalForeignMessage().getC());
    Assert.assertEquals(0, message.getOptionalImportMessage ().getD());

    // Enums without defaults are set to the first value in the enum.
    Assert.assertEquals(TestAllTypes.NestedEnum.FOO, message.getOptionalNestedEnum ());
    Assert.assertEquals(ForeignEnum.FOREIGN_FOO, message.getOptionalForeignEnum());
    Assert.assertEquals(ImportEnum.IMPORT_FOO, message.getOptionalImportEnum());

    Assert.assertEquals("", message.getOptionalStringPiece());
    Assert.assertEquals("", message.getOptionalCord());

    // Repeated fields are empty.
    Assert.assertEquals(0, message.getRepeatedInt32Count   ());
    Assert.assertEquals(0, message.getRepeatedInt64Count   ());
    Assert.assertEquals(0, message.getRepeatedUint32Count  ());
    Assert.assertEquals(0, message.getRepeatedUint64Count  ());
    Assert.assertEquals(0, message.getRepeatedSint32Count  ());
    Assert.assertEquals(0, message.getRepeatedSint64Count  ());
    Assert.assertEquals(0, message.getRepeatedFixed32Count ());
    Assert.assertEquals(0, message.getRepeatedFixed64Count ());
    Assert.assertEquals(0, message.getRepeatedSfixed32Count());
    Assert.assertEquals(0, message.getRepeatedSfixed64Count());
    Assert.assertEquals(0, message.getRepeatedFloatCount   ());
    Assert.assertEquals(0, message.getRepeatedDoubleCount  ());
    Assert.assertEquals(0, message.getRepeatedBoolCount    ());
    Assert.assertEquals(0, message.getRepeatedStringCount  ());
    Assert.assertEquals(0, message.getRepeatedBytesCount   ());

    Assert.assertEquals(0, message.getRepeatedGroupCount         ());
    Assert.assertEquals(0, message.getRepeatedNestedMessageCount ());
    Assert.assertEquals(0, message.getRepeatedForeignMessageCount());
    Assert.assertEquals(0, message.getRepeatedImportMessageCount ());
    Assert.assertEquals(0, message.getRepeatedNestedEnumCount    ());
    Assert.assertEquals(0, message.getRepeatedForeignEnumCount   ());
    Assert.assertEquals(0, message.getRepeatedImportEnumCount    ());

    Assert.assertEquals(0, message.getRepeatedStringPieceCount());
    Assert.assertEquals(0, message.getRepeatedCordCount());

    // hasBlah() should also be false for all default fields.
    Assert.assertFalse(message.hasDefaultInt32   ());
    Assert.assertFalse(message.hasDefaultInt64   ());
    Assert.assertFalse(message.hasDefaultUint32  ());
    Assert.assertFalse(message.hasDefaultUint64  ());
    Assert.assertFalse(message.hasDefaultSint32  ());
    Assert.assertFalse(message.hasDefaultSint64  ());
    Assert.assertFalse(message.hasDefaultFixed32 ());
    Assert.assertFalse(message.hasDefaultFixed64 ());
    Assert.assertFalse(message.hasDefaultSfixed32());
    Assert.assertFalse(message.hasDefaultSfixed64());
    Assert.assertFalse(message.hasDefaultFloat   ());
    Assert.assertFalse(message.hasDefaultDouble  ());
    Assert.assertFalse(message.hasDefaultBool    ());
    Assert.assertFalse(message.hasDefaultString  ());
    Assert.assertFalse(message.hasDefaultBytes   ());

    Assert.assertFalse(message.hasDefaultNestedEnum ());
    Assert.assertFalse(message.hasDefaultForeignEnum());
    Assert.assertFalse(message.hasDefaultImportEnum ());

    Assert.assertFalse(message.hasDefaultStringPiece());
    Assert.assertFalse(message.hasDefaultCord());

    // Fields with defaults have their default values (duh).
    Assert.assertEquals( 41    , message.getDefaultInt32   ());
    Assert.assertEquals( 42    , message.getDefaultInt64   ());
    Assert.assertEquals( 43    , message.getDefaultUint32  ());
    Assert.assertEquals( 44    , message.getDefaultUint64  ());
    Assert.assertEquals(-45    , message.getDefaultSint32  ());
    Assert.assertEquals( 46    , message.getDefaultSint64  ());
    Assert.assertEquals( 47    , message.getDefaultFixed32 ());
    Assert.assertEquals( 48    , message.getDefaultFixed64 ());
    Assert.assertEquals( 49    , message.getDefaultSfixed32());
    Assert.assertEquals(-50    , message.getDefaultSfixed64());
    Assert.assertEquals( 51.5  , message.getDefaultFloat   (), 0.0);
    Assert.assertEquals( 52e3  , message.getDefaultDouble  (), 0.0);
    Assert.assertEquals(true   , message.getDefaultBool    ());
    Assert.assertEquals("hello", message.getDefaultString  ());
    Assert.assertEquals(toBytes("world"), message.getDefaultBytes());

    Assert.assertEquals(TestAllTypes.NestedEnum.BAR, message.getDefaultNestedEnum ());
    Assert.assertEquals(ForeignEnum.FOREIGN_BAR, message.getDefaultForeignEnum());
    Assert.assertEquals(ImportEnum.IMPORT_BAR, message.getDefaultImportEnum());

    Assert.assertEquals("abc", message.getDefaultStringPiece());
    Assert.assertEquals("123", message.getDefaultCord());
  }

  // -------------------------------------------------------------------

  /**
   * Assert (using {@code junit.framework.Assert}} that all fields of
   * {@code message} are set to the values assigned by {@code setAllFields}
   * followed by {@code modifyRepeatedFields}.
   */
  public static void assertRepeatedFieldsModified(TestAllTypes message) {
    // ModifyRepeatedFields only sets the second repeated element of each
    // field.  In addition to verifying this, we also verify that the first
    // element and size were *not* modified.
    Assert.assertEquals(2, message.getRepeatedInt32Count   ());
    Assert.assertEquals(2, message.getRepeatedInt64Count   ());
    Assert.assertEquals(2, message.getRepeatedUint32Count  ());
    Assert.assertEquals(2, message.getRepeatedUint64Count  ());
    Assert.assertEquals(2, message.getRepeatedSint32Count  ());
    Assert.assertEquals(2, message.getRepeatedSint64Count  ());
    Assert.assertEquals(2, message.getRepeatedFixed32Count ());
    Assert.assertEquals(2, message.getRepeatedFixed64Count ());
    Assert.assertEquals(2, message.getRepeatedSfixed32Count());
    Assert.assertEquals(2, message.getRepeatedSfixed64Count());
    Assert.assertEquals(2, message.getRepeatedFloatCount   ());
    Assert.assertEquals(2, message.getRepeatedDoubleCount  ());
    Assert.assertEquals(2, message.getRepeatedBoolCount    ());
    Assert.assertEquals(2, message.getRepeatedStringCount  ());
    Assert.assertEquals(2, message.getRepeatedBytesCount   ());

    Assert.assertEquals(2, message.getRepeatedGroupCount         ());
    Assert.assertEquals(2, message.getRepeatedNestedMessageCount ());
    Assert.assertEquals(2, message.getRepeatedForeignMessageCount());
    Assert.assertEquals(2, message.getRepeatedImportMessageCount ());
    Assert.assertEquals(2, message.getRepeatedNestedEnumCount    ());
    Assert.assertEquals(2, message.getRepeatedForeignEnumCount   ());
    Assert.assertEquals(2, message.getRepeatedImportEnumCount    ());

    Assert.assertEquals(2, message.getRepeatedStringPieceCount());
    Assert.assertEquals(2, message.getRepeatedCordCount());

    Assert.assertEquals(201  , message.getRepeatedInt32   (0));
    Assert.assertEquals(202L , message.getRepeatedInt64   (0));
    Assert.assertEquals(203  , message.getRepeatedUint32  (0));
    Assert.assertEquals(204L , message.getRepeatedUint64  (0));
    Assert.assertEquals(205  , message.getRepeatedSint32  (0));
    Assert.assertEquals(206L , message.getRepeatedSint64  (0));
    Assert.assertEquals(207  , message.getRepeatedFixed32 (0));
    Assert.assertEquals(208L , message.getRepeatedFixed64 (0));
    Assert.assertEquals(209  , message.getRepeatedSfixed32(0));
    Assert.assertEquals(210L , message.getRepeatedSfixed64(0));
    Assert.assertEquals(211F , message.getRepeatedFloat   (0));
    Assert.assertEquals(212D , message.getRepeatedDouble  (0));
    Assert.assertEquals(true , message.getRepeatedBool    (0));
    Assert.assertEquals("215", message.getRepeatedString  (0));
    Assert.assertEquals(toBytes("216"), message.getRepeatedBytes(0));

    Assert.assertEquals(217, message.getRepeatedGroup         (0).getA());
    Assert.assertEquals(218, message.getRepeatedNestedMessage (0).getBb());
    Assert.assertEquals(219, message.getRepeatedForeignMessage(0).getC());
    Assert.assertEquals(220, message.getRepeatedImportMessage (0).getD());

    Assert.assertEquals(TestAllTypes.NestedEnum.BAR, message.getRepeatedNestedEnum (0));
    Assert.assertEquals(ForeignEnum.FOREIGN_BAR, message.getRepeatedForeignEnum(0));
    Assert.assertEquals(ImportEnum.IMPORT_BAR, message.getRepeatedImportEnum(0));

    Assert.assertEquals("224", message.getRepeatedStringPiece(0));
    Assert.assertEquals("225", message.getRepeatedCord(0));

    // Actually verify the second (modified) elements now.
    Assert.assertEquals(501  , message.getRepeatedInt32   (1));
    Assert.assertEquals(502L , message.getRepeatedInt64   (1));
    Assert.assertEquals(503  , message.getRepeatedUint32  (1));
    Assert.assertEquals(504L , message.getRepeatedUint64  (1));
    Assert.assertEquals(505  , message.getRepeatedSint32  (1));
    Assert.assertEquals(506L , message.getRepeatedSint64  (1));
    Assert.assertEquals(507  , message.getRepeatedFixed32 (1));
    Assert.assertEquals(508L , message.getRepeatedFixed64 (1));
    Assert.assertEquals(509  , message.getRepeatedSfixed32(1));
    Assert.assertEquals(510L , message.getRepeatedSfixed64(1));
    Assert.assertEquals(511F , message.getRepeatedFloat   (1));
    Assert.assertEquals(512D , message.getRepeatedDouble  (1));
    Assert.assertEquals(true , message.getRepeatedBool    (1));
    Assert.assertEquals("515", message.getRepeatedString  (1));
    Assert.assertEquals(toBytes("516"), message.getRepeatedBytes(1));

    Assert.assertEquals(517, message.getRepeatedGroup         (1).getA());
    Assert.assertEquals(518, message.getRepeatedNestedMessage (1).getBb());
    Assert.assertEquals(519, message.getRepeatedForeignMessage(1).getC());
    Assert.assertEquals(520, message.getRepeatedImportMessage (1).getD());

    Assert.assertEquals(TestAllTypes.NestedEnum.FOO, message.getRepeatedNestedEnum (1));
    Assert.assertEquals(ForeignEnum.FOREIGN_FOO, message.getRepeatedForeignEnum(1));
    Assert.assertEquals(ImportEnum.IMPORT_FOO, message.getRepeatedImportEnum(1));

    Assert.assertEquals("524", message.getRepeatedStringPiece(1));
    Assert.assertEquals("525", message.getRepeatedCord(1));
  }

  // ===================================================================
  // Like above, but for extensions

  // Java gets confused with things like assertEquals(int, Integer):  it can't
  // decide whether to call assertEquals(int, int) or assertEquals(Object,
  // Object).  So we define these methods to help it.
  private static void assertEqualsExactType(int a, int b) {
    Assert.assertEquals(a, b);
  }
  private static void assertEqualsExactType(long a, long b) {
    Assert.assertEquals(a, b);
  }
  private static void assertEqualsExactType(float a, float b) {
    Assert.assertEquals(a, b, 0.0);
  }
  private static void assertEqualsExactType(double a, double b) {
    Assert.assertEquals(a, b, 0.0);
  }
  private static void assertEqualsExactType(boolean a, boolean b) {
    Assert.assertEquals(a, b);
  }
  private static void assertEqualsExactType(String a, String b) {
    Assert.assertEquals(a, b);
  }
  private static void assertEqualsExactType(ByteString a, ByteString b) {
    Assert.assertEquals(a, b);
  }
  private static void assertEqualsExactType(TestAllTypes.NestedEnum a,
                                            TestAllTypes.NestedEnum b) {
    Assert.assertEquals(a, b);
  }
  private static void assertEqualsExactType(ForeignEnum a, ForeignEnum b) {
    Assert.assertEquals(a, b);
  }
  private static void assertEqualsExactType(ImportEnum a, ImportEnum b) {
    Assert.assertEquals(a, b);
  }

  /**
   * Get an unmodifiable {@link ExtensionRegistry} containing all the
   * extensions of {@code TestAllExtensions}.
   */
  public static ExtensionRegistry getExtensionRegistry() {
    ExtensionRegistry registry = ExtensionRegistry.newInstance();
    registerAllExtensions(registry);
    return registry.getUnmodifiable();
  }

  /**
   * Register all of {@code TestAllExtensions}' extensions with the
   * given {@link ExtensionRegistry}.
   */
  public static void registerAllExtensions(ExtensionRegistry registry) {
    UnittestProto.registerAllExtensions(registry);
  }

  /**
   * Set every field of {@code message} to the values expected by
   * {@code assertAllExtensionsSet()}.
   */
  public static void setAllExtensions(TestAllExtensions.Builder message) {
    message.setExtension(UnittestProto.optionalInt32Extension   , 101);
    message.setExtension(UnittestProto.optionalInt64Extension   , 102L);
    message.setExtension(UnittestProto.optionalUint32Extension  , 103);
    message.setExtension(UnittestProto.optionalUint64Extension  , 104L);
    message.setExtension(UnittestProto.optionalSint32Extension  , 105);
    message.setExtension(UnittestProto.optionalSint64Extension  , 106L);
    message.setExtension(UnittestProto.optionalFixed32Extension , 107);
    message.setExtension(UnittestProto.optionalFixed64Extension , 108L);
    message.setExtension(UnittestProto.optionalSfixed32Extension, 109);
    message.setExtension(UnittestProto.optionalSfixed64Extension, 110L);
    message.setExtension(UnittestProto.optionalFloatExtension   , 111F);
    message.setExtension(UnittestProto.optionalDoubleExtension  , 112D);
    message.setExtension(UnittestProto.optionalBoolExtension    , true);
    message.setExtension(UnittestProto.optionalStringExtension  , "115");
    message.setExtension(UnittestProto.optionalBytesExtension   , toBytes("116"));

    message.setExtension(UnittestProto.optionalGroupExtension,
      UnittestProto.OptionalGroup_extension.newBuilder().setA(117).build());
    message.setExtension(UnittestProto.optionalNestedMessageExtension,
      TestAllTypes.NestedMessage.newBuilder().setBb(118).build());
    message.setExtension(UnittestProto.optionalForeignMessageExtension,
      ForeignMessage.newBuilder().setC(119).build());
    message.setExtension(UnittestProto.optionalImportMessageExtension,
      ImportMessage.newBuilder().setD(120).build());

    message.setExtension(UnittestProto.optionalNestedEnumExtension,
                         TestAllTypes.NestedEnum.BAZ);
    message.setExtension(UnittestProto.optionalForeignEnumExtension,
                         ForeignEnum.FOREIGN_BAZ);
    message.setExtension(UnittestProto.optionalImportEnumExtension,
                         ImportEnum.IMPORT_BAZ);

    message.setExtension(UnittestProto.optionalStringPieceExtension, "124");
    message.setExtension(UnittestProto.optionalCordExtension, "125");

    // -----------------------------------------------------------------

    message.addExtension(UnittestProto.repeatedInt32Extension   , 201);
    message.addExtension(UnittestProto.repeatedInt64Extension   , 202L);
    message.addExtension(UnittestProto.repeatedUint32Extension  , 203);
    message.addExtension(UnittestProto.repeatedUint64Extension  , 204L);
    message.addExtension(UnittestProto.repeatedSint32Extension  , 205);
    message.addExtension(UnittestProto.repeatedSint64Extension  , 206L);
    message.addExtension(UnittestProto.repeatedFixed32Extension , 207);
    message.addExtension(UnittestProto.repeatedFixed64Extension , 208L);
    message.addExtension(UnittestProto.repeatedSfixed32Extension, 209);
    message.addExtension(UnittestProto.repeatedSfixed64Extension, 210L);
    message.addExtension(UnittestProto.repeatedFloatExtension   , 211F);
    message.addExtension(UnittestProto.repeatedDoubleExtension  , 212D);
    message.addExtension(UnittestProto.repeatedBoolExtension    , true);
    message.addExtension(UnittestProto.repeatedStringExtension  , "215");
    message.addExtension(UnittestProto.repeatedBytesExtension   , toBytes("216"));

    message.addExtension(UnittestProto.repeatedGroupExtension,
      UnittestProto.RepeatedGroup_extension.newBuilder().setA(217).build());
    message.addExtension(UnittestProto.repeatedNestedMessageExtension,
      TestAllTypes.NestedMessage.newBuilder().setBb(218).build());
    message.addExtension(UnittestProto.repeatedForeignMessageExtension,
      ForeignMessage.newBuilder().setC(219).build());
    message.addExtension(UnittestProto.repeatedImportMessageExtension,
      ImportMessage.newBuilder().setD(220).build());

    message.addExtension(UnittestProto.repeatedNestedEnumExtension,
                         TestAllTypes.NestedEnum.BAR);
    message.addExtension(UnittestProto.repeatedForeignEnumExtension,
                         ForeignEnum.FOREIGN_BAR);
    message.addExtension(UnittestProto.repeatedImportEnumExtension,
                         ImportEnum.IMPORT_BAR);

    message.addExtension(UnittestProto.repeatedStringPieceExtension, "224");
    message.addExtension(UnittestProto.repeatedCordExtension, "225");

    // Add a second one of each field.
    message.addExtension(UnittestProto.repeatedInt32Extension   , 301);
    message.addExtension(UnittestProto.repeatedInt64Extension   , 302L);
    message.addExtension(UnittestProto.repeatedUint32Extension  , 303);
    message.addExtension(UnittestProto.repeatedUint64Extension  , 304L);
    message.addExtension(UnittestProto.repeatedSint32Extension  , 305);
    message.addExtension(UnittestProto.repeatedSint64Extension  , 306L);
    message.addExtension(UnittestProto.repeatedFixed32Extension , 307);
    message.addExtension(UnittestProto.repeatedFixed64Extension , 308L);
    message.addExtension(UnittestProto.repeatedSfixed32Extension, 309);
    message.addExtension(UnittestProto.repeatedSfixed64Extension, 310L);
    message.addExtension(UnittestProto.repeatedFloatExtension   , 311F);
    message.addExtension(UnittestProto.repeatedDoubleExtension  , 312D);
    message.addExtension(UnittestProto.repeatedBoolExtension    , false);
    message.addExtension(UnittestProto.repeatedStringExtension  , "315");
    message.addExtension(UnittestProto.repeatedBytesExtension   , toBytes("316"));

    message.addExtension(UnittestProto.repeatedGroupExtension,
      UnittestProto.RepeatedGroup_extension.newBuilder().setA(317).build());
    message.addExtension(UnittestProto.repeatedNestedMessageExtension,
      TestAllTypes.NestedMessage.newBuilder().setBb(318).build());
    message.addExtension(UnittestProto.repeatedForeignMessageExtension,
      ForeignMessage.newBuilder().setC(319).build());
    message.addExtension(UnittestProto.repeatedImportMessageExtension,
      ImportMessage.newBuilder().setD(320).build());

    message.addExtension(UnittestProto.repeatedNestedEnumExtension,
                         TestAllTypes.NestedEnum.BAZ);
    message.addExtension(UnittestProto.repeatedForeignEnumExtension,
                         ForeignEnum.FOREIGN_BAZ);
    message.addExtension(UnittestProto.repeatedImportEnumExtension,
                         ImportEnum.IMPORT_BAZ);

    message.addExtension(UnittestProto.repeatedStringPieceExtension, "324");
    message.addExtension(UnittestProto.repeatedCordExtension, "325");

    // -----------------------------------------------------------------

    message.setExtension(UnittestProto.defaultInt32Extension   , 401);
    message.setExtension(UnittestProto.defaultInt64Extension   , 402L);
    message.setExtension(UnittestProto.defaultUint32Extension  , 403);
    message.setExtension(UnittestProto.defaultUint64Extension  , 404L);
    message.setExtension(UnittestProto.defaultSint32Extension  , 405);
    message.setExtension(UnittestProto.defaultSint64Extension  , 406L);
    message.setExtension(UnittestProto.defaultFixed32Extension , 407);
    message.setExtension(UnittestProto.defaultFixed64Extension , 408L);
    message.setExtension(UnittestProto.defaultSfixed32Extension, 409);
    message.setExtension(UnittestProto.defaultSfixed64Extension, 410L);
    message.setExtension(UnittestProto.defaultFloatExtension   , 411F);
    message.setExtension(UnittestProto.defaultDoubleExtension  , 412D);
    message.setExtension(UnittestProto.defaultBoolExtension    , false);
    message.setExtension(UnittestProto.defaultStringExtension  , "415");
    message.setExtension(UnittestProto.defaultBytesExtension   , toBytes("416"));

    message.setExtension(UnittestProto.defaultNestedEnumExtension,
                         TestAllTypes.NestedEnum.FOO);
    message.setExtension(UnittestProto.defaultForeignEnumExtension,
                         ForeignEnum.FOREIGN_FOO);
    message.setExtension(UnittestProto.defaultImportEnumExtension,
                         ImportEnum.IMPORT_FOO);

    message.setExtension(UnittestProto.defaultStringPieceExtension, "424");
    message.setExtension(UnittestProto.defaultCordExtension, "425");
  }

  // -------------------------------------------------------------------

  /**
   * Modify the repeated extensions of {@code message} to contain the values
   * expected by {@code assertRepeatedExtensionsModified()}.
   */
  public static void modifyRepeatedExtensions(
      TestAllExtensions.Builder message) {
    message.setExtension(UnittestProto.repeatedInt32Extension   , 1, 501);
    message.setExtension(UnittestProto.repeatedInt64Extension   , 1, 502L);
    message.setExtension(UnittestProto.repeatedUint32Extension  , 1, 503);
    message.setExtension(UnittestProto.repeatedUint64Extension  , 1, 504L);
    message.setExtension(UnittestProto.repeatedSint32Extension  , 1, 505);
    message.setExtension(UnittestProto.repeatedSint64Extension  , 1, 506L);
    message.setExtension(UnittestProto.repeatedFixed32Extension , 1, 507);
    message.setExtension(UnittestProto.repeatedFixed64Extension , 1, 508L);
    message.setExtension(UnittestProto.repeatedSfixed32Extension, 1, 509);
    message.setExtension(UnittestProto.repeatedSfixed64Extension, 1, 510L);
    message.setExtension(UnittestProto.repeatedFloatExtension   , 1, 511F);
    message.setExtension(UnittestProto.repeatedDoubleExtension  , 1, 512D);
    message.setExtension(UnittestProto.repeatedBoolExtension    , 1, true);
    message.setExtension(UnittestProto.repeatedStringExtension  , 1, "515");
    message.setExtension(UnittestProto.repeatedBytesExtension   , 1, toBytes("516"));

    message.setExtension(UnittestProto.repeatedGroupExtension, 1,
      UnittestProto.RepeatedGroup_extension.newBuilder().setA(517).build());
    message.setExtension(UnittestProto.repeatedNestedMessageExtension, 1,
      TestAllTypes.NestedMessage.newBuilder().setBb(518).build());
    message.setExtension(UnittestProto.repeatedForeignMessageExtension, 1,
      ForeignMessage.newBuilder().setC(519).build());
    message.setExtension(UnittestProto.repeatedImportMessageExtension, 1,
      ImportMessage.newBuilder().setD(520).build());

    message.setExtension(UnittestProto.repeatedNestedEnumExtension , 1,
                         TestAllTypes.NestedEnum.FOO);
    message.setExtension(UnittestProto.repeatedForeignEnumExtension, 1,
                         ForeignEnum.FOREIGN_FOO);
    message.setExtension(UnittestProto.repeatedImportEnumExtension , 1,
                         ImportEnum.IMPORT_FOO);

    message.setExtension(UnittestProto.repeatedStringPieceExtension, 1, "524");
    message.setExtension(UnittestProto.repeatedCordExtension, 1, "525");
  }

  // -------------------------------------------------------------------

  /**
   * Assert (using {@code junit.framework.Assert}} that all extensions of
   * {@code message} are set to the values assigned by {@code setAllExtensions}.
   */
  public static void assertAllExtensionsSet(TestAllExtensions message) {
    Assert.assertTrue(message.hasExtension(UnittestProto.optionalInt32Extension   ));
    Assert.assertTrue(message.hasExtension(UnittestProto.optionalInt64Extension   ));
    Assert.assertTrue(message.hasExtension(UnittestProto.optionalUint32Extension  ));
    Assert.assertTrue(message.hasExtension(UnittestProto.optionalUint64Extension  ));
    Assert.assertTrue(message.hasExtension(UnittestProto.optionalSint32Extension  ));
    Assert.assertTrue(message.hasExtension(UnittestProto.optionalSint64Extension  ));
    Assert.assertTrue(message.hasExtension(UnittestProto.optionalFixed32Extension ));
    Assert.assertTrue(message.hasExtension(UnittestProto.optionalFixed64Extension ));
    Assert.assertTrue(message.hasExtension(UnittestProto.optionalSfixed32Extension));
    Assert.assertTrue(message.hasExtension(UnittestProto.optionalSfixed64Extension));
    Assert.assertTrue(message.hasExtension(UnittestProto.optionalFloatExtension   ));
    Assert.assertTrue(message.hasExtension(UnittestProto.optionalDoubleExtension  ));
    Assert.assertTrue(message.hasExtension(UnittestProto.optionalBoolExtension    ));
    Assert.assertTrue(message.hasExtension(UnittestProto.optionalStringExtension  ));
    Assert.assertTrue(message.hasExtension(UnittestProto.optionalBytesExtension   ));

    Assert.assertTrue(message.hasExtension(UnittestProto.optionalGroupExtension         ));
    Assert.assertTrue(message.hasExtension(UnittestProto.optionalNestedMessageExtension ));
    Assert.assertTrue(message.hasExtension(UnittestProto.optionalForeignMessageExtension));
    Assert.assertTrue(message.hasExtension(UnittestProto.optionalImportMessageExtension ));

    Assert.assertTrue(message.getExtension(UnittestProto.optionalGroupExtension         ).hasA());
    Assert.assertTrue(message.getExtension(UnittestProto.optionalNestedMessageExtension ).hasBb());
    Assert.assertTrue(message.getExtension(UnittestProto.optionalForeignMessageExtension).hasC());
    Assert.assertTrue(message.getExtension(UnittestProto.optionalImportMessageExtension ).hasD());

    Assert.assertTrue(message.hasExtension(UnittestProto.optionalNestedEnumExtension ));
    Assert.assertTrue(message.hasExtension(UnittestProto.optionalForeignEnumExtension));
    Assert.assertTrue(message.hasExtension(UnittestProto.optionalImportEnumExtension ));

    Assert.assertTrue(message.hasExtension(UnittestProto.optionalStringPieceExtension));
    Assert.assertTrue(message.hasExtension(UnittestProto.optionalCordExtension));

    assertEqualsExactType(101  , message.getExtension(UnittestProto.optionalInt32Extension   ));
    assertEqualsExactType(102L , message.getExtension(UnittestProto.optionalInt64Extension   ));
    assertEqualsExactType(103  , message.getExtension(UnittestProto.optionalUint32Extension  ));
    assertEqualsExactType(104L , message.getExtension(UnittestProto.optionalUint64Extension  ));
    assertEqualsExactType(105  , message.getExtension(UnittestProto.optionalSint32Extension  ));
    assertEqualsExactType(106L , message.getExtension(UnittestProto.optionalSint64Extension  ));
    assertEqualsExactType(107  , message.getExtension(UnittestProto.optionalFixed32Extension ));
    assertEqualsExactType(108L , message.getExtension(UnittestProto.optionalFixed64Extension ));
    assertEqualsExactType(109  , message.getExtension(UnittestProto.optionalSfixed32Extension));
    assertEqualsExactType(110L , message.getExtension(UnittestProto.optionalSfixed64Extension));
    assertEqualsExactType(111F , message.getExtension(UnittestProto.optionalFloatExtension   ));
    assertEqualsExactType(112D , message.getExtension(UnittestProto.optionalDoubleExtension  ));
    assertEqualsExactType(true , message.getExtension(UnittestProto.optionalBoolExtension    ));
    assertEqualsExactType("115", message.getExtension(UnittestProto.optionalStringExtension  ));
    assertEqualsExactType(toBytes("116"), message.getExtension(UnittestProto.optionalBytesExtension));

    assertEqualsExactType(117, message.getExtension(UnittestProto.optionalGroupExtension         ).getA());
    assertEqualsExactType(118, message.getExtension(UnittestProto.optionalNestedMessageExtension ).getBb());
    assertEqualsExactType(119, message.getExtension(UnittestProto.optionalForeignMessageExtension).getC());
    assertEqualsExactType(120, message.getExtension(UnittestProto.optionalImportMessageExtension ).getD());

    assertEqualsExactType(TestAllTypes.NestedEnum.BAZ,
      message.getExtension(UnittestProto.optionalNestedEnumExtension));
    assertEqualsExactType(ForeignEnum.FOREIGN_BAZ,
      message.getExtension(UnittestProto.optionalForeignEnumExtension));
    assertEqualsExactType(ImportEnum.IMPORT_BAZ,
      message.getExtension(UnittestProto.optionalImportEnumExtension));

    assertEqualsExactType("124", message.getExtension(UnittestProto.optionalStringPieceExtension));
    assertEqualsExactType("125", message.getExtension(UnittestProto.optionalCordExtension));

    // -----------------------------------------------------------------

    Assert.assertEquals(2, message.getExtensionCount(UnittestProto.repeatedInt32Extension   ));
    Assert.assertEquals(2, message.getExtensionCount(UnittestProto.repeatedInt64Extension   ));
    Assert.assertEquals(2, message.getExtensionCount(UnittestProto.repeatedUint32Extension  ));
    Assert.assertEquals(2, message.getExtensionCount(UnittestProto.repeatedUint64Extension  ));
    Assert.assertEquals(2, message.getExtensionCount(UnittestProto.repeatedSint32Extension  ));
    Assert.assertEquals(2, message.getExtensionCount(UnittestProto.repeatedSint64Extension  ));
    Assert.assertEquals(2, message.getExtensionCount(UnittestProto.repeatedFixed32Extension ));
    Assert.assertEquals(2, message.getExtensionCount(UnittestProto.repeatedFixed64Extension ));
    Assert.assertEquals(2, message.getExtensionCount(UnittestProto.repeatedSfixed32Extension));
    Assert.assertEquals(2, message.getExtensionCount(UnittestProto.repeatedSfixed64Extension));
    Assert.assertEquals(2, message.getExtensionCount(UnittestProto.repeatedFloatExtension   ));
    Assert.assertEquals(2, message.getExtensionCount(UnittestProto.repeatedDoubleExtension  ));
    Assert.assertEquals(2, message.getExtensionCount(UnittestProto.repeatedBoolExtension    ));
    Assert.assertEquals(2, message.getExtensionCount(UnittestProto.repeatedStringExtension  ));
    Assert.assertEquals(2, message.getExtensionCount(UnittestProto.repeatedBytesExtension   ));

    Assert.assertEquals(2, message.getExtensionCount(UnittestProto.repeatedGroupExtension         ));
    Assert.assertEquals(2, message.getExtensionCount(UnittestProto.repeatedNestedMessageExtension ));
    Assert.assertEquals(2, message.getExtensionCount(UnittestProto.repeatedForeignMessageExtension));
    Assert.assertEquals(2, message.getExtensionCount(UnittestProto.repeatedImportMessageExtension ));
    Assert.assertEquals(2, message.getExtensionCount(UnittestProto.repeatedNestedEnumExtension    ));
    Assert.assertEquals(2, message.getExtensionCount(UnittestProto.repeatedForeignEnumExtension   ));
    Assert.assertEquals(2, message.getExtensionCount(UnittestProto.repeatedImportEnumExtension    ));

    Assert.assertEquals(2, message.getExtensionCount(UnittestProto.repeatedStringPieceExtension));
    Assert.assertEquals(2, message.getExtensionCount(UnittestProto.repeatedCordExtension));

    assertEqualsExactType(201  , message.getExtension(UnittestProto.repeatedInt32Extension   , 0));
    assertEqualsExactType(202L , message.getExtension(UnittestProto.repeatedInt64Extension   , 0));
    assertEqualsExactType(203  , message.getExtension(UnittestProto.repeatedUint32Extension  , 0));
    assertEqualsExactType(204L , message.getExtension(UnittestProto.repeatedUint64Extension  , 0));
    assertEqualsExactType(205  , message.getExtension(UnittestProto.repeatedSint32Extension  , 0));
    assertEqualsExactType(206L , message.getExtension(UnittestProto.repeatedSint64Extension  , 0));
    assertEqualsExactType(207  , message.getExtension(UnittestProto.repeatedFixed32Extension , 0));
    assertEqualsExactType(208L , message.getExtension(UnittestProto.repeatedFixed64Extension , 0));
    assertEqualsExactType(209  , message.getExtension(UnittestProto.repeatedSfixed32Extension, 0));
    assertEqualsExactType(210L , message.getExtension(UnittestProto.repeatedSfixed64Extension, 0));
    assertEqualsExactType(211F , message.getExtension(UnittestProto.repeatedFloatExtension   , 0));
    assertEqualsExactType(212D , message.getExtension(UnittestProto.repeatedDoubleExtension  , 0));
    assertEqualsExactType(true , message.getExtension(UnittestProto.repeatedBoolExtension    , 0));
    assertEqualsExactType("215", message.getExtension(UnittestProto.repeatedStringExtension  , 0));
    assertEqualsExactType(toBytes("216"), message.getExtension(UnittestProto.repeatedBytesExtension, 0));

    assertEqualsExactType(217, message.getExtension(UnittestProto.repeatedGroupExtension         , 0).getA());
    assertEqualsExactType(218, message.getExtension(UnittestProto.repeatedNestedMessageExtension , 0).getBb());
    assertEqualsExactType(219, message.getExtension(UnittestProto.repeatedForeignMessageExtension, 0).getC());
    assertEqualsExactType(220, message.getExtension(UnittestProto.repeatedImportMessageExtension , 0).getD());

    assertEqualsExactType(TestAllTypes.NestedEnum.BAR,
      message.getExtension(UnittestProto.repeatedNestedEnumExtension, 0));
    assertEqualsExactType(ForeignEnum.FOREIGN_BAR,
      message.getExtension(UnittestProto.repeatedForeignEnumExtension, 0));
    assertEqualsExactType(ImportEnum.IMPORT_BAR,
      message.getExtension(UnittestProto.repeatedImportEnumExtension, 0));

    assertEqualsExactType("224", message.getExtension(UnittestProto.repeatedStringPieceExtension, 0));
    assertEqualsExactType("225", message.getExtension(UnittestProto.repeatedCordExtension, 0));

    assertEqualsExactType(301  , message.getExtension(UnittestProto.repeatedInt32Extension   , 1));
    assertEqualsExactType(302L , message.getExtension(UnittestProto.repeatedInt64Extension   , 1));
    assertEqualsExactType(303  , message.getExtension(UnittestProto.repeatedUint32Extension  , 1));
    assertEqualsExactType(304L , message.getExtension(UnittestProto.repeatedUint64Extension  , 1));
    assertEqualsExactType(305  , message.getExtension(UnittestProto.repeatedSint32Extension  , 1));
    assertEqualsExactType(306L , message.getExtension(UnittestProto.repeatedSint64Extension  , 1));
    assertEqualsExactType(307  , message.getExtension(UnittestProto.repeatedFixed32Extension , 1));
    assertEqualsExactType(308L , message.getExtension(UnittestProto.repeatedFixed64Extension , 1));
    assertEqualsExactType(309  , message.getExtension(UnittestProto.repeatedSfixed32Extension, 1));
    assertEqualsExactType(310L , message.getExtension(UnittestProto.repeatedSfixed64Extension, 1));
    assertEqualsExactType(311F , message.getExtension(UnittestProto.repeatedFloatExtension   , 1));
    assertEqualsExactType(312D , message.getExtension(UnittestProto.repeatedDoubleExtension  , 1));
    assertEqualsExactType(false, message.getExtension(UnittestProto.repeatedBoolExtension    , 1));
    assertEqualsExactType("315", message.getExtension(UnittestProto.repeatedStringExtension  , 1));
    assertEqualsExactType(toBytes("316"), message.getExtension(UnittestProto.repeatedBytesExtension, 1));

    assertEqualsExactType(317, message.getExtension(UnittestProto.repeatedGroupExtension         , 1).getA());
    assertEqualsExactType(318, message.getExtension(UnittestProto.repeatedNestedMessageExtension , 1).getBb());
    assertEqualsExactType(319, message.getExtension(UnittestProto.repeatedForeignMessageExtension, 1).getC());
    assertEqualsExactType(320, message.getExtension(UnittestProto.repeatedImportMessageExtension , 1).getD());

    assertEqualsExactType(TestAllTypes.NestedEnum.BAZ,
      message.getExtension(UnittestProto.repeatedNestedEnumExtension, 1));
    assertEqualsExactType(ForeignEnum.FOREIGN_BAZ,
      message.getExtension(UnittestProto.repeatedForeignEnumExtension, 1));
    assertEqualsExactType(ImportEnum.IMPORT_BAZ,
      message.getExtension(UnittestProto.repeatedImportEnumExtension, 1));

    assertEqualsExactType("324", message.getExtension(UnittestProto.repeatedStringPieceExtension, 1));
    assertEqualsExactType("325", message.getExtension(UnittestProto.repeatedCordExtension, 1));

    // -----------------------------------------------------------------

    Assert.assertTrue(message.hasExtension(UnittestProto.defaultInt32Extension   ));
    Assert.assertTrue(message.hasExtension(UnittestProto.defaultInt64Extension   ));
    Assert.assertTrue(message.hasExtension(UnittestProto.defaultUint32Extension  ));
    Assert.assertTrue(message.hasExtension(UnittestProto.defaultUint64Extension  ));
    Assert.assertTrue(message.hasExtension(UnittestProto.defaultSint32Extension  ));
    Assert.assertTrue(message.hasExtension(UnittestProto.defaultSint64Extension  ));
    Assert.assertTrue(message.hasExtension(UnittestProto.defaultFixed32Extension ));
    Assert.assertTrue(message.hasExtension(UnittestProto.defaultFixed64Extension ));
    Assert.assertTrue(message.hasExtension(UnittestProto.defaultSfixed32Extension));
    Assert.assertTrue(message.hasExtension(UnittestProto.defaultSfixed64Extension));
    Assert.assertTrue(message.hasExtension(UnittestProto.defaultFloatExtension   ));
    Assert.assertTrue(message.hasExtension(UnittestProto.defaultDoubleExtension  ));
    Assert.assertTrue(message.hasExtension(UnittestProto.defaultBoolExtension    ));
    Assert.assertTrue(message.hasExtension(UnittestProto.defaultStringExtension  ));
    Assert.assertTrue(message.hasExtension(UnittestProto.defaultBytesExtension   ));

    Assert.assertTrue(message.hasExtension(UnittestProto.defaultNestedEnumExtension ));
    Assert.assertTrue(message.hasExtension(UnittestProto.defaultForeignEnumExtension));
    Assert.assertTrue(message.hasExtension(UnittestProto.defaultImportEnumExtension ));

    Assert.assertTrue(message.hasExtension(UnittestProto.defaultStringPieceExtension));
    Assert.assertTrue(message.hasExtension(UnittestProto.defaultCordExtension));

    assertEqualsExactType(401  , message.getExtension(UnittestProto.defaultInt32Extension   ));
    assertEqualsExactType(402L , message.getExtension(UnittestProto.defaultInt64Extension   ));
    assertEqualsExactType(403  , message.getExtension(UnittestProto.defaultUint32Extension  ));
    assertEqualsExactType(404L , message.getExtension(UnittestProto.defaultUint64Extension  ));
    assertEqualsExactType(405  , message.getExtension(UnittestProto.defaultSint32Extension  ));
    assertEqualsExactType(406L , message.getExtension(UnittestProto.defaultSint64Extension  ));
    assertEqualsExactType(407  , message.getExtension(UnittestProto.defaultFixed32Extension ));
    assertEqualsExactType(408L , message.getExtension(UnittestProto.defaultFixed64Extension ));
    assertEqualsExactType(409  , message.getExtension(UnittestProto.defaultSfixed32Extension));
    assertEqualsExactType(410L , message.getExtension(UnittestProto.defaultSfixed64Extension));
    assertEqualsExactType(411F , message.getExtension(UnittestProto.defaultFloatExtension   ));
    assertEqualsExactType(412D , message.getExtension(UnittestProto.defaultDoubleExtension  ));
    assertEqualsExactType(false, message.getExtension(UnittestProto.defaultBoolExtension    ));
    assertEqualsExactType("415", message.getExtension(UnittestProto.defaultStringExtension  ));
    assertEqualsExactType(toBytes("416"), message.getExtension(UnittestProto.defaultBytesExtension));

    assertEqualsExactType(TestAllTypes.NestedEnum.FOO,
      message.getExtension(UnittestProto.defaultNestedEnumExtension ));
    assertEqualsExactType(ForeignEnum.FOREIGN_FOO,
      message.getExtension(UnittestProto.defaultForeignEnumExtension));
    assertEqualsExactType(ImportEnum.IMPORT_FOO,
      message.getExtension(UnittestProto.defaultImportEnumExtension));

    assertEqualsExactType("424", message.getExtension(UnittestProto.defaultStringPieceExtension));
    assertEqualsExactType("425", message.getExtension(UnittestProto.defaultCordExtension));
  }

  // -------------------------------------------------------------------

  /**
   * Assert (using {@code junit.framework.Assert}} that all extensions of
   * {@code message} are cleared, and that getting the extensions returns their
   * default values.
   */
  public static void assertExtensionsClear(TestAllExtensions message) {
    // hasBlah() should initially be false for all optional fields.
    Assert.assertFalse(message.hasExtension(UnittestProto.optionalInt32Extension   ));
    Assert.assertFalse(message.hasExtension(UnittestProto.optionalInt64Extension   ));
    Assert.assertFalse(message.hasExtension(UnittestProto.optionalUint32Extension  ));
    Assert.assertFalse(message.hasExtension(UnittestProto.optionalUint64Extension  ));
    Assert.assertFalse(message.hasExtension(UnittestProto.optionalSint32Extension  ));
    Assert.assertFalse(message.hasExtension(UnittestProto.optionalSint64Extension  ));
    Assert.assertFalse(message.hasExtension(UnittestProto.optionalFixed32Extension ));
    Assert.assertFalse(message.hasExtension(UnittestProto.optionalFixed64Extension ));
    Assert.assertFalse(message.hasExtension(UnittestProto.optionalSfixed32Extension));
    Assert.assertFalse(message.hasExtension(UnittestProto.optionalSfixed64Extension));
    Assert.assertFalse(message.hasExtension(UnittestProto.optionalFloatExtension   ));
    Assert.assertFalse(message.hasExtension(UnittestProto.optionalDoubleExtension  ));
    Assert.assertFalse(message.hasExtension(UnittestProto.optionalBoolExtension    ));
    Assert.assertFalse(message.hasExtension(UnittestProto.optionalStringExtension  ));
    Assert.assertFalse(message.hasExtension(UnittestProto.optionalBytesExtension   ));

    Assert.assertFalse(message.hasExtension(UnittestProto.optionalGroupExtension         ));
    Assert.assertFalse(message.hasExtension(UnittestProto.optionalNestedMessageExtension ));
    Assert.assertFalse(message.hasExtension(UnittestProto.optionalForeignMessageExtension));
    Assert.assertFalse(message.hasExtension(UnittestProto.optionalImportMessageExtension ));

    Assert.assertFalse(message.hasExtension(UnittestProto.optionalNestedEnumExtension ));
    Assert.assertFalse(message.hasExtension(UnittestProto.optionalForeignEnumExtension));
    Assert.assertFalse(message.hasExtension(UnittestProto.optionalImportEnumExtension ));

    Assert.assertFalse(message.hasExtension(UnittestProto.optionalStringPieceExtension));
    Assert.assertFalse(message.hasExtension(UnittestProto.optionalCordExtension));

    // Optional fields without defaults are set to zero or something like it.
    assertEqualsExactType(0    , message.getExtension(UnittestProto.optionalInt32Extension   ));
    assertEqualsExactType(0L   , message.getExtension(UnittestProto.optionalInt64Extension   ));
    assertEqualsExactType(0    , message.getExtension(UnittestProto.optionalUint32Extension  ));
    assertEqualsExactType(0L   , message.getExtension(UnittestProto.optionalUint64Extension  ));
    assertEqualsExactType(0    , message.getExtension(UnittestProto.optionalSint32Extension  ));
    assertEqualsExactType(0L   , message.getExtension(UnittestProto.optionalSint64Extension  ));
    assertEqualsExactType(0    , message.getExtension(UnittestProto.optionalFixed32Extension ));
    assertEqualsExactType(0L   , message.getExtension(UnittestProto.optionalFixed64Extension ));
    assertEqualsExactType(0    , message.getExtension(UnittestProto.optionalSfixed32Extension));
    assertEqualsExactType(0L   , message.getExtension(UnittestProto.optionalSfixed64Extension));
    assertEqualsExactType(0F   , message.getExtension(UnittestProto.optionalFloatExtension   ));
    assertEqualsExactType(0D   , message.getExtension(UnittestProto.optionalDoubleExtension  ));
    assertEqualsExactType(false, message.getExtension(UnittestProto.optionalBoolExtension    ));
    assertEqualsExactType(""   , message.getExtension(UnittestProto.optionalStringExtension  ));
    assertEqualsExactType(ByteString.EMPTY, message.getExtension(UnittestProto.optionalBytesExtension));

    // Embedded messages should also be clear.
    Assert.assertFalse(message.getExtension(UnittestProto.optionalGroupExtension         ).hasA());
    Assert.assertFalse(message.getExtension(UnittestProto.optionalNestedMessageExtension ).hasBb());
    Assert.assertFalse(message.getExtension(UnittestProto.optionalForeignMessageExtension).hasC());
    Assert.assertFalse(message.getExtension(UnittestProto.optionalImportMessageExtension ).hasD());

    assertEqualsExactType(0, message.getExtension(UnittestProto.optionalGroupExtension         ).getA());
    assertEqualsExactType(0, message.getExtension(UnittestProto.optionalNestedMessageExtension ).getBb());
    assertEqualsExactType(0, message.getExtension(UnittestProto.optionalForeignMessageExtension).getC());
    assertEqualsExactType(0, message.getExtension(UnittestProto.optionalImportMessageExtension ).getD());

    // Enums without defaults are set to the first value in the enum.
    assertEqualsExactType(TestAllTypes.NestedEnum.FOO,
      message.getExtension(UnittestProto.optionalNestedEnumExtension ));
    assertEqualsExactType(ForeignEnum.FOREIGN_FOO,
      message.getExtension(UnittestProto.optionalForeignEnumExtension));
    assertEqualsExactType(ImportEnum.IMPORT_FOO,
      message.getExtension(UnittestProto.optionalImportEnumExtension));

    assertEqualsExactType("", message.getExtension(UnittestProto.optionalStringPieceExtension));
    assertEqualsExactType("", message.getExtension(UnittestProto.optionalCordExtension));

    // Repeated fields are empty.
    Assert.assertEquals(0, message.getExtensionCount(UnittestProto.repeatedInt32Extension   ));
    Assert.assertEquals(0, message.getExtensionCount(UnittestProto.repeatedInt64Extension   ));
    Assert.assertEquals(0, message.getExtensionCount(UnittestProto.repeatedUint32Extension  ));
    Assert.assertEquals(0, message.getExtensionCount(UnittestProto.repeatedUint64Extension  ));
    Assert.assertEquals(0, message.getExtensionCount(UnittestProto.repeatedSint32Extension  ));
    Assert.assertEquals(0, message.getExtensionCount(UnittestProto.repeatedSint64Extension  ));
    Assert.assertEquals(0, message.getExtensionCount(UnittestProto.repeatedFixed32Extension ));
    Assert.assertEquals(0, message.getExtensionCount(UnittestProto.repeatedFixed64Extension ));
    Assert.assertEquals(0, message.getExtensionCount(UnittestProto.repeatedSfixed32Extension));
    Assert.assertEquals(0, message.getExtensionCount(UnittestProto.repeatedSfixed64Extension));
    Assert.assertEquals(0, message.getExtensionCount(UnittestProto.repeatedFloatExtension   ));
    Assert.assertEquals(0, message.getExtensionCount(UnittestProto.repeatedDoubleExtension  ));
    Assert.assertEquals(0, message.getExtensionCount(UnittestProto.repeatedBoolExtension    ));
    Assert.assertEquals(0, message.getExtensionCount(UnittestProto.repeatedStringExtension  ));
    Assert.assertEquals(0, message.getExtensionCount(UnittestProto.repeatedBytesExtension   ));

    Assert.assertEquals(0, message.getExtensionCount(UnittestProto.repeatedGroupExtension         ));
    Assert.assertEquals(0, message.getExtensionCount(UnittestProto.repeatedNestedMessageExtension ));
    Assert.assertEquals(0, message.getExtensionCount(UnittestProto.repeatedForeignMessageExtension));
    Assert.assertEquals(0, message.getExtensionCount(UnittestProto.repeatedImportMessageExtension ));
    Assert.assertEquals(0, message.getExtensionCount(UnittestProto.repeatedNestedEnumExtension    ));
    Assert.assertEquals(0, message.getExtensionCount(UnittestProto.repeatedForeignEnumExtension   ));
    Assert.assertEquals(0, message.getExtensionCount(UnittestProto.repeatedImportEnumExtension    ));

    Assert.assertEquals(0, message.getExtensionCount(UnittestProto.repeatedStringPieceExtension));
    Assert.assertEquals(0, message.getExtensionCount(UnittestProto.repeatedCordExtension));

    // hasBlah() should also be false for all default fields.
    Assert.assertFalse(message.hasExtension(UnittestProto.defaultInt32Extension   ));
    Assert.assertFalse(message.hasExtension(UnittestProto.defaultInt64Extension   ));
    Assert.assertFalse(message.hasExtension(UnittestProto.defaultUint32Extension  ));
    Assert.assertFalse(message.hasExtension(UnittestProto.defaultUint64Extension  ));
    Assert.assertFalse(message.hasExtension(UnittestProto.defaultSint32Extension  ));
    Assert.assertFalse(message.hasExtension(UnittestProto.defaultSint64Extension  ));
    Assert.assertFalse(message.hasExtension(UnittestProto.defaultFixed32Extension ));
    Assert.assertFalse(message.hasExtension(UnittestProto.defaultFixed64Extension ));
    Assert.assertFalse(message.hasExtension(UnittestProto.defaultSfixed32Extension));
    Assert.assertFalse(message.hasExtension(UnittestProto.defaultSfixed64Extension));
    Assert.assertFalse(message.hasExtension(UnittestProto.defaultFloatExtension   ));
    Assert.assertFalse(message.hasExtension(UnittestProto.defaultDoubleExtension  ));
    Assert.assertFalse(message.hasExtension(UnittestProto.defaultBoolExtension    ));
    Assert.assertFalse(message.hasExtension(UnittestProto.defaultStringExtension  ));
    Assert.assertFalse(message.hasExtension(UnittestProto.defaultBytesExtension   ));

    Assert.assertFalse(message.hasExtension(UnittestProto.defaultNestedEnumExtension ));
    Assert.assertFalse(message.hasExtension(UnittestProto.defaultForeignEnumExtension));
    Assert.assertFalse(message.hasExtension(UnittestProto.defaultImportEnumExtension ));

    Assert.assertFalse(message.hasExtension(UnittestProto.defaultStringPieceExtension));
    Assert.assertFalse(message.hasExtension(UnittestProto.defaultCordExtension));

    // Fields with defaults have their default values (duh).
    assertEqualsExactType( 41    , message.getExtension(UnittestProto.defaultInt32Extension   ));
    assertEqualsExactType( 42L   , message.getExtension(UnittestProto.defaultInt64Extension   ));
    assertEqualsExactType( 43    , message.getExtension(UnittestProto.defaultUint32Extension  ));
    assertEqualsExactType( 44L   , message.getExtension(UnittestProto.defaultUint64Extension  ));
    assertEqualsExactType(-45    , message.getExtension(UnittestProto.defaultSint32Extension  ));
    assertEqualsExactType( 46L   , message.getExtension(UnittestProto.defaultSint64Extension  ));
    assertEqualsExactType( 47    , message.getExtension(UnittestProto.defaultFixed32Extension ));
    assertEqualsExactType( 48L   , message.getExtension(UnittestProto.defaultFixed64Extension ));
    assertEqualsExactType( 49    , message.getExtension(UnittestProto.defaultSfixed32Extension));
    assertEqualsExactType(-50L   , message.getExtension(UnittestProto.defaultSfixed64Extension));
    assertEqualsExactType( 51.5F , message.getExtension(UnittestProto.defaultFloatExtension   ));
    assertEqualsExactType( 52e3D , message.getExtension(UnittestProto.defaultDoubleExtension  ));
    assertEqualsExactType(true   , message.getExtension(UnittestProto.defaultBoolExtension    ));
    assertEqualsExactType("hello", message.getExtension(UnittestProto.defaultStringExtension  ));
    assertEqualsExactType(toBytes("world"), message.getExtension(UnittestProto.defaultBytesExtension));

    assertEqualsExactType(TestAllTypes.NestedEnum.BAR,
      message.getExtension(UnittestProto.defaultNestedEnumExtension ));
    assertEqualsExactType(ForeignEnum.FOREIGN_BAR,
      message.getExtension(UnittestProto.defaultForeignEnumExtension));
    assertEqualsExactType(ImportEnum.IMPORT_BAR,
      message.getExtension(UnittestProto.defaultImportEnumExtension));

    assertEqualsExactType("abc", message.getExtension(UnittestProto.defaultStringPieceExtension));
    assertEqualsExactType("123", message.getExtension(UnittestProto.defaultCordExtension));
  }

  // -------------------------------------------------------------------

  /**
   * Assert (using {@code junit.framework.Assert}} that all extensions of
   * {@code message} are set to the values assigned by {@code setAllExtensions}
   * followed by {@code modifyRepeatedExtensions}.
   */
  public static void assertRepeatedExtensionsModified(
      TestAllExtensions message) {
    // ModifyRepeatedFields only sets the second repeated element of each
    // field.  In addition to verifying this, we also verify that the first
    // element and size were *not* modified.
    Assert.assertEquals(2, message.getExtensionCount(UnittestProto.repeatedInt32Extension   ));
    Assert.assertEquals(2, message.getExtensionCount(UnittestProto.repeatedInt64Extension   ));
    Assert.assertEquals(2, message.getExtensionCount(UnittestProto.repeatedUint32Extension  ));
    Assert.assertEquals(2, message.getExtensionCount(UnittestProto.repeatedUint64Extension  ));
    Assert.assertEquals(2, message.getExtensionCount(UnittestProto.repeatedSint32Extension  ));
    Assert.assertEquals(2, message.getExtensionCount(UnittestProto.repeatedSint64Extension  ));
    Assert.assertEquals(2, message.getExtensionCount(UnittestProto.repeatedFixed32Extension ));
    Assert.assertEquals(2, message.getExtensionCount(UnittestProto.repeatedFixed64Extension ));
    Assert.assertEquals(2, message.getExtensionCount(UnittestProto.repeatedSfixed32Extension));
    Assert.assertEquals(2, message.getExtensionCount(UnittestProto.repeatedSfixed64Extension));
    Assert.assertEquals(2, message.getExtensionCount(UnittestProto.repeatedFloatExtension   ));
    Assert.assertEquals(2, message.getExtensionCount(UnittestProto.repeatedDoubleExtension  ));
    Assert.assertEquals(2, message.getExtensionCount(UnittestProto.repeatedBoolExtension    ));
    Assert.assertEquals(2, message.getExtensionCount(UnittestProto.repeatedStringExtension  ));
    Assert.assertEquals(2, message.getExtensionCount(UnittestProto.repeatedBytesExtension   ));

    Assert.assertEquals(2, message.getExtensionCount(UnittestProto.repeatedGroupExtension         ));
    Assert.assertEquals(2, message.getExtensionCount(UnittestProto.repeatedNestedMessageExtension ));
    Assert.assertEquals(2, message.getExtensionCount(UnittestProto.repeatedForeignMessageExtension));
    Assert.assertEquals(2, message.getExtensionCount(UnittestProto.repeatedImportMessageExtension ));
    Assert.assertEquals(2, message.getExtensionCount(UnittestProto.repeatedNestedEnumExtension    ));
    Assert.assertEquals(2, message.getExtensionCount(UnittestProto.repeatedForeignEnumExtension   ));
    Assert.assertEquals(2, message.getExtensionCount(UnittestProto.repeatedImportEnumExtension    ));

    Assert.assertEquals(2, message.getExtensionCount(UnittestProto.repeatedStringPieceExtension));
    Assert.assertEquals(2, message.getExtensionCount(UnittestProto.repeatedCordExtension));

    assertEqualsExactType(201  , message.getExtension(UnittestProto.repeatedInt32Extension   , 0));
    assertEqualsExactType(202L , message.getExtension(UnittestProto.repeatedInt64Extension   , 0));
    assertEqualsExactType(203  , message.getExtension(UnittestProto.repeatedUint32Extension  , 0));
    assertEqualsExactType(204L , message.getExtension(UnittestProto.repeatedUint64Extension  , 0));
    assertEqualsExactType(205  , message.getExtension(UnittestProto.repeatedSint32Extension  , 0));
    assertEqualsExactType(206L , message.getExtension(UnittestProto.repeatedSint64Extension  , 0));
    assertEqualsExactType(207  , message.getExtension(UnittestProto.repeatedFixed32Extension , 0));
    assertEqualsExactType(208L , message.getExtension(UnittestProto.repeatedFixed64Extension , 0));
    assertEqualsExactType(209  , message.getExtension(UnittestProto.repeatedSfixed32Extension, 0));
    assertEqualsExactType(210L , message.getExtension(UnittestProto.repeatedSfixed64Extension, 0));
    assertEqualsExactType(211F , message.getExtension(UnittestProto.repeatedFloatExtension   , 0));
    assertEqualsExactType(212D , message.getExtension(UnittestProto.repeatedDoubleExtension  , 0));
    assertEqualsExactType(true , message.getExtension(UnittestProto.repeatedBoolExtension    , 0));
    assertEqualsExactType("215", message.getExtension(UnittestProto.repeatedStringExtension  , 0));
    assertEqualsExactType(toBytes("216"), message.getExtension(UnittestProto.repeatedBytesExtension, 0));

    assertEqualsExactType(217, message.getExtension(UnittestProto.repeatedGroupExtension         , 0).getA());
    assertEqualsExactType(218, message.getExtension(UnittestProto.repeatedNestedMessageExtension , 0).getBb());
    assertEqualsExactType(219, message.getExtension(UnittestProto.repeatedForeignMessageExtension, 0).getC());
    assertEqualsExactType(220, message.getExtension(UnittestProto.repeatedImportMessageExtension , 0).getD());

    assertEqualsExactType(TestAllTypes.NestedEnum.BAR,
      message.getExtension(UnittestProto.repeatedNestedEnumExtension, 0));
    assertEqualsExactType(ForeignEnum.FOREIGN_BAR,
      message.getExtension(UnittestProto.repeatedForeignEnumExtension, 0));
    assertEqualsExactType(ImportEnum.IMPORT_BAR,
      message.getExtension(UnittestProto.repeatedImportEnumExtension, 0));

    assertEqualsExactType("224", message.getExtension(UnittestProto.repeatedStringPieceExtension, 0));
    assertEqualsExactType("225", message.getExtension(UnittestProto.repeatedCordExtension, 0));

    // Actually verify the second (modified) elements now.
    assertEqualsExactType(501  , message.getExtension(UnittestProto.repeatedInt32Extension   , 1));
    assertEqualsExactType(502L , message.getExtension(UnittestProto.repeatedInt64Extension   , 1));
    assertEqualsExactType(503  , message.getExtension(UnittestProto.repeatedUint32Extension  , 1));
    assertEqualsExactType(504L , message.getExtension(UnittestProto.repeatedUint64Extension  , 1));
    assertEqualsExactType(505  , message.getExtension(UnittestProto.repeatedSint32Extension  , 1));
    assertEqualsExactType(506L , message.getExtension(UnittestProto.repeatedSint64Extension  , 1));
    assertEqualsExactType(507  , message.getExtension(UnittestProto.repeatedFixed32Extension , 1));
    assertEqualsExactType(508L , message.getExtension(UnittestProto.repeatedFixed64Extension , 1));
    assertEqualsExactType(509  , message.getExtension(UnittestProto.repeatedSfixed32Extension, 1));
    assertEqualsExactType(510L , message.getExtension(UnittestProto.repeatedSfixed64Extension, 1));
    assertEqualsExactType(511F , message.getExtension(UnittestProto.repeatedFloatExtension   , 1));
    assertEqualsExactType(512D , message.getExtension(UnittestProto.repeatedDoubleExtension  , 1));
    assertEqualsExactType(true , message.getExtension(UnittestProto.repeatedBoolExtension    , 1));
    assertEqualsExactType("515", message.getExtension(UnittestProto.repeatedStringExtension  , 1));
    assertEqualsExactType(toBytes("516"), message.getExtension(UnittestProto.repeatedBytesExtension, 1));

    assertEqualsExactType(517, message.getExtension(UnittestProto.repeatedGroupExtension         , 1).getA());
    assertEqualsExactType(518, message.getExtension(UnittestProto.repeatedNestedMessageExtension , 1).getBb());
    assertEqualsExactType(519, message.getExtension(UnittestProto.repeatedForeignMessageExtension, 1).getC());
    assertEqualsExactType(520, message.getExtension(UnittestProto.repeatedImportMessageExtension , 1).getD());

    assertEqualsExactType(TestAllTypes.NestedEnum.FOO,
      message.getExtension(UnittestProto.repeatedNestedEnumExtension, 1));
    assertEqualsExactType(ForeignEnum.FOREIGN_FOO,
      message.getExtension(UnittestProto.repeatedForeignEnumExtension, 1));
    assertEqualsExactType(ImportEnum.IMPORT_FOO,
      message.getExtension(UnittestProto.repeatedImportEnumExtension, 1));

    assertEqualsExactType("524", message.getExtension(UnittestProto.repeatedStringPieceExtension, 1));
    assertEqualsExactType("525", message.getExtension(UnittestProto.repeatedCordExtension, 1));
  }

  // ===================================================================

  /**
   * Performs the same things that the methods of {@code TestUtil} do, but
   * via the reflection interface.  This is its own class because it needs
   * to know what descriptor to use.
   */
  public static class ReflectionTester {
    private final Descriptors.Descriptor baseDescriptor;
    private final ExtensionRegistry extensionRegistry;

    private final Descriptors.FileDescriptor file;
    private final Descriptors.FileDescriptor importFile;

    private final Descriptors.Descriptor optionalGroup;
    private final Descriptors.Descriptor repeatedGroup;
    private final Descriptors.Descriptor nestedMessage;
    private final Descriptors.Descriptor foreignMessage;
    private final Descriptors.Descriptor importMessage;

    private final Descriptors.FieldDescriptor groupA;
    private final Descriptors.FieldDescriptor repeatedGroupA;
    private final Descriptors.FieldDescriptor nestedB;
    private final Descriptors.FieldDescriptor foreignC;
    private final Descriptors.FieldDescriptor importD;

    private final Descriptors.EnumDescriptor nestedEnum;
    private final Descriptors.EnumDescriptor foreignEnum;
    private final Descriptors.EnumDescriptor importEnum;

    private final Descriptors.EnumValueDescriptor nestedFoo;
    private final Descriptors.EnumValueDescriptor nestedBar;
    private final Descriptors.EnumValueDescriptor nestedBaz;
    private final Descriptors.EnumValueDescriptor foreignFoo;
    private final Descriptors.EnumValueDescriptor foreignBar;
    private final Descriptors.EnumValueDescriptor foreignBaz;
    private final Descriptors.EnumValueDescriptor importFoo;
    private final Descriptors.EnumValueDescriptor importBar;
    private final Descriptors.EnumValueDescriptor importBaz;

    /**
     * Construct a {@code ReflectionTester} that will expect messages using
     * the given descriptor.
     *
     * Normally {@code baseDescriptor} should be a descriptor for the type
     * {@code TestAllTypes}, defined in
     * {@code google/protobuf/unittest.proto}.  However, if
     * {@code extensionRegistry} is non-null, then {@code baseDescriptor} should
     * be for {@code TestAllExtensions} instead, and instead of reading and
     * writing normal fields, the tester will read and write extensions.
     * All of {@code TestAllExtensions}' extensions must be registered in the
     * registry.
     */
    public ReflectionTester(Descriptors.Descriptor baseDescriptor,
                            ExtensionRegistry extensionRegistry) {
      this.baseDescriptor = baseDescriptor;
      this.extensionRegistry = extensionRegistry;

      this.file = baseDescriptor.getFile();
      Assert.assertEquals(1, file.getDependencies().size());
      this.importFile = file.getDependencies().get(0);

      Descriptors.Descriptor testAllTypes;
      if (extensionRegistry == null) {
        testAllTypes = baseDescriptor;
      } else {
        testAllTypes = file.findMessageTypeByName("TestAllTypes");
        Assert.assertNotNull(testAllTypes);
      }

      if (extensionRegistry == null) {
        this.optionalGroup =
          baseDescriptor.findNestedTypeByName("OptionalGroup");
        this.repeatedGroup =
          baseDescriptor.findNestedTypeByName("RepeatedGroup");
      } else {
        this.optionalGroup =
          file.findMessageTypeByName("OptionalGroup_extension");
        this.repeatedGroup =
          file.findMessageTypeByName("RepeatedGroup_extension");
      }
      this.nestedMessage = testAllTypes.findNestedTypeByName("NestedMessage");
      this.foreignMessage = file.findMessageTypeByName("ForeignMessage");
      this.importMessage = importFile.findMessageTypeByName("ImportMessage");

      this.nestedEnum = testAllTypes.findEnumTypeByName("NestedEnum");
      this.foreignEnum = file.findEnumTypeByName("ForeignEnum");
      this.importEnum = importFile.findEnumTypeByName("ImportEnum");

      Assert.assertNotNull(optionalGroup );
      Assert.assertNotNull(repeatedGroup );
      Assert.assertNotNull(nestedMessage );
      Assert.assertNotNull(foreignMessage);
      Assert.assertNotNull(importMessage );
      Assert.assertNotNull(nestedEnum    );
      Assert.assertNotNull(foreignEnum   );
      Assert.assertNotNull(importEnum    );

      this.nestedB  = nestedMessage .findFieldByName("bb");
      this.foreignC = foreignMessage.findFieldByName("c");
      this.importD  = importMessage .findFieldByName("d");
      this.nestedFoo = nestedEnum.findValueByName("FOO");
      this.nestedBar = nestedEnum.findValueByName("BAR");
      this.nestedBaz = nestedEnum.findValueByName("BAZ");
      this.foreignFoo = foreignEnum.findValueByName("FOREIGN_FOO");
      this.foreignBar = foreignEnum.findValueByName("FOREIGN_BAR");
      this.foreignBaz = foreignEnum.findValueByName("FOREIGN_BAZ");
      this.importFoo = importEnum.findValueByName("IMPORT_FOO");
      this.importBar = importEnum.findValueByName("IMPORT_BAR");
      this.importBaz = importEnum.findValueByName("IMPORT_BAZ");

      this.groupA = optionalGroup.findFieldByName("a");
      this.repeatedGroupA = repeatedGroup.findFieldByName("a");

      Assert.assertNotNull(groupA        );
      Assert.assertNotNull(repeatedGroupA);
      Assert.assertNotNull(nestedB       );
      Assert.assertNotNull(foreignC      );
      Assert.assertNotNull(importD       );
      Assert.assertNotNull(nestedFoo     );
      Assert.assertNotNull(nestedBar     );
      Assert.assertNotNull(nestedBaz     );
      Assert.assertNotNull(foreignFoo    );
      Assert.assertNotNull(foreignBar    );
      Assert.assertNotNull(foreignBaz    );
      Assert.assertNotNull(importFoo     );
      Assert.assertNotNull(importBar     );
      Assert.assertNotNull(importBaz     );
    }

    /**
     * Shorthand to get a FieldDescriptor for a field of unittest::TestAllTypes.
     */
    private Descriptors.FieldDescriptor f(String name) {
      Descriptors.FieldDescriptor result;
      if (extensionRegistry == null) {
        result = baseDescriptor.findFieldByName(name);
      } else {
        result = file.findExtensionByName(name + "_extension");
      }
      Assert.assertNotNull(result);
      return result;
    }

    /**
     * Calls {@code parent.newBuilderForField()} or uses the
     * {@code ExtensionRegistry} to find an appropriate builder, depending
     * on what type is being tested.
     */
    private Message.Builder newBuilderForField(
        Message.Builder parent, Descriptors.FieldDescriptor field) {
      if (extensionRegistry == null) {
        return parent.newBuilderForField(field);
      } else {
        ExtensionRegistry.ExtensionInfo extension =
          extensionRegistry.findExtensionByNumber(field.getContainingType(),
                                                  field.getNumber());
        Assert.assertNotNull(extension);
        Assert.assertNotNull(extension.defaultInstance);
        return extension.defaultInstance.newBuilderForType();
      }
    }

    // -------------------------------------------------------------------

    /**
     * Set every field of {@code message} to the values expected by
     * {@code assertAllFieldsSet()}, using the {@link Message.Builder}
     * reflection interface.
     */
    void setAllFieldsViaReflection(Message.Builder message) {
      message.setField(f("optional_int32"   ), 101 );
      message.setField(f("optional_int64"   ), 102L);
      message.setField(f("optional_uint32"  ), 103 );
      message.setField(f("optional_uint64"  ), 104L);
      message.setField(f("optional_sint32"  ), 105 );
      message.setField(f("optional_sint64"  ), 106L);
      message.setField(f("optional_fixed32" ), 107 );
      message.setField(f("optional_fixed64" ), 108L);
      message.setField(f("optional_sfixed32"), 109 );
      message.setField(f("optional_sfixed64"), 110L);
      message.setField(f("optional_float"   ), 111F);
      message.setField(f("optional_double"  ), 112D);
      message.setField(f("optional_bool"    ), true);
      message.setField(f("optional_string"  ), "115");
      message.setField(f("optional_bytes"   ), toBytes("116"));

      message.setField(f("optionalgroup"),
        newBuilderForField(message, f("optionalgroup"))
               .setField(groupA, 117).build());
      message.setField(f("optional_nested_message"),
        newBuilderForField(message, f("optional_nested_message"))
               .setField(nestedB, 118).build());
      message.setField(f("optional_foreign_message"),
        newBuilderForField(message, f("optional_foreign_message"))
               .setField(foreignC, 119).build());
      message.setField(f("optional_import_message"),
        newBuilderForField(message, f("optional_import_message"))
               .setField(importD, 120).build());

      message.setField(f("optional_nested_enum" ),  nestedBaz);
      message.setField(f("optional_foreign_enum"), foreignBaz);
      message.setField(f("optional_import_enum" ),  importBaz);

      message.setField(f("optional_string_piece" ), "124");
      message.setField(f("optional_cord" ), "125");

      // -----------------------------------------------------------------

      message.addRepeatedField(f("repeated_int32"   ), 201 );
      message.addRepeatedField(f("repeated_int64"   ), 202L);
      message.addRepeatedField(f("repeated_uint32"  ), 203 );
      message.addRepeatedField(f("repeated_uint64"  ), 204L);
      message.addRepeatedField(f("repeated_sint32"  ), 205 );
      message.addRepeatedField(f("repeated_sint64"  ), 206L);
      message.addRepeatedField(f("repeated_fixed32" ), 207 );
      message.addRepeatedField(f("repeated_fixed64" ), 208L);
      message.addRepeatedField(f("repeated_sfixed32"), 209 );
      message.addRepeatedField(f("repeated_sfixed64"), 210L);
      message.addRepeatedField(f("repeated_float"   ), 211F);
      message.addRepeatedField(f("repeated_double"  ), 212D);
      message.addRepeatedField(f("repeated_bool"    ), true);
      message.addRepeatedField(f("repeated_string"  ), "215");
      message.addRepeatedField(f("repeated_bytes"   ), toBytes("216"));

      message.addRepeatedField(f("repeatedgroup"),
        newBuilderForField(message, f("repeatedgroup"))
               .setField(repeatedGroupA, 217).build());
      message.addRepeatedField(f("repeated_nested_message"),
        newBuilderForField(message, f("repeated_nested_message"))
               .setField(nestedB, 218).build());
      message.addRepeatedField(f("repeated_foreign_message"),
        newBuilderForField(message, f("repeated_foreign_message"))
               .setField(foreignC, 219).build());
      message.addRepeatedField(f("repeated_import_message"),
        newBuilderForField(message, f("repeated_import_message"))
               .setField(importD, 220).build());

      message.addRepeatedField(f("repeated_nested_enum" ),  nestedBar);
      message.addRepeatedField(f("repeated_foreign_enum"), foreignBar);
      message.addRepeatedField(f("repeated_import_enum" ),  importBar);

      message.addRepeatedField(f("repeated_string_piece" ), "224");
      message.addRepeatedField(f("repeated_cord" ), "225");

      // Add a second one of each field.
      message.addRepeatedField(f("repeated_int32"   ), 301 );
      message.addRepeatedField(f("repeated_int64"   ), 302L);
      message.addRepeatedField(f("repeated_uint32"  ), 303 );
      message.addRepeatedField(f("repeated_uint64"  ), 304L);
      message.addRepeatedField(f("repeated_sint32"  ), 305 );
      message.addRepeatedField(f("repeated_sint64"  ), 306L);
      message.addRepeatedField(f("repeated_fixed32" ), 307 );
      message.addRepeatedField(f("repeated_fixed64" ), 308L);
      message.addRepeatedField(f("repeated_sfixed32"), 309 );
      message.addRepeatedField(f("repeated_sfixed64"), 310L);
      message.addRepeatedField(f("repeated_float"   ), 311F);
      message.addRepeatedField(f("repeated_double"  ), 312D);
      message.addRepeatedField(f("repeated_bool"    ), false);
      message.addRepeatedField(f("repeated_string"  ), "315");
      message.addRepeatedField(f("repeated_bytes"   ), toBytes("316"));

      message.addRepeatedField(f("repeatedgroup"),
        newBuilderForField(message, f("repeatedgroup"))
               .setField(repeatedGroupA, 317).build());
      message.addRepeatedField(f("repeated_nested_message"),
        newBuilderForField(message, f("repeated_nested_message"))
               .setField(nestedB, 318).build());
      message.addRepeatedField(f("repeated_foreign_message"),
        newBuilderForField(message, f("repeated_foreign_message"))
               .setField(foreignC, 319).build());
      message.addRepeatedField(f("repeated_import_message"),
        newBuilderForField(message, f("repeated_import_message"))
               .setField(importD, 320).build());

      message.addRepeatedField(f("repeated_nested_enum" ),  nestedBaz);
      message.addRepeatedField(f("repeated_foreign_enum"), foreignBaz);
      message.addRepeatedField(f("repeated_import_enum" ),  importBaz);

      message.addRepeatedField(f("repeated_string_piece" ), "324");
      message.addRepeatedField(f("repeated_cord" ), "325");

      // -----------------------------------------------------------------

      message.setField(f("default_int32"   ), 401 );
      message.setField(f("default_int64"   ), 402L);
      message.setField(f("default_uint32"  ), 403 );
      message.setField(f("default_uint64"  ), 404L);
      message.setField(f("default_sint32"  ), 405 );
      message.setField(f("default_sint64"  ), 406L);
      message.setField(f("default_fixed32" ), 407 );
      message.setField(f("default_fixed64" ), 408L);
      message.setField(f("default_sfixed32"), 409 );
      message.setField(f("default_sfixed64"), 410L);
      message.setField(f("default_float"   ), 411F);
      message.setField(f("default_double"  ), 412D);
      message.setField(f("default_bool"    ), false);
      message.setField(f("default_string"  ), "415");
      message.setField(f("default_bytes"   ), toBytes("416"));

      message.setField(f("default_nested_enum" ),  nestedFoo);
      message.setField(f("default_foreign_enum"), foreignFoo);
      message.setField(f("default_import_enum" ),  importFoo);

      message.setField(f("default_string_piece" ), "424");
      message.setField(f("default_cord" ), "425");
    }

    // -------------------------------------------------------------------

    /**
     * Modify the repeated fields of {@code message} to contain the values
     * expected by {@code assertRepeatedFieldsModified()}, using the
     * {@link Message.Builder} reflection interface.
     */
    void modifyRepeatedFieldsViaReflection(Message.Builder message) {
      message.setRepeatedField(f("repeated_int32"   ), 1, 501 );
      message.setRepeatedField(f("repeated_int64"   ), 1, 502L);
      message.setRepeatedField(f("repeated_uint32"  ), 1, 503 );
      message.setRepeatedField(f("repeated_uint64"  ), 1, 504L);
      message.setRepeatedField(f("repeated_sint32"  ), 1, 505 );
      message.setRepeatedField(f("repeated_sint64"  ), 1, 506L);
      message.setRepeatedField(f("repeated_fixed32" ), 1, 507 );
      message.setRepeatedField(f("repeated_fixed64" ), 1, 508L);
      message.setRepeatedField(f("repeated_sfixed32"), 1, 509 );
      message.setRepeatedField(f("repeated_sfixed64"), 1, 510L);
      message.setRepeatedField(f("repeated_float"   ), 1, 511F);
      message.setRepeatedField(f("repeated_double"  ), 1, 512D);
      message.setRepeatedField(f("repeated_bool"    ), 1, true);
      message.setRepeatedField(f("repeated_string"  ), 1, "515");
      message.setRepeatedField(f("repeated_bytes"   ), 1, toBytes("516"));

      message.setRepeatedField(f("repeatedgroup"), 1,
        newBuilderForField(message, f("repeatedgroup"))
               .setField(repeatedGroupA, 517).build());
      message.setRepeatedField(f("repeated_nested_message"), 1,
        newBuilderForField(message, f("repeated_nested_message"))
               .setField(nestedB, 518).build());
      message.setRepeatedField(f("repeated_foreign_message"), 1,
        newBuilderForField(message, f("repeated_foreign_message"))
               .setField(foreignC, 519).build());
      message.setRepeatedField(f("repeated_import_message"), 1,
        newBuilderForField(message, f("repeated_import_message"))
               .setField(importD, 520).build());

      message.setRepeatedField(f("repeated_nested_enum" ), 1,  nestedFoo);
      message.setRepeatedField(f("repeated_foreign_enum"), 1, foreignFoo);
      message.setRepeatedField(f("repeated_import_enum" ), 1,  importFoo);

      message.setRepeatedField(f("repeated_string_piece"), 1, "524");
      message.setRepeatedField(f("repeated_cord"), 1, "525");
    }

    // -------------------------------------------------------------------

    /**
     * Assert (using {@code junit.framework.Assert}} that all fields of
     * {@code message} are set to the values assigned by {@code setAllFields},
     * using the {@link Message} reflection interface.
     */
    public void assertAllFieldsSetViaReflection(Message message) {
      Assert.assertTrue(message.hasField(f("optional_int32"   )));
      Assert.assertTrue(message.hasField(f("optional_int64"   )));
      Assert.assertTrue(message.hasField(f("optional_uint32"  )));
      Assert.assertTrue(message.hasField(f("optional_uint64"  )));
      Assert.assertTrue(message.hasField(f("optional_sint32"  )));
      Assert.assertTrue(message.hasField(f("optional_sint64"  )));
      Assert.assertTrue(message.hasField(f("optional_fixed32" )));
      Assert.assertTrue(message.hasField(f("optional_fixed64" )));
      Assert.assertTrue(message.hasField(f("optional_sfixed32")));
      Assert.assertTrue(message.hasField(f("optional_sfixed64")));
      Assert.assertTrue(message.hasField(f("optional_float"   )));
      Assert.assertTrue(message.hasField(f("optional_double"  )));
      Assert.assertTrue(message.hasField(f("optional_bool"    )));
      Assert.assertTrue(message.hasField(f("optional_string"  )));
      Assert.assertTrue(message.hasField(f("optional_bytes"   )));

      Assert.assertTrue(message.hasField(f("optionalgroup"           )));
      Assert.assertTrue(message.hasField(f("optional_nested_message" )));
      Assert.assertTrue(message.hasField(f("optional_foreign_message")));
      Assert.assertTrue(message.hasField(f("optional_import_message" )));

      Assert.assertTrue(
        ((Message)message.getField(f("optionalgroup"))).hasField(groupA));
      Assert.assertTrue(
        ((Message)message.getField(f("optional_nested_message")))
                         .hasField(nestedB));
      Assert.assertTrue(
        ((Message)message.getField(f("optional_foreign_message")))
                         .hasField(foreignC));
      Assert.assertTrue(
        ((Message)message.getField(f("optional_import_message")))
                         .hasField(importD));

      Assert.assertTrue(message.hasField(f("optional_nested_enum" )));
      Assert.assertTrue(message.hasField(f("optional_foreign_enum")));
      Assert.assertTrue(message.hasField(f("optional_import_enum" )));

      Assert.assertTrue(message.hasField(f("optional_string_piece")));
      Assert.assertTrue(message.hasField(f("optional_cord")));

      Assert.assertEquals(101  , message.getField(f("optional_int32"   )));
      Assert.assertEquals(102L , message.getField(f("optional_int64"   )));
      Assert.assertEquals(103  , message.getField(f("optional_uint32"  )));
      Assert.assertEquals(104L , message.getField(f("optional_uint64"  )));
      Assert.assertEquals(105  , message.getField(f("optional_sint32"  )));
      Assert.assertEquals(106L , message.getField(f("optional_sint64"  )));
      Assert.assertEquals(107  , message.getField(f("optional_fixed32" )));
      Assert.assertEquals(108L , message.getField(f("optional_fixed64" )));
      Assert.assertEquals(109  , message.getField(f("optional_sfixed32")));
      Assert.assertEquals(110L , message.getField(f("optional_sfixed64")));
      Assert.assertEquals(111F , message.getField(f("optional_float"   )));
      Assert.assertEquals(112D , message.getField(f("optional_double"  )));
      Assert.assertEquals(true , message.getField(f("optional_bool"    )));
      Assert.assertEquals("115", message.getField(f("optional_string"  )));
      Assert.assertEquals(toBytes("116"), message.getField(f("optional_bytes")));

      Assert.assertEquals(117,
        ((Message)message.getField(f("optionalgroup"))).getField(groupA));
      Assert.assertEquals(118,
        ((Message)message.getField(f("optional_nested_message")))
                         .getField(nestedB));
      Assert.assertEquals(119,
        ((Message)message.getField(f("optional_foreign_message")))
                         .getField(foreignC));
      Assert.assertEquals(120,
        ((Message)message.getField(f("optional_import_message")))
                         .getField(importD));

      Assert.assertEquals( nestedBaz, message.getField(f("optional_nested_enum" )));
      Assert.assertEquals(foreignBaz, message.getField(f("optional_foreign_enum")));
      Assert.assertEquals( importBaz, message.getField(f("optional_import_enum" )));

      Assert.assertEquals("124", message.getField(f("optional_string_piece")));
      Assert.assertEquals("125", message.getField(f("optional_cord")));

      // -----------------------------------------------------------------

      Assert.assertEquals(2, message.getRepeatedFieldCount(f("repeated_int32"   )));
      Assert.assertEquals(2, message.getRepeatedFieldCount(f("repeated_int64"   )));
      Assert.assertEquals(2, message.getRepeatedFieldCount(f("repeated_uint32"  )));
      Assert.assertEquals(2, message.getRepeatedFieldCount(f("repeated_uint64"  )));
      Assert.assertEquals(2, message.getRepeatedFieldCount(f("repeated_sint32"  )));
      Assert.assertEquals(2, message.getRepeatedFieldCount(f("repeated_sint64"  )));
      Assert.assertEquals(2, message.getRepeatedFieldCount(f("repeated_fixed32" )));
      Assert.assertEquals(2, message.getRepeatedFieldCount(f("repeated_fixed64" )));
      Assert.assertEquals(2, message.getRepeatedFieldCount(f("repeated_sfixed32")));
      Assert.assertEquals(2, message.getRepeatedFieldCount(f("repeated_sfixed64")));
      Assert.assertEquals(2, message.getRepeatedFieldCount(f("repeated_float"   )));
      Assert.assertEquals(2, message.getRepeatedFieldCount(f("repeated_double"  )));
      Assert.assertEquals(2, message.getRepeatedFieldCount(f("repeated_bool"    )));
      Assert.assertEquals(2, message.getRepeatedFieldCount(f("repeated_string"  )));
      Assert.assertEquals(2, message.getRepeatedFieldCount(f("repeated_bytes"   )));

      Assert.assertEquals(2, message.getRepeatedFieldCount(f("repeatedgroup"           )));
      Assert.assertEquals(2, message.getRepeatedFieldCount(f("repeated_nested_message" )));
      Assert.assertEquals(2, message.getRepeatedFieldCount(f("repeated_foreign_message")));
      Assert.assertEquals(2, message.getRepeatedFieldCount(f("repeated_import_message" )));
      Assert.assertEquals(2, message.getRepeatedFieldCount(f("repeated_nested_enum"    )));
      Assert.assertEquals(2, message.getRepeatedFieldCount(f("repeated_foreign_enum"   )));
      Assert.assertEquals(2, message.getRepeatedFieldCount(f("repeated_import_enum"    )));

      Assert.assertEquals(2, message.getRepeatedFieldCount(f("repeated_string_piece")));
      Assert.assertEquals(2, message.getRepeatedFieldCount(f("repeated_cord")));

      Assert.assertEquals(201  , message.getRepeatedField(f("repeated_int32"   ), 0));
      Assert.assertEquals(202L , message.getRepeatedField(f("repeated_int64"   ), 0));
      Assert.assertEquals(203  , message.getRepeatedField(f("repeated_uint32"  ), 0));
      Assert.assertEquals(204L , message.getRepeatedField(f("repeated_uint64"  ), 0));
      Assert.assertEquals(205  , message.getRepeatedField(f("repeated_sint32"  ), 0));
      Assert.assertEquals(206L , message.getRepeatedField(f("repeated_sint64"  ), 0));
      Assert.assertEquals(207  , message.getRepeatedField(f("repeated_fixed32" ), 0));
      Assert.assertEquals(208L , message.getRepeatedField(f("repeated_fixed64" ), 0));
      Assert.assertEquals(209  , message.getRepeatedField(f("repeated_sfixed32"), 0));
      Assert.assertEquals(210L , message.getRepeatedField(f("repeated_sfixed64"), 0));
      Assert.assertEquals(211F , message.getRepeatedField(f("repeated_float"   ), 0));
      Assert.assertEquals(212D , message.getRepeatedField(f("repeated_double"  ), 0));
      Assert.assertEquals(true , message.getRepeatedField(f("repeated_bool"    ), 0));
      Assert.assertEquals("215", message.getRepeatedField(f("repeated_string"  ), 0));
      Assert.assertEquals(toBytes("216"), message.getRepeatedField(f("repeated_bytes"), 0));

      Assert.assertEquals(217,
        ((Message)message.getRepeatedField(f("repeatedgroup"), 0))
                         .getField(repeatedGroupA));
      Assert.assertEquals(218,
        ((Message)message.getRepeatedField(f("repeated_nested_message"), 0))
                         .getField(nestedB));
      Assert.assertEquals(219,
        ((Message)message.getRepeatedField(f("repeated_foreign_message"), 0))
                         .getField(foreignC));
      Assert.assertEquals(220,
        ((Message)message.getRepeatedField(f("repeated_import_message"), 0))
                         .getField(importD));

      Assert.assertEquals( nestedBar, message.getRepeatedField(f("repeated_nested_enum" ),0));
      Assert.assertEquals(foreignBar, message.getRepeatedField(f("repeated_foreign_enum"),0));
      Assert.assertEquals( importBar, message.getRepeatedField(f("repeated_import_enum" ),0));

      Assert.assertEquals("224", message.getRepeatedField(f("repeated_string_piece"), 0));
      Assert.assertEquals("225", message.getRepeatedField(f("repeated_cord"), 0));

      Assert.assertEquals(301  , message.getRepeatedField(f("repeated_int32"   ), 1));
      Assert.assertEquals(302L , message.getRepeatedField(f("repeated_int64"   ), 1));
      Assert.assertEquals(303  , message.getRepeatedField(f("repeated_uint32"  ), 1));
      Assert.assertEquals(304L , message.getRepeatedField(f("repeated_uint64"  ), 1));
      Assert.assertEquals(305  , message.getRepeatedField(f("repeated_sint32"  ), 1));
      Assert.assertEquals(306L , message.getRepeatedField(f("repeated_sint64"  ), 1));
      Assert.assertEquals(307  , message.getRepeatedField(f("repeated_fixed32" ), 1));
      Assert.assertEquals(308L , message.getRepeatedField(f("repeated_fixed64" ), 1));
      Assert.assertEquals(309  , message.getRepeatedField(f("repeated_sfixed32"), 1));
      Assert.assertEquals(310L , message.getRepeatedField(f("repeated_sfixed64"), 1));
      Assert.assertEquals(311F , message.getRepeatedField(f("repeated_float"   ), 1));
      Assert.assertEquals(312D , message.getRepeatedField(f("repeated_double"  ), 1));
      Assert.assertEquals(false, message.getRepeatedField(f("repeated_bool"    ), 1));
      Assert.assertEquals("315", message.getRepeatedField(f("repeated_string"  ), 1));
      Assert.assertEquals(toBytes("316"), message.getRepeatedField(f("repeated_bytes"), 1));

      Assert.assertEquals(317,
        ((Message)message.getRepeatedField(f("repeatedgroup"), 1))
                         .getField(repeatedGroupA));
      Assert.assertEquals(318,
        ((Message)message.getRepeatedField(f("repeated_nested_message"), 1))
                         .getField(nestedB));
      Assert.assertEquals(319,
        ((Message)message.getRepeatedField(f("repeated_foreign_message"), 1))
                         .getField(foreignC));
      Assert.assertEquals(320,
        ((Message)message.getRepeatedField(f("repeated_import_message"), 1))
                         .getField(importD));

      Assert.assertEquals( nestedBaz, message.getRepeatedField(f("repeated_nested_enum" ),1));
      Assert.assertEquals(foreignBaz, message.getRepeatedField(f("repeated_foreign_enum"),1));
      Assert.assertEquals( importBaz, message.getRepeatedField(f("repeated_import_enum" ),1));

      Assert.assertEquals("324", message.getRepeatedField(f("repeated_string_piece"), 1));
      Assert.assertEquals("325", message.getRepeatedField(f("repeated_cord"), 1));

      // -----------------------------------------------------------------

      Assert.assertTrue(message.hasField(f("default_int32"   )));
      Assert.assertTrue(message.hasField(f("default_int64"   )));
      Assert.assertTrue(message.hasField(f("default_uint32"  )));
      Assert.assertTrue(message.hasField(f("default_uint64"  )));
      Assert.assertTrue(message.hasField(f("default_sint32"  )));
      Assert.assertTrue(message.hasField(f("default_sint64"  )));
      Assert.assertTrue(message.hasField(f("default_fixed32" )));
      Assert.assertTrue(message.hasField(f("default_fixed64" )));
      Assert.assertTrue(message.hasField(f("default_sfixed32")));
      Assert.assertTrue(message.hasField(f("default_sfixed64")));
      Assert.assertTrue(message.hasField(f("default_float"   )));
      Assert.assertTrue(message.hasField(f("default_double"  )));
      Assert.assertTrue(message.hasField(f("default_bool"    )));
      Assert.assertTrue(message.hasField(f("default_string"  )));
      Assert.assertTrue(message.hasField(f("default_bytes"   )));

      Assert.assertTrue(message.hasField(f("default_nested_enum" )));
      Assert.assertTrue(message.hasField(f("default_foreign_enum")));
      Assert.assertTrue(message.hasField(f("default_import_enum" )));

      Assert.assertTrue(message.hasField(f("default_string_piece")));
      Assert.assertTrue(message.hasField(f("default_cord")));

      Assert.assertEquals(401  , message.getField(f("default_int32"   )));
      Assert.assertEquals(402L , message.getField(f("default_int64"   )));
      Assert.assertEquals(403  , message.getField(f("default_uint32"  )));
      Assert.assertEquals(404L , message.getField(f("default_uint64"  )));
      Assert.assertEquals(405  , message.getField(f("default_sint32"  )));
      Assert.assertEquals(406L , message.getField(f("default_sint64"  )));
      Assert.assertEquals(407  , message.getField(f("default_fixed32" )));
      Assert.assertEquals(408L , message.getField(f("default_fixed64" )));
      Assert.assertEquals(409  , message.getField(f("default_sfixed32")));
      Assert.assertEquals(410L , message.getField(f("default_sfixed64")));
      Assert.assertEquals(411F , message.getField(f("default_float"   )));
      Assert.assertEquals(412D , message.getField(f("default_double"  )));
      Assert.assertEquals(false, message.getField(f("default_bool"    )));
      Assert.assertEquals("415", message.getField(f("default_string"  )));
      Assert.assertEquals(toBytes("416"), message.getField(f("default_bytes")));

      Assert.assertEquals( nestedFoo, message.getField(f("default_nested_enum" )));
      Assert.assertEquals(foreignFoo, message.getField(f("default_foreign_enum")));
      Assert.assertEquals( importFoo, message.getField(f("default_import_enum" )));

      Assert.assertEquals("424", message.getField(f("default_string_piece")));
      Assert.assertEquals("425", message.getField(f("default_cord")));
    }

    // -------------------------------------------------------------------

    /**
     * Assert (using {@code junit.framework.Assert}} that all fields of
     * {@code message} are cleared, and that getting the fields returns their
     * default values, using the {@link Message} reflection interface.
     */
    public void assertClearViaReflection(Message message) {
      // has_blah() should initially be false for all optional fields.
      Assert.assertFalse(message.hasField(f("optional_int32"   )));
      Assert.assertFalse(message.hasField(f("optional_int64"   )));
      Assert.assertFalse(message.hasField(f("optional_uint32"  )));
      Assert.assertFalse(message.hasField(f("optional_uint64"  )));
      Assert.assertFalse(message.hasField(f("optional_sint32"  )));
      Assert.assertFalse(message.hasField(f("optional_sint64"  )));
      Assert.assertFalse(message.hasField(f("optional_fixed32" )));
      Assert.assertFalse(message.hasField(f("optional_fixed64" )));
      Assert.assertFalse(message.hasField(f("optional_sfixed32")));
      Assert.assertFalse(message.hasField(f("optional_sfixed64")));
      Assert.assertFalse(message.hasField(f("optional_float"   )));
      Assert.assertFalse(message.hasField(f("optional_double"  )));
      Assert.assertFalse(message.hasField(f("optional_bool"    )));
      Assert.assertFalse(message.hasField(f("optional_string"  )));
      Assert.assertFalse(message.hasField(f("optional_bytes"   )));

      Assert.assertFalse(message.hasField(f("optionalgroup"           )));
      Assert.assertFalse(message.hasField(f("optional_nested_message" )));
      Assert.assertFalse(message.hasField(f("optional_foreign_message")));
      Assert.assertFalse(message.hasField(f("optional_import_message" )));

      Assert.assertFalse(message.hasField(f("optional_nested_enum" )));
      Assert.assertFalse(message.hasField(f("optional_foreign_enum")));
      Assert.assertFalse(message.hasField(f("optional_import_enum" )));

      Assert.assertFalse(message.hasField(f("optional_string_piece")));
      Assert.assertFalse(message.hasField(f("optional_cord")));

      // Optional fields without defaults are set to zero or something like it.
      Assert.assertEquals(0    , message.getField(f("optional_int32"   )));
      Assert.assertEquals(0L   , message.getField(f("optional_int64"   )));
      Assert.assertEquals(0    , message.getField(f("optional_uint32"  )));
      Assert.assertEquals(0L   , message.getField(f("optional_uint64"  )));
      Assert.assertEquals(0    , message.getField(f("optional_sint32"  )));
      Assert.assertEquals(0L   , message.getField(f("optional_sint64"  )));
      Assert.assertEquals(0    , message.getField(f("optional_fixed32" )));
      Assert.assertEquals(0L   , message.getField(f("optional_fixed64" )));
      Assert.assertEquals(0    , message.getField(f("optional_sfixed32")));
      Assert.assertEquals(0L   , message.getField(f("optional_sfixed64")));
      Assert.assertEquals(0F   , message.getField(f("optional_float"   )));
      Assert.assertEquals(0D   , message.getField(f("optional_double"  )));
      Assert.assertEquals(false, message.getField(f("optional_bool"    )));
      Assert.assertEquals(""   , message.getField(f("optional_string"  )));
      Assert.assertEquals(ByteString.EMPTY, message.getField(f("optional_bytes")));

      // Embedded messages should also be clear.
      Assert.assertFalse(
        ((Message)message.getField(f("optionalgroup"))).hasField(groupA));
      Assert.assertFalse(
        ((Message)message.getField(f("optional_nested_message")))
                         .hasField(nestedB));
      Assert.assertFalse(
        ((Message)message.getField(f("optional_foreign_message")))
                         .hasField(foreignC));
      Assert.assertFalse(
        ((Message)message.getField(f("optional_import_message")))
                         .hasField(importD));

      Assert.assertEquals(0,
        ((Message)message.getField(f("optionalgroup"))).getField(groupA));
      Assert.assertEquals(0,
        ((Message)message.getField(f("optional_nested_message")))
                         .getField(nestedB));
      Assert.assertEquals(0,
        ((Message)message.getField(f("optional_foreign_message")))
                         .getField(foreignC));
      Assert.assertEquals(0,
        ((Message)message.getField(f("optional_import_message")))
                         .getField(importD));

      // Enums without defaults are set to the first value in the enum.
      Assert.assertEquals( nestedFoo, message.getField(f("optional_nested_enum" )));
      Assert.assertEquals(foreignFoo, message.getField(f("optional_foreign_enum")));
      Assert.assertEquals( importFoo, message.getField(f("optional_import_enum" )));

      Assert.assertEquals("", message.getField(f("optional_string_piece")));
      Assert.assertEquals("", message.getField(f("optional_cord")));

      // Repeated fields are empty.
      Assert.assertEquals(0, message.getRepeatedFieldCount(f("repeated_int32"   )));
      Assert.assertEquals(0, message.getRepeatedFieldCount(f("repeated_int64"   )));
      Assert.assertEquals(0, message.getRepeatedFieldCount(f("repeated_uint32"  )));
      Assert.assertEquals(0, message.getRepeatedFieldCount(f("repeated_uint64"  )));
      Assert.assertEquals(0, message.getRepeatedFieldCount(f("repeated_sint32"  )));
      Assert.assertEquals(0, message.getRepeatedFieldCount(f("repeated_sint64"  )));
      Assert.assertEquals(0, message.getRepeatedFieldCount(f("repeated_fixed32" )));
      Assert.assertEquals(0, message.getRepeatedFieldCount(f("repeated_fixed64" )));
      Assert.assertEquals(0, message.getRepeatedFieldCount(f("repeated_sfixed32")));
      Assert.assertEquals(0, message.getRepeatedFieldCount(f("repeated_sfixed64")));
      Assert.assertEquals(0, message.getRepeatedFieldCount(f("repeated_float"   )));
      Assert.assertEquals(0, message.getRepeatedFieldCount(f("repeated_double"  )));
      Assert.assertEquals(0, message.getRepeatedFieldCount(f("repeated_bool"    )));
      Assert.assertEquals(0, message.getRepeatedFieldCount(f("repeated_string"  )));
      Assert.assertEquals(0, message.getRepeatedFieldCount(f("repeated_bytes"   )));

      Assert.assertEquals(0, message.getRepeatedFieldCount(f("repeatedgroup"           )));
      Assert.assertEquals(0, message.getRepeatedFieldCount(f("repeated_nested_message" )));
      Assert.assertEquals(0, message.getRepeatedFieldCount(f("repeated_foreign_message")));
      Assert.assertEquals(0, message.getRepeatedFieldCount(f("repeated_import_message" )));
      Assert.assertEquals(0, message.getRepeatedFieldCount(f("repeated_nested_enum"    )));
      Assert.assertEquals(0, message.getRepeatedFieldCount(f("repeated_foreign_enum"   )));
      Assert.assertEquals(0, message.getRepeatedFieldCount(f("repeated_import_enum"    )));

      Assert.assertEquals(0, message.getRepeatedFieldCount(f("repeated_string_piece")));
      Assert.assertEquals(0, message.getRepeatedFieldCount(f("repeated_cord")));

      // has_blah() should also be false for all default fields.
      Assert.assertFalse(message.hasField(f("default_int32"   )));
      Assert.assertFalse(message.hasField(f("default_int64"   )));
      Assert.assertFalse(message.hasField(f("default_uint32"  )));
      Assert.assertFalse(message.hasField(f("default_uint64"  )));
      Assert.assertFalse(message.hasField(f("default_sint32"  )));
      Assert.assertFalse(message.hasField(f("default_sint64"  )));
      Assert.assertFalse(message.hasField(f("default_fixed32" )));
      Assert.assertFalse(message.hasField(f("default_fixed64" )));
      Assert.assertFalse(message.hasField(f("default_sfixed32")));
      Assert.assertFalse(message.hasField(f("default_sfixed64")));
      Assert.assertFalse(message.hasField(f("default_float"   )));
      Assert.assertFalse(message.hasField(f("default_double"  )));
      Assert.assertFalse(message.hasField(f("default_bool"    )));
      Assert.assertFalse(message.hasField(f("default_string"  )));
      Assert.assertFalse(message.hasField(f("default_bytes"   )));

      Assert.assertFalse(message.hasField(f("default_nested_enum" )));
      Assert.assertFalse(message.hasField(f("default_foreign_enum")));
      Assert.assertFalse(message.hasField(f("default_import_enum" )));

      Assert.assertFalse(message.hasField(f("default_string_piece" )));
      Assert.assertFalse(message.hasField(f("default_cord" )));

      // Fields with defaults have their default values (duh).
      Assert.assertEquals( 41    , message.getField(f("default_int32"   )));
      Assert.assertEquals( 42L   , message.getField(f("default_int64"   )));
      Assert.assertEquals( 43    , message.getField(f("default_uint32"  )));
      Assert.assertEquals( 44L   , message.getField(f("default_uint64"  )));
      Assert.assertEquals(-45    , message.getField(f("default_sint32"  )));
      Assert.assertEquals( 46L   , message.getField(f("default_sint64"  )));
      Assert.assertEquals( 47    , message.getField(f("default_fixed32" )));
      Assert.assertEquals( 48L   , message.getField(f("default_fixed64" )));
      Assert.assertEquals( 49    , message.getField(f("default_sfixed32")));
      Assert.assertEquals(-50L   , message.getField(f("default_sfixed64")));
      Assert.assertEquals( 51.5F , message.getField(f("default_float"   )));
      Assert.assertEquals( 52e3D , message.getField(f("default_double"  )));
      Assert.assertEquals(true   , message.getField(f("default_bool"    )));
      Assert.assertEquals("hello", message.getField(f("default_string"  )));
      Assert.assertEquals(toBytes("world"), message.getField(f("default_bytes")));

      Assert.assertEquals( nestedBar, message.getField(f("default_nested_enum" )));
      Assert.assertEquals(foreignBar, message.getField(f("default_foreign_enum")));
      Assert.assertEquals( importBar, message.getField(f("default_import_enum" )));

      Assert.assertEquals("abc", message.getField(f("default_string_piece")));
      Assert.assertEquals("123", message.getField(f("default_cord")));
    }

    // ---------------------------------------------------------------

    public void assertRepeatedFieldsModifiedViaReflection(Message message) {
      // ModifyRepeatedFields only sets the second repeated element of each
      // field.  In addition to verifying this, we also verify that the first
      // element and size were *not* modified.
      Assert.assertEquals(2, message.getRepeatedFieldCount(f("repeated_int32"   )));
      Assert.assertEquals(2, message.getRepeatedFieldCount(f("repeated_int64"   )));
      Assert.assertEquals(2, message.getRepeatedFieldCount(f("repeated_uint32"  )));
      Assert.assertEquals(2, message.getRepeatedFieldCount(f("repeated_uint64"  )));
      Assert.assertEquals(2, message.getRepeatedFieldCount(f("repeated_sint32"  )));
      Assert.assertEquals(2, message.getRepeatedFieldCount(f("repeated_sint64"  )));
      Assert.assertEquals(2, message.getRepeatedFieldCount(f("repeated_fixed32" )));
      Assert.assertEquals(2, message.getRepeatedFieldCount(f("repeated_fixed64" )));
      Assert.assertEquals(2, message.getRepeatedFieldCount(f("repeated_sfixed32")));
      Assert.assertEquals(2, message.getRepeatedFieldCount(f("repeated_sfixed64")));
      Assert.assertEquals(2, message.getRepeatedFieldCount(f("repeated_float"   )));
      Assert.assertEquals(2, message.getRepeatedFieldCount(f("repeated_double"  )));
      Assert.assertEquals(2, message.getRepeatedFieldCount(f("repeated_bool"    )));
      Assert.assertEquals(2, message.getRepeatedFieldCount(f("repeated_string"  )));
      Assert.assertEquals(2, message.getRepeatedFieldCount(f("repeated_bytes"   )));

      Assert.assertEquals(2, message.getRepeatedFieldCount(f("repeatedgroup"           )));
      Assert.assertEquals(2, message.getRepeatedFieldCount(f("repeated_nested_message" )));
      Assert.assertEquals(2, message.getRepeatedFieldCount(f("repeated_foreign_message")));
      Assert.assertEquals(2, message.getRepeatedFieldCount(f("repeated_import_message" )));
      Assert.assertEquals(2, message.getRepeatedFieldCount(f("repeated_nested_enum"    )));
      Assert.assertEquals(2, message.getRepeatedFieldCount(f("repeated_foreign_enum"   )));
      Assert.assertEquals(2, message.getRepeatedFieldCount(f("repeated_import_enum"    )));

      Assert.assertEquals(2, message.getRepeatedFieldCount(f("repeated_string_piece")));
      Assert.assertEquals(2, message.getRepeatedFieldCount(f("repeated_cord")));

      Assert.assertEquals(201  , message.getRepeatedField(f("repeated_int32"   ), 0));
      Assert.assertEquals(202L , message.getRepeatedField(f("repeated_int64"   ), 0));
      Assert.assertEquals(203  , message.getRepeatedField(f("repeated_uint32"  ), 0));
      Assert.assertEquals(204L , message.getRepeatedField(f("repeated_uint64"  ), 0));
      Assert.assertEquals(205  , message.getRepeatedField(f("repeated_sint32"  ), 0));
      Assert.assertEquals(206L , message.getRepeatedField(f("repeated_sint64"  ), 0));
      Assert.assertEquals(207  , message.getRepeatedField(f("repeated_fixed32" ), 0));
      Assert.assertEquals(208L , message.getRepeatedField(f("repeated_fixed64" ), 0));
      Assert.assertEquals(209  , message.getRepeatedField(f("repeated_sfixed32"), 0));
      Assert.assertEquals(210L , message.getRepeatedField(f("repeated_sfixed64"), 0));
      Assert.assertEquals(211F , message.getRepeatedField(f("repeated_float"   ), 0));
      Assert.assertEquals(212D , message.getRepeatedField(f("repeated_double"  ), 0));
      Assert.assertEquals(true , message.getRepeatedField(f("repeated_bool"    ), 0));
      Assert.assertEquals("215", message.getRepeatedField(f("repeated_string"  ), 0));
      Assert.assertEquals(toBytes("216"), message.getRepeatedField(f("repeated_bytes"), 0));

      Assert.assertEquals(217,
        ((Message)message.getRepeatedField(f("repeatedgroup"), 0))
                         .getField(repeatedGroupA));
      Assert.assertEquals(218,
        ((Message)message.getRepeatedField(f("repeated_nested_message"), 0))
                         .getField(nestedB));
      Assert.assertEquals(219,
        ((Message)message.getRepeatedField(f("repeated_foreign_message"), 0))
                         .getField(foreignC));
      Assert.assertEquals(220,
        ((Message)message.getRepeatedField(f("repeated_import_message"), 0))
                         .getField(importD));

      Assert.assertEquals( nestedBar, message.getRepeatedField(f("repeated_nested_enum" ),0));
      Assert.assertEquals(foreignBar, message.getRepeatedField(f("repeated_foreign_enum"),0));
      Assert.assertEquals( importBar, message.getRepeatedField(f("repeated_import_enum" ),0));

      Assert.assertEquals("224", message.getRepeatedField(f("repeated_string_piece"), 0));
      Assert.assertEquals("225", message.getRepeatedField(f("repeated_cord"), 0));

      Assert.assertEquals(501  , message.getRepeatedField(f("repeated_int32"   ), 1));
      Assert.assertEquals(502L , message.getRepeatedField(f("repeated_int64"   ), 1));
      Assert.assertEquals(503  , message.getRepeatedField(f("repeated_uint32"  ), 1));
      Assert.assertEquals(504L , message.getRepeatedField(f("repeated_uint64"  ), 1));
      Assert.assertEquals(505  , message.getRepeatedField(f("repeated_sint32"  ), 1));
      Assert.assertEquals(506L , message.getRepeatedField(f("repeated_sint64"  ), 1));
      Assert.assertEquals(507  , message.getRepeatedField(f("repeated_fixed32" ), 1));
      Assert.assertEquals(508L , message.getRepeatedField(f("repeated_fixed64" ), 1));
      Assert.assertEquals(509  , message.getRepeatedField(f("repeated_sfixed32"), 1));
      Assert.assertEquals(510L , message.getRepeatedField(f("repeated_sfixed64"), 1));
      Assert.assertEquals(511F , message.getRepeatedField(f("repeated_float"   ), 1));
      Assert.assertEquals(512D , message.getRepeatedField(f("repeated_double"  ), 1));
      Assert.assertEquals(true , message.getRepeatedField(f("repeated_bool"    ), 1));
      Assert.assertEquals("515", message.getRepeatedField(f("repeated_string"  ), 1));
      Assert.assertEquals(toBytes("516"), message.getRepeatedField(f("repeated_bytes"), 1));

      Assert.assertEquals(517,
        ((Message)message.getRepeatedField(f("repeatedgroup"), 1))
                         .getField(repeatedGroupA));
      Assert.assertEquals(518,
        ((Message)message.getRepeatedField(f("repeated_nested_message"), 1))
                         .getField(nestedB));
      Assert.assertEquals(519,
        ((Message)message.getRepeatedField(f("repeated_foreign_message"), 1))
                         .getField(foreignC));
      Assert.assertEquals(520,
        ((Message)message.getRepeatedField(f("repeated_import_message"), 1))
                         .getField(importD));

      Assert.assertEquals( nestedFoo, message.getRepeatedField(f("repeated_nested_enum" ),1));
      Assert.assertEquals(foreignFoo, message.getRepeatedField(f("repeated_foreign_enum"),1));
      Assert.assertEquals( importFoo, message.getRepeatedField(f("repeated_import_enum" ),1));

      Assert.assertEquals("524", message.getRepeatedField(f("repeated_string_piece"), 1));
      Assert.assertEquals("525", message.getRepeatedField(f("repeated_cord"), 1));
    }
  }

  /**
   * @param filePath The path relative to
   * {@link com.google.testing.util.TestUtil#getDefaultSrcDir}.
   */
  public static String readTextFromFile(String filePath) {
    return readBytesFromFile(filePath).toStringUtf8();
  }

  private static File getTestDataDir() {
    // Search each parent directory looking for "src/google/protobuf".
    File ancestor = new File(".");
    try {
      ancestor = ancestor.getCanonicalFile();
    } catch (IOException e) {
      throw new RuntimeException(
        "Couldn't get canonical name of working directory.", e);
    }
    while (ancestor != null && ancestor.exists()) {
      if (new File(ancestor, "src/google/protobuf").exists()) {
        return new File(ancestor, "src/google/protobuf/testdata");
      }
      ancestor = ancestor.getParentFile();
    }

    throw new RuntimeException(
      "Could not find golden files.  This test must be run from within the " +
      "protobuf source package so that it can read test data files from the " +
      "C++ source tree.");
  }

  /**
   * @param filePath The path relative to
   * {@link com.google.testing.util.TestUtil#getDefaultSrcDir}.
   */
  public static ByteString readBytesFromFile(String filename) {
    File fullPath = new File(getTestDataDir(), filename);
    try {
      RandomAccessFile file = new RandomAccessFile(fullPath, "r");
      byte[] content = new byte[(int) file.length()];
      file.readFully(content);
      return ByteString.copyFrom(content);
    } catch (IOException e) {
      // Throw a RuntimeException here so that we can call this function from
      // static initializers.
      throw new IllegalArgumentException(
        "Couldn't read file: " + fullPath.getPath(), e);
    }
  }

  /**
   * Get the bytes of the "golden message".  This is a serialized TestAllTypes
   * with all fields set as they would be by
   * {@link setAllFields(TestAllTypes.Builder)}, but it is loaded from a file
   * on disk rather than generated dynamically.  The file is actually generated
   * by C++ code, so testing against it verifies compatibility with C++.
   */
  public static ByteString getGoldenMessage() {
    if (goldenMessage == null) {
      goldenMessage = readBytesFromFile("golden_message");
    }
    return goldenMessage;
  }
  private static ByteString goldenMessage = null;
}
