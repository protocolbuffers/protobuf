// Protocol Buffers - Google's data interchange format
// Copyright 2025 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

package com.google.protobuf;

import static com.google.protobuf.Internal.checkNotNull;

final class BuiltinExtensionRegistry {

  private static volatile ExtensionRegistry instance = null;

  static ExtensionRegistry getInstance() {
    ExtensionRegistry instance = BuiltinExtensionRegistry.instance;
    if (instance == null) {
      synchronized (BuiltinExtensionRegistry.class) {
        instance = BuiltinExtensionRegistry.instance;
        if (instance == null) {
          instance = ExtensionRegistry.newInstance();
          instance.add(checkNotNull(JavaFeaturesProto.java_));
          instance = instance.getUnmodifiable();
          BuiltinExtensionRegistry.instance = instance;
        }
      }
    }
    return instance;
  }

  private BuiltinExtensionRegistry() {}
}
