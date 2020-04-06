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

import java.util.Random;

/** Utility class that provides data primitives for filling out protobuf messages. */
public final class ExperimentalTestDataProvider {
  private static final Random RANDOM = new Random(100);

  private final Varint32Provider varint32s = new Varint32Provider();
  private final Varint64Provider varint64s = new Varint64Provider();
  private final int stringLength;

  public ExperimentalTestDataProvider(int stringLength) {
    this.stringLength = stringLength;
  }

  public double getDouble() {
    double value = 0.0;
    while (Double.compare(0.0, value) == 0) {
      value = RANDOM.nextDouble();
    }
    return value;
  }

  public float getFloat() {
    float value = 0.0f;
    while (Float.compare(0.0f, value) == 0) {
      value = RANDOM.nextFloat();
    }
    return value;
  }

  public long getLong() {
    return varint64s.getLong();
  }

  public int getInt() {
    return varint32s.getInt();
  }

  public boolean getBool() {
    return true;
  }

  public int getEnum() {
    return Math.abs(getInt()) % 3;
  }

  public String getString() {
    StringBuilder builder = new StringBuilder(stringLength);
    for (int i = 0; i < stringLength; ++i) {
      builder.append((char) (RANDOM.nextInt('z' - 'a') + 'a'));
    }
    return builder.toString();
  }

  public ByteString getBytes() {
    return ByteString.copyFromUtf8(getString());
  }

  /**
   * Iterator over integer values. Uses a simple distribution over 32-bit varints (generally
   * favoring smaller values).
   */
  private static final class Varint32Provider {
    private static final int[][] VALUES = {
      new int[] {1, 50, 100, 127}, // 1 byte values
      new int[] {128, 500, 10000, 16383}, // 2 bytes values
      new int[] {16384, 50000, 1000000, 2097151}, // 3 bytes values
      new int[] {2097152, 10000000, 200000000, 268435455}, // 4 bytes values
      new int[] {268435456, 0x30000000, 0x7FFFFFFF, 0xFFFFFFFF} // 5 bytes values
    };

    /** Number of samples that should be taken from each value array. */
    private static final int[] NUM_SAMPLES = {3, 2, 1, 1, 2};

    /**
     * The index into the {@link #VALUES} array that identifies the list of samples currently being
     * iterated over.
     */
    private int listIndex;

    /** The index of the next sample within a list. */
    private int sampleIndex;

    /** The number of successive samples that have been taken from the current list. */
    private int samplesTaken;

    public int getInt() {
      if (samplesTaken++ > NUM_SAMPLES[listIndex]) {
        // Done taking samples from this list. Go to the next one.
        listIndex = (listIndex + 1) % VALUES.length;
        sampleIndex = 0;
        samplesTaken = 0;
      }

      int value = VALUES[listIndex][sampleIndex];

      // All lists are exactly 4 long (i.e. power of 2), so we can optimize the mod operation
      // with masking.
      sampleIndex = (sampleIndex + 1) & 3;

      return value;
    }
  }

  /**
   * Iterator over integer values. Uses a simple distribution over 64-bit varints (generally
   * favoring smaller values).
   */
  private static final class Varint64Provider {
    private static final long[][] VALUES = {
      new long[] {1, 50, 100, 127},
      new long[] {128, 500, 10000, 16383},
      new long[] {16384, 50000, 1000000, 2097151},
      new long[] {2097152, 10000000, 200000000, 268435455},
      new long[] {268435456, 0x30000000, 0x7FFFFFFF, 34359738367L},
      new long[] {34359738368L, 2000000000000L, 4000000000000L, 4398046511103L},
      new long[] {4398046511104L, 200000000000000L, 500000000000000L, 562949953421311L},
      new long[] {0x4000000000000L, 0x5000000000000L, 0x6000000000000L, 0x0FFFFFFFFFFFFFFL},
      new long[] {0x100000000000000L, 0x3FFFFFFFFFFFFFFFL, 0x5FFFFFFFFFFFFFFL, 0x7FFFFFFFFFFFFFFFL},
      new long[] {
        0xFFFFFFFFFFFFFFFFL, 0xFFFFFFFFFFFFFFFFL, 0xFFFFFFFFFFFFFFFFL, 0xFFFFFFFFFFFFFFFFL
      }
    };

    /** Number of samples that should be taken from each value array. */
    private static final int[] NUM_SAMPLES = {4, 2, 2, 1, 1, 1, 1, 2, 2, 4};

    /**
     * The index into the {@link #VALUES} array that identifies the list of samples currently being
     * iterated over.
     */
    private int listIndex;

    /** The index of the next sample within a list. */
    private int sampleIndex;

    /** The number of successive samples that have been taken from the current list. */
    private int samplesTaken;

    public long getLong() {
      if (samplesTaken++ > NUM_SAMPLES[listIndex]) {
        // Done taking samples from this list. Go to the next one.
        listIndex = (listIndex + 1) % VALUES.length;
        sampleIndex = 0;
        samplesTaken = 0;
      }

      long value = VALUES[listIndex][sampleIndex];

      // All lists are exactly 4 long (i.e. power of 2), so we can optimize the mod operation
      // with masking.
      sampleIndex = (sampleIndex + 1) & 3;

      return value;
    }
  }
}
