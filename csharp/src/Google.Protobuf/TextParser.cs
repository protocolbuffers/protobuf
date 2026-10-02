#region Copyright notice and license
// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd
#endregion

using Google.Protobuf.Reflection;
using Google.Protobuf.WellKnownTypes;
using System;
using System.Collections;
using System.Collections.Generic;
using System.Globalization;
using System.IO;
using System.Text;

namespace Google.Protobuf
{
    /// <summary>
    /// Reflection-based parser for the Protocol Buffers text format (often called "textproto").
    /// </summary>
    /// <remarks>
    /// <para>
    /// Instances of this class are thread-safe, with no mutable state.
    /// </para>
    /// <para>
    /// This parser supports scalar fields (including hexadecimal and octal integer literals,
    /// the <c>inf</c>, <c>infinity</c> and <c>nan</c> floating point literals, and C-style
    /// escaped string and bytes literals), nested messages delimited by <c>{ }</c> or <c>&lt; &gt;</c>,
    /// proto2 groups and editions delimited fields, repeated fields (both as repeated entries and as
    /// <c>[ ]</c> lists), map fields, and <c>#</c> comments.
    /// </para>
    /// <para>
    /// Extension fields and expanded <c>google.protobuf.Any</c> messages (both written as a bracketed
    /// field name such as <c>[type.googleapis.com/pkg.Type]</c>) are not currently supported; encountering
    /// one throws <see cref="NotSupportedException"/> unless <see cref="Settings.IgnoreUnknownFields"/> is set,
    /// in which case the field is skipped.
    /// </para>
    /// </remarks>
    public sealed class TextParser
    {
        private static readonly TextParser defaultInstance = new TextParser(Settings.Default);

        // Strict decoder used to validate the contents of string fields: invalid UTF-8 is an error.
        private static readonly Encoding StrictUtf8 = new UTF8Encoding(encoderShouldEmitUTF8Identifier: false, throwOnInvalidBytes: true);

        private readonly Settings settings;

        /// <summary>
        /// Returns a parser using the default settings (a recursion limit matching
        /// <see cref="CodedInputStream.DefaultRecursionLimit"/>, and rejecting unknown fields).
        /// </summary>
        public static TextParser Default => defaultInstance;

        /// <summary>
        /// Creates a new parser using the given settings.
        /// </summary>
        /// <param name="settings">The settings to use.</param>
        public TextParser(Settings settings)
        {
            this.settings = ProtoPreconditions.CheckNotNull(settings, nameof(settings));
        }

        /// <summary>
        /// Parses the given text format into a new message.
        /// </summary>
        /// <typeparam name="T">The type of message to create.</typeparam>
        /// <param name="text">The text format to parse.</param>
        /// <exception cref="InvalidProtocolBufferException">The text does not represent a Protocol Buffers message correctly.</exception>
        public T Parse<T>(string text) where T : IMessage, new()
        {
            ProtoPreconditions.CheckNotNull(text, nameof(text));
            return Parse<T>(new StringReader(text));
        }

        /// <summary>
        /// Parses text format read from the given text reader into a new message.
        /// </summary>
        /// <typeparam name="T">The type of message to create.</typeparam>
        /// <param name="textReader">Reader providing the text format to parse.</param>
        /// <exception cref="InvalidProtocolBufferException">The text does not represent a Protocol Buffers message correctly.</exception>
        public T Parse<T>(TextReader textReader) where T : IMessage, new()
        {
            ProtoPreconditions.CheckNotNull(textReader, nameof(textReader));
            T message = new T();
            Merge(message, textReader);
            return message;
        }

        /// <summary>
        /// Parses the given text format into a new message of the type described by the given descriptor.
        /// </summary>
        /// <param name="text">The text format to parse.</param>
        /// <param name="descriptor">Descriptor of the message type to create.</param>
        /// <exception cref="InvalidProtocolBufferException">The text does not represent a Protocol Buffers message correctly.</exception>
        public IMessage Parse(string text, MessageDescriptor descriptor)
        {
            ProtoPreconditions.CheckNotNull(text, nameof(text));
            ProtoPreconditions.CheckNotNull(descriptor, nameof(descriptor));
            return Parse(new StringReader(text), descriptor);
        }

