#region Copyright notice and license
// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd
#endregion

using System;
using System.Collections.Generic;
using System.IO;
using System.Text;

namespace Google.Protobuf
{
    /// <summary>
    /// Tokenizer for the Protocol Buffers text format.
    /// </summary>
    /// <remarks>
    /// <para>
    /// This tokenizer is stateful and supports single-token lookahead. It performs lexical
    /// validation only (for example, that string literals are correctly escaped); structural
    /// validation is the responsibility of <see cref="TextParser"/>.
    /// </para>
    /// <para>
    /// Numeric literals are returned with their raw source text so that the parser can apply
    /// type-specific range checks. String literals are unescaped into raw bytes, and adjacent
    /// string literals are concatenated into a single token.
    /// </para>
    /// <para>Not thread-safe.</para>
    /// </remarks>
    internal sealed class TextTokenizer
    {
        private const int EndOfInput = -1;

        private readonly TextReader reader;
        private TextToken? bufferedToken;
        // Single-character push-back for the underlying reader. -2 means "nothing pushed back".
        private int pushedBackChar = -2;
        // 1-based line and column of the most recently read character, for error messages.
        private int line = 1;
        private int column = 0;

        internal TextTokenizer(TextReader reader)
        {
            this.reader = ProtoPreconditions.CheckNotNull(reader, nameof(reader));
        }

        /// <summary>
        /// Returns the next token without consuming it.
        /// </summary>
        internal TextToken Peek() => bufferedToken ??= ReadToken();

        /// <summary>
        /// Returns the next token, consuming it.
        /// </summary>
        internal TextToken Next()
        {
            if (bufferedToken.HasValue)
            {
                var result = bufferedToken.Value;
                bufferedToken = null;
                return result;
            }
            return ReadToken();
        }

        /// <summary>
        /// Consumes the next token if it is of the given type, returning whether it was consumed.
        /// </summary>
        internal bool TryConsume(TextToken.TokenType type)
        {
            if (Peek().Type == type)
            {
                Next();
                return true;
            }
            return false;
        }

        /// <summary>
        /// Reads the remainder of a bracketed field name (an extension name such as <c>pkg.ext</c>
        /// or an Any type URL such as <c>type.googleapis.com/pkg.Type</c>), including the closing
        /// <c>]</c>, and returns the name without brackets. The opening <c>[</c> token must already
        /// have been consumed with <see cref="Next"/>.
        /// </summary>
        internal string ReadBracketedName()
        {
            if (bufferedToken.HasValue)
            {
                throw new InvalidOperationException("Can't read a bracketed name with a buffered token");
            }
            // '.' and '/' are not tokens anywhere else in the grammar, so this is read at the character level.
            var builder = new StringBuilder();
            SkipWhitespaceAndComments();
            while (true)
            {
                int c = ReadChar();
                if (IsIdentifierPart(c) || c == '.' || c == '/')
                {
                    builder.Append((char) c);
                    continue;
                }
                PushBackChar(c);
                SkipWhitespaceAndComments();
                c = ReadChar();
                string name = builder.ToString();
                if (c != ']')
                {
                    throw Error("Expected ']' after bracketed field name '" + name + "'");
                }
                if (name.Length == 0 || name[0] == '.' || name[name.Length - 1] == '.' || name.Contains(".."))
                {
                    throw Error("Invalid bracketed field name '" + name + "'");
                }
                return name;
            }
        }

