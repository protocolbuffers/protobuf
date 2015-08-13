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

package com.google.protobuf.nano;

import com.google.protobuf.nano.MapFactories.MapFactory;

import java.io.IOException;
import java.nio.charset.Charset;
import java.util.Arrays;
import java.util.Map;
import java.util.Map.Entry;

/**
 * The classes contained within are used internally by the Protocol Buffer
 * library and generated message implementations. They are public only because
 * those generated messages do not reside in the {@code protobuf} package.
 * Others should not use this class directly.
 *
 * @author kenton@google.com (Kenton Varda)
 */
public final class InternalNano {

  public static final int TYPE_DOUBLE   = 1;
  public static final int TYPE_FLOAT    = 2;
  public static final int TYPE_INT64    = 3;
  public static final int TYPE_UINT64   = 4;
  public static final int TYPE_INT32    = 5;
  public static final int TYPE_FIXED64  = 6;
  public static final int TYPE_FIXED32  = 7;
  public static final int TYPE_BOOL     = 8;
  public static final int TYPE_STRING   = 9;
  public static final int TYPE_GROUP    = 10;
  public static final int TYPE_MESSAGE  = 11;
  public static final int TYPE_BYTES    = 12;
  public static final int TYPE_UINT32   = 13;
  public static final int TYPE_ENUM     = 14;
  public static final int TYPE_SFIXED32 = 15;
  public static final int TYPE_SFIXED64 = 16;
  public static final int TYPE_SINT32   = 17;
  public static final int TYPE_SINT64   = 18;

