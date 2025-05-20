package com.google.protobuf;

/**
 * The Android version of UnredactedDebugFormatForTest, which just calls the default toString()
 * implementation.
 */
public final class UnredactedDebugFormatForTest {

  private UnredactedDebugFormatForTest() {}

  public static String unredactedStringValueOf(
          Object object) {
    return String.valueOf(object);
  }
}
