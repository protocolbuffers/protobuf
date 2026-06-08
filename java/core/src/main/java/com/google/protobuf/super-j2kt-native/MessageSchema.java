package com.google.protobuf;

import java.io.IOException;

final class MessageSchema<T> implements Schema<T> {
  static <T> MessageSchema<T> newSchema(
      Class<T> messageClass,
      MessageInfo messageInfo,
      NewInstanceSchema newInstanceSchema,
      ListFieldSchema listFieldSchema,
      UnknownFieldSchema<?, ?> unknownFieldSchema,
      ExtensionSchema<?> extensionSchema,
      MapFieldSchema mapFieldSchema) {
    return new MessageSchema<T>();
  }

  private MessageSchema() {}

  @Override
  public void writeTo(T message, Writer writer) throws IOException {
    throw new UnsupportedOperationException();
  }

  @Override
  public void mergeFrom(T message, Reader reader, ExtensionRegistryLite extensionRegistry)
      throws IOException {
    throw new UnsupportedOperationException();
  }

  @Override
  public void mergeFrom(
      T message, byte[] data, int position, int limit, ArrayDecoders.Registers registers)
      throws IOException {
    throw new UnsupportedOperationException();
  }

  @Override
  public void makeImmutable(T message) {
    throw new UnsupportedOperationException();
  }

  @Override
  public final boolean isInitialized(T message) {
    throw new UnsupportedOperationException();
  }

  @Override
  public int getSerializedSize(T message) {
    throw new UnsupportedOperationException();
  }

  @Override
  public T newInstance() {
    throw new UnsupportedOperationException();
  }

  @Override
  public boolean equals(T message, T other) {
    throw new UnsupportedOperationException();
  }

  @Override
  public int hashCode(T message) {
    throw new UnsupportedOperationException();
  }

  @Override
  public void mergeFrom(T message, T other) {
    throw new UnsupportedOperationException();
  }

  int parseMessage(
      T message,
      byte[] data,
      int position,
      int limit,
      int endDelimited,
      ArrayDecoders.Registers registers)
      throws IOException {
    throw new UnsupportedOperationException();
  }

  static UnknownFieldSetLite getMutableUnknownFields(Object message) {
    throw new UnsupportedOperationException();
  }
}