  protected static final Charset UTF_8 = Charset.forName("UTF-8");
  protected static final Charset ISO_8859_1 = Charset.forName("ISO-8859-1");

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
    return new String(bytes.getBytes(ISO_8859_1), InternalNano.UTF_8);
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
    return bytes.getBytes(ISO_8859_1);
  }

  /**
   * Helper function to convert a string into UTF-8 while turning the
   * UnsupportedEncodingException to a RuntimeException.
   */
  public static byte[] copyFromUtf8(final String text) {
    return text.getBytes(InternalNano.UTF_8);
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
  private static Object primitiveDefaultValue(int type) {
    switch (type) {
      case TYPE_BOOL:
        return Boolean.FALSE;
      case TYPE_BYTES:
        return WireFormatNano.EMPTY_BYTES;
      case TYPE_STRING:
        return "";
      case TYPE_FLOAT:
        return Float.valueOf(0);
      case TYPE_DOUBLE:
        return Double.valueOf(0);
      case TYPE_ENUM:
      case TYPE_FIXED32:
      case TYPE_INT32:
      case TYPE_UINT32:
      case TYPE_SINT32:
      case TYPE_SFIXED32:
        return Integer.valueOf(0);
      case TYPE_INT64:
      case TYPE_UINT64:
      case TYPE_SINT64:
      case TYPE_FIXED64:
      case TYPE_SFIXED64:
        return Long.valueOf(0L);
      case TYPE_MESSAGE:
      case TYPE_GROUP:
      default:
        throw new IllegalArgumentException(
            "Type: " + type + " is not a primitive type.");
    }
  }

  /**
   * Merges the map entry into the map field. Note this is only supposed to
   * be called by generated messages.
   *
   * @param map the map field; may be null, in which case a map will be
   *        instantiated using the {@link MapFactories.MapFactory}
   * @param input the input byte buffer
   * @param keyType key type, as defined in InternalNano.TYPE_*
   * @param valueType value type, as defined in InternalNano.TYPE_*
   * @param value an new instance of the value, if the value is a TYPE_MESSAGE;
   *        otherwise this parameter can be null and will be ignored.
   * @param keyTag wire tag for the key
   * @param valueTag wire tag for the value
   * @return the map field
   * @throws IOException
   */
  @SuppressWarnings("unchecked")
  public static final <K, V> Map<K, V> mergeMapEntry(
      CodedInputByteBufferNano input,
      Map<K, V> map,
      MapFactory mapFactory,
      int keyType,
      int valueType,
      V value,
      int keyTag,
      int valueTag) throws IOException {
    map = mapFactory.forMap(map);
    final int length = input.readRawVarint32();
    final int oldLimit = input.pushLimit(length);
    K key = null;
    while (true) {
      int tag = input.readTag();
      if (tag == 0) {
        break;
      }
      if (tag == keyTag) {
        key = (K) input.readPrimitiveField(keyType);
      } else if (tag == valueTag) {
        if (valueType == TYPE_MESSAGE) {
          input.readMessage((MessageNano) value);
        } else {
          value = (V) input.readPrimitiveField(valueType);
        }
      } else {
        if (!input.skipField(tag)) {
          break;
        }
      }
    }
    input.checkLastTagWas(0);
    input.popLimit(oldLimit);

    if (key == null) {
      // key can only be primitive types.
      key = (K) primitiveDefaultValue(keyType);
    }

    if (value == null) {
      // message type value will be initialized by code-gen.
      value = (V) primitiveDefaultValue(valueType);
    }

    map.put(key, value);
    return map;
  }

  public static <K, V> void serializeMapField(
      CodedOutputByteBufferNano output,
      Map<K, V> map, int number, int keyType, int valueType)
          throws IOException {
    for (Entry<K, V> entry: map.entrySet()) {
      K key = entry.getKey();
      V value = entry.getValue();
      if (key == null || value == null) {
        throw new IllegalStateException(
            "keys and values in maps cannot be null");
      }
      int entrySize =
          CodedOutputByteBufferNano.computeFieldSize(1, keyType, key) +
          CodedOutputByteBufferNano.computeFieldSize(2, valueType, value);
      output.writeTag(number, WireFormatNano.WIRETYPE_LENGTH_DELIMITED);
      output.writeRawVarint32(entrySize);
      output.writeField(1, keyType, key);
      output.writeField(2, valueType, value);
    }
  }

  public static <K, V> int computeMapFieldSize(
      Map<K, V> map, int number, int keyType, int valueType) {
    int size = 0;
    int tagSize = CodedOutputByteBufferNano.computeTagSize(number);
    for (Entry<K, V> entry: map.entrySet()) {
      K key = entry.getKey();
      V value = entry.getValue();
      if (key == null || value == null) {
        throw new IllegalStateException(
            "keys and values in maps cannot be null");
      }
      int entrySize =
          CodedOutputByteBufferNano.computeFieldSize(1, keyType, key) +
          CodedOutputByteBufferNano.computeFieldSize(2, valueType, value);
      size += tagSize + entrySize
          + CodedOutputByteBufferNano.computeRawVarint32Size(entrySize);
    }
    return size;
  }

  /**
   * Checks whether two {@link Map} are equal. We don't use the default equals
   * method of {@link Map} because it compares by identity not by content for
   * byte arrays.
   */
  public static <K, V> boolean equals(Map<K, V> a, Map<K, V> b) {
    if (a == b) {
      return true;
    }
    if (a == null) {
      return b.size() == 0;
    }
    if (b == null) {
      return a.size() == 0;
    }
    if (a.size() != b.size()) {
      return false;
    }
    for (Entry<K, V> entry : a.entrySet()) {
      if (!b.containsKey(entry.getKey())) {
        return false;
      }
      if (!equalsMapValue(entry.getValue(), b.get(entry.getKey()))) {
        return false;
      }
    }
    return true;
  }

  private static boolean equalsMapValue(Object a, Object b) {
    if (a == null || b == null) {
      throw new IllegalStateException(
          "keys and values in maps cannot be null");
    }
    if (a instanceof byte[] && b instanceof byte[]) {
      return Arrays.equals((byte[]) a, (byte[]) b);
    }
    return a.equals(b);
  }

  public static <K, V> int hashCode(Map<K, V> map) {
    if (map == null) {
      return 0;
    }
    int result = 0;
    for (Entry<K, V> entry : map.entrySet()) {
      result += hashCodeForMap(entry.getKey())
          ^ hashCodeForMap(entry.getValue());
    }
    return result;
  }

  private static int hashCodeForMap(Object o) {
    if (o instanceof byte[]) {
      return Arrays.hashCode((byte[]) o);
    }
    return o.hashCode();
  }

  // This avoids having to make FieldArray public.
  public static void cloneUnknownFieldData(ExtendableMessageNano original,
      ExtendableMessageNano cloned) {
    if (original.unknownFieldData != null) {
      cloned.unknownFieldData = (FieldArray) original.unknownFieldData.clone();
    }
  }
}
