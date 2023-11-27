// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

package com.google.protobuf;

import static com.google.common.truth.Truth.assertThat;

import protobuf_unittest.UnittestProto;
import java.io.IOException;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

/**
 * Tests to make sure the lazy conversion of UTF8-encoded byte arrays to strings works
 * correctly.
 */
@RunWith(JUnit4.class)
public class LazyStringEndToEndTest {

  private static final ByteString TEST_ALL_TYPES_SERIALIZED_WITH_ILLEGAL_UTF8 =
      ByteString.copyFrom(
          new byte[] {
            114, 4, -1, 0, -1, 0, -30, 2, 4, -1,
            0, -1, 0, -30, 2, 4, -1, 0, -1, 0,
          });

  private ByteString encodedTestAllTypes;

  @Before
  public void setUp() throws Exception {
    this.encodedTestAllTypes =
        UnittestProto.TestAllTypes.newBuilder()
            .setOptionalString("foo")
            .addRepeatedString("bar")
            .addRepeatedString("baz")
            .build()
            .toByteString();
  }

  /** Tests that an invalid UTF8 string will roundtrip through a parse and serialization. */
  @Test
  public void testParseAndSerialize() throws InvalidProtocolBufferException {
    UnittestProto.TestAllTypes tV2 =
        UnittestProto.TestAllTypes.parseFrom(
            TEST_ALL_TYPES_SERIALIZED_WITH_ILLEGAL_UTF8,
            ExtensionRegistryLite.getEmptyRegistry());
    ByteString bytes = tV2.toByteString();
    assertThat(bytes).isEqualTo(TEST_ALL_TYPES_SERIALIZED_WITH_ILLEGAL_UTF8);

    String unused = tV2.getOptionalString();
    bytes = tV2.toByteString();
    assertThat(bytes).isEqualTo(TEST_ALL_TYPES_SERIALIZED_WITH_ILLEGAL_UTF8);
  }

  @Test
  public void testParseAndWrite() throws IOException {
    UnittestProto.TestAllTypes tV2 =
        UnittestProto.TestAllTypes.parseFrom(
            TEST_ALL_TYPES_SERIALIZED_WITH_ILLEGAL_UTF8,
            ExtensionRegistryLite.getEmptyRegistry());
    byte[] sink = new byte[TEST_ALL_TYPES_SERIALIZED_WITH_ILLEGAL_UTF8.size()];
    CodedOutputStream outputStream = CodedOutputStream.newInstance(sink);
    tV2.writeTo(outputStream);
    outputStream.flush();
    assertThat(ByteString.copyFrom(sink)).isEqualTo(TEST_ALL_TYPES_SERIALIZED_WITH_ILLEGAL_UTF8);
  }

  @Test
  public void testCaching() {
    String a = "a";
    String b = "b";
    String c = "c";
    UnittestProto.TestAllTypes proto =
        UnittestProto.TestAllTypes.newBuilder()
            .setOptionalString(a)
            .addRepeatedString(b)
            .addRepeatedString(c)
            .build();

    // String should be the one we passed it.
    assertThat(proto.getOptionalString()).isSameInstanceAs(a);
    assertThat(proto.getRepeatedString(0)).isSameInstanceAs(b);
    assertThat(proto.getRepeatedString(1)).isSameInstanceAs(c);

    // Ensure serialization keeps strings cached.
    ByteString unused = proto.toByteString();

    // And now the string should stay cached.
    assertThat(proto.getOptionalString()).isSameInstanceAs(a);
    assertThat(proto.getRepeatedString(0)).isSameInstanceAs(b);
    assertThat(proto.getRepeatedString(1)).isSameInstanceAs(c);
  }

  @Test
  public void testNoStringCachingIfOnlyBytesAccessed() throws Exception {
    UnittestProto.TestAllTypes proto =
        UnittestProto.TestAllTypes.parseFrom(
            encodedTestAllTypes, ExtensionRegistryLite.getEmptyRegistry());
    ByteString optional = proto.getOptionalStringBytes();
    assertThat(proto.getOptionalStringBytes()).isSameInstanceAs(optional);
    assertThat(proto.toBuilder().getOptionalStringBytes()).isSameInstanceAs(optional);

    ByteString repeated0 = proto.getRepeatedStringBytes(0);
    ByteString repeated1 = proto.getRepeatedStringBytes(1);
    assertThat(proto.getRepeatedStringBytes(0)).isSameInstanceAs(repeated0);
    assertThat(proto.getRepeatedStringBytes(1)).isSameInstanceAs(repeated1);
    assertThat(proto.toBuilder().getRepeatedStringBytes(0)).isSameInstanceAs(repeated0);
    assertThat(proto.toBuilder().getRepeatedStringBytes(1)).isSameInstanceAs(repeated1);
  }
}
