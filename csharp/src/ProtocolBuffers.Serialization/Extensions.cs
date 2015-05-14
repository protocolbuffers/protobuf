using System;
using System.Text;
using System.IO;
using System.Xml;
using Google.ProtocolBuffers.Serialization;
using Google.ProtocolBuffers.Serialization.Http;

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
        public static string ToJson(
#if !NOEXTENSIONS
            this
#endif
            IMessageLite message)
        {
            JsonFormatWriter w = JsonFormatWriter.CreateInstance();
            w.WriteMessage(message);
            return w.ToString();
        }
        /// <summary>
        /// Serializes the message to XML text.  This is a trivial wrapper
        /// around Serialization.XmlFormatWriter.WriteMessage.
        /// </summary>
        public static string ToXml(
#if !NOEXTENSIONS
            this
#endif
            IMessageLite message)
        {
            StringWriter w = new StringWriter(new StringBuilder(4096));
            XmlFormatWriter.CreateInstance(w).WriteMessage(message);
            return w.ToString();
        }
        /// <summary>
        /// Serializes the message to XML text using the element name provided.
        /// This is a trivial wrapper around Serialization.XmlFormatWriter.WriteMessage.
        /// </summary>
        public static string ToXml(
#if !NOEXTENSIONS
            this
#endif
            IMessageLite message, string rootElementName)
        {
            StringWriter w = new StringWriter(new StringBuilder(4096));
            XmlFormatWriter.CreateInstance(w).WriteMessage(rootElementName, message);
            return w.ToString();
        }

        /// <summary>
        /// Writes the message instance to the stream using the content type provided
        /// </summary>
        /// <param name="message">An instance of a message</param>
        /// <param name="options">Options specific to writing this message and/or content type</param>
        /// <param name="contentType">The mime type of the content to be written</param>
        /// <param name="output">The stream to write the message to</param>
        public static void WriteTo(
#if !NOEXTENSIONS
            this
#endif
            IMessageLite message, MessageFormatOptions options, string contentType, Stream output)
        {
            ICodedOutputStream codedOutput = MessageFormatFactory.CreateOutputStream(options, contentType, output);

            // Output the appropriate message preamble
            codedOutput.WriteMessageStart();

            // Write the message content to the output
            message.WriteTo(codedOutput);

            // Write the closing message fragment
            codedOutput.WriteMessageEnd();
            codedOutput.Flush();
        }

        #endregion
        #region IBuilderLite Extensions
        /// <summary>
        /// Merges a JSON object into this builder and returns
        /// </summary>
        public static TBuilder MergeFromJson<TBuilder>(
#if !NOEXTENSIONS
            this
#endif
            TBuilder builder, string jsonText) where TBuilder : IBuilderLite
        {
            return JsonFormatReader.CreateInstance(jsonText)
                .Merge(builder);
        }
        /// <summary>
        /// Merges a JSON object into this builder and returns
        /// </summary>
        public static TBuilder MergeFromJson<TBuilder>(
#if !NOEXTENSIONS
            this
#endif
            TBuilder builder, TextReader reader) where TBuilder : IBuilderLite
        {
            return MergeFromJson(builder, reader, ExtensionRegistry.Empty);
        }
        /// <summary>
        /// Merges a JSON object into this builder using the extensions provided and returns
        /// </summary>
        public static TBuilder MergeFromJson<TBuilder>(
#if !NOEXTENSIONS
            this
#endif
            TBuilder builder, TextReader reader, ExtensionRegistry extensionRegistry) where TBuilder : IBuilderLite
        {
            return JsonFormatReader.CreateInstance(reader)
                .Merge(builder, extensionRegistry);
        }

        /// <summary>
        /// Merges an XML object into this builder and returns
        /// </summary>
        public static TBuilder MergeFromXml<TBuilder>(
#if !NOEXTENSIONS
            this
#endif
            TBuilder builder, XmlReader reader) where TBuilder : IBuilderLite
        {
            return MergeFromXml(builder, XmlFormatReader.DefaultRootElementName, reader, ExtensionRegistry.Empty);
        }

        /// <summary>
        /// Merges an XML object into this builder and returns
        /// </summary>
        public static TBuilder MergeFromXml<TBuilder>(
#if !NOEXTENSIONS
            this
#endif
            TBuilder builder, string rootElementName, XmlReader reader) where TBuilder : IBuilderLite
        {
            return MergeFromXml(builder, rootElementName, reader, ExtensionRegistry.Empty);
        }

        /// <summary>
        /// Merges an XML object into this builder using the extensions provided and returns
        /// </summary>
        public static TBuilder MergeFromXml<TBuilder>(
#if !NOEXTENSIONS
            this
#endif
            TBuilder builder, string rootElementName, XmlReader reader,
            ExtensionRegistry extensionRegistry) where TBuilder : IBuilderLite
        {
            return XmlFormatReader.CreateInstance(reader)
                .Merge(rootElementName, builder, extensionRegistry);
        }

        /// <summary>
        /// Merges the message from the input stream based on the contentType provided
        /// </summary>
        /// <typeparam name="TBuilder">A type derived from IBuilderLite</typeparam>
        /// <param name="builder">An instance of a message builder</param>
        /// <param name="options">Options specific to reading this message and/or content type</param>
        /// <param name="contentType">The mime type of the input stream content</param>
        /// <param name="input">The stream to read the message from</param>
        /// <returns>The same builder instance that was supplied in the builder parameter</returns>
        public static TBuilder MergeFrom<TBuilder>(
#if !NOEXTENSIONS
            this
#endif
            TBuilder builder, MessageFormatOptions options, string contentType, Stream input) where TBuilder : IBuilderLite
        {
            ICodedInputStream codedInput = MessageFormatFactory.CreateInputStream(options, contentType, input);
            codedInput.ReadMessageStart();
            builder.WeakMergeFrom(codedInput, options.ExtensionRegistry);
            codedInput.ReadMessageEnd();
            return builder;
        }

        #endregion
    }
}
