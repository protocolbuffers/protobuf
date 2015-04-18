using System;
using System.IO;
using System.Text;

namespace Google.ProtocolBuffers.Serialization.Http
{
    /// <summary>
    /// Allows reading messages from a name/value dictionary
    /// </summary>
    public class FormUrlEncodedReader : AbstractTextReader
    {
        private readonly TextReader _input;
        private string _fieldName, _fieldValue;
        private bool _ready;

        /// <summary>
        /// Creates a dictionary reader from an enumeration of KeyValuePair data, like an IDictionary
        /// </summary>
        FormUrlEncodedReader(TextReader input)
        {
            _input = input;
            int ch = input.Peek();
            if (ch == '?')
            {
                input.Read();
            }
            _ready = ReadNext();
        }

        #region CreateInstance overloads
        /// <summary>
        /// Constructs a FormUrlEncodedReader to parse form data, or url query text into a message.
        /// </summary>
        public static FormUrlEncodedReader CreateInstance(Stream stream)
        {
            return new FormUrlEncodedReader(new StreamReader(stream, Encoding.UTF8, false));
        }

        /// <summary>
        /// Constructs a FormUrlEncodedReader to parse form data, or url query text into a message.
        /// </summary>
        public static FormUrlEncodedReader CreateInstance(byte[] bytes)
        {
            return new FormUrlEncodedReader(new StreamReader(new MemoryStream(bytes, false), Encoding.UTF8, false));
        }

        /// <summary>
        /// Constructs a FormUrlEncodedReader to parse form data, or url query text into a message.
        /// </summary>
        public static FormUrlEncodedReader CreateInstance(string text)
        {
            return new FormUrlEncodedReader(new StringReader(text));
        }

        /// <summary>
        /// Constructs a FormUrlEncodedReader to parse form data, or url query text into a message.
        /// </summary>
        public static FormUrlEncodedReader CreateInstance(TextReader input)
        {
            return new FormUrlEncodedReader(input);
        }
        #endregion

        private bool ReadNext()
        {
            StringBuilder field = new StringBuilder(32);
            StringBuilder value = new StringBuilder(64);
            int ch;
            while (-1 != (ch = _input.Read()) && ch != '=' && ch != '&')
            {
                field.Append((char)ch);
            }

            if (ch != -1 && ch != '&')
            {
                while (-1 != (ch = _input.Read()) && ch != '&')
                {
                    value.Append((char)ch);
                }
            }

            _fieldName = field.ToString();
            _fieldValue = Uri.UnescapeDataString(value.Replace('+', ' ').ToString());
            
            return !String.IsNullOrEmpty(_fieldName);
        }

        /// <summary>
        /// No-op
        /// </summary>
        public override void ReadMessageStart()
        { }

        /// <summary>
        /// No-op
        /// </summary>
        public override void ReadMessageEnd()
        { }

        /// <summary>
        /// Merges the contents of stream into the provided message builder
        /// </summary>
        public override TBuilder Merge<TBuilder>(TBuilder builder, ExtensionRegistry registry)
        {
            builder.WeakMergeFrom(this, registry);
            return builder;
        }

        /// <summary>
        /// Causes the reader to skip past this field
        /// </summary>
        protected override void Skip()
        {
            _ready = ReadNext();
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
            field = _ready ? _fieldName : null;
            return field != null;
        }

        /// <summary>
        /// Returns true if it was able to read a String from the input
        /// </summary>
        protected override bool ReadAsText(ref string value, Type typeInfo)
        {
            if (_ready)
            {
                value = _fieldValue;
                _ready = ReadNext();
                return true;
            }
            return false;
        }

        /// <summary>
        /// It's unlikely this will work for anything but text data as bytes UTF8 are transformed to text and back to bytes
        /// </summary>
        protected override ByteString DecodeBytes(string bytes)
        { return ByteString.CopyFromUtf8(bytes); }

        /// <summary>
        /// Not Supported
        /// </summary>
        public override bool ReadGroup(IBuilderLite value, ExtensionRegistry registry)
        { throw new NotSupportedException(); }

        /// <summary>
        /// Not Supported
        /// </summary>
        protected override bool ReadMessage(IBuilderLite builder, ExtensionRegistry registry)
        { throw new NotSupportedException(); }
    }
}