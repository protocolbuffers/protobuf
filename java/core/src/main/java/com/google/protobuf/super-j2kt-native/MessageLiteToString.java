package com.google.protobuf;

final class MessageLiteToString {
  private MessageLiteToString() {}

  static String toString(MessageLite messageLite, String commentString) {
    return commentString;
  }

  static void printField(StringBuilder buffer, int indent, String name, Object object) {
    // J2KT: No-op
  }
}
