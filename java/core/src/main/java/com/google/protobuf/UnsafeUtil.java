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

import java.lang.reflect.Field;
import java.nio.Buffer;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.security.AccessController;
import java.security.PrivilegedExceptionAction;
import java.util.logging.Level;
import java.util.logging.Logger;

/** Utility class for working with unsafe operations. */
final class UnsafeUtil {
  private static final sun.misc.Unsafe UNSAFE = getUnsafe();
  private static final Class<?> MEMORY_CLASS = Android.getMemoryClass();
  private static final boolean IS_ANDROID_64 = determineAndroidSupportByAddressSize(long.class);
  private static final boolean IS_ANDROID_32 = determineAndroidSupportByAddressSize(int.class);
  private static final MemoryAccessor MEMORY_ACCESSOR = getMemoryAccessor();
  private static final boolean HAS_UNSAFE_BYTEBUFFER_OPERATIONS =
      supportsUnsafeByteBufferOperations();
  private static final boolean HAS_UNSAFE_ARRAY_OPERATIONS = supportsUnsafeArrayOperations();

  static final long BYTE_ARRAY_BASE_OFFSET = arrayBaseOffset(byte[].class);
  // Micro-optimization: we can assume a scale of 1 and skip the multiply
  // private static final long BYTE_ARRAY_INDEX_SCALE = 1;

  private static final long BOOLEAN_ARRAY_BASE_OFFSET = arrayBaseOffset(boolean[].class);
  private static final long BOOLEAN_ARRAY_INDEX_SCALE = arrayIndexScale(boolean[].class);

  private static final long INT_ARRAY_BASE_OFFSET = arrayBaseOffset(int[].class);
  private static final long INT_ARRAY_INDEX_SCALE = arrayIndexScale(int[].class);

  private static final long LONG_ARRAY_BASE_OFFSET = arrayBaseOffset(long[].class);
  private static final long LONG_ARRAY_INDEX_SCALE = arrayIndexScale(long[].class);

  private static final long FLOAT_ARRAY_BASE_OFFSET = arrayBaseOffset(float[].class);
  private static final long FLOAT_ARRAY_INDEX_SCALE = arrayIndexScale(float[].class);

  private static final long DOUBLE_ARRAY_BASE_OFFSET = arrayBaseOffset(double[].class);
  private static final long DOUBLE_ARRAY_INDEX_SCALE = arrayIndexScale(double[].class);

  private static final long OBJECT_ARRAY_BASE_OFFSET = arrayBaseOffset(Object[].class);
  private static final long OBJECT_ARRAY_INDEX_SCALE = arrayIndexScale(Object[].class);

  private static final long BUFFER_ADDRESS_OFFSET = fieldOffset(bufferAddressField());

  private static final int STRIDE = 8;
  private static final int STRIDE_ALIGNMENT_MASK = STRIDE - 1;
  private static final int BYTE_ARRAY_ALIGNMENT =
      (int) (BYTE_ARRAY_BASE_OFFSET & STRIDE_ALIGNMENT_MASK);

  static final boolean IS_BIG_ENDIAN = ByteOrder.nativeOrder() == ByteOrder.BIG_ENDIAN;

  private UnsafeUtil() {}

  static boolean hasUnsafeArrayOperations() {
    return HAS_UNSAFE_ARRAY_OPERATIONS;
  }

  static boolean hasUnsafeByteBufferOperations() {
    return HAS_UNSAFE_BYTEBUFFER_OPERATIONS;
  }

  static boolean isAndroid64() {
    return IS_ANDROID_64;
  }

  @SuppressWarnings("unchecked") // safe by method contract
  static <T> T allocateInstance(Class<T> clazz) {
    try {
      return (T) UNSAFE.allocateInstance(clazz);
    } catch (InstantiationException e) {
      throw new IllegalStateException(e);
    }
  }

  static long objectFieldOffset(Field field) {
    return MEMORY_ACCESSOR.objectFieldOffset(field);
  }

  private static int arrayBaseOffset(Class<?> clazz) {
    return HAS_UNSAFE_ARRAY_OPERATIONS ? MEMORY_ACCESSOR.arrayBaseOffset(clazz) : -1;
  }

