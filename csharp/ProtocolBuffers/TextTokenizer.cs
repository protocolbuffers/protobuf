// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.
// http://code.google.com/p/protobuf/
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
using System;
using System.Globalization;
using System.Text.RegularExpressions;

namespace Google.ProtocolBuffers {
  /// <summary>
  /// Represents a stream of tokens parsed from a string.
  /// </summary>
  internal sealed class TextTokenizer {
    private readonly string text;
    private string currentToken;

    /// <summary>
    /// The character index within the text to perform the next regex match at.
    /// </summary>
    private int matchPos = 0;

    /// <summary>
    /// The character index within the text at which the current token begins.
    /// </summary>
    private int pos = 0;

    /// <summary>
    /// The line number of the current token.
    /// </summary>
    private int line = 0;
    /// <summary>
    /// The column number of the current token.
    /// </summary>
    private int column = 0;

    /// <summary>
    /// The line number of the previous token.
    /// </summary>
    private int previousLine = 0;
    /// <summary>
    /// The column number of the previous token.
    /// </summary>
    private int previousColumn = 0;

    private static Regex WhitespaceAndCommentPattern = new Regex("\\G(\\s|(#[^\\\n]*\\n))+", RegexOptions.Compiled);
    private static Regex TokenPattern = new Regex(
      "\\G[a-zA-Z_][0-9a-zA-Z_+-]*|" +              // an identifier
      "\\G[0-9+-][0-9a-zA-Z_.+-]*|" +                  // a number
      "\\G\"([^\"\\\n\\\\]|\\\\[^\\\n])*(\"|\\\\?$)|" +    // a double-quoted string
      "\\G\'([^\"\\\n\\\\]|\\\\[^\\\n])*(\'|\\\\?$)",      // a single-quoted string
      RegexOptions.Compiled);

    /** Construct a tokenizer that parses tokens from the given text. */
    public TextTokenizer(string text) {
      this.text = text;
      SkipWhitespace();
      NextToken();
    }

    /// <summary>
    /// Are we at the end of the input?
    /// </summary>
    public bool AtEnd {
      get { return currentToken.Length == 0; }
    }

    /// <summary>
    /// Advances to the next token.
    /// </summary>
    public void NextToken() {
      previousLine = line;
      previousColumn = column;

      // Advance the line counter to the current position.
      while (pos < matchPos) {
        if (text[pos] == '\n') {
          ++line;
          column = 0;
        } else {
          ++column;
        }
        ++pos;
      }

      // Match the next token.
      if (matchPos == text.Length) {
        // EOF
        currentToken = "";
      } else {
        Match match = TokenPattern.Match(text, matchPos);
        if (match.Success) {
          currentToken = match.Value;
          matchPos += match.Length;
        } else {
          // Take one character.
          currentToken = text[matchPos].ToString();
          matchPos++;
        }

        SkipWhitespace();
      }
    }

    /// <summary>
    /// Skip over any whitespace so that matchPos starts at the next token.
    /// </summary>
    private void SkipWhitespace() {
      Match match = WhitespaceAndCommentPattern.Match(text, matchPos);
      if (match.Success) {
        matchPos += match.Length;
      }
    }

    /// <summary>
    /// If the next token exactly matches the given token, consume it and return
    /// true. Otherwise, return false without doing anything.
    /// </summary>
    public bool TryConsume(string token) {
      if (currentToken == token) {
        NextToken();
        return true;
      }
      return false;
    }

    /*
     * If the next token exactly matches {@code token}, consume it.  Otherwise,
     * throw a {@link ParseException}.
     */
    /// <summary>
    /// If the next token exactly matches the specified one, consume it.
    /// Otherwise, throw a FormatException.
    /// </summary>
    /// <param name="token"></param>
    public void Consume(string token) {
      if (!TryConsume(token)) {
        throw CreateFormatException("Expected \"" + token + "\".");
      }
    }

    /// <summary>
    /// Returns true if the next token is an integer, but does not consume it.
    /// </summary>
    public bool LookingAtInteger() {
      if (currentToken.Length == 0) {
        return false;
      }

      char c = currentToken[0];
      return ('0' <= c && c <= '9') || c == '-' || c == '+';
    }

    /// <summary>
    /// If the next token is an identifier, consume it and return its value.
    /// Otherwise, throw a FormatException.
    /// </summary>
    public string ConsumeIdentifier() {
      foreach (char c in currentToken) {
        if (('a' <= c && c <= 'z') ||
            ('A' <= c && c <= 'Z') ||
            ('0' <= c && c <= '9') ||
            (c == '_') || (c == '.')) {
          // OK
        } else {
          throw CreateFormatException("Expected identifier.");
        }
      }

      string result = currentToken;
      NextToken();
      return result;
    }

    /// <summary>
    /// If the next token is a 32-bit signed integer, consume it and return its 
    /// value. Otherwise, throw a FormatException.
    /// </summary>
    public int ConsumeInt32()  {
      try {
        int result = TextFormat.ParseInt32(currentToken);
        NextToken();
        return result;
      } catch (FormatException e) {
        throw CreateIntegerParseException(e);
      }
    }

