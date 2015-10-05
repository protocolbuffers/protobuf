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

import com.google.protobuf.Descriptors.Descriptor;
import com.google.protobuf.Descriptors.EnumDescriptor;
import com.google.protobuf.Descriptors.EnumValueDescriptor;
import com.google.protobuf.Descriptors.FieldDescriptor;

import java.io.IOException;
import java.math.BigInteger;
import java.nio.CharBuffer;
import java.util.ArrayList;
import java.util.List;
import java.util.Locale;
import java.util.Map;
import java.util.logging.Logger;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

/**
 * Provide text parsing and formatting support for proto2 instances.
 * The implementation largely follows google/protobuf/text_format.cc.
 *
 * @author wenboz@google.com Wenbo Zhu
 * @author kenton@google.com Kenton Varda
 */
public final class TextFormat {
  private TextFormat() {}

  private static final Logger logger =
      Logger.getLogger(TextFormat.class.getName());

  private static final Printer DEFAULT_PRINTER = new Printer();
  private static final Printer SINGLE_LINE_PRINTER =
      (new Printer()).setSingleLineMode(true);
  private static final Printer UNICODE_PRINTER =
      (new Printer()).setEscapeNonAscii(false);

  /**
   * Outputs a textual representation of the Protocol Message supplied into
   * the parameter output. (This representation is the new version of the
   * classic "ProtocolPrinter" output from the original Protocol Buffer system)
   */
  public static void print(
      final MessageOrBuilder message, final Appendable output)
      throws IOException {
    DEFAULT_PRINTER.print(message, new TextGenerator(output));
  }

  /** Outputs a textual representation of {@code fields} to {@code output}. */
  public static void print(final UnknownFieldSet fields,
                           final Appendable output)
                           throws IOException {
    DEFAULT_PRINTER.printUnknownFields(fields, new TextGenerator(output));
  }

  /**
   * Same as {@code print()}, except that non-ASCII characters are not
   * escaped.
   */
  public static void printUnicode(
      final MessageOrBuilder message, final Appendable output)
      throws IOException {
    UNICODE_PRINTER.print(message, new TextGenerator(output));
  }

  /**
   * Same as {@code print()}, except that non-ASCII characters are not
   * escaped.
   */
  public static void printUnicode(final UnknownFieldSet fields,
                                  final Appendable output)
                                  throws IOException {
    UNICODE_PRINTER.printUnknownFields(fields, new TextGenerator(output));
  }

  /**
   * Generates a human readable form of this message, useful for debugging and
   * other purposes, with no newline characters.
   */
  public static String shortDebugString(final MessageOrBuilder message) {
    try {
      final StringBuilder sb = new StringBuilder();
      SINGLE_LINE_PRINTER.print(message, new TextGenerator(sb));
      // Single line mode currently might have an extra space at the end.
      return sb.toString().trim();
    } catch (IOException e) {
      throw new IllegalStateException(e);
    }
  }

  /**
   * Generates a human readable form of the field, useful for debugging
   * and other purposes, with no newline characters.
   */
  public static String shortDebugString(final FieldDescriptor field,
                                        final Object value) {
    try {
      final StringBuilder sb = new StringBuilder();
      SINGLE_LINE_PRINTER.printField(field, value, new TextGenerator(sb));
      return sb.toString().trim();
    } catch (IOException e) {
        throw new IllegalStateException(e);
    }
  }

  /**
   * Generates a human readable form of the unknown fields, useful for debugging
   * and other purposes, with no newline characters.
   */
  public static String shortDebugString(final UnknownFieldSet fields) {
    try {
      final StringBuilder sb = new StringBuilder();
      SINGLE_LINE_PRINTER.printUnknownFields(fields, new TextGenerator(sb));
      // Single line mode currently might have an extra space at the end.
      return sb.toString().trim();
    } catch (IOException e) {
      throw new IllegalStateException(e);
    }
  }

  /**
   * Like {@code print()}, but writes directly to a {@code String} and
   * returns it.
   */
  public static String printToString(final MessageOrBuilder message) {
    try {
      final StringBuilder text = new StringBuilder();
      print(message, text);
      return text.toString();
    } catch (IOException e) {
      throw new IllegalStateException(e);
    }
  }

  /**
   * Like {@code print()}, but writes directly to a {@code String} and
   * returns it.
   */
  public static String printToString(final UnknownFieldSet fields) {
    try {
      final StringBuilder text = new StringBuilder();
      print(fields, text);
      return text.toString();
    } catch (IOException e) {
      throw new IllegalStateException(e);
    }
  }

  /**
   * Same as {@code printToString()}, except that non-ASCII characters
   * in string type fields are not escaped in backslash+octals.
   */
  public static String printToUnicodeString(final MessageOrBuilder message) {
    try {
      final StringBuilder text = new StringBuilder();
      UNICODE_PRINTER.print(message, new TextGenerator(text));
      return text.toString();
    } catch (IOException e) {
      throw new IllegalStateException(e);
    }
  }

  /**
   * Same as {@code printToString()}, except that non-ASCII characters
   * in string type fields are not escaped in backslash+octals.
   */
  public static String printToUnicodeString(final UnknownFieldSet fields) {
    try {
      final StringBuilder text = new StringBuilder();
      UNICODE_PRINTER.printUnknownFields(fields, new TextGenerator(text));
      return text.toString();
    } catch (IOException e) {
      throw new IllegalStateException(e);
    }
  }

  public static void printField(final FieldDescriptor field,
                                final Object value,
                                final Appendable output)
                                throws IOException {
    DEFAULT_PRINTER.printField(field, value, new TextGenerator(output));
  }

  public static String printFieldToString(final FieldDescriptor field,
                                          final Object value) {
    try {
      final StringBuilder text = new StringBuilder();
      printField(field, value, text);
      return text.toString();
    } catch (IOException e) {
      throw new IllegalStateException(e);
    }
  }

  /**
   * Outputs a textual representation of the value of given field value.
   *
   * @param field the descriptor of the field
   * @param value the value of the field
   * @param output the output to which to append the formatted value
   * @throws ClassCastException if the value is not appropriate for the
   *     given field descriptor
   * @throws IOException if there is an exception writing to the output
   */
  public static void printFieldValue(final FieldDescriptor field,
                                     final Object value,
                                     final Appendable output)
                                     throws IOException {
    DEFAULT_PRINTER.printFieldValue(field, value, new TextGenerator(output));
  }

  /**
   * Outputs a textual representation of the value of an unknown field.
   *
   * @param tag the field's tag number
   * @param value the value of the field
   * @param output the output to which to append the formatted value
   * @throws ClassCastException if the value is not appropriate for the
   *     given field descriptor
   * @throws IOException if there is an exception writing to the output
   */
  public static void printUnknownFieldValue(final int tag,
                                            final Object value,
                                            final Appendable output)
                                            throws IOException {
    printUnknownFieldValue(tag, value, new TextGenerator(output));
  }

