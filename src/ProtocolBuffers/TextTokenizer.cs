#region Copyright notice and license

// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
// http://github.com/jskeet/dotnet-protobufs/
// Original C++/Java/Python code:
// http://code.google.com/p/protobuf/
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

#endregion

using System;
using System.Globalization;
using System.Text.RegularExpressions;

namespace Google.ProtocolBuffers
{
    /// <summary>
    /// Represents a stream of tokens parsed from a string.
    /// </summary>
    internal sealed class TextTokenizer
    {
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

        // Note: atomic groups used to mimic possessive quantifiers in Java in both of these regexes
        internal static readonly Regex WhitespaceAndCommentPattern = new Regex("\\G(?>(\\s|(#.*$))+)",
                                                                               SilverlightCompatibility.
                                                                                   CompiledRegexWhereAvailable |
                                                                               RegexOptions.Multiline);

        private static readonly Regex TokenPattern = new Regex(
            "\\G[a-zA-Z_](?>[0-9a-zA-Z_+-]*)|" + // an identifier
            "\\G[0-9+-](?>[0-9a-zA-Z_.+-]*)|" + // a number
            "\\G\"(?>([^\"\\\n\\\\]|\\\\.)*)(\"|\\\\?$)|" + // a double-quoted string
            "\\G\'(?>([^\"\\\n\\\\]|\\\\.)*)(\'|\\\\?$)", // a single-quoted string
            SilverlightCompatibility.CompiledRegexWhereAvailable | RegexOptions.Multiline);

        private static readonly Regex DoubleInfinity = new Regex("^-?inf(inity)?$",
                                                                 SilverlightCompatibility.CompiledRegexWhereAvailable |
                                                                 RegexOptions.IgnoreCase);

        private static readonly Regex FloatInfinity = new Regex("^-?inf(inity)?f?$",
                                                                SilverlightCompatibility.CompiledRegexWhereAvailable |
                                                                RegexOptions.IgnoreCase);

        private static readonly Regex FloatNan = new Regex("^nanf?$",
                                                           SilverlightCompatibility.CompiledRegexWhereAvailable |
                                                           RegexOptions.IgnoreCase);

        /** Construct a tokenizer that parses tokens from the given text. */

        public TextTokenizer(string text)
        {
            this.text = text;
            SkipWhitespace();
            NextToken();
        }

        /// <summary>
        /// Are we at the end of the input?
        /// </summary>
        public bool AtEnd
        {
            get { return currentToken.Length == 0; }
        }

