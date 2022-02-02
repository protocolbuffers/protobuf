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