        private TextToken ReadToken()
        {
            SkipWhitespaceAndComments();
            int c = ReadChar();
            switch (c)
            {
                case EndOfInput:
                    return TextToken.EndDocument;
                case ':':
                    return TextToken.Colon;
                case '{':
                    return TextToken.OpenBrace;
                case '}':
                    return TextToken.CloseBrace;
                case '<':
                    return TextToken.OpenAngle;
                case '>':
                    return TextToken.CloseAngle;
                case '[':
                    return TextToken.OpenBracket;
                case ']':
                    return TextToken.CloseBracket;
                case ',':
                    return TextToken.Comma;
                case ';':
                    return TextToken.Semicolon;
                case '"':
                case '\'':
                    return ReadStringLiterals((char) c);
                case '-':
                    return ReadNegativeNumber();
                case '.':
                    return ReadNumber("", startedWithDot: true);
            }
            if (IsDigit(c))
            {
                return ReadNumber(((char) c).ToString(), startedWithDot: false);
            }
            if (IsIdentifierStart(c))
            {
                return TextToken.Identifier(ReadIdentifier(c));
            }
            throw Error("Unexpected character '" + (char) c + "'");
        }

        private string ReadIdentifier(int firstChar)
        {
            var builder = new StringBuilder();
            builder.Append((char) firstChar);
            while (true)
            {
                int c = ReadChar();
                if (IsIdentifierPart(c))
                {
                    builder.Append((char) c);
                }
                else
                {
                    PushBackChar(c);
                    return builder.ToString();
                }
            }
        }

        private TextToken ReadNegativeNumber()
        {
            // The text format allows whitespace between a '-' and the number or inf/nan identifier.
            SkipWhitespaceAndComments();
            int c = ReadChar();
            if (c == '.')
            {
                return ReadNumber("-", startedWithDot: true);
            }
            if (IsDigit(c))
            {
                return ReadNumber("-" + (char) c, startedWithDot: false);
            }
            if (IsIdentifierStart(c))
            {
                string identifier = ReadIdentifier(c);
                string lower = identifier.ToLowerInvariant();
                if (lower == "inf" || lower == "infinity" || lower == "nan")
                {
                    return TextToken.Float("-" + identifier);
                }
                throw Error("Expected a number after '-', got '" + identifier + "'");
            }
            throw Error("Expected a number after '-'");
        }

        /// <summary>
        /// Reads a numeric literal. <paramref name="prefix"/> contains the characters consumed so far (an
        /// optional '-' and the first digit); when <paramref name="startedWithDot"/> is true the leading '.'
        /// has been consumed and is not included in the prefix.
        /// </summary>
        private TextToken ReadNumber(string prefix, bool startedWithDot)
        {
            var builder = new StringBuilder();
            builder.Append(prefix);
            bool isFloat = false;
            bool startedWithZero = prefix.Length > 0 && prefix[prefix.Length - 1] == '0';
            int c;
            if (startedWithDot)
            {
                builder.Append('.');
                isFloat = true;
                c = ReadChar();
                if (!IsDigit(c))
                {
                    throw Error("Expected a digit after '.'");
                }
                c = ConsumeDigits(builder, c);
            }
            else
            {
                c = ReadChar();
                if (startedWithZero && (c == 'x' || c == 'X'))
                {
                    builder.Append((char) c);
                    c = ReadChar();
                    if (!IsHexDigit(c))
                    {
                        throw Error("\"0x\" must be followed by hex digits");
                    }
                    while (IsHexDigit(c))
                    {
                        builder.Append((char) c);
                        c = ReadChar();
                    }
                    return FinishNumber(builder, c, isFloat: false);
                }
                if (startedWithZero && IsDigit(c))
                {
                    // Octal.
                    while (IsDigit(c))
                    {
                        if (c > '7')
                        {
                            throw Error("Numbers starting with a leading zero must be in octal");
                        }
                        builder.Append((char) c);
                        c = ReadChar();
                    }
                    return FinishNumber(builder, c, isFloat: false);
                }
                c = ConsumeDigits(builder, c);
                if (c == '.')
                {
                    isFloat = true;
                    builder.Append('.');
                    c = ReadChar();
                    c = ConsumeDigits(builder, c);
                }
            }
            if (c == 'e' || c == 'E')
            {
                isFloat = true;
                builder.Append((char) c);
                c = ReadChar();
                if (c == '-' || c == '+')
                {
                    builder.Append((char) c);
                    c = ReadChar();
                }
                if (!IsDigit(c))
                {
                    throw Error("\"e\" must be followed by an exponent");
                }
                c = ConsumeDigits(builder, c);
            }
            if (c == 'f' || c == 'F')
            {
                isFloat = true;
                builder.Append((char) c);
                c = ReadChar();
            }
            return FinishNumber(builder, c, isFloat);
        }

