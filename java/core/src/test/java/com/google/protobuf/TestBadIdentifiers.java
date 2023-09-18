// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

package com.google.protobuf;

import junit.framework.TestCase;

/**
 * Tests that proto2 api generation doesn't cause compile errors when compiling protocol buffers
 * that have names that would otherwise conflict if not fully qualified (like @Deprecated
 * and @Override).
 *
 * @author jonp@google.com (Jon Perlow)
 */
public class TestBadIdentifiers extends TestCase {

  public void testCompilation() {
    // If this compiles, it means the generation was correct.
    TestBadIdentifiersProto.Deprecated unused1 =
        TestBadIdentifiersProto.Deprecated.newBuilder().build();
    TestBadIdentifiersProto.Override unused2 =
        TestBadIdentifiersProto.Override.getDefaultInstance();
  }

  @SuppressWarnings({"IgnoredPureGetter", "CheckReturnValue"}) // TODO: Fix this
  public void testGetDescriptor() {
    TestBadIdentifiersProto.getDescriptor();
    TestBadIdentifiersProto.Descriptor.getDefaultInstance().getDescriptor();
    TestBadIdentifiersProto.Descriptor.getDefaultInstance().getDescriptorForType();
    TestBadIdentifiersProto.Descriptor.NestedDescriptor.getDefaultInstance().getDescriptor();
    TestBadIdentifiersProto.Descriptor.NestedDescriptor.getDefaultInstance().getDescriptorForType();
  }

  public void testConflictingFieldNames() throws Exception {
    TestBadIdentifiersProto.TestConflictingFieldNames message =
        TestBadIdentifiersProto.TestConflictingFieldNames.getDefaultInstance();
    // Make sure generated accessors are properly named.
    assertEquals(0, message.getInt32Field1Count());
    assertEquals(0, message.getEnumField2Count());
    assertEquals(0, message.getStringField3Count());
    assertEquals(0, message.getBytesField4Count());
    assertEquals(0, message.getMessageField5Count());

    assertEquals(0, message.getInt32FieldCount11());
    assertEquals(0, message.getEnumFieldCount12().getNumber());
    assertEquals("", message.getStringFieldCount13());
    assertEquals(ByteString.EMPTY, message.getBytesFieldCount14());
    assertEquals(0, message.getMessageFieldCount15().getSerializedSize());

    assertEquals(0, message.getInt32Field21Count());
    assertEquals(0, message.getEnumField22Count());
    assertEquals(0, message.getStringField23Count());
    assertEquals(0, message.getBytesField24Count());
    assertEquals(0, message.getMessageField25Count());

    assertEquals(0, message.getInt32Field1List().size());
    assertEquals(0, message.getInt32FieldList31());

    assertEquals(0, message.getInt64FieldCount());
    assertEquals(
        0L,
        message
            .getExtension(TestBadIdentifiersProto.TestConflictingFieldNames.int64FieldCount)
            .longValue());
    assertEquals(
        0L,
        message
            .getExtension(TestBadIdentifiersProto.TestConflictingFieldNames.int64FieldList)
            .longValue());

    assertEquals("", message.getFieldName32());
    assertEquals("", message.getFieldName33());
    assertEquals(0, message.get2Conflict34());
    assertEquals(0, message.get2Conflict35());
  }

  public void testNumberFields() throws Exception {
    TestBadIdentifiersProto.TestLeadingNumberFields message =
        TestBadIdentifiersProto.TestLeadingNumberFields.getDefaultInstance();
    // Make sure generated accessors are properly named.
    assertFalse(message.has30DayImpressions());
    assertEquals(0, message.get30DayImpressions());
    assertEquals(0, message.get60DayImpressionsCount());
    assertEquals(0, message.get60DayImpressionsList().size());

    assertFalse(message.has2Underscores());
    assertEquals("", message.get2Underscores());
    assertEquals(0, message.get2RepeatedUnderscoresCount());
    assertEquals(0, message.get2RepeatedUnderscoresList().size());

    assertFalse(message.has32());
    assertEquals(0, message.get32());
    assertEquals(0, message.get64Count());
    assertEquals(0, message.get64List().size());
  }
}
