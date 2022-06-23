#region Copyright notice and license
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
#endregion

using System;
using System.Collections.Generic;
using System.Globalization;
using System.IO;
using System.Text;

namespace Google.Protobuf
{
    /// <summary>
    /// Simple but strict JSON tokenizer, rigidly following RFC 7159.
    /// </summary>
    /// <remarks>
    /// <para>
    /// This tokenizer is stateful, and only returns "useful" tokens - names, values etc.
    /// It does not create tokens for the separator between names and values, or for the comma
    /// between values. It validates the token stream as it goes - so callers can assume that the
    /// tokens it produces are appropriate. For example, it would never produce "start object, end array."
    /// </para>
    /// <para>Implementation details: the base class handles single token push-back and </para>
    /// <para>Not thread-safe.</para>
    /// </remarks>
    internal abstract class JsonTokenizer
    {
        private JsonToken bufferedToken;

        /// <summary>
        ///  Creates a tokenizer that reads from the given text reader.
        /// </summary>
        internal static JsonTokenizer FromTextReader(TextReader reader)
        {
            return new JsonTextTokenizer(reader);
        }

        /// <summary>
        /// Creates a tokenizer that first replays the given list of tokens, then continues reading
        /// from another tokenizer. Note that if the returned tokenizer is "pushed back", that does not push back
        /// on the continuation tokenizer, or vice versa. Care should be taken when using this method - it was
        /// created for the sake of Any parsing.
        /// </summary>
        internal static JsonTokenizer FromReplayedTokens(IList<JsonToken> tokens, JsonTokenizer continuation)
        {
            return new JsonReplayTokenizer(tokens, continuation);
        }

        /// <summary>
        /// Returns the depth of the stack, purely in objects (not collections).
        /// Informally, this is the number of remaining unclosed '{' characters we have.
        /// </summary>
        internal int ObjectDepth { get; private set; }

        // TODO: Why do we allow a different token to be pushed back? It might be better to always remember the previous
        // token returned, and allow a parameterless Rewind() method (which could only be called once, just like the current PushBack).
        internal void PushBack(JsonToken token)
        {
            if (bufferedToken != null)
            {
                throw new InvalidOperationException("Can't push back twice");
            }
            bufferedToken = token;
            if (token.Type == JsonToken.TokenType.StartObject)
            {
                ObjectDepth--;
            }
            else if (token.Type == JsonToken.TokenType.EndObject)
            {
                ObjectDepth++;
            }
        }

        /// <summary>
        /// Returns the next JSON token in the stream. An EndDocument token is returned to indicate the end of the stream,
        /// after which point <c>Next()</c> should not be called again.
        /// </summary>
        /// <remarks>This implementation provides single-token buffering, and calls <see cref="NextImpl"/> if there is no buffered token.</remarks>
        /// <returns>The next token in the stream. This is never null.</returns>
        /// <exception cref="InvalidOperationException">This method is called after an EndDocument token has been returned</exception>
        /// <exception cref="InvalidJsonException">The input text does not comply with RFC 7159</exception>
        internal JsonToken Next()
        {
            JsonToken tokenToReturn;
            if (bufferedToken != null)
            {
                tokenToReturn = bufferedToken;
                bufferedToken = null;
            }
            else
            {
                tokenToReturn = NextImpl();
            }
            if (tokenToReturn.Type == JsonToken.TokenType.StartObject)
            {
                ObjectDepth++;
            }
            else if (tokenToReturn.Type == JsonToken.TokenType.EndObject)
            {
                ObjectDepth--;
            }
            return tokenToReturn;
        }

        /// <summary>
        /// Returns the next JSON token in the stream, when requested by the base class. (The <see cref="Next"/> method delegates
        /// to this if it doesn't have a buffered token.)
        /// </summary>
        /// <exception cref="InvalidOperationException">This method is called after an EndDocument token has been returned</exception>
        /// <exception cref="InvalidJsonException">The input text does not comply with RFC 7159</exception>
        protected abstract JsonToken NextImpl();

