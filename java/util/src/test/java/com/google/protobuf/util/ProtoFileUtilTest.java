// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

package com.google.protobuf.util;

import static com.google.common.truth.Truth.assertThat;

import com.google.protobuf.DescriptorProtos.Edition;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

/** Unit tests for {@link ProtoFileUtil}. */
@RunWith(JUnit4.class)
public class ProtoFileUtilTest {
  @Test
  public void testGetEditionFromString() throws Exception {
    assertThat(ProtoFileUtil.getEditionString(Edition.EDITION_2023)).isEqualTo("2023");
  }
}
