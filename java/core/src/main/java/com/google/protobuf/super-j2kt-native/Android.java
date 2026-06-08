package com.google.protobuf;

@SuppressWarnings("nullness")
final class Android {
  private Android() {}

  static boolean assumeLiteRuntime = false;

  static boolean isOnAndroidDevice() {
    return false;
  }

  static Class<?> getMemoryClass() {
    return null;
  }
}
