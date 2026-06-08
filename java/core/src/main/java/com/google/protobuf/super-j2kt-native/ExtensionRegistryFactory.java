package com.google.protobuf;

final class ExtensionRegistryFactory {
  private ExtensionRegistryFactory() {}

  public static ExtensionRegistryLite create() {
    return new ExtensionRegistryLite();
  }

  public static ExtensionRegistryLite createEmpty() {
    return ExtensionRegistryLite.EMPTY_REGISTRY_LITE;
  }

  static ExtensionRegistryLite loadGeneratedRegistry() {
    return ExtensionRegistryLite.EMPTY_REGISTRY_LITE;
  }

  static boolean isFullRegistry(ExtensionRegistryLite registry) {
    return false;
  }
}
