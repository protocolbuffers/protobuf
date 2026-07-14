// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

package com.google.protobuf;

import static com.google.common.truth.Truth.assertThat;

import com.google.protobuf.Internal.BitmaskEnumVerifier;
import com.google.protobuf.Internal.SequentialEnumVerifier;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

/** Tests for {@link Internal} helper classes. */
@RunWith(JUnit4.class)
public class InternalTest {

  @Test
  public void testSequentialEnumVerifier() {
    SequentialEnumVerifier verifier = new SequentialEnumVerifier(2, 5);
    assertThat(verifier.isInRange(1)).isFalse();
    assertThat(verifier.isInRange(2)).isTrue();
    assertThat(verifier.isInRange(3)).isTrue();
    assertThat(verifier.isInRange(4)).isTrue();
    assertThat(verifier.isInRange(5)).isTrue();
    assertThat(verifier.isInRange(6)).isFalse();
  }

  @Test
  public void testSequentialEnumVerifier_singleValue() {
    SequentialEnumVerifier verifier = new SequentialEnumVerifier(3, 3);
    assertThat(verifier.isInRange(2)).isFalse();
    assertThat(verifier.isInRange(3)).isTrue();
    assertThat(verifier.isInRange(4)).isFalse();
  }

  @Test
  public void testBitmaskEnumVerifier() {
    // Mask for values: 2, 3, 5, 8
    // mask = (1L << 2) | (1L << 3) | (1L << 5) | (1L << 8) = 4 + 8 + 32 + 256 = 300
    BitmaskEnumVerifier verifier = new BitmaskEnumVerifier(300L);

    assertThat(verifier.isInRange(1)).isFalse();
    assertThat(verifier.isInRange(2)).isTrue();
    assertThat(verifier.isInRange(3)).isTrue();
    assertThat(verifier.isInRange(4)).isFalse();
    assertThat(verifier.isInRange(5)).isTrue();
    assertThat(verifier.isInRange(6)).isFalse();
    assertThat(verifier.isInRange(7)).isFalse();
    assertThat(verifier.isInRange(8)).isTrue();
    assertThat(verifier.isInRange(9)).isFalse();

    // Test boundaries
    assertThat(verifier.isInRange(-1)).isFalse();
    assertThat(verifier.isInRange(63)).isFalse();
    assertThat(verifier.isInRange(64)).isFalse();
  }

  @Test
  public void testBitmaskEnumVerifier_boundaryBit63() {
    // mask has only bit 63 set
    BitmaskEnumVerifier verifier = new BitmaskEnumVerifier(1L << 63);
    assertThat(verifier.isInRange(0)).isFalse();
    assertThat(verifier.isInRange(62)).isFalse();
    assertThat(verifier.isInRange(63)).isTrue();
    assertThat(verifier.isInRange(64)).isFalse();
  }
}
