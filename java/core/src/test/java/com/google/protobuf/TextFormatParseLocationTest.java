// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

package com.google.protobuf;

import static com.google.common.truth.Truth.assertThat;
import static com.google.common.truth.Truth.assertWithMessage;

import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

/** Test {@link TextFormatParseLocation}. */
@RunWith(JUnit4.class)
public class TextFormatParseLocationTest {

  @Test
  public void testCreateEmpty() {
    TextFormatParseLocation location = TextFormatParseLocation.create(-1, -1);
    assertThat(location).isEqualTo(TextFormatParseLocation.EMPTY);
  }

  @Test
  public void testCreate() {
    TextFormatParseLocation location = TextFormatParseLocation.create(2, 1);
    assertThat(location.getLine()).isEqualTo(2);
    assertThat(location.getColumn()).isEqualTo(1);
  }

  @Test
  public void testCreateThrowsIllegalArgumentExceptionForInvalidIndex() {
    try {
      TextFormatParseLocation.create(-1, 0);
      assertWithMessage("Should throw IllegalArgumentException if line is less than 0").fail();
    } catch (IllegalArgumentException unused) {
      // pass
    }
    try {
      TextFormatParseLocation.create(0, -1);
      assertWithMessage("Should throw, column < 0").fail();
    } catch (IllegalArgumentException unused) {
      // pass
    }
  }

  @Test
  public void testHashCode() {
    TextFormatParseLocation loc0 = TextFormatParseLocation.create(2, 1);
    TextFormatParseLocation loc1 = TextFormatParseLocation.create(2, 1);

    assertThat(loc0.hashCode()).isEqualTo(loc1.hashCode());
    assertThat(TextFormatParseLocation.EMPTY.hashCode())
        .isEqualTo(TextFormatParseLocation.EMPTY.hashCode());
  }

  @Test
  public void testEquals() {
    TextFormatParseLocation loc0 = TextFormatParseLocation.create(2, 1);
    TextFormatParseLocation loc1 = TextFormatParseLocation.create(1, 2);
    TextFormatParseLocation loc2 = TextFormatParseLocation.create(2, 2);
    TextFormatParseLocation loc3 = TextFormatParseLocation.create(2, 1);

    assertThat(loc0).isEqualTo(loc3);
    assertThat(loc0).isNotSameInstanceAs(loc1);
    assertThat(loc0).isNotSameInstanceAs(loc2);
    assertThat(loc1).isNotSameInstanceAs(loc2);
  }
}
