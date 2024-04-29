// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

package com.google.protobuf;

import java.util.Arrays;

/**
 * A location in the source code.
 *
 * <p>A location is the starting line number and starting column number.
 */
public final class TextFormatParseLocation {

  /** The empty location. */
  public static final TextFormatParseLocation EMPTY = new TextFormatParseLocation(-1, -1);

  /**
   * Create a location.
   *
   * @param line the starting line number
   * @param column the starting column number
   * @return a {@code ParseLocation}
   */
  static TextFormatParseLocation create(int line, int column) {
    if (line == -1 && column == -1) {
      return EMPTY;
    }
    if (line < 0 || column < 0) {
      throw new IllegalArgumentException(
          String.format("line and column values must be >= 0: line %d, column: %d", line, column));
    }
    return new TextFormatParseLocation(line, column);
  }

  private final int line;
  private final int column;

  private TextFormatParseLocation(int line, int column) {
    this.line = line;
    this.column = column;
  }

  public int getLine() {
    return line;
  }

  public int getColumn() {
    return column;
  }

  @Override
  public String toString() {
    return String.format("ParseLocation{line=%d, column=%d}", line, column);
  }

  @Override
  public boolean equals(Object o) {
    if (o == this) {
      return true;
    }
    if (!(o instanceof TextFormatParseLocation)) {
      return false;
    }
    TextFormatParseLocation that = (TextFormatParseLocation) o;
    return (this.line == that.getLine()) && (this.column == that.getColumn());
  }

  @Override
  public int hashCode() {
    int[] values = {line, column};
    return Arrays.hashCode(values);
  }
}