        /// <summary>
        /// Parses text format read from the given text reader into a new message of the type described by
        /// the given descriptor.
        /// </summary>
        /// <param name="textReader">Reader providing the text format to parse.</param>
        /// <param name="descriptor">Descriptor of the message type to create.</param>
        /// <exception cref="InvalidProtocolBufferException">The text does not represent a Protocol Buffers message correctly.</exception>
        public IMessage Parse(TextReader textReader, MessageDescriptor descriptor)
        {
            ProtoPreconditions.CheckNotNull(textReader, nameof(textReader));
            ProtoPreconditions.CheckNotNull(descriptor, nameof(descriptor));
            IMessage message = descriptor.Parser.CreateTemplate();
            Merge(message, textReader);
            return message;
        }

        /// <summary>
        /// Parses the given text format and merges the information into the given message.
        /// </summary>
        /// <param name="message">The message to merge the text format information into.</param>
        /// <param name="text">The text format to parse.</param>
        internal void Merge(IMessage message, string text) => Merge(message, new StringReader(text));

        /// <summary>
        /// Parses text format from the given reader and merges the information into the given message.
        /// </summary>
        /// <param name="message">The message to merge the text format information into.</param>
        /// <param name="textReader">Reader providing the text format to parse.</param>
        internal void Merge(IMessage message, TextReader textReader)
        {
            ProtoPreconditions.CheckNotNull(message, nameof(message));
            var tokenizer = new TextTokenizer(textReader);
            MergeMessageBody(message, tokenizer, closingDelimiter: null, depth: 0);
        }

        /// <summary>
        /// Parses a sequence of fields into <paramref name="message"/> until the given closing delimiter
        /// is reached (or the end of input, when parsing the top-level message). The closing delimiter
        /// token is consumed.
        /// </summary>
        private void MergeMessageBody(IMessage message, TextTokenizer tokenizer, TextToken.TokenType? closingDelimiter, int depth)
        {
            if (depth > settings.RecursionLimit)
            {
                throw InvalidProtocolBufferException.TextRecursionLimitExceeded();
            }
            var descriptor = message.Descriptor;
            HashSet<FieldDescriptor> seenSingularFields = null;
            HashSet<OneofDescriptor> seenOneofs = null;

            while (true)
            {
                var token = tokenizer.Next();
                switch (token.Type)
                {
                    case TextToken.TokenType.EndDocument:
                        if (closingDelimiter == null)
                        {
                            return;
                        }
                        throw tokenizer.Error("Unexpected end of input; expected '" + DelimiterText(closingDelimiter.Value) + "'");
                    case TextToken.TokenType.CloseBrace:
                    case TextToken.TokenType.CloseAngle:
                        if (token.Type == closingDelimiter)
                        {
                            return;
                        }
                        throw closingDelimiter == null
                            ? tokenizer.Error("Unexpected '" + token.TextValue + "' at top level")
                            : tokenizer.Error("Expected '" + DelimiterText(closingDelimiter.Value) + "', got '" + token.TextValue + "'");
                    case TextToken.TokenType.OpenBracket:
                        {
                            string bracketedName = tokenizer.ReadBracketedName();
                            if (settings.IgnoreUnknownFields)
                            {
                                SkipFieldValue(tokenizer, bracketedName, depth);
                                ConsumeOptionalSeparator(tokenizer);
                                continue;
                            }
                            throw new NotSupportedException(
                                $"TextParser does not support extension fields or expanded Any messages ([{bracketedName}]).");
                        }
                    case TextToken.TokenType.Identifier:
                        break;
                    default:
                        throw tokenizer.Error("Expected a field name, got " + token);
                }

                string name = token.TextValue;
                var field = FindField(descriptor, name);
                if (field == null)
                {
                    if (settings.IgnoreUnknownFields)
                    {
                        SkipFieldValue(tokenizer, name, depth);
                        ConsumeOptionalSeparator(tokenizer);
                        continue;
                    }
                    throw tokenizer.Error($"Message type \"{descriptor.FullName}\" has no field named \"{name}\"");
                }

                var oneof = field.RealContainingOneof;
                if (oneof != null)
                {
                    seenOneofs ??= new HashSet<OneofDescriptor>();
                    if (!seenOneofs.Add(oneof))
                    {
                        throw tokenizer.Error($"Field \"{name}\" is specified along with another member of oneof \"{oneof.Name}\"");
                    }
                }

                if (field.IsMap)
                {
                    MergeMapField(message, field, tokenizer, depth);
                }
                else if (field.IsRepeated)
                {
                    MergeRepeatedField(message, field, tokenizer, depth);
                }
                else if (IsMessageField(field))
                {
                    tokenizer.TryConsume(TextToken.TokenType.Colon);
                    // Merge into any existing sub-message rather than replacing it.
                    object existing = field.Accessor.GetValue(message);
                    field.Accessor.SetValue(message, ParseMessageFieldValue(field, existing, tokenizer, depth));
                }
                else
                {
                    seenSingularFields ??= new HashSet<FieldDescriptor>();
                    if (!seenSingularFields.Add(field))
                    {
                        throw tokenizer.Error($"Non-repeated field \"{name}\" is specified multiple times");
                    }
                    ExpectColon(tokenizer, name);
                    field.Accessor.SetValue(message, ParseScalarValue(field, tokenizer));
                }

                ConsumeOptionalSeparator(tokenizer);
            }
        }