  private static void printUnknownFieldValue(final int tag,
                                             final Object value,
                                             final TextGenerator generator)
                                             throws IOException {
    switch (WireFormat.getTagWireType(tag)) {
      case WireFormat.WIRETYPE_VARINT:
        generator.print(unsignedToString((Long) value));
        break;
      case WireFormat.WIRETYPE_FIXED32:
        generator.print(
            String.format((Locale) null, "0x%08x", (Integer) value));
        break;
      case WireFormat.WIRETYPE_FIXED64:
        generator.print(String.format((Locale) null, "0x%016x", (Long) value));
        break;
      case WireFormat.WIRETYPE_LENGTH_DELIMITED:
        generator.print("\"");
        generator.print(escapeBytes((ByteString) value));
        generator.print("\"");
        break;
      case WireFormat.WIRETYPE_START_GROUP:
        DEFAULT_PRINTER.printUnknownFields((UnknownFieldSet) value, generator);
        break;
      default:
        throw new IllegalArgumentException("Bad tag: " + tag);
    }
  }

  /** Helper class for converting protobufs to text. */
  private static final class Printer {
    /** Whether to omit newlines from the output. */
    boolean singleLineMode = false;

    /** Whether to escape non ASCII characters with backslash and octal. */
    boolean escapeNonAscii = true;

    private Printer() {}

    /** Setter of singleLineMode */
    private Printer setSingleLineMode(boolean singleLineMode) {
      this.singleLineMode = singleLineMode;
      return this;
    }

    /** Setter of escapeNonAscii */
    private Printer setEscapeNonAscii(boolean escapeNonAscii) {
      this.escapeNonAscii = escapeNonAscii;
      return this;
    }

    private void print(
        final MessageOrBuilder message, final TextGenerator generator)
        throws IOException {
      for (Map.Entry<FieldDescriptor, Object> field
          : message.getAllFields().entrySet()) {
        printField(field.getKey(), field.getValue(), generator);
      }
      printUnknownFields(message.getUnknownFields(), generator);
    }

    private void printField(final FieldDescriptor field, final Object value,
        final TextGenerator generator) throws IOException {
      if (field.isRepeated()) {
        // Repeated field.  Print each element.
        for (Object element : (List<?>) value) {
          printSingleField(field, element, generator);
        }
      } else {
        printSingleField(field, value, generator);
      }
    }

    private void printSingleField(final FieldDescriptor field,
                                  final Object value,
                                  final TextGenerator generator)
                                  throws IOException {
      if (field.isExtension()) {
        generator.print("[");
        // We special-case MessageSet elements for compatibility with proto1.
        if (field.getContainingType().getOptions().getMessageSetWireFormat()
            && (field.getType() == FieldDescriptor.Type.MESSAGE)
            && (field.isOptional())
            // object equality
            && (field.getExtensionScope() == field.getMessageType())) {
          generator.print(field.getMessageType().getFullName());
        } else {
          generator.print(field.getFullName());
        }
        generator.print("]");
      } else {
        if (field.getType() == FieldDescriptor.Type.GROUP) {
          // Groups must be serialized with their original capitalization.
          generator.print(field.getMessageType().getName());
        } else {
          generator.print(field.getName());
        }
      }

      if (field.getJavaType() == FieldDescriptor.JavaType.MESSAGE) {
        if (singleLineMode) {
          generator.print(" { ");
        } else {
          generator.print(" {\n");
          generator.indent();
        }
      } else {
        generator.print(": ");
      }

      printFieldValue(field, value, generator);

      if (field.getJavaType() == FieldDescriptor.JavaType.MESSAGE) {
        if (singleLineMode) {
          generator.print("} ");
        } else {
          generator.outdent();
          generator.print("}\n");
        }
      } else {
        if (singleLineMode) {
          generator.print(" ");
        } else {
          generator.print("\n");
        }
      }
    }

    private void printFieldValue(final FieldDescriptor field,
                                 final Object value,
                                 final TextGenerator generator)
                                 throws IOException {
      switch (field.getType()) {
        case INT32:
        case SINT32:
        case SFIXED32:
          generator.print(((Integer) value).toString());
          break;

        case INT64:
        case SINT64:
        case SFIXED64:
          generator.print(((Long) value).toString());
          break;

        case BOOL:
          generator.print(((Boolean) value).toString());
          break;

        case FLOAT:
          generator.print(((Float) value).toString());
          break;

        case DOUBLE:
          generator.print(((Double) value).toString());
          break;

        case UINT32:
        case FIXED32:
          generator.print(unsignedToString((Integer) value));
          break;

        case UINT64:
        case FIXED64:
          generator.print(unsignedToString((Long) value));
          break;

        case STRING:
          generator.print("\"");
          generator.print(escapeNonAscii
              ? escapeText((String) value)
              : escapeDoubleQuotesAndBackslashes((String) value)
                  .replace("\n", "\\n"));
          generator.print("\"");
          break;

        case BYTES:
          generator.print("\"");
          if (value instanceof ByteString) {
            generator.print(escapeBytes((ByteString) value));
          } else {
            generator.print(escapeBytes((byte[]) value));
          }
          generator.print("\"");
          break;

        case ENUM:
          generator.print(((EnumValueDescriptor) value).getName());
          break;

        case MESSAGE:
        case GROUP:
          print((Message) value, generator);
          break;
      }
    }

    private void printUnknownFields(final UnknownFieldSet unknownFields,
                                    final TextGenerator generator)
                                    throws IOException {
      for (Map.Entry<Integer, UnknownFieldSet.Field> entry :
               unknownFields.asMap().entrySet()) {
        final int number = entry.getKey();
        final UnknownFieldSet.Field field = entry.getValue();
        printUnknownField(number, WireFormat.WIRETYPE_VARINT,
            field.getVarintList(), generator);
        printUnknownField(number, WireFormat.WIRETYPE_FIXED32,
            field.getFixed32List(), generator);
        printUnknownField(number, WireFormat.WIRETYPE_FIXED64,
            field.getFixed64List(), generator);
        printUnknownField(number, WireFormat.WIRETYPE_LENGTH_DELIMITED,
            field.getLengthDelimitedList(), generator);
        for (final UnknownFieldSet value : field.getGroupList()) {
          generator.print(entry.getKey().toString());
          if (singleLineMode) {
            generator.print(" { ");
          } else {
            generator.print(" {\n");
            generator.indent();
          }
          printUnknownFields(value, generator);
          if (singleLineMode) {
            generator.print("} ");
          } else {
            generator.outdent();
            generator.print("}\n");
          }
        }
      }
    }

    private void printUnknownField(final int number,
                                   final int wireType,
                                   final List<?> values,
                                   final TextGenerator generator)
                                   throws IOException {
      for (final Object value : values) {
        generator.print(String.valueOf(number));
        generator.print(": ");
        printUnknownFieldValue(wireType, value, generator);
        generator.print(singleLineMode ? " " : "\n");
      }
    }
  }

  /** Convert an unsigned 32-bit integer to a string. */
  public static String unsignedToString(final int value) {
    if (value >= 0) {
      return Integer.toString(value);
    } else {
      return Long.toString(value & 0x00000000FFFFFFFFL);
    }
  }

