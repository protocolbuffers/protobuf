// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

package com.google.protobuf;

/**
 * Provide text format escaping of proto instances. These ASCII characters are escaped:
 *
 * ASCII #7   (bell) --> \a
 * ASCII #8   (backspace) --> \b
 * ASCII #9   (horizontal tab) --> \t
 * ASCII #10  (linefeed) --> \n
 * ASCII #11  (vertical tab) --> \v
 * ASCII #13  (carriage return) --> \r
 * ASCII #12  (formfeed) --> \f
 * ASCII #34  (apostrophe) --> \'
 * ASCII #39  (straight double quote) --> \"
 * ASCII #92  (backslash) --> \\
 *
 * Other printable ASCII characters between 32 and 127 inclusive are output as is, unescaped.
 * Other ASCII characters less than 32 and all Unicode characters 128 or greater are
 * first encoded as UTF-8, then each byte is escaped individually as a 3-digit octal escape.
 */
final class TextFormatEscaper {
  private TextFormatEscaper() {}

  private interface ByteSequence {
    int size();

    byte byteAt(int offset);
  }

  /**
   * Backslash escapes bytes in the format used in protocol buffer text format.
   */
  static String escapeBytes(ByteSequence input) {
    final StringBuilder builder = new StringBuilder(input.size());
    for (int i = 0; i < input.size(); i++) {
      byte b = input.byteAt(i);
      switch (b) {
        case 0x07:
          builder.append("\\a");
          break;
        case '\b':
          builder.append("\\b");
          break;
        case '\f':
          builder.append("\\f");
          break;
        case '\n':
          builder.append("\\n");
          break;
        case '\r':
          builder.append("\\r");
          break;
        case '\t':
          builder.append("\\t");
          break;
        case 0x0b:
          builder.append("\\v");
          break;
        case '\\':
          builder.append("\\\\");
          break;
        case '\'':
          builder.append("\\\'");
          break;
        case '"':
          builder.append("\\\"");
          break;
        default:
          // Only ASCII characters between 0x20 (space) and 0x7e (tilde) are
          // printable.  Other byte values must be escaped.
          if (b >= 0x20 && b <= 0x7e) {
            builder.append((char) b);
          } else {
            builder.append('\\');
            builder.append((char) ('0' + ((b >>> 6) & 3)));
            builder.append((char) ('0' + ((b >>> 3) & 7)));
            builder.append((char) ('0' + (b & 7)));
          }
          break;
      }
    }
    return builder.toString();
  }

  /**
   * Backslash escapes bytes in the format used in protocol buffer text format.
   */
  static String escapeBytes(final ByteString input) {
    return escapeBytes(
        new ByteSequence() {
          @Override
          public int size() {
            return input.size();
          }

          @Override
          public byte byteAt(int offset) {
            return input.byteAt(offset);
          }
        });
  }

  /** Like {@link #escapeBytes(ByteString)}, but used for byte array. */
  static String escapeBytes(final byte[] input) {
    return escapeBytes(
        new ByteSequence() {
          @Override
          public int size() {
            return input.length;
          }

          @Override
          public byte byteAt(int offset) {
            return input[offset];
          }
        });
  }

  /**
   * Like {@link #escapeBytes(ByteString)}, but escapes a text string.
   */
  static String escapeText(String input) {
    return escapeBytes(ByteString.copyFromUtf8(input));
  }

  /** Escape double quotes and backslashes in a String for unicode output of a message. */
  static String escapeDoubleQuotesAndBackslashes(String input) {
    return input.replace("\\", "\\\\").replace("\"", "\\\"");
  }
}
