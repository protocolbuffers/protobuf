using System;
using System.IO;
using System.Collections.Generic;
using Google.ProtocolBuffers.Collections;

namespace Google.ProtocolBuffers.Serialization.Http
{
    /// <summary>
    /// Defines control information for the various formatting used with HTTP services
    /// </summary>
    public class MessageFormatOptions
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

        /// <summary>The mime type for query strings and x-www-form-urlencoded content</summary>
        /// <remarks>This mime type is input-only</remarks>
        public const string ContentFormUrlEncoded = "application/x-www-form-urlencoded";

        /// <summary>
        /// Default mime-type handling for input
        /// </summary>
        private static readonly IDictionary<string, Converter<Stream, ICodedInputStream>> MimeInputDefaults =
            new ReadOnlyDictionary<string, Converter<Stream, ICodedInputStream>>(
            new Dictionary<string, Converter<Stream, ICodedInputStream>>(StringComparer.OrdinalIgnoreCase)
                {
                    {"application/json", JsonFormatReader.CreateInstance},
                    {"application/x-json", JsonFormatReader.CreateInstance},
                    {"application/x-javascript", JsonFormatReader.CreateInstance},
                    {"text/javascript", JsonFormatReader.CreateInstance},
                    {"text/x-javascript", JsonFormatReader.CreateInstance},
                    {"text/x-json", JsonFormatReader.CreateInstance},
                    {"text/json", JsonFormatReader.CreateInstance},
                    {"text/xml", XmlFormatReader.CreateInstance},
                    {"application/xml", XmlFormatReader.CreateInstance},
                    {"application/binary", CodedInputStream.CreateInstance},
                    {"application/x-protobuf", CodedInputStream.CreateInstance},
                    {"application/vnd.google.protobuf", CodedInputStream.CreateInstance},
                    {"application/x-www-form-urlencoded", FormUrlEncodedReader.CreateInstance},
                }
            );

        /// <summary>
        /// Default mime-type handling for output
        /// </summary>
        private static readonly IDictionary<string, Converter<Stream, ICodedOutputStream>> MimeOutputDefaults =
            new ReadOnlyDictionary<string, Converter<Stream, ICodedOutputStream>>(
            new Dictionary<string, Converter<Stream, ICodedOutputStream>>(StringComparer.OrdinalIgnoreCase)
                {
                    {"application/json", JsonFormatWriter.CreateInstance},
                    {"application/x-json", JsonFormatWriter.CreateInstance},
                    {"application/x-javascript", JsonFormatWriter.CreateInstance},
                    {"text/javascript", JsonFormatWriter.CreateInstance},
                    {"text/x-javascript", JsonFormatWriter.CreateInstance},
                    {"text/x-json", JsonFormatWriter.CreateInstance},
                    {"text/json", JsonFormatWriter.CreateInstance},
                    {"text/xml", XmlFormatWriter.CreateInstance},
                    {"application/xml", XmlFormatWriter.CreateInstance},
                    {"application/binary", CodedOutputStream.CreateInstance},
                    {"application/x-protobuf", CodedOutputStream.CreateInstance},
                    {"application/vnd.google.protobuf", CodedOutputStream.CreateInstance},
                }
            );




        private string _defaultContentType;
        private string _xmlReaderRootElementName;
        private string _xmlWriterRootElementName;
        private ExtensionRegistry _extensionRegistry;
        private Dictionary<string, Converter<Stream, ICodedInputStream>> _mimeInputTypes;
        private Dictionary<string, Converter<Stream, ICodedOutputStream>> _mimeOutputTypes;

        /// <summary> Provides access to modify the mime-type input stream construction </summary>
        public IDictionary<string, Converter<Stream, ICodedInputStream>> MimeInputTypes
        {
            get
            {
                return _mimeInputTypes ??
                    (_mimeInputTypes = new Dictionary<string, Converter<Stream, ICodedInputStream>>(
                                           MimeInputDefaults, StringComparer.OrdinalIgnoreCase));
            }
        }

        /// <summary> Provides access to modify the mime-type input stream construction </summary>
        public IDictionary<string, Converter<Stream, ICodedOutputStream>> MimeOutputTypes
        {
            get
            {
                return _mimeOutputTypes ??
                    (_mimeOutputTypes = new Dictionary<string, Converter<Stream, ICodedOutputStream>>(
                                           MimeOutputDefaults, StringComparer.OrdinalIgnoreCase));
            }
        }

        internal IDictionary<string, Converter<Stream, ICodedInputStream>> MimeInputTypesReadOnly
        { get { return _mimeInputTypes ?? MimeInputDefaults; } }

        internal IDictionary<string, Converter<Stream, ICodedOutputStream>> MimeOutputTypesReadOnly
        { get { return _mimeOutputTypes ?? MimeOutputDefaults; } }

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