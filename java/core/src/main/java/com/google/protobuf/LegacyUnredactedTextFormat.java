package com.google.protobuf;

/**
 * The legacy APIs preserve the existing toString() behavior (output TextFormat), which allows us to
 * migrate toString callers that expect TextFormat output off toString. Eventually, we will make
 * toString output DebugFormat, which is randomized and redacts SPII fields (incompatible with
 * TextFormat).
 */
public final class LegacyUnredactedTextFormat {

  private LegacyUnredactedTextFormat() {}

  /** Like {@code TextFormat.printer().printToString(message)}, but for legacy purposes. */
  static String legacyUnredactedMultilineString(MessageOrBuilder message) {
    return TextFormat.printer().printToString(message);
  }

  /** Like {@code TextFormat.printer().printToString(fields)}, but for legacy purposes. */
  static String legacyUnredactedMultilineString(UnknownFieldSet fields) {
    return TextFormat.printer().printToString(fields);
  }

  /**
   * Like {@code TextFormat.printer().emittingSingleLine(true).printToString(message)}, but for
   * legacy purposes.
   */
  static String legacyUnredactedSingleLineString(MessageOrBuilder message) {
    return TextFormat.printer().emittingSingleLine(true).printToString(message);
  }

  /**
   * Like {@code TextFormat.printer().emittingSingleLine(true).printToString(fields)}, but for
   * legacy purposes.
   */
  static String legacyUnredactedSingleLineString(UnknownFieldSet fields) {
    return TextFormat.printer().emittingSingleLine(true).printToString(fields);
  }
}
