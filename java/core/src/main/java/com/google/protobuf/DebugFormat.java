package com.google.protobuf;

import com.google.protobuf.Descriptors.FieldDescriptor;

/**
 * Provides an explicit API for unstable, redacting debug output suitable for debug logging. This
 * implementation is based on TextFormat, but should not be parsed.
 */
final class DebugFormat {

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
        .printToString(message);
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
    return new LazyDebugOutput(message, this.isSingleLine);
  }

  public Object lazyToString(UnknownFieldSet fields) {
    return new LazyDebugOutput(fields, this.isSingleLine);
  }

  private static class LazyDebugOutput {
    private final MessageOrBuilder message;
    private final UnknownFieldSet fields;
    private final boolean isSingleLine;

    LazyDebugOutput(MessageOrBuilder message, boolean isSingleLine) {
      this.message = message;
      this.fields = null;
      this.isSingleLine = isSingleLine;
    }

    LazyDebugOutput(UnknownFieldSet fields, boolean isSingleLine) {
      this.fields = fields;
      this.message = null;
      this.isSingleLine = isSingleLine;
    }

    @Override
    public String toString() {
      if (message != null) {
        return this.isSingleLine
            ? DebugFormat.singleLine().toString(message)
            : DebugFormat.multiline().toString(message);
      }
      return this.isSingleLine
          ? DebugFormat.singleLine().toString(fields)
          : DebugFormat.multiline().toString(fields);
    }
  }
}
