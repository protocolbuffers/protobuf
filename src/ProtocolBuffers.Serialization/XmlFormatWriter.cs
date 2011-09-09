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
        public const string DefaultRootElementName = "root";
        private const int NestedArrayFlag = 0x0001;
        private readonly XmlWriter _output;
        private string _rootElementName;

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
            return new XmlFormatWriter(XmlWriter.Create(output, DefaultSettings(Encoding.UTF8)));
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
            _rootElementName = DefaultRootElementName;
        }

        /// <summary>
        /// Closes the underlying XmlTextWriter
        /// </summary>
        protected override void Dispose(bool disposing)
        {
            if (disposing)
            {
                if (_output.WriteState != WriteState.Closed && _output.WriteState != WriteState.Start)
                    _output.WriteEndDocument();

                _output.Close();
            }

            base.Dispose(disposing);
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
        /// complete the message with a call to EndMessage().
        /// </summary>
        public override void StartMessage()
        {
            StartMessage(_rootElementName);
        }

        /// <summary>
        /// Used to write the root-message preamble, in xml this is open element for elementName. 
        /// After this call you can call IMessageLite.MergeTo(...) and  complete the message with 
        /// a call to EndMessage().
        /// </summary>
        public void StartMessage(string elementName)
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
        }

        /// <summary>
        /// Used to complete a root-message previously started with a call to StartMessage()
        /// </summary>
        public override void EndMessage()
        {
            _output.WriteEndElement();
            _output.Flush();
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
            StartMessage(elementName);
            message.WriteTo(this);
            EndMessage();
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