// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

package com.google.protobuf;

import java.util.ArrayList;
import java.util.List;
import java.util.Locale;

/**
 * Represents a stream of tokens parsed from a {@code String}.
 *
 * <p>The Java standard library provides many classes that you might think would be useful for
 * implementing this, but aren't. For example:
 *
 * <ul>
 *   <li>{@code java.io.StreamTokenizer}: This almost does what we want -- or, at least, something
 *       that would get us close to what we want -- except for one fatal flaw: It automatically
 *       un-escapes strings using Java escape sequences, which do not include all the escape
 *       sequences we need to support (e.g. '\x').
 *   <li>{@code java.util.Scanner}: This seems like a great way at least to parse regular
 *       expressions out of a stream (so we wouldn't have to load the entire input into a single
 *       string before parsing). Sadly, {@code Scanner} requires that tokens be delimited with some
 *       delimiter. Thus, although the text "foo:" should parse to two tokens ("foo" and ":"),
 *       {@code Scanner} would recognize it only as a single token. Furthermore, {@code Scanner}
 *       provides no way to inspect the contents of delimiters, making it impossible to keep track
 *       of line and column numbers.
 * </ul>
 */
public final class TxtpbTokenizer {
  private static final String DEBUG_STRING_SILENT_MARKER = " \t ";

  private final CharSequence text;
  // The token that has been ingested by nextToken(), but not consumed yet.
  private String currentToken;

  // The character index within this.text at which the next token will be ingested.
  private int pos = 0;

  // The line and column numbers of the current token.
  private int line = 0;
  private int column = 0;
  private int lineInfoTrackingPos = 0;

  // The line and column numbers of the previous token (allows throwing
  // errors *after* consuming).
  private int previousLine = 0;
  private int previousColumn = 0;
  // Start index of currentToken.
  private int previousPos = 0;

  /**
   * {@link containsSilentMarkerAfterCurrentToken} indicates if there is a silent marker after the
   * current token. This value is moved to {@link containsSilentMarkerAfterPrevToken} every time the
   * next token is parsed.
   */
  private boolean containsSilentMarkerAfterCurrentToken = false;

  private boolean containsSilentMarkerAfterPrevToken = false;

  /** Construct a tokenizer that parses tokens from the given text. */
  public TxtpbTokenizer(final CharSequence text) {
    this.text = text;
    skipWhitespace();
    nextToken();
  }

  public String getCurrentToken() {
    return currentToken;
  }

  public int getPreviousLine() {
    return previousLine;
  }

  public int getPreviousColumn() {
    return previousColumn;
  }

  public int getPreviousPos() {
    return previousPos;
  }

  public int getLine() {
    return line;
  }

  public int getColumn() {
    return column;
  }

  boolean getContainsSilentMarkerAfterCurrentToken() {
    return containsSilentMarkerAfterCurrentToken;
  }

  boolean getContainsSilentMarkerAfterPrevToken() {
    return containsSilentMarkerAfterPrevToken;
  }

  /** Are we at the end of the input? */
  public boolean atEnd() {
    return currentToken.isEmpty();
  }

  /** Advance to the next token. */
  public void nextToken() {
    previousLine = line;
    previousColumn = column;
    previousPos = pos;

    // Advance the line counter to the current position.
    while (lineInfoTrackingPos < pos) {
      if (text.charAt(lineInfoTrackingPos) == '\n') {
        ++line;
        column = 0;
      } else {
        ++column;
      }
      ++lineInfoTrackingPos;
    }

    // Match the next token.
    if (pos == text.length()) {
      currentToken = ""; // EOF
    } else {
      currentToken = nextTokenInternal();
      skipWhitespace();
    }
  }

