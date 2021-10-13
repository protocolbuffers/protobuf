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

import static java.lang.Character.MIN_HIGH_SURROGATE;
import static java.lang.Character.MIN_LOW_SURROGATE;
import static java.lang.Character.MIN_SURROGATE;

import java.util.Random;

/** Utilities for benchmarking UTF-8. */
final class Utf8Utils {
  private Utf8Utils() {}

  static class MaxCodePoint {
    final int value;

    /**
     * Convert the input string to a code point. Accepts regular decimal numerals, hex strings, and
     * some symbolic names meaningful to humans.
     */
    private static int decode(String userFriendly) {
      try {
        return Integer.decode(userFriendly);
      } catch (NumberFormatException ignored) {
        if (userFriendly.matches("(?i)(?:American|English|ASCII)")) {
          // 1-byte UTF-8 sequences - "American" ASCII text
          return 0x80;
        } else if (userFriendly.matches("(?i)(?:Danish|Latin|Western.*European)")) {
          // Mostly 1-byte UTF-8 sequences, mixed with occasional 2-byte
          // sequences - "Western European" text
          return 0x90;
        } else if (userFriendly.matches("(?i)(?:Greek|Cyrillic|European|ISO.?8859)")) {
          // Mostly 2-byte UTF-8 sequences - "European" text
          return 0x800;
        } else if (userFriendly.matches("(?i)(?:Chinese|Han|Asian|BMP)")) {
          // Mostly 3-byte UTF-8 sequences - "Asian" text
          return Character.MIN_SUPPLEMENTARY_CODE_POINT;
        } else if (userFriendly.matches("(?i)(?:Cuneiform|rare|exotic|supplementary.*)")) {
          // Mostly 4-byte UTF-8 sequences - "rare exotic" text
          return Character.MAX_CODE_POINT;
        } else {
          throw new IllegalArgumentException("Can't decode codepoint " + userFriendly);
        }
      }
    }

    public static MaxCodePoint valueOf(String userFriendly) {
      return new MaxCodePoint(userFriendly);
    }

    public MaxCodePoint(String userFriendly) {
      value = decode(userFriendly);
    }
  }

  /**
   * The Utf8 distribution of real data. The distribution is an array with length 4.
   * "distribution[i]" means the total number of characters who are encoded with (i + 1) bytes.
   *
   * <p>GMM_UTF8_DISTRIBUTION is the distribution of gmm data set. GSR_UTF8_DISTRIBUTION is the
   * distribution of gsreq/gsresp data set
   */
  public enum Utf8Distribution {
    GMM_UTF8_DISTRIBUTION {
      @Override
      public int[] getDistribution() {
        return new int[] {53059, 104, 0, 0};
      }
    },
    GSR_UTF8_DISTRIBUTION {
      @Override
      public int[] getDistribution() {
        return new int[] {119458, 74, 2706, 0};
      }
    };

    public abstract int[] getDistribution();
  }

  /**
   * Creates an array of random strings.
   *
   * @param stringCount the number of strings to be created.
   * @param charCount the number of characters per string.
   * @param maxCodePoint the maximum code point for the characters in the strings.
   * @return an array of random strings.
   */
  static String[] randomStrings(int stringCount, int charCount, MaxCodePoint maxCodePoint) {
    final long seed = 99;
    final Random rnd = new Random(seed);
    String[] strings = new String[stringCount];
    for (int i = 0; i < stringCount; i++) {
      strings[i] = randomString(rnd, charCount, maxCodePoint);
    }
    return strings;
  }

  /**
   * Creates a random string
   *
   * @param rnd the random generator.
   * @param charCount the number of characters per string.
   * @param maxCodePoint the maximum code point for the characters in the strings.
   */
  static String randomString(Random rnd, int charCount, MaxCodePoint maxCodePoint) {
    StringBuilder sb = new StringBuilder();
    for (int i = 0; i < charCount; i++) {
      int codePoint;
      do {
        codePoint = rnd.nextInt(maxCodePoint.value);
      } while (Utf8Utils.isSurrogate(codePoint));
      sb.appendCodePoint(codePoint);
    }
    return sb.toString();
  }

  /** Character.isSurrogate was added in Java SE 7. */
  static boolean isSurrogate(int c) {
    return Character.MIN_HIGH_SURROGATE <= c && c <= Character.MAX_LOW_SURROGATE;
  }

  /**
   * Creates an array of random strings according to UTF8 distribution.
   *
   * @param stringCount the number of strings to be created.
   * @param charCount the number of characters per string.
   */
  static String[] randomStringsWithDistribution(
      int stringCount, int charCount, Utf8Distribution utf8Distribution) {
    final int[] distribution = utf8Distribution.getDistribution();
    for (int i = 0; i < 3; i++) {
      distribution[i + 1] += distribution[i];
    }
    final long seed = 99;
    final Random rnd = new Random(seed);
    String[] strings = new String[stringCount];
    for (int i = 0; i < stringCount; i++) {
      StringBuilder sb = new StringBuilder();
      for (int j = 0; j < charCount; j++) {
        int codePoint;
        do {
          codePoint = rnd.nextInt(distribution[3]);
          if (codePoint < distribution[0]) {
            // 1 bytes
            sb.append((char) 0x7F);
          } else if (codePoint < distribution[1]) {
            // 2 bytes
            sb.append((char) 0x7FF);
          } else if (codePoint < distribution[2]) {
            // 3 bytes
            sb.append((char) (MIN_SURROGATE - 1));
          } else {
            // 4 bytes
            sb.append(MIN_HIGH_SURROGATE);
            sb.append(MIN_LOW_SURROGATE);
          }
        } while (Utf8Utils.isSurrogate(codePoint));
      }
      strings[i] = sb.toString();
    }
    return strings;
  }
}