        /// <summary>
        /// Skips the value we're about to read. This must only be called immediately after reading a property name.
        /// If the value is an object or an array, the complete object/array is skipped.
        /// </summary>
        internal void SkipValue()
        {
            // We'll assume that Next() makes sure that the end objects and end arrays are all valid.
            // All we care about is the total nesting depth we need to close.
            int depth = 0;

            // do/while rather than while loop so that we read at least one token.
            do
            {
                var token = Next();
                switch (token.Type)
                {
                    case JsonToken.TokenType.EndArray:
                    case JsonToken.TokenType.EndObject:
                        depth--;
                        break;
                    case JsonToken.TokenType.StartArray:
                    case JsonToken.TokenType.StartObject:
                        depth++;
                        break;
                }
            } while (depth != 0);
        }

        /// <summary>
        /// Tokenizer which first exhausts a list of tokens, then consults another tokenizer.
        /// </summary>
        private class JsonReplayTokenizer : JsonTokenizer
        {
            private readonly IList<JsonToken> tokens;
            private readonly JsonTokenizer nextTokenizer;
            private int nextTokenIndex;

            internal JsonReplayTokenizer(IList<JsonToken> tokens, JsonTokenizer nextTokenizer)
            {
                this.tokens = tokens;
                this.nextTokenizer = nextTokenizer;
            }

            // FIXME: Object depth not maintained...
            protected override JsonToken NextImpl()
            {
                if (nextTokenIndex >= tokens.Count)
                {
                    return nextTokenizer.Next();
                }
                return tokens[nextTokenIndex++];
            }
        }

        /// <summary>
        /// Tokenizer which does all the *real* work of parsing JSON.
        /// </summary>
        private sealed class JsonTextTokenizer : JsonTokenizer
        {
            // The set of states in which a value is valid next token.
            private static readonly State ValueStates = State.ArrayStart | State.ArrayAfterComma | State.ObjectAfterColon | State.StartOfDocument;

            private readonly Stack<ContainerType> containerStack = new Stack<ContainerType>();
            private readonly PushBackReader reader;
            private State state;

            internal JsonTextTokenizer(TextReader reader)
            {
                this.reader = new PushBackReader(reader);
                state = State.StartOfDocument;
                containerStack.Push(ContainerType.Document);
            }

