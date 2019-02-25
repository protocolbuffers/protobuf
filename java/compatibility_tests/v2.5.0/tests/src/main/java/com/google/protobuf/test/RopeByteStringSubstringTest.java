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

package com.google.protobuf.test;
import com.google.protobuf.*;

import java.io.UnsupportedEncodingException;
import java.util.Iterator;

/**
 * This class tests {@link RopeByteString#substring(int, int)} by inheriting the tests from
 * {@link LiteralByteStringTest}.  Only a couple of methods are overridden.  
 *
 * @author carlanton@google.com (Carl Haverl)
 */
public class RopeByteStringSubstringTest extends LiteralByteStringTest {

  @Override
  protected void setUp() throws Exception {
    classUnderTest = "RopeByteString";
    byte[] sourceBytes = ByteStringTest.getTestBytes(22341, 22337766L);
    Iterator<ByteString> iter = ByteStringTest.makeConcretePieces(sourceBytes).iterator();
    ByteString sourceString = iter.next();
    while (iter.hasNext()) {
      sourceString = sourceString.concat(iter.next());
    }

    int from = 1130;
    int to = sourceBytes.length - 5555;
    stringUnderTest = sourceString.substring(from, to);
    referenceBytes = new byte[to - from];
    System.arraycopy(sourceBytes, from, referenceBytes, 0, to - from);
    expectedHashCode = -1259260680;
  }
}