  private static int arrayIndexScale(Class<?> clazz) {
    return HAS_UNSAFE_ARRAY_OPERATIONS ? MEMORY_ACCESSOR.arrayIndexScale(clazz) : -1;
  }

  static byte getByte(Object target, long offset) {
    return MEMORY_ACCESSOR.getByte(target, offset);
  }

  static void putByte(Object target, long offset, byte value) {
    MEMORY_ACCESSOR.putByte(target, offset, value);
  }

  static int getInt(Object target, long offset) {
    return MEMORY_ACCESSOR.getInt(target, offset);
  }

  static void putInt(Object target, long offset, int value) {
    MEMORY_ACCESSOR.putInt(target, offset, value);
  }

  static long getLong(Object target, long offset) {
    return MEMORY_ACCESSOR.getLong(target, offset);
  }

  static void putLong(Object target, long offset, long value) {
    MEMORY_ACCESSOR.putLong(target, offset, value);
  }

  static boolean getBoolean(Object target, long offset) {
    return MEMORY_ACCESSOR.getBoolean(target, offset);
  }

  static void putBoolean(Object target, long offset, boolean value) {
    MEMORY_ACCESSOR.putBoolean(target, offset, value);
  }

  static float getFloat(Object target, long offset) {
    return MEMORY_ACCESSOR.getFloat(target, offset);
  }

  static void putFloat(Object target, long offset, float value) {
    MEMORY_ACCESSOR.putFloat(target, offset, value);
  }

  static double getDouble(Object target, long offset) {
    return MEMORY_ACCESSOR.getDouble(target, offset);
  }

  static void putDouble(Object target, long offset, double value) {
    MEMORY_ACCESSOR.putDouble(target, offset, value);
  }

  static Object getObject(Object target, long offset) {
    return MEMORY_ACCESSOR.getObject(target, offset);
  }

  static void putObject(Object target, long offset, Object value) {
    MEMORY_ACCESSOR.putObject(target, offset, value);
  }

  static byte getByte(byte[] target, long index) {
    return MEMORY_ACCESSOR.getByte(target, BYTE_ARRAY_BASE_OFFSET + index);
  }

  static void putByte(byte[] target, long index, byte value) {
    MEMORY_ACCESSOR.putByte(target, BYTE_ARRAY_BASE_OFFSET + index, value);
  }

  static int getInt(int[] target, long index) {
    return MEMORY_ACCESSOR.getInt(target, INT_ARRAY_BASE_OFFSET + (index * INT_ARRAY_INDEX_SCALE));
  }

  static void putInt(int[] target, long index, int value) {
    MEMORY_ACCESSOR.putInt(target, INT_ARRAY_BASE_OFFSET + (index * INT_ARRAY_INDEX_SCALE), value);
  }

  static long getLong(long[] target, long index) {
    return MEMORY_ACCESSOR.getLong(
        target, LONG_ARRAY_BASE_OFFSET + (index * LONG_ARRAY_INDEX_SCALE));
  }

  static void putLong(long[] target, long index, long value) {
    MEMORY_ACCESSOR.putLong(
        target, LONG_ARRAY_BASE_OFFSET + (index * LONG_ARRAY_INDEX_SCALE), value);
  }

  static boolean getBoolean(boolean[] target, long index) {
    return MEMORY_ACCESSOR.getBoolean(
        target, BOOLEAN_ARRAY_BASE_OFFSET + (index * BOOLEAN_ARRAY_INDEX_SCALE));
  }

  static void putBoolean(boolean[] target, long index, boolean value) {
    MEMORY_ACCESSOR.putBoolean(
        target, BOOLEAN_ARRAY_BASE_OFFSET + (index * BOOLEAN_ARRAY_INDEX_SCALE), value);
  }

  static float getFloat(float[] target, long index) {
    return MEMORY_ACCESSOR.getFloat(
        target, FLOAT_ARRAY_BASE_OFFSET + (index * FLOAT_ARRAY_INDEX_SCALE));
  }

  static void putFloat(float[] target, long index, float value) {
    MEMORY_ACCESSOR.putFloat(
        target, FLOAT_ARRAY_BASE_OFFSET + (index * FLOAT_ARRAY_INDEX_SCALE), value);
  }

