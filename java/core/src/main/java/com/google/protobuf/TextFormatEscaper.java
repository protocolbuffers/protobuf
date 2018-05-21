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

/**
 * Provide text format escaping support for proto2 instances.
 */
final class TextFormatEscaper {
  private TextFormatEscaper() {}

  private interface ByteSequence {
    int size();
    byte byteAt(int offset);
  }

  /**
   * Escapes bytes in the format used in protocol buffer text format, which
   * is the same as the format used for C string literals.  All bytes
   * that are not printable 7-bit ASCII characters are escaped, as well as
   * backslash, single-quote, and double-quote characters.  Characters for
   * which no defined short-hand escape sequence is defined will be escaped
   * using 3-digit octal sequences.
   */
  static String escapeBytes(final ByteSequence input) {
    final StringBuilder builder = new StringBuilder(input.size());
    for (int i = 0; i < input.size(); i++) {
      final byte b = input.byteAt(i);
      switch (b) {
        // Java does not recognize \a or \v, apparently.
        case 0x07: builder.append("\\a"); break;
        case '\b': builder.append("\\b"); break;
        case '\f': builder.append("\\f"); break;
        case '\n': builder.append("\\n"); break;
        case '\r': builder.append("\\r"); break;
        case '\t': builder.append("\\t"); break;
        case 0x0b: builder.append("\\v"); break;
        case '\\': builder.append("\\\\"); break;
        case '\'': builder.append("\\\'"); break;
        case '"' : builder.append("\\\""); break;
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
   * Escapes bytes in the format used in protocol buffer text format, which
   * is the same as the format used for C string literals.  All bytes
   * that are not printable 7-bit ASCII characters are escaped, as well as
   * backslash, single-quote, and double-quote characters.  Characters for
   * which no defined short-hand escape sequence is defined will be escaped
   * using 3-digit octal sequences.
   */
  static String escapeBytes(final ByteString input) {
    return escapeBytes(new ByteSequence() {
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

  /**
   * Like {@link #escapeBytes(ByteString)}, but used for byte array.
   */
  static String escapeBytes(final byte[] input) {
    return escapeBytes(new ByteSequence() {
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
   * Non-ASCII characters are first encoded as UTF-8, then each byte is escaped
   * individually as a 3-digit octal escape.  Yes, it's weird.
   */
  static String escapeText(final String input) {
    return escapeBytes(ByteString.copyFromUtf8(input));
  }

  /**
   * Escape double quotes and backslashes in a String for unicode output of a message.
   */
  static String escapeDoubleQuotesAndBackslashes(final String input) {
    return input.replace("\\", "\\\\").replace("\"", "\\\"");
  }
}