    /// <summary>
    /// If the next token is a 32-bit unsigned integer, consume it and return its
    /// value. Otherwise, throw a FormatException.
    /// </summary>
    public uint ConsumeUInt32() {
      try {
        uint result = TextFormat.ParseUInt32(currentToken);
        NextToken();
        return result;
      } catch (FormatException e) {
        throw CreateIntegerParseException(e);
      }
    }

    /// <summary>
    /// If the next token is a 64-bit signed integer, consume it and return its
    /// value. Otherwise, throw a FormatException.
    /// </summary>
    public long ConsumeInt64() {
      try {
        long result = TextFormat.ParseInt64(currentToken);
        NextToken();
        return result;
      } catch (FormatException e) {
        throw CreateIntegerParseException(e);
      }
    }

    /// <summary>
    /// If the next token is a 64-bit unsigned integer, consume it and return its
    /// value. Otherwise, throw a FormatException.
    /// </summary>
    public ulong ConsumeUInt64() {
      try {
        ulong result = TextFormat.ParseUInt64(currentToken);
        NextToken();
        return result;
      } catch (FormatException e) {
        throw CreateIntegerParseException(e);
      }
    }

    /// <summary>
    /// If the next token is a double, consume it and return its value.
    /// Otherwise, throw a FormatException.
    /// </summary>
    public double ConsumeDouble() {
      try {
        double result = double.Parse(currentToken, CultureInfo.InvariantCulture);
        NextToken();
        return result;
      } catch (FormatException e) {
        throw CreateFloatParseException(e);
      } catch (OverflowException e) {
        throw CreateFloatParseException(e);
      }
    }

    /// <summary>
    /// If the next token is a float, consume it and return its value.
    /// Otherwise, throw a FormatException.
    /// </summary>
    public float consumeFloat() {
      try {
        float result = float.Parse(currentToken, CultureInfo.InvariantCulture);
        NextToken();
        return result;
      } catch (FormatException e) {
        throw CreateFloatParseException(e);
      } catch (OverflowException e) {
        throw CreateFloatParseException(e);
      }
    }

    /// <summary>
    /// If the next token is a Boolean, consume it and return its value.
    /// Otherwise, throw a FormatException.    
    /// </summary>
    public bool ConsumeBoolean() {
      if (currentToken == "true") {
        NextToken();
        return true;
      } 
      if (currentToken == "false") {
        NextToken();
        return false;
      }
      throw CreateFormatException("Expected \"true\" or \"false\".");
    }

    /// <summary>
    /// If the next token is a string, consume it and return its (unescaped) value.
    /// Otherwise, throw a FormatException.
    /// </summary>
    public string ConsumeString() {
      return ConsumeByteString().ToStringUtf8();
    }

    /// <summary>
    /// If the next token is a string, consume it, unescape it as a
    /// ByteString and return it. Otherwise, throw a FormatException.
    /// </summary>
    public ByteString ConsumeByteString() {
      char quote = currentToken.Length > 0 ? currentToken[0] : '\0';
      if (quote != '\"' && quote != '\'') {
        throw CreateFormatException("Expected string.");
      }

      if (currentToken.Length < 2 ||
          currentToken[currentToken.Length-1] != quote) {
        throw CreateFormatException("String missing ending quote.");
      }

      try {
        string escaped = currentToken.Substring(1, currentToken.Length - 2);
        ByteString result = TextFormat.UnescapeBytes(escaped);
        NextToken();
        return result;
      } catch (FormatException e) {
        throw CreateFormatException(e.Message);
      }
    }

    /// <summary>
    /// Returns a format exception with the current line and column numbers
    /// in the description, suitable for throwing.
    /// </summary>
    public FormatException CreateFormatException(string description) {
      // Note:  People generally prefer one-based line and column numbers.
      return new FormatException((line + 1) + ":" + (column + 1) + ": " + description);
    }

    /// <summary>
    /// Returns a format exception with the line and column numbers of the
    /// previous token in the description, suitable for throwing.
    /// </summary>
    public FormatException CreateFormatExceptionPreviousToken(string description) {
      // Note:  People generally prefer one-based line and column numbers.
      return new FormatException((previousLine + 1) + ":" + (previousColumn + 1) + ": " + description);
    }

    /// <summary>
    /// Constructs an appropriate FormatException for the given existing exception
    /// when trying to parse an integer.
    /// </summary>
    private FormatException CreateIntegerParseException(FormatException e) {
      return CreateFormatException("Couldn't parse integer: " + e.Message);
    }

    /// <summary>
    /// Constructs an appropriate FormatException for the given existing exception
    /// when trying to parse a float or double.
    /// </summary>
    private FormatException CreateFloatParseException(Exception e) {
      return CreateFormatException("Couldn't parse number: " + e.Message);
    }
  }
}
