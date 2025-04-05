// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

package com.google.protobuf;

import static com.google.common.truth.Truth.assertThat;
import static org.junit.Assert.assertThrows;

import com.google.protobuf.large.openenum.edition.LargeOpenEnum;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

@RunWith(JUnit4.class)
public class LargeEnumLiteTest {

  @Test
  public void testOpenLargeEnum() throws Exception {
    assertThrows(IllegalArgumentException.class, LargeOpenEnum.UNRECOGNIZED::getNumber);
    assertThat(LargeOpenEnum.values())
        .isEqualTo(
            new LargeOpenEnum[] {
              LargeOpenEnum.LARGE_ENUM_UNSPECIFIED,
              LargeOpenEnum.LARGE_ENUM1,
              LargeOpenEnum.LARGE_ENUM2,
              LargeOpenEnum.UNRECOGNIZED
            });
  }
}
