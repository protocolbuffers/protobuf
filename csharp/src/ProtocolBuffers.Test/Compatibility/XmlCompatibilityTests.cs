using System.IO;
using System.Xml;
using Google.ProtocolBuffers.Serialization;

namespace Google.ProtocolBuffers.Compatibility
{
    public class XmlCompatibilityTests : CompatibilityTests
    {
        protected override object SerializeMessage<TMessage, TBuilder>(TMessage message)
        {
            StringWriter text = new StringWriter();
            XmlFormatWriter writer = XmlFormatWriter.CreateInstance(text);
            writer.WriteMessage("root", message);
            return text.ToString();
        }

        protected override TBuilder DeserializeMessage<TMessage, TBuilder>(object message, TBuilder builder, ExtensionRegistry registry)
        {
            XmlFormatReader reader = XmlFormatReader.CreateInstance((string)message);
            return reader.Merge("root", builder, registry);
        }
    }

    public class XmlCompatibilityFormattedTests : CompatibilityTests
    {
        protected override object SerializeMessage<TMessage, TBuilder>(TMessage message)
        {
            StringWriter text = new StringWriter();
            XmlWriter xwtr = XmlWriter.Create(text, new XmlWriterSettings { Indent = true, IndentChars = "  " });

            XmlFormatWriter writer = XmlFormatWriter.CreateInstance(xwtr).SetOptions(XmlWriterOptions.OutputNestedArrays);
            writer.WriteMessage("root", message);
            return text.ToString();
        }

        protected override TBuilder DeserializeMessage<TMessage, TBuilder>(object message, TBuilder builder, ExtensionRegistry registry)
        {
            XmlFormatReader reader = XmlFormatReader.CreateInstance((string)message).SetOptions(XmlReaderOptions.ReadNestedArrays);
            return reader.Merge("root", builder, registry);
        }
    }
}