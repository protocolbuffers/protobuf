package com.google.protobuf;

final class GeneratedMessageInfoFactory implements MessageInfoFactory {
  private static final GeneratedMessageInfoFactory instance = new GeneratedMessageInfoFactory();

  private GeneratedMessageInfoFactory() {}

  public static GeneratedMessageInfoFactory getInstance() {
    return instance;
  }

  @Override
  public boolean isSupported(Class<?> messageType) {
    return false;
  }

  @Override
  public MessageInfo messageInfoFor(Class<?> messageType) {
    throw new UnsupportedOperationException();
  }
}