            /// <remarks>
            /// This method essentially just loops through characters skipping whitespace, validating and
            /// changing state (e.g. from ObjectBeforeColon to ObjectAfterColon)
            /// until it reaches something which will be a genuine token (e.g. a start object, or a value) at which point
            /// it returns the token. Although the method is large, it would be relatively hard to break down further... most
            /// of it is the large switch statement, which sometimes returns and sometimes doesn't.
            /// </remarks>
            protected override JsonToken NextImpl()
            {
                if (state == State.ReaderExhausted)
                {
                    throw new InvalidOperationException("Next() called after end of document");
                }
                while (true)
                {
                    var next = reader.Read();
                    if (next == null)
                    {
                        ValidateState(State.ExpectedEndOfDocument, "Unexpected end of document in state: ");
                        state = State.ReaderExhausted;
                        return JsonToken.EndDocument;
                    }
                    switch (next.Value)
                    {
                        // Skip whitespace between tokens
                        case ' ':
                        case '\t':
                        case '\r':
                        case '\n':
                            break;
                        case ':':
                            ValidateState(State.ObjectBeforeColon, "Invalid state to read a colon: ");
                            state = State.ObjectAfterColon;
                            break;
                        case ',':
                            ValidateState(State.ObjectAfterProperty | State.ArrayAfterValue, "Invalid state to read a comma: ");
                            state = state == State.ObjectAfterProperty ? State.ObjectAfterComma : State.ArrayAfterComma;
                            break;
                        case '"':
                            string stringValue = ReadString();
                            if ((state & (State.ObjectStart | State.ObjectAfterComma)) != 0)
                            {
                                state = State.ObjectBeforeColon;
                                return JsonToken.Name(stringValue);
                            }
                            else
                            {
                                ValidateAndModifyStateForValue("Invalid state to read a double quote: ");
                                return JsonToken.Value(stringValue);
                            }
                        case '{':
                            ValidateState(ValueStates, "Invalid state to read an open brace: ");
                            state = State.ObjectStart;
                            containerStack.Push(ContainerType.Object);
                            return JsonToken.StartObject;
                        case '}':
                            ValidateState(State.ObjectAfterProperty | State.ObjectStart, "Invalid state to read a close brace: ");
                            PopContainer();
                            return JsonToken.EndObject;
                        case '[':
                            ValidateState(ValueStates, "Invalid state to read an open square bracket: ");
                            state = State.ArrayStart;
                            containerStack.Push(ContainerType.Array);
                            return JsonToken.StartArray;
                        case ']':
                            ValidateState(State.ArrayAfterValue | State.ArrayStart, "Invalid state to read a close square bracket: ");
                            PopContainer();
                            return JsonToken.EndArray;
                        case 'n': // Start of null
                            ConsumeLiteral("null");
                            ValidateAndModifyStateForValue("Invalid state to read a null literal: ");
                            return JsonToken.Null;
                        case 't': // Start of true
                            ConsumeLiteral("true");
                            ValidateAndModifyStateForValue("Invalid state to read a true literal: ");
                            return JsonToken.True;
                        case 'f': // Start of false
                            ConsumeLiteral("false");
                            ValidateAndModifyStateForValue("Invalid state to read a false literal: ");
                            return JsonToken.False;
                        case '-': // Start of a number
                        case '0':
                        case '1':
                        case '2':
                        case '3':
                        case '4':
                        case '5':
                        case '6':
                        case '7':
                        case '8':
                        case '9':
                            double number = ReadNumber(next.Value);
                            ValidateAndModifyStateForValue("Invalid state to read a number token: ");
                            return JsonToken.Value(number);
                        default:
                            throw new InvalidJsonException("Invalid first character of token: " + next.Value);
                    }
                }
            }

            private void ValidateState(State validStates, string errorPrefix)
            {
                if ((validStates & state) == 0)
                {
                    throw reader.CreateException(errorPrefix + state);
                }
            }

            /// <summary>
            /// Reads a string token. It is assumed that the opening " has already been read.
            /// </summary>
            private string ReadString()
            {
                var value = new StringBuilder();
                bool haveHighSurrogate = false;
                while (true)
                {
                    char c = reader.ReadOrFail("Unexpected end of text while reading string");
                    if (c < ' ')
                    {
                        throw reader.CreateException(string.Format(CultureInfo.InvariantCulture, "Invalid character in string literal: U+{0:x4}", (int) c));
                    }
                    if (c == '"')
                    {
                        if (haveHighSurrogate)
                        {
                            throw reader.CreateException("Invalid use of surrogate pair code units");
                        }
                        return value.ToString();
                    }
                    if (c == '\\')
                    {
                        c = ReadEscapedCharacter();
                    }
                    // TODO: Consider only allowing surrogate pairs that are either both escaped,
                    // or both not escaped. It would be a very odd text stream that contained a "lone" high surrogate
                    // followed by an escaped low surrogate or vice versa... and that couldn't even be represented in UTF-8.
                    if (haveHighSurrogate != char.IsLowSurrogate(c))
                    {
                        throw reader.CreateException("Invalid use of surrogate pair code units");
                    }
                    haveHighSurrogate = char.IsHighSurrogate(c);
                    value.Append(c);
                }
            }