  /** Convert an unsigned 64-bit integer to a string. */
  public static String unsignedToString(final long value) {
    if (value >= 0) {
      return Long.toString(value);
    } else {
      // Pull off the most-significant bit so that BigInteger doesn't think
      // the number is negative, then set it again using setBit().
      return BigInteger.valueOf(value & 0x7FFFFFFFFFFFFFFFL)
                       .setBit(63).toString();
    }
  }

  /**
   * An inner class for writing text to the output stream.
   */
  private static final class TextGenerator {
    private final Appendable output;
    private final StringBuilder indent = new StringBuilder();
    private boolean atStartOfLine = true;

    private TextGenerator(final Appendable output) {
      this.output = output;
    }

    /**
     * Indent text by two spaces.  After calling Indent(), two spaces will be
     * inserted at the beginning of each line of text.  Indent() may be called
     * multiple times to produce deeper indents.
     */
    public void indent() {
      indent.append("  ");
    }

    /**
     * Reduces the current indent level by two spaces, or crashes if the indent
     * level is zero.
     */
    public void outdent() {
      final int length = indent.length();
      if (length == 0) {
        throw new IllegalArgumentException(
            " Outdent() without matching Indent().");
      }
      indent.delete(length - 2, length);
    }

    /**
     * Print text to the output stream.
     */
    public void print(final CharSequence text) throws IOException {
      final int size = text.length();
      int pos = 0;

      for (int i = 0; i < size; i++) {
        if (text.charAt(i) == '\n') {
          write(text.subSequence(pos, i + 1));
          pos = i + 1;
          atStartOfLine = true;
        }
      }
      write(text.subSequence(pos, size));
    }

    private void write(final CharSequence data) throws IOException {
      if (data.length() == 0) {
        return;
      }
      if (atStartOfLine) {
        atStartOfLine = false;
        output.append(indent);
      }
      output.append(data);
    }
  }

  // =================================================================
  // Parsing

  /**
   * Represents a stream of tokens parsed from a {@code String}.
   *
   * <p>The Java standard library provides many classes that you might think
   * would be useful for implementing this, but aren't.  For example:
   *
   * <ul>
   * <li>{@code java.io.StreamTokenizer}:  This almost does what we want -- or,
   *   at least, something that would get us close to what we want -- except
   *   for one fatal flaw:  It automatically un-escapes strings using Java
   *   escape sequences, which do not include all the escape sequences we
   *   need to support (e.g. '\x').
   * <li>{@code java.util.Scanner}:  This seems like a great way at least to
   *   parse regular expressions out of a stream (so we wouldn't have to load
   *   the entire input into a single string before parsing).  Sadly,
   *   {@code Scanner} requires that tokens be delimited with some delimiter.
   *   Thus, although the text "foo:" should parse to two tokens ("foo" and
   *   ":"), {@code Scanner} would recognize it only as a single token.
   *   Furthermore, {@code Scanner} provides no way to inspect the contents
   *   of delimiters, making it impossible to keep track of line and column
   *   numbers.
   * </ul>
   *
   * <p>Luckily, Java's regular expression support does manage to be useful to
   * us.  (Barely:  We need {@code Matcher.usePattern()}, which is new in
   * Java 1.5.)  So, we can use that, at least.  Unfortunately, this implies
   * that we need to have the entire input in one contiguous string.
   */
  private static final class Tokenizer {
    private final CharSequence text;
    private final Matcher matcher;
    private String currentToken;

    // The character index within this.text at which the current token begins.
    private int pos = 0;

    // The line and column numbers of the current token.
    private int line = 0;
    private int column = 0;

    // The line and column numbers of the previous token (allows throwing
    // errors *after* consuming).
    private int previousLine = 0;
    private int previousColumn = 0;

    // We use possessive quantifiers (*+ and ++) because otherwise the Java
    // regex matcher has stack overflows on large inputs.
    private static final Pattern WHITESPACE =
      Pattern.compile("(\\s|(#.*$))++", Pattern.MULTILINE);
    private static final Pattern TOKEN = Pattern.compile(
      "[a-zA-Z_][0-9a-zA-Z_+-]*+|" +                // an identifier
      "[.]?[0-9+-][0-9a-zA-Z_.+-]*+|" +             // a number
      "\"([^\"\n\\\\]|\\\\.)*+(\"|\\\\?$)|" +       // a double-quoted string
      "\'([^\'\n\\\\]|\\\\.)*+(\'|\\\\?$)",         // a single-quoted string
      Pattern.MULTILINE);

    private static final Pattern DOUBLE_INFINITY = Pattern.compile(
      "-?inf(inity)?",
      Pattern.CASE_INSENSITIVE);
    private static final Pattern FLOAT_INFINITY = Pattern.compile(
      "-?inf(inity)?f?",
      Pattern.CASE_INSENSITIVE);
    private static final Pattern FLOAT_NAN = Pattern.compile(
      "nanf?",
      Pattern.CASE_INSENSITIVE);

    /** Construct a tokenizer that parses tokens from the given text. */
    private Tokenizer(final CharSequence text) {
      this.text = text;
      this.matcher = WHITESPACE.matcher(text);
      skipWhitespace();
      nextToken();
    }

    /** Are we at the end of the input? */
    public boolean atEnd() {
      return currentToken.length() == 0;
    }

    /** Advance to the next token. */
    public void nextToken() {
      previousLine = line;
      previousColumn = column;

      // Advance the line counter to the current position.
      while (pos < matcher.regionStart()) {
        if (text.charAt(pos) == '\n') {
          ++line;
          column = 0;
        } else {
          ++column;
        }
        ++pos;
      }

      // Match the next token.
      if (matcher.regionStart() == matcher.regionEnd()) {
        // EOF
        currentToken = "";
      } else {
        matcher.usePattern(TOKEN);
        if (matcher.lookingAt()) {
          currentToken = matcher.group();
          matcher.region(matcher.end(), matcher.regionEnd());
        } else {
          // Take one character.
          currentToken = String.valueOf(text.charAt(pos));
          matcher.region(pos + 1, matcher.regionEnd());
        }

        skipWhitespace();
      }
    }

    /**
     * Skip over any whitespace so that the matcher region starts at the next
     * token.
     */
    private void skipWhitespace() {
      matcher.usePattern(WHITESPACE);
      if (matcher.lookingAt()) {
        matcher.region(matcher.end(), matcher.regionEnd());
      }
    }

    /**
     * If the next token exactly matches {@code token}, consume it and return
     * {@code true}.  Otherwise, return {@code false} without doing anything.
     */
    public boolean tryConsume(final String token) {
      if (currentToken.equals(token)) {
        nextToken();
        return true;
      } else {
        return false;
      }
    }

    /**
     * If the next token exactly matches {@code token}, consume it.  Otherwise,
     * throw a {@link ParseException}.
     */
    public void consume(final String token) throws ParseException {
      if (!tryConsume(token)) {
        throw parseException("Expected \"" + token + "\".");
      }
    }

