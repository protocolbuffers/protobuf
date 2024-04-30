package com.google.protobuf;

/**
 * This file contains a set of legacy protobuf-printing APIs that protobuf toString() callers may
 * temporarily migrate to, as part of making protobuf toString() output non-deterministic. These
 * APIs provide TextFormat-compatible output. There should be no new uses of this API after
 * refactoring is complete.
 */
final class LegacyUnredactedTextFormat {

  private LegacyUnredactedTextFormat() {}

  static String legacyUnredactedTextFormat(MessageOrBuilder message) {
    return TextFormat.printer().printToString(message);
  }

  static String legacyUnredactedTextFormat(UnknownFieldSet fields) {
    return TextFormat.printer().printToString(fields);
  }

  static String legacyUnredactedSingleLineTextFormat(MessageOrBuilder message) {
    return TextFormat.printer().emittingSingleLine(true).printToString(message);
  }

  static String legacyUnredactedSingleLineTextFormat(UnknownFieldSet fields) {
    return TextFormat.printer().emittingSingleLine(true).printToString(fields);
  }
}
