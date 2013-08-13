using System;
using System.Collections;
using System.Collections.Generic;
using System.IO;
using System.Text;
using Google.ProtocolBuffers.Descriptors;

namespace Google.ProtocolBuffers.Serialization
{
    /// <summary>
    /// JsonFormatWriter is a .NET 2.0 friendly json formatter for proto buffer messages.  For .NET 3.5
    /// you may also use the XmlFormatWriter with an XmlWriter created by the
    /// <see cref="System.Runtime.Serialization.Json.JsonReaderWriterFactory">JsonReaderWriterFactory</see>.
    /// </summary>
    public abstract class JsonFormatWriter : AbstractTextWriter
    {
        #region buffering implementations

        private class JsonTextWriter : JsonFormatWriter
        {
            private readonly char[] _buffer;
            private TextWriter _output;
            private int _bufferPos;

            public JsonTextWriter(TextWriter output)
            {
                _buffer = new char[4096];
                _bufferPos = 0;
                _output = output;
                _counter.Add(0);
            }

            /// <summary>
            /// Returns the output of TextWriter.ToString() where TextWriter is the ctor argument.
            /// </summary>
            public override string ToString()
            {
                Flush();

                if (_output != null)
                {
                    return _output.ToString();
                }

                return new String(_buffer, 0, _bufferPos);
            }

            protected override void WriteToOutput(char[] chars, int offset, int len)
            {
                if (_bufferPos + len >= _buffer.Length)
                {
                    if (_output == null)
                    {
                        _output = new StringWriter(new StringBuilder(_buffer.Length*2 + len));
                    }
                    Flush();
                }

                if (len < _buffer.Length)
                {
                    if (len <= 12)
                    {
                        int stop = offset + len;
                        for (int i = offset; i < stop; i++)
                        {
                            _buffer[_bufferPos++] = chars[i];
                        }
                    }
                    else
                    {
                        Buffer.BlockCopy(chars, offset << 1, _buffer, _bufferPos << 1, len << 1);
                        _bufferPos += len;
                    }
                }
                else
                {
                    _output.Write(chars, offset, len);
                }
            }

            protected override void WriteToOutput(char ch)
            {
                if (_bufferPos >= _buffer.Length)
                {
                    if (_output == null)
                    {
                        _output = new StringWriter(new StringBuilder(_buffer.Length * 2));
                    }
                    Flush();
                }
                _buffer[_bufferPos++] = ch;
            }

            public override void Flush()
            {
                if (_bufferPos > 0 && _output != null)
                {
                    _output.Write(_buffer, 0, _bufferPos);
                    _bufferPos = 0;
                }
                base.Flush();
            }
        }

        private class JsonStreamWriter : JsonFormatWriter
        {
            static readonly Encoding Encoding = new UTF8Encoding(false);
            private readonly byte[] _buffer;
            private Stream _output;
            private int _bufferPos;

            public JsonStreamWriter(Stream output)
            {
                _buffer = new byte[8192];
                _bufferPos = 0;
                _output = output;
                _counter.Add(0);
            }

            protected override void WriteToOutput(char[] chars, int offset, int len)
            {
                if (_bufferPos + len >= _buffer.Length)
                {
                    Flush();
                }

                if (len < _buffer.Length)
                {
                    if (len <= 12)
                    {
                        int stop = offset + len;
                        for (int i = offset; i < stop; i++)
                        {
                            _buffer[_bufferPos++] = (byte) chars[i];
                        }
                    }
                    else
                    {
                        _bufferPos += Encoding.GetBytes(chars, offset, len, _buffer, _bufferPos);
                    }
                }
                else
                {
                    byte[] temp = Encoding.GetBytes(chars, offset, len);
                    _output.Write(temp, 0, temp.Length);
                }
            }

            protected override void WriteToOutput(char ch)
            {
                if (_bufferPos >= _buffer.Length)
                {
                    Flush();
                }
                _buffer[_bufferPos++] = (byte) ch;
            }

            public override void Flush()
            {
                if (_bufferPos > 0 && _output != null)
                {
                    _output.Write(_buffer, 0, _bufferPos);
                    _bufferPos = 0;
                }
                base.Flush();
            }
        }

        #endregion

        //Tracks the writer depth and the array element count at that depth.
        private readonly List<int> _counter;
        //True if the top-level of the writer is an array as opposed to a single message.
        private bool _isArray;

        /// <summary>
        /// Constructs a JsonFormatWriter, use the ToString() member to extract the final Json on completion.
        /// </summary>
        protected JsonFormatWriter()
        {
            _counter = new List<int>();
        }

