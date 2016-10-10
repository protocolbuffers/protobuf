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
import java.security.AccessController;
import java.security.PrivilegedExceptionAction;
import sun.misc.Unsafe;

/** Utility class for working with unsafe operations. */
// TODO(nathanmittler): Add support for Android Memory/MemoryBlock
final class UnsafeUtil {
  private static final sun.misc.Unsafe UNSAFE = getUnsafe();
  private static final boolean HAS_UNSAFE_BYTEBUFFER_OPERATIONS =
      supportsUnsafeByteBufferOperations();
  private static final boolean HAS_UNSAFE_ARRAY_OPERATIONS = supportsUnsafeArrayOperations();
  private static final long ARRAY_BASE_OFFSET = byteArrayBaseOffset();
  private static final long BUFFER_ADDRESS_OFFSET = fieldOffset(field(Buffer.class, "address"));

  private UnsafeUtil() {}

  static boolean hasUnsafeArrayOperations() {
    return HAS_UNSAFE_ARRAY_OPERATIONS;
  }

  static boolean hasUnsafeByteBufferOperations() {
    return HAS_UNSAFE_BYTEBUFFER_OPERATIONS;
  }

  static Object allocateInstance(Class<?> clazz) {
    try {
      return UNSAFE.allocateInstance(clazz);
    } catch (InstantiationException e) {
      throw new RuntimeException(e);
    }
  }

  static long objectFieldOffset(Field field) {
    return UNSAFE.objectFieldOffset(field);
  }

  static long getArrayBaseOffset() {
    return ARRAY_BASE_OFFSET;
  }

  static byte getByte(Object target, long offset) {
    return UNSAFE.getByte(target, offset);
  }

  static void putByte(Object target, long offset, byte value) {
    UNSAFE.putByte(target, offset, value);
  }

  static int getInt(Object target, long offset) {
    return UNSAFE.getInt(target, offset);
  }

  static void putInt(Object target, long offset, int value) {
    UNSAFE.putInt(target, offset, value);
  }

  static long getLong(Object target, long offset) {
    return UNSAFE.getLong(target, offset);
  }

  static void putLong(Object target, long offset, long value) {
    UNSAFE.putLong(target, offset, value);
  }

  static boolean getBoolean(Object target, long offset) {
    return UNSAFE.getBoolean(target, offset);
  }

  static void putBoolean(Object target, long offset, boolean value) {
    UNSAFE.putBoolean(target, offset, value);
  }

  static float getFloat(Object target, long offset) {
    return UNSAFE.getFloat(target, offset);
  }

  static void putFloat(Object target, long offset, float value) {
    UNSAFE.putFloat(target, offset, value);
  }

  static double getDouble(Object target, long offset) {
    return UNSAFE.getDouble(target, offset);
  }

  static void putDouble(Object target, long offset, double value) {
    UNSAFE.putDouble(target, offset, value);
  }

  static Object getObject(Object target, long offset) {
    return UNSAFE.getObject(target, offset);
  }

  static void putObject(Object target, long offset, Object value) {
    UNSAFE.putObject(target, offset, value);
  }

  static void copyMemory(
      Object src, long srcOffset, Object target, long targetOffset, long length) {
    UNSAFE.copyMemory(src, srcOffset, target, targetOffset, length);
  }

  static byte getByte(long address) {
    return UNSAFE.getByte(address);
  }

  static void putByte(long address, byte value) {
    UNSAFE.putByte(address, value);
  }

  static int getInt(long address) {
    return UNSAFE.getInt(address);
  }

  static void putInt(long address, int value) {
    UNSAFE.putInt(address, value);
  }

  static long getLong(long address) {
    return UNSAFE.getLong(address);
  }

  static void putLong(long address, long value) {
    UNSAFE.putLong(address, value);
  }

  static void copyMemory(long srcAddress, long targetAddress, long length) {
    UNSAFE.copyMemory(srcAddress, targetAddress, length);
  }

  static void setMemory(long address, long numBytes, byte value) {
    UNSAFE.setMemory(address, numBytes, value);
  }

