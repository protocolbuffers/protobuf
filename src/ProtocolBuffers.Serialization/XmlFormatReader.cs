using System;
using System.Collections.Generic;
using System.IO;
using System.Xml;
using System.Diagnostics;

namespace Google.ProtocolBuffers.Serialization
{
    /// <summary>
    /// Parses a proto buffer from an XML document or fragment.  .NET 3.5 users may also
    /// use this class to process Json by setting the options to support Json and providing
    /// an XmlReader obtained from <see cref="System.Runtime.Serialization.Json.JsonReaderWriterFactory"/>.
    /// </summary>
    public class XmlFormatReader : AbstractTextReader
    {
        public const string DefaultRootElementName = XmlFormatWriter.DefaultRootElementName;
        private readonly XmlReader _input;
        // Tracks the message element for each nested message read
        private readonly Stack<ElementStackEntry> _elements;
        // The default element name for ReadMessageStart
        private string _rootElementName;

        private struct ElementStackEntry
        {
            public readonly string LocalName;
            public readonly int Depth;
            public readonly bool IsEmpty;

            public ElementStackEntry(string localName, int depth, bool isEmpty) : this()
            {
                LocalName = localName;
                IsEmpty = isEmpty;
                Depth = depth;
            }
        }

        private static XmlReaderSettings DefaultSettings
        {
            get
            {
                return new XmlReaderSettings()
                           {CheckCharacters = false, IgnoreComments = true, IgnoreProcessingInstructions = true};
            }
        }

        /// <summary>
        /// Constructs the XmlFormatReader using the stream provided as the xml
        /// </summary>
        public static XmlFormatReader CreateInstance(byte[] input)
        {
            return new XmlFormatReader(XmlReader.Create(new MemoryStream(input, false), DefaultSettings));
        }

        /// <summary>
        /// Constructs the XmlFormatReader using the stream provided as the xml
        /// </summary>
        public static XmlFormatReader CreateInstance(Stream input)
        {
            return new XmlFormatReader(XmlReader.Create(input, DefaultSettings));
        }

        /// <summary>
        /// Constructs the XmlFormatReader using the string provided as the xml to be read
        /// </summary>
        public static XmlFormatReader CreateInstance(String input)
        {
            return new XmlFormatReader(XmlReader.Create(new StringReader(input), DefaultSettings));
        }

        /// <summary>
        /// Constructs the XmlFormatReader using the xml in the TextReader
        /// </summary>
        public static XmlFormatReader CreateInstance(TextReader input)
        {
            return new XmlFormatReader(XmlReader.Create(input, DefaultSettings));
        }

        /// <summary>
        /// Constructs the XmlFormatReader with the XmlReader
        /// </summary>
        public static XmlFormatReader CreateInstance(XmlReader input)
        {
            return new XmlFormatReader(input);
        }

        /// <summary>
        /// Constructs the XmlFormatReader with the XmlReader and options
        /// </summary>
        protected XmlFormatReader(XmlReader input)
        {
            _input = input;
            _rootElementName = DefaultRootElementName;
            _elements = new Stack<ElementStackEntry>();
            Options = XmlReaderOptions.None;
        }

        /// <summary>
        /// Gets or sets the options to use when reading the xml
        /// </summary>
        public XmlReaderOptions Options { get; set; }

        /// <summary>
        /// Sets the options to use while generating the XML
        /// </summary>
        public XmlFormatReader SetOptions(XmlReaderOptions options)
        {
            Options = options;
            return this;
        }

        /// <summary>
        /// Gets or sets the default element name to use when using the Merge&lt;TBuilder>()
        /// </summary>
        public string RootElementName
        {
            get { return _rootElementName; }
            set
            {
                ThrowHelper.ThrowIfNull(value, "RootElementName");
                _rootElementName = value;
            }
        }

        [DebuggerNonUserCode]
        private static void Assert(bool cond)
        {
            if (!cond)
            {
                throw new FormatException();
            }
        }

        /// <summary>
        /// Reads the root-message preamble specific to this formatter
        /// </summary>
        public override void ReadMessageStart()
        {
            ReadMessageStart(_rootElementName);
        }

        /// <summary>
        /// Reads the root-message preamble specific to this formatter
        /// </summary>
        public void ReadMessageStart(string element)
        {
            while (!_input.IsStartElement() && _input.Read())
            {
                continue;
            }
            Assert(_input.IsStartElement() && _input.LocalName == element);
            _elements.Push(new ElementStackEntry(element, _input.Depth, _input.IsEmptyElement));
            _input.Read();
        }

