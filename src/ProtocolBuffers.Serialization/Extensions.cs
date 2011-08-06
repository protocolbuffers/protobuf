using System;
using System.Text;
using System.IO;
using System.Xml;
using Google.ProtocolBuffers.Serialization;

namespace Google.ProtocolBuffers
{
    /// <summary>
    /// Extension methods for using serializers on instances of IMessageLite/IBuilderLite
    /// </summary>
    public static class Extensions
    {
        #region IMessageLite Extension
        /// <summary>
        /// Serializes the message to JSON text.  This is a trivial wrapper
        /// around Serialization.JsonFormatWriter.WriteMessage.
        /// </summary>
        public static string ToJson(this IMessageLite message)
        {
            JsonFormatWriter w = JsonFormatWriter.CreateInstance();
            w.WriteMessage(message);
            return w.ToString();
        }
        /// <summary>
        /// Serializes the message to XML text.  This is a trivial wrapper
        /// around Serialization.XmlFormatWriter.WriteMessage.
        /// </summary>
        public static string ToXml(this IMessageLite message)
        {
            StringWriter w = new StringWriter(new StringBuilder(4096));
            XmlFormatWriter.CreateInstance(w).WriteMessage(message);
            return w.ToString();
        }
        /// <summary>
        /// Serializes the message to XML text using the element name provided.
        /// This is a trivial wrapper around Serialization.XmlFormatWriter.WriteMessage.
        /// </summary>
        public static string ToXml(this IMessageLite message, string rootElementName)
        {
            StringWriter w = new StringWriter(new StringBuilder(4096));
            XmlFormatWriter.CreateInstance(w).WriteMessage(rootElementName, message);
            return w.ToString();
        }

        #endregion
        #region IBuilderLite Extensions
        /// <summary>
        /// Merges a JSON object into this builder and returns
        /// </summary>
        public static TBuilder MergeFromJson<TBuilder>(this TBuilder builder, string jsonText) where TBuilder : IBuilderLite
        {
            return JsonFormatReader.CreateInstance(jsonText)
                .Merge(builder);
        }
        /// <summary>
        /// Merges a JSON object into this builder and returns
        /// </summary>
        public static TBuilder MergeFromJson<TBuilder>(this TBuilder builder, TextReader reader) where TBuilder : IBuilderLite
        {
            return MergeFromJson(builder, reader, ExtensionRegistry.Empty);
        }
        /// <summary>
        /// Merges a JSON object into this builder using the extensions provided and returns
        /// </summary>
        public static TBuilder MergeFromJson<TBuilder>(this TBuilder builder, TextReader reader, ExtensionRegistry extensionRegistry) where TBuilder : IBuilderLite
        {
            return JsonFormatReader.CreateInstance(reader)
                .Merge(builder, extensionRegistry);
        }

        /// <summary>
        /// Merges an XML object into this builder and returns
        /// </summary>
        public static TBuilder MergeFromXml<TBuilder>(this TBuilder builder, XmlReader reader) where TBuilder : IBuilderLite
        {
            return MergeFromXml(builder, XmlFormatReader.DefaultRootElementName, reader, ExtensionRegistry.Empty);
        }

        /// <summary>
        /// Merges an XML object into this builder and returns
        /// </summary>
        public static TBuilder MergeFromXml<TBuilder>(this TBuilder builder, string rootElementName, XmlReader reader) where TBuilder : IBuilderLite
        {
            return MergeFromXml(builder, rootElementName, reader, ExtensionRegistry.Empty);
        }

        /// <summary>
        /// Merges an XML object into this builder using the extensions provided and returns
        /// </summary>
        public static TBuilder MergeFromXml<TBuilder>(this TBuilder builder, string rootElementName, XmlReader reader,
                                            ExtensionRegistry extensionRegistry) where TBuilder : IBuilderLite
        {
            return XmlFormatReader.CreateInstance(reader)
                .Merge(rootElementName, builder, extensionRegistry);
        }

        #endregion
    }
}