  private String nextTokenInternal() {
    final int textLength = this.text.length();
    final int startPos = this.pos;
    final char startChar = this.text.charAt(startPos);

    int endPos = pos;
    if (isAlphaUnder(startChar)) { // Identifier
      while (++endPos != textLength) {
        char c = this.text.charAt(endPos);
        if (!(isAlphaUnder(c) || isDigitPlusMinus(c))) {
          break;
        }
      }
    } else if (isDigitPlusMinus(startChar) || startChar == '.') { // Number
      if (startChar == '.') { // Optional leading dot
        if (++endPos == textLength) {
          return nextTokenSingleChar();
        }

        if (!isDigitPlusMinus(this.text.charAt(endPos))) { // Mandatory first digit
          return nextTokenSingleChar();
        }
      }

      while (++endPos != textLength) {
        char c = this.text.charAt(endPos);
        if (!(isDigitPlusMinus(c) || isAlphaUnder(c) || c == '.')) {
          break;
        }
      }
    } else if (startChar == '"' || startChar == '\'') { // String
      while (++endPos != textLength) {
        char c = this.text.charAt(endPos);
        if (c == startChar) {
          ++endPos;
          break; // Quote terminates
        } else if (c == '\n') {
          break; // Newline terminates (error during parsing) (not consumed)
        } else if (c == '\\') {
          if (++endPos == textLength) {
            break; // Escape into end-of-text terminates (error during parsing)
          } else if (this.text.charAt(endPos) == '\n') {
            break; // Escape into newline terminates (error during parsing) (not consumed)
          } else {
            // Otherwise the escaped char is legal and consumed
          }
        } else {
          // Otherwise the char is a legal and consumed
        }
      }
    } else {
      return nextTokenSingleChar(); // Unrecognized start character
    }

    this.pos = endPos;
    return this.text.subSequence(startPos, endPos).toString();
  }

  private static boolean isAlphaUnder(char c) {
    // Defining this char-class with numeric comparisons is much faster than using a regex.
    return ('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z') || c == '_';
  }

  private static boolean isDigitPlusMinus(char c) {
    // Defining this char-class with numeric comparisons is much faster than using a regex.
    return ('0' <= c && c <= '9') || c == '+' || c == '-';
  }

  private static boolean isWhitespace(char c) {
    // Defining this char-class with numeric comparisons is much faster than using a regex.
    return c == ' ' || c == '\f' || c == '\n' || c == '\r' || c == '\t';
  }

  /**
   * Produce a token for the single char at the current position.
   *
   * <p>We hardcode the expected single-char tokens to avoid allocating a unique string every time,
   * which is a GC risk. String-literals are always loaded from the class constant pool.
   *
   * <p>This method must not be called if the current position is after the end-of-text.
   */
  private String nextTokenSingleChar() {
    final char c = this.text.charAt(this.pos++);
    switch (c) {
      case ':':
        return ":";
      case ',':
        return ",";
      case '[':
        return "[";
      case ']':
        return "]";
      case '{':
        return "{";
      case '}':
        return "}";
      case '<':
        return "<";
      case '>':
        return ">";
      default:
        // If we don't recognize the char, create a string and let the parser report any errors
        return String.valueOf(c);
    }
  }

  /** Skip over any whitespace so that the matcher region starts at the next token. */
  private void skipWhitespace() {
    final int textLength = this.text.length();
    final int startPos = this.pos;

    int endPos = this.pos - 1;
    while (++endPos != textLength) {
      char c = this.text.charAt(endPos);
      if (c == '#') {
        while (++endPos != textLength) {
          if (this.text.charAt(endPos) == '\n') {
            break; // Consume the newline as whitespace.
          }
        }
        if (endPos == textLength) {
          break;
        }
      } else if (isWhitespace(c)) {
        // OK
      } else {
        break;
      }
    }

    this.pos = endPos;
  }

  /**
   * If the next token exactly matches {@code token}, consume it and return {@code true}. Otherwise,
   * return {@code false} without doing anything.
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
   * If the next token exactly matches {@code token}, consume it. Otherwise, throw a {@link
   * TextFormat.ParseException}.
   */
  public void consume(final String token) throws TextFormat.ParseException {
    if (!tryConsume(token)) {
      throw parseException("Expected \"" + token + "\".");
    }
  }

  /** Returns {@code true} if the next token is an integer, but does not consume it. */
  boolean lookingAtInteger() {
    if (currentToken.isEmpty()) {
      return false;
    }

    return isDigitPlusMinus(currentToken.charAt(0));
  }

  /** Returns {@code true} if the current token's text is equal to that specified. */
  boolean lookingAt(String text) {
    return currentToken.equals(text);
  }

  /**
   * If the next token is an identifier, consume it and return its value. Otherwise, throw a {@link
   * TextFormat.ParseException}.
   */
  public String consumeIdentifier() throws TextFormat.ParseException {
    for (int i = 0; i < currentToken.length(); i++) {
      final char c = currentToken.charAt(i);
      if (isAlphaUnder(c) || ('0' <= c && c <= '9') || (c == '.')) {
        // OK
      } else {
        throw parseException("Expected identifier. Found '" + currentToken + "'");
      }
    }

    final String result = currentToken;
    nextToken();
    return result;
  }

