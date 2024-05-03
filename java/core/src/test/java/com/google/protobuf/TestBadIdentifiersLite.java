// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

package com.google.protobuf;

import static com.google.common.truth.Truth.assertThat;

import junit.framework.TestCase;

/**
 * Tests that proto2 api generation doesn't cause compile errors when compiling protocol buffers
 * that have names that would otherwise conflict if not fully qualified (like @Deprecated
 * and @Override).
 *
 * <p>Forked from {@link TestBadIdentifiers}.
 *
 * @author jonp@google.com (Jon Perlow)
 */
public final class TestBadIdentifiersLite extends TestCase {

  public void testCompilation() {
    // If this compiles, it means the generation was correct.
    TestBadIdentifiersProto.Deprecated.Builder builder1 =
        TestBadIdentifiersProto.Deprecated.newBuilder();
    TestBadIdentifiersProto.Override.Builder builder2 =
        TestBadIdentifiersProto.Override.newBuilder();
  }

  public void testConflictingFieldNames() throws Exception {
    TestBadIdentifiersProto.TestConflictingFieldNames message =
        TestBadIdentifiersProto.TestConflictingFieldNames.getDefaultInstance();
    // Make sure generated accessors are properly named.
    assertThat(message.getInt32Field1Count()).isEqualTo(0);
    assertThat(message.getEnumField2Count()).isEqualTo(0);
    assertThat(message.getStringField3Count()).isEqualTo(0);
    assertThat(message.getBytesField4Count()).isEqualTo(0);
    assertThat(message.getMessageField5Count()).isEqualTo(0);

    assertThat(message.getInt32FieldCount11()).isEqualTo(0);
    assertThat(message.getEnumFieldCount12().getNumber()).isEqualTo(0);
    assertThat(message.getStringFieldCount13()).isEmpty();
    assertThat(message.getBytesFieldCount14()).isEqualTo(ByteString.EMPTY);
    assertThat(message.getMessageFieldCount15().getSerializedSize()).isEqualTo(0);

    assertThat(message.getInt32Field21Count()).isEqualTo(0);
    assertThat(message.getEnumField22Count()).isEqualTo(0);
    assertThat(message.getStringField23Count()).isEqualTo(0);
    assertThat(message.getBytesField24Count()).isEqualTo(0);
    assertThat(message.getMessageField25Count()).isEqualTo(0);

    assertThat(message.getInt32Field1List()).isEmpty();
    assertThat(message.getInt32FieldList31()).isEqualTo(0);

    assertThat(message.getInt64FieldCount()).isEqualTo(0);
    assertThat(
            message
                .getExtension(TestBadIdentifiersProto.TestConflictingFieldNames.int64FieldCount)
                .longValue())
        .isEqualTo(0L);
    assertThat(
            message
                .getExtension(TestBadIdentifiersProto.TestConflictingFieldNames.int64FieldList)
                .longValue())
        .isEqualTo(0L);
  }
}
