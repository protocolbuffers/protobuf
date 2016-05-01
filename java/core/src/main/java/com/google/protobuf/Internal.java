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

import java.io.IOException;
import java.lang.reflect.Method;
import java.nio.ByteBuffer;
import java.nio.charset.Charset;
import java.util.AbstractList;
import java.util.AbstractMap;
import java.util.AbstractSet;
import java.util.Arrays;
import java.util.Iterator;
import java.util.List;
import java.util.Map;
import java.util.RandomAccess;
import java.util.Set;

/**
 * The classes contained within are used internally by the Protocol Buffer
 * library and generated message implementations. They are public only because
 * those generated messages do not reside in the {@code protobuf} package.
 * Others should not use this class directly.
 *
 * @author kenton@google.com (Kenton Varda)
 */
public final class Internal {

  private Internal() {}

  static final Charset UTF_8 = Charset.forName("UTF-8");
  static final Charset ISO_8859_1 = Charset.forName("ISO-8859-1");

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
    return new String(bytes.getBytes(ISO_8859_1), UTF_8);
  }

  /**
   * Helper called by generated code to construct default values for bytes
   * fields.
   * <p>
   * This is a lot like {@link #stringDefaultValue}, but for bytes fields.
   * In this case we only need the second of the two hacks -- allowing us to
   * embed raw bytes as a string literal with ISO-8859-1 encoding.
   */
  public static ByteString bytesDefaultValue(String bytes) {
    return ByteString.copyFrom(bytes.getBytes(ISO_8859_1));
  }
  /**
   * Helper called by generated code to construct default values for bytes
   * fields.
   * <p>
   * This is like {@link #bytesDefaultValue}, but returns a byte array.
   */
  public static byte[] byteArrayDefaultValue(String bytes) {
    return bytes.getBytes(ISO_8859_1);
  }

  /**
   * Helper called by generated code to construct default values for bytes
   * fields.
   * <p>
   * This is like {@link #bytesDefaultValue}, but returns a ByteBuffer.
   */
  public static ByteBuffer byteBufferDefaultValue(String bytes) {
    return ByteBuffer.wrap(byteArrayDefaultValue(bytes));
  }

  /**
   * Create a new ByteBuffer and copy all the content of {@code source}
   * ByteBuffer to the new ByteBuffer. The new ByteBuffer's limit and
   * capacity will be source.capacity(), and its position will be 0.
   * Note that the state of {@code source} ByteBuffer won't be changed.
   */
  public static ByteBuffer copyByteBuffer(ByteBuffer source) {
    // Make a duplicate of the source ByteBuffer and read data from the
    // duplicate. This is to avoid affecting the source ByteBuffer's state.
    ByteBuffer temp = source.duplicate();
    // We want to copy all the data in the source ByteBuffer, not just the
    // remaining bytes.
    temp.clear();
    ByteBuffer result = ByteBuffer.allocate(temp.capacity());
    result.put(temp);
    result.clear();
    return result;
  }

  /**
   * Helper called by generated code to determine if a byte array is a valid
   * UTF-8 encoded string such that the original bytes can be converted to
   * a String object and then back to a byte array round tripping the bytes
   * without loss.  More precisely, returns {@code true} whenever:
   * <pre>   {@code
   * Arrays.equals(byteString.toByteArray(),
   *     new String(byteString.toByteArray(), "UTF-8").getBytes("UTF-8"))
   * }</pre>
   *
   * <p>This method rejects "overlong" byte sequences, as well as
   * 3-byte sequences that would map to a surrogate character, in
   * accordance with the restricted definition of UTF-8 introduced in
   * Unicode 3.1.  Note that the UTF-8 decoder included in Oracle's
   * JDK has been modified to also reject "overlong" byte sequences,
   * but currently (2011) still accepts 3-byte surrogate character
   * byte sequences.
   *
   * <p>See the Unicode Standard,<br>
   * Table 3-6. <em>UTF-8 Bit Distribution</em>,<br>
   * Table 3-7. <em>Well Formed UTF-8 Byte Sequences</em>.
   *
   * <p>As of 2011-02, this method simply returns the result of {@link
   * ByteString#isValidUtf8()}.  Calling that method directly is preferred.
   *
   * @param byteString the string to check
   * @return whether the byte array is round trippable
   */
  public static boolean isValidUtf8(ByteString byteString) {
    return byteString.isValidUtf8();
  }

  /**
   * Like {@link #isValidUtf8(ByteString)} but for byte arrays.
   */
  public static boolean isValidUtf8(byte[] byteArray) {
    return Utf8.isValidUtf8(byteArray);
  }

  /**
   * Helper method to get the UTF-8 bytes of a string.
   */
  public static byte[] toByteArray(String value) {
    return value.getBytes(UTF_8);
  }

  /**
   * Helper method to convert a byte array to a string using UTF-8 encoding.
   */
  public static String toStringUtf8(byte[] bytes) {
    return new String(bytes, UTF_8);
  }

  /**
   * Interface for an enum value or value descriptor, to be used in FieldSet.
   * The lite library stores enum values directly in FieldSets but the full
   * library stores EnumValueDescriptors in order to better support reflection.
   */
  public interface EnumLite {
    int getNumber();
  }

  /**
   * Interface for an object which maps integers to {@link EnumLite}s.
   * {@link Descriptors.EnumDescriptor} implements this interface by mapping
   * numbers to {@link Descriptors.EnumValueDescriptor}s.  Additionally,
   * every generated enum type has a static method internalGetValueMap() which
   * returns an implementation of this type that maps numbers to enum values.
   */
  public interface EnumLiteMap<T extends EnumLite> {
    T findValueByNumber(int number);
  }

  /**
   * Helper method for implementing {@link Message#hashCode()} for longs.
   * @see Long#hashCode()
   */
  public static int hashLong(long n) {
    return (int) (n ^ (n >>> 32));
  }

  /**
   * Helper method for implementing {@link Message#hashCode()} for
   * booleans.
   * @see Boolean#hashCode()
   */
  public static int hashBoolean(boolean b) {
    return b ? 1231 : 1237;
  }

  /**
   * Helper method for implementing {@link Message#hashCode()} for enums.
   * <p>
   * This is needed because {@link java.lang.Enum#hashCode()} is final, but we
   * need to use the field number as the hash code to ensure compatibility
   * between statically and dynamically generated enum objects.
   */
  public static int hashEnum(EnumLite e) {
    return e.getNumber();
  }

  /**
   * Helper method for implementing {@link Message#hashCode()} for
   * enum lists.
   */
  public static int hashEnumList(List<? extends EnumLite> list) {
    int hash = 1;
    for (EnumLite e : list) {
      hash = 31 * hash + hashEnum(e);
    }
    return hash;
  }

  /**
   * Helper method for implementing {@link Message#equals(Object)} for bytes field.
   */
  public static boolean equals(List<byte[]> a, List<byte[]> b) {
    if (a.size() != b.size()) return false;
    for (int i = 0; i < a.size(); ++i) {
      if (!Arrays.equals(a.get(i), b.get(i))) {
        return false;
      }
    }
    return true;
  }

  /**
   * Helper method for implementing {@link Message#hashCode()} for bytes field.
   */
  public static int hashCode(List<byte[]> list) {
    int hash = 1;
    for (byte[] bytes : list) {
      hash = 31 * hash + hashCode(bytes);
    }
    return hash;
  }

  /**
   * Helper method for implementing {@link Message#hashCode()} for bytes field.
   */
  public static int hashCode(byte[] bytes) {
    // The hash code for a byte array should be the same as the hash code for a
    // ByteString with the same content. This is to ensure that the generated
    // hashCode() method will return the same value as the pure reflection
    // based hashCode() method.
    return Internal.hashCode(bytes, 0, bytes.length);
  }
  
  /**
   * Helper method for implementing {@link LiteralByteString#hashCode()}.
   */
  static int hashCode(byte[] bytes, int offset, int length) {
    // The hash code for a byte array should be the same as the hash code for a
    // ByteString with the same content. This is to ensure that the generated
    // hashCode() method will return the same value as the pure reflection
    // based hashCode() method.
    int h = Internal.partialHash(length, bytes, offset, length);
    return h == 0 ? 1 : h;
  }

  /**
   * Helper method for continuously hashing bytes.
   */
  static int partialHash(int h, byte[] bytes, int offset, int length) {
    for (int i = offset; i < offset + length; i++) {
      h = h * 31 + bytes[i];
    }
    return h;
  }
  
  /**
   * Helper method for implementing {@link Message#equals(Object)} for bytes
   * field.
   */
  public static boolean equalsByteBuffer(ByteBuffer a, ByteBuffer b) {
    if (a.capacity() != b.capacity()) {
      return false;
    }
    // ByteBuffer.equals() will only compare the remaining bytes, but we want to
    // compare all the content.
    return a.duplicate().clear().equals(b.duplicate().clear());
  }

  /**
   * Helper method for implementing {@link Message#equals(Object)} for bytes
   * field.
   */
  public static boolean equalsByteBuffer(
      List<ByteBuffer> a, List<ByteBuffer> b) {
    if (a.size() != b.size()) {
      return false;
    }
    for (int i = 0; i < a.size(); ++i) {
      if (!equalsByteBuffer(a.get(i), b.get(i))) {
        return false;
      }
    }
    return true;
  }

  /**
   * Helper method for implementing {@link Message#hashCode()} for bytes
   * field.
   */
  public static int hashCodeByteBuffer(List<ByteBuffer> list) {
    int hash = 1;
    for (ByteBuffer bytes : list) {
      hash = 31 * hash + hashCodeByteBuffer(bytes);
    }
    return hash;
  }

  private static final int DEFAULT_BUFFER_SIZE = 4096;

  /**
   * Helper method for implementing {@link Message#hashCode()} for bytes
   * field.
   */
  public static int hashCodeByteBuffer(ByteBuffer bytes) {
    if (bytes.hasArray()) {
      // Fast path.
      int h = partialHash(bytes.capacity(), bytes.array(), bytes.arrayOffset(), bytes.capacity());
      return h == 0 ? 1 : h;
    } else {
      // Read the data into a temporary byte array before calculating the
      // hash value.
      final int bufferSize = bytes.capacity() > DEFAULT_BUFFER_SIZE
          ? DEFAULT_BUFFER_SIZE : bytes.capacity();
      final byte[] buffer = new byte[bufferSize];
      final ByteBuffer duplicated = bytes.duplicate();
      duplicated.clear();
      int h = bytes.capacity();
      while (duplicated.remaining() > 0) {
        final int length = duplicated.remaining() <= bufferSize ?
            duplicated.remaining() : bufferSize;
        duplicated.get(buffer, 0, length);
        h = partialHash(h, buffer, 0, length);
      }
      return h == 0 ? 1 : h;
    }
  }

  @SuppressWarnings("unchecked")
  public static <T extends MessageLite> T getDefaultInstance(Class<T> clazz) {
    try {
      Method method = clazz.getMethod("getDefaultInstance");
      return (T) method.invoke(method);
    } catch (Exception e) {
      throw new RuntimeException(
          "Failed to get default instance for " + clazz, e);
    }
  }

  /**
   * An empty byte array constant used in generated code.
   */
  public static final byte[] EMPTY_BYTE_ARRAY = new byte[0];

  /**
   * An empty byte array constant used in generated code.
   */
  public static final ByteBuffer EMPTY_BYTE_BUFFER =
      ByteBuffer.wrap(EMPTY_BYTE_ARRAY);

  /** An empty coded input stream constant used in generated code. */
  public static final CodedInputStream EMPTY_CODED_INPUT_STREAM =
      CodedInputStream.newInstance(EMPTY_BYTE_ARRAY);


  /**
   * Provides an immutable view of {@code List<T>} around a {@code List<F>}.
   *
   * Protobuf internal. Used in protobuf generated code only.
   */
  public static class ListAdapter<F, T> extends AbstractList<T> {
    /**
     * Convert individual elements of the List from F to T.
     */
    public interface Converter<F, T> {
      T convert(F from);
    }

    private final List<F> fromList;
    private final Converter<F, T> converter;

    public ListAdapter(List<F> fromList, Converter<F, T> converter) {
      this.fromList = fromList;
      this.converter = converter;
    }

    @Override
    public T get(int index) {
      return converter.convert(fromList.get(index));
    }

    @Override
    public int size() {
      return fromList.size();
    }
  }

  /**
   * Wrap around a {@code Map<K, RealValue>} and provide a {@code Map<K, V>}
   * interface.
   */
  public static class MapAdapter<K, V, RealValue> extends AbstractMap<K, V> {
    /**
     * An interface used to convert between two types.
     */
    public interface Converter<A, B> {
      B doForward(A object);
      A doBackward(B object);
    }

    public static <T extends EnumLite> Converter<Integer, T> newEnumConverter(
        final EnumLiteMap<T> enumMap, final T unrecognizedValue) {
      return new Converter<Integer, T>() {
        @Override
        public T doForward(Integer value) {
          T result = enumMap.findValueByNumber(value);
          return result == null ? unrecognizedValue : result;
        }

        @Override
        public Integer doBackward(T value) {
          return value.getNumber();
        }
      };
    }

    private final Map<K, RealValue> realMap;
    private final Converter<RealValue, V> valueConverter;

    public MapAdapter(Map<K, RealValue> realMap,
        Converter<RealValue, V> valueConverter) {
      this.realMap = realMap;
      this.valueConverter = valueConverter;
    }

    @SuppressWarnings("unchecked")
    @Override
    public V get(Object key) {
      RealValue result = realMap.get(key);
      if (result == null) {
        return null;
      }
      return valueConverter.doForward(result);
    }

    @Override
    public V put(K key, V value) {
      RealValue oldValue = realMap.put(key, valueConverter.doBackward(value));
      if (oldValue == null) {
        return null;
      }
      return valueConverter.doForward(oldValue);
    }

    @Override
    public Set<java.util.Map.Entry<K, V>> entrySet() {
      return new SetAdapter(realMap.entrySet());
    }

    private class SetAdapter extends AbstractSet<Map.Entry<K, V>> {
      private final Set<Map.Entry<K, RealValue>> realSet;
      public SetAdapter(Set<Map.Entry<K, RealValue>> realSet) {
        this.realSet = realSet;
      }

      @Override
      public Iterator<java.util.Map.Entry<K, V>> iterator() {
        return new IteratorAdapter(realSet.iterator());
      }

      @Override
      public int size() {
        return realSet.size();
      }
    }

    private class IteratorAdapter implements Iterator<Map.Entry<K, V>> {
      private final Iterator<Map.Entry<K, RealValue>> realIterator;

      public IteratorAdapter(
          Iterator<Map.Entry<K, RealValue>> realIterator) {
        this.realIterator = realIterator;
      }

      @Override
      public boolean hasNext() {
        return realIterator.hasNext();
      }

      @Override
      public java.util.Map.Entry<K, V> next() {
        return new EntryAdapter(realIterator.next());
      }

      @Override
      public void remove() {
        realIterator.remove();
      }
    }

    private class EntryAdapter implements Map.Entry<K, V> {
      private final Map.Entry<K, RealValue> realEntry;

      public EntryAdapter(Map.Entry<K, RealValue> realEntry) {
        this.realEntry = realEntry;
      }

      @Override
      public K getKey() {
        return realEntry.getKey();
      }

      @Override
      public V getValue() {
        return valueConverter.doForward(realEntry.getValue());
      }

      @Override
      public V setValue(V value) {
        RealValue oldValue = realEntry.setValue(
            valueConverter.doBackward(value));
        if (oldValue == null) {
          return null;
        }
        return valueConverter.doForward(oldValue);
      }
    }
  }

  /**
   * Extends {@link List} to add the capability to make the list immutable and inspect if it is
   * modifiable.
   * <p>
   * All implementations must support efficient random access.
   */
  public static interface ProtobufList<E> extends List<E>, RandomAccess {

    /**
     * Makes this list immutable. All subsequent modifications will throw an
     * {@link UnsupportedOperationException}.
     */
    void makeImmutable();

    /**
     * Returns whether this list can be modified via the publicly accessible {@link List} methods.
     */
    boolean isModifiable();

    /**
     * Returns a mutable clone of this list with the specified capacity.
     */
    ProtobufList<E> mutableCopyWithCapacity(int capacity);
  }

  /**
   * A {@link java.util.List} implementation that avoids boxing the elements into Integers if
   * possible. Does not support null elements.
   */
  public static interface IntList extends ProtobufList<Integer> {

    /**
     * Like {@link #get(int)} but more efficient in that it doesn't box the returned value.
     */
    int getInt(int index);

    /**
     * Like {@link #add(Integer)} but more efficient in that it doesn't box the element.
     */
    void addInt(int element);

    /**
     * Like {@link #set(int, Integer)} but more efficient in that it doesn't box the element.
     */
    int setInt(int index, int element);

    /**
     * Returns a mutable clone of this list with the specified capacity.
     */
    @Override
    IntList mutableCopyWithCapacity(int capacity);
  }

  /**
   * A {@link java.util.List} implementation that avoids boxing the elements into Booleans if
   * possible. Does not support null elements.
   */
  public static interface BooleanList extends ProtobufList<Boolean> {

    /**
     * Like {@link #get(int)} but more efficient in that it doesn't box the returned value.
     */
    boolean getBoolean(int index);

    /**
     * Like {@link #add(Boolean)} but more efficient in that it doesn't box the element.
     */
    void addBoolean(boolean element);

    /**
     * Like {@link #set(int, Boolean)} but more efficient in that it doesn't box the element.
     */
    boolean setBoolean(int index, boolean element);

    /**
     * Returns a mutable clone of this list with the specified capacity.
     */
    @Override
    BooleanList mutableCopyWithCapacity(int capacity);
  }

  /**
   * A {@link java.util.List} implementation that avoids boxing the elements into Longs if
   * possible. Does not support null elements.
   */
  public static interface LongList extends ProtobufList<Long> {

    /**
     * Like {@link #get(int)} but more efficient in that it doesn't box the returned value.
     */
    long getLong(int index);

    /**
     * Like {@link #add(Long)} but more efficient in that it doesn't box the element.
     */
    void addLong(long element);

    /**
     * Like {@link #set(int, Long)} but more efficient in that it doesn't box the element.
     */
    long setLong(int index, long element);

    /**
     * Returns a mutable clone of this list with the specified capacity.
     */
    @Override
    LongList mutableCopyWithCapacity(int capacity);
  }

  /**
   * A {@link java.util.List} implementation that avoids boxing the elements into Doubles if
   * possible. Does not support null elements.
   */
  public static interface DoubleList extends ProtobufList<Double> {

    /**
     * Like {@link #get(int)} but more efficient in that it doesn't box the returned value.
     */
    double getDouble(int index);

    /**
     * Like {@link #add(Double)} but more efficient in that it doesn't box the element.
     */
    void addDouble(double element);

    /**
     * Like {@link #set(int, Double)} but more efficient in that it doesn't box the element.
     */
    double setDouble(int index, double element);

    /**
     * Returns a mutable clone of this list with the specified capacity.
     */
    @Override
    DoubleList mutableCopyWithCapacity(int capacity);
  }

  /**
   * A {@link java.util.List} implementation that avoids boxing the elements into Floats if
   * possible. Does not support null elements.
   */
  public static interface FloatList extends ProtobufList<Float> {

    /**
     * Like {@link #get(int)} but more efficient in that it doesn't box the returned value.
     */
    float getFloat(int index);

    /**
     * Like {@link #add(Float)} but more efficient in that it doesn't box the element.
     */
    void addFloat(float element);

    /**
     * Like {@link #set(int, Float)} but more efficient in that it doesn't box the element.
     */
    float setFloat(int index, float element);

    /**
     * Returns a mutable clone of this list with the specified capacity.
     */
    @Override
    FloatList mutableCopyWithCapacity(int capacity);
  }
}
