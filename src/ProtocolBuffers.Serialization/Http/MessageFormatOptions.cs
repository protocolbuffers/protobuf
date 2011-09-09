using System;

namespace Google.ProtocolBuffers.Serialization.Http
{
    /// <summary>
    /// Defines control information for the various formatting used with HTTP services
    /// </summary>
    public struct MessageFormatOptions
    {
        /// <summary>The mime type for xml content</summary>
        /// <remarks>Other valid xml mime types include: application/binary, application/x-protobuf</remarks>
        public const string ContentTypeProtoBuffer = "application/vnd.google.protobuf";

        /// <summary>The mime type for xml content</summary>
        /// <remarks>Other valid xml mime types include: text/xml</remarks>
        public const string ContentTypeXml = "application/xml";
        
        /// <summary>The mime type for json content</summary>
        /// <remarks>
        /// Other valid json mime types include: application/json, application/x-json, 
        /// application/x-javascript, text/javascript, text/x-javascript, text/x-json, text/json
        /// </remarks>
        public const string ContentTypeJson = "application/json";

        private string _defaultContentType;
        private string _xmlReaderRootElementName;
        private string _xmlWriterRootElementName;
        private ExtensionRegistry _extensionRegistry;

        /// <summary>
        /// The default content type to use if the input type is null or empty.  If this
        /// value is not supplied an ArgumentOutOfRangeException exception will be raised.
        /// </summary>
        public string DefaultContentType
        {
            get { return _defaultContentType ?? String.Empty; }
            set { _defaultContentType = value; }
        }

        /// <summary>
        /// The extension registry to use when reading messages
        /// </summary>
        public ExtensionRegistry ExtensionRegistry
        {
            get { return _extensionRegistry ?? ExtensionRegistry.Empty; }
            set { _extensionRegistry = value; }
        }

        /// <summary>
        /// The name of the xml root element when reading messages
        /// </summary>
        public string XmlReaderRootElementName
        {
            get { return _xmlReaderRootElementName ?? XmlFormatReader.DefaultRootElementName; }
            set { _xmlReaderRootElementName = value; }
        }

        /// <summary>
        /// Xml reader options
        /// </summary>
        public XmlReaderOptions XmlReaderOptions { get; set; }

        /// <summary>
        /// True to use formatted output including new-lines and default indentation
        /// </summary>
        public bool FormattedOutput { get; set; }

        /// <summary>
        /// The name of the xml root element when writing messages
        /// </summary>
        public string XmlWriterRootElementName
        {
            get { return _xmlWriterRootElementName ?? XmlFormatWriter.DefaultRootElementName; }
            set { _xmlWriterRootElementName = value; }
        }

        /// <summary>
        /// Xml writer options
        /// </summary>
        public XmlWriterOptions XmlWriterOptions { get; set; }
    }
}