            /// <summary>
            /// Reads an escaped character. It is assumed that the leading backslash has already been read.
            /// </summary>
            private char ReadEscapedCharacter()
            {
                char c = reader.ReadOrFail("Unexpected end of text while reading character escape sequence");
                return c switch
                {
                    'n' => '\n',
                    '\\' => '\\',
                    'b' => '\b',
                    'f' => '\f',
                    'r' => '\r',
                    't' => '\t',
                    '"' => '"',
                    '/' => '/',
                    'u' => ReadUnicodeEscape(),
                    _ => throw reader.CreateException(string.Format(CultureInfo.InvariantCulture, "Invalid character in character escape sequence: U+{0:x4}", (int)c)),
                };
            }

            /// <summary>
            /// Reads an escaped Unicode 4-nybble hex sequence. It is assumed that the leading \u has already been read.
            /// </summary>
            private char ReadUnicodeEscape()
            {
                int result = 0;
                for (int i = 0; i < 4; i++)
                {
                    char c = reader.ReadOrFail("Unexpected end of text while reading Unicode escape sequence");
                    int nybble;
                    if (c >= '0' && c <= '9')
                    {
                        nybble = c - '0';
                    }
                    else if (c >= 'a' && c <= 'f')
                    {
                        nybble = c - 'a' + 10;
                    }
                    else if (c >= 'A' && c <= 'F')
                    {
                        nybble = c - 'A' + 10;
                    }
                    else
                    {
                        throw reader.CreateException(string.Format(CultureInfo.InvariantCulture, "Invalid character in character escape sequence: U+{0:x4}", (int) c));
                    }
                    result = (result << 4) + nybble;
                }
                return (char) result;
            }

            /// <summary>
            /// Consumes a text-only literal, throwing an exception if the read text doesn't match it.
            /// It is assumed that the first letter of the literal has already been read.
            /// </summary>
            private void ConsumeLiteral(string text)
            {
                for (int i = 1; i < text.Length; i++)
                {
                    char? next = reader.Read();
                    if (next == null)
                    {
                        throw reader.CreateException("Unexpected end of text while reading literal token " + text);
                    }
                    if (next.Value != text[i])
                    {
                        throw reader.CreateException("Unexpected character while reading literal token " + text);
                    }
                }
            }

            private double ReadNumber(char initialCharacter)
            {
                StringBuilder builder = new StringBuilder();
                if (initialCharacter == '-')
                {
                    builder.Append("-");
                }
                else
                {
                    reader.PushBack(initialCharacter);
                }
                // Each method returns the character it read that doesn't belong in that part,
                // so we know what to do next, including pushing the character back at the end.
                // null is returned for "end of text".
                char? next = ReadInt(builder);
                if (next == '.')
                {
                    next = ReadFrac(builder);
                }
                if (next == 'e' || next == 'E')
                {
                    next = ReadExp(builder);
                }
                // If we read a character which wasn't part of the number, push it back so we can read it again
                // to parse the next token.
                if (next != null)
                {
                    reader.PushBack(next.Value);
                }

                // TODO: What exception should we throw if the value can't be represented as a double?
                try
                {
                    double result = double.Parse(builder.ToString(),
                        NumberStyles.AllowLeadingSign | NumberStyles.AllowDecimalPoint | NumberStyles.AllowExponent,
                        CultureInfo.InvariantCulture);

                    // .NET Core 3.0 and later returns infinity if the number is too large or small to be represented.
                    // For compatibility with other Protobuf implementations the tokenizer should still throw.
                    if (double.IsInfinity(result))
                    {
                        throw reader.CreateException("Numeric value out of range: " + builder);
                    }

                    return result;
                }
                catch (OverflowException)
                {
                    throw reader.CreateException("Numeric value out of range: " + builder);
                }
            }

            private char? ReadInt(StringBuilder builder)
            {
                char first = reader.ReadOrFail("Invalid numeric literal");
                if (first < '0' || first > '9')
                {
                    throw reader.CreateException("Invalid numeric literal");
                }
                builder.Append(first);
                char? next = ConsumeDigits(builder, out int digitCount);
                if (first == '0' && digitCount != 0)
                {
                    throw reader.CreateException("Invalid numeric literal: leading 0 for non-zero value.");
                }
                return next;
            }