        /// <summary>
        /// Resolves a field name, including the group-like conventions where a delimited field may be
        /// written using its message type name (e.g. <c>OptionalGroup { ... }</c>).
        /// </summary>
        private static FieldDescriptor FindField(MessageDescriptor descriptor, string name)
        {
            var field = descriptor.FindFieldByName(name);
            if (field == null)
            {
                // Group-like delimited fields accept the capitalized type name as well as the lowercase field name.
                field = descriptor.FindFieldByName(name.ToLowerInvariant());
                if (field != null && !IsGroupLike(field))
                {
                    field = null;
                }
            }
            return field;
        }

        private static bool IsGroupLike(FieldDescriptor field)
        {
            if (field.FieldType != FieldType.Group)
            {
                return false;
            }
            var messageType = field.MessageType;
            // Group fields are always named after their type, lowercased, and the type is declared
            // in the same scope as the field.
            return field.Name == messageType.Name.ToLowerInvariant()
                && ReferenceEquals(messageType.File, field.File)
                && ReferenceEquals(messageType.ContainingType, field.ContainingType);
        }

        private static bool IsMessageField(FieldDescriptor field) =>
            field.FieldType == FieldType.Message || field.FieldType == FieldType.Group;

        /// <summary>
        /// Parses a <c>{ ... }</c> or <c>&lt; ... &gt;</c> block (the opening delimiter being the next token)
        /// into the given message.
        /// </summary>
        private void MergeMessageValue(IMessage message, TextTokenizer tokenizer, int depth)
        {
            var closingDelimiter = ConsumeOpeningDelimiter(tokenizer);
            MergeMessageBody(message, tokenizer, closingDelimiter, depth + 1);
        }

        /// <summary>
        /// Parses a message block for a message-typed field, merging into <paramref name="existing"/>
        /// (which may be null), and returns the value to store in the containing message or collection.
        /// For wrapper types (whose generated CLR representation is the unwrapped value, e.g. <c>int?</c>)
        /// the block is parsed into a temporary wrapper message and its inner value is returned.
        /// </summary>
        private object ParseMessageFieldValue(FieldDescriptor field, object existing, TextTokenizer tokenizer, int depth)
        {
            var messageType = field.MessageType;
            if (messageType.IsWrapperType)
            {
                var valueField = messageType.Fields[WrappersReflection.WrapperValueFieldNumber];
                IMessage wrapper = messageType.Parser.CreateTemplate();
                if (existing != null)
                {
                    valueField.Accessor.SetValue(wrapper, existing);
                }
                MergeMessageValue(wrapper, tokenizer, depth);
                return valueField.Accessor.GetValue(wrapper);
            }
            IMessage subMessage = existing as IMessage ?? messageType.Parser.CreateTemplate();
            MergeMessageValue(subMessage, tokenizer, depth);
            return subMessage;
        }

        private static TextToken.TokenType ConsumeOpeningDelimiter(TextTokenizer tokenizer)
        {
            var token = tokenizer.Next();
            switch (token.Type)
            {
                case TextToken.TokenType.OpenBrace:
                    return TextToken.TokenType.CloseBrace;
                case TextToken.TokenType.OpenAngle:
                    return TextToken.TokenType.CloseAngle;
                default:
                    throw tokenizer.Error("Expected '{' or '<' to start a message value, got " + token);
            }
        }

