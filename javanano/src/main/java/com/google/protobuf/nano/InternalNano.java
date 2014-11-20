// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
// http://code.google.com/p/protobuf/
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

package com.google.protobuf.nano;

import java.io.UnsupportedEncodingException;
import java.util.Arrays;

/**
 * The classes contained within are used internally by the Protocol Buffer
 * library and generated message implementations. They are public only because
 * those generated messages do not reside in the {@code protobuf} package.
 * Others should not use this class directly.
 *
 * @author kenton@google.com (Kenton Varda)
 */
public final class InternalNano {

  private InternalNano() {}

  /**
   * An object to provide synchronization when lazily initializing static fields
   * of {@link MessageNano} subclasses.
   * <p>
   * To enable earlier versions of ProGuard to inline short methods from a
   * generated MessageNano subclass to the call sites, that class must not have
   * a class initializer, which will be created if there is any static variable
   * initializers. To lazily initialize the static variables in a thread-safe
   * manner, the initialization code will synchronize on this object.
   */
  public static final Object LAZY_INIT_LOCK = new Object();

  /**
   * Helper called by generated code to construct default values for string
   * fields.
   * <p>
   * The protocol compiler does not actually contain a UTF-8 decoder -- it
   * just pushes UTF-8-encoded text around without touching it.  The one place
   * where this presents a problem is when generating Java string literals.
   * Unicode characters in the string literal would normally need to be encoded
   * using a Unicode escape sequence, which would require decoding them.
   * To get around this, protoc instead embeds the UTF-8 bytes into the
   * generated code and leaves it to the runtime library to decode them.
   * <p>
   * It gets worse, though.  If protoc just generated a byte array, like:
   *   new byte[] {0x12, 0x34, 0x56, 0x78}
   * Java actually generates *code* which allocates an array and then fills
   * in each value.  This is much less efficient than just embedding the bytes
   * directly into the bytecode.  To get around this, we need another
   * work-around.  String literals are embedded directly, so protoc actually
   * generates a string literal corresponding to the bytes.  The easiest way
   * to do this is to use the ISO-8859-1 character set, which corresponds to
   * the first 256 characters of the Unicode range.  Protoc can then use
   * good old CEscape to generate the string.
   * <p>
   * So we have a string literal which represents a set of bytes which
   * represents another string.  This function -- stringDefaultValue --
   * converts from the generated string to the string we actually want.  The
   * generated code calls this automatically.
   */
  public static String stringDefaultValue(String bytes) {
    try {
      return new String(bytes.getBytes("ISO-8859-1"), "UTF-8");
    } catch (UnsupportedEncodingException e) {
      // This should never happen since all JVMs are required to implement
      // both of the above character sets.
      throw new IllegalStateException(
          "Java VM does not support a standard character set.", e);
    }
  }

  /**
   * Helper called by generated code to construct default values for bytes
   * fields.
   * <p>
   * This is a lot like {@link #stringDefaultValue}, but for bytes fields.
   * In this case we only need the second of the two hacks -- allowing us to
   * embed raw bytes as a string literal with ISO-8859-1 encoding.
   */
  public static byte[] bytesDefaultValue(String bytes) {
    try {
      return bytes.getBytes("ISO-8859-1");
    } catch (UnsupportedEncodingException e) {
      // This should never happen since all JVMs are required to implement
      // ISO-8859-1.
      throw new IllegalStateException(
          "Java VM does not support a standard character set.", e);
    }
  }

  /**
   * Helper function to convert a string into UTF-8 while turning the
   * UnsupportedEncodingException to a RuntimeException.
   */
  public static byte[] copyFromUtf8(final String text) {
    try {
      return text.getBytes("UTF-8");
    } catch (UnsupportedEncodingException e) {
      throw new RuntimeException("UTF-8 not supported?");
    }
  }

  /**
   * Checks repeated int field equality; null-value and 0-length fields are
   * considered equal.
   */
  public static boolean equals(int[] field1, int[] field2) {
    if (field1 == null || field1.length == 0) {
      return field2 == null || field2.length == 0;
    } else {
      return Arrays.equals(field1, field2);
    }
  }

  /**
   * Checks repeated long field equality; null-value and 0-length fields are
   * considered equal.
   */
  public static boolean equals(long[] field1, long[] field2) {
    if (field1 == null || field1.length == 0) {
      return field2 == null || field2.length == 0;
    } else {
      return Arrays.equals(field1, field2);
    }
  }

  /**
   * Checks repeated float field equality; null-value and 0-length fields are
   * considered equal.
   */
  public static boolean equals(float[] field1, float[] field2) {
    if (field1 == null || field1.length == 0) {
      return field2 == null || field2.length == 0;
    } else {
      return Arrays.equals(field1, field2);
    }
  }

