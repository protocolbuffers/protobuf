using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Globalization;
using System.IO;

namespace Google.ProtocolBuffers.Serialization
{
    /// <summary>
    /// JSon Tokenizer used by JsonFormatReader
    /// </summary>
    internal abstract class JsonCursor
    {
        public enum JsType
        {
            String,
            Number,
            Object,
            Array,
            True,
            False,
            Null
        }

        #region Buffering implementations

        private class JsonStreamCursor : JsonCursor
        {
            private readonly byte[] _buffer;
            private int _bufferPos;
            private readonly Stream _input;

            public JsonStreamCursor(Stream input)
            {
                _input = input;
                _next = _input.ReadByte();
            }

            public JsonStreamCursor(byte[] input)
            {
                _input = null;
                _buffer = input;
                _next = _buffer[_bufferPos];
            }

            protected override int Peek()
            {
                if (_input != null)
                {
                    return _next;
                }
                else if (_bufferPos < _buffer.Length)
                {
                    return _buffer[_bufferPos];
                }
                else
                {
                    return -1;
                }
            }

            protected override int Read()
            {
                if (_input != null)
                {
                    int result = _next;
                    _next = _input.ReadByte();
                    return result;
                }
                else if (_bufferPos < _buffer.Length)
                {
                    return _buffer[_bufferPos++];
                }
                else
                {
                    return -1;
                }
            }
        }

        private class JsonTextCursor : JsonCursor
        {
            private readonly char[] _buffer;
            private int _bufferPos;
            private readonly TextReader _input;

            public JsonTextCursor(char[] input)
            {
                _input = null;
                _buffer = input;
                _bufferPos = 0;
                _next = Peek();
            }

            public JsonTextCursor(TextReader input)
            {
                _input = input;
                _next = Peek();
            }

            protected override int Peek()
            {
                if (_input != null)
                {
                    return _input.Peek();
                }
                else if (_bufferPos < _buffer.Length)
                {
                    return _buffer[_bufferPos];
                }
                else
                {
                    return -1;
                }
            }

            protected override int Read()
            {
                if (_input != null)
                {
                    return _input.Read();
                }
                else if (_bufferPos < _buffer.Length)
                {
                    return _buffer[_bufferPos++];
                }
                else
                {
                    return -1;
                }
            }
        }

        #endregion

        protected int _next;
        private int _lineNo, _linePos;

        public static JsonCursor CreateInstance(byte[] input)
        {
            return new JsonStreamCursor(input);
        }

        public static JsonCursor CreateInstance(Stream input)
        {
            return new JsonStreamCursor(input);
        }

        public static JsonCursor CreateInstance(string input)
        {
            return new JsonTextCursor(input.ToCharArray());
        }

        public static JsonCursor CreateInstance(TextReader input)
        {
            return new JsonTextCursor(input);
        }

        protected JsonCursor()
        {
            _lineNo = 1;
            _linePos = 0;
        }

        /// <summary>Returns the next character without actually 'reading' it</summary>
        protected abstract int Peek();

        /// <summary>Reads the next character in the input</summary>
        protected abstract int Read();

        public Char NextChar
        {
            get
            {
                SkipWhitespace();
                return (char) _next;
            }
        }

        #region Assert(...)

        [DebuggerNonUserCode]
        private string CharDisplay(int ch)
        {
            return ch == -1
                       ? "EOF"
                       : (ch > 32 && ch < 127)
                             ? String.Format("'{0}'", (char) ch)
                             : String.Format("'\\u{0:x4}'", ch);
        }

        [DebuggerNonUserCode]
        private void Assert(bool cond, char expected)
        {
            if (!cond)
            {
                throw new FormatException(
                    String.Format(FrameworkPortability.InvariantCulture,
                                  "({0}:{1}) error: Unexpected token {2}, expected: {3}.",
                                  _lineNo, _linePos,
                                  CharDisplay(_next),
                                  CharDisplay(expected)
                        ));
            }
        }

        [DebuggerNonUserCode]
        public void Assert(bool cond, string message)
        {
            if (!cond)
            {
                throw new FormatException(
                    String.Format(FrameworkPortability.InvariantCulture,
                                  "({0},{1}) error: {2}", _lineNo, _linePos, message));
            }
        }

        [DebuggerNonUserCode]
        public void Assert(bool cond, string format, params object[] args)
        {
            if (!cond)
            {
                if (args != null && args.Length > 0)
                {
                    format = String.Format(format, args);
                }
                throw new FormatException(
                    String.Format(FrameworkPortability.InvariantCulture,
                                  "({0},{1}) error: {2}", _lineNo, _linePos, format));
            }
        }