        /// <summary>
        /// Parses either a bracketed list of values (<c>: [elem, elem]</c>) or a single value for a field.
        /// </summary>
        private static void ParseValuesOrList(TextTokenizer tokenizer, string fieldName, Action<bool> parseElement)
        {
            bool sawColon = tokenizer.TryConsume(TextToken.TokenType.Colon);
            if (tokenizer.TryConsume(TextToken.TokenType.OpenBracket))
            {
                if (!sawColon)
                {
                    throw tokenizer.Error($"Expected ':' before list value for field \"{fieldName}\"");
                }
                if (tokenizer.TryConsume(TextToken.TokenType.CloseBracket))
                {
                    return;
                }
                while (true)
                {
                    parseElement(true);
                    if (tokenizer.TryConsume(TextToken.TokenType.CloseBracket))
                    {
                        return;
                    }
                    if (!tokenizer.TryConsume(TextToken.TokenType.Comma))
                    {
                        throw tokenizer.Error("Expected ',' or ']' in list value, got " + tokenizer.Peek());
                    }
                }
            }
            parseElement(sawColon);
        }

        private void MergeRepeatedField(IMessage message, FieldDescriptor field, TextTokenizer tokenizer, int depth)
        {
            IList list = (IList) field.Accessor.GetValue(message);
            ParseValuesOrList(tokenizer, field.Name, sawColon =>
                list.Add(ParseRepeatedElement(field, tokenizer, depth, sawColon)));
        }

        private object ParseRepeatedElement(FieldDescriptor field, TextTokenizer tokenizer, int depth, bool sawColon)
        {
            if (IsMessageField(field))
            {
                return ParseMessageFieldValue(field, existing: null, tokenizer, depth);
            }
            if (!sawColon)
            {
                throw tokenizer.Error($"Expected ':' after field name \"{field.Name}\"");
            }
            return ParseScalarValue(field, tokenizer);
        }

        private void MergeMapField(IMessage message, FieldDescriptor field, TextTokenizer tokenizer, int depth)
        {
            var entryType = field.MessageType;
            var keyField = entryType.FindFieldByNumber(1);
            var valueField = entryType.FindFieldByNumber(2);
            if (keyField == null || valueField == null)
            {
                throw new InvalidProtocolBufferException("Invalid map field: " + field.FullName);
            }
            IDictionary dictionary = (IDictionary) field.Accessor.GetValue(message);
            ParseValuesOrList(tokenizer, field.Name, _ =>
                MergeMapEntry(dictionary, keyField, valueField, tokenizer, depth));
        }

        /// <summary>
        /// Parses a single <c>{ key: ... value: ... }</c> entry block and stores it in the dictionary.
        /// Missing keys or values take their default value; a repeated key replaces the earlier entry.
        /// </summary>
        private void MergeMapEntry(IDictionary dictionary, FieldDescriptor keyField, FieldDescriptor valueField,
            TextTokenizer tokenizer, int depth)
        {
            var closingDelimiter = ConsumeOpeningDelimiter(tokenizer);
            if (depth + 1 > settings.RecursionLimit)
            {
                throw InvalidProtocolBufferException.TextRecursionLimitExceeded();
            }
            object key = GetDefaultValue(keyField);
            object value = GetDefaultValue(valueField);
            bool seenKey = false;
            bool seenValue = false;

            while (true)
            {
                var token = tokenizer.Next();
                if (token.Type == closingDelimiter)
                {
                    break;
                }
                if (token.Type == TextToken.TokenType.EndDocument)
                {
                    throw tokenizer.Error("Unexpected end of input in map entry; expected '" + DelimiterText(closingDelimiter) + "'");
                }
                if (token.Type != TextToken.TokenType.Identifier)
                {
                    throw tokenizer.Error("Expected 'key' or 'value' in map entry, got " + token);
                }
                if (token.TextValue == keyField.Name)
                {
                    if (seenKey)
                    {
                        throw tokenizer.Error("Non-repeated field \"key\" is specified multiple times");
                    }
                    seenKey = true;
                    ExpectColon(tokenizer, keyField.Name);
                    key = ParseScalarValue(keyField, tokenizer);
                }
                else if (token.TextValue == valueField.Name)
                {
                    if (IsMessageField(valueField))
                    {
                        tokenizer.TryConsume(TextToken.TokenType.Colon);
                        value = ParseMessageFieldValue(valueField, value, tokenizer, depth + 1);
                    }
                    else
                    {
                        if (seenValue)
                        {
                            throw tokenizer.Error("Non-repeated field \"value\" is specified multiple times");
                        }
                        ExpectColon(tokenizer, valueField.Name);
                        value = ParseScalarValue(valueField, tokenizer);
                    }
                    seenValue = true;
                }
                else if (settings.IgnoreUnknownFields)
                {
                    SkipFieldValue(tokenizer, token.TextValue, depth + 1);
                }
                else
                {
                    throw tokenizer.Error($"Map entry has no field named \"{token.TextValue}\"");
                }
                ConsumeOptionalSeparator(tokenizer);
            }
            dictionary[key] = value;
        }

