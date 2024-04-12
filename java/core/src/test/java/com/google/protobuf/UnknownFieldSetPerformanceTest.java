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

import java.io.ByteArrayInputStream;
import java.io.IOException;
import java.io.InputStream;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

@RunWith(JUnit4.class)
public final class UnknownFieldSetPerformanceTest {

  private static byte[] generateBytes(int length) {
    assertThat(length % 4).isEqualTo(0);
    byte[] input = new byte[length];
    for (int i = 0; i < length; i += 4) {
        input[i] =     (byte) 0x08; // field 1, wiretype 0
        input[i + 1] = (byte) 0x08; // field 1, payload 8
        input[i + 2] = (byte) 0x20; // field 4, wiretype 0
        input[i + 3] = (byte) 0x20; // field 4, payload 32
    }
    return input;
  }

  @Test
  // This is a performance test. Failure here is a timeout.
  public void testAlternatingFieldNumbers() throws IOException {
    byte[] input = generateBytes(800000);
    InputStream in = new ByteArrayInputStream(input);
    UnknownFieldSet.Builder builder = UnknownFieldSet.newBuilder();
    CodedInputStream codedInput = CodedInputStream.newInstance(in);
    builder.mergeFrom(codedInput);
  }

  @Test
  // This is a performance test. Failure here is a timeout.
  public void testAddField() {
    UnknownFieldSet.Builder builder = UnknownFieldSet.newBuilder();
    for (int i = 1; i <= 100000; i++) {
      UnknownFieldSet.Field field = UnknownFieldSet.Field.newBuilder().addFixed32(i).build();
      builder.addField(i, field);
    }
    UnknownFieldSet fieldSet = builder.build();
    assertThat(fieldSet.getField(100000).getFixed32List().get(0)).isEqualTo(100000);
  }
}