  static double getDouble(double[] target, long index) {
    return MEMORY_ACCESSOR.getDouble(
        target, DOUBLE_ARRAY_BASE_OFFSET + (index * DOUBLE_ARRAY_INDEX_SCALE));
  }

  static void putDouble(double[] target, long index, double value) {
    MEMORY_ACCESSOR.putDouble(
        target, DOUBLE_ARRAY_BASE_OFFSET + (index * DOUBLE_ARRAY_INDEX_SCALE), value);
  }

  static Object getObject(Object[] target, long index) {
    return MEMORY_ACCESSOR.getObject(
        target, OBJECT_ARRAY_BASE_OFFSET + (index * OBJECT_ARRAY_INDEX_SCALE));
  }

  static void putObject(Object[] target, long index, Object value) {
    MEMORY_ACCESSOR.putObject(
        target, OBJECT_ARRAY_BASE_OFFSET + (index * OBJECT_ARRAY_INDEX_SCALE), value);
  }

  static void copyMemory(byte[] src, long srcIndex, long targetOffset, long length) {
    MEMORY_ACCESSOR.copyMemory(src, srcIndex, targetOffset, length);
  }

  static void copyMemory(long srcOffset, byte[] target, long targetIndex, long length) {
    MEMORY_ACCESSOR.copyMemory(srcOffset, target, targetIndex, length);
  }

  static void copyMemory(byte[] src, long srcIndex, byte[] target, long targetIndex, long length) {
    System.arraycopy(src, (int) srcIndex, target, (int) targetIndex, (int) length);
  }

  static byte getByte(long address) {
    return MEMORY_ACCESSOR.getByte(address);
  }

  static void putByte(long address, byte value) {
    MEMORY_ACCESSOR.putByte(address, value);
  }

  static int getInt(long address) {
    return MEMORY_ACCESSOR.getInt(address);
  }

  static void putInt(long address, int value) {
    MEMORY_ACCESSOR.putInt(address, value);
  }

  static long getLong(long address) {
    return MEMORY_ACCESSOR.getLong(address);
  }

  static void putLong(long address, long value) {
    MEMORY_ACCESSOR.putLong(address, value);
  }

  /** Gets the offset of the {@code address} field of the given direct {@link ByteBuffer}. */
  static long addressOffset(ByteBuffer buffer) {
    return MEMORY_ACCESSOR.getLong(buffer, BUFFER_ADDRESS_OFFSET);
  }

  static Object getStaticObject(Field field) {
    return MEMORY_ACCESSOR.getStaticObject(field);
  }

  /**
   * Gets the {@code sun.misc.Unsafe} instance, or {@code null} if not available on this platform.
   */
  static sun.misc.Unsafe getUnsafe() {
    sun.misc.Unsafe unsafe = null;
    try {
      unsafe =
          AccessController.doPrivileged(
              new PrivilegedExceptionAction<sun.misc.Unsafe>() {
                @Override
                public sun.misc.Unsafe run() throws Exception {
                  Class<sun.misc.Unsafe> k = sun.misc.Unsafe.class;

                  for (Field f : k.getDeclaredFields()) {
                    f.setAccessible(true);
                    Object x = f.get(null);
                    if (k.isInstance(x)) {
                      return k.cast(x);
                    }
                  }
                  // The sun.misc.Unsafe field does not exist.
                  return null;
                }
              });
    } catch (Throwable e) {
      // Catching Throwable here due to the fact that Google AppEngine raises NoClassDefFoundError
      // for Unsafe.
    }
    return unsafe;
  }

  /** Get a {@link MemoryAccessor} appropriate for the platform, or null if not supported. */
  private static MemoryAccessor getMemoryAccessor() {
    if (UNSAFE == null) {
      return null;
    }
    if (Android.isOnAndroidDevice()) {
      if (IS_ANDROID_64) {
        return new Android64MemoryAccessor(UNSAFE);
      } else if (IS_ANDROID_32) {
        return new Android32MemoryAccessor(UNSAFE);
      } else {
        return null;
      }
    }

    return new JvmMemoryAccessor(UNSAFE);
  }

