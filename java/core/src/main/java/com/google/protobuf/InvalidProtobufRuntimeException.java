package com.google.protobuf;

/**
 * RuntimeException thrown when a Protobuf operation fails due to invalid data.
 */
public final class InvalidProtobufRuntimeException extends RuntimeException {
  public InvalidProtobufRuntimeException(String message) {
    super(message);
  }

  public InvalidProtobufRuntimeException(Exception e) {
    super(e.getMessage(), e);
  }
}