        /// <summary>
        /// Constructs a JsonFormatWriter, use ToString() to extract the final output
        /// </summary>
        public static JsonFormatWriter CreateInstance()
        {
            return new JsonTextWriter(null);
        }

        /// <summary>
        /// Constructs a JsonFormatWriter to output to the given text writer
        /// </summary>
        public static JsonFormatWriter CreateInstance(TextWriter output)
        {
            return new JsonTextWriter(output);
        }

        /// <summary>
        /// Constructs a JsonFormatWriter to output to the given stream
        /// </summary>
        public static JsonFormatWriter CreateInstance(Stream output)
        {
            return new JsonStreamWriter(output);
        }

        /// <summary> Write to the output stream </summary>
        protected void WriteToOutput(string format, params object[] args)
        {
            WriteToOutput(String.Format(format, args));
        }

        /// <summary> Write to the output stream </summary>
        protected void WriteToOutput(string text)
        {
            WriteToOutput(text.ToCharArray(), 0, text.Length);
        }

        /// <summary> Write to the output stream </summary>
        protected abstract void WriteToOutput(char ch);

        /// <summary> Write to the output stream </summary>
        protected abstract void WriteToOutput(char[] chars, int offset, int len);

        /// <summary> Sets the output formatting to use Environment.NewLine with 4-character indentions </summary>
        public JsonFormatWriter Formatted()
        {
            NewLine = FrameworkPortability.NewLine;
            Indent = "    ";
            Whitespace = " ";
            return this;
        }

        /// <summary> Gets or sets the characters to use for the new-line, default = empty </summary>
        public string NewLine { get; set; }

        /// <summary> Gets or sets the text to use for indenting, default = empty </summary>
        public string Indent { get; set; }

        /// <summary> Gets or sets the whitespace to use to separate the text, default = empty </summary>
        public string Whitespace { get; set; }

        private void Seperator()
        {
            if (_counter.Count == 0)
            {
                throw new InvalidOperationException("Mismatched open/close in Json writer.");
            }

            int index = _counter.Count - 1;
            if (_counter[index] > 0)
            {
                WriteToOutput(',');
            }

            WriteLine(String.Empty);
            _counter[index] = _counter[index] + 1;
        }

        private void WriteLine(string content)
        {
            if (!String.IsNullOrEmpty(NewLine))
            {
                WriteToOutput(NewLine);
                for (int i = 1; i < _counter.Count; i++)
                {
                    WriteToOutput(Indent);
                }
            }
            else if (!String.IsNullOrEmpty(Whitespace))
            {
                WriteToOutput(Whitespace);
            }

            WriteToOutput(content);
        }

        private void WriteName(string field)
        {
            Seperator();
            if (!String.IsNullOrEmpty(field))
            {
                WriteToOutput('"');
                WriteToOutput(field);
                WriteToOutput('"');
                WriteToOutput(':');
                if (!String.IsNullOrEmpty(Whitespace))
                {
                    WriteToOutput(Whitespace);
                }
            }
        }