  private static boolean supportsUnsafeArrayOperations() {
    if (MEMORY_ACCESSOR == null) {
      return false;
    }
    return MEMORY_ACCESSOR.supportsUnsafeArrayOperations();
  }

  private static boolean supportsUnsafeByteBufferOperations() {
    if (MEMORY_ACCESSOR == null) {
      return false;
    }
    return MEMORY_ACCESSOR.supportsUnsafeByteBufferOperations();
  }

  private static boolean determineAndroidSupportByAddressSize(Class<?> addressClass) {
    if (!Android.isOnAndroidDevice()) {
      return false;
    }
    try {
      Class<?> clazz = MEMORY_CLASS;
      clazz.getMethod("peekLong", addressClass, boolean.class);
      clazz.getMethod("pokeLong", addressClass, long.class, boolean.class);
      clazz.getMethod("pokeInt", addressClass, int.class, boolean.class);
      clazz.getMethod("peekInt", addressClass, boolean.class);
      clazz.getMethod("pokeByte", addressClass, byte.class);
      clazz.getMethod("peekByte", addressClass);
      clazz.getMethod("pokeByteArray", addressClass, byte[].class, int.class, int.class);
      clazz.getMethod("peekByteArray", addressClass, byte[].class, int.class, int.class);
      return true;
    } catch (Throwable t) {
      return false;
    }
  }

  /** Finds the address field within a direct {@link Buffer}. */
  private static Field bufferAddressField() {
    if (Android.isOnAndroidDevice()) {
      // Old versions of Android had renamed the address field to 'effectiveDirectAddress', but
      // recent versions of Android (>M?) use the OpenJDK implementation. Fall through in that case.
      Field field = field(Buffer.class, "effectiveDirectAddress");
      if (field != null) {
        return field;
      }
    }
    Field field = field(Buffer.class, "address");
    return field != null && field.getType() == long.class ? field : null;
  }

  /**
   * Returns the index of the first byte where left and right differ, in the range [0, 8]. If {@code
   * left == right}, the result will be 8, otherwise less than 8.
   *
   * <p>This counts from the *first* byte, which may be the most or least significant byte depending
   * on the system endianness.
   */
  private static int firstDifferingByteIndexNativeEndian(long left, long right) {
    int n =
        IS_BIG_ENDIAN
            ? Long.numberOfLeadingZeros(left ^ right)
            : Long.numberOfTrailingZeros(left ^ right);
    return n >> 3;
  }

  /**
   * Returns the lowest {@code index} such that {@code 0 <= index < length} and {@code left[leftOff
   * + index] != right[rightOff + index]}. If no such value exists -- if {@code left} and {@code
   * right} match up to {@code length} bytes from their respective offsets -- returns -1.
   *
   * <p>{@code leftOff + length} must be less than or equal to {@code left.length}, and the same for
   * {@code right}.
   */
  static int mismatch(byte[] left, int leftOff, byte[] right, int rightOff, int length) {
    if (leftOff < 0
        || rightOff < 0
        || length < 0
        || leftOff + length > left.length
        || rightOff + length > right.length) {
      throw new IndexOutOfBoundsException();
    }

    int index = 0;
    if (HAS_UNSAFE_ARRAY_OPERATIONS) {
      int leftAlignment = (BYTE_ARRAY_ALIGNMENT + leftOff) & STRIDE_ALIGNMENT_MASK;

      // Most CPUs handle getting chunks of bytes better on addresses that are a multiple of 4
      // or 8.
      // We walk one byte at a time until the left address, at least, is a multiple of 8.
      // If the right address is, too, so much the better.
      for (;
          index < length && (leftAlignment & STRIDE_ALIGNMENT_MASK) != 0;
          index++, leftAlignment++) {
        if (left[leftOff + index] != right[rightOff + index]) {
          return index;
        }
      }

      // Stride!  Grab eight bytes at a time from left and right and check them for equality.

      int strideLength = ((length - index) & ~STRIDE_ALIGNMENT_MASK) + index;
      // strideLength is the point where we want to stop striding: it differs from index by
      // a multiple of STRIDE, and it's the largest such number <= length.

      for (; index < strideLength; index += STRIDE) {
        long leftLongWord = getLong(left, BYTE_ARRAY_BASE_OFFSET + leftOff + index);
        long rightLongWord = getLong(right, BYTE_ARRAY_BASE_OFFSET + rightOff + index);
        if (leftLongWord != rightLongWord) {
          // one of these eight bytes differ!  use a helper to find out which one
          return index + firstDifferingByteIndexNativeEndian(leftLongWord, rightLongWord);
        }
      }
    }

    // If we were able to stride, there are at most STRIDE - 1 bytes left to compare.
    // If we weren't, then this loop covers the whole thing.
    for (; index < length; index++) {
      if (left[leftOff + index] != right[rightOff + index]) {
        return index;
      }
    }
    return -1;
  }