        private void SkipFieldValue(TextTokenizer tokenizer, string fieldName, int depth)
        {
            ParseValuesOrList(tokenizer, fieldName, sawColon =>
            {
                var token = tokenizer.Peek();
                if (token.Type == TextToken.TokenType.OpenBrace || token.Type == TextToken.TokenType.OpenAngle)
                {
                    SkipMessageBlock(tokenizer, depth + 1);
                }
                else
                {
                    if (!sawColon)
                    {
                        throw tokenizer.Error("Expected ':' or '{' or '<' after field name, got " + token);
                    }
                    var scalar = tokenizer.Next();
                    switch (scalar.Type)
                    {
                        case TextToken.TokenType.Identifier:
                        case TextToken.TokenType.String:
                        case TextToken.TokenType.Integer:
                        case TextToken.TokenType.Float:
                            return;
                        default:
                            throw tokenizer.Error("Expected a field value, got " + scalar);
                    }
                }
            });
        }

        private void SkipMessageBlock(TextTokenizer tokenizer, int depth)
        {
            if (depth > settings.RecursionLimit)
            {
                throw InvalidProtocolBufferException.TextRecursionLimitExceeded();
            }
            var closingDelimiter = ConsumeOpeningDelimiter(tokenizer);
            while (true)
            {
                var token = tokenizer.Next();
                if (token.Type == closingDelimiter)
                {
                    return;
                }
                switch (token.Type)
                {
                    case TextToken.TokenType.EndDocument:
                        throw tokenizer.Error("Unexpected end of input inside message block");
                    case TextToken.TokenType.CloseBrace:
                    case TextToken.TokenType.CloseAngle:
                        throw tokenizer.Error("Mismatched closing delimiter " + token);
                    case TextToken.TokenType.Identifier:
                        SkipFieldValue(tokenizer, token.TextValue, depth);
                        break;
                    case TextToken.TokenType.OpenBracket:
                        string bracketedName = tokenizer.ReadBracketedName();
                        SkipFieldValue(tokenizer, bracketedName, depth);
                        break;
                    default:
                        throw tokenizer.Error("Expected a field name, got " + token);
                }
                ConsumeOptionalSeparator(tokenizer);
            }
        }

        private static void ExpectColon(TextTokenizer tokenizer, string fieldName)
        {
            if (!tokenizer.TryConsume(TextToken.TokenType.Colon))
            {
                throw tokenizer.Error($"Expected ':' after field name \"{fieldName}\", got " + tokenizer.Peek());
            }
        }

        private static void ConsumeOptionalSeparator(TextTokenizer tokenizer)
        {
            if (!tokenizer.TryConsume(TextToken.TokenType.Comma))
            {
                tokenizer.TryConsume(TextToken.TokenType.Semicolon);
            }
        }

        private static string DelimiterText(TextToken.TokenType type) =>
            type == TextToken.TokenType.CloseBrace ? "}" : ">";

        #region Scalar value parsing