  /**
   * Checks repeated double field equality; null-value and 0-length fields are
   * considered equal.
   */
  public static boolean equals(double[] field1, double[] field2) {
    if (field1 == null || field1.length == 0) {
      return field2 == null || field2.length == 0;
    } else {
      return Arrays.equals(field1, field2);
    }
  }

  /**
   * Checks repeated boolean field equality; null-value and 0-length fields are
   * considered equal.
   */
  public static boolean equals(boolean[] field1, boolean[] field2) {
    if (field1 == null || field1.length == 0) {
      return field2 == null || field2.length == 0;
    } else {
      return Arrays.equals(field1, field2);
    }
  }

  /**
   * Checks repeated bytes field equality. Only non-null elements are tested.
   * Returns true if the two fields have the same sequence of non-null
   * elements. Null-value fields and fields of any length with only null
   * elements are considered equal.
   */
  public static boolean equals(byte[][] field1, byte[][] field2) {
    int index1 = 0;
    int length1 = field1 == null ? 0 : field1.length;
    int index2 = 0;
    int length2 = field2 == null ? 0 : field2.length;
    while (true) {
      while (index1 < length1 && field1[index1] == null) {
        index1++;
      }
      while (index2 < length2 && field2[index2] == null) {
        index2++;
      }
      boolean atEndOf1 = index1 >= length1;
      boolean atEndOf2 = index2 >= length2;
      if (atEndOf1 && atEndOf2) {
        // no more non-null elements to test in both arrays
        return true;
      } else if (atEndOf1 != atEndOf2) {
        // one of the arrays have extra non-null elements
        return false;
      } else if (!Arrays.equals(field1[index1], field2[index2])) {
        // element mismatch
        return false;
      }
      index1++;
      index2++;
    }
  }

  /**
   * Checks repeated string/message field equality. Only non-null elements are
   * tested. Returns true if the two fields have the same sequence of non-null
   * elements. Null-value fields and fields of any length with only null
   * elements are considered equal.
   */
  public static boolean equals(Object[] field1, Object[] field2) {
    int index1 = 0;
    int length1 = field1 == null ? 0 : field1.length;
    int index2 = 0;
    int length2 = field2 == null ? 0 : field2.length;
    while (true) {
      while (index1 < length1 && field1[index1] == null) {
        index1++;
      }
      while (index2 < length2 && field2[index2] == null) {
        index2++;
      }
      boolean atEndOf1 = index1 >= length1;
      boolean atEndOf2 = index2 >= length2;
      if (atEndOf1 && atEndOf2) {
        // no more non-null elements to test in both arrays
        return true;
      } else if (atEndOf1 != atEndOf2) {
        // one of the arrays have extra non-null elements
        return false;
      } else if (!field1[index1].equals(field2[index2])) {
        // element mismatch
        return false;
      }
      index1++;
      index2++;
    }
  }

  /**
   * Computes the hash code of a repeated int field. Null-value and 0-length
   * fields have the same hash code.
   */
  public static int hashCode(int[] field) {
    return field == null || field.length == 0 ? 0 : Arrays.hashCode(field);
  }

  /**
   * Computes the hash code of a repeated long field. Null-value and 0-length
   * fields have the same hash code.
   */
  public static int hashCode(long[] field) {
    return field == null || field.length == 0 ? 0 : Arrays.hashCode(field);
  }

  /**
   * Computes the hash code of a repeated float field. Null-value and 0-length
   * fields have the same hash code.
   */
  public static int hashCode(float[] field) {
    return field == null || field.length == 0 ? 0 : Arrays.hashCode(field);
  }

  /**
   * Computes the hash code of a repeated double field. Null-value and 0-length
   * fields have the same hash code.
   */
  public static int hashCode(double[] field) {
    return field == null || field.length == 0 ? 0 : Arrays.hashCode(field);
  }

  /**
   * Computes the hash code of a repeated boolean field. Null-value and 0-length
   * fields have the same hash code.
   */
  public static int hashCode(boolean[] field) {
    return field == null || field.length == 0 ? 0 : Arrays.hashCode(field);
  }

  /**
   * Computes the hash code of a repeated bytes field. Only the sequence of all
   * non-null elements are used in the computation. Null-value fields and fields
   * of any length with only null elements have the same hash code.
   */
  public static int hashCode(byte[][] field) {
    int result = 0;
    for (int i = 0, size = field == null ? 0 : field.length; i < size; i++) {
      byte[] element = field[i];
      if (element != null) {
        result = 31 * result + Arrays.hashCode(element);
      }
    }
    return result;
  }

  /**
   * Computes the hash code of a repeated string/message field. Only the
   * sequence of all non-null elements are used in the computation. Null-value
   * fields and fields of any length with only null elements have the same hash
   * code.
   */
  public static int hashCode(Object[] field) {
    int result = 0;
    for (int i = 0, size = field == null ? 0 : field.length; i < size; i++) {
      Object element = field[i];
      if (element != null) {
        result = 31 * result + element.hashCode();
      }
    }
    return result;
  }

}
