package com.google.protobuf;

import com.google.protobuf.Descriptors.FieldDescriptor;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.Random;

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
    return applyUnstablePrefix(
        TextFormat.printer()
            .emittingSingleLine(this.isSingleLine)
            .redacting(true)
            .printToString(message));
  }

  public String toString(FieldDescriptor field, Object value) {
    return applyUnstablePrefix(
        TextFormat.printer()
            .emittingSingleLine(this.isSingleLine)
            .redacting(true)
            .printFieldToString(field, value));
  }

  public String toString(UnknownFieldSet fields) {
    return applyUnstablePrefix(
        TextFormat.printer()
            .emittingSingleLine(this.isSingleLine)
            .redacting(true)
            .printToString(fields));
  }

  // This prefix should remain stable only per-process.
  private String applyUnstablePrefix(String debugString) {
    return debugString;
  }

  public Object lazyToString(MessageOrBuilder message) {
    return new LazyDebugOutput(toString(message));
  }

  public Object lazyToString(UnknownFieldSet fields) {
    return new LazyDebugOutput(toString(fields));
  }

  private static class LazyDebugOutput {
    private final String debugOutput;

    LazyDebugOutput(String debugOutput) {
      this.debugOutput = debugOutput;
    }

    @Override
    public String toString() {
      return debugOutput;
    }
  }
}
