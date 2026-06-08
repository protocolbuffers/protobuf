package com.google.protobuf;

import java.nio.ByteBuffer;

final class UnsafeUtil {
  private UnsafeUtil() {}

  static boolean hasUnsafeArrayOperations() {
    return false;
  }

  static boolean hasUnsafeByteBufferOperations() {
    return false;
  }

  static long getArrayBaseOffset() {
    return -1;
  }

  static byte getByte(Object target, long offset) {
    throw new UnsupportedOperationException();
  }

  static void putByte(Object target, long offset, byte value) {
    throw new UnsupportedOperationException();
  }

  static int getInt(Object target, long offset) {
    throw new UnsupportedOperationException();
  }

  static void putInt(Object target, long offset, int value) {
    throw new UnsupportedOperationException();
  }

  static long getLong(Object target, long offset) {
    throw new UnsupportedOperationException();
  }

  static void putLong(Object target, long offset, long value) {
    throw new UnsupportedOperationException();
  }

  static boolean getBoolean(Object target, long offset) {
    throw new UnsupportedOperationException();
  }

  static void putBoolean(Object target, long offset, boolean value) {
    throw new UnsupportedOperationException();
  }

  static float getFloat(Object target, long offset) {
    throw new UnsupportedOperationException();
  }

  static void putFloat(Object target, long offset, float value) {
    throw new UnsupportedOperationException();
  }

  static double getDouble(Object target, long offset) {
    throw new UnsupportedOperationException();
  }

  static void putDouble(Object target, long offset, double value) {
    throw new UnsupportedOperationException();
  }

  static Object getObject(Object target, long offset) {
    throw new UnsupportedOperationException();
  }

  static void putObject(Object target, long offset, Object value) {
    throw new UnsupportedOperationException();
  }

  static byte getByte(long address) {
    throw new UnsupportedOperationException();
  }

  static void putByte(long address, byte value) {
    throw new UnsupportedOperationException();
  }

  static int getInt(long address) {
    throw new UnsupportedOperationException();
  }

  static void putInt(long address, int value) {
    throw new UnsupportedOperationException();
  }

  static long getLong(long address) {
    throw new UnsupportedOperationException();
  }

  static void putLong(long address, long value) {
    throw new UnsupportedOperationException();
  }

  static void copyMemory(long srcAddress, long targetAddress, long length) {
    throw new UnsupportedOperationException();
  }

  static void copyMemory(
      Object src, long srcOffset, Object target, long targetOffset, long length) {
    throw new UnsupportedOperationException();
  }

  static long addressOffset(ByteBuffer buffer) {
    throw new UnsupportedOperationException();
  }

  static <T> T allocateInstance(Class<T> clazz) {
    throw new UnsupportedOperationException();
  }
}
