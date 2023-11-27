// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

package com.google.protobuf;

import static com.google.protobuf.ExtensionRegistryLite.EMPTY_REGISTRY_LITE;

/**
 * A factory object to create instances of {@link ExtensionRegistryLite}.
 *
 * <p>This factory detects (via reflection) if the full (non-Lite) protocol buffer libraries are
 * available, and if so, the instances returned are actually {@link ExtensionRegistry}.
 */
final class ExtensionRegistryFactory {

  static final String FULL_REGISTRY_CLASS_NAME = "com.google.protobuf.ExtensionRegistry";

  /* Visible for Testing
  @Nullable */
  static final Class<?> EXTENSION_REGISTRY_CLASS = reflectExtensionRegistry();

  static Class<?> reflectExtensionRegistry() {
    try {
      return Class.forName(FULL_REGISTRY_CLASS_NAME);
    } catch (ClassNotFoundException e) {
      // The exception allocation is potentially expensive on Android (where it can be triggered
      // many times at start up). Is there a way to ameliorate this?
      return null;
    }
  }

  /** Construct a new, empty instance. */
  public static ExtensionRegistryLite create() {
    ExtensionRegistryLite result = invokeSubclassFactory("newInstance");

    return result != null ? result : new ExtensionRegistryLite();
  }

  /** Get the unmodifiable singleton empty instance. */
  public static ExtensionRegistryLite createEmpty() {
    ExtensionRegistryLite result = invokeSubclassFactory("getEmptyRegistry");

    return result != null ? result : EMPTY_REGISTRY_LITE;
  }

  static boolean isFullRegistry(ExtensionRegistryLite registry) {
    return EXTENSION_REGISTRY_CLASS != null
        && EXTENSION_REGISTRY_CLASS.isAssignableFrom(registry.getClass());
  }

  private static final ExtensionRegistryLite invokeSubclassFactory(String methodName) {
    if (EXTENSION_REGISTRY_CLASS == null) {
      return null;
    }

    try {
      return (ExtensionRegistryLite)
          EXTENSION_REGISTRY_CLASS.getDeclaredMethod(methodName).invoke(null);
    } catch (Exception e) {
      return null;
    }
  }
}