        #endregion

        private char ReadChar()
        {
            int ch = Read();
            Assert(ch != -1, "Unexpected end of file.");
            if (ch == '\n')
            {
                _lineNo++;
                _linePos = 0;
            }
            else if (ch != '\r')
            {
                _linePos++;
            }
            _next = Peek();
            return (char) ch;
        }

        public void Consume(char ch)
        {
            Assert(TryConsume(ch), ch);
        }

        public bool TryConsume(char ch)
        {
            SkipWhitespace();
            if (_next == ch)
            {
                ReadChar();
                return true;
            }
            return false;
        }

        public void Consume(string sequence)
        {
            SkipWhitespace();

            foreach (char ch in sequence)
            {
                Assert(ch == ReadChar(), "Expected token '{0}'.", sequence);
            }
        }

        public void SkipWhitespace()
        {
            int chnext = _next;
            while (chnext != -1)
            {
                if (!Char.IsWhiteSpace((char) chnext))
                {
                    break;
                }
                ReadChar();
                chnext = _next;
            }
        }

        public string ReadString()
        {
            SkipWhitespace();
            Consume('"');
            List<Char> sb = new List<char>(100);
            while (_next != '"')
            {
                if (_next == '\\')
                {
                    Consume('\\'); //skip the escape
                    char ch = ReadChar();
                    switch (ch)
                    {
                        case 'b':
                            sb.Add('\b');
                            break;
                        case 'f':
                            sb.Add('\f');
                            break;
                        case 'n':
                            sb.Add('\n');
                            break;
                        case 'r':
                            sb.Add('\r');
                            break;
                        case 't':
                            sb.Add('\t');
                            break;
                        case 'u':
                            {
                                string hex = new string(new char[] {ReadChar(), ReadChar(), ReadChar(), ReadChar()});
                                int result;
                                Assert(
                                    FrameworkPortability.TryParseInt32(hex, NumberStyles.AllowHexSpecifier, FrameworkPortability.InvariantCulture,
                                                 out result),
                                    "Expected a 4-character hex specifier.");
                                sb.Add((char) result);
                                break;
                            }
                        default:
                            sb.Add(ch);
                            break;
                    }
                }
                else
                {
                    Assert(_next != '\n' && _next != '\r' && _next != '\f' && _next != -1, '"');
                    sb.Add(ReadChar());
                }
            }
            Consume('"');
            return new String(sb.ToArray());
        }

        public string ReadNumber()
        {
            SkipWhitespace();
            List<Char> sb = new List<char>(24);
            if (_next == '-')
            {
                sb.Add(ReadChar());
            }
            Assert(_next >= '0' && _next <= '9', "Expected a numeric type.");
            while ((_next >= '0' && _next <= '9') || _next == '.')
            {
                sb.Add(ReadChar());
            }
            if (_next == 'e' || _next == 'E')
            {
                sb.Add(ReadChar());
                if (_next == '-' || _next == '+')
                {
                    sb.Add(ReadChar());
                }
                Assert(_next >= '0' && _next <= '9', "Expected a numeric type.");
                while (_next >= '0' && _next <= '9')
                {
                    sb.Add(ReadChar());
                }
            }
            return new String(sb.ToArray());
        }

        public JsType ReadVariant(out object value)
        {
            SkipWhitespace();
            switch (_next)
            {
                case 'n':
                    Consume("null");
                    value = null;
                    return JsType.Null;
                case 't':
                    Consume("true");
                    value = true;
                    return JsType.True;
                case 'f':
                    Consume("false");
                    value = false;
                    return JsType.False;
                case '"':
                    value = ReadString();
                    return JsType.String;
                case '{':
                    {
                        Consume('{');
                        while (NextChar != '}')
                        {
                            ReadString();
                            Consume(':');
                            object tmp;
                            ReadVariant(out tmp);
                            if (!TryConsume(','))
                            {
                                break;
                            }
                        }
                        Consume('}');
                        value = null;
                        return JsType.Object;
                    }
                case '[':
                    {
                        Consume('[');
                        List<object> values = new List<object>();
                        while (NextChar != ']')
                        {
                            object tmp;
                            ReadVariant(out tmp);
                            values.Add(tmp);
                            if (!TryConsume(','))
                            {
                                break;
                            }
                        }
                        Consume(']');
                        value = values.ToArray();
                        return JsType.Array;
                    }
                default:
                    if ((_next >= '0' && _next <= '9') || _next == '-')
                    {
                        value = ReadNumber();
                        return JsType.Number;
                    }
                    Assert(false, "Expected a value.");
                    throw new FormatException();
            }
        }
    }
}