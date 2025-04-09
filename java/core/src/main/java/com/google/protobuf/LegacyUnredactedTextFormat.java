package com.google.protobuf;

import javax.annotation.Nullable;

/**
 * The legacy APIs preserve the existing toString() behavior (output TextFormat), which allows us to
 * migrate toString callers that expect TextFormat output off toString. Eventually, we will make
 * toString output DebugFormat, which is randomized and redacts SPII fields (incompatible with
 * TextFormat).
 */
public final class LegacyUnredactedTextFormat {

  private LegacyUnredactedTextFormat() {}

  /** Like {@code TextFormat.printer().printToString(message)}, but for legacy purposes. */
  public static String legacyUnredactedMultilineString(MessageOrBuilder message) {
    return TextFormat.printer()
        .printToString(message, TextFormat.Printer.FieldReporterLevel.LEGACY_MULTILINE);
  }

  /** Like {@code TextFormat.printer().printToString(fields)}, but for legacy purposes. */
  public static String legacyUnredactedMultilineString(UnknownFieldSet fields) {
    return TextFormat.printer().printToString(fields);
  }

  /**
   * Like {@code TextFormat.printer().emittingSingleLine(true).printToString(message)}, but for
   * legacy purposes.
   */
  public static String legacyUnredactedSingleLineString(MessageOrBuilder message) {
    return TextFormat.printer()
        .emittingSingleLine(true)
        .printToString(message, TextFormat.Printer.FieldReporterLevel.LEGACY_SINGLE_LINE);
  }

  /**
   * Like {@code TextFormat.printer().emittingSingleLine(true).printToString(fields)}, but for
   * legacy purposes.
   */
  public static String legacyUnredactedSingleLineString(UnknownFieldSet fields) {
    return TextFormat.printer().emittingSingleLine(true).printToString(fields);
  }

  /**
   * Return object.toString() with the guarantee that any Protobuf Message.toString() invoked under
   * this call always returns TextFormat (except for Message.toString() calls that are also under
   * ProtobufToStringOutput.callWithDebugFormat). This is particularly useful for toString calls on
   * objects that contain Protobuf messages (e.g collections) and existing code expects toString()
   * on these objects to contain Message.toString() outputs in TextFormat.
   */
  public static String legacyUnredactedToString(Object object) {
    String[] result = new String[1];
    ProtobufToStringOutput.callWithTextFormat(() -> result[0] = object.toString());
    return result[0];
  }

  /**
   * Return String.valueOf() with the guarantee that any Protobuf Message.toString() invoked under
   * this call always returns TextFormat (except for Message.toString() calls that are also under
   * ProtobufToStringOutput.callWithDebugFormat). This is particularly useful for explicit and
   * implicit String.valueOf() calls on objects that contain Protobuf messages (e.g collections) and
   * may be null, and existing code expects toString() on these objects to contain
   * Message.toString() outputs in TextFormat.
   */
  public static String legacyUnredactedStringValueOf(@Nullable Object object) {
    return (object == null) ? String.valueOf(object) : legacyUnredactedToString(object);
  }
}
