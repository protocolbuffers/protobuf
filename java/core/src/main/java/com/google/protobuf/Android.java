// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

package com.google.protobuf;

final class Android {
  private Android() {
  }

  // Set to true in lite_proguard_android.pgcfg.
  @SuppressWarnings("ConstantField")
  private static boolean ASSUME_ANDROID;

  private static final Class<?> MEMORY_CLASS = getClassForName("libcore.io.Memory");

  private static final boolean IS_ROBOLECTRIC =
      !ASSUME_ANDROID && getClassForName("org.robolectric.Robolectric") != null;

  /** Returns {@code true} if running on an Android device. */
  static boolean isOnAndroidDevice() {
    return ASSUME_ANDROID || (MEMORY_CLASS != null && !IS_ROBOLECTRIC);
  }

  /** Returns the memory class or {@code null} if not on Android device. */
  static Class<?> getMemoryClass() {
    return MEMORY_CLASS;
  }

  @SuppressWarnings("unchecked")
  private static <T> Class<T> getClassForName(String name) {
    try {
      return (Class<T>) Class.forName(name);
    } catch (Throwable e) {
      return null;
    }
  }
}