  /**
   * Returns the offset of the provided field, or {@code -1} if {@code sun.misc.Unsafe} is not
   * available.
   */
  private static long fieldOffset(Field field) {
    return field == null || MEMORY_ACCESSOR == null ? -1 : MEMORY_ACCESSOR.objectFieldOffset(field);
  }

  /**
   * Gets the field with the given name within the class, or {@code null} if not found.
   */
  private static Field field(Class<?> clazz, String fieldName) {
    Field field;
    try {
      field = clazz.getDeclaredField(fieldName);
    } catch (Throwable t) {
      // Failed to access the fields.
      field = null;
    }
    return field;
  }

  private abstract static class MemoryAccessor {

    sun.misc.Unsafe unsafe;

    MemoryAccessor(sun.misc.Unsafe unsafe) {
      this.unsafe = unsafe;
    }

    public final long objectFieldOffset(Field field) {
      return unsafe.objectFieldOffset(field);
    }

    public final int arrayBaseOffset(Class<?> clazz) {
      return unsafe.arrayBaseOffset(clazz);
    }

    public final int arrayIndexScale(Class<?> clazz) {
      return unsafe.arrayIndexScale(clazz);
    }

    public abstract Object getStaticObject(Field field);

    // Relative Address Operations ---------------------------------------------

    // Indicates whether the following relative address operations are supported
    // by this memory accessor.
    public boolean supportsUnsafeArrayOperations() {
      if (unsafe == null) {
        return false;
      }
      try {
        Class<?> clazz = unsafe.getClass();
        clazz.getMethod("objectFieldOffset", Field.class);
        clazz.getMethod("arrayBaseOffset", Class.class);
        clazz.getMethod("arrayIndexScale", Class.class);
        clazz.getMethod("getInt", Object.class, long.class);
        clazz.getMethod("putInt", Object.class, long.class, int.class);
        clazz.getMethod("getLong", Object.class, long.class);
        clazz.getMethod("putLong", Object.class, long.class, long.class);
        clazz.getMethod("getObject", Object.class, long.class);
        clazz.getMethod("putObject", Object.class, long.class, Object.class);

        return true;
      } catch (Throwable e) {
        logMissingMethod(e);
      }
      return false;
    }

    public abstract byte getByte(Object target, long offset);

    public abstract void putByte(Object target, long offset, byte value);

    public final int getInt(Object target, long offset) {
      return unsafe.getInt(target, offset);
    }

    public final void putInt(Object target, long offset, int value) {
      unsafe.putInt(target, offset, value);
    }

    public final long getLong(Object target, long offset) {
      return unsafe.getLong(target, offset);
    }

    public final void putLong(Object target, long offset, long value) {
      unsafe.putLong(target, offset, value);
    }

    public abstract boolean getBoolean(Object target, long offset);

    public abstract void putBoolean(Object target, long offset, boolean value);

    public abstract float getFloat(Object target, long offset);

    public abstract void putFloat(Object target, long offset, float value);

    public abstract double getDouble(Object target, long offset);

    public abstract void putDouble(Object target, long offset, double value);

    public final Object getObject(Object target, long offset) {
      return unsafe.getObject(target, offset);
    }

    public final void putObject(Object target, long offset, Object value) {
      unsafe.putObject(target, offset, value);
    }

    // Absolute Address Operations --------------------------------------------