        private void EncodeText(string value)
        {
            char[] text = value.ToCharArray();
            int len = text.Length;
            int pos = 0;

            while (pos < len)
            {
                int next = pos;
                while (next < len && text[next] >= 32 && text[next] < 127 && text[next] != '\\' && text[next] != '/' &&
                       text[next] != '"')
                {
                    next++;
                }
                WriteToOutput(text, pos, next - pos);
                if (next < len)
                {
                    switch (text[next])
                    {
                        case '"':
                            WriteToOutput(@"\""");
                            break;
                        case '\\':
                            WriteToOutput(@"\\");
                            break;
                            //odd at best to escape '/', most Json implementations don't, but it is defined in the rfc-4627
                        case '/':
                            WriteToOutput(@"\/");
                            break;
                        case '\b':
                            WriteToOutput(@"\b");
                            break;
                        case '\f':
                            WriteToOutput(@"\f");
                            break;
                        case '\n':
                            WriteToOutput(@"\n");
                            break;
                        case '\r':
                            WriteToOutput(@"\r");
                            break;
                        case '\t':
                            WriteToOutput(@"\t");
                            break;
                        default:
                            WriteToOutput(@"\u{0:x4}", (int) text[next]);
                            break;
                    }
                    next++;
                }
                pos = next;
            }
        }

        /// <summary>
        /// Writes a String value
        /// </summary>
        protected override void WriteAsText(string field, string textValue, object typedValue)
        {
            WriteName(field);
            if (typedValue is bool || typedValue is int || typedValue is uint || typedValue is long ||
                typedValue is ulong || typedValue is double || typedValue is float)
            {
                WriteToOutput(textValue);
            }
            else
            {
                WriteToOutput('"');
                if (typedValue is string)
                {
                    EncodeText(textValue);
                }
                else
                {
                    WriteToOutput(textValue);
                }
                WriteToOutput('"');
            }
        }

        /// <summary>
        /// Writes a Double value
        /// </summary>
        protected override void Write(string field, double value)
        {
            if (double.IsNaN(value) || double.IsNegativeInfinity(value) || double.IsPositiveInfinity(value))
            {
                throw new InvalidOperationException("This format does not support NaN, Infinity, or -Infinity");
            }
            base.Write(field, value);
        }

        /// <summary>
        /// Writes a Single value
        /// </summary>
        protected override void Write(string field, float value)
        {
            if (float.IsNaN(value) || float.IsNegativeInfinity(value) || float.IsPositiveInfinity(value))
            {
                throw new InvalidOperationException("This format does not support NaN, Infinity, or -Infinity");
            }
            base.Write(field, value);
        }

        // Treat enum as string
        protected override void WriteEnum(string field, int number, string name)
        {
            Write(field, name);
        }

        /// <summary>
        /// Writes an array of field values
        /// </summary>
        protected override void WriteArray(FieldType type, string field, IEnumerable items)
        {
            IEnumerator enumerator = items.GetEnumerator();
            try
            {
                if (!enumerator.MoveNext())
                {
                    return;
                }
            }
            finally
            {
                if (enumerator is IDisposable)
                {
                    ((IDisposable) enumerator).Dispose();
                }
            }

            WriteName(field);
            WriteToOutput("[");
            _counter.Add(0);

            base.WriteArray(type, String.Empty, items);

            _counter.RemoveAt(_counter.Count - 1);
            WriteLine("]");
        }

        /// <summary>
        /// Writes a message
        /// </summary>
        protected override void WriteMessageOrGroup(string field, IMessageLite message)
        {
            WriteName(field);
            WriteMessage(message);
        }

        /// <summary>
        /// Writes the message to the the formatted stream.
        /// </summary>
        public override void WriteMessage(IMessageLite message)
        {
            WriteMessageStart();
            message.WriteTo(this);
            WriteMessageEnd();
        }

        /// <summary>
        /// Used to write the root-message preamble, in json this is the left-curly brace '{'.
        /// After this call you can call IMessageLite.MergeTo(...) and complete the message with
        /// a call to WriteMessageEnd().
        /// </summary>
        public override void WriteMessageStart()
        {
            if (_isArray)
            {
                Seperator();
            }
            WriteToOutput("{");
            _counter.Add(0);
        }

        /// <summary>
        /// Used to complete a root-message previously started with a call to WriteMessageStart()
        /// </summary>
        public override void WriteMessageEnd()
        {
            _counter.RemoveAt(_counter.Count - 1);
            WriteLine("}");
            Flush();
        }

        /// <summary>
        /// Used in streaming arrays of objects to the writer
        /// </summary>
        /// <example>
        /// <code>
        /// using(writer.StartArray())
        ///     foreach(IMessageLite m in messages)
        ///         writer.WriteMessage(m);
        /// </code>
        /// </example>
        public sealed class JsonArray : IDisposable
        {
            private JsonFormatWriter _writer;

            internal JsonArray(JsonFormatWriter writer)
            {
                _writer = writer;
                _writer.WriteToOutput("[");
                _writer._counter.Add(0);
            }

            /// <summary>
            /// Causes the end of the array character to be written.
            /// </summary>
            private void EndArray()
            {
                if (_writer != null)
                {
                    _writer._counter.RemoveAt(_writer._counter.Count - 1);
                    _writer.WriteLine("]");
                    _writer.Flush();
                }
                _writer = null;
            }

            void IDisposable.Dispose()
            {
                EndArray();
            }
        }

        /// <summary>
        /// Used to write an array of messages as the output rather than a single message.
        /// </summary>
        /// <example>
        /// <code>
        /// using(writer.StartArray())
        ///     foreach(IMessageLite m in messages)
        ///         writer.WriteMessage(m);
        /// </code>
        /// </example>
        public JsonArray StartArray()
        {
            if (_isArray)
            {
                Seperator();
            }
            _isArray = true;
            return new JsonArray(this);
        }
    }
}