        /// <summary>
        /// Parses the next token as a value for the given non-message field, returning a boxed value of the
        /// CLR type expected by the field's accessor.
        /// </summary>
        private static object ParseScalarValue(FieldDescriptor field, TextTokenizer tokenizer)
        {
            var token = tokenizer.Next();
            switch (field.FieldType)
            {
                case FieldType.Int32:
                case FieldType.SInt32:
                case FieldType.SFixed32:
                    return (int) ParseSignedInteger(token, tokenizer, int.MinValue, int.MaxValue);
                case FieldType.Int64:
                case FieldType.SInt64:
                case FieldType.SFixed64:
                    return ParseSignedInteger(token, tokenizer, long.MinValue, long.MaxValue);
                case FieldType.UInt32:
                case FieldType.Fixed32:
                    return (uint) ParseUnsignedInteger(token, tokenizer, uint.MaxValue);
                case FieldType.UInt64:
                case FieldType.Fixed64:
                    return ParseUnsignedInteger(token, tokenizer, ulong.MaxValue);
                case FieldType.Float:
                    // Values out of the range of float become infinity, as specified by the text format.
                    return (float) ParseDouble(token, tokenizer);
                case FieldType.Double:
                    return ParseDouble(token, tokenizer);
                case FieldType.Bool:
                    return ParseBool(token, tokenizer);
                case FieldType.String:
                    return ParseString(token, tokenizer);
                case FieldType.Bytes:
                    if (token.Type != TextToken.TokenType.String)
                    {
                        throw tokenizer.Error("Expected a string literal, got " + token);
                    }
                    return ByteString.AttachBytes(token.RawBytes);
                case FieldType.Enum:
                    return ParseEnum(field, token, tokenizer);
                default:
                    throw tokenizer.Error($"Unexpected field type {field.FieldType} for field \"{field.Name}\"");
            }
        }

        private static long ParseSignedInteger(TextToken token, TextTokenizer tokenizer, long min, long max)
        {
            ParseIntegerLiteral(token, tokenizer, out bool negative, out ulong magnitude);
            if (negative)
            {
                // -min is representable as a ulong even for long.MinValue.
                ulong minMagnitude = (ulong) (-(min + 1)) + 1;
                if (magnitude > minMagnitude)
                {
                    throw tokenizer.Error($"Integer out of range: {token.TextValue}");
                }
                return magnitude == minMagnitude ? min : -(long) magnitude;
            }
            if (magnitude > (ulong) max)
            {
                throw tokenizer.Error($"Integer out of range: {token.TextValue}");
            }
            return (long) magnitude;
        }

        private static ulong ParseUnsignedInteger(TextToken token, TextTokenizer tokenizer, ulong max)
        {
            ParseIntegerLiteral(token, tokenizer, out bool negative, out ulong magnitude);
            if (negative)
            {
                throw tokenizer.Error($"Expected an unsigned integer, got {token.TextValue}");
            }
            if (magnitude > max)
            {
                throw tokenizer.Error($"Integer out of range: {token.TextValue}");
            }
            return magnitude;
        }

        /// <summary>
        /// Parses the raw text of an integer token (decimal, <c>0x</c> hexadecimal or leading-zero octal,
        /// with an optional leading minus sign) into a sign and an unsigned magnitude.
        /// </summary>
        private static void ParseIntegerLiteral(TextToken token, TextTokenizer tokenizer, out bool negative, out ulong magnitude)
        {
            if (token.Type != TextToken.TokenType.Integer)
            {
                throw tokenizer.Error("Expected an integer, got " + token);
            }
            string text = token.TextValue;
            int index = 0;
            negative = text[0] == '-';
            if (negative)
            {
                index++;
            }
            uint radix = 10;
            if (text.Length - index > 1 && text[index] == '0')
            {
                if (text[index + 1] == 'x' || text[index + 1] == 'X')
                {
                    radix = 16;
                    index += 2;
                }
                else
                {
                    radix = 8;
                    index++;
                }
            }
            magnitude = 0;
            for (; index < text.Length; index++)
            {
                uint digit = (uint) TextTokenizer.HexValue(text[index]);
                if (magnitude > (ulong.MaxValue - digit) / radix)
                {
                    throw tokenizer.Error($"Integer out of range: {text}");
                }
                magnitude = magnitude * radix + digit;
            }
        }

