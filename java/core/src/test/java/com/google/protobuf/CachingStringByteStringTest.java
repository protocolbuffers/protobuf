// Protocol Buffers - Google's data interchange format
// Copyright 2025 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

package com.google.protobuf;

import static com.google.common.truth.Truth.assertThat;

import java.nio.charset.StandardCharsets;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

@RunWith(JUnit4.class)
public final class CachingStringByteStringTest {

  private static final String PARENT_STRING = "parent";
  private final ByteString parentByteString = ByteString.copyFromUtf8(PARENT_STRING);
  private final ByteString.CachingStringByteString testByteString =
      ByteString.withStringCached(parentByteString);

  @Test
  public void cachingString_returnsCachedValue() {
    assertThat(testByteString.cache).isNull();
    assertThat(testByteString.toStringUtf8()).isEqualTo(PARENT_STRING);
    assertThat(testByteString.cache).isEqualTo(PARENT_STRING);
    assertThat(testByteString.cache).isSameInstanceAs(testByteString.toStringUtf8());
  }

  @Test
  public void cachingStringNonUtf8_returnsFreshValue() {
    assertThat(testByteString.cache).isNull();
    assertThat(testByteString.toString(StandardCharsets.ISO_8859_1)).isEqualTo(PARENT_STRING);
    assertThat(testByteString.cache).isNull();
  }

  @Test
  public void cachingUtf8Validity_returnsCachedValue() {
    assertThat(testByteString.validUtf8).isNull();
    assertThat(testByteString.isValidUtf8()).isTrue();
    assertThat(testByteString.validUtf8).isTrue();
  }

  @Test
  public void cachingStringWithRopeByteString_returnsCachedValue() {
    ByteString rope = ByteString.copyFromUtf8("hello").concat(ByteString.copyFromUtf8(" world"));
    ByteString.CachingStringByteString bs = ByteString.withStringCached(rope);
    assertThat(bs.toStringUtf8()).isEqualTo("hello world");
    assertThat(bs.cache).isSameInstanceAs(bs.toStringUtf8());
    assertThat(bs.isValidUtf8()).isTrue();
    assertThat(bs.equals(rope)).isTrue();
  }

  @Test
  public void cachingStringEquals_returnsTrue() {
    ByteString.CachingStringByteString a =
        ByteString.withStringCached(ByteString.copyFromUtf8("hello"));
    ByteString.CachingStringByteString b =
        ByteString.withStringCached(ByteString.copyFromUtf8("hello"));
    assertThat(a).isEqualTo(b);
    assertThat(a).isNotSameInstanceAs(b);
  }

  @Test
  public void nestedCachingStringEquals_isSameInstance() {
    ByteString.CachingStringByteString a =
        ByteString.withStringCached(ByteString.copyFromUtf8("hello"));
    ByteString.CachingStringByteString a2 = ByteString.withStringCached(a);
    assertThat(a2).isSameInstanceAs(a);
  }
}