    // Indicates whether the following absolute address operations are
    // supported by this memory accessor.
    public boolean supportsUnsafeByteBufferOperations() {
      if (unsafe == null) {
        return false;
      }
      try {
        Class<?> clazz = unsafe.getClass();
        // Methods for getting direct buffer address.
        clazz.getMethod("objectFieldOffset", Field.class);
        clazz.getMethod("getLong", Object.class, long.class);

        if (bufferAddressField() == null) {
          return false;
        }

        return true;
      } catch (Throwable e) {
        logMissingMethod(e);
      }
      return false;
    }

    public abstract byte getByte(long address);

    public abstract void putByte(long address, byte value);

    public abstract int getInt(long address);

    public abstract void putInt(long address, int value);

    public abstract long getLong(long address);

    public abstract void putLong(long address, long value);

    public abstract void copyMemory(long srcOffset, byte[] target, long targetIndex, long length);

    public abstract void copyMemory(byte[] src, long srcIndex, long targetOffset, long length);
  }

  private static final class JvmMemoryAccessor extends MemoryAccessor {

    JvmMemoryAccessor(sun.misc.Unsafe unsafe) {
      super(unsafe);
    }

    @Override
    public Object getStaticObject(Field field) {
      return getObject(unsafe.staticFieldBase(field), unsafe.staticFieldOffset(field));
    }

    @Override
    public boolean supportsUnsafeArrayOperations() {
      if (!super.supportsUnsafeArrayOperations()) {
        return false;
      }

      try {
        Class<?> clazz = unsafe.getClass();
        clazz.getMethod("getByte", Object.class, long.class);
        clazz.getMethod("putByte", Object.class, long.class, byte.class);
        clazz.getMethod("getBoolean", Object.class, long.class);
        clazz.getMethod("putBoolean", Object.class, long.class, boolean.class);
        clazz.getMethod("getFloat", Object.class, long.class);
        clazz.getMethod("putFloat", Object.class, long.class, float.class);
        clazz.getMethod("getDouble", Object.class, long.class);
        clazz.getMethod("putDouble", Object.class, long.class, double.class);

        return true;
      } catch (Throwable e) {
        logMissingMethod(e);
      }
      return false;
    }

    @Override
    public byte getByte(Object target, long offset) {
      return unsafe.getByte(target, offset);
    }

    @Override
    public void putByte(Object target, long offset, byte value) {
      unsafe.putByte(target, offset, value);
    }

    @Override
    public boolean getBoolean(Object target, long offset) {
      return unsafe.getBoolean(target, offset);
    }

    @Override
    public void putBoolean(Object target, long offset, boolean value) {
      unsafe.putBoolean(target, offset, value);
    }

    @Override
    public float getFloat(Object target, long offset) {
      return unsafe.getFloat(target, offset);
    }

    @Override
    public void putFloat(Object target, long offset, float value) {
      unsafe.putFloat(target, offset, value);
    }

    @Override
    public double getDouble(Object target, long offset) {
      return unsafe.getDouble(target, offset);
    }

    @Override
    public void putDouble(Object target, long offset, double value) {
      unsafe.putDouble(target, offset, value);
    }

    @Override
    public boolean supportsUnsafeByteBufferOperations() {
      if (!super.supportsUnsafeByteBufferOperations()) {
        return false;
      }

      try {
        Class<?> clazz = unsafe.getClass();
        clazz.getMethod("getByte", long.class);
        clazz.getMethod("putByte", long.class, byte.class);
        clazz.getMethod("getInt", long.class);
        clazz.getMethod("putInt", long.class, int.class);
        clazz.getMethod("getLong", long.class);
        clazz.getMethod("putLong", long.class, long.class);
        clazz.getMethod("copyMemory", long.class, long.class, long.class);
        clazz.getMethod(
            "copyMemory", Object.class, long.class, Object.class, long.class, long.class);
        return true;
      } catch (Throwable e) {
        logMissingMethod(e);
      }
      return false;
    }

    @Override
    public byte getByte(long address) {
      return unsafe.getByte(address);
    }

    @Override
    public void putByte(long address, byte value) {
      unsafe.putByte(address, value);
    }

    @Override
    public int getInt(long address) {
      return unsafe.getInt(address);
    }

    @Override
    public void putInt(long address, int value) {
      unsafe.putInt(address, value);
    }