    /**
     * Returns {@code true} if the next token is an integer, but does
     * not consume it.
     */
    public boolean lookingAtInteger() {
      if (currentToken.length() == 0) {
        return false;
      }

      final char c = currentToken.charAt(0);
      return ('0' <= c && c <= '9')
          || c == '-' || c == '+';
    }

    /**
     * Returns {@code true} if the current token's text is equal to that
     * specified.
     */
    public boolean lookingAt(String text) {
      return currentToken.equals(text);
    }

    /**
     * If the next token is an identifier, consume it and return its value.
     * Otherwise, throw a {@link ParseException}.
     */
    public String consumeIdentifier() throws ParseException {
      for (int i = 0; i < currentToken.length(); i++) {
        final char c = currentToken.charAt(i);
        if (('a' <= c && c <= 'z')
            || ('A' <= c && c <= 'Z')
            || ('0' <= c && c <= '9')
            || (c == '_') || (c == '.')) {
          // OK
        } else {
          throw parseException(
              "Expected identifier. Found '" + currentToken + "'");
        }
      }

      final String result = currentToken;
      nextToken();
      return result;
    }

    /**
     * If the next token is an identifier, consume it and return {@code true}.
     * Otherwise, return {@code false} without doing anything.
     */
    public boolean tryConsumeIdentifier() {
      try {
        consumeIdentifier();
        return true;
      } catch (ParseException e) {
        return false;
      }
    }

    /**
     * If the next token is a 32-bit signed integer, consume it and return its
     * value.  Otherwise, throw a {@link ParseException}.
     */
    public int consumeInt32() throws ParseException {
      try {
        final int result = parseInt32(currentToken);
        nextToken();
        return result;
      } catch (NumberFormatException e) {
        throw integerParseException(e);
      }
    }

    /**
     * If the next token is a 32-bit unsigned integer, consume it and return its
     * value.  Otherwise, throw a {@link ParseException}.
     */
    public int consumeUInt32() throws ParseException {
      try {
        final int result = parseUInt32(currentToken);
        nextToken();
        return result;
      } catch (NumberFormatException e) {
        throw integerParseException(e);
      }
    }

    /**
     * If the next token is a 64-bit signed integer, consume it and return its
     * value.  Otherwise, throw a {@link ParseException}.
     */
    public long consumeInt64() throws ParseException {
      try {
        final long result = parseInt64(currentToken);
        nextToken();
        return result;
      } catch (NumberFormatException e) {
        throw integerParseException(e);
      }
    }

    /**
     * If the next token is a 64-bit signed integer, consume it and return
     * {@code true}.  Otherwise, return {@code false} without doing anything.
     */
    public boolean tryConsumeInt64() {
      try {
        consumeInt64();
        return true;
      } catch (ParseException e) {
        return false;
      }
    }

    /**
     * If the next token is a 64-bit unsigned integer, consume it and return its
     * value.  Otherwise, throw a {@link ParseException}.
     */
    public long consumeUInt64() throws ParseException {
      try {
        final long result = parseUInt64(currentToken);
        nextToken();
        return result;
      } catch (NumberFormatException e) {
        throw integerParseException(e);
      }
    }

    /**
     * If the next token is a 64-bit unsigned integer, consume it and return
     * {@code true}.  Otherwise, return {@code false} without doing anything.
     */
    public boolean tryConsumeUInt64() {
      try {
        consumeUInt64();
        return true;
      } catch (ParseException e) {
        return false;
      }
    }

    /**
     * If the next token is a double, consume it and return its value.
     * Otherwise, throw a {@link ParseException}.
     */
    public double consumeDouble() throws ParseException {
      // We need to parse infinity and nan separately because
      // Double.parseDouble() does not accept "inf", "infinity", or "nan".
      if (DOUBLE_INFINITY.matcher(currentToken).matches()) {
        final boolean negative = currentToken.startsWith("-");
        nextToken();
        return negative ? Double.NEGATIVE_INFINITY : Double.POSITIVE_INFINITY;
      }
      if (currentToken.equalsIgnoreCase("nan")) {
        nextToken();
        return Double.NaN;
      }
      try {
        final double result = Double.parseDouble(currentToken);
        nextToken();
        return result;
      } catch (NumberFormatException e) {
        throw floatParseException(e);
      }
    }

    /**
     * If the next token is a double, consume it and return {@code true}.
     * Otherwise, return {@code false} without doing anything.
     */
    public boolean tryConsumeDouble() {
      try {
        consumeDouble();
        return true;
      } catch (ParseException e) {
        return false;
      }
    }

    /**
     * If the next token is a float, consume it and return its value.
     * Otherwise, throw a {@link ParseException}.
     */
    public float consumeFloat() throws ParseException {
      // We need to parse infinity and nan separately because
      // Float.parseFloat() does not accept "inf", "infinity", or "nan".
      if (FLOAT_INFINITY.matcher(currentToken).matches()) {
        final boolean negative = currentToken.startsWith("-");
        nextToken();
        return negative ? Float.NEGATIVE_INFINITY : Float.POSITIVE_INFINITY;
      }
      if (FLOAT_NAN.matcher(currentToken).matches()) {
        nextToken();
        return Float.NaN;
      }
      try {
        final float result = Float.parseFloat(currentToken);
        nextToken();
        return result;
      } catch (NumberFormatException e) {
        throw floatParseException(e);
      }
    }

    /**
     * If the next token is a float, consume it and return {@code true}.
     * Otherwise, return {@code false} without doing anything.
     */
    public boolean tryConsumeFloat() {
      try {
        consumeFloat();
        return true;
      } catch (ParseException e) {
        return false;
      }
    }

    /**
     * If the next token is a boolean, consume it and return its value.
     * Otherwise, throw a {@link ParseException}.
     */
    public boolean consumeBoolean() throws ParseException {
      if (currentToken.equals("true")
          || currentToken.equals("t")
          || currentToken.equals("1")) {
        nextToken();
        return true;
      } else if (currentToken.equals("false")
          || currentToken.equals("f")
          || currentToken.equals("0")) {
        nextToken();
        return false;
      } else {
        throw parseException("Expected \"true\" or \"false\".");
      }
    }

    /**
     * If the next token is a string, consume it and return its (unescaped)
     * value.  Otherwise, throw a {@link ParseException}.
     */
    public String consumeString() throws ParseException {
      return consumeByteString().toStringUtf8();
    }

    /**
     * If the next token is a string, consume it and return true.  Otherwise,
     * return false.
     */
    public boolean tryConsumeString() {
      try {
        consumeString();
        return true;
      } catch (ParseException e) {
        return false;
      }
    }

    /**
     * If the next token is a string, consume it, unescape it as a
     * {@link ByteString}, and return it.  Otherwise, throw a
     * {@link ParseException}.
     */
    public ByteString consumeByteString() throws ParseException {
      List<ByteString> list = new ArrayList<ByteString>();
      consumeByteString(list);
      while (currentToken.startsWith("'") || currentToken.startsWith("\"")) {
        consumeByteString(list);
      }
      return ByteString.copyFrom(list);
    }

