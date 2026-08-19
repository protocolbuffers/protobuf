// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

package com.google.protobuf;

import static com.google.common.truth.Truth.assertThat;

import com.google.protobuf.Internal.BitmaskEnumMap;
import com.google.protobuf.Internal.BitmaskEnumVerifier;
import com.google.protobuf.Internal.SequentialEnumMap;
import com.google.protobuf.Internal.SequentialEnumVerifier;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

/** Tests for {@link Internal} helper classes. */
@RunWith(JUnit4.class)
public class InternalTest {

  @SuppressWarnings("ShouldNotSubclass")
  private enum TestEnum implements Internal.EnumLite {
    ZERO(0),
    ONE(1),
    TWO(2),
    THREE(3),
    FOUR(4),
    FIVE(5),
    EIGHT(8),
    SIXTY_THREE(63);

    private final int number;

    TestEnum(int number) {
      this.number = number;
    }

    @Override
    public int getNumber() {
      return number;
    }
  }

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

  @Test
  public void testSequentialEnumMap() {
    SequentialEnumMap<TestEnum> map =
        new SequentialEnumMap<>(
            2, new TestEnum[] {TestEnum.TWO, TestEnum.THREE, TestEnum.FOUR, TestEnum.FIVE});
    assertThat(map.findValueByNumber(1)).isNull();
    assertThat(map.findValueByNumber(2)).isEqualTo(TestEnum.TWO);
    assertThat(map.findValueByNumber(3)).isEqualTo(TestEnum.THREE);
    assertThat(map.findValueByNumber(4)).isEqualTo(TestEnum.FOUR);
    assertThat(map.findValueByNumber(5)).isEqualTo(TestEnum.FIVE);
    assertThat(map.findValueByNumber(6)).isNull();
    assertThat(map.findValueByNumber(-100)).isNull();
    assertThat(map.findValueByNumber(100)).isNull();
    assertThat(map.findValueByNumber(Integer.MIN_VALUE)).isNull();
    assertThat(map.findValueByNumber(Integer.MAX_VALUE)).isNull();
  }

  @Test
  public void testSequentialEnumMap_zeroMin() {
    SequentialEnumMap<TestEnum> map =
        new SequentialEnumMap<>(new TestEnum[] {TestEnum.ZERO, TestEnum.ONE});
    assertThat(map.findValueByNumber(-1)).isNull();
    assertThat(map.findValueByNumber(0)).isEqualTo(TestEnum.ZERO);
    assertThat(map.findValueByNumber(1)).isEqualTo(TestEnum.ONE);
    assertThat(map.findValueByNumber(2)).isNull();
  }

  @Test
  public void testSequentialEnumMap_singleValue() {
    SequentialEnumMap<TestEnum> map = new SequentialEnumMap<>(3, new TestEnum[] {TestEnum.THREE});
    assertThat(map.findValueByNumber(2)).isNull();
    assertThat(map.findValueByNumber(3)).isEqualTo(TestEnum.THREE);
    assertThat(map.findValueByNumber(4)).isNull();
  }

  @Test
  public void testBitmaskEnumMap() {
    // Mask for values: 2, 3, 5, 8
    // mask = (1L << 2) | (1L << 3) | (1L << 5) | (1L << 8) = 4 + 8 + 32 + 256 = 300
    BitmaskEnumMap<TestEnum> map =
        new BitmaskEnumMap<>(
            300L, new TestEnum[] {TestEnum.TWO, TestEnum.THREE, TestEnum.FIVE, TestEnum.EIGHT});

    assertThat(map.findValueByNumber(1)).isNull();
    assertThat(map.findValueByNumber(2)).isEqualTo(TestEnum.TWO);
    assertThat(map.findValueByNumber(3)).isEqualTo(TestEnum.THREE);
    assertThat(map.findValueByNumber(4)).isNull();
    assertThat(map.findValueByNumber(5)).isEqualTo(TestEnum.FIVE);
    assertThat(map.findValueByNumber(6)).isNull();
    assertThat(map.findValueByNumber(7)).isNull();
    assertThat(map.findValueByNumber(8)).isEqualTo(TestEnum.EIGHT);
    assertThat(map.findValueByNumber(9)).isNull();

    // Test boundaries
    assertThat(map.findValueByNumber(-1)).isNull();
    assertThat(map.findValueByNumber(63)).isNull();
    assertThat(map.findValueByNumber(64)).isNull();
    assertThat(map.findValueByNumber(Integer.MIN_VALUE)).isNull();
    assertThat(map.findValueByNumber(Integer.MAX_VALUE)).isNull();
  }

  @Test
  public void testBitmaskEnumMap_boundaryBit63() {
    // mask has only bit 63 set
    BitmaskEnumMap<TestEnum> map =
        new BitmaskEnumMap<>(1L << 63, new TestEnum[] {TestEnum.SIXTY_THREE});
    assertThat(map.findValueByNumber(0)).isNull();
    assertThat(map.findValueByNumber(62)).isNull();
    assertThat(map.findValueByNumber(63)).isEqualTo(TestEnum.SIXTY_THREE);
    assertThat(map.findValueByNumber(64)).isNull();
  }
}
