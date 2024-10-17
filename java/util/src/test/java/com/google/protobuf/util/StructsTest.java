// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

package com.google.protobuf.util;

import static com.google.common.truth.Truth.assertThat;

import com.google.protobuf.Struct;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

@RunWith(JUnit4.class)
public final class StructsTest {

  @Test
  public void test1pair_constructsObject() throws Exception {
    Struct.Builder expected = Struct.newBuilder();
    JsonFormat.parser().merge("{\"k1\": 1}", expected);
    assertThat(Structs.of("k1", Values.of(1))).isEqualTo(expected.build());
  }

  @Test
  public void test2pair_constructsObject() throws Exception {
    Struct.Builder expected = Struct.newBuilder();
    JsonFormat.parser().merge("{\"k1\": 1, \"k2\": 2}", expected);
    assertThat(Structs.of("k1", Values.of(1), "k2", Values.of(2))).isEqualTo(expected.build());
  }

  @Test
  public void test3pair_constructsObject() throws Exception {
    Struct.Builder expected = Struct.newBuilder();
    JsonFormat.parser().merge("{\"k1\": 1, \"k2\": 2, \"k3\": 3}", expected);
    assertThat(Structs.of("k1", Values.of(1), "k2", Values.of(2), "k3", Values.of(3)))
        .isEqualTo(expected.build());
  }
}