    @Override
    public long getLong(long address) {
      return unsafe.getLong(address);
    }

    @Override
    public void putLong(long address, long value) {
      unsafe.putLong(address, value);
    }

    @Override
    public void copyMemory(long srcOffset, byte[] target, long targetIndex, long length) {
      unsafe.copyMemory(null, srcOffset, target, BYTE_ARRAY_BASE_OFFSET + targetIndex, length);
    }

    @Override
    public void copyMemory(byte[] src, long srcIndex, long targetOffset, long length) {
      unsafe.copyMemory(src, BYTE_ARRAY_BASE_OFFSET + srcIndex, null, targetOffset, length);
    }
  }

  private static final class Android64MemoryAccessor extends MemoryAccessor {

    Android64MemoryAccessor(sun.misc.Unsafe unsafe) {
      super(unsafe);
    }

    @Override
    public Object getStaticObject(Field field) {
      try {
        return field.get(null);
      } catch (IllegalAccessException e) {
        return null;
      }
    }

    @Override
    public byte getByte(Object target, long offset) {
      if (IS_BIG_ENDIAN) {
        return getByteBigEndian(target, offset);
      } else {
        return getByteLittleEndian(target, offset);
      }
    }

    @Override
    public void putByte(Object target, long offset, byte value) {
      if (IS_BIG_ENDIAN) {
        putByteBigEndian(target, offset, value);
      } else {
        putByteLittleEndian(target, offset, value);
      }
    }

    @Override
    public boolean getBoolean(Object target, long offset) {
      if (IS_BIG_ENDIAN) {
        return getBooleanBigEndian(target, offset);
      } else {
        return getBooleanLittleEndian(target, offset);
      }
    }

    @Override
    public void putBoolean(Object target, long offset, boolean value) {
      if (IS_BIG_ENDIAN) {
        putBooleanBigEndian(target, offset, value);
      } else {
        putBooleanLittleEndian(target, offset, value);
      }
    }

    @Override
    public float getFloat(Object target, long offset) {
      return Float.intBitsToFloat(getInt(target, offset));
    }

    @Override
    public void putFloat(Object target, long offset, float value) {
      putInt(target, offset, Float.floatToIntBits(value));
    }

    @Override
    public double getDouble(Object target, long offset) {
      return Double.longBitsToDouble(getLong(target, offset));
    }

    @Override
    public void putDouble(Object target, long offset, double value) {
      putLong(target, offset, Double.doubleToLongBits(value));
    }

    @Override
    public boolean supportsUnsafeByteBufferOperations() {
      return false;
    }

    @Override
    public byte getByte(long address) {
      throw new UnsupportedOperationException();
    }

    @Override
    public void putByte(long address, byte value) {
      throw new UnsupportedOperationException();
    }

    @Override
    public int getInt(long address) {
      throw new UnsupportedOperationException();
    }

    @Override
    public void putInt(long address, int value) {
      throw new UnsupportedOperationException();
    }

    @Override
    public long getLong(long address) {
      throw new UnsupportedOperationException();
    }

    @Override
    public void putLong(long address, long value) {
      throw new UnsupportedOperationException();
    }

    @Override
    public void copyMemory(long srcOffset, byte[] target, long targetIndex, long length) {
      throw new UnsupportedOperationException();
    }

    @Override
    public void copyMemory(byte[] src, long srcIndex, long targetOffset, long length) {
      throw new UnsupportedOperationException();
    }
  }

  private static final class Android32MemoryAccessor extends MemoryAccessor {

    /** Mask used to convert a 64 bit memory address to a 32 bit address. */
    private static final long SMALL_ADDRESS_MASK = 0x00000000FFFFFFFF;

    /** Truncate a {@code long} address into a short {@code int} address. */
    private static int smallAddress(long address) {
      return (int) (SMALL_ADDRESS_MASK & address);
    }

    Android32MemoryAccessor(sun.misc.Unsafe unsafe) {
      super(unsafe);
    }

    @Override
    public Object getStaticObject(Field field) {
      try {
        return field.get(null);
      } catch (IllegalAccessException e) {
        return null;
      }
    }

