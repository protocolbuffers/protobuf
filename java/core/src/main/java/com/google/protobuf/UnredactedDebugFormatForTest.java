package com.google.protobuf;

import javax.annotation.Nullable;

/**
 * These APIs are restricted to test-only targets, and are suitable for test purposes where it is
 * impractical to compare protos directly via ProtoTruth (e.g. log output).
 */
public final class UnredactedDebugFormatForTest {

  private UnredactedDebugFormatForTest() {}

  /** Like {@code TextFormat.printer().printToString(message)}, but for test assertion purposes. */
  public static String unredactedMultilineString(MessageOrBuilder message) {
    return TextFormat.printer()
        .printToString(message, TextFormat.Printer.FieldReporterLevel.TEXT_GENERATOR);
  }

  /** Like {@code TextFormat.printer().printToString(fields)}, but for test assertion purposes. */
  public static String unredactedMultilineString(UnknownFieldSet fields) {
    return TextFormat.printer().printToString(fields);
  }

  /**
   * Like {@code TextFormat.printer().emittingSingleLine(true).printToString(message)}, but for test
   * assertion purposes.
   */
  public static String unredactedSingleLineString(MessageOrBuilder message) {
    return TextFormat.printer()
        .emittingSingleLine(true)
        .printToString(message, TextFormat.Printer.FieldReporterLevel.TEXT_GENERATOR);
  }

  /**
   * Like {@code TextFormat.printer().emittingSingleLine(true).printToString(fields)}, but for test
   * assertion purposes.
   */
  public static String unredactedSingleLineString(UnknownFieldSet fields) {
    return TextFormat.printer().emittingSingleLine(true).printToString(fields);
  }

  /**
   * Return object.toString() with the guarantee that any Protobuf Message.toString() invoked under
   * this call always returns TextFormat (except for Message.toString() calls that are also under
   * ProtobufToStringOutput.callWithDebugFormat). This is particularly useful for toString calls on
   * objects that contain Protobuf messages (e.g collections) and existing code expects toString()
   * on these objects to contain Message.toString() outputs in TextFormat.
   */
  public static String unredactedToString(Object object) {
    return LegacyUnredactedTextFormat.legacyUnredactedToString(object);
  }

  /**
   * Return String.valueOf() with the guarantee that any Protobuf Message.toString() invoked under
   * this call always returns TextFormat (except for Message.toString() calls that are also under
   * ProtobufToStringOutput.callWithDebugFormat). This is particularly useful for explicit and
   * implicit String.valueOf() calls on objects that contain Protobuf messages (e.g collections) and
   * may be null, and existing code expects toString() on these objects to contain
   * Message.toString() outputs in TextFormat.
   */
  public static String unredactedStringValueOf(@Nullable Object object) {
    return LegacyUnredactedTextFormat.legacyUnredactedStringValueOf(object);
  }
}