    /**
     * Like {@link #consumeByteString()} but adds each token of the string to
     * the given list.  String literals (whether bytes or text) may come in
     * multiple adjacent tokens which are automatically concatenated, like in
     * C or Python.
     */
    private void consumeByteString(List<ByteString> list)
        throws ParseException {
      final char quote = currentToken.length() > 0
          ? currentToken.charAt(0)
          : '\0';
      if (quote != '\"' && quote != '\'') {
        throw parseException("Expected string.");
      }

      if (currentToken.length() < 2
          || currentToken.charAt(currentToken.length() - 1) != quote) {
        throw parseException("String missing ending quote.");
      }

      try {
        final String escaped =
            currentToken.substring(1, currentToken.length() - 1);
        final ByteString result = unescapeBytes(escaped);
        nextToken();
        list.add(result);
      } catch (InvalidEscapeSequenceException e) {
        throw parseException(e.getMessage());
      }
    }

    /**
     * Returns a {@link ParseException} with the current line and column
     * numbers in the description, suitable for throwing.
     */
    public ParseException parseException(final String description) {
      // Note:  People generally prefer one-based line and column numbers.
      return new ParseException(
        line + 1, column + 1, description);
    }

    /**
     * Returns a {@link ParseException} with the line and column numbers of
     * the previous token in the description, suitable for throwing.
     */
    public ParseException parseExceptionPreviousToken(
        final String description) {
      // Note:  People generally prefer one-based line and column numbers.
      return new ParseException(
        previousLine + 1, previousColumn + 1, description);
    }

    /**
     * Constructs an appropriate {@link ParseException} for the given
     * {@code NumberFormatException} when trying to parse an integer.
     */
    private ParseException integerParseException(
        final NumberFormatException e) {
      return parseException("Couldn't parse integer: " + e.getMessage());
    }

    /**
     * Constructs an appropriate {@link ParseException} for the given
     * {@code NumberFormatException} when trying to parse a float or double.
     */
    private ParseException floatParseException(final NumberFormatException e) {
      return parseException("Couldn't parse number: " + e.getMessage());
    }
  }

  /** Thrown when parsing an invalid text format message. */
  public static class ParseException extends IOException {
    private static final long serialVersionUID = 3196188060225107702L;

    private final int line;
    private final int column;

    /** Create a new instance, with -1 as the line and column numbers. */
    public ParseException(final String message) {
      this(-1, -1, message);
    }

    /**
     * Create a new instance
     *
     * @param line the line number where the parse error occurred,
     * using 1-offset.
     * @param column the column number where the parser error occurred,
     * using 1-offset.
     */
    public ParseException(final int line, final int column,
        final String message) {
      super(Integer.toString(line) + ":" + column + ": " + message);
      this.line = line;
      this.column = column;
    }

    /**
     * Return the line where the parse exception occurred, or -1 when
     * none is provided. The value is specified as 1-offset, so the first
     * line is line 1.
     */
    public int getLine() {
      return line;
    }

    /**
     * Return the column where the parse exception occurred, or -1 when
     * none is provided. The value is specified as 1-offset, so the first
     * line is line 1.
     */
    public int getColumn() {
      return column;
    }
  }

  private static final Parser PARSER = Parser.newBuilder().build();

  /**
   * Return a {@link Parser} instance which can parse text-format
   * messages. The returned instance is thread-safe.
   */
  public static Parser getParser() {
    return PARSER;
  }

  /**
   * Parse a text-format message from {@code input} and merge the contents
   * into {@code builder}.
   */
  public static void merge(final Readable input,
                           final Message.Builder builder)
                           throws IOException {
    PARSER.merge(input, builder);
  }

  /**
   * Parse a text-format message from {@code input} and merge the contents
   * into {@code builder}.
   */
  public static void merge(final CharSequence input,
                           final Message.Builder builder)
                           throws ParseException {
    PARSER.merge(input, builder);
  }

  /**
   * Parse a text-format message from {@code input} and merge the contents
   * into {@code builder}.  Extensions will be recognized if they are
   * registered in {@code extensionRegistry}.
   */
  public static void merge(final Readable input,
                           final ExtensionRegistry extensionRegistry,
                           final Message.Builder builder)
                           throws IOException {
    PARSER.merge(input, extensionRegistry, builder);
  }


  /**
   * Parse a text-format message from {@code input} and merge the contents
   * into {@code builder}.  Extensions will be recognized if they are
   * registered in {@code extensionRegistry}.
   */
  public static void merge(final CharSequence input,
                           final ExtensionRegistry extensionRegistry,
                           final Message.Builder builder)
                           throws ParseException {
    PARSER.merge(input, extensionRegistry, builder);
  }


  /**
   * Parser for text-format proto2 instances. This class is thread-safe.
   * The implementation largely follows google/protobuf/text_format.cc.
   *
   * <p>Use {@link TextFormat#getParser()} to obtain the default parser, or
   * {@link Builder} to control the parser behavior.
   */
  public static class Parser {
    /**
     * Determines if repeated values for non-repeated fields and
     * oneofs are permitted. For example, given required/optional field "foo"
     * and a oneof containing "baz" and "qux":
     * <ul>
     * <li>"foo: 1 foo: 2"
     * <li>"baz: 1 qux: 2"
     * <li>merging "foo: 2" into a proto in which foo is already set, or
     * <li>merging "qux: 2" into a proto in which baz is already set.
     * </ul>
     */
    public enum SingularOverwritePolicy {
      /** The last value is retained. */
      ALLOW_SINGULAR_OVERWRITES,
      /** An error is issued. */
      FORBID_SINGULAR_OVERWRITES
    }

    private final boolean allowUnknownFields;
    private final SingularOverwritePolicy singularOverwritePolicy;

    private Parser(boolean allowUnknownFields,
        SingularOverwritePolicy singularOverwritePolicy) {
      this.allowUnknownFields = allowUnknownFields;
      this.singularOverwritePolicy = singularOverwritePolicy;
    }

    /**
     * Returns a new instance of {@link Builder}.
     */
    public static Builder newBuilder() {
      return new Builder();
    }

    /**
     * Builder that can be used to obtain new instances of {@link Parser}.
     */
    public static class Builder {
      private boolean allowUnknownFields = false;
      private SingularOverwritePolicy singularOverwritePolicy =
          SingularOverwritePolicy.ALLOW_SINGULAR_OVERWRITES;


      /**
       * Sets parser behavior when a non-repeated field appears more than once.
       */
      public Builder setSingularOverwritePolicy(SingularOverwritePolicy p) {
        this.singularOverwritePolicy = p;
        return this;
      }

      public Parser build() {
        return new Parser(allowUnknownFields, singularOverwritePolicy);
      }
    }

    /**
     * Parse a text-format message from {@code input} and merge the contents
     * into {@code builder}.
     */
    public void merge(final Readable input,
                      final Message.Builder builder)
                      throws IOException {
      merge(input, ExtensionRegistry.getEmptyRegistry(), builder);
    }

