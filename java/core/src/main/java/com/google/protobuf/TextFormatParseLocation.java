// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
// https://developers.google.com/protocol-buffers/
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

package com.google.protobuf;

import java.util.Arrays;

/**
 * A location in the source code.
 *
 * <p>A location is the starting line number and starting column number.
 */
public final class TextFormatParseLocation {

  /**
   * The empty location.
   */
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
    return (this.line == that.getLine())
         && (this.column == that.getColumn());
  }

  @Override
  public int hashCode() {
    int[] values = {line, column};
    return Arrays.hashCode(values);
  }
}
