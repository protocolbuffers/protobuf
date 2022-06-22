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

namespace Google.Protobuf
{
    internal sealed class JsonToken : IEquatable<JsonToken>
    {
        internal static JsonToken Null { get; } = new JsonToken(TokenType.Null);
        internal static JsonToken False { get; } = new JsonToken(TokenType.False);
        internal static JsonToken True { get; } = new JsonToken(TokenType.True);
        internal static JsonToken StartObject { get; } = new JsonToken(TokenType.StartObject);
        internal static JsonToken EndObject { get; } = new JsonToken(TokenType.EndObject);
        internal static JsonToken StartArray { get; } = new JsonToken(TokenType.StartArray);
        internal static JsonToken EndArray { get; } = new JsonToken(TokenType.EndArray);
        internal static JsonToken EndDocument { get; } = new JsonToken(TokenType.EndDocument);

        internal static JsonToken Name(string name)
        {
            return new JsonToken(TokenType.Name, stringValue: name);
        }

        internal static JsonToken Value(string value)
        {
            return new JsonToken(TokenType.StringValue, stringValue: value);
        }

        internal static JsonToken Value(double value)
        {
            return new JsonToken(TokenType.Number, numberValue: value);
        }

        internal enum TokenType
        {
            Null,
            False,
            True,
            StringValue,
            Number,
            Name,
            StartObject,
            EndObject,
            StartArray,
            EndArray,
            EndDocument
        }

        // A value is a string, number, array, object, null, true or false
        // Arrays and objects have start/end
        // A document consists of a value
        // Objects are name/value sequences.

        private readonly TokenType type;
        private readonly string stringValue;
        private readonly double numberValue;

        internal TokenType Type => type;
        internal string StringValue => stringValue;
        internal double NumberValue => numberValue;

        private JsonToken(TokenType type, string stringValue = null, double numberValue = 0)
        {
            this.type = type;
            this.stringValue = stringValue;
            this.numberValue = numberValue;
        }

        public override bool Equals(object obj) => Equals(obj as JsonToken);

        public override int GetHashCode()
        {
            unchecked
            {
                int hash = 17;
                hash = hash * 31 + (int) type;
                hash = hash * 31 + stringValue == null ? 0 : stringValue.GetHashCode();
                hash = hash * 31 + numberValue.GetHashCode();
                return hash;
            }
        }

        public override string ToString()
        {
            return type switch
            {
                TokenType.Null => "null",
                TokenType.True => "true",
                TokenType.False => "false",
                TokenType.Name => $"name ({stringValue})",
                TokenType.StringValue => $"value ({stringValue})",
                TokenType.Number => $"number ({numberValue})",
                TokenType.StartObject => "start-object",
                TokenType.EndObject => "end-object",
                TokenType.StartArray => "start-array",
                TokenType.EndArray => "end-array",
                TokenType.EndDocument => "end-document",
                _ => throw new InvalidOperationException($"Token is of unknown type {type}"),
            };
        }

        public bool Equals(JsonToken other)
        {
            if (other is null)
            {
                return false;
            }
            // Note use of other.numberValue.Equals rather than ==, so that NaN compares appropriately.
            return other.type == type && other.stringValue == stringValue && other.numberValue.Equals(numberValue);
        }
    }
}