    /**
     * Parse a text-format message from {@code input} and merge the contents
     * into {@code builder}.
     */
    public void merge(final CharSequence input,
                      final Message.Builder builder)
                      throws ParseException {
      merge(input, ExtensionRegistry.getEmptyRegistry(), builder);
    }

    /**
     * Parse a text-format message from {@code input} and merge the contents
     * into {@code builder}.  Extensions will be recognized if they are
     * registered in {@code extensionRegistry}.
     */
    public void merge(final Readable input,
                      final ExtensionRegistry extensionRegistry,
                      final Message.Builder builder)
                      throws IOException {
      // Read the entire input to a String then parse that.

      // If StreamTokenizer were not quite so crippled, or if there were a kind
      // of Reader that could read in chunks that match some particular regex,
      // or if we wanted to write a custom Reader to tokenize our stream, then
      // we would not have to read to one big String.  Alas, none of these is
      // the case.  Oh well.

      merge(toStringBuilder(input), extensionRegistry, builder);
    }


    private static final int BUFFER_SIZE = 4096;

    // TODO(chrisn): See if working around java.io.Reader#read(CharBuffer)
    // overhead is worthwhile
    private static StringBuilder toStringBuilder(final Readable input)
        throws IOException {
      final StringBuilder text = new StringBuilder();
      final CharBuffer buffer = CharBuffer.allocate(BUFFER_SIZE);
      while (true) {
        final int n = input.read(buffer);
        if (n == -1) {
          break;
        }
        buffer.flip();
        text.append(buffer, 0, n);
      }
      return text;
    }

    /**
     * Parse a text-format message from {@code input} and merge the contents
     * into {@code builder}.  Extensions will be recognized if they are
     * registered in {@code extensionRegistry}.
     */
    public void merge(final CharSequence input,
                      final ExtensionRegistry extensionRegistry,
                      final Message.Builder builder)
                      throws ParseException {
      final Tokenizer tokenizer = new Tokenizer(input);
      MessageReflection.BuilderAdapter target =
          new MessageReflection.BuilderAdapter(builder);

      while (!tokenizer.atEnd()) {
        mergeField(tokenizer, extensionRegistry, target);
      }
    }


    /**
     * Parse a single field from {@code tokenizer} and merge it into
     * {@code builder}.
     */
    private void mergeField(final Tokenizer tokenizer,
                            final ExtensionRegistry extensionRegistry,
                            final MessageReflection.MergeTarget target)
                            throws ParseException {
      FieldDescriptor field = null;
      final Descriptor type = target.getDescriptorForType();
      ExtensionRegistry.ExtensionInfo extension = null;

      if (tokenizer.tryConsume("[")) {
        // An extension.
        final StringBuilder name =
            new StringBuilder(tokenizer.consumeIdentifier());
        while (tokenizer.tryConsume(".")) {
          name.append('.');
          name.append(tokenizer.consumeIdentifier());
        }

        extension = target.findExtensionByName(
            extensionRegistry, name.toString());

        if (extension == null) {
          if (!allowUnknownFields) {
            throw tokenizer.parseExceptionPreviousToken(
              "Extension \"" + name + "\" not found in the ExtensionRegistry.");
          } else {
            logger.warning(
              "Extension \"" + name + "\" not found in the ExtensionRegistry.");
          }
        } else {
          if (extension.descriptor.getContainingType() != type) {
            throw tokenizer.parseExceptionPreviousToken(
              "Extension \"" + name + "\" does not extend message type \""
              + type.getFullName() + "\".");
          }
          field = extension.descriptor;
        }

        tokenizer.consume("]");
      } else {
        final String name = tokenizer.consumeIdentifier();
        field = type.findFieldByName(name);

        // Group names are expected to be capitalized as they appear in the
        // .proto file, which actually matches their type names, not their field
        // names.
        if (field == null) {
          // Explicitly specify US locale so that this code does not break when
          // executing in Turkey.
          final String lowerName = name.toLowerCase(Locale.US);
          field = type.findFieldByName(lowerName);
          // If the case-insensitive match worked but the field is NOT a group,
          if (field != null && field.getType() != FieldDescriptor.Type.GROUP) {
            field = null;
          }
        }
        // Again, special-case group names as described above.
        if (field != null && field.getType() == FieldDescriptor.Type.GROUP
            && !field.getMessageType().getName().equals(name)) {
          field = null;
        }

        if (field == null) {
          if (!allowUnknownFields) {
            throw tokenizer.parseExceptionPreviousToken(
              "Message type \"" + type.getFullName()
              + "\" has no field named \"" + name + "\".");
          } else {
            logger.warning(
              "Message type \"" + type.getFullName()
              + "\" has no field named \"" + name + "\".");
          }
        }
      }

      // Skips unknown fields.
      if (field == null) {
        // Try to guess the type of this field.
        // If this field is not a message, there should be a ":" between the
        // field name and the field value and also the field value should not
        // start with "{" or "<" which indicates the beginning of a message body.
        // If there is no ":" or there is a "{" or "<" after ":", this field has
        // to be a message or the input is ill-formed.
        if (tokenizer.tryConsume(":")
            && !tokenizer.lookingAt("{")
            && !tokenizer.lookingAt("<")) {
          skipFieldValue(tokenizer);
        } else {
          skipFieldMessage(tokenizer);
        }
        return;
      }

      // Handle potential ':'.
      if (field.getJavaType() == FieldDescriptor.JavaType.MESSAGE) {
        tokenizer.tryConsume(":");  // optional
      } else {
        tokenizer.consume(":");  // required
      }
      // Support specifying repeated field values as a comma-separated list.
      // Ex."foo: [1, 2, 3]"
      if (field.isRepeated() && tokenizer.tryConsume("[")) {
        while (true) {
          consumeFieldValue(tokenizer, extensionRegistry, target, field, extension);
          if (tokenizer.tryConsume("]")) {
            // End of list.
            break;
          }
          tokenizer.consume(",");
        }
      } else {
        consumeFieldValue(tokenizer, extensionRegistry, target, field, extension);
      }

      // For historical reasons, fields may optionally be separated by commas or
      // semicolons.
      if (!tokenizer.tryConsume(";")) {
        tokenizer.tryConsume(",");
      }
    }

