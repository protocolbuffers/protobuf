#region Copyright notice and license
// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd
#endregion

namespace Google.Protobuf
{
    /// <summary>
    /// A single lexical token produced by <see cref="TextTokenizer"/> when reading
    /// Protocol Buffers text format.
    /// </summary>
    internal readonly struct TextToken
    {
        internal enum TokenType
        {
            /// <summary>End of input.</summary>
            EndDocument,
            /// <summary>An identifier such as a field name, enum value name, or a keyword like <c>true</c>.</summary>
            Identifier,
            /// <summary>
            /// A string literal (possibly several adjacent literals concatenated), already unescaped
            /// into <see cref="RawBytes"/>.
            /// </summary>
            String,
            /// <summary>An integer literal. <see cref="TextValue"/> holds the raw (unparsed) source text.</summary>
            Integer,
            /// <summary>A floating point literal. <see cref="TextValue"/> holds the raw (unparsed) source text.</summary>
            Float,
            /// <summary>The <c>:</c> character.</summary>
            Colon,
            /// <summary>The <c>{</c> character.</summary>
            OpenBrace,
            /// <summary>The <c>}</c> character.</summary>
            CloseBrace,
            /// <summary>The <c>&lt;</c> character.</summary>
            OpenAngle,
            /// <summary>The <c>&gt;</c> character.</summary>
            CloseAngle,
            /// <summary>The <c>[</c> character.</summary>
            OpenBracket,
            /// <summary>The <c>]</c> character.</summary>
            CloseBracket,
            /// <summary>The <c>,</c> character.</summary>
            Comma,
            /// <summary>The <c>;</c> character.</summary>
            Semicolon,
        }

        internal static readonly TextToken EndDocument = new TextToken(TokenType.EndDocument, "");
        internal static readonly TextToken Colon = new TextToken(TokenType.Colon, ":");
        internal static readonly TextToken OpenBrace = new TextToken(TokenType.OpenBrace, "{");
        internal static readonly TextToken CloseBrace = new TextToken(TokenType.CloseBrace, "}");
        internal static readonly TextToken OpenAngle = new TextToken(TokenType.OpenAngle, "<");
        internal static readonly TextToken CloseAngle = new TextToken(TokenType.CloseAngle, ">");
        internal static readonly TextToken OpenBracket = new TextToken(TokenType.OpenBracket, "[");
        internal static readonly TextToken CloseBracket = new TextToken(TokenType.CloseBracket, "]");
        internal static readonly TextToken Comma = new TextToken(TokenType.Comma, ",");
        internal static readonly TextToken Semicolon = new TextToken(TokenType.Semicolon, ";");

        internal static TextToken Identifier(string name) => new TextToken(TokenType.Identifier, name);
        internal static TextToken Integer(string rawText) => new TextToken(TokenType.Integer, rawText);
        internal static TextToken Float(string rawText) => new TextToken(TokenType.Float, rawText);
        internal static TextToken String(byte[] rawBytes) => new TextToken(TokenType.String, null, rawBytes);

        internal TokenType Type { get; }
        internal string TextValue { get; }
        internal byte[] RawBytes { get; }

        private TextToken(TokenType type, string textValue, byte[] rawBytes = null)
        {
            Type = type;
            TextValue = textValue;
            RawBytes = rawBytes;
        }

        public override string ToString() => Type switch
        {
            TokenType.EndDocument => "end of input",
            TokenType.String => "string literal",
            _ => TextValue,
        };
    }
}