            private char? ReadFrac(StringBuilder builder)
            {
                builder.Append('.'); // Already consumed this
                char? next = ConsumeDigits(builder, out int digitCount);
                if (digitCount == 0)
                {
                    throw reader.CreateException("Invalid numeric literal: fraction with no trailing digits");
                }
                return next;
            }

            private char? ReadExp(StringBuilder builder)
            {
                builder.Append('E'); // Already consumed this (or 'e')
                char? next = reader.Read();
                if (next == null)
                {
                    throw reader.CreateException("Invalid numeric literal: exponent with no trailing digits");
                }
                if (next == '-' || next == '+')
                {
                    builder.Append(next.Value);
                }
                else
                {
                    reader.PushBack(next.Value);
                }
                next = ConsumeDigits(builder, out int digitCount);
                if (digitCount == 0)
                {
                    throw reader.CreateException("Invalid numeric literal: exponent without value");
                }
                return next;
            }

            private char? ConsumeDigits(StringBuilder builder, out int count)
            {
                count = 0;
                while (true)
                {
                    char? next = reader.Read();
                    if (next == null || next.Value < '0' || next.Value > '9')
                    {
                        return next;
                    }
                    count++;
                    builder.Append(next.Value);
                }
            }

            /// <summary>
            /// Validates that we're in a valid state to read a value (using the given error prefix if necessary)
            /// and changes the state to the appropriate one, e.g. ObjectAfterColon to ObjectAfterProperty.
            /// </summary>
            private void ValidateAndModifyStateForValue(string errorPrefix)
            {
                ValidateState(ValueStates, errorPrefix);
                switch (state)
                {
                    case State.StartOfDocument:
                        state = State.ExpectedEndOfDocument;
                        return;
                    case State.ObjectAfterColon:
                        state = State.ObjectAfterProperty;
                        return;
                    case State.ArrayStart:
                    case State.ArrayAfterComma:
                        state = State.ArrayAfterValue;
                        return;
                    default:
                        throw new InvalidOperationException("ValidateAndModifyStateForValue does not handle all value states (and should)");
                }
            }

            /// <summary>
            /// Pops the top-most container, and sets the state to the appropriate one for the end of a value
            /// in the parent container.
            /// </summary>
            private void PopContainer()
            {
                containerStack.Pop();
                var parent = containerStack.Peek();
                state = parent switch
                {
                    ContainerType.Object => State.ObjectAfterProperty,
                    ContainerType.Array => State.ArrayAfterValue,
                    ContainerType.Document => State.ExpectedEndOfDocument,
                    _ => throw new InvalidOperationException("Unexpected container type: " + parent),
                };
            }

            private enum ContainerType
            {
                Document, Object, Array
            }

