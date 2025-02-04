package com.google.protobuf;

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
}