        private static bool IsDecimalIntegerLiteral(string text)
        {
            int index = text[0] == '-' ? 1 : 0;
            // "0" alone is decimal; "0x..." and "0..." (with more digits) are not.
            return text.Length - index == 1 || text[index] != '0';
        }

        private static double ParseDouble(TextToken token, TextTokenizer tokenizer)
        {
            switch (token.Type)
            {
                case TextToken.TokenType.Identifier:
                case TextToken.TokenType.Float:
                    break;
                case TextToken.TokenType.Integer:
                    if (!IsDecimalIntegerLiteral(token.TextValue))
                    {
                        throw tokenizer.Error($"Hexadecimal and octal literals are not valid floating point values: {token.TextValue}");
                    }
                    break;
                default:
                    throw tokenizer.Error("Expected a floating point value, got " + token);
            }

            // Handle the sign separately so that "-0" reliably yields negative zero.
            string text = token.TextValue;
            bool negative = text[0] == '-';
            if (negative)
            {
                text = text.Substring(1);
            }
            double magnitude;
            string lower = text.ToLowerInvariant();
            if (lower == "inf" || lower == "infinity")
            {
                magnitude = double.PositiveInfinity;
            }
            else if (lower == "nan")
            {
                magnitude = double.NaN;
            }
            else if (token.Type == TextToken.TokenType.Identifier)
            {
                throw tokenizer.Error($"Expected a floating point value, got identifier \"{token.TextValue}\"");
            }
            else
            {
                // Numeric literal, optionally with an 'f' or 'F' suffix.
                if (lower.EndsWith("f"))
                {
                    text = text.Substring(0, text.Length - 1);
                    lower = lower.Substring(0, lower.Length - 1);
                }
                try
                {
                    magnitude = double.Parse(text, NumberStyles.AllowDecimalPoint | NumberStyles.AllowExponent, CultureInfo.InvariantCulture);
                }
                catch (OverflowException)
                {
                    // Older runtimes throw rather than saturating. The text format maps overflow to
                    // infinity and underflow to zero; the sign of the exponent tells us which happened.
                    int exponentIndex = lower.IndexOf('e');
                    bool negativeExponent = exponentIndex >= 0 && exponentIndex + 1 < lower.Length && lower[exponentIndex + 1] == '-';
                    magnitude = negativeExponent ? 0.0 : double.PositiveInfinity;
                }
                catch (FormatException)
                {
                    throw tokenizer.Error($"Invalid floating point value: {token.TextValue}");
                }
            }
            return negative ? -magnitude : magnitude;
        }

        private static bool ParseBool(TextToken token, TextTokenizer tokenizer)
        {
            switch (token.Type)
            {
                case TextToken.TokenType.Identifier:
                    switch (token.TextValue)
                    {
                        case "true":
                        case "True":
                        case "t":
                            return true;
                        case "false":
                        case "False":
                        case "f":
                            return false;
                    }
                    break;
                case TextToken.TokenType.Integer:
                    ParseIntegerLiteral(token, tokenizer, out bool negative, out ulong magnitude);
                    if (!negative && magnitude == 1)
                    {
                        return true;
                    }
                    if (!negative && magnitude == 0)
                    {
                        return false;
                    }
                    break;
            }
            throw tokenizer.Error("Expected a boolean value (true, false, t, f, 1 or 0), got " + token);
        }

        private static string ParseString(TextToken token, TextTokenizer tokenizer)
        {
            if (token.Type != TextToken.TokenType.String)
            {
                throw tokenizer.Error("Expected a string literal, got " + token);
            }
            try
            {
                return StrictUtf8.GetString(token.RawBytes);
            }
            catch (ArgumentException)
            {
                // DecoderFallbackException derives from ArgumentException.
                throw tokenizer.Error("String field contains invalid UTF-8");
            }
        }

        private static object ParseEnum(FieldDescriptor field, TextToken token, TextTokenizer tokenizer)
        {
            switch (token.Type)
            {
                case TextToken.TokenType.Identifier:
                    {
                        var enumValue = field.EnumType.FindValueByName(token.TextValue);
                        if (enumValue == null)
                        {
                            throw tokenizer.Error($"Unknown enum value \"{token.TextValue}\" for enum type {field.EnumType.FullName}");
                        }
                        // Return the number as an int and let the CLR convert it to the enum type.
                        return enumValue.Number;
                    }
                case TextToken.TokenType.Integer:
                    // Deliberately no check that the number is a known value, matching binary parsing of open enums.
                    return (int) ParseSignedInteger(token, tokenizer, int.MinValue, int.MaxValue);
                default:
                    throw tokenizer.Error($"Expected an enum value name or number for field \"{field.Name}\", got " + token);
            }
        }

