// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

package com.google.protobuf;

import java.nio.charset.StandardCharsets;

/**
 * Provide text format escaping of proto instances. These ASCII characters are escaped:
 *
 * <ul>
 *   <li>ASCII #7 (bell) --> \a
 *   <li>ASCII #8 (backspace) --> \b
 *   <li>ASCII #9 (horizontal tab) --> \t
 *   <li>ASCII #10 (linefeed) --> \n
 *   <li>ASCII #11 (vertical tab) --> \v
 *   <li>ASCII #13 (carriage return) --> \r
 *   <li>ASCII #12 (formfeed) --> \f
 *   <li>ASCII #34 (apostrophe) --> \'
 *   <li>ASCII #39 (straight double quote) --> \"
 *   <li>ASCII #92 (backslash) --> \\
 *   <li>ASCII characters besides those three which are in the range [32..127] inclusive are output
 *       as is, unescaped.
 *   <li>All other bytes are escaped as octal sequences. If we are printing text, we convert to
 *       UTF-8 and print any high codepoints as their UTF-8 encoded units in octal escapes.
 * </ul>
 */
final class TextFormatEscaper {
  private TextFormatEscaper() {}

  /** Backslash escapes bytes in the format used in protocol buffer text format. */
  static String escapeBytes(byte[] input) {
    final StringBuilder builder = new StringBuilder(input.length);
    for (int i = 0; i < input.length; i++) {
      byte b = input[i];
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

  /** Backslash escapes bytes in the format used in protocol buffer text format. */
  static String escapeBytes(final ByteString input) {
    return escapeBytes(input.toByteArray());
  }

  /** Like {@link #escapeBytes(ByteString)}, but escapes a text string. */
  static String escapeText(String input) {
    boolean hasSingleQuote = false;
    boolean hasDoubleQuote = false;
    boolean hasBackslash = false;

    for (int i = 0; i < input.length(); ++i) {
      char c = input.charAt(i);

      // If there are any characters outside of ASCII range we eagerly convert to UTF and escape on
      // those bytes (including quotes as well). Note that escaping to UTF8 bytes instead of \\u
      // sequences is itself somewhat nonsensical, but JavaProto has behaved this way for a long
      // time, and changing the behavior would be disruptive.
      if (c < 0x20 || c > 0x7e) {
        return escapeBytes(input.getBytes(StandardCharsets.UTF_8));
      }

      // While in this loop, keep track if there are any single quotes, double quotes, or
      // backslashes. This can help avoid multiple passes over the string looking for each of the
      // bad characters.
      switch (c) {
        case '\'':
          hasSingleQuote = true;
          break;
        case '"':
          hasDoubleQuote = true;
          break;
        case '\\':
          hasBackslash = true;
          break;
        default:
          break;
      }
    }

    // Note: escape backslashes first. Order matters to avoid double-escaping the backslashes that
    // are added when escaping the quotes.
    if (hasBackslash) {
      input = input.replace("\\", "\\\\");
    }
    if (hasSingleQuote) {
      input = input.replace("\'", "\\\'");
    }
    if (hasDoubleQuote) {
      input = input.replace("\"", "\\\"");
    }

    return input;
  }

  /** Escape double quotes and backslashes in a String for unicode output of a message. */
  static String escapeDoubleQuotesAndBackslashes(String input) {
    return input.replace("\\", "\\\\").replace("\"", "\\\"");
  }
}
