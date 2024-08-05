package com.google.protobuf;

import com.google.protobuf.Descriptors.FieldDescriptor;

/**
 * Provides an explicit API for unstable, redacting debug output suitable for debug logging. This
 * implementation is based on TextFormat, but should not be parsed.
 */
public final class DebugFormat {

  private final boolean isSingleLine;

  private DebugFormat(boolean singleLine) {
    isSingleLine = singleLine;
  }

  public static DebugFormat singleLine() {
    return new DebugFormat(true);
  }

  public static DebugFormat multiline() {
    return new DebugFormat(false);
  }

  public String toString(MessageOrBuilder message) {
    return TextFormat.printer()
        .emittingSingleLine(this.isSingleLine)
        .enablingSafeDebugFormat(true)
        .printToString(message, this.isSingleLine
                ? TextFormat.Printer.FieldReporterLevel.DEBUG_SINGLE_LINE
                : TextFormat.Printer.FieldReporterLevel.DEBUG_MULTILINE);
  }

  public String toString(FieldDescriptor field, Object value) {
    return TextFormat.printer()
        .emittingSingleLine(this.isSingleLine)
        .enablingSafeDebugFormat(true)
        .printFieldToString(field, value);
  }

  public String toString(UnknownFieldSet fields) {
    return TextFormat.printer()
        .emittingSingleLine(this.isSingleLine)
        .enablingSafeDebugFormat(true)
        .printToString(fields);
  }

  public Object lazyToString(MessageOrBuilder message) {
    return new LazyDebugOutput(message, this);
  }

  public Object lazyToString(UnknownFieldSet fields) {
    return new LazyDebugOutput(fields, this);
  }

  private static class LazyDebugOutput {
    private final MessageOrBuilder message;
    private final UnknownFieldSet fields;
    private final DebugFormat format;

    LazyDebugOutput(MessageOrBuilder message, DebugFormat format) {
      this.message = message;
      this.fields = null;
      this.format = format;
    }

    LazyDebugOutput(UnknownFieldSet fields, DebugFormat format) {
      this.message = null;
      this.fields = fields;
      this.format = format;
    }

    @Override
    public String toString() {
      if (message != null) {
        return format.toString(message);
      }
      return format.toString(fields);
    }
  }
}
