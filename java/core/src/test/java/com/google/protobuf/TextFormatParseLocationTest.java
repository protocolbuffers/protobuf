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