        /// <summary>
        /// Advances to the next token.
        /// </summary>
        public void NextToken()
        {
            previousLine = line;
            previousColumn = column;

            // Advance the line counter to the current position.
            while (pos < matchPos)
            {
                if (text[pos] == '\n')
                {
                    ++line;
                    column = 0;
                }
                else
                {
                    ++column;
                }
                ++pos;
            }

            // Match the next token.
            if (matchPos == text.Length)
            {
                // EOF
                currentToken = "";
            }
            else
            {
                Match match = TokenPattern.Match(text, matchPos);
                if (match.Success)
                {
                    currentToken = match.Value;
                    matchPos += match.Length;
                }
                else
                {
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
        private void SkipWhitespace()
        {
            Match match = WhitespaceAndCommentPattern.Match(text, matchPos);
            if (match.Success)
            {
                matchPos += match.Length;
            }
        }

        /// <summary>
        /// If the next token exactly matches the given token, consume it and return
        /// true. Otherwise, return false without doing anything.
        /// </summary>
        public bool TryConsume(string token)
        {
            if (currentToken == token)
            {
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
        public void Consume(string token)
        {
            if (!TryConsume(token))
            {
                throw CreateFormatException("Expected \"" + token + "\".");
            }
        }

        /// <summary>
        /// Returns true if the next token is an integer, but does not consume it.
        /// </summary>
        public bool LookingAtInteger()
        {
            if (currentToken.Length == 0)
            {
                return false;
            }

            char c = currentToken[0];
            return ('0' <= c && c <= '9') || c == '-' || c == '+';
        }

        /// <summary>
        /// If the next token is an identifier, consume it and return its value.
        /// Otherwise, throw a FormatException.
        /// </summary>
        public string ConsumeIdentifier()
        {
            foreach (char c in currentToken)
            {
                if (('a' <= c && c <= 'z') ||
                    ('A' <= c && c <= 'Z') ||
                    ('0' <= c && c <= '9') ||
                    (c == '_') || (c == '.'))
                {
                    // OK
                }
                else
                {
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
        public int ConsumeInt32()
        {
            try
            {
                int result = TextFormat.ParseInt32(currentToken);
                NextToken();
                return result;
            }
            catch (FormatException e)
            {
                throw CreateIntegerParseException(e);
            }
        }

        /// <summary>
        /// If the next token is a 32-bit unsigned integer, consume it and return its
        /// value. Otherwise, throw a FormatException.
        /// </summary>
        public uint ConsumeUInt32()
        {
            try
            {
                uint result = TextFormat.ParseUInt32(currentToken);
                NextToken();
                return result;
            }
            catch (FormatException e)
            {
                throw CreateIntegerParseException(e);
            }
        }

        /// <summary>
        /// If the next token is a 64-bit signed integer, consume it and return its
        /// value. Otherwise, throw a FormatException.
        /// </summary>
        public long ConsumeInt64()
        {
            try
            {
                long result = TextFormat.ParseInt64(currentToken);
                NextToken();
                return result;
            }
            catch (FormatException e)
            {
                throw CreateIntegerParseException(e);
            }
        }

        /// <summary>
        /// If the next token is a 64-bit unsigned integer, consume it and return its
        /// value. Otherwise, throw a FormatException.
        /// </summary>
        public ulong ConsumeUInt64()
        {
            try
            {
                ulong result = TextFormat.ParseUInt64(currentToken);
                NextToken();
                return result;
            }
            catch (FormatException e)
            {
                throw CreateIntegerParseException(e);
            }
        }

        /// <summary>
        /// If the next token is a double, consume it and return its value.
        /// Otherwise, throw a FormatException.
        /// </summary>
        public double ConsumeDouble()
        {
            // We need to parse infinity and nan separately because
            // double.Parse() does not accept "inf", "infinity", or "nan".
            if (DoubleInfinity.IsMatch(currentToken))
            {
                bool negative = currentToken.StartsWith("-");
                NextToken();
                return negative ? double.NegativeInfinity : double.PositiveInfinity;
            }
            if (currentToken.Equals("nan", StringComparison.InvariantCultureIgnoreCase))
            {
                NextToken();
                return Double.NaN;
            }

            try
            {
                double result = double.Parse(currentToken, CultureInfo.InvariantCulture);
                NextToken();
                return result;
            }
            catch (FormatException e)
            {
                throw CreateFloatParseException(e);
            }
            catch (OverflowException e)
            {
                throw CreateFloatParseException(e);
            }
        }

        /// <summary>
        /// If the next token is a float, consume it and return its value.
        /// Otherwise, throw a FormatException.
        /// </summary>
        public float ConsumeFloat()
        {
            // We need to parse infinity and nan separately because
            // Float.parseFloat() does not accept "inf", "infinity", or "nan".
            if (FloatInfinity.IsMatch(currentToken))
            {
                bool negative = currentToken.StartsWith("-");
                NextToken();
                return negative ? float.NegativeInfinity : float.PositiveInfinity;
            }
            if (FloatNan.IsMatch(currentToken))
            {
                NextToken();
                return float.NaN;
            }

            if (currentToken.EndsWith("f"))
            {
                currentToken = currentToken.TrimEnd('f');
            }

            try
            {
                float result = float.Parse(currentToken, CultureInfo.InvariantCulture);
                NextToken();
                return result;
            }
            catch (FormatException e)
            {
                throw CreateFloatParseException(e);
            }
            catch (OverflowException e)
            {
                throw CreateFloatParseException(e);
            }
        }

        /// <summary>
        /// If the next token is a Boolean, consume it and return its value.
        /// Otherwise, throw a FormatException.    
        /// </summary>
        public bool ConsumeBoolean()
        {
            if (currentToken == "true")
            {
                NextToken();
                return true;
            }
            if (currentToken == "false")
            {
                NextToken();
                return false;
            }
            throw CreateFormatException("Expected \"true\" or \"false\".");
        }

        /// <summary>
        /// If the next token is a string, consume it and return its (unescaped) value.
        /// Otherwise, throw a FormatException.
        /// </summary>
        public string ConsumeString()
        {
            return ConsumeByteString().ToStringUtf8();
        }

        /// <summary>
        /// If the next token is a string, consume it, unescape it as a
        /// ByteString and return it. Otherwise, throw a FormatException.
        /// </summary>
        public ByteString ConsumeByteString()
        {
            char quote = currentToken.Length > 0 ? currentToken[0] : '\0';
            if (quote != '\"' && quote != '\'')
            {
                throw CreateFormatException("Expected string.");
            }

            if (currentToken.Length < 2 ||
                currentToken[currentToken.Length - 1] != quote)
            {
                throw CreateFormatException("String missing ending quote.");
            }

            try
            {
                string escaped = currentToken.Substring(1, currentToken.Length - 2);
                ByteString result = TextFormat.UnescapeBytes(escaped);
                NextToken();
                return result;
            }
            catch (FormatException e)
            {
                throw CreateFormatException(e.Message);
            }
        }

        /// <summary>
        /// Returns a format exception with the current line and column numbers
        /// in the description, suitable for throwing.
        /// </summary>
        public FormatException CreateFormatException(string description)
        {
            // Note:  People generally prefer one-based line and column numbers.
            return new FormatException((line + 1) + ":" + (column + 1) + ": " + description);
        }

        /// <summary>
        /// Returns a format exception with the line and column numbers of the
        /// previous token in the description, suitable for throwing.
        /// </summary>
        public FormatException CreateFormatExceptionPreviousToken(string description)
        {
            // Note:  People generally prefer one-based line and column numbers.
            return new FormatException((previousLine + 1) + ":" + (previousColumn + 1) + ": " + description);
        }

        /// <summary>
        /// Constructs an appropriate FormatException for the given existing exception
        /// when trying to parse an integer.
        /// </summary>
        private FormatException CreateIntegerParseException(FormatException e)
        {
            return CreateFormatException("Couldn't parse integer: " + e.Message);
        }

        /// <summary>
        /// Constructs an appropriate FormatException for the given existing exception
        /// when trying to parse a float or double.
        /// </summary>
        private FormatException CreateFloatParseException(Exception e)
        {
            return CreateFormatException("Couldn't parse number: " + e.Message);
        }
    }
}