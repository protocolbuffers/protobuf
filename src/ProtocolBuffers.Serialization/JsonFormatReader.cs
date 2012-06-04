using System;
using System.Collections.Generic;
using System.IO;
using System.Xml;

namespace Google.ProtocolBuffers.Serialization
{
    /// <summary>
    /// JsonFormatReader is used to parse Json into a message or an array of messages
    /// </summary>
    public class JsonFormatReader : AbstractTextReader
    {
        private readonly JsonCursor _input;
        // The expected token that ends the current item, either ']' or '}'
        private readonly Stack<int> _stopChar;

        private enum ReaderState
        {
            Start,
            BeginValue,
            EndValue,
            BeginObject,
            BeginArray
        }

        private string _current;
        private ReaderState _state;

        /// <summary>
        /// Constructs a JsonFormatReader to parse Json into a message, this method does not use text encoding, all bytes MUST
        /// represent ASCII character values.
        /// </summary>
        public static JsonFormatReader CreateInstance(Stream stream)
        {
            return new JsonFormatReader(JsonCursor.CreateInstance(stream));
        }

        /// <summary>
        /// Constructs a JsonFormatReader to parse Json into a message, this method does not use text encoding, all bytes MUST
        /// represent ASCII character values.
        /// </summary>
        public static JsonFormatReader CreateInstance(byte[] bytes)
        {
            return new JsonFormatReader(JsonCursor.CreateInstance(bytes));
        }

        /// <summary>
        /// Constructs a JsonFormatReader to parse Json into a message
        /// </summary>
        public static JsonFormatReader CreateInstance(string jsonText)
        {
            return new JsonFormatReader(JsonCursor.CreateInstance(jsonText));
        }

        /// <summary>
        /// Constructs a JsonFormatReader to parse Json into a message
        /// </summary>
        public static JsonFormatReader CreateInstance(TextReader input)
        {
            return new JsonFormatReader(JsonCursor.CreateInstance(input));
        }

        /// <summary>
        /// Constructs a JsonFormatReader to parse Json into a message
        /// </summary>
        internal JsonFormatReader(JsonCursor input)
        {
            _input = input;
            _stopChar = new Stack<int>();
            _stopChar.Push(-1);
            _state = ReaderState.Start;
        }

        /// <summary>
        /// Constructs a JsonFormatReader to parse Json into a message
        /// </summary>
        protected JsonFormatReader(TextReader input)
            : this(JsonCursor.CreateInstance(input))
        {
        }

        /// <summary>
        /// Returns true if the reader is currently on an array element
        /// </summary>
        public bool IsArrayMessage
        {
            get { return _input.NextChar == '['; }
        }

        /// <summary>
        /// Returns an enumerator that is used to cursor over an array of messages
        /// </summary>
        /// <remarks>
        /// This is generally used when receiving an array of messages rather than a single root message
        /// </remarks>
        public IEnumerable<JsonFormatReader> EnumerateArray()
        {
            foreach (string ignored in ForeachArrayItem(_current))
            {
                yield return this;
            }
        }

        /// <summary>
        /// Reads the root-message preamble specific to this formatter
        /// </summary>
        public override void ReadMessageStart()
        {
            _input.Consume('{');
            _stopChar.Push('}');

            _state = ReaderState.BeginObject;
        }

        /// <summary>
        /// Reads the root-message close specific to this formatter
        /// </summary>
        public override void ReadMessageEnd()
        {
            _input.Consume((char)_stopChar.Pop());
            _state = ReaderState.EndValue;
        }

        /// <summary>
        /// Merges the contents of stream into the provided message builder
        /// </summary>
        public override TBuilder Merge<TBuilder>(TBuilder builder, ExtensionRegistry registry)
        {
            ReadMessageStart();
            builder.WeakMergeFrom(this, registry);
            ReadMessageEnd();
            return builder;
        }

        /// <summary>
        /// Causes the reader to skip past this field
        /// </summary>
        protected override void Skip()
        {
            object temp;
            _input.ReadVariant(out temp);
            _state = ReaderState.EndValue;
        }

        /// <summary>
        /// Peeks at the next field in the input stream and returns what information is available.
        /// </summary>
        /// <remarks>
        /// This may be called multiple times without actually reading the field.  Only after the field
        /// is either read, or skipped, should PeekNext return a different value.
        /// </remarks>
        protected override bool PeekNext(out string field)
        {
            field = _current;
            if (_state == ReaderState.BeginValue)
            {
                return true;
            }

            int next = _input.NextChar;
            if (next == _stopChar.Peek())
            {
                return false;
            }

            _input.Assert(next != -1, "Unexpected end of file.");

            //not sure about this yet, it will allow {, "a":true }
            if (_state == ReaderState.EndValue && !_input.TryConsume(','))
            {
                return false;
            }

            field = _current = _input.ReadString();
            _input.Consume(':');
            _state = ReaderState.BeginValue;
            return true;
        }

        /// <summary>
        /// Returns true if it was able to read a String from the input
        /// </summary>
        protected override bool ReadAsText(ref string value, Type typeInfo)
        {
            object temp;
            JsonCursor.JsType type = _input.ReadVariant(out temp);
            _state = ReaderState.EndValue;

            _input.Assert(type != JsonCursor.JsType.Array && type != JsonCursor.JsType.Object,
                          "Encountered {0} while expecting {1}", type, typeInfo);
            if (type == JsonCursor.JsType.Null)
            {
                return false;
            }
            if (type == JsonCursor.JsType.True)
            {
                value = "1";
            }
            else if (type == JsonCursor.JsType.False)
            {
                value = "0";
            }
            else
            {
                value = temp as string;
            }

            //exponent representation of integer number:
            if (value != null && type == JsonCursor.JsType.Number &&
                (typeInfo != typeof(double) && typeInfo != typeof(float)) &&
                value.IndexOf("e", StringComparison.OrdinalIgnoreCase) > 0)
            {
                value = XmlConvert.ToString((long) Math.Round(XmlConvert.ToDouble(value), 0));
            }
            return value != null;
        }

        /// <summary>
        /// Returns true if it was able to read a ByteString from the input
        /// </summary>
        protected override bool Read(ref ByteString value)
        {
            string bytes = null;
            if (Read(ref bytes))
            {
                value = ByteString.FromBase64(bytes);
                return true;
            }
            return false;
        }

        /// <summary>
        /// Cursors through the array elements and stops at the end of the array
        /// </summary>
        protected override IEnumerable<string> ForeachArrayItem(string field)
        {
            _input.Consume('[');
            _stopChar.Push(']');
            _state = ReaderState.BeginArray;
            while (_input.NextChar != ']')
            {
                _current = field;
                yield return field;
                if (!_input.TryConsume(','))
                {
                    break;
                }
            }
            _input.Consume((char) _stopChar.Pop());
            _state = ReaderState.EndValue;
        }

        /// <summary>
        /// Merges the input stream into the provided IBuilderLite 
        /// </summary>
        protected override bool ReadMessage(IBuilderLite builder, ExtensionRegistry registry)
        {
            Merge(builder, registry);
            return true;
        }
    }
}