        private TextToken FinishNumber(StringBuilder builder, int nextChar, bool isFloat)
        {
            string text = builder.ToString();
            if (IsIdentifierPart(nextChar))
            {
                throw Error("Need a space between number '" + text + "' and identifier");
            }
            if (nextChar == '.')
            {
                throw isFloat
                    ? Error("Already saw a decimal point or exponent in '" + text + "'; can't have another one")
                    : Error("Hex and octal numbers must be integers: '" + text + "'");
            }
            PushBackChar(nextChar);
            return isFloat ? TextToken.Float(text) : TextToken.Integer(text);
        }

        private int ConsumeDigits(StringBuilder builder, int c)
        {
            while (IsDigit(c))
            {
                builder.Append((char) c);
                c = ReadChar();
            }
            return c;
        }

        /// <summary>
        /// Reads one or more adjacent string literals (separated only by whitespace and comments) and
        /// concatenates their raw unescaped bytes into a single token.
        /// </summary>
        private TextToken ReadStringLiterals(char firstQuote)
        {
            var bytes = new List<byte>();
            char quote = firstQuote;
            while (true)
            {
                ReadStringLiteral(quote, bytes);
                SkipWhitespaceAndComments();
                int c = ReadChar();
                if (c == '"' || c == '\'')
                {
                    quote = (char) c;
                    continue;
                }
                PushBackChar(c);
                break;
            }
            return TextToken.String(bytes.ToArray());
        }

        private void ReadStringLiteral(char quote, List<byte> output)
        {
            while (true)
            {
                int c = ReadChar();
                if (c == EndOfInput)
                {
                    throw Error("Unexpected end of input in string literal");
                }
                if (c == '\n')
                {
                    throw Error("String literals cannot cross line boundaries");
                }
                if (c == quote)
                {
                    return;
                }
                if (c != '\\')
                {
                    AppendUtf8((char) c, output);
                    continue;
                }
                ReadEscapeSequence(output);
            }
        }

        private void ReadEscapeSequence(List<byte> output)
        {
            int c = ReadChar();
            switch (c)
            {
                case 'a': output.Add((byte) '\a'); return;
                case 'b': output.Add((byte) '\b'); return;
                case 'f': output.Add((byte) '\f'); return;
                case 'n': output.Add((byte) '\n'); return;
                case 'r': output.Add((byte) '\r'); return;
                case 't': output.Add((byte) '\t'); return;
                case 'v': output.Add((byte) '\v'); return;
                case '\\': output.Add((byte) '\\'); return;
                case '\'': output.Add((byte) '\''); return;
                case '"': output.Add((byte) '"'); return;
                case '?': output.Add((byte) '?'); return;
                case 'x':
                case 'X':
                    {
                        int first = ReadChar();
                        if (!IsHexDigit(first))
                        {
                            throw Error("Expected hex digits after \\x");
                        }
                        int value = HexValue(first);
                        int second = ReadChar();
                        if (IsHexDigit(second))
                        {
                            value = value * 16 + HexValue(second);
                        }
                        else
                        {
                            PushBackChar(second);
                        }
                        output.Add((byte) value);
                        return;
                    }
                case 'u':
                    AppendUnicodeEscape(ReadHexDigits(4), output);
                    return;
                case 'U':
                    AppendUnicodeEscape(ReadHexDigits(8), output);
                    return;
                default:
                    if (c >= '0' && c <= '7')
                    {
                        int value = c - '0';
                        for (int i = 0; i < 2; i++)
                        {
                            int next = ReadChar();
                            if (next >= '0' && next <= '7')
                            {
                                value = value * 8 + (next - '0');
                            }
                            else
                            {
                                PushBackChar(next);
                                break;
                            }
                        }
                        if (value > 0xFF)
                        {
                            throw Error("Octal escape sequence out of range");
                        }
                        output.Add((byte) value);
                        return;
                    }
                    if (c == EndOfInput)
                    {
                        throw Error("Unexpected end of input in escape sequence");
                    }
                    throw Error("Invalid escape sequence '\\" + (char) c + "'");
            }
        }