  /**
   * If the next token is an identifier, consume it and return {@code true}. Otherwise, return
   * {@code false} without doing anything.
   */
  public boolean tryConsumeIdentifier() {
    try {
      String unused = consumeIdentifier();
      return true;
    } catch (TextFormat.ParseException e) {
      return false;
    }
  }

  /**
   * If the next token is a 32-bit signed integer, consume it and return its value. Otherwise, throw
   * a {@link TextFormat.ParseException}.
   */
  public int consumeInt32() throws TextFormat.ParseException {
    try {
      final int result = TextFormat.parseInt32(currentToken);
      nextToken();
      return result;
    } catch (NumberFormatException e) {
      throw integerParseException(e);
    }
  }

  /**
   * If the next token is a 32-bit unsigned integer, consume it and return its value. Otherwise,
   * throw a {@link TextFormat.ParseException}.
   */
  public int consumeUInt32() throws TextFormat.ParseException {
    try {
      final int result = TextFormat.parseUInt32(currentToken);
      nextToken();
      return result;
    } catch (NumberFormatException e) {
      throw integerParseException(e);
    }
  }

  /**
   * If the next token is a 64-bit signed integer, consume it and return its value. Otherwise, throw
   * a {@link TextFormat.ParseException}.
   */
  public long consumeInt64() throws TextFormat.ParseException {
    try {
      final long result = TextFormat.parseInt64(currentToken);
      nextToken();
      return result;
    } catch (NumberFormatException e) {
      throw integerParseException(e);
    }
  }

  /**
   * If the next token is a 64-bit signed integer, consume it and return {@code true}. Otherwise,
   * return {@code false} without doing anything.
   */
  public boolean tryConsumeInt64() {
    try {
      long unused = consumeInt64();
      return true;
    } catch (TextFormat.ParseException e) {
      return false;
    }
  }

  /**
   * If the next token is a 64-bit unsigned integer, consume it and return its value. Otherwise,
   * throw a {@link TextFormat.ParseException}.
   */
  public long consumeUInt64() throws TextFormat.ParseException {
    try {
      final long result = TextFormat.parseUInt64(currentToken);
      nextToken();
      return result;
    } catch (NumberFormatException e) {
      throw integerParseException(e);
    }
  }

  /**
   * If the next token is a 64-bit unsigned integer, consume it and return {@code true}. Otherwise,
   * return {@code false} without doing anything.
   */
  public boolean tryConsumeUInt64() {
    try {
      long unused = consumeUInt64();
      return true;
    } catch (TextFormat.ParseException e) {
      return false;
    }
  }