    @Override
    public byte getByte(Object target, long offset) {
      if (IS_BIG_ENDIAN) {
        return getByteBigEndian(target, offset);
      } else {
        return getByteLittleEndian(target, offset);
      }
    }

    @Override
    public void putByte(Object target, long offset, byte value) {
      if (IS_BIG_ENDIAN) {
        putByteBigEndian(target, offset, value);
      } else {
        putByteLittleEndian(target, offset, value);
      }
    }

    @Override
    public boolean getBoolean(Object target, long offset) {
      if (IS_BIG_ENDIAN) {
        return getBooleanBigEndian(target, offset);
      } else {
        return getBooleanLittleEndian(target, offset);
      }
    }

    @Override
    public void putBoolean(Object target, long offset, boolean value) {
      if (IS_BIG_ENDIAN) {
        putBooleanBigEndian(target, offset, value);
      } else {
        putBooleanLittleEndian(target, offset, value);
      }
    }

    @Override
    public float getFloat(Object target, long offset) {
      return Float.intBitsToFloat(getInt(target, offset));
    }

    @Override
    public void putFloat(Object target, long offset, float value) {
      putInt(target, offset, Float.floatToIntBits(value));
    }

    @Override
    public double getDouble(Object target, long offset) {
      return Double.longBitsToDouble(getLong(target, offset));
    }

    @Override
    public void putDouble(Object target, long offset, double value) {
      putLong(target, offset, Double.doubleToLongBits(value));
    }

    @Override
    public boolean supportsUnsafeByteBufferOperations() {
      return false;
    }

    @Override
    public byte getByte(long address) {
      throw new UnsupportedOperationException();
    }

    @Override
    public void putByte(long address, byte value) {
      throw new UnsupportedOperationException();
    }

    @Override
    public int getInt(long address) {
      throw new UnsupportedOperationException();
    }

    @Override
    public void putInt(long address, int value) {
      throw new UnsupportedOperationException();
    }

    @Override
    public long getLong(long address) {
      throw new UnsupportedOperationException();
    }

    @Override
    public void putLong(long address, long value) {
      throw new UnsupportedOperationException();
    }

    @Override
    public void copyMemory(long srcOffset, byte[] target, long targetIndex, long length) {
      throw new UnsupportedOperationException();
    }

    @Override
    public void copyMemory(byte[] src, long srcIndex, long targetOffset, long length) {
      throw new UnsupportedOperationException();
    }
  }

  private static byte getByteBigEndian(Object target, long offset) {
    return (byte) ((getInt(target, offset & ~3) >>> ((~offset & 3) << 3)) & 0xFF);
  }

  private static byte getByteLittleEndian(Object target, long offset) {
    return (byte) ((getInt(target, offset & ~3) >>> ((offset & 3) << 3)) & 0xFF);
  }

  private static void putByteBigEndian(Object target, long offset, byte value) {
    int intValue = getInt(target, offset & ~3);
    int shift = ((~(int) offset) & 3) << 3;
    int output = (intValue & ~(0xFF << shift)) | ((0xFF & value) << shift);
    putInt(target, offset & ~3, output);
  }

  private static void putByteLittleEndian(Object target, long offset, byte value) {
    int intValue = getInt(target, offset & ~3);
    int shift = (((int) offset) & 3) << 3;
    int output = (intValue & ~(0xFF << shift)) | ((0xFF & value) << shift);
    putInt(target, offset & ~3, output);
  }

  private static boolean getBooleanBigEndian(Object target, long offset) {
    return getByteBigEndian(target, offset) != 0;
  }

  private static boolean getBooleanLittleEndian(Object target, long offset) {
    return getByteLittleEndian(target, offset) != 0;
  }

  private static void putBooleanBigEndian(Object target, long offset, boolean value) {
    putByteBigEndian(target, offset, (byte) (value ? 1 : 0));
  }

  private static void putBooleanLittleEndian(Object target, long offset, boolean value) {
    putByteLittleEndian(target, offset, (byte) (value ? 1 : 0));
  }

  private static void logMissingMethod(Throwable e) {
    Logger.getLogger(UnsafeUtil.class.getName())
        .log(
            Level.WARNING,
            "platform method missing - proto runtime falling back to safer methods: " + e);
  }
}
