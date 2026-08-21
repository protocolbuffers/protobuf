// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

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