  /**
   * If the next token is a double, consume it and return its value. Otherwise, throw a {@link
   * TextFormat.ParseException}.
   */
  public double consumeDouble() throws TextFormat.ParseException {
    // We need to parse infinity and nan separately because
    // Double.parseDouble() does not accept "inf", "infinity", or "nan".
    switch (currentToken.toLowerCase(Locale.ROOT)) {
      case "-inf":
      case "-infinity":
        nextToken();
        return Double.NEGATIVE_INFINITY;
      case "inf":
      case "infinity":
        nextToken();
        return Double.POSITIVE_INFINITY;
      case "nan":
        nextToken();
        return Double.NaN;
      default:
        // fall through
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
   * If the next token is a double, consume it and return {@code true}. Otherwise, return {@code
   * false} without doing anything.
   */
  public boolean tryConsumeDouble() {
    try {
      double unused = consumeDouble();
      return true;
    } catch (TextFormat.ParseException e) {
      return false;
    }
  }

  /**
   * If the next token is a float, consume it and return its value. Otherwise, throw a {@link
   * TextFormat.ParseException}.
   */
  public float consumeFloat() throws TextFormat.ParseException {
    // We need to parse infinity and nan separately because
    // Float.parseFloat() does not accept "inf", "infinity", or "nan".
    switch (currentToken.toLowerCase(Locale.ROOT)) {
      case "-inf":
      case "-inff":
      case "-infinity":
      case "-infinityf":
        nextToken();
        return Float.NEGATIVE_INFINITY;
      case "inf":
      case "inff":
      case "infinity":
      case "infinityf":
        nextToken();
        return Float.POSITIVE_INFINITY;
      case "nan":
      case "nanf":
        nextToken();
        return Float.NaN;
      default:
        // fall through
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
   * If the next token is a float, consume it and return {@code true}. Otherwise, return {@code
   * false} without doing anything.
   */
  public boolean tryConsumeFloat() {
    try {
      float unused = consumeFloat();
      return true;
    } catch (TextFormat.ParseException e) {
      return false;
    }
  }

  /**
   * If the next token is a boolean, consume it and return its value. Otherwise, throw a {@link
   * TextFormat.ParseException}.
   */
  public boolean consumeBoolean() throws TextFormat.ParseException {
    if (currentToken.equals("true")
        || currentToken.equals("True")
        || currentToken.equals("t")
        || currentToken.equals("1")) {
      nextToken();
      return true;
    } else if (currentToken.equals("false")
        || currentToken.equals("False")
        || currentToken.equals("f")
        || currentToken.equals("0")) {
      nextToken();
      return false;
    } else {
      throw parseException("Expected \"true\" or \"false\". Found \"" + currentToken + "\".");
    }
  }

  /**
   * If the next token is a string, consume it and return its (unescaped) value. Otherwise, throw a
   * {@link TextFormat.ParseException}.
   */
  public String consumeString() throws TextFormat.ParseException {
    return consumeByteString().toStringUtf8();
  }

  /**
   * If the next token is a string, consume it, unescape it as a {@link ByteString}, and return it.
   * Otherwise, throw a {@link TextFormat.ParseException}.
   */
  @CanIgnoreReturnValue
  public ByteString consumeByteString() throws TextFormat.ParseException {
    ArrayList<ByteString> list = new ArrayList<ByteString>();
    consumeByteString(list);
    while (currentToken.startsWith("'") || currentToken.startsWith("\"")) {
      consumeByteString(list);
    }
    return ByteString.copyFrom(list);
  }

  /**
   * Like {@link #consumeByteString()} but adds each token of the string to the given list. String
   * literals (whether bytes or text) may come in multiple adjacent tokens which are automatically
   * concatenated, like in C or Python.
   */
  private void consumeByteString(List<ByteString> list) throws TextFormat.ParseException {
    final char quote = currentToken.length() > 0 ? currentToken.charAt(0) : '\0';
    if (quote != '\"' && quote != '\'') {
      throw parseException("Expected string.");
    }

    if (currentToken.length() < 2 || currentToken.charAt(currentToken.length() - 1) != quote) {
      throw parseException("String missing ending quote.");
    }

    try {
      final String escaped = currentToken.substring(1, currentToken.length() - 1);
      final ByteString result = TextFormat.unescapeBytes(escaped);
      nextToken();
      list.add(result);
    } catch (TextFormat.InvalidEscapeSequenceException e) {
      throw parseException(e.getMessage());
    }
  }

  /** If the next token is a string, consume it and return true. Otherwise, return false. */
  public boolean tryConsumeByteString() {
    try {
      consumeByteString();
      return true;
    } catch (TextFormat.ParseException e) {
      return false;
    }
  }

  /**
   * Returns a {@link TextFormat.ParseException} with the current line and column numbers in the
   * description, suitable for throwing.
   */
  public TextFormat.ParseException parseException(final String description) {
    // Note:  People generally prefer one-based line and column numbers.
    return new TextFormat.ParseException(line + 1, column + 1, description);
  }

  /**
   * Returns a {@link TextFormat.ParseException} with the line and column numbers of the previous
   * token in the description, suitable for throwing.
   */
  TextFormat.ParseException parseExceptionPreviousToken(final String description) {
    // Note:  People generally prefer one-based line and column numbers.
    return new TextFormat.ParseException(previousLine + 1, previousColumn + 1, description);
  }

  /**
   * Constructs an appropriate {@link TextFormat.ParseException} for the given {@code
   * NumberFormatException} when trying to parse an integer.
   */
  private TextFormat.ParseException integerParseException(final NumberFormatException e) {
    return parseException("Couldn't parse integer: " + e.getMessage());
  }

  /**
   * Constructs an appropriate {@link TextFormat.ParseException} for the given {@code
   * NumberFormatException} when trying to parse a float or double.
   */
  private TextFormat.ParseException floatParseException(final NumberFormatException e) {
    return parseException("Couldn't parse number: " + e.getMessage());
  }
}