    /**
     * Parse a single field value from {@code tokenizer} and merge it into
     * {@code builder}.
     */
    private void consumeFieldValue(
        final Tokenizer tokenizer,
        final ExtensionRegistry extensionRegistry,
        final MessageReflection.MergeTarget target,
        final FieldDescriptor field,
        final ExtensionRegistry.ExtensionInfo extension)
        throws ParseException {
      Object value = null;

      if (field.getJavaType() == FieldDescriptor.JavaType.MESSAGE) {
        final String endToken;
        if (tokenizer.tryConsume("<")) {
          endToken = ">";
        } else {
          tokenizer.consume("{");
          endToken = "}";
        }

        final MessageReflection.MergeTarget subField;
        subField = target.newMergeTargetForField(field,
            (extension == null) ? null : extension.defaultInstance);

        while (!tokenizer.tryConsume(endToken)) {
          if (tokenizer.atEnd()) {
            throw tokenizer.parseException(
              "Expected \"" + endToken + "\".");
          }
          mergeField(tokenizer, extensionRegistry, subField);
        }

        value = subField.finish();

      } else {
        switch (field.getType()) {
          case INT32:
          case SINT32:
          case SFIXED32:
            value = tokenizer.consumeInt32();
            break;

          case INT64:
          case SINT64:
          case SFIXED64:
            value = tokenizer.consumeInt64();
            break;

          case UINT32:
          case FIXED32:
            value = tokenizer.consumeUInt32();
            break;

          case UINT64:
          case FIXED64:
            value = tokenizer.consumeUInt64();
            break;

          case FLOAT:
            value = tokenizer.consumeFloat();
            break;

          case DOUBLE:
            value = tokenizer.consumeDouble();
            break;

          case BOOL:
            value = tokenizer.consumeBoolean();
            break;

          case STRING:
            value = tokenizer.consumeString();
            break;

          case BYTES:
            value = tokenizer.consumeByteString();
            break;

          case ENUM:
            final EnumDescriptor enumType = field.getEnumType();

            if (tokenizer.lookingAtInteger()) {
              final int number = tokenizer.consumeInt32();
              value = enumType.findValueByNumber(number);
              if (value == null) {
                throw tokenizer.parseExceptionPreviousToken(
                  "Enum type \"" + enumType.getFullName()
                  + "\" has no value with number " + number + '.');
              }
            } else {
              final String id = tokenizer.consumeIdentifier();
              value = enumType.findValueByName(id);
              if (value == null) {
                throw tokenizer.parseExceptionPreviousToken(
                  "Enum type \"" + enumType.getFullName()
                  + "\" has no value named \"" + id + "\".");
              }
            }

            break;

          case MESSAGE:
          case GROUP:
            throw new RuntimeException("Can't get here.");
        }
      }

      if (field.isRepeated()) {
        target.addRepeatedField(field, value);
      } else if ((singularOverwritePolicy
              == SingularOverwritePolicy.FORBID_SINGULAR_OVERWRITES)
          && target.hasField(field)) {
        throw tokenizer.parseExceptionPreviousToken("Non-repeated field \""
            + field.getFullName() + "\" cannot be overwritten.");
      } else if ((singularOverwritePolicy
              == SingularOverwritePolicy.FORBID_SINGULAR_OVERWRITES)
          && field.getContainingOneof() != null
          && target.hasOneof(field.getContainingOneof())) {
        Descriptors.OneofDescriptor oneof = field.getContainingOneof();
        throw tokenizer.parseExceptionPreviousToken("Field \""
            + field.getFullName() + "\" is specified along with field \""
            + target.getOneofFieldDescriptor(oneof).getFullName()
            + "\", another member of oneof \"" + oneof.getName() + "\".");
      } else {
        target.setField(field, value);
      }
    }

    /**
     * Skips the next field including the field's name and value.
     */
    private void skipField(Tokenizer tokenizer) throws ParseException {
      if (tokenizer.tryConsume("[")) {
        // Extension name.
        do {
          tokenizer.consumeIdentifier();
        } while (tokenizer.tryConsume("."));
        tokenizer.consume("]");
      } else {
        tokenizer.consumeIdentifier();
      }

      // Try to guess the type of this field.
      // If this field is not a message, there should be a ":" between the
      // field name and the field value and also the field value should not
      // start with "{" or "<" which indicates the beginning of a message body.
      // If there is no ":" or there is a "{" or "<" after ":", this field has
      // to be a message or the input is ill-formed.
      if (tokenizer.tryConsume(":")
          && !tokenizer.lookingAt("<")
          && !tokenizer.lookingAt("{")) {
        skipFieldValue(tokenizer);
      } else {
        skipFieldMessage(tokenizer);
      }
      // For historical reasons, fields may optionally be separated by commas or
      // semicolons.
      if (!tokenizer.tryConsume(";")) {
        tokenizer.tryConsume(",");
      }
    }

    /**
     * Skips the whole body of a message including the beginning delimiter and
     * the ending delimiter.
     */
    private void skipFieldMessage(Tokenizer tokenizer) throws ParseException {
      final String delimiter;
      if (tokenizer.tryConsume("<")) {
        delimiter = ">";
      } else {
        tokenizer.consume("{");
        delimiter = "}";
      }
      while (!tokenizer.lookingAt(">") && !tokenizer.lookingAt("}")) {
        skipField(tokenizer);
      }
      tokenizer.consume(delimiter);
    }

    /**
     * Skips a field value.
     */
    private void skipFieldValue(Tokenizer tokenizer) throws ParseException {
      if (tokenizer.tryConsumeString()) {
        while (tokenizer.tryConsumeString()) {}
        return;
      }
      if (!tokenizer.tryConsumeIdentifier()   // includes enum & boolean
          && !tokenizer.tryConsumeInt64()     // includes int32
          && !tokenizer.tryConsumeUInt64()    // includes uint32
          && !tokenizer.tryConsumeDouble()
          && !tokenizer.tryConsumeFloat()) {
        throw tokenizer.parseException(
            "Invalid field value: " + tokenizer.currentToken);
      }
    }
  }