            /// <summary>
            /// Possible states of the tokenizer.
            /// </summary>
            /// <remarks>
            /// <para>This is a flags enum purely so we can simply and efficiently represent a set of valid states
            /// for checking.</para>
            /// <para>
            /// Each is documented with an example,
            /// where ^ represents the current position within the text stream. The examples all use string values,
            /// but could be any value, including nested objects/arrays.
            /// The complete state of the tokenizer also includes a stack to indicate the contexts (arrays/objects).
            /// Any additional notional state of "AfterValue" indicates that a value has been completed, at which
            /// point there's an immediate transition to ExpectedEndOfDocument,  ObjectAfterProperty or ArrayAfterValue.
            /// </para>
            /// <para>
            /// These states were derived manually by reading RFC 7159 carefully.
            /// </para>
            /// </remarks>
            [Flags]
            private enum State
            {
                /// <summary>
                /// ^ { "foo": "bar" }
                /// Before the value in a document. Next states: ObjectStart, ArrayStart, "AfterValue"
                /// </summary>
                StartOfDocument = 1 << 0,
                /// <summary>
                /// { "foo": "bar" } ^
                /// After the value in a document. Next states: ReaderExhausted
                /// </summary>
                ExpectedEndOfDocument = 1 << 1,
                /// <summary>
                /// { "foo": "bar" } ^ (and already read to the end of the reader)
                /// Terminal state.
                /// </summary>
                ReaderExhausted = 1 << 2,
                /// <summary>
                /// { ^ "foo": "bar" }
                /// Before the *first* property in an object.
                /// Next states:
                /// "AfterValue" (empty object)
                /// ObjectBeforeColon (read a name)
                /// </summary>
                ObjectStart = 1 << 3,
                /// <summary>
                /// { "foo" ^ : "bar", "x": "y" }
                /// Next state: ObjectAfterColon
                /// </summary>
                ObjectBeforeColon = 1 << 4,
                /// <summary>
                /// { "foo" : ^ "bar", "x": "y" }
                /// Before any property other than the first in an object.
                /// (Equivalently: after any property in an object)
                /// Next states:
                /// "AfterValue" (value is simple)
                /// ObjectStart (value is object)
                /// ArrayStart (value is array)
                /// </summary>
                ObjectAfterColon = 1 << 5,
                /// <summary>
                /// { "foo" : "bar" ^ , "x" : "y" }
                /// At the end of a property, so expecting either a comma or end-of-object
                /// Next states: ObjectAfterComma or "AfterValue"
                /// </summary>
                ObjectAfterProperty = 1 << 6,
                /// <summary>
                /// { "foo":"bar", ^ "x":"y" }
                /// Read the comma after the previous property, so expecting another property.
                /// This is like ObjectStart, but closing brace isn't valid here
                /// Next state: ObjectBeforeColon.
                /// </summary>
                ObjectAfterComma = 1 << 7,
                /// <summary>
                /// [ ^ "foo", "bar" ]
                /// Before the *first* value in an array.
                /// Next states:
                /// "AfterValue" (read a value)
                /// "AfterValue" (end of array; will pop stack)
                /// </summary>
                ArrayStart = 1 << 8,
                /// <summary>
                /// [ "foo" ^ , "bar" ]
                /// After any value in an array, so expecting either a comma or end-of-array
                /// Next states: ArrayAfterComma or "AfterValue"
                /// </summary>
                ArrayAfterValue = 1 << 9,
                /// <summary>
                /// [ "foo", ^ "bar" ]
                /// After a comma in an array, so there *must* be another value (simple or complex).
                /// Next states: "AfterValue" (simple value), StartObject, StartArray
                /// </summary>
                ArrayAfterComma = 1 << 10
            }

            /// <summary>
            /// Wrapper around a text reader allowing small amounts of buffering and location handling.
            /// </summary>
            private class PushBackReader
            {
                // TODO: Add locations for errors etc.

                private readonly TextReader reader;

                internal PushBackReader(TextReader reader)
                {
                    // TODO: Wrap the reader in a BufferedReader?
                    this.reader = reader;
                }

                /// <summary>
                /// The buffered next character, if we have one.
                /// </summary>
                private char? nextChar;

                /// <summary>
                /// Returns the next character in the stream, or null if we have reached the end.
                /// </summary>
                /// <returns></returns>
                internal char? Read()
                {
                    if (nextChar != null)
                    {
                        char? tmp = nextChar;
                        nextChar = null;
                        return tmp;
                    }
                    int next = reader.Read();
                    return next == -1 ? null : (char?) next;
                }

                internal char ReadOrFail(string messageOnFailure)
                {
                    char? next = Read();
                    if (next == null)
                    {
                        throw CreateException(messageOnFailure);
                    }
                    return next.Value;
                }

                internal void PushBack(char c)
                {
                    if (nextChar != null)
                    {
                        throw new InvalidOperationException("Cannot push back when already buffering a character");
                    }
                    nextChar = c;
                }

                /// <summary>
                /// Creates a new exception appropriate for the current state of the reader.
                /// </summary>
                internal InvalidJsonException CreateException(string message)
                {
                    // TODO: Keep track of and use the location.
                    return new InvalidJsonException(message);
                }
            }
        }
    }
}