  /**
   * Gets the offset of the {@code address} field of the given direct {@link ByteBuffer}.
   */
  static long addressOffset(ByteBuffer buffer) {
    return UNSAFE.getLong(buffer, BUFFER_ADDRESS_OFFSET);
  }

  /**
   * Gets the {@code sun.misc.Unsafe} instance, or {@code null} if not available on this platform.
   */
  private static sun.misc.Unsafe getUnsafe() {
    sun.misc.Unsafe unsafe = null;
    try {
      unsafe =
          AccessController.doPrivileged(
              new PrivilegedExceptionAction<Unsafe>() {
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

  /** Indicates whether or not unsafe array operations are supported on this platform. */
  private static boolean supportsUnsafeArrayOperations() {
    boolean supported = false;
    if (UNSAFE != null) {
      try {
        Class<?> clazz = UNSAFE.getClass();
        clazz.getMethod("objectFieldOffset", Field.class);
        clazz.getMethod("allocateInstance", Class.class);
        clazz.getMethod("arrayBaseOffset", Class.class);
        clazz.getMethod("getByte", Object.class, long.class);
        clazz.getMethod("putByte", Object.class, long.class, byte.class);
        clazz.getMethod("getBoolean", Object.class, long.class);
        clazz.getMethod("putBoolean", Object.class, long.class, boolean.class);
        clazz.getMethod("getInt", Object.class, long.class);
        clazz.getMethod("putInt", Object.class, long.class, int.class);
        clazz.getMethod("getLong", Object.class, long.class);
        clazz.getMethod("putLong", Object.class, long.class, long.class);
        clazz.getMethod("getFloat", Object.class, long.class);
        clazz.getMethod("putFloat", Object.class, long.class, float.class);
        clazz.getMethod("getDouble", Object.class, long.class);
        clazz.getMethod("putDouble", Object.class, long.class, double.class);
        clazz.getMethod("getObject", Object.class, long.class);
        clazz.getMethod("putObject", Object.class, long.class, Object.class);
        clazz.getMethod(
            "copyMemory", Object.class, long.class, Object.class, long.class, long.class);
        supported = true;
      } catch (Throwable e) {
        // Do nothing.
      }
    }
    return supported;
  }

  private static boolean supportsUnsafeByteBufferOperations() {
    boolean supported = false;
    if (UNSAFE != null) {
      try {
        Class<?> clazz = UNSAFE.getClass();
        // Methods for getting direct buffer address.
        clazz.getMethod("objectFieldOffset", Field.class);
        clazz.getMethod("getLong", Object.class, long.class);

        clazz.getMethod("getByte", long.class);
        clazz.getMethod("putByte", long.class, byte.class);
        clazz.getMethod("getInt", long.class);
        clazz.getMethod("putInt", long.class, int.class);
        clazz.getMethod("getLong", long.class);
        clazz.getMethod("putLong", long.class, long.class);
        clazz.getMethod("setMemory", long.class, long.class, byte.class);
        clazz.getMethod("copyMemory", long.class, long.class, long.class);
        supported = true;
      } catch (Throwable e) {
        // Do nothing.
      }
    }
    return supported;
  }

  /**
   * Get the base offset for byte arrays, or {@code -1} if {@code sun.misc.Unsafe} is not available.
   */
  private static int byteArrayBaseOffset() {
    return HAS_UNSAFE_ARRAY_OPERATIONS ? UNSAFE.arrayBaseOffset(byte[].class) : -1;
  }

  /**
   * Returns the offset of the provided field, or {@code -1} if {@code sun.misc.Unsafe} is not
   * available.
   */
  private static long fieldOffset(Field field) {
    return field == null || UNSAFE == null ? -1 : UNSAFE.objectFieldOffset(field);
  }

  /**
   * Gets the field with the given name within the class, or {@code null} if not found. If found,
   * the field is made accessible.
   */
  private static Field field(Class<?> clazz, String fieldName) {
    Field field;
    try {
      field = clazz.getDeclaredField(fieldName);
      field.setAccessible(true);
    } catch (Throwable t) {
      // Failed to access the fields.
      field = null;
    }
    return field;
  }
}
