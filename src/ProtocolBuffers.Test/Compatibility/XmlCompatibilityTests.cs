using System.IO;
using System.Xml;
using Google.ProtocolBuffers.Serialization;
using Google.ProtocolBuffers.TestProtos;
#if SILVERLIGHT
using TestClass = Microsoft.VisualStudio.TestTools.UnitTesting.TestClassAttribute;
using Test = Microsoft.VisualStudio.TestTools.UnitTesting.TestMethodAttribute;
using Assert = Microsoft.VisualStudio.TestTools.UnitTesting.Assert;
#else
using Microsoft.VisualStudio.TestTools.UnitTesting;
#endif


namespace Google.ProtocolBuffers.Compatibility
{
    [TestClass]
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

    [TestClass]
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