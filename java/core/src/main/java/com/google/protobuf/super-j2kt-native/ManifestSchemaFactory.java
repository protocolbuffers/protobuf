package com.google.protobuf;

@SuppressWarnings("nullness")
final class ManifestSchemaFactory {
  public ManifestSchemaFactory() {}

  public <T> Schema<T> createSchema(Class<T> messageType) {
    return MessageSchema.newSchema(messageType, null, null, null, null, null, null);
  }
}