        /// <summary>
        /// Reads the root-message close specific to this formatter, MUST be called
        /// on the reader obtained from ReadMessageStart(string element).
        /// </summary>
        public override void ReadMessageEnd()
        {
            Assert(_elements.Count > 0);

            ElementStackEntry stop = _elements.Peek();
            while (_input.NodeType != XmlNodeType.EndElement && _input.NodeType != XmlNodeType.Element
                   && _input.Depth > stop.Depth && _input.Read())
            {
                continue;
            }

            if (!stop.IsEmpty)
            {
                Assert(_input.NodeType == XmlNodeType.EndElement
                       && _input.LocalName == stop.LocalName
                       && _input.Depth == stop.Depth);

                _input.Read();
            }
            _elements.Pop();
        }

        /// <summary>
        /// Merge the provided builder as an element named <see cref="RootElementName"/> in the current context
        /// </summary>
        public override TBuilder Merge<TBuilder>(TBuilder builder, ExtensionRegistry registry)
        {
            return Merge(_rootElementName, builder, registry);
        }

        /// <summary>
        /// Merge the provided builder as an element of the current context
        /// </summary>
        public TBuilder Merge<TBuilder>(string element, TBuilder builder) where TBuilder : IBuilderLite
        {
            return Merge(element, builder, ExtensionRegistry.Empty);
        }

        /// <summary>
        /// Merge the provided builder as an element of the current context
        /// </summary>
        public TBuilder Merge<TBuilder>(string element, TBuilder builder, ExtensionRegistry registry)
            where TBuilder : IBuilderLite
        {
            ReadMessageStart(element);
            builder.WeakMergeFrom(this, registry);
            ReadMessageEnd();
            return builder;
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
            ElementStackEntry stopNode;
            if (_elements.Count == 0)
            {
                stopNode = new ElementStackEntry(null, _input.Depth - 1, false);
            }
            else
            {
                stopNode = _elements.Peek();
            }

            if (!stopNode.IsEmpty)
            {
                while (!_input.IsStartElement() && _input.Depth > stopNode.Depth && _input.Read())
                {
                    continue;
                }

                if (_input.IsStartElement() && _input.Depth > stopNode.Depth)
                {
                    field = _input.LocalName;
                    return true;
                }
            }
            field = null;
            return false;
        }

        /// <summary>
        /// Causes the reader to skip past this field
        /// </summary>
        protected override void Skip()
        {
            if (_input.IsStartElement())
            {
                if (!_input.IsEmptyElement)
                {
                    int depth = _input.Depth;
                    while (_input.Depth >= depth && _input.NodeType != XmlNodeType.EndElement)
                    {
                        Assert(_input.Read());
                    }
                }
                _input.Read();
            }
        }

        /// <summary>
        /// returns true if it was able to read a single value into the value reference.  The value
        /// stored may be of type System.String, System.Int32, or an IEnumLite from the IEnumLiteMap.
        /// </summary>
        protected override bool ReadEnum(ref object value)
        {
            int number;
            string temp;
            if (null != (temp = _input.GetAttribute("value")) && FrameworkPortability.TryParseInt32(temp, out number))
            {
                Skip();
                value = number;
                return true;
            }
            return base.ReadEnum(ref value);
        }

        /// <summary>
        /// Returns true if it was able to read a String from the input
        /// </summary>
        protected override bool ReadAsText(ref string value, Type type)
        {
            Assert(_input.NodeType == XmlNodeType.Element);
            value = _input.ReadElementContentAsString();

            return true;
        }

        /// <summary>
        /// Merges the input stream into the provided IBuilderLite 
        /// </summary>
        protected override bool ReadMessage(IBuilderLite builder, ExtensionRegistry registry)
        {
            Assert(_input.IsStartElement());
            ReadMessageStart(_input.LocalName);
            builder.WeakMergeFrom(this, registry);
            ReadMessageEnd();
            return true;
        }

        private IEnumerable<string> NonNestedArrayItems(string field)
        {
            return base.ForeachArrayItem(field);
        }

        /// <summary>
        /// Cursors through the array elements and stops at the end of the array
        /// </summary>
        protected override IEnumerable<string> ForeachArrayItem(string field)
        {
            bool isNested = (Options & XmlReaderOptions.ReadNestedArrays) != 0;

            if (!isNested)
            {
                foreach (string item in NonNestedArrayItems(field))
                {
                    yield return item;
                }
            }
            else
            {
                string found;
                ReadMessageStart(field);
                if (PeekNext(out found) && found == "item")
                {
                    foreach (string item in NonNestedArrayItems("item"))
                    {
                        yield return item;
                    }
                }
                ReadMessageEnd();
            }
        }
    }
}