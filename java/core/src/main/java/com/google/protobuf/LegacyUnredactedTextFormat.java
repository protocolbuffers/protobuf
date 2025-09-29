package com.google.protobuf;

import static java.util.Arrays.stream;

import java.util.stream.Collectors;
import java.util.stream.StreamSupport;

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
  public static String legacyUnredactedStringValueOf(
          Object object) {
    return (object == null) ? String.valueOf(object) : legacyUnredactedToString(object);
  }

  /**
   * Map each element in an {@code Iterable<T>} to {@code Iterable<String>} using
   * legacyUnredactedStringValueOf. This is a useful shortcut for callers that operate on an
   * Iterable of objects.
   */
  @Deprecated
  public static Iterable<String> legacyUnredactedToStringList(
          Iterable<?> iterable) {
    return iterable == null
        ? null
        : StreamSupport.stream(iterable.spliterator(), false)
            .map(LegacyUnredactedTextFormat::legacyUnredactedStringValueOf)
            .collect(Collectors.toList());
  }

  /**
   * Map each element in an Object[] to String using legacyUnredactedStringValueOf. This is a useful
   * shortcut for callers that operate on an Object[] of objects.
   */
  @Deprecated
  public static String[] legacyUnredactedToStringArray(
          Object[] objects) {
    return objects == null
        ? null
        : stream(objects)
            .map(LegacyUnredactedTextFormat::legacyUnredactedStringValueOf)
            .toArray(String[]::new);
  }

  /**
   * Return String.format() with the guarantee that any Protobuf Message.toString() invoked under
   * this call always returns TextFormat (except for Message.toString() calls that are also under
   * ProtobufToStringOutput.callWithDebugFormat). This is useful for refactoring String.format()
   * calls when
   * 1) the objects being interpolated are Protobuf messages or contain Protobuf messages, and
   * 2) the existing code expects toString() on the interpolated objects to output the text format.
   */
  public static String legacyUnredactedStringFormat(String format, Object... args) {
    String[] result = new String[1];
    ProtobufToStringOutput.callWithTextFormat(() -> result[0] = String.format(format, args));
    return result[0];
  }
}
