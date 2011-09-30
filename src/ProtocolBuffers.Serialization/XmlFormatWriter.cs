using System;
using System.Collections;
using System.IO;
using System.Text;
using System.Xml;
using Google.ProtocolBuffers.Descriptors;

namespace Google.ProtocolBuffers.Serialization
{
    /// <summary>
    /// Writes a proto buffer to an XML document or fragment.  .NET 3.5 users may also
    /// use this class to produce Json by setting the options to support Json and providing
    /// an XmlWriter obtained from <see cref="System.Runtime.Serialization.Json.JsonReaderWriterFactory"/>.
    /// </summary>
    public class XmlFormatWriter : AbstractTextWriter
    {
        private static readonly Encoding DefaultEncoding = new UTF8Encoding(false);
        public const string DefaultRootElementName = "root";

        private readonly XmlWriter _output;
        // The default element name used for WriteMessageStart
        private string _rootElementName;
        // Used to assert matching WriteMessageStart/WriteMessageEnd calls
        private int _messageOpenCount;

        private static XmlWriterSettings DefaultSettings(Encoding encoding)
        {
            return new XmlWriterSettings()
                       {
                           CheckCharacters = false,
                           NewLineHandling = NewLineHandling.Entitize,
                           OmitXmlDeclaration = true,
                           Encoding = encoding,
                       };
        }

        /// <summary>
        /// Constructs the XmlFormatWriter to write to the given TextWriter
        /// </summary>
        public static XmlFormatWriter CreateInstance(TextWriter output)
        {
            return new XmlFormatWriter(XmlWriter.Create(output, DefaultSettings(output.Encoding)));
        }

        /// <summary>
        /// Constructs the XmlFormatWriter to write to the given stream
        /// </summary>
        public static XmlFormatWriter CreateInstance(Stream output)
        {
            return new XmlFormatWriter(XmlWriter.Create(output, DefaultSettings(DefaultEncoding)));
        }

        /// <summary>
        /// Constructs the XmlFormatWriter to write to the given stream
        /// </summary>
        public static XmlFormatWriter CreateInstance(Stream output, Encoding encoding)
        {
            return new XmlFormatWriter(XmlWriter.Create(output, DefaultSettings(encoding)));
        }

        /// <summary>
        /// Constructs the XmlFormatWriter to write to the given XmlWriter
        /// </summary>
        public static XmlFormatWriter CreateInstance(XmlWriter output)
        {
            return new XmlFormatWriter(output);
        }

        protected XmlFormatWriter(XmlWriter output)
        {
            _output = output;
            _messageOpenCount = 0;
            _rootElementName = DefaultRootElementName;
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

        /// <summary>
        /// Gets or sets the options to use while generating the XML
        /// </summary>
        public XmlWriterOptions Options { get; set; }

        /// <summary>
        /// Sets the options to use while generating the XML
        /// </summary>
        public XmlFormatWriter SetOptions(XmlWriterOptions options)
        {
            Options = options;
            return this;
        }

        private bool TestOption(XmlWriterOptions option)
        {
            return (Options & option) != 0;
        }

        /// <summary>
        /// Completes any pending write operations
        /// </summary>
        public override void Flush()
        {
            _output.Flush();
            base.Flush();
        }

        /// <summary>
        /// Used to write the root-message preamble, in xml this is open element for RootElementName,
        /// by default "&lt;root&gt;". After this call you can call IMessageLite.MergeTo(...) and 
        /// complete the message with a call to WriteMessageEnd().
        /// </summary>
        public override void WriteMessageStart()
        {
            WriteMessageStart(_rootElementName);
        }

        /// <summary>
        /// Used to write the root-message preamble, in xml this is open element for elementName. 
        /// After this call you can call IMessageLite.MergeTo(...) and  complete the message with 
        /// a call to WriteMessageEnd().
        /// </summary>
        public void WriteMessageStart(string elementName)
        {
            if (TestOption(XmlWriterOptions.OutputJsonTypes))
            {
                _output.WriteStartElement("root"); // json requires this is the root-element
                _output.WriteAttributeString("type", "object");
            }
            else
            {
                _output.WriteStartElement(elementName);
            }
            _messageOpenCount++;
        }

        /// <summary>
        /// Used to complete a root-message previously started with a call to WriteMessageStart()
        /// </summary>
        public override void WriteMessageEnd()
        {
            if (_messageOpenCount <= 0)
            {
                throw new InvalidOperationException();
            }

            _output.WriteEndElement();
            _output.Flush();
            _messageOpenCount--;
        }

        /// <summary>
        /// Writes a message as an element using the name defined in <see cref="RootElementName"/>
        /// </summary>
        public override void WriteMessage(IMessageLite message)
        {
            WriteMessage(_rootElementName, message);
        }

        /// <summary>
        /// Writes a message as an element with the given name
        /// </summary>
        public void WriteMessage(string elementName, IMessageLite message)
        {
            WriteMessageStart(elementName);
            message.WriteTo(this);
            WriteMessageEnd();
        }

        /// <summary>
        /// Writes a message
        /// </summary>
        protected override void WriteMessageOrGroup(string field, IMessageLite message)
        {
            _output.WriteStartElement(field);

            if (TestOption(XmlWriterOptions.OutputJsonTypes))
            {
                _output.WriteAttributeString("type", "object");
            }

            message.WriteTo(this);
            _output.WriteEndElement();
        }

        /// <summary>
        /// Writes a String value
        /// </summary>
        protected override void WriteAsText(string field, string textValue, object typedValue)
        {
            _output.WriteStartElement(field);

            if (TestOption(XmlWriterOptions.OutputJsonTypes))
            {
                if (typedValue is int || typedValue is uint || typedValue is long || typedValue is ulong ||
                    typedValue is double || typedValue is float)
                {
                    _output.WriteAttributeString("type", "number");
                }
                else if (typedValue is bool)
                {
                    _output.WriteAttributeString("type", "boolean");
                }
            }
            _output.WriteString(textValue);

            //Empty strings should not be written as empty elements '<item/>', rather as '<item></item>'
            if (_output.WriteState == WriteState.Element)
            {
                _output.WriteRaw("");
            }

            _output.WriteEndElement();
        }

        /// <summary>
        /// Writes an array of field values
        /// </summary>
        protected override void WriteArray(FieldType fieldType, string field, IEnumerable items)
        {
            //see if it's empty
            IEnumerator eitems = items.GetEnumerator();
            try
            {
                if (!eitems.MoveNext())
                {
                    return;
                }
            }
            finally
            {
                if (eitems is IDisposable)
                {
                    ((IDisposable) eitems).Dispose();
                }
            }

            if (TestOption(XmlWriterOptions.OutputNestedArrays | XmlWriterOptions.OutputJsonTypes))
            {
                _output.WriteStartElement(field);
                if (TestOption(XmlWriterOptions.OutputJsonTypes))
                {
                    _output.WriteAttributeString("type", "array");
                }

                base.WriteArray(fieldType, "item", items);
                _output.WriteEndElement();
            }
            else
            {
                base.WriteArray(fieldType, field, items);
            }
        }

        /// <summary>
        /// Writes a System.Enum by the numeric and textual value
        /// </summary>
        protected override void WriteEnum(string field, int number, string name)
        {
            _output.WriteStartElement(field);

            if (!TestOption(XmlWriterOptions.OutputJsonTypes) && TestOption(XmlWriterOptions.OutputEnumValues))
            {
                _output.WriteAttributeString("value", XmlConvert.ToString(number));
            }

            _output.WriteString(name);
            _output.WriteEndElement();
        }
    }
}