        /// <summary>
        /// Returns the default value of a field in its CLR representation: a new message for message
        /// fields (or the unwrapped default for wrapper types), and the proto3 default for scalars.
        /// </summary>
        private static object GetDefaultValue(FieldDescriptor field)
        {
            if (IsMessageField(field))
            {
                return field.MessageType.IsWrapperType
                    ? GetDefaultValue(field.MessageType.Fields[WrappersReflection.WrapperValueFieldNumber])
                    : field.MessageType.Parser.CreateTemplate();
            }
            switch (field.FieldType)
            {
                case FieldType.Bool:
                    return false;
                case FieldType.Bytes:
                    return ByteString.Empty;
                case FieldType.String:
                    return "";
                case FieldType.Double:
                    return 0.0;
                case FieldType.Float:
                    return 0f;
                case FieldType.Int32:
                case FieldType.SInt32:
                case FieldType.SFixed32:
                case FieldType.Enum:
                    return 0;
                case FieldType.UInt32:
                case FieldType.Fixed32:
                    return 0U;
                case FieldType.Int64:
                case FieldType.SInt64:
                case FieldType.SFixed64:
                    return 0L;
                case FieldType.UInt64:
                case FieldType.Fixed64:
                    return 0UL;
                default:
                    throw new InvalidProtocolBufferException($"Unexpected field type {field.FieldType} for field \"{field.FullName}\"");
            }
        }

        #endregion

        /// <summary>
        /// Settings controlling text format parsing.
        /// </summary>
        public sealed class Settings
        {
            /// <summary>
            /// Default settings, as used by <see cref="TextParser.Default"/>. This has the same default
            /// recursion limit as <see cref="CodedInputStream"/>, and rejects unknown fields.
            /// </summary>
            public static Settings Default { get; }

            // Workaround for the Mono compiler complaining about XML comments not being on
            // valid language elements.
            static Settings()
            {
                Default = new Settings(CodedInputStream.DefaultRecursionLimit);
            }

            /// <summary>
            /// The maximum depth of messages to parse. Note that this limit only applies to parsing
            /// messages, not collections - so a message within a collection within a message only counts as
            /// depth 2, not 3.
            /// </summary>
            public int RecursionLimit { get; }

            /// <summary>
            /// Whether the parser should ignore unknown fields (<c>true</c>) or throw an exception when
            /// they are encountered (<c>false</c>). When unknown fields are ignored, extension fields and
            /// expanded <c>Any</c> messages (which are not currently supported) are skipped as well.
            /// </summary>
            public bool IgnoreUnknownFields { get; }

            private Settings(int recursionLimit, bool ignoreUnknownFields)
            {
                RecursionLimit = recursionLimit;
                IgnoreUnknownFields = ignoreUnknownFields;
            }

            /// <summary>
            /// Creates a new <see cref="Settings"/> object with the specified recursion limit.
            /// </summary>
            /// <param name="recursionLimit">The maximum depth of messages to parse</param>
            public Settings(int recursionLimit) : this(recursionLimit, false)
            {
            }

            /// <summary>
            /// Creates a new <see cref="Settings"/> object based on this one, but set to either ignore unknown fields,
            /// or throw an exception when unknown fields are encountered.
            /// </summary>
            /// <param name="ignoreUnknownFields"><c>true</c> if unknown fields should be ignored when parsing; <c>false</c> to throw an exception.</param>
            public Settings WithIgnoreUnknownFields(bool ignoreUnknownFields) => new Settings(RecursionLimit, ignoreUnknownFields);

            /// <summary>
            /// Creates a new <see cref="Settings"/> object based on this one, but with the specified recursion limit.
            /// </summary>
            /// <param name="recursionLimit">The new recursion limit.</param>
            public Settings WithRecursionLimit(int recursionLimit) => new Settings(recursionLimit, IgnoreUnknownFields);
        }
    }
}
