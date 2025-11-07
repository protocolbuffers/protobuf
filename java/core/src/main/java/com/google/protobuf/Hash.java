// Protocol Buffers - Google's data interchange format
// Copyright 2025 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

package com.google.protobuf;

final class Hash {

  static final int SEED = System.identityHashCode(Hash.class);

  /**
   * Mix two {@code int}.
   *
   * <p>Logic adapted from Abseil for mixing two integrals but scaled down for 4-byte instead of
   * 8-byte.
   */
  private static int mix(int lhs, int rhs) {
    final long m = Integer.toUnsignedLong(lhs) * Integer.toUnsignedLong(rhs);
    return ((int) ((m >> 32) & 0xffffffff)) ^ ((int) (m & 0xffffffff));
  }

  /**
   * Combines two {@link Object#hashCode}.
   *
   * <p>Logic adapted from Abseil for mixing two integrals but scaled down for 4-byte instead of
   * 8-byte.
   */
  static int combine(int state, int value) {
    return mix(state ^ value, 0xde1e8cf5);
  }

  static int hash(int value) {
    return combine(SEED, value);
  }

  private Hash() {}
}
