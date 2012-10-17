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
using System.Collections.Generic;
using System.Globalization;
using System.IO;
using System.Text;

namespace Google.ProtocolBuffers
{
    /// <summary>
    /// All generated protocol message classes extend this class. It implements
    /// most of the IMessage interface using reflection. Users
    /// can ignore this class as an implementation detail.
    /// </summary>
    public abstract partial class GeneratedMessageLite<TMessage, TBuilder> : AbstractMessageLite<TMessage, TBuilder>
        where TMessage : GeneratedMessageLite<TMessage, TBuilder>
        where TBuilder : GeneratedBuilderLite<TMessage, TBuilder>
    {
        protected abstract TMessage ThisMessage { get; }

        public override sealed string ToString()
        {
            using (StringWriter wtr = new StringWriter())
            {
                PrintTo(wtr);
                return wtr.ToString();
            }
        }

        /// <summary>
        /// PrintTo() helper methods for Lite Runtime
        /// </summary>
        protected static void PrintField<T>(string name, IList<T> value, TextWriter writer)
        {
            foreach (T item in value)
            {
                PrintField(name, true, (object) item, writer);
            }
        }

        /// <summary>
        /// PrintTo() helper methods for Lite Runtime
        /// </summary>
        protected static void PrintField(string name, bool hasValue, object value, TextWriter writer)
        {
            if (!hasValue)
            {
                return;
            }
            if (value is IMessageLite)
            {
                writer.WriteLine("{0} {{", name);
                ((IMessageLite) value).PrintTo(writer);
                writer.WriteLine("}");
            }
            else if (value is ByteString || value is String)
            {
                writer.Write("{0}: \"", name);
                if (value is String)
                {
                    EscapeBytes(Encoding.UTF8.GetBytes((string) value), writer);
                }
                else
                {
                    EscapeBytes(((ByteString) value), writer);
                }
                writer.WriteLine("\"");
            }
            else if (value is bool)
            {
                writer.WriteLine("{0}: {1}", name, (bool) value ? "true" : "false");
            }
            else if (value is IEnumLite)
            {
                writer.WriteLine("{0}: {1}", name, ((IEnumLite) value).Name);
            }
            else
            {
                writer.WriteLine("{0}: {1}", name, ((IConvertible)value).ToString(FrameworkPortability.InvariantCulture));
            }
        }

        /// <summary>
        /// COPIED from TextFormat
        /// Escapes bytes in the format used in protocol buffer text format, which
        /// is the same as the format used for C string literals.  All bytes
        /// that are not printable 7-bit ASCII characters are escaped, as well as
        /// backslash, single-quote, and double-quote characters.  Characters for
        /// which no defined short-hand escape sequence is defined will be escaped
        /// using 3-digit octal sequences.
        /// The returned value is guaranteed to be entirely ASCII.
        /// </summary>
        private static void EscapeBytes(IEnumerable<byte> input, TextWriter writer)
        {
            foreach (byte b in input)
            {
                switch (b)
                {
                        // C# does not use \a or \v
                    case 0x07:
                        writer.Write("\\a");
                        break;
                    case (byte) '\b':
                        writer.Write("\\b");
                        break;
                    case (byte) '\f':
                        writer.Write("\\f");
                        break;
                    case (byte) '\n':
                        writer.Write("\\n");
                        break;
                    case (byte) '\r':
                        writer.Write("\\r");
                        break;
                    case (byte) '\t':
                        writer.Write("\\t");
                        break;
                    case 0x0b:
                        writer.Write("\\v");
                        break;
                    case (byte) '\\':
                        writer.Write("\\\\");
                        break;
                    case (byte) '\'':
                        writer.Write("\\\'");
                        break;
                    case (byte) '"':
                        writer.Write("\\\"");
                        break;
                    default:
                        if (b >= 0x20 && b < 128)
                        {
                            writer.Write((char) b);
                        }
                        else
                        {
                            writer.Write('\\');
                            writer.Write((char) ('0' + ((b >> 6) & 3)));
                            writer.Write((char) ('0' + ((b >> 3) & 7)));
                            writer.Write((char) ('0' + (b & 7)));
                        }
                        break;
                }
            }
        }
    }
}