using System;
using System.IO;
using System.Xml;
using System.Text;

namespace Google.ProtocolBuffers.Serialization
{
    /// <summary>
    /// Extensions and helpers to abstract the reading/writing of messages by a client-specified content type.
    /// </summary>
    public static class MessageFormatFactory
    {
        /// <summary>
        /// Constructs an ICodedInputStream from the input stream based on the contentType provided
        /// </summary>
        /// <param name="options">Options specific to reading this message and/or content type</param>
        /// <param name="contentType">The mime type of the input stream content</param>
        /// <param name="input">The stream to read the message from</param>
        /// <returns>The ICodedInputStream that can be given to the IBuilder.MergeFrom(...) method</returns>
        public static ICodedInputStream CreateInputStream(MessageFormatOptions options, string contentType, Stream input)
        {
            FormatType inputType = ContentTypeToFormat(contentType, options.DefaultContentType);

            ICodedInputStream codedInput;
            if (inputType == FormatType.ProtoBuffer)
            {
                codedInput = CodedInputStream.CreateInstance(input);
            }
            else if (inputType == FormatType.Json)
            {
                JsonFormatReader reader = JsonFormatReader.CreateInstance(input);
                codedInput = reader.ReadStartMessage();
            }
            else if (inputType == FormatType.Xml)
            {
                XmlFormatReader reader = XmlFormatReader.CreateInstance(input);
                reader.RootElementName = options.XmlReaderRootElementName;
                reader.Options = options.XmlReaderOptions;
                codedInput = reader.ReadStartMessage();
            }
            else
                throw new NotSupportedException();

            return codedInput;
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
        public static TBuilder MergeFrom<TBuilder>(this TBuilder builder, MessageFormatOptions options, string contentType, Stream input) where TBuilder : IBuilderLite
        {
            ICodedInputStream codedInput = CreateInputStream(options, contentType, input);
            return (TBuilder)builder.WeakMergeFrom(codedInput, options.ExtensionRegistry);
        }

        /// <summary>
        /// Writes the message instance to the stream using the content type provided
        /// </summary>
        /// <param name="message">An instance of a message</param>
        /// <param name="options">Options specific to writing this message and/or content type</param>
        /// <param name="contentType">The mime type of the content to be written</param>
        /// <param name="output">The stream to write the message to</param>
        public static void WriteTo(this IMessageLite message, MessageFormatOptions options, string contentType, Stream output)
        {
            FormatType outputType = ContentTypeToFormat(contentType, options.DefaultContentType);

            ICodedOutputStream codedOutput;
            if (outputType == FormatType.ProtoBuffer)
            {
                codedOutput = CodedOutputStream.CreateInstance(output);
            }
            else if (outputType == FormatType.Json)
            {
                JsonFormatWriter writer = JsonFormatWriter.CreateInstance(output);
                if (options.FormattedOutput)
                {
                    writer.Formatted();
                }
                writer.StartMessage();
                codedOutput = writer;
            }
            else if (outputType == FormatType.Xml)
            {
                XmlFormatWriter writer;
                if (options.FormattedOutput)
                {
                    writer = XmlFormatWriter.CreateInstance(output);
                }
                else
                {
                    XmlWriterSettings settings = new XmlWriterSettings()
                       {
                           CheckCharacters = false,
                           NewLineHandling = NewLineHandling.Entitize,
                           OmitXmlDeclaration = true,
                           Encoding = Encoding.UTF8,
                           Indent = true,
                           IndentChars = "  ",
                           NewLineChars = Environment.NewLine,
                       };
                    writer = XmlFormatWriter.CreateInstance(XmlWriter.Create(output, settings));
                }
                writer.RootElementName = options.XmlWriterRootElementName;
                writer.Options = options.XmlWriterOptions;
                writer.StartMessage();
                codedOutput = writer;
            }
            else
                throw new NotSupportedException();

            message.WriteTo(codedOutput);

            if (codedOutput is AbstractWriter)
                ((AbstractWriter) codedOutput).EndMessage();

            codedOutput.Flush();
        }


        enum FormatType { ProtoBuffer, Json, Xml };

        private static FormatType ContentTypeToFormat(string contentType, string defaultType)
        {
            switch ((contentType ?? String.Empty).Split(';')[0].Trim().ToLower())
            {
                case "application/json":
                case "application/x-json":
                case "application/x-javascript":
                case "text/javascript":
                case "text/x-javascript":
                case "text/x-json":
                case "text/json":
                    {
                        return FormatType.Json;
                    }

                case "text/xml":
                case "application/xml":
                    {
                        return FormatType.Xml;
                    }

                case "application/binary":
                case "application/x-protobuf":
                case "application/vnd.google.protobuf":
                    {
                        return FormatType.ProtoBuffer;
                    }

                case "":
                case null:
                    if (!String.IsNullOrEmpty(defaultType))
                    {
                        return ContentTypeToFormat(defaultType, null);
                    }
                    break;
            }

            throw new ArgumentOutOfRangeException("contentType");
        }
    }
}