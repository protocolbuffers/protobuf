// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

package com.google.protobuf;

import static com.google.common.truth.Truth.assertThat;

import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

@RunWith(JUnit4.class)
public class InvalidProtocolBufferExceptionTest {

  @Test
  public void testWrapRuntimeException() {
    ArrayIndexOutOfBoundsException root = new ArrayIndexOutOfBoundsException();
    InvalidProtocolBufferException wrapper = new InvalidProtocolBufferException(root);
    assertThat(wrapper).hasCauseThat().isEqualTo(root);
  }

}