        private long ReadHexDigits(int count)
        {
            long value = 0;
            for (int i = 0; i < count; i++)
            {
                int c = ReadChar();
                if (!IsHexDigit(c))
                {
                    throw Error("Expected " + count + " hex digits in Unicode escape sequence");
                }
                value = (value << 4) | (uint) HexValue(c);
            }
            return value;
        }

        private void AppendUnicodeEscape(long codePoint, List<byte> output)
        {
            // The text format specification does not allow surrogate code points in Unicode escapes
            // (not even as a correctly ordered pair), nor code points beyond U+10FFFF.
            if (codePoint > 0x10FFFF || (codePoint >= 0xD800 && codePoint <= 0xDFFF))
            {
                throw Error("Invalid Unicode code point in escape sequence: U+" + codePoint.ToString("X"));
            }
            output.AddRange(Encoding.UTF8.GetBytes(char.ConvertFromUtf32((int) codePoint)));
        }

        private void AppendUtf8(char c, List<byte> output)
        {
            if (char.IsHighSurrogate(c))
            {
                int next = ReadChar();
                if (next == EndOfInput || !char.IsLowSurrogate((char) next))
                {
                    throw Error("Unpaired surrogate in string literal");
                }
                output.AddRange(Encoding.UTF8.GetBytes(new[] { c, (char) next }));
            }
            else if (char.IsLowSurrogate(c))
            {
                throw Error("Unpaired surrogate in string literal");
            }
            else if (c < 0x80)
            {
                output.Add((byte) c);
            }
            else
            {
                output.AddRange(Encoding.UTF8.GetBytes(new[] { c }));
            }
        }

        private void SkipWhitespaceAndComments()
        {
            while (true)
            {
                int c = ReadChar();
                if (c == ' ' || c == '\t' || c == '\r' || c == '\n' || c == '\v' || c == '\f')
                {
                    continue;
                }
                if (c == '#')
                {
                    while (c != '\n' && c != EndOfInput)
                    {
                        c = ReadChar();
                    }
                    continue;
                }
                PushBackChar(c);
                return;
            }
        }

        private int ReadChar()
        {
            if (pushedBackChar != -2)
            {
                int result = pushedBackChar;
                pushedBackChar = -2;
                return result;
            }
            int c = reader.Read();
            if (c == '\n')
            {
                line++;
                column = 0;
            }
            else if (c != EndOfInput)
            {
                column++;
            }
            return c;
        }

        private void PushBackChar(int c)
        {
            if (pushedBackChar != -2)
            {
                throw new InvalidOperationException("Can't push back a character twice");
            }
            pushedBackChar = c;
        }

        private static bool IsDigit(int c) => c >= '0' && c <= '9';

        private static bool IsHexDigit(int c) =>
            IsDigit(c) || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');

        internal static int HexValue(int c)
        {
            if (IsDigit(c))
            {
                return c - '0';
            }
            if (c >= 'a' && c <= 'f')
            {
                return c - 'a' + 10;
            }
            return c - 'A' + 10;
        }

        private static bool IsIdentifierStart(int c) =>
            (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';

        private static bool IsIdentifierPart(int c) => IsIdentifierStart(c) || IsDigit(c);

        /// <summary>
        /// Creates an exception describing a problem at (approximately) the current input position.
        /// </summary>
        internal InvalidProtocolBufferException Error(string message) =>
            new InvalidProtocolBufferException($"Invalid text format at line {line}, column {column}: {message}");
    }
}