  // =================================================================
  // Utility functions
  //
  // Some of these methods are package-private because Descriptors.java uses
  // them.

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
  public static String escapeBytes(final ByteSequence input) {
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
  public static String escapeBytes(final ByteString input) {
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
  public static String escapeBytes(final byte[] input) {
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
   * Un-escape a byte sequence as escaped using
   * {@link #escapeBytes(ByteString)}.  Two-digit hex escapes (starting with
   * "\x") are also recognized.
   */
  public static ByteString unescapeBytes(final CharSequence charString)
      throws InvalidEscapeSequenceException {
    // First convert the Java character sequence to UTF-8 bytes.
    ByteString input = ByteString.copyFromUtf8(charString.toString());
    // Then unescape certain byte sequences introduced by ASCII '\\'.  The valid
    // escapes can all be expressed with ASCII characters, so it is safe to
    // operate on bytes here.
    //
    // Unescaping the input byte array will result in a byte sequence that's no
    // longer than the input.  That's because each escape sequence is between
    // two and four bytes long and stands for a single byte.
    final byte[] result = new byte[input.size()];
    int pos = 0;
    for (int i = 0; i < input.size(); i++) {
      byte c = input.byteAt(i);
      if (c == '\\') {
        if (i + 1 < input.size()) {
          ++i;
          c = input.byteAt(i);
          if (isOctal(c)) {
            // Octal escape.
            int code = digitValue(c);
            if (i + 1 < input.size() && isOctal(input.byteAt(i + 1))) {
              ++i;
              code = code * 8 + digitValue(input.byteAt(i));
            }
            if (i + 1 < input.size() && isOctal(input.byteAt(i + 1))) {
              ++i;
              code = code * 8 + digitValue(input.byteAt(i));
            }
            // TODO: Check that 0 <= code && code <= 0xFF.
            result[pos++] = (byte) code;
          } else {
            switch (c) {
              case 'a' : result[pos++] = 0x07; break;
              case 'b' : result[pos++] = '\b'; break;
              case 'f' : result[pos++] = '\f'; break;
              case 'n' : result[pos++] = '\n'; break;
              case 'r' : result[pos++] = '\r'; break;
              case 't' : result[pos++] = '\t'; break;
              case 'v' : result[pos++] = 0x0b; break;
              case '\\': result[pos++] = '\\'; break;
              case '\'': result[pos++] = '\''; break;
              case '"' : result[pos++] = '\"'; break;

              case 'x':
                // hex escape
                int code = 0;
                if (i + 1 < input.size() && isHex(input.byteAt(i + 1))) {
                  ++i;
                  code = digitValue(input.byteAt(i));
                } else {
                  throw new InvalidEscapeSequenceException(
                      "Invalid escape sequence: '\\x' with no digits");
                }
                if (i + 1 < input.size() && isHex(input.byteAt(i + 1))) {
                  ++i;
                  code = code * 16 + digitValue(input.byteAt(i));
                }
                result[pos++] = (byte) code;
                break;

              default:
                throw new InvalidEscapeSequenceException(
                    "Invalid escape sequence: '\\" + (char) c + '\'');
            }
          }
        } else {
          throw new InvalidEscapeSequenceException(
              "Invalid escape sequence: '\\' at end of string.");
        }
      } else {
        result[pos++] = c;
      }
    }

    return ByteString.copyFrom(result, 0, pos);
  }

  /**
   * Thrown by {@link TextFormat#unescapeBytes} and
   * {@link TextFormat#unescapeText} when an invalid escape sequence is seen.
   */
  public static class InvalidEscapeSequenceException extends IOException {
    private static final long serialVersionUID = -8164033650142593304L;

    InvalidEscapeSequenceException(final String description) {
      super(description);
    }
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
  public static String escapeDoubleQuotesAndBackslashes(final String input) {
    return input.replace("\\", "\\\\").replace("\"", "\\\"");
  }

  /**
   * Un-escape a text string as escaped using {@link #escapeText(String)}.
   * Two-digit hex escapes (starting with "\x") are also recognized.
   */
  static String unescapeText(final String input)
                             throws InvalidEscapeSequenceException {
    return unescapeBytes(input).toStringUtf8();
  }

  /** Is this an octal digit? */
  private static boolean isOctal(final byte c) {
    return '0' <= c && c <= '7';
  }

  /** Is this a hex digit? */
  private static boolean isHex(final byte c) {
    return ('0' <= c && c <= '9')
        || ('a' <= c && c <= 'f')
        || ('A' <= c && c <= 'F');
  }

  /**
   * Interpret a character as a digit (in any base up to 36) and return the
   * numeric value.  This is like {@code Character.digit()} but we don't accept
   * non-ASCII digits.
   */
  private static int digitValue(final byte c) {
    if ('0' <= c && c <= '9') {
      return c - '0';
    } else if ('a' <= c && c <= 'z') {
      return c - 'a' + 10;
    } else {
      return c - 'A' + 10;
    }
  }

  /**
   * Parse a 32-bit signed integer from the text.  Unlike the Java standard
   * {@code Integer.parseInt()}, this function recognizes the prefixes "0x"
   * and "0" to signify hexadecimal and octal numbers, respectively.
   */
  static int parseInt32(final String text) throws NumberFormatException {
    return (int) parseInteger(text, true, false);
  }

  /**
   * Parse a 32-bit unsigned integer from the text.  Unlike the Java standard
   * {@code Integer.parseInt()}, this function recognizes the prefixes "0x"
   * and "0" to signify hexadecimal and octal numbers, respectively.  The
   * result is coerced to a (signed) {@code int} when returned since Java has
   * no unsigned integer type.
   */
  static int parseUInt32(final String text) throws NumberFormatException {
    return (int) parseInteger(text, false, false);
  }

  /**
   * Parse a 64-bit signed integer from the text.  Unlike the Java standard
   * {@code Integer.parseInt()}, this function recognizes the prefixes "0x"
   * and "0" to signify hexadecimal and octal numbers, respectively.
   */
  static long parseInt64(final String text) throws NumberFormatException {
    return parseInteger(text, true, true);
  }

  /**
   * Parse a 64-bit unsigned integer from the text.  Unlike the Java standard
   * {@code Integer.parseInt()}, this function recognizes the prefixes "0x"
   * and "0" to signify hexadecimal and octal numbers, respectively.  The
   * result is coerced to a (signed) {@code long} when returned since Java has
   * no unsigned long type.
   */
  static long parseUInt64(final String text) throws NumberFormatException {
    return parseInteger(text, false, true);
  }

  private static long parseInteger(final String text,
                                   final boolean isSigned,
                                   final boolean isLong)
                                   throws NumberFormatException {
    int pos = 0;

    boolean negative = false;
    if (text.startsWith("-", pos)) {
      if (!isSigned) {
        throw new NumberFormatException("Number must be positive: " + text);
      }
      ++pos;
      negative = true;
    }

    int radix = 10;
    if (text.startsWith("0x", pos)) {
      pos += 2;
      radix = 16;
    } else if (text.startsWith("0", pos)) {
      radix = 8;
    }

    final String numberText = text.substring(pos);

    long result = 0;
    if (numberText.length() < 16) {
      // Can safely assume no overflow.
      result = Long.parseLong(numberText, radix);
      if (negative) {
        result = -result;
      }

      // Check bounds.
      // No need to check for 64-bit numbers since they'd have to be 16 chars
      // or longer to overflow.
      if (!isLong) {
        if (isSigned) {
          if (result > Integer.MAX_VALUE || result < Integer.MIN_VALUE) {
            throw new NumberFormatException(
              "Number out of range for 32-bit signed integer: " + text);
          }
        } else {
          if (result >= (1L << 32) || result < 0) {
            throw new NumberFormatException(
              "Number out of range for 32-bit unsigned integer: " + text);
          }
        }
      }
    } else {
      BigInteger bigValue = new BigInteger(numberText, radix);
      if (negative) {
        bigValue = bigValue.negate();
      }

      // Check bounds.
      if (!isLong) {
        if (isSigned) {
          if (bigValue.bitLength() > 31) {
            throw new NumberFormatException(
              "Number out of range for 32-bit signed integer: " + text);
          }
        } else {
          if (bigValue.bitLength() > 32) {
            throw new NumberFormatException(
              "Number out of range for 32-bit unsigned integer: " + text);
          }
        }
      } else {
        if (isSigned) {
          if (bigValue.bitLength() > 63) {
            throw new NumberFormatException(
              "Number out of range for 64-bit signed integer: " + text);
          }
        } else {
          if (bigValue.bitLength() > 64) {
            throw new NumberFormatException(
              "Number out of range for 64-bit unsigned integer: " + text);
          }
        }
      }

      result = bigValue.longValue();
    }

    return result;
  }
}
