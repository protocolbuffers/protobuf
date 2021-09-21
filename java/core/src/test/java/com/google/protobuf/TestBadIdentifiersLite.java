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
