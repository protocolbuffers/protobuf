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
