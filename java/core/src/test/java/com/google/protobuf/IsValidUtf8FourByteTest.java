// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

package com.google.protobuf;

import static com.google.protobuf.IsValidUtf8TestUtil.DIRECT_NIO_FACTORY;
import static com.google.protobuf.IsValidUtf8TestUtil.HEAP_NIO_FACTORY;
import static com.google.protobuf.IsValidUtf8TestUtil.LITERAL_FACTORY;
import static com.google.protobuf.IsValidUtf8TestUtil.testBytes;

import com.google.protobuf.IsValidUtf8TestUtil.Shard;
import java.io.UnsupportedEncodingException;
import java.util.Collection;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.Parameterized;
import org.junit.runners.Parameterized.Parameters;

/**
 * Tests cases for {@link ByteString#isValidUtf8()} with four byte encodings. This tests every
 * permutation of four byte sequences to ensure that the method produces the right answer for every
 * possible byte encoding where "right" means it's consistent with java's UTF-8 string
 * encoding/decoding such that the method returns true for any sequence that will round trip when
 * converted to a String and then back to bytes and will return false for any sequence that will not
 * round trip.
 */
@RunWith(Parameterized.class)
public class IsValidUtf8FourByteTest {

  @Parameters
  public static Collection<Shard> data() {
    return IsValidUtf8TestUtil.FOUR_BYTE_SHARDS;
  }

  private final Shard shard;

  public IsValidUtf8FourByteTest(Shard shard) {
    this.shard = shard;
  }

  @Test
  public void runTest() throws UnsupportedEncodingException {
    testBytes(LITERAL_FACTORY, 4, shard.expected, shard.start, shard.lim);
    testBytes(HEAP_NIO_FACTORY, 4, shard.expected, shard.start, shard.lim);
    testBytes(DIRECT_NIO_FACTORY, 4, shard.expected, shard.start, shard.lim);
  